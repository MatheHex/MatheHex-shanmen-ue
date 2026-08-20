#pragma once

#include "CoreMinimal.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapRewardSourceProjection.h"

enum class Edemo_mapFullMapRewardSlotKind : uint8
{
	Enemy,
	Container
};

enum class Edemo_mapFullMapRewardClass : uint8
{
	EnemyStandard,
	EnemyElite,
	Boss,
	ContainerBasicWood,
	ContainerBasicOre,
	ContainerHighValue
};

/** One immutable, array-index-independent P8 reward-source declaration. */
struct Fdemo_mapFullMapRewardSlot
{
	FName SlotId = NAME_None;
	FName StableSourceRoleId = NAME_None;
	FName MarkerId = NAME_None;
	FName RouteId = NAME_None;
	FName AreaId = NAME_None;
	/** Stable key into Fdemo_mapItemDefinitions' P73.3 distribution manifest. */
	FName DistributionProfileId = NAME_None;
	/** Read-only manifest projection retained for diagnostics and historical tests; never a planning input. */
	FName ProjectionId = NAME_None;
	FName BudgetProfileId = NAME_None;
	TArray<FName> SourceTags;
	int64 BaseSourceValue = 0;
	Edemo_mapFullMapRewardSlotKind Kind =
		Edemo_mapFullMapRewardSlotKind::Container;
	Edemo_mapFullMapRewardClass RewardClass =
		Edemo_mapFullMapRewardClass::ContainerBasicWood;
	FName SourceMarkerType = NAME_None;
	int32 SourceMarkerIndex = INDEX_NONE;
	FVector LocalOffset = FVector::ZeroVector;
	int32 ContainerOrdinal = INDEX_NONE;
	Fdemo_mapEnemyEncounterSpawnRecord EnemyRecord;

	bool IsEnemy() const
	{
		return Kind == Edemo_mapFullMapRewardSlotKind::Enemy;
	}

	bool IsContainer() const
	{
		return Kind == Edemo_mapFullMapRewardSlotKind::Container;
	}

	bool IsValid() const;
};

struct Fdemo_mapFullMapDistributionCounts
{
	int32 StandardEnemies = 0;
	int32 EliteEnemies = 0;
	int32 Bosses = 0;
	int32 BasicWoodContainers = 0;
	int32 BasicOreContainers = 0;
	int32 HighValueContainers = 0;
	int64 BaseSourceValue = 0;

	int32 EnemyTotal() const
	{
		return StandardEnemies + EliteEnemies + Bosses;
	}

	int32 BasicContainerTotal() const
	{
		return BasicWoodContainers + BasicOreContainers;
	}

	int32 ContainerTotal() const
	{
		return BasicContainerTotal() + HighValueContainers;
	}

	int32 SlotTotal() const
	{
		return EnemyTotal() + ContainerTotal();
	}
};

/**
 * The single P8 authority for default-V3 reward-source counts, identities,
 * classes, projections, budget profiles, tags, base values, and placement
 * declarations.
 */
struct Fdemo_mapRewardFullMapDistribution
{
	static constexpr int32 StandardEnemyCount = 10;
	static constexpr int32 EliteEnemyCount = 3;
	static constexpr int32 BossCount = 1;
	static constexpr int32 BasicContainerCount = 120;
	static constexpr int32 HighValueContainerCount = 15;
	static constexpr int32 TotalEnemyCount = 14;
	static constexpr int32 TotalContainerCount = 135;
	static constexpr int32 TotalSlotCount = 149;
	static constexpr int64 TotalBaseSourceValue = 115500;

	static const TArray<Fdemo_mapFullMapRewardSlot>& GetSlots();
	static const Fdemo_mapFullMapRewardSlot* Find(FName SlotId);
	static const Fdemo_mapFullMapRewardSlot* FindEnemyByEncounterId(
		FName EncounterId);
	static Fdemo_mapRewardSourceProjection BuildProjection(
		const Fdemo_mapFullMapRewardSlot& Slot);
	static Fdemo_mapFullMapDistributionCounts Count();
	static bool Validate(FString* OutError = nullptr);
};
