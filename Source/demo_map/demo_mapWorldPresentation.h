#pragma once

#include "CoreMinimal.h"

class UTextRenderComponent;

/** Shared deterministic presentation rules for fixed-camera world labels. */
struct Fdemo_mapWorldPresentation
{
	/** Returns true when a live PlayerCameraManager supplied the orientation. */
	static bool FaceLabelToCamera(UTextRenderComponent* Label);
	static FText MakeItemWorldLabel(const FString& StableWorldName, int32 Quantity);
};
