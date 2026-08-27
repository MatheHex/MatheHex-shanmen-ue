#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

struct SHANMENITEMS_API FShanmenItemNativeTags
{
	static FGameplayTag CapabilityConsumeQuantity();
	static FGameplayTag CapabilityDeploy();
	static FGameplayTag CapabilityDurability();
	static FGameplayTag CapabilityCharges();

	static FGameplayTag ItemWeaponFlyingSword();
	static FGameplayTag ItemWeaponThrown();
	static FGameplayTag ItemFormationMaterial();
	static FGameplayTag ItemArtifactLethalGuard();
};
