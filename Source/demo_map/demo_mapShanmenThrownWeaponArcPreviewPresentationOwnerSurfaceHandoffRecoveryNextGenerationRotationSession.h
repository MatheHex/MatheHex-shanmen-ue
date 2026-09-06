#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.h"

/** Explicit caller intent to open one later generation from an adopted terminal. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
			Adoption,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			NewCheckpoint,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest&
			OutRequest,
		FString& OutDiagnostic);

	bool IsValid() const;
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
	GetAdoption() const
	{
		return Adoption;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
	GetNewCheckpoint() const
	{
		return NewCheckpoint;
	}
	const FGuid& GetRequestId() const { return RequestId; }

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption
		Adoption;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint
		NewCheckpoint;
	FGuid RequestId;
};

/**
 * Deterministic evidence that one adopted terminal journal became the exact
 * prefix of a later pending bundle.
 *
 * The source terminal and candidate bundle are retained together so validity
 * can prove a one-record append, reject historical checkpoint reuse and bind
 * the new bundle identity back to the exact P20.57 adoption.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			SourceTerminalJournal,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			PendingBundle,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation&
			OutRotation);

	bool IsValid() const;
	bool MatchesRequest(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest&
			Request) const;

	int32 GetSourceGeneration() const
	{
		return Request.GetAdoption().GetGeneration();
	}
	int32 GetTargetGeneration() const
	{
		return PendingBundle.GetGeneration();
	}
	const FGuid& GetRotationId() const { return RotationId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest&
	GetRequest() const
	{
		return Request;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetSourceTerminalJournal() const
	{
		return SourceTerminalJournal;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetPendingJournal() const
	{
		return PendingBundle.GetJournal();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
	GetPendingBundle() const
	{
		return PendingBundle;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest
		Request;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		SourceTerminalJournal;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle
		PendingBundle;
	FGuid RotationId;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionStatus
	: uint8
{
	Invalid,
	RequestRejected,
	OperationInProgress,
	CurrentJournalRejected,
	CheckpointHistoryConflict,
	CompletionAuthorityReadRejected,
	CompletionAuthorityUnavailable,
	CompletionAuthorityMissing,
	CompletionAuthorityStateRejected,
	CompletionAuthorityNotCurrent,
	CompletionAuthorityAhead,
	CompletionAuthorityConflict,
	TrustedCompletionLoadRejected,
	TrustedCompletionMismatch,
	AdoptionEvidenceMismatch,
	AdoptionAuthorityReadRejected,
	AdoptionAuthorityUnavailable,
	AdoptionAuthorityMissing,
	AdoptionAuthorityStateRejected,
	AdoptionAuthorityNotCurrent,
	AdoptionAuthorityAhead,
	AdoptionAuthorityConflict,
	JournalAppendRejected,
	EnvelopeCreationRejected,
	BundleCreationRejected,
	RotationCreationRejected,
	PendingAuthorityReadRejected,
	PendingAuthorityUnavailable,
	PendingAuthorityMissing,
	PendingAuthorityStateRejected,
	PendingAuthorityNotCurrent,
	PendingAuthorityAhead,
	PendingAuthorityConflict,
	BundleCommitRejected,
	BundleCommittedWatermarkPending,
	PendingWatermarkOutcomeUnresolved,
	Rotated,
	Replayed
};

/** Auditable result of one bounded adopted-terminal rotation call. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsSuccess() const;
	bool IsReplay() const;
	bool WasCompletionTrusted() const { return bCompletionTrusted; }
	bool WasAdoptionTrusted() const { return bAdoptionTrusted; }
	bool WasPendingAuthorityAccepted() const
	{
		return bPendingAuthorityAccepted;
	}
	bool DidMutateSurface() const { return false; }
	bool DidWriteCompletionEvidence() const { return false; }
	bool DidAdvanceAdoptionAuthority() const { return false; }

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest&
	GetRequest() const
	{
		return Request;
	}
	int32 GetSourceGeneration() const { return SourceGeneration; }
	int32 GetTargetGeneration() const { return TargetGeneration; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetCompletionAuthorityReadStatus() const
	{
		return CompletionAuthorityReadStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
	GetCompletionLoadStatus() const
	{
		return CompletionLoadStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetAdoptionAuthorityReadStatus() const
	{
		return AdoptionAuthorityReadStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetPendingAuthorityReadStatus() const
	{
		return PendingAuthorityReadStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult&
	GetJournalAppendResult() const
	{
		return JournalAppendResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult&
	GetBundleCommitResult() const
	{
		return BundleCommitResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
	GetCompletion() const
	{
		return Completion;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
	GetVerifiedAdoption() const
	{
		return VerifiedAdoption;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation&
	GetRotation() const
	{
		return Rotation;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest
		Request;
	int32 SourceGeneration = 0;
	int32 TargetGeneration = 0;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		CompletionAuthorityReadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus
		CompletionLoadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		AdoptionAuthorityReadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		PendingAuthorityReadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	bool bCompletionTrusted = false;
	bool bAdoptionTrusted = false;
	bool bPendingAuthorityAccepted = false;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult
		JournalAppendResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult
		BundleCommitResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
		Completion;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption
		VerifiedAdoption;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation
		Rotation;
	bool bValidated = false;
};

/**
 * Explicit one-generation rotation after trusted terminal adoption.
 *
 * Completion storage and adoption authority are read-only. The only durable
 * mutation is delegated to the existing save/read-back/CAS pending-bundle
 * coordinator. No surface callback, completion rewrite, adoption advance,
 * directory scan, deletion, scheduler or retry loop occurs here.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionResult
	ExecuteExplicit(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext&
			CompletionStorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			PendingBundleStorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			CurrentTerminalJournal,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem&
			FileSystem,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			CompletionAuthority,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			AdoptionAuthority,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			PendingAuthority);

	bool IsOperationInProgress() const { return bOperationInProgress; }

private:
	bool bOperationInProgress = false;
};
