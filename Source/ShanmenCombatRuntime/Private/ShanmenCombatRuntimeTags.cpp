#include "ShanmenCombatRuntimeTags.h"

#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Ability_Combat_Action, "Shanmen.Ability.Combat.Action");

FGameplayTag FShanmenCombatRuntimeNativeTags::AbilityCombatAction()
{
	return TAG_Shanmen_Ability_Combat_Action;
}
