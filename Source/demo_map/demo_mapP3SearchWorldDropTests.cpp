#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapRuntimeContainer.h"
#include "Engine/GameInstance.h"

namespace
{
	struct FP3SearchFixture
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		Udemo_mapItemSubsystem* Items =
			NewObject<Udemo_mapItemSubsystem>(GameInstance);
		Fdemo_mapRuntimeContainerAuthority Container;
		FGuid ContainerId = FGuid::NewGuid();
		FGuid ContainerItemId;
		FGuid EntryId;

		bool Initialize(
			FAutomationTestBase& Test,
			bool bAllowPlayerDeposit = true)
		{
			if (!Test.TestTrue(TEXT("P3 run becomes active"),
				Items->BeginRun().bSuccess))
			{
				return false;
			}
			if (!Test.TestTrue(TEXT("P3 creates one Container-owned item"),
				Items->CreateContainerItem(
					ContainerId,
					Fdemo_mapItemIds::AncientToken,
					1,
					ContainerItemId).bSuccess))
			{
				return false;
			}
			Fdemo_mapRuntimeContainerResolvedSeedEntry Seed;
			Seed.Section = Edemo_mapRuntimeContainerSection::Chest;
			Seed.SlotIndex = 0;
			Seed.ItemInstanceId = ContainerItemId;
			Seed.DefinitionId = Fdemo_mapItemIds::AncientToken;
			Seed.StackCount = 1;
			Seed.SearchDurationSeconds =
				Fdemo_mapSearchContainerPrototypeConfig::ChestEntrySearchSeconds;
			FString Diagnostic;
			if (!Test.TestTrue(TEXT("P3 container initializes"),
				Container.Initialize(
					ContainerId,
					Items->GetActiveRunId(),
					Edemo_mapRuntimeContainerKind::Chest,
					{ Seed },
					Diagnostic,
					bAllowPlayerDeposit)))
			{
				return false;
			}
			Fdemo_mapRuntimeContainerIntent Open;
			Open.ExpectedRunId = Items->GetActiveRunId();
			Open.ContainerId = ContainerId;
			Open.ExpectedRevision = Container.GetRevision();
			Open.Action = Edemo_mapRuntimeContainerActionKind::BeginOpen;
			if (!Test.TestTrue(TEXT("P3 container opens"),
				Container.SubmitIntent(
					Open,
					true,
					true,
					[](FGuid ItemId)
					{
						return Fdemo_mapItemOperationResult::Success(ItemId);
					}).bSuccess
					&& Container.CompleteActiveAction().bSuccess))
			{
				return false;
			}
			EntryId = Container.GetEntriesForAudit()[0].EntryId;
			Fdemo_mapRuntimeContainerIntent Search;
			Search.ExpectedRunId = Items->GetActiveRunId();
			Search.ContainerId = ContainerId;
			Search.ExpectedRevision = Container.GetRevision();
			Search.EntryId = EntryId;
			Search.Action = Edemo_mapRuntimeContainerActionKind::BeginSearch;
			return EntryId.IsValid()
				&& Test.TestTrue(TEXT("P3 source becomes identified"),
					Container.SubmitIntent(
						Search,
						true,
						true,
						[](FGuid ItemId)
						{
							return Fdemo_mapItemOperationResult::Success(ItemId);
						}).bSuccess
						&& Container.CompleteActiveAction().bSuccess);
		}

		Fdemo_mapSearchContainerDropIntent BaseIntent() const
		{
			Fdemo_mapSearchContainerDropIntent Intent;
			Intent.ExpectedRunId = Items->GetActiveRunId();
			Intent.ContainerId = ContainerId;
			Intent.ExpectedContainerRevision = Container.GetRevision();
			Intent.ExpectedAuthorityRevision =
				Items->GetAuthority().GetAuthorityRevision();
			return Intent;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapP3SearchDragDropTransaction,
	"demo_map.P3.SearchWorldDrop.SearchDragDropAtomicAndStaleReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapP3SearchDragDropTransaction::RunTest(const FString&)
{
	FP3SearchFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fdemo_mapSearchContainerDropIntent Take = Fixture.BaseIntent();
	Take.bSourceIsContainer = true;
	Take.SourceEntryId = Fixture.EntryId;
	Take.ExpectedSourceItemInstanceId = Fixture.ContainerItemId;
	Take.TargetPlayerArea = Edemo_mapPlayerItemArea::BaseQuickItems;
	Take.TargetPlayerSlotIndex = 0;
	const Fdemo_mapSearchContainerDropResult Taken =
		Fixture.Items->ExecuteSearchContainerDrop(Take, Fixture.Container);
	TestTrue(TEXT("P3 container-to-player is one committed move"),
		Taken.IsSuccess()
		&& Taken.Kind == Edemo_mapPlayerItemDropKind::Move
		&& Fixture.Items->GetAuthority().FindInventorySlot(
			Fixture.ContainerItemId) == 0
		&& Fixture.Container.GetEntriesForAudit()[0].State
			== Edemo_mapRuntimeContainerEntryState::Taken);

	const Fdemo_mapItemAuthorityState BeforeStale =
		Fixture.Items->GetAuthority().CaptureState();
	const int32 ContainerRevisionBeforeStale = Fixture.Container.GetRevision();
	const Fdemo_mapSearchContainerDropResult Stale =
		Fixture.Items->ExecuteSearchContainerDrop(Take, Fixture.Container);
	TestTrue(TEXT("P3 stale dual revision rejects without mutation"),
		!Stale.IsSuccess()
		&& Stale.Kind == Edemo_mapPlayerItemDropKind::Reject
		&& Fixture.Items->GetAuthority().CaptureState().InventorySlots
			== BeforeStale.InventorySlots
		&& Fixture.Container.GetRevision() == ContainerRevisionBeforeStale);

	Fdemo_mapSearchContainerDropIntent PutBack = Fixture.BaseIntent();
	PutBack.bSourceIsContainer = false;
	PutBack.ExpectedSourceItemInstanceId = Fixture.ContainerItemId;
	PutBack.SourcePlayerArea = Edemo_mapPlayerItemArea::BaseQuickItems;
	PutBack.SourcePlayerSlotIndex = 0;
	PutBack.TargetSection = Edemo_mapRuntimeContainerSection::Chest;
	PutBack.TargetContainerSlotIndex = 1;
	const Fdemo_mapSearchContainerDropResult Returned =
		Fixture.Items->ExecuteSearchContainerDrop(PutBack, Fixture.Container);
	const Fdemo_mapRuntimeContainerEntryRecord* ReturnedEntry =
		Fixture.Container.GetEntriesForAudit().FindByPredicate(
			[](const Fdemo_mapRuntimeContainerEntryRecord& Entry)
			{
				return Entry.Section == Edemo_mapRuntimeContainerSection::Chest
					&& Entry.SlotIndex == 1;
			});
	FString InvariantError;
	TestTrue(TEXT("P3 player-to-search target commits and keeps one authority"),
		Returned.IsSuccess()
		&& Returned.Kind == Edemo_mapPlayerItemDropKind::Move
		&& Fixture.Items->GetAuthority().FindInventorySlot(
			Fixture.ContainerItemId) == INDEX_NONE
		&& ReturnedEntry
		&& ReturnedEntry->InternalItemInstanceId == Fixture.ContainerItemId
		&& ReturnedEntry->State == Edemo_mapRuntimeContainerEntryState::Identified
		&& Fixture.Items->GetAuthority().ValidateInvariants(&InvariantError));

	FP3SearchFixture ResourceFixture;
	if (!ResourceFixture.Initialize(*this, false))
	{
		return false;
	}
	TArray<FGuid> PlayerItems;
	ResourceFixture.Items->AddDefinition(
		Fdemo_mapItemIds::HealingPillLevel1,
		1,
		&PlayerItems);
	const FGuid PlayerItemId = PlayerItems.IsEmpty() ? FGuid() : PlayerItems[0];
	Fdemo_mapSearchContainerDropIntent ResourcePut =
		ResourceFixture.BaseIntent();
	ResourcePut.ExpectedSourceItemInstanceId = PlayerItemId;
	ResourcePut.SourcePlayerArea = Edemo_mapPlayerItemArea::BaseQuickItems;
	ResourcePut.SourcePlayerSlotIndex = ResourceFixture.Items
		->GetAuthority().FindInventorySlot(PlayerItemId);
	ResourcePut.TargetSection = Edemo_mapRuntimeContainerSection::Chest;
	ResourcePut.TargetContainerSlotIndex = 1;
	const Fdemo_mapSearchContainerDropResult ResourceRejected =
		ResourceFixture.Items->ExecuteSearchContainerDrop(
			ResourcePut,
			ResourceFixture.Container);
	TestTrue(TEXT("P3 resource container is authoritatively take-only"),
		!ResourceRejected.IsSuccess()
		&& ResourceRejected.Kind == Edemo_mapPlayerItemDropKind::Reject
		&& ResourceFixture.Items->GetAuthority().FindInventorySlot(PlayerItemId)
			!= INDEX_NONE);
	return true;
}

#endif
