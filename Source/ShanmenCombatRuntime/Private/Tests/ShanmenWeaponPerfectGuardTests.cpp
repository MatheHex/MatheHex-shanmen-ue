#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWeaponPerfectGuard.h"

namespace
{
	const FGuid PerfectRunId(
		0xDC100001, 0xDC100002, 0xDC100003, 0xDC100004);
	const FGuid PerfectOwnerId(
		0xDC110001, 0xDC110002, 0xDC110003, 0xDC110004);
	const FGuid PerfectSourceId(
		0xDC120001, 0xDC120002, 0xDC120003, 0xDC120004);
	const FGuid PerfectWeaponId(
		0xDC130001, 0xDC130002, 0xDC130003, 0xDC130004);
	const FGuid PerfectTimelineId(
		0xDC140001, 0xDC140002, 0xDC140003, 0xDC140004);
	const FGuid PerfectAttackSourceId(
		0xDC150001, 0xDC150002, 0xDC150003, 0xDC150004);

	FShanmenCombatActionSnapshot MakeGuardAction(
		const FString& Digest = TEXT("TEST-DIGEST-P11.1-GUARD"),
		uint64 ActivationSequence = 1110)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = PerfectRunId;
		Capture.OwnerId = PerfectOwnerId;
		Capture.SourceEntityId = PerfectSourceId;
		Capture.SourceItemInstanceId = PerfectWeaponId;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.1");
		Capture.Content.Digest = Digest;
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

	FShanmenWeaponGuardDefinition MakeDefinition()
	{
		FShanmenWeaponGuardDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Sword.WeaponGuard01");
		Capture.GuardFraction = 0.25f;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.BlockedDamageTags.AddTag(
			FShanmenCombatNativeTags::DamageMental());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	void OpenWindow(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenWeaponGuardWindow& OutWindow,
		FShanmenWeaponGuardWindowReceipt& OutOpen)
	{
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Commit));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		check(FShanmenWeaponGuardWindow::TryOpen(
			Action,
			MakeDefinition(),
			Commit,
			OutRuntime,
			OutWindow,
			OutOpen));
	}

	FShanmenWeaponPerfectGuardPolicy MakePolicy(
		const FShanmenWeaponGuardWindowReceipt& Open,
		int64 PerfectEndTick = 15,
		FName PerfectRuleId = TEXT("Defense.Sword.PerfectGuard01"))
	{
		FShanmenWeaponPerfectGuardPolicy Policy;
		check(FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open,
			PerfectTimelineId,
			10,
			PerfectEndTick,
			PerfectRuleId,
			Policy));
		return Policy;
	}

	FShanmenWeaponGuardTimelineObservation MakeObservation(
		int64 Tick,
		const FGuid& TimelineId = PerfectTimelineId)
	{
		FShanmenWeaponGuardTimelineObservation Observation;
		check(FShanmenWeaponGuardTimelineObservation::TryCapture(
			TimelineId, Tick, Observation));
		return Observation;
	}

	FShanmenCombatActionSnapshot MakeAttackAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = PerfectRunId;
		Capture.OwnerId = FGuid(
			0xDC160001, 0xDC160002, 0xDC160003, 0xDC160004);
		Capture.SourceEntityId = PerfectAttackSourceId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.P11_1Attack");
		Capture.Content.Version = TEXT("0.0.10.P11.1");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.1-ATTACK");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1111);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseLayer& Layer)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAttackAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = PerfectSourceId;
		Request.Candidate.DetectorId = TEXT("Detector.Test.P11_1Attack");
		Request.Candidate.DetectorKind =
			EShanmenHitDetectorKind::WeaponTrajectory;
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.P11_1Attack");
		Request.Damage.RawDamage = 100.0f;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 11;
		Request.Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Request.Defense.Layers.Add(Layer);
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponPerfectGuardPolicyContractTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard.PolicyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponPerfectGuardPolicyContractTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	FShanmenWeaponPerfectGuardPolicy Policy;
	TestTrue(TEXT("Caller-owned half-open timing policy captures immutably"),
		FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open,
			PerfectTimelineId,
			10,
			15,
			TEXT("Defense.Sword.PerfectGuard01"),
			Policy)
			&& Policy.IsValid()
			&& Policy.GetWindow().GetReceiptId() == Open.GetReceiptId()
			&& Policy.GetTimelineId() == PerfectTimelineId
			&& Policy.GetActiveStartTick() == 10
			&& Policy.GetPerfectEndTick() == 15);

	TestFalse(TEXT("A timeline identity is mandatory"),
		FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open, FGuid(), 10, 15, TEXT("Defense.Perfect"), Policy));
	TestFalse(TEXT("Perfect interval must contain at least one tick"),
		FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open,
			PerfectTimelineId,
			10,
			10,
			TEXT("Defense.Perfect"),
			Policy));
	TestFalse(TEXT("A named perfect-guard rule is mandatory"),
		FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open, PerfectTimelineId, 10, 15, NAME_None, Policy));
	TestFalse(TEXT("Policy cannot bind an invalid guard window"),
		FShanmenWeaponPerfectGuardPolicy::TryCapture(
			FShanmenWeaponGuardWindowReceipt(),
			PerfectTimelineId,
			10,
			15,
			TEXT("Defense.Perfect"),
			Policy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponPerfectGuardProjectionTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard.PerfectBandProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponPerfectGuardProjectionTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponPerfectGuardPolicy Policy = MakePolicy(Open);
	FShanmenWeaponGuardTimingProjectionReceipt AtStart;
	FShanmenWeaponGuardTimingProjectionReceipt AtLastPerfectTick;
	TestTrue(TEXT("Start and end-minus-one are inside [start,end)"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window, Runtime, Policy, MakeObservation(10), AtStart)
			&& FShanmenWeaponGuardTimingEvaluator::TryProject(
				Window,
				Runtime,
				Policy,
				MakeObservation(14),
				AtLastPerfectTick)
			&& AtStart.IsValid()
			&& AtLastPerfectTick.IsValid()
			&& AtStart.GetBand()
				== EShanmenWeaponGuardTimingBand::Perfect
			&& AtLastPerfectTick.GetBand()
				== EShanmenWeaponGuardTimingBand::Perfect);

	const FShanmenDefenseLayer& Layer = AtLastPerfectTick.GetLayer();
	TestTrue(TEXT("Perfect timing replaces rather than stacks ordinary guard"),
		Layer.Operation == EShanmenDefenseOperation::PreventAll
			&& Layer.Order == FShanmenDefenseOrder::PerfectGuard
			&& FMath::IsNearlyZero(Layer.Magnitude)
			&& Layer.SourceInstanceId == PerfectWeaponId
			&& !Layer.bRequiresCommitOnTrigger
			&& Layer.LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard())
			&& !Layer.LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard()));

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Layer));
	TestTrue(TEXT("Perfect guard fully prevents and conserves 100 damage"),
		Result.bAccepted
			&& Result.Outcome == EShanmenDefenseOutcome::PerfectGuarded
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 100.0f)
			&& FMath::IsNearlyZero(Result.FinalDamage)
			&& Result.TriggeredLayers.Num() == 1
			&& Result.TriggeredLayers[0].Order
				== FShanmenDefenseOrder::PerfectGuard);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponPerfectGuardOrdinaryBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard.OrdinaryBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponPerfectGuardOrdinaryBoundaryTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	FShanmenWeaponGuardProjectionReceipt OrdinaryProjection;
	check(Window.TryProjectDefenseLayer(Runtime, OrdinaryProjection));
	FShanmenWeaponGuardTimingProjectionReceipt Projection;
	TestTrue(TEXT("Exact end tick falls back to the existing ordinary guard"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window,
			Runtime,
			MakePolicy(Open),
			MakeObservation(15),
			Projection)
			&& Projection.IsValid()
			&& Projection.GetBand()
				== EShanmenWeaponGuardTimingBand::Ordinary
			&& Projection.GetLayer().LayerId
				== OrdinaryProjection.GetLayer().LayerId
			&& Projection.GetLayer().Operation
				== EShanmenDefenseOperation::ReduceFraction
			&& Projection.GetLayer().Order == FShanmenDefenseOrder::Guard
			&& Projection.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard())
			&& !Projection.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard()));

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Projection.GetLayer()));
	TestTrue(TEXT("Ordinary fallback preserves P11.0 authored fraction"),
		Result.bAccepted
			&& Result.Outcome == EShanmenDefenseOutcome::Mitigated
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 25.0f)
			&& FMath::IsNearlyEqual(Result.FinalDamage, 75.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponPerfectGuardTimelineBindingTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard.TimelineBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponPerfectGuardTimelineBindingTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponPerfectGuardPolicy Policy = MakePolicy(Open);
	FShanmenWeaponGuardTimingProjectionReceipt Projection;
	TestFalse(TEXT("Observation before active-start tick fails closed"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window, Runtime, Policy, MakeObservation(9), Projection));
	const FGuid ForeignTimeline(
		0xDC170001, 0xDC170002, 0xDC170003, 0xDC170004);
	TestFalse(TEXT("Observation from a foreign timeline fails closed"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window,
			Runtime,
			Policy,
			MakeObservation(10, ForeignTimeline),
			Projection));

	FShanmenActionOrchestrator ForeignRuntime;
	FShanmenWeaponGuardWindow ForeignWindow;
	FShanmenWeaponGuardWindowReceipt ForeignOpen;
	OpenWindow(
		MakeGuardAction(TEXT("TEST-DIGEST-P11.1-FOREIGN"), 1112),
		ForeignRuntime,
		ForeignWindow,
		ForeignOpen);
	TestFalse(TEXT("Policy from another window cannot be replayed here"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window,
			Runtime,
			MakePolicy(ForeignOpen),
			MakeObservation(10),
			Projection));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponPerfectGuardActionPhaseBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard.ActionPhaseBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponPerfectGuardActionPhaseBoundaryTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponPerfectGuardPolicy Policy = MakePolicy(Open);
	FShanmenActionTransitionReceipt Recovery;
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	FShanmenWeaponGuardTimingProjectionReceipt Projection;
	TestFalse(TEXT("Recovery rejects timing projection"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window, Runtime, Policy, MakeObservation(10), Projection));

	FShanmenActionOrchestrator InterruptedRuntime;
	FShanmenWeaponGuardWindow InterruptedWindow;
	FShanmenWeaponGuardWindowReceipt InterruptedOpen;
	OpenWindow(
		MakeGuardAction(TEXT("TEST-DIGEST-P11.1-INTERRUPTED"), 1113),
		InterruptedRuntime,
		InterruptedWindow,
		InterruptedOpen);
	const FShanmenWeaponPerfectGuardPolicy InterruptedPolicy =
		MakePolicy(InterruptedOpen);
	FShanmenActionTransitionReceipt Interruption;
	check(InterruptedRuntime.TryInterrupt(
		EShanmenCombatActionPhase::Active, Interruption));
	TestFalse(TEXT("Interruption rejects timing projection"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			InterruptedWindow,
			InterruptedRuntime,
			InterruptedPolicy,
			MakeObservation(10),
			Projection));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponPerfectGuardDeterministicReplayTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponPerfectGuardDeterministicReplayTest::RunTest(
	const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeGuardAction();
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenWeaponGuardWindow FirstWindow;
	FShanmenWeaponGuardWindowReceipt FirstOpen;
	OpenWindow(Action, FirstRuntime, FirstWindow, FirstOpen);
	const FShanmenWeaponPerfectGuardPolicy FirstPolicy =
		MakePolicy(FirstOpen);
	const FShanmenWeaponGuardTimelineObservation Observation =
		MakeObservation(12);
	FShanmenWeaponGuardTimingProjectionReceipt FirstProjection;
	check(FShanmenWeaponGuardTimingEvaluator::TryProject(
		FirstWindow,
		FirstRuntime,
		FirstPolicy,
		Observation,
		FirstProjection));

	FShanmenActionOrchestrator ReplayRuntime;
	FShanmenWeaponGuardWindow ReplayWindow;
	FShanmenWeaponGuardWindowReceipt ReplayOpen;
	OpenWindow(Action, ReplayRuntime, ReplayWindow, ReplayOpen);
	const FShanmenWeaponPerfectGuardPolicy ReplayPolicy =
		MakePolicy(ReplayOpen);
	FShanmenWeaponGuardTimingProjectionReceipt ReplayProjection;
	TestTrue(TEXT("Equivalent frozen inputs replay every timing identity"),
		FShanmenWeaponGuardTimingEvaluator::TryProject(
			ReplayWindow,
			ReplayRuntime,
			ReplayPolicy,
			Observation,
			ReplayProjection)
			&& ReplayPolicy.GetPolicyId() == FirstPolicy.GetPolicyId()
			&& ReplayProjection.GetReceiptId()
				== FirstProjection.GetReceiptId()
			&& ReplayProjection.GetLayer().LayerId
				== FirstProjection.GetLayer().LayerId);

	const FShanmenWeaponPerfectGuardPolicy LongerPolicy =
		MakePolicy(FirstOpen, 16);
	const FShanmenWeaponPerfectGuardPolicy OtherRulePolicy = MakePolicy(
		FirstOpen, 15, TEXT("Defense.Sword.PerfectGuard02"));
	FShanmenWeaponGuardTimingProjectionReceipt LongerProjection;
	FShanmenWeaponGuardTimingProjectionReceipt OtherRuleProjection;
	check(FShanmenWeaponGuardTimingEvaluator::TryProject(
		FirstWindow,
		FirstRuntime,
		LongerPolicy,
		Observation,
		LongerProjection));
	check(FShanmenWeaponGuardTimingEvaluator::TryProject(
		FirstWindow,
		FirstRuntime,
		OtherRulePolicy,
		Observation,
		OtherRuleProjection));
	TestTrue(TEXT("Interval or rule changes produce distinct identities"),
		LongerPolicy.GetPolicyId() != FirstPolicy.GetPolicyId()
			&& LongerProjection.GetLayer().LayerId
				!= FirstProjection.GetLayer().LayerId
			&& OtherRulePolicy.GetPolicyId() != FirstPolicy.GetPolicyId()
			&& OtherRuleProjection.GetLayer().LayerId
				!= FirstProjection.GetLayer().LayerId);
	return true;
}

#endif
