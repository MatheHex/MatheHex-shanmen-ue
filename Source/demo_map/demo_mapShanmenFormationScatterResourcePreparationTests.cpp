#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationScatterDeploymentCommit.h"
#include "demo_mapShanmenFormationScatterResourceCommit.h"
#include "demo_mapShanmenFormationScatterResourcePreparation.h"
#include "demo_mapShanmenFormationScatterWorldPlacementHandoff.h"
#include "demo_mapShanmenFormationScatterWorldPublication.h"
#include "demo_mapShanmenFormationScatterWorldPublicationCommandHost.h"
#include "demo_mapShanmenFormationScatterWorldPublicationRunRoute.h"
#include "demo_mapShanmenFormationScatterWorldPublicationSession.h"
#include "demo_mapShanmenFormationRunLifecycle.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	using EPreparationStatus =
		Edemo_mapShanmenFormationScatterResourcePreparationStatus;
	using ECommitStatus =
		Edemo_mapShanmenFormationScatterResourceCommitStatus;
	using EDeploymentCommitStatus =
		Edemo_mapShanmenFormationScatterDeploymentCommitStatus;
	using EWorldHandoffStatus =
		Edemo_mapShanmenFormationScatterWorldPlacementHandoffStatus;
	using EWorldPublicationStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationStatus;
	using EWorldPublicationSessionState =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionState;
	using EWorldPublicationSessionStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionStatus;
	using EWorldPublicationCommandOperation =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation;
	using EWorldPublicationCommandStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandStatus;
	using EWorldPublicationRunEvent =
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent;
	using EWorldPublicationRunRouteStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationRunRouteStatus;

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

	FString PublicationGuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt
	MakePublicationReceipt(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent,
		const FString& ActorClassPath)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Receipt.Intent = Intent;
		Receipt.ActorClassPath = ActorClassPath;
		Receipt.PlacementTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
				Intent.PlacementId);
		Receipt.DeploymentTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Intent.DeploymentId);
		Receipt.ReceiptId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacementReceipt.r1"),
			{ PublicationGuidDigits(Intent.PlacementId), ActorClassPath });
		return Receipt;
	}

	bool BuildWorldHandoffEvidence(
		FPreparationFixture& Fixture,
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			OutEvidence)
	{
		OutEvidence =
			Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence();
		const auto Resources = Fixture.ExecuteCommit();
		const auto Deployment =
			Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
				Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
		if (!Resources.IsCommitted() || !Deployment.IsCommitted())
		{
			return false;
		}
		const auto Handoff =
			Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::
				Build(Deployment.Evidence);
		if (!Handoff.IsReady())
		{
			return false;
		}
		OutEvidence = Handoff.Evidence;
		return OutEvidence.IsValid();
	}

	class FFakeScatterWorldPlacementPort final
		: public Idemo_mapShanmenFormationScatterWorldPlacementPort
	{
	public:
		explicit FFakeScatterWorldPlacementPort(
			FString InActorClassPath =
				TEXT("/Script/demo_map.P27_23FakeAnchor"))
			: ActorClassPath(MoveTemp(InActorClassPath))
		{
		}

		virtual FString GetActorClassPath() const override
		{
			return ActorClassPath;
		}

		virtual Fdemo_mapShanmenFormationWorldResult Publish(
			const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent)
			override
		{
			const int32 ThisCall = PublishCallCount++;
			if (RejectOnceAtCall == ThisCall)
			{
				RejectOnceAtCall = INDEX_NONE;
				Fdemo_mapShanmenFormationWorldResult Rejected;
				Rejected.Status =
					Edemo_mapShanmenFormationWorldStatus::SpawnRejected;
				Rejected.Diagnostic = TEXT("Injected bounded World rejection.");
				Rejected.Intent = Intent;
				return Rejected;
			}

			const FString ReceiptClassPath = bReturnForeignClassReceipt
				? TEXT("/Script/demo_map.P27_23ForeignAnchor")
				: ActorClassPath;
			const bool bReplay = Receipts.Contains(Intent.PlacementId);
			Fdemo_mapShanmenFormationAnchorPlacementReceipt& Receipt =
				Receipts.FindOrAdd(Intent.PlacementId);
			if (!bReplay)
			{
				Receipt = MakePublicationReceipt(Intent, ReceiptClassPath);
			}
			Fdemo_mapShanmenFormationWorldResult Result;
			Result.Status = bReplay
				? Edemo_mapShanmenFormationWorldStatus::Replayed
				: Edemo_mapShanmenFormationWorldStatus::Placed;
			Result.Diagnostic = bReplay
				? TEXT("Fake World replayed the exact placement.")
				: TEXT("Fake World accepted the exact placement.");
			Result.Intent = Intent;
			Result.PlacementReceipt = Receipt;
			return Result;
		}

		FString ActorClassPath;
		int32 RejectOnceAtCall = INDEX_NONE;
		int32 PublishCallCount = 0;
		bool bReturnForeignClassReceipt = false;
		TMap<FGuid, Fdemo_mapShanmenFormationAnchorPlacementReceipt> Receipts;
	};

	struct FScopedScatterPublicationWorld
	{
		UWorld* World = nullptr;

		bool Start()
		{
			if (!GEngine)
			{
				return false;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return false;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.SetCurrentWorld(World);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(false)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(false)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(false)
					.SetTransactional(false)
					.CreateFXSystem(false));
			return true;
		}

		~FScopedScatterPublicationWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}
	};

	int32 CountPublicationActors(UWorld* World, const FName DeploymentTag)
	{
		int32 Count = 0;
		if (!World || DeploymentTag.IsNone())
		{
			return Count;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (::IsValid(Actor) && !Actor->IsActorBeingDestroyed()
				&& Actor->ActorHasTag(DeploymentTag))
			{
				++Count;
			}
		}
		return Count;
	}

	TSet<AActor*> CollectPublicationActors(
		UWorld* World,
		const FName DeploymentTag)
	{
		TSet<AActor*> Actors;
		if (!World || DeploymentTag.IsNone())
		{
			return Actors;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (::IsValid(Actor) && !Actor->IsActorBeingDestroyed()
				&& Actor->ActorHasTag(DeploymentTag))
			{
				Actors.Add(Actor);
			}
		}
		return Actors;
	}

	bool PublicationActorSetsMatch(
		const TSet<AActor*>& Left,
		const TSet<AActor*>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (AActor* Actor : Left)
		{
			if (!Right.Contains(Actor))
			{
				return false;
			}
		}
		return true;
	}

	struct FScatterPublicationRunLifecycleFixture
	{
		FPreparationFixture Preparation;
		FScopedScatterPublicationWorld ScopedWorld;
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* ItemAuthority = nullptr;
		APawn* Player = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Fdemo_mapCombatRunCoordinator CombatRun;
		Fdemo_mapShanmenFormationRunLifecycle Lifecycle;
		FString Diagnostic;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!Preparation.Build(Label)
				|| !BuildWorldHandoffEvidence(Preparation, Handoff)
				|| !ScopedWorld.Start() || !GEngine)
			{
				Test.AddError(TEXT(
					"Could not build P27.27 committed Handoff and World."));
				return false;
			}

			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			ScopedWorld.World->SetGameInstance(GameInstance);
			ItemAuthority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Player = ScopedWorld.World->SpawnActor<APawn>(
				APawn::StaticClass(), FTransform::Identity, Parameters);
			UBoxComponent* PlayerRoot = Player
				? NewObject<UBoxComponent>(
					Player, TEXT("P2727PlayerRoot"), RF_Transient)
				: nullptr;
			PlayerHealth = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player, TEXT("P2727PlayerHealth"), RF_Transient)
				: nullptr;
			if (!ItemAuthority || !Player || !PlayerRoot || !PlayerHealth)
			{
				return false;
			}
			Player->SetRootComponent(PlayerRoot);
			Player->AddInstanceComponent(PlayerRoot);
			Player->AddInstanceComponent(PlayerHealth);

			const FGuid RunId = Handoff.GetDeploymentEvidence()
				.GetResourceEvidence().GetPlan().GetActiveRunId();
			if (!CombatRun.TryBeginRun(
					RunId, Player, PlayerHealth, Diagnostic)
				|| !Lifecycle.TryBegin(CombatRun, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P27.27 lifecycle binding failed: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		FGuid GetRunId() const
		{
			return Handoff.GetDeploymentEvidence()
				.GetResourceEvidence().GetPlan().GetActiveRunId();
		}

		FName GetDeploymentTag() const
		{
			return Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Handoff.GetHandoffs()[0].GetPlacementIntent().DeploymentId);
		}

		void Stop()
		{
			if (ScopedWorld.World)
			{
				ScopedWorld.World->SetGameInstance(nullptr);
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
				ItemAuthority = nullptr;
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
			}
		}

		~FScatterPublicationRunLifecycleFixture()
		{
			Stop();
		}
	};
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPlacementHandoffWholeBatchTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPlacementHandoff.WholeBatchIntent",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPlacementHandoffWholeBatchTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("WorldHandoffWholeBatch")))
	{
		AddError(TEXT("Could not build the P27.22 whole-batch fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	const auto Deployment =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	if (!Resources.IsCommitted() || !Deployment.IsCommitted())
	{
		AddError(TEXT("Could not build complete P27.21 deployment evidence."));
		return false;
	}

	const auto Result =
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::Build(
			Deployment.Evidence);
	TestTrue(TEXT("Every committed scatter anchor becomes one canonical intent"),
		Result.Status == EWorldHandoffStatus::Ready
			&& Result.IsValid() && Result.IsReady()
			&& Result.Evidence.GetHandoffs().Num() == 2
			&& Result.Evidence.GetTotalCommittedQuantity() == 6);

	bool bExactChain = Result.IsReady();
	TSet<FGuid> PlacementIds;
	for (int32 Index = 0;
		bExactChain && Index < Result.Evidence.GetHandoffs().Num(); ++Index)
	{
		const auto& Handoff = Result.Evidence.GetHandoffs()[Index];
		const auto& Source = Deployment.Evidence.GetHandoffs()[Index];
		const auto& Progress = Fixture.Deployment.GetAnchors()[Index];
		const auto& Intent = Handoff.GetPlacementIntent();
		bExactChain = Handoff.IsValid()
			&& Handoff.GetAnchorOrder() == Index
			&& Handoff.GetDeploymentCommitEvidenceId()
				== Deployment.Evidence.GetEvidenceId()
			&& Handoff.GetDeploymentHandoffId() == Source.GetHandoffId()
			&& Intent.AttemptId == Source.GetHandoffId()
			&& Intent.RunId == Fixture.Correlation.ActiveRunId
			&& Intent.OwnerId == OwnerId
			&& Intent.DeploymentId == Fixture.Deployment.GetDeploymentId()
			&& Intent.AnchorDefinitionId
				== Progress.GetAnchorDefinitionId()
			&& Intent.AnchorInstanceId == Progress.GetAnchorInstanceId()
			&& Intent.WorldLocation.Equals(
				Progress.GetWorldLocation(), KINDA_SMALL_NUMBER)
			&& Intent.FulfillmentId
				== Source.GetDeploymentEvidence().FulfillmentId
			&& Intent.DeploymentReceiptId
				== Source.GetDeploymentReceipt().GetReceiptId()
			&& !PlacementIds.Contains(Intent.PlacementId);
		PlacementIds.Add(Intent.PlacementId);
	}
	TestTrue(TEXT("The handoff preserves the exact deployment-to-placement chain"),
		bExactChain && PlacementIds.Num() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPlacementHandoffReplayTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPlacementHandoff.DeterministicReplayIsReadOnly",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPlacementHandoffReplayTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("WorldHandoffReplay")))
	{
		AddError(TEXT("Could not build the P27.22 replay fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	const auto Deployment =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	FShanmenItemAuthoritySnapshot Before;
	if (!Resources.IsCommitted() || !Deployment.IsCommitted()
		|| !Fixture.Capture(Before))
	{
		AddError(TEXT("Could not capture the committed P27.21 baseline."));
		return false;
	}
	const int32 ReceiptCount = Fixture.Deployment.GetReceipts().Num();
	const int32 CommittedCount = Fixture.Deployment.GetCommittedAnchorCount();

	const auto First =
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::Build(
			Deployment.Evidence);
	const auto Replay =
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::Build(
			Deployment.Evidence);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	TestTrue(TEXT("Exact replay reconstructs byte-stable logical evidence"),
		First.IsReady() && Replay.IsReady()
			&& Replay.Evidence == First.Evidence);
	TestTrue(TEXT("Planning cannot reconsume resources or mutate deployment"),
		After == Before
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Active
			&& Fixture.Deployment.GetCommittedAnchorCount() == CommittedCount
			&& Fixture.Deployment.GetReceipts().Num() == ReceiptCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPlacementHandoffMismatchTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPlacementHandoff.ForeignEvidenceRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPlacementHandoffMismatchTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	if (!Fixture.Build(TEXT("WorldHandoffMismatch")))
	{
		AddError(TEXT("Could not build the P27.22 mismatch fixture."));
		return false;
	}
	const auto Resources = Fixture.ExecuteCommit();
	const auto Deployment =
		Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
			Fixture.Runtime, Fixture.Deployment, Resources.Evidence);
	if (!Resources.IsCommitted() || !Deployment.IsCommitted())
	{
		AddError(TEXT("Could not build complete P27.21 mismatch evidence."));
		return false;
	}

	const auto& Source = Deployment.Evidence.GetHandoffs()[0];
	FShanmenFormationAnchorFulfillmentEvidence Foreign =
		Source.GetDeploymentEvidence();
	Foreign.FulfillmentId = FGuid(0xF8F19F22, 0, 0, 1);
	Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
	const bool bForeignAccepted =
		Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
			Deployment.Evidence.GetDeployment(), Source.GetHandoffId(),
			Foreign, Source.GetDeploymentReceipt(), Intent);
	const bool bMissingAttemptAccepted =
		Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
			Deployment.Evidence.GetDeployment(), FGuid(),
			Source.GetDeploymentEvidence(), Source.GetDeploymentReceipt(),
			Intent);
	TestTrue(TEXT("Foreign fulfillment and missing handoff identity fail closed"),
		Foreign.IsValid() && !bForeignAccepted
			&& !bMissingAttemptAccepted && !Intent.IsValid());

	Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff First;
	Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff Missing;
	TestTrue(TEXT("Only an in-range canonical P27.21 anchor can be bridged"),
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::
			BuildAnchorHandoff(Deployment.Evidence, 0, First)
			&& First.IsValid()
			&& !Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::
				BuildAnchorHandoff(Deployment.Evidence, 2, Missing)
			&& !Missing.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPlacementHandoffInvalidTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPlacementHandoff.InvalidCompletionRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPlacementHandoffInvalidTest::RunTest(
	const FString&)
{
	const auto Result =
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::Build(
			Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence());
	TestTrue(TEXT("Missing P27.21 completion evidence is rejected"),
		Result.Status == EWorldHandoffStatus::DeploymentEvidenceInvalid
			&& Result.IsValid() && !Result.IsReady()
			&& !Result.Evidence.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationWholeBatchTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublication.ConcreteWholeBatchAndReplay",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationWholeBatchTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationWholeBatch"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.23 concrete World fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const int32 DeploymentReceiptCount =
		Fixture.Deployment.GetReceipts().Num();

	Fdemo_mapShanmenFormationWorldAdapter Adapter;
	Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort Port(
		ScopedWorld.World, ACharacter::StaticClass(), Adapter);
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger Ledger;
	const auto First =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, Port, Ledger);
	const auto FirstEvidence = First.Evidence;
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Handoff.GetDeploymentEvidence()
				.GetDeployment().GetDeploymentId());
	TestTrue(TEXT("The concrete port publishes the complete canonical batch"),
		First.Status == EWorldPublicationStatus::Published
			&& First.IsValid() && First.IsSuccess()
			&& First.InitialPublishedCount == 0
			&& First.FinalPublishedCount == 2
			&& First.NewRecords.Num() == 2
			&& First.Evidence.IsValid()
			&& Ledger.GetPublishedCount() == 2
			&& Adapter.IsValid() && Adapter.GetPlacementCount() == 2
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 2);

	const auto Replay =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, Port, Ledger);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	TestTrue(TEXT("Exact replay proves the World prefix without growth"),
		Replay.Status == EWorldPublicationStatus::Replayed
			&& Replay.IsValid() && Replay.IsSuccess()
			&& Replay.InitialPublishedCount == 2
			&& Replay.FinalPublishedCount == 2
			&& Replay.NewRecords.IsEmpty()
			&& Replay.Evidence == FirstEvidence
			&& Ledger.GetPublishedCount() == 2
			&& Adapter.GetPlacementCount() == 2
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 2);
	TestTrue(TEXT("World publication cannot reconsume or rewrite source facts"),
		After == Before
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Active
			&& Fixture.Deployment.GetReceipts().Num()
				== DeploymentReceiptCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationRecoveryTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublication.PartialFailureRecoversPrefix",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationRecoveryTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	if (!Fixture.Build(TEXT("WorldPublicationRecovery"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff))
	{
		AddError(TEXT("Could not build the P27.23 recovery fixture."));
		return false;
	}

	FFakeScatterWorldPlacementPort Port;
	Port.RejectOnceAtCall = 1;
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger Ledger;
	const auto Partial =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, Port, Ledger);
	const bool bPartialRetained =
		Partial.Status == EWorldPublicationStatus::PublicationRejected
			&& Partial.IsValid() && !Partial.IsSuccess()
			&& Partial.InitialPublishedCount == 0
			&& Partial.FinalPublishedCount == 1
			&& Partial.FailedAnchorOrder == 1
			&& Partial.NewRecords.Num() == 1
			&& Ledger.IsValid() && Ledger.GetPublishedCount() == 1;
	TestTrue(TEXT("A middle rejection retains only the proven prefix"),
		bPartialRetained);
	if (!bPartialRetained)
	{
		return false;
	}

	const FGuid FirstRecordId = Ledger.GetRecords()[0].GetRecordId();
	const auto Recovered =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, Port, Ledger);
	TestTrue(TEXT("Retry verifies the prefix then publishes only the suffix"),
		Recovered.Status == EWorldPublicationStatus::Recovered
			&& Recovered.IsValid() && Recovered.IsSuccess()
			&& Recovered.InitialPublishedCount == 1
			&& Recovered.FinalPublishedCount == 2
			&& Recovered.NewRecords.Num() == 1
			&& Ledger.GetRecords()[0].GetRecordId() == FirstRecordId
			&& Port.Receipts.Num() == 2
			&& Recovered.Evidence.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationConflictTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublication.ForeignLedgerAndActorClassRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationConflictTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	if (!Fixture.Build(TEXT("WorldPublicationConflict"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff))
	{
		AddError(TEXT("Could not build the P27.23 conflict fixture."));
		return false;
	}
	FFakeScatterWorldPlacementPort Port;
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger Ledger;
	const auto Published =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, Port, Ledger);
	if (!Published.IsSuccess())
	{
		AddError(TEXT("Could not seed the P27.23 complete ledger."));
		return false;
	}

	FPreparationFixture ForeignFixture;
	ForeignFixture.Content.Digest = TEXT("foreign-publication-source");
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence
		ForeignHandoff;
	if (!ForeignFixture.Build(TEXT("WorldPublicationForeign"))
		|| !BuildWorldHandoffEvidence(ForeignFixture, ForeignHandoff))
	{
		AddError(TEXT("Could not build the foreign P27.23 evidence."));
		return false;
	}
	const int32 CallsBeforeConflict = Port.PublishCallCount;
	const auto ForeignRejected =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			ForeignHandoff, Port, Ledger);
	FFakeScatterWorldPlacementPort ForeignClassPort(
		TEXT("/Script/demo_map.P27_23OtherAnchor"));
	const auto ClassRejected =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, ForeignClassPort, Ledger);
	TestTrue(TEXT("A ledger cannot cross evidence or Actor-class ownership"),
		ForeignRejected.Status == EWorldPublicationStatus::LedgerConflict
			&& ForeignRejected.IsValid() && !ForeignRejected.IsSuccess()
			&& ClassRejected.Status
				== EWorldPublicationStatus::LedgerConflict
			&& ClassRejected.IsValid() && !ClassRejected.IsSuccess()
			&& Port.PublishCallCount == CallsBeforeConflict
			&& ForeignClassPort.PublishCallCount == 0
			&& Ledger.GetPublishedCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationInvalidTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublication.InvalidSourcePortAndReceiptRejected",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationInvalidTest::RunTest(
	const FString&)
{
	FFakeScatterWorldPlacementPort Port;
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger EmptyLedger;
	const auto InvalidSource =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence(),
			Port, EmptyLedger);
	TestTrue(TEXT("Missing P27.22 evidence is rejected before publication"),
		InvalidSource.Status
				== EWorldPublicationStatus::HandoffEvidenceInvalid
			&& InvalidSource.IsValid() && !InvalidSource.IsSuccess()
			&& Port.PublishCallCount == 0 && EmptyLedger.IsEmpty());

	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	if (!Fixture.Build(TEXT("WorldPublicationInvalid"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff))
	{
		AddError(TEXT("Could not build the P27.23 invalid-input fixture."));
		return false;
	}
	FFakeScatterWorldPlacementPort EmptyPathPort{ FString() };
	const auto InvalidPort =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, EmptyPathPort, EmptyLedger);
	FFakeScatterWorldPlacementPort ForgedReceiptPort;
	ForgedReceiptPort.bReturnForeignClassReceipt = true;
	const auto ForgedReceipt =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			Handoff, ForgedReceiptPort, EmptyLedger);
	TestTrue(TEXT("Invalid ports and self-valid foreign receipts fail closed"),
		InvalidPort.Status == EWorldPublicationStatus::PortInvalid
			&& InvalidPort.IsValid() && !InvalidPort.IsSuccess()
			&& EmptyPathPort.PublishCallCount == 0
			&& ForgedReceipt.Status
				== EWorldPublicationStatus::ReceiptInvalid
			&& ForgedReceipt.IsValid() && !ForgedReceipt.IsSuccess()
			&& ForgedReceipt.FailedAnchorOrder == 0
			&& EmptyLedger.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationSessionLifecycleTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationSession.PublishReplayAndTerminalTeardown",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationSessionLifecycleTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationSessionLifecycle"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.24 lifecycle fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const int32 DeploymentReceiptCount =
		Fixture.Deployment.GetReceipts().Num();

	Fdemo_mapShanmenFormationScatterWorldPublicationSession Session;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, ACharacter::StaticClass(), Session))
	{
		AddError(TEXT("Could not start the P27.24 publication session."));
		return false;
	}
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Handoff.GetDeploymentEvidence()
				.GetDeployment().GetDeploymentId());
	const auto Published = Session.TryPublish(ScopedWorld.World);
	TestTrue(TEXT("The session owns one complete World publication"),
		Published.Status == EWorldPublicationSessionStatus::Published
			&& Published.IsValid() && Published.IsPublicationSuccess()
			&& Session.IsValid()
			&& Session.GetState()
				== EWorldPublicationSessionState::Published
			&& Session.IsBoundToWorld(ScopedWorld.World)
			&& Session.GetLedger().GetPublishedCount() == 2
			&& Session.GetCompletionEvidence().IsValid()
			&& Session.GetWorldAdapter().GetPlacementCount() == 2
			&& CountPublicationActors(
				ScopedWorld.World, DeploymentTag) == 2);

	const auto Completion = Session.GetCompletionEvidence();
	const auto Replayed = Session.TryPublish(ScopedWorld.World);
	TestTrue(TEXT("Exact publication replay preserves every owned fact"),
		Replayed.Status == EWorldPublicationSessionStatus::Replayed
			&& Replayed.IsValid() && Replayed.IsPublicationSuccess()
			&& Session.GetCompletionEvidence() == Completion
			&& Session.GetLedger().GetPublishedCount() == 2
			&& Session.GetWorldAdapter().GetPlacementCount() == 2
			&& CountPublicationActors(
				ScopedWorld.World, DeploymentTag) == 2);

	const auto Ended = Session.TryTeardown(
		ScopedWorld.World,
		Edemo_mapShanmenFormationSessionState::Ended);
	TestTrue(TEXT("An explicit terminal command removes the owned batch"),
		Ended.Status == EWorldPublicationSessionStatus::TeardownComplete
			&& Ended.IsValid() && Ended.IsTeardownSuccess()
			&& Session.IsValid() && Session.IsTerminal()
			&& Session.GetState() == EWorldPublicationSessionState::Ended
			&& Session.GetTeardownReceipt().IsValid()
			&& Session.GetTeardownReceipt().CommittedAnchorCount == 2
			&& Session.GetTeardownReceipt().RemovedActorCount == 2
			&& CountPublicationActors(
				ScopedWorld.World, DeploymentTag) == 0);

	const auto TeardownReceipt = Session.GetTeardownReceipt();
	const auto EndReplay = Session.TryTeardown(
		ScopedWorld.World,
		Edemo_mapShanmenFormationSessionState::Ended);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	TestTrue(TEXT("Terminal replay is stable and source facts stay immutable"),
		EndReplay.Status
				== EWorldPublicationSessionStatus::TeardownReplayed
			&& EndReplay.IsValid() && EndReplay.IsTeardownSuccess()
			&& Session.GetTeardownReceipt().ReceiptId
				== TeardownReceipt.ReceiptId
			&& Session.GetTeardownReceipt().RemovedActorCount == 2
			&& After == Before
			&& Fixture.Deployment.GetState()
				== EShanmenFormationDeploymentState::Active
			&& Fixture.Deployment.GetReceipts().Num()
				== DeploymentReceiptCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationSessionRecoveryTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationSession.ReconstructionAdoptionAndDirectTeardown",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationSessionRecoveryTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationSessionRecovery"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.24 recovery fixture."));
		return false;
	}
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Handoff.GetDeploymentEvidence()
				.GetDeployment().GetDeploymentId());

	Fdemo_mapShanmenFormationScatterWorldPublicationSession Original;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, ACharacter::StaticClass(), Original)
		|| !Original.TryPublish(ScopedWorld.World).IsPublicationSuccess())
	{
		AddError(TEXT("Could not seed the P27.24 World publication."));
		return false;
	}
	const TSet<AActor*> OriginalActors = CollectPublicationActors(
		ScopedWorld.World, DeploymentTag);

	Fdemo_mapShanmenFormationScatterWorldPublicationSession Reconstructed;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, ACharacter::StaticClass(), Reconstructed))
	{
		AddError(TEXT("Could not reconstruct the P27.24 publication session."));
		return false;
	}
	const auto Adopted = Reconstructed.TryPublish(ScopedWorld.World);
	const TSet<AActor*> AdoptedActors = CollectPublicationActors(
		ScopedWorld.World, DeploymentTag);
	TestTrue(TEXT("A reconstructed owner adopts the exact tagged Actors"),
		Adopted.Status == EWorldPublicationSessionStatus::Published
			&& Adopted.IsValid() && Adopted.IsPublicationSuccess()
			&& Reconstructed.GetSessionId() == Original.GetSessionId()
			&& Reconstructed.GetLedger().GetPublishedCount() == 2
			&& Reconstructed.GetWorldAdapter().GetPlacementCount() == 2
			&& PublicationActorSetsMatch(OriginalActors, AdoptedActors));

	Fdemo_mapShanmenFormationScatterWorldPublicationSession CleanupOwner;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, ACharacter::StaticClass(), CleanupOwner))
	{
		AddError(TEXT("Could not start the P27.24 cleanup recovery owner."));
		return false;
	}
	const auto Cancelled = CleanupOwner.TryTeardown(
		ScopedWorld.World,
		Edemo_mapShanmenFormationSessionState::Cancelled);
	TestTrue(TEXT("A fresh owner can recover terminal cleanup from World tags"),
		Cancelled.Status
				== EWorldPublicationSessionStatus::TeardownComplete
			&& Cancelled.IsValid() && Cancelled.IsTeardownSuccess()
			&& CleanupOwner.IsValid() && CleanupOwner.IsTerminal()
			&& CleanupOwner.GetLedger().IsEmpty()
			&& CleanupOwner.GetTeardownReceipt().RemovedActorCount == 2
			&& CountPublicationActors(
				ScopedWorld.World, DeploymentTag) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationSessionConflictTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationSession.WorldAndTerminalConflicts",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationSessionConflictTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld FirstWorld;
	FScopedScatterPublicationWorld SecondWorld;
	if (!Fixture.Build(TEXT("WorldPublicationSessionConflict"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !FirstWorld.Start() || !SecondWorld.Start())
	{
		AddError(TEXT("Could not build the P27.24 conflict fixture."));
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationSession Session;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, ACharacter::StaticClass(), Session)
		|| !Session.TryPublish(FirstWorld.World).IsPublicationSuccess())
	{
		AddError(TEXT("Could not seed the P27.24 conflict session."));
		return false;
	}
	const auto ForeignPublish = Session.TryPublish(SecondWorld.World);
	const auto ForeignTeardown = Session.TryTeardown(
		SecondWorld.World,
		Edemo_mapShanmenFormationSessionState::Ended);
	TestTrue(TEXT("A session cannot publish or teardown across Worlds"),
		ForeignPublish.Status == EWorldPublicationSessionStatus::WorldConflict
			&& ForeignPublish.IsValid()
			&& ForeignTeardown.Status
				== EWorldPublicationSessionStatus::WorldConflict
			&& ForeignTeardown.IsValid()
			&& Session.IsBoundToWorld(FirstWorld.World)
			&& Session.GetLedger().GetPublishedCount() == 2);

	const auto Ended = Session.TryTeardown(
		FirstWorld.World,
		Edemo_mapShanmenFormationSessionState::Ended);
	const FGuid ReceiptId = Session.GetTeardownReceipt().ReceiptId;
	const auto TerminalConflict = Session.TryTeardown(
		FirstWorld.World,
		Edemo_mapShanmenFormationSessionState::Cancelled);
	const auto PublishAfterTerminal = Session.TryPublish(FirstWorld.World);
	TestTrue(TEXT("A terminal session cannot change reason or publish again"),
		Ended.IsTeardownSuccess()
			&& TerminalConflict.Status
				== EWorldPublicationSessionStatus::TerminalConflict
			&& TerminalConflict.IsValid()
			&& PublishAfterTerminal.Status
				== EWorldPublicationSessionStatus::SessionTerminal
			&& PublishAfterTerminal.IsValid()
			&& Session.GetTeardownReceipt().ReceiptId == ReceiptId
			&& Session.GetState() == EWorldPublicationSessionState::Ended);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationSessionInvalidTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationSession.InvalidStartWorldAndTerminalState",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationSessionInvalidTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenFormationScatterWorldPublicationSession InvalidSession;
	Fdemo_mapShanmenFormationScatterWorldPublicationSession OutSession;
	TestTrue(TEXT("Missing evidence cannot start a product owner"),
		!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence(),
			ACharacter::StaticClass(), OutSession)
			&& !OutSession.IsValid()
			&& InvalidSession.TryPublish(nullptr).Status
				== EWorldPublicationSessionStatus::SessionInvalid);

	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationSessionInvalid"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.24 invalid fixture."));
		return false;
	}
	TestTrue(TEXT("A missing Actor class cannot start a product owner"),
		!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, TSubclassOf<AActor>(), OutSession)
			&& !OutSession.IsValid());
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			Handoff, ACharacter::StaticClass(), OutSession))
	{
		AddError(TEXT("Could not start the valid P27.24 control session."));
		return false;
	}
	const auto InvalidWorld = OutSession.TryPublish(nullptr);
	const auto InvalidTerminal = OutSession.TryTeardown(
		ScopedWorld.World,
		Edemo_mapShanmenFormationSessionState::Active);
	TestTrue(TEXT("Invalid World and nonterminal cleanup fail without binding"),
		InvalidWorld.Status == EWorldPublicationSessionStatus::WorldInvalid
			&& InvalidWorld.IsValid()
			&& InvalidTerminal.Status
				== EWorldPublicationSessionStatus::TerminalStateInvalid
			&& InvalidTerminal.IsValid()
			&& OutSession.IsValid()
			&& OutSession.GetState() == EWorldPublicationSessionState::Ready
			&& !OutSession.IsBoundToWorld(ScopedWorld.World)
			&& OutSession.GetLedger().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationCommandHostReplayTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationCommandHost.PublishReplayAndCommandIdentity",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationCommandHostReplayTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationCommandHostReplay"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.25 replay fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Host;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
			Handoff, ACharacter::StaticClass(), Host))
	{
		AddError(TEXT("Could not open the P27.25 command Host."));
		return false;
	}
	const FGuid PublishId(0xF8F25001, 0, 0, 1);
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Publish;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand ConflictingEnd;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand OtherPublish;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(PublishId, Host.GetBinding(), Publish)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCaptureEnd(PublishId, Host.GetBinding(), ConflictingEnd)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(
				FGuid(0xF8F25002, 0, 0, 2), Host.GetBinding(), OtherPublish))
	{
		AddError(TEXT("Could not capture P27.25 immutable commands."));
		return false;
	}
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Host.GetBinding().GetDeploymentId());
	const auto Applied = Host.TrySubmit(ScopedWorld.World, Publish);
	const auto Completion = Host.GetSession().GetCompletionEvidence();
	const auto Replayed = Host.TrySubmit(ScopedWorld.World, Publish);
	const auto IdConflict = Host.TrySubmit(ScopedWorld.World, ConflictingEnd);
	const auto OperationConflict =
		Host.TrySubmit(ScopedWorld.World, OtherPublish);
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord Record;
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	TestTrue(TEXT("One publish command owns one stable replayable receipt"),
		Applied.Status == EWorldPublicationCommandStatus::Applied
			&& Applied.IsValid() && Applied.IsSuccess()
			&& Applied.bHostStateCommitted && !Applied.bReplay
			&& Replayed.Status == EWorldPublicationCommandStatus::Replayed
			&& Replayed.IsValid() && Replayed.IsSuccess()
			&& Replayed.bReplay && !Replayed.bHostStateCommitted
			&& Host.GetSession().GetCompletionEvidence() == Completion
			&& Host.GetRecordCount() == 1
			&& Host.TryGetRecord(PublishId, Record)
			&& Record.Result.Status
				== EWorldPublicationCommandStatus::Applied
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 2
			&& After == Before);
	TestTrue(TEXT("Command and operation identities cannot be rewritten"),
		IdConflict.Status
				== EWorldPublicationCommandStatus::CommandIdConflict
			&& IdConflict.IsValid() && !IdConflict.IsSuccess()
			&& OperationConflict.Status
				== EWorldPublicationCommandStatus::OperationIdentityConflict
			&& OperationConflict.IsValid() && !OperationConflict.IsSuccess()
			&& Host.GetRecordCount() == 1
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationCommandHostTakeoverTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationCommandHost.TakeoverPreservesReceiptsAndTerminalRoute",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationCommandHostTakeoverTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationCommandHostTakeover"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.25 takeover fixture."));
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Previous;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
			Handoff, ACharacter::StaticClass(), Previous))
	{
		AddError(TEXT("Could not open the original P27.25 Host."));
		return false;
	}
	const FGuid HostId = Previous.GetBinding().GetHostId();
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Publish;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(
				FGuid(0xF8F25101, 0, 0, 1), Previous.GetBinding(), Publish)
		|| !Previous.TrySubmit(ScopedWorld.World, Publish).IsSuccess())
	{
		AddError(TEXT("Could not seed the P27.25 takeover Host."));
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Active;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::
			TryTakeover(Previous, Active))
	{
		AddError(TEXT("Could not transfer the P27.25 Host owner."));
		return false;
	}
	const auto PublishReplay = Active.TrySubmit(ScopedWorld.World, Publish);
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand End;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommand::TryCaptureEnd(
			FGuid(0xF8F25102, 0, 0, 2), Active.GetBinding(), End))
	{
		AddError(TEXT("Could not capture the P27.25 End command."));
		return false;
	}
	const auto Ended = Active.TrySubmit(ScopedWorld.World, End);
	const auto EndReplay = Active.TrySubmit(ScopedWorld.World, End);
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Active.GetBinding().GetDeploymentId());
	TestTrue(TEXT("Takeover invalidates the old owner and preserves command truth"),
		!Previous.IsValid() && Active.IsValid()
			&& Active.GetBinding().GetHostId() == HostId
			&& PublishReplay.Status
				== EWorldPublicationCommandStatus::Replayed
			&& PublishReplay.IsSuccess()
			&& Ended.Status == EWorldPublicationCommandStatus::Applied
			&& Ended.IsValid() && Ended.IsSuccess()
			&& Ended.Session.Status
				== EWorldPublicationSessionStatus::TeardownComplete
			&& EndReplay.Status == EWorldPublicationCommandStatus::Replayed
			&& EndReplay.IsValid() && EndReplay.IsSuccess()
			&& Active.GetRecordCount() == 2
			&& Active.GetSession().GetState()
				== EWorldPublicationSessionState::Ended
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationCommandHostRecoveryTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationCommandHost.ReconstructionAdoptsTaggedWorldBatch",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationCommandHostRecoveryTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationCommandHostRecovery"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.25 recovery fixture."));
		return false;
	}

	FGuid OriginalHostId;
	TSet<AActor*> OriginalActors;
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Original;
		if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
				Handoff, ACharacter::StaticClass(), Original))
		{
			AddError(TEXT("Could not open the original P27.25 recovery Host."));
			return false;
		}
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand Publish;
		if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
				TryCapturePublish(
					FGuid(0xF8F25201, 0, 0, 1),
					Original.GetBinding(), Publish)
			|| !Original.TrySubmit(ScopedWorld.World, Publish).IsSuccess())
		{
			AddError(TEXT("Could not seed the P27.25 recovery World."));
			return false;
		}
		OriginalHostId = Original.GetBinding().GetHostId();
		OriginalActors = CollectPublicationActors(
			ScopedWorld.World,
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Original.GetBinding().GetDeploymentId()));
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost RecoveredHost;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
			Handoff, ACharacter::StaticClass(), RecoveredHost))
	{
		AddError(TEXT("Could not reconstruct the P27.25 Host."));
		return false;
	}
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Publish;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Cancel;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(
				FGuid(0xF8F25201, 0, 0, 1),
				RecoveredHost.GetBinding(), Publish)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCaptureCancel(
				FGuid(0xF8F25202, 0, 0, 2),
				RecoveredHost.GetBinding(), Cancel))
	{
		AddError(TEXT("Could not capture P27.25 recovery commands."));
		return false;
	}
	const auto Adopted = RecoveredHost.TrySubmit(ScopedWorld.World, Publish);
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			RecoveredHost.GetBinding().GetDeploymentId());
	const TSet<AActor*> AdoptedActors =
		CollectPublicationActors(ScopedWorld.World, DeploymentTag);
	const auto Cancelled = RecoveredHost.TrySubmit(ScopedWorld.World, Cancel);
	TestTrue(TEXT("Reconstruction derives one identity and adopts the tagged batch"),
		RecoveredHost.GetBinding().GetHostId() == OriginalHostId
			&& Adopted.Status == EWorldPublicationCommandStatus::Applied
			&& Adopted.IsValid() && Adopted.IsSuccess()
			&& PublicationActorSetsMatch(OriginalActors, AdoptedActors)
			&& AdoptedActors.Num() == 2
			&& Cancelled.Status == EWorldPublicationCommandStatus::Applied
			&& Cancelled.IsValid() && Cancelled.IsSuccess()
			&& RecoveredHost.GetSession().GetState()
				== EWorldPublicationSessionState::Cancelled
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationCommandHostConflictTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationCommandHost.BindingWorldAndTerminalConflicts",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationCommandHostConflictTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld FirstWorld;
	FScopedScatterPublicationWorld SecondWorld;
	if (!Fixture.Build(TEXT("WorldPublicationCommandHostConflict"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !FirstWorld.Start() || !SecondWorld.Start())
	{
		AddError(TEXT("Could not build the P27.25 conflict fixture."));
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Host;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost ForeignHost;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
			Handoff, ACharacter::StaticClass(), Host)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
			Handoff, APawn::StaticClass(), ForeignHost))
	{
		AddError(TEXT("Could not open the P27.25 conflict Hosts."));
		return false;
	}
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Publish;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand ForeignPublish;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand End;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Cancel;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(
				FGuid(0xF8F25301, 0, 0, 1), Host.GetBinding(), Publish)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(
				FGuid(0xF8F25302, 0, 0, 2),
				ForeignHost.GetBinding(), ForeignPublish)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommand::TryCaptureEnd(
			FGuid(0xF8F25303, 0, 0, 3), Host.GetBinding(), End)
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCaptureCancel(
				FGuid(0xF8F25304, 0, 0, 4), Host.GetBinding(), Cancel))
	{
		AddError(TEXT("Could not capture P27.25 conflict commands."));
		return false;
	}
	const auto Invalid = Host.TrySubmit(
		FirstWorld.World,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand());
	const auto Foreign = Host.TrySubmit(FirstWorld.World, ForeignPublish);
	const auto Published = Host.TrySubmit(FirstWorld.World, Publish);
	const auto WrongWorld = Host.TrySubmit(SecondWorld.World, End);
	const auto Ended = Host.TrySubmit(FirstWorld.World, End);
	const auto TerminalConflict = Host.TrySubmit(FirstWorld.World, Cancel);
	TestTrue(TEXT("Invalid and foreign commands fail before product mutation"),
		Invalid.Status == EWorldPublicationCommandStatus::CommandInvalid
			&& Invalid.IsValid() && !Invalid.IsSuccess()
			&& Foreign.Status == EWorldPublicationCommandStatus::BindingConflict
			&& Foreign.IsValid() && !Foreign.IsSuccess());
	TestTrue(TEXT("World rejection is retryable but terminal identity is final"),
		Published.IsSuccess()
			&& WrongWorld.Status
				== EWorldPublicationCommandStatus::SessionRejected
			&& WrongWorld.IsValid() && !WrongWorld.IsSuccess()
			&& !WrongWorld.bHostStateCommitted
			&& WrongWorld.Session.Status
				== EWorldPublicationSessionStatus::WorldConflict
			&& Ended.Status == EWorldPublicationCommandStatus::Applied
			&& Ended.IsValid() && Ended.IsSuccess()
			&& TerminalConflict.Status
				== EWorldPublicationCommandStatus::OperationIdentityConflict
			&& TerminalConflict.IsValid() && !TerminalConflict.IsSuccess()
			&& Host.GetRecordCount() == 2
			&& Host.GetSession().GetState()
				== EWorldPublicationSessionState::Ended);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationRunRouteReplayTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationRunRoute.PublishReplayAndRunBinding",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationRunRouteReplayTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationRunRouteReplay"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.26 replay fixture."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Capture(Before);
	const FGuid RunId = Handoff.GetDeploymentEvidence()
		.GetResourceEvidence().GetPlan().GetActiveRunId();
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Route;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
			RunId, Handoff, ACharacter::StaticClass(), Route))
	{
		AddError(TEXT("Could not open the P27.26 Run route."));
		return false;
	}

	const auto Applied = Route.TryPublish(RunId, ScopedWorld.World);
	const auto Completion =
		Route.GetHost().GetSession().GetCompletionEvidence();
	const auto Replayed = Route.TryPublish(RunId, nullptr);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Capture(After);
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Route.GetHost().GetBinding().GetDeploymentId());
	TestTrue(TEXT("One Run route derives one replayable Publish command"),
		Route.IsValid() && Route.GetRunId() == RunId
			&& Applied.Status == EWorldPublicationRunRouteStatus::Applied
			&& Applied.IsValid() && Applied.IsSuccess()
			&& Applied.Event == EWorldPublicationRunEvent::Publish
			&& Applied.Command.Status
				== EWorldPublicationCommandStatus::Applied
			&& Replayed.Status == EWorldPublicationRunRouteStatus::Replayed
			&& Replayed.IsValid() && Replayed.IsSuccess()
			&& Replayed.Command.Status
				== EWorldPublicationCommandStatus::Replayed
			&& Replayed.Command.bReplay
			&& Applied.RouteId == Replayed.RouteId
			&& Applied.CommandId == Replayed.CommandId
			&& Route.GetHost().GetRecordCount() == 1
			&& Route.GetHost().GetSession().GetCompletionEvidence()
				== Completion
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 2
			&& After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationRunRouteTakeoverTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationRunRoute.TakeoverPreservesOwnerAndEndRoute",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationRunRouteTakeoverTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationRunRouteTakeover"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.26 takeover fixture."));
		return false;
	}
	const FGuid RunId = Handoff.GetDeploymentEvidence()
		.GetResourceEvidence().GetPlan().GetActiveRunId();
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Previous;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
			RunId, Handoff, ACharacter::StaticClass(), Previous))
	{
		AddError(TEXT("Could not open the original P27.26 route."));
		return false;
	}
	const auto Published = Previous.TryPublish(RunId, ScopedWorld.World);
	const FGuid RouteId = Previous.GetRouteId();
	const FGuid HostId = Previous.GetHost().GetBinding().GetHostId();
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Active;
	if (!Published.IsSuccess()
		|| !Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::
			TryTakeover(Previous, Active))
	{
		AddError(TEXT("Could not transfer the P27.26 route owner."));
		return false;
	}

	const auto PublishReplay = Active.TryPublish(RunId, nullptr);
	const auto Ended = Active.TryEnd(RunId, ScopedWorld.World);
	const auto EndReplay = Active.TryEnd(RunId, nullptr);
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Active.GetHost().GetBinding().GetDeploymentId());
	TestTrue(TEXT("Takeover invalidates only the old composition owner"),
		Previous.IsEmpty() && !Previous.IsValid()
			&& Active.IsValid() && Active.GetRouteId() == RouteId
			&& Active.GetHost().GetBinding().GetHostId() == HostId
			&& PublishReplay.Status
				== EWorldPublicationRunRouteStatus::Replayed
			&& PublishReplay.CommandId == Published.CommandId
			&& Ended.Status == EWorldPublicationRunRouteStatus::Applied
			&& Ended.IsValid() && Ended.IsSuccess()
			&& Ended.Event == EWorldPublicationRunEvent::End
			&& EndReplay.Status == EWorldPublicationRunRouteStatus::Replayed
			&& EndReplay.IsValid() && EndReplay.IsSuccess()
			&& EndReplay.CommandId == Ended.CommandId
			&& Active.GetHost().GetRecordCount() == 2
			&& Active.GetHost().GetSession().GetState()
				== EWorldPublicationSessionState::Ended
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationRunRouteReconstructionTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationRunRoute.ReconstructionAdoptsAndCancelsBatch",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationRunRouteReconstructionTest::
RunTest(const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld ScopedWorld;
	if (!Fixture.Build(TEXT("WorldPublicationRunRouteReconstruction"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !ScopedWorld.Start())
	{
		AddError(TEXT("Could not build the P27.26 reconstruction fixture."));
		return false;
	}
	const FGuid RunId = Handoff.GetDeploymentEvidence()
		.GetResourceEvidence().GetPlan().GetActiveRunId();
	FGuid OriginalRouteId;
	FGuid OriginalPublishId;
	TSet<AActor*> OriginalActors;
	FName DeploymentTag;
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Original;
		if (!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
				RunId, Handoff, ACharacter::StaticClass(), Original))
		{
			AddError(TEXT("Could not open the original P27.26 route."));
			return false;
		}
		const auto Published = Original.TryPublish(RunId, ScopedWorld.World);
		if (!Published.IsSuccess())
		{
			AddError(TEXT("Could not seed the P27.26 reconstruction World."));
			return false;
		}
		OriginalRouteId = Original.GetRouteId();
		OriginalPublishId = Published.CommandId;
		DeploymentTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Original.GetHost().GetBinding().GetDeploymentId());
		OriginalActors = CollectPublicationActors(
			ScopedWorld.World, DeploymentTag);
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Reconstructed;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
			RunId, Handoff, ACharacter::StaticClass(), Reconstructed))
	{
		AddError(TEXT("Could not reconstruct the P27.26 route."));
		return false;
	}
	const auto Adopted = Reconstructed.TryPublish(RunId, ScopedWorld.World);
	const TSet<AActor*> AdoptedActors =
		CollectPublicationActors(ScopedWorld.World, DeploymentTag);
	const auto Cancelled = Reconstructed.TryCancel(RunId, ScopedWorld.World);
	TestTrue(TEXT("Reconstruction derives the same route and adopts the batch"),
		Reconstructed.IsValid()
			&& Reconstructed.GetRouteId() == OriginalRouteId
			&& Adopted.Status == EWorldPublicationRunRouteStatus::Applied
			&& Adopted.IsValid() && Adopted.IsSuccess()
			&& Adopted.CommandId == OriginalPublishId
			&& Adopted.Command.Session.Status
				== EWorldPublicationSessionStatus::Published
			&& PublicationActorSetsMatch(OriginalActors, AdoptedActors)
			&& AdoptedActors.Num() == 2
			&& Cancelled.Status == EWorldPublicationRunRouteStatus::Applied
			&& Cancelled.IsValid() && Cancelled.IsSuccess()
			&& Cancelled.Event == EWorldPublicationRunEvent::Cancel
			&& Reconstructed.GetHost().GetSession().GetState()
				== EWorldPublicationSessionState::Cancelled
			&& CountPublicationActors(ScopedWorld.World, DeploymentTag) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterWorldPublicationRunRouteConflictTest,
	"Shanmen.0_0_10.Product.FormationScatterWorldPublicationRunRoute.RunWorldAndTerminalConflicts",
	PreparationFlags)

bool Fdemo_mapFormationScatterWorldPublicationRunRouteConflictTest::RunTest(
	const FString&)
{
	FPreparationFixture Fixture;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Handoff;
	FScopedScatterPublicationWorld FirstWorld;
	FScopedScatterPublicationWorld SecondWorld;
	if (!Fixture.Build(TEXT("WorldPublicationRunRouteConflict"))
		|| !BuildWorldHandoffEvidence(Fixture, Handoff)
		|| !FirstWorld.Start() || !SecondWorld.Start())
	{
		AddError(TEXT("Could not build the P27.26 conflict fixture."));
		return false;
	}
	const FGuid RunId = Handoff.GetDeploymentEvidence()
		.GetResourceEvidence().GetPlan().GetActiveRunId();
	const FGuid ForeignRunId(0xF8F26001, 0, 0, 1);
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute InvalidBinding;
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Route;
	TestTrue(TEXT("A route cannot bind the Handoff to a foreign Run"),
		!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
			ForeignRunId, Handoff, ACharacter::StaticClass(), InvalidBinding)
			&& InvalidBinding.IsEmpty()
			&& Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
				RunId, Handoff, ACharacter::StaticClass(), Route));
	if (!Route.IsValid())
	{
		AddError(TEXT("Could not open the valid P27.26 control route."));
		return false;
	}

	const auto Foreign = Route.TryPublish(ForeignRunId, FirstWorld.World);
	const auto Invalid = Route.TryRoute(
		RunId, FirstWorld.World, EWorldPublicationRunEvent::Invalid);
	const auto Published = Route.TryPublish(RunId, FirstWorld.World);
	const auto WrongWorld = Route.TryEnd(RunId, SecondWorld.World);
	const auto Ended = Route.TryEnd(RunId, FirstWorld.World);
	const auto TerminalConflict = Route.TryCancel(RunId, FirstWorld.World);
	TestTrue(TEXT("Foreign Run and invalid events fail before Host mutation"),
		Foreign.Status == EWorldPublicationRunRouteStatus::RunMismatch
			&& Foreign.IsValid() && !Foreign.IsSuccess()
			&& Invalid.Status == EWorldPublicationRunRouteStatus::EventInvalid
			&& Invalid.IsValid() && !Invalid.IsSuccess()
			&& Published.IsSuccess());
	TestTrue(TEXT("World rejection is retryable and terminal reason is frozen"),
		WrongWorld.Status == EWorldPublicationRunRouteStatus::HostRejected
			&& WrongWorld.IsValid() && !WrongWorld.IsSuccess()
			&& WrongWorld.Command.Status
				== EWorldPublicationCommandStatus::SessionRejected
			&& WrongWorld.Command.Session.Status
				== EWorldPublicationSessionStatus::WorldConflict
			&& !WrongWorld.Command.bHostStateCommitted
			&& Ended.Status == EWorldPublicationRunRouteStatus::Applied
			&& Ended.IsValid() && Ended.IsSuccess()
			&& TerminalConflict.Status
				== EWorldPublicationRunRouteStatus::HostRejected
			&& TerminalConflict.IsValid() && !TerminalConflict.IsSuccess()
			&& TerminalConflict.Command.Status
				== EWorldPublicationCommandStatus::OperationIdentityConflict
			&& Route.GetHost().GetRecordCount() == 2
			&& Route.GetHost().GetSession().GetState()
				== EWorldPublicationSessionState::Ended);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterPublicationLifecycleSlotTest,
	"Shanmen.0_0_10.Product.FormationRunLifecycle.ScatterPublication.SoleSlotPublishReplay",
	PreparationFlags)

bool Fdemo_mapFormationScatterPublicationLifecycleSlotTest::RunTest(
	const FString&)
{
	FScatterPublicationRunLifecycleFixture Fixture;
	if (!Fixture.Start(*this, TEXT("ScatterPublicationLifecycleSlot")))
	{
		return false;
	}
	const FGuid RunId = Fixture.GetRunId();
	TestFalse(TEXT("Publish is fenced before the sole route is bound"),
		Fixture.Lifecycle.TryPublishScatterPublication(
			Fixture.ScopedWorld.World).IsSuccess());
	TestTrue(TEXT("Committed Handoff opens the sole lifecycle route"),
		Fixture.Lifecycle.TryOpenScatterPublicationRoute(
			Fixture.Handoff,
			ACharacter::StaticClass(),
			Fixture.Diagnostic));
	const FGuid RouteId = Fixture.Lifecycle.GetScatterPublicationRoute()
		? Fixture.Lifecycle.GetScatterPublicationRoute()->GetRouteId()
		: FGuid();
	TestTrue(TEXT("Exact open is idempotent without replacing the owner"),
		Fixture.Lifecycle.TryOpenScatterPublicationRoute(
			Fixture.Handoff,
			ACharacter::StaticClass(),
			Fixture.Diagnostic)
			&& Fixture.Lifecycle.GetScatterPublicationRoute()
			&& Fixture.Lifecycle.GetScatterPublicationRoute()->GetRouteId()
				== RouteId);
	TestFalse(TEXT("The sole route slot rejects Actor-class drift"),
		Fixture.Lifecycle.TryOpenScatterPublicationRoute(
			Fixture.Handoff,
			APawn::StaticClass(),
			Fixture.Diagnostic));

	const auto Published = Fixture.Lifecycle.TryPublishScatterPublication(
		Fixture.ScopedWorld.World);
	const auto Replayed = Fixture.Lifecycle.TryPublishScatterPublication(
		nullptr);
	TestTrue(TEXT("Explicit lifecycle Publish owns one replayable batch"),
		Published.Status == EWorldPublicationRunRouteStatus::Applied
			&& Published.IsSuccess()
			&& Replayed.Status
				== EWorldPublicationRunRouteStatus::Replayed
			&& Replayed.IsSuccess()
			&& Published.RouteId == RouteId
			&& Published.CommandId == Replayed.CommandId
			&& Fixture.Lifecycle.IsValid()
			&& Fixture.Lifecycle.HasScatterPublicationRoute()
			&& CountPublicationActors(
				Fixture.ScopedWorld.World,
				Fixture.GetDeploymentTag()) == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterPublicationLifecycleTeardownFenceTest,
	"Shanmen.0_0_10.Product.FormationRunLifecycle.ScatterPublication.WorldFailureFencesProductTeardown",
	PreparationFlags)

bool Fdemo_mapFormationScatterPublicationLifecycleTeardownFenceTest::RunTest(
	const FString&)
{
	FScatterPublicationRunLifecycleFixture Fixture;
	FScopedScatterPublicationWorld WrongWorld;
	if (!Fixture.Start(*this, TEXT("ScatterPublicationLifecycleFence"))
		|| !WrongWorld.Start()
		|| !Fixture.Lifecycle.TryOpenScatterPublicationRoute(
			Fixture.Handoff,
			ACharacter::StaticClass(),
			Fixture.Diagnostic)
		|| !Fixture.Lifecycle.TryPublishScatterPublication(
			Fixture.ScopedWorld.World).IsSuccess())
	{
		AddError(TEXT("Could not publish the P27.27 fence fixture."));
		return false;
	}

	const auto Rejected = Fixture.Lifecycle.TryTeardownProduct(
		*Fixture.ItemAuthority,
		WrongWorld.World,
		Fixture.CombatRun);
	TestTrue(TEXT("Wrong World fails before product teardown starts"),
		Rejected.Status
			== Edemo_mapShanmenFormationRunLifecycleEndStatus::
				ScatterPublicationTeardownRejected
			&& Rejected.bHadScatterPublicationRoute
			&& !Rejected.bReusedScatterPublicationTeardown
			&& !Fixture.Lifecycle.
				HasScatterPublicationTeardownCheckpoint()
			&& !Fixture.Lifecycle.HasProductTeardownCheckpoint()
			&& Fixture.Lifecycle.GetController().IsActive()
			&& Fixture.CombatRun.IsActive()
			&& CountPublicationActors(
				Fixture.ScopedWorld.World,
				Fixture.GetDeploymentTag()) == 2);

	const auto Retried = Fixture.Lifecycle.TryTeardownProduct(
		*Fixture.ItemAuthority,
		Fixture.ScopedWorld.World,
		Fixture.CombatRun);
	TestTrue(TEXT("Exact retry checkpoints scatter before product teardown"),
		Retried.IsProductTeardownComplete()
			&& Retried.bHadScatterPublicationRoute
			&& !Retried.bReusedScatterPublicationTeardown
			&& !Retried.bReusedProductTeardown
			&& Retried.ScatterPublicationTeardown.Event
				== EWorldPublicationRunEvent::End
			&& Fixture.Lifecycle.
				HasScatterPublicationTeardownCheckpoint()
			&& Fixture.Lifecycle.HasProductTeardownCheckpoint()
			&& Fixture.Lifecycle.GetController().IsEmpty()
			&& Fixture.CombatRun.IsActive()
			&& CountPublicationActors(
				Fixture.ScopedWorld.World,
				Fixture.GetDeploymentTag()) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterPublicationLifecycleReleaseRecoveryTest,
	"Shanmen.0_0_10.Product.FormationRunLifecycle.ScatterPublication.CheckpointedCoordinatorRecovery",
	PreparationFlags)

bool Fdemo_mapFormationScatterPublicationLifecycleReleaseRecoveryTest::
RunTest(const FString&)
{
	FScatterPublicationRunLifecycleFixture Fixture;
	if (!Fixture.Start(*this, TEXT("ScatterPublicationLifecycleRecovery"))
		|| !Fixture.Lifecycle.TryOpenScatterPublicationRoute(
			Fixture.Handoff,
			ACharacter::StaticClass(),
			Fixture.Diagnostic)
		|| !Fixture.Lifecycle.TryPublishScatterPublication(
			Fixture.ScopedWorld.World).IsSuccess())
	{
		AddError(TEXT("Could not publish the P27.27 recovery fixture."));
		return false;
	}
	const FGuid RunId = Fixture.GetRunId();
	const FGuid ExpectedPlayerId = Fixture.CombatRun.GetPlayerEntityId();
	const FGuid WrongPlayerId(0xF8F27001, 0, 0, 1);
	if (!Fixture.PlayerHealth->TryEndCombatEntityBinding(ExpectedPlayerId)
		|| !Fixture.PlayerHealth->TryBindCombatEntity(WrongPlayerId))
	{
		AddError(TEXT("Could not inject the P27.27 Coordinator rejection."));
		return false;
	}

	const auto Rejected = Fixture.Lifecycle.TryEndRun(
		*Fixture.ItemAuthority,
		Fixture.ScopedWorld.World,
		Fixture.CombatRun);
	const auto* ScatterCheckpoint = Fixture.Lifecycle.
		GetScatterPublicationTeardownCheckpoint();
	TestTrue(TEXT("Coordinator rejection retains both ordered checkpoints"),
		Rejected.Status
			== Edemo_mapShanmenFormationRunLifecycleEndStatus::
				CoordinatorEndRejected
			&& !Rejected.bReusedScatterPublicationTeardown
			&& !Rejected.bReusedProductTeardown
			&& ScatterCheckpoint && ScatterCheckpoint->IsSuccess()
			&& ScatterCheckpoint->CommandId
				== Rejected.ScatterPublicationTeardown.CommandId
			&& Fixture.Lifecycle.HasProductTeardownCheckpoint()
			&& Fixture.Lifecycle.IsActive()
			&& Fixture.Lifecycle.IsValid()
			&& Fixture.CombatRun.IsActive());
	TestFalse(TEXT("Checkpointed teardown fences late publication"),
		Fixture.Lifecycle.TryPublishScatterPublication(nullptr).IsSuccess());

	if (!Fixture.PlayerHealth->TryEndCombatEntityBinding(WrongPlayerId)
		|| !Fixture.PlayerHealth->TryBindCombatEntity(ExpectedPlayerId))
	{
		AddError(TEXT("Could not repair the P27.27 Coordinator identity."));
		return false;
	}
	const auto Retried = Fixture.Lifecycle.TryEndRun(
		*Fixture.ItemAuthority,
		nullptr,
		Fixture.CombatRun);
	TestTrue(TEXT("Release retry uses both proofs without World re-entry"),
		Retried.IsEnded()
			&& Retried.RunId == RunId
			&& Retried.bReusedScatterPublicationTeardown
			&& Retried.bReusedProductTeardown
			&& Retried.ScatterPublicationTeardown.CommandId
				== Rejected.ScatterPublicationTeardown.CommandId
			&& Fixture.Lifecycle.IsEmpty()
			&& Fixture.Lifecycle.IsValid()
			&& !Fixture.CombatRun.IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterPublicationLifecycleTakeoverTest,
	"Shanmen.0_0_10.Product.FormationRunLifecycle.ScatterPublication.TakeoverMovesSoleOwner",
	PreparationFlags)

bool Fdemo_mapFormationScatterPublicationLifecycleTakeoverTest::RunTest(
	const FString&)
{
	FScatterPublicationRunLifecycleFixture Fixture;
	if (!Fixture.Start(*this, TEXT("ScatterPublicationLifecycleTakeover"))
		|| !Fixture.Lifecycle.TryOpenScatterPublicationRoute(
			Fixture.Handoff,
			ACharacter::StaticClass(),
			Fixture.Diagnostic))
	{
		AddError(TEXT("Could not open the P27.27 takeover fixture."));
		return false;
	}
	const auto Published = Fixture.Lifecycle.TryPublishScatterPublication(
		Fixture.ScopedWorld.World);
	const FGuid RouteId = Published.RouteId;
	Fdemo_mapShanmenFormationRunLifecycle Active;
	if (!Published.IsSuccess()
		|| !Fdemo_mapShanmenFormationRunLifecycle::TryTakeover(
			Fixture.Lifecycle, Active))
	{
		AddError(TEXT("Could not transfer the P27.27 lifecycle owner."));
		return false;
	}

	const auto OldOwnerPublish =
		Fixture.Lifecycle.TryPublishScatterPublication(nullptr);
	const auto NewOwnerReplay = Active.TryPublishScatterPublication(nullptr);
	const auto Ended = Active.TryEndRun(
		*Fixture.ItemAuthority,
		Fixture.ScopedWorld.World,
		Fixture.CombatRun);
	TestTrue(TEXT("Only the moved lifecycle can replay and close the Run"),
		Fixture.Lifecycle.IsEmpty()
			&& Fixture.Lifecycle.IsValid()
			&& !OldOwnerPublish.IsSuccess()
			&& NewOwnerReplay.Status
				== EWorldPublicationRunRouteStatus::Replayed
			&& NewOwnerReplay.IsSuccess()
			&& NewOwnerReplay.RouteId == RouteId
			&& Ended.IsEnded()
			&& Ended.ScatterPublicationTeardown.RouteId == RouteId
			&& Active.IsEmpty() && Active.IsValid()
			&& !Fixture.CombatRun.IsActive()
			&& CountPublicationActors(
				Fixture.ScopedWorld.World,
				Fixture.GetDeploymentTag()) == 0);
	return true;
}

#endif
