#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.h"

/** Frozen transport identity plus exactly one P12.18 Router command. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope
{
public:
	static bool TryCapture(
		const FGuid& HostId,
		int64 Sequence,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			OutEnvelope);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Other) const;
	const FGuid& GetHostId() const { return HostId; }
	int64 GetSequence() const { return Sequence; }
	const FGuid& GetDispatchId() const { return DispatchId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& GetCommand()
		const
	{
		return Command;
	}

private:
	FGuid HostId;
	int64 Sequence = INDEX_NONE;
	FGuid DispatchId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
};

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus
	: uint8
{
	Routed,
	RouterRejected,
	EnvelopeInvalid,
	HostInvalid,
	HostIdentityConflict,
	SequenceGap,
	SequenceConflict,
	CommandReplayConflict,
	LifecycleConflict,
	ExecutorContextMismatch,
	StateInvalid
};

/** Stable transport receipt around one explicit P12.18 Router invocation. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
			EnvelopeInvalid;
	FGuid HostId;
	int64 Sequence = INDEX_NONE;
	FGuid DispatchId;
	FGuid CommandId;
	FGuid RunId;
	FGuid BatchId;
	bool bReplay = false;
	bool bHostStateCommitted = false;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Router;

	bool IsSuccess() const;
	bool IsDurableRecord() const;
	bool IsReplay() const { return bReplay; }
};

/** Frozen transport envelope plus its first durable Host receipt. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Envelope;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Result;

	bool IsValid() const;
};

/**
 * Run-local ordered transport boundary around exactly one P12.18 Router.
 *
 * Sequence zero binds the caller-owned HostId through one Create command.
 * Later envelopes must be contiguous. Exact historical envelopes replay the
 * stored Host receipt without Router or executor re-entry. A durable Router
 * receipt consumes its transport sequence even when one consumer rejected or
 * requested retry, because sibling acknowledgement state may already exist.
 * Reusing a Router CommandId at a fresh sequence is rejected without consuming
 * that sequence. A successful End forms a terminal fence while all historical
 * receipts remain replayable. This Host owns no executor, World, asset,
 * ProductSession, timer, thread, polling loop or background retry.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost
{
public:
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult TryRoute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
	TryRouteProcessNext(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsBound() const { return HostId.IsValid(); }
	bool IsTerminal() const { return bTerminal; }
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetBatchId() const { return BatchId; }
	int64 GetNextSequence() const { return NextSequence; }
	int32 GetRecordCount() const { return Records.Num(); }
	bool TryGetRecord(
		int64 Sequence,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord&
			OutRecord) const;
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter&
	GetRouter() const
	{
		return Router;
	}

private:
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
	TryRouteInternal(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor* VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor* AudioExecutor);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Commit(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult RouterResult,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Candidate);

	FGuid HostId;
	FGuid RunId;
	FGuid BatchId;
	int64 NextSequence = 0;
	bool bTerminal = false;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Router;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord>
		Records;
};
