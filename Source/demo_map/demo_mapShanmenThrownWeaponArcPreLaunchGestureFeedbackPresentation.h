#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreLaunchPreviewContext.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentation.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"

enum class Edemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackMode : uint8
{
	Invalid,
	TargetRequired,
	ReadyToConfirm
};

/**
 * Immutable MainHUD text projection of one armed Arc pre-launch gesture.
 *
 * The projection reads the sole Run context, revisionless choice read and
 * physical preview cursor. It owns no selection, input binding, choice,
 * presentation cursor, item, launch, World, Actor or mutable UI state.
 */
class Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext& Context,
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult&
			ChoiceRead,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PhysicalPreviewCursor,
		const FString& HotbarKeyLabel,
		Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation&
			OutPresentation);

	bool IsValid() const;
	bool NeedsArcTarget() const;
	bool IsReadyToConfirm() const;
	Edemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackMode GetMode() const
	{
		return Mode;
	}
	const FGuid& GetRunId() const { return RunId; }
	int32 GetArmedHotbarSlotNumber() const
	{
		return ArmedHotbarSlotNumber;
	}
	uint64 GetContextRevision() const { return ContextRevision; }
	const FString& GetHotbarKeyLabel() const { return HotbarKeyLabel; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackMode Mode =
		Edemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackMode::Invalid;
	FGuid RunId;
	int32 ArmedHotbarSlotNumber = INDEX_NONE;
	uint64 ContextRevision = 0;
	FString HotbarKeyLabel;
	FString DisplayText;
};
