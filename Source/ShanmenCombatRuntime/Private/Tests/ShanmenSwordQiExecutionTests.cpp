#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSwordQiExecution.h"

namespace
{
	const FGuid SwordQiRunId(0x58100001, 0, 0, 1);
	const FGuid SwordQiOwnerId(0x58100002, 0, 0, 1);
	const FGuid SwordQiSourceEntityId(0x58100003, 0, 0, 1);
	const FGuid SwordQiItemId(0x58100004, 0, 0, 1);
	const FGuid SwordQiTargetA(0x58100005, 0, 0, 1);
	const FGuid SwordQiTargetB(0x58100006, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeSwordQiAction(
		uint64 ActivationSequence = 1,
		bool bWithSwordItem = true)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = SwordQiRunId;
		Capture.OwnerId = SwordQiOwnerId;
		Capture.SourceEntityId = SwordQiSourceEntityId;
		Capture.SourceItemInstanceId = bWithSwordItem
			? SwordQiItemId
			: FGuid();
		Capture.ActionDefinitionId =
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P18.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P18.0");
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

	FShanmenSwordQiDefinitionCapture MakeSwordQiDefinitionCapture()
	{
		FShanmenSwordQiDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Sword.Qi.Basic01");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Qi.Basic01.r1");
		Capture.BaseDamage = 10.0f;
		Capture.AttackPowerCoefficient = 0.5f;
		Capture.FlightSpeed = 1200.0f;
		Capture.MaximumRange = 1600.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamageSpirit());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		return Capture;
	}

	FShanmenSwordQiDefinition MakeSwordQiDefinition()
	{
		FShanmenSwordQiDefinition Definition;
		check(FShanmenSwordQiDefinition::TryCapture(
			MakeSwordQiDefinitionCapture(), Definition));
		return Definition;
	}

	FShanmenSwordQiOffenseSnapshot MakeSwordQiOffense(
		float AttackPower = 40.0f)
	{
		FShanmenSwordQiOffenseSnapshot Offense;
		check(FShanmenSwordQiOffenseSnapshot::TryCapture(
			AttackPower, Offense));
		return Offense;
	}

	FShanmenTargetVitalitySnapshot MakeSwordQiVitality()
	{
		FShanmenTargetVitalitySnapshot Vitality;
		Vitality.CurrentVitality = 100.0f;
		Vitality.MaximumVitality = 100.0f;
		return Vitality;
	}

	FShanmenDefenseSnapshot MakeSwordQiDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}

	FShanmenHitCandidate MakeSwordQiCandidate(
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
		Candidate.HitLocation = FVector(500.0, 20.0, 70.0);
		Candidate.HitNormal = FVector::BackwardVector;
		return Candidate;
	}

	void StartActiveSwordQi(
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenSwordQiExecution& OutExecution,
		FShanmenActionTransitionReceipt& OutPhaseReceipt,
		const FShanmenCombatActionSnapshot& Action = MakeSwordQiAction())
	{
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutActionRuntime, OutPhaseReceipt));
		check(FShanmenSwordQiExecution::TryCreate(
			Action,
			MakeSwordQiDefinition(),
			MakeSwordQiOffense(),
			OutExecution));
		check(OutActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, OutPhaseReceipt));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordQiDefinitionAndSwordBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.SwordQi.DefinitionAndSwordBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordQiDefinitionAndSwordBoundaryTest::RunTest(
	const FString&)
{
	FShanmenSwordQiDefinition Definition;
	TestTrue(TEXT("Valid authored sword-qi content captures immutably"),
		FShanmenSwordQiDefinition::TryCapture(
			MakeSwordQiDefinitionCapture(), Definition));
	TestTrue(TEXT("Definition retains range, speed, and open damage channel"),
		Definition.IsValid()
			&& FMath::IsNearlyEqual(Definition.GetFlightSpeed(), 1200.0f)
			&& FMath::IsNearlyEqual(Definition.GetMaximumRange(), 1600.0f)
			&& Definition.GetDamageTags().HasTagExact(
				FShanmenCombatNativeTags::DamageSpirit()));

	FShanmenSwordQiDefinitionCapture Physical =
		MakeSwordQiDefinitionCapture();
	Physical.DamageTags.Reset();
	Physical.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	TestTrue(TEXT("Contract does not freeze sword qi to one damage channel"),
		FShanmenSwordQiDefinition::TryCapture(Physical, Definition));

	FShanmenSwordQiDefinitionCapture Invalid =
		MakeSwordQiDefinitionCapture();
	Invalid.ActionDefinitionId = TEXT("Combat.Action.Projectile.Generic");
	TestFalse(TEXT("Generic projectile identity cannot enter sword qi"),
		FShanmenSwordQiDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeSwordQiDefinitionCapture();
	Invalid.DamageTags.Reset();
	Invalid.DamageTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	TestFalse(TEXT("Damage tags must belong to the Damage tree"),
		FShanmenSwordQiDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeSwordQiDefinitionCapture();
	Invalid.FlightSpeed = 0.0f;
	TestFalse(TEXT("Content must author positive flight speed"),
		FShanmenSwordQiDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeSwordQiDefinitionCapture();
	Invalid.MaximumRange = 0.0f;
	TestFalse(TEXT("Content must author positive maximum range"),
		FShanmenSwordQiDefinition::TryCapture(Invalid, Definition));

	FShanmenSwordQiOffenseSnapshot Offense;
	TestTrue(TEXT("Zero attack power is a valid frozen snapshot"),
		FShanmenSwordQiOffenseSnapshot::TryCapture(0.0f, Offense)
			&& Offense.IsValid());
	TestFalse(TEXT("Negative attack power fails closed"),
		FShanmenSwordQiOffenseSnapshot::TryCapture(-1.0f, Offense));
	TestFalse(TEXT("Default offense is not an implicit snapshot"),
		FShanmenSwordQiOffenseSnapshot().IsValid());

	FShanmenSwordQiExecution Rejected;
	TestFalse(TEXT("Sword qi requires one exact source sword instance"),
		FShanmenSwordQiExecution::TryCreate(
			MakeSwordQiAction(1, false),
			MakeSwordQiDefinition(),
			MakeSwordQiOffense(),
			Rejected));
	TestTrue(TEXT("Exact sword prepares without spawning a world object"),
		FShanmenSwordQiExecution::TryCreate(
			MakeSwordQiAction(),
			MakeSwordQiDefinition(),
			MakeSwordQiOffense(),
			Rejected)
			&& Rejected.IsValid()
			&& Rejected.GetState() == EShanmenSwordQiState::Ready
			&& !Rejected.GetLaunchReceipt().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordQiLaunchLifecycleTest,
	"Shanmen.0_0_10.CombatRuntime.SwordQi.LaunchLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordQiLaunchLifecycleTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeSwordQiAction();
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	TestTrue(TEXT("Sword-qi action enters Startup"),
		FShanmenActionOrchestrator::TryStart(
			Action, ActionRuntime, PhaseReceipt));
	FShanmenSwordQiExecution Execution;
	TestTrue(TEXT("Frozen sword-qi execution prepares in Ready"),
		FShanmenSwordQiExecution::TryCreate(
			Action,
			MakeSwordQiDefinition(),
			MakeSwordQiOffense(),
			Execution));

	const FVector Origin(10.0, -0.0, 75.0);
	FShanmenSwordQiLaunchReceipt Launch;
	TestFalse(TEXT("Startup cannot release sword qi"),
		Execution.TryLaunch(
			ActionRuntime, Origin, FVector::ForwardVector, Launch));
	TestTrue(TEXT("Action crosses its canonical commit point"),
		ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, PhaseReceipt));
	TestFalse(TEXT("Zero aim direction cannot define flight"),
		Execution.TryLaunch(
			ActionRuntime, Origin, FVector::ZeroVector, Launch));
	TestTrue(TEXT("Active releases one canonical sword qi"),
		Execution.TryLaunch(
			ActionRuntime, Origin, FVector(10.0, 0.0, 0.0), Launch));
	const FShanmenSwordQiLaunchReceipt FirstLaunch = Launch;
	TestTrue(TEXT("Launch freezes sword, pose, direction, speed, and range"),
		Launch.IsValid()
			&& Launch.GetAction().GetSourceItemInstanceId() == SwordQiItemId
			&& Launch.GetOrigin().Equals(FVector(10.0, 0.0, 75.0))
			&& Launch.GetDirection().Equals(FVector::ForwardVector)
			&& FMath::IsNearlyEqual(Launch.GetSpeed(), 1200.0f)
			&& FMath::IsNearlyEqual(Launch.GetMaximumRange(), 1600.0f)
			&& Execution.GetState() == EShanmenSwordQiState::InFlight);
	TestTrue(TEXT("Equivalent aim scale replays the same launch"),
		Execution.TryLaunch(
			ActionRuntime, Origin, FVector::ForwardVector, Launch)
			&& Launch.GetLaunchId() == FirstLaunch.GetLaunchId());
	TestFalse(TEXT("In-flight sword qi cannot be redirected"),
		Execution.TryLaunch(
			ActionRuntime, Origin, FVector::RightVector, Launch));
	TestFalse(TEXT("Conflicting launch clears caller output"),
		Launch.IsValid());

	FShanmenWorldHitContext Context;
	TestTrue(TEXT("In-flight state opens a Projectile contact sample"),
		Execution.TryBeginEmission(ActionRuntime, Context)
			&& Context.GetDetectorKind()
				== EShanmenHitDetectorKind::Projectile);
	TestFalse(TEXT("Sword qi cannot dissipate during an open sample"),
		Execution.TryDissipate(ActionRuntime));
	TestTrue(TEXT("Contact sample closes explicitly"),
		Execution.TryEndEmission(ActionRuntime));
	TestTrue(TEXT("Closed flight may dissipate before Recovery"),
		Execution.TryDissipate(ActionRuntime));
	TestTrue(TEXT("Exact replay does not revive dissipated sword qi"),
		Execution.TryLaunch(
			ActionRuntime, Origin, FVector::ForwardVector, Launch)
			&& Launch.GetLaunchId() == FirstLaunch.GetLaunchId()
			&& Execution.GetState() == EShanmenSwordQiState::Dissipated);
	TestFalse(TEXT("Dissipated sword qi cannot emit again"),
		Execution.TryBeginEmission(ActionRuntime, Context));
	TestTrue(TEXT("Only dissipated sword qi lets the caller enter Recovery"),
		ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active, PhaseReceipt));
	TestTrue(TEXT("Recovery completes normally"),
		ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery, PhaseReceipt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordQiImpactPolicyTest,
	"Shanmen.0_0_10.CombatRuntime.SwordQi.ImpactPolicyAndOrdinals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordQiImpactPolicyTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSwordQiExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveSwordQi(ActionRuntime, Execution, PhaseReceipt);
	FShanmenSwordQiLaunchReceipt Launch;
	check(Execution.TryLaunch(
		ActionRuntime,
		FVector(5.0, 10.0, 60.0),
		FVector::ForwardVector,
		Launch));

	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("First projectile contact sample opens"),
		Execution.TryBeginEmission(ActionRuntime, FirstContext));
	FShanmenSwordQiImpactReceipt Impact;
	TestFalse(TEXT("Self target is rejected by target policy"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeSwordQiCandidate(FirstContext, SwordQiSourceEntityId),
			MakeSwordQiVitality(),
			MakeSwordQiDefense(),
			Impact));

	FShanmenDefenseSnapshot MissingLiving;
	const FShanmenHitCandidate TargetA =
		MakeSwordQiCandidate(FirstContext, SwordQiTargetA);
	TestFalse(TEXT("Missing Living tag is rejected without consumption"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetA,
			MakeSwordQiVitality(),
			MissingLiving,
			Impact));
	TestTrue(TEXT("Corrected target evidence resolves through CombatCore"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetA,
			MakeSwordQiVitality(),
			MakeSwordQiDefense(),
			Impact));
	const FGuid FirstImpactId = Impact.GetRequest().ImpactId;
	TestTrue(TEXT("Fixture computes 10 + 40 * 0.5 = 30 damage"),
		Impact.IsValid()
			&& FMath::IsNearlyEqual(Impact.GetResult().RawDamage, 30.0f)
			&& FMath::IsNearlyEqual(Impact.GetResult().FinalDamage, 30.0f));
	TestTrue(TEXT("Authored Spirit channel reaches the Impact request"),
		Impact.GetRequest().Damage.DamageTags.HasTagExact(
			FShanmenCombatNativeTags::DamageSpirit()));
	TestFalse(TEXT("Repeated projectile callback cannot resolve twice"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetA,
			MakeSwordQiVitality(),
			MakeSwordQiDefense(),
			Impact));
	TestTrue(TEXT("Second target shares sample but gets distinct identity"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeSwordQiCandidate(FirstContext, SwordQiTargetB),
			MakeSwordQiVitality(),
			MakeSwordQiDefense(),
			Impact)
			&& Impact.GetRequest().ImpactId != FirstImpactId);
	TestTrue(TEXT("First sample closes"),
		Execution.TryEndEmission(ActionRuntime));

	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("Later contact sample advances ordinal once"),
		Execution.TryBeginEmission(ActionRuntime, SecondContext)
			&& SecondContext.GetHitOrdinal() == 1);
	TestTrue(TEXT("Same target in later sample gets a new Impact identity"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeSwordQiCandidate(SecondContext, SwordQiTargetA),
			MakeSwordQiVitality(),
			MakeSwordQiDefense(),
			Impact)
			&& Impact.GetRequest().ImpactId != FirstImpactId);
	TestEqual(TEXT("Ledger owns three unique sword-qi impacts"),
		Execution.NumAcceptedImpacts(), 3);
	TestTrue(TEXT("Second sample closes before dissipation"),
		Execution.TryEndEmission(ActionRuntime)
			&& Execution.TryDissipate(ActionRuntime));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordQiDefenseReplayAndTerminationTest,
	"Shanmen.0_0_10.CombatRuntime.SwordQi.DefenseReplayAndTermination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordQiDefenseReplayAndTerminationTest::RunTest(
	const FString&)
{
	FShanmenDefenseSnapshot Defense = MakeSwordQiDefense();
	FShanmenDefenseLayer& Shield = Defense.Layers.AddDefaulted_GetRef();
	Shield.LayerId = FGuid(0x58100007, 0, 0, 1);
	Shield.RuleId = TEXT("Defense.Shield.TestSevenSpirit");
	Shield.Operation = EShanmenDefenseOperation::AbsorbPoints;
	Shield.Order = FShanmenDefenseOrder::Shield;
	Shield.Magnitude = 7.0f;
	Shield.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
	Shield.RequiredDamageTags.AddTag(
		FShanmenCombatNativeTags::DamageSpirit());
	TestTrue(TEXT("Spirit-only defense fixture is valid"), Defense.IsValid());

	FShanmenActionOrchestrator FirstAction;
	FShanmenSwordQiExecution FirstExecution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveSwordQi(FirstAction, FirstExecution, PhaseReceipt);
	FShanmenSwordQiLaunchReceipt FirstLaunch;
	check(FirstExecution.TryLaunch(
		FirstAction,
		FVector(1.0, 2.0, 3.0),
		FVector(2.0, 1.0, 0.0),
		FirstLaunch));
	FShanmenWorldHitContext FirstContext;
	check(FirstExecution.TryBeginEmission(FirstAction, FirstContext));
	FShanmenSwordQiImpactReceipt FirstImpact;
	TestTrue(TEXT("First defended sword-qi impact resolves"),
		FirstExecution.TryResolveCandidate(
			FirstAction,
			MakeSwordQiCandidate(FirstContext, SwordQiTargetA),
			MakeSwordQiVitality(),
			Defense,
			FirstImpact));
	TestTrue(TEXT("Spirit shield conserves 30 = 7 prevented + 23 final"),
		FMath::IsNearlyEqual(FirstImpact.GetResult().RawDamage, 30.0f)
			&& FMath::IsNearlyEqual(
				FirstImpact.GetResult().PreventedDamage, 7.0f)
			&& FMath::IsNearlyEqual(
				FirstImpact.GetResult().FinalDamage, 23.0f)
			&& FirstImpact.GetResult().IsConserved());

	FShanmenActionOrchestrator ReplayAction;
	FShanmenSwordQiExecution ReplayExecution;
	StartActiveSwordQi(ReplayAction, ReplayExecution, PhaseReceipt);
	FShanmenSwordQiLaunchReceipt ReplayLaunch;
	TestTrue(TEXT("Equivalent activation reproduces launch identity"),
		ReplayExecution.TryLaunch(
			ReplayAction,
			FVector(1.0, 2.0, 3.0),
			FVector(4.0, 2.0, 0.0),
			ReplayLaunch)
			&& ReplayLaunch.GetLaunchId() == FirstLaunch.GetLaunchId());
	FShanmenWorldHitContext ReplayContext;
	check(ReplayExecution.TryBeginEmission(ReplayAction, ReplayContext));
	FShanmenSwordQiImpactReceipt ReplayImpact;
	TestTrue(TEXT("Equivalent contact reproduces Impact and damage receipt"),
		ReplayExecution.TryResolveCandidate(
			ReplayAction,
			MakeSwordQiCandidate(ReplayContext, SwordQiTargetA),
			MakeSwordQiVitality(),
			Defense,
			ReplayImpact)
			&& ReplayImpact.GetRequest().ImpactId
				== FirstImpact.GetRequest().ImpactId
			&& FMath::IsNearlyEqual(
				ReplayImpact.GetResult().FinalDamage,
				FirstImpact.GetResult().FinalDamage));

	const FShanmenCombatActionSnapshot OtherActivation =
		MakeSwordQiAction(2);
	FShanmenActionOrchestrator OtherAction;
	FShanmenSwordQiExecution OtherExecution;
	StartActiveSwordQi(
		OtherAction, OtherExecution, PhaseReceipt, OtherActivation);
	FShanmenSwordQiLaunchReceipt OtherLaunch;
	TestTrue(TEXT("Different activation produces different launch identity"),
		OtherExecution.TryLaunch(
			OtherAction,
			FVector(1.0, 2.0, 3.0),
			FVector(2.0, 1.0, 0.0),
			OtherLaunch)
			&& OtherLaunch.GetLaunchId() != FirstLaunch.GetLaunchId());

	TestTrue(TEXT("Action termination may race an open contact sample"),
		FirstAction.TryInterrupt(
			EShanmenCombatActionPhase::Active, PhaseReceipt));
	TestFalse(TEXT("Terminal action cannot use active-only close"),
		FirstExecution.TryEndEmission(FirstAction));
	FirstExecution.EndForActionTermination();
	TestTrue(TEXT("Termination closes sample and dissipates in-flight state"),
		FirstExecution.IsValid()
			&& !FirstExecution.IsEmissionActive()
			&& FirstExecution.GetState()
				== EShanmenSwordQiState::Dissipated);
	TestFalse(TEXT("Terminal action cannot relaunch or reopen contacts"),
		FirstExecution.TryLaunch(
			FirstAction,
			FVector(1.0, 2.0, 3.0),
			FVector(2.0, 1.0, 0.0),
			FirstLaunch)
			|| FirstExecution.TryBeginEmission(FirstAction, FirstContext));
	return true;
}

#endif
