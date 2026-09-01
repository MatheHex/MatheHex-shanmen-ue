#include "demo_mapShanmenCombatConditionComponent.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsAcceptedApplyMutation(
		const Edemo_mapExactModifierMutationStatus Status)
	{
		return Status == Edemo_mapExactModifierMutationStatus::Applied
			|| Status == Edemo_mapExactModifierMutationStatus::ApplyReplayed;
	}

	bool IsAcceptedRemoveMutation(
		const Edemo_mapExactModifierMutationStatus Status)
	{
		return Status == Edemo_mapExactModifierMutationStatus::Removed
			|| Status == Edemo_mapExactModifierMutationStatus::RemoveReplayed;
	}
}

bool Fdemo_mapShanmenCombatConditionApplicationReceipt::IsValid() const
{
	return ApplicationId.IsValid()
		&& RunId.IsValid()
		&& TargetEntityId.IsValid()
		&& TimelineId.IsValid()
		&& ImpactId.IsValid()
		&& ResolutionId.IsValid()
		&& !DefinitionId.IsNone()
		&& DefinitionId
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& AppliedAtTick >= 0
		&& ExpiryTick > AppliedAtTick
		&& ConditionRevision > 0
		&& ApplicationId
			== Udemo_mapShanmenCombatConditionComponent::MakeApplicationId(
				RunId,
				TargetEntityId,
				TimelineId,
				ImpactId,
				ResolutionId);
}

Udemo_mapShanmenCombatConditionComponent::
	Udemo_mapShanmenCombatConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void Udemo_mapShanmenCombatConditionComponent::OnComponentDestroyed(
	const bool bDestroyingHierarchy)
{
	Reset();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

FName Udemo_mapShanmenCombatConditionComponent::MeridianShockDefinitionId()
{
	static const FName Value(TEXT("Condition.Injury.MeridianShock.Minor.r1"));
	return Value;
}

FName
Udemo_mapShanmenCombatConditionComponent::MeridianShockModifierSourceId()
{
	static const FName Value(
		TEXT("Condition.Injury.MeridianShock.Minor.MoveSpeed.r1"));
	return Value;
}

FGuid Udemo_mapShanmenCombatConditionComponent::MakeApplicationId(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const FGuid& ImpactId,
	const FGuid& ResolutionId)
{
	if (!RequestedRunId.IsValid()
		|| !RequestedTargetEntityId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| !ImpactId.IsValid()
		|| !ResolutionId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.Condition.Application.r1"),
		{
			GuidDigits(RequestedRunId),
			GuidDigits(RequestedTargetEntityId),
			GuidDigits(RequestedTimelineId),
			GuidDigits(ImpactId),
			GuidDigits(ResolutionId),
			MeridianShockDefinitionId().ToString()
		});
}

Fdemo_mapModifierHandle
Udemo_mapShanmenCombatConditionComponent::MakeMeridianShockModifierHandle(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId)
{
	Fdemo_mapModifierHandle Handle;
	if (RequestedRunId.IsValid() && RequestedTargetEntityId.IsValid())
	{
		Handle.Value = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.Modifier.r1"),
			{
				GuidDigits(RequestedRunId),
				GuidDigits(RequestedTargetEntityId),
				MeridianShockDefinitionId().ToString(),
				Fdemo_mapAttributeIds::MoveSpeed.ToString()
			});
	}
	return Handle;
}

Fdemo_mapModifierSpec
Udemo_mapShanmenCombatConditionComponent::MakeMeridianShockModifierSpec()
{
	Fdemo_mapModifierSpec Spec;
	Spec.SourceId = MeridianShockModifierSourceId();
	Spec.AttributeId = Fdemo_mapAttributeIds::MoveSpeed;
	Spec.Operation = Edemo_mapModifierOperation::Multiply;
	Spec.Value = MeridianShockMoveSpeedMultiplier();
	Spec.Priority = 500;
	return Spec;
}

bool Udemo_mapShanmenCombatConditionComponent::TryBegin(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	Udemo_mapAttributeComponent* RequestedAttributeComponent,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedTargetEntityId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| RequestedTimelineId
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				RequestedRunId)
		|| !::IsValid(RequestedAttributeComponent)
		|| !RequestedAttributeComponent->IsAttributeRegistered(
			Fdemo_mapAttributeIds::MoveSpeed))
	{
		OutDiagnostic =
			TEXT("Combat condition begin requires valid Run, target, canonical timeline and attribute authority.");
		return false;
	}
	if (!IsEmpty())
	{
		if (RunId == RequestedRunId
			&& TargetEntityId == RequestedTargetEntityId
			&& TimelineId == RequestedTimelineId
			&& AttributeComponent.Get() == RequestedAttributeComponent)
		{
			OutDiagnostic =
				TEXT("Combat condition authority is already active for this Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("Combat condition authority rejects a second active Run.");
		return false;
	}

	const Fdemo_mapModifierHandle CandidateHandle =
		MakeMeridianShockModifierHandle(
			RequestedRunId,
			RequestedTargetEntityId);
	if (!CandidateHandle.IsValid()
		|| !IsAcceptedRemoveMutation(
			RequestedAttributeComponent->EnsureModifierRemoved(
				MakeMeridianShockModifierSpec(),
				CandidateHandle)))
	{
		OutDiagnostic =
			TEXT("Combat condition begin could not establish an empty exact modifier slot.");
		return false;
	}

	RunId = RequestedRunId;
	TargetEntityId = RequestedTargetEntityId;
	TimelineId = RequestedTimelineId;
	AttributeComponent = RequestedAttributeComponent;
	MeridianShockModifierHandle = CandidateHandle;
	LastObservedTick = 0;
	MeridianShockExpiryTick = INDEX_NONE;
	ConditionRevision = 0;
	bMeridianShockActive = false;
	ProcessedApplications.Reset();
	OutDiagnostic =
		TEXT("Combat condition authority began empty at canonical tick zero.");
	return IsValid();
}

Fdemo_mapShanmenCombatConditionAdvanceResult
Udemo_mapShanmenCombatConditionComponent::TryAdvance(
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	Fdemo_mapShanmenCombatConditionAdvanceResult Result;
	Result.ConditionRevision = ConditionRevision;
	if (!IsValid() || IsEmpty() || !AttributeComponent.IsValid())
	{
		return Result;
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId() != TimelineId)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::TimelineMismatch;
		return Result;
	}
	if (TimelineSample.GetCurrentTick() < LastObservedTick)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::StaleTimeline;
		return Result;
	}

	const int64 ObservedTick = TimelineSample.GetCurrentTick();
	if (!bMeridianShockActive || ObservedTick < MeridianShockExpiryTick)
	{
		LastObservedTick = ObservedTick;
		Result.Status =
			Edemo_mapShanmenCombatConditionAdvanceStatus::Observed;
		Result.Error = Edemo_mapShanmenCombatConditionError::None;
		Result.ObservedTick = LastObservedTick;
		return Result;
	}
	if (ConditionRevision == MAX_int64)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::RevisionExhausted;
		return Result;
	}
	if (!IsAcceptedRemoveMutation(
			AttributeComponent->EnsureModifierRemoved(
				MakeMeridianShockModifierSpec(),
				MeridianShockModifierHandle)))
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::ModifierRejected;
		return Result;
	}

	bMeridianShockActive = false;
	MeridianShockExpiryTick = INDEX_NONE;
	LastObservedTick = ObservedTick;
	++ConditionRevision;
	Result.Status = Edemo_mapShanmenCombatConditionAdvanceStatus::Expired;
	Result.Error = Edemo_mapShanmenCombatConditionError::None;
	Result.ObservedTick = LastObservedTick;
	Result.ConditionRevision = ConditionRevision;
	return Result;
}

Fdemo_mapShanmenCombatConditionApplicationResult
Udemo_mapShanmenCombatConditionComponent::TryApplyMeridianShock(
	const FShanmenVitalityCommitReceipt& VitalityReceipt,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	Fdemo_mapShanmenCombatConditionApplicationResult Result;
	if (!IsValid() || IsEmpty() || !AttributeComponent.IsValid())
	{
		return Result;
	}
	if (!VitalityReceipt.IsValid())
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::InvalidReceipt;
		return Result;
	}
	if (VitalityReceipt.GetTargetEntityId() != TargetEntityId)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::TargetMismatch;
		return Result;
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId() != TimelineId)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::TimelineMismatch;
		return Result;
	}
	if (TimelineSample.GetCurrentTick() < LastObservedTick)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::StaleTimeline;
		return Result;
	}

	const Fdemo_mapShanmenCombatConditionAdvanceResult Advance =
		TryAdvance(TimelineSample);
	if (!Advance.IsSuccess())
	{
		Result.Error = Advance.Error;
		return Result;
	}
	if (VitalityReceipt.GetAppliedDamage() <= 0.0f)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::NoCommittedDamage;
		return Result;
	}
	if (const FProcessedApplication* Existing =
		ProcessedApplications.Find(VitalityReceipt.GetImpactId()))
	{
		if (Existing->ResolutionId != VitalityReceipt.GetResolutionId())
		{
			Result.Error =
				Edemo_mapShanmenCombatConditionError::ImpactConflict;
			return Result;
		}
		Result.Status =
			Edemo_mapShanmenCombatConditionApplicationStatus::AlreadyApplied;
		Result.Error = Edemo_mapShanmenCombatConditionError::None;
		Result.Receipt = Existing->Receipt;
		return Result;
	}

	const int64 AppliedAtTick = TimelineSample.GetCurrentTick();
	if (AppliedAtTick > MAX_int64 - MeridianShockDurationTicks()
		|| ConditionRevision == MAX_int64)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::RevisionExhausted;
		return Result;
	}
	const int64 CandidateExpiry =
		AppliedAtTick + MeridianShockDurationTicks();
	const int64 EffectiveExpiry = bMeridianShockActive
		? FMath::Max(MeridianShockExpiryTick, CandidateExpiry)
		: CandidateExpiry;
	const int64 CandidateRevision = ConditionRevision + 1;

	Fdemo_mapShanmenCombatConditionApplicationReceipt Receipt;
	Receipt.RunId = RunId;
	Receipt.TargetEntityId = TargetEntityId;
	Receipt.TimelineId = TimelineId;
	Receipt.ImpactId = VitalityReceipt.GetImpactId();
	Receipt.ResolutionId = VitalityReceipt.GetResolutionId();
	Receipt.DefinitionId = MeridianShockDefinitionId();
	Receipt.AppliedAtTick = AppliedAtTick;
	Receipt.ExpiryTick = EffectiveExpiry;
	Receipt.ConditionRevision = CandidateRevision;
	Receipt.ApplicationId = MakeApplicationId(
		RunId,
		TargetEntityId,
		TimelineId,
		Receipt.ImpactId,
		Receipt.ResolutionId);
	if (!Receipt.IsValid())
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::InvalidReceipt;
		return Result;
	}

	const bool bWasActive = bMeridianShockActive;
	if (!IsAcceptedApplyMutation(
			AttributeComponent->EnsureModifierApplied(
				MakeMeridianShockModifierSpec(),
				MeridianShockModifierHandle)))
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::ModifierRejected;
		return Result;
	}

	bMeridianShockActive = true;
	MeridianShockExpiryTick = EffectiveExpiry;
	ConditionRevision = CandidateRevision;
	FProcessedApplication& Processed =
		ProcessedApplications.Add(Receipt.ImpactId);
	Processed.ResolutionId = Receipt.ResolutionId;
	Processed.Receipt = Receipt;
	Result.Status = bWasActive
		? Edemo_mapShanmenCombatConditionApplicationStatus::Refreshed
		: Edemo_mapShanmenCombatConditionApplicationStatus::Applied;
	Result.Error = Edemo_mapShanmenCombatConditionError::None;
	Result.Receipt = Receipt;
	return Result;
}

bool Udemo_mapShanmenCombatConditionComponent::
	TryCaptureMeridianShockStatus(
		Fdemo_mapShanmenCombatConditionStatusSnapshot& OutStatus) const
{
	OutStatus = Fdemo_mapShanmenCombatConditionStatusSnapshot();
	return IsValid()
		&& !IsEmpty()
		&& Fdemo_mapShanmenCombatConditionStatusSnapshot::TryCapture(
			RunId,
			TargetEntityId,
			TimelineId,
			LastObservedTick,
			MeridianShockExpiryTick,
			ConditionRevision,
			bMeridianShockActive,
			OutStatus);
}

bool Udemo_mapShanmenCombatConditionComponent::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ExpectedRunId.IsValid() || !IsValid())
	{
		OutDiagnostic =
			TEXT("Combat condition end requires valid state and Run identity.");
		return false;
	}
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Combat condition authority is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Combat condition authority rejects mismatched Run teardown.");
		return false;
	}
	if (bMeridianShockActive
		&& (!AttributeComponent.IsValid()
			|| !IsAcceptedRemoveMutation(
				AttributeComponent->EnsureModifierRemoved(
					MakeMeridianShockModifierSpec(),
					MeridianShockModifierHandle))))
	{
		OutDiagnostic =
			TEXT("Combat condition teardown could not remove its exact modifier.");
		return false;
	}
	ClearState();
	OutDiagnostic =
		TEXT("Combat condition authority ended and removed Run projections.");
	return true;
}

void Udemo_mapShanmenCombatConditionComponent::Reset()
{
	if (bMeridianShockActive && AttributeComponent.IsValid())
	{
		AttributeComponent->EnsureModifierRemoved(
			MakeMeridianShockModifierSpec(),
			MeridianShockModifierHandle);
	}
	ClearState();
}

void Udemo_mapShanmenCombatConditionComponent::ClearState()
{
	RunId.Invalidate();
	TargetEntityId.Invalidate();
	TimelineId.Invalidate();
	AttributeComponent.Reset();
	MeridianShockModifierHandle.Reset();
	LastObservedTick = INDEX_NONE;
	MeridianShockExpiryTick = INDEX_NONE;
	ConditionRevision = 0;
	bMeridianShockActive = false;
	ProcessedApplications.Reset();
}

bool Udemo_mapShanmenCombatConditionComponent::IsValid() const
{
	if (!RunId.IsValid()
		&& !TargetEntityId.IsValid()
		&& !TimelineId.IsValid())
	{
		return !AttributeComponent.IsValid()
			&& !MeridianShockModifierHandle.IsValid()
			&& LastObservedTick == INDEX_NONE
			&& MeridianShockExpiryTick == INDEX_NONE
			&& ConditionRevision == 0
			&& !bMeridianShockActive
			&& ProcessedApplications.IsEmpty();
	}

	if (!RunId.IsValid()
		|| !TargetEntityId.IsValid()
		|| !TimelineId.IsValid()
		|| TimelineId
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId)
		|| !AttributeComponent.IsValid()
		|| !MeridianShockModifierHandle.IsValid()
		|| !(MeridianShockModifierHandle
			== MakeMeridianShockModifierHandle(RunId, TargetEntityId))
		|| LastObservedTick < 0
		|| ConditionRevision < 0
		|| (bMeridianShockActive
			? MeridianShockExpiryTick <= LastObservedTick
			: MeridianShockExpiryTick != INDEX_NONE))
	{
		return false;
	}

	for (const TPair<FGuid, FProcessedApplication>& Pair :
		ProcessedApplications)
	{
		const Fdemo_mapShanmenCombatConditionApplicationReceipt& Receipt =
			Pair.Value.Receipt;
		if (!Receipt.IsValid()
			|| Pair.Key != Receipt.GetImpactId()
			|| Pair.Value.ResolutionId != Receipt.GetResolutionId()
			|| Receipt.GetRunId() != RunId
			|| Receipt.GetTargetEntityId() != TargetEntityId
			|| Receipt.GetTimelineId() != TimelineId
			|| Receipt.GetConditionRevision() > ConditionRevision)
		{
			return false;
		}
	}
	return true;
}

bool Udemo_mapShanmenCombatConditionComponent::IsEmpty() const
{
	return IsValid() && !RunId.IsValid();
}
