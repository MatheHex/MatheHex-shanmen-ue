#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritEvasionMovementAdapter.h"

class ACharacter;

/** Explicit linear-trajectory values selected by the product policy owner. */
struct Fdemo_mapShanmenSpiritEvasionTrajectoryCapture
{
	float DurationSeconds = 0.0f;
	int32 SegmentCount = 0;
};

/** Immutable timing and segmentation contract; it owns no clock or Tick. */
class Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenSpiritEvasionTrajectoryCapture& Capture,
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& OutTrajectory);

	bool IsValid() const;
	float GetDurationSeconds() const { return DurationSeconds; }
	int32 GetSegmentCount() const { return SegmentCount; }

private:
	float DurationSeconds = 0.0f;
	int32 SegmentCount = 0;
};

/** P10.2 movement plan bound to one explicit P10.3 trajectory. */
class Fdemo_mapShanmenSpiritEvasionMotionPlan
{
public:
	bool IsValid() const;
	const FGuid& GetMotionPlanId() const { return MotionPlanId; }
	const Fdemo_mapShanmenSpiritEvasionMovementPlan& GetMovementPlan() const
	{
		return MovementPlan;
	}
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& GetTrajectory() const
	{
		return Trajectory;
	}

private:
	friend struct Fdemo_mapShanmenSpiritEvasionMotionPlanner;

	FGuid MotionPlanId;
	Fdemo_mapShanmenSpiritEvasionMovementPlan MovementPlan;
	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
};

struct Fdemo_mapShanmenSpiritEvasionMotionPlanner
{
	static bool TryBuildPlan(
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& MovementPlan,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		Fdemo_mapShanmenSpiritEvasionMotionPlan& OutPlan);
};

/** One replay-stable, ordinal segment request; it performs no movement itself. */
class Fdemo_mapShanmenSpiritEvasionSegmentCommand
{
public:
	bool IsValid() const;
	const FGuid& GetCommandId() const { return CommandId; }
	const FGuid& GetSessionId() const { return SessionId; }
	const FGuid& GetMotionPlanId() const { return MotionPlanId; }
	int32 GetSegmentOrdinal() const { return SegmentOrdinal; }
	int32 GetSegmentCount() const { return SegmentCount; }
	double GetScheduledElapsedSeconds() const
	{
		return ScheduledElapsedSeconds;
	}
	float GetRequestedDistance() const { return RequestedDistance; }
	const FVector& GetPlanarDirection() const { return PlanarDirection; }

private:
	friend class Fdemo_mapShanmenSpiritEvasionMotionSession;

	FGuid CommandId;
	FGuid SessionId;
	FGuid MotionPlanId;
	int32 SegmentOrdinal = 0;
	int32 SegmentCount = 0;
	double ScheduledElapsedSeconds = 0.0;
	float RequestedDistance = 0.0f;
	FVector PlanarDirection = FVector::ZeroVector;
};

/** Actor-free receipt for the actual distance resolved by one swept move. */
class Fdemo_mapShanmenSpiritEvasionSegmentReceipt
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command,
		const Fdemo_mapCombatDisplacementResult& Displacement,
		Fdemo_mapShanmenSpiritEvasionSegmentReceipt& OutReceipt);

	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetCommandId() const { return Command.GetCommandId(); }
	const Fdemo_mapShanmenSpiritEvasionSegmentCommand& GetCommand() const
	{
		return Command;
	}
	float GetResolvedDistance() const { return ResolvedDistance; }
	bool WasBlocked() const { return bBlocked; }

private:
	FGuid ReceiptId;
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
	float ResolvedDistance = 0.0f;
	bool bBlocked = false;
};

enum class Edemo_mapShanmenSpiritEvasionMotionState : uint8
{
	Invalid,
	Active,
	Completed,
	Blocked
};

/**
 * Deterministic segment scheduler and receipt ledger.
 *
 * Elapsed time is supplied by the product caller. The session owns no World,
 * clock, Tick, Character or movement component.
 */
class Fdemo_mapShanmenSpiritEvasionMotionSession
{
public:
	static bool TryStart(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& MotionPlan,
		const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult& Preflight,
		Fdemo_mapShanmenSpiritEvasionMotionSession& OutSession);

	bool IsValid() const;
	bool TryIssueNextCommand(
		double ElapsedSeconds,
		Fdemo_mapShanmenSpiritEvasionSegmentCommand& OutCommand);
	bool TryAcceptReceipt(
		const Fdemo_mapShanmenSpiritEvasionSegmentReceipt& Receipt);

	const FGuid& GetSessionId() const { return SessionId; }
	const Fdemo_mapShanmenSpiritEvasionMotionPlan& GetMotionPlan() const
	{
		return MotionPlan;
	}
	Edemo_mapShanmenSpiritEvasionMotionState GetState() const { return State; }
	int32 GetAcceptedSegmentCount() const { return AcceptedSegmentCount; }
	float GetTargetDistance() const { return TargetDistance; }
	float GetResolvedDistance() const { return ResolvedDistance; }
	double GetLastElapsedSeconds() const { return LastElapsedSeconds; }
	bool HasPendingCommand() const { return PendingCommand.IsValid(); }
	const Fdemo_mapShanmenSpiritEvasionSegmentCommand& GetPendingCommand() const
	{
		return PendingCommand;
	}

private:
	FGuid SessionId;
	Fdemo_mapShanmenSpiritEvasionMotionPlan MotionPlan;
	Edemo_mapShanmenSpiritEvasionMotionState State =
		Edemo_mapShanmenSpiritEvasionMotionState::Invalid;
	float TargetDistance = 0.0f;
	float ResolvedDistance = 0.0f;
	int32 AcceptedSegmentCount = 0;
	double LastElapsedSeconds = 0.0;
	Fdemo_mapShanmenSpiritEvasionSegmentCommand PendingCommand;
};

enum class Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus : uint8
{
	InvalidCommand,
	CharacterUnavailable,
	MovementUnavailable,
	Committed,
	Blocked
};

/** Immediate product result; only committed/blocked results contain a receipt. */
struct Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult
{
	Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus Status =
		Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::InvalidCommand;
	FGuid CommandId;
	Fdemo_mapCombatDisplacementResult Displacement;
	Fdemo_mapShanmenSpiritEvasionSegmentReceipt Receipt;

	bool HasReceipt() const
	{
		return (Status
				== Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Committed
			|| Status
				== Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Blocked)
			&& Receipt.IsValid();
	}
};

/** Sole product mutation seam for one P10.3 segment. */
struct Fdemo_mapShanmenSpiritEvasionSegmentExecutor
{
	static Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult ExecuteSwept(
		ACharacter* Character,
		const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command);
};
