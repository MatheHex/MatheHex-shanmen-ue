#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

#include "ShanmenDeterministicId.h"

namespace
{
	constexpr double TargetIntentMinimumMagnitudeSquared = 1.0e-12;
	constexpr double UnitDiscTolerance = 1.0e-6;

	double CanonicalZero(const double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	double CanonicalInputScalar(const double Value)
	{
		return CanonicalZero(static_cast<double>(static_cast<float>(Value)));
	}

	FVector2D CanonicalVector(FVector2D Value)
	{
		Value.X = CanonicalZero(Value.X);
		Value.Y = CanonicalZero(Value.Y);
		return Value;
	}

	bool IsFiniteVector(const FVector2D& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
	}

	bool IsKnownTrajectory(
		const Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind Value)
	{
		return Value
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
			|| Value
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc;
	}

	bool IsKnownCommandKind(
		const Edemo_mapShanmenThrownWeaponInputChoiceCommandKind Value)
	{
		return Value
			== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
				SelectTrajectory
			|| Value
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					SetArcTargetIntent
			|| Value
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					AdjustArcApex
			|| Value
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					ClearArcTargetIntent;
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool IsZeroVector(const FVector2D& Value)
	{
		return Value.X == 0.0 && Value.Y == 0.0;
	}

	bool VectorsMatch(
		const FVector2D& Left,
		const FVector2D& Right)
	{
		return DoubleBits(Left.X) == DoubleBits(Right.X)
			&& DoubleBits(Left.Y) == DoubleBits(Right.Y);
	}

	bool IsCanonicalTargetIntent(const FVector2D& Value)
	{
		if (!IsFiniteVector(Value))
		{
			return false;
		}
		const double MagnitudeSquared = Value.SizeSquared();
		return FMath::IsFinite(MagnitudeSquared)
			&& MagnitudeSquared > TargetIntentMinimumMagnitudeSquared
			&& MagnitudeSquared <= 1.0 + UnitDiscTolerance;
	}

	bool TryNormalizeTargetIntent(
		const FVector2D& Raw,
		FVector2D& OutCanonical)
	{
		OutCanonical = FVector2D::ZeroVector;
		if (!IsFiniteVector(Raw))
		{
			return false;
		}
		const double MagnitudeSquared = Raw.SizeSquared();
		if (!FMath::IsFinite(MagnitudeSquared)
			|| MagnitudeSquared <= TargetIntentMinimumMagnitudeSquared)
		{
			return false;
		}
		OutCanonical = MagnitudeSquared > 1.0
			? Raw / FMath::Sqrt(MagnitudeSquared)
			: Raw;
		OutCanonical.X = CanonicalInputScalar(OutCanonical.X);
		OutCanonical.Y = CanonicalInputScalar(OutCanonical.Y);
		OutCanonical = CanonicalVector(OutCanonical);
		return IsCanonicalTargetIntent(OutCanonical);
	}

	bool HasCanonicalCommandShape(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		if (Command.GetExpectedRevision() == MAX_uint64
			|| !IsKnownCommandKind(Command.GetKind()))
		{
			return false;
		}

		const bool bTargetIsZero =
			IsZeroVector(Command.GetArcTargetIntent());
		const bool bApexIsZero =
			Command.GetArcApexAdjustmentDelta() == 0.0;
		switch (Command.GetKind())
		{
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			SelectTrajectory:
			return IsKnownTrajectory(Command.GetTrajectoryKind())
				&& bTargetIsZero && bApexIsZero;
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			SetArcTargetIntent:
			return Command.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid
				&& IsCanonicalTargetIntent(Command.GetArcTargetIntent())
				&& bApexIsZero;
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			AdjustArcApex:
			return Command.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid
				&& bTargetIsZero
				&& FMath::IsFinite(Command.GetArcApexAdjustmentDelta())
				&& Command.GetArcApexAdjustmentDelta() >= -1.0
				&& Command.GetArcApexAdjustmentDelta() <= 1.0
				&& !bApexIsZero;
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			ClearArcTargetIntent:
			return Command.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid
				&& bTargetIsZero && bApexIsZero;
		default:
			return false;
		}
	}

	FGuid MakeCommandId(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		if (!HasCanonicalCommandShape(Command))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.InputChoiceCommand.r1"),
			{
				FString::Printf(
					TEXT("%llu"), Command.GetExpectedRevision()),
				FString::FromInt(static_cast<int32>(Command.GetKind())),
				FString::FromInt(
					static_cast<int32>(Command.GetTrajectoryKind())),
				DoubleBits(Command.GetArcTargetIntent().X),
				DoubleBits(Command.GetArcTargetIntent().Y),
				DoubleBits(Command.GetArcApexAdjustmentDelta())
			});
	}

	bool HasCanonicalStateShape(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		if (State.GetRevision() == MAX_uint64
			|| !IsKnownTrajectory(State.GetTrajectoryKind())
			|| !FMath::IsFinite(State.GetArcApexAdjustment())
			|| State.GetArcApexAdjustment() < -1.0
			|| State.GetArcApexAdjustment() > 1.0
			|| (State.GetRevision() == 0
				? State.GetLastCommandId().IsValid()
				: !State.GetLastCommandId().IsValid()))
		{
			return false;
		}

		if (State.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
		{
			return !State.HasArcTargetIntent()
				&& IsZeroVector(State.GetArcTargetIntent())
				&& State.GetArcApexAdjustment() == 0.0;
		}
		return State.HasArcTargetIntent()
			? IsCanonicalTargetIntent(State.GetArcTargetIntent())
			: IsZeroVector(State.GetArcTargetIntent());
	}

	FGuid MakeStateId(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		if (!HasCanonicalStateShape(State))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.InputChoiceState.r1"),
			{
				FString::Printf(TEXT("%llu"), State.GetRevision()),
				State.GetLastCommandId().IsValid()
					? State.GetLastCommandId().ToString(EGuidFormats::Digits)
					: TEXT("NONE"),
				FString::FromInt(
					static_cast<int32>(State.GetTrajectoryKind())),
				State.HasArcTargetIntent() ? TEXT("1") : TEXT("0"),
				DoubleBits(State.GetArcTargetIntent().X),
				DoubleBits(State.GetArcTargetIntent().Y),
				DoubleBits(State.GetArcApexAdjustment())
			});
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reject(
		const Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Accept(
		const Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.State = State;
		return Result;
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
TryCaptureTrajectorySelection(
	const uint64 InExpectedRevision,
	const Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind InTrajectoryKind,
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand();
	if (InExpectedRevision == MAX_uint64
		|| !IsKnownTrajectory(InTrajectoryKind))
	{
		return false;
	}
	OutCommand.ExpectedRevision = InExpectedRevision;
	OutCommand.Kind =
		Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::SelectTrajectory;
	OutCommand.TrajectoryKind = InTrajectoryKind;
	OutCommand.CommandId = MakeCommandId(OutCommand);
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
TryCaptureArcTargetIntent(
	const uint64 InExpectedRevision,
	const FVector2D& RawTargetIntent,
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand();
	FVector2D CanonicalTarget;
	if (InExpectedRevision == MAX_uint64
		|| !TryNormalizeTargetIntent(RawTargetIntent, CanonicalTarget))
	{
		return false;
	}
	OutCommand.ExpectedRevision = InExpectedRevision;
	OutCommand.Kind =
		Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::SetArcTargetIntent;
	OutCommand.ArcTargetIntent = CanonicalTarget;
	OutCommand.CommandId = MakeCommandId(OutCommand);
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
TryCaptureArcApexAdjustment(
	const uint64 InExpectedRevision,
	const double RawNormalizedDelta,
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand();
	if (InExpectedRevision == MAX_uint64
		|| !FMath::IsFinite(RawNormalizedDelta)
		|| RawNormalizedDelta == 0.0)
	{
		return false;
	}
	const double CanonicalDelta = CanonicalInputScalar(
		FMath::Clamp(RawNormalizedDelta, -1.0, 1.0));
	if (CanonicalDelta == 0.0)
	{
		return false;
	}
	OutCommand.ExpectedRevision = InExpectedRevision;
	OutCommand.Kind =
		Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::AdjustArcApex;
	OutCommand.ArcApexAdjustmentDelta = CanonicalDelta;
	OutCommand.CommandId = MakeCommandId(OutCommand);
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
TryCaptureArcTargetClear(
	const uint64 InExpectedRevision,
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand();
	if (InExpectedRevision == MAX_uint64)
	{
		return false;
	}
	OutCommand.ExpectedRevision = InExpectedRevision;
	OutCommand.Kind = Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
		ClearArcTargetIntent;
	OutCommand.CommandId = MakeCommandId(OutCommand);
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceCommand::IsValid() const
{
	return CommandId.IsValid()
		&& HasCanonicalCommandShape(*this)
		&& CommandId == MakeCommandId(*this);
}

Fdemo_mapShanmenThrownWeaponInputChoiceState
Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial()
{
	Fdemo_mapShanmenThrownWeaponInputChoiceState State;
	State.Revision = 0;
	State.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight;
	State.StateId = MakeStateId(State);
	return State;
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceState::TryRehydrate(
	const FGuid& ExpectedStateId,
	const FGuid& InLastCommandId,
	const uint64 InRevision,
	const Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
		InTrajectoryKind,
	const bool bInHasArcTargetIntent,
	const FVector2D& InArcTargetIntent,
	const double InArcApexAdjustment,
	Fdemo_mapShanmenThrownWeaponInputChoiceState& OutState)
{
	OutState = Fdemo_mapShanmenThrownWeaponInputChoiceState();
	Fdemo_mapShanmenThrownWeaponInputChoiceState Candidate;
	Candidate.LastCommandId = InLastCommandId;
	Candidate.Revision = InRevision;
	Candidate.TrajectoryKind = InTrajectoryKind;
	Candidate.bHasArcTargetIntent = bInHasArcTargetIntent;
	Candidate.ArcTargetIntent = InArcTargetIntent;
	Candidate.ArcApexAdjustment = InArcApexAdjustment;
	Candidate.StateId = MakeStateId(Candidate);
	if (!ExpectedStateId.IsValid()
		|| Candidate.StateId != ExpectedStateId
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutState = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceState::IsValid() const
{
	return StateId.IsValid()
		&& HasCanonicalStateShape(*this)
		&& StateId == MakeStateId(*this);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceState::Matches(
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& Other) const
{
	return IsValid() && Other.IsValid() && StateId == Other.StateId;
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult::IsSuccess() const
{
	return (Status
			== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Reduced
		|| Status
			== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::NoChange
		|| Status
			== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Replay)
		&& State.IsValid();
}

Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult
Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& PreviousState,
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
{
	if (!PreviousState.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::StateInvalid,
			TEXT("Thrown-weapon input choice requires one valid previous state."));
	}
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::CommandInvalid,
			TEXT("Thrown-weapon input choice rejected a malformed command."));
	}
	if (PreviousState.GetLastCommandId() == Command.GetCommandId()
		&& PreviousState.GetRevision() == Command.GetExpectedRevision() + 1)
	{
		return Accept(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Replay,
			TEXT("The exact input command was already reduced."),
			PreviousState);
	}
	if (PreviousState.GetRevision() != Command.GetExpectedRevision())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::
				RevisionMismatch,
			TEXT("Input command does not continue the consumer-owned revision."));
	}
	if (PreviousState.GetRevision() == MAX_uint64 - 1)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::
				RevisionExhausted,
			TEXT("Input choice revision is exhausted and cannot wrap."));
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState Candidate = PreviousState;
	switch (Command.GetKind())
	{
	case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::SelectTrajectory:
		if (PreviousState.GetTrajectoryKind() == Command.GetTrajectoryKind())
		{
			return Accept(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::NoChange,
				TEXT("Requested trajectory is already selected."),
				PreviousState);
		}
		Candidate.TrajectoryKind = Command.GetTrajectoryKind();
		Candidate.bHasArcTargetIntent = false;
		Candidate.ArcTargetIntent = FVector2D::ZeroVector;
		Candidate.ArcApexAdjustment = 0.0;
		break;
	case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::SetArcTargetIntent:
		if (PreviousState.GetTrajectoryKind()
			!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::ModeMismatch,
				TEXT("Arc target intent requires BallisticArc mode."));
		}
		if (PreviousState.HasArcTargetIntent()
			&& VectorsMatch(
				PreviousState.GetArcTargetIntent(),
				Command.GetArcTargetIntent()))
		{
			return Accept(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::NoChange,
				TEXT("Canonical Arc target intent is unchanged."),
				PreviousState);
		}
		Candidate.bHasArcTargetIntent = true;
		Candidate.ArcTargetIntent = Command.GetArcTargetIntent();
		break;
	case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::AdjustArcApex:
		if (PreviousState.GetTrajectoryKind()
			!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::ModeMismatch,
				TEXT("Arc apex adjustment requires BallisticArc mode."));
		}
		Candidate.ArcApexAdjustment = CanonicalZero(FMath::Clamp(
			PreviousState.GetArcApexAdjustment()
				+ Command.GetArcApexAdjustmentDelta(),
			-1.0,
			1.0));
		if (DoubleBits(Candidate.ArcApexAdjustment)
			== DoubleBits(PreviousState.GetArcApexAdjustment()))
		{
			return Accept(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::NoChange,
				TEXT("Arc apex adjustment is already at its normalized limit."),
				PreviousState);
		}
		break;
	case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
		ClearArcTargetIntent:
		if (PreviousState.GetTrajectoryKind()
			!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::ModeMismatch,
				TEXT("Arc target clear requires BallisticArc mode."));
		}
		if (!PreviousState.HasArcTargetIntent())
		{
			return Accept(
				Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::NoChange,
				TEXT("Arc target intent is already empty."),
				PreviousState);
		}
		Candidate.bHasArcTargetIntent = false;
		Candidate.ArcTargetIntent = FVector2D::ZeroVector;
		break;
	default:
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::CommandInvalid,
			TEXT("Thrown-weapon input choice command kind is invalid."));
	}

	Candidate.Revision = PreviousState.GetRevision() + 1;
	Candidate.LastCommandId = Command.GetCommandId();
	Candidate.StateId = MakeStateId(Candidate);
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::StateRejected,
			TEXT("Reduced input choice failed deterministic self-validation."));
	}
	return Accept(
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Reduced,
		TEXT("Thrown-weapon input choice advanced exactly once."),
		Candidate);
}
