#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

struct Fdemo_mapInputActionDefinition
{
	FName ActionId = NAME_None;
	FKey DefaultKey;
	bool bRequiresReleasedEvent = false;
	FString DisplayLabel;
	FString CategoryLabel;
};

struct Fdemo_mapInputActionIds
{
	static const FName MoveForward;
	static const FName MoveBackward;
	static const FName MoveLeft;
	static const FName MoveRight;
	static const FName PrimaryAttack;
	static const FName SkillGroundCircle;
	static const FName SkillSelfSector;
	static const FName SkillStraightProjectile;
	static const FName ControlledWeaponLaunchRecall;
	static const FName ControlledWeaponRedirect;
	static const FName ThrownWeaponTrajectoryToggle;
	static const FName ThrownWeaponArcTargetSet;
	static const FName ThrownWeaponArcApexIncrease;
	static const FName ThrownWeaponArcApexDecrease;
	static const FName ThrownWeaponArcTargetClear;
	static const FName SpiritEvasion;
	static const FName WeaponGuard;
	static const FName Interact;
	static const FName ResetRun;
	static const FName Inventory;
	static const FName Back;
	static const FName Hotbar1;
	static const FName Hotbar2;
	static const FName Hotbar3;
	static const FName Hotbar4;
	static const FName Hotbar5;
	static const FName Hotbar6;
	static const FName Hotbar7;
	static const FName Hotbar8;
	static const FName Hotbar9;
};

class Fdemo_mapInputActionRegistry
{
public:
	static const TArray<Fdemo_mapInputActionDefinition>& GetExactDefaultActions();
	static const Fdemo_mapInputActionDefinition* Find(FName ActionId);
	static FName HotbarActionId(int32 ExternalSlotNumber);
	static FString DisplayLabel(FName ActionId);
	static bool ValidateExactDefaults(FString* OutError = nullptr);
};
