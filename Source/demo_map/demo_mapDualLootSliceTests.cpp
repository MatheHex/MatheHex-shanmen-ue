#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardSourceProjection.h"
#include "demo_mapRuntimeContainer.h"
#include "demo_mapSearchContainerPresenter.h"

namespace
{
	struct FP6DualLootFixture
	{
		Fdemo_mapItemAuthority Items;
		Fdemo_mapRuntimeContainerAuthority Container;
		FGuid ContainerId = FGuid::NewGuid();
		FGuid RunId = FGuid::NewGuid();

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

		Fdemo_mapRuntimeContainerResult Submit(
			Edemo_mapRuntimeContainerActionKind Action,
			FGuid EntryId = FGuid())
		{
			Fdemo_mapRuntimeContainerIntent Intent;
			Intent.ExpectedRunId = RunId;
			Intent.ContainerId = ContainerId;
			Intent.ExpectedRevision = Container.GetRevision();
			Intent.EntryId = EntryId;
			Intent.Action = Action;
			return Container.SubmitIntent(
				Intent,
				true,
				true,
				[this](FGuid ItemId)
				{
					return Items.TransferContainerToInventoryWhole(
						ItemId,
						ContainerId);
				});
		}

		bool Open()
		{
			return Submit(
					Edemo_mapRuntimeContainerActionKind::BeginOpen)
					.bSuccess
				&& Container.CompleteActiveAction().bSuccess;
		}

		Fdemo_mapRuntimeContainerSnapshot Snapshot(
			float Progress01 = 0.0f) const
		{
			return Container.BuildSnapshot(
				Items,
				Progress01,
				Items.GetUsedInventorySlots(),
				Items.GetInventoryCapacity());
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

		Fdemo_mapSearchContainerViewState RuntimeView() const
		{
			return Fdemo_mapSearchContainerPresenter::BuildRuntime(
				Snapshot(),
				Items,
				Fdemo_mapHotbarBindingSnapshot(),
				Fdemo_mapItemUseCooldownSnapshot());
		}
	};

	const Fdemo_mapDualLootTargetRegionView* FindRegion(
		const Fdemo_mapSearchContainerViewState& View,
		Edemo_mapDualLootTargetRegion Region)
	{
		return View.TargetRegions.FindByPredicate(
			[Region](
				const Fdemo_mapDualLootTargetRegionView& Candidate)
			{
				return Candidate.Region == Region;
			});
	}

	Fdemo_mapRewardPlannedStack Planned(
		FName DefinitionId,
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex)
	{
		Fdemo_mapRewardPlannedStack Stack;
		Stack.DefinitionId = DefinitionId;
		Stack.StackCount = 1;
		Stack.Section = Section;
		Stack.SlotIndex = SlotIndex;
		return Stack;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot01OrdinaryContainer,
	"demo_map.P6.DualLoot.01.OrdinaryContainerTarget",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot01OrdinaryContainer::RunTest(const FString&)
{
	FP6DualLootFixture Fixture;
	TestTrue(
		TEXT("Chest initializes and opens"),
		Fixture.Initialize(
			Edemo_mapRuntimeContainerKind::Chest,
			Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0))
			&& Fixture.Open());
	const Fdemo_mapSearchContainerViewState View = Fixture.RuntimeView();
	const Fdemo_mapDualLootTargetRegionView* Grid =
		FindRegion(View, Edemo_mapDualLootTargetRegion::ContainerGrid);
	TestTrue(
		TEXT("Ordinary target is dual-sided and exposes only six container cells"),
		View.bDualSided
			&& View.bOrdinaryContainer
			&& !View.bCorpse
			&& View.TargetRegions.Num() == 1
			&& Grid
			&& Grid->Capacity
				== Fdemo_mapSearchContainerPrototypeConfig::
					ChestPrototypeCapacity
			&& View.PlayerLoadout.bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot02CorpseWithoutSpatialItem,
	"demo_map.P6.DualLoot.02.CorpseWithoutSpatialItem",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot02CorpseWithoutSpatialItem::RunTest(const FString&)
{
	FP6DualLootFixture Fixture;
	TestTrue(
		TEXT("Prototype corpse opens"),
		Fixture.Initialize(
			Edemo_mapRuntimeContainerKind::Corpse,
			Fdemo_mapSearchContainerPrototypeConfig::
				BuildPrototypeCorpseSeed())
			&& Fixture.Open());
	const Fdemo_mapSearchContainerViewState View = Fixture.RuntimeView();
	const Fdemo_mapDualLootTargetRegionView* Weapon =
		FindRegion(View, Edemo_mapDualLootTargetRegion::Weapon);
	const Fdemo_mapDualLootTargetRegionView* BaseQuick =
		FindRegion(View, Edemo_mapDualLootTargetRegion::BaseQuickItems);
	const Fdemo_mapDualLootTargetRegionView* Body =
		FindRegion(View, Edemo_mapDualLootTargetRegion::Body);
	TestTrue(
		TEXT("Corpse has direct equipment, fixed six, body, and no phantom spatial storage"),
		View.bCorpse
			&& Weapon
			&& Weapon->Cells.Num() == 1
			&& Weapon->Cells[0].State
				== Edemo_mapRuntimeContainerEntryState::Identified
			&& BaseQuick
			&& BaseQuick->Capacity == 6
			&& Body
			&& Body->Capacity == 2
			&& FindRegion(
				View,
				Edemo_mapDualLootTargetRegion::SpatialStorage)
				== nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot03CorpseSpatialItemCapacity,
	"demo_map.P6.DualLoot.03.CorpseSpatialItemCapacity",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot03CorpseSpatialItemCapacity::RunTest(const FString&)
{
	FP6DualLootFixture Fixture;
	const int32 SpatialItemSlotIndex =
		Fdemo_mapItemDefinitions::GetEquipmentSlotIds().IndexOfByKey(
			Fdemo_mapItemIds::BackpackSlot);
	TestTrue(
		TEXT("Backpack has a canonical corpse equipment slot"),
		SpatialItemSlotIndex != INDEX_NONE);
	const TArray<Fdemo_mapRuntimeContainerSeedEntry> Seed = {
		{
			Edemo_mapRuntimeContainerSection::Equipment,
			SpatialItemSlotIndex,
			Fdemo_mapItemIds::BackpackLevel1,
			1
		},
		{
			Edemo_mapRuntimeContainerSection::Backpack,
			0,
			Fdemo_mapItemIds::HealingPillLevel1,
			1
		}
	};
	TestTrue(
		TEXT("Spatial corpse opens"),
		Fixture.Initialize(
			Edemo_mapRuntimeContainerKind::Corpse,
			Seed)
			&& Fixture.Open());
	const Fdemo_mapSearchContainerViewState View = Fixture.RuntimeView();
	const Fdemo_mapDualLootTargetRegionView* SpatialItem =
		FindRegion(View, Edemo_mapDualLootTargetRegion::SpatialItem);
	const Fdemo_mapDualLootTargetRegionView* SpatialStorage =
		FindRegion(View, Edemo_mapDualLootTargetRegion::SpatialStorage);
	const Fdemo_mapSpatialStorageCapacityResult ExpectedStorage =
		Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
			Fdemo_mapItemIds::BackpackLevel1);
	TestTrue(
		TEXT("Identified spatial item exposes Definition-backed dynamic storage"),
		ExpectedStorage.bSuccess
			&& SpatialItem
			&& SpatialItem->Cells.Num() == 1
			&& SpatialItem->Cells[0].State
				== Edemo_mapRuntimeContainerEntryState::Identified
			&& SpatialStorage
			&& SpatialStorage->Capacity == ExpectedStorage.Capacity
			&& SpatialStorage->FirstSourceSlotIndex
				== Fdemo_mapSearchContainerPrototypeConfig::
					CorpseBaseQuickItemCapacity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot04EquipmentSlotCanonicalization,
	"demo_map.P6.DualLoot.04.EquipmentSlotCanonicalization",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot04EquipmentSlotCanonicalization::RunTest(const FString&)
{
	Fdemo_mapRewardSourceProjectionResult Result;
	Result.Status = Edemo_mapRewardGenerationStatus::Success;
	Result.PlannedStacks = {
		Planned(
			Fdemo_mapItemIds::WeaponLevel1,
			Edemo_mapRuntimeContainerSection::Equipment,
			0),
		Planned(
			Fdemo_mapItemIds::WeaponLevel2,
			Edemo_mapRuntimeContainerSection::Equipment,
			1),
		Planned(
			Fdemo_mapItemIds::ArmorRobeLevel1,
			Edemo_mapRuntimeContainerSection::Equipment,
			2),
		Planned(
			Fdemo_mapItemIds::EvasionCharm,
			Edemo_mapRuntimeContainerSection::Equipment,
			3),
		Planned(
			Fdemo_mapItemIds::AccessoryLevel1,
			Edemo_mapRuntimeContainerSection::Equipment,
			4),
		Planned(
			Fdemo_mapItemIds::BackpackLevel1,
			Edemo_mapRuntimeContainerSection::Equipment,
			5)
	};
	const TArray<Fdemo_mapRuntimeContainerSeedEntry> Seed =
		Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(Result);
	TSet<int32> EquipmentSlots;
	int32 BackpackCount = 0;
	for (const Fdemo_mapRuntimeContainerSeedEntry& Entry : Seed)
	{
		if (Entry.Section
			== Edemo_mapRuntimeContainerSection::Equipment)
		{
			EquipmentSlots.Add(Entry.SlotIndex);
		}
		else if (Entry.Section
			== Edemo_mapRuntimeContainerSection::Backpack)
		{
			++BackpackCount;
		}
	}
	const int32 CanonicalEquipmentRoleCount =
		Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num();
	bool bEveryCanonicalEquipmentSlotPresent =
		EquipmentSlots.Num() == CanonicalEquipmentRoleCount;
	for (int32 SlotIndex = 0;
		bEveryCanonicalEquipmentSlotPresent
			&& SlotIndex < CanonicalEquipmentRoleCount;
		++SlotIndex)
	{
		bEveryCanonicalEquipmentSlotPresent =
			EquipmentSlots.Contains(SlotIndex);
	}
	TestTrue(
		TEXT("Each equipment role is 0-1 and duplicate weapon becomes ordinary carried loot"),
		Seed.Num() == Result.PlannedStacks.Num()
			&& bEveryCanonicalEquipmentSlotPresent
			&& BackpackCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot05RewardMetadataPreserved,
	"demo_map.P6.DualLoot.05.RewardMetadataPreserved",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot05RewardMetadataPreserved::RunTest(const FString&)
{
	Fdemo_mapRewardSourceProjectionResult Result;
	Result.Status = Edemo_mapRewardGenerationStatus::Success;
	Fdemo_mapRewardPlannedStack Stack = Planned(
		Fdemo_mapItemIds::WeaponLevel2,
		Edemo_mapRuntimeContainerSection::Equipment,
		0);
	Stack.StackCount = 1;
	Stack.RewardEventKind = Edemo_mapRewardEventKind::Jackpot;
	Stack.RewardEventId = FGuid::NewGuid();
	Stack.RewardValueMultiplierBps = 60000;
	Stack.RareRewardEventId = FGuid::NewGuid();
	Stack.RareRewardBonusValue = 1234;
	Result.PlannedStacks.Add(Stack);
	const TArray<Fdemo_mapRuntimeContainerSeedEntry> Seed =
		Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(Result);
	TestTrue(
		TEXT("Canonical projection changes location only"),
		Seed.Num() == 1
			&& Seed[0].DefinitionId == Stack.DefinitionId
			&& Seed[0].StackCount == Stack.StackCount
			&& Seed[0].RewardEventKind == Stack.RewardEventKind
			&& Seed[0].RewardEventId == Stack.RewardEventId
			&& Seed[0].RewardValueMultiplierBps
				== Stack.RewardValueMultiplierBps
			&& Seed[0].RareRewardEventId == Stack.RareRewardEventId
			&& Seed[0].RareRewardBonusValue
				== Stack.RareRewardBonusValue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot06SameGuidWholeTransfer,
	"demo_map.P6.DualLoot.06.SameGuidWholeTransfer",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot06SameGuidWholeTransfer::RunTest(const FString&)
{
	FP6DualLootFixture Fixture;
	TestTrue(
		TEXT("Corpse opens"),
		Fixture.Initialize(
			Edemo_mapRuntimeContainerKind::Corpse,
			Fdemo_mapSearchContainerPrototypeConfig::
				BuildPrototypeCorpseSeed())
			&& Fixture.Open());
	const Fdemo_mapRuntimeContainerEntryRecord* Equipment =
		Fixture.Entry(
			Edemo_mapRuntimeContainerSection::Equipment,
			0);
	const FGuid BeforeId =
		Equipment ? Equipment->InternalItemInstanceId : FGuid();
	const Fdemo_mapItemInstance* Before =
		Fixture.Items.FindInstance(BeforeId);
	const FName BeforeDefinition =
		Before ? Before->DefinitionId : NAME_None;
	const int32 BeforeQuantity = Before ? Before->Quantity : 0;
	const Fdemo_mapRuntimeContainerResult Take =
		Fixture.Submit(
			Edemo_mapRuntimeContainerActionKind::Take,
			Equipment ? Equipment->EntryId : FGuid());
	const Fdemo_mapItemInstance* After =
		Fixture.Items.FindInstance(BeforeId);
	TestTrue(
		TEXT("Take preserves GUID, Definition, quantity, and moves the whole instance"),
		Take.bSuccess
			&& Take.ItemInstanceId == BeforeId
			&& After
			&& After->InstanceId == BeforeId
			&& After->DefinitionId == BeforeDefinition
			&& After->Quantity == BeforeQuantity
			&& After->OwnershipState
				== Edemo_mapItemOwnershipState::Inventory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot07CapacityRejection,
	"demo_map.P6.DualLoot.07.CapacityRejection",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot07CapacityRejection::RunTest(const FString&)
{
	FP6DualLootFixture Fixture;
	TestTrue(
		TEXT("Chest opens"),
		Fixture.Initialize(
			Edemo_mapRuntimeContainerKind::Chest,
			Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0))
			&& Fixture.Open());
	for (int32 Index = 0; Index < 6; ++Index)
	{
		TestTrue(
			TEXT("Fill base carried slot"),
			Fixture.Items.AddDefinition(
				Fdemo_mapItemIds::WeaponLevel1,
				1).bSuccess);
	}
	const Fdemo_mapRuntimeContainerEntryRecord* Entry =
		Fixture.Entry(
			Edemo_mapRuntimeContainerSection::Chest,
			0);
	TestTrue(
		TEXT("Entry search completes"),
		Fixture.Submit(
			Edemo_mapRuntimeContainerActionKind::BeginSearch,
			Entry ? Entry->EntryId : FGuid()).bSuccess
			&& Fixture.Container.CompleteActiveAction().bSuccess);
	const Fdemo_mapRuntimeContainerResult Take =
		Fixture.Submit(
			Edemo_mapRuntimeContainerActionKind::Take,
			Entry ? Entry->EntryId : FGuid());
	TestTrue(
		TEXT("Full inventory rejects without changing source state"),
		!Take.bSuccess
			&& Take.Code
				== Edemo_mapRuntimeContainerResultCode::
					ItemAuthorityRejected
			&& Fixture.Entry(
				Edemo_mapRuntimeContainerSection::Chest,
				0)->State
				== Edemo_mapRuntimeContainerEntryState::Identified);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP6DualLoot08CancelAndClose,
	"demo_map.P6.DualLoot.08.CancelAndClose",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP6DualLoot08CancelAndClose::RunTest(const FString&)
{
	FP6DualLootFixture Fixture;
	TestTrue(
		TEXT("Chest opens"),
		Fixture.Initialize(
			Edemo_mapRuntimeContainerKind::Chest,
			Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(0))
			&& Fixture.Open());
	const Fdemo_mapRuntimeContainerEntryRecord* Entry =
		Fixture.Entry(
			Edemo_mapRuntimeContainerSection::Chest,
			0);
	const FGuid EntryId = Entry ? Entry->EntryId : FGuid();
	TestTrue(
		TEXT("Search starts"),
		Fixture.Submit(
			Edemo_mapRuntimeContainerActionKind::BeginSearch,
			EntryId).bSuccess);
	TestTrue(
		TEXT("Cancel restores hidden state and clears the active action"),
		Fixture.Submit(
			Edemo_mapRuntimeContainerActionKind::CancelSearch,
			EntryId).bSuccess
			&& !Fixture.Container.IsActionActive()
			&& Fixture.Entry(
				Edemo_mapRuntimeContainerSection::Chest,
				0)->State
				== Edemo_mapRuntimeContainerEntryState::Hidden);
	TestTrue(
		TEXT("Close is idempotent after cancel"),
		Fixture.Submit(
			Edemo_mapRuntimeContainerActionKind::Close).bSuccess
			&& !Fixture.Container.IsActionActive());
	return true;
}

#endif
