#include "Misc/AutomationTest.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapRuntimeContainer.h"
#include "demo_mapSearchContainerPresenter.h"
#include "demo_mapSearchContainerTypes.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FSearchContainerFixture
	{
		Fdemo_mapItemAuthority Items;
		Fdemo_mapRuntimeContainerAuthority Container;
		FGuid ContainerId = FGuid::NewGuid();
		FGuid RunId = FGuid::NewGuid();
		TArray<FGuid> ItemIds;

		bool Initialize(
			Edemo_mapRuntimeContainerKind Kind,
			const TArray<Fdemo_mapRuntimeContainerSeedEntry>& Seed)
		{
			TArray<Fdemo_mapRuntimeContainerResolvedSeedEntry> Resolved;
			for (const Fdemo_mapRuntimeContainerSeedEntry& Entry : Seed)
			{
				FGuid ItemId;
				if (!Items.CreateContainerDefinition(
					Entry.DefinitionId,
					Entry.StackCount,
					ContainerId,
					RunId,
					ItemId).bSuccess)
				{
					return false;
				}
				ItemIds.Add(ItemId);
				Fdemo_mapRuntimeContainerResolvedSeedEntry ResolvedEntry;
				ResolvedEntry.Section = Entry.Section;
				ResolvedEntry.SlotIndex = Entry.SlotIndex;
				ResolvedEntry.ItemInstanceId = ItemId;
				ResolvedEntry.DefinitionId = Entry.DefinitionId;
				ResolvedEntry.StackCount = Entry.StackCount;
				ResolvedEntry.SearchDurationSeconds =
					Fdemo_mapSearchContainerPrototypeConfig::GetSearchSeconds(
						Kind,
						Entry.Section);
				Resolved.Add(ResolvedEntry);
			}
			FString Diagnostic;
			return Container.Initialize(
				ContainerId,
				RunId,
				Kind,
				Resolved,
				Diagnostic);
		}

		Fdemo_mapRuntimeContainerIntent Intent(
			Edemo_mapRuntimeContainerActionKind Action,
			FGuid EntryId = FGuid()) const
		{
			Fdemo_mapRuntimeContainerIntent Value;
			Value.ExpectedRunId = RunId;
			Value.ContainerId = ContainerId;
			Value.ExpectedRevision = Container.GetRevision();
			Value.EntryId = EntryId;
			Value.Action = Action;
			return Value;
		}

		Fdemo_mapRuntimeContainerResult Submit(
			Edemo_mapRuntimeContainerActionKind Action,
			FGuid EntryId = FGuid(),
			bool bAlive = true,
			bool bInRange = true,
			bool bFailAfterMutation = false)
		{
			return Container.SubmitIntent(
				Intent(Action, EntryId),
				bAlive,
				bInRange,
				[this, bFailAfterMutation](FGuid ItemId)
				{
					return Items.TransferContainerToInventoryWhole(
						ItemId,
						ContainerId,
						bFailAfterMutation);
				});
		}

		bool Open()
		{
			return Submit(Edemo_mapRuntimeContainerActionKind::BeginOpen).bSuccess
				&& Container.CompleteActiveAction().bSuccess
				&& Container.GetState()
					== Edemo_mapRuntimeContainerState::Opened;
		}

		bool Search(FGuid EntryId)
		{
			return Submit(
				Edemo_mapRuntimeContainerActionKind::BeginSearch,
				EntryId).bSuccess
				&& Container.CompleteActiveAction().bSuccess;
		}

		const Fdemo_mapRuntimeContainerEntryRecord* Entry(
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex) const
		{
			return Container.GetEntriesForAudit().FindByPredicate(
				[Section, SlotIndex](
					const Fdemo_mapRuntimeContainerEntryRecord& Candidate)
				{
					return Candidate.Section == Section
						&& Candidate.SlotIndex == SlotIndex;
				});
		}

		Fdemo_mapRuntimeContainerSnapshot Snapshot(float Progress = 0.0f) const
		{
			return Container.BuildSnapshot(
				Items,
				Progress,
				Items.GetUsedInventorySlots(),
				Items.GetInventoryCapacity());
		}
	};

	struct FFileEvidence
	{
		bool bExists = false;
		TArray<uint8> Bytes;
	};

	FFileEvidence ReadEvidence(const FString& Path)
	{
		FFileEvidence Result;
		Result.bExists = IFileManager::Get().FileExists(*Path);
		if (Result.bExists)
		{
			FFileHelper::LoadFileToArray(Result.Bytes, *Path);
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSearchContainer01,
	"demo_map.SearchContainer.01.PrototypeConfigExact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer01::RunTest(const FString&)
{
	TestEqual(TEXT("Chest open"), Fdemo_mapSearchContainerPrototypeConfig::ChestOpenSeconds, 1.0f);
	TestEqual(TEXT("Corpse open"), Fdemo_mapSearchContainerPrototypeConfig::CorpseOpenSeconds, 1.0f);
	TestEqual(TEXT("Chest search"), Fdemo_mapSearchContainerPrototypeConfig::ChestEntrySearchSeconds, 0.75f);
	TestEqual(TEXT("Equipment search"), Fdemo_mapSearchContainerPrototypeConfig::CorpseEquipmentSearchSeconds, 0.0f);
	TestEqual(TEXT("Backpack search"), Fdemo_mapSearchContainerPrototypeConfig::CorpseBackpackEntrySearchSeconds, 1.0f);
	TestEqual(TEXT("Body search"), Fdemo_mapSearchContainerPrototypeConfig::CorpseBodyEntrySearchSeconds, 1.5f);
	TestEqual(TEXT("Chest capacity"), Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity, 6);
	TestEqual(TEXT("Concurrent action"), Fdemo_mapSearchContainerPrototypeConfig::MaxConcurrentContainerAction, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer02, "demo_map.SearchContainer.02.ChestClosedOpeningOpened", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer02::RunTest(const FString&)
{
	FSearchContainerFixture F;
	TestTrue(TEXT("Initialize"), F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)));
	TestTrue(TEXT("Begin"), F.Submit(Edemo_mapRuntimeContainerActionKind::BeginOpen).bSuccess);
	TestEqual(TEXT("Opening"), F.Container.GetState(), Edemo_mapRuntimeContainerState::Opening);
	TestTrue(TEXT("Complete"), F.Container.CompleteActiveAction().bSuccess);
	TestEqual(TEXT("Opened"), F.Container.GetState(), Edemo_mapRuntimeContainerState::Opened);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer03, "demo_map.SearchContainer.03.CorpseClosedOpeningOpened", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer03::RunTest(const FString&)
{
	FSearchContainerFixture F;
	TestTrue(TEXT("Initialize"), F.Initialize(Edemo_mapRuntimeContainerKind::Corpse, Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed()));
	TestTrue(TEXT("Corpse opens"), F.Open());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer04, "demo_map.SearchContainer.04.ReleaseCancelsOpening", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer04::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0));
	F.Submit(Edemo_mapRuntimeContainerActionKind::BeginOpen);
	TestTrue(TEXT("Cancel"), F.Container.CancelActiveAction(TEXT("Release")).bSuccess);
	TestEqual(TEXT("Closed"), F.Container.GetState(), Edemo_mapRuntimeContainerState::Closed);
	TestFalse(TEXT("Idle"), F.Container.IsActionActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer05, "demo_map.SearchContainer.05.RangeCancelsOpening", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer05::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0));
	F.Submit(Edemo_mapRuntimeContainerActionKind::BeginOpen);
	F.Container.CancelActiveAction(TEXT("OutOfRange"));
	TestEqual(TEXT("Range cancellation returns Closed"), F.Container.GetState(), Edemo_mapRuntimeContainerState::Closed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer06, "demo_map.SearchContainer.06.PositiveDamageCancelsOpening", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer06::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Corpse, Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed());
	F.Submit(Edemo_mapRuntimeContainerActionKind::BeginOpen);
	F.Container.CancelActiveAction(TEXT("PositiveDamage"));
	TestTrue(TEXT("Damage cancellation is idle Closed"), !F.Container.IsActionActive() && F.Container.GetState() == Edemo_mapRuntimeContainerState::Closed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer07, "demo_map.SearchContainer.07.OpenedPersistsAcrossCloseReopen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer07::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	TestTrue(TEXT("Close is accepted"), F.Submit(Edemo_mapRuntimeContainerActionKind::Close).bSuccess);
	TestEqual(TEXT("Close does not relock"), F.Container.GetState(), Edemo_mapRuntimeContainerState::Opened);
	TestEqual(TEXT("Re-open is a no-op"), F.Submit(Edemo_mapRuntimeContainerActionKind::BeginOpen).Code, Edemo_mapRuntimeContainerResultCode::NoOp);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer08, "demo_map.SearchContainer.08.ChestSectionCapacitySlots", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer08::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0));
	const auto S = F.Snapshot();
	TestTrue(TEXT("One six-slot Chest section"), S.Sections.Num() == 1 && S.Sections[0].Section == Edemo_mapRuntimeContainerSection::Chest && S.Sections[0].Capacity == 6);
	TestTrue(TEXT("Deterministic slots"), S.Sections[0].OrderedOccupiedEntries.Num() == 2 && S.Sections[0].OrderedOccupiedEntries[0].SlotIndex == 0 && S.Sections[0].OrderedOccupiedEntries[1].SlotIndex == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer09, "demo_map.SearchContainer.09.CorpseSectionOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer09::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Corpse, Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed());
	const auto S = F.Snapshot();
	TestTrue(TEXT("Three ordered sections"), S.Sections.Num() == 3 && S.Sections[0].Section == Edemo_mapRuntimeContainerSection::Equipment && S.Sections[1].Section == Edemo_mapRuntimeContainerSection::Backpack && S.Sections[2].Section == Edemo_mapRuntimeContainerSection::Body);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer10, "demo_map.SearchContainer.10.EquipmentCurrentSlotsImmediateIdentified", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer10::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Corpse, Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed()); F.Open();
	const auto S = F.Snapshot(); const auto* E = F.Entry(Edemo_mapRuntimeContainerSection::Equipment, 0);
	TestTrue(TEXT("Equipment current slots and identified"),
		S.Sections[0].Capacity == Fdemo_mapSearchContainerPrototypeConfig::CorpseEquipmentCapacity
		&& E && E->State == Edemo_mapRuntimeContainerEntryState::Identified);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer11, "demo_map.SearchContainer.11.ChestEntriesInitiallyHidden", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer11::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	TestTrue(TEXT("Both Chest entries hidden"), F.Container.GetEntriesForAudit().ContainsByPredicate([](const auto& E){ return E.SlotIndex == 0 && E.State == Edemo_mapRuntimeContainerEntryState::Hidden; }) && F.Container.GetEntriesForAudit().ContainsByPredicate([](const auto& E){ return E.SlotIndex == 1 && E.State == Edemo_mapRuntimeContainerEntryState::Hidden; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer12, "demo_map.SearchContainer.12.CorpseHiddenSearchDurations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer12::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Corpse, Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed()); F.Open();
	const auto* B = F.Entry(Edemo_mapRuntimeContainerSection::Backpack, 0); const auto* D = F.Entry(Edemo_mapRuntimeContainerSection::Body, 0);
	TestTrue(TEXT("Backpack hidden 1.0"), B && B->State == Edemo_mapRuntimeContainerEntryState::Hidden && FMath::IsNearlyEqual(B->SearchDurationSeconds, 1.0f));
	TestTrue(TEXT("Body hidden 1.5"), D && D->State == Edemo_mapRuntimeContainerEntryState::Hidden && FMath::IsNearlyEqual(D->SearchDurationSeconds, 1.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer13, "demo_map.SearchContainer.13.HiddenSnapshotRedaction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer13::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const auto E = F.Snapshot().Sections[0].OrderedOccupiedEntries[0];
	TestTrue(TEXT("No identity leak"), !E.ItemInstanceId.IsValid() && E.DefinitionId.IsNone() && E.DisplayName.IsEmpty() && E.CategoryId.IsNone() && E.Level == 0 && E.StackCount == 0 && E.UnitSellPrice == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer14, "demo_map.SearchContainer.14.SingleConcurrentSearch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer14::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const auto& Entries = F.Container.GetEntriesForAudit();
	TestTrue(TEXT("First search"), F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Entries[0].EntryId).bSuccess);
	TestEqual(TEXT("Second rejected"), F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Entries[1].EntryId).Code, Edemo_mapRuntimeContainerResultCode::ConcurrentAction);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer15, "demo_map.SearchContainer.15.HiddenSearchingIdentified", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer15::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const FGuid Id = F.Container.GetEntriesForAudit()[0].EntryId;
	F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id);
	TestEqual(TEXT("Searching"), F.Container.FindEntry(Id)->State, Edemo_mapRuntimeContainerEntryState::Searching);
	F.Container.CompleteActiveAction();
	TestEqual(TEXT("Identified"), F.Container.FindEntry(Id)->State, Edemo_mapRuntimeContainerEntryState::Identified);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer16, "demo_map.SearchContainer.16.CloseCancelsOnlyCurrentSearch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer16::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const FGuid Id = F.Container.GetEntriesForAudit()[0].EntryId; F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id);
	F.Submit(Edemo_mapRuntimeContainerActionKind::Close, Id);
	TestTrue(TEXT("Only current returns Hidden"), F.Container.FindEntry(Id)->State == Edemo_mapRuntimeContainerEntryState::Hidden && !F.Container.IsActionActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer17, "demo_map.SearchContainer.17.RangeAndDamageCancelSearching", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer17::RunTest(const FString&)
{
	for (const TCHAR* Reason : { TEXT("OutOfRange"), TEXT("PositiveDamage") })
	{
		FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
		const FGuid Id = F.Container.GetEntriesForAudit()[0].EntryId; F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id);
		F.Container.CancelActiveAction(Reason);
		TestTrue(Reason, F.Container.FindEntry(Id)->State == Edemo_mapRuntimeContainerEntryState::Hidden && !F.Container.IsActionActive());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer18, "demo_map.SearchContainer.18.IdentifiedTakenNeverRegress", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer18::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const FGuid First = F.Container.GetEntriesForAudit()[0].EntryId; const FGuid Second = F.Container.GetEntriesForAudit()[1].EntryId;
	F.Search(First); F.Submit(Edemo_mapRuntimeContainerActionKind::Take, First);
	F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Second); F.Container.CancelActiveAction(TEXT("Damage"));
	TestTrue(TEXT("Taken remains Taken"), F.Container.FindEntry(First)->State == Edemo_mapRuntimeContainerEntryState::Taken);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer19, "demo_map.SearchContainer.19.TakenSlotStableNoReorder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer19::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const FGuid First = F.Container.GetEntriesForAudit()[0].EntryId; F.Search(First); F.Submit(Edemo_mapRuntimeContainerActionKind::Take, First);
	const auto S = F.Snapshot();
	TestTrue(TEXT("Taken slot retained"), S.Sections[0].OrderedOccupiedEntries[0].EntryId == First && S.Sections[0].OrderedOccupiedEntries[0].SlotIndex == 0 && S.Sections[0].OrderedOccupiedEntries[1].SlotIndex == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer20, "demo_map.SearchContainer.20.TakeWholeSameGuidStack", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer20::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const auto* E = F.Entry(Edemo_mapRuntimeContainerSection::Chest, 0); const FGuid EntryId = E->EntryId; const FGuid ItemId = E->InternalItemInstanceId;
	F.Search(EntryId); const auto R = F.Submit(Edemo_mapRuntimeContainerActionKind::Take, EntryId); const auto* Item = F.Items.FindInstance(ItemId);
	TestTrue(TEXT("Same full instance"), R.bSuccess && R.ItemInstanceId == ItemId && Item && Item->Quantity == 2 && Item->OwnershipState == Edemo_mapItemOwnershipState::Inventory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer21, "demo_map.SearchContainer.21.InventoryFullTakeZeroChange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer21::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	for (int32 Index = 0; Index < F.Items.GetInventoryCapacity(); ++Index) F.Items.AddDefinition(Fdemo_mapItemIds::WeaponLevel1, 1);
	const auto* E = F.Entry(Edemo_mapRuntimeContainerSection::Chest, 0); const FGuid EntryId = E->EntryId; const FGuid ItemId = E->InternalItemInstanceId; F.Search(EntryId);
	const int32 UsedBefore = F.Items.GetUsedInventorySlots(); const auto R = F.Submit(Edemo_mapRuntimeContainerActionKind::Take, EntryId); const auto* Item = F.Items.FindInstance(ItemId);
	TestTrue(TEXT("Full rejects without mutation"), !R.bSuccess && F.Items.GetUsedInventorySlots() == UsedBefore && Item && Item->OwnershipState == Edemo_mapItemOwnershipState::Container && F.Container.FindEntry(EntryId)->State == Edemo_mapRuntimeContainerEntryState::Identified);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer22, "demo_map.SearchContainer.22.InvalidAndStaleIntentRejections", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer22::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const FGuid Id = F.Container.GetEntriesForAudit()[0].EntryId;
	TestFalse(TEXT("Hidden Take rejected"), F.Submit(Edemo_mapRuntimeContainerActionKind::Take, Id).bSuccess);
	auto Stale = F.Intent(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id); --Stale.ExpectedRevision;
	TestEqual(TEXT("Stale"), F.Container.SubmitIntent(Stale, true, true, [](FGuid){ return Fdemo_mapItemOperationResult::Success(); }).Code, Edemo_mapRuntimeContainerResultCode::StaleRevision);
	auto WrongRun = F.Intent(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id); WrongRun.ExpectedRunId = FGuid::NewGuid();
	TestEqual(TEXT("Wrong run"), F.Container.SubmitIntent(WrongRun, true, true, [](FGuid){ return Fdemo_mapItemOperationResult::Success(); }).Code, Edemo_mapRuntimeContainerResultCode::InvalidRun);
	TestEqual(TEXT("Unknown entry"), F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, FGuid::NewGuid()).Code, Edemo_mapRuntimeContainerResultCode::InvalidEntry);
	F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id);
	TestFalse(TEXT("Searching Take rejected"), F.Submit(Edemo_mapRuntimeContainerActionKind::Take, Id).bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer23, "demo_map.SearchContainer.23.AuthorityFailureFullRollback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer23::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const auto* E = F.Entry(Edemo_mapRuntimeContainerSection::Chest, 0); const FGuid EntryId = E->EntryId; const FGuid ItemId = E->InternalItemInstanceId; F.Search(EntryId);
	const auto Before = F.Items.CaptureState(); const auto R = F.Submit(Edemo_mapRuntimeContainerActionKind::Take, EntryId, true, true, true); const auto After = F.Items.CaptureState();
	const Fdemo_mapItemInstance* BeforeItem = Before.Instances.Find(ItemId);
	const Fdemo_mapItemInstance* AfterItem = After.Instances.Find(ItemId);
	TestTrue(TEXT("Injected failure rolls back"), !R.bSuccess
		&& Before.Instances.Num() == After.Instances.Num()
		&& Before.InventorySlots == After.InventorySlots
		&& Before.EquipmentSlots.OrderIndependentCompareEqual(After.EquipmentSlots)
		&& Before.SessionStash == After.SessionStash
		&& BeforeItem && AfterItem
		&& BeforeItem->DefinitionId == AfterItem->DefinitionId
		&& BeforeItem->Quantity == AfterItem->Quantity
		&& BeforeItem->OwnershipState == AfterItem->OwnershipState
		&& BeforeItem->ContainerId == AfterItem->ContainerId
		&& F.Container.FindEntry(EntryId)->State == Edemo_mapRuntimeContainerEntryState::Identified
		&& F.Items.FindInstance(ItemId)->OwnershipState == Edemo_mapItemOwnershipState::Container);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer24, "demo_map.SearchContainer.24.TakeNoEquipHotbarUse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer24::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Corpse, Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed()); F.Open();
	const auto* E = F.Entry(Edemo_mapRuntimeContainerSection::Equipment, 0); const FGuid ItemId = E->InternalItemInstanceId;
	F.Submit(Edemo_mapRuntimeContainerActionKind::Take, E->EntryId);
	const Fdemo_mapHotbarBindingSnapshot Hotbar;
	TestTrue(TEXT("Inventory only"), F.Items.FindInstance(ItemId)->OwnershipState == Edemo_mapItemOwnershipState::Inventory && !F.Items.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot).IsValid() && !Hotbar.SlotBindings.Contains(ItemId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer25, "demo_map.SearchContainer.25.CleanupOnlyUnclaimed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer25::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const auto First = F.Container.GetEntriesForAudit()[0]; const auto Second = F.Container.GetEntriesForAudit()[1]; F.Search(First.EntryId); F.Submit(Edemo_mapRuntimeContainerActionKind::Take, First.EntryId);
	const int32 Count = F.Container.CleanupUnclaimed([&F](FGuid Id){ return F.Items.DestroyContainer(Id, F.ContainerId); });
	TestTrue(TEXT("Only unclaimed destroyed"), Count == 1 && F.Items.FindInstance(First.InternalItemInstanceId)->OwnershipState == Edemo_mapItemOwnershipState::Inventory && F.Items.FindInstance(Second.InternalItemInstanceId)->OwnershipState == Edemo_mapItemOwnershipState::Destroyed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer26, "demo_map.SearchContainer.26.AllTerminalReasonsCancelAndReset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer26::RunTest(const FString&)
{
	for (Edemo_mapRunEndReason Reason : { Edemo_mapRunEndReason::Death, Edemo_mapRunEndReason::Extraction, Edemo_mapRunEndReason::Abandon, Edemo_mapRunEndReason::RecoveredAbandon })
	{
		FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open(); const FGuid Id = F.Container.GetEntriesForAudit()[0].EntryId; F.Submit(Edemo_mapRuntimeContainerActionKind::BeginSearch, Id);
		F.Container.CancelActiveAction(FString::FromInt(static_cast<int32>(Reason))); F.Container.CleanupUnclaimed([&F](FGuid Item){ return F.Items.DestroyContainer(Item, F.ContainerId); });
		TestTrue(TEXT("Terminal reset idle and empty"), !F.Container.IsActionActive() && F.Container.IsEmpty() && F.Container.FindEntry(Id)->State == Edemo_mapRuntimeContainerEntryState::Taken);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer27, "demo_map.SearchContainer.27.PresenterPureProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer27::RunTest(const FString&)
{
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	const auto Before = F.Container.GetRevision(); const auto Snapshot = F.Snapshot(); const auto A = Fdemo_mapSearchContainerPresenter::Build(Snapshot); const auto B = Fdemo_mapSearchContainerPresenter::Build(Snapshot);
	TestTrue(TEXT("Pure deterministic projection"), F.Container.GetRevision() == Before && A.Header == B.Header && A.Sections.Num() == 1 && A.Sections[0].Rows.Num() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer28, "demo_map.SearchContainer.28.SchemaProfileProductionUnchanged", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer28::RunTest(const FString&)
{
	const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
	const TArray<FString> Paths = { Production.PrimaryPath(), Production.BackupPath(), Production.TempPath() };
	TArray<FFileEvidence> Before; for (const FString& Path : Paths) Before.Add(ReadEvidence(Path));
	FSearchContainerFixture F; F.Initialize(Edemo_mapRuntimeContainerKind::Chest, Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0)); F.Open();
	Fdemo_mapPersistentProfile Profile;
	TestTrue(TEXT("Schema and default Profile economy remain unchanged"),
		Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
		&& Profile.SaveGeneration == 0
		&& Profile.PersistentSpiritStones == 0
		&& Profile.TownLevel == 0);
	for (int32 Index = 0; Index < Paths.Num(); ++Index)
	{
		const FFileEvidence After = ReadEvidence(Paths[Index]);
		TestTrue(TEXT("Production bytes and existence unchanged"), Before[Index].bExists == After.bExists && Before[Index].Bytes == After.Bytes);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer29, "demo_map.SearchContainer.29.UniqueIdentityStableSeedOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer29::RunTest(const FString&)
{
	FSearchContainerFixture A; FSearchContainerFixture B; const auto Seed = Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0); A.Initialize(Edemo_mapRuntimeContainerKind::Chest, Seed); B.Initialize(Edemo_mapRuntimeContainerKind::Chest, Seed);
	TestTrue(TEXT("Unique run identities"), A.ContainerId != B.ContainerId && A.RunId != B.RunId && A.ItemIds[0] != B.ItemIds[0]);
	TestTrue(TEXT("Stable definition/slot order"), A.Container.GetEntriesForAudit()[0].ExpectedDefinitionId == B.Container.GetEntriesForAudit()[0].ExpectedDefinitionId && A.Container.GetEntriesForAudit()[0].SlotIndex == B.Container.GetEntriesForAudit()[0].SlotIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSearchContainer30, "demo_map.SearchContainer.30.LooseWorldPickupUnaffected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSearchContainer30::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Items; FGuid Id; TestTrue(TEXT("Create loose world"), Items.CreateWorldDefinition(Fdemo_mapItemIds::SpiritWoodLevel1, 2, Id).bSuccess);
	TestTrue(TEXT("Existing pickup path"), Items.PickupWorld(Id).bSuccess);
	TestTrue(TEXT("World identity moves to inventory"), Items.FindInstance(Id) && Items.FindInstance(Id)->OwnershipState == Edemo_mapItemOwnershipState::Inventory);
	return true;
}

#endif
