#include "demo_mapShanmenSpiritEvasionMotionRuntime.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString FloatBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(
			TEXT("%016llX"), static_cast<unsigned long long>(Bits));
	}

	bool IsCanonicalPlanarDirection(const FVector& Direction)
	{
		return FMath::IsFinite(Direction.X)
			&& FMath::IsFinite(Direction.Y)
			&& FMath::IsFinite(Direction.Z)
			&& Direction.Z == 0.0
			&& !Direction.IsNearlyZero()
			&& FMath::IsNearlyEqual(
				Direction.SizeSquared2D(), 1.0, 1.e-8);
	}

	FGuid MakeMotionPlanId(
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& MovementPlan,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory)
	{
		if (!MovementPlan.IsValid() || !Trajectory.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.MotionPlan.r1"),
			{
				GuidDigits(MovementPlan.GetPlanId()),
				FloatBits(Trajectory.GetDurationSeconds()),
				FString::FromInt(Trajectory.GetSegmentCount())
			});
	}

	bool PreflightMatchesPlan(
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& MovementPlan,
		const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult& Preflight)
	{
		return MovementPlan.IsValid()
			&& Preflight.Status
				== Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready
			&& Preflight.PlanId == MovementPlan.GetPlanId()
			&& FMath::IsFinite(Preflight.Displacement.RequestedDistance)
			&& FMath::IsFinite(Preflight.Displacement.ResolvedDistance)
			&& Preflight.Displacement.RequestedDistance
				== MovementPlan.GetPolicy().GetRequestedDistance()
			&& Fdemo_mapShanmenSpiritEvasionMovementAdapter::
				IsResolvedDistanceAccepted(
					MovementPlan,
					Preflight.Displacement.ResolvedDistance);
	}

	FGuid MakeSessionId(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& MotionPlan,
		float TargetDistance)
	{
		if (!MotionPlan.IsValid()
			|| !FMath::IsFinite(TargetDistance)
			|| TargetDistance <= 0.0f)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.MotionSession.r1"),
			{
				GuidDigits(MotionPlan.GetMotionPlanId()),
				FloatBits(TargetDistance)
			});
	}

	FGuid MakeCommandId(
		const FGuid& SessionId,
		const FGuid& MotionPlanId,
		int32 SegmentOrdinal,
		int32 SegmentCount,
		double ScheduledElapsedSeconds,
		float RequestedDistance,
		const FVector& PlanarDirection)
	{
		if (!SessionId.IsValid()
			|| !MotionPlanId.IsValid()
			|| SegmentOrdinal <= 0
			|| SegmentCount <= 0
			|| SegmentOrdinal > SegmentCount
			|| !FMath::IsFinite(ScheduledElapsedSeconds)
			|| ScheduledElapsedSeconds <= 0.0
			|| !FMath::IsFinite(RequestedDistance)
			|| RequestedDistance <= 0.0f
			|| !IsCanonicalPlanarDirection(PlanarDirection))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.SegmentCommand.r1"),
			{
				GuidDigits(SessionId),
				GuidDigits(MotionPlanId),
				FString::FromInt(SegmentOrdinal),
				FString::FromInt(SegmentCount),
				DoubleBits(ScheduledElapsedSeconds),
				FloatBits(RequestedDistance),
				DoubleBits(PlanarDirection.X),
				DoubleBits(PlanarDirection.Y),
				DoubleBits(PlanarDirection.Z)
			});
	}

	FGuid MakeReceiptId(
		const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command,
		float ResolvedDistance,
		bool bBlocked)
	{
		if (!Command.IsValid()
			|| !FMath::IsFinite(ResolvedDistance)
			|| ResolvedDistance < 0.0f
			|| ResolvedDistance
				> Command.GetRequestedDistance() + KINDA_SMALL_NUMBER)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.SegmentReceipt.r1"),
			{
				GuidDigits(Command.GetCommandId()),
				FloatBits(ResolvedDistance),
				bBlocked ? TEXT("blocked") : TEXT("committed")
			});
	}

	FGuid MakeTerminationReceiptId(
		const FGuid& SessionId,
		const FGuid& PendingCommandId,
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason,
		int32 AcceptedSegmentCount,
		float ResolvedDistance,
		double LastElapsedSeconds)
	{
		if (!SessionId.IsValid()
			|| Reason
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None
			|| AcceptedSegmentCount < 0
			|| !FMath::IsFinite(ResolvedDistance)
			|| ResolvedDistance < 0.0f
			|| !FMath::IsFinite(LastElapsedSeconds)
			|| LastElapsedSeconds < 0.0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.MotionTermination.r1"),
			{
				GuidDigits(SessionId),
				PendingCommandId.IsValid()
					? GuidDigits(PendingCommandId)
					: TEXT("none"),
				FString::FromInt(static_cast<uint8>(Reason)),
				FString::FromInt(AcceptedSegmentCount),
				FloatBits(ResolvedDistance),
				DoubleBits(LastElapsedSeconds)
			});
	}
}

bool Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
	const Fdemo_mapShanmenSpiritEvasionTrajectoryCapture& Capture,
	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& OutTrajectory)
{
	OutTrajectory = Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot();
	if (!FMath::IsFinite(Capture.DurationSeconds)
		|| Capture.DurationSeconds <= 0.0f
		|| Capture.SegmentCount <= 0)
	{
		return false;
	}
	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Candidate;
	Candidate.DurationSeconds = Capture.DurationSeconds;
	Candidate.SegmentCount = Capture.SegmentCount;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutTrajectory = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::IsValid() const
{
	return FMath::IsFinite(DurationSeconds)
		&& DurationSeconds > 0.0f
		&& SegmentCount > 0;
}

bool Fdemo_mapShanmenSpiritEvasionMotionPlan::IsValid() const
{
	return MotionPlanId.IsValid()
		&& MovementPlan.IsValid()
		&& Trajectory.IsValid()
		&& MotionPlanId == MakeMotionPlanId(MovementPlan, Trajectory);
}

bool Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
	const Fdemo_mapShanmenSpiritEvasionMovementPlan& MovementPlan,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
	Fdemo_mapShanmenSpiritEvasionMotionPlan& OutPlan)
{
	OutPlan = Fdemo_mapShanmenSpiritEvasionMotionPlan();
	if (!MovementPlan.IsValid() || !Trajectory.IsValid())
	{
		return false;
	}
	Fdemo_mapShanmenSpiritEvasionMotionPlan Candidate;
	Candidate.MovementPlan = MovementPlan;
	Candidate.Trajectory = Trajectory;
	Candidate.MotionPlanId = MakeMotionPlanId(
		Candidate.MovementPlan, Candidate.Trajectory);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPlan = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionSegmentCommand::IsValid() const
{
	return CommandId.IsValid()
		&& CommandId == MakeCommandId(
			SessionId,
			MotionPlanId,
			SegmentOrdinal,
			SegmentCount,
			ScheduledElapsedSeconds,
			RequestedDistance,
			PlanarDirection);
}

bool Fdemo_mapShanmenSpiritEvasionSegmentReceipt::TryCapture(
	const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command,
	const Fdemo_mapCombatDisplacementResult& Displacement,
	Fdemo_mapShanmenSpiritEvasionSegmentReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSpiritEvasionSegmentReceipt();
	if (!Command.IsValid()
		|| !FMath::IsFinite(Displacement.RequestedDistance)
		|| !FMath::IsFinite(Displacement.ResolvedDistance)
		|| Displacement.RequestedDistance != Command.GetRequestedDistance()
		|| Displacement.ResolvedDistance < 0.0f
		|| Displacement.ResolvedDistance
			> Displacement.RequestedDistance + KINDA_SMALL_NUMBER
		|| (!Displacement.bBlocked
			&& Displacement.ResolvedDistance + KINDA_SMALL_NUMBER
				< Displacement.RequestedDistance))
	{
		return false;
	}
	Fdemo_mapShanmenSpiritEvasionSegmentReceipt Candidate;
	Candidate.Command = Command;
	Candidate.ResolvedDistance = Displacement.ResolvedDistance;
	Candidate.bBlocked = Displacement.bBlocked;
	Candidate.ReceiptId = MakeReceiptId(
		Candidate.Command,
		Candidate.ResolvedDistance,
		Candidate.bBlocked);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionSegmentReceipt::IsValid() const
{
	return ReceiptId.IsValid()
		&& Command.IsValid()
		&& FMath::IsFinite(ResolvedDistance)
		&& ResolvedDistance >= 0.0f
		&& ResolvedDistance
			<= Command.GetRequestedDistance() + KINDA_SMALL_NUMBER
		&& (bBlocked
			|| ResolvedDistance + KINDA_SMALL_NUMBER
				>= Command.GetRequestedDistance())
		&& ReceiptId == MakeReceiptId(Command, ResolvedDistance, bBlocked);
}

bool Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt::IsValid() const
{
	return ReceiptId.IsValid()
		&& SessionId.IsValid()
		&& Reason
			!= Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None
		&& AcceptedSegmentCount >= 0
		&& FMath::IsFinite(ResolvedDistance)
		&& ResolvedDistance >= 0.0f
		&& FMath::IsFinite(LastElapsedSeconds)
		&& LastElapsedSeconds >= 0.0
		&& ReceiptId == MakeTerminationReceiptId(
			SessionId,
			PendingCommandId,
			Reason,
			AcceptedSegmentCount,
			ResolvedDistance,
			LastElapsedSeconds);
}

bool Fdemo_mapShanmenSpiritEvasionMotionSession::TryStart(
	const Fdemo_mapShanmenSpiritEvasionMotionPlan& MotionPlan,
	const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult& Preflight,
	Fdemo_mapShanmenSpiritEvasionMotionSession& OutSession)
{
	OutSession = Fdemo_mapShanmenSpiritEvasionMotionSession();
	if (!MotionPlan.IsValid()
		|| !PreflightMatchesPlan(MotionPlan.GetMovementPlan(), Preflight))
	{
		return false;
	}
	Fdemo_mapShanmenSpiritEvasionMotionSession Candidate;
	Candidate.MotionPlan = MotionPlan;
	Candidate.TargetDistance = Preflight.Displacement.ResolvedDistance;
	Candidate.SessionId = MakeSessionId(
		Candidate.MotionPlan, Candidate.TargetDistance);
	Candidate.State = Edemo_mapShanmenSpiritEvasionMotionState::Active;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSession = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionMotionSession::IsValid() const
{
	if (!SessionId.IsValid()
		|| !MotionPlan.IsValid()
		|| SessionId != MakeSessionId(MotionPlan, TargetDistance)
		|| !FMath::IsFinite(TargetDistance)
		|| !FMath::IsFinite(ResolvedDistance)
		|| !FMath::IsFinite(LastElapsedSeconds)
		|| TargetDistance <= 0.0f
		|| ResolvedDistance < 0.0f
		|| ResolvedDistance > TargetDistance + KINDA_SMALL_NUMBER
		|| LastElapsedSeconds < 0.0
		|| AcceptedSegmentCount < 0
		|| AcceptedSegmentCount
			> MotionPlan.GetTrajectory().GetSegmentCount())
	{
		return false;
	}
	const int32 SegmentCount = MotionPlan.GetTrajectory().GetSegmentCount();
	if (PendingCommand.IsValid()
		&& (State != Edemo_mapShanmenSpiritEvasionMotionState::Active
			|| PendingCommand.GetSessionId() != SessionId
			|| PendingCommand.GetMotionPlanId()
				!= MotionPlan.GetMotionPlanId()
			|| PendingCommand.GetSegmentOrdinal()
				!= AcceptedSegmentCount + 1
			|| PendingCommand.GetSegmentCount() != SegmentCount))
	{
		return false;
	}
	if (State == Edemo_mapShanmenSpiritEvasionMotionState::Terminated)
	{
		return !PendingCommand.IsValid()
			&& TerminationReason
				!= Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None
			&& TerminationReceipt.IsValid()
			&& TerminationReceipt.GetSessionId() == SessionId
			&& TerminationReceipt.GetReason() == TerminationReason
			&& TerminationReceipt.GetAcceptedSegmentCount()
				== AcceptedSegmentCount
			&& TerminationReceipt.GetResolvedDistance()
				== ResolvedDistance
			&& TerminationReceipt.GetLastElapsedSeconds()
				== LastElapsedSeconds;
	}
	if (TerminationReason
			!= Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None
		|| TerminationReceipt.IsValid())
	{
		return false;
	}
	if (State == Edemo_mapShanmenSpiritEvasionMotionState::Active)
	{
		return AcceptedSegmentCount < SegmentCount;
	}
	if (PendingCommand.IsValid())
	{
		return false;
	}
	return (State == Edemo_mapShanmenSpiritEvasionMotionState::Completed
			&& AcceptedSegmentCount == SegmentCount)
		|| (State == Edemo_mapShanmenSpiritEvasionMotionState::Blocked
			&& AcceptedSegmentCount > 0
			&& AcceptedSegmentCount <= SegmentCount);
}

bool Fdemo_mapShanmenSpiritEvasionMotionSession::TryIssueNextCommand(
	double ElapsedSeconds,
	Fdemo_mapShanmenSpiritEvasionSegmentCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSpiritEvasionSegmentCommand();
	if (!IsValid()
		|| State != Edemo_mapShanmenSpiritEvasionMotionState::Active
		|| PendingCommand.IsValid()
		|| !FMath::IsFinite(ElapsedSeconds)
		|| ElapsedSeconds < LastElapsedSeconds)
	{
		return false;
	}

	LastElapsedSeconds = ElapsedSeconds;
	const int32 SegmentCount = MotionPlan.GetTrajectory().GetSegmentCount();
	const int32 SegmentOrdinal = AcceptedSegmentCount + 1;
	const double ScheduledElapsedSeconds =
		static_cast<double>(MotionPlan.GetTrajectory().GetDurationSeconds())
		* static_cast<double>(SegmentOrdinal)
		/ static_cast<double>(SegmentCount);
	if (ElapsedSeconds + UE_DOUBLE_SMALL_NUMBER < ScheduledElapsedSeconds)
	{
		return false;
	}

	const double PriorTarget =
		static_cast<double>(TargetDistance)
		* static_cast<double>(SegmentOrdinal - 1)
		/ static_cast<double>(SegmentCount);
	const double NextTarget =
		static_cast<double>(TargetDistance)
		* static_cast<double>(SegmentOrdinal)
		/ static_cast<double>(SegmentCount);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Candidate;
	Candidate.SessionId = SessionId;
	Candidate.MotionPlanId = MotionPlan.GetMotionPlanId();
	Candidate.SegmentOrdinal = SegmentOrdinal;
	Candidate.SegmentCount = SegmentCount;
	Candidate.ScheduledElapsedSeconds = ScheduledElapsedSeconds;
	Candidate.RequestedDistance = static_cast<float>(NextTarget - PriorTarget);
	Candidate.PlanarDirection = MotionPlan.GetMovementPlan()
		.GetRequest().GetIntent().GetPlanarDirection();
	Candidate.CommandId = MakeCommandId(
		Candidate.SessionId,
		Candidate.MotionPlanId,
		Candidate.SegmentOrdinal,
		Candidate.SegmentCount,
		Candidate.ScheduledElapsedSeconds,
		Candidate.RequestedDistance,
		Candidate.PlanarDirection);
	if (!Candidate.IsValid())
	{
		return false;
	}
	PendingCommand = Candidate;
	OutCommand = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionMotionSession::TryAcceptReceipt(
	const Fdemo_mapShanmenSpiritEvasionSegmentReceipt& Receipt)
{
	if (!IsValid()
		|| State != Edemo_mapShanmenSpiritEvasionMotionState::Active
		|| !PendingCommand.IsValid()
		|| !Receipt.IsValid()
		|| Receipt.GetCommandId() != PendingCommand.GetCommandId())
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionMotionSession Candidate = *this;
	Candidate.ResolvedDistance += Receipt.GetResolvedDistance();
	++Candidate.AcceptedSegmentCount;
	Candidate.PendingCommand =
		Fdemo_mapShanmenSpiritEvasionSegmentCommand();
	if (Receipt.WasBlocked())
	{
		Candidate.State = Edemo_mapShanmenSpiritEvasionMotionState::Blocked;
	}
	else if (Candidate.AcceptedSegmentCount
		>= Candidate.MotionPlan.GetTrajectory().GetSegmentCount())
	{
		Candidate.State = Edemo_mapShanmenSpiritEvasionMotionState::Completed;
	}
	if (!Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionMotionSession::TryTerminate(
	Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason,
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt();
	if (!IsValid()
		|| State != Edemo_mapShanmenSpiritEvasionMotionState::Active
		|| Reason
			== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None)
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionMotionSession Candidate = *this;
	Candidate.TerminationReceipt.SessionId = Candidate.SessionId;
	Candidate.TerminationReceipt.PendingCommandId =
		Candidate.PendingCommand.IsValid()
			? Candidate.PendingCommand.GetCommandId()
			: FGuid();
	Candidate.TerminationReceipt.Reason = Reason;
	Candidate.TerminationReceipt.AcceptedSegmentCount =
		Candidate.AcceptedSegmentCount;
	Candidate.TerminationReceipt.ResolvedDistance =
		Candidate.ResolvedDistance;
	Candidate.TerminationReceipt.LastElapsedSeconds =
		Candidate.LastElapsedSeconds;
	Candidate.TerminationReceipt.ReceiptId = MakeTerminationReceiptId(
		Candidate.TerminationReceipt.SessionId,
		Candidate.TerminationReceipt.PendingCommandId,
		Candidate.TerminationReceipt.Reason,
		Candidate.TerminationReceipt.AcceptedSegmentCount,
		Candidate.TerminationReceipt.ResolvedDistance,
		Candidate.TerminationReceipt.LastElapsedSeconds);
	Candidate.PendingCommand =
		Fdemo_mapShanmenSpiritEvasionSegmentCommand();
	Candidate.TerminationReason = Reason;
	Candidate.State = Edemo_mapShanmenSpiritEvasionMotionState::Terminated;
	if (!Candidate.IsValid())
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	OutReceipt = TerminationReceipt;
	return true;
}

Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult
Fdemo_mapShanmenSpiritEvasionSegmentExecutor::ExecuteSwept(
	ACharacter* Character,
	const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command)
{
	Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Result;
	if (!Command.IsValid())
	{
		return Result;
	}
	Result.CommandId = Command.GetCommandId();
	if (!Character)
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
			CharacterUnavailable;
		return Result;
	}
	const UCharacterMovementComponent* Movement =
		Character->GetCharacterMovement();
	if (!Movement || !Movement->UpdatedComponent)
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
			MovementUnavailable;
		return Result;
	}

	Result.Displacement = Fdemo_mapCombatDisplacement::MoveCharacterSwept(
		Character,
		Command.GetPlanarDirection(),
		Command.GetRequestedDistance());
	if (!Fdemo_mapShanmenSpiritEvasionSegmentReceipt::TryCapture(
			Command, Result.Displacement, Result.Receipt))
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
			MovementUnavailable;
		return Result;
	}
	Result.Status = Result.Receipt.WasBlocked()
		? Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Blocked
		: Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Committed;
	return Result;
}
