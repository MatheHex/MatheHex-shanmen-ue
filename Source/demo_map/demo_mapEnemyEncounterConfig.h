#pragma once

#include "CoreMinimal.h"
#include "demo_mapEnemyEncounterTypes.h"

struct Fdemo_mapEnemyEncounterIds
{
	static const FName MainRoute;
	static const FName SideMeleeRoute;
	static const FName SideRangedRoute;
	static const FName MainMeleeStandard;
	static const FName MainMeleeHeavy;
	static const FName MainRangedStandard;
	static const FName SideMeleeEnhanced;
	static const FName SideRangedEnhanced;
	static const FName MarkerMainMeleeStandard;
	static const FName MarkerMainMeleeHeavy;
	static const FName MarkerMainRangedStandard;
	static const FName MarkerSideMeleeEnhanced;
	static const FName MarkerSideRangedEnhanced;
};

/** Single immutable authority for the P7 five-enemy route composition. */
struct Fdemo_mapEnemyEncounterConfig
{
	static const TArray<Fdemo_mapEnemyEncounterSpawnRecord>& GetSpawnRecords();
	static const Fdemo_mapEnemyEncounterSpawnRecord* Find(FName EncounterId);
	static bool Validate(FString* OutError = nullptr);
};
