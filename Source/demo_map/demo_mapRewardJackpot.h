#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardGenerationTypes.h"

struct Fdemo_mapRewardJackpotPolicy
{
	FName PolicyId = NAME_None;
	int32 ChanceBps = 0;
	int32 JackpotMultiplierBps = 0;

	bool IsValid() const;
};

struct Fdemo_mapRewardJackpotDecision
{
	FName PolicyId = NAME_None;
	uint64 RollSeed = 0;
	uint64 SelectionSeed = 0;
	int32 Roll = INDEX_NONE;
	bool bHit = false;
	int32 SelectedPlannedEntryIndex = INDEX_NONE;
	FGuid EventId;
	int64 BaseGeneratedValue = 0;
	int64 SelectedBaseValue = 0;
	int64 JackpotBonusValue = 0;
	int64 FinalEffectiveRewardValue = 0;
	FString Diagnostic;

	bool IsSuccess() const { return Diagnostic == TEXT("success"); }
};

struct Fdemo_mapRewardJackpotPolicyRegistry
{
	static const FName DefaultPolicyId;
	static const Fdemo_mapRewardJackpotPolicy& GetDefault();
	static const Fdemo_mapRewardJackpotPolicy* Find(FName PolicyId);
	static bool Validate(FString* OutError = nullptr);
};

/** Pure post-plan annotation. It never consumes or changes P1/P2 RNG state. */
struct Fdemo_mapRewardJackpot
{
	static bool IsHitRoll(int32 Roll, int32 ChanceBps);
	static Fdemo_mapRewardJackpotDecision Annotate(
		const Fdemo_mapRewardJackpotPolicy& Policy,
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		TArray<Fdemo_mapRewardPlannedStack>& InOutPlannedStacks);
	static bool FindNaturalHitAndMiss(
		const Fdemo_mapRewardJackpotPolicy& Policy,
		FName StableSourceRoleId,
		FName ProjectionId,
		int32 MaxAttempts,
		FGuid& OutHitRunId,
		FGuid& OutMissRunId);
	static bool FindNaturalRunForHitAndMissSources(
		const Fdemo_mapRewardJackpotPolicy& Policy,
		FName HitStableSourceRoleId,
		FName HitProjectionId,
		FName MissStableSourceRoleId,
		FName MissProjectionId,
		int32 MaxAttempts,
		FGuid& OutRunId);
	static bool FindNaturalRunForSingleHitAcrossSources(
		const Fdemo_mapRewardJackpotPolicy& Policy,
		FName HitStableSourceRoleId,
		FName HitProjectionId,
		const TArray<TPair<FName, FName>>& AllSourceRoleProjectionPairs,
		int32 MaxAttempts,
		FGuid& OutRunId);
};
