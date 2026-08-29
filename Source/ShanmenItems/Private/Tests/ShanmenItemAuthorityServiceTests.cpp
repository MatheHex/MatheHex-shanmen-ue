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

#endif
