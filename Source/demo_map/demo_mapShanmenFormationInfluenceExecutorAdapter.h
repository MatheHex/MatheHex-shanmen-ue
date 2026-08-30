#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceEvaluationBinding.h"
#include "demo_mapShanmenFormationProductHost.h"

/** Caller-owned identity for exactly one canonical execution attempt. */
struct Fdemo_mapShanmenFormationInfluenceExecutionCommand
{
	FGuid IntentId;
	FGuid AttemptId;
	Fdemo_mapShanmenFormationInfluenceEvaluationBinding Evaluation;

	bool IsValid() const;
};

/** Read-only envelope supplied to an injected executor. */
struct Fdemo_mapShanmenFormationInfluenceExecutorInvocation
{
	FGuid LedgerId;
	Fdemo_mapShanmenFormationInfluenceIntent Intent;
	FGuid AttemptId;
	Fdemo_mapShanmenFormationInfluenceEvaluationBinding Evaluation;

	bool IsValid() const;
};

/**
 * Opaque executor evidence sealed back to the invocation.
 *
 * The adapter does not interpret magnitude, duration, stacking, handles, or
 * product objects. The invocation carries already-evaluated evidence while
 * ExecutorReceiptId remains owned by the injected executor.
 */
struct Fdemo_mapShanmenFormationInfluenceExecutorReceipt
{
	FGuid LedgerId;
	FGuid IntentId;
	FGuid AttemptId;
	FGuid ExecutorReceiptId;
	Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome =
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation)
		const;
};

enum class Edemo_mapShanmenFormationInfluenceExecutorStatus : uint8
{
	Completed,
	Rejected
};

struct Fdemo_mapShanmenFormationInfluenceExecutorResult
{
	Edemo_mapShanmenFormationInfluenceExecutorStatus Status =
		Edemo_mapShanmenFormationInfluenceExecutorStatus::Rejected;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceExecutorReceipt Receipt;

	bool IsSuccess() const;
};

/** Narrow injectable effect boundary; implementations own no Host state. */
class Idemo_mapShanmenFormationInfluenceExecutor
{
public:
	virtual ~Idemo_mapShanmenFormationInfluenceExecutor() = default;

	virtual Fdemo_mapShanmenFormationInfluenceExecutorResult Execute(
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation)
		= 0;
};

enum class Edemo_mapShanmenFormationInfluenceExecutionStatus : uint8
{
	Succeeded,
	RetryRecorded,
	AttemptReplayed,
	HostInvalid,
	CorrelationMismatch,
	CommandInvalid,
	LedgerUnavailable,
	NoPendingIntent,
	IntentOutOfOrder,
	ExecutorRejected,
	ExecutorEvidenceMismatch,
	AcknowledgementRejected,
	StateInvalid
};

/** One adapter call with executor and Host acknowledgement evidence. */
struct Fdemo_mapShanmenFormationInfluenceExecutionResult
{
	Edemo_mapShanmenFormationInfluenceExecutionStatus Status =
		Edemo_mapShanmenFormationInfluenceExecutionStatus::CommandInvalid;
	FString Diagnostic;
	bool bExecutorInvoked = false;
	Fdemo_mapShanmenFormationInfluenceExecutorInvocation Invocation;
	Fdemo_mapShanmenFormationInfluenceExecutorResult Executor;
	Fdemo_mapShanmenFormationHostInfluenceResult HostAcknowledgement;

	bool IsSuccess() const;
};

/**
 * Stateless bridge from the Host's canonical pending intent to one executor.
 *
 * Existing attempt evidence is replayed without invoking the executor again.
 * New attempts may address only the current first pending intent. The Host
 * ledger remains the sole retry, ordering, acknowledgement, and seal authority.
 */
class Fdemo_mapShanmenFormationInfluenceExecutorAdapter
{
public:
	static Fdemo_mapShanmenFormationInfluenceExecutionResult TryExecute(
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationInfluenceExecutionCommand& Command,
		Idemo_mapShanmenFormationInfluenceExecutor& Executor);
};
