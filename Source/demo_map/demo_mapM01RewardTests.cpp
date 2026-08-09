#include "demo_mapM01RewardDistribution.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardGenerator.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapM01RewardTests,
	"demo_map.M01.Reward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void Fdemo_mapM01RewardTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	static const TCHAR* Names[] = {
		TEXT("01.Policy149And115500"),
		TEXT("02.EnemyDistribution"),
		TEXT("03.ResourceDistribution"),
		TEXT("04.HighValueDistribution"),
		TEXT("05.StableIdentitiesUnique"),
		TEXT("06.ProfilesAndBases"),
		TEXT("07.ResourcePoolIsolation"),
		TEXT("08.TierPoolMapping"),
		TEXT("09.RepresentativePlans"),
		TEXT("10.BossEquipment"),
		TEXT("11.RunSeedAndLedger"),
		TEXT("12.NoFixedFallback")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		OutBeautifiedNames.Add(Names[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

namespace
{
	const Fdemo_mapM01RewardSlot* First(Edemo_mapM01RewardSourceClass RewardClass)
	{
		return Fdemo_mapM01RewardDistribution::GetSlots().FindByPredicate(
			[RewardClass](const auto& Slot) { return Slot.RewardClass == RewardClass; });
	}

	bool HasDefinitionTag(const Fdemo_mapRewardPoolEntry& Entry, FName Tag)
	{
		return Entry.ItemTags.Contains(Tag);
	}

	FGuid Run(int32 Number)
	{
		return FGuid(0x4D303150u, 0x34524557u, 0x41524400u, Number);
	}
}

bool Fdemo_mapM01RewardTests::RunTest(const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const auto& Slots = Fdemo_mapM01RewardDistribution::GetSlots();
	const auto Counts = Fdemo_mapM01RewardDistribution::Count();
	auto Check = [this](const TCHAR* Label, bool bValue) { TestTrue(Label, bValue); };
	switch (Case)
	{
	case 0:
	{
		FString Error;
		Check(TEXT("M01 policy validates"),
			Fdemo_mapM01RewardDistribution::Validate(&Error));
		Check(TEXT("149 stable sources"), Slots.Num() == 149);
		Check(TEXT("115500 base value"), Counts.BaseSourceValue == 115500);
		break;
	}
	case 1:
		Check(TEXT("4 LOW, 6 MID, 3 Elite, 1 Boss"),
			Counts.LowEnemies == 4 && Counts.MidEnemies == 6
			&& Counts.EliteEnemies == 3 && Counts.Bosses == 1
			&& Counts.EnemyTotal() == 14);
		break;
	case 2:
		Check(TEXT("Tier/material counts exact"),
			Counts.Tier1Wood == 24 && Counts.Tier1Ore == 24
			&& Counts.Tier2Wood == 24 && Counts.Tier2Ore == 24
			&& Counts.Tier3Wood == 12 && Counts.Tier3Ore == 12
			&& Counts.ResourceTotal() == 120);
		break;
	case 3:
		Check(TEXT("15 TIER_3 HIGH value sources"),
			Counts.HighValue == 15
			&& Algo::AllOf(Slots, [](const auto& Slot)
			{
				return Slot.RewardClass != Edemo_mapM01RewardSourceClass::HighValue
					|| (Slot.SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::Tier3)
						&& Slot.SourceTags.Contains(Fdemo_mapRewardProjectionTagIds::RiskHigh)
						&& Slot.BaseSourceValue == 2000);
			}));
		break;
	case 4:
	{
		TSet<FName> SlotIds, Roles, Markers;
		for (const auto& Slot : Slots)
		{
			SlotIds.Add(Slot.SlotId);
			Roles.Add(Slot.StableSourceRoleId);
			Markers.Add(Slot.MarkerId);
		}
		Check(TEXT("Slot/source/marker identities unique"),
			SlotIds.Num() == 149 && Roles.Num() == 149 && Markers.Num() == 149);
		break;
	}
	case 5:
		Check(TEXT("All profiles resolve with exact base"),
			Algo::AllOf(Slots, [](const auto& Slot)
			{
				const auto* Profile = Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
					Slot.BudgetProfileId);
				return Profile && Profile->BaseValue == Slot.BaseSourceValue;
			}));
		break;
	case 6:
	{
		const auto WoodProjection = Fdemo_mapM01RewardDistribution::BuildProjection(
			*First(Edemo_mapM01RewardSourceClass::ResourceWoodTier1));
		const auto OreProjection = Fdemo_mapM01RewardDistribution::BuildProjection(
			*First(Edemo_mapM01RewardSourceClass::ResourceOreTier1));
		const auto WoodPool = Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
			WoodProjection, WoodProjection.Sections[0]);
		const auto OrePool = Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
			OreProjection, OreProjection.Sections[0]);
		Check(TEXT("Wood and Ore pools do not cross"),
			!WoodPool.IsEmpty() && !OrePool.IsEmpty()
			&& Algo::AllOf(WoodPool, [](const auto& Entry)
			{
				return HasDefinitionTag(Entry, Fdemo_mapRewardTagIds::ItemMaterialWood)
					&& !HasDefinitionTag(Entry, Fdemo_mapRewardTagIds::ItemMaterialOre);
			})
			&& Algo::AllOf(OrePool, [](const auto& Entry)
			{
				return HasDefinitionTag(Entry, Fdemo_mapRewardTagIds::ItemMaterialOre)
					&& !HasDefinitionTag(Entry, Fdemo_mapRewardTagIds::ItemMaterialWood);
			}));
		break;
	}
	case 7:
	{
		const auto Tier1 = Fdemo_mapM01RewardDistribution::BuildProjection(
			*First(Edemo_mapM01RewardSourceClass::ResourceWoodTier1));
		const auto Tier2 = Fdemo_mapM01RewardDistribution::BuildProjection(
			*First(Edemo_mapM01RewardSourceClass::ResourceWoodTier2));
		const auto Tier3 = Fdemo_mapM01RewardDistribution::BuildProjection(
			*First(Edemo_mapM01RewardSourceClass::ResourceWoodTier3));
		auto LevelsFit = [](const auto& Projection, int32 Min, int32 Max)
		{
			const auto Pool = Fdemo_mapRewardSourceProjectionRegistry::BuildSectionPool(
				Projection, Projection.Sections[0]);
			return !Pool.IsEmpty() && Algo::AllOf(Pool, [Min, Max](const auto& Entry)
			{
				const auto* Definition = Fdemo_mapItemDefinitions::Find(Entry.DefinitionId);
				return Definition && Definition->Level >= Min && Definition->Level <= Max;
			});
		};
		Check(TEXT("Tier level mapping uses legal Registry definitions"),
			LevelsFit(Tier1, 1, 1) && LevelsFit(Tier2, 1, 2)
			&& LevelsFit(Tier3, 2, MAX_int32));
		break;
	}
	case 8:
	{
		static const Edemo_mapM01RewardSourceClass Classes[] = {
			Edemo_mapM01RewardSourceClass::EnemyLow,
			Edemo_mapM01RewardSourceClass::EnemyMid,
			Edemo_mapM01RewardSourceClass::EnemyElite,
			Edemo_mapM01RewardSourceClass::Boss,
			Edemo_mapM01RewardSourceClass::ResourceWoodTier1,
			Edemo_mapM01RewardSourceClass::ResourceOreTier2,
			Edemo_mapM01RewardSourceClass::ResourceWoodTier3,
			Edemo_mapM01RewardSourceClass::HighValue
		};
		bool bAllSucceeded = true;
		for (const auto RewardClass : Classes)
		{
			const auto* Slot = First(RewardClass);
			const auto Plan = Slot ? Fdemo_mapRewardSourceProjectionPlanner::Plan(
				Fdemo_mapM01RewardDistribution::BuildProjection(*Slot), Run(1))
				: Fdemo_mapRewardSourceProjectionResult();
			bAllSucceeded &= Plan.IsSuccess() && !Plan.PlannedStacks.IsEmpty();
		}
		Check(TEXT("Representative generation plans succeed"), bAllSucceeded);
		break;
	}
	case 9:
	{
		const auto* Boss = First(Edemo_mapM01RewardSourceClass::Boss);
		const auto Plan = Fdemo_mapRewardSourceProjectionPlanner::Plan(
			Fdemo_mapM01RewardDistribution::BuildProjection(*Boss), Run(2));
		Check(TEXT("Boss has legal equipment"), Plan.IsSuccess()
			&& Plan.PlannedStacks.ContainsByPredicate([](const auto& Stack)
			{
				return Stack.Section == Edemo_mapRuntimeContainerSection::Equipment;
			}));
		break;
	}
	case 10:
	{
		const auto* Slot = First(Edemo_mapM01RewardSourceClass::HighValue);
		const auto Projection = Fdemo_mapM01RewardDistribution::BuildProjection(*Slot);
		const auto A = Fdemo_mapRewardSourceProjectionPlanner::Plan(Projection, Run(3));
		const auto B = Fdemo_mapRewardSourceProjectionPlanner::Plan(Projection, Run(4));
		Fdemo_mapRewardGenerationSession Ledger;
		Check(TEXT("New Run changes seed and ledger commits once per Run"),
			A.IsSuccess() && B.IsSuccess() && A.Trace.EffectiveSeed != B.Trace.EffectiveSeed
			&& Ledger.Commit(Run(3), Slot->StableSourceRoleId)
			&& !Ledger.Commit(Run(3), Slot->StableSourceRoleId)
			&& Ledger.Commit(Run(4), Slot->StableSourceRoleId));
		break;
	}
	case 11:
		Check(TEXT("M01 runtime projections prohibit fallback"),
			Algo::AllOf(Slots, [](const auto& Slot)
			{
				return !Fdemo_mapM01RewardDistribution::BuildProjection(Slot).
					bAllowFixedFallbackOnFailure;
			}));
		break;
	}
	return true;
}

#endif
