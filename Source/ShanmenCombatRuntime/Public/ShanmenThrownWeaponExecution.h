#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenDetectorEmissionSession.h"

#include "ShanmenThrownWeaponExecution.generated.h"

/** Runtime state for one exact, hand-thrown item activation. */
UENUM(BlueprintType)
enum class EShanmenThrownWeaponState : uint8
{
	Ready,
	InFlight,
	Spent
};

/**
 * Content-owned values for the first straight-flight thrown-weapon action.
 * Arc solving, homing, input bindings, and inventory policy stay external.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FName DetectorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FName FormulaId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	float TechniquePowerCoefficient = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FGameplayTagContainer DamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	bool bRejectSelf = true;
};

/** Immutable content definition for initial straight throwing. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenThrownWeaponDefinitionCapture& Capture,
		FShanmenThrownWeaponDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetDetectorId() const { return DetectorId; }
	FName GetFormulaId() const { return FormulaId; }
	float GetBaseDamage() const { return BaseDamage; }
	float GetTechniquePowerCoefficient() const
	{
		return TechniquePowerCoefficient;
	}
	float GetLaunchSpeed() const { return LaunchSpeed; }
	const FGameplayTagContainer& GetDamageTags() const { return DamageTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const
	{
		return RequiredTargetTags;
	}
	bool RejectsSelf() const { return bRejectSelf; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FName DetectorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FName FormulaId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	float BaseDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	float TechniquePowerCoefficient = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	float LaunchSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer DamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	bool bRejectSelf = true;
};

/** Technique value frozen when this exact physical item is activated. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponOffenseSnapshot
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		float TechniquePower,
		FShanmenThrownWeaponOffenseSnapshot& OutSnapshot);
	bool IsValid() const;
	float GetTechniquePower() const { return TechniquePower; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	float TechniquePower = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	bool bCaptured = false;
};

/**
 * Immutable straight-flight launch proof. The direction is canonical and there
 * is intentionally no redirect, steering, arc, or homing field.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponLaunchReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetLaunchId() const { return LaunchId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetDirection() const { return Direction; }
	float GetSpeed() const { return Speed; }

private:
	friend class FShanmenThrownWeaponExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FGuid LaunchId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	float Speed = 0.0f;
};

/** Auditable pure-kernel result for one accepted thrown-item contact. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponImpactReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class FShanmenThrownWeaponExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactResult Result;
};

/**
 * Pure deterministic kernel for one exact initial-tier thrown item.
 *
 * The runtime freezes a single straight launch and projectile candidate stream.
 * Product Actor motion, inventory reserve/commit, input, gravity, arc authoring,
 * path finding, and presentation remain later adapters.
 */
class SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponExecution
{
public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenThrownWeaponDefinition& Definition,
		const FShanmenThrownWeaponOffenseSnapshot& Offense,
		FShanmenThrownWeaponExecution& OutExecution);

	bool IsValid() const;
	bool TryLaunchStraight(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FVector& Origin,
		const FVector& AimDirection,
		FShanmenThrownWeaponLaunchReceipt& OutReceipt);
	bool TryBeginEmission(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWorldHitContext& OutContext);
	bool TryResolveCandidate(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenHitCandidate& Candidate,
		const FShanmenTargetVitalitySnapshot& TargetVitality,
		const FShanmenDefenseSnapshot& Defense,
		FShanmenThrownWeaponImpactReceipt& OutReceipt);
	bool TryEndEmission(const FShanmenActionOrchestrator& ActionRuntime);
	bool TryFinishFlight(const FShanmenActionOrchestrator& ActionRuntime);
	void EndEmissionForTermination();
	void Reset();

	EShanmenThrownWeaponState GetState() const { return State; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenThrownWeaponDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenThrownWeaponLaunchReceipt& GetLaunchReceipt() const
	{
		return LaunchReceipt;
	}
	bool IsEmissionActive() const { return EmissionSession.IsEmissionActive(); }
	int32 NumAcceptedImpacts() const { return ImpactLedger.Num(); }

private:
	bool MatchesActionRuntime(
		const FShanmenActionOrchestrator& ActionRuntime) const;
	bool IsTargetAllowed(
		const FShanmenHitCandidate& Candidate,
		const FShanmenDefenseSnapshot& Defense) const;
	bool TryBuildDamagePacket(FShanmenDamagePacket& OutPacket) const;

	FShanmenCombatActionSnapshot Action;
	FShanmenThrownWeaponDefinition Definition;
	FShanmenThrownWeaponOffenseSnapshot Offense;
	FShanmenThrownWeaponLaunchReceipt LaunchReceipt;
	FShanmenDetectorEmissionSession EmissionSession;
	FShanmenImpactLedger ImpactLedger;
	EShanmenThrownWeaponState State = EShanmenThrownWeaponState::Ready;
	bool bInitialized = false;
};
