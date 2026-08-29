#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenDetectorEmissionSession.h"

#include "ShanmenControlledWeaponExecution.generated.h"

UENUM(BlueprintType)
enum class EShanmenControlledWeaponState : uint8
{
	Orbiting,
	Directed,
	Recalled
};

UENUM(BlueprintType)
enum class EShanmenControlledWeaponCommandKind : uint8
{
	Launch,
	Redirect,
	Recall
};

/** Authoring input for the first controlled flying-sword action. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon")
	FName DetectorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon")
	FName FormulaId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon", meta = (ClampMin = "0.0"))
	float ControlPowerCoefficient = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon")
	FGameplayTagContainer DamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ControlledWeapon")
	bool bRejectSelf = true;
};

/** Frozen content definition. Movement speed and steering rate remain product-authored later. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenControlledWeaponDefinitionCapture& Capture,
		FShanmenControlledWeaponDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetDetectorId() const { return DetectorId; }
	FName GetFormulaId() const { return FormulaId; }
	float GetBaseDamage() const { return BaseDamage; }
	float GetControlPowerCoefficient() const { return ControlPowerCoefficient; }
	const FGameplayTagContainer& GetDamageTags() const { return DamageTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const { return RequiredTargetTags; }
	bool RejectsSelf() const { return bRejectSelf; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FName DetectorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FName FormulaId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	float BaseDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	float ControlPowerCoefficient = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer DamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	bool bRejectSelf = true;
};

/** Frozen control strength sampled when the physical flying sword is activated. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponOffenseSnapshot
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		float ControlPower,
		FShanmenControlledWeaponOffenseSnapshot& OutSnapshot);
	bool IsValid() const;
	float GetControlPower() const { return ControlPower; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	float ControlPower = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	bool bCaptured = false;
};

/** Immutable receipt for one accepted launch, redirect, or recall command. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponCommandReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetCommandId() const { return CommandId; }
	const FGuid& GetActivationId() const { return ActivationId; }
	const FGuid& GetSourceItemInstanceId() const { return SourceItemInstanceId; }
	int64 GetSequence() const { return Sequence; }
	EShanmenControlledWeaponCommandKind GetKind() const { return Kind; }
	EShanmenControlledWeaponState GetStateBefore() const { return StateBefore; }
	EShanmenControlledWeaponState GetStateAfter() const { return StateAfter; }
	const FVector& GetDirectionAfter() const { return DirectionAfter; }

private:
	friend class FShanmenControlledWeaponExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FGuid ActivationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FGuid SourceItemInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	int64 Sequence = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	EShanmenControlledWeaponCommandKind Kind = EShanmenControlledWeaponCommandKind::Launch;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	EShanmenControlledWeaponState StateBefore = EShanmenControlledWeaponState::Orbiting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	EShanmenControlledWeaponState StateAfter = EShanmenControlledWeaponState::Orbiting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FVector DirectionAfter = FVector::ZeroVector;
};

/** Auditable pure-kernel result for one controlled-object contact. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponImpactReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class FShanmenControlledWeaponExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ControlledWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactResult Result;
};

/**
 * Deterministic pure runtime for one physically sourced controlled weapon.
 *
 * Product code owns input, movement, Actors, VFX, and validation that the
 * SourceItemInstanceId is deployed in the active Run. This runtime requires
 * that exact item identity, owns monotonic control commands and exact replay,
 * and only emits ControlledObject candidates while both the action and weapon
 * are active. It never mutates item durability, inventory, vitality, or World.
 */
class SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponExecution
{
public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenControlledWeaponDefinition& Definition,
		const FShanmenControlledWeaponOffenseSnapshot& Offense,
		FShanmenControlledWeaponExecution& OutExecution);

	bool IsValid() const;
	bool TryIssueCommand(
		const FShanmenActionOrchestrator& ActionRuntime,
		int64 ExpectedSequence,
		EShanmenControlledWeaponCommandKind Kind,
		const FVector& DesiredDirection,
		FShanmenControlledWeaponCommandReceipt& OutReceipt);
	bool TryBeginEmission(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWorldHitContext& OutContext);
	/** Opens a candidate-only Orbit sample on the same authoritative ordinal stream. */
	bool TryBeginOrbitThreatEmission(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWorldHitContext& OutContext);
	/** Accepts geometry identity only; this path never builds or resolves damage. */
	bool TryAcceptOrbitThreatCandidate(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenHitCandidate& Candidate);
	bool TryEndOrbitThreatEmission(
		const FShanmenActionOrchestrator& ActionRuntime);
	bool TryResolveCandidate(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenHitCandidate& Candidate,
		const FShanmenTargetVitalitySnapshot& TargetVitality,
		const FShanmenDefenseSnapshot& Defense,
		FShanmenControlledWeaponImpactReceipt& OutReceipt);
	bool TryEndEmission(const FShanmenActionOrchestrator& ActionRuntime);
	void EndEmissionForTermination();
	void Reset();

	EShanmenControlledWeaponState GetState() const { return State; }
	const FVector& GetCurrentDirection() const { return CurrentDirection; }
	int64 GetNextCommandSequence() const { return NextCommandSequence; }
	bool IsEmissionActive() const { return EmissionSession.IsEmissionActive(); }
	int32 NumAcceptedImpacts() const { return ImpactLedger.Num(); }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenControlledWeaponDefinition& GetDefinition() const { return Definition; }
	const FShanmenControlledWeaponOffenseSnapshot& GetOffense() const { return Offense; }

private:
	bool MatchesActionRuntime(const FShanmenActionOrchestrator& ActionRuntime) const;
	bool IsTargetAllowed(
		const FShanmenHitCandidate& Candidate,
		const FShanmenDefenseSnapshot& Defense) const;
	bool TryBuildDamagePacket(FShanmenDamagePacket& OutPacket) const;

	FShanmenCombatActionSnapshot Action;
	FShanmenControlledWeaponDefinition Definition;
	FShanmenControlledWeaponOffenseSnapshot Offense;
	FShanmenDetectorEmissionSession EmissionSession;
	FShanmenImpactLedger ImpactLedger;
	TMap<int64, FShanmenControlledWeaponCommandReceipt> CommandLedger;
	EShanmenControlledWeaponState State = EShanmenControlledWeaponState::Orbiting;
	FVector CurrentDirection = FVector::ZeroVector;
	int64 NextCommandSequence = 0;
	bool bInitialized = false;
};
