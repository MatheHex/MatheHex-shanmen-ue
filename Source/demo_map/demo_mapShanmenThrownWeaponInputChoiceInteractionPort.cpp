#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using ECapability =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability;
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	FString DoubleBits(double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool VectorsMatch(const FVector2D& Left, const FVector2D& Right)
	{
		return DoubleBits(Left.X) == DoubleBits(Right.X)
			&& DoubleBits(Left.Y) == DoubleBits(Right.Y);
	}

	ECapability ExpectedCapabilities(
		const ETrajectory Trajectory,
		const bool bHasTarget,
		const double Apex)
	{
		if (Trajectory == ETrajectory::Straight)
		{
			return ECapability::SelectBallisticArcTrajectory;
		}
		if (Trajectory != ETrajectory::BallisticArc)
		{
			return ECapability::None;
		}

		ECapability Result = ECapability::SelectStraightTrajectory
			| ECapability::SetArcTargetIntent;
		if (Apex < 1.0)
		{
			Result |= ECapability::IncreaseArcApex;
		}
		if (Apex > -1.0)
		{
			Result |= ECapability::DecreaseArcApex;
		}
		if (bHasTarget)
		{
			Result |= ECapability::ClearArcTargetIntent;
		}
		return Result;
	}

	FGuid MakeReadModelId(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& Model)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT(
				"demo_map.ShanmenThrownWeapon.InputChoiceInteractionReadModel.r1")),
			{
				FString::FromInt(static_cast<int32>(Model.GetTrajectoryKind())),
				Model.HasArcTargetIntent() ? TEXT("1") : TEXT("0"),
				DoubleBits(Model.GetArcTargetIntent().X),
				DoubleBits(Model.GetArcTargetIntent().Y),
				DoubleBits(Model.GetArcApexAdjustment()),
				FString::FromInt(static_cast<int32>(Model.GetCapabilities()))
			});
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::TryProject(
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& OutModel)
{
	OutModel = Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel();
	if (!State.IsValid())
	{
		return false;
	}
	OutModel.TrajectoryKind = State.GetTrajectoryKind();
	OutModel.bHasArcTargetIntent = State.HasArcTargetIntent();
	OutModel.ArcTargetIntent = State.GetArcTargetIntent();
	OutModel.ArcApexAdjustment = State.GetArcApexAdjustment();
	OutModel.Capabilities = ExpectedCapabilities(
		OutModel.TrajectoryKind,
		OutModel.bHasArcTargetIntent,
		OutModel.ArcApexAdjustment);
	OutModel.ReadModelId = MakeReadModelId(OutModel);
	return OutModel.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::IsValid()
	const
{
	if (!ReadModelId.IsValid()
		|| !FMath::IsFinite(ArcApexAdjustment)
		|| ArcApexAdjustment < -1.0 || ArcApexAdjustment > 1.0
		|| Capabilities != ExpectedCapabilities(
			TrajectoryKind, bHasArcTargetIntent, ArcApexAdjustment))
	{
		return false;
	}
	if (TrajectoryKind == ETrajectory::Straight)
	{
		if (bHasArcTargetIntent || !ArcTargetIntent.IsZero()
			|| ArcApexAdjustment != 0.0)
		{
			return false;
		}
	}
	else if (TrajectoryKind == ETrajectory::BallisticArc)
	{
		if (bHasArcTargetIntent == ArcTargetIntent.IsZero()
			|| !FMath::IsFinite(ArcTargetIntent.X)
			|| !FMath::IsFinite(ArcTargetIntent.Y)
			|| ArcTargetIntent.SizeSquared() > 1.0 + 1.0e-6)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return ReadModelId == MakeReadModelId(*this);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::Matches(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& ReadModelId == Other.ReadModelId
		&& TrajectoryKind == Other.TrajectoryKind
		&& bHasArcTargetIntent == Other.bHasArcTargetIntent
		&& VectorsMatch(ArcTargetIntent, Other.ArcTargetIntent)
		&& DoubleBits(ArcApexAdjustment)
			== DoubleBits(Other.ArcApexAdjustment)
		&& Capabilities == Other.Capabilities;
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::HasCapability(
	const ECapability Capability) const
{
	return IsValid() && Capability != ECapability::None
		&& EnumHasAllFlags(Capabilities, Capability);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult::IsValid()
	const
{
	if (Status == EReadStatus::Invalid || Diagnostic.IsEmpty()
		|| ChoiceSourceResolutionCount != 1
		|| ChoiceStateReadCount < 0 || ChoiceStateReadCount > 1
		|| ProjectionCount < 0 || ProjectionCount > 1)
	{
		return false;
	}
	switch (Status)
	{
	case EReadStatus::ChoiceSourceUnavailable:
		return ChoiceStateReadCount == 0 && ProjectionCount == 0
			&& !ReadModel.IsValid();
	case EReadStatus::ChoiceStateInvalid:
		return ChoiceStateReadCount == 1 && ProjectionCount == 0
			&& !ReadModel.IsValid();
	case EReadStatus::Projected:
		return ChoiceStateReadCount == 1 && ProjectionCount == 1
			&& ReadModel.IsValid();
	case EReadStatus::ProjectionRejected:
		return ChoiceStateReadCount == 1 && ProjectionCount == 1
			&& !ReadModel.IsValid();
	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult::IsProjected()
	const
{
	return IsValid() && Status == EReadStatus::Projected;
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult
Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
	FResolveChoiceSource ResolveChoiceSource,
	FReadChoiceState ReadChoiceState)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult Result;
	Result.ChoiceSourceResolutionCount = 1;
	if (!ResolveChoiceSource())
	{
		Result.Status = EReadStatus::ChoiceSourceUnavailable;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice interaction source is unavailable.");
		return Result;
	}

	Result.ChoiceStateReadCount = 1;
	const Fdemo_mapShanmenThrownWeaponInputChoiceState State =
		ReadChoiceState();
	if (!State.IsValid())
	{
		Result.Status = EReadStatus::ChoiceStateInvalid;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice interaction read an invalid state.");
		return Result;
	}

	Result.ProjectionCount = 1;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::
		TryProject(State, Result.ReadModel))
	{
		Result.Status = EReadStatus::ProjectionRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice interaction projection was rejected.");
		return Result;
	}
	Result.Status = EReadStatus::Projected;
	Result.Diagnostic =
		TEXT("Thrown-weapon choice interaction projection is current.");
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
TryEmitTrajectorySelection(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const ETrajectory TrajectoryKind,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	const ECapability Required = TrajectoryKind == ETrajectory::Straight
		? ECapability::SelectStraightTrajectory
		: TrajectoryKind == ETrajectory::BallisticArc
			? ECapability::SelectBallisticArcTrajectory
			: ECapability::None;
	return Required != ECapability::None
		&& ReadModel.HasCapability(Required)
		&& Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureTrajectorySelection(TrajectoryKind, OutIntent);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
TryEmitArcTargetIntent(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const FVector2D& RawTargetIntent,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	return ReadModel.HasCapability(ECapability::SetArcTargetIntent)
		&& Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcTargetIntent(RawTargetIntent, OutIntent);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
TryEmitArcApexAdjustment(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const double RawNormalizedDelta,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	if (!FMath::IsFinite(RawNormalizedDelta) || RawNormalizedDelta == 0.0)
	{
		return false;
	}
	const ECapability Required = RawNormalizedDelta > 0.0
		? ECapability::IncreaseArcApex
		: ECapability::DecreaseArcApex;
	return ReadModel.HasCapability(Required)
		&& Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcApexAdjustment(RawNormalizedDelta, OutIntent);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
TryEmitArcTargetClear(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponInputChoiceIntent();
	return ReadModel.HasCapability(ECapability::ClearArcTargetIntent)
		&& Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcTargetClear(OutIntent);
}
