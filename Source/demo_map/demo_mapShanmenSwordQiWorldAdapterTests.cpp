#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiWorldAdapter.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "demo_mapHUD.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordQiAvailabilityCommandRouter.h"

namespace
{
	const FGuid SwordQiWorldRunId(0xD3810001, 0, 0, 1);
	const FGuid SwordQiWorldOwnerId(0xD3810002, 0, 0, 1);
	const FGuid SwordQiWorldItemId(0xD3810003, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindSwordQiTargetDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeSwordQiTargetIdentity(
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

	struct FSwordQiWorldFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FSwordQiWorldFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P181PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P181PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindSwordQiTargetDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P181EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}

			Enemy->AddInstanceComponent(EnemyIdentity);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeSwordQiTargetIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					SwordQiWorldRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	struct FSwordQiCollisionWorldFixture
	{
		UWorld* World = nullptr;
		APawn* Source = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		AActor* Blocker = nullptr;
		UBoxComponent* BlockerRoot = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;

		FSwordQiCollisionWorldFixture()
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
			PlayerHealth = Source
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Source, TEXT("P245WorldPlayerHealth"), RF_Transient)
				: nullptr;
			Blocker = World->SpawnActor<AActor>(
				AActor::StaticClass(),
				FTransform(FVector(1000.0f, 1000.0f, 100.0f)),
				Parameters);
			BlockerRoot = Blocker
				? NewObject<UBoxComponent>(
					Blocker, TEXT("P245SwordQiBlocker"), RF_Transient)
				: nullptr;
			if (!Source || !PlayerHealth || !Blocker || !BlockerRoot)
			{
				return;
			}
			Blocker->SetRootComponent(BlockerRoot);
			Blocker->AddInstanceComponent(BlockerRoot);
			BlockerRoot->InitBoxExtent(FVector(1.0f, 24.0f, 24.0f));
			BlockerRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			BlockerRoot->SetCollisionObjectType(ECC_WorldStatic);
			BlockerRoot->SetCollisionResponseToAllChannels(ECR_Block);
			BlockerRoot->RegisterComponent();
			World->UpdateWorldComponents(true, false);
			World->InitializeActorsForPlay(FURL());
			Coordinator.TryBeginRun(
				SwordQiWorldRunId, Source, PlayerHealth, Diagnostic);
		}

		~FSwordQiCollisionWorldFixture()
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
			return World && Source && PlayerHealth && Blocker && BlockerRoot
				&& Coordinator.IsReady();
		}
	};

	FShanmenCombatActionSnapshot MakeSwordQiWorldAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		uint64 ActivationSequence = 1)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Coordinator.GetRunId();
		Capture.OwnerId = SwordQiWorldOwnerId;
		Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
		Capture.SourceItemInstanceId = SwordQiWorldItemId;
		Capture.ActionDefinitionId =
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P18.1");
		Capture.Content.Digest = TEXT("SwordQi.WorldDelivery.r1");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSwordQiDefinition MakeSwordQiWorldDefinition()
	{
		FShanmenSwordQiDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.SwordQi.P18.1.Product");
		Capture.FormulaId = TEXT("Formula.SwordQi.P18.1.Product");
		Capture.BaseDamage = 0.5f;
		Capture.AttackPowerCoefficient = 0.01f;
		Capture.FlightSpeed = 900.0f;
		Capture.MaximumRange = 1400.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamageSpirit());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		FShanmenSwordQiDefinition Definition;
		check(FShanmenSwordQiDefinition::TryCapture(Capture, Definition));
		return Definition;
	}

	void StartSwordQiWorldAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenSwordQiExecution& OutExecution,
		uint64 ActivationSequence = 1)
	{
		const FShanmenCombatActionSnapshot Action =
			MakeSwordQiWorldAction(Coordinator, ActivationSequence);
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Transition));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Transition));
		FShanmenSwordQiOffenseSnapshot Offense;
		check(FShanmenSwordQiOffenseSnapshot::TryCapture(20.0f, Offense));
		check(FShanmenSwordQiExecution::TryCreate(
			Action, MakeSwordQiWorldDefinition(), Offense, OutExecution));
	}

	FHitResult MakeSwordQiEnemyHit(const FSwordQiWorldFixture& Fixture)
	{
		FHitResult Hit(
			Fixture.Enemy,
			Fixture.GetEnemyRoot(),
			FVector(240.0, 15.0, 55.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(240.0, 15.0, 55.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}

	bool StageAndPublishSwordQi(
		FSwordQiWorldFixture& Fixture,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenSwordQiExecution& OutExecution,
		Ademo_mapShanmenSwordQiProjectile*& OutProjectile,
		uint64 ActivationSequence = 1)
	{
		StartSwordQiWorldAction(
			Fixture.Coordinator,
			OutRuntime,
			OutExecution,
			ActivationSequence);
		OutProjectile = NewObject<Ademo_mapShanmenSwordQiProjectile>(
			GetTransientPackage());
		if (!OutProjectile)
		{
			return false;
		}
		const Fdemo_mapShanmenSwordQiLaunchResult Staged =
			Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
				OutRuntime,
				OutExecution,
				*OutProjectile,
				Fixture.Coordinator,
				Fixture.Pawn,
				FVector(10.0, 20.0, 60.0),
				FVector::ForwardVector);
		return Staged.IsStaged()
			&& Fdemo_mapShanmenSwordQiWorldAdapter::PublishStagedLaunch(
				OutRuntime,
				Staged.Plan,
				OutExecution,
				*OutProjectile);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiWorldLaunchGateTest,
	"Shanmen.0_0_10.Product.SwordQiWorldDelivery.LaunchGateAndFlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiWorldLaunchGateTest::RunTest(const FString&)
{
	FSwordQiWorldFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.1 launch fixture."));
		return false;
	}
	FShanmenActionOrchestrator Runtime;
	FShanmenSwordQiExecution Execution;
	StartSwordQiWorldAction(Fixture.Coordinator, Runtime, Execution);
	Ademo_mapShanmenSwordQiProjectile* Projectile =
		NewObject<Ademo_mapShanmenSwordQiProjectile>(GetTransientPackage());
	AActor* UnregisteredSource = NewObject<AActor>(GetTransientPackage());
	if (!Projectile || !UnregisteredSource)
	{
		return false;
	}
	UStaticMeshComponent* EnergyBladeVisual = nullptr;
	UPointLightComponent* FlightCueLight = nullptr;
	{
		TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
		Projectile->GetComponents(MeshComponents);
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent
				&& MeshComponent->GetFName()
					== TEXT("SwordQiEnergyBladeVisual"))
			{
				EnergyBladeVisual = MeshComponent;
			}
		}
		TInlineComponentArray<UPointLightComponent*> LightComponents;
		Projectile->GetComponents(LightComponents);
		for (UPointLightComponent* LightComponent : LightComponents)
		{
			if (LightComponent
				&& LightComponent->GetFName()
					== TEXT("SwordQiFlightCueLight"))
			{
				FlightCueLight = LightComponent;
			}
		}
	}
	if (!EnergyBladeVisual || !FlightCueLight)
	{
		AddError(TEXT("Sword-qi presentation components are missing."));
		return false;
	}
	UMaterialInstanceDynamic* EnergyBladeMaterial =
		Cast<UMaterialInstanceDynamic>(EnergyBladeVisual->GetMaterial(0));
	TestTrue(TEXT("Sword qi owns one collisionless authored energy blade"),
		EnergyBladeVisual->GetAttachParent()
			== Projectile->GetCollisionComponent()
			&& EnergyBladeVisual->GetStaticMesh()
			&& EnergyBladeVisual->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& !EnergyBladeVisual->GetGenerateOverlapEvents()
			&& EnergyBladeVisual->GetRelativeScale3D().Equals(
				FVector(0.72f, 0.12f, 0.03f),
				KINDA_SMALL_NUMBER)
			&& EnergyBladeMaterial
			&& Projectile->HasPresentationMaterialColor());
	TestTrue(TEXT("Empty carrier keeps every sword-qi cue hidden"),
		!EnergyBladeVisual->IsVisible()
			&& !FlightCueLight->IsVisible()
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());

	const Fdemo_mapShanmenSwordQiLaunchResult Rejected =
		Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			UnregisteredSource,
			FVector(10.0, 20.0, 60.0),
			FVector::ForwardVector);
	TestTrue(TEXT("Launch rejects a source outside the run registry"),
		Rejected.Error
			== Edemo_mapShanmenSwordQiLaunchError::SourceNotRegistered
			&& Execution.GetState() == EShanmenSwordQiState::Ready
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Empty
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());

	const Fdemo_mapShanmenSwordQiLaunchResult Staged =
		Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			Fixture.Pawn,
			FVector(10.0, 20.0, 60.0),
			FVector(4.0, 0.0, 0.0));
	TestTrue(TEXT("Exact registered sword owner stages canonical evidence"),
		Staged.IsStaged()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Staged);
	TestTrue(TEXT("Staging leaves collision, motion, and live execution inert"),
		Execution.GetState() == EShanmenSwordQiState::Ready
			&& !Execution.IsEmissionActive()
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& !Projectile->GetMovementComponent()->IsActive()
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());

	const bool bPublished = Staged.IsStaged()
		&& Fdemo_mapShanmenSwordQiWorldAdapter::PublishStagedLaunch(
			Runtime, Staged.Plan, Execution, *Projectile);
	TestTrue(TEXT("Publication atomically opens one physical flight"),
		bPublished
			&& Execution.GetState() == EShanmenSwordQiState::InFlight
			&& Execution.IsEmissionActive()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::InFlight
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::QueryOnly
			&& Projectile->GetMovementComponent()->IsActive()
			&& Projectile->GetMovementComponent()->Velocity.Equals(
				FVector::ForwardVector * 900.0f)
			&& Projectile->IsPresentationVisible()
			&& Projectile->IsFlightCueVisible()
			&& Projectile->GetFlightCueColor().Equals(
				FLinearColor(0.22f, 0.82f, 1.0f), 0.01f));
	TestTrue(TEXT("Sword qi has authored speed without gravity, bounce, or homing"),
		FMath::IsNearlyEqual(
			Projectile->GetMovementComponent()->InitialSpeed, 900.0f)
			&& FMath::IsNearlyEqual(
				Projectile->GetMovementComponent()->MaxSpeed, 900.0f)
			&& Projectile->GetMovementComponent()->ProjectileGravityScale == 0.0f
			&& !Projectile->GetMovementComponent()->bShouldBounce
			&& !Projectile->GetMovementComponent()->bIsHomingProjectile);
	TestTrue(TEXT("Dissipation removes sword-qi geometry and light together"),
		Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
			Runtime, Execution, *Projectile)
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiLaunchCorridorGateTest,
	"Shanmen.0_0_10.Product.SwordQiWorldDelivery.LaunchCorridorGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiLaunchCorridorGateTest::RunTest(const FString&)
{
	FSwordQiCollisionWorldFixture Fixture;
	if (!Fixture.IsValid())
	{
		AddError(TEXT("Could not build the P24.5 launch-corridor fixture."));
		return false;
	}

	const FVector SourceOrigin = Fixture.Source->GetActorLocation();
	const FVector LaunchOrigin =
		SourceOrigin + (FVector::ForwardVector * 200.0f);
	FActorSpawnParameters Parameters;
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapShanmenSwordQiProjectile* Projectile =
		Fixture.World->SpawnActor<Ademo_mapShanmenSwordQiProjectile>(
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			FTransform(LaunchOrigin),
			Parameters);
	if (!Projectile)
	{
		AddError(TEXT("Could not spawn the P24.5 inert sword-qi carrier."));
		return false;
	}

	FShanmenActionOrchestrator Runtime;
	FShanmenSwordQiExecution Execution;
	StartSwordQiWorldAction(Fixture.Coordinator, Runtime, Execution, 11);
	Fixture.Blocker->SetActorLocation(
		LaunchOrigin,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Fixture.World->UpdateWorldComponents(true, false);
	const Fdemo_mapShanmenSwordQiLaunchResult Occupied =
		Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			Fixture.Source,
			LaunchOrigin,
			FVector::ForwardVector);
	TestTrue(TEXT("A blocking launch volume rejects sword qi before publication"),
		!Occupied.IsStaged()
			&& Occupied.Error
				== Edemo_mapShanmenSwordQiLaunchError::LaunchPathBlocked);
	Fdemo_mapShanmenSwordQiAvailabilityCommandResult FeedbackResult;
	FeedbackResult.Status =
		Edemo_mapShanmenSwordQiAvailabilityCommandStatus::Dispatched;
	FeedbackResult.CommandEvent.Status =
		Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected;
	FeedbackResult.CommandEvent.Input.Status =
		Edemo_mapShanmenSwordQiInputStatus::ProductRejected;
	FeedbackResult.CommandEvent.Input.Product.Status =
		Edemo_mapShanmenSwordQiControllerStatus::RouteRejected;
	FeedbackResult.CommandEvent.Input.Product.Route.Status =
		Edemo_mapShanmenSwordQiProductRouteStatus::LaunchRejectedInterrupted;
	FeedbackResult.CommandEvent.Input.Product.Route.HostStart.Error =
		Edemo_mapShanmenSwordQiHostStartError::LaunchRejected;
	FeedbackResult.CommandEvent.Input.Product.Route.HostStart.Launch = Occupied;
	FeedbackResult.Diagnostic = TEXT("Sword Qi launch path is blocked.");
	FString FeedbackText;
	FLinearColor FeedbackColor;
	TestTrue(TEXT("The exact launch-obstruction proof reaches the player HUD"),
		Ademo_mapHUD::TryBuildSwordQiFeedback(
			FeedbackResult,
			TEXT("B"),
			FeedbackText,
			FeedbackColor)
			&& FeedbackText == TEXT("SWORD QI · LAUNCH BLOCKED")
			&& FeedbackColor.Equals(
				FLinearColor(1.0f, 0.72f, 0.18f), 0.001f));
	Fdemo_mapShanmenSwordQiAvailabilityCommandResult BrokenProof =
		FeedbackResult;
	BrokenProof.CommandEvent.Input.Product.Route.HostStart.Error =
		Edemo_mapShanmenSwordQiHostStartError::AdoptionRejected;
	TestTrue(TEXT("An incomplete launch-obstruction chain cannot claim a blocked path"),
		Ademo_mapHUD::TryBuildSwordQiFeedback(
			BrokenProof,
			TEXT("B"),
			FeedbackText,
			FeedbackColor)
			&& FeedbackText == TEXT("SWORD QI · UNAVAILABLE"));
	TestTrue(TEXT("Blocked launch keeps execution and every carrier cue inert"),
		Execution.GetState() == EShanmenSwordQiState::Ready
			&& !Execution.IsEmissionActive()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Empty
			&& !Projectile->GetLaunchReceipt().IsValid()
			&& !Projectile->GetHitContext().IsValid()
			&& Projectile->GetCollisionComponent()->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& !Projectile->GetMovementComponent()->IsActive()
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());

	Fixture.Blocker->SetActorLocation(
		SourceOrigin + (FVector::ForwardVector * 100.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Fixture.World->UpdateWorldComponents(true, false);
	const Fdemo_mapShanmenSwordQiLaunchResult CorridorBlocked =
		Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			Fixture.Source,
			LaunchOrigin,
			FVector::ForwardVector);
	TestTrue(TEXT("A clear endpoint beyond a thin wall still rejects sword qi"),
		!CorridorBlocked.IsStaged()
			&& CorridorBlocked.Error
				== Edemo_mapShanmenSwordQiLaunchError::LaunchPathBlocked
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Empty);

	Fixture.Blocker->SetActorLocation(
		FVector(1000.0f, 1000.0f, 100.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Fixture.World->UpdateWorldComponents(true, false);
	const Fdemo_mapShanmenSwordQiLaunchResult Clear =
		Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			Fixture.Source,
			LaunchOrigin,
			FVector::ForwardVector);
	TestTrue(TEXT("The unchanged sword-qi launch stages after its path clears"),
		Clear.IsStaged()
			&& Projectile->IsStagedFor(Clear.Plan.Launch, Clear.Plan.Context));
	TestTrue(TEXT("A clear staged retry remains safely cancellable"),
		Projectile->CancelStagedLaunch()
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Empty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiWorldContactTest,
	"Shanmen.0_0_10.Product.SwordQiWorldDelivery.ContactToVitalityAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiWorldContactTest::RunTest(const FString&)
{
	FSwordQiWorldFixture Fixture;
	FShanmenActionOrchestrator Runtime;
	FShanmenSwordQiExecution Execution;
	Ademo_mapShanmenSwordQiProjectile* Projectile = nullptr;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !StageAndPublishSwordQi(
			Fixture, Runtime, Execution, Projectile))
	{
		AddError(TEXT("Could not build the P18.1 delivery fixture."));
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

	const Fdemo_mapShanmenSwordQiWorldDeliveryResult Delivered =
		Fdemo_mapShanmenSwordQiWorldAdapter::ResolveProjectileContact(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			MakeSwordQiEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot After;
	VitalityHost->TryCaptureCombatVitalitySnapshot(After);
	TestTrue(TEXT("World contact reaches the canonical vitality authority"),
		Delivered.IsDelivered());
	TestTrue(TEXT("Frozen sword-qi formula commits exactly 0.7 vitality"),
		FMath::IsNearlyEqual(Delivered.GetNewlyCommittedDamage(), 0.7f)
			&& FMath::IsNearlyEqual(
				Before.CurrentVitality - After.CurrentVitality,
				0.7f,
				KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Authority advances exactly once while flight stays open"),
		After.AuthorityRevision == Before.AuthorityRevision + 1
			&& Execution.GetState() == EShanmenSwordQiState::InFlight
			&& Execution.IsEmissionActive()
			&& Execution.NumAcceptedImpacts() == 1
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::InFlight);

	const Fdemo_mapShanmenSwordQiWorldDeliveryResult Replay =
		Fdemo_mapShanmenSwordQiWorldAdapter::ResolveProjectileContact(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			MakeSwordQiEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot AfterReplay;
	VitalityHost->TryCaptureCombatVitalitySnapshot(AfterReplay);
	TestTrue(TEXT("Same target callback is idempotently rejected"),
		Replay.Error
			== Edemo_mapShanmenSwordQiWorldDeliveryError::CandidateRejected
			&& AfterReplay.AuthorityRevision == After.AuthorityRevision
			&& FMath::IsNearlyEqual(
				AfterReplay.CurrentVitality, After.CurrentVitality));
	TestTrue(TEXT("Explicit range or geometry terminal closes the flight"),
		Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
			Runtime, Execution, *Projectile)
			&& Execution.GetState() == EShanmenSwordQiState::Dissipated
			&& !Execution.IsEmissionActive()
			&& Execution.NumAcceptedImpacts() == 1
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiWorldFailClosedTest,
	"Shanmen.0_0_10.Product.SwordQiWorldDelivery.FailClosedRangeAndTermination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiWorldFailClosedTest::RunTest(const FString&)
{
	FSwordQiWorldFixture Fixture;
	FShanmenActionOrchestrator Runtime;
	FShanmenSwordQiExecution Execution;
	Ademo_mapShanmenSwordQiProjectile* Projectile = nullptr;
	if (!Fixture.bReady
		|| !StageAndPublishSwordQi(
			Fixture, Runtime, Execution, Projectile))
	{
		return false;
	}

	AActor* Unregistered = NewObject<AActor>(GetTransientPackage());
	UBoxComponent* UnregisteredRoot = Unregistered
		? NewObject<UBoxComponent>(Unregistered, TEXT("P181UnregisteredRoot"))
		: nullptr;
	if (!Unregistered || !UnregisteredRoot)
	{
		return false;
	}
	Unregistered->SetRootComponent(UnregisteredRoot);
	FHitResult UnregisteredHit(
		Unregistered,
		UnregisteredRoot,
		FVector(300.0, 0.0, 55.0),
		FVector::BackwardVector);
	UnregisteredHit.ImpactPoint = FVector(300.0, 0.0, 55.0);
	UnregisteredHit.ImpactNormal = FVector::BackwardVector;
	const Fdemo_mapShanmenSwordQiWorldDeliveryResult Rejected =
		Fdemo_mapShanmenSwordQiWorldAdapter::ResolveProjectileContact(
			Runtime,
			Execution,
			*Projectile,
			Fixture.Coordinator,
			UnregisteredHit);
	TestTrue(TEXT("Unregistered contact fails without mutating flight"),
		Rejected.Error
			== Edemo_mapShanmenSwordQiWorldDeliveryError::ContactNotResolved
			&& Execution.GetState() == EShanmenSwordQiState::InFlight
			&& Execution.IsEmissionActive()
			&& Execution.NumAcceptedImpacts() == 0
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::InFlight
			&& Projectile->IsPresentationVisible()
			&& Projectile->IsFlightCueVisible());
	TestTrue(TEXT("Range expiry has an explicit no-impact terminal"),
		Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
			Runtime, Execution, *Projectile)
			&& Execution.GetState() == EShanmenSwordQiState::Dissipated
			&& Execution.NumAcceptedImpacts() == 0
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated
			&& !Projectile->IsPresentationVisible()
			&& !Projectile->IsFlightCueVisible());

	FShanmenActionOrchestrator InterruptedRuntime;
	FShanmenSwordQiExecution InterruptedExecution;
	Ademo_mapShanmenSwordQiProjectile* InterruptedProjectile = nullptr;
	if (!StageAndPublishSwordQi(
		Fixture,
		InterruptedRuntime,
		InterruptedExecution,
		InterruptedProjectile,
		2))
	{
		return false;
	}
	FShanmenActionTransitionReceipt Transition;
	TestTrue(TEXT("Action interruption reaches a terminal phase"),
		InterruptedRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active, Transition));
	TestTrue(TEXT("Terminal cleanup closes execution and carrier together"),
		Fdemo_mapShanmenSwordQiWorldAdapter::EndForActionTermination(
			InterruptedExecution, *InterruptedProjectile)
			&& InterruptedExecution.GetState()
				== EShanmenSwordQiState::Dissipated
			&& !InterruptedExecution.IsEmissionActive()
			&& InterruptedProjectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated
			&& !InterruptedProjectile->IsPresentationVisible()
			&& !InterruptedProjectile->IsFlightCueVisible());
	TestFalse(TEXT("Terminal cleanup cannot publish twice"),
		Fdemo_mapShanmenSwordQiWorldAdapter::EndForActionTermination(
			InterruptedExecution, *InterruptedProjectile));
	return true;
}

#endif
