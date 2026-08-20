#include "demo_mapM01RewardDistribution.h"

#include "demo_mapItemDefinitions.h"

namespace
{
	const FName AnchorTier1(TEXT("M01.Resource.TIER_1.Cluster.01"));
	const FName AnchorTier2(TEXT("M01.Resource.TIER_2.Cluster.01"));
	const FName AnchorTier3(TEXT("M01.Resource.TIER_3.Cluster.01"));
	const FName AnchorBoss(TEXT("M01.Boss.Main"));
	const FName RouteLow(TEXT("M01.Route.Low.North"));
	const FName RouteMid(TEXT("M01.Route.Mid.Loop.North"));
	const FName RouteHigh(TEXT("M01.Route.High.Core"));

	FName StableName(const TCHAR* Prefix, int32 Number)
	{
		return FName(*FString::Printf(TEXT("%s.%03d"), Prefix, Number));
	}

	FVector GridOffset(int32 ZeroBased, int32 Columns, int32 Rows, float Spacing)
	{
		const int32 Row = ZeroBased / Columns;
		const int32 Column = ZeroBased % Columns;
		return FVector(
			(static_cast<float>(Column) - (Columns - 1) * 0.5f) * Spacing,
			(static_cast<float>(Row) - (Rows - 1) * 0.5f) * Spacing,
			0.0f);
	}

	FName DistributionProfileForEnemy(
		const Fdemo_mapM01EnemyDefinition& Enemy)
	{
		if (Enemy.IsBoss()) return TEXT("M01.Distribution.Enemy.Boss");
		if (Enemy.IsElite()) return TEXT("M01.Distribution.Enemy.Elite");
		const TCHAR* Risk = Enemy.RiskTierId
			== Fdemo_mapRewardProjectionTagIds::RiskLow ? TEXT("Low") : TEXT("Mid");
		const TCHAR* Archetype = Enemy.Archetype
			== Edemo_mapM01EnemyArchetype::StandardRanged ? TEXT("Ranged")
			: Enemy.Archetype == Edemo_mapM01EnemyArchetype::StandardBruiser
				? TEXT("Bruiser") : TEXT("Melee");
		return FName(*FString::Printf(
			TEXT("M01.Distribution.Enemy.%s.%s"), Risk, Archetype));
	}

	Edemo_mapM01RewardSourceClass EnemyClass(
		const Fdemo_mapM01EnemyDefinition& Enemy)
	{
		return Enemy.IsBoss() ? Edemo_mapM01RewardSourceClass::Boss
			: Enemy.IsElite() ? Edemo_mapM01RewardSourceClass::EnemyElite
			: Enemy.RiskTierId == Fdemo_mapRewardProjectionTagIds::RiskLow
				? Edemo_mapM01RewardSourceClass::EnemyLow
				: Edemo_mapM01RewardSourceClass::EnemyMid;
	}

	FName TierForClass(Edemo_mapM01RewardSourceClass RewardClass)
	{
		switch (RewardClass)
		{
		case Edemo_mapM01RewardSourceClass::EnemyLow:
		case Edemo_mapM01RewardSourceClass::ResourceWoodTier1:
		case Edemo_mapM01RewardSourceClass::ResourceOreTier1:
			return Fdemo_mapRewardProjectionTagIds::Tier1;
		case Edemo_mapM01RewardSourceClass::EnemyMid:
		case Edemo_mapM01RewardSourceClass::ResourceWoodTier2:
		case Edemo_mapM01RewardSourceClass::ResourceOreTier2:
			return Fdemo_mapRewardProjectionTagIds::Tier2;
		default:
			return Fdemo_mapRewardProjectionTagIds::Tier3;
		}
	}

	void BuildEnemySlots(TArray<Fdemo_mapM01RewardSlot>& Slots)
	{
		for (const Fdemo_mapM01EnemyDefinition& Enemy :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			Fdemo_mapM01RewardSlot Slot;
			Slot.SlotId = FName(*FString::Printf(
				TEXT("M01.Reward.Slot.Enemy.%s"), *Enemy.EncounterId.ToString()));
			Slot.StableSourceRoleId = FName(*FString::Printf(
				TEXT("M01.Reward.Source.Enemy.%s"), *Enemy.EncounterId.ToString()));
			Slot.MarkerId = Enemy.SpawnMarkerId;
			Slot.AnchorMarkerId = Enemy.ParentMarkerId;
			Slot.RouteId = Enemy.RouteId;
			Slot.AreaId = Enemy.ParentMarkerId;
			Slot.DistributionProfileId = DistributionProfileForEnemy(Enemy);
			Slot.RewardClass = EnemyClass(Enemy);
			Slot.LocalOffset = Enemy.LocalOffset;
			Slot.EncounterId = Enemy.EncounterId;
			Slot.CorpseIdentity = Enemy.CorpseIdentity;
			Slots.Add(MoveTemp(Slot));
		}
	}

	void AddResourceSlot(
		TArray<Fdemo_mapM01RewardSlot>& Slots,
		bool bWood,
		int32 Tier,
		int32 ClassOrdinal,
		int32 CombinedOrdinal,
		int32& ContainerOrdinal)
	{
		const bool bTier1 = Tier == 1;
		const bool bTier2 = Tier == 2;
		const Edemo_mapM01RewardSourceClass RewardClass = bWood
			? bTier1 ? Edemo_mapM01RewardSourceClass::ResourceWoodTier1
				: bTier2 ? Edemo_mapM01RewardSourceClass::ResourceWoodTier2
					: Edemo_mapM01RewardSourceClass::ResourceWoodTier3
			: bTier1 ? Edemo_mapM01RewardSourceClass::ResourceOreTier1
				: bTier2 ? Edemo_mapM01RewardSourceClass::ResourceOreTier2
					: Edemo_mapM01RewardSourceClass::ResourceOreTier3;
		const FName Anchor = bTier1 ? AnchorTier1 : bTier2 ? AnchorTier2 : AnchorTier3;
		const FName Route = bTier1 ? RouteLow : bTier2 ? RouteMid : RouteHigh;
		const TCHAR* Material = bWood ? TEXT("Wood") : TEXT("Ore");
		Fdemo_mapM01RewardSlot Slot;
		Slot.SlotId = StableName(
			*FString::Printf(TEXT("M01.Reward.Slot.Resource.TIER_%d.%s"), Tier, Material),
			ClassOrdinal);
		Slot.StableSourceRoleId = StableName(
			*FString::Printf(TEXT("M01.Reward.Source.Resource.TIER_%d.%s"), Tier, Material),
			ClassOrdinal);
		Slot.MarkerId = StableName(
			*FString::Printf(TEXT("M01.Reward.Marker.Resource.TIER_%d.%s"), Tier, Material),
			ClassOrdinal);
		Slot.AnchorMarkerId = Anchor;
		Slot.RouteId = Route;
		Slot.AreaId = Anchor;
		Slot.DistributionProfileId = FName(*FString::Printf(
			TEXT("M01.Distribution.Resource.%s.Tier%d"), Material, Tier));
		Slot.RewardClass = RewardClass;
		Slot.LocalOffset = Tier < 3
			? GridOffset(CombinedOrdinal, 8, 6, 240.0f)
			: GridOffset(CombinedOrdinal, 6, 4, 240.0f);
		Slot.ContainerOrdinal = ContainerOrdinal++;
		Slots.Add(MoveTemp(Slot));
	}

	void BuildContainerSlots(TArray<Fdemo_mapM01RewardSlot>& Slots)
	{
		int32 ContainerOrdinal = 0;
		for (int32 Tier = 1; Tier <= 3; ++Tier)
		{
			const int32 PerMaterial = Tier < 3 ? 24 : 12;
			for (int32 Number = 0; Number < PerMaterial; ++Number)
			{
				AddResourceSlot(Slots, true, Tier, Number + 1, Number * 2, ContainerOrdinal);
				AddResourceSlot(Slots, false, Tier, Number + 1, Number * 2 + 1, ContainerOrdinal);
			}
		}

		for (int32 Number = 0; Number < 15; ++Number)
		{
			const bool bEliteArea = Number < 5;
			Fdemo_mapM01RewardSlot Slot;
			Slot.SlotId = StableName(TEXT("M01.Reward.Slot.HighValue"), Number + 1);
			Slot.StableSourceRoleId = StableName(TEXT("M01.Reward.Source.HighValue"), Number + 1);
			Slot.MarkerId = StableName(TEXT("M01.Reward.Marker.HighValue"), Number + 1);
			Slot.AnchorMarkerId = bEliteArea ? AnchorTier3 : AnchorBoss;
			Slot.RouteId = RouteHigh;
			Slot.AreaId = bEliteArea ? AnchorTier3 : AnchorBoss;
			Slot.DistributionProfileId = TEXT("M01.Distribution.HighValue");
			Slot.RewardClass = Edemo_mapM01RewardSourceClass::HighValue;
			Slot.LocalOffset = bEliteArea
				? FVector((Number - 2) * 300.0f, 1050.0f, 0.0f)
				: GridOffset(Number - 5, 5, 2, 300.0f) + FVector(0.0f, 650.0f, 0.0f);
			Slot.ContainerOrdinal = ContainerOrdinal++;
			Slots.Add(MoveTemp(Slot));
		}
	}
}

bool Fdemo_mapM01RewardSlot::IsValid() const
{
	return !SlotId.IsNone() && !StableSourceRoleId.IsNone()
		&& !MarkerId.IsNone() && !AnchorMarkerId.IsNone()
		&& !RouteId.IsNone() && !AreaId.IsNone()
		&& !DistributionProfileId.IsNone()
		&& !ProjectionId.IsNone() && !BudgetProfileId.IsNone()
		&& !SourceTags.IsEmpty() && !SourceTags.Contains(NAME_None)
		&& BaseSourceValue > 0
		&& FMath::IsFinite(LocalOffset.X) && FMath::IsFinite(LocalOffset.Y)
		&& FMath::IsFinite(LocalOffset.Z) && LocalOffset.GetAbsMax() <= 10000.0f
		&& (IsEnemy()
			? !EncounterId.IsNone() && !CorpseIdentity.IsNone()
			: EncounterId.IsNone() && CorpseIdentity.IsNone());
}

const TArray<Fdemo_mapM01RewardSlot>& Fdemo_mapM01RewardDistribution::GetSlots()
{
	static const TArray<Fdemo_mapM01RewardSlot> Slots = []()
	{
		TArray<Fdemo_mapM01RewardSlot> Result;
		Result.Reserve(TotalSlotCount);
		BuildEnemySlots(Result);
		BuildContainerSlots(Result);
		for (Fdemo_mapM01RewardSlot& Slot : Result)
		{
			const Fdemo_mapRewardDistributionProfile* Distribution =
				Fdemo_mapItemDefinitions::FindGeneratedRewardDistributionProfile(
					Slot.DistributionProfileId);
			if (!Distribution)
			{
				continue;
			}

			// Compatibility projection only: planning resolves the manifest again in BuildProjection.
			Slot.ProjectionId = Distribution->ProjectionId;
			Slot.BudgetProfileId = Distribution->BudgetProfileId;
			Slot.SourceTags = Distribution->SourceTags;
			Slot.BaseSourceValue = Distribution->BaseSourceValue;
		}
		return Result;
	}();
	return Slots;
}

const Fdemo_mapM01RewardSlot* Fdemo_mapM01RewardDistribution::Find(FName SlotId)
{
	return GetSlots().FindByPredicate([SlotId](const auto& Slot)
	{
		return Slot.SlotId == SlotId;
	});
}

const Fdemo_mapM01RewardSlot*
Fdemo_mapM01RewardDistribution::FindEnemyByEncounterId(FName EncounterId)
{
	return GetSlots().FindByPredicate([EncounterId](const auto& Slot)
	{
		return Slot.IsEnemy() && Slot.EncounterId == EncounterId;
	});
}

Fdemo_mapRewardSourceProjection Fdemo_mapM01RewardDistribution::BuildProjection(
	const Fdemo_mapM01RewardSlot& Slot)
{
	const Fdemo_mapRewardDistributionProfile* Distribution =
		Fdemo_mapItemDefinitions::FindGeneratedRewardDistributionProfile(
			Slot.DistributionProfileId);
	const Fdemo_mapRewardSourceProjection* Prototype = Distribution
		? Fdemo_mapItemDefinitions::FindGeneratedRewardProjectionProfile(
			Distribution->ProjectionId) : nullptr;
	if (!Distribution || !Distribution->IsValid() || !Prototype)
		return Fdemo_mapRewardSourceProjection();
	Fdemo_mapRewardSourceProjection Result = *Prototype;
	Result.DistributionProfileId = Slot.DistributionProfileId;
	Result.SlotId = Slot.SlotId;
	Result.StableSourceRoleId = Slot.StableSourceRoleId;
	Result.MarkerId = Slot.MarkerId;
	Result.EncounterId = Slot.IsEnemy() ? Slot.EncounterId : NAME_None;
	Result.BudgetProfileId = Distribution->BudgetProfileId;
	Result.SourceTags = Distribution->SourceTags;
	Result.BaseSourceValue = Distribution->BaseSourceValue;
	Result.bAllowFixedFallbackOnFailure =
		Distribution->bAllowFixedFallbackOnFailure;
	Result.SourceDisplayLabel = FString::Printf(
		TEXT("M01 %s\n%s"),
		Slot.RewardClass == Edemo_mapM01RewardSourceClass::HighValue
			? TEXT("HIGH VALUE")
			: Slot.IsEnemy() ? TEXT("CORPSE REWARD")
				: Distribution->SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::SourceContainerWood)
					? TEXT("WOOD") : TEXT("ORE"),
		*TierForClass(Slot.RewardClass).ToString());
	return Result;
}

Fdemo_mapM01RewardDistributionCounts Fdemo_mapM01RewardDistribution::Count()
{
	Fdemo_mapM01RewardDistributionCounts Result;
	for (const Fdemo_mapM01RewardSlot& Slot : GetSlots())
	{
		switch (Slot.RewardClass)
		{
		case Edemo_mapM01RewardSourceClass::EnemyLow: ++Result.LowEnemies; break;
		case Edemo_mapM01RewardSourceClass::EnemyMid: ++Result.MidEnemies; break;
		case Edemo_mapM01RewardSourceClass::EnemyElite: ++Result.EliteEnemies; break;
		case Edemo_mapM01RewardSourceClass::Boss: ++Result.Bosses; break;
		case Edemo_mapM01RewardSourceClass::ResourceWoodTier1: ++Result.Tier1Wood; break;
		case Edemo_mapM01RewardSourceClass::ResourceOreTier1: ++Result.Tier1Ore; break;
		case Edemo_mapM01RewardSourceClass::ResourceWoodTier2: ++Result.Tier2Wood; break;
		case Edemo_mapM01RewardSourceClass::ResourceOreTier2: ++Result.Tier2Ore; break;
		case Edemo_mapM01RewardSourceClass::ResourceWoodTier3: ++Result.Tier3Wood; break;
		case Edemo_mapM01RewardSourceClass::ResourceOreTier3: ++Result.Tier3Ore; break;
		case Edemo_mapM01RewardSourceClass::HighValue: ++Result.HighValue; break;
		}
		const Fdemo_mapRewardDistributionProfile* Distribution =
			Fdemo_mapItemDefinitions::FindGeneratedRewardDistributionProfile(
				Slot.DistributionProfileId);
		Result.BaseSourceValue += Distribution ? Distribution->BaseSourceValue : 0;
	}
	return Result;
}

bool Fdemo_mapM01RewardDistribution::Validate(FString* OutError)
{
	FString DependencyError;
	if (!Fdemo_mapItemDefinitions::Validate(&DependencyError)
		|| !Fdemo_mapM01EnemyConfig::Validate(&DependencyError))
	{
		if (OutError) *OutError = TEXT("dependency_validation_failed:") + DependencyError;
		return false;
	}
	TSet<FName> SlotIds;
	TSet<FName> Roles;
	TSet<FName> Markers;
	TSet<int32> ContainerOrdinals;
	TSet<FName> EnemyEncounters;
	for (const Fdemo_mapM01RewardSlot& Slot : GetSlots())
	{
		const Fdemo_mapRewardDistributionProfile* Distribution =
			Fdemo_mapItemDefinitions::FindGeneratedRewardDistributionProfile(
				Slot.DistributionProfileId);
		const Fdemo_mapRewardSourceProjection Projection = BuildProjection(Slot);
		if (!Slot.IsValid() || !Distribution || !Distribution->IsValid()
			|| Slot.ProjectionId != Distribution->ProjectionId
			|| Slot.BudgetProfileId != Distribution->BudgetProfileId
			|| Slot.SourceTags != Distribution->SourceTags
			|| Slot.BaseSourceValue != Distribution->BaseSourceValue
			|| !Projection.IsValid() || Projection.bAllowFixedFallbackOnFailure
			|| SlotIds.Contains(Slot.SlotId)
			|| Roles.Contains(Slot.StableSourceRoleId)
			|| Markers.Contains(Slot.MarkerId)
			|| (Slot.IsContainer() && ContainerOrdinals.Contains(Slot.ContainerOrdinal))
			|| (Slot.IsEnemy() && EnemyEncounters.Contains(Slot.EncounterId)))
		{
			if (OutError) *OutError = FString::Printf(
				TEXT("invalid_or_duplicate_m01_reward_slot:%s"), *Slot.SlotId.ToString());
			return false;
		}
		SlotIds.Add(Slot.SlotId);
		Roles.Add(Slot.StableSourceRoleId);
		Markers.Add(Slot.MarkerId);
		if (Slot.IsContainer()) ContainerOrdinals.Add(Slot.ContainerOrdinal);
		else EnemyEncounters.Add(Slot.EncounterId);
	}
	const Fdemo_mapM01RewardDistributionCounts Counts = Count();
	const bool bExact = GetSlots().Num() == TotalSlotCount
		&& Counts.LowEnemies == LowEnemyCount
		&& Counts.MidEnemies == MidEnemyCount
		&& Counts.EliteEnemies == EliteEnemyCount
		&& Counts.Bosses == BossCount
		&& Counts.EnemyTotal() == TotalEnemyCount
		&& Counts.Tier1Wood == 24 && Counts.Tier1Ore == 24
		&& Counts.Tier2Wood == 24 && Counts.Tier2Ore == 24
		&& Counts.Tier3Wood == 12 && Counts.Tier3Ore == 12
		&& Counts.ResourceTotal() == 120
		&& Counts.HighValue == HighValueCount
		&& Counts.ContainerTotal() == TotalContainerCount
		&& Counts.BaseSourceValue == TotalBaseSourceValue;
	if (!bExact && OutError) *OutError = TEXT("M01 reward distribution count or value contract drifted.");
	return bExact;
}
