#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapM01BossCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRangedEnemyCharacter.h"

namespace
{
	const FGuid CoordinatorRunA(0x54300001, 0, 0, 1);
	const FGuid CoordinatorRunB(0x54300002, 0, 0, 1);
	const FGuid CoordinatorOwnerId(0x54300003, 0, 0, 1);

	struct FCombatRunCoordinatorFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FCombatRunCoordinatorFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("PlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("PlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					CoordinatorRunA,
					Pawn,
					Health,
					Diagnostic);
		}
	};

	const Fdemo_mapM01EnemyDefinition* FindM01MeleeDefinition()
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

	AActor* NewM01ProductActor(
		const Fdemo_mapM01EnemyDefinition& Definition)
	{
		switch (Definition.Archetype)
		{
		case Edemo_mapM01EnemyArchetype::StandardSkirmisher:
		case Edemo_mapM01EnemyArchetype::EliteStalker:
			return NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
		case Edemo_mapM01EnemyArchetype::StandardRanged:
			return NewObject<Ademo_mapRangedEnemyCharacter>(
				GetTransientPackage());
		case Edemo_mapM01EnemyArchetype::StandardBruiser:
		case Edemo_mapM01EnemyArchetype::EliteBulwark:
			return NewObject<Ademo_mapHeavyEnemyCharacter>(
				GetTransientPackage());
		case Edemo_mapM01EnemyArchetype::BossMain:
			return NewObject<Ademo_mapM01BossCharacter>(GetTransientPackage());
		default:
			return nullptr;
		}
	}

	struct FM01MeleeEnemyFixture
	{
		const Fdemo_mapM01EnemyDefinition* Definition = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* Identity = nullptr;
		bool bReady = false;

		FM01MeleeEnemyFixture()
		{
			Definition = FindM01MeleeDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			Identity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy,
					TEXT("M01AuthoredIdentity"))
				: nullptr;
			if (!Definition || !Enemy || !Identity)
			{
				return;
			}
			Enemy->AddInstanceComponent(Identity);
			Fdemo_mapEnemyEncounterIdentity LegacyIdentity;
			LegacyIdentity.EncounterId = Definition->EncounterId;
			LegacyIdentity.RouteId = Definition->RouteId;
			LegacyIdentity.SpawnMarkerId = Definition->SpawnMarkerId;
			LegacyIdentity.LootTableId = Definition->CorpseIdentity;
			LegacyIdentity.SkillProfileId = Definition->SkillProfileId;
			bReady = Identity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					LegacyIdentity,
					Definition->Tuning,
					Definition->IsElite());
		}
	};

	FShanmenCombatActionSnapshot MakeCoordinatorAction(
		const FGuid& RunId,
		const FGuid& SourceEntityId)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = CoordinatorOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.SourceItemInstanceId = FGuid(0x54300004, 0, 0, 1);
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P4.4");
		Capture.Content.Digest =
			TEXT("TEST-DIGEST-P4.4-COMBAT-RUN-COORDINATOR");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenBasicSwordDefinition MakeCoordinatorSwordDefinition()
	{
		FShanmenBasicSwordDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Weapon.Main");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Basic01.r1");
		Capture.BaseDamage = 2.0f;
		Capture.AttackPowerCoefficient = 0.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		FShanmenBasicSwordDefinition Definition;
		check(FShanmenBasicSwordDefinition::TryCapture(Capture, Definition));
		return Definition;
	}

	bool TryResolveSwordImpact(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenCombatActionSnapshot& Action,
		AActor* TargetActor,
		UPrimitiveComponent* TargetRoot,
		const FShanmenTargetVitalitySnapshot& Vitality,
		FShanmenBasicSwordImpactReceipt& OutImpact)
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Phase;
		if (!FShanmenActionOrchestrator::TryStart(Action, Runtime, Phase)
			|| !Runtime.TryAdvance(EShanmenCombatActionPhase::Startup, Phase))
		{
			return false;
		}

		FShanmenBasicSwordOffenseSnapshot Offense;
		if (!FShanmenBasicSwordOffenseSnapshot::TryCapture(1.0f, Offense))
		{
			return false;
		}
		FShanmenBasicSwordExecution Sword;
		if (!FShanmenBasicSwordExecution::TryCreate(
			Action,
			MakeCoordinatorSwordDefinition(),
			Offense,
			Sword))
		{
			return false;
		}
		FShanmenWorldHitContext Context;
		if (!Sword.TryBeginEmission(Runtime, Context))
		{
			return false;
		}

		FHitResult WorldHit(
			TargetActor,
			TargetRoot,
			FVector(20.0, 0.0, 50.0),
			FVector::BackwardVector);
		WorldHit.ImpactPoint = FVector(20.0, 0.0, 50.0);
		WorldHit.ImpactNormal = FVector::BackwardVector;
		WorldHit.Item = 0;
		FShanmenHitCandidate Candidate;
		if (!FShanmenWorldHitAdapter::TryFromSweep(
			Context,
			WorldHit,
			Coordinator.GetEntityRegistry(),
			Candidate))
		{
			return false;
		}

		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Sword.TryResolveCandidate(
			Runtime,
			Candidate,
			Vitality,
			Defense,
			OutImpact);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorLifecycleTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.RunLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorLifecycleTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("Product combat Run fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid ExpectedFirstId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunA,
			Fdemo_mapCombatRunCoordinator::PlayerSpawnSourceId(),
			0);
	TestTrue(TEXT("Player identity uses only Run and fixed spawn tuple"),
		Fixture.Coordinator.GetPlayerEntityId() == ExpectedFirstId);
	TestTrue(TEXT("Coordinator and vitality share one player identity"),
		Fixture.Coordinator.IsReady()
			&& Fixture.Health->GetCombatEntityId() == ExpectedFirstId);

	FGuid Resolved;
	TestTrue(TEXT("Registry resolves the product Pawn alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			Fixture.Pawn,
			INDEX_NONE,
			Resolved)
			&& Resolved == ExpectedFirstId);
	TestTrue(TEXT("Registry resolves the collision-root alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			Fixture.CollisionRoot,
			INDEX_NONE,
			Resolved)
			&& Resolved == ExpectedFirstId);
	TestTrue(TEXT("Same product Run bind is idempotent"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunA,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	TestFalse(TEXT("Live coordinator rejects silent cross-Run switch"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	TestFalse(TEXT("Foreign Run cannot release the live identity"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunB,
			Fixture.Diagnostic));
	TestTrue(TEXT("Exact Run release clears registry and vitality binding"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic)
			&& !Fixture.Coordinator.IsActive()
			&& !Fixture.Health->IsCombatEntityBound());

	TestTrue(TEXT("Persistent Pawn may enter a later authority Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	const FGuid ExpectedSecondId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunB,
			Fdemo_mapCombatRunCoordinator::PlayerSpawnSourceId(),
			0);
	TestTrue(TEXT("New Run deterministically produces a distinct player identity"),
		ExpectedSecondId.IsValid()
			&& ExpectedSecondId != ExpectedFirstId
			&& Fixture.Coordinator.GetPlayerEntityId() == ExpectedSecondId
			&& Fixture.Health->GetCombatEntityId() == ExpectedSecondId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorPlayerDeliveryTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.PlayerDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorPlayerDeliveryTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("Product combat Run fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid SourceEntityId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunA,
			TEXT("Spawn.Combat.TestSource"),
			0);
	const FShanmenCombatActionSnapshot Action =
		MakeCoordinatorAction(CoordinatorRunA, SourceEntityId);
	FShanmenTargetVitalitySnapshot Vitality;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Vitality));
	FShanmenBasicSwordImpactReceipt Impact;
	TestTrue(TEXT("Registered player world hit resolves one Impact"),
		TryResolveSwordImpact(
			Fixture.Coordinator,
			Action,
			Fixture.Pawn,
			Fixture.CollisionRoot,
			Vitality,
			Impact));

	const Fdemo_mapCombatImpactDeliveryResult First =
		Fixture.Coordinator.DeliverBasicSwordImpactToPlayer(Impact);
	TestTrue(TEXT("Coordinator commits resolved Impact to the player host"),
		First.IsSuccess()
			&& First.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation()
				== 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverBasicSwordImpactToPlayer(Impact);
	TestTrue(TEXT("Player receipt replay cannot double-commit"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation()
				== 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01IdentityTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01AuthoredIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01IdentityTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Player and M01 melee fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady);
	if (!Fixture.bReady || !EnemyFixture.bReady)
	{
		return false;
	}

	TestTrue(TEXT("Authored M01 enemy enters the shared Run registry"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic));
	const FGuid ExpectedEnemyId =
		Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
			CoordinatorRunA,
			*EnemyFixture.Definition);
	TestTrue(TEXT("Enemy identity uses Run plus authored SpawnMarkerId only"),
		ExpectedEnemyId.IsValid()
			&& ExpectedEnemyId
				== FShanmenWorldEntityIdFactory::MakeEntityId(
					CoordinatorRunA,
					EnemyFixture.Definition->SpawnMarkerId,
					0)
			&& EnemyFixture.Enemy->GetCombatEntityId() == ExpectedEnemyId);

	FGuid Resolved;
	UPrimitiveComponent* EnemyRoot = Cast<UPrimitiveComponent>(
		EnemyFixture.Enemy->GetRootComponent());
	TestTrue(TEXT("Registry resolves the M01 actor alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			EnemyFixture.Enemy,
			INDEX_NONE,
			Resolved)
			&& Resolved == ExpectedEnemyId);
	TestTrue(TEXT("Registry resolves the M01 collision-root alias"),
		EnemyRoot
			&& Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
				CoordinatorRunA,
				EnemyRoot,
				INDEX_NONE,
				Resolved)
			&& Resolved == ExpectedEnemyId);
	TestTrue(TEXT("Same authored actor registration is idempotent"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic));
	FM01MeleeEnemyFixture ConflictingFixture;
	TestTrue(TEXT("Second fixture with same authored identity initializes"),
		ConflictingFixture.bReady);
	TestFalse(TEXT("Duplicate authored spawn cannot claim another actor"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			ConflictingFixture.Enemy,
			Fixture.Diagnostic));

	TSet<FGuid> AuthoredEntityIds;
	AuthoredEntityIds.Add(ExpectedEnemyId);
	TArray<AActor*> OtherAuthoredActors;
	for (const Fdemo_mapM01EnemyDefinition& Definition :
		Fdemo_mapM01EnemyConfig::GetDefinitions())
	{
		if (Definition.SpawnMarkerId
			== EnemyFixture.Definition->SpawnMarkerId)
		{
			continue;
		}
		AActor* AuthoredActor = NewM01ProductActor(Definition);
		Udemo_mapM01EnemyIdentityComponent* AuthoredIdentity = AuthoredActor
			? NewObject<Udemo_mapM01EnemyIdentityComponent>(AuthoredActor)
			: nullptr;
		if (AuthoredActor && AuthoredIdentity)
		{
			AuthoredActor->AddInstanceComponent(AuthoredIdentity);
		}
		const FGuid AuthoredEntityId =
			Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
				CoordinatorRunA,
				Definition);
		const bool bRegistered = AuthoredActor
			&& AuthoredIdentity
			&& AuthoredIdentity->Configure(Definition)
			&& !AuthoredEntityIds.Contains(AuthoredEntityId)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				AuthoredActor,
				Fixture.Diagnostic);
		TestTrue(
			FString::Printf(
				TEXT("Authored M01 spawn registers uniquely: %s"),
				*Definition.SpawnMarkerId.ToString()),
			bRegistered);
		if (bRegistered)
		{
			AuthoredEntityIds.Add(AuthoredEntityId);
			OtherAuthoredActors.Add(AuthoredActor);
		}
	}
	TestTrue(TEXT("All 14 authored enemies share one Run registry"),
		Fixture.Coordinator.NumRegisteredM01Enemies() == 14
			&& AuthoredEntityIds.Num() == 14);
	int32 ExpectedMeleeVitalityHosts = 0;
	for (const Fdemo_mapM01EnemyDefinition& Definition :
		Fdemo_mapM01EnemyConfig::GetDefinitions())
	{
		if (Definition.Archetype
				== Edemo_mapM01EnemyArchetype::StandardSkirmisher
			|| Definition.Archetype
				== Edemo_mapM01EnemyArchetype::EliteStalker)
		{
			++ExpectedMeleeVitalityHosts;
		}
	}
	TestTrue(TEXT("Every migrated melee actor owns one vitality ledger"),
		ExpectedMeleeVitalityHosts > 0
			&& Fixture.Coordinator.NumVitalityBoundM01Enemies()
				== ExpectedMeleeVitalityHosts);
	TestTrue(TEXT("Exact Run release clears player and enemy vitality ledgers"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic)
			&& !Fixture.Health->IsCombatEntityBound()
			&& !EnemyFixture.Enemy->IsCombatEntityBound());

	TestTrue(TEXT("Same product actors enter a later Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	const FGuid SecondEnemyId =
		Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
			CoordinatorRunB,
			*EnemyFixture.Definition);
	TestTrue(TEXT("New Run changes enemy identity without random inputs"),
		SecondEnemyId.IsValid()
			&& SecondEnemyId != ExpectedEnemyId
			&& EnemyFixture.Enemy->GetCombatEntityId() == SecondEnemyId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01DeliveryTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01BasicSwordDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01DeliveryTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Player and M01 melee fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady);
	if (!Fixture.bReady || !EnemyFixture.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FShanmenCombatActionSnapshot Action = MakeCoordinatorAction(
		CoordinatorRunA,
		Fixture.Coordinator.GetPlayerEntityId());
	FShanmenTargetVitalitySnapshot Vitality;
	check(EnemyFixture.Enemy->TryCaptureCombatVitalitySnapshot(Vitality));
	FShanmenBasicSwordImpactReceipt Impact;
	TestTrue(TEXT("M01 collision root resolves the authored target Impact"),
		TryResolveSwordImpact(
			Fixture.Coordinator,
			Action,
			EnemyFixture.Enemy,
			Cast<UPrimitiveComponent>(EnemyFixture.Enemy->GetRootComponent()),
			Vitality,
			Impact));

	const float VitalityBefore = EnemyFixture.Enemy->GetCurrentVitality();
	const Fdemo_mapCombatImpactDeliveryResult First =
		Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
			Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("First resolved sword receipt commits once to M01 vitality"),
		First.IsSuccess()
			&& First.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 2.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 1
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
			Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("M01 receipt replay is visible but cannot double-apply"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 2.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 1
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);

	TestTrue(TEXT("Exact first Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Persistent hosts bind the next Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
			Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("Delayed old-Run receipt fails before enemy mutation"),
		DelayedOldRun.Error
				== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldRun.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 2.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 0
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 0
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);
	return true;
}

#endif
