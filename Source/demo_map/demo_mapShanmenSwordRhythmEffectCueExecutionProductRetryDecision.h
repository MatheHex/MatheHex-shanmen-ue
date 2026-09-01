#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.h"

/** Caller-owned retry limit and deterministic decision namespace. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionPolicy
{
	FGuid PolicySeed;
	int32 MaxRenewals = 0;

	bool IsValid() const
	{
		return PolicySeed.IsValid() && MaxRenewals >= 0;
	}
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionPolicy&
			Other) const
	{
		return IsValid() && Other.IsValid()
			&& PolicySeed == Other.PolicySeed
			&& MaxRenewals == Other.MaxRenewals;
	}
};

/** Explicit caller snapshot of how many retry renewals were already consumed. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionPolicy
		Policy;
	int32 RenewalsUsed = 0;

	bool IsValid() const
	{
		return Policy.IsValid()
			&& RenewalsUsed >= 0
			&& RenewalsUsed <= Policy.MaxRenewals;
	}
	bool HasBudget() const
	{
		return IsValid() && RenewalsUsed < Policy.MaxRenewals;
	}
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
			Other) const
	{
		return IsValid() && Other.IsValid()
			&& Policy.Matches(Other.Policy)
			&& RenewalsUsed == Other.RenewalsUsed;
	}
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome
	: uint8
{
	Retry,
	StopBudgetExhausted,
	StopCompleted,
	StopNotRetryable
};

/**
 * Immutable answer for one caller-requested retry decision.
 *
 * A Retry decision owns exactly one P12.26 prepared continuation. Stop
 * decisions retain the durable Host receipt and reason but intentionally do
 * not expose an executable continuation. The value owns no Host, executor,
 * counter, scheduler, timer, queue, thread or background work.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision&
			Other) const;
	bool ShouldRetry() const
	{
		return IsValid()
			&& Outcome
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome::
					Retry;
	}

	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
	GetPreparedDispatch() const
	{
		return PreparedDispatch;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
	GetRequest() const
	{
		return Request;
	}
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus
	GetPreparationStatus() const
	{
		return PreparationStatus;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed&
	GetRetrySeed() const
	{
		return RetrySeed;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord&
	GetObservedRecord() const
	{
		return ObservedRecord;
	}
	int64 GetObservedNextSequence() const
	{
		return ObservedNextSequence;
	}
	int32 GetObservedRecordCount() const
	{
		return ObservedRecordCount;
	}
	bool WasHostTerminal() const
	{
		return bObservedTerminal;
	}
	int32 GetNextRenewalsUsed() const
	{
		return NextRenewalsUsed;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry&
	GetPreparedRetry() const
	{
		return PreparedRetry;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService;

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
		PreparedDispatch;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest
		Request;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome
		Outcome =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome::
				StopNotRetryable;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus
		PreparationStatus =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				PreparedDispatchInvalid;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed
		RetrySeed;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord
		ObservedRecord;
	int64 ObservedNextSequence = 0;
	int32 ObservedRecordCount = 0;
	bool bObservedTerminal = false;
	int32 NextRenewalsUsed = 0;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry
		PreparedRetry;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionStatus
	: uint8
{
	Decided,
	RequestInvalid,
	PreparedDispatchInvalid,
	HostInvalid,
	PreparedRootMismatch,
	PreparationRejected,
	StateInvalid
};

/** Complete evidence for one read-only retry decision pass. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionStatus::
				RequestInvalid;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus
		PreparationStatus =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				PreparedDispatchInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision Decision;

	bool IsDecided() const;
};

/** Stateless budget-and-receipt decision boundary; never executes a retry. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionResult
	Decide(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
			Request);
};
