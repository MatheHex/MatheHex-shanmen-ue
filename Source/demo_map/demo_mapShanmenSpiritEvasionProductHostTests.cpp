#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapShanmenSpiritEvasionProductHost.h"

#include <limits>

namespace
{
	const FGuid HostRunId(
		0xDF100001, 0xDF100002, 0xDF100003, 0xDF100004);
	const FGuid HostOwnerId(
		0xDF110001, 0xDF110002, 0xDF110003, 0xDF110004);
	const FGuid HostSourceId(
		0xDF120001, 0xDF120002, 0xDF120003, 0xDF120004);
	const FName HostPolicyId(
		TEXT("Movement.Spell.SpiritEvasion.GroundStep"));
	constexpr double HostStartTime = 100.0;

	FShanmenCombatActionSnapshot MakeHostAction(
		uint64 ActivationSequence = 1501,
		const FString& Digest = TEXT("TEST-DIGEST-P10.5"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = HostRunId;
		Capture.OwnerId = HostOwnerId;
		Capture.SourceEntityId = HostSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.5");
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

	FShanmenSpiritEvasionDefinition MakeHostDefinition()
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

	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot MakeHostPolicy()
	{
		Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture Capture;
		Capture.MovementPolicyId = HostPolicyId;
		Capture.RequestedDistance = 400.0f;
		Capture.MinimumResolvedDistance = 100.0f;
		Capture.WorldStaticClearance = 2.0f;
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
		check(Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
		return Policy;
	}

	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot MakeHostTrajectory()
	{
		Fdemo_mapShanmenSpiritEvasionTrajectoryCapture Capture;
		Capture.DurationSeconds = 0.4f;
		Capture.SegmentCount = 4;
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
		check(Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory));
		return Trajectory;
	}

	class FFakePreflightPort final
		: public Idemo_mapShanmenSpiritEvasionPreflightPort
	{
	public:
		enum class EMode : uint8
		{
			Ready,
			CharacterUnavailable,
			Insufficient,
			MismatchedPlan
		};

		explicit FFakePreflightPort(EMode InMode = EMode::Ready)
			: Mode(InMode)
		{
		}

		virtual Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Evaluate(
			const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan) override
		{
			++EvaluationCount;
			LastPlanId = Plan.GetPlanId();
			Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Result;
			Result.PlanId = Mode == EMode::MismatchedPlan
				? FGuid(1, 2, 3, 4)
				: Plan.GetPlanId();
			Result.Displacement.RequestedDistance =
				Plan.GetPolicy().GetRequestedDistance();
			if (Mode == EMode::CharacterUnavailable)
			{
				Result.Status =
					Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
					CharacterUnavailable;
				return Result;
			}
			if (Mode == EMode::Insufficient)
			{
				Result.Status =
					Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
					InsufficientResolvedDistance;
				Result.Displacement.ResolvedDistance = 50.0f;
				Result.Displacement.bBlocked = true;
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready;
			Result.Displacement.ResolvedDistance = 400.0f;
			Result.Displacement.bBlocked = false;
			return Result;
		}

		EMode Mode = EMode::Ready;
		int32 EvaluationCount = 0;
		FGuid LastPlanId;
	};

	class FFakeHostExecutionPort final
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

		explicit FFakeHostExecutionPort(EMode InMode = EMode::Commit)
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

	Fdemo_mapShanmenSpiritEvasionHostStartResult StartHost(
		Fdemo_mapShanmenSpiritEvasionProductHost& OutHost,
		FFakePreflightPort& Preflight,
		double StartTime = HostStartTime,
		uint64 ActivationSequence = 1501,
		const FString& Digest = TEXT("TEST-DIGEST-P10.5"))
	{
		return Fdemo_mapShanmenSpiritEvasionProductHost::TryStart(
			MakeHostAction(ActivationSequence, Digest),
			MakeHostDefinition(),
			MakeHostPolicy(),
			MakeHostTrajectory(),
			FVector(3.0, 4.0, 9.0),
			StartTime,
			Preflight,
			OutHost);
	}

	Fdemo_mapShanmenSpiritEvasionProductHost MakeHost(
		double StartTime = HostStartTime,
		uint64 ActivationSequence = 1501,
		const FString& Digest = TEXT("TEST-DIGEST-P10.5"))
	{
		FFakePreflightPort Preflight;
		Fdemo_mapShanmenSpiritEvasionProductHost Host;
		check(StartHost(
			Host, Preflight, StartTime, ActivationSequence, Digest).IsSuccess());
		return Host;
	}

	double SegmentTime(int32 Ordinal)
	{
		return HostStartTime + 0.1 * static_cast<double>(Ordinal);
	}

	Fdemo_mapShanmenSpiritEvasionHostStepResult DriveToRecovery(
		Fdemo_mapShanmenSpiritEvasionProductHost& Host,
		FFakeHostExecutionPort& Port)
	{
		Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
		for (int32 Ordinal = 1; Ordinal <= 4; ++Ordinal)
		{
			Result = Host.TryAdvance(SegmentTime(Ordinal), Port);
			check(Result.IsSuccess());
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostStartTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.StartAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostStartTest::RunTest(const FString&)
{
	FFakePreflightPort FirstPreflight;
	Fdemo_mapShanmenSpiritEvasionProductHost First;
	const Fdemo_mapShanmenSpiritEvasionHostStartResult FirstResult =
		StartHost(First, FirstPreflight);
	TestTrue(TEXT("Product host owns the exact committed action and motion chain"),
		FirstResult.IsSuccess()
			&& First.IsValid() && First.IsActive()
			&& FirstResult.HostId == First.GetHostId()
			&& FirstResult.Preflight.PlanId
				== First.GetCoordinator().GetMotionSession()
					.GetMotionPlan().GetMovementPlan().GetPlanId()
			&& FirstPreflight.EvaluationCount == 1);
	FShanmenSpiritEvasionProjectionReceipt FirstProjection;
	FShanmenSpiritEvasionProjectionReceipt ReplayedProjection;
	TestTrue(TEXT("active product host exposes the exact immutable defense projection"),
		First.TryProjectDefenseLayer(FirstProjection)
			&& First.TryProjectDefenseLayer(ReplayedProjection)
			&& FirstProjection.IsValid()
			&& FirstProjection.GetProjectionId()
				== ReplayedProjection.GetProjectionId()
			&& FirstProjection.GetWindow().GetAction().GetActivationId()
				== First.GetActionRuntime().GetAction().GetActivationId());

	FFakePreflightPort ReplayPreflight;
	Fdemo_mapShanmenSpiritEvasionProductHost Replay;
	const Fdemo_mapShanmenSpiritEvasionHostStartResult ReplayResult =
		StartHost(Replay, ReplayPreflight);
	TestEqual(TEXT("Equivalent activation and start sample reproduce HostId"),
		ReplayResult.HostId, FirstResult.HostId);

	FFakePreflightPort OtherTimePreflight;
	Fdemo_mapShanmenSpiritEvasionProductHost OtherTime;
	const Fdemo_mapShanmenSpiritEvasionHostStartResult OtherTimeResult =
		StartHost(OtherTime, OtherTimePreflight, HostStartTime + 1.0);
	TestTrue(TEXT("Absolute start sample participates in host identity"),
		OtherTimeResult.IsSuccess()
			&& OtherTimeResult.HostId != FirstResult.HostId);

	FFakePreflightPort Mismatch(
		FFakePreflightPort::EMode::MismatchedPlan);
	Fdemo_mapShanmenSpiritEvasionProductHost Rejected;
	const Fdemo_mapShanmenSpiritEvasionHostStartResult MismatchResult =
		StartHost(Rejected, Mismatch);
	TestTrue(TEXT("Foreign preflight proof fails closed"),
		MismatchResult.IsValid() && !MismatchResult.IsSuccess()
			&& MismatchResult.Error
				== Edemo_mapShanmenSpiritEvasionHostStartError::PreflightRejected
			&& !Rejected.IsValid());

	const Fdemo_mapShanmenSpiritEvasionHostStartResult MissingResult =
		Fdemo_mapShanmenSpiritEvasionProductHost::TryStartCharacter(
			MakeHostAction(),
			MakeHostDefinition(),
			MakeHostPolicy(),
			MakeHostTrajectory(),
			FVector(3.0, 4.0, 0.0),
			HostStartTime,
			nullptr,
			Rejected);
	TestTrue(TEXT("Production preflight adapter exposes unavailable owner"),
		!MissingResult.IsSuccess()
			&& MissingResult.Error
				== Edemo_mapShanmenSpiritEvasionHostStartError::PreflightRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostLifecycleTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.CompletionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostLifecycleTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSpiritEvasionProductHost Host = MakeHost();
	FFakeHostExecutionPort Port;
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Waiting =
		Host.TryAdvance(HostStartTime + 0.05, Port);
	TestTrue(TEXT("Absolute sample before the first segment waits"),
		Waiting.IsSuccess()
			&& Waiting.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::Waiting
			&& Host.GetLastObservedTimeSeconds() == HostStartTime + 0.05
			&& Port.ExecutionCount == 0);

	const Fdemo_mapShanmenSpiritEvasionHostStepResult Recovery =
		DriveToRecovery(Host, Port);
	TestTrue(TEXT("Final motion receipt advances the owned action to Recovery"),
		Recovery.IsSuccess()
			&& Recovery.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryCompleted
			&& Recovery.ActionTransition.GetFromPhase()
				== EShanmenCombatActionPhase::Active
			&& Host.IsRecovery() && !Host.IsTerminal()
			&& Port.ExecutionCount == 4);

	const Fdemo_mapShanmenSpiritEvasionHostStepResult Completed =
		Host.TryFinishRecovery();
	TestTrue(TEXT("Explicit recovery completion closes the action authority"),
		Completed.IsSuccess()
			&& Completed.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::Completed
			&& Host.IsTerminal()
			&& Host.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Replay =
		Host.TryFinishRecovery();
	TestTrue(TEXT("Terminal recovery signal is idempotent without new receipts"),
		Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::AlreadyTerminal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostBlockedTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.BlockedRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostBlockedTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSpiritEvasionProductHost Host = MakeHost();
	FFakeHostExecutionPort Port(FFakeHostExecutionPort::EMode::Block);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Result =
		Host.TryAdvance(SegmentTime(1), Port);
	TestTrue(TEXT("Blocked swept segment advances the action to Recovery"),
		Result.IsSuccess()
			&& Result.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryBlocked
			&& Host.IsRecovery()
			&& Host.GetCoordinator().GetMotionSession().GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Blocked
			&& Host.GetCoordinator().GetMotionSession().GetResolvedDistance()
				== 40.0f);
	TestTrue(TEXT("Blocked recovery can complete through the same action owner"),
		Host.TryFinishRecovery().IsSuccess() && Host.IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostCancellationTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.CancelAndInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostCancellationTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSpiritEvasionProductHost Cancelled = MakeHost();
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Cancel =
		Cancelled.TryCancel();
	TestTrue(TEXT("Explicit cancel closes motion and interrupts committed action"),
		Cancel.IsSuccess()
			&& Cancel.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::Cancelled
			&& Cancel.Motion.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExplicitCancel
			&& Cancelled.IsTerminal());
	TestTrue(TEXT("Duplicate cancel observes the existing terminal state"),
		Cancelled.TryCancel().Status
			== Edemo_mapShanmenSpiritEvasionHostStepStatus::AlreadyTerminal);

	Fdemo_mapShanmenSpiritEvasionProductHost Interrupted = MakeHost(
		HostStartTime, 1502, TEXT("TEST-DIGEST-P10.5-INTERRUPT"));
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Interrupt =
		Interrupted.TryInterrupt();
	TestTrue(TEXT("External interruption uses the action-interrupted motion proof"),
		Interrupt.IsSuccess()
			&& Interrupt.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::Interrupted
			&& Interrupt.Motion.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ActionInterrupted
			&& Interrupted.IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostOwnerEndTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.OwnerEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostOwnerEndTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSpiritEvasionProductHost Active = MakeHost();
	const Fdemo_mapShanmenSpiritEvasionHostStepResult ActiveEnd =
		Active.TryOwnerEnd();
	TestTrue(TEXT("Owner end while active leaves an owner termination receipt"),
		ActiveEnd.IsSuccess()
			&& ActiveEnd.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded
			&& ActiveEnd.Motion.Termination.GetReason()
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::OwnerEnded
			&& Active.IsTerminal());

	Fdemo_mapShanmenSpiritEvasionProductHost Recovery = MakeHost(
		HostStartTime, 1503, TEXT("TEST-DIGEST-P10.5-RECOVERY-END"));
	FFakeHostExecutionPort Port;
	check(DriveToRecovery(Recovery, Port).IsSuccess());
	const Fdemo_mapShanmenSpiritEvasionHostStepResult RecoveryEnd =
		Recovery.TryOwnerEnd();
	TestTrue(TEXT("Owner end during recovery interrupts action without rewriting motion"),
		RecoveryEnd.IsSuccess()
			&& RecoveryEnd.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded
			&& !RecoveryEnd.Motion.IsValid()
			&& RecoveryEnd.ActionTransition.GetFromPhase()
				== EShanmenCombatActionPhase::Recovery
			&& Recovery.GetCoordinator().GetMotionSession().GetState()
				== Edemo_mapShanmenSpiritEvasionMotionState::Completed
			&& Recovery.IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostExecutionFailureTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.ExecutionFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostExecutionFailureTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSpiritEvasionProductHost OwnerMissing = MakeHost();
	FFakeHostExecutionPort MissingOwner(
		FFakeHostExecutionPort::EMode::CharacterUnavailable);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult OwnerResult =
		OwnerMissing.TryAdvance(SegmentTime(1), MissingOwner);
	TestTrue(TEXT("Unavailable Character closes both motion and action"),
		OwnerResult.IsSuccess()
			&& OwnerResult.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded
			&& !OwnerMissing.GetCoordinator().GetMotionSession()
				.HasPendingCommand()
			&& OwnerMissing.IsTerminal());

	Fdemo_mapShanmenSpiritEvasionProductHost MovementMissing = MakeHost(
		HostStartTime, 1504, TEXT("TEST-DIGEST-P10.5-MOVEMENT-MISSING"));
	FFakeHostExecutionPort MissingMovement(
		FFakeHostExecutionPort::EMode::MovementUnavailable);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult MovementResult =
		MovementMissing.TryAdvance(SegmentTime(1), MissingMovement);
	TestTrue(TEXT("Unavailable movement authority is a distinct terminal result"),
		MovementResult.IsSuccess()
			&& MovementResult.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::
				ExecutionUnavailable
			&& MovementMissing.IsTerminal());

	Fdemo_mapShanmenSpiritEvasionProductHost ProductionMissing = MakeHost(
		HostStartTime, 1505, TEXT("TEST-DIGEST-P10.5-PRODUCTION-MISSING"));
	const Fdemo_mapShanmenSpiritEvasionHostStepResult ProductionResult =
		ProductionMissing.TryAdvanceCharacter(SegmentTime(1), nullptr);
	TestTrue(TEXT("Production execution adapter maps null owner without mutation"),
		ProductionResult.IsSuccess()
			&& ProductionResult.Status
				== Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded
			&& ProductionMissing.IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionHostTimeFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductHost.TimeFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionHostTimeFenceTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSpiritEvasionProductHost Host = MakeHost();
	FFakeHostExecutionPort Port;
	const Fdemo_mapShanmenSpiritEvasionHostStepResult BeforeStart =
		Host.TryAdvance(HostStartTime - 0.001, Port);
	TestTrue(TEXT("Sample before host start is rejected without execution"),
		BeforeStart.IsValid() && !BeforeStart.IsSuccess()
			&& BeforeStart.Error
				== Edemo_mapShanmenSpiritEvasionHostStepError::InvalidTime
			&& Port.ExecutionCount == 0);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Invalid =
		Host.TryAdvance(std::numeric_limits<double>::quiet_NaN(), Port);
	TestTrue(TEXT("Non-finite absolute time is rejected"),
		!Invalid.IsSuccess()
			&& Invalid.Error
				== Edemo_mapShanmenSpiritEvasionHostStepError::InvalidTime);
	check(Host.TryAdvance(HostStartTime + 0.05, Port).IsSuccess());
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Rewind =
		Host.TryAdvance(HostStartTime + 0.04, Port);
	TestTrue(TEXT("Absolute-time rewind preserves the accepted watermark"),
		!Rewind.IsSuccess()
			&& Rewind.Error
				== Edemo_mapShanmenSpiritEvasionHostStepError::InvalidTime
			&& Host.GetLastObservedTimeSeconds() == HostStartTime + 0.05
			&& Port.ExecutionCount == 0);
	return true;
}

#endif
