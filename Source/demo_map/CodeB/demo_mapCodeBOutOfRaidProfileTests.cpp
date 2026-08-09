#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"

#include "demo_mapItemDefinitions.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

namespace
{
	using namespace demo_map_code_b;

	FGuid TestGuid(uint32 Value)
	{
		return FGuid(0xC0DE5000u + Value, 0x00000005u, 0x00000009u, 0x000000B5u);
	}

	Fdemo_mapPersistentItemRecord LegacyItem(FGuid ItemId, FName DefinitionId, int32 Quantity = 1)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = ItemId;
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = Quantity;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		return Item;
	}

	Fdemo_mapProfileSessionSnapshot LegacyPopulated(FGuid ProfileId)
	{
		Fdemo_mapProfileSessionSnapshot Snapshot;
		Snapshot.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation;
		Snapshot.ProfileId = ProfileId;
		Snapshot.SaveGeneration = 7;
		Snapshot.OrderedPermanentStash = {
			LegacyItem(TestGuid(101), Fdemo_mapItemIds::WeaponLevel1),
			LegacyItem(TestGuid(102), Fdemo_mapItemIds::ArmorRobeLevel1),
			LegacyItem(TestGuid(103), Fdemo_mapItemIds::EvasionCharm),
			LegacyItem(TestGuid(104), Fdemo_mapItemIds::WindTalisman),
			LegacyItem(TestGuid(105), Fdemo_mapItemIds::BackpackLevel1),
			LegacyItem(TestGuid(106), Fdemo_mapItemIds::SpiritDust, 3),
			LegacyItem(TestGuid(107), Fdemo_mapItemIds::SpiritWoodLevel1, 2)
		};
		Snapshot.PreparationLayout.WeaponItemInstanceId = TestGuid(101);
		Snapshot.PreparationLayout.ArmorItemInstanceId = TestGuid(102);
		Snapshot.PreparationLayout.AccessoryItemInstanceId = TestGuid(103);
		Snapshot.PreparationLayout.SpatialRingItemInstanceId = TestGuid(104);
		Snapshot.PreparationLayout.BackpackItemInstanceId = TestGuid(105);
		Snapshot.PreparationLayout.OrderedRunInventoryItemInstanceIds = { TestGuid(106) };
		// Existing Profile compatibility input: SpiritWood was stored inside the
		// old equipped ring.  P5 must retain this relationship in the real Code
		// B child container rather than placing a second active copy in warehouse.
		Snapshot.OrderedPermanentStash[6].LegacySpatialParentItemInstanceId = TestGuid(104);
		return Snapshot;
	}

	const FCodeBItemInstance* FindItem(const FCodeBRepository& Repository, const FGuid& ItemId)
	{
		return Repository.FindItem(ItemId);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCodeBOutOfRaidProfileHandoffTest,
	"demo_map.CodeB.P5.ProfileHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeBOutOfRaidProfileHandoffTest::RunTest(const FString& Parameters)
{
	const FString Root = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.9B.P5.0.r0"),
		FString::Printf(TEXT("ProfileHandoff_%lld"), FDateTime::UtcNow().GetTicks()));

	const FGuid LegacyOwner = TestGuid(1);
	const Fdemo_mapProfileSessionSnapshot Legacy = LegacyPopulated(LegacyOwner);
	FCodeBOutOfRaidProfileStore Store(Root, LegacyOwner);
	FCodeBRepository Repository;
	FCodeBP2PlayerLayout Layout;
	const FCodeBOutOfRaidOpenResult FirstOpen = Store.OpenOrMigrate(Legacy, Repository, Layout);
	AddInfo(FString::Printf(TEXT("P5 LegacyPopulated open: %s"), *FirstOpen.Diagnostic));
	TestTrue(TEXT("LegacyPopulated creates a real Code B record"), FirstOpen.bSuccess && FirstOpen.bCreated);
	TestTrue(TEXT("LegacyPopulated does not create fixture definitions"), Repository.FindDefinition(FName(TEXT("Weapon.A"))) == nullptr);
	TestEqual(TEXT("LegacyPopulated has exactly one mapped item per valid legacy item"), Store.GetRecord().Receipt.ItemMappings.Num(), 7);
	TestEqual(TEXT("LegacyPopulated starts with one committed receipt"), static_cast<int32>(Store.GetRecord().Receipt.State), static_cast<int32>(ECodeBOutOfRaidHandoffState::Committed));
	TestTrue(TEXT("Accessory remains a real accessory rather than being recast as a spatial ring"),
		FindItem(Repository, TestGuid(103))
		&& Layout.AccessoryContainerIds.IsValidIndex(0)
		&& FindItem(Repository, TestGuid(103))->ParentContainerId == Layout.AccessoryContainerIds[0]);
	TestTrue(TEXT("Ring is equipped in the real Profile layout"), FindItem(Repository, TestGuid(104)) && FindItem(Repository, TestGuid(104))->ParentContainerId == Layout.SpatialContainerId);
	TestTrue(TEXT("Backpack is equipped in the real Profile layout"), FindItem(Repository, TestGuid(105)) && FindItem(Repository, TestGuid(105))->ParentContainerId == Layout.BackpackContainerId);
	TestTrue(TEXT("Legacy spatial content retains the migrated ring ChildContainerId"),
		FindItem(Repository, TestGuid(104)) && FindItem(Repository, TestGuid(107))
		&& FindItem(Repository, TestGuid(104))->ChildContainerId.IsValid()
		&& FindItem(Repository, TestGuid(107))->ParentContainerId == FindItem(Repository, TestGuid(104))->ChildContainerId);
	const FCodeBItemDefinition* MigratedBackpackDefinition = Repository.FindDefinition(Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(TEXT("Backpack remains a distinct equipped spatial-item type"),
		MigratedBackpackDefinition && MigratedBackpackDefinition->ItemType == ECodeBItemType::Backpack);

	// P3's conditional child panels must follow the actually equipped object's
	// ChildContainerId.  Deliberately poison the retained legacy-layout pointers:
	// a future equip/replace must not make the UI show an old reserved container.
	FCodeBP2PlayerLayout DynamicChildLayout = Layout;
	DynamicChildLayout.SpatialInternalContainerId = TestGuid(901);
	DynamicChildLayout.PouchInternalContainerId = TestGuid(902);
	FCodeBP2Projection DynamicChildProjection;
	FString DynamicChildProjectionError;
	TestTrue(TEXT("P2 builds a dynamic real-Profile child projection"),
		FCodeBP2ProjectionBuilder::Build(Repository, DynamicChildLayout, DynamicChildProjection, nullptr, &DynamicChildProjectionError));
	const FCodeBP2ContainerView* DynamicQuickSpatial = DynamicChildProjection.Containers.FindByPredicate([](const FCodeBP2ContainerView& Candidate)
	{
		return Candidate.Role == FName(TEXT("QuickSpatial"));
	});
	const FCodeBP2ContainerView* DynamicPouch = DynamicChildProjection.Containers.FindByPredicate([](const FCodeBP2ContainerView& Candidate)
	{
		return Candidate.Role == FName(TEXT("PouchInternal"));
	});
	TestTrue(TEXT("Quick spatial panel follows the equipped ring child"),
		DynamicQuickSpatial && FindItem(Repository, TestGuid(104))
		&& DynamicQuickSpatial->ContainerId == FindItem(Repository, TestGuid(104))->ChildContainerId);
	TestTrue(TEXT("Pouch panel follows the equipped backpack child"),
		DynamicPouch && FindItem(Repository, TestGuid(105))
		&& DynamicPouch->ContainerId == FindItem(Repository, TestGuid(105))->ChildContainerId);

	FCodeBP2ApplicationService Service(Repository, Layout);
	const FCodeBItemInstance* Dust = FindItem(Repository, TestGuid(106));
	FCodeBP2Command MoveDust;
	MoveDust.TransactionId = TestGuid(201);
	MoveDust.Operation = ECodeBOperation::Move;
	MoveDust.ItemId = TestGuid(106);
	MoveDust.SourceContainerId = Dust ? Dust->ParentContainerId : FGuid();
	MoveDust.SourceSlot = Dust ? Dust->SlotIndex : INDEX_NONE;
	MoveDust.TargetContainerId = Layout.SpatialInternalContainerId;
	MoveDust.TargetSlot = 1;
	MoveDust.ExpectedRevision = Repository.GetRevision();
	const FCodeBP2ApplicationResult MoveResult = Service.Apply(MoveDust);
	TestTrue(TEXT("Real P2 move places a second item into the equipped ring child"), MoveResult.IsSuccess());
	FString CommitError;
	const int32 PersistentRevisionBeforeMove = Store.GetPersistentRevision();
	TestTrue(TEXT("Accepted P1/P2 result commits one persistent snapshot"), Store.CommitAcceptedSnapshot(Repository.CaptureSnapshot(), &CommitError));
	TestEqual(TEXT("Accepted result increments persistent revision exactly once"), Store.GetPersistentRevision(), PersistentRevisionBeforeMove + 1);
	const int32 RepositoryRevisionBeforeLoadedReject = Repository.GetRevision();
	const int32 PersistentRevisionBeforeLoadedReject = Store.GetPersistentRevision();
	FCodeBP2Command LoadedRingMove;
	LoadedRingMove.TransactionId = TestGuid(202);
	LoadedRingMove.Operation = ECodeBOperation::Move;
	LoadedRingMove.ItemId = TestGuid(104);
	LoadedRingMove.SourceContainerId = Layout.SpatialContainerId;
	LoadedRingMove.SourceSlot = 0;
	LoadedRingMove.TargetContainerId = Layout.WarehouseContainerId;
	LoadedRingMove.TargetSlot = 20;
	LoadedRingMove.ExpectedRevision = Repository.GetRevision();
	const FCodeBP2ApplicationResult LoadedReject = Service.Apply(LoadedRingMove);
	TestFalse(TEXT("Loaded ring move is rejected before P1 write"), LoadedReject.IsSuccess());
	TestEqual(TEXT("Loaded ring rejection preserves repository revision"), Repository.GetRevision(), RepositoryRevisionBeforeLoadedReject);
	TestEqual(TEXT("Loaded ring rejection preserves persistent revision"), Store.GetPersistentRevision(), PersistentRevisionBeforeLoadedReject);

	FCodeBOutOfRaidProfileStore RestartedStore(Root, LegacyOwner);
	FCodeBRepository RestartedRepository;
	FCodeBP2PlayerLayout RestartedLayout;
	const FCodeBOutOfRaidOpenResult RestartedOpen = RestartedStore.OpenOrMigrate(Legacy, RestartedRepository, RestartedLayout);
	TestTrue(TEXT("Close/reopen/restart recovers the same Profile record"), RestartedOpen.bSuccess && !RestartedOpen.bCreated);
	TestTrue(TEXT("Restart preserves the accepted snapshot exactly"), RestartedRepository.CaptureSnapshot() == Repository.CaptureSnapshot());
	TestEqual(TEXT("Restart preserves persistent revision"), RestartedStore.GetPersistentRevision(), Store.GetPersistentRevision());

	const FGuid EmptyOwner = TestGuid(2);
	Fdemo_mapProfileSessionSnapshot Empty;
	Empty.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation;
	Empty.ProfileId = EmptyOwner;
	FCodeBOutOfRaidProfileStore EmptyStore(Root, EmptyOwner);
	FCodeBRepository EmptyRepository;
	FCodeBP2PlayerLayout EmptyLayout;
	const FCodeBOutOfRaidOpenResult EmptyOpen = EmptyStore.OpenOrMigrate(Empty, EmptyRepository, EmptyLayout);
	TestTrue(TEXT("EmptyProfile creates a real empty Code B record"), EmptyOpen.bSuccess && EmptyRepository.CaptureSnapshot().Items.IsEmpty());

	const FGuid InvalidOwner = TestGuid(3);
	Fdemo_mapProfileSessionSnapshot Invalid = LegacyPopulated(InvalidOwner);
	Invalid.OrderedPermanentStash.Add(LegacyItem(TestGuid(301), FName(TEXT("Missing.LegacyWeapon"))));
	Invalid.PreparationLayout.WeaponItemInstanceId = TestGuid(301);
	FCodeBOutOfRaidProfileStore InvalidStore(Root, InvalidOwner);
	FCodeBRepository InvalidRepository;
	FCodeBP2PlayerLayout InvalidLayout;
	const FCodeBOutOfRaidOpenResult InvalidOpen = InvalidStore.OpenOrMigrate(Invalid, InvalidRepository, InvalidLayout);
	TestTrue(TEXT("LegacyInvalid still opens safe valid content"), InvalidOpen.bSuccess);
	TestTrue(TEXT("LegacyInvalid records an auditable invalid-entry count"), InvalidStore.GetRecord().Receipt.InvalidLegacyEntries.Num() >= 1);

	const FGuid InterruptedOwner = TestGuid(4);
	const Fdemo_mapProfileSessionSnapshot Interrupted = LegacyPopulated(InterruptedOwner);
	FCodeBOutOfRaidProfileStore InterruptedStore(Root, InterruptedOwner);
	InterruptedStore.SetInterruptAfterPreparedReceiptForAutomation(true);
	FCodeBRepository InterruptedRepository;
	FCodeBP2PlayerLayout InterruptedLayout;
	const FCodeBOutOfRaidOpenResult InterruptedOpen = InterruptedStore.OpenOrMigrate(Interrupted, InterruptedRepository, InterruptedLayout);
	TestFalse(TEXT("Interrupted handoff does not expose a half-open repository"), InterruptedOpen.bSuccess);
	FCodeBOutOfRaidProfileStore RecoveredStore(Root, InterruptedOwner);
	FCodeBRepository RecoveredRepository;
	FCodeBP2PlayerLayout RecoveredLayout;
	const FCodeBOutOfRaidOpenResult RecoveredOpen = RecoveredStore.OpenOrMigrate(Interrupted, RecoveredRepository, RecoveredLayout);
	TestTrue(TEXT("Prepared receipt is finalized idempotently after restart"), RecoveredOpen.bSuccess && RecoveredOpen.bRecoveredPendingReceipt);
	TestEqual(TEXT("Interrupted recovery has no duplicated instances"), RecoveredRepository.CaptureSnapshot().Items.Num(), 7);
	TestEqual(TEXT("Interrupted recovery has one receipt mapping set"), RecoveredStore.GetRecord().Receipt.ItemMappings.Num(), 7);

	const FGuid SecondOwner = TestGuid(5);
	Fdemo_mapProfileSessionSnapshot Second = LegacyPopulated(SecondOwner);
	Second.OrderedPermanentStash.RemoveAt(0);
	FCodeBOutOfRaidProfileStore SecondStore(Root, SecondOwner);
	FCodeBRepository SecondRepository;
	FCodeBP2PlayerLayout SecondLayout;
	const FCodeBOutOfRaidOpenResult SecondOpen = SecondStore.OpenOrMigrate(Second, SecondRepository, SecondLayout);
	TestTrue(TEXT("TwoProfiles opens the second OwnerId independently"), SecondOpen.bSuccess);
	TestNotEqual(TEXT("TwoProfiles uses separate warehouse containers"), Layout.WarehouseContainerId, SecondLayout.WarehouseContainerId);
	TestNotEqual(TEXT("TwoProfiles keeps separate visible item sets"), Repository.CaptureSnapshot().Items.Num(), SecondRepository.CaptureSnapshot().Items.Num());

	// A terminal RunId is a durable history tombstone, not a live P6 session.
	// Returning to ReadyForPreparation must make the P5 warehouse usable
	// without discarding that history.
	Fdemo_mapProfileSessionSnapshot TerminalHistory = LegacyPopulated(TestGuid(5));
	TerminalHistory.ActiveRunId = TestGuid(501);
	FCodeBOutOfRaidProfileStore TerminalHistoryStore(Root, TerminalHistory.ProfileId);
	FCodeBRepository TerminalHistoryRepository;
	FCodeBP2PlayerLayout TerminalHistoryLayout;
	const FCodeBOutOfRaidOpenResult TerminalHistoryOpen = TerminalHistoryStore.OpenOrMigrate(
		TerminalHistory, TerminalHistoryRepository, TerminalHistoryLayout);
	TestTrue(TEXT("Terminal Run history opens P5 after returning to preparation"),
		TerminalHistoryOpen.bSuccess && TerminalHistoryStore.GetRecord().Receipt.SourceProfileId == TerminalHistory.ProfileId);

	Fdemo_mapProfileSessionSnapshot ActiveRun = LegacyPopulated(TestGuid(6));
	ActiveRun.SessionState = Edemo_mapProfileSessionState::RunActive;
	ActiveRun.ActiveRunId = TestGuid(601);
	FCodeBOutOfRaidProfileStore ActiveStore(Root, ActiveRun.ProfileId);
	FCodeBRepository ActiveRepository;
	FCodeBP2PlayerLayout ActiveLayout;
	const FCodeBOutOfRaidOpenResult ActiveOpen = ActiveStore.OpenOrMigrate(ActiveRun, ActiveRepository, ActiveLayout);
	TestFalse(TEXT("Active Run rejects handoff with zero write"), ActiveOpen.bSuccess);
	TestTrue(TEXT("Active Run rejection gives the mandated Chinese message"), ActiveOpen.Diagnostic.Contains(TEXT("结束当前 Run 后再整理")));
	TestFalse(TEXT("Active Run rejection creates no sidecar record"), IFileManager::Get().FileExists(*ActiveStore.GetPrimaryPath()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCodeBRunInventoryBridgeTest,
	"demo_map.CodeB.P6.RunInventoryBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeBRunInventoryBridgeTest::RunTest(const FString& Parameters)
{
	const FString Root = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.9B.P6.0.r0"),
		FString::Printf(TEXT("RunBridge_%lld"), FDateTime::UtcNow().GetTicks()));
	const FGuid Owner = TestGuid(700);
	const FGuid RunOne = TestGuid(701);
	Fdemo_mapProfileSessionSnapshot Legacy = LegacyPopulated(Owner);
	// This item is never selected by the P5 carry layout and must remain out of
	// raid after P6 commits the same ItemId set into the Run session.
	Legacy.OrderedPermanentStash.Add(LegacyItem(TestGuid(108), Fdemo_mapItemIds::SpiritDust, 2));
	FCodeBOutOfRaidProfileStore ProfileStore(Root, Owner);
	FCodeBRepository ProfileRepository;
	FCodeBP2PlayerLayout ProfileLayout;
	const FCodeBOutOfRaidOpenResult ProfileOpen = ProfileStore.OpenOrMigrate(Legacy, ProfileRepository, ProfileLayout);
	TestTrue(TEXT("P6 fixture is a committed P5 Profile sidecar"), ProfileOpen.bSuccess && !ProfileStore.GetRecord().bHasActiveRunInventorySession);
	const FCodeBRunInventoryBridgeResult First = FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(Root, Owner, RunOne);
	TestTrue(TEXT("P6 creates one committed Run session"), First.Status == ECodeBRunInventoryBridgeStatus::Committed && First.IsCommitted());
	TestEqual(TEXT("P6 moves only the carry layout and equipped child content"), First.Session.Receipt.MovedItemIds.Num(), 7);
	TestTrue(TEXT("P6 keeps the original spatial child container identity"),
		First.Session.RepositorySnapshot.Items.FindRef(TestGuid(104)).ChildContainerId.IsValid()
		&& First.Session.RepositorySnapshot.Items.FindRef(TestGuid(107)).ParentContainerId
			== First.Session.RepositorySnapshot.Items.FindRef(TestGuid(104)).ChildContainerId);
	TestTrue(TEXT("P6 leaves the warehouse-only item in the P5 snapshot"),
		ProfileStore.NotifySuccessfulRunForAutomation(RunOne).Status == ECodeBRunInventoryBridgeStatus::AlreadyCommitted);
	FCodeBOutOfRaidProfileStore ReadAfterBridge(Root, Owner);
	FCodeBRepository ReadRepository;
	FCodeBP2PlayerLayout ReadLayout;
	const FCodeBOutOfRaidOpenResult ReadOpen = ReadAfterBridge.OpenOrMigrate(Legacy, ReadRepository, ReadLayout);
	TestTrue(TEXT("P6 restart reads the committed profile sidecar"), ReadOpen.bSuccess);
	TestFalse(TEXT("P6 removes moved items from out-of-raid authority"), ReadAfterBridge.GetRecord().RepositorySnapshot.Items.Contains(TestGuid(101)));
	TestTrue(TEXT("P6 preserves unselected warehouse item"), ReadAfterBridge.GetRecord().RepositorySnapshot.Items.Contains(TestGuid(108)));
	TestTrue(TEXT("P6 marks the normal out-of-raid page read-only while session is active"),
		FCodeBOutOfRaidProfileStore::HasActiveRunInventorySession(Root, Owner));
	const int32 RevisionAfterCommit = ReadAfterBridge.GetPersistentRevision();
	const FCodeBRunInventoryBridgeResult Duplicate = FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(Root, Owner, RunOne);
	TestTrue(TEXT("P6 same OwnerId plus RunInstanceId is idempotent"), Duplicate.Status == ECodeBRunInventoryBridgeStatus::AlreadyCommitted);
	FCodeBOutOfRaidProfileStore ReadAfterDuplicate(Root, Owner);
	FCodeBRepository DuplicateRepository;
	FCodeBP2PlayerLayout DuplicateLayout;
	const FCodeBOutOfRaidOpenResult DuplicateOpen = ReadAfterDuplicate.OpenOrMigrate(Legacy, DuplicateRepository, DuplicateLayout);
	TestTrue(TEXT("P6 duplicate notification does not advance out-of-raid revision"),
		DuplicateOpen.bSuccess && ReadAfterDuplicate.GetPersistentRevision() == RevisionAfterCommit);
	const FCodeBRunInventoryBridgeResult Conflict = FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(Root, Owner, TestGuid(702));
	TestTrue(TEXT("P6 different RunInstanceId is refused without changing the existing session"),
		Conflict.Status == ECodeBRunInventoryBridgeStatus::ActiveSessionConflict
		&& Conflict.Session.RunInstanceId == RunOne);

	const FGuid EmptyOwner = TestGuid(703);
	Fdemo_mapProfileSessionSnapshot EmptyProfile;
	EmptyProfile.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation;
	EmptyProfile.ProfileId = EmptyOwner;
	FCodeBOutOfRaidProfileStore EmptyStore(Root, EmptyOwner);
	FCodeBRepository EmptyRepository;
	FCodeBP2PlayerLayout EmptyLayout;
	TestTrue(TEXT("P6 empty Profile first creates its real P5 sidecar"), EmptyStore.OpenOrMigrate(EmptyProfile, EmptyRepository, EmptyLayout).bSuccess);
	const FCodeBRunInventoryBridgeResult EmptyResult = FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(Root, EmptyOwner, TestGuid(704));
	TestTrue(TEXT("P6 empty Profile commits a real empty Run session"), EmptyResult.IsCommitted() && EmptyResult.Session.RepositorySnapshot.Items.IsEmpty());

	const FGuid UnenrolledOwner = TestGuid(705);
	const FCodeBRunInventoryBridgeResult Unenrolled = FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(Root, UnenrolledOwner, TestGuid(706));
	FCodeBOutOfRaidProfileStore UnenrolledStore(Root, UnenrolledOwner);
	TestTrue(TEXT("P6 never enrolls an absent P5 sidecar during Start Run notification"),
		Unenrolled.Status == ECodeBRunInventoryBridgeStatus::NotEnrolled
		&& !IFileManager::Get().FileExists(*UnenrolledStore.GetPrimaryPath()));

	const FGuid InterruptedOwner = TestGuid(707);
	Fdemo_mapProfileSessionSnapshot InterruptedLegacy = LegacyPopulated(InterruptedOwner);
	FCodeBOutOfRaidProfileStore InterruptedStore(Root, InterruptedOwner);
	FCodeBRepository InterruptedRepository;
	FCodeBP2PlayerLayout InterruptedLayout;
	TestTrue(TEXT("P6 interruption fixture has a committed P5 sidecar"),
		InterruptedStore.OpenOrMigrate(InterruptedLegacy, InterruptedRepository, InterruptedLayout).bSuccess);
	InterruptedStore.SetInterruptAfterRunPreparedReceiptForAutomation(true);
	const FCodeBRunInventoryBridgeResult Interrupted = InterruptedStore.NotifySuccessfulRunForAutomation(TestGuid(708));
	TestTrue(TEXT("P6 interruption leaves only a verified Prepared receipt"),
		Interrupted.Status == ECodeBRunInventoryBridgeStatus::StorageFailure);
	const FCodeBRunInventoryBridgeResult Recovered = FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(Root, InterruptedOwner, TestGuid(708));
	TestTrue(TEXT("P6 restart deterministically finalizes the Prepared receipt"),
		Recovered.Status == ECodeBRunInventoryBridgeStatus::RecoveredPreparedReceipt && Recovered.IsCommitted());
	TestEqual(TEXT("P6 recovery retains one unique moved ItemId set"), Recovered.Session.RepositorySnapshot.Items.Num(), 7);
	return true;
}

#endif
