#include "demo_mapRewardGenerationRegistry.h"

#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"

const FName Fdemo_mapRewardBudgetProfileIds::EnemyStandard(
	TEXT("Reward.Budget.Enemy.Standard"));
const FName Fdemo_mapRewardBudgetProfileIds::EnemyElite(
	TEXT("Reward.Budget.Enemy.Elite"));
const FName Fdemo_mapRewardBudgetProfileIds::ContainerHighValue(
	TEXT("Reward.Budget.Container.HighValue"));
const FName Fdemo_mapRewardBudgetProfileIds::Boss(
	TEXT("Reward.Budget.Boss"));
const FName Fdemo_mapRewardBudgetProfileIds::ContainerBasic(
	TEXT("Reward.Budget.Container.Basic"));
const FName Fdemo_mapRewardBudgetProfileIds::M01EnemyLow(
	TEXT("M01.Reward.Budget.Enemy.LOW"));
const FName Fdemo_mapRewardBudgetProfileIds::M01EnemyMid(
	TEXT("M01.Reward.Budget.Enemy.MID"));
const FName Fdemo_mapRewardBudgetProfileIds::M01ResourceTier1(
	TEXT("M01.Reward.Budget.Resource.TIER_1"));
const FName Fdemo_mapRewardBudgetProfileIds::M01ResourceTier2(
	TEXT("M01.Reward.Budget.Resource.TIER_2"));
const FName Fdemo_mapRewardBudgetProfileIds::M01ResourceTier3(
	TEXT("M01.Reward.Budget.Resource.TIER_3"));

const FName Fdemo_mapRewardTagIds::SourceContainerGeneral(
	TEXT("Reward.Source.Container.General"));
const FName Fdemo_mapRewardTagIds::SourceContainerHighValue(
	TEXT("Reward.Source.Container.HighValue"));
const FName Fdemo_mapRewardTagIds::ItemEquipmentWeapon(
	TEXT("Reward.Item.Equipment.Weapon"));
const FName Fdemo_mapRewardTagIds::ItemEquipmentRobe(
	TEXT("Reward.Item.Equipment.Robe"));
const FName Fdemo_mapRewardTagIds::ItemEquipmentAccessory(
	TEXT("Reward.Item.Equipment.Accessory"));
const FName Fdemo_mapRewardTagIds::ItemEquipmentBackpack(
	TEXT("Reward.Item.Equipment.Backpack"));
const FName Fdemo_mapRewardTagIds::ItemConsumablePill(
	TEXT("Reward.Item.Consumable.Pill"));
const FName Fdemo_mapRewardTagIds::ItemMaterialWood(
	TEXT("Reward.Item.Material.Wood"));
const FName Fdemo_mapRewardTagIds::ItemMaterialOre(
	TEXT("Reward.Item.Material.Ore"));
const FName Fdemo_mapRewardTagIds::ItemBodyBone(
	TEXT("Reward.Item.Body.Bone"));
const FName Fdemo_mapRewardTagIds::ItemBodyInnerCore(
	TEXT("Reward.Item.Body.InnerCore"));

const FName Fdemo_mapRewardSourceIds::ChestMainA(
	TEXT("P7.RewardSource.Chest.Main.A"));
const FName Fdemo_mapRewardSourceIds::ChestMainB(
	TEXT("P7.RewardSource.Chest.Main.B"));
const FName Fdemo_mapRewardSourceIds::ChestSideHighValue(
	TEXT("P1.RewardSource.Chest.Side.HighValue"));

namespace
{
	Fdemo_mapRewardBudgetProfile Budget(
		FName Id,
		int32 PlannedCount,
		int64 BaseValue)
	{
		Fdemo_mapRewardBudgetProfile Result;
		Result.ProfileId = Id;
		Result.PlannedSourceCount = PlannedCount;
		Result.BaseValue = BaseValue;
		Result.MinMultiplierBps =
			Fdemo_mapRewardGenerationRegistry::NormalMultiplierMinBps;
		Result.MaxMultiplierBps =
			Fdemo_mapRewardGenerationRegistry::NormalMultiplierMaxBps;
		return Result;
	}

	Fdemo_mapRewardPoolEntry Pool(
		const TCHAR* EntryId,
		FName DefinitionId,
		FName ItemTag,
		int64 Weight,
		int32 MaxStack)
	{
		Fdemo_mapRewardPoolEntry Result;
		Result.EntryId = FName(EntryId);
		Result.DefinitionId = DefinitionId;
		Result.ItemTags = { ItemTag };
		Result.RequiredSourceTags = {
			Fdemo_mapRewardTagIds::SourceContainerGeneral,
			Fdemo_mapRewardTagIds::SourceContainerHighValue
		};
		Result.Weight = Weight;
		Result.MinStack = 1;
		Result.MaxStack = MaxStack;
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		Result.MinItemLevel = Definition ? Definition->Level : 0;
		Result.MaxItemLevel = Definition ? Definition->Level : MAX_int32;
		Result.MinUnitValue = 1;
		Result.MaxUnitValue = MAX_int64;
		return Result;
	}

	Fdemo_mapRewardSourceDefinition FixedSource(
		FName Marker,
		FName Source,
		FName Table)
	{
		Fdemo_mapRewardSourceDefinition Result;
		Result.MarkerId = Marker;
		Result.RewardSourceId = Source;
		Result.Mode = Edemo_mapRewardSourceMode::FixedTable;
		Result.TargetSection = Edemo_mapRuntimeContainerSection::Chest;
		Result.Capacity =
			Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity;
		Result.FixedTableId = Table;
		return Result;
	}

	Fdemo_mapRewardSourceDefinition GeneratedHighValueSource()
	{
		Fdemo_mapRewardSourceDefinition Result;
		Result.MarkerId = Fdemo_mapFixedLootTableIds::MarkerChestSideA;
		Result.RewardSourceId =
			Fdemo_mapRewardSourceIds::ChestSideHighValue;
		Result.Mode = Edemo_mapRewardSourceMode::GeneratedReward;
		Result.BudgetProfileId =
			Fdemo_mapRewardBudgetProfileIds::ContainerHighValue;
		Result.SourceTags = {
			Fdemo_mapRewardTagIds::SourceContainerGeneral,
			Fdemo_mapRewardTagIds::SourceContainerHighValue
		};
		Result.TargetSection =
			Edemo_mapRuntimeContainerSection::Chest;
		Result.Capacity =
			Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity;
		Result.FixedFallbackTableId =
			Fdemo_mapFixedLootTableIds::ChestSideA;
		Result.bAllowFixedFallbackOnFailure = true;
		return Result;
	}
}

const TArray<Fdemo_mapRewardBudgetProfile>&
Fdemo_mapRewardGenerationRegistry::GetBudgetProfiles()
{
	static const TArray<Fdemo_mapRewardBudgetProfile> Profiles = {
		Budget(Fdemo_mapRewardBudgetProfileIds::EnemyStandard, 10, 1200),
		Budget(Fdemo_mapRewardBudgetProfileIds::EnemyElite, 3, 4500),
		Budget(Fdemo_mapRewardBudgetProfileIds::ContainerHighValue, 15, 2000),
		Budget(Fdemo_mapRewardBudgetProfileIds::Boss, 1, 12000),
		Budget(Fdemo_mapRewardBudgetProfileIds::ContainerBasic, 120, 400)
	};
	return Profiles;
}

const TArray<Fdemo_mapRewardBudgetProfile>&
Fdemo_mapRewardGenerationRegistry::GetM01BudgetProfiles()
{
	static const TArray<Fdemo_mapRewardBudgetProfile> Profiles = {
		Budget(Fdemo_mapRewardBudgetProfileIds::M01EnemyLow, 4, 900),
		Budget(Fdemo_mapRewardBudgetProfileIds::M01EnemyMid, 6, 1400),
		Budget(Fdemo_mapRewardBudgetProfileIds::M01ResourceTier1, 48, 250),
		Budget(Fdemo_mapRewardBudgetProfileIds::M01ResourceTier2, 48, 400),
		Budget(Fdemo_mapRewardBudgetProfileIds::M01ResourceTier3, 24, 700)
	};
	return Profiles;
}

const Fdemo_mapRewardBudgetProfile*
Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(FName ProfileId)
{
	const Fdemo_mapRewardBudgetProfile* Core = GetBudgetProfiles().FindByPredicate(
		[ProfileId](const Fdemo_mapRewardBudgetProfile& Profile)
		{
			return Profile.ProfileId == ProfileId;
		});
	return Core ? Core : GetM01BudgetProfiles().FindByPredicate(
		[ProfileId](const Fdemo_mapRewardBudgetProfile& Profile)
		{
			return Profile.ProfileId == ProfileId;
		});
}

const TArray<Fdemo_mapRewardPoolEntry>&
Fdemo_mapRewardGenerationRegistry::GetHighValueContainerPool()
{
	static const TArray<Fdemo_mapRewardPoolEntry> Entries = {
		Pool(TEXT("P1.Pool.Weapon.L1"), Fdemo_mapItemIds::WeaponLevel1, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 36, 1),
		Pool(TEXT("P1.Pool.Weapon.L2"), Fdemo_mapItemIds::WeaponLevel2, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 24, 1),
		Pool(TEXT("P1.Pool.Weapon.L3"), Fdemo_mapItemIds::WeaponLevel3, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 12, 1),
		Pool(TEXT("P1.Pool.Weapon.L4"), Fdemo_mapItemIds::WeaponLevel4, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 5, 1),
		Pool(TEXT("P1.Pool.Robe.L1"), Fdemo_mapItemIds::ArmorRobeLevel1, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 36, 1),
		Pool(TEXT("P1.Pool.Robe.L2"), Fdemo_mapItemIds::ArmorRobeLevel2, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 24, 1),
		Pool(TEXT("P1.Pool.Robe.L3"), Fdemo_mapItemIds::ArmorRobeLevel3, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 12, 1),
		Pool(TEXT("P1.Pool.Robe.L4"), Fdemo_mapItemIds::ArmorRobeLevel4, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 5, 1),
		Pool(TEXT("P1.Pool.Accessory.L1"), Fdemo_mapItemIds::AccessoryLevel1, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 34, 1),
		Pool(TEXT("P1.Pool.Accessory.L2"), Fdemo_mapItemIds::AccessoryLevel2, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 22, 1),
		Pool(TEXT("P1.Pool.Accessory.L3"), Fdemo_mapItemIds::AccessoryLevel3, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 11, 1),
		Pool(TEXT("P1.Pool.Accessory.L4"), Fdemo_mapItemIds::AccessoryLevel4, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 4, 1),
		Pool(TEXT("P1.Pool.Backpack.L1"), Fdemo_mapItemIds::BackpackLevel1, Fdemo_mapRewardTagIds::ItemEquipmentBackpack, 24, 1),
		Pool(TEXT("P1.Pool.Backpack.L2"), Fdemo_mapItemIds::BackpackLevel2, Fdemo_mapRewardTagIds::ItemEquipmentBackpack, 14, 1),
		Pool(TEXT("P1.Pool.Pill.L1"), Fdemo_mapItemIds::HealingPillLevel1, Fdemo_mapRewardTagIds::ItemConsumablePill, 48, 8),
		Pool(TEXT("P1.Pool.Pill.L2"), Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapRewardTagIds::ItemConsumablePill, 32, 6),
		Pool(TEXT("P1.Pool.Pill.L3"), Fdemo_mapItemIds::HealingPillLevel3, Fdemo_mapRewardTagIds::ItemConsumablePill, 18, 4),
		Pool(TEXT("P1.Pool.Wood.L1"), Fdemo_mapItemIds::SpiritWoodLevel1, Fdemo_mapRewardTagIds::ItemMaterialWood, 52, 12),
		Pool(TEXT("P1.Pool.Wood.L2"), Fdemo_mapItemIds::SpiritWoodLevel2, Fdemo_mapRewardTagIds::ItemMaterialWood, 34, 10),
		Pool(TEXT("P1.Pool.Wood.L3"), Fdemo_mapItemIds::SpiritWoodLevel3, Fdemo_mapRewardTagIds::ItemMaterialWood, 18, 8),
		Pool(TEXT("P1.Pool.Ore.L1"), Fdemo_mapItemIds::SpiritOreLevel1, Fdemo_mapRewardTagIds::ItemMaterialOre, 50, 12),
		Pool(TEXT("P1.Pool.Ore.L2"), Fdemo_mapItemIds::SpiritOreLevel2, Fdemo_mapRewardTagIds::ItemMaterialOre, 32, 10),
		Pool(TEXT("P1.Pool.Ore.L3"), Fdemo_mapItemIds::SpiritOreLevel3, Fdemo_mapRewardTagIds::ItemMaterialOre, 16, 8),
		Pool(TEXT("P1.Pool.Bone.Soul"), Fdemo_mapItemIds::SoulBone, Fdemo_mapRewardTagIds::ItemBodyBone, 20, 4),
		Pool(TEXT("P1.Pool.Bone.Spirit"), Fdemo_mapItemIds::SpiritBone, Fdemo_mapRewardTagIds::ItemBodyBone, 10, 3),
		Pool(TEXT("P1.Pool.Bone.Dao"), Fdemo_mapItemIds::DaoBone, Fdemo_mapRewardTagIds::ItemBodyBone, 4, 2),
		Pool(TEXT("P1.Pool.Core.L5"), Fdemo_mapItemIds::InnerCoreLevel5, Fdemo_mapRewardTagIds::ItemBodyInnerCore, 16, 4),
		Pool(TEXT("P1.Pool.Core.L10"), Fdemo_mapItemIds::InnerCoreLevel10, Fdemo_mapRewardTagIds::ItemBodyInnerCore, 8, 2),
		Pool(TEXT("P1.Pool.Core.L15"), Fdemo_mapItemIds::InnerCoreLevel15, Fdemo_mapRewardTagIds::ItemBodyInnerCore, 3, 1)
	};
	return Entries;
}

const TArray<Fdemo_mapRewardSourceDefinition>&
Fdemo_mapRewardGenerationRegistry::GetChestSources()
{
	static const TArray<Fdemo_mapRewardSourceDefinition> Sources = {
		FixedSource(
			Fdemo_mapFixedLootTableIds::MarkerChestMainA,
			Fdemo_mapRewardSourceIds::ChestMainA,
			Fdemo_mapFixedLootTableIds::ChestMainA),
		FixedSource(
			Fdemo_mapFixedLootTableIds::MarkerChestMainB,
			Fdemo_mapRewardSourceIds::ChestMainB,
			Fdemo_mapFixedLootTableIds::ChestMainB),
		GeneratedHighValueSource()
	};
	return Sources;
}

const Fdemo_mapRewardSourceDefinition*
Fdemo_mapRewardGenerationRegistry::FindChestSource(int32 ChestIndex)
{
	return GetChestSources().IsValidIndex(ChestIndex)
		? &GetChestSources()[ChestIndex]
		: nullptr;
}

bool Fdemo_mapRewardGenerationRegistry::Validate(FString* OutError)
{
	const TArray<Fdemo_mapRewardBudgetProfile>& Profiles =
		GetBudgetProfiles();
	const TArray<Fdemo_mapRewardPoolEntry>& PoolEntries =
		GetHighValueContainerPool();
	const TArray<Fdemo_mapRewardSourceDefinition>& Sources =
		GetChestSources();
	TSet<FName> ProfileIds;
	for (const Fdemo_mapRewardBudgetProfile& Profile : Profiles)
	{
		if (!Profile.IsValid()
			|| Profile.MinMultiplierBps != NormalMultiplierMinBps
			|| Profile.MaxMultiplierBps != NormalMultiplierMaxBps
			|| ProfileIds.Contains(Profile.ProfileId))
		{
			if (OutError) *OutError = TEXT("Reward Budget Profile registry is invalid or duplicated.");
			return false;
		}
		ProfileIds.Add(Profile.ProfileId);
	}
	for (const Fdemo_mapRewardBudgetProfile& Profile : GetM01BudgetProfiles())
	{
		if (!Profile.IsValid()
			|| Profile.MinMultiplierBps != NormalMultiplierMinBps
			|| Profile.MaxMultiplierBps != NormalMultiplierMaxBps
			|| ProfileIds.Contains(Profile.ProfileId))
		{
			if (OutError) *OutError = TEXT("M01 Reward Budget Profile registry is invalid or duplicated.");
			return false;
		}
		ProfileIds.Add(Profile.ProfileId);
	}
	if (Profiles.Num() != 5
		|| !FindBudgetProfile(Fdemo_mapRewardBudgetProfileIds::ContainerHighValue)
		|| FindBudgetProfile(Fdemo_mapRewardBudgetProfileIds::ContainerHighValue)->BaseValue != 2000)
	{
		if (OutError) *OutError = TEXT("Reward Budget Profile identities drifted from the P1 contract.");
		return false;
	}

	TSet<FName> EntryIds;
	for (const Fdemo_mapRewardPoolEntry& Entry : PoolEntries)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Entry.DefinitionId);
		if (!Entry.IsValid()
			|| EntryIds.Contains(Entry.EntryId)
			|| !Definition
			|| Definition->SellPrice <= 0
			|| Entry.MaxStack > Definition->MaxStackSize)
		{
			if (OutError) *OutError = FString::Printf(
				TEXT("Reward Pool entry is invalid: %s"),
				*Entry.EntryId.ToString());
			return false;
		}
		EntryIds.Add(Entry.EntryId);
	}
	if (PoolEntries.IsEmpty())
	{
		if (OutError) *OutError = TEXT("Reward Pool must not be empty.");
		return false;
	}

	TSet<FName> SourceIds;
	for (const Fdemo_mapRewardSourceDefinition& Source : Sources)
	{
		if (!Source.IsValid()
			|| SourceIds.Contains(Source.RewardSourceId)
			|| (Source.Mode == Edemo_mapRewardSourceMode::FixedTable
				&& !Fdemo_mapFixedLootTableRegistry::Find(Source.FixedTableId))
			|| (Source.Mode == Edemo_mapRewardSourceMode::GeneratedReward
				&& (!FindBudgetProfile(Source.BudgetProfileId)
					|| (Source.bAllowFixedFallbackOnFailure
						&& !Fdemo_mapFixedLootTableRegistry::Find(
							Source.FixedFallbackTableId)))))
		{
			if (OutError) *OutError = TEXT("Reward Source registry is invalid or duplicated.");
			return false;
		}
		SourceIds.Add(Source.RewardSourceId);
	}
	if (Sources.Num() != 3
		|| Sources[0].Mode != Edemo_mapRewardSourceMode::FixedTable
		|| Sources[1].Mode != Edemo_mapRewardSourceMode::FixedTable
		|| Sources[2].Mode != Edemo_mapRewardSourceMode::GeneratedReward
		|| Sources[2].FixedFallbackTableId != Fdemo_mapFixedLootTableIds::ChestSideA)
	{
		if (OutError) *OutError = TEXT("Exactly the P7 side Chest must use GeneratedReward.");
		return false;
	}
	return true;
}
