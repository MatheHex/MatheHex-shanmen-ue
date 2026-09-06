#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.h"

/**
 * One monotonic compare-and-advance request for a caller-owned trust domain.
 *
 * Generation zero means that the authority has no committed watermark yet.
 * The target generation is always strictly newer than the observed one.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest
{
public:
	static bool TryCreate(
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId,
		int32 ExpectedGeneration,
		int32 TargetGeneration,
		const FGuid& BundleId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest&
			OutRequest,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetAuthorityDomainId() const { return AuthorityDomainId; }
	const FGuid& GetLineageId() const { return LineageId; }
	int32 GetExpectedGeneration() const { return ExpectedGeneration; }
	int32 GetTargetGeneration() const { return TargetGeneration; }
	const FGuid& GetBundleId() const { return BundleId; }
	const FGuid& GetRequestId() const { return RequestId; }

private:
	FGuid AuthorityDomainId;
	FGuid LineageId;
	int32 ExpectedGeneration = INDEX_NONE;
	int32 TargetGeneration = 0;
	FGuid BundleId;
	FGuid RequestId;
};

/** Deterministic audit evidence for one accepted watermark advance. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest&
			Request,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
			OutReceipt);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest&
			Request) const;
	const FGuid& GetAuthorityDomainId() const { return AuthorityDomainId; }
	const FGuid& GetLineageId() const { return LineageId; }
	int32 GetPreviousGeneration() const { return PreviousGeneration; }
	int32 GetCommittedGeneration() const { return CommittedGeneration; }
	const FGuid& GetBundleId() const { return BundleId; }
	const FGuid& GetRequestId() const { return RequestId; }
	const FGuid& GetReceiptId() const { return ReceiptId; }

private:
	FGuid AuthorityDomainId;
	FGuid LineageId;
	int32 PreviousGeneration = INDEX_NONE;
	int32 CommittedGeneration = 0;
	FGuid BundleId;
	FGuid RequestId;
	FGuid ReceiptId;
};

/** Current monotonic authority state, including its exact last audit receipt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
			LastReceipt,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
			OutState);

	bool IsValid() const;
	bool MatchesTarget(
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId,
		int32 Generation,
		const FGuid& BundleId) const;
	bool MatchesRequest(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest&
			Request) const;
	const FGuid& GetAuthorityDomainId() const
	{
		return LastReceipt.GetAuthorityDomainId();
	}
	const FGuid& GetLineageId() const { return LastReceipt.GetLineageId(); }
	int32 GetGeneration() const
	{
		return LastReceipt.GetCommittedGeneration();
	}
	const FGuid& GetBundleId() const { return LastReceipt.GetBundleId(); }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
	GetLastReceipt() const
	{
		return LastReceipt;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt
		LastReceipt;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	: uint8
{
	Invalid,
	Missing,
	Current,
	Rejected,
	Unavailable
};

/** One bounded authority read. The interface decides where trusted state lives. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
	Missing(const FString& Diagnostic);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
	Current(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
			State,
		const FString& Diagnostic);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
	Failure(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
			Status,
		const FString& Diagnostic);

	bool IsValid() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
	GetState() const
	{
		return State;
	}

private:
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState
		State;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
	: uint8
{
	Invalid,
	Advanced,
	AlreadyCurrent,
	Conflict,
	Rejected,
	Unavailable,
	OutcomeUnknown
};

/** Result of exactly one compare-and-advance call; the coordinator never retries it. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
	Committed(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
			Status,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
			Receipt,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
			State,
		const FString& Diagnostic);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
	Failure(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
			Status,
		const FString& Diagnostic);
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
	Unknown(const FString& Diagnostic);

	bool IsValid() const;
	bool IsSuccess() const;
	bool MayHaveAdvanced() const { return bMayHaveAdvanced; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
	GetState() const
	{
		return State;
	}

private:
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt
		Receipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState
		State;
	bool bMayHaveAdvanced = false;
};

/**
 * Caller-owned trust boundary.
 *
 * Implementations may use protected platform storage, a remote authority, or
 * another trust domain. This contract intentionally provides no ordinary-file
 * implementation that could be mistaken for rollback-resistant storage.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority() =
		default;

	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
	Read(
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId) const = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
	CompareAndAdvance(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest&
			Request) = 0;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus
	: uint8
{
	Invalid,
	Committed,
	AlreadyCommitted,
	CommittedAfterAuthorityRecheck,
	InputRejected,
	AuthorityReadRejected,
	AuthorityUnavailable,
	AuthorityStateRejected,
	AuthorityAheadOfBundle,
	AuthorityBundleConflict,
	BundleSaveRejected,
	BundleVerificationRejected,
	WatermarkAdvanceRejected,
	WatermarkAdvanceConflict,
	BundleCommittedWatermarkPending,
	WatermarkOutcomeUnresolved
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult
{
public:
	bool IsSuccess() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetAuthorityDomainId() const { return AuthorityDomainId; }
	const FGuid& GetLineageId() const { return LineageId; }
	const FGuid& GetBundleId() const { return BundleId; }
	int32 GetPreviousGeneration() const { return PreviousGeneration; }
	int32 GetTargetGeneration() const { return TargetGeneration; }
	bool WasBundleVerified() const { return bBundleVerified; }
	bool IsWatermarkCurrent() const { return bWatermarkCurrent; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus
	GetBundleSaveStatus() const
	{
		return BundleSaveStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
	GetBundleLoadStatus() const
	{
		return BundleLoadStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetAuthorityReadStatus() const
	{
		return AuthorityReadStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
	GetAuthorityRecheckStatus() const
	{
		return AuthorityRecheckStatus;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
	GetAuthorityAdvanceStatus() const
	{
		return AuthorityAdvanceStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
	GetState() const
	{
		return State;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitCoordinator;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus::
				Invalid;
	FString Diagnostic;
	FGuid AuthorityDomainId;
	FGuid LineageId;
	FGuid BundleId;
	int32 PreviousGeneration = INDEX_NONE;
	int32 TargetGeneration = 0;
	bool bBundleVerified = false;
	bool bWatermarkCurrent = false;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus
		BundleSaveStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
		BundleLoadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		AuthorityReadStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus
		AuthorityRecheckStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus
		AuthorityAdvanceStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt
		Receipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState
		State;
};

/**
 * Bounded crash-ordering composition:
 *   1. read trusted watermark once;
 *   2. save and re-load exact bundle evidence;
 *   3. compare-and-advance the watermark once;
 *   4. only for an unknown authority outcome, read once more.
 *
 * A crash before step 3 leaves a conservative old watermark. Step 3 can never
 * run before exact bundle evidence is readable. No recovery is executed here.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitCoordinator
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult
	CommitBundleThenAdvance(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			StorageContext,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			Bundle,
		const FGuid& AuthorityDomainId,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem&
			FileSystem,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
			Authority) const;
};
