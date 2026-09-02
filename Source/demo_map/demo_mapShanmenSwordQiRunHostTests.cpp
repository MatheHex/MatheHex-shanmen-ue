#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiRunHost.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	const FGuid SwordQiHostRunId(0xD3820001, 0, 0, 1);
	const FGuid SwordQiHostOwnerId(0xD3820002, 0, 0, 1);
	const FGuid SwordQiHostItemId(0xD3820003, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindSwordQiHostTargetDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeSwordQiHostTargetIdentity(
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

	struct FSwordQiHostFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FSwordQiHostFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P182PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P182PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindSwordQiHostTargetDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P182EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}

			Enemy->AddInstanceComponent(EnemyIdentity);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeSwordQiHostTargetIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					SwordQiHostRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	struct FSwordQiSpawnRunFixture
	{
		UWorld* World = nullptr;
		APawn* Pawn = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FSwordQiSpawnRunFixture()
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
			Pawn = World->SpawnActor<APawn>();
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P182WorldPlayerHealth"))
				: nullptr;
			bReady = Pawn
				&& PlayerHealth
				&& Coordinator.TryBeginRun(
					SwordQiHostRunId, Pawn, PlayerHealth, Diagnostic);
		}

		~FSwordQiSpawnRunFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	FShanmenCombatActionSnapshot MakeSwordQiHostAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		uint64 ActivationSequence)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Coordinator.GetRunId();
		Capture.OwnerId = SwordQiHostOwnerId;
		Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
		Capture.SourceItemInstanceId = SwordQiHostItemId;
		Capture.ActionDefinitionId =
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P18.2");
		Capture.Content.Digest = TEXT("SwordQi.RunHost.r1");
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

	FShanmenSwordQiDefinition MakeSwordQiHostDefinition()
	{
		FShanmenSwordQiDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.SwordQi.P18.2.Host");
		Capture.FormulaId = TEXT("Formula.SwordQi.P18.2.Host");
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

	void StartSwordQiHostAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		uint64 ActivationSequence,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenSwordQiExecution& OutExecution)
	{
		const FShanmenCombatActionSnapshot Action =
			MakeSwordQiHostAction(Coordinator, ActivationSequence);
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Transition));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Transition));
		FShanmenSwordQiOffenseSnapshot Offense;
		check(FShanmenSwordQiOffenseSnapshot::TryCapture(20.0f, Offense));
		check(FShanmenSwordQiExecution::TryCreate(
			Action, MakeSwordQiHostDefinition(), Offense, OutExecution));
	}

	bool StartHostedSwordQi(
		FSwordQiHostFixture& Fixture,
		uint64 ActivationSequence,
		Fdemo_mapShanmenSwordQiRunHost& OutHost,
		Ademo_mapShanmenSwordQiProjectile*& OutProjectile)
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenSwordQiExecution Execution;
		StartSwordQiHostAction(
			Fixture.Coordinator, ActivationSequence, Runtime, Execution);
		OutProjectile = NewObject<Ademo_mapShanmenSwordQiProjectile>(
			GetTransientPackage());
		if (!OutProjectile)
		{
			return false;
		}
		return OutHost.TryLaunchCarrier(
			Runtime,
			Execution,
			*OutProjectile,
			Fixture.Coordinator,
			Fixture.Pawn,
			FVector(10.0, 20.0, 60.0),
			FVector::ForwardVector,
			false).IsStarted();
	}

	FHitResult MakeSwordQiHostEnemyHit(const FSwordQiHostFixture& Fixture)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiRunHostSpawnTest,
	"Shanmen.0_0_10.Product.SwordQiRunHost.SpawnLaunchAndOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiRunHostSpawnTest::RunTest(const FString&)
{
	AActor* DetachedSource = NewObject<AActor>(GetTransientPackage());
	const Fdemo_mapShanmenSwordQiSpawnResult NoWorld =
		Fdemo_mapShanmenSwordQiRunHost::SpawnStagedCarrier(
			nullptr,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			DetachedSource,
			FVector::ZeroVector);
	TestTrue(TEXT("A missing World cannot create a physical carrier"),
		NoWorld.Error == Edemo_mapShanmenSwordQiSpawnError::WorldUnavailable
			&& !NoWorld.IsSpawned());

	FSwordQiSpawnRunFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.2 spawn/run fixture."));
		return false;
	}
	FShanmenActionOrchestrator Runtime;
	FShanmenSwordQiExecution Execution;
	StartSwordQiHostAction(Fixture.Coordinator, 1, Runtime, Execution);
	Fdemo_mapShanmenSwordQiRunHost Host;
	const Fdemo_mapShanmenSwordQiHostStartResult Started =
		Host.TrySpawnAndLaunch(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Runtime,
			Execution,
			Fixture.Coordinator,
			Fixture.Pawn,
			FVector(25.0, 40.0, 60.0),
			FVector::ForwardVector);
	TestTrue(TEXT("The product start creates and adopts one exact carrier"),
		Started.IsStarted()
			&& Started.Spawn.Error
				== Edemo_mapShanmenSwordQiSpawnError::None
			&& Started.Spawn.Projectile.IsValid()
			&& Host.IsValid()
			&& Host.IsInFlight()
			&& Host.GetProjectile() == Started.Spawn.Projectile.Get()
			&& FMath::IsNearlyEqual(Host.GetMaximumDistance(), 1400.0f));
	TestTrue(TEXT("Frozen range and speed own the carrier lifetime"),
		Host.GetProjectile()
			&& FMath::IsNearlyEqual(
				Host.GetProjectile()->GetLifeSpan(),
				1400.0f / 900.0f,
				0.01f));
	TestFalse(TEXT("An active host cannot reset"), Host.Reset());
	TestTrue(TEXT("Explicit interruption retires Actor and Action once"),
		Host.TryInterrupt()
			&& Host.IsValid()
			&& Host.IsTerminal()
			&& Host.GetExecution().GetState()
				== EShanmenSwordQiState::Dissipated
			&& Host.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& Host.GetTerminalReceipt().Kind
				== Edemo_mapShanmenSwordQiTerminalKind::Interrupted);
	TestTrue(TEXT("A terminal host can release its transient binding"),
		Host.Reset()
			&& Host.GetState() == Edemo_mapShanmenSwordQiHostState::Empty
			&& !Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiRunHostImpactTest,
	"Shanmen.0_0_10.Product.SwordQiRunHost.ImpactLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiRunHostImpactTest::RunTest(const FString&)
{
	FSwordQiHostFixture Fixture;
	Fdemo_mapShanmenSwordQiRunHost Host;
	Ademo_mapShanmenSwordQiProjectile* Projectile = nullptr;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !StartHostedSwordQi(Fixture, 1, Host, Projectile))
	{
		AddError(TEXT("Could not build the P18.2 impact fixture."));
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

	Projectile->OnContact().Broadcast(
		*Projectile, MakeSwordQiHostEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot After;
	VitalityHost->TryCaptureCombatVitalitySnapshot(After);
	TestTrue(TEXT("Native contact reaches canonical vitality exactly once"),
		Host.IsValid()
			&& Host.IsTerminal()
			&& Host.GetTerminalReceipt().Kind
				== Edemo_mapShanmenSwordQiTerminalKind::Impact
			&& Host.GetTerminalReceipt().Delivery.IsDelivered()
			&& FMath::IsNearlyEqual(
				Before.CurrentVitality - After.CurrentVitality,
				0.7f,
				KINDA_SMALL_NUMBER)
			&& After.AuthorityRevision == Before.AuthorityRevision + 1);
	TestTrue(TEXT("First blocking impact closes flight and Action together"),
		Host.GetExecution().GetState() == EShanmenSwordQiState::Dissipated
			&& Host.GetExecution().NumAcceptedImpacts() == 1
			&& Host.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed
			&& Projectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated);

	Projectile->OnContact().Broadcast(
		*Projectile, MakeSwordQiHostEnemyHit(Fixture));
	FShanmenTargetVitalitySnapshot AfterReplay;
	VitalityHost->TryCaptureCombatVitalitySnapshot(AfterReplay);
	TestTrue(TEXT("Terminal publication removes the contact delegate"),
		AfterReplay.AuthorityRevision == After.AuthorityRevision
			&& FMath::IsNearlyEqual(
				AfterReplay.CurrentVitality, After.CurrentVitality));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiRunHostTerminalTest,
	"Shanmen.0_0_10.Product.SwordQiRunHost.MissRangeAndFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSwordQiRunHostTerminalTest::RunTest(const FString&)
{
	FSwordQiHostFixture Fixture;
	Fdemo_mapShanmenSwordQiRunHost BlockingHost;
	Ademo_mapShanmenSwordQiProjectile* BlockingProjectile = nullptr;
	if (!Fixture.bReady
		|| !StartHostedSwordQi(
			Fixture, 1, BlockingHost, BlockingProjectile))
	{
		return false;
	}
	AActor* Unregistered = NewObject<AActor>(GetTransientPackage());
	UBoxComponent* Root = Unregistered
		? NewObject<UBoxComponent>(Unregistered, TEXT("P182BlockingRoot"))
		: nullptr;
	if (!Unregistered || !Root)
	{
		return false;
	}
	Unregistered->SetRootComponent(Root);
	FHitResult Blocking(
		Unregistered,
		Root,
		FVector(500.0, 0.0, 55.0),
		FVector::BackwardVector);
	Blocking.ImpactPoint = FVector(500.0, 0.0, 55.0);
	Blocking.ImpactNormal = FVector::BackwardVector;
	BlockingProjectile->OnContact().Broadcast(*BlockingProjectile, Blocking);
	TestTrue(TEXT("Unresolved blocking contact completes without damage"),
		BlockingHost.IsValid()
			&& BlockingHost.GetTerminalReceipt().Kind
				== Edemo_mapShanmenSwordQiTerminalKind::BlockingMiss
			&& BlockingHost.GetTerminalReceipt().Delivery.Error
				== Edemo_mapShanmenSwordQiWorldDeliveryError::ContactNotResolved
			&& BlockingHost.GetExecution().NumAcceptedImpacts() == 0
			&& BlockingHost.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed);

	Fdemo_mapShanmenSwordQiRunHost RangeHost;
	Ademo_mapShanmenSwordQiProjectile* RangeProjectile = nullptr;
	if (!StartHostedSwordQi(Fixture, 2, RangeHost, RangeProjectile))
	{
		return false;
	}
	TestTrue(TEXT("Range expiry uses the same explicit no-impact terminal"),
		RangeHost.TryExpireRange()
			&& RangeHost.IsValid()
			&& RangeHost.GetTerminalReceipt().Kind
				== Edemo_mapShanmenSwordQiTerminalKind::RangeExpired
			&& RangeHost.GetExecution().NumAcceptedImpacts() == 0
			&& RangeProjectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated);
	TestFalse(TEXT("A terminal range callback cannot run twice"),
		RangeHost.TryExpireRange());

	Ademo_mapShanmenSwordQiProjectile* AbandonedProjectile = nullptr;
	{
		Fdemo_mapShanmenSwordQiRunHost ScopedHost;
		if (!StartHostedSwordQi(
			Fixture, 3, ScopedHost, AbandonedProjectile))
		{
			return false;
		}
	}
	TestTrue(TEXT("Destroying an active Host fails closed on GameThread"),
		AbandonedProjectile
			&& AbandonedProjectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::Dissipated);
	return true;
}

#endif
