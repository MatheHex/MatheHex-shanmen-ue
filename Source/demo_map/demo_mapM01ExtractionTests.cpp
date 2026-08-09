#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapM01Extraction.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	bool AddOne(Fdemo_mapItemAuthority& Authority, FName DefinitionId, FGuid& OutId)
	{
		TArray<FGuid> Added;
		const Fdemo_mapItemOperationResult Result = Authority.AddDefinition(DefinitionId, 1, &Added);
		if (!Result.bSuccess || Added.Num() != 1) return false;
		OutId = Added[0];
		return true;
	}

	bool PrepareSpatialFixture(
		Fdemo_mapItemAuthority& Authority,
		FGuid& OutBackpack,
		TArray<FGuid>& OutInventory)
	{
		if (!AddOne(Authority, Fdemo_mapItemIds::BackpackLevel1, OutBackpack)
			|| !Authority.Equip(OutBackpack, Fdemo_mapItemIds::BackpackSlot).bSuccess)
		{
			return false;
		}
		for (int32 Index = 0; Index < 7; ++Index)
		{
			FGuid Id;
			if (!AddOne(Authority, Fdemo_mapItemIds::TrainingBlade, Id)) return false;
			OutInventory.Add(Id);
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FM01ExtractionIdentityTest,
	"demo_map.M01Extraction.01.StableIdentities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FM01ExtractionIdentityTest::RunTest(const FString&)
{
	FString Error;
	TestTrue(TEXT("stable IDs validate"), Fdemo_mapM01Ids::Validate(&Error));
	TestEqual(TEXT("map identity"), Fdemo_mapM01Ids::Map, FName(TEXT("M01")));
	TestEqual(TEXT("regular exit"), Fdemo_mapM01Ids::ExitRegular, FName(TEXT("M01.Exit.Regular")));
	TestEqual(TEXT("boss exit"), Fdemo_mapM01Ids::ExitBoss, FName(TEXT("M01.Exit.Boss")));
	TestEqual(TEXT("discard exit"), Fdemo_mapM01Ids::ExitDiscardSpatial, FName(TEXT("M01.Exit.DiscardSpatial")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FM01ExtractionConditionsTest,
	"demo_map.M01Extraction.02.ConditionsBossUnlockAndRunReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FM01ExtractionConditionsTest::RunTest(const FString&)
{
	Fdemo_mapM01ExtractionAuthority Authority;
	Authority.ResetForNewRun(true);
	TestEqual(TEXT("regular initially available"), Authority.GetSnapshot(Edemo_mapM01ExitType::Regular).State, Edemo_mapM01ExtractionState::Available);
	TestEqual(TEXT("boss initially locked"), Authority.GetSnapshot(Edemo_mapM01ExitType::Boss).State, Edemo_mapM01ExtractionState::Locked);
	TestEqual(TEXT("discard locked with spatial item"), Authority.GetSnapshot(Edemo_mapM01ExitType::DiscardSpatial).State, Edemo_mapM01ExtractionState::Locked);
	TestFalse(TEXT("wrong boss ID rejected"), Authority.NotifyBossDefeated(FName(TEXT("M01.Boss.NotMain"))));
	TestTrue(TEXT("main boss unlocks once"), Authority.NotifyBossDefeated(Fdemo_mapM01Ids::MainBoss));
	TestFalse(TEXT("duplicate boss event ignored"), Authority.NotifyBossDefeated(Fdemo_mapM01Ids::MainBoss));
	Authority.SetSpatialItemEquipped(false);
	TestTrue(TEXT("discard becomes available"), Authority.GetSnapshot(Edemo_mapM01ExitType::DiscardSpatial).bConditionSatisfied);
	Authority.ResetForNewRun(false);
	TestFalse(TEXT("new run locks boss again"), Authority.IsBossUnlocked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FM01ExtractionCountdownTest,
	"demo_map.M01Extraction.03.CountdownCancelAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FM01ExtractionCountdownTest::RunTest(const FString&)
{
	Fdemo_mapM01ExtractionAuthority Authority;
	Authority.ResetForNewRun(false);
	Authority.SetInRange(Edemo_mapM01ExitType::Regular, true);
	TestTrue(TEXT("countdown starts by interaction"), Authority.BeginInteraction(Edemo_mapM01ExitType::Regular));
	Edemo_mapM01ExitType Completed = Edemo_mapM01ExitType::Boss;
	TestFalse(TEXT("not complete before three seconds"), Authority.Advance(1.25f, Completed));
	Authority.NotifyEffectiveDamage();
	TestEqual(TEXT("damage cancels"), Authority.GetSnapshot(Edemo_mapM01ExitType::Regular).CancelReason, Edemo_mapM01ExtractionCancelReason::Damaged);
	TestTrue(TEXT("cancelled countdown can restart"), Authority.BeginInteraction(Edemo_mapM01ExitType::Regular));
	Authority.SetInRange(Edemo_mapM01ExitType::Regular, false);
	TestEqual(TEXT("leaving range cancels"), Authority.GetSnapshot(Edemo_mapM01ExitType::Regular).CancelReason, Edemo_mapM01ExtractionCancelReason::LeftRange);
	Authority.SetInRange(Edemo_mapM01ExitType::Regular, true);
	TestTrue(TEXT("third attempt starts"), Authority.BeginInteraction(Edemo_mapM01ExitType::Regular));
	TestTrue(TEXT("three seconds completes"), Authority.Advance(3.0f, Completed));
	TestEqual(TEXT("regular completion token"), Completed, Edemo_mapM01ExitType::Regular);
	TestFalse(TEXT("completion emitted once"), Authority.Advance(3.0f, Completed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FM01ExtractionConditionCancelTest,
	"demo_map.M01Extraction.04.ConditionInvalidationAndTerminalCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FM01ExtractionConditionCancelTest::RunTest(const FString&)
{
	Fdemo_mapM01ExtractionAuthority Authority;
	Authority.ResetForNewRun(false);
	Authority.SetInRange(Edemo_mapM01ExitType::DiscardSpatial, true);
	TestTrue(TEXT("discard countdown starts"), Authority.BeginInteraction(Edemo_mapM01ExitType::DiscardSpatial));
	Authority.SetSpatialItemEquipped(true);
	TestEqual(TEXT("re-equip cancels"), Authority.GetSnapshot(Edemo_mapM01ExitType::DiscardSpatial).CancelReason, Edemo_mapM01ExtractionCancelReason::ConditionInvalidated);
	Authority.SetSpatialItemEquipped(false);
	TestTrue(TEXT("can retry after dropping again"), Authority.BeginInteraction(Edemo_mapM01ExitType::DiscardSpatial));
	Authority.NotifyPlayerDefeated();
	TestEqual(TEXT("defeat cancels"), Authority.GetSnapshot(Edemo_mapM01ExitType::DiscardSpatial).CancelReason, Edemo_mapM01ExtractionCancelReason::PlayerDefeated);
	Authority.NotifyRunTerminal();
	TestEqual(TEXT("terminal makes exit unavailable"), Authority.GetSnapshot(Edemo_mapM01ExitType::DiscardSpatial).State, Edemo_mapM01ExtractionState::Unavailable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FM01SpatialBundleTest,
	"demo_map.M01Extraction.05.SpatialBundleAtomicDiscardRecoverSameGuids",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FM01SpatialBundleTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid Backpack;
	TArray<FGuid> Inventory;
	TestTrue(TEXT("fixture"), PrepareSpatialFixture(Authority, Backpack, Inventory));
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	Fdemo_mapSpatialDiscardBundle InjectedFailure;
	const Fdemo_mapItemOperationResult Failed = Authority.DiscardSpatialBundle(InjectedFailure, true);
	TestFalse(TEXT("injected discard fails"), Failed.bSuccess);
	TestEqual(TEXT("rollback equipment"), Authority.GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot), Backpack);
	TestTrue(TEXT("rollback inventory"), Authority.GetInventorySlotSnapshot() == Before.InventorySlots);

	Fdemo_mapSpatialDiscardBundle Bundle;
	TestTrue(TEXT("discard commits"), Authority.DiscardSpatialBundle(Bundle).bSuccess);
	TestEqual(TEXT("only seventh cell is bundled"), Bundle.StoredItemInstanceIds.Num(), 1);
	TestEqual(TEXT("same backpack GUID in world"), Bundle.SpatialItemInstanceId, Backpack);
	for (const FGuid Id : Bundle.GetAllInstanceIds())
	{
		const Fdemo_mapItemInstance* Instance = Authority.FindInstance(Id);
		TestTrue(TEXT("bundle member is world-owned"), Instance && Instance->OwnershipState == Edemo_mapItemOwnershipState::World);
	}
	TestEqual(TEXT("base six remain carried"), Authority.GetUsedInventorySlots(), 6);
	TestTrue(TEXT("recover commits"), Authority.RecoverSpatialBundle(Bundle).bSuccess);
	TestEqual(TEXT("same backpack re-equipped"), Authority.GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot), Backpack);
	TestTrue(TEXT("same stored GUID recovered"), Authority.GetInventorySlotSnapshot().Contains(Bundle.StoredItemInstanceIds[0]));
	FString Error;
	TestTrue(TEXT("post-recovery invariants"), Authority.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FM01SpatialSettlementLossTest,
	"demo_map.M01Extraction.06.DiscardedBundleExcludedFromExtractionSettlement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FM01SpatialSettlementLossTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid Backpack;
	TArray<FGuid> Inventory;
	TestTrue(TEXT("fixture"), PrepareSpatialFixture(Authority, Backpack, Inventory));
	const FGuid RunId = FGuid::NewGuid();
	TArray<FGuid> AllIds = Inventory;
	AllIds.Add(Backpack);
	TestTrue(TEXT("tag run"), Authority.TagInstancesForRun(AllIds, RunId).bSuccess);
	Fdemo_mapSpatialDiscardBundle Bundle;
	TestTrue(TEXT("discard"), Authority.DiscardSpatialBundle(Bundle).bSuccess);
	TSet<FGuid> Deployed(AllIds);
	TArray<Fdemo_mapSettlementItemRow> Rows;
	TestTrue(TEXT("settle extraction"), Authority.SettleRunItems(RunId, Deployed, true, Rows).bSuccess);
	for (const FGuid LostId : Bundle.GetAllInstanceIds())
	{
		const Fdemo_mapSettlementItemRow* Row = Rows.FindByPredicate([LostId](const Fdemo_mapSettlementItemRow& Candidate)
		{
			return Candidate.InstanceId == LostId;
		});
		TestTrue(TEXT("discarded member reported lost"), Row
			&& Row->SourceOwnership == Edemo_mapItemOwnershipState::World
			&& Row->FinalOwnership == Edemo_mapItemOwnershipState::Destroyed);
		TestFalse(TEXT("discarded member not in stash"), Authority.GetSessionStashSnapshot().Contains(LostId));
	}
	TestEqual(TEXT("base six secured"), Authority.GetSessionStashSnapshot().Num(), 6);
	return true;
}

#endif

