#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenControlledWeaponExecution.h"
#include "ShanmenControlledWeaponThreatPresenceAuthority.h"

namespace
{
	const FGuid ControlledRunId(0x56000001, 0, 0, 1);
	const FGuid ControlledOwnerId(0x56000002, 0, 0, 1);
	const FGuid ControlledSourceEntityId(0x56000003, 0, 0, 1);
	const FGuid ControlledItemId(0x56000004, 0, 0, 1);
	const FGuid ControlledTargetA(0x56000005, 0, 0, 1);
	const FGuid ControlledTargetB(0x56000006, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeControlledAction(bool bWithItem = true)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ControlledRunId;
		Capture.OwnerId = ControlledOwnerId;
		Capture.SourceEntityId = ControlledSourceEntityId;
		Capture.SourceItemInstanceId = bWithItem
			? ControlledItemId
			: FGuid();
		Capture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P6.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P6.0");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			6);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenControlledWeaponDefinition MakeControlledDefinition()
	{
		FShanmenControlledWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.ControlledWeapon.FlyingSword01");
		Capture.FormulaId = TEXT("Combat.Formula.ControlledWeapon.FlyingSword01.r1");
		Capture.BaseDamage = 10.0f;
		Capture.ControlPowerCoefficient = 0.25f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;

		FShanmenControlledWeaponDefinition Definition;
		check(FShanmenControlledWeaponDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenControlledWeaponOffenseSnapshot MakeControlledOffense()
	{
		FShanmenControlledWeaponOffenseSnapshot Offense;
		check(FShanmenControlledWeaponOffenseSnapshot::TryCapture(
			40.0f, Offense));
		return Offense;
	}

	FShanmenTargetVitalitySnapshot MakeControlledVitality()
	{
		FShanmenTargetVitalitySnapshot Vitality;
		Vitality.CurrentVitality = 100.0f;
		Vitality.MaximumVitality = 100.0f;
		return Vitality;
	}

	FShanmenDefenseSnapshot MakeControlledDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}

	FShanmenHitCandidate MakeControlledCandidate(
		const FShanmenWorldHitContext& Context,
		const FGuid& TargetId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = Context.GetAction().GetActivationId();
		Candidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
		Candidate.TargetEntityId = TargetId;
		Candidate.DetectorId = Context.GetDetectorId();
		Candidate.DetectorKind = Context.GetDetectorKind();
		Candidate.HitOrdinal = Context.GetHitOrdinal();
		Candidate.HitLocation = FVector(300.0, 20.0, 80.0);
		Candidate.HitNormal = FVector::BackwardVector;
		return Candidate;
	}

	FShanmenControlledWeaponThreatTargetEvidence MakeThreatEvidence(
		const FGuid& TargetId,
		bool bLiving)
	{
		FGameplayTagContainer TargetTags;
		if (bLiving)
		{
			TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		}
		FShanmenControlledWeaponThreatTargetEvidence Evidence;
		check(FShanmenControlledWeaponThreatTargetEvidence::TryCapture(
			TargetId, TargetTags, Evidence));
		return Evidence;
	}

	void StartActiveControlledWeapon(
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenControlledWeaponExecution& OutExecution,
		FShanmenActionTransitionReceipt& OutPhaseReceipt)
	{
		const FShanmenCombatActionSnapshot Action = MakeControlledAction();
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutActionRuntime, OutPhaseReceipt));
		check(FShanmenControlledWeaponExecution::TryCreate(
			Action,
			MakeControlledDefinition(),
			MakeControlledOffense(),
			OutExecution));
		check(OutActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, OutPhaseReceipt));
	}

	FShanmenControlledWeaponThreatPresenceReceipt BuildLivingThreatPresence(
		FShanmenActionOrchestrator& ActionRuntime,
		FShanmenControlledWeaponExecution& Execution,
		const TArray<FGuid>& TargetIds)
	{
		FShanmenWorldHitContext Context;
		check(Execution.TryBeginOrbitThreatEmission(
			ActionRuntime, Context));
		for (const FGuid& TargetId : TargetIds)
		{
			check(Execution.TryAcceptOrbitThreatCandidate(
				ActionRuntime,
				MakeControlledCandidate(Context, TargetId)));
		}

		FShanmenDetectorEmissionReceipt Emission;
		check(Execution.TryEndOrbitThreatEmission(
			ActionRuntime, Emission));
		TArray<FShanmenControlledWeaponThreatTargetEvidence> Evidence;
		Evidence.Reserve(TargetIds.Num());
		for (const FGuid& TargetId : TargetIds)
		{
			Evidence.Add(MakeThreatEvidence(TargetId, true));
		}

		FShanmenControlledWeaponThreatPolicyReceipt Policy;
		check(Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, Evidence, Policy));
		FShanmenControlledWeaponThreatPresenceReceipt Presence;
		check(Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, Policy, Presence));
		return Presence;
	}

	bool CommandReceiptsMatch(
		const FShanmenControlledWeaponCommandReceipt& A,
		const FShanmenControlledWeaponCommandReceipt& B)
	{
		return A.GetCommandId() == B.GetCommandId()
			&& A.GetActivationId() == B.GetActivationId()
			&& A.GetSourceItemInstanceId() == B.GetSourceItemInstanceId()
			&& A.GetSequence() == B.GetSequence()
			&& A.GetKind() == B.GetKind()
			&& A.GetStateBefore() == B.GetStateBefore()
			&& A.GetStateAfter() == B.GetStateAfter()
			&& A.GetDirectionAfter().Equals(
				B.GetDirectionAfter(), UE_DOUBLE_SMALL_NUMBER);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponPhysicalItemBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.PhysicalItemBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponPhysicalItemBoundaryTest::RunTest(const FString&)
{
	FShanmenControlledWeaponExecution Rejected;
	TestFalse(TEXT("A controlled weapon cannot exist without a physical item identity"),
		FShanmenControlledWeaponExecution::TryCreate(
			MakeControlledAction(false),
			MakeControlledDefinition(),
			MakeControlledOffense(),
			Rejected));
	TestFalse(TEXT("Default control offense is not a frozen snapshot"),
		FShanmenControlledWeaponOffenseSnapshot().IsValid());

	FShanmenControlledWeaponDefinitionCapture WrongCapture;
	WrongCapture.ActionDefinitionId = TEXT("Combat.Action.Projectile.Generic");
	WrongCapture.DetectorId = TEXT("Detector.ControlledWeapon.FlyingSword01");
	WrongCapture.FormulaId = TEXT("Combat.Formula.ControlledWeapon.FlyingSword01.r1");
	WrongCapture.BaseDamage = 10.0f;
	WrongCapture.ControlPowerCoefficient = 0.25f;
	WrongCapture.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	WrongCapture.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	FShanmenControlledWeaponDefinition WrongDefinition;
	TestFalse(TEXT("A generic projectile identity cannot enter the controlled-weapon contract"),
		FShanmenControlledWeaponDefinition::TryCapture(
			WrongCapture, WrongDefinition));

	FShanmenControlledWeaponExecution Accepted;
	TestTrue(TEXT("An exact physical source prepares an orbiting flying sword"),
		FShanmenControlledWeaponExecution::TryCreate(
			MakeControlledAction(),
			MakeControlledDefinition(),
			MakeControlledOffense(),
			Accepted));
	TestTrue(TEXT("Prepared execution retains the exact item identity"),
		Accepted.IsValid()
			&& Accepted.GetAction().GetSourceItemInstanceId()
				== ControlledItemId
			&& Accepted.GetState()
				== EShanmenControlledWeaponState::Orbiting);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponCommandSequenceTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.CommandSequenceAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponCommandSequenceTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeControlledAction();
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	TestTrue(TEXT("Controlled action enters Startup"),
		FShanmenActionOrchestrator::TryStart(
			Action, ActionRuntime, PhaseReceipt));
	FShanmenControlledWeaponExecution Execution;
	TestTrue(TEXT("Controlled execution captures physical item and content"),
		FShanmenControlledWeaponExecution::TryCreate(
			Action,
			MakeControlledDefinition(),
			MakeControlledOffense(),
			Execution));
	FShanmenControlledWeaponCommandReceipt Receipt;
	TestFalse(TEXT("Startup cannot launch the physical sword"),
		Execution.TryIssueCommand(
			ActionRuntime, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector, Receipt));
	TestTrue(TEXT("Action crosses its existing commit point"),
		ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, PhaseReceipt));

	TestTrue(TEXT("Sequence zero launches into directed flight"),
		Execution.TryIssueCommand(
			ActionRuntime, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector(10.0, 0.0, 0.0), Receipt));
	const FShanmenControlledWeaponCommandReceipt FirstLaunch = Receipt;
	TestTrue(TEXT("Launch receipt is canonical and item-bound"),
		Receipt.IsValid()
			&& Receipt.GetSourceItemInstanceId() == ControlledItemId
			&& Receipt.GetStateAfter()
				== EShanmenControlledWeaponState::Directed
			&& Receipt.GetDirectionAfter().Equals(FVector::ForwardVector));
	TestEqual(TEXT("One accepted command advances sequence once"),
		Execution.GetNextCommandSequence(), int64(1));
	TestTrue(TEXT("Exact launch replay returns the original receipt"),
		Execution.TryIssueCommand(
			ActionRuntime, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector(1.0, 0.0, 0.0), Receipt)
			&& CommandReceiptsMatch(FirstLaunch, Receipt));
	TestEqual(TEXT("Exact replay does not consume another sequence"),
		Execution.GetNextCommandSequence(), int64(1));
	TestFalse(TEXT("Conflicting sequence-zero direction fails closed"),
		Execution.TryIssueCommand(
			ActionRuntime, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::RightVector, Receipt));
	TestFalse(TEXT("A skipped command sequence fails closed"),
		Execution.TryIssueCommand(
			ActionRuntime, 2,
			EShanmenControlledWeaponCommandKind::Redirect,
			FVector::RightVector, Receipt));
	TestTrue(TEXT("Sequence one redirects without spawning a new weapon"),
		Execution.TryIssueCommand(
			ActionRuntime, 1,
			EShanmenControlledWeaponCommandKind::Redirect,
			FVector::RightVector, Receipt));
	TestTrue(TEXT("Redirect preserves the same physical item and directed state"),
		Receipt.IsValid()
			&& Receipt.GetSourceItemInstanceId() == ControlledItemId
			&& Execution.GetState()
				== EShanmenControlledWeaponState::Directed
			&& Execution.GetCurrentDirection().Equals(FVector::RightVector));

	FShanmenWorldHitContext Context;
	TestTrue(TEXT("Directed flight may open a controlled-object emission"),
		Execution.TryBeginEmission(ActionRuntime, Context));
	TestFalse(TEXT("Recall cannot race an open contact emission"),
		Execution.TryIssueCommand(
			ActionRuntime, 2,
			EShanmenControlledWeaponCommandKind::Recall,
			FVector::ZeroVector, Receipt));
	TestTrue(TEXT("Contact emission closes before recall"),
		Execution.TryEndEmission(ActionRuntime));
	TestTrue(TEXT("Sequence two recalls the same item"),
		Execution.TryIssueCommand(
			ActionRuntime, 2,
			EShanmenControlledWeaponCommandKind::Recall,
			FVector::ZeroVector, Receipt));
	const FShanmenControlledWeaponCommandReceipt Recall = Receipt;
	TestTrue(TEXT("Recall is terminal for this activation"),
		Receipt.IsValid()
			&& Execution.GetState()
				== EShanmenControlledWeaponState::Recalled
			&& Execution.GetCurrentDirection().IsNearlyZero());
	TestTrue(TEXT("Exact recall replay is idempotent"),
		Execution.TryIssueCommand(
			ActionRuntime, 2,
			EShanmenControlledWeaponCommandKind::Recall,
			FVector::ZeroVector, Receipt)
			&& CommandReceiptsMatch(Recall, Receipt));
	TestFalse(TEXT("A recalled item cannot relaunch in the same activation"),
		Execution.TryIssueCommand(
			ActionRuntime, 3,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector, Receipt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponOrbitThreatTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.OrbitThreatCandidateOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponOrbitThreatTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenControlledWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveControlledWeapon(ActionRuntime, Execution, PhaseReceipt);

	FShanmenWorldHitContext OrbitContext;
	TestTrue(TEXT("Orbiting execution opens a candidate-only emission"),
		Execution.TryBeginOrbitThreatEmission(
			ActionRuntime, OrbitContext)
		&& OrbitContext.GetHitOrdinal() == 0
		&& Execution.GetState()
			== EShanmenControlledWeaponState::Orbiting);
	const FShanmenHitCandidate Threat =
		MakeControlledCandidate(OrbitContext, ControlledTargetA);
	const FShanmenHitCandidate ThreatB =
		MakeControlledCandidate(OrbitContext, ControlledTargetB);
	TestTrue(TEXT("Two targets are accepted in callback order as geometry"),
		Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, ThreatB)
			&& Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, Threat));
	TestFalse(TEXT("Duplicate near-threat geometry is rejected"),
		Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, Threat));

	FShanmenControlledWeaponImpactReceipt Impact;
	TestFalse(TEXT("Orbit threat candidate cannot enter the damage resolver"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			Threat,
			MakeControlledVitality(),
			MakeControlledDefense(),
			Impact));
	TestEqual(TEXT("Candidate-only observation does not write impact ledger"),
		Execution.NumAcceptedImpacts(), 0);

	FShanmenControlledWeaponCommandReceipt Command;
	TestFalse(TEXT("Launch cannot change state while Orbit sample is open"),
		Execution.TryIssueCommand(
			ActionRuntime,
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));
	FShanmenDetectorEmissionReceipt ThreatReceipt;
	TestTrue(TEXT("Closing Orbit sample returns canonical geometry evidence"),
		Execution.TryEndOrbitThreatEmission(ActionRuntime, ThreatReceipt)
		&& ThreatReceipt.IsValid()
		&& ThreatReceipt.GetCandidates().Num() == 2
		&& ThreatReceipt.GetContext().GetAction().GetSourceItemInstanceId()
			== ControlledItemId
		&& ThreatReceipt.GetCandidates()[0].TargetEntityId
			.ToString(EGuidFormats::Digits)
			< ThreatReceipt.GetCandidates()[1].TargetEntityId
				.ToString(EGuidFormats::Digits));
	TestTrue(TEXT("Canonical receipt preserves the shared ordinal stream"),
		Execution.TryIssueCommand(
			ActionRuntime,
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));

	FShanmenWorldHitContext DirectedContext;
	TestTrue(TEXT("First Directed sample follows the Orbit ordinal"),
		Execution.TryBeginEmission(ActionRuntime, DirectedContext)
		&& DirectedContext.GetHitOrdinal() == 1);
	const FShanmenHitCandidate Directed =
		MakeControlledCandidate(DirectedContext, ControlledTargetA);
	TestTrue(TEXT("Directed candidate still resolves through the damage path"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			Directed,
			MakeControlledVitality(),
			MakeControlledDefense(),
			Impact)
		&& Impact.IsValid()
		&& Execution.NumAcceptedImpacts() == 1
		&& Execution.TryEndEmission(ActionRuntime));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponOrbitThreatPolicyTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.OrbitThreatTargetPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponOrbitThreatPolicyTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenControlledWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveControlledWeapon(ActionRuntime, Execution, PhaseReceipt);

	FShanmenWorldHitContext Context;
	if (!Execution.TryBeginOrbitThreatEmission(ActionRuntime, Context))
	{
		AddError(TEXT("Could not open P6.12 Orbit policy fixture."));
		return false;
	}
	const FShanmenHitCandidate Living =
		MakeControlledCandidate(Context, ControlledTargetA);
	const FShanmenHitCandidate MissingTags =
		MakeControlledCandidate(Context, ControlledTargetB);
	const FShanmenHitCandidate Self =
		MakeControlledCandidate(Context, ControlledSourceEntityId);
	FShanmenDetectorEmissionReceipt Emission;
	TestTrue(TEXT("Geometry authority accepts candidates without policy decisions"),
		Execution.TryAcceptOrbitThreatCandidate(
			ActionRuntime, MissingTags)
		&& Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, Self)
		&& Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, Living)
		&& Execution.TryEndOrbitThreatEmission(ActionRuntime, Emission));

	TArray<FShanmenControlledWeaponThreatTargetEvidence> Evidence;
	Evidence.Add(MakeThreatEvidence(ControlledTargetB, false));
	Evidence.Add(MakeThreatEvidence(ControlledTargetA, true));
	Evidence.Add(MakeThreatEvidence(ControlledSourceEntityId, true));
	FShanmenControlledWeaponThreatPolicyReceipt Policy;
	TestTrue(TEXT("Policy joins reverse-order evidence to canonical geometry"),
		Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, Evidence, Policy)
		&& Policy.IsValid()
		&& Policy.GetEmission().GetContext().GetHitOrdinal()
			== Context.GetHitOrdinal()
		&& Policy.GetRequiredTargetTags().HasTag(
			FShanmenCombatNativeTags::TargetLiving())
		&& Policy.RejectsSelf()
		&& Policy.GetTargets().Num() == 3
		&& Policy.NumAcceptedTargets() == 1);

	const FShanmenControlledWeaponThreatTargetReceipt* SelfResult =
		Policy.GetTargets().FindByPredicate(
			[](const FShanmenControlledWeaponThreatTargetReceipt& Target)
			{
				return Target.GetCandidate().TargetEntityId
					== ControlledSourceEntityId;
			});
	const FShanmenControlledWeaponThreatTargetReceipt* LivingResult =
		Policy.GetTargets().FindByPredicate(
			[](const FShanmenControlledWeaponThreatTargetReceipt& Target)
			{
				return Target.GetCandidate().TargetEntityId
					== ControlledTargetA;
			});
	const FShanmenControlledWeaponThreatTargetReceipt* MissingResult =
		Policy.GetTargets().FindByPredicate(
			[](const FShanmenControlledWeaponThreatTargetReceipt& Target)
			{
				return Target.GetCandidate().TargetEntityId
					== ControlledTargetB;
			});
	TestTrue(TEXT("Every target has an explicit auditable decision"),
		SelfResult
		&& SelfResult->GetDecision()
			== EShanmenControlledWeaponThreatTargetDecision::RejectedSelf
		&& LivingResult
		&& LivingResult->GetDecision()
			== EShanmenControlledWeaponThreatTargetDecision::Accepted
		&& MissingResult
		&& MissingResult->GetDecision()
			== EShanmenControlledWeaponThreatTargetDecision::RejectedMissingRequiredTags);
	TestEqual(TEXT("Policy evaluation never writes the impact ledger"),
		Execution.NumAcceptedImpacts(), 0);

	FShanmenControlledWeaponThreatPolicyReceipt Replay;
	TestTrue(TEXT("Exact evidence replay is deterministic and side-effect free"),
		Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, Evidence, Replay)
		&& Replay.IsValid()
		&& Replay.NumAcceptedTargets() == Policy.NumAcceptedTargets());

	TArray<FShanmenControlledWeaponThreatTargetEvidence> MissingEvidence = Evidence;
	MissingEvidence.Pop();
	FShanmenControlledWeaponThreatPolicyReceipt Rejected;
	TestFalse(TEXT("Incomplete evidence fails closed"),
		Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, MissingEvidence, Rejected));
	TArray<FShanmenControlledWeaponThreatTargetEvidence> DuplicateEvidence = Evidence;
	DuplicateEvidence[2] = DuplicateEvidence[1];
	TestFalse(TEXT("Duplicate evidence fails closed"),
		Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, DuplicateEvidence, Rejected));

	FShanmenWorldHitContext EmptyContext;
	FShanmenDetectorEmissionReceipt EmptyEmission;
	FShanmenControlledWeaponThreatPolicyReceipt EmptyPolicy;
	TestTrue(TEXT("A completed no-target sample remains explicit evidence"),
		Execution.TryBeginOrbitThreatEmission(ActionRuntime, EmptyContext)
		&& Execution.TryEndOrbitThreatEmission(ActionRuntime, EmptyEmission)
		&& Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, EmptyEmission, {}, EmptyPolicy)
		&& EmptyPolicy.IsValid()
		&& EmptyPolicy.GetTargets().IsEmpty()
		&& EmptyPolicy.NumAcceptedTargets() == 0);

	FShanmenControlledWeaponCommandReceipt Launch;
	TestTrue(TEXT("Completed policy does not prevent the later Launch command"),
		Execution.TryIssueCommand(
			ActionRuntime,
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Launch));
	TestFalse(TEXT("An old Orbit receipt cannot be evaluated after state change"),
		Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, Evidence, Rejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponOrbitThreatPresenceTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.OrbitThreatPresenceIntents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponOrbitThreatPresenceTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenControlledWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveControlledWeapon(ActionRuntime, Execution, PhaseReceipt);

	FShanmenWorldHitContext FirstContext;
	if (!Execution.TryBeginOrbitThreatEmission(ActionRuntime, FirstContext))
	{
		AddError(TEXT("Could not open P6.14 threat-presence fixture."));
		return false;
	}
	const FShanmenHitCandidate MissingTags =
		MakeControlledCandidate(FirstContext, ControlledTargetB);
	const FShanmenHitCandidate Self =
		MakeControlledCandidate(FirstContext, ControlledSourceEntityId);
	const FShanmenHitCandidate Living =
		MakeControlledCandidate(FirstContext, ControlledTargetA);
	FShanmenDetectorEmissionReceipt FirstEmission;
	TestTrue(TEXT("Presence fixture closes one canonical geometry sample"),
		Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, MissingTags)
		&& Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, Self)
		&& Execution.TryAcceptOrbitThreatCandidate(ActionRuntime, Living)
		&& Execution.TryEndOrbitThreatEmission(
			ActionRuntime, FirstEmission));

	TArray<FShanmenControlledWeaponThreatTargetEvidence> Evidence;
	Evidence.Add(MakeThreatEvidence(ControlledTargetB, false));
	Evidence.Add(MakeThreatEvidence(ControlledTargetA, true));
	Evidence.Add(MakeThreatEvidence(ControlledSourceEntityId, true));
	FShanmenControlledWeaponThreatPolicyReceipt FirstPolicy;
	FShanmenControlledWeaponThreatPresenceReceipt FirstPresence;
	TestTrue(TEXT("Only accepted targets become threat-presence intents"),
		Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, FirstEmission, Evidence, FirstPolicy)
		&& Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, FirstPolicy, FirstPresence)
		&& FirstPresence.IsValid()
		&& FirstPresence.GetIntents().Num() == 1
		&& FirstPresence.GetIntents()[0].GetCandidate().TargetEntityId
			== ControlledTargetA
		&& FirstPresence.GetIntents()[0].GetSourceItemInstanceId()
			== FirstContext.GetAction().GetSourceItemInstanceId()
		&& FirstPresence.GetIntents()[0].GetRunId()
			== FirstContext.GetAction().GetRunId()
		&& FirstPresence.GetIntents()[0].GetIntentId().IsValid()
		&& Execution.NumAcceptedImpacts() == 0
		&& Execution.GetState()
			== EShanmenControlledWeaponState::Orbiting);

	FShanmenControlledWeaponThreatPresenceReceipt ReplayPresence;
	TestTrue(TEXT("Exact policy replay keeps the deterministic intent identity"),
		Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, FirstPolicy, ReplayPresence)
		&& ReplayPresence.IsValid()
		&& ReplayPresence.GetIntents().Num() == 1
		&& ReplayPresence.GetIntents()[0].GetIntentId()
			== FirstPresence.GetIntents()[0].GetIntentId());

	FShanmenWorldHitContext SecondContext;
	FShanmenDetectorEmissionReceipt SecondEmission;
	FShanmenControlledWeaponThreatPolicyReceipt SecondPolicy;
	FShanmenControlledWeaponThreatPresenceReceipt SecondPresence;
	TestTrue(TEXT("A later explicit sample receives a distinct intent identity"),
		Execution.TryBeginOrbitThreatEmission(ActionRuntime, SecondContext)
		&& Execution.TryAcceptOrbitThreatCandidate(
			ActionRuntime,
			MakeControlledCandidate(SecondContext, ControlledTargetA))
		&& Execution.TryEndOrbitThreatEmission(
			ActionRuntime, SecondEmission)
		&& Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime,
			SecondEmission,
			{ MakeThreatEvidence(ControlledTargetA, true) },
			SecondPolicy)
		&& Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, SecondPolicy, SecondPresence)
		&& SecondPresence.IsValid()
		&& SecondContext.GetHitOrdinal()
			== FirstContext.GetHitOrdinal() + 1
		&& SecondPresence.GetIntents()[0].GetIntentId()
			!= FirstPresence.GetIntents()[0].GetIntentId()
		&& Execution.NumAcceptedImpacts() == 0);

	FShanmenWorldHitContext EmptyContext;
	FShanmenDetectorEmissionReceipt EmptyEmission;
	FShanmenControlledWeaponThreatPolicyReceipt EmptyPolicy;
	FShanmenControlledWeaponThreatPresenceReceipt EmptyPresence;
	TestTrue(TEXT("A no-target sample emits an explicit empty receipt"),
		Execution.TryBeginOrbitThreatEmission(ActionRuntime, EmptyContext)
		&& Execution.TryEndOrbitThreatEmission(ActionRuntime, EmptyEmission)
		&& Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, EmptyEmission, {}, EmptyPolicy)
		&& Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, EmptyPolicy, EmptyPresence)
		&& EmptyPresence.IsValid()
		&& EmptyPresence.GetIntents().IsEmpty());

	FShanmenControlledWeaponCommandReceipt Launch;
	TestTrue(TEXT("Presence intent production leaves command ownership explicit"),
		Execution.TryIssueCommand(
			ActionRuntime,
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Launch));
	FShanmenControlledWeaponThreatPresenceReceipt Rejected;
	TestFalse(TEXT("An old Orbit policy cannot emit after state transition"),
		Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, FirstPolicy, Rejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponThreatPresenceAuthorityTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.ThreatPresenceAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponThreatPresenceAuthorityTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenControlledWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveControlledWeapon(ActionRuntime, Execution, PhaseReceipt);

	const FShanmenControlledWeaponThreatPresenceReceipt FirstPresence =
		BuildLivingThreatPresence(
			ActionRuntime,
			Execution,
			{ ControlledTargetB, ControlledTargetA });
	FShanmenControlledWeaponThreatPresenceAuthority Authority;
	TestTrue(TEXT("Run-scoped presence authority captures exact ownership"),
		FShanmenControlledWeaponThreatPresenceAuthority::TryCreate(
			ControlledRunId,
			ControlledSourceEntityId,
			Authority)
		&& Authority.IsValid()
		&& Authority.GetAuthorityRevision() == 0
		&& Authority.NumConsumedIntents() == 0);

	const FShanmenControlledWeaponThreatPresenceConsumeResult First =
		Authority.Consume(FirstPresence);
	TestTrue(TEXT("First batch consumes every intent atomically"),
		First.IsSuccess()
		&& First.GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		&& First.GetAuthorityRevisionBefore() == 0
		&& First.GetAuthorityRevisionAfter() == 2
		&& First.GetReceipts().Num() == 2
		&& First.GetReceipts()[0].GetAuthorityRevision() == 1
		&& First.GetReceipts()[1].GetAuthorityRevision() == 2
		&& First.GetReceipts()[0].GetIntent().GetCandidate().TargetEntityId
			== ControlledTargetA
		&& First.GetReceipts()[1].GetIntent().GetCandidate().TargetEntityId
			== ControlledTargetB
		&& Authority.GetAuthorityRevision() == 2
		&& Authority.NumConsumedIntents() == 2
		&& Authority.Contains(
			First.GetReceipts()[0].GetIntent().GetIntentId()));

	const FShanmenControlledWeaponThreatPresenceConsumeResult Replay =
		Authority.Consume(FirstPresence);
	TestTrue(TEXT("Exact replay returns the original audit without mutation"),
		Replay.IsSuccess()
		&& Replay.GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::AlreadyConsumed
		&& Replay.GetAuthorityRevisionBefore() == 2
		&& Replay.GetAuthorityRevisionAfter() == 2
		&& Replay.GetReceipts().Num() == 2
		&& Replay.GetReceipts()[0].GetAuthorityRevision()
			== First.GetReceipts()[0].GetAuthorityRevision()
		&& Replay.GetReceipts()[1].GetAuthorityRevision()
			== First.GetReceipts()[1].GetAuthorityRevision()
		&& Authority.NumConsumedIntents() == 2);

	const FShanmenControlledWeaponThreatPresenceReceipt LaterPresence =
		BuildLivingThreatPresence(
			ActionRuntime, Execution, { ControlledTargetA });
	const FShanmenControlledWeaponThreatPresenceConsumeResult Later =
		Authority.Consume(LaterPresence);
	TestTrue(TEXT("A later sample advances the authority exactly once"),
		Later.IsSuccess()
		&& Later.GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		&& Later.GetAuthorityRevisionBefore() == 2
		&& Later.GetAuthorityRevisionAfter() == 3
		&& Later.GetReceipts().Num() == 1
		&& Later.GetReceipts()[0].GetAuthorityRevision() == 3
		&& Authority.NumConsumedIntents() == 3);

	const FShanmenControlledWeaponThreatPresenceReceipt EmptyPresence =
		BuildLivingThreatPresence(ActionRuntime, Execution, {});
	const FShanmenControlledWeaponThreatPresenceConsumeResult Empty =
		Authority.Consume(EmptyPresence);
	TestTrue(TEXT("A completed empty sample is an explicit no-op"),
		Empty.IsSuccess()
		&& Empty.GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Empty.GetAuthorityRevisionBefore() == 3
		&& Empty.GetAuthorityRevisionAfter() == 3
		&& Empty.GetReceipts().IsEmpty()
		&& Authority.NumConsumedIntents() == 3);

	FShanmenControlledWeaponThreatPresenceAuthority ForeignRun;
	check(FShanmenControlledWeaponThreatPresenceAuthority::TryCreate(
		FGuid(0x56F00001, 0, 0, 1),
		ControlledSourceEntityId,
		ForeignRun));
	const FShanmenControlledWeaponThreatPresenceConsumeResult RunRejected =
		ForeignRun.Consume(FirstPresence);
	TestTrue(TEXT("A foreign Run rejects the complete batch without mutation"),
		RunRejected.IsValid()
		&& !RunRejected.IsSuccess()
		&& RunRejected.GetError()
			== EShanmenControlledWeaponThreatPresenceConsumeError::RunMismatch
		&& ForeignRun.NumConsumedIntents() == 0);

	FShanmenControlledWeaponThreatPresenceAuthority ForeignSource;
	check(FShanmenControlledWeaponThreatPresenceAuthority::TryCreate(
		ControlledRunId,
		FGuid(0x56F00002, 0, 0, 1),
		ForeignSource));
	const FShanmenControlledWeaponThreatPresenceConsumeResult SourceRejected =
		ForeignSource.Consume(FirstPresence);
	TestTrue(TEXT("A foreign source rejects the batch without mutation"),
		SourceRejected.IsValid()
		&& !SourceRejected.IsSuccess()
		&& SourceRejected.GetError()
			== EShanmenControlledWeaponThreatPresenceConsumeError::SourceMismatch
		&& ForeignSource.NumConsumedIntents() == 0);
	TestTrue(TEXT("Consumption remains a zero-effect audit boundary"),
		Execution.NumAcceptedImpacts() == 0
		&& Execution.GetState()
			== EShanmenControlledWeaponState::Orbiting);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponImpactTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.ControlledObjectImpacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponImpactTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenControlledWeaponExecution Execution;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveControlledWeapon(ActionRuntime, Execution, PhaseReceipt);
	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Physical flying sword launches"),
		Execution.TryIssueCommand(
			ActionRuntime, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector, Command));

	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("Directed sword opens first ControlledObject emission"),
		Execution.TryBeginEmission(ActionRuntime, FirstContext));
	TestTrue(TEXT("Detector kind remains distinct from a fire-and-forget projectile"),
		FirstContext.GetDetectorKind()
			== EShanmenHitDetectorKind::ControlledObject);
	const FShanmenHitCandidate TargetAFirst =
		MakeControlledCandidate(FirstContext, ControlledTargetA);
	FShanmenControlledWeaponImpactReceipt Impact;
	TestTrue(TEXT("Controlled contact resolves through the pure CombatCore path"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetAFirst,
			MakeControlledVitality(),
			MakeControlledDefense(),
			Impact));
	const FGuid FirstImpactId = Impact.GetResult().ImpactId;
	TestTrue(TEXT("Frozen formula produces ten plus forty times one quarter"),
		Impact.IsValid()
			&& FMath::IsNearlyEqual(Impact.GetResult().RawDamage, 20.0f)
			&& FMath::IsNearlyEqual(Impact.GetResult().FinalDamage, 20.0f));
	TestTrue(TEXT("Impact retains exact physical source item identity"),
		Impact.GetRequest().Action.GetSourceItemInstanceId()
			== ControlledItemId);
	TestFalse(TEXT("Repeated contact callback cannot resolve twice"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			TargetAFirst,
			MakeControlledVitality(),
			MakeControlledDefense(),
			Impact));
	TestTrue(TEXT("A second target shares the emission but gets a distinct Impact"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeControlledCandidate(FirstContext, ControlledTargetB),
			MakeControlledVitality(),
			MakeControlledDefense(),
			Impact)
			&& Impact.GetResult().ImpactId != FirstImpactId);
	TestTrue(TEXT("First emission closes"),
		Execution.TryEndEmission(ActionRuntime));

	TestTrue(TEXT("Steering changes direction without replacing item identity"),
		Execution.TryIssueCommand(
			ActionRuntime, 1,
			EShanmenControlledWeaponCommandKind::Redirect,
			FVector::RightVector, Command)
			&& Command.GetSourceItemInstanceId() == ControlledItemId);
	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("Steered sword opens a later emission"),
		Execution.TryBeginEmission(ActionRuntime, SecondContext));
	TestEqual(TEXT("Later controlled sample advances the stable ordinal"),
		SecondContext.GetHitOrdinal(), 1);
	TestTrue(TEXT("Same target may be hit in a later controlled sample"),
		Execution.TryResolveCandidate(
			ActionRuntime,
			MakeControlledCandidate(SecondContext, ControlledTargetA),
			MakeControlledVitality(),
			MakeControlledDefense(),
			Impact)
			&& Impact.GetResult().ImpactId != FirstImpactId);
	TestEqual(TEXT("Three unique controlled impacts are accepted"),
		Execution.NumAcceptedImpacts(), 3);
	TestTrue(TEXT("Active action may be interrupted with contact open"),
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active, PhaseReceipt));
	TestFalse(TEXT("Terminal action cannot use active-only close"),
		Execution.TryEndEmission(ActionRuntime));
	Execution.EndEmissionForTermination();
	TestFalse(TEXT("Termination cleanup closes the contact window"),
		Execution.IsEmissionActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponDeterministicReplayTest,
	"Shanmen.0_0_10.CombatRuntime.ControlledWeapon.DeterministicControlReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponDeterministicReplayTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator FirstAction;
	FShanmenControlledWeaponExecution First;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveControlledWeapon(FirstAction, First, PhaseReceipt);
	FShanmenActionOrchestrator ReplayAction;
	FShanmenControlledWeaponExecution Replay;
	StartActiveControlledWeapon(ReplayAction, Replay, PhaseReceipt);

	FShanmenControlledWeaponCommandReceipt FirstReceipt;
	FShanmenControlledWeaponCommandReceipt ReplayReceipt;
	const FVector LaunchDirection(2.0, 3.0, 1.0);
	TestTrue(TEXT("First launch succeeds"),
		First.TryIssueCommand(
			FirstAction, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			LaunchDirection, FirstReceipt));
	TestTrue(TEXT("Replay launch succeeds"),
		Replay.TryIssueCommand(
			ReplayAction, 0,
			EShanmenControlledWeaponCommandKind::Launch,
			LaunchDirection, ReplayReceipt));
	TestTrue(TEXT("Same action item sequence and direction reproduce command identity"),
		CommandReceiptsMatch(FirstReceipt, ReplayReceipt));

	const FVector RedirectDirection(-1.0, 4.0, 2.0);
	TestTrue(TEXT("First redirect succeeds"),
		First.TryIssueCommand(
			FirstAction, 1,
			EShanmenControlledWeaponCommandKind::Redirect,
			RedirectDirection, FirstReceipt));
	TestTrue(TEXT("Replay redirect succeeds"),
		Replay.TryIssueCommand(
			ReplayAction, 1,
			EShanmenControlledWeaponCommandKind::Redirect,
			RedirectDirection, ReplayReceipt));
	TestTrue(TEXT("Redirect receipt replays exactly"),
		CommandReceiptsMatch(FirstReceipt, ReplayReceipt));
	TestTrue(TEXT("Different commands own distinct deterministic identities"),
		FirstReceipt.GetCommandId()
			!= First.GetAction().GetActivationId());
	return true;
}

#endif
