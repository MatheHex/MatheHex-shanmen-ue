#include "ShanmenItemTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Capability_ConsumeQuantity, "Shanmen.Item.Capability.ConsumeQuantity");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Capability_Deploy, "Shanmen.Item.Capability.Deploy");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Capability_Durability, "Shanmen.Item.Capability.Durability");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Capability_Charges, "Shanmen.Item.Capability.Charges");

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Weapon_FlyingSword, "Shanmen.Item.Weapon.FlyingSword");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Weapon_Thrown, "Shanmen.Item.Weapon.Thrown");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Formation_Material, "Shanmen.Item.Formation.Material");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Shanmen_Item_Artifact_LethalGuard, "Shanmen.Item.Artifact.LethalGuard");

FGameplayTag FShanmenItemNativeTags::CapabilityConsumeQuantity() { return TAG_Shanmen_Item_Capability_ConsumeQuantity; }
FGameplayTag FShanmenItemNativeTags::CapabilityDeploy() { return TAG_Shanmen_Item_Capability_Deploy; }
FGameplayTag FShanmenItemNativeTags::CapabilityDurability() { return TAG_Shanmen_Item_Capability_Durability; }
FGameplayTag FShanmenItemNativeTags::CapabilityCharges() { return TAG_Shanmen_Item_Capability_Charges; }
FGameplayTag FShanmenItemNativeTags::ItemWeaponFlyingSword() { return TAG_Shanmen_Item_Weapon_FlyingSword; }
FGameplayTag FShanmenItemNativeTags::ItemWeaponThrown() { return TAG_Shanmen_Item_Weapon_Thrown; }
FGameplayTag FShanmenItemNativeTags::ItemFormationMaterial() { return TAG_Shanmen_Item_Formation_Material; }
FGameplayTag FShanmenItemNativeTags::ItemArtifactLethalGuard() { return TAG_Shanmen_Item_Artifact_LethalGuard; }
