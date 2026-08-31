#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueCommandHostFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xAD000001, 0xAD000002, 0xAD000003, 0xAD000004);
	const FGuid ForeignRun(
		0xAD000011, 0xAD000012, 0xAD000013, 0xAD000014);
	const FGuid TestHost(
		0xAD010001, 0xAD010002, 0xAD010003, 0xAD010004);
	const FGuid ForeignHost(
		0xAD010011, 0xAD010012, 0xAD010013, 0xAD010014);
	const FGuid TestWeapon(
		0xAD020001, 0xAD020002, 0xAD020003, 0xAD020004);
	const FGuid VisualConsumer(
		0xAD030001, 0xAD030002, 0xAD030003, 0xAD030004);
	const FGuid AudioConsumer(
		0xAD040001, 0xAD040002, 0xAD040003, 0xAD040004);

	struct FSwordRhythmCueCommandHostFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueCommandHostFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueCommandHostPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueCommandHostPlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(TestRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(TestRun, Diagnostic)
				&& Session.TryBegin(TestRun, Diagnostic);
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
				TestWeapon, 1.0f, {});
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

	enum class EHostExecutorMode : uint8
	{
		Succeeded,
		RetryableFailure,
		Rejected
	};

	class FHostFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FHostFakeExecutor(const uint32 InReceiptSeed)
			: ReceiptSeed(InReceiptSeed)
		{
		}

		EHostExecutorMode Mode = EHostExecutorMode::Succeeded;
		int32 InvocationCount = 0;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == EHostExecutorMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Host fake executor rejected command.");
				return Result;
			}
			const auto Outcome = Mode == EHostExecutorMode::RetryableFailure
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
				Result.Diagnostic = TEXT("Host fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT("Host fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	bool TryCaptureCreate(
		const FGuid& HostId,
		const int64 Sequence,
		const FGuid& CommandId,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutEnvelope)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureCreate(
					CommandId,
					Events,
					VisualConsumer,
					AudioConsumer,
					Command)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(HostId, Sequence, Command, OutEnvelope);
	}

	bool TryCaptureProcess(
		const FGuid& HostId,
		const int64 Sequence,
		const FGuid& CommandId,
		const FGuid& RunId,
		const FGuid& BatchId,
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutEnvelope)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureProcessNext(
					CommandId,
					RunId,
					BatchId,
					VisualAttemptId,
					AudioAttemptId,
					Command)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(HostId, Sequence, Command, OutEnvelope);
	}

	bool TryCaptureProcess(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const int64 Sequence,
		const FGuid& CommandId,
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutEnvelope)
	{
		return TryCaptureProcess(
			Host.GetHostId(),
			Sequence,
			CommandId,
			Host.GetRunId(),
			Host.GetBatchId(),
			VisualAttemptId,
			AudioAttemptId,
			OutEnvelope);
	}

	bool TryCaptureEnd(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const int64 Sequence,
		const FGuid& CommandId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutEnvelope)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
				CommandId, Host.GetRunId(), Host.GetBatchId(), Command)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(
					Host.GetHostId(), Sequence, Command, OutEnvelope);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandHostLifecycleTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost.OrderedLifecycle",
	SwordRhythmCueCommandHostFlags)

bool Fdemo_mapSwordRhythmCueCommandHostLifecycleTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandHostFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("ordered batch is ready"),
		Fixture.bReady && Fixture.TryMakeOrderedBatch(Events));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Create;
	TestTrue(TEXT("sequence zero Create envelope captures"),
		TryCaptureCreate(
			TestHost, 0, FGuid(0xAD100001, 0, 0, 1), Events, Create));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created = Host.TryRoute(Create);
	const auto CreateReplay = Host.TryRoute(Create);
	TestTrue(TEXT("Create binds Host and exact envelope replay is stable"),
		Created.IsSuccess()
			&& Created.IsDurableRecord()
			&& CreateReplay.IsSuccess()
			&& CreateReplay.IsReplay()
			&& !CreateReplay.bHostStateCommitted
			&& Host.IsValid()
			&& Host.IsBound()
			&& Host.GetHostId() == TestHost
			&& Host.GetNextSequence() == 1);

	FHostFakeExecutor VisualExecutor(0xAD200000);
	FHostFakeExecutor AudioExecutor(0xAD300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope FirstProcess;
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Process;
		TestTrue(TEXT("contiguous ProcessNext envelope captures"),
			TryCaptureProcess(
				Host,
				Index + 1,
				FGuid(0xAD110000 + Index, 0, 0, 1),
				FGuid(0xAD210000 + Index, 0, 0, 1),
				FGuid(0xAD310000 + Index, 0, 0, 1),
				Process));
		if (Index == 0)
		{
			FirstProcess = Process;
		}
		const auto Processed = Host.TryRouteProcessNext(
			Process, VisualExecutor, AudioExecutor);
		TestTrue(TEXT("one envelope advances one event and one sequence"),
			Processed.IsSuccess()
				&& Processed.IsDurableRecord()
				&& Host.GetRouter().NumCompletedEvents() == Index + 1
				&& Host.GetNextSequence() == Index + 2);
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope End;
	TestTrue(TEXT("terminal envelope captures"),
		TryCaptureEnd(
			Host, Events.Num() + 1, FGuid(0xAD120001, 0, 0, 1), End));
	const auto Ended = Host.TryRoute(End);
	const int32 BeforeReplay =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	const auto ProcessReplay = Host.TryRouteProcessNext(
		FirstProcess, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("End fences new work while historical process remains replayable"),
		Ended.IsSuccess()
			&& Host.IsTerminal()
			&& Host.GetNextSequence() == Events.Num() + 2
			&& Host.TryRoute(End).IsReplay()
			&& ProcessReplay.IsSuccess()
			&& ProcessReplay.IsReplay()
			&& BeforeReplay
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandHostSequenceFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost.SequenceFence",
	SwordRhythmCueCommandHostFlags)

bool Fdemo_mapSwordRhythmCueCommandHostSequenceFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueCommandHostFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("single event is ready"),
		Fixture.bReady && Fixture.TryMakeEffectBatch(Events));
	Events.SetNum(1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope GapCreate;
	TestTrue(TEXT("future Create envelope captures"),
		TryCaptureCreate(
			TestHost, 1, FGuid(0xAD100011, 0, 0, 1), Events, GapCreate));
	TestTrue(TEXT("future sequence rejects without binding Host"),
		Host.TryRoute(GapCreate).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				SequenceGap
			&& Host.IsEmpty());

	FHostFakeExecutor VisualExecutor(0xAD201000);
	FHostFakeExecutor AudioExecutor(0xAD301000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope BeforeCreate;
	TestTrue(TEXT("sequence zero Process envelope captures structurally"),
		TryCaptureProcess(
			TestHost,
			0,
			FGuid(0xAD110011, 0, 0, 1),
			ForeignRun,
			FGuid(0xAD500011, 0, 0, 1),
			FGuid(0xAD210011, 0, 0, 1),
			FGuid(0xAD310011, 0, 0, 1),
			BeforeCreate));
	TestTrue(TEXT("non-Create sequence zero rejects before executors"),
		Host.TryRouteProcessNext(
			BeforeCreate, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				LifecycleConflict
			&& Host.IsEmpty()
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Create;
	TestTrue(TEXT("valid sequence zero Create captures and routes"),
		TryCaptureCreate(
			TestHost, 0, FGuid(0xAD100012, 0, 0, 1), Events, Create)
			&& Host.TryRoute(Create).IsSuccess());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Foreign;
	TestTrue(TEXT("foreign Host envelope captures"),
		TryCaptureProcess(
			ForeignHost,
			1,
			FGuid(0xAD110012, 0, 0, 1),
			Host.GetRunId(),
			Host.GetBatchId(),
			FGuid(0xAD210012, 0, 0, 1),
			FGuid(0xAD310012, 0, 0, 1),
			Foreign));
	TestTrue(TEXT("foreign HostId rejects before executors"),
		Host.TryRouteProcessNext(Foreign, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				HostIdentityConflict);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Future;
	TestTrue(TEXT("future process captures"),
		TryCaptureProcess(
			Host,
			2,
			FGuid(0xAD110013, 0, 0, 1),
			FGuid(0xAD210013, 0, 0, 1),
			FGuid(0xAD310013, 0, 0, 1),
			Future));
	TestTrue(TEXT("gap rejects before executors"),
		Host.TryRouteProcessNext(Future, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				SequenceGap);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Process;
	TestTrue(TEXT("next process captures and commits"),
		TryCaptureProcess(
			Host,
			1,
			FGuid(0xAD110014, 0, 0, 1),
			FGuid(0xAD210014, 0, 0, 1),
			FGuid(0xAD310014, 0, 0, 1),
			Process)
			&& Host.TryRouteProcessNext(
				Process, VisualExecutor, AudioExecutor).IsSuccess());
	const int32 BeforeConflict =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Conflict;
	TestTrue(TEXT("different envelope at historical sequence captures"),
		TryCaptureProcess(
			Host,
			1,
			FGuid(0xAD110015, 0, 0, 1),
			FGuid(0xAD210015, 0, 0, 1),
			FGuid(0xAD310015, 0, 0, 1),
			Conflict));
	TestTrue(TEXT("historical payload conflict rejects without re-entry"),
		Host.TryRouteProcessNext(
			Conflict, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				SequenceConflict
			&& Host.GetNextSequence() == 2
			&& BeforeConflict
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandHostRetryTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost.RetrySequenceRecovery",
	SwordRhythmCueCommandHostFlags)

bool Fdemo_mapSwordRhythmCueCommandHostRetryTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandHostFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Create;
	TestTrue(TEXT("effect batch Host is ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectBatch(Events)
			&& TryCaptureCreate(
				TestHost, 0, FGuid(0xAD100021, 0, 0, 1), Events, Create)
			&& Host.TryRoute(Create).IsSuccess());
	FHostFakeExecutor VisualExecutor(0xAD202000);
	FHostFakeExecutor AudioExecutor(0xAD302000);
	AudioExecutor.Mode = EHostExecutorMode::RetryableFailure;
	const FGuid CommandId(0xAD110021, 0, 0, 1);
	const FGuid VisualAttempt(0xAD210021, 0, 0, 1);
	const FGuid AudioAttempt(0xAD310021, 0, 0, 1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope RetryEnvelope;
	TestTrue(TEXT("retry envelope captures"),
		TryCaptureProcess(
			Host, 1, CommandId, VisualAttempt, AudioAttempt, RetryEnvelope));
	const auto Retry = Host.TryRouteProcessNext(
		RetryEnvelope, VisualExecutor, AudioExecutor);
	AudioExecutor.Mode = EHostExecutorMode::Succeeded;
	const auto Replay = Host.TryRouteProcessNext(
		RetryEnvelope, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("retry receipt consumes sequence and exact replay is inert"),
		Retry.IsSuccess()
			&& Retry.Router.Session.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending
			&& Retry.IsDurableRecord()
			&& Host.GetNextSequence() == 2
			&& Replay.IsReplay()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Conflict;
	TestTrue(TEXT("changed historical retry payload captures"),
		TryCaptureProcess(
			Host,
			1,
			CommandId,
			VisualAttempt,
			FGuid(0xAD310022, 0, 0, 1),
			Conflict));
	TestTrue(TEXT("changed retry payload conflicts at sequence fence"),
		Host.TryRouteProcessNext(
			Conflict, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				SequenceConflict);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Recover;
	TestTrue(TEXT("fresh failed-channel recovery captures"),
		TryCaptureProcess(
			Host,
			2,
			FGuid(0xAD110022, 0, 0, 1),
			VisualAttempt,
			FGuid(0xAD310022, 0, 0, 1),
			Recover));
	const auto Recovered = Host.TryRouteProcessNext(
		Recover, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("fresh sequence recovers only failed Audio channel"),
		Recovered.IsSuccess()
			&& Recovered.Router.Session.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventCompleted
			&& Host.GetNextSequence() == 3
			&& Host.GetRouter().NumCompletedEvents() == 1
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandHostRejectTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost.RejectAndRouterReplayFence",
	SwordRhythmCueCommandHostFlags)

bool Fdemo_mapSwordRhythmCueCommandHostRejectTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandHostFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Create;
	TestTrue(TEXT("single effect event Host is ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectBatch(Events));
	Events.SetNum(1);
	TestTrue(TEXT("Create commits"),
		TryCaptureCreate(
			TestHost, 0, FGuid(0xAD100031, 0, 0, 1), Events, Create)
			&& Host.TryRoute(Create).IsSuccess());
	FHostFakeExecutor VisualExecutor(0xAD203000);
	FHostFakeExecutor AudioExecutor(0xAD303000);
	VisualExecutor.Mode = EHostExecutorMode::Rejected;
	const FGuid AudioAttempt(0xAD310031, 0, 0, 1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope RejectedEnvelope;
	TestTrue(TEXT("reject envelope captures"),
		TryCaptureProcess(
			Host,
			1,
			FGuid(0xAD110031, 0, 0, 1),
			FGuid(0xAD210031, 0, 0, 1),
			AudioAttempt,
			RejectedEnvelope));
	const auto Rejected = Host.TryRouteProcessNext(
		RejectedEnvelope, VisualExecutor, AudioExecutor);
	const auto ReplayedReject = Host.TryRouteProcessNext(
		RejectedEnvelope, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("partial reject is durable and consumes sequence"),
		Rejected.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				RouterRejected
			&& !Rejected.IsSuccess()
			&& Rejected.IsDurableRecord()
			&& Host.GetNextSequence() == 2
			&& ReplayedReject.IsReplay()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);

	VisualExecutor.Mode = EHostExecutorMode::Succeeded;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Recover;
	TestTrue(TEXT("fresh Visual recovery captures"),
		TryCaptureProcess(
			Host,
			2,
			FGuid(0xAD110032, 0, 0, 1),
			FGuid(0xAD210032, 0, 0, 1),
			AudioAttempt,
			Recover));
	const auto Recovered = Host.TryRouteProcessNext(
		Recover, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("fresh recovery completes through failed channel only"),
		Recovered.IsSuccess()
			&& Recovered.Router.Session.IsBatchComplete()
			&& Host.GetNextSequence() == 3
			&& VisualExecutor.InvocationCount == 2
			&& AudioExecutor.InvocationCount == 1);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope RouterReplay;
	TestTrue(TEXT("old Router command can be wrapped in fresh sequence"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::TryCapture(
			Host.GetHostId(), 3, Recover.GetCommand(), RouterReplay));
	const int32 BeforeRouterReplay =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	const auto RouterReplayConflict = Host.TryRouteProcessNext(
		RouterReplay, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("Router replay cannot consume a second Host sequence"),
		RouterReplayConflict.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				CommandReplayConflict
			&& Host.GetNextSequence() == 3
			&& Host.GetRecordCount() == 3
			&& BeforeRouterReplay
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandHostValidationTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost.ValidationAndTerminal",
	SwordRhythmCueCommandHostFlags)

bool Fdemo_mapSwordRhythmCueCommandHostValidationTest::RunTest(
	const FString&)
{
	FSwordRhythmCueCommandHostFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("single validation event is ready"),
		Fixture.bReady && Fixture.TryMakeEffectBatch(Events));
	Events.SetNum(1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Invalid;
	TestTrue(TEXT("invalid HostId and negative sequence fail capture"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureCreate(
			FGuid(0xAD100041, 0, 0, 1),
			Events,
			VisualConsumer,
			AudioConsumer,
			Command)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(FGuid(), 0, Command, Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(TestHost, -1, Command, Invalid));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	TestTrue(TEXT("default invalid envelope rejects without state"),
		Host.TryRoute(Invalid).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				EnvelopeInvalid
			&& Host.IsEmpty());

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Create;
	TestTrue(TEXT("valid Create envelope captures"),
		TryCaptureCreate(
			TestHost, 0, FGuid(0xAD100042, 0, 0, 1), Events, Create));
	FHostFakeExecutor VisualExecutor(0xAD204000);
	FHostFakeExecutor AudioExecutor(0xAD304000);
	TestTrue(TEXT("Create rejects executor overload without sequence commit"),
		Host.TryRouteProcessNext(
			Create, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				ExecutorContextMismatch
			&& Host.IsEmpty()
			&& Host.TryRoute(Create).IsSuccess());

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope EarlyEnd;
	TestTrue(TEXT("early End captures"),
		TryCaptureEnd(
			Host, 1, FGuid(0xAD120041, 0, 0, 1), EarlyEnd));
	const auto EarlyRejected = Host.TryRoute(EarlyEnd);
	TestTrue(TEXT("early End rejection is durable and replayable"),
		EarlyRejected.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				RouterRejected
			&& EarlyRejected.IsDurableRecord()
			&& Host.GetNextSequence() == 2
			&& Host.TryRoute(EarlyEnd).IsReplay());

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Process;
	TestTrue(TEXT("completion envelope captures and routes"),
		TryCaptureProcess(
			Host,
			2,
			FGuid(0xAD110041, 0, 0, 1),
			FGuid(0xAD210041, 0, 0, 1),
			FGuid(0xAD310041, 0, 0, 1),
			Process)
			&& Host.TryRouteProcessNext(
				Process, VisualExecutor, AudioExecutor).IsSuccess());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope End;
	TestTrue(TEXT("terminal End captures and routes"),
		TryCaptureEnd(
			Host, 3, FGuid(0xAD120042, 0, 0, 1), End)
			&& Host.TryRoute(End).IsSuccess()
			&& Host.IsTerminal());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope AfterEnd;
	TestTrue(TEXT("post-terminal Process captures"),
		TryCaptureProcess(
			Host,
			4,
			FGuid(0xAD110042, 0, 0, 1),
			FGuid(0xAD210042, 0, 0, 1),
			FGuid(0xAD310042, 0, 0, 1),
			AfterEnd));
	const int32 BeforeAfterEnd =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Record;
	TestTrue(TEXT("terminal fence rejects new work and preserves history"),
		Host.TryRouteProcessNext(
			AfterEnd, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				LifecycleConflict
			&& Host.GetNextSequence() == 4
			&& Host.TryGetRecord(1, Record)
			&& Record.Result.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
					RouterRejected
			&& Host.TryRoute(Create).IsReplay()
			&& BeforeAfterEnd
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

#endif
