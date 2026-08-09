#include "demo_mapProfileStartupMode.h"

Edemo_mapProfileStartupMode Fdemo_mapProfileStartupModeSelector::Select(
	const Fdemo_mapProfileStartupInputs& Inputs)
{
	if (!Inputs.bIsV3World || Inputs.bIsEditorToolWorld)
	{
		return Edemo_mapProfileStartupMode::Disconnected;
	}
	if (Inputs.bLegacyAutomationRequested)
	{
		return Edemo_mapProfileStartupMode::LegacyAutomation;
	}
#if !UE_BUILD_SHIPPING
	if (Inputs.bProfileAutomationRequested)
	{
		return Edemo_mapProfileStartupMode::ProfileAutomation;
	}
#endif
	return Edemo_mapProfileStartupMode::ProductionProfile;
}

bool Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(
	Edemo_mapProfileStartupMode Mode)
{
	return Mode == Edemo_mapProfileStartupMode::ProfileAutomation
		|| Mode == Edemo_mapProfileStartupMode::ProductionProfile;
}

bool Fdemo_mapProfileStartupModeSelector::UsesLegacyRuntimeBegin(
	Edemo_mapProfileStartupMode Mode)
{
	return Mode == Edemo_mapProfileStartupMode::LegacyAutomation;
}
