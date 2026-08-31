#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionSession.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueSessionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SessionRun(
		0xAB000001, 0xAB000002, 0xAB000003, 0xAB000004);
	const FGuid ForeignRun(
		0xAB000011, 0xAB000012, 0xAB000013, 0xAB000014);
	const FGuid SessionWeapon(
		0xAB010001, 0xAB010002, 0xAB010003, 0xAB010004);
	const FGuid ForeignWeapon(
		0xAB010011, 0xAB010012, 0xAB010013, 0xAB010014);
	const FGuid VisualConsumer(
		0xAB020001, 0xAB020002, 0xAB020003, 0xAB020004);
	const FGuid AudioConsumer(
		0xAB030001, 0xAB030002, 0xAB030003, 0xAB030004);
	const FGuid AlternateAudioConsumer(
		0xAB030011, 0xAB030012, 0xAB030013, 0xAB030014);

	struct FSwordRhythmCueSessionFixture
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

		FSwordRhythmCueSessionFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueSessionPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueSessionPlayerHealth"))
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

		bool TryMakeOrderedBatch(
			TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& OutEvents)
		{
			OutEvents.Reset();
			Fdemo_mapShanmenSwordRhythmEffectCueEvent First;
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Second;
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Third;
			if (!TryExecute(First)
				|| !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				|| !TryExecute(Second)
				|| !TryExecute(Third))
			{
				return false;
			}
			OutEvents = {First, Second, Third};
			return First.GetState().GetObservationRevision()
					< Second.GetState().GetObservationRevision()
				&& Second.GetState().GetObservationRevision()
					< Third.GetState().GetObservationRevision();
		}

		bool TryMakeEffectBatch(
			TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& OutEvents)
		{
			OutEvents.Reset();
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Discarded;
			Fdemo_mapShanmenSwordRhythmEffectCueEvent First;
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Second;
			if (!TryExecute(Discarded)
				|| !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				|| !TryExecute(Discarded)
				|| !TryExecute(First)
				|| !TryAdvanceTicks(1)
				|| !TryExecute(Second))
			{
				return false;
			}
			OutEvents = {First, Second};
			return First.NumCommands() == 2
				&& First.GetState().GetObservationRevision()
					< Second.GetState().GetObservationRevision();
		}
	};

	enum class ESessionExecutorMode : uint8
	{
		Succeeded,
		RetryableFailure,
		Rejected
	};

	struct FSessionInvocationTrace
	{
		FGuid EventId;
		Edemo_mapShanmenSwordRhythmEffectCueChannel Channel =
			Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;
	};

	class FSessionFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FSessionFakeExecutor(
			const uint32 InReceiptSeed,
			TArray<FSessionInvocationTrace>* InSharedTrace = nullptr)
			: ReceiptSeed(InReceiptSeed)
			, SharedTrace(InSharedTrace)
		{
		}

		ESessionExecutorMode Mode = ESessionExecutorMode::Succeeded;
		int32 InvocationCount = 0;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			if (SharedTrace)
			{
				SharedTrace->Add({
					Invocation.GetRoute().GetDelivery().GetEvent().GetEventId(),
					Invocation.GetRoute().GetChannel()});
			}
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ESessionExecutorMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Session fake executor rejected the batch.");
				return Result;
			}
			const auto Outcome = Mode == ESessionExecutorMode::RetryableFailure
				? Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure
				: Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded;
			if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
					Invocation,
					FGuid(ReceiptSeed + InvocationCount, 0, 0, 1),
					Outcome,
					Result.Receipt))
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Session fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT("Session fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
		TArray<FSessionInvocationTrace>* SharedTrace = nullptr;
	};

	bool TryMakeSession(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession& OutSession,
		const FGuid& InAudioConsumer = AudioConsumer)
	{
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate(
			Events,
			VisualConsumer,
			InAudioConsumer,
			OutSession);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueSessionOrderedBatchTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession.OrderedBatchCompletion",
	SwordRhythmCueSessionFlags)

bool Fdemo_mapSwordRhythmCueSessionOrderedBatchTest::RunTest(const FString&)
{
	FSwordRhythmCueSessionFixture Fixture(SessionRun, SessionWeapon);
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Session;
	TestTrue(TEXT("real ordered batch and session are ready"),
		Fixture.bReady
			&& Fixture.TryMakeOrderedBatch(Events)
			&& TryMakeSession(Events, Session));
	TArray<FSessionInvocationTrace> Trace;
	FSessionFakeExecutor VisualExecutor(0xAB040000, &Trace);
	FSessionFakeExecutor AudioExecutor(0xAB050000, &Trace);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Current;
	TestTrue(TEXT("session initially exposes the first frozen event"),
		Session.TryGetCurrentEvent(Current) && Current.Matches(Events[0]));
	const auto First = Session.ProcessNext(
		FGuid(0xAB060001, 0, 0, 1),
		FGuid(0xAB070001, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	const auto Second = Session.ProcessNext(
		FGuid(0xAB060002, 0, 0, 1),
		FGuid(0xAB070002, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	const auto Third = Session.ProcessNext(
		FGuid(0xAB060003, 0, 0, 1),
		FGuid(0xAB070003, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	const int32 CountBeforeAlready =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	const auto Already = Session.ProcessNext(
		FGuid(0xAB060004, 0, 0, 1),
		FGuid(0xAB070004, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("one completed event advances exactly one batch slot"),
		First.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventCompleted
			&& First.IsSuccess()
			&& First.CompletedEvents == 1
			&& Second.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventCompleted
			&& Second.CompletedEvents == 2
			&& Third.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::BatchCompleted
			&& Third.IsBatchComplete()
			&& Session.IsComplete()
			&& Session.NumCompletedEvents() == Events.Num());
	TestTrue(TEXT("completed batch is idempotent and invokes no executor"),
		Already.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::AlreadyCompleted
			&& Already.IsBatchComplete()
			&& CountBeforeAlready
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);

	TArray<FSessionInvocationTrace> Expected;
	for (const auto& Event : Events)
	{
		if (Event.NumCommands() > 0)
		{
			Expected.Add({
				Event.GetEventId(),
				Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual});
			Expected.Add({
				Event.GetEventId(),
				Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio});
		}
	}
	bool bTraceMatches = Trace.Num() == Expected.Num();
	for (int32 Index = 0; bTraceMatches && Index < Trace.Num(); ++Index)
	{
		bTraceMatches = Trace[Index].EventId == Expected[Index].EventId
			&& Trace[Index].Channel == Expected[Index].Channel;
	}
	TestTrue(TEXT("executor trace preserves event order and Visual then Audio"),
		bTraceMatches);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueSessionRetryTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession.RetryHoldsCursor",
	SwordRhythmCueSessionFlags)

bool Fdemo_mapSwordRhythmCueSessionRetryTest::RunTest(const FString&)
{
	FSwordRhythmCueSessionFixture Fixture(SessionRun, SessionWeapon);
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Session;
	TestTrue(TEXT("effect-first batch and session are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectBatch(Events)
			&& TryMakeSession(Events, Session));
	FSessionFakeExecutor VisualExecutor(0xAB041000);
	FSessionFakeExecutor AudioExecutor(0xAB051000);
	AudioExecutor.Mode = ESessionExecutorMode::RetryableFailure;
	const FGuid VisualAttempt(0xAB060011, 0, 0, 1);
	const FGuid AudioAttempt(0xAB070011, 0, 0, 1);
	const auto Retry = Session.ProcessNext(
		VisualAttempt,
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	AudioExecutor.Mode = ESessionExecutorMode::Succeeded;
	const auto ExactReplay = Session.ProcessNext(
		VisualAttempt,
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("retry and exact replay keep the batch cursor on event zero"),
		Retry.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending
			&& Retry.IsSuccess()
			&& ExactReplay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending
			&& ExactReplay.IsSuccess()
			&& Session.GetNextEventIndex() == 0
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);
	const auto Recovered = Session.ProcessNext(
		VisualAttempt,
		FGuid(0xAB070012, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Current;
	TestTrue(TEXT("fresh failed-channel attempt advances only after recovery"),
		Recovered.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventCompleted
			&& Recovered.CompletedEvents == 1
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 2
			&& Session.TryGetCurrentEvent(Current)
			&& Current.Matches(Events[1]));
	const auto Completed = Session.ProcessNext(
		FGuid(0xAB060013, 0, 0, 1),
		FGuid(0xAB070013, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("next event begins only after the retried event completes"),
		Completed.IsBatchComplete() && Session.IsComplete());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueSessionRejectTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession.RejectHoldsCursor",
	SwordRhythmCueSessionFlags)

bool Fdemo_mapSwordRhythmCueSessionRejectTest::RunTest(const FString&)
{
	FSwordRhythmCueSessionFixture Fixture(SessionRun, SessionWeapon);
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("effect event is ready"),
		Fixture.bReady && Fixture.TryMakeEffectBatch(Events));
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> OneEvent = {Events[0]};
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Session;
	TestTrue(TEXT("single event session is ready"),
		TryMakeSession(OneEvent, Session));
	FSessionFakeExecutor VisualExecutor(0xAB042000);
	FSessionFakeExecutor AudioExecutor(0xAB052000);
	VisualExecutor.Mode = ESessionExecutorMode::Rejected;
	const FGuid AudioAttempt(0xAB070021, 0, 0, 1);
	const auto Rejected = Session.ProcessNext(
		FGuid(0xAB060021, 0, 0, 1),
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("consumer rejection preserves sibling acknowledgement"),
		Rejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventRejected
			&& !Rejected.IsSuccess()
			&& Rejected.Host.NumAcknowledgedConsumers() == 1
			&& Session.GetNextEventIndex() == 0
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);
	VisualExecutor.Mode = ESessionExecutorMode::Succeeded;
	const auto Recovered = Session.ProcessNext(
		FGuid(0xAB060022, 0, 0, 1),
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("recovery invokes only the rejected channel"),
		Recovered.IsBatchComplete()
			&& VisualExecutor.InvocationCount == 2
			&& AudioExecutor.InvocationCount == 1
			&& Session.IsComplete());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueSessionValidationTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession.BatchValidation",
	SwordRhythmCueSessionFlags)

bool Fdemo_mapSwordRhythmCueSessionValidationTest::RunTest(const FString&)
{
	FSwordRhythmCueSessionFixture Fixture(SessionRun, SessionWeapon);
	FSwordRhythmCueSessionFixture Foreign(ForeignRun, ForeignWeapon);
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent ForeignEvent;
	TestTrue(TEXT("local and foreign event sources are ready"),
		Fixture.bReady
			&& Foreign.bReady
			&& Fixture.TryMakeOrderedBatch(Events)
			&& Foreign.TryExecute(ForeignEvent));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Invalid;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Reversed = Events;
	Algo::Reverse(Reversed);
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Duplicate = {
		Events[0], Events[0]};
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Mixed = {
		Events[0], ForeignEvent};
	TestTrue(TEXT("empty reordered duplicate mixed and aliased batches reject"),
		!TryMakeSession({}, Invalid)
			&& !TryMakeSession(Reversed, Invalid)
			&& !TryMakeSession(Duplicate, Invalid)
			&& !TryMakeSession(Mixed, Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate(
				Events,
				VisualConsumer,
				VisualConsumer,
				Invalid));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession First;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Replay;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Alternate;
	TestTrue(TEXT("batch identity is deterministic and consumer scoped"),
		TryMakeSession(Events, First)
			&& TryMakeSession(Events, Replay)
			&& TryMakeSession(Events, Alternate, AlternateAudioConsumer)
			&& First.GetBatchId() == Replay.GetBatchId()
			&& First.GetBatchId() != Alternate.GetBatchId());
	FSessionFakeExecutor VisualExecutor(0xAB043000);
	FSessionFakeExecutor AudioExecutor(0xAB053000);
	const auto InvalidAttempt = First.ProcessNext(
		FGuid(),
		FGuid(0xAB070031, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("invalid attempts preserve event zero before executors"),
		InvalidAttempt.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::AttemptInvalid
			&& First.IsValid()
			&& First.GetNextEventIndex() == 0
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueSessionLifecycleTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession.GuardedTeardown",
	SwordRhythmCueSessionFlags)

bool Fdemo_mapSwordRhythmCueSessionLifecycleTest::RunTest(const FString&)
{
	FSwordRhythmCueSessionFixture Fixture(SessionRun, SessionWeapon);
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("source batch is ready"),
		Fixture.bReady && Fixture.TryMakeOrderedBatch(Events));
	Events.SetNum(2);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Session;
	TestTrue(TEXT("two event session is ready"),
		TryMakeSession(Events, Session));
	FString EndDiagnostic;
	TestTrue(TEXT("incomplete batch cannot be ended"),
		!Session.TryEnd(SessionRun, EndDiagnostic) && Session.IsValid());
	TestTrue(TEXT("source ProductSession can end after batch capture"),
		Fixture.Session.TryEnd(SessionRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty());
	FSessionFakeExecutor VisualExecutor(0xAB044000);
	FSessionFakeExecutor AudioExecutor(0xAB054000);
	const auto First = Session.ProcessNext(
		FGuid(0xAB060041, 0, 0, 1),
		FGuid(0xAB070041, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	const auto Second = Session.ProcessNext(
		FGuid(0xAB060042, 0, 0, 1),
		FGuid(0xAB070042, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Current;
	TestTrue(TEXT("captured events complete after source teardown"),
		First.IsSuccess()
			&& Second.IsBatchComplete()
			&& Session.IsComplete()
			&& !Session.TryGetCurrentEvent(Current));
	TestTrue(TEXT("wrong Run cannot end completed session"),
		!Session.TryEnd(ForeignRun, EndDiagnostic) && Session.IsComplete());
	const int32 CountBeforeEnd =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	TestTrue(TEXT("matching Run ends and empties session"),
		Session.TryEnd(SessionRun, EndDiagnostic) && Session.IsEmpty());
	const auto AfterEnd = Session.ProcessNext(
		FGuid(0xAB060043, 0, 0, 1),
		FGuid(0xAB070043, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("ended session rejects before executor"),
		AfterEnd.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::SessionInvalid
			&& CountBeforeEnd
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

#endif
