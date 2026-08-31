#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.h"

#include "demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.generated.h"

/** Immutable all-or-nothing cue batch supplied to one injected executor. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const FGuid& AttemptId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& OutInvocation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Other)
		const;
	const FGuid& GetInvocationId() const { return InvocationId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& GetRoute() const
	{
		return Route;
	}
	const FGuid& GetAttemptId() const { return AttemptId; }
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& GetCommands()
		const
	{
		return Route.GetCommands();
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	FGuid InvocationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Route;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	FGuid AttemptId;
};

/** Opaque all-or-nothing executor evidence sealed to one invocation. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation,
		const FGuid& ExecutorReceiptId,
		Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt& OutReceipt);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
		const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
		GetInvocation() const
	{
		return Invocation;
	}
	const FGuid& GetExecutorReceiptId() const { return ExecutorReceiptId; }
	Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome GetOutcome() const
	{
		return Outcome;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Invocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	FGuid ExecutorReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Executor", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome =
		Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::RetryableFailure;
};

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus : uint8
{
	Completed,
	Rejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt Receipt;

	bool IsSuccess() const;
};

/**
 * Narrow injected batch boundary.
 *
 * Implementations must treat the invocation as one all-or-nothing batch and
 * return one opaque batch receipt. They own no coordinator or delivery state.
 */
class Idemo_mapShanmenSwordRhythmEffectCueExecutor
{
public:
	virtual ~Idemo_mapShanmenSwordRhythmEffectCueExecutor() = default;

	virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
		= 0;
};

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus : uint8
{
	Succeeded,
	RetryRecorded,
	AttemptReplayed,
	NoOpSucceeded,
	NoOpReplayed,
	CoordinatorInvalid,
	RouteInvalid,
	ScopeMismatch,
	AttemptInvalid,
	RoutePending,
	RouteUnavailable,
	InvocationRejected,
	ExecutorRejected,
	ExecutorEvidenceMismatch,
	AttemptRejected,
	StateInvalid
};

/** Complete adapter evidence for one caller-driven execution attempt. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid;
	FString Diagnostic;
	bool bExecutorInvoked = false;
	bool bNoOp = false;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Invocation;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Executor;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult Submission;

	bool IsSuccess() const;
};

/**
 * Stateless bridge from one prepared consumer route to an injected executor.
 *
 * Existing attempts replay from coordinator evidence without invoking the
 * executor. Zero-command routes commit through explicit NoOpSucceeded without
 * constructing an executor invocation. All other routes invoke exactly once,
 * validate opaque batch evidence, then submit it to the coordinator.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter
{
public:
	static Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult TryExecute(
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& Coordinator,
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const FGuid& AttemptId,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& Executor);
};
