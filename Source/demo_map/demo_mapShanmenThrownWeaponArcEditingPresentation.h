#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"

/**
 * Immutable, device-neutral presentation of the current Ballistic Arc edits.
 *
 * The projection exposes only the canonical target/apex values and operations
 * already authorized by one P20.20 read. It owns no key, device, input route,
 * revision, session, World, Actor, command, intent, or mutable choice state.
 */
class Fdemo_mapShanmenThrownWeaponArcEditingPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
		Fdemo_mapShanmenThrownWeaponArcEditingPresentation& OutPresentation);

	bool IsValid() const;
	bool HasArcTargetIntent() const { return bHasArcTargetIntent; }
	const FVector2D& GetArcTargetIntent() const { return ArcTargetIntent; }
	double GetArcApexAdjustment() const { return ArcApexAdjustment; }
	bool CanSetArcTargetIntent() const { return bCanSetArcTargetIntent; }
	bool CanIncreaseArcApex() const { return bCanIncreaseArcApex; }
	bool CanDecreaseArcApex() const { return bCanDecreaseArcApex; }
	bool CanClearArcTargetIntent() const { return bCanClearArcTargetIntent; }
	const FString& GetTargetDisplayText() const { return TargetDisplayText; }
	const FString& GetApexDisplayText() const { return ApexDisplayText; }

private:
	bool bHasArcTargetIntent = false;
	FVector2D ArcTargetIntent = FVector2D::ZeroVector;
	double ArcApexAdjustment = 0.0;
	bool bCanSetArcTargetIntent = false;
	bool bCanIncreaseArcApex = false;
	bool bCanDecreaseArcApex = false;
	bool bCanClearArcTargetIntent = false;
	FString TargetDisplayText;
	FString ApexDisplayText;
};
