#include "demo_mapRewardFullMapDistribution.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardGenerator.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardFullMapDistributionTests,
	"demo_map.RewardFullMapDistribution",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

void Fdemo_mapRewardFullMapDistributionTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	static const TCHAR* Names[] = {
		TEXT("01.PolicyValid"),
		TEXT("02.TotalSlots149"),
		TEXT("03.StandardEnemies10"),
		TEXT("04.EliteEnemies3"),
		TEXT("05.Bosses1"),
		TEXT("06.Enemies14"),
		TEXT("07.BasicWood60"),
		TEXT("08.BasicOre60"),
		TEXT("09.BasicContainers120"),
		TEXT("10.HighValueContainers15"),
		TEXT("11.Containers135"),
		TEXT("12.TotalBase115500"),
		TEXT("13.StandardSubtotal12000"),
		TEXT("14.EliteSubtotal13500"),
		TEXT("15.BossSubtotal12000"),
		TEXT("16.BasicSubtotal48000"),
		TEXT("17.HighValueSubtotal30000"),
		TEXT("18.UniqueSlotIds"),
		TEXT("19.UniqueSourceRoles"),
		TEXT("20.UniqueMarkerIds"),
		TEXT("21.UniqueEnemyEncounters"),
		TEXT("22.UniqueContainerOrdinals"),
		TEXT("23.AllSlotsValid"),
		TEXT("24.AllProfilesResolve"),
		TEXT("25.AllProfileBasesExact"),
		TEXT("26.AllProjectionsResolve"),
		TEXT("27.AllRuntimeProjectionsValid"),
		TEXT("28.RuntimeRoleOverrideExact"),
		TEXT("29.RuntimeProjectionIdentityExact"),
		TEXT("30.RuntimeTagsExact"),
		TEXT("31.RuntimeBaseExact"),
		TEXT("32.AllOffsetsFinite"),
		TEXT("33.AllOffsetsBounded"),
		TEXT("34.AllEnemyRecordsValid"),
		TEXT("35.ContainerAnchorsLegal"),
		TEXT("36.WoodDeclarationPositive"),
		TEXT("37.OreDeclarationPositive"),
		TEXT("38.BossRolePreserved"),
		TEXT("39.BossProjectionPreserved"),
		TEXT("40.BossEncounterPreserved"),
		TEXT("41.BossBase12000"),
		TEXT("42.BossProfileExact"),
		TEXT("43.BossSemanticTags"),
		TEXT("44.ExactlyOneBossRole"),
		TEXT("45.P7FiveEncountersRetained"),
		TEXT("46.AllWoodUseWoodProjection"),
		TEXT("47.AllOreUseOreProjection"),
		TEXT("48.AllHighUseHighProjection"),
		TEXT("49.AllStandardUseStandardProfile"),
		TEXT("50.AllEliteUseEliteProfile"),
		TEXT("51.BossUsesBossProfile"),
		TEXT("52.AllContainersUseContainerProfiles"),
		TEXT("53.AllSourceTagsNonEmpty"),
		TEXT("54.AllStableIdsNonEmpty"),
		TEXT("55.AllRoutesNonEmpty"),
		TEXT("56.AllAreasNonEmpty"),
		TEXT("57.AllMarkersNonEmpty"),
		TEXT("58.EverySlotFindable"),
		TEXT("59.EveryEnemyEncounterFindable"),
		TEXT("60.LookupArrayIndexIndependent"),
		TEXT("61.NoDuplicateTuple"),
		TEXT("62.StandardPlanSucceeds"),
		TEXT("63.ElitePlanSucceeds"),
		TEXT("64.BossPlanSucceeds"),
		TEXT("65.WoodPlanSucceeds"),
		TEXT("66.OrePlanSucceeds"),
		TEXT("67.HighValuePlanSucceeds"),
		TEXT("68.PlanTraceRoleMatches"),
		TEXT("69.PlanDefinitionsResolve"),
		TEXT("70.PlansNeverUseFallback"),
		TEXT("71.PlanDeterministic"),
		TEXT("72.NewRunChangesSeed"),
		TEXT("73.GenerationLedgerExactlyOnce"),
		TEXT("74.GenerationLedgerAcceptsNewRun"),
		TEXT("75.BossPlanHasEquipment"),
		TEXT("76.BossPlanStackBounds"),
		TEXT("77.ContainerPlansNonEmpty"),
		TEXT("78.EnemyProjectionHasThreeSections"),
		TEXT("79.ContainerProjectionHasOneSection"),
		TEXT("80.SumRecomputedExact"),
		TEXT("81.ClassSubtotalsExact"),
		TEXT("82.DeclaredContainerPlacementsUnique"),
		TEXT("83.EnemyMarkerTypesLegal"),
		TEXT("84.PolicySignatureStable")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		OutBeautifiedNames.Add(Names[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

namespace
{
	const Fdemo_mapFullMapRewardSlot* First(
		Edemo_mapFullMapRewardClass RewardClass)
	{
		return Fdemo_mapRewardFullMapDistribution::GetSlots().
			FindByPredicate(
				[RewardClass](const Fdemo_mapFullMapRewardSlot& Slot)
				{
					return Slot.RewardClass == RewardClass;
				});
	}

	FGuid TestRun(int32 Number)
	{
		return FGuid(
			0x50380000u,
			0x46554C4Cu,
			0x4D415000u,
			static_cast<uint32>(Number));
	}

	Fdemo_mapRewardSourceProjectionResult Plan(
		Edemo_mapFullMapRewardClass RewardClass,
		FGuid RunId)
	{
		const auto* Slot = First(RewardClass);
		return Slot
			? Fdemo_mapRewardSourceProjectionPlanner::Plan(
				Fdemo_mapRewardFullMapDistribution::
					BuildProjection(*Slot),
				RunId)
			: Fdemo_mapRewardSourceProjectionResult();
	}

	int64 Subtotal(Edemo_mapFullMapRewardClass RewardClass)
	{
		int64 Result = 0;
		for (const auto& Slot :
			Fdemo_mapRewardFullMapDistribution::GetSlots())
		{
			if (Slot.RewardClass == RewardClass)
			{
				Result += Slot.BaseSourceValue;
			}
		}
		return Result;
	}

	bool AllClass(
		Edemo_mapFullMapRewardClass RewardClass,
		TFunctionRef<bool(const Fdemo_mapFullMapRewardSlot&)> Predicate)
	{
		for (const auto& Slot :
			Fdemo_mapRewardFullMapDistribution::GetSlots())
		{
			if (Slot.RewardClass == RewardClass
				&& !Predicate(Slot))
			{
				return false;
			}
		}
		return true;
	}
}

bool Fdemo_mapRewardFullMapDistributionTests::RunTest(
	const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const auto& Slots = Fdemo_mapRewardFullMapDistribution::GetSlots();
	const auto Counts = Fdemo_mapRewardFullMapDistribution::Count();
	const auto* Boss = First(Edemo_mapFullMapRewardClass::Boss);
	const FGuid RunA = TestRun(801);
	const FGuid RunB = TestRun(802);
	const auto StandardPlan =
		Plan(Edemo_mapFullMapRewardClass::EnemyStandard, RunA);
	const auto ElitePlan =
		Plan(Edemo_mapFullMapRewardClass::EnemyElite, RunA);
	const auto BossPlan =
		Plan(Edemo_mapFullMapRewardClass::Boss, RunA);
	const auto WoodPlan =
		Plan(Edemo_mapFullMapRewardClass::ContainerBasicWood, RunA);
	const auto OrePlan =
		Plan(Edemo_mapFullMapRewardClass::ContainerBasicOre, RunA);
	const auto HighPlan =
		Plan(Edemo_mapFullMapRewardClass::ContainerHighValue, RunA);
	const TArray<Fdemo_mapRewardSourceProjectionResult> Plans = {
		StandardPlan, ElitePlan, BossPlan, WoodPlan, OrePlan, HighPlan
	};
	const auto Check = [this](const TCHAR* Label, bool bValue)
	{
		TestTrue(Label, bValue);
	};
	const auto UniqueNames =
		[&Slots](TFunctionRef<FName(const Fdemo_mapFullMapRewardSlot&)> Select)
		{
			TSet<FName> Values;
			for (const auto& Slot : Slots)
			{
				Values.Add(Select(Slot));
			}
			return Values.Num() == Slots.Num();
		};

	switch (Case)
	{
	case 0:
	{
		FString Error;
		Check(TEXT("Policy validates"),
			Fdemo_mapRewardFullMapDistribution::Validate(&Error));
		break;
	}
	case 1: Check(TEXT("149 slots"), Slots.Num() == 149); break;
	case 2: Check(TEXT("10 Standard"), Counts.StandardEnemies == 10); break;
	case 3: Check(TEXT("3 Elite"), Counts.EliteEnemies == 3); break;
	case 4: Check(TEXT("1 Boss"), Counts.Bosses == 1); break;
	case 5: Check(TEXT("14 enemies"), Counts.EnemyTotal() == 14); break;
	case 6: Check(TEXT("60 Wood"), Counts.BasicWoodContainers == 60); break;
	case 7: Check(TEXT("60 Ore"), Counts.BasicOreContainers == 60); break;
	case 8: Check(TEXT("120 Basic"), Counts.BasicContainerTotal() == 120); break;
	case 9: Check(TEXT("15 High"), Counts.HighValueContainers == 15); break;
	case 10: Check(TEXT("135 containers"), Counts.ContainerTotal() == 135); break;
	case 11: Check(TEXT("115500 base"), Counts.BaseSourceValue == 115500); break;
	case 12: Check(TEXT("Standard subtotal"), Subtotal(Edemo_mapFullMapRewardClass::EnemyStandard) == 12000); break;
	case 13: Check(TEXT("Elite subtotal"), Subtotal(Edemo_mapFullMapRewardClass::EnemyElite) == 13500); break;
	case 14: Check(TEXT("Boss subtotal"), Subtotal(Edemo_mapFullMapRewardClass::Boss) == 12000); break;
	case 15:
		Check(TEXT("Basic subtotal"),
			Subtotal(Edemo_mapFullMapRewardClass::ContainerBasicWood)
				+ Subtotal(Edemo_mapFullMapRewardClass::ContainerBasicOre)
				== 48000);
		break;
	case 16: Check(TEXT("High subtotal"), Subtotal(Edemo_mapFullMapRewardClass::ContainerHighValue) == 30000); break;
	case 17: Check(TEXT("Unique slot ids"), UniqueNames([](const auto& Slot){ return Slot.SlotId; })); break;
	case 18: Check(TEXT("Unique source roles"), UniqueNames([](const auto& Slot){ return Slot.StableSourceRoleId; })); break;
	case 19: Check(TEXT("Unique marker ids"), UniqueNames([](const auto& Slot){ return Slot.MarkerId; })); break;
	case 20:
	{
		TSet<FName> Values;
		for (const auto& Slot : Slots)
		{
			if (Slot.IsEnemy()) Values.Add(Slot.EnemyRecord.Identity.EncounterId);
		}
		Check(TEXT("Unique enemy encounters"), Values.Num() == 14);
		break;
	}
	case 21:
	{
		TSet<int32> Values;
		for (const auto& Slot : Slots)
		{
			if (Slot.IsContainer()) Values.Add(Slot.ContainerOrdinal);
		}
		Check(TEXT("Unique container ordinals"), Values.Num() == 135);
		break;
	}
	case 22: Check(TEXT("All slots valid"), Algo::AllOf(Slots, [](const auto& Slot){ return Slot.IsValid(); })); break;
	case 23: Check(TEXT("Profiles resolve"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(Slot.BudgetProfileId) != nullptr; })); break;
	case 24: Check(TEXT("Profile bases exact"), Algo::AllOf(Slots, [](const auto& Slot){ const auto* Profile = Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(Slot.BudgetProfileId); return Profile && Profile->BaseValue == Slot.BaseSourceValue; })); break;
	case 25: Check(TEXT("Projections resolve"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardSourceProjectionRegistry::Find(Slot.ProjectionId) != nullptr; })); break;
	case 26: Check(TEXT("Runtime projections valid"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot).IsValid(); })); break;
	case 27: Check(TEXT("Role overrides exact"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot).StableSourceRoleId == Slot.StableSourceRoleId; })); break;
	case 28: Check(TEXT("Projection ids exact"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot).ProjectionId == Slot.ProjectionId; })); break;
	case 29: Check(TEXT("Tags exact"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot).SourceTags == Slot.SourceTags; })); break;
	case 30: Check(TEXT("Bases exact"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot).BaseSourceValue == Slot.BaseSourceValue; })); break;
	case 31: Check(TEXT("Offsets finite"), Algo::AllOf(Slots, [](const auto& Slot){ return FMath::IsFinite(Slot.LocalOffset.X) && FMath::IsFinite(Slot.LocalOffset.Y) && FMath::IsFinite(Slot.LocalOffset.Z); })); break;
	case 32: Check(TEXT("Offsets bounded"), Algo::AllOf(Slots, [](const auto& Slot){ return Slot.LocalOffset.GetAbsMax() <= 10000.0f; })); break;
	case 33: Check(TEXT("Enemy records valid"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.IsEnemy() || Slot.EnemyRecord.IsValid(); })); break;
	case 34: Check(TEXT("Container anchors legal"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.IsContainer() || (Slot.SourceMarkerType == TEXT("Chest") && Slot.SourceMarkerIndex >= 0 && Slot.SourceMarkerIndex <= 2); })); break;
	case 35: Check(TEXT("Wood positive"), Counts.BasicWoodContainers > 0); break;
	case 36: Check(TEXT("Ore positive"), Counts.BasicOreContainers > 0); break;
	case 37: Check(TEXT("Boss role"), Boss && Boss->StableSourceRoleId == Fdemo_mapRewardSourceRoleIds::BossPrototype); break;
	case 38: Check(TEXT("Boss projection"), Boss && Boss->ProjectionId == Fdemo_mapRewardProjectionIds::CorpseBossPrototype); break;
	case 39: Check(TEXT("Boss encounter"), Boss && Boss->EnemyRecord.Identity.EncounterId == Fdemo_mapEnemyEncounterIds::MainMeleeHeavy); break;
	case 40: Check(TEXT("Boss base"), Boss && Boss->BaseSourceValue == 12000); break;
	case 41: Check(TEXT("Boss profile"), Boss && Boss->BudgetProfileId == Fdemo_mapRewardBudgetProfileIds::Boss); break;
	case 42: Check(TEXT("Boss tags"), Boss && Boss->SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::SourceBoss) && Boss->SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::SourceCorpse) && Boss->SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::ValueHigh) && Boss->SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::Generated)); break;
	case 43: Check(TEXT("One Boss role"), Slots.FilterByPredicate([](const auto& Slot){ return Slot.StableSourceRoleId == Fdemo_mapRewardSourceRoleIds::BossPrototype; }).Num() == 1); break;
	case 44:
		Check(TEXT("P7 encounters retained"),
			Algo::AllOf(
				Fdemo_mapEnemyEncounterConfig::GetSpawnRecords(),
				[](const auto& Record)
				{
					return Fdemo_mapRewardFullMapDistribution::
						FindEnemyByEncounterId(
							Record.Identity.EncounterId) != nullptr;
				}));
		break;
	case 45: Check(TEXT("Wood projection"), AllClass(Edemo_mapFullMapRewardClass::ContainerBasicWood, [](const auto& Slot){ return Slot.ProjectionId == Fdemo_mapRewardProjectionIds::ChestMainWood; })); break;
	case 46: Check(TEXT("Ore projection"), AllClass(Edemo_mapFullMapRewardClass::ContainerBasicOre, [](const auto& Slot){ return Slot.ProjectionId == Fdemo_mapRewardProjectionIds::ChestMainOre; })); break;
	case 47: Check(TEXT("High projection"), AllClass(Edemo_mapFullMapRewardClass::ContainerHighValue, [](const auto& Slot){ return Slot.ProjectionId == Fdemo_mapRewardProjectionIds::ChestSideHighValue; })); break;
	case 48: Check(TEXT("Standard profile"), AllClass(Edemo_mapFullMapRewardClass::EnemyStandard, [](const auto& Slot){ return Slot.BudgetProfileId == Fdemo_mapRewardBudgetProfileIds::EnemyStandard; })); break;
	case 49: Check(TEXT("Elite profile"), AllClass(Edemo_mapFullMapRewardClass::EnemyElite, [](const auto& Slot){ return Slot.BudgetProfileId == Fdemo_mapRewardBudgetProfileIds::EnemyElite; })); break;
	case 50: Check(TEXT("Boss profile exact"), AllClass(Edemo_mapFullMapRewardClass::Boss, [](const auto& Slot){ return Slot.BudgetProfileId == Fdemo_mapRewardBudgetProfileIds::Boss; })); break;
	case 51: Check(TEXT("Container profiles"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.IsContainer() || Slot.BudgetProfileId == (Slot.RewardClass == Edemo_mapFullMapRewardClass::ContainerHighValue ? Fdemo_mapRewardBudgetProfileIds::ContainerHighValue : Fdemo_mapRewardBudgetProfileIds::ContainerBasic); })); break;
	case 52: Check(TEXT("Tags nonempty"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.SourceTags.IsEmpty() && !Slot.SourceTags.Contains(NAME_None); })); break;
	case 53: Check(TEXT("Stable ids"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.SlotId.IsNone() && !Slot.StableSourceRoleId.IsNone(); })); break;
	case 54: Check(TEXT("Routes"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.RouteId.IsNone(); })); break;
	case 55: Check(TEXT("Areas"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.AreaId.IsNone(); })); break;
	case 56: Check(TEXT("Markers"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.MarkerId.IsNone(); })); break;
	case 57: Check(TEXT("Every slot findable"), Algo::AllOf(Slots, [](const auto& Slot){ return Fdemo_mapRewardFullMapDistribution::Find(Slot.SlotId) == &Slot; })); break;
	case 58: Check(TEXT("Every enemy findable"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.IsEnemy() || Fdemo_mapRewardFullMapDistribution::FindEnemyByEncounterId(Slot.EnemyRecord.Identity.EncounterId) == &Slot; })); break;
	case 59: Check(TEXT("Index-independent lookup"), Slots.Num() == 149 && Fdemo_mapRewardFullMapDistribution::Find(Slots[0].SlotId) == &Slots[0] && Fdemo_mapRewardFullMapDistribution::Find(Slots[74].SlotId) == &Slots[74] && Fdemo_mapRewardFullMapDistribution::Find(Slots[148].SlotId) == &Slots[148]); break;
	case 60:
	{
		TSet<FString> Tuples;
		for (const auto& Slot : Slots)
		{
			Tuples.Add(Slot.SlotId.ToString() + TEXT("|") + Slot.StableSourceRoleId.ToString() + TEXT("|") + Slot.MarkerId.ToString());
		}
		Check(TEXT("No duplicate tuple"), Tuples.Num() == Slots.Num());
		break;
	}
	case 61: Check(TEXT("Standard plan"), StandardPlan.IsSuccess()); break;
	case 62: Check(TEXT("Elite plan"), ElitePlan.IsSuccess()); break;
	case 63: Check(TEXT("Boss plan"), BossPlan.IsSuccess()); break;
	case 64: Check(TEXT("Wood plan"), WoodPlan.IsSuccess()); break;
	case 65: Check(TEXT("Ore plan"), OrePlan.IsSuccess()); break;
	case 66: Check(TEXT("High plan"), HighPlan.IsSuccess()); break;
	case 67: Check(TEXT("Trace roles"), Algo::AllOf(Slots, [RunA](const auto& Slot){ const auto PlanResult = Fdemo_mapRewardSourceProjectionPlanner::Plan(Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot), RunA); return PlanResult.IsSuccess() && PlanResult.Trace.StableSourceRoleId == Slot.StableSourceRoleId; })); break;
	case 68: Check(TEXT("Definitions resolve"), Algo::AllOf(Plans, [](const auto& PlanResult){ return PlanResult.IsSuccess() && Algo::AllOf(PlanResult.PlannedStacks, [](const auto& Stack){ return Fdemo_mapItemDefinitions::Find(Stack.DefinitionId) != nullptr; }); })); break;
	case 69: Check(TEXT("No fallback"), Algo::AllOf(Plans, [](const auto& PlanResult){ return PlanResult.IsSuccess() && !PlanResult.Trace.bFallbackUsed; })); break;
	case 70:
	{
		const auto Replay = Plan(Edemo_mapFullMapRewardClass::Boss, RunA);
		Check(TEXT("Deterministic"), Replay.IsSuccess() && Replay.Trace.EffectiveSeed == BossPlan.Trace.EffectiveSeed && Replay.PlannedStacks == BossPlan.PlannedStacks);
		break;
	}
	case 71:
	{
		const auto Other = Plan(Edemo_mapFullMapRewardClass::Boss, RunB);
		Check(TEXT("New Run changes seed"), Other.IsSuccess() && Other.Trace.EffectiveSeed != BossPlan.Trace.EffectiveSeed);
		break;
	}
	case 72:
	{
		Fdemo_mapRewardGenerationSession Ledger;
		Check(TEXT("Exactly once"), Boss && Ledger.Commit(RunA, Boss->StableSourceRoleId) && !Ledger.Commit(RunA, Boss->StableSourceRoleId));
		break;
	}
	case 73:
	{
		Fdemo_mapRewardGenerationSession Ledger;
		Check(TEXT("New Run accepted"), Boss && Ledger.Commit(RunA, Boss->StableSourceRoleId) && Ledger.Commit(RunB, Boss->StableSourceRoleId));
		break;
	}
	case 74: Check(TEXT("Boss Equipment"), BossPlan.PlannedStacks.ContainsByPredicate([](const auto& Stack){ return Stack.Section == Edemo_mapRuntimeContainerSection::Equipment; })); break;
	case 75: Check(TEXT("Boss stack bounds"), BossPlan.PlannedStacks.Num() >= 3 && BossPlan.PlannedStacks.Num() <= 6); break;
	case 76: Check(TEXT("Container nonempty"), !WoodPlan.PlannedStacks.IsEmpty() && !OrePlan.PlannedStacks.IsEmpty() && !HighPlan.PlannedStacks.IsEmpty()); break;
	case 77: Check(TEXT("Enemy sections"), Boss && Fdemo_mapRewardFullMapDistribution::BuildProjection(*Boss).Sections.Num() == 3); break;
	case 78:
	{
		const auto* Wood = First(Edemo_mapFullMapRewardClass::ContainerBasicWood);
		Check(TEXT("Container section"), Wood && Fdemo_mapRewardFullMapDistribution::BuildProjection(*Wood).Sections.Num() == 1);
		break;
	}
	case 79:
	{
		int64 Sum = 0;
		for (const auto& Slot : Slots) Sum += Slot.BaseSourceValue;
		Check(TEXT("Recomputed sum"), Sum == 115500);
		break;
	}
	case 80: Check(TEXT("Class subtotals"), Subtotal(Edemo_mapFullMapRewardClass::EnemyStandard) == 12000 && Subtotal(Edemo_mapFullMapRewardClass::EnemyElite) == 13500 && Subtotal(Edemo_mapFullMapRewardClass::Boss) == 12000 && Subtotal(Edemo_mapFullMapRewardClass::ContainerBasicWood) + Subtotal(Edemo_mapFullMapRewardClass::ContainerBasicOre) == 48000 && Subtotal(Edemo_mapFullMapRewardClass::ContainerHighValue) == 30000); break;
	case 81:
	{
		TSet<FString> Placements;
		for (const auto& Slot : Slots)
		{
			if (Slot.IsContainer())
			{
				Placements.Add(FString::Printf(TEXT("%d|%.1f|%.1f|%.1f"), Slot.SourceMarkerIndex, Slot.LocalOffset.X, Slot.LocalOffset.Y, Slot.LocalOffset.Z));
			}
		}
		Check(TEXT("Container placements unique"), Placements.Num() == 135);
		break;
	}
	case 82: Check(TEXT("Enemy marker types"), Algo::AllOf(Slots, [](const auto& Slot){ return !Slot.IsEnemy() || Slot.SourceMarkerType == TEXT("MeleeEnemySpawn") || Slot.SourceMarkerType == TEXT("RangedEnemySpawn") || Slot.SourceMarkerType == TEXT("HeavyEnemySpawn") || Slot.SourceMarkerType == TEXT("TrainingTargetSpawn"); })); break;
	case 83:
	{
		FString SignatureA;
		FString SignatureB;
		for (const auto& Slot : Slots) SignatureA += Slot.SlotId.ToString() + TEXT("|");
		for (const auto& Slot : Fdemo_mapRewardFullMapDistribution::GetSlots()) SignatureB += Slot.SlotId.ToString() + TEXT("|");
		Check(TEXT("Stable signature"), !SignatureA.IsEmpty() && SignatureA == SignatureB);
		break;
	}
	default:
		AddError(TEXT("Unknown Full Map Distribution case."));
		break;
	}
	return true;
}

#endif
