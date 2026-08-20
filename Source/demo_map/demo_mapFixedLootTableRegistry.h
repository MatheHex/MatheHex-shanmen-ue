#pragma once

#include "CoreMinimal.h"
#include "demo_mapFixedLootTableTypes.h"

struct Fdemo_mapFixedLootTableIds
{
	static const FName CorpseMainMeleeStandard;
	static const FName CorpseMainMeleeHeavy;
	static const FName CorpseMainRangedStandard;
	static const FName CorpseSideMeleeEnhanced;
	static const FName CorpseSideRangedEnhanced;
	static const FName ChestMainA;
	static const FName ChestMainB;
	static const FName ChestSideA;
	static const FName MarkerChestMainA;
	static const FName MarkerChestMainB;
	static const FName MarkerChestSideA;
};

/**
 * Legacy P7 lookup surface. Profile data lives only in the P73 Code B content
 * manifest (Fdemo_mapItemDefinitions); this type preserves stable profile IDs.
 */
struct Fdemo_mapFixedLootTableRegistry
{
	static const TArray<Fdemo_mapFixedLootTableDefinition>& GetAll();
	static const Fdemo_mapFixedLootTableDefinition* Find(FName TableId);
	static FName GetChestTableId(int32 ChestIndex);
	static FName GetChestMarkerId(int32 ChestIndex);
	static bool Validate(FString* OutError = nullptr);
};
