#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.h"

/**
 * Parsed metadata from one canonical checkpoint-envelope manifest.
 *
 * The manifest identifies a caller-owned value but does not contain the
 * checkpoint payload required for standalone reconstruction.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifest
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifest&
			Other) const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	const FGuid& GetEnvelopeId() const { return EnvelopeId; }
	const FGuid& GetCheckpointId() const { return CheckpointId; }
	int32 GetRecordCount() const { return RecordCount; }

private:
	friend class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec;

	int32 SchemaVersion = 0;
	FGuid EnvelopeId;
	FGuid CheckpointId;
	int32 RecordCount = 0;
};

/**
 * Fixed-width canonical binary manifest codec for one P12.32 envelope.
 *
 * Layout: 8-byte magic, big-endian schema, envelope GUID, checkpoint GUID,
 * and big-endian record count. DecodeVerified is candidate-bound by design:
 * it never invents private lower-layer checkpoint state from metadata alone.
 * The codec performs no file/network IO, command execution, retry or async
 * work and owns no Host/executor.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec
{
public:
	static constexpr int32 EncodedSize()
	{
		return 48;
	}

	static bool TryEncode(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope&
			Envelope,
		TArray<uint8>& OutBytes);

	static bool TryDecode(
		const TArray<uint8>& Bytes,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifest&
			OutManifest);

	static bool TryVerify(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifest&
			Manifest,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope&
			ExpectedEnvelope);

	static bool TryDecodeVerified(
		const TArray<uint8>& Bytes,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope&
			ExpectedEnvelope,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope&
			OutEnvelope);
};
