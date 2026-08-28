#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfilePreparationFlow.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewFlowRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.4.14.r0"),
			TEXT("ProfilePreparationFlow"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	bool ReadFlowBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	bool ContainsItem(const Fdemo_mapProfileSessionSnapshot& Snapshot, const FGuid& ItemId)
	{
		return Snapshot.OrderedPermanentStash.ContainsByPredicate(
			[&ItemId](const Fdemo_mapPersistentItemRecord& Item)
			{
				return Item.ItemInstanceId == ItemId;
			});
	}

	FGuid FindDefinition(const Fdemo_mapProfileSessionSnapshot& Snapshot, FName DefinitionId)
	{
		for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
		{
			if (Item.ItemDefinitionId == DefinitionId)
			{
				return Item.ItemInstanceId;
			}
		}
		return FGuid();
	}

	struct FProductionSnapshot
	{
		TArray<FString> Paths;
		TArray<bool> Existed;
		TArray<TArray<uint8>> Bytes;

		FProductionSnapshot()
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
					FFileHelper::LoadFileToArray(Value, *Paths[Index]);
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

	struct FFlowFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Udemo_mapItemSubsystem* Runtime = nullptr;
		bool bStarted = false;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for Profile Preparation Flow fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Failed to allocate Profile Preparation Flow GameInstance."));
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
				Test.AddError(TEXT("Could not initialize the existing transient Preparation UI."));
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

		~FFlowFixture() { Stop(); }
	};

	bool InitializeFlow(FAutomationTestBase& Test, FFlowFixture& Fixture, Fdemo_mapProfilePreparationFlow& Flow, const FString& Root)
	{
		if (!Fixture.Start(Test)) return false;
		const Fdemo_mapProfileSessionInitializeResult Init = Flow.InitializeExplicit(Fixture.GameInstance, Root);
		if (!Init.IsReady())
		{
			Test.AddError(FString::Printf(TEXT("Flow initialization failed: %s"), *Init.Diagnostic));
			return false;
		}
		return true;
	}

	Fdemo_mapProfileSessionBeginResult StartRun(
		FAutomationTestBase& Test,
		FFlowFixture& Fixture,
		Fdemo_mapProfilePreparationFlow& Flow,
		const TArray<TPair<FName, FGuid>>& Equipment = {},
		const TArray<FGuid>& Materials = {})
	{
		Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(Test);
		if (!Widget) return Fdemo_mapProfileSessionBeginResult();
		for (const TPair<FName, FGuid>& Entry : Equipment)
		{
			const auto Selection = Widget->SelectEquipment(Entry.Key, Entry.Value);
			if (!Selection.IsAccepted()) Test.AddError(Selection.Diagnostic);
		}
		for (const FGuid& ItemId : Materials)
		{
			const auto Selection = Widget->SelectMaterial(ItemId, true);
			if (!Selection.IsAccepted()) Test.AddError(Selection.Diagnostic);
		}
		return Flow.StartPreparedRunThroughWidget(Widget);
	}

	Fdemo_mapProfileSessionSettlementResult Settle(
		FFlowFixture& Fixture,
		Fdemo_mapProfilePreparationFlow& Flow,
		Edemo_mapRunEndReason Reason,
		Fdemo_mapSettlementSummary* OutSummary = nullptr)
	{
		Fdemo_mapSettlementSummary Summary;
		const Fdemo_mapItemOperationResult RuntimeResult = Fixture.Runtime->RequestSettlement(Reason, Summary);
		if (!RuntimeResult.bSuccess) return Fdemo_mapProfileSessionSettlementResult();
		if (OutSummary) *OutSummary = Summary;
		return Flow.CommitRuntimeSettlement(Summary);
	}

	void AddMaterialRecords(const FString& Root)
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

	bool InitializeCutoverFlow(
		FAutomationTestBase& Test,
		FFlowFixture& Fixture,
		Fdemo_mapProfilePreparationFlow& Flow,
		const FString& Root,
		Udemo_mapShanmenItemAuthoritySubsystem*& OutAuthority,
		Fdemo_map0909BSectWarehouseService& OutWarehouse)
	{
		OutAuthority = nullptr;
		if (!InitializeFlow(Test, Fixture, Flow, Root))
		{
			return false;
		}
		OutAuthority = Fixture.GameInstance->GetSubsystem<
			Udemo_mapShanmenItemAuthoritySubsystem>();
		Fdemo_map0909BWarehousePresentation Presentation;
		FString Diagnostic;
		if (!OutAuthority
			|| !OutWarehouse.OpenForSect(
				Root, Fixture.Session->GetSnapshot(),
				Edemo_map0909BTopState::AtSect,
				Presentation, Diagnostic))
		{
			Test.AddError(FString::Printf(
				TEXT("P1.13 could not open the stable Code B migration source: %s"),
				*Diagnostic));
			return false;
		}
		const Fdemo_mapShanmenItemCutoverResult Cutover =
			Fdemo_mapShanmenItemCutoverCoordinator::Execute(
				Fdemo_mapProfileStorageContext::ForRoot(Root),
				Flow.GetProfileId(), *OutAuthority,
				*Fixture.Session, OutWarehouse);
		if (!Cutover.IsReady() || !Flow.UsesShanmenItemLifecycle())
		{
			Test.AddError(FString::Printf(
				TEXT("P1.13 authority cutover did not bind the product Flow: %s"),
				*Cutover.Diagnostic));
			return false;
		}
		return true;
	}

	int32 CountSuccessfulAuthorityOperation(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const EShanmenItemTransactionOperation Operation)
	{
		int32 Count = 0;
		for (const FShanmenItemProcessedRequestSnapshot& Processed :
			Snapshot.ProcessedRequests)
		{
			Count += Processed.Receipt.IsSuccess()
				&& Processed.Receipt.Operation == Operation ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow01, "demo_map.ProfilePreparationFlow.01.DefaultOffAndProductionZeroIO", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow01::RunTest(const FString&)
{
	FProductionSnapshot Production;
	Fdemo_mapProfilePreparationFlow Flow;
	FString Canonical, Diagnostic;
	TestTrue(TEXT("Adapter defaults disabled"), Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Disabled && !Flow.GetSession() && !Flow.GetRuntime());
	TestFalse(TEXT("Missing root rejected"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(TEXT(""), Canonical, Diagnostic));
	TestFalse(TEXT("Production root rejected"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(Fdemo_mapProfileStorageContext::Production().RootDirectory, Canonical, Diagnostic));
	TestTrue(TEXT("Production paths untouched"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow02, "demo_map.ProfilePreparationFlow.02.RootBoundaryMissingOutsideProductionAndReparsePolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow02::RunTest(const FString&)
{
	FString Canonical, Diagnostic;
	const FString Valid = NewFlowRoot();
	const FString Outside = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("OtherTask"));
	const FString Escaped = FPaths::Combine(Fdemo_mapProfilePreparationFlow::AllowedAutomationRoot(), TEXT(".."), TEXT("Escaped"));
	TestTrue(TEXT("Unique task root accepted"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(Valid, Canonical, Diagnostic) && Canonical.Contains(TEXT("Dev.D.UE.0.0.4.14.r0")));
	TestFalse(TEXT("Outside task root rejected"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(Outside, Canonical, Diagnostic));
	TestFalse(TEXT("Collapsed escape rejected"), Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(Escaped, Canonical, Diagnostic));
	TestFalse(TEXT("Existing reparse/symlink root kind is rejected"), Fdemo_mapProfilePreparationFlow::IsRootKindAcceptedForAutomation(true, true));
	TestTrue(TEXT("Missing fresh root and existing real directory kinds are accepted"), Fdemo_mapProfilePreparationFlow::IsRootKindAcceptedForAutomation(false, false) && Fdemo_mapProfilePreparationFlow::IsRootKindAcceptedForAutomation(true, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow03, "demo_map.ProfilePreparationFlow.03.FreshPreparationUIVisibleRuntimeOffInputGateContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow03::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this);
	TestTrue(TEXT("Preparation ready and Runtime inactive"), Widget && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation && Widget->GetViewState().bPreparationOperationsEnabled && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow04, "demo_map.ProfilePreparationFlow.04.EmptyLoadoutSinglePreparedRunNoLegacyBegin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow04::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	const auto Begin = StartRun(*this, Fixture, Flow);
	const FGuid RunId = Fixture.Runtime->GetActiveRunId();
	const auto LegacySecondBegin = Fixture.Runtime->BeginRun();
	TestTrue(TEXT("Exactly one prepared run is active"), Begin.IsRunActive() && RunId.IsValid() && RunId == Flow.GetStartedRunId() && Begin.Snapshot.ActiveRunId == RunId);
	TestFalse(TEXT("A legacy second BeginRun cannot create another run"), LegacySecondBegin.bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow05, "demo_map.ProfilePreparationFlow.05.MaximumSixCanonicalIdsUniqueRiskAndActivation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow05::RunTest(const FString&)
{
	const FString Root = NewFlowRoot(); AddMaterialRecords(Root);
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, Root)) return false;
	const auto Snapshot = Fixture.Session->GetSnapshot();
	TArray<TPair<FName, FGuid>> Equipment = {
		{ Fdemo_mapItemIds::WeaponSlot, FindDefinition(Snapshot, Fdemo_mapItemIds::TrainingBlade) },
		{ Fdemo_mapItemIds::ArmorSlot, FindDefinition(Snapshot, Fdemo_mapItemIds::TrainingVest) },
		{ Fdemo_mapItemIds::SpatialRingSlot, FindDefinition(Snapshot, Fdemo_mapItemIds::WindTalisman) } };
	TArray<FGuid> Materials; for (const auto& Item : Snapshot.OrderedPermanentStash) if (Item.ItemDefinitionId == Fdemo_mapItemIds::SpiritDust || Item.ItemDefinitionId == Fdemo_mapItemIds::IronShard) Materials.Add(Item.ItemInstanceId);
	const auto Begin = StartRun(*this, Fixture, Flow, Equipment, Materials);
	TSet<FGuid> Unique; for (const FGuid& ItemId : Begin.RuntimeResult.DeployedItemIds) Unique.Add(ItemId);
	TestTrue(TEXT("Six exact canonical identities materialized once"), Begin.IsRunActive() && Begin.RuntimeResult.DeployedItemIds.Num() == 6 && Unique.Num() == 6 && Fixture.Runtime->GetDeployedItemIds().Num() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow06, "demo_map.ProfilePreparationFlow.06.BeginPrecommitFailurePreservesSelectionAndRetry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow06::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	const FGuid Blade = FindDefinition(Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade);
	Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); Widget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, Blade);
	Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp);
	const auto Failed = Flow.StartPreparedRunThroughWidget(Widget);
	const auto Retry = Flow.StartPreparedRunThroughWidget(Widget);
	TestEqual(
		TEXT("Precommit failure remains a persistent commit rejection"),
		Failed.Status,
		Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected);
	TestEqual(
		TEXT("Retry returns the Flow to RunActive"),
		Flow.GetPhase(),
		Edemo_mapProfilePreparationFlowPhase::RunActive);
	TestTrue(
		TEXT("Precommit failure preserves the selected item for one successful retry"),
		Retry.IsRunActive());
	TestEqual(
		TEXT("Retry Session and Runtime share one ActiveRunId"),
		Retry.Snapshot.ActiveRunId,
		Fixture.Runtime->GetActiveRunId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow07, "demo_map.ProfilePreparationFlow.07.RuntimeMaterializationFailureEntersRecoveryWorldOff", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow07::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	const FGuid Blade = FindDefinition(Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade);
	Fixture.Runtime->SetPreparedRunFailureAfterMutationForAutomation(1);
	const auto Begin = StartRun(*this, Fixture, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }});
	TestTrue(TEXT("Materialization failure rolls Runtime idle and exposes recovery"), Begin.Status == Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::RecoveryRequired && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow08, "demo_map.ProfilePreparationFlow.08.WorldActivationFailureRollsBackExactlyOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow08::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	const auto Begin = StartRun(*this, Fixture, Flow); const auto End = Flow.CancelActiveRunForActivationFailure(); const auto Duplicate = Flow.CancelActiveRunForActivationFailure();
	TestTrue(TEXT("Activation failure rolls back through one technical terminal Summary"), Begin.IsRunActive() && End.IsDurablySettled() && End.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::ActivationFailure && Flow.GetSettlementSubmitCount() == 1 && Duplicate.Status == Edemo_mapProfileSessionSettlementStatus::SessionStateRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow09, "demo_map.ProfilePreparationFlow.09.ExtractionKeepsExactIdAndReturnsPreparation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow09::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	const FGuid Blade = FindDefinition(Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); const auto Begin = StartRun(*this, Fixture, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); const auto End = Settle(Fixture, Flow, Edemo_mapRunEndReason::Extraction);
	TestTrue(TEXT("Extraction keeps exact identity"), Begin.IsRunActive() && End.IsDurablySettled() && ContainsItem(End.Snapshot, Blade) && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow10, "demo_map.ProfilePreparationFlow.10.SecondProcessRedeploySameIdDeathRemovesEveryDomain", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow10::RunTest(const FString&)
{
	const FString Root = NewFlowRoot(); FGuid Blade, FirstRun, FirstSettlement;
	{
		FFlowFixture First; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, First, Flow, Root)) return false;
		Blade = FindDefinition(First.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); StartRun(*this, First, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); FirstRun = Flow.GetStartedRunId(); const auto End = Settle(First, Flow, Edemo_mapRunEndReason::Extraction); FirstSettlement = End.Snapshot.LastSettlementId;
	}
	FFlowFixture Second; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Second, Flow, Root)) return false;
	const auto Begin = StartRun(*this, Second, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); const FGuid SecondRun = Flow.GetStartedRunId(); const auto End = Settle(Second, Flow, Edemo_mapRunEndReason::Death);
	TestTrue(TEXT("New process preserves profile identity and removes same dead item"), Begin.IsRunActive() && Flow.GetPreviousRunId() == FirstRun && Flow.GetPreviousSettlementId() == FirstSettlement && SecondRun != FirstRun && SecondRun != FirstSettlement && End.IsDurablySettled() && !ContainsItem(End.Snapshot, Blade) && !Second.Runtime->GetAuthority().FindInstance(Blade));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow11, "demo_map.ProfilePreparationFlow.11.AbandonLosesRunItemButPreservesSafeStash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow11::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	const auto Before = Fixture.Session->GetSnapshot(); const FGuid Blade = FindDefinition(Before, Fdemo_mapItemIds::TrainingBlade); const FGuid Vest = FindDefinition(Before, Fdemo_mapItemIds::TrainingVest); StartRun(*this, Fixture, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); const auto End = Settle(Fixture, Flow, Edemo_mapRunEndReason::Abandon);
	TestTrue(TEXT("Only deployed item is lost on Abandon"), End.IsDurablySettled() && !ContainsItem(End.Snapshot, Blade) && ContainsItem(End.Snapshot, Vest));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow12, "demo_map.ProfilePreparationFlow.12.FirstEventWinsOneSubmitOneGeneration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow12::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	StartRun(*this, Fixture, Flow); Fdemo_mapSettlementSummary Summary; const auto End = Settle(Fixture, Flow, Edemo_mapRunEndReason::Extraction, &Summary); const int32 Generation = End.Snapshot.SaveGeneration; const auto Duplicate = Flow.CommitRuntimeSettlement(Summary);
	TestTrue(TEXT("First immutable summary wins"), End.IsDurablySettled() && Duplicate.Status == Edemo_mapProfileSessionSettlementStatus::SessionStateRejected && Flow.GetSettlementSubmitCount() == 1 && Fixture.Session->GetSnapshot().SaveGeneration == Generation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow13, "demo_map.ProfilePreparationFlow.13.PrecommitPendingRetryBlocksNewRunAndRetriesOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow13::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	StartRun(*this, Fixture, Flow); Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp); const auto Pending = Settle(Fixture, Flow, Edemo_mapRunEndReason::Extraction); Udemo_mapProfilePreparationWidget* Widget = Fixture.MakeWidget(*this); const auto Blocked = Flow.StartPreparedRunThroughWidget(Widget); const auto Retry = Flow.RetryPendingSettlement();
	TestTrue(TEXT("Pending evidence allows one explicit retry only"), Pending.Status == Edemo_mapProfileSessionSettlementStatus::PendingRetry && Blocked.Status == Edemo_mapProfileSessionBeginStatus::SessionNotReady && Retry.IsDurablySettled() && Flow.GetSettlementSubmitCount() == 1 && Flow.GetSettlementRetryCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow14, "demo_map.ProfilePreparationFlow.14.PostcommitAmbiguityReloadReconcilesOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow14::RunTest(const FString&)
{
	FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;
	StartRun(*this, Fixture, Flow); Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary); const auto End = Settle(Fixture, Flow, Edemo_mapRunEndReason::Extraction);
	TestTrue(TEXT("Ambiguous postcommit is reconciled through existing reload path"), End.Status == Edemo_mapProfileSessionSettlementStatus::ReconciledAfterReload && End.IsDurablySettled() && Flow.GetSettlementSubmitCount() == 1 && Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow15, "demo_map.ProfilePreparationFlow.15.RebuildUnbindNoDuplicateBindingsOrStaleControls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow15::RunTest(const FString&)
{
	const FString Root = NewFlowRoot(); FFlowFixture Fixture; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, Fixture, Flow, Root)) return false;
	Udemo_mapProfilePreparationWidget* First = Fixture.MakeWidget(*this); Udemo_mapProfilePreparationWidget* Second = Fixture.MakeWidget(*this); const FGuid ProfileId = Flow.GetProfileId(); Flow.Unbind(); const auto Rebound = Flow.InitializeExplicit(Fixture.GameInstance, Root);
	TestTrue(TEXT("Widget rebuild is value-identical and adapter unbind/rebind is clean"), First && Second && First != Second && First->GetViewState().ProfileId == Second->GetViewState().ProfileId && Rebound.IsReady() && Flow.GetProfileId() == ProfileId && Flow.GetSettlementSubmitCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow16, "demo_map.ProfilePreparationFlow.16.DeinitNewGIRecoversPreparedRunDefaultDisconnected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow16::RunTest(const FString&)
{
	FProductionSnapshot Production; const FString Root = NewFlowRoot(); FGuid Blade, RunId, ProfileId;
	{
		FFlowFixture First; Fdemo_mapProfilePreparationFlow Flow; if (!InitializeFlow(*this, First, Flow, Root)) return false;
		ProfileId = Flow.GetProfileId(); Blade = FindDefinition(First.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); StartRun(*this, First, Flow, {{ Fdemo_mapItemIds::WeaponSlot, Blade }}); RunId = Flow.GetStartedRunId(); Flow.Unbind();
	}
	FFlowFixture Second; Fdemo_mapProfilePreparationFlow Recovered; if (!Second.Start(*this)) return false; const auto Init = Recovered.InitializeExplicit(Second.GameInstance, Root);
	TestTrue(TEXT("New GI resolves orphan Prepared run once as recovered abandon"), Init.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && Init.Snapshot.ProfileId == ProfileId && Init.Snapshot.ActiveRunId == RunId && Init.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon && !ContainsItem(Init.Snapshot, Blade) && Recovered.GetPhase() == Edemo_mapProfilePreparationFlowPhase::Preparation);
	TestTrue(TEXT("Normal production storage remains disconnected"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfilePreparationFlow17, "demo_map.ProfilePreparationFlow.17.SameProcessSettlementClearsRuntimeAndSecondRunStarts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfilePreparationFlow17::RunTest(const FString&)
{
	FFlowFixture Fixture;
	Fdemo_mapProfilePreparationFlow Flow;
	if (!InitializeFlow(*this, Fixture, Flow, NewFlowRoot())) return false;

	const FGuid Blade = FindDefinition(
		Fixture.Session->GetSnapshot(),
		Fdemo_mapItemIds::TrainingBlade);
	const auto FirstBegin = StartRun(
		*this,
		Fixture,
		Flow,
		{{ Fdemo_mapItemIds::WeaponSlot, Blade }});
	const FGuid FirstRunId = Flow.GetStartedRunId();
	const auto FirstEnd = Settle(
		Fixture,
		Flow,
		Edemo_mapRunEndReason::Extraction);
	const bool bRuntimeClean =
		Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive
		&& !Fixture.Runtime->GetActiveRunId().IsValid()
		&& Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty()
		&& Fixture.Runtime->GetDeployedItemIds().IsEmpty();
	const bool bPersistentItemPreserved =
		ContainsItem(FirstEnd.Snapshot, Blade);

	const auto SecondBegin = StartRun(
		*this,
		Fixture,
		Flow,
		{{ Fdemo_mapItemIds::WeaponSlot, Blade }});
	const FGuid SecondRunId = Flow.GetStartedRunId();
	const auto SecondEnd = Settle(
		Fixture,
		Flow,
		Edemo_mapRunEndReason::Extraction);

	TestTrue(
		TEXT("Durable settlement clears only Runtime residue and preserves the Profile item"),
		FirstBegin.IsRunActive()
			&& FirstEnd.IsDurablySettled()
			&& bRuntimeClean
			&& bPersistentItemPreserved);
	TestTrue(
		TEXT("The same GameInstance can start and settle a second prepared run"),
		SecondBegin.IsRunActive()
			&& SecondRunId.IsValid()
			&& SecondRunId != FirstRunId
			&& SecondEnd.IsDurablySettled()
			&& ContainsItem(SecondEnd.Snapshot, Blade)
			&& Flow.GetPhase()
				== Edemo_mapProfilePreparationFlowPhase::Preparation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductFlowAtomicStartAndTerminalTest,
	"Shanmen.0_0_10.Items.ProductFlow.AtomicStartAndTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductFlowAtomicStartAndTerminalTest::RunTest(const FString&)
{
	const FString Root = NewFlowRoot();
	FFlowFixture Fixture;
	Fdemo_mapProfilePreparationFlow Flow;
	Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
	Fdemo_map0909BSectWarehouseService Warehouse;
	if (!InitializeCutoverFlow(
			*this, Fixture, Flow, Root, Authority, Warehouse))
	{
		return false;
	}

	const FGuid Blade = FindDefinition(
		Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("P1.13 selection writes only the cutover authority"),
		Blade.IsValid()
		&& Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Blade).IsAccepted());
	Fixture.Runtime->ResetForAutomation();
	TArray<uint8> ProfileBefore;
	FShanmenItemAuthorityDocument BeforeStart;
	TestTrue(TEXT("P1.13 captures retired Profile and authority baselines"),
		ReadFlowBytes(
			Fdemo_mapProfileStorageContext::ForRoot(Root).PrimaryPath(),
			ProfileBefore)
		&& Authority->TryGetDocument(BeforeStart));

	const Fdemo_mapProfileSessionBeginResult Begin =
		Flow.StartPreparedRunDirect();
	const FGuid ActiveRunId = Begin.Snapshot.ActiveRunId;
	const Fdemo_mapProfileSessionSnapshot RawProfile =
		Fixture.Session->GetSnapshot();
	FShanmenItemAuthorityDocument AfterStart;
	FShanmenItemAuthoritySnapshot ActiveAuthority;
	TestTrue(TEXT("Product Start publishes one atomic durable Run before Runtime"),
		Begin.IsRunActive()
		&& ActiveRunId.IsValid()
		&& ActiveRunId == Fixture.Runtime->GetActiveRunId()
		&& ActiveRunId == Flow.GetStartedRunId()
		&& Flow.GetPhase() == Edemo_mapProfilePreparationFlowPhase::RunActive
		&& RawProfile.SessionState
			== Edemo_mapProfileSessionState::ReadyForPreparation
		&& !RawProfile.ActiveRunId.IsValid()
		&& Authority->TryGetDocument(AfterStart)
		&& Authority->TryCaptureSnapshot(ActiveAuthority)
		&& AfterStart.SaveGeneration == BeforeStart.SaveGeneration + 1
		&& CountSuccessfulAuthorityOperation(
			ActiveAuthority,
			EShanmenItemTransactionOperation::StartPreparedRun) == 1
		&& CountSuccessfulAuthorityOperation(
			ActiveAuthority,
			EShanmenItemTransactionOperation::CommitBatch) == 0
		&& CountSuccessfulAuthorityOperation(
			ActiveAuthority,
			EShanmenItemTransactionOperation::ClaimPreparedRun) == 0);
	Fdemo_mapShanmenRunCorrelation StartCorrelation;
	Fdemo_mapShanmenRunCorrelation ReplayCorrelation;
	FShanmenItemAuthorityDocument AfterReadOnlyProbes;
	TestTrue(TEXT("P1.14 reconstructs one immutable authority Run correlation without a write"),
		Flow.TryGetActiveShanmenRunCorrelation(StartCorrelation)
		&& Flow.TryGetActiveShanmenRunCorrelation(ReplayCorrelation)
		&& StartCorrelation == ReplayCorrelation
		&& StartCorrelation.OwnerId == Begin.Snapshot.ProfileId
		&& StartCorrelation.ActiveRunId == ActiveRunId
		&& StartCorrelation.OrderedPreparedItemInstanceIds.Contains(Blade)
		&& StartCorrelation.OrderedPreparedItemInstanceIds.Num() == 1
		&& StartCorrelation.OrderedRunInventoryItemInstanceIds.IsEmpty()
		&& StartCorrelation.HotbarItemInstanceIds.Num()
			== Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
		&& Authority->TryGetDocument(AfterReadOnlyProbes)
		&& AfterReadOnlyProbes == AfterStart);

	Fdemo_mapSettlementSummary Summary;
	const Fdemo_mapItemOperationResult RuntimeSettlement =
		Fixture.Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Extraction, Summary);
	const Fdemo_mapProfileSessionSettlementResult End =
		RuntimeSettlement.bSuccess
			? Flow.CommitRuntimeSettlement(Summary)
			: Fdemo_mapProfileSessionSettlementResult();
	FShanmenItemAuthoritySnapshot TerminalAuthority;
	FGuid RecoverableRunId;
	Fdemo_mapShanmenRunCorrelation TerminalCorrelation;
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Terminal settlement finalizes only ShanmenItems and clears Runtime"),
		RuntimeSettlement.bSuccess
		&& End.Status == Edemo_mapProfileSessionSettlementStatus::Committed
		&& End.IsDurablySettled()
		&& Flow.GetPhase()
			== Edemo_mapProfilePreparationFlowPhase::Preparation
		&& Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive
		&& !Fixture.Runtime->GetActiveRunId().IsValid()
		&& Authority->TryCaptureSnapshot(TerminalAuthority)
		&& !Fdemo_mapShanmenRunLifecycleAdapter::
			TryFindRecoverableActiveRun(*Authority, RecoverableRunId)
		&& !Flow.TryGetActiveShanmenRunCorrelation(TerminalCorrelation)
		&& CountSuccessfulAuthorityOperation(
			TerminalAuthority,
			EShanmenItemTransactionOperation::FinalizePreparedRun) == 1
		&& ReadFlowBytes(
			Fdemo_mapProfileStorageContext::ForRoot(Root).PrimaryPath(),
			ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductFlowRuntimeRecoveryTest,
	"Shanmen.0_0_10.Items.ProductFlow.RuntimeFailureRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductFlowRuntimeRecoveryTest::RunTest(const FString&)
{
	const FString Root = NewFlowRoot();
	FFlowFixture Fixture;
	Fdemo_mapProfilePreparationFlow Flow;
	Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
	Fdemo_map0909BSectWarehouseService Warehouse;
	if (!InitializeCutoverFlow(
			*this, Fixture, Flow, Root, Authority, Warehouse))
	{
		return false;
	}

	const FGuid Blade = FindDefinition(
		Fixture.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade);
	if (!Blade.IsValid()
		|| !Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Blade).IsAccepted())
	{
		AddError(TEXT("P1.13 recovery fixture could not select the migrated blade."));
		return false;
	}
	Fixture.Runtime->ResetForAutomation();
	TArray<uint8> ProfileBefore;
	ReadFlowBytes(
		Fdemo_mapProfileStorageContext::ForRoot(Root).PrimaryPath(),
		ProfileBefore);
	Fixture.Runtime->SetPreparedRunFailureAfterMutationForAutomation(1);
	const Fdemo_mapProfileSessionBeginResult Failed =
		Flow.StartPreparedRunDirect();
	const FGuid ActiveRunId = Flow.GetRecoverableShanmenRunId();
	const Fdemo_mapProfilePreparationSnapshot RecoveryPresentation =
		Fixture.Session->GetPreparationSnapshot();
	FShanmenItemAuthorityDocument AfterFailure;
	FShanmenItemAuthoritySnapshot FailedAuthority;
	Fdemo_mapShanmenRunCorrelation FailureCorrelation;
	TestTrue(TEXT("Runtime mutation failure exposes one retryable durable identity"),
		Failed.Status
			== Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed
		&& ActiveRunId.IsValid()
		&& Flow.GetStartedRunId() == ActiveRunId
		&& Flow.GetPhase()
			== Edemo_mapProfilePreparationFlowPhase::Preparation
		&& Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive
		&& RecoveryPresentation.bCanStartRun
		&& RecoveryPresentation.VisibleDiagnostic.Contains(TEXT("可恢复"))
		&& Authority->TryGetDocument(AfterFailure)
		&& Authority->TryCaptureSnapshot(FailedAuthority)
		&& Flow.TryGetActiveShanmenRunCorrelation(FailureCorrelation)
		&& FailureCorrelation.ActiveRunId == ActiveRunId
		&& CountSuccessfulAuthorityOperation(
			FailedAuthority,
			EShanmenItemTransactionOperation::StartPreparedRun) == 1);

	const Fdemo_mapProfileSessionBeginResult Recovered =
		Flow.StartPreparedRunDirect();
	FShanmenItemAuthorityDocument AfterRecovery;
	Fdemo_mapShanmenRunCorrelation RecoveryCorrelation;
	TestTrue(TEXT("Retry rematerializes the same Run without a second durable write"),
		Recovered.IsRunActive()
		&& Recovered.Snapshot.ActiveRunId == ActiveRunId
		&& Fixture.Runtime->GetActiveRunId() == ActiveRunId
		&& Flow.TryGetActiveShanmenRunCorrelation(RecoveryCorrelation)
		&& RecoveryCorrelation == FailureCorrelation
		&& Authority->TryGetDocument(AfterRecovery)
		&& AfterRecovery == AfterFailure);

	const Fdemo_mapProfileSessionSettlementResult TechnicalRollback =
		Flow.CancelActiveRunForActivationFailure();
	FShanmenItemAuthorityDocument AfterTechnicalRollback;
	FGuid RollbackRecoveryId;
	Fdemo_mapShanmenRunCorrelation RollbackCorrelation;
	TestTrue(TEXT("World activation rollback is Runtime-only and never player Abandon"),
		TechnicalRollback.Status
			== Edemo_mapProfileSessionSettlementStatus::RuntimeRollbackReady
		&& !TechnicalRollback.IsDurablySettled()
		&& TechnicalRollback.Snapshot.LastTerminalReason
			== Edemo_mapRunEndReason::ActivationFailure
		&& Flow.GetPhase()
			== Edemo_mapProfilePreparationFlowPhase::Preparation
		&& Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive
		&& Authority->TryGetDocument(AfterTechnicalRollback)
		&& AfterTechnicalRollback == AfterFailure
		&& Fdemo_mapShanmenRunLifecycleAdapter::TryFindRecoverableActiveRun(
			*Authority, RollbackRecoveryId)
		&& RollbackRecoveryId == ActiveRunId
		&& Flow.TryGetActiveShanmenRunCorrelation(RollbackCorrelation)
		&& RollbackCorrelation == FailureCorrelation
		&& CountSuccessfulAuthorityOperation(
			AfterTechnicalRollback.Authority,
			EShanmenItemTransactionOperation::FinalizePreparedRun) == 0);

	const Fdemo_mapProfileSessionBeginResult RecoveredAgain =
		Flow.StartPreparedRunDirect();
	Fdemo_mapShanmenRunCorrelation SecondRecoveryCorrelation;
	const bool bHasSecondRecoveryCorrelation =
		Flow.TryGetActiveShanmenRunCorrelation(SecondRecoveryCorrelation);
	Fdemo_mapSettlementSummary Summary;
	const Fdemo_mapItemOperationResult RuntimeSettlement =
		RecoveredAgain.IsRunActive()
			? Fixture.Runtime->RequestSettlement(
				Edemo_mapRunEndReason::Extraction, Summary)
			: Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::RunNotActive,
				TEXT("P1.13 exact-identity retry did not materialize Runtime."));
	const Fdemo_mapProfileSessionSettlementResult End =
		RuntimeSettlement.bSuccess
			? Flow.CommitRuntimeSettlement(Summary)
			: Fdemo_mapProfileSessionSettlementResult();
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Recovered identity remains terminally settleable without Profile writes"),
		RecoveredAgain.IsRunActive()
		&& RecoveredAgain.Snapshot.ActiveRunId == ActiveRunId
		&& bHasSecondRecoveryCorrelation
		&& SecondRecoveryCorrelation == FailureCorrelation
		&& RuntimeSettlement.bSuccess
		&& End.IsDurablySettled()
		&& ReadFlowBytes(
			Fdemo_mapProfileStorageContext::ForRoot(Root).PrimaryPath(),
			ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	Fdemo_mapShanmenRunCorrelation TerminalCorrelation;
	TestFalse(TEXT("Finalized Run has no active authority correlation"),
		Flow.TryGetActiveShanmenRunCorrelation(TerminalCorrelation));
	return true;
}

#endif
