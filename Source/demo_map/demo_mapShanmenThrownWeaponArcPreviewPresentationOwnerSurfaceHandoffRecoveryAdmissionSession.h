#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.h"

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;

/**
 * Explicit caller intent for one exact pending recovery transaction.
 *
 * The request does not discover a journal, checkpoint or lineage. Its
 * deterministic identity binds the exact evidence that the caller intends to
 * admit before any trusted-state or file callback occurs.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest
{
public:
	static bool TryCreate(
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId,
		const FGuid& ExpectedJournalId,
		const FGuid& ExpectedCheckpointId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest&
			OutRequest,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetAuthorityDomainId() const { return AuthorityDomainId; }
	const FGuid& GetLineageId() const { return LineageId; }
	const FGuid& GetExpectedJournalId() const { return ExpectedJournalId; }
	const FGuid& GetExpectedCheckpointId() const
	{
		return ExpectedCheckpointId;
	}
	const FGuid& GetRequestId() const { return RequestId; }

private:
	FGuid AuthorityDomainId;
	FGuid LineageId;
	FGuid ExpectedJournalId;
	FGuid ExpectedCheckpointId;
	FGuid RequestId;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionStatus
	: uint8
{
	Invalid,
	RequestRejected,
	OperationInProgress,
	CurrentJournalRejected,
	AuthorityMissing,
	AuthorityReadRejected,
	AuthorityUnavailable,
	AuthorityStateRejected,
	BundleLoadRejected,
	BundleWatermarkMismatch,
	BundleJournalMismatch,
	CheckpointEvidenceRejected,
	RecoveryRejected,
	Recovered,
	Replayed
};

/**
 * Auditable result of one bounded admission and optional P20.48 execution.
 *
 * A checkpoint is admitted only after trusted state, exact bundle bytes and
 * the caller's current pending journal all agree. RecoveryRejected means that
 * P20.48 was invoked and its fresh live authority checks denied the attempt.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsSuccess() const;
	bool WasRecovered() const;
	bool IsReplay() const;
	bool WasCheckpointAdmitted() const { return bCheckpointAdmitted; }
	bool DidInvokeRecovery() const { return bRecoveryInvoked; }
	bool DidMutateSurface() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest&
	GetRequest() const
	{
		return Request;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetAuthorityReadStatus() const
	{
		return AuthorityReadStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
	GetAuthorityState() const
	{
		return AuthorityState;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
	GetBundleLoadStatus() const
	{
		return BundleLoadStatus;
	}
	int32 GetLoadedGeneration() const { return LoadedGeneration; }
	const FGuid& GetLoadedBundleId() const { return LoadedBundleId; }
	const FGuid& GetLoadedJournalId() const { return LoadedJournalId; }
	const FGuid& GetLoadedCheckpointId() const
	{
		return LoadedCheckpointId;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
	GetCheckpoint() const
	{
		return Checkpoint;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult&
	GetRecoveryResult() const
	{
		return RecoveryResult;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest
		Request;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		AuthorityReadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState
		AuthorityState;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
		BundleLoadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus::
				Invalid;
	int32 LoadedGeneration = 0;
	FGuid LoadedBundleId;
	FGuid LoadedJournalId;
	FGuid LoadedCheckpointId;
	bool bCheckpointAdmitted = false;
	bool bRecoveryInvoked = false;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint
		Checkpoint;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult
		RecoveryResult;
	bool bValidated = false;
};

/**
 * Explicit, bounded process-recovery admission.
 *
 * One call performs at most one trusted authority read, one P20.53 bundle
 * load and one P20.48 recovery execution. It never saves a bundle, advances a
 * watermark, appends a journal, mutates a surface, retries or schedules work.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult
	ExecuteExplicit(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			StorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
			CurrentJournal,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner&
			Owner,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			ExpectedRetiredSurface,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			NewSurface,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem&
			FileSystem,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			Authority);

	bool IsOperationInProgress() const { return bOperationInProgress; }

private:
	bool bOperationInProgress = false;
};
