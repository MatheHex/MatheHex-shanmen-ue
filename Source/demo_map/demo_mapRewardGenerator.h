#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardGenerationTypes.h"

/** Pure, deterministic value-first planner. It never creates ItemInstances. */
struct Fdemo_mapRewardGenerator
{
	static uint64 ComputeStableSeed(
		FGuid RunId,
		FName LootSourceId,
		FName BudgetProfileId);

	static Fdemo_mapRewardGenerationResult Generate(
		const Fdemo_mapRewardGenerationRequest& Request);

	static Fdemo_mapRewardGenerationResult GenerateWithData(
		const Fdemo_mapRewardGenerationRequest& Request,
		const Fdemo_mapRewardBudgetProfile& Profile,
		const TArray<Fdemo_mapRewardPoolEntry>& PoolEntries);

	static TArray<Fdemo_mapRuntimeContainerSeedEntry> BuildContainerSeed(
		const Fdemo_mapRewardGenerationResult& Result);
};

/** Per-run duplicate-source ledger. Commit occurs only after materialization. */
class Fdemo_mapRewardGenerationSession
{
public:
	bool IsProcessed(FGuid RunId, FName LootSourceId) const;
	bool Commit(FGuid RunId, FName LootSourceId);
	void Reset();
	int32 Num() const { return ProcessedSourceKeys.Num(); }

private:
	static FString MakeKey(FGuid RunId, FName LootSourceId);
	TSet<FString> ProcessedSourceKeys;
};

