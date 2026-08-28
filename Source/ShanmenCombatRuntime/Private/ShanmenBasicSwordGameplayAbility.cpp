#include "ShanmenBasicSwordGameplayAbility.h"

#include "ShanmenCombatRuntimeTags.h"

UShanmenBasicSwordGameplayAbility::UShanmenBasicSwordGameplayAbility(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(FShanmenCombatRuntimeNativeTags::AbilityCombatActionSwordBasic01());
	SetAssetTags(Tags);
}

bool UShanmenBasicSwordGameplayAbility::TryPrepareBasicSword(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenBasicSwordDefinition& Definition,
	const FShanmenBasicSwordOffenseSnapshot& Offense,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	FShanmenBasicSwordExecution PreparedExecution;
	if (GetShanmenActionRuntime().IsValid()
		|| !FShanmenBasicSwordExecution::TryCreate(
			Action,
			Definition,
			Offense,
			PreparedExecution))
	{
		return false;
	}

	SwordExecution = MoveTemp(PreparedExecution);
	if (!TryStartShanmenAction(Action, OutReceipt))
	{
		SwordExecution.Reset();
		return false;
	}
	return true;
}

bool UShanmenBasicSwordGameplayAbility::TryBeginBasicSwordEmission(
	FShanmenWorldHitContext& OutContext)
{
	return SwordExecution.TryBeginEmission(GetShanmenActionRuntime(), OutContext);
}

bool UShanmenBasicSwordGameplayAbility::TryResolveBasicSwordCandidate(
	const FShanmenHitCandidate& Candidate,
	const FShanmenTargetVitalitySnapshot& TargetVitality,
	const FShanmenDefenseSnapshot& Defense,
	FShanmenBasicSwordImpactReceipt& OutReceipt)
{
	return SwordExecution.TryResolveCandidate(
		GetShanmenActionRuntime(),
		Candidate,
		TargetVitality,
		Defense,
		OutReceipt);
}

bool UShanmenBasicSwordGameplayAbility::TryEndBasicSwordEmission()
{
	return SwordExecution.TryEndEmission(GetShanmenActionRuntime());
}

void UShanmenBasicSwordGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	SwordExecution.EndEmissionForTermination();
	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}
