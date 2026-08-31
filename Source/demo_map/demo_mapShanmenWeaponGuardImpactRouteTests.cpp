#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/UObjectGlobals.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenWeaponGuardProductSession.h"

namespace
{
	const EAutomationTestFlags WeaponGuardImpactRouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid WeaponGuardImpactRun(
		0xE1A20001, 0xE1A20002, 0xE1A20003, 0xE1A20004);
	const FGuid WeaponGuardImpactTimeline(
		0xE1A30001, 0xE1A30002, 0xE1A30003, 0xE1A30004);
	const FGuid ForeignTimeline(
		0xE1A40001, 0xE1A40002, 0xE1A40003, 0xE1A40004);

	const Fdemo_mapM01EnemyDefinition* FindStandardMeleeDefinition()
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

	struct FWeaponGuardImpactRouteFixture
	{
		UWorld* World = nullptr;
		APawn* Player = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapItemAuthority Items;
		Fdemo_mapShanmenWeaponGuardProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FWeaponGuardImpactRouteFixture()
		{
			Start();
		}

		~FWeaponGuardImpactRouteFixture()
		{
			Session.TryTerminate(
				Edemo_mapShanmenWeaponGuardTerminationReason::RunTeardown);
			Coordinator.Reset();
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

		void Start()
		{
			if (!GEngine)
			{
				Diagnostic = TEXT("GEngine is unavailable.");
				return;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				Diagnostic = TEXT("Transient World allocation failed.");
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

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Player = World->SpawnActor<APawn>(
				APawn::StaticClass(), FTransform::Identity, SpawnParameters);
			PlayerRoot = Player
				? NewObject<UBoxComponent>(
					Player, TEXT("WeaponGuardImpactPlayerRoot"), RF_Transient)
				: nullptr;
			PlayerHealth = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player, TEXT("WeaponGuardImpactPlayerHealth"), RF_Transient)
				: nullptr;
			if (!Player || !PlayerRoot || !PlayerHealth)
			{
				Diagnostic = TEXT("Player fixture allocation failed.");
				return;
			}
			Player->AddInstanceComponent(PlayerRoot);
			Player->SetRootComponent(PlayerRoot);
			PlayerRoot->RegisterComponent();
			Player->AddInstanceComponent(PlayerHealth);
			PlayerHealth->RegisterComponent();
			Player->SetActorLocationAndRotation(
				FVector::ZeroVector,
				FRotator::ZeroRotator);

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindStandardMeleeDefinition();
			Enemy = Definition
				? World->SpawnActor<Ademo_mapEnemyCharacter>(
					Ademo_mapEnemyCharacter::StaticClass(),
					FTransform::Identity,
					SpawnParameters)
				: nullptr;
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("WeaponGuardImpactEnemyIdentity"), RF_Transient)
				: nullptr;
			if (!Definition || !Enemy || !EnemyIdentity)
			{
				Diagnostic = TEXT("Enemy fixture allocation failed.");
				return;
			}
			Enemy->AddInstanceComponent(EnemyIdentity);
			EnemyIdentity->RegisterComponent();
			Enemy->SetActorLocation(FVector(100.0, 0.0, 0.0));
			const Fdemo_mapEnemyEncounterIdentity EncounterIdentity =
				MakeEncounterIdentity(*Definition);
			if (!EnemyIdentity->Configure(*Definition)
				|| !Enemy->ConfigureEncounter(
					EncounterIdentity,
					Definition->Tuning,
					Definition->IsElite())
				|| !Coordinator.TryBeginRun(
					WeaponGuardImpactRun,
					Player,
					PlayerHealth,
					Diagnostic)
				|| !Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic))
			{
				return;
			}

			TArray<FGuid> AddedIds;
			const Fdemo_mapItemOperationResult Added = Items.AddDefinition(
				Fdemo_mapItemIds::TrainingBlade, 1, &AddedIds);
			if (!Added.bSuccess
				|| AddedIds.Num() != 1
				|| !Items.Equip(
					AddedIds[0], Fdemo_mapItemIds::WeaponSlot).bSuccess)
			{
				Diagnostic = TEXT("Training blade fixture setup failed.");
				return;
			}
			const Fdemo_mapShanmenWeaponGuardSessionStartResult Started =
				Session.TryStart(
					&Items,
					Coordinator,
					WeaponGuardImpactTimeline,
					10);
			bReady = Started.IsStarted();
			if (!bReady)
			{
				Diagnostic = Started.Diagnostic;
			}
		}

		Fdemo_mapM01EnemyAttackWeaponGuardContext Context(
			const FGuid& TimelineId,
			int64 ObservedTick)
		{
			Fdemo_mapM01EnemyAttackWeaponGuardContext Result;
			Result.Session = &Session;
			Result.TimelineId = TimelineId;
			Result.ObservedTick = ObservedTick;
			return Result;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardImpactRoutePerfectFrontTest,
	"Shanmen.0_0_10.Product.WeaponGuardImpactRoute.PerfectFront",
	WeaponGuardImpactRouteFlags)

bool Fdemo_mapWeaponGuardImpactRoutePerfectFrontTest::RunTest(const FString&)
{
	FWeaponGuardImpactRouteFixture Fixture;
	TestTrue(TEXT("real World impact-route fixture starts"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const float VitalityBefore = Fixture.PlayerHealth->GetCurrentVitality();
	Fdemo_mapM01EnemyAttackWeaponGuardContext Context = Fixture.Context(
		WeaponGuardImpactTimeline, 10);
	const Fdemo_mapM01EnemyAttackExecutionResult Result =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			Fixture.Enemy,
			Fixture.Player,
			4.0f,
			&Context);
	TestTrue(TEXT("front contact consumes the active perfect-guard route"),
		Result.IsExecuted()
			&& Result.bWeaponGuardInspected
			&& Result.WeaponGuardDefense.IsSuccess()
			&& Result.WeaponGuardDefense.HasGuardLayer()
			&& Result.Impact.GetResult().Outcome
				== EShanmenDefenseOutcome::PerfectGuarded
			&& FMath::IsNearlyEqual(
				Result.Impact.GetResult().FinalDamage, 0.0f));
	TestTrue(TEXT("perfect guard commits a zero-damage canonical receipt"),
		Result.Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				Fixture.PlayerHealth->GetCurrentVitality(), VitalityBefore)
			&& Fixture.PlayerHealth->NumCommittedCombatImpacts() == 1);
	TestTrue(TEXT("session retains the active Host at the observed tick"),
		Fixture.Session.HasActive()
			&& Fixture.Session.GetActiveHost()
			&& Fixture.Session.GetActiveHost()->GetLastObservedTick() == 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardImpactRouteOrdinaryAndOutsideArcTest,
	"Shanmen.0_0_10.Product.WeaponGuardImpactRoute.OrdinaryAndOutsideArc",
	WeaponGuardImpactRouteFlags)

bool Fdemo_mapWeaponGuardImpactRouteOrdinaryAndOutsideArcTest::RunTest(
	const FString&)
{
	FWeaponGuardImpactRouteFixture Fixture;
	TestTrue(TEXT("real World impact-route fixture starts"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	Fdemo_mapM01EnemyAttackWeaponGuardContext OrdinaryContext =
		Fixture.Context(WeaponGuardImpactTimeline, 15);
	const Fdemo_mapM01EnemyAttackExecutionResult Ordinary =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			Fixture.Enemy,
			Fixture.Player,
			4.0f,
			&OrdinaryContext);
	TestTrue(TEXT("front contact after perfect window applies ordinary guard"),
		Ordinary.IsExecuted()
			&& Ordinary.WeaponGuardDefense.HasGuardLayer()
			&& Ordinary.Impact.GetResult().Outcome
				== EShanmenDefenseOutcome::Mitigated
			&& FMath::IsNearlyEqual(
				Ordinary.Impact.GetResult().PreventedDamage, 1.0f)
			&& FMath::IsNearlyEqual(
				Ordinary.Impact.GetResult().FinalDamage, 3.0f)
			&& FMath::IsNearlyEqual(
				Fixture.PlayerHealth->GetCurrentVitality(), 2.0f));

	Fixture.Enemy->SetActorLocation(FVector(-100.0, 0.0, 0.0));
	Fdemo_mapM01EnemyAttackWeaponGuardContext OutsideContext =
		Fixture.Context(WeaponGuardImpactTimeline, 16);
	const Fdemo_mapM01EnemyAttackExecutionResult Outside =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			Fixture.Enemy,
			Fixture.Player,
			1.0f,
			&OutsideContext);
	TestTrue(TEXT("rear contact is inspected without appending a guard layer"),
		Outside.IsExecuted()
			&& Outside.bWeaponGuardInspected
			&& Outside.WeaponGuardDefense.IsSuccess()
			&& !Outside.WeaponGuardDefense.HasGuardLayer()
			&& Outside.WeaponGuardDefense.Status
				== Edemo_mapShanmenWeaponGuardSessionDefenseStatus::
					ComposedOutsideArc
			&& Outside.Impact.GetRequest().Defense.Layers.IsEmpty()
			&& FMath::IsNearlyEqual(
				Outside.Impact.GetResult().FinalDamage, 1.0f)
			&& FMath::IsNearlyEqual(
				Fixture.PlayerHealth->GetCurrentVitality(), 1.0f));
	TestTrue(TEXT("both accepted observations advance one persistent Host"),
		Fixture.Session.GetActiveHost()
			&& Fixture.Session.GetActiveHost()->GetLastObservedTick() == 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardImpactRouteFailureAtomicTest,
	"Shanmen.0_0_10.Product.WeaponGuardImpactRoute.FailureAtomic",
	WeaponGuardImpactRouteFlags)

bool Fdemo_mapWeaponGuardImpactRouteFailureAtomicTest::RunTest(const FString&)
{
	FWeaponGuardImpactRouteFixture Fixture;
	TestTrue(TEXT("real World impact-route fixture starts"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const float VitalityBefore = Fixture.PlayerHealth->GetCurrentVitality();
	const FGuid HostId = Fixture.Session.GetActiveHost()
		? Fixture.Session.GetActiveHost()->GetHostId()
		: FGuid();
	Fdemo_mapM01EnemyAttackWeaponGuardContext WrongTimeline =
		Fixture.Context(ForeignTimeline, 11);
	const Fdemo_mapM01EnemyAttackExecutionResult Rejected =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			Fixture.Enemy,
			Fixture.Player,
			4.0f,
			&WrongTimeline);
	TestTrue(TEXT("timeline mismatch rejects before vitality delivery"),
		Rejected.Error
				== Edemo_mapM01EnemyAttackExecutionError::
					WeaponGuardDefensePreparationFailed
			&& Rejected.bWeaponGuardInspected
			&& Rejected.WeaponGuardDefense.Error
				== Edemo_mapShanmenWeaponGuardSessionDefenseError::
					TimelineMismatch
			&& !Rejected.Delivery.IsSuccess()
			&& FMath::IsNearlyEqual(
				Fixture.PlayerHealth->GetCurrentVitality(), VitalityBefore)
			&& Fixture.PlayerHealth->NumCommittedCombatImpacts() == 0);
	TestTrue(TEXT("rejected composition preserves exact Session state"),
		Fixture.Session.HasActive()
			&& Fixture.Session.GetActiveHost()
			&& Fixture.Session.GetActiveHost()->GetHostId() == HostId
			&& Fixture.Session.GetActiveHost()->GetLastObservedTick() == 10);

	Fdemo_mapM01EnemyAttackWeaponGuardContext ValidContext =
		Fixture.Context(WeaponGuardImpactTimeline, 11);
	const Fdemo_mapM01EnemyAttackExecutionResult Accepted =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			Fixture.Enemy,
			Fixture.Player,
			4.0f,
			&ValidContext);
	TestTrue(TEXT("same Session remains usable after rejected composition"),
		Accepted.IsExecuted()
			&& Accepted.WeaponGuardDefense.HasGuardLayer()
			&& Fixture.Session.GetActiveHost()
			&& Fixture.Session.GetActiveHost()->GetLastObservedTick() == 11);

	Fdemo_mapM01EnemyAttackWeaponGuardContext StaleContext =
		Fixture.Context(WeaponGuardImpactTimeline, 9);
	const int32 CommitCountBeforeStale =
		Fixture.PlayerHealth->NumCommittedCombatImpacts();
	const Fdemo_mapM01EnemyAttackExecutionResult Stale =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			Fixture.Enemy,
			Fixture.Player,
			1.0f,
			&StaleContext);
	TestTrue(TEXT("non-monotonic observation also fails without mutation"),
		Stale.Error
				== Edemo_mapM01EnemyAttackExecutionError::
					WeaponGuardDefensePreparationFailed
			&& Stale.WeaponGuardDefense.Error
				== Edemo_mapShanmenWeaponGuardSessionDefenseError::HostRejected
			&& Stale.WeaponGuardDefense.Defense.Error
				== Edemo_mapShanmenWeaponGuardHostDefenseError::
					NonMonotonicObservation
			&& Fixture.PlayerHealth->NumCommittedCombatImpacts()
				== CommitCountBeforeStale
			&& Fixture.Session.GetActiveHost()
			&& Fixture.Session.GetActiveHost()->GetLastObservedTick() == 11);
	return true;
}

#endif
