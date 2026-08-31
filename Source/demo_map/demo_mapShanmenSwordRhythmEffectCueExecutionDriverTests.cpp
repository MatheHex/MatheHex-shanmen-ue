#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionDriver.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueDriverFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid DriverRun(
		0xA9000001, 0xA9000002, 0xA9000003, 0xA9000004);
	const FGuid ForeignRun(
		0xA9000011, 0xA9000012, 0xA9000013, 0xA9000014);
	const FGuid DriverWeapon(
		0xA9010001, 0xA9010002, 0xA9010003, 0xA9010004);
	const FGuid ForeignWeapon(
		0xA9010011, 0xA9010012, 0xA9010013, 0xA9010014);
	const FGuid VisualConsumer(
		0xA9020001, 0xA9020002, 0xA9020003, 0xA9020004);
	const FGuid AudioConsumer(
		0xA9030001, 0xA9030002, 0xA9030003, 0xA9030004);

	bool TryMakeCoordinator(
		const FGuid& RunId,
		const FGuid& ConsumerId,
		const FName RoleId,
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& Out)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Scope;
		return Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				RunId, ConsumerId, RoleId, Scope)
			&& Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::TryCreate(
				Scope, Out);
	}

	struct FSwordRhythmCueDriverFixture
	{
		FGuid RunId;
		FGuid WeaponId;
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueDriverFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueDriverPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueDriverPlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(RunId, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(RunId, Diagnostic)
				&& Session.TryBegin(RunId, Diagnostic);
		}

		bool TryAdvanceTicks(const int64 RequestedTicks)
		{
			int64 AdvancedTicks = 0;
			const double Seconds = static_cast<double>(RequestedTicks)
				/ static_cast<double>(
					Fdemo_mapShanmenCombatRunFixedTimeline::
						CanonicalTicksPerSecond());
			return Timeline.TryAdvance(Seconds, AdvancedTicks, Diagnostic)
				&& AdvancedTicks == RequestedTicks;
		}

		bool TryExecute(Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				WeaponId, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			if (!Action.IsExecuted()
				|| !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic))
			{
				return false;
			}
			const auto Adapted =
				Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
					Session.GetPresentationState(),
					Session.GetConfig().GetEffectCuePolicy());
			if (!Adapted.IsAdapted())
			{
				Diagnostic = Adapted.Diagnostic;
				return false;
			}
			OutEvent = Adapted.Event;
			return true;
		}

		bool TryMakeEffectEvent(
			Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
		{
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Discarded;
			return TryExecute(Discarded)
				&& TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				&& TryExecute(Discarded)
				&& TryExecute(OutEvent)
				&& OutEvent.IsValid()
				&& OutEvent.NumCommands() == 2;
		}
	};

	enum class EDriverExecutorMode : uint8
	{
		Succeeded,
		RetryableFailure,
		Rejected
	};

	class FDriverFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		EDriverExecutorMode Mode = EDriverExecutorMode::Succeeded;
		int32 InvocationCount = 0;
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation LastInvocation;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			LastInvocation = Invocation;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == EDriverExecutorMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Driver fake executor rejected the batch.");
				return Result;
			}
			const auto Outcome = Mode == EDriverExecutorMode::RetryableFailure
				? Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure
				: Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded;
			if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
					Invocation,
					FGuid(0xA9040000 + InvocationCount, 0, 0, 1),
					Outcome,
					Result.Receipt))
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Driver fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT("Driver fake returned opaque evidence.");
			return Result;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDriverSuccessReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver.SuccessReplay",
	SwordRhythmCueDriverFlags)

bool Fdemo_mapSwordRhythmCueDriverSuccessReplayTest::RunTest(const FString&)
{
	FSwordRhythmCueDriverFixture Fixture(DriverRun, DriverWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("real source event and visual consumer are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeCoordinator(
				DriverRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	FDriverFakeExecutor Executor;
	const FGuid Attempt(0xA9050001, 0, 0, 1);
	const auto Executed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, Event, Attempt, Executor);
	const auto Replayed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, Event, Attempt, Executor);
	TestTrue(TEXT("driver prepares executes and acknowledges once"),
		Executed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed
			&& Executed.IsSuccess()
			&& Executed.IsAcknowledged()
			&& Executed.Preparation.IsRouted()
			&& Executor.InvocationCount == 1);
	TestTrue(TEXT("acknowledged exact attempt replays without executor"),
		Replayed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed
			&& Replayed.IsSuccess()
			&& Replayed.IsAcknowledged()
			&& Replayed.Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
					AlreadyAcknowledged
			&& !Replayed.Execution.bExecutorInvoked
			&& Executor.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDriverRetryOrderingTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver.RetryOrdering",
	SwordRhythmCueDriverFlags)

bool Fdemo_mapSwordRhythmCueDriverRetryOrderingTest::RunTest(const FString&)
{
	FSwordRhythmCueDriverFixture Fixture(DriverRun, DriverWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent FirstEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("effect event and consumer are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(FirstEvent)
			&& TryMakeCoordinator(
				DriverRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	FDriverFakeExecutor Executor;
	Executor.Mode = EDriverExecutorMode::RetryableFailure;
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			FirstEvent,
			FGuid(0xA9050011, 0, 0, 1),
			Executor);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent NextEvent;
	TestTrue(TEXT("retry remains pending and newer event is available"),
		Retry.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::RetryRecorded
			&& Retry.IsSuccess()
			&& !Retry.IsAcknowledged()
			&& Consumer.IsPending()
			&& Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecute(NextEvent));
	Executor.Mode = EDriverExecutorMode::Succeeded;
	const FGuid NextAttempt(0xA9050012, 0, 0, 1);
	const auto Blocked =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, NextEvent, NextAttempt, Executor);
	TestTrue(TEXT("pending earlier event blocks newer executor invocation"),
		Blocked.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					ExecutionRejected
			&& Blocked.Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RoutePending
			&& Executor.InvocationCount == 1);
	const auto FirstSuccess =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			FirstEvent,
			FGuid(0xA9050013, 0, 0, 1),
			Executor);
	const int32 CountBeforeNext = Executor.InvocationCount;
	const auto NextSuccess =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, NextEvent, NextAttempt, Executor);
	TestTrue(TEXT("old retry resolves before newer event advances"),
		FirstSuccess.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed
			&& FirstSuccess.IsAcknowledged()
			&& NextSuccess.IsSuccess()
			&& NextSuccess.IsAcknowledged()
			&& Executor.InvocationCount
				== CountBeforeNext
					+ (NextSuccess.Execution.bNoOp ? 0 : 1));
	const int32 CountAfterNext = Executor.InvocationCount;
	const auto Stale =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			FirstEvent,
			FGuid(0xA9050014, 0, 0, 1),
			Executor);
	TestTrue(TEXT("older event is stale before another executor call"),
		Stale.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					PreparationRejected
			&& Stale.Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::StaleEvent
			&& Executor.InvocationCount == CountAfterNext);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDriverNoOpTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver.NoOp",
	SwordRhythmCueDriverFlags)

bool Fdemo_mapSwordRhythmCueDriverNoOpTest::RunTest(const FString&)
{
	FSwordRhythmCueDriverFixture Fixture(DriverRun, DriverWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent EmptyEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("real zero-command event and consumer are ready"),
		Fixture.bReady
			&& Fixture.TryExecute(EmptyEvent)
			&& EmptyEvent.NumCommands() == 0
			&& TryMakeCoordinator(
				DriverRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	FDriverFakeExecutor Executor;
	Executor.Mode = EDriverExecutorMode::Rejected;
	const FGuid Attempt(0xA9050021, 0, 0, 1);
	const auto NoOp =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, EmptyEvent, Attempt, Executor);
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, EmptyEvent, Attempt, Executor);
	const auto Already =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			EmptyEvent,
			FGuid(0xA9050022, 0, 0, 1),
			Executor);
	TestTrue(TEXT("no-op executes and exact replay stays executor-free"),
		NoOp.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpExecuted
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpReplayed
			&& NoOp.IsAcknowledged()
			&& Replay.IsAcknowledged()
			&& Executor.InvocationCount == 0);
	TestTrue(TEXT("new attempt observes already acknowledged event"),
		Already.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					AlreadyAcknowledged
			&& Already.IsSuccess()
			&& Already.IsAcknowledged()
			&& Already.ExistingAcknowledgement.IsValid()
			&& !Already.Execution.bExecutorInvoked
			&& Executor.InvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDriverConsumerIsolationTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver.ConsumerIsolation",
	SwordRhythmCueDriverFlags)

bool Fdemo_mapSwordRhythmCueDriverConsumerIsolationTest::RunTest(
	const FString&)
{
	FSwordRhythmCueDriverFixture Fixture(DriverRun, DriverWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Visual;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Audio;
	TestTrue(TEXT("dual-channel event and consumers are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeCoordinator(
				DriverRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Visual)
			&& TryMakeCoordinator(
				DriverRun,
				AudioConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					AudioConsumerRoleId(),
				Audio));
	FDriverFakeExecutor VisualExecutor;
	FDriverFakeExecutor AudioExecutor;
	const FGuid Attempt(0xA9050031, 0, 0, 1);
	const auto VisualResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Visual, Event, Attempt, VisualExecutor);
	const auto AudioResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Audio, Event, Attempt, AudioExecutor);
	TestTrue(TEXT("each consumer owns one isolated executor transaction"),
		VisualResult.IsAcknowledged()
			&& AudioResult.IsAcknowledged()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1
			&& VisualExecutor.LastInvocation.GetRoute().GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& AudioExecutor.LastInvocation.GetRoute().GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio
			&& VisualExecutor.LastInvocation.GetRoute().GetRouteId()
				!= AudioExecutor.LastInvocation.GetRoute().GetRouteId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDriverPreflightTeardownTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver.PreflightTeardown",
	SwordRhythmCueDriverFlags)

bool Fdemo_mapSwordRhythmCueDriverPreflightTeardownTest::RunTest(
	const FString&)
{
	FSwordRhythmCueDriverFixture Fixture(DriverRun, DriverWeapon);
	FSwordRhythmCueDriverFixture Foreign(ForeignRun, ForeignWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent ForeignEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Unsupported;
	TestTrue(TEXT("local foreign and unsupported fixtures are ready"),
		Fixture.bReady
			&& Foreign.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& Foreign.TryMakeEffectEvent(ForeignEvent)
			&& TryMakeCoordinator(
				DriverRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer)
			&& TryMakeCoordinator(
				DriverRun,
				AudioConsumer,
				TEXT("Presentation.Unsupported"),
				Unsupported));
	FDriverFakeExecutor Executor;
	const auto InvalidEvent =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			Fdemo_mapShanmenSwordRhythmEffectCueEvent(),
			FGuid(0xA9050041, 0, 0, 1),
			Executor);
	const auto InvalidAttempt =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer, Event, FGuid(), Executor);
	const auto CrossRun =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			ForeignEvent,
			FGuid(0xA9050042, 0, 0, 1),
			Executor);
	const auto UnsupportedRole =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Unsupported,
			Event,
			FGuid(0xA9050043, 0, 0, 1),
			Executor);
	TestTrue(TEXT("invalid scope and role reject before executor"),
		InvalidEvent.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::EventInvalid
			&& InvalidAttempt.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptInvalid
			&& CrossRun.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					PreparationRejected
			&& CrossRun.Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::RunMismatch
			&& UnsupportedRole.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					PreparationRejected
			&& UnsupportedRole.Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
					RoleUnsupported
			&& Executor.InvocationCount == 0);
	Executor.Mode = EDriverExecutorMode::Rejected;
	const auto Rejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			Event,
			FGuid(0xA9050044, 0, 0, 1),
			Executor);
	TestTrue(TEXT("executor rejection leaves event retryable"),
		Rejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::ExecutionRejected
			&& Rejected.Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					ExecutorRejected
			&& Consumer.IsEmpty()
			&& Executor.InvocationCount == 1);
	TestTrue(TEXT("source ProductSession teardown succeeds"),
		Fixture.Session.TryEnd(DriverRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty());
	Executor.Mode = EDriverExecutorMode::Succeeded;
	const auto AfterTeardown =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
			Consumer,
			Event,
			FGuid(0xA9050045, 0, 0, 1),
			Executor);
	TestTrue(TEXT("immutable event executes after source teardown"),
		AfterTeardown.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed
			&& AfterTeardown.IsAcknowledged()
			&& Executor.InvocationCount == 2);
	return true;
}

#endif
