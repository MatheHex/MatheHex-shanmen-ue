// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CodeB/demo_mapCodeBP3.h"
#include "Misc/AutomationTest.h"

namespace
{
	using namespace demo_map_code_b;

	bool OpenController(FAutomationTestBase& Test, FCodeBP3UIController& Controller)
	{
		FString Error;
		const bool bOpened = Controller.Open(&Error);
		Test.TestTrue(TEXT("P3 controller opens its isolated development fixture"), bOpened);
		if (!bOpened)
		{
			Test.AddError(Error);
		}
		return bOpened;
	}

	bool Address(FAutomationTestBase& Test, const FCodeBP3UIController& Controller, const FGuid& ContainerId, const int32 SlotIndex, FCodeBP3SlotAddress& OutAddress)
	{
		const bool bFound = Controller.MakeAddress(ContainerId, SlotIndex, OutAddress);
		Test.TestTrue(TEXT("Projection supplies a stable grid address"), bFound);
		return bFound;
	}

	bool Select(FAutomationTestBase& Test, FCodeBP3UIController& Controller, const FGuid& ContainerId, const int32 SlotIndex)
	{
		FCodeBP3SlotAddress Source;
		return Address(Test, Controller, ContainerId, SlotIndex, Source) && Controller.ActivateAddress(Source);
	}

	bool Execute(
		FAutomationTestBase& Test,
		FCodeBP3UIController& Controller,
		const FGuid& SourceContainerId,
		const int32 SourceSlot,
		const ECodeBP3OperationMode Mode,
		const FGuid& TargetContainerId,
		const int32 TargetSlot,
		const bool bExpectedSuccess = true,
		const int32 ExpectedRevisionOverride = INDEX_NONE)
	{
		FCodeBP3SlotAddress Target;
		if (!Select(Test, Controller, SourceContainerId, SourceSlot)
			|| !Controller.BeginOperation(Mode)
			|| !Address(Test, Controller, TargetContainerId, TargetSlot, Target))
		{
			return false;
		}
		const bool bResult = Controller.ActivateAddress(Target, ExpectedRevisionOverride);
		Test.TestEqual(TEXT("P3 action result matches expectation"), bResult, bExpectedSuccess);
		return bResult == bExpectedSuccess;
	}

	const FCodeBP2SlotView* FindSlot(const FCodeBP2Projection& Projection, const FGuid& ContainerId, const int32 SlotIndex)
	{
		const FCodeBP2ContainerView* Container = Projection.Containers.FindByPredicate([&ContainerId](const FCodeBP2ContainerView& Candidate)
		{
			return Candidate.ContainerId == ContainerId;
		});
		return Container && Container->Slots.IsValidIndex(SlotIndex) ? &Container->Slots[SlotIndex] : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3DisabledStateTest, "demo_map.CodeB.P3.DisabledState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3DisabledStateTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	TestFalse(TEXT("Default-off controller has no P2 fixture"), Controller.HasFixture());
	TestFalse(TEXT("Default-off controller has no open page state"), Controller.IsOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3SingleFixtureTest, "demo_map.CodeB.P3.SingleFixture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3SingleFixtureTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const int32 FirstRevision = Controller.GetProjection().Revision;
	TestTrue(TEXT("One controller exposes exactly one fixture/service boundary"), Controller.HasFixture());
	TestTrue(TEXT("Duplicate open is idempotent"), Controller.Open());
	TestEqual(TEXT("Duplicate open does not recreate projection state"), Controller.GetProjection().Revision, FirstRevision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3ProjectionShapeTest, "demo_map.CodeB.P3.ProjectionShape", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3ProjectionShapeTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2Projection& Projection = Controller.GetProjection();
	TestEqual(TEXT("P3 renders the distinct ring quick-space and pouch non-quick-space containers"), Projection.Containers.Num(), 9);
	TestEqual(TEXT("P3 stash projection retains thirty cells"), Projection.Containers[0].Capacity, 30);
	TestEqual(TEXT("P3 player projection retains basic six cells"), Projection.Containers[1].Capacity, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3HotbarPlaceholderTest, "demo_map.CodeB.P3.HotbarPlaceholder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3HotbarPlaceholderTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	TestFalse(TEXT("No P2 container represents the disabled 1-9 hotbar"), Controller.GetProjection().Containers.ContainsByPredicate([](const FCodeBP2ContainerView& Container) { return Container.Role.ToString().Contains(TEXT("Hotbar")); }));
	TestEqual(TEXT("No ItemId is selected before a grid click"), Controller.GetSelectedAddress().IsSet(), false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3StableAddressTest, "demo_map.CodeB.P3.StableAddress", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3StableAddressTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP3SlotAddress First;
	FCodeBP3SlotAddress Second;
	if (!Ids || !Address(*this, Controller, Ids->WarehouseContainerId, 6, First) || !Address(*this, Controller, Ids->WarehouseContainerId, 6, Second)) return false;
	TestEqual(TEXT("Repeated render address has the same container"), First.ContainerId, Second.ContainerId);
	TestEqual(TEXT("Repeated render address has the same stable SlotId"), First.SlotId, Second.SlotId);
	TestEqual(TEXT("Repeated render address has the same ItemId"), First.ItemId, Second.ItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3DetailSelectionTest, "demo_map.CodeB.P3.DetailSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3DetailSelectionTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Select(*this, Controller, Ids->WarehouseContainerId, 0)) return false;
	TestTrue(TEXT("Cell selection produces detail address"), Controller.GetSelectedAddress().IsSet());
	TestEqual(TEXT("Detail selection keeps stable ItemId"), Controller.GetSelectedAddress()->ItemId, Ids->WeaponAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3EmptyClickTest, "demo_map.CodeB.P3.EmptyClickBoundary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3EmptyClickTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP3SlotAddress Empty;
	if (!Ids || !Address(*this, Controller, Ids->BasicContainerId, 0, Empty)) return false;
	TestFalse(TEXT("Empty cell does not select or mutate outside target mode"), Controller.ActivateAddress(Empty));
	TestFalse(TEXT("Empty click leaves selection clear"), Controller.GetSelectedAddress().IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3StashBasicRoundTripTest, "demo_map.CodeB.P3.StashBasicRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3StashBasicRoundTripTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Move, Ids->BasicContainerId, 0)
		|| !Execute(*this, Controller, Ids->BasicContainerId, 0, ECodeBP3OperationMode::Move, Ids->WarehouseContainerId, 0)) return false;
	const FCodeBP2SlotView* Slot = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 0);
	TestTrue(TEXT("Round-trip refreshes cell from returned Projection"), Slot && Slot->ItemId == Ids->WeaponAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3WeaponEquipTest, "demo_map.CodeB.P3.WeaponEquip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3WeaponEquipTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Equip, Ids->WeaponContainerId, 0)) return false;
	const FCodeBP2SlotView* Slot = FindSlot(Controller.GetProjection(), Ids->WeaponContainerId, 0);
	TestTrue(TEXT("Legal equip maps to the weapon slot"), Slot && Slot->ItemId == Ids->WeaponAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3WeaponReplacementTest, "demo_map.CodeB.P3.WeaponReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3WeaponReplacementTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Equip, Ids->WeaponContainerId, 0)
		|| !Execute(*this, Controller, Ids->WarehouseContainerId, 1, ECodeBP3OperationMode::Equip, Ids->WeaponContainerId, 0)) return false;
	const FCodeBP2SlotView* Weapon = FindSlot(Controller.GetProjection(), Ids->WeaponContainerId, 0);
	TestTrue(TEXT("Replacement equips the second weapon through P2"), Weapon && Weapon->ItemId == Ids->WeaponBItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3UnequipExplicitTargetTest, "demo_map.CodeB.P3.UnequipExplicitTarget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3UnequipExplicitTargetTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Equip, Ids->WeaponContainerId, 0)
		|| !Execute(*this, Controller, Ids->WeaponContainerId, 0, ECodeBP3OperationMode::Unequip, Ids->WarehouseContainerId, 12)) return false;
	const FCodeBP2SlotView* Target = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 12);
	TestTrue(TEXT("Unequip respects the explicitly clicked target cell"), Target && Target->ItemId == Ids->WeaponAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3SpatialRoundTripTest, "demo_map.CodeB.P3.SpatialRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3SpatialRoundTripTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 5, ECodeBP3OperationMode::Equip, Ids->SpatialContainerId, 0)
		|| !Execute(*this, Controller, Ids->WarehouseContainerId, 6, ECodeBP3OperationMode::Move, Ids->SpatialInternalContainerId, 0)
		|| !Execute(*this, Controller, Ids->SpatialInternalContainerId, 0, ECodeBP3OperationMode::Move, Ids->WarehouseContainerId, 6)) return false;
	const FCodeBP2SlotView* Slot = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 6);
	TestTrue(TEXT("Spatial storage move returns the same ItemId"), Slot && Slot->ItemId == Ids->DustAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3SplitCancelTest, "demo_map.CodeB.P3.SplitCancel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3SplitCancelTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Select(*this, Controller, Ids->WarehouseContainerId, 6) || !Controller.BeginOperation(ECodeBP3OperationMode::Split)) return false;
	const int32 RevisionBeforeCancel = Controller.GetProjection().Revision;
	Controller.CancelOperation();
	TestEqual(TEXT("Split cancel leaves Revision unchanged"), Controller.GetProjection().Revision, RevisionBeforeCancel);
	TestEqual(TEXT("Split cancel leaves no pending target mode"), Controller.GetOperationMode(), ECodeBP3OperationMode::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3SplitSuccessTest, "demo_map.CodeB.P3.SplitSuccess", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3SplitSuccessTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids) return false;
	Controller.SetSplitQuantity(4);
	if (!Execute(*this, Controller, Ids->WarehouseContainerId, 6, ECodeBP3OperationMode::Split, Ids->BasicContainerId, 0)) return false;
	const FCodeBP2SlotView* Source = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 6);
	const FCodeBP2SlotView* Created = FindSlot(Controller.GetProjection(), Ids->BasicContainerId, 0);
	TestTrue(TEXT("Split updates only from P2 returned Projection"), Source && Created && Source->Quantity + Created->Quantity == 12 && Created->ItemId != Ids->DustAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3MergeSelectionTest, "demo_map.CodeB.P3.MergeSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3MergeSelectionTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids) return false;
	Controller.SetSplitQuantity(4);
	if (!Execute(*this, Controller, Ids->WarehouseContainerId, 6, ECodeBP3OperationMode::Split, Ids->BasicContainerId, 0)
		|| !Execute(*this, Controller, Ids->BasicContainerId, 0, ECodeBP3OperationMode::Merge, Ids->WarehouseContainerId, 7)) return false;
	TestTrue(TEXT("Merge leaves a valid selected detail target"), Controller.GetSelectedAddress().IsSet());
	TestEqual(TEXT("Merge selection follows surviving target stack"), Controller.GetSelectedAddress()->ItemId, Ids->DustBItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3OccupiedErrorTest, "demo_map.CodeB.P3.OccupiedError", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3OccupiedErrorTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Move, Ids->BasicContainerId, 0)) return false;
	const int32 RevisionBeforeFailure = Controller.GetProjection().Revision;
	Execute(*this, Controller, Ids->WarehouseContainerId, 1, ECodeBP3OperationMode::Move, Ids->BasicContainerId, 0, false);
	TestEqual(TEXT("Occupied target preserves Revision"), Controller.GetProjection().Revision, RevisionBeforeFailure);
	TestTrue(TEXT("Occupied target maps to visible Chinese feedback"), Controller.GetFeedback().Contains(TEXT("已被占用")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3FullFeedbackTest, "demo_map.CodeB.P3.FullFeedback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3FullFeedbackTest::RunTest(const FString& Parameters)
{
	FCodeBP2ApplicationResult FullResult;
	FullResult.Code = ECodeBP2ResultCode::P1Failure;
	FullResult.P1Result.Code = ECodeBResultCode::TargetFull;
	TestTrue(TEXT("P3 maps full container results to Chinese feedback"), FCodeBP3UIController::GetResultFeedback(FullResult).Contains(TEXT("已满")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3IncompatibleErrorTest, "demo_map.CodeB.P3.IncompatibleError", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3IncompatibleErrorTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids) return false;
	const int32 RevisionBeforeFailure = Controller.GetProjection().Revision;
	Execute(*this, Controller, Ids->WarehouseContainerId, 9, ECodeBP3OperationMode::Equip, Ids->WeaponContainerId, 0, false);
	TestEqual(TEXT("Incompatible equip preserves Revision"), Controller.GetProjection().Revision, RevisionBeforeFailure);
	TestTrue(TEXT("Incompatible equip has visible type feedback"), Controller.GetFeedback().Contains(TEXT("类型")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3LoadedSpatialGraphMoveTest, "demo_map.CodeB.P3.LoadedSpatialGraphMove", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3LoadedSpatialGraphMoveTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 5, ECodeBP3OperationMode::Equip, Ids->SpatialContainerId, 0)
		|| !Execute(*this, Controller, Ids->WarehouseContainerId, 6, ECodeBP3OperationMode::Move, Ids->SpatialInternalContainerId, 0)) return false;
	const FCodeBP2SlotView* ParentBefore = FindSlot(Controller.GetProjection(), Ids->SpatialContainerId, 0);
	const FGuid ChildContainerId = ParentBefore ? ParentBefore->ChildContainerId : FGuid();
	const int32 RevisionBeforeMove = Controller.GetProjection().Revision;
	if (!Execute(*this, Controller, Ids->SpatialContainerId, 0, ECodeBP3OperationMode::Unequip, Ids->WarehouseContainerId, 5)) return false;
	const FCodeBP2SlotView* ParentAfter = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 5);
	const FCodeBP2SlotView* ChildAfter = FindSlot(Controller.GetProjection(), ChildContainerId, 0);
	TestEqual(TEXT("Loaded spatial whole-graph move increments Revision once"), Controller.GetProjection().Revision, RevisionBeforeMove + 1);
	TestTrue(TEXT("Loaded spatial parent preserves ItemId and ChildContainerId"), ParentAfter && ParentAfter->ItemId == Ids->SpatialItemId && ParentAfter->ChildContainerId == ChildContainerId);
	TestTrue(TEXT("Loaded spatial child preserves placement"), ChildAfter && ChildAfter->ItemId == Ids->DustAItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3StaleRevisionTest, "demo_map.CodeB.P3.StaleRevision", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3StaleRevisionTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids) return false;
	const int32 RevisionBeforeFailure = Controller.GetProjection().Revision;
	Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Move, Ids->BasicContainerId, 0, false, RevisionBeforeFailure - 1);
	TestEqual(TEXT("Stale command does not auto-replay"), Controller.GetProjection().Revision, RevisionBeforeFailure);
	TestFalse(TEXT("Stale command clears unsafe selection"), Controller.GetSelectedAddress().IsSet());
	TestTrue(TEXT("Stale command provides reselect feedback"), Controller.GetFeedback().Contains(TEXT("重新操作")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3CloseReopenTest, "demo_map.CodeB.P3.CloseReopen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3CloseReopenTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Execute(*this, Controller, Ids->WarehouseContainerId, 0, ECodeBP3OperationMode::Move, Ids->BasicContainerId, 0)) return false;
	const int32 RevisionBeforeClose = Controller.GetProjection().Revision;
	Controller.Close();
	TestFalse(TEXT("Close hides the page state without releasing fixture"), Controller.IsOpen());
	TestTrue(TEXT("Close preserves fixture for reopening"), Controller.HasFixture());
	if (!Controller.Open()) return false;
	TestEqual(TEXT("Reopen preserves Revision and state"), Controller.GetProjection().Revision, RevisionBeforeClose);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3DuplicateOpenTest, "demo_map.CodeB.P3.DuplicateOpen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3DuplicateOpenTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2Projection Initial = Controller.GetProjection();
	Controller.Open();
	Controller.Open();
	TestTrue(TEXT("Repeated open leaves projection stable"), Controller.GetProjection() == Initial);
	TestTrue(TEXT("Repeated open leaves only one active controller state"), Controller.IsOpen() && Controller.HasFixture());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3EscapeCancelTest, "demo_map.CodeB.P3.EscapeCancel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3EscapeCancelTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	if (!Ids || !Select(*this, Controller, Ids->WarehouseContainerId, 6) || !Controller.BeginOperation(ECodeBP3OperationMode::Move)) return false;
	const int32 RevisionBeforeEscape = Controller.GetProjection().Revision;
	Controller.CancelOperation(TEXT("已取消当前操作"));
	Controller.Close();
	TestEqual(TEXT("Escape cancel and close performs no write"), Controller.GetProjection().Revision, RevisionBeforeEscape);
	TestFalse(TEXT("Escape close leaves no pending source"), Controller.GetPendingSource().IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP3ShutdownCleanupTest, "demo_map.CodeB.P3.ShutdownCleanup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP3ShutdownCleanupTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!OpenController(*this, Controller)) return false;
	Controller.Shutdown();
	TestFalse(TEXT("Host exit releases service and fixture"), Controller.HasFixture());
	TestFalse(TEXT("Host exit leaves no open UI state"), Controller.IsOpen());
	TestFalse(TEXT("Host exit leaves no selected item"), Controller.GetSelectedAddress().IsSet());
	return true;
}

#endif
