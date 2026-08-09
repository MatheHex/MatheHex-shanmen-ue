#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardGenerator.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	Fdemo_mapRewardGenerationRequest Request(uint64 Seed = 123456789ull)
	{
		Fdemo_mapRewardGenerationRequest Value;
		Value.RequestId = TEXT("P1.Automation.Request");
		Value.RunId = FGuid(1, 2, 3, 4);
		Value.LootSourceId =
			Fdemo_mapRewardSourceIds::ChestSideHighValue;
		Value.BudgetProfileId =
			Fdemo_mapRewardBudgetProfileIds::ContainerHighValue;
		Value.SourceTags = {
			Fdemo_mapRewardTagIds::SourceContainerGeneral,
			Fdemo_mapRewardTagIds::SourceContainerHighValue
		};
		Value.StableSeed = Seed;
		Value.TargetSection =
			Edemo_mapRuntimeContainerSection::Chest;
		Value.TargetCapacity =
			Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity;
		return Value;
	}

	Fdemo_mapRewardPoolEntry Entry(
		FName EntryId,
		FName DefinitionId,
		int64 Weight = 1,
		int32 MinStack = 1,
		int32 MaxStack = 1)
	{
		Fdemo_mapRewardPoolEntry Value;
		Value.EntryId = EntryId;
		Value.DefinitionId = DefinitionId;
		Value.ItemTags = {
			Fdemo_mapRewardTagIds::ItemMaterialWood
		};
		Value.RequiredSourceTags = {
			Fdemo_mapRewardTagIds::SourceContainerGeneral,
			Fdemo_mapRewardTagIds::SourceContainerHighValue
		};
		Value.Weight = Weight;
		Value.MinStack = MinStack;
		Value.MaxStack = MaxStack;
		return Value;
	}

	Fdemo_mapRewardBudgetProfile Profile(int64 BaseValue = 2000)
	{
		Fdemo_mapRewardBudgetProfile Value;
		Value.ProfileId =
			Fdemo_mapRewardBudgetProfileIds::ContainerHighValue;
		Value.PlannedSourceCount = 15;
		Value.BaseValue = BaseValue;
		Value.MinMultiplierBps = 10000;
		Value.MaxMultiplierBps = 10000;
		return Value;
	}

	bool PlanIsLegal(const Fdemo_mapRewardGenerationResult& Result)
	{
		if (!Result.IsSuccess()
			|| Result.PlannedStacks.IsEmpty()
			|| Result.Trace.GeneratedTotalValue < 0
			|| Result.Trace.GeneratedTotalValue >
				Result.Trace.RandomizedBudget
			|| Result.Trace.ResidualValue !=
				Result.Trace.RandomizedBudget
					- Result.Trace.GeneratedTotalValue)
		{
			return false;
		}
		for (int32 Index = 0;
			Index < Result.PlannedStacks.Num();
			++Index)
		{
			const Fdemo_mapRewardPlannedStack& Stack =
				Result.PlannedStacks[Index];
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
			if (!Definition
				|| Definition->SellPrice != Stack.UnitValue
				|| Stack.StackCount <= 0
				|| Stack.StackCount > Definition->MaxStackSize
				|| Stack.TotalValue !=
					Stack.UnitValue * Stack.StackCount
				|| Stack.SlotIndex != Index
				|| Stack.Section !=
					Edemo_mapRuntimeContainerSection::Chest)
			{
				return false;
			}
		}
		return true;
	}

	bool RunRewardCase(FAutomationTestBase& Test, int32 Case)
	{
		const auto& Profiles =
			Fdemo_mapRewardGenerationRegistry::GetBudgetProfiles();
		const auto& Pool =
			Fdemo_mapRewardGenerationRegistry::GetHighValueContainerPool();
		const auto& Sources =
			Fdemo_mapRewardGenerationRegistry::GetChestSources();
		switch (Case)
		{
		case 1:
		{
			FString Error;
			Test.TestTrue(
				TEXT("Reward registry validates"),
				Fdemo_mapRewardGenerationRegistry::Validate(&Error));
			break;
		}
		case 2:
			Test.TestEqual(TEXT("Five Budget Profiles"), Profiles.Num(), 5);
			break;
		case 3:
			Test.TestTrue(
				TEXT("Budget profile contract"),
				Profiles[0].PlannedSourceCount == 10
				&& Profiles[0].BaseValue == 1200
				&& Profiles[1].PlannedSourceCount == 3
				&& Profiles[1].BaseValue == 4500
				&& Profiles[2].PlannedSourceCount == 15
				&& Profiles[2].BaseValue == 2000
				&& Profiles[3].PlannedSourceCount == 1
				&& Profiles[3].BaseValue == 12000
				&& Profiles[4].PlannedSourceCount == 120
				&& Profiles[4].BaseValue == 400);
			break;
		case 4:
			Test.TestTrue(
				TEXT("Normal multiplier centralized"),
				!Profiles.ContainsByPredicate([](const auto& Value)
				{
					return Value.MinMultiplierBps != 8000
						|| Value.MaxMultiplierBps != 12000;
				}));
			break;
		case 5:
			Test.TestTrue(
				TEXT("Pool uses registered positive SellPrice"),
				!Pool.IsEmpty()
				&& !Pool.ContainsByPredicate([](const auto& Value)
				{
					const auto* Definition =
						Fdemo_mapItemDefinitions::Find(
							Value.DefinitionId);
					return !Definition
						|| Definition->SellPrice <= 0;
				}));
			break;
		case 6:
			Test.TestTrue(
				TEXT("Only side Chest is generated"),
				Sources.Num() == 3
				&& Sources[0].Mode ==
					Edemo_mapRewardSourceMode::FixedTable
				&& Sources[1].Mode ==
					Edemo_mapRewardSourceMode::FixedTable
				&& Sources[2].Mode ==
					Edemo_mapRewardSourceMode::GeneratedReward);
			break;
		case 7:
			Test.TestTrue(
				TEXT("Side fixed table is explicit fallback"),
				Sources[2].bAllowFixedFallbackOnFailure
				&& Sources[2].FixedFallbackTableId ==
					Fdemo_mapFixedLootTableIds::ChestSideA
				&& Fdemo_mapFixedLootTableRegistry::Find(
					Sources[2].FixedFallbackTableId) != nullptr);
			break;
		case 8:
			Test.TestEqual(
				TEXT("Eight fixed tables retained"),
				Fdemo_mapFixedLootTableRegistry::GetAll().Num(),
				8);
			break;
		case 9:
		{
			const uint64 A =
				Fdemo_mapRewardGenerator::ComputeStableSeed(
					FGuid(1, 2, 3, 4),
					TEXT("Source"),
					TEXT("Profile"));
			const uint64 B =
				Fdemo_mapRewardGenerator::ComputeStableSeed(
					FGuid(1, 2, 3, 4),
					TEXT("Source"),
					TEXT("Profile"));
			Test.TestEqual(TEXT("Stable seed"), A, B);
			break;
		}
		case 10:
			Test.TestNotEqual(
				TEXT("Source identity affects seed"),
				Fdemo_mapRewardGenerator::ComputeStableSeed(
					FGuid(1, 2, 3, 4),
					TEXT("Source.A"),
					TEXT("Profile")),
				Fdemo_mapRewardGenerator::ComputeStableSeed(
					FGuid(1, 2, 3, 4),
					TEXT("Source.B"),
					TEXT("Profile")));
			break;
		case 11:
		{
			const auto A = Fdemo_mapRewardGenerator::Generate(
				Request(999));
			const auto B = Fdemo_mapRewardGenerator::Generate(
				Request(999));
			Test.TestTrue(TEXT("Plan is deterministic"), A == B);
			break;
		}
		case 12:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			Test.TestTrue(
				TEXT("Budget in normal range"),
				Result.Trace.RandomizedBudget >= 1600
				&& Result.Trace.RandomizedBudget <= 2400);
			break;
		}
		case 13:
			Test.TestTrue(
				TEXT("Generated plan is legal"),
				PlanIsLegal(
					Fdemo_mapRewardGenerator::Generate(Request())));
			break;
		case 14:
		{
			const auto Value = Request(777);
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Value);
			Test.TestTrue(
				TEXT("Trace identities are complete"),
				Result.Trace.RequestId == Value.RequestId
				&& Result.Trace.RunId == Value.RunId
				&& Result.Trace.LootSourceId ==
					Value.LootSourceId
				&& Result.Trace.BudgetProfileId ==
					Value.BudgetProfileId
				&& Result.Trace.EffectiveSeed == 777);
			break;
		}
		case 15:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			Test.TestTrue(
				TEXT("Capacity and slots bounded"),
				Result.PlannedStacks.Num() <= 6
				&& !Result.PlannedStacks.ContainsByPredicate(
					[](const auto& Value)
					{
						return Value.SlotIndex < 0
							|| Value.SlotIndex >= 6;
					}));
			break;
		}
		case 16:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			int64 Sum = 0;
			for (const auto& Stack : Result.PlannedStacks)
			{
				Sum += Stack.TotalValue;
			}
			Test.TestEqual(
				TEXT("Trace total equals stack total"),
				Sum,
				Result.Trace.GeneratedTotalValue);
			break;
		}
		case 17:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			Test.TestTrue(
				TEXT("Capacity limitation is explicit"),
				Result.Status !=
					Edemo_mapRewardGenerationStatus::CapacityLimited
				|| (Result.Trace.bCapacityLimited
					&& Result.Trace.Diagnostic ==
						TEXT("capacity_limited")));
			break;
		}
		case 18:
		{
			auto Value = Request();
			Value.SourceTags = { TEXT("Reward.Source.Unrelated") };
			Test.TestEqual(
				TEXT("Tag mismatch rejects"),
				Fdemo_mapRewardGenerator::Generate(Value).Status,
				Edemo_mapRewardGenerationStatus::NoEligibleItem);
			break;
		}
		case 19:
		{
			auto A = Request(789);
			auto B = A;
			Algo::Reverse(B.SourceTags);
			Test.TestTrue(
				TEXT("Source tag order does not alter plan"),
				Fdemo_mapRewardGenerator::Generate(A)
					== Fdemo_mapRewardGenerator::Generate(B));
			break;
		}
		case 20:
		{
			const TArray<Fdemo_mapRewardPoolEntry> Values = {
				Entry(TEXT("Missing"), TEXT("Missing.Definition"))
			};
			Test.TestEqual(
				TEXT("Missing Definition rejects"),
				Fdemo_mapRewardGenerator::GenerateWithData(
					Request(),
					Profile(),
					Values).Status,
				Edemo_mapRewardGenerationStatus::MissingDefinition);
			break;
		}
		case 21:
		{
			const TArray<Fdemo_mapRewardPoolEntry> Values = {
				Entry(
					TEXT("ZeroValue"),
					Fdemo_mapItemIds::HeavyPracticeBlade)
			};
			Test.TestEqual(
				TEXT("Zero SellPrice rejects"),
				Fdemo_mapRewardGenerator::GenerateWithData(
					Request(),
					Profile(),
					Values).Status,
				Edemo_mapRewardGenerationStatus::InvalidUnitValue);
			break;
		}
		case 22:
			Test.TestEqual(
				TEXT("Empty pool rejects"),
				Fdemo_mapRewardGenerator::GenerateWithData(
					Request(),
					Profile(),
					{}).Status,
				Edemo_mapRewardGenerationStatus::InvalidPool);
			break;
		case 23:
		{
			const TArray<Fdemo_mapRewardPoolEntry> Values = {
				Entry(
					TEXT("Unaffordable"),
					Fdemo_mapItemIds::WeaponLevel4)
			};
			Test.TestEqual(
				TEXT("Budget too small is explicit"),
				Fdemo_mapRewardGenerator::GenerateWithData(
					Request(),
					Profile(1),
					Values).Status,
				Edemo_mapRewardGenerationStatus::BudgetTooSmall);
			break;
		}
		case 24:
		{
			auto Value = Request(0);
			Test.TestEqual(
				TEXT("Zero seed rejects"),
				Fdemo_mapRewardGenerator::Generate(Value).Status,
				Edemo_mapRewardGenerationStatus::InvalidRequest);
			break;
		}
		case 25:
		{
			auto Overflow = Profile(MAX_int64);
			Overflow.MinMultiplierBps = 12000;
			Overflow.MaxMultiplierBps = 12000;
			Test.TestEqual(
				TEXT("Budget arithmetic overflow rejects"),
				Fdemo_mapRewardGenerator::GenerateWithData(
					Request(),
					Overflow,
					Pool).Status,
				Edemo_mapRewardGenerationStatus::ArithmeticOverflow);
			break;
		}
		case 26:
		{
			auto Value = Request();
			Value.TargetCapacity = 1;
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Value);
			Test.TestTrue(
				TEXT("One-slot capacity is finite"),
				Result.IsSuccess()
				&& Result.PlannedStacks.Num() == 1
				&& Result.Trace.ResidualValue >= 0);
			break;
		}
		case 27:
		{
			Fdemo_mapRewardGenerationSession Session;
			const FGuid Run(1, 2, 3, 4);
			Test.TestTrue(
				TEXT("First source commits"),
				Session.Commit(Run, TEXT("Source")));
			Test.TestFalse(
				TEXT("Duplicate source rejects"),
				Session.Commit(Run, TEXT("Source")));
			Test.TestTrue(
				TEXT("Duplicate source recorded"),
				Session.IsProcessed(Run, TEXT("Source")));
			break;
		}
		case 28:
		{
			Fdemo_mapRewardGenerationSession Session;
			Session.Commit(FGuid(1, 2, 3, 4), TEXT("Source"));
			Session.Reset();
			Test.TestEqual(TEXT("Session reset"), Session.Num(), 0);
			break;
		}
		case 29:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			const auto Seed =
				Fdemo_mapRewardGenerator::BuildContainerSeed(Result);
			Test.TestTrue(
				TEXT("Container seed preserves plan"),
				Seed.Num() == Result.PlannedStacks.Num()
				&& !Seed.ContainsByPredicate(
					[&Result](const auto& Value)
					{
						const auto& Planned =
							Result.PlannedStacks[
								Value.SlotIndex];
						return Value.DefinitionId !=
								Planned.DefinitionId
							|| Value.StackCount !=
								Planned.StackCount
							|| Value.Section !=
								Planned.Section;
					}));
			break;
		}
		case 30:
		{
			Udemo_mapItemSubsystem* Items =
				NewObject<Udemo_mapItemSubsystem>(
					NewObject<UGameInstance>(GetTransientPackage()));
			Test.TestTrue(TEXT("Begin Run"), Items->BeginRun().bSuccess);
			const int32 Before =
				Items->GetAuthority().GetInstanceSnapshot().Num();
			TArray<FGuid> Ids;
			const TArray<Fdemo_mapContainerMaterializationRequest>
				Requests = {
					{ Fdemo_mapItemIds::SpiritWoodLevel1, 2 },
					{ Fdemo_mapItemIds::SpiritOreLevel1, 1 }
				};
			const auto Result =
				Items->MaterializeContainerItemsAtomically(
					FGuid::NewGuid(),
					Requests,
					[](const TArray<FGuid>& Values, FString&)
					{
						return Values.Num() == 2;
					},
					Ids);
			Test.TestTrue(
				TEXT("Atomic materialization commits all"),
				Result.bSuccess
				&& Ids.Num() == 2
				&& Items->GetAuthority()
					.GetInstanceSnapshot().Num() == Before + 2);
			break;
		}
		case 31:
		{
			Udemo_mapItemSubsystem* Items =
				NewObject<Udemo_mapItemSubsystem>(
					NewObject<UGameInstance>(GetTransientPackage()));
			Items->BeginRun();
			const int32 Before =
				Items->GetAuthority().GetInstanceSnapshot().Num();
			TArray<FGuid> Ids;
			const TArray<Fdemo_mapContainerMaterializationRequest>
				Requests = {
					{ Fdemo_mapItemIds::SpiritWoodLevel1, 2 },
					{ Fdemo_mapItemIds::SpiritOreLevel1, 1 }
				};
			const auto Result =
				Items->MaterializeContainerItemsAtomically(
					FGuid::NewGuid(),
					Requests,
					[](const TArray<FGuid>&, FString&)
					{
						return true;
					},
					Ids,
					1);
			Test.TestTrue(
				TEXT("Injected mutation rolls back exact state"),
				!Result.bSuccess
				&& Ids.IsEmpty()
				&& Items->GetAuthority()
					.GetInstanceSnapshot().Num() == Before);
			break;
		}
		case 32:
		{
			Udemo_mapItemSubsystem* Items =
				NewObject<Udemo_mapItemSubsystem>(
					NewObject<UGameInstance>(GetTransientPackage()));
			Items->BeginRun();
			const int32 Before =
				Items->GetAuthority().GetInstanceSnapshot().Num();
			TArray<FGuid> Ids;
			const TArray<Fdemo_mapContainerMaterializationRequest>
				Requests = {
					{ Fdemo_mapItemIds::SpiritWoodLevel1, 2 }
				};
			const auto Result =
				Items->MaterializeContainerItemsAtomically(
					FGuid::NewGuid(),
					Requests,
					[](const TArray<FGuid>&, FString& Error)
					{
						Error = TEXT("Injected finalizer failure.");
						return false;
					},
					Ids);
			Test.TestTrue(
				TEXT("Finalizer rejection rolls back exact state"),
				!Result.bSuccess
				&& Ids.IsEmpty()
				&& Items->GetAuthority()
					.GetInstanceSnapshot().Num() == Before);
			break;
		}
		case 33:
			Test.TestTrue(
				TEXT("Generated product source contract"),
				Sources[2].RewardSourceId ==
					Fdemo_mapRewardSourceIds::ChestSideHighValue
				&& Sources[2].BudgetProfileId ==
					Fdemo_mapRewardBudgetProfileIds::ContainerHighValue
				&& Sources[2].Capacity == 6);
			break;
		case 34:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			Test.TestTrue(
				TEXT("Plan cannot leak definitions outside pool"),
				!Result.PlannedStacks.ContainsByPredicate(
					[&Pool](const auto& Value)
					{
						return !Pool.ContainsByPredicate(
							[&Value](const auto& Candidate)
							{
								return Candidate.DefinitionId ==
									Value.DefinitionId;
							});
					}));
			break;
		}
		case 35:
		{
			bool bAllLegal = true;
			for (uint64 Seed = 1; Seed <= 128; ++Seed)
			{
				bAllLegal &= PlanIsLegal(
					Fdemo_mapRewardGenerator::Generate(
						Request(Seed)));
			}
			Test.TestTrue(
				TEXT("Many different seeds remain legal"),
				bAllLegal);
			break;
		}
		case 36:
		{
			const auto Result =
				Fdemo_mapRewardGenerator::Generate(Request());
			Test.TestTrue(
				TEXT("Eligible trace ordering is stable"),
				Result.Trace.OrderedEligibleEntryIds.Num() ==
					Pool.Num()
				&& Algo::IsSorted(
					Result.Trace.OrderedEligibleEntryIds,
					[](FName A, FName B)
					{
						return A.LexicalLess(B);
					}));
			break;
		}
		default:
			return false;
		}
		return true;
	}
}

#define REWARD_TEST(TypeName, Number, DisplayName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		TypeName, \
		"demo_map.RewardGeneration." DisplayName, \
		EAutomationTestFlags::EditorContext \
			| EAutomationTestFlags::EngineFilter) \
	bool TypeName::RunTest(const FString&) \
	{ \
		return RunRewardCase(*this, Number); \
	}

REWARD_TEST(FRewardGeneration01, 1, "01.RegistryValid")
REWARD_TEST(FRewardGeneration02, 2, "02.FiveBudgetProfiles")
REWARD_TEST(FRewardGeneration03, 3, "03.BudgetProfileContract")
REWARD_TEST(FRewardGeneration04, 4, "04.NormalMultiplier")
REWARD_TEST(FRewardGeneration05, 5, "05.SellPriceAuthority")
REWARD_TEST(FRewardGeneration06, 6, "06.SourceModes")
REWARD_TEST(FRewardGeneration07, 7, "07.ExplicitFallback")
REWARD_TEST(FRewardGeneration08, 8, "08.FixedTablesRetained")
REWARD_TEST(FRewardGeneration09, 9, "09.StableSeed")
REWARD_TEST(FRewardGeneration10, 10, "10.SourceAffectsSeed")
REWARD_TEST(FRewardGeneration11, 11, "11.DeterministicPlan")
REWARD_TEST(FRewardGeneration12, 12, "12.BudgetRange")
REWARD_TEST(FRewardGeneration13, 13, "13.LegalPlan")
REWARD_TEST(FRewardGeneration14, 14, "14.TraceIdentity")
REWARD_TEST(FRewardGeneration15, 15, "15.CapacityBound")
REWARD_TEST(FRewardGeneration16, 16, "16.TotalValue")
REWARD_TEST(FRewardGeneration17, 17, "17.CapacityDiagnostic")
REWARD_TEST(FRewardGeneration18, 18, "18.TagMismatch")
REWARD_TEST(FRewardGeneration19, 19, "19.TagOrderIndependent")
REWARD_TEST(FRewardGeneration20, 20, "20.MissingDefinition")
REWARD_TEST(FRewardGeneration21, 21, "21.ZeroSellPrice")
REWARD_TEST(FRewardGeneration22, 22, "22.EmptyPool")
REWARD_TEST(FRewardGeneration23, 23, "23.BudgetTooSmall")
REWARD_TEST(FRewardGeneration24, 24, "24.ZeroSeed")
REWARD_TEST(FRewardGeneration25, 25, "25.CheckedOverflow")
REWARD_TEST(FRewardGeneration26, 26, "26.FiniteOneSlotPlan")
REWARD_TEST(FRewardGeneration27, 27, "27.DuplicateSource")
REWARD_TEST(FRewardGeneration28, 28, "28.SessionReset")
REWARD_TEST(FRewardGeneration29, 29, "29.ContainerSeed")
REWARD_TEST(FRewardGeneration30, 30, "30.AtomicMaterialize")
REWARD_TEST(FRewardGeneration31, 31, "31.AtomicMutationRollback")
REWARD_TEST(FRewardGeneration32, 32, "32.AtomicFinalizerRollback")
REWARD_TEST(FRewardGeneration33, 33, "33.ProductSourceContract")
REWARD_TEST(FRewardGeneration34, 34, "34.NoPoolLeak")
REWARD_TEST(FRewardGeneration35, 35, "35.ManySeedsLegal")
REWARD_TEST(FRewardGeneration36, 36, "36.StableEligibilityOrder")

#undef REWARD_TEST
#endif
