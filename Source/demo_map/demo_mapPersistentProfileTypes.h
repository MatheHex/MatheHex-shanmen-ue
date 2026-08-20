#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffixTypes.h"
#include "demo_mapRewardEventTypes.h"
#include "demo_mapRewardSourceProjection.h"

enum class Edemo_mapPersistentDomain : uint8
{
	PermanentStash,
	ActiveRun,
	ShopStock
};

enum class Edemo_mapPersistentActiveRunState : uint8
{
	None,
	Prepared,
	InProgress,
	Settled,
	Extraction,
	Death,
	Abandon,
	RecoveredAbandon,
	ActivationFailure
};

struct Fdemo_mapPersistentItemRecord
{
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	int32 StackCount = 0;
	Edemo_mapPersistentDomain PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
	/**
	 * Read-only compatibility relationship for an old out-of-raid spatial
	 * container.  Code A does not consume this field; P5 imports it once into
	 * the Code B-owned ChildContainerId graph and retains the mapping in its
	 * receipt.  An invalid reference is reported during migration instead of
	 * inventing a replacement item or silently dropping the valid record.
	 */
	FGuid LegacySpatialParentItemInstanceId;
	FName EquipmentSlotId = NAME_None;
	FGuid OriginRunId;
	Edemo_mapRewardEventKind RewardEventKind =
		Edemo_mapRewardEventKind::None;
	FGuid RewardEventId;
	int32 RewardValueMultiplierBps =
		Fdemo_mapRewardEventRules::NormalMultiplierBps;
	FName RewardSourceRoleId = NAME_None;
	FGuid RareRewardEventId;
	FName RareRewardPolicyId = NAME_None;
	FName RareRewardTierId = NAME_None;
	int64 RareRewardBonusValue = 0;
	Fdemo_mapRewardAffixSet AffixSet;

	bool operator==(const Fdemo_mapPersistentItemRecord& Other) const;
};

enum class Edemo_mapPersistentShopStockEntryState : uint8
{
	Available,
	Sold
};

struct Fdemo_mapPersistentShopStockEntry
{
	FName SlotId = NAME_None;
	int32 SlotOrdinal = INDEX_NONE;
	Edemo_mapPersistentShopStockEntryState State =
		Edemo_mapPersistentShopStockEntryState::Available;
	Fdemo_mapPersistentItemRecord Item;
	FGuid SoldItemInstanceId;
	int64 QuotedBuyValue = 0;

	bool operator==(const Fdemo_mapPersistentShopStockEntry& Other) const;
};

/**
 * Optional Schema 3 shop state. bInitialized=false is the only legacy/default
 * representation and is materialized exactly once by the Preparation authority.
 */
struct Fdemo_mapPersistentShopStockState
{
	bool bInitialized = false;
	FName PolicyId = NAME_None;
	int32 Generation = INDEX_NONE;
	FGuid ShopStockEventId;
	FGuid LastAppliedTerminalId;
	TArray<Fdemo_mapPersistentShopStockEntry> Entries;

	bool operator==(const Fdemo_mapPersistentShopStockState& Other) const;
};

/**
 * The durable, pre-runtime item projection for one generated source.  The
 * Profile Repository owns these IDs before any ItemAuthority or Actor write;
 * section/slot preserves the container presentation without another roll.
 */
struct Fdemo_mapPersistentGeneratedRewardSourceEntry
{
	Fdemo_mapPersistentItemRecord Item;
	Edemo_mapRuntimeContainerSection Section =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 SlotIndex = INDEX_NONE;

	bool operator==(const Fdemo_mapPersistentGeneratedRewardSourceEntry& Other) const;
};

/** One atomic Profile candidate: immutable receipt plus its exact source items. */
struct Fdemo_mapPersistentGeneratedRewardSource
{
	Fdemo_mapRewardSourceAcceptanceReceipt Receipt;
	FGuid ContainerId;
	TArray<Fdemo_mapPersistentGeneratedRewardSourceEntry> Entries;

	bool operator==(const Fdemo_mapPersistentGeneratedRewardSource& Other) const;
};

struct Fdemo_mapPersistentActiveRunRecord
{
	bool bHasActiveRun = false;
	FGuid ActiveRunId;
	Edemo_mapPersistentActiveRunState ActiveRunState = Edemo_mapPersistentActiveRunState::None;
	int64 RiskSpiritStones = 0;
	TArray<FGuid> DeployedItemIds;
	TArray<Fdemo_mapPersistentItemRecord> ActiveRunItems;
	/** P73.4 durable duplicate gate and recoverable generated-source projection. */
	TArray<Fdemo_mapPersistentGeneratedRewardSource> GeneratedRewardSources;
	TArray<FName> ConsumedSpiritStoneSourceIds;
	FGuid CommittedSettlementId;

	bool operator==(const Fdemo_mapPersistentActiveRunRecord& Other) const;
};

/**
 * Schema 3 preparation authority. Invalid GUIDs are empty optional slots.
 * HotbarItemInstanceIds is always exactly nine entries and only references
 * selected consumables from OrderedRunInventoryItemInstanceIds.
 */
struct Fdemo_mapPersistentPreparationLayout
{
	static constexpr int32 BaseQuickItemSlotCount = 6;
	static constexpr int32 MaximumRingQuickItemCount = 12;
	static constexpr int32 MaximumSpatialStorageItemCount = 36;
	static constexpr int32 MaxRunInventoryItems =
		BaseQuickItemSlotCount
		+ MaximumRingQuickItemCount
		+ MaximumSpatialStorageItemCount;
	static constexpr int32 HotbarSlotCount = 9;

	Fdemo_mapPersistentPreparationLayout()
	{
		HotbarItemInstanceIds.Init(FGuid(), HotbarSlotCount);
	}

	FGuid WeaponItemInstanceId;
	FGuid ArmorItemInstanceId;
	FGuid AccessoryItemInstanceId;
	FGuid SpatialRingItemInstanceId;
	FGuid BackpackItemInstanceId;
	TArray<FGuid> OrderedRunInventoryItemInstanceIds;
	TArray<FGuid> HotbarItemInstanceIds;

	bool IsEmpty() const
	{
		return !WeaponItemInstanceId.IsValid()
			&& !ArmorItemInstanceId.IsValid()
			&& !AccessoryItemInstanceId.IsValid()
			&& !SpatialRingItemInstanceId.IsValid()
			&& !BackpackItemInstanceId.IsValid()
			&& OrderedRunInventoryItemInstanceIds.IsEmpty()
			&& !HotbarItemInstanceIds.ContainsByPredicate([](const FGuid& Id) { return Id.IsValid(); });
	}

	bool operator==(const Fdemo_mapPersistentPreparationLayout& Other) const;
};

/**
 * Optional warehouse slot hints. PermanentStash remains the ownership and item
 * authority; an uninitialized layout is the legacy representation and projects
 * stash array order deterministically. Invalid GUIDs are intentional empty
 * cells. Stale hints are ignored so existing trade/settlement transactions do
 * not need to become warehouse-layout authorities.
 */
struct Fdemo_mapPersistentWarehouseLayout
{
	static constexpr int32 SlotCount = 30;

	bool bInitialized = false;
	TArray<FGuid> SlotItemInstanceIds;

	bool operator==(const Fdemo_mapPersistentWarehouseLayout& Other) const;
};

struct Fdemo_mapPersistentProfileMetadata
{
	FString ProfileName = TEXT("Default");
	FString CreatedUtc;
	FString LastSavedUtc;

	bool operator==(const Fdemo_mapPersistentProfileMetadata& Other) const;
};

struct Fdemo_mapPersistentProfile
{
	static constexpr int32 CurrentSchemaVersion = 7;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid ProfileId;
	int32 SaveGeneration = 0;
	Fdemo_mapPersistentProfileMetadata ProfileMetadata;
	int64 PersistentSpiritStones = 0;
	/** P1 town foundation; levels 0—5 are display-only until a later upgrade task. */
	int32 TownLevel = 0;
	TArray<Fdemo_mapPersistentItemRecord> PermanentStash;
	Fdemo_mapPersistentShopStockState ShopStock;
	Fdemo_mapPersistentPreparationLayout PreparationLayout;
	Fdemo_mapPersistentWarehouseLayout WarehouseLayout;
	Fdemo_mapPersistentActiveRunRecord ActiveRun;
	FGuid LastSettlementId;

	bool operator==(const Fdemo_mapPersistentProfile& Other) const;
};

enum class Edemo_mapProfileLoadStatus : uint8
{
	LoadedPrimary,
	CreatedFreshAndCommitted,
	RecoveredFromBackup,
	PrimaryMissingBackupRecovered,
	FutureSchemaRejected,
	CorruptPrimaryNoValidBackup,
	InvalidProfileData,
	ReadFailed,
	WriteRecoveryFailed
};

enum class Edemo_mapProfileSaveStatus : uint8
{
	Saved,
	ValidationRejected,
	SerializationFailed,
	TempWriteFailed,
	TempFlushOrCloseFailed,
	TempVerificationFailed,
	BackupPreparationFailed,
	AtomicReplaceFailed,
	PostCommitVerificationFailed
};

enum class Edemo_mapProfileFailureStage : uint8
{
	None,
	CreateDirectory,
	WriteTemp,
	FlushOrCloseTemp,
	ReadBackTemp,
	ValidateTemp,
	PrepareBackup,
	AtomicReplace,
	ReadBackCommittedPrimary,
	CleanupTemp
};

struct Fdemo_mapProfileStorageContext
{
	FString RootDirectory;
#if WITH_DEV_AUTOMATION_TESTS
	Edemo_mapProfileFailureStage InjectedFailure = Edemo_mapProfileFailureStage::None;
#endif

	static Fdemo_mapProfileStorageContext Production();
	static Fdemo_mapProfileStorageContext ForRoot(const FString& Root);
	FString PrimaryPath() const;
	FString BackupPath() const;
	FString TempPath() const;
	FString CorruptDirectory() const;
};

struct Fdemo_mapProfileSaveResult
{
	Edemo_mapProfileSaveStatus Status = Edemo_mapProfileSaveStatus::ValidationRejected;
	FString Diagnostic;
	FString PrimaryPath;
	FString BackupPath;
	FString TempPath;
	bool bDiskStateChanged = false;
	bool bCleanupSucceeded = true;
	int32 CommittedGeneration = 0;

	bool IsSuccess() const { return Status == Edemo_mapProfileSaveStatus::Saved; }
};

struct Fdemo_mapProfileLoadResult
{
	Edemo_mapProfileLoadStatus Status = Edemo_mapProfileLoadStatus::ReadFailed;
	FString Diagnostic;
	FString PrimaryPath;
	FString BackupPath;
	FString TempPath;
	FString QuarantinedPath;
	bool bDiskStateChanged = false;
	int32 CommittedGeneration = 0;
	Fdemo_mapPersistentProfile Profile;

	bool IsSuccess() const
	{
		return Status == Edemo_mapProfileLoadStatus::LoadedPrimary
			|| Status == Edemo_mapProfileLoadStatus::CreatedFreshAndCommitted
			|| Status == Edemo_mapProfileLoadStatus::RecoveredFromBackup
			|| Status == Edemo_mapProfileLoadStatus::PrimaryMissingBackupRecovered;
	}
};
