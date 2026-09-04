#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenThrownWeaponExecution.h"

namespace
{
	const FGuid ArcExecutionRunId(0x20100001, 0, 0, 1);
	const FGuid ArcExecutionOwnerId(0x20100002, 0, 0, 1);
	const FGuid ArcExecutionSourceId(0x20100003, 0, 0, 1);
	const FGuid ArcExecutionItemId(0x20100004, 0, 0, 1);
	const FGuid ArcExecutionTargetId(0x20100005, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeArcExecutionAction(
		const FString& Digest = TEXT("TEST-DIGEST-P20.1"),
		int32 ActivationOrdinal = 20)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ArcExecutionRunId;
		Capture.OwnerId = ArcExecutionOwnerId;
		Capture.SourceEntityId = ArcExecutionSourceId;
		Capture.SourceItemInstanceId = ArcExecutionItemId;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P20.1");
		Capture.Content.Digest = Digest;
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationOrdinal);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponDefinition MakeArcExecutionDefinition(
		float MaximumLaunchSpeed = 2000.0f)
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.Arc01");
		Capture.FormulaId = TEXT("Combat.Formula.ThrownWeapon.Arc01.r1");
		Capture.BaseDamage = 12.0f;
		Capture.TechniquePowerCoefficient = 0.3f;
		Capture.LaunchSpeed = MaximumLaunchSpeed;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;

		FShanmenThrownWeaponDefinition Definition;
		check(FShanmenThrownWeaponDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenThrownWeaponArcPlan MakeArcExecutionPlan(
		const FShanmenCombatActionSnapshot& Action,
		double MaximumLaunchSpeed = 2000.0,
		double ApexClearance = 250.0)
	{
		FShanmenThrownWeaponArcRequestCapture Capture;
		Capture.Action = Action;
		Capture.TechniqueTier =
			EShanmenThrownWeaponTechniqueTier::Intermediate;
		Capture.Origin = FVector(10.0, -20.0, 75.0);
		Capture.Target = FVector(1010.0, 180.0, 75.0);
		Capture.GravityMagnitude = 980.0;
		Capture.ApexClearance = ApexClearance;
		Capture.MaximumLaunchSpeed = MaximumLaunchSpeed;
		Capture.MaximumFlightTime = 10.0;

		const FShanmenThrownWeaponArcPlanResult Result =
			FShanmenThrownWeaponArcPlanner::Plan(Capture);
		check(Result.IsPlanned());
		return Result.Plan;
	}

	FShanmenThrownWeaponOffenseSnapshot MakeArcExecutionOffense()
	{
		FShanmenThrownWeaponOffenseSnapshot Offense;
		check(FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			40.0f, Offense));
		return Offense;
	}

	void PrepareArcExecution(
		const FShanmenCombatActionSnapshot& Action,
		float MaximumLaunchSpeed,
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenThrownWeaponExecution& OutExecution,
		FShanmenActionTransitionReceipt& OutPhaseReceipt,
		bool bAdvanceToActive = true)
	{
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutActionRuntime, OutPhaseReceipt));
		check(FShanmenThrownWeaponExecution::TryCreate(
			Action,
			MakeArcExecutionDefinition(MaximumLaunchSpeed),
			MakeArcExecutionOffense(),
			OutExecution));
		if (bAdvanceToActive)
		{
			check(OutActionRuntime.TryAdvance(
				EShanmenCombatActionPhase::Startup, OutPhaseReceipt));
		}
	}

	FShanmenHitCandidate MakeArcExecutionCandidate(
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
		Candidate.HitLocation = FVector(900.0, 150.0, 90.0);
		Candidate.HitNormal = FVector::BackwardVector;
		return Candidate;
	}

	FShanmenTargetVitalitySnapshot MakeArcExecutionVitality()
	{
		FShanmenTargetVitalitySnapshot Vitality;
		Vitality.CurrentVitality = 100.0f;
		Vitality.MaximumVitality = 100.0f;
		return Vitality;
	}

	FShanmenDefenseSnapshot MakeArcExecutionDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcExecutionBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArcExecution.ActionBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcExecutionBoundaryTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot ArcAction = MakeArcExecutionAction();
	const FShanmenThrownWeaponDefinition ArcDefinition =
		MakeArcExecutionDefinition();
	TestTrue(TEXT("Arc content is admitted by the shared thrown authority"),
		ArcDefinition.IsValid()
			&& ArcDefinition.GetActionDefinitionId()
				== FShanmenThrownWeaponDefinition::ArcActionDefinitionId());

	FShanmenThrownWeaponDefinitionCapture InvalidCapture;
	InvalidCapture.ActionDefinitionId = TEXT("Combat.Action.Projectile.Generic");
	InvalidCapture.DetectorId = TEXT("Detector.Invalid");
	InvalidCapture.FormulaId = TEXT("Formula.Invalid");
	InvalidCapture.BaseDamage = 1.0f;
	InvalidCapture.LaunchSpeed = 1.0f;
	InvalidCapture.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	InvalidCapture.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	FShanmenThrownWeaponDefinition InvalidDefinition;
	TestFalse(TEXT("Generic projectile actions remain outside the contract"),
		FShanmenThrownWeaponDefinition::TryCapture(
			InvalidCapture, InvalidDefinition));

	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	PrepareArcExecution(
		ArcAction, 2000.0f, Runtime, Execution, PhaseReceipt);
	FShanmenThrownWeaponLaunchReceipt Receipt;
	TestFalse(TEXT("An arc action cannot enter through the straight API"),
		Execution.TryLaunchStraight(
			Runtime,
			FVector::ZeroVector,
			FVector::ForwardVector,
			Receipt));
	TestFalse(TEXT("A missing ballistic plan fails closed"),
		Execution.TryLaunchArc(
			Runtime, FShanmenThrownWeaponArcPlan(), Receipt));
	TestTrue(TEXT("Rejected launch attempts leave execution ready"),
		Execution.IsValid()
			&& Execution.GetState() == EShanmenThrownWeaponState::Ready
			&& !Execution.GetLaunchReceipt().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcExecutionLaunchTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArcExecution.LaunchAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcExecutionLaunchTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeArcExecutionAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	PrepareArcExecution(Action, 2000.0f, Runtime, Execution, PhaseReceipt);
	const FShanmenThrownWeaponArcPlan Plan = MakeArcExecutionPlan(Action);

	FShanmenThrownWeaponLaunchReceipt Receipt;
	TestTrue(TEXT("One planned arc enters the shared in-flight state"),
		Execution.TryLaunchArc(Runtime, Plan, Receipt));
	const FShanmenThrownWeaponLaunchReceipt FirstReceipt = Receipt;
	TestTrue(TEXT("Receipt embeds the exact self-validating motion proof"),
		Receipt.IsValid()
			&& Receipt.GetTrajectoryKind()
				== EShanmenThrownWeaponTrajectoryKind::BallisticArc
			&& Receipt.GetArcPlan().Matches(Plan)
			&& Receipt.GetOrigin() == Plan.GetRequest().GetOrigin()
			&& Receipt.GetInitialVelocity() == Plan.GetInitialVelocity()
			&& Receipt.GetGravityAcceleration()
				== Plan.GetGravityAcceleration()
			&& FMath::IsNearlyEqual(
				Receipt.GetFlightTimeSeconds(),
				Plan.GetFlightTimeSeconds())
			&& Execution.GetState() == EShanmenThrownWeaponState::InFlight);

	FVector TerminalPosition = FVector::ZeroVector;
	TestTrue(TEXT("Embedded plan still reconstructs the authored target"),
		Receipt.GetArcPlan().TrySamplePosition(
			Receipt.GetFlightTimeSeconds(), TerminalPosition)
			&& TerminalPosition.Equals(
				Plan.GetRequest().GetTarget(), 1.0e-6));
	TestTrue(TEXT("Equivalent plan replay returns the same launch identity"),
		Execution.TryLaunchArc(Runtime, Plan, Receipt)
			&& Receipt.GetLaunchId() == FirstReceipt.GetLaunchId());
	const FShanmenThrownWeaponArcPlan OtherPlan =
		MakeArcExecutionPlan(Action, 2000.0, 350.0);
	TestFalse(TEXT("A second plan cannot redirect an in-flight item"),
		Execution.TryLaunchArc(Runtime, OtherPlan, Receipt));

	FShanmenWorldHitContext Context;
	TestTrue(TEXT("Arc flight opens the existing projectile sample"),
		Execution.TryBeginEmission(Runtime, Context));
	TestTrue(TEXT("Sample closes before flight completion"),
		Execution.TryEndEmission(Runtime)
			&& Execution.TryFinishFlight(Runtime));
	TestTrue(TEXT("Exact replay is idempotent and cannot revive spent flight"),
		Execution.TryLaunchArc(Runtime, Plan, Receipt)
			&& Receipt.GetLaunchId() == FirstReceipt.GetLaunchId()
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcExecutionIdentityFenceTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArcExecution.IdentityAndEnvelopeFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcExecutionIdentityFenceTest::RunTest(
	const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeArcExecutionAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	PrepareArcExecution(
		Action, 1200.0f, Runtime, Execution, PhaseReceipt, false);
	const FShanmenThrownWeaponArcPlan CompatiblePlan =
		MakeArcExecutionPlan(Action, 1200.0);
	FShanmenThrownWeaponLaunchReceipt Receipt;
	TestFalse(TEXT("Startup cannot publish a ballistic launch"),
		Execution.TryLaunchArc(Runtime, CompatiblePlan, Receipt));
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Startup, PhaseReceipt));

	const FShanmenThrownWeaponArcPlan OtherActionPlan =
		MakeArcExecutionPlan(
			MakeArcExecutionAction(TEXT("TEST-DIGEST-P20.1-OTHER")));
	TestFalse(TEXT("A plan from another frozen content identity is rejected"),
		Execution.TryLaunchArc(Runtime, OtherActionPlan, Receipt));
	const FShanmenThrownWeaponArcPlan OverbroadEnvelopePlan =
		MakeArcExecutionPlan(Action, 2000.0);
	TestTrue(TEXT("Fixture proves actual speed fits the runtime ceiling"),
		OverbroadEnvelopePlan.GetLaunchSpeed() < 1200.0);
	TestFalse(TEXT("A plan cannot widen the content-owned speed envelope"),
		Execution.TryLaunchArc(Runtime, OverbroadEnvelopePlan, Receipt));
	TestTrue(TEXT("The same geometry launches under the exact envelope"),
		Execution.TryLaunchArc(Runtime, CompatiblePlan, Receipt)
			&& Receipt.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcExecutionImpactTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArcExecution.SharedImpactAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcExecutionImpactTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeArcExecutionAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenThrownWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	PrepareArcExecution(Action, 2000.0f, Runtime, Execution, PhaseReceipt);
	FShanmenThrownWeaponLaunchReceipt Launch;
	check(Execution.TryLaunchArc(
		Runtime, MakeArcExecutionPlan(Action), Launch));

	FShanmenWorldHitContext Context;
	check(Execution.TryBeginEmission(Runtime, Context));
	FShanmenThrownWeaponImpactReceipt Impact;
	TestFalse(TEXT("Shared target policy still rejects the source entity"),
		Execution.TryResolveCandidate(
			Runtime,
			MakeArcExecutionCandidate(Context, ArcExecutionSourceId),
			MakeArcExecutionVitality(),
			MakeArcExecutionDefense(),
			Impact));
	const FShanmenHitCandidate Target =
		MakeArcExecutionCandidate(Context, ArcExecutionTargetId);
	TestTrue(TEXT("Arc contact resolves through the existing impact authority"),
		Execution.TryResolveCandidate(
			Runtime,
			Target,
			MakeArcExecutionVitality(),
			MakeArcExecutionDefense(),
			Impact)
			&& Impact.IsValid()
			&& FMath::IsNearlyEqual(Impact.GetResult().RawDamage, 24.0f)
			&& FMath::IsNearlyEqual(Impact.GetResult().FinalDamage, 24.0f)
			&& Execution.NumAcceptedImpacts() == 1);
	TestFalse(TEXT("Shared impact ledger rejects duplicate callbacks"),
		Execution.TryResolveCandidate(
			Runtime,
			Target,
			MakeArcExecutionVitality(),
			MakeArcExecutionDefense(),
			Impact));
	TestTrue(TEXT("Arc flight terminates through the existing lifecycle"),
		Execution.TryEndEmission(Runtime)
			&& Execution.TryFinishFlight(Runtime)
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent);
	return true;
}

#endif
