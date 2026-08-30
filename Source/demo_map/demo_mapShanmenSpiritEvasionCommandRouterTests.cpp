#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSpiritEvasionCommandRouter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	const EAutomationTestFlags CommandRouterFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid CommandRunId(
		0xE0200001, 0xE0200002, 0xE0200003, 0xE0200004);
	const FGuid CommandOwnerId(
		0xE0210001, 0xE0210002, 0xE0210003, 0xE0210004);
	constexpr double CommandStartTime = 400.0;

	struct FCommandRouterFixture
	{
		UWorld* World = nullptr;
		ACharacter* Character = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapShanmenSpiritEvasionComponent* Component = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FCommandRouterFixture()
		{
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World || !GEngine)
			{
				return;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.SetCurrentWorld(World);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(false)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(false)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(false)
					.SetTransactional(false)
					.CreateFXSystem(false));
			Character = World->SpawnActor<ACharacter>();
			Health = Character
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Character, TEXT("SpiritEvasionCommandHealth"))
				: nullptr;
			if (Character && Health)
			{
				Character->AddInstanceComponent(Health);
				Health->RegisterComponent();
			}
			const Fdemo_mapShanmenSpiritEvasionInstallationResult Installation =
				Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
					Character);
			Component = Installation.Component;
			bReady = Character
				&& Health
				&& Health->IsRegistered()
				&& Installation.IsSuccess()
				&& Coordinator.TryBeginRun(
					CommandRunId, Character, Health, Diagnostic);
		}

		~FCommandRouterFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	FShanmenCombatActionSnapshot MakeCommandAction(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		uint64 ActivationSequence = 1701)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = CommandOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.7");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P10.7-COMMAND-ROUTER");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSpiritEvasionDefinition MakeCommandDefinition()
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

	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot MakeCommandPolicy()
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

	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot MakeCommandTrajectory()
	{
		Fdemo_mapShanmenSpiritEvasionTrajectoryCapture Capture;
		Capture.DurationSeconds = 0.2f;
		Capture.SegmentCount = 2;
		Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
		check(Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			Capture, Trajectory));
		return Trajectory;
	}

	Fdemo_mapShanmenSpiritEvasionCommand MakeStartCommand(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		uint64 ActivationSequence = 1701,
		const FGuid* RunOverride = nullptr,
		const FGuid* SourceOverride = nullptr)
	{
		const FGuid& RunId = RunOverride
			? *RunOverride
			: Coordinator.GetRunId();
		const FGuid& SourceId = SourceOverride
			? *SourceOverride
			: Coordinator.GetPlayerEntityId();
		Fdemo_mapShanmenSpiritEvasionCommand Command;
		check(Fdemo_mapShanmenSpiritEvasionCommand::TryCaptureStart(
			MakeCommandAction(RunId, SourceId, ActivationSequence),
			MakeCommandDefinition(),
			MakeCommandPolicy(),
			MakeCommandTrajectory(),
			FVector(3.0f, 4.0f, 0.0f),
			Command));
		return Command;
	}

	class FCommandPreflightPort final
		: public Idemo_mapShanmenSpiritEvasionPreflightPort
	{
	public:
		virtual Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Evaluate(
			const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan) override
		{
			++EvaluationCount;
			Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Result;
			Result.Status =
				Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready;
			Result.PlanId = Plan.GetPlanId();
			Result.Displacement.RequestedDistance =
				Plan.GetPolicy().GetRequestedDistance();
			Result.Displacement.ResolvedDistance =
				Plan.GetPolicy().GetRequestedDistance();
			return Result;
		}

		int32 EvaluationCount = 0;
	};

	class FCommandExecutionPort final
		: public Idemo_mapShanmenSpiritEvasionSegmentExecutionPort
	{
	public:
		virtual Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execute(
			const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command) override
		{
			Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Result;
			Result.Status =
				Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::Committed;
			Result.CommandId = Command.GetCommandId();
			Result.Displacement.RequestedDistance =
				Command.GetRequestedDistance();
			Result.Displacement.ResolvedDistance =
				Command.GetRequestedDistance();
			check(Fdemo_mapShanmenSpiritEvasionSegmentReceipt::TryCapture(
				Command, Result.Displacement, Result.Receipt));
			return Result;
		}
	};

	Fdemo_mapShanmenSpiritEvasionCommandResult StartFixture(
		FCommandRouterFixture& Fixture,
		FCommandPreflightPort& Preflight,
		uint64 ActivationSequence = 1701)
	{
		return Fdemo_mapShanmenSpiritEvasionCommandRouter::
			TryRouteAtForAutomation(
				Fixture.Component,
				Fixture.Coordinator,
				Fixture.Character,
				MakeStartCommand(Fixture.Coordinator, ActivationSequence),
				CommandStartTime,
				Preflight);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCommandCaptureTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.FrozenCommandCapture",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionCommandCaptureTest::RunTest(
	const FString& Parameters)
{
	const FGuid SourceId(0xE0220001, 0xE0220002, 0xE0220003, 0xE0220004);
	Fdemo_mapShanmenSpiritEvasionCommand Command;
	TestTrue(
		TEXT("start capture succeeds"),
		Fdemo_mapShanmenSpiritEvasionCommand::TryCaptureStart(
			MakeCommandAction(CommandRunId, SourceId),
			MakeCommandDefinition(),
			MakeCommandPolicy(),
			MakeCommandTrajectory(),
			FVector(3.0f, 4.0f, 0.0f),
			Command));
	TestTrue(TEXT("start is valid"), Command.IsValid());
	TestEqual(
		TEXT("direction is normalized exactly once"),
		Command.GetCandidateDirection(),
		FVector(0.6f, 0.8f, 0.0f));
	TestEqual(
		TEXT("activation identity is preserved"),
		Command.GetActivationId(),
		Command.GetAction().GetActivationId());

	Fdemo_mapShanmenSpiritEvasionCommand Rejected;
	TestFalse(
		TEXT("zero direction fails closed"),
		Fdemo_mapShanmenSpiritEvasionCommand::TryCaptureStart(
			MakeCommandAction(CommandRunId, SourceId),
			MakeCommandDefinition(),
			MakeCommandPolicy(),
			MakeCommandTrajectory(),
			FVector::ZeroVector,
			Rejected));
	TestTrue(
		TEXT("cancel signal is payload-free and valid"),
		Fdemo_mapShanmenSpiritEvasionCommand::MakeCancel().IsValid());
	TestTrue(
		TEXT("interrupt signal is payload-free and valid"),
		Fdemo_mapShanmenSpiritEvasionCommand::MakeInterrupt().IsValid());
	TestTrue(
		TEXT("finish signal is payload-free and valid"),
		Fdemo_mapShanmenSpiritEvasionCommand::MakeFinishRecovery().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionInstallationTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.UniqueInstallation",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionInstallationTest::RunTest(
	const FString& Parameters)
{
	const Fdemo_mapShanmenSpiritEvasionInstallationResult Invalid =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(nullptr);
	TestFalse(TEXT("null owner is rejected"), Invalid.IsSuccess());
	TestEqual(
		TEXT("null owner status"),
		Invalid.Status,
		Edemo_mapShanmenSpiritEvasionInstallationStatus::OwnerUnavailable);

	FCommandRouterFixture Fixture;
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	const Fdemo_mapShanmenSpiritEvasionInstallationResult Existing =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
			Fixture.Character);
	TestTrue(TEXT("second ensure is idempotent"), Existing.IsSuccess());
	TestEqual(
		TEXT("existing component is reused"),
		Existing.Component,
		Fixture.Component);
	TestEqual(TEXT("exactly one component exists"), Existing.ExistingComponentCount, 1);

	ACharacter* DuplicateOwner = Fixture.World
		? Fixture.World->SpawnActor<ACharacter>()
		: nullptr;
	for (int32 Index = 0; DuplicateOwner && Index < 2; ++Index)
	{
		Udemo_mapShanmenSpiritEvasionComponent* Duplicate =
			NewObject<Udemo_mapShanmenSpiritEvasionComponent>(
				DuplicateOwner,
				Index == 0
					? TEXT("DuplicateSpiritEvasionA")
					: TEXT("DuplicateSpiritEvasionB"));
		DuplicateOwner->AddInstanceComponent(Duplicate);
		Duplicate->RegisterComponent();
	}
	const Fdemo_mapShanmenSpiritEvasionInstallationResult Duplicate =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
			DuplicateOwner);
	TestFalse(TEXT("duplicates fail closed"), Duplicate.IsSuccess());
	TestEqual(
		TEXT("duplicate status"),
		Duplicate.Status,
		Edemo_mapShanmenSpiritEvasionInstallationStatus::DuplicateComponents);
	TestEqual(TEXT("duplicate count is observable"), Duplicate.ExistingComponentCount, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCommandAuthorityFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.RunAndSourceFences",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionCommandAuthorityFenceTest::RunTest(
	const FString& Parameters)
{
	FCommandRouterFixture Fixture;
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	FCommandPreflightPort Preflight;
	Fdemo_mapCombatRunCoordinator UnreadyCoordinator;
	const Fdemo_mapShanmenSpiritEvasionCommandResult Unready =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRouteAtForAutomation(
			Fixture.Component,
			UnreadyCoordinator,
			Fixture.Character,
			MakeStartCommand(Fixture.Coordinator),
			CommandStartTime,
			Preflight);
	TestEqual(
		TEXT("unready coordinator is rejected"),
		Unready.Status,
		Edemo_mapShanmenSpiritEvasionCommandStatus::CoordinatorNotReady);

	const FGuid ForeignRun(0xE0230001, 0, 0, 1);
	const Fdemo_mapShanmenSpiritEvasionCommandResult WrongRun =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRouteAtForAutomation(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			MakeStartCommand(
				Fixture.Coordinator, 1702, &ForeignRun, nullptr),
			CommandStartTime,
			Preflight);
	TestEqual(
		TEXT("foreign run is rejected"),
		WrongRun.Status,
		Edemo_mapShanmenSpiritEvasionCommandStatus::RunMismatch);

	const FGuid ForeignSource(0xE0240001, 0, 0, 1);
	const Fdemo_mapShanmenSpiritEvasionCommandResult WrongSource =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRouteAtForAutomation(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			MakeStartCommand(
				Fixture.Coordinator, 1703, nullptr, &ForeignSource),
			CommandStartTime,
			Preflight);
	TestEqual(
		TEXT("foreign source is rejected"),
		WrongSource.Status,
		Edemo_mapShanmenSpiritEvasionCommandStatus::SourceMismatch);
	TestEqual(TEXT("rejections perform no preflight"), Preflight.EvaluationCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCommandStartTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.StartAndBusyFence",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionCommandStartTest::RunTest(
	const FString& Parameters)
{
	FCommandRouterFixture Fixture;
	FCommandPreflightPort Preflight;
	const Fdemo_mapShanmenSpiritEvasionCommandResult Started =
		StartFixture(Fixture, Preflight);
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestTrue(TEXT("typed start is accepted"), Started.IsAccepted());
	TestTrue(TEXT("component is active"), Fixture.Component->IsActive());
	TestEqual(TEXT("one preflight occurred"), Preflight.EvaluationCount, 1);
	const Fdemo_mapShanmenSpiritEvasionCommandResult Repeated =
		StartFixture(Fixture, Preflight);
	TestEqual(
		TEXT("busy component rejects duplicate start"),
		Repeated.Status,
		Edemo_mapShanmenSpiritEvasionCommandStatus::ComponentRejected);
	TestEqual(TEXT("busy rejection performs no preflight"), Preflight.EvaluationCount, 1);
	TestEqual(
		TEXT("active host identity remains stable"),
		Fixture.Component->GetHost().GetHostId(),
		Started.Start.HostStart.HostId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCommandCancelTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.CancelAfterRunClose",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionCommandCancelTest::RunTest(
	const FString& Parameters)
{
	FCommandRouterFixture Fixture;
	FCommandPreflightPort Preflight;
	TestTrue(TEXT("start succeeds"), StartFixture(Fixture, Preflight).IsAccepted());
	Fixture.Coordinator.Reset();
	const Fdemo_mapShanmenSpiritEvasionCommandResult Cancelled =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			Fdemo_mapShanmenSpiritEvasionCommand::MakeCancel());
	TestTrue(TEXT("cancel remains available after Run close"), Cancelled.IsAccepted());
	TestEqual(
		TEXT("cancel reaches exact host signal"),
		Cancelled.Step.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Cancelled);
	TestTrue(TEXT("component is terminal"), Fixture.Component->IsTerminal());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCommandInterruptTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.InterruptAndOwnerFence",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionCommandInterruptTest::RunTest(
	const FString& Parameters)
{
	FCommandRouterFixture Fixture;
	FCommandPreflightPort Preflight;
	TestTrue(TEXT("start succeeds"), StartFixture(Fixture, Preflight).IsAccepted());
	const Fdemo_mapShanmenSpiritEvasionCommandResult Interrupted =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			Fdemo_mapShanmenSpiritEvasionCommand::MakeInterrupt());
	TestTrue(TEXT("interrupt is accepted"), Interrupted.IsAccepted());
	TestEqual(
		TEXT("interrupt reaches exact host signal"),
		Interrupted.Step.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Interrupted);

	ACharacter* ForeignOwner = Fixture.World
		? Fixture.World->SpawnActor<ACharacter>()
		: nullptr;
	const Fdemo_mapShanmenSpiritEvasionCommandResult Mismatch =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
			Fixture.Component,
			Fixture.Coordinator,
			ForeignOwner,
			Fdemo_mapShanmenSpiritEvasionCommand::MakeCancel());
	TestEqual(
		TEXT("foreign owner cannot route control"),
		Mismatch.Status,
		Edemo_mapShanmenSpiritEvasionCommandStatus::ComponentOwnerMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenSpiritEvasionCommandFinishRecoveryTest,
	"Shanmen.0_0_10.Product.SpiritEvasionCommandRouter.FinishRecovery",
	CommandRouterFlags)

bool Fdemo_mapShanmenSpiritEvasionCommandFinishRecoveryTest::RunTest(
	const FString& Parameters)
{
	FCommandRouterFixture Fixture;
	FCommandPreflightPort Preflight;
	TestTrue(TEXT("start succeeds"), StartFixture(Fixture, Preflight).IsAccepted());
	FCommandExecutionPort Execution;
	Fdemo_mapShanmenSpiritEvasionHostStepResult Step =
		Fixture.Component->TryAdvanceAtForAutomation(
			CommandStartTime + 0.1, Execution);
	TestTrue(TEXT("first segment commits"), Step.IsSuccess());
	Step = Fixture.Component->TryAdvanceAtForAutomation(
		CommandStartTime + 0.2, Execution);
	TestEqual(
		TEXT("motion completion enters recovery"),
		Step.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryCompleted);
	TestTrue(TEXT("component is in recovery"), Fixture.Component->IsRecovery());

	const Fdemo_mapShanmenSpiritEvasionCommandResult Finished =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			Fdemo_mapShanmenSpiritEvasionCommand::MakeFinishRecovery());
	TestTrue(TEXT("finish recovery is accepted"), Finished.IsAccepted());
	TestEqual(
		TEXT("finish reaches completed transition"),
		Finished.Step.Status,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Completed);
	TestTrue(TEXT("component is terminal"), Fixture.Component->IsTerminal());
	return true;
}

#endif
