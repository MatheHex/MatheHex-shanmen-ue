#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponRunCommandRouter.h"

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

	FShanmenThrownWeaponDefinition MakeRouterDefinition()
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.4.Product");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.4.Product");
		Capture.BaseDamage = 8.0f;
		Capture.TechniquePowerCoefficient = 0.25f;
		Capture.LaunchSpeed = 850.0f;
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

		FShanmenCombatActionSnapshot MakeAction(uint64 Sequence) const
		{
			FShanmenCombatActionCapture Capture;
			Capture.RunId = Correlation.ActiveRunId;
			Capture.OwnerId = RouterOwnerId;
			Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
			Capture.SourceItemInstanceId = RouterItemId;
			Capture.ActionDefinitionId =
				FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
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
			&& Host.GetProjectile()
			&& Router.IsValid()
			&& Router.NumProcessedIntents() == 1
			&& After.AuthorityRevision == Before.AuthorityRevision + 2);

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

#endif
