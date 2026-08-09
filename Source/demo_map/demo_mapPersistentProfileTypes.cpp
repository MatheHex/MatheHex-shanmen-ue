#include "demo_mapPersistentProfileTypes.h"
#include "Misc/Paths.h"

bool Fdemo_mapPersistentItemRecord::operator==(const Fdemo_mapPersistentItemRecord& Other) const
{
	return ItemInstanceId == Other.ItemInstanceId
		&& ItemDefinitionId == Other.ItemDefinitionId
		&& StackCount == Other.StackCount
		&& PersistentDomain == Other.PersistentDomain
		&& LegacySpatialParentItemInstanceId
			== Other.LegacySpatialParentItemInstanceId
		&& EquipmentSlotId == Other.EquipmentSlotId
		&& OriginRunId == Other.OriginRunId
		&& RewardEventKind == Other.RewardEventKind
		&& RewardEventId == Other.RewardEventId
		&& RewardValueMultiplierBps
			== Other.RewardValueMultiplierBps
		&& RewardSourceRoleId == Other.RewardSourceRoleId
		&& RareRewardEventId == Other.RareRewardEventId
		&& RareRewardPolicyId == Other.RareRewardPolicyId
		&& RareRewardTierId == Other.RareRewardTierId
		&& RareRewardBonusValue == Other.RareRewardBonusValue
		&& AffixSet == Other.AffixSet;
}

bool Fdemo_mapPersistentShopStockEntry::operator==(
	const Fdemo_mapPersistentShopStockEntry& Other) const
{
	return SlotId == Other.SlotId
		&& SlotOrdinal == Other.SlotOrdinal
		&& State == Other.State
		&& Item == Other.Item
		&& SoldItemInstanceId == Other.SoldItemInstanceId
		&& QuotedBuyValue == Other.QuotedBuyValue;
}

bool Fdemo_mapPersistentShopStockState::operator==(
	const Fdemo_mapPersistentShopStockState& Other) const
{
	return bInitialized == Other.bInitialized
		&& PolicyId == Other.PolicyId
		&& Generation == Other.Generation
		&& ShopStockEventId == Other.ShopStockEventId
		&& LastAppliedTerminalId == Other.LastAppliedTerminalId
		&& Entries == Other.Entries;
}

bool Fdemo_mapPersistentActiveRunRecord::operator==(const Fdemo_mapPersistentActiveRunRecord& Other) const
{
	return bHasActiveRun == Other.bHasActiveRun
		&& ActiveRunId == Other.ActiveRunId
		&& ActiveRunState == Other.ActiveRunState
		&& RiskSpiritStones == Other.RiskSpiritStones
		&& DeployedItemIds == Other.DeployedItemIds
		&& ActiveRunItems == Other.ActiveRunItems
		&& ConsumedSpiritStoneSourceIds == Other.ConsumedSpiritStoneSourceIds
		&& CommittedSettlementId == Other.CommittedSettlementId;
}

bool Fdemo_mapPersistentPreparationLayout::operator==(const Fdemo_mapPersistentPreparationLayout& Other) const
{
	return WeaponItemInstanceId == Other.WeaponItemInstanceId
		&& ArmorItemInstanceId == Other.ArmorItemInstanceId
		&& AccessoryItemInstanceId == Other.AccessoryItemInstanceId
		&& SpatialRingItemInstanceId == Other.SpatialRingItemInstanceId
		&& BackpackItemInstanceId == Other.BackpackItemInstanceId
		&& OrderedRunInventoryItemInstanceIds == Other.OrderedRunInventoryItemInstanceIds
		&& HotbarItemInstanceIds == Other.HotbarItemInstanceIds;
}

bool Fdemo_mapPersistentWarehouseLayout::operator==(
	const Fdemo_mapPersistentWarehouseLayout& Other) const
{
	return bInitialized == Other.bInitialized
		&& SlotItemInstanceIds == Other.SlotItemInstanceIds;
}

bool Fdemo_mapPersistentProfileMetadata::operator==(const Fdemo_mapPersistentProfileMetadata& Other) const
{
	return ProfileName == Other.ProfileName && CreatedUtc == Other.CreatedUtc && LastSavedUtc == Other.LastSavedUtc;
}

bool Fdemo_mapPersistentProfile::operator==(const Fdemo_mapPersistentProfile& Other) const
{
	return SchemaVersion == Other.SchemaVersion
		&& ProfileId == Other.ProfileId
		&& SaveGeneration == Other.SaveGeneration
		&& ProfileMetadata == Other.ProfileMetadata
		&& PersistentSpiritStones == Other.PersistentSpiritStones
		&& TownLevel == Other.TownLevel
		&& PermanentStash == Other.PermanentStash
		&& ShopStock == Other.ShopStock
		&& PreparationLayout == Other.PreparationLayout
		&& WarehouseLayout == Other.WarehouseLayout
		&& ActiveRun == Other.ActiveRun
		&& LastSettlementId == Other.LastSettlementId;
}

Fdemo_mapProfileStorageContext Fdemo_mapProfileStorageContext::Production()
{
	return ForRoot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Shanmen")));
}

Fdemo_mapProfileStorageContext Fdemo_mapProfileStorageContext::ForRoot(const FString& Root)
{
	Fdemo_mapProfileStorageContext Context;
	Context.RootDirectory = FPaths::ConvertRelativePathToFull(Root);
	return Context;
}

FString Fdemo_mapProfileStorageContext::PrimaryPath() const { return FPaths::Combine(RootDirectory, TEXT("Profile_Default.json")); }
FString Fdemo_mapProfileStorageContext::BackupPath() const { return PrimaryPath() + TEXT(".bak"); }
FString Fdemo_mapProfileStorageContext::TempPath() const { return PrimaryPath() + TEXT(".tmp"); }
FString Fdemo_mapProfileStorageContext::CorruptDirectory() const { return FPaths::Combine(RootDirectory, TEXT("Corrupt")); }
