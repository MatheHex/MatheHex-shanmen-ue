#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"

const FName Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard(TEXT("P7.Loot.Corpse.Main.Melee.Standard"));
const FName Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy(TEXT("P7.Loot.Corpse.Main.Melee.Heavy"));
const FName Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard(TEXT("P7.Loot.Corpse.Main.Ranged.Standard"));
const FName Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced(TEXT("P7.Loot.Corpse.Side.Melee.Enhanced"));
const FName Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced(TEXT("P7.Loot.Corpse.Side.Ranged.Enhanced"));
const FName Fdemo_mapFixedLootTableIds::ChestMainA(TEXT("P7.Loot.Chest.Main.A"));
const FName Fdemo_mapFixedLootTableIds::ChestMainB(TEXT("P7.Loot.Chest.Main.B"));
const FName Fdemo_mapFixedLootTableIds::ChestSideA(TEXT("P7.Loot.Chest.Side.A"));
const FName Fdemo_mapFixedLootTableIds::MarkerChestMainA(TEXT("P7.Marker.Chest.Main.A"));
const FName Fdemo_mapFixedLootTableIds::MarkerChestMainB(TEXT("P7.Marker.Chest.Main.B"));
const FName Fdemo_mapFixedLootTableIds::MarkerChestSideA(TEXT("P7.Marker.Chest.Side.A"));

const TArray<Fdemo_mapFixedLootTableDefinition>&
Fdemo_mapFixedLootTableRegistry::GetAll()
{
	return Fdemo_mapItemDefinitions::GetFixedLootProfiles();
}

const Fdemo_mapFixedLootTableDefinition*
Fdemo_mapFixedLootTableRegistry::Find(FName TableId)
{
	return Fdemo_mapItemDefinitions::FindFixedLootProfile(TableId);
}

FName Fdemo_mapFixedLootTableRegistry::GetChestTableId(int32 ChestIndex)
{
	switch (ChestIndex)
	{
	case 0: return Fdemo_mapFixedLootTableIds::ChestMainA;
	case 1: return Fdemo_mapFixedLootTableIds::ChestMainB;
	case 2: return Fdemo_mapFixedLootTableIds::ChestSideA;
	default: return NAME_None;
	}
}

FName Fdemo_mapFixedLootTableRegistry::GetChestMarkerId(int32 ChestIndex)
{
	switch (ChestIndex)
	{
	case 0: return Fdemo_mapFixedLootTableIds::MarkerChestMainA;
	case 1: return Fdemo_mapFixedLootTableIds::MarkerChestMainB;
	case 2: return Fdemo_mapFixedLootTableIds::MarkerChestSideA;
	default: return NAME_None;
	}
}

bool Fdemo_mapFixedLootTableRegistry::Validate(FString* OutError)
{
	const TArray<Fdemo_mapFixedLootTableDefinition>& Tables = GetAll();
	TSet<FName> Ids;
	int32 Corpses = 0;
	int32 Chests = 0;
	for (const Fdemo_mapFixedLootTableDefinition& TableDefinition : Tables)
	{
		if (Ids.Contains(TableDefinition.TableId)
			|| !TableDefinition.IsValid(OutError))
		{
			if (OutError && OutError->IsEmpty())
			{
				*OutError = TEXT("Duplicate fixed loot table id.");
			}
			return false;
		}
		Ids.Add(TableDefinition.TableId);
		if (TableDefinition.Kind == Edemo_mapRuntimeContainerKind::Corpse) ++Corpses;
		else ++Chests;
	}
	if (Tables.Num() != 8 || Corpses != 5 || Chests != 3)
	{
		if (OutError) *OutError = TEXT("Fixed registry must contain exactly five Corpse and three Chest tables.");
		return false;
	}
	return true;
}
