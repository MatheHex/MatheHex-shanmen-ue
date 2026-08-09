#pragma once

#include "CoreMinimal.h"
#include "demo_mapCombatTypes.generated.h"

UENUM(BlueprintType)
enum class Edemo_mapFaction : uint8
{
	Player,
	Friendly,
	Hostile,
	Neutral
};

UENUM(BlueprintType)
enum class Edemo_mapTargetRelation : uint8
{
	Self,
	Friendly,
	Hostile,
	Neutral,
	Invalid
};

USTRUCT(BlueprintType)
struct Fdemo_mapTargetFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAffectSelf = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAffectFriendly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAffectHostile = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAffectNeutral = false;
};

USTRUCT(BlueprintType)
struct Fdemo_mapCommonSkillParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Cooldown = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float VerticalTolerance = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) Fdemo_mapTargetFilter TargetFilter;
};

USTRUCT(BlueprintType)
struct Fdemo_mapCircleSkillParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CastRange = 800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) Fdemo_mapCommonSkillParams CommonParams;
};

USTRUCT(BlueprintType)
struct Fdemo_mapConeSkillParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FullAngleDegrees = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) Fdemo_mapCommonSkillParams CommonParams;
};

USTRUCT(BlueprintType)
struct Fdemo_mapProjectileSkillParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Width = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CollisionRadius = 24.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Speed = 1200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxDistance = 1200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) Fdemo_mapCommonSkillParams CommonParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPierceHostiles = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPassThroughFriendlies = true;
};
