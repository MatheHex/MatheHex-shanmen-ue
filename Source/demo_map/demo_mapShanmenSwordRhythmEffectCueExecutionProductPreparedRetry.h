#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.h"

/** Caller-owned root for one explicit retry renewal. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed
{
	FGuid RetrySeed;

	bool IsValid() const { return RetrySeed.IsValid(); }
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryChannels
	: uint8
{
	None,
	Visual,
	Audio,
	VisualAndAudio
};

/**
 * Immutable continuation of one retry-pending prepared Product dispatch.
 *
 * The source ProcessNext envelope remains part of the value's identity. A
 * renewed ProcessNext uses the next contiguous Host sequence, a fresh command
 * identity and fresh attempt identities only for channels that were pending.
 * Already-acknowledged channel attempts are retained verbatim. The value owns
 * no Host, executor, retry policy, timer, thread, queue or background work.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry&
			Other) const;

	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
	GetPreparedDispatch() const
	{
		return PreparedDispatch;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed&
	GetSeed() const
	{
		return Seed;
	}
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryChannels
	GetRenewedChannels() const
	{
		return RenewedChannels;
	}
	bool RenewsVisual() const;
	bool RenewsAudio() const;
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
	GetSourceProcessEnvelope() const
	{
		return SourceProcessEnvelope;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
	GetRetryProcessEnvelope() const
	{
		return RetryProcessEnvelope;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
	GetEndEnvelope() const
	{
		return EndEnvelope;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService;

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
		PreparedDispatch;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed Seed;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryChannels
		RenewedChannels =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryChannels::
				None;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope
		SourceProcessEnvelope;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope
		RetryProcessEnvelope;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope EndEnvelope;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus
	: uint8
{
	Prepared,
	PreparedDispatchInvalid,
	SeedInvalid,
	HostInvalid,
	PreparedRootMismatch,
	RetryStateUnavailable,
	CaptureRejected,
	StateInvalid
};

/** Evidence for one read-only retry-renewal preparation pass. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				PreparedDispatchInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry Prepared;

	bool IsPrepared() const;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus
	: uint8
{
	Completed,
	Resumed,
	Replayed,
	RetryPending,
	PreparedRejected,
	HostStateMismatch,
	ProcessRejected,
	EndRejected,
	StateInvalid
};

/** Complete evidence for one bounded prepared-retry execution pass. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				PreparedRejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry Prepared;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Process;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult End;
	int64 FinalNextSequence = 0;
	int32 FinalRecordCount = 0;
	bool bFinalTerminal = false;
	bool bAnyReplay = false;
	bool bAllReplay = false;

	bool IsCompleted() const;
	bool HasDurableProgress() const;
};

/** Stateless caller-driven retry renewal and one-shot execution boundary. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService
{
public:
	/**
	 * Freeze the next continuation from the Host's latest retry-pending receipt.
	 * This function does not mutate the Host or invoke executors.
	 */
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareResult
	PrepareRetry(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed&
			Seed);

	/**
	 * Route the frozen retry ProcessNext once and End once only if complete.
	 * Exact replay and Process-then-End resumption are receipt-driven.
	 */
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult
	TryExecutePreparedRetry(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry&
			Prepared,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
};
