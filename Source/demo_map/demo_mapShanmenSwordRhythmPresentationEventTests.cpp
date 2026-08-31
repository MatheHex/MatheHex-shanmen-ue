#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmPresentationEvent.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmEventFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid EventRun(
		0xA6F40001, 0xA6F40002, 0xA6F40003, 0xA6F40004);
	const FGuid EventWeapon(
		0xA6F50001, 0xA6F50002, 0xA6F50003, 0xA6F50004);

	struct FSwordRhythmEventFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmEventFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("PresentationEventPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("PresentationEventPlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					EventRun,
					Pawn,
					Health,
					Diagnostic)
				&& Timeline.TryBegin(EventRun, Diagnostic)
				&& Session.TryBegin(EventRun, Diagnostic);
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

		bool TryExecute(
			Fdemo_mapBasicSwordProductExecutionResult& OutAction,
			Fdemo_mapShanmenSwordRhythmPresentationEvent& OutEvent)
		{
			OutAction = Coordinator.ExecutePlayerBasicSwordSweep(
				EventWeapon,
				1.0f,
				{});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			if (!OutAction.IsExecuted()
				|| !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					OutAction,
					Sample,
					Receipt,
					Diagnostic))
			{
				return false;
			}
			const auto Adapted =
				Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt(
					Session.GetPresentationState());
			if (!Adapted.IsAdapted())
			{
				Diagnostic = Adapted.Diagnostic;
				return false;
			}
			OutEvent = Adapted.Event;
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmPresentationEventIdentityTest,
	"Shanmen.0_0_10.Product.SwordRhythmPresentationEvent.IdentityAndPolling",
	SwordRhythmEventFlags)

bool Fdemo_mapSwordRhythmPresentationEventIdentityTest::RunTest(
	const FString&)
{
	FSwordRhythmEventFixture Fixture;
	TestTrue(TEXT("real Run fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	Fdemo_mapBasicSwordProductExecutionResult Action;
	Fdemo_mapShanmenSwordRhythmPresentationEvent Event;
	TestTrue(TEXT("first accepted action adapts to one event"),
		Fixture.TryExecute(Action, Event));
	const auto Repeated =
		Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt(
			Fixture.Session.GetPresentationState());
	TestTrue(TEXT("polling the same state preserves event identity"),
		Event.IsValid()
			&& Repeated.IsAdapted()
			&& Event.Matches(Repeated.Event));
	TestTrue(TEXT("started event exposes only identity cue ticks and revision"),
		!Action.AppliedDamage()
			&& Event.GetPresentationStateId()
				== Fixture.Session.GetPresentationState()
					.GetPresentationStateId()
			&& Event.GetRunId() == EventRun
			&& Event.GetReceiptId()
				== Fixture.Session.GetLastReceipt().GetReceiptId()
			&& Event.GetActivationId() == Action.ActivationId
			&& Event.GetCue()
				== Edemo_mapShanmenSwordRhythmPresentationCue::SequenceStarted
			&& Event.GetObservationRevision() == 1
			&& Event.GetPreviousInputTick() == INDEX_NONE
			&& Event.GetCurrentInputTick() == 0
			&& Event.GetTransitionOffsetTicks() == INDEX_NONE
			&& Event.GetLinkOpenOffsetTicks() == 8
			&& Event.GetLinkCloseOffsetTicks() == 13
			&& Event.GetTimelineTicksPerSecond() == 30);

	const auto Invalid =
		Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt(
			Fdemo_mapShanmenSwordRhythmPresentationState());
	TestTrue(TEXT("invalid source state fails closed with typed status"),
		!Invalid.IsAdapted()
			&& Invalid.Status
				== Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::
					StateInvalid
			&& !Invalid.Event.IsValid());

	Ademo_mapGameMode* EmptyGameMode =
		NewObject<Ademo_mapGameMode>(GetTransientPackage());
	TestNotNull(TEXT("GameMode event-query fixture is available"), EmptyGameMode);
	Fdemo_mapShanmenSwordRhythmPresentationEvent EmptyEvent;
	const bool bEventAvailable = EmptyGameMode
		? EmptyGameMode->TryGetSwordRhythmPresentationEvent(EmptyEvent)
		: false;
	TestFalse(TEXT("GameMode event query fails closed before first action"),
		bEventAvailable);
	TestFalse(TEXT("failed GameMode query returns canonical empty event"),
		EmptyEvent.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmPresentationEventTransitionTest,
	"Shanmen.0_0_10.Product.SwordRhythmPresentationEvent.AllTransitionCues",
	SwordRhythmEventFlags)

bool Fdemo_mapSwordRhythmPresentationEventTransitionTest::RunTest(
	const FString&)
{
	FSwordRhythmEventFixture Fixture;
	TestTrue(TEXT("real transition fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	TArray<Fdemo_mapShanmenSwordRhythmPresentationEvent> Events;
	TArray<Fdemo_mapBasicSwordProductExecutionResult> Actions;
	auto ExecuteAtCurrentTick = [&Fixture, &Events, &Actions]()
	{
		Fdemo_mapBasicSwordProductExecutionResult Action;
		Fdemo_mapShanmenSwordRhythmPresentationEvent Event;
		if (!Fixture.TryExecute(Action, Event))
		{
			return false;
		}
		Actions.Add(Action);
		Events.Add(Event);
		return true;
	};

	TestTrue(TEXT("Started cue is produced at tick zero"),
		ExecuteAtCurrentTick());
	TestTrue(TEXT("early transition advances by one fixed tick"),
		Fixture.TryAdvanceTicks(1) && ExecuteAtCurrentTick());
	TestTrue(TEXT("precise transition advances to the open boundary"),
		Fixture.TryAdvanceTicks(8) && ExecuteAtCurrentTick());
	TestTrue(TEXT("late transition advances to the close boundary"),
		Fixture.TryAdvanceTicks(13) && ExecuteAtCurrentTick());
	if (Events.Num() != 4 || Actions.Num() != 4)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	TestTrue(TEXT("all four cue identities are unique and ordered"),
		Events[0].GetEventId() != Events[1].GetEventId()
			&& Events[1].GetEventId() != Events[2].GetEventId()
			&& Events[2].GetEventId() != Events[3].GetEventId()
			&& Events[0].GetObservationRevision() == 1
			&& Events[1].GetObservationRevision() == 2
			&& Events[2].GetObservationRevision() == 3
			&& Events[3].GetObservationRevision() == 4);
	TestTrue(TEXT("fixed ticks map exactly to authored cue boundaries"),
		Events[0].GetCue()
				== Edemo_mapShanmenSwordRhythmPresentationCue::SequenceStarted
			&& Events[0].GetCurrentInputTick() == 0
			&& Events[0].GetTransitionOffsetTicks() == INDEX_NONE
			&& Events[1].GetCue()
				== Edemo_mapShanmenSwordRhythmPresentationCue::
					SequenceRestartedEarly
			&& Events[1].GetPreviousInputTick() == 0
			&& Events[1].GetCurrentInputTick() == 1
			&& Events[1].GetTransitionOffsetTicks() == 1
			&& Events[2].GetCue()
				== Edemo_mapShanmenSwordRhythmPresentationCue::PreciseLink
			&& Events[2].GetPreviousInputTick() == 1
			&& Events[2].GetCurrentInputTick() == 9
			&& Events[2].GetTransitionOffsetTicks() == 8
			&& Events[3].GetCue()
				== Edemo_mapShanmenSwordRhythmPresentationCue::
					SequenceRestartedLate
			&& Events[3].GetPreviousInputTick() == 9
			&& Events[3].GetCurrentInputTick() == 22
			&& Events[3].GetTransitionOffsetTicks() == 13);
	TestTrue(TEXT("presentation cue sequence never applies damage"),
		Algo::AllOf(Actions, [](const auto& Action)
		{
			return Action.IsExecuted() && !Action.AppliedDamage();
		}));

	const auto FinalEvent = Events.Last();
	TestTrue(TEXT("Run teardown invalidates the source without mutating event copy"),
		Fixture.Session.TryEnd(EventRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty()
			&& FinalEvent.IsValid()
			&& !Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt(
				Fixture.Session.GetPresentationState()).IsAdapted());
	return true;
}

#endif
