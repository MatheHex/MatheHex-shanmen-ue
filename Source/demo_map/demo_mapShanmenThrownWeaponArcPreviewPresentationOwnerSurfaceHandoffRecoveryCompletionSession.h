#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.h"

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;

using Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem =
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem;
using Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileWriteStatus =
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileWriteStatus;
using Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileReadStatus =
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileReadStatus;

/** Explicit caller intent for one exact durable recovery-completion closure. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest
{
public:
	static bool TryCreate(
		const FGuid& CompletionAuthorityDomainId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest&
			AdmissionRequest,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
			OutRequest,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetCompletionAuthorityDomainId() const
	{
		return CompletionAuthorityDomainId;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest&
	GetAdmissionRequest() const
	{
		return AdmissionRequest;
	}
	const FGuid& GetRequestId() const { return RequestId; }

private:
	FGuid CompletionAuthorityDomainId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest
		AdmissionRequest;
	FGuid RequestId;
};

/**
 * Canonical terminal evidence for one completed recovery generation.
 *
 * SourceJournal remains the immutable pending evidence. TerminalJournal is a
 * distinct append-only extension containing the exact P20.48 receipt. The
 * old P20.53 pending bundle is never rewritten or relabelled as terminal.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult&
			AdmissionResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			SourceJournal,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			TerminalJournal,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			OutCompletion);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			Other) const;
	bool MatchesRequest(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
			Request) const;
	bool MatchesCurrentJournal(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			CurrentJournal) const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	int32 GetGeneration() const { return Generation; }
	const FGuid& GetCompletionId() const { return CompletionId; }
	const FGuid& GetCompletionDigest() const { return CompletionDigest; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
	GetRequest() const
	{
		return Request;
	}
	const FGuid& GetSourceBundleId() const { return SourceBundleId; }
	const FGuid& GetRecoveryReceiptId() const { return RecoveryReceiptId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetSourceJournal() const
	{
		return SourceJournal;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetTerminalJournal() const
	{
		return TerminalJournal;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionCodec;

	int32 SchemaVersion = 0;
	int32 Generation = 0;
	FGuid CompletionId;
	FGuid CompletionDigest;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest
		Request;
	FGuid SourceBundleId;
	FGuid RecoveryReceiptId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		SourceJournal;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		TerminalJournal;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus
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
	IdentityMismatch,
	DigestMismatch,
	SourceJournalRejected,
	TerminalJournalRejected,
	JournalIdentityMismatch,
	CompletionRejected,
	NonCanonicalRepresentation
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeResult
{
public:
	bool IsSuccess() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetSourceSchemaVersion() const { return SourceSchemaVersion; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
	GetSourceJournalDecodeStatus() const
	{
		return SourceJournalDecodeStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
	GetTerminalJournalDecodeStatus() const
	{
		return TerminalJournalDecodeStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
	GetCompletion() const
	{
		return Completion;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionCodec;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus::
				Invalid;
	FString Diagnostic;
	int32 SourceSchemaVersion = 0;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
		SourceJournalDecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
		TerminalJournalDecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
		Completion;
};

/** Big-endian bounded codec for one source/terminal journal pair. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionCodec
{
public:
	static constexpr int32 CurrentSchemaVersion() { return 1; }
	static constexpr int32 HeaderSize() { return 220; }
	static constexpr int32 MaximumJournalEncodedBytes()
	{
		return Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec::
			HeaderSize()
			+ Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal::
				MaxRecordCount()
				* Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec::
					CurrentRecordSize();
	}
	static constexpr int32 MaximumEncodedBytes()
	{
		return HeaderSize() + MaximumJournalEncodedBytes() * 2;
	}

	static bool TryEncode(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			Completion,
		TArray<uint8>& OutBytes);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeResult
	Decode(const TArray<uint8>& Bytes);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeResult
	MakeDecodeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus
			Status,
		const TCHAR* Diagnostic,
		int32 SourceSchemaVersion = 0,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
			SourceJournalDecodeStatus =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
					Invalid,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus
			TerminalJournalDecodeStatus =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus::
					Invalid,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			Completion = {});
};

/** One caller-owned root plus one stable lineage completion slot. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext
{
public:
	static constexpr int32 MaximumRootDirectoryCharacters() { return 1024; }

	static bool TryCreate(
		const FString& AbsoluteRootDirectory,
		const FGuid& ExpectedLineageId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext&
			OutContext,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FString& GetRootDirectory() const { return RootDirectory; }
	const FString& GetStorageDirectory() const { return StorageDirectory; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	const FString& GetTemporaryPath() const { return TemporaryPath; }
	const FGuid& GetExpectedLineageId() const { return ExpectedLineageId; }

private:
	FString RootDirectory;
	FString StorageDirectory;
	FString PrimaryPath;
	FString TemporaryPath;
	FGuid ExpectedLineageId;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus
	: uint8
{
	Invalid,
	Saved,
	AlreadyCurrent,
	ContextRejected,
	CompletionRejected,
	LineageSlotMismatch,
	MinimumGenerationRejected,
	GenerationBelowWatermark,
	EncodingRejected,
	ExistingReadFailed,
	ExistingSizeRejected,
	ExistingDecodeRejected,
	ExistingLineageMismatch,
	ExistingGenerationNewer,
	ExistingGenerationConflict,
	DirectoryCreationFailed,
	StaleTemporaryCleanupFailed,
	TemporaryOpenFailed,
	TemporaryWriteFailed,
	TemporaryFlushFailed,
	TemporaryReadBackFailed,
	TemporaryValidationFailed,
	AtomicReplaceFailed,
	CommittedReadBackFailed,
	CommittedValidationFailed
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveResult
{
public:
	bool IsSuccess() const;
	bool WasAlreadyCurrent() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetLineageId() const { return LineageId; }
	const FGuid& GetCompletionId() const { return CompletionId; }
	int32 GetGeneration() const { return Generation; }
	int32 GetMinimumGeneration() const { return MinimumGeneration; }
	int64 GetEncodedByteCount() const { return EncodedByteCount; }
	bool DidReplacePrimary() const { return bDidReplacePrimary; }
	bool TemporaryFileMayRemain() const { return bTemporaryFileMayRemain; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus::
				Invalid;
	FString Diagnostic;
	FGuid LineageId;
	FGuid CompletionId;
	int32 Generation = 0;
	int32 MinimumGeneration = 0;
	int64 EncodedByteCount = 0;
	bool bDidReplacePrimary = false;
	bool bTemporaryFileMayRemain = false;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
	: uint8
{
	Invalid,
	Loaded,
	ContextRejected,
	MinimumGenerationRejected,
	Missing,
	ReadFailed,
	SizeRejected,
	DecodeRejected,
	LineageSlotMismatch,
	GenerationBelowWatermark
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadResult
{
public:
	bool IsSuccess() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetLineageId() const { return LineageId; }
	int32 GetGeneration() const { return Generation; }
	int32 GetMinimumGeneration() const { return MinimumGeneration; }
	int64 GetObservedByteCount() const { return ObservedByteCount; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus
	GetDecodeStatus() const
	{
		return DecodeStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
	GetCompletion() const
	{
		return Completion;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus::
				Invalid;
	FString Diagnostic;
	FGuid LineageId;
	int32 Generation = 0;
	int32 MinimumGeneration = 0;
	int64 ObservedByteCount = INDEX_NONE;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus
		DecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
		Completion;
};

/** Stateless atomic storage for one canonical completion per lineage. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveResult
	Save(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext&
			Context,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			Completion,
		int32 MinimumGeneration,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem&
			FileSystem) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadResult
	Load(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext&
			Context,
		int32 MinimumGeneration,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem&
			FileSystem) const;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionStatus
	: uint8
{
	Invalid,
	RequestRejected,
	OperationInProgress,
	CurrentJournalRejected,
	CompletionAuthorityReadRejected,
	CompletionAuthorityUnavailable,
	CompletionAuthorityStateRejected,
	CompletionAuthorityAhead,
	TrustedCompletionLoadRejected,
	TrustedCompletionMismatch,
	AdmissionRejected,
	JournalAppendRejected,
	CompletionCreationRejected,
	CompletionSaveRejected,
	CompletionVerificationRejected,
	CompletionAdvanceRejected,
	CompletionAdvanceConflict,
	CompletionEvidenceCommittedAuthorityPending,
	CompletionOutcomeUnresolved,
	Completed,
	CompletedAfterAuthorityRecheck,
	Replayed
};

/** Auditable result of one bounded recovery-and-durable-completion call. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsSuccess() const;
	bool IsReplay() const;
	bool DidInvokeAdmission() const { return bAdmissionInvoked; }
	bool WasCompletionVerified() const { return bCompletionVerified; }
	bool IsCompletionAuthorityCurrent() const { return bAuthorityCurrent; }
	bool DidMutateSurface() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
	GetRequest() const
	{
		return Request;
	}
	int32 GetPreviousCompletionGeneration() const
	{
		return PreviousCompletionGeneration;
	}
	int32 GetTargetGeneration() const { return TargetGeneration; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetCompletionAuthorityReadStatus() const
	{
		return CompletionAuthorityReadStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
	GetCompletionAuthorityAdvanceStatus() const
	{
		return CompletionAuthorityAdvanceStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetCompletionAuthorityRecheckStatus() const
	{
		return CompletionAuthorityRecheckStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus
	GetCompletionSaveStatus() const
	{
		return CompletionSaveStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
	GetCompletionLoadStatus() const
	{
		return CompletionLoadStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult&
	GetAdmissionResult() const
	{
		return AdmissionResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult&
	GetJournalAppendResult() const
	{
		return JournalAppendResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
	GetCompletion() const
	{
		return Completion;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetTerminalJournal() const
	{
		return TerminalJournal;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest
		Request;
	int32 PreviousCompletionGeneration = INDEX_NONE;
	int32 TargetGeneration = 0;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		CompletionAuthorityReadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
		CompletionAuthorityAdvanceStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		CompletionAuthorityRecheckStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus
		CompletionSaveStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
		CompletionLoadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus::
				Invalid;
	bool bAdmissionInvoked = false;
	bool bCompletionVerified = false;
	bool bAuthorityCurrent = false;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult
		AdmissionResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult
		JournalAppendResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
		Completion;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		TerminalJournal;
	bool bValidated = false;
};

/**
 * Explicit bounded closure for P20.55 success.
 *
 * One call reads completion authority once. An exact trusted completion
 * short-circuits before admission. Otherwise it invokes P20.55 once, appends
 * the exact receipt to a copy of the pending journal, atomically saves and
 * verifies a distinct completion, then advances the completion watermark
 * once. Only an unknown advance outcome permits one authority re-read.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionResult
	ExecuteExplicit(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			PendingStorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext&
			CompletionStorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			CurrentJournal,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner& Owner,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			ExpectedRetiredSurface,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface& NewSurface,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem&
			FileSystem,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			Authority);

	bool IsOperationInProgress() const { return bOperationInProgress; }

private:
	bool bOperationInProgress = false;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession
		AdmissionSession;
};
