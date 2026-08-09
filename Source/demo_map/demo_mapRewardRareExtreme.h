#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardGenerationTypes.h"

struct Fdemo_mapRewardJackpotPolicy;

struct Fdemo_mapRewardRareExtremeTier
{
	FName TierId = NAME_None;
	int32 MultiplierBps = 0;
	int32 Weight = 0;

	bool IsValid() const;
};

struct Fdemo_mapRewardRareExtremePolicy
{
	FName PolicyId = NAME_None;
	int32 ChanceBps = 0;
	int64 MaxTargetValue = 0;
	int32 MaxCarrierItems = 0;
	TArray<Fdemo_mapRewardRareExtremeTier> Tiers;

	bool IsValid() const;
};

struct Fdemo_mapRewardRareExtremeDecision
{
	FName PolicyId = NAME_None;
	uint64 RollSeed = 0;
	uint64 TierSeed = 0;
	uint64 CarrierSelectionSeed = 0;
	int32 Roll = INDEX_NONE;
	int32 TierRoll = INDEX_NONE;
	bool bHit = false;
	FName TierId = NAME_None;
	int32 TierMultiplierBps = 0;
	int64 BaseSourceValue = 0;
	int64 TargetValue = 0;
	FGuid EventId;
	int64 BaseGeneratedValue = 0;
	int64 BonusPoolValue = 0;
	int32 CarrierCount = 0;
	TArray<int32> CarrierPlannedEntryIndexes;
	FString Diagnostic;

	bool IsSuccess() const { return Diagnostic == TEXT("success"); }
};

struct Fdemo_mapRewardRareExtremePolicyRegistry
{
	static const FName DefaultPolicyId;
	static const FName Tier10Id;
	static const FName Tier25Id;
	static const FName Tier50Id;
	static const FName Tier125Id;

	static const Fdemo_mapRewardRareExtremePolicy& GetDefault();
	static const Fdemo_mapRewardRareExtremePolicy* Find(FName PolicyId);
	static bool Validate(FString* OutError = nullptr);
};

/**
 * Pure pre-plan decision and post-plan bonus annotation. All domains are
 * intentionally separated from the normal generator and Jackpot planner.
 */
struct Fdemo_mapRewardRareExtreme
{
	static bool IsHitRoll(int32 Roll, int32 ChanceBps);
	static Fdemo_mapRewardRareExtremeDecision Decide(
		const Fdemo_mapRewardRareExtremePolicy& Policy,
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		int64 BaseSourceValue);
	static bool Annotate(
		const Fdemo_mapRewardRareExtremePolicy& Policy,
		FName StableSourceRoleId,
		Fdemo_mapRewardRareExtremeDecision& InOutDecision,
		TArray<Fdemo_mapRewardPlannedStack>& InOutPlannedStacks);
	static bool FindNaturalTier125JackpotMiss(
		const Fdemo_mapRewardRareExtremePolicy& Policy,
		const Fdemo_mapRewardJackpotPolicy& JackpotPolicy,
		FName StableSourceRoleId,
		FName ProjectionId,
		int64 BaseSourceValue,
		int32 MaxAttempts,
		FGuid& OutRunId,
		int32& OutAttempt);
};
