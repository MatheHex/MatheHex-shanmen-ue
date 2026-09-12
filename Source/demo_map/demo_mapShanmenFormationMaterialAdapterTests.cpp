#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationMaterialAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid OwnerId(0xF8100001, 0, 0, 1);
	const FGuid ScopeId(0xF8100002, 0, 0, 1);
	const FGuid SourceEntityId(0xF8100003, 0, 0, 1);
	const FGuid ContainerId(0xF8100004, 0, 0, 1);
	const FGuid WoodAId(0xF8100005, 0, 0, 1);
	const FGuid CoreId(0xF8100006, 0, 0, 1);
	const FGuid WoodBId(0xF8100007, 0, 0, 1);
	const FGuid AttemptA(0xF8100008, 0, 0, 1);
	const FGuid AttemptB(0xF8100009, 0, 0, 1);
	const FName WoodDefinition(TEXT("Item.Material.FormationWood.P8.1"));
	const FName CoreDefinition(TEXT("Item.Material.SpiritCore.P8.1"));
	const FName AnchorId(TEXT("Formation.Anchor.MaterialAdapter.P8.1"));

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P8.1");
		Content.Digest = TEXT("P8.1.FormationMaterialAdapter.v1");
		return Content;
	}

	FShanmenOperationContext MakeContext(const uint32 Sequence)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = FGuid(0xF8110000 + Sequence, 0, 0, 1);
		Context.Content = MakeContent();
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

	FShanmenFormationDiagramDefinition MakeDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.MaterialAdapter.P8.1");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.ActivationEnergy.MaterialAdapter.P8.1");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 10.0f;
		FShanmenFormationAnchorCapture& Anchor =
			Capture.Anchors.AddDefaulted_GetRef();
		Anchor.Order = 0;
		Anchor.AnchorDefinitionId = AnchorId;
		Anchor.RelativeOffset = FVector(100.0, 0.0, 0.0);
		FShanmenFormationMaterialRequirementCapture& Wood =
			Anchor.Requirements.AddDefaulted_GetRef();
		Wood.Order = 0;
		Wood.MaterialDefinitionId = WoodDefinition;
		Wood.Quantity = 4;
		FShanmenFormationMaterialRequirementCapture& Core =
			Anchor.Requirements.AddDefaulted_GetRef();
		Core.Order = 1;
		Core.MaterialDefinitionId = CoreDefinition;
		Core.Quantity = 1;
		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			Capture, Diagram));
		return Diagram;
	}

	struct FFormationMaterialFixture
	{
		FShanmenItemRepository Repository;
		FShanmenItemAuthoritySnapshot Snapshot;
		Fdemo_mapShanmenRunCorrelation Correlation;
		FShanmenCombatActionSnapshot Action;
		FShanmenActionOrchestrator Runtime;
		FShanmenFormationDeployment Deployment;

		bool Build()
		{
			FShanmenItemAuthoritySnapshot Initial;
			Initial.Content = MakeContent();
			Initial.Definitions =
			{
				MakeDefinition(WoodDefinition),
				MakeDefinition(CoreDefinition)
			};
			FShanmenItemContainer Container;
			Container.ContainerId = ContainerId;
			Container.RunId = ScopeId;
			Container.OwnerId = OwnerId;
			Container.ContainerType =
				TEXT("Container.Test.P8.1.RunInventory");
			Container.Slots = { WoodAId, CoreId, WoodBId };
			Initial.Containers.Add(Container);
			Initial.Items =
			{
				MakeItem(WoodAId, WoodDefinition, 0, 2),
				MakeItem(CoreId, CoreDefinition, 1, 1),
				MakeItem(WoodBId, WoodDefinition, 2, 3)
			};
			if (!Repository.TryLoadSnapshot(Initial))
			{
				return false;
			}

			TArray<FShanmenItemTransactionReceipt> Reserved;
			const TArray<FGuid> ItemIds = { WoodAId, CoreId, WoodBId };
			const TArray<int32> Quantities = { 2, 1, 3 };
			for (int32 Index = 0; Index < ItemIds.Num(); ++Index)
			{
				FShanmenItemReserveRequest Request;
				Request.Context = MakeContext(10 + Index);
				Request.ItemInstanceId = ItemIds[Index];
				Request.ResourceKind = EShanmenItemResourceKind::Quantity;
				Request.Amount = Quantities[Index];
				Request.ExpectedItemRevision = 0;
				Request.PurposeId = TEXT("Test.P8.1.PreparedMaterial");
				Reserved.Add(Repository.Reserve(Request));
				if (!Reserved.Last().IsSuccess())
				{
					return false;
				}
			}
			FShanmenItemRunStartRequest Start;
			Start.Context = MakeContext(20);
			for (const FShanmenItemTransactionReceipt& Receipt : Reserved)
			{
				Start.ReservationIds.Add(Receipt.ReservationId);
			}
			const FShanmenItemTransactionReceipt Started =
				Repository.StartPreparedRun(Start);
			if (!Started.IsSuccess())
			{
				return false;
			}

			Correlation.CorrelationId = FGuid(0xF8120001, 0, 0, 1);
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
				FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
			ActionCapture.Content = MakeContent();
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId,
					ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId,
					81);
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
			Snapshot = Repository.CaptureSnapshot();
			return Deployment.IsValid();
		}

		Fdemo_mapShanmenFormationMaterialResult Prepare(
			const FGuid& AttemptId)
		{
			Fdemo_mapShanmenFormationMaterialResult Result =
				Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
					Snapshot, Correlation, Deployment, AnchorId, AttemptId);
			if (!Result.HasPlan())
			{
				return Result;
			}
			Result.PrepareReceipts.Reset();
			for (const Fdemo_mapShanmenFormationMaterialPlanLine& Line :
				Result.Lines)
			{
				const FShanmenItemTransactionReceipt Receipt =
					Repository.PreparePreparedRunQuantityIntent(
						Line.PrepareRequest);
				Result.PrepareReceipts.Add(Receipt);
				if (!Receipt.IsSuccess())
				{
					Result.Status =
						Edemo_mapShanmenFormationMaterialStatus::PreparationInvalid;
					return Result;
				}
			}
			Result.Status = Edemo_mapShanmenFormationMaterialStatus::Prepared;
			Snapshot = Repository.CaptureSnapshot();
			return Result;
		}

		TArray<FShanmenItemTransactionReceipt> Finalize(
			const Fdemo_mapShanmenFormationMaterialResult& Preparation,
			const bool bCommit,
			const int32 FirstIndex = 0,
			int32 Count = INDEX_NONE)
		{
			if (Count == INDEX_NONE)
			{
				Count = Preparation.Lines.Num() - FirstIndex;
			}
			TArray<FShanmenItemTransactionReceipt> Receipts;
			for (int32 Index = FirstIndex;
				Index < FirstIndex + Count;
				++Index)
			{
				FShanmenItemRunQuantityIntentFinalizeRequest Request;
				if (!Fdemo_mapShanmenFormationMaterialAdapter::
					BuildFinalizeRequest(
						Preparation, Index, bCommit, Request))
				{
					Receipts.AddDefaulted();
					continue;
				}
				Receipts.Add(
					Repository.FinalizePreparedRunQuantityIntent(Request));
			}
			Snapshot = Repository.CaptureSnapshot();
			return Receipts;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMaterialAllocationTest,
	"Shanmen.0_0_10.Product.FormationMaterialAdapter.AllocationAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMaterialAllocationTest::RunTest(const FString&)
{
	FFormationMaterialFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P8.1 allocation fixture."));
		return false;
	}
	const Fdemo_mapShanmenFormationMaterialResult First =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptA);
	const Fdemo_mapShanmenFormationMaterialResult Replay =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptA);
	const Fdemo_mapShanmenFormationMaterialResult OtherAttempt =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptB);
	TestTrue(TEXT("Two wood stacks and one core satisfy the ordered anchor"),
		First.HasPlan() && First.Lines.Num() == 3
			&& First.Lines[0].ItemInstanceId == WoodAId
			&& First.Lines[0].Quantity == 2
			&& First.Lines[1].ItemInstanceId == WoodBId
			&& First.Lines[1].Quantity == 2
			&& First.Lines[2].ItemInstanceId == CoreId
			&& First.Lines[2].Quantity == 1);
	TestTrue(TEXT("The same attempt reproduces every request identity"),
		Replay.HasPlan()
			&& Replay.TransactionId == First.TransactionId
			&& Replay.Lines[0].PrepareRequest.Context.RequestId
				== First.Lines[0].PrepareRequest.Context.RequestId
			&& Replay.Lines[1].IntentId == First.Lines[1].IntentId);
	TestTrue(TEXT("A new explicit attempt cannot collide with a cancelled retry"),
		OtherAttempt.HasPlan()
			&& OtherAttempt.TransactionId != First.TransactionId
			&& OtherAttempt.Lines[0].IntentId != First.Lines[0].IntentId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMaterialCommitTest,
	"Shanmen.0_0_10.Product.FormationMaterialAdapter.CommitEvidenceAndDeployment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMaterialCommitTest::RunTest(const FString&)
{
	FFormationMaterialFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P8.1 commit fixture."));
		return false;
	}
	Fdemo_mapShanmenFormationMaterialResult Prepared =
		Fixture.Prepare(AttemptA);
	const TArray<FShanmenItemTransactionReceipt> CommittedReceipts =
		Fixture.Finalize(Prepared, true);
	const Fdemo_mapShanmenFormationMaterialResult Committed =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildCommittedEvidence(
			Fixture.Correlation, Fixture.Deployment, Prepared,
			CommittedReceipts);
	FShanmenFormationDeploymentReceipt AnchorReceipt;
	const bool bApplied = Committed.IsCommitted()
		&& Fixture.Deployment.TryCommitAnchor(
			Fixture.Runtime, Committed.Evidence, AnchorReceipt);
	FShanmenFormationDeploymentReceipt ReplayedAnchor;
	TestTrue(TEXT("Durable commits alone create exact P8.0 fulfillment evidence"),
		Committed.IsCommitted()
			&& Committed.Evidence.Lines.Num() == 3
			&& Committed.Evidence.AuthorityRevision
				== CommittedReceipts.Last().AuthorityRevision);
	TestTrue(TEXT("The evidence is accepted once by the exact deploying anchor"),
		bApplied && AnchorReceipt.IsValid()
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Active
			&& Fixture.Deployment.TryCommitAnchor(
				Fixture.Runtime, Committed.Evidence, ReplayedAnchor)
			&& ReplayedAnchor.GetReceiptId() == AnchorReceipt.GetReceiptId());
	TestTrue(TEXT("Committed quantities follow each physical stack receipt"),
		CommittedReceipts.Num() == 3
			&& CommittedReceipts[0].ResourceAfter == 0
			&& CommittedReceipts[1].ResourceAfter == 1
			&& CommittedReceipts[2].ResourceAfter == 0
			&& Fixture.Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMaterialCancelTest,
	"Shanmen.0_0_10.Product.FormationMaterialAdapter.CancelAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMaterialCancelTest::RunTest(const FString&)
{
	FFormationMaterialFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P8.1 cancellation fixture."));
		return false;
	}
	Fdemo_mapShanmenFormationMaterialResult Prepared =
		Fixture.Prepare(AttemptA);
	const TArray<FShanmenItemTransactionReceipt> FirstCancelled =
		Fixture.Finalize(Prepared, false, 0, 1);
	const Fdemo_mapShanmenFormationMaterialResult PartialRecovery =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptA);
	TArray<FShanmenItemTransactionReceipt> Cancelled = FirstCancelled;
	Cancelled.Append(Fixture.Finalize(Prepared, false, 1));
	Prepared.Status = Edemo_mapShanmenFormationMaterialStatus::Cancelled;
	Prepared.FinalizeReceipts = Cancelled;
	const Fdemo_mapShanmenFormationMaterialResult OldAttempt =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptA);
	const Fdemo_mapShanmenFormationMaterialResult NewAttempt =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptB);
	TestTrue(TEXT("Cancellation releases every line without creating evidence"),
		Prepared.IsCancelled() && Cancelled.Num() == 3
			&& Cancelled[0].ResourceBefore == Cancelled[0].ResourceAfter
			&& Cancelled[1].ResourceBefore == Cancelled[1].ResourceAfter
			&& Cancelled[2].ResourceBefore == Cancelled[2].ResourceAfter
			&& !Prepared.Evidence.IsValid());
	TestTrue(TEXT("Partial cancellation reconstructs the complete stable plan"),
		PartialRecovery.HasPlan()
			&& PartialRecovery.Lines.Num() == Prepared.Lines.Num()
			&& PartialRecovery.Lines[0].ExistingFinalizeReceipt.Phase
				== EShanmenItemTransactionPhase::Cancelled
			&& !PartialRecovery.Lines[1].ExistingFinalizeReceipt.IsSuccess()
			&& PartialRecovery.Lines[1].IntentId
				== Prepared.Lines[1].IntentId);
	TestTrue(TEXT("The terminal old attempt is reconstructed but a new attempt may allocate"),
		OldAttempt.HasPlan()
			&& OldAttempt.Lines[0].ExistingFinalizeReceipt.Phase
				== EShanmenItemTransactionPhase::Cancelled
			&& NewAttempt.HasPlan()
			&& NewAttempt.Lines[0].ExpectedQuantityBefore == 2
			&& Fixture.Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMaterialRecoveryTest,
	"Shanmen.0_0_10.Product.FormationMaterialAdapter.RecoveryAndFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMaterialRecoveryTest::RunTest(const FString&)
{
	FFormationMaterialFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P8.1 recovery fixture."));
		return false;
	}
	Fdemo_mapShanmenFormationMaterialResult Prepared =
		Fixture.Prepare(AttemptA);
	TArray<FShanmenItemTransactionReceipt> Receipts =
		Fixture.Finalize(Prepared, true, 0, 1);
	const Fdemo_mapShanmenFormationMaterialResult RecoveredPlan =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Deployment,
			AnchorId, AttemptA);
	Receipts.Append(Fixture.Finalize(Prepared, true, 1));
	const Fdemo_mapShanmenFormationMaterialResult RecoveredCommit =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildCommittedEvidence(
			Fixture.Correlation, Fixture.Deployment, Prepared, Receipts);
	TestTrue(TEXT("Partial forward commit reconstructs original pre-consumption plan"),
		RecoveredPlan.HasPlan()
			&& RecoveredPlan.TransactionId == Prepared.TransactionId
			&& RecoveredPlan.Lines[0].ExpectedQuantityBefore == 2
			&& RecoveredPlan.Lines[0].ExistingFinalizeReceipt.Phase
				== EShanmenItemTransactionPhase::Committed
			&& RecoveredPlan.Lines[1].Quantity == 2);
	TestTrue(TEXT("Exact replay completes remaining commits and evidence"),
		RecoveredCommit.IsCommitted()
			&& Fixture.Repository.ValidateInvariants());

	FFormationMaterialFixture ConflictFixture;
	check(ConflictFixture.Build());
	FShanmenItemRunQuantityIntentRequest Foreign;
	Foreign.Context = MakeContext(90);
	Foreign.ActiveRunId = ConflictFixture.Correlation.ActiveRunId;
	Foreign.IntentId = FGuid(0xF8190001, 0, 0, 1);
	Foreign.ItemInstanceId = WoodAId;
	Foreign.Amount = 1;
	Foreign.ExpectedQuantityBefore = 2;
	Foreign.PurposeId = TEXT("Test.P8.1.ForeignPendingIntent");
	check(ConflictFixture.Repository.PreparePreparedRunQuantityIntent(
		Foreign).IsSuccess());
	ConflictFixture.Snapshot = ConflictFixture.Repository.CaptureSnapshot();
	const Fdemo_mapShanmenFormationMaterialResult Conflict =
		Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
			ConflictFixture.Snapshot, ConflictFixture.Correlation,
			ConflictFixture.Deployment, AnchorId, AttemptA);
	TestTrue(TEXT("Another pending action fences shared stack allocation"),
		Conflict.Status
			== Edemo_mapShanmenFormationMaterialStatus::ConflictingIntent
			&& !Conflict.HasPlan());

	if (!GEngine)
	{
		AddError(TEXT("GEngine is unavailable for the P8.1 facade check."));
		return false;
	}
	UGameInstance* GameInstance = NewObject<UGameInstance>(
		GEngine, NAME_None, RF_Transient);
	if (!GameInstance)
	{
		AddError(TEXT("Could not allocate the P8.1 facade GameInstance."));
		return false;
	}
	GameInstance->AddToRoot();
	GameInstance->Init();
	Udemo_mapShanmenItemAuthoritySubsystem* Unbound =
		GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	TestTrue(TEXT("Product facade fails closed before authority binding"),
		Unbound
			&& Fdemo_mapShanmenFormationMaterialAdapter::PrepareMaterials(
				*Unbound, ConflictFixture.Correlation,
				ConflictFixture.Deployment, AnchorId, AttemptB).Status
				== Edemo_mapShanmenFormationMaterialStatus::AuthorityNotReady);
	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	GameInstance->MarkAsGarbage();
	return true;
}

#endif
