#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.h"

/** Explicit caller intent to adopt one exact trusted recovery completion. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest
{
public:
	static bool TryCreate(
		const FGuid& AdoptionAuthorityDomainId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
			CompletionRequest,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest&
			OutRequest,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetAdoptionAuthorityDomainId() const
	{
		return AdoptionAuthorityDomainId;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest&
	GetCompletionRequest() const
	{
		return CompletionRequest;
	}
	const FGuid& GetRequestId() const { return RequestId; }

private:
	FGuid AdoptionAuthorityDomainId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest
		CompletionRequest;
	FGuid RequestId;
};

/**
 * Deterministic evidence that a trusted P20.56 terminal journal may replace
 * the caller-selected source journal.
 *
 * This object does not mutate caller memory and does not create a later
 * checkpoint. NextGeneration is only a capacity/ordering boundary: a later
 * stage still needs one explicit real handoff checkpoint before rotation.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			Completion,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
			OutAdoption);

	bool IsValid() const;
	bool MatchesRequest(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest&
			Request) const;
	bool MatchesCompletion(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			Completion) const;

	int32 GetGeneration() const { return Generation; }
	int32 GetNextGeneration() const { return NextGeneration; }
	bool HasNextGenerationCapacity() const { return NextGeneration > 0; }
	const FGuid& GetAdoptionId() const { return AdoptionId; }
	const FGuid& GetCompletionId() const { return CompletionId; }
	const FGuid& GetCompletionDigest() const { return CompletionDigest; }
	const FGuid& GetSourceJournalId() const { return SourceJournalId; }
	const FGuid& GetTerminalJournalId() const { return TerminalJournalId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest&
	GetRequest() const
	{
		return Request;
	}

private:
	int32 Generation = 0;
	int32 NextGeneration = 0;
	FGuid AdoptionId;
	FGuid CompletionId;
	FGuid CompletionDigest;
	FGuid SourceJournalId;
	FGuid TerminalJournalId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest
		Request;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionStatus
	: uint8
{
	Invalid,
	RequestRejected,
	OperationInProgress,
	CurrentJournalRejected,
	CompletionAuthorityReadRejected,
	CompletionAuthorityUnavailable,
	CompletionAuthorityMissing,
	CompletionAuthorityStateRejected,
	CompletionAuthorityNotCurrent,
	CompletionAuthorityAhead,
	TrustedCompletionLoadRejected,
	TrustedCompletionMismatch,
	AdoptionCreationRejected,
	AdoptionAuthorityReadRejected,
	AdoptionAuthorityUnavailable,
	AdoptionAuthorityStateRejected,
	AdoptionAuthorityAhead,
	AdoptionAuthorityConflict,
	AdoptionAdvanceRejected,
	AdoptionAdvanceConflict,
	AdoptionOutcomeUnresolved,
	Adopted,
	AdoptedAfterAuthorityRecheck,
	Replayed
};

/** Auditable result of one bounded trusted-terminal adoption call. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsSuccess() const;
	bool IsReplay() const;
	bool IsCompletionTrusted() const { return bCompletionTrusted; }
	bool IsAdoptionAuthorityCurrent() const { return bAuthorityCurrent; }
	bool HasNextGenerationCapacity() const;
	bool DidMutateSurface() const { return false; }

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest&
	GetRequest() const
	{
		return Request;
	}
	int32 GetPreviousAdoptionGeneration() const
	{
		return PreviousAdoptionGeneration;
	}
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
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
	GetAdoptionAuthorityAdvanceStatus() const
	{
		return AdoptionAuthorityAdvanceStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetAdoptionAuthorityRecheckStatus() const
	{
		return AdoptionAuthorityRecheckStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
	GetCompletion() const
	{
		return Completion;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
	GetAdoption() const
	{
		return Adoption;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
	GetTerminalJournal() const
	{
		return TerminalJournal;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest
		Request;
	int32 PreviousAdoptionGeneration = INDEX_NONE;
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
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
		AdoptionAuthorityAdvanceStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		AdoptionAuthorityRecheckStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	bool bCompletionTrusted = false;
	bool bAuthorityCurrent = false;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
		Completion;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption
		Adoption;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal
		TerminalJournal;
	bool bValidated = false;
};

/**
 * Explicit bounded caller-state adoption.
 *
 * One call verifies the exact P20.56 completion authority and artifact, then
 * compare-and-advances a separate caller adoption authority once. Only an
 * unknown advance outcome permits one exact adoption-authority re-read. No
 * surface callback, journal append, filesystem write, scan, retry loop,
 * deletion or next-generation checkpoint creation occurs here.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionResult
	ExecuteExplicit(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext&
			CompletionStorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			CurrentJournal,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem&
			FileSystem,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			CompletionAuthority,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			AdoptionAuthority);

	bool IsOperationInProgress() const { return bOperationInProgress; }

private:
	bool bOperationInProgress = false;
};
