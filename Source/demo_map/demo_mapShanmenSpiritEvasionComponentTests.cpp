#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapShanmenSpiritEvasionComponent.h"

namespace
{
	const FGuid ComponentRunId(
		0xE0100001, 0xE0100002, 0xE0100003, 0xE0100004);
	const FGuid ComponentOwnerId(
		0xE0110001, 0xE0110002, 0xE0110003, 0xE0110004);
	const FGuid ComponentSourceId(
		0xE0120001, 0xE0120002, 0xE0120003, 0xE0120004);
	constexpr double ComponentStartTime = 300.0;

	FShanmenCombatActionSnapshot MakeComponentAction(
		uint64 ActivationSequence = 1601,
		const FString& Digest = TEXT("TEST-DIGEST-P10.6"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ComponentRunId;
		Capture.OwnerId = ComponentOwnerId;
		Capture.SourceEntityId = ComponentSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.6");
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

	FShanmenSpiritEvasionDefinition MakeComponentDefinition()
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

	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot MakeComponentPolicy()
	{
		Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture Capture;
		Capture.MovementPolicyId =
			TEXT("Movement.Spell.SpiritEvasion.GroundStep");
		Capture.RequestedDistance = 400.0f;
		Capture.MinimumResolvedDistance = 100.0f;
		Capture.WorldStaticClearance = 2.0f;
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
		check(Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			Capture, Policy));
		return Policy;
	}

	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot MakeComponentTrajectory()
	{
		Fdemo_mapShanmenSpiritEvasionTrajectoryCapture Capture;
		Capture.DurationSeconds = 0.4f;
		Capture.SegmentCount = 4;
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
		check(Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory));
		return Trajectory;
	}

	class FComponentPreflightPort final
		: public Idemo_mapShanmenSpiritEvasionPreflightPort
	{
	public:
		explicit FComponentPreflightPort(bool bInReady = true)
			: bReady(bInReady)
		{
		}

		virtual Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Evaluate(
			const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan) override
		{
			++EvaluationCount;
			Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Result;
			Result.PlanId = Plan.GetPlanId();
			Result.Displacement.RequestedDistance =
				Plan.GetPolicy().GetRequestedDistance();
			Result.Displacement.ResolvedDistance = bReady ? 400.0f : 50.0f;
			Result.Displacement.bBlocked = !bReady;
			Result.Status = bReady
				? Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready
				: Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
					InsufficientResolvedDistance;
			return Result;
		}

		bool bReady = true;
		int32 EvaluationCount = 0;
	};

	class FComponentExecutionPort final
		: public Idemo_mapShanmenSpiritEvasionSegmentExecutionPort
	{
	public:
		enum class EMode : uint8
		{
			Commit,
			Block,
			MovementUnavailable
		};

		explicit FComponentExecutionPort(EMode InMode = EMode::Commit)
			: Mode(InMode)
		{
		}

		virtual Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execute(
			const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command) override
		{
			++ExecutionCount;
			Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Result;
			Result.CommandId = Command.GetCommandId();
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
				? Command.GetRequestedDistance() * 0.25f
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
	};

	Udemo_mapShanmenSpiritEvasionComponent* MakeComponent()
	{
		return NewObject<Udemo_mapShanmenSpiritEvasionComponent>();
	}

	Fdemo_mapShanmenSpiritEvasionComponentStartResult StartComponent(
		Udemo_mapShanmenSpiritEvasionComponent& Component,
		FComponentPreflightPort& Preflight,
		double StartTime = ComponentStartTime,
		uint64 ActivationSequence = 1601,
		const FString& Digest = TEXT("TEST-DIGEST-P10.6"))
	{
		return Component.TryStartAtForAutomation(
			MakeComponentAction(ActivationSequence, Digest),
			MakeComponentDefinition(),
			MakeComponentPolicy(),
			MakeComponentTrajectory(),
			FVector(3.0, 4.0, 8.0),
			StartTime,
			Preflight);
	}

	double SegmentTime(int32 Ordinal)
	{
		return ComponentStartTime + 0.1 * static_cast<double>(Ordinal);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentOwnerFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.OwnerFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentOwnerFenceTest::RunTest(
	const FString&)
{
	Udemo_mapShanmenSpiritEvasionComponent* Component = MakeComponent();
	TestNotNull(TEXT("component exists"), Component);
	TestTrue(TEXT("component can ever tick"),
		Component->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("component starts without tick"),
		Component->PrimaryComponentTick.bStartWithTickEnabled);
	const Fdemo_mapShanmenSpiritEvasionComponentStartResult Result =
		Component->TryStart(
			MakeComponentAction(),
			MakeComponentDefinition(),
			MakeComponentPolicy(),
			MakeComponentTrajectory(),
			FVector::ForwardVector);
	TestTrue(TEXT("owner rejection is a valid result"), Result.IsValid());
	TestEqual(TEXT("ownerless component rejects explicitly"),
		Result.Error,
		Edemo_mapShanmenSpiritEvasionComponentStartError::OwnerUnavailable);
	TestFalse(TEXT("owner rejection publishes no host"), Component->HasHost());
	TestFalse(TEXT("owner rejection requires no tick"),
		Component->RequiresExecutionTick());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentStartTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.StartAndBusyFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentStartTest::RunTest(const FString&)
{
	Udemo_mapShanmenSpiritEvasionComponent* Component = MakeComponent();
	FComponentPreflightPort Preflight;
	const Fdemo_mapShanmenSpiritEvasionComponentStartResult Start =
		StartComponent(*Component, Preflight);
	TestTrue(TEXT("component start succeeds"), Start.IsSuccess());
	TestTrue(TEXT("component owns active host"),
		Component->HasHost() && Component->IsActive());
	TestTrue(TEXT("active host requires execution tick"),
		Component->RequiresExecutionTick());
	const FGuid HostId = Component->GetHost().GetHostId();

	FComponentPreflightPort SecondPreflight;
	const Fdemo_mapShanmenSpiritEvasionComponentStartResult Busy =
		StartComponent(
			*Component,
			SecondPreflight,
			ComponentStartTime + 1.0,
			1602,
			TEXT("BUSY"));
	TestTrue(TEXT("busy rejection is valid"), Busy.IsValid());
	TestEqual(TEXT("busy component rejects second action"),
		Busy.Error,
		Edemo_mapShanmenSpiritEvasionComponentStartError::ComponentBusy);
	TestEqual(TEXT("busy rejection does not run preflight"),
		SecondPreflight.EvaluationCount,
		0);
	TestEqual(TEXT("busy rejection preserves host"),
		Component->GetHost().GetHostId(),
		HostId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentLifecycleTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.CompletionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentLifecycleTest::RunTest(
	const FString&)
{
	Udemo_mapShanmenSpiritEvasionComponent* Component = MakeComponent();
	FComponentPreflightPort Preflight;
	TestTrue(TEXT("start succeeds"),
		StartComponent(*Component, Preflight).IsSuccess());
	FComponentExecutionPort Execution;
	Fdemo_mapShanmenSpiritEvasionHostStepResult Step;
	for (int32 Ordinal = 1; Ordinal <= 4; ++Ordinal)
	{
		Step = Component->TryAdvanceAtForAutomation(
			SegmentTime(Ordinal), Execution);
		TestTrue(TEXT("segment step succeeds"), Step.IsSuccess());
	}
	TestEqual(TEXT("fourth segment enters recovery"),
		Step.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryCompleted);
	TestTrue(TEXT("component exposes recovery"), Component->IsRecovery());
	TestFalse(TEXT("recovery disables execution tick requirement"),
		Component->RequiresExecutionTick());
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Finished =
		Component->TryFinishRecovery();
	TestEqual(TEXT("explicit finish completes action"),
		Finished.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Completed);
	TestTrue(TEXT("component becomes terminal"), Component->IsTerminal());
	TestEqual(TEXT("last step remains observable"),
		Component->GetLastStep().Status,
		Finished.Status);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentBlockedRestartTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.BlockedRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentBlockedRestartTest::RunTest(
	const FString&)
{
	Udemo_mapShanmenSpiritEvasionComponent* Component = MakeComponent();
	FComponentPreflightPort Preflight;
	TestTrue(TEXT("start succeeds"),
		StartComponent(*Component, Preflight).IsSuccess());
	const FGuid FirstHostId = Component->GetHost().GetHostId();
	FComponentExecutionPort Blocked(FComponentExecutionPort::EMode::Block);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Block =
		Component->TryAdvanceAtForAutomation(SegmentTime(1), Blocked);
	TestEqual(TEXT("blocked movement enters recovery"),
		Block.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryBlocked);
	TestTrue(TEXT("blocked recovery finishes"),
		Component->TryFinishRecovery().IsSuccess());
	TestTrue(TEXT("terminal component can restart"), Component->CanStart());

	FComponentPreflightPort RestartPreflight;
	const Fdemo_mapShanmenSpiritEvasionComponentStartResult Restart =
		StartComponent(
			*Component,
			RestartPreflight,
			ComponentStartTime + 10.0,
			1602,
			TEXT("RESTART"));
	TestTrue(TEXT("restart succeeds"), Restart.IsSuccess());
	TestTrue(TEXT("restart publishes a new host identity"),
		Component->GetHost().GetHostId() != FirstHostId);
	TestTrue(TEXT("restart is active"), Component->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentSignalsTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.CancellationSignals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentSignalsTest::RunTest(
	const FString&)
{
	FComponentPreflightPort CancelPreflight;
	Udemo_mapShanmenSpiritEvasionComponent* Cancelled = MakeComponent();
	check(StartComponent(*Cancelled, CancelPreflight).IsSuccess());
	TestEqual(TEXT("cancel routes to host"),
		Cancelled->TryCancel().Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Cancelled);
	TestTrue(TEXT("cancel is terminal"), Cancelled->IsTerminal());

	FComponentPreflightPort InterruptPreflight;
	Udemo_mapShanmenSpiritEvasionComponent* Interrupted = MakeComponent();
	check(StartComponent(
		*Interrupted,
		InterruptPreflight,
		ComponentStartTime,
		1602,
		TEXT("INTERRUPT")).IsSuccess());
	TestEqual(TEXT("interrupt routes to host"),
		Interrupted->TryInterrupt().Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Interrupted);
	TestTrue(TEXT("interrupt is terminal"), Interrupted->IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentOwnerEndTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.OwnerTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentOwnerEndTest::RunTest(
	const FString&)
{
	FComponentPreflightPort ActivePreflight;
	Udemo_mapShanmenSpiritEvasionComponent* Active = MakeComponent();
	check(StartComponent(*Active, ActivePreflight).IsSuccess());
	TestEqual(TEXT("active owner end routes to host"),
		Active->TryOwnerEnd().Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded);
	TestTrue(TEXT("active owner end is terminal"), Active->IsTerminal());

	FComponentPreflightPort RecoveryPreflight;
	Udemo_mapShanmenSpiritEvasionComponent* Recovery = MakeComponent();
	check(StartComponent(
		*Recovery,
		RecoveryPreflight,
		ComponentStartTime,
		1602,
		TEXT("RECOVERY-END")).IsSuccess());
	FComponentExecutionPort Blocked(FComponentExecutionPort::EMode::Block);
	check(Recovery->TryAdvanceAtForAutomation(
		SegmentTime(1), Blocked).IsSuccess());
	TestEqual(TEXT("recovery owner end routes to host"),
		Recovery->TryOwnerEnd().Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded);
	TestTrue(TEXT("recovery owner end is terminal"), Recovery->IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionComponentFailureTest,
	"Shanmen.0_0_10.Product.SpiritEvasionComponent.FailureAndTimeFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenSpiritEvasionComponentFailureTest::RunTest(
	const FString&)
{
	Udemo_mapShanmenSpiritEvasionComponent* Rejected = MakeComponent();
	FComponentPreflightPort Insufficient(false);
	const Fdemo_mapShanmenSpiritEvasionComponentStartResult RejectedStart =
		StartComponent(*Rejected, Insufficient);
	TestTrue(TEXT("host rejection is valid"), RejectedStart.IsValid());
	TestEqual(TEXT("preflight failure is host rejection"),
		RejectedStart.Error,
		Edemo_mapShanmenSpiritEvasionComponentStartError::HostRejected);
	TestFalse(TEXT("rejected start publishes no host"), Rejected->HasHost());

	Udemo_mapShanmenSpiritEvasionComponent* Active = MakeComponent();
	FComponentPreflightPort Ready;
	check(StartComponent(*Active, Ready).IsSuccess());
	FComponentExecutionPort Execution;
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Rewind =
		Active->TryAdvanceAtForAutomation(
			ComponentStartTime - 0.01, Execution);
	TestEqual(TEXT("rewind is rejected"),
		Rewind.Error,
		Edemo_mapShanmenSpiritEvasionHostStepError::InvalidTime);
	TestTrue(TEXT("rewind preserves active host"), Active->IsActive());

	FComponentExecutionPort Missing(
		FComponentExecutionPort::EMode::MovementUnavailable);
	const Fdemo_mapShanmenSpiritEvasionHostStepResult Failure =
		Active->TryAdvanceAtForAutomation(SegmentTime(1), Missing);
	TestEqual(TEXT("execution failure is explicit"),
		Failure.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::ExecutionUnavailable);
	TestTrue(TEXT("execution failure is terminal"), Active->IsTerminal());
	TestFalse(TEXT("terminal host requires no tick"),
		Active->RequiresExecutionTick());
	return true;
}

#endif
