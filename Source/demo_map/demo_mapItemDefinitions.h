#pragma once

#include "CoreMinimal.h"
#include "demo_mapFixedLootTableTypes.h"
#include "demo_mapItemTypes.h"
#include "demo_mapRewardGenerationTypes.h"
#include "demo_mapRewardSourceProjection.h"

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
	/** First 0.0.10 defense content whose mitigation consumes durable authority. */
	static const FName SpiritGuardRobe;
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
	/** First condition-specific treatment consumable in the 0.0.10 product loop. */
	static const FName MeridianStabilizingPillLevel1;
	/** First real product item accepted by the 0.0.10 thrown-weapon pipeline. */
	static const FName TrainingThrowingKnife;
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

/**
 * P73.3's typed manifest-owned map reward policy.  Map distributions retain
 * only stable slot identity, placement, and encounter linkage; every input
 * that changes planning or fallback is frozen here with the manifest identity.
 */
struct Fdemo_mapRewardDistributionProfile
{
	FName ProfileId = NAME_None;
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
	FName ProjectionId = NAME_None;
	FName BudgetProfileId = NAME_None;
	TArray<FName> SourceTags;
	int64 BaseSourceValue = 0;
	bool bAllowFixedFallbackOnFailure = false;

	bool IsValid() const;
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
	/** P73's one authoritative Code B content manifest. */
	static FName GetContentVersionId();
	static const FString& GetContentDigest();
	static bool IsCurrentContentIdentity(
		FName ContentVersionId,
		const FString& ContentDigest);

	static const TArray<Fdemo_mapItemDefinition>& GetAll();
	static const Fdemo_mapItemDefinition* Find(FName DefinitionId);
	static const TArray<FName>& GetEquipmentSlotIds();
	static const TArray<FName>& GetPurchasableDefinitionIds();
	/** Primary P73 fixed/container loot profiles; the P7 facade delegates here. */
	static const TArray<Fdemo_mapFixedLootTableDefinition>& GetFixedLootProfiles();
	static const Fdemo_mapFixedLootTableDefinition* FindFixedLootProfile(FName ProfileId);
	/** Typed enemy source profile, retained separately from container topology. */
	static FName GetEnemyLootProfileId(Edemo_mapEnemyLootArchetype Archetype);
	static const TArray<Fdemo_mapLootTableEntry>* FindEnemyLootProfile(FName ProfileId);
	/** P73.1 generated-reward configuration; all new planning reads this manifest. */
	static const TArray<Fdemo_mapRewardBudgetProfile>& GetGeneratedRewardBudgetProfiles();
	static const TArray<Fdemo_mapRewardBudgetProfile>& GetGeneratedRewardM01BudgetProfiles();
	static const Fdemo_mapRewardBudgetProfile* FindGeneratedRewardBudgetProfile(FName ProfileId);
	static const TArray<Fdemo_mapRewardPoolEntry>& GetGeneratedRewardPool();
	/** P73.2 canonical generated-source policy profiles; the old registry only delegates. */
	static const TArray<Fdemo_mapRewardSourceProjection>& GetGeneratedRewardProjectionProfiles();
	static const Fdemo_mapRewardSourceProjection* FindGeneratedRewardProjectionProfile(FName ProjectionId);
	/** P73.3 map distribution policy; M01/P8 only resolve profiles from here. */
	static const TArray<Fdemo_mapRewardDistributionProfile>& GetGeneratedRewardDistributionProfiles();
	static const Fdemo_mapRewardDistributionProfile* FindGeneratedRewardDistributionProfile(FName ProfileId);
	/** Current identity accepts new generation; known historical identities are read-only receipt evidence. */
	static bool IsKnownContentIdentity(
		FName ContentVersionId,
		const FString& ContentDigest);
	static Fdemo_mapSpatialStorageCapacityResult ResolveSpatialStorageCapacity(
		FName BackpackDefinitionId);
	static Fdemo_mapSpatialRingCapacityResult ResolveSpatialRingCapacity(
		FName SpatialRingDefinitionId);
	static Fdemo_mapInventoryCapacityResult ResolveInventoryCapacity(
		FName BackpackDefinitionId,
		FName SpatialRingDefinitionId = NAME_None);
	static bool Validate(FString* OutError = nullptr);
};
