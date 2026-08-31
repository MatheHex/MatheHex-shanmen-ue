#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueExecutorFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ExecutorRun(
		0xA8000001, 0xA8000002, 0xA8000003, 0xA8000004);
	const FGuid ForeignRun(
		0xA8000011, 0xA8000012, 0xA8000013, 0xA8000014);
	const FGuid ExecutorWeapon(
		0xA8010001, 0xA8010002, 0xA8010003, 0xA8010004);
	const FGuid ForeignWeapon(
		0xA8010011, 0xA8010012, 0xA8010013, 0xA8010014);
	const FGuid VisualConsumer(
		0xA8020001, 0xA8020002, 0xA8020003, 0xA8020004);
	const FGuid AudioConsumer(
		0xA8030001, 0xA8030002, 0xA8030003, 0xA8030004);

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

	struct FSwordRhythmCueExecutorFixture
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

		FSwordRhythmCueExecutorFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueExecutorPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueExecutorPlayerHealth"))
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

	enum class EFakeCueExecutorMode : uint8
	{
		Succeeded,
		RetryableFailure,
		Rejected,
		MismatchedEvidence
	};

	class FFakeCueExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		EFakeCueExecutorMode Mode = EFakeCueExecutorMode::Succeeded;
		int32 InvocationCount = 0;
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation LastInvocation;
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation
			EvidenceInvocationOverride;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			LastInvocation = Invocation;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == EFakeCueExecutorMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Injected executor rejected the batch.");
				return Result;
			}

			const auto& ReceiptInvocation =
				Mode == EFakeCueExecutorMode::MismatchedEvidence
					? EvidenceInvocationOverride
					: Invocation;
			const auto Outcome =
				Mode == EFakeCueExecutorMode::RetryableFailure
					? Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
						RetryableFailure
					: Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded;
			if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
					ReceiptInvocation,
					FGuid(0xA8040000 + InvocationCount, 0, 0, 1),
					Outcome,
					Result.Receipt))
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT("Injected executor returned opaque evidence.");
			return Result;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueExecutorBatchSuccessTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter.BatchSuccess",
	SwordRhythmCueExecutorFlags)

bool Fdemo_mapSwordRhythmCueExecutorBatchSuccessTest::RunTest(const FString&)
{
	FSwordRhythmCueExecutorFixture Fixture(ExecutorRun, ExecutorWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("real source event and visual consumer are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeCoordinator(
				ExecutorRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto Prepared = Consumer.Prepare(Event);
	FFakeCueExecutor Executor;
	const auto Executed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer,
			Prepared.Route,
			FGuid(0xA8050001, 0, 0, 1),
			Executor);
	TestTrue(TEXT("one complete route batch invokes once and acknowledges"),
		Prepared.IsRouted()
			&& Prepared.Route.NumCommands() == 1
			&& Executed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::Succeeded
			&& Executed.IsSuccess()
			&& Executed.bExecutorInvoked
			&& Executor.InvocationCount == 1);
	TestTrue(TEXT("invocation preserves the complete typed route batch"),
		Executor.LastInvocation.IsValid()
			&& Executor.LastInvocation.Matches(Executed.Invocation)
			&& Executor.LastInvocation.GetCommands().Num()
				== Prepared.Route.NumCommands()
			&& Executor.LastInvocation.GetCommands()[0].GetCommandId()
				== Prepared.Route.GetCommands()[0].GetCommandId());
	TestTrue(TEXT("successful batch is now acknowledged by the consumer"),
		Consumer.Prepare(Event).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				AlreadyAcknowledged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueExecutorRetryReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter.RetryReplayAndSuccess",
	SwordRhythmCueExecutorFlags)

bool Fdemo_mapSwordRhythmCueExecutorRetryReplayTest::RunTest(const FString&)
{
	FSwordRhythmCueExecutorFixture Fixture(ExecutorRun, ExecutorWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("route source is ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeCoordinator(
				ExecutorRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto Route = Consumer.Prepare(Event).Route;
	FFakeCueExecutor Executor;
	Executor.Mode = EFakeCueExecutorMode::RetryableFailure;
	const FGuid RetryAttempt(0xA8050011, 0, 0, 1);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer, Route, RetryAttempt, Executor);
	Executor.Mode = EFakeCueExecutorMode::Succeeded;
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer, Route, RetryAttempt, Executor);
	TestTrue(TEXT("retry remains pending and exact replay skips executor"),
		Retry.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RetryRecorded
			&& Retry.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					AttemptReplayed
			&& Replay.IsSuccess()
			&& !Replay.bExecutorInvoked
			&& Executor.InvocationCount == 1
			&& Consumer.IsPending());
	const FGuid SuccessAttempt(0xA8050012, 0, 0, 1);
	const auto Succeeded =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer, Route, SuccessAttempt, Executor);
	const auto SuccessReplay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer, Route, SuccessAttempt, Executor);
	TestTrue(TEXT("new attempt succeeds once and then replays locally"),
		Succeeded.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::Succeeded
			&& SuccessReplay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					AttemptReplayed
			&& Succeeded.IsSuccess()
			&& SuccessReplay.IsSuccess()
			&& Executor.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueExecutorNoOpTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter.NoOp",
	SwordRhythmCueExecutorFlags)

bool Fdemo_mapSwordRhythmCueExecutorNoOpTest::RunTest(const FString&)
{
	FSwordRhythmCueExecutorFixture Fixture(ExecutorRun, ExecutorWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent EmptyEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("real initial observation forms one zero-command route"),
		Fixture.bReady
			&& Fixture.TryExecute(EmptyEvent)
			&& EmptyEvent.NumCommands() == 0
			&& TryMakeCoordinator(
				ExecutorRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto Route = Consumer.Prepare(EmptyEvent).Route;
	FFakeCueExecutor Executor;
	Executor.Mode = EFakeCueExecutorMode::Rejected;
	const FGuid Attempt(0xA8050021, 0, 0, 1);
	const auto NoOp =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer, Route, Attempt, Executor);
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer, Route, Attempt, Executor);
	TestTrue(TEXT("zero-command route succeeds and replays without executor"),
		Route.IsValid()
			&& Route.NumCommands() == 0
			&& NoOp.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpSucceeded
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpReplayed
			&& NoOp.IsSuccess()
			&& Replay.IsSuccess()
			&& Executor.InvocationCount == 0
			&& !NoOp.Invocation.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueExecutorEvidenceFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter.EvidenceFence",
	SwordRhythmCueExecutorFlags)

bool Fdemo_mapSwordRhythmCueExecutorEvidenceFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueExecutorFixture Fixture(ExecutorRun, ExecutorWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Visual;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Audio;
	TestTrue(TEXT("visual and audio routes are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeCoordinator(
				ExecutorRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Visual)
			&& TryMakeCoordinator(
				ExecutorRun,
				AudioConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					AudioConsumerRoleId(),
				Audio));
	const auto VisualRoute = Visual.Prepare(Event).Route;
	const auto AudioRoute = Audio.Prepare(Event).Route;
	FFakeCueExecutor Executor;
	Executor.Mode = EFakeCueExecutorMode::Rejected;
	const auto Rejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Visual,
			VisualRoute,
			FGuid(0xA8050031, 0, 0, 1),
			Executor);
	TestTrue(TEXT("executor rejection records no coordinator attempt"),
		Rejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					ExecutorRejected
			&& Executor.InvocationCount == 1
			&& Visual.IsEmpty()
			&& Visual.Prepare(Event).IsRouted());

	TestTrue(TEXT("foreign invocation fixture captures"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::TryCreate(
			AudioRoute,
			FGuid(0xA8050032, 0, 0, 1),
			Executor.EvidenceInvocationOverride));
	Executor.Mode = EFakeCueExecutorMode::MismatchedEvidence;
	const auto Mismatch =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Visual,
			VisualRoute,
			FGuid(0xA8050033, 0, 0, 1),
			Executor);
	TestTrue(TEXT("valid but foreign receipt fails evidence correlation"),
		Mismatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					ExecutorEvidenceMismatch
			&& Executor.InvocationCount == 2
			&& Visual.IsEmpty());
	const auto CrossConsumer =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Visual,
			AudioRoute,
			FGuid(0xA8050034, 0, 0, 1),
			Executor);
	const auto InvalidAttempt =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Visual, VisualRoute, FGuid(), Executor);
	TestTrue(TEXT("scope and attempt preflight reject before executor"),
		CrossConsumer.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::ScopeMismatch
			&& InvalidAttempt.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptInvalid
			&& Executor.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueExecutorPendingTeardownTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter.PendingAndTeardown",
	SwordRhythmCueExecutorFlags)

bool Fdemo_mapSwordRhythmCueExecutorPendingTeardownTest::RunTest(
	const FString&)
{
	FSwordRhythmCueExecutorFixture Fixture(ExecutorRun, ExecutorWeapon);
	FSwordRhythmCueExecutorFixture Foreign(ForeignRun, ForeignWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent FirstEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent ForeignEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("two source Runs and one consumer are ready"),
		Fixture.bReady
			&& Foreign.bReady
			&& Fixture.TryMakeEffectEvent(FirstEvent)
			&& Foreign.TryMakeEffectEvent(ForeignEvent)
			&& TryMakeCoordinator(
				ExecutorRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto FirstRoute = Consumer.Prepare(FirstEvent).Route;
	FFakeCueExecutor Executor;
	Executor.Mode = EFakeCueExecutorMode::RetryableFailure;
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer,
			FirstRoute,
			FGuid(0xA8050041, 0, 0, 1),
			Executor);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent NextEvent;
	TestTrue(TEXT("strictly newer source observation is available"),
		Retry.IsSuccess()
			&& Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecute(NextEvent)
			&& NextEvent.GetState().GetObservationRevision()
				> FirstEvent.GetState().GetObservationRevision());
	const auto NextRoute = Consumer.Prepare(NextEvent).Route;
	Executor.Mode = EFakeCueExecutorMode::Succeeded;
	const auto Blocked =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer,
			NextRoute,
			FGuid(0xA8050042, 0, 0, 1),
			Executor);
	TestTrue(TEXT("pending earlier route blocks newer executor invocation"),
		Blocked.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RoutePending
			&& Executor.InvocationCount == 1);
	const auto FirstSuccess =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer,
			FirstRoute,
			FGuid(0xA8050043, 0, 0, 1),
			Executor);
	TestTrue(TEXT("earlier route succeeds before source teardown"),
		FirstSuccess.IsSuccess() && Executor.InvocationCount == 2);
	TestTrue(TEXT("source ProductSession teardown succeeds"),
		Fixture.Session.TryEnd(ExecutorRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty());
	const auto NextSuccess =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer,
			NextRoute,
			FGuid(0xA8050044, 0, 0, 1),
			Executor);
	TestTrue(TEXT("prepared immutable route executes after source teardown"),
		NextSuccess.IsSuccess()
			&& Executor.InvocationCount == 2 + (NextRoute.NumCommands() > 0 ? 1 : 0)
			&& NextSuccess.Submission.Receipt.IsValid());
	const int32 InvocationCountAfterNext = Executor.InvocationCount;
	const auto Stale =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Consumer,
			FirstRoute,
			FGuid(0xA8050045, 0, 0, 1),
			Executor);
	TestTrue(TEXT("stale route is rejected before another executor call"),
		Stale.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					RouteUnavailable
			&& Executor.InvocationCount == InvocationCountAfterNext);
	return true;
}

#endif
