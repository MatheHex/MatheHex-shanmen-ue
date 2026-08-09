#include "demo_mapRewardFullMapDistribution.h"

#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapRewardGenerationRegistry.h"

namespace
{
	const FName AreaMain(TEXT("P8.Area.MainRoute"));
	const FName AreaSideMelee(TEXT("P8.Area.Side.Melee"));
	const FName AreaSideRanged(TEXT("P8.Area.Side.Ranged"));
	const FName AreaWood(TEXT("P8.Area.Container.Wood"));
	const FName AreaOre(TEXT("P8.Area.Container.Ore"));
	const FName AreaHighValue(TEXT("P8.Area.Container.HighValue"));

	FName Numbered(const TCHAR* Prefix, int32 Number)
	{
		return FName(*FString::Printf(TEXT("%s.%03d"), Prefix, Number));
	}

	FVector GridOffset(int32 Number, int32 Columns, float Spacing)
	{
		if (Number == 1)
		{
			return FVector::ZeroVector;
		}
		const int32 ZeroBased = Number - 2;
		const int32 Row = ZeroBased / Columns;
		const int32 Column = ZeroBased % Columns;
		const float CenterColumn = (Columns - 1) * 0.5f;
		const float CenterRow = 2.5f;
		return FVector(
			(static_cast<float>(Column) - CenterColumn) * Spacing,
			(static_cast<float>(Row) - CenterRow) * Spacing,
			0.0f);
	}

	Edemo_mapFullMapRewardClass ClassForBaseEnemy(
		const Fdemo_mapEnemyEncounterSpawnRecord& Record)
	{
		if (Record.Identity.EncounterId
			== Fdemo_mapEnemyEncounterIds::MainMeleeHeavy)
		{
			return Edemo_mapFullMapRewardClass::Boss;
		}
		return Record.bEnhanced
			? Edemo_mapFullMapRewardClass::EnemyElite
			: Edemo_mapFullMapRewardClass::EnemyStandard;
	}

	const Fdemo_mapRewardSourceProjection* ProjectionForEnemyRecord(
		const Fdemo_mapEnemyEncounterSpawnRecord& Record,
		Edemo_mapFullMapRewardClass RewardClass)
	{
		return RewardClass == Edemo_mapFullMapRewardClass::Boss
			? Fdemo_mapRewardSourceProjectionRegistry::FindBossPrototype()
			: Fdemo_mapRewardSourceProjectionRegistry::
				FindCorpseByFallbackTable(Record.Identity.LootTableId);
	}

	FName ProfileForClass(Edemo_mapFullMapRewardClass RewardClass)
	{
		switch (RewardClass)
		{
		case Edemo_mapFullMapRewardClass::EnemyStandard:
			return Fdemo_mapRewardBudgetProfileIds::EnemyStandard;
		case Edemo_mapFullMapRewardClass::EnemyElite:
			return Fdemo_mapRewardBudgetProfileIds::EnemyElite;
		case Edemo_mapFullMapRewardClass::Boss:
			return Fdemo_mapRewardBudgetProfileIds::Boss;
		case Edemo_mapFullMapRewardClass::ContainerBasicWood:
		case Edemo_mapFullMapRewardClass::ContainerBasicOre:
			return Fdemo_mapRewardBudgetProfileIds::ContainerBasic;
		case Edemo_mapFullMapRewardClass::ContainerHighValue:
			return Fdemo_mapRewardBudgetProfileIds::ContainerHighValue;
		}
		return NAME_None;
	}

	int64 BaseValueForClass(Edemo_mapFullMapRewardClass RewardClass)
	{
		switch (RewardClass)
		{
		case Edemo_mapFullMapRewardClass::EnemyStandard:
			return 1200;
		case Edemo_mapFullMapRewardClass::EnemyElite:
			return 4500;
		case Edemo_mapFullMapRewardClass::Boss:
			return 12000;
		case Edemo_mapFullMapRewardClass::ContainerBasicWood:
		case Edemo_mapFullMapRewardClass::ContainerBasicOre:
			return 400;
		case Edemo_mapFullMapRewardClass::ContainerHighValue:
			return 2000;
		}
		return 0;
	}

	FName AreaForEnemy(const Fdemo_mapEnemyEncounterSpawnRecord& Record)
	{
		if (Record.Identity.RouteId
			== Fdemo_mapEnemyEncounterIds::SideMeleeRoute)
		{
			return AreaSideMelee;
		}
		if (Record.Identity.RouteId
			== Fdemo_mapEnemyEncounterIds::SideRangedRoute)
		{
			return AreaSideRanged;
		}
		return AreaMain;
	}

	Fdemo_mapFullMapRewardSlot EnemySlot(
		int32 ClassOrdinal,
		Edemo_mapFullMapRewardClass RewardClass,
		const Fdemo_mapEnemyEncounterSpawnRecord& Record)
	{
		const TCHAR* ClassName =
			RewardClass == Edemo_mapFullMapRewardClass::EnemyStandard
				? TEXT("Standard")
				: RewardClass == Edemo_mapFullMapRewardClass::EnemyElite
					? TEXT("Elite")
					: TEXT("Boss");
		Fdemo_mapFullMapRewardSlot Slot;
		Slot.SlotId = Numbered(
			*FString::Printf(TEXT("P8.Slot.Enemy.%s"), ClassName),
			ClassOrdinal);
		Slot.StableSourceRoleId =
			RewardClass == Edemo_mapFullMapRewardClass::Boss
				? Fdemo_mapRewardSourceRoleIds::BossPrototype
				: Numbered(
					*FString::Printf(
						TEXT("P8.SourceRole.Enemy.%s"),
						ClassName),
					ClassOrdinal);
		Slot.MarkerId = Record.Identity.SpawnMarkerId;
		Slot.RouteId = Record.Identity.RouteId;
		Slot.AreaId = AreaForEnemy(Record);
		const Fdemo_mapRewardSourceProjection* Projection =
			ProjectionForEnemyRecord(Record, RewardClass);
		Slot.ProjectionId =
			Projection ? Projection->ProjectionId : NAME_None;
		Slot.BudgetProfileId = ProfileForClass(RewardClass);
		Slot.SourceTags =
			Projection ? Projection->SourceTags : TArray<FName>();
		Slot.BaseSourceValue = BaseValueForClass(RewardClass);
		Slot.Kind = Edemo_mapFullMapRewardSlotKind::Enemy;
		Slot.RewardClass = RewardClass;
		Slot.SourceMarkerType = Record.SourceMarkerType;
		Slot.SourceMarkerIndex = Record.SourceMarkerIndex;
		Slot.LocalOffset = Record.LocalOffset;
		Slot.EnemyRecord = Record;
		return Slot;
	}

	Fdemo_mapEnemyEncounterSpawnRecord CloneEnemy(
		const Fdemo_mapEnemyEncounterSpawnRecord& Source,
		Edemo_mapFullMapRewardClass RewardClass,
		int32 ClassOrdinal,
		FVector LocalOffset)
	{
		Fdemo_mapEnemyEncounterSpawnRecord Result = Source;
		const TCHAR* ClassName =
			RewardClass == Edemo_mapFullMapRewardClass::EnemyStandard
				? TEXT("Standard")
				: TEXT("Elite");
		Result.Identity.EncounterId = Numbered(
			*FString::Printf(TEXT("P8.Encounter.Enemy.%s"), ClassName),
			ClassOrdinal);
		Result.Identity.RouteId = Numbered(
			*FString::Printf(TEXT("P8.Route.Enemy.%s"), ClassName),
			ClassOrdinal);
		Result.Identity.SpawnMarkerId = Numbered(
			*FString::Printf(TEXT("P8.Marker.Enemy.%s"), ClassName),
			ClassOrdinal);
		Result.LocalOffset = LocalOffset;
		return Result;
	}

	Fdemo_mapFullMapRewardSlot ContainerSlot(
		Edemo_mapFullMapRewardClass RewardClass,
		int32 ClassOrdinal,
		int32 ContainerOrdinal,
		int32 AnchorIndex,
		FVector LocalOffset,
		const Fdemo_mapRewardSourceProjection& Projection,
		FName AreaId)
	{
		const TCHAR* ClassName =
			RewardClass == Edemo_mapFullMapRewardClass::ContainerBasicWood
				? TEXT("Basic.Wood")
				: RewardClass
					== Edemo_mapFullMapRewardClass::ContainerBasicOre
					? TEXT("Basic.Ore")
					: TEXT("HighValue");
		Fdemo_mapFullMapRewardSlot Slot;
		Slot.SlotId = Numbered(
			*FString::Printf(TEXT("P8.Slot.Container.%s"), ClassName),
			ClassOrdinal);
		Slot.StableSourceRoleId = Numbered(
			*FString::Printf(TEXT("P8.SourceRole.Container.%s"), ClassName),
			ClassOrdinal);
		Slot.MarkerId = Numbered(
			*FString::Printf(TEXT("P8.Marker.Container.%s"), ClassName),
			ClassOrdinal);
		Slot.RouteId = FName(*FString::Printf(
			TEXT("P8.Route.Container.Anchor.%d"),
			AnchorIndex));
		Slot.AreaId = AreaId;
		Slot.ProjectionId = Projection.ProjectionId;
		Slot.BudgetProfileId = ProfileForClass(RewardClass);
		Slot.SourceTags = Projection.SourceTags;
		Slot.BaseSourceValue = BaseValueForClass(RewardClass);
		Slot.Kind = Edemo_mapFullMapRewardSlotKind::Container;
		Slot.RewardClass = RewardClass;
		Slot.SourceMarkerType = TEXT("Chest");
		Slot.SourceMarkerIndex = AnchorIndex;
		Slot.LocalOffset = LocalOffset;
		Slot.ContainerOrdinal = ContainerOrdinal;
		return Slot;
	}

	void BuildSlots(TArray<Fdemo_mapFullMapRewardSlot>& Slots)
	{
		int32 StandardOrdinal = 0;
		int32 EliteOrdinal = 0;
		int32 BossOrdinal = 0;
		const TArray<Fdemo_mapEnemyEncounterSpawnRecord>& Base =
			Fdemo_mapEnemyEncounterConfig::GetSpawnRecords();
		for (const Fdemo_mapEnemyEncounterSpawnRecord& Record : Base)
		{
			const Edemo_mapFullMapRewardClass RewardClass =
				ClassForBaseEnemy(Record);
			int32 ClassOrdinal = RewardClass
					== Edemo_mapFullMapRewardClass::EnemyStandard
				? ++StandardOrdinal
				: RewardClass == Edemo_mapFullMapRewardClass::EnemyElite
					? ++EliteOrdinal
					: ++BossOrdinal;
			Slots.Add(EnemySlot(ClassOrdinal, RewardClass, Record));
		}

		const Fdemo_mapEnemyEncounterSpawnRecord* StandardMelee =
			Fdemo_mapEnemyEncounterConfig::Find(
				Fdemo_mapEnemyEncounterIds::MainMeleeStandard);
		const Fdemo_mapEnemyEncounterSpawnRecord* StandardRanged =
			Fdemo_mapEnemyEncounterConfig::Find(
				Fdemo_mapEnemyEncounterIds::MainRangedStandard);
		const Fdemo_mapEnemyEncounterSpawnRecord* EliteMelee =
			Fdemo_mapEnemyEncounterConfig::Find(
				Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced);
		static const FVector StandardOffsets[] = {
			FVector(320.0f, 0.0f, 0.0f),
			FVector(-320.0f, 0.0f, 0.0f),
			FVector(0.0f, 320.0f, 0.0f),
			FVector(0.0f, -320.0f, 0.0f),
			FVector(480.0f, 320.0f, 0.0f),
			FVector(-480.0f, -320.0f, 0.0f),
			FVector(640.0f, -320.0f, 0.0f),
			FVector(-640.0f, 320.0f, 0.0f)
		};
		for (int32 Index = 0;
			StandardMelee && StandardRanged
				&& StandardOrdinal
					< Fdemo_mapRewardFullMapDistribution::
						StandardEnemyCount;
			++Index)
		{
			const Fdemo_mapEnemyEncounterSpawnRecord& Prototype =
				(Index % 2 == 0) ? *StandardMelee : *StandardRanged;
			const int32 Ordinal = ++StandardOrdinal;
			Slots.Add(EnemySlot(
				Ordinal,
				Edemo_mapFullMapRewardClass::EnemyStandard,
				CloneEnemy(
					Prototype,
					Edemo_mapFullMapRewardClass::EnemyStandard,
					Ordinal,
					StandardOffsets[Index])));
		}
		if (EliteMelee
			&& EliteOrdinal
				< Fdemo_mapRewardFullMapDistribution::EliteEnemyCount)
		{
			const int32 Ordinal = ++EliteOrdinal;
			Slots.Add(EnemySlot(
				Ordinal,
				Edemo_mapFullMapRewardClass::EnemyElite,
				CloneEnemy(
					*EliteMelee,
					Edemo_mapFullMapRewardClass::EnemyElite,
					Ordinal,
					FVector(0.0f, 520.0f, 0.0f))));
		}

		const auto* Wood =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::ChestMainWood);
		const auto* Ore =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::ChestMainOre);
		const auto* High =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::ChestSideHighValue);
		int32 ContainerOrdinal = 0;
		if (Wood)
		{
			for (int32 Number = 1; Number <= 60; ++Number)
			{
				Slots.Add(ContainerSlot(
					Edemo_mapFullMapRewardClass::ContainerBasicWood,
					Number,
					ContainerOrdinal++,
					0,
					GridOffset(Number, 10, 190.0f),
					*Wood,
					AreaWood));
			}
		}
		if (Ore)
		{
			for (int32 Number = 1; Number <= 60; ++Number)
			{
				Slots.Add(ContainerSlot(
					Edemo_mapFullMapRewardClass::ContainerBasicOre,
					Number,
					ContainerOrdinal++,
					1,
					GridOffset(Number, 10, 190.0f),
					*Ore,
					AreaOre));
			}
		}
		if (High)
		{
			for (int32 Number = 1; Number <= 15; ++Number)
			{
				Slots.Add(ContainerSlot(
					Edemo_mapFullMapRewardClass::ContainerHighValue,
					Number,
					ContainerOrdinal++,
					2,
					GridOffset(Number, 5, 240.0f),
					*High,
					AreaHighValue));
			}
		}
	}
}

bool Fdemo_mapFullMapRewardSlot::IsValid() const
{
	const bool bFiniteOffset =
		FMath::IsFinite(LocalOffset.X)
		&& FMath::IsFinite(LocalOffset.Y)
		&& FMath::IsFinite(LocalOffset.Z)
		&& LocalOffset.GetAbsMax() <= 10000.0f;
	return !SlotId.IsNone()
		&& !StableSourceRoleId.IsNone()
		&& !MarkerId.IsNone()
		&& !RouteId.IsNone()
		&& !AreaId.IsNone()
		&& !ProjectionId.IsNone()
		&& !BudgetProfileId.IsNone()
		&& !SourceTags.IsEmpty()
		&& !SourceTags.Contains(NAME_None)
		&& BaseSourceValue > 0
		&& !SourceMarkerType.IsNone()
		&& SourceMarkerIndex >= INDEX_NONE
		&& bFiniteOffset
		&& (IsEnemy()
			? EnemyRecord.IsValid()
				&& ContainerOrdinal == INDEX_NONE
				&& MarkerId == EnemyRecord.Identity.SpawnMarkerId
			: ContainerOrdinal >= 0
				&& SourceMarkerType == TEXT("Chest")
				&& SourceMarkerIndex >= 0
				&& SourceMarkerIndex <= 2);
}

const TArray<Fdemo_mapFullMapRewardSlot>&
Fdemo_mapRewardFullMapDistribution::GetSlots()
{
	static const TArray<Fdemo_mapFullMapRewardSlot> Slots = []()
	{
		TArray<Fdemo_mapFullMapRewardSlot> Result;
		Result.Reserve(TotalSlotCount);
		BuildSlots(Result);
		return Result;
	}();
	return Slots;
}

const Fdemo_mapFullMapRewardSlot*
Fdemo_mapRewardFullMapDistribution::Find(FName SlotId)
{
	return GetSlots().FindByPredicate(
		[SlotId](const Fdemo_mapFullMapRewardSlot& Slot)
		{
			return Slot.SlotId == SlotId;
		});
}

const Fdemo_mapFullMapRewardSlot*
Fdemo_mapRewardFullMapDistribution::FindEnemyByEncounterId(FName EncounterId)
{
	return GetSlots().FindByPredicate(
		[EncounterId](const Fdemo_mapFullMapRewardSlot& Slot)
		{
			return Slot.IsEnemy()
				&& Slot.EnemyRecord.Identity.EncounterId == EncounterId;
		});
}

Fdemo_mapRewardSourceProjection
Fdemo_mapRewardFullMapDistribution::BuildProjection(
	const Fdemo_mapFullMapRewardSlot& Slot)
{
	const Fdemo_mapRewardSourceProjection* Source =
		Fdemo_mapRewardSourceProjectionRegistry::Find(Slot.ProjectionId);
	if (!Source)
	{
		return Fdemo_mapRewardSourceProjection();
	}
	Fdemo_mapRewardSourceProjection Result = *Source;
	Result.StableSourceRoleId = Slot.StableSourceRoleId;
	Result.MarkerId = Slot.MarkerId;
	Result.EncounterId = Slot.IsEnemy()
		? Slot.EnemyRecord.Identity.EncounterId
		: NAME_None;
	Result.BudgetProfileId = Slot.BudgetProfileId;
	Result.SourceTags = Slot.SourceTags;
	Result.BaseSourceValue = Slot.BaseSourceValue;
	return Result;
}

Fdemo_mapFullMapDistributionCounts
Fdemo_mapRewardFullMapDistribution::Count()
{
	Fdemo_mapFullMapDistributionCounts Result;
	for (const Fdemo_mapFullMapRewardSlot& Slot : GetSlots())
	{
		switch (Slot.RewardClass)
		{
		case Edemo_mapFullMapRewardClass::EnemyStandard:
			++Result.StandardEnemies;
			break;
		case Edemo_mapFullMapRewardClass::EnemyElite:
			++Result.EliteEnemies;
			break;
		case Edemo_mapFullMapRewardClass::Boss:
			++Result.Bosses;
			break;
		case Edemo_mapFullMapRewardClass::ContainerBasicWood:
			++Result.BasicWoodContainers;
			break;
		case Edemo_mapFullMapRewardClass::ContainerBasicOre:
			++Result.BasicOreContainers;
			break;
		case Edemo_mapFullMapRewardClass::ContainerHighValue:
			++Result.HighValueContainers;
			break;
		}
		Result.BaseSourceValue += Slot.BaseSourceValue;
	}
	return Result;
}

bool Fdemo_mapRewardFullMapDistribution::Validate(FString* OutError)
{
	FString DependencyError;
	if (!Fdemo_mapRewardGenerationRegistry::Validate(&DependencyError)
		|| !Fdemo_mapRewardSourceProjectionRegistry::Validate(&DependencyError)
		|| !Fdemo_mapEnemyEncounterConfig::Validate(&DependencyError))
	{
		if (OutError)
		{
			*OutError = TEXT("dependency_validation_failed:") + DependencyError;
		}
		return false;
	}

	TSet<FName> SlotIds;
	TSet<FName> SourceRoleIds;
	TSet<FName> MarkerIds;
	TSet<FName> EncounterIds;
	TSet<int32> ContainerOrdinals;
	for (const Fdemo_mapFullMapRewardSlot& Slot : GetSlots())
	{
		const auto* Profile =
			Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
				Slot.BudgetProfileId);
		const auto* Projection =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Slot.ProjectionId);
		const Fdemo_mapRewardSourceProjection RuntimeProjection =
			BuildProjection(Slot);
		const bool bDuplicate =
			SlotIds.Contains(Slot.SlotId)
			|| SourceRoleIds.Contains(Slot.StableSourceRoleId)
			|| MarkerIds.Contains(Slot.MarkerId)
			|| (Slot.IsEnemy()
				&& EncounterIds.Contains(
					Slot.EnemyRecord.Identity.EncounterId))
			|| (Slot.IsContainer()
				&& ContainerOrdinals.Contains(
					Slot.ContainerOrdinal));
		if (!Slot.IsValid()
			|| bDuplicate
			|| !Profile
			|| !Projection
			|| !RuntimeProjection.IsValid()
			|| Profile->ProfileId != Slot.BudgetProfileId
			|| Profile->BaseValue != Slot.BaseSourceValue
			|| Projection->BudgetProfileId != Slot.BudgetProfileId
			|| Projection->SourceTags != Slot.SourceTags
			|| Slot.BaseSourceValue
				!= BaseValueForClass(Slot.RewardClass))
		{
			if (OutError)
			{
				*OutError = FString::Printf(
					TEXT("invalid_or_duplicate_slot:%s"),
					*Slot.SlotId.ToString());
			}
			return false;
		}
		SlotIds.Add(Slot.SlotId);
		SourceRoleIds.Add(Slot.StableSourceRoleId);
		MarkerIds.Add(Slot.MarkerId);
		if (Slot.IsEnemy())
		{
			EncounterIds.Add(Slot.EnemyRecord.Identity.EncounterId);
		}
		else
		{
			ContainerOrdinals.Add(Slot.ContainerOrdinal);
		}
	}

	const Fdemo_mapFullMapDistributionCounts Counts = Count();
	const Fdemo_mapFullMapRewardSlot* Boss =
		GetSlots().FindByPredicate(
			[](const Fdemo_mapFullMapRewardSlot& Slot)
			{
				return Slot.RewardClass
					== Edemo_mapFullMapRewardClass::Boss;
			});
	const bool bExact =
		GetSlots().Num() == TotalSlotCount
		&& SlotIds.Num() == TotalSlotCount
		&& SourceRoleIds.Num() == TotalSlotCount
		&& MarkerIds.Num() == TotalSlotCount
		&& Counts.StandardEnemies == StandardEnemyCount
		&& Counts.EliteEnemies == EliteEnemyCount
		&& Counts.Bosses == BossCount
		&& Counts.EnemyTotal() == TotalEnemyCount
		&& Counts.BasicWoodContainers > 0
		&& Counts.BasicOreContainers > 0
		&& Counts.BasicContainerTotal() == BasicContainerCount
		&& Counts.HighValueContainers == HighValueContainerCount
		&& Counts.ContainerTotal() == TotalContainerCount
		&& Counts.SlotTotal() == TotalSlotCount
		&& Counts.BaseSourceValue == TotalBaseSourceValue
		&& ContainerOrdinals.Num() == TotalContainerCount
		&& Boss
		&& Boss->StableSourceRoleId
			== Fdemo_mapRewardSourceRoleIds::BossPrototype
		&& Boss->ProjectionId
			== Fdemo_mapRewardProjectionIds::CorpseBossPrototype
		&& Boss->EnemyRecord.Identity.EncounterId
			== Fdemo_mapEnemyEncounterIds::MainMeleeHeavy;
	if (!bExact)
	{
		if (OutError)
		{
			*OutError = FString::Printf(
				TEXT("count_or_total_mismatch:slots=%d standard=%d elite=%d boss=%d wood=%d ore=%d high=%d base=%lld"),
				GetSlots().Num(),
				Counts.StandardEnemies,
				Counts.EliteEnemies,
				Counts.Bosses,
				Counts.BasicWoodContainers,
				Counts.BasicOreContainers,
				Counts.HighValueContainers,
				Counts.BaseSourceValue);
		}
		return false;
	}
	return true;
}
