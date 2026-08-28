#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenPreparationAdapter.h"

#include "ShanmenItemRepository.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

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
	const FShanmenItemReservationSnapshot* ReservationBefore =
		BeforeFailure.Reservations.FindByPredicate(
			[&Fixture](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ItemInstanceId == Fixture.PillOneId
					&& Reservation.ResourceKind
						== EShanmenItemResourceKind::Quantity
					&& Reservation.State
						== EShanmenItemReservationState::Reserved;
			});
	const FGuid ReservationIdBefore = ReservationBefore
		? ReservationBefore->ReservationId : FGuid();
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapProfilePreparationSelectionResult Failed =
		Fixture.Session->SetPreparationHotbarSlot(2, Fixture.PillOneId);
	FShanmenItemAuthoritySnapshot AfterFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFailure);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Failed atomic metadata amendment rolls back before publishing"),
		!Failed.IsAccepted() && AfterFailure == BeforeFailure
		&& Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[0] == Fixture.PillOneId
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[1].IsValid());
	const bool bRetryAccepted = Fixture.Session->SetPreparationHotbarSlot(
		2, Fixture.PillOneId).IsAccepted();
	FShanmenItemAuthoritySnapshot AfterRetry;
	Fixture.Authority->TryCaptureSnapshot(AfterRetry);
	const FShanmenItemReservationSnapshot* ReservationAfter =
		AfterRetry.Reservations.FindByPredicate(
			[&Fixture](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ItemInstanceId == Fixture.PillOneId
					&& Reservation.ResourceKind
						== EShanmenItemResourceKind::Quantity
					&& Reservation.State
						== EShanmenItemReservationState::Reserved;
			});
	TestTrue(TEXT("Autonomous retry atomically amends the same reservation once"),
		bRetryAccepted
		&& ReservationIdBefore.IsValid()
		&& ReservationAfter
		&& ReservationAfter->ReservationId == ReservationIdBefore
		&& AfterRetry.AuthorityRevision == BeforeFailure.AuthorityRevision + 1
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationAtomicCommitTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.AtomicPreparedLoadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationAtomicCommitTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("AtomicPreparedLoadout")))
	{
		return false;
	}
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Retired Profile bytes captured before P1.8 commit"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));
	TestTrue(TEXT("Migrated baseline becomes an explicit pending lock"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.TrainingBladeId).IsAccepted());
	TestTrue(TEXT("Two complete stacks and one Hotbar binding prepare"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.DustId, true).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			3, Fixture.PillOneId).IsAccepted());

	FShanmenItemAuthoritySnapshot Before;
	FShanmenItemAuthorityDocument DocumentBefore;
	TestTrue(TEXT("Pre-commit authority and generation captured"),
		Fixture.Authority->TryCaptureSnapshot(Before)
		&& Fixture.Authority->TryGetDocument(DocumentBefore));
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenPreparedLoadoutResult Failed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	FShanmenItemAuthoritySnapshot AfterFailure;
	FShanmenItemAuthorityDocument DocumentAfterFailure;
	TestTrue(TEXT("Failed durable batch publishes no partial item state"),
		!Failed.IsCommitted()
		&& Fixture.Authority->TryCaptureSnapshot(AfterFailure)
		&& AfterFailure == Before
		&& Fixture.Authority->TryGetDocument(DocumentAfterFailure)
		&& DocumentAfterFailure == DocumentBefore);

	const Fdemo_mapShanmenPreparedLoadoutResult Committed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	FShanmenItemAuthoritySnapshot After;
	FShanmenItemAuthorityDocument DocumentAfter;
	TestTrue(TEXT("Prepared loadout commits durably"),
		Committed.IsCommitted()
		&& Committed.Status
			== Edemo_mapShanmenPreparationAdapterStatus::Accepted
		&& Fixture.Authority->TryCaptureSnapshot(After)
		&& Fixture.Authority->TryGetDocument(DocumentAfter));
	TestTrue(TEXT("One batch revision and one document generation publish"),
		After.AuthorityRevision == Before.AuthorityRevision + 1
		&& DocumentAfter.SaveGeneration
			== DocumentBefore.SaveGeneration + 1);
	TestTrue(TEXT("Receipt freezes equipment, order, and Hotbar"),
		Committed.Receipt.WeaponItemInstanceId
			== Fixture.TrainingBladeId
		&& Committed.Receipt.OrderedRunInventoryItemInstanceIds
			== TArray<FGuid>({ Fixture.DustId, Fixture.PillOneId })
		&& Committed.Receipt.HotbarItemInstanceIds[2]
			== Fixture.PillOneId
		&& Committed.Receipt.OrderedLines.Num() == 3);
	const FShanmenItemInstance* Weapon = After.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.TrainingBladeId;
		});
	const FShanmenItemInstance* Dust = After.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.DustId;
		});
	const FShanmenItemInstance* Pill = After.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.PillOneId;
		});
	TestTrue(TEXT("Batch deploys equipment and consumes both full stacks"),
		Weapon && Weapon->State == EShanmenItemInstanceState::Deployed
		&& Dust && Dust->State == EShanmenItemInstanceState::Depleted
		&& Pill && Pill->State == EShanmenItemInstanceState::Depleted);
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Atomic preparation never rewrites retired Profile items"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);

	if (!Fixture.RestartAndBind(*this)) return false;
	const Fdemo_mapShanmenPreparedLoadoutResult Replayed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	TestTrue(TEXT("Restart reconstructs the same committed loadout without another write"),
		Replayed.IsCommitted()
		&& Replayed.Status
			== Edemo_mapShanmenPreparationAdapterStatus::NoChange
		&& Replayed.Receipt.BatchRequestId
			== Committed.Receipt.BatchRequestId
		&& Replayed.Receipt.BatchReceiptId
			== Committed.Receipt.BatchReceiptId
		&& Replayed.Receipt.OrderedRunInventoryItemInstanceIds
			== Committed.Receipt.OrderedRunInventoryItemInstanceIds);
	FShanmenItemAuthorityDocument RestartedDocument;
	TestTrue(TEXT("Receipt reconstruction performs no extra persistence"),
		Fixture.Authority->TryGetDocument(RestartedDocument)
		&& RestartedDocument.SaveGeneration
			== DocumentAfter.SaveGeneration);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationPlainPurposeCompatibilityTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.PlainPurposeReceiptCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationPlainPurposeCompatibilityTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("PlainPurposeCompatibility")))
	{
		return false;
	}

	FShanmenItemAuthoritySnapshot Before;
	if (!Fixture.Authority->TryCaptureSnapshot(Before))
	{
		AddError(TEXT("Could not capture authority before the compatibility reserve."));
		return false;
	}
	const FShanmenItemInstance* Dust = Before.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.DustId;
		});
	if (!Dust)
	{
		AddError(TEXT("Compatibility fixture is missing its Spirit Dust stack."));
		return false;
	}

	// P1.7/P1.8 persisted the logical purpose directly, before P1.9 began
	// appending an exact source-cell placement envelope for settlement.
	const FName PlainPurpose(
		TEXT("Shanmen.Preparation.RunInventory.r1.O00000000.H00"));
	FShanmenItemReserveRequest ReserveRequest;
	ReserveRequest.Context.RunId = Dust->RunId;
	ReserveRequest.Context.OwnerId = Dust->OwnerId;
	ReserveRequest.Context.RequestId = FGuid::NewGuid();
	ReserveRequest.Context.Content = Before.Content;
	ReserveRequest.ItemInstanceId = Dust->ItemInstanceId;
	ReserveRequest.ResourceKind = EShanmenItemResourceKind::Quantity;
	ReserveRequest.Amount = Dust->Quantity;
	ReserveRequest.ExpectedItemRevision = Dust->Revision;
	ReserveRequest.PurposeId = PlainPurpose;
	const FShanmenItemDurableCommandResult Reserved =
		Fixture.Authority->ReserveDurable(ReserveRequest);
	TestTrue(TEXT("Legacy plain-purpose Quantity intent remains durable"),
		Reserved.IsCommandSuccess());

	const Fdemo_mapShanmenPreparedLoadoutResult Committed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	const Fdemo_mapShanmenPreparedLoadoutLine* DustLine =
		Committed.Receipt.OrderedLines.FindByPredicate(
			[&Fixture](const Fdemo_mapShanmenPreparedLoadoutLine& Line)
			{
				return Line.ItemInstanceId == Fixture.DustId;
			});
	TestTrue(TEXT("Atomic commit accepts the pre-placement plain purpose"),
		Committed.IsCommitted()
		&& Committed.Receipt.OrderedRunInventoryItemInstanceIds
			== TArray<FGuid>({ Fixture.DustId })
		&& DustLine
		&& DustLine->PurposeId == PlainPurpose
		&& !DustLine->SourceContainerId.IsValid()
		&& DustLine->SourceSlotIndex == INDEX_NONE);

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	const Fdemo_mapShanmenPreparedLoadoutResult Replayed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	const Fdemo_mapShanmenPreparedLoadoutLine* ReplayedDustLine =
		Replayed.Receipt.OrderedLines.FindByPredicate(
			[&Fixture](const Fdemo_mapShanmenPreparedLoadoutLine& Line)
			{
				return Line.ItemInstanceId == Fixture.DustId;
			});
	TestTrue(TEXT("Restart reconstructs the legacy receipt without another write"),
		Replayed.IsCommitted()
		&& Replayed.Status == Edemo_mapShanmenPreparationAdapterStatus::NoChange
		&& Replayed.Receipt.BatchRequestId == Committed.Receipt.BatchRequestId
		&& ReplayedDustLine
		&& ReplayedDustLine->PurposeId == PlainPurpose
		&& !ReplayedDustLine->SourceContainerId.IsValid()
		&& ReplayedDustLine->SourceSlotIndex == INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparedRunLifecycleRestartTest,
	"Shanmen.0_0_10.Items.RunLifecycle.ClaimRestartFinalize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparedRunLifecycleRestartTest::RunTest(const FString&)
{
	FString DefinitionsError;
	TestTrue(TEXT("Runtime dependency manifest validates before lifecycle handoff"),
		Fdemo_mapItemDefinitions::Validate(&DefinitionsError));
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("RunLifecycle")))
	{
		return false;
	}
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Retired Profile captured before P1.9 lifecycle"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));
	TestTrue(TEXT("Recoverable equipment and complete stacks prepare"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.TrainingBladeId).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.DustId, true).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			4, Fixture.PillOneId).IsAccepted());

	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Runtime)
	{
		AddError(TEXT("P1.9 fixture has no Runtime item subsystem."));
		return false;
	}
	Runtime->ResetForAutomation();
	Runtime->SetPreparedRunFailureAfterMutationForAutomation(1);
	const Fdemo_mapShanmenRunStartResult FailedStart =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	AddInfo(FString::Printf(
		TEXT("P1.9 first start status=%d diagnostic=%s claim_status=%d claim_error=%d runtime_status=%d"),
		static_cast<int32>(FailedStart.Status), *FailedStart.Diagnostic,
		static_cast<int32>(FailedStart.ClaimCommand.Status),
		static_cast<int32>(FailedStart.ClaimCommand.Receipt.Error),
		static_cast<int32>(FailedStart.RuntimeResult.Status)));
	FShanmenItemAuthoritySnapshot Claimed;
	const bool bCapturedClaimed =
		Fixture.Authority->TryCaptureSnapshot(Claimed);
	TestTrue(TEXT("Runtime failure rolls back transient items but retains one durable claim"),
		!FailedStart.IsStarted()
		&& FailedStart.Status
			== Edemo_mapShanmenRunLifecycleStatus::RuntimeMaterializationRejected
		&& FailedStart.ActiveRunId.IsValid()
		&& Runtime->GetRunState() == Edemo_mapRunState::Inactive
		&& Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty()
		&& bCapturedClaimed);
	int32 ClaimCount = 0;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Claimed.ProcessedRequests)
	{
		ClaimCount += Processed.Receipt.IsSuccess()
			&& Processed.Receipt.Operation
				== EShanmenItemTransactionOperation::ClaimPreparedRun ? 1 : 0;
	}
	TestEqual(TEXT("Exactly one claim marker survives Runtime rollback"),
		ClaimCount, 1);

	const FGuid ActiveRunId = FailedStart.ActiveRunId;
	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	Runtime = Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	const Fdemo_mapShanmenRunStartResult Resumed = Runtime
		? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime)
		: Fdemo_mapShanmenRunStartResult();
	TestTrue(TEXT("Restart replays claim and materializes the same ActiveRunId"),
		Runtime && Resumed.IsStarted()
		&& Resumed.Status == Edemo_mapShanmenRunLifecycleStatus::Resumed
		&& Resumed.ActiveRunId == ActiveRunId
		&& Runtime->GetRunState() == Edemo_mapRunState::Active
		&& Runtime->GetActiveRunId() == ActiveRunId
		&& Runtime->GetDeployedItemIds().Num() == 3);

	Fdemo_mapSettlementSummary Summary;
	const Fdemo_mapItemOperationResult RuntimeSettlement =
		Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Extraction, Summary);
	TestTrue(TEXT("Runtime emits one extraction snapshot for all originals"),
		RuntimeSettlement.bSuccess && Summary.bValid
		&& Summary.RuntimeSnapshot.bValid
		&& Summary.RunId == ActiveRunId
		&& Summary.RuntimeSnapshot.OrderedSecuredItems.Num() == 3);
	Fdemo_mapRuntimeSettlementItem* PartiallyConsumedPill =
		Summary.RuntimeSnapshot.OrderedSecuredItems.FindByPredicate(
			[&Fixture](const Fdemo_mapRuntimeSettlementItem& Item)
			{
				return Item.ItemInstanceId == Fixture.PillOneId;
			});
	TestTrue(TEXT("Fixture models one consumed pill in the immutable Runtime handoff"),
		PartiallyConsumedPill != nullptr);
	if (PartiallyConsumedPill)
	{
		PartiallyConsumedPill->StackCount = 2;
	}

	FShanmenItemAuthoritySnapshot BeforeFinalizeFailure;
	Fixture.Authority->TryCaptureSnapshot(BeforeFinalizeFailure);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenRunFinalizeResult FailedFinalize =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, Summary);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	FShanmenItemAuthoritySnapshot AfterFinalizeFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFinalizeFailure);
	TestTrue(TEXT("Failed terminal save restores exact active authority"),
		!FailedFinalize.IsFinalized()
		&& AfterFinalizeFailure == BeforeFinalizeFailure);

	const Fdemo_mapShanmenRunFinalizeResult Finalized =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, Summary);
	FShanmenItemAuthoritySnapshot Terminal;
	TestTrue(TEXT("Autonomous retry atomically finalizes extraction"),
		Finalized.IsFinalized()
		&& Finalized.Status
			== Edemo_mapShanmenRunLifecycleStatus::Finalized
		&& Fixture.Authority->TryCaptureSnapshot(Terminal));
	const FShanmenItemInstance* Weapon = Terminal.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.TrainingBladeId;
		});
	const FShanmenItemInstance* Dust = Terminal.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.DustId;
		});
	const FShanmenItemInstance* Pill = Terminal.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.PillOneId;
		});
	int32 ReleasedLines = 0;
	int32 FinalizeCount = 0;
	for (const FShanmenItemReservationSnapshot& Reservation :
		Terminal.Reservations)
	{
		ReleasedLines += Reservation.State
			== EShanmenItemReservationState::Released ? 1 : 0;
	}
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Terminal.ProcessedRequests)
	{
		FinalizeCount += Processed.Receipt.IsSuccess()
			&& Processed.Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRun ? 1 : 0;
	}
	TestTrue(TEXT("Terminal candidate restores exact source cells and releases every line"),
		Weapon && Weapon->State == EShanmenItemInstanceState::Stored
		&& !Weapon->DeploymentReservationId.IsValid()
		&& Dust && Dust->State == EShanmenItemInstanceState::Stored
		&& Dust->Quantity == 3 && Dust->ParentContainerId.IsValid()
		&& Pill && Pill->State == EShanmenItemInstanceState::Stored
		&& Pill->Quantity == 2 && Pill->ParentContainerId.IsValid()
		&& ReleasedLines == 3 && FinalizeCount == 1);
	const Fdemo_mapShanmenRunFinalizeResult ReplayFinalize =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, Summary);
	TestTrue(TEXT("Repeated settlement observes terminal marker without another write"),
		ReplayFinalize.IsFinalized()
		&& ReplayFinalize.Status
			== Edemo_mapShanmenRunLifecycleStatus::NoChange);

	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("P1.9 lifecycle never rewrites retired Profile bytes"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Restarted;
	TestTrue(TEXT("Restart keeps one terminal marker and restored originals"),
		Fixture.Authority->TryCaptureSnapshot(Restarted)
		&& Restarted == Terminal);
	return true;
}

#endif
