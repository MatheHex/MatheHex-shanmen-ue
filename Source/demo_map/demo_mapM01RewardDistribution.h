#pragma once

#include "CoreMinimal.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapRewardSourceProjection.h"

enum class Edemo_mapM01RewardSourceClass : uint8
{
	EnemyLow,
	EnemyMid,
	EnemyElite,
	Boss,
	ResourceWoodTier1,
	ResourceOreTier1,
	ResourceWoodTier2,
	ResourceOreTier2,
	ResourceWoodTier3,
	ResourceOreTier3,
	HighValue
};

/** Immutable, array-index-independent declaration for one M01 reward source. */
struct Fdemo_mapM01RewardSlot
{
	FName SlotId = NAME_None;
	FName StableSourceRoleId = NAME_None;
	FName MarkerId = NAME_None;
	FName AnchorMarkerId = NAME_None;
	FName RouteId = NAME_None;
	FName AreaId = NAME_None;
	FName ProjectionId = NAME_None;
	FName BudgetProfileId = NAME_None;
	TArray<FName> SourceTags;
	int64 BaseSourceValue = 0;
	Edemo_mapM01RewardSourceClass RewardClass =
		Edemo_mapM01RewardSourceClass::HighValue;
	FVector LocalOffset = FVector::ZeroVector;
	int32 ContainerOrdinal = INDEX_NONE;
	FName EncounterId = NAME_None;
	FName CorpseIdentity = NAME_None;

	bool IsEnemy() const { return ContainerOrdinal == INDEX_NONE; }
	bool IsContainer() const { return ContainerOrdinal >= 0; }
	bool IsValid() const;
};

struct Fdemo_mapM01RewardDistributionCounts
{
	int32 LowEnemies = 0;
	int32 MidEnemies = 0;
	int32 EliteEnemies = 0;
	int32 Bosses = 0;
	int32 Tier1Wood = 0;
	int32 Tier1Ore = 0;
	int32 Tier2Wood = 0;
	int32 Tier2Ore = 0;
	int32 Tier3Wood = 0;
	int32 Tier3Ore = 0;
	int32 HighValue = 0;
	int64 BaseSourceValue = 0;

	int32 EnemyTotal() const
	{
		return LowEnemies + MidEnemies + EliteEnemies + Bosses;
	}
	int32 ResourceTotal() const
	{
		return Tier1Wood + Tier1Ore + Tier2Wood + Tier2Ore
			+ Tier3Wood + Tier3Ore;
	}
	int32 ContainerTotal() const { return ResourceTotal() + HighValue; }
	int32 SlotTotal() const { return EnemyTotal() + ContainerTotal(); }
};

/** Single M01 authority for the P4 source distribution and runtime projection data. */
struct Fdemo_mapM01RewardDistribution
{
	static constexpr int32 LowEnemyCount = 4;
	static constexpr int32 MidEnemyCount = 6;
	static constexpr int32 EliteEnemyCount = 3;
	static constexpr int32 BossCount = 1;
	static constexpr int32 Tier1ResourceCount = 48;
	static constexpr int32 Tier2ResourceCount = 48;
	static constexpr int32 Tier3ResourceCount = 24;
	static constexpr int32 HighValueCount = 15;
	static constexpr int32 TotalEnemyCount = 14;
	static constexpr int32 TotalContainerCount = 135;
	static constexpr int32 TotalSlotCount = 149;
	static constexpr int64 TotalBaseSourceValue = 115500;

	static const TArray<Fdemo_mapM01RewardSlot>& GetSlots();
	static const Fdemo_mapM01RewardSlot* Find(FName SlotId);
	static const Fdemo_mapM01RewardSlot* FindEnemyByEncounterId(FName EncounterId);
	static Fdemo_mapRewardSourceProjection BuildProjection(
		const Fdemo_mapM01RewardSlot& Slot);
	static Fdemo_mapM01RewardDistributionCounts Count();
	static bool Validate(FString* OutError = nullptr);
};
