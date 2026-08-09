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

namespace
{
	Fdemo_mapRuntimeContainerSeedEntry Entry(
		Edemo_mapRuntimeContainerSection Section,
		int32 Slot,
		FName Definition,
		int32 Quantity)
	{
		Fdemo_mapRuntimeContainerSeedEntry Result;
		Result.Section = Section;
		Result.SlotIndex = Slot;
		Result.DefinitionId = Definition;
		Result.StackCount = Quantity;
		return Result;
	}

	Fdemo_mapFixedLootTableDefinition Table(
		FName Id,
		Edemo_mapRuntimeContainerKind Kind,
		std::initializer_list<Fdemo_mapRuntimeContainerSeedEntry> Entries,
		int32 Total)
	{
		Fdemo_mapFixedLootTableDefinition Result;
		Result.TableId = Id;
		Result.Kind = Kind;
		for (const Fdemo_mapRuntimeContainerSeedEntry& Seed : Entries)
		{
			Result.Entries.Add(Seed);
		}
		Result.TotalPrototypeValue = Total;
		return Result;
	}
}

const TArray<Fdemo_mapFixedLootTableDefinition>&
Fdemo_mapFixedLootTableRegistry::GetAll()
{
	using Section = Edemo_mapRuntimeContainerSection;
	using Kind = Edemo_mapRuntimeContainerKind;
	static const TArray<Fdemo_mapFixedLootTableDefinition> Tables = {
		Table(Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard, Kind::Corpse, {
			Entry(Section::Equipment, 0, Fdemo_mapItemIds::WeaponLevel1, 1),
			Entry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritOreLevel1, 2),
			Entry(Section::Body, 0, Fdemo_mapItemIds::SoulBone, 1)}, 180),
		Table(Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy, Kind::Corpse, {
			Entry(Section::Equipment, 0, Fdemo_mapItemIds::ArmorRobeLevel1, 1),
			Entry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritWoodLevel1, 2),
			Entry(Section::Body, 0, Fdemo_mapItemIds::SoulBone, 1)}, 170),
		Table(Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard, Kind::Corpse, {
			Entry(Section::Equipment, 0, Fdemo_mapItemIds::AccessoryLevel1, 1),
			Entry(Section::Backpack, 0, Fdemo_mapItemIds::HealingPillLevel1, 1),
			Entry(Section::Body, 0, Fdemo_mapItemIds::SoulBone, 1)}, 155),
		Table(Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced, Kind::Corpse, {
			Entry(Section::Equipment, 0, Fdemo_mapItemIds::WeaponLevel2, 1),
			Entry(Section::Equipment, 1, Fdemo_mapItemIds::BackpackLevel2, 1),
			Entry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritOreLevel2, 2),
			Entry(Section::Body, 0, Fdemo_mapItemIds::SpiritBone, 1),
			Entry(Section::Body, 1, Fdemo_mapItemIds::InnerCoreLevel10, 1)}, 970),
		Table(Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced, Kind::Corpse, {
			Entry(Section::Equipment, 0, Fdemo_mapItemIds::ArmorRobeLevel2, 1),
			Entry(Section::Equipment, 1, Fdemo_mapItemIds::AccessoryLevel2, 1),
			Entry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritWoodLevel3, 2),
			Entry(Section::Body, 0, Fdemo_mapItemIds::SpiritBone, 1),
			Entry(Section::Body, 1, Fdemo_mapItemIds::InnerCoreLevel10, 1)}, 950),
		Table(Fdemo_mapFixedLootTableIds::ChestMainA, Kind::Chest, {
			Entry(Section::Chest, 0, Fdemo_mapItemIds::SpiritWoodLevel1, 2),
			Entry(Section::Chest, 1, Fdemo_mapItemIds::HealingPillLevel1, 1)}, 35),
		Table(Fdemo_mapFixedLootTableIds::ChestMainB, Kind::Chest, {
			Entry(Section::Chest, 0, Fdemo_mapItemIds::SpiritOreLevel1, 2),
			Entry(Section::Chest, 1, Fdemo_mapItemIds::HealingPillLevel1, 1)}, 45),
		Table(Fdemo_mapFixedLootTableIds::ChestSideA, Kind::Chest, {
			Entry(Section::Chest, 0, Fdemo_mapItemIds::SpiritWoodLevel2, 2),
			Entry(Section::Chest, 1, Fdemo_mapItemIds::SpiritOreLevel2, 2),
			Entry(Section::Chest, 2, Fdemo_mapItemIds::HealingPillLevel2, 1)}, 150)
	};
	return Tables;
}

const Fdemo_mapFixedLootTableDefinition*
Fdemo_mapFixedLootTableRegistry::Find(FName TableId)
{
	return GetAll().FindByPredicate(
		[TableId](const Fdemo_mapFixedLootTableDefinition& Table)
		{
			return Table.TableId == TableId;
		});
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
