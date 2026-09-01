#include "demo_mapRewardSourceProjection.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardGenerator.h"
#include "demo_mapRewardProjectionTestSupport.h"
#include "Engine/GameInstance.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardSourceProjectionTests,
	"demo_map.RewardSourceProjection",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

void Fdemo_mapRewardSourceProjectionTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	static const TCHAR* Names[] = {
		TEXT("RegistrySingleAuthority"), TEXT("ProjectionCount"),
		TEXT("ProjectionIdsUnique"), TEXT("SourceRolesUnique"),
		TEXT("UnknownProjectionRejected"), TEXT("ProfilesResolve"),
		TEXT("TagOrderIndependent"), TEXT("DuplicateSectionRejected"),
		TEXT("InvalidWeightRejected"), TEXT("SectionBudgetExact"),
		TEXT("RoundingDeterministic"), TEXT("ResidualDeterministic"),
		TEXT("BudgetNeverExceeded"), TEXT("MainAWoodOnly"),
		TEXT("MainAExclusions"), TEXT("MainBOreOnly"),
		TEXT("MainBExclusions"), TEXT("SideChestP1Compatible"),
		TEXT("StandardBase1200"), TEXT("EliteBase4500"),
		TEXT("ThreeMainStandard"), TEXT("TwoSideElite"),
		TEXT("EquipmentEligibility"), TEXT("BackpackEligibility"),
		TEXT("BodyEligibility"), TEXT("BodyExclusions"),
		TEXT("MissingFleshExplicit"), TEXT("DuplicateLedgerRejected"),
		TEXT("NewRunNewIdentity"), TEXT("FiveCorpseFallbacks"),
		TEXT("ThreeChestFallbacks"), TEXT("NormalPlansNoFallback"),
		TEXT("MultiSectionSeed"), TEXT("MutationRollback"),
		TEXT("FinalizerRollback"), TEXT("FailureDoesNotCommit"),
		TEXT("SuccessCommitsOnce"), TEXT("SameGuidTransfer"),
		TEXT("ResetLeavesNoOrphans"), TEXT("TraceCompleteness")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		OutBeautifiedNames.Add(Names[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

namespace
{
	const Fdemo_mapRewardSourceProjection& StandardProjection()
	{
		static const Fdemo_mapRewardSourceProjection Projection =
			demo_mapRewardProjectionTestSupport::FindBound(
				Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard);
		return Projection;
	}

	bool PoolOnlyHas(
		const TArray<Fdemo_mapRewardPoolEntry>& Pool,
		const TArray<FName>& Allowed)
	{
		return !Pool.IsEmpty()
			&& !Pool.ContainsByPredicate(
				[&Allowed](const auto& Entry)
				{
					return !Entry.ItemTags.ContainsByPredicate(
						[&Allowed](FName Tag)
						{
							return Allowed.Contains(Tag);
						});
				});
	}
}

bool Fdemo_mapRewardSourceProjectionTests::RunTest(
	const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const auto& All = Fdemo_mapRewardSourceProjectionRegistry::GetAll();
	const FGuid RunId(
		0x10203040,
		0x50607080,
		0x90A0B0C0,
		0xD0E0F001 + Case);
	auto Plan = [&]()
	{
		return Fdemo_mapRewardSourceProjectionPlanner::Plan(
			StandardProjection(),
			RunId);
	};
	switch (Case)
	{
	case 0:
	{
		FString Error;
		TestTrue(TEXT("Registry validates"), Fdemo_mapRewardSourceProjectionRegistry::Validate(&Error));
		TestTrue(TEXT("Canonical manifest entries are valid unbound policy prototypes"),
			!All.ContainsByPredicate([](const auto& Projection)
			{
				return !Projection.IsPolicyPrototypeValid()
					|| Projection.IsValid();
			}));
		break;
	}
	case 1:
		TestEqual(TEXT("P2 eight plus P7 Boss"), All.Num(), 9);
		break;
	case 2:
	case 3:
	{
		TSet<FName> Values;
		for (const auto& Projection : All)
		{
			Values.Add(Case == 2
				? Projection.ProjectionId
				: Projection.StableSourceRoleId);
		}
		TestEqual(TEXT("Identity is unique"), Values.Num(), All.Num());
		break;
	}
	case 4:
		TestNull(TEXT("Unknown rejected"), Fdemo_mapRewardSourceProjectionRegistry::Find(TEXT("Unknown")));
		break;
	case 5:
	{
		const bool bAllResolve = !All.ContainsByPredicate(
			[](const auto& Projection)
			{
				return !Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
					Projection.BudgetProfileId);
			});
		TestTrue(TEXT("Every profile resolves"), bAllResolve);
		break;
	}
	case 6:
	{
		Fdemo_mapRewardSourceProjection Copy = StandardProjection();
		Algo::Reverse(Copy.SourceTags);
		TestEqual(TEXT("Tag order does not change plan"),
			Fdemo_mapRewardSourceProjectionPlanner::Plan(Copy, RunId).PlannedStacks,
			Plan().PlannedStacks);
		break;
	}
	case 7:
	{
		auto Copy = StandardProjection();
		const Fdemo_mapRewardProjectionSection Duplicate =
			Copy.Sections[0];
		Copy.Sections.Add(Duplicate);
		TestFalse(TEXT("Duplicate Section rejected"), Copy.IsValid());
		break;
	}
	case 8:
	{
		auto Copy = StandardProjection();
		Copy.Sections[0].BudgetWeightBps = 3999;
		TestFalse(TEXT("Invalid weight sum rejected"), Copy.IsValid());
		break;
	}
	case 9:
	case 10:
	{
		const auto Result = Plan();
		int64 Sum = 0;
		for (const auto& Section : Result.Trace.Sections)
		{
			Sum += Section.InitialBudget;
		}
		TestEqual(TEXT("Initial budgets exactly equal total"), Sum, Result.Trace.RandomizedBudget);
		break;
	}
	case 11:
		TestEqual(TEXT("Same input same trace residual"), Plan().Trace.ResidualValue, Plan().Trace.ResidualValue);
		break;
	case 12:
	{
		const auto Result = Plan();
		TestTrue(TEXT("Generated value under total budget"), Result.Trace.GeneratedTotalValue <= Result.Trace.RandomizedBudget);
		break;
	}
	case 13:
	case 14:
	case 15:
	case 16:
	{
		const int32 Index = Case < 15 ? 0 : 1;
		const auto& Projection = All[Index];
		const auto Pool = Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
			Projection,
			Projection.Sections[0]);
		const FName Allowed = Index == 0
			? Fdemo_mapRewardTagIds::ItemMaterialWood
			: Fdemo_mapRewardTagIds::ItemMaterialOre;
		TestTrue(TEXT("Basic container has only intended material"), PoolOnlyHas(Pool, { Allowed }));
		break;
	}
	case 17:
		TestEqual(TEXT("Side keeps P1 profile"), All[2].BudgetProfileId, Fdemo_mapRewardBudgetProfileIds::ContainerHighValue);
		break;
	case 18:
	case 19:
	{
		const FName Id = Case == 18
			? Fdemo_mapRewardBudgetProfileIds::EnemyStandard
			: Fdemo_mapRewardBudgetProfileIds::EnemyElite;
		const auto* Profile = Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(Id);
		TestEqual(TEXT("Enemy base"), Profile ? Profile->BaseValue : 0, Case == 18 ? 1200ll : 4500ll);
		break;
	}
	case 20:
	case 21:
	{
		int32 Count = 0;
		const FName Wanted = Case == 20
			? Fdemo_mapRewardBudgetProfileIds::EnemyStandard
			: Fdemo_mapRewardBudgetProfileIds::EnemyElite;
		for (int32 Index = 3; Index < All.Num(); ++Index)
		{
			Count += All[Index].BudgetProfileId == Wanted ? 1 : 0;
		}
		TestEqual(TEXT("Explicit enemy tier count"), Count, Case == 20 ? 3 : 2);
		break;
	}
	case 22:
	case 23:
	case 24:
	case 25:
	{
		const int32 SectionIndex = Case == 22 ? 0 : Case == 23 ? 1 : 2;
		const auto Pool = Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
			StandardProjection(),
			StandardProjection().Sections[SectionIndex]);
		const TArray<FName> Allowed = SectionIndex == 0
			? TArray<FName>{
				Fdemo_mapRewardTagIds::ItemEquipmentWeapon,
				Fdemo_mapRewardTagIds::ItemEquipmentRobe,
				Fdemo_mapRewardTagIds::ItemEquipmentAccessory,
				Fdemo_mapRewardTagIds::ItemEquipmentBackpack }
			: SectionIndex == 1
				? TArray<FName>{
					Fdemo_mapRewardTagIds::ItemConsumablePill,
					Fdemo_mapRewardTagIds::ItemMaterialWood,
					Fdemo_mapRewardTagIds::ItemMaterialOre,
					Fdemo_mapRewardTagIds::ItemEquipmentAccessory }
				: TArray<FName>{
					Fdemo_mapRewardTagIds::ItemConsumablePill,
					Fdemo_mapRewardTagIds::ItemBodyBone,
					Fdemo_mapRewardTagIds::ItemBodyInnerCore };
		TestTrue(TEXT("Section eligibility exact"), PoolOnlyHas(Pool, Allowed));
		break;
	}
	case 26:
		TestFalse(TEXT("No fabricated Flesh entry"),
			Fdemo_mapRewardGenerationRegistry::GetHighValueContainerPool().ContainsByPredicate(
				[](const auto& Entry)
				{
					return Entry.EntryId.ToString().Contains(TEXT("Flesh"));
				}));
		break;
	case 27:
	case 28:
	{
		Fdemo_mapRewardGenerationSession Session;
		TestTrue(TEXT("Initial commit"), Session.Commit(RunId, StandardProjection().StableSourceRoleId));
		if (Case == 27)
		{
			TestFalse(TEXT("Duplicate rejected"), Session.Commit(RunId, StandardProjection().StableSourceRoleId));
		}
		else
		{
			TestTrue(TEXT("New run accepted"), Session.Commit(FGuid::NewGuid(), StandardProjection().StableSourceRoleId));
		}
		break;
	}
	case 29:
	case 30:
	{
		const int32 Begin = Case == 29 ? 3 : 0;
		const int32 End = Case == 29 ? 8 : 3;
		bool bResolved = true;
		for (int32 Index = Begin; Index < End; ++Index)
		{
			bResolved &= Fdemo_mapFixedLootTableRegistry::Find(All[Index].FixedFallbackTableId) != nullptr;
		}
		TestTrue(TEXT("Fallbacks resolve"), bResolved);
		break;
	}
	case 31:
		TestFalse(TEXT("Normal plan never uses fallback"), Plan().Trace.bFallbackUsed);
		break;
	case 32:
	{
		const auto Seed = Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(Plan());
		TSet<Edemo_mapRuntimeContainerSection> Sections;
		for (const auto& Entry : Seed) Sections.Add(Entry.Section);
		TestEqual(TEXT("All three semantic sections seeded"), Sections.Num(), 3);
		break;
	}
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	{
		Udemo_mapItemSubsystem* Items =
			NewObject<Udemo_mapItemSubsystem>(
				NewObject<UGameInstance>(GetTransientPackage()));
		TestTrue(TEXT("Begin run"), Items->BeginRun().bSuccess);
		const int32 Before = Items->GetAuthority().GetInstanceSnapshot().Num();
		const FGuid ContainerId = FGuid::NewGuid();
		TArray<FGuid> Ids;
		const TArray<Fdemo_mapContainerMaterializationRequest> Requests = {
			{ Fdemo_mapItemIds::WeaponLevel1, 1 },
			{ Fdemo_mapItemIds::HealingPillLevel1, 1 },
			{ Fdemo_mapItemIds::SoulBone, 1 }
		};
		const bool bFinalizerPass = Case != 34 && Case != 35;
		const auto Result = Items->MaterializeContainerItemsAtomically(
			ContainerId,
			Requests,
			[bFinalizerPass](const TArray<FGuid>& Values, FString& Error)
			{
				if (!bFinalizerPass) Error = TEXT("Injected finalizer rejection.");
				return bFinalizerPass && Values.Num() == 3;
			},
			Ids,
			Case == 33 ? 1 : INDEX_NONE);
		if (Case == 33 || Case == 34 || Case == 35)
		{
			TestTrue(TEXT("Failure rolls back entire batch"),
				!Result.bSuccess && Ids.IsEmpty()
					&& Items->GetAuthority().GetInstanceSnapshot().Num() == Before);
		}
		else if (Case == 36)
		{
			Fdemo_mapRewardGenerationSession Session;
			TestTrue(TEXT("Success commits exactly once"),
				Result.bSuccess
					&& Session.Commit(RunId, StandardProjection().StableSourceRoleId)
					&& !Session.Commit(RunId, StandardProjection().StableSourceRoleId));
		}
		else if (Case == 37)
		{
			const FGuid SameId = Ids[0];
			TestTrue(TEXT("Same GUID transfers"),
				Result.bSuccess
					&& Items->TransferContainerItemToInventory(ContainerId, SameId).bSuccess
					&& Items->GetAuthority().FindInstance(SameId)
					&& Items->GetAuthority().FindInventorySlot(SameId) != INDEX_NONE);
		}
		else
		{
			Items->ResetForAutomation();
			TestEqual(TEXT("Reset removes all instances"),
				Items->GetAuthority().GetInstanceSnapshot().Num(), 0);
		}
		break;
	}
	case 39:
	{
		const auto Result = Plan();
		TestTrue(TEXT("Trace includes projection, budgets, redistribution and residual"),
			Result.IsSuccess()
				&& Result.Trace.ProjectionId == StandardProjection().ProjectionId
				&& Result.Trace.Sections.Num() == 3
				&& Result.Trace.RandomizedBudget > 0
				&& Result.Trace.ResidualValue >= 0
				&& !Result.Trace.bFallbackUsed);
		break;
	}
	default:
		AddError(TEXT("Unknown case"));
		break;
	}
	return true;
}

#endif
