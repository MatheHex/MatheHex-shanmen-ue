#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "ShanmenItemPersistence.h"
#include "ShanmenItemRepository.h"
#include "Templates/Function.h"

/** Runtime state of the single serialized 0.0.10 item-authority lifecycle. */
enum class EShanmenItemAuthorityServiceState : uint8
{
	Closed,
	Ready,
	RecoveryRequired
};

/** Outcome of opening the durable authority or performing its one-time migration. */
enum class EShanmenItemAuthorityStartStatus : uint8
{
	OpenedExisting,
	RecoveredExisting,
	CreatedFromMigration,
	AlreadyReady,
	MigrationRequired,
	MigrationNotAuthorized,
	InvalidRequest,
	PersistenceFailure,
	RepositoryLoadFailure
};

/** Durable outcome of one repository command. */
enum class EShanmenItemDurableCommandStatus : uint8
{
	Persisted,
	Replayed,
	RejectedAndPersisted,
	RejectedWithoutMutation,
	ResolvedAfterReopen,
	PersistenceFailedRolledBack,
	RecoveryRequired,
	NotReady
};

/**
 * Capability object for the one-time legacy handoff. It only authorizes the
 * exact MigrationId reviewed by the caller and cannot authorize an empty id.
 */
struct SHANMENITEMS_API FShanmenItemMigrationAuthorization
{
	static FShanmenItemMigrationAuthorization Explicit(const FGuid& MigrationId);

	bool Permits(const FShanmenItemMigrationEvidence& Evidence) const;

private:
	FGuid AuthorizedMigrationId;
};

struct SHANMENITEMS_API FShanmenItemAuthorityStartResult
{
	EShanmenItemAuthorityStartStatus Status =
		EShanmenItemAuthorityStartStatus::PersistenceFailure;
	EShanmenItemLoadStatus LoadStatus = EShanmenItemLoadStatus::ReadFailed;
	EShanmenItemOpenStatus OpenStatus = EShanmenItemOpenStatus::PersistenceFailure;
	FString Diagnostic;
	bool bDiskStateChanged = false;
	int32 DocumentGeneration = INDEX_NONE;

	bool IsReady() const;
};

struct SHANMENITEMS_API FShanmenItemDurableCommandResult
{
	EShanmenItemDurableCommandStatus Status =
		EShanmenItemDurableCommandStatus::NotReady;
	FShanmenItemTransactionReceipt Receipt;
	EShanmenItemSaveStatus SaveStatus =
		EShanmenItemSaveStatus::ValidationRejected;
	EShanmenItemLoadStatus ReconcileLoadStatus =
		EShanmenItemLoadStatus::ReadFailed;
	FString Diagnostic;
	bool bRepositoryMutated = false;
	bool bDiskStateChanged = false;
	int32 DocumentGeneration = INDEX_NONE;

	/** True when the returned receipt is backed by the currently durable state. */
	bool IsDurable() const;
	/** True only for a successful repository receipt that is also durable. */
	bool IsCommandSuccess() const;
};

/**
 * The sole lifecycle/facade permitted to mutate the 0.0.10 item authority.
 *
 * Startup always probes the durable versioned authority document first. A missing
 * document requires an explicit migration capability. Every command is
 * serialized and is reported successful only after the resulting snapshot is
 * durable (or an exact durable replay has been verified). Persistence
 * ambiguity is reconciled by reopening the document; divergent state fails
 * closed and moves the service to RecoveryRequired.
 */
class SHANMENITEMS_API FShanmenItemAuthorityService
{
public:
	FShanmenItemAuthorityService() = default;
	FShanmenItemAuthorityService(const FShanmenItemAuthorityService&) = delete;
	FShanmenItemAuthorityService& operator=(
		const FShanmenItemAuthorityService&) = delete;

	FShanmenItemAuthorityStartResult StartExisting(
		const FShanmenItemStorageContext& InStorage);

	FShanmenItemAuthorityStartResult StartFromAuthorizedMigration(
		const FShanmenItemStorageContext& InStorage,
		const FShanmenItemMigrationAuthorization& Authorization,
		const FShanmenItemAuthoritySnapshot& MigrationCandidate,
		const FShanmenItemMigrationEvidence& Migration);

	FShanmenItemDurableCommandResult ReserveDurable(
		const FShanmenItemReserveRequest& Request);
	FShanmenItemDurableCommandResult CommitDurable(
		const FShanmenItemReservationActionRequest& Request);
	FShanmenItemDurableCommandResult CommitBatchDurable(
		const FShanmenItemReservationBatchRequest& Request);
	FShanmenItemDurableCommandResult AmendReservationPurposeDurable(
		const FShanmenItemReservationAmendRequest& Request);
	FShanmenItemDurableCommandResult CancelDurable(
		const FShanmenItemReservationActionRequest& Request);
	FShanmenItemDurableCommandResult ReleaseDeploymentDurable(
		const FShanmenItemReservationActionRequest& Request);
	FShanmenItemDurableCommandResult ClaimPreparedRunDurable(
		const FShanmenItemRunClaimRequest& Request);
	FShanmenItemDurableCommandResult FinalizePreparedRunDurable(
		const FShanmenItemRunFinalizeRequest& Request);

	EShanmenItemAuthorityServiceState GetState() const;
	bool TryGetDocument(FShanmenItemAuthorityDocument& OutDocument) const;
	bool TryCaptureSnapshot(FShanmenItemAuthoritySnapshot& OutSnapshot) const;

#if WITH_DEV_AUTOMATION_TESTS
	/** Changes only the next persistence failure-injection stage used by tests. */
	void SetInjectedFailureForTests(EShanmenItemStoreFailureStage Stage);
#endif

private:
	mutable FCriticalSection Mutex;
	EShanmenItemAuthorityServiceState State =
		EShanmenItemAuthorityServiceState::Closed;
	FShanmenItemStorageContext Storage;
	FShanmenItemAuthorityDocument Document;
	FShanmenItemRepository Repository;
	FShanmenItemAuthorityStore Store;

	FShanmenItemAuthorityStartResult StartLocked(
		const FShanmenItemStorageContext& InStorage,
		const FShanmenItemMigrationAuthorization* Authorization,
		const FShanmenItemAuthoritySnapshot* MigrationCandidate,
		const FShanmenItemMigrationEvidence* Migration);
	bool InstallDocumentLocked(
		const FShanmenItemAuthorityDocument& InDocument,
		const FShanmenItemStorageContext& InStorage,
		FString& OutDiagnostic);

	FShanmenItemDurableCommandResult ExecuteCommandLocked(
		TFunctionRef<FShanmenItemTransactionReceipt(FShanmenItemRepository&)>
			Command);
	FShanmenItemDurableCommandResult ReconcileFailedSaveLocked(
		const FShanmenItemSaveResult& SaveResult,
		const FShanmenItemAuthorityDocument& BeforeDocument,
		const FShanmenItemAuthoritySnapshot& After,
		const FShanmenItemTransactionReceipt& Receipt,
		bool bRepositoryMutated);
	FShanmenItemDurableCommandResult EnterRecoveryRequiredLocked(
		const FShanmenItemSaveResult& SaveResult,
		const FShanmenItemLoadResult& LoadResult,
		const FShanmenItemTransactionReceipt& Receipt,
		bool bRepositoryMutated,
		const FString& Diagnostic);
};
