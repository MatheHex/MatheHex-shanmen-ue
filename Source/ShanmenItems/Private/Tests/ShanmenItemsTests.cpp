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
			{ FShanmenItemNativeTags::CapabilityCharges(), FShanmenItemNativeTags::ItemArtifactLethalGuard() },
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
		Container.ContainerType = TEXT("Container.CombatLoadout");
		Container.Slots = { DartId, SwordId, MirrorId, FormationMaterialId };
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

	FShanmenItemReservationActionRequest MakeAction(
		uint32 Sequence,
		const FGuid& ReservationId)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context = MakeContext(Sequence);
		Request.ReservationId = ReservationId;
		return Request;
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
