#include "demo_mapShanmenThrownWeaponArcLaunchInputAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus = Edemo_mapShanmenThrownWeaponArcLaunchInputStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	bool HasCommandShape(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
	{
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice =
			Command.GetChoiceState();
		return Command.GetHotbarSlotNumber() >= 1
			&& Command.GetHotbarSlotNumber()
				<= Fdemo_mapShanmenThrownWeaponArcLaunchCommand::
					MaximumHotbarSlotNumber
			&& Choice.IsValid()
			&& Choice.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Choice.HasArcTargetIntent()
			&& Command.GetPolicy().IsValid();
	}

	FGuid MakeCommandId(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
	{
		if (!HasCommandShape(Command))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcLaunchCommand.r1")),
			{
				FString::FromInt(Command.GetHotbarSlotNumber()),
				Command.GetChoiceState().GetStateId().ToString(
					EGuidFormats::Digits),
				FString::Printf(
					TEXT("%llu"), Command.GetChoiceState().GetRevision()),
				Command.GetPolicy().GetPolicyId().ToString(
					EGuidFormats::Digits)
			});
	}

	bool HasNoProductEvidence(
		const Fdemo_mapShanmenThrownWeaponArcLaunchInputResult& Result)
	{
		return !Result.GetCurrentChoiceState().IsValid()
			&& !Result.GetProductRoute().IsValid();
	}
}

bool Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
	const int32 InHotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& InChoiceState,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& InPolicy,
	Fdemo_mapShanmenThrownWeaponArcLaunchCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenThrownWeaponArcLaunchCommand();
	OutCommand.HotbarSlotNumber = InHotbarSlotNumber;
	OutCommand.ChoiceState = InChoiceState;
	OutCommand.Policy = InPolicy;
	OutCommand.CommandId = MakeCommandId(OutCommand);
	if (!OutCommand.IsValid())
	{
		OutCommand = Fdemo_mapShanmenThrownWeaponArcLaunchCommand();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcLaunchCommand::IsValid() const
{
	return CommandId.IsValid()
		&& HasCommandShape(*this)
		&& CommandId == MakeCommandId(*this);
}

bool Fdemo_mapShanmenThrownWeaponArcLaunchCommand::Matches(
	const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& CommandId == Other.CommandId
		&& HotbarSlotNumber == Other.HotbarSlotNumber
		&& ChoiceState.Matches(Other.ChoiceState)
		&& Policy.Matches(Other.Policy);
}

bool Fdemo_mapShanmenThrownWeaponArcLaunchInputResult::IsValid() const
{
	if (Status == EStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| ProductRouteResolutionCount < 0
		|| ProductRouteResolutionCount > 1
		|| ChoiceStateReadCount < 0 || ChoiceStateReadCount > 1
		|| ProductRouteInvocationCount < 0
		|| ProductRouteInvocationCount > 1)
	{
		return false;
	}

	if (Status == EStatus::CommandInvalid)
	{
		return !Command.IsValid()
			&& ProductRouteResolutionCount == 0
			&& ChoiceStateReadCount == 0
			&& ProductRouteInvocationCount == 0
			&& HasNoProductEvidence(*this);
	}
	if (!Command.IsValid())
	{
		return false;
	}

	switch (Status)
	{
	case EStatus::GameplayBlocked:
	case EStatus::InputSurfaceBlocked:
	case EStatus::InputModeBlocked:
		return ProductRouteResolutionCount == 0
			&& ChoiceStateReadCount == 0
			&& ProductRouteInvocationCount == 0
			&& HasNoProductEvidence(*this);

	case EStatus::ProductRouteUnavailable:
		return ProductRouteResolutionCount == 1
			&& ChoiceStateReadCount == 0
			&& ProductRouteInvocationCount == 0
			&& HasNoProductEvidence(*this);

	case EStatus::ChoiceStateInvalid:
		return ProductRouteResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& ProductRouteInvocationCount == 0
			&& !CurrentChoiceState.IsValid()
			&& !ProductRoute.IsValid();

	case EStatus::ChoiceStateMismatch:
		return ProductRouteResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& ProductRouteInvocationCount == 0
			&& CurrentChoiceState.IsValid()
			&& !CurrentChoiceState.Matches(Command.GetChoiceState())
			&& !ProductRoute.IsValid();

	case EStatus::Delegated:
		return ProductRouteResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& ProductRouteInvocationCount == 1
			&& CurrentChoiceState.Matches(Command.GetChoiceState())
			&& ProductRoute.IsValid();

	case EStatus::ProductProtocolRejected:
		return ProductRouteResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& ProductRouteInvocationCount == 1
			&& CurrentChoiceState.Matches(Command.GetChoiceState())
			&& !ProductRoute.IsValid();

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcLaunchInputResult::IsAccepted() const
{
	return IsValid()
		&& Status == EStatus::Delegated
		&& ProductRoute.IsAccepted();
}

Fdemo_mapShanmenThrownWeaponArcLaunchInputResult
Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter::Route(
	const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command,
	const bool bGameplayInputAllowed,
	const bool bGameplaySurface,
	const bool bGameOnlyInputMode,
	FResolveProductRoute ResolveProductRoute,
	FReadCurrentChoiceState ReadCurrentChoiceState,
	FRouteProduct RouteProduct)
{
	Fdemo_mapShanmenThrownWeaponArcLaunchInputResult Result;
	Result.Command = Command;
	if (!Command.IsValid())
	{
		Result.Status = EStatus::CommandInvalid;
		Result.Diagnostic = TEXT("Arc launch command is invalid.");
		return Result;
	}
	if (!bGameplayInputAllowed)
	{
		Result.Status = EStatus::GameplayBlocked;
		Result.Diagnostic =
			TEXT("Arc launch is blocked by the current gameplay state.");
		return Result;
	}
	if (!bGameplaySurface)
	{
		Result.Status = EStatus::InputSurfaceBlocked;
		Result.Diagnostic =
			TEXT("Arc launch requires the Gameplay input surface.");
		return Result;
	}
	if (!bGameOnlyInputMode)
	{
		Result.Status = EStatus::InputModeBlocked;
		Result.Diagnostic = TEXT("Arc launch requires GameOnly input mode.");
		return Result;
	}

	Result.ProductRouteResolutionCount = 1;
	if (!ResolveProductRoute())
	{
		Result.Status = EStatus::ProductRouteUnavailable;
		Result.Diagnostic =
			TEXT("Arc launch requires the authoritative GameMode product route.");
		return Result;
	}

	Result.ChoiceStateReadCount = 1;
	Result.CurrentChoiceState = ReadCurrentChoiceState();
	if (!Result.CurrentChoiceState.IsValid())
	{
		Result.Status = EStatus::ChoiceStateInvalid;
		Result.Diagnostic =
			TEXT("Arc launch read an invalid authoritative choice state.");
		return Result;
	}
	if (!Result.CurrentChoiceState.Matches(Command.GetChoiceState()))
	{
		Result.Status = EStatus::ChoiceStateMismatch;
		Result.Diagnostic =
			TEXT("Arc launch command choice state is stale.");
		return Result;
	}

	Result.ProductRouteInvocationCount = 1;
	Result.ProductRoute = RouteProduct(
		Command.GetHotbarSlotNumber(), Command.GetPolicy());
	if (!Result.ProductRoute.IsValid())
	{
		Result.Status = EStatus::ProductProtocolRejected;
		Result.Diagnostic =
			TEXT("Arc launch rejected an invalid P20.14 product route result.");
		return Result;
	}

	Result.Status = EStatus::Delegated;
	Result.Diagnostic = Result.ProductRoute.GetDiagnostic();
	return Result;
}
