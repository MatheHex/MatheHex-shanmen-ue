#if WITH_DEV_AUTOMATION_TESTS

#include "CodeB/demo_mapCodeBP2.h"
#include "Misc/AutomationTest.h"

namespace
{
	using namespace demo_map_code_b;

	FGuid MakeTestTransactionGuid(uint32 Step)
	{
		return FGuid(0xB3000000u + Step, 0x00000002u, 0x00000009u, 0x000000B0u);
	}

	FCodeBP2Command MakeCommand(
		const FCodeBRepository& Repository,
		ECodeBOperation Operation,
		const FGuid& ItemId,
		const FGuid& TargetContainerId,
		int32 TargetSlot,
		uint32 TransactionStep,
		int32 Quantity = 0,
		int32 ExpectedRevision = INDEX_NONE)
	{
		FCodeBP2Command Command;
		Command.TransactionId = MakeTestTransactionGuid(TransactionStep);
		Command.Operation = Operation;
		Command.ItemId = ItemId;
		Command.TargetContainerId = TargetContainerId;
		Command.TargetSlot = TargetSlot;
		Command.Quantity = Quantity;
		const FCodeBItemInstance* Item = Repository.FindItem(ItemId);
		if (Item)
		{
			Command.SourceContainerId = Item->ParentContainerId;
			Command.SourceSlot = Item->SlotIndex;
		}
		Command.ExpectedRevision = ExpectedRevision == INDEX_NONE ? Repository.GetRevision() : ExpectedRevision;
		return Command;
	}

	bool BuildFixture(FAutomationTestBase& Test, FCodeBP2Fixture& Fixture)
	{
		FString Error;
		const bool bBuilt = FCodeBP2Fixture::Build(Fixture, &Error);
		Test.TestTrue(TEXT("P2 fixture builds"), bBuilt);
		if (!bBuilt)
		{
			Test.AddError(Error);
		}
		return bBuilt;
	}

	bool ExpectSuccess(FAutomationTestBase& Test, const FCodeBP2ApplicationResult& Result, const TCHAR* Label)
	{
		const bool bSuccess = Result.IsSuccess();
		Test.TestTrue(FString::Printf(TEXT("%s succeeds"), Label), bSuccess);
		if (!bSuccess)
		{
			Test.AddError(FString::Printf(TEXT("%s failed: %s"), Label, *Result.Message));
		}
		return bSuccess;
	}

	bool ExpectFailure(
		FAutomationTestBase& Test,
		const FCodeBP2ApplicationResult& Result,
		ECodeBResultCode ExpectedCode,
		const TCHAR* Label)
	{
		const bool bFailure = !Result.IsSuccess() && Result.P1Result.Code == ExpectedCode;
		Test.TestTrue(FString::Printf(TEXT("%s fails with expected P1 code"), Label), bFailure);
		if (!bFailure)
		{
			Test.AddError(FString::Printf(TEXT("%s returned P2=%d P1=%d."), Label, static_cast<int32>(Result.Code), static_cast<int32>(Result.P1Result.Code)));
		}
		return bFailure;
	}

	struct FReplayStats
	{
		int32 Successes = 0;
		int32 Failures = 0;
	};

	bool RunReplaySequence(FAutomationTestBase& Test, FCodeBP2Fixture& Fixture, FReplayStats& Stats, bool bReportEachStep)
	{
		FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
		const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
		const FCodeBRepository& Repository = Fixture.GetRepository();
		FGuid SplitItemId;
		bool bAllGood = true;

		auto ApplySuccess = [&](ECodeBOperation Operation, const FGuid& ItemId, const FGuid& TargetContainerId, int32 TargetSlot, uint32 Step, int32 Quantity = 0) -> FCodeBP2ApplicationResult
		{
			FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Repository, Operation, ItemId, TargetContainerId, TargetSlot, Step, Quantity));
			++Stats.Successes;
			const bool bOk = Result.IsSuccess();
			bAllGood &= bOk;
			if (bReportEachStep)
			{
				ExpectSuccess(Test, Result, *FString::Printf(TEXT("Replay step %u"), Step));
			}
			return Result;
		};

		auto ApplyFailure = [&](ECodeBOperation Operation, const FGuid& ItemId, const FGuid& TargetContainerId, int32 TargetSlot, uint32 Step, ECodeBResultCode ExpectedCode, int32 ExpectedRevision = INDEX_NONE) -> FCodeBP2ApplicationResult
		{
			FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Repository, Operation, ItemId, TargetContainerId, TargetSlot, Step, 0, ExpectedRevision));
			++Stats.Failures;
			const bool bOk = !Result.IsSuccess() && Result.P1Result.Code == ExpectedCode;
			bAllGood &= bOk;
			if (bReportEachStep)
			{
				ExpectFailure(Test, Result, ExpectedCode, *FString::Printf(TEXT("Replay failure step %u"), Step));
			}
			return Result;
		};

		ApplySuccess(ECodeBOperation::Move, Ids.WeaponAItemId, Ids.BasicContainerId, 0, 1);
		ApplyFailure(ECodeBOperation::Move, Ids.WeaponBItemId, Ids.BasicContainerId, 0, 2, ECodeBResultCode::TargetOccupied);
		ApplySuccess(ECodeBOperation::Move, Ids.WeaponAItemId, Ids.WarehouseContainerId, 0, 3);
		ApplySuccess(ECodeBOperation::Equip, Ids.WeaponAItemId, Ids.WeaponContainerId, 0, 4);
		ApplySuccess(ECodeBOperation::Equip, Ids.WeaponBItemId, Ids.WeaponContainerId, 0, 5);
		ApplySuccess(ECodeBOperation::Equip, Ids.WeaponAItemId, Ids.WeaponContainerId, 0, 6);
		ApplySuccess(ECodeBOperation::Unequip, Ids.WeaponAItemId, Ids.WarehouseContainerId, 0, 7);
		ApplySuccess(ECodeBOperation::Equip, Ids.ArmorItemId, Ids.ArmorContainerId, 0, 8);
		ApplySuccess(ECodeBOperation::Unequip, Ids.ArmorItemId, Ids.WarehouseContainerId, 2, 9);
		ApplySuccess(ECodeBOperation::Equip, Ids.AccessoryAItemId, Ids.Accessory0ContainerId, 0, 10);
		ApplySuccess(ECodeBOperation::Unequip, Ids.AccessoryAItemId, Ids.WarehouseContainerId, 3, 11);
		ApplySuccess(ECodeBOperation::Equip, Ids.AccessoryBItemId, Ids.Accessory1ContainerId, 0, 12);
		ApplySuccess(ECodeBOperation::Unequip, Ids.AccessoryBItemId, Ids.WarehouseContainerId, 4, 13);
		ApplySuccess(ECodeBOperation::Equip, Ids.SpatialItemId, Ids.SpatialContainerId, 0, 14);
		ApplySuccess(ECodeBOperation::Move, Ids.DustAItemId, Ids.SpatialInternalContainerId, 0, 15);
		ApplySuccess(ECodeBOperation::Move, Ids.DustAItemId, Ids.WarehouseContainerId, 6, 16);
		ApplySuccess(ECodeBOperation::Unequip, Ids.SpatialItemId, Ids.WarehouseContainerId, 5, 17);
		ApplyFailure(ECodeBOperation::Move, Ids.PotionItemId, Ids.BasicContainerId, 1, 18, ECodeBResultCode::StaleRevision, Repository.GetRevision() - 1);

		FCodeBP2ApplicationResult SplitResult = ApplySuccess(ECodeBOperation::Split, Ids.DustAItemId, Ids.BasicContainerId, 0, 19, 4);
		if (SplitResult.IsSuccess())
		{
			SplitItemId = SplitResult.P1Result.CreatedItemId;
		}
		FCodeBP2ApplicationResult MergeResult = ApplySuccess(ECodeBOperation::Merge, SplitItemId, Ids.WarehouseContainerId, 7, 20, 4);
		if (!MergeResult.IsSuccess())
		{
			bAllGood = false;
		}
		FCodeBP2ApplicationResult SplitBackResult = ApplySuccess(ECodeBOperation::Split, Ids.DustBItemId, Ids.BasicContainerId, 0, 21, 4);
		if (SplitBackResult.IsSuccess())
		{
			SplitItemId = SplitBackResult.P1Result.CreatedItemId;
		}
		FCodeBP2ApplicationResult MergeBackResult = ApplySuccess(ECodeBOperation::Merge, SplitItemId, Ids.WarehouseContainerId, 6, 22, 4);
		if (!MergeBackResult.IsSuccess())
		{
			bAllGood = false;
		}
		ApplyFailure(ECodeBOperation::Equip, Ids.InvalidEquipItemId, Ids.WeaponContainerId, 0, 23, ECodeBResultCode::IncompatibleSlot);
		return bAllGood;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2FixtureIdentityTest, "demo_map.CodeB.P2.FixtureIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2FixtureIdentityTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture))
	{
		return false;
	}
	FCodeBP2Projection Projection;
	FString Error;
	TestTrue(TEXT("Fixture projection builds"), FCodeBP2ProjectionBuilder::Build(Fixture.GetRepository(), Fixture.GetLayout(), Projection, nullptr, &Error));
	TestEqual(TEXT("Projection exposes ring quick-space and pouch non-quick-space containers separately"), Projection.Containers.Num(), 9);
	TestEqual(TEXT("Stash has thirty slots"), Projection.Containers[0].Capacity, 30);
	TestEqual(TEXT("Basic storage has six slots"), Projection.Containers[1].Capacity, 6);
	TestEqual(TEXT("Two accessory containers exist"), Fixture.GetLayout().AccessoryContainerIds.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2ProjectionStableTest, "demo_map.CodeB.P2.ProjectionStable", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2ProjectionStableTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture))
	{
		return false;
	}
	FCodeBP2Projection First;
	FCodeBP2Projection Second;
	FString Error;
	TestTrue(TEXT("First projection builds"), FCodeBP2ProjectionBuilder::Build(Fixture.GetRepository(), Fixture.GetLayout(), First, nullptr, &Error));
	TestTrue(TEXT("Second projection builds"), FCodeBP2ProjectionBuilder::Build(Fixture.GetRepository(), Fixture.GetLayout(), Second, nullptr, &Error));
	TestTrue(TEXT("Repeated projection is field and order stable"), First == Second);
	TestEqual(TEXT("Empty slots have stable addresses"), First.Containers[1].Slots[0].SlotId, FName(TEXT("Basic6.00")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2StashBasicRoundTripTest, "demo_map.CodeB.P2.StashBasicRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2StashBasicRoundTripTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FCodeBItemInstance Before = *Fixture.GetRepository().FindItem(Ids.WeaponAItemId);
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponAItemId, Ids.BasicContainerId, 0, 1)), TEXT("Stash to Basic6"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponAItemId, Ids.WarehouseContainerId, 0, 2)), TEXT("Basic6 to stash"));
	const FCodeBItemInstance* After = Fixture.GetRepository().FindItem(Ids.WeaponAItemId);
	TestTrue(TEXT("Round-trip preserves the unique instance fields"), After && *After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2WeaponEquipTest, "demo_map.CodeB.P2.WeaponEquip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2WeaponEquipTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.WeaponAItemId, Ids.WeaponContainerId, 0, 3));
	ExpectSuccess(*this, Result, TEXT("Weapon equip"));
	const FCodeBItemInstance* Item = Fixture.GetRepository().FindItem(Ids.WeaponAItemId);
	TestTrue(TEXT("Weapon is in the weapon slot"), Item && Item->ParentContainerId == Ids.WeaponContainerId && Item->SlotIndex == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2WeaponReplacementTest, "demo_map.CodeB.P2.WeaponReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2WeaponReplacementTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.WeaponAItemId, Ids.WeaponContainerId, 0, 4)), TEXT("Initial weapon equip"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.WeaponBItemId, Ids.WeaponContainerId, 0, 5)), TEXT("Weapon replacement"));
	const FCodeBItemInstance* OldWeapon = Fixture.GetRepository().FindItem(Ids.WeaponAItemId);
	const FCodeBItemInstance* NewWeapon = Fixture.GetRepository().FindItem(Ids.WeaponBItemId);
	TestTrue(TEXT("Replacement leaves each weapon in exactly one location"), OldWeapon && NewWeapon && OldWeapon->ParentContainerId == Ids.WarehouseContainerId && OldWeapon->SlotIndex == 1 && NewWeapon->ParentContainerId == Ids.WeaponContainerId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2OtherEquipmentTest, "demo_map.CodeB.P2.OtherEquipment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2OtherEquipmentTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.ArmorItemId, Ids.ArmorContainerId, 0, 6)), TEXT("Robe equip"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.AccessoryAItemId, Ids.Accessory0ContainerId, 0, 7)), TEXT("Accessory slot zero equip"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.AccessoryBItemId, Ids.Accessory1ContainerId, 0, 8)), TEXT("Accessory slot one equip"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.SpatialItemId, Ids.SpatialContainerId, 0, 9)), TEXT("Spatial item equip"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2IncompatibleEquipTest, "demo_map.CodeB.P2.IncompatibleEquipFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2IncompatibleEquipTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FCodeBSnapshot Before = Fixture.GetRepository().CaptureSnapshot();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.InvalidEquipItemId, Ids.WeaponContainerId, 0, 10));
	ExpectFailure(*this, Result, ECodeBResultCode::IncompatibleSlot, TEXT("Incompatible equipment"));
	TestTrue(TEXT("Incompatible equip is atomic"), Fixture.GetRepository().CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2SplitTest, "demo_map.CodeB.P2.Split", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2SplitTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Split, Ids.DustAItemId, Ids.BasicContainerId, 0, 11, 4));
	ExpectSuccess(*this, Result, TEXT("Stack split"));
	const FCodeBItemInstance* Source = Fixture.GetRepository().FindItem(Ids.DustAItemId);
	const FCodeBItemInstance* Created = Fixture.GetRepository().FindItem(Result.P1Result.CreatedItemId);
	TestTrue(TEXT("Split creates a new ItemId and conserves quantity"), Source && Created && Result.P1Result.CreatedItemId.IsValid() && Source->Quantity + Created->Quantity == 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2MergeTest, "demo_map.CodeB.P2.Merge", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2MergeTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FCodeBP2ApplicationResult Split = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Split, Ids.DustAItemId, Ids.BasicContainerId, 0, 12, 4));
	ExpectSuccess(*this, Split, TEXT("Merge setup split"));
	const FCodeBP2ApplicationResult Merge = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Merge, Split.P1Result.CreatedItemId, Ids.WarehouseContainerId, 7, 13, 4));
	ExpectSuccess(*this, Merge, TEXT("Stack merge"));
	const FCodeBItemInstance* Target = Fixture.GetRepository().FindItem(Ids.DustBItemId);
	TestTrue(TEXT("Merge increases the target stack"), Target && Target->Quantity == 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2SpatialInternalRoundTripTest, "demo_map.CodeB.P2.SpatialInternalRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2SpatialInternalRoundTripTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.SpatialItemId, Ids.SpatialContainerId, 0, 14)), TEXT("Spatial item equip for internal storage"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.DustAItemId, Ids.SpatialInternalContainerId, 0, 15)), TEXT("Move item into spatial internal storage"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.DustAItemId, Ids.WarehouseContainerId, 6, 16)), TEXT("Move item out of spatial internal storage"));
	const FCodeBItemInstance* Item = Fixture.GetRepository().FindItem(Ids.DustAItemId);
	TestTrue(TEXT("Internal round trip preserves ItemId and placement"), Item && Item->ParentContainerId == Ids.WarehouseContainerId && Item->SlotIndex == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2LoadedSpatialMoveTest, "demo_map.CodeB.P2.LoadedSpatialMoveUnsupported", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2LoadedSpatialMoveTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Equip, Ids.SpatialItemId, Ids.SpatialContainerId, 0, 160)), TEXT("Loaded spatial item equip"));
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.DustAItemId, Ids.SpatialInternalContainerId, 0, 161)), TEXT("Load spatial item internal storage"));
	const FCodeBSnapshot Before = Fixture.GetRepository().CaptureSnapshot();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Unequip, Ids.SpatialItemId, Ids.WarehouseContainerId, 5, 162));
	TestTrue(TEXT("Loaded spatial item move is explicitly unsupported"), !Result.IsSuccess() && Result.Code == ECodeBP2ResultCode::LoadedSpatialItemMoveUnsupported);
	TestTrue(TEXT("Unsupported loaded spatial item move is atomic"), Fixture.GetRepository().CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2OccupiedTargetTest, "demo_map.CodeB.P2.OccupiedTargetFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2OccupiedTargetTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponAItemId, Ids.BasicContainerId, 0, 17)), TEXT("Occupy basic target"));
	const FCodeBSnapshot Before = Fixture.GetRepository().CaptureSnapshot();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponBItemId, Ids.BasicContainerId, 0, 18));
	ExpectFailure(*this, Result, ECodeBResultCode::TargetOccupied, TEXT("Occupied target"));
	TestTrue(TEXT("Occupied target failure preserves snapshot"), Fixture.GetRepository().CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2FullTargetTest, "demo_map.CodeB.P2.FullTargetFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2FullTargetTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FGuid Items[] = { Ids.WeaponAItemId, Ids.WeaponBItemId, Ids.ArmorItemId, Ids.AccessoryAItemId, Ids.AccessoryBItemId, Ids.SpatialItemId };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Items); ++Index)
	{
		ExpectSuccess(*this, Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Items[Index], Ids.BasicContainerId, Index, 20 + Index)), TEXT("Fill basic target"));
	}
	const FCodeBSnapshot Before = Fixture.GetRepository().CaptureSnapshot();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.DustAItemId, Ids.BasicContainerId, INDEX_NONE, 27));
	ExpectFailure(*this, Result, ECodeBResultCode::TargetFull, TEXT("Full target"));
	TestTrue(TEXT("Full target failure preserves snapshot"), Fixture.GetRepository().CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2SourceMismatchTest, "demo_map.CodeB.P2.SourceMismatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2SourceMismatchTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	FCodeBP2Command Command = MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponAItemId, Ids.BasicContainerId, 0, 28);
	Command.SourceContainerId = Ids.BasicContainerId;
	const FCodeBSnapshot Before = Fixture.GetRepository().CaptureSnapshot();
	const FCodeBP2ApplicationResult Result = Service.Apply(Command);
	ExpectFailure(*this, Result, ECodeBResultCode::SourceMismatch, TEXT("Source mismatch"));
	TestTrue(TEXT("Source mismatch preserves snapshot"), Fixture.GetRepository().CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2StaleRevisionTest, "demo_map.CodeB.P2.StaleRevision", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2StaleRevisionTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const FCodeBSnapshot Before = Fixture.GetRepository().CaptureSnapshot();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponAItemId, Ids.BasicContainerId, 0, 29, 0, Fixture.GetRepository().GetRevision() - 1));
	ExpectFailure(*this, Result, ECodeBResultCode::StaleRevision, TEXT("Stale revision"));
	TestTrue(TEXT("Stale revision preserves snapshot"), Fixture.GetRepository().CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2RevisionAndAffectedIdsTest, "demo_map.CodeB.P2.RevisionAndAffectedIds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2RevisionAndAffectedIdsTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	const FCodeBP2FixtureIds& Ids = Fixture.GetIds();
	const int32 BeforeRevision = Fixture.GetRepository().GetRevision();
	const FCodeBP2ApplicationResult Result = Service.Apply(MakeCommand(Fixture.GetRepository(), ECodeBOperation::Move, Ids.WeaponAItemId, Ids.BasicContainerId, 0, 30));
	ExpectSuccess(*this, Result, TEXT("Revision operation"));
	TestEqual(TEXT("Successful application increments exactly once"), Result.P1Result.NewRevision, BeforeRevision + 1);
	TestEqual(TEXT("Projection Revision equals Repository Revision"), Result.Projection.Revision, Fixture.GetRepository().GetRevision());
	TestTrue(TEXT("Affected ItemId is reported"), Result.P1Result.AffectedItemIds.Contains(Ids.WeaponAItemId));
	TestTrue(TEXT("Affected containers are reported"), Result.P1Result.AffectedContainerIds.Contains(Ids.WarehouseContainerId) && Result.P1Result.AffectedContainerIds.Contains(Ids.BasicContainerId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2ProjectionImmutableTest, "demo_map.CodeB.P2.ProjectionImmutable", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2ProjectionImmutableTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	FCodeBP2ApplicationService Service = Fixture.MakeApplicationService();
	FCodeBP2Projection Before;
	FString Error;
	TestTrue(TEXT("Initial projection builds"), Service.BuildCurrentProjection(Before, &Error));
	FCodeBP2Projection LocalCopy = Before;
	LocalCopy.Containers[0].Slots[0].ItemId = MakeTestTransactionGuid(999);
	LocalCopy.Containers[0].Slots[0].bOccupied = true;
	FCodeBP2Projection After;
	TestTrue(TEXT("Repository can rebuild after local projection mutation"), Service.BuildCurrentProjection(After, &Error));
	TestTrue(TEXT("Local projection mutation cannot change Repository truth"), After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2CompleteChainTest, "demo_map.CodeB.P2.CompleteConfigureReorganizeChain", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2CompleteChainTest::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture Fixture;
	if (!BuildFixture(*this, Fixture)) return false;
	const FCodeBSnapshot InitialSnapshot = Fixture.GetRepository().CaptureSnapshot();
	FCodeBP2Projection InitialProjection;
	FString Error;
	TestTrue(TEXT("Initial chain projection builds"), Fixture.MakeApplicationService().BuildCurrentProjection(InitialProjection, &Error));
	FReplayStats Stats;
	TestTrue(TEXT("Complete configure/reorganize chain succeeds"), RunReplaySequence(*this, Fixture, Stats, true));
	FCodeBSnapshot FinalSnapshot = Fixture.GetRepository().CaptureSnapshot();
	TestEqual(TEXT("Complete chain Revision advances once per successful command"), FinalSnapshot.Revision, InitialSnapshot.Revision + Stats.Successes);
	FinalSnapshot.Revision = InitialSnapshot.Revision;
	TestTrue(TEXT("Complete chain returns Repository data to the same snapshot"), FinalSnapshot == InitialSnapshot);
	FCodeBP2Projection FinalProjection;
	TestTrue(TEXT("Final chain projection builds"), Fixture.MakeApplicationService().BuildCurrentProjection(FinalProjection, &Error));
	TestEqual(TEXT("Complete chain projection Revision matches Repository"), FinalProjection.Revision, Fixture.GetRepository().GetRevision());
	FinalProjection.Revision = InitialProjection.Revision;
	TestTrue(TEXT("Complete chain returns Projection to the same state"), FinalProjection == InitialProjection);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP2Replay100Test, "demo_map.CodeB.P2.Replay100", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP2Replay100Test::RunTest(const FString& Parameters)
{
	FCodeBP2Fixture ReferenceFixture;
	if (!BuildFixture(*this, ReferenceFixture)) return false;
	const FCodeBSnapshot InitialSnapshot = ReferenceFixture.GetRepository().CaptureSnapshot();
	FCodeBP2Projection InitialProjection;
	FString Error;
	TestTrue(TEXT("Replay reference projection builds"), ReferenceFixture.MakeApplicationService().BuildCurrentProjection(InitialProjection, &Error));
	FReplayStats ReferenceStats;
	TestTrue(TEXT("Replay reference sequence succeeds"), RunReplaySequence(*this, ReferenceFixture, ReferenceStats, false));
	const FCodeBSnapshot ReferenceFinalSnapshot = ReferenceFixture.GetRepository().CaptureSnapshot();
	FCodeBP2Projection ReferenceFinalProjection;
	TestTrue(TEXT("Replay reference final projection builds"), ReferenceFixture.MakeApplicationService().BuildCurrentProjection(ReferenceFinalProjection, &Error));
	FReplayStats TotalStats;
	bool bAllRoundsGood = true;
	for (int32 Round = 0; Round < 100; ++Round)
	{
		FCodeBP2Fixture Fixture;
		if (!FCodeBP2Fixture::Build(Fixture, &Error))
		{
			TestTrue(TEXT("Replay Fixture rebuilds"), false);
			bAllRoundsGood = false;
			break;
		}
		FReplayStats RoundStats;
		bAllRoundsGood &= RunReplaySequence(*this, Fixture, RoundStats, false);
		TotalStats.Successes += RoundStats.Successes;
		TotalStats.Failures += RoundStats.Failures;
		bAllRoundsGood &= Fixture.GetRepository().CaptureSnapshot() == ReferenceFinalSnapshot;
		FCodeBP2Projection FinalProjection;
		bAllRoundsGood &= Fixture.MakeApplicationService().BuildCurrentProjection(FinalProjection, &Error) && FinalProjection == ReferenceFinalProjection;
	}
	TestEqual(TEXT("Replay successes"), TotalStats.Successes, 2000);
	TestEqual(TEXT("Replay legal failures"), TotalStats.Failures, 300);
	TestTrue(TEXT("100 fixed P2 command replays have no cross-round residue"), bAllRoundsGood);
	AddInfo(FString::Printf(TEXT("CodeB P2 replay seed=20260805 rounds=100 successes=%d failures=%d"), TotalStats.Successes, TotalStats.Failures));
	return bAllRoundsGood;
}

#endif
