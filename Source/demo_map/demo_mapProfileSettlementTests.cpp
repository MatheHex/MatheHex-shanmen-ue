#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString NewSettlementRoot()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("Dev.D.UE.0.0.4.7.r0"), TEXT("ProfileSettlement"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	struct FProductionSnapshot
	{
		TArray<FString> Paths;
		TArray<bool> Existed;
		TArray<TArray<uint8>> Bytes;

		FProductionSnapshot()
		{
			const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
			Paths = { Production.PrimaryPath(), Production.BackupPath(), Production.TempPath() };
			for (const FString& Path : Paths)
			{
				const bool bExists = IFileManager::Get().FileExists(*Path);
				Existed.Add(bExists);
				TArray<uint8> Value;
				if (bExists) FFileHelper::LoadFileToArray(Value, *Path);
				Bytes.Add(MoveTemp(Value));
			}
		}

		bool IsUnchanged() const
		{
			for (int32 Index = 0; Index < Paths.Num(); ++Index)
			{
				if (IFileManager::Get().FileExists(*Paths[Index]) != Existed[Index]) return false;
				if (Existed[Index])
				{
					TArray<uint8> After;
					if (!FFileHelper::LoadFileToArray(After, *Paths[Index]) || After != Bytes[Index]) return false;
				}
			}
			return true;
		}
	};

	Fdemo_mapPersistentProfile PreparedProfile(Fdemo_mapProfileRepository& Repository, const Fdemo_mapProfileStorageContext& Storage, bool bAddMaterial = false)
	{
		Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
		FGuid MaterialId;
		if (bAddMaterial)
		{
			Fdemo_mapPersistentItemRecord Material;
			Material.ItemInstanceId = FGuid::NewGuid();
			Material.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust;
			Material.StackCount = 4;
			Profile.PermanentStash.Add(Material);
			MaterialId = Material.ItemInstanceId;
			Repository.SaveProfile(Profile, Storage);
		}
		Fdemo_mapBeginRunRequest Begin;
		Begin.ExpectedProfileId = Profile.ProfileId;
		Begin.ExpectedSaveGeneration = Profile.SaveGeneration;
		Begin.Loadout.WeaponItemInstanceId = Profile.PermanentStash[0].ItemInstanceId;
		if (MaterialId.IsValid()) Begin.Loadout.MaterialStackItemInstanceIds.Add(MaterialId);
		Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Begin, Repository, Storage);
		return Profile;
	}

	Fdemo_mapRuntimeSettlementSnapshot SnapshotFor(
		const Fdemo_mapPersistentProfile& Profile,
		Edemo_mapRunEndReason Reason,
		bool bIncludeDeployed = true)
	{
		Fdemo_mapRuntimeSettlementSnapshot Snapshot;
		Snapshot.ActiveRunId = Profile.ActiveRun.ActiveRunId;
		Snapshot.CommittedEndReason = Reason;
		Snapshot.bValid = true;
		if (bIncludeDeployed && Reason == Edemo_mapRunEndReason::Extraction)
		{
			for (const Fdemo_mapPersistentItemRecord& Record : Profile.ActiveRun.ActiveRunItems)
			{
				Fdemo_mapRuntimeSettlementItem Item;
				Item.ItemInstanceId = Record.ItemInstanceId;
				Item.ItemDefinitionId = Record.ItemDefinitionId;
				Item.StackCount = Record.StackCount;
				Item.OriginRunId = Record.OriginRunId;
				Snapshot.OrderedSecuredItems.Add(Item);
			}
		}
		return Snapshot;
	}

	Fdemo_mapProfileSettlementRequest RequestFor(
		const Fdemo_mapPersistentProfile& Profile,
		Edemo_mapRunEndReason Reason,
		bool bAttachSnapshot = false)
	{
		Fdemo_mapProfileSettlementRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		Request.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
		Request.RequestedEndReason = Reason;
		if (bAttachSnapshot) Request.RuntimeSnapshot = SnapshotFor(Profile, Reason);
		return Request;
	}

	bool IsTombstone(const Fdemo_mapPersistentProfile& Profile, Edemo_mapPersistentActiveRunState State)
	{
		return !Profile.ActiveRun.bHasActiveRun
			&& Profile.ActiveRun.ActiveRunState == State
			&& Profile.ActiveRun.ActiveRunId.IsValid()
			&& Profile.ActiveRun.ActiveRunItems.IsEmpty()
			&& Profile.ActiveRun.DeployedItemIds.IsEmpty()
			&& Profile.ActiveRun.CommittedSettlementId.IsValid()
			&& Profile.ActiveRun.CommittedSettlementId == Profile.LastSettlementId;
	}

	Udemo_mapItemSubsystem* NewRuntime()
	{
		return NewObject<Udemo_mapItemSubsystem>(NewObject<UGameInstance>(GetTransientPackage()));
	}

	Fdemo_mapCommittedRunLoadoutPlan RuntimePlan()
	{
		Fdemo_mapCommittedRunLoadoutPlan Plan;
		Plan.ProfileId = FGuid::NewGuid();
		Plan.CommittedGeneration = 1;
		Plan.ActiveRunId = FGuid::NewGuid();
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust;
		Item.StackCount = 4;
		Item.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
		Plan.OrderedItems.Add(Item);
		Plan.DeployedItemIds.Add(Item.ItemInstanceId);
		return Plan;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement01, "demo_map.ProfileSettlement.01.ExtractionIdentityOrderAndTombstone", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement01::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage, true); const auto Before = Profile; const TArray<Fdemo_mapPersistentItemRecord> Survivors = Profile.PermanentStash;
	auto Snapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); Fdemo_mapRuntimeSettlementItem Acquired; Acquired.ItemInstanceId = FGuid::NewGuid(); Acquired.ItemDefinitionId = Fdemo_mapItemIds::AncientToken; Acquired.StackCount = 1; Acquired.OriginRunId = Profile.ActiveRun.ActiveRunId; Snapshot.OrderedSecuredItems.Insert(Acquired, 1);
	auto Request = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Request.RuntimeSnapshot = Snapshot; const auto Result = Fdemo_mapProfileSettlementTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Extraction committed"), Result.IsCommitted() && IsTombstone(Profile, Edemo_mapPersistentActiveRunState::Extraction)); TestEqual(TEXT("Generation once"), Profile.SaveGeneration, Before.SaveGeneration + 1); TestTrue(TEXT("Unique settlement"), Result.SettlementId.IsValid() && Result.SettlementId != Before.ActiveRun.ActiveRunId && Result.SettlementId != Before.ProfileId);
	TestEqual(TEXT("Survivors then secured"), Profile.PermanentStash.Num(), Survivors.Num() + Snapshot.OrderedSecuredItems.Num()); for (int32 Index = 0; Index < Survivors.Num(); ++Index) TestTrue(TEXT("Existing order preserved"), Profile.PermanentStash[Index] == Survivors[Index]);
	for (int32 Index = 0; Index < Snapshot.OrderedSecuredItems.Num(); ++Index) { const auto& Stored = Profile.PermanentStash[Survivors.Num() + Index]; const auto& Runtime = Snapshot.OrderedSecuredItems[Index]; TestTrue(TEXT("Same instance appended in Snapshot order"), Stored.ItemInstanceId == Runtime.ItemInstanceId && Stored.ItemDefinitionId == Runtime.ItemDefinitionId && Stored.StackCount == Runtime.StackCount && Stored.PersistentDomain == Edemo_mapPersistentDomain::PermanentStash && Stored.EquipmentSlotId.IsNone() && !Stored.OriginRunId.IsValid()); }
	TestTrue(TEXT("Result order"), Result.OrderedSecuredItemIds == TArray<FGuid>{Snapshot.OrderedSecuredItems[0].ItemInstanceId, Snapshot.OrderedSecuredItems[1].ItemInstanceId, Snapshot.OrderedSecuredItems[2].ItemInstanceId}); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement02, "demo_map.ProfileSettlement.02.ExtractionWorldLeftRiskIsLost", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement02::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); const FGuid LostId = Profile.ActiveRun.ActiveRunItems[0].ItemInstanceId; auto Request = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Request.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction, false); const auto Result = Fdemo_mapProfileSettlementTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Empty extraction commits"), Result.IsCommitted()); TestFalse(TEXT("Unsecured risk absent"), Profile.PermanentStash.ContainsByPredicate([LostId](const auto& Item){ return Item.ItemInstanceId == LostId; })); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement03, "demo_map.ProfileSettlement.03.DeathAndAbandonLoseRisk", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement03::RunTest(const FString&)
{
	for (const auto Pair : { TPair<Edemo_mapRunEndReason, Edemo_mapPersistentActiveRunState>(Edemo_mapRunEndReason::Death, Edemo_mapPersistentActiveRunState::Death), TPair<Edemo_mapRunEndReason, Edemo_mapPersistentActiveRunState>(Edemo_mapRunEndReason::Abandon, Edemo_mapPersistentActiveRunState::Abandon) })
	{
		Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); const FGuid Risk = Profile.ActiveRun.ActiveRunItems[0].ItemInstanceId; auto Request = RequestFor(Profile, Pair.Key); Request.RuntimeSnapshot = SnapshotFor(Profile, Pair.Key, false); const auto Result = Fdemo_mapProfileSettlementTransaction().Execute(Profile, Request, Repository, Storage);
		TestTrue(TEXT("Terminal commits"), Result.IsCommitted() && IsTombstone(Profile, Pair.Value)); TestFalse(TEXT("Risk lost"), Profile.PermanentStash.ContainsByPredicate([Risk](const auto& Item){ return Item.ItemInstanceId == Risk; }));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement04, "demo_map.ProfileSettlement.04.RecoveredAbandonWithoutRuntime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement04::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Prepared = PreparedProfile(Repository, Storage); const FGuid Risk = Prepared.ActiveRun.ActiveRunItems[0].ItemInstanceId; Fdemo_mapPersistentProfile Reloaded = Repository.LoadExistingProfile(Storage).Profile; const auto Result = Fdemo_mapProfileSettlementTransaction().Execute(Reloaded, RequestFor(Reloaded, Edemo_mapRunEndReason::RecoveredAbandon), Repository, Storage);
	TestTrue(TEXT("RecoveredAbandon commits reloaded Prepared"), Result.IsCommitted() && IsTombstone(Reloaded, Edemo_mapPersistentActiveRunState::RecoveredAbandon)); TestFalse(TEXT("Risk lost"), Reloaded.PermanentStash.ContainsByPredicate([Risk](const auto& Item){ return Item.ItemInstanceId == Risk; })); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement05, "demo_map.ProfileSettlement.05.ReloadReplayAndCompetingReasonFirstWins", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement05::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); const auto OriginalRequest = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); auto FirstRequest = OriginalRequest; FirstRequest.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); const auto First = Fdemo_mapProfileSettlementTransaction().Execute(Profile, FirstRequest, Repository, Storage); Fdemo_mapPersistentProfile Reloaded = Repository.LoadExistingProfile(Storage).Profile; auto Competing = OriginalRequest; Competing.RequestedEndReason = Edemo_mapRunEndReason::Death; const auto Again = Fdemo_mapProfileSettlementTransaction().Execute(Reloaded, Competing, Repository, Storage);
	TestTrue(TEXT("Replay succeeds without Save"), First.IsCommitted() && Again.Status == Edemo_mapProfileSettlementStatus::AlreadyCommitted && !Again.bDiskStateChanged); TestTrue(TEXT("First identity and reason immutable"), Again.SettlementId == First.SettlementId && Again.CommittedEndReason == Edemo_mapRunEndReason::Extraction && Reloaded.SaveGeneration == Profile.SaveGeneration); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement06, "demo_map.ProfileSettlement.06.BeginRunReplacesTombstone", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement06::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); auto Settlement = RequestFor(Profile, Edemo_mapRunEndReason::Death); const FGuid OldRun = Profile.ActiveRun.ActiveRunId; const auto End = Fdemo_mapProfileSettlementTransaction().Execute(Profile, Settlement, Repository, Storage); const FGuid LastSettlement = Profile.LastSettlementId; Fdemo_mapBeginRunRequest Begin; Begin.ExpectedProfileId = Profile.ProfileId; Begin.ExpectedSaveGeneration = Profile.SaveGeneration; const auto Next = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Begin, Repository, Storage);
	TestTrue(TEXT("New Prepared replaces tombstone"), End.IsCommitted() && Next.IsCommitted() && Profile.ActiveRun.bHasActiveRun && Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::Prepared); TestTrue(TEXT("Identities do not collide"), Profile.ActiveRun.ActiveRunId != OldRun && Profile.ActiveRun.ActiveRunId != LastSettlement && Profile.LastSettlementId == LastSettlement && !Profile.ActiveRun.CommittedSettlementId.IsValid()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement07, "demo_map.ProfileSettlement.07.SnapshotPresenceAndBoundaryValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement07::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage);
	TestTrue(TEXT("Missing extraction Snapshot"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, RequestFor(Profile, Edemo_mapRunEndReason::Extraction), Repository, Storage).Status == Edemo_mapProfileSettlementStatus::SnapshotMissing);
	auto Extra = RequestFor(Profile, Edemo_mapRunEndReason::RecoveredAbandon); Extra.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::RecoveredAbandon, false); TestTrue(TEXT("Recovered Snapshot extra"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Extra, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::SnapshotUnexpected);
	auto WrongRun = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); WrongRun.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); WrongRun.RuntimeSnapshot->ActiveRunId = FGuid::NewGuid(); TestTrue(TEXT("Snapshot Run mismatch"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, WrongRun, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::SnapshotRunIdMismatch);
	auto WrongReason = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); WrongReason.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Death, false); TestTrue(TEXT("Snapshot reason mismatch"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, WrongReason, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::SnapshotReasonMismatch);
	auto Invalid = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Invalid.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); Invalid.RuntimeSnapshot->bValid = false; TestTrue(TEXT("Invalid Snapshot"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Invalid, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::SnapshotInvalid); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement08, "demo_map.ProfileSettlement.08.ItemDuplicateAndPersistentConflict", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement08::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); auto Duplicate = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Duplicate.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); const Fdemo_mapRuntimeSettlementItem DuplicateItem = Duplicate.RuntimeSnapshot->OrderedSecuredItems[0]; Duplicate.RuntimeSnapshot->OrderedSecuredItems.Add(DuplicateItem); TestTrue(TEXT("Duplicate"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Duplicate, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::ItemIdentityDuplicate);
	auto Conflict = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Conflict.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); Conflict.RuntimeSnapshot->OrderedSecuredItems[0].ItemInstanceId = Profile.PermanentStash[0].ItemInstanceId; TestTrue(TEXT("Conflict"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Conflict, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::ItemIdentityConflict); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement09, "demo_map.ProfileSettlement.09.DeployedAndAcquiredIdentityValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement09::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage);
	auto Deployed = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Deployed.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction); Deployed.RuntimeSnapshot->OrderedSecuredItems[0].ItemDefinitionId = Fdemo_mapItemIds::TrainingVest; TestTrue(TEXT("Deployed mismatch"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Deployed, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::DeployedIdentityMismatch);
	auto Acquired = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Acquired.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction, false); Fdemo_mapRuntimeSettlementItem Item; Item.ItemInstanceId = FGuid::NewGuid(); Item.ItemDefinitionId = Fdemo_mapItemIds::AncientToken; Item.StackCount = 1; Item.OriginRunId = FGuid::NewGuid(); Acquired.RuntimeSnapshot->OrderedSecuredItems.Add(Item); TestTrue(TEXT("Acquired Origin mismatch"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Acquired, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::AcquiredOriginMismatch); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement10, "demo_map.ProfileSettlement.10.DefinitionAndQuantityValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement10::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); Fdemo_mapRuntimeSettlementItem Item; Item.ItemInstanceId = FGuid::NewGuid(); Item.ItemDefinitionId = TEXT("Prototype.Item.Unknown"); Item.StackCount = 1; Item.OriginRunId = Profile.ActiveRun.ActiveRunId;
	auto Unknown = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Unknown.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction, false); Unknown.RuntimeSnapshot->OrderedSecuredItems.Add(Item); TestTrue(TEXT("Unknown"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Unknown, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::UnknownItemDefinition);
	Item.ItemDefinitionId = Fdemo_mapItemIds::AncientToken; Item.StackCount = 2; auto Quantity = RequestFor(Profile, Edemo_mapRunEndReason::Extraction); Quantity.RuntimeSnapshot = SnapshotFor(Profile, Edemo_mapRunEndReason::Extraction, false); Quantity.RuntimeSnapshot->OrderedSecuredItems.Add(Item); TestTrue(TEXT("Quantity"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, Quantity, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::InvalidItemQuantity); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement11, "demo_map.ProfileSettlement.11.OptimisticIdentityAndStateRejections", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement11::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); auto Request = RequestFor(Profile, Edemo_mapRunEndReason::Death);
	auto WrongProfile = Request; WrongProfile.ExpectedProfileId = FGuid::NewGuid(); TestTrue(TEXT("Profile"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, WrongProfile, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::ProfileIdentityMismatch);
	auto WrongGeneration = Request; --WrongGeneration.ExpectedSaveGeneration; TestTrue(TEXT("Generation"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, WrongGeneration, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::ProfileGenerationMismatch);
	auto WrongRun = Request; WrongRun.ExpectedActiveRunId = FGuid::NewGuid(); TestTrue(TEXT("Run"), Fdemo_mapProfileSettlementTransaction().Execute(Profile, WrongRun, Repository, Storage).Status == Edemo_mapProfileSettlementStatus::ActiveRunIdMismatch);
	Fdemo_mapPersistentProfile InProgress = Profile; InProgress.ActiveRun.ActiveRunState = Edemo_mapPersistentActiveRunState::InProgress; TestTrue(TEXT("State"), Fdemo_mapProfileSettlementTransaction().Execute(InProgress, RequestFor(InProgress, Edemo_mapRunEndReason::Death), Repository, Storage).Status == Edemo_mapProfileSettlementStatus::InvalidActiveRunState);
	const auto IdleStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Idle = Repository.LoadOrCreateDefaultProfile(IdleStorage).Profile; auto IdleRequest = RequestFor(Idle, Edemo_mapRunEndReason::Death); TestTrue(TEXT("No Prepared"), Fdemo_mapProfileSettlementTransaction().Execute(Idle, IdleRequest, Repository, IdleStorage).Status == Edemo_mapProfileSettlementStatus::NoPreparedRun); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement12, "demo_map.ProfileSettlement.12.RepositoryPrecommitFailureIsAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement12::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto BaseStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, BaseStorage); const auto Before = Profile; TArray<uint8> BeforeBytes; FFileHelper::LoadFileToArray(BeforeBytes, *BaseStorage.PrimaryPath()); auto FailureStorage = BaseStorage; FailureStorage.InjectedFailure = Edemo_mapProfileFailureStage::WriteTemp; const auto Result = Fdemo_mapProfileSettlementTransaction().Execute(Profile, RequestFor(Profile, Edemo_mapRunEndReason::Death), Repository, FailureStorage); TArray<uint8> AfterBytes; FFileHelper::LoadFileToArray(AfterBytes, *BaseStorage.PrimaryPath());
	TestTrue(TEXT("Save rejected"), Result.Status == Edemo_mapProfileSettlementStatus::RepositorySaveRejected && Result.RepositorySaveStatus == Edemo_mapProfileSaveStatus::TempWriteFailed); TestTrue(TEXT("Caller and primary unchanged"), Profile == Before && BeforeBytes == AfterBytes && Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::Prepared); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement13, "demo_map.ProfileSettlement.13.PostCommitRequiresReloadThenReplays", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement13::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const auto BaseStorage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, BaseStorage); const auto Before = Profile; const auto OriginalRequest = RequestFor(Profile, Edemo_mapRunEndReason::Death); auto FailureStorage = BaseStorage; FailureStorage.InjectedFailure = Edemo_mapProfileFailureStage::ReadBackCommittedPrimary; const auto Uncertain = Fdemo_mapProfileSettlementTransaction().Execute(Profile, OriginalRequest, Repository, FailureStorage);
	TestTrue(TEXT("Caller unchanged after uncertain commit"), Uncertain.Status == Edemo_mapProfileSettlementStatus::CommitOutcomeRequiresReload && Uncertain.bDiskStateChanged && Profile == Before); Fdemo_mapPersistentProfile Reloaded = Repository.LoadExistingProfile(BaseStorage).Profile; const auto Replay = Fdemo_mapProfileSettlementTransaction().Execute(Reloaded, OriginalRequest, Repository, BaseStorage); TestTrue(TEXT("Reload sees durable tombstone"), IsTombstone(Reloaded, Edemo_mapPersistentActiveRunState::Death) && Replay.Status == Edemo_mapProfileSettlementStatus::AlreadyCommitted && Replay.SettlementId == Reloaded.LastSettlementId); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement14, "demo_map.ProfileSettlement.14.TerminalSchemaOneJsonRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement14::RunTest(const FString&)
{
	for (const Edemo_mapRunEndReason Reason : { Edemo_mapRunEndReason::Extraction, Edemo_mapRunEndReason::Death, Edemo_mapRunEndReason::Abandon, Edemo_mapRunEndReason::RecoveredAbandon })
	{
		Fdemo_mapProfileRepository Repository; const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot()); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); auto Request = RequestFor(Profile, Reason); if (Reason == Edemo_mapRunEndReason::Extraction) Request.RuntimeSnapshot = SnapshotFor(Profile, Reason); const auto End = Fdemo_mapProfileSettlementTransaction().Execute(Profile, Request, Repository, Storage); const auto Load = Repository.LoadExistingProfile(Storage); TestTrue(TEXT("Round trip"), End.IsCommitted() && Load.IsSuccess() && Load.Profile == Profile && Load.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement15, "demo_map.ProfileSettlement.15.RuntimeSnapshotIdentityOrderAndRepeat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement15::RunTest(const FString&)
{
	auto* Items = NewRuntime(); const auto Plan = RuntimePlan(); Fdemo_mapPreparedRunRuntimeRequest Materialize; Materialize.CommittedPlan = Plan; TestTrue(TEXT("Materialized"), Items->MaterializePreparedRun(Materialize).IsMaterialized()); TArray<FGuid> Acquired; Items->AddDefinition(Fdemo_mapItemIds::AncientToken, 1, &Acquired); Fdemo_mapSettlementSummary First, Again; TestTrue(TEXT("Extraction"), Items->RequestSettlement(Edemo_mapRunEndReason::Extraction, First).bSuccess && First.RuntimeSnapshot.bValid && First.RuntimeSnapshot.ActiveRunId == Plan.ActiveRunId && First.RuntimeSnapshot.CommittedEndReason == Edemo_mapRunEndReason::Extraction);
	TArray<FGuid> Expected; for (const FGuid& Id : Items->GetAuthority().GetSessionStashSnapshot()) { const auto* Item = Items->GetAuthority().FindInstance(Id); if (Item && (Items->GetDeployedItemIds().Contains(Id) || Item->OriginRunId == Plan.ActiveRunId)) Expected.Add(Id); } TArray<FGuid> Actual; for (const auto& Item : First.RuntimeSnapshot.OrderedSecuredItems) { Actual.Add(Item.ItemInstanceId); const auto* Runtime = Items->GetAuthority().FindInstance(Item.ItemInstanceId); TestTrue(TEXT("Pure-value identity exact"), Runtime && Runtime->DefinitionId == Item.ItemDefinitionId && Runtime->Quantity == Item.StackCount && Runtime->OriginRunId == Item.OriginRunId); }
	TestTrue(TEXT("Session Stash order"), Actual == Expected && Actual.Contains(Plan.DeployedItemIds[0]) && Actual.Contains(Acquired[0])); const auto Repeat = Items->RequestSettlement(Edemo_mapRunEndReason::Death, Again); TestTrue(TEXT("Repeated terminal returns first Snapshot semantics"), !Repeat.bSuccess && Again.RuntimeSnapshot == First.RuntimeSnapshot); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement16, "demo_map.ProfileSettlement.16.RuntimeSnapshotFiltersPriorRunStash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement16::RunTest(const FString&)
{
	auto* Items = NewRuntime(); Items->BeginRun(); TArray<FGuid> Prior; Items->AddDefinition(Fdemo_mapItemIds::AncientToken, 1, &Prior); Fdemo_mapSettlementSummary PriorSummary; Items->RequestSettlement(Edemo_mapRunEndReason::Extraction, PriorSummary); Items->BeginRun(); const FGuid CurrentRun = Items->GetActiveRunId(); TArray<FGuid> Current; Items->AddDefinition(Fdemo_mapItemIds::SpiritDust, 2, &Current); Fdemo_mapSettlementSummary Summary; Items->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary); TArray<FGuid> Secured; for (const auto& Item : Summary.RuntimeSnapshot.OrderedSecuredItems) Secured.Add(Item.ItemInstanceId);
	TestTrue(TEXT("Current item included"), Current.Num() == 1 && Secured.Contains(Current[0])); TestTrue(TEXT("Prior item filtered"), Prior.Num() == 1 && !Secured.Contains(Prior[0])); TestTrue(TEXT("Current Origin retained"), Summary.RuntimeSnapshot.OrderedSecuredItems.Num() == 1 && Summary.RuntimeSnapshot.OrderedSecuredItems[0].OriginRunId == CurrentRun); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement17, "demo_map.ProfileSettlement.17.NonExtractionRuntimeSnapshotIsEmpty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement17::RunTest(const FString&)
{
	for (const Edemo_mapRunEndReason Reason : { Edemo_mapRunEndReason::Death, Edemo_mapRunEndReason::Abandon })
	{
		auto* Items = NewRuntime(); Items->BeginRun(); Items->AddDefinition(Fdemo_mapItemIds::SpiritDust, 2); Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("Terminal"), Items->RequestSettlement(Reason, Summary).bSuccess); TestTrue(TEXT("Snapshot valid and empty"), Summary.RuntimeSnapshot.bValid && Summary.RuntimeSnapshot.CommittedEndReason == Reason && Summary.RuntimeSnapshot.OrderedSecuredItems.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement18, "demo_map.ProfileSettlement.18.AutomationAndProductionIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement18::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const FString Root = NewSettlementRoot(); const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); Fdemo_mapPersistentProfile Profile = PreparedProfile(Repository, Storage); const auto Result = Fdemo_mapProfileSettlementTransaction().Execute(Profile, RequestFor(Profile, Edemo_mapRunEndReason::RecoveredAbandon), Repository, Storage); TestTrue(TEXT("Unique automation root"), Result.IsCommitted() && (Root.Contains(TEXT("Saved/Automation/Dev.D.UE.0.0.4.7.r0/ProfileSettlement")) || Root.Contains(TEXT("Saved\\Automation\\Dev.D.UE.0.0.4.7.r0\\ProfileSettlement")))); TestTrue(TEXT("Production untouched"), Production.IsUnchanged());
	TArray<FString> Sources; IFileManager::Get().FindFilesRecursive(Sources, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("demo_map")), TEXT("*.cpp"), true, false); bool bUnexpected = false; for (const FString& Path : Sources) { FString Text; if (!FFileHelper::LoadFileToString(Text, *Path) || !Text.Contains(TEXT("Fdemo_mapProfileSettlementTransaction"))) continue; if (!Path.EndsWith(TEXT("demo_mapProfileSettlementTransaction.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSessionCoordinator.cpp")) && !Path.EndsWith(TEXT("Tests.cpp"))) { bUnexpected = true; AddError(FString::Printf(TEXT("Unexpected production/runtime Settlement transaction reference: %s"), *Path)); } } TestFalse(TEXT("No normal startup, ItemSubsystem, Manager, UI, or map hook"), bUnexpected); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSettlement19, "demo_map.ProfileSettlement.19.DeathClearsLostSpatialRingLayout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSettlement19::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const auto Storage = Fdemo_mapProfileStorageContext::ForRoot(NewSettlementRoot());
	Fdemo_mapPersistentProfile Profile = Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid SpatialRingId = Profile.PermanentStash[2].ItemInstanceId;
	// This is the historical prepared-run transaction, not the P4x product
	// direct-start route.  Keep its settlement reconciliation contract covered
	// without requiring direct Start Run to write legacy PreparationLayout data.
	Profile.PreparationLayout.SpatialRingItemInstanceId = SpatialRingId;
	Fdemo_mapBeginRunRequest Begin;
	Begin.ExpectedProfileId = Profile.ProfileId;
	Begin.ExpectedSaveGeneration = Profile.SaveGeneration;
	Begin.bRequireCommittedPreparationLayout = true;
	const Fdemo_mapBeginRunResult Begun =
		Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Begin, Repository, Storage);
	const Fdemo_mapProfileSettlementResult Settled =
		Fdemo_mapProfileSettlementTransaction().Execute(
			Profile,
			RequestFor(Profile, Edemo_mapRunEndReason::Death),
			Repository,
			Storage);
	TestTrue(
		TEXT("Death removes the lost spatial ring from the persistent layout"),
		Begun.IsCommitted()
			&& Settled.IsCommitted()
			&& !Profile.PreparationLayout.SpatialRingItemInstanceId.IsValid()
			&& Settled.ClearedPreparationItemIds.Contains(SpatialRingId));
	return true;
}

#endif
