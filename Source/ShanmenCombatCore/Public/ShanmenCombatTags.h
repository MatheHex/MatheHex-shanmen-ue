#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/** Native tags owned by CombatCore; public access is through exported value-returning functions. */
struct SHANMENCOMBATCORE_API FShanmenCombatNativeTags
{
	static FGameplayTag Damage();
	static FGameplayTag DamagePhysical();
	static FGameplayTag DamagePhysicalSlash();
	static FGameplayTag DamageSpirit();
	static FGameplayTag DamageMental();

	static FGameplayTag SourcePlayer();
	static FGameplayTag TargetLiving();

	static FGameplayTag Influence();
	static FGameplayTag InfluenceOffense();
	static FGameplayTag InfluenceOffensePower();
	static FGameplayTag InfluenceDefense();
	static FGameplayTag InfluenceDefenseGuard();

	static FGameplayTag DefenseEvade();
	static FGameplayTag DefensePerfectGuard();
	static FGameplayTag DefenseGuard();
	static FGameplayTag DefenseShield();
	static FGameplayTag DefenseArmor();
	static FGameplayTag DefenseLethalIntercept();
};
