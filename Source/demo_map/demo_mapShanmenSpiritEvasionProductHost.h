#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritEvasionActionCoordinator.h"

class ACharacter;

/** Narrow read-only collision capability used once while opening a host. */
class Idemo_mapShanmenSpiritEvasionPreflightPort
{
public:
	virtual ~Idemo_mapShanmenSpiritEvasionPreflightPort() = default;
	virtual Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Evaluate(
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan) = 0;
};

/** Transient production preflight adapter; the host never stores a Character. */
class Fdemo_mapShanmenSpiritEvasionCharacterPreflightPort final
	: public Idemo_mapShanmenSpiritEvasionPreflightPort
{
public:
	explicit Fdemo_mapShanmenSpiritEvasionCharacterPreflightPort(
		const ACharacter* InCharacter)
		: Character(InCharacter)
	{
	}

	virtual Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Evaluate(
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan) override;

private:
	const ACharacter* Character = nullptr;
};

enum class Edemo_mapShanmenSpiritEvasionHostStartStatus : uint8
{
	Rejected,
	Started
};

enum class Edemo_mapShanmenSpiritEvasionHostStartError : uint8
{
	None,
	InvalidInput,
	ActionStartRejected,
	ActionCommitRejected,
	WindowRejected,
	IntentRejected,
	MovementRequestRejected,
	MovementPlanRejected,
	PreflightRejected,
	MotionPlanRejected,
	CoordinatorRejected,
	StateDesynchronized
};

/** Full proof that one host owns the exact action-to-motion chain. */
struct Fdemo_mapShanmenSpiritEvasionHostStartResult
{
	Edemo_mapShanmenSpiritEvasionHostStartStatus Status =
		Edemo_mapShanmenSpiritEvasionHostStartStatus::Rejected;
	Edemo_mapShanmenSpiritEvasionHostStartError Error =
		Edemo_mapShanmenSpiritEvasionHostStartError::InvalidInput;
	FGuid HostId;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt ActiveCommit;
	FShanmenSpiritEvasionWindowReceipt Window;
	Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Preflight;

	bool IsValid() const;
	bool IsSuccess() const;
};

enum class Edemo_mapShanmenSpiritEvasionHostStepStatus : uint8
{
	Rejected,
	Waiting,
	SegmentCommitted,
	RecoveryCompleted,
	RecoveryBlocked,
	Completed,
	Cancelled,
	Interrupted,
	OwnerEnded,
	ExecutionUnavailable,
	AlreadyTerminal
};

enum class Edemo_mapShanmenSpiritEvasionHostStepError : uint8
{
	None,
	HostNotReady,
	HostNotActive,
	InvalidTime,
	CoordinatorRejected,
	ActionTransitionRejected,
	StateDesynchronized
};

/** One host operation with nested motion and action lifecycle evidence. */
struct Fdemo_mapShanmenSpiritEvasionHostStepResult
{
	Edemo_mapShanmenSpiritEvasionHostStepStatus Status =
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Rejected;
	Edemo_mapShanmenSpiritEvasionHostStepError Error =
		Edemo_mapShanmenSpiritEvasionHostStepError::HostNotReady;
	FGuid HostId;
	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Motion;
	FShanmenActionTransitionReceipt ActionTransition;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Sole product owner for one spirit-evasion action and its motion coordinator.
 *
 * The caller supplies explicit policy, direction, absolute time samples and
 * narrow World/movement ports. The host owns the action runtime and converts
 * absolute samples to relative motion elapsed time. It stores no Actor, World,
 * component, Tick, Timer, input binding or resource balance.
 */
class Fdemo_mapShanmenSpiritEvasionProductHost
{
public:
	static Fdemo_mapShanmenSpiritEvasionHostStartResult TryStart(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		const FVector& CandidateDirection,
		double StartTimeSeconds,
		Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort,
		Fdemo_mapShanmenSpiritEvasionProductHost& OutHost);
	static Fdemo_mapShanmenSpiritEvasionHostStartResult TryStartCharacter(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		const FVector& CandidateDirection,
		double StartTimeSeconds,
		const ACharacter* Character,
		Fdemo_mapShanmenSpiritEvasionProductHost& OutHost);

	Fdemo_mapShanmenSpiritEvasionHostStepResult TryAdvance(
		double NowSeconds,
		Idemo_mapShanmenSpiritEvasionSegmentExecutionPort& ExecutionPort);
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryAdvanceCharacter(
		double NowSeconds,
		ACharacter* Character);
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryCancel();
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryInterrupt();
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryOwnerEnd();
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryFinishRecovery();
	/** Projects the still-active canonical window without mutating the Host. */
	bool TryProjectDefenseLayer(
		FShanmenSpiritEvasionProjectionReceipt& OutReceipt) const;

	bool IsValid() const;
	bool IsActive() const;
	bool IsRecovery() const;
	bool IsTerminal() const;
	const FGuid& GetHostId() const { return HostId; }
	double GetStartTimeSeconds() const { return StartTimeSeconds; }
	double GetLastObservedTimeSeconds() const;
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const Fdemo_mapShanmenSpiritEvasionActionCoordinator& GetCoordinator() const
	{
		return Coordinator;
	}

private:
	Fdemo_mapShanmenSpiritEvasionHostStepResult Terminate(
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason MotionReason,
		Edemo_mapShanmenSpiritEvasionHostStepStatus ResultStatus);

	FGuid HostId;
	double StartTimeSeconds = 0.0;
	FShanmenActionOrchestrator ActionRuntime;
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Coordinator;
	bool bStarted = false;
};
