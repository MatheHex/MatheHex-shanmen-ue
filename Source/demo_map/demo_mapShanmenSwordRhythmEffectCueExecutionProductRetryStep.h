#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.h"

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus
	: uint8
{
	Completed,
	RetryPending,
	StoppedBudgetExhausted,
	StoppedCompleted,
	StoppedNotRetryable,
	DecisionRejected,
	ExecutionRejected,
	StateInvalid
};

/**
 * Evidence from one caller-requested retry step.
 *
 * A step decides exactly once and executes at most one prepared continuation.
 * RetryPending returns the next caller-owned renewal count, but never loops,
 * schedules, sleeps, queues or retains the Host/executors for later work.
 */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus::
			DecisionRejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionResult
		Decision;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult
		Execution;
	int32 NextRenewalsUsed = 0;
	int64 FinalNextSequence = 0;
	int32 FinalRecordCount = 0;
	bool bFinalTerminal = false;

	/** True only for a complete, pending or intentional stop outcome. */
	bool IsHandled() const;

	/** True when this step handed exactly one prepared continuation to P12.26. */
	bool DidExecuteRetry() const;

	/** Caller may explicitly request another step with NextRenewalsUsed. */
	bool CanRequestAnotherStep() const;
};

/** Stateless one-decision/at-most-one-execution retry boundary. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult
	TryRunStep(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
			Request);
};
