#include "demo_mapShanmenThrownWeaponHotbarConfirmationAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponHotbarConfirmationStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	bool IsValidSlot(const int32 HotbarSlotNumber)
	{
		return HotbarSlotNumber >= 1
			&& HotbarSlotNumber
				<= Fdemo_mapShanmenThrownWeaponArcLaunchCommand::
					MaximumHotbarSlotNumber;
	}

	bool IsStraightInputProtocolValid(
		const Fdemo_mapShanmenThrownWeaponInputResult& Input,
		const int32 ExpectedHotbarSlotNumber)
	{
		return !Input.Diagnostic.IsEmpty()
			&& Input.HotbarSlotNumber == ExpectedHotbarSlotNumber;
	}

	bool HasNoChoiceOrRouteEvidence(
		const Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult& Result)
	{
		return !Result.GetChoiceState().IsValid()
			&& Result.GetPolicyReadCount() == 0
			&& Result.GetStraightRouteInvocationCount() == 0
			&& Result.GetArcRouteInvocationCount() == 0
			&& Result.GetConfirmationOrdinal() == 0
			&& !Result.GetConfirmationEventId().IsValid()
			&& !Result.GetPolicy().IsValid()
			&& !Result.GetArcIntent().IsValid()
			&& !Result.GetArcConfirmation().IsValid();
	}

	bool HasNoArcEvidence(
		const Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult& Result)
	{
		return Result.GetPolicyReadCount() == 0
			&& Result.GetArcRouteInvocationCount() == 0
			&& Result.GetConfirmationOrdinal() == 0
			&& !Result.GetConfirmationEventId().IsValid()
			&& !Result.GetPolicy().IsValid()
			&& !Result.GetArcIntent().IsValid()
			&& !Result.GetArcConfirmation().IsValid();
	}

	bool HasCanonicalArcIdentity(
		const Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult& Result)
	{
		return Result.GetConfirmationOrdinal() > 0
			&& Result.GetConfirmationOrdinal() < MAX_uint64
			&& Result.GetConfirmationEventId().IsValid()
			&& Result.GetConfirmationEventId()
				== Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
					MakeConfirmationEventId(
						Result.GetConfirmationOrdinal());
	}

	bool ArcIntentMatchesResult(
		const Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult& Result)
	{
		return Result.GetArcIntent().IsValid()
			&& Result.GetArcIntent().GetConfirmationEventId()
				== Result.GetConfirmationEventId()
			&& Result.GetArcIntent().GetHotbarSlotNumber()
				== Result.GetHotbarSlotNumber()
			&& Result.GetArcIntent().GetPolicy().Matches(Result.GetPolicy());
	}
}

const Fdemo_mapShanmenThrownWeaponArcChoicePolicy&
Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::GetCanonical()
{
	static const Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy = []()
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Value;
		const bool bCaptured =
			Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
				400.0,
				1200.0,
				300.0,
				100.0,
				500.0,
				Value);
		check(bCaptured && Value.IsValid());
		return Value;
	}();
	return Policy;
}

bool Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult::IsValid() const
{
	if (Status == EStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| ChoiceStateReadCount < 0 || ChoiceStateReadCount > 1
		|| PolicyReadCount < 0 || PolicyReadCount > 1
		|| StraightRouteInvocationCount < 0
		|| StraightRouteInvocationCount > 1
		|| ArcRouteInvocationCount < 0
		|| ArcRouteInvocationCount > 1)
	{
		return false;
	}

	if (Status == EStatus::InvalidSlot)
	{
		return !IsValidSlot(HotbarSlotNumber)
			&& ChoiceStateReadCount == 0
			&& HasNoChoiceOrRouteEvidence(*this);
	}
	if (!IsValidSlot(HotbarSlotNumber))
	{
		return false;
	}

	if (Status == EStatus::InputBlocked
		|| Status == EStatus::ConsumerUnavailable)
	{
		return ChoiceStateReadCount == 0
			&& HasNoChoiceOrRouteEvidence(*this);
	}
	if (Status == EStatus::ChoiceStateInvalid)
	{
		return ChoiceStateReadCount == 1
			&& !ChoiceState.IsValid()
			&& PolicyReadCount == 0
			&& StraightRouteInvocationCount == 0
			&& ArcRouteInvocationCount == 0
			&& ConfirmationOrdinal == 0
			&& !ConfirmationEventId.IsValid()
			&& !Policy.IsValid()
			&& !ArcIntent.IsValid()
			&& !ArcConfirmation.IsValid();
	}
	if (ChoiceStateReadCount != 1 || !ChoiceState.IsValid())
	{
		return false;
	}

	if (Status == EStatus::StraightDelegated
		|| Status == EStatus::StraightRouteProtocolRejected)
	{
		const bool bProtocolValid = IsStraightInputProtocolValid(
			StraightInput, HotbarSlotNumber);
		return ChoiceState.GetTrajectoryKind() == ETrajectory::Straight
			&& StraightRouteInvocationCount == 1
			&& HasNoArcEvidence(*this)
			&& (Status == EStatus::StraightDelegated
				? bProtocolValid
				: !bProtocolValid);
	}

	if (ChoiceState.GetTrajectoryKind() != ETrajectory::BallisticArc
		|| StraightRouteInvocationCount != 0)
	{
		return false;
	}
	if (Status == EStatus::ConfirmationSequenceExhausted)
	{
		return PolicyReadCount == 0
			&& ArcRouteInvocationCount == 0
			&& (ConfirmationOrdinal == 0
				|| ConfirmationOrdinal == MAX_uint64)
			&& !ConfirmationEventId.IsValid()
			&& !Policy.IsValid()
			&& !ArcIntent.IsValid()
			&& !ArcConfirmation.IsValid();
	}
	if (!HasCanonicalArcIdentity(*this) || PolicyReadCount != 1)
	{
		return false;
	}
	if (Status == EStatus::ArcPolicyInvalid)
	{
		return !Policy.IsValid()
			&& ArcRouteInvocationCount == 0
			&& !ArcIntent.IsValid()
			&& !ArcConfirmation.IsValid();
	}
	if (!Policy.IsValid())
	{
		return false;
	}
	if (Status == EStatus::ArcIntentCaptureRejected)
	{
		return ArcRouteInvocationCount == 0
			&& !ArcIntent.IsValid()
			&& !ArcConfirmation.IsValid();
	}
	if (!ArcIntentMatchesResult(*this) || ArcRouteInvocationCount != 1)
	{
		return false;
	}

	const bool bConfirmationMatches = ArcConfirmation.IsValid()
		&& ArcConfirmation.GetIntent().Matches(ArcIntent);
	if (Status == EStatus::ArcDelegated)
	{
		return bConfirmationMatches;
	}
	if (Status == EStatus::ArcRouteProtocolRejected)
	{
		return !bConfirmationMatches;
	}
	return false;
}

bool
Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult::ShouldPassThrough() const
{
	if (!IsValid())
	{
		return false;
	}
	if (Status == EStatus::StraightDelegated)
	{
		return StraightInput.ShouldPassThrough();
	}
	if (Status != EStatus::ArcDelegated
		|| ArcConfirmation.GetStatus()
			!= Edemo_mapShanmenThrownWeaponArcConfirmationStatus::Routed)
	{
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponArcLaunchInputResult& Launch =
		ArcConfirmation.GetLaunchInput();
	if (!Launch.IsValid()
		|| Launch.GetStatus()
			!= Edemo_mapShanmenThrownWeaponArcLaunchInputStatus::Delegated)
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult& Product =
		Launch.GetProductRoute();
	return Product.IsValid()
		&& Product.GetComposition().IsValid()
		&& Product.GetComposition().GetInput().ShouldPassThrough();
}

bool Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult::IsAccepted() const
{
	if (!IsValid())
	{
		return false;
	}
	return Status == EStatus::StraightDelegated
		? StraightInput.IsAccepted()
		: Status == EStatus::ArcDelegated
			&& ArcConfirmation.IsAccepted();
}

FGuid
Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::MakeConfirmationEventId(
	const uint64 ConfirmationOrdinal)
{
	if (ConfirmationOrdinal == 0 || ConfirmationOrdinal == MAX_uint64)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT(
			"demo_map.ShanmenThrownWeapon.HotbarConfirmationEvent.r1")),
		{FString::Printf(TEXT("%llu"), ConfirmationOrdinal)});
}

Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult
Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::Route(
	const int32 HotbarSlotNumber,
	const bool bInputAllowed,
	const bool bConsumerAvailable,
	FReadCurrentChoiceState ReadCurrentChoiceState,
	FReadArcPolicy ReadArcPolicy,
	FRouteStraight RouteStraight,
	FRouteArcConfirmation RouteArcConfirmation)
{
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult Result;
	Result.HotbarSlotNumber = HotbarSlotNumber;
	if (!IsValidSlot(HotbarSlotNumber))
	{
		Result.Status = EStatus::InvalidSlot;
		Result.Diagnostic = TEXT("Hotbar confirmation requires slot 1 through 9.");
		return Result;
	}
	if (!bInputAllowed)
	{
		Result.Status = EStatus::InputBlocked;
		Result.Diagnostic =
			TEXT("Hotbar confirmation is blocked by the current input context.");
		return Result;
	}
	if (!bConsumerAvailable)
	{
		Result.Status = EStatus::ConsumerUnavailable;
		Result.Diagnostic =
			TEXT("Hotbar confirmation requires the authoritative GameMode consumer.");
		return Result;
	}

	Result.ChoiceStateReadCount = 1;
	Result.ChoiceState = ReadCurrentChoiceState();
	if (!Result.ChoiceState.IsValid())
	{
		Result.Status = EStatus::ChoiceStateInvalid;
		Result.Diagnostic =
			TEXT("Hotbar confirmation read an invalid trajectory choice state.");
		return Result;
	}

	if (Result.ChoiceState.GetTrajectoryKind() == ETrajectory::Straight)
	{
		Result.StraightRouteInvocationCount = 1;
		Result.StraightInput = RouteStraight(HotbarSlotNumber);
		if (!IsStraightInputProtocolValid(
				Result.StraightInput, HotbarSlotNumber))
		{
			Result.Status = EStatus::StraightRouteProtocolRejected;
			Result.Diagnostic =
				TEXT("Straight hotbar route returned invalid slot or diagnostic evidence.");
			return Result;
		}
		Result.Status = EStatus::StraightDelegated;
		Result.Diagnostic = Result.StraightInput.Diagnostic;
		return Result;
	}

	if (NextConfirmationOrdinal == 0
		|| NextConfirmationOrdinal == MAX_uint64)
	{
		Result.Status = EStatus::ConfirmationSequenceExhausted;
		Result.ConfirmationOrdinal = NextConfirmationOrdinal;
		Result.Diagnostic =
			TEXT("Arc hotbar confirmation event sequence is exhausted.");
		return Result;
	}

	Result.ConfirmationOrdinal = NextConfirmationOrdinal;
	++NextConfirmationOrdinal;
	Result.ConfirmationEventId =
		MakeConfirmationEventId(Result.ConfirmationOrdinal);
	Result.PolicyReadCount = 1;
	Result.Policy = ReadArcPolicy();
	if (!Result.Policy.IsValid())
	{
		Result.Status = EStatus::ArcPolicyInvalid;
		Result.Diagnostic =
			TEXT("Arc hotbar confirmation read an invalid product policy.");
		return Result;
	}

	if (!Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
			Result.ConfirmationEventId,
			HotbarSlotNumber,
			Result.Policy,
			Result.ArcIntent))
	{
		Result.Status = EStatus::ArcIntentCaptureRejected;
		Result.Diagnostic =
			TEXT("Arc hotbar confirmation could not capture P20.16 intent.");
		return Result;
	}

	Result.ArcRouteInvocationCount = 1;
	Result.ArcConfirmation = RouteArcConfirmation(Result.ArcIntent);
	if (!Result.ArcConfirmation.IsValid()
		|| !Result.ArcConfirmation.GetIntent().Matches(Result.ArcIntent))
	{
		Result.Status = EStatus::ArcRouteProtocolRejected;
		Result.Diagnostic =
			TEXT("Arc confirmation route returned invalid or mismatched P20.16 evidence.");
		return Result;
	}

	Result.Status = EStatus::ArcDelegated;
	Result.Diagnostic = Result.ArcConfirmation.GetDiagnostic();
	return Result;
}
