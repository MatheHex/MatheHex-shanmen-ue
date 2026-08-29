#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
	float ItemTestAttribute(const Udemo_mapAttributeComponent* Attributes, FName AttributeId)
	{
		float Value = 0.0f;
		Attributes->GetFinalValue(AttributeId, Value);
		return Value;
	}

	FGuid AddOne(FAutomationTestBase& Test, Fdemo_mapItemAuthority& Authority, FName DefinitionId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result = Authority.AddDefinition(DefinitionId, 1, &Affected);
		Test.TestTrue(FString::Printf(TEXT("Add %s"), *DefinitionId.ToString()), Result.bSuccess && Affected.Num() == 1 && Affected[0].IsValid());
		return Affected.Num() == 1 ? Affected[0] : FGuid();
	}

	FGuid AddOne(FAutomationTestBase& Test, Udemo_mapItemSubsystem* Items, FName DefinitionId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result = Items->AddDefinition(DefinitionId, 1, &Affected);
		Test.TestTrue(FString::Printf(TEXT("Subsystem add %s"), *DefinitionId.ToString()), Result.bSuccess && Affected.Num() == 1 && Affected[0].IsValid());
		return Affected.Num() == 1 ? Affected[0] : FGuid();
	}

	int32 TotalDefinitionQuantity(const Fdemo_mapItemAuthority& Authority, FName DefinitionId)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapItemDefinitionsTest, "demo_map.V3.Items.DefinitionRegistry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapItemDefinitionsTest::RunTest(const FString&)
{
	FString Error;
	TestTrue(TEXT("Registry validates"), Fdemo_mapItemDefinitions::Validate(&Error));
	TestEqual(TEXT("Current catalog contains legacy and 0.0.10 definitions"), Fdemo_mapItemDefinitions::GetAll().Num(), 40);
	TestEqual(TEXT("Five stable slots"), Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num(), 5);
	const Fdemo_mapItemDefinition* Weapon = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapItemDefinition* Armor = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingVest);
	const Fdemo_mapItemDefinition* Accessory = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::EvasionCharm);
	const Fdemo_mapItemDefinition* SpatialRing = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::WindTalisman);
	const Fdemo_mapItemDefinition* Material = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::SpiritDust);
	const Fdemo_mapItemDefinition* HeavyWeapon = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::HeavyPracticeBlade);
	const Fdemo_mapItemDefinition* ReinforcedArmor = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::ReinforcedVest);
	const Fdemo_mapItemDefinition* Iron = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::IronShard);
	const Fdemo_mapItemDefinition* Token = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::AncientToken);
	const Fdemo_mapItemDefinition* SpiritGuard = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::SpiritGuardRobe);
	const Fdemo_mapItemDefinition* ThrowingKnife = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingThrowingKnife);
	TestTrue(TEXT("Required definitions exist"), Weapon && Armor && Accessory && SpatialRing && Material && HeavyWeapon && ReinforcedArmor && Iron && Token && SpiritGuard && ThrowingKnife);
	if (!Weapon || !Armor || !Accessory || !SpatialRing || !Material || !HeavyWeapon || !ReinforcedArmor || !Iron || !Token || !SpiritGuard || !ThrowingKnife) return false;
	TestTrue(TEXT("Weapon compatibility"), Weapon->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::WeaponSlot });
	TestTrue(TEXT("Armor compatibility"), Armor->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::ArmorSlot });
	TestTrue(TEXT("Accessory compatibility"), Accessory->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::AccessorySlot });
	TestTrue(TEXT("Spatial ring compatibility"), SpatialRing->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::SpatialRingSlot });
	TestTrue(TEXT("Material is not equipable"), Material->CompatibleSlotIds.IsEmpty() && Material->Modifiers.IsEmpty());
	TestEqual(TEXT("SpiritDust max stack"), Material->MaxStackSize, 5);
	TestTrue(TEXT("New non-equipment loot remains non-equipable"), Iron->CompatibleSlotIds.IsEmpty() && Token->CompatibleSlotIds.IsEmpty() && Token->PrototypeValue == 500);
	TestTrue(TEXT("Spirit Guard owns explicit durability content"), SpiritGuard->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::ArmorSlot } && SpiritGuard->MaxDurability == 20 && SpiritGuard->MaxCharges == 0 && SpiritGuard->MaxStackSize == 1);
	TestTrue(TEXT("Training throwing knife owns exact typed product semantics"), ThrowingKnife->CategoryId == Fdemo_mapItemIds::ConsumableCategory && ThrowingKnife->MaxStackSize == 20 && ThrowingKnife->bHotbarEligible && ThrowingKnife->HasGameplaySemantic(Edemo_mapItemGameplaySemantic::ThrownWeapon) && ThrowingKnife->GameplaySemantics.Num() == 1 && ThrowingKnife->CompatibleSlotIds.IsEmpty());
	TestTrue(TEXT("Display name is not a key"), Fdemo_mapItemDefinitions::Find(FName(*Weapon->DisplayName.ToString())) == nullptr);
	TestTrue(TEXT("Deterministic registry order"), Fdemo_mapItemDefinitions::GetAll()[0].DefinitionId == Fdemo_mapItemIds::TrainingBlade && Fdemo_mapItemDefinitions::GetAll()[3].DefinitionId == Fdemo_mapItemIds::SpiritDust && Fdemo_mapItemDefinitions::GetAll()[8].DefinitionId == Fdemo_mapItemIds::AncientToken);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapItemInventoryTest, "demo_map.V3.Items.InventoryStackingAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapItemInventoryTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Stacks;
	TArray<FGuid> Affected;
	TestTrue(TEXT("Add dust 3"), Stacks.AddDefinition(Fdemo_mapItemIds::SpiritDust, 3, &Affected).bSuccess);
	TestEqual(TEXT("One stack after 3"), Stacks.GetUsedInventorySlots(), 1);
	TestEqual(TEXT("Quantity after 3"), TotalDefinitionQuantity(Stacks, Fdemo_mapItemIds::SpiritDust), 3);
	TestTrue(TEXT("Add dust 4"), Stacks.AddDefinition(Fdemo_mapItemIds::SpiritDust, 4, &Affected).bSuccess);
	const TArray<FGuid> DustStacks = Stacks.FindInventoryInstancesByDefinition(Fdemo_mapItemIds::SpiritDust);
	TestEqual(TEXT("Two stacks after total 7"), DustStacks.Num(), 2);
	TestTrue(TEXT("First stack filled deterministically"), DustStacks.Num() == 2 && Stacks.FindInstance(DustStacks[0])->Quantity == 5 && Stacks.FindInstance(DustStacks[1])->Quantity == 2);
	TestEqual(TEXT("Total dust 7"), TotalDefinitionQuantity(Stacks, Fdemo_mapItemIds::SpiritDust), 7);
	TestTrue(TEXT("Fill remaining four slots"), Stacks.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 4).bSuccess);
	const Fdemo_mapItemAuthorityState BeforeOverflow = Stacks.CaptureState();
	const Fdemo_mapItemOperationResult Overflow = Stacks.AddDefinition(Fdemo_mapItemIds::SpiritDust, 4);
	TestTrue(TEXT("Overflow returns InventoryFull"), !Overflow.bSuccess && Overflow.Code == Edemo_mapItemResultCode::InventoryFull);
	TestTrue(TEXT("Overflow slots unchanged"), Stacks.GetInventorySlotSnapshot() == BeforeOverflow.InventorySlots);
	TestEqual(TEXT("Overflow instances unchanged"), Stacks.GetInstanceSnapshot().Num(), BeforeOverflow.Instances.Num());
	TestEqual(TEXT("Overflow stack quantities unchanged"), TotalDefinitionQuantity(Stacks, Fdemo_mapItemIds::SpiritDust), 7);

	Fdemo_mapItemAuthority Capacity;
	TestEqual(TEXT("Base capacity fixed 6"), Capacity.GetInventoryCapacity(), 6);
	TestTrue(TEXT("Add six nonstack items"), Capacity.AddDefinition(Fdemo_mapItemIds::TrainingBlade, 6).bSuccess);
	const Fdemo_mapItemAuthorityState FullState = Capacity.CaptureState();
	const Fdemo_mapItemOperationResult Seventh = Capacity.AddDefinition(Fdemo_mapItemIds::TrainingVest, 1);
	TestTrue(TEXT("Seventh rejected"), !Seventh.bSuccess && Seventh.Code == Edemo_mapItemResultCode::InventoryFull);
	TestTrue(TEXT("Full inventory unchanged"), Capacity.GetInventorySlotSnapshot() == FullState.InventorySlots && Capacity.GetInstanceSnapshot().Num() == FullState.Instances.Num());

	Fdemo_mapItemAuthority Identity;
	TSet<FGuid> UniqueIds;
	for (int32 Index = 0; Index < 100; ++Index)
	{
		const FGuid InstanceId = AddOne(*this, Identity, Fdemo_mapItemIds::TrainingBlade);
		TestTrue(TEXT("GUID unique and nonzero"), InstanceId.IsValid() && !UniqueIds.Contains(InstanceId));
		UniqueIds.Add(InstanceId);
		TestTrue(TEXT("Destroy for next identity iteration"), Identity.Destroy(InstanceId).bSuccess);
	}
	TestEqual(TEXT("100 unique GUIDs"), UniqueIds.Num(), 100);
	FString Error;
	TestTrue(TEXT("Identity invariants"), Identity.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapItemEquipmentTest, "demo_map.V3.Items.EquipmentTransactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapItemEquipmentTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Items;
	const FGuid Weapon = AddOne(*this, Items, Fdemo_mapItemIds::TrainingBlade);
	const FGuid Armor = AddOne(*this, Items, Fdemo_mapItemIds::TrainingVest);
	const FGuid Material = AddOne(*this, Items, Fdemo_mapItemIds::SpiritDust);
	const TArray<FGuid> BeforeInvalid = Items.GetInventorySlotSnapshot();
	TestTrue(TEXT("Weapon to Armor fails"), Items.Equip(Weapon, Fdemo_mapItemIds::ArmorSlot).Code == Edemo_mapItemResultCode::IncompatibleSlot);
	TestTrue(TEXT("Material cannot equip"), Items.Equip(Material, Fdemo_mapItemIds::WeaponSlot).Code == Edemo_mapItemResultCode::NotEquipable);
	TestTrue(TEXT("Invalid equip does not change inventory"), Items.GetInventorySlotSnapshot() == BeforeInvalid);
	TestTrue(TEXT("Weapon equip"), Items.Equip(Weapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Same ID moved to equipment"), Items.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == Weapon && Items.FindInventorySlot(Weapon) == INDEX_NONE && Items.FindInstance(Weapon)->OwnershipState == Edemo_mapItemOwnershipState::Equipped);
	TestTrue(TEXT("Armor equip"), Items.Equip(Armor, Fdemo_mapItemIds::ArmorSlot).bSuccess);
	TestTrue(TEXT("Weapon unequip"), Items.Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Weapon same ID returned"), Items.FindInventorySlot(Weapon) != INDEX_NONE && Items.FindInstance(Weapon)->OwnershipState == Edemo_mapItemOwnershipState::Inventory);
	TestTrue(TEXT("Destroy inventory weapon"), Items.Destroy(Weapon).bSuccess);
	TestTrue(TEXT("Destroyed state inactive"), Items.FindInstance(Weapon)->OwnershipState == Edemo_mapItemOwnershipState::Destroyed && Items.FindInventorySlot(Weapon) == INDEX_NONE);
	TestTrue(TEXT("Destroyed cannot equip"), Items.Equip(Weapon, Fdemo_mapItemIds::WeaponSlot).Code == Edemo_mapItemResultCode::AlreadyDestroyed);

	Fdemo_mapItemAuthority FullUnequip;
	const FGuid EquippedWeapon = AddOne(*this, FullUnequip, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Equip before filling"), FullUnequip.Equip(EquippedWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Fill all six base inventory slots"), FullUnequip.AddDefinition(Fdemo_mapItemIds::TrainingVest, 6).bSuccess);
	const Fdemo_mapItemAuthorityState BeforeUnequip = FullUnequip.CaptureState();
	const Fdemo_mapItemOperationResult UnequipFull = FullUnequip.Unequip(Fdemo_mapItemIds::WeaponSlot);
	TestTrue(TEXT("Full unequip rejected"), !UnequipFull.bSuccess && UnequipFull.Code == Edemo_mapItemResultCode::InventoryFull);
	TestTrue(TEXT("Full unequip state unchanged"), FullUnequip.GetInventorySlotSnapshot() == BeforeUnequip.InventorySlots && FullUnequip.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == EquippedWeapon);

	Fdemo_mapItemAuthority Replacement;
	const FGuid OldWeapon = AddOne(*this, Replacement, Fdemo_mapItemIds::TrainingBlade);
	const FGuid NewWeapon = AddOne(*this, Replacement, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Equip old weapon"), Replacement.Equip(OldWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Fill replacement inventory to 6"), Replacement.AddDefinition(Fdemo_mapItemIds::TrainingVest, 5).bSuccess);
	const int32 NewWeaponSlot = Replacement.FindInventorySlot(NewWeapon);
	TestTrue(TEXT("Full inventory atomic replacement"), Replacement.Equip(NewWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("New equipped, old occupies released slot"), Replacement.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == NewWeapon && Replacement.GetInventorySlotSnapshot()[NewWeaponSlot] == OldWeapon && Replacement.GetUsedInventorySlots() == 6);
	const Fdemo_mapItemAuthorityState BeforeFailedReplace = Replacement.CaptureState();
	const FGuid IncompatibleArmor = Replacement.FindInventoryInstancesByDefinition(Fdemo_mapItemIds::TrainingVest)[0];
	TestTrue(TEXT("Incompatible replacement rejected"), Replacement.Equip(IncompatibleArmor, Fdemo_mapItemIds::WeaponSlot).Code == Edemo_mapItemResultCode::IncompatibleSlot);
	TestTrue(TEXT("Failed replacement rollback exact"), Replacement.GetInventorySlotSnapshot() == BeforeFailedReplace.InventorySlots && Replacement.GetEquipmentSlotSnapshot().OrderIndependentCompareEqual(BeforeFailedReplace.EquipmentSlots));
	FString Error;
	TestTrue(TEXT("Replacement invariants"), Replacement.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapItemModifierBridgeTest, "demo_map.V3.Items.ModifierBridgeAndRebind", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapItemModifierBridgeTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	Udemo_mapItemSubsystem* Items = NewObject<Udemo_mapItemSubsystem>(GameInstance);
	Udemo_mapAttributeComponent* FirstAttributes = NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* SecondAttributes = NewObject<Udemo_mapAttributeComponent>();
	TestTrue(TEXT("Bind first attributes"), Items->BindAttributeComponent(FirstAttributes));
	const FGuid Weapon = AddOne(*this, Items, Fdemo_mapItemIds::TrainingBlade);
	const FGuid Armor = AddOne(*this, Items, Fdemo_mapItemIds::TrainingVest);
	const FGuid Accessory = AddOne(*this, Items, Fdemo_mapItemIds::WindTalisman);
	TestTrue(TEXT("Equip three slots"), Items->Equip(Weapon, Fdemo_mapItemIds::WeaponSlot).bSuccess && Items->Equip(Armor, Fdemo_mapItemIds::ArmorSlot).bSuccess && Items->Equip(Accessory, Fdemo_mapItemIds::SpatialRingSlot).bSuccess);
	TestTrue(TEXT("Weapon modifier"), FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f));
	TestTrue(TEXT("Armor modifier"), FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::MaxHealth), 7.0f));
	TestTrue(TEXT("Accessory modifier"), FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::MoveSpeed), 660.0f));
	TestEqual(TEXT("Three unique equipment sources"), Items->GetActiveModifierSources().Num(), 3);
	TestTrue(TEXT("Repeated synchronization idempotent"), Items->SynchronizeEquipmentModifiers() && Items->SynchronizeEquipmentModifiers() && FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f) && FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::MoveSpeed), 660.0f));

	Fdemo_mapModifierSpec External; External.SourceId = TEXT("Automation.PersistentSource"); External.AttributeId = Fdemo_mapAttributeIds::AttackPower; External.Operation = Edemo_mapModifierOperation::Add; External.Value = 5.0f;
	Fdemo_mapModifierHandle ExternalHandle;
	TestTrue(TEXT("Add non-equipment source"), FirstAttributes->AddModifier(External, ExternalHandle));
	TestTrue(TEXT("Unequip weapon preserves external source"), Items->Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess && FirstAttributes->GetModifierCountBySource(External.SourceId) == 1 && FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::AttackPower), 6.0f));
	TestTrue(TEXT("Re-equip weapon"), Items->Equip(Weapon, Fdemo_mapItemIds::WeaponSlot).bSuccess && FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::AttackPower), 7.0f));
	TestTrue(TEXT("Bind replacement attributes"), Items->BindAttributeComponent(SecondAttributes));
	TestTrue(TEXT("Old attributes lose equipment only"), FirstAttributes->GetActiveModifierCount() == 1 && FirstAttributes->GetModifierCountBySource(External.SourceId) == 1 && FMath::IsNearlyEqual(ItemTestAttribute(FirstAttributes, Fdemo_mapAttributeIds::AttackPower), 6.0f));
	TestTrue(TEXT("New attributes receive one set"), FMath::IsNearlyEqual(ItemTestAttribute(SecondAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f) && FMath::IsNearlyEqual(ItemTestAttribute(SecondAttributes, Fdemo_mapAttributeIds::MaxHealth), 7.0f) && FMath::IsNearlyEqual(ItemTestAttribute(SecondAttributes, Fdemo_mapAttributeIds::MoveSpeed), 660.0f));
	TestTrue(TEXT("Duplicate replacement bind idempotent"), Items->BindAttributeComponent(SecondAttributes) && Items->GetActiveModifierSources().Num() == 3 && SecondAttributes->GetActiveModifierCount() == 3);
	TestTrue(TEXT("Unequip one source only"), Items->Unequip(Fdemo_mapItemIds::SpatialRingSlot).bSuccess && FMath::IsNearlyEqual(ItemTestAttribute(SecondAttributes, Fdemo_mapAttributeIds::MoveSpeed), 600.0f) && FMath::IsNearlyEqual(ItemTestAttribute(SecondAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f) && FMath::IsNearlyEqual(ItemTestAttribute(SecondAttributes, Fdemo_mapAttributeIds::MaxHealth), 7.0f));
	FString Error;
	TestTrue(TEXT("Bridge invariants"), Items->ValidateInvariants(&Error));

	UGameInstance* ReplacementGameInstance = NewObject<UGameInstance>();
	Udemo_mapItemSubsystem* ReplacementItems = NewObject<Udemo_mapItemSubsystem>(ReplacementGameInstance);
	Udemo_mapAttributeComponent* ReplacementAttributes = NewObject<Udemo_mapAttributeComponent>();
	ReplacementItems->BindAttributeComponent(ReplacementAttributes);
	const FGuid OldWeapon = AddOne(*this, ReplacementItems, Fdemo_mapItemIds::TrainingBlade);
	const FGuid NewWeapon = AddOne(*this, ReplacementItems, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Bridge equip old weapon"), ReplacementItems->Equip(OldWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestTrue(TEXT("Bridge fill inventory"), ReplacementItems->AddDefinition(Fdemo_mapItemIds::TrainingVest, 5).bSuccess && ReplacementItems->GetAuthority().GetUsedInventorySlots() == 6);
	const FName OldSource = Udemo_mapItemSubsystem::MakeModifierSourceId(OldWeapon);
	const FName NewSource = Udemo_mapItemSubsystem::MakeModifierSourceId(NewWeapon);
	TestTrue(TEXT("Full inventory replacement preserves one effect"), ReplacementItems->Equip(NewWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess && FMath::IsNearlyEqual(ItemTestAttribute(ReplacementAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f) && ReplacementItems->GetActiveModifierSources().Num() == 1 && ReplacementAttributes->GetModifierCountBySource(OldSource) == 0 && ReplacementAttributes->GetModifierCountBySource(NewSource) == 1);
	const Fdemo_mapItemOperationResult FullUnequip = ReplacementItems->Unequip(Fdemo_mapItemIds::WeaponSlot);
	TestTrue(TEXT("Full inventory unequip preserves modifier"), !FullUnequip.bSuccess && FullUnequip.Code == Edemo_mapItemResultCode::InventoryFull && ReplacementItems->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == NewWeapon && FMath::IsNearlyEqual(ItemTestAttribute(ReplacementAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f) && ReplacementAttributes->GetModifierCountBySource(NewSource) == 1);
	const FGuid IncompatibleArmor = ReplacementItems->GetAuthority().FindInventoryInstancesByDefinition(Fdemo_mapItemIds::TrainingVest)[0];
	TestTrue(TEXT("Failed replacement keeps source set"), ReplacementItems->Equip(IncompatibleArmor, Fdemo_mapItemIds::WeaponSlot).Code == Edemo_mapItemResultCode::IncompatibleSlot && ReplacementItems->GetActiveModifierSources().Num() == 1 && ReplacementAttributes->GetModifierCountBySource(NewSource) == 1 && FMath::IsNearlyEqual(ItemTestAttribute(ReplacementAttributes, Fdemo_mapAttributeIds::AttackPower), 2.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapItemStressTest, "demo_map.V3.Items.OwnershipInvariantStress", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapItemStressTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Items;
	TSet<FGuid> AllIds;
	int32 Operations = 0;
	auto Verify = [this, &Items, &Operations](const TCHAR* Context)
	{
		++Operations;
		FString Error;
		return TestTrue(FString::Printf(TEXT("Stress invariant after %s operation %d"), Context, Operations), Items.ValidateInvariants(&Error));
	};
	for (int32 Round = 0; Round < 10; ++Round)
	{
		TArray<FGuid> DustIds;
		TestTrue(TEXT("Stress stack create"), Items.AddDefinition(Fdemo_mapItemIds::SpiritDust, 3, &DustIds).bSuccess && DustIds.Num() == 1); Verify(TEXT("stack-create"));
		TestTrue(TEXT("Stress stack fill"), Items.AddDefinition(Fdemo_mapItemIds::SpiritDust, 2).bSuccess && Items.FindInstance(DustIds[0])->Quantity == 5); Verify(TEXT("stack-fill"));
		TestTrue(TEXT("Stress stack destroy"), Items.Destroy(DustIds[0]).bSuccess); Verify(TEXT("stack-destroy"));
		const FGuid OldWeapon = AddOne(*this, Items, Fdemo_mapItemIds::TrainingBlade); TestTrue(TEXT("Stress old GUID unique"), !AllIds.Contains(OldWeapon)); AllIds.Add(OldWeapon); Verify(TEXT("old-create"));
		TestTrue(TEXT("Stress equip old"), Items.Equip(OldWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess); Verify(TEXT("old-equip"));
		const FGuid NewWeapon = AddOne(*this, Items, Fdemo_mapItemIds::TrainingBlade); TestTrue(TEXT("Stress new GUID unique"), !AllIds.Contains(NewWeapon)); AllIds.Add(NewWeapon); Verify(TEXT("new-create"));
		TestTrue(TEXT("Stress replace"), Items.Equip(NewWeapon, Fdemo_mapItemIds::WeaponSlot).bSuccess && Items.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) == NewWeapon); Verify(TEXT("replace"));
		TestTrue(TEXT("Stress unequip new"), Items.Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess); Verify(TEXT("unequip"));
		TestTrue(TEXT("Stress destroy new"), Items.Destroy(NewWeapon).bSuccess); Verify(TEXT("new-destroy"));
		TestTrue(TEXT("Stress destroy old"), Items.Destroy(OldWeapon).bSuccess); Verify(TEXT("old-destroy"));
	}
	TestEqual(TEXT("At least 100 deterministic operations"), Operations, 100);
	TestEqual(TEXT("No active inventory residue"), Items.GetUsedInventorySlots(), 0);
	TestTrue(TEXT("No equipment residue"), !Items.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot).IsValid());
	TestEqual(TEXT("Unique destroyed registry entries"), Items.GetInstanceSnapshot().Num(), 30);
	return true;
}

#endif
