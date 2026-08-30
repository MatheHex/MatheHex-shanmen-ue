#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapShanmenSpiritEvasionActionCoordinator.h"

namespace
{
	const FGuid CoordinatorRunId(
		0xDE100001, 0xDE100002, 0xDE100003, 0xDE100004);
	const FGuid CoordinatorOwnerId(
		0xDE110001, 0xDE110002, 0xDE110003, 0xDE110004);
	const FGuid CoordinatorSourceId(
		0xDE120001, 0xDE120002, 0xDE120003, 0xDE120004);
	const FName CoordinatorPolicyId(
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"));

	FShanmenCombatActionSnapshot MakeCoordinatorAction(
		uint64 ActivationSequence = 1401,
		const FString& Digest = TEXT("TEST-DIGEST-P10.4"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = CoordinatorRunId;
		Capture.OwnerId = CoordinatorOwnerId;
		Capture.SourceEntityId = CoordinatorSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.4");
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

	FShanmenSpiritEvasionDefinition MakeCoordinatorDefinition()
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

	struct FCoordinatorFixture
	{
		FShanmenCombatActionSnapshot Action;
		FShanmenActionOrchestrator ActionRuntime;
		FShanmenSpiritEvasionWindow Window;
		Fdemo_mapShanmenSpiritEvasionMotionPlan MotionPlan;
		Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Preflight;
	};

	FCoordinatorFixture MakeCoordinatorFixture(
		uint64 ActivationSequence = 1401,
		const FString& Digest = TEXT("TEST-DIGEST-P10.4"))
	{
		FCoordinatorFixture Fixture;
		Fixture.Action = MakeCoordinatorAction(ActivationSequence, Digest);
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(
			Fixture.Action, Fixture.ActionRuntime, Commit));
		check(Fixture.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		FShanmenSpiritEvasionWindowReceipt OpenReceipt;
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Fixture.Action,
			MakeCoordinatorDefinition(),
			Commit,
			Fixture.ActionRuntime,
			Fixture.Window,
			OpenReceipt));

		FShanmenSpiritEvasionMovementIntentCapture IntentCapture;
		IntentCapture.Action = Fixture.Action;
		IntentCapture.MovementPolicyId = CoordinatorPolicyId;
		IntentCapture.CandidateDirection = FVector(3.0, 4.0, 9.0);
		FShanmenSpiritEvasionMovementIntent Intent;
		check(FShanmenSpiritEvasionMovementIntent::TryCapture(
			IntentCapture, Intent));
		FShanmenSpiritEvasionMovementRequest Request;
		check(FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, Fixture.Window, Fixture.ActionRuntime, Request));

		Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture PolicyCapture;
		PolicyCapture.MovementPolicyId = CoordinatorPolicyId;
		PolicyCapture.RequestedDistance = 400.0f;
		PolicyCapture.MinimumResolvedDistance = 100.0f;
		PolicyCapture.WorldStaticClearance = 2.0f;
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
		check(Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			PolicyCapture, Policy));
		Fdemo_mapShanmenSpiritEvasionMovementPlan MovementPlan;
		check(Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
			Request, Policy, MovementPlan));

		Fdemo_mapShanmenSpiritEvasionTrajectoryCapture TrajectoryCapture;
		TrajectoryCapture.DurationSeconds = 0.4f;
		TrajectoryCapture.SegmentCount = 4;
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
		check(Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			TrajectoryCapture, Trajectory));
		check(Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
			MovementPlan, Trajectory, Fixture.MotionPlan));

		Fixture.Preflight.Status =
			Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready;
		Fixture.Preflight.PlanId = MovementPlan.GetPlanId();
		Fixture.Preflight.Displacement.RequestedDistance = 400.0f;
		Fixture.Preflight.Displacement.ResolvedDistance = 400.0f;
		Fixture.Preflight.Displacement.bBlocked = false;
		return Fixture;
	}

	double CoordinatorScheduledTime(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& Plan,
		int32 SegmentOrdinal)
	{
		return static_cast<double>(Plan.GetTrajectory().GetDurationSeconds())
			* static_cast<double>(SegmentOrdinal)
			/ static_cast<double>(Plan.GetTrajectory().GetSegmentCount());
	}

	Fdemo_mapShanmenSpiritEvasionActionCoordinator OpenCoordinator(
		const FCoordinatorFixture& Fixture)
	{
		Fdemo_mapShanmenSpiritEvasionActionCoordinator Coordinator;
		check(Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryOpen(
			Fixture.MotionPlan,
			Fixture.Preflight,
			Fixture.Window,
			Fixture.ActionRuntime,
			Coordinator));
		return Coordinator;
	}

	class FFakeExecutionPort final
		: public Idemo_mapShanmenSpiritEvasionSegmentExecutionPort
	{
	public:
		enum class EMode : uint8
		{
			Commit,
			Block,
			CharacterUnavailable,
			MovementUnavailable
		};

		explicit FFakeExecutionPort(EMode InMode = EMode::Commit)
			: Mode(InMode)
		{
		}

		virtual Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execute(
			const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command) override
		{
			++ExecutionCount;
			LastCommandId = Command.GetCommandId();
			Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Result;
			Result.CommandId = Command.GetCommandId();
			if (Mode == EMode::CharacterUnavailable)
			{
				Result.Status =
					Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
					CharacterUnavailable;
				return Result;
			}
			if (Mode == EMode::MovementUnavailable)
			{
				Result.Status =
					Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
					MovementUnavailable;
				return Result;
			}

			Result.Displacement.RequestedDistance =
				Command.GetRequestedDistance();
			Result.Displacement.ResolvedDistance = Mode == EMode::Block
				? Command.GetRequestedDistance() * 0.4f
				: Command.GetRequestedDistance();
			Result.Displacement.bBlocked = Mode == EMode::Block;
			check(Fdemo_mapShanmenSpiritEvasionSegmentReceipt::TryCapture(
				Command, Result.Displacement, Result.Receipt));
			Result.Status = Mode == EMode::Block
				? Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Blocked
				: Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Committed;
			return Result;
		}

		EMode Mode = EMode::Commit;
		int32 ExecutionCount = 0;
		FGuid LastCommandId;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorBindingTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.BindingAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorBindingTest::RunTest(
	const FString&)
{
	const FCoordinatorFixture Fixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator First =
		OpenCoordinator(Fixture);
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Replay =
		OpenCoordinator(Fixture);
	TestTrue(TEXT("Exact action window and motion session open one owner"),
		First.IsValid() && First.IsActive()
			&& First.GetCoordinatorId().IsValid()
			&& Replay.GetCoordinatorId() == First.GetCoordinatorId());

	const FCoordinatorFixture Foreign = MakeCoordinatorFixture(
		1402, TEXT("TEST-DIGEST-P10.4-FOREIGN"));
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Rejected;
	TestFalse(TEXT("Foreign action window cannot own this motion plan"),
		Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryOpen(
			Fixture.MotionPlan,
			Fixture.Preflight,
			Foreign.Window,
			Foreign.ActionRuntime,
			Rejected));
	Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Mismatch =
		Fixture.Preflight;
	Mismatch.PlanId = FGuid(1, 2, 3, 4);
	TestFalse(TEXT("Foreign preflight cannot open a coordinator"),
		Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryOpen(
			Fixture.MotionPlan,
			Mismatch,
			Fixture.Window,
			Fixture.ActionRuntime,
			Rejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorDriveTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.WaitAndCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorDriveTest::RunTest(
	const FString&)
{
	const FCoordinatorFixture Fixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Coordinator =
		OpenCoordinator(Fixture);
	FFakeExecutionPort Port;
	const double FirstDue = CoordinatorScheduledTime(Fixture.MotionPlan, 1);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Waiting =
		Coordinator.TryAdvance(
			Fixture.ActionRuntime, FirstDue - 0.001, Port);
	TestTrue(TEXT("Before the first boundary the owner waits without execution"),
		Waiting.IsSuccess()
			&& Waiting.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Waiting
			&& Port.ExecutionCount == 0);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Committed =
		Coordinator.TryAdvance(Fixture.ActionRuntime, FirstDue, Port);
	TestTrue(TEXT("At the boundary one command executes and commits"),
		Committed.IsSuccess()
			&& Committed.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::
				SegmentCommitted
			&& Port.ExecutionCount == 1
			&& Port.LastCommandId == Committed.Command.GetCommandId()
			&& Coordinator.GetMotionSession().GetAcceptedSegmentCount() == 1
			&& !Coordinator.GetMotionSession().HasPendingCommand());
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult SameSample =
		Coordinator.TryAdvance(Fixture.ActionRuntime, FirstDue, Port);
	TestTrue(TEXT("Same elapsed sample cannot execute the next segment early"),
		SameSample.Status
			== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Waiting
			&& Port.ExecutionCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorCompletionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.Completion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorCompletionTest::RunTest(
	const FString&)
{
	const FCoordinatorFixture Fixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Coordinator =
		OpenCoordinator(Fixture);
	FFakeExecutionPort Port;
	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Result;
	for (int32 Ordinal = 1; Ordinal <= 4; ++Ordinal)
	{
		Result = Coordinator.TryAdvance(
			Fixture.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, Ordinal),
			Port);
		TestTrue(FString::Printf(TEXT("Coordinator accepts segment %d"), Ordinal),
			Result.IsSuccess());
	}
	TestTrue(TEXT("Fourth receipt completes the exact motion target"),
		Result.Status
			== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Completed
			&& Coordinator.IsValid() && !Coordinator.IsActive()
			&& Coordinator.GetMotionSession().GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Completed
			&& Coordinator.GetMotionSession().GetAcceptedSegmentCount() == 4
			&& Coordinator.GetMotionSession().GetResolvedDistance() == 400.0f
			&& Port.ExecutionCount == 4);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Extra =
		Coordinator.TryAdvance(
			Fixture.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, 4),
			Port);
	TestTrue(TEXT("Completed owner rejects further execution"),
		Extra.IsValid() && !Extra.IsSuccess()
			&& Extra.Error
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
				CoordinatorNotReady
			&& Port.ExecutionCount == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorBlockedTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.Blocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorBlockedTest::RunTest(
	const FString&)
{
	const FCoordinatorFixture Fixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Coordinator =
		OpenCoordinator(Fixture);
	FFakeExecutionPort Port(FFakeExecutionPort::EMode::Block);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Blocked =
		Coordinator.TryAdvance(
			Fixture.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, 1),
			Port);
	TestTrue(TEXT("Blocked swept receipt closes motion after actual distance"),
		Blocked.IsSuccess()
			&& Blocked.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Blocked
			&& Blocked.Execution.Receipt.WasBlocked()
			&& Coordinator.GetMotionSession().GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Blocked
			&& Coordinator.GetMotionSession().GetResolvedDistance() == 40.0f);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Extra =
		Coordinator.TryAdvance(
			Fixture.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, 4),
			Port);
	TestTrue(TEXT("Blocked owner cannot execute again"),
		!Extra.IsSuccess() && Port.ExecutionCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorActionLifecycleTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.ActionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorActionLifecycleTest::RunTest(
	const FString&)
{
	FCoordinatorFixture EndedFixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Ended =
		OpenCoordinator(EndedFixture);
	FShanmenActionTransitionReceipt Transition;
	check(EndedFixture.ActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Active, Transition));
	FFakeExecutionPort EndedPort;
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult EndedResult =
		Ended.TryAdvance(
			EndedFixture.ActionRuntime,
			CoordinatorScheduledTime(EndedFixture.MotionPlan, 1),
			EndedPort);
	TestTrue(TEXT("Active to Recovery closes motion before another move"),
		EndedResult.IsSuccess()
			&& EndedResult.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Terminated
			&& EndedResult.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ActionEnded
			&& EndedPort.ExecutionCount == 0);

	FCoordinatorFixture InterruptedFixture = MakeCoordinatorFixture(
		1403, TEXT("TEST-DIGEST-P10.4-INTERRUPTED"));
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Interrupted =
		OpenCoordinator(InterruptedFixture);
	check(InterruptedFixture.ActionRuntime.TryInterrupt(
		EShanmenCombatActionPhase::Active, Transition));
	FFakeExecutionPort InterruptedPort;
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult
		InterruptedResult = Interrupted.TryAdvance(
			InterruptedFixture.ActionRuntime,
			CoordinatorScheduledTime(InterruptedFixture.MotionPlan, 1),
			InterruptedPort);
	TestTrue(TEXT("Interrupted action closes motion before another move"),
		InterruptedResult.IsSuccess()
			&& InterruptedResult.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ActionInterrupted
			&& InterruptedPort.ExecutionCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorTerminationTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.ExplicitAndOwnerEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorTerminationTest::RunTest(
	const FString&)
{
	const FCoordinatorFixture Fixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Cancelled =
		OpenCoordinator(Fixture);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult CancelResult =
		Cancelled.TryTerminate(
			Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
			ExplicitCancel);
	TestTrue(TEXT("Explicit product cancellation emits one terminal proof"),
		CancelResult.IsSuccess()
			&& CancelResult.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExplicitCancel
			&& !Cancelled.IsActive());
	TestFalse(TEXT("Terminal owner rejects duplicate cancellation"),
		Cancelled.TryTerminate(
			Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
			ExplicitCancel).IsSuccess());

	Fdemo_mapShanmenSpiritEvasionActionCoordinator OwnerEnded =
		OpenCoordinator(Fixture);
	FFakeExecutionPort MissingOwner(
		FFakeExecutionPort::EMode::CharacterUnavailable);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult OwnerResult =
		OwnerEnded.TryAdvance(
			Fixture.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, 1),
			MissingOwner);
	TestTrue(TEXT("Unavailable owner abandons the issued command explicitly"),
		OwnerResult.IsSuccess()
			&& OwnerResult.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::OwnerEnded
			&& OwnerResult.Termination.GetPendingCommandId()
				== OwnerResult.Command.GetCommandId()
			&& !OwnerEnded.GetMotionSession().HasPendingCommand());

	Fdemo_mapShanmenSpiritEvasionActionCoordinator ExecutionEnded =
		OpenCoordinator(Fixture);
	FFakeExecutionPort MissingMovement(
		FFakeExecutionPort::EMode::MovementUnavailable);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult ExecutionResult =
		ExecutionEnded.TryAdvance(
			Fixture.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, 1),
			MissingMovement);
	TestTrue(TEXT("Unavailable movement authority leaves no pending command"),
		ExecutionResult.IsSuccess()
			&& ExecutionResult.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExecutionUnavailable
			&& !ExecutionEnded.GetMotionSession().HasPendingCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCoordinatorFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator.InputFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionCoordinatorFenceTest::RunTest(
	const FString&)
{
	const FCoordinatorFixture Fixture = MakeCoordinatorFixture();
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Coordinator =
		OpenCoordinator(Fixture);
	const FCoordinatorFixture Foreign = MakeCoordinatorFixture(
		1404, TEXT("TEST-DIGEST-P10.4-MISMATCH"));
	FFakeExecutionPort Port;
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Mismatch =
		Coordinator.TryAdvance(
			Foreign.ActionRuntime,
			CoordinatorScheduledTime(Fixture.MotionPlan, 1),
			Port);
	TestTrue(TEXT("Foreign action runtime is rejected without closing motion"),
		Mismatch.IsValid() && !Mismatch.IsSuccess()
			&& Mismatch.Error
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
				ActionMismatch
			&& Coordinator.IsActive() && Port.ExecutionCount == 0);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult InvalidElapsed =
		Coordinator.TryAdvance(Fixture.ActionRuntime, -1.0, Port);
	TestTrue(TEXT("Negative elapsed is rejected without execution"),
		!InvalidElapsed.IsSuccess()
			&& InvalidElapsed.Error
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
				InvalidElapsed
			&& Coordinator.GetMotionSession().GetLastElapsedSeconds() == 0.0
			&& Port.ExecutionCount == 0);
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Waiting =
		Coordinator.TryAdvance(Fixture.ActionRuntime, 0.05, Port);
	check(Waiting.IsSuccess());
	const Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Rewind =
		Coordinator.TryAdvance(Fixture.ActionRuntime, 0.04, Port);
	TestTrue(TEXT("Elapsed rewind is rejected and preserves the watermark"),
		!Rewind.IsSuccess()
			&& Coordinator.GetMotionSession().GetLastElapsedSeconds() == 0.05
			&& Port.ExecutionCount == 0);
	return true;
}

#endif
