#if WITH_DEV_AUTOMATION_TESTS

#include "ShanmenItemAuthorityService.h"

#include "ShanmenItemTags.h"

#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const FGuid ServiceOwnerId(0x51300001, 0, 0, 1);
	const FGuid ServiceRunId(0x51300002, 0, 0, 1);
	const FGuid ServiceContainerId(0x51300003, 0, 0, 1);
	const FGuid ServiceItemId(0x51300004, 0, 0, 1);
	const FGuid ServiceMigrationId(0x51300005, 0, 0, 1);
	const FGuid ServiceDeployItemId(0x51300006, 0, 0, 1);

	FString NewServiceRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.3.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp ServiceContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.Items.0.0.10.P1.3");
		Content.Digest = TEXT("P1.3.SerializedAuthorityLifecycle.v1");
		return Content;
	}

	FShanmenItemAuthoritySnapshot ServiceCandidate(int32 Quantity = 16)
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Snapshot.Content = ServiceContent();

		FShanmenItemDefinition Definition;
		Definition.DefinitionId = TEXT("Item.Test.AuthorityServiceStack");
		Definition.MaxStack = 32;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		Snapshot.Definitions.Add(Definition);

		FShanmenItemDefinition DeployDefinition;
		DeployDefinition.DefinitionId = TEXT("Item.Test.AuthorityServiceDeployable");
		DeployDefinition.MaxStack = 1;
		DeployDefinition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityDeploy());
		DeployDefinition.MaxDurability = 5;
		DeployDefinition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityDurability());
		Snapshot.Definitions.Add(DeployDefinition);

		FShanmenItemContainer Container;
		Container.ContainerId = ServiceContainerId;
		Container.RunId = ServiceRunId;
		Container.OwnerId = ServiceOwnerId;
		Container.ContainerType = TEXT("Container.Test.AuthorityService");
		Container.Slots = { ServiceItemId, ServiceDeployItemId };
		Snapshot.Containers.Add(Container);

		FShanmenItemInstance Item;
		Item.ItemInstanceId = ServiceItemId;
		Item.DefinitionId = Definition.DefinitionId;
		Item.RunId = ServiceRunId;
		Item.OwnerId = ServiceOwnerId;
		Item.ParentContainerId = ServiceContainerId;
		Item.SlotIndex = 0;
		Item.Quantity = Quantity;
		Snapshot.Items.Add(Item);

		FShanmenItemInstance DeployItem;
		DeployItem.ItemInstanceId = ServiceDeployItemId;
		DeployItem.DefinitionId = DeployDefinition.DefinitionId;
		DeployItem.RunId = ServiceRunId;
		DeployItem.OwnerId = ServiceOwnerId;
		DeployItem.ParentContainerId = ServiceContainerId;
		DeployItem.SlotIndex = 1;
		DeployItem.Quantity = 1;
		DeployItem.Durability = 5;
		Snapshot.Items.Add(DeployItem);
		return Snapshot;
	}

	FShanmenItemMigrationEvidence ServiceEvidence()
	{
		FShanmenItemMigrationEvidence Evidence;
		Evidence.MigrationId = ServiceMigrationId;
		Evidence.OwnerId = ServiceOwnerId;
		Evidence.SourceProfileSchema = 7;
		Evidence.SourceSaveGeneration = 13;
		Evidence.SourceCodeBPersistentRevision = 2;
		Evidence.SourceCodeBRepositoryRevision = 5;
		Evidence.DefinitionCount = 2;
		Evidence.ContainerCount = 1;
		Evidence.ItemCount = 2;
		Evidence.SourceFingerprint = TEXT("P1.3.AuthorityService.SourceFixture.v1");
		Evidence.CandidateDigest = TEXT("P1.3.AuthorityService.CandidateFixture.v1");
		return Evidence;
	}

	FShanmenItemMigrationAuthorization ServiceAuthorization()
	{
		return FShanmenItemMigrationAuthorization::Explicit(
			ServiceMigrationId);
	}

	FShanmenItemReserveRequest ServiceReserve(
		uint32 Sequence,
		int32 Amount = 1,
		int32 ExpectedItemRevision = 0,
		EShanmenItemResourceKind Kind = EShanmenItemResourceKind::Quantity)
	{
		FShanmenItemReserveRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId = FGuid(0x51310000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ItemInstanceId = Kind == EShanmenItemResourceKind::Quantity
			? ServiceItemId : ServiceDeployItemId;
		Request.ResourceKind = Kind;
		Request.Amount = Amount;
		Request.ExpectedItemRevision = ExpectedItemRevision;
		Request.PurposeId = TEXT("Test.AuthorityService.Command");
		return Request;
	}

	FShanmenItemRunResourceCommitRequest ServiceRunResourceCommit(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& ReservationId)
	{
		FShanmenItemRunResourceCommitRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x51350000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ActiveRunId = ActiveRunId;
		FShanmenItemRunResourceCommitLine& Line =
			Request.OrderedLines.AddDefaulted_GetRef();
		Line.ReservationId = ReservationId;
		Line.ItemInstanceId = ServiceDeployItemId;
		Request.PurposeId = TEXT("Test.AuthorityService.DefenseResource");
		return Request;
	}

	FShanmenItemRunResourceIntentRequest ServiceRunResourceIntent(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& IntentId,
		const FGuid& ReservationId)
	{
		FShanmenItemRunResourceIntentRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x51360000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ActiveRunId = ActiveRunId;
		Request.IntentId = IntentId;
		FShanmenItemRunResourceCommitLine& Line =
			Request.OrderedLines.AddDefaulted_GetRef();
		Line.ReservationId = ReservationId;
		Line.ItemInstanceId = ServiceDeployItemId;
		Request.TriggeredLineCount = 1;
		Request.IntentMetadata = TEXT("Test.AuthorityService.VitalityCAS.r1");
		return Request;
	}

	FShanmenItemRunResourceIntentFinalizeRequest
	ServiceRunResourceIntentFinalize(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& PrepareRequestId,
		const FGuid& IntentId,
		bool bExternalCommitSucceeded)
	{
		FShanmenItemRunResourceIntentFinalizeRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x51370000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ActiveRunId = ActiveRunId;
		Request.PrepareRequestId = PrepareRequestId;
		Request.IntentId = IntentId;
		Request.bExternalCommitSucceeded = bExternalCommitSucceeded;
		return Request;
	}

	FShanmenItemRunQuantityIntentRequest ServiceRunQuantityIntent(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& IntentId,
		int32 Amount,
		int32 ExpectedQuantityBefore)
	{
		FShanmenItemRunQuantityIntentRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x51390000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ActiveRunId = ActiveRunId;
		Request.IntentId = IntentId;
		Request.ItemInstanceId = ServiceItemId;
		Request.Amount = Amount;
		Request.ExpectedQuantityBefore = ExpectedQuantityBefore;
		Request.PurposeId = TEXT("Test.AuthorityService.ThrownLaunch.r1");
		return Request;
	}

	FShanmenItemRunQuantityIntentFinalizeRequest
	ServiceRunQuantityIntentFinalize(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& PrepareRequestId,
		const FGuid& IntentId,
		bool bCommit)
	{
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x513A0000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ActiveRunId = ActiveRunId;
		Request.PrepareRequestId = PrepareRequestId;
		Request.IntentId = IntentId;
		Request.ItemInstanceId = ServiceItemId;
		Request.bCommit = bCommit;
		return Request;
	}

	FShanmenItemReservationActionRequest ServiceAction(
		uint32 Sequence, const FGuid& ReservationId)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId = FGuid(0x51320000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ReservationId = ReservationId;
		return Request;
	}

	FShanmenItemReservationBatchRequest ServiceBatch(
		uint32 Sequence, const TArray<FGuid>& ReservationIds)
	{
		FShanmenItemReservationBatchRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x51330000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ReservationIds = ReservationIds;
		return Request;
	}

	FShanmenItemRunStartRequest ServiceRunStart(
		uint32 Sequence, const TArray<FGuid>& ReservationIds)
	{
		FShanmenItemRunStartRequest Request;
		Request.Context.RunId = ServiceRunId;
		Request.Context.OwnerId = ServiceOwnerId;
		Request.Context.RequestId =
			FGuid(0x51340000 + Sequence, 0, 0, 1);
		Request.Context.Content = ServiceContent();
		Request.ReservationIds = ReservationIds;
		return Request;
	}

	FShanmenItemAuthorityStartResult CreateService(
		FShanmenItemAuthorityService& Service,
		const FShanmenItemStorageContext& Storage,
		int32 Quantity = 16)
	{
		return Service.StartFromAuthorizedMigration(
			Storage, ServiceAuthorization(), ServiceCandidate(Quantity),
			ServiceEvidence());
	}

	bool ReadServiceBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	void RemoveServiceRoot(const FString& Root)
	{
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceStartupTest,
	"Shanmen.0_0_10.Items.AuthorityService.ExistingFirstAndExplicitMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceStartupTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("Startup"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;

	const FShanmenItemAuthorityStartResult Missing =
		Service.StartExisting(Storage);
	TestTrue(TEXT("Missing durable state requires migration without writing"),
		Missing.Status == EShanmenItemAuthorityStartStatus::MigrationRequired
		&& Service.GetState() == EShanmenItemAuthorityServiceState::Closed
		&& !IFileManager::Get().FileExists(*Storage.PrimaryPath()));

	const FShanmenItemMigrationAuthorization WrongAuthorization =
		FShanmenItemMigrationAuthorization::Explicit(
			FGuid(0x51300006, 0, 0, 1));
	const FShanmenItemAuthorityStartResult Unauthorized =
		Service.StartFromAuthorizedMigration(
			Storage, WrongAuthorization, ServiceCandidate(), ServiceEvidence());
	TestTrue(TEXT("Mismatched migration capability cannot publish"),
		Unauthorized.Status
			== EShanmenItemAuthorityStartStatus::MigrationNotAuthorized
		&& Service.GetState() == EShanmenItemAuthorityServiceState::Closed
		&& !IFileManager::Get().FileExists(*Storage.PrimaryPath()));

	const FShanmenItemAuthorityStartResult Created =
		CreateService(Service, Storage);
	FShanmenItemAuthorityDocument CreatedDocument;
	TestTrue(TEXT("Exact capability publishes and installs generation one"),
		Created.Status
			== EShanmenItemAuthorityStartStatus::CreatedFromMigration
		&& Created.DocumentGeneration == 1
		&& Service.TryGetDocument(CreatedDocument));

	FShanmenItemAuthorityService ExistingFirst;
	const FShanmenItemMigrationAuthorization EmptyAuthorization;
	const FShanmenItemAuthorityStartResult Reopened =
		ExistingFirst.StartFromAuthorizedMigration(
			Storage, EmptyAuthorization,
			FShanmenItemAuthoritySnapshot(),
			FShanmenItemMigrationEvidence());
	FShanmenItemAuthorityDocument ReopenedDocument;
	TestTrue(TEXT("Existing document wins before malformed legacy inputs are read"),
		Reopened.Status == EShanmenItemAuthorityStartStatus::OpenedExisting
		&& ExistingFirst.TryGetDocument(ReopenedDocument)
		&& ReopenedDocument == CreatedDocument);
	TestTrue(TEXT("A ready service refuses storage rebinding"),
		ExistingFirst.StartExisting(Storage).Status
			== EShanmenItemAuthorityStartStatus::AlreadyReady);

	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceDurableReplayTest,
	"Shanmen.0_0_10.Items.AuthorityService.DurableCommandReplayAndRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceDurableReplayTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("DurableReplay"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Replay fixture publishes"),
		CreateService(Service, Storage).IsReady());

	const FShanmenItemReserveRequest ReserveRequest = ServiceReserve(1, 2);
	const FShanmenItemDurableCommandResult Reserved =
		Service.ReserveDurable(ReserveRequest);
	TestTrue(TEXT("Reserve is reported only after generation two is durable"),
		Reserved.Status == EShanmenItemDurableCommandStatus::Persisted
		&& Reserved.IsCommandSuccess()
		&& Reserved.DocumentGeneration == 2);
	TArray<uint8> BeforeReplay;
	ReadServiceBytes(Storage.PrimaryPath(), BeforeReplay);
	const FShanmenItemDurableCommandResult ReserveReplay =
		Service.ReserveDurable(ReserveRequest);
	TArray<uint8> AfterReplay;
	TestTrue(TEXT("Exact reserve replay is receipt- and byte-stable"),
		ReserveReplay.Status == EShanmenItemDurableCommandStatus::Replayed
		&& ReserveReplay.Receipt == Reserved.Receipt
		&& ReserveReplay.DocumentGeneration == 2
		&& !ReserveReplay.bDiskStateChanged
		&& ReadServiceBytes(Storage.PrimaryPath(), AfterReplay)
		&& AfterReplay == BeforeReplay);

	const FShanmenItemReservationActionRequest CommitRequest =
		ServiceAction(2, Reserved.Receipt.ReservationId);
	const FShanmenItemDurableCommandResult Committed =
		Service.CommitDurable(CommitRequest);
	TestTrue(TEXT("Commit is durably advanced to generation three"),
		Committed.Status == EShanmenItemDurableCommandStatus::Persisted
		&& Committed.IsCommandSuccess()
		&& Committed.DocumentGeneration == 3);

	const FShanmenItemDurableCommandResult Cancelled =
		Service.CancelDurable(ServiceAction(
			4,
			Service.ReserveDurable(ServiceReserve(3, 1, 1)).Receipt.ReservationId));
	TestTrue(TEXT("Cancel wrapper persists its terminal reservation state"),
		Cancelled.Status == EShanmenItemDurableCommandStatus::Persisted
		&& Cancelled.IsCommandSuccess());

	const FShanmenItemDurableCommandResult DeploymentReserved =
		Service.ReserveDurable(ServiceReserve(
			5, 1, 0, EShanmenItemResourceKind::DeploymentLock));
	const FShanmenItemDurableCommandResult DeploymentCommitted =
		Service.CommitDurable(ServiceAction(
			6, DeploymentReserved.Receipt.ReservationId));
	const FShanmenItemDurableCommandResult DeploymentReleased =
		Service.ReleaseDeploymentDurable(ServiceAction(
			7, DeploymentReserved.Receipt.ReservationId));
	TestTrue(TEXT("Deployment commit and release wrappers are both durable"),
		DeploymentReserved.IsCommandSuccess()
		&& DeploymentCommitted.IsCommandSuccess()
		&& DeploymentReleased.IsCommandSuccess());

	FShanmenItemAuthoritySnapshot BeforeRestart;
	TestTrue(TEXT("Final durable snapshot is readable"),
		Service.TryCaptureSnapshot(BeforeRestart));
	FShanmenItemAuthorityService Restarted;
	TestTrue(TEXT("Restart opens the latest generation"),
		Restarted.StartExisting(Storage).Status
			== EShanmenItemAuthorityStartStatus::OpenedExisting);
	FShanmenItemAuthoritySnapshot AfterRestart;
	TestTrue(TEXT("Restart restores the exact graph and request ledger"),
		Restarted.TryCaptureSnapshot(AfterRestart)
		&& AfterRestart == BeforeRestart);
	TestTrue(TEXT("Reserve and commit replay across restart without writes"),
		Restarted.ReserveDurable(ReserveRequest).Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& Restarted.CommitDurable(CommitRequest).Status
			== EShanmenItemDurableCommandStatus::Replayed);

	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceRejectedDurabilityTest,
	"Shanmen.0_0_10.Items.AuthorityService.RejectedCommandDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceRejectedDurabilityTest::RunTest(
	const FString&)
{
	const FString Root = NewServiceRoot(TEXT("RejectedDurability"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Rejected fixture publishes"),
		CreateService(Service, Storage, 3).IsReady());

	const FShanmenItemReserveRequest TooLarge = ServiceReserve(20, 9);
	const FShanmenItemDurableCommandResult Rejected =
		Service.ReserveDurable(TooLarge);
	TestTrue(TEXT("Ledger-recorded rejection is persisted before return"),
		Rejected.Status
			== EShanmenItemDurableCommandStatus::RejectedAndPersisted
		&& Rejected.IsDurable()
		&& !Rejected.IsCommandSuccess()
		&& Rejected.Receipt.Error
			== EShanmenItemTransactionError::InsufficientResource
		&& Rejected.DocumentGeneration == 2);

	TArray<uint8> BeforeReplay;
	ReadServiceBytes(Storage.PrimaryPath(), BeforeReplay);
	FShanmenItemAuthorityService Restarted;
	TestTrue(TEXT("Rejected ledger restarts"),
		Restarted.StartExisting(Storage).IsReady());
	const FShanmenItemDurableCommandResult Replayed =
		Restarted.ReserveDurable(TooLarge);
	FShanmenItemReserveRequest Conflict = TooLarge;
	Conflict.Amount = 8;
	const FShanmenItemDurableCommandResult ConflictResult =
		Restarted.ReserveDurable(Conflict);
	TArray<uint8> AfterConflict;
	TestTrue(TEXT("Rejected replay and RequestId conflict do not rewrite authority"),
		Replayed.Status
			== EShanmenItemDurableCommandStatus::RejectedWithoutMutation
		&& Replayed.Receipt == Rejected.Receipt
		&& ConflictResult.Status
			== EShanmenItemDurableCommandStatus::RejectedWithoutMutation
		&& ConflictResult.Receipt.Error
			== EShanmenItemTransactionError::RequestIdConflict
		&& ReadServiceBytes(Storage.PrimaryPath(), AfterConflict)
		&& AfterConflict == BeforeReplay);
	FShanmenItemReserveRequest Invalid = ServiceReserve(21);
	Invalid.Context.RequestId.Invalidate();
	const FShanmenItemDurableCommandResult InvalidResult =
		Restarted.ReserveDurable(Invalid);
	TestTrue(TEXT("An invalid non-ledger receipt is safe but not called durable"),
		InvalidResult.Status
			== EShanmenItemDurableCommandStatus::RejectedWithoutMutation
		&& !InvalidResult.Receipt.IsValid()
		&& !InvalidResult.IsDurable());

	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceRollbackTest,
	"Shanmen.0_0_10.Items.AuthorityService.PersistenceFailureRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceRollbackTest::RunTest(const FString&)
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
	uint32 Sequence = 30;
	for (const EShanmenItemStoreFailureStage Stage : Stages)
	{
		const FString Root = NewServiceRoot(TEXT("Rollback"));
		const FShanmenItemStorageContext Storage =
			FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
		FShanmenItemAuthorityService Service;
		TestTrue(TEXT("Rollback fixture publishes"),
			CreateService(Service, Storage).IsReady());
		FShanmenItemAuthoritySnapshot BeforeSnapshot;
		Service.TryCaptureSnapshot(BeforeSnapshot);
		TArray<uint8> BeforeBytes;
		ReadServiceBytes(Storage.PrimaryPath(), BeforeBytes);
		const FShanmenItemReserveRequest Request =
			ServiceReserve(Sequence++);

		Service.SetInjectedFailureForTests(Stage);
		const FShanmenItemDurableCommandResult Failed =
			Service.ReserveDurable(Request);
		FShanmenItemAuthoritySnapshot AfterFailure;
		TArray<uint8> AfterFailureBytes;
		TestTrue(TEXT("Pre-commit failure restores exact memory and disk"),
			Failed.Status
				== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& !Failed.IsDurable()
			&& Service.GetState()
				== EShanmenItemAuthorityServiceState::Ready
			&& Service.TryCaptureSnapshot(AfterFailure)
			&& AfterFailure == BeforeSnapshot
			&& ReadServiceBytes(Storage.PrimaryPath(), AfterFailureBytes)
			&& AfterFailureBytes == BeforeBytes);

		Service.SetInjectedFailureForTests(
			EShanmenItemStoreFailureStage::None);
		const FShanmenItemDurableCommandResult Retried =
			Service.ReserveDurable(Request);
		TestTrue(TEXT("Same request can be retried exactly once after rollback"),
			Retried.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Retried.IsCommandSuccess()
			&& Retried.DocumentGeneration == 2);
		RemoveServiceRoot(Root);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceAmbiguityTest,
	"Shanmen.0_0_10.Items.AuthorityService.PostCommitAmbiguityReconciled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceAmbiguityTest::RunTest(const FString&)
{
	const FString PublishRoot = NewServiceRoot(TEXT("FirstPublishAmbiguity"));
	FShanmenItemStorageContext PublishStorage =
		FShanmenItemStorageContext::ForRoot(PublishRoot, ServiceOwnerId);
	PublishStorage.InjectedFailure =
		EShanmenItemStoreFailureStage::ReadBackCommittedPrimary;
	FShanmenItemAuthorityService PublishService;
	const FShanmenItemAuthorityStartResult AmbiguousPublish =
		CreateService(PublishService, PublishStorage);
	TestTrue(TEXT("First-publish ambiguity is resolved by existing-first reopen"),
		AmbiguousPublish.Status
			== EShanmenItemAuthorityStartStatus::OpenedExisting
		&& AmbiguousPublish.DocumentGeneration == 1
		&& PublishService.GetState()
			== EShanmenItemAuthorityServiceState::Ready);
	RemoveServiceRoot(PublishRoot);

	const FString Root = NewServiceRoot(TEXT("Ambiguity"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Ambiguity fixture publishes"),
		CreateService(Service, Storage).IsReady());
	const FShanmenItemReserveRequest Request = ServiceReserve(50);

	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::ReadBackCommittedPrimary);
	const FShanmenItemDurableCommandResult Ambiguous =
		Service.ReserveDurable(Request);
	TestTrue(TEXT("Reopen proves a post-commit result before reporting success"),
		Ambiguous.Status
			== EShanmenItemDurableCommandStatus::ResolvedAfterReopen
		&& Ambiguous.SaveStatus
			== EShanmenItemSaveStatus::PostCommitVerificationFailed
		&& Ambiguous.IsCommandSuccess()
		&& Ambiguous.DocumentGeneration == 2
		&& Service.GetState() == EShanmenItemAuthorityServiceState::Ready);

	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Resolved command replays without another generation"),
		Service.ReserveDurable(Request).Status
			== EShanmenItemDurableCommandStatus::Replayed);
	FShanmenItemAuthorityService Restarted;
	FShanmenItemAuthoritySnapshot Current;
	FShanmenItemAuthoritySnapshot Reopened;
	TestTrue(TEXT("Resolved generation survives a fresh lifecycle"),
		Service.TryCaptureSnapshot(Current)
		&& Restarted.StartExisting(Storage).IsReady()
		&& Restarted.TryCaptureSnapshot(Reopened)
		&& Reopened == Current);

	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceDivergenceTest,
	"Shanmen.0_0_10.Items.AuthorityService.ExternalWriterDivergenceFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceDivergenceTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("ExternalDivergence"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService First;
	FShanmenItemAuthorityService Stale;
	TestTrue(TEXT("First lifecycle publishes shared fixture"),
		CreateService(First, Storage).IsReady());
	TestTrue(TEXT("Second lifecycle opens the same initial document"),
		Stale.StartExisting(Storage).IsReady());
	TestTrue(TEXT("First lifecycle advances durable authority"),
		First.ReserveDurable(ServiceReserve(70)).IsCommandSuccess());
	FShanmenItemAuthoritySnapshot DurableFirst;
	First.TryCaptureSnapshot(DurableFirst);

	const FShanmenItemDurableCommandResult Diverged =
		Stale.ReserveDurable(ServiceReserve(71));
	TestTrue(TEXT("Stale writer cannot overwrite a newer durable generation"),
		Diverged.Status == EShanmenItemDurableCommandStatus::RecoveryRequired
		&& Stale.GetState()
			== EShanmenItemAuthorityServiceState::RecoveryRequired);
	TestTrue(TEXT("Faulted stale writer rejects all subsequent commands"),
		Stale.ReserveDurable(ServiceReserve(72)).Status
			== EShanmenItemDurableCommandStatus::RecoveryRequired);

	FShanmenItemAuthorityService Restarted;
	FShanmenItemAuthoritySnapshot DurableReopened;
	TestTrue(TEXT("External-writer conflict preserves the winning document"),
		Restarted.StartExisting(Storage).IsReady()
		&& Restarted.TryCaptureSnapshot(DurableReopened)
		&& DurableReopened == DurableFirst);

	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceNoFallbackTest,
	"Shanmen.0_0_10.Items.AuthorityService.CorruptAuthorityNeverRemigrates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceNoFallbackTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("NoFallback"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Initial;
	TestTrue(TEXT("No-fallback fixture publishes without backup"),
		CreateService(Initial, Storage).IsReady()
		&& !IFileManager::Get().FileExists(*Storage.BackupPath()));
	const TArray<uint8> CorruptBytes = { 0x7b, 0x6e, 0x6f, 0x70, 0x65, 0x7d };
	TestTrue(TEXT("Corrupt authority fixture writes"),
		FFileHelper::SaveArrayToFile(CorruptBytes, *Storage.PrimaryPath()));

	FShanmenItemAuthorityService Restarted;
	const FShanmenItemAuthorityStartResult Failed =
		Restarted.StartFromAuthorizedMigration(
			Storage, ServiceAuthorization(), ServiceCandidate(), ServiceEvidence());
	TArray<uint8> After;
	TestTrue(TEXT("Corrupt durable authority blocks even an authorized migration"),
		Failed.Status == EShanmenItemAuthorityStartStatus::PersistenceFailure
		&& Restarted.GetState()
			== EShanmenItemAuthorityServiceState::RecoveryRequired
		&& ReadServiceBytes(Storage.PrimaryPath(), After)
		&& After == CorruptBytes);
	TestTrue(TEXT("Faulted lifecycle rejects commands without touching disk"),
		Restarted.ReserveDurable(ServiceReserve(60)).Status
			== EShanmenItemDurableCommandStatus::RecoveryRequired);

	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceAtomicBatchTest,
	"Shanmen.0_0_10.Items.AuthorityService.AtomicBatchDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceAtomicBatchTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("AtomicBatch"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Atomic batch fixture publishes"),
		CreateService(Service, Storage, 12).IsReady());
	const FShanmenItemDurableCommandResult Quantity =
		Service.ReserveDurable(ServiceReserve(80, 4));
	const FShanmenItemDurableCommandResult Equipment =
		Service.ReserveDurable(ServiceReserve(
			81, 1, 0, EShanmenItemResourceKind::DeploymentLock));
	TestTrue(TEXT("Atomic batch fixture reserves durably"),
		Quantity.IsCommandSuccess() && Equipment.IsCommandSuccess());
	const FShanmenItemReservationBatchRequest Request = ServiceBatch(
		82, { Equipment.Receipt.ReservationId,
			Quantity.Receipt.ReservationId });

	FShanmenItemAuthoritySnapshot Before;
	FShanmenItemAuthorityDocument DocumentBefore;
	TArray<uint8> BytesBefore;
	TestTrue(TEXT("Pre-batch durable state is readable"),
		Service.TryCaptureSnapshot(Before)
		&& Service.TryGetDocument(DocumentBefore)
		&& ReadServiceBytes(Storage.PrimaryPath(), BytesBefore));
#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult Failed =
		Service.CommitBatchDurable(Request);
	FShanmenItemAuthoritySnapshot AfterFailure;
	FShanmenItemAuthorityDocument DocumentAfterFailure;
	TArray<uint8> BytesAfterFailure;
	TestTrue(TEXT("Failed persistence rolls back the whole batch"),
		Failed.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
		&& Service.TryCaptureSnapshot(AfterFailure)
		&& AfterFailure == Before
		&& Service.TryGetDocument(DocumentAfterFailure)
		&& DocumentAfterFailure == DocumentBefore
		&& ReadServiceBytes(Storage.PrimaryPath(), BytesAfterFailure)
		&& BytesAfterFailure == BytesBefore);

#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Committed =
		Service.CommitBatchDurable(Request);
	FShanmenItemAuthoritySnapshot AfterCommit;
	FShanmenItemAuthorityDocument DocumentAfterCommit;
	TestTrue(TEXT("Retry durably publishes every batch line once"),
		Committed.Status == EShanmenItemDurableCommandStatus::Persisted
		&& Committed.IsCommandSuccess()
		&& Service.TryCaptureSnapshot(AfterCommit)
		&& Service.TryGetDocument(DocumentAfterCommit)
		&& AfterCommit.AuthorityRevision == Before.AuthorityRevision + 1
		&& DocumentAfterCommit.SaveGeneration
			== DocumentBefore.SaveGeneration + 1);
	const FShanmenItemInstance* Stack = AfterCommit.Items.FindByPredicate(
		[](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ServiceItemId;
		});
	const FShanmenItemInstance* Deploy = AfterCommit.Items.FindByPredicate(
		[](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ServiceDeployItemId;
		});
	TestTrue(TEXT("Durable aggregate applies quantity and deployment"),
		Stack && Stack->Quantity == 8
		&& Deploy
		&& Deploy->State == EShanmenItemInstanceState::Deployed);

	FShanmenItemAuthorityService Restarted;
	const FShanmenItemDurableCommandResult Replay =
		Restarted.StartExisting(Storage).IsReady()
			? Restarted.CommitBatchDurable(Request)
			: FShanmenItemDurableCommandResult();
	TestTrue(TEXT("Restart replays the exact durable batch receipt"),
		Replay.Status == EShanmenItemDurableCommandStatus::Replayed
		&& Replay.Receipt == Committed.Receipt);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceAtomicRunStartTest,
	"Shanmen.0_0_10.Items.AuthorityService.AtomicPreparedRunStartDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceAtomicRunStartTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("AtomicRunStart"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Atomic Run-start fixture publishes"),
		CreateService(Service, Storage, 12).IsReady());
	const FShanmenItemDurableCommandResult Quantity =
		Service.ReserveDurable(ServiceReserve(83, 4));
	const FShanmenItemDurableCommandResult Equipment =
		Service.ReserveDurable(ServiceReserve(
			84, 1, 0, EShanmenItemResourceKind::DeploymentLock));
	const FShanmenItemRunStartRequest Request = ServiceRunStart(
		85, { Equipment.Receipt.ReservationId,
			Quantity.Receipt.ReservationId });
	TestTrue(TEXT("Atomic Run-start fixture reserves durably"),
		Quantity.IsCommandSuccess() && Equipment.IsCommandSuccess()
		&& Request.IsValid());

	FShanmenItemAuthoritySnapshot Before;
	FShanmenItemAuthorityDocument DocumentBefore;
	TArray<uint8> BytesBefore;
	TestTrue(TEXT("Pre-start durable state is readable"),
		Service.TryCaptureSnapshot(Before)
		&& Service.TryGetDocument(DocumentBefore)
		&& ReadServiceBytes(Storage.PrimaryPath(), BytesBefore));
#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult Failed =
		Service.StartPreparedRunDurable(Request);
	FShanmenItemAuthoritySnapshot AfterFailure;
	FShanmenItemAuthorityDocument DocumentAfterFailure;
	TArray<uint8> BytesAfterFailure;
	TestTrue(TEXT("Failed persistence exposes no partial commit or ActiveRun"),
		Failed.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
		&& Service.TryCaptureSnapshot(AfterFailure)
		&& AfterFailure == Before
		&& Service.TryGetDocument(DocumentAfterFailure)
		&& DocumentAfterFailure == DocumentBefore
		&& ReadServiceBytes(Storage.PrimaryPath(), BytesAfterFailure)
		&& BytesAfterFailure == BytesBefore);

#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Started =
		Service.StartPreparedRunDurable(Request);
	FShanmenItemAuthoritySnapshot AfterStart;
	FShanmenItemAuthorityDocument DocumentAfterStart;
	TestTrue(TEXT("Retry commits preparation and ActiveRun in one generation"),
		Started.Status == EShanmenItemDurableCommandStatus::Persisted
		&& Started.IsCommandSuccess()
		&& Started.Receipt.Operation
			== EShanmenItemTransactionOperation::StartPreparedRun
		&& Service.TryCaptureSnapshot(AfterStart)
		&& Service.TryGetDocument(DocumentAfterStart)
		&& AfterStart.AuthorityRevision == Before.AuthorityRevision + 1
		&& DocumentAfterStart.SaveGeneration
			== DocumentBefore.SaveGeneration + 1);

	FShanmenItemAuthorityService Restarted;
	const bool bRestarted = Restarted.StartExisting(Storage).IsReady();
	const FShanmenItemDurableCommandResult Replay = bRestarted
		? Restarted.StartPreparedRunDurable(Request)
		: FShanmenItemDurableCommandResult();
	FShanmenItemAuthorityDocument DocumentAfterReplay;
	TestTrue(TEXT("Restart replays the exact atomic-start receipt without a write"),
		bRestarted
		&& Replay.Status == EShanmenItemDurableCommandStatus::Replayed
		&& Replay.Receipt == Started.Receipt
		&& Restarted.TryGetDocument(DocumentAfterReplay)
		&& DocumentAfterReplay.SaveGeneration
			== DocumentAfterStart.SaveGeneration);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServicePreparedRunResourceCommitTest,
	"Shanmen.0_0_10.Items.AuthorityService.PreparedRunResourceCommitDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServicePreparedRunResourceCommitTest::RunTest(
	const FString&)
{
	const FString Root = NewServiceRoot(TEXT("PreparedRunResourceCommit"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Prepared resource fixture publishes"),
		CreateService(Service, Storage, 12).IsReady());
	const FShanmenItemDurableCommandResult Deployment =
		Service.ReserveDurable(ServiceReserve(
			130, 1, 0, EShanmenItemResourceKind::DeploymentLock));
	const FShanmenItemDurableCommandResult Started =
		Deployment.IsCommandSuccess()
			? Service.StartPreparedRunDurable(ServiceRunStart(
				131, { Deployment.Receipt.ReservationId }))
			: FShanmenItemDurableCommandResult();
	const FShanmenItemDurableCommandResult Durability =
		Started.IsCommandSuccess()
			? Service.ReserveDurable(ServiceReserve(
				132, 2, 1, EShanmenItemResourceKind::Durability))
			: FShanmenItemDurableCommandResult();
	const FShanmenItemRunResourceCommitRequest Request =
		ServiceRunResourceCommit(
			133, Started.Receipt.ReservationId,
			Durability.Receipt.ReservationId);
	TestTrue(TEXT("Active Run owns one post-start durability intent"),
		Deployment.IsCommandSuccess() && Started.IsCommandSuccess()
			&& Durability.IsCommandSuccess() && Request.IsValid());

	FShanmenItemAuthoritySnapshot Before;
	FShanmenItemAuthorityDocument DocumentBefore;
	TestTrue(TEXT("Pre-commit durable state is readable"),
		Service.TryCaptureSnapshot(Before)
			&& Service.TryGetDocument(DocumentBefore));
#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult Failed =
		Service.CommitPreparedRunResourcesDurable(Request);
	FShanmenItemAuthoritySnapshot AfterFailure;
	TestTrue(TEXT("Persistence failure rolls back every triggered resource"),
		Failed.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& Service.TryCaptureSnapshot(AfterFailure)
			&& AfterFailure == Before);

#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Committed =
		Service.CommitPreparedRunResourcesDurable(Request);
	FShanmenItemAuthoritySnapshot AfterCommit;
	FShanmenItemAuthorityDocument DocumentAfterCommit;
	const FShanmenItemInstance* WornItem = nullptr;
	if (Service.TryCaptureSnapshot(AfterCommit))
	{
		WornItem = AfterCommit.Items.FindByPredicate(
			[](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == ServiceDeployItemId;
			});
	}
	TestTrue(TEXT("Retry durably commits wear in one authority revision"),
		Committed.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Committed.IsCommandSuccess()
			&& Committed.Receipt.Operation
				== EShanmenItemTransactionOperation::CommitPreparedRunResources
			&& WornItem && WornItem->Durability == 3
			&& Service.TryGetDocument(DocumentAfterCommit)
			&& AfterCommit.AuthorityRevision == Before.AuthorityRevision + 1
			&& DocumentAfterCommit.SaveGeneration
				== DocumentBefore.SaveGeneration + 1);

	FShanmenItemAuthorityService Restarted;
	const FShanmenItemDurableCommandResult Replay =
		Restarted.StartExisting(Storage).IsReady()
			? Restarted.CommitPreparedRunResourcesDurable(Request)
			: FShanmenItemDurableCommandResult();
	FShanmenItemAuthoritySnapshot RestartedSnapshot;
	TestTrue(TEXT("Restart replays the exact resource receipt without double wear"),
		Replay.Status == EShanmenItemDurableCommandStatus::Replayed
			&& Replay.Receipt == Committed.Receipt
			&& Restarted.TryCaptureSnapshot(RestartedSnapshot)
			&& RestartedSnapshot == AfterCommit);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServicePreparedRunResourceIntentTest,
	"Shanmen.0_0_10.Items.AuthorityService.PreparedRunResourceIntentDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServicePreparedRunResourceIntentTest::RunTest(
	const FString&)
{
	const FString Root = NewServiceRoot(TEXT("PreparedRunResourceIntent"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Prepared resource-intent fixture publishes"),
		CreateService(Service, Storage, 12).IsReady());
	const FShanmenItemDurableCommandResult Deployment =
		Service.ReserveDurable(ServiceReserve(
			140, 1, 0, EShanmenItemResourceKind::DeploymentLock));
	const FShanmenItemDurableCommandResult Started =
		Deployment.IsCommandSuccess()
			? Service.StartPreparedRunDurable(ServiceRunStart(
				141, { Deployment.Receipt.ReservationId }))
			: FShanmenItemDurableCommandResult();
	const FShanmenItemDurableCommandResult Durability =
		Started.IsCommandSuccess()
			? Service.ReserveDurable(ServiceReserve(
				142, 2, 1, EShanmenItemResourceKind::Durability))
			: FShanmenItemDurableCommandResult();
	const FGuid IntentId(0x51380000, 0, 0, 1);
	const FShanmenItemRunResourceIntentRequest PrepareRequest =
		ServiceRunResourceIntent(
			143, Started.Receipt.ReservationId,
			IntentId, Durability.Receipt.ReservationId);
	TestTrue(TEXT("Active Run owns one recoverable resource intent"),
		Deployment.IsCommandSuccess() && Started.IsCommandSuccess()
			&& Durability.IsCommandSuccess() && PrepareRequest.IsValid());

	FShanmenItemAuthoritySnapshot BeforePrepare;
	FShanmenItemAuthorityDocument DocumentBeforePrepare;
	TestTrue(TEXT("Pre-prepare durable state is readable"),
		Service.TryCaptureSnapshot(BeforePrepare)
			&& Service.TryGetDocument(DocumentBeforePrepare));
#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult FailedPrepare =
		Service.PreparePreparedRunResourceIntentDurable(PrepareRequest);
	FShanmenItemAuthoritySnapshot AfterFailedPrepare;
	TestTrue(TEXT("Prepare write failure exposes neither intent nor partial spend"),
		FailedPrepare.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& Service.TryCaptureSnapshot(AfterFailedPrepare)
			&& AfterFailedPrepare == BeforePrepare);

#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Prepared =
		Service.PreparePreparedRunResourceIntentDurable(PrepareRequest);
	FShanmenItemAuthoritySnapshot AfterPrepare;
	FShanmenItemAuthorityDocument DocumentAfterPrepare;
	const FShanmenItemInstance* PreparedItem = nullptr;
	if (Service.TryCaptureSnapshot(AfterPrepare))
	{
		PreparedItem = AfterPrepare.Items.FindByPredicate(
			[](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == ServiceDeployItemId;
			});
	}
	TestTrue(TEXT("Prepare persists in one generation without consuming durability"),
		Prepared.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Prepared.IsCommandSuccess()
			&& Prepared.Receipt.Operation
				== EShanmenItemTransactionOperation::PreparePreparedRunResourceIntent
			&& PreparedItem && PreparedItem->Durability == 5
			&& Service.TryGetDocument(DocumentAfterPrepare)
			&& AfterPrepare.AuthorityRevision
				== BeforePrepare.AuthorityRevision + 1
			&& DocumentAfterPrepare.SaveGeneration
				== DocumentBeforePrepare.SaveGeneration + 1);

	FShanmenItemAuthorityService RestartedPending;
	FShanmenItemAuthoritySnapshot PendingSnapshot;
	TestTrue(TEXT("Restart restores the exact pending intent"),
		RestartedPending.StartExisting(Storage).IsReady()
			&& RestartedPending.TryCaptureSnapshot(PendingSnapshot)
			&& PendingSnapshot == AfterPrepare);

	const FShanmenItemRunResourceIntentFinalizeRequest FinalizeRequest =
		ServiceRunResourceIntentFinalize(
			144, Started.Receipt.ReservationId,
			PrepareRequest.Context.RequestId, IntentId, true);
	FShanmenItemAuthorityDocument DocumentBeforeFinalize;
	TestTrue(TEXT("Pending document is readable before finalization"),
		RestartedPending.TryGetDocument(DocumentBeforeFinalize));
#if WITH_DEV_AUTOMATION_TESTS
	RestartedPending.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult FailedFinalize =
		RestartedPending.FinalizePreparedRunResourceIntentDurable(
			FinalizeRequest);
	FShanmenItemAuthoritySnapshot AfterFailedFinalize;
	TestTrue(TEXT("Finalize write failure preserves the recoverable pending intent"),
		FailedFinalize.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& RestartedPending.TryCaptureSnapshot(AfterFailedFinalize)
			&& AfterFailedFinalize == PendingSnapshot);

#if WITH_DEV_AUTOMATION_TESTS
	RestartedPending.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Finalized =
		RestartedPending.FinalizePreparedRunResourceIntentDurable(
			FinalizeRequest);
	FShanmenItemAuthoritySnapshot AfterFinalize;
	FShanmenItemAuthorityDocument DocumentAfterFinalize;
	const FShanmenItemInstance* WornItem = nullptr;
	if (RestartedPending.TryCaptureSnapshot(AfterFinalize))
	{
		WornItem = AfterFinalize.Items.FindByPredicate(
			[](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == ServiceDeployItemId;
			});
	}
	TestTrue(TEXT("Retry atomically publishes the external success decision"),
		Finalized.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Finalized.IsCommandSuccess()
			&& Finalized.Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRunResourceIntent
			&& WornItem && WornItem->Durability == 3
			&& RestartedPending.TryGetDocument(DocumentAfterFinalize)
			&& AfterFinalize.AuthorityRevision
				== PendingSnapshot.AuthorityRevision + 1
			&& DocumentAfterFinalize.SaveGeneration
				== DocumentBeforeFinalize.SaveGeneration + 1);

	FShanmenItemAuthorityService RestartedFinal;
	const FShanmenItemDurableCommandResult Replay =
		RestartedFinal.StartExisting(Storage).IsReady()
			? RestartedFinal.FinalizePreparedRunResourceIntentDurable(
				FinalizeRequest)
			: FShanmenItemDurableCommandResult();
	FShanmenItemAuthoritySnapshot ReplayedSnapshot;
	TestTrue(TEXT("Final decision replays after restart without double wear"),
		Replay.Status == EShanmenItemDurableCommandStatus::Replayed
			&& Replay.Receipt == Finalized.Receipt
			&& RestartedFinal.TryCaptureSnapshot(ReplayedSnapshot)
			&& ReplayedSnapshot == AfterFinalize);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServicePreparedRunQuantityIntentTest,
	"Shanmen.0_0_10.Items.AuthorityService.PreparedRunQuantityIntentDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServicePreparedRunQuantityIntentTest::RunTest(
	const FString&)
{
	const FString Root = NewServiceRoot(TEXT("PreparedRunQuantityIntent"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Prepared quantity-intent fixture publishes"),
		CreateService(Service, Storage, 8).IsReady());
	const FShanmenItemDurableCommandResult Quantity =
		Service.ReserveDurable(ServiceReserve(150, 8));
	const FShanmenItemDurableCommandResult Started =
		Quantity.IsCommandSuccess()
			? Service.StartPreparedRunDurable(ServiceRunStart(
				151, { Quantity.Receipt.ReservationId }))
			: FShanmenItemDurableCommandResult();
	const FGuid IntentId(0x513B0000, 0, 0, 1);
	const FShanmenItemRunQuantityIntentRequest PrepareRequest =
		ServiceRunQuantityIntent(
			152, Started.Receipt.ReservationId, IntentId, 2, 8);
	TestTrue(TEXT("Active Run owns one durable stack intent"),
		Quantity.IsCommandSuccess() && Started.IsCommandSuccess()
			&& PrepareRequest.IsValid());

	FShanmenItemAuthoritySnapshot BeforePrepare;
	FShanmenItemAuthorityDocument DocumentBeforePrepare;
	TestTrue(TEXT("Pre-prepare quantity state is readable"),
		Service.TryCaptureSnapshot(BeforePrepare)
			&& Service.TryGetDocument(DocumentBeforePrepare));
#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult FailedPrepare =
		Service.PreparePreparedRunQuantityIntentDurable(PrepareRequest);
	FShanmenItemAuthoritySnapshot AfterFailedPrepare;
	TestTrue(TEXT("Prepare write failure exposes no partial stack intent"),
		FailedPrepare.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& Service.TryCaptureSnapshot(AfterFailedPrepare)
			&& AfterFailedPrepare == BeforePrepare);

#if WITH_DEV_AUTOMATION_TESTS
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Prepared =
		Service.PreparePreparedRunQuantityIntentDurable(PrepareRequest);
	FShanmenItemAuthoritySnapshot AfterPrepare;
	FShanmenItemAuthorityDocument DocumentAfterPrepare;
	TestTrue(TEXT("Prepare persists one unchanged eight-unit balance"),
		Prepared.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Prepared.IsCommandSuccess()
			&& Prepared.Receipt.Operation
				== EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
			&& Prepared.Receipt.ResourceBefore == 8
			&& Prepared.Receipt.ResourceAfter == 8
			&& Prepared.Receipt.AvailableAfter == 6
			&& Service.TryCaptureSnapshot(AfterPrepare)
			&& Service.TryGetDocument(DocumentAfterPrepare)
			&& AfterPrepare.AuthorityRevision
				== BeforePrepare.AuthorityRevision + 1
			&& DocumentAfterPrepare.SaveGeneration
				== DocumentBeforePrepare.SaveGeneration + 1);

	FShanmenItemAuthorityService RestartedPending;
	FShanmenItemAuthoritySnapshot PendingSnapshot;
	TestTrue(TEXT("Restart restores the exact pending stack intent"),
		RestartedPending.StartExisting(Storage).IsReady()
			&& RestartedPending.TryCaptureSnapshot(PendingSnapshot)
			&& PendingSnapshot == AfterPrepare);

	const FShanmenItemRunQuantityIntentFinalizeRequest FinalizeRequest =
		ServiceRunQuantityIntentFinalize(
			153, Started.Receipt.ReservationId,
			PrepareRequest.Context.RequestId, IntentId, true);
	FShanmenItemAuthorityDocument DocumentBeforeFinalize;
	TestTrue(TEXT("Pending stack document is readable before finalization"),
		RestartedPending.TryGetDocument(DocumentBeforeFinalize));
#if WITH_DEV_AUTOMATION_TESTS
	RestartedPending.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::WriteTemp);
#endif
	const FShanmenItemDurableCommandResult FailedFinalize =
		RestartedPending.FinalizePreparedRunQuantityIntentDurable(
			FinalizeRequest);
	FShanmenItemAuthoritySnapshot AfterFailedFinalize;
	TestTrue(TEXT("Finalize write failure preserves the pending stack intent"),
		FailedFinalize.Status
			== EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& RestartedPending.TryCaptureSnapshot(AfterFailedFinalize)
			&& AfterFailedFinalize == PendingSnapshot);

#if WITH_DEV_AUTOMATION_TESTS
	RestartedPending.SetInjectedFailureForTests(
		EShanmenItemStoreFailureStage::None);
#endif
	const FShanmenItemDurableCommandResult Finalized =
		RestartedPending.FinalizePreparedRunQuantityIntentDurable(
			FinalizeRequest);
	FShanmenItemAuthoritySnapshot AfterFinalize;
	FShanmenItemAuthorityDocument DocumentAfterFinalize;
	TestTrue(TEXT("Retry atomically publishes one two-unit launch consumption"),
		Finalized.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Finalized.IsCommandSuccess()
			&& Finalized.Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
			&& Finalized.Receipt.Phase
				== EShanmenItemTransactionPhase::Committed
			&& Finalized.Receipt.ResourceBefore == 8
			&& Finalized.Receipt.ResourceAfter == 6
			&& RestartedPending.TryCaptureSnapshot(AfterFinalize)
			&& RestartedPending.TryGetDocument(DocumentAfterFinalize)
			&& AfterFinalize.AuthorityRevision
				== PendingSnapshot.AuthorityRevision + 1
			&& DocumentAfterFinalize.SaveGeneration
				== DocumentBeforeFinalize.SaveGeneration + 1);

	FShanmenItemAuthorityService RestartedFinal;
	const FShanmenItemDurableCommandResult Replay =
		RestartedFinal.StartExisting(Storage).IsReady()
			? RestartedFinal.FinalizePreparedRunQuantityIntentDurable(
				FinalizeRequest)
			: FShanmenItemDurableCommandResult();
	FShanmenItemAuthoritySnapshot ReplayedSnapshot;
	TestTrue(TEXT("Final launch decision replays without double consumption"),
		Replay.Status == EShanmenItemDurableCommandStatus::Replayed
			&& Replay.Receipt == Finalized.Receipt
			&& RestartedFinal.TryCaptureSnapshot(ReplayedSnapshot)
			&& ReplayedSnapshot == AfterFinalize);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemAuthorityServiceSerializationTest,
	"Shanmen.0_0_10.Items.AuthorityService.ConcurrentCommandsSerialized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemAuthorityServiceSerializationTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("Concurrent"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Concurrency fixture publishes"),
		CreateService(Service, Storage, 12).IsReady());

	constexpr int32 CommandCount = 8;
	TArray<TFuture<FShanmenItemDurableCommandResult>> Futures;
	for (int32 Index = 0; Index < CommandCount; ++Index)
	{
		const FShanmenItemReserveRequest Request =
			ServiceReserve(100 + static_cast<uint32>(Index));
		Futures.Add(Async(
			EAsyncExecution::ThreadPool,
			[&Service, Request]()
			{
				return Service.ReserveDurable(Request);
			}));
	}
	for (TFuture<FShanmenItemDurableCommandResult>& Future : Futures)
	{
		const FShanmenItemDurableCommandResult Result = Future.Get();
		TestTrue(TEXT("Concurrent command is serialized and durable"),
			Result.Status == EShanmenItemDurableCommandStatus::Persisted
			&& Result.IsCommandSuccess());
	}

	FShanmenItemAuthorityDocument Document;
	FShanmenItemAuthoritySnapshot Snapshot;
	TestTrue(TEXT("Eight serialized commands produce eight exact generations"),
		Service.TryGetDocument(Document)
		&& Service.TryCaptureSnapshot(Snapshot)
		&& Document.SaveGeneration == 1 + CommandCount
		&& Snapshot.AuthorityRevision == CommandCount
		&& Snapshot.Reservations.Num() == CommandCount
		&& Snapshot.ProcessedRequests.Num() == CommandCount);
	FShanmenItemAuthorityService Restarted;
	FShanmenItemAuthoritySnapshot RestartedSnapshot;
	TestTrue(TEXT("Serialized final state survives restart exactly"),
		Restarted.StartExisting(Storage).IsReady()
		&& Restarted.TryCaptureSnapshot(RestartedSnapshot)
		&& RestartedSnapshot == Snapshot);

	RemoveServiceRoot(Root);
	return true;
}

namespace
{
	FGuid StartSourceRun(FAutomationTestBase& Test, FShanmenItemAuthorityService& Service,
		const FShanmenItemStorageContext& Storage)
	{
		Test.TestTrue(TEXT("Source fixture migration is durable"), CreateService(Service, Storage).IsReady());
		const auto Reserved = Service.ReserveDurable(ServiceReserve(200, 4));
		Test.TestTrue(TEXT("Source fixture reserves four nonzero units"), Reserved.IsCommandSuccess());
		const auto Started = Service.StartPreparedRunDurable(ServiceRunStart(201, { Reserved.Receipt.ReservationId }));
		Test.TestTrue(TEXT("Source fixture owns a real active Run"), Started.IsCommandSuccess());
		return Started.Receipt.ReservationId;
	}
	FShanmenItemGeneratedSourceRequest SourceRequest(const FGuid& RunId, FName Role = TEXT("Source.Test.First"),
		int64 Sequence = 0, int32 PityBefore = 0, int32 PityAfter = 7)
	{
		FShanmenItemGeneratedSourceRequest R;
		R.ItemContent = ServiceContent();
		auto& P = R.Plan;
		P.OwnerId = ServiceOwnerId; P.RunId = RunId; P.SourceRoleId = Role;
		P.Content.Version = TEXT("Source.Test.Manifest.v1"); P.Content.Digest = TEXT("Source.Manifest.Nonzero");
		P.ProjectionId = TEXT("Projection.Test"); P.DistributionProfileId = TEXT("Distribution.Test");
		P.BudgetProfileId = TEXT("Budget.Test"); P.EffectiveSeed = MAX_uint64;
		P.RandomizedBudget = MAX_int64; P.GeneratedTotalValue = MAX_int64 - 1; P.ResidualValue = 1;
		P.ExpectedSequence = Sequence; P.PityStateBefore = PityBefore; P.PityStateAfter = PityAfter;
		P.bPityCommitRequired = true;
		auto& E = P.Entries.AddDefaulted_GetRef();
		E.Definition = ServiceCandidate().Definitions[0]; E.Quantity = 3;
		E.SectionId = TEXT("Section.Test"); E.SlotIndex = 0;
		E.UnitValue = 9007199254740993LL; E.TotalValue = MAX_int64 - 1;
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceDurableRestartTest,
	"Shanmen.0_0_10.Items.GeneratedSource.DurableRestartAndTerminalReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceDurableRestartTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("SourceRestart"));
	const auto Storage = FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	const FGuid Run = StartSourceRun(*this, Service, Storage);
	const auto First = SourceRequest(Run);
	const auto Second = SourceRequest(Run, TEXT("Source.Test.Second"), 1, 7, 8);
	FShanmenItemAuthorityDocument Before, After;
	Service.TryGetDocument(Before);
	const auto Accepted = Service.AcceptGeneratedSourceDurable(First);
	TestTrue(TEXT("Source and receipt commit in the inventory document"), Accepted.IsCommandSuccess());
	TestTrue(TEXT("Next source uses the first source's nonzero pity and cursor"),
		Service.AcceptGeneratedSourceDurable(Second).IsCommandSuccess());
	TestTrue(TEXT("Exactly two generations; source entries are not unproven inventory items"),
		Service.TryGetDocument(After) && After.SaveGeneration == Before.SaveGeneration + 2
		&& After.Authority.GeneratedSources.Num() == 2 && After.Authority.Items == Before.Authority.Items);
	FShanmenItemAuthorityService Restart;
	FShanmenItemGeneratedSourceReceipt Source;
	TestTrue(TEXT("Fresh authority restores the complete lossless source plan"),
		Restart.StartExisting(Storage).IsReady()
		&& Restart.TryGetGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId, Source)
		&& Source.GetPlan() == First.Plan && Source.IsValid());
	TArray<uint8> BytesBefore, BytesAfter;
	ReadServiceBytes(Storage.PrimaryPath(), BytesBefore);
	const auto Replay = Restart.AcceptGeneratedSourceDurable(First);
	TestTrue(TEXT("Older source replay after a later source is exact and write-free"),
		Replay.Status == EShanmenItemDurableCommandStatus::Replayed && Replay.Receipt == Accepted.Receipt
		&& ReadServiceBytes(Storage.PrimaryPath(), BytesAfter) && BytesAfter == BytesBefore);
	TestFalse(TEXT("Wrong-owner read clears a previously valid output"),
		Restart.TryGetGeneratedSource(FGuid(8, 8, 8, 8), Run, First.Plan.SourceRoleId, Source));
	TestFalse(TEXT("No stale output survives denied read"), Source.IsValid());
	FShanmenItemRunFinalizeRequest End;
	End.Context.RunId = ServiceRunId; End.Context.OwnerId = ServiceOwnerId;
	End.Context.RequestId = FGuid(0x513F0100, 0, 0, 1); End.Context.Content = ServiceContent();
	End.ActiveRunId = Run; End.TerminalReason = EShanmenItemRunTerminalReason::Abandon;
	TestTrue(TEXT("Existing Run terminal command preserves source history"), Restart.FinalizePreparedRunDurable(End).IsCommandSuccess());
	TestTrue(TEXT("Exact source history remains replayable after terminal"), Restart.AcceptGeneratedSourceDurable(First).IsCommandSuccess());
	TestFalse(TEXT("Terminal Run cannot accept a new source"),
		Restart.AcceptGeneratedSourceDurable(SourceRequest(Run, TEXT("Source.Test.Third"), 2, 8, 9)).IsCommandSuccess());
	FShanmenItemAuthorityService Closed, TerminalRestart;
	TestFalse(TEXT("Closed service cannot publish sources"), Closed.TryGetGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId, Source));
	TestTrue(TEXT("Terminal and immutable history survive another reopen"), TerminalRestart.StartExisting(Storage).IsReady()
		&& TerminalRestart.TryGetGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId, Source) && Source.GetPlan() == First.Plan);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceDurableFailureTest,
	"Shanmen.0_0_10.Items.GeneratedSource.DurableFailureAndReconcile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceDurableFailureTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("SourceFailures"));
	const auto Storage = FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	const auto Request = SourceRequest(StartSourceRun(*this, Service, Storage));
	FShanmenItemAuthorityDocument Before, After;
	Service.TryGetDocument(Before);
	TArray<uint8> BytesBefore, BytesAfter;
	ReadServiceBytes(Storage.PrimaryPath(), BytesBefore);
	for (const auto Stage : { EShanmenItemStoreFailureStage::WriteTemp, EShanmenItemStoreFailureStage::ReadBackTemp,
		EShanmenItemStoreFailureStage::PrepareBackup, EShanmenItemStoreFailureStage::AtomicReplace })
	{
		Service.SetInjectedFailureForTests(Stage);
		const auto Failed = Service.AcceptGeneratedSourceDurable(Request);
		FShanmenItemGeneratedSourceReceipt Source;
		TestTrue(TEXT("Pre-publication failure rolls back document, cursor, pity and receipt"),
			!Failed.IsCommandSuccess() && Failed.Status == EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& Service.TryGetDocument(After) && After == Before
			&& ReadServiceBytes(Storage.PrimaryPath(), BytesAfter) && BytesAfter == BytesBefore
			&& !Service.TryGetGeneratedSource(ServiceOwnerId, Request.Plan.RunId, Request.Plan.SourceRoleId, Source));
	}
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::ReadBackCommittedPrimary);
	const auto Reconciled = Service.AcceptGeneratedSourceDurable(Request);
	TestTrue(TEXT("Post-replace ambiguity resolves only from exact durable after-state"),
		Reconciled.Status == EShanmenItemDurableCommandStatus::ResolvedAfterReopen && Reconciled.IsCommandSuccess()
		&& Service.TryGetDocument(After) && After.SaveGeneration == Before.SaveGeneration + 1);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Retry cannot duplicate a reconciled acceptance"),
		Service.AcceptGeneratedSourceDurable(Request).Status == EShanmenItemDurableCommandStatus::Replayed);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceDurableGuardsTest,
	"Shanmen.0_0_10.Items.GeneratedSource.DurableGuardsAndSnapshotIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceDurableGuardsTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("SourceGuards"));
	const auto Storage = FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	const FGuid Run = StartSourceRun(*this, Service, Storage);
	const auto First = SourceRequest(Run);
	const auto Accepted = Service.AcceptGeneratedSourceDurable(First);
	if (!TestTrue(TEXT("Guard baseline has accepted nonzero source"), Accepted.IsCommandSuccess()))
	{
		AddError(Accepted.Diagnostic);
		RemoveServiceRoot(Root);
		return false;
	}
	FShanmenItemAuthorityDocument Before, After;
	Service.TryGetDocument(Before);
	for (int32 Case = 0; Case < 9; ++Case)
	{
		auto R = SourceRequest(Run, TEXT("Source.Test.Second"), 1, 7, 8);
		switch (Case)
		{
		case 0: R.Plan.OwnerId = FGuid(9, 9, 9, 9); break;
		case 1: R.Plan.RunId = ServiceRunId; break;
		case 2: R.ItemContent.Digest += TEXT(".drift"); break;
		case 3: R.Plan.ExpectedSequence = 0; break;
		case 4: R.Plan.PityStateBefore = 0; break;
		case 5: R.Plan.Content.Digest += TEXT(".drift"); break;
		case 6: R.Plan.Entries[0].Definition.MaxStack = 31; break;
		case 7: R = First; ++R.Plan.EffectiveSeed; break; // wraps to invalid zero
		case 8: R = First; --R.Plan.EffectiveSeed; break; // valid but conflicting identity
		}
		const auto Denied = Service.AcceptGeneratedSourceDurable(R);
		TestTrue(*FString::Printf(TEXT("Invalid source %d cannot mutate any authority field"), Case),
			!Denied.IsCommandSuccess() && Service.TryGetDocument(After) && After == Before);
	}
	for (int32 Case = 0; Case < 6; ++Case)
	{
		auto Bad = Before.Authority;
		const FGuid Id = FShanmenItemGeneratedSourceContract::MakeSourceId(ServiceOwnerId, Run, First.Plan.SourceRoleId);
		switch (Case)
		{
		case 0: Bad.GeneratedSources.Reset(); break;
		case 1: Bad.ProcessedRequests.RemoveAll([&](const auto& R) { return R.RequestId == Id; }); break;
		case 2: --Bad.GeneratedSources[0].EffectiveSeed; break;
		case 3: Bad.GeneratedSources[0].ExpectedSequence = 1; break;
		case 4: { const auto Duplicate = Bad.GeneratedSources[0]; Bad.GeneratedSources.Add(Duplicate); break; }
		case 5: Bad.GeneratedSources[0].PityStateAfter = 8; break;
		}
		FShanmenItemRepository Repo;
		TestFalse(*FString::Printf(TEXT("Damaged source/receipt snapshot %d fails closed"), Case), Repo.TryLoadSnapshot(Bad));
	}
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceDurableConcurrentTest,
	"Shanmen.0_0_10.Items.GeneratedSource.DurableConcurrentAcceptance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceDurableConcurrentTest::RunTest(const FString&)
{
	const FString Root = NewServiceRoot(TEXT("SourceConcurrency"));
	const auto Storage = FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	const FGuid Run = StartSourceRun(*this, Service, Storage);
	const auto Request = SourceRequest(Run);
	FShanmenItemAuthorityDocument Before, After;
	Service.TryGetDocument(Before);
	TArray<TFuture<FShanmenItemDurableCommandResult>> Futures;
	for (int32 I = 0; I < 8; ++I)
	{
		Futures.Add(Async(EAsyncExecution::ThreadPool, [&Service, Request]() {
			const auto Accepted = Service.AcceptGeneratedSourceDurable(Request);
			const auto Read = Service.ReadGeneratedSource(ServiceOwnerId, Request.Plan.RunId, Request.Plan.SourceRoleId);
			if (Read.Status != EShanmenItemGeneratedSourceReadStatus::Accepted || Read.AcceptedSequence != 1
				|| Read.PityState != 7 || !(Read.Receipt.GetPlan() == Request.Plan))
			{
				return FShanmenItemDurableCommandResult();
			}
			return Accepted;
		}));
	}
	int32 Persisted = 0, Replayed = 0;
	for (auto& Future : Futures)
	{
		const auto R = Future.Get();
		TestTrue(TEXT("Every concurrent same-source command returns durable success"), R.IsCommandSuccess());
		Persisted += R.Status == EShanmenItemDurableCommandStatus::Persisted;
		Replayed += R.Status == EShanmenItemDurableCommandStatus::Replayed;
	}
	TestTrue(TEXT("Eight concurrent attempts accept exactly once"), Persisted == 1 && Replayed == 7
		&& Service.TryGetDocument(After) && After.SaveGeneration == Before.SaveGeneration + 1
		&& After.Authority.GeneratedSources.Num() == 1);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceReadLifecycleTest,
	"Shanmen.0_0_10.Items.GeneratedSource.Read.LifecycleAndRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceReadLifecycleTest::RunTest(const FString&)
{
	using ERead = EShanmenItemGeneratedSourceReadStatus;
	using ERun = EShanmenItemGeneratedSourceRunState;
	const FString Root = NewServiceRoot(TEXT("SourceReadLifecycle"));
	const auto Storage = FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service;
	const FGuid Run = StartSourceRun(*this, Service, Storage);
	const auto First = SourceRequest(Run);
	const auto Empty = Service.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
	TestTrue(TEXT("First source is proven absent, with unbound source manifest and actual item content"),
		Empty.Status == ERead::Absent && Empty.RunState == ERun::Active
		&& Empty.OwnerId == ServiceOwnerId && Empty.RunId == Run && Empty.SourceRoleId == First.Plan.SourceRoleId
		&& Empty.AcceptedSequence == 0 && Empty.PityState == 0 && !Empty.SourceContent.IsValid()
		&& Empty.ItemContent.Version == ServiceContent().Version && Empty.ItemContent.Digest == ServiceContent().Digest
		&& Empty.AuthorityRevision > 0 && !Empty.Receipt.IsValid());
	TestTrue(TEXT("Nonzero durable first source"), Service.AcceptGeneratedSourceDurable(First).IsCommandSuccess());
	const auto ReadFirst = Service.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
	TestTrue(TEXT("One read binds accepted plan, cursor, pity, manifest and revision"),
		ReadFirst.Status == ERead::Accepted && ReadFirst.RunState == ERun::Active
		&& ReadFirst.Receipt.GetPlan() == First.Plan && ReadFirst.AcceptedSequence == 1 && ReadFirst.PityState == 7
		&& ReadFirst.SourceContent.Version == First.Plan.Content.Version && ReadFirst.SourceContent.Digest == First.Plan.Content.Digest
		&& ReadFirst.AuthorityRevision == Empty.AuthorityRevision + 1);
	const auto Second = SourceRequest(Run, TEXT("Source.Test.Second"), ReadFirst.AcceptedSequence, ReadFirst.PityState, 8);
	const auto Stale = SourceRequest(Run, TEXT("Source.Test.Stale"), ReadFirst.AcceptedSequence, ReadFirst.PityState, 9);
	TestTrue(TEXT("Next source accepts from read cursor"), Service.AcceptGeneratedSourceDurable(Second).IsCommandSuccess());
	FShanmenItemAuthorityDocument Before, After;
	TArray<uint8> BytesBefore, BytesAfter;
	TestTrue(TEXT("Capture nonempty state baseline"), Service.TryGetDocument(Before) && ReadServiceBytes(Storage.PrimaryPath(), BytesBefore));
	TestFalse(TEXT("Read result is not a lease: intervening acceptance rejects stale cursor"), Service.AcceptGeneratedSourceDurable(Stale).IsCommandSuccess());
	const auto Older = Service.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
	const auto Absent = Service.ReadGeneratedSource(ServiceOwnerId, Run, Stale.Plan.SourceRoleId);
	TestTrue(TEXT("Older receipt and current cursor coexist without re-planning"),
		Older.Status == ERead::Accepted && Older.Receipt == ReadFirst.Receipt
		&& Older.AcceptedSequence == 2 && Older.PityState == 8 && Older.AuthorityRevision == ReadFirst.AuthorityRevision + 1);
	TestTrue(TEXT("Absent source still reports current nonzero Run facts"), Absent.Status == ERead::Absent
		&& Absent.AcceptedSequence == 2 && Absent.PityState == 8 && !Absent.Receipt.IsValid());
	TestTrue(TEXT("Reads and stale rejection do not mutate document or bytes"), Service.TryGetDocument(After) && After == Before
		&& ReadServiceBytes(Storage.PrimaryPath(), BytesAfter) && BytesAfter == BytesBefore);
	FShanmenItemAuthorityService Restart;
	TestTrue(TEXT("Reopen without re-planning"), Restart.StartExisting(Storage).IsReady());
	const auto Reopened = Restart.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
	TestTrue(TEXT("Fresh service restores exact read facts"), Reopened.Status == Older.Status && Reopened.Receipt == Older.Receipt
		&& Reopened.AcceptedSequence == 2 && Reopened.PityState == 8 && Reopened.AuthorityRevision == Older.AuthorityRevision);
	FShanmenItemRunFinalizeRequest End;
	End.Context.RunId = ServiceRunId; End.Context.OwnerId = ServiceOwnerId;
	End.Context.RequestId = FGuid(0x513F0300, 0, 0, 1); End.Context.Content = ServiceContent();
	End.ActiveRunId = Run; End.TerminalReason = EShanmenItemRunTerminalReason::Abandon;
	TestTrue(TEXT("Finalize real source Run"), Restart.FinalizePreparedRunDurable(End).IsCommandSuccess());
	FShanmenItemAuthorityService TerminalRestart;
	TestTrue(TEXT("Terminal is durable across reopen"), TerminalRestart.StartExisting(Storage).IsReady());
	const auto Terminal = TerminalRestart.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
	const auto TerminalAbsent = TerminalRestart.ReadGeneratedSource(ServiceOwnerId, Run, Stale.Plan.SourceRoleId);
	TestTrue(TEXT("Finalized accepted history remains visible, without reopening Run"), Terminal.Status == ERead::Accepted
		&& Terminal.RunState == ERun::Finalized && Terminal.Receipt == Older.Receipt && Terminal.AcceptedSequence == 2 && Terminal.PityState == 8);
	TestTrue(TEXT("Terminal absence never implies fresh generation permission"), TerminalAbsent.Status == ERead::Absent
		&& TerminalAbsent.RunState == ERun::Finalized && TerminalAbsent.AcceptedSequence == 2 && TerminalAbsent.PityState == 8);
	RemoveServiceRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceReadUnavailableTest,
	"Shanmen.0_0_10.Items.GeneratedSource.Read.UnavailableAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceReadUnavailableTest::RunTest(const FString&)
{
	using ERead = EShanmenItemGeneratedSourceReadStatus;
	auto Unavailable = [&](const FShanmenItemGeneratedSourceReadResult& R) {
		TestTrue(TEXT("Unavailable is not absence and exposes no stale facts"), R.Status == ERead::Unavailable
			&& R.RunState == EShanmenItemGeneratedSourceRunState::Unavailable && !R.OwnerId.IsValid() && !R.RunId.IsValid()
			&& R.SourceRoleId.IsNone() && !R.ItemContent.IsValid() && !R.SourceContent.IsValid()
			&& R.AuthorityRevision == INDEX_NONE && R.AcceptedSequence == 0 && R.PityState == 0 && !R.Receipt.IsValid());
	};
	const FString Root = NewServiceRoot(TEXT("SourceReadUnavailable"));
	const auto Storage = FShanmenItemStorageContext::ForRoot(Root, ServiceOwnerId);
	FShanmenItemAuthorityService Service, Closed;
	const auto First = SourceRequest(StartSourceRun(*this, Service, Storage));
	const FGuid Run = First.Plan.RunId;
	Unavailable(Closed.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId));
	for (const auto Stage : { EShanmenItemStoreFailureStage::WriteTemp, EShanmenItemStoreFailureStage::AtomicReplace })
	{
		Service.SetInjectedFailureForTests(Stage);
		const auto Failed = Service.AcceptGeneratedSourceDurable(First);
		const auto Read = Service.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
		TestTrue(TEXT("Proven rollback is Ready absence, not a candidate receipt"),
			Failed.Status == EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack
			&& Read.Status == ERead::Absent && Read.AcceptedSequence == 0 && Read.PityState == 0 && !Read.Receipt.IsValid());
	}
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::ReadBackCommittedPrimary);
	TestTrue(TEXT("Post-commit readback failure reconciles durable source"),
		Service.AcceptGeneratedSourceDurable(First).Status == EShanmenItemDurableCommandStatus::ResolvedAfterReopen);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
	const auto Good = Service.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId);
	TestTrue(TEXT("Reconciled read carries actual nonzero state"), Good.Status == ERead::Accepted && Good.AcceptedSequence == 1 && Good.PityState == 7);
	Unavailable(Service.ReadGeneratedSource(FGuid(9, 9, 9, 9), Run, First.Plan.SourceRoleId));
	Unavailable(Service.ReadGeneratedSource(FGuid(), Run, First.Plan.SourceRoleId));
	Unavailable(Service.ReadGeneratedSource(ServiceOwnerId, FGuid(), First.Plan.SourceRoleId));
	Unavailable(Service.ReadGeneratedSource(ServiceOwnerId, ServiceRunId, First.Plan.SourceRoleId));
	Unavailable(Service.ReadGeneratedSource(ServiceOwnerId, Run, NAME_None));
	FShanmenItemAuthorityService Stale;
	TestTrue(TEXT("Second lifecycle establishes nonzero read baseline"), Stale.StartExisting(Storage).IsReady()
		&& Stale.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId).Receipt == Good.Receipt);
	TestTrue(TEXT("Winning lifecycle advances"), Service.AcceptGeneratedSourceDurable(SourceRequest(Run, TEXT("Source.Winner"), 1, 7, 8)).IsCommandSuccess());
	const auto Diverged = Stale.AcceptGeneratedSourceDurable(SourceRequest(Run, TEXT("Source.Loser"), 1, 7, 9));
	TestTrue(TEXT("Actual divergent disk outcome enters recovery"), Diverged.Status == EShanmenItemDurableCommandStatus::RecoveryRequired);
	Unavailable(Stale.ReadGeneratedSource(ServiceOwnerId, Run, First.Plan.SourceRoleId));
	Unavailable(Stale.ReadGeneratedSource(ServiceOwnerId, Run, TEXT("Source.Loser")));
	FShanmenItemAuthorityService Recovered;
	TestTrue(TEXT("Fresh lifecycle recovers winning history only"), Recovered.StartExisting(Storage).IsReady());
	const auto Winner = Recovered.ReadGeneratedSource(ServiceOwnerId, Run, TEXT("Source.Winner"));
	const auto Loser = Recovered.ReadGeneratedSource(ServiceOwnerId, Run, TEXT("Source.Loser"));
	TestTrue(TEXT("Recovery distinguishes accepted winner from absent loser with identical current cursor"),
		Winner.Status == ERead::Accepted && Loser.Status == ERead::Absent
		&& Winner.AcceptedSequence == 2 && Loser.AcceptedSequence == 2 && Winner.PityState == 8 && Loser.PityState == 8);
	RemoveServiceRoot(Root);
	return true;
}

#endif
