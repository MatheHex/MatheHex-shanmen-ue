#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWeaponGuardArc.h"

namespace
{
	const FGuid ArcRunId(
		0xDD100001, 0xDD100002, 0xDD100003, 0xDD100004);
	const FGuid ArcDefenderOwnerId(
		0xDD110001, 0xDD110002, 0xDD110003, 0xDD110004);
	const FGuid ArcDefenderId(
		0xDD120001, 0xDD120002, 0xDD120003, 0xDD120004);
	const FGuid ArcWeaponId(
		0xDD130001, 0xDD130002, 0xDD130003, 0xDD130004);
	const FGuid ArcTimelineId(
		0xDD140001, 0xDD140002, 0xDD140003, 0xDD140004);
	const FGuid ArcAttackerId(
		0xDD150001, 0xDD150002, 0xDD150003, 0xDD150004);

	FShanmenCombatActionSnapshot MakeGuardAction(
		const FString& Digest = TEXT("TEST-DIGEST-P11.2-GUARD"),
		uint64 ActivationSequence = 1120)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ArcRunId;
		Capture.OwnerId = ArcDefenderOwnerId;
		Capture.SourceEntityId = ArcDefenderId;
		Capture.SourceItemInstanceId = ArcWeaponId;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.2");
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

	FShanmenWeaponPerfectGuardPolicy MakeTimingPolicy(
		const FShanmenWeaponGuardWindowReceipt& Open)
	{
		FShanmenWeaponPerfectGuardPolicy Policy;
		check(FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open,
			ArcTimelineId,
			10,
			15,
			TEXT("Defense.Sword.PerfectGuard01"),
			Policy));
		return Policy;
	}

	FShanmenWeaponGuardTimingProjectionReceipt MakeTimingProjection(
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenActionOrchestrator& Runtime,
		const FShanmenWeaponGuardWindowReceipt& Open,
		int64 Tick)
	{
		FShanmenWeaponGuardTimelineObservation Observation;
		check(FShanmenWeaponGuardTimelineObservation::TryCapture(
			ArcTimelineId, Tick, Observation));
		FShanmenWeaponGuardTimingProjectionReceipt Projection;
		check(FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window,
			Runtime,
			MakeTimingPolicy(Open),
			Observation,
			Projection));
		return Projection;
	}

	FShanmenWeaponGuardArcPolicy MakeArcPolicy(
		const FShanmenWeaponGuardWindowReceipt& Open,
		double MinimumFacingDot = 0.5,
		FName ArcRuleId = TEXT("Defense.Sword.WeaponGuardArc01"))
	{
		FShanmenWeaponGuardArcPolicy Policy;
		check(FShanmenWeaponGuardArcPolicy::TryCapture(
			Open, ArcRuleId, MinimumFacingDot, Policy));
		return Policy;
	}

	FShanmenHitCandidate MakeCandidate(
		const FGuid& TargetEntityId = ArcDefenderId,
		int32 HitOrdinal = 0)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			ArcRunId,
			ArcAttackerId,
			TEXT("Combat.Action.Test.P11_2Attack"),
			1121);
		Candidate.SourceEntityId = ArcAttackerId;
		Candidate.TargetEntityId = TargetEntityId;
		Candidate.DetectorId = TEXT("Detector.Test.P11_2Attack");
		Candidate.DetectorKind =
			EShanmenHitDetectorKind::WeaponTrajectory;
		Candidate.HitLocation = FVector(100.0, 20.0, 5.0);
		Candidate.HitNormal = FVector::BackwardVector;
		Candidate.HitOrdinal = HitOrdinal;
		check(Candidate.IsValid());
		return Candidate;
	}

	FShanmenWeaponGuardThreatSample MakeSample(
		const FVector& GuardFacing = FVector::ForwardVector,
		const FVector& DirectionToThreat = FVector::ForwardVector,
		const FShanmenHitCandidate& Candidate = MakeCandidate())
	{
		FShanmenWeaponGuardThreatSample Sample;
		check(FShanmenWeaponGuardThreatSample::TryCapture(
			Candidate, GuardFacing, DirectionToThreat, Sample));
		return Sample;
	}

	FShanmenCombatActionSnapshot MakeAttackAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ArcRunId;
		Capture.OwnerId = FGuid(
			0xDD160001, 0xDD160002, 0xDD160003, 0xDD160004);
		Capture.SourceEntityId = ArcAttackerId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.P11_2Attack");
		Capture.Content.Version = TEXT("0.0.10.P11.2");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.2-ATTACK");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1121);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseLayer& Layer)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAttackAction();
		Request.Candidate = MakeCandidate();
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.P11_2Attack");
		Request.Damage.RawDamage = 100.0f;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 12;
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
	FShanmenWeaponGuardArcPolicyAndSampleTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuardArc.PolicyAndSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardArcPolicyAndSampleTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	FShanmenWeaponGuardArcPolicy Policy;
	TestTrue(TEXT("Content-owned inclusive dot policy captures immutably"),
		FShanmenWeaponGuardArcPolicy::TryCapture(
			Open,
			TEXT("Defense.Sword.WeaponGuardArc01"),
			0.5,
			Policy)
			&& Policy.IsValid()
			&& Policy.GetWindow().GetReceiptId() == Open.GetReceiptId()
			&& FMath::IsNearlyEqual(Policy.GetMinimumFacingDot(), 0.5));
	TestFalse(TEXT("Arc rule identity is mandatory"),
		FShanmenWeaponGuardArcPolicy::TryCapture(
			Open, NAME_None, 0.5, Policy));
	TestFalse(TEXT("Threshold above one fails closed"),
		FShanmenWeaponGuardArcPolicy::TryCapture(
			Open, TEXT("Defense.Arc"), 1.01, Policy));
	TestFalse(TEXT("Threshold below negative one fails closed"),
		FShanmenWeaponGuardArcPolicy::TryCapture(
			Open, TEXT("Defense.Arc"), -1.01, Policy));

	FShanmenWeaponGuardThreatSample Sample;
	TestTrue(TEXT("Raw 3D directions normalize into immutable unit vectors"),
		FShanmenWeaponGuardThreatSample::TryCapture(
			MakeCandidate(),
			FVector(2.0, 0.0, 0.0),
			FVector(4.0, 0.0, 0.0),
			Sample)
			&& Sample.IsValid()
			&& Sample.GetGuardFacing() == FVector::ForwardVector
			&& Sample.GetDirectionToThreat() == FVector::ForwardVector);
	TestFalse(TEXT("Zero guard facing has no directional meaning"),
		FShanmenWeaponGuardThreatSample::TryCapture(
			MakeCandidate(),
			FVector::ZeroVector,
			FVector::ForwardVector,
			Sample));
	TestFalse(TEXT("Zero threat direction has no directional meaning"),
		FShanmenWeaponGuardThreatSample::TryCapture(
			MakeCandidate(),
			FVector::ForwardVector,
			FVector::ZeroVector,
			Sample));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardArcPerfectProjectionTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuardArc.QualifiedPerfect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardArcPerfectProjectionTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponGuardTimingProjectionReceipt Timing =
		MakeTimingProjection(Window, Runtime, Open, 10);
	FShanmenWeaponGuardArcEvaluation Evaluation;
	TestTrue(TEXT("Front threat qualifies the already-selected perfect layer"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			Runtime,
			MakeArcPolicy(Open),
			Timing,
			MakeSample(),
			Evaluation)
			&& Evaluation.IsValid()
			&& Evaluation.IsQualified()
			&& Evaluation.HasLayer()
			&& FMath::IsNearlyEqual(Evaluation.GetAlignmentDot(), 1.0)
			&& Evaluation.GetLayer().LayerId == Timing.GetLayer().LayerId
			&& Evaluation.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard())
			&& !Evaluation.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard()));

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Evaluation.GetLayer()));
	TestTrue(TEXT("Qualified perfect layer still conserves 100 damage"),
		Result.bAccepted
			&& Result.Outcome == EShanmenDefenseOutcome::PerfectGuarded
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 100.0f)
			&& FMath::IsNearlyZero(Result.FinalDamage));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardArcOrdinaryProjectionTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuardArc.QualifiedOrdinary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardArcOrdinaryProjectionTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponGuardTimingProjectionReceipt Timing =
		MakeTimingProjection(Window, Runtime, Open, 15);
	FShanmenWeaponGuardArcEvaluation Evaluation;
	TestTrue(TEXT("Direction gate preserves the ordinary timing selection"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			Runtime,
			MakeArcPolicy(Open),
			Timing,
			MakeSample(),
			Evaluation)
			&& Evaluation.IsQualified()
			&& Evaluation.GetLayer().LayerId == Timing.GetLayer().LayerId
			&& Evaluation.GetLayer().Operation
				== EShanmenDefenseOperation::ReduceFraction
			&& Evaluation.GetLayer().Order == FShanmenDefenseOrder::Guard
			&& Evaluation.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard())
			&& !Evaluation.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard()));

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Evaluation.GetLayer()));
	TestTrue(TEXT("Qualified ordinary layer preserves P11.0 25/75 result"),
		Result.bAccepted
			&& Result.Outcome == EShanmenDefenseOutcome::Mitigated
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 25.0f)
			&& FMath::IsNearlyEqual(Result.FinalDamage, 75.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardArcOutsideArcTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuardArc.OutsideArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardArcOutsideArcTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	FShanmenWeaponGuardArcEvaluation Evaluation;
	TestTrue(TEXT("Rear threat yields an auditable result with no layer"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			Runtime,
			MakeArcPolicy(Open, 0.0),
			MakeTimingProjection(Window, Runtime, Open, 10),
			MakeSample(FVector::ForwardVector, FVector::BackwardVector),
			Evaluation)
			&& Evaluation.IsValid()
			&& !Evaluation.IsQualified()
			&& !Evaluation.HasLayer()
			&& Evaluation.GetStatus()
				== EShanmenWeaponGuardArcStatus::OutsideArc
			&& FMath::IsNearlyEqual(Evaluation.GetAlignmentDot(), -1.0)
			&& !Evaluation.GetLayer().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardArcBindingAndLifecycleTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuardArc.BindingAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardArcBindingAndLifecycleTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponGuardTimingProjectionReceipt Timing =
		MakeTimingProjection(Window, Runtime, Open, 10);
	const FShanmenWeaponGuardArcPolicy Policy = MakeArcPolicy(Open);
	FShanmenWeaponGuardArcEvaluation Evaluation;
	const FGuid ForeignTarget(
		0xDD170001, 0xDD170002, 0xDD170003, 0xDD170004);
	TestFalse(TEXT("Contact must target the defending guard action owner"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			Runtime,
			Policy,
			Timing,
			MakeSample(
				FVector::ForwardVector,
				FVector::ForwardVector,
				MakeCandidate(ForeignTarget)),
			Evaluation));

	FShanmenActionOrchestrator ForeignRuntime;
	FShanmenWeaponGuardWindow ForeignWindow;
	FShanmenWeaponGuardWindowReceipt ForeignOpen;
	OpenWindow(
		MakeGuardAction(TEXT("TEST-DIGEST-P11.2-FOREIGN"), 1122),
		ForeignRuntime,
		ForeignWindow,
		ForeignOpen);
	TestFalse(TEXT("Arc policy from another guard window fails closed"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			Runtime,
			MakeArcPolicy(ForeignOpen),
			Timing,
			MakeSample(),
			Evaluation));

	FShanmenActionTransitionReceipt Recovery;
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	TestFalse(TEXT("Recovery closes directional qualification"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window, Runtime, Policy, Timing, MakeSample(), Evaluation));

	FShanmenActionOrchestrator InterruptedRuntime;
	FShanmenWeaponGuardWindow InterruptedWindow;
	FShanmenWeaponGuardWindowReceipt InterruptedOpen;
	OpenWindow(
		MakeGuardAction(TEXT("TEST-DIGEST-P11.2-INTERRUPTED"), 1123),
		InterruptedRuntime,
		InterruptedWindow,
		InterruptedOpen);
	const FShanmenWeaponGuardTimingProjectionReceipt InterruptedTiming =
		MakeTimingProjection(
			InterruptedWindow,
			InterruptedRuntime,
			InterruptedOpen,
			10);
	const FShanmenWeaponGuardArcPolicy InterruptedPolicy =
		MakeArcPolicy(InterruptedOpen);
	FShanmenActionTransitionReceipt Interruption;
	check(InterruptedRuntime.TryInterrupt(
		EShanmenCombatActionPhase::Active, Interruption));
	TestFalse(TEXT("Interruption closes directional qualification"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			InterruptedWindow,
			InterruptedRuntime,
			InterruptedPolicy,
			InterruptedTiming,
			MakeSample(),
			Evaluation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardArcBoundaryAndReplayTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuardArc.BoundaryAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardArcBoundaryAndReplayTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(MakeGuardAction(), Runtime, Window, Open);
	const FShanmenWeaponGuardTimingProjectionReceipt Timing =
		MakeTimingProjection(Window, Runtime, Open, 10);
	const FShanmenWeaponGuardArcPolicy BoundaryPolicy =
		MakeArcPolicy(Open, 0.0);
	const FShanmenWeaponGuardThreatSample BoundarySample = MakeSample(
		FVector(2.0, 0.0, 0.0), FVector(0.0, 4.0, 0.0));
	FShanmenWeaponGuardArcEvaluation First;
	FShanmenWeaponGuardArcEvaluation Replay;
	TestTrue(TEXT("Inclusive dot boundary qualifies and replays exactly"),
		FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			Runtime,
			BoundaryPolicy,
			Timing,
			BoundarySample,
			First)
			&& FShanmenWeaponGuardArcEvaluator::TryEvaluate(
				Window,
				Runtime,
				BoundaryPolicy,
				Timing,
				MakeSample(
					FVector::ForwardVector, FVector::RightVector),
				Replay)
			&& First.IsQualified()
			&& FMath::IsNearlyZero(First.GetAlignmentDot())
			&& Replay.GetSample().GetSampleId()
				== BoundarySample.GetSampleId()
			&& Replay.GetEvaluationId() == First.GetEvaluationId());

	const FShanmenWeaponGuardArcPolicy NarrowerPolicy =
		MakeArcPolicy(Open, 0.25);
	FShanmenWeaponGuardArcEvaluation Narrower;
	check(FShanmenWeaponGuardArcEvaluator::TryEvaluate(
		Window,
		Runtime,
		NarrowerPolicy,
		Timing,
		BoundarySample,
		Narrower));
	TestTrue(TEXT("Threshold change produces distinct outside-arc evidence"),
		NarrowerPolicy.GetPolicyId() != BoundaryPolicy.GetPolicyId()
			&& Narrower.GetStatus()
				== EShanmenWeaponGuardArcStatus::OutsideArc
			&& Narrower.GetEvaluationId() != First.GetEvaluationId());
	return true;
}

#endif
