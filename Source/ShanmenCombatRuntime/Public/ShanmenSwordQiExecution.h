#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenDetectorEmissionSession.h"

#include "ShanmenSwordQiExecution.generated.h"

/** Runtime state for one exact sword-qi activation. */
UENUM(BlueprintType)
enum class EShanmenSwordQiState : uint8
{
	Ready,
	InFlight,
	Dissipated
};

/**
 * Content-owned values for the first sword-qi action.
 *
 * The damage channel is authored rather than fixed to Physical or Spirit.
 * Actor class, input, presentation, collision shape, and lifetime stay outside
 * this pure runtime contract.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordQiDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi")
	FName DetectorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi")
	FName FormulaId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi", meta = (ClampMin = "0.0"))
	float AttackPowerCoefficient = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi", meta = (ClampMin = "0.0"))
	float FlightSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi", meta = (ClampMin = "0.0"))
	float MaximumRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi")
	FGameplayTagContainer DamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordQi")
	bool bRejectSelf = true;
};

/** Immutable content definition for one basic sword-qi release. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordQiDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenSwordQiDefinitionCapture& Capture,
		FShanmenSwordQiDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetDetectorId() const { return DetectorId; }
	FName GetFormulaId() const { return FormulaId; }
	float GetBaseDamage() const { return BaseDamage; }
	float GetAttackPowerCoefficient() const { return AttackPowerCoefficient; }
	float GetFlightSpeed() const { return FlightSpeed; }
	float GetMaximumRange() const { return MaximumRange; }
	const FGameplayTagContainer& GetDamageTags() const { return DamageTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const
	{
		return RequiredTargetTags;
	}
	bool RejectsSelf() const { return bRejectSelf; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FName DetectorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FName FormulaId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float BaseDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float AttackPowerCoefficient = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float FlightSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float MaximumRange = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer DamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	bool bRejectSelf = true;
};

/** Offensive value frozen when the sword action is activated. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordQiOffenseSnapshot
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		float AttackPower,
		FShanmenSwordQiOffenseSnapshot& OutSnapshot);
	bool IsValid() const;
	float GetAttackPower() const { return AttackPower; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float AttackPower = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	bool bCaptured = false;
};

/** Immutable proof of one canonical sword-qi release. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordQiLaunchReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetLaunchId() const { return LaunchId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetDirection() const { return Direction; }
	float GetSpeed() const { return Speed; }
	float GetMaximumRange() const { return MaximumRange; }

private:
	friend class FShanmenSwordQiExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FGuid LaunchId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float Speed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	float MaximumRange = 0.0f;
};

/** Auditable pure-kernel result for one accepted sword-qi contact. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordQiImpactReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class FShanmenSwordQiExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordQi", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactResult Result;
};

/**
 * Pure deterministic kernel for one sword-qi release from an exact sword.
 *
 * Active commits a single immutable launch. While the action remains Active,
 * projectile contacts may enter CombatCore through the normal emission and
 * Impact ledger. This class owns no Actor, input, inventory mutation, GAS task,
 * vitality write, collision query, timer, or presentation state.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSwordQiExecution
{
public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSwordQiDefinition& Definition,
		const FShanmenSwordQiOffenseSnapshot& Offense,
		FShanmenSwordQiExecution& OutExecution);

	bool IsValid() const;
	bool TryLaunch(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FVector& Origin,
		const FVector& AimDirection,
		FShanmenSwordQiLaunchReceipt& OutReceipt);
	bool TryBeginEmission(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWorldHitContext& OutContext);
	bool TryResolveCandidate(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenHitCandidate& Candidate,
		const FShanmenTargetVitalitySnapshot& TargetVitality,
		const FShanmenDefenseSnapshot& Defense,
		FShanmenSwordQiImpactReceipt& OutReceipt);
	bool TryEndEmission(const FShanmenActionOrchestrator& ActionRuntime);
	bool TryDissipate(const FShanmenActionOrchestrator& ActionRuntime);
	void EndForActionTermination();
	void Reset();

	EShanmenSwordQiState GetState() const { return State; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSwordQiDefinition& GetDefinition() const { return Definition; }
	const FShanmenSwordQiLaunchReceipt& GetLaunchReceipt() const
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
	FShanmenSwordQiDefinition Definition;
	FShanmenSwordQiOffenseSnapshot Offense;
	FShanmenSwordQiLaunchReceipt LaunchReceipt;
	FShanmenDetectorEmissionSession EmissionSession;
	FShanmenImpactLedger ImpactLedger;
	EShanmenSwordQiState State = EShanmenSwordQiState::Ready;
	bool bInitialized = false;
};
