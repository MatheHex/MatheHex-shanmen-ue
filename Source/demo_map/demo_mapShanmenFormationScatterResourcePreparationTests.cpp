#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationScatterDeploymentCommit.h"
#include "demo_mapShanmenFormationScatterResourceCommit.h"
#include "demo_mapShanmenFormationScatterResourcePreparation.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"

namespace
{
	using EPreparationStatus =
		Edemo_mapShanmenFormationScatterResourcePreparationStatus;
	using ECommitStatus =
		Edemo_mapShanmenFormationScatterResourceCommitStatus;
	using EDeploymentCommitStatus =
		Edemo_mapShanmenFormationScatterDeploymentCommitStatus;

	constexpr EAutomationTestFlags PreparationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid OwnerId(0xF8F19001, 0, 0, 1);
	const FGuid ScopeId(0xF8F19002, 0, 0, 2);
	const FGuid SourceEntityId(0xF8F19003, 0, 0, 3);
	const FGuid ContainerId(0xF8F19004, 0, 0, 4);
	const FGuid WoodAId(0xF8F19005, 0, 0, 5);
	const FGuid StoneId(0xF8F19006, 0, 0, 6);
	const FGuid WoodBId(0xF8F19007, 0, 0, 7);
	const FGuid OperationId(0xF8F19008, 0, 0, 8);
	const FGuid MigrationId(0xF8F19009, 0, 0, 9);
	const FName WoodMaterial(TEXT("Item.Material.P27_19.Wood"));
	const FName StoneMaterial(TEXT("Item.Material.P27_19.Stone"));
	const FName EastAnchor(TEXT("Formation.Anchor.P27_19.East"));
	const FName NorthAnchor(TEXT("Formation.Anchor.P27_19.North"));

	FString NewPreparationRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P27.19.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.19.Test");
		Content.Digest = TEXT("scatter-resource-preparation");
		return Content;
	}

	FShanmenOperationContext MakeContext(
		const FShanmenContentStamp& Content,
		const uint32 Sequence)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = FGuid(0xF8F19100 + Sequence, 0, 0, 1);
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
			TEXT("Formation.Diagram.P27_19.ResourcePreparation");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.Energy.P27_19.ResourcePreparation");
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

	FShanmenItemMigrationEvidence MakeMigrationEvidence(
		const FShanmenItemAuthoritySnapshot& Snapshot)
	{
		FShanmenItemMigrationEvidence Evidence;
		Evidence.MigrationId = MigrationId;
		Evidence.OwnerId = OwnerId;
		Evidence.SourceProfileSchema = 7;
		Evidence.SourceSaveGeneration = 19;
		Evidence.SourceCodeBPersistentRevision = 19;
		Evidence.SourceCodeBRepositoryRevision = 19;
		Evidence.DefinitionCount = Snapshot.Definitions.Num();
		Evidence.ContainerCount = Snapshot.Containers.Num();
		Evidence.ItemCount = Snapshot.Items.Num();
		Evidence.SourceFingerprint =
			TEXT("P27.19.ScatterPreparation.SourceFixture.v1");
		Evidence.CandidateDigest =
			TEXT("P27.19.ScatterPreparation.CandidateFixture.v1");
		return Evidence;
	}

	class FServiceAuthority final
		: public Idemo_mapShanmenFormationScatterResourceAuthority
	{
	public:
		explicit FServiceAuthority(FShanmenItemAuthorityService& InService)
			: Service(InService)
		{
		}

		int32 InjectPrepareFailureAt = INDEX_NONE;
		int32 InjectFinalizeFailureAt = INDEX_NONE;
		int32 PrepareCalls = 0;
		int32 FinalizeCalls = 0;
		bool bPersistRejectFirstCommit = false;
		bool bDidPersistRejectFirstCommit = false;
		FGuid FirstRejectedCommitRequestId;

		virtual bool IsReady() const override
		{
			return Service.GetState()
				== EShanmenItemAuthorityServiceState::Ready;
		}

		virtual bool TryCaptureSnapshot(
			FShanmenItemAuthoritySnapshot& OutSnapshot) const override
		{
			return Service.TryCaptureSnapshot(OutSnapshot);
		}

		virtual FShanmenItemDurableCommandResult PrepareQuantity(
			const FShanmenItemRunQuantityIntentRequest& Request) override
		{
			++PrepareCalls;
			if (PrepareCalls == InjectPrepareFailureAt)
			{
				Service.SetInjectedFailureForTests(
					EShanmenItemStoreFailureStage::WriteTemp);
			}
			FShanmenItemDurableCommandResult Result =
				Service.PreparePreparedRunQuantityIntentDurable(Request);
			Service.SetInjectedFailureForTests(
				EShanmenItemStoreFailureStage::None);
			return Result;
		}

		virtual FShanmenItemDurableCommandResult FinalizeQuantity(
			const FShanmenItemRunQuantityIntentFinalizeRequest& Request)
			override
		{
			++FinalizeCalls;
			if (FinalizeCalls == InjectFinalizeFailureAt)
			{
				Service.SetInjectedFailureForTests(
					EShanmenItemStoreFailureStage::WriteTemp);
			}
			FShanmenItemRunQuantityIntentFinalizeRequest Dispatched =
				Request;
			if (bPersistRejectFirstCommit && Request.bCommit
				&& !bDidPersistRejectFirstCommit)
			{
				bDidPersistRejectFirstCommit = true;
				FirstRejectedCommitRequestId = Request.Context.RequestId;
				Dispatched.Context.Content.Digest += TEXT(".mismatch");
			}
			FShanmenItemDurableCommandResult Result =
				Service.FinalizePreparedRunQuantityIntentDurable(Dispatched);
			Service.SetInjectedFailureForTests(
				EShanmenItemStoreFailureStage::None);
			return Result;
		}

	private:
		FShanmenItemAuthorityService& Service;
	};

	const FShanmenItemInstance* FindItem(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemId)
	{
		return Snapshot.Items.FindByPredicate(
			[&ItemId](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == ItemId;
			});
	}

	int32 CountPendingPlanPrepares(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		int32 Pending = 0;
		for (const auto& Reservation : Plan.GetReservations())
		{
			const FGuid PrepareId =
				Reservation.GetPrepareRequest().Context.RequestId;
			const bool bPrepared = Snapshot.ProcessedRequests.ContainsByPredicate(
				[&PrepareId](
					const FShanmenItemProcessedRequestSnapshot& Processed)
				{
					return Processed.RequestId == PrepareId
						&& Processed.Receipt.IsSuccess()
						&& Processed.Receipt.Operation
							== EShanmenItemTransactionOperation::
								PreparePreparedRunQuantityIntent;
				});
			const bool bFinalized =
				Snapshot.ProcessedRequests.ContainsByPredicate(
					[&PrepareId](
						const FShanmenItemProcessedRequestSnapshot& Processed)
					{
						return Processed.Receipt.IsSuccess()
							&& Processed.Receipt.Operation
								== EShanmenItemTransactionOperation::
									FinalizePreparedRunQuantityIntent
							&& Processed.Receipt.ReservationIds.Num() == 2
							&& Processed.Receipt.ReservationIds[1]
								== PrepareId;
					});
			Pending += bPrepared && !bFinalized ? 1 : 0;
		}
		return Pending;
	}

	int32 CountPlanCommits(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		int32 Committed = 0;
		for (const auto& Reservation : Plan.GetReservations())
		{
			const FGuid PrepareId =
				Reservation.GetPrepareRequest().Context.RequestId;
			Committed += Snapshot.ProcessedRequests.ContainsByPredicate(
				[&PrepareId](
					const FShanmenItemProcessedRequestSnapshot& Processed)
				{
					return Processed.Receipt.IsSuccess()
						&& Processed.Receipt.Operation
							== EShanmenItemTransactionOperation::
								FinalizePreparedRunQuantityIntent
						&& Processed.Receipt.Phase
							== EShanmenItemTransactionPhase::Committed
						&& Processed.Receipt.ReservationIds.Num() == 2
						&& Processed.Receipt.ReservationIds[1]
							== PrepareId;
				}) ? 1 : 0;
		}
		return Committed;
	}

	bool SamePhysicalQuantities(
		const FShanmenItemAuthoritySnapshot& Left,
		const FShanmenItemAuthoritySnapshot& Right)
	{
		const TArray<FGuid> ItemIds = { WoodAId, StoneId, WoodBId };
		for (const FGuid& ItemId : ItemIds)
		{
			const FShanmenItemInstance* LeftItem = FindItem(Left, ItemId);
			const FShanmenItemInstance* RightItem = FindItem(Right, ItemId);
			if (!LeftItem || !RightItem
				|| LeftItem->Quantity != RightItem->Quantity
				|| LeftItem->Revision != RightItem->Revision)
			{
				return false;
			}
		}
		return true;
	}

	const FShanmenItemTransactionReceipt* FindCommitReceiptForItem(
		const Fdemo_mapShanmenFormationScatterResourceCommitResult& Result,
		const FGuid& ItemId)
	{
		return Result.CommitReceipts.FindByPredicate(
			[&ItemId](const FShanmenItemTransactionReceipt& Receipt)
			{
				return Receipt.IsSuccess()
					&& Receipt.ItemInstanceId == ItemId
					&& Receipt.Phase
						== EShanmenItemTransactionPhase::Committed;
			});
	}

	struct FPreparationFixture
	{
		FString Root;
		FShanmenContentStamp Content = MakeContent();
		FShanmenItemAuthorityService Service;
		FServiceAuthority Authority;
		Fdemo_mapShanmenRunCorrelation Correlation;
		FShanmenActionOrchestrator Runtime;
		FShanmenFormationDeployment Deployment;
		Fdemo_mapShanmenFormationMasteryProjectionResult Projection;
		Fdemo_mapShanmenFormationScatterResourcePlan Plan;

		FPreparationFixture()
			: Authority(Service)
		{
		}

		~FPreparationFixture()
		{
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
			}
		}

		bool Build(const TCHAR* Label);

		Fdemo_mapShanmenFormationScatterResourcePreparationResult Execute()
		{
			return Fdemo_mapShanmenFormationScatterResourcePreparation::
				PrepareWithAuthority(
					Authority, Projection, Deployment, Correlation, Plan);
		}

		Fdemo_mapShanmenFormationScatterResourceCommitResult ExecuteCommit()
		{
			return Fdemo_mapShanmenFormationScatterResourceCommitter::
				CommitWithAuthority(
					Authority, Projection, Deployment, Correlation, Plan);
		}

		bool Capture(FShanmenItemAuthoritySnapshot& Out) const
		{
			return Service.TryCaptureSnapshot(Out);
		}
	};

	bool FPreparationFixture::Build(const TCHAR* Label)
	{
		FShanmenItemRepository Repository;
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
			TEXT("Container.Test.P27_19.RunInventory");
		Container.Slots = { WoodAId, StoneId, WoodBId };
		Initial.Containers.Add(Container);
		Initial.Items =
		{
			MakeItem(WoodAId, WoodMaterial, 0, 4),
			MakeItem(StoneId, StoneMaterial, 1, 1),
			MakeItem(WoodBId, WoodMaterial, 2, 2)
		};
		if (!Repository.TryLoadSnapshot(Initial))
		{
			return false;
		}

		const TArray<FGuid> ItemIds =
			{ WoodAId, StoneId, WoodBId };
		const TArray<int32> Quantities = { 4, 1, 2 };
		TArray<FShanmenItemTransactionReceipt> Reserved;
		for (int32 Index = 0; Index < ItemIds.Num(); ++Index)
		{
			FShanmenItemReserveRequest Request;
			Request.Context = MakeContext(Content, 10 + Index);
			Request.ItemInstanceId = ItemIds[Index];
			Request.ResourceKind = EShanmenItemResourceKind::Quantity;
			Request.Amount = Quantities[Index];
			Request.ExpectedItemRevision = 0;
			Request.PurposeId = TEXT("Test.P27_19.PreparedMaterial");
			Reserved.Add(Repository.Reserve(Request));
			if (!Reserved.Last().IsSuccess())
			{
				return false;
			}
		}
		FShanmenItemRunStartRequest Start;
		Start.Context = MakeContext(Content, 20);
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

		Correlation.CorrelationId = FGuid(0xF8F19201, 0, 0, 1);
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
				191);
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
			Out.AuthorityRevision = 19;
			Out.Content = Content;
			Out.MasteryTier = EShanmenFormationMasteryTier::Master;
			OutDiagnostic = TEXT("P27.19 fixture authority read.");
			return true;
		};
		Projection =
			Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
				Content, OwnerId, ReadAuthority);
		const auto Authorization =
			Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
				Projection, Deployment, OperationId,
				EShanmenFormationMaterialDeliveryMode::ScatterFormation,
				NAME_None);
		if (!Authorization.IsAuthorized())
		{
			return false;
		}
		const auto Batch =
			Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
				Projection, Deployment, Authorization.Authorization);
		if (!Batch.IsPlanned())
		{
			return false;
		}

		const FShanmenItemAuthoritySnapshot Snapshot =
			Repository.CaptureSnapshot();
		const auto Planned =
			Fdemo_mapShanmenFormationScatterResourcePlanner::Plan(
				Projection, Deployment, Snapshot, Correlation, Batch.Batch);
		if (!Planned.IsPlanned())
		{
			return false;
		}
		Plan = Planned.Plan;
		Root = NewPreparationRoot(Label);
		const FShanmenItemStorageContext Storage =
			FShanmenItemStorageContext::ForRoot(Root, OwnerId);
		return Service.StartFromAuthorizedMigration(
			Storage,
			FShanmenItemMigrationAuthorization::Explicit(MigrationId),
			Snapshot, MakeMigrationEvidence(Snapshot)).IsReady();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourcePreparationDurableReplayTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePreparation.DurableWholeBatchAndReplay",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourcePreparationDurableReplayTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("DurableReplay")))
	{
		AddError(TEXT("Could not build the P27.19 durable replay fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const auto First = Fixture.Execute();
	FShanmenItemAuthoritySnapshot Prepared;
	Fixture.Capture(Prepared);
	TestTrue(TEXT("All physical stacks become durably prepared"),
		First.Status == EPreparationStatus::Prepared
			&& First.IsValid() && First.IsPrepared()
			&& First.PrepareCommands.Num() == 3
			&& First.RollbackCommands.IsEmpty()
			&& CountPendingPlanPrepares(Prepared, Fixture.Plan) == 3);
	TestTrue(TEXT("Preparation reserves availability without consuming quantity"),
		SamePhysicalQuantities(Before, Prepared));

	const auto Replay = Fixture.Execute();
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Capture(AfterReplay);
	bool bEveryCommandReplayed = Replay.PrepareCommands.Num() == 3;
	for (const auto& Command : Replay.PrepareCommands)
	{
		bEveryCommandReplayed &= Command.Status
			== EShanmenItemDurableCommandStatus::Replayed;
	}
	TestTrue(TEXT("The exact plan replays without duplicate intents"),
		Replay.Status == EPreparationStatus::Replayed
			&& Replay.IsValid() && Replay.IsPrepared()
			&& bEveryCommandReplayed
			&& AfterReplay == Prepared);

	FShanmenItemRunQuantityIntentFinalizeRequest Commit;
	FShanmenItemRunQuantityIntentFinalizeRequest Cancel;
	TestTrue(TEXT("Commit and cancel share one deterministic terminal identity"),
		Fdemo_mapShanmenFormationScatterResourcePreparation::
			BuildFinalizeRequest(Fixture.Plan, 0, true, Commit)
			&& Fdemo_mapShanmenFormationScatterResourcePreparation::
				BuildFinalizeRequest(Fixture.Plan, 0, false, Cancel)
			&& Commit.Context.RequestId == Cancel.Context.RequestId
			&& Commit.bCommit && !Cancel.bCommit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourcePreparationRollbackTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePreparation.MiddleFailureRollsBackEarlierLines",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourcePreparationRollbackTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("Rollback")))
	{
		AddError(TEXT("Could not build the P27.19 rollback fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	Fixture.Authority.InjectPrepareFailureAt = 2;
	const auto Result = Fixture.Execute();
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	TestTrue(TEXT("A second-line durability failure cancels the first line"),
		Result.Status == EPreparationStatus::PrepareRejectedRolledBack
			&& Result.IsValid() && Result.IsCancelled()
			&& Result.PrepareCommands.Num() == 2
			&& Result.RollbackCommands.Num() == 1
			&& Result.PrepareCommands[1].Status
				== EShanmenItemDurableCommandStatus::
					PersistenceFailedRolledBack
			&& Result.RollbackCommands[0].IsCommandSuccess());
	TestTrue(TEXT("The bounded rollback leaves no quantity or pending change"),
		SamePhysicalQuantities(Before, After)
			&& CountPendingPlanPrepares(After, Fixture.Plan) == 0
			&& Fixture.Service.GetState()
				== EShanmenItemAuthorityServiceState::Ready);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourcePreparationRecoveryTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePreparation.RollbackRecoveryAndExactResume",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourcePreparationRecoveryTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("Recovery")))
	{
		AddError(TEXT("Could not build the P27.19 recovery fixture."));
		return false;
	}
	Fixture.Authority.InjectPrepareFailureAt = 2;
	Fixture.Authority.InjectFinalizeFailureAt = 1;
	const auto Failed = Fixture.Execute();
	FShanmenItemAuthoritySnapshot Partial;
	Fixture.Capture(Partial);
	TestTrue(TEXT("An unproven rollback reports recovery with exact pending state"),
		Failed.Status == EPreparationStatus::RollbackRecoveryRequired
			&& Failed.IsValid() && Failed.RequiresRecovery()
			&& CountPendingPlanPrepares(Partial, Fixture.Plan) == 1
			&& Fixture.Service.GetState()
				== EShanmenItemAuthorityServiceState::Ready);

	Fixture.Authority.InjectPrepareFailureAt = INDEX_NONE;
	Fixture.Authority.InjectFinalizeFailureAt = INDEX_NONE;
	const auto Recovered = Fixture.Execute();
	FShanmenItemAuthoritySnapshot Complete;
	Fixture.Capture(Complete);
	TestTrue(TEXT("The next bounded pass reconstructs and exactly resumes"),
		Recovered.Status == EPreparationStatus::Prepared
			&& Recovered.IsValid() && Recovered.IsPrepared()
			&& Recovered.PrepareCommands.Num() == 3
			&& Recovered.PrepareCommands[0].Status
				== EShanmenItemDurableCommandStatus::Replayed
			&& CountPendingPlanPrepares(Complete, Fixture.Plan) == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourcePreparationStaleTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePreparation.StalePlanFailsBeforeMutation",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourcePreparationStaleTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("Stale")))
	{
		AddError(TEXT("Could not build the P27.19 stale-plan fixture."));
		return false;
	}
	FShanmenItemRunQuantityIntentRequest Foreign;
	Foreign.Context = MakeContext(Fixture.Content, 90);
	Foreign.ActiveRunId = Fixture.Correlation.ActiveRunId;
	Foreign.IntentId = FGuid(0xF8F19901, 0, 0, 1);
	Foreign.ItemInstanceId = WoodAId;
	Foreign.Amount = 1;
	Foreign.ExpectedQuantityBefore = 4;
	Foreign.PurposeId = TEXT("Test.P27_19.ForeignPendingIntent");
	TestTrue(TEXT("Foreign authority mutation is durable"),
		Fixture.Service.PreparePreparedRunQuantityIntentDurable(Foreign)
			.IsCommandSuccess());
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const auto Result = Fixture.Execute();
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	TestTrue(TEXT("An untouched stale plan fails before its first command"),
		Result.Status == EPreparationStatus::PlanStale
			&& Result.IsValid()
			&& Result.PrepareCommands.IsEmpty()
			&& Result.RollbackCommands.IsEmpty()
			&& After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourcePreparationCancellationTest,
	"Shanmen.0_0_10.Product.FormationScatterResourcePreparation.PartialCancellationTerminatesAttempt",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourcePreparationCancellationTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("Cancellation")))
	{
		AddError(TEXT("Could not build the P27.19 cancellation fixture."));
		return false;
	}
	const auto Prepared = Fixture.Execute();
	FShanmenItemRunQuantityIntentFinalizeRequest FirstCancel;
	if (!Prepared.IsPrepared()
		|| !Fdemo_mapShanmenFormationScatterResourcePreparation::
			BuildFinalizeRequest(Fixture.Plan, 0, false, FirstCancel)
		|| !Fixture.Service.FinalizePreparedRunQuantityIntentDurable(
			FirstCancel).IsCommandSuccess())
	{
		AddError(TEXT("Could not seed the P27.19 partial cancellation."));
		return false;
	}

	const auto Cancelled = Fixture.Execute();
	FShanmenItemAuthoritySnapshot Terminal;
	Fixture.Capture(Terminal);
	TestTrue(TEXT("One observed cancellation cancels every remaining line"),
		Cancelled.Status == EPreparationStatus::AttemptCancelled
			&& Cancelled.IsValid() && Cancelled.IsCancelled()
			&& Cancelled.PrepareCommands.IsEmpty()
			&& Cancelled.RollbackCommands.Num() == 2
			&& CountPendingPlanPrepares(Terminal, Fixture.Plan) == 0);
	for (const auto& Receipt : Cancelled.FinalizeReceipts)
	{
		TestTrue(TEXT("Every prepared line has a terminal cancellation"),
			Receipt.IsSuccess()
				&& Receipt.Phase
					== EShanmenItemTransactionPhase::Cancelled);
	}

	const auto Replay = Fixture.Execute();
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Capture(AfterReplay);
	TestTrue(TEXT("A cancelled plan is terminal and replays without commands"),
		Replay.Status == EPreparationStatus::AttemptCancelled
			&& Replay.IsValid() && Replay.IsCancelled()
			&& Replay.PrepareCommands.IsEmpty()
			&& Replay.RollbackCommands.IsEmpty()
			&& AfterReplay == Terminal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceCommitDurableReplayTest,
	"Shanmen.0_0_10.Product.FormationScatterResourceCommit.DurableWholeBatchCommitAndReplay",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourceCommitDurableReplayTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("CommitDurableReplay")))
	{
		AddError(TEXT("Could not build the P27.20 durable commit fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const auto First = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot Committed;
	Fixture.Capture(Committed);
	const FShanmenItemTransactionReceipt* WoodA =
		FindCommitReceiptForItem(First, WoodAId);
	const FShanmenItemTransactionReceipt* Stone =
		FindCommitReceiptForItem(First, StoneId);
	const FShanmenItemTransactionReceipt* WoodB =
		FindCommitReceiptForItem(First, WoodBId);
	TestTrue(TEXT("Every prepared stack reaches one durable commit"),
		First.Status == ECommitStatus::Committed
			&& First.IsValid() && First.IsCommitted()
			&& First.Preparation.IsPrepared()
			&& First.CommitCommands.Num() == 3
			&& CountPendingPlanPrepares(Committed, Fixture.Plan) == 0
			&& CountPlanCommits(Committed, Fixture.Plan) == 3);
	TestTrue(TEXT("Committed active-Run balances match the cumulative plan"),
		WoodA && Stone && WoodB
			&& WoodA->ResourceBefore == 4 && WoodA->Amount == 4
			&& WoodA->ResourceAfter == 0
			&& Stone->ResourceBefore == 1 && Stone->Amount == 1
			&& Stone->ResourceAfter == 0
			&& WoodB->ResourceBefore == 2 && WoodB->Amount == 1
			&& WoodB->ResourceAfter == 1
			&& SamePhysicalQuantities(Before, Committed));

	int32 FulfillmentLineCount = 0;
	bool bEastUsesWoodA = false;
	bool bNorthUsesWoodA = false;
	FGuid EastWoodReceipt;
	FGuid NorthWoodReceipt;
	for (const auto& Fulfillment :
		First.Evidence.GetAnchorFulfillments())
	{
		FulfillmentLineCount += Fulfillment.GetLines().Num();
		for (const auto& Line : Fulfillment.GetLines())
		{
			if (Line.GetItemInstanceId() != WoodAId)
			{
				continue;
			}
			if (Fulfillment.GetAnchorDefinitionId() == EastAnchor)
			{
				bEastUsesWoodA = true;
				EastWoodReceipt = Line.GetCommitReceiptId();
			}
			if (Fulfillment.GetAnchorDefinitionId() == NorthAnchor)
			{
				bNorthUsesWoodA = true;
				NorthWoodReceipt = Line.GetCommitReceiptId();
			}
		}
	}
	TestTrue(TEXT("Evidence attributes every allocation slice once"),
		First.Evidence.IsValid()
			&& First.Evidence.GetAnchorFulfillments().Num() == 2
			&& First.Evidence.GetRequirementCount() == 3
			&& First.Evidence.GetTotalCommittedQuantity() == 6
			&& FulfillmentLineCount == 4
			&& bEastUsesWoodA && bNorthUsesWoodA
			&& EastWoodReceipt.IsValid()
			&& EastWoodReceipt == NorthWoodReceipt);

	const auto Replay = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Capture(AfterReplay);
	TestTrue(TEXT("A complete terminal set replays without another command"),
		Replay.Status == ECommitStatus::Replayed
			&& Replay.IsValid() && Replay.IsCommitted()
			&& Replay.CommitCommands.IsEmpty()
			&& Replay.Evidence == First.Evidence
			&& AfterReplay == Committed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceCommitMiddleFailureTest,
	"Shanmen.0_0_10.Product.FormationScatterResourceCommit.MiddleFailureRequiresForwardRecovery",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourceCommitMiddleFailureTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("CommitMiddleFailure")))
	{
		AddError(TEXT("Could not build the P27.20 middle-failure fixture."));
		return false;
	}
	Fixture.Authority.InjectFinalizeFailureAt = 2;
	const auto Failed = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot Partial;
	Fixture.Capture(Partial);
	TestTrue(TEXT("A durable prefix makes a later failure forward-only"),
		Failed.Status == ECommitStatus::ForwardRecoveryRequired
			&& Failed.IsValid() && Failed.RequiresRecovery()
			&& Failed.CommitCommands.Num() == 2
			&& Failed.CommitCommands[0].IsCommandSuccess()
			&& Failed.CommitCommands[1].Status
				== EShanmenItemDurableCommandStatus::
					PersistenceFailedRolledBack
			&& CountPlanCommits(Partial, Fixture.Plan) == 1
			&& CountPendingPlanPrepares(Partial, Fixture.Plan) == 2);

	Fixture.Authority.InjectFinalizeFailureAt = INDEX_NONE;
	const auto Recovered = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot Complete;
	Fixture.Capture(Complete);
	TestTrue(TEXT("The next pass preserves the prefix and commits only the suffix"),
		Recovered.Status == ECommitStatus::Committed
			&& Recovered.IsValid() && Recovered.IsCommitted()
			&& Recovered.CommitCommands.Num() == 2
			&& CountPlanCommits(Complete, Fixture.Plan) == 3
			&& CountPendingPlanPrepares(Complete, Fixture.Plan) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceCommitFirstFailureTest,
	"Shanmen.0_0_10.Product.FormationScatterResourceCommit.FirstFailureRetriesWithoutConsumption",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourceCommitFirstFailureTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("CommitFirstFailure")))
	{
		AddError(TEXT("Could not build the P27.20 first-failure fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	Fixture.Authority.InjectFinalizeFailureAt = 1;
	const auto Failed = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot Pending;
	Fixture.Capture(Pending);
	TestTrue(TEXT("A first-line save failure leaves the whole batch retryable"),
		Failed.Status == ECommitStatus::CommitRetryRequired
			&& Failed.IsValid() && Failed.RequiresRecovery()
			&& Failed.CommitCommands.Num() == 1
			&& CountPlanCommits(Pending, Fixture.Plan) == 0
			&& CountPendingPlanPrepares(Pending, Fixture.Plan) == 3
			&& SamePhysicalQuantities(Before, Pending));

	Fixture.Authority.InjectFinalizeFailureAt = INDEX_NONE;
	const auto Retried = Fixture.ExecuteCommit();
	TestTrue(TEXT("An unchanged durable state retries the exact pass safely"),
		Retried.Status == ECommitStatus::Committed
			&& Retried.IsValid() && Retried.IsCommitted()
			&& Retried.CommitCommands.Num() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceCommitPersistedRejectionTest,
	"Shanmen.0_0_10.Product.FormationScatterResourceCommit.PersistedRejectionRotatesCommitPass",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourceCommitPersistedRejectionTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("CommitPersistedRejection")))
	{
		AddError(TEXT("Could not build the P27.20 persisted-rejection fixture."));
		return false;
	}
	Fixture.Authority.bPersistRejectFirstCommit = true;
	const auto Rejected = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot AfterRejected;
	Fixture.Capture(AfterRejected);
	TestTrue(TEXT("A valid mismatched command is rejected and recorded"),
		Rejected.Status == ECommitStatus::CommitRetryRequired
			&& Rejected.IsValid() && Rejected.RequiresRecovery()
			&& Rejected.CommitCommands.Num() == 1
			&& Rejected.CommitCommands[0].Status
				== EShanmenItemDurableCommandStatus::RejectedAndPersisted
			&& Rejected.CommitCommands[0].Receipt.Error
				== EShanmenItemTransactionError::ContentMismatch
			&& Fixture.Authority.FirstRejectedCommitRequestId.IsValid()
			&& CountPlanCommits(AfterRejected, Fixture.Plan) == 0
			&& CountPendingPlanPrepares(AfterRejected, Fixture.Plan) == 3);

	const auto Recovered = Fixture.ExecuteCommit();
	TestTrue(TEXT("Authority revision rotates the pass identity after rejection"),
		Recovered.Status == ECommitStatus::Committed
			&& Recovered.IsValid() && Recovered.IsCommitted()
			&& Recovered.CommitCommands.Num() == 3
			&& Recovered.CommitCommands[0].Receipt.RequestId
				!= Fixture.Authority.FirstRejectedCommitRequestId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterResourceCommitCancellationTest,
	"Shanmen.0_0_10.Product.FormationScatterResourceCommit.CancellationPreventsCommit",
	PreparationFlags)

bool Fdemo_mapFormationScatterResourceCommitCancellationTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("CommitCancellation")))
	{
		AddError(TEXT("Could not build the P27.20 cancellation fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const auto Prepared = Fixture.Execute();
	FShanmenItemRunQuantityIntentFinalizeRequest Cancel;
	if (!Prepared.IsPrepared()
		|| !Fdemo_mapShanmenFormationScatterResourcePreparation::
			BuildFinalizeRequest(Fixture.Plan, 0, false, Cancel)
		|| !Fixture.Service.FinalizePreparedRunQuantityIntentDurable(Cancel)
			.IsCommandSuccess())
	{
		AddError(TEXT("Could not seed the P27.20 cancelled attempt."));
		return false;
	}

	const auto Result = Fixture.ExecuteCommit();
	FShanmenItemAuthoritySnapshot Terminal;
	Fixture.Capture(Terminal);
	TestTrue(TEXT("One cancellation terminates the attempt before commit"),
		Result.Status == ECommitStatus::AttemptCancelled
			&& Result.IsValid()
			&& Result.CommitCommands.IsEmpty()
			&& CountPlanCommits(Terminal, Fixture.Plan) == 0
			&& CountPendingPlanPrepares(Terminal, Fixture.Plan) == 0
			&& SamePhysicalQuantities(Before, Terminal));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterDeploymentCommitWholeBatchTest,
	"Shanmen.0_0_10.Product.FormationScatterDeploymentCommit.WholeBatchCommitAndReplay",
	PreparationFlags)

bool Fdemo_mapFormationScatterDeploymentCommitWholeBatchTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("DeploymentWholeBatch")))
	{
		AddError(TEXT("Could not build the P27.21 whole-batch fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	if (!Resources.IsCommitted())
	{
		AddError(TEXT("Could not commit P27.20 resource evidence."));
		return false;
	}

	const int32 ReceiptCountBefore = Fixture.Deployment.GetReceipts().Num();
	const auto First =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	TestTrue(TEXT("Every resource-backed anchor commits in canonical order"),
		First.Status == EDeploymentCommitStatus::Committed
			&& First.IsValid() && First.IsCommitted()
			&& First.InitialCommittedAnchorCount == 0
			&& First.NewCommitReceipts.Num() == 2
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Active
			&& Fixture.Deployment.GetCommittedAnchorCount() == 2
			&& Fixture.Deployment.GetReceipts().Num()
				== ReceiptCountBefore + 2);
	TestTrue(TEXT("Completion evidence preserves all resource attribution"),
		First.Evidence.IsValid()
			&& First.Evidence.GetResourceEvidence() == Resources.Evidence
			&& First.Evidence.GetHandoffs().Num() == 2
			&& First.Evidence.GetTotalCommittedQuantity() == 6
			&& First.Evidence.GetHandoffs()[0].GetResourceFulfillment().
				GetAnchorDefinitionId() == EastAnchor
			&& First.Evidence.GetHandoffs()[1].GetResourceFulfillment().
				GetAnchorDefinitionId() == NorthAnchor);

	const int32 ReceiptCountCommitted = Fixture.Deployment.GetReceipts().Num();
	const auto Replay =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	TestTrue(TEXT("A complete deployment replays without another commit"),
		Replay.Status == EDeploymentCommitStatus::Replayed
			&& Replay.IsValid() && Replay.IsCommitted()
			&& Replay.InitialCommittedAnchorCount == 2
			&& Replay.NewCommitReceipts.IsEmpty()
			&& Replay.Evidence == First.Evidence
			&& Fixture.Deployment.GetReceipts().Num()
				== ReceiptCountCommitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterDeploymentCommitPrefixRecoveryTest,
	"Shanmen.0_0_10.Product.FormationScatterDeploymentCommit.CompatiblePrefixRecovery",
	PreparationFlags)

bool Fdemo_mapFormationScatterDeploymentCommitPrefixRecoveryTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("DeploymentPrefixRecovery")))
	{
		AddError(TEXT("Could not build the P27.21 prefix fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	FShanmenFormationAnchorFulfillmentEvidence FirstEvidence;
	FShanmenFormationDeploymentReceipt FirstReceipt;
	if (!Resources.IsCommitted()
		|| !Fdemo_mapShanmenFormationScatterDeploymentCommitter::
			BuildAnchorEvidence(Resources.Evidence, 0, FirstEvidence)
		|| !Fixture.Deployment.TryCommitAnchor(
			Fixture.Runtime, FirstEvidence, FirstReceipt))
	{
		AddError(TEXT("Could not seed the compatible committed prefix."));
		return false;
	}

	const auto Recovered =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	TestTrue(TEXT("A compatible prefix advances only its missing suffix"),
		Recovered.Status == EDeploymentCommitStatus::Committed
			&& Recovered.IsValid() && Recovered.IsCommitted()
			&& Recovered.InitialCommittedAnchorCount == 1
			&& Recovered.NewCommitReceipts.Num() == 1
			&& Recovered.NewCommitReceipts[0].GetAnchorDefinitionId()
				== NorthAnchor
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Active);
	TestTrue(TEXT("The recovered proof adopts the exact existing prefix"),
		Recovered.Evidence.GetHandoffs()[0].GetDeploymentReceipt().
			GetReceiptId() == FirstReceipt.GetReceiptId()
			&& Recovered.Evidence.GetHandoffs()[0].
				GetDeploymentEvidence().FulfillmentId
				== FirstEvidence.FulfillmentId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterDeploymentCommitConflictTest,
	"Shanmen.0_0_10.Product.FormationScatterDeploymentCommit.ConflictingPrefixRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterDeploymentCommitConflictTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("DeploymentConflict")))
	{
		AddError(TEXT("Could not build the P27.21 conflict fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	FShanmenFormationAnchorFulfillmentEvidence Foreign;
	FShanmenFormationDeploymentReceipt ForeignReceipt;
	if (!Resources.IsCommitted()
		|| !Fdemo_mapShanmenFormationScatterDeploymentCommitter::
			BuildAnchorEvidence(Resources.Evidence, 0, Foreign))
	{
		AddError(TEXT("Could not build the expected first-anchor evidence."));
		return false;
	}
	Foreign.FulfillmentId = FGuid(0xF8F19F01, 0, 0, 1);
	if (!Foreign.IsValid()
		|| !Fixture.Deployment.TryCommitAnchor(
			Fixture.Runtime, Foreign, ForeignReceipt))
	{
		AddError(TEXT("Could not seed the conflicting committed prefix."));
		return false;
	}

	const int32 ReceiptCount = Fixture.Deployment.GetReceipts().Num();
	const auto Rejected =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	TestTrue(TEXT("A foreign committed prefix fails closed"),
		Rejected.Status == EDeploymentCommitStatus::ExistingAnchorConflict
			&& Rejected.IsValid() && !Rejected.IsCommitted()
			&& Rejected.NewCommitReceipts.IsEmpty()
			&& Fixture.Deployment.GetCommittedAnchorCount() == 1
			&& Fixture.Deployment.GetReceipts().Num() == ReceiptCount
			&& !Fixture.Deployment.GetAnchors()[1].IsCommitted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterDeploymentCommitTerminalTest,
	"Shanmen.0_0_10.Product.FormationScatterDeploymentCommit.TerminalDeploymentRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterDeploymentCommitTerminalTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("DeploymentTerminal")))
	{
		AddError(TEXT("Could not build the P27.21 terminal fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	FShanmenFormationDeploymentReceipt CancelReceipt;
	if (!Resources.IsCommitted()
		|| !Fixture.Deployment.TryCancel(Fixture.Runtime, CancelReceipt))
	{
		AddError(TEXT("Could not seed the cancelled deployment."));
		return false;
	}

	const int32 ReceiptCount = Fixture.Deployment.GetReceipts().Num();
	const auto Rejected =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	TestTrue(TEXT("Committed resources cannot revive a terminal deployment"),
		Rejected.Status == EDeploymentCommitStatus::DeploymentTerminal
			&& Rejected.IsValid() && !Rejected.IsCommitted()
			&& Rejected.NewCommitReceipts.IsEmpty()
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Cancelled
			&& Fixture.Deployment.GetCommittedAnchorCount() == 0
			&& Fixture.Deployment.GetReceipts().Num() == ReceiptCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterDeploymentCommitInvalidEvidenceTest,
	"Shanmen.0_0_10.Product.FormationScatterDeploymentCommit.InvalidResourceEvidenceRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterDeploymentCommitInvalidEvidenceTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("DeploymentInvalidEvidence")))
	{
		AddError(TEXT("Could not build the P27.21 invalid-evidence fixture."));
		return false;
	}
	const int32 ReceiptCount = Fixture.Deployment.GetReceipts().Num();
	const auto Rejected =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime,
			Fixture.Deployment,
			Fdemo_mapShanmenFormationScatterResourceCommitEvidence());
	TestTrue(TEXT("Invalid resource evidence is rejected before mutation"),
		Rejected.Status
			== EDeploymentCommitStatus::ResourceEvidenceInvalid
			&& Rejected.IsValid() && !Rejected.IsCommitted()
			&& Rejected.InitialCommittedAnchorCount == INDEX_NONE
			&& Rejected.NewCommitReceipts.IsEmpty()
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Deploying
			&& Fixture.Deployment.GetCommittedAnchorCount() == 0
			&& Fixture.Deployment.GetReceipts().Num() == ReceiptCount);
	return true;
}

#endif
