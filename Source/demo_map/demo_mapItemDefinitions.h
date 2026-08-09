#pragma once

#include "CoreMinimal.h"
#include "demo_mapItemTypes.h"

struct Fdemo_mapItemIds
{
	static const FName InventoryContainer;
	static const FName EquipmentContainer;
	static const FName WorldContainer;
	static const FName SessionStashContainer;
	static const FName LocalPlayerOwner;

	static const FName WeaponSlot;
	static const FName ArmorSlot;
	static const FName AccessorySlot;
	/** A spatial ring is independent from an ordinary accessory. */
	static const FName SpatialRingSlot;
	static const FName BackpackSlot;

	static const FName WeaponCategory;
	static const FName ArmorCategory;
	static const FName AccessoryCategory;
	static const FName SpatialRingCategory;
	static const FName BackpackCategory;
	static const FName MaterialCategory;
	static const FName ConsumableCategory;
	static const FName CoreCategory;
	static const FName LootCategory;

	static const FName TrainingBlade;
	static const FName TrainingVest;
	static const FName WindTalisman;
	static const FName SpiritDust;
	static const FName HeavyPracticeBlade;
	static const FName ReinforcedVest;
	static const FName EvasionCharm;
	static const FName IronShard;
	static const FName AncientToken;

	static const FName WeaponLevel1;
	static const FName WeaponLevel2;
	static const FName WeaponLevel3;
	static const FName WeaponLevel4;
	static const FName ArmorRobeLevel1;
	static const FName ArmorRobeLevel2;
	static const FName ArmorRobeLevel3;
	static const FName ArmorRobeLevel4;
	static const FName AccessoryLevel1;
	static const FName AccessoryLevel2;
	static const FName AccessoryLevel3;
	static const FName AccessoryLevel4;
	static const FName BackpackLevel1;
	static const FName BackpackLevel2;
	static const FName SpiritWoodLevel1;
	static const FName SpiritWoodLevel2;
	static const FName SpiritWoodLevel3;
	static const FName SpiritOreLevel1;
	static const FName SpiritOreLevel2;
	static const FName SpiritOreLevel3;
	static const FName HealingPillLevel1;
	static const FName HealingPillLevel2;
	static const FName HealingPillLevel3;
	static const FName SoulBone;
	static const FName SpiritBone;
	static const FName DaoBone;
	static const FName InnerCoreLevel5;
	static const FName InnerCoreLevel10;
	static const FName InnerCoreLevel15;
};

struct Fdemo_mapItemEffectIds
{
	static const FName AttackBonus;
	static const FName MaxHealthBonus;
	static const FName FlatDamageReduction;
	static const FName CooldownMultiplier;
	static const FName TotalCapacity;
	static const FName RingQuickCapacity;
	static const FName HealAmount;
};

/** P1.0 only defines these integer contracts; no source, sink, or transfer is activated. */
struct Fdemo_mapSpiritStoneRules
{
	static constexpr int64 FixedWorldPickupValueForFutureTasks = 20;
	static constexpr int64 FixedWorldPickupValue = FixedWorldPickupValueForFutureTasks;
	static constexpr int32 BaseInventoryCapacityWithoutBackpack = 6;
};

struct Fdemo_mapLootTableIds
{
	static const FName EnemyMelee;
	static const FName EnemyRanged;
	static const FName EnemyHeavy;
};

struct Fdemo_mapLootTableEntry
{
	FName DefinitionId = NAME_None;
	int32 Quantity = 0;
};

struct Fdemo_mapLootTables
{
	static FName GetTableId(Edemo_mapEnemyLootArchetype Archetype);
	static const TArray<Fdemo_mapLootTableEntry>* Find(FName TableId);
	static bool Validate(FString* OutError = nullptr);
};

/** Central immutable prototype registry. Runtime state stores only DefinitionId. */
struct Fdemo_mapItemDefinitions
{
	static const TArray<Fdemo_mapItemDefinition>& GetAll();
	static const Fdemo_mapItemDefinition* Find(FName DefinitionId);
	static const TArray<FName>& GetEquipmentSlotIds();
	static const TArray<FName>& GetPurchasableDefinitionIds();
	static Fdemo_mapSpatialStorageCapacityResult ResolveSpatialStorageCapacity(
		FName BackpackDefinitionId);
	static Fdemo_mapSpatialRingCapacityResult ResolveSpatialRingCapacity(
		FName SpatialRingDefinitionId);
	static Fdemo_mapInventoryCapacityResult ResolveInventoryCapacity(
		FName BackpackDefinitionId,
		FName SpatialRingDefinitionId = NAME_None);
	static bool Validate(FString* OutError = nullptr);
};
