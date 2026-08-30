#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapShanmenSpiritEvasionMovementAdapter.h"

namespace
{
	const FGuid ProductMovementRunId(
		0xDC100001, 0xDC100002, 0xDC100003, 0xDC100004);
	const FGuid ProductMovementOwnerId(
		0xDC110001, 0xDC110002, 0xDC110003, 0xDC110004);
	const FGuid ProductMovementSourceId(
		0xDC120001, 0xDC120002, 0xDC120003, 0xDC120004);
	const FName GroundStepPolicyId(
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"));

	FShanmenCombatActionSnapshot MakeProductMovementAction(
		const FString& Digest = TEXT("TEST-DIGEST-P10.2"),
		uint64 ActivationSequence = 1201)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ProductMovementRunId;
		Capture.OwnerId = ProductMovementOwnerId;
		Capture.SourceEntityId = ProductMovementSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.2");
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

	FShanmenSpiritEvasionDefinition MakeProductMovementDefinition()
	{
		FShanmenSpiritEvasionDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Spell.SpiritEvasion01");
		FShanmenSpiritEvasionDefinition Definition;
		check(FShanmenSpiritEvasionDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenSpiritEvasionMovementRequest MakeProductMovementRequest(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenSpiritEvasionWindow& OutWindow,
		FName MovementPolicyId = GroundStepPolicyId)
	{
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Commit));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		FShanmenSpiritEvasionWindowReceipt OpenReceipt;
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			MakeProductMovementDefinition(),
			Commit,
			OutRuntime,
			OutWindow,
			OpenReceipt));

		FShanmenSpiritEvasionMovementIntentCapture IntentCapture;
		IntentCapture.Action = Action;
		IntentCapture.MovementPolicyId = MovementPolicyId;
		IntentCapture.CandidateDirection = FVector(3.0, 4.0, 9.0);
		FShanmenSpiritEvasionMovementIntent Intent;
		check(FShanmenSpiritEvasionMovementIntent::TryCapture(
			IntentCapture, Intent));

		FShanmenSpiritEvasionMovementRequest Request;
		check(FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, OutWindow, OutRuntime, Request));
		return Request;
	}

	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot MakeProductPolicy(
		FName MovementPolicyId = GroundStepPolicyId,
		float RequestedDistance = 600.0f,
		float MinimumResolvedDistance = 120.0f,
		float WorldStaticClearance = 2.0f)
	{
		Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture Capture;
		Capture.MovementPolicyId = MovementPolicyId;
		Capture.RequestedDistance = RequestedDistance;
		Capture.MinimumResolvedDistance = MinimumResolvedDistance;
		Capture.WorldStaticClearance = WorldStaticClearance;
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
		check(Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
		return Policy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMovementPolicyCaptureTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter.PolicyCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMovementPolicyCaptureTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture Capture;
	Capture.MovementPolicyId = GroundStepPolicyId;
	Capture.RequestedDistance = 600.0f;
	Capture.MinimumResolvedDistance = 120.0f;
	Capture.WorldStaticClearance = 2.0f;
	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
	TestTrue(TEXT("Explicit product values freeze one valid policy"),
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy)
			&& Policy.IsValid()
			&& Policy.GetMovementPolicyId() == GroundStepPolicyId
			&& Policy.GetRequestedDistance() == 600.0f
			&& Policy.GetMinimumResolvedDistance() == 120.0f
			&& Policy.GetWorldStaticClearance() == 2.0f);

	Capture.MovementPolicyId = NAME_None;
	TestFalse(TEXT("Unnamed policy is rejected"),
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
	Capture.MovementPolicyId = GroundStepPolicyId;
	Capture.MinimumResolvedDistance = 0.0f;
	TestFalse(TEXT("Zero minimum distance is rejected"),
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
	Capture.MinimumResolvedDistance = 601.0f;
	TestFalse(TEXT("Minimum cannot exceed requested distance"),
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
	Capture.MinimumResolvedDistance = 120.0f;
	Capture.WorldStaticClearance = -1.0f;
	TestFalse(TEXT("Negative WorldStatic clearance is rejected"),
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMovementRequestPlanBindingTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter.RequestPlanBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMovementRequestPlanBindingTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenSpiritEvasionWindow Window;
	const FShanmenSpiritEvasionMovementRequest Request =
		MakeProductMovementRequest(
			MakeProductMovementAction(), Runtime, Window);
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy =
		MakeProductPolicy();
	Fdemo_mapShanmenSpiritEvasionMovementPlan Plan;
	TestTrue(TEXT("Matching request and policy bind to one immutable plan"),
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
			Request, Policy, Plan)
			&& Plan.IsValid()
			&& Plan.GetPlanId().IsValid()
			&& Plan.GetRequest().GetRequestId() == Request.GetRequestId()
			&& Plan.GetPolicy().GetMovementPolicyId()
				== Request.GetIntent().GetMovementPolicyId());

	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot OtherPolicy =
		MakeProductPolicy(
			TEXT("Movement.Spell.SpiritEvasion.AerialStep"));
	TestFalse(TEXT("Policy identity cannot be substituted after input capture"),
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
			Request, OtherPolicy, Plan));
	TestFalse(TEXT("Invalid request cannot produce a plan"),
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
			FShanmenSpiritEvasionMovementRequest(), Policy, Plan));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMovementDeterministicReplayTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMovementDeterministicReplayTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenSpiritEvasionWindow Window;
	const FShanmenSpiritEvasionMovementRequest Request =
		MakeProductMovementRequest(
			MakeProductMovementAction(), Runtime, Window);
	Fdemo_mapShanmenSpiritEvasionMovementPlan First;
	check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
		Request, MakeProductPolicy(), First));
	Fdemo_mapShanmenSpiritEvasionMovementPlan Replay;
	check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
		Request, MakeProductPolicy(), Replay));
	TestEqual(TEXT("Equivalent replay reproduces plan identity"),
		Replay.GetPlanId(), First.GetPlanId());

	Fdemo_mapShanmenSpiritEvasionMovementPlan OtherDistance;
	check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
		Request, MakeProductPolicy(GroundStepPolicyId, 601.0f), OtherDistance));
	FShanmenActionOrchestrator OtherRuntime;
	FShanmenSpiritEvasionWindow OtherWindow;
	const FShanmenSpiritEvasionMovementRequest OtherRequest =
		MakeProductMovementRequest(
			MakeProductMovementAction(
				TEXT("TEST-DIGEST-P10.2-OTHER"), 1202),
			OtherRuntime,
			OtherWindow);
	Fdemo_mapShanmenSpiritEvasionMovementPlan OtherAction;
	check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
		OtherRequest, MakeProductPolicy(), OtherAction));
	TestTrue(TEXT("Distance and action identities remain distinct"),
		OtherDistance.GetPlanId() != First.GetPlanId()
			&& OtherAction.GetPlanId() != First.GetPlanId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMovementExplicitClearanceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter.ExplicitClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMovementExplicitClearanceTest::RunTest(
	const FString&)
{
	TestEqual(TEXT("Blocked preflight applies caller-owned clearance"),
		Fdemo_mapCombatDisplacement::ClampPreflightDistance(
			100.0f, 60.0f, true, 2.0f),
		58.0f);
	TestEqual(TEXT("Unblocked preflight preserves requested distance"),
		Fdemo_mapCombatDisplacement::ClampPreflightDistance(
			100.0f, 0.0f, false, 2.0f),
		100.0f);
	TestEqual(TEXT("Clearance cannot produce negative displacement"),
		Fdemo_mapCombatDisplacement::ClampPreflightDistance(
			100.0f, 1.0f, true, 2.0f),
		0.0f);
	TestEqual(TEXT("Invalid clearance fails closed"),
		Fdemo_mapCombatDisplacement::ClampPreflightDistance(
			100.0f, 60.0f, true, -1.0f),
		0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMovementPreflightBoundaryTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter.PreflightBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMovementPreflightBoundaryTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenSpiritEvasionWindow Window;
	const FShanmenSpiritEvasionMovementRequest Request =
		MakeProductMovementRequest(
			MakeProductMovementAction(), Runtime, Window);
	Fdemo_mapShanmenSpiritEvasionMovementPlan Plan;
	check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
		Request, MakeProductPolicy(), Plan));
	TestTrue(TEXT("Exact minimum distance is accepted"),
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::
		IsResolvedDistanceAccepted(Plan, 120.0f));
	TestFalse(TEXT("Sub-minimum distance is rejected"),
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::
		IsResolvedDistanceAccepted(Plan, 119.0f));
	TestFalse(TEXT("Distance beyond the frozen request is rejected"),
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::
		IsResolvedDistanceAccepted(Plan, 601.0f));

	const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Invalid =
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::PreflightWorldStatic(
			nullptr, Fdemo_mapShanmenSpiritEvasionMovementPlan());
	TestTrue(TEXT("Invalid plan fails before product world access"),
		Invalid.Status
			== Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::InvalidPlan
			&& !Invalid.PlanId.IsValid()
			&& !Invalid.IsReady());
	const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Unavailable =
		Fdemo_mapShanmenSpiritEvasionMovementAdapter::PreflightWorldStatic(
			nullptr, Plan);
	TestTrue(TEXT("Unavailable Character is explicit and never moves anything"),
		Unavailable.Status
			== Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
				CharacterUnavailable
			&& Unavailable.PlanId == Plan.GetPlanId()
			&& Unavailable.Displacement.ResolvedDistance == 0.0f
			&& !Unavailable.IsReady());
	return true;
}

#endif
