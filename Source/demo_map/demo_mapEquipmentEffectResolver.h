#pragma once

#include "CoreMinimal.h"
#include "demo_mapItemTypes.h"

struct Fdemo_mapEquipmentEffectResolution
{
	bool bSuccess = false;
	TArray<Fdemo_mapModifierSpec> Modifiers;
	FString Diagnostic;
};

/** Generic Effect Key / Category / Slot bridge into the existing Attribute authority. */
struct Fdemo_mapEquipmentEffectResolver
{
	static Fdemo_mapEquipmentEffectResolution Resolve(
		const Fdemo_mapItemDefinition& Definition,
		FName EquippedSlotId);
	static Fdemo_mapEquipmentEffectResolution ResolveAffixes(
		const Fdemo_mapItemDefinition& Definition,
		const Fdemo_mapRewardAffixSet& AffixSet);
};
