#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionHost.h"

enum class Edemo_mapShanmenSwordRhythmEffectCueSessionStatus : uint8
{
	EventCompleted,
	BatchCompleted,
	AlreadyCompleted,
	RetryPending,
	EventRejected,
	SessionInvalid,
	AttemptInvalid,
	StateInvalid
};

/** Progress and host evidence for one caller-driven batch step. */
struct Fdemo_mapShanmenSwordRhythmEffectCueSessionResult
{
	Edemo_mapShanmenSwordRhythmEffectCueSessionStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::StateInvalid;
	FString Diagnostic;
	int32 EventIndex = INDEX_NONE;
	int32 CompletedEvents = 0;
	int32 TotalEvents = 0;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueHostResult Host;

	bool IsSuccess() const;
	bool IsBatchComplete() const;
};

/**
 * Caller-owned, Run-local execution session for one immutable event batch.
 *
 * The batch is non-empty, same-Run and strictly revision ordered. One call
 * processes only the current event through the Visual/Audio host. The cursor
 * advances only after both consumers acknowledge it. The session owns no
 * executors, World state, assets, timers, threads or ProductSession.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession
{
public:
	static bool TryCreate(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		const FGuid& VisualConsumerId,
		const FGuid& AudioConsumerId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession& OutSession);

	Fdemo_mapShanmenSwordRhythmEffectCueSessionResult ProcessNext(
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
	bool TryGetCurrentEvent(
		Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent) const;
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsComplete() const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetBatchId() const { return BatchId; }
	int32 NumEvents() const { return Events.Num(); }
	int32 NumCompletedEvents() const { return NextEventIndex; }
	int32 GetNextEventIndex() const { return NextEventIndex; }
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& GetEvents() const
	{
		return Events;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost& GetHost() const
	{
		return Host;
	}

private:
	FGuid RunId;
	FGuid BatchId;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	int32 NextEventIndex = INDEX_NONE;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Host;
};
