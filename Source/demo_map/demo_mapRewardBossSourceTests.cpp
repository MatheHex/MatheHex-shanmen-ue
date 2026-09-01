#include "demo_mapRewardSourceProjection.h"
#include "demo_mapRewardProjectionTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardGenerator.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardRareExtreme.h"
#include "demo_mapSearchContainerPresenter.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardBossSourceTests,
	"demo_map.RewardBossSource",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

void Fdemo_mapRewardBossSourceTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	static const TCHAR* Names[] = {
		TEXT("01.RegistryValid"),
		TEXT("02.ProjectionExists"),
		TEXT("03.ProjectionIdExact"),
		TEXT("04.SourceRoleExact"),
		TEXT("05.HeavyEncounterExact"),
		TEXT("06.BudgetProfileExact"),
		TEXT("07.BaseSourceValue12000"),
		TEXT("08.P1ProfileResolves"),
		TEXT("09.P1ProfileBase12000"),
		TEXT("10.JackpotPolicyDefault"),
		TEXT("11.RarePolicyDefault"),
		TEXT("12.AffixPolicyDefault"),
		TEXT("13.SourceBossTag"),
		TEXT("14.SourceCorpseTag"),
		TEXT("15.ValueHighTag"),
		TEXT("16.GeneratedTag"),
		TEXT("17.SemanticTagCount"),
		TEXT("18.FallbackDisabled"),
		TEXT("19.FallbackTableExplicit"),
		TEXT("20.FallbackTableResolves"),
		TEXT("21.DisplayLabelExact"),
		TEXT("22.MinStacksThree"),
		TEXT("23.MaxStacksSix"),
		TEXT("24.RequiredEquipmentOne"),
		TEXT("25.SectionCountThree"),
		TEXT("26.SectionWeightsExact"),
		TEXT("27.TotalCapacitySix"),
		TEXT("28.EquipmentCapacityThree"),
		TEXT("29.BackpackCapacityOne"),
		TEXT("30.BodyCapacityTwo"),
		TEXT("31.AllSectionsRequired"),
		TEXT("32.EquipmentSectionIdentity"),
		TEXT("33.BackpackSectionIdentity"),
		TEXT("34.BodySectionIdentity"),
		TEXT("35.EquipmentPoolNonEmpty"),
		TEXT("36.EquipmentPoolHasWeapon"),
		TEXT("37.EquipmentPoolHasRobe"),
		TEXT("38.EquipmentPoolHasAccessory"),
		TEXT("39.EquipmentPoolExcludesBackpack"),
		TEXT("40.PlanSucceeds"),
		TEXT("41.PlanNoFallback"),
		TEXT("42.PlanProjectionIdentity"),
		TEXT("43.PlanRoleIdentity"),
		TEXT("44.PlanBudgetPositive"),
		TEXT("45.PlanAtLeastThreeStacks"),
		TEXT("46.PlanAtMostSixStacks"),
		TEXT("47.PlanHasEquipment"),
		TEXT("48.PlanEquipmentNotBackpack"),
		TEXT("49.AllStacksHaveBossRole"),
		TEXT("50.AllDefinitionsResolve"),
		TEXT("51.AllStackValuesPositive"),
		TEXT("52.AllSlotIndexesValid"),
		TEXT("53.SeedCountMatchesPlan"),
		TEXT("54.SeedRolePreserved"),
		TEXT("55.DeterministicPlan"),
		TEXT("56.NewRunChangesSeed"),
		TEXT("57.NaturalRunFoundBounded"),
		TEXT("58.NaturalRunJackpotMiss"),
		TEXT("59.NaturalRunRareMiss"),
		TEXT("60.NaturalRunHasAffixedEquipment"),
		TEXT("61.GenerationLedgerOnce"),
		TEXT("62.GenerationLedgerNewRun"),
		TEXT("63.BossHeaderVisible"),
		TEXT("64.LegacyHeavyProjectionPreserved")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		OutBeautifiedNames.Add(Names[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

namespace
{
	const Fdemo_mapRewardSourceProjection* BossProjection()
	{
		return Fdemo_mapRewardSourceProjectionRegistry::
			FindBossPrototype();
	}

	const Fdemo_mapRewardSourceProjection* BoundBossProjection()
	{
		static const Fdemo_mapRewardSourceProjection Projection = []()
		{
			const Fdemo_mapRewardSourceProjection* Prototype =
				BossProjection();
			return Prototype
				? demo_mapRewardProjectionTestSupport::
					BindPrototype(*Prototype)
				: Fdemo_mapRewardSourceProjection();
		}();
		return Projection.IsValid() ? &Projection : nullptr;
	}

	FGuid BossCandidateRunId(int32 Attempt)
	{
		return FGuid(
			0x50370000u,
			0x424F5353u,
			0x52455744u,
			static_cast<uint32>(Attempt + 1));
	}

	bool IsNonBackpackEquipment(FName DefinitionId)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		return Definition
			&& (Definition->CategoryId
					== Fdemo_mapItemIds::WeaponCategory
				|| Definition->CategoryId
					== Fdemo_mapItemIds::ArmorCategory
				|| Definition->CategoryId
					== Fdemo_mapItemIds::AccessoryCategory);
	}

	bool FindNaturalBossRun(
		FGuid& OutRunId,
		Fdemo_mapRewardSourceProjectionResult& OutPlan)
	{
		static bool bSearched = false;
		static FGuid CachedRunId;
		static Fdemo_mapRewardSourceProjectionResult CachedPlan;
		if (!bSearched)
		{
			bSearched = true;
			const auto* Projection = BoundBossProjection();
			for (int32 Attempt = 0;
				Projection && Attempt < 500000;
				++Attempt)
			{
				const FGuid Candidate =
					BossCandidateRunId(Attempt);
				const auto Plan =
					Fdemo_mapRewardSourceProjectionPlanner::Plan(
						*Projection,
						Candidate);
				const bool bAffixedEquipment =
					Plan.PlannedStacks.ContainsByPredicate(
						[](const auto& Stack)
						{
							const auto* Definition =
								Fdemo_mapItemDefinitions::Find(
									Stack.DefinitionId);
							return Definition
								&& Definition->CategoryId
									== Fdemo_mapItemIds::
										WeaponCategory
								&& !Stack.AffixSet.Affixes.IsEmpty();
						});
				if (Plan.IsSuccess()
					&& !Plan.Trace.bJackpotHit
					&& !Plan.Trace.bRareExtremeHit
					&& bAffixedEquipment)
				{
					CachedRunId = Candidate;
					CachedPlan = Plan;
					break;
				}
			}
		}
		OutRunId = CachedRunId;
		OutPlan = CachedPlan;
		return CachedRunId.IsValid()
			&& CachedRunId.D <= 500000u;
	}

	bool HasItemTag(
		const Fdemo_mapRewardPoolEntry& Entry,
		FName Tag)
	{
		return Entry.ItemTags.Contains(Tag);
	}
}

bool Fdemo_mapRewardBossSourceTests::RunTest(
	const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const auto* Boss = BossProjection();
	const auto* BoundBoss = BoundBossProjection();
	const FGuid RunId = BossCandidateRunId(700);
	const auto Plan = BoundBoss
		? Fdemo_mapRewardSourceProjectionPlanner::Plan(
			*BoundBoss,
			RunId)
		: Fdemo_mapRewardSourceProjectionResult();
	FGuid NaturalRunId;
	Fdemo_mapRewardSourceProjectionResult NaturalPlan;
	const bool bNaturalFound =
		FindNaturalBossRun(NaturalRunId, NaturalPlan);
	const auto* EquipmentSection =
		Boss && Boss->Sections.IsValidIndex(0)
			? &Boss->Sections[0] : nullptr;
	const TArray<Fdemo_mapRewardPoolEntry> EquipmentPool =
		Boss && EquipmentSection
			? Fdemo_mapRewardSourceProjectionRegistry::
				BuildSectionPool(*Boss, *EquipmentSection)
			: TArray<Fdemo_mapRewardPoolEntry>();
	const auto Check = [this](const TCHAR* Label, bool bValue)
	{
		TestTrue(Label, bValue);
	};

	switch (Case)
	{
	case 0:
	{
		FString Error;
		Check(TEXT("Registry validates"),
			Fdemo_mapRewardSourceProjectionRegistry::Validate(
				&Error));
		break;
	}
	case 1:
		Check(TEXT("Boss projection exists"), Boss != nullptr);
		break;
	case 2:
		Check(TEXT("Projection id exact"), Boss
			&& Boss->ProjectionId
				== Fdemo_mapRewardProjectionIds::
					CorpseBossPrototype);
		break;
	case 3:
		Check(TEXT("Role exact"), Boss
			&& Boss->StableSourceRoleId
				== Fdemo_mapRewardSourceRoleIds::
					BossPrototype);
		break;
	case 4:
		Check(TEXT("Heavy encounter exact"), Boss
			&& Boss->EncounterId
				== Fdemo_mapEnemyEncounterIds::
					MainMeleeHeavy);
		break;
	case 5:
		Check(TEXT("Boss profile exact"), Boss
			&& Boss->BudgetProfileId
				== Fdemo_mapRewardBudgetProfileIds::Boss);
		break;
	case 6:
		Check(TEXT("Base value explicit"), Boss
			&& Boss->BaseSourceValue == 12000);
		break;
	case 7:
		Check(TEXT("P1 profile resolves"), Boss
			&& Fdemo_mapRewardGenerationRegistry::
				FindBudgetProfile(Boss->BudgetProfileId));
		break;
	case 8:
	{
		const auto* Profile = Boss
			? Fdemo_mapRewardGenerationRegistry::
				FindBudgetProfile(Boss->BudgetProfileId)
			: nullptr;
		Check(TEXT("P1 profile base"), Profile
			&& Profile->BaseValue == 12000);
		break;
	}
	case 9:
		Check(TEXT("Jackpot default"), Boss
			&& Boss->JackpotPolicyId
				== Fdemo_mapRewardJackpotPolicyRegistry::
					DefaultPolicyId);
		break;
	case 10:
		Check(TEXT("Rare default"), Boss
			&& Boss->RareExtremePolicyId
				== Fdemo_mapRewardRareExtremePolicyRegistry::
					DefaultPolicyId);
		break;
	case 11:
		Check(TEXT("Affix default"), Boss
			&& Boss->AffixPolicyId
				== Fdemo_mapRewardAffixPolicyRegistry::
					DefaultPolicyId);
		break;
	case 12:
	case 13:
	case 14:
	case 15:
	{
		const FName Tags[] = {
			Fdemo_mapRewardProjectionTagIds::SourceBoss,
			Fdemo_mapRewardProjectionTagIds::SourceCorpse,
			Fdemo_mapRewardProjectionTagIds::ValueHigh,
			Fdemo_mapRewardProjectionTagIds::Generated
		};
		Check(TEXT("Semantic tag present"), Boss
			&& Boss->SourceTags.Contains(Tags[Case - 12]));
		break;
	}
	case 16:
		Check(TEXT("Four exact semantic tags"), Boss
			&& Boss->SourceTags.Num() == 4);
		break;
	case 17:
		Check(TEXT("Fallback disabled"), Boss
			&& !Boss->bAllowFixedFallbackOnFailure);
		break;
	case 18:
		Check(TEXT("Fallback table explicit"), Boss
			&& Boss->FixedFallbackTableId
				== Fdemo_mapFixedLootTableIds::
					CorpseMainMeleeHeavy);
		break;
	case 19:
		Check(TEXT("Fallback table resolves"), Boss
			&& Fdemo_mapFixedLootTableRegistry::Find(
				Boss->FixedFallbackTableId));
		break;
	case 20:
		Check(TEXT("Label exact"), Boss
			&& Boss->SourceDisplayLabel
				== TEXT("BOSS REWARD"));
		break;
	case 21:
		Check(TEXT("Minimum three"), Boss
			&& Boss->MinGeneratedStacks == 3);
		break;
	case 22:
		Check(TEXT("Maximum six"), Boss
			&& Boss->MaxGeneratedStacks == 6);
		break;
	case 23:
		Check(TEXT("Equipment required"), Boss
			&& Boss->RequiredEquipmentCount == 1);
		break;
	case 24:
		Check(TEXT("Three sections"), Boss
			&& Boss->Sections.Num() == 3);
		break;
	case 25:
		Check(TEXT("Weights exact"), Boss
			&& Boss->Sections.Num() == 3
			&& Boss->Sections[0].BudgetWeightBps == 4000
			&& Boss->Sections[1].BudgetWeightBps == 3500
			&& Boss->Sections[2].BudgetWeightBps == 2500);
		break;
	case 26:
		Check(TEXT("Total capacity six"), Boss
			&& Boss->Sections.Num() == 3
			&& Boss->Sections[0].Capacity
				+ Boss->Sections[1].Capacity
				+ Boss->Sections[2].Capacity == 6);
		break;
	case 27:
	case 28:
	case 29:
		Check(TEXT("Section capacity exact"), Boss
			&& Boss->Sections.IsValidIndex(Case - 27)
			&& Boss->Sections[Case - 27].Capacity
				== (Case == 27 ? 3 : Case == 28 ? 1 : 2));
		break;
	case 30:
		Check(TEXT("All sections required"), Boss
			&& Algo::AllOf(
				Boss->Sections,
				[](const auto& Section)
				{
					return Section.bRequiredNonEmpty;
				}));
		break;
	case 31:
	case 32:
	case 33:
	{
		const Edemo_mapRuntimeContainerSection Expected[] = {
			Edemo_mapRuntimeContainerSection::Equipment,
			Edemo_mapRuntimeContainerSection::Backpack,
			Edemo_mapRuntimeContainerSection::Body
		};
		Check(TEXT("Runtime section exact"), Boss
			&& Boss->Sections.IsValidIndex(Case - 31)
			&& Boss->Sections[Case - 31].RuntimeSection
				== Expected[Case - 31]);
		break;
	}
	case 34:
		Check(TEXT("Equipment pool nonempty"),
			!EquipmentPool.IsEmpty());
		break;
	case 35:
	case 36:
	case 37:
	{
		const FName Tags[] = {
			Fdemo_mapRewardTagIds::ItemEquipmentWeapon,
			Fdemo_mapRewardTagIds::ItemEquipmentRobe,
			Fdemo_mapRewardTagIds::ItemEquipmentAccessory
		};
		Check(TEXT("Equipment category available"),
			EquipmentPool.ContainsByPredicate(
				[&](const auto& Entry)
				{
					return HasItemTag(
						Entry,
						Tags[Case - 35]);
				}));
		break;
	}
	case 38:
		Check(TEXT("Backpack excluded"),
			!EquipmentPool.ContainsByPredicate(
				[](const auto& Entry)
				{
					return HasItemTag(
						Entry,
						Fdemo_mapRewardTagIds::
							ItemEquipmentBackpack);
				}));
		break;
	case 39:
		Check(TEXT("Plan succeeds"), Plan.IsSuccess());
		break;
	case 40:
		Check(TEXT("Plan no fallback"), Plan.IsSuccess()
			&& !Plan.Trace.bFallbackUsed);
		break;
	case 41:
		Check(TEXT("Trace projection"), Boss
			&& Plan.Trace.ProjectionId
				== Boss->ProjectionId);
		break;
	case 42:
		Check(TEXT("Trace role"), Boss
			&& Plan.Trace.StableSourceRoleId
				== Boss->StableSourceRoleId);
		break;
	case 43:
		Check(TEXT("Budget positive"),
			Plan.Trace.RandomizedBudget > 0);
		break;
	case 44:
		Check(TEXT("At least three"),
			Plan.PlannedStacks.Num() >= 3);
		break;
	case 45:
		Check(TEXT("At most six"),
			Plan.PlannedStacks.Num() <= 6);
		break;
	case 46:
		Check(TEXT("Has Equipment"),
			Plan.PlannedStacks.ContainsByPredicate(
				[](const auto& Stack)
				{
					return Stack.Section
						== Edemo_mapRuntimeContainerSection::
							Equipment;
				}));
		break;
	case 47:
		Check(TEXT("Equipment never Backpack"),
			!Plan.PlannedStacks.ContainsByPredicate(
				[](const auto& Stack)
				{
					return Stack.Section
							== Edemo_mapRuntimeContainerSection::
								Equipment
						&& !IsNonBackpackEquipment(
							Stack.DefinitionId);
				}));
		break;
	case 48:
		Check(TEXT("Boss role on every stack"), Boss
			&& Algo::AllOf(
				Plan.PlannedStacks,
				[Boss](const auto& Stack)
				{
					return Stack.RewardSourceRoleId
						== Boss->StableSourceRoleId;
				}));
		break;
	case 49:
		Check(TEXT("All definitions resolve"),
			Algo::AllOf(
				Plan.PlannedStacks,
				[](const auto& Stack)
				{
					return Fdemo_mapItemDefinitions::Find(
						Stack.DefinitionId) != nullptr;
				}));
		break;
	case 50:
		Check(TEXT("All values positive"),
			Algo::AllOf(
				Plan.PlannedStacks,
				[](const auto& Stack)
				{
					return Stack.StackCount > 0
						&& Stack.UnitValue > 0
						&& Stack.TotalValue > 0;
				}));
		break;
	case 51:
		Check(TEXT("All slots valid"),
			Algo::AllOf(
				Plan.PlannedStacks,
				[](const auto& Stack)
				{
					return Stack.SlotIndex >= 0;
				}));
		break;
	case 52:
		Check(TEXT("Seed count"),
			Fdemo_mapRewardSourceProjectionPlanner::
				BuildContainerSeed(Plan).Num()
				== Plan.PlannedStacks.Num());
		break;
	case 53:
		Check(TEXT("Seed role"),
			Boss && Algo::AllOf(
				Fdemo_mapRewardSourceProjectionPlanner::
					BuildContainerSeed(Plan),
				[Boss](const auto& Entry)
				{
					return Entry.RewardSourceRoleId
						== Boss->StableSourceRoleId;
				}));
		break;
	case 54:
	{
		const auto Replay = BoundBoss
			? Fdemo_mapRewardSourceProjectionPlanner::Plan(
				*BoundBoss,
				RunId)
			: Fdemo_mapRewardSourceProjectionResult();
		Check(TEXT("Deterministic plan"),
			Replay.PlannedStacks == Plan.PlannedStacks
				&& Replay.Trace.EffectiveSeed
					== Plan.Trace.EffectiveSeed);
		break;
	}
	case 55:
	{
		const auto Other = BoundBoss
			? Fdemo_mapRewardSourceProjectionPlanner::Plan(
				*BoundBoss,
				BossCandidateRunId(701))
			: Fdemo_mapRewardSourceProjectionResult();
		Check(TEXT("New Run changes seed"),
			Other.Trace.EffectiveSeed
				!= Plan.Trace.EffectiveSeed);
		break;
	}
	case 56:
		Check(TEXT("Natural Run found"), bNaturalFound);
		break;
	case 57:
		Check(TEXT("Natural Jackpot miss"),
			bNaturalFound
				&& !NaturalPlan.Trace.bJackpotHit);
		break;
	case 58:
		Check(TEXT("Natural Rare miss"),
			bNaturalFound
				&& !NaturalPlan.Trace.bRareExtremeHit);
		break;
	case 59:
		Check(TEXT("Natural Affix Equipment"),
			bNaturalFound
				&& NaturalPlan.PlannedStacks.
					ContainsByPredicate(
						[](const auto& Stack)
						{
							return IsNonBackpackEquipment(
									Stack.DefinitionId)
								&& !Stack.AffixSet.Affixes.IsEmpty();
						}));
		break;
	case 60:
	{
		Fdemo_mapRewardGenerationSession Ledger;
		Check(TEXT("Once per source"), Boss
			&& Ledger.Commit(
				RunId,
				Boss->StableSourceRoleId)
			&& !Ledger.Commit(
				RunId,
				Boss->StableSourceRoleId));
		break;
	}
	case 61:
	{
		Fdemo_mapRewardGenerationSession Ledger;
		Check(TEXT("New Run accepted"), Boss
			&& Ledger.Commit(
				RunId,
				Boss->StableSourceRoleId)
			&& Ledger.Commit(
				BossCandidateRunId(701),
				Boss->StableSourceRoleId));
		break;
	}
	case 62:
	{
		Fdemo_mapRuntimeContainerSnapshot Snapshot;
		Snapshot.Kind =
			Edemo_mapRuntimeContainerKind::Corpse;
		Snapshot.SourceDisplayLabel =
			TEXT("BOSS REWARD");
		Check(TEXT("Boss header visible"),
			Fdemo_mapSearchContainerPresenter::Build(
				Snapshot).Header == TEXT("BOSS REWARD"));
		break;
	}
	case 63:
	{
		const auto* Legacy =
			Fdemo_mapRewardSourceProjectionRegistry::Find(
				Fdemo_mapRewardProjectionIds::
					CorpseMainMeleeHeavy);
		Check(TEXT("Legacy Heavy preserved"), Legacy
			&& Legacy->StableSourceRoleId
				== Fdemo_mapEnemyEncounterIds::
					MainMeleeHeavy
			&& Legacy->BudgetProfileId
				== Fdemo_mapRewardBudgetProfileIds::
					EnemyStandard);
		break;
	}
	default:
		AddError(TEXT("Unknown Boss source case."));
		break;
	}
	return true;
}

#endif
