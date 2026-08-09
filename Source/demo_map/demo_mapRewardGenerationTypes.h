#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffixTypes.h"
#include "demo_mapRewardEventTypes.h"
#include "demo_mapSearchContainerTypes.h"

enum class Edemo_mapRewardSourceMode : uint8
{
	FixedTable,
	GeneratedReward
};

enum class Edemo_mapRewardGenerationStatus : uint8
{
	Success,
	InvalidRequest,
	InvalidBudgetProfile,
	InvalidPool,
	MissingDefinition,
	InvalidUnitValue,
	NoEligibleItem,
	BudgetTooSmall,
	ArithmeticOverflow,
	CapacityLimited,
	DuplicateSource,
	MaterializationFailed
};

struct Fdemo_mapRewardBudgetProfile
{
	FName ProfileId = NAME_None;
	int32 PlannedSourceCount = 0;
	int64 BaseValue = 0;
	int32 MinMultiplierBps = 10000;
	int32 MaxMultiplierBps = 10000;

	bool IsValid() const;
};

struct Fdemo_mapRewardPoolEntry
{
	FName EntryId = NAME_None;
	FName DefinitionId = NAME_None;
	TArray<FName> ItemTags;
	TArray<FName> RequiredSourceTags;
	TArray<FName> ExcludedSourceTags;
	int64 Weight = 0;
	int32 MinStack = 1;
	int32 MaxStack = 1;
	int32 MinItemLevel = 0;
	int32 MaxItemLevel = MAX_int32;
	int64 MinUnitValue = 1;
	int64 MaxUnitValue = MAX_int64;

	bool IsValid() const;
};

struct Fdemo_mapRewardSourceDefinition
{
	FName MarkerId = NAME_None;
	FName RewardSourceId = NAME_None;
	Edemo_mapRewardSourceMode Mode = Edemo_mapRewardSourceMode::FixedTable;
	FName BudgetProfileId = NAME_None;
	TArray<FName> SourceTags;
	Edemo_mapRuntimeContainerSection TargetSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 Capacity = 0;
	FName FixedTableId = NAME_None;
	FName FixedFallbackTableId = NAME_None;
	bool bAllowFixedFallbackOnFailure = false;

	bool IsValid() const;
};

struct Fdemo_mapRewardGenerationRequest
{
	FName RequestId = NAME_None;
	FGuid RunId;
	FName LootSourceId = NAME_None;
	FName BudgetProfileId = NAME_None;
	TArray<FName> SourceTags;
	uint64 StableSeed = 0;
	Edemo_mapRuntimeContainerSection TargetSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 TargetCapacity = 0;
};

struct Fdemo_mapRewardPlannedStack
{
	FName DefinitionId = NAME_None;
	int32 StackCount = 0;
	int64 UnitValue = 0;
	int64 TotalValue = 0;
	Edemo_mapRuntimeContainerSection Section =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 SlotIndex = INDEX_NONE;
	Edemo_mapRewardEventKind RewardEventKind =
		Edemo_mapRewardEventKind::None;
	FGuid RewardEventId;
	int32 RewardValueMultiplierBps =
		Fdemo_mapRewardEventRules::NormalMultiplierBps;
	FName RewardSourceRoleId = NAME_None;
	FGuid RareRewardEventId;
	FName RareRewardPolicyId = NAME_None;
	FName RareRewardTierId = NAME_None;
	int64 RareRewardBonusValue = 0;
	Fdemo_mapRewardAffixSet AffixSet;

	bool operator==(const Fdemo_mapRewardPlannedStack& Other) const
	{
		return DefinitionId == Other.DefinitionId
			&& StackCount == Other.StackCount
			&& UnitValue == Other.UnitValue
			&& TotalValue == Other.TotalValue
			&& Section == Other.Section
			&& SlotIndex == Other.SlotIndex;
	}
};

struct Fdemo_mapRewardGenerationTrace
{
	FName RequestId = NAME_None;
	FGuid RunId;
	FName LootSourceId = NAME_None;
	FName BudgetProfileId = NAME_None;
	int64 BaseValue = 0;
	int32 MultiplierBps = 0;
	int64 RandomizedBudget = 0;
	int32 EligibleDefinitionCount = 0;
	uint64 EffectiveSeed = 0;
	int64 GeneratedTotalValue = 0;
	int64 ResidualValue = 0;
	bool bCapacityLimited = false;
	TArray<FName> OrderedEligibleEntryIds;
	FString Diagnostic;

	bool operator==(const Fdemo_mapRewardGenerationTrace& Other) const
	{
		return RequestId == Other.RequestId
			&& RunId == Other.RunId
			&& LootSourceId == Other.LootSourceId
			&& BudgetProfileId == Other.BudgetProfileId
			&& BaseValue == Other.BaseValue
			&& MultiplierBps == Other.MultiplierBps
			&& RandomizedBudget == Other.RandomizedBudget
			&& EligibleDefinitionCount == Other.EligibleDefinitionCount
			&& EffectiveSeed == Other.EffectiveSeed
			&& GeneratedTotalValue == Other.GeneratedTotalValue
			&& ResidualValue == Other.ResidualValue
			&& bCapacityLimited == Other.bCapacityLimited
			&& OrderedEligibleEntryIds == Other.OrderedEligibleEntryIds
			&& Diagnostic == Other.Diagnostic;
	}
};

struct Fdemo_mapRewardGenerationResult
{
	Edemo_mapRewardGenerationStatus Status =
		Edemo_mapRewardGenerationStatus::InvalidRequest;
	TArray<Fdemo_mapRewardPlannedStack> PlannedStacks;
	Fdemo_mapRewardGenerationTrace Trace;

	bool IsSuccess() const
	{
		return Status == Edemo_mapRewardGenerationStatus::Success
			|| Status == Edemo_mapRewardGenerationStatus::CapacityLimited;
	}

	bool operator==(const Fdemo_mapRewardGenerationResult& Other) const
	{
		return Status == Other.Status
			&& PlannedStacks == Other.PlannedStacks
			&& Trace == Other.Trace;
	}
};
