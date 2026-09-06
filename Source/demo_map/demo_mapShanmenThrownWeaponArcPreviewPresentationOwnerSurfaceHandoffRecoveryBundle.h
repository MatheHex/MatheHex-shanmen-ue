#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.h"

/**
 * Canonical pairing of one current recovery journal and its pending checkpoint
 * payload. The bundle is evidence only: callers still need a current journal
 * match and the P20.48 live Owner/Host/Adapter authority gate.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle
{
public:
	static constexpr int32 MaximumGeneration()
	{
		return Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal::
			MaxRecordCount() / 2;
	}

	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
			Envelope,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			OutBundle);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			Other) const;
	bool TryCopyPendingCheckpointEvidenceForJournal(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			CurrentJournal,
		int32 MinimumGeneration,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			OutCheckpoint) const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	int32 GetGeneration() const { return Generation; }
	const FGuid& GetBundleId() const { return BundleId; }
	const FGuid& GetBundleDigest() const { return BundleDigest; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetJournal() const
	{
		return Journal;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
	GetEnvelope() const
	{
		return Envelope;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleCodec;

	int32 SchemaVersion = 0;
	int32 Generation = 0;
	FGuid BundleId;
	FGuid BundleDigest;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		Journal;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope
		Envelope;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus
	: uint8
{
	Invalid,
	Decoded,
	InputEmpty,
	MagicMismatch,
	UnsupportedSchema,
	SizeMismatch,
	GenerationOutOfRange,
	SectionSizeOutOfRange,
	BundleDigestMismatch,
	BundleIdentityMismatch,
	JournalDecodeRejected,
	PayloadDecodeRejected,
	JournalIdentityMismatch,
	EnvelopeIdentityMismatch,
	PairingRejected,
	NonCanonicalRepresentation
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeResult
{
public:
	bool IsSuccess() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetSourceSchemaVersion() const { return SourceSchemaVersion; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
	GetJournalDecodeStatus() const
	{
		return JournalDecodeStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
	GetPayloadDecodeStatus() const
	{
		return PayloadDecodeStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
	GetBundle() const
	{
		return Bundle;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleCodec;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus::
				Invalid;
	FString Diagnostic;
	int32 SourceSchemaVersion = 0;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
		JournalDecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
		PayloadDecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle
		Bundle;
};

/** Big-endian, bounded codec for one generation-bound recovery evidence pair. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleCodec
{
public:
	static constexpr int32 CurrentSchemaVersion() { return 1; }
	static constexpr int32 HeaderSize() { return 92; }
	static constexpr int32 MaximumJournalEncodedBytes()
	{
		return Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec::
			HeaderSize()
			+ Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal::
				MaxRecordCount()
				* Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec::
					CurrentRecordSize();
	}
	static constexpr int32 MaximumPayloadEncodedBytes()
	{
		return Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec::
			MaximumEncodedBytes();
	}
	static constexpr int32 MaximumEncodedBytes()
	{
		return HeaderSize() + MaximumJournalEncodedBytes()
			+ MaximumPayloadEncodedBytes();
	}

	static bool TryEncode(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			Bundle,
		TArray<uint8>& OutBytes);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeResult
	Decode(const TArray<uint8>& Bytes);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeResult
	MakeDecodeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus
			Status,
		const TCHAR* Diagnostic,
		int32 SourceSchemaVersion = 0,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
			JournalDecodeStatus =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
					Invalid,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
			PayloadDecodeStatus =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus::
					Invalid,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			Bundle = {});
};
