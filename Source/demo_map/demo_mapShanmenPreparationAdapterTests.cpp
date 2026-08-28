#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenPreparationAdapter.h"

#include "ShanmenItemRepository.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemCutover.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewPreparationAdapterRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.6.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	void RemovePreparationAdapterRoot(const FString& Root)
	{
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	const FShanmenItemDefinition* FindAuthorityDefinition(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FName DefinitionId)
	{
		return Snapshot.Definitions.FindByPredicate([DefinitionId](
			const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == DefinitionId;
		});
	}

	const Fdemo_mapProfilePreparationStashRow* FindRow(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		const FGuid& ItemId)
	{
		return Snapshot.OrderedPermanentStashRows.FindByPredicate([&ItemId](
			const Fdemo_mapProfilePreparationStashRow& Row)
		{
			return Row.ItemInstanceId == ItemId;
		});
	}

	struct FPreparationAdapterFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid TrainingBladeId;
		FGuid HeavyBladeId;
		FGuid ArmorId;
		FGuid DustId;
		FGuid PillOneId;
		FGuid PillTwoId;
		FGuid BackpackId;
		TArray<FGuid> ExtraMaterialIds;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Fdemo_map0909BSectWarehouseService Warehouse;

		bool Seed(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewPreparationAdapterRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			for (const Fdemo_mapPersistentItemRecord& Item : SeedProfile.PermanentStash)
			{
				if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade)
				{
					TrainingBladeId = Item.ItemInstanceId;
				}
				else if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest)
				{
					ArmorId = Item.ItemInstanceId;
				}
			}
			Fdemo_mapPersistentItemRecord Heavy;
			HeavyBladeId = Heavy.ItemInstanceId = FGuid::NewGuid();
			Heavy.ItemDefinitionId = Fdemo_mapItemIds::HeavyPracticeBlade;
			Heavy.StackCount = 1;
			Heavy.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Heavy);

			Fdemo_mapPersistentItemRecord Dust;
			DustId = Dust.ItemInstanceId = FGuid::NewGuid();
			Dust.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust;
			Dust.StackCount = 3;
			Dust.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Dust);

			Fdemo_mapPersistentItemRecord PillOne;
			PillOneId = PillOne.ItemInstanceId = FGuid::NewGuid();
			PillOne.ItemDefinitionId = Fdemo_mapItemIds::HealingPillLevel1;
			PillOne.StackCount = 3;
			PillOne.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(PillOne);
			Fdemo_mapPersistentItemRecord PillTwo;
			PillTwoId = PillTwo.ItemInstanceId = FGuid::NewGuid();
			PillTwo.ItemDefinitionId = Fdemo_mapItemIds::HealingPillLevel2;
			PillTwo.StackCount = 2;
			PillTwo.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(PillTwo);
			Fdemo_mapPersistentItemRecord Backpack;
			BackpackId = Backpack.ItemInstanceId = FGuid::NewGuid();
			Backpack.ItemDefinitionId = Fdemo_mapItemIds::BackpackLevel1;
			Backpack.StackCount = 1;
			Backpack.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Backpack);
			for (int32 Index = 0; Index < 4; ++Index)
			{
				Fdemo_mapPersistentItemRecord Material;
				Material.ItemInstanceId = FGuid::NewGuid();
				Material.ItemDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
				Material.StackCount = Index + 1;
				Material.PersistentDomain =
					Edemo_mapPersistentDomain::PermanentStash;
				ExtraMaterialIds.Add(Material.ItemInstanceId);
				SeedProfile.PermanentStash.Add(Material);
			}
			SeedProfile.PreparationLayout.WeaponItemInstanceId = TrainingBladeId;
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!TrainingBladeId.IsValid() || !ArmorId.IsValid()
				|| !Saved.IsSuccess())
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 seed Profile failed: %s"), *Saved.Diagnostic));
				return false;
			}
			return true;
		}

		bool StartGameInstance(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P1.6 fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate the P1.6 GameInstance."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			Session = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			return Authority && Session;
		}

		bool StartAndCutover(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!Seed(Test, Label) || !StartGameInstance(Test))
			{
				return false;
			}
			const Fdemo_mapProfileSessionInitializeResult Initialized =
				Session->InitializeSession(Storage);
			Fdemo_map0909BWarehousePresentation Presentation;
			FString Diagnostic;
			if (!Initialized.IsReady()
				|| !Warehouse.OpenForSect(
					Root, Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 stable source open failed: %s"), *Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenItemCutoverResult Cutover =
				Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage, SeedProfile.ProfileId, *Authority,
					*Session, Warehouse);
			if (!Cutover.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 cutover failed: %s"), *Cutover.Diagnostic));
				return false;
			}
			return true;
		}

		bool RestartAndBind(FAutomationTestBase& Test)
		{
			Stop();
			if (!StartGameInstance(Test)
				|| !Session->InitializeSession(Storage).IsReady())
			{
				return false;
			}
			const Fdemo_mapShanmenItemAuthorityBindResult Bound =
				Authority->BindExisting(Storage, SeedProfile.ProfileId);
			if (!Bound.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 restart bind failed: %s"), *Bound.Diagnostic));
				return false;
			}
			return true;
		}

		void Stop()
		{
			if (!GameInstance) return;
			GameInstance->Shutdown();
			Authority = nullptr;
			Session = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
		}

		~FPreparationAdapterFixture()
		{
			Stop();
			if (!Root.IsEmpty()) RemovePreparationAdapterRoot(Root);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationProjectionContractTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.CapabilityAndProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationProjectionContractTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("Projection"))) return false;
	FShanmenItemAuthoritySnapshot AuthoritySnapshot;
	TestTrue(TEXT("Ready authority snapshot is readable"),
		Fixture.Authority->TryCaptureSnapshot(AuthoritySnapshot));
	const FShanmenItemDefinition* Weapon = FindAuthorityDefinition(
		AuthoritySnapshot, Fdemo_mapItemIds::TrainingBlade);
	const FShanmenItemDefinition* Material = FindAuthorityDefinition(
		AuthoritySnapshot, Fdemo_mapItemIds::SpiritDust);
	TestTrue(TEXT("Migration grants equipment only the deployment capability"),
		Weapon
		&& Weapon->Supports(EShanmenItemResourceKind::DeploymentLock)
		&& !Weapon->Supports(EShanmenItemResourceKind::Quantity));
	TestTrue(TEXT("Migration keeps stackables quantity-exclusive"),
		Material
		&& Material->Supports(EShanmenItemResourceKind::Quantity)
		&& !Material->Supports(EShanmenItemResourceKind::DeploymentLock));

	const Fdemo_mapProfilePreparationSnapshot Projection =
		Fixture.Session->GetPreparationSnapshot();
	const Fdemo_mapProfilePreparationStashRow* DustRow =
		FindRow(Projection, Fixture.DustId);
	TestTrue(TEXT("Existing equipment is the no-history migration baseline"),
		Projection.SelectedWeaponId == Fixture.TrainingBladeId
		&& Projection.SaveGeneration == AuthoritySnapshot.AuthorityRevision);
	TestTrue(TEXT("All authority items project and eligible complete stacks are selectable"),
		Projection.OrderedPermanentStashRows.Num() == AuthoritySnapshot.Items.Num()
		&& DustRow && DustRow->bMaterialSelectionEligible
		&& !Projection.bCanStartRun
		&& Projection.OrderedSelectedMaterialIds.IsEmpty()
		&& Projection.HotbarBindings.SlotBindings.Num()
			== Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
	TestTrue(TEXT("Canonical authority warehouse projects all thirty cells"),
		Projection.WarehouseLayout.bInitialized
		&& Projection.WarehouseLayout.SlotItemInstanceIds.Num()
			== Fdemo_mapPersistentWarehouseLayout::SlotCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationDurableRestartTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.ReplaceClearRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationDurableRestartTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("Restart"))) return false;
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Legacy Profile bytes are captured after cutover"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));

	const Fdemo_mapProfilePreparationSelectionResult Replace =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot AfterReplace;
	Fixture.Authority->TryCaptureSnapshot(AfterReplace);
	const int32 ReplaceRevision = AfterReplace.AuthorityRevision;
	const Fdemo_mapProfilePreparationSelectionResult Replay =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Replacement is durable and exact replay is mutation-free"),
		Replace.IsAccepted() && Replay.IsAccepted()
		&& Fixture.Session->GetPreparationSnapshot().SelectedWeaponId
			== Fixture.HeavyBladeId
		&& AfterReplay.AuthorityRevision == ReplaceRevision);

	const Fdemo_mapProfilePreparationSelectionResult Clear =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, FGuid());
	const Fdemo_mapProfilePreparationSelectionResult ClearReplay =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, FGuid());
	FShanmenItemAuthoritySnapshot AfterClearReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterClearReplay);
	const int32 ClearRevision = AfterClearReplay.AuthorityRevision;
	TestTrue(TEXT("Terminal latest reservation is a durable empty tombstone"),
		Clear.IsAccepted() && ClearReplay.IsAccepted()
		&& !Fixture.Session->GetPreparationSnapshot().SelectedWeaponId.IsValid());

	const Fdemo_mapProfilePreparationSelectionResult Reselect =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot BeforeRestart;
	Fixture.Authority->TryCaptureSnapshot(BeforeRestart);
	int32 ActiveWeaponIntents = 0;
	for (const FShanmenItemReservationSnapshot& Reservation :
		BeforeRestart.Reservations)
	{
		ActiveWeaponIntents += Reservation.PurposeId
				== FName(TEXT("Shanmen.Preparation.Weapon"))
			&& Reservation.State == EShanmenItemReservationState::Reserved
			? 1 : 0;
	}
	TestTrue(TEXT("Reselect after tombstone creates one new active intent"),
		Reselect.IsAccepted()
		&& BeforeRestart.AuthorityRevision > ClearRevision
		&& ActiveWeaponIntents == 1);

	if (!Fixture.RestartAndBind(*this)) return false;
	TArray<uint8> ProfileAfter;
	const Fdemo_mapProfilePreparationSnapshot RestartProjection =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Restart restores selection only from ShanmenItems"),
		RestartProjection.SelectedWeaponId == Fixture.HeavyBladeId
		&& ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationFailureFenceTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.RejectionAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationFailureFenceTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("FailureFence"))) return false;
	TArray<uint8> ProfileBefore;
	ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore);
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);

	const Fdemo_mapProfilePreparationSelectionResult Incompatible =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.ArmorId);
	const Fdemo_mapProfilePreparationSelectionResult Material =
		Fixture.Session->SetPreparationMaterial(Fixture.ArmorId, true);
	const Fdemo_mapWarehouseMoveResult WarehouseMove =
		Fixture.Session->MoveWarehouseItem(0, 1);
	FShanmenItemAuthoritySnapshot AfterRejected;
	Fixture.Authority->TryCaptureSnapshot(AfterRejected);
	TestTrue(TEXT("Invalid and not-yet-adapted paths mutate neither authority"),
		Incompatible.Status
			== Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected
		&& Material.Status
			== Edemo_mapProfilePreparationSelectionStatus::MaterialRejected
		&& WarehouseMove.Status == Edemo_mapWarehouseMoveStatus::SessionNotReady
		&& AfterRejected == Before);

	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapProfilePreparationSelectionResult Failed =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot AfterFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFailure);
	// The authority test hook is intentionally sticky, unlike a transient OS
	// write error. Remove it before exercising the autonomous retry path.
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	const Fdemo_mapProfilePreparationSelectionResult Retry =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Persistence failure rolls back and an autonomous retry succeeds"),
		Failed.Status
			== Edemo_mapProfilePreparationSelectionStatus::AuthorityCommandRejected
		&& AfterFailure == Before
		&& Retry.IsAccepted()
		&& Fixture.Session->GetPreparationSnapshot().SelectedWeaponId
			== Fixture.HeavyBladeId);
	TestTrue(TEXT("Every P1.6 item path leaves the retired Profile byte-identical"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationRunInventoryRestartTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.QuantityHotbarRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationRunInventoryRestartTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("RunInventoryRestart")))
	{
		return false;
	}
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Retired Profile bytes captured before P1.7 commands"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));

	const Fdemo_mapProfilePreparationSelectionResult Dust =
		Fixture.Session->SetPreparationMaterial(Fixture.DustId, true);
	const Fdemo_mapProfilePreparationSelectionResult PillOne =
		Fixture.Session->SetPreparationMaterial(Fixture.PillOneId, true);
	const Fdemo_mapProfilePreparationSelectionResult PillTwo =
		Fixture.Session->SetPreparationMaterial(Fixture.PillTwoId, true);
	const TArray<FGuid> ExpectedOrder =
		{ Fixture.DustId, Fixture.PillOneId, Fixture.PillTwoId };
	TestTrue(TEXT("Three complete stacks reserve in explicit selection order"),
		Dust.IsAccepted() && PillOne.IsAccepted() && PillTwo.IsAccepted()
		&& Fixture.Session->GetPreparationSnapshot()
			.OrderedSelectedMaterialIds == ExpectedOrder);

	FShanmenItemAuthoritySnapshot ReservedSnapshot;
	TestTrue(TEXT("Reserved authority snapshot captured"),
		Fixture.Authority->TryCaptureSnapshot(ReservedSnapshot));
	FShanmenItemRepository Repository;
	EShanmenItemTransactionError LoadError =
		EShanmenItemTransactionError::None;
	TestTrue(TEXT("Reserved authority remains repository-valid"),
		Repository.TryLoadSnapshot(ReservedSnapshot, &LoadError));
	int32 ActivePreparationQuantityReservations = 0;
	for (const FShanmenItemReservationSnapshot& Reservation :
		ReservedSnapshot.Reservations)
	{
		if (Reservation.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Reservation.State == EShanmenItemReservationState::Reserved
			&& Reservation.PurposeId.ToString().StartsWith(
				TEXT("Shanmen.Preparation.RunInventory.r1.O")))
		{
			++ActivePreparationQuantityReservations;
			const FShanmenItemInstance* Item =
				Repository.FindItem(Reservation.ItemInstanceId);
			TestTrue(TEXT("Preparation Quantity reserve freezes one whole stack"),
				Item && Reservation.Amount == Item->Quantity
				&& Repository.GetAvailableResource(
					Item->ItemInstanceId,
					EShanmenItemResourceKind::Quantity) == 0);
		}
	}
	TestEqual(TEXT("Exactly one active Quantity reserve exists per selected stack"),
		ActivePreparationQuantityReservations, 3);

	TestTrue(TEXT("Occupied slot replacement and stable-item movement succeed"),
		Fixture.Session->SetPreparationHotbarSlot(
			2, Fixture.PillOneId).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			2, Fixture.PillTwoId).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			9, Fixture.PillOneId).IsAccepted());
	const Fdemo_mapProfilePreparationSnapshot Bound =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Hotbar keeps exact nine slots without duplicate bindings"),
		Bound.HotbarBindings.SlotBindings.Num() == 9
		&& Bound.HotbarBindings.SlotBindings[1] == Fixture.PillTwoId
		&& Bound.HotbarBindings.SlotBindings[8] == Fixture.PillOneId
		&& Bound.HotbarBindings.SlotBindings.FilterByPredicate(
			[&Fixture](const FGuid& Id)
			{
				return Id == Fixture.PillOneId;
			}).Num() == 1);

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	const Fdemo_mapProfilePreparationSnapshot Restarted =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Restart restores order and Hotbar only from ShanmenItems"),
		Restarted.OrderedSelectedMaterialIds == ExpectedOrder
		&& Restarted.HotbarBindings.SlotBindings[1]
			== Fixture.PillTwoId
		&& Restarted.HotbarBindings.SlotBindings[8]
			== Fixture.PillOneId);
	TestTrue(TEXT("Removing a selected stack clears its encoded Hotbar binding"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, false).IsAccepted()
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[8].IsValid());
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("P1.7 selection and Hotbar commands never rewrite retired Profile"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationRunInventoryFenceTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.QuantityCapacityAndFailureFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationRunInventoryFenceTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("RunInventoryFence")))
	{
		return false;
	}
	TArray<FGuid> SixStacks =
		{ Fixture.DustId, Fixture.PillOneId, Fixture.PillTwoId };
	SixStacks.Append(Fixture.ExtraMaterialIds.GetData(), 3);
	for (const FGuid& ItemId : SixStacks)
	{
		TestTrue(TEXT("Complete stack fits one of six base cells"),
			Fixture.Session->SetPreparationMaterial(
				ItemId, true).IsAccepted());
	}
	TestTrue(TEXT("Seventh stack fails closed without selected storage"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.ExtraMaterialIds[3], true).Status
			== Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded);
	TestTrue(TEXT("Backpack expands authority-native carried capacity"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot,
			Fixture.BackpackId).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.ExtraMaterialIds[3], true).IsAccepted());
	TestTrue(TEXT("Capacity equipment cannot be removed while selected stacks depend on it"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot, FGuid()).Status
			== Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded);
	TestTrue(TEXT("Removing the overflow stack makes backpack clear legal"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.ExtraMaterialIds[3], false).IsAccepted()
		&& Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot, FGuid()).IsAccepted());
	TestTrue(TEXT("Material cannot bind to Hotbar"),
		Fixture.Session->SetPreparationHotbarSlot(
			1, Fixture.DustId).Status
			== Edemo_mapProfilePreparationSelectionStatus::MaterialRejected);

	TestTrue(TEXT("Consumable binds before injected persistence failure"),
		Fixture.Session->SetPreparationHotbarSlot(
			1, Fixture.PillOneId).IsAccepted());
	FShanmenItemAuthoritySnapshot BeforeFailure;
	Fixture.Authority->TryCaptureSnapshot(BeforeFailure);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapProfilePreparationSelectionResult Failed =
		Fixture.Session->SetPreparationHotbarSlot(2, Fixture.PillOneId);
	FShanmenItemAuthoritySnapshot AfterFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFailure);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Failed metadata transition rolls back before publishing"),
		!Failed.IsAccepted() && AfterFailure == BeforeFailure
		&& Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[0] == Fixture.PillOneId
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[1].IsValid());
	TestTrue(TEXT("Autonomous retry moves the binding once"),
		Fixture.Session->SetPreparationHotbarSlot(
			2, Fixture.PillOneId).IsAccepted()
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[0].IsValid()
		&& Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[1] == Fixture.PillOneId);
	const Fdemo_mapProfilePreparationSelectionResult ClearAll =
		Fixture.Session->ClearPreparationSelection();
	const Fdemo_mapProfilePreparationSnapshot Cleared =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Clear All releases Quantity reserves before capacity equipment"),
		ClearAll.IsAccepted()
		&& Cleared.OrderedSelectedMaterialIds.IsEmpty()
		&& !Cleared.HotbarBindings.SlotBindings.ContainsByPredicate(
			[](const FGuid& Id) { return Id.IsValid(); })
		&& !Cleared.SelectedWeaponId.IsValid()
		&& !Cleared.SelectedBackpackId.IsValid());
	return true;
}

#endif
