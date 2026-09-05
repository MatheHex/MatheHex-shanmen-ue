#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"

/**
 * Immutable, device-neutral text projection of the current thrown trajectory.
 *
 * The projection deliberately owns no target, apex, revision, session, World,
 * Actor, input routing, or mutable choice state. Presentation code refreshes it
 * from one current P20.20 read and supplies the configured toggle-key label.
 */
class Fdemo_mapShanmenThrownWeaponTrajectoryPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
		const FString& ToggleKeyLabel,
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& OutPresentation);

	bool IsValid() const;
	bool IsBallisticArc() const;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	const FString& GetModeLabel() const { return ModeLabel; }
	const FString& GetToggleKeyLabel() const { return ToggleKeyLabel; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	FString ModeLabel;
	FString ToggleKeyLabel;
	FString DisplayText;
};
