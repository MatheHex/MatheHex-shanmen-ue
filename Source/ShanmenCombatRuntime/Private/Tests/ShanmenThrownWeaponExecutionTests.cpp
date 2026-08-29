#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenThrownWeaponExecution.h"

namespace
{
	const FGuid ThrownRunId(0x57000001, 0, 0, 1);
	const FGuid ThrownOwnerId(0x57000002, 0, 0, 1);
	const FGuid ThrownSourceEntityId(0x57000003, 0, 0, 1);
	const FGuid ThrownItemId(0x57000004, 0, 0, 1);
	const FGuid ThrownTargetA(0x57000005, 0, 0, 1);
	const FGuid ThrownTargetB(0x57000006, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeThrownAction(
		bool bWithItem = true,
		const FString& Digest = TEXT("TEST-DIGEST-P7.0"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ThrownRunId;
		Capture.OwnerId = ThrownOwnerId;
		Capture.SourceEntityId = ThrownSourceEntityId;
		Capture.SourceItemInstanceId = bWithItem
			? ThrownItemId
			: FGuid();
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P7.0");
		Capture.Content.Digest = Digest;
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			7);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponDefinitionCapture MakeThrownDefinitionCapture()
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.Straight01");
		Capture.FormulaId = TEXT("Combat.Formula.ThrownWeapon.Straight01.r1");
		Capture.BaseDamage = 12.0f;
		Capture.TechniquePowerCoefficient = 0.3f;
		Capture.LaunchSpeed = 900.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		return Capture;
	}

	FShanmenThrownWeaponDefinition MakeThrownDefinition()
	{
		FShanmenThrownWeaponDefinition Definition;
		check(FShanmenThrownWeaponDefinition::TryCapture(
			MakeThrownDefinitionCapture(), Definition));
		return Definition;
	}

	FShanmenThrownWeaponOffenseSnapshot MakeThrownOffense()
	{
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			40.0f, Offense));
		return Offense;
	}

	FShanmenTargetVitalitySnapshot MakeThrownVitality()
	{
		FShanmenTargetVitalitySnapshot Vitality;
		Vitality.CurrentVitality = 100.0f;
		Vitality.MaximumVitality = 100.0f;
		return Vitality;
	}

	FShanmenDefenseSnapshot MakeThrownDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}

	FShanmenHitCandidate MakeThrownCandidate(
		const FShanmenWorldHitContext& Context,
		const FGuid& TargetEntityId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = Context.GetAction().GetActivationId();
		Candidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
		Candidate.TargetEntityId = TargetEntityId;
		Candidate.DetectorId = Context.GetDetectorId();
		Candidate.DetectorKind = Context.GetDetectorKind();
		Candidate.HitOrdinal = Context.GetHitOrdinal();
		Candidate.HitLocation = FVector(250.0, 10.0, 70.0);
		Candidate.HitNormal = FVector::BackwardVector;
		return Candidate;
	}

	void StartActiveThrownWeapon(
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenThrownWeaponExecution& OutExecution,
		FShanmenActionTransitionReceipt& OutPhaseReceipt,
		const FShanmenCombatActionSnapshot& Action = MakeThrownAction())
	{
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutActionRuntime, OutPhaseReceipt));
		check(FShanmenThrownWeaponExecution::TryCreate(
			Action,
			MakeThrownDefinition(),
			MakeThrownOffense(),
			OutExecution));
		check(OutActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, OutPhaseReceipt));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponPhysicalItemBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeapon.PhysicalItemBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponPhysicalItemBoundaryTest::RunTest(const FString&)
{
	FShanmenThrownWeaponExecution Rejected;
	TestFalse(TEXT("A thrown weapon requires one exact physical source item"),
		FShanmenThrownWeaponExecution::TryCreate(
			MakeThrownAction(false),
			MakeThrownDefinition(),
			MakeThrownOffense(),
			Rejected));
	TestFalse(TEXT("Default technique power is not a frozen snapshot"),
		FShanmenThrownWeaponOffenseSnapshot().IsValid());

	FShanmenThrownWeaponDefinitionCapture Invalid =
		MakeThrownDefinitionCapture();
	Invalid.ActionDefinitionId = TEXT("Combat.Action.Projectile.Generic");
	FShanmenThrownWeaponDefinition InvalidDefinition;
	TestFalse(TEXT("A generic projectile cannot enter the thrown-item contract"),
		FShanmenThrownWeaponDefinition::TryCapture(
			Invalid, InvalidDefinition));
	Invalid = MakeThrownDefinitionCapture();
	Invalid.LaunchSpeed = 0.0f;
	TestFalse(TEXT("Content must author a positive straight-flight speed"),
		FShanmenThrownWeaponDefinition::TryCapture(
			Invalid, InvalidDefinition));

	FShanmenThrownWeaponExecution Accepted;
	TestTrue(TEXT("Valid content prepares the exact item without launching it"),
		FShanmenThrownWeaponExecution::TryCreate(
			MakeThrownAction(),
			MakeThrownDefinition(),
			MakeThrownOffense(),
			Accepted));
	TestTrue(TEXT("Prepared state retains identity and has no launch proof"),
		Accepted.IsValid()
			&& Accepted.GetState() == EShanmenThrownWeaponState::Ready
			&& Accepted.GetAction().GetSourceItemInstanceId() == ThrownItemId
			&& !Accepted.GetLaunchReceipt().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponStraightLaunchTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeapon.StraightLaunchAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponStraightLaunchTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeThrownAction();
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	TestTrue(TEXT("Thrown action enters Startup"),
		FShanmenActionOrchestrator::TryStart(
			Action, ActionRuntime, PhaseReceipt));
	FShanmenThrownWeaponExecution Execution;
	TestTrue(TEXT("Exact item prepares before the commit point"),
		FShanmenThrownWeaponExecution::TryCreate(
			Action,
			MakeThrownDefinition(),
			MakeThrownOffense(),
			Execution));

	const FVector Origin(10.0, -0.0, 75.0);
	FShanmenThrownWeaponLaunchReceipt Launch;
	TestFalse(TEXT("Startup cannot launch the physical item"),
		Execution.TryLaunchStraight(
			ActionRuntime, Origin, FVector::ForwardVector, Launch));
	TestTrue(TEXT("Action crosses the existing commit point"),
		ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, PhaseReceipt));
	TestFalse(TEXT("Zero direction is never a straight trajectory"),
		Execution.TryLaunchStraight(
			ActionRuntime, Origin, FVector::ZeroVector, Launch));
	TestTrue(TEXT("Active captures one canonical straight launch"),
		Execution.TryLaunchStraight(
			ActionRuntime, Origin, FVector(10.0, 0.0, 0.0), Launch));
	const FShanmenThrownWeaponLaunchReceipt FirstLaunch = Launch;
	TestTrue(TEXT("Launch proof freezes exact item, pose, direction, and speed"),
		Launch.IsValid()
			&& Launch.GetAction().GetSourceItemInstanceId() == ThrownItemId
			&& Launch.GetOrigin().Equals(FVector(10.0, 0.0, 75.0))
			&& Launch.GetDirection().Equals(FVector::ForwardVector)
			&& FMath::IsNearlyEqual(Launch.GetSpeed(), 900.0f)
			&& Execution.GetState() == EShanmenThrownWeaponState::InFlight);
	TestTrue(TEXT("Equivalent aim scale replays the original launch exactly"),
		Execution.TryLaunchStraight(
			ActionRuntime, Origin, FVector(1.0, 0.0, 0.0), Launch)
			&& Launch.GetLaunchId() == FirstLaunch.GetLaunchId());
	TestFalse(TEXT("A second direction cannot steer an in-flight thrown item"),
		Execution.TryLaunchStraight(
			ActionRuntime, Origin, FVector::RightVector, Launch));
	TestFalse(TEXT("A second origin cannot replace the frozen launch"),
		Execution.TryLaunchStraight(
			ActionRuntime,
			Origin + FVector::UpVector,
			FVector::ForwardVector,
			Launch));

	FShanmenWorldHitContext Context;
	TestTrue(TEXT("In-flight state can open a projectile contact sample"),
		Execution.TryBeginEmission(ActionRuntime, Context));
	TestFalse(TEXT("Flight cannot finish while a contact sample is open"),
		Execution.TryFinishFlight(ActionRuntime));
	TestTrue(TEXT("Contact sample closes explicitly"),
		Execution.TryEndEmission(ActionRuntime));
	TestTrue(TEXT("Closed in-flight execution can become spent"),
		Execution.TryFinishFlight(ActionRuntime));
	TestTrue(TEXT("Exact launch replay is idempotent and does not revive flight"),
		Execution.TryLaunchStraight(
			ActionRuntime, Origin, FVector::ForwardVector, Launch)
			&& Launch.GetLaunchId() == FirstLaunch.GetLaunchId()
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent);
	TestFalse(TEXT("Spent execution cannot emit another contact sample"),
		Execution.TryBeginEmission(ActionRuntime, Context));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponImpactPolicyTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeapon.ImpactPolicyAndOrdinals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponImpactPolicyTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveThrownWeapon(ActionRuntime, Execution, PhaseReceipt);
	FShanmenThrownWeaponLaunchReceipt Launch;
	TestTrue(TEXT("Straight item launches before contact evaluation"),
		Execution.TryLaunchStraight(
			ActionRuntime,
			FVector(5.0, 10.0, 60.0),
			FVector::ForwardVector,
			Launch));

	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("First projectile emission opens"),
		Execution.TryBeginEmission(ActionRuntime, FirstContext));
	TestTrue(TEXT("Detector kind remains Projectile, not ControlledObject"),
		FirstContext.GetDetectorKind() == EShanmenHitDetectorKind::Projectile);
	FShanmenThrownWeaponImpactReceipt Impact;
	TestFalse(TEXT("Self target is rejected by policy"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeThrownCandidate(FirstContext, ThrownSourceEntityId),
			MakeThrownVitality(),
			MakeThrownDefense(),
			Impact));

	FShanmenDefenseSnapshot MissingLiving;
	const FShanmenHitCandidate TargetA =
		MakeThrownCandidate(FirstContext, ThrownTargetA);
	TestFalse(TEXT("Missing target tag is rejected without consuming contact"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetA,
			MakeThrownVitality(),
			MissingLiving,
			Impact));
	TestTrue(TEXT("Corrected target evidence resolves through CombatCore"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetA,
			MakeThrownVitality(),
			MakeThrownDefense(),
			Impact));
	const FGuid FirstImpactId = Impact.GetRequest().ImpactId;
	TestTrue(TEXT("Formula freezes 12 + 40 * 0.3 = 24 raw damage"),
		Impact.IsValid()
			&& FMath::IsNearlyEqual(Impact.GetResult().RawDamage, 24.0f)
			&& FMath::IsNearlyEqual(Impact.GetResult().FinalDamage, 24.0f));
	TestFalse(TEXT("Repeated projectile callback cannot resolve twice"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetA,
			MakeThrownVitality(),
			MakeThrownDefense(),
			Impact));
	TestTrue(TEXT("A second target shares the sample but not Impact identity"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeThrownCandidate(FirstContext, ThrownTargetB),
			MakeThrownVitality(),
			MakeThrownDefense(),
			Impact)
			&& Impact.GetRequest().ImpactId != FirstImpactId);
	TestTrue(TEXT("First sample closes"),
		Execution.TryEndEmission(ActionRuntime));

	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("A later physical sample advances ordinal once"),
		Execution.TryBeginEmission(ActionRuntime, SecondContext)
			&& SecondContext.GetHitOrdinal() == 1);
	TestTrue(TEXT("Same target in a later sample has a new Impact identity"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeThrownCandidate(SecondContext, ThrownTargetA),
			MakeThrownVitality(),
			MakeThrownDefense(),
			Impact)
			&& Impact.GetRequest().ImpactId != FirstImpactId);
	TestEqual(TEXT("Ledger owns three unique impacts"),
		Execution.NumAcceptedImpacts(), 3);
	TestTrue(TEXT("Second sample closes before flight completion"),
		Execution.TryEndEmission(ActionRuntime)
			&& Execution.TryFinishFlight(ActionRuntime));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponDeterminismAndTerminationTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeapon.DeterminismAndTermination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponDeterminismAndTerminationTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator FirstAction;
	FShanmenThrownWeaponExecution FirstExecution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveThrownWeapon(FirstAction, FirstExecution, PhaseReceipt);
	FShanmenThrownWeaponLaunchReceipt FirstLaunch;
	TestTrue(TEXT("First deterministic launch succeeds"),
		FirstExecution.TryLaunchStraight(
			FirstAction,
			FVector(1.0, 2.0, 3.0),
			FVector(2.0, 1.0, 0.0),
			FirstLaunch));
	FShanmenWorldHitContext FirstContext;
	check(FirstExecution.TryBeginEmission(FirstAction, FirstContext));
	FShanmenThrownWeaponImpactReceipt FirstImpact;
	check(FirstExecution.TryResolveCandidate(
		FirstAction,
		MakeThrownCandidate(FirstContext, ThrownTargetA),
		MakeThrownVitality(),
		MakeThrownDefense(),
		FirstImpact));

	FShanmenActionOrchestrator ReplayAction;
	FShanmenThrownWeaponExecution ReplayExecution;
	StartActiveThrownWeapon(ReplayAction, ReplayExecution, PhaseReceipt);
	FShanmenThrownWeaponLaunchReceipt ReplayLaunch;
	TestTrue(TEXT("Equivalent frozen activation reproduces launch identity"),
		ReplayExecution.TryLaunchStraight(
			ReplayAction,
			FVector(1.0, 2.0, 3.0),
			FVector(4.0, 2.0, 0.0),
			ReplayLaunch)
			&& ReplayLaunch.GetLaunchId() == FirstLaunch.GetLaunchId());
	FShanmenWorldHitContext ReplayContext;
	check(ReplayExecution.TryBeginEmission(ReplayAction, ReplayContext));
	FShanmenThrownWeaponImpactReceipt ReplayImpact;
	TestTrue(TEXT("Equivalent contact reproduces canonical Impact identity"),
		ReplayExecution.TryResolveCandidate(
			ReplayAction,
			MakeThrownCandidate(ReplayContext, ThrownTargetA),
			MakeThrownVitality(),
			MakeThrownDefense(),
			ReplayImpact)
			&& ReplayImpact.GetRequest().ImpactId
				== FirstImpact.GetRequest().ImpactId);

	const FShanmenCombatActionSnapshot OtherContent =
		MakeThrownAction(true, TEXT("TEST-DIGEST-P7.0-OTHER"));
	FShanmenActionOrchestrator OtherAction;
	FShanmenThrownWeaponExecution OtherExecution;
	StartActiveThrownWeapon(
		OtherAction, OtherExecution, PhaseReceipt, OtherContent);
	FShanmenThrownWeaponLaunchReceipt OtherLaunch;
	TestTrue(TEXT("Content stamp participates in launch identity"),
		OtherExecution.TryLaunchStraight(
			OtherAction,
			FVector(1.0, 2.0, 3.0),
			FVector(2.0, 1.0, 0.0),
			OtherLaunch)
			&& OtherLaunch.GetLaunchId() != FirstLaunch.GetLaunchId());

	TestTrue(TEXT("Terminal transition may race an open projectile sample"),
		FirstAction.TryInterrupt(
			EShanmenCombatActionPhase::Active, PhaseReceipt));
	TestFalse(TEXT("Terminal action cannot use active-only close"),
		FirstExecution.TryEndEmission(FirstAction));
	FirstExecution.EndEmissionForTermination();
	TestFalse(TEXT("Termination cleanup closes the sample"),
		FirstExecution.IsEmissionActive());
	TestFalse(TEXT("Terminal action cannot reopen or finish flight"),
		FirstExecution.TryBeginEmission(FirstAction, FirstContext)
			|| FirstExecution.TryFinishFlight(FirstAction));
	return true;
}

#endif
