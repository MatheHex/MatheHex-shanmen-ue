#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapRewardEventKind : uint8
{
	None,
	Jackpot
};

struct Fdemo_mapRewardEventRules
{
	static constexpr int32 NormalMultiplierBps = 10000;
	static constexpr int32 JackpotMultiplierBps = 60000;

	static bool IsValid(
		Edemo_mapRewardEventKind Kind,
		const FGuid& EventId,
		int32 MultiplierBps,
		FName SourceRoleId,
		FString* OutError = nullptr);
	static bool IsValid(
		Edemo_mapRewardEventKind Kind,
		const FGuid& EventId,
		int32 MultiplierBps,
		FName SourceRoleId,
		const FGuid& RareEventId,
		FName RarePolicyId,
		FName RareTierId,
		int64 RareBonusValue,
		FString* OutError = nullptr);
	static bool AreStackCompatible(
		FName DefinitionA,
		Edemo_mapRewardEventKind KindA,
		const FGuid& EventIdA,
		int32 MultiplierBpsA,
		FName SourceRoleA,
		FName DefinitionB,
		Edemo_mapRewardEventKind KindB,
		const FGuid& EventIdB,
		int32 MultiplierBpsB,
		FName SourceRoleB);
	static bool AreStackCompatible(
		FName DefinitionA,
		Edemo_mapRewardEventKind KindA,
		const FGuid& EventIdA,
		int32 MultiplierBpsA,
		FName SourceRoleA,
		const FGuid& RareEventIdA,
		FName RarePolicyIdA,
		FName RareTierIdA,
		int64 RareBonusValueA,
		FName DefinitionB,
		Edemo_mapRewardEventKind KindB,
		const FGuid& EventIdB,
		int32 MultiplierBpsB,
		FName SourceRoleB,
		const FGuid& RareEventIdB,
		FName RarePolicyIdB,
		FName RareTierIdB,
		int64 RareBonusValueB);
	static bool TrySplitRareBonus(
		int32 SourceQuantity,
		int32 TakenQuantity,
		int64 SourceRareBonusValue,
		int64& OutTakenRareBonusValue,
		int64& OutRemainingRareBonusValue);
};

/** The single instance-aware sell valuation used by presentation and transaction paths. */
struct Fdemo_mapItemSellValueRules
{
	static bool TryCompute(
		FName DefinitionId,
		int32 Quantity,
		int32 RewardValueMultiplierBps,
		int64& OutEffectiveValue,
		FString* OutError = nullptr);
	static bool TryCompute(
		FName DefinitionId,
		int32 Quantity,
		int32 RewardValueMultiplierBps,
		int64 RareRewardBonusValue,
		int64& OutEffectiveValue,
		FString* OutError = nullptr);
	static bool TryCompute(
		FName DefinitionId,
		int32 Quantity,
		int32 RewardValueMultiplierBps,
		int64 AffixResolvedValue,
		int64 RareRewardBonusValue,
		int64& OutEffectiveValue,
		FString* OutError = nullptr);
};

/** The single instance-aware buy valuation used by Shop presentation and commit. */
struct Fdemo_mapItemBuyValueRules
{
	static bool TryCompute(
		FName DefinitionId,
		int32 Quantity,
		int32 RewardValueMultiplierBps,
		int64 AffixResolvedValue,
		int64 RareRewardBonusValue,
		int64& OutEffectiveValue,
		FString* OutError = nullptr);
};
