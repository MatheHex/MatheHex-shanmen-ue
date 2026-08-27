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
enum class EShanmenDefenseOperation : uint8
{
	PreventAll,
	ReduceFraction,
	AbsorbPoints,
	PreventLethal
};

UENUM(BlueprintType)
enum class EShanmenDefenseOutcome : uint8
{
	Invalid,
	Applied,
	Mitigated,
	FullyPrevented,
	Evaded,
	PerfectGuarded
};

/** Recommended ordering bands. Sources may share a band and use Order for deterministic priority. */
struct SHANMENCOMBATCORE_API FShanmenDefenseOrder
{
	static constexpr int32 Avoidance = 100;
	static constexpr int32 PerfectGuard = 200;
	static constexpr int32 Guard = 300;
	static constexpr int32 Shield = 400;
	static constexpr int32 Resistance = 500;
	static constexpr int32 LethalInterception = 600;
};

/** Mutable capture input. A successful capture produces a read-only action snapshot. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenCombatActionCapture
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer SourceTags;

	bool IsValid() const;
};

/** Frozen activation values. Fields are private and Blueprint-read-only by construction. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenCombatActionSnapshot
{
	GENERATED_BODY()

public:
	static bool TryCapture(const FShanmenCombatActionCapture& Capture, FShanmenCombatActionSnapshot& OutSnapshot);

	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	const FGuid& GetActivationId() const { return ActivationId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const FGuid& GetSourceItemInstanceId() const { return SourceItemInstanceId; }
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FGameplayTagContainer& GetSourceTags() const { return SourceTags; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FGuid OwnerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FGuid ActivationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FGuid SourceEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FGuid SourceItemInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FShanmenContentStamp Content;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer SourceTags;
};

/** A geometric contact sample. Target policy decides whether the candidate is legal. */
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

/** Output of the future offensive formula layer; DefenseResolver consumes but never computes it. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenDamagePacket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FName FormulaId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float RawDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer DamageTags;

	bool IsValid() const;
};

/** Target vitality sampled at the same authoritative instant as defense layers. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenTargetVitalitySnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float CurrentVitality = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float MaximumVitality = 0.0f;

	bool IsValid() const;
};

/** One ordered, tagged defense source. Magnitude semantics are determined by Operation. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenDefenseLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid LayerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FName RuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGuid SourceInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	EShanmenDefenseOperation Operation = EShanmenDefenseOperation::ReduceFraction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	int32 Order = 0;

	/** PreventAll: ignored; ReduceFraction: 0..1; AbsorbPoints: points; PreventLethal: vitality floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat", meta = (ClampMin = "0.0"))
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	bool bRequiresCommitOnTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer LayerTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer BlockedTargetTags;

	bool IsValid() const;
	bool IsApplicable(
		const FGameplayTagContainer& DamageTags,
		const FGameplayTagContainer& SourceTags,
		const FGameplayTagContainer& TargetTags) const;
};

/** Active defense sources captured for this target and impact. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenDefenseSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	TArray<FShanmenDefenseLayer> Layers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat")
	FGameplayTagContainer TargetTags;

	bool IsValid() const;
};

/** Complete immutable request consumed by the pure defense resolver. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenImpactRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FGuid ImpactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FShanmenHitCandidate Candidate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FShanmenDamagePacket Damage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FShanmenTargetVitalitySnapshot TargetVitality;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FShanmenDefenseSnapshot Defense;

	bool IsValid() const;
};

/** Per-layer audit output; exact source identity supports later commit receipts. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATCORE_API FShanmenDefenseLayerResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FGuid LayerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FGuid SourceInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	EShanmenDefenseOperation Operation = EShanmenDefenseOperation::ReduceFraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	int32 Order = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float PreventedDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	bool bRequiresCommit = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	FGameplayTagContainer LayerTags;
};

/** Auditable output satisfying RawDamage = PreventedDamage + FinalDamage. */
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
	float PreventedDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	float FinalDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat")
	TArray<FShanmenDefenseLayerResult> TriggeredLayers;

	bool IsConserved(float Tolerance = KINDA_SMALL_NUMBER) const;
};
