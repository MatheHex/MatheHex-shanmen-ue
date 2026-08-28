#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenDetectorEmissionSession.h"

#include "ShanmenBasicSwordExecution.generated.h"

/** Authoring input; balance values are supplied by content, not hard-coded by the runtime. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenBasicSwordDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword")
	FName DetectorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword")
	FName FormulaId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword", meta = (ClampMin = "0.0"))
	float AttackPowerCoefficient = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword")
	FGameplayTagContainer DamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Sword")
	bool bRejectSelf = true;
};

/** Frozen definition for the first authored sword action. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenBasicSwordDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenBasicSwordDefinitionCapture& Capture,
		FShanmenBasicSwordDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetDetectorId() const { return DetectorId; }
	FName GetFormulaId() const { return FormulaId; }
	float GetBaseDamage() const { return BaseDamage; }
	float GetAttackPowerCoefficient() const { return AttackPowerCoefficient; }
	const FGameplayTagContainer& GetDamageTags() const { return DamageTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const { return RequiredTargetTags; }
	bool RejectsSelf() const { return bRejectSelf; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FName DetectorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FName FormulaId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	float BaseDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	float AttackPowerCoefficient = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer DamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	bool bRejectSelf = true;
};

/** Offensive values captured at activation before equipment can change. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenBasicSwordOffenseSnapshot
{
	GENERATED_BODY()

public:
	static bool TryCapture(float AttackPower, FShanmenBasicSwordOffenseSnapshot& OutSnapshot);
	bool IsValid() const;
	float GetAttackPower() const { return AttackPower; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	float AttackPower = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	bool bCaptured = false;
};

/** Auditable pure-kernel input and output for one accepted sword candidate. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenBasicSwordImpactReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class FShanmenBasicSwordExecution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Sword", meta = (AllowPrivateAccess = "true"))
	FShanmenImpactResult Result;
};

/**
 * Active-window execution for Combat.Action.Sword.Basic01.
 *
 * The P3 action runtime supplies phase authority, P2 supplies candidates, and
 * callers inject target vitality/defense snapshots. This class owns target
 * policy, formula construction, emission dedupe and per-activation ImpactIds.
 */
class SHANMENCOMBATRUNTIME_API FShanmenBasicSwordExecution
{
public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenBasicSwordDefinition& Definition,
		const FShanmenBasicSwordOffenseSnapshot& Offense,
		FShanmenBasicSwordExecution& OutExecution);

	bool IsValid() const;
	bool TryBeginEmission(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWorldHitContext& OutContext);
	bool TryResolveCandidate(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenHitCandidate& Candidate,
		const FShanmenTargetVitalitySnapshot& TargetVitality,
		const FShanmenDefenseSnapshot& Defense,
		FShanmenBasicSwordImpactReceipt& OutReceipt);
	bool TryEndEmission(const FShanmenActionOrchestrator& ActionRuntime);
	void EndEmissionForTermination();
	void Reset();

	bool IsEmissionActive() const { return EmissionSession.IsEmissionActive(); }
	int32 NumAcceptedImpacts() const { return ImpactLedger.Num(); }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenBasicSwordDefinition& GetDefinition() const { return Definition; }

private:
	bool MatchesActionRuntime(const FShanmenActionOrchestrator& ActionRuntime) const;
	bool IsTargetAllowed(
		const FShanmenHitCandidate& Candidate,
		const FShanmenDefenseSnapshot& Defense) const;
	bool TryBuildDamagePacket(FShanmenDamagePacket& OutPacket) const;

	FShanmenCombatActionSnapshot Action;
	FShanmenBasicSwordDefinition Definition;
	FShanmenBasicSwordOffenseSnapshot Offense;
	FShanmenDetectorEmissionSession EmissionSession;
	FShanmenImpactLedger ImpactLedger;
};
