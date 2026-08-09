#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapEnemyEncounterArchetype : uint8
{
	MeleeStandard,
	MeleeHeavy,
	RangedStandard,
	MeleeEnhanced,
	RangedEnhanced
};

struct Fdemo_mapEnemyEncounterIdentity
{
	FName EncounterId = NAME_None;
	FName RouteId = NAME_None;
	FName SpawnMarkerId = NAME_None;
	FName LootTableId = NAME_None;
	FName SkillProfileId = NAME_None;

	bool IsValid() const
	{
		return !EncounterId.IsNone()
			&& !RouteId.IsNone()
			&& !SpawnMarkerId.IsNone()
			&& !LootTableId.IsNone();
	}
};

struct Fdemo_mapEnemyCombatTuning
{
	int32 MaxHealth = 0;
	float MovementSpeed = 0.0f;
	float AttackDamage = 0.0f;
	float AttackWindup = 0.0f;
	float AttackCooldown = 0.0f;
	float ProjectileWidth = 0.0f;
	float ProjectileCollisionRadius = 0.0f;
	float ProjectileSpeed = 0.0f;
	float ProjectileMaxDistance = 0.0f;

	bool IsValid() const;
};

/** One immutable P7 product-world enemy projection record. */
struct Fdemo_mapEnemyEncounterSpawnRecord
{
	Edemo_mapEnemyEncounterArchetype Archetype =
		Edemo_mapEnemyEncounterArchetype::MeleeStandard;
	Fdemo_mapEnemyEncounterIdentity Identity;
	Fdemo_mapEnemyCombatTuning Tuning;
	FName SourceMarkerType = NAME_None;
	int32 SourceMarkerIndex = INDEX_NONE;
	FVector LocalOffset = FVector::ZeroVector;
	bool bEnhanced = false;

	bool IsValid() const;
};
