#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	int32 TotalInventoryQuantity(const Fdemo_mapItemAuthority& Authority, FName DefinitionId)
	{
		int32 Total = 0;
		for (const FGuid& InstanceId : Authority.GetInventorySlotSnapshot())
		{
			const Fdemo_mapItemInstance* Instance = Authority.FindInstance(InstanceId);
			if (Instance && Instance->DefinitionId == DefinitionId) Total += Instance->Quantity;
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapV3DefinitionExtensionTest, "demo_map.V3.WorldInteraction.DefinitionExtension", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapV3DefinitionExtensionTest::RunTest(const FString&)
{
	TestEqual(TEXT("Current catalog definitions"), Fdemo_mapItemDefinitions::GetAll().Num(), 41);
	const Fdemo_mapItemDefinition* Weapon = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::HeavyPracticeBlade);
	const Fdemo_mapItemDefinition* Armor = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::ReinforcedVest);
	const Fdemo_mapItemDefinition* Accessory = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::EvasionCharm);
	const Fdemo_mapItemDefinition* Iron = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::IronShard);
	const Fdemo_mapItemDefinition* Token = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::AncientToken);
	TestTrue(TEXT("All five extension definitions"), Weapon && Armor && Accessory && Iron && Token);
	if (!Weapon || !Armor || !Accessory || !Iron || !Token) return false;
	TestTrue(TEXT("Heavy weapon prototype"), Weapon->MaxStackSize == 1 && Weapon->CompatibleSlotIds == TArray<FName>{Fdemo_mapItemIds::WeaponSlot} && Weapon->Modifiers.Num() == 1 && FMath::IsNearlyEqual(Weapon->Modifiers[0].Value, 2.0f) && Weapon->PrototypeValue == 180);
	TestTrue(TEXT("Reinforced armor prototype"), Armor->CompatibleSlotIds == TArray<FName>{Fdemo_mapItemIds::ArmorSlot} && Armor->Modifiers.Num() == 1 && FMath::IsNearlyEqual(Armor->Modifiers[0].Value, 3.0f) && Armor->PrototypeValue == 150);
	TestTrue(TEXT("Evasion accessory prototype"), Accessory->CompatibleSlotIds == TArray<FName>{Fdemo_mapItemIds::AccessorySlot} && Accessory->Modifiers.Num() == 1 && FMath::IsNearlyEqual(Accessory->Modifiers[0].Value, 0.10f) && Accessory->PrototypeValue == 120);
	TestTrue(TEXT("Materials and loot"), Iron->MaxStackSize == 5 && Iron->PrototypeValue == 3 && Token->MaxStackSize == 1 && Token->PrototypeValue == 500 && Token->CompatibleSlotIds.IsEmpty());
	TSet<FString> WorldLabels;
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
	{
		bool bAscii = !Definition.WorldLabelName.IsEmpty();
		for (TCHAR Character : Definition.WorldLabelName) bAscii = bAscii && Character <= 127;
		TestTrue(TEXT("World label uses explicit ASCII fallback"), bAscii);
		WorldLabels.Add(Definition.WorldLabelName);
	}
	TestEqual(TEXT("World labels are one-to-one with definitions"), WorldLabels.Num(), Fdemo_mapItemDefinitions::GetAll().Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapV3WorldOwnershipTest, "demo_map.V3.WorldInteraction.WorldOwnershipAndPickup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapV3WorldOwnershipTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid WorldId;
	TestTrue(TEXT("Create world item"), Authority.CreateWorldDefinition(Fdemo_mapItemIds::HeavyPracticeBlade, 1, WorldId).bSuccess && WorldId.IsValid());
	const Fdemo_mapItemInstance* World = Authority.FindInstance(WorldId);
	TestTrue(TEXT("World metadata"), World && World->OwnershipState == Edemo_mapItemOwnershipState::World && World->ContainerId == Fdemo_mapItemIds::WorldContainer && Authority.FindInventorySlot(WorldId) == INDEX_NONE);
	TestTrue(TEXT("Pickup same GUID"), Authority.PickupWorld(WorldId).bSuccess && Authority.FindInventorySlot(WorldId) != INDEX_NONE);
	const Fdemo_mapItemInstance* Inventory = Authority.FindInstance(WorldId);
	TestTrue(TEXT("Inventory metadata"), Inventory && Inventory->OwnershipState == Edemo_mapItemOwnershipState::Inventory && Inventory->ContainerId == Fdemo_mapItemIds::InventoryContainer);
	FString Error;
	TestTrue(TEXT("Final invariant"), Authority.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapV3StackPickupTest, "demo_map.V3.WorldInteraction.AtomicStackPickup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapV3StackPickupTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TestTrue(TEXT("Existing dust four"), Authority.AddDefinition(Fdemo_mapItemIds::SpiritDust, 4).bSuccess);
	const FGuid ExistingId = Authority.FindInventoryInstancesByDefinition(Fdemo_mapItemIds::SpiritDust)[0];
	FGuid WorldId;
	TestTrue(TEXT("World dust three"), Authority.CreateWorldDefinition(Fdemo_mapItemIds::SpiritDust, 3, WorldId).bSuccess);
	TestTrue(TEXT("Atomic pickup"), Authority.PickupWorld(WorldId).bSuccess);
	TestEqual(TEXT("Existing stack filled"), Authority.FindInstance(ExistingId)->Quantity, 5);
	TestTrue(TEXT("Same world instance carries remainder"), Authority.FindInstance(WorldId)->OwnershipState == Edemo_mapItemOwnershipState::Inventory && Authority.FindInstance(WorldId)->Quantity == 2);
	TestEqual(TEXT("Total quantity seven"), TotalInventoryQuantity(Authority, Fdemo_mapItemIds::SpiritDust), 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapV3FullPickupTest, "demo_map.V3.WorldInteraction.FullInventoryRollback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapV3FullPickupTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TestTrue(TEXT("Fill all base slots"), Authority.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 6).bSuccess && Authority.GetFreeInventorySlots() == 0);
	FGuid WorldId;
	TestTrue(TEXT("Create world item"), Authority.CreateWorldDefinition(Fdemo_mapItemIds::HeavyPracticeBlade, 1, WorldId).bSuccess);
	const int32 BeforeInstances = Authority.GetInstanceSnapshot().Num();
	const TArray<FGuid> BeforeSlots = Authority.GetInventorySlotSnapshot();
	const Fdemo_mapItemOperationResult Result = Authority.PickupWorld(WorldId);
	TestTrue(TEXT("InventoryFull result"), !Result.bSuccess && Result.Code == Edemo_mapItemResultCode::InventoryFull);
	TestTrue(TEXT("World item unchanged"), Authority.FindInstance(WorldId)->OwnershipState == Edemo_mapItemOwnershipState::World && Authority.FindInstance(WorldId)->Quantity == 1);
	TestTrue(TEXT("Slots unchanged"), Authority.GetInventorySlotSnapshot() == BeforeSlots);
	TestEqual(TEXT("Registry unchanged"), Authority.GetInstanceSnapshot().Num(), BeforeInstances);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapV3DropAndDuplicateTest, "demo_map.V3.InventoryUI.DropRoundTripAndDuplicateClaim", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapV3DropAndDuplicateTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TArray<FGuid> Added;
	TestTrue(TEXT("Create inventory stack"), Authority.AddDefinition(Fdemo_mapItemIds::IronShard, 5, &Added).bSuccess && Added.Num() == 1);
	const FGuid InstanceId = Added[0];
	TestTrue(TEXT("Move same GUID to world"), Authority.MoveInventoryToWorld(InstanceId).bSuccess && Authority.FindInventorySlot(InstanceId) == INDEX_NONE);
	TestTrue(TEXT("Pickup same GUID"), Authority.PickupWorld(InstanceId).bSuccess && Authority.FindInventorySlot(InstanceId) != INDEX_NONE);
	const Fdemo_mapItemOperationResult Duplicate = Authority.PickupWorld(InstanceId);
	TestTrue(TEXT("Duplicate claim rejected deterministically"), !Duplicate.bSuccess && Duplicate.Code == Edemo_mapItemResultCode::AlreadyClaimed);
	TestEqual(TEXT("Only one stack remains"), Authority.FindInventoryInstancesByDefinition(Fdemo_mapItemIds::IronShard).Num(), 1);
	return true;
}

#endif
