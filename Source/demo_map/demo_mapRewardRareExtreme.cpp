#include "demo_mapRewardRareExtreme.h"

#include "demo_mapRewardGenerator.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardSourceProjection.h"

const FName Fdemo_mapRewardRareExtremePolicyRegistry::DefaultPolicyId(
	TEXT("Reward.RareExtreme.Default"));
const FName Fdemo_mapRewardRareExtremePolicyRegistry::Tier10Id(
	TEXT("Reward.RareExtreme.Tier10"));
const FName Fdemo_mapRewardRareExtremePolicyRegistry::Tier25Id(
	TEXT("Reward.RareExtreme.Tier25"));
const FName Fdemo_mapRewardRareExtremePolicyRegistry::Tier50Id(
	TEXT("Reward.RareExtreme.Tier50"));
const FName Fdemo_mapRewardRareExtremePolicyRegistry::Tier125Id(
	TEXT("Reward.RareExtreme.Tier125"));

namespace
{
	FName DomainName(
		FName ProjectionId,
		FName PolicyId,
		const TCHAR* Domain)
	{
		return FName(*FString::Printf(
			TEXT("%s|%s|%s"),
			*ProjectionId.ToString(),
			*PolicyId.ToString(),
			Domain));
	}

	uint64 RareSeed(
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		FName PolicyId,
		const TCHAR* Domain)
	{
		return Fdemo_mapRewardGenerator::ComputeStableSeed(
			RunId,
			StableSourceRoleId,
			DomainName(ProjectionId, PolicyId, Domain));
	}

	uint64 JackpotRollSeed(
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		FName PolicyId)
	{
		return Fdemo_mapRewardGenerator::ComputeStableSeed(
			RunId,
			StableSourceRoleId,
			DomainName(ProjectionId, PolicyId, TEXT("Roll")));
	}

	FGuid StableEventId(
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		FName PolicyId)
	{
		const uint64 First = RareSeed(
			RunId, StableSourceRoleId, ProjectionId, PolicyId,
			TEXT("EventId.A"));
		const uint64 Second = RareSeed(
			RunId, StableSourceRoleId, ProjectionId, PolicyId,
			TEXT("EventId.B"));
		FGuid Result(
			static_cast<uint32>(First >> 32),
			static_cast<uint32>(First),
			static_cast<uint32>(Second >> 32),
			static_cast<uint32>(Second));
		if (!Result.IsValid())
		{
			Result.D = 1;
		}
		return Result;
	}

	FGuid CandidateRunId(int32 Attempt)
	{
		return FGuid(
			0x50340000u,
			0x52415245u,
			0x45585400u,
			static_cast<uint32>(Attempt + 1));
	}

	const Fdemo_mapRewardRareExtremeTier* SelectTier(
		const Fdemo_mapRewardRareExtremePolicy& Policy,
		int32 Roll)
	{
		int32 Upper = 0;
		for (const Fdemo_mapRewardRareExtremeTier& Tier : Policy.Tiers)
		{
			Upper += Tier.Weight;
			if (Roll < Upper)
			{
				return &Tier;
			}
		}
		return nullptr;
	}
}

bool Fdemo_mapRewardRareExtremeTier::IsValid() const
{
	return !TierId.IsNone() && MultiplierBps > 0 && Weight > 0;
}

bool Fdemo_mapRewardRareExtremePolicy::IsValid() const
{
	if (PolicyId.IsNone()
		|| ChanceBps <= 0 || ChanceBps >= 10000
		|| MaxTargetValue <= 0 || MaxCarrierItems <= 0
		|| Tiers.IsEmpty())
	{
		return false;
	}
	int32 TotalWeight = 0;
	TSet<FName> Ids;
	for (const Fdemo_mapRewardRareExtremeTier& Tier : Tiers)
	{
		if (!Tier.IsValid() || Ids.Contains(Tier.TierId))
		{
			return false;
		}
		Ids.Add(Tier.TierId);
		TotalWeight += Tier.Weight;
	}
	return TotalWeight == 10000;
}

const Fdemo_mapRewardRareExtremePolicy&
Fdemo_mapRewardRareExtremePolicyRegistry::GetDefault()
{
	static const Fdemo_mapRewardRareExtremePolicy Policy = {
		DefaultPolicyId,
		5,
		150000,
		3,
		{
			{Tier10Id, 100000, 7000},
			{Tier25Id, 250000, 2000},
			{Tier50Id, 500000, 800},
			{Tier125Id, 1250000, 200}
		}
	};
	return Policy;
}

const Fdemo_mapRewardRareExtremePolicy*
Fdemo_mapRewardRareExtremePolicyRegistry::Find(FName PolicyId)
{
	return PolicyId == DefaultPolicyId ? &GetDefault() : nullptr;
}

bool Fdemo_mapRewardRareExtremePolicyRegistry::Validate(FString* OutError)
{
	const Fdemo_mapRewardRareExtremePolicy& Policy = GetDefault();
	const bool bExact =
		Policy.IsValid()
		&& Policy.ChanceBps == 5
		&& Policy.MaxTargetValue == 150000
		&& Policy.MaxCarrierItems == 3
		&& Policy.Tiers.Num() == 4
		&& Policy.Tiers[0].TierId == Tier10Id
		&& Policy.Tiers[0].MultiplierBps == 100000
		&& Policy.Tiers[0].Weight == 7000
		&& Policy.Tiers[1].TierId == Tier25Id
		&& Policy.Tiers[1].MultiplierBps == 250000
		&& Policy.Tiers[1].Weight == 2000
		&& Policy.Tiers[2].TierId == Tier50Id
		&& Policy.Tiers[2].MultiplierBps == 500000
		&& Policy.Tiers[2].Weight == 800
		&& Policy.Tiers[3].TierId == Tier125Id
		&& Policy.Tiers[3].MultiplierBps == 1250000
		&& Policy.Tiers[3].Weight == 200;
	if (!bExact && OutError)
	{
		*OutError = TEXT("Default Rare Extreme policy is invalid.");
	}
	return bExact;
}

bool Fdemo_mapRewardRareExtreme::IsHitRoll(int32 Roll, int32 ChanceBps)
{
	return Roll >= 0 && Roll < 10000
		&& ChanceBps >= 0 && ChanceBps <= 10000
		&& Roll < ChanceBps;
}

Fdemo_mapRewardRareExtremeDecision Fdemo_mapRewardRareExtreme::Decide(
	const Fdemo_mapRewardRareExtremePolicy& Policy,
	FGuid RunId,
	FName StableSourceRoleId,
	FName ProjectionId,
	int64 BaseSourceValue)
{
	Fdemo_mapRewardRareExtremeDecision Decision;
	Decision.PolicyId = Policy.PolicyId;
	Decision.BaseSourceValue = BaseSourceValue;
	if (!Policy.IsValid() || !RunId.IsValid()
		|| StableSourceRoleId.IsNone() || ProjectionId.IsNone()
		|| BaseSourceValue <= 0)
	{
		Decision.Diagnostic = TEXT("invalid_request");
		return Decision;
	}
	Decision.RollSeed = RareSeed(
		RunId, StableSourceRoleId, ProjectionId, Policy.PolicyId,
		TEXT("Roll"));
	Decision.Roll = static_cast<int32>(Decision.RollSeed % 10000ull);
	Decision.bHit = IsHitRoll(Decision.Roll, Policy.ChanceBps);
	if (!Decision.bHit)
	{
		Decision.Diagnostic = TEXT("success");
		return Decision;
	}
	Decision.TierSeed = RareSeed(
		RunId, StableSourceRoleId, ProjectionId, Policy.PolicyId,
		TEXT("Tier"));
	Decision.TierRoll = static_cast<int32>(Decision.TierSeed % 10000ull);
	const Fdemo_mapRewardRareExtremeTier* Tier =
		SelectTier(Policy, Decision.TierRoll);
	if (!Tier)
	{
		Decision.Diagnostic = TEXT("tier_selection_failed");
		return Decision;
	}
	Decision.TierId = Tier->TierId;
	Decision.TierMultiplierBps = Tier->MultiplierBps;
	if (BaseSourceValue
		> MAX_int64 / static_cast<int64>(Tier->MultiplierBps))
	{
		Decision.Diagnostic = TEXT("target_value_overflow");
		return Decision;
	}
	Decision.TargetValue = FMath::Min(
		BaseSourceValue * static_cast<int64>(Tier->MultiplierBps) / 10000,
		Policy.MaxTargetValue);
	if (Decision.TargetValue <= 0)
	{
		Decision.Diagnostic = TEXT("invalid_target_value");
		return Decision;
	}
	Decision.CarrierSelectionSeed = RareSeed(
		RunId, StableSourceRoleId, ProjectionId, Policy.PolicyId,
		TEXT("CarrierSelection"));
	Decision.EventId = StableEventId(
		RunId, StableSourceRoleId, ProjectionId, Policy.PolicyId);
	Decision.Diagnostic = TEXT("success");
	return Decision;
}

bool Fdemo_mapRewardRareExtreme::Annotate(
	const Fdemo_mapRewardRareExtremePolicy& Policy,
	FName StableSourceRoleId,
	Fdemo_mapRewardRareExtremeDecision& InOutDecision,
	TArray<Fdemo_mapRewardPlannedStack>& InOutPlannedStacks)
{
	if (!Policy.IsValid() || !InOutDecision.IsSuccess()
		|| StableSourceRoleId.IsNone())
	{
		InOutDecision.Diagnostic = TEXT("invalid_annotation_request");
		return false;
	}
	for (const Fdemo_mapRewardPlannedStack& Stack : InOutPlannedStacks)
	{
		const int64 AffixValue = Stack.AffixSet.TotalResolvedValue();
		if (Stack.TotalValue < 0 || AffixValue < 0
			|| Stack.TotalValue > MAX_int64 - AffixValue
			|| InOutDecision.BaseGeneratedValue
				> MAX_int64 - Stack.TotalValue - AffixValue)
		{
			InOutDecision.Diagnostic = TEXT("base_generated_overflow");
			return false;
		}
		InOutDecision.BaseGeneratedValue += Stack.TotalValue + AffixValue;
	}
	if (!InOutDecision.bHit)
	{
		return true;
	}
	if (!InOutDecision.EventId.IsValid()
		|| InOutDecision.TierId.IsNone()
		|| InOutDecision.BaseGeneratedValue > InOutDecision.TargetValue)
	{
		InOutDecision.Diagnostic = TEXT("invalid_hit_plan");
		return false;
	}
	InOutDecision.BonusPoolValue =
		InOutDecision.TargetValue - InOutDecision.BaseGeneratedValue;
	TArray<int32> Eligible;
	for (int32 Index = 0; Index < InOutPlannedStacks.Num(); ++Index)
	{
		const int64 AffixValue =
			InOutPlannedStacks[Index].AffixSet.TotalResolvedValue();
		if (InOutPlannedStacks[Index].TotalValue > 0
			&& AffixValue >= 0
			&& InOutPlannedStacks[Index].TotalValue
				<= MAX_int64 - AffixValue)
		{
			Eligible.Add(Index);
		}
	}
	if (Eligible.IsEmpty() || InOutDecision.BonusPoolValue <= 0)
	{
		InOutDecision.Diagnostic = TEXT("hit_without_positive_bonus_carrier");
		return false;
	}
	const int32 CarrierCount = FMath::Min(
		Policy.MaxCarrierItems, Eligible.Num());
	const int32 Start = static_cast<int32>(
		InOutDecision.CarrierSelectionSeed
			% static_cast<uint64>(Eligible.Num()));
	for (int32 Offset = 0; Offset < CarrierCount; ++Offset)
	{
		InOutDecision.CarrierPlannedEntryIndexes.Add(
			Eligible[(Start + Offset) % Eligible.Num()]);
	}
	InOutDecision.CarrierPlannedEntryIndexes.Sort();
	int64 SelectedBaseValue = 0;
	for (const int32 Index : InOutDecision.CarrierPlannedEntryIndexes)
	{
		SelectedBaseValue +=
			InOutPlannedStacks[Index].TotalValue
			+ InOutPlannedStacks[Index].AffixSet.TotalResolvedValue();
	}
	if (SelectedBaseValue <= 0)
	{
		InOutDecision.Diagnostic = TEXT("invalid_carrier_base_value");
		return false;
	}
	int64 Assigned = 0;
	for (const int32 Index : InOutDecision.CarrierPlannedEntryIndexes)
	{
		const int64 Base =
			InOutPlannedStacks[Index].TotalValue
			+ InOutPlannedStacks[Index].AffixSet.TotalResolvedValue();
		const int64 Quotient = InOutDecision.BonusPoolValue
			/ SelectedBaseValue;
		const int64 Remainder = InOutDecision.BonusPoolValue
			% SelectedBaseValue;
		const int64 Bonus =
			Quotient * Base + (Remainder * Base) / SelectedBaseValue;
		InOutPlannedStacks[Index].RareRewardBonusValue = Bonus;
		Assigned += Bonus;
	}
	int64 Remaining = InOutDecision.BonusPoolValue - Assigned;
	for (int32 Cursor = 0; Remaining > 0; ++Cursor, --Remaining)
	{
		const int32 Index =
			InOutDecision.CarrierPlannedEntryIndexes[
				Cursor % InOutDecision.CarrierPlannedEntryIndexes.Num()];
		++InOutPlannedStacks[Index].RareRewardBonusValue;
	}
	for (const int32 Index : InOutDecision.CarrierPlannedEntryIndexes)
	{
		Fdemo_mapRewardPlannedStack& Stack = InOutPlannedStacks[Index];
		if (Stack.RareRewardBonusValue <= 0)
		{
			InOutDecision.Diagnostic = TEXT("non_positive_carrier_bonus");
			return false;
		}
		Stack.RareRewardEventId = InOutDecision.EventId;
		Stack.RareRewardPolicyId = Policy.PolicyId;
		Stack.RareRewardTierId = InOutDecision.TierId;
		Stack.RewardSourceRoleId = StableSourceRoleId;
	}
	InOutDecision.CarrierCount =
		InOutDecision.CarrierPlannedEntryIndexes.Num();
	return true;
}

bool Fdemo_mapRewardRareExtreme::FindNaturalTier125JackpotMiss(
	const Fdemo_mapRewardRareExtremePolicy& Policy,
	const Fdemo_mapRewardJackpotPolicy& JackpotPolicy,
	FName StableSourceRoleId,
	FName ProjectionId,
	int64 BaseSourceValue,
	int32 MaxAttempts,
	FGuid& OutRunId,
	int32& OutAttempt)
{
	OutRunId.Invalidate();
	OutAttempt = INDEX_NONE;
	if (!Policy.IsValid() || !JackpotPolicy.IsValid()
		|| StableSourceRoleId.IsNone() || ProjectionId.IsNone()
		|| BaseSourceValue <= 0 || MaxAttempts <= 0)
	{
		return false;
	}
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const FGuid Candidate = CandidateRunId(Attempt);
		const Fdemo_mapRewardRareExtremeDecision Rare = Decide(
			Policy, Candidate, StableSourceRoleId, ProjectionId,
			BaseSourceValue);
		const int32 JackpotRoll = static_cast<int32>(
			JackpotRollSeed(
				Candidate, StableSourceRoleId, ProjectionId,
				JackpotPolicy.PolicyId)
			% 10000ull);
		const Fdemo_mapRewardSourceProjection* MissProjection =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::ChestMainWood);
		const Fdemo_mapRewardRareExtremeDecision RealSourceMiss =
			MissProjection
				? Decide(
					Policy,
					Candidate,
					MissProjection->StableSourceRoleId,
					MissProjection->ProjectionId,
					1200)
				: Fdemo_mapRewardRareExtremeDecision();
		if (Rare.IsSuccess() && Rare.bHit
			&& Rare.TierId == Fdemo_mapRewardRareExtremePolicyRegistry::Tier125Id
			&& RealSourceMiss.IsSuccess()
			&& !RealSourceMiss.bHit
			&& !Fdemo_mapRewardJackpot::IsHitRoll(
				JackpotRoll, JackpotPolicy.ChanceBps))
		{
			OutRunId = Candidate;
			OutAttempt = Attempt + 1;
			return true;
		}
	}
	return false;
}
