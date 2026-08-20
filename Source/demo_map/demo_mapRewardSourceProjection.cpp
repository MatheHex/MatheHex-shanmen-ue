#include "demo_mapRewardSourceProjection.h"

#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardGenerator.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardRareExtreme.h"

const FName Fdemo_mapRewardProjectionTagIds::SourceContainerWood(
	TEXT("Reward.Source.Container.Wood"));
const FName Fdemo_mapRewardProjectionTagIds::SourceContainerOre(
	TEXT("Reward.Source.Container.Ore"));
const FName Fdemo_mapRewardProjectionTagIds::SourceCorpse(
	TEXT("Reward.Source.Corpse"));
const FName Fdemo_mapRewardProjectionTagIds::SourceBoss(
	TEXT("Reward.Source.Boss"));
const FName Fdemo_mapRewardProjectionTagIds::ValueHigh(
	TEXT("Reward.Value.High"));
const FName Fdemo_mapRewardProjectionTagIds::Generated(
	TEXT("Reward.Generated"));
const FName Fdemo_mapRewardProjectionTagIds::SectionEquipment(
	TEXT("Reward.Source.Corpse.Equipment"));
const FName Fdemo_mapRewardProjectionTagIds::SectionBackpack(
	TEXT("Reward.Source.Corpse.Backpack"));
const FName Fdemo_mapRewardProjectionTagIds::SectionBody(
	TEXT("Reward.Source.Corpse.Body"));
const FName Fdemo_mapRewardProjectionTagIds::RiskLow(TEXT("M01.Risk.LOW"));
const FName Fdemo_mapRewardProjectionTagIds::RiskMid(TEXT("M01.Risk.MID"));
const FName Fdemo_mapRewardProjectionTagIds::RiskHigh(TEXT("M01.Risk.HIGH"));
const FName Fdemo_mapRewardProjectionTagIds::Tier1(TEXT("M01.Reward.TIER_1"));
const FName Fdemo_mapRewardProjectionTagIds::Tier2(TEXT("M01.Reward.TIER_2"));
const FName Fdemo_mapRewardProjectionTagIds::Tier3(TEXT("M01.Reward.TIER_3"));

const FName Fdemo_mapRewardProjectionIds::ChestMainWood(
	TEXT("P2.Projection.Chest.Main.Wood"));
const FName Fdemo_mapRewardProjectionIds::ChestMainOre(
	TEXT("P2.Projection.Chest.Main.Ore"));
const FName Fdemo_mapRewardProjectionIds::ChestSideHighValue(
	TEXT("P2.Projection.Chest.Side.HighValue"));
const FName Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard(
	TEXT("P2.Projection.Corpse.Main.Melee.Standard"));
const FName Fdemo_mapRewardProjectionIds::CorpseMainMeleeHeavy(
	TEXT("P2.Projection.Corpse.Main.Melee.Heavy"));
const FName Fdemo_mapRewardProjectionIds::CorpseMainRangedStandard(
	TEXT("P2.Projection.Corpse.Main.Ranged.Standard"));
const FName Fdemo_mapRewardProjectionIds::CorpseSideMeleeEnhanced(
	TEXT("P2.Projection.Corpse.Side.Melee.Enhanced"));
const FName Fdemo_mapRewardProjectionIds::CorpseSideRangedEnhanced(
	TEXT("P2.Projection.Corpse.Side.Ranged.Enhanced"));
const FName Fdemo_mapRewardProjectionIds::CorpseBossPrototype(
	TEXT("Reward.Projection.Corpse.Boss.Prototype"));

const FName Fdemo_mapRewardSourceRoleIds::BossPrototype(
	TEXT("Reward.SourceRole.Boss.Prototype"));

bool Fdemo_mapRewardProjectionSection::IsValid() const
{
	return !SectionId.IsNone()
		&& !SectionTags.IsEmpty()
		&& !SectionTags.Contains(NAME_None)
		&& BudgetWeightBps > 0
		&& BudgetWeightBps <= 10000
		&& Capacity > 0
		&& ResidualRedistributionPriority >= 0;
}

bool Fdemo_mapRewardSourceProjection::IsValid() const
{
	if (ProjectionId.IsNone()
		|| ContentVersionId.IsNone()
		|| ContentDigest.IsEmpty()
		|| StableSourceRoleId.IsNone()
		|| SlotId.IsNone()
		|| BudgetProfileId.IsNone()
		|| JackpotPolicyId.IsNone()
		|| RareExtremePolicyId.IsNone()
		|| AffixPolicyId.IsNone()
		|| SourceTags.IsEmpty()
		|| SourceTags.Contains(NAME_None)
		|| Sections.IsEmpty()
		|| FixedFallbackTableId.IsNone()
		|| BaseSourceValue < 0
		|| MinGeneratedStacks < 0
		|| MaxGeneratedStacks < 0
		|| RequiredEquipmentCount < 0
		|| (MaxGeneratedStacks > 0
			&& MinGeneratedStacks > MaxGeneratedStacks))
	{
		return false;
	}
	int32 Weight = 0;
	int32 Capacity = 0;
	int32 EquipmentCapacity = 0;
	TSet<FName> SectionIds;
	for (const Fdemo_mapRewardProjectionSection& Section : Sections)
	{
		if (!Section.IsValid() || SectionIds.Contains(Section.SectionId))
		{
			return false;
		}
		SectionIds.Add(Section.SectionId);
		Weight += Section.BudgetWeightBps;
		Capacity += Section.Capacity;
		if (Section.RuntimeSection
			== Edemo_mapRuntimeContainerSection::Equipment)
		{
			EquipmentCapacity += Section.Capacity;
		}
	}
	return Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
		ContentVersionId,
		ContentDigest)
		&& Weight == 10000
		&& (MaxGeneratedStacks == 0
			|| MaxGeneratedStacks <= Capacity)
		&& RequiredEquipmentCount <= EquipmentCapacity;
}

bool Fdemo_mapRewardSourceAcceptanceReceipt::IsValid() const
{
	if (!RunId.IsValid()
		|| StableSourceRoleId.IsNone()
		|| BudgetProfileId.IsNone()
		|| EffectiveSeed == 0
		|| RandomizedBudget < 0
		|| GeneratedTotalValue < 0
		|| ResidualValue < 0
		|| PityStateIn < 0 || PityStateIn > 3
		|| PityStateOut < 0 || PityStateOut > 3
		|| PlannedStacks.IsEmpty())
	{
		return false;
	}
	return bLegacyCompatibilityView
		? (!ContentVersionId.IsNone()
			&& !ContentDigest.IsEmpty()
			&& Fdemo_mapItemDefinitions::IsKnownContentIdentity(
				ContentVersionId,
				ContentDigest))
		: (!ProjectionId.IsNone()
			&& !DistributionProfileId.IsNone()
			&& Fdemo_mapItemDefinitions::IsKnownContentIdentity(
				ContentVersionId,
				ContentDigest));
}

Fdemo_mapRewardSourceAcceptanceReceipt
Fdemo_mapRewardSourceAcceptanceReceipt::FromProjection(
	const Fdemo_mapRewardSourceProjection& Projection,
	const Fdemo_mapRewardSourceProjectionResult& Result)
{
	Fdemo_mapRewardSourceAcceptanceReceipt Receipt;
	Receipt.RunId = Result.Trace.RunId;
	Receipt.StableSourceRoleId = Result.Trace.StableSourceRoleId;
	Receipt.SlotId = Projection.SlotId;
	Receipt.ProjectionId = Result.Trace.ProjectionId;
	Receipt.DistributionProfileId = Projection.DistributionProfileId;
	Receipt.ContentVersionId = Result.Trace.ContentVersionId;
	Receipt.ContentDigest = Result.Trace.ContentDigest;
	Receipt.BudgetProfileId = Projection.BudgetProfileId;
	Receipt.MarkerId = Projection.MarkerId;
	Receipt.EncounterId = Projection.EncounterId;
	Receipt.JackpotPolicyId = Projection.JackpotPolicyId;
	Receipt.RareExtremePolicyId = Projection.RareExtremePolicyId;
	Receipt.AffixPolicyId = Projection.AffixPolicyId;
	Receipt.EffectiveSeed = Result.Trace.EffectiveSeed;
	Receipt.RandomizedBudget = Result.Trace.RandomizedBudget;
	Receipt.GeneratedTotalValue = Result.Trace.GeneratedTotalValue;
	Receipt.ResidualValue = Result.Trace.ResidualValue;
	Receipt.PityStateIn = Result.Trace.PityStateIn;
	Receipt.PityStateOut = Result.Trace.PityStateOut;
	Receipt.bPityCommitRequired = Result.Trace.PityDecisions.ContainsByPredicate(
		[](const Fdemo_mapRewardAffixPityDecision& Decision)
		{
			return Decision.bCommitRequired;
		});
	Receipt.bFallbackUsed = Result.Trace.bFallbackUsed;
	Receipt.PlannedStacks = Result.PlannedStacks;
	return Receipt;
}

Fdemo_mapRewardSourceAcceptanceReceipt
Fdemo_mapRewardSourceAcceptanceReceipt::FromGeneratedResult(
	FName StableSourceRoleId,
	const Fdemo_mapRewardGenerationResult& Result)
{
	Fdemo_mapRewardSourceAcceptanceReceipt Receipt;
	Receipt.RunId = Result.Trace.RunId;
	Receipt.StableSourceRoleId = StableSourceRoleId;
	Receipt.SlotId = StableSourceRoleId;
	Receipt.DistributionProfileId = Result.Trace.BudgetProfileId;
	Receipt.ContentVersionId = Result.Trace.ContentVersionId;
	Receipt.ContentDigest = Result.Trace.ContentDigest;
	Receipt.BudgetProfileId = Result.Trace.BudgetProfileId;
	Receipt.EffectiveSeed = Result.Trace.EffectiveSeed;
	Receipt.RandomizedBudget = Result.Trace.RandomizedBudget;
	Receipt.GeneratedTotalValue = Result.Trace.GeneratedTotalValue;
	Receipt.ResidualValue = Result.Trace.ResidualValue;
	Receipt.bFallbackUsed = false;
	Receipt.bLegacyCompatibilityView = true;
	Receipt.PlannedStacks = Result.PlannedStacks;
	return Receipt;
}

namespace
{
	bool HasItemTag(const Fdemo_mapRewardPoolEntry& Entry, FName Tag)
	{
		return Entry.ItemTags.Contains(Tag);
	}

	bool IsEquipment(const Fdemo_mapRewardPoolEntry& Entry)
	{
		return HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemEquipmentWeapon)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemEquipmentRobe)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemEquipmentAccessory)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemEquipmentBackpack);
	}

	bool IsPortable(const Fdemo_mapRewardPoolEntry& Entry)
	{
		return HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemConsumablePill)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemMaterialWood)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemMaterialOre)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemEquipmentAccessory);
	}

	bool IsBody(const Fdemo_mapRewardPoolEntry& Entry)
	{
		return HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemConsumablePill)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemBodyBone)
			|| HasItemTag(Entry, Fdemo_mapRewardTagIds::ItemBodyInnerCore);
	}
}

const TArray<Fdemo_mapRewardSourceProjection>&
Fdemo_mapRewardSourceProjectionRegistry::GetAll()
{
	return Fdemo_mapItemDefinitions::GetGeneratedRewardProjectionProfiles();
}

const Fdemo_mapRewardSourceProjection*
Fdemo_mapRewardSourceProjectionRegistry::Find(FName ProjectionId)
{
	return GetAll().FindByPredicate(
		[ProjectionId](const auto& Projection)
		{
			return Projection.ProjectionId == ProjectionId;
		});
}

const Fdemo_mapRewardSourceProjection*
Fdemo_mapRewardSourceProjectionRegistry::FindChest(int32 ChestIndex)
{
	return GetAll().IsValidIndex(ChestIndex) && ChestIndex < 3
		? &GetAll()[ChestIndex]
		: nullptr;
}

const Fdemo_mapRewardSourceProjection*
Fdemo_mapRewardSourceProjectionRegistry::FindBossPrototype()
{
	return Find(Fdemo_mapRewardProjectionIds::CorpseBossPrototype);
}

const Fdemo_mapRewardSourceProjection*
Fdemo_mapRewardSourceProjectionRegistry::FindCorpseByFallbackTable(
	FName FixedTableId)
{
	return GetAll().FindByPredicate(
		[FixedTableId](const auto& Projection)
		{
			return !Projection.EncounterId.IsNone()
				&& Projection.FixedFallbackTableId == FixedTableId;
		});
}

TArray<Fdemo_mapRewardPoolEntry>
Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
	const Fdemo_mapRewardSourceProjection& Projection,
	const Fdemo_mapRewardProjectionSection& Section)
{
	TArray<Fdemo_mapRewardPoolEntry> Result;
	for (const Fdemo_mapRewardPoolEntry& Source :
		Fdemo_mapItemDefinitions::GetGeneratedRewardPool())
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Source.DefinitionId);
		const bool bBodySection =
			Section.RuntimeSection == Edemo_mapRuntimeContainerSection::Body;
		const bool bTierAllowed = bBodySection || !Definition
			? true
			: Projection.SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::Tier1)
				? Definition->Level == 1
				: Projection.SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::Tier2)
					? Definition->Level >= 1 && Definition->Level <= 2
					: Projection.SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::Tier3)
						? Definition->Level >= 2
						: true;
		const bool bAllowed =
			Section.RuntimeSection == Edemo_mapRuntimeContainerSection::Chest
				? (Projection.SourceTags.Contains(
						Fdemo_mapRewardProjectionTagIds::SourceContainerWood)
						? HasItemTag(Source, Fdemo_mapRewardTagIds::ItemMaterialWood)
						: Projection.SourceTags.Contains(
							Fdemo_mapRewardProjectionTagIds::SourceContainerOre)
							? HasItemTag(Source, Fdemo_mapRewardTagIds::ItemMaterialOre)
							: true)
				: Section.RuntimeSection == Edemo_mapRuntimeContainerSection::Equipment
					? (IsEquipment(Source)
						&& (!Projection.SourceTags.Contains(
								Fdemo_mapRewardProjectionTagIds::SourceBoss)
							|| !HasItemTag(
								Source,
								Fdemo_mapRewardTagIds::
									ItemEquipmentBackpack)))
					: Section.RuntimeSection == Edemo_mapRuntimeContainerSection::Backpack
						? IsPortable(Source)
						: IsBody(Source);
		if (!bAllowed || !bTierAllowed)
		{
			continue;
		}
		Fdemo_mapRewardPoolEntry Copy = Source;
		Copy.EntryId = FName(*FString::Printf(
			TEXT("P2.%s.%s"),
			*Section.SectionId.ToString(),
			*Source.EntryId.ToString()));
		Copy.RequiredSourceTags = Projection.SourceTags;
		Copy.RequiredSourceTags.Append(Section.SectionTags);
		Copy.ExcludedSourceTags.Reset();
		Result.Add(MoveTemp(Copy));
	}
	return Result;
}

bool Fdemo_mapRewardSourceProjectionRegistry::Validate(FString* OutError)
{
	TSet<FName> ProjectionIds;
	TSet<FName> RoleIds;
	for (const Fdemo_mapRewardSourceProjection& Projection : GetAll())
	{
		if (!Projection.IsValid()
			|| ProjectionIds.Contains(Projection.ProjectionId)
			|| RoleIds.Contains(Projection.StableSourceRoleId)
			|| !Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
				Projection.BudgetProfileId)
			|| !Fdemo_mapRewardJackpotPolicyRegistry::Find(
				Projection.JackpotPolicyId)
			|| !Fdemo_mapRewardRareExtremePolicyRegistry::Find(
				Projection.RareExtremePolicyId)
			|| Projection.AffixPolicyId
				!= Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId
			|| !Fdemo_mapRewardAffixPolicyRegistry::Validate()
			|| !Fdemo_mapItemDefinitions::FindFixedLootProfile(
				Projection.FixedFallbackTableId))
		{
			if (OutError)
			{
				*OutError = FString::Printf(
					TEXT("Invalid or duplicated Reward Source Projection: %s"),
					*Projection.ProjectionId.ToString());
			}
			return false;
		}
		const Fdemo_mapRewardBudgetProfile* Budget =
			Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
				Projection.BudgetProfileId);
		if (Projection.BaseSourceValue > 0
			&& (!Budget
				|| Budget->BaseValue
					!= Projection.BaseSourceValue))
		{
			if (OutError)
			{
				*OutError = TEXT(
					"Projection BaseSourceValue does not match its P1 budget profile.");
			}
			return false;
		}
		ProjectionIds.Add(Projection.ProjectionId);
		RoleIds.Add(Projection.StableSourceRoleId);
		for (const auto& Section : Projection.Sections)
		{
			if (BuildSectionPool(Projection, Section).IsEmpty())
			{
				if (OutError) *OutError = TEXT("Projection Section has no eligible pool.");
				return false;
			}
		}
	}
	if (GetAll().Num() != 9)
	{
		if (OutError) *OutError = TEXT("P2/P7 require three Chest, five standard Corpse, and one Boss Corpse projection.");
		return false;
	}
	return true;
}

Fdemo_mapRewardSourceProjectionResult
Fdemo_mapRewardSourceProjectionPlanner::Plan(
	const Fdemo_mapRewardSourceProjection& Projection,
	FGuid RunId,
	int32 PityStateIn)
{
	Fdemo_mapRewardSourceProjectionResult Result;
	Result.Trace.ProjectionId = Projection.ProjectionId;
	Result.Trace.ContentVersionId = Projection.ContentVersionId;
	Result.Trace.ContentDigest = Projection.ContentDigest;
	Result.Trace.StableSourceRoleId = Projection.StableSourceRoleId;
	Result.Trace.RunId = RunId;
	Result.Trace.PityStateIn = PityStateIn;
	if (!Projection.IsValid()
		|| !Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			Projection.ContentVersionId,
			Projection.ContentDigest)
		|| !RunId.IsValid())
	{
		Result.Trace.Diagnostic = TEXT("invalid_projection_request");
		return Result;
	}
	const Fdemo_mapRewardBudgetProfile* Profile =
		Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
			Projection.BudgetProfileId);
	if (!Profile)
	{
		Result.Status = Edemo_mapRewardGenerationStatus::InvalidBudgetProfile;
		Result.Trace.Diagnostic = TEXT("unknown_budget_profile");
		return Result;
	}
	const uint64 Seed = Fdemo_mapRewardGenerator::ComputeStableSeed(
		RunId,
		Projection.StableSourceRoleId,
		Projection.BudgetProfileId);
	Result.Trace.EffectiveSeed = Seed;
	Fdemo_mapRewardGenerationRequest ProbeRequest;
	ProbeRequest.RequestId = FName(*FString::Printf(
		TEXT("%s.Budget"),
		*Projection.ProjectionId.ToString()));
	ProbeRequest.RunId = RunId;
	ProbeRequest.LootSourceId = Projection.StableSourceRoleId;
	ProbeRequest.ContentVersionId = Projection.ContentVersionId;
	ProbeRequest.ContentDigest = Projection.ContentDigest;
	ProbeRequest.BudgetProfileId = Projection.BudgetProfileId;
	ProbeRequest.SourceTags = Projection.SourceTags;
	ProbeRequest.SourceTags.Append(Projection.Sections[0].SectionTags);
	ProbeRequest.StableSeed = Seed;
	ProbeRequest.TargetSection = Projection.Sections[0].RuntimeSection;
	ProbeRequest.TargetCapacity = Projection.Sections[0].Capacity;
	const TArray<Fdemo_mapRewardPoolEntry> ProbePool =
		Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
			Projection,
			Projection.Sections[0]);
	const Fdemo_mapRewardGenerationResult Probe =
		Fdemo_mapRewardGenerator::GenerateWithData(
			ProbeRequest,
			*Profile,
			ProbePool);
	if (!Probe.IsSuccess())
	{
		Result.Status = Probe.Status;
		Result.Trace.Diagnostic = Probe.Trace.Diagnostic;
		return Result;
	}
	Result.Trace.MultiplierBps = Probe.Trace.MultiplierBps;
	Result.Trace.NormalRandomizedBudget = Probe.Trace.RandomizedBudget;
	const Fdemo_mapRewardRareExtremePolicy* RarePolicy =
		Fdemo_mapRewardRareExtremePolicyRegistry::Find(
			Projection.RareExtremePolicyId);
	if (!RarePolicy)
	{
		Result.Status = Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic = TEXT("unknown_rare_extreme_policy");
		return Result;
	}
	Fdemo_mapRewardRareExtremeDecision Rare =
		Fdemo_mapRewardRareExtreme::Decide(
			*RarePolicy,
			RunId,
			Projection.StableSourceRoleId,
			Projection.ProjectionId,
			Profile->BaseValue);
	if (!Rare.IsSuccess())
	{
		Result.Status = Rare.Diagnostic.Contains(TEXT("overflow"))
			? Edemo_mapRewardGenerationStatus::ArithmeticOverflow
			: Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic =
			TEXT("rare_extreme_decision_failed:") + Rare.Diagnostic;
		return Result;
	}
	Result.Trace.RandomizedBudget =
		Rare.bHit ? Rare.TargetValue : Probe.Trace.RandomizedBudget;

	int64 Assigned = 0;
	int64 Carry = 0;
	for (int32 Index = 0; Index < Projection.Sections.Num(); ++Index)
	{
		const Fdemo_mapRewardProjectionSection& SectionDef =
			Projection.Sections[Index];
		Fdemo_mapRewardProjectionSectionTrace SectionTrace;
		SectionTrace.SectionId = SectionDef.SectionId;
		SectionTrace.InitialBudget =
			Index + 1 == Projection.Sections.Num()
				? Result.Trace.RandomizedBudget - Assigned
				: (Result.Trace.RandomizedBudget
					* SectionDef.BudgetWeightBps) / 10000;
		Assigned += SectionTrace.InitialBudget;
		SectionTrace.RedistributedIn = Carry;
		SectionTrace.FinalBudget = SectionTrace.InitialBudget + Carry;
		Carry = 0;

		Fdemo_mapRewardBudgetProfile SectionProfile = *Profile;
		SectionProfile.BaseValue = SectionTrace.FinalBudget;
		SectionProfile.MinMultiplierBps = 10000;
		SectionProfile.MaxMultiplierBps = 10000;
		Fdemo_mapRewardGenerationRequest SectionRequest;
		SectionRequest.RequestId = FName(*FString::Printf(
			TEXT("%s.%s"),
			*Projection.ProjectionId.ToString(),
			*SectionDef.SectionId.ToString()));
		SectionRequest.RunId = RunId;
		SectionRequest.LootSourceId = Projection.StableSourceRoleId;
		SectionRequest.ContentVersionId = Projection.ContentVersionId;
		SectionRequest.ContentDigest = Projection.ContentDigest;
		SectionRequest.BudgetProfileId = Projection.BudgetProfileId;
		SectionRequest.SourceTags = Projection.SourceTags;
		SectionRequest.SourceTags.Append(SectionDef.SectionTags);
		SectionRequest.StableSeed = Fdemo_mapRewardGenerator::ComputeStableSeed(
			RunId,
			SectionRequest.RequestId,
			Projection.BudgetProfileId);
		SectionRequest.TargetSection = SectionDef.RuntimeSection;
		// Rare Extreme keeps the real section capacities as upper bounds while
		// reserving one product-takeable carrier candidate per required section.
		SectionRequest.TargetCapacity =
			Rare.bHit ? 1 : SectionDef.Capacity;
		const Fdemo_mapRewardGenerationResult SectionResult =
			Fdemo_mapRewardGenerator::GenerateWithData(
				SectionRequest,
				SectionProfile,
				Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
					Projection,
					SectionDef));
		if (!SectionResult.IsSuccess()
			|| (SectionDef.bRequiredNonEmpty
				&& SectionResult.PlannedStacks.IsEmpty()))
		{
			Result.Status = SectionResult.Status;
			Result.Trace.Diagnostic = FString::Printf(
				TEXT("section_failed:%s:%s"),
				*SectionDef.SectionId.ToString(),
				*SectionResult.Trace.Diagnostic);
			return Result;
		}
		const int32 SlotBase = 0;
		for (Fdemo_mapRewardPlannedStack Stack :
			SectionResult.PlannedStacks)
		{
			Stack.Section = SectionDef.RuntimeSection;
			Stack.SlotIndex = SlotBase
				+ Result.PlannedStacks.FilterByPredicate(
					[&SectionDef](const auto& Existing)
					{
						return Existing.Section == SectionDef.RuntimeSection;
					}).Num();
			Result.PlannedStacks.Add(Stack);
		}
		SectionTrace.GeneratedValue =
			SectionResult.Trace.GeneratedTotalValue;
		SectionTrace.ResidualValue =
			SectionResult.Trace.ResidualValue;
		Carry = SectionTrace.ResidualValue;
		Result.Trace.GeneratedTotalValue +=
			SectionTrace.GeneratedValue;
		Result.Trace.Sections.Add(SectionTrace);
	}
	Result.Trace.ResidualValue =
		Result.Trace.RandomizedBudget
		- Result.Trace.GeneratedTotalValue;
	Fdemo_mapRewardAffixPlanTrace AffixTrace;
	if (!Fdemo_mapRewardAffixPlanner::Apply(
		Fdemo_mapRewardAffixPolicyRegistry::GetDefault(),
		RunId,
		Projection.StableSourceRoleId,
		Projection.ProjectionId,
		Result.Trace.ResidualValue,
		PityStateIn,
		Result.PlannedStacks,
		AffixTrace))
	{
		Result.Status = Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic =
			TEXT("affix_planning_failed:") + AffixTrace.Diagnostic;
		Result.PlannedStacks.Reset();
		return Result;
	}
	Result.Trace.AffixPolicyId = AffixTrace.PolicyId;
	Result.Trace.AffixBudgetBefore = AffixTrace.BudgetBefore;
	Result.Trace.AffixValueSpent = AffixTrace.AffixValueSpent;
	Result.Trace.AffixBudgetAfter = AffixTrace.BudgetAfter;
	Result.Trace.AffixEligibleEquipmentCount =
		AffixTrace.EligibleEquipmentCount;
	Result.Trace.AffixedEquipmentCount =
		AffixTrace.AffixedEquipmentCount;
	Result.Trace.PityDecisions = AffixTrace.PityDecisions;
	Result.Trace.PityStateOut = AffixTrace.PityStateOut;
	Result.Trace.GeneratedTotalValue += AffixTrace.AffixValueSpent;
	Result.Trace.ResidualValue = AffixTrace.BudgetAfter;
	if (!Fdemo_mapRewardRareExtreme::Annotate(
		*RarePolicy,
		Projection.StableSourceRoleId,
		Rare,
		Result.PlannedStacks))
	{
		Result.Status = Rare.Diagnostic.Contains(TEXT("overflow"))
			? Edemo_mapRewardGenerationStatus::ArithmeticOverflow
			: Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic =
			TEXT("rare_extreme_annotation_failed:") + Rare.Diagnostic;
		Result.PlannedStacks.Reset();
		return Result;
	}
	Result.Trace.RareExtremePolicyId = Rare.PolicyId;
	Result.Trace.RareRollSeed = Rare.RollSeed;
	Result.Trace.RareTierSeed = Rare.TierSeed;
	Result.Trace.RareCarrierSelectionSeed = Rare.CarrierSelectionSeed;
	Result.Trace.RareRoll = Rare.Roll;
	Result.Trace.RareTierRoll = Rare.TierRoll;
	Result.Trace.bRareExtremeHit = Rare.bHit;
	Result.Trace.RareExtremeTierId = Rare.TierId;
	Result.Trace.RareExtremeTierMultiplierBps =
		Rare.TierMultiplierBps;
	Result.Trace.RareExtremeBaseSourceValue = Rare.BaseSourceValue;
	Result.Trace.RareExtremeTargetValue = Rare.TargetValue;
	Result.Trace.RareExtremeEventId = Rare.EventId;
	Result.Trace.RareExtremeBaseGeneratedValue =
		Rare.BaseGeneratedValue;
	Result.Trace.RareExtremeBonusPoolValue = Rare.BonusPoolValue;
	Result.Trace.RareExtremeCarrierCount = Rare.CarrierCount;
	Result.Trace.RareExtremeCarrierPlannedEntryIndexes =
		Rare.CarrierPlannedEntryIndexes;
	const Fdemo_mapRewardJackpotPolicy* JackpotPolicy =
		Fdemo_mapRewardJackpotPolicyRegistry::Find(
			Projection.JackpotPolicyId);
	if (!JackpotPolicy)
	{
		Result.Status = Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic = TEXT("unknown_jackpot_policy");
		Result.PlannedStacks.Reset();
		return Result;
	}
	const Fdemo_mapRewardJackpotDecision Jackpot =
		Fdemo_mapRewardJackpot::Annotate(
			*JackpotPolicy,
			RunId,
			Projection.StableSourceRoleId,
			Projection.ProjectionId,
			Result.PlannedStacks);
	if (!Jackpot.IsSuccess())
	{
		Result.Status = Jackpot.Diagnostic.Contains(TEXT("overflow"))
			? Edemo_mapRewardGenerationStatus::ArithmeticOverflow
			: Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic =
			TEXT("jackpot_annotation_failed:") + Jackpot.Diagnostic;
		Result.PlannedStacks.Reset();
		return Result;
	}
	Result.Trace.JackpotPolicyId = Jackpot.PolicyId;
	Result.Trace.JackpotRollSeed = Jackpot.RollSeed;
	Result.Trace.JackpotSelectionSeed = Jackpot.SelectionSeed;
	Result.Trace.JackpotRoll = Jackpot.Roll;
	Result.Trace.bJackpotHit = Jackpot.bHit;
	Result.Trace.JackpotSelectedPlannedEntryIndex =
		Jackpot.SelectedPlannedEntryIndex;
	Result.Trace.JackpotEventId = Jackpot.EventId;
	Result.Trace.JackpotSelectedBaseValue = Jackpot.SelectedBaseValue;
	Result.Trace.JackpotBonusValue = Jackpot.JackpotBonusValue;
	Result.Trace.FinalEffectiveRewardValue =
		Jackpot.FinalEffectiveRewardValue
		+ AffixTrace.AffixValueSpent
		+ Rare.BonusPoolValue;
	if (Projection.SourceTags.Contains(
			Fdemo_mapRewardProjectionTagIds::SourceBoss)
		|| Projection.StableSourceRoleId.ToString().
			StartsWith(TEXT("P8.SourceRole.")))
	{
		for (Fdemo_mapRewardPlannedStack& Stack :
			Result.PlannedStacks)
		{
			Stack.RewardSourceRoleId =
				Projection.StableSourceRoleId;
		}
	}
	int32 EquipmentCount = 0;
	for (const Fdemo_mapRewardPlannedStack& Stack :
		Result.PlannedStacks)
	{
		EquipmentCount += Stack.Section
				== Edemo_mapRuntimeContainerSection::Equipment
			? 1 : 0;
	}
	if ((Projection.MinGeneratedStacks > 0
			&& Result.PlannedStacks.Num()
				< Projection.MinGeneratedStacks)
		|| (Projection.MaxGeneratedStacks > 0
			&& Result.PlannedStacks.Num()
				> Projection.MaxGeneratedStacks)
		|| EquipmentCount < Projection.RequiredEquipmentCount)
	{
		Result.Status =
			Edemo_mapRewardGenerationStatus::InvalidRequest;
		Result.Trace.Diagnostic =
			TEXT("projection_output_contract_failed");
		Result.PlannedStacks.Reset();
		return Result;
	}
	Result.Status = Edemo_mapRewardGenerationStatus::Success;
	Result.Trace.Diagnostic = TEXT("success");
	return Result;
}

TArray<Fdemo_mapRuntimeContainerSeedEntry>
Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(
	const Fdemo_mapRewardSourceProjectionResult& Result)
{
	TArray<Fdemo_mapRuntimeContainerSeedEntry> Seed;
	if (!Result.IsSuccess())
	{
		return Seed;
	}
	TSet<int32> OccupiedEquipmentSlots;
	TSet<int32> OccupiedBackpackSlots;
	const auto ResolveEquipmentSlot =
		[](FName DefinitionId) -> int32
		{
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(DefinitionId);
			if (!Definition)
			{
				return INDEX_NONE;
			}
			if (Definition->EquipmentSlotId == Fdemo_mapItemIds::WeaponSlot)
			{
				return 0;
			}
			if (Definition->EquipmentSlotId == Fdemo_mapItemIds::ArmorSlot)
			{
				return 1;
			}
			if (Definition->EquipmentSlotId == Fdemo_mapItemIds::AccessorySlot)
			{
				return 2;
			}
			if (Definition->EquipmentSlotId == Fdemo_mapItemIds::SpatialRingSlot)
			{
				return 3;
			}
			if (Definition->EquipmentSlotId == Fdemo_mapItemIds::BackpackSlot)
			{
				return 4;
			}
			return INDEX_NONE;
		};
	const auto FindFreeBackpackSlot =
		[&OccupiedBackpackSlots]() -> int32
		{
			for (int32 SlotIndex = 0;
				SlotIndex
					< Fdemo_mapSearchContainerPrototypeConfig::
						CorpseRuntimeBackpackCapacity;
				++SlotIndex)
			{
				if (!OccupiedBackpackSlots.Contains(SlotIndex))
				{
					return SlotIndex;
				}
			}
			return INDEX_NONE;
		};

	for (const Fdemo_mapRewardPlannedStack& Stack : Result.PlannedStacks)
	{
		Fdemo_mapRuntimeContainerSeedEntry Entry;
		Entry.Section = Stack.Section;
		Entry.SlotIndex = Stack.SlotIndex;
		if (Stack.Section == Edemo_mapRuntimeContainerSection::Equipment)
		{
			const int32 CanonicalEquipmentSlot =
				ResolveEquipmentSlot(Stack.DefinitionId);
			if (CanonicalEquipmentSlot != INDEX_NONE
				&& !OccupiedEquipmentSlots.Contains(
					CanonicalEquipmentSlot))
			{
				Entry.SlotIndex = CanonicalEquipmentSlot;
				OccupiedEquipmentSlots.Add(CanonicalEquipmentSlot);
			}
			else
			{
				Entry.Section =
					Edemo_mapRuntimeContainerSection::Backpack;
				Entry.SlotIndex = FindFreeBackpackSlot();
				if (Entry.SlotIndex != INDEX_NONE)
				{
					OccupiedBackpackSlots.Add(Entry.SlotIndex);
				}
			}
		}
		else if (Stack.Section
			== Edemo_mapRuntimeContainerSection::Backpack)
		{
			if (Entry.SlotIndex < 0
				|| Entry.SlotIndex
					>= Fdemo_mapSearchContainerPrototypeConfig::
						CorpseRuntimeBackpackCapacity
				|| OccupiedBackpackSlots.Contains(Entry.SlotIndex))
			{
				Entry.SlotIndex = FindFreeBackpackSlot();
			}
			if (Entry.SlotIndex != INDEX_NONE)
			{
				OccupiedBackpackSlots.Add(Entry.SlotIndex);
			}
		}
		Entry.DefinitionId = Stack.DefinitionId;
		Entry.StackCount = Stack.StackCount;
		Entry.RewardEventKind = Stack.RewardEventKind;
		Entry.RewardEventId = Stack.RewardEventId;
		Entry.RewardValueMultiplierBps =
			Stack.RewardValueMultiplierBps;
		Entry.RewardSourceRoleId = Stack.RewardSourceRoleId;
		Entry.RareRewardEventId = Stack.RareRewardEventId;
		Entry.RareRewardPolicyId = Stack.RareRewardPolicyId;
		Entry.RareRewardTierId = Stack.RareRewardTierId;
		Entry.RareRewardBonusValue = Stack.RareRewardBonusValue;
		Entry.AffixSet = Stack.AffixSet;
		if (Entry.SlotIndex != INDEX_NONE)
		{
			Seed.Add(Entry);
		}
	}
	return Seed;
}
