#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponRunHost.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenControlledWeaponActor.h"
#include "demo_mapShanmenControlledWeaponThreatReadoutPresentation.h"

namespace
{
	const FGuid HostRunId(0xD3650001, 0, 0, 1);
	const FGuid HostLowItemId(0xD3650002, 0, 0, 1);
	const FGuid HostHighItemId(0xD3650003, 0, 0, 1);
	const FGuid HostThirdItemId(0xD3650004, 0, 0, 1);
	const FGuid HostOwnerId(0xD3650010, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindHostEnemyDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeHostEncounterIdentity(
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

	struct FControlledWeaponHostFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		TArray<AActor*> Weapons;
		TArray<UBoxComponent*> WeaponRoots;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FControlledWeaponHostFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P65PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P65PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}

			for (int32 Index = 0; Index < 3; ++Index)
			{
				AActor* Weapon = NewObject<AActor>(GetTransientPackage());
				UBoxComponent* Root = Weapon
					? NewObject<UBoxComponent>(
						Weapon,
						FName(*FString::Printf(
							TEXT("P65WeaponRoot%d"), Index)))
					: nullptr;
				if (Weapon && Root)
				{
					Weapon->SetRootComponent(Root);
					Weapon->SetActorLocation(
						FVector(10.0, 100.0 * Index, 30.0));
				}
				Weapons.Add(Weapon);
				WeaponRoots.Add(Root);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindHostEnemyDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P65EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| Weapons.Contains(nullptr)
				|| WeaponRoots.Contains(nullptr)
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}

			Enemy->AddInstanceComponent(EnemyIdentity);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeHostEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					HostRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	FShanmenContentStamp MakeHostContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.5");
		Content.Digest = TEXT("P6.5.ControlledWeaponRunHost.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakeHostPrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence)
	{
		const uint32 SequenceBits = static_cast<uint32>(ActivationSequence);
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId =
			FGuid(0xD3650100 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = HostOwnerId;
		Prepared.Evidence.ItemInstanceId = ItemInstanceId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.5");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3650200 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 13;
		Prepared.Evidence.ItemRevision = 7;
		Prepared.Evidence.Content = MakeHostContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = HostOwnerId;
		ActionCapture.SourceEntityId = Coordinator.GetPlayerEntityId();
		ActionCapture.SourceItemInstanceId = ItemInstanceId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeHostContent();
		ActionCapture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		ActionCapture.SourceTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponFlyingSword());
		ActionCapture.ActivationId =
			FShanmenCombatIdFactory::MakeActivationId(
				ActionCapture.RunId,
				ActionCapture.SourceEntityId,
				ActionCapture.ActionDefinitionId,
				ActivationSequence);
		check(FShanmenCombatActionSnapshot::TryCapture(
			ActionCapture, Prepared.Action));

		FShanmenControlledWeaponDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.DetectorId = FName(*FString::Printf(
			TEXT("Detector.ControlledWeapon.P6.5.%lld"),
			ActivationSequence));
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.5.HostTest");
		DefinitionCapture.BaseDamage = 0.5f;
		DefinitionCapture.ControlPowerCoefficient = 0.01f;
		DefinitionCapture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		DefinitionCapture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		check(FShanmenControlledWeaponDefinition::TryCapture(
			DefinitionCapture, Prepared.Definition));
		check(FShanmenControlledWeaponOffenseSnapshot::TryCapture(
			40.0f, Prepared.Offense));
		check(FShanmenControlledWeaponExecution::TryCreate(
			Prepared.Action,
			Prepared.Definition,
			Prepared.Offense,
			Prepared.Execution));
		check(Prepared.IsPrepared());
		return Prepared;
	}

	Fdemo_mapShanmenControlledWeaponMotionCapture MakeHostMotion(
		float InitialOrbitPhaseRadians = 0.0f)
	{
		Fdemo_mapShanmenControlledWeaponMotionCapture Motion;
		Motion.DirectedSpeed = 400.0f;
		Motion.OrbitCenterOffset = FVector(0.0, 0.0, 50.0);
		Motion.OrbitPlaneNormal = FVector::UpVector;
		Motion.OrbitReferenceAxis = FVector::ForwardVector;
		Motion.OrbitRadius = 100.0f;
		Motion.OrbitAngularSpeedRadiansPerSecond = UE_PI * 0.5f;
		Motion.InitialOrbitPhaseRadians = InitialOrbitPhaseRadians;
		Motion.MaximumStepSeconds = 0.5f;
		return Motion;
	}

	Fdemo_mapShanmenControlledWeaponHostAttachResult AttachHostWeapon(
		FControlledWeaponHostFixture& Fixture,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence,
		int32 WeaponIndex,
		float InitialOrbitPhaseRadians = 0.0f)
	{
		return Host.TryAttach(
			MakeHostPrepared(
				Fixture.Coordinator,
				ItemInstanceId,
				ActivationSequence),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[WeaponIndex],
			Fixture.WeaponRoots[WeaponIndex],
			MakeHostMotion(InitialOrbitPhaseRadians));
	}

	FHitResult MakeHostSweepHit(
		const FControlledWeaponHostFixture& Fixture)
	{
		FHitResult Hit(
			Fixture.Enemy,
			Fixture.GetEnemyRoot(),
			FVector(100.0, 20.0, 30.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(100.0, 20.0, 30.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}

	FOverlapResult MakeHostOverlap(
		const FControlledWeaponHostFixture& Fixture)
	{
		FOverlapResult Overlap;
		Overlap.OverlapObjectHandle = FActorInstanceHandle(Fixture.Enemy);
		Overlap.Component = Fixture.GetEnemyRoot();
		Overlap.ItemIndex = 0;
		return Overlap;
	}

	Fdemo_mapShanmenControlledWeaponOrbitThreatContact MakeHostThreatContact(
		const FControlledWeaponHostFixture& Fixture)
	{
		Fdemo_mapShanmenControlledWeaponOrbitThreatContact Contact;
		Contact.Overlap = MakeHostOverlap(Fixture);
		Contact.ContactLocation = FVector(80.0, 20.0, 30.0);
		Contact.ContactNormal = FVector::BackwardVector;
		return Contact;
	}

	struct FControlledWeaponDirectedWorldFixture
	{
		UWorld* World = nullptr;
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapShanmenControlledWeaponActor* Weapon = nullptr;
		UBoxComponent* WeaponRoot = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenControlledWeaponRunHost Host;
		FString Diagnostic;

		bool Start(FAutomationTestBase& Test)
		{
			const Fdemo_mapM01EnemyDefinition* Definition =
				FindHostEnemyDefinition();
			if (!GEngine || !Definition)
			{
				Test.AddError(TEXT(
					"P21.8 directed-flight fixture requires Engine and M01 content."));
				return false;
			}

			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				Test.AddError(TEXT("P21.8 could not allocate a preview World."));
				return false;
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
			Pawn = World->SpawnActor<APawn>(
				APawn::StaticClass(),
				FTransform(FVector(-1000.0f, 0.0f, 0.0f)),
				Parameters);
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("P218PlayerRoot"), RF_Transient)
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P218PlayerHealth"), RF_Transient)
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth)
			{
				Test.AddError(TEXT("P21.8 could not construct the source Pawn."));
				return false;
			}
			Pawn->SetRootComponent(PlayerRoot);
			Pawn->AddInstanceComponent(PlayerRoot);
			Pawn->AddInstanceComponent(PlayerHealth);
			PlayerRoot->RegisterComponent();
			PlayerHealth->RegisterComponent();

			Weapon = World->SpawnActor<Ademo_mapShanmenControlledWeaponActor>(
				Ademo_mapShanmenControlledWeaponActor::StaticClass(),
				FTransform::Identity,
				Parameters);
			WeaponRoot = Weapon ? Weapon->GetCollisionComponent() : nullptr;
			if (!Weapon || !WeaponRoot)
			{
				Test.AddError(TEXT("P21.8 could not construct the weapon Actor."));
				return false;
			}
			WeaponRoot->InitBoxExtent(FVector(10.0f));
			if (!Weapon->TryBindProductIdentity(
					HostRunId, HostLowItemId, Pawn))
			{
				Test.AddError(TEXT("P21.13 could not bind the weapon Actor."));
				return false;
			}
			WeaponRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			Enemy = World->SpawnActor<Ademo_mapEnemyCharacter>(
				Ademo_mapEnemyCharacter::StaticClass(),
				FTransform(FVector(120.0f, 0.0f, 0.0f)),
				Parameters);
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P218EnemyIdentity"), RF_Transient)
				: nullptr;
			if (!Enemy || !EnemyIdentity)
			{
				Test.AddError(TEXT("P21.8 could not construct the blocking enemy."));
				return false;
			}
			Enemy->AddInstanceComponent(EnemyIdentity);
			EnemyIdentity->RegisterComponent();
			Enemy->SetCombatSuppressed(true);
			if (!EnemyIdentity->Configure(*Definition)
				|| !Enemy->ConfigureEncounter(
					MakeHostEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				|| !Coordinator.TryBeginRun(
					HostRunId, Pawn, PlayerHealth, Diagnostic)
				|| !Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic)
				|| !Host.TryAttach(
					MakeHostPrepared(Coordinator, HostLowItemId, 1),
					Coordinator,
					Pawn,
					Weapon,
					WeaponRoot,
					MakeHostMotion()).IsAttached())
			{
				Test.AddError(FString::Printf(
					TEXT("P21.8 world binding failed: %s"), *Diagnostic));
				return false;
			}
			World->UpdateWorldComponents(true, false);
			return true;
		}

		void Stop()
		{
			Host.Reset();
			if (Coordinator.IsActive())
			{
				Coordinator.TryEndRun(HostRunId, Diagnostic);
			}
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
			}
			Pawn = nullptr;
			PlayerRoot = nullptr;
			PlayerHealth = nullptr;
			Weapon = nullptr;
			WeaponRoot = nullptr;
			Enemy = nullptr;
			EnemyIdentity = nullptr;
		}

		~FControlledWeaponDirectedWorldFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostStableOrderTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.StableIdentityAndOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostStableOrderTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady)
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.5 host fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	TestTrue(TEXT("Reverse attachment accepts two exact physical items"),
		AttachHostWeapon(Fixture, Host, HostHighItemId, 2, 1).IsAttached()
		&& AttachHostWeapon(Fixture, Host, HostLowItemId, 1, 0).IsAttached()
		&& Host.IsValid()
		&& Host.NumBound() == 2
		&& Host.NumActive() == 2);
	const TArray<FGuid> Ordered = Host.GetOrderedItemInstanceIds();
	TestTrue(TEXT("Stable item identity, not attachment order, owns batching"),
		Ordered.Num() == 2
		&& Ordered[0] == HostLowItemId
		&& Ordered[1] == HostHighItemId);

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Each exact item accepts an independent Launch sequence"),
		Host.TryLaunch(HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(HostHighItemId, 0, FVector::RightVector, Command));
	const FVector LowStart = Fixture.Weapons[0]->GetActorLocation();
	const FVector HighStart = Fixture.Weapons[1]->GetActorLocation();
	Fdemo_mapShanmenControlledWeaponHostMovementBatch Batch;
	TestTrue(TEXT("Directed swords advance in stable item order"),
		Host.TryAdvanceDirectedInOrder(0.1f, Batch)
		&& Batch.IsFullyAdvanced()
		&& Batch.Entries.Num() == 2
		&& Batch.Entries[0].ItemInstanceId == HostLowItemId
		&& Batch.Entries[1].ItemInstanceId == HostHighItemId
		&& Fixture.Weapons[0]->GetActorLocation().Equals(
			LowStart + FVector(40.0, 0.0, 0.0))
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighStart + FVector(0.0, 40.0, 0.0)));

	const FVector LowBeforeRejected =
		Fixture.Weapons[0]->GetActorLocation();
	const FVector HighBeforeRejected =
		Fixture.Weapons[1]->GetActorLocation();
	TestFalse(TEXT("Any oversized sample fails host preflight"),
		Host.TryAdvanceDirectedInOrder(0.75f, Batch));
	TestTrue(TEXT("Rejected batch moves none of the physical Actors"),
		Fixture.Weapons[0]->GetActorLocation().Equals(LowBeforeRejected)
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighBeforeRejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostFixedTimelineMovementTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.FixedTimelineDirectedMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostFixedTimelineMovementTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P21.8 fixed-timeline fixture."));
		return false;
	}

	FShanmenControlledWeaponCommandReceipt Launch;
	if (!Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch))
	{
		AddError(TEXT("P21.8 fixed-timeline fixture could not launch."));
		return false;
	}
	const FVector Start = Fixture.Weapons[0]->GetActorLocation();
	const FGuid TimelineId =
		Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(HostRunId);
	Fdemo_mapShanmenCombatRunTimelineSample TickZero;
	Fdemo_mapShanmenCombatRunTimelineSample TickThree;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 0, TickZero));
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 3, TickThree));

	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Zero =
		Host.AdvanceDirectedFixedTicks(
			TickZero, 0, Fixture.Coordinator);
	TestTrue(TEXT("Sub-tick owner frames cannot move a directed weapon"),
		Zero.IsNoOp()
		&& Zero.StartTick == 0
		&& Zero.EndTick == 0
		&& Fixture.Weapons[0]->GetActorLocation().Equals(Start));

	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Three =
		Host.AdvanceDirectedFixedTicks(
			TickThree, 3, Fixture.Coordinator);
	TestTrue(TEXT("Three canonical ticks perform three exact swept steps"),
		Three.IsAdvanced()
		&& Three.StartTick == 0
		&& Three.EndTick == 3
		&& Three.RequestedTickCount == 3
		&& Three.MovementTickCount == 3
		&& Three.MovementCount == 3
		&& Three.BlockingContactCount == 0
		&& Fixture.Weapons[0]->GetActorLocation().Equals(
			Start + FVector(40.0f, 0.0f, 0.0f), 0.01f));

	const FVector BeforeRejected = Fixture.Weapons[0]->GetActorLocation();
	Fdemo_mapShanmenCombatRunTimelineSample Foreign;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		FGuid(0xD365FFFF, 0, 0, 1), 4, Foreign));
	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Mismatch =
		Host.AdvanceDirectedFixedTicks(
			Foreign, 1, Fixture.Coordinator);
	TestTrue(TEXT("A foreign timeline fails before physical movement"),
		Mismatch.Error
			== Edemo_mapShanmenControlledWeaponDirectedTimelineError::
				CoordinatorMismatch
		&& Fixture.Weapons[0]->GetActorLocation().Equals(BeforeRejected));

	Fdemo_mapShanmenCombatRunTimelineSample OversizedSample;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 304, OversizedSample));
	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Oversized =
		Host.AdvanceDirectedFixedTicks(
			OversizedSample, 301, Fixture.Coordinator);
	TestTrue(TEXT("An abusive catch-up request is bounded before movement"),
		Oversized.Error
			== Edemo_mapShanmenControlledWeaponDirectedTimelineError::
				TickBudgetExceeded
		&& Fixture.Weapons[0]->GetActorLocation().Equals(BeforeRejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostFixedTimelineReturnTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.FixedTimelineVisibleReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostFixedTimelineReturnTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P21.9 fixed-timeline return fixture."));
		return false;
	}

	FShanmenControlledWeaponCommandReceipt Launch;
	if (!Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch))
	{
		AddError(TEXT("P21.9 return fixture could not launch."));
		return false;
	}
	const FGuid TimelineId =
		Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(HostRunId);
	Fdemo_mapShanmenCombatRunTimelineSample TickThree;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 3, TickThree));
	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Outbound =
		Host.AdvanceDirectedFixedTicks(
			TickThree, 3, Fixture.Coordinator);
	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	const FVector OutboundLocation = Fixture.Weapons[0]->GetActorLocation();
	TestTrue(TEXT("Existing lifecycle terminalizes before physical return"),
		Outbound.IsAdvanced()
		&& Host.TryRecallAndComplete(
			HostLowItemId, 1, Recall, Recovery, Completed)
		&& Host.FindController(HostLowItemId)
		&& Host.FindController(HostLowItemId)->IsCompletedForReturn()
		&& !Host.FindController(HostLowItemId)->IsAtInitialOrbitLocation());

	const Fdemo_mapShanmenControlledWeaponReturnTimelineResult Zero =
		Host.AdvanceCompletedReturnsFixedTicks(
			TickThree, 0, Fixture.Coordinator);
	TestTrue(TEXT("Sub-tick owner frames cannot move a completed weapon"),
		Zero.IsNoOp()
		&& Fixture.Weapons[0]->GetActorLocation().Equals(OutboundLocation));

	Fdemo_mapShanmenCombatRunTimelineSample Foreign;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		FGuid(0xD365FFFF, 0, 0, 2), 4, Foreign));
	const Fdemo_mapShanmenControlledWeaponReturnTimelineResult Mismatch =
		Host.AdvanceCompletedReturnsFixedTicks(
			Foreign, 1, Fixture.Coordinator);
	Fdemo_mapShanmenCombatRunTimelineSample OversizedSample;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 304, OversizedSample));
	const Fdemo_mapShanmenControlledWeaponReturnTimelineResult Oversized =
		Host.AdvanceCompletedReturnsFixedTicks(
			OversizedSample, 301, Fixture.Coordinator);
	TestTrue(TEXT("Foreign and abusive return pumps fail before movement"),
		Mismatch.Error
			== Edemo_mapShanmenControlledWeaponReturnTimelineError::
				CoordinatorMismatch
		&& Oversized.Error
			== Edemo_mapShanmenControlledWeaponReturnTimelineError::
				TickBudgetExceeded
		&& Fixture.Weapons[0]->GetActorLocation().Equals(OutboundLocation));

	Fdemo_mapShanmenCombatRunTimelineSample TickFive;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 5, TickFive));
	const Fdemo_mapShanmenControlledWeaponReturnTimelineResult Partial =
		Host.AdvanceCompletedReturnsFixedTicks(
			TickFive, 2, Fixture.Coordinator);
	const FVector PartialLocation = Fixture.Weapons[0]->GetActorLocation();
	TestTrue(TEXT("Canonical ticks visibly move the same terminal Actor home"),
		Partial.IsSuccess()
		&& Partial.ProcessedTickCount == 2
		&& Partial.MovementCount == 2
		&& Partial.ArrivedItemInstanceIds.IsEmpty()
		&& !PartialLocation.Equals(OutboundLocation)
		&& Host.FindController(HostLowItemId)->IsCompletedForReturn());

	Fdemo_mapShanmenCombatRunTimelineSample TickEight;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		TimelineId, 8, TickEight));
	const Fdemo_mapShanmenControlledWeaponReturnTimelineResult Arrived =
		Host.AdvanceCompletedReturnsFixedTicks(
			TickEight, 3, Fixture.Coordinator);
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Host.FindController(HostLowItemId);
	TestTrue(TEXT("Return snaps exactly once to the canonical initial anchor"),
		Arrived.IsSuccess()
		&& Arrived.HasArrivals()
		&& Arrived.ArrivedItemInstanceIds.Num() == 1
		&& Arrived.ArrivedItemInstanceIds[0] == HostLowItemId
		&& Controller
		&& Controller->IsAtInitialOrbitLocation()
		&& Controller->GetSession().IsTerminal()
		&& Fixture.Weapons[0]->GetActorLocation().Equals(
			FVector(100.0f, 0.f, 50.0f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostBlockingTimelineTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.BlockingContactTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostBlockingTimelineTest::RunTest(
	const FString&)
{
	FControlledWeaponDirectedWorldFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Fixture.Enemy->SetActorLocation(FVector(0.0f, 120.0f, 0.0f));
	Fixture.World->UpdateWorldComponents(true, false);
	FShanmenControlledWeaponCommandReceipt Launch;
	if (!Fixture.Host.TryLaunch(
			HostLowItemId, 0, FVector::RightVector, Launch))
	{
		AddError(TEXT("P21.8 blocking fixture could not launch."));
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Fixture.Host.FindController(HostLowItemId);
	Fdemo_mapShanmenControlledWeaponFlightReadModel DirectedReadModel;
	if (!Controller
		|| !Controller->TryCaptureFlightReadModel(
			false, DirectedReadModel)
		|| !Fixture.Weapon->TryPresentFlightReadModel(DirectedReadModel))
	{
		AddError(TEXT("P21.13 could not publish directed presentation."));
		return false;
	}
	const FLinearColor DirectedPresentationColor =
		Fixture.Weapon->GetResolvedPresentationColor();
	const FRotator CollisionActorRotation = Fixture.Weapon->GetActorRotation();

	const float VitalityBefore = Fixture.Enemy->GetCurrentVitality();
	const int32 ImpactsBefore = Fixture.Enemy->NumCommittedCombatImpacts();
	Fdemo_mapShanmenCombatRunTimelineSample TickFifteen;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(HostRunId),
		15,
		TickFifteen));
	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Blocked =
		Fixture.Host.AdvanceDirectedFixedTicks(
			TickFifteen, 15, Fixture.Coordinator);
	TestTrue(TEXT("A real swept enemy block delivers once and terminalizes"),
		Blocked.IsAdvanced()
		&& Blocked.MovementTickCount > 0
		&& Blocked.MovementTickCount < 15
		&& Blocked.BlockingContactCount == 1
		&& Blocked.DeliveredImpactCount == 1
		&& Blocked.TerminalizedCount == 1
		&& Blocked.FallbackInterruptedCount == 0
		&& Controller
		&& Controller->GetSession().IsTerminal()
		&& !Controller->HasActiveContactWindow()
		&& Fixture.Host.NumActive() == 0
		&& Fixture.Enemy->GetCurrentVitality() < VitalityBefore
		&& Fixture.Enemy->NumCommittedCombatImpacts() == ImpactsBefore + 1);
	TestEqual(TEXT("the timeline preserves one canonical delivery"),
		Blocked.DeliveredImpacts.Num(), 1);
	if (Blocked.DeliveredImpacts.Num() == 1)
	{
		const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult& Delivery =
			Blocked.DeliveredImpacts[0];
		TestTrue(TEXT("the preserved delivery remains self-valid"),
			Delivery.IsDelivered());
		TestTrue(TEXT("the preserved delivery retains the exact item"),
			Delivery.Impact.GetRequest().Action.GetSourceItemInstanceId()
				== HostLowItemId);
		TestTrue(TEXT("the preserved delivery retains the exact target"),
			Delivery.TargetEntityId == Fixture.Enemy->GetCombatEntityId());
		TestTrue(TEXT("the preserved delivery is the fresh commit"),
			Delivery.Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed);
		TestEqual(TEXT("the preserved damage matches target vitality"),
			Delivery.GetNewlyCommittedDamage(),
			VitalityBefore - Fixture.Enemy->GetCurrentVitality(),
			KINDA_SMALL_NUMBER);
	}

	Fdemo_mapShanmenControlledWeaponFlightReadModel ReturningReadModel;
	Fdemo_mapShanmenControlledWeaponThreatReadoutPlan ReturningPlan;
	const bool bPresentedImpact = Blocked.DeliveredImpacts.Num() == 1
		&& Fixture.Weapon->TryPresentCommittedImpactFeedback(
			Blocked.DeliveredImpacts[0]);
	const bool bPresentedReplay = Blocked.DeliveredImpacts.Num() == 1
		&& Fixture.Weapon->TryPresentCommittedImpactFeedback(
			Blocked.DeliveredImpacts[0]);
	const bool bCapturedReturningReadModel = Controller
		&& Controller->TryCaptureFlightReadModel(false, ReturningReadModel);
	const FVector PresentedTravelDirection = bCapturedReturningReadModel
		? (ReturningReadModel.GetWeaponLocation()
			- DirectedReadModel.GetWeaponLocation()).GetSafeNormal()
		: FVector::ZeroVector;
	TestTrue(TEXT("impact color waits for the canonical return phase"),
		bPresentedImpact
			&& bPresentedReplay
			&& !Fixture.Weapon->IsCommittedImpactCueActive()
			&& Fixture.Weapon->GetResolvedPresentationColor()
				== DirectedPresentationColor);
	TestTrue(TEXT("the same Actor projects the fresh commit during return"),
		bPresentedImpact
			&& bPresentedReplay
			&& bCapturedReturningReadModel
			&& ReturningReadModel.GetPhase()
				== Edemo_mapShanmenControlledWeaponFlightPhase::Returning
			&& Fixture.Weapon->TryPresentFlightReadModel(ReturningReadModel)
			&& Fixture.Weapon->HasCommittedImpactFeedback()
			&& Fixture.Weapon->IsCommittedImpactCueActive()
			&& Fixture.Weapon->GetResolvedPresentationColor()
				== FLinearColor(1.00f, 0.82f, 0.05f)
			&& Fixture.Weapon->GetPresentedImpactId()
				== Blocked.DeliveredImpacts[0].Impact.GetRequest().ImpactId
			&& Fixture.Weapon->GetPresentedImpactTargetEntityId()
				== Fixture.Enemy->GetCombatEntityId()
			&& Fdemo_mapShanmenControlledWeaponThreatReadoutPlan::TryPlan(
				FVector2D(1920.0, 1080.0),
				ReturningReadModel.GetPhase(),
				0,
				TEXT("X"),
				TEXT("C"),
				Fixture.Weapon->HasCommittedImpactFeedback(),
				Fixture.Weapon->GetPresentedImpactAppliedDamage(),
				Fixture.Weapon->DidPresentedImpactDefeatTarget(),
				true,
				FVector2D(960.0, 540.0),
				Fdemo_mapShanmenControlledWeaponThreatReadoutStyle(),
				ReturningPlan)
			&& ReturningPlan.GetText() == FString::Printf(
				TEXT("飞剑 · 返航 · 命中 -%.1f"),
				static_cast<double>(
					Fixture.Weapon->GetPresentedImpactAppliedDamage())));
	TestTrue(TEXT("the visual mesh follows travel without rotating collision"),
		PresentedTravelDirection.Equals(FVector::RightVector, 0.01f)
			&& Fixture.Weapon->GetPresentationForwardDirection().Equals(
				PresentedTravelDirection, 0.01f)
			&& Fixture.Weapon->GetActorRotation().Equals(
				CollisionActorRotation, 0.01f));

	Fdemo_mapShanmenControlledWeaponFlightReadModel NextActivationReadModel;
	const FGuid NextActivationId(0xD36500F1, 0, 0, 1);
	TestTrue(TEXT("a later activation clears the previous hit feedback"),
		Fdemo_mapShanmenControlledWeaponFlightReadModel::TryCapture(
			HostRunId,
			HostLowItemId,
			NextActivationId,
			Edemo_mapShanmenControlledWeaponFlightPhase::Orbiting,
			Fixture.Weapon->GetActorLocation(),
			ReturningReadModel.GetReturnAnchor(),
			NextActivationReadModel)
			&& Fixture.Weapon->TryPresentFlightReadModel(
				NextActivationReadModel)
			&& !Fixture.Weapon->HasCommittedImpactFeedback()
			&& !Fixture.Weapon->IsCommittedImpactCueActive()
			&& Fixture.Weapon->GetResolvedPresentationColor()
				!= FLinearColor(1.00f, 0.82f, 0.05f));

	Fdemo_mapShanmenCombatRunTimelineSample TickSixteen;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(HostRunId),
		16,
		TickSixteen));
	const FVector TerminalLocation = Fixture.Weapon->GetActorLocation();
	const Fdemo_mapShanmenControlledWeaponDirectedTimelineResult AfterTerminal =
		Fixture.Host.AdvanceDirectedFixedTicks(
			TickSixteen, 1, Fixture.Coordinator);
	TestTrue(TEXT("Later ticks cannot move or damage a terminal weapon again"),
		AfterTerminal.IsNoOp()
		&& AfterTerminal.DeliveredImpacts.IsEmpty()
		&& Fixture.Weapon->GetActorLocation().Equals(TerminalLocation)
		&& Fixture.Enemy->NumCommittedCombatImpacts() == ImpactsBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostOrbitOrderTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitOrderAndPreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostOrbitOrderTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not prepare P6.8 host orbit fixture."));
		return false;
	}
	Fixture.Pawn->SetActorLocation(FVector(50.0, 60.0, 10.0));
	if (!AttachHostWeapon(
			Fixture,
			Host,
			HostHighItemId,
			2,
			1,
			UE_PI).IsAttached()
		|| !AttachHostWeapon(
			Fixture,
			Host,
			HostLowItemId,
			1,
			0,
			0.0f).IsAttached())
	{
		AddError(TEXT("Could not attach P6.8 host orbit items."));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponHostOrbitBatch Orbit;
	TestTrue(TEXT("Host advances reverse-attached orbit items in stable order"),
		Host.TryAdvanceOrbitingInOrder(0.5f, Orbit)
		&& Orbit.IsFullyAdvanced()
		&& Orbit.Entries.Num() == 2
		&& Orbit.Entries[0].ItemInstanceId == HostLowItemId
		&& Orbit.Entries[1].ItemInstanceId == HostHighItemId
		&& FMath::IsNearlyEqual(
			Orbit.Entries[0].Movement.EndPhaseRadians,
			UE_PI * 0.25f)
		&& FMath::IsNearlyEqual(
			Orbit.Entries[1].Movement.EndPhaseRadians,
			UE_PI * 1.25f));

	const FVector LowBeforeRejected =
		Fixture.Weapons[0]->GetActorLocation();
	const FVector HighBeforeRejected =
		Fixture.Weapons[1]->GetActorLocation();
	const float LowPhaseBeforeRejected =
		Host.FindController(HostLowItemId)
			->GetCurrentOrbitPhaseRadians();
	const float HighPhaseBeforeRejected =
		Host.FindController(HostHighItemId)
			->GetCurrentOrbitPhaseRadians();
	TestFalse(TEXT("Any oversized orbit sample fails host preflight"),
		Host.TryAdvanceOrbitingInOrder(0.75f, Orbit));
	TestTrue(TEXT("Rejected orbit batch changes no transform or phase"),
		Fixture.Weapons[0]->GetActorLocation().Equals(LowBeforeRejected)
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighBeforeRejected)
		&& FMath::IsNearlyEqual(
			Host.FindController(HostLowItemId)
				->GetCurrentOrbitPhaseRadians(),
			LowPhaseBeforeRejected)
		&& FMath::IsNearlyEqual(
			Host.FindController(HostHighItemId)
				->GetCurrentOrbitPhaseRadians(),
			HighPhaseBeforeRejected));

	FShanmenControlledWeaponCommandReceipt Launch;
	TestTrue(TEXT("One exact item can leave Orbiting independently"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	TestTrue(TEXT("Later orbit batch advances only the remaining item"),
		Host.TryAdvanceOrbitingInOrder(0.1f, Orbit)
		&& Orbit.IsFullyAdvanced()
		&& Orbit.AttemptedCount == 1
		&& Orbit.Entries[0].ItemInstanceId == HostHighItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostDefenseReadinessTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitDefenseReadiness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostDefenseReadinessTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1, UE_PI).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0, 0.0f).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.23 defense-readiness fixture."));
		return false;
	}

	const TArray<FGuid> ReverseRequested = {
		HostHighItemId, HostLowItemId };
	Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch Batch;
	TestFalse(TEXT("Logical Orbiting alone cannot claim an unplaced world pose"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			ReverseRequested, Batch));
	TestFalse(TEXT("Rejected capture leaves no partial readiness evidence"),
		Batch.IsFullyCaptured());

	Fdemo_mapShanmenControlledWeaponHostOrbitBatch Orbit;
	TestTrue(TEXT("One explicit orbit step places both exact items"),
		Host.TryAdvanceOrbitingInOrder(0.1f, Orbit)
		&& Orbit.IsFullyAdvanced());
	TestTrue(TEXT("Caller-selected items capture atomically in stable GUID order"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			ReverseRequested, Batch)
		&& Batch.IsFullyCaptured()
		&& Batch.RunId == HostRunId
		&& Batch.SourceEntityId == Fixture.Coordinator.GetPlayerEntityId()
		&& Batch.RequestedCount == 2
		&& Batch.Entries[0].ItemInstanceId == HostLowItemId
		&& Batch.Entries[1].ItemInstanceId == HostHighItemId
		&& Host.IsOrbitDefenseReadinessCurrent(Batch));

	const FGuid LowPoseId = Batch.Entries[0].Readiness.GetSnapshotId();
	const FGuid HighPoseId = Batch.Entries[1].Readiness.GetSnapshotId();
	const FGuid LowRuntimeId = Batch.Entries[0].Readiness.GetRuntime()
		.GetReadinessId();
	const FGuid HighRuntimeId = Batch.Entries[1].Readiness.GetRuntime()
		.GetReadinessId();
	TestTrue(TEXT("Every entry proves the exact item and physical orbit equation"),
		LowPoseId.IsValid()
		&& HighPoseId.IsValid()
		&& LowPoseId != HighPoseId
		&& Batch.Entries[0].Readiness.GetWeaponLocation().Equals(
			Fixture.Weapons[0]->GetActorLocation())
		&& Batch.Entries[1].Readiness.GetWeaponLocation().Equals(
			Fixture.Weapons[1]->GetActorLocation()));

	Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch Rejected;
	const TArray<FGuid> DuplicateRequested = {
		HostLowItemId, HostLowItemId };
	TestFalse(TEXT("Duplicate exact-item participation fails closed"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			DuplicateRequested, Rejected));
	const TArray<FGuid> UnknownRequested = {
		FGuid(0xD36500FF, 0, 0, 1) };
	TestFalse(TEXT("Unknown exact item cannot enter readiness evidence"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			UnknownRequested, Rejected));
	TestFalse(TEXT("Empty participation policy is not inferred by the Host"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			TArray<FGuid>(), Rejected));
	TestFalse(TEXT("Every rejected subset clears its output"),
		Rejected.IsFullyCaptured());

	TestTrue(TEXT("A later orbit pose advances without changing command state"),
		Host.TryAdvanceOrbitingInOrder(0.1f, Orbit));
	TestFalse(TEXT("Prior physical pose evidence becomes stale after movement"),
		Host.IsOrbitDefenseReadinessCurrent(Batch));
	Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch Moved;
	TestTrue(TEXT("Fresh pose recaptures the same logical readiness checkpoint"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			ReverseRequested, Moved)
		&& Moved.Entries[0].Readiness.GetSnapshotId() != LowPoseId
		&& Moved.Entries[1].Readiness.GetSnapshotId() != HighPoseId
		&& Moved.Entries[0].Readiness.GetRuntime().GetReadinessId()
			== LowRuntimeId
		&& Moved.Entries[1].Readiness.GetRuntime().GetReadinessId()
			== HighRuntimeId);

	FShanmenControlledWeaponCommandReceipt Launch;
	TestTrue(TEXT("One exact item may leave readiness by launching"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	TestFalse(TEXT("Launch invalidates any batch containing that item"),
		Host.IsOrbitDefenseReadinessCurrent(Moved));
	TestFalse(TEXT("Mixed Orbiting and Directed subset fails atomically"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			ReverseRequested, Rejected));
	const TArray<FGuid> HighOnly = { HostHighItemId };
	Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch HighReady;
	TestTrue(TEXT("Caller may explicitly retain only the remaining Orbiting item"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			HighOnly, HighReady)
		&& HighReady.IsFullyCaptured()
		&& Host.IsOrbitDefenseReadinessCurrent(HighReady));

	Fixture.Pawn->SetActorLocation(FVector(25.0, -10.0, 5.0));
	TestFalse(TEXT("Moving the source anchor stales old pose evidence"),
		Host.IsOrbitDefenseReadinessCurrent(HighReady));
	TestFalse(TEXT("Unfollowed source anchor cannot produce fresh readiness"),
		Host.TryCaptureOrbitDefenseReadinessInOrder(
			HighOnly, Rejected));
	TestTrue(TEXT("Next explicit orbit step restores physical preparation"),
		Host.TryAdvanceOrbitingInOrder(0.1f, Orbit)
		&& Host.TryCaptureOrbitDefenseReadinessInOrder(
			HighOnly, HighReady)
		&& Host.IsOrbitDefenseReadinessCurrent(HighReady));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostFrameOwnerTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.FrameOwnerResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostFrameOwnerTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not prepare P6.9 frame-owner fixture."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult InvalidDelta =
		Host.AdvanceOrbitingFrame(0.0f);
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Empty =
		Host.AdvanceOrbitingFrame(0.1f);
	TestTrue(TEXT("Frame result separates invalid delta from an empty no-op"),
		InvalidDelta.IsValid()
		&& InvalidDelta.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::DeltaInvalid
		&& Empty.IsNoOp()
		&& !Empty.RunId.IsValid()
		&& Empty.BoundCount == 0);

	if (!AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1, UE_PI).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not attach P6.9 frame-owner items."));
		return false;
	}
	TestEqual(TEXT("Both newly attached items are Orbiting"),
		Host.NumOrbiting(), 2);

	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Advanced =
		Host.AdvanceOrbitingFrame(0.25f);
	TestTrue(TEXT("Owner frame advances all Orbiting items in one batch"),
		Advanced.IsAdvanced()
		&& Advanced.RunId == HostRunId
		&& Advanced.BoundCount == 2
		&& Advanced.OrbitingCount == 2
		&& Advanced.Batch.Entries.Num() == 2
		&& Advanced.Batch.Entries[0].ItemInstanceId == HostLowItemId
		&& Advanced.Batch.Entries[1].ItemInstanceId == HostHighItemId);

	const FVector LowBeforeRejected =
		Fixture.Weapons[0]->GetActorLocation();
	const FVector HighBeforeRejected =
		Fixture.Weapons[1]->GetActorLocation();
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Rejected =
		Host.AdvanceOrbitingFrame(0.75f);
	TestTrue(TEXT("Oversized owner frame is an auditable rejection"),
		Rejected.IsValid()
		&& Rejected.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::MovementRejected
		&& Rejected.RunId == HostRunId
		&& Rejected.BoundCount == 2
		&& Rejected.OrbitingCount == 2
		&& Rejected.Batch.AttemptedCount == 0
		&& Fixture.Weapons[0]->GetActorLocation().Equals(LowBeforeRejected)
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighBeforeRejected));

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Canonical Launch removes both items from Orbit cadence"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(
			HostHighItemId, 0, FVector::RightVector, Command)
		&& Host.NumOrbiting() == 0);
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult DirectedNoOp =
		Host.AdvanceOrbitingFrame(0.1f);
	TestTrue(TEXT("A valid Host with no Orbiting items is a clean no-op"),
		DirectedNoOp.IsNoOp()
		&& DirectedNoOp.RunId == HostRunId
		&& DirectedNoOp.BoundCount == 2);

	Fixture.Weapons[0]->SetRootComponent(nullptr);
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Corrupted =
		Host.AdvanceOrbitingFrame(0.1f);
	TestTrue(TEXT("Corrupted non-empty Host is never reported as idle"),
		Corrupted.IsValid()
		&& Corrupted.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::HostInvalid
		&& Corrupted.RunId == HostRunId
		&& Corrupted.BoundCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostIndependentLifecycleTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.IndependentContactAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostIndependentLifecycleTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.5 lifecycle fixture."));
		return false;
	}

	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenWorldHitContext LowContext;
	FShanmenWorldHitContext HighContext;
	TestTrue(TEXT("Both item Sessions own independent contact windows"),
		Host.TryLaunch(HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(HostHighItemId, 0, FVector::RightVector, Command)
		&& Host.TryBeginContactWindow(HostLowItemId, LowContext)
		&& Host.TryBeginContactWindow(HostHighItemId, HighContext));

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Low =
		Host.ResolveSweepContact(
			HostLowItemId,
			Fixture.Coordinator,
			MakeHostSweepHit(Fixture));
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult High =
		Host.ResolveOverlapContact(
			HostHighItemId,
			Fixture.Coordinator,
			MakeHostOverlap(Fixture),
			FVector(110.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Two exact items retain distinct impact identities"),
		Low.IsDelivered()
		&& High.IsDelivered()
		&& Low.Impact.GetRequest().Action.GetSourceItemInstanceId()
			== HostLowItemId
		&& High.Impact.GetRequest().Action.GetSourceItemInstanceId()
			== HostHighItemId
		&& Low.Impact.GetRequest().ImpactId
			!= High.Impact.GetRequest().ImpactId);
	TestTrue(TEXT("Both accepted impacts reach one canonical vitality"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality - After.CurrentVitality,
			Low.GetNewlyCommittedDamage()
				+ High.GetNewlyCommittedDamage(),
			0.001f));

	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Duplicate =
		Host.ResolveSweepContact(
			HostLowItemId,
			Fixture.Coordinator,
			MakeHostSweepHit(Fixture));
	const Fdemo_mapShanmenControlledWeaponProductController* HighController =
		Host.FindController(HostHighItemId);
	TestTrue(TEXT("One item's duplicate callback cannot mutate the other"),
		!Duplicate.IsDelivered()
		&& HighController
		&& HighController->GetSession().GetExecution()
			.NumAcceptedImpacts() == 1);

	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("Low item completes without closing the high item"),
		Host.TryEndContactWindow(HostLowItemId)
		&& Host.TryRecallAndComplete(
			HostLowItemId, 1, Recall, Recovery, Completed)
		&& Host.NumActive() == 1);
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt> Interrupted;
	TestTrue(TEXT("Host-wide interrupt terminates only remaining active items"),
		Host.TryInterruptAll(Interrupted)
		&& Interrupted.Num() == 1
		&& Interrupted[0].ItemInstanceId == HostHighItemId
		&& Host.NumActive() == 0);
	HighController = Host.FindController(HostHighItemId);
	TestTrue(TEXT("Interrupt also closes the high item's contact window"),
		HighController
		&& HighController->GetSession().IsTerminal()
		&& !HighController->HasActiveContactWindow());
	TestTrue(TEXT("Terminal items can be retired independently"),
		Host.TryRemoveTerminal(HostLowItemId)
		&& Host.NumBound() == 1
		&& Host.TryRemoveTerminal(HostHighItemId)
		&& Host.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostOrbitThreatTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitThreatRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostOrbitThreatTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.10 Host Orbit threat fixture."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	FShanmenWorldHitContext ThreatContext;
	TestTrue(TEXT("Host routes threat window to the exact Orbiting item"),
		Host.TryBeginOrbitThreatWindow(HostLowItemId, ThreatContext));
	const Fdemo_mapShanmenControlledWeaponOrbitThreatResult Projected =
		Host.ProjectOrbitThreatOverlap(
			HostLowItemId,
			Fixture.Coordinator,
			MakeHostOverlap(Fixture),
			FVector(80.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Host projection retains exact item and zero-damage boundary"),
		Projected.IsProjected()
		&& Projected.Context.GetAction().GetSourceItemInstanceId()
			== HostLowItemId
		&& FMath::IsNearlyEqual(
			Before.CurrentVitality, After.CurrentVitality));
	TestFalse(TEXT("Unknown item cannot borrow another item's threat window"),
		Host.ProjectOrbitThreatOverlap(
			HostHighItemId,
			Fixture.Coordinator,
			MakeHostOverlap(Fixture),
			FVector(80.0, 20.0, 30.0),
			FVector::BackwardVector).IsProjected());

	FShanmenControlledWeaponCommandReceipt Launch;
	TestFalse(TEXT("Host Launch is fenced while threat sample is active"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	FShanmenDetectorEmissionReceipt ThreatReceipt;
	TestTrue(TEXT("Host returns exact-item canonical threat evidence"),
		Host.TryEndOrbitThreatWindow(HostLowItemId, ThreatReceipt)
		&& ThreatReceipt.IsValid()
		&& ThreatReceipt.GetCandidates().Num() == 1
		&& ThreatReceipt.GetCandidates()[0].TargetEntityId
			== Projected.Candidate.TargetEntityId
		&& ThreatReceipt.GetContext().GetAction().GetSourceItemInstanceId()
			== HostLowItemId);
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Evidence;
	FShanmenControlledWeaponThreatPolicyReceipt ThreatPolicy;
	TestTrue(TEXT("Host captures world evidence for the exact item"),
		Host.TryEvaluateOrbitThreatActors(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			Evidence,
			ThreatPolicy)
		&& Evidence.IsCaptured()
		&& Evidence.TargetEvidence.Num() == 1
		&& Evidence.TargetEvidence[0].GetTargetEntityId()
			== Projected.Candidate.TargetEntityId
		&& ThreatPolicy.IsValid()
		&& ThreatPolicy.NumAcceptedTargets() == 1);
	FShanmenControlledWeaponThreatPolicyReceipt RejectedPolicy;
	TestFalse(TEXT("Unknown item cannot capture another item's targets"),
		Host.TryEvaluateOrbitThreatActors(
			HostHighItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			Evidence,
			RejectedPolicy));
	FShanmenControlledWeaponThreatPresenceReceipt Presence;
	TestTrue(TEXT("Host emits threat presence for the exact routed item"),
		Host.TryBuildOrbitThreatPresenceIntents(
			HostLowItemId, ThreatPolicy, Presence)
		&& Presence.IsValid()
		&& Presence.GetIntents().Num() == 1
		&& Presence.GetIntents()[0].GetSourceItemInstanceId()
			== HostLowItemId
		&& Presence.GetIntents()[0].GetCandidate().TargetEntityId
			== Projected.Candidate.TargetEntityId);
	FShanmenControlledWeaponThreatPresenceReceipt RejectedPresence;
	TestFalse(TEXT("Unknown item cannot borrow another item's presence policy"),
		Host.TryBuildOrbitThreatPresenceIntents(
			HostHighItemId, ThreatPolicy, RejectedPresence));

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult FirstFinalized;
	TestTrue(TEXT("Run Host atomically finalizes exact-item threat evidence"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			FirstFinalized)
		&& FirstFinalized.IsFinalized()
		&& FirstFinalized.GetItemInstanceId() == HostLowItemId
		&& FirstFinalized.GetEvidence().TargetEvidence.Num() == 1
		&& FirstFinalized.GetPresence().GetIntents().Num() == 1
		&& FirstFinalized.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		&& FirstFinalized.GetConsumption().GetReceipts().Num() == 1
		&& FirstFinalized.GetConsumption().GetReceipts()[0].GetIntent()
			.GetCandidate().TargetEntityId
			== Projected.Candidate.TargetEntityId
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult ReplayFinalized;
	TestTrue(TEXT("Atomic threat finalization is exact-replay idempotent"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			ReplayFinalized)
		&& ReplayFinalized.IsFinalized()
		&& ReplayFinalized.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::AlreadyConsumed
		&& ReplayFinalized.GetConsumption().GetReceipts()[0]
			.GetAuthorityRevision()
			== FirstFinalized.GetConsumption().GetReceipts()[0]
				.GetAuthorityRevision()
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult RejectedFinalized;
	TestFalse(TEXT("Unknown item cannot finalize another item's threat sample"),
		Host.TryFinalizeOrbitThreatSample(
			HostHighItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			RejectedFinalized));
	TestFalse(TEXT("Incomplete world evidence cannot partially consume"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{},
			RejectedFinalized));
	TestTrue(TEXT("Rejected finalization preserves the authority ledger"),
		!RejectedFinalized.IsFinalized()
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);

	FShanmenWorldHitContext EmptyThreatContext;
	FShanmenDetectorEmissionReceipt EmptyThreatReceipt;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult EmptyFinalized;
	TestTrue(TEXT("Explicit empty threat samples finalize as a zero-effect no-op"),
		Host.TryBeginOrbitThreatWindow(HostLowItemId, EmptyThreatContext)
		&& Host.TryEndOrbitThreatWindow(HostLowItemId, EmptyThreatReceipt)
		&& EmptyThreatReceipt.GetCandidates().IsEmpty()
		&& Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			EmptyThreatReceipt,
			{},
			EmptyFinalized)
		&& EmptyFinalized.IsFinalized()
		&& EmptyFinalized.GetEvidence().TargetEvidence.IsEmpty()
		&& EmptyFinalized.GetPresence().GetIntents().IsEmpty()
		&& EmptyFinalized.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == EmptyThreatContext.GetHitOrdinal()
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult ExpiredFinalized;
	TestFalse(TEXT("A newer empty sample expires an older completed sample"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			ExpiredFinalized));
	TestTrue(TEXT("Expired finalization cannot resurrect retained intents"),
		!ExpiredFinalized.IsFinalized()
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	FShanmenTargetVitalitySnapshot AfterConsume;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterConsume));
	TestTrue(TEXT("Presence consumption mutates no vitality or impact ledger"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality, AfterConsume.CurrentVitality)
		&& Host.FindController(HostLowItemId)
		&& Host.FindController(HostLowItemId)->GetSession().GetExecution()
			.NumAcceptedImpacts() == 0);
	TestTrue(TEXT("Consumed presence leaves the item free to Launch"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	FShanmenControlledWeaponThreatPresenceConsumeResult StateRejected;
	TestFalse(TEXT("A Directed item cannot replay its latest Orbit presence"),
		Host.TryConsumeOrbitThreatPresence(
			HostLowItemId,
			EmptyFinalized.GetPresence(),
			StateRejected));
	FShanmenWorldHitContext DirectedContext;
	TestTrue(TEXT("Host-directed window follows the same detector ordinal"),
		Host.TryBeginContactWindow(HostLowItemId, DirectedContext)
		&& DirectedContext.GetHitOrdinal()
			== EmptyThreatContext.GetHitOrdinal() + 1
		&& Host.TryEndContactWindow(HostLowItemId));
	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("Last-item retirement clears the Run-scoped authority"),
		Host.TryRecallAndComplete(
			HostLowItemId, 1, Recall, Recovery, Completed)
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.TryRemoveTerminal(HostLowItemId)
		&& Host.IsEmpty()
		&& Host.NumConsumedThreatPresenceIntents() == 0
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostAtomicThreatSampleTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.AtomicThreatSamplingTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostAtomicThreatSampleTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.20 atomic sample fixture."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>
		OneContact = { MakeHostThreatContact(Fixture) };
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult First;
	TestTrue(TEXT("One call commits the complete exact-item sample transaction"),
		Host.TrySampleOrbitThreat(
			HostLowItemId, Fixture.Coordinator, OneContact, First)
		&& First.IsFinalized()
		&& First.GetItemInstanceId() == HostLowItemId
		&& First.GetEvidence().ExpectedTargetCount == 1
		&& First.GetPresence().GetPolicy().GetEmission()
			.GetContext().GetHitOrdinal() == 0
		&& First.GetPresence().GetIntents().Num() == 1
		&& First.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1
		&& Host.FindController(HostLowItemId)
		&& !Host.FindController(HostLowItemId)->HasActiveContactWindow());

	const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>
		DuplicateContacts = {
			MakeHostThreatContact(Fixture),
			MakeHostThreatContact(Fixture)
		};
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Rejected;
	TestFalse(TEXT("One rejected contact rolls back the entire Host candidate"),
		Host.TrySampleOrbitThreat(
			HostLowItemId,
			Fixture.Coordinator,
			DuplicateContacts,
			Rejected));
	TestTrue(TEXT("Rejected batch leaks no window, checkpoint, or authority"),
		!Rejected.IsFinalized()
		&& Host.FindController(HostLowItemId)
		&& !Host.FindController(HostLowItemId)->HasActiveContactWindow()
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Empty;
	TestTrue(TEXT("Retry reuses the rolled-back ordinal as an explicit no-op"),
		Host.TrySampleOrbitThreat(
			HostLowItemId,
			Fixture.Coordinator,
			{},
			Empty)
		&& Empty.IsFinalized()
		&& Empty.GetEvidence().ExpectedTargetCount == 0
		&& Empty.GetEvidence().TargetEvidence.IsEmpty()
		&& Empty.GetPresence().GetPolicy().GetEmission()
			.GetContext().GetHitOrdinal() == 1
		&& Empty.GetPresence().GetIntents().IsEmpty()
		&& Empty.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);

	Fdemo_mapShanmenControlledWeaponOrbitThreatContact InvalidContact;
	InvalidContact.ContactLocation = FVector(80.0, 20.0, 30.0);
	InvalidContact.ContactNormal = FVector::BackwardVector;
	TestFalse(TEXT("Unresolved geometry also fails the whole transaction"),
		Host.TrySampleOrbitThreat(
			HostLowItemId,
			Fixture.Coordinator,
			{ InvalidContact },
			Rejected));
	TestFalse(TEXT("Unknown items cannot open an atomic sample transaction"),
		Host.TrySampleOrbitThreat(
			HostThirdItemId,
			Fixture.Coordinator,
			{},
			Rejected));
	TestTrue(TEXT("All rejected transactions preserve the last exact sample"),
		!Rejected.IsFinalized()
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1
		&& Host.FindController(HostLowItemId)
		&& !Host.FindController(HostLowItemId)->HasActiveContactWindow());

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Retry;
	TestTrue(TEXT("A valid retry proves failed geometry did not spend ordinal two"),
		Host.TrySampleOrbitThreat(
			HostLowItemId, Fixture.Coordinator, OneContact, Retry)
		&& Retry.IsFinalized()
		&& Retry.GetPresence().GetPolicy().GetEmission()
			.GetContext().GetHitOrdinal() == 2
		&& Host.NumConsumedThreatPresenceIntents() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 3
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2);

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult High;
	TestTrue(TEXT("Failed low-item work cannot advance another item ordinal"),
		Host.TrySampleOrbitThreat(
			HostHighItemId,
			Fixture.Coordinator,
			{},
			High)
		&& High.IsFinalized()
		&& High.GetItemInstanceId() == HostHighItemId
		&& High.GetPresence().GetPolicy().GetEmission()
			.GetContext().GetHitOrdinal() == 0
		&& High.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostHighItemId) == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 4
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2);

	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Atomic sampling remains presence-only and zero-impact"),
		FMath::IsNearlyEqual(Before.CurrentVitality, After.CurrentVitality)
		&& Host.FindController(HostLowItemId)
		&& Host.FindController(HostLowItemId)->GetSession().GetExecution()
			.NumAcceptedImpacts() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostAtomicThreatSampleBatchTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.AtomicThreatSampleBatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostAtomicThreatSampleBatchTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.21 atomic batch fixture."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>
		OneContact = { MakeHostThreatContact(Fixture) };
	const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>
		DuplicateContacts = {
			MakeHostThreatContact(Fixture),
			MakeHostThreatContact(Fixture)
		};

	Fdemo_mapShanmenControlledWeaponThreatSampleRequest LowRequest;
	LowRequest.ItemInstanceId = HostLowItemId;
	LowRequest.Contacts = OneContact;
	Fdemo_mapShanmenControlledWeaponThreatSampleRequest HighRequest;
	HighRequest.ItemInstanceId = HostHighItemId;
	HighRequest.Contacts = OneContact;
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch First;
	TestTrue(TEXT("Reverse caller order commits in stable exact-item order"),
		Host.TrySampleOrbitThreatsInOrder(
			Fixture.Coordinator,
			{ HighRequest, LowRequest },
			First)
		&& First.IsFullyFinalized()
		&& First.RunId == HostRunId
		&& First.AttemptedCount == 2
		&& First.FinalizedCount == 2
		&& First.Entries.Num() == 2
		&& First.Entries[0].ItemInstanceId == HostLowItemId
		&& First.Entries[1].ItemInstanceId == HostHighItemId
		&& First.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& First.Entries[1].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& First.Entries[0].Finalization.GetConsumption().GetReceipts().Num()
			== 1
		&& First.Entries[1].Finalization.GetConsumption().GetReceipts().Num()
			== 1
		&& First.Entries[0].Finalization.GetConsumption().GetReceipts()[0]
			.GetAuthorityRevision() == 1
		&& First.Entries[1].Finalization.GetConsumption().GetReceipts()[0]
			.GetAuthorityRevision() == 2
		&& Host.NumConsumedThreatPresenceIntents() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2);
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch Reordered = First;
	Reordered.Entries.Swap(0, 1);
	TestFalse(TEXT("Batch receipt rejects non-canonical item ordering"),
		Reordered.IsFullyFinalized());
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch WrongRun = First;
	WrongRun.RunId = HostThirdItemId;
	TestFalse(TEXT("Batch receipt rejects a mismatched Run identity"),
		WrongRun.IsFullyFinalized());

	Fdemo_mapShanmenControlledWeaponThreatSampleRequest LowEmpty = LowRequest;
	LowEmpty.Contacts.Reset();
	Fdemo_mapShanmenControlledWeaponThreatSampleRequest HighRejected =
		HighRequest;
	HighRejected.Contacts = DuplicateContacts;
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch Rejected;
	TestFalse(TEXT("A later rejected item discards the whole Host candidate"),
		Host.TrySampleOrbitThreatsInOrder(
			Fixture.Coordinator,
			{ HighRejected, LowEmpty },
			Rejected));
	TestTrue(TEXT("Rejected batch spends no item ordinal or authority revision"),
		!Rejected.IsFullyFinalized()
		&& !Rejected.RunId.IsValid()
		&& Rejected.AttemptedCount == 0
		&& Rejected.FinalizedCount == 0
		&& Rejected.Entries.IsEmpty()
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == 0
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostHighItemId) == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2
		&& Host.NumConsumedThreatPresenceIntents() == 2);

	Fdemo_mapShanmenControlledWeaponThreatSampleRequest HighEmpty =
		HighRequest;
	HighEmpty.Contacts.Reset();
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch Retry;
	TestTrue(TEXT("Retry reuses both rolled-back ordinals as ordered no-ops"),
		Host.TrySampleOrbitThreatsInOrder(
			Fixture.Coordinator,
			{ HighEmpty, LowEmpty },
			Retry)
		&& Retry.IsFullyFinalized()
		&& Retry.Entries[0].ItemInstanceId == HostLowItemId
		&& Retry.Entries[1].ItemInstanceId == HostHighItemId
		&& Retry.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 1
		&& Retry.Entries[1].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 1
		&& Retry.Entries[0].Finalization.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Retry.Entries[1].Finalization.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 4
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2
		&& Host.NumConsumedThreatPresenceIntents() == 2);

	Fdemo_mapShanmenControlledWeaponThreatSampleRequest UnknownRequest;
	UnknownRequest.ItemInstanceId = HostThirdItemId;
	TestFalse(TEXT("Duplicate exact-item requests fail before any sample"),
		Host.TrySampleOrbitThreatsInOrder(
			Fixture.Coordinator,
			{ LowEmpty, LowEmpty },
			Rejected));
	TestFalse(TEXT("Unknown exact items fail batch preflight"),
		Host.TrySampleOrbitThreatsInOrder(
			Fixture.Coordinator,
			{ UnknownRequest },
			Rejected));
	TestFalse(TEXT("An empty caller subset is not a sample batch"),
		Host.TrySampleOrbitThreatsInOrder(
			Fixture.Coordinator,
			{},
			Rejected));
	TestTrue(TEXT("All preflight failures leave the committed batch untouched"),
		!Rejected.IsFullyFinalized()
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == 1
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostHighItemId) == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 4
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2
		&& Host.FindController(HostLowItemId)
		&& !Host.FindController(HostLowItemId)->HasActiveContactWindow()
		&& Host.FindController(HostHighItemId)
		&& !Host.FindController(HostHighItemId)->HasActiveContactWindow());

	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Atomic batch sampling remains presence-only and zero-impact"),
		FMath::IsNearlyEqual(Before.CurrentVitality, After.CurrentVitality)
		&& Host.FindController(HostLowItemId)->GetSession().GetExecution()
			.NumAcceptedImpacts() == 0
		&& Host.FindController(HostHighItemId)->GetSession().GetExecution()
			.NumAcceptedImpacts() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostThreatWatermarkTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.PerItemThreatSampleWatermarks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostThreatWatermarkTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.18 activation lifecycle fixture."));
		return false;
	}
	TestTrue(TEXT("Host admits one exact activation per attached item"),
		Host.GetThreatPresenceAuthority().NumRegisteredItemActivations() == 2
		&& Host.GetThreatPresenceAuthority().GetRegisteredActivationId(
			HostLowItemId).IsValid()
		&& Host.GetThreatPresenceAuthority().GetRegisteredActivationId(
			HostHighItemId).IsValid());

	FShanmenWorldHitContext LowFirstContext;
	FShanmenDetectorEmissionReceipt LowFirstEmission;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult LowFirst;
	TestTrue(TEXT("First low-item sample establishes one empty watermark"),
		Host.TryBeginOrbitThreatWindow(HostLowItemId, LowFirstContext)
		&& Host.TryEndOrbitThreatWindow(
			HostLowItemId, LowFirstEmission)
		&& Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			LowFirstEmission,
			{},
			LowFirst)
		&& LowFirst.IsFinalized()
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 1
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == LowFirstContext.GetHitOrdinal());

	FShanmenWorldHitContext HighFirstContext;
	FShanmenDetectorEmissionReceipt HighFirstEmission;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult HighFirst;
	TestTrue(TEXT("A second item owns an independent sample watermark"),
		Host.TryBeginOrbitThreatWindow(HostHighItemId, HighFirstContext)
		&& Host.TryEndOrbitThreatWindow(
			HostHighItemId, HighFirstEmission)
		&& Host.TryFinalizeOrbitThreatSample(
			HostHighItemId,
			Fixture.Coordinator,
			HighFirstEmission,
			{},
			HighFirst)
		&& HighFirst.IsFinalized()
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == LowFirstContext.GetHitOrdinal()
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostHighItemId) == HighFirstContext.GetHitOrdinal());

	FShanmenWorldHitContext LowLaterContext;
	FShanmenDetectorEmissionReceipt LowLaterEmission;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult LowLater;
	FShanmenControlledWeaponThreatPresenceConsumeResult StaleConsumption;
	TestTrue(TEXT("The low item opens its next explicit sample"),
		Host.TryBeginOrbitThreatWindow(HostLowItemId, LowLaterContext));
	TestFalse(TEXT("An open newer sample fences direct prior-presence replay"),
		Host.TryConsumeOrbitThreatPresence(
			HostLowItemId,
			LowFirst.GetPresence(),
			StaleConsumption));
	TestTrue(TEXT("The low item closes its newer geometry sample"),
		Host.TryEndOrbitThreatWindow(
			HostLowItemId, LowLaterEmission));
	TestFalse(TEXT("Completed newer geometry expires prior presence immediately"),
		Host.TryConsumeOrbitThreatPresence(
			HostLowItemId,
			LowFirst.GetPresence(),
			StaleConsumption));
	TestTrue(TEXT("Only the low-item watermark advances on finalization"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			LowLaterEmission,
			{},
			LowLater)
		&& LowLater.IsFinalized()
		&& LowLaterContext.GetHitOrdinal()
			== LowFirstContext.GetHitOrdinal() + 1
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 3
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == LowLaterContext.GetHitOrdinal()
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostHighItemId) == HighFirstContext.GetHitOrdinal());

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Replay;
	TestTrue(TEXT("The other item's exact latest sample remains replayable"),
		Host.TryFinalizeOrbitThreatSample(
			HostHighItemId,
			Fixture.Coordinator,
			HighFirstEmission,
			{},
			Replay)
		&& Replay.IsFinalized()
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 3);
	TestFalse(TEXT("The superseded low-item sample cannot replay"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			LowFirstEmission,
			{},
			Replay));
	TestTrue(TEXT("Rejected stale replay preserves both item watermarks"),
		!Replay.IsFinalized()
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 3
		&& Host.NumConsumedThreatPresenceIntents() == 0);

	FShanmenControlledWeaponCommandReceipt Launch;
	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("One terminal item retires only its own activation and sample"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch)
		&& Host.TryRecallAndComplete(
			HostLowItemId, 1, Recall, Recovery, Completed)
		&& Host.TryRemoveTerminal(HostLowItemId)
		&& Host.IsValid()
		&& Host.NumBound() == 1
		&& Host.GetThreatPresenceAuthority().NumRegisteredItemActivations() == 1
		&& !Host.GetThreatPresenceAuthority().GetRegisteredActivationId(
			HostLowItemId).IsValid()
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 1
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == INDEX_NONE
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostHighItemId) == HighFirstContext.GetHitOrdinal()
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 4);

	const Fdemo_mapShanmenControlledWeaponHostAttachResult Replacement =
		AttachHostWeapon(Fixture, Host, HostLowItemId, 3, 0);
	TestTrue(TEXT("The same physical item can attach with a new activation"),
		Replacement.IsAttached()
		&& Replacement.ActivationId
			!= LowFirstContext.GetAction().GetActivationId()
		&& Host.IsValid()
		&& Host.NumBound() == 2
		&& Host.GetThreatPresenceAuthority().NumRegisteredItemActivations() == 2
		&& Host.GetThreatPresenceAuthority().GetRegisteredActivationId(
			HostLowItemId) == Replacement.ActivationId
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == INDEX_NONE
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 4);

	FShanmenWorldHitContext ReplacementContext;
	FShanmenDetectorEmissionReceipt ReplacementEmission;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult
		ReplacementFinalized;
	TestTrue(TEXT("The replacement activation starts a fresh ordinal stream"),
		Host.TryBeginOrbitThreatWindow(
			HostLowItemId, ReplacementContext)
		&& Host.TryEndOrbitThreatWindow(
			HostLowItemId, ReplacementEmission)
		&& Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ReplacementEmission,
			{},
			ReplacementFinalized)
		&& ReplacementFinalized.IsFinalized()
		&& ReplacementContext.GetAction().GetActivationId()
			== Replacement.ActivationId
		&& ReplacementContext.GetHitOrdinal() == 0
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 2
		&& Host.GetThreatPresenceAuthority().GetLatestSampleOrdinal(
			HostLowItemId) == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 5);
	TestFalse(TEXT("The retired activation cannot replay after reattachment"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			LowLaterEmission,
			{},
			Replay));
	TestTrue(TEXT("Reattachment preserves the other item's exact replay"),
		!Replay.IsFinalized()
		&& Host.TryFinalizeOrbitThreatSample(
			HostHighItemId,
			Fixture.Coordinator,
			HighFirstEmission,
			{},
			Replay)
		&& Replay.IsFinalized()
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 5);

	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt> Interrupted;
	TestTrue(TEXT("Run teardown clears reattached activation watermarks"),
		Host.TryInterruptAll(Interrupted)
		&& Interrupted.Num() == 2
		&& Host.TryRemoveTerminal(HostLowItemId)
		&& Host.GetThreatPresenceAuthority().NumRegisteredItemActivations() == 1
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 1
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 6
		&& Host.TryRemoveTerminal(HostHighItemId)
		&& Host.IsEmpty()
		&& Host.GetThreatPresenceAuthority().NumRegisteredItemActivations() == 0
		&& Host.GetThreatPresenceAuthority().NumTrackedSamples() == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostAtomicFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.AttachFencesAndAtomicInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostAtomicFenceTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.5 fence fixture."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponHostAttachResult DuplicateItem =
		AttachHostWeapon(Fixture, Host, HostLowItemId, 2, 1);
	const Fdemo_mapShanmenControlledWeaponHostAttachResult DuplicateActor =
		Host.TryAttach(
			MakeHostPrepared(Fixture.Coordinator, HostHighItemId, 2),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[0],
			Fixture.WeaponRoots[0],
			MakeHostMotion());
	const Fdemo_mapShanmenControlledWeaponHostAttachResult
		DuplicateActivation = Host.TryAttach(
			MakeHostPrepared(Fixture.Coordinator, HostThirdItemId, 1),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[2],
			Fixture.WeaponRoots[2],
			MakeHostMotion());
	TestTrue(TEXT("Exact item identity cannot be attached twice"),
		DuplicateItem.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::ItemAlreadyBound);
	TestTrue(TEXT("One physical Actor cannot represent two items"),
		DuplicateActor.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::WeaponActorAlreadyBound);
	TestTrue(TEXT("One activation identity cannot represent two items"),
		DuplicateActivation.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::ActivationAlreadyBound);
	TestTrue(TEXT("Rejected attachments leave the original host unchanged"),
		Host.IsValid()
		&& Host.NumBound() == 1
		&& Host.FindController(HostLowItemId) != nullptr);

	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenWorldHitContext Context;
	TestTrue(TEXT("Interrupt fixture owns one live contact window"),
		Host.TryLaunch(HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryBeginContactWindow(HostLowItemId, Context));
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt> Interrupted;
	TestTrue(TEXT("Host interrupt is an atomic terminal transition"),
		Host.TryInterruptAll(Interrupted)
		&& Interrupted.Num() == 1
		&& Interrupted[0].ItemInstanceId == HostLowItemId
		&& Host.IsValid()
		&& Host.NumActive() == 0);
	TestFalse(TEXT("A second interrupt has no partial work to commit"),
		Host.TryInterruptAll(Interrupted));
	TestTrue(TEXT("No-op interrupt returns an empty receipt set"),
		Interrupted.IsEmpty());
	TestTrue(TEXT("Terminal retirement resets the empty host binding"),
		Host.TryRemoveTerminal(HostLowItemId)
		&& Host.IsEmpty()
		&& !Host.IsValid());
	return true;
}

#endif
