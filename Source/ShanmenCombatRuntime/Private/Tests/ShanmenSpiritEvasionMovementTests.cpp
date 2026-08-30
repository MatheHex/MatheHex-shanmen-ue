#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritEvasionMovement.h"

namespace
{
	const FGuid MovementRunId(
		0xDB100001, 0xDB100002, 0xDB100003, 0xDB100004);
	const FGuid MovementOwnerId(
		0xDB110001, 0xDB110002, 0xDB110003, 0xDB110004);
	const FGuid MovementSourceId(
		0xDB120001, 0xDB120002, 0xDB120003, 0xDB120004);

	FShanmenCombatActionSnapshot MakeAction(
		FName ActionDefinitionId,
		const FString& Digest,
		uint64 ActivationSequence)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = MovementRunId;
		Capture.OwnerId = MovementOwnerId;
		Capture.SourceEntityId = MovementSourceId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P10.1");
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

	FShanmenCombatActionSnapshot MakeMovementAction(
		const FString& Digest = TEXT("TEST-DIGEST-P10.1"),
		uint64 ActivationSequence = 1101)
	{
		return MakeAction(
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId(),
			Digest,
			ActivationSequence);
	}

	FShanmenSpiritEvasionDefinition MakeDefinition(
		FName RuleId = TEXT("Defense.Spell.SpiritEvasion01"))
	{
		FShanmenSpiritEvasionDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = RuleId;
		FShanmenSpiritEvasionDefinition Definition;
		check(FShanmenSpiritEvasionDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	void OpenWindow(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenSpiritEvasionWindow& OutWindow)
	{
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Commit));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		FShanmenSpiritEvasionWindowReceipt OpenReceipt;
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			MakeDefinition(),
			Commit,
			OutRuntime,
			OutWindow,
			OpenReceipt));
	}

	FShanmenSpiritEvasionMovementIntent MakeIntent(
		const FShanmenCombatActionSnapshot& Action,
		FName MovementPolicyId =
			TEXT("Movement.Spell.SpiritEvasion.GroundStep"),
		const FVector& CandidateDirection = FVector(3.0, 4.0, 0.0))
	{
		FShanmenSpiritEvasionMovementIntentCapture Capture;
		Capture.Action = Action;
		Capture.MovementPolicyId = MovementPolicyId;
		Capture.CandidateDirection = CandidateDirection;
		FShanmenSpiritEvasionMovementIntent Intent;
		check(FShanmenSpiritEvasionMovementIntent::TryCapture(
			Capture, Intent));
		return Intent;
	}

	FShanmenSpiritEvasionMovementRequest MakeRequest(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenSpiritEvasionWindow& OutWindow,
		FName MovementPolicyId =
			TEXT("Movement.Spell.SpiritEvasion.GroundStep"),
		const FVector& CandidateDirection = FVector(3.0, 4.0, 0.0))
	{
		OpenWindow(Action, OutRuntime, OutWindow);
		const FShanmenSpiritEvasionMovementIntent Intent = MakeIntent(
			Action, MovementPolicyId, CandidateDirection);
		FShanmenSpiritEvasionMovementRequest Request;
		check(FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, OutWindow, OutRuntime, Request));
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionMovementIntentCaptureTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement.IntentCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionMovementIntentCaptureTest::RunTest(const FString&)
{
	FShanmenSpiritEvasionMovementIntentCapture Capture;
	Capture.Action = MakeMovementAction();
	Capture.MovementPolicyId =
		TEXT("Movement.Spell.SpiritEvasion.GroundStep");
	Capture.CandidateDirection = FVector(3.0, 4.0, 7.0);
	FShanmenSpiritEvasionMovementIntent Intent;
	TestTrue(TEXT("Valid input freezes one normalized planar intent"),
		FShanmenSpiritEvasionMovementIntent::TryCapture(Capture, Intent)
			&& Intent.IsValid()
			&& Intent.GetIntentId().IsValid()
			&& Intent.GetAction().GetActivationId()
				== Capture.Action.GetActivationId()
			&& Intent.GetMovementPolicyId() == Capture.MovementPolicyId
			&& FMath::IsNearlyEqual(
				Intent.GetPlanarDirection().X, 0.6)
			&& FMath::IsNearlyEqual(
				Intent.GetPlanarDirection().Y, 0.8)
			&& Intent.GetPlanarDirection().Z == 0.0
			&& FMath::IsNearlyEqual(
				Intent.GetPlanarDirection().SizeSquared2D(), 1.0));

	Capture.MovementPolicyId = NAME_None;
	TestFalse(TEXT("Movement policy identity is mandatory"),
		FShanmenSpiritEvasionMovementIntent::TryCapture(Capture, Intent));
	Capture.MovementPolicyId =
		TEXT("Movement.Spell.SpiritEvasion.GroundStep");
	Capture.CandidateDirection = FVector(0.0, 0.0, 10.0);
	TestFalse(TEXT("Vertical-only input cannot invent a planar direction"),
		FShanmenSpiritEvasionMovementIntent::TryCapture(Capture, Intent));
	Capture.CandidateDirection = FVector::ForwardVector;
	Capture.Action = MakeAction(
		TEXT("Combat.Action.Spell.GenericEvasion"),
		TEXT("TEST-DIGEST-P10.1-GENERIC"),
		1102);
	TestFalse(TEXT("A foreign action cannot emit spirit-evasion movement"),
		FShanmenSpiritEvasionMovementIntent::TryCapture(Capture, Intent));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionMovementActiveWindowRequestTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement.ActiveWindowRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionMovementActiveWindowRequestTest::RunTest(
	const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeMovementAction();
	const FShanmenSpiritEvasionMovementIntent Intent = MakeIntent(Action);
	FShanmenActionOrchestrator Runtime;
	FShanmenSpiritEvasionWindow Window;
	OpenWindow(Action, Runtime, Window);
	FShanmenSpiritEvasionMovementRequest Request;
	TestTrue(TEXT("Exact active window emits one movement-policy request"),
		Window.IsActiveFor(Runtime)
			&& FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
				Intent, Window, Runtime, Request)
			&& Request.IsValid()
			&& Request.GetRequestId().IsValid()
			&& Request.GetIntent().GetIntentId() == Intent.GetIntentId()
			&& Request.GetWindow().GetReceiptId()
				== Window.GetOpenReceipt().GetReceiptId());

	const FShanmenCombatActionSnapshot ForeignAction = MakeMovementAction(
		TEXT("TEST-DIGEST-P10.1-FOREIGN"), 1103);
	FShanmenActionOrchestrator ForeignRuntime;
	FShanmenSpiritEvasionWindow ForeignWindow;
	OpenWindow(ForeignAction, ForeignRuntime, ForeignWindow);
	TestFalse(TEXT("A foreign runtime cannot validate the original window"),
		FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, Window, ForeignRuntime, Request));
	TestFalse(TEXT("A foreign window cannot carry the original intent"),
		FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, ForeignWindow, ForeignRuntime, Request));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionMovementLifecycleBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement.LifecycleBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionMovementLifecycleBoundaryTest::RunTest(
	const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeMovementAction();
	const FShanmenSpiritEvasionMovementIntent Intent = MakeIntent(Action);
	FShanmenActionOrchestrator Runtime;
	FShanmenSpiritEvasionWindow Window;
	OpenWindow(Action, Runtime, Window);
	FShanmenSpiritEvasionMovementRequest Request;
	check(FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
		Intent, Window, Runtime, Request));
	FShanmenActionTransitionReceipt Recovery;
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	TestTrue(TEXT("Existing request remains immutable audit evidence"),
		Request.IsValid());
	TestFalse(TEXT("Recovery closes future movement request emission"),
		Window.IsActiveFor(Runtime)
			|| FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
				Intent, Window, Runtime, Request));

	const FShanmenCombatActionSnapshot InterruptedAction = MakeMovementAction(
		TEXT("TEST-DIGEST-P10.1-INTERRUPTED"), 1104);
	const FShanmenSpiritEvasionMovementIntent InterruptedIntent =
		MakeIntent(InterruptedAction);
	FShanmenActionOrchestrator InterruptedRuntime;
	FShanmenSpiritEvasionWindow InterruptedWindow;
	OpenWindow(
		InterruptedAction, InterruptedRuntime, InterruptedWindow);
	FShanmenActionTransitionReceipt Interruption;
	check(InterruptedRuntime.TryInterrupt(
		EShanmenCombatActionPhase::Active, Interruption));
	TestFalse(TEXT("Interruption closes movement request emission immediately"),
		FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			InterruptedIntent,
			InterruptedWindow,
			InterruptedRuntime,
			Request));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionMovementDeterministicReplayTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionMovementDeterministicReplayTest::RunTest(
	const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeMovementAction();
	const FShanmenSpiritEvasionMovementIntent FirstIntent = MakeIntent(
		Action,
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"),
		FVector(3.0, 4.0, 20.0));
	const FShanmenSpiritEvasionMovementIntent ReplayIntent = MakeIntent(
		Action,
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"),
		FVector(6.0, 8.0, -20.0));
	TestTrue(TEXT("Scaled planar input reproduces the frozen intent"),
		ReplayIntent.GetIntentId() == FirstIntent.GetIntentId()
			&& ReplayIntent.GetPlanarDirection()
				== FirstIntent.GetPlanarDirection());

	FShanmenActionOrchestrator FirstRuntime;
	FShanmenSpiritEvasionWindow FirstWindow;
	OpenWindow(Action, FirstRuntime, FirstWindow);
	FShanmenSpiritEvasionMovementRequest FirstRequest;
	check(FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
		FirstIntent, FirstWindow, FirstRuntime, FirstRequest));
	FShanmenActionOrchestrator ReplayRuntime;
	FShanmenSpiritEvasionWindow ReplayWindow;
	OpenWindow(Action, ReplayRuntime, ReplayWindow);
	FShanmenSpiritEvasionMovementRequest ReplayRequest;
	TestTrue(TEXT("Equivalent replay reproduces the request identity"),
		FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			ReplayIntent, ReplayWindow, ReplayRuntime, ReplayRequest)
			&& ReplayRequest.GetRequestId()
				== FirstRequest.GetRequestId());

	const FShanmenSpiritEvasionMovementIntent OtherPolicy = MakeIntent(
		Action,
		TEXT("Movement.Spell.SpiritEvasion.AerialStep"),
		FVector(3.0, 4.0, 0.0));
	const FShanmenSpiritEvasionMovementIntent OtherDirection = MakeIntent(
		Action,
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"),
		FVector(4.0, 3.0, 0.0));
	const FShanmenSpiritEvasionMovementIntent OtherAction = MakeIntent(
		MakeMovementAction(TEXT("TEST-DIGEST-P10.1-OTHER"), 1105));
	TestTrue(TEXT("Policy, direction and action identity remain distinct"),
		OtherPolicy.GetIntentId() != FirstIntent.GetIntentId()
			&& OtherDirection.GetIntentId() != FirstIntent.GetIntentId()
			&& OtherAction.GetIntentId() != FirstIntent.GetIntentId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionMovementIdempotencyLedgerTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement.IdempotencyLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionMovementIdempotencyLedgerTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenSpiritEvasionWindow FirstWindow;
	const FShanmenSpiritEvasionMovementRequest First = MakeRequest(
		MakeMovementAction(), FirstRuntime, FirstWindow);
	FShanmenSpiritEvasionMovementLedger Ledger;
	TestTrue(TEXT("First valid request is accepted exactly once"),
		Ledger.TryAccept(First)
			&& Ledger.Contains(First.GetRequestId())
			&& Ledger.Num() == 1);
	TestFalse(TEXT("Exact replay is rejected by request identity"),
		Ledger.TryAccept(First));

	FShanmenActionOrchestrator SecondRuntime;
	FShanmenSpiritEvasionWindow SecondWindow;
	const FShanmenSpiritEvasionMovementRequest Second = MakeRequest(
		MakeMovementAction(TEXT("TEST-DIGEST-P10.1-SECOND"), 1106),
		SecondRuntime,
		SecondWindow);
	TestTrue(TEXT("Distinct action request remains independently acceptable"),
		Ledger.TryAccept(Second) && Ledger.Num() == 2);
	Ledger.Reset();
	TestTrue(TEXT("Reset clears the per-run delivery gate"),
		Ledger.Num() == 0
			&& !Ledger.Contains(First.GetRequestId())
			&& Ledger.TryAccept(First));
	TestFalse(TEXT("Invalid requests never enter the ledger"),
		Ledger.TryAccept(FShanmenSpiritEvasionMovementRequest()));
	return true;
}

#endif
