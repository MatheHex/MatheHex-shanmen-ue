#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfilePreparationWidget.h"
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
	FString NewPreparationUIRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.4.11.r0"),
			TEXT("ProfilePreparationUI"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	bool ReadPreparationUIBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	struct FPreparationUIProductionSnapshot
	{
		TArray<FString> Paths;
		TArray<bool> Existed;
		TArray<TArray<uint8>> Bytes;

		FPreparationUIProductionSnapshot()
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
					ReadPreparationUIBytes(Paths[Index], Value);
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
					if (!ReadPreparationUIBytes(Paths[Index], After) || After != Bytes[Index])
					{
						return false;
					}
				}
			}
			return true;
		}
	};

	struct FPreparationUIFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Udemo_mapItemSubsystem* Runtime = nullptr;
		bool bStarted = false;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for Preparation UI fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Failed to allocate Preparation UI GameInstance fixture."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			bStarted = true;
			Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
			Runtime = GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
			if (!Session || !Runtime)
			{
				Test.AddError(TEXT("Preparation UI fixture could not acquire existing GameInstance subsystems."));
				return false;
			}
			return true;
		}

		Udemo_mapProfilePreparationWidget* MakeWidget(FAutomationTestBase& Test)
		{
			Udemo_mapProfilePreparationWidget* Widget = NewObject<Udemo_mapProfilePreparationWidget>(GameInstance, NAME_None, RF_Transient);
			if (!Widget)
			{
				Test.AddError(TEXT("Failed to allocate transient Preparation C++ Widget."));
				return nullptr;
			}
			if (!Widget->Initialize())
			{
				Test.AddError(TEXT("Transient Preparation C++ Widget failed UUserWidget initialization."));
				return nullptr;
			}
			Widget->InitializeForSession(Session);
			return Widget;
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

		~FPreparationUIFixture()
		{
			Stop();
		}
	};

	FGuid AddPreparationUIStashRecord(Fdemo_mapPersistentProfile& Profile, FName DefinitionId, int32 StackCount)
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

	FGuid FindPreparationUIId(const Fdemo_mapProfilePreparationViewState& View, FName DefinitionId, int32 Occurrence = 0)
	{
		int32 Found = 0;
		for (const Fdemo_mapProfilePreparationRowView& Row : View.OrderedPermanentStashRows)
		{
			if (Row.ItemDefinitionId == DefinitionId && Found++ == Occurrence)
			{
				return Row.ItemInstanceId;
			}
		}
		return FGuid();
	}

	int32 FindPreparationUIRow(const Fdemo_mapProfilePreparationViewState& View, const FGuid& ItemId)
	{
		return View.OrderedPermanentStashRows.IndexOfByPredicate(
			[&ItemId](const Fdemo_mapProfilePreparationRowView& Row)
			{
				return Row.ItemInstanceId == ItemId;
			});
	}

	bool HasNoPreparationUISelection(const Fdemo_mapProfilePreparationViewState& View)
	{
		return View.OrderedSelectedMaterialIds.IsEmpty()
			&& !View.OrderedEquipmentSlots.ContainsByPredicate(
				[](const Fdemo_mapProfilePreparationEquipmentSlotView& Slot)
				{
					return Slot.ItemInstanceId.IsValid();
				})
			&& !View.OrderedPermanentStashRows.ContainsByPredicate(
				[](const Fdemo_mapProfilePreparationRowView& Row)
				{
					return Row.bSelected;
				});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI01, "demo_map.ProfilePreparationUI.01.UninitializedSafetyAndProductionProtection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI01::RunTest(const FString&)
{
	FPreparationUIProductionSnapshot Production;
	const FString UnusedRoot = NewPreparationUIRoot();
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const auto Select = Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, FGuid::NewGuid());
	const auto Start = Widget->RequestStartRun();
	const auto& View = Widget->GetViewState();
	TestTrue(TEXT("Real transient C++ Widget builds safely before session activation"), Widget->IsInterfaceBuilt() && View.SessionState == Edemo_mapProfileSessionState::Uninitialized && View.OrderedPermanentStashRows.IsEmpty());
	TestTrue(TEXT("Non-Preparation interaction rejects with visible diagnostic"), Select.Status == Edemo_mapProfilePreparationSelectionStatus::SessionNotReady && Start.Status == Edemo_mapProfileSessionBeginStatus::SessionNotReady && !Widget->GetLastInteractionDiagnostic().IsEmpty() && !View.bPreparationOperationsEnabled);
	TestFalse(TEXT("Unused isolated storage remains absent"), IFileManager::Get().DirectoryExists(*UnusedRoot));
	TestTrue(TEXT("Production path remains unchanged"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI02, "demo_map.ProfilePreparationUI.02.FreshRowsSlotsAndOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI02::RunTest(const FString&)
{
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot()));
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const auto& View = Widget->GetViewState();
	TestTrue(TEXT("Fresh view is Ready, empty-loadout startable, and button-backed"), Init.IsReady() && View.bPreparationOperationsEnabled && View.bCanStartRun && Widget->GetRowButtonCount() == View.OrderedPermanentStashRows.Num());
	TestTrue(TEXT("Fresh Stash order remains Weapon Armor SpatialRing"), View.OrderedPermanentStashRows.Num() == 3
		&& View.OrderedPermanentStashRows[0].ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade
		&& View.OrderedPermanentStashRows[1].ItemDefinitionId == Fdemo_mapItemIds::TrainingVest
		&& View.OrderedPermanentStashRows[2].ItemDefinitionId == Fdemo_mapItemIds::WindTalisman);
	TestTrue(TEXT("Five ordered equipment slots are presented"), View.OrderedEquipmentSlots.Num() == 5
		&& View.OrderedEquipmentSlots[0].SlotId == Fdemo_mapItemIds::WeaponSlot
		&& View.OrderedEquipmentSlots[1].SlotId == Fdemo_mapItemIds::ArmorSlot
		&& View.OrderedEquipmentSlots[2].SlotId == Fdemo_mapItemIds::AccessorySlot
		&& View.OrderedEquipmentSlots[3].SlotId == Fdemo_mapItemIds::SpatialRingSlot
		&& View.OrderedEquipmentSlots[4].SlotId == Fdemo_mapItemIds::BackpackSlot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI03, "demo_map.ProfilePreparationUI.03.RowDetailsRiskAndStartState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI03::RunTest(const FString&)
{
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot()));
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const FGuid Weapon = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade);
	const int32 WeaponRow = FindPreparationUIRow(Widget->GetViewState(), Weapon);
	const auto Result = Widget->ActivateStashRow(WeaponRow);
	const auto& View = Widget->GetViewState();
	const auto* Row = View.OrderedPermanentStashRows.FindByPredicate([&Weapon](const auto& Candidate){ return Candidate.ItemInstanceId == Weapon; });
	TestTrue(TEXT("Row exposes definition name quantity slot category and focus"), Result.IsAccepted() && Row && !Row->DisplayName.IsEmpty() && Row->StackCount == 1 && Row->CompatibleEquipmentSlotId == Fdemo_mapItemIds::WeaponSlot && Widget->GetFocusedItemId() == Weapon);
	TestTrue(TEXT("Selected safe Stash item is accurately precommit-risk labeled"), Row && Row->bSelected && Row->RiskLabel == TEXT("WILL BE AT RISK") && View.RiskPhase == Edemo_mapProfilePreparationRiskPhase::WillBeAtRisk && View.RiskLabel == TEXT("WILL BE AT RISK") && View.RiskWarning.Contains(TEXT("SAFE")));
	TestTrue(TEXT("Start remains enabled while selection is only an intent"), View.bCanStartRun && View.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI04, "demo_map.ProfilePreparationUI.04.EquipmentForwardingCancelAndCompatibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI04::RunTest(const FString&)
{
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot()));
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const FGuid Weapon = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade);
	const int32 WeaponRow = FindPreparationUIRow(Widget->GetViewState(), Weapon);
	TestTrue(TEXT("Weapon row forwards existing equipment intent"), Widget->ActivateStashRow(WeaponRow).IsAccepted() && Widget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId == Weapon);
	const auto Wrong = Widget->SelectEquipment(Fdemo_mapItemIds::ArmorSlot, Weapon);
	TestTrue(TEXT("Compatibility rejection is visible and preserves valid selection"), Wrong.Status == Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection && !Widget->GetLastInteractionDiagnostic().IsEmpty() && Widget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId == Weapon);
	TestTrue(TEXT("Second row activation cancels through existing API"), Widget->ActivateStashRow(WeaponRow).IsAccepted() && !Widget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI05, "demo_map.ProfilePreparationUI.05.MaterialWholeStacksAndCanonicalOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI05::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid Iron = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::IronShard, 3);
	const FGuid DustB = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 1);
	TestTrue(TEXT("Material UI fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	Widget->SelectMaterial(DustB, true); Widget->SelectMaterial(Iron, true); Widget->SelectMaterial(DustA, true);
	const TArray<FGuid> Expected = { DustB, Iron, DustA };
	const auto& View = Widget->GetViewState();
	TestTrue(TEXT("Persistent RunInventory preserves explicit selection order"), View.OrderedSelectedMaterialIds == Expected);
	TestTrue(TEXT("Material rows retain complete Stack quantities"), View.OrderedPermanentStashRows.FindByPredicate([&DustA](const auto& Row){ return Row.ItemInstanceId == DustA; })->StackCount == 5
		&& View.OrderedPermanentStashRows.FindByPredicate([&Iron](const auto& Row){ return Row.ItemInstanceId == Iron; })->StackCount == 3
		&& View.OrderedPermanentStashRows.FindByPredicate([&DustB](const auto& Row){ return Row.ItemInstanceId == DustB; })->StackCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI06, "demo_map.ProfilePreparationUI.06.VisibleInvalidDuplicateAndLimitRejections", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI06::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid IronA = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::IronShard, 4);
	const FGuid DustB = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2);
	const FGuid IronB = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::IronShard, 1);
	const FGuid Token = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::AncientToken, 1);
	TestTrue(TEXT("Invalid UI fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	Widget->SelectMaterial(DustA, true); Widget->SelectMaterial(IronA, true); Widget->SelectMaterial(DustB, true);
	const auto Duplicate = Widget->SelectMaterial(DustA, true);
	const auto Fourth = Widget->SelectMaterial(IronB, true);
	const auto StashOnly = Widget->SelectMaterial(Token, true);
	const auto Missing = Widget->SelectMaterial(FGuid::NewGuid(), true);
	TestTrue(TEXT("Duplicate stash-only and missing IDs visibly reject while a fourth quick-cell Stack is accepted"), Duplicate.Status == Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection
		&& Fourth.IsAccepted()
		&& StashOnly.Status == Edemo_mapProfilePreparationSelectionStatus::MaterialRejected
		&& Missing.Status == Edemo_mapProfilePreparationSelectionStatus::ItemNotFound
		&& !Widget->GetLastInteractionDiagnostic().IsEmpty());
	TestTrue(TEXT("Rejected UI operations preserve four valid selections"), Widget->GetViewState().OrderedSelectedMaterialIds == TArray<FGuid>({ DustA, IronA, DustB, IronB }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI07, "demo_map.ProfilePreparationUI.07.ClearRefreshNoPersistentOrRuntimeMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI07::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Dust = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	TestTrue(TEXT("No-mutation UI fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const auto BeforeSession = Fixture.Session->GetSnapshot(); TArray<uint8> Before; ReadPreparationUIBytes(Storage.PrimaryPath(), Before);
	Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade));
	Widget->SelectMaterial(Dust, true); Widget->RefreshFromSession(); Widget->ClearAllSelection(); Widget->RefreshFromSession();
	const auto AfterSession = Fixture.Session->GetSnapshot(); TArray<uint8> After; ReadPreparationUIBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Persistent selections commit atomically and Clear preserves identity and Stash"), Before != After && BeforeSession.ProfileId == AfterSession.ProfileId && BeforeSession.SaveGeneration + 3 == AfterSession.SaveGeneration && BeforeSession.OrderedPermanentStash == AfterSession.OrderedPermanentStash && AfterSession.PreparationLayout.IsEmpty());
	TestTrue(TEXT("Clear and Refresh leave Runtime inactive and view empty"), Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty() && HasNoPreparationUISelection(Widget->GetViewState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI08, "demo_map.ProfilePreparationUI.08.StaleReloadRecoveryDiagnosticsAndClear", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI08::RunTest(const FString&)
{
	{
		FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
		Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot()));
		Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
		const auto Initial = Widget->GetViewState();
		Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationUIId(Initial, Fdemo_mapItemIds::TrainingBlade));
		Fdemo_mapBeginRunRequest Stale; Stale.ExpectedProfileId = Initial.ProfileId; Stale.ExpectedSaveGeneration = Initial.SaveGeneration - 1;
		const auto Result = Fixture.Session->BeginRun(Stale); Widget->RefreshFromSession();
		TestTrue(TEXT("Stale request is visible and cannot erase the committed layout"), Result.Status == Edemo_mapProfileSessionBeginStatus::StaleIntent && Widget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId.IsValid() && !Widget->GetViewState().VisibleDiagnostic.IsEmpty());
	}
	{
		FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
		Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot()));
		Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
		Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade));
		Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary);
		const auto Result = Widget->RequestStartRun();
		TestTrue(TEXT("Reload recovery-required state disables UI and exposes durable reconciliation"), Result.Status == Edemo_mapProfileSessionBeginStatus::CommitOutcomeRequiresReload
			&& Widget->GetViewState().SessionState == Edemo_mapProfileSessionState::RecoveryRequired
			&& !Widget->GetViewState().bPreparationOperationsEnabled
			&& !Widget->GetLastInteractionDiagnostic().IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI09, "demo_map.ProfilePreparationUI.09.EmptyLoadoutStartForwarding", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI09::RunTest(const FString&)
{
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot()));
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	TestTrue(TEXT("Empty loadout is shown startable"), Widget->GetViewState().bCanStartRun && HasNoPreparationUISelection(Widget->GetViewState()));
	const auto Result = Widget->RequestStartRun();
	TestTrue(TEXT("UI forwards empty Start to existing unique BeginRun transaction"), Result.IsRunActive() && Result.Snapshot.ActiveRunId.IsValid() && Result.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId() && Result.RuntimeResult.DeployedItemIds.IsEmpty());
	TestTrue(TEXT("RunActive UI is AT RISK and Preparation-disabled"), Widget->GetViewState().SessionState == Edemo_mapProfileSessionState::RunActive && Widget->GetViewState().RiskPhase == Edemo_mapProfilePreparationRiskPhase::AtRisk && !Widget->GetViewState().bPreparationOperationsEnabled && !Widget->GetViewState().bCanStartRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI10, "demo_map.ProfilePreparationUI.10.MaximumSixIdentityAndRunActive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI10::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid Iron = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::IronShard, 3);
	const FGuid DustB = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 1);
	TestTrue(TEXT("Maximum UI fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const FGuid Weapon = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade);
	const FGuid Armor = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingVest);
	const FGuid SpatialRing = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::WindTalisman);
	Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, Weapon);
	Widget->SelectEquipment(Fdemo_mapItemIds::ArmorSlot, Armor);
	Widget->SelectEquipment(Fdemo_mapItemIds::SpatialRingSlot, SpatialRing);
	Widget->SelectMaterial(DustB, true); Widget->SelectMaterial(Iron, true); Widget->SelectMaterial(DustA, true);
	const TArray<FGuid> Expected = { Weapon, Armor, SpatialRing, DustB, Iron, DustA };
	TestTrue(TEXT("Maximum six committed preparation records display WILL BE AT RISK"), Widget->GetViewState().RiskPhase == Edemo_mapProfilePreparationRiskPhase::WillBeAtRisk && Widget->GetViewState().OrderedSelectedMaterialIds == TArray<FGuid>({ DustB, Iron, DustA }));
	const auto Result = Widget->RequestStartRun();
	TestTrue(TEXT("Maximum UI Start preserves original IDs and unique RunId"), Result.IsRunActive() && Result.PersistentResult.CommittedLoadoutPlan.IsSet() && Result.PersistentResult.CommittedLoadoutPlan->DeployedItemIds == Expected && Result.RuntimeResult.DeployedItemIds == Expected && Result.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId());
	TestTrue(TEXT("Successful Start retains committed layout and enters disabled AT RISK state"), Widget->GetViewState().RiskLabel == TEXT("AT RISK") && !Widget->GetViewState().bPreparationOperationsEnabled && Widget->GetViewState().OrderedSelectedMaterialIds == TArray<FGuid>({ DustB, Iron, DustA }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI11, "demo_map.ProfilePreparationUI.11.PrecommitFailureVisibleRetryOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI11::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Dust = AddPreparationUIStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	TestTrue(TEXT("Retry UI fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
	Fixture.Session->InitializeSession(Storage);
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const FGuid Weapon = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade);
	Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, Weapon); Widget->SelectMaterial(Dust, true);
	TArray<uint8> Before; ReadPreparationUIBytes(Storage.PrimaryPath(), Before);
	Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp);
	const auto Failed = Widget->RequestStartRun(); TArray<uint8> After; ReadPreparationUIBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Precommit failure is visible and preserves same identity and intent"), Failed.Status == Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected && Before == After && Widget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId == Weapon && Widget->GetViewState().OrderedSelectedMaterialIds == TArray<FGuid>({ Dust }) && !Widget->GetLastInteractionDiagnostic().IsEmpty());
	const auto Retry = Widget->RequestStartRun();
	TestTrue(TEXT("Same UI intent succeeds on one normal retry"), Retry.IsRunActive() && Retry.RuntimeResult.DeployedItemIds == TArray<FGuid>({ Weapon, Dust }) && Widget->GetViewState().SessionState == Edemo_mapProfileSessionState::RunActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationUI12, "demo_map.ProfilePreparationUI.12.DestroyDeinitializeNewGameInstanceAndStartupIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationUI12::RunTest(const FString&)
{
	FPreparationUIProductionSnapshot Production;
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewPreparationUIRoot());
	TArray<uint8> BeforeShutdown;
	{
		FPreparationUIFixture Fixture; if (!Fixture.Start(*this)) return false;
		Fixture.Session->InitializeSession(Storage);
		Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
		const FGuid Weapon = FindPreparationUIId(Widget->GetViewState(), Fdemo_mapItemIds::TrainingBlade);
		Widget->ActivateStashRow(FindPreparationUIRow(Widget->GetViewState(), Weapon));
		ReadPreparationUIBytes(Storage.PrimaryPath(), BeforeShutdown);
		Widget->MarkAsGarbage(); Widget = nullptr; CollectGarbage(RF_NoFlags);
		Udemo_mapProfilePreparationWidget* Replacement = Fixture.MakeWidget(*this); if (!Replacement) return false;
		TestTrue(TEXT("Replacement Widget does not inherit focus or interaction feedback"), !Replacement->GetFocusedItemId().IsValid() && Replacement->GetLastInteractionDiagnostic().IsEmpty());
		TestTrue(TEXT("Widget destruction does not mutate Subsystem intent or disk"), Replacement->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId == Weapon);
	}
	TArray<uint8> AfterShutdown; ReadPreparationUIBytes(Storage.PrimaryPath(), AfterShutdown);
	FPreparationUIFixture Reloaded; if (!Reloaded.Start(*this)) return false;
	TestTrue(TEXT("Bare new GameInstance remains lazy until the V3 startup owner runs"), !Reloaded.Session->IsExplicitlyInitialized() && Reloaded.Session->GetPreparationSnapshot().SessionState == Edemo_mapProfileSessionState::Uninitialized);
	const auto Init = Reloaded.Session->InitializeSession(Storage);
	Udemo_mapProfilePreparationWidget* ReloadedWidget = Reloaded.MakeWidget(*this); if (!ReloadedWidget) return false;
	TestTrue(TEXT("Deinitialize writes nothing and new GameInstance reloads the committed layout"), BeforeShutdown == AfterShutdown && Init.IsReady() && ReloadedWidget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId == Reloaded.Session->GetSnapshot().PreparationLayout.WeaponItemInstanceId && ReloadedWidget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId.IsValid());

	TArray<FString> Sources;
	IFileManager::Get().FindFilesRecursive(Sources, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("demo_map")), TEXT("*.cpp"), true, false);
	bool bUnexpectedActivation = false;
	for (const FString& Path : Sources)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path) || !Text.Contains(TEXT("Udemo_mapProfilePreparationWidget")))
		{
			continue;
		}
		if (!Path.EndsWith(TEXT("demo_mapProfilePreparationWidget.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationUITests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationFlow.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationFlowTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileNormalStartupTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileTradeTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapEntityLoadoutTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapP7IntegrationTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapV3ProgressionManager.cpp")))
		{
			bUnexpectedActivation = true;
			AddError(FString::Printf(TEXT("Unexpected normal-startup Preparation UI activation: %s"), *Path));
		}
	}
	TestFalse(TEXT("Only the existing Flow and V3 Manager may activate Preparation UI outside tests"), bUnexpectedActivation);
	TestTrue(TEXT("UI tests leave Production Save unchanged"), Production.IsUnchanged());
	return true;
}

#endif
