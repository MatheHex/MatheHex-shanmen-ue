#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.h"

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;

/**
 * Immutable evidence that a P20.47 attempt stopped after old-surface
 * retirement but before Owner/Adapter pointer commit.
 *
 * A checkpoint never authorizes recovery by itself. Recovery must re-read
 * both exact live surfaces and the unchanged Host/Adapter authority scope.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult&
			FailedHandoff,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			OutCheckpoint,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool MatchesFailedHandoff(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult&
			FailedHandoff) const;

	const FGuid& GetCheckpointId() const { return CheckpointId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
	GetTransitionTicket() const
	{
		return TransitionTicket;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
	GetSourceFailureStatus() const
	{
		return SourceFailureStatus;
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
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousSurfaceCursor() const
	{
		return PreviousSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetRetiredSurfaceCursor() const
	{
		return RetiredSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}

private:
	FGuid CheckpointId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket
		TransitionTicket;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
		SourceFailureStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus::
				Invalid;
	FGuid SourceRetirementResponseId;
	FGuid PreviousSurfaceInstanceId;
	FGuid SurfaceInstanceId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		RetiredSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

/** Immutable evidence for one zero-mutation recovery pointer commit. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
{
public:
	bool IsValid() const;
	bool MatchesCheckpoint(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint) const;
	bool MatchesCurrentBinding(
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		const FGuid& SurfaceInstanceId) const;

	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetCheckpointId() const { return CheckpointId; }
	const FGuid& GetTransitionTicketId() const
	{
		return TransitionTicketId;
	}
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const FGuid& GetPreviousSurfaceInstanceId() const
	{
		return PreviousSurfaceInstanceId;
	}
	const FGuid& GetSurfaceInstanceId() const { return SurfaceInstanceId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetRetiredSurfaceCursor() const
	{
		return RetiredSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;

	FGuid ReceiptId;
	FGuid CheckpointId;
	FGuid TransitionTicketId;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	FGuid PreviousSurfaceInstanceId;
	FGuid SurfaceInstanceId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		RetiredSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus
	: uint8
{
	Invalid,
	CheckpointInvalid,
	OperationInProgress,
	OwnerInactive,
	OwnerStructureMismatch,
	CheckpointReplayConflict,
	OldSurfaceMismatch,
	OldSnapshotMismatch,
	NewSnapshotMismatch,
	AuthoritySnapshotMismatch,
	CommitInvariantViolation,
	Recovered,
	Replayed
};

/** Self-validating result for one bounded, zero-mutation recovery attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsAccepted() const;
	bool WasRecovered() const;
	bool IsReplay() const;
	bool HasReceipt() const;
	bool DidMutateSurface() const { return false; }

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
	GetCheckpoint() const
	{
		return Checkpoint;
	}
	int32 GetSurfaceSnapshotReadCount() const
	{
		return SurfaceSnapshotReadCount;
	}
	const FGuid& GetObservedPreviousSurfaceInstanceId() const
	{
		return ObservedPreviousSurfaceInstanceId;
	}
	const FGuid& GetObservedSurfaceInstanceId() const
	{
		return ObservedSurfaceInstanceId;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetObservedRetiredSurfaceCursor() const
	{
		return ObservedRetiredSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetObservedSurfaceCursor() const
	{
		return ObservedSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	bool IsOwnerValidAfter() const { return bOwnerValidAfter; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint
		Checkpoint;
	int32 SurfaceSnapshotReadCount = 0;
	FGuid ObservedPreviousSurfaceInstanceId;
	FGuid ObservedSurfaceInstanceId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ObservedRetiredSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ObservedSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		Receipt;
	bool bOwnerValidAfter = false;
	bool bValidated = false;
};

/**
 * Explicitly recovers the one P20.47 half-commit shape that can be proven by
 * a checkpoint plus fresh dual-surface observation. It never mutates either
 * surface and never treats the failed retirement response as authorization.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult
	Execute(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner&
			Owner,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			ExpectedRetiredSurface,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			NewSurface);

private:
	bool TryMakeReceipt(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			RetiredSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			OutReceipt) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint&
			Checkpoint,
		int32 SurfaceSnapshotReadCount,
		const FGuid& ObservedPreviousSurfaceInstanceId,
		const FGuid& ObservedSurfaceInstanceId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ObservedRetiredSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ObservedSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			Receipt,
		bool bOwnerValidAfter) const;
};
