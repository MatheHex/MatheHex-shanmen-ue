#include "demo_mapProfileBeginRunTransaction.h"

#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardRareExtreme.h"
#include "demo_mapRewardFullMapDistribution.h"
#include "demo_mapRewardSourceProjection.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	Fdemo_mapBeginRunResult Reject(Edemo_mapBeginRunStatus Status, const FString& Diagnostic, const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapBeginRunResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.ProfileId = Profile.ProfileId;
		Result.PreviousGeneration = Profile.SaveGeneration;
		return Result;
	}

	bool IsRunInventoryDefinition(FName DefinitionId)
	{
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
		return Definition
			&& (Definition->CategoryId == Fdemo_mapItemIds::MaterialCategory
				|| Definition->CategoryId == Fdemo_mapItemIds::ConsumableCategory);
	}

	FGuid P5CandidateRunId(int32 Attempt)
	{
		return FGuid(
			0x50350000u,
			0x41464649u,
			0x58504954u,
			static_cast<uint32>(Attempt + 1));
	}

	FGuid P7BossCandidateRunId(int32 Attempt)
	{
		return FGuid(
			0x50370000u,
			0x424F5353u,
			0x52455744u,
			static_cast<uint32>(Attempt + 1));
	}

	FGuid P8FullMapCandidateRunId(int32 Attempt)
	{
		return FGuid(
			0x50380000u,
			0x46554C4Cu,
			0x4D415000u,
			static_cast<uint32>(Attempt + 1));
	}

	FGuid F0FullSystemCandidateRunId(int32 Attempt)
	{
		return FGuid(
			0x46300000u,
			0x46554C4Cu,
			0x53595354u,
			static_cast<uint32>(Attempt + 1));
	}

	bool IsGeneratedFullSystemCategory(
		const Fdemo_mapRewardPlannedStack& Stack,
		Edemo_mapRuntimeContainerSection Section,
		FName CategoryId)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
		return Stack.Section == Section
			&& Definition
			&& Definition->CategoryId == CategoryId;
	}

	bool FindF0GeneratedFullSystemRunId(
		int32 SearchLimit,
		FGuid& OutRunId,
		int32& OutAttempt)
	{
		OutRunId.Invalidate();
		OutAttempt = INDEX_NONE;
		const Fdemo_mapFullMapRewardSlot* Target =
			Fdemo_mapRewardFullMapDistribution::FindEnemyByEncounterId(
				Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced);
		if (!Target
			|| Target->RewardClass
				!= Edemo_mapFullMapRewardClass::EnemyElite)
		{
			return false;
		}
		for (int32 Attempt = 0; Attempt < SearchLimit; ++Attempt)
		{
			const FGuid Candidate =
				F0FullSystemCandidateRunId(Attempt);
			int32 PityState = 0;
			bool bValid = true;
			for (const Fdemo_mapFullMapRewardSlot& Slot :
				Fdemo_mapRewardFullMapDistribution::GetSlots())
			{
				if (!Slot.IsContainer())
				{
					continue;
				}
				const Fdemo_mapRewardSourceProjectionResult Plan =
					Fdemo_mapRewardSourceProjectionPlanner::Plan(
						Fdemo_mapRewardFullMapDistribution::
							BuildProjection(Slot),
						Candidate,
						PityState);
				if (!Plan.IsSuccess() || Plan.Trace.bFallbackUsed)
				{
					bValid = false;
					break;
				}
				PityState = Plan.Trace.PityStateOut;
			}
			if (!bValid)
			{
				continue;
			}
			const Fdemo_mapRewardSourceProjectionResult Plan =
				Fdemo_mapRewardSourceProjectionPlanner::Plan(
					Fdemo_mapRewardFullMapDistribution::
						BuildProjection(*Target),
					Candidate,
					PityState);
			const bool bBackpack =
				Plan.PlannedStacks.ContainsByPredicate(
					[](const Fdemo_mapRewardPlannedStack& Stack)
					{
						return IsGeneratedFullSystemCategory(
							Stack,
							Edemo_mapRuntimeContainerSection::Equipment,
							Fdemo_mapItemIds::BackpackCategory);
					});
			const bool bInnerCore =
				Plan.PlannedStacks.ContainsByPredicate(
					[](const Fdemo_mapRewardPlannedStack& Stack)
					{
						return IsGeneratedFullSystemCategory(
								Stack,
								Edemo_mapRuntimeContainerSection::Body,
								Fdemo_mapItemIds::CoreCategory)
							&& Stack.DefinitionId.ToString().StartsWith(
								TEXT("Prototype.Item.Core.Inner."));
					});
			if (Plan.IsSuccess()
				&& !Plan.Trace.bFallbackUsed
				&& !Plan.Trace.bJackpotHit
				&& !Plan.Trace.bRareExtremeHit
				&& bBackpack
				&& bInnerCore)
			{
				OutRunId = Candidate;
				OutAttempt = Attempt + 1;
				return true;
			}
		}
		return false;
	}

	bool IsP7AffixSmokeWeapon(FName DefinitionId)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		return Definition
			&& Definition->CategoryId
				== Fdemo_mapItemIds::WeaponCategory;
	}

	bool FindP7BossProductSmokeRunId(
		int32 SearchLimit,
		FGuid& OutRunId)
	{
		OutRunId.Invalidate();
		const auto* Boss =
			Fdemo_mapRewardSourceProjectionRegistry::
				FindBossPrototype();
		for (int32 Attempt = 0;
			Boss && Attempt < SearchLimit;
			++Attempt)
		{
			const FGuid Candidate =
				P7BossCandidateRunId(Attempt);
			const auto Plan =
				Fdemo_mapRewardSourceProjectionPlanner::Plan(
					*Boss,
					Candidate);
			const bool bNaturalAffixedEquipment =
				Plan.PlannedStacks.ContainsByPredicate(
					[](const auto& Stack)
					{
						return IsP7AffixSmokeWeapon(
								Stack.DefinitionId)
							&& !Stack.AffixSet.Affixes.IsEmpty();
					});
			if (Plan.IsSuccess()
				&& !Plan.Trace.bJackpotHit
				&& !Plan.Trace.bRareExtremeHit
				&& bNaturalAffixedEquipment)
			{
				OutRunId = Candidate;
				return true;
			}
		}
		return false;
	}

	bool FindP8FullMapProductSmokeRunId(
		int32 SearchLimit,
		FGuid& OutRunId)
	{
		OutRunId.Invalidate();
		const Fdemo_mapFullMapRewardSlot* Standard = nullptr;
		const Fdemo_mapFullMapRewardSlot* Elite = nullptr;
		const Fdemo_mapFullMapRewardSlot* Boss = nullptr;
		for (const Fdemo_mapFullMapRewardSlot& Slot :
			Fdemo_mapRewardFullMapDistribution::GetSlots())
		{
			if (Slot.RewardClass
				== Edemo_mapFullMapRewardClass::EnemyStandard
				&& !Standard)
			{
				Standard = &Slot;
			}
			else if (Slot.RewardClass
				== Edemo_mapFullMapRewardClass::EnemyElite
				&& !Elite)
			{
				Elite = &Slot;
			}
			else if (Slot.RewardClass
				== Edemo_mapFullMapRewardClass::Boss)
			{
				Boss = &Slot;
			}
		}
		for (int32 Attempt = 0;
			Standard && Elite && Boss && Attempt < SearchLimit;
			++Attempt)
		{
			const FGuid Candidate =
				P8FullMapCandidateRunId(Attempt);
			int32 PityState = 0;
			bool bValid = true;
			for (const Fdemo_mapFullMapRewardSlot& Slot :
				Fdemo_mapRewardFullMapDistribution::GetSlots())
			{
				if (!Slot.IsContainer())
				{
					continue;
				}
				const auto Plan =
					Fdemo_mapRewardSourceProjectionPlanner::Plan(
						Fdemo_mapRewardFullMapDistribution::
							BuildProjection(Slot),
						Candidate,
						PityState);
				if (!Plan.IsSuccess()
					|| Plan.Trace.bFallbackUsed)
				{
					bValid = false;
					break;
				}
				PityState = Plan.Trace.PityStateOut;
			}
			if (!bValid)
			{
				continue;
			}
			const Fdemo_mapFullMapRewardSlot* DeathOrder[] = {
				Standard,
				Elite,
				Boss
			};
			Fdemo_mapRewardSourceProjectionResult BossPlan;
			for (const Fdemo_mapFullMapRewardSlot* Slot :
				DeathOrder)
			{
				const auto Plan =
					Fdemo_mapRewardSourceProjectionPlanner::Plan(
						Fdemo_mapRewardFullMapDistribution::
							BuildProjection(*Slot),
						Candidate,
						PityState);
				if (!Plan.IsSuccess()
					|| Plan.Trace.bFallbackUsed)
				{
					bValid = false;
					break;
				}
				PityState = Plan.Trace.PityStateOut;
				if (Slot == Boss)
				{
					BossPlan = Plan;
				}
			}
			const bool bNaturalAffixedEquipment =
				BossPlan.PlannedStacks.ContainsByPredicate(
					[](const auto& Stack)
					{
						return IsP7AffixSmokeWeapon(
								Stack.DefinitionId)
							&& !Stack.AffixSet.Affixes.IsEmpty();
					});
			if (bValid
				&& BossPlan.IsSuccess()
				&& !BossPlan.Trace.bJackpotHit
				&& !BossPlan.Trace.bRareExtremeHit
				&& bNaturalAffixedEquipment)
			{
				OutRunId = Candidate;
				return true;
			}
		}
		return false;
	}

	bool AdvanceP5ProductAttempts(
		const Fdemo_mapRewardSourceProjectionResult& Plan,
		int32& InOutAttemptCount,
		bool& OutGuaranteeInSource)
	{
		OutGuaranteeInSource = false;
		const int32 InitialState =
			InOutAttemptCount < 4 ? InOutAttemptCount : 0;
		if (!Plan.IsSuccess() || Plan.Trace.bFallbackUsed
			|| Plan.Trace.PityStateIn != InitialState)
		{
			return false;
		}
		for (const Fdemo_mapRewardAffixPityDecision& Decision :
			Plan.Trace.PityDecisions)
		{
			if (!Decision.bCommitRequired)
			{
				continue;
			}
			if (InOutAttemptCount >= 4
				|| !Decision.bEligibleWeaponAttempt)
			{
				return false;
			}
			const bool bFourth = InOutAttemptCount == 3;
			const int32 ExpectedStateOut =
				bFourth ? 0 : InOutAttemptCount + 1;
			if (Decision.StateIn != InOutAttemptCount
				|| Decision.StateOut != ExpectedStateOut
				|| Decision.bGuaranteeApplied != bFourth
				|| Decision.bQualifyingTier3 != bFourth)
			{
				return false;
			}
			++InOutAttemptCount;
			OutGuaranteeInSource |= bFourth;
		}
		const int32 ExpectedFinalState =
			InOutAttemptCount == 4 ? 0 : InOutAttemptCount;
		return Plan.Trace.PityStateOut == ExpectedFinalState;
	}

	bool FindP5ProductSmokeRunId(
		int32 SearchLimit,
		FGuid& OutRunId,
		int32& OutMaxAttemptCount)
	{
		OutRunId.Invalidate();
		OutMaxAttemptCount = 0;
		const FName ChestProjectionIds[] = {
			Fdemo_mapRewardProjectionIds::ChestMainWood,
			Fdemo_mapRewardProjectionIds::ChestMainOre,
			Fdemo_mapRewardProjectionIds::ChestSideHighValue
		};
		const FName CorpseProjectionIds[] = {
			Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard,
			Fdemo_mapRewardProjectionIds::CorpseMainMeleeHeavy,
			Fdemo_mapRewardProjectionIds::CorpseMainRangedStandard,
			Fdemo_mapRewardProjectionIds::CorpseSideMeleeEnhanced,
			Fdemo_mapRewardProjectionIds::CorpseSideRangedEnhanced
		};
		for (int32 Attempt = 0; Attempt < SearchLimit; ++Attempt)
		{
			const FGuid Candidate = P5CandidateRunId(Attempt);
			bool bValid = true;
			bool bGuaranteeSourceValid = false;
			int32 AttemptCount = 0;
			for (int32 SourceIndex = 0; SourceIndex < 3; ++SourceIndex)
			{
				const Fdemo_mapRewardSourceProjection* Projection =
					Fdemo_mapRewardSourceProjectionRegistry::Find(
						ChestProjectionIds[SourceIndex]);
				const Fdemo_mapRewardSourceProjectionResult Plan =
					Projection
						? Fdemo_mapRewardSourceProjectionPlanner::Plan(
							*Projection,
							Candidate,
							AttemptCount)
						: Fdemo_mapRewardSourceProjectionResult();
				bool bGuaranteeInSource = false;
				const bool bAdvanced = AdvanceP5ProductAttempts(
					Plan,
					AttemptCount,
					bGuaranteeInSource);
				if (!bAdvanced)
				{
					bValid = false;
					break;
				}
				OutMaxAttemptCount =
					FMath::Max(OutMaxAttemptCount, AttemptCount);
				if (bGuaranteeInSource)
				{
					bGuaranteeSourceValid =
						!Plan.Trace.bJackpotHit
						&& !Plan.Trace.bRareExtremeHit;
				}
			}
			if (!bValid)
			{
				continue;
			}
			OutMaxAttemptCount =
				FMath::Max(OutMaxAttemptCount, AttemptCount);
			if (AttemptCount == 4)
			{
				if (bGuaranteeSourceValid)
				{
					OutRunId = Candidate;
					return true;
				}
				continue;
			}
			TFunction<bool(int32, int32)> SearchCorpses;
			SearchCorpses =
				[&](int32 CurrentAttemptCount, int32 UsedMask)
				{
					for (int32 Index = 0; Index < 5; ++Index)
					{
						if ((UsedMask & (1 << Index)) != 0)
						{
							continue;
						}
						const auto* Projection =
							Fdemo_mapRewardSourceProjectionRegistry::Find(
								CorpseProjectionIds[Index]);
						const auto Plan = Projection
							? Fdemo_mapRewardSourceProjectionPlanner::Plan(
								*Projection,
								Candidate,
								CurrentAttemptCount)
							: Fdemo_mapRewardSourceProjectionResult();
						int32 NextAttemptCount = CurrentAttemptCount;
						bool bGuaranteeInSource = false;
						if (!AdvanceP5ProductAttempts(
							Plan,
							NextAttemptCount,
							bGuaranteeInSource)
							|| NextAttemptCount
								== CurrentAttemptCount)
						{
							continue;
						}
						OutMaxAttemptCount = FMath::Max(
							OutMaxAttemptCount,
							NextAttemptCount);
						if (NextAttemptCount == 4)
						{
							return bGuaranteeInSource
								&& !Plan.Trace.bJackpotHit
								&& !Plan.Trace.bRareExtremeHit;
						}
						if (SearchCorpses(
							NextAttemptCount,
							UsedMask | (1 << Index)))
						{
							return true;
						}
					}
					return false;
				};
			if (SearchCorpses(AttemptCount, 0))
			{
				OutRunId = Candidate;
				return true;
			}
		}
		return false;
	}

	FGuid GenerateRunId(const Fdemo_mapPersistentProfile& Profile)
	{
		TSet<FGuid> Forbidden;
		Forbidden.Add(Profile.ProfileId);
		if (Profile.LastSettlementId.IsValid()) Forbidden.Add(Profile.LastSettlementId);
		if (Profile.ActiveRun.ActiveRunId.IsValid()) Forbidden.Add(Profile.ActiveRun.ActiveRunId);
		if (Profile.ActiveRun.CommittedSettlementId.IsValid()) Forbidden.Add(Profile.ActiveRun.CommittedSettlementId);
			for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash) Forbidden.Add(Item.ItemInstanceId);
			for (const Fdemo_mapPersistentItemRecord& Item : Profile.ActiveRun.ActiveRunItems) Forbidden.Add(Item.ItemInstanceId);
#if !UE_BUILD_SHIPPING
			if (FParse::Param(
				FCommandLine::Get(),
				TEXT("FullSystemLoopAutomation"))
				&& !Profile.LastSettlementId.IsValid())
			{
				FGuid NaturalRunId;
				int32 ActualAttempt = INDEX_NONE;
				if (FindF0GeneratedFullSystemRunId(
						500000,
						NaturalRunId,
						ActualAttempt)
					&& !Forbidden.Contains(NaturalRunId))
				{
					UE_LOG(
						LogTemp,
						Log,
						TEXT("F0_GENERATED_FULL_SYSTEM_IDENTITY_SEARCH: PASS run=%s bounded_limit=500000 actual_attempt=%d."),
						*NaturalRunId.ToString(
							EGuidFormats::DigitsWithHyphens),
						ActualAttempt);
					return NaturalRunId;
				}
				UE_LOG(
					LogTemp,
					Error,
					TEXT("F0_GENERATED_FULL_SYSTEM_IDENTITY_SEARCH: FAIL bounded_limit=500000."));
			}
			if (FParse::Param(
				FCommandLine::Get(),
				TEXT("RewardFullMapDistributionAutomation"))
				&& !Profile.LastSettlementId.IsValid())
			{
				FGuid NaturalRunId;
				if (FindP8FullMapProductSmokeRunId(
						500000,
						NaturalRunId)
					&& !Forbidden.Contains(NaturalRunId))
				{
					return NaturalRunId;
				}
				UE_LOG(
					LogTemp,
					Error,
					TEXT("P8_FULL_MAP_REWARD_PRODUCT_SEARCH: finite search failed limit=500000."));
			}
			if (FParse::Param(
				FCommandLine::Get(),
				TEXT("RewardBossSourceAutomation"))
				&& !Profile.LastSettlementId.IsValid())
			{
				FGuid NaturalRunId;
				if (FindP7BossProductSmokeRunId(
						500000,
						NaturalRunId)
					&& !Forbidden.Contains(NaturalRunId))
				{
					return NaturalRunId;
				}
				UE_LOG(
					LogTemp,
					Error,
					TEXT("P7_BOSS_REWARD_PRODUCT_SEARCH: finite search failed limit=500000."));
			}
			if (FParse::Param(
				FCommandLine::Get(),
				TEXT("RewardAffixPityAutomation"))
				&& !Profile.LastSettlementId.IsValid())
			{
				FGuid NaturalRunId;
				int32 MaxAttemptCount = 0;
				if (FindP5ProductSmokeRunId(
						250000,
						NaturalRunId,
						MaxAttemptCount)
					&& !Forbidden.Contains(NaturalRunId))
				{
					return NaturalRunId;
				}
				UE_LOG(
					LogTemp,
					Error,
					TEXT("P5_REWARD_AFFIX_PITY_PRODUCT_SEARCH: finite search failed limit=250000 max_attempt_count=%d."),
					MaxAttemptCount);
			}
			if (FParse::Param(
				FCommandLine::Get(),
				TEXT("RewardRareExtremeAutomation")))
			{
				const Fdemo_mapFullMapRewardSlot* HitSlot =
					Fdemo_mapRewardFullMapDistribution::GetSlots().
						FindByPredicate([](const auto& Slot)
						{
							return Slot.StableSourceRoleId
								== FName(
									TEXT("P8.SourceRole.Enemy.Standard.001"))
								&& Slot.ProjectionId
									== Fdemo_mapRewardProjectionIds::
										CorpseMainMeleeStandard;
						});
				const Fdemo_mapRewardSourceProjection HitProjection =
					HitSlot
						? Fdemo_mapRewardFullMapDistribution::
							BuildProjection(*HitSlot)
						: Fdemo_mapRewardSourceProjection();
				FGuid NaturalRunId;
				int32 Attempt = INDEX_NONE;
				if (HitProjection.IsValid()
					&& Fdemo_mapRewardRareExtreme::
						FindNaturalTier125JackpotMiss(
							Fdemo_mapRewardRareExtremePolicyRegistry::
								GetDefault(),
							Fdemo_mapRewardJackpotPolicyRegistry::
								GetDefault(),
							HitProjection.StableSourceRoleId,
							HitProjection.ProjectionId,
							1200,
							2000000,
							NaturalRunId,
							Attempt)
					&& !Forbidden.Contains(NaturalRunId))
				{
					return NaturalRunId;
				}
			}
			if (FParse::Param(
				FCommandLine::Get(),
				TEXT("RewardJackpotAutomation")))
			{
				const Fdemo_mapRewardSourceProjection* HitProjection =
					Fdemo_mapRewardSourceProjectionRegistry::Find(
						Fdemo_mapRewardProjectionIds::ChestMainWood);
				TArray<TPair<FName, FName>> AllSources;
				for (const Fdemo_mapRewardSourceProjection& Projection :
					Fdemo_mapRewardSourceProjectionRegistry::GetAll())
				{
					AllSources.Emplace(
						Projection.StableSourceRoleId,
						Projection.ProjectionId);
				}
				FGuid NaturalRunId;
				if (HitProjection
					&& Fdemo_mapRewardJackpot::
						FindNaturalRunForSingleHitAcrossSources(
							Fdemo_mapRewardJackpotPolicyRegistry::GetDefault(),
							HitProjection->StableSourceRoleId,
							HitProjection->ProjectionId,
							AllSources,
							4096,
							NaturalRunId)
					&& !Forbidden.Contains(NaturalRunId))
				{
					return NaturalRunId;
				}
			}
#endif
			for (int32 Attempt = 0; Attempt < 16; ++Attempt)
		{
			const FGuid Candidate = FGuid::NewGuid();
			if (Candidate.IsValid() && !Forbidden.Contains(Candidate)) return Candidate;
		}
		return FGuid();
	}
}

Fdemo_mapBeginRunResult Fdemo_mapProfileBeginRunTransaction::Execute(
	Fdemo_mapPersistentProfile& InOutProfile,
	const Fdemo_mapBeginRunRequest& Request,
	const Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	FString ValidationError;
	if (!Repository.ValidateProfile(InOutProfile, &ValidationError)) return Reject(Edemo_mapBeginRunStatus::ProfileValidationRejected, ValidationError, InOutProfile);
	if (Request.ExpectedProfileId != InOutProfile.ProfileId) return Reject(Edemo_mapBeginRunStatus::ProfileIdentityMismatch, TEXT("ExpectedProfileId does not match the caller Profile."), InOutProfile);
	if (Request.ExpectedSaveGeneration != InOutProfile.SaveGeneration) return Reject(Edemo_mapBeginRunStatus::ProfileGenerationMismatch, TEXT("ExpectedSaveGeneration is stale."), InOutProfile);
	if (InOutProfile.ActiveRun.bHasActiveRun) return Reject(Edemo_mapBeginRunStatus::ActiveRunAlreadyExists, TEXT("Profile already contains an ActiveRun."), InOutProfile);
	if (Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
			< Fdemo_mapPersistentPreparationLayout::BaseQuickItemSlotCount
		|| Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num() != 5)
		return Reject(Edemo_mapBeginRunStatus::RuntimeCapacityContractRejected, TEXT("Runtime inventory or equipment capacity is below the frozen loadout contract."), InOutProfile);
	Fdemo_mapPersistentPreparationLayout EffectiveLayout;
	if (Request.bRequireCommittedPreparationLayout)
	{
		EffectiveLayout = InOutProfile.PreparationLayout;
	}
	else
	{
		// A direct product Start Run must neither inspect nor reconstruct the
		// retired PreparationLayout.  The empty request-owned layout is used only
		// to form this run's immutable materialization plan.
		EffectiveLayout.WeaponItemInstanceId = Request.Loadout.WeaponItemInstanceId;
		EffectiveLayout.ArmorItemInstanceId = Request.Loadout.ArmorItemInstanceId;
		EffectiveLayout.AccessoryItemInstanceId = Request.Loadout.AccessoryItemInstanceId;
		EffectiveLayout.SpatialRingItemInstanceId = Request.Loadout.SpatialRingItemInstanceId;
		EffectiveLayout.BackpackItemInstanceId = Request.Loadout.BackpackItemInstanceId;
		EffectiveLayout.OrderedRunInventoryItemInstanceIds = Request.Loadout.MaterialStackItemInstanceIds;
		EffectiveLayout.HotbarItemInstanceIds =
			Request.Loadout.HotbarItemInstanceIds.Num() == Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
				? Request.Loadout.HotbarItemInstanceIds
				: Fdemo_mapPersistentPreparationLayout().HotbarItemInstanceIds;
	}
	const Fdemo_mapPersistentPreparationLayout& Layout = EffectiveLayout;
	if (Layout.OrderedRunInventoryItemInstanceIds.Num() > Fdemo_mapPersistentPreparationLayout::MaxRunInventoryItems)
		return Reject(Edemo_mapBeginRunStatus::SelectionLimitExceeded, TEXT("Committed carried inventory exceeds the nested base/ring/bag structural maximum."), InOutProfile);

	struct FSlotSelection { FGuid Id; FName Slot; };
	const TArray<FSlotSelection> Equipment = {
		{ Layout.WeaponItemInstanceId, Fdemo_mapItemIds::WeaponSlot },
		{ Layout.ArmorItemInstanceId, Fdemo_mapItemIds::ArmorSlot },
		{ Layout.AccessoryItemInstanceId, Fdemo_mapItemIds::AccessorySlot },
		{ Layout.SpatialRingItemInstanceId, Fdemo_mapItemIds::SpatialRingSlot },
		{ Layout.BackpackItemInstanceId, Fdemo_mapItemIds::BackpackSlot }
	};
	TSet<FGuid> Selected;
	int32 EquipmentCount = 0;
	for (const FSlotSelection& Entry : Equipment) if (Entry.Id.IsValid())
	{
		++EquipmentCount;
		if (Selected.Contains(Entry.Id)) return Reject(Edemo_mapBeginRunStatus::DuplicateSelection, TEXT("An ItemInstanceId is selected more than once."), InOutProfile);
		Selected.Add(Entry.Id);
	}
	for (const FGuid& Id : Layout.OrderedRunInventoryItemInstanceIds)
	{
		if (!Id.IsValid()) return Reject(Edemo_mapBeginRunStatus::SelectedItemNotFound, TEXT("A RunInventory selection has an invalid ItemInstanceId."), InOutProfile);
		if (Selected.Contains(Id)) return Reject(Edemo_mapBeginRunStatus::DuplicateSelection, TEXT("An ItemInstanceId is selected more than once."), InOutProfile);
		Selected.Add(Id);
	}
	if (EquipmentCount > 5
		|| Selected.Num()
			> 5 + Fdemo_mapPersistentPreparationLayout::MaxRunInventoryItems)
	{
		return Reject(
			Edemo_mapBeginRunStatus::SelectionLimitExceeded,
			TEXT("The five-equipment plus carried-inventory structural limit was exceeded."),
			InOutProfile);
	}

	TMap<FGuid, int32> StashIndexes;
	for (int32 Index = 0; Index < InOutProfile.PermanentStash.Num(); ++Index) StashIndexes.Add(InOutProfile.PermanentStash[Index].ItemInstanceId, Index);
	auto FindSelected = [&InOutProfile, &StashIndexes](FGuid Id) -> const Fdemo_mapPersistentItemRecord*
	{
		const int32* Index = StashIndexes.Find(Id);
		return Index ? &InOutProfile.PermanentStash[*Index] : nullptr;
	};
	for (const FGuid& Id : Selected)
	{
		if (!StashIndexes.Contains(Id))
		{
			const bool bExistsElsewhere = InOutProfile.ActiveRun.ActiveRunItems.ContainsByPredicate([Id](const Fdemo_mapPersistentItemRecord& Item){ return Item.ItemInstanceId == Id; });
			return Reject(bExistsElsewhere ? Edemo_mapBeginRunStatus::SelectedItemNotInPermanentStash : Edemo_mapBeginRunStatus::SelectedItemNotFound, bExistsElsewhere ? TEXT("Selected item is not owned by Permanent Stash.") : TEXT("Selected ItemInstanceId was not found."), InOutProfile);
		}
	}

	for (const FSlotSelection& Entry : Equipment) if (Entry.Id.IsValid())
	{
		const Fdemo_mapPersistentItemRecord* Item = FindSelected(Entry.Id);
		const Fdemo_mapItemDefinition* Definition = Item ? Fdemo_mapItemDefinitions::Find(Item->ItemDefinitionId) : nullptr;
		if (!Item || Item->PersistentDomain != Edemo_mapPersistentDomain::PermanentStash || !Definition || Item->StackCount != 1 || !Definition->CompatibleSlotIds.Contains(Entry.Slot))
			return Reject(Edemo_mapBeginRunStatus::EquipmentSlotOrCompatibilityRejected, TEXT("Equipment selection violates ownership, quantity, Definition, or slot compatibility."), InOutProfile);
	}
	for (const FGuid& Id : Layout.OrderedRunInventoryItemInstanceIds)
	{
		const Fdemo_mapPersistentItemRecord* Item = FindSelected(Id);
		const Fdemo_mapItemDefinition* Definition = Item ? Fdemo_mapItemDefinitions::Find(Item->ItemDefinitionId) : nullptr;
		if (!Item || Item->PersistentDomain != Edemo_mapPersistentDomain::PermanentStash || !Definition || !IsRunInventoryDefinition(Item->ItemDefinitionId)
			|| Item->StackCount <= 0 || Item->StackCount > Definition->MaxStackSize || !Item->EquipmentSlotId.IsNone())
			return Reject(Edemo_mapBeginRunStatus::MaterialSelectionRejected, TEXT("RunInventory selection must be one complete Material or Consumable Stash record."), InOutProfile);
	}
	FName BackpackDefinitionId = NAME_None;
	if (Layout.BackpackItemInstanceId.IsValid())
	{
		const Fdemo_mapPersistentItemRecord* Backpack = FindSelected(Layout.BackpackItemInstanceId);
		if (Backpack) BackpackDefinitionId = Backpack->ItemDefinitionId;
	}
	FName SpatialRingDefinitionId = NAME_None;
	if (Layout.SpatialRingItemInstanceId.IsValid())
	{
		const Fdemo_mapPersistentItemRecord* SpatialRing =
			FindSelected(Layout.SpatialRingItemInstanceId);
		if (SpatialRing) SpatialRingDefinitionId = SpatialRing->ItemDefinitionId;
	}
	const Fdemo_mapInventoryCapacityResult Capacity =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			BackpackDefinitionId,
			SpatialRingDefinitionId);
	if (!Capacity.bSuccess || Layout.OrderedRunInventoryItemInstanceIds.Num() > Capacity.Capacity)
		return Reject(Edemo_mapBeginRunStatus::RuntimeCapacityContractRejected, TEXT("Committed RunInventory does not fit the selected Backpack capacity."), InOutProfile);

	Fdemo_mapPersistentProfile Candidate = InOutProfile;
	if (Request.bRequireCommittedPreparationLayout)
	{
		Candidate.PreparationLayout = Layout;
	}
	Candidate.ActiveRun.bHasActiveRun = true;
	Candidate.ActiveRun.ActiveRunId = GenerateRunId(Candidate);
	if (!Candidate.ActiveRun.ActiveRunId.IsValid()) return Reject(Edemo_mapBeginRunStatus::ProfileValidationRejected, TEXT("A unique ActiveRunId could not be generated."), InOutProfile);
	Candidate.ActiveRun.ActiveRunState = Edemo_mapPersistentActiveRunState::Prepared;
	Candidate.ActiveRun.RiskSpiritStones = 0;
	Candidate.ActiveRun.ConsumedSpiritStoneSourceIds.Reset();
	Candidate.ActiveRun.CommittedSettlementId.Invalidate();
	Candidate.ActiveRun.ActiveRunItems.Reset();
	Candidate.ActiveRun.DeployedItemIds.Reset();

	auto MoveEquipment = [&Candidate, &StashIndexes](FGuid Id, FName Slot)
	{
		if (!Id.IsValid()) return;
		Fdemo_mapPersistentItemRecord Item = Candidate.PermanentStash[StashIndexes.FindChecked(Id)];
		Item.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
		Item.EquipmentSlotId = Slot;
		Candidate.ActiveRun.ActiveRunItems.Add(Item);
		Candidate.ActiveRun.DeployedItemIds.Add(Item.ItemInstanceId);
	};
	for (const FSlotSelection& Entry : Equipment) MoveEquipment(Entry.Id, Entry.Slot);
	auto MoveRunInventory = [&Candidate, &StashIndexes](FGuid Id)
	{
		Fdemo_mapPersistentItemRecord Item = Candidate.PermanentStash[StashIndexes.FindChecked(Id)];
		Item.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
		Item.EquipmentSlotId = NAME_None;
		Candidate.ActiveRun.ActiveRunItems.Add(Item);
		Candidate.ActiveRun.DeployedItemIds.Add(Item.ItemInstanceId);
	};
	if (Request.bRequireCommittedPreparationLayout)
	{
		for (const FGuid& Id : Layout.OrderedRunInventoryItemInstanceIds)
			MoveRunInventory(Id);
	}
	else
	{
		for (const Fdemo_mapPersistentItemRecord& StashItem : InOutProfile.PermanentStash)
		{
			if (Layout.OrderedRunInventoryItemInstanceIds.Contains(StashItem.ItemInstanceId))
				MoveRunInventory(StashItem.ItemInstanceId);
		}
	}
	Candidate.PermanentStash.RemoveAll([&Selected](const Fdemo_mapPersistentItemRecord& Item){ return Selected.Contains(Item.ItemInstanceId); });
	if (!Repository.ValidateProfile(Candidate, &ValidationError)) return Reject(Edemo_mapBeginRunStatus::ProfileValidationRejected, TEXT("BeginRun candidate validation failed: ") + ValidationError, InOutProfile);

	Fdemo_mapPersistentProfile Committed = Candidate;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Committed, Storage);
	Fdemo_mapBeginRunResult Result = Reject(Edemo_mapBeginRunStatus::RepositorySaveRejected, Save.Diagnostic, InOutProfile);
	Result.bDiskStateChanged = Save.bDiskStateChanged;
	Result.RepositorySaveStatus = Save.Status;
	if (Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Result.Status = Edemo_mapBeginRunStatus::CommitOutcomeRequiresReload;
		return Result;
	}
	if (!Save.IsSuccess()) return Result;
	InOutProfile = Committed;
	Result.Status = Edemo_mapBeginRunStatus::Committed;
	Result.ProfileId = Committed.ProfileId;
	Result.CommittedGeneration = Committed.SaveGeneration;
	Result.ActiveRunId = Committed.ActiveRun.ActiveRunId;
	Result.CommittedProfile = Committed;
	Fdemo_mapCommittedRunLoadoutPlan Plan;
	Plan.ProfileId = Committed.ProfileId;
	Plan.CommittedGeneration = Committed.SaveGeneration;
	Plan.ActiveRunId = Committed.ActiveRun.ActiveRunId;
	Plan.OrderedItems = Committed.ActiveRun.ActiveRunItems;
	Plan.DeployedItemIds = Committed.ActiveRun.DeployedItemIds;
	Plan.HotbarItemInstanceIds = Layout.HotbarItemInstanceIds;
	Plan.RunInventoryCapacity = Capacity.Capacity;
	Result.CommittedLoadoutPlan = MoveTemp(Plan);
	return Result;
}
