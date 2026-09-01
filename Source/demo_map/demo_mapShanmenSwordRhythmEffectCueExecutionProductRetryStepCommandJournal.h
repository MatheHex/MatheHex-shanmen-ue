#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.h"

/** One immutable command receipt retained by the caller-owned journal. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord
{
public:
	bool IsValid() const;

	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand&
	GetCommand() const
	{
		return Receipt.GetCommand();
	}
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus
	GetAdapterStatus() const
	{
		return AdapterStatus;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt&
	GetReceipt() const
	{
		return Receipt;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal;

	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus
		AdapterStatus =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus::
				CommandInvalid;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt
		Receipt;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus
	: uint8
{
	Recorded,
	Replayed,
	CommandInvalid,
	CommandReplayConflict,
	AdapterRejected,
	StateInvalid
};

/** Result of one idempotent journal command submission. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus::
				CommandInvalid;
	FString Diagnostic;
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus
		AdapterStatus =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus::
				CommandInvalid;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt
		Receipt;
	bool bReplay = false;

	bool IsSuccess() const;
	bool IsReplay() const;
};

/**
 * Caller-owned idempotency ledger for P12.29 retry-step commands.
 *
 * It stores only valid immutable receipts. It owns no Host or executor, does
 * not schedule work, and never performs a retry on the caller's behalf.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal
{
public:
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalResult
	TryExecute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand&
			Command,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);

	bool IsValid() const;
	int32 GetRecordCount() const { return Records.Num(); }
	bool TryGetRecord(
		const FGuid& CommandId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord&
			OutRecord) const;

private:
	TArray<
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord>
		Records;
	TMap<FGuid, int32> RecordIndexByCommandId;
};
