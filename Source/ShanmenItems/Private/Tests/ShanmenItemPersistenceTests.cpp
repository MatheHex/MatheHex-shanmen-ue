#if WITH_DEV_AUTOMATION_TESTS

#include "ShanmenItemPersistence.h"

#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const FGuid PersistenceOwnerId(0x50120001, 0, 0, 1);
	const FGuid PersistenceRunId(0x50120002, 0, 0, 1);
	const FGuid PersistenceContainerId(0x50120003, 0, 0, 1);
	const FGuid PersistenceItemId(0x50120004, 0, 0, 1);
	const FGuid PersistenceMigrationId(0x50120005, 0, 0, 1);

	FString NewPersistenceRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.2.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp PersistenceContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.Items.0.0.10.P1.2");
		Content.Digest = TEXT("P1.2.AuthorityDocument.Contract.v1");
		return Content;
	}

	FShanmenItemAuthoritySnapshot PersistenceCandidate(int32 Quantity = 5)
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Snapshot.Content = PersistenceContent();

		FShanmenItemDefinition Definition;
		Definition.DefinitionId = TEXT("Item.Test.PersistenceStack");
		Definition.MaxStack = 20;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		Snapshot.Definitions.Add(Definition);

		FShanmenItemContainer Container;
		Container.ContainerId = PersistenceContainerId;
		Container.RunId = PersistenceRunId;
		Container.OwnerId = PersistenceOwnerId;
		Container.ContainerType = TEXT("Container.Test.Persistence");
		Container.Slots = { PersistenceItemId };
		Snapshot.Containers.Add(Container);

		FShanmenItemInstance Item;
		Item.ItemInstanceId = PersistenceItemId;
		Item.DefinitionId = Definition.DefinitionId;
		Item.RunId = PersistenceRunId;
		Item.OwnerId = PersistenceOwnerId;
		Item.ParentContainerId = PersistenceContainerId;
		Item.SlotIndex = 0;
		Item.Quantity = Quantity;
		Snapshot.Items.Add(Item);
		return Snapshot;
	}

	FShanmenItemMigrationEvidence PersistenceEvidence()
	{
		FShanmenItemMigrationEvidence Evidence;
		Evidence.MigrationId = PersistenceMigrationId;
		Evidence.OwnerId = PersistenceOwnerId;
		Evidence.SourceProfileSchema = 7;
		Evidence.SourceSaveGeneration = 12;
		Evidence.SourceCodeBPersistentRevision = 1;
		Evidence.SourceCodeBRepositoryRevision = 4;
		Evidence.DefinitionCount = 1;
		Evidence.ContainerCount = 1;
		Evidence.ItemCount = 1;
		Evidence.SourceFingerprint = TEXT("P1.2.Persistence.SourceFixture.v1");
		Evidence.CandidateDigest = TEXT("P1.2.Persistence.CandidateFixture.v1");
		return Evidence;
	}

	FShanmenItemAuthoritySnapshot ReserveOne(
		FAutomationTestBase& Test,
		const FShanmenItemAuthoritySnapshot& Before,
		uint32 Sequence)
	{
		FShanmenItemRepository Repository;
		EShanmenItemTransactionError LoadError =
			EShanmenItemTransactionError::None;
		Test.TestTrue(TEXT("Authority fixture reloads before mutation"),
			Repository.TryLoadSnapshot(Before, &LoadError));

		FShanmenItemReserveRequest Request;
		Request.Context.RunId = PersistenceRunId;
		Request.Context.OwnerId = PersistenceOwnerId;
		Request.Context.RequestId = FGuid(0x50121000 + Sequence, 0, 0, 1);
		Request.Context.Content = PersistenceContent();
		Request.ItemInstanceId = PersistenceItemId;
		Request.ResourceKind = EShanmenItemResourceKind::Quantity;
		Request.Amount = 1;
		Request.ExpectedItemRevision = 0;
		Request.PurposeId = TEXT("Test.Persistence.Reserve");
		Test.TestTrue(TEXT("Real item transaction produces persistence mutation"),
			Repository.Reserve(Request).IsSuccess());
		return Repository.CaptureSnapshot();
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	void RemovePersistenceRoot(const FString& Root)
	{
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemPersistenceRoundTripTest,
	"Shanmen.0_0_10.Items.PersistenceDocument.RoundTripGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemPersistenceRoundTripTest::RunTest(const FString&)
{
	const FString Root = NewPersistenceRoot(TEXT("RoundTrip"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, PersistenceOwnerId);
	const FShanmenItemAuthoritySnapshot Candidate = PersistenceCandidate();
	const FShanmenItemMigrationEvidence Evidence = PersistenceEvidence();
	FShanmenItemAuthorityStore Store;

	const FShanmenItemOpenResult Created =
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Storage);
	TestTrue(TEXT("First migration publishes generation one"),
		Created.Status == EShanmenItemOpenStatus::CreatedFromMigration
		&& Created.Document.SaveGeneration == 1);
	TArray<uint8> CreatedBytes;
	TestTrue(TEXT("Published primary is readable"),
		ReadBytes(Storage.PrimaryPath(), CreatedBytes));

	const FShanmenItemOpenResult Reopened =
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Storage);
	TArray<uint8> ReopenedBytes;
	TestTrue(TEXT("Same migration reopens without a write"),
		Reopened.Status == EShanmenItemOpenStatus::OpenedExisting
		&& !Reopened.bDiskStateChanged
		&& Reopened.Document == Created.Document
		&& ReadBytes(Storage.PrimaryPath(), ReopenedBytes)
		&& ReopenedBytes == CreatedBytes);

	FShanmenItemAuthorityDocument Mutable = Reopened.Document;
	const FShanmenItemAuthoritySnapshot Updated =
		ReserveOne(*this, Mutable.Authority, 1);
	const FShanmenItemSaveResult Saved =
		Store.SaveAuthority(Mutable, Updated, Storage);
	TestTrue(TEXT("Changed authority advances exactly one generation"),
		Saved.IsSuccess() && Saved.bDiskStateChanged
		&& Mutable.SaveGeneration == 2
		&& IFileManager::Get().FileExists(*Storage.BackupPath()));

	const FShanmenItemLoadResult Loaded = Store.LoadExisting(Storage);
	TestTrue(TEXT("Committed generation round-trips exactly"),
		Loaded.Status == EShanmenItemLoadStatus::LoadedPrimary
		&& Loaded.Document == Mutable);
	TArray<uint8> BeforeNoOp;
	ReadBytes(Storage.PrimaryPath(), BeforeNoOp);
	const FShanmenItemSaveResult NoOp =
		Store.SaveAuthority(Mutable, Mutable.Authority, Storage);
	TArray<uint8> AfterNoOp;
	TestTrue(TEXT("Unchanged save is byte-stable and generation-stable"),
		NoOp.IsSuccess() && !NoOp.bDiskStateChanged
		&& Mutable.SaveGeneration == 2
		&& ReadBytes(Storage.PrimaryPath(), AfterNoOp)
		&& AfterNoOp == BeforeNoOp);

	RemovePersistenceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemPersistenceMigrationConflictTest,
	"Shanmen.0_0_10.Items.PersistenceDocument.MigrationConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemPersistenceMigrationConflictTest::RunTest(const FString&)
{
	const FString Root = NewPersistenceRoot(TEXT("MigrationConflict"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, PersistenceOwnerId);
	const FShanmenItemAuthoritySnapshot Candidate = PersistenceCandidate();
	const FShanmenItemMigrationEvidence Evidence = PersistenceEvidence();
	FShanmenItemAuthorityStore Store;
	TestTrue(TEXT("Fixture authority publishes"),
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Storage).IsSuccess());
	TArray<uint8> Before;
	ReadBytes(Storage.PrimaryPath(), Before);

	FShanmenItemMigrationEvidence DifferentEvidence = Evidence;
	DifferentEvidence.MigrationId = FGuid(0x50120006, 0, 0, 1);
	const FShanmenItemOpenResult EvidenceConflict =
		Store.OpenOrCreateFromMigration(Candidate, DifferentEvidence, Storage);
	TArray<uint8> AfterEvidence;
	TestTrue(TEXT("Different migration identity fails closed"),
		EvidenceConflict.Status == EShanmenItemOpenStatus::MigrationConflict
		&& ReadBytes(Storage.PrimaryPath(), AfterEvidence)
		&& AfterEvidence == Before);

	const FShanmenItemOpenResult CandidateConflict =
		Store.OpenOrCreateFromMigration(PersistenceCandidate(4), Evidence, Storage);
	TArray<uint8> AfterCandidate;
	TestTrue(TEXT("Different initial candidate fails closed"),
		CandidateConflict.Status == EShanmenItemOpenStatus::MigrationConflict
		&& ReadBytes(Storage.PrimaryPath(), AfterCandidate)
		&& AfterCandidate == Before);

	RemovePersistenceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemPersistencePreCommitFailureTest,
	"Shanmen.0_0_10.Items.PersistenceDocument.PreCommitFailureIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemPersistencePreCommitFailureTest::RunTest(const FString&)
{
	const TArray<EShanmenItemStoreFailureStage> Stages =
	{
		EShanmenItemStoreFailureStage::CreateDirectory,
		EShanmenItemStoreFailureStage::WriteTemp,
		EShanmenItemStoreFailureStage::FlushOrCloseTemp,
		EShanmenItemStoreFailureStage::ReadBackTemp,
		EShanmenItemStoreFailureStage::ValidateTemp,
		EShanmenItemStoreFailureStage::PrepareBackup,
		EShanmenItemStoreFailureStage::AtomicReplace
	};
	FShanmenItemAuthorityStore Store;
	uint32 Sequence = 10;
	for (const EShanmenItemStoreFailureStage Stage : Stages)
	{
		const FString Root = NewPersistenceRoot(TEXT("PreCommitFailure"));
		const FShanmenItemStorageContext Normal =
			FShanmenItemStorageContext::ForRoot(Root, PersistenceOwnerId);
		const FShanmenItemOpenResult Created = Store.OpenOrCreateFromMigration(
			PersistenceCandidate(), PersistenceEvidence(), Normal);
		TestTrue(TEXT("Failure fixture publishes"), Created.IsSuccess());
		FShanmenItemAuthorityDocument Caller = Created.Document;
		const FShanmenItemAuthorityDocument CallerBefore = Caller;
		const FShanmenItemAuthoritySnapshot Updated =
			ReserveOne(*this, Caller.Authority, Sequence++);
		TArray<uint8> PrimaryBefore;
		ReadBytes(Normal.PrimaryPath(), PrimaryBefore);

		FShanmenItemStorageContext Injected = Normal;
		Injected.InjectedFailure = Stage;
		const FShanmenItemSaveResult Failed =
			Store.SaveAuthority(Caller, Updated, Injected);
		TArray<uint8> PrimaryAfter;
		TestTrue(TEXT("Injected pre-commit stage reports failure"),
			!Failed.IsSuccess());
		TestTrue(TEXT("Injected pre-commit stage preserves caller and primary"),
			Caller == CallerBefore
			&& ReadBytes(Normal.PrimaryPath(), PrimaryAfter)
			&& PrimaryAfter == PrimaryBefore);
		RemovePersistenceRoot(Root);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemPersistenceAmbiguousCommitTest,
	"Shanmen.0_0_10.Items.PersistenceDocument.AmbiguousCommitReopen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemPersistenceAmbiguousCommitTest::RunTest(const FString&)
{
	const FString PublishRoot =
		NewPersistenceRoot(TEXT("AmbiguousFirstPublish"));
	const FShanmenItemStorageContext PublishNormal =
		FShanmenItemStorageContext::ForRoot(
			PublishRoot, PersistenceOwnerId);
	FShanmenItemStorageContext PublishInjected = PublishNormal;
	PublishInjected.InjectedFailure =
		EShanmenItemStoreFailureStage::ReadBackCommittedPrimary;
	const FShanmenItemAuthoritySnapshot Candidate = PersistenceCandidate();
	const FShanmenItemMigrationEvidence Evidence = PersistenceEvidence();
	FShanmenItemAuthorityStore Store;
	const FShanmenItemOpenResult AmbiguousPublish =
		Store.OpenOrCreateFromMigration(
			Candidate, Evidence, PublishInjected);
	TestTrue(TEXT("First-publish ambiguity is reported without claiming success"),
		AmbiguousPublish.Status
			== EShanmenItemOpenStatus::PersistenceFailure);
	const FShanmenItemOpenResult ResolvedPublish =
		Store.OpenOrCreateFromMigration(
			Candidate, Evidence, PublishNormal);
	TestTrue(TEXT("Same MigrationId resolves ambiguous first publish by reopen"),
		ResolvedPublish.Status == EShanmenItemOpenStatus::OpenedExisting
		&& ResolvedPublish.Document.SaveGeneration == 1
		&& ResolvedPublish.Document.Authority == Candidate);
	RemovePersistenceRoot(PublishRoot);

	const FString Root = NewPersistenceRoot(TEXT("AmbiguousCommit"));
	const FShanmenItemStorageContext Normal =
		FShanmenItemStorageContext::ForRoot(Root, PersistenceOwnerId);
	const FShanmenItemOpenResult Created =
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Normal);
	TestTrue(TEXT("Ambiguity fixture publishes"), Created.IsSuccess());
	FShanmenItemAuthorityDocument Caller = Created.Document;
	const FShanmenItemAuthorityDocument CallerBefore = Caller;
	const FShanmenItemAuthoritySnapshot Updated =
		ReserveOne(*this, Caller.Authority, 30);

	FShanmenItemStorageContext Injected = Normal;
	Injected.InjectedFailure =
		EShanmenItemStoreFailureStage::ReadBackCommittedPrimary;
	const FShanmenItemSaveResult Ambiguous =
		Store.SaveAuthority(Caller, Updated, Injected);
	TestTrue(TEXT("Post-commit verification failure leaves caller unresolved"),
		Ambiguous.Status
			== EShanmenItemSaveStatus::PostCommitVerificationFailed
		&& Caller == CallerBefore);

	const FShanmenItemOpenResult Reopened =
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Normal);
	TestTrue(TEXT("Reopen resolves intended commit without legacy replay"),
		Reopened.Status == EShanmenItemOpenStatus::OpenedExisting
		&& Reopened.Document.SaveGeneration == 2
		&& Reopened.Document.Authority == Updated);

	FShanmenItemAuthorityDocument CleanupCaller = Reopened.Document;
	const FShanmenItemAuthoritySnapshot SecondUpdate =
		ReserveOne(*this, CleanupCaller.Authority, 31);
	Injected.InjectedFailure = EShanmenItemStoreFailureStage::CleanupTemp;
	const FShanmenItemSaveResult CleanupWarning =
		Store.SaveAuthority(CleanupCaller, SecondUpdate, Injected);
	TestTrue(TEXT("Post-commit cleanup warning does not roll back commit"),
		CleanupWarning.IsSuccess() && !CleanupWarning.bCleanupSucceeded
		&& CleanupCaller.SaveGeneration == 3);

	RemovePersistenceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemPersistenceRecoveryTest,
	"Shanmen.0_0_10.Items.PersistenceDocument.BackupRecoveryAndFutureSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemPersistenceRecoveryTest::RunTest(const FString&)
{
	const FString Root = NewPersistenceRoot(TEXT("Recovery"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, PersistenceOwnerId);
	FShanmenItemAuthorityStore Store;
	const FShanmenItemOpenResult Created = Store.OpenOrCreateFromMigration(
		PersistenceCandidate(), PersistenceEvidence(), Storage);
	FShanmenItemAuthorityDocument Mutable = Created.Document;
	TestTrue(TEXT("Recovery fixture publishes"), Created.IsSuccess());
	TestTrue(TEXT("Second generation creates verified backup"),
		Store.SaveAuthority(
			Mutable, ReserveOne(*this, Mutable.Authority, 40), Storage).IsSuccess()
		&& IFileManager::Get().FileExists(*Storage.BackupPath()));

	const TArray<uint8> CorruptBytes = { 0x7b, 0x62, 0x61, 0x64, 0x7d };
	TestTrue(TEXT("Primary corruption fixture writes"),
		FFileHelper::SaveArrayToFile(CorruptBytes, *Storage.PrimaryPath()));
	const FShanmenItemLoadResult Recovered = Store.LoadExisting(Storage);
	TestTrue(TEXT("Valid backup restores corrupt primary and preserves evidence"),
		Recovered.Status == EShanmenItemLoadStatus::RecoveredFromBackup
		&& Recovered.Document.SaveGeneration == 1
		&& IFileManager::Get().FileExists(*Recovered.QuarantinedPath));
	TArray<uint8> QuarantinedBytes;
	TestTrue(TEXT("Corrupt source bytes are retained exactly"),
		ReadBytes(Recovered.QuarantinedPath, QuarantinedBytes)
		&& QuarantinedBytes == CorruptBytes);

	FString FutureJson;
	TestTrue(TEXT("Recovered primary JSON is readable"),
		FFileHelper::LoadFileToString(FutureJson, *Storage.PrimaryPath()));
	TestEqual(TEXT("One current schema token becomes future schema"),
		FutureJson.ReplaceInline(
			TEXT("\"SchemaVersion\":1"), TEXT("\"SchemaVersion\":2"),
			ESearchCase::CaseSensitive), 1);
	TestTrue(TEXT("Future schema fixture replaces primary"),
		FFileHelper::SaveStringToFile(
			FutureJson, *Storage.PrimaryPath(),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	TArray<uint8> FutureBefore;
	ReadBytes(Storage.PrimaryPath(), FutureBefore);
	const FShanmenItemLoadResult Future = Store.LoadExisting(Storage);
	TArray<uint8> FutureAfter;
	TestTrue(TEXT("Future schema fails closed without backup downgrade"),
		Future.Status == EShanmenItemLoadStatus::FutureSchemaRejected
		&& ReadBytes(Storage.PrimaryPath(), FutureAfter)
		&& FutureAfter == FutureBefore);

	TestTrue(TEXT("Future primary can be removed for missing-primary recovery"),
		IFileManager::Get().Delete(*Storage.PrimaryPath(), false, true, true));
	const FShanmenItemLoadResult MissingRecovered = Store.LoadExisting(Storage);
	TestTrue(TEXT("Missing primary restores the verified backup"),
		MissingRecovered.Status
			== EShanmenItemLoadStatus::PrimaryMissingBackupRecovered
		&& MissingRecovered.Document.SaveGeneration == 1);

	RemovePersistenceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemPersistenceNoSilentResetTest,
	"Shanmen.0_0_10.Items.PersistenceDocument.NoSilentReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemPersistenceNoSilentResetTest::RunTest(const FString&)
{
	const FString Root = NewPersistenceRoot(TEXT("NoSilentReset"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, PersistenceOwnerId);
	const FShanmenItemAuthoritySnapshot Candidate = PersistenceCandidate();
	const FShanmenItemMigrationEvidence Evidence = PersistenceEvidence();
	FShanmenItemAuthorityStore Store;
	const FShanmenItemOpenResult Created =
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Storage);
	TestTrue(TEXT("No-reset fixture publishes without a backup"),
		Created.IsSuccess()
		&& !IFileManager::Get().FileExists(*Storage.BackupPath()));
	const TArray<uint8> CorruptBytes = { 0x00, 0x10, 0x20, 0x30 };
	TestTrue(TEXT("No-reset corruption fixture writes"),
		FFileHelper::SaveArrayToFile(CorruptBytes, *Storage.PrimaryPath()));
	FShanmenItemAuthorityDocument Caller = Created.Document;
	const FShanmenItemSaveResult NoOpAgainstCorrupt =
		Store.SaveAuthority(Caller, Caller.Authority, Storage);
	TArray<uint8> AfterNoOp;
	TestTrue(TEXT("No-op save cannot mask a divergent durable primary"),
		!NoOpAgainstCorrupt.IsSuccess()
		&& Caller == Created.Document
		&& ReadBytes(Storage.PrimaryPath(), AfterNoOp)
		&& AfterNoOp == CorruptBytes);
	const FShanmenItemOpenResult Failed =
		Store.OpenOrCreateFromMigration(Candidate, Evidence, Storage);
	TArray<uint8> After;
	TestTrue(TEXT("Corrupt authority never silently resets from legacy"),
		Failed.Status == EShanmenItemOpenStatus::PersistenceFailure
		&& ReadBytes(Storage.PrimaryPath(), After)
		&& After == CorruptBytes);

	RemovePersistenceRoot(Root);
	return true;
}

#endif
