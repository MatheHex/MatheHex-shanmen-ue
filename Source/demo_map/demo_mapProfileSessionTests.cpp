#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionCoordinator.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString NewSessionRoot()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("Dev.D.UE.0.0.4.8.r0"), TEXT("ProfileSession"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
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
				const bool bFile = Index > 0 && IFileManager::Get().FileExists(*Paths[Index]);
				const bool bDirectory = Index == 0 && IFileManager::Get().DirectoryExists(*Paths[Index]);
				Existed.Add(bFile || bDirectory);
				TArray<uint8> Value;
				if (bFile) ReadBytes(Paths[Index], Value);
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
				if (Index > 0 && Existed[Index])
				{
					TArray<uint8> After;
					if (!ReadBytes(Paths[Index], After) || After != Bytes[Index]) return false;
				}
			}
			return true;
		}
	};

	Udemo_mapItemSubsystem* NewRuntime()
	{
		return NewObject<Udemo_mapItemSubsystem>(NewObject<UGameInstance>(GetTransientPackage()));
	}

	FGuid AddStashRecord(Fdemo_mapPersistentProfile& Profile, FName DefinitionId, int32 StackCount)
	{
		Fdemo_mapPersistentItemRecord Item;
		do Item.ItemInstanceId = FGuid::NewGuid();
		while (Profile.PermanentStash.ContainsByPredicate([&Item](const Fdemo_mapPersistentItemRecord& Existing){ return Existing.ItemInstanceId == Item.ItemInstanceId; }));
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = StackCount;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item.ItemInstanceId;
	}

	Fdemo_mapBeginRunRequest BeginRequest(const Fdemo_mapProfileSessionSnapshot& Snapshot)
	{
		Fdemo_mapBeginRunRequest Request;
		Request.ExpectedProfileId = Snapshot.ProfileId;
		Request.ExpectedSaveGeneration = Snapshot.SaveGeneration;
		return Request;
	}

	Fdemo_mapPersistentProfile CreatePrepared(
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage,
		bool bSelectWeapon = true)
	{
		Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
		Fdemo_mapBeginRunRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		if (bSelectWeapon) Request.Loadout.WeaponItemInstanceId = Profile.PermanentStash[0].ItemInstanceId;
		Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
		return Profile;
	}

	Fdemo_mapProfileSettlementRequest PersistentSettlementRequest(
		const Fdemo_mapPersistentProfile& Profile,
		Edemo_mapRunEndReason Reason)
	{
		Fdemo_mapProfileSettlementRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		Request.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
		Request.RequestedEndReason = Reason;
		return Request;
	}

	bool ContainsId(const TArray<Fdemo_mapPersistentItemRecord>& Records, const FGuid& Id)
	{
		return Records.ContainsByPredicate([&Id](const Fdemo_mapPersistentItemRecord& Item){ return Item.ItemInstanceId == Id; });
	}

	bool IsTerminalSnapshot(const Fdemo_mapProfileSessionSnapshot& Snapshot, Edemo_mapRunEndReason Reason)
	{
		return Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
			&& Snapshot.LastSettlementId.IsValid()
			&& Snapshot.LastTerminalReason == Reason;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession01, "demo_map.ProfileSession.01.FreshInitializeReady", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession01::RunTest(const FString&)
{
	FProductionSnapshot Production;
	Fdemo_mapProfileSessionCoordinator Session;
	const auto Result = Session.InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()));
	TestTrue(TEXT("Fresh initializes Ready"), Result.IsReady() && Result.Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation);
	TestTrue(TEXT("Fresh Profile identity/generation/order"), Result.Snapshot.ProfileId.IsValid() && Result.Snapshot.SaveGeneration == 1 && Result.Snapshot.OrderedPermanentStash.Num() == 3
		&& Result.Snapshot.OrderedPermanentStash[0].ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade
		&& Result.Snapshot.OrderedPermanentStash[1].ItemDefinitionId == Fdemo_mapItemIds::TrainingVest
		&& Result.Snapshot.OrderedPermanentStash[2].ItemDefinitionId == Fdemo_mapItemIds::WindTalisman);
	TestTrue(TEXT("Snapshot capabilities"), Result.Snapshot.bCanBeginRun && !Result.Snapshot.bCanRetrySettlement);
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession02, "demo_map.ProfileSession.02.IdleAndTerminalLoadReady", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession02::RunTest(const FString&)
{
	const auto IdleStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapPersistentProfile Idle = Repository.LoadOrCreateDefaultProfile(IdleStorage).Profile;
	Fdemo_mapProfileSessionCoordinator IdleSession;
	TestTrue(TEXT("Existing idle loads Ready"), IdleSession.InitializeSession(IdleStorage).IsReady() && IdleSession.GetSnapshot().ProfileId == Idle.ProfileId);

	const auto TerminalStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	Fdemo_mapPersistentProfile Terminal = CreatePrepared(Repository, TerminalStorage);
	const auto End = Fdemo_mapProfileSettlementTransaction().Execute(Terminal, PersistentSettlementRequest(Terminal, Edemo_mapRunEndReason::Death), Repository, TerminalStorage);
	const int32 TerminalGeneration = Terminal.SaveGeneration;
	const int32 TerminalShopGeneration = Terminal.ShopStock.Generation;
	Fdemo_mapProfileSessionCoordinator TerminalSession;
	const auto Loaded = TerminalSession.InitializeSession(TerminalStorage);
	TestTrue(TEXT("Terminal tombstone loads Ready after exactly one pending restock"), End.IsCommitted() && Loaded.IsReady()
		&& IsTerminalSnapshot(Loaded.Snapshot, Edemo_mapRunEndReason::Death)
		&& Loaded.Snapshot.SaveGeneration == TerminalGeneration + 1
		&& Loaded.Snapshot.ShopStock.Generation == TerminalShopGeneration + 1
		&& Loaded.Snapshot.ShopStock.LastAppliedTerminalId == Loaded.Snapshot.LastSettlementId);
	Fdemo_mapProfileSessionCoordinator Reload;
	const auto Reloaded = Reload.InitializeSession(TerminalStorage);
	TestTrue(TEXT("Applied terminal restock is reload-idempotent"), Reloaded.IsReady()
		&& Reloaded.Snapshot.SaveGeneration == Loaded.Snapshot.SaveGeneration
		&& Reloaded.Snapshot.ShopStock.Generation == Loaded.Snapshot.ShopStock.Generation
		&& Reloaded.Snapshot.ShopStock.ShopStockEventId == Loaded.Snapshot.ShopStock.ShopStockEventId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession03, "demo_map.ProfileSession.03.PreparedInitializeRecoveredAbandonOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession03::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	const Fdemo_mapPersistentProfile Prepared = CreatePrepared(Repository, Storage);
	Fdemo_mapProfileSessionCoordinator Session;
	const auto Result = Session.InitializeSession(Storage);
	TestTrue(TEXT("Prepared recovered once"), Result.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && IsTerminalSnapshot(Result.Snapshot, Edemo_mapRunEndReason::RecoveredAbandon));
	TestTrue(TEXT("Recovery and one pending restock each commit once"),
		Result.Snapshot.SaveGeneration == Prepared.SaveGeneration + 2
		&& Result.Snapshot.ShopStock.Generation == Prepared.ShopStock.Generation + 1
		&& Result.Snapshot.ShopStock.LastAppliedTerminalId == Result.Snapshot.LastSettlementId);
	Fdemo_mapProfileSessionCoordinator Reload;
	const auto Reloaded = Reload.InitializeSession(Storage);
	TestTrue(TEXT("Recovered-abandon restock is reload-idempotent"), Reloaded.IsReady()
		&& Reloaded.Snapshot.SaveGeneration == Result.Snapshot.SaveGeneration
		&& Reloaded.Snapshot.ShopStock.Generation == Result.Snapshot.ShopStock.Generation
		&& Reloaded.Snapshot.ShopStock.ShopStockEventId == Result.Snapshot.ShopStock.ShopStockEventId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession04, "demo_map.ProfileSession.04.RepeatedRecoveryKeepsSettlementId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession04::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	CreatePrepared(Repository, Storage);
	FGuid SettlementId; int32 Generation = 0;
	{
		Fdemo_mapProfileSessionCoordinator First;
		const auto Initial = First.InitializeSession(Storage);
		SettlementId = Initial.Snapshot.LastSettlementId;
		Generation = Initial.Snapshot.SaveGeneration;
	}
	Fdemo_mapProfileSessionCoordinator Again;
	const auto Reload = Again.InitializeSession(Storage);
	TestTrue(TEXT("Repeated initialization is idempotent"), Reload.IsReady() && Reload.Snapshot.LastSettlementId == SettlementId && Reload.Snapshot.SaveGeneration == Generation && Reload.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession05, "demo_map.ProfileSession.05.FutureSchemaReadOnlyFatal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession05::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	Repository.LoadOrCreateDefaultProfile(Storage);
	FString Json; FFileHelper::LoadFileToString(Json, *Storage.PrimaryPath());
	const FString CurrentSchema = FString::Printf(
		TEXT("\"SchemaVersion\":%d"),
		Fdemo_mapPersistentProfile::CurrentSchemaVersion);
	const FString FutureSchema = FString::Printf(
		TEXT("\"SchemaVersion\":%d"),
		Fdemo_mapPersistentProfile::CurrentSchemaVersion + 1);
	TestEqual(TEXT("Fixture schema replacement"), Json.ReplaceInline(*CurrentSchema, *FutureSchema, ESearchCase::CaseSensitive), 1);
	FFileHelper::SaveStringToFile(Json, *Storage.PrimaryPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TArray<uint8> Before; ReadBytes(Storage.PrimaryPath(), Before);
	Fdemo_mapProfileSessionCoordinator Session;
	const auto Result = Session.InitializeSession(Storage);
	TArray<uint8> After; ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Future schema blocks without overwrite"), Result.Status == Edemo_mapProfileSessionInitializeStatus::FatalProfileError && Result.Snapshot.SessionState == Edemo_mapProfileSessionState::FatalProfileError && Before == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession06, "demo_map.ProfileSession.06.CorruptWithoutBackupDoesNotReset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession06::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	IFileManager::Get().MakeDirectory(*Storage.RootDirectory, true);
	const FString Corrupt = TEXT("{not-valid-json");
	FFileHelper::SaveStringToFile(Corrupt, *Storage.PrimaryPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TArray<uint8> Before; ReadBytes(Storage.PrimaryPath(), Before);
	Fdemo_mapProfileSessionCoordinator Session;
	const auto Result = Session.InitializeSession(Storage);
	TArray<uint8> After; ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Corrupt primary is fatal and preserved"), Result.Status == Edemo_mapProfileSessionInitializeStatus::FatalProfileError && Before == After && !IFileManager::Get().FileExists(*Storage.BackupPath()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession07, "demo_map.ProfileSession.07.EmptyLoadoutUsesSameRunId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession07::RunTest(const FString&)
{
	Fdemo_mapProfileSessionCoordinator Session;
	const auto Init = Session.InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()));
	Udemo_mapItemSubsystem* Runtime = NewRuntime();
	const auto Begin = Session.BeginRun(BeginRequest(Init.Snapshot), *Runtime);
	TestTrue(TEXT("Empty loadout commits and materializes"), Begin.IsRunActive() && Begin.PersistentResult.ActiveRunId.IsValid() && Begin.PersistentResult.ActiveRunId == Runtime->GetActiveRunId() && Begin.Snapshot.ActiveRunId == Runtime->GetActiveRunId());
	TestTrue(TEXT("Empty runtime authority"), Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty() && Runtime->GetDeployedItemIds().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession08, "demo_map.ProfileSession.08.MaximumSixPreservesIdentityAndOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession08::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5); const FGuid Iron = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 5); const FGuid DustB = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2);
	TestTrue(TEXT("Fixture save"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot);
	Request.Loadout.WeaponItemInstanceId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId; Request.Loadout.ArmorItemInstanceId = Init.Snapshot.OrderedPermanentStash[1].ItemInstanceId; Request.Loadout.SpatialRingItemInstanceId = Init.Snapshot.OrderedPermanentStash[2].ItemInstanceId; Request.Loadout.MaterialStackItemInstanceIds = { DustB, Iron, DustA };
	Udemo_mapItemSubsystem* Runtime = NewRuntime(); const auto Begin = Session.BeginRun(Request, *Runtime);
	const TArray<FGuid> Expected = { Request.Loadout.WeaponItemInstanceId, Request.Loadout.ArmorItemInstanceId, Request.Loadout.SpatialRingItemInstanceId, DustA, Iron, DustB };
	TestTrue(TEXT("Maximum six exact order"), Begin.IsRunActive() && Begin.PersistentResult.CommittedLoadoutPlan->DeployedItemIds == Expected && Begin.RuntimeResult.DeployedItemIds == Expected);
	for (const FGuid& Id : Expected) TestTrue(TEXT("Original identity exists once in Runtime"), Runtime->GetAuthority().FindInstance(Id) != nullptr && Runtime->GetDeployedItemIds().Contains(Id));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession09, "demo_map.ProfileSession.09.StaleIntentRejectedWithoutMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession09::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); TArray<uint8> Before; ReadBytes(Storage.PrimaryPath(), Before);
	Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); --Request.ExpectedSaveGeneration; Udemo_mapItemSubsystem* Runtime = NewRuntime(); const auto Begin = Session.BeginRun(Request, *Runtime); TArray<uint8> After; ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Stale intent rejected"), Begin.Status == Edemo_mapProfileSessionBeginStatus::StaleIntent && Before == After && Runtime->GetRunState() == Edemo_mapRunState::Inactive && Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty() && Session.GetSnapshot().bCanBeginRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession10, "demo_map.ProfileSession.10.BeginRepositoryFailureDoesNotTouchRuntime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession10::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); TArray<uint8> Before; ReadBytes(Storage.PrimaryPath(), Before);
	Session.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp); Udemo_mapItemSubsystem* Runtime = NewRuntime(); const auto Begin = Session.BeginRun(BeginRequest(Init.Snapshot), *Runtime); TArray<uint8> After; ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Precommit failure is atomic"), Begin.Status == Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected && Before == After && Runtime->GetRunState() == Edemo_mapRunState::Inactive && Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty() && Session.GetSnapshot().bCanBeginRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession11, "demo_map.ProfileSession.11.RuntimeFaultRollsBackAndRequiresRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession11::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId;
	Udemo_mapItemSubsystem* Runtime = NewRuntime(); Runtime->SetPreparedRunFailureAfterMutationForAutomation(1); const auto Begin = Session.BeginRun(Request, *Runtime); const auto Disk = Fdemo_mapProfileRepository().LoadExistingProfile(Storage);
	TestTrue(TEXT("Runtime rollback and recovery state"), Begin.Status == Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed && Begin.Snapshot.SessionState == Edemo_mapProfileSessionState::RecoveryRequired && Runtime->GetRunState() == Edemo_mapRunState::Inactive && Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty() && Disk.IsSuccess() && Disk.Profile.ActiveRun.bHasActiveRun && Disk.Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::Prepared);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession12, "demo_map.ProfileSession.12.NewSessionRecoversMaterializationFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession12::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); FGuid FailedRun;
	{
		Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId; Udemo_mapItemSubsystem* Runtime = NewRuntime(); Runtime->SetPreparedRunFailureAfterMutationForAutomation(1); const auto Begin = Session.BeginRun(Request, *Runtime); FailedRun = Begin.Snapshot.ActiveRunId; TestTrue(TEXT("Fixture recovery required"), Begin.Status == Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed);
	}
	Fdemo_mapProfileSessionCoordinator Recovery; const auto Result = Recovery.InitializeSession(Storage);
	TestTrue(TEXT("New session recovers Prepared once"), Result.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && Result.Snapshot.ActiveRunId == FailedRun && IsTerminalSnapshot(Result.Snapshot, Edemo_mapRunEndReason::RecoveredAbandon));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession13, "demo_map.ProfileSession.13.ExtractionSurvivesFullFixtureRecreation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession13::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); FGuid WeaponId; FGuid RunId; FGuid SettlementId; int32 Generation = 0;
	{
		Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); WeaponId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId; Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = WeaponId; Udemo_mapItemSubsystem* Runtime = NewRuntime(); const auto Begin = Session.BeginRun(Request, *Runtime); RunId = Begin.Snapshot.ActiveRunId; Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("Runtime extraction"), Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess); const auto End = Session.CommitRuntimeSettlement(Summary); TestTrue(TEXT("Persistent extraction retains the weapon identity without reviving a legacy preparation layout"), End.IsDurablySettled() && End.Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation && !End.Snapshot.PreparationLayout.WeaponItemInstanceId.IsValid()); SettlementId = End.Snapshot.LastSettlementId; Generation = End.Snapshot.SaveGeneration;
	}
	Fdemo_mapProfileSessionCoordinator Reloaded; const auto Load = Reloaded.InitializeSession(Storage);
	TestTrue(TEXT("Independent fixture reloads exact extraction without a legacy preparation gate"), Load.IsReady() && Load.Snapshot.ActiveRunId == RunId && Load.Snapshot.LastSettlementId == SettlementId && Load.Snapshot.SaveGeneration == Generation && ContainsId(Load.Snapshot.OrderedPermanentStash, WeaponId) && !Load.Snapshot.PreparationLayout.WeaponItemInstanceId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession14, "demo_map.ProfileSession.14.RedeployedExtractedIdDiesPermanently", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession14::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); FGuid WeaponId;
	{
		Fdemo_mapProfileSessionCoordinator First; const auto Init = First.InitializeSession(Storage); WeaponId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId; Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = WeaponId; Udemo_mapItemSubsystem* Runtime = NewRuntime(); First.BeginRun(Request, *Runtime); Fdemo_mapSettlementSummary Summary; Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary); TestTrue(TEXT("First extraction committed"), First.CommitRuntimeSettlement(Summary).IsDurablySettled());
	}
	{
		Fdemo_mapProfileSessionCoordinator Second; const auto Init = Second.InitializeSession(Storage); TestTrue(TEXT("Same ID available"), ContainsId(Init.Snapshot.OrderedPermanentStash, WeaponId)); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = WeaponId; Udemo_mapItemSubsystem* Runtime = NewRuntime(); TestTrue(TEXT("Same ID redeployed"), Second.BeginRun(Request, *Runtime).IsRunActive()); Fdemo_mapSettlementSummary Summary; Runtime->RequestSettlement(Edemo_mapRunEndReason::Death, Summary); TestTrue(TEXT("Death committed"), Second.CommitRuntimeSettlement(Summary).IsDurablySettled());
	}
	Fdemo_mapProfileSessionCoordinator Third; const auto Reload = Third.InitializeSession(Storage); TestFalse(TEXT("Exact ID permanently absent"), ContainsId(Reload.Snapshot.OrderedPermanentStash, WeaponId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession15, "demo_map.ProfileSession.15.AbandonAndWorldLeftRemainLost", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession15::RunTest(const FString&)
{
	const auto AbandonStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); FGuid AbandonedId;
	{
		Fdemo_mapProfileRepository Repository; Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(AbandonStorage).Profile; AbandonedId = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 5); Repository.SaveProfile(Profile, AbandonStorage); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(AbandonStorage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.MaterialStackItemInstanceIds = { AbandonedId }; Udemo_mapItemSubsystem* Runtime = NewRuntime(); Session.BeginRun(Request, *Runtime); Fdemo_mapSettlementSummary Summary; Runtime->RequestSettlement(Edemo_mapRunEndReason::Abandon, Summary); TestTrue(TEXT("Abandon committed"), Session.CommitRuntimeSettlement(Summary).IsDurablySettled());
	}
	Fdemo_mapProfileSessionCoordinator AbandonReload; TestFalse(TEXT("Abandoned ID lost"), ContainsId(AbandonReload.InitializeSession(AbandonStorage).Snapshot.OrderedPermanentStash, AbandonedId));

	const auto WorldStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); FGuid WorldLeftId;
	{
		Fdemo_mapProfileRepository Repository; Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(WorldStorage).Profile; WorldLeftId = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5); Repository.SaveProfile(Profile, WorldStorage); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(WorldStorage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.MaterialStackItemInstanceIds = { WorldLeftId }; Udemo_mapItemSubsystem* Runtime = NewRuntime(); Session.BeginRun(Request, *Runtime); TestTrue(TEXT("Fixture moves deployed item to World"), const_cast<Fdemo_mapItemAuthority&>(Runtime->GetAuthority()).MoveInventoryToWorld(WorldLeftId).bSuccess); Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("Runtime extraction settles world-left as loss"), Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess && Summary.RuntimeSnapshot.OrderedSecuredItems.IsEmpty()); TestTrue(TEXT("Empty extraction persisted"), Session.CommitRuntimeSettlement(Summary).IsDurablySettled());
	}
	Fdemo_mapProfileSessionCoordinator WorldReload; TestFalse(TEXT("World-left ID lost"), ContainsId(WorldReload.InitializeSession(WorldStorage).Snapshot.OrderedPermanentStash, WorldLeftId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession16, "demo_map.ProfileSession.16.PendingSnapshotRetriesExactlyOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession16::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); Udemo_mapItemSubsystem* Runtime = NewRuntime(); Session.BeginRun(BeginRequest(Init.Snapshot), *Runtime); Fdemo_mapSettlementSummary Summary; Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary); const int32 PreparedGeneration = Session.GetSnapshot().SaveGeneration;
	const int32 PreparedShopGeneration = Session.GetSnapshot().ShopStock.Generation;
	Session.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp); const auto Failed = Session.CommitRuntimeSettlement(Summary); TestTrue(TEXT("Failure retains immutable evidence"), Failed.Status == Edemo_mapProfileSessionSettlementStatus::PendingRetry && Failed.Snapshot.bCanRetrySettlement && Failed.Snapshot.SaveGeneration == PreparedGeneration);
	Fdemo_mapSettlementSummary Changed = Summary; Changed.Reason = Edemo_mapRunEndReason::Death; Changed.RuntimeSnapshot.CommittedEndReason = Edemo_mapRunEndReason::Death; TestTrue(TEXT("Changed terminal evidence rejected"), Session.CommitRuntimeSettlement(Changed).Status == Edemo_mapProfileSessionSettlementStatus::SessionStateRejected);
	const auto Retry = Session.RetryPendingSettlement(); TestTrue(TEXT("Explicit retry commits settlement and one terminal restock"), Retry.Status == Edemo_mapProfileSessionSettlementStatus::Committed
		&& Retry.Snapshot.SaveGeneration == PreparedGeneration + 2
		&& Retry.Snapshot.ShopStock.Generation == PreparedShopGeneration + 1
		&& Retry.Snapshot.ShopStock.LastAppliedTerminalId == Retry.Snapshot.LastSettlementId
		&& !Retry.Snapshot.bCanRetrySettlement);
	const auto Again = Session.RetryPendingSettlement();
	TestTrue(TEXT("Second retry has no evidence or additional commit"), Again.Status == Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement
		&& Again.Snapshot.SaveGeneration == Retry.Snapshot.SaveGeneration
		&& Again.Snapshot.ShopStock.ShopStockEventId == Retry.Snapshot.ShopStock.ShopStockEventId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession17, "demo_map.ProfileSession.17.PostCommitReloadAndFirstEventWins", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession17::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSessionRoot()); Fdemo_mapProfileSessionCoordinator Session; const auto Init = Session.InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId; Udemo_mapItemSubsystem* Runtime = NewRuntime(); Session.BeginRun(Request, *Runtime); Fdemo_mapSettlementSummary First, Competing; Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, First);
	Session.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary); const auto End = Session.CommitRuntimeSettlement(First); const auto SecondRuntime = Runtime->RequestSettlement(Edemo_mapRunEndReason::Death, Competing);
	TestTrue(TEXT("Postcommit ambiguity reconciled"), End.Status == Edemo_mapProfileSessionSettlementStatus::ReconciledAfterReload && IsTerminalSnapshot(End.Snapshot, Edemo_mapRunEndReason::Extraction));
	TestTrue(TEXT("Runtime first event remains authoritative"), !SecondRuntime.bSuccess && SecondRuntime.Code == Edemo_mapItemResultCode::SettlementAlreadyCompleted && Competing.RuntimeSnapshot == First.RuntimeSnapshot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSession18, "demo_map.ProfileSession.18.DefaultLazyAndProductionProtected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSession18::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileSessionCoordinator Session; const auto Snapshot = Session.GetSnapshot();
	TestTrue(TEXT("Construction is fully lazy"), Snapshot.SessionState == Edemo_mapProfileSessionState::Uninitialized && !Snapshot.ProfileId.IsValid() && Snapshot.OrderedPermanentStash.IsEmpty() && !Snapshot.bCanBeginRun && !Snapshot.bCanRetrySettlement);
	TArray<FString> Sources; IFileManager::Get().FindFilesRecursive(Sources, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("demo_map")), TEXT("*.cpp"), true, false); bool bUnexpected = false;
	for (const FString& Path : Sources)
	{
		FString Text; if (!FFileHelper::LoadFileToString(Text, *Path) || !Text.Contains(TEXT("Fdemo_mapProfileSessionCoordinator"))) continue;
		if (!Path.EndsWith(TEXT("demo_mapProfileSessionCoordinator.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSessionSubsystem.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSessionTests.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSessionSubsystemTests.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileTradeTests.cpp"))) { bUnexpected = true; AddError(FString::Printf(TEXT("Unexpected normal-startup ProfileSession reference: %s"), *Path)); }
	}
	TestFalse(TEXT("No normal startup, Manager, GameMode, UI, map, or Package hook"), bUnexpected);
	TestTrue(TEXT("Production Save remains unchanged"), Production.IsUnchanged());
	return true;
}

#endif
