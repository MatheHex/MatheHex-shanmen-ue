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

/** Single fixed-table authority used by all P7 Corpses and Chests. */
struct Fdemo_mapFixedLootTableRegistry
{
	static const TArray<Fdemo_mapFixedLootTableDefinition>& GetAll();
	static const Fdemo_mapFixedLootTableDefinition* Find(FName TableId);
	static FName GetChestTableId(int32 ChestIndex);
	static FName GetChestMarkerId(int32 ChestIndex);
	static bool Validate(FString* OutError = nullptr);
};
