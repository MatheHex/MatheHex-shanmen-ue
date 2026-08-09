#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapFixedLootTableRegistry.h"

const FName Fdemo_mapEnemyEncounterIds::MainRoute(TEXT("P7.Route.Main"));
const FName Fdemo_mapEnemyEncounterIds::SideMeleeRoute(TEXT("P7.Route.Side.Melee"));
const FName Fdemo_mapEnemyEncounterIds::SideRangedRoute(TEXT("P7.Route.Side.Ranged"));
const FName Fdemo_mapEnemyEncounterIds::MainMeleeStandard(TEXT("P7.Encounter.Main.Melee.Standard"));
const FName Fdemo_mapEnemyEncounterIds::MainMeleeHeavy(TEXT("P7.Encounter.Main.Melee.Heavy"));
const FName Fdemo_mapEnemyEncounterIds::MainRangedStandard(TEXT("P7.Encounter.Main.Ranged.Standard"));
const FName Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced(TEXT("P7.Encounter.Side.Melee.Enhanced"));
const FName Fdemo_mapEnemyEncounterIds::SideRangedEnhanced(TEXT("P7.Encounter.Side.Ranged.Enhanced"));
const FName Fdemo_mapEnemyEncounterIds::MarkerMainMeleeStandard(TEXT("P7.Marker.Enemy.Main.Melee.Standard"));
const FName Fdemo_mapEnemyEncounterIds::MarkerMainMeleeHeavy(TEXT("P7.Marker.Enemy.Main.Melee.Heavy"));
const FName Fdemo_mapEnemyEncounterIds::MarkerMainRangedStandard(TEXT("P7.Marker.Enemy.Main.Ranged.Standard"));
const FName Fdemo_mapEnemyEncounterIds::MarkerSideMeleeEnhanced(TEXT("P7.Marker.Enemy.Side.Melee.Enhanced"));
const FName Fdemo_mapEnemyEncounterIds::MarkerSideRangedEnhanced(TEXT("P7.Marker.Enemy.Side.Ranged.Enhanced"));

namespace
{
	Fdemo_mapEnemyCombatTuning Melee(
		int32 Health,
		float Speed,
		float Damage,
		float Cooldown)
	{
		Fdemo_mapEnemyCombatTuning Result;
		Result.MaxHealth = Health;
		Result.MovementSpeed = Speed;
		Result.AttackDamage = Damage;
		Result.AttackCooldown = Cooldown;
		return Result;
	}

	Fdemo_mapEnemyCombatTuning Heavy()
	{
		Fdemo_mapEnemyCombatTuning Result = Melee(5, 180.0f, 1.0f, 2.40f);
		Result.AttackWindup = 0.85f;
		return Result;
	}

	Fdemo_mapEnemyCombatTuning Ranged(
		int32 Health,
		float Speed,
		float Damage,
		float Windup,
		float Cooldown)
	{
		Fdemo_mapEnemyCombatTuning Result;
		Result.MaxHealth = Health;
		Result.MovementSpeed = Speed;
		Result.AttackDamage = Damage;
		Result.AttackWindup = Windup;
		Result.AttackCooldown = Cooldown;
		Result.ProjectileWidth = 50.0f;
		Result.ProjectileCollisionRadius = 25.0f;
		Result.ProjectileSpeed = 800.0f;
		Result.ProjectileMaxDistance = 1800.0f;
		return Result;
	}

	Fdemo_mapEnemyEncounterSpawnRecord Record(
		Edemo_mapEnemyEncounterArchetype Archetype,
		FName EncounterId,
		FName RouteId,
		FName SpawnMarkerId,
		FName LootTableId,
		FName SkillProfileId,
		FName SourceMarkerType,
		int32 SourceMarkerIndex,
		FVector LocalOffset,
		const Fdemo_mapEnemyCombatTuning& Tuning,
		bool bEnhanced)
	{
		Fdemo_mapEnemyEncounterSpawnRecord Result;
		Result.Archetype = Archetype;
		Result.Identity = {
			EncounterId,
			RouteId,
			SpawnMarkerId,
			LootTableId,
			SkillProfileId };
		Result.SourceMarkerType = SourceMarkerType;
		Result.SourceMarkerIndex = SourceMarkerIndex;
		Result.LocalOffset = LocalOffset;
		Result.Tuning = Tuning;
		Result.bEnhanced = bEnhanced;
		return Result;
	}
}

const TArray<Fdemo_mapEnemyEncounterSpawnRecord>&
Fdemo_mapEnemyEncounterConfig::GetSpawnRecords()
{
	static const TArray<Fdemo_mapEnemyEncounterSpawnRecord> Records = {
		Record(
			Edemo_mapEnemyEncounterArchetype::MeleeStandard,
			Fdemo_mapEnemyEncounterIds::MainMeleeStandard,
			Fdemo_mapEnemyEncounterIds::MainRoute,
			Fdemo_mapEnemyEncounterIds::MarkerMainMeleeStandard,
			Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard,
			Fdemo_mapEnemySkillProfileIds::StandardMeleeDash,
			TEXT("MeleeEnemySpawn"),
			INDEX_NONE,
			FVector::ZeroVector,
			Melee(3, 260.0f, 1.0f, 1.20f),
			false),
		Record(
			Edemo_mapEnemyEncounterArchetype::MeleeHeavy,
			Fdemo_mapEnemyEncounterIds::MainMeleeHeavy,
			Fdemo_mapEnemyEncounterIds::MainRoute,
			Fdemo_mapEnemyEncounterIds::MarkerMainMeleeHeavy,
			Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy,
			NAME_None,
			TEXT("HeavyEnemySpawn"),
			INDEX_NONE,
			FVector::ZeroVector,
			Heavy(),
			false),
		Record(
			Edemo_mapEnemyEncounterArchetype::RangedStandard,
			Fdemo_mapEnemyEncounterIds::MainRangedStandard,
			Fdemo_mapEnemyEncounterIds::MainRoute,
			Fdemo_mapEnemyEncounterIds::MarkerMainRangedStandard,
			Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard,
			Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep,
			TEXT("RangedEnemySpawn"),
			INDEX_NONE,
			FVector::ZeroVector,
			Ranged(3, 240.0f, 1.0f, 0.40f, 1.60f),
			false),
		Record(
			Edemo_mapEnemyEncounterArchetype::MeleeEnhanced,
			Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced,
			Fdemo_mapEnemyEncounterIds::SideMeleeRoute,
			Fdemo_mapEnemyEncounterIds::MarkerSideMeleeEnhanced,
			Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced,
			Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash,
			TEXT("TrainingTargetSpawn"),
			1,
			FVector(420.0f, 0.0f, 0.0f),
			Melee(6, 300.0f, 2.0f, 1.00f),
			true),
		Record(
			Edemo_mapEnemyEncounterArchetype::RangedEnhanced,
			Fdemo_mapEnemyEncounterIds::SideRangedEnhanced,
			Fdemo_mapEnemyEncounterIds::SideRangedRoute,
			Fdemo_mapEnemyEncounterIds::MarkerSideRangedEnhanced,
			Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced,
			Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep,
			TEXT("TrainingTargetSpawn"),
			2,
			FVector(-420.0f, 0.0f, 0.0f),
			Ranged(5, 280.0f, 2.0f, 0.32f, 1.40f),
			true)
	};
	return Records;
}

const Fdemo_mapEnemyEncounterSpawnRecord*
Fdemo_mapEnemyEncounterConfig::Find(FName EncounterId)
{
	return GetSpawnRecords().FindByPredicate(
		[EncounterId](const Fdemo_mapEnemyEncounterSpawnRecord& Record)
		{
			return Record.Identity.EncounterId == EncounterId;
		});
}

bool Fdemo_mapEnemyEncounterConfig::Validate(FString* OutError)
{
	const TArray<Fdemo_mapEnemyEncounterSpawnRecord>& Records = GetSpawnRecords();
	TSet<FName> Encounters;
	TSet<FName> Markers;
	TSet<FName> LootTables;
	int32 MainCount = 0;
	int32 SideCount = 0;
	for (const Fdemo_mapEnemyEncounterSpawnRecord& Record : Records)
	{
		if (!Record.IsValid()
			|| Encounters.Contains(Record.Identity.EncounterId)
			|| Markers.Contains(Record.Identity.SpawnMarkerId)
			|| LootTables.Contains(Record.Identity.LootTableId)
			|| !Fdemo_mapFixedLootTableRegistry::Find(Record.Identity.LootTableId)
			|| (!Record.Identity.SkillProfileId.IsNone()
				&& !Fdemo_mapEnemySkillPrototypeConfig::FindProfile(
					Record.Identity.SkillProfileId)))
		{
			if (OutError) *OutError = TEXT("P7 encounter record or identity/profile/table binding is invalid.");
			return false;
		}
		Encounters.Add(Record.Identity.EncounterId);
		Markers.Add(Record.Identity.SpawnMarkerId);
		LootTables.Add(Record.Identity.LootTableId);
		if (Record.Identity.RouteId == Fdemo_mapEnemyEncounterIds::MainRoute) ++MainCount;
		else ++SideCount;
	}
	if (Records.Num() != 5 || MainCount != 3 || SideCount != 2)
	{
		if (OutError) *OutError = TEXT("P7 composition must contain exactly three main and two side enemies.");
		return false;
	}
	return true;
}
