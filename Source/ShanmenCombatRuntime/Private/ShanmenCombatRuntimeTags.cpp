#include "ShanmenCombatRuntimeTags.h"

#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Ability_Combat_Action, "Shanmen.Ability.Combat.Action");
UE_DEFINE_GAMEPLAY_TAG_STATIC(
	TAG_Shanmen_Ability_Combat_Action_Sword_Basic01,
	"Shanmen.Ability.Combat.Action.Sword.Basic01");

FGameplayTag FShanmenCombatRuntimeNativeTags::AbilityCombatAction()
{
	return TAG_Shanmen_Ability_Combat_Action;
}

FGameplayTag FShanmenCombatRuntimeNativeTags::AbilityCombatActionSwordBasic01()
{
	return TAG_Shanmen_Ability_Combat_Action_Sword_Basic01;
}
