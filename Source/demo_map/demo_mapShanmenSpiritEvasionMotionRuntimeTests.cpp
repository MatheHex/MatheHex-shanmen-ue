#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapShanmenSpiritEvasionMotionRuntime.h"

namespace
{
	const FGuid MotionRunId(
		0xDD100001, 0xDD100002, 0xDD100003, 0xDD100004);
	const FGuid MotionOwnerId(
		0xDD110001, 0xDD110002, 0xDD110003, 0xDD110004);
	const FGuid MotionSourceId(
		0xDD120001, 0xDD120002, 0xDD120003, 0xDD120004);
	const FName MotionPolicyId(
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"));

	FShanmenCombatActionSnapshot MakeMotionAction(
		const FString& Digest = TEXT("TEST-DIGEST-P10.3"),
		uint64 ActivationSequence = 1301)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = MotionRunId;
		Capture.OwnerId = MotionOwnerId;
		Capture.SourceEntityId = MotionSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.3");
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

	FShanmenSpiritEvasionDefinition MakeMotionDefinition()
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

	FShanmenSpiritEvasionMovementRequest MakeMotionRequest(
		const FShanmenCombatActionSnapshot& Action = MakeMotionAction())
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Commit));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		FShanmenSpiritEvasionWindow Window;
		FShanmenSpiritEvasionWindowReceipt OpenReceipt;
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			MakeMotionDefinition(),
			Commit,
			Runtime,
			Window,
			OpenReceipt));

		FShanmenSpiritEvasionMovementIntentCapture IntentCapture;
		IntentCapture.Action = Action;
		IntentCapture.MovementPolicyId = MotionPolicyId;
		IntentCapture.CandidateDirection = FVector(3.0, 4.0, 8.0);
		FShanmenSpiritEvasionMovementIntent Intent;
		check(FShanmenSpiritEvasionMovementIntent::TryCapture(
			IntentCapture, Intent));
		FShanmenSpiritEvasionMovementRequest Request;
		check(FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, Window, Runtime, Request));
		return Request;
	}

	Fdemo_mapShanmenSpiritEvasionMovementPlan MakeMovementPlan(
		const FShanmenCombatActionSnapshot& Action = MakeMotionAction())
	{
		Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture PolicyCapture;
		PolicyCapture.MovementPolicyId = MotionPolicyId;
		PolicyCapture.RequestedDistance = 400.0f;
		PolicyCapture.MinimumResolvedDistance = 100.0f;
		PolicyCapture.WorldStaticClearance = 2.0f;
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
		check(Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			PolicyCapture, Policy));
		Fdemo_mapShanmenSpiritEvasionMovementPlan Plan;
		check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
			MakeMotionRequest(Action), Policy, Plan));
		return Plan;
	}

	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot MakeTrajectory(
		float DurationSeconds = 0.4f,
		int32 SegmentCount = 4)
	{
		Fdemo_mapShanmenSpiritEvasionTrajectoryCapture Capture;
		Capture.DurationSeconds = DurationSeconds;
		Capture.SegmentCount = SegmentCount;
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
		check(Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory));
		return Trajectory;
	}

	Fdemo_mapShanmenSpiritEvasionMotionPlan MakeMotionPlan()
	{
		Fdemo_mapShanmenSpiritEvasionMotionPlan Plan;
		check(Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
			MakeMovementPlan(), MakeTrajectory(), Plan));
		return Plan;
	}

	Fdemo_mapShanmenSpiritEvasionMovementPreflightResult MakeReadyPreflight(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& Plan,
		float ResolvedDistance = 400.0f)
	{
		Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Result;
		Result.Status =
			Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready;
		Result.PlanId = Plan.GetMovementPlan().GetPlanId();
		Result.Displacement.RequestedDistance = Plan.GetMovementPlan()
			.GetPolicy().GetRequestedDistance();
		Result.Displacement.ResolvedDistance = ResolvedDistance;
		Result.Displacement.bBlocked =
			ResolvedDistance < Result.Displacement.RequestedDistance;
		return Result;
	}

	Fdemo_mapShanmenSpiritEvasionMotionSession MakeMotionSession(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& Plan)
	{
		Fdemo_mapShanmenSpiritEvasionMotionSession Session;
		check(Fdemo_mapShanmenSpiritEvasionMotionSession::TryStart(
			Plan, MakeReadyPreflight(Plan), Session));
		return Session;
	}

	double ScheduledTime(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& Plan,
		int32 SegmentOrdinal)
	{
		return static_cast<double>(Plan.GetTrajectory().GetDurationSeconds())
			* static_cast<double>(SegmentOrdinal)
			/ static_cast<double>(Plan.GetTrajectory().GetSegmentCount());
	}

	Fdemo_mapShanmenSpiritEvasionSegmentReceipt MakeSegmentReceipt(
		const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command,
		float ResolvedDistance,
		bool bBlocked)
	{
		Fdemo_mapCombatDisplacementResult Displacement;
		Displacement.RequestedDistance = Command.GetRequestedDistance();
		Displacement.ResolvedDistance = ResolvedDistance;
		Displacement.bBlocked = bBlocked;
		Fdemo_mapShanmenSpiritEvasionSegmentReceipt Receipt;
		check(Fdemo_mapShanmenSpiritEvasionSegmentReceipt::TryCapture(
			Command, Displacement, Receipt));
		return Receipt;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionTrajectoryCaptureTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.TrajectoryCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionTrajectoryCaptureTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSpiritEvasionTrajectoryCapture Capture;
	Capture.DurationSeconds = 0.4f;
	Capture.SegmentCount = 4;
	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
	TestTrue(TEXT("Explicit duration and segment count freeze one trajectory"),
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory)
			&& Trajectory.IsValid()
			&& Trajectory.GetDurationSeconds() == 0.4f
			&& Trajectory.GetSegmentCount() == 4);
	Capture.DurationSeconds = 0.0f;
	TestFalse(TEXT("Zero duration is rejected"),
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory));
	Capture.DurationSeconds = 0.4f;
	Capture.SegmentCount = 0;
	TestFalse(TEXT("Zero segment count is rejected"),
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMotionPlanBindingTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.PlanBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMotionPlanBindingTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenSpiritEvasionMovementPlan MovementPlan =
		MakeMovementPlan();
	Fdemo_mapShanmenSpiritEvasionMotionPlan First;
	TestTrue(TEXT("Movement plan and trajectory bind immutably"),
		Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
			MovementPlan, MakeTrajectory(), First)
			&& First.IsValid()
			&& First.GetMotionPlanId().IsValid());
	Fdemo_mapShanmenSpiritEvasionMotionPlan Replay;
	check(Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
		MovementPlan, MakeTrajectory(), Replay));
	Fdemo_mapShanmenSpiritEvasionMotionPlan OtherDuration;
	check(Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
		MovementPlan, MakeTrajectory(0.5f), OtherDuration));
	TestTrue(TEXT("Equivalent replay is stable and duration remains distinct"),
		Replay.GetMotionPlanId() == First.GetMotionPlanId()
			&& OtherDuration.GetMotionPlanId() != First.GetMotionPlanId());

	Fdemo_mapShanmenSpiritEvasionMotionPlan Invalid;
	TestFalse(TEXT("Invalid movement plan cannot produce motion"),
		Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
			Fdemo_mapShanmenSpiritEvasionMovementPlan(),
			MakeTrajectory(),
			Invalid));
	Fdemo_mapShanmenSpiritEvasionMotionSession Session;
	Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Mismatch =
		MakeReadyPreflight(First);
	Mismatch.PlanId = FGuid(1, 2, 3, 4);
	TestFalse(TEXT("Foreign preflight cannot start the session"),
		Fdemo_mapShanmenSpiritEvasionMotionSession::TryStart(
			First, Mismatch, Session));
	Mismatch = MakeReadyPreflight(First);
	Mismatch.Displacement.RequestedDistance = 399.0f;
	TestFalse(TEXT("Preflight request distance must match frozen policy"),
		Fdemo_mapShanmenSpiritEvasionMotionSession::TryStart(
			First, Mismatch, Session));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMotionScheduleTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.DeterministicSchedule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMotionScheduleTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenSpiritEvasionMotionPlan Plan = MakeMotionPlan();
	Fdemo_mapShanmenSpiritEvasionMotionSession First = MakeMotionSession(Plan);
	const double FirstDue = ScheduledTime(Plan, 1);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand FirstCommand;
	TestFalse(TEXT("Segment cannot issue before its deterministic boundary"),
		First.TryIssueNextCommand(FirstDue - 0.001, FirstCommand));
	TestTrue(TEXT("First boundary issues exactly one quarter-distance command"),
		First.TryIssueNextCommand(FirstDue, FirstCommand)
			&& FirstCommand.IsValid()
			&& FirstCommand.GetSegmentOrdinal() == 1
			&& FirstCommand.GetSegmentCount() == 4
			&& FirstCommand.GetScheduledElapsedSeconds() == FirstDue
			&& FirstCommand.GetRequestedDistance() == 100.0f);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Duplicate;
	TestFalse(TEXT("Pending command prevents duplicate delivery"),
		First.TryIssueNextCommand(FirstDue, Duplicate));

	Fdemo_mapShanmenSpiritEvasionMotionSession Replay = MakeMotionSession(Plan);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand ReplayCommand;
	TestTrue(TEXT("Equivalent elapsed sample reproduces command identity"),
		Replay.TryIssueNextCommand(FirstDue, ReplayCommand)
			&& ReplayCommand.GetCommandId() == FirstCommand.GetCommandId());
	TestTrue(TEXT("Accepted receipt permits deterministic catch-up"),
		First.TryAcceptReceipt(MakeSegmentReceipt(
			FirstCommand, FirstCommand.GetRequestedDistance(), false)));
	Fdemo_mapShanmenSpiritEvasionSegmentCommand CatchUp;
	TestTrue(TEXT("Late sample emits only the next ordinal"),
		First.TryIssueNextCommand(
			ScheduledTime(Plan, Plan.GetTrajectory().GetSegmentCount()),
			CatchUp)
			&& CatchUp.GetSegmentOrdinal() == 2
			&& CatchUp.GetScheduledElapsedSeconds() == ScheduledTime(Plan, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMotionCompletionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.CompletionLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMotionCompletionTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenSpiritEvasionMotionPlan Plan = MakeMotionPlan();
	Fdemo_mapShanmenSpiritEvasionMotionSession Session = MakeMotionSession(Plan);
	Fdemo_mapShanmenSpiritEvasionSegmentReceipt FirstReceipt;
	for (int32 Ordinal = 1; Ordinal <= 4; ++Ordinal)
	{
		Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
		TestTrue(FString::Printf(TEXT("Segment %d issues"), Ordinal),
			Session.TryIssueNextCommand(
				ScheduledTime(Plan, Ordinal), Command)
				&& Command.GetSegmentOrdinal() == Ordinal);
		const Fdemo_mapShanmenSpiritEvasionSegmentReceipt Receipt =
			MakeSegmentReceipt(
				Command, Command.GetRequestedDistance(), false);
		if (Ordinal == 1)
		{
			FirstReceipt = Receipt;
		}
		TestTrue(FString::Printf(TEXT("Segment %d receipt commits"), Ordinal),
			Session.TryAcceptReceipt(Receipt));
	}
	TestTrue(TEXT("All four receipts complete exact target distance"),
		Session.IsValid()
			&& Session.GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Completed
			&& Session.GetAcceptedSegmentCount() == 4
			&& Session.GetResolvedDistance() == 400.0f
			&& !Session.HasPendingCommand());
	TestFalse(TEXT("Receipt replay is rejected after completion"),
		Session.TryAcceptReceipt(FirstReceipt));
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Extra;
	TestFalse(TEXT("Completed session cannot issue a fifth segment"),
		Session.TryIssueNextCommand(ScheduledTime(Plan, 4), Extra));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMotionBlockedTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.BlockedTermination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMotionBlockedTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenSpiritEvasionMotionPlan Plan = MakeMotionPlan();
	Fdemo_mapShanmenSpiritEvasionMotionSession Session = MakeMotionSession(Plan);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
	check(Session.TryIssueNextCommand(ScheduledTime(Plan, 1), Command));
	Fdemo_mapCombatDisplacementResult InvalidShortMove;
	InvalidShortMove.RequestedDistance = Command.GetRequestedDistance();
	InvalidShortMove.ResolvedDistance = 40.0f;
	Fdemo_mapShanmenSpiritEvasionSegmentReceipt InvalidReceipt;
	TestFalse(TEXT("Short unblocked movement cannot forge a commit"),
		Fdemo_mapShanmenSpiritEvasionSegmentReceipt::TryCapture(
			Command, InvalidShortMove, InvalidReceipt));
	TestTrue(TEXT("Blocked actual movement terminates after one receipt"),
		Session.TryAcceptReceipt(MakeSegmentReceipt(Command, 40.0f, true))
			&& Session.GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Blocked
			&& Session.GetAcceptedSegmentCount() == 1
			&& Session.GetResolvedDistance() == 40.0f);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Extra;
	TestFalse(TEXT("Blocked session cannot issue another segment"),
		Session.TryIssueNextCommand(ScheduledTime(Plan, 4), Extra));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceiptTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.TerminationReceipt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionMotionTerminationReceiptTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenSpiritEvasionMotionPlan Plan = MakeMotionPlan();
	Fdemo_mapShanmenSpiritEvasionMotionSession Session = MakeMotionSession(Plan);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
	check(Session.TryIssueNextCommand(ScheduledTime(Plan, 1), Command));
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt Termination;
	TestTrue(TEXT("Explicit termination closes and audits the pending command"),
		Session.TryTerminate(
			Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
			ExplicitCancel,
			Termination)
			&& Session.IsValid()
			&& Session.GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Terminated
			&& Session.GetTerminationReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExplicitCancel
			&& !Session.HasPendingCommand()
			&& Termination.IsValid()
			&& Termination.GetSessionId() == Session.GetSessionId()
			&& Termination.GetPendingCommandId() == Command.GetCommandId()
			&& Termination.GetAcceptedSegmentCount() == 0
			&& Termination.GetResolvedDistance() == 0.0f
			&& Termination.GetLastElapsedSeconds() == ScheduledTime(Plan, 1));
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt Duplicate;
	TestFalse(TEXT("Terminal session cannot emit a second closure"),
		Session.TryTerminate(
			Edemo_mapShanmenSpiritEvasionMotionTerminationReason::ActionEnded,
			Duplicate));
	TestFalse(TEXT("Terminal session cannot accept its abandoned command"),
		Session.TryAcceptReceipt(MakeSegmentReceipt(
			Command, Command.GetRequestedDistance(), false)));
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Extra;
	TestFalse(TEXT("Terminal session cannot issue another segment"),
		Session.TryIssueNextCommand(ScheduledTime(Plan, 4), Extra));

	Fdemo_mapShanmenSpiritEvasionMotionSession Replay = MakeMotionSession(Plan);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand ReplayCommand;
	check(Replay.TryIssueNextCommand(ScheduledTime(Plan, 1), ReplayCommand));
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt ReplayTermination;
	check(Replay.TryTerminate(
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason::ExplicitCancel,
		ReplayTermination));
	TestEqual(TEXT("Equivalent closure reproduces termination identity"),
		ReplayTermination.GetReceiptId(), Termination.GetReceiptId());

	Fdemo_mapShanmenSpiritEvasionMotionSession Fresh = MakeMotionSession(Plan);
	TestFalse(TEXT("None is not a terminal reason"),
		Fresh.TryTerminate(
			Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None,
			Duplicate));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionSegmentExecutionBoundaryTest,
	"Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime.ExecutionBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionSegmentExecutionBoundaryTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Invalid =
		Fdemo_mapShanmenSpiritEvasionSegmentExecutor::ExecuteSwept(
			nullptr, Fdemo_mapShanmenSpiritEvasionSegmentCommand());
	TestTrue(TEXT("Invalid command fails before product access"),
		Invalid.Status
			== Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::InvalidCommand
			&& !Invalid.CommandId.IsValid()
			&& !Invalid.HasReceipt());

	const Fdemo_mapShanmenSpiritEvasionMotionPlan Plan = MakeMotionPlan();
	Fdemo_mapShanmenSpiritEvasionMotionSession Session = MakeMotionSession(Plan);
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
	check(Session.TryIssueNextCommand(ScheduledTime(Plan, 1), Command));
	const Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Unavailable =
		Fdemo_mapShanmenSpiritEvasionSegmentExecutor::ExecuteSwept(
			nullptr, Command);
	TestTrue(TEXT("Unavailable Character returns no receipt or state mutation"),
		Unavailable.Status
			== Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
				CharacterUnavailable
			&& Unavailable.CommandId == Command.GetCommandId()
			&& !Unavailable.HasReceipt()
			&& Session.HasPendingCommand()
			&& Session.GetAcceptedSegmentCount() == 0);
	TestTrue(TEXT("Same pending command can later accept actual movement"),
		Session.TryAcceptReceipt(MakeSegmentReceipt(
			Command, Command.GetRequestedDistance(), false))
			&& Session.GetAcceptedSegmentCount() == 1
			&& !Session.HasPendingCommand());
	return true;
}

#endif
