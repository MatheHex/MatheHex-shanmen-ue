#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.h"

/**
 * Journal-bound, self-validating payload for one pending P20.48 checkpoint.
 *
 * The envelope reconstructs immutable evidence only. It never authorizes a
 * recovery attempt: a caller must still match the latest pending journal and
 * pass the checkpoint to P20.48, which re-reads both live surfaces and the
 * unchanged Host/Adapter authority scope before committing any pointer.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope
{
public:
	static bool TryWrap(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
			OutEnvelope);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
			Other) const;
	bool MatchesJournal(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal) const;
	bool TryUnwrapForJournal(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			OutCheckpoint) const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	const FGuid& GetEnvelopeId() const { return EnvelopeId; }
	const FGuid& GetJournalId() const { return JournalId; }
	const FGuid& GetPayloadDigest() const { return PayloadDigest; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
	GetCheckpoint() const
	{
		return Checkpoint;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec;

	int32 SchemaVersion = 0;
	FGuid EnvelopeId;
	FGuid JournalId;
	FGuid PayloadDigest;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint
		Checkpoint;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
	: uint8
{
	Invalid,
	Decoded,
	InputEmpty,
	MagicMismatch,
	UnsupportedSchema,
	SizeMismatch,
	PayloadDigestMismatch,
	PayloadMalformed,
	SemanticReconstructionRejected,
	EnvelopeIdentityMismatch
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeResult
{
public:
	bool IsSuccess() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetSourceSchemaVersion() const { return SourceSchemaVersion; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
	GetEnvelope() const
	{
		return Envelope;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus::
				Invalid;
	FString Diagnostic;
	int32 SourceSchemaVersion = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope
		Envelope;
};

/** Big-endian canonical codec. It performs no file, network or surface IO. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec
{
public:
	static constexpr int32 CurrentSchemaVersion() { return 1; }
	static constexpr int32 HeaderSize() { return 64; }
	static constexpr int32 MaximumEncodedBytes() { return 64 * 1024; }
	static constexpr int32 MaximumStringBytes() { return 2048; }
	static constexpr int32 MaximumSourceTagCount() { return 64; }

	static bool TryEncode(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
			Envelope,
		TArray<uint8>& OutBytes);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeResult
	Decode(const TArray<uint8>& Bytes);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeResult
	MakeDecodeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
			Status,
		const TCHAR* Diagnostic,
		int32 SourceSchemaVersion = 0,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
			Envelope = {});
};
