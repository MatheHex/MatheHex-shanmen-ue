#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.h"

enum class Edemo_mapShanmenSwordRhythmEffectCueDriverStatus : uint8
{
	Executed,
	RetryRecorded,
	AttemptReplayed,
	NoOpExecuted,
	NoOpReplayed,
	AlreadyAcknowledged,
	CoordinatorInvalid,
	EventInvalid,
	AttemptInvalid,
	PreparationRejected,
	ExecutionRejected,
	StateInvalid
};

/** Complete evidence for one caller-driven consumer execution transaction. */
struct Fdemo_mapShanmenSwordRhythmEffectCueDriverResult
{
	Edemo_mapShanmenSwordRhythmEffectCueDriverStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::StateInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerPrepareResult Preparation;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult Execution;
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt
		ExistingAcknowledgement;

	bool IsSuccess() const;
	bool IsAcknowledged() const;
};

/**
 * Stateless, caller-driven composition of consumer preparation and execution.
 *
 * One call handles one immutable event and one caller-owned attempt identity.
 * The driver never polls, retries by itself, owns an executor, or reaches into
 * assets, World, components, timers, ProductSession or gameplay authority.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver
{
public:
	static Fdemo_mapShanmenSwordRhythmEffectCueDriverResult Process(
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& Coordinator,
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event,
		const FGuid& AttemptId,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& Executor);
};
