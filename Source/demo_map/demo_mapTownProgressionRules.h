#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

/**
 * P1's immutable planning contract. Town upgrades remain presentation-only;
 * later tasks may consume these values, but must not duplicate or mutate them.
 */
struct Fdemo_mapTownUpgradeCost
{
	int32 SpiritWood = 0;
	int32 SpiritOre = 0;
	int64 SpiritStones = 0;
};

class Fdemo_mapTownProgressionRules final
{
public:
	static constexpr int32 MaxTownLevel = 5;
	static constexpr int32 NormalSpiritStoneMin = 15;
	static constexpr int32 NormalSpiritStoneMax = 25;
	static constexpr int32 EliteSpiritStoneMin = 35;
	static constexpr int32 EliteSpiritStoneMax = 55;
	static constexpr int32 BossSpiritStoneMin = 80;
	static constexpr int32 BossSpiritStoneMax = 120;

	static bool TryGetNextLevelCost(
		int32 CurrentTownLevel,
		Fdemo_mapTownUpgradeCost& OutCost)
	{
		static const Fdemo_mapTownUpgradeCost Costs[] = {
			{ 30, 30, 100 }, { 40, 40, 500 }, { 50, 50, 2500 },
			{ 60, 60, 12500 }, { 70, 70, 62500 }
		};
		if (CurrentTownLevel < 0 || CurrentTownLevel >= UE_ARRAY_COUNT(Costs))
		{
			return false;
		}
		OutCost = Costs[CurrentTownLevel];
		return true;
	}

	/**
	 * One deterministic currency roll per enemy occurrence.  The run identity is
	 * deliberately part of the key: re-reading the same run cannot re-roll it,
	 * while a later run has a distinct authority key.
	 */
	static bool TryResolveEnemySpiritStoneValue(
		const FGuid& RunId,
		FName EncounterId,
		FName RiskTierId,
		bool bElite,
		bool bBoss,
		int64& OutValue)
	{
		if (!RunId.IsValid() || EncounterId.IsNone() || RiskTierId.IsNone())
		{
			return false;
		}
		const int32 Minimum = bBoss ? BossSpiritStoneMin
			: (bElite ? EliteSpiritStoneMin : NormalSpiritStoneMin);
		const int32 Maximum = bBoss ? BossSpiritStoneMax
			: (bElite ? EliteSpiritStoneMax : NormalSpiritStoneMax);
		const FString StableKey = FString::Printf(
			TEXT("%s|%s|%s|P5.SpiritStoneDomain"),
			*RunId.ToString(EGuidFormats::DigitsWithHyphens),
			*EncounterId.ToString(),
			*RiskTierId.ToString());
		OutValue = Minimum + static_cast<int32>(FCrc::StrCrc32(*StableKey)
			% static_cast<uint32>(Maximum - Minimum + 1));
		return true;
	}
};
