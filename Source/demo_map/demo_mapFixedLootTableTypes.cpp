#include "demo_mapFixedLootTableTypes.h"
#include "demo_mapItemDefinitions.h"

bool Fdemo_mapFixedLootTableDefinition::IsValid(FString* OutError) const
{
	TSet<uint64> OccupiedSlots;
	int32 ComputedValue = 0;
	for (const Fdemo_mapRuntimeContainerSeedEntry& Entry : Entries)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Entry.DefinitionId);
		const uint64 SlotKey =
			(static_cast<uint64>(Entry.Section) << 32)
			| static_cast<uint32>(Entry.SlotIndex);
		if (!Definition
			|| Entry.StackCount <= 0
			|| Entry.StackCount > Definition->MaxStackSize
			|| Entry.SlotIndex < 0
			|| OccupiedSlots.Contains(SlotKey))
		{
			if (OutError) *OutError = TEXT("Fixed loot entry has an invalid definition, quantity, or duplicate slot.");
			return false;
		}
		OccupiedSlots.Add(SlotKey);
		ComputedValue += static_cast<int32>(
			Definition->PrototypeValue * Entry.StackCount);
	}
	const bool bValid = !TableId.IsNone()
		&& !Entries.IsEmpty()
		&& ComputedValue == TotalPrototypeValue;
	if (!bValid && OutError)
	{
		*OutError = FString::Printf(
			TEXT("Fixed loot table value mismatch expected=%d actual=%d."),
			TotalPrototypeValue,
			ComputedValue);
	}
	return bValid;
}
