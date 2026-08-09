#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapProfileStartupMode : uint8
{
	Disconnected,
	LegacyAutomation,
	ProfileAutomation,
	ProductionProfile
};

struct Fdemo_mapProfileStartupInputs
{
	bool bIsV3World = false;
	bool bIsEditorToolWorld = false;
	bool bLegacyAutomationRequested = false;
	bool bProfileAutomationRequested = false;
};

/** Pure startup decision. It performs no Profile, Runtime, world, UI, or disk work. */
struct Fdemo_mapProfileStartupModeSelector
{
	static Edemo_mapProfileStartupMode Select(const Fdemo_mapProfileStartupInputs& Inputs);
	static bool UsesProfilePreparation(Edemo_mapProfileStartupMode Mode);
	static bool UsesLegacyRuntimeBegin(Edemo_mapProfileStartupMode Mode);
};
