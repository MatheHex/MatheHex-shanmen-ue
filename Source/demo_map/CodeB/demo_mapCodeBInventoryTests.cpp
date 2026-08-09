#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Math/RandomStream.h"
#include "CodeB/demo_mapCodeBInventory.h"

namespace
{
	using namespace demo_map_code_b;

	struct FCodeBFixture
	{
		FCodeBRepository Repository;
		FGuid Storage;
		FGuid WeaponSlot;
		FGuid ArmorSlot;
		FGuid AccessorySlot;
		FGuid SpatialSlot;

		FName WeaponDefinition = TEXT("CodeB.Weapon");
		FName ArmorDefinition = TEXT("CodeB.Armor");
		FName AccessoryDefinition = TEXT("CodeB.Accessory");
		FName SpatialDefinition = TEXT("CodeB.SpatialItem");
		FName DustDefinition = TEXT("CodeB.Dust");
		FName PotionDefinition = TEXT("CodeB.Potion");

		bool Seed(int32 StorageCapacity = 32)
		{
			return Register(WeaponDefinition, ECodeBItemType::Weapon, false, 1, ECodeBEquipSlot::Weapon)
				&& Register(ArmorDefinition, ECodeBItemType::Armor, false, 1, ECodeBEquipSlot::Armor)
				&& Register(AccessoryDefinition, ECodeBItemType::Accessory, false, 1, ECodeBEquipSlot::Accessory)
				&& Register(SpatialDefinition, ECodeBItemType::SpatialItem, false, 1, ECodeBEquipSlot::SpatialItem)
				&& Register(DustDefinition, ECodeBItemType::Material, true, 10, ECodeBEquipSlot::None)
				&& Register(PotionDefinition, ECodeBItemType::Consumable, true, 5, ECodeBEquipSlot::None)
				&& CreateContainers(StorageCapacity);
		}

		FGuid Add(FName DefinitionId, int32 Quantity, int32 Slot)
		{
			return Repository.CreateItem(DefinitionId, Quantity, Storage, Slot);
		}

		FCodeBTransactionRequest Request(
			ECodeBOperation Operation,
			const FGuid& ItemId,
			const FGuid& TargetContainerId,
			int32 TargetSlot,
			int32 Quantity = 0,
			bool bUseCurrentRevision = true) const
		{
			FCodeBTransactionRequest RequestValue;
			RequestValue.TransactionId = FGuid::NewGuid();
			RequestValue.Operation = Operation;
			RequestValue.ItemId = ItemId;
			RequestValue.TargetContainerId = TargetContainerId;
			RequestValue.TargetSlot = TargetSlot;
			RequestValue.Quantity = Quantity;
			if (const FCodeBItemInstance* Item = Repository.FindItem(ItemId))
			{
				RequestValue.SourceContainerId = Item->ParentContainerId;
				RequestValue.SourceSlot = Item->SlotIndex;
			}
			RequestValue.ExpectedRevision = bUseCurrentRevision ? Repository.GetRevision() : Repository.GetRevision() - 1;
			return RequestValue;
		}

	private:
		bool Register(FName Id, ECodeBItemType Type, bool bStackable, int32 MaxStack, ECodeBEquipSlot EquipSlot)
		{
			FCodeBItemDefinition Definition;
			Definition.DefinitionId = Id;
			Definition.ItemType = Type;
			Definition.bStackable = bStackable;
			Definition.MaxStack = MaxStack;
			Definition.EquipSlot = EquipSlot;
			return Repository.RegisterDefinition(Definition);
		}

		bool CreateContainers(int32 StorageCapacity)
		{
			Storage = Repository.CreateContainer(TEXT("CodeB.Storage.Player"), StorageCapacity);
			WeaponSlot = Repository.CreateContainer(TEXT("CodeB.Equipment.Weapon"), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Weapon);
			ArmorSlot = Repository.CreateContainer(TEXT("CodeB.Equipment.Armor"), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Armor);
			AccessorySlot = Repository.CreateContainer(TEXT("CodeB.Equipment.Accessory"), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Accessory);
			SpatialSlot = Repository.CreateContainer(TEXT("CodeB.Equipment.Spatial"), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::SpatialItem);
			return Storage.IsValid() && WeaponSlot.IsValid() && ArmorSlot.IsValid() && AccessorySlot.IsValid() && SpatialSlot.IsValid();
		}
	};

	bool ExpectSuccess(FAutomationTestBase& Test, const FCodeBTransactionResult& Result, const TCHAR* Label)
	{
		return Test.TestTrue(Label, Result.IsSuccess());
	}

	bool ExpectFailureAndNoMutation(
		FAutomationTestBase& Test,
		FCodeBFixture& Fixture,
		const FCodeBTransactionRequest& Request,
		ECodeBResultCode ExpectedCode,
		const TCHAR* Label)
	{
		const FCodeBSnapshot Before = Fixture.Repository.CaptureSnapshot();
		const FCodeBTransactionResult Result = Fixture.Repository.ExecuteTransaction(Request);
		const FCodeBSnapshot After = Fixture.Repository.CaptureSnapshot();
		return Test.TestTrue(Label, !Result.IsSuccess() && Result.Code == ExpectedCode && Before == After);
	}

	TArray<FGuid> SnapshotItemIds(const FCodeBSnapshot& Snapshot)
	{
		TArray<FGuid> ItemIds;
		Snapshot.Items.GetKeys(ItemIds);
		return ItemIds;
	}

	int32 RandomStorageSlot(FRandomStream& Stream, const FCodeBFixture& Fixture)
	{
		const FCodeBContainer* Storage = Fixture.Repository.FindContainer(Fixture.Storage);
		return Storage ? Stream.RandRange(0, Storage->Slots.Num() - 1) : INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCodeBRepositoryAndMoveTest,
	"demo_map.CodeB.P1.RepositoryAndMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeBRepositoryAndMoveTest::RunTest(const FString&)
{
	FCodeBFixture Fixture;
	TestTrue(TEXT("Seed Code B definitions and containers"), Fixture.Seed());
	const FGuid Weapon = Fixture.Add(Fixture.WeaponDefinition, 1, 0);
	const FGuid Armor = Fixture.Add(Fixture.ArmorDefinition, 1, 1);
	TestTrue(TEXT("Create unique placed items"), Weapon.IsValid() && Armor.IsValid() && Weapon != Armor);
	FString Error;
	TestTrue(TEXT("Initial repository invariants"), Fixture.Repository.ValidateInvariants(&Error));

	const FCodeBTransactionResult MoveResult = Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Move, Weapon, Fixture.Storage, 4));
	ExpectSuccess(*this, MoveResult, TEXT("Move succeeds"));
	TestTrue(TEXT("Move preserves identity and updates one location"), Fixture.Repository.FindItem(Weapon)->ParentContainerId == Fixture.Storage && Fixture.Repository.FindItem(Weapon)->SlotIndex == 4);
	TestTrue(TEXT("Occupied Move rolls back"), ExpectFailureAndNoMutation(*this, Fixture, Fixture.Request(ECodeBOperation::Move, Armor, Fixture.Storage, 4), ECodeBResultCode::TargetOccupied, TEXT("Occupied Move failure")));

	const FCodeBTransactionResult SwapResult = Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Swap, Weapon, Fixture.Storage, 1));
	ExpectSuccess(*this, SwapResult, TEXT("Swap succeeds"));
	TestTrue(TEXT("Swap exchanges the two item locations"), Fixture.Repository.FindItem(Weapon)->SlotIndex == 1 && Fixture.Repository.FindItem(Armor)->SlotIndex == 4);
	TestTrue(TEXT("Final Move invariants"), Fixture.Repository.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCodeBStackTransactionsTest,
	"demo_map.CodeB.P1.MergeSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeBStackTransactionsTest::RunTest(const FString&)
{
	FCodeBFixture Fixture;
	TestTrue(TEXT("Seed stack fixture"), Fixture.Seed());
	const FGuid Source = Fixture.Add(Fixture.DustDefinition, 8, 0);
	const FGuid Target = Fixture.Add(Fixture.DustDefinition, 2, 1);
	const FGuid DifferentDefinitionTarget = Fixture.Add(Fixture.PotionDefinition, 2, 3);
	TestTrue(TEXT("Create compatible and incompatible stack targets"), Source.IsValid() && Target.IsValid() && DifferentDefinitionTarget.IsValid());

	const FCodeBTransactionResult PartialMerge = Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Merge, Source, Fixture.Storage, 1, 2));
	ExpectSuccess(*this, PartialMerge, TEXT("Partial Merge succeeds"));
	TestEqual(TEXT("Partial Merge preserves source quantity"), Fixture.Repository.FindItem(Source)->Quantity, 6);
	TestEqual(TEXT("Partial Merge increases target quantity"), Fixture.Repository.FindItem(Target)->Quantity, 4);

	const FCodeBTransactionResult SplitResult = Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Split, Source, Fixture.Storage, 2, 3));
	ExpectSuccess(*this, SplitResult, TEXT("Split succeeds"));
	TestTrue(TEXT("Split creates a new GUID"), SplitResult.CreatedItemId.IsValid() && SplitResult.CreatedItemId != Source);
	TestEqual(TEXT("Split reduces original quantity"), Fixture.Repository.FindItem(Source)->Quantity, 3);
	TestEqual(TEXT("Split creates requested quantity"), Fixture.Repository.FindItem(SplitResult.CreatedItemId)->Quantity, 3);

	const FCodeBTransactionResult FullMerge = Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Merge, SplitResult.CreatedItemId, Fixture.Storage, 1, 3));
	ExpectSuccess(*this, FullMerge, TEXT("Full Merge succeeds"));
	TestTrue(TEXT("Full Merge removes zero source instance"), Fixture.Repository.FindItem(SplitResult.CreatedItemId) == nullptr && Fixture.Repository.FindItem(Target)->Quantity == 7);

	TestTrue(TEXT("Different definition Merge fails atomically"), ExpectFailureAndNoMutation(
		*this,
		Fixture,
		Fixture.Request(ECodeBOperation::Merge, Source, Fixture.Storage, 3, 1),
		ECodeBResultCode::StackMismatch,
		TEXT("Different definition Merge failure")));
	FString Error;
	TestTrue(TEXT("Stack invariants"), Fixture.Repository.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCodeBEquipmentTransactionsTest,
	"demo_map.CodeB.P1.EquipUnequip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeBEquipmentTransactionsTest::RunTest(const FString&)
{
	FCodeBFixture Fixture;
	TestTrue(TEXT("Seed equipment fixture"), Fixture.Seed());
	const FGuid Weapon = Fixture.Add(Fixture.WeaponDefinition, 1, 0);
	const FGuid ReplacementWeapon = Fixture.Add(Fixture.WeaponDefinition, 1, 1);
	const FGuid Armor = Fixture.Add(Fixture.ArmorDefinition, 1, 2);
	const FGuid Dust = Fixture.Add(Fixture.DustDefinition, 2, 3);

	ExpectSuccess(*this, Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Equip, Weapon, Fixture.WeaponSlot, 0)), TEXT("Weapon Equip succeeds"));
	ExpectSuccess(*this, Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Equip, Armor, Fixture.ArmorSlot, 0)), TEXT("Armor Equip succeeds"));
	TestTrue(TEXT("Equip keeps the same ItemId"), Fixture.Repository.FindItem(Weapon)->ParentContainerId == Fixture.WeaponSlot);

	const FCodeBSnapshot BeforeInvalid = Fixture.Repository.CaptureSnapshot();
	const FCodeBTransactionResult InvalidEquip = Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Equip, Dust, Fixture.WeaponSlot, 0));
	TestTrue(TEXT("Material Equip rejects incompatible slot and rolls back"), !InvalidEquip.IsSuccess() && InvalidEquip.Code == ECodeBResultCode::IncompatibleSlot && BeforeInvalid == Fixture.Repository.CaptureSnapshot());

	ExpectSuccess(*this, Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Equip, ReplacementWeapon, Fixture.WeaponSlot, 0)), TEXT("Atomic equipment replacement succeeds"));
	TestTrue(TEXT("Replacement equips new and returns old to released source slot"), Fixture.Repository.FindItem(ReplacementWeapon)->ParentContainerId == Fixture.WeaponSlot && Fixture.Repository.FindItem(Weapon)->ParentContainerId == Fixture.Storage && Fixture.Repository.FindItem(Weapon)->SlotIndex == 1);

	ExpectSuccess(*this, Fixture.Repository.ExecuteTransaction(
		Fixture.Request(ECodeBOperation::Unequip, ReplacementWeapon, Fixture.Storage, 4)), TEXT("Unequip succeeds"));
	TestTrue(TEXT("Unequip preserves identity"), Fixture.Repository.FindItem(ReplacementWeapon)->ParentContainerId == Fixture.Storage && Fixture.Repository.FindItem(ReplacementWeapon)->SlotIndex == 4);

	const FCodeBSnapshot BeforeStale = Fixture.Repository.CaptureSnapshot();
	const FCodeBTransactionRequest StaleRequest = Fixture.Request(ECodeBOperation::Move, Armor, Fixture.Storage, 5, 0, false);
	const FCodeBTransactionResult StaleResult = Fixture.Repository.ExecuteTransaction(StaleRequest);
	TestTrue(TEXT("Stale revision rejects without mutation"), !StaleResult.IsSuccess() && StaleResult.Code == ECodeBResultCode::StaleRevision && BeforeStale == Fixture.Repository.CaptureSnapshot());
	FString Error;
	TestTrue(TEXT("Equipment invariants"), Fixture.Repository.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCodeBRandomTransactionInvariantTest,
	"demo_map.CodeB.P1.Random1000Invariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeBRandomTransactionInvariantTest::RunTest(const FString&)
{
	FCodeBFixture Fixture;
	TestTrue(TEXT("Seed random transaction fixture"), Fixture.Seed(64));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		TestTrue(TEXT("Seed weapon instances"), Fixture.Add(Fixture.WeaponDefinition, 1, Index).IsValid());
		TestTrue(TEXT("Seed armor instances"), Fixture.Add(Fixture.ArmorDefinition, 1, Index + 4).IsValid());
		TestTrue(TEXT("Seed accessory instances"), Fixture.Add(Fixture.AccessoryDefinition, 1, Index + 8).IsValid());
		TestTrue(TEXT("Seed spatial instances"), Fixture.Add(Fixture.SpatialDefinition, 1, Index + 12).IsValid());
	}
	for (int32 Index = 0; Index < 5; ++Index)
	{
		TestTrue(TEXT("Seed dust stacks"), Fixture.Add(Fixture.DustDefinition, 5, Index + 16).IsValid());
		TestTrue(TEXT("Seed potion stacks"), Fixture.Add(Fixture.PotionDefinition, 3, Index + 21).IsValid());
	}

	FRandomStream Stream(20260805);
	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	for (int32 Attempt = 0; Attempt < 1000; ++Attempt)
	{
		const FCodeBSnapshot Before = Fixture.Repository.CaptureSnapshot();
		const TArray<FGuid> ItemIds = SnapshotItemIds(Before);
		TestTrue(TEXT("Random fixture retains item candidates"), ItemIds.Num() > 0);
		const FGuid ItemId = ItemIds[Stream.RandRange(0, ItemIds.Num() - 1)];
		const int32 TargetSlot = RandomStorageSlot(Stream, Fixture);
		const int32 OperationIndex = Stream.RandRange(0, 5);
		const bool bUseCurrentRevision = (Attempt % 11) != 0;
		ECodeBOperation Operation = static_cast<ECodeBOperation>(OperationIndex);
		FGuid TargetContainer = Fixture.Storage;
		if (Operation == ECodeBOperation::Equip)
		{
			const int32 EquipIndex = Stream.RandRange(0, 3);
			TargetContainer = EquipIndex == 0 ? Fixture.WeaponSlot : EquipIndex == 1 ? Fixture.ArmorSlot : EquipIndex == 2 ? Fixture.AccessorySlot : Fixture.SpatialSlot;
		}

		FCodeBTransactionRequest Request = Fixture.Request(Operation, ItemId, TargetContainer, Operation == ECodeBOperation::Equip ? 0 : TargetSlot, Stream.RandRange(1, 3), bUseCurrentRevision);
		const FCodeBTransactionResult Result = Fixture.Repository.ExecuteTransaction(Request);
		const FCodeBSnapshot After = Fixture.Repository.CaptureSnapshot();
		FString Error;
		if (!Fixture.Repository.ValidateInvariants(&Error))
		{
			AddError(FString::Printf(TEXT("Random attempt %d invariant failure: %s"), Attempt, *Error));
			return false;
		}
		if (Result.IsSuccess())
		{
			++SuccessCount;
			if (After.Revision != Before.Revision + 1)
			{
				AddError(FString::Printf(TEXT("Random attempt %d successful revision did not advance once."), Attempt));
				return false;
			}
		}
		else
		{
			++FailureCount;
			if (After != Before)
			{
				AddError(FString::Printf(TEXT("Random attempt %d failure mutated repository (%s)."), Attempt, *Result.Message));
				return false;
			}
		}
	}

	TestTrue(TEXT("Random test exercised both successful and legal failed transactions"), SuccessCount > 0 && FailureCount > 0);
	TestEqual(TEXT("Random test performed exactly 1000 attempts"), SuccessCount + FailureCount, 1000);
	AddInfo(FString::Printf(TEXT("CodeB random seed=20260805 attempts=1000 success=%d legal_failures=%d"), SuccessCount, FailureCount));
	return true;
}

#endif
