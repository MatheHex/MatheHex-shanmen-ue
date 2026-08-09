#include "demo_mapRewardJackpot.h"

#include "demo_mapRewardEventTypes.h"
#include "demo_mapRewardGenerator.h"

const FName Fdemo_mapRewardJackpotPolicyRegistry::DefaultPolicyId(
	TEXT("Reward.Jackpot.Default"));

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

	uint64 Seed(
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

	FGuid StableEventId(
		FGuid RunId,
		FName StableSourceRoleId,
		FName ProjectionId,
		FName PolicyId,
		int32 SelectedIndex)
	{
		const uint64 First = Seed(
			RunId,
			StableSourceRoleId,
			ProjectionId,
			PolicyId,
			TEXT("EventId.A"));
		const FName IndexedProjection(*FString::Printf(
			TEXT("%s|%d"),
			*ProjectionId.ToString(),
			SelectedIndex));
		const uint64 Second = Seed(
			RunId,
			StableSourceRoleId,
			IndexedProjection,
			PolicyId,
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
			0x50330000u,
			0x4A41434Bu,
			0x504F5400u,
			static_cast<uint32>(Attempt + 1));
	}
}

bool Fdemo_mapRewardJackpotPolicy::IsValid() const
{
	return !PolicyId.IsNone()
		&& ChanceBps > 0
		&& ChanceBps < 10000
		&& JackpotMultiplierBps
			== Fdemo_mapRewardEventRules::JackpotMultiplierBps;
}

const Fdemo_mapRewardJackpotPolicy&
Fdemo_mapRewardJackpotPolicyRegistry::GetDefault()
{
	static const Fdemo_mapRewardJackpotPolicy Policy = {
		DefaultPolicyId,
		500,
		Fdemo_mapRewardEventRules::JackpotMultiplierBps
	};
	return Policy;
}

const Fdemo_mapRewardJackpotPolicy*
Fdemo_mapRewardJackpotPolicyRegistry::Find(FName PolicyId)
{
	return PolicyId == DefaultPolicyId ? &GetDefault() : nullptr;
}

bool Fdemo_mapRewardJackpotPolicyRegistry::Validate(FString* OutError)
{
	if (!GetDefault().IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("Default Jackpot policy is invalid.");
		}
		return false;
	}
	return true;
}

bool Fdemo_mapRewardJackpot::IsHitRoll(int32 Roll, int32 ChanceBps)
{
	return Roll >= 0 && Roll < 10000
		&& ChanceBps >= 0 && ChanceBps <= 10000
		&& Roll < ChanceBps;
}

Fdemo_mapRewardJackpotDecision Fdemo_mapRewardJackpot::Annotate(
	const Fdemo_mapRewardJackpotPolicy& Policy,
	FGuid RunId,
	FName StableSourceRoleId,
	FName ProjectionId,
	TArray<Fdemo_mapRewardPlannedStack>& InOutPlannedStacks)
{
	Fdemo_mapRewardJackpotDecision Decision;
	Decision.PolicyId = Policy.PolicyId;
	if (!Policy.IsValid() || !RunId.IsValid()
		|| StableSourceRoleId.IsNone() || ProjectionId.IsNone())
	{
		Decision.Diagnostic = TEXT("invalid_request");
		return Decision;
	}
	for (const Fdemo_mapRewardPlannedStack& Stack : InOutPlannedStacks)
	{
		if (Stack.TotalValue <= 0
			|| Decision.BaseGeneratedValue > MAX_int64 - Stack.TotalValue)
		{
			Decision.Diagnostic = TEXT("base_value_overflow_or_ineligible");
			return Decision;
		}
		Decision.BaseGeneratedValue += Stack.TotalValue;
	}
	if (InOutPlannedStacks.IsEmpty())
	{
		Decision.Diagnostic = TEXT("empty_plan");
		return Decision;
	}
	Decision.RollSeed = Seed(
		RunId,
		StableSourceRoleId,
		ProjectionId,
		Policy.PolicyId,
		TEXT("Roll"));
	Decision.Roll = static_cast<int32>(Decision.RollSeed % 10000ull);
	Decision.bHit = IsHitRoll(Decision.Roll, Policy.ChanceBps);
	Decision.FinalEffectiveRewardValue = Decision.BaseGeneratedValue;
	if (!Decision.bHit)
	{
		Decision.Diagnostic = TEXT("success");
		return Decision;
	}

	TArray<int32> EligibleIndexes;
	for (int32 Index = 0; Index < InOutPlannedStacks.Num(); ++Index)
	{
		if (InOutPlannedStacks[Index].TotalValue > 0)
		{
			EligibleIndexes.Add(Index);
		}
	}
	if (EligibleIndexes.IsEmpty())
	{
		Decision.Diagnostic = TEXT("hit_without_eligible_entry");
		return Decision;
	}
	Decision.SelectionSeed = Seed(
		RunId,
		StableSourceRoleId,
		ProjectionId,
		Policy.PolicyId,
		TEXT("Selection"));
	Decision.SelectedPlannedEntryIndex =
		EligibleIndexes[
			static_cast<int32>(
				Decision.SelectionSeed
				% static_cast<uint64>(EligibleIndexes.Num()))];
	Decision.SelectedBaseValue =
		InOutPlannedStacks[Decision.SelectedPlannedEntryIndex].TotalValue;
	if (Decision.SelectedBaseValue > MAX_int64 / 5)
	{
		Decision.Diagnostic = TEXT("bonus_overflow");
		return Decision;
	}
	Decision.JackpotBonusValue = Decision.SelectedBaseValue * 5;
	if (Decision.BaseGeneratedValue
		> MAX_int64 - Decision.JackpotBonusValue)
	{
		Decision.Diagnostic = TEXT("final_value_overflow");
		return Decision;
	}
	Decision.FinalEffectiveRewardValue =
		Decision.BaseGeneratedValue + Decision.JackpotBonusValue;
	Decision.EventId = StableEventId(
		RunId,
		StableSourceRoleId,
		ProjectionId,
		Policy.PolicyId,
		Decision.SelectedPlannedEntryIndex);
	Fdemo_mapRewardPlannedStack& Selected =
		InOutPlannedStacks[Decision.SelectedPlannedEntryIndex];
	Selected.RewardEventKind = Edemo_mapRewardEventKind::Jackpot;
	Selected.RewardEventId = Decision.EventId;
	Selected.RewardValueMultiplierBps = Policy.JackpotMultiplierBps;
	Selected.RewardSourceRoleId = StableSourceRoleId;
	Decision.Diagnostic = TEXT("success");
	return Decision;
}

bool Fdemo_mapRewardJackpot::FindNaturalHitAndMiss(
	const Fdemo_mapRewardJackpotPolicy& Policy,
	FName StableSourceRoleId,
	FName ProjectionId,
	int32 MaxAttempts,
	FGuid& OutHitRunId,
	FGuid& OutMissRunId)
{
	OutHitRunId.Invalidate();
	OutMissRunId.Invalidate();
	if (!Policy.IsValid() || StableSourceRoleId.IsNone()
		|| ProjectionId.IsNone() || MaxAttempts <= 0)
	{
		return false;
	}
	for (int32 Attempt = 0; Attempt < MaxAttempts
		&& (!OutHitRunId.IsValid() || !OutMissRunId.IsValid()); ++Attempt)
	{
		const FGuid RunId = CandidateRunId(Attempt);
		const uint64 RollSeed = Seed(
			RunId,
			StableSourceRoleId,
			ProjectionId,
			Policy.PolicyId,
			TEXT("Roll"));
		const bool bHit = IsHitRoll(
			static_cast<int32>(RollSeed % 10000ull),
			Policy.ChanceBps);
		if (bHit && !OutHitRunId.IsValid())
		{
			OutHitRunId = RunId;
		}
		else if (!bHit && !OutMissRunId.IsValid())
		{
			OutMissRunId = RunId;
		}
	}
	return OutHitRunId.IsValid() && OutMissRunId.IsValid();
}

bool Fdemo_mapRewardJackpot::FindNaturalRunForHitAndMissSources(
	const Fdemo_mapRewardJackpotPolicy& Policy,
	FName HitStableSourceRoleId,
	FName HitProjectionId,
	FName MissStableSourceRoleId,
	FName MissProjectionId,
	int32 MaxAttempts,
	FGuid& OutRunId)
{
	OutRunId.Invalidate();
	if (!Policy.IsValid()
		|| HitStableSourceRoleId.IsNone()
		|| HitProjectionId.IsNone()
		|| MissStableSourceRoleId.IsNone()
		|| MissProjectionId.IsNone()
		|| MaxAttempts <= 0)
	{
		return false;
	}
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const FGuid Candidate = CandidateRunId(Attempt);
		const int32 HitRoll = static_cast<int32>(
			Seed(
				Candidate,
				HitStableSourceRoleId,
				HitProjectionId,
				Policy.PolicyId,
				TEXT("Roll"))
			% 10000ull);
		const int32 MissRoll = static_cast<int32>(
			Seed(
				Candidate,
				MissStableSourceRoleId,
				MissProjectionId,
				Policy.PolicyId,
				TEXT("Roll"))
			% 10000ull);
		if (IsHitRoll(HitRoll, Policy.ChanceBps)
			&& !IsHitRoll(MissRoll, Policy.ChanceBps))
		{
			OutRunId = Candidate;
			return true;
		}
	}
	return false;
}

bool Fdemo_mapRewardJackpot::FindNaturalRunForSingleHitAcrossSources(
	const Fdemo_mapRewardJackpotPolicy& Policy,
	FName HitStableSourceRoleId,
	FName HitProjectionId,
	const TArray<TPair<FName, FName>>&
		AllSourceRoleProjectionPairs,
	int32 MaxAttempts,
	FGuid& OutRunId)
{
	OutRunId.Invalidate();
	if (!Policy.IsValid()
		|| HitStableSourceRoleId.IsNone()
		|| HitProjectionId.IsNone()
		|| AllSourceRoleProjectionPairs.IsEmpty()
		|| MaxAttempts <= 0)
	{
		return false;
	}
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const FGuid Candidate = CandidateRunId(Attempt);
		int32 HitCount = 0;
		bool bDesignatedHit = false;
		bool bInvalid = false;
		for (const TPair<FName, FName>& Source :
			AllSourceRoleProjectionPairs)
		{
			if (Source.Key.IsNone() || Source.Value.IsNone())
			{
				bInvalid = true;
				break;
			}
			const bool bHit = IsHitRoll(
				static_cast<int32>(
					Seed(
						Candidate,
						Source.Key,
						Source.Value,
						Policy.PolicyId,
						TEXT("Roll"))
					% 10000ull),
				Policy.ChanceBps);
			HitCount += bHit ? 1 : 0;
			if (Source.Key == HitStableSourceRoleId
				&& Source.Value == HitProjectionId)
			{
				bDesignatedHit = bHit;
			}
		}
		if (!bInvalid && bDesignatedHit && HitCount == 1)
		{
			OutRunId = Candidate;
			return true;
		}
	}
	return false;
}
