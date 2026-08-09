#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

struct Fdemo_mapLoadoutSelection
{
	FGuid WeaponItemInstanceId;
	FGuid ArmorItemInstanceId;
	FGuid AccessoryItemInstanceId;
	FGuid SpatialRingItemInstanceId;
	FGuid BackpackItemInstanceId;
	TArray<FGuid> MaterialStackItemInstanceIds;
	TArray<FGuid> HotbarItemInstanceIds;
};

struct Fdemo_mapBeginRunRequest
{
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	Fdemo_mapLoadoutSelection Loadout;
	/** Production P8 path is true. False preserves direct P1-P7 transaction compatibility only. */
	bool bRequireCommittedPreparationLayout = false;
};

enum class Edemo_mapBeginRunStatus : uint8
{
	Committed,
	ProfileValidationRejected,
	ProfileIdentityMismatch,
	ProfileGenerationMismatch,
	ActiveRunAlreadyExists,
	DuplicateSelection,
	SelectedItemNotFound,
	SelectedItemNotInPermanentStash,
	SelectionLimitExceeded,
	EquipmentSlotOrCompatibilityRejected,
	MaterialSelectionRejected,
	RuntimeCapacityContractRejected,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapCommittedRunLoadoutPlan
{
	FGuid ProfileId;
	int32 CommittedGeneration = 0;
	FGuid ActiveRunId;
	TArray<Fdemo_mapPersistentItemRecord> OrderedItems;
	TArray<FGuid> DeployedItemIds;
	TArray<FGuid> HotbarItemInstanceIds;
	int32 RunInventoryCapacity = 0;
};

struct Fdemo_mapBeginRunResult
{
	Edemo_mapBeginRunStatus Status = Edemo_mapBeginRunStatus::ProfileValidationRejected;
	FString Diagnostic;
	FGuid ProfileId;
	int32 PreviousGeneration = 0;
	int32 CommittedGeneration = 0;
	FGuid ActiveRunId;
	bool bDiskStateChanged = false;
	Edemo_mapProfileSaveStatus RepositorySaveStatus = Edemo_mapProfileSaveStatus::ValidationRejected;
	TOptional<Fdemo_mapPersistentProfile> CommittedProfile;
	TOptional<Fdemo_mapCommittedRunLoadoutPlan> CommittedLoadoutPlan;

	bool IsCommitted() const { return Status == Edemo_mapBeginRunStatus::Committed; }
};

enum class Edemo_mapPreparedRunRuntimeStatus : uint8
{
	Materialized,
	CommittedPlanInvalid,
	RuntimeNotIdle,
	RunIdInvalidOrConflicting,
	DuplicateOrMissingDeployedId,
	UnknownDefinition,
	QuantityRejected,
	SlotCompatibilityRejected,
	CapacityRejected,
	AuthorityMutationRejected,
	RollbackFailed
};

struct Fdemo_mapPreparedRunRuntimeRequest
{
	Fdemo_mapCommittedRunLoadoutPlan CommittedPlan;
};

struct Fdemo_mapPreparedRunRuntimeResult
{
	Edemo_mapPreparedRunRuntimeStatus Status = Edemo_mapPreparedRunRuntimeStatus::CommittedPlanInvalid;
	FString Diagnostic;
	FGuid ActiveRunId;
	TArray<FGuid> DeployedItemIds;

	bool IsMaterialized() const { return Status == Edemo_mapPreparedRunRuntimeStatus::Materialized; }
};
