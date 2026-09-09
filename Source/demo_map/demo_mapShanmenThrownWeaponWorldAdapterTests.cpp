#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponWorldAdapter.h"
#include "demo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation.h"
#include "demo_mapShanmenThrownWeaponRunHost.h"
#include "demo_mapShanmenThrownWeaponTerminalFeedbackPresentation.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	struct FThrownLethalWorldFixture
	{
		UWorld* World = nullptr;
		APawn* Pawn = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FThrownLethalWorldFixture()
		{
			const Fdemo_mapM01EnemyDefinition* Definition =
				FindMeleeDefinition();
			if (!GEngine || !Definition)
			{
				Diagnostic = TEXT("P22.2 lethal World fixture requires GEngine and the melee definition.");
				return;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				Diagnostic = TEXT("P22.2 lethal World fixture could not allocate its temporary World.");
				return;
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
			Pawn = World->SpawnActor<APawn>();
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P222LethalPlayerHealth"))
				: nullptr;
			Enemy = World->SpawnActor<Ademo_mapEnemyCharacter>();
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P222LethalEnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerHealth || !Enemy || !EnemyIdentity)
			{
				Diagnostic = TEXT("P22.2 lethal World fixture could not construct its combat hosts.");
				return;
			}
			Pawn->AddInstanceComponent(PlayerHealth);
			Enemy->AddInstanceComponent(EnemyIdentity);
			if (!EnemyIdentity->Configure(*Definition))
			{
				Diagnostic = TEXT("P22.2 lethal World fixture could not configure enemy identity.");
				return;
			}
			if (!Enemy->ConfigureEncounter(
					MakeEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite()))
			{
				Diagnostic = TEXT("P22.2 lethal World fixture could not configure enemy vitality.");
				return;
			}
			if (!Coordinator.TryBeginRun(
					WorldRunId, Pawn, PlayerHealth, Diagnostic))
			{
				return;
			}
			bReady = Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		~FThrownLethalWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	struct FThrownCollisionWorldFixture
	{
		UWorld* World = nullptr;
		APawn* Source = nullptr;
		AActor* Blocker = nullptr;
		UBoxComponent* BlockerRoot = nullptr;

		FThrownCollisionWorldFixture()
		{
			if (!GEngine)
			{
				return;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.SetCurrentWorld(World);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(true)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(true)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(true)
					.SetTransactional(false)
					.CreateFXSystem(false));

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Source = World->SpawnActor<APawn>(
				APawn::StaticClass(),
				FTransform(FVector(-1000.0f, 0.0f, 100.0f)),
				Parameters);
			Blocker = World->SpawnActor<AActor>(
				AActor::StaticClass(),
				FTransform(FVector(8.0f, 100.0f, 100.0f)),
				Parameters);
			BlockerRoot = Blocker
				? NewObject<UBoxComponent>(
					Blocker, TEXT("P221NarrowBlocker"), RF_Transient)
				: nullptr;
			if (!Source || !Blocker || !BlockerRoot)
			{
				return;
			}
			Blocker->SetRootComponent(BlockerRoot);
			Blocker->AddInstanceComponent(BlockerRoot);
			BlockerRoot->InitBoxExtent(FVector(1.0f, 2.0f, 10.0f));
			BlockerRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			BlockerRoot->SetCollisionObjectType(ECC_WorldStatic);
			BlockerRoot->SetCollisionResponseToAllChannels(ECR_Block);
			BlockerRoot->RegisterComponent();
			World->UpdateWorldComponents(true, false);
			World->InitializeActorsForPlay(FURL());
		}

		~FThrownCollisionWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}

		bool IsValid() const
		{
			return World && Source && Blocker && BlockerRoot;
		}
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
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId(),
		const float BaseDamage = 0.5f)
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.2.Product");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.2.Product");
		// Keep this transient Actor fixture alive; death behavior belongs to the
		// coordinator suite and requires a registered World.
		Capture.BaseDamage = BaseDamage;
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
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId(),
		const float TechniquePower = 20.0f,
		const float BaseDamage = 0.5f)
	{
		OutAction = MakeAction(Coordinator, ActionDefinitionId);
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			OutAction, OutRuntime, Transition));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Transition));
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			TechniquePower, Offense));
		check(FShanmenThrownWeaponExecution::TryCreate(
			OutAction,
			MakeDefinition(ActionDefinitionId, BaseDamage),
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

	FHitResult MakeEnemyHit(const FThrownLethalWorldFixture& Fixture)
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

	bool StartHostedFlightWithBindings(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		APawn& Source,
		Fdemo_mapShanmenThrownWeaponRunHost& OutHost,
		Ademo_mapShanmenThrownWeaponProjectile*& OutProjectile,
		const float TechniquePower = 20.0f,
		UWorld* CarrierWorld = nullptr,
		const float BaseDamage = 0.5f)
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenThrownWeaponExecution Execution;
		FShanmenCombatActionSnapshot Action;
		StartAction(
			Coordinator,
			Runtime,
			Execution,
			Action,
			FShanmenThrownWeaponDefinition::StraightActionDefinitionId(),
			TechniquePower,
			BaseDamage);
		if (CarrierWorld)
		{
			const Fdemo_mapShanmenThrownWeaponSpawnResult Spawn =
				Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
					CarrierWorld,
					Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
					&Source,
					FVector(10.0, 20.0, 30.0));
			OutProjectile = Spawn.Projectile.Get();
		}
		else
		{
			OutProjectile = NewObject<Ademo_mapShanmenThrownWeaponProjectile>(
				GetTransientPackage());
		}
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
				&Source,
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
				Coordinator,
				&Source,
				1200.0f,
				false);
	}

	bool StartHostedFlight(
		FThrownWorldFixture& Fixture,
		Fdemo_mapShanmenThrownWeaponRunHost& OutHost,
		Ademo_mapShanmenThrownWeaponProjectile*& OutProjectile,
		const float TechniquePower = 20.0f,
		const float BaseDamage = 0.5f)
	{
		return Fixture.Pawn
			&& StartHostedFlightWithBindings(
				Fixture.Coordinator,
				*Fixture.Pawn,
				OutHost,
				OutProjectile,
				TechniquePower,
				nullptr,
				BaseDamage);
	}

	bool StartHostedFlight(
		FThrownLethalWorldFixture& Fixture,
		Fdemo_mapShanmenThrownWeaponRunHost& OutHost,
		Ademo_mapShanmenThrownWeaponProjectile*& OutProjectile,
		const float TechniquePower,
		const float BaseDamage = 0.5f)
	{
		return Fixture.Pawn
			&& StartHostedFlightWithBindings(
				Fixture.Coordinator,
				*Fixture.Pawn,
				OutHost,
				OutProjectile,
				TechniquePower,
				Fixture.World,
				BaseDamage);
	}

	Fdemo_mapShanmenThrownWeaponTrajectoryPresentation
	MakeStraightTrajectoryPresentation()
	{
		const Fdemo_mapShanmenThrownWeaponInputChoiceState Choice =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		const auto Read =
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
				[]() { return true; },
				[&Choice]() { return Choice; });
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation Presentation;
		check(Fdemo_mapShanmenThrownWeaponTrajectoryPresentation::TryProject(
			Read, TEXT("V"), Presentation));
		return Presentation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponWorldDurableGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.DurableLaunchGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponWorldDurableGateTest::RunTest(const FString&)
{
	FThrownWorldFixture Fixture;
	FThrownSpawnWorldFixture WorldFixture;
	if (!Fixture.bReady || !WorldFixture.IsValid())
	{
		AddError(TEXT("Could not build the P7.2 launch fixture."));
		return false;
	}
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenCombatActionSnapshot Action;
	StartAction(Fixture.Coordinator, Runtime, Execution, Action);
	const Fdemo_mapShanmenThrownWeaponSpawnResult Spawned =
		Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
			WorldFixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			WorldFixture.Source,
			FVector(10.0, 20.0, 30.0));
	Ademo_mapShanmenThrownWeaponProjectile* Projectile =
		Spawned.Projectile.Get();
	URotatingMovementComponent* VisualRoll = Projectile
		? Projectile->FindComponentByClass<URotatingMovementComponent>()
		: nullptr;
	UStaticMeshComponent* BladeVisual = nullptr;
	UStaticMeshComponent* BladeEdgeVisual = nullptr;
	UStaticMeshComponent* GripVisual = nullptr;
	int32 NumPresentationMeshes = 0;
	if (Projectile)
	{
		TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
		Projectile->GetComponents(MeshComponents);
		NumPresentationMeshes = MeshComponents.Num();
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent
				&& MeshComponent->GetFName() == TEXT("ThrownWeaponVisual"))
			{
				BladeVisual = MeshComponent;
			}
			else if (MeshComponent
				&& MeshComponent->GetFName()
					== TEXT("ThrownWeaponBladeEdgeVisual"))
			{
				BladeEdgeVisual = MeshComponent;
			}
			else if (MeshComponent
				&& MeshComponent->GetFName()
					== TEXT("ThrownWeaponGripVisual"))
			{
				GripVisual = MeshComponent;
			}
		}
	}
	USceneComponent* PresentationPivot = BladeVisual
		? BladeVisual->GetAttachParent()
		: nullptr;
	if (!Spawned.IsSpawned()
		|| !Projectile
		|| !VisualRoll
		|| !BladeVisual
		|| !BladeEdgeVisual
		|| !GripVisual
		|| !PresentationPivot)
	{
		return false;
	}
	const FVector BladeBodyFullSize =
		BladeVisual->GetRelativeScale3D() * 100.0f;
	const FVector BladeEdgeFullSize =
		BladeEdgeVisual->GetRelativeScale3D() * 100.0f;
	const FVector GripFullSize =
		GripVisual->GetRelativeScale3D() * 100.0f;
	UMaterialInstanceDynamic* BladeMaterial = Cast<UMaterialInstanceDynamic>(
		BladeVisual->GetMaterial(0));
	UMaterialInstanceDynamic* BladeEdgeMaterial =
		Cast<UMaterialInstanceDynamic>(BladeEdgeVisual->GetMaterial(0));
	UMaterialInstanceDynamic* GripMaterial = Cast<UMaterialInstanceDynamic>(
		GripVisual->GetMaterial(0));
	TestTrue(TEXT("Prototype silhouette is one blade body, edge, and grip"),
		NumPresentationMeshes == 3
			&& BladeVisual->GetStaticMesh()
			&& BladeEdgeVisual->GetStaticMesh()
				== BladeVisual->GetStaticMesh()
			&& GripVisual->GetStaticMesh() == BladeVisual->GetStaticMesh()
			&& BladeVisual->GetAttachParent() == PresentationPivot
			&& BladeEdgeVisual->GetAttachParent() == PresentationPivot
			&& GripVisual->GetAttachParent() == PresentationPivot
			&& PresentationPivot->GetAttachParent()
				== Projectile->GetCollisionComponent()
			&& VisualRoll->UpdatedComponent == PresentationPivot
			&& BladeVisual->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& BladeEdgeVisual->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& GripVisual->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& BladeBodyFullSize.Equals(
				FVector(22.0f, 4.0f, 1.2f), KINDA_SMALL_NUMBER)
			&& BladeEdgeFullSize.Equals(
				FVector(22.0f, 0.5f, 1.2f), KINDA_SMALL_NUMBER)
			&& GripFullSize.Equals(
				FVector(8.0f, 3.0f, 1.2f), KINDA_SMALL_NUMBER)
			&& BladeVisual->GetRelativeLocation().Equals(
				FVector(4.0f, 0.25f, 0.0f), KINDA_SMALL_NUMBER)
			&& BladeEdgeVisual->GetRelativeLocation().Equals(
				FVector(4.0f, -2.0f, 0.0f), KINDA_SMALL_NUMBER)
			&& GripVisual->GetRelativeLocation().Equals(
				FVector(-11.0f, 0.0f, 0.0f), KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(
				BladeVisual->GetRelativeLocation().X
					- BladeBodyFullSize.X * 0.5f,
				GripVisual->GetRelativeLocation().X + GripFullSize.X * 0.5f)
			&& FMath::IsNearlyEqual(
				BladeEdgeVisual->GetRelativeLocation().X
					- BladeEdgeFullSize.X * 0.5f,
				GripVisual->GetRelativeLocation().X + GripFullSize.X * 0.5f)
			&& FMath::IsNearlyEqual(
				GripVisual->GetRelativeLocation().X - GripFullSize.X * 0.5f,
				-Projectile->GetCollisionComponent()->GetUnscaledBoxExtent().X)
			&& FMath::IsNearlyEqual(
				BladeVisual->GetRelativeLocation().X
					+ BladeBodyFullSize.X * 0.5f,
				Projectile->GetCollisionComponent()->GetUnscaledBoxExtent().X)
			&& FMath::IsNearlyEqual(
				BladeEdgeVisual->GetRelativeLocation().X
					+ BladeEdgeFullSize.X * 0.5f,
				Projectile->GetCollisionComponent()->GetUnscaledBoxExtent().X));
	TestTrue(TEXT("Asymmetric edge exactly tiles the existing blade width"),
		FMath::IsNearlyEqual(
			BladeEdgeVisual->GetRelativeLocation().Y
				- BladeEdgeFullSize.Y * 0.5f,
			-Projectile->GetCollisionComponent()->GetUnscaledBoxExtent().Y)
			&& FMath::IsNearlyEqual(
				BladeEdgeVisual->GetRelativeLocation().Y
					+ BladeEdgeFullSize.Y * 0.5f,
				BladeVisual->GetRelativeLocation().Y
					- BladeBodyFullSize.Y * 0.5f)
			&& FMath::IsNearlyEqual(
				BladeVisual->GetRelativeLocation().Y
					+ BladeBodyFullSize.Y * 0.5f,
				Projectile->GetCollisionComponent()->GetUnscaledBoxExtent().Y));
	TestTrue(TEXT("Blade body, edge, and grip use distinct prototype colors"),
		Projectile->HasPresentationMaterialContrast()
			&& BladeMaterial
			&& BladeEdgeMaterial
			&& GripMaterial
			&& BladeMaterial != BladeEdgeMaterial
			&& BladeMaterial != GripMaterial
			&& BladeEdgeMaterial != GripMaterial
			&& BladeMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(
				FLinearColor(0.62f, 0.78f, 1.0f), KINDA_SMALL_NUMBER)
			&& BladeEdgeMaterial->K2_GetVectorParameterValue(
				TEXT("Color")).Equals(
					FLinearColor(0.92f, 0.97f, 1.0f),
					KINDA_SMALL_NUMBER)
			&& GripMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(
				FLinearColor(0.16f, 0.045f, 0.012f),
				KINDA_SMALL_NUMBER));
	const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			WorldFixture.Source,
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
			&& !BladeVisual->IsVisible()
			&& !BladeEdgeVisual->IsVisible()
			&& !GripVisual->IsVisible()
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsPresentationRollActive()
			&& !Projectile->IsFlightCueVisible()
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
				== Edemo_mapShanmenThrownWeaponProjectileState::Empty
			&& !BladeVisual->IsVisible()
			&& !BladeEdgeVisual->IsVisible()
			&& !GripVisual->IsVisible()
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsPresentationRollActive()
			&& !Projectile->IsFlightCueVisible());
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
			WorldFixture.Source,
			FVector(10.0, 20.0, 30.0),
			FVector::RightVector);
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
			&& BladeVisual->IsVisible()
			&& BladeEdgeVisual->IsVisible()
			&& GripVisual->IsVisible()
			&& Projectile->IsPresentationVisible()
			&& Projectile->IsPresentationRollActive()
			&& Projectile->IsFlightCueVisible()
			&& Projectile->GetFlightCueColor().Equals(
				FLinearColor(1.0f, 0.48f, 0.08f), 0.01f)
			&& Projectile->GetPresentationForwardDirection().Equals(
				FVector::RightVector, KINDA_SMALL_NUMBER)
			&& Projectile->GetMovementComponent()->IsActive()
			&& Projectile->GetMovementComponent()->Velocity.Equals(
				FVector::RightVector * 750.0f));
	const FQuat ActorRotationBeforeRoll = Projectile->GetActorQuat();
	const FQuat CollisionRotationBeforeRoll =
		Projectile->GetCollisionComponent()->GetComponentQuat();
	const FVector PresentationUpBeforeRoll =
		Projectile->GetPresentationUpDirection();
	const FVector BladeUpBeforeRoll = BladeVisual->GetUpVector();
	const FVector BladeEdgeUpBeforeRoll = BladeEdgeVisual->GetUpVector();
	const FVector BladeEdgeLocationBeforeRoll =
		BladeEdgeVisual->GetComponentLocation();
	const FVector GripUpBeforeRoll = GripVisual->GetUpVector();
	VisualRoll->TickComponent(0.125f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Flight roll moves only the collisionless blade presentation"),
		!Projectile->GetPresentationUpDirection().Equals(
			PresentationUpBeforeRoll, KINDA_SMALL_NUMBER)
			&& !BladeVisual->GetUpVector().Equals(
				BladeUpBeforeRoll, KINDA_SMALL_NUMBER)
			&& !BladeEdgeVisual->GetUpVector().Equals(
				BladeEdgeUpBeforeRoll, KINDA_SMALL_NUMBER)
			&& !BladeEdgeVisual->GetComponentLocation().Equals(
				BladeEdgeLocationBeforeRoll, KINDA_SMALL_NUMBER)
			&& !GripVisual->GetUpVector().Equals(
				GripUpBeforeRoll, KINDA_SMALL_NUMBER)
			&& BladeVisual->GetUpVector().Equals(
				GripVisual->GetUpVector(), KINDA_SMALL_NUMBER)
			&& BladeEdgeVisual->GetUpVector().Equals(
				GripVisual->GetUpVector(), KINDA_SMALL_NUMBER)
			&& BladeVisual->GetMaterial(0) == BladeMaterial
			&& BladeEdgeVisual->GetMaterial(0) == BladeEdgeMaterial
			&& GripVisual->GetMaterial(0) == GripMaterial
			&& Projectile->GetPresentationForwardDirection().Equals(
				FVector::RightVector, KINDA_SMALL_NUMBER)
			&& Projectile->GetActorQuat().Equals(
				ActorRotationBeforeRoll, KINDA_SMALL_NUMBER)
			&& Projectile->GetCollisionComponent()->GetComponentQuat().Equals(
				CollisionRotationBeforeRoll, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Physical contract has no gravity, bounce, or homing"),
		Projectile->GetMovementComponent()->ProjectileGravityScale == 0.0f
			&& !Projectile->GetMovementComponent()->bShouldBounce
			&& !Projectile->GetMovementComponent()->bIsHomingProjectile);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponLaunchClearanceGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.LaunchClearanceGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponLaunchClearanceGateTest::RunTest(const FString&)
{
	FThrownWorldFixture IdentityFixture;
	FThrownCollisionWorldFixture WorldFixture;
	if (!IdentityFixture.bReady || !WorldFixture.IsValid())
	{
		AddError(TEXT("Could not build the P22.9 launch-clearance fixture."));
		return false;
	}

	const FVector Origin = WorldFixture.Blocker->GetActorLocation();
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
		AddError(TEXT("Could not spawn the P22.9 inert launch carrier."));
		return false;
	}

	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenCombatActionSnapshot Action;
	StartAction(IdentityFixture.Coordinator, Runtime, Execution, Action);
	const Fdemo_mapShanmenThrownWeaponLaunchResult Blocked =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			WorldFixture.Source,
			Origin,
			FVector::RightVector);
	TestTrue(TEXT("A blocking overlap rejects launch before durable publication"),
		!Blocked.IsStaged()
			&& !Blocked.IsCommitted()
			&& Blocked.Error
				== Edemo_mapShanmenThrownWeaponLaunchError::ProjectileStageRejected);
	TestTrue(TEXT("Blocked clearance leaves the physical carrier inert and empty"),
		Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Empty
			&& !Projectile->GetLaunchReceipt().IsValid()
			&& !Projectile->GetHitContext().IsValid()
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& !Projectile->GetMovementComponent()->IsActive()
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());

	WorldFixture.Blocker->SetActorLocation(
		FVector(1000.0f, 1000.0f, 100.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	WorldFixture.World->UpdateWorldComponents(true, false);
	const Fdemo_mapShanmenThrownWeaponLaunchResult Clear =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			WorldFixture.Source,
			Origin,
			FVector::RightVector);
	TestTrue(TEXT("The unchanged launch stages once its exact volume is clear"),
		Clear.IsStaged()
			&& !Clear.IsCommitted()
			&& Projectile->IsStagedFor(Clear.Plan.Launch, Clear.Plan.Context));
	TestTrue(TEXT("A clear staged retry remains safely cancellable"),
		Projectile->CancelStagedLaunch()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Empty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponCollisionProfileSweepTest,
	"Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.CollisionProfileSweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponCollisionProfileSweepTest::RunTest(const FString&)
{
	FThrownWorldFixture IdentityFixture;
	FThrownCollisionWorldFixture WorldFixture;
	if (!IdentityFixture.bReady || !WorldFixture.IsValid())
	{
		AddError(TEXT("Could not build the P22.1 collision World fixture."));
		return false;
	}

	const FVector Origin(0.0f, 0.0f, 100.0f);
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
		AddError(TEXT("Could not spawn the P22.1 collision carrier."));
		return false;
	}

	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenCombatActionSnapshot Action;
	StartAction(
		IdentityFixture.Coordinator, Runtime, Execution, Action);
	const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			MakeCorrelation(),
			MakePrepared(Action),
			Runtime,
			Execution,
			*Projectile,
			WorldFixture.Source,
			Origin,
			FVector::RightVector);
	const bool bPublished = Staged.IsStaged()
		&& Fdemo_mapShanmenThrownWeaponWorldAdapter::PublishCommittedLaunch(
			Runtime,
			Staged.Plan,
			MakeCommitted(Staged.Plan),
			Execution,
			*Projectile);
	UBoxComponent* Collision = Projectile->GetCollisionComponent();
	if (!bPublished || !Collision)
	{
		AddError(TEXT("Could not publish the P22.1 collision carrier."));
		return false;
	}

	TestTrue(TEXT("The contact profile matches the visible knife dimensions"),
		Collision->GetUnscaledBoxExtent().Equals(
			FVector(15.0f, 2.25f, 0.6f), KINDA_SMALL_NUMBER)
			&& Projectile->GetActorForwardVector().Equals(
				FVector::RightVector, KINDA_SMALL_NUMBER));
	int32 ContactCount = 0;
	AActor* ContactActor = nullptr;
	Projectile->OnContact().AddLambda(
		[&ContactCount, &ContactActor](
			Ademo_mapShanmenThrownWeaponProjectile&,
			const FHitResult& Hit)
		{
			++ContactCount;
			ContactActor = Hit.GetActor();
		});
	TestTrue(TEXT("Projectile stop owns the production contact handler"),
		Projectile->GetMovementComponent()->OnProjectileStop.IsBound());
	const TArray<UObject*> StopBindingObjects =
		Projectile->GetMovementComponent()->OnProjectileStop.GetAllObjects();
	TestTrue(TEXT("The live carrier owns the projectile stop binding"),
		StopBindingObjects.Contains(Projectile));
	TestTrue(TEXT("The test observer is bound to the native contact seam"),
		Projectile->OnContact().IsBound());

	FHitResult SideOffsetSweep;
	Collision->MoveComponent(
		FVector(0.0f, 200.0f, 0.0f),
		Projectile->GetActorQuat(),
		true,
		&SideOffsetSweep,
		MOVECOMP_NoFlags,
		ETeleportType::None);
	TestTrue(TEXT("A narrow blocker outside the visible width does not hit"),
		!SideOffsetSweep.bBlockingHit
			&& ContactCount == 0
			&& Projectile->GetActorLocation().Equals(
				Origin + FVector(0.0f, 200.0f, 0.0f), 0.01f));

	const bool bResetForAlignedSweep =
		Projectile->SetActorLocationAndRotation(
		Origin,
		FVector::RightVector.Rotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	WorldFixture.Blocker->SetActorLocation(
		FVector(0.0f, 100.0f, 100.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	WorldFixture.World->UpdateWorldComponents(true, false);

	FHitResult AlignedSweep;
	Collision->MoveComponent(
		FVector(0.0f, 200.0f, 0.0f),
		Projectile->GetActorQuat(),
		true,
		&AlignedSweep,
		MOVECOMP_NoFlags,
		ETeleportType::None);
	TestTrue(TEXT("The aligned knife nose produces one real blocking sweep"),
		bResetForAlignedSweep
			&& AlignedSweep.bBlockingHit
			&& AlignedSweep.GetActor() == WorldFixture.Blocker
			&& AlignedSweep.GetComponent() == WorldFixture.BlockerRoot
			&& AlignedSweep.Time > 0.0f
			&& AlignedSweep.Time < 1.0f);
	const bool bResetForMovement = Projectile->SetActorLocationAndRotation(
		Origin,
		FVector::RightVector.Rotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	ContactCount = 0;
	ContactActor = nullptr;
	Projectile->GetMovementComponent()->Velocity =
		FVector::RightVector * 750.0f;
	Projectile->GetMovementComponent()->Activate(true);
	TestTrue(TEXT("The movement proof resets the carrier before its frame"),
		bResetForMovement
			&& Projectile->GetActorLocation().Equals(Origin, 0.01f)
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::InFlight
			&& Projectile->GetMovementComponent()->UpdatedComponent
				== Collision);
	Projectile->GetMovementComponent()->TickComponent(
		0.2f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Canonical projectile movement emits one contact"),
		ContactCount, 1);
	TestTrue(TEXT("Canonical movement identifies the aligned blocker"),
		ContactActor == WorldFixture.Blocker);
	TestTrue(TEXT("Canonical movement stops at the aligned knife nose"),
		Projectile->GetActorLocation().Y > Origin.Y
			&& Projectile->GetActorLocation().Y <
				WorldFixture.Blocker->GetActorLocation().Y
			&& Projectile->GetMovementComponent()->UpdatedComponent
				== nullptr);
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
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsPresentationRollActive()
			&& !Projectile->IsFlightCueVisible()
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
			&& Projectile->IsPresentationVisible()
			&& Projectile->IsPresentationRollActive()
			&& Projectile->IsFlightCueVisible()
			&& Projectile->GetFlightCueColor().Equals(
				FLinearColor(0.20f, 0.72f, 1.0f), 0.01f)
			&& Projectile->GetPresentationForwardDirection().Equals(
				ArcPlan.GetInitialVelocity().GetSafeNormal(),
				KINDA_SMALL_NUMBER)
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
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsPresentationRollActive()
			&& !Projectile->IsFlightCueVisible());
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
				== ECollisionEnabled::NoCollision
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsPresentationRollActive()
			&& !Projectile->IsFlightCueVisible());

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
				== Edemo_mapShanmenThrownWeaponProjectileState::InFlight
			&& Projectile->IsPresentationRollActive()
			&& Projectile->IsFlightCueVisible());
	TestTrue(TEXT("Range expiry has an explicit no-impact terminal path"),
		Fdemo_mapShanmenThrownWeaponWorldAdapter::FinishFlightWithoutImpact(
			Runtime, Execution, *Projectile)
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent
			&& Execution.NumAcceptedImpacts() == 0
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenThrownWeaponProjectileState::Spent
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsPresentationRollActive()
			&& !Projectile->IsFlightCueVisible());
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
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation Interrupted;
	TestTrue(TEXT("Interruption projects an honest terminal HUD message"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			Host.GetTerminalReceipt(), Interrupted)
			&& Interrupted.GetKind()
				== Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::Interrupted
			&& Interrupted.GetDisplayText() == TEXT("飞刀 · 已中断"));
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
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation ImpactFeedback;
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation ImpactReplay;
	TestTrue(TEXT("Committed impact projects exact applied damage once"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			Host.GetTerminalReceipt(), ImpactFeedback)
			&& Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::
				TryProject(Host.GetTerminalReceipt(), ImpactReplay)
			&& ImpactFeedback.Matches(ImpactReplay)
			&& ImpactFeedback.GetKind()
				== Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::Impact
			&& FMath::IsNearlyEqual(
				ImpactFeedback.GetAppliedDamage(), 0.7f)
			&& !ImpactFeedback.DidDefeatTarget()
			&& ImpactFeedback.GetDisplayText()
				== TEXT("飞刀 · 命中 · -0.7"));
	Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation Stack;
	TestTrue(TEXT("The existing combat hint stack appends terminal feedback"),
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation::
			TryCompose(
				MakeStraightTrajectoryPresentation(),
				Fdemo_mapShanmenThrownWeaponArcEditingPresentation(),
				Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation(),
				Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation(),
				ImpactFeedback,
				Stack)
			&& Stack.IsValid()
			&& Stack.NumLines() == 2
			&& Stack.GetLines()[1].GetKind()
				== Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind::
					TerminalFeedback
			&& Stack.GetLines()[1].GetTone()
				== Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone::Impact
			&& Stack.GetLines()[1].GetDisplayText()
				== ImpactFeedback.GetDisplayText());

	Projectile->OnContact().Broadcast(*Projectile, MakeEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot AfterReplay;
	VitalityHost->TryCaptureCombatVitalitySnapshot(AfterReplay);
	TestTrue(TEXT("Terminal publication removes the contact delegate"),
		AfterReplay.AuthorityRevision == After.AuthorityRevision
			&& FMath::IsNearlyEqual(
				AfterReplay.CurrentVitality, After.CurrentVitality));

	FThrownLethalWorldFixture DefeatFixture;
	Fdemo_mapShanmenThrownWeaponRunHost DefeatHost;
	Ademo_mapShanmenThrownWeaponProjectile* DefeatProjectile = nullptr;
	TestTrue(TEXT("Lethal feedback World fixture is ready"),
		DefeatFixture.bReady);
	if (!DefeatFixture.bReady)
	{
		AddError(DefeatFixture.Diagnostic);
		return false;
	}
	const bool bDefeatFlightStarted = StartHostedFlight(
		DefeatFixture, DefeatHost, DefeatProjectile, 1000.0f);
	TestTrue(TEXT("Lethal feedback flight starts"), bDefeatFlightStarted);
	if (!bDefeatFlightStarted)
	{
		return false;
	}
	DefeatProjectile->OnContact().Broadcast(
		*DefeatProjectile, MakeEnemyHit(DefeatFixture));
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation DefeatFeedback;
	TestTrue(TEXT("Lethal committed impact is distinguished from a hit"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			DefeatHost.GetTerminalReceipt(), DefeatFeedback)
			&& DefeatFeedback.DidDefeatTarget()
			&& DefeatFeedback.GetAppliedDamage() > 0.0f
			&& DefeatFeedback.GetDisplayText().Contains(TEXT("飞刀 · 击破 · -")));

	FThrownWorldFixture ZeroDamageFixture;
	Fdemo_mapShanmenThrownWeaponRunHost ZeroDamageHost;
	Ademo_mapShanmenThrownWeaponProjectile* ZeroDamageProjectile = nullptr;
	const bool bZeroDamageFlightStarted = ZeroDamageFixture.bReady
		&& StartHostedFlight(
			ZeroDamageFixture,
			ZeroDamageHost,
			ZeroDamageProjectile,
			0.0f,
			0.0f);
	TestTrue(TEXT("Zero-damage feedback flight starts"),
		bZeroDamageFlightStarted);
	if (!bZeroDamageFlightStarted)
	{
		return false;
	}
	ZeroDamageProjectile->OnContact().Broadcast(
		*ZeroDamageProjectile, MakeEnemyHit(ZeroDamageFixture));
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation ZeroDamageFeedback;
	TestTrue(TEXT("Committed zero damage remains distinct from a miss"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			ZeroDamageHost.GetTerminalReceipt(), ZeroDamageFeedback)
			&& ZeroDamageFeedback.GetKind()
				== Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::Impact
			&& ZeroDamageFeedback.GetAppliedDamage() == 0.0f
			&& !ZeroDamageFeedback.DidDefeatTarget()
			&& ZeroDamageFeedback.GetDisplayText()
				== TEXT("飞刀 · 未造成伤害"));
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
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation BlockingFeedback;
	TestTrue(TEXT("Blocking geometry is visible without fabricated damage"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			ContactHost.GetTerminalReceipt(), BlockingFeedback)
			&& BlockingFeedback.GetKind()
				== Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::BlockingMiss
			&& BlockingFeedback.GetAppliedDamage() == 0.0f
			&& BlockingFeedback.GetDisplayText() == TEXT("飞刀 · 命中阻挡"));

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
	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation RangeFeedback;
	TestTrue(TEXT("Range expiry projects a distinct terminal message"),
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation::TryProject(
			RangeHost.GetTerminalReceipt(), RangeFeedback)
			&& RangeFeedback.GetKind()
				== Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::RangeExpired
			&& RangeFeedback.GetDisplayText() == TEXT("飞刀 · 超出射程"));
	TestFalse(TEXT("A terminal range callback cannot run twice"),
		RangeHost.TryExpireRange());
	return true;
}

#endif
