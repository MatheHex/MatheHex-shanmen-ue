#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueConsumerFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ConsumerRun(
		0xA7000001, 0xA7000002, 0xA7000003, 0xA7000004);
	const FGuid OtherRun(
		0xA7000011, 0xA7000012, 0xA7000013, 0xA7000014);
	const FGuid ConsumerWeapon(
		0xA7010001, 0xA7010002, 0xA7010003, 0xA7010004);
	const FGuid OtherWeapon(
		0xA7010011, 0xA7010012, 0xA7010013, 0xA7010014);
	const FGuid VisualConsumer(
		0xA7020001, 0xA7020002, 0xA7020003, 0xA7020004);
	const FGuid AudioConsumer(
		0xA7030001, 0xA7030002, 0xA7030003, 0xA7030004);

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

	bool TryMakeCommand(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const FGuid& AttemptId,
		const FGuid& ExecutorReceiptId,
		const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome,
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Out)
	{
		return Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand::TryCreate(
			Route, AttemptId, ExecutorReceiptId, Outcome, Out);
	}

	struct FSwordRhythmCueConsumerFixture
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

		FSwordRhythmCueConsumerFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueConsumerPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueConsumerPlayerHealth"))
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueConsumerRoleProjectionTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt.RoleProjection",
	SwordRhythmCueConsumerFlags)

bool Fdemo_mapSwordRhythmCueConsumerRoleProjectionTest::RunTest(
	const FString&)
{
	FSwordRhythmCueConsumerFixture Fixture(ConsumerRun, ConsumerWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	TestTrue(TEXT("real product route creates one visual and one audio cue"),
		Fixture.bReady && Fixture.TryMakeEffectEvent(Event));
	if (!Event.IsValid())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Visual;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Audio;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Unsupported;
	TestTrue(TEXT("visual audio and syntactically valid unknown consumers capture"),
		TryMakeCoordinator(
			ConsumerRun,
			VisualConsumer,
			Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
				VisualConsumerRoleId(),
			Visual)
			&& TryMakeCoordinator(
				ConsumerRun,
				AudioConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					AudioConsumerRoleId(),
				Audio)
			&& TryMakeCoordinator(
				ConsumerRun,
				FGuid(0xA7040001, 0, 0, 1),
				FName(TEXT("Presentation.Unknown.SwordRhythm")),
				Unsupported));
	const auto VisualRoute = Visual.Prepare(Event);
	const auto AudioRoute = Audio.Prepare(Event);
	const auto UnknownRoute = Unsupported.Prepare(Event);
	TestTrue(TEXT("each canonical role receives only its typed channel"),
		VisualRoute.IsRouted()
			&& AudioRoute.IsRouted()
			&& VisualRoute.Route.NumCommands() == 1
			&& AudioRoute.Route.NumCommands() == 1
			&& VisualRoute.Route.GetCommands()[0].GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& AudioRoute.Route.GetCommands()[0].GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio);
	TestTrue(TEXT("consumer identity remains part of route identity"),
		VisualRoute.Route.GetRouteId() != AudioRoute.Route.GetRouteId());
	TestTrue(TEXT("unknown role fails closed before projection"),
		UnknownRoute.Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				RoleUnsupported);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueConsumerRetryTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt.RetryThenSuccess",
	SwordRhythmCueConsumerFlags)

bool Fdemo_mapSwordRhythmCueConsumerRetryTest::RunTest(const FString&)
{
	FSwordRhythmCueConsumerFixture Fixture(ConsumerRun, ConsumerWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("consumer and effect event are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeCoordinator(
				ConsumerRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto Prepared = Consumer.Prepare(Event);
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Retry;
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Conflict;
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Success;
	const FGuid RetryAttempt(0xA7050001, 0, 0, 1);
	TestTrue(TEXT("opaque retry success and conflicting evidence capture"),
		Prepared.IsRouted()
			&& TryMakeCommand(
				Prepared.Route,
				RetryAttempt,
				FGuid(0xA7060001, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure,
				Retry)
			&& TryMakeCommand(
				Prepared.Route,
				RetryAttempt,
				FGuid(0xA7060002, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure,
				Conflict)
			&& TryMakeCommand(
				Prepared.Route,
				FGuid(0xA7050002, 0, 0, 1),
				FGuid(0xA7060003, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
				Success));
	const auto Failed = Consumer.Submit(Prepared.Route, Retry);
	const auto RetryReplay = Consumer.Submit(Prepared.Route, Retry);
	const auto AttemptConflict = Consumer.Submit(Prepared.Route, Conflict);
	const auto PreparedAgain = Consumer.Prepare(Event);
	TestTrue(TEXT("retry remains pending and exact replay is stable"),
		Failed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					RetryRecorded
			&& RetryReplay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					RetryReplayed
			&& Failed.Receipt.Matches(RetryReplay.Receipt)
			&& Consumer.IsPending()
			&& PreparedAgain.IsRouted());
	TestTrue(TEXT("same AttemptId cannot change executor evidence"),
		AttemptConflict.Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				AttemptConflict);
	const auto Accepted = Consumer.Submit(Prepared.Route, Success);
	const auto SuccessReplay = Consumer.Submit(Prepared.Route, Success);
	const auto Already = Consumer.Prepare(Event);
	TestTrue(TEXT("only success advances delivery acknowledgement"),
		Accepted.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					Acknowledged
			&& Accepted.IsAcknowledged()
			&& SuccessReplay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					AcknowledgementReplayed
			&& SuccessReplay.Receipt.Matches(Accepted.Receipt)
			&& Already.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
					AlreadyAcknowledged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueConsumerNoOpTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt.NoOp",
	SwordRhythmCueConsumerFlags)

bool Fdemo_mapSwordRhythmCueConsumerNoOpTest::RunTest(const FString&)
{
	FSwordRhythmCueConsumerFixture Fixture(ConsumerRun, ConsumerWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent EmptyEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("initial valid observation has no symbolic cue commands"),
		Fixture.bReady
			&& Fixture.TryExecute(EmptyEvent)
			&& EmptyEvent.IsValid()
			&& EmptyEvent.NumCommands() == 0
			&& TryMakeCoordinator(
				ConsumerRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto Prepared = Consumer.Prepare(EmptyEvent);
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand InvalidPlayback;
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand NoOp;
	TestFalse(TEXT("zero-command route cannot claim executor playback"),
		TryMakeCommand(
			Prepared.Route,
			FGuid(0xA7070001, 0, 0, 1),
			FGuid(0xA7080001, 0, 0, 1),
			Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
			InvalidPlayback));
	TestTrue(TEXT("zero-command route requires explicit executor-free no-op"),
		Prepared.IsRouted()
			&& Prepared.Route.NumCommands() == 0
			&& TryMakeCommand(
				Prepared.Route,
				FGuid(0xA7070002, 0, 0, 1),
				FGuid(),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					NoOpSucceeded,
				NoOp));
	const auto Accepted = Consumer.Submit(Prepared.Route, NoOp);
	const auto Replay = Consumer.Submit(Prepared.Route, NoOp);
	TestTrue(TEXT("no-op success advances once and replays idempotently"),
		Accepted.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpAcknowledged
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpReplayed
			&& Accepted.Receipt.Matches(Replay.Receipt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueConsumerOrderingTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt.PendingAndOrdering",
	SwordRhythmCueConsumerFlags)

bool Fdemo_mapSwordRhythmCueConsumerOrderingTest::RunTest(const FString&)
{
	FSwordRhythmCueConsumerFixture Fixture(ConsumerRun, ConsumerWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent FirstEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
	TestTrue(TEXT("first route is ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(FirstEvent)
			&& TryMakeCoordinator(
				ConsumerRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Consumer));
	const auto First = Consumer.Prepare(FirstEvent);
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Retry;
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand FirstSuccess;
	TestTrue(TEXT("first route attempt evidence captures"),
		First.IsRouted()
			&& TryMakeCommand(
				First.Route,
				FGuid(0xA7090001, 0, 0, 1),
				FGuid(0xA70A0001, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure,
				Retry)
			&& TryMakeCommand(
				First.Route,
				FGuid(0xA7090002, 0, 0, 1),
				FGuid(0xA70A0002, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
				FirstSuccess));
	TestTrue(TEXT("first route enters retry-pending state"),
		Consumer.Submit(First.Route, Retry).IsSuccess());
	Fdemo_mapShanmenSwordRhythmEffectCueEvent NextEvent;
	TestTrue(TEXT("source creates a strictly newer observation"),
		Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecute(NextEvent)
			&& NextEvent.GetState().GetObservationRevision()
				> FirstEvent.GetState().GetObservationRevision());
	const auto Next = Consumer.Prepare(NextEvent);
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand NextSuccess;
	const bool bNextIsNoOp = Next.IsRouted()
		&& Next.Route.NumCommands() == 0;
	TestTrue(TEXT("newer route success evidence captures"),
		Next.IsRouted()
			&& TryMakeCommand(
				Next.Route,
				FGuid(0xA7090003, 0, 0, 1),
				bNextIsNoOp
					? FGuid()
					: FGuid(0xA70A0003, 0, 0, 1),
				bNextIsNoOp
					? Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
						NoOpSucceeded
					: Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
						Succeeded,
				NextSuccess));
	TestTrue(TEXT("pending earlier route blocks newer submission"),
		Consumer.Submit(Next.Route, NextSuccess).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				PendingRoute);
	TestTrue(TEXT("earlier success then permits newer acknowledgement"),
		Consumer.Submit(First.Route, FirstSuccess).IsAcknowledged()
			&& Consumer.Submit(Next.Route, NextSuccess).IsAcknowledged());
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand OldNewAttempt;
	TestTrue(TEXT("old route command remains structurally valid"),
		TryMakeCommand(
			First.Route,
			FGuid(0xA7090004, 0, 0, 1),
			FGuid(0xA70A0004, 0, 0, 1),
			Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
			OldNewAttempt));
	TestTrue(TEXT("old route fails closed after current route advances"),
		Consumer.Submit(First.Route, OldNewAttempt).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				StaleRoute);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueConsumerFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt.FenceAndTeardown",
	SwordRhythmCueConsumerFlags)

bool Fdemo_mapSwordRhythmCueConsumerFenceTest::RunTest(const FString&)
{
	FSwordRhythmCueConsumerFixture Fixture(ConsumerRun, ConsumerWeapon);
	FSwordRhythmCueConsumerFixture Foreign(OtherRun, OtherWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent ForeignEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Visual;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Audio;
	TestTrue(TEXT("two Runs and two consumer roles are ready"),
		Fixture.bReady
			&& Foreign.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& Foreign.TryMakeEffectEvent(ForeignEvent)
			&& TryMakeCoordinator(
				ConsumerRun,
				VisualConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId(),
				Visual)
			&& TryMakeCoordinator(
				ConsumerRun,
				AudioConsumer,
				Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					AudioConsumerRoleId(),
				Audio));
	const auto VisualRoute = Visual.Prepare(Event);
	const auto AudioRoute = Audio.Prepare(Event);
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand VisualSuccess;
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand AudioSuccess;
	TestTrue(TEXT("both channel-specific attempts capture"),
		VisualRoute.IsRouted()
			&& AudioRoute.IsRouted()
			&& TryMakeCommand(
				VisualRoute.Route,
				FGuid(0xA70B0001, 0, 0, 1),
				FGuid(0xA70C0001, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
				VisualSuccess)
			&& TryMakeCommand(
				AudioRoute.Route,
				FGuid(0xA70B0002, 0, 0, 1),
				FGuid(0xA70C0002, 0, 0, 1),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
				AudioSuccess));
	TestTrue(TEXT("cross-consumer route fails scope fence"),
		Visual.Submit(AudioRoute.Route, AudioSuccess).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				ScopeMismatch);
	TestTrue(TEXT("cross-route command fails route fence"),
		Visual.Submit(VisualRoute.Route, AudioSuccess).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				RouteMismatch);
	TestTrue(TEXT("cross-Run event fails before routing"),
		Visual.Prepare(ForeignEvent).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				RunMismatch);
	TestTrue(TEXT("invalid command fails without state mutation"),
		Visual.Submit(
			VisualRoute.Route,
			Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand()).Status
			== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				CommandInvalid);
	const auto Accepted = Visual.Submit(VisualRoute.Route, VisualSuccess);
	const auto StableRoute = VisualRoute.Route;
	const auto StableReceipt = Accepted.Receipt;
	TestTrue(TEXT("own route succeeds"), Accepted.IsAcknowledged());
	TestTrue(TEXT("source ProductSession tears down"),
		Fixture.Session.TryEnd(ConsumerRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty());
	TestTrue(TEXT("immutable route receipt and coordinator survive teardown"),
		StableRoute.IsValid()
			&& StableReceipt.IsValid()
			&& Visual.IsValid());
	return true;
}

#endif
