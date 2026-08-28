#pragma once

#include "CoreMinimal.h"
#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapProfilePreparationTypes.h"

enum class Edemo_mapProfilePreparationRiskPhase : uint8
{
	Safe,
	WillBeAtRisk,
	AtRisk
};

struct Fdemo_mapProfilePreparationRowView
{
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	int32 StackCount = 0;
	int32 Durability = 0;
	int32 MaxDurability = 0;
	int32 Charges = 0;
	int32 MaxCharges = 0;
	FString ResourceLabel;
	Edemo_mapRewardEventKind RewardEventKind = Edemo_mapRewardEventKind::None;
	FGuid RewardEventId;
	int32 RewardValueMultiplierBps = Fdemo_mapRewardEventRules::NormalMultiplierBps;
	FName RewardSourceRoleId = NAME_None;
	FGuid RareRewardEventId;
	FName RareRewardPolicyId = NAME_None;
	FName RareRewardTierId = NAME_None;
	int64 RareRewardBonusValue = 0;
	Fdemo_mapRewardAffixSet AffixSet;
	FString RewardLabel;
	FName CompatibleEquipmentSlotId = NAME_None;
	FName ItemCategoryId = NAME_None;
	FString SlotOrCategoryLabel;
	FString RiskLabel;
	int32 GridIndex = INDEX_NONE;
	bool bSelected = false;
	bool bMaterialSelectionEligible = false;
	bool bStashOnly = false;
	bool bCanSell = false;
	int64 UnitSellPrice = 0;
	int64 TotalSellPrice = 0;
	FString SellDiagnostic;
};

struct Fdemo_mapProfileShopRowView
{
	FName SlotId = NAME_None;
	int32 SlotOrdinal = INDEX_NONE;
	Edemo_mapPersistentShopStockEntryState State =
		Edemo_mapPersistentShopStockEntryState::Available;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FString CategoryAndLevel;
	int32 MaxStackSize = 1;
	int64 BuyPrice = 0;
	int64 SellPrice = 0;
	Fdemo_mapRewardAffixSet AffixSet;
	FString AffixLabel;
	FString StockLabel;
	bool bCanAfford = false;
	bool bCanBuy = false;
	FString BuyDiagnostic;
};

struct Fdemo_mapProfilePreparationEquipmentSlotView
{
	FName SlotId = NAME_None;
	FGuid ItemInstanceId;
	FString DisplayName;
};

struct Fdemo_mapProfilePreparationViewState
{
	Edemo_mapProfileSessionState SessionState = Edemo_mapProfileSessionState::Uninitialized;
	FGuid ProfileId;
	int32 SaveGeneration = 0;
	int64 PersistentSpiritStones = 0;
	FName ShopStockPolicyId = NAME_None;
	int32 ShopStockGeneration = INDEX_NONE;
	FGuid ShopStockEventId;
	TArray<Fdemo_mapProfileShopRowView> OrderedShopRows;
	TArray<Fdemo_mapProfilePreparationRowView> OrderedPermanentStashRows;
	TArray<Fdemo_mapProfilePreparationEquipmentSlotView> OrderedEquipmentSlots;
	TArray<FGuid> OrderedSelectedMaterialIds;
	Fdemo_mapGridContainerSnapshot PermanentStashGrid;
	Fdemo_mapGridContainerSnapshot RunInventoryGrid;
	Fdemo_mapEntityLoadoutView EntityLoadout;
	Fdemo_mapHotbarBindingSnapshot HotbarBindings;
	int32 RunInventoryUsedSlots = 0;
	int32 RunInventoryCapacity = 0;
	Edemo_mapProfilePreparationRiskPhase RiskPhase = Edemo_mapProfilePreparationRiskPhase::Safe;
	FString SessionLabel;
	FString RiskLabel;
	FString RiskWarning;
	FString VisibleDiagnostic;
	FGuid LastSettlementId;
	Edemo_mapRunEndReason LastTerminalReason = Edemo_mapRunEndReason::None;
	FString LastSettlementSummary;
	bool bPreparationOperationsEnabled = false;
	bool bCanStartRun = false;
	bool bCanRetrySettlement = false;
	FString LastTradeSummary;
};

/** Pure value projection from the existing Preparation snapshot and immutable item definitions. */
struct Fdemo_mapProfilePreparationPresenter
{
	static Fdemo_mapProfilePreparationViewState BuildViewState(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot);
	static FString SessionStateLabel(Edemo_mapProfileSessionState State);
	static FString TerminalReasonLabel(Edemo_mapRunEndReason Reason);
	static FString SlotLabel(FName SlotId);
};
