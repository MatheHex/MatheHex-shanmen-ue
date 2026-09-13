#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationScatterResourcePlan.h"

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"

namespace
{
	using EPlanStatus =
		Edemo_mapShanmenFormationScatterResourcePlanStatus;

	const FGuid OwnerId(0xF8F18001, 0, 0, 1);
	const FGuid ScopeId(0xF8F18002, 0, 0, 2);
	const FGuid SourceEntityId(0xF8F18003, 0, 0, 3);
	const FGuid ContainerId(0xF8F18004, 0, 0, 4);
	const FGuid WoodAId(0xF8F18005, 0, 0, 5);
	const FGuid StoneId(0xF8F18006, 0, 0, 6);
	const FGuid WoodBId(0xF8F18007, 0, 0, 7);
	const FGuid OperationId(0xF8F18008, 0, 0, 8);
	const FName WoodMaterial(TEXT("Item.Material.P27_18.Wood"));
	const FName StoneMaterial(TEXT("Item.Material.P27_18.Stone"));
	const FName EastAnchor(TEXT("Formation.Anchor.P27_18.East"));
	const FName NorthAnchor(TEXT("Formation.Anchor.P27_18.North"));

	FShanmenContentStamp MakeContent(const TCHAR* Digest = TEXT("resource-plan"))
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.18.Test");
		Content.Digest = Digest;
		return Content;
	}

	FShanmenOperationContext MakeContext(
		const FShanmenContentStamp& Content,
		const uint32 Sequence)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = FGuid(0xF8F18100 + Sequence, 0, 0, 1);
		Context.Content = Content;
		return Context;
	}

	FShanmenItemDefinition MakeDefinition(const FName DefinitionId)
	{
		FShanmenItemDefinition Definition;
		Definition.DefinitionId = DefinitionId;
		Definition.MaxStack = 16;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		return Definition;
	}

	FShanmenItemInstance MakeItem(
		const FGuid& ItemId,
		const FName DefinitionId,
		const int32 Slot,
		const int32 Quantity)
	{
		FShanmenItemInstance Item;
		Item.ItemInstanceId = ItemId;
		Item.DefinitionId = DefinitionId;
		Item.RunId = ScopeId;
		Item.OwnerId = OwnerId;
		Item.ParentContainerId = ContainerId;
		Item.SlotIndex = Slot;
		Item.Quantity = Quantity;
		return Item;
	}

	FShanmenFormationMaterialRequirementCapture MakeRequirement(
		const int32 Order,
		const FName DefinitionId,
		const int32 Quantity)
	{
		FShanmenFormationMaterialRequirementCapture Requirement;
		Requirement.Order = Order;
		Requirement.MaterialDefinitionId = DefinitionId;
		Requirement.Quantity = Quantity;
		return Requirement;
	}

	FShanmenFormationAnchorCapture MakeAnchor(
		const int32 Order,
		const FName AnchorDefinitionId,
		const FVector& Offset,
		const TArray<FShanmenFormationMaterialRequirementCapture>&
			Requirements)
	{
		FShanmenFormationAnchorCapture Anchor;
		Anchor.Order = Order;
		Anchor.AnchorDefinitionId = AnchorDefinitionId;
		Anchor.RelativeOffset = Offset;
		Anchor.Requirements = Requirements;
		return Anchor;
	}

	FShanmenFormationDiagramDefinition MakeDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.P27_18.ResourcePlan");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.Energy.P27_18.ResourcePlan");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 1.0f;
		Capture.Anchors =
		{
			MakeAnchor(
				1, NorthAnchor, FVector(0.0, 100.0, 0.0),
				{ MakeRequirement(0, WoodMaterial, 3) }),
			MakeAnchor(
				0, EastAnchor, FVector(100.0, 0.0, 0.0),
				{
					MakeRequirement(1, StoneMaterial, 1),
					MakeRequirement(0, WoodMaterial, 2)
				})
		};
		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			Capture, Diagram));
		return Diagram;
	}

	struct FScatterResourceFixture
	{
		FShanmenContentStamp Content = MakeContent();
		FShanmenItemRepository Repository;
		FShanmenItemAuthoritySnapshot Snapshot;
		Fdemo_mapShanmenRunCorrelation Correlation;
		FShanmenActionOrchestrator Runtime;
		FShanmenFormationDeployment Deployment;
		Fdemo_mapShanmenFormationMasteryProjectionResult Projection;
		Fdemo_mapShanmenFormationScatterBatchIntent Batch;

		bool Build(
			const int32 WoodAQuantity = 4,
			const int32 WoodBQuantity = 2)
		{
			FShanmenItemAuthoritySnapshot Initial;
			Initial.Content = Content;
			Initial.Definitions =
			{
				MakeDefinition(WoodMaterial),
				MakeDefinition(StoneMaterial)
			};
			FShanmenItemContainer Container;
			Container.ContainerId = ContainerId;
			Container.RunId = ScopeId;
			Container.OwnerId = OwnerId;
			Container.ContainerType =
				TEXT("Container.Test.P27_18.RunInventory");
			Container.Slots = { WoodAId, StoneId, WoodBId };
			Initial.Containers.Add(Container);
			Initial.Items =
			{
				MakeItem(WoodAId, WoodMaterial, 0, WoodAQuantity),
				MakeItem(StoneId, StoneMaterial, 1, 1),
				MakeItem(WoodBId, WoodMaterial, 2, WoodBQuantity)
			};
			if (!Repository.TryLoadSnapshot(Initial))
			{
				return false;
			}

			const TArray<FGuid> ItemIds =
				{ WoodAId, StoneId, WoodBId };
			const TArray<int32> Quantities =
				{ WoodAQuantity, 1, WoodBQuantity };
			TArray<FShanmenItemTransactionReceipt> Reserved;
			for (int32 Index = 0; Index < ItemIds.Num(); ++Index)
			{
				FShanmenItemReserveRequest Request;
				Request.Context = MakeContext(Content, 10 + Index);
				Request.ItemInstanceId = ItemIds[Index];
				Request.ResourceKind =
					EShanmenItemResourceKind::Quantity;
				Request.Amount = Quantities[Index];
				Request.ExpectedItemRevision = 0;
				Request.PurposeId = TEXT("Test.P27_18.PreparedMaterial");
				Reserved.Add(Repository.Reserve(Request));
				if (!Reserved.Last().IsSuccess())
				{
					return false;
				}
			}
			FShanmenItemRunStartRequest Start;
			Start.Context = MakeContext(Content, 20);
			for (const auto& Receipt : Reserved)
			{
				Start.ReservationIds.Add(Receipt.ReservationId);
			}
			const FShanmenItemTransactionReceipt Started =
				Repository.StartPreparedRun(Start);
			if (!Started.IsSuccess())
			{
				return false;
			}

			Correlation.CorrelationId = FGuid(0xF8F18201, 0, 0, 1);
			Correlation.OwnerId = OwnerId;
			Correlation.ScopeId = ScopeId;
			Correlation.ActiveRunId = Started.ReservationId;
			Correlation.PreparedRequestId = Start.Context.RequestId;
			Correlation.PreparedReceiptId = Reserved.Last().ReceiptId;
			Correlation.LifecycleRequestId = Started.RequestId;
			Correlation.LifecycleReceiptId = Started.ReceiptId;
			Correlation.PreparedAuthorityRevision =
				Reserved.Last().AuthorityRevision;
			Correlation.LifecycleAuthorityRevision =
				Started.AuthorityRevision;
			Correlation.OrderedPreparedItemInstanceIds = ItemIds;
			Correlation.OrderedRunInventoryItemInstanceIds = ItemIds;
			Correlation.HotbarItemInstanceIds.SetNum(
				Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
			if (!Correlation.IsValid())
			{
				return false;
			}

			FShanmenCombatActionCapture ActionCapture;
			ActionCapture.RunId = Correlation.ActiveRunId;
			ActionCapture.OwnerId = OwnerId;
			ActionCapture.SourceEntityId = SourceEntityId;
			ActionCapture.ActionDefinitionId =
				FShanmenFormationDiagramDefinition::
					CanonicalActionDefinitionId();
			ActionCapture.Content = Content;
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId,
					ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId,
					181);
			FShanmenCombatActionSnapshot Action;
			if (!FShanmenCombatActionSnapshot::TryCapture(
					ActionCapture, Action))
			{
				return false;
			}
			FShanmenActionTransitionReceipt Transition;
			if (!FShanmenActionOrchestrator::TryStart(
					Action, Runtime, Transition)
				|| !Runtime.TryAdvance(
					EShanmenCombatActionPhase::Startup, Transition)
				|| !FShanmenFormationDeployment::TryCreate(
					Action, MakeDiagram(), FVector::ZeroVector,
					FVector::ForwardVector, Deployment))
			{
				return false;
			}
			FShanmenFormationDeploymentReceipt Begin;
			if (!Deployment.TryBeginDeployment(Runtime, Begin))
			{
				return false;
			}

			auto ReadAuthority = [this](
				const FGuid&,
				Fdemo_mapShanmenFormationMasteryAuthorityCapture& Out,
				FString& OutDiagnostic)
			{
				Out.OwnerId = OwnerId;
				Out.AuthorityRevision = 18;
				Out.Content = Content;
				Out.MasteryTier =
					EShanmenFormationMasteryTier::Master;
				OutDiagnostic = TEXT("P27.18 fixture authority read.");
				return true;
			};
			Projection =
				Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
					Content, OwnerId, ReadAuthority);
			const auto Authorization =
				Fdemo_mapShanmenFormationMasteryOperationAuthorizer::
					Authorize(
						Projection,
						Deployment,
						OperationId,
						EShanmenFormationMaterialDeliveryMode::
							ScatterFormation,
						NAME_None);
			if (!Authorization.IsAuthorized())
			{
				return false;
			}
			const auto BatchResult =
				Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
					Projection, Deployment, Authorization.Authorization);
			if (!BatchResult.IsPlanned())
			{
				return false;
			}
			Batch = BatchResult.Batch;
			Snapshot = Repository.CaptureSnapshot();
			return true;
		}

		Fdemo_mapShanmenFormationScatterResourcePlanResult Plan() const
		{
			return Fdemo_mapShanmenFormationScatterResourcePlanner::Plan(
				Projection, Deployment, Snapshot, Correlation, Batch);
		}

		bool AddPendingWoodAIntent()
		{
			FShanmenItemRunQuantityIntentRequest Request;
			Request.Context = MakeContext(Content, 40);
			Request.ActiveRunId = Correlation.ActiveRunId;
			Request.IntentId = FGuid(0xF8F18301, 0, 0, 1);
			Request.ItemInstanceId = WoodAId;
			Request.Amount = 1;
			Request.ExpectedQuantityBefore = 4;
			Request.PurposeId = TEXT("Test.P27_18.PendingConflict");
			const auto Receipt =
				Repository.PreparePreparedRunQuantityIntent(Request);
			Snapshot = Repository.CaptureSnapshot();
			return Receipt.IsSuccess();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceDeterminismTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePlan.DeterministicCumulativeAllocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterResourceDeterminismTest::RunTest(
	const FString&)
{
	FScatterResourceFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P27.18 deterministic fixture."));
		return false;
	}
	const auto First = Fixture.Plan();
	const auto Replay = Fixture.Plan();
	TestTrue(TEXT("The same whole-batch evidence reproduces every identity"),
		First.IsPlanned()
			&& Replay.IsPlanned()
			&& First.Plan == Replay.Plan
			&& First.Plan.GetPlanId() == Replay.Plan.GetPlanId());
	TestTrue(TEXT("All three authored requirements receive exact quantities"),
		First.Plan.GetTotalRequirementCount() == 3
			&& First.Plan.GetTotalAllocatedQuantity() == 6
			&& First.Plan.GetSlices().Num() == 4
			&& First.Plan.GetReservations().Num() == 3);

	const auto& Slices = First.Plan.GetSlices();
	TestTrue(TEXT("Canonical allocation carries one stack across anchors"),
		Slices[0].GetAnchorOrder() == 0
			&& Slices[0].GetRequirementOrder() == 0
			&& Slices[0].GetItemInstanceId() == WoodAId
			&& Slices[0].GetQuantity() == 2
			&& Slices[0].GetSourceQuantityBefore() == 4
			&& Slices[0].GetSourceQuantityAfter() == 2
			&& Slices[1].GetItemInstanceId() == StoneId
			&& Slices[1].GetQuantity() == 1
			&& Slices[2].GetAnchorOrder() == 1
			&& Slices[2].GetItemInstanceId() == WoodAId
			&& Slices[2].GetQuantity() == 2
			&& Slices[2].GetSourceQuantityBefore() == 2
			&& Slices[2].GetSourceQuantityAfter() == 0
			&& Slices[3].GetItemInstanceId() == WoodBId
			&& Slices[3].GetQuantity() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceAggregationTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePlan.StackAggregationAndAttribution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterResourceAggregationTest::RunTest(
	const FString&)
{
	FScatterResourceFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P27.18 aggregation fixture."));
		return false;
	}
	const auto Planned = Fixture.Plan();
	if (!Planned.IsPlanned())
	{
		AddError(TEXT("P27.18 aggregation fixture did not plan."));
		return false;
	}
	const auto& Reservations = Planned.Plan.GetReservations();
	const auto& WoodA = Reservations[0];
	TestTrue(TEXT("One physical stack produces exactly one prepare command"),
		WoodA.GetItemInstanceId() == WoodAId
			&& WoodA.GetMaterialDefinitionId() == WoodMaterial
			&& WoodA.GetQuantity() == 4
			&& WoodA.GetExpectedQuantityBefore() == 4
			&& WoodA.GetQuantityAfter() == 0
			&& WoodA.GetSliceIds().Num() == 2
			&& WoodA.GetSliceIds()[0]
				== Planned.Plan.GetSlices()[0].GetSliceId()
			&& WoodA.GetSliceIds()[1]
				== Planned.Plan.GetSlices()[2].GetSliceId()
			&& WoodA.GetPrepareRequest().Amount == 4);

	TArray<FShanmenItemTransactionReceipt> Receipts;
	for (const auto& Reservation : Reservations)
	{
		Receipts.Add(
			Fixture.Repository.PreparePreparedRunQuantityIntent(
				Reservation.GetPrepareRequest()));
	}
	TestTrue(TEXT("All aggregated commands are accepted by ShanmenItems"),
		Receipts.Num() == 3
			&& Receipts[0].IsSuccess()
			&& Receipts[1].IsSuccess()
			&& Receipts[2].IsSuccess()
			&& Receipts[0].Amount == 4
			&& Receipts[1].Amount == 1
			&& Receipts[2].Amount == 1
			&& Fixture.Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceFencesTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePlan.QuantityAndPendingFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterResourceFencesTest::RunTest(const FString&)
{
	FScatterResourceFixture Insufficient;
	FScatterResourceFixture Conflict;
	if (!Insufficient.Build(2, 2)
		|| !Conflict.Build()
		|| !Conflict.AddPendingWoodAIntent())
	{
		AddError(TEXT("Could not build the P27.18 fence fixtures."));
		return false;
	}
	const auto Missing = Insufficient.Plan();
	const auto Pending = Conflict.Plan();
	TestTrue(TEXT("A whole-batch shortage returns no partial allocation"),
		Missing.IsValid()
			&& Missing.Status == EPlanStatus::QuantityUnavailable
			&& !Missing.Plan.IsValid()
			&& Missing.Plan.GetSlices().IsEmpty());
	TestTrue(TEXT("A pending physical-stack mutation fails closed distinctly"),
		Pending.IsValid()
			&& Pending.Status == EPlanStatus::ConflictingIntent
			&& !Pending.Plan.IsValid());

	FShanmenItemAuthoritySnapshot WrongContent = Insufficient.Snapshot;
	WrongContent.Content = MakeContent(TEXT("different-content"));
	const auto Mismatch =
		Fdemo_mapShanmenFormationScatterResourcePlanner::Plan(
			Insufficient.Projection,
			Insufficient.Deployment,
			WrongContent,
			Insufficient.Correlation,
			Insufficient.Batch);
	TestTrue(TEXT("Cross-content resource evidence is rejected before allocation"),
		Mismatch.IsValid()
			&& Mismatch.Status == EPlanStatus::ContentMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceCurrentnessTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePlan.CurrentnessAndNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterResourceCurrentnessTest::RunTest(
	const FString&)
{
	FScatterResourceFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P27.18 currentness fixture."));
		return false;
	}
	const FShanmenItemAuthoritySnapshot Before =
		Fixture.Repository.CaptureSnapshot();
	const auto Planned = Fixture.Plan();
	const FShanmenItemAuthoritySnapshot AfterPlan =
		Fixture.Repository.CaptureSnapshot();
	TestTrue(TEXT("Planning is pure and current for its exact snapshot"),
		Planned.IsPlanned()
			&& Before == AfterPlan
			&& Fdemo_mapShanmenFormationScatterResourcePlanner::
				IsCurrentPlan(
					Fixture.Projection,
					Fixture.Deployment,
					AfterPlan,
					Fixture.Correlation,
					Planned.Plan));

	const auto Prepared =
		Fixture.Repository.PreparePreparedRunQuantityIntent(
			Planned.Plan.GetReservations()[0].GetPrepareRequest());
	const FShanmenItemAuthoritySnapshot Mutated =
		Fixture.Repository.CaptureSnapshot();
	TestTrue(TEXT("The authority accepted the first explicit downstream command"),
		Prepared.IsSuccess() && Mutated.AuthorityRevision
			== Before.AuthorityRevision + 1);
	TestFalse(TEXT("A later authority snapshot invalidates historical currentness"),
		Fdemo_mapShanmenFormationScatterResourcePlanner::IsCurrentPlan(
			Fixture.Projection,
			Fixture.Deployment,
			Mutated,
			Fixture.Correlation,
			Planned.Plan));
	TestTrue(TEXT("The old plan remains immutable and internally auditable"),
		Planned.Plan.IsValid());
	return true;
}

#endif
