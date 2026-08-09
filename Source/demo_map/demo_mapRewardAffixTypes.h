#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapRewardAffixTier : uint8
{
	None = 0,
	Tier1 = 1,
	Tier2 = 2,
	Tier3 = 3
};

enum class Edemo_mapRewardAffixEffect : uint8
{
	AttackPower,
	MaxHealth,
	FlatDamageReduction,
	CooldownMultiplierDeltaBps
};

enum class Edemo_mapRewardAffixAcquisition : uint8
{
	None,
	Natural,
	PityGuaranteed
};

struct Fdemo_mapResolvedRewardAffix
{
	FName AffixId = NAME_None;
	Edemo_mapRewardAffixTier Tier = Edemo_mapRewardAffixTier::None;
	int32 ResolvedMagnitudeScaled = 0;
	int64 ResolvedValue = 0;

	bool operator==(const Fdemo_mapResolvedRewardAffix& Other) const
	{
		return AffixId == Other.AffixId
			&& Tier == Other.Tier
			&& ResolvedMagnitudeScaled == Other.ResolvedMagnitudeScaled
			&& ResolvedValue == Other.ResolvedValue;
	}
};

struct Fdemo_mapRewardAffixSet
{
	FGuid AffixSetEventId;
	FName AffixPolicyId = NAME_None;
	Edemo_mapRewardAffixAcquisition Acquisition =
		Edemo_mapRewardAffixAcquisition::None;
	TArray<Fdemo_mapResolvedRewardAffix> Affixes;

	int64 TotalResolvedValue() const
	{
		int64 Total = 0;
		for (const Fdemo_mapResolvedRewardAffix& Affix : Affixes)
		{
			if (Affix.ResolvedValue < 0
				|| Total > MAX_int64 - Affix.ResolvedValue)
			{
				return -1;
			}
			Total += Affix.ResolvedValue;
		}
		return Total;
	}

	bool IsEmpty() const
	{
		return !AffixSetEventId.IsValid()
			&& AffixPolicyId.IsNone()
			&& Acquisition == Edemo_mapRewardAffixAcquisition::None
			&& Affixes.IsEmpty();
	}

	bool operator==(const Fdemo_mapRewardAffixSet& Other) const
	{
		return AffixSetEventId == Other.AffixSetEventId
			&& AffixPolicyId == Other.AffixPolicyId
			&& Acquisition == Other.Acquisition
			&& Affixes == Other.Affixes;
	}
};
