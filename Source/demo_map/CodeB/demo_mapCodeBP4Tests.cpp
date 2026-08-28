// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CodeB/demo_mapCodeBP4.h"
#include "Misc/AutomationTest.h"

namespace
{
	using namespace demo_map_code_b;

	bool Open(FAutomationTestBase& Test, FCodeBP3UIController& Controller)
	{
		FString Error;
		const bool bOpened = Controller.Open(&Error);
		Test.TestTrue(TEXT("P4 controller opens the isolated P3 host"), bOpened);
		if (!bOpened) Test.AddError(Error);
		return bOpened;
	}

	bool MakeAddress(FAutomationTestBase& Test, const FCodeBP3UIController& Controller, const FGuid& ContainerId, const int32 Slot, FCodeBP3SlotAddress& OutAddress)
	{
		const bool bFound = Controller.MakeAddress(ContainerId, Slot, OutAddress);
		Test.TestTrue(TEXT("P4 reads a stable address from the authoritative P2 Projection"), bFound);
		return bFound;
	}

	bool Payload(FAutomationTestBase& Test, FCodeBP4InteractionController& Interaction, const FCodeBP3UIController& Controller, const FGuid& ContainerId, const int32 Slot, FCodeBP4DragPayload& OutPayload)
	{
		FCodeBP3SlotAddress Source;
		return MakeAddress(Test, Controller, ContainerId, Slot, Source) && Interaction.BeginDrag(Source, OutPayload);
	}

	const FCodeBP2SlotView* FindSlot(const FCodeBP2Projection& Projection, const FGuid& ContainerId, const int32 SlotIndex)
	{
		const FCodeBP2ContainerView* Container = Projection.Containers.FindByPredicate([&ContainerId](const FCodeBP2ContainerView& Candidate)
		{
			return Candidate.ContainerId == ContainerId;
		});
		return Container && Container->Slots.IsValidIndex(SlotIndex) ? &Container->Slots[SlotIndex] : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4PayloadTest, "demo_map.CodeB.P4.Payload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4PayloadTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload DragPayload;
	const int32 InitialRevision = Controller.GetProjection().Revision;
	if (!Ids || !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 6, DragPayload)) return false;
	TestTrue(TEXT("Payload is valid and contains a stable source"), DragPayload.IsValid());
	TestEqual(TEXT("Payload retains ItemId rather than an item instance"), DragPayload.ItemId, Ids->DustAItemId);
	TestEqual(TEXT("Payload records the source Revision"), DragPayload.ExpectedRevision, Controller.GetProjection().Revision);
	TestEqual(TEXT("Begin drag does not write a transaction"), Controller.GetProjection().Revision, InitialRevision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4InvalidSourceTest, "demo_map.CodeB.P4.InvalidSource", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4InvalidSourceTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload DragPayload;
	if (!Ids || !Payload(*this, Interaction, Controller, Ids->BasicContainerId, 0, DragPayload))
	{
		TestFalse(TEXT("Empty cells cannot create a valid P4 drag payload"), DragPayload.IsValid());
		return true;
	}
	return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4MovePreviewAndCommitTest, "demo_map.CodeB.P4.MovePreviewAndCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4MovePreviewAndCommitTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload DragPayload;
	FCodeBP3SlotAddress Target;
	const int32 InitialRevision = Controller.GetProjection().Revision;
	if (!Ids || !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 0, DragPayload) || !MakeAddress(*this, Controller, Ids->BasicContainerId, 0, Target)) return false;
	const FCodeBP4DropPreview Preview = Interaction.PreviewDrop(DragPayload, Target);
	TestTrue(TEXT("Storage-to-empty target previews a legal move"), Preview.bAllowed && Preview.Kind == ECodeBP4DropKind::Move);
	TestTrue(TEXT("Drop commits once through P3 to P2/P1"), Interaction.CommitDrop(DragPayload, Target));
	TestEqual(TEXT("Move increments Revision exactly once"), Controller.GetProjection().Revision, InitialRevision + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4MergePreviewAndCommitTest, "demo_map.CodeB.P4.MergePreviewAndCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4MergePreviewAndCommitTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload DragPayload;
	FCodeBP3SlotAddress Target;
	const int32 InitialRevision = Controller.GetProjection().Revision;
	if (!Ids || !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 6, DragPayload) || !MakeAddress(*this, Controller, Ids->WarehouseContainerId, 7, Target)) return false;
	const FCodeBP4DropPreview Preview = Interaction.PreviewDrop(DragPayload, Target);
	TestTrue(TEXT("Compatible complete stacks preview merge"), Preview.bAllowed && Preview.Kind == ECodeBP4DropKind::Merge);
	TestTrue(TEXT("Drag merge commits atomically"), Interaction.CommitDrop(DragPayload, Target));
	TestEqual(TEXT("Merge increments Revision exactly once"), Controller.GetProjection().Revision, InitialRevision + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4SwapPreviewAndCommitTest, "demo_map.CodeB.P4.SwapPreviewAndCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4SwapPreviewAndCommitTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload DragPayload;
	FCodeBP3SlotAddress Target;
	if (!Ids || !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 0, DragPayload) || !MakeAddress(*this, Controller, Ids->WarehouseContainerId, 1, Target)) return false;
	const FCodeBP4DropPreview Preview = Interaction.PreviewDrop(DragPayload, Target);
	TestTrue(TEXT("Occupied non-stack storage target previews swap"), Preview.bAllowed && Preview.Kind == ECodeBP4DropKind::Swap);
	TestTrue(TEXT("Drag swap commits atomically"), Interaction.CommitDrop(DragPayload, Target));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4EquipReplacementTest, "demo_map.CodeB.P4.EquipReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4EquipReplacementTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload FirstPayload;
	FCodeBP4DragPayload ReplacementPayload;
	FCodeBP3SlotAddress WeaponTarget;
	if (!Ids || !MakeAddress(*this, Controller, Ids->WeaponContainerId, 0, WeaponTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 0, FirstPayload)
		|| !Interaction.CommitDrop(FirstPayload, WeaponTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 1, ReplacementPayload)) return false;
	const FCodeBP4DropPreview Preview = Interaction.PreviewDrop(ReplacementPayload, WeaponTarget);
	TestTrue(TEXT("Occupied compatible equipment slot previews replacement"), Preview.bAllowed && Preview.Kind == ECodeBP4DropKind::Replacement);
	TestTrue(TEXT("Replacement commits through the same P2 route"), Interaction.CommitDrop(ReplacementPayload, WeaponTarget));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4UnequipAndIncompatibleTest, "demo_map.CodeB.P4.UnequipAndIncompatible", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4UnequipAndIncompatibleTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload WeaponPayload;
	FCodeBP4DragPayload InvalidPayload;
	FCodeBP3SlotAddress WeaponTarget;
	FCodeBP3SlotAddress EmptyStash;
	if (!Ids || !MakeAddress(*this, Controller, Ids->WeaponContainerId, 0, WeaponTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 0, WeaponPayload)
		|| !Interaction.CommitDrop(WeaponPayload, WeaponTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WeaponContainerId, 0, WeaponPayload)
		|| !MakeAddress(*this, Controller, Ids->WarehouseContainerId, 12, EmptyStash)) return false;
	TestTrue(TEXT("Equipment-to-storage previews unequip"), Interaction.PreviewDrop(WeaponPayload, EmptyStash).Kind == ECodeBP4DropKind::Unequip);
	TestTrue(TEXT("Unequip drag succeeds"), Interaction.CommitDrop(WeaponPayload, EmptyStash));
	if (!Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 9, InvalidPayload)) return false;
	TestFalse(TEXT("Invalid type target is visibly rejected before drop"), Interaction.PreviewDrop(InvalidPayload, WeaponTarget).bAllowed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4CancelAndStaleTest, "demo_map.CodeB.P4.CancelAndStale", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4CancelAndStaleTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload StalePayload;
	FCodeBP4DragPayload OtherPayload;
	FCodeBP3SlotAddress BasicTarget;
	if (!Ids || !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 0, StalePayload)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 1, OtherPayload)
		|| !MakeAddress(*this, Controller, Ids->BasicContainerId, 0, BasicTarget)
		|| !Interaction.CommitDrop(OtherPayload, BasicTarget)) return false;
	const int32 RevisionAfterOtherDrop = Controller.GetProjection().Revision;
	TestFalse(TEXT("Stale drag never auto-replays"), Interaction.CommitDrop(StalePayload, BasicTarget));
	TestEqual(TEXT("Stale drag preserves the newer Revision"), Controller.GetProjection().Revision, RevisionAfterOtherDrop);
	Interaction.CancelInteraction();
	TestEqual(TEXT("Cancellation writes no transaction"), Controller.GetProjection().Revision, RevisionAfterOtherDrop);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4ExplicitDragOnlyPolicyTest, "demo_map.CodeB.P4.ExplicitDragOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4ExplicitDragOnlyPolicyTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP3SlotAddress Dust;
	FCodeBP3SlotAddress ExplicitBasicTarget;
	FCodeBP4DragPayload Payload;
	if (!Ids || !MakeAddress(*this, Controller, Ids->WarehouseContainerId, 6, Dust)
		|| !MakeAddress(*this, Controller, Ids->BasicContainerId, 0, ExplicitBasicTarget)) return false;
	const int32 RevisionBeforeDrag = Controller.GetProjection().Revision;
	TestTrue(TEXT("A real source begins a drag without inferring any destination"), Interaction.BeginDrag(Dust, Payload));
	TestEqual(TEXT("Beginning a drag is zero-write"), Controller.GetProjection().Revision, RevisionBeforeDrag);
	TestTrue(TEXT("Only the explicitly supplied target receives an allowed preview"), Interaction.PreviewDrop(Payload, ExplicitBasicTarget).bAllowed);
	TestTrue(TEXT("Only the explicit drag/drop commit changes the location"), Interaction.CommitDrop(Payload, ExplicitBasicTarget));
	TestEqual(TEXT("The explicit drop increments the revision exactly once"), Controller.GetProjection().Revision, RevisionBeforeDrag + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4LoadedSpatialAndCloseTest, "demo_map.CodeB.P4.LoadedSpatialAndClose", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4LoadedSpatialAndCloseTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload SpatialPayload;
	FCodeBP4DragPayload DustPayload;
	FCodeBP3SlotAddress SpatialTarget;
	FCodeBP3SlotAddress InternalTarget;
	FCodeBP3SlotAddress StashTarget;
	if (!Ids || !MakeAddress(*this, Controller, Ids->SpatialContainerId, 0, SpatialTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 5, SpatialPayload)
		|| !Interaction.CommitDrop(SpatialPayload, SpatialTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 6, DustPayload)
		|| !MakeAddress(*this, Controller, Ids->SpatialInternalContainerId, 0, InternalTarget)
		|| !Interaction.CommitDrop(DustPayload, InternalTarget)
		|| !Payload(*this, Interaction, Controller, Ids->SpatialContainerId, 0, SpatialPayload)
		|| !MakeAddress(*this, Controller, Ids->WarehouseContainerId, 5, StashTarget)) return false;
	const FCodeBP2SlotView* ParentBefore = FindSlot(Controller.GetProjection(), Ids->SpatialContainerId, 0);
	const FGuid ChildContainerId = ParentBefore ? ParentBefore->ChildContainerId : FGuid();
	const int32 RevisionBeforeMove = Controller.GetProjection().Revision;
	TestTrue(TEXT("Loaded spatial item moves as one complete graph"), Interaction.CommitDrop(SpatialPayload, StashTarget));
	const FCodeBP2SlotView* ParentAfter = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 5);
	const FCodeBP2SlotView* ChildAfter = FindSlot(Controller.GetProjection(), ChildContainerId, 0);
	TestEqual(TEXT("Loaded spatial whole-graph move increments Revision once"), Controller.GetProjection().Revision, RevisionBeforeMove + 1);
	TestTrue(TEXT("Loaded spatial parent preserves identity"), ParentAfter && ParentAfter->ItemId == Ids->SpatialItemId && ParentAfter->ChildContainerId == ChildContainerId);
	TestTrue(TEXT("Loaded spatial child preserves placement"), ChildAfter && ChildAfter->ItemId == Ids->DustAItemId);
	Controller.Close();
	TestFalse(TEXT("Closed page payload cannot submit"), Interaction.CommitDrop(SpatialPayload, StashTarget));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeBP4SpatialPouchPolicyTest, "demo_map.CodeB.P4.SpatialPouchPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodeBP4SpatialPouchPolicyTest::RunTest(const FString& Parameters)
{
	FCodeBP3UIController Controller;
	if (!Open(*this, Controller)) return false;
	const FCodeBP2FixtureIds* Ids = Controller.GetFixtureIds();
	FCodeBP4InteractionController Interaction(Controller);
	FCodeBP4DragPayload PouchPayload;
	FCodeBP4DragPayload EmptyPouchPayload;
	FCodeBP4DragPayload DustPayload;
	FCodeBP3SlotAddress BasicTarget;
	FCodeBP3SlotAddress PouchInternalTarget;
	FCodeBP3SlotAddress RingTarget;
	FCodeBP3SlotAddress WarehouseTarget;
	if (!Ids
		|| !MakeAddress(*this, Controller, Ids->BasicContainerId, 0, BasicTarget)
		|| !Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 10, PouchPayload)
		|| !Interaction.CommitDrop(PouchPayload, BasicTarget)
		|| !MakeAddress(*this, Controller, Ids->SpatialContainerId, 0, RingTarget)) return false;
	if (!Payload(*this, Interaction, Controller, Ids->BasicContainerId, 0, EmptyPouchPayload)) return false;
	TestFalse(TEXT("Ordinary spatial pouch is not compatible with the spatial-ring equipment slot"), Interaction.PreviewDrop(EmptyPouchPayload, RingTarget).bAllowed);
	if (!Payload(*this, Interaction, Controller, Ids->WarehouseContainerId, 6, DustPayload)
		|| !MakeAddress(*this, Controller, Ids->PouchInternalContainerId, 0, PouchInternalTarget)
		|| !Interaction.CommitDrop(DustPayload, PouchInternalTarget)
		|| !Payload(*this, Interaction, Controller, Ids->BasicContainerId, 0, PouchPayload)
		|| !MakeAddress(*this, Controller, Ids->WarehouseContainerId, 10, WarehouseTarget)) return false;
	const FCodeBP2SlotView* PouchBefore = FindSlot(Controller.GetProjection(), Ids->BasicContainerId, 0);
	const FGuid PouchChildContainerId = PouchBefore ? PouchBefore->ChildContainerId : FGuid();
	const int32 RevisionBeforeLoadedMove = Controller.GetProjection().Revision;
	TestTrue(TEXT("Loaded ordinary spatial pouch moves as a whole"), Interaction.CommitDrop(PouchPayload, WarehouseTarget));
	const FCodeBP2SlotView* PouchAfter = FindSlot(Controller.GetProjection(), Ids->WarehouseContainerId, 10);
	const FCodeBP2SlotView* ChildAfter = FindSlot(Controller.GetProjection(), PouchChildContainerId, 0);
	TestEqual(TEXT("Loaded pouch move increments the authoritative Revision once"), Controller.GetProjection().Revision, RevisionBeforeLoadedMove + 1);
	TestTrue(TEXT("Loaded pouch preserves parent and child-container identity"), PouchAfter && PouchAfter->ItemId == PouchPayload.ItemId && PouchAfter->ChildContainerId == PouchChildContainerId);
	TestTrue(TEXT("Loaded pouch preserves child placement"), ChildAfter && ChildAfter->ItemId == Ids->DustAItemId);
	return true;
}

#endif
