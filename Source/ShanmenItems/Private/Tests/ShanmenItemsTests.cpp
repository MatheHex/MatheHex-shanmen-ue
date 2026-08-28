#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"

namespace
{
	const FGuid RunId(1, 0, 0, 1);
	const FGuid OwnerId(2, 0, 0, 1);
	const FGuid ContainerId(3, 0, 0, 1);
	const FGuid DartId(4, 0, 0, 1);
	const FGuid SwordId(4, 0, 0, 2);
	const FGuid MirrorId(4, 0, 0, 3);
	const FGuid FormationMaterialId(4, 0, 0, 4);
	const FGuid AcquiredOreId(4, 0, 0, 5);
	const FGuid RecoveredChildContainerId(4, 0, 0, 6);
	const FGuid SafeChildItemId(4, 0, 0, 7);
	const FGuid AcquiredSatchelId(4, 0, 0, 8);
	const FGuid ForeignContainerId(4, 0, 0, 9);
	const FGuid ForeignRunId(4, 0, 0, 10);
	const FGuid ForeignOwnerId(4, 0, 0, 11);

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P1.0");
		Content.Digest = TEXT("TEST-ITEM-CONTENT-P1.0");
		return Content;
	}

	FShanmenOperationContext MakeContext(uint32 Sequence)
	{
		FShanmenOperationContext Context;
		Context.RunId = RunId;
		Context.OwnerId = OwnerId;
		Context.RequestId = FGuid(100, 0, 0, Sequence);
		Context.Content = MakeContent();
		return Context;
	}

	FShanmenItemDefinition MakeDefinition(
		FName DefinitionId,
		int32 MaxStack,
		const TArray<FGameplayTag>& Tags,
		int32 MaxDurability = 0,
		int32 MaxCharges = 0)
	{
		FShanmenItemDefinition Definition;
		Definition.DefinitionId = DefinitionId;
		Definition.MaxStack = MaxStack;
		Definition.MaxDurability = MaxDurability;
		Definition.MaxCharges = MaxCharges;
		for (const FGameplayTag& Tag : Tags)
		{
			Definition.ItemTags.AddTag(Tag);
		}
		return Definition;
	}

	FShanmenItemInstance MakeItem(
		const FGuid& ItemId,
		FName DefinitionId,
		int32 Slot,
		int32 Quantity,
		int32 Durability = 0,
		int32 Charges = 0)
	{
		FShanmenItemInstance Item;
		Item.ItemInstanceId = ItemId;
		Item.DefinitionId = DefinitionId;
		Item.RunId = RunId;
		Item.OwnerId = OwnerId;
		Item.ParentContainerId = ContainerId;
		Item.SlotIndex = Slot;
		Item.Quantity = Quantity;
		Item.Durability = Durability;
		Item.Charges = Charges;
		return Item;
	}

	FShanmenItemAuthoritySnapshot MakeSnapshot()
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Snapshot.Content = MakeContent();
		Snapshot.Definitions.Add(MakeDefinition(
			TEXT("Item.Weapon.ThrowingDart"),
			20,
			{ FShanmenItemNativeTags::CapabilityConsumeQuantity(), FShanmenItemNativeTags::ItemWeaponThrown() }));
		Snapshot.Definitions.Add(MakeDefinition(
			TEXT("Item.Weapon.FlyingSword"),
			1,
			{
				FShanmenItemNativeTags::CapabilityDeploy(),
				FShanmenItemNativeTags::CapabilityDurability(),
				FShanmenItemNativeTags::ItemWeaponFlyingSword()
			},
			100));
		Snapshot.Definitions.Add(MakeDefinition(
			TEXT("Item.Artifact.HeartMirror"),
			1,
			{
				FShanmenItemNativeTags::CapabilityDeploy(),
				FShanmenItemNativeTags::CapabilityCharges(),
				FShanmenItemNativeTags::ItemArtifactLethalGuard()
			},
			0,
			1));
		Snapshot.Definitions.Add(MakeDefinition(
			TEXT("Item.Material.FormationWood"),
			10,
			{ FShanmenItemNativeTags::CapabilityConsumeQuantity(), FShanmenItemNativeTags::ItemFormationMaterial() }));

		FShanmenItemContainer Container;
		Container.ContainerId = ContainerId;
		Container.RunId = RunId;
		Container.OwnerId = OwnerId;
		Container.ContainerType = TEXT("Warehouse");
		Container.Slots = {
			DartId, SwordId, MirrorId, FormationMaterialId, FGuid() };
		Snapshot.Containers.Add(Container);

		Snapshot.Items.Add(MakeItem(DartId, TEXT("Item.Weapon.ThrowingDart"), 0, 10));
		Snapshot.Items.Add(MakeItem(SwordId, TEXT("Item.Weapon.FlyingSword"), 1, 1, 100));
		Snapshot.Items.Add(MakeItem(MirrorId, TEXT("Item.Artifact.HeartMirror"), 2, 1, 0, 1));
		Snapshot.Items.Add(MakeItem(FormationMaterialId, TEXT("Item.Material.FormationWood"), 3, 3));
		return Snapshot;
	}

	FShanmenItemReserveRequest MakeReserve(
		uint32 Sequence,
		const FGuid& ItemId,
		EShanmenItemResourceKind Kind,
		int32 Amount,
		FName Purpose,
		int32 ExpectedRevision = 0)
	{
		FShanmenItemReserveRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ItemInstanceId = ItemId;
		Request.ResourceKind = Kind;
		Request.Amount = Amount;
		Request.ExpectedItemRevision = ExpectedRevision;
		Request.PurposeId = Purpose;
		return Request;
	}

	FShanmenItemRunResourceCommitRequest MakeRunResourceCommit(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const TArray<FShanmenItemRunResourceCommitLine>& OrderedLines)
	{
		FShanmenItemRunResourceCommitRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ActiveRunId = ActiveRunId;
		Request.OrderedLines = OrderedLines;
		Request.PurposeId = TEXT("Test.Combat.DefenseResources.r1");
		return Request;
	}

	FShanmenItemRunResourceIntentRequest MakeRunResourceIntent(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& IntentId,
		const TArray<FShanmenItemRunResourceCommitLine>& OrderedLines,
		int32 TriggeredLineCount)
	{
		FShanmenItemRunResourceIntentRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ActiveRunId = ActiveRunId;
		Request.IntentId = IntentId;
		Request.OrderedLines = OrderedLines;
		Request.TriggeredLineCount = TriggeredLineCount;
		Request.IntentMetadata = TEXT("Test.ExternalVitalityCAS.r1");
		return Request;
	}

	FShanmenItemRunResourceIntentFinalizeRequest MakeRunResourceIntentFinalize(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& PrepareRequestId,
		const FGuid& IntentId,
		bool bExternalCommitSucceeded)
	{
		FShanmenItemRunResourceIntentFinalizeRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ActiveRunId = ActiveRunId;
		Request.PrepareRequestId = PrepareRequestId;
		Request.IntentId = IntentId;
		Request.bExternalCommitSucceeded = bExternalCommitSucceeded;
		return Request;
	}

	FShanmenItemRunResourceCommitLine ResourceLine(
		const FGuid& ReservationId,
		const FGuid& ItemInstanceId)
	{
		FShanmenItemRunResourceCommitLine Line;
		Line.ReservationId = ReservationId;
		Line.ItemInstanceId = ItemInstanceId;
		return Line;
	}

	FShanmenItemReservationActionRequest MakeAction(
		uint32 Sequence,
		const FGuid& ReservationId)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ReservationId = ReservationId;
		return Request;
	}

	FShanmenItemReservationBatchRequest MakeBatch(
		uint32 Sequence,
		const TArray<FGuid>& ReservationIds)
	{
		FShanmenItemReservationBatchRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ReservationIds = ReservationIds;
		return Request;
	}

	FShanmenItemRunStartRequest MakeRunStart(
		uint32 Sequence,
		const TArray<FGuid>& ReservationIds)
	{
		FShanmenItemRunStartRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ReservationIds = ReservationIds;
		return Request;
	}

	FShanmenItemRunConsumeRequest MakeRunConsume(
		uint32 Sequence,
		const FGuid& ActiveRunId,
		const FGuid& ItemId,
		int32 Amount,
		int32 ExpectedQuantityBefore)
	{
		FShanmenItemRunConsumeRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ActiveRunId = ActiveRunId;
		Request.ItemInstanceId = ItemId;
		Request.Amount = Amount;
		Request.ExpectedQuantityBefore = ExpectedQuantityBefore;
		Request.PurposeId = TEXT("Test.RunItemUse.r1");
		return Request;
	}

	FShanmenItemReservationAmendRequest MakeAmend(
		uint32 Sequence,
		const FGuid& ReservationId,
		FName ExpectedPurposeId,
		FName PurposeId)
	{
		FShanmenItemReservationAmendRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ReservationId = ReservationId;
		Request.ExpectedPurposeId = ExpectedPurposeId;
		Request.PurposeId = PurposeId;
		return Request;
	}

	FShanmenItemRunSecuredOriginal Secured(
		const FGuid& ItemInstanceId,
		int32 RemainingQuantity)
	{
		FShanmenItemRunSecuredOriginal Original;
		Original.ItemInstanceId = ItemInstanceId;
		Original.RemainingQuantity = RemainingQuantity;
		return Original;
	}

	FShanmenItemRunAcquiredItem AcquiredOre(int32 Quantity = 2)
	{
		FShanmenItemRunAcquiredItem Acquired;
		Acquired.ItemInstanceId = AcquiredOreId;
		Acquired.Definition = MakeDefinition(
			TEXT("Item.Material.ExtractedOre"), 10,
			{ FShanmenItemNativeTags::CapabilityConsumeQuantity() });
		Acquired.Quantity = Quantity;
		return Acquired;
	}

	FShanmenItemRunAcquiredItem AcquiredSatchel()
	{
		FShanmenItemRunAcquiredItem Acquired;
		Acquired.ItemInstanceId = AcquiredSatchelId;
		Acquired.Definition = MakeDefinition(
			TEXT("Item.Backpack.ExtractedSatchel"), 1,
			{ FShanmenItemNativeTags::CapabilityDeploy() });
		Acquired.Quantity = 1;
		Acquired.ChildContainerType = TEXT("Backpack");
		Acquired.ChildContainerCapacity = 2;
		return Acquired;
	}

	bool LoadFixture(FAutomationTestBase& Test, FShanmenItemRepository& Repository)
	{
		EShanmenItemTransactionError Error = EShanmenItemTransactionError::None;
		const bool bLoaded = Repository.TryLoadSnapshot(MakeSnapshot(), &Error);
		Test.TestTrue(TEXT("Fixture snapshot loads"), bLoaded);
		Test.TestTrue(TEXT("Fixture has no validation error"), Error == EShanmenItemTransactionError::None);
		return bLoaded;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsSnapshotInvariantTest,
	"Shanmen.0_0_10.Items.SnapshotInvariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsSnapshotInvariantTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository))
	{
		return false;
	}
	TestTrue(TEXT("Loaded repository validates"), Repository.ValidateInvariants());
	TestTrue(TEXT("Canonical capture is stable"), Repository.CaptureSnapshot() == Repository.CaptureSnapshot());

	FShanmenItemAuthoritySnapshot Invalid = MakeSnapshot();
	Invalid.Containers[0].Slots[3] = DartId;
	const FShanmenItemAuthoritySnapshot BeforeRejectedReload = Repository.CaptureSnapshot();
	EShanmenItemTransactionError ExistingError = EShanmenItemTransactionError::None;
	TestFalse(TEXT("Invalid reload is rejected atomically"), Repository.TryLoadSnapshot(Invalid, &ExistingError));
	TestTrue(TEXT("Invalid reload preserves the existing authority"), Repository.CaptureSnapshot() == BeforeRejectedReload);

	FShanmenItemRepository Rejected;
	EShanmenItemTransactionError Error = EShanmenItemTransactionError::None;
	TestFalse(TEXT("Duplicate placement fails closed"), Rejected.TryLoadSnapshot(Invalid, &Error));
	TestTrue(TEXT("Invalid closure reports an invariant error"), Error == EShanmenItemTransactionError::InvariantViolation);
	TestFalse(TEXT("Failed load cannot partially initialize"), Rejected.IsInitialized());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsQuantityCommitTest,
	"Shanmen.0_0_10.Items.QuantityReserveCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsQuantityCommitTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FShanmenItemReserveRequest ReserveRequest = MakeReserve(
		1, DartId, EShanmenItemResourceKind::Quantity, 3, TEXT("Combat.Throw.Dart"));
	const FShanmenItemTransactionReceipt Reserved = Repository.Reserve(ReserveRequest);
	TestTrue(TEXT("Quantity reserve succeeds"), Reserved.IsSuccess());
	TestEqual(TEXT("Reserve does not consume quantity"), Repository.FindItem(DartId)->Quantity, 10);
	TestEqual(TEXT("Reserved quantity is unavailable"), Repository.GetAvailableResource(DartId, EShanmenItemResourceKind::Quantity), 7);
	TestTrue(TEXT("Same reserve request replays exact receipt"), Repository.Reserve(ReserveRequest) == Reserved);

	const FShanmenItemReservationActionRequest CommitRequest = MakeAction(2, Reserved.ReservationId);
	const FShanmenItemTransactionReceipt Committed = Repository.Commit(CommitRequest);
	TestTrue(TEXT("Commit succeeds"), Committed.IsSuccess());
	TestEqual(TEXT("Commit consumes exactly three"), Repository.FindItem(DartId)->Quantity, 7);
	TestEqual(TEXT("Commit advances item revision"), Repository.FindItem(DartId)->Revision, 1);
	TestTrue(TEXT("Commit retry is exactly idempotent"), Repository.Commit(CommitRequest) == Committed);
	TestEqual(TEXT("Retry cannot double consume"), Repository.FindItem(DartId)->Quantity, 7);
	TestTrue(TEXT("Cancel after commit fails closed"), Repository.Cancel(MakeAction(3, Reserved.ReservationId)).Error
		== EShanmenItemTransactionError::ReservationAlreadyCommitted);
	TestTrue(TEXT("Post-commit state validates"), Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsAtomicBatchCommitTest,
	"Shanmen.0_0_10.Items.AtomicBatchCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsAtomicBatchCommitTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FShanmenItemTransactionReceipt Quantity = Repository.Reserve(
		MakeReserve(70, DartId, EShanmenItemResourceKind::Quantity, 3,
			TEXT("Preparation.RunInventory.O00000000.H00")));
	const FShanmenItemTransactionReceipt Equipment = Repository.Reserve(
		MakeReserve(71, SwordId,
			EShanmenItemResourceKind::DeploymentLock, 1,
			TEXT("Preparation.Weapon")));
	TestTrue(TEXT("Batch fixture reservations succeed"),
		Quantity.IsSuccess() && Equipment.IsSuccess());

	const FShanmenItemTransactionReceipt Rejected = Repository.CommitBatch(
		MakeBatch(72, { Quantity.ReservationId, FGuid(9, 9, 9, 9) }));
	TestTrue(TEXT("Missing batch line rejects the whole command"),
		!Rejected.IsSuccess()
			&& Rejected.Error
				== EShanmenItemTransactionError::ReservationNotFound);
	TestEqual(TEXT("Rejected batch consumes no quantity"),
		Repository.FindItem(DartId)->Quantity, 10);
	TestTrue(TEXT("Rejected batch deploys no equipment"),
		Repository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Stored);
	TestTrue(TEXT("Both reservations remain pending after rejection"),
		Repository.FindReservation(Quantity.ReservationId)->State
			== EShanmenItemReservationState::Reserved
		&& Repository.FindReservation(Equipment.ReservationId)->State
			== EShanmenItemReservationState::Reserved);

	const int32 RevisionBefore = Repository.GetAuthorityRevision();
	const FShanmenItemReservationBatchRequest Request = MakeBatch(
		73, { Equipment.ReservationId, Quantity.ReservationId });
	const FShanmenItemTransactionReceipt Committed =
		Repository.CommitBatch(Request);
	TestTrue(TEXT("Valid batch commits"), Committed.IsSuccess()
		&& Committed.Operation
			== EShanmenItemTransactionOperation::CommitBatch
		&& Committed.ReservationIds == Request.ReservationIds);
	TestEqual(TEXT("Whole batch publishes one authority revision"),
		Repository.GetAuthorityRevision(), RevisionBefore + 1);
	TestEqual(TEXT("Batch quantity line consumes exactly once"),
		Repository.FindItem(DartId)->Quantity, 7);
	TestTrue(TEXT("Batch equipment line deploys exactly once"),
		Repository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Deployed);
	TestTrue(TEXT("Exact batch retry replays the aggregate receipt"),
		Repository.CommitBatch(Request) == Committed);
	TestEqual(TEXT("Batch replay cannot double-consume"),
		Repository.FindItem(DartId)->Quantity, 7);

	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Batch state and aggregate ledger survive reload"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
		&& Restarted.CommitBatch(Request) == Committed
		&& Restarted.ValidateInvariants());
	FShanmenItemReservationBatchRequest Conflict = Request;
	Conflict.ReservationIds.Swap(0, 1);
	TestTrue(TEXT("Reordered lines conflict with the same RequestId"),
		Restarted.CommitBatch(Conflict).Error
			== EShanmenItemTransactionError::RequestIdConflict);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsReservationPurposeAmendTest,
	"Shanmen.0_0_10.Items.AtomicReservationPurposeAmend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsReservationPurposeAmendTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FName OriginalPurpose(
		TEXT("Preparation.RunInventory.O00000000.H01"));
	const FName AmendedPurpose(
		TEXT("Preparation.RunInventory.O00000000.H02"));
	const FShanmenItemTransactionReceipt Reserved = Repository.Reserve(
		MakeReserve(80, DartId, EShanmenItemResourceKind::Quantity, 3,
			OriginalPurpose));
	const int32 RevisionBefore = Repository.GetAuthorityRevision();
	const int32 ItemRevisionBefore = Repository.FindItem(DartId)->Revision;
	const FShanmenItemReservationAmendRequest Request = MakeAmend(
		81, Reserved.ReservationId, OriginalPurpose, AmendedPurpose);
	const FShanmenItemTransactionReceipt Amended =
		Repository.AmendReservationPurpose(Request);
	TestTrue(TEXT("Pending reservation Purpose amends atomically"),
		Amended.IsSuccess()
		&& Amended.Operation
			== EShanmenItemTransactionOperation::AmendReservationPurpose
		&& Amended.ReservationId == Reserved.ReservationId
		&& Amended.PurposeId == AmendedPurpose
		&& Repository.FindReservation(Reserved.ReservationId)->PurposeId
			== AmendedPurpose);
	TestTrue(TEXT("Purpose amend changes no resource or item revision"),
		Repository.GetAuthorityRevision() == RevisionBefore + 1
		&& Repository.FindItem(DartId)->Quantity == 10
		&& Repository.FindItem(DartId)->Revision == ItemRevisionBefore
		&& Repository.GetAvailableResource(
			DartId, EShanmenItemResourceKind::Quantity) == 7);
	TestTrue(TEXT("Exact Purpose amend retry replays one receipt"),
		Repository.AmendReservationPurpose(Request) == Amended);

	FShanmenItemReservationAmendRequest Conflict = Request;
	Conflict.PurposeId = TEXT("Preparation.RunInventory.O00000000.H03");
	TestTrue(TEXT("Same RequestId cannot amend to another Purpose"),
		Repository.AmendReservationPurpose(Conflict).Error
			== EShanmenItemTransactionError::RequestIdConflict);
	TestTrue(TEXT("Compare-and-swap rejects a stale expected Purpose"),
		Repository.AmendReservationPurpose(MakeAmend(
			82, Reserved.ReservationId, OriginalPurpose,
			TEXT("Preparation.RunInventory.O00000000.H04"))).Error
			== EShanmenItemTransactionError::ReservationPurposeMismatch
		&& Repository.FindReservation(Reserved.ReservationId)->PurposeId
			== AmendedPurpose);

	const FShanmenItemReservationBatchRequest Batch = MakeBatch(
		83, { Reserved.ReservationId });
	TestTrue(TEXT("Amended reservation remains batch-committable"),
		Repository.CommitBatch(Batch).IsSuccess()
		&& Repository.FindItem(DartId)->Quantity == 7);
	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Amend ledger survives terminal commit and restart"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
		&& Restarted.AmendReservationPurpose(Request) == Amended
		&& Restarted.CommitBatch(Batch).IsSuccess()
		&& Restarted.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsAtomicPreparedRunStartTest,
	"Shanmen.0_0_10.Items.AtomicPreparedRunStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsAtomicPreparedRunStartTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FName RecoverablePurpose =
		FShanmenItemReservationPlacement::Encode(
			TEXT("Preparation.RunInventory.O00000000.H00"),
			ContainerId, 0);
	const FShanmenItemTransactionReceipt Equipment = Repository.Reserve(
		MakeReserve(86, SwordId,
			EShanmenItemResourceKind::DeploymentLock, 1,
			TEXT("Preparation.Weapon")));
	const FShanmenItemTransactionReceipt Quantity = Repository.Reserve(
		MakeReserve(87, DartId, EShanmenItemResourceKind::Quantity, 10,
			RecoverablePurpose));
	FShanmenItemRunStartRequest StartRequest = MakeRunStart(
		88, { Equipment.ReservationId, Quantity.ReservationId });
	const int32 RevisionBeforeStart = Repository.GetAuthorityRevision();
	const FShanmenItemTransactionReceipt Started =
		Repository.StartPreparedRun(StartRequest);
	TestTrue(TEXT("One command commits resources and publishes ActiveRunId"),
		Equipment.IsSuccess() && Quantity.IsSuccess()
		&& Started.IsSuccess()
		&& Started.Operation
			== EShanmenItemTransactionOperation::StartPreparedRun
		&& Started.ItemInstanceId == StartRequest.Context.RequestId
		&& Started.ReservationId.IsValid()
		&& Repository.GetAuthorityRevision() == RevisionBeforeStart + 1
		&& Repository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Deployed
		&& Repository.FindItem(DartId)->State
			== EShanmenItemInstanceState::Depleted);

	int32 AtomicStartCount = 0;
	int32 LegacyBatchOrClaimCount = 0;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Repository.CaptureSnapshot().ProcessedRequests)
	{
		AtomicStartCount += Processed.Receipt.IsSuccess()
			&& Processed.Receipt.Operation
				== EShanmenItemTransactionOperation::StartPreparedRun ? 1 : 0;
		LegacyBatchOrClaimCount += Processed.Receipt.IsSuccess()
			&& (Processed.Receipt.Operation
					== EShanmenItemTransactionOperation::CommitBatch
				|| Processed.Receipt.Operation
					== EShanmenItemTransactionOperation::ClaimPreparedRun) ? 1 : 0;
	}
	TestTrue(TEXT("Atomic start writes no intermediate batch or claim marker"),
		AtomicStartCount == 1 && LegacyBatchOrClaimCount == 0
		&& Repository.StartPreparedRun(StartRequest) == Started);

	FShanmenItemRunStartRequest Conflict = StartRequest;
	Swap(Conflict.ReservationIds[0], Conflict.ReservationIds[1]);
	const int32 RevisionBeforeConflict = Repository.GetAuthorityRevision();
	TestTrue(TEXT("Same start RequestId with another ordered plan conflicts without mutation"),
		Repository.StartPreparedRun(Conflict).Error
			== EShanmenItemTransactionError::RequestIdConflict
		&& Repository.GetAuthorityRevision() == RevisionBeforeConflict);

	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Atomic start survives restart and replays exactly"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
		&& Restarted.StartPreparedRun(StartRequest) == Started);

	FShanmenItemRunFinalizeRequest Finalize;
	Finalize.Context = MakeContext(89);
	Finalize.ActiveRunId = Started.ReservationId;
	Finalize.TerminalReason = EShanmenItemRunTerminalReason::Extraction;
	Finalize.SecuredOriginals = {
		Secured(SwordId, 1), Secured(DartId, 7) };
	const FShanmenItemTransactionReceipt Finalized =
		Restarted.FinalizePreparedRun(Finalize);
	TestTrue(TEXT("Atomic-start receipt is accepted by terminal reconciliation"),
		Finalized.IsSuccess()
		&& Restarted.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Stored
		&& Restarted.FindItem(DartId)->State
			== EShanmenItemInstanceState::Stored
		&& Restarted.FindItem(DartId)->Quantity == 7
		&& Restarted.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsPreparedRunConsumptionTest,
	"Shanmen.0_0_10.Items.PreparedRunConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsPreparedRunConsumptionTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FName RecoverablePurpose =
		FShanmenItemReservationPlacement::Encode(
			TEXT("Preparation.RunInventory.O00000000.H01"),
			ContainerId, 0);
	const FShanmenItemTransactionReceipt Reserved = Repository.Reserve(
		MakeReserve(300, DartId, EShanmenItemResourceKind::Quantity, 10,
			RecoverablePurpose));
	const FShanmenItemTransactionReceipt Started =
		Repository.StartPreparedRun(
			MakeRunStart(301, { Reserved.ReservationId }));
	if (!Reserved.IsSuccess() || !Started.IsSuccess())
	{
		AddError(TEXT("Prepared consumption fixture could not start its Run."));
		return false;
	}

	const FShanmenItemRunConsumeRequest FirstRequest = MakeRunConsume(
		302, Started.ReservationId, DartId, 1, 10);
	const FShanmenItemTransactionReceipt First =
		Repository.ConsumePreparedRunItem(FirstRequest);
	TestTrue(TEXT("First use appends one exact active-Run balance receipt"),
		First.IsSuccess()
		&& First.Operation
			== EShanmenItemTransactionOperation::ConsumePreparedRunItem
		&& First.ReservationId == Started.ReservationId
		&& First.ItemInstanceId == DartId
		&& First.ResourceBefore == 10
		&& First.ResourceAfter == 9
		&& First.AvailableAfter == 9
		&& Repository.ConsumePreparedRunItem(FirstRequest) == First);

	FShanmenItemRunConsumeRequest RequestConflict = FirstRequest;
	RequestConflict.PurposeId = TEXT("Test.RunItemUse.Changed");
	const int32 RevisionBeforeConflict = Repository.GetAuthorityRevision();
	TestTrue(TEXT("Same RequestId with another use payload conflicts without mutation"),
		Repository.ConsumePreparedRunItem(RequestConflict).Error
			== EShanmenItemTransactionError::RequestIdConflict
		&& Repository.GetAuthorityRevision() == RevisionBeforeConflict);
	TestTrue(TEXT("Stale Runtime quantity is rejected explicitly"),
		Repository.ConsumePreparedRunItem(MakeRunConsume(
			303, Started.ReservationId, DartId, 1, 10)).Error
			== EShanmenItemTransactionError::RunItemQuantityConflict);
	TestTrue(TEXT("Foreign ActiveRun cannot consume a prepared identity"),
		Repository.ConsumePreparedRunItem(MakeRunConsume(
			304, FGuid(999, 0, 0, 1), DartId, 1, 9)).Error
			== EShanmenItemTransactionError::RunNotFound);

	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Consumption chain survives restart and exact replay"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
		&& Restarted.ConsumePreparedRunItem(FirstRequest) == First
		&& Restarted.ValidateInvariants());
	FShanmenItemRunFinalizeRequest Overclaim;
	Overclaim.Context = MakeContext(305);
	Overclaim.ActiveRunId = Started.ReservationId;
	Overclaim.TerminalReason = EShanmenItemRunTerminalReason::Extraction;
	Overclaim.SecuredOriginals = { Secured(DartId, 10) };
	TestTrue(TEXT("Settlement cannot restore a durably consumed unit"),
		Restarted.FinalizePreparedRun(Overclaim).Error
			== EShanmenItemTransactionError::SecuredItemMismatch);
	FShanmenItemRunFinalizeRequest Exact = Overclaim;
	Exact.Context = MakeContext(306);
	Exact.SecuredOriginals = { Secured(DartId, 9) };
	const FShanmenItemTransactionReceipt Finalized =
		Restarted.FinalizePreparedRun(Exact);
	TestTrue(TEXT("Exact remaining balance finalizes and remains reload-valid"),
		Finalized.IsSuccess()
		&& Restarted.FindItem(DartId)->Quantity == 9
		&& Restarted.ValidateInvariants());
	FShanmenItemRepository TerminalRestart;
	TestTrue(TEXT("Terminal consumption history reloads without resurrecting quantity"),
		TerminalRestart.TryLoadSnapshot(Restarted.CaptureSnapshot())
		&& TerminalRestart.FindItem(DartId)->Quantity == 9
		&& TerminalRestart.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsPreparedRunLifecycleLedgerTest,
	"Shanmen.0_0_10.Items.PreparedRunLifecycleLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsPreparedRunLifecycleLedgerTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FName RecoverablePurpose =
		FShanmenItemReservationPlacement::Encode(
			TEXT("Preparation.RunInventory.O00000000.H00"),
			ContainerId, 0);
	const FShanmenItemTransactionReceipt Equipment = Repository.Reserve(
		MakeReserve(90, SwordId,
			EShanmenItemResourceKind::DeploymentLock, 1,
			TEXT("Preparation.Weapon")));
	const FShanmenItemTransactionReceipt Quantity = Repository.Reserve(
		MakeReserve(91, DartId, EShanmenItemResourceKind::Quantity, 10,
			RecoverablePurpose));
	FShanmenItemReservationBatchRequest Batch = MakeBatch(
		92, { Equipment.ReservationId, Quantity.ReservationId });
	const FShanmenItemTransactionReceipt Committed =
		Repository.CommitBatch(Batch);
	TestTrue(TEXT("Prepared originals commit as one batch"),
		Equipment.IsSuccess() && Quantity.IsSuccess()
		&& Committed.IsSuccess()
		&& Repository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Deployed
		&& Repository.FindItem(DartId)->State
			== EShanmenItemInstanceState::Depleted);

	FShanmenItemRunClaimRequest ClaimRequest;
	ClaimRequest.Context = MakeContext(93);
	ClaimRequest.PreparedBatchRequestId = Batch.Context.RequestId;
	const int32 RevisionBeforeClaim = Repository.GetAuthorityRevision();
	const FShanmenItemTransactionReceipt Claim =
		Repository.ClaimPreparedRun(ClaimRequest);
	TestTrue(TEXT("Claim publishes one deterministic active-Run marker"),
		Claim.IsSuccess()
		&& Claim.Operation
			== EShanmenItemTransactionOperation::ClaimPreparedRun
		&& Claim.ReservationId.IsValid()
		&& Repository.GetAuthorityRevision() == RevisionBeforeClaim + 1
		&& Repository.ClaimPreparedRun(ClaimRequest) == Claim);

	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Active claim survives restart and replays exactly"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
		&& Restarted.ClaimPreparedRun(ClaimRequest) == Claim);
	FShanmenItemAuthoritySnapshot ActiveSnapshot =
		Restarted.CaptureSnapshot();
	FShanmenItemContainer ForeignWarehouse;
	ForeignWarehouse.ContainerId = ForeignContainerId;
	ForeignWarehouse.RunId = ForeignRunId;
	ForeignWarehouse.OwnerId = ForeignOwnerId;
	ForeignWarehouse.ContainerType = TEXT("Warehouse");
	ForeignWarehouse.Slots = { FGuid() };
	ActiveSnapshot.Containers.Add(ForeignWarehouse);
	FShanmenItemAuthoritySnapshot DestructiveSnapshot = ActiveSnapshot;
	FShanmenItemInstance* DestructiveSword =
		DestructiveSnapshot.Items.FindByPredicate(
			[](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == SwordId;
			});
	if (DestructiveSword)
	{
		DestructiveSword->ChildContainerId = RecoveredChildContainerId;
	}
	FShanmenItemContainer SafeChild;
	SafeChild.ContainerId = RecoveredChildContainerId;
	SafeChild.RunId = RunId;
	SafeChild.OwnerId = OwnerId;
	SafeChild.ContainerType = TEXT("Backpack");
	SafeChild.Slots = { SafeChildItemId };
	DestructiveSnapshot.Containers.Add(SafeChild);
	FShanmenItemInstance SafeItem = MakeItem(
		SafeChildItemId, TEXT("Item.Material.FormationWood"), 0, 1);
	SafeItem.ParentContainerId = RecoveredChildContainerId;
	DestructiveSnapshot.Items.Add(SafeItem);

	FShanmenItemRepository DeathRepository;
	FShanmenItemRunFinalizeRequest Death;
	Death.Context = MakeContext(94);
	Death.ActiveRunId = Claim.ReservationId;
	Death.TerminalReason = EShanmenItemRunTerminalReason::Death;
	const FShanmenItemTransactionReceipt DeathReceipt =
		DeathRepository.TryLoadSnapshot(DestructiveSnapshot)
		? DeathRepository.FinalizePreparedRun(Death)
		: FShanmenItemTransactionReceipt();
	const FShanmenItemContainer* DeathContainer =
		DeathRepository.FindContainer(ContainerId);
	TestTrue(TEXT("Death atomically destroys every prepared identity"),
		DeathReceipt.IsSuccess()
		&& DeathReceipt.PurposeId
			== FShanmenItemRunLifecyclePurpose::Death()
		&& DeathRepository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Destroyed
		&& DeathRepository.FindItem(DartId)->State
			== EShanmenItemInstanceState::Destroyed
		&& DeathRepository.FindItem(SafeChildItemId)->State
			== EShanmenItemInstanceState::Stored
		&& DeathRepository.FindContainer(RecoveredChildContainerId)
		&& DeathRepository.FindContainer(RecoveredChildContainerId)->ContainerType
			== FShanmenItemRunLifecyclePurpose::RecoveredStorage()
		&& DeathContainer && !DeathContainer->Slots[0].IsValid()
		&& !DeathContainer->Slots[1].IsValid()
		&& DeathRepository.FindReservation(Equipment.ReservationId)->State
			== EShanmenItemReservationState::Released
		&& DeathRepository.FindReservation(Quantity.ReservationId)->State
			== EShanmenItemReservationState::Released
		&& DeathRepository.ValidateInvariants()
		&& DeathRepository.FinalizePreparedRun(Death) == DeathReceipt);

	FShanmenItemRepository AbandonRepository;
	FShanmenItemRunFinalizeRequest Abandon = Death;
	Abandon.Context = MakeContext(95);
	Abandon.TerminalReason = EShanmenItemRunTerminalReason::Abandon;
	const FShanmenItemTransactionReceipt AbandonReceipt =
		AbandonRepository.TryLoadSnapshot(DestructiveSnapshot)
		? AbandonRepository.FinalizePreparedRun(Abandon)
		: FShanmenItemTransactionReceipt();
	TestTrue(TEXT("Player abandon uses the same loss policy with its own marker"),
		AbandonReceipt.IsSuccess()
		&& AbandonReceipt.PurposeId
			== FShanmenItemRunLifecyclePurpose::Abandon()
		&& AbandonRepository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Destroyed
		&& AbandonRepository.ValidateInvariants());

	FShanmenItemRunFinalizeRequest MissingEquipment;
	MissingEquipment.Context = MakeContext(96);
	MissingEquipment.ActiveRunId = Claim.ReservationId;
	MissingEquipment.TerminalReason =
		EShanmenItemRunTerminalReason::Extraction;
	MissingEquipment.SecuredOriginals = { Secured(DartId, 7) };
	TestTrue(TEXT("Extraction cannot silently lose deployed equipment"),
		Restarted.FinalizePreparedRun(MissingEquipment).Error
			== EShanmenItemTransactionError::SecuredItemMismatch);

	FShanmenItemAuthoritySnapshot FullWarehouseSnapshot = ActiveSnapshot;
	FShanmenItemContainer* FullWarehouse =
		FullWarehouseSnapshot.Containers.FindByPredicate(
			[](const FShanmenItemContainer& Candidate)
			{
				return Candidate.ContainerId == ContainerId;
			});
	FShanmenItemInstance CapacityBlocker = MakeItem(
		SafeChildItemId, TEXT("Item.Material.FormationWood"), 4, 1);
	if (FullWarehouse && FullWarehouse->Slots.IsValidIndex(4))
	{
		FullWarehouse->Slots[4] = SafeChildItemId;
		FullWarehouseSnapshot.Items.Add(CapacityBlocker);
	}
	FShanmenItemRepository CapacityRepository;
	FShanmenItemRunFinalizeRequest NoCapacity;
	NoCapacity.Context = MakeContext(97);
	NoCapacity.ActiveRunId = Claim.ReservationId;
	NoCapacity.TerminalReason = EShanmenItemRunTerminalReason::Extraction;
	NoCapacity.SecuredOriginals = {
		Secured(SwordId, 1), Secured(DartId, 7) };
	NoCapacity.AcquiredItems = { AcquiredOre() };
	const FShanmenItemTransactionReceipt CapacityRejected =
		CapacityRepository.TryLoadSnapshot(FullWarehouseSnapshot)
		? CapacityRepository.FinalizePreparedRun(NoCapacity)
		: FShanmenItemTransactionReceipt();
	TestTrue(TEXT("Extraction capacity failure imports nothing and restores nothing"),
		CapacityRejected.Error
			== EShanmenItemTransactionError::ImportPlacementUnavailable
		&& !CapacityRepository.FindItem(AcquiredOreId)
		&& CapacityRepository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Deployed
		&& CapacityRepository.FindItem(DartId)->State
			== EShanmenItemInstanceState::Depleted
		&& CapacityRepository.ValidateInvariants());

	FShanmenItemRunFinalizeRequest Finalize;
	Finalize.Context = MakeContext(98);
	Finalize.ActiveRunId = Claim.ReservationId;
	Finalize.TerminalReason = EShanmenItemRunTerminalReason::Extraction;
	Finalize.SecuredOriginals = {
		Secured(SwordId, 1), Secured(DartId, 7) };
	Finalize.AcquiredItems = { AcquiredSatchel() };
	FShanmenItemRepository ExtractionRepository;
	TestTrue(TEXT("Extraction authority accepts unrelated owner storage"),
		ExtractionRepository.TryLoadSnapshot(ActiveSnapshot));
	const int32 RevisionBeforeFinalize =
		ExtractionRepository.GetAuthorityRevision();
	const FShanmenItemTransactionReceipt Finalized =
		ExtractionRepository.FinalizePreparedRun(Finalize);
	const FShanmenItemContainer* Container =
		ExtractionRepository.FindContainer(ContainerId);
	TestTrue(TEXT("Extraction atomically restores originals and closes the claim"),
		Finalized.IsSuccess()
		&& Finalized.Operation
			== EShanmenItemTransactionOperation::FinalizePreparedRun
		&& ExtractionRepository.GetAuthorityRevision()
			== RevisionBeforeFinalize + 1
		&& ExtractionRepository.FindItem(SwordId)->State
			== EShanmenItemInstanceState::Stored
		&& !ExtractionRepository.FindItem(
			SwordId)->DeploymentReservationId.IsValid()
		&& ExtractionRepository.FindItem(DartId)->State
			== EShanmenItemInstanceState::Stored
		&& ExtractionRepository.FindItem(DartId)->Quantity == 7
		&& Container && Container->Slots[0] == DartId
		&& Container->Slots[4] == AcquiredSatchelId
		&& ExtractionRepository.FindItem(AcquiredSatchelId)
		&& ExtractionRepository.FindItem(AcquiredSatchelId)->DefinitionId
			== TEXT("Item.Backpack.ExtractedSatchel")
		&& ExtractionRepository.FindItem(
			AcquiredSatchelId)->ChildContainerId.IsValid()
		&& ExtractionRepository.FindContainer(
			ExtractionRepository.FindItem(
				AcquiredSatchelId)->ChildContainerId)
		&& ExtractionRepository.FindContainer(
			ExtractionRepository.FindItem(
				AcquiredSatchelId)->ChildContainerId)->Slots.Num() == 2
		&& ExtractionRepository.FindDefinition(
			TEXT("Item.Backpack.ExtractedSatchel"))
		&& ExtractionRepository.FindReservation(
			Equipment.ReservationId)->State
			== EShanmenItemReservationState::Released
		&& ExtractionRepository.FindReservation(
			Quantity.ReservationId)->State
			== EShanmenItemReservationState::Released
		&& ExtractionRepository.ValidateInvariants());
	TestTrue(TEXT("Terminal retry replays without another mutation"),
		ExtractionRepository.FinalizePreparedRun(Finalize) == Finalized);

	FShanmenItemRepository TerminalRestart;
	TestTrue(TEXT("Terminal marker and restored graph survive restart"),
		TerminalRestart.TryLoadSnapshot(
			ExtractionRepository.CaptureSnapshot())
		&& TerminalRestart.FinalizePreparedRun(Finalize) == Finalized
		&& TerminalRestart.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsCancelAndConflictTest,
	"Shanmen.0_0_10.Items.CancelAndRequestConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsCancelAndConflictTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	FShanmenItemReserveRequest ReserveRequest = MakeReserve(
		10, FormationMaterialId, EShanmenItemResourceKind::Quantity, 2, TEXT("Formation.Anchor.01"));
	const FShanmenItemTransactionReceipt Reserved = Repository.Reserve(ReserveRequest);
	const int32 RevisionAfterReserve = Repository.GetAuthorityRevision();

	FShanmenItemReserveRequest Conflicting = ReserveRequest;
	Conflicting.Amount = 1;
	const FShanmenItemTransactionReceipt Conflict = Repository.Reserve(Conflicting);
	TestFalse(TEXT("Same RequestId with different payload is rejected"), Conflict.IsSuccess());
	TestTrue(TEXT("Conflict has explicit code"), Conflict.Error == EShanmenItemTransactionError::RequestIdConflict);
	TestEqual(TEXT("Conflict does not mutate authority revision"), Repository.GetAuthorityRevision(), RevisionAfterReserve);
	TestTrue(TEXT("Original command remains replayable"), Repository.Reserve(ReserveRequest) == Reserved);

	const FShanmenItemReservationActionRequest CancelRequest = MakeAction(11, Reserved.ReservationId);
	const FShanmenItemTransactionReceipt Cancelled = Repository.Cancel(CancelRequest);
	TestTrue(TEXT("Cancellation succeeds"), Cancelled.IsSuccess());
	TestEqual(TEXT("Cancellation restores all availability"), Repository.GetAvailableResource(FormationMaterialId, EShanmenItemResourceKind::Quantity), 3);
	TestEqual(TEXT("Cancellation does not consume quantity"), Repository.FindItem(FormationMaterialId)->Quantity, 3);
	TestTrue(TEXT("Cancellation retry replays exact receipt"), Repository.Cancel(CancelRequest) == Cancelled);

	const FShanmenItemTransactionReceipt CommitAfterCancel = Repository.Commit(MakeAction(12, Reserved.ReservationId));
	TestTrue(TEXT("Commit after cancel fails closed"), CommitAfterCancel.Error == EShanmenItemTransactionError::ReservationAlreadyCancelled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsConcurrentReservationTest,
	"Shanmen.0_0_10.Items.ConcurrentReservations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsConcurrentReservationTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FShanmenItemTransactionReceipt Six = Repository.Reserve(MakeReserve(
		20, DartId, EShanmenItemResourceKind::Quantity, 6, TEXT("Combat.Throw.VolleyA")));
	const FShanmenItemTransactionReceipt TooMany = Repository.Reserve(MakeReserve(
		21, DartId, EShanmenItemResourceKind::Quantity, 5, TEXT("Combat.Throw.VolleyB")));
	const FShanmenItemTransactionReceipt Four = Repository.Reserve(MakeReserve(
		22, DartId, EShanmenItemResourceKind::Quantity, 4, TEXT("Combat.Throw.VolleyC")));
	TestTrue(TEXT("First reservation succeeds"), Six.IsSuccess());
	TestTrue(TEXT("Over-reservation fails"), TooMany.Error == EShanmenItemTransactionError::InsufficientResource);
	TestTrue(TEXT("Exact remaining quantity can be reserved"), Four.IsSuccess());
	TestEqual(TEXT("All quantity is now reserved"), Repository.GetAvailableResource(DartId, EShanmenItemResourceKind::Quantity), 0);

	TestTrue(TEXT("First commit succeeds"), Repository.Commit(MakeAction(23, Six.ReservationId)).IsSuccess());
	TestEqual(TEXT("First commit leaves four"), Repository.FindItem(DartId)->Quantity, 4);
	TestTrue(TEXT("Second reservation survives item revision advancement"), Repository.Commit(MakeAction(24, Four.ReservationId)).IsSuccess());
	const FShanmenItemInstance* Depleted = Repository.FindItem(DartId);
	TestTrue(TEXT("Fully consumed identity remains as an audit tombstone"), Depleted && Depleted->State == EShanmenItemInstanceState::Depleted);
	TestEqual(TEXT("Tombstone quantity is zero"), Depleted->Quantity, 0);
	TestFalse(TEXT("Depleted item is removed from its slot"), Repository.FindContainer(ContainerId)->Slots[0].IsValid());
	TestTrue(TEXT("Depleted graph validates"), Repository.ValidateInvariants());
	FShanmenItemRepository ReloadedDepleted;
	TestTrue(TEXT("Depleted audit tombstone survives persistence reload"), ReloadedDepleted.TryLoadSnapshot(Repository.CaptureSnapshot()));
	TestTrue(TEXT("Reloaded tombstone remains depleted"), ReloadedDepleted.FindItem(DartId)->State == EShanmenItemInstanceState::Depleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsPreparedRunDefenseResourceCommitTest,
	"Shanmen.0_0_10.Items.PreparedRunDefenseResourceCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsPreparedRunDefenseResourceCommitTest::RunTest(
	const FString&)
{
	auto StartFixture = [this](
		FShanmenItemRepository& Repository,
		FShanmenItemTransactionReceipt& OutRun,
		FShanmenItemTransactionReceipt& OutDurability,
		FShanmenItemTransactionReceipt& OutCharge)
	{
		if (!LoadFixture(*this, Repository))
		{
			return false;
		}
		const FShanmenItemTransactionReceipt SwordDeployment =
			Repository.Reserve(MakeReserve(
				300, SwordId, EShanmenItemResourceKind::DeploymentLock,
				1, TEXT("Preparation.Weapon")));
		const FShanmenItemTransactionReceipt MirrorDeployment =
			Repository.Reserve(MakeReserve(
				301, MirrorId, EShanmenItemResourceKind::DeploymentLock,
				1, TEXT("Preparation.Accessory")));
		OutRun = Repository.StartPreparedRun(MakeRunStart(
			302,
			{ SwordDeployment.ReservationId,
				MirrorDeployment.ReservationId }));
		OutDurability = Repository.Reserve(MakeReserve(
			303, SwordId, EShanmenItemResourceKind::Durability,
			2, TEXT("Combat.FlyingSword.Impact"), 1));
		OutCharge = Repository.Reserve(MakeReserve(
			304, MirrorId, EShanmenItemResourceKind::Charges,
			1, TEXT("Defense.Artifact.HeartMirror"), 1));
		return SwordDeployment.IsSuccess()
			&& MirrorDeployment.IsSuccess() && OutRun.IsSuccess()
			&& OutDurability.IsSuccess() && OutCharge.IsSuccess();
	};

	FShanmenItemRepository Repository;
	FShanmenItemTransactionReceipt ActiveRun;
	FShanmenItemTransactionReceipt Durability;
	FShanmenItemTransactionReceipt Charge;
	if (!StartFixture(Repository, ActiveRun, Durability, Charge))
	{
		AddError(TEXT("Prepared defense-resource fixture failed to start."));
		return false;
	}
	const FShanmenItemRunResourceCommitRequest Request =
		MakeRunResourceCommit(
			305, ActiveRun.ReservationId,
			{
				ResourceLine(Durability.ReservationId, SwordId),
				ResourceLine(Charge.ReservationId, MirrorId)
			});
	const int32 RevisionBefore = Repository.GetAuthorityRevision();
	const FShanmenItemTransactionReceipt Committed =
		Repository.CommitPreparedRunResources(Request);
	TestTrue(TEXT("One impact commits all triggered resources atomically"),
		Committed.IsSuccess()
			&& Committed.Operation
				== EShanmenItemTransactionOperation::CommitPreparedRunResources
			&& Committed.ReservationId == ActiveRun.ReservationId
			&& Committed.ReservationIds
				== TArray<FGuid>({
					Durability.ReservationId, Charge.ReservationId })
			&& Repository.GetAuthorityRevision() == RevisionBefore + 1
			&& Repository.FindItem(SwordId)->Durability == 98
			&& Repository.FindItem(MirrorId)->Charges == 0
			&& Repository.FindReservation(Durability.ReservationId)->State
				== EShanmenItemReservationState::Committed
			&& Repository.FindReservation(Charge.ReservationId)->State
				== EShanmenItemReservationState::Committed
			&& Repository.ValidateInvariants());

	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Restart replays the exact impact commit without double spend"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
			&& Restarted.CommitPreparedRunResources(Request) == Committed
			&& Restarted.FindItem(SwordId)->Durability == 98
			&& Restarted.FindItem(MirrorId)->Charges == 0);

	FShanmenItemRunFinalizeRequest Finalize;
	Finalize.Context = MakeContext(306);
	Finalize.ActiveRunId = ActiveRun.ReservationId;
	Finalize.TerminalReason = EShanmenItemRunTerminalReason::Extraction;
	Finalize.SecuredOriginals = {
		Secured(SwordId, 1), Secured(MirrorId, 1) };
	const FShanmenItemTransactionReceipt Finalized =
		Restarted.FinalizePreparedRun(Finalize);
	FShanmenItemRunResourceCommitRequest AfterTerminal = Request;
	AfterTerminal.Context = MakeContext(307);
	TestTrue(TEXT("Extraction preserves wear and closes new impact commits"),
		Finalized.IsSuccess()
			&& Restarted.FindItem(SwordId)->State
				== EShanmenItemInstanceState::Stored
			&& Restarted.FindItem(SwordId)->Durability == 98
			&& Restarted.FindItem(MirrorId)->Charges == 0
			&& Restarted.CommitPreparedRunResources(AfterTerminal).Error
				== EShanmenItemTransactionError::RunAlreadyFinalized);

	FShanmenItemRepository AtomicFailure;
	FShanmenItemTransactionReceipt FailedRun;
	FShanmenItemTransactionReceipt PendingDurability;
	FShanmenItemTransactionReceipt PendingCharge;
	if (!StartFixture(
			AtomicFailure, FailedRun, PendingDurability, PendingCharge))
	{
		AddError(TEXT("Atomic rejection fixture failed to start."));
		return false;
	}
	const FShanmenItemRunResourceCommitRequest Mismatched =
		MakeRunResourceCommit(
			308, FailedRun.ReservationId,
			{
				ResourceLine(PendingDurability.ReservationId, SwordId),
				ResourceLine(PendingCharge.ReservationId, SwordId)
			});
	const FShanmenItemTransactionReceipt Rejected =
		AtomicFailure.CommitPreparedRunResources(Mismatched);
	TestTrue(TEXT("One mismatched source rejects every line without partial wear"),
		!Rejected.IsSuccess()
			&& Rejected.Error
				== EShanmenItemTransactionError::SecuredItemMismatch
			&& AtomicFailure.FindItem(SwordId)->Durability == 100
			&& AtomicFailure.FindItem(MirrorId)->Charges == 1
			&& AtomicFailure.FindReservation(
				PendingDurability.ReservationId)->State
				== EShanmenItemReservationState::Reserved
			&& AtomicFailure.FindReservation(PendingCharge.ReservationId)->State
				== EShanmenItemReservationState::Reserved
			&& AtomicFailure.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsPreparedRunDefenseResourceIntentTest,
	"Shanmen.0_0_10.Items.PreparedRunDefenseResourceIntentRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsPreparedRunDefenseResourceIntentTest::RunTest(
	const FString&)
{
	auto StartFixture = [this](
		FShanmenItemRepository& Repository,
		FShanmenItemTransactionReceipt& OutRun,
		FShanmenItemTransactionReceipt& OutDurability,
		FShanmenItemTransactionReceipt& OutCharge)
	{
		if (!LoadFixture(*this, Repository))
		{
			return false;
		}
		const FShanmenItemTransactionReceipt SwordDeployment =
			Repository.Reserve(MakeReserve(
				400, SwordId, EShanmenItemResourceKind::DeploymentLock,
				1, TEXT("Preparation.Weapon")));
		const FShanmenItemTransactionReceipt MirrorDeployment =
			Repository.Reserve(MakeReserve(
				401, MirrorId, EShanmenItemResourceKind::DeploymentLock,
				1, TEXT("Preparation.Accessory")));
		OutRun = Repository.StartPreparedRun(MakeRunStart(
			402,
			{ SwordDeployment.ReservationId,
				MirrorDeployment.ReservationId }));
		OutDurability = Repository.Reserve(MakeReserve(
			403, SwordId, EShanmenItemResourceKind::Durability,
			2, TEXT("Combat.FlyingSword.Impact"), 1));
		OutCharge = Repository.Reserve(MakeReserve(
			404, MirrorId, EShanmenItemResourceKind::Charges,
			1, TEXT("Defense.Artifact.HeartMirror"), 1));
		return SwordDeployment.IsSuccess()
			&& MirrorDeployment.IsSuccess() && OutRun.IsSuccess()
			&& OutDurability.IsSuccess() && OutCharge.IsSuccess();
	};

	FShanmenItemRepository Repository;
	FShanmenItemTransactionReceipt ActiveRun;
	FShanmenItemTransactionReceipt Durability;
	FShanmenItemTransactionReceipt Charge;
	if (!StartFixture(Repository, ActiveRun, Durability, Charge))
	{
		AddError(TEXT("Prepared resource-intent fixture failed to start."));
		return false;
	}

	const FGuid IntentId(0x0A001000, 0, 0, 1);
	const FShanmenItemRunResourceIntentRequest PrepareRequest =
		MakeRunResourceIntent(
			405, ActiveRun.ReservationId, IntentId,
			{
				ResourceLine(Durability.ReservationId, SwordId),
				ResourceLine(Charge.ReservationId, MirrorId)
			},
			1);
	const int32 RevisionBeforePrepare = Repository.GetAuthorityRevision();
	const FShanmenItemTransactionReceipt Prepared =
		Repository.PreparePreparedRunResourceIntent(PrepareRequest);
	TestTrue(TEXT("Prepare durably freezes one triggered prefix without spend"),
		Prepared.IsSuccess()
			&& Prepared.Operation
				== EShanmenItemTransactionOperation::PreparePreparedRunResourceIntent
			&& Prepared.Phase == EShanmenItemTransactionPhase::Reserved
			&& Prepared.ReservationId == IntentId
			&& Prepared.ItemInstanceId == ActiveRun.ReservationId
			&& Prepared.Amount == 1
			&& Prepared.ReservationIds
				== TArray<FGuid>({
					Durability.ReservationId, Charge.ReservationId })
			&& Repository.GetAuthorityRevision() == RevisionBeforePrepare + 1
			&& Repository.FindItem(SwordId)->Durability == 100
			&& Repository.FindItem(MirrorId)->Charges == 1
			&& Repository.FindReservation(Durability.ReservationId)->State
				== EShanmenItemReservationState::Reserved
			&& Repository.FindReservation(Charge.ReservationId)->State
				== EShanmenItemReservationState::Reserved
			&& Repository.FindReservation(Durability.ReservationId)->PurposeId
				== PrepareRequest.IntentMetadata
			&& Repository.FindReservation(Charge.ReservationId)->PurposeId
				== PrepareRequest.IntentMetadata
			&& Repository.ValidateInvariants());

	FShanmenItemRepository Restarted;
	TestTrue(TEXT("Pending intent and exact prepare replay survive restart"),
		Restarted.TryLoadSnapshot(Repository.CaptureSnapshot())
			&& Restarted.PreparePreparedRunResourceIntent(PrepareRequest)
				== Prepared
			&& Restarted.FindItem(SwordId)->Durability == 100
			&& Restarted.FindItem(MirrorId)->Charges == 1);

	FShanmenItemRunResourceIntentRequest Overlap = PrepareRequest;
	Overlap.Context = MakeContext(406);
	Overlap.IntentId = FGuid(0x0A001000, 0, 0, 2);
	TestTrue(TEXT("A second external mutation cannot overlap the pending intent"),
		Restarted.PreparePreparedRunResourceIntent(Overlap).Error
			== EShanmenItemTransactionError::ResourceIntentConflict);

	FShanmenItemRunFinalizeRequest PrematureRunFinalize;
	PrematureRunFinalize.Context = MakeContext(407);
	PrematureRunFinalize.ActiveRunId = ActiveRun.ReservationId;
	PrematureRunFinalize.TerminalReason =
		EShanmenItemRunTerminalReason::Extraction;
	PrematureRunFinalize.SecuredOriginals = {
		Secured(SwordId, 1), Secured(MirrorId, 1) };
	TestTrue(TEXT("Run finalization cannot discard an in-doubt external intent"),
		Restarted.FinalizePreparedRun(PrematureRunFinalize).Error
			== EShanmenItemTransactionError::ResourceIntentConflict);

	const FShanmenItemRunResourceIntentFinalizeRequest FinalizeRequest =
		MakeRunResourceIntentFinalize(
			408, ActiveRun.ReservationId,
			PrepareRequest.Context.RequestId, IntentId, true);
	const int32 RevisionBeforeFinalize = Restarted.GetAuthorityRevision();
	const FShanmenItemTransactionReceipt Finalized =
		Restarted.FinalizePreparedRunResourceIntent(FinalizeRequest);
	TestTrue(TEXT("External success commits only the triggered prefix atomically"),
		Finalized.IsSuccess()
			&& Finalized.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRunResourceIntent
			&& Finalized.Phase == EShanmenItemTransactionPhase::Committed
			&& Finalized.ReservationId == IntentId
			&& Finalized.ItemInstanceId == PrepareRequest.Context.RequestId
			&& Restarted.GetAuthorityRevision() == RevisionBeforeFinalize + 1
			&& Restarted.FindItem(SwordId)->Durability == 98
			&& Restarted.FindItem(MirrorId)->Charges == 1
			&& Restarted.FindReservation(Durability.ReservationId)->State
				== EShanmenItemReservationState::Committed
			&& Restarted.FindReservation(Charge.ReservationId)->State
				== EShanmenItemReservationState::Cancelled
			&& Restarted.ValidateInvariants());

	FShanmenItemRepository FinalizedRestart;
	TestTrue(TEXT("Final decision replays after restart without double wear"),
		FinalizedRestart.TryLoadSnapshot(Restarted.CaptureSnapshot())
			&& FinalizedRestart.FinalizePreparedRunResourceIntent(FinalizeRequest)
				== Finalized
			&& FinalizedRestart.FindItem(SwordId)->Durability == 98
			&& FinalizedRestart.FindItem(MirrorId)->Charges == 1);

	FShanmenItemRunFinalizeRequest RunFinalize = PrematureRunFinalize;
	RunFinalize.Context = MakeContext(409);
	TestTrue(TEXT("Run can terminate only after the intent reaches a durable decision"),
		FinalizedRestart.FinalizePreparedRun(RunFinalize).IsSuccess()
			&& FinalizedRestart.FindItem(SwordId)->State
				== EShanmenItemInstanceState::Stored
			&& FinalizedRestart.FindItem(MirrorId)->State
				== EShanmenItemInstanceState::Stored);

	FShanmenItemRepository RejectedExternal;
	FShanmenItemTransactionReceipt RejectedRun;
	FShanmenItemTransactionReceipt RejectedDurability;
	FShanmenItemTransactionReceipt RejectedCharge;
	if (!StartFixture(
			RejectedExternal, RejectedRun,
			RejectedDurability, RejectedCharge))
	{
		AddError(TEXT("External rejection fixture failed to start."));
		return false;
	}
	const FGuid RejectedIntentId(0x0A001000, 0, 0, 3);
	const FShanmenItemRunResourceIntentRequest RejectedPrepareRequest =
		MakeRunResourceIntent(
			410, RejectedRun.ReservationId, RejectedIntentId,
			{
				ResourceLine(RejectedDurability.ReservationId, SwordId),
				ResourceLine(RejectedCharge.ReservationId, MirrorId)
			},
			2);
	const FShanmenItemTransactionReceipt RejectedPrepared =
		RejectedExternal.PreparePreparedRunResourceIntent(
			RejectedPrepareRequest);
	const FShanmenItemRunResourceIntentFinalizeRequest Rejection =
		MakeRunResourceIntentFinalize(
			411, RejectedRun.ReservationId,
			RejectedPrepareRequest.Context.RequestId,
			RejectedIntentId, false);
	const FShanmenItemTransactionReceipt Cancelled =
		RejectedExternal.FinalizePreparedRunResourceIntent(Rejection);
	TestTrue(TEXT("External rejection cancels every line and consumes nothing"),
		RejectedPrepared.IsSuccess() && Cancelled.IsSuccess()
			&& Cancelled.Phase == EShanmenItemTransactionPhase::Cancelled
			&& RejectedExternal.FindItem(SwordId)->Durability == 100
			&& RejectedExternal.FindItem(MirrorId)->Charges == 1
			&& RejectedExternal.FindReservation(
				RejectedDurability.ReservationId)->State
				== EShanmenItemReservationState::Cancelled
			&& RejectedExternal.FindReservation(
				RejectedCharge.ReservationId)->State
				== EShanmenItemReservationState::Cancelled
			&& RejectedExternal.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsDeploymentLifecycleTest,
	"Shanmen.0_0_10.Items.DeploymentLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsDeploymentLifecycleTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FShanmenItemTransactionReceipt Reserved = Repository.Reserve(MakeReserve(
		30, SwordId, EShanmenItemResourceKind::DeploymentLock, 1, TEXT("Combat.FlyingSword.Orbit")));
	TestTrue(TEXT("Flying sword lock reserves"), Reserved.IsSuccess());
	TestEqual(TEXT("Deployment lock is exclusive"), Repository.GetAvailableResource(SwordId, EShanmenItemResourceKind::DeploymentLock), 0);
	TestTrue(TEXT("Deployment lock blocks a concurrent durability reservation"), Repository.Reserve(MakeReserve(
		34, SwordId, EShanmenItemResourceKind::Durability, 1, TEXT("Combat.FlyingSword.Durability"))).Error
		== EShanmenItemTransactionError::InsufficientResource);
	TestTrue(TEXT("Second deployment cannot reserve"), Repository.Reserve(MakeReserve(
		31, SwordId, EShanmenItemResourceKind::DeploymentLock, 1, TEXT("Combat.FlyingSword.Launch"))).Error
		== EShanmenItemTransactionError::InsufficientResource);

	const FShanmenItemTransactionReceipt Deployed = Repository.Commit(MakeAction(32, Reserved.ReservationId));
	TestTrue(TEXT("Deployment commit succeeds"), Deployed.IsSuccess());
	TestTrue(TEXT("Same item identity becomes deployed"), Repository.FindItem(SwordId)->State == EShanmenItemInstanceState::Deployed);
	TestEqual(TEXT("Deployment does not consume the sword"), Repository.FindItem(SwordId)->Quantity, 1);
	TestTrue(TEXT("Deployment receipt is attached to item"), Repository.FindItem(SwordId)->DeploymentReservationId == Reserved.ReservationId);

	const FShanmenItemTransactionReceipt DurabilityReserved = Repository.Reserve(MakeReserve(
		35, SwordId, EShanmenItemResourceKind::Durability, 1, TEXT("Combat.FlyingSword.Impact"), 1));
	TestTrue(TEXT("A deployed sword can reserve impact durability"), DurabilityReserved.IsSuccess());
	TestTrue(TEXT("Deployed durability commit succeeds"), Repository.Commit(MakeAction(36, DurabilityReserved.ReservationId)).IsSuccess());
	TestEqual(TEXT("Impact consumes exactly one durability"), Repository.FindItem(SwordId)->Durability, 99);
	TestTrue(TEXT("Durability commit does not recover the sword"), Repository.FindItem(SwordId)->State == EShanmenItemInstanceState::Deployed);

	const FShanmenItemTransactionReceipt Released = Repository.ReleaseDeployment(MakeAction(33, Reserved.ReservationId));
	TestTrue(TEXT("Recovery release succeeds"), Released.IsSuccess());
	TestTrue(TEXT("Recovered sword returns to stored state"), Repository.FindItem(SwordId)->State == EShanmenItemInstanceState::Stored);
	TestFalse(TEXT("Recovered sword clears deployment identity"), Repository.FindItem(SwordId)->DeploymentReservationId.IsValid());
	TestEqual(TEXT("Recovered sword can be reserved again"), Repository.GetAvailableResource(SwordId, EShanmenItemResourceKind::DeploymentLock), 1);
	TestTrue(TEXT("Deployment lifecycle validates"), Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsTriggeredChargeTest,
	"Shanmen.0_0_10.Items.TriggeredChargeCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsTriggeredChargeTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;

	const FShanmenItemTransactionReceipt Reserved = Repository.Reserve(MakeReserve(
		40, MirrorId, EShanmenItemResourceKind::Charges, 1, TEXT("Defense.Artifact.HeartMirror")));
	TestTrue(TEXT("Heart mirror charge reserves"), Reserved.IsSuccess());
	TestEqual(TEXT("Charge remains until impact commit point"), Repository.FindItem(MirrorId)->Charges, 1);
	const FShanmenItemTransactionReceipt Committed = Repository.Commit(MakeAction(41, Reserved.ReservationId));
	TestTrue(TEXT("Triggered artifact commit succeeds"), Committed.IsSuccess());
	TestEqual(TEXT("Exactly one charge is consumed"), Repository.FindItem(MirrorId)->Charges, 0);
	TestEqual(TEXT("Item instance itself remains present"), Repository.FindItem(MirrorId)->Quantity, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsPersistenceReplayTest,
	"Shanmen.0_0_10.Items.PersistenceReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsPersistenceReplayTest::RunTest(const FString&)
{
	FShanmenItemRepository First;
	if (!LoadFixture(*this, First)) return false;
	const FShanmenItemReserveRequest ReserveRequest = MakeReserve(
		50, FormationMaterialId, EShanmenItemResourceKind::Quantity, 2, TEXT("Formation.Anchor.Persistent"));
	const FShanmenItemTransactionReceipt Reserved = First.Reserve(ReserveRequest);
	const FShanmenItemAuthoritySnapshot ReservedSnapshot = First.CaptureSnapshot();

	FShanmenItemRepository Reloaded;
	TestTrue(TEXT("Snapshot with active reservation reloads"), Reloaded.TryLoadSnapshot(ReservedSnapshot));
	TestTrue(TEXT("Reserve replay survives reload"), Reloaded.Reserve(ReserveRequest) == Reserved);
	const FShanmenItemReservationActionRequest CommitRequest = MakeAction(51, Reserved.ReservationId);
	const FShanmenItemTransactionReceipt Committed = Reloaded.Commit(CommitRequest);
	TestTrue(TEXT("Reloaded active reservation commits"), Committed.IsSuccess());

	FShanmenItemRepository ReloadedAgain;
	TestTrue(TEXT("Committed snapshot reloads"), ReloadedAgain.TryLoadSnapshot(Reloaded.CaptureSnapshot()));
	TestTrue(TEXT("Commit replay survives second reload"), ReloadedAgain.Commit(CommitRequest) == Committed);
	TestEqual(TEXT("Reload replay cannot double consume"), ReloadedAgain.FindItem(FormationMaterialId)->Quantity, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsFailureIsolationTest,
	"Shanmen.0_0_10.Items.FailureIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsFailureIsolationTest::RunTest(const FString&)
{
	FShanmenItemRepository Repository;
	if (!LoadFixture(*this, Repository)) return false;
	const int32 InitialAuthorityRevision = Repository.GetAuthorityRevision();
	const FShanmenItemInstance InitialItem = *Repository.FindItem(DartId);

	const FShanmenItemReserveRequest TooLarge = MakeReserve(
		60, DartId, EShanmenItemResourceKind::Quantity, 11, TEXT("Combat.Throw.Invalid"));
	const FShanmenItemTransactionReceipt Rejected = Repository.Reserve(TooLarge);
	TestTrue(TEXT("Insufficient request is explicit"), Rejected.Error == EShanmenItemTransactionError::InsufficientResource);
	TestTrue(TEXT("Rejected request replays deterministically"), Repository.Reserve(TooLarge) == Rejected);
	TestEqual(TEXT("Rejected receipt advances only the persisted ledger revision"), Repository.GetAuthorityRevision(), InitialAuthorityRevision + 1);
	TestTrue(TEXT("Rejected request cannot mutate item"), *Repository.FindItem(DartId) == InitialItem);

	FShanmenItemReserveRequest WrongOwner = MakeReserve(
		61, DartId, EShanmenItemResourceKind::Quantity, 1, TEXT("Combat.Throw.WrongOwner"));
	WrongOwner.Context.OwnerId = FGuid(999, 0, 0, 1);
	TestTrue(TEXT("Cross-owner request fails closed"), Repository.Reserve(WrongOwner).Error == EShanmenItemTransactionError::ScopeMismatch);

	FShanmenItemReserveRequest WrongContent = MakeReserve(
		62, DartId, EShanmenItemResourceKind::Quantity, 1, TEXT("Combat.Throw.StaleContent"));
	WrongContent.Context.Content.Digest = TEXT("STALE");
	TestTrue(TEXT("Content mismatch fails closed"), Repository.Reserve(WrongContent).Error == EShanmenItemTransactionError::ContentMismatch);

	FShanmenItemReserveRequest StaleRevision = MakeReserve(
		63, DartId, EShanmenItemResourceKind::Quantity, 1, TEXT("Combat.Throw.StaleRevision"), 9);
	TestTrue(TEXT("Stale item revision fails closed"), Repository.Reserve(StaleRevision).Error == EShanmenItemTransactionError::StaleItemRevision);

	FShanmenItemReserveRequest Unsupported = MakeReserve(
		64, SwordId, EShanmenItemResourceKind::Quantity, 1, TEXT("Combat.FlyingSword.Consume"));
	TestTrue(TEXT("Unsupported resource channel fails closed"), Repository.Reserve(Unsupported).Error == EShanmenItemTransactionError::ResourceUnsupported);
	TestTrue(TEXT("Failure ledger remains a valid part of snapshot"), Repository.ValidateInvariants());
	return true;
}

#endif
