#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.h"

/**
 * Versioned immutable value envelope around one P12.31 checkpoint.
 *
 * This is a caller-owned schema boundary, not a byte/file/network codec. It
 * performs no IO, command execution, retry, scheduling, or Host ownership.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope
{
public:
	static constexpr int32 CurrentSchemaVersion()
	{
		return 1;
	}

	static bool TryWrap(
		int32 SchemaVersion,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint&
			Checkpoint,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope&
			OutEnvelope);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope&
			Other) const;
	bool TryUnwrap(
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint&
			OutCheckpoint) const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	const FGuid& GetEnvelopeId() const { return EnvelopeId; }
	int32 GetRecordCount() const { return Checkpoint.GetRecordCount(); }

private:
	int32 SchemaVersion = 0;
	FGuid EnvelopeId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint
		Checkpoint;
};
