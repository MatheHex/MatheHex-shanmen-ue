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
	return Fdemo_mapItemDefinitions::GetGeneratedRewardBudgetProfiles();
}

const TArray<Fdemo_mapRewardBudgetProfile>&
Fdemo_mapRewardGenerationRegistry::GetM01BudgetProfiles()
{
	return Fdemo_mapItemDefinitions::GetGeneratedRewardM01BudgetProfiles();
}

const Fdemo_mapRewardBudgetProfile*
Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(FName ProfileId)
{
	return Fdemo_mapItemDefinitions::FindGeneratedRewardBudgetProfile(ProfileId);
}

const TArray<Fdemo_mapRewardPoolEntry>&
Fdemo_mapRewardGenerationRegistry::GetHighValueContainerPool()
{
	return Fdemo_mapItemDefinitions::GetGeneratedRewardPool();
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
				&& !Fdemo_mapItemDefinitions::FindFixedLootProfile(Source.FixedTableId))
			|| (Source.Mode == Edemo_mapRewardSourceMode::GeneratedReward
				&& (!FindBudgetProfile(Source.BudgetProfileId)
					|| (Source.bAllowFixedFallbackOnFailure
						&& !Fdemo_mapItemDefinitions::FindFixedLootProfile(
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
