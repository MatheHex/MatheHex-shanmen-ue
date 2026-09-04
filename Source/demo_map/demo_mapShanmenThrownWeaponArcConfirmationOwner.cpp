#include "demo_mapShanmenThrownWeaponArcConfirmationOwner.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus = Edemo_mapShanmenThrownWeaponArcConfirmationStatus;

	bool HasIntentShape(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent)
	{
		return Intent.GetConfirmationEventId().IsValid()
			&& Intent.GetHotbarSlotNumber() >= 1
			&& Intent.GetHotbarSlotNumber()
				<= Fdemo_mapShanmenThrownWeaponArcLaunchCommand::
					MaximumHotbarSlotNumber
			&& Intent.GetPolicy().IsValid();
	}

	FGuid MakeIntentId(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent)
	{
		if (!HasIntentShape(Intent))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcConfirmationIntent.r1")),
			{
				Intent.GetConfirmationEventId().ToString(EGuidFormats::Digits),
				FString::FromInt(Intent.GetHotbarSlotNumber()),
				Intent.GetPolicy().GetPolicyId().ToString(EGuidFormats::Digits)
			});
	}

	bool HasNoChoiceOrLaunchEvidence(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationResult& Result)
	{
		return !Result.GetChoiceState().IsValid()
			&& !Result.GetLaunchCommand().IsValid()
			&& !Result.GetLaunchInput().IsValid();
	}

	bool HasNoLaunchEvidence(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationResult& Result)
	{
		return !Result.GetLaunchCommand().IsValid()
			&& !Result.GetLaunchInput().IsValid();
	}

}

bool Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
	const FGuid& InConfirmationEventId,
	const int32 InHotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& InPolicy,
	Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponArcConfirmationIntent();
	OutIntent.ConfirmationEventId = InConfirmationEventId;
	OutIntent.HotbarSlotNumber = InHotbarSlotNumber;
	OutIntent.Policy = InPolicy;
	OutIntent.IntentId = MakeIntentId(OutIntent);
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponArcConfirmationIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::IsValid() const
{
	return IntentId.IsValid()
		&& HasIntentShape(*this)
		&& IntentId == MakeIntentId(*this);
}

bool Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::Matches(
	const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& IntentId == Other.IntentId
		&& ConfirmationEventId == Other.ConfirmationEventId
		&& HotbarSlotNumber == Other.HotbarSlotNumber
		&& Policy.Matches(Other.Policy);
}

bool Fdemo_mapShanmenThrownWeaponArcConfirmationResult::IsValid() const
{
	if (Status == EStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| ChoiceStateReadCount < 0 || ChoiceStateReadCount > 1
		|| LaunchRouteInvocationCount < 0
		|| LaunchRouteInvocationCount > 1)
	{
		return false;
	}

	if (Status == EStatus::IntentInvalid)
	{
		return !bReplay
			&& !Intent.IsValid()
			&& ChoiceStateReadCount == 0
			&& LaunchRouteInvocationCount == 0
			&& HasNoChoiceOrLaunchEvidence(*this);
	}
	if (!Intent.IsValid())
	{
		return false;
	}
	if (Status == EStatus::OwnerInvalid || Status == EStatus::EventConflict)
	{
		return !bReplay
			&& ChoiceStateReadCount == 0
			&& LaunchRouteInvocationCount == 0
			&& HasNoChoiceOrLaunchEvidence(*this);
	}

	const int32 ExpectedChoiceReads = bReplay ? 0 : 1;
	const int32 ExpectedLaunchRoutes = bReplay ? 0 : 1;
	switch (Status)
	{
	case EStatus::ConfirmationBlocked:
		return ChoiceStateReadCount == 0
			&& LaunchRouteInvocationCount == 0
			&& HasNoChoiceOrLaunchEvidence(*this);

	case EStatus::ChoiceStateInvalid:
		return ChoiceStateReadCount == ExpectedChoiceReads
			&& LaunchRouteInvocationCount == 0
			&& !ChoiceState.IsValid()
			&& HasNoLaunchEvidence(*this);

	case EStatus::CommandCaptureRejected:
		return ChoiceStateReadCount == ExpectedChoiceReads
			&& LaunchRouteInvocationCount == 0
			&& ChoiceState.IsValid()
			&& HasNoLaunchEvidence(*this);

	case EStatus::Routed:
		return ChoiceStateReadCount == ExpectedChoiceReads
			&& LaunchRouteInvocationCount == ExpectedLaunchRoutes
			&& ChoiceState.IsValid()
			&& LaunchCommand.IsValid()
			&& LaunchCommand.GetHotbarSlotNumber()
				== Intent.GetHotbarSlotNumber()
			&& LaunchCommand.GetChoiceState().Matches(ChoiceState)
			&& LaunchCommand.GetPolicy().Matches(Intent.GetPolicy())
			&& LaunchInput.IsValid()
			&& LaunchInput.GetCommand().Matches(LaunchCommand);

	case EStatus::InputProtocolRejected:
		return ChoiceStateReadCount == ExpectedChoiceReads
			&& LaunchRouteInvocationCount == ExpectedLaunchRoutes
			&& ChoiceState.IsValid()
			&& LaunchCommand.IsValid()
			&& LaunchCommand.GetHotbarSlotNumber()
				== Intent.GetHotbarSlotNumber()
			&& LaunchCommand.GetChoiceState().Matches(ChoiceState)
			&& LaunchCommand.GetPolicy().Matches(Intent.GetPolicy())
			&& (!LaunchInput.IsValid()
				|| !LaunchInput.GetCommand().Matches(LaunchCommand));

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcConfirmationResult::IsAccepted() const
{
	return IsValid()
		&& Status == EStatus::Routed
		&& LaunchInput.IsAccepted();
}

Fdemo_mapShanmenThrownWeaponArcConfirmationResult
Fdemo_mapShanmenThrownWeaponArcConfirmationOwner::Confirm(
	const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent,
	const bool bConfirmationAllowed,
	FReadCurrentChoiceState ReadCurrentChoiceState,
	FRouteLaunchCommand RouteLaunchCommand)
{
	if (!Intent.IsValid())
	{
		Fdemo_mapShanmenThrownWeaponArcConfirmationResult Result;
		Result.Status = EStatus::IntentInvalid;
		Result.Diagnostic = TEXT("Arc confirmation intent is invalid.");
		return Result;
	}
	if (!IsValid())
	{
		Fdemo_mapShanmenThrownWeaponArcConfirmationResult Result;
		Result.Status = EStatus::OwnerInvalid;
		Result.Diagnostic =
			TEXT("Arc confirmation owner invariants are invalid.");
		Result.Intent = Intent;
		return Result;
	}

	for (const FRetainedEvent& Entry : RetainedEvents)
	{
		if (Entry.Intent.GetConfirmationEventId()
			!= Intent.GetConfirmationEventId())
		{
			continue;
		}
		if (!Entry.Intent.Matches(Intent))
		{
			Fdemo_mapShanmenThrownWeaponArcConfirmationResult Result;
			Result.Status = EStatus::EventConflict;
			Result.Diagnostic =
				TEXT("Arc confirmation event identity was reused with different slot or policy.");
			Result.Intent = Intent;
			return Result;
		}

		Fdemo_mapShanmenThrownWeaponArcConfirmationResult Replay =
			Entry.Result;
		Replay.bReplay = true;
		Replay.ChoiceStateReadCount = 0;
		Replay.LaunchRouteInvocationCount = 0;
		Replay.Diagnostic =
			TEXT("Arc confirmation replay returned retained evidence without repeating work.");
		return Replay;
	}

	Fdemo_mapShanmenThrownWeaponArcConfirmationResult Result;
	Result.Intent = Intent;
	if (!bConfirmationAllowed)
	{
		Result.Status = EStatus::ConfirmationBlocked;
		Result.Diagnostic =
			TEXT("Arc confirmation is blocked by the current local input context.");
		Retain(Intent, Result);
		return Result;
	}

	Result.ChoiceStateReadCount = 1;
	Result.ChoiceState = ReadCurrentChoiceState();
	if (!Result.ChoiceState.IsValid())
	{
		Result.Status = EStatus::ChoiceStateInvalid;
		Result.Diagnostic =
			TEXT("Arc confirmation could not read one valid current choice state.");
		Retain(Intent, Result);
		return Result;
	}

	if (!Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			Intent.GetHotbarSlotNumber(),
			Result.ChoiceState,
			Intent.GetPolicy(),
			Result.LaunchCommand))
	{
		Result.Status = EStatus::CommandCaptureRejected;
		Result.Diagnostic =
			TEXT("Current choice cannot produce an Arc launch command.");
		Retain(Intent, Result);
		return Result;
	}

	Result.LaunchRouteInvocationCount = 1;
	Result.LaunchInput = RouteLaunchCommand(Result.LaunchCommand);
	if (!Result.LaunchInput.IsValid()
		|| !Result.LaunchInput.GetCommand().Matches(Result.LaunchCommand))
	{
		Result.Status = EStatus::InputProtocolRejected;
		Result.Diagnostic =
			TEXT("Arc confirmation rejected invalid or mismatched P20.15 input evidence.");
		Retain(Intent, Result);
		return Result;
	}

	Result.Status = EStatus::Routed;
	Result.Diagnostic = Result.LaunchInput.GetDiagnostic();
	Retain(Intent, Result);
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponArcConfirmationOwner::IsValid() const
{
	if (RetainedEvents.Num() < 0
		|| RetainedEvents.Num() > MaximumRetainedEvents)
	{
		return false;
	}

	TSet<FGuid> EventIds;
	for (const FRetainedEvent& Entry : RetainedEvents)
	{
		if (!Entry.Intent.IsValid()
			|| !Entry.Result.IsValid()
			|| Entry.Result.WasReplay()
			|| !Entry.Result.GetIntent().Matches(Entry.Intent)
			|| EventIds.Contains(Entry.Intent.GetConfirmationEventId()))
		{
			return false;
		}
		EventIds.Add(Entry.Intent.GetConfirmationEventId());
	}
	return true;
}

void Fdemo_mapShanmenThrownWeaponArcConfirmationOwner::Retain(
	const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent,
	const Fdemo_mapShanmenThrownWeaponArcConfirmationResult& Result)
{
	if (!Intent.IsValid() || !Result.IsValid() || Result.WasReplay())
	{
		return;
	}
	if (RetainedEvents.Num() == MaximumRetainedEvents)
	{
		RetainedEvents.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FRetainedEvent& Entry = RetainedEvents.AddDefaulted_GetRef();
	Entry.Intent = Intent;
	Entry.Result = Result;
}
