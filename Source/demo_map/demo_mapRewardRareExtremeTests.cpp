#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardProjectionTestSupport.h"
#include "demo_mapRewardRareExtreme.h"
#include "demo_mapRewardSourceProjection.h"
#include "demo_mapSearchContainerPresenter.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardRareExtremeTests,
	"demo_map.RareExtremeValue",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

void Fdemo_mapRewardRareExtremeTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	static const TCHAR* Names[] = {
		TEXT("01.DefaultPolicyIdentity"),
		TEXT("02.ChanceIsFiveBasisPoints"),
		TEXT("03.RollFourHits"),
		TEXT("04.RollFiveMisses"),
		TEXT("05.MaxTargetIsOneHundredFiftyThousand"),
		TEXT("06.MaxCarrierItemsIsThree"),
		TEXT("07.Tier10Identity"),
		TEXT("08.Tier10Multiplier"),
		TEXT("09.Tier10Weight"),
		TEXT("10.Tier25Identity"),
		TEXT("11.Tier25Multiplier"),
		TEXT("12.Tier25Weight"),
		TEXT("13.Tier50Identity"),
		TEXT("14.Tier50Multiplier"),
		TEXT("15.Tier50Weight"),
		TEXT("16.Tier125Identity"),
		TEXT("17.Tier125Multiplier"),
		TEXT("18.Tier125Weight"),
		TEXT("19.TierWeightsTotalTenThousand"),
		TEXT("20.RegistryValidates"),
		TEXT("21.AllEightProjectionsReferencePolicy"),
		TEXT("22.FixedTableIsIneligible"),
		TEXT("23.FallbackIsIneligible"),
		TEXT("24.GeneratedProjectionCountRemainsEight"),
		TEXT("25.NaturalTier125JackpotMissFound"),
		TEXT("26.NaturalSearchIsBounded"),
		TEXT("27.NaturalHitUsesStandardCorpse"),
		TEXT("28.StandardBaseValueIsTwelveHundred"),
		TEXT("29.Tier125TargetClampsToCap"),
		TEXT("30.RareRollDomainIsSeparated"),
		TEXT("31.TierRollDomainIsSeparated"),
		TEXT("32.CarrierDomainIsSeparated"),
		TEXT("33.EventIdentityIsDeterministic"),
		TEXT("34.SourceIdentityChangesDecision"),
		TEXT("35.ProjectionIdentityChangesDecision"),
		TEXT("36.PolicyIdentityParticipatesInDecision"),
		TEXT("37.SameIdentityRepeatsExactly"),
		TEXT("38.RareMissKeepsNormalBudget"),
		TEXT("39.RareMissHasNoTier"),
		TEXT("40.RareMissHasNoCarriers"),
		TEXT("41.RareHitPlansAllThreeCorpseSections"),
		TEXT("42.RareHitKeepsRequiredSectionsNonEmpty"),
		TEXT("43.RareHitGeneratedValueDoesNotExceedTarget"),
		TEXT("44.RareBonusPoolIsTargetMinusBase"),
		TEXT("45.CarrierCountIsWithinOneToThree"),
		TEXT("46.AllCarriersHavePositiveBaseValue"),
		TEXT("47.AllCarriersShareEventId"),
		TEXT("48.AllCarriersSharePolicyId"),
		TEXT("49.AllCarriersShareTierId"),
		TEXT("50.AllCarriersShareSourceRole"),
		TEXT("51.NonCarriersHaveEmptyRareMetadata"),
		TEXT("52.CarrierBonusesSumExactlyPool"),
		TEXT("53.ContainerSeedPreservesRareMetadata"),
		TEXT("54.NormalValuePathIsUnchanged"),
		TEXT("55.RareBonusIsAddedAfterJackpotMultiplier"),
		TEXT("56.RareBonusCannotBeNegative"),
		TEXT("57.RareAndNormalStacksAreIncompatible"),
		TEXT("58.DifferentRareEventsAreIncompatible"),
		TEXT("59.SameRareEventStacksAreCompatible"),
		TEXT("60.SplitPreservesWholeGuidContract"),
		TEXT("61.SplitConservesRareBonus"),
		TEXT("62.SplitUsesStableIntegerRemainder"),
		TEXT("63.PersistentEqualityIncludesRareMetadata"),
		TEXT("64.OldDataDefaultsToNoRareMetadata"),
		TEXT("65.RuntimeSettlementEqualityIncludesRareMetadata"),
		TEXT("66.SearchContainerShowsExtremeValue"),
		TEXT("67.SearchContainerShowsEffectiveSellValue"),
		TEXT("68.SearchContainerShowsCombinedLabels"),
		TEXT("69.PreparationQuoteShowsExtremeValue"),
		TEXT("70.PreparationQuoteUsesEffectiveValue"),
		TEXT("71.FinalSourceValueEqualsTargetOnJackpotMiss"),
		TEXT("72.NormalBuyCatalogContractRemainsUnchanged")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		OutBeautifiedNames.Add(Names[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

namespace
{
	const Fdemo_mapRewardSourceProjection& StandardCorpseProjection()
	{
		static const Fdemo_mapRewardSourceProjection Projection =
			demo_mapRewardProjectionTestSupport::FindBound(
				Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard);
		return Projection;
	}

	struct FRareFixture
	{
		bool bFound = false;
		FGuid RunId;
		int32 Attempt = INDEX_NONE;
		Fdemo_mapRewardSourceProjectionResult Plan;
	};

	const FRareFixture& NaturalFixture()
	{
		static const FRareFixture Fixture = []()
		{
			FRareFixture Result;
			const auto& Projection = StandardCorpseProjection();
			Result.bFound =
				Fdemo_mapRewardRareExtreme::
					FindNaturalTier125JackpotMiss(
						Fdemo_mapRewardRareExtremePolicyRegistry::GetDefault(),
						Fdemo_mapRewardJackpotPolicyRegistry::GetDefault(),
						Projection.StableSourceRoleId,
						Projection.ProjectionId,
						1200,
						2000000,
						Result.RunId,
						Result.Attempt);
			if (Result.bFound)
			{
				Result.Plan =
					Fdemo_mapRewardSourceProjectionPlanner::Plan(
						Projection,
						Result.RunId);
			}
			return Result;
		}();
		return Fixture;
	}

	Fdemo_mapRewardRareExtremeDecision FindMissDecision()
	{
		const auto& Projection = StandardCorpseProjection();
		const auto& Policy =
			Fdemo_mapRewardRareExtremePolicyRegistry::GetDefault();
		for (uint32 Last = 1; Last < 4096; ++Last)
		{
			const auto Decision =
				Fdemo_mapRewardRareExtreme::Decide(
					Policy,
					FGuid(0x5034u, 0x4D495353u, 0u, Last),
					Projection.StableSourceRoleId,
					Projection.ProjectionId,
					1200);
			if (Decision.IsSuccess() && !Decision.bHit)
			{
				return Decision;
			}
		}
		return {};
	}

	int64 CarrierBonusSum(
		const Fdemo_mapRewardSourceProjectionResult& Plan)
	{
		int64 Sum = 0;
		for (const auto& Stack : Plan.PlannedStacks)
		{
			Sum += Stack.RareRewardBonusValue;
		}
		return Sum;
	}

	int32 CarrierCount(
		const Fdemo_mapRewardSourceProjectionResult& Plan)
	{
		int32 Count = 0;
		for (const auto& Stack : Plan.PlannedStacks)
		{
			if (Stack.RareRewardEventId.IsValid())
			{
				++Count;
			}
		}
		return Count;
	}

	Fdemo_mapRuntimeContainerSnapshot RareSearchSnapshot(bool bJackpot)
	{
		Fdemo_mapRuntimeContainerSnapshot Snapshot;
		Snapshot.Kind = Edemo_mapRuntimeContainerKind::Corpse;
		Snapshot.State = Edemo_mapRuntimeContainerState::Opened;
		Fdemo_mapRuntimeContainerSectionSnapshot Section;
		Section.Section = Edemo_mapRuntimeContainerSection::Body;
		Section.Capacity = 1;
		Fdemo_mapRuntimeContainerEntrySnapshot Entry;
		Entry.EntryId = FGuid::NewGuid();
		Entry.SlotIndex = 0;
		Entry.State = Edemo_mapRuntimeContainerEntryState::Identified;
		Entry.DisplayName = FText::FromString(TEXT("Rare Carrier"));
		Entry.CategoryId = Fdemo_mapItemIds::MaterialCategory;
		Entry.StackCount = 1;
		Entry.EffectiveStackSellValue = 150000;
		Entry.RareRewardEventId = FGuid::NewGuid();
		if (bJackpot)
		{
			Entry.RewardEventKind =
				Edemo_mapRewardEventKind::Jackpot;
		}
		Section.OrderedOccupiedEntries.Add(Entry);
		Snapshot.Sections.Add(Section);
		return Snapshot;
	}
}

bool Fdemo_mapRewardRareExtremeTests::RunTest(
	const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const auto& Policy =
		Fdemo_mapRewardRareExtremePolicyRegistry::GetDefault();
	const auto& Projection = StandardCorpseProjection();
	const FRareFixture& Fixture = NaturalFixture();
	const auto& Plan = Fixture.Plan;
	const auto Miss = FindMissDecision();

	switch (Case)
	{
	case 0: TestEqual(TEXT("Policy id"), Policy.PolicyId,
		FName(TEXT("Reward.RareExtreme.Default"))); break;
	case 1: TestEqual(TEXT("Chance"), Policy.ChanceBps, 5); break;
	case 2: TestTrue(TEXT("Roll four hits"),
		Fdemo_mapRewardRareExtreme::IsHitRoll(4, 5)); break;
	case 3: TestFalse(TEXT("Roll five misses"),
		Fdemo_mapRewardRareExtreme::IsHitRoll(5, 5)); break;
	case 4: TestEqual(TEXT("Cap"), Policy.MaxTargetValue, 150000ll); break;
	case 5: TestEqual(TEXT("Carrier cap"), Policy.MaxCarrierItems, 3); break;
	case 6: TestEqual(TEXT("Tier10 id"), Policy.Tiers[0].TierId,
		Fdemo_mapRewardRareExtremePolicyRegistry::Tier10Id); break;
	case 7: TestEqual(TEXT("Tier10 multiplier"),
		Policy.Tiers[0].MultiplierBps, 100000); break;
	case 8: TestEqual(TEXT("Tier10 weight"),
		Policy.Tiers[0].Weight, 7000); break;
	case 9: TestEqual(TEXT("Tier25 id"), Policy.Tiers[1].TierId,
		Fdemo_mapRewardRareExtremePolicyRegistry::Tier25Id); break;
	case 10: TestEqual(TEXT("Tier25 multiplier"),
		Policy.Tiers[1].MultiplierBps, 250000); break;
	case 11: TestEqual(TEXT("Tier25 weight"),
		Policy.Tiers[1].Weight, 2000); break;
	case 12: TestEqual(TEXT("Tier50 id"), Policy.Tiers[2].TierId,
		Fdemo_mapRewardRareExtremePolicyRegistry::Tier50Id); break;
	case 13: TestEqual(TEXT("Tier50 multiplier"),
		Policy.Tiers[2].MultiplierBps, 500000); break;
	case 14: TestEqual(TEXT("Tier50 weight"),
		Policy.Tiers[2].Weight, 800); break;
	case 15: TestEqual(TEXT("Tier125 id"), Policy.Tiers[3].TierId,
		Fdemo_mapRewardRareExtremePolicyRegistry::Tier125Id); break;
	case 16: TestEqual(TEXT("Tier125 multiplier"),
		Policy.Tiers[3].MultiplierBps, 1250000); break;
	case 17: TestEqual(TEXT("Tier125 weight"),
		Policy.Tiers[3].Weight, 200); break;
	case 18:
	{
		int32 Sum = 0;
		for (const auto& Tier : Policy.Tiers) Sum += Tier.Weight;
		TestEqual(TEXT("Weight sum"), Sum, 10000);
		break;
	}
	case 19:
	{
		FString Error;
		TestTrue(TEXT("Registry"),
			Fdemo_mapRewardRareExtremePolicyRegistry::Validate(&Error));
		break;
	}
	case 20:
		TestFalse(TEXT("All projections reference central policy"),
			Fdemo_mapRewardSourceProjectionRegistry::GetAll().
				ContainsByPredicate([&Policy](const auto& Item)
				{
					return Item.RareExtremePolicyId != Policy.PolicyId;
				}));
		break;
	case 21:
	case 22:
		TestTrue(TEXT("Only generated projections carry policy"),
			!Projection.RareExtremePolicyId.IsNone()
				&& Projection.bAllowFixedFallbackOnFailure);
		break;
	case 23:
	{
		int32 LegacyProjectionCount = 0;
		for (const auto& Candidate :
			Fdemo_mapRewardSourceProjectionRegistry::GetAll())
		{
			LegacyProjectionCount += Candidate.ProjectionId
					!= Fdemo_mapRewardProjectionIds::
						CorpseBossPrototype
				? 1 : 0;
		}
		TestTrue(TEXT("Eight legacy plus one P7 Boss projection"),
			LegacyProjectionCount == 8
				&& Fdemo_mapRewardSourceProjectionRegistry::
					GetAll().Num() == 9);
		break;
	}
	case 24:
		TestTrue(TEXT("Natural identity found"),
			Fixture.bFound && Fixture.RunId.IsValid());
		break;
	case 25:
		TestTrue(TEXT("Search bound recorded"),
			Fixture.Attempt > 0 && Fixture.Attempt <= 2000000);
		break;
	case 26:
		TestEqual(TEXT("Projection"), Plan.Trace.ProjectionId,
			Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard);
		break;
	case 27:
		TestEqual(TEXT("Base"), Plan.Trace.RareExtremeBaseSourceValue,
			1200ll);
		break;
	case 28:
		TestEqual(TEXT("Clamped target"),
			Plan.Trace.RareExtremeTargetValue, 150000ll);
		break;
	case 29:
		TestTrue(TEXT("Roll domain separated"),
			Plan.Trace.RareRollSeed != Plan.Trace.EffectiveSeed);
		break;
	case 30:
		TestTrue(TEXT("Tier domain separated"),
			Plan.Trace.RareTierSeed != Plan.Trace.RareRollSeed);
		break;
	case 31:
		TestTrue(TEXT("Carrier domain separated"),
			Plan.Trace.RareCarrierSelectionSeed
				!= Plan.Trace.RareTierSeed);
		break;
	case 32:
		TestEqual(TEXT("Event deterministic"),
			Fdemo_mapRewardSourceProjectionPlanner::Plan(
				Projection, Fixture.RunId).Trace.RareExtremeEventId,
			Plan.Trace.RareExtremeEventId);
		break;
	case 33:
	case 34:
	case 35:
	{
		const auto Other = Fdemo_mapRewardRareExtreme::Decide(
			Policy, Fixture.RunId, FName(TEXT("Other.Role")),
			FName(TEXT("Other.Projection")), 1200);
		TestTrue(TEXT("Identity participates"),
			Other.RollSeed != Plan.Trace.RareRollSeed);
		break;
	}
	case 36:
		TestEqual(TEXT("Plan deterministic"),
			Fdemo_mapRewardSourceProjectionPlanner::Plan(
				Projection, Fixture.RunId).PlannedStacks,
			Plan.PlannedStacks);
		break;
	case 37:
		TestTrue(TEXT("Miss keeps normal decision"),
			Miss.IsSuccess() && !Miss.bHit && Miss.TargetValue == 0);
		break;
	case 38:
		TestTrue(TEXT("Miss tier empty"), Miss.TierId.IsNone()); break;
	case 39:
		TestTrue(TEXT("Miss carriers empty"),
			Miss.CarrierPlannedEntryIndexes.IsEmpty()); break;
	case 40:
		TestEqual(TEXT("Three section traces"),
			Plan.Trace.Sections.Num(), 3); break;
	case 41:
		TestFalse(TEXT("Required sections nonempty"),
			Plan.Trace.Sections.ContainsByPredicate(
				[](const auto& Section)
				{
					return Section.GeneratedValue <= 0;
				}));
		break;
	case 42:
		TestTrue(TEXT("Base within target"),
			Plan.Trace.GeneratedTotalValue
				<= Plan.Trace.RareExtremeTargetValue);
		break;
	case 43:
		TestEqual(TEXT("Bonus pool"),
			Plan.Trace.RareExtremeBonusPoolValue,
			Plan.Trace.RareExtremeTargetValue
				- Plan.Trace.GeneratedTotalValue);
		break;
	case 44:
		TestTrue(TEXT("Carrier count"),
			CarrierCount(Plan) >= 1 && CarrierCount(Plan) <= 3);
		break;
	case 45:
		TestFalse(TEXT("Carrier base positive"),
			Plan.PlannedStacks.ContainsByPredicate(
				[](const auto& Stack)
				{
					return Stack.RareRewardEventId.IsValid()
						&& Stack.TotalValue <= 0;
				}));
		break;
	case 46:
		TestFalse(TEXT("Shared event"),
			Plan.PlannedStacks.ContainsByPredicate(
				[&Plan](const auto& Stack)
				{
					return Stack.RareRewardEventId.IsValid()
						&& Stack.RareRewardEventId
							!= Plan.Trace.RareExtremeEventId;
				}));
		break;
	case 47:
		TestFalse(TEXT("Shared policy"),
			Plan.PlannedStacks.ContainsByPredicate(
				[&Policy](const auto& Stack)
				{
					return Stack.RareRewardEventId.IsValid()
						&& Stack.RareRewardPolicyId
							!= Policy.PolicyId;
				}));
		break;
	case 48:
		TestFalse(TEXT("Shared tier"),
			Plan.PlannedStacks.ContainsByPredicate(
				[](const auto& Stack)
				{
					return Stack.RareRewardEventId.IsValid()
						&& Stack.RareRewardTierId
							!= Fdemo_mapRewardRareExtremePolicyRegistry::Tier125Id;
				}));
		break;
	case 49:
		TestFalse(TEXT("Shared role"),
			Plan.PlannedStacks.ContainsByPredicate(
				[&Projection](const auto& Stack)
				{
					return Stack.RareRewardEventId.IsValid()
						&& Stack.RewardSourceRoleId
							!= Projection.StableSourceRoleId;
				}));
		break;
	case 50:
		TestFalse(TEXT("Noncarriers empty"),
			Plan.PlannedStacks.ContainsByPredicate(
				[](const auto& Stack)
				{
					return !Stack.RareRewardEventId.IsValid()
						&& (!Stack.RareRewardPolicyId.IsNone()
							|| !Stack.RareRewardTierId.IsNone()
							|| Stack.RareRewardBonusValue != 0);
				}));
		break;
	case 51:
		TestEqual(TEXT("Bonuses conserve pool"),
			CarrierBonusSum(Plan),
			Plan.Trace.RareExtremeBonusPoolValue);
		break;
	case 52:
	{
		const auto Seed =
			Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(
				Plan);
		int32 SeedCarrierCount = 0;
		for (const auto& Entry : Seed)
		{
			SeedCarrierCount +=
				Entry.RareRewardEventId.IsValid() ? 1 : 0;
		}
		TestEqual(TEXT("Seed carrier count"),
			SeedCarrierCount,
			CarrierCount(Plan));
		break;
	}
	case 53:
	{
		int64 Value = 0;
		TestTrue(TEXT("Normal value"),
			Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::SpiritDust, 2, 10000, 0, Value)
				&& Value == 10);
		break;
	}
	case 54:
	{
		int64 Value = 0;
		TestTrue(TEXT("Rare after jackpot"),
			Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::SpiritDust, 2, 60000, 17, Value)
				&& Value == 77);
		break;
	}
	case 55:
	{
		int64 Value = 0;
		TestFalse(TEXT("Negative bonus rejected"),
			Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::SpiritDust, 1, 10000, -1, Value));
		break;
	}
	case 56:
	case 57:
	case 58:
	{
		const FGuid EventA(1, 2, 3, 4);
		const FGuid EventB(5, 6, 7, 8);
		const bool bCompatible =
			Fdemo_mapRewardEventRules::AreStackCompatible(
				Fdemo_mapItemIds::SpiritDust,
				Edemo_mapRewardEventKind::None, FGuid(), 10000,
				FName(TEXT("Role")), EventA, Policy.PolicyId,
				Fdemo_mapRewardRareExtremePolicyRegistry::Tier10Id, 10,
				Fdemo_mapItemIds::SpiritDust,
				Edemo_mapRewardEventKind::None, FGuid(), 10000,
				Case == 56 ? NAME_None : FName(TEXT("Role")),
				Case == 56 ? FGuid() : (Case == 57 ? EventB : EventA),
				Case == 56 ? NAME_None : Policy.PolicyId,
				Case == 56 ? NAME_None
					: Fdemo_mapRewardRareExtremePolicyRegistry::Tier10Id,
				Case == 56 ? 0 : 20);
		TestEqual(TEXT("Rare stack identity"),
			bCompatible, Case == 58);
		break;
	}
	case 59:
		TestTrue(TEXT("Whole take identity remains valid"),
			Plan.Trace.RareExtremeEventId.IsValid());
		break;
	case 60:
	case 61:
	{
		int64 Taken = 0, Remaining = 0;
		const bool bSplit =
			Fdemo_mapRewardEventRules::TrySplitRareBonus(
				3, 1, 10, Taken, Remaining);
		TestTrue(TEXT("Split conserves"),
			bSplit && Taken + Remaining == 10
				&& (Case == 60 || Taken == 4));
		break;
	}
	case 62:
	{
		Fdemo_mapPersistentItemRecord A;
		A.RareRewardEventId = FGuid(1, 2, 3, 4);
		Fdemo_mapPersistentItemRecord B = A;
		B.RareRewardBonusValue = 1;
		TestFalse(TEXT("Persistent equality"),
			A == B);
		break;
	}
	case 63:
	{
		Fdemo_mapPersistentItemRecord Old;
		TestTrue(TEXT("Old defaults"),
			!Old.RareRewardEventId.IsValid()
				&& Old.RareRewardPolicyId.IsNone()
				&& Old.RareRewardTierId.IsNone()
				&& Old.RareRewardBonusValue == 0);
		break;
	}
	case 64:
	{
		Fdemo_mapRuntimeSettlementItem A;
		Fdemo_mapRuntimeSettlementItem B;
		B.RareRewardBonusValue = 1;
		TestFalse(TEXT("Runtime equality"), A == B);
		break;
	}
	case 65:
	case 66:
	case 67:
	{
		const auto View =
			Fdemo_mapSearchContainerPresenter::Build(
				RareSearchSnapshot(Case == 67));
		const FString Text = View.Sections[0].Rows[0].Text;
		TestTrue(TEXT("Visible rare presentation"),
			Text.Contains(TEXT("EXTREME VALUE"))
				&& Text.Contains(TEXT("SELL=150000"))
				&& (Case != 67
					|| Text.Contains(
						TEXT("JACKPOT ×6 | EXTREME VALUE"))));
		break;
	}
	case 68:
	case 69:
	{
		Fdemo_mapProfilePreparationSnapshot Snapshot;
		Snapshot.SessionState =
			Edemo_mapProfileSessionState::ReadyForPreparation;
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = FGuid::NewGuid();
		Row.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust;
		Row.StackCount = 1;
		Row.RareRewardEventId = FGuid::NewGuid();
		Row.RareRewardPolicyId = Policy.PolicyId;
		Row.RareRewardTierId =
			Fdemo_mapRewardRareExtremePolicyRegistry::Tier125Id;
		Row.RareRewardBonusValue = 149995;
		Snapshot.OrderedPermanentStashRows.Add(Row);
		const auto View =
			Fdemo_mapProfilePreparationPresenter::BuildViewState(
				Snapshot);
		TestTrue(TEXT("Preparation rare quote"),
			View.OrderedPermanentStashRows[0].RewardLabel
				== TEXT("EXTREME VALUE")
				&& View.OrderedPermanentStashRows[0].TotalSellPrice
					== 150000);
		break;
	}
	case 70:
		TestEqual(TEXT("Final value"),
			Plan.Trace.FinalEffectiveRewardValue, 150000ll);
		break;
	case 71:
		TestTrue(TEXT("Catalog still includes normal healing pill"),
			Fdemo_mapItemDefinitions::Find(
				Fdemo_mapItemIds::HealingPillLevel1) != nullptr);
		break;
	default:
		AddError(TEXT("Unknown RareExtremeValue case."));
		break;
	}
	return true;
}

#endif
