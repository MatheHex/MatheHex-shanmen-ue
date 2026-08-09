#pragma once

#include "CoreMinimal.h"
#include "demo_mapSearchContainerTypes.h"

struct Fdemo_mapFixedLootTableDefinition
{
	FName TableId = NAME_None;
	Edemo_mapRuntimeContainerKind Kind = Edemo_mapRuntimeContainerKind::Chest;
	TArray<Fdemo_mapRuntimeContainerSeedEntry> Entries;
	int32 TotalPrototypeValue = 0;

	bool IsValid(FString* OutError = nullptr) const;
};
