#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "Engine/GameInstance.h"

namespace
{
	FGuid GridAddOne(FAutomationTestBase& Test, Fdemo_mapItemAuthority& Authority, FName DefinitionId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result = Authority.AddDefinition(DefinitionId, 1, &Affected);
		Test.TestTrue(FString::Printf(TEXT("Add %s"), *DefinitionId.ToString()), Result.bSuccess && Affected.Num() == 1);
		return Affected.Num() == 1 ? Affected[0] : FGuid();
	}

	FGuid GridAddOne(FAutomationTestBase& Test, Udemo_mapItemSubsystem* Items, FName DefinitionId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result = Items->AddDefinition(DefinitionId, 1, &Affected);
		Test.TestTrue(FString::Printf(TEXT("Subsystem add %s"), *DefinitionId.ToString()), Result.bSuccess && Affected.Num() == 1);
		return Affected.Num() == 1 ? Affected[0] : FGuid();
	}

	bool GridInstanceEqual(const Fdemo_mapItemInstance& A, const Fdemo_mapItemInstance& B)
	{
		return A.InstanceId == B.InstanceId
			&& A.DefinitionId == B.DefinitionId
			&& A.Quantity == B.Quantity
			&& A.OwnershipState == B.OwnershipState
			&& A.OwnerId == B.OwnerId
			&& A.ContainerId == B.ContainerId
			&& A.EquippedSlotId == B.EquippedSlotId
			&& A.OriginRunId == B.OriginRunId;
	}

	bool GridStateEqual(const Fdemo_mapItemAuthorityState& A, const Fdemo_mapItemAuthorityState& B)
	{
		if (A.InventorySlots != B.InventorySlots
			|| !A.EquipmentSlots.OrderIndependentCompareEqual(B.EquipmentSlots)
			|| A.SessionStash != B.SessionStash
			|| A.Instances.Num() != B.Instances.Num())
		{
			return false;
		}
		for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair : A.Instances)
		{
			const Fdemo_mapItemInstance* Other = B.Instances.Find(Pair.Key);
			if (!Other || !GridInstanceEqual(Pair.Value, *Other))
			{
				return false;
			}
		}
		return true;
	}

	Fdemo_mapProfilePreparationStashRow GridRow(FGuid Id, FName DefinitionId, int32 StackCount = 1)
	{
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = Id;
		Row.ItemDefinitionId = DefinitionId;
		Row.StackCount = StackCount;
		Row.bSafeInPermanentStash = true;
		if (const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId))
		{
			Row.ItemCategoryId = Definition->CategoryId;
			Row.CompatibleEquipmentSlotId = Definition->CompatibleSlotIds.Num() == 1
				? Definition->CompatibleSlotIds[0]
				: NAME_None;
		}
		return Row;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory01BaseCapacity, "demo_map.GridInventory.01.BaseCapacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory01BaseCapacity::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const Fdemo_mapInventoryCapacityResult Result = Authority.GetInventoryCapacityResult();
	TestTrue(TEXT("Base capacity is the one immutable six-slot source"), Result.bSuccess && Result.Capacity == 6 && Authority.GetInventorySlotSnapshot().Num() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory02BackpackLevel1, "demo_map.GridInventory.02.BackpackLevel1Capacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory02BackpackLevel1::RunTest(const FString&)
{
	const Fdemo_mapInventoryCapacityResult Result = Fdemo_mapItemDefinitions::ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Level 1 total combines six quick plus ten space-item cells"), Result.bSuccess && Result.Capacity == 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory03BackpackLevel2, "demo_map.GridInventory.03.BackpackLevel2Capacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory03BackpackLevel2::RunTest(const FString&)
{
	const Fdemo_mapInventoryCapacityResult Result = Fdemo_mapItemDefinitions::ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel2);
	TestTrue(TEXT("Level 2 total combines six quick plus fourteen space-item cells"), Result.bSuccess && Result.Capacity == 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory04OneByOne, "demo_map.GridInventory.04.OneByOneStackCell", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory04OneByOne::RunTest(const FString&)
{
	bool bAllOneByOne = true;
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
	{
		bAllOneByOne &= Definition.GridWidth == 1 && Definition.GridHeight == 1;
	}
	Fdemo_mapItemAuthority Authority;
	TestTrue(TEXT("Add one full material Stack"), Authority.AddDefinition(Fdemo_mapItemIds::SpiritDust, 5).bSuccess);
	const Fdemo_mapGridContainerSnapshot Grid = Authority.BuildRunInventoryGridSnapshot();
	TestTrue(TEXT("Every Definition is 1x1 and one Stack occupies one cell"), bAllOneByOne && Grid.bValid && Grid.UsedSlots == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory05PermanentProjection, "demo_map.GridInventory.05.PermanentStashProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory05PermanentProjection::RunTest(const FString&)
{
	const TArray<FGuid> Ordered = { FGuid::NewGuid(), FGuid::NewGuid(), FGuid::NewGuid(), FGuid::NewGuid(), FGuid::NewGuid(), FGuid::NewGuid(), FGuid::NewGuid() };
	const Fdemo_mapGridContainerSnapshot Grid = Fdemo_mapItemViewRules::BuildGridFromOccupiedOrder(Ordered, Ordered.Num());
	TestTrue(TEXT("Permanent Stash projection preserves order without the six-slot cap"), Grid.bValid && Grid.Capacity == 7 && Grid.UsedSlots == 7 && Grid.OrderedSlots[6].ItemInstanceId == Ordered[6]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory06RunLength, "demo_map.GridInventory.06.RunSnapshotLength", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory06RunLength::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TestTrue(TEXT("Base add"), Authority.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 2).bSuccess);
	const Fdemo_mapGridContainerSnapshot Base = Authority.BuildRunInventoryGridSnapshot();
	const FGuid Backpack = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Equip Level 1 backpack"), Authority.Equip(Backpack, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	const Fdemo_mapGridContainerSnapshot Expanded = Authority.BuildRunInventoryGridSnapshot();
	TestTrue(TEXT("Run snapshot length follows layered capacity"), Base.bValid && Base.OrderedSlots.Num() == 6 && Expanded.bValid && Expanded.OrderedSlots.Num() == 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory07Determinism, "demo_map.GridInventory.07.DeterministicProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory07Determinism::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TestTrue(TEXT("Create deterministic source state"), Authority.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 3).bSuccess);
	TestTrue(TEXT("Repeated projections are value-identical"), Authority.BuildRunInventoryGridSnapshot() == Authority.BuildRunInventoryGridSnapshot());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory08FullSix, "demo_map.GridInventory.08.FullSixRejectsAtomically", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory08FullSix::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TestTrue(TEXT("Fill six cells"), Authority.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 6).bSuccess);
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	const Fdemo_mapItemOperationResult Result = Authority.AddDefinition(Fdemo_mapItemIds::TrainingVest, 1);
	TestTrue(TEXT("Seventh cell rejects with zero mutation"), !Result.bSuccess && Result.Code == Edemo_mapItemResultCode::InventoryFull && GridStateEqual(Before, Authority.CaptureState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory09ExpandedBoundaries, "demo_map.GridInventory.09.FullTenAndFourteenBoundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory09ExpandedBoundaries::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Ten;
	const FGuid Level1 = GridAddOne(*this, Ten, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Equip Level 1"), Ten.Equip(Level1, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	TestTrue(TEXT("Sixteen succeeds and seventeen rejects"), Ten.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 16).bSuccess && !Ten.AddDefinition(Fdemo_mapItemIds::TrainingVest, 1).bSuccess);

	Fdemo_mapItemAuthority Fourteen;
	const FGuid Level2 = GridAddOne(*this, Fourteen, Fdemo_mapItemIds::BackpackLevel2);
	TestTrue(TEXT("Equip Level 2"), Fourteen.Equip(Level2, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	TestTrue(TEXT("Twenty succeeds and twenty-one rejects"), Fourteen.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 20).bSuccess && !Fourteen.AddDefinition(Fdemo_mapItemIds::TrainingVest, 1).bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory10LegacySlots, "demo_map.GridInventory.10.LegacyThreeSlots", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory10LegacySlots::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Weapon = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	const FGuid Armor = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingVest);
	const FGuid Accessory = GridAddOne(*this, Authority, Fdemo_mapItemIds::WindTalisman);
	TestTrue(TEXT("Weapon Armor Accessory behavior remains"), Authority.Equip(Weapon, Fdemo_mapItemIds::WeaponSlot).bSuccess && Authority.Equip(Armor, Fdemo_mapItemIds::ArmorSlot).bSuccess && Authority.Equip(Accessory, Fdemo_mapItemIds::AccessorySlot).bSuccess);
	TestTrue(TEXT("Legacy identities remain equipped"), Authority.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == Weapon && Authority.GetEquippedInstance(Fdemo_mapItemIds::ArmorSlot) == Armor && Authority.GetEquippedInstance(Fdemo_mapItemIds::AccessorySlot) == Accessory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory11BackpackCompatibility, "demo_map.GridInventory.11.BackpackSlotCompatibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory11BackpackCompatibility::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Backpack = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel1);
	const FGuid Weapon = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	TestTrue(TEXT("Weapon cannot enter Backpack slot"), Authority.Equip(Weapon, Fdemo_mapItemIds::BackpackSlot).Code == Edemo_mapItemResultCode::IncompatibleSlot && GridStateEqual(Before, Authority.CaptureState()));
	TestTrue(TEXT("Backpack cannot enter Weapon slot"), Authority.Equip(Backpack, Fdemo_mapItemIds::WeaponSlot).Code == Edemo_mapItemResultCode::IncompatibleSlot);
	TestTrue(TEXT("Backpack enters only Backpack slot"), Authority.Equip(Backpack, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory12EmptyBackpackEquip, "demo_map.GridInventory.12.EmptyBackpackEquip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory12EmptyBackpackEquip::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Backpack = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Empty slot equip preserves ID and adds ten spatial cells after the six base quick cells"), Authority.Equip(Backpack, Fdemo_mapItemIds::BackpackSlot).bSuccess && Authority.GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot) == Backpack && Authority.GetInventoryCapacity() == 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory13Upgrade, "demo_map.GridInventory.13.Level1ToLevel2Atomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory13Upgrade::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Level1 = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel1);
	const FGuid Level2 = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel2);
	TestTrue(TEXT("Equip Level 1"), Authority.Equip(Level1, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	TestTrue(TEXT("Upgrade preserves both identities"), Authority.Equip(Level2, Fdemo_mapItemIds::BackpackSlot).bSuccess && Authority.GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot) == Level2 && Authority.FindInventorySlot(Level1) != INDEX_NONE && Authority.GetInventoryCapacity() == 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory14SafeDowngrade, "demo_map.GridInventory.14.Level2ToLevel1WithinTen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory14SafeDowngrade::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Level2 = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel2);
	const FGuid Level1 = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Equip Level 2"), Authority.Equip(Level2, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	TestTrue(TEXT("Safe downgrade succeeds"), Authority.Equip(Level1, Fdemo_mapItemIds::BackpackSlot).bSuccess && Authority.GetInventoryCapacity() == 16 && Authority.FindInventorySlot(Level2) != INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory15UnsafeDowngrade, "demo_map.GridInventory.15.Level2ToLevel1OverTenRejects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory15UnsafeDowngrade::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Level2 = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel2);
	const FGuid Level1 = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Equip Level 2"), Authority.Equip(Level2, Fdemo_mapItemIds::BackpackSlot).bSuccess);
	TestTrue(TEXT("Reach seventeen occupied cells"), Authority.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 16).bSuccess && Authority.GetUsedInventorySlots() == 17);
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	const Fdemo_mapItemOperationResult Result = Authority.Equip(Level1, Fdemo_mapItemIds::BackpackSlot);
	TestTrue(TEXT("Unsafe downgrade rejects completely"), !Result.bSuccess && Result.Code == Edemo_mapItemResultCode::InventoryFull && GridStateEqual(Before, Authority.CaptureState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory16BackpackUnequip, "demo_map.GridInventory.16.BackpackUnequipPreflight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory16BackpackUnequip::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Safe;
	const FGuid SafeBackpack = GridAddOne(*this, Safe, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Fill base alongside backpack"), Safe.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 5).bSuccess);
	TestTrue(TEXT("Safe backpack round trip"), Safe.Equip(SafeBackpack, Fdemo_mapItemIds::BackpackSlot).bSuccess && Safe.Unequip(Fdemo_mapItemIds::BackpackSlot).bSuccess && Safe.GetInventoryCapacity() == 6 && Safe.GetUsedInventorySlots() == 6);

	Fdemo_mapItemAuthority Full;
	const FGuid FullBackpack = GridAddOne(*this, Full, Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Equip and fill six"), Full.Equip(FullBackpack, Fdemo_mapItemIds::BackpackSlot).bSuccess && Full.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 6).bSuccess);
	const Fdemo_mapItemAuthorityState Before = Full.CaptureState();
	TestTrue(TEXT("Seven into base rejects"), !Full.Unequip(Fdemo_mapItemIds::BackpackSlot).bSuccess && GridStateEqual(Before, Full.CaptureState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory17NormalUnequipFull, "demo_map.GridInventory.17.NormalUnequipFullRejects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory17NormalUnequipFull::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Backpack = GridAddOne(*this, Authority, Fdemo_mapItemIds::BackpackLevel2);
	const FGuid Weapon = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Equip backpack and weapon"), Authority.Equip(Backpack, Fdemo_mapItemIds::BackpackSlot).bSuccess && Authority.Equip(Weapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Fill twenty cells"), Authority.AddDefinition(Fdemo_mapItemIds::TrainingVest, 20).bSuccess);
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	TestTrue(TEXT("Full normal unequip rejects completely"), !Authority.Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess && GridStateEqual(Before, Authority.CaptureState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory18AtomicReplacement, "demo_map.GridInventory.18.AtomicReplacementIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory18AtomicReplacement::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid OldWeapon = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	const FGuid NewWeapon = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Equip old"), Authority.Equip(OldWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Atomic replacement preserves both IDs"), Authority.Equip(NewWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess && Authority.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == NewWeapon && Authority.FindInventorySlot(OldWeapon) != INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory19InvalidOperations, "demo_map.GridInventory.19.InvalidOperationsZeroMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory19InvalidOperations::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Weapon = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	TestTrue(TEXT("Wrong slot rejects"), !Authority.Equip(Weapon, Fdemo_mapItemIds::ArmorSlot).bSuccess && GridStateEqual(Before, Authority.CaptureState()));
	Before = Authority.CaptureState();
	TestTrue(TEXT("Unknown Definition rejects"), !Authority.AddDefinition(TEXT("Prototype.Item.Unknown"), 1).bSuccess && GridStateEqual(Before, Authority.CaptureState()));
	FGuid WorldId;
	TestTrue(TEXT("Create World-owned instance"), Authority.CreateWorldDefinition(Fdemo_mapItemIds::TrainingBlade, 1, WorldId).bSuccess);
	Before = Authority.CaptureState();
	TestTrue(TEXT("Wrong ownership rejects"), !Authority.Equip(WorldId, Fdemo_mapItemIds::WeaponSlot).bSuccess && GridStateEqual(Before, Authority.CaptureState()));
	Before = Authority.CaptureState();
	TestTrue(TEXT("Illegal Stack rejects"), !Authority.AddDefinition(Fdemo_mapItemIds::SpiritDust, 0).bSuccess && GridStateEqual(Before, Authority.CaptureState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory20HotbarOrder, "demo_map.GridInventory.20.HotbarOrderBindUnbind", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory20HotbarOrder::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	Udemo_mapItemSubsystem* Items = NewObject<Udemo_mapItemSubsystem>(GameInstance);
	const FGuid Pill1 = GridAddOne(*this, Items, Fdemo_mapItemIds::HealingPillLevel1);
	const FGuid Pill3 = GridAddOne(*this, Items, Fdemo_mapItemIds::HealingPillLevel3);
	TestTrue(TEXT("Bind external slots 1 and 9"), Items->BindHotbarSlot(1, Pill1).bSuccess && Items->BindHotbarSlot(9, Pill3).bSuccess);
	TestTrue(TEXT("Stable 0..8 order"), Items->GetHotbarBindingSnapshot().SlotBindings.Num() == 9 && Items->GetHotbarBindingSnapshot().SlotBindings[0] == Pill1 && Items->GetHotbarBindingSnapshot().SlotBindings[8] == Pill3);
	TestTrue(TEXT("Unbind clears only one value"), Items->UnbindHotbarSlot(1).bSuccess && !Items->GetHotbarBindingSnapshot().SlotBindings[0].IsValid() && Items->GetHotbarBindingSnapshot().SlotBindings[8] == Pill3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory21HotbarRebind, "demo_map.GridInventory.21.HotbarAtomicRebind", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory21HotbarRebind::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	Udemo_mapItemSubsystem* Items = NewObject<Udemo_mapItemSubsystem>(GameInstance);
	const FGuid Pill = GridAddOne(*this, Items, Fdemo_mapItemIds::HealingPillLevel2);
	TestTrue(TEXT("Initial bind"), Items->BindHotbarSlot(2, Pill).bSuccess);
	TestTrue(TEXT("Rebind clears old slot atomically"), Items->BindHotbarSlot(7, Pill).bSuccess && !Items->GetHotbarBindingSnapshot().SlotBindings[1].IsValid() && Items->GetHotbarBindingSnapshot().SlotBindings[6] == Pill);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory22HotbarRejects, "demo_map.GridInventory.22.HotbarInvalidBindingRejects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory22HotbarRejects::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	Udemo_mapItemSubsystem* Items = NewObject<Udemo_mapItemSubsystem>(GameInstance);
	const FGuid Weapon = GridAddOne(*this, Items, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Non-consumable rejects"), !Items->BindHotbarSlot(1, Weapon).bSuccess);
	TestTrue(TEXT("Unknown identity rejects"), !Items->BindHotbarSlot(1, FGuid::NewGuid()).bSuccess);
	Fdemo_mapItemInstance Zero;
	Zero.InstanceId = FGuid::NewGuid();
	Zero.DefinitionId = Fdemo_mapItemIds::HealingPillLevel1;
	Zero.OwnershipState = Edemo_mapItemOwnershipState::Inventory;
	Zero.Quantity = 0;
	TestTrue(TEXT("Zero quantity rejects through shared rule"), !Fdemo_mapItemViewRules::IsHotbarBindable(Zero, *Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::HealingPillLevel1)));
	const FGuid Pill = GridAddOne(*this, Items, Fdemo_mapItemIds::HealingPillLevel1);
	TestTrue(TEXT("Bind then destroy"), Items->BindHotbarSlot(1, Pill).bSuccess && Items->Destroy(Pill).bSuccess);
	TestTrue(TEXT("Destroyed binding is cleared"), !Items->GetHotbarBindingSnapshot().SlotBindings[0].IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory23HotbarStaleCleanup, "demo_map.GridInventory.23.HotbarStaleCleanupPure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory23HotbarStaleCleanup::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	Udemo_mapItemSubsystem* Items = NewObject<Udemo_mapItemSubsystem>(GameInstance);
	const FGuid Pill = GridAddOne(*this, Items, Fdemo_mapItemIds::HealingPillLevel1);
	TestTrue(TEXT("Bind consumable"), Items->BindHotbarSlot(4, Pill).bSuccess);
	Fdemo_mapItemAuthority& Authority = const_cast<Fdemo_mapItemAuthority&>(Items->GetAuthority());
	TestTrue(TEXT("Create stale binding without using the hotbar API"), Authority.Destroy(Pill).bSuccess);
	const Fdemo_mapItemAuthorityState BeforeRefresh = Authority.CaptureState();
	TestTrue(TEXT("Refresh clears one stale value"), Items->RefreshHotbarBindings() == 1 && !Items->GetHotbarBindingSnapshot().SlotBindings[3].IsValid());
	TestTrue(TEXT("Stale cleanup leaves Item Authority unchanged"), GridStateEqual(BeforeRefresh, Authority.CaptureState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory24SchemaAndCurrency, "demo_map.GridInventory.24.SchemaCurrencyAndSaveIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory24SchemaAndCurrency::RunTest(const FString&)
{
	bool bSpiritStoneDefinitionAbsent = true;
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
	{
		bSpiritStoneDefinitionAbsent &= !Definition.DefinitionId.ToString().Contains(TEXT("SpiritStone"));
	}
	Fdemo_mapItemAuthority Authority;
	const Fdemo_mapGridContainerSnapshot Grid = Authority.BuildRunInventoryGridSnapshot();
	TestTrue(TEXT("Spirit Stone has no Registry Grid Equipment or Hotbar identity"), bSpiritStoneDefinitionAbsent && Grid.bValid && Grid.UsedSlots == 0 && !Authority.GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot).IsValid());
	TestEqual(TEXT("Profile Schema is three"), Fdemo_mapPersistentProfile::CurrentSchemaVersion, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory25PreparationCounts, "demo_map.GridInventory.25.PreparationFourAndNine", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory25PreparationCounts::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	Snapshot.OrderedPermanentStashRows = {
		GridRow(FGuid::NewGuid(), Fdemo_mapItemIds::TrainingBlade),
		GridRow(FGuid::NewGuid(), Fdemo_mapItemIds::HealingPillLevel1, 3)
	};
	const Fdemo_mapProfilePreparationViewState View = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	TestTrue(TEXT("Preparation expresses four equipment slots and nine hotbar positions"), View.OrderedEquipmentSlots.Num() == 4 && View.HotbarBindings.SlotBindings.Num() == 9);
	TestTrue(TEXT("Permanent Stash is an ordered 1x1 grid projection"), View.PermanentStashGrid.bValid && View.PermanentStashGrid.UsedSlots == 2 && View.PermanentStashGrid.Capacity == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory26PreparationCapacity, "demo_map.GridInventory.26.PreparationUsedCapacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory26PreparationCapacity::RunTest(const FString&)
{
	const FGuid Material = FGuid::NewGuid();
	const FGuid Level1 = FGuid::NewGuid();
	const FGuid Level2 = FGuid::NewGuid();
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	Snapshot.OrderedPermanentStashRows = {
		GridRow(Material, Fdemo_mapItemIds::SpiritDust, 5),
		GridRow(Level1, Fdemo_mapItemIds::BackpackLevel1),
		GridRow(Level2, Fdemo_mapItemIds::BackpackLevel2)
	};
	Snapshot.OrderedSelectedMaterialIds = { Material };
	const Fdemo_mapProfilePreparationViewState Base = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	Snapshot.SelectedBackpackId = Level1;
	const Fdemo_mapProfilePreparationViewState Ten = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	Snapshot.SelectedBackpackId = Level2;
	const Fdemo_mapProfilePreparationViewState Fourteen = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	TestTrue(TEXT("Preparation displays used over 6 16 20"), Base.RunInventoryUsedSlots == 1 && Base.RunInventoryCapacity == 6 && Ten.RunInventoryCapacity == 16 && Fourteen.RunInventoryCapacity == 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory27PresenterPure, "demo_map.GridInventory.27.PresenterPureProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory27PresenterPure::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	Snapshot.ProfileId = FGuid::NewGuid();
	Snapshot.SaveGeneration = 17;
	Snapshot.OrderedPermanentStashRows = { GridRow(FGuid::NewGuid(), Fdemo_mapItemIds::TrainingBlade) };
	const Fdemo_mapProfilePreparationViewState First = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	const Fdemo_mapProfilePreparationViewState Second = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	TestTrue(TEXT("Presenter rebuild is deterministic"), First.PermanentStashGrid == Second.PermanentStashGrid && First.RunInventoryGrid == Second.RunInventoryGrid && First.HotbarBindings == Second.HotbarBindings);
	TestTrue(TEXT("Presenter does not mutate Profile identity or Generation"), Snapshot.ProfileId.IsValid() && Snapshot.SaveGeneration == 17);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapGridInventory28P2DropTransaction, "demo_map.P2.ItemDragDrop.RuntimeTransaction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapGridInventory28P2DropTransaction::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const FGuid Blade = GridAddOne(*this, Authority, Fdemo_mapItemIds::TrainingBlade);
	Fdemo_mapPlayerItemDropIntent Equip;
	Equip.ExpectedAuthorityRevision = Authority.GetAuthorityRevision();
	Equip.ExpectedSourceItemInstanceId = Blade;
	Equip.SourceArea = Edemo_mapPlayerItemArea::BaseQuickItems;
	Equip.SourceSlotIndex = 0;
	Equip.TargetArea = Edemo_mapPlayerItemArea::Equipment;
	Equip.TargetEquipmentSlotId = Fdemo_mapItemIds::WeaponSlot;
	const Fdemo_mapPlayerItemDropResult Equipped = Authority.ExecutePlayerItemDrop(Equip);
	TestTrue(TEXT("P2 inventory-to-equipment drag is one committed Equip"),
		Equipped.IsSuccess()
		&& Equipped.Kind == Edemo_mapPlayerItemDropKind::Equip
		&& Authority.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == Blade);

	Fdemo_mapPlayerItemDropIntent Stale = Equip;
	Stale.ExpectedAuthorityRevision = Equip.ExpectedAuthorityRevision;
	const Fdemo_mapItemAuthorityState BeforeStale = Authority.CaptureState();
	const Fdemo_mapPlayerItemDropResult Rejected = Authority.ExecutePlayerItemDrop(Stale);
	TestTrue(TEXT("P2 stale drag rejects without mutation"),
		!Rejected.IsSuccess()
		&& Rejected.Kind == Edemo_mapPlayerItemDropKind::Reject
		&& GridStateEqual(BeforeStale, Authority.CaptureState()));

	Fdemo_mapItemAuthority Stacks;
	const Fdemo_mapItemDefinition* Dust =
		Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::SpiritDust);
	TestNotNull(TEXT("Stack definition exists"), Dust);
	if (!Dust) return false;
	Stacks.AddDefinition(Fdemo_mapItemIds::SpiritDust, Dust->MaxStackSize);
	Stacks.AddDefinition(Fdemo_mapItemIds::SpiritDust, 1);
	const TArray<FGuid>& Slots = Stacks.GetInventorySlotSnapshot();
	Fdemo_mapPlayerItemDropIntent Merge;
	Merge.ExpectedAuthorityRevision = Stacks.GetAuthorityRevision();
	Merge.ExpectedSourceItemInstanceId = Slots[0];
	Merge.SourceArea = Edemo_mapPlayerItemArea::BaseQuickItems;
	Merge.SourceSlotIndex = 0;
	Merge.TargetArea = Edemo_mapPlayerItemArea::BaseQuickItems;
	Merge.TargetSlotIndex = 1;
	const Fdemo_mapPlayerItemDropResult Merged = Stacks.ExecutePlayerItemDrop(Merge);
	TestTrue(TEXT("P2 compatible partial merge retains target GUID"),
		Merged.IsSuccess()
		&& Merged.Kind == Edemo_mapPlayerItemDropKind::Merge
		&& Merged.TargetItemInstanceId == Slots[1]
		&& Stacks.FindInstance(Slots[1])->Quantity == Dust->MaxStackSize);
	return true;
}

#endif
