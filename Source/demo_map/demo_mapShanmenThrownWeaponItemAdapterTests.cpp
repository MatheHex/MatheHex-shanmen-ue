#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponItemAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid OwnerId(0xD3710001, 0, 0, 1);
	const FGuid ScopeId(0xD3710002, 0, 0, 1);
	const FGuid SourceEntityId(0xD3710003, 0, 0, 1);
	const FGuid DartId(0xD3710004, 0, 0, 1);
	const FGuid ContainerId(0xD3710005, 0, 0, 1);
	const FGuid ReserveRequestId(0xD3710006, 0, 0, 1);
	const FGuid StartRequestId(0xD3710007, 0, 0, 1);
	const FName DartDefinitionId(TEXT("Item.Test.ThrownDart.P7.1"));

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P7.1");
		Content.Digest = TEXT("P7.1.ThrownWeaponItemTransaction.v1");
		return Content;
	}

	FShanmenOperationContext MakeContext(const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = RequestId;
		Context.Content = MakeContent();
		return Context;
	}

	FShanmenCombatActionSnapshot MakeAction(
		uint64 ActivationSequence = 7,
		const FGuid& ItemId = DartId,
		FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = FGuid(0xD3710008, 0, 0, 1);
		Capture.OwnerId = OwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.SourceItemInstanceId = ItemId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content = MakeContent();
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId, Capture.SourceEntityId,
			Capture.ActionDefinitionId, ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponDefinition MakeThrownDefinition(
		FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.1.Test");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.1.Test");
		Capture.BaseDamage = 10.0f;
		Capture.TechniquePowerCoefficient = 0.25f;
		Capture.LaunchSpeed = ActionDefinitionId
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
			? 1200.0f : 800.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenThrownWeaponDefinition Definition;
		check(FShanmenThrownWeaponDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenThrownWeaponLaunchReceipt MakeLaunch(
		const FShanmenCombatActionSnapshot& Action)
	{
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			20.0f, Offense));
		FShanmenThrownWeaponExecution Execution;
		check(FShanmenThrownWeaponExecution::TryCreate(
			Action, MakeThrownDefinition(), Offense, Execution));
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Phase;
		check(FShanmenActionOrchestrator::TryStart(
			Action, Runtime, Phase));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Phase));
		FShanmenThrownWeaponLaunchReceipt Launch;
		check(Execution.TryLaunchStraight(
			Runtime, FVector(10.0, 20.0, 30.0),
			FVector::ForwardVector, Launch));
		return Launch;
	}

	FShanmenThrownWeaponArcPlan MakeArcPlan(
		const FShanmenCombatActionSnapshot& Action)
	{
		FShanmenThrownWeaponArcRequestCapture Capture;
		Capture.Action = Action;
		Capture.TechniqueTier =
			EShanmenThrownWeaponTechniqueTier::Intermediate;
		Capture.Origin = FVector(10.0, 20.0, 30.0);
		Capture.Target = FVector(510.0, 20.0, 30.0);
		Capture.GravityMagnitude = 980.0;
		Capture.ApexClearance = 150.0;
		Capture.MaximumLaunchSpeed = 1200.0;
		Capture.MaximumFlightTime = 5.0;
		const FShanmenThrownWeaponArcPlanResult Result =
			FShanmenThrownWeaponArcPlanner::Plan(Capture);
		check(Result.IsPlanned());
		return Result.Plan;
	}

	FShanmenThrownWeaponLaunchReceipt MakeArcLaunch(
		const FShanmenCombatActionSnapshot& Action)
	{
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			20.0f, Offense));
		FShanmenThrownWeaponExecution Execution;
		check(FShanmenThrownWeaponExecution::TryCreate(
			Action,
			MakeThrownDefinition(
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId()),
			Offense,
			Execution));
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Phase;
		check(FShanmenActionOrchestrator::TryStart(
			Action, Runtime, Phase));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Phase));
		FShanmenThrownWeaponLaunchReceipt Launch;
		check(Execution.TryLaunchArc(
			Runtime, MakeArcPlan(Action), Launch));
		return Launch;
	}

	struct FThrownItemFixture
	{
		FShanmenItemRepository Repository;
		FShanmenItemAuthoritySnapshot Snapshot;
		Fdemo_mapShanmenRunCorrelation Correlation;
		FShanmenCombatActionSnapshot Action;

		bool Build(
			FName ActionDefinitionId =
				FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
		{
			FShanmenItemAuthoritySnapshot Initial;
			Initial.Content = MakeContent();
			FShanmenItemDefinition Definition;
			Definition.DefinitionId = DartDefinitionId;
			Definition.MaxStack = 16;
			Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityConsumeQuantity());
			Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::ItemWeaponThrown());
			Initial.Definitions.Add(Definition);

			FShanmenItemContainer Container;
			Container.ContainerId = ContainerId;
			Container.RunId = ScopeId;
			Container.OwnerId = OwnerId;
			Container.ContainerType = TEXT("Container.Test.P7.1.RunInventory");
			Container.Slots.Add(DartId);
			Initial.Containers.Add(Container);

			FShanmenItemInstance Item;
			Item.ItemInstanceId = DartId;
			Item.DefinitionId = DartDefinitionId;
			Item.RunId = ScopeId;
			Item.OwnerId = OwnerId;
			Item.ParentContainerId = ContainerId;
			Item.SlotIndex = 0;
			Item.Quantity = 4;
			Initial.Items.Add(Item);
			if (!Repository.TryLoadSnapshot(Initial))
			{
				return false;
			}

			FShanmenItemReserveRequest Reserve;
			Reserve.Context = MakeContext(ReserveRequestId);
			Reserve.ItemInstanceId = DartId;
			Reserve.ResourceKind = EShanmenItemResourceKind::Quantity;
			Reserve.Amount = 4;
			Reserve.ExpectedItemRevision = 0;
			Reserve.PurposeId = TEXT("Prepare.RunInventory.ThrownDart.P7.1");
			const FShanmenItemTransactionReceipt Reserved =
				Repository.Reserve(Reserve);
			FShanmenItemRunStartRequest Start;
			Start.Context = MakeContext(StartRequestId);
			Start.ReservationIds.Add(Reserved.ReservationId);
			const FShanmenItemTransactionReceipt Started =
				Reserved.IsSuccess()
					? Repository.StartPreparedRun(Start)
					: FShanmenItemTransactionReceipt();
			if (!Started.IsSuccess())
			{
				return false;
			}
			Snapshot = Repository.CaptureSnapshot();

			Correlation.CorrelationId = FGuid(0xD3710010, 0, 0, 1);
			Correlation.OwnerId = OwnerId;
			Correlation.ScopeId = ScopeId;
			Correlation.ActiveRunId = Started.ReservationId;
			Correlation.PreparedRequestId = ReserveRequestId;
			Correlation.PreparedReceiptId = Reserved.ReceiptId;
			Correlation.LifecycleRequestId = Started.RequestId;
			Correlation.LifecycleReceiptId = Started.ReceiptId;
			Correlation.PreparedAuthorityRevision = Reserved.AuthorityRevision;
			Correlation.LifecycleAuthorityRevision = Started.AuthorityRevision;
			Correlation.OrderedPreparedItemInstanceIds.Add(DartId);
			Correlation.OrderedRunInventoryItemInstanceIds.Add(DartId);
			Correlation.HotbarItemInstanceIds.SetNum(
				Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
			Correlation.HotbarItemInstanceIds[0] = DartId;

			Action = MakeAction(7, DartId, ActionDefinitionId);
			FShanmenCombatActionCapture Corrected;
			Corrected.RunId = Correlation.ActiveRunId;
			Corrected.OwnerId = OwnerId;
			Corrected.SourceEntityId = SourceEntityId;
			Corrected.SourceItemInstanceId = DartId;
			Corrected.ActionDefinitionId = ActionDefinitionId;
			Corrected.Content = MakeContent();
			Corrected.SourceTags.AddTag(
				FShanmenCombatNativeTags::SourcePlayer());
			Corrected.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					Corrected.RunId, Corrected.SourceEntityId,
					Corrected.ActionDefinitionId, 7);
			return Correlation.IsValid()
				&& FShanmenCombatActionSnapshot::TryCapture(
					Corrected, Action);
		}

		Fdemo_mapShanmenThrownWeaponItemResult PrepareInRepository()
		{
			Fdemo_mapShanmenThrownWeaponItemResult Result =
				Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
					Snapshot, Correlation, Action);
			if (Result.Status
				!= Edemo_mapShanmenThrownWeaponItemStatus::RequestReady)
			{
				return Result;
			}
			Result.PrepareCommand.Status =
				EShanmenItemDurableCommandStatus::Persisted;
			Result.PrepareCommand.Receipt =
				Repository.PreparePreparedRunQuantityIntent(
					Result.PrepareRequest);
			Result.Status = Result.PrepareCommand.IsCommandSuccess()
				? Edemo_mapShanmenThrownWeaponItemStatus::Prepared
				: Edemo_mapShanmenThrownWeaponItemStatus::PrepareRejected;
			Snapshot = Repository.CaptureSnapshot();
			return Result;
		}
	};

	void TestRejected(
		FAutomationTestBase& Test,
		const TCHAR* What,
		const Fdemo_mapShanmenThrownWeaponItemResult& Result,
		Edemo_mapShanmenThrownWeaponItemStatus Expected)
	{
		Test.TestTrue(What,
			Result.Status == Expected && !Result.HasPrepareRequest());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponItemEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponItemAdapter.EvidenceAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponItemEvidenceTest::RunTest(const FString&)
{
	FThrownItemFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P7.1 thrown-item fixture."));
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponItemResult First =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Action);
	const Fdemo_mapShanmenThrownWeaponItemResult Second =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Action);
	TestTrue(TEXT("Exact thrown Run item produces one canonical prepare request"),
		First.Status == Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
			&& First.HasPrepareRequest()
			&& First.PrepareRequest.IntentId
				== Fixture.Action.GetActivationId()
			&& First.PrepareRequest.ItemInstanceId == DartId
			&& First.PrepareRequest.ExpectedQuantityBefore == 4
			&& First.PrepareRequest.Amount == 1);
	TestTrue(TEXT("Identical authority evidence produces identical identity"),
		Second.HasPrepareRequest()
			&& Second.PrepareRequest.Context.RequestId
				== First.PrepareRequest.Context.RequestId
			&& Second.PrepareRequest.IntentId
				== First.PrepareRequest.IntentId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponItemCommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponItemAdapter.LaunchCommitAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponItemCommitTest::RunTest(const FString&)
{
	FThrownItemFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P7.1 commit fixture."));
		return false;
	}
	Fdemo_mapShanmenThrownWeaponItemResult Prepared =
		Fixture.PrepareInRepository();
	const FShanmenThrownWeaponLaunchReceipt Launch =
		MakeLaunch(Fixture.Action);
	const Fdemo_mapShanmenThrownWeaponItemResult Commit =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCommitRequest(
			Fixture.Correlation, Prepared, Launch);
	TestTrue(TEXT("Only the matching immutable launch opens the commit point"),
		Prepared.IsPrepared() && Launch.IsValid()
			&& Commit.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
			&& Commit.FinalizeRequest.IsValid()
			&& Commit.FinalizeRequest.bCommit);
	const FShanmenItemTransactionReceipt Committed =
		Fixture.Repository.FinalizePreparedRunQuantityIntent(
			Commit.FinalizeRequest);
	Fixture.Snapshot = Fixture.Repository.CaptureSnapshot();
	const Fdemo_mapShanmenThrownWeaponItemResult Replay =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Action);
	TestTrue(TEXT("Committed action reconstructs the exact original replay request"),
		Committed.IsSuccess()
			&& Committed.ResourceBefore == 4
			&& Committed.ResourceAfter == 3
			&& Replay.HasPrepareRequest()
			&& Replay.PrepareRequest.Context.RequestId
				== Prepared.PrepareRequest.Context.RequestId
			&& Replay.PrepareRequest.ExpectedQuantityBefore == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcItemCommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponItemAdapter.ArcLaunchCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcItemCommitTest::RunTest(const FString&)
{
	FThrownItemFixture Fixture;
	if (!Fixture.Build(
		FShanmenThrownWeaponDefinition::ArcActionDefinitionId()))
	{
		AddError(TEXT("Could not build the P20.2 arc-item fixture."));
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponItemResult Request =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Action);
	TestTrue(TEXT("Arc actions receive their own auditable Quantity purpose"),
		Request.Status
			== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
			&& Request.PrepareRequest.PurposeId
				== FName(TEXT("Shanmen.ThrownWeapon.ArcLaunch.r1")));

	const Fdemo_mapShanmenThrownWeaponItemResult Prepared =
		Fixture.PrepareInRepository();
	const FShanmenThrownWeaponLaunchReceipt Launch =
		MakeArcLaunch(Fixture.Action);
	const Fdemo_mapShanmenThrownWeaponItemResult Commit =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCommitRequest(
			Fixture.Correlation, Prepared, Launch);
	const FShanmenItemTransactionReceipt Committed =
		Commit.FinalizeRequest.IsValid()
			? Fixture.Repository.FinalizePreparedRunQuantityIntent(
				Commit.FinalizeRequest)
			: FShanmenItemTransactionReceipt();
	TestTrue(TEXT("Arc launch consumes the same exact durable item intent"),
		Prepared.IsPrepared()
			&& Launch.IsValid()
			&& Launch.GetTrajectoryKind()
				== EShanmenThrownWeaponTrajectoryKind::BallisticArc
			&& Commit.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
			&& Commit.FinalizeRequest.IntentId
				== Fixture.Action.GetActivationId()
			&& Committed.IsSuccess()
			&& Committed.ResourceBefore == 4
			&& Committed.ResourceAfter == 3
			&& Fixture.Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponItemCancelTest,
	"Shanmen.0_0_10.Product.ThrownWeaponItemAdapter.PreLaunchCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponItemCancelTest::RunTest(const FString&)
{
	FThrownItemFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P7.1 cancel fixture."));
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponItemResult Prepared =
		Fixture.PrepareInRepository();
	const Fdemo_mapShanmenThrownWeaponItemResult Cancel =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCancelRequest(
			Fixture.Correlation, Prepared);
	const FShanmenItemTransactionReceipt Cancelled =
		Cancel.FinalizeRequest.IsValid()
			? Fixture.Repository.FinalizePreparedRunQuantityIntent(
				Cancel.FinalizeRequest)
			: FShanmenItemTransactionReceipt();
	TestTrue(TEXT("Pre-launch cancellation preserves all active-Run Quantity"),
		Prepared.IsPrepared()
			&& Cancel.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
			&& !Cancel.FinalizeRequest.bCommit
			&& Cancelled.IsSuccess()
			&& Cancelled.Phase == EShanmenItemTransactionPhase::Cancelled
			&& Cancelled.ResourceBefore == 4
			&& Cancelled.ResourceAfter == 4
			&& Fixture.Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponItemFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponItemAdapter.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponItemFailClosedTest::RunTest(const FString&)
{
	FThrownItemFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P7.1 fail-closed fixture."));
		return false;
	}

	FShanmenItemAuthoritySnapshot Generic = Fixture.Snapshot;
	FShanmenItemDefinition* Definition =
		Generic.Definitions.FindByPredicate(
			[](const FShanmenItemDefinition& Candidate)
			{
				return Candidate.DefinitionId == DartDefinitionId;
			});
	check(Definition);
	Definition->ItemTags.RemoveTag(
		FShanmenItemNativeTags::ItemWeaponThrown());
	TestRejected(*this, TEXT("A generic consumable cannot borrow thrown semantics"),
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
			Generic, Fixture.Correlation, Fixture.Action),
		Edemo_mapShanmenThrownWeaponItemStatus::DefinitionNotThrownWeapon);

	Fdemo_mapShanmenThrownWeaponItemResult Prepared =
		Fixture.PrepareInRepository();
	FShanmenCombatActionCapture OtherCapture;
	OtherCapture.RunId = Fixture.Correlation.ActiveRunId;
	OtherCapture.OwnerId = OwnerId;
	OtherCapture.SourceEntityId = SourceEntityId;
	OtherCapture.SourceItemInstanceId = DartId;
	OtherCapture.ActionDefinitionId =
		FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
	OtherCapture.Content = MakeContent();
	OtherCapture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	OtherCapture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
		OtherCapture.RunId, OtherCapture.SourceEntityId,
		OtherCapture.ActionDefinitionId, 8);
	FShanmenCombatActionSnapshot OtherAction;
	check(FShanmenCombatActionSnapshot::TryCapture(
		OtherCapture, OtherAction));
	const Fdemo_mapShanmenThrownWeaponItemResult WrongLaunch =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCommitRequest(
			Fixture.Correlation, Prepared, MakeLaunch(OtherAction));
	TestTrue(TEXT("Another activation's launch cannot consume this intent"),
		WrongLaunch.Status
			== Edemo_mapShanmenThrownWeaponItemStatus::LaunchMismatch
			&& !WrongLaunch.FinalizeRequest.IsValid());

	FThrownItemFixture Insufficient;
	check(Insufficient.Build());
	TestRejected(*this, TEXT("Requested Quantity must exist in the active Run"),
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
			Insufficient.Snapshot, Insufficient.Correlation,
			Insufficient.Action, 5),
		Edemo_mapShanmenThrownWeaponItemStatus::QuantityUnavailable);

	if (!GEngine)
	{
		AddError(TEXT("GEngine is unavailable for the P7.1 facade fixture."));
		return false;
	}
	UGameInstance* GameInstance = NewObject<UGameInstance>(
		GEngine, NAME_None, RF_Transient);
	if (!GameInstance)
	{
		AddError(TEXT("Could not allocate the P7.1 product GameInstance."));
		return false;
	}
	GameInstance->AddToRoot();
	GameInstance->Init();
	Udemo_mapShanmenItemAuthoritySubsystem* Unbound =
		GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	TestTrue(TEXT("Product facade fails closed before authority binding"),
		Unbound
			&& Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
				*Unbound, Fixture.Correlation, Fixture.Action).Status
				== Edemo_mapShanmenThrownWeaponItemStatus::AuthorityNotReady);
	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	GameInstance->MarkAsGarbage();
	return true;
}

#endif
