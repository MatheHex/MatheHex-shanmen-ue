#pragma once

#include "CoreMinimal.h"
#include "demo_mapItemTypes.h"
#include "demo_mapPersistentProfileTypes.h"

enum class Edemo_mapProfileSettlementStatus : uint8
{
	Committed,
	AlreadyCommitted,
	ProfileValidationRejected,
	ProfileIdentityMismatch,
	ProfileGenerationMismatch,
	ActiveRunIdMismatch,
	NoPreparedRun,
	InvalidActiveRunState,
	InvalidEndReason,
	SnapshotMissing,
	SnapshotUnexpected,
	SnapshotRunIdMismatch,
	SnapshotReasonMismatch,
	SnapshotInvalid,
	ItemIdentityDuplicate,
	ItemIdentityConflict,
	DeployedIdentityMismatch,
	AcquiredOriginMismatch,
	UnknownItemDefinition,
	InvalidItemQuantity,
	CurrencyOverflow,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapProfileSettlementRequest
{
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	FGuid ExpectedActiveRunId;
	Edemo_mapRunEndReason RequestedEndReason = Edemo_mapRunEndReason::None;
	TOptional<Fdemo_mapRuntimeSettlementSnapshot> RuntimeSnapshot;
};

struct Fdemo_mapProfileSettlementResult
{
	Edemo_mapProfileSettlementStatus Status = Edemo_mapProfileSettlementStatus::ProfileValidationRejected;
	FString Diagnostic;
	FGuid ProfileId;
	int32 PreviousGeneration = 0;
	int32 CommittedGeneration = 0;
	FGuid ActiveRunId;
	FGuid SettlementId;
	Edemo_mapRunEndReason CommittedEndReason = Edemo_mapRunEndReason::None;
	bool bDiskStateChanged = false;
	Edemo_mapProfileSaveStatus RepositorySaveStatus = Edemo_mapProfileSaveStatus::ValidationRejected;
	TOptional<Fdemo_mapPersistentProfile> CommittedProfile;
	TArray<FGuid> OrderedSecuredItemIds;
	TArray<FGuid> ClearedPreparationItemIds;
	int64 RiskBefore = 0;
	int64 RiskTransferred = 0;
	int64 RiskLost = 0;
	int64 PersistentBefore = 0;
	int64 PersistentAfter = 0;

	bool IsCommitted() const { return Status == Edemo_mapProfileSettlementStatus::Committed; }
	bool IsIdempotentSuccess() const { return IsCommitted() || Status == Edemo_mapProfileSettlementStatus::AlreadyCommitted; }
};
