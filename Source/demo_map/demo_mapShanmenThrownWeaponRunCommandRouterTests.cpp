#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponRunCommandRouter.h"
#include "demo_mapShanmenThrownWeaponTerminalFeedbackPresentation.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	const FGuid RouterOwnerId(0xD3740001, 0, 0, 1);
	const FGuid RouterScopeId(0xD3740002, 0, 0, 1);
	const FGuid RouterContainerId(0xD3740003, 0, 0, 1);
	const FGuid RouterItemId(0xD3740004, 0, 0, 1);
	const FGuid RouterMigrationId(0xD3740005, 0, 0, 1);
	const FGuid RouterReserveRequestId(0xD3740006, 0, 0, 1);
	const FGuid RouterStartRequestId(0xD3740007, 0, 0, 1);
	const FName RouterItemDefinitionId(TEXT("Item.Test.ThrownDart.P7.4"));

	FString NewRouterRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P7.4.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp RouterContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P7.4");
		Content.Digest = TEXT("P7.4.ThrownWeaponRunCommandRouter.v1");
		return Content;
	}

	FShanmenOperationContext RouterContext(const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = RouterScopeId;
		Context.OwnerId = RouterOwnerId;
		Context.RequestId = RequestId;
		Context.Content = RouterContent();
		return Context;
	}

	FShanmenItemAuthoritySnapshot RouterCandidate()
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Snapshot.Content = RouterContent();

		FShanmenItemDefinition Definition;
		Definition.DefinitionId = RouterItemDefinitionId;
		Definition.MaxStack = 16;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponThrown());
		Snapshot.Definitions.Add(Definition);

		FShanmenItemContainer Container;
		Container.ContainerId = RouterContainerId;
		Container.RunId = RouterScopeId;
		Container.OwnerId = RouterOwnerId;
		Container.ContainerType = TEXT("Container.Test.P7.4.RunInventory");
		Container.Slots.Add(RouterItemId);
		Snapshot.Containers.Add(Container);

		FShanmenItemInstance Item;
		Item.ItemInstanceId = RouterItemId;
		Item.DefinitionId = RouterItemDefinitionId;
		Item.RunId = RouterScopeId;
		Item.OwnerId = RouterOwnerId;
		Item.ParentContainerId = RouterContainerId;
		Item.SlotIndex = 0;
		Item.Quantity = 3;
		Snapshot.Items.Add(Item);
		return Snapshot;
	}

	FShanmenItemMigrationEvidence RouterEvidence()
	{
		FShanmenItemMigrationEvidence Evidence;
		Evidence.MigrationId = RouterMigrationId;
		Evidence.OwnerId = RouterOwnerId;
		Evidence.SourceProfileSchema = 7;
		Evidence.SourceSaveGeneration = 74;
		Evidence.SourceCodeBPersistentRevision = 7;
		Evidence.SourceCodeBRepositoryRevision = 4;
		Evidence.DefinitionCount = 1;
		Evidence.ContainerCount = 1;
		Evidence.ItemCount = 1;
		Evidence.SourceFingerprint = TEXT("P7.4.Router.SourceFixture.v1");
		Evidence.CandidateDigest = TEXT("P7.4.Router.CandidateFixture.v1");
		return Evidence;
	}

	FShanmenThrownWeaponDefinition MakeRouterDefinition(
		FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.4.Product");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.4.Product");
		Capture.BaseDamage = 8.0f;
		Capture.TechniquePowerCoefficient = 0.25f;
		Capture.LaunchSpeed = ActionDefinitionId
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
			? 1200.0f : 850.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenThrownWeaponDefinition Definition;
		check(FShanmenThrownWeaponDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenThrownWeaponOffenseSnapshot MakeRouterOffense()
	{
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			24.0f, Offense));
		return Offense;
	}

	FShanmenThrownWeaponArcPlan MakeRouterArcPlan(
		const FShanmenCombatActionSnapshot& Action,
		const FVector& Origin,
		const FVector& Target)
	{
		FShanmenThrownWeaponArcRequestCapture Capture;
		Capture.Action = Action;
		Capture.TechniqueTier =
			EShanmenThrownWeaponTechniqueTier::Intermediate;
		Capture.Origin = Origin;
		Capture.Target = Target;
		Capture.GravityMagnitude = 980.0;
		Capture.ApexClearance = 150.0;
		Capture.MaximumLaunchSpeed = 1200.0;
		Capture.MaximumFlightTime = 5.0;
		const FShanmenThrownWeaponArcPlanResult Result =
			FShanmenThrownWeaponArcPlanner::Plan(Capture);
		check(Result.IsPlanned());
		return Result.Plan;
	}

	struct FRouterFixture
	{
		FString Root;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		UWorld* World = nullptr;
		APawn* Source = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenRunCorrelation Correlation;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewRouterRoot(Label);
			const FShanmenItemStorageContext Storage =
				FShanmenItemStorageContext::ForRoot(Root, RouterOwnerId);
			{
				FShanmenItemAuthorityService Bootstrap;
				const FShanmenItemAuthorityStartResult StartedAuthority =
					Bootstrap.StartFromAuthorizedMigration(
						Storage,
						FShanmenItemMigrationAuthorization::Explicit(
							RouterMigrationId),
						RouterCandidate(),
						RouterEvidence());
				FShanmenItemReserveRequest Reserve;
				Reserve.Context = RouterContext(RouterReserveRequestId);
				Reserve.ItemInstanceId = RouterItemId;
				Reserve.ResourceKind = EShanmenItemResourceKind::Quantity;
				Reserve.Amount = 3;
				Reserve.ExpectedItemRevision = 0;
				Reserve.PurposeId = TEXT("Prepare.P7.4.ThrownWeapon");
				const FShanmenItemDurableCommandResult Reserved =
					StartedAuthority.IsReady()
					? Bootstrap.ReserveDurable(Reserve)
					: FShanmenItemDurableCommandResult();
				FShanmenItemRunStartRequest StartRun;
				StartRun.Context = RouterContext(RouterStartRequestId);
				if (Reserved.IsCommandSuccess())
				{
					StartRun.ReservationIds.Add(
						Reserved.Receipt.ReservationId);
				}
				const FShanmenItemDurableCommandResult RunStarted =
					StartRun.ReservationIds.IsEmpty()
					? FShanmenItemDurableCommandResult()
					: Bootstrap.StartPreparedRunDurable(StartRun);
				if (!RunStarted.IsCommandSuccess())
				{
					Test.AddError(TEXT("Could not publish the P7.4 active Run."));
					return false;
				}

				Correlation.CorrelationId =
					FGuid(0xD3740010, 0, 0, 1);
				Correlation.OwnerId = RouterOwnerId;
				Correlation.ScopeId = RouterScopeId;
				Correlation.ActiveRunId =
					RunStarted.Receipt.ReservationId;
				Correlation.PreparedRequestId =
					Reserve.Context.RequestId;
				Correlation.PreparedReceiptId = Reserved.Receipt.ReceiptId;
				Correlation.LifecycleRequestId =
					StartRun.Context.RequestId;
				Correlation.LifecycleReceiptId =
					RunStarted.Receipt.ReceiptId;
				Correlation.PreparedAuthorityRevision =
					Reserved.Receipt.AuthorityRevision;
				Correlation.LifecycleAuthorityRevision =
					RunStarted.Receipt.AuthorityRevision;
				Correlation.OrderedPreparedItemInstanceIds.Add(RouterItemId);
				Correlation.OrderedRunInventoryItemInstanceIds.Add(
					RouterItemId);
				Correlation.HotbarItemInstanceIds.SetNum(
					Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
				Correlation.HotbarItemInstanceIds[0] = RouterItemId;
				if (!Correlation.IsValid())
				{
					Test.AddError(TEXT("P7.4 Run correlation is invalid."));
					return false;
				}
			}

			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P7.4 fixture."));
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
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			const Fdemo_mapShanmenItemAuthorityBindResult Bound = Authority
				? Authority->BindExisting(
					Fdemo_mapProfileStorageContext::ForRoot(Root),
					RouterOwnerId)
				: Fdemo_mapShanmenItemAuthorityBindResult();
			if (!Authority || !Bound.IsReady())
			{
				Test.AddError(TEXT("Could not bind the P7.4 product authority."));
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
			Source = World->SpawnActor<APawn>();
			Health = Source
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Source, TEXT("P74PlayerHealth"))
				: nullptr;
			FString Diagnostic;
			if (!Source || !Health
				|| !Coordinator.TryBeginRun(
					Correlation.ActiveRunId,
					Source,
					Health,
					Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("Could not bind P7.4 combat Run: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		FShanmenCombatActionSnapshot MakeAction(
			uint64 Sequence,
			FName ActionDefinitionId =
				FShanmenThrownWeaponDefinition::StraightActionDefinitionId()) const
		{
			FShanmenCombatActionCapture Capture;
			Capture.RunId = Correlation.ActiveRunId;
			Capture.OwnerId = RouterOwnerId;
			Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
			Capture.SourceItemInstanceId = RouterItemId;
			Capture.ActionDefinitionId = ActionDefinitionId;
			Capture.Content = RouterContent();
			Capture.SourceTags.AddTag(
				FShanmenCombatNativeTags::SourcePlayer());
			Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
				Capture.RunId,
				Capture.SourceEntityId,
				Capture.ActionDefinitionId,
				Sequence);
			FShanmenCombatActionSnapshot Action;
			check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
			return Action;
		}

		bool MakeIntent(
			uint64 Sequence,
			const FVector& Direction,
			Fdemo_mapShanmenThrownWeaponRunCommandIntent& OutIntent,
			float MaximumDistance = 1200.0f) const
		{
			return Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCapture(
				Correlation,
				MakeAction(Sequence),
				MakeRouterDefinition(),
				MakeRouterOffense(),
				FVector(25.0, 40.0, 60.0),
				Direction,
				MaximumDistance,
				OutIntent);
		}

		bool MakeArcIntent(
			uint64 Sequence,
			const FVector& Target,
			Fdemo_mapShanmenThrownWeaponRunCommandIntent& OutIntent,
			const FVector& Origin = FVector(25.0, 40.0, 60.0)) const
		{
			const FShanmenCombatActionSnapshot Action = MakeAction(
				Sequence,
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId());
			return Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCaptureArc(
				Correlation,
				Action,
				MakeRouterDefinition(
					FShanmenThrownWeaponDefinition::ArcActionDefinitionId()),
				MakeRouterOffense(),
				MakeRouterArcPlan(Action, Origin, Target),
				OutIntent);
		}

		void Stop()
		{
			if (Coordinator.IsActive())
			{
				FString Diagnostic;
				Coordinator.TryEndRun(
					Correlation.ActiveRunId, Diagnostic);
			}
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				Source = nullptr;
				Health = nullptr;
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
				Authority = nullptr;
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
			}
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
				Root.Reset();
			}
		}

		~FRouterFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunCommandIntentTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunCommand.IntentContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunCommandIntentTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Intent")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Forward;
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Right;
	TestTrue(TEXT("Input direction is frozen as one canonical unit vector"),
		Fixture.MakeIntent(1, FVector(4.0, 0.0, 0.0), Forward)
			&& Forward.IsValid()
			&& Forward.GetAimDirection() == FVector::ForwardVector
			&& Forward.GetIntentId()
				== Forward.GetAction().GetActivationId());
	TestTrue(TEXT("The same activation with another payload is detectable"),
		Fixture.MakeIntent(1, FVector(0.0, 3.0, 0.0), Right)
			&& Forward.GetIntentId() == Right.GetIntentId()
			&& !Forward.Matches(Right));
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Invalid;
	TestFalse(TEXT("A non-positive product range cannot enter the Router"),
		Fixture.MakeIntent(2, FVector::ForwardVector, Invalid, 0.0f));

	Fdemo_mapShanmenThrownWeaponRunCommandIntent Arc;
	const FVector ArcTarget(525.0, 40.0, 60.0);
	TestTrue(TEXT("Arc capture freezes one explicit ballistic plan only"),
		Fixture.MakeArcIntent(3, ArcTarget, Arc)
			&& Arc.IsValid()
			&& Arc.GetTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc
			&& Arc.GetArcPlan().IsValid()
			&& Arc.GetArcPlan().GetRequest().GetTarget() == ArcTarget
			&& Arc.GetOrigin() == FVector::ZeroVector
			&& Arc.GetAimDirection() == FVector::ZeroVector
			&& Arc.GetMaximumDistance() == 0.0f);

	const FShanmenCombatActionSnapshot ArcAction = Fixture.MakeAction(
		4, FShanmenThrownWeaponDefinition::ArcActionDefinitionId());
	const FShanmenThrownWeaponArcPlan ArcPlan = MakeRouterArcPlan(
		ArcAction,
		FVector(25.0, 40.0, 60.0),
		ArcTarget);
	Fdemo_mapShanmenThrownWeaponRunCommandIntent WrongTrajectory;
	TestFalse(TEXT("Arc action cannot enter the straight command capture"),
		Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCapture(
			Fixture.Correlation,
			ArcAction,
			MakeRouterDefinition(
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId()),
			MakeRouterOffense(),
			FVector(25.0, 40.0, 60.0),
			FVector::ForwardVector,
			1200.0f,
			WrongTrajectory));
	TestFalse(TEXT("Arc plan cannot be rebound to another action snapshot"),
		Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCaptureArc(
			Fixture.Correlation,
			Fixture.MakeAction(
				5,
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId()),
			MakeRouterDefinition(
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId()),
			MakeRouterOffense(),
			ArcPlan,
			WrongTrajectory));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunCommandApplyTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunCommand.ApplyReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunCommandApplyTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("Apply")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Intent;
	if (!Fixture.MakeIntent(7, FVector(5.0, 0.0, 0.0), Intent))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Applied =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("One command prepares, commits, spawns, and adopts once"),
		Applied.IsAccepted()
			&& !Applied.IsReplay()
			&& Applied.Preparation.IsPrepared()
			&& Applied.HostStart.Launch.ItemCommit.IsFinalized()
			&& Applied.HostStart.Launch.ItemCommit.FinalizeRequest.bCommit
			&& Applied.HostStart.Launch.ItemCommit.FinalizeCommand.Receipt
				.ResourceBefore == 3
			&& Applied.HostStart.Launch.ItemCommit.FinalizeCommand.Receipt
				.ResourceAfter == 2
			&& Host.IsInFlight()
			&& Host.GetLifetimeKind()
				== Edemo_mapShanmenThrownWeaponHostLifetimeKind::RangeDistance
			&& Host.GetFlightTimeSeconds() == 0.0
			&& Host.GetProjectile()
			&& Router.IsValid()
			&& Router.NumProcessedIntents() == 1
			&& After.AuthorityRevision == Before.AuthorityRevision + 2);
	TestFalse(TEXT("Straight flight cannot enter the Arc expiry route"),
		Host.TryExpireFlightTime());

	Ademo_mapShanmenThrownWeaponProjectile* FirstProjectile =
		Host.GetProjectile();
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Replay =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Exact replay performs no item I/O and spawns no second Actor"),
		Replay.IsAccepted()
			&& Replay.IsReplay()
			&& Replay.LaunchId == Applied.LaunchId
			&& Host.GetProjectile() == FirstProjectile
			&& AfterReplay == After
			&& Router.NumProcessedIntents() == 1);

	Fdemo_mapShanmenThrownWeaponRunCommandIntent Conflict;
	Fixture.MakeIntent(7, FVector::RightVector, Conflict);
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Rejected =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Conflict);
	FShanmenItemAuthoritySnapshot AfterConflict;
	Fixture.Authority->TryCaptureSnapshot(AfterConflict);
	TestTrue(TEXT("ActivationId payload conflict fails without mutation"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::IntentIdConflict
			&& !Rejected.IsAccepted()
			&& Host.GetProjectile() == FirstProjectile
			&& AfterConflict == After);
	Host.TryInterrupt();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunCommandCancelTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunCommand.PreLaunchCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunCommandCancelTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("Cancel")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Intent;
	Fixture.MakeIntent(8, FVector::ForwardVector, Intent);
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Cancelled =
		Router.TryRoute(
			Host,
			Fixture.World,
			nullptr,
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Spawn rejection durably releases the prepared Quantity"),
		Cancelled.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			&& Cancelled.IsDurableTerminal()
			&& Cancelled.Cancellation.IsFinalized()
			&& !Cancelled.Cancellation.FinalizeRequest.bCommit
			&& Cancelled.Cancellation.FinalizeCommand.Receipt.ResourceBefore == 3
			&& Cancelled.Cancellation.FinalizeCommand.Receipt.ResourceAfter == 3
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& After.AuthorityRevision == Before.AuthorityRevision + 2
			&& Router.IsValid());

	const Fdemo_mapShanmenThrownWeaponRunCommandResult Replay =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("A cancelled action cannot be relaunched by exact replay"),
		Replay.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			&& Replay.IsReplay()
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& AfterReplay == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunCommandRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunCommand.CancellationRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunCommandRecoveryTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("Recovery")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Intent;
	Fixture.MakeIntent(9, FVector::ForwardVector, Intent);
	const Fdemo_mapShanmenThrownWeaponItemResult Preprepared =
		Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
			*Fixture.Authority,
			Fixture.Correlation,
			Intent.GetAction());
	FShanmenItemAuthoritySnapshot Pending;
	Fixture.Authority->TryCaptureSnapshot(Pending);
	if (!Preprepared.IsPrepared())
	{
		AddError(TEXT("Could not preprepare the P7.4 recovery action."));
		return false;
	}
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Interrupted =
		Router.TryRoute(
			Host,
			Fixture.World,
			nullptr,
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot AfterFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFailure);
	TestTrue(TEXT("Cancellation persistence failure freezes an explicit recovery record"),
		Interrupted.RequiresRecovery()
			&& Interrupted.Preparation.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::Replayed
			&& Interrupted.Cancellation.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::FinalizeRejected
			&& !Interrupted.HostStart.Launch.IsCommitted()
			&& Router.IsValid()
			&& Router.NumProcessedIntents() == 1
			&& AfterFailure == Pending);

	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Recovered =
		Router.TryRecoverCancellation(*Fixture.Authority, Intent);
	FShanmenItemAuthoritySnapshot AfterRecovery;
	Fixture.Authority->TryCaptureSnapshot(AfterRecovery);
	TestTrue(TEXT("Explicit recovery retries cancellation but never launch"),
		Recovered.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			&& Recovered.IsDurableTerminal()
			&& Recovered.Cancellation.IsFinalized()
			&& !Recovered.Cancellation.FinalizeRequest.bCommit
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& AfterRecovery.AuthorityRevision
				== Pending.AuthorityRevision + 1
			&& Router.IsValid());

	const Fdemo_mapShanmenThrownWeaponRunCommandResult Replay =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot Final;
	Fixture.Authority->TryCaptureSnapshot(Final);
	TestTrue(TEXT("Recovered terminal replay remains side-effect-free"),
		Replay.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			&& Replay.IsReplay()
			&& Final == AfterRecovery);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcRunCommandApplyTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunCommand.ArcApplyReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcRunCommandApplyTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("ArcApply")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Intent;
	const FVector Target(525.0, 40.0, 60.0);
	if (!Fixture.MakeArcIntent(10, Target, Intent))
	{
		AddError(TEXT("Could not capture the P20.4 Arc run command."));
		return false;
	}

	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Applied =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	Ademo_mapShanmenThrownWeaponProjectile* FirstProjectile =
		Host.GetProjectile();
	const double FlightTime = Intent.GetArcPlan().GetFlightTimeSeconds();
	TestTrue(TEXT("Arc command prepares, commits, spawns, and adopts once"),
		Applied.IsAccepted()
			&& !Applied.IsReplay()
			&& Applied.HostStart.Launch.IsCommitted()
			&& Applied.HostStart.Launch.ItemCommit.FinalizeCommand.Receipt
				.ResourceBefore == 3
			&& Applied.HostStart.Launch.ItemCommit.FinalizeCommand.Receipt
				.ResourceAfter == 2
			&& Host.IsInFlight()
			&& Host.GetLifetimeKind()
				== Edemo_mapShanmenThrownWeaponHostLifetimeKind::ArcFlightTime
			&& Host.GetMaximumDistance() == 0.0f
			&& Host.GetFlightTimeSeconds() == FlightTime
			&& FirstProjectile
			&& FMath::IsNearlyEqual(
				FirstProjectile->GetLifeSpan(),
				static_cast<float>(FlightTime),
				0.01f)
			&& Router.IsValid()
			&& Router.NumProcessedIntents() == 1
			&& After.AuthorityRevision == Before.AuthorityRevision + 2);
	TestFalse(TEXT("Arc command cannot enter the straight expiry route"),
		Host.TryExpireRange());

	const Fdemo_mapShanmenThrownWeaponRunCommandResult Replay =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Exact Arc replay performs no item I/O or Actor creation"),
		Replay.IsAccepted()
			&& Replay.IsReplay()
			&& Replay.LaunchId == Applied.LaunchId
			&& Host.GetProjectile() == FirstProjectile
			&& AfterReplay == After
			&& Router.NumProcessedIntents() == 1);

	Fdemo_mapShanmenThrownWeaponRunCommandIntent Conflict;
	Fixture.MakeArcIntent(10, FVector(625.0, 40.0, 60.0), Conflict);
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Rejected =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Conflict);
	FShanmenItemAuthoritySnapshot AfterConflict;
	Fixture.Authority->TryCaptureSnapshot(AfterConflict);
	TestTrue(TEXT("Arc ActivationId payload conflict fails without mutation"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::IntentIdConflict
			&& !Rejected.IsAccepted()
			&& Host.GetProjectile() == FirstProjectile
			&& AfterConflict == After);
	Host.TryInterrupt();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcRunCommandCancelTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunCommand.ArcPreLaunchCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcRunCommandCancelTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("ArcCancel")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponRunCommandIntent Intent;
	if (!Fixture.MakeArcIntent(11, FVector(525.0, 40.0, 60.0), Intent))
	{
		AddError(TEXT("Could not capture the P20.4 Arc cancellation command."));
		return false;
	}

	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Cancelled =
		Router.TryRoute(
			Host,
			Fixture.World,
			nullptr,
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Arc spawn rejection releases prepared Quantity durably"),
		Cancelled.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			&& Cancelled.IsDurableTerminal()
			&& Cancelled.Cancellation.IsFinalized()
			&& !Cancelled.Cancellation.FinalizeRequest.bCommit
			&& Cancelled.Cancellation.FinalizeCommand.Receipt.ResourceBefore == 3
			&& Cancelled.Cancellation.FinalizeCommand.Receipt.ResourceAfter == 3
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& After.AuthorityRevision == Before.AuthorityRevision + 2
			&& Router.IsValid());

	const Fdemo_mapShanmenThrownWeaponRunCommandResult Replay =
		Router.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Cancelled Arc replay never commits or launches"),
		Replay.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			&& Replay.IsReplay()
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& AfterReplay == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcRunHostLifecycleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunHost.ArcFlightTimeLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcRunHostLifecycleTest::RunTest(const FString&)
{
	FRouterFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("ArcRunHost")))
	{
		return false;
	}

	const FName ArcActionDefinitionId =
		FShanmenThrownWeaponDefinition::ArcActionDefinitionId();
	const FShanmenCombatActionSnapshot Action =
		Fixture.MakeAction(10, ArcActionDefinitionId);
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Transition;
	FShanmenThrownWeaponExecution Execution;
	if (!FShanmenActionOrchestrator::TryStart(
			Action, Runtime, Transition)
		|| !Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Transition)
		|| !FShanmenThrownWeaponExecution::TryCreate(
			Action,
			MakeRouterDefinition(ArcActionDefinitionId),
			MakeRouterOffense(),
			Execution))
	{
		AddError(TEXT("Could not create the P20.3 Arc execution."));
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponItemResult Preparation =
		Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
			*Fixture.Authority, Fixture.Correlation, Action);
	if (!Preparation.IsPrepared())
	{
		AddError(TEXT("Could not reserve the P20.3 Arc thrown item."));
		return false;
	}
	FShanmenItemAuthoritySnapshot BeforeWrongRoute;
	Fixture.Authority->TryCaptureSnapshot(BeforeWrongRoute);

	const FVector Origin(25.0, 40.0, 60.0);
	const FVector Target(525.0, 40.0, 60.0);
	const FShanmenThrownWeaponArcPlan ArcPlan =
		MakeRouterArcPlan(Action, Origin, Target);
	const Fdemo_mapShanmenThrownWeaponHostStartResult WrongRoute =
		Host.TrySpawnAndLaunchPrepared(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Correlation,
			Preparation,
			Runtime,
			Execution,
			Fixture.Coordinator,
			Fixture.Source,
			Origin,
			FVector::ForwardVector,
			1200.0f);
	FShanmenItemAuthoritySnapshot AfterWrongRoute;
	Fixture.Authority->TryCaptureSnapshot(AfterWrongRoute);
	TestTrue(TEXT("Arc actions cannot cross the straight Host route"),
		WrongRoute.Error
			== Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& BeforeWrongRoute == AfterWrongRoute);

	const Fdemo_mapShanmenThrownWeaponHostStartResult Started =
		Host.TrySpawnAndLaunchPreparedArc(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Correlation,
			Preparation,
			Runtime,
			Execution,
			Fixture.Coordinator,
			Fixture.Source,
			ArcPlan);
	FShanmenItemAuthoritySnapshot AfterArcStart;
	Fixture.Authority->TryCaptureSnapshot(AfterArcStart);
	Ademo_mapShanmenThrownWeaponProjectile* Projectile =
		Host.GetProjectile();
	const double FlightTime = ArcPlan.GetFlightTimeSeconds();
	TestTrue(TEXT("Arc Host commits once and retains the solver lifetime"),
		Started.IsStarted()
			&& Started.Launch.IsCommitted()
			&& Started.Launch.ItemCommit.FinalizeCommand.Receipt.ResourceBefore
				== 3
			&& Started.Launch.ItemCommit.FinalizeCommand.Receipt.ResourceAfter
				== 2
			&& AfterArcStart.AuthorityRevision
				== BeforeWrongRoute.AuthorityRevision + 1
			&& Host.IsValid()
			&& Host.IsInFlight()
			&& Host.GetLifetimeKind()
				== Edemo_mapShanmenThrownWeaponHostLifetimeKind::ArcFlightTime
			&& Host.GetMaximumDistance() == 0.0f
			&& Host.GetFlightTimeSeconds() == FlightTime
			&& FMath::IsNearlyEqual(
				Host.GetActorLifeSpanSeconds(),
				static_cast<float>(FlightTime))
			&& Projectile
			&& FMath::IsNearlyEqual(
				Projectile->GetLifeSpan(),
				static_cast<float>(FlightTime),
				0.01f));
	TestFalse(TEXT("Arc flight cannot enter the straight expiry route"),
		Host.TryExpireRange());

	if (!Projectile)
	{
		return false;
	}
	Projectile->OnRangeExpired().Broadcast(*Projectile);
	TestTrue(TEXT("Actor lifespan callback publishes one typed Arc terminal"),
		Host.IsValid()
			&& Host.IsTerminal()
			&& Host.GetTerminalReceipt().Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::FlightTimeExpired
			&& Host.GetExecution().GetState()
				== EShanmenThrownWeaponState::Spent
			&& Host.GetExecution().NumAcceptedImpacts() == 0
			&& Host.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent);
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation ExpiredFeedback;
	TestTrue(TEXT("Arc flight-time expiry projects a distinct miss message"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			Host.GetTerminalReceipt(), ExpiredFeedback)
			&& ExpiredFeedback.GetKind()
				== Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::
					FlightTimeExpired
			&& ExpiredFeedback.GetAppliedDamage() == 0.0f
			&& ExpiredFeedback.GetDisplayText() == TEXT("飞刀 · 落空"));
	TestFalse(TEXT("Terminal Arc expiry cannot execute twice"),
		Host.TryExpireFlightTime());
	return true;
}

#endif
