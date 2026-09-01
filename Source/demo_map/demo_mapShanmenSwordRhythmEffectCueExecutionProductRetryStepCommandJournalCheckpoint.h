#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.h"

/** Immutable ordered checkpoint for one caller-owned retry-command journal. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint
{
public:
	static bool TryCreate(
		const TArray<
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord>&
			OrderedRecords,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint&
			OutCheckpoint);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint&
			Other) const;
	const FGuid& GetCheckpointId() const { return CheckpointId; }
	int32 GetRecordCount() const { return Records.Num(); }
	bool TryGetRecordAt(
		int32 Index,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord&
			OutRecord) const;

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointService;

	FGuid CheckpointId;
	TArray<
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord>
		Records;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointStatus
	: uint8
{
	Exported,
	Restored,
	CheckpointInvalid,
	JournalInvalid,
	TargetNotEmpty,
	RestoreStateInvalid
};

struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointStatus::
				CheckpointInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint
		Checkpoint;

	bool IsSuccess() const;
};

/**
 * Pure checkpoint boundary for caller-managed persistence.
 *
 * It performs no file IO, command execution, retry, scheduling, or Host /
 * executor ownership. Restore only accepts a valid empty target journal.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointService
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointResult
	Export(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal&
			Journal);

	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointResult
	Restore(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint&
			Checkpoint,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal&
			InOutJournal);
};
