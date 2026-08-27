#include "ShanmenCombatTags.h"

#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Damage, "Shanmen.Damage");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Damage_Physical, "Shanmen.Damage.Physical");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Damage_Physical_Slash, "Shanmen.Damage.Physical.Slash");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Damage_Spirit, "Shanmen.Damage.Spirit");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Damage_Mental, "Shanmen.Damage.Mental");

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Source_Player, "Shanmen.Source.Player");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Target_Living, "Shanmen.Target.Living");

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Defense_Evade, "Shanmen.Defense.Evade");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Defense_PerfectGuard, "Shanmen.Defense.PerfectGuard");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Defense_Guard, "Shanmen.Defense.Guard");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Defense_Shield, "Shanmen.Defense.Shield");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Defense_Armor, "Shanmen.Defense.Armor");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Defense_LethalIntercept, "Shanmen.Defense.LethalIntercept");

FGameplayTag FShanmenCombatNativeTags::Damage() { return TAG_Shanmen_Damage; }
FGameplayTag FShanmenCombatNativeTags::DamagePhysical() { return TAG_Shanmen_Damage_Physical; }
FGameplayTag FShanmenCombatNativeTags::DamagePhysicalSlash() { return TAG_Shanmen_Damage_Physical_Slash; }
FGameplayTag FShanmenCombatNativeTags::DamageSpirit() { return TAG_Shanmen_Damage_Spirit; }
FGameplayTag FShanmenCombatNativeTags::DamageMental() { return TAG_Shanmen_Damage_Mental; }

FGameplayTag FShanmenCombatNativeTags::SourcePlayer() { return TAG_Shanmen_Source_Player; }
FGameplayTag FShanmenCombatNativeTags::TargetLiving() { return TAG_Shanmen_Target_Living; }

FGameplayTag FShanmenCombatNativeTags::DefenseEvade() { return TAG_Shanmen_Defense_Evade; }
FGameplayTag FShanmenCombatNativeTags::DefensePerfectGuard() { return TAG_Shanmen_Defense_PerfectGuard; }
FGameplayTag FShanmenCombatNativeTags::DefenseGuard() { return TAG_Shanmen_Defense_Guard; }
FGameplayTag FShanmenCombatNativeTags::DefenseShield() { return TAG_Shanmen_Defense_Shield; }
FGameplayTag FShanmenCombatNativeTags::DefenseArmor() { return TAG_Shanmen_Defense_Armor; }
FGameplayTag FShanmenCombatNativeTags::DefenseLethalIntercept() { return TAG_Shanmen_Defense_LethalIntercept; }
