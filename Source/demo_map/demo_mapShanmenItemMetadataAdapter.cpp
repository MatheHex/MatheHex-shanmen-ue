#include "demo_mapShanmenItemMetadataAdapter.h"

#include "demo_mapItemTypes.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardEventTypes.h"

namespace
{
	bool MapRewardEventKind(
		Edemo_mapRewardEventKind Source,
		EShanmenItemRewardEventKind& Out)
	{
		switch (Source)
		{
		case Edemo_mapRewardEventKind::None:
			Out = EShanmenItemRewardEventKind::None;
			return true;
		case Edemo_mapRewardEventKind::Jackpot:
			Out = EShanmenItemRewardEventKind::Jackpot;
			return true;
		default:
			return false;
		}
	}

	bool MapAffixTier(
		Edemo_mapRewardAffixTier Source,
		EShanmenItemRewardAffixTier& Out)
	{
		switch (Source)
		{
		case Edemo_mapRewardAffixTier::Tier1:
			Out = EShanmenItemRewardAffixTier::Tier1;
			return true;
		case Edemo_mapRewardAffixTier::Tier2:
			Out = EShanmenItemRewardAffixTier::Tier2;
			return true;
		case Edemo_mapRewardAffixTier::Tier3:
			Out = EShanmenItemRewardAffixTier::Tier3;
			return true;
		case Edemo_mapRewardAffixTier::None:
		default:
			return false;
		}
	}

	bool MapAffixAcquisition(
		Edemo_mapRewardAffixAcquisition Source,
		EShanmenItemRewardAffixAcquisition& Out)
	{
		switch (Source)
		{
		case Edemo_mapRewardAffixAcquisition::None:
			Out = EShanmenItemRewardAffixAcquisition::None;
			return true;
		case Edemo_mapRewardAffixAcquisition::Natural:
			Out = EShanmenItemRewardAffixAcquisition::Natural;
			return true;
		case Edemo_mapRewardAffixAcquisition::PityGuaranteed:
			Out = EShanmenItemRewardAffixAcquisition::PityGuaranteed;
			return true;
		default:
			return false;
		}
	}

	template <typename SourceType>
	bool Convert(
		const SourceType& Source,
		FShanmenItemRewardMetadata& OutMetadata,
		FString& OutDiagnostic)
	{
		OutMetadata = FShanmenItemRewardMetadata();
		if (!Fdemo_mapRewardEventRules::IsValid(
				Source.RewardEventKind,
				Source.RewardEventId,
				Source.RewardValueMultiplierBps,
				Source.RewardSourceRoleId,
				Source.RareRewardEventId,
				Source.RareRewardPolicyId,
				Source.RareRewardTierId,
				Source.RareRewardBonusValue,
				&OutDiagnostic))
		{
			return false;
		}
		if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
				Source.ItemDefinitionId,
				Source.StackCount,
				Source.AffixSet,
				&OutDiagnostic))
		{
			return false;
		}
		if (!MapRewardEventKind(
				Source.RewardEventKind,
				OutMetadata.RewardEventKind))
		{
			OutDiagnostic = TEXT("Reward metadata contains an unknown event kind.");
			return false;
		}

		OutMetadata.RewardEventId = Source.RewardEventId;
		OutMetadata.RewardValueMultiplierBps =
			Source.RewardValueMultiplierBps;
		OutMetadata.RewardSourceRoleId = Source.RewardSourceRoleId;
		OutMetadata.RareRewardEventId = Source.RareRewardEventId;
		OutMetadata.RareRewardPolicyId = Source.RareRewardPolicyId;
		OutMetadata.RareRewardTierId = Source.RareRewardTierId;
		OutMetadata.RareRewardBonusValue = Source.RareRewardBonusValue;
		OutMetadata.AffixSetEventId = Source.AffixSet.AffixSetEventId;
		OutMetadata.AffixPolicyId = Source.AffixSet.AffixPolicyId;
		if (!MapAffixAcquisition(
				Source.AffixSet.Acquisition,
				OutMetadata.AffixAcquisition))
		{
			OutDiagnostic = TEXT("Reward metadata contains an unknown affix acquisition kind.");
			return false;
		}
		OutMetadata.Affixes.Reserve(Source.AffixSet.Affixes.Num());
		for (const Fdemo_mapResolvedRewardAffix& SourceAffix :
			Source.AffixSet.Affixes)
		{
			FShanmenItemResolvedRewardAffix& Affix =
				OutMetadata.Affixes.AddDefaulted_GetRef();
			Affix.AffixId = SourceAffix.AffixId;
			if (!MapAffixTier(SourceAffix.Tier, Affix.Tier))
			{
				OutDiagnostic = TEXT("Reward metadata contains an unknown affix tier.");
				return false;
			}
			Affix.ResolvedMagnitudeScaled =
				SourceAffix.ResolvedMagnitudeScaled;
			Affix.ResolvedValue = SourceAffix.ResolvedValue;
		}
		if (!OutMetadata.IsValid())
		{
			OutDiagnostic =
				TEXT("Validated product reward metadata did not form a canonical authority value.");
			return false;
		}
		OutDiagnostic.Reset();
		return true;
	}
}

bool Fdemo_mapShanmenItemMetadataAdapter::FromPersistentItem(
	const Fdemo_mapPersistentItemRecord& Source,
	FShanmenItemRewardMetadata& OutMetadata,
	FString& OutDiagnostic)
{
	return Convert(Source, OutMetadata, OutDiagnostic);
}

bool Fdemo_mapShanmenItemMetadataAdapter::FromRuntimeItem(
	const Fdemo_mapRuntimeSettlementItem& Source,
	FShanmenItemRewardMetadata& OutMetadata,
	FString& OutDiagnostic)
{
	return Convert(Source, OutMetadata, OutDiagnostic);
}
