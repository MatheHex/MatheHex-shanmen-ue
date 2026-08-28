#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"

#include "ShanmenCombatGameplayAbility.generated.h"

/**
 * Thin GAS owner for Shanmen's deterministic action lifecycle.
 *
 * Derived abilities capture their authoritative ActionSnapshot, then use these
 * protected transition methods. GAS may schedule animation/tasks, but cannot
 * skip phase legality or move the commit point away from Startup -> Active.
 */
UCLASS(Abstract, Blueprintable)
class SHANMENCOMBATRUNTIME_API UShanmenCombatGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UShanmenCombatGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Shanmen|Combat|Action")
	EShanmenCombatActionPhase GetShanmenActionPhase() const { return ActionRuntime.GetPhase(); }

	UFUNCTION(BlueprintPure, Category = "Shanmen|Combat|Action")
	bool IsShanmenActionTerminal() const { return ActionRuntime.IsTerminal(); }

	UFUNCTION(BlueprintPure, Category = "Shanmen|Combat|Action")
	bool CanShanmenActionEmitCandidates() const { return ActionRuntime.CanEmitCandidates(); }

	const FShanmenActionOrchestrator& GetShanmenActionRuntime() const { return ActionRuntime; }

protected:
	bool TryStartShanmenAction(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionTransitionReceipt& OutReceipt);
	bool TryAdvanceShanmenAction(
		EShanmenCombatActionPhase ExpectedPhase,
		FShanmenActionTransitionReceipt& OutReceipt);
	bool TryCancelShanmenAction(
		EShanmenCombatActionPhase ExpectedPhase,
		FShanmenActionTransitionReceipt& OutReceipt);
	bool TryInterruptShanmenAction(
		EShanmenCombatActionPhase ExpectedPhase,
		FShanmenActionTransitionReceipt& OutReceipt);

	UFUNCTION(BlueprintImplementableEvent, Category = "Shanmen|Combat|Action", meta = (DisplayName = "On Shanmen Action Transition"))
	void K2_OnShanmenActionTransition(const FShanmenActionTransitionReceipt& Receipt);

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	void NotifyTransition(const FShanmenActionTransitionReceipt& Receipt);

	FShanmenActionOrchestrator ActionRuntime;
};
