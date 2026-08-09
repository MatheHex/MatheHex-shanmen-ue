#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfilePreparationFlow.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapProfileStartupMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewNormalStartupRoot()
	{
		return FPaths::Combine(
			Fdemo_mapProfilePreparationFlow::AllowedAutomationRoot(),
			TEXT("ProfileNormalStartup"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	bool ReadNormalStartupBytes(const FString& Path, TArray<uint8>& Out)
	{
		Out.Reset();
		return FFileHelper::LoadFileToArray(Out, *Path);
	}

	FGuid FindNormalStartupItem(const Fdemo_mapProfileSessionSnapshot& Snapshot, FName DefinitionId)
	{
		for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
		{
			if (Item.ItemDefinitionId == DefinitionId) return Item.ItemInstanceId;
		}
		return FGuid();
	}

	bool NormalStartupContains(const Fdemo_mapProfileSessionSnapshot& Snapshot, const FGuid& ItemId)
	{
		return Snapshot.OrderedPermanentStash.ContainsByPredicate(
			[&ItemId](const Fdemo_mapPersistentItemRecord& Item)
			{
				return Item.ItemInstanceId == ItemId;
			});
	}

	struct FNormalStartupProductionSnapshot
	{
		TArray<FString> Paths;
		TArray<bool> Existed;
		TArray<TArray<uint8>> Bytes;

		FNormalStartupProductionSnapshot()
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
				if (Index > 0 && bExists) FFileHelper::LoadFileToArray(Value, *Paths[Index]);
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
				if (bExists != Existed[Index]) return false;
				if (Index > 0 && bExists)
				{
					TArray<uint8> After;
					if (!FFileHelper::LoadFileToArray(After, *Paths[Index]) || After != Bytes[Index]) return false;
				}
			}
			return true;
		}
	};

	struct FNormalStartupFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Udemo_mapItemSubsystem* Runtime = nullptr;
		bool bStarted = false;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for ProfileNormalStartup fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate ProfileNormalStartup GameInstance."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			bStarted = true;
			Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
			Runtime = GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
			return Session && Runtime;
		}

		Udemo_mapProfilePreparationWidget* MakeWidget(FAutomationTestBase& Test)
		{
			Udemo_mapProfilePreparationWidget* Widget = NewObject<Udemo_mapProfilePreparationWidget>(GameInstance, NAME_None, RF_Transient);
			if (!Widget || !Widget->Initialize())
			{
				Test.AddError(TEXT("Could not initialize the real programmatic Preparation Widget."));
				return nullptr;
			}
			Widget->InitializeForSession(Session);
			return Widget;
		}

		void Stop()
		{
			if (!GameInstance) return;
			if (bStarted) GameInstance->Shutdown();
			Session = nullptr;
			Runtime = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			bStarted = false;
			CollectGarbage(RF_NoFlags);
		}

		~FNormalStartupFixture() { Stop(); }
	};

	bool InitializeNormalStartup(
		FAutomationTestBase& Test,
		FNormalStartupFixture& Fixture,
		Fdemo_mapProfilePreparationFlow& Flow,
		const FString& Root)
	{
		if (!Fixture.Start(Test)) return false;
		const Fdemo_mapProfileSessionInitializeResult Init = Flow.InitializeExplicit(Fixture.GameInstance, Root);
		if (!Init.IsReady())
		{
			Test.AddError(FString::Printf(TEXT("ProfileNormalStartup initialization failed: %s"), *Init.Diagnostic));
			return false;
		}
		return true;
	}

	Fdemo_mapProfileSessionBeginResult BeginNormalStartup(
		FAutomationTestBase& Test,
		FNormalStartupFixture& Fixture,
		Fdemo_mapProfilePreparationFlow& Flow,
		const TArray<TPair<FName, FGuid>>& Equipment = {},
		const TArray<FGuid>& Materials = {})
	{
		Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(Test);
		if (!Widget) return Fdemo_mapProfileSessionBeginResult();
		for (const TPair<FName, FGuid>& Entry : Equipment)
		{
			const Fdemo_mapProfilePreparationSelectionResult Selection = Widget->SelectEquipment(Entry.Key, Entry.Value);
			if (!Selection.IsAccepted()) Test.AddError(Selection.Diagnostic);
		}
		for (const FGuid& ItemId : Materials)
		{
			const Fdemo_mapProfilePreparationSelectionResult Selection = Widget->SelectMaterial(ItemId, true);
			if (!Selection.IsAccepted()) Test.AddError(Selection.Diagnostic);
		}
		return Flow.StartPreparedRunThroughWidget(Widget);
	}

	Fdemo_mapProfileSessionSettlementResult SettleNormalStartup(
		FNormalStartupFixture& Fixture,
		Fdemo_mapProfilePreparationFlow& Flow,
		Edemo_mapRunEndReason Reason,
		Fdemo_mapSettlementSummary* OutSummary = nullptr)
	{
		Fdemo_mapSettlementSummary Summary;
		if (!Fixture.Runtime->RequestSettlement(Reason, Summary).bSuccess)
		{
			return Fdemo_mapProfileSessionSettlementResult();
		}
		if (OutSummary) *OutSummary = Summary;
		return Flow.CommitRuntimeSettlement(Summary);
	}

	void AddNormalStartupMaterials(const FString& Root)
	{
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
		Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Fdemo_mapPersistentItemRecord Item;
			Item.ItemInstanceId = FGuid::NewGuid();
			Item.ItemDefinitionId = Index == 1 ? Fdemo_mapItemIds::IronShard : Fdemo_mapItemIds::SpiritDust;
			Item.StackCount = Index + 1;
			Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			Profile.PermanentStash.Add(Item);
		}
		Repository.SaveProfile(Profile, Storage);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup01, "demo_map.ProfileNormalStartup.01.DefaultV3SelectsProductionWithoutLegacyBegin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup01::RunTest(const FString&)
{
	Fdemo_mapProfileStartupInputs Inputs; Inputs.bIsV3World = true;
	const Edemo_mapProfileStartupMode Mode = Fdemo_mapProfileStartupModeSelector::Select(Inputs);
	TestTrue(TEXT("Default V3 selects the product Profile lifecycle"), Mode == Edemo_mapProfileStartupMode::ProductionProfile && Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(Mode));
	TestFalse(TEXT("Production Profile mode never selects the legacy Runtime Begin path"), Fdemo_mapProfileStartupModeSelector::UsesLegacyRuntimeBegin(Mode));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup02, "demo_map.ProfileNormalStartup.02.V2ToolAndLegacyAutomationStayDisconnected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup02::RunTest(const FString&)
{
	FNormalStartupProductionSnapshot Production;
	Fdemo_mapProfileStartupInputs V2;
	Fdemo_mapProfileStartupInputs Tool; Tool.bIsV3World = true; Tool.bIsEditorToolWorld = true;
	Fdemo_mapProfileStartupInputs Legacy; Legacy.bIsV3World = true; Legacy.bLegacyAutomationRequested = true; Legacy.bProfileAutomationRequested = true;
	TestTrue(TEXT("V2 and editor tool scenes remain disconnected"), Fdemo_mapProfileStartupModeSelector::Select(V2) == Edemo_mapProfileStartupMode::Disconnected && Fdemo_mapProfileStartupModeSelector::Select(Tool) == Edemo_mapProfileStartupMode::Disconnected);
	TestTrue(TEXT("Legacy automation has priority and does not select Production"), Fdemo_mapProfileStartupModeSelector::Select(Legacy) == Edemo_mapProfileStartupMode::LegacyAutomation && Fdemo_mapProfileStartupModeSelector::UsesLegacyRuntimeBegin(Fdemo_mapProfileStartupModeSelector::Select(Legacy)));
	TestTrue(TEXT("Mode selection performs zero Production I/O"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup03, "demo_map.ProfileNormalStartup.03.ProductionStorageResolutionIsPure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup03::RunTest(const FString&)
{
	FNormalStartupProductionSnapshot ProductionBefore;
	const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
	const FString Expected = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Shanmen"));
	TestTrue(TEXT("Production storage resolves through the authoritative context"), FPaths::IsSamePath(Production.RootDirectory, Expected) && Production.PrimaryPath().StartsWith(Production.RootDirectory));
	TestTrue(TEXT("Resolving Production storage does not create or write it"), ProductionBefore.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup04, "demo_map.ProfileNormalStartup.04.AutomationRootCannotEscapeOrOverlapProduction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup04::RunTest(const FString&)
{
	FString Canonical, Diagnostic;
	TestTrue(TEXT("Unique task-isolated root is accepted"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(NewNormalStartupRoot(), Canonical, Diagnostic));
	TestFalse(TEXT("Production root is rejected"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(Fdemo_mapProfileStorageContext::Production().RootDirectory, Canonical, Diagnostic));
	TestFalse(TEXT("Sibling task root is rejected"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("OtherTask")), Canonical, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup05, "demo_map.ProfileNormalStartup.05.IdleInitializationShowsPreparationBeforeRuntime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup05::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow;
	if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this);
	TestTrue(TEXT("Fresh/Idle lifecycle exposes the real Preparation view first"), Widget && Widget->GetViewState().SessionState == Edemo_mapProfileSessionState::ReadyForPreparation && Widget->GetViewState().bCanStartRun);
	TestTrue(TEXT("Runtime remains inactive before Start"), Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && !Fixture.Runtime->GetActiveRunId().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup06, "demo_map.ProfileNormalStartup.06.WidgetRebuildAndInputLockContractAreIdempotent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup06::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow;
	if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	const int32 RowCount = Widget->GetRowButtonCount(); const FGuid ProfileId = Widget->GetViewState().ProfileId;
	Widget->InitializeForSession(Fixture.Session); Widget->InitializeForSession(Fixture.Session);
	FString ManagerSource, ControllerSource;
	FFileHelper::LoadFileToString(ManagerSource, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/demo_map/demo_mapV3ProgressionManager.cpp")));
	FFileHelper::LoadFileToString(ControllerSource, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/demo_map/demo_mapPlayerController.cpp")));
	TestTrue(TEXT("Repeated Widget initialization retains one interface and value projection"), Widget->IsInterfaceBuilt() && Widget->GetRowButtonCount() == RowCount && Widget->GetViewState().ProfileId == ProfileId);
	TestTrue(TEXT("Normal owner applies UI-only input lock/focus and restores gameplay through the existing controller"), ManagerSource.Contains(TEXT("BeginProfilePreparationInputLock")) && ControllerSource.Contains(TEXT("SetWidgetToFocus")) && ControllerSource.Contains(TEXT("RestoreGameplayControlForNewRun")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup07, "demo_map.ProfileNormalStartup.07.FutureAndCorruptProfilesRemainFatalAndByteExact", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup07::RunTest(const FString&)
{
	auto VerifyRejected = [this](bool bFuture)
	{
		const FString Root = NewNormalStartupRoot(); const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); Fdemo_mapProfileRepository Repository;
		Repository.LoadOrCreateDefaultProfile(Storage); IFileManager::Get().Delete(*Storage.BackupPath(), false, true, true);
		TArray<uint8> Authority;
		if (bFuture)
		{
			ReadNormalStartupBytes(Storage.PrimaryPath(), Authority);
			FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Authority.GetData()), Authority.Num()); FString Json(Converted.Length(), Converted.Get());
			const FString CurrentSchema = FString::Printf(
				TEXT("\"SchemaVersion\":%d"),
				Fdemo_mapPersistentProfile::CurrentSchemaVersion);
			Json.ReplaceInline(*CurrentSchema, TEXT("\"SchemaVersion\":999"), ESearchCase::CaseSensitive);
			FTCHARToUTF8 Utf8(*Json); Authority.Reset(); Authority.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		}
		else
		{
			Authority = { 0x00, 0x01, 0x02, 0x03 };
		}
		FFileHelper::SaveArrayToFile(Authority, *Storage.PrimaryPath());
		FNormalStartupFixture Fixture; if (!Fixture.Start(*this)) return false; Fdemo_mapProfilePreparationFlow Flow;
		const Fdemo_mapProfileSessionInitializeResult Init = Flow.InitializeExplicit(Fixture.GameInstance, Root); TArray<uint8> After; ReadNormalStartupBytes(Storage.PrimaryPath(), After);
		return Init.Status == Edemo_mapProfileSessionInitializeStatus::FatalProfileError && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::RecoveryRequired && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && After == Authority;
	};
	TestTrue(TEXT("Future Schema stays fatal, world-off and byte-exact"), VerifyRejected(true));
	TestTrue(TEXT("Corrupt primary without backup stays fatal, world-off and byte-exact"), VerifyRejected(false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup08, "demo_map.ProfileNormalStartup.08.EmptyLoadoutCreatesOneSharedRunId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup08::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	const Fdemo_mapProfileSessionBeginResult Begin = BeginNormalStartup(*this, Fixture, Flow); const Fdemo_mapItemOperationResult Duplicate = Fixture.Runtime->BeginRun();
	TestTrue(TEXT("Empty Start creates one Session/Runtime RunId"), Begin.IsRunActive() && Begin.Snapshot.ActiveRunId.IsValid() && Begin.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId() && Begin.Snapshot.ActiveRunId == Flow.GetStartedRunId());
	TestFalse(TEXT("Legacy Begin cannot create a second active Run"), Duplicate.bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup17, "demo_map.ProfileNormalStartup.17.P4xDirectStartIgnoresPreparationLayout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup17::RunTest(const FString&)
{
	const FString Root = NewNormalStartupRoot();
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
	Fdemo_mapProfileRepository Repository;
	Fdemo_mapPersistentProfile LegacyProfile = Repository.CreateFreshProfile();
	LegacyProfile.PreparationLayout.WeaponItemInstanceId = FGuid::NewGuid();
	LegacyProfile.PreparationLayout.SpatialRingItemInstanceId = FGuid::NewGuid();
	LegacyProfile.PreparationLayout.HotbarItemInstanceIds.SetNum(2);
	TestTrue(TEXT("P4x fixture persists deliberately stale legacy preparation references"), Repository.SaveProfile(LegacyProfile, Storage).IsSuccess());

	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, Root)) return false;
	const Fdemo_mapProfileSessionSnapshot Before = Fixture.Session->GetSnapshot();
	const Fdemo_mapProfileSessionBeginResult Begin = Flow.StartPreparedRunDirect();
	TestTrue(TEXT("P4x direct Start Run succeeds without constructing a preparation widget or selecting equipment"), Begin.IsRunActive() && Begin.Snapshot.ActiveRunId.IsValid() && Begin.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId());
	TestTrue(TEXT("P4x direct Start Run deploys no implicit preparation items"), Fixture.Runtime->GetDeployedItemIds().IsEmpty());
	TestTrue(TEXT("P4x direct Start Run ignores rather than clears the stale legacy preparation layout"), !Before.PreparationLayout.IsEmpty() && Begin.Snapshot.PreparationLayout == Before.PreparationLayout);
	const Fdemo_mapProfileSessionSettlementResult Settlement = SettleNormalStartup(Fixture, Flow, Edemo_mapRunEndReason::Extraction);
	const Fdemo_mapProfileSessionBeginResult Restart = Flow.StartPreparedRunDirect();
	TestTrue(TEXT("P4x can settle and directly Start Run again without reviving a preparation gate"), Settlement.IsDurablySettled() && Restart.IsRunActive() && Restart.Snapshot.ActiveRunId.IsValid() && Restart.Snapshot.ActiveRunId != Begin.Snapshot.ActiveRunId && Fixture.Runtime->GetDeployedItemIds().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup09, "demo_map.ProfileNormalStartup.09.MaximumSixKeepCanonicalIdentityAndOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup09::RunTest(const FString&)
{
	const FString Root = NewNormalStartupRoot(); AddNormalStartupMaterials(Root); FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, Root)) return false;
	const Fdemo_mapProfileSessionSnapshot Snapshot = Fixture.Session->GetSnapshot();
	TArray<TPair<FName, FGuid>> Equipment = {
		{ Fdemo_mapItemIds::WeaponSlot, FindNormalStartupItem(Snapshot, Fdemo_mapItemIds::TrainingBlade) },
		{ Fdemo_mapItemIds::ArmorSlot, FindNormalStartupItem(Snapshot, Fdemo_mapItemIds::TrainingVest) },
		{ Fdemo_mapItemIds::SpatialRingSlot, FindNormalStartupItem(Snapshot, Fdemo_mapItemIds::WindTalisman) } };
	TArray<FGuid> Materials; for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash) if (Item.ItemDefinitionId == Fdemo_mapItemIds::SpiritDust || Item.ItemDefinitionId == Fdemo_mapItemIds::IronShard) Materials.Add(Item.ItemInstanceId);
	const Fdemo_mapProfileSessionBeginResult Begin = BeginNormalStartup(*this, Fixture, Flow, Equipment, Materials); TSet<FGuid> Unique; for (const FGuid& ItemId : Begin.RuntimeResult.DeployedItemIds) Unique.Add(ItemId);
	TestTrue(TEXT("Maximum loadout preserves six original identities in canonical order"), Begin.IsRunActive() && Begin.RuntimeResult.DeployedItemIds.Num() == 6 && Unique.Num() == 6 && Begin.PersistentResult.CommittedLoadoutPlan.IsSet() && Begin.RuntimeResult.DeployedItemIds == Begin.PersistentResult.CommittedLoadoutPlan->DeployedItemIds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup10, "demo_map.ProfileNormalStartup.10.PrecommitFailurePreservesSelectionForOneRetry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup10::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	const FGuid Blade = FindNormalStartupItem(Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); if (!Widget) return false;
	Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, Blade); Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp);
	const Fdemo_mapProfileSessionBeginResult Failed = Flow.StartPreparedRunThroughWidget(Widget); const FGuid Preserved = Widget->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId; const Fdemo_mapProfileSessionBeginResult Retry = Flow.StartPreparedRunThroughWidget(Widget);
	TestTrue(TEXT("Precommit rejection preserves selection and the normal retry succeeds once"), Failed.Status == Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected && Preserved == Blade && Retry.IsRunActive() && Retry.RuntimeResult.DeployedItemIds == TArray<FGuid>({ Blade }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup11, "demo_map.ProfileNormalStartup.11.RuntimeMaterializationFailureRollsBackAndRecovers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup11::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	const FGuid Blade = FindNormalStartupItem(Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); Fixture.Runtime->SetPreparedRunFailureAfterMutationForAutomation(1);
	const Fdemo_mapProfileSessionBeginResult Begin = BeginNormalStartup(*this, Fixture, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }});
	TestTrue(TEXT("Materialization failure rolls Runtime back and exposes Recovery without world activation"), Begin.Status == Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::RecoveryRequired && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup12, "demo_map.ProfileNormalStartup.12.WorldActivationFailureRollsBackWithoutPlayerAbandon", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup12::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	const Fdemo_mapProfileSessionBeginResult Begin = BeginNormalStartup(*this, Fixture, Flow); const Fdemo_mapProfileSessionSettlementResult End = Flow.CancelActiveRunForActivationFailure(); const Fdemo_mapProfileSessionSettlementResult Again = Flow.CancelActiveRunForActivationFailure();
	TestTrue(TEXT("Activation rollback submits exactly one non-player terminal transaction"), Begin.IsRunActive() && End.IsDurablySettled() && End.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::ActivationFailure && Flow.GetSettlementSubmitCount() == 1 && Again.Status == Edemo_mapProfileSessionSettlementStatus::SessionStateRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup13, "demo_map.ProfileNormalStartup.13.ExtractionReturnsSameIdentityToPreparation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup13::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	const FGuid Blade = FindNormalStartupItem(Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); const Fdemo_mapProfileSessionBeginResult Begin = BeginNormalStartup(*this, Fixture, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); const Fdemo_mapProfileSessionSettlementResult End = SettleNormalStartup(Fixture, Flow, Edemo_mapRunEndReason::Extraction);
	TestTrue(TEXT("Extraction commits once and returns the original identity to Preparation"), Begin.IsRunActive() && End.IsDurablySettled() && NormalStartupContains(End.Snapshot, Blade) && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation && Flow.GetSettlementSubmitCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup14, "demo_map.ProfileNormalStartup.14.DeathRemovesRiskAndPreservesSafeStash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup14::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	const Fdemo_mapProfileSessionSnapshot Before = Fixture.Session->GetSnapshot(); const FGuid Blade = FindNormalStartupItem(Before, Fdemo_mapItemIds::TrainingBlade); const FGuid Vest = FindNormalStartupItem(Before, Fdemo_mapItemIds::TrainingVest);
	BeginNormalStartup(*this, Fixture, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); const Fdemo_mapProfileSessionSettlementResult End = SettleNormalStartup(Fixture, Flow, Edemo_mapRunEndReason::Death);
	TestTrue(TEXT("Death removes the risk identity from every domain and keeps safe Stash"), End.IsDurablySettled() && !NormalStartupContains(End.Snapshot, Blade) && !Fixture.Runtime->GetAuthority().FindInstance(Blade) && NormalStartupContains(End.Snapshot, Vest) && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup15, "demo_map.ProfileNormalStartup.15.PendingRetryReplaysSameEvidenceOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup15::RunTest(const FString&)
{
	FNormalStartupFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, Fixture, Flow, NewNormalStartupRoot())) return false;
	BeginNormalStartup(*this, Fixture, Flow); Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp); const Fdemo_mapProfileSessionSettlementResult Pending = SettleNormalStartup(Fixture, Flow, Edemo_mapRunEndReason::Extraction); const FGuid EvidenceRunId = Pending.PersistentResult.ActiveRunId;
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); const bool bRetryVisible = Widget && Widget->GetViewState().bCanRetrySettlement; const Fdemo_mapProfileSessionBeginResult Blocked = Flow.StartPreparedRunThroughWidget(Widget); const Fdemo_mapProfileSessionSettlementResult Retry = Flow.RetryPendingSettlement(); const Fdemo_mapProfileSessionSettlementResult Again = Flow.RetryPendingSettlement();
	TestTrue(TEXT("Pending blocks Start and exposes one retry of the same immutable evidence"), Pending.Status == Edemo_mapProfileSessionSettlementStatus::PendingRetry && EvidenceRunId.IsValid() && !Pending.Snapshot.LastSettlementId.IsValid() && bRetryVisible && Blocked.Status == Edemo_mapProfileSessionBeginStatus::SessionNotReady && Retry.IsDurablySettled() && Retry.PersistentResult.ActiveRunId == EvidenceRunId && Retry.PersistentResult.SettlementId.IsValid() && Retry.Snapshot.LastSettlementId == Retry.PersistentResult.SettlementId && Retry.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::Extraction && Again.Status == Edemo_mapProfileSessionSettlementStatus::SessionStateRejected && Flow.GetSettlementRetryCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileNormalStartup16, "demo_map.ProfileNormalStartup.16.TeardownPreservesPreparedForSingleRecoveredAbandon", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileNormalStartup16::RunTest(const FString&)
{
	FNormalStartupProductionSnapshot Production; const FString Root = NewNormalStartupRoot(); FGuid Blade, RunId, SettlementId; int32 RecoveredGeneration = 0;
	{
		FNormalStartupFixture First; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeNormalStartup(*this, First, Flow, Root)) return false;
		Blade = FindNormalStartupItem(First.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); const Fdemo_mapProfileSessionBeginResult Begin = BeginNormalStartup(*this, First, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); RunId = Begin.Snapshot.ActiveRunId; Flow.Unbind();
	}
	{
		FNormalStartupFixture Recovery; if (!Recovery.Start(*this)) return false; Fdemo_mapProfilePreparationFlow Flow; const Fdemo_mapProfileSessionInitializeResult Init = Flow.InitializeExplicit(Recovery.GameInstance, Root);
		TestTrue(TEXT("New process resolves preserved Prepared Run once as RecoveredAbandon"), Init.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && Init.Snapshot.ActiveRunId == RunId && Init.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon && !NormalStartupContains(Init.Snapshot, Blade));
		SettlementId = Init.Snapshot.LastSettlementId; RecoveredGeneration = Init.Snapshot.SaveGeneration;
	}
	FNormalStartupFixture Idempotent; if (!Idempotent.Start(*this)) return false; Fdemo_mapProfilePreparationFlow Reloaded; const Fdemo_mapProfileSessionInitializeResult Reload = Reloaded.InitializeExplicit(Idempotent.GameInstance, Root);
	TestTrue(TEXT("Further startup neither rebinds nor re-settles the recovered run"), Reload.IsReady() && Reload.Status != Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && Reload.Snapshot.LastSettlementId == SettlementId && Reload.Snapshot.SaveGeneration == RecoveredGeneration && Reloaded.GetSettlementSubmitCount() == 0);
	TestTrue(TEXT("All recovery verification leaves Production bytes untouched"), Production.IsUnchanged());
	return true;
}

#endif
