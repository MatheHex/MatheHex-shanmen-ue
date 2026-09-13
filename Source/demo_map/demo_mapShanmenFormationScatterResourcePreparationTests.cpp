#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

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
			FShanmenItemDurableCommandResult Result =
				Service.FinalizePreparedRunQuantityIntentDurable(Request);
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

#endif
