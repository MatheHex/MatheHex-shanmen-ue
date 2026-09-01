#include "Misc/AutomationTest.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapSearchContainerTypes.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	const Fdemo_mapEnemyEncounterSpawnRecord* Encounter(FName Id)
	{
		return Fdemo_mapEnemyEncounterConfig::Find(Id);
	}

	const Fdemo_mapFixedLootTableDefinition* Loot(FName Id)
	{
		return Fdemo_mapFixedLootTableRegistry::Find(Id);
	}

	bool IsUniqueSeed(const Fdemo_mapFixedLootTableDefinition& Table)
	{
		TSet<uint64> Slots;
		for (const Fdemo_mapRuntimeContainerSeedEntry& Entry : Table.Entries)
		{
			const uint64 Key =
				(static_cast<uint64>(Entry.Section) << 32)
				| static_cast<uint32>(Entry.SlotIndex);
			if (Slots.Contains(Key)
				|| !Fdemo_mapItemDefinitions::Find(Entry.DefinitionId))
			{
				return false;
			}
			Slots.Add(Key);
		}
		return true;
	}

	bool RunP7Case(FAutomationTestBase& Test, int32 Case)
	{
		const TArray<Fdemo_mapEnemyEncounterSpawnRecord>& Encounters =
			Fdemo_mapEnemyEncounterConfig::GetSpawnRecords();
		const TArray<Fdemo_mapFixedLootTableDefinition>& Tables =
			Fdemo_mapFixedLootTableRegistry::GetAll();
		switch (Case)
		{
		case 1:
		{
			FString Error;
			Test.TestTrue(TEXT("Encounter authority validates"), Fdemo_mapEnemyEncounterConfig::Validate(&Error));
			break;
		}
		case 2:
			Test.TestEqual(TEXT("Exactly five encounter records"), Encounters.Num(), 5);
			break;
		case 3:
			Test.TestEqual(
				TEXT("Exactly three main-route enemies"),
				Encounters.FilterByPredicate([](const auto& E)
				{
					return E.Identity.RouteId == Fdemo_mapEnemyEncounterIds::MainRoute;
				}).Num(),
				3);
			break;
		case 4:
			Test.TestEqual(
				TEXT("Exactly two side-route enemies"),
				Encounters.FilterByPredicate([](const auto& E)
				{
					return E.Identity.RouteId != Fdemo_mapEnemyEncounterIds::MainRoute;
				}).Num(),
				2);
			break;
		case 5:
		{
			TSet<FName> Ids;
			for (const auto& E : Encounters) Ids.Add(E.Identity.EncounterId);
			Test.TestEqual(TEXT("Encounter ids unique"), Ids.Num(), 5);
			break;
		}
		case 6:
		{
			TSet<FName> Ids;
			for (const auto& E : Encounters) Ids.Add(E.Identity.SpawnMarkerId);
			Test.TestEqual(TEXT("Spawn marker ids unique"), Ids.Num(), 5);
			break;
		}
		case 7:
		{
			TSet<FName> Ids;
			for (const auto& E : Encounters) Ids.Add(E.Identity.LootTableId);
			Test.TestEqual(TEXT("Enemy table bindings unique"), Ids.Num(), 5);
			break;
		}
		case 8:
			Test.TestTrue(TEXT("All records are complete"), Encounters.ContainsByPredicate([](const auto& E){ return !E.IsValid(); }) == false);
			break;
		case 9:
			Test.TestTrue(TEXT("Main melee identity"), Encounter(Fdemo_mapEnemyEncounterIds::MainMeleeStandard) != nullptr);
			break;
		case 10:
			Test.TestTrue(TEXT("Main heavy identity"), Encounter(Fdemo_mapEnemyEncounterIds::MainMeleeHeavy) != nullptr);
			break;
		case 11:
			Test.TestTrue(TEXT("Main ranged identity"), Encounter(Fdemo_mapEnemyEncounterIds::MainRangedStandard) != nullptr);
			break;
		case 12:
			Test.TestTrue(TEXT("Side melee identity"), Encounter(Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced) != nullptr);
			break;
		case 13:
			Test.TestTrue(TEXT("Side ranged identity"), Encounter(Fdemo_mapEnemyEncounterIds::SideRangedEnhanced) != nullptr);
			break;
		case 14:
		{
			const auto* E = Encounter(Fdemo_mapEnemyEncounterIds::MainMeleeStandard);
			Test.TestTrue(TEXT("Standard melee tuning"), E && E->Tuning.MaxHealth == 3 && E->Tuning.MovementSpeed == 260.0f && E->Tuning.AttackDamage == 1.0f && E->Tuning.AttackCooldown == 1.20f);
			break;
		}
		case 15:
		{
			const auto* E = Encounter(Fdemo_mapEnemyEncounterIds::MainMeleeHeavy);
			Test.TestTrue(TEXT("Heavy tuning"), E && E->Tuning.MaxHealth == 5 && E->Tuning.MovementSpeed == 180.0f && E->Tuning.AttackWindup == 0.85f && E->Tuning.AttackCooldown == 2.40f);
			break;
		}
		case 16:
		{
			const auto* E = Encounter(Fdemo_mapEnemyEncounterIds::MainRangedStandard);
			Test.TestTrue(TEXT("Standard ranged tuning"), E && E->Tuning.MaxHealth == 3 && E->Tuning.MovementSpeed == 240.0f && E->Tuning.AttackWindup == 0.40f && E->Tuning.AttackCooldown == 1.60f);
			break;
		}
		case 17:
		{
			const auto* E = Encounter(Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced);
			Test.TestTrue(TEXT("Enhanced melee tuning"), E && E->bEnhanced && E->Tuning.MaxHealth == 6 && E->Tuning.MovementSpeed == 300.0f && E->Tuning.AttackDamage == 2.0f && E->Tuning.AttackCooldown == 1.0f);
			break;
		}
		case 18:
		{
			const auto* E = Encounter(Fdemo_mapEnemyEncounterIds::SideRangedEnhanced);
			Test.TestTrue(TEXT("Enhanced ranged tuning"), E && E->bEnhanced && E->Tuning.MaxHealth == 5 && E->Tuning.MovementSpeed == 280.0f && E->Tuning.AttackDamage == 2.0f && E->Tuning.AttackWindup == 0.32f && E->Tuning.AttackCooldown == 1.40f);
			break;
		}
		case 19:
			Test.TestTrue(TEXT("P6 melee profile resolves"), Fdemo_mapEnemySkillPrototypeConfig::FindProfile(Fdemo_mapEnemySkillProfileIds::StandardMeleeDash) != nullptr);
			break;
		case 20:
			Test.TestTrue(TEXT("P6 ranged profile resolves"), Fdemo_mapEnemySkillPrototypeConfig::FindProfile(Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep) != nullptr);
			break;
		case 21:
			Test.TestTrue(TEXT("P7 melee profile resolves"), Fdemo_mapEnemySkillPrototypeConfig::FindProfile(Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash) != nullptr);
			break;
		case 22:
			Test.TestTrue(TEXT("P7 ranged profile resolves"), Fdemo_mapEnemySkillPrototypeConfig::FindProfile(Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep) != nullptr);
			break;
		case 23:
			Test.TestTrue(TEXT("Unknown profile rejects without fallback"), Fdemo_mapEnemySkillPrototypeConfig::FindProfile(TEXT("P7.Invalid.Profile")) == nullptr);
			break;
		case 24:
		{
			const auto& C = Fdemo_mapEnemySkillPrototypeConfig::Get();
			Test.TestTrue(TEXT("P6 profile values frozen"), C.MeleeDash.DisplacementDistance == 440.0f && C.RangedBackstepShot.DisplacementDistance == 320.0f);
			break;
		}
		case 25:
		{
			const auto& D = Fdemo_mapEnemySkillPrototypeConfig::Get().EnhancedMeleeDash;
			Test.TestTrue(TEXT("Enhanced melee skill values"), D.TriggerMinDistance == 240.0f && D.TriggerMaxDistance == 720.0f && D.WindupDuration == 0.18f && D.DisplacementDistance == 520.0f && D.DisplacementDuration == 0.30f && D.MinimumResolvedDistance == 140.0f && D.RecoveryDuration == 0.24f && D.CooldownDuration == 2.20f);
			break;
		}
		case 26:
		{
			const auto& D = Fdemo_mapEnemySkillPrototypeConfig::Get().EnhancedRangedBackstepShot;
			Test.TestTrue(TEXT("Enhanced ranged skill values"), D.TriggerMaxDistance == 560.0f && D.WindupDuration == 0.14f && D.DisplacementDistance == 380.0f && D.DisplacementDuration == 0.26f && D.MinimumResolvedDistance == 140.0f && D.RecoveryDuration == 0.18f && D.CooldownDuration == 2.0f);
			break;
		}
		case 27:
		{
			const auto& D = Fdemo_mapEnemySkillPrototypeConfig::Get().EnhancedMeleeDash;
			Test.TestTrue(TEXT("Dash locks and stops on first hit"), D.bLockDirectionAtEndOfWindup && D.bAppliesContactDamage && D.bStopsOnFirstLegalHit);
			break;
		}
		case 28:
		{
			const auto& D = Fdemo_mapEnemySkillPrototypeConfig::Get().EnhancedRangedBackstepShot;
			Test.TestTrue(TEXT("Ranged resolution revalidates target and LOS"), D.CanResolveRangedFire(140.0f, true, true) && !D.CanResolveRangedFire(140.0f, false, true) && !D.CanResolveRangedFire(140.0f, true, false));
			break;
		}
		case 29:
		{
			FString Error;
			Test.TestTrue(TEXT("Fixed loot registry validates"), Fdemo_mapFixedLootTableRegistry::Validate(&Error));
			break;
		}
		case 30:
			Test.TestEqual(TEXT("Exactly eight fixed tables"), Tables.Num(), 8);
			break;
		case 31:
			Test.TestEqual(TEXT("Exactly five corpse tables"), Tables.FilterByPredicate([](const auto& T){ return T.Kind == Edemo_mapRuntimeContainerKind::Corpse; }).Num(), 5);
			break;
		case 32:
			Test.TestEqual(TEXT("Exactly three chest tables"), Tables.FilterByPredicate([](const auto& T){ return T.Kind == Edemo_mapRuntimeContainerKind::Chest; }).Num(), 3);
			break;
		case 33:
			Test.TestTrue(TEXT("Main melee fixed total"), Loot(Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard) && Loot(Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard)->TotalPrototypeValue == 180);
			break;
		case 34:
			Test.TestTrue(TEXT("Main heavy fixed total"), Loot(Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy) && Loot(Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy)->TotalPrototypeValue == 170);
			break;
		case 35:
			Test.TestTrue(TEXT("Main ranged fixed total"), Loot(Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard) && Loot(Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard)->TotalPrototypeValue == 155);
			break;
		case 36:
			Test.TestTrue(TEXT("Side melee fixed total"), Loot(Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced) && Loot(Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced)->TotalPrototypeValue == 970);
			break;
		case 37:
			Test.TestTrue(TEXT("Side ranged fixed total"), Loot(Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced) && Loot(Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced)->TotalPrototypeValue == 950);
			break;
		case 38:
			Test.TestTrue(TEXT("Main chest A fixed total"), Loot(Fdemo_mapFixedLootTableIds::ChestMainA) && Loot(Fdemo_mapFixedLootTableIds::ChestMainA)->TotalPrototypeValue == 35);
			break;
		case 39:
			Test.TestTrue(TEXT("Main chest B fixed total"), Loot(Fdemo_mapFixedLootTableIds::ChestMainB) && Loot(Fdemo_mapFixedLootTableIds::ChestMainB)->TotalPrototypeValue == 45);
			break;
		case 40:
			Test.TestTrue(TEXT("Side chest fixed total"), Loot(Fdemo_mapFixedLootTableIds::ChestSideA) && Loot(Fdemo_mapFixedLootTableIds::ChestSideA)->TotalPrototypeValue == 150);
			break;
		case 41:
			Test.TestTrue(TEXT("P4 search times preserved"), Fdemo_mapSearchContainerPrototypeConfig::CorpseEquipmentSearchSeconds == 0.0f && Fdemo_mapSearchContainerPrototypeConfig::CorpseBackpackEntrySearchSeconds == 1.0f && Fdemo_mapSearchContainerPrototypeConfig::CorpseBodyEntrySearchSeconds == 1.5f);
			break;
		case 42:
			Test.TestEqual(
				TEXT("Corpse equipment capacity follows the canonical equipment roles"),
				Fdemo_mapSearchContainerPrototypeConfig::GetSectionCapacity(
					Edemo_mapRuntimeContainerKind::Corpse,
					Edemo_mapRuntimeContainerSection::Equipment),
				Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num());
			Test.TestEqual(
				TEXT("Enhanced corpse body capacity represented"),
				Fdemo_mapSearchContainerPrototypeConfig::GetSectionCapacity(
					Edemo_mapRuntimeContainerKind::Corpse,
					Edemo_mapRuntimeContainerSection::Body),
				2);
			break;
		case 43:
			Test.TestTrue(TEXT("All seed slots and definitions valid"), !Tables.ContainsByPredicate([](const auto& T){ return !IsUniqueSeed(T); }));
			break;
		case 44:
		{
			const int32 HighestMain = FMath::Max3(
				Loot(Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard)->TotalPrototypeValue,
				Loot(Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy)->TotalPrototypeValue,
				Loot(Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard)->TotalPrototypeValue);
			Test.TestTrue(TEXT("Side rewards exceed main rewards"), Loot(Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced)->TotalPrototypeValue > HighestMain && Loot(Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced)->TotalPrototypeValue > HighestMain && Loot(Fdemo_mapFixedLootTableIds::ChestSideA)->TotalPrototypeValue > Loot(Fdemo_mapFixedLootTableIds::ChestMainA)->TotalPrototypeValue && Loot(Fdemo_mapFixedLootTableIds::ChestSideA)->TotalPrototypeValue > Loot(Fdemo_mapFixedLootTableIds::ChestMainB)->TotalPrototypeValue);
			break;
		}
		default:
			return false;
		}
		return true;
	}
}

#define P7_TEST(TypeName, Number, DisplayName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		TypeName, \
		"demo_map.EnemyRouteLoot." DisplayName, \
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool TypeName::RunTest(const FString&) { return RunP7Case(*this, Number); }

P7_TEST(FEnemyRouteLoot01, 1, "01.ConfigValid")
P7_TEST(FEnemyRouteLoot02, 2, "02.ExactlyFiveEnemies")
P7_TEST(FEnemyRouteLoot03, 3, "03.MainRouteCount")
P7_TEST(FEnemyRouteLoot04, 4, "04.SideRouteCount")
P7_TEST(FEnemyRouteLoot05, 5, "05.UniqueEncounterIds")
P7_TEST(FEnemyRouteLoot06, 6, "06.UniqueSpawnMarkerIds")
P7_TEST(FEnemyRouteLoot07, 7, "07.UniqueEnemyLootBindings")
P7_TEST(FEnemyRouteLoot08, 8, "08.CompleteSpawnRecords")
P7_TEST(FEnemyRouteLoot09, 9, "09.MainMeleeIdentity")
P7_TEST(FEnemyRouteLoot10, 10, "10.MainHeavyIdentity")
P7_TEST(FEnemyRouteLoot11, 11, "11.MainRangedIdentity")
P7_TEST(FEnemyRouteLoot12, 12, "12.SideMeleeIdentity")
P7_TEST(FEnemyRouteLoot13, 13, "13.SideRangedIdentity")
P7_TEST(FEnemyRouteLoot14, 14, "14.StandardMeleeTuning")
P7_TEST(FEnemyRouteLoot15, 15, "15.HeavyTuning")
P7_TEST(FEnemyRouteLoot16, 16, "16.StandardRangedTuning")
P7_TEST(FEnemyRouteLoot17, 17, "17.EnhancedMeleeTuning")
P7_TEST(FEnemyRouteLoot18, 18, "18.EnhancedRangedTuning")
P7_TEST(FEnemyRouteLoot19, 19, "19.StandardMeleeProfile")
P7_TEST(FEnemyRouteLoot20, 20, "20.StandardRangedProfile")
P7_TEST(FEnemyRouteLoot21, 21, "21.EnhancedMeleeProfile")
P7_TEST(FEnemyRouteLoot22, 22, "22.EnhancedRangedProfile")
P7_TEST(FEnemyRouteLoot23, 23, "23.InvalidProfileReject")
P7_TEST(FEnemyRouteLoot24, 24, "24.P6ValuesFrozen")
P7_TEST(FEnemyRouteLoot25, 25, "25.EnhancedMeleeSkill")
P7_TEST(FEnemyRouteLoot26, 26, "26.EnhancedRangedSkill")
P7_TEST(FEnemyRouteLoot27, 27, "27.DashFirstHitLock")
P7_TEST(FEnemyRouteLoot28, 28, "28.RangedResolveRevalidation")
P7_TEST(FEnemyRouteLoot29, 29, "29.FixedRegistryValid")
P7_TEST(FEnemyRouteLoot30, 30, "30.ExactlyEightTables")
P7_TEST(FEnemyRouteLoot31, 31, "31.ExactlyFiveCorpseTables")
P7_TEST(FEnemyRouteLoot32, 32, "32.ExactlyThreeChestTables")
P7_TEST(FEnemyRouteLoot33, 33, "33.MainMeleeLootValue")
P7_TEST(FEnemyRouteLoot34, 34, "34.MainHeavyLootValue")
P7_TEST(FEnemyRouteLoot35, 35, "35.MainRangedLootValue")
P7_TEST(FEnemyRouteLoot36, 36, "36.SideMeleeLootValue")
P7_TEST(FEnemyRouteLoot37, 37, "37.SideRangedLootValue")
P7_TEST(FEnemyRouteLoot38, 38, "38.MainChestAValue")
P7_TEST(FEnemyRouteLoot39, 39, "39.MainChestBValue")
P7_TEST(FEnemyRouteLoot40, 40, "40.SideChestValue")
P7_TEST(FEnemyRouteLoot41, 41, "41.SearchTimesPreserved")
P7_TEST(FEnemyRouteLoot42, 42, "42.EnhancedCorpseCapacity")
P7_TEST(FEnemyRouteLoot43, 43, "43.SeedSlotsAndDefinitions")
P7_TEST(FEnemyRouteLoot44, 44, "44.SideRewardsExceedMain")

#undef P7_TEST
#endif
