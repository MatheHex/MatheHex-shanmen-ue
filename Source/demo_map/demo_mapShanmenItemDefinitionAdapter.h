#pragma once

#include "ShanmenItemTypes.h"

/** Canonical product definition -> authority capabilities. Never grants inventory. */
struct Fdemo_mapShanmenItemDefinitionAdapter
{
	static bool Build(FName DefinitionId, FShanmenItemDefinition& OutDefinition);
};
