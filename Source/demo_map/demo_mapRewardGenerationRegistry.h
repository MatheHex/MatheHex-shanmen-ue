#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardGenerationTypes.h"

struct Fdemo_mapRewardBudgetProfileIds
{
	static const FName EnemyStandard;
	static const FName EnemyElite;
	static const FName ContainerHighValue;
	static const FName Boss;
	static const FName ContainerBasic;
	static const FName M01EnemyLow;
	static const FName M01EnemyMid;
	static const FName M01ResourceTier1;
	static const FName M01ResourceTier2;
	static const FName M01ResourceTier3;
};

struct Fdemo_mapRewardTagIds
{
	static const FName SourceContainerGeneral;
	static const FName SourceContainerHighValue;
	static const FName ItemEquipmentWeapon;
	static const FName ItemEquipmentRobe;
	static const FName ItemEquipmentAccessory;
	static const FName ItemEquipmentBackpack;
	static const FName ItemConsumablePill;
	static const FName ItemMaterialWood;
	static const FName ItemMaterialOre;
	static const FName ItemBodyBone;
	static const FName ItemBodyInnerCore;
};

struct Fdemo_mapRewardSourceIds
{
	static const FName ChestMainA;
	static const FName ChestMainB;
	static const FName ChestSideHighValue;
};

/**
 * Stable P1 lookup API. P73.1 moved generated budget and candidate data into
 * Fdemo_mapItemDefinitions; this surface now only preserves existing callers.
 */
struct Fdemo_mapRewardGenerationRegistry
{
	static constexpr int32 NormalMultiplierMinBps = 8000;
	static constexpr int32 NormalMultiplierMaxBps = 12000;

	static const TArray<Fdemo_mapRewardBudgetProfile>& GetBudgetProfiles();
	/** M01-only profiles; kept separate so the original five-profile P1 contract remains stable. */
	static const TArray<Fdemo_mapRewardBudgetProfile>& GetM01BudgetProfiles();
	static const Fdemo_mapRewardBudgetProfile* FindBudgetProfile(FName ProfileId);
	static const TArray<Fdemo_mapRewardPoolEntry>& GetHighValueContainerPool();
	static const TArray<Fdemo_mapRewardSourceDefinition>& GetChestSources();
	static const Fdemo_mapRewardSourceDefinition* FindChestSource(int32 ChestIndex);
	static bool Validate(FString* OutError = nullptr);
};
