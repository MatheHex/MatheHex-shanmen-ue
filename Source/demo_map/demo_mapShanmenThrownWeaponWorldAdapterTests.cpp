#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponWorldAdapter.h"
#include "demo_mapShanmenThrownWeaponRunHost.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	const FGuid WorldRunId(0xD3720001, 0, 0, 1);
	const FGuid WorldOwnerId(0xD3720002, 0, 0, 1);
	const FGuid WorldScopeId(0xD3720003, 0, 0, 1);
	const FGuid WorldItemId(0xD3720004, 0, 0, 1);
	const FName StraightLaunchPurpose(
		TEXT("Shanmen.ThrownWeapon.StraightLaunch.r1"));
	const FName ArcLaunchPurpose(TEXT("Shanmen.ThrownWeapon.ArcLaunch.r1"));

	const Fdemo_mapM01EnemyDefinition* FindMeleeDefinition()
	{
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype
				== Edemo_mapM01EnemyArchetype::StandardSkirmisher)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	Fdemo_mapEnemyEncounterIdentity MakeEncounterIdentity(
		const Fdemo_mapM01EnemyDefinition& Definition)
	{
		Fdemo_mapEnemyEncounterIdentity Identity;
		Identity.EncounterId = Definition.EncounterId;
		Identity.RouteId = Definition.RouteId;
		Identity.SpawnMarkerId = Definition.SpawnMarkerId;
		Identity.LootTableId = Definition.CorpseIdentity;
		Identity.SkillProfileId = Definition.SkillProfileId;
		return Identity;
	}

	struct FThrownWorldFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FThrownWorldFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P72PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P72PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindMeleeDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P72EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}
			Enemy->AddInstanceComponent(EnemyIdentity);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					WorldRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	struct FThrownSpawnWorldFixture
	{
		UWorld* World = nullptr;
		APawn* Source = nullptr;

		FThrownSpawnWorldFixture()
		{
			if (!GEngine)
			{
				return;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
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
		}

		~FThrownSpawnWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		bool IsValid() const { return World && Source; }
	};

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P7.2");
		Content.Digest = TEXT("P7.2.ThrownWeaponWorldDelivery.v1");
		return Content;
	}

	Fdemo_mapShanmenRunCorrelation MakeCorrelation()
	{
		Fdemo_mapShanmenRunCorrelation Correlation;
		Correlation.CorrelationId = FGuid(0xD3720010, 0, 0, 1);
		Correlation.OwnerId = WorldOwnerId;
		Correlation.ScopeId = WorldScopeId;
		Correlation.ActiveRunId = WorldRunId;
		Correlation.PreparedRequestId = FGuid(0xD3720011, 0, 0, 1);
		Correlation.PreparedReceiptId = FGuid(0xD3720012, 0, 0, 1);
		Correlation.LifecycleRequestId = FGuid(0xD3720013, 0, 0, 1);
		Correlation.LifecycleReceiptId = FGuid(0xD3720014, 0, 0, 1);
		Correlation.PreparedAuthorityRevision = 4;
		Correlation.LifecycleAuthorityRevision = 5;
		Correlation.OrderedPreparedItemInstanceIds.Add(WorldItemId);
		Correlation.OrderedRunInventoryItemInstanceIds.Add(WorldItemId);
		Correlation.HotbarItemInstanceIds.SetNum(
			Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
		Correlation.HotbarItemInstanceIds[0] = WorldItemId;
		check(Correlation.IsValid());
		return Correlation;
	}

	FShanmenCombatActionSnapshot MakeAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Coordinator.GetRunId();
		Capture.OwnerId = WorldOwnerId;
		Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
		Capture.SourceItemInstanceId = WorldItemId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content = MakeContent();
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			2);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponDefinition MakeDefinition(
		FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.2.Product");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.2.Product");
		// Keep this transient Actor fixture alive; death behavior belongs to the
		// coordinator suite and requires a registered World.
		Capture.BaseDamage = 0.5f;
		Capture.TechniquePowerCoefficient = 0.01f;
		Capture.LaunchSpeed = ActionDefinitionId
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
			? 1200.0f : 750.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenThrownWeaponDefinition Definition;
		check(FShanmenThrownWeaponDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	void StartAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenThrownWeaponExecution& OutExecution,
		FShanmenCombatActionSnapshot& OutAction,
		FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		OutAction = MakeAction(Coordinator, ActionDefinitionId);
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			OutAction, OutRuntime, Transition));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Transition));
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			20.0f, Offense));
		check(FShanmenThrownWeaponExecution::TryCreate(
			OutAction,
			MakeDefinition(ActionDefinitionId),
			Offense,
			OutExecution));
	}

	FShanmenThrownWeaponArcPlan MakeArcPlan(
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

	FShanmenItemTransactionReceipt MakePrepareReceipt(
		const FShanmenItemRunQuantityIntentRequest& Request,
		const FGuid& ReceiptId)
	{
		FShanmenItemTransactionReceipt Receipt;
		Receipt.bSuccess = true;
		Receipt.Operation =
			EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent;
		Receipt.Phase = EShanmenItemTransactionPhase::Reserved;
		Receipt.Error = EShanmenItemTransactionError::None;
		Receipt.ReceiptId = ReceiptId;
		Receipt.RequestId = Request.Context.RequestId;
		Receipt.ReservationId = Request.IntentId;
		Receipt.ItemInstanceId = Request.ItemInstanceId;
		Receipt.ResourceKind = EShanmenItemResourceKind::Quantity;
		Receipt.Amount = Request.Amount;
		Receipt.ResourceBefore = Request.ExpectedQuantityBefore;
		Receipt.ResourceAfter = Request.ExpectedQuantityBefore;
		Receipt.AvailableAfter =
			Request.ExpectedQuantityBefore - Request.Amount;
		Receipt.ItemRevision = 2;
		Receipt.AuthorityRevision = 6;
		Receipt.PurposeId = Request.PurposeId;
		Receipt.ReservationIds.Add(Request.ActiveRunId);
		check(Receipt.IsValid());
		return Receipt;
	}

	Fdemo_mapShanmenThrownWeaponItemResult MakePrepared(
		const FShanmenCombatActionSnapshot& Action)
	{
		Fdemo_mapShanmenThrownWeaponItemResult Prepared;
		Prepared.Status = Edemo_mapShanmenThrownWeaponItemStatus::Prepared;
		Prepared.Action = Action;
		Prepared.PrepareRequest.Context.RunId = WorldScopeId;
		Prepared.PrepareRequest.Context.OwnerId = WorldOwnerId;
		Prepared.PrepareRequest.Context.RequestId =
			FGuid(0xD3720020, 0, 0, 1);
		Prepared.PrepareRequest.Context.Content = MakeContent();
		Prepared.PrepareRequest.ActiveRunId = WorldRunId;
		Prepared.PrepareRequest.IntentId = Action.GetActivationId();
		Prepared.PrepareRequest.ItemInstanceId = WorldItemId;
		Prepared.PrepareRequest.Amount = 1;
		Prepared.PrepareRequest.ExpectedQuantityBefore = 3;
		Prepared.PrepareRequest.PurposeId =
			Action.GetActionDefinitionId()
				== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
			? ArcLaunchPurpose : StraightLaunchPurpose;
		Prepared.PrepareCommand.Status =
			EShanmenItemDurableCommandStatus::Persisted;
		Prepared.PrepareCommand.Receipt = MakePrepareReceipt(
			Prepared.PrepareRequest, FGuid(0xD3720021, 0, 0, 1));
		check(Prepared.IsPrepared());
		return Prepared;
	}

	Fdemo_mapShanmenThrownWeaponItemResult MakeCommitted(
		const Fdemo_mapShanmenThrownWeaponLaunchPlan& Plan)
	{
		Fdemo_mapShanmenThrownWeaponItemResult Committed =
			Plan.ItemCommitRequest;
		Committed.Status = Edemo_mapShanmenThrownWeaponItemStatus::Committed;
		Committed.FinalizeCommand.Status =
			EShanmenItemDurableCommandStatus::Persisted;
		FShanmenItemTransactionReceipt& Receipt =
			Committed.FinalizeCommand.Receipt;
		Receipt.bSuccess = true;
		Receipt.Operation =
			EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent;
		Receipt.Phase = EShanmenItemTransactionPhase::Committed;
		Receipt.Error = EShanmenItemTransactionError::None;
		Receipt.ReceiptId = FGuid(0xD3720022, 0, 0, 1);
		Receipt.RequestId = Committed.FinalizeRequest.Context.RequestId;
		Receipt.ReservationId = Committed.FinalizeRequest.IntentId;
		Receipt.ItemInstanceId = Committed.FinalizeRequest.ItemInstanceId;
		Receipt.ResourceKind = EShanmenItemResourceKind::Quantity;
		Receipt.Amount = 1;
		Receipt.ResourceBefore = 3;
		Receipt.ResourceAfter = 2;
		Receipt.AvailableAfter = 2;
		Receipt.ItemRevision = 3;
		Receipt.AuthorityRevision = 7;
		Receipt.PurposeId = Committed.PrepareRequest.PurposeId;
		Receipt.ReservationIds =
		{
			Committed.FinalizeRequest.ActiveRunId,
			Committed.FinalizeRequest.PrepareRequestId
		};
		check(Receipt.IsValid());
		check(Committed.IsFinalized());
		return Committed;
	}

	FHitResult MakeEnemyHit(const FThrownWorldFixture& Fixture)
	{
		FHitResult Hit(
			Fixture.Enemy,
			Fixture.GetEnemyRoot(),
			FVector(120.0, 10.0, 40.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(120.0, 10.0, 40.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}

	bool StageAndPublish(
		const FThrownWorldFixture& Fixture,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenThrownWeaponExecution& OutExecution,
		Ademo_mapShanmenThrownWeaponProjectile*& OutProjectile)
	{
		FShanmenCombatActionSnapshot Action;
		StartAction(Fixture.Coordinator, OutRuntime, OutExecution, Action);
		OutProjectile = NewObject<Ademo_mapShanmenThrownWeaponProjectile>(
			GetTransientPackage());
		if (!OutProjectile)
		{
			return false;
		}
		const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
			Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
				MakeCorrelation(),
				MakePrepared(Action),
				OutRuntime,
				OutExecution,
				*OutProjectile,
				Fixture.Pawn,
				FVector(10.0, 20.0, 30.0),
				FVector::ForwardVector);
		return Staged.IsStaged()
			&& Fdemo_mapShanmenThrownWeaponWorldAdapter::PublishCommittedLaunch(
				OutRuntime,
				Staged.Plan,
				MakeCommitted(Staged.Plan),
				OutExecution,
				*OutProjectile);
	}

	bool StartHostedFlight(
		FThrownWorldFixture& Fixture,
		Fdemo_mapShanmenThrownWeaponRunHost& OutHost,
		Ademo_mapShanmenThrownWeaponProjectile*& OutProjectile)
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenThrownWeaponExecution Execution;
		FShanmenCombatActionSnapshot Action;
		StartAction(Fixture.Coordinator, Runtime, Execution, Action);
		OutProjectile = NewObject<Ademo_mapShanmenThrownWeaponProjectile>(
			GetTransientPackage());
		if (!OutProjectile)
		{
			return false;
		}
		const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
			Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
				MakeCorrelation(),
				MakePrepared(Action),
				Runtime,
				Execution,
				*OutProjectile,
				Fixture.Pawn,
				FVector(10.0, 20.0, 30.0),
				FVector::ForwardVector);
		if (!Staged.IsStaged())
		{
			return false;
		}
		const Fdemo_mapShanmenThrownWeaponItemResult Committed =
			MakeCommitted(Staged.Plan);
		return Fdemo_mapShanmenThrownWeaponWorldAdapter::
				PublishCommittedLaunch(
					Runtime,
					Staged.Plan,
					Committed,
					Execution,
					*OutProjectile)
			&& OutHost.TryAdoptPublishedFlight(
				Runtime,
				Execution,
				Committed,
				*OutProjectile,
				Fixture.Coordinator,
				Fixture.Pawn,
				1200.0f,
				false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponWorldDurableGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.DurableLaunchGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponWorldDurableGateTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P7.2 launch fixture."));
		return false;
	}
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenCombatActionSnapshot Action;
	StartAction(Fixture.Coordinator, Runtime, Execution, Action);
	Ademo_mapShanmenThrownWeaponProjectile* Projectile =
		NewObject<Ademo_mapShanmenThrownWeaponProjectile>(GetTransientPackage());
	if (!Projectile)
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			Fixture.Pawn,
			FVector(10.0, 20.0, 30.0),
			FVector(4.0, 0.0, 0.0));
	TestTrue(TEXT("Staging freezes canonical launch evidence"),
		Staged.IsStaged()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Staged);
	TestTrue(TEXT("Staging cannot publish movement, collision, or live execution"),
		Execution.GetState() == EShanmenThrownWeaponState::Ready
			&& !Execution.IsEmissionActive()
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& !Projectile->GetMovementComponent()->IsActive());

	if (!GEngine)
	{
		AddError(TEXT("GEngine is unavailable for the P7.2 authority gate."));
		return false;
	}
	UGameInstance* GameInstance = NewObject<UGameInstance>(
		GEngine, NAME_None, RF_Transient);
	GameInstance->AddToRoot();
	GameInstance->Init();
	Udemo_mapShanmenItemAuthoritySubsystem* Unbound =
		GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	const Fdemo_mapShanmenThrownWeaponLaunchResult Rejected =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::CommitStagedLaunch(
			*Unbound, Runtime, Staged.Plan, Execution, *Projectile);
	TestTrue(TEXT("An unavailable durable authority cancels the inert Actor"),
		Rejected.Error
			== Edemo_mapShanmenThrownWeaponLaunchError::AuthorityCommitRejected
			&& Execution.GetState() == EShanmenThrownWeaponState::Ready
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Empty);
	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	GameInstance->MarkAsGarbage();

	const Fdemo_mapShanmenThrownWeaponLaunchResult Restaged =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			Fixture.Pawn,
			FVector(10.0, 20.0, 30.0),
			FVector::ForwardVector);
	const bool bPublished = Restaged.IsStaged()
		&& Fdemo_mapShanmenThrownWeaponWorldAdapter::PublishCommittedLaunch(
			Runtime,
			Restaged.Plan,
			MakeCommitted(Restaged.Plan),
			Execution,
			*Projectile);
	TestTrue(TEXT("Exact durable proof atomically publishes straight flight"),
		bPublished
			&& Execution.GetState() == EShanmenThrownWeaponState::InFlight
			&& Execution.IsEmissionActive()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::InFlight
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::QueryOnly
			&& Projectile->GetMovementComponent()->IsActive()
			&& Projectile->GetMovementComponent()->Velocity.Equals(
				FVector::ForwardVector * 750.0f));
	TestTrue(TEXT("Physical contract has no gravity, bounce, or homing"),
		Projectile->GetMovementComponent()->ProjectileGravityScale == 0.0f
			&& !Projectile->GetMovementComponent()->bShouldBounce
			&& !Projectile->GetMovementComponent()->bIsHomingProjectile);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcWorldMotionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.ArcReceiptMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcWorldMotionTest::RunTest(const FString&)
{
	FThrownWorldFixture IdentityFixture;
	FThrownSpawnWorldFixture WorldFixture;
	if (!IdentityFixture.bReady || !WorldFixture.IsValid())
	{
		AddError(TEXT("Could not build the P20.2 arc World fixture."));
		return false;
	}

	const FVector Origin(25.0, 40.0, 60.0);
	const FVector Target(525.0, 40.0, 60.0);
	const Fdemo_mapShanmenThrownWeaponSpawnResult Spawned =
		Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
			WorldFixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			WorldFixture.Source,
			Origin);
	Ademo_mapShanmenThrownWeaponProjectile* Projectile =
		Spawned.Projectile.Get();
	if (!Spawned.IsSpawned() || !Projectile)
	{
		AddError(TEXT("Could not spawn the P20.2 arc carrier."));
		return false;
	}

	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenCombatActionSnapshot Action;
	StartAction(
		IdentityFixture.Coordinator,
		Runtime,
		Execution,
		Action,
		FShanmenThrownWeaponDefinition::ArcActionDefinitionId());
	const FShanmenThrownWeaponArcPlan ArcPlan =
		MakeArcPlan(Action, Origin, Target);
	const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedArcLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			WorldFixture.Source,
			ArcPlan);
	const UProjectileMovementComponent* Movement =
		Projectile->GetMovementComponent();
	const double ExpectedGravityScale =
		ArcPlan.GetGravityAcceleration().Z
		/ static_cast<double>(WorldFixture.World->GetGravityZ());
	TestTrue(TEXT("Arc staging consumes the immutable plan while remaining inert"),
		Staged.IsStaged()
			&& Staged.Plan.Launch.GetTrajectoryKind()
				== EShanmenThrownWeaponTrajectoryKind::BallisticArc
			&& Staged.Plan.Launch.GetArcPlan().Matches(ArcPlan)
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Staged
			&& Movement
			&& !Movement->IsActive()
			&& Movement->Velocity.Equals(
				ArcPlan.GetInitialVelocity(), KINDA_SMALL_NUMBER)
			&& Movement->MaxSpeed == 0.0f
			&& FMath::IsNearlyEqual(
				Movement->ProjectileGravityScale,
				static_cast<float>(ExpectedGravityScale)));

	const bool bPublished = Staged.IsStaged()
		&& Fdemo_mapShanmenThrownWeaponWorldAdapter::PublishCommittedLaunch(
			Runtime,
			Staged.Plan,
			MakeCommitted(Staged.Plan),
			Execution,
			*Projectile);
	TestTrue(TEXT("Durable publication activates the same ballistic carrier"),
		bPublished
			&& Execution.GetState() == EShanmenThrownWeaponState::InFlight
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::InFlight
			&& Movement->IsActive()
			&& Movement->Velocity.Equals(
				ArcPlan.GetInitialVelocity(), KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(
				Movement->ProjectileGravityScale,
				static_cast<float>(ExpectedGravityScale)));
	TestTrue(TEXT("Arc flight terminates through the existing no-impact path"),
		Fdemo_mapShanmenThrownWeaponWorldAdapter::FinishFlightWithoutImpact(
			Runtime, Execution, *Projectile)
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcWorldFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.ArcFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcWorldFailClosedTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P20.2 fail-closed fixture."));
		return false;
	}
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenCombatActionSnapshot Action;
	StartAction(
		Fixture.Coordinator,
		Runtime,
		Execution,
		Action,
		FShanmenThrownWeaponDefinition::ArcActionDefinitionId());
	const FShanmenThrownWeaponArcPlan ArcPlan = MakeArcPlan(
		Action, FVector(10.0, 20.0, 30.0), FVector(510.0, 20.0, 30.0));
	Ademo_mapShanmenThrownWeaponProjectile* Worldless =
		NewObject<Ademo_mapShanmenThrownWeaponProjectile>(
			GetTransientPackage());
	if (!Worldless)
	{
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponLaunchResult WrongTrajectory =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Worldless,
			Fixture.Pawn,
			ArcPlan.GetRequest().GetOrigin(),
			ArcPlan.GetInitialVelocity());
	TestTrue(TEXT("Arc actions cannot fall back to the straight staging path"),
		WrongTrajectory.Error
			== Edemo_mapShanmenThrownWeaponLaunchError::LaunchRejected
			&& Execution.GetState() == EShanmenThrownWeaponState::Ready
			&& Worldless->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Empty);

	const Fdemo_mapShanmenThrownWeaponLaunchResult NoWorld =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedArcLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Worldless,
			Fixture.Pawn,
			ArcPlan);
	TestTrue(TEXT("Ballistic publication requires a World gravity authority"),
		NoWorld.Error
			== Edemo_mapShanmenThrownWeaponLaunchError::ProjectileStageRejected
			&& Execution.GetState() == EShanmenThrownWeaponState::Ready
			&& !Execution.IsEmissionActive()
			&& Worldless->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Empty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponWorldDeliveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.ContactToVitality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponWorldDeliveryTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	Ademo_mapShanmenThrownWeaponProjectile* Projectile = nullptr;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !StageAndPublish(Fixture, Runtime, Execution, Projectile))
	{
		AddError(TEXT("Could not build the P7.2 delivery fixture."));
		return false;
	}
	Idemo_mapCombatVitalityHost* VitalityHost =
		Cast<Idemo_mapCombatVitalityHost>(Fixture.Enemy);
	FShanmenTargetVitalitySnapshot Before;
	FShanmenTargetVitalitySnapshot After;
	if (!VitalityHost
		|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Before))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponWorldDeliveryResult Delivered =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::ResolveProjectileContact(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			MakeEnemyHit(Fixture));
	VitalityHost->TryCaptureCombatVitalitySnapshot(After);
	TestTrue(TEXT("Projectile evidence reaches the canonical vitality authority"),
		Delivered.IsDelivered());
	TestTrue(TEXT("Resolver reports the frozen non-lethal formula result"),
		FMath::IsNearlyEqual(
			Delivered.GetNewlyCommittedDamage(), 0.7f));
	TestTrue(TEXT("Enemy vitality commits the exact resolved damage"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality - After.CurrentVitality,
			0.7f,
			KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Enemy vitality authority advances exactly once"),
		After.AuthorityRevision, Before.AuthorityRevision + 1);
	TestTrue(TEXT("First successful impact terminally spends the physical item"),
		Execution.GetState() == EShanmenThrownWeaponState::Spent
			&& !Execution.IsEmissionActive()
			&& Execution.NumAcceptedImpacts() == 1
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision);

	const Fdemo_mapShanmenThrownWeaponWorldDeliveryResult Replay =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::ResolveProjectileContact(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			MakeEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot AfterReplay;
	VitalityHost->TryCaptureCombatVitalitySnapshot(AfterReplay);
	TestTrue(TEXT("A spent projectile cannot deliver the callback twice"),
		Replay.Error
			== Edemo_mapShanmenThrownWeaponWorldDeliveryError::FlightNotActive
			&& AfterReplay.AuthorityRevision == After.AuthorityRevision
			&& FMath::IsNearlyEqual(
				AfterReplay.CurrentVitality, After.CurrentVitality));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponWorldMissTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.FailClosedAndMiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponWorldMissTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	Ademo_mapShanmenThrownWeaponProjectile* Projectile = nullptr;
	if (!Fixture.bReady
		|| !StageAndPublish(Fixture, Runtime, Execution, Projectile))
	{
		return false;
	}
	AActor* Unregistered = NewObject<AActor>(GetTransientPackage());
	UBoxComponent* UnregisteredRoot = NewObject<UBoxComponent>(
		Unregistered, TEXT("P72UnregisteredRoot"));
	Unregistered->SetRootComponent(UnregisteredRoot);
	FHitResult UnregisteredHit(
		Unregistered,
		UnregisteredRoot,
		FVector(200.0, 0.0, 30.0),
		FVector::BackwardVector);
	UnregisteredHit.ImpactPoint = FVector(200.0, 0.0, 30.0);
	UnregisteredHit.ImpactNormal = FVector::BackwardVector;
	const Fdemo_mapShanmenThrownWeaponWorldDeliveryResult Rejected =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::ResolveProjectileContact(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			UnregisteredHit);
	TestTrue(TEXT("Unregistered world contacts fail without consuming flight"),
		Rejected.Error
			== Edemo_mapShanmenThrownWeaponWorldDeliveryError::ContactNotResolved
			&& Execution.GetState() == EShanmenThrownWeaponState::InFlight
			&& Execution.NumAcceptedImpacts() == 0
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::InFlight);
	TestTrue(TEXT("Range expiry has an explicit no-impact terminal path"),
		Fdemo_mapShanmenThrownWeaponWorldAdapter::FinishFlightWithoutImpact(
			Runtime, Execution, *Projectile)
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent
			&& Execution.NumAcceptedImpacts() == 0
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunHostSpawnGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunHost.SpawnGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunHostSpawnGateTest::RunTest(const FString&)
{
	AActor* Source = NewObject<AActor>(GetTransientPackage());
	const Fdemo_mapShanmenThrownWeaponSpawnResult NoWorld =
		Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
			nullptr,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Source,
			FVector::ZeroVector);
	TestTrue(TEXT("A missing World cannot create a physical carrier"),
		NoWorld.Error
			== Edemo_mapShanmenThrownWeaponSpawnError::WorldUnavailable
			&& !NoWorld.IsSpawned());
	FThrownSpawnWorldFixture SpawnFixture;
	if (!SpawnFixture.IsValid())
	{
		AddError(TEXT("Could not build the P7.3 spawn World fixture."));
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponSpawnResult Spawned =
		Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
			SpawnFixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			SpawnFixture.Source,
			FVector(25.0, 40.0, 60.0));
	TestTrue(TEXT("The production spawner creates exactly one inert carrier"),
		Spawned.IsSpawned()
			&& Spawned.Projectile->GetOwner() == SpawnFixture.Source
			&& Spawned.Projectile->GetActorLocation().Equals(
				FVector(25.0, 40.0, 60.0)));

	FThrownWorldFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	Ademo_mapShanmenThrownWeaponProjectile* Projectile = nullptr;
	if (!Fixture.bReady
		|| !StartHostedFlight(Fixture, Host, Projectile))
	{
		AddError(TEXT("Could not build the P7.3 host fixture."));
		return false;
	}
	TestTrue(TEXT("Recovery adoption owns one exact active flight"),
		Host.IsValid()
			&& Host.IsInFlight()
			&& Host.GetProjectile() == Projectile
			&& Host.GetActionRuntime().CanEmitCandidates()
			&& FMath::IsNearlyEqual(Host.GetMaximumDistance(), 1200.0f));
	TestFalse(TEXT("An active host cannot be reset without a terminal"),
		Host.Reset());
	TestTrue(TEXT("Explicit interruption spends flight and action once"),
		Host.TryInterrupt()
			&& Host.IsValid()
			&& Host.IsTerminal()
			&& Host.GetExecution().GetState()
				== EShanmenThrownWeaponState::Spent
			&& Host.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& Host.GetTerminalReceipt().Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::Interrupted);
	TestTrue(TEXT("A terminal host can release its transient binding"),
		Host.Reset()
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& !Host.IsValid());

	Ademo_mapShanmenThrownWeaponProjectile* AbandonedProjectile = nullptr;
	{
		Fdemo_mapShanmenThrownWeaponRunHost ScopedHost;
		if (!StartHostedFlight(
			Fixture, ScopedHost, AbandonedProjectile))
		{
			return false;
		}
	}
	TestTrue(TEXT("Destroying an active host fails closed on the GameThread"),
		AbandonedProjectile
			&& AbandonedProjectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunHostContactTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunHost.ContactLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunHostContactTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	Ademo_mapShanmenThrownWeaponProjectile* Projectile = nullptr;
	if (!Fixture.bReady
		|| !Fixture.GetEnemyRoot()
		|| !StartHostedFlight(Fixture, Host, Projectile))
	{
		return false;
	}
	Idemo_mapCombatVitalityHost* VitalityHost =
		Cast<Idemo_mapCombatVitalityHost>(Fixture.Enemy);
	FShanmenTargetVitalitySnapshot Before;
	if (!VitalityHost
		|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Before))
	{
		return false;
	}

	Projectile->OnContact().Broadcast(*Projectile, MakeEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot After;
	VitalityHost->TryCaptureCombatVitalitySnapshot(After);
	TestTrue(TEXT("The native contact delegate reaches canonical vitality"),
		Host.IsValid()
			&& Host.IsTerminal()
			&& Host.GetTerminalReceipt().Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::Impact
			&& Host.GetTerminalReceipt().Delivery.IsDelivered()
			&& FMath::IsNearlyEqual(
				Before.CurrentVitality - After.CurrentVitality,
				0.7f,
				KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Impact completes execution, action, and Actor together"),
		Host.GetExecution().GetState()
				== EShanmenThrownWeaponState::Spent
			&& Host.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent);

	Projectile->OnContact().Broadcast(*Projectile, MakeEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot AfterReplay;
	VitalityHost->TryCaptureCombatVitalitySnapshot(AfterReplay);
	TestTrue(TEXT("Terminal publication removes the contact delegate"),
		AfterReplay.AuthorityRevision == After.AuthorityRevision
			&& FMath::IsNearlyEqual(
				AfterReplay.CurrentVitality, After.CurrentVitality));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponRunHostMissTest,
	"Shanmen.0_0_10.Product.ThrownWeaponRunHost.MissAndRangeExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponRunHostMissTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost ContactHost;
	Ademo_mapShanmenThrownWeaponProjectile* ContactProjectile = nullptr;
	if (!Fixture.bReady
		|| !StartHostedFlight(Fixture, ContactHost, ContactProjectile))
	{
		return false;
	}
	AActor* Unregistered = NewObject<AActor>(GetTransientPackage());
	UBoxComponent* Root = NewObject<UBoxComponent>(
		Unregistered, TEXT("P73UnregisteredRoot"));
	Unregistered->SetRootComponent(Root);
	FHitResult Blocking(
		Unregistered,
		Root,
		FVector(500.0, 0.0, 30.0),
		FVector::BackwardVector);
	Blocking.ImpactPoint = FVector(500.0, 0.0, 30.0);
	Blocking.ImpactNormal = FVector::BackwardVector;
	ContactProjectile->OnContact().Broadcast(*ContactProjectile, Blocking);
	TestTrue(TEXT("An unresolved blocking contact is one no-impact terminal"),
		ContactHost.IsValid()
			&& ContactHost.IsTerminal()
			&& ContactHost.GetTerminalReceipt().Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::BlockingMiss
			&& ContactHost.GetTerminalReceipt().Delivery.Error
				== Edemo_mapShanmenThrownWeaponWorldDeliveryError::
					ContactNotResolved
			&& ContactHost.GetExecution().NumAcceptedImpacts() == 0
			&& ContactHost.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed);

	Fdemo_mapShanmenThrownWeaponRunHost RangeHost;
	Ademo_mapShanmenThrownWeaponProjectile* RangeProjectile = nullptr;
	if (!StartHostedFlight(Fixture, RangeHost, RangeProjectile))
	{
		return false;
	}
	TestTrue(TEXT("Range expiry uses the same explicit miss terminal"),
		RangeHost.TryExpireRange()
			&& RangeHost.IsValid()
			&& RangeHost.GetTerminalReceipt().Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::RangeExpired
			&& RangeHost.GetExecution().NumAcceptedImpacts() == 0
			&& RangeProjectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent);
	TestFalse(TEXT("A terminal range callback cannot run twice"),
		RangeHost.TryExpireRange());
	return true;
}

#endif
