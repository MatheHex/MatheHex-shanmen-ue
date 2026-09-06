#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind
	: uint8
{
	Invalid,
	CheckpointPrepared,
	RecoveryCommitted
};

/**
 * Standalone canonical metadata for one checkpoint or recovery receipt.
 *
 * This record is an audit/rebinding manifest. It deliberately stores stable
 * identities rather than the private Arc presentation payload, so decoding a
 * journal never manufactures a P20.48 checkpoint or authorizes recovery.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord&
			Other) const;
	bool MatchesCheckpoint(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint) const;
	bool MatchesRecoveryReceipt(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			Receipt) const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind
	GetKind() const
	{
		return Kind;
	}
	int32 GetSequence() const { return Sequence; }
	const FGuid& GetRecordId() const { return RecordId; }
	const FGuid& GetPreviousRecordId() const { return PreviousRecordId; }
	const FGuid& GetCheckpointId() const { return CheckpointId; }
	const FGuid& GetTransitionTicketId() const { return TransitionTicketId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetConsumerDefinitionDigest() const
	{
		return ConsumerDefinitionDigest;
	}
	const FGuid& GetSourceRetirementResponseId() const
	{
		return SourceRetirementResponseId;
	}
	const FGuid& GetPreviousSurfaceInstanceId() const
	{
		return PreviousSurfaceInstanceId;
	}
	const FGuid& GetSurfaceInstanceId() const { return SurfaceInstanceId; }
	const FGuid& GetPreviousSurfaceCursorId() const
	{
		return PreviousSurfaceCursorId;
	}
	const FGuid& GetSurfaceCursorId() const { return SurfaceCursorId; }
	const FGuid& GetRecoveryReceiptId() const { return RecoveryReceiptId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
	GetSourceFailureStatus() const
	{
		return SourceFailureStatus;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind
		Kind =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind::
				Invalid;
	int32 Sequence = INDEX_NONE;
	FGuid RecordId;
	FGuid PreviousRecordId;
	FGuid CheckpointId;
	FGuid TransitionTicketId;
	FGuid RunId;
	FGuid ConsumerDefinitionDigest;
	FGuid SourceRetirementResponseId;
	FGuid PreviousSurfaceInstanceId;
	FGuid SurfaceInstanceId;
	FGuid PreviousSurfaceCursorId;
	FGuid SurfaceCursorId;
	FGuid RecoveryReceiptId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
		SourceFailureStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus::
				Invalid;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition
	: uint8
{
	Empty,
	CheckpointPending,
	RecoveryCommitted
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus
	: uint8
{
	Invalid,
	Appended,
	Replayed,
	JournalInvalid,
	CheckpointInvalid,
	ReceiptInvalid,
	PendingCheckpointConflict,
	CheckpointNotPending,
	CapacityExceeded,
	RecordInvariantViolation
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult
{
public:
	bool IsValid() const;
	bool IsSuccess() const;
	bool DidAppend() const;
	bool IsReplay() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetPreviousRecordCount() const { return PreviousRecordCount; }
	int32 GetRecordCount() const { return RecordCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord&
	GetRecord() const
	{
		return Record;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus::
				Invalid;
	FString Diagnostic;
	int32 PreviousRecordCount = 0;
	int32 RecordCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord
		Record;
};

/**
 * Caller-owned append-only metadata journal with a strict capacity.
 *
 * It owns no surface, Owner, file handle, retry loop or scheduler. Rejection
 * never evicts or rewrites an earlier record.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
{
public:
	static constexpr int32 MaxRecordCount()
	{
		return 16;
	}

	bool IsValid() const;
	FGuid GetJournalId() const;
	int32 GetRecordCount() const { return Records.Num(); }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition
	GetLatestDisposition() const;
	bool TryGetRecordAt(
		int32 Index,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord&
			OutRecord) const;
	bool TryGetLatestRecord(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord&
			OutRecord) const;
	bool MatchesLatestCheckpoint(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint) const;
	bool MatchesLatestRecovery(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			Receipt) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult
	AppendCheckpoint(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult
	AppendRecoveryReceipt(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			Receipt);

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus
			Status,
		const TCHAR* Diagnostic,
		int32 PreviousRecordCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord&
			Record = {}) const;
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord
	MakeRecord(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind
			Kind,
		int32 Sequence,
		const FGuid& PreviousRecordId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			Receipt);

	TArray<
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord>
		Records;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
	: uint8
{
	Invalid,
	DecodedCurrent,
	MigratedPrevious,
	InputEmpty,
	MagicMismatch,
	UnsupportedSchema,
	SizeMismatch,
	RecordCountOutOfRange,
	JournalIdentityMismatch,
	RecordInvalid,
	JournalInvalid,
	PreviousSchemaShapeUnsupported
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeResult
{
public:
	bool IsSuccess() const;
	bool WasMigrated() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetSourceSchemaVersion() const { return SourceSchemaVersion; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetJournal() const
	{
		return Journal;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
				Invalid;
	FString Diagnostic;
	int32 SourceSchemaVersion = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		Journal;
};

/**
 * Big-endian canonical binary codec for the bounded metadata journal.
 *
 * Current schema carries explicit sequence/predecessor/record identities.
 * N-1 can represent one initial pending checkpoint and migrates it into the
 * current chain model. Decode performs no IO and never reconstructs private
 * checkpoint or receipt payloads.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec
{
public:
	static constexpr int32 CurrentSchemaVersion() { return 2; }
	static constexpr int32 PreviousSchemaVersion() { return 1; }
	static constexpr int32 HeaderSize() { return 32; }
	static constexpr int32 CurrentRecordSize() { return 204; }
	static constexpr int32 PreviousRecordSize() { return 168; }

	static bool TryEncode(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal,
		TArray<uint8>& OutBytes);
	static bool TryEncodePreviousSchema(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal,
		TArray<uint8>& OutBytes);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeResult
	Decode(const TArray<uint8>& Bytes);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeResult
	MakeDecodeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
			Status,
		const TCHAR* Diagnostic,
		int32 SourceSchemaVersion = 0,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			Journal = {});
};
