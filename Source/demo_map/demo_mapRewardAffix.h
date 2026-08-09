#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffixTypes.h"
#include "demo_mapRewardGenerationTypes.h"

struct Fdemo_mapRewardAffixDescriptor
{
	FName AffixId = NAME_None;
	FName CompatibilityGroup = NAME_None;
	FName RequiredCategoryId = NAME_None;
	Edemo_mapRewardAffixTier Tier = Edemo_mapRewardAffixTier::None;
	Edemo_mapRewardAffixEffect Effect =
		Edemo_mapRewardAffixEffect::AttackPower;
	int32 MagnitudeScaled = 0;
	int64 ResolvedValue = 0;
	FString DisplayLabel;

	bool IsValid() const;
};

struct Fdemo_mapRewardAffixPolicy
{
	FName PolicyId = NAME_None;
	bool IsValid() const { return !PolicyId.IsNone(); }
};

struct Fdemo_mapRewardAffixPityDecision
{
	int32 StateIn = 0;
	int32 StateOut = 0;
	bool bEligibleWeaponAttempt = false;
	bool bQualifyingTier3 = false;
	bool bGuaranteeApplied = false;
	bool bCommitRequired = false;
	int32 TierRoll = INDEX_NONE;
	FString Reason;
};

struct Fdemo_mapRewardAffixPlanTrace
{
	FName PolicyId = NAME_None;
	int64 BudgetBefore = 0;
	int64 AffixValueSpent = 0;
	int64 BudgetAfter = 0;
	int32 EligibleEquipmentCount = 0;
	int32 AffixedEquipmentCount = 0;
	int32 PityStateIn = 0;
	int32 PityStateOut = 0;
	TArray<Fdemo_mapRewardAffixPityDecision> PityDecisions;
	FString Diagnostic;
};

struct Fdemo_mapRewardAffixPolicyRegistry
{
	static const FName DefaultPolicyId;
	static const FName PityPolicyId;
	static const FName PityChannelId;

	static const Fdemo_mapRewardAffixPolicy& GetDefault();
	static const TArray<Fdemo_mapRewardAffixDescriptor>& GetAll();
	static const Fdemo_mapRewardAffixDescriptor* Find(FName AffixId);
	static bool Validate(FString* OutError = nullptr);
	static bool ValidateSet(
		FName DefinitionId,
		int32 Quantity,
		const Fdemo_mapRewardAffixSet& Set,
		FString* OutError = nullptr);
	static FString BuildDisplayLabel(const Fdemo_mapRewardAffixSet& Set);
};

struct Fdemo_mapRewardAffixPlanner
{
	static bool Apply(
		const Fdemo_mapRewardAffixPolicy& Policy,
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		int64 AvailableBudget,
		int32 PityStateIn,
		TArray<Fdemo_mapRewardPlannedStack>& InOutStacks,
		Fdemo_mapRewardAffixPlanTrace& OutTrace);
};

/** Active-run-only value ledger. It is deliberately absent from profile data. */
class Fdemo_mapRewardAffixPityLedger
{
public:
	int32 GetState(FGuid RunId, FName ChannelId) const;
	bool Commit(
		FGuid RunId,
		FName ChannelId,
		FName SourceRoleId,
		int32 StateIn,
		int32 StateOut);
	void ClearRun(FGuid RunId);
	void Reset();

private:
	TMap<FString, int32> States;
	TSet<FString> CommittedSources;
};
