#include "demo_mapRewardGenerationTypes.h"

bool Fdemo_mapRewardBudgetProfile::IsValid() const
{
	return !ProfileId.IsNone()
		&& PlannedSourceCount > 0
		&& BaseValue > 0
		&& MinMultiplierBps > 0
		&& MaxMultiplierBps >= MinMultiplierBps;
}

bool Fdemo_mapRewardPoolEntry::IsValid() const
{
	return !EntryId.IsNone()
		&& !DefinitionId.IsNone()
		&& Weight > 0
		&& MinStack > 0
		&& MaxStack >= MinStack
		&& MinItemLevel >= 0
		&& MaxItemLevel >= MinItemLevel
		&& MinUnitValue > 0
		&& MaxUnitValue >= MinUnitValue
		&& !ItemTags.IsEmpty()
		&& !ItemTags.Contains(NAME_None)
		&& !RequiredSourceTags.Contains(NAME_None)
		&& !ExcludedSourceTags.Contains(NAME_None);
}

bool Fdemo_mapRewardSourceDefinition::IsValid() const
{
	if (MarkerId.IsNone() || RewardSourceId.IsNone() || Capacity <= 0)
	{
		return false;
	}
	if (Mode == Edemo_mapRewardSourceMode::FixedTable)
	{
		return !FixedTableId.IsNone()
			&& BudgetProfileId.IsNone()
			&& SourceTags.IsEmpty();
	}
	return !BudgetProfileId.IsNone()
		&& !SourceTags.IsEmpty()
		&& FixedTableId.IsNone()
		&& (!bAllowFixedFallbackOnFailure
			|| !FixedFallbackTableId.IsNone());
}
