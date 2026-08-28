#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "Components/BoxComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Pawn.h"
#include "demo_mapPlayerCombatCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	const FGuid CoordinatorRunA(0x54300001, 0, 0, 1);
	const FGuid CoordinatorRunB(0x54300002, 0, 0, 1);
	const FGuid CoordinatorOwnerId(0x54300003, 0, 0, 1);

	struct FPlayerCombatCoordinatorFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapPlayerCombatCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FPlayerCombatCoordinatorFixture()
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
		Capture.Content.Version = TEXT("0.0.10.P4.3");
		Capture.Content.Digest =
			TEXT("TEST-DIGEST-P4.3-PLAYER-COMBAT-COORDINATOR");
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPlayerCombatCoordinatorLifecycleTest,
	"Shanmen.0_0_10.Product.PlayerCombatCoordinator.RunLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPlayerCombatCoordinatorLifecycleTest::RunTest(const FString&)
{
	FPlayerCombatCoordinatorFixture Fixture;
	TestTrue(TEXT("Product player combat fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid ExpectedFirstId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunA,
			Fdemo_mapPlayerCombatCoordinator::PlayerSpawnSourceId(),
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
			Fdemo_mapPlayerCombatCoordinator::PlayerSpawnSourceId(),
			0);
	TestTrue(TEXT("New Run deterministically produces a distinct player identity"),
		ExpectedSecondId.IsValid()
			&& ExpectedSecondId != ExpectedFirstId
			&& Fixture.Coordinator.GetPlayerEntityId() == ExpectedSecondId
			&& Fixture.Health->GetCombatEntityId() == ExpectedSecondId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPlayerCombatCoordinatorBasicSwordDeliveryTest,
	"Shanmen.0_0_10.Product.PlayerCombatCoordinator.BasicSwordDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPlayerCombatCoordinatorBasicSwordDeliveryTest::RunTest(
	const FString&)
{
	FPlayerCombatCoordinatorFixture Fixture;
	TestTrue(TEXT("Product player combat fixture initializes"), Fixture.bReady);
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
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Phase;
	check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Phase));
	check(Runtime.TryAdvance(EShanmenCombatActionPhase::Startup, Phase));

	FShanmenBasicSwordOffenseSnapshot Offense;
	check(FShanmenBasicSwordOffenseSnapshot::TryCapture(1.0f, Offense));
	FShanmenBasicSwordExecution Sword;
	check(FShanmenBasicSwordExecution::TryCreate(
		Action,
		MakeCoordinatorSwordDefinition(),
		Offense,
		Sword));
	FShanmenWorldHitContext Context;
	check(Sword.TryBeginEmission(Runtime, Context));

	FHitResult WorldHit(
		Fixture.Pawn,
		Fixture.CollisionRoot,
		FVector(20.0, 0.0, 50.0),
		FVector::BackwardVector);
	WorldHit.ImpactPoint = FVector(20.0, 0.0, 50.0);
	WorldHit.ImpactNormal = FVector::BackwardVector;
	WorldHit.Item = 0;
	FShanmenHitCandidate Candidate;
	TestTrue(TEXT("World hit resolves the registered product player identity"),
		FShanmenWorldHitAdapter::TryFromSweep(
			Context,
			WorldHit,
			Fixture.Coordinator.GetEntityRegistry(),
			Candidate)
			&& Candidate.TargetEntityId
				== Fixture.Coordinator.GetPlayerEntityId());

	FShanmenTargetVitalitySnapshot Vitality;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Vitality));
	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenBasicSwordImpactReceipt Impact;
	TestTrue(TEXT("One accepted world candidate resolves one BasicSword Impact"),
		Sword.TryResolveCandidate(
			Runtime,
			Candidate,
			Vitality,
			Defense,
			Impact));
	FShanmenBasicSwordImpactReceipt DuplicateImpact;
	TestFalse(TEXT("Repeated callback cannot resolve a second candidate Impact"),
		Sword.TryResolveCandidate(
			Runtime,
			Candidate,
			Vitality,
			Defense,
			DuplicateImpact));

	const Fdemo_mapCombatImpactDeliveryResult First =
		Fixture.Coordinator.DeliverBasicSwordImpact(Impact);
	TestTrue(TEXT("Coordinator commits resolved Impact to the product host"),
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
		Fixture.Coordinator.DeliverBasicSwordImpact(Impact);
	TestTrue(TEXT("Receipt replay is successful but cannot double-commit"),
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
	TestTrue(TEXT("Emission closes after its single accepted candidate"),
		Sword.TryEndEmission(Runtime));
	TestTrue(TEXT("Exact Run release closes the product delivery gate"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Persistent player binds the next Run before new combat"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverBasicSwordImpact(Impact);
	TestTrue(TEXT("Delayed old-Run receipt is rejected before product mutation"),
		DelayedOldRun.Error
				== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldRun.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation()
				== 1);
	return true;
}

#endif
