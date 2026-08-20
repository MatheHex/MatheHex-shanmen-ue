#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardGenerationTypes.h"

struct Fdemo_mapRewardProjectionTagIds
{
	static const FName SourceContainerWood;
	static const FName SourceContainerOre;
	static const FName SourceCorpse;
	static const FName SourceBoss;
	static const FName ValueHigh;
	static const FName Generated;
	static const FName SectionEquipment;
	static const FName SectionBackpack;
	static const FName SectionBody;
	static const FName RiskLow;
	static const FName RiskMid;
	static const FName RiskHigh;
	static const FName Tier1;
	static const FName Tier2;
	static const FName Tier3;
};

struct Fdemo_mapRewardProjectionIds
{
	static const FName ChestMainWood;
	static const FName ChestMainOre;
	static const FName ChestSideHighValue;
	static const FName CorpseMainMeleeStandard;
	static const FName CorpseMainMeleeHeavy;
	static const FName CorpseMainRangedStandard;
	static const FName CorpseSideMeleeEnhanced;
	static const FName CorpseSideRangedEnhanced;
	static const FName CorpseBossPrototype;
};

struct Fdemo_mapRewardSourceRoleIds
{
	static const FName BossPrototype;
};

struct Fdemo_mapRewardProjectionSection
{
	FName SectionId = NAME_None;
	Edemo_mapRuntimeContainerSection RuntimeSection =
		Edemo_mapRuntimeContainerSection::Chest;
	TArray<FName> SectionTags;
	int32 BudgetWeightBps = 0;
	int32 Capacity = 0;
	bool bRequiredNonEmpty = false;
	int32 ResidualRedistributionPriority = 0;

	bool IsValid() const;
};

struct Fdemo_mapRewardSourceProjection
{
	FName ProjectionId = NAME_None;
	/** P73.4 binds every current source to its immutable distribution slot. */
	FName DistributionProfileId = NAME_None;
	FName SlotId = NAME_None;
	/** Manifest identity frozen into current requests and accepted source receipts. */
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
	FName StableSourceRoleId = NAME_None;
	FName MarkerId = NAME_None;
	FName EncounterId = NAME_None;
	FName BudgetProfileId = NAME_None;
	FName JackpotPolicyId = NAME_None;
	FName RareExtremePolicyId = NAME_None;
	FName AffixPolicyId = NAME_None;
	TArray<FName> SourceTags;
	TArray<Fdemo_mapRewardProjectionSection> Sections;
	FName FixedFallbackTableId = NAME_None;
	bool bAllowFixedFallbackOnFailure = true;
	int64 BaseSourceValue = 0;
	int32 MinGeneratedStacks = 0;
	int32 MaxGeneratedStacks = 0;
	int32 RequiredEquipmentCount = 0;
	FString SourceDisplayLabel;

	bool IsValid() const;
};

struct Fdemo_mapRewardProjectionSectionTrace
{
	FName SectionId = NAME_None;
	int64 InitialBudget = 0;
	int64 RedistributedIn = 0;
	int64 FinalBudget = 0;
	int64 GeneratedValue = 0;
	int64 ResidualValue = 0;
};

struct Fdemo_mapRewardSourceProjectionTrace
{
	FName ProjectionId = NAME_None;
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
	FName StableSourceRoleId = NAME_None;
	FGuid RunId;
	uint64 EffectiveSeed = 0;
	int32 MultiplierBps = 0;
	int64 RandomizedBudget = 0;
	int64 NormalRandomizedBudget = 0;
	int64 GeneratedTotalValue = 0;
	int64 ResidualValue = 0;
	bool bFallbackUsed = false;
	FName JackpotPolicyId = NAME_None;
	uint64 JackpotRollSeed = 0;
	uint64 JackpotSelectionSeed = 0;
	int32 JackpotRoll = INDEX_NONE;
	bool bJackpotHit = false;
	int32 JackpotSelectedPlannedEntryIndex = INDEX_NONE;
	FGuid JackpotEventId;
	int64 JackpotSelectedBaseValue = 0;
	int64 JackpotBonusValue = 0;
	int64 FinalEffectiveRewardValue = 0;
	FName RareExtremePolicyId = NAME_None;
	uint64 RareRollSeed = 0;
	uint64 RareTierSeed = 0;
	uint64 RareCarrierSelectionSeed = 0;
	int32 RareRoll = INDEX_NONE;
	int32 RareTierRoll = INDEX_NONE;
	bool bRareExtremeHit = false;
	FName RareExtremeTierId = NAME_None;
	int32 RareExtremeTierMultiplierBps = 0;
	int64 RareExtremeBaseSourceValue = 0;
	int64 RareExtremeTargetValue = 0;
	FGuid RareExtremeEventId;
	int64 RareExtremeBaseGeneratedValue = 0;
	int64 RareExtremeBonusPoolValue = 0;
	int32 RareExtremeCarrierCount = 0;
	TArray<int32> RareExtremeCarrierPlannedEntryIndexes;
	FName AffixPolicyId = NAME_None;
	int64 AffixBudgetBefore = 0;
	int64 AffixValueSpent = 0;
	int64 AffixBudgetAfter = 0;
	int32 AffixEligibleEquipmentCount = 0;
	int32 AffixedEquipmentCount = 0;
	int32 PityStateIn = 0;
	int32 PityStateOut = 0;
	TArray<Fdemo_mapRewardAffixPityDecision> PityDecisions;
	TArray<Fdemo_mapRewardProjectionSectionTrace> Sections;
	FString Diagnostic;
};

struct Fdemo_mapRewardSourceProjectionResult
{
	Edemo_mapRewardGenerationStatus Status =
		Edemo_mapRewardGenerationStatus::InvalidRequest;
	TArray<Fdemo_mapRewardPlannedStack> PlannedStacks;
	Fdemo_mapRewardSourceProjectionTrace Trace;

	bool IsSuccess() const
	{
		return Status == Edemo_mapRewardGenerationStatus::Success
			|| Status == Edemo_mapRewardGenerationStatus::CapacityLimited;
	}
};

/**
 * The existing generated-source ledger's immutable accepted payload. It records
 * the exact manifest view and accepted plan; it is not a second item authority.
 */
struct Fdemo_mapRewardSourceAcceptanceReceipt
{
	FGuid RunId;
	FName StableSourceRoleId = NAME_None;
	FName SlotId = NAME_None;
	FName ProjectionId = NAME_None;
	FName DistributionProfileId = NAME_None;
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
	FName BudgetProfileId = NAME_None;
	FName MarkerId = NAME_None;
	FName EncounterId = NAME_None;
	FName JackpotPolicyId = NAME_None;
	FName RareExtremePolicyId = NAME_None;
	FName AffixPolicyId = NAME_None;
	uint64 EffectiveSeed = 0;
	int64 RandomizedBudget = 0;
	int64 GeneratedTotalValue = 0;
	int64 ResidualValue = 0;
	/** Durable replay input/output for the active pity read model. */
	int32 PityStateIn = 0;
	int32 PityStateOut = 0;
	bool bPityCommitRequired = false;
	bool bFallbackUsed = false;
	bool bLegacyCompatibilityView = false;
	TArray<Fdemo_mapRewardPlannedStack> PlannedStacks;

	bool IsValid() const;
	static Fdemo_mapRewardSourceAcceptanceReceipt FromProjection(
		const Fdemo_mapRewardSourceProjection& Projection,
		const Fdemo_mapRewardSourceProjectionResult& Result);
	static Fdemo_mapRewardSourceAcceptanceReceipt FromGeneratedResult(
		FName StableSourceRoleId,
		const Fdemo_mapRewardGenerationResult& Result);
};

/** Single authority for the P2 projections plus the P7 Boss role projection. */
struct Fdemo_mapRewardSourceProjectionRegistry
{
	static const TArray<Fdemo_mapRewardSourceProjection>& GetAll();
	static const Fdemo_mapRewardSourceProjection* Find(FName ProjectionId);
	static const Fdemo_mapRewardSourceProjection* FindChest(int32 ChestIndex);
	static const Fdemo_mapRewardSourceProjection* FindBossPrototype();
	static const Fdemo_mapRewardSourceProjection* FindCorpseByFallbackTable(
		FName FixedTableId);
	static TArray<Fdemo_mapRewardPoolEntry> BuildSectionPool(
		const Fdemo_mapRewardSourceProjection& Projection,
		const Fdemo_mapRewardProjectionSection& Section);
	static bool Validate(FString* OutError = nullptr);
};

/** Pure deterministic multi-section planner; materialization remains in ItemSubsystem. */
struct Fdemo_mapRewardSourceProjectionPlanner
{
	static Fdemo_mapRewardSourceProjectionResult Plan(
		const Fdemo_mapRewardSourceProjection& Projection,
		FGuid RunId,
		int32 PityStateIn = 0);
	static TArray<Fdemo_mapRuntimeContainerSeedEntry> BuildContainerSeed(
		const Fdemo_mapRewardSourceProjectionResult& Result);
};
