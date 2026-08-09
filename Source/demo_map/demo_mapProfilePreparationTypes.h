#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffixTypes.h"
#include "demo_mapRewardEventTypes.h"
#include "demo_mapProfileSessionTypes.h"
#include "demo_mapProfileTradeTypes.h"

enum class Edemo_mapProfilePreparationSelectionStatus : uint8
{
	Accepted,
	SessionNotReady,
	StaleSelectionCleared,
	ItemNotFound,
	DuplicateSelection,
	EquipmentSlotRejected,
	MaterialRejected,
	SelectionLimitExceeded
};

struct Fdemo_mapProfilePreparationStashRow
{
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	int32 StackCount = 0;
	Edemo_mapRewardEventKind RewardEventKind = Edemo_mapRewardEventKind::None;
	FGuid RewardEventId;
	int32 RewardValueMultiplierBps = Fdemo_mapRewardEventRules::NormalMultiplierBps;
	FName RewardSourceRoleId = NAME_None;
	FGuid RareRewardEventId;
	FName RareRewardPolicyId = NAME_None;
	FName RareRewardTierId = NAME_None;
	int64 RareRewardBonusValue = 0;
	Fdemo_mapRewardAffixSet AffixSet;
	FName CompatibleEquipmentSlotId = NAME_None;
	FName ItemCategoryId = NAME_None;
	bool bMaterialSelectionEligible = false;
	bool bInBaseQuickItemArea = false;
	bool bSelected = false;
	bool bSafeInPermanentStash = false;
};

struct Fdemo_mapProfilePreparationSnapshot
{
	Edemo_mapProfileSessionState SessionState = Edemo_mapProfileSessionState::Uninitialized;
	FGuid ProfileId;
	int32 SaveGeneration = 0;
	int64 PersistentSpiritStones = 0;
	FName ShopStockPolicyId = NAME_None;
	int32 ShopStockGeneration = INDEX_NONE;
	FGuid ShopStockEventId;
	TArray<Fdemo_mapProfileShopStockRow> OrderedShopStockRows;
	/** Legacy test-only shape; the product presenter no longer reads it. */
	TArray<Fdemo_mapProfileShopCatalogRow> OrderedShopCatalogRows;
	TArray<Fdemo_mapProfilePreparationStashRow> OrderedPermanentStashRows;
	Fdemo_mapPersistentWarehouseLayout WarehouseLayout;
	FGuid SelectedWeaponId;
	FGuid SelectedArmorId;
	FGuid SelectedAccessoryId;
	FGuid SelectedSpatialRingId;
	FGuid SelectedBackpackId;
	TArray<FGuid> OrderedSelectedMaterialIds;
	Fdemo_mapHotbarBindingSnapshot HotbarBindings;
	bool bCanStartRun = false;
	bool bCanRetrySettlement = false;
	FString VisibleDiagnostic;
	FGuid LastSettlementId;
	Edemo_mapRunEndReason LastTerminalReason = Edemo_mapRunEndReason::None;
	TOptional<Fdemo_mapProfileTradeResult> LastTradeResult;
};

struct Fdemo_mapProfilePreparationSelectionResult
{
	Edemo_mapProfilePreparationSelectionStatus Status = Edemo_mapProfilePreparationSelectionStatus::SessionNotReady;
	FString Diagnostic;
	Fdemo_mapProfilePreparationSnapshot Snapshot;

	bool IsAccepted() const { return Status == Edemo_mapProfilePreparationSelectionStatus::Accepted; }
};
