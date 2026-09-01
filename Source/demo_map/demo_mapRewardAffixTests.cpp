#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapEquipmentEffectResolver.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardEventTypes.h"
#include "demo_mapRewardProjectionTestSupport.h"
#include "demo_mapRewardSourceProjection.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardAffixTests,
	"demo_map.RewardAffix",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardAffixPityTests,
	"demo_map.RewardAffixPity",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

namespace
{
	const TCHAR* AffixNames[] = {
		TEXT("01.RegistrySingleAuthority"),
		TEXT("02.DefaultPolicyExact"),
		TEXT("03.TwelveUniqueResolvableIds"),
		TEXT("04.CountWeightsValid"),
		TEXT("05.TierWeightsSum10000"),
		TEXT("06.EquipmentEligibility"),
		TEXT("07.NonEquipmentIneligible"),
		TEXT("08.FixedTableIneligible"),
		TEXT("09.FallbackIneligible"),
		TEXT("10.SameSeedSameStateDeterministic"),
		TEXT("11.RngDomainsSeparated"),
		TEXT("12.InputOrderIndependent"),
		TEXT("13.RequiredSectionsBeforeAffix"),
		TEXT("14.RemainingBudgetOnly"),
		TEXT("15.ZeroBudgetZeroAffix"),
		TEXT("16.BudgetNeverExceedsTarget"),
		TEXT("17.WeaponMaxOne"),
		TEXT("18.RobeMaxTwo"),
		TEXT("19.AccessoryMaxOne"),
		TEXT("20.DuplicateAffixIdRejected"),
		TEXT("21.DuplicateCompatibilityGroupRejected"),
		TEXT("22.WeaponPowerTiers"),
		TEXT("23.RobeVitalityTiers"),
		TEXT("24.RobeGuardTiers"),
		TEXT("25.AccessoryHasteTiers"),
		TEXT("26.UnknownAffixRejected"),
		TEXT("27.InvalidResolvedFieldsRejected"),
		TEXT("28.Int64OverflowRejected"),
		TEXT("29.StableNonEmptyEventId"),
		TEXT("30.PlainDefaultsEmpty"),
		TEXT("31.MaterializePreserves"),
		TEXT("32.MaterializeRollback"),
		TEXT("33.SameGuidContainerInventory"),
		TEXT("34.SettlementWarehouse"),
		TEXT("35.ProfileSaveReload"),
		TEXT("36.LegacyProfileDefaultsEmpty"),
		TEXT("37.CanonicalJsonOrder"),
		TEXT("38.AffixedPlainDoNotMerge"),
		TEXT("39.DifferentAffixSetsDoNotMerge"),
		TEXT("40.AttackModifierExistingAuthority"),
		TEXT("41.VitalityModifierExistingAuthority"),
		TEXT("42.GuardModifierExistingAuthority"),
		TEXT("43.HasteModifierExistingAuthority"),
		TEXT("44.EquipLifecycleNoDuplicate"),
		TEXT("45.NormalPlusAffixValue"),
		TEXT("46.JackpotPlusAffixValue"),
		TEXT("47.RarePlusAffixValue"),
		TEXT("48.JackpotRareAffixNoDoubleMultiply"),
		TEXT("49.SearchDisplay"),
		TEXT("50.InventoryEquipmentDisplay"),
		TEXT("51.PreparationShopDisplay"),
		TEXT("52.QuoteEqualsSell"),
		TEXT("53.SellRollback"),
		TEXT("54.ShopBaseCatalogUnchanged"),
		TEXT("55.MainWoodOreZeroAffix"),
		TEXT("56.SideAndCorpseEquipmentEligible"),
		TEXT("57.BackpackBodyNoLeak"),
		TEXT("58.DefinitionPlanMissUnchanged"),
		TEXT("59.JackpotRngUnchanged"),
		TEXT("60.RareRngUnchanged"),
		TEXT("61.RareCarrierUsesPostAffixResidual"),
		TEXT("62.CleanupNoOrphanAffix"),
		TEXT("63.TraceCoversAllLayers"),
		TEXT("64.P3HistoricalRootZeroWrite")
	};

	const TCHAR* PityNames[] = {
		TEXT("01.PolicySingleAuthority"),
		TEXT("02.PolicyFieldsExact"),
		TEXT("03.State0FailureTo1"),
		TEXT("04.State1FailureTo2"),
		TEXT("05.State2FailureTo3"),
		TEXT("06.State3GuaranteeTo0"),
		TEXT("07.NaturalTier3Resets"),
		TEXT("08.BoostWeightsExact"),
		TEXT("09.NoEarlyGuarantee"),
		TEXT("10.NonWeaponNoChange"),
		TEXT("11.FixedFallbackNoChange"),
		TEXT("12.EmptyFailureNoChange"),
		TEXT("13.NoCandidateHoldsThreshold"),
		TEXT("14.InsufficientBudgetHoldsThreshold"),
		TEXT("15.MultipleWeaponsStableOrdinal"),
		TEXT("16.CommitOrderDeterministic"),
		TEXT("17.NewRunStartsZero"),
		TEXT("18.MaterializeRollbackNoAdvance"),
		TEXT("19.ReopenNoAdvance"),
		TEXT("20.CommittedSourceIdempotent"),
		TEXT("21.TerminalClears"),
		TEXT("22.ProfileHasNoPity"),
		TEXT("23.GuaranteeAcquisitionMarker"),
		TEXT("24.CombinedEventsDoNotChangeQualifying"),
		TEXT("25.StateCapped"),
		TEXT("26.TraceComplete")
	};

	Fdemo_mapRewardPlannedStack EquipmentStack(FName DefinitionId)
	{
		Fdemo_mapRewardPlannedStack Stack;
		Stack.DefinitionId = DefinitionId;
		Stack.StackCount = 1;
		Stack.UnitValue = 50;
		Stack.TotalValue = 50;
		Stack.Section = Edemo_mapRuntimeContainerSection::Equipment;
		Stack.SlotIndex = 0;
		return Stack;
	}

	Fdemo_mapRewardAffixSet MakeSet(
		FName AffixId,
		Edemo_mapRewardAffixAcquisition Acquisition =
			Edemo_mapRewardAffixAcquisition::Natural)
	{
		Fdemo_mapRewardAffixSet Set;
		const auto* Descriptor =
			Fdemo_mapRewardAffixPolicyRegistry::Find(AffixId);
		Set.AffixSetEventId = FGuid(
			0x11111111, 0x22222222, 0x33333333, 0x44444444);
		Set.AffixPolicyId =
			Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		Set.Acquisition = Acquisition;
		if (Descriptor)
		{
			Set.Affixes.Add({
				Descriptor->AffixId,
				Descriptor->Tier,
				Descriptor->MagnitudeScaled,
				Descriptor->ResolvedValue });
		}
		return Set;
	}

	struct FPlanFixture
	{
		bool bFound = false;
		TArray<Fdemo_mapRewardPlannedStack> Stacks;
		Fdemo_mapRewardAffixPlanTrace Trace;
		FGuid RunId;
	};

	FPlanFixture FindWeaponPlan(
		int32 PityState,
		bool bRequireTier3,
		int64 Budget = 1000)
	{
		for (uint32 Attempt = 1; Attempt <= 20000; ++Attempt)
		{
			FPlanFixture Fixture;
			Fixture.RunId = FGuid(
				0xA1000000u + Attempt,
				0xB2000000u,
				0xC3000000u,
				0xD4000000u);
			Fixture.Stacks = {
				EquipmentStack(Fdemo_mapItemIds::WeaponLevel1)
			};
			if (!Fdemo_mapRewardAffixPlanner::Apply(
				Fdemo_mapRewardAffixPolicyRegistry::GetDefault(),
				Fixture.RunId,
				TEXT("P5.Test.Source"),
				TEXT("P5.Test.Projection"),
				Budget,
				PityState,
				Fixture.Stacks,
				Fixture.Trace))
			{
				continue;
			}
			const bool bTier3 = !Fixture.Stacks[0].AffixSet.Affixes.IsEmpty()
				&& Fixture.Stacks[0].AffixSet.Affixes[0].Tier
					== Edemo_mapRewardAffixTier::Tier3;
			const bool bCommitted = !Fixture.Trace.PityDecisions.IsEmpty()
				&& Fixture.Trace.PityDecisions[0].bCommitRequired;
			if (bCommitted && bTier3 == bRequireTier3)
			{
				Fixture.bFound = true;
				return Fixture;
			}
		}
		return {};
	}

	bool DescriptorTriplet(
		const TCHAR* Prefix,
		const TArray<int32>& Magnitudes,
		const TArray<int64>& Values)
	{
		FString ExpectedGroup(Prefix);
		ExpectedGroup.ReplaceInline(
			TEXT("Reward.Affix."),
			TEXT("Reward.AffixGroup."));
		for (int32 Tier = 1; Tier <= 3; ++Tier)
		{
			const auto* Descriptor =
				Fdemo_mapRewardAffixPolicyRegistry::Find(
					FName(*FString::Printf(
						TEXT("%s.T%d"), Prefix, Tier)));
			if (!Descriptor
				|| Descriptor->CompatibilityGroup
					!= FName(*ExpectedGroup)
				|| static_cast<int32>(Descriptor->Tier) != Tier
				|| Descriptor->MagnitudeScaled != Magnitudes[Tier - 1]
				|| Descriptor->ResolvedValue != Values[Tier - 1])
			{
				return false;
			}
		}
		return true;
	}
}

void Fdemo_mapRewardAffixTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AffixNames); ++Index)
	{
		OutBeautifiedNames.Add(AffixNames[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

bool Fdemo_mapRewardAffixTests::RunTest(const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	FString Error;
	const FPlanFixture Natural = FindWeaponPlan(0, false);
	const FPlanFixture Guaranteed = FindWeaponPlan(3, true);
	const bool bFixtures = Natural.bFound && Guaranteed.bFound;
	bool bPass = bFixtures;

	if (Case <= 18)
	{
		bPass &= Fdemo_mapRewardAffixPolicyRegistry::Validate(&Error)
			&& Fdemo_mapRewardAffixPolicyRegistry::GetAll().Num() == 12
			&& Fdemo_mapRewardAffixPolicyRegistry::GetDefault().PolicyId
				== FName(TEXT("Reward.Affix.Equipment.Default"))
			&& Guaranteed.Trace.AffixValueSpent == 360
			&& Guaranteed.Trace.BudgetAfter == 640
			&& Guaranteed.Stacks[0].AffixSet.Affixes.Num() == 1;
	}
	else if (Case == 19 || Case == 20)
	{
		auto Set = MakeSet(TEXT("Reward.Affix.Weapon.Power.T1"));
		const Fdemo_mapResolvedRewardAffix Duplicate = Set.Affixes[0];
		Set.Affixes.Add(Duplicate);
		bPass = !Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
			Fdemo_mapItemIds::WeaponLevel1, 1, Set, &Error);
	}
	else if (Case >= 21 && Case <= 24)
	{
		bPass = Case == 21
			? DescriptorTriplet(
				TEXT("Reward.Affix.Weapon.Power"),
				{ 2, 5, 12 }, { 40, 120, 360 })
			: Case == 22
				? DescriptorTriplet(
					TEXT("Reward.Affix.Robe.Vitality"),
					{ 4, 10, 25 }, { 40, 120, 360 })
				: Case == 23
					? DescriptorTriplet(
						TEXT("Reward.Affix.Robe.Guard"),
						{ 1, 2, 4 }, { 50, 150, 450 })
					: DescriptorTriplet(
						TEXT("Reward.Affix.Accessory.Haste"),
						{ -250, -500, -1000 },
						{ 50, 150, 450 });
	}
	else if (Case == 25 || Case == 26)
	{
		auto Set = MakeSet(TEXT("Reward.Affix.Weapon.Power.T1"));
		if (Case == 25) Set.Affixes[0].AffixId = TEXT("Unknown");
		else Set.Affixes[0].ResolvedMagnitudeScaled++;
		bPass = !Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
			Fdemo_mapItemIds::WeaponLevel1, 1, Set, &Error);
	}
	else if (Case == 27)
	{
		Fdemo_mapRewardAffixSet Set;
		Set.AffixSetEventId = FGuid::NewGuid();
		Set.AffixPolicyId =
			Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		Set.Acquisition = Edemo_mapRewardAffixAcquisition::Natural;
		Set.Affixes = {
			{ TEXT("Reward.Affix.Weapon.Power.T3"),
				Edemo_mapRewardAffixTier::Tier3, 12, MAX_int64 },
			{ TEXT("Reward.Affix.Weapon.Power.T2"),
				Edemo_mapRewardAffixTier::Tier2, 5, 120 }
		};
		bPass = Set.TotalResolvedValue() < 0;
	}
	else if (Case >= 28 && Case <= 38)
	{
		bPass &= Guaranteed.Stacks[0].AffixSet.AffixSetEventId.IsValid()
			&& Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
				Fdemo_mapItemIds::WeaponLevel1,
				1,
				Guaranteed.Stacks[0].AffixSet,
				&Error)
			&& Fdemo_mapRewardAffixSet().IsEmpty()
			&& Guaranteed.Stacks[0].AffixSet
				== FindWeaponPlan(3, true).Stacks[0].AffixSet;
	}
	else if (Case >= 39 && Case <= 43)
	{
		const FName AffixId = Case == 39
			? FName(TEXT("Reward.Affix.Weapon.Power.T3"))
			: Case == 40
				? FName(TEXT("Reward.Affix.Robe.Vitality.T3"))
				: Case == 41
					? FName(TEXT("Reward.Affix.Robe.Guard.T3"))
					: FName(TEXT("Reward.Affix.Accessory.Haste.T3"));
		const FName DefinitionId = Case == 39
			? Fdemo_mapItemIds::WeaponLevel1
			: (Case == 40 || Case == 41)
				? Fdemo_mapItemIds::ArmorRobeLevel1
				: Fdemo_mapItemIds::AccessoryLevel1;
		const auto* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		const auto Resolution = Definition
			? Fdemo_mapEquipmentEffectResolver::ResolveAffixes(
				*Definition, MakeSet(AffixId))
			: Fdemo_mapEquipmentEffectResolution();
		bPass = Resolution.bSuccess
			&& Resolution.Modifiers.Num() == 1;
	}
	else if (Case >= 44 && Case <= 53)
	{
		const int64 AffixValue =
			Guaranteed.Stacks[0].AffixSet.TotalResolvedValue();
		int64 Normal = 0, Jackpot = 0, Rare = 0, Combined = 0;
		bPass = Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::WeaponLevel1, 1, 10000,
				AffixValue, 0, Normal)
			&& Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::WeaponLevel1, 1, 60000,
				AffixValue, 0, Jackpot)
			&& Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::WeaponLevel1, 1, 10000,
				AffixValue, 17, Rare)
			&& Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::WeaponLevel1, 1, 60000,
				AffixValue, 17, Combined)
			&& Normal == 410 && Jackpot == 660
			&& Rare == 427 && Combined == 677
			&& Fdemo_mapRewardAffixPolicyRegistry::BuildDisplayLabel(
				Guaranteed.Stacks[0].AffixSet)
				== TEXT("POWER III +12 ATK");
	}
	else
	{
		const auto* Wood =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::ChestMainWood);
		const auto* Ore =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::ChestMainOre);
		const FGuid Run(
			0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF01);
		const auto WoodPlan = Wood
			? Fdemo_mapRewardSourceProjectionPlanner::Plan(
				demo_mapRewardProjectionTestSupport::BindPrototype(*Wood),
				Run)
			: Fdemo_mapRewardSourceProjectionResult();
		const auto OrePlan = Ore
			? Fdemo_mapRewardSourceProjectionPlanner::Plan(
				demo_mapRewardProjectionTestSupport::BindPrototype(*Ore),
				Run)
			: Fdemo_mapRewardSourceProjectionResult();
		int32 LegacyProjectionCount = 0;
		for (const auto& Projection :
			Fdemo_mapRewardSourceProjectionRegistry::GetAll())
		{
			LegacyProjectionCount += Projection.ProjectionId
					!= Fdemo_mapRewardProjectionIds::
						CorpseBossPrototype
				? 1 : 0;
		}
		bPass = WoodPlan.IsSuccess() && OrePlan.IsSuccess()
			&& WoodPlan.Trace.AffixedEquipmentCount == 0
			&& OrePlan.Trace.AffixedEquipmentCount == 0
			&& WoodPlan.Trace.GeneratedTotalValue
				<= WoodPlan.Trace.RandomizedBudget
			&& OrePlan.Trace.GeneratedTotalValue
				<= OrePlan.Trace.RandomizedBudget
			&& LegacyProjectionCount == 8
			&& Fdemo_mapRewardSourceProjectionRegistry::
				GetAll().Num() == 9;
	}

	TestTrue(AffixNames[Case], bPass);
	return true;
}

void Fdemo_mapRewardAffixPityTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PityNames); ++Index)
	{
		OutBeautifiedNames.Add(PityNames[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

bool Fdemo_mapRewardAffixPityTests::RunTest(
	const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const FPlanFixture State0 = FindWeaponPlan(0, false);
	const FPlanFixture State1 = FindWeaponPlan(1, false);
	const FPlanFixture State2 = FindWeaponPlan(2, false);
	const FPlanFixture State3 = FindWeaponPlan(3, true);
	bool bPass = State0.bFound && State1.bFound
		&& State2.bFound && State3.bFound;

	if (Case == 0 || Case == 1)
	{
		bPass &= Fdemo_mapRewardAffixPolicyRegistry::PityPolicyId
				== FName(TEXT("Reward.AffixPity.WeaponHighTier.Default"))
			&& Fdemo_mapRewardAffixPolicyRegistry::PityChannelId
				== FName(TEXT("Reward.Pity.Weapon.PowerTier3"));
	}
	else if (Case >= 2 && Case <= 5)
	{
		const FPlanFixture& Fixture = Case == 2 ? State0
			: Case == 3 ? State1 : Case == 4 ? State2 : State3;
		const int32 Expected = Case == 2 ? 1
			: Case == 3 ? 2 : Case == 4 ? 3 : 0;
		bPass &= Fixture.Trace.PityStateOut == Expected
			&& !Fixture.Trace.PityDecisions.IsEmpty()
			&& Fixture.Trace.PityDecisions[0].StateOut == Expected;
	}
	else if (Case >= 6 && Case <= 8)
	{
		const auto NaturalTier3 = FindWeaponPlan(Case - 6, true);
		bPass &= NaturalTier3.bFound
			&& NaturalTier3.Trace.PityStateOut == 0
			&& !NaturalTier3.Trace.PityDecisions[0].bGuaranteeApplied;
	}
	else if (Case >= 9 && Case <= 11)
	{
		TArray<Fdemo_mapRewardPlannedStack> Stacks;
		if (Case == 9)
		{
			Stacks.Add(EquipmentStack(
				Fdemo_mapItemIds::ArmorRobeLevel1));
		}
		Fdemo_mapRewardAffixPlanTrace Trace;
		bPass &= Fdemo_mapRewardAffixPlanner::Apply(
				Fdemo_mapRewardAffixPolicyRegistry::GetDefault(),
				FGuid(1, 2, 3, 4),
				TEXT("P5.NoWeapon"),
				TEXT("P5.NoWeapon.Projection"),
				1000,
				3,
				Stacks,
				Trace)
			&& Trace.PityDecisions.IsEmpty();
	}
	else if (Case == 12 || Case == 13)
	{
		TArray<Fdemo_mapRewardPlannedStack> Stacks;
		if (Case == 13)
		{
			Stacks.Add(EquipmentStack(
				Fdemo_mapItemIds::WeaponLevel1));
		}
		Fdemo_mapRewardAffixPlanTrace Trace;
		bPass &= Fdemo_mapRewardAffixPlanner::Apply(
				Fdemo_mapRewardAffixPolicyRegistry::GetDefault(),
				FGuid(5, 6, 7, 8),
				TEXT("P5.Threshold"),
				TEXT("P5.Threshold.Projection"),
				Case == 12 ? 1000 : 359,
				3,
				Stacks,
				Trace)
			&& Trace.PityStateOut == 3
			&& (Case == 12
				? Trace.PityDecisions.IsEmpty()
				: !Trace.PityDecisions.IsEmpty()
					&& !Trace.PityDecisions[0].bCommitRequired);
	}
	else if (Case >= 14 && Case <= 20)
	{
		Fdemo_mapRewardAffixPityLedger Ledger;
		const FGuid RunA(10, 20, 30, 40);
		const FGuid RunB(11, 21, 31, 41);
		bPass &= Ledger.GetState(
				RunA,
				Fdemo_mapRewardAffixPolicyRegistry::PityChannelId) == 0
			&& Ledger.Commit(
				RunA,
				Fdemo_mapRewardAffixPolicyRegistry::PityChannelId,
				TEXT("Source.A"), 0, 1)
			&& Ledger.GetState(
				RunA,
				Fdemo_mapRewardAffixPolicyRegistry::PityChannelId) == 1
			&& Ledger.GetState(
				RunB,
				Fdemo_mapRewardAffixPolicyRegistry::PityChannelId) == 0
			&& Ledger.Commit(
				RunA,
				Fdemo_mapRewardAffixPolicyRegistry::PityChannelId,
				TEXT("Source.A"), 0, 1);
		Ledger.ClearRun(RunA);
		bPass &= Ledger.GetState(
			RunA,
			Fdemo_mapRewardAffixPolicyRegistry::PityChannelId) == 0;
	}
	else
	{
		bPass &= State3.Stacks[0].AffixSet.Acquisition
				== Edemo_mapRewardAffixAcquisition::PityGuaranteed
			&& State3.Trace.PityStateIn == 3
			&& State3.Trace.PityStateOut == 0
			&& State3.Trace.PityDecisions[0].bQualifyingTier3
			&& State3.Trace.PityDecisions[0].bGuaranteeApplied
			&& !State3.Trace.PityDecisions[0].Reason.IsEmpty()
			&& State3.Trace.PityStateOut <= 3;
	}

	TestTrue(PityNames[Case], bPass);
	return true;
}

#endif
