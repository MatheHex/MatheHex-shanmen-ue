#pragma once

#include "CoreMinimal.h"
#include "demo_mapItemTypes.h"
#include "demo_mapProfileRunTypes.h"
#include "demo_mapProfileSettlementTypes.h"

enum class Edemo_mapProfileSessionState : uint8
{
	Uninitialized,
	ReadyForPreparation,
	RunActive,
	PendingSettlementRetry,
	RecoveryRequired,
	FatalProfileError
};

enum class Edemo_mapProfileSessionErrorClass : uint8
{
	None,
	LoadFailure,
	PreparedRecoveryFailure,
	StaleIntent,
	PersistenceFailure,
	RuntimeMaterializationFailure,
	SettlementEvidenceInvalid,
	AmbiguousCommit
};

struct Fdemo_mapProfileSessionSnapshot
{
	Edemo_mapProfileSessionState SessionState = Edemo_mapProfileSessionState::Uninitialized;
	FGuid ProfileId;
	int32 SaveGeneration = 0;
	int64 PersistentSpiritStones = 0;
	int32 TownLevel = 0;
	int64 RiskSpiritStones = 0;
	TArray<Fdemo_mapPersistentItemRecord> OrderedPermanentStash;
	Fdemo_mapPersistentShopStockState ShopStock;
	Fdemo_mapPersistentPreparationLayout PreparationLayout;
	Fdemo_mapPersistentWarehouseLayout WarehouseLayout;
	TArray<FName> ConsumedSpiritStoneSourceIds;
	FGuid ActiveRunId;
	FGuid LastSettlementId;
	Edemo_mapRunEndReason LastTerminalReason = Edemo_mapRunEndReason::None;
	bool bCanBeginRun = false;
	bool bCanRetrySettlement = false;
	Edemo_mapProfileSessionErrorClass ErrorClassification = Edemo_mapProfileSessionErrorClass::None;
	FString VisibleDiagnostic;
};

enum class Edemo_mapProfileSessionInitializeStatus : uint8
{
	Ready,
	RecoveredAbandonCommitted,
	RecoveredAbandonAlreadyCommitted,
	RecoveryRequired,
	FatalProfileError
};

struct Fdemo_mapProfileSessionInitializeResult
{
	Edemo_mapProfileSessionInitializeStatus Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
	FString Diagnostic;
	Fdemo_mapProfileSessionSnapshot Snapshot;

	bool IsReady() const
	{
		return Status == Edemo_mapProfileSessionInitializeStatus::Ready
			|| Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted
			|| Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted;
	}
};

enum class Edemo_mapProfileSessionBeginStatus : uint8
{
	CommittedAndMaterialized,
	SessionNotReady,
	StaleIntent,
	PersistentCommitRejected,
	CommitOutcomeRequiresReload,
	RuntimeMaterializationFailed
};

struct Fdemo_mapProfileSessionBeginResult
{
	Edemo_mapProfileSessionBeginStatus Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
	FString Diagnostic;
	Fdemo_mapBeginRunResult PersistentResult;
	Fdemo_mapPreparedRunRuntimeResult RuntimeResult;
	Fdemo_mapProfileSessionSnapshot Snapshot;

	bool IsRunActive() const { return Status == Edemo_mapProfileSessionBeginStatus::CommittedAndMaterialized; }
};

enum class Edemo_mapProfileSessionSettlementStatus : uint8
{
	Committed,
	AlreadyCommitted,
	ReconciledAfterReload,
	PendingRetry,
	SessionStateRejected,
	EvidenceRejected,
	NoPendingSettlement,
	FatalProfileError,
	/** Runtime-only technical rollback; durable Shanmen ActiveRun remains resumable. */
	RuntimeRollbackReady
};

struct Fdemo_mapProfileSessionSettlementResult
{
	Edemo_mapProfileSessionSettlementStatus Status = Edemo_mapProfileSessionSettlementStatus::SessionStateRejected;
	FString Diagnostic;
	Fdemo_mapProfileSettlementResult PersistentResult;
	Fdemo_mapProfileSessionSnapshot Snapshot;

	bool IsDurablySettled() const
	{
		return Status == Edemo_mapProfileSessionSettlementStatus::Committed
			|| Status == Edemo_mapProfileSessionSettlementStatus::AlreadyCommitted
			|| Status == Edemo_mapProfileSessionSettlementStatus::ReconciledAfterReload;
	}
};

enum class Edemo_mapProfileGeneratedRewardSourceStatus : uint8
{
	Committed,
	AlreadyCommitted,
	ReconciledAfterReload,
	CommittedReconciliationRequired,
	SessionStateRejected,
	ReceiptRejected,
	DuplicateSourceRejected,
	PersistentCommitRejected,
	RecoveryRequired
};

/** Result for the one Profile Repository candidate that owns receipt and source items. */
struct Fdemo_mapProfileGeneratedRewardSourceResult
{
	Edemo_mapProfileGeneratedRewardSourceStatus Status =
		Edemo_mapProfileGeneratedRewardSourceStatus::SessionStateRejected;
	FString Diagnostic;
	Fdemo_mapPersistentGeneratedRewardSource Source;
	int32 SaveGenerationAfter = 0;

	bool IsDurablyCommitted() const
	{
		return Status == Edemo_mapProfileGeneratedRewardSourceStatus::Committed
			|| Status == Edemo_mapProfileGeneratedRewardSourceStatus::AlreadyCommitted
			|| Status == Edemo_mapProfileGeneratedRewardSourceStatus::ReconciledAfterReload
			|| Status == Edemo_mapProfileGeneratedRewardSourceStatus::CommittedReconciliationRequired;
	}

	bool RequiresRuntimeReconciliation() const
	{
		return Status == Edemo_mapProfileGeneratedRewardSourceStatus::CommittedReconciliationRequired;
	}
};
