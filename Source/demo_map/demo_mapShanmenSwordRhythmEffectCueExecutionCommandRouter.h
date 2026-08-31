#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionSession.h"

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind : uint8
{
	Invalid,
	Create,
	ProcessNext,
	End
};

/** Frozen caller intent for exactly one execution-session operation. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand
{
public:
	static bool TryCaptureCreate(
		const FGuid& CommandId,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		const FGuid& VisualConsumerId,
		const FGuid& AudioConsumerId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand);
	static bool TryCaptureProcessNext(
		const FGuid& CommandId,
		const FGuid& RunId,
		const FGuid& BatchId,
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand);
	static bool TryCaptureEnd(
		const FGuid& CommandId,
		const FGuid& RunId,
		const FGuid& BatchId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Other) const;
	const FGuid& GetCommandId() const { return CommandId; }
	Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind GetKind() const
	{
		return Kind;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetBatchId() const { return BatchId; }
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& GetEvents() const
	{
		return Events;
	}
	const FGuid& GetVisualConsumerId() const { return VisualConsumerId; }
	const FGuid& GetAudioConsumerId() const { return AudioConsumerId; }
	const FGuid& GetVisualAttemptId() const { return VisualAttemptId; }
	const FGuid& GetAudioAttemptId() const { return AudioAttemptId; }

private:
	FGuid CommandId;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind Kind =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Invalid;
	FGuid RunId;
	FGuid BatchId;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	FGuid VisualConsumerId;
	FGuid AudioConsumerId;
	FGuid VisualAttemptId;
	FGuid AudioAttemptId;
};

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus : uint8
{
	Created,
	Processed,
	Ended,
	SessionRejected,
	CommandInvalid,
	RouterInvalid,
	CommandIdConflict,
	LifecycleConflict,
	IdentityMismatch,
	ExecutorContextMismatch,
	StateInvalid
};

/** Stable outer receipt around one explicit execution-session operation. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
			CommandInvalid;
	FGuid CommandId;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind Kind =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Invalid;
	FGuid RunId;
	FGuid BatchId;
	bool bReplay = false;
	bool bRouterStateCommitted = false;
	int32 CompletedEvents = 0;
	int32 TotalEvents = 0;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueSessionResult Session;

	bool IsSuccess() const;
	bool IsDurableRecord() const;
	bool IsReplay() const { return bReplay; }
};

/** Frozen command plus its first durable Router receipt. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Result;

	bool IsValid() const;
};

/**
 * Caller-owned command boundary around one P12.17 execution session.
 *
 * Create and End use the executor-free route. ProcessNext requires explicit
 * caller-owned Visual/Audio executors. One call routes one frozen command.
 * Exact durable replay returns stored evidence without re-entry. Partial
 * retry/reject receipts are durable because a sibling acknowledgement may
 * already be committed; recovery therefore requires a fresh CommandId and
 * failed-channel attempt. The Router owns no World, assets, ProductSession,
 * executor, timer, thread, polling or background retry.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter
{
public:
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult TryRoute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
	TryRouteProcessNext(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsBound() const { return RunId.IsValid() && BatchId.IsValid(); }
	bool IsEnded() const { return bEnded; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetBatchId() const { return BatchId; }
	int32 NumEvents() const { return TotalEvents; }
	int32 NumCompletedEvents() const;
	int32 GetRecordCount() const { return Records.Num(); }
	bool TryGetRecord(
		const FGuid& CommandId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord&
			OutRecord) const;
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession&
	GetSession() const
	{
		return Session;
	}

private:
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
	TryRouteInternal(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor* VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor* AudioExecutor);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Commit(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Result,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Candidate);

	FGuid RunId;
	FGuid BatchId;
	int32 TotalEvents = 0;
	bool bEnded = false;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Session;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord> Records;
};
