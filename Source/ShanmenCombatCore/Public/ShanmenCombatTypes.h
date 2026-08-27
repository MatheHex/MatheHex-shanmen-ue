#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenCoreTypes.h"

#include "ShanmenCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EShanmenCombatActionPhase : uint8
{
	Idle,
	Startup,
	Active,
	Recovery,
	Cancelled,
	Interrupted
};

UENUM(BlueprintType)
enum class EShanmenHitDetectorKind : uint8
{
	WeaponTrajectory,
	Shape,
	Projectile,
	ControlledObject,
	PersistentZone,
	TargetedRule
};

UENUM(BlueprintType)
enum class EShanmenDefenseOutcome : uint8
{
	Invalid,
	Applied,
	Mitigated,
	Evaded,
	PerfectGuarded
};

/** Values frozen at action activation; later equipment changes cannot mutate this action. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenCombatActionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid RunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid ActivationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid SourceEntityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid SourceItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FShanmenContentStamp Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float BasePower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer SourceTags;

	bool IsValid() const;
};

/** A geometric contact sample. It is not yet an accepted combat hit. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenHitCandidate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid ActivationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid SourceEntityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid TargetEntityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FName DetectorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	EShanmenHitDetectorKind DetectorKind = EShanmenHitDetectorKind::Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FVector HitNormal = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0"))
	int32 HitOrdinal = 0;

	bool IsValid() const;
};

/** Active defenses sampled at the authoritative impact instant. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenDefenseSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	bool bDodgeWindowActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	bool bPerfectGuardWindowActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	bool bGuardActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardReductionFraction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float ShieldPoints = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArmorResistanceFraction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer TargetTags;

	bool IsValid() const;
};

/** Complete immutable request consumed by the pure defense resolver. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenImpactRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid ImpactId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FShanmenHitCandidate Candidate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float IncomingDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FShanmenDefenseSnapshot Defense;

	bool IsValid() const;
};

/** Auditable output; every prevention layer remains visible to receipts and tests. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenImpactResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	bool bAccepted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FGuid ImpactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	EShanmenDefenseOutcome Outcome = EShanmenDefenseOutcome::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float GuardPrevented = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float ShieldAbsorbed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float ArmorPrevented = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float FinalDamage = 0.0f;
};
