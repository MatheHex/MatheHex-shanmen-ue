#include "demo_mapRewardEventTypes.h"

#include "demo_mapItemDefinitions.h"

bool Fdemo_mapRewardEventRules::IsValid(
	Edemo_mapRewardEventKind Kind,
	const FGuid& EventId,
	int32 MultiplierBps,
	FName SourceRoleId,
	FString* OutError)
{
	return IsValid(
		Kind,
		EventId,
		MultiplierBps,
		SourceRoleId,
		FGuid(),
		NAME_None,
		NAME_None,
		0,
		OutError);
}

bool Fdemo_mapRewardEventRules::IsValid(
	Edemo_mapRewardEventKind Kind,
	const FGuid& EventId,
	int32 MultiplierBps,
	FName SourceRoleId,
	const FGuid& RareEventId,
	FName RarePolicyId,
	FName RareTierId,
	int64 RareBonusValue,
	FString* OutError)
{
	auto Fail = [OutError](const TCHAR* Message)
	{
		if (OutError)
		{
			*OutError = Message;
		}
		return false;
	};
	const bool bRareEmpty =
		!RareEventId.IsValid() && RarePolicyId.IsNone()
		&& RareTierId.IsNone() && RareBonusValue == 0;
	const bool bRareCarrier =
		RareEventId.IsValid() && !RarePolicyId.IsNone()
		&& !RareTierId.IsNone() && RareBonusValue > 0
		&& !SourceRoleId.IsNone();
	const bool bBossSourceProvenance =
		SourceRoleId
			== FName(TEXT(
				"Reward.SourceRole.Boss.Prototype"));
	const bool bFullMapSourceProvenance =
		SourceRoleId.ToString().StartsWith(
			TEXT("P8.SourceRole."));
	const bool bM01SourceProvenance =
		SourceRoleId.ToString().StartsWith(
			TEXT("M01.Reward.Source."));
	if (!bRareEmpty && !bRareCarrier)
	{
		return Fail(TEXT("Rare reward metadata must be entirely empty or contain event, policy, tier, positive bonus, and source role."));
	}
	if (Kind == Edemo_mapRewardEventKind::None)
	{
		return !EventId.IsValid()
			&& MultiplierBps == NormalMultiplierBps
			&& (bRareCarrier
				|| SourceRoleId.IsNone()
				|| bBossSourceProvenance
				|| bFullMapSourceProvenance
				|| bM01SourceProvenance)
			? true
			: Fail(TEXT("Normal reward metadata must be None/empty/10000 and may carry only an approved Boss, P8, or M01 provenance role."));
	}
	if (Kind == Edemo_mapRewardEventKind::Jackpot)
	{
		return EventId.IsValid()
			&& MultiplierBps == JackpotMultiplierBps
			&& !SourceRoleId.IsNone()
			? true
			: Fail(TEXT("Jackpot reward metadata must have EventId, source role, and multiplier 60000."));
	}
	return Fail(TEXT("Unknown RewardEventKind."));
}

bool Fdemo_mapRewardEventRules::AreStackCompatible(
	FName DefinitionA,
	Edemo_mapRewardEventKind KindA,
	const FGuid& EventIdA,
	int32 MultiplierBpsA,
	FName SourceRoleA,
	FName DefinitionB,
	Edemo_mapRewardEventKind KindB,
	const FGuid& EventIdB,
	int32 MultiplierBpsB,
	FName SourceRoleB)
{
	return DefinitionA == DefinitionB
		&& KindA == KindB
		&& EventIdA == EventIdB
		&& MultiplierBpsA == MultiplierBpsB
		&& SourceRoleA == SourceRoleB;
}

bool Fdemo_mapRewardEventRules::AreStackCompatible(
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
	int64 RareBonusValueB)
{
	const bool bSameRareIdentity =
		RareEventIdA == RareEventIdB
		&& RarePolicyIdA == RarePolicyIdB
		&& RareTierIdA == RareTierIdB;
	const bool bBothNormalRare =
		!RareEventIdA.IsValid() && !RareEventIdB.IsValid()
		&& RareBonusValueA == 0 && RareBonusValueB == 0;
	const bool bSameRareEvent =
		RareEventIdA.IsValid() && RareEventIdB.IsValid()
		&& bSameRareIdentity
		&& RareBonusValueA > 0 && RareBonusValueB > 0;
	return DefinitionA == DefinitionB
		&& KindA == KindB
		&& EventIdA == EventIdB
		&& MultiplierBpsA == MultiplierBpsB
		&& SourceRoleA == SourceRoleB
		&& (bBothNormalRare || bSameRareEvent);
}

bool Fdemo_mapRewardEventRules::TrySplitRareBonus(
	int32 SourceQuantity,
	int32 TakenQuantity,
	int64 SourceRareBonusValue,
	int64& OutTakenRareBonusValue,
	int64& OutRemainingRareBonusValue)
{
	OutTakenRareBonusValue = 0;
	OutRemainingRareBonusValue = 0;
	if (SourceQuantity <= 0 || TakenQuantity <= 0
		|| TakenQuantity > SourceQuantity || SourceRareBonusValue < 0)
	{
		return false;
	}
	if (TakenQuantity == SourceQuantity)
	{
		OutTakenRareBonusValue = SourceRareBonusValue;
		return true;
	}
	const int64 Quotient = SourceRareBonusValue / SourceQuantity;
	const int64 Remainder = SourceRareBonusValue % SourceQuantity;
	OutTakenRareBonusValue =
		Quotient * TakenQuantity
		+ FMath::Min<int64>(Remainder, TakenQuantity);
	OutRemainingRareBonusValue =
		SourceRareBonusValue - OutTakenRareBonusValue;
	return true;
}

bool Fdemo_mapItemSellValueRules::TryCompute(
	FName DefinitionId,
	int32 Quantity,
	int32 RewardValueMultiplierBps,
	int64& OutEffectiveValue,
	FString* OutError)
{
	return TryCompute(
		DefinitionId,
		Quantity,
		RewardValueMultiplierBps,
		0,
		0,
		OutEffectiveValue,
		OutError);
}

bool Fdemo_mapItemSellValueRules::TryCompute(
	FName DefinitionId,
	int32 Quantity,
	int32 RewardValueMultiplierBps,
	int64 RareRewardBonusValue,
	int64& OutEffectiveValue,
	FString* OutError)
{
	return TryCompute(
		DefinitionId,
		Quantity,
		RewardValueMultiplierBps,
		0,
		RareRewardBonusValue,
		OutEffectiveValue,
		OutError);
}

bool Fdemo_mapItemSellValueRules::TryCompute(
	FName DefinitionId,
	int32 Quantity,
	int32 RewardValueMultiplierBps,
	int64 AffixResolvedValue,
	int64 RareRewardBonusValue,
	int64& OutEffectiveValue,
	FString* OutError)
{
	OutEffectiveValue = 0;
	auto Fail = [OutError](const TCHAR* Message)
	{
		if (OutError)
		{
			*OutError = Message;
		}
		return false;
	};
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (!Definition || !Definition->bSellable || Definition->SellPrice <= 0)
	{
		return Fail(TEXT("Item definition is not sellable."));
	}
	if (Quantity <= 0)
	{
		return Fail(TEXT("Sell quantity must be positive."));
	}
	if (AffixResolvedValue < 0 || RareRewardBonusValue < 0)
	{
		return Fail(TEXT("Affix or Rare reward bonus value cannot be negative."));
	}
	if (RewardValueMultiplierBps != Fdemo_mapRewardEventRules::NormalMultiplierBps
		&& RewardValueMultiplierBps != Fdemo_mapRewardEventRules::JackpotMultiplierBps)
	{
		return Fail(TEXT("Unsupported reward value multiplier."));
	}
	if (Definition->SellPrice > MAX_int64 / static_cast<int64>(Quantity))
	{
		return Fail(TEXT("Base stack sell value overflowed int64."));
	}
	const int64 BaseStack =
		Definition->SellPrice * static_cast<int64>(Quantity);
	if (BaseStack > MAX_int64 / static_cast<int64>(RewardValueMultiplierBps))
	{
		return Fail(TEXT("Effective stack sell value overflowed int64."));
	}
	const int64 Scaled =
		BaseStack * static_cast<int64>(RewardValueMultiplierBps);
	// Integer arithmetic is floor-toward-zero; P3's two allowed multipliers divide exactly.
	const int64 JackpotAdjusted =
		Scaled / Fdemo_mapRewardEventRules::NormalMultiplierBps;
	if (JackpotAdjusted > MAX_int64 - AffixResolvedValue
		|| JackpotAdjusted + AffixResolvedValue
			> MAX_int64 - RareRewardBonusValue)
	{
		return Fail(TEXT("Affix or Rare reward effective sell value overflowed int64."));
	}
	OutEffectiveValue =
		JackpotAdjusted + AffixResolvedValue + RareRewardBonusValue;
	return true;
}

bool Fdemo_mapItemBuyValueRules::TryCompute(
	FName DefinitionId,
	int32 Quantity,
	int32 RewardValueMultiplierBps,
	int64 AffixResolvedValue,
	int64 RareRewardBonusValue,
	int64& OutEffectiveValue,
	FString* OutError)
{
	OutEffectiveValue = 0;
	auto Fail = [OutError](const TCHAR* Message)
	{
		if (OutError) *OutError = Message;
		return false;
	};
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (!Definition || !Definition->bPurchasable || Definition->BuyPrice <= 0)
	{
		return Fail(TEXT("Item definition is not purchasable."));
	}
	if (Quantity <= 0)
	{
		return Fail(TEXT("Buy quantity must be positive."));
	}
	if (AffixResolvedValue < 0 || RareRewardBonusValue < 0)
	{
		return Fail(TEXT("Affix or Rare reward bonus value cannot be negative."));
	}
	if (RewardValueMultiplierBps
			!= Fdemo_mapRewardEventRules::NormalMultiplierBps
		&& RewardValueMultiplierBps
			!= Fdemo_mapRewardEventRules::JackpotMultiplierBps)
	{
		return Fail(TEXT("Unsupported reward value multiplier."));
	}
	if (Definition->BuyPrice > MAX_int64 / static_cast<int64>(Quantity))
	{
		return Fail(TEXT("Base stack buy value overflowed int64."));
	}
	const int64 BaseStack =
		Definition->BuyPrice * static_cast<int64>(Quantity);
	if (BaseStack > MAX_int64 / static_cast<int64>(RewardValueMultiplierBps))
	{
		return Fail(TEXT("Effective stack buy value overflowed int64."));
	}
	const int64 Adjusted =
		BaseStack * static_cast<int64>(RewardValueMultiplierBps)
		/ Fdemo_mapRewardEventRules::NormalMultiplierBps;
	if (Adjusted > MAX_int64 - AffixResolvedValue
		|| Adjusted + AffixResolvedValue > MAX_int64 - RareRewardBonusValue)
	{
		return Fail(TEXT("Affix or Rare reward effective buy value overflowed int64."));
	}
	OutEffectiveValue =
		Adjusted + AffixResolvedValue + RareRewardBonusValue;
	return true;
}
