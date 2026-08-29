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

/** Caller-captured target identity and tags for one completed threat sample. */
class SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponThreatTargetEvidence
{
public:
	static bool TryCapture(
		const FGuid& TargetEntityId,
		const FGameplayTagContainer& TargetTags,
		FShanmenControlledWeaponThreatTargetEvidence& OutEvidence);

	bool IsValid() const { return TargetEntityId.IsValid(); }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGameplayTagContainer& GetTargetTags() const { return TargetTags; }

private:
	FGuid TargetEntityId;
	FGameplayTagContainer TargetTags;
};

/** Explicit outcome of the frozen controlled-weapon target policy. */
enum class EShanmenControlledWeaponThreatTargetDecision : uint8
{
	Accepted,
	RejectedSelf,
	RejectedMissingRequiredTags
};

/** One candidate aligned with its target evidence and policy decision. */
class SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponThreatTargetReceipt
{
public:
	bool IsValid() const;
	const FShanmenHitCandidate& GetCandidate() const { return Candidate; }
	const FGameplayTagContainer& GetTargetTags() const { return TargetTags; }
	EShanmenControlledWeaponThreatTargetDecision GetDecision() const
	{
		return Decision;
	}
	bool IsAccepted() const
	{
		return Decision
			== EShanmenControlledWeaponThreatTargetDecision::Accepted;
	}

private:
	friend class FShanmenControlledWeaponExecution;
	friend class FShanmenControlledWeaponThreatPolicyReceipt;

	FShanmenHitCandidate Candidate;
	FGameplayTagContainer TargetTags;
	EShanmenControlledWeaponThreatTargetDecision Decision =
		EShanmenControlledWeaponThreatTargetDecision::RejectedMissingRequiredTags;
};

/**
 * Deterministic policy audit for one completed Orbit threat emission.
 *
 * It retains the canonical geometry receipt, the exact frozen target policy,
 * and one decision per target. It never produces damage or changes authority.
 */
class SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponThreatPolicyReceipt
{
public:
	bool IsValid() const;
	const FShanmenDetectorEmissionReceipt& GetEmission() const
	{
		return Emission;
	}
	const FGameplayTagContainer& GetRequiredTargetTags() const
	{
		return RequiredTargetTags;
	}
	bool RejectsSelf() const { return bRejectSelf; }
	const TArray<FShanmenControlledWeaponThreatTargetReceipt>& GetTargets() const
	{
		return Targets;
	}
	int32 NumAcceptedTargets() const;

private:
	friend class FShanmenControlledWeaponExecution;

	FShanmenDetectorEmissionReceipt Emission;
	FGameplayTagContainer RequiredTargetTags;
	bool bRejectSelf = false;
	TArray<FShanmenControlledWeaponThreatTargetReceipt> Targets;
};

/** One sample-scoped observation that a legal target is inside the threat envelope. */
class SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponThreatPresenceIntent
{
public:
	bool IsValid() const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	const FShanmenHitCandidate& GetCandidate() const { return Candidate; }

private:
	friend class FShanmenControlledWeaponExecution;
	friend class FShanmenControlledWeaponThreatPresenceReceipt;

	FGuid IntentId;
	FGuid RunId;
	FGuid SourceItemInstanceId;
	FShanmenHitCandidate Candidate;
};

/**
 * Deterministic, zero-magnitude output for one completed target-policy sample.
 * Consumers may deduplicate exact replay by IntentId; this receipt owns no
 * cross-sample cooldown, duration, damage, control, or World mutation.
 */
class SHANMENCOMBATRUNTIME_API FShanmenControlledWeaponThreatPresenceReceipt
{
public:
	bool IsValid() const;
	const FShanmenControlledWeaponThreatPolicyReceipt& GetPolicy() const
	{
		return Policy;
	}
	const TArray<FShanmenControlledWeaponThreatPresenceIntent>& GetIntents() const
	{
		return Intents;
	}

private:
	friend class FShanmenControlledWeaponExecution;

	FShanmenControlledWeaponThreatPolicyReceipt Policy;
	TArray<FShanmenControlledWeaponThreatPresenceIntent> Intents;
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
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenDetectorEmissionReceipt& OutReceipt);
	bool TryEndOrbitThreatEmission(
		const FShanmenActionOrchestrator& ActionRuntime);
	/** Applies only the already-frozen definition target policy to completed geometry. */
	bool TryEvaluateOrbitThreatReceipt(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
		FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const;
	/** Emits one deterministic presence intent per accepted target. */
	bool TryBuildOrbitThreatPresenceIntents(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
		FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const;
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
