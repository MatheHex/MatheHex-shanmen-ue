#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcEditingPresentation.h"

/**
 * Immutable text projection of the configured Ballistic Arc edit controls.
 *
 * The caller supplies one P20.24 presentation snapshot and display labels read
 * from the unified input settings. This type owns no input setting, binding,
 * device event, choice state, revision, World, Actor, route, or mutable UI state.
 */
class Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& ArcPresentation,
		const FString& TargetKeyLabel,
		const FString& ApexIncreaseKeyLabel,
		const FString& ApexDecreaseKeyLabel,
		const FString& ClearKeyLabel,
		Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation& OutPresentation);

	bool IsValid() const;
	const FString& GetTargetKeyLabel() const { return TargetKeyLabel; }
	const FString& GetApexIncreaseKeyLabel() const { return ApexIncreaseKeyLabel; }
	const FString& GetApexDecreaseKeyLabel() const { return ApexDecreaseKeyLabel; }
	const FString& GetClearKeyLabel() const { return ClearKeyLabel; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	bool bCanIncreaseArcApex = false;
	bool bCanDecreaseArcApex = false;
	bool bCanClearArcTargetIntent = false;
	FString TargetKeyLabel;
	FString ApexIncreaseKeyLabel;
	FString ApexDecreaseKeyLabel;
	FString ClearKeyLabel;
	FString DisplayText;
};
