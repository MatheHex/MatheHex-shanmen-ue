#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardGenerationTypes.h"
#include "demo_mapRewardSourceProjection.h"
#include "demo_mapSearchContainerActor.h"
#include "demo_mapLootChest.generated.h"

class Ademo_mapWorldItem;
class Ademo_mapV3ProgressionManager;
class Udemo_mapItemSubsystem;

/** Existing V3 Chest Actor, upgraded in place to the unified P4 Runtime Container. */
UCLASS()
class Ademo_mapLootChest : public Ademo_mapSearchContainerActor
{
	GENERATED_BODY()

public:
	Ademo_mapLootChest();
	void ConfigureChest(int32 InChestIndex, FName InChestId);
	void ConfigureFixedChest(
		int32 InChestIndex,
		FName InChestMarkerId,
		FName InLootTableId);
	void ConfigureRewardChest(
		int32 InChestIndex,
		const Fdemo_mapRewardSourceDefinition& Source);
	void ConfigureRewardProjection(
		int32 InChestIndex,
		const Fdemo_mapRewardSourceProjection& Projection);
	bool InitializeChest(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId);
	bool IsOpened() const { return IsContainerOpened(); }
	FName GetChestId() const { return ChestId; }
	FName GetLootTableId() const { return LootTableId; }
	int32 GetChestIndex() const { return ChestIndex; }
	bool IsP4PrototypeChest() const { return ChestIndex == 0; }
	bool HasOpenedPresentation() const;
	Edemo_mapRewardSourceMode GetRewardSourceMode() const
	{
		return RewardSourceMode;
	}
	FName GetRewardSourceId() const { return RewardSourceId; }
	FName GetRewardProjectionId() const { return RewardProjectionId; }
	FName GetRewardBudgetProfileId() const { return RewardBudgetProfileId; }
	bool HasRewardSourceTag(FName Tag) const { return RewardSourceTags.Contains(Tag); }
	bool UsedFixedFallback() const { return bUsedFixedFallback; }
	const Fdemo_mapRewardGenerationResult& GetLastRewardGenerationResult() const
	{
		return LastRewardGenerationResult;
	}
	const Fdemo_mapRewardSourceProjectionResult& GetLastProjectionResult() const
	{
		return LastProjectionResult;
	}

	// Kept only as a source-compatible empty view for historical harnesses. P4
	// containers never spawn loose loot from a Chest.
	const TArray<TWeakObjectPtr<Ademo_mapWorldItem>>& GetSpawnedLoot() const
	{
		return NoLooseLoot;
	}

private:
	FName ChestId = NAME_None;
	FName LootTableId = NAME_None;
	FName RewardSourceId = NAME_None;
	FName RewardProjectionId = NAME_None;
	FName RewardBudgetProfileId = NAME_None;
	TArray<FName> RewardSourceTags;
	Fdemo_mapRewardSourceProjection ConfiguredRewardProjection;
	FName FixedFallbackTableId = NAME_None;
	Edemo_mapRewardSourceMode RewardSourceMode =
		Edemo_mapRewardSourceMode::FixedTable;
	bool bAllowFixedFallbackOnFailure = false;
	bool bUsedFixedFallback = false;
	Fdemo_mapRewardGenerationResult LastRewardGenerationResult;
	Fdemo_mapRewardSourceProjectionResult LastProjectionResult;
	int32 ChestIndex = INDEX_NONE;
	TArray<TWeakObjectPtr<Ademo_mapWorldItem>> NoLooseLoot;
};
