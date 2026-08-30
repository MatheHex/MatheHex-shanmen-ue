#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritEvasionMotionRuntime.h"

class ACharacter;

/** Narrow capability consumed by the coordinator for one issued segment. */
class Idemo_mapShanmenSpiritEvasionSegmentExecutionPort
{
public:
	virtual ~Idemo_mapShanmenSpiritEvasionSegmentExecutionPort() = default;
	virtual Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execute(
		const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command) = 0;
};

/** Transient production port; the coordinator itself never stores an Actor. */
class Fdemo_mapShanmenSpiritEvasionCharacterExecutionPort final
	: public Idemo_mapShanmenSpiritEvasionSegmentExecutionPort
{
public:
	explicit Fdemo_mapShanmenSpiritEvasionCharacterExecutionPort(
		ACharacter* InCharacter)
		: Character(InCharacter)
	{
	}

	virtual Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execute(
		const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command) override;

private:
	ACharacter* Character = nullptr;
};

enum class Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus : uint8
{
	Invalid,
	Waiting,
	SegmentCommitted,
	Completed,
	Blocked,
	Terminated,
	Rejected
};

enum class Edemo_mapShanmenSpiritEvasionCoordinatorStepError : uint8
{
	None,
	CoordinatorNotReady,
	ActionMismatch,
	InvalidElapsed,
	ExecutionRejected,
	StateDesynchronized
};

/** Complete outcome of one product-owned lifecycle step. */
struct Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult
{
	Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus Status =
		Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Invalid;
	Edemo_mapShanmenSpiritEvasionCoordinatorStepError Error =
		Edemo_mapShanmenSpiritEvasionCoordinatorStepError::None;
	FGuid CoordinatorId;
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
	Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execution;
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt Termination;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Single product owner for an active spirit-evasion motion lifecycle.
 *
 * The coordinator binds one exact action window to one P10.3 motion session.
 * It owns scheduling and receipt acceptance, but no Actor, World, clock,
 * Timer, Tick, resource balance or input state. The product supplies the exact
 * action runtime, elapsed sample and a narrow execution capability per step.
 */
class Fdemo_mapShanmenSpiritEvasionActionCoordinator
{
public:
	static bool TryOpen(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& MotionPlan,
		const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult& Preflight,
		const FShanmenSpiritEvasionWindow& Window,
		const FShanmenActionOrchestrator& ActionRuntime,
		Fdemo_mapShanmenSpiritEvasionActionCoordinator& OutCoordinator);

	bool IsValid() const;
	bool IsActive() const;
	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult TryAdvance(
		const FShanmenActionOrchestrator& ActionRuntime,
		double ElapsedSeconds,
		Idemo_mapShanmenSpiritEvasionSegmentExecutionPort& ExecutionPort);
	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult TryTerminate(
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason);
	void Reset();

	const FGuid& GetCoordinatorId() const { return CoordinatorId; }
	const FShanmenSpiritEvasionWindow& GetWindow() const { return Window; }
	const Fdemo_mapShanmenSpiritEvasionMotionSession& GetMotionSession() const
	{
		return MotionSession;
	}

private:
	FGuid CoordinatorId;
	FShanmenSpiritEvasionWindow Window;
	Fdemo_mapShanmenSpiritEvasionMotionSession MotionSession;
	bool bOpen = false;
};
