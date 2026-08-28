#include "ShanmenCombatGameplayAbility.h"

#include "ShanmenCombatRuntimeTags.h"

UShanmenCombatGameplayAbility::UShanmenCombatGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FGameplayTagContainer Tags;
	Tags.AddTag(FShanmenCombatRuntimeNativeTags::AbilityCombatAction());
	SetAssetTags(Tags);
}

bool UShanmenCombatGameplayAbility::TryStartShanmenAction(
	const FShanmenCombatActionSnapshot& Action,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	if (ActionRuntime.IsValid()
		|| !FShanmenActionOrchestrator::TryStart(Action, ActionRuntime, OutReceipt))
	{
		return false;
	}

	NotifyTransition(OutReceipt);
	return true;
}

bool UShanmenCombatGameplayAbility::TryAdvanceShanmenAction(
	EShanmenCombatActionPhase ExpectedPhase,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	if (!ActionRuntime.TryAdvance(ExpectedPhase, OutReceipt))
	{
		return false;
	}

	NotifyTransition(OutReceipt);
	return true;
}

bool UShanmenCombatGameplayAbility::TryCancelShanmenAction(
	EShanmenCombatActionPhase ExpectedPhase,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	if (!ActionRuntime.TryCancel(ExpectedPhase, OutReceipt))
	{
		return false;
	}

	NotifyTransition(OutReceipt);
	return true;
}

bool UShanmenCombatGameplayAbility::TryInterruptShanmenAction(
	EShanmenCombatActionPhase ExpectedPhase,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	if (!ActionRuntime.TryInterrupt(ExpectedPhase, OutReceipt))
	{
		return false;
	}

	NotifyTransition(OutReceipt);
	return true;
}

void UShanmenCombatGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActionRuntime.IsValid() && !ActionRuntime.IsTerminal())
	{
		FShanmenActionTransitionReceipt Receipt;
		const EShanmenCombatActionPhase Phase = ActionRuntime.GetPhase();
		const bool bTransitioned = bWasCancelled && Phase == EShanmenCombatActionPhase::Startup
			? ActionRuntime.TryCancel(Phase, Receipt)
			: ActionRuntime.TryInterrupt(Phase, Receipt);
		if (bTransitioned)
		{
			NotifyTransition(Receipt);
		}
	}

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UShanmenCombatGameplayAbility::NotifyTransition(
	const FShanmenActionTransitionReceipt& Receipt)
{
	check(Receipt.IsValid());
	K2_OnShanmenActionTransition(Receipt);
}
