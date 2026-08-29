#include "demo_mapShanmenControlledWeaponRunCommandRouter.h"

namespace
{
	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return Left.ToString(EGuidFormats::Digits)
			< Right.ToString(EGuidFormats::Digits);
	}

	bool IsCommandKind(EShanmenControlledWeaponCommandKind Kind)
	{
		return Kind == EShanmenControlledWeaponCommandKind::Launch
			|| Kind == EShanmenControlledWeaponCommandKind::Redirect
			|| Kind == EShanmenControlledWeaponCommandKind::Recall;
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}
}

bool Fdemo_mapShanmenControlledWeaponRunCommandIntent::TryCapture(
	const FGuid& RequestedIntentId,
	const FGuid& RequestedRunId,
	EShanmenControlledWeaponCommandKind RequestedKind,
	const TArray<FGuid>& RequestedTargetItemInstanceIds,
	const FVector& RequestedDirection,
	Fdemo_mapShanmenControlledWeaponRunCommandIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenControlledWeaponRunCommandIntent();
	if (!RequestedIntentId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !IsCommandKind(RequestedKind)
		|| RequestedTargetItemInstanceIds.IsEmpty())
	{
		return false;
	}

	TSet<FGuid> UniqueTargets;
	for (const FGuid& ItemInstanceId : RequestedTargetItemInstanceIds)
	{
		if (!ItemInstanceId.IsValid()
			|| UniqueTargets.Contains(ItemInstanceId))
		{
			return false;
		}
		UniqueTargets.Add(ItemInstanceId);
	}

	FVector FrozenDirection = FVector::ZeroVector;
	if (RequestedKind == EShanmenControlledWeaponCommandKind::Recall)
	{
		if (!IsFiniteVector(RequestedDirection)
			|| !RequestedDirection.IsNearlyZero())
		{
			return false;
		}
	}
	else
	{
		if (!IsFiniteVector(RequestedDirection)
			|| RequestedDirection.IsNearlyZero())
		{
			return false;
		}
		FrozenDirection = RequestedDirection.GetSafeNormal();
		if (!FrozenDirection.IsNormalized())
		{
			return false;
		}
	}

	OutIntent.IntentId = RequestedIntentId;
	OutIntent.RunId = RequestedRunId;
	OutIntent.Kind = RequestedKind;
	OutIntent.TargetItemInstanceIds = RequestedTargetItemInstanceIds;
	OutIntent.TargetItemInstanceIds.Sort(GuidLess);
	OutIntent.DesiredDirection = FrozenDirection;
	return OutIntent.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponRunCommandIntent::IsValid() const
{
	if (!IntentId.IsValid()
		|| !RunId.IsValid()
		|| !IsCommandKind(Kind)
		|| TargetItemInstanceIds.IsEmpty())
	{
		return false;
	}
	for (int32 Index = 0; Index < TargetItemInstanceIds.Num(); ++Index)
	{
		if (!TargetItemInstanceIds[Index].IsValid()
			|| (Index > 0
				&& !GuidLess(
					TargetItemInstanceIds[Index - 1],
					TargetItemInstanceIds[Index])))
		{
			return false;
		}
	}
	if (!IsFiniteVector(DesiredDirection))
	{
		return false;
	}
	return Kind == EShanmenControlledWeaponCommandKind::Recall
		? DesiredDirection.IsNearlyZero()
		: DesiredDirection.IsNormalized();
}

bool Fdemo_mapShanmenControlledWeaponRunCommandIntent::Matches(
	const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& IntentId == Other.IntentId
		&& RunId == Other.RunId
		&& Kind == Other.Kind
		&& TargetItemInstanceIds == Other.TargetItemInstanceIds
		&& DesiredDirection == Other.DesiredDirection;
}

bool Fdemo_mapShanmenControlledWeaponRunCommandEntry::IsValidFor(
	EShanmenControlledWeaponCommandKind Kind) const
{
	if (!ItemInstanceId.IsValid()
		|| ExpectedSequence < 0
		|| !Command.IsValid()
		|| Command.GetSourceItemInstanceId() != ItemInstanceId
		|| Command.GetSequence() != ExpectedSequence
		|| Command.GetKind() != Kind)
	{
		return false;
	}
	if (Kind != EShanmenControlledWeaponCommandKind::Recall)
	{
		return !Recovery.IsValid() && !Completed.IsValid();
	}
	return Recovery.IsValid()
		&& Completed.IsValid()
		&& Recovery.GetActivationId() == Command.GetActivationId()
		&& Completed.GetActivationId() == Command.GetActivationId()
		&& Recovery.GetFromPhase() == EShanmenCombatActionPhase::Active
		&& Recovery.GetToPhase() == EShanmenCombatActionPhase::Recovery
		&& Completed.GetFromPhase() == EShanmenCombatActionPhase::Recovery
		&& Completed.GetToPhase() == EShanmenCombatActionPhase::Idle
		&& Completed.GetTerminalReason()
			== EShanmenActionTerminalReason::Completed;
}

bool Fdemo_mapShanmenControlledWeaponRunCommandResult::IsAccepted() const
{
	if ((Status != Edemo_mapShanmenControlledWeaponRunCommandStatus::Applied
			&& Status
				!= Edemo_mapShanmenControlledWeaponRunCommandStatus::Replayed)
		|| !IntentId.IsValid()
		|| !RunId.IsValid()
		|| !IsCommandKind(Kind)
		|| TargetCount <= 0
		|| Entries.Num() != TargetCount)
	{
		return false;
	}
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (!Entries[Index].IsValidFor(Kind)
			|| (Index > 0
				&& !GuidLess(
					Entries[Index - 1].ItemInstanceId,
					Entries[Index].ItemInstanceId)))
		{
			return false;
		}
	}
	return true;
}

Fdemo_mapShanmenControlledWeaponRunCommandResult
Fdemo_mapShanmenControlledWeaponRunCommandRouter::TryRoute(
	Fdemo_mapShanmenControlledWeaponRunHost& Host,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Intent)
{
	Fdemo_mapShanmenControlledWeaponRunCommandResult Result;
	Result.IntentId = Intent.GetIntentId();
	Result.RunId = Intent.GetRunId();
	Result.Kind = Intent.GetKind();
	Result.TargetCount = Intent.GetTargetItemInstanceIds().Num();
	if (!Coordinator.IsReady())
	{
		Result.Diagnostic =
			TEXT("Controlled-weapon command requires one ready Coordinator Run.");
		return Result;
	}
	if (!Intent.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::IntentInvalid;
		Result.Diagnostic = TEXT("Controlled-weapon command intent is invalid.");
		return Result;
	}
	if (!Host.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::HostInvalid;
		Result.Diagnostic = TEXT("Controlled-weapon command Host is invalid.");
		return Result;
	}
	if (Intent.GetRunId() != Coordinator.GetRunId()
		|| Intent.GetRunId() != Host.GetRunId()
		|| Host.GetSourceEntityId() != Coordinator.GetPlayerEntityId())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::RunMismatch;
		Result.Diagnostic =
			TEXT("Controlled-weapon command identities do not name one Run.");
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::RouterInvalid;
		Result.Diagnostic = TEXT("Controlled-weapon command Router is invalid.");
		return Result;
	}
	if (!IsEmpty() && RunId != Intent.GetRunId())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::RouterRunMismatch;
		Result.Diagnostic =
			TEXT("Controlled-weapon command Router belongs to another Run.");
		return Result;
	}

	if (const FProcessedIntent* Existing =
		ProcessedIntents.Find(Intent.GetIntentId()))
	{
		if (!Existing->Intent.Matches(Intent))
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunCommandStatus::IntentIdConflict;
			Result.Diagnostic =
				TEXT("Controlled-weapon IntentId was reused with another payload.");
			return Result;
		}
		Result = Existing->Result;
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::Replayed;
		Result.Diagnostic =
			TEXT("Controlled-weapon command returned its accepted receipts.");
		return Result;
	}

	for (const FGuid& ItemInstanceId : Intent.GetTargetItemInstanceIds())
	{
		if (!Host.FindController(ItemInstanceId))
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunCommandStatus::TargetNotBound;
			Result.Diagnostic =
				TEXT("Controlled-weapon command targets an unbound exact item.");
			return Result;
		}
	}

	Fdemo_mapShanmenControlledWeaponRunHost HostCandidate = Host;
	Result.Entries.Reserve(Result.TargetCount);
	for (const FGuid& ItemInstanceId : Intent.GetTargetItemInstanceIds())
	{
		const Fdemo_mapShanmenControlledWeaponProductController* Controller =
			HostCandidate.FindController(ItemInstanceId);
		Fdemo_mapShanmenControlledWeaponRunCommandEntry Entry;
		Entry.ItemInstanceId = ItemInstanceId;
		Entry.ExpectedSequence = Controller
			? Controller->GetSession().GetExecution()
				.GetNextCommandSequence()
			: INDEX_NONE;
		bool bAccepted = false;
		switch (Intent.GetKind())
		{
		case EShanmenControlledWeaponCommandKind::Launch:
			bAccepted = HostCandidate.TryLaunch(
				ItemInstanceId,
				Entry.ExpectedSequence,
				Intent.GetDesiredDirection(),
				Entry.Command);
			break;
		case EShanmenControlledWeaponCommandKind::Redirect:
			bAccepted = HostCandidate.TryRedirect(
				ItemInstanceId,
				Entry.ExpectedSequence,
				Intent.GetDesiredDirection(),
				Entry.Command);
			break;
		case EShanmenControlledWeaponCommandKind::Recall:
			bAccepted = HostCandidate.TryRecallAndComplete(
				ItemInstanceId,
				Entry.ExpectedSequence,
				Entry.Command,
				Entry.Recovery,
				Entry.Completed);
			break;
		}
		if (!bAccepted || !Entry.IsValidFor(Intent.GetKind()))
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunCommandStatus::CommandRejected;
			Result.Entries.Reset();
			Result.Diagnostic = FString::Printf(
				TEXT("Controlled-weapon item %s rejected the atomic command."),
				*ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens));
			return Result;
		}
		Result.Entries.Add(MoveTemp(Entry));
	}

	Result.Status =
		Edemo_mapShanmenControlledWeaponRunCommandStatus::Applied;
	Result.Diagnostic =
		TEXT("Controlled-weapon command committed in stable item order.");
	if (!Result.IsAccepted() || !HostCandidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::CommandRejected;
		Result.Entries.Reset();
		Result.Diagnostic =
			TEXT("Controlled-weapon command produced invalid staged state.");
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponRunCommandRouter RouterCandidate = *this;
	if (RouterCandidate.IsEmpty())
	{
		RouterCandidate.RunId = Intent.GetRunId();
	}
	FProcessedIntent Processed;
	Processed.Intent = Intent;
	Processed.Result = Result;
	RouterCandidate.ProcessedIntents.Add(Intent.GetIntentId(), MoveTemp(Processed));
	if (!RouterCandidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::RouterInvalid;
		Result.Entries.Reset();
		Result.Diagnostic =
			TEXT("Controlled-weapon command could not commit its replay record.");
		return Result;
	}

	Host = MoveTemp(HostCandidate);
	*this = MoveTemp(RouterCandidate);
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponRunCommandRouter::IsValid() const
{
	if (ProcessedIntents.IsEmpty())
	{
		return !RunId.IsValid();
	}
	if (!RunId.IsValid())
	{
		return false;
	}
	for (const TPair<FGuid, FProcessedIntent>& Pair : ProcessedIntents)
	{
		const FProcessedIntent& Processed = Pair.Value;
		if (Pair.Key != Processed.Intent.GetIntentId()
			|| !Processed.Intent.IsValid()
			|| Processed.Intent.GetRunId() != RunId
			|| Processed.Result.Status
				!= Edemo_mapShanmenControlledWeaponRunCommandStatus::Applied
			|| !Processed.Result.IsAccepted()
			|| Processed.Result.IntentId != Pair.Key
			|| Processed.Result.RunId != RunId
			|| Processed.Result.Kind != Processed.Intent.GetKind()
			|| Processed.Result.TargetCount
				!= Processed.Intent.GetTargetItemInstanceIds().Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Processed.Result.Entries.Num(); ++Index)
		{
			if (Processed.Result.Entries[Index].ItemInstanceId
				!= Processed.Intent.GetTargetItemInstanceIds()[Index])
			{
				return false;
			}
		}
	}
	return true;
}

void Fdemo_mapShanmenControlledWeaponRunCommandRouter::Reset()
{
	*this = Fdemo_mapShanmenControlledWeaponRunCommandRouter();
}
