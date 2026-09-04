#include "demo_mapShanmenThrownWeaponInputChoiceIntentAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EIntentKind =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentKind;
	using EIntentStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentStatus;
	using EControllerStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus;

	FString DoubleBits(double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool IsZeroVector(const FVector2D& Value)
	{
		return Value.X == 0.0 && Value.Y == 0.0;
	}

	bool VectorsMatch(const FVector2D& Left, const FVector2D& Right)
	{
		return DoubleBits(Left.X) == DoubleBits(Right.X)
			&& DoubleBits(Left.Y) == DoubleBits(Right.Y);
	}

	bool TryBuildCommandUnchecked(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent,
		const uint64 ExpectedRevision,
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand)
	{
		switch (Intent.GetKind())
		{
		case EIntentKind::SelectTrajectory:
			return Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureTrajectorySelection(
					ExpectedRevision,
					Intent.GetTrajectoryKind(),
					OutCommand);
		case EIntentKind::SetArcTargetIntent:
			return Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcTargetIntent(
					ExpectedRevision,
					Intent.GetArcTargetIntent(),
					OutCommand);
		case EIntentKind::AdjustArcApex:
			return Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcApexAdjustment(
					ExpectedRevision,
					Intent.GetArcApexAdjustmentDelta(),
					OutCommand);
		case EIntentKind::ClearArcTargetIntent:
			return Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcTargetClear(ExpectedRevision, OutCommand);
		default:
			OutCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand();
			return false;
		}
	}

	bool HasCanonicalIntentShape(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Canonical;
		if (!TryBuildCommandUnchecked(Intent, 0, Canonical))
		{
			return false;
		}

		switch (Intent.GetKind())
		{
		case EIntentKind::SelectTrajectory:
			return IsZeroVector(Intent.GetArcTargetIntent())
				&& Intent.GetArcApexAdjustmentDelta() == 0.0
				&& Canonical.GetTrajectoryKind()
					== Intent.GetTrajectoryKind();
		case EIntentKind::SetArcTargetIntent:
			return Intent.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
						Invalid
				&& Intent.GetArcApexAdjustmentDelta() == 0.0
				&& VectorsMatch(
					Canonical.GetArcTargetIntent(),
					Intent.GetArcTargetIntent());
		case EIntentKind::AdjustArcApex:
			return Intent.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
						Invalid
				&& IsZeroVector(Intent.GetArcTargetIntent())
				&& DoubleBits(Canonical.GetArcApexAdjustmentDelta())
					== DoubleBits(Intent.GetArcApexAdjustmentDelta());
		case EIntentKind::ClearArcTargetIntent:
			return Intent.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
						Invalid
				&& IsZeroVector(Intent.GetArcTargetIntent())
				&& Intent.GetArcApexAdjustmentDelta() == 0.0;
		default:
			return false;
		}
	}

	FGuid MakeIntentId(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent)
	{
		if (!HasCanonicalIntentShape(Intent))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.InputChoiceIntent.r1")),
			{
				FString::FromInt(static_cast<int32>(Intent.GetKind())),
				FString::FromInt(
					static_cast<int32>(Intent.GetTrajectoryKind())),
				DoubleBits(Intent.GetArcTargetIntent().X),
				DoubleBits(Intent.GetArcTargetIntent().Y),
				DoubleBits(Intent.GetArcApexAdjustmentDelta())
			});
	}

	bool CommandMatchesIntentAndState(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		if (!Intent.IsValid() || !State.IsValid() || !Command.IsValid()
			|| Command.GetExpectedRevision() != State.GetRevision())
		{
			return false;
		}
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Expected;
		return Intent.TryCaptureCommand(State.GetRevision(), Expected)
			&& Expected.GetCommandId() == Command.GetCommandId();
	}

	bool HasNoControllerEvidence(
		const Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult& Result)
	{
		return !Result.IsValid()
			&& Result.GetStatus() == EControllerStatus::Invalid
			&& Result.GetDiagnostic().IsEmpty()
			&& Result.GetChoiceSessionResolutionCount() == 0
			&& Result.GetChoiceSessionSubmissionCount() == 0
			&& !Result.GetCommand().IsValid();
	}

	bool ControllerMatchesCommand(
		const Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult& Result,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		return Result.IsValid()
			&& Result.GetCommand().IsValid()
			&& Result.GetCommand().GetCommandId() == Command.GetCommandId()
			&& Result.GetCommand().GetExpectedRevision()
				== Command.GetExpectedRevision();
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
TryCaptureTrajectorySelection(
	const Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind InTrajectoryKind,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Canonical;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureTrajectorySelection(0, InTrajectoryKind, Canonical))
	{
		return false;
	}
	OutIntent.Kind = EIntentKind::SelectTrajectory;
	OutIntent.TrajectoryKind = Canonical.GetTrajectoryKind();
	OutIntent.IntentId = MakeIntentId(OutIntent);
	return OutIntent.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
TryCaptureArcTargetIntent(
	const FVector2D& RawTargetIntent,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Canonical;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcTargetIntent(0, RawTargetIntent, Canonical))
	{
		return false;
	}
	OutIntent.Kind = EIntentKind::SetArcTargetIntent;
	OutIntent.ArcTargetIntent = Canonical.GetArcTargetIntent();
	OutIntent.IntentId = MakeIntentId(OutIntent);
	return OutIntent.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
TryCaptureArcApexAdjustment(
	const double RawNormalizedDelta,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Canonical;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcApexAdjustment(0, RawNormalizedDelta, Canonical))
	{
		return false;
	}
	OutIntent.Kind = EIntentKind::AdjustArcApex;
	OutIntent.ArcApexAdjustmentDelta =
		Canonical.GetArcApexAdjustmentDelta();
	OutIntent.IntentId = MakeIntentId(OutIntent);
	return OutIntent.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::TryCaptureArcTargetClear(
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Canonical;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcTargetClear(0, Canonical))
	{
		return false;
	}
	OutIntent.Kind = EIntentKind::ClearArcTargetIntent;
	OutIntent.IntentId = MakeIntentId(OutIntent);
	return OutIntent.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::TryCaptureCommand(
	const uint64 ExpectedRevision,
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand) const
{
	OutCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand();
	return IsValid()
		&& TryBuildCommandUnchecked(*this, ExpectedRevision, OutCommand);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::IsValid() const
{
	return IntentId.IsValid()
		&& HasCanonicalIntentShape(*this)
		&& IntentId == MakeIntentId(*this);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntent::Matches(
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& IntentId == Other.IntentId
		&& Kind == Other.Kind
		&& TrajectoryKind == Other.TrajectoryKind
		&& VectorsMatch(ArcTargetIntent, Other.ArcTargetIntent)
		&& DoubleBits(ArcApexAdjustmentDelta)
			== DoubleBits(Other.ArcApexAdjustmentDelta);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult::IsValid() const
{
	if (Status == EIntentStatus::Invalid || Diagnostic.IsEmpty()
		|| ChoiceSourceResolutionCount < 0
		|| ChoiceSourceResolutionCount > 1
		|| ChoiceStateReadCount < 0 || ChoiceStateReadCount > 1
		|| CommandCaptureCount < 0 || CommandCaptureCount > 1
		|| ControllerRouteCount < 0 || ControllerRouteCount > 1)
	{
		return false;
	}

	if (Status == EIntentStatus::IntentInvalid)
	{
		return !Intent.IsValid()
			&& ChoiceSourceResolutionCount == 0
			&& ChoiceStateReadCount == 0
			&& CommandCaptureCount == 0
			&& ControllerRouteCount == 0
			&& !ChoiceState.IsValid()
			&& !Command.IsValid()
			&& HasNoControllerEvidence(ControllerResult);
	}
	if (!Intent.IsValid())
	{
		return false;
	}

	switch (Status)
	{
	case EIntentStatus::ChoiceSourceUnavailable:
		return ChoiceSourceResolutionCount == 1
			&& ChoiceStateReadCount == 0
			&& CommandCaptureCount == 0
			&& ControllerRouteCount == 0
			&& !ChoiceState.IsValid()
			&& !Command.IsValid()
			&& HasNoControllerEvidence(ControllerResult);

	case EIntentStatus::ChoiceStateInvalid:
		return ChoiceSourceResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& CommandCaptureCount == 0
			&& ControllerRouteCount == 0
			&& !ChoiceState.IsValid()
			&& !Command.IsValid()
			&& HasNoControllerEvidence(ControllerResult);

	case EIntentStatus::CommandCaptureRejected:
		return ChoiceSourceResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& CommandCaptureCount == 1
			&& ControllerRouteCount == 0
			&& ChoiceState.IsValid()
			&& !Command.IsValid()
			&& HasNoControllerEvidence(ControllerResult);

	case EIntentStatus::Delegated:
		return ChoiceSourceResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& CommandCaptureCount == 1
			&& ControllerRouteCount == 1
			&& CommandMatchesIntentAndState(Intent, ChoiceState, Command)
			&& ControllerMatchesCommand(ControllerResult, Command)
			&& Diagnostic == ControllerResult.GetDiagnostic();

	case EIntentStatus::ControllerProtocolRejected:
		return ChoiceSourceResolutionCount == 1
			&& ChoiceStateReadCount == 1
			&& CommandCaptureCount == 1
			&& ControllerRouteCount == 1
			&& CommandMatchesIntentAndState(Intent, ChoiceState, Command)
			&& !ControllerMatchesCommand(ControllerResult, Command);

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult::IsAccepted() const
{
	return IsValid()
		&& Status == EIntentStatus::Delegated
		&& ControllerResult.IsAccepted();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult::
WasRejectedByController() const
{
	return IsValid()
		&& Status == EIntentStatus::Delegated
		&& !ControllerResult.IsAccepted();
}

Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult
Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter::Route(
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent,
	FResolveChoiceSource ResolveChoiceSource,
	FReadChoiceState ReadChoiceState,
	FRouteController RouteController)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult Result;
	Result.Intent = Intent;
	if (!Intent.IsValid())
	{
		Result.Status = EIntentStatus::IntentInvalid;
		Result.Diagnostic = TEXT("Thrown-weapon choice edit intent is invalid.");
		return Result;
	}

	Result.ChoiceSourceResolutionCount = 1;
	if (!ResolveChoiceSource())
	{
		Result.Status = EIntentStatus::ChoiceSourceUnavailable;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit requires an authoritative choice source.");
		return Result;
	}

	Result.ChoiceStateReadCount = 1;
	Result.ChoiceState = ReadChoiceState();
	if (!Result.ChoiceState.IsValid())
	{
		Result.Status = EIntentStatus::ChoiceStateInvalid;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit read an invalid authoritative state.");
		return Result;
	}

	Result.CommandCaptureCount = 1;
	if (!Intent.TryCaptureCommand(
			Result.ChoiceState.GetRevision(), Result.Command))
	{
		Result.Status = EIntentStatus::CommandCaptureRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit could not freeze a P20.10 command.");
		return Result;
	}

	Result.ControllerRouteCount = 1;
	Result.ControllerResult = RouteController(Result.Command);
	if (!ControllerMatchesCommand(Result.ControllerResult, Result.Command))
	{
		Result.Status = EIntentStatus::ControllerProtocolRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit rejected invalid P20.18 controller evidence.");
		return Result;
	}

	Result.Status = EIntentStatus::Delegated;
	Result.Diagnostic = Result.ControllerResult.GetDiagnostic();
	return Result;
}
