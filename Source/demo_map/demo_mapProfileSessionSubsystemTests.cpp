#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewSubsystemRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.4.9.r0"),
			TEXT("ProfileSessionSubsystem"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
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
				const bool bExists = Index == 0
					? IFileManager::Get().DirectoryExists(*Paths[Index])
					: IFileManager::Get().FileExists(*Paths[Index]);
				Existed.Add(bExists);
				TArray<uint8> Value;
				if (Index > 0 && bExists)
				{
					ReadBytes(Paths[Index], Value);
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
					if (!ReadBytes(Paths[Index], After) || After != Bytes[Index])
					{
						return false;
					}
				}
			}
			return true;
		}
	};

	struct FSubsystemFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Udemo_mapItemSubsystem* Runtime = nullptr;
		bool bStarted = false;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for GameInstance Subsystem fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Failed to allocate GameInstance fixture."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			bStarted = true;
			Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
			Runtime = GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
			if (!Session || !Runtime)
			{
				Test.AddError(TEXT("Normal GameInstance Subsystem acquisition did not return both authorities."));
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

		~FSubsystemFixture()
		{
			Stop();
		}
	};

	Fdemo_mapBeginRunRequest BeginRequest(const Fdemo_mapProfileSessionSnapshot& Snapshot)
	{
		Fdemo_mapBeginRunRequest Request;
		Request.ExpectedProfileId = Snapshot.ProfileId;
		Request.ExpectedSaveGeneration = Snapshot.SaveGeneration;
		return Request;
	}

	FGuid AddStashRecord(Fdemo_mapPersistentProfile& Profile, FName DefinitionId, int32 StackCount)
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

	bool ContainsId(const TArray<Fdemo_mapPersistentItemRecord>& Records, const FGuid& Id)
	{
		return Records.ContainsByPredicate(
			[&Id](const Fdemo_mapPersistentItemRecord& Item)
			{
				return Item.ItemInstanceId == Id;
			});
	}

	bool RequestAndCommit(
		FAutomationTestBase& Test,
		FSubsystemFixture& Fixture,
		Edemo_mapRunEndReason Reason,
		Fdemo_mapProfileSessionSettlementResult& OutResult)
	{
		Fdemo_mapSettlementSummary Summary;
		const Fdemo_mapItemOperationResult RuntimeResult = Fixture.Runtime->RequestSettlement(Reason, Summary);
		Test.TestTrue(TEXT("Runtime terminal event accepted"), RuntimeResult.bSuccess);
		OutResult = Fixture.Session->CommitRuntimeSettlement(Summary);
		Test.TestTrue(TEXT("Persistent terminal event committed"), OutResult.IsDurablySettled());
		return RuntimeResult.bSuccess && OutResult.IsDurablySettled();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem01, "demo_map.ProfileSessionSubsystem.01.UniqueAndLazyLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem01::RunTest(const FString&)
{
	FProductionSnapshot Production;
	const FString UnusedRoot = NewSubsystemRoot();
	FSubsystemFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	Udemo_mapProfileSessionSubsystem* Again = Fixture.GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
	const auto All = Fixture.GameInstance->GetSubsystemArrayCopy<Udemo_mapProfileSessionSubsystem>();
	const auto Snapshot = Fixture.Session->GetSnapshot();
	TestTrue(TEXT("One subsystem per GameInstance"), Again == Fixture.Session && All.Num() == 1 && All[0] == Fixture.Session);
	TestTrue(TEXT("Native lifecycle remains lazy"), !Fixture.Session->IsExplicitlyInitialized() && Snapshot.SessionState == Edemo_mapProfileSessionState::Uninitialized && !Snapshot.ProfileId.IsValid());
	TestFalse(TEXT("Unused isolation root was not created"), IFileManager::Get().DirectoryExists(*UnusedRoot));
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem02, "demo_map.ProfileSessionSubsystem.02.UninitializedCallsRejectSafely", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem02::RunTest(const FString&)
{
	FProductionSnapshot Production;
	FSubsystemFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	const auto Begin = Fixture.Session->BeginRun(Fdemo_mapBeginRunRequest());
	const auto End = Fixture.Session->CommitRuntimeSettlement(Fdemo_mapSettlementSummary());
	const auto Retry = Fixture.Session->RetryPendingSettlement();
	TestTrue(TEXT("Uninitialized Begin rejects"), Begin.Status == Edemo_mapProfileSessionBeginStatus::SessionNotReady);
	TestTrue(TEXT("Uninitialized Settlement rejects"), End.Status == Edemo_mapProfileSessionSettlementStatus::SessionStateRejected);
	TestTrue(TEXT("Uninitialized Retry rejects"), Retry.Status == Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement);
	TestTrue(TEXT("Runtime remains inactive"), Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty());
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem03, "demo_map.ProfileSessionSubsystem.03.ExplicitFreshReady", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem03::RunTest(const FString&)
{
	FSubsystemFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()));
	TestTrue(TEXT("Fresh explicit initialization Ready"), Init.IsReady() && Fixture.Session->IsExplicitlyInitialized() && Init.Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation);
	TestTrue(TEXT("Fresh identity, generation, and order"), Init.Snapshot.ProfileId.IsValid() && Init.Snapshot.SaveGeneration == 1 && Init.Snapshot.OrderedPermanentStash.Num() == 3
		&& Init.Snapshot.OrderedPermanentStash[0].ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade
		&& Init.Snapshot.OrderedPermanentStash[1].ItemDefinitionId == Fdemo_mapItemIds::TrainingVest
		&& Init.Snapshot.OrderedPermanentStash[2].ItemDefinitionId == Fdemo_mapItemIds::WindTalisman);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem04, "demo_map.ProfileSessionSubsystem.04.RepeatedInitializeDoesNotReloadOrSwitch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem04::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	const auto OtherStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	FSubsystemFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	const auto First = Fixture.Session->InitializeSession(Storage);
	TArray<uint8> Before; ReadBytes(Storage.PrimaryPath(), Before);
	const auto Same = Fixture.Session->InitializeSession(Storage);
	const auto Different = Fixture.Session->InitializeSession(OtherStorage);
	TArray<uint8> After; ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Repeated initialization keeps current snapshot"), Same.Snapshot.ProfileId == First.Snapshot.ProfileId && Same.Snapshot.SaveGeneration == First.Snapshot.SaveGeneration && Different.Snapshot.ProfileId == First.Snapshot.ProfileId && Different.Snapshot.SaveGeneration == First.Snapshot.SaveGeneration);
	TestTrue(TEXT("Repeated initialization performs no disk write"), Before == After);
	TestFalse(TEXT("Storage switching creates no alternate root"), IFileManager::Get().DirectoryExists(*OtherStorage.RootDirectory));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem05, "demo_map.ProfileSessionSubsystem.05.NewGameInstanceLoadsIdleAndTerminal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem05::RunTest(const FString&)
{
	const auto IdleStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	FGuid IdleProfile; int32 IdleGeneration = 0;
	{
		FSubsystemFixture First; if (!First.Start(*this)) return false;
		const auto Init = First.Session->InitializeSession(IdleStorage); IdleProfile = Init.Snapshot.ProfileId; IdleGeneration = Init.Snapshot.SaveGeneration;
	}
	{
		FSubsystemFixture Reloaded; if (!Reloaded.Start(*this)) return false;
		const auto Init = Reloaded.Session->InitializeSession(IdleStorage);
		TestTrue(TEXT("New GameInstance loads idle Profile"), Init.IsReady() && Init.Snapshot.ProfileId == IdleProfile && Init.Snapshot.SaveGeneration == IdleGeneration);
	}

	const auto TerminalStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	FGuid SettlementId; int32 TerminalGeneration = 0;
	{
		FSubsystemFixture First; if (!First.Start(*this)) return false;
		const auto Init = First.Session->InitializeSession(TerminalStorage);
		TestTrue(TEXT("Terminal fixture Begin"), First.Session->BeginRun(BeginRequest(Init.Snapshot)).IsRunActive());
		Fdemo_mapProfileSessionSettlementResult End; RequestAndCommit(*this, First, Edemo_mapRunEndReason::Death, End);
		SettlementId = End.Snapshot.LastSettlementId; TerminalGeneration = End.Snapshot.SaveGeneration;
	}
	{
		FSubsystemFixture Reloaded; if (!Reloaded.Start(*this)) return false;
		const auto Init = Reloaded.Session->InitializeSession(TerminalStorage);
		TestTrue(TEXT("New GameInstance loads terminal tombstone"), Init.IsReady() && Init.Snapshot.LastSettlementId == SettlementId && Init.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::Death && Init.Snapshot.SaveGeneration == TerminalGeneration);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem06, "demo_map.ProfileSessionSubsystem.06.EmptyLoadoutUsesSingleRunId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem06::RunTest(const FString&)
{
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()));
	const auto Begin = Fixture.Session->BeginRun(BeginRequest(Init.Snapshot));
	TestTrue(TEXT("Empty Begin committed and materialized"), Begin.IsRunActive() && Begin.PersistentResult.ActiveRunId.IsValid());
	TestTrue(TEXT("One RunId across persistent, adapter, and Runtime"), Begin.PersistentResult.ActiveRunId == Begin.Snapshot.ActiveRunId && Begin.Snapshot.ActiveRunId == Fixture.Runtime->GetActiveRunId());
	TestTrue(TEXT("Same GameInstance Runtime authority is empty"), Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty() && Fixture.Runtime->GetDeployedItemIds().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem07, "demo_map.ProfileSessionSubsystem.07.MaximumSixIdentityOrderAndRisk", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem07::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid DustA = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	const FGuid Iron = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 5);
	const FGuid DustB = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2);
	TestTrue(TEXT("Maximum-six fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot);
	Request.Loadout.WeaponItemInstanceId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId;
	Request.Loadout.ArmorItemInstanceId = Init.Snapshot.OrderedPermanentStash[1].ItemInstanceId;
	Request.Loadout.SpatialRingItemInstanceId = Init.Snapshot.OrderedPermanentStash[2].ItemInstanceId;
	Request.Loadout.MaterialStackItemInstanceIds = { DustB, Iron, DustA };
	const auto Begin = Fixture.Session->BeginRun(Request);
	const TArray<FGuid> Expected = { Request.Loadout.WeaponItemInstanceId, Request.Loadout.ArmorItemInstanceId, Request.Loadout.SpatialRingItemInstanceId, DustA, Iron, DustB };
	TestTrue(TEXT("Maximum six preserve canonical order"), Begin.IsRunActive() && Begin.PersistentResult.CommittedLoadoutPlan->DeployedItemIds == Expected && Begin.RuntimeResult.DeployedItemIds == Expected);
	for (const FGuid& Id : Expected)
	{
		TestTrue(TEXT("Original instance has one Runtime identity and risk domain"), Fixture.Runtime->GetAuthority().FindInstance(Id) != nullptr && Fixture.Runtime->GetDeployedItemIds().Contains(Id) && Fixture.Runtime->IsItemAtRiskInActiveRun(Id));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem08, "demo_map.ProfileSessionSubsystem.08.SecondBeginAndStaleIntentRejectAtomically", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem08::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Storage);
	Fdemo_mapBeginRunRequest Stale = BeginRequest(Init.Snapshot); --Stale.ExpectedSaveGeneration;
	TArray<uint8> Before; ReadBytes(Storage.PrimaryPath(), Before);
	const auto StaleResult = Fixture.Session->BeginRun(Stale);
	TArray<uint8> AfterStale; ReadBytes(Storage.PrimaryPath(), AfterStale);
	const auto First = Fixture.Session->BeginRun(BeginRequest(Init.Snapshot));
	const auto Second = Fixture.Session->BeginRun(BeginRequest(Init.Snapshot));
	TestTrue(TEXT("Stale intent rejects without mutation"), StaleResult.Status == Edemo_mapProfileSessionBeginStatus::StaleIntent && Before == AfterStale && Fixture.Runtime->GetActiveRunId() == First.Snapshot.ActiveRunId);
	TestTrue(TEXT("Second Begin rejects in RunActive"), First.IsRunActive() && Second.Status == Edemo_mapProfileSessionBeginStatus::SessionNotReady && Fixture.Runtime->GetActiveRunId() == First.Snapshot.ActiveRunId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem09, "demo_map.ProfileSessionSubsystem.09.ExtractionReturnsOriginalId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem09::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	FGuid WeaponId; FGuid SettlementId; int32 Generation = 0;
	{
		FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
		const auto Init = Fixture.Session->InitializeSession(Storage); WeaponId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId;
		Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = WeaponId;
		TestTrue(TEXT("Extraction fixture Begin"), Fixture.Session->BeginRun(Request).IsRunActive());
		Fdemo_mapProfileSessionSettlementResult End; RequestAndCommit(*this, Fixture, Edemo_mapRunEndReason::Extraction, End);
		TestTrue(TEXT("Original extracted ID returned"), ContainsId(End.Snapshot.OrderedPermanentStash, WeaponId));
		SettlementId = End.Snapshot.LastSettlementId; Generation = End.Snapshot.SaveGeneration;
	}
	FSubsystemFixture Reloaded; if (!Reloaded.Start(*this)) return false;
	const auto Load = Reloaded.Session->InitializeSession(Storage);
	TestTrue(TEXT("New GameInstance preserves extraction identity"), Load.IsReady() && ContainsId(Load.Snapshot.OrderedPermanentStash, WeaponId) && Load.Snapshot.LastSettlementId == SettlementId && Load.Snapshot.SaveGeneration == Generation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem10, "demo_map.ProfileSessionSubsystem.10.DeathAbandonAndWorldLeftRemainLost", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem10::RunTest(const FString&)
{
	for (const Edemo_mapRunEndReason Reason : { Edemo_mapRunEndReason::Death, Edemo_mapRunEndReason::Abandon })
	{
		FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
		const auto Init = Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()));
		const FGuid WeaponId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId;
		Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = WeaponId;
		TestTrue(TEXT("Loss fixture Begin"), Fixture.Session->BeginRun(Request).IsRunActive());
		Fdemo_mapProfileSessionSettlementResult End; RequestAndCommit(*this, Fixture, Reason, End);
		TestFalse(TEXT("Death or Abandon loses deployed original"), ContainsId(End.Snapshot.OrderedPermanentStash, WeaponId));
	}

	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid WorldLeftId = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
	TestTrue(TEXT("World-left fixture saved"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.MaterialStackItemInstanceIds = { WorldLeftId };
	TestTrue(TEXT("World-left fixture Begin"), Fixture.Session->BeginRun(Request).IsRunActive());
	TestTrue(TEXT("Deployed item moved to World"), const_cast<Fdemo_mapItemAuthority&>(Fixture.Runtime->GetAuthority()).MoveInventoryToWorld(WorldLeftId).bSuccess);
	Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("World-left extraction Runtime terminal"), Fixture.Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess && Summary.RuntimeSnapshot.OrderedSecuredItems.IsEmpty());
	const auto End = Fixture.Session->CommitRuntimeSettlement(Summary);
	TestTrue(TEXT("World-left extraction committed as loss"), End.IsDurablySettled() && !ContainsId(End.Snapshot.OrderedPermanentStash, WorldLeftId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem11, "demo_map.ProfileSessionSubsystem.11.PendingEvidenceRetriesExactlyOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem11::RunTest(const FString&)
{
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()));
	TestTrue(TEXT("Retry fixture Begin"), Fixture.Session->BeginRun(BeginRequest(Init.Snapshot)).IsRunActive());
	Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("Retry fixture Runtime terminal"), Fixture.Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess);
	const int32 PreparedGeneration = Fixture.Session->GetSnapshot().SaveGeneration;
	const int32 PreparedShopGeneration = Fixture.Session->GetSnapshot().ShopStock.Generation;
	Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp);
	const auto Failed = Fixture.Session->CommitRuntimeSettlement(Summary);
	const auto Retry = Fixture.Session->RetryPendingSettlement();
	const auto Again = Fixture.Session->RetryPendingSettlement();
	TestTrue(TEXT("Precommit failure retains evidence"), Failed.Status == Edemo_mapProfileSessionSettlementStatus::PendingRetry && Failed.Snapshot.bCanRetrySettlement && Failed.Snapshot.SaveGeneration == PreparedGeneration);
	TestTrue(TEXT("Explicit retry commits settlement and one terminal restock"), Retry.Status == Edemo_mapProfileSessionSettlementStatus::Committed
		&& Retry.Snapshot.SaveGeneration == PreparedGeneration + 2
		&& Retry.Snapshot.ShopStock.Generation == PreparedShopGeneration + 1
		&& Retry.Snapshot.ShopStock.LastAppliedTerminalId == Retry.Snapshot.LastSettlementId
		&& !Retry.Snapshot.bCanRetrySettlement);
	TestTrue(TEXT("Second retry has no evidence or additional commit"), Again.Status == Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement
		&& Again.Snapshot.SaveGeneration == Retry.Snapshot.SaveGeneration
		&& Again.Snapshot.ShopStock.ShopStockEventId == Retry.Snapshot.ShopStock.ShopStockEventId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem12, "demo_map.ProfileSessionSubsystem.12.PostCommitAmbiguityReloadsOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem12::RunTest(const FString&)
{
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	const auto Init = Fixture.Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()));
	TestTrue(TEXT("Ambiguity fixture Begin"), Fixture.Session->BeginRun(BeginRequest(Init.Snapshot)).IsRunActive());
	Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("Ambiguity fixture Runtime terminal"), Fixture.Runtime->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess);
	Fixture.Session->SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary);
	const auto End = Fixture.Session->CommitRuntimeSettlement(Summary);
	TestTrue(TEXT("Postcommit ambiguity reconciled by one reload"), End.Status == Edemo_mapProfileSessionSettlementStatus::ReconciledAfterReload && End.Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation && End.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::Extraction);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem13, "demo_map.ProfileSessionSubsystem.13.RuntimeRollbackThenNewGameInstanceRecovers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem13::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()); FGuid FailedRun;
	{
		FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
		const auto Init = Fixture.Session->InitializeSession(Storage); Fdemo_mapBeginRunRequest Request = BeginRequest(Init.Snapshot); Request.Loadout.WeaponItemInstanceId = Init.Snapshot.OrderedPermanentStash[0].ItemInstanceId;
		Fixture.Runtime->SetPreparedRunFailureAfterMutationForAutomation(1);
		const auto Begin = Fixture.Session->BeginRun(Request); FailedRun = Begin.Snapshot.ActiveRunId;
		TestTrue(TEXT("Runtime failure rolls back and requires recovery"), Begin.Status == Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed && Begin.Snapshot.SessionState == Edemo_mapProfileSessionState::RecoveryRequired && Fixture.Runtime->GetRunState() == Edemo_mapRunState::Inactive && Fixture.Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty());
	}
	Fdemo_mapProfileRepository Repository; const auto Prepared = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Failed materialization leaves Prepared on disk"), Prepared.IsSuccess() && Prepared.Profile.ActiveRun.bHasActiveRun && Prepared.Profile.ActiveRun.ActiveRunId == FailedRun && Prepared.Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::Prepared);
	FSubsystemFixture Recovery; if (!Recovery.Start(*this)) return false;
	const auto Init = Recovery.Session->InitializeSession(Storage);
	TestTrue(TEXT("New GameInstance resolves Prepared once"), Init.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && Init.Snapshot.ActiveRunId == FailedRun && Init.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem14, "demo_map.ProfileSessionSubsystem.14.DeinitializeDoesNotSettlePrepared", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem14::RunTest(const FString&)
{
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSubsystemRoot()); FGuid RunId; int32 PreparedGeneration = 0; TArray<uint8> BeforeShutdown;
	{
		FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
		const auto Init = Fixture.Session->InitializeSession(Storage); const auto Begin = Fixture.Session->BeginRun(BeginRequest(Init.Snapshot));
		RunId = Begin.Snapshot.ActiveRunId; PreparedGeneration = Begin.Snapshot.SaveGeneration; ReadBytes(Storage.PrimaryPath(), BeforeShutdown);
		TestTrue(TEXT("Prepared fixture active"), Begin.IsRunActive());
	}
	TArray<uint8> AfterShutdown; ReadBytes(Storage.PrimaryPath(), AfterShutdown);
	const auto StillPrepared = Fdemo_mapProfileRepository().LoadExistingProfile(Storage);
	TestTrue(TEXT("Deinitialize performs no persistent write"), BeforeShutdown == AfterShutdown && StillPrepared.IsSuccess() && StillPrepared.Profile.ActiveRun.bHasActiveRun && StillPrepared.Profile.ActiveRun.ActiveRunId == RunId && StillPrepared.Profile.SaveGeneration == PreparedGeneration);
	FGuid SettlementId; int32 RecoveredGeneration = 0;
	{
		FSubsystemFixture Recovery; if (!Recovery.Start(*this)) return false;
		const auto Init = Recovery.Session->InitializeSession(Storage);
		TestTrue(TEXT("New GameInstance recovers once"), Init.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted && Init.Snapshot.ActiveRunId == RunId && Init.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon);
		SettlementId = Init.Snapshot.LastSettlementId; RecoveredGeneration = Init.Snapshot.SaveGeneration;
	}
	FSubsystemFixture Idempotent; if (!Idempotent.Start(*this)) return false;
	const auto Reload = Idempotent.Session->InitializeSession(Storage);
	TestTrue(TEXT("Further GameInstance reload is idempotent"), Reload.IsReady() && Reload.Snapshot.LastSettlementId == SettlementId && Reload.Snapshot.SaveGeneration == RecoveredGeneration && Reload.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSessionSubsystem15, "demo_map.ProfileSessionSubsystem.15.LazySubsystemSingleStartupOwnerProductionZeroIO", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSessionSubsystem15::RunTest(const FString&)
{
	FProductionSnapshot Production;
	FSubsystemFixture Fixture; if (!Fixture.Start(*this)) return false;
	TestTrue(TEXT("Bare GameInstance creation leaves adapter lazy until the V3 startup owner initializes it"), Fixture.Session->GetSnapshot().SessionState == Edemo_mapProfileSessionState::Uninitialized && !Fixture.Session->IsExplicitlyInitialized());
	TArray<FString> Sources;
	IFileManager::Get().FindFilesRecursive(Sources, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("demo_map")), TEXT("*.cpp"), true, false);
	bool bUnexpected = false;
	for (const FString& Path : Sources)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path) || !Text.Contains(TEXT("Udemo_mapProfileSessionSubsystem")))
		{
			continue;
		}
		if (!Path.EndsWith(TEXT("Tests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileSessionSubsystem.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileSessionSubsystemTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationWidget.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationUITests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationFlow.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfilePreparationFlowTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileNormalStartupTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapProfileTradeTests.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapHUD.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapPlayerController.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapSpiritStonePickup.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapV3ProgressionManager.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapSectNavigationWidget.cpp"))
			&& !Path.EndsWith(TEXT("demo_map0909BFramework.cpp"))
			&& !Path.EndsWith(TEXT("demo_mapShanmenItemCutover.cpp")))
		{
			bUnexpected = true;
			AddError(FString::Printf(TEXT("Unexpected normal-startup Profile Session Subsystem reference: %s"), *Path));
		}
	}
	TestFalse(TEXT("Only the existing lifecycle, tests, and Sect product CTA may reference the Session adapter"), bUnexpected);
	TestTrue(TEXT("Subsystem tests leave Production Save unchanged"), Production.IsUnchanged());
	return true;
}

#endif
