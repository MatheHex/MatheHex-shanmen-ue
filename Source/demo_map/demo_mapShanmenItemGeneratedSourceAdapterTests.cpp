#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenItemGeneratedSourceAdapter.h"
#include "demo_mapShanmenItemDefinitionAdapter.h"
#include "demo_mapShanmenItemMetadataAdapter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapM01RewardDistribution.h"
#include "demo_mapRewardFullMapDistribution.h"
#include "ShanmenItemGeneratedSourceCodec.h"
#include "ShanmenItemTags.h"
#include "ShanmenItemRepository.h"

namespace
{
	FShanmenContentStamp ItemContent()
	{
		FShanmenContentStamp C;
		C.Version = TEXT("Shanmen.Items.0.0.10"); C.Digest = TEXT("Shanmen.Items.LegacyAuthority.Schema1.v1");
		return C;
	}
	const FGuid Owner(17, 29, 31, 43);
	const FGuid Run(53, 67, 79, 83);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceAdapterSlotsTest,
	"Shanmen.0_0_10.Items.GeneratedSourceAdapter.CanonicalSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceAdapterSlotsTest::RunTest(const FString&)
{
	int32 Count = 0;
	TSet<FGuid> SourceIds;
	auto Check = [&](const Fdemo_mapRewardSourceProjection& Projection)
	{
		FShanmenItemGeneratedSourceRequest Request, Again;
		FString Diagnostic;
		const bool Built = Fdemo_mapShanmenItemGeneratedSourceAdapter::BuildRequest(
			ItemContent(), Owner, Run, Projection.SlotId, 7, 2, Request, Diagnostic);
		if (!TestTrue(*FString::Printf(TEXT("%s candidate: %s"), *Projection.SlotId.ToString(), *Diagnostic), Built)) { return; }
		TestTrue(TEXT("Current source content, nonzero cursor/pity, original policy binding"),
			Request.Plan.IsValid() && Request.Plan.OwnerId == Owner && Request.Plan.RunId == Run
			&& Request.Plan.ExpectedSequence == 7 && Request.Plan.PityStateBefore == 2
			&& Request.Plan.SourceRoleId == Projection.StableSourceRoleId
			&& Request.Plan.DistributionProfileId == Projection.DistributionProfileId
			&& Request.Plan.MarkerId == Projection.MarkerId && Request.Plan.EncounterId == Projection.EncounterId
			&& Request.Plan.JackpotPolicyId == Projection.JackpotPolicyId
			&& Request.Plan.RareExtremePolicyId == Projection.RareExtremePolicyId
			&& Request.Plan.AffixPolicyId == Projection.AffixPolicyId
			&& Fdemo_mapItemDefinitions::IsCurrentContentIdentity(Request.Plan.Content.Version, Request.Plan.Content.Digest));
		TestTrue(TEXT("Identical fresh planning inputs are deterministic"),
			Fdemo_mapShanmenItemGeneratedSourceAdapter::BuildRequest(ItemContent(), Owner, Run,
				Projection.SlotId, 7, 2, Again, Diagnostic) && Again.Plan == Request.Plan);
		const auto Planned = Fdemo_mapRewardSourceProjectionPlanner::Plan(Projection, Run, 2);
		const auto Seed = Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(Planned);
		TestEqual(TEXT("Complete seed, not a seed-only receipt"), Request.Plan.Entries.Num(), Seed.Num());
		for (int32 I = 0; I < Request.Plan.Entries.Num(); ++I)
		{
			const auto& E = Request.Plan.Entries[I];
			FShanmenItemRewardMetadata Metadata;
			TestTrue(TEXT("Metadata validates against the single existing product policy codec"),
				Fdemo_mapShanmenItemMetadataAdapter::FromPlannedStack(Planned.PlannedStacks[I], Metadata, Diagnostic)
				&& Metadata == E.RewardMetadata);
			TestTrue(TEXT("Exact quantity/value/order and canonical seed slot"), E.Definition.DefinitionId == Seed[I].DefinitionId
				&& E.Quantity == Seed[I].StackCount && E.SlotIndex == Seed[I].SlotIndex
				&& E.UnitValue == Planned.PlannedStacks[I].UnitValue && E.TotalValue == Planned.PlannedStacks[I].TotalValue);
		}
		FShanmenItemGeneratedSourceView View;
		View.OwnerId = Owner; View.RunId = Run; View.SourceContent = Request.Plan.Content;
		View.State = EShanmenItemGeneratedSourceRunState::Active; View.AcceptedSequence = 7; View.PityState = 2;
		const auto Accepted = FShanmenItemGeneratedSourceContract::Evaluate(View, Request.Plan);
		TestTrue(TEXT("Domain candidate forms deterministic distinct identities; not a durable commit"), Accepted.Receipt.IsValid()
			&& !SourceIds.Contains(Accepted.Receipt.GetSourceId()));
		SourceIds.Add(Accepted.Receipt.GetSourceId());
		TSharedPtr<FJsonObject> Object;
		FShanmenItemGeneratedSourcePlan RoundTrip;
		TestTrue(TEXT("Complete canonical candidate survives the existing lossless codec"),
			FShanmenItemGeneratedSourceCodec::Encode(Request.Plan, Object)
			&& FShanmenItemGeneratedSourceCodec::Decode(Object, RoundTrip) && RoundTrip == Request.Plan);
		++Count;
	};
	for (const auto& Slot : Fdemo_mapM01RewardDistribution::GetSlots()) { Check(Fdemo_mapM01RewardDistribution::BuildProjection(Slot)); }
	for (const auto& Slot : Fdemo_mapRewardFullMapDistribution::GetSlots()) { Check(Fdemo_mapRewardFullMapDistribution::BuildProjection(Slot)); }
	TestEqual(TEXT("All 149 M01 plus 149 P8 registered source slots"), Count, 298);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceAdapterRejectTest,
	"Shanmen.0_0_10.Items.GeneratedSourceAdapter.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceAdapterRejectTest::RunTest(const FString&)
{
	const FName Slot = Fdemo_mapM01RewardDistribution::GetSlots()[0].SlotId;
	for (int32 Variant = 0; Variant < 9; ++Variant)
	{
		auto Content = ItemContent(); auto O = Owner; auto R = Run; auto S = Slot;
		int64 Sequence = 7; int32 Pity = 2;
		switch (Variant)
		{
		case 0: Content = FShanmenContentStamp(); break;
		case 1: O.Invalidate(); break;
		case 2: R.Invalidate(); break;
		case 3: S = NAME_None; break;
		case 4: S = TEXT("Automation.Unregistered.Source"); break;
		case 5: Sequence = -1; break;
		case 6: Sequence = MAX_int64; break;
		case 7: Pity = -1; break;
		case 8: Pity = 4; break;
		}
		FShanmenItemGeneratedSourceRequest Out;
		FString Diagnostic;
		TestTrue(TEXT("Nonempty output baseline"), Fdemo_mapShanmenItemGeneratedSourceAdapter::BuildRequest(
			ItemContent(), Owner, Run, Slot, 7, 2, Out, Diagnostic));
		TestFalse(TEXT("Invalid fresh inputs are rejected"), Fdemo_mapShanmenItemGeneratedSourceAdapter::BuildRequest(
			Content, O, R, S, Sequence, Pity, Out, Diagnostic));
		TestTrue(TEXT("No stale request escapes rejection"), Out.Plan.Entries.IsEmpty() && !Out.Plan.OwnerId.IsValid()
			&& !Out.ItemContent.IsValid() && !Diagnostic.IsEmpty());
	}
	FShanmenItemDefinition Definition;
	TestTrue(TEXT("Valid definition baseline"), Fdemo_mapShanmenItemDefinitionAdapter::Build(Fdemo_mapItemIds::TrainingFlyingSword, Definition));
	TestFalse(TEXT("Unknown definition rejected"), Fdemo_mapShanmenItemDefinitionAdapter::Build(TEXT("Unknown.Definition"), Definition));
	TestTrue(TEXT("Unknown definition clears prior capability grant"), Definition.DefinitionId.IsNone() && Definition.ItemTags.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceAdapterDefinitionsTest,
	"Shanmen.0_0_10.Items.GeneratedSourceAdapter.DefinitionAndMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceAdapterDefinitionsTest::RunTest(const FString&)
{
	for (const auto& Product : Fdemo_mapItemDefinitions::GetAll())
	{
		FShanmenItemDefinition Definition;
		TestTrue(TEXT("Canonical catalog definition projection"), Fdemo_mapShanmenItemDefinitionAdapter::Build(Product.DefinitionId, Definition));
		TestTrue(TEXT("Identity and resource maxima preserved"), Definition.DefinitionId == Product.DefinitionId
			&& Definition.MaxStack == Product.MaxStackSize && Definition.MaxDurability == Product.MaxDurability && Definition.MaxCharges == Product.MaxCharges);
		TestEqual(TEXT("Flying sword capability survives source conversion"), Definition.ItemTags.HasTagExact(FShanmenItemNativeTags::ItemWeaponFlyingSword()),
			Product.HasGameplaySemantic(Edemo_mapItemGameplaySemantic::FlyingSword));
		TestEqual(TEXT("Thrown weapon capability survives source conversion"), Definition.ItemTags.HasTagExact(FShanmenItemNativeTags::ItemWeaponThrown()),
			Product.HasGameplaySemantic(Edemo_mapItemGameplaySemantic::ThrownWeapon));
	}
	const auto Projection = Fdemo_mapM01RewardDistribution::BuildProjection(Fdemo_mapM01RewardDistribution::GetSlots()[0]);
	const auto Planned = Fdemo_mapRewardSourceProjectionPlanner::Plan(Projection, Run, 2);
	if (!TestTrue(TEXT("Nonempty real canonical plan"), Planned.IsSuccess() && !Planned.PlannedStacks.IsEmpty())) { return false; }
	auto Stack = Planned.PlannedStacks[0];
	FShanmenItemRewardMetadata Metadata;
	FString Diagnostic;
	TestTrue(TEXT("Valid metadata baseline"), Fdemo_mapShanmenItemMetadataAdapter::FromPlannedStack(Stack, Metadata, Diagnostic));
	Stack.RewardEventKind = static_cast<Edemo_mapRewardEventKind>(255);
	TestFalse(TEXT("Unknown event never gets cast through"), Fdemo_mapShanmenItemMetadataAdapter::FromPlannedStack(Stack, Metadata, Diagnostic));
	TestTrue(TEXT("Malformed metadata clears previous output"), Metadata == FShanmenItemRewardMetadata() && !Diagnostic.IsEmpty());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenSourceCatalogProductTest,
	"Shanmen.0_0_10.Items.GeneratedSource.Catalog.CanonicalProductAdmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenSourceCatalogProductTest::RunTest(const FString&)
{
	FShanmenItemAuthoritySnapshot Initial;
	Initial.Content = ItemContent();
	FShanmenItemDefinition Carry;
	if (!TestTrue(TEXT("Real canonical carried definition"), Fdemo_mapShanmenItemDefinitionAdapter::Build(
		Fdemo_mapItemIds::TrainingFlyingSword, Carry))) { return false; }
	Initial.Definitions.Add(Carry);
	FShanmenItemInstance Item;
	Item.ItemInstanceId = FGuid(0x513F0500, 0, 0, 1); Item.OwnerId = Owner; Item.RunId = Run;
	Item.DefinitionId = Carry.DefinitionId; Item.Quantity = 1; Item.Durability = Carry.MaxDurability;
	Item.Charges = Carry.MaxCharges; Item.ParentContainerId = FGuid(0x513F0500, 0, 0, 2); Item.SlotIndex = 0;
	Initial.Items.Add(Item);
	FShanmenItemContainer Container;
	Container.ContainerId = Item.ParentContainerId; Container.OwnerId = Owner; Container.RunId = Run;
	Container.ContainerType = TEXT("Source.Catalog.Fixture"); Container.Slots.Add(Item.ItemInstanceId);
	Initial.Containers.Add(Container);
	FShanmenItemRepository Repository;
	if (!TestTrue(TEXT("Sparse catalog fixture is valid"), Repository.TryLoadSnapshot(Initial))) { return false; }
	FShanmenItemReserveRequest Reserve;
	Reserve.Context.Content = Initial.Content; Reserve.Context.OwnerId = Owner; Reserve.Context.RunId = Run;
	Reserve.Context.RequestId = FGuid(0x513F0500, 0, 0, 3); Reserve.ItemInstanceId = Item.ItemInstanceId;
	Reserve.ResourceKind = EShanmenItemResourceKind::DeploymentLock; Reserve.Amount = 1;
	Reserve.PurposeId = TEXT("Source.Catalog.Fixture");
	const auto Reserved = Repository.Reserve(Reserve);
	if (!TestTrue(TEXT("Real carried item is reserved"), Reserved.IsSuccess())) { return false; }
	FShanmenItemRunStartRequest Start;
	Start.Context = Reserve.Context; Start.Context.RequestId = FGuid(0x513F0500, 0, 0, 4);
	Start.ReservationIds.Add(Reserved.ReservationId);
	const auto Started = Repository.StartPreparedRun(Start);
	if (!TestTrue(TEXT("Real prepared Run, not fabricated source ownership"), Started.IsSuccess())) { return false; }
	const auto Before = Repository.CaptureSnapshot();
	int32 AcceptedCount = 0;
	auto AcceptSlot = [&](FName SlotId)
	{
		FShanmenItemGeneratedSourceRequest Request;
		FString Diagnostic;
		// Source snapshots are not sequence-sorted. Obtain the authoritative cursor via the read API.
		const auto Read = Repository.ReadGeneratedSource(Owner, Started.ReservationId, TEXT("Catalog.Unused.Query"));
		if (!TestTrue(TEXT("Canonical source builds from current authority cursor"),
			Fdemo_mapShanmenItemGeneratedSourceAdapter::BuildRequest(Initial.Content, Owner, Started.ReservationId,
				SlotId, Read.AcceptedSequence, Read.PityState, Request, Diagnostic))) { return; }
		const auto Result = Repository.AcceptGeneratedSource(Request);
		if (!TestTrue(*FString::Printf(TEXT("Canonical slot %s admits definitions absent from sparse catalog"), *SlotId.ToString()), Result.IsSuccess())) { return; }
		const auto Stored = Repository.ReadGeneratedSource(Owner, Started.ReservationId, Request.Plan.SourceRoleId);
		TestTrue(TEXT("Exact original plan is retained"), Stored.Receipt.GetPlan() == Request.Plan);
		++AcceptedCount;
	};
	AcceptSlot(Fdemo_mapM01RewardDistribution::GetSlots()[0].SlotId);
	AcceptSlot(Fdemo_mapRewardFullMapDistribution::GetSlots()[0].SlotId);
	const auto After = Repository.CaptureSnapshot();
	TestTrue(TEXT("Both canonical distributions expand catalog without inventory materialization"), AcceptedCount == 2
		&& After.Definitions.Num() > Before.Definitions.Num() && After.Items == Before.Items && After.Containers == Before.Containers);
	return true;
}
#endif
