#pragma once

#include "CoreMinimal.h"
#include "demo_mapSearchContainerTypes.h"

struct Fdemo_mapFixedLootTableDefinition
{
	FName TableId = NAME_None;
	/** Stable P73 content-manifest identity; checked before any materialization. */
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
	Edemo_mapRuntimeContainerKind Kind = Edemo_mapRuntimeContainerKind::Chest;
	/** Deterministic profiles use CandidateWeight=1 and NoDropWeight=0. */
	int32 CandidateWeight = 0;
	int32 NoDropWeight = 0;
	/** Optional exact equipment target retained by corpse/profile receipts. */
	FName FixedEquipmentDefinitionId = NAME_None;
	TArray<Fdemo_mapRuntimeContainerSeedEntry> Entries;
	int32 TotalPrototypeValue = 0;

	bool IsValid(FString* OutError = nullptr) const;
};
