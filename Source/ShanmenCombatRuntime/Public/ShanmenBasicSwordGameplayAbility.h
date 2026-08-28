#pragma once

#include "CoreMinimal.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatGameplayAbility.h"

#include "ShanmenBasicSwordGameplayAbility.generated.h"

/**
 * GAS family base for Combat.Action.Sword.Basic01.
 *
 * It remains abstract until P3.2 provides the product-side source snapshot and
 * animation/task adapter. The deterministic vertical slice is already owned by
 * BasicSwordExecution and the inherited action phase runtime.
 */
UCLASS(Abstract, Blueprintable)
class SHANMENCOMBATRUNTIME_API UShanmenBasicSwordGameplayAbility : public UShanmenCombatGameplayAbility
{
	GENERATED_BODY()

public:
	UShanmenBasicSwordGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	const FShanmenBasicSwordExecution& GetBasicSwordExecution() const { return SwordExecution; }

protected:
	bool TryPrepareBasicSword(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenBasicSwordDefinition& Definition,
		const FShanmenBasicSwordOffenseSnapshot& Offense,
		FShanmenActionTransitionReceipt& OutReceipt);
	bool TryBeginBasicSwordEmission(FShanmenWorldHitContext& OutContext);
	bool TryResolveBasicSwordCandidate(
		const FShanmenHitCandidate& Candidate,
		const FShanmenTargetVitalitySnapshot& TargetVitality,
		const FShanmenDefenseSnapshot& Defense,
		FShanmenBasicSwordImpactReceipt& OutReceipt);
	bool TryEndBasicSwordEmission();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	FShanmenBasicSwordExecution SwordExecution;
};
