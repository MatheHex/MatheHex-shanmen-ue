#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewPreparationRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.4.10.r0"),
			TEXT("ProfilePreparation"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	bool ReadPreparationBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	struct FPreparationProductionSnapshot
	{
		TArray<FString> Paths;
		TArray<bool> Existed;
		TArray<TArray<uint8>> Bytes;

		FPreparationProductionSnapshot()
		{
			const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
			Paths = { Production.RootDirectory, Production.PrimaryPath(), Production.BackupPath(), Production.TempPath() };
			for (int32 Index = 0; Index < Paths.Num(); ++Index)
			{
				const bool bExists = Index == 0
					? IFileManager::Get().DirectoryExists(*Paths[Index])
					: IFileManager::Get().FileExists(*Paths[Index]);
				Existed.Add(bExists);
				TArray<uint8> Value;
				if (Index > 0 && bExists)
				{
					ReadPreparationBytes(Paths[Index], Value);
				}
				Bytes.Add(MoveTemp(Value));
			}
		}

		bool IsUnchanged() const
		{
			for (int32 Index = 0; Index < Paths.Num(); ++Index)
			{
				const bool bExists = Index == 0
					? IFileManager::Get().DirectoryExists(*Paths[Index])
					: IFileManager::Get().FileExists(*Paths[Index]);
				if (bExists != Existed[Index])
				{
					return false;
				}
				if (Index > 0 && bExists)
				{
					TArray<uint8> After;
					if (!ReadPreparationBytes(Paths[Index], After) || After != Bytes[Index])
					{
						return false;
					}
				}
			}
			return true;
		}
	};

	struct FPreparationFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Udemo_mapItemSubsystem* Runtime = nullptr;
		bool bStarted = false;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for Preparation GameInstance fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Failed to allocate Preparation GameInstance fixture."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			bStarted = true;
			Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
			Runtime = GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
			if (!Session || !Runtime)
			{
				Test.AddError(TEXT("Normal GameInstance acquisition did not return ProfileSession and Item subsystems."));
				return false;
			}
			return true;
		}

		void Stop()
		{
			if (!GameInstance)
			{
				return;
			}
			if (bStarted)
			{
				GameInstance->Shutdown();
			}
			Session = nullptr;
			Runtime = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			bStarted = false;
			CollectGarbage(RF_NoFlags);
		}

		~FPreparationFixture()
		{
			Stop();
		}
	};

	FGuid AddPreparationStashRecord(Fdemo_mapPersistentProfile& Profile, FName DefinitionId, int32 StackCount)
	{
		Fdemo_mapPersistentItemRecord Item;
		do
		{
			Item.ItemInstanceId = FGuid::NewGuid();
		}
		while (Profile.PermanentStash.ContainsByPredicate(
			[&Item](const Fdemo_mapPersistentItemRecord& Existing)
			{
				return Existing.ItemInstanceId == Item.ItemInstanceId;
			}));
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = StackCount;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item.ItemInstanceId;
	}

	FGuid FindPreparationId(const Fdemo_mapProfilePreparationSnapshot& Snapshot, FName DefinitionId, int32 Occurrence = 0)
	{
		int32 Found = 0;
		for (const Fdemo_mapProfilePreparationStashRow& Row : Snapshot.OrderedPermanentStashRows)
		{
			if (Row.ItemDefinitionId == DefinitionId && Found++ == Occurrence)
			{
				return Row.ItemInstanceId;
			}
		}
		return FGuid();
	}

	bool HasNoPreparationSelection(const Fdemo_mapProfilePreparationSnapshot& Snapshot)
	{
		return !Snapshot.SelectedWeaponId.IsValid()
			&& !Snapshot.SelectedArmorId.IsValid()
			&& !Snapshot.SelectedAccessoryId.IsValid()
			&& !Snapshot.SelectedSpatialRingId.IsValid()
			&& !Snapshot.SelectedBackpackId.IsValid()
			&& Snapshot.OrderedSelectedMaterialIds.IsEmpty()
			&& !Snapshot.OrderedPermanentStashRows.ContainsByPredicate(
				[](const Fdemo_mapProfilePreparationStashRow& Row)
				{
					return Row.bSelected;
				});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation01, "demo_map.ProfilePreparation.01.LazyUninitializedAndProductionProtected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation01::RunTest(const FString&)
{
	FPreparationProductionSnapshot Production;
	const FString UnusedRoot = NewPreparationRoot();
	FPreparationFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
	const auto Select = Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, FGuid::NewGuid());
	const auto Start = Fixture.Session->StartPreparedRun();
	TestTrue(TEXT("Preparation starts lazy and empty"), Snapshot.SessionState == Edemo_mapProfileSessionState::Uninitialized && !Snapshot.ProfileId.IsValid() && Snapshot.OrderedPermanentStashRows.IsEmpty() && HasNoPreparationSelection(Snapshot) && !Snapshot.bCanStartRun);
	TestTrue(TEXT("Uninitialized selection and Start reject safely"), Select.Status == Edemo_mapProfilePreparationSelectionStatus::SessionNotReady && Start.Status == Edemo_mapProfileSessionBeginStatus::SessionNotReady);
	TestFalse(TEXT("Unused isolation root remains absent"), IFileManager::Get().DirectoryExists(*UnusedRoot));
	TestTrue(TEXT("Production remains byte-identical"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation02, "demo_map.ProfilePreparation.02.FreshIdleSnapshotOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation02::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	FGuid ProfileId; int32 Generation = 0;
	{
		FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
		const auto Init = Fixture.Session->InitializeSession(Storage); const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
		ProfileId = Snapshot.ProfileId; Generation = Snapshot.SaveGeneration;
		TestTrue(TEXT("Fresh Preparation snapshot is Ready and startable empty"), Init.IsReady() && Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation && Snapshot.bCanStartRun && HasNoPreparationSelection(Snapshot));
		TestTrue(TEXT("Fresh Stash order is the frozen three equipment records"), Snapshot.OrderedPermanentStashRows.Num() == 3
			&& Snapshot.OrderedPermanentStashRows[0].ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade
			&& Snapshot.OrderedPermanentStashRows[0].CompatibleEquipmentSlotId == Fdemo_mapItemIds::WeaponSlot
			&& Snapshot.OrderedPermanentStashRows[1].ItemDefinitionId == Fdemo_mapItemIds::TrainingVest
			&& Snapshot.OrderedPermanentStashRows[1].CompatibleEquipmentSlotId == Fdemo_mapItemIds::ArmorSlot
			&& Snapshot.OrderedPermanentStashRows[2].ItemDefinitionId == Fdemo_mapItemIds::WindTalisman
			&& Snapshot.OrderedPermanentStashRows[2].CompatibleEquipmentSlotId == Fdemo_mapItemIds::SpatialRingSlot);
		for (const auto& Row : Snapshot.OrderedPermanentStashRows) TestTrue(TEXT("Fresh rows are safe and unselected"), Row.bSafeInPermanentStash && !Row.bSelected);
	}
	FPreparationFixture Reloaded; if (!Reloaded.Start(*this)) return false;
	const auto Reload = Reloaded.Session->InitializeSession(Storage); const auto Snapshot = Reloaded.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Idle reload preserves identity/order without temporary selection"), Reload.IsReady() && Snapshot.ProfileId == ProfileId && Snapshot.SaveGeneration == Generation && Snapshot.OrderedPermanentStashRows.Num() == 3 && HasNoPreparationSelection(Snapshot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation03, "demo_map.ProfilePreparation.03.EquipmentSlotsAndCompatibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation03::RunTest(const FString&)
{
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot()));
	const auto Initial = Fixture.Session->GetPreparationSnapshot();
	const FGuid Weapon = FindPreparationId(Initial, Fdemo_mapItemIds::TrainingBlade);
	const FGuid Armor = FindPreparationId(Initial, Fdemo_mapItemIds::TrainingVest);
	const FGuid SpatialRing = FindPreparationId(Initial, Fdemo_mapItemIds::WindTalisman);
	const auto Wrong = Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::ArmorSlot, Weapon);
	const auto WeaponResult = Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, Weapon);
	const auto ArmorResult = Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::ArmorSlot, Armor);
	const auto SpatialRingResult = Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::SpatialRingSlot, SpatialRing);
	const auto Duplicate = Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::AccessorySlot, Weapon);
	const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Wrong slot rejects explicitly"), Wrong.Status == Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected);
	TestTrue(TEXT("All three compatible slots accept original IDs"), WeaponResult.IsAccepted() && ArmorResult.IsAccepted() && SpatialRingResult.IsAccepted() && Snapshot.SelectedWeaponId == Weapon && Snapshot.SelectedArmorId == Armor && Snapshot.SelectedSpatialRingId == SpatialRing);
	TestTrue(TEXT("Cross-slot duplicate ID rejects"), Duplicate.Status == Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation04, "demo_map.ProfilePreparation.04.MaterialWhitelistWholeStacksAndLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation04::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid IronA = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::IronShard, 4);
	const FGuid DustB = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2);
	const FGuid IronB = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::IronShard, 1);
	const FGuid Token = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::AncientToken, 1);
	TestTrue(TEXT("Material fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	TestTrue(TEXT("Four complete allowed Stacks fit the six base quick cells"), Fixture.Session->SetPreparationMaterial(DustA, true).IsAccepted() && Fixture.Session->SetPreparationMaterial(IronA, true).IsAccepted() && Fixture.Session->SetPreparationMaterial(DustB, true).IsAccepted() && Fixture.Session->SetPreparationMaterial(IronB, true).IsAccepted());
	TestTrue(TEXT("AncientToken remains stash-only"), Fixture.Session->SetPreparationMaterial(Token, true).Status == Edemo_mapProfilePreparationSelectionStatus::MaterialRejected);
	const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Snapshot carries original full Stack counts"), Snapshot.OrderedSelectedMaterialIds.Num() == 4
		&& Snapshot.OrderedPermanentStashRows.FindByPredicate([&DustA](const auto& Row){ return Row.ItemInstanceId == DustA; })->StackCount == 5
		&& Snapshot.OrderedPermanentStashRows.FindByPredicate([&IronA](const auto& Row){ return Row.ItemInstanceId == IronA; })->StackCount == 4
		&& Snapshot.OrderedPermanentStashRows.FindByPredicate([&DustB](const auto& Row){ return Row.ItemInstanceId == DustB; })->StackCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation05, "demo_map.ProfilePreparation.05.InvalidMissingDuplicateAndIncompatibleReject", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation05::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Dust = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid Token = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::AncientToken, 1);
	TestTrue(TEXT("Invalid-selection fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage); const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
	const FGuid Armor = FindPreparationId(Snapshot, Fdemo_mapItemIds::TrainingVest);
	TestTrue(TEXT("Missing or expired ID rejects"), Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, FGuid::NewGuid()).Status == Edemo_mapProfilePreparationSelectionStatus::ItemNotFound);
	TestTrue(TEXT("Incompatible definition rejects"), Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, Armor).Status == Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected);
	TestTrue(TEXT("Unknown slot rejects"), Fixture.Session->SetPreparationEquipment(FName(TEXT("Prototype.Slot.Unknown")), Armor).Status == Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected);
	TestTrue(TEXT("AncientToken material selection rejects"), Fixture.Session->SetPreparationMaterial(Token, true).Status == Edemo_mapProfilePreparationSelectionStatus::MaterialRejected);
	TestTrue(TEXT("Material accepts once"), Fixture.Session->SetPreparationMaterial(Dust, true).IsAccepted());
	TestTrue(TEXT("Duplicate material ID rejects"), Fixture.Session->SetPreparationMaterial(Dust, true).Status == Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation06, "demo_map.ProfilePreparation.06.EmptyLoadoutIsLegal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation06::RunTest(const FString&)
{
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot()));
	const auto Before = Fixture.Session->GetPreparationSnapshot();
	const auto Begin = Fixture.Session->StartPreparedRun();
	TestTrue(TEXT("Empty Preparation Snapshot can start"), Before.bCanStartRun && HasNoPreparationSelection(Before));
	TestTrue(TEXT("Empty Start uses existing unique run"), Begin.IsRunActive() && Begin.Snapshot.ActiveRunId.IsValid() && Begin.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId() && Begin.RuntimeResult.DeployedItemIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation07, "demo_map.ProfilePreparation.07.MaterialOrderCanonicalizedByStash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation07::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid Iron = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::IronShard, 3);
	const FGuid DustB = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 1);
	TestTrue(TEXT("Canonical-order fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	Fixture.Session->SetPreparationMaterial(DustB, true);
	Fixture.Session->SetPreparationMaterial(Iron, true);
	Fixture.Session->SetPreparationMaterial(DustA, true);
	const TArray<FGuid> Expected = { DustB, Iron, DustA };
	const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
	const auto Begin = Fixture.Session->StartPreparedRun();
	TestTrue(TEXT("Snapshot preserves explicit quick-cell selection order"), Snapshot.OrderedSelectedMaterialIds == Expected);
	TestTrue(TEXT("Begin request reaches transaction in explicit quick-cell order"), Begin.IsRunActive() && Begin.PersistentResult.CommittedLoadoutPlan.IsSet() && Begin.PersistentResult.CommittedLoadoutPlan->DeployedItemIds == Expected && Begin.RuntimeResult.DeployedItemIds == Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation08, "demo_map.ProfilePreparation.08.SelectionHasNoPersistentOrRuntimeMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation08::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Dust = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	TestTrue(TEXT("No-mutation fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage); const auto BeforeSession = Fixture.Session->GetSnapshot(); const auto BeforePreparation = Fixture.Session->GetPreparationSnapshot();
	TArray<uint8> BeforeBytes; ReadPreparationBytes(Storage.PrimaryPath(), BeforeBytes);
	Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationId(BeforePreparation, Fdemo_mapItemIds::TrainingBlade));
	Fixture.Session->SetPreparationMaterial(Dust, true);
	TArray<uint8> AfterBytes; ReadPreparationBytes(Storage.PrimaryPath(), AfterBytes); const auto AfterSession = Fixture.Session->GetSnapshot();
	TestTrue(TEXT("Selection commits layout without changing profile identity or Stash"), BeforeBytes != AfterBytes && BeforeSession.ProfileId == AfterSession.ProfileId && BeforeSession.SaveGeneration + 2 == AfterSession.SaveGeneration && BeforeSession.OrderedPermanentStash == AfterSession.OrderedPermanentStash);
	TestTrue(TEXT("Selection does not create ActiveRun or mutate Runtime"), !AfterSession.ActiveRunId.IsValid() && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation09, "demo_map.ProfilePreparation.09.SuccessfulStartPreservesIdentityAndClears", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation09::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Dust = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid Iron = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::IronShard, 2);
	TestTrue(TEXT("Successful Start fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage); const auto Initial = Fixture.Session->GetPreparationSnapshot();
	const FGuid Weapon = FindPreparationId(Initial, Fdemo_mapItemIds::TrainingBlade);
	const FGuid Armor = FindPreparationId(Initial, Fdemo_mapItemIds::TrainingVest);
	const FGuid SpatialRing = FindPreparationId(Initial, Fdemo_mapItemIds::WindTalisman);
	Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, Weapon);
	Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::ArmorSlot, Armor);
	Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::SpatialRingSlot, SpatialRing);
	Fixture.Session->SetPreparationMaterial(Iron, true);
	Fixture.Session->SetPreparationMaterial(Dust, true);
	const TArray<FGuid> Expected = { Weapon, Armor, SpatialRing, Iron, Dust };
	const auto Begin = Fixture.Session->StartPreparedRun(); const auto ActivePreparation = Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Successful Start preserves original identities and one RunId"), Begin.IsRunActive() && Begin.PersistentResult.CommittedLoadoutPlan->DeployedItemIds == Expected && Begin.RuntimeResult.DeployedItemIds == Expected && Begin.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId());
	TestTrue(TEXT("Successful Start keeps the committed layout visible while disabling edits"), ActivePreparation.SessionState == Edemo_mapProfileSessionState::RunActive && ActivePreparation.SelectedWeaponId == Weapon && ActivePreparation.SelectedSpatialRingId == SpatialRing && ActivePreparation.OrderedSelectedMaterialIds == TArray<FGuid>({ Iron, Dust }) && !ActivePreparation.bCanStartRun);
	Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("Runtime Extraction accepted"), Fixture.Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess);
	const auto End = Fixture.Session->CommitRuntimeSettlement(Summary); const auto ReadyPreparation = Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Settlement returns Ready with the committed layout available for the next preparation"), End.IsDurablySettled() && ReadyPreparation.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation && ReadyPreparation.SelectedWeaponId == Weapon && ReadyPreparation.SelectedSpatialRingId == SpatialRing && ReadyPreparation.OrderedSelectedMaterialIds == TArray<FGuid>({ Iron, Dust }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation10, "demo_map.ProfilePreparation.10.PrecommitFailurePreservesRetrySelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation10::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Dust = AddPreparationStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	TestTrue(TEXT("Retry selection fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage); const auto Initial = Fixture.Session->GetPreparationSnapshot(); const FGuid Weapon = FindPreparationId(Initial, Fdemo_mapItemIds::TrainingBlade);
	Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, Weapon); Fixture.Session->SetPreparationMaterial(Dust, true);
	TArray<uint8> Before; ReadPreparationBytes(Storage.PrimaryPath(), Before);
	Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp);
	const auto Failed = Fixture.Session->StartPreparedRun(); TArray<uint8> After; ReadPreparationBytes(Storage.PrimaryPath(), After); const auto Preserved = Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Explicit precommit failure preserves safe selection"), Failed.Status == Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected && Failed.Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation && Preserved.SelectedWeaponId == Weapon && Preserved.OrderedSelectedMaterialIds.Num() == 1 && Preserved.OrderedSelectedMaterialIds[0] == Dust);
	TestTrue(TEXT("Precommit failure leaves disk and Runtime unchanged"), Before == After && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty());
	const auto Retry = Fixture.Session->StartPreparedRun();
	TestTrue(TEXT("Same preserved intent may be submitted once more"), Retry.IsRunActive() && Fixture.Session->GetPreparationSnapshot().SelectedWeaponId == Weapon && Fixture.Session->GetPreparationSnapshot().OrderedSelectedMaterialIds == TArray<FGuid>({ Dust }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation11, "demo_map.ProfilePreparation.11.StaleReloadAndRecoveryClearSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation11::RunTest(const FString&)
{
	{
		FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
		Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot())); const auto Initial = Fixture.Session->GetPreparationSnapshot();
		Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationId(Initial, Fdemo_mapItemIds::TrainingBlade));
		Fdemo_mapBeginRunRequest Stale; Stale.ExpectedProfileId = Initial.ProfileId; Stale.ExpectedSaveGeneration = Initial.SaveGeneration - 1;
		const auto Result = Fixture.Session->BeginRun(Stale);
		TestTrue(TEXT("Stale Generation rejects without erasing the committed layout"), Result.Status == Edemo_mapProfileSessionBeginStatus::StaleIntent && Fixture.Session->GetPreparationSnapshot().SelectedWeaponId.IsValid());
	}
	{
		FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
		Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot())); const auto Initial = Fixture.Session->GetPreparationSnapshot();
		Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationId(Initial, Fdemo_mapItemIds::TrainingBlade));
		Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary);
		const auto Result = Fixture.Session->StartPreparedRun(); const auto Snapshot = Fixture.Session->GetPreparationSnapshot();
		TestTrue(TEXT("Reload or Recovery-required result keeps the committed layout available for reconciliation"), Result.Status == Edemo_mapProfileSessionBeginStatus::CommitOutcomeRequiresReload && Result.Snapshot.SessionState == Edemo_mapProfileSessionState::RecoveryRequired && Snapshot.SelectedWeaponId.IsValid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation12, "demo_map.ProfilePreparation.12.DeinitializeAndNewGameInstanceDoNotInherit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation12::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationRoot()); TArray<uint8> BeforeShutdown;
	{
		FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
		Fixture.Session->InitializeSession(Storage); const auto Initial = Fixture.Session->GetPreparationSnapshot();
		Fixture.Session->SetPreparationEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationId(Initial, Fdemo_mapItemIds::TrainingBlade));
		ReadPreparationBytes(Storage.PrimaryPath(), BeforeShutdown);
		TestTrue(TEXT("First GameInstance holds one in-memory selection"), Fixture.Session->GetPreparationSnapshot().SelectedWeaponId.IsValid());
	}
	TArray<uint8> AfterShutdown; ReadPreparationBytes(Storage.PrimaryPath(), AfterShutdown);
	FPreparationFixture Reloaded; if (!Reloaded.Start(*this)) return false;
	const auto Init = Reloaded.Session->InitializeSession(Storage); const auto Snapshot = Reloaded.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Deinitialize writes nothing"), BeforeShutdown == AfterShutdown);
	TestTrue(TEXT("New GameInstance reloads the committed preparation layout"), Init.IsReady() && Snapshot.bCanStartRun && Snapshot.SelectedWeaponId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparation13, "demo_map.ProfilePreparation.13.SingleNormalStartupOwnerAndProductionZeroIO", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparation13::RunTest(const FString&)
{
	FPreparationProductionSnapshot Production;
	FPreparationFixture Fixture; if (!Fixture.Start(*this)) return false;
	TestTrue(TEXT("A bare transient GameInstance remains lazy until the V3 startup owner selects a mode"), !Fixture.Session->IsExplicitlyInitialized() && Fixture.Session->GetPreparationSnapshot().SessionState == Edemo_mapProfileSessionState::Uninitialized);
	TArray<FString> Sources;
	IFileManager::Get().FindFilesRecursive(Sources, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("demo_map")), TEXT("*.cpp"), true, false);
	bool bUnexpected = false;
	for (const FString& Path : Sources)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path)
			|| Path.EndsWith(TEXT("Tests.cpp"))
			|| (!Text.Contains(TEXT("->InitializeSession("))
				&& !Text.Contains(TEXT("->StartPreparedRun("))
				&& !Text.Contains(TEXT("->SetPreparationEquipment("))
				&& !Text.Contains(TEXT("->SetPreparationMaterial("))))
		{
			continue;
		}
		if (!Path.EndsWith(TEXT("demo_mapProfileSessionSubsystem.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationWidget.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationUITests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationFlow.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationFlowTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileNormalStartupTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileTradeTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapSectNavigationWidget.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapEntityLoadoutTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapHotbarSliceTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapV3ProgressionManager.cpp")))
		{
			bUnexpected = true;
			AddError(FString::Printf(TEXT("Unexpected normal-startup Preparation activation reference: %s"), *Path));
		}
	}
	TestFalse(TEXT("Only explicit product owners may mutate or initialize Preparation"), bUnexpected);
	TestTrue(TEXT("Pure Preparation tests leave Production Save unchanged"), Production.IsUnchanged());
	return true;
}

#endif
