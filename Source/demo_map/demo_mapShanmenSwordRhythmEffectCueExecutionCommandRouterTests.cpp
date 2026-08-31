#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueCommandRouterFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid RouterRun(
		0xAC000001, 0xAC000002, 0xAC000003, 0xAC000004);
	const FGuid ForeignRun(
		0xAC000011, 0xAC000012, 0xAC000013, 0xAC000014);
	const FGuid RouterWeapon(
		0xAC010001, 0xAC010002, 0xAC010003, 0xAC010004);
	const FGuid VisualConsumer(
		0xAC020001, 0xAC020002, 0xAC020003, 0xAC020004);
	const FGuid AudioConsumer(
		0xAC030001, 0xAC030002, 0xAC030003, 0xAC030004);
	const FGuid AlternateAudioConsumer(
		0xAC030011, 0xAC030012, 0xAC030013, 0xAC030014);

	struct FSwordRhythmCueCommandRouterFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueCommandRouterFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueCommandRouterPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueCommandRouterPlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					RouterRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(RouterRun, Diagnostic)
				&& Session.TryBegin(RouterRun, Diagnostic);
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
				RouterWeapon, 1.0f, {});
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

	enum class ERouterExecutorMode : uint8
	{
		Succeeded,
		RetryableFailure,
		Rejected
	};

	class FRouterFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FRouterFakeExecutor(const uint32 InReceiptSeed)
			: ReceiptSeed(InReceiptSeed)
		{
		}

		ERouterExecutorMode Mode = ERouterExecutorMode::Succeeded;
		int32 InvocationCount = 0;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ERouterExecutorMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Router fake executor rejected command.");
				return Result;
			}
			const auto Outcome = Mode == ERouterExecutorMode::RetryableFailure
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
				Result.Diagnostic = TEXT("Router fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT("Router fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	bool TryCaptureCreate(
		const FGuid& CommandId,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand,
		const FGuid& InAudioConsumer = AudioConsumer)
	{
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureCreate(
				CommandId,
				Events,
				VisualConsumer,
				InAudioConsumer,
				OutCommand);
	}

	bool TryCaptureProcess(
		const FGuid& CommandId,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter& Router,
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand)
	{
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureProcessNext(
				CommandId,
				Router.GetRunId(),
				Router.GetBatchId(),
				VisualAttemptId,
				AudioAttemptId,
				OutCommand);
	}

	bool TryBeginRouter(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		const FGuid& CommandId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter& OutRouter)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
		return TryCaptureCreate(CommandId, Events, Command)
			&& OutRouter.TryRoute(Command).IsSuccess();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandRouterLifecycleTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter.CreateProcessEnd",
	SwordRhythmCueCommandRouterFlags)

bool Fdemo_mapSwordRhythmCueCommandRouterLifecycleTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandRouterFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("real ordered batch is ready"),
		Fixture.bReady && Fixture.TryMakeOrderedBatch(Events));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Create;
	TestTrue(TEXT("create command is frozen"),
		TryCaptureCreate(FGuid(0xAC100001, 0, 0, 1), Events, Create));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Router;
	const auto Created = Router.TryRoute(Create);
	const auto CreateReplay = Router.TryRoute(Create);
	TestTrue(TEXT("create commits one bound lifecycle and exact replay"),
		Created.IsSuccess()
			&& Created.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					Created
			&& CreateReplay.IsSuccess()
			&& CreateReplay.IsReplay()
			&& Router.IsValid()
			&& Router.IsBound()
			&& Router.GetRecordCount() == 1
			&& Router.NumEvents() == Events.Num());

	FRouterFakeExecutor VisualExecutor(0xAC200000);
	FRouterFakeExecutor AudioExecutor(0xAC300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand FirstProcess;
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Process;
		TestTrue(TEXT("process command captures current batch identity"),
			TryCaptureProcess(
				FGuid(0xAC110000 + Index, 0, 0, 1),
				Router,
				FGuid(0xAC210000 + Index, 0, 0, 1),
				FGuid(0xAC310000 + Index, 0, 0, 1),
				Process));
		if (Index == 0)
		{
			FirstProcess = Process;
		}
		const auto Processed = Router.TryRouteProcessNext(
			Process, VisualExecutor, AudioExecutor);
		TestTrue(TEXT("one command advances exactly one event"),
			Processed.IsSuccess()
				&& Processed.Status
					== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
						Processed
				&& Router.NumCompletedEvents() == Index + 1
				&& (Index == Events.Num() - 1
					? Processed.Session.IsBatchComplete()
					: Processed.Session.Status
						== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::
							EventCompleted));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand End;
	TestTrue(TEXT("end command captures bound identity"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
			FGuid(0xAC120001, 0, 0, 1),
			Router.GetRunId(),
			Router.GetBatchId(),
			End));
	const auto Ended = Router.TryRoute(End);
	const auto EndReplay = Router.TryRoute(End);
	const int32 InvocationsBeforeProcessReplay =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	const auto ProcessReplay = Router.TryRouteProcessNext(
		FirstProcess, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("end closes session while durable commands remain replayable"),
		Ended.IsSuccess()
			&& Ended.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Ended
			&& Router.IsValid()
			&& Router.IsEnded()
			&& Router.GetSession().IsEmpty()
			&& Router.NumCompletedEvents() == Events.Num()
			&& EndReplay.IsReplay()
			&& ProcessReplay.IsReplay()
			&& InvocationsBeforeProcessReplay
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandRouterRetryTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter.RetryReplayRecovery",
	SwordRhythmCueCommandRouterFlags)

bool Fdemo_mapSwordRhythmCueCommandRouterRetryTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandRouterFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Router;
	TestTrue(TEXT("effect batch router is ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectBatch(Events)
			&& TryBeginRouter(
				Events, FGuid(0xAC100011, 0, 0, 1), Router));
	FRouterFakeExecutor VisualExecutor(0xAC201000);
	FRouterFakeExecutor AudioExecutor(0xAC301000);
	AudioExecutor.Mode = ERouterExecutorMode::RetryableFailure;
	const FGuid CommandId(0xAC110011, 0, 0, 1);
	const FGuid VisualAttempt(0xAC210011, 0, 0, 1);
	const FGuid AudioAttempt(0xAC310011, 0, 0, 1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Process;
	TestTrue(TEXT("retry process command is captured"),
		TryCaptureProcess(
			CommandId, Router, VisualAttempt, AudioAttempt, Process));
	const auto Retry = Router.TryRouteProcessNext(
		Process, VisualExecutor, AudioExecutor);
	AudioExecutor.Mode = ERouterExecutorMode::Succeeded;
	const auto Replay = Router.TryRouteProcessNext(
		Process, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("retry receipt is durable and exact replay is executor free"),
		Retry.IsSuccess()
			&& Retry.Session.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending
			&& Router.NumCompletedEvents() == 0
			&& Replay.IsReplay()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Conflict;
	TestTrue(TEXT("same CommandId with another attempt captures structurally"),
		TryCaptureProcess(
			CommandId,
			Router,
			VisualAttempt,
			FGuid(0xAC310012, 0, 0, 1),
			Conflict));
	const auto Conflicted = Router.TryRouteProcessNext(
		Conflict, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("CommandId conflict rejects before executor"),
		Conflicted.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				CommandIdConflict
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Recover;
	TestTrue(TEXT("fresh command and failed-channel attempt capture"),
		TryCaptureProcess(
			FGuid(0xAC110012, 0, 0, 1),
			Router,
			VisualAttempt,
			FGuid(0xAC310012, 0, 0, 1),
			Recover));
	const auto Recovered = Router.TryRouteProcessNext(
		Recover, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("fresh recovery invokes only failed Audio and advances"),
		Recovered.IsSuccess()
			&& Recovered.Session.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::
					EventCompleted
			&& Router.NumCompletedEvents() == 1
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandRouterRejectTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter.RejectReplayRecovery",
	SwordRhythmCueCommandRouterFlags)

bool Fdemo_mapSwordRhythmCueCommandRouterRejectTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandRouterFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("effect event is ready"),
		Fixture.bReady && Fixture.TryMakeEffectBatch(Events));
	Events.SetNum(1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Router;
	TestTrue(TEXT("single-event router is ready"),
		TryBeginRouter(Events, FGuid(0xAC100021, 0, 0, 1), Router));
	FRouterFakeExecutor VisualExecutor(0xAC202000);
	FRouterFakeExecutor AudioExecutor(0xAC302000);
	VisualExecutor.Mode = ERouterExecutorMode::Rejected;
	const FGuid AudioAttempt(0xAC310021, 0, 0, 1);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand RejectCommand;
	TestTrue(TEXT("reject command captures"),
		TryCaptureProcess(
			FGuid(0xAC110021, 0, 0, 1),
			Router,
			FGuid(0xAC210021, 0, 0, 1),
			AudioAttempt,
			RejectCommand));
	const auto Rejected = Router.TryRouteProcessNext(
		RejectCommand, VisualExecutor, AudioExecutor);
	const auto Replay = Router.TryRouteProcessNext(
		RejectCommand, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("reject receipt preserves sibling acknowledgement durably"),
		Rejected.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				SessionRejected
			&& !Rejected.IsSuccess()
			&& Rejected.bRouterStateCommitted
			&& Rejected.Session.Host.NumAcknowledgedConsumers() == 1
			&& Router.IsValid()
			&& Router.NumCompletedEvents() == 0
			&& Replay.IsReplay()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);

	VisualExecutor.Mode = ERouterExecutorMode::Succeeded;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Recover;
	TestTrue(TEXT("fresh Visual recovery command captures"),
		TryCaptureProcess(
			FGuid(0xAC110022, 0, 0, 1),
			Router,
			FGuid(0xAC210022, 0, 0, 1),
			AudioAttempt,
			Recover));
	const auto Recovered = Router.TryRouteProcessNext(
		Recover, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("recovery invokes only rejected Visual and completes batch"),
		Recovered.IsSuccess()
			&& Recovered.Session.IsBatchComplete()
			&& Router.NumCompletedEvents() == 1
			&& VisualExecutor.InvocationCount == 2
			&& AudioExecutor.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandRouterFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter.IdentityLifecycleFence",
	SwordRhythmCueCommandRouterFlags)

bool Fdemo_mapSwordRhythmCueCommandRouterFenceTest::RunTest(const FString&)
{
	FSwordRhythmCueCommandRouterFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("single effect event is ready"),
		Fixture.bReady && Fixture.TryMakeEffectBatch(Events));
	Events.SetNum(1);
	FRouterFakeExecutor VisualExecutor(0xAC203000);
	FRouterFakeExecutor AudioExecutor(0xAC303000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Router;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand BeforeCreate;
	TestTrue(TEXT("pre-create process command is structurally valid"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureProcessNext(
				FGuid(0xAC110031, 0, 0, 1),
				RouterRun,
				FGuid(0xAC400001, 0, 0, 1),
				FGuid(0xAC210031, 0, 0, 1),
				FGuid(0xAC310031, 0, 0, 1),
				BeforeCreate));
	TestTrue(TEXT("process before Create rejects without executor"),
		Router.TryRouteProcessNext(
			BeforeCreate, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				LifecycleConflict
			&& Router.IsEmpty()
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);
	TestTrue(TEXT("router Create succeeds"),
		TryBeginRouter(Events, FGuid(0xAC100031, 0, 0, 1), Router));

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand ForeignIdentity;
	TestTrue(TEXT("foreign Run command captures"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureProcessNext(
				FGuid(0xAC110032, 0, 0, 1),
				ForeignRun,
				Router.GetBatchId(),
				FGuid(0xAC210032, 0, 0, 1),
				FGuid(0xAC310032, 0, 0, 1),
				ForeignIdentity));
	const auto Foreign = Router.TryRouteProcessNext(
		ForeignIdentity, VisualExecutor, AudioExecutor);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand SecondCreate;
	TestTrue(TEXT("second Create command captures"),
		TryCaptureCreate(
			FGuid(0xAC100032, 0, 0, 1), Events, SecondCreate));
	const auto DuplicateCreate = Router.TryRoute(SecondCreate);
	TestTrue(TEXT("identity and second lifecycle reject before mutation"),
		Foreign.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				IdentityMismatch
			&& DuplicateCreate.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					LifecycleConflict
			&& Router.GetRecordCount() == 1
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand EarlyEnd;
	TestTrue(TEXT("early End captures"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
			FGuid(0xAC120031, 0, 0, 1),
			Router.GetRunId(),
			Router.GetBatchId(),
			EarlyEnd));
	const auto EarlyRejected = Router.TryRoute(EarlyEnd);
	TestTrue(TEXT("early End rejection is a stable durable receipt"),
		EarlyRejected.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				SessionRejected
			&& EarlyRejected.bRouterStateCommitted
			&& Router.TryRoute(EarlyEnd).IsReplay()
			&& Router.GetRecordCount() == 2);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Process;
	TestTrue(TEXT("completion command captures"),
		TryCaptureProcess(
			FGuid(0xAC110033, 0, 0, 1),
			Router,
			FGuid(0xAC210033, 0, 0, 1),
			FGuid(0xAC310033, 0, 0, 1),
			Process));
	const auto Completed = Router.TryRouteProcessNext(
		Process, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("fresh process completes batch"),
		Completed.IsSuccess() && Completed.Session.IsBatchComplete());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand End;
	TestTrue(TEXT("fresh End captures after completion"),
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
			FGuid(0xAC120032, 0, 0, 1),
			Router.GetRunId(),
			Router.GetBatchId(),
			End));
	TestTrue(TEXT("fresh End closes lifecycle"),
		Router.TryRoute(End).IsSuccess() && Router.IsEnded());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand AfterEnd;
	TestTrue(TEXT("post-end process captures"),
		TryCaptureProcess(
			FGuid(0xAC110034, 0, 0, 1),
			Router,
			FGuid(0xAC210034, 0, 0, 1),
			FGuid(0xAC310034, 0, 0, 1),
			AfterEnd));
	const int32 InvocationsBeforeAfterEnd =
		VisualExecutor.InvocationCount + AudioExecutor.InvocationCount;
	TestTrue(TEXT("new process after End rejects before executor"),
		Router.TryRouteProcessNext(
			AfterEnd, VisualExecutor, AudioExecutor).Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				LifecycleConflict
			&& InvocationsBeforeAfterEnd
				== VisualExecutor.InvocationCount + AudioExecutor.InvocationCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueCommandRouterValidationTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter.ValidationAndContext",
	SwordRhythmCueCommandRouterFlags)

bool Fdemo_mapSwordRhythmCueCommandRouterValidationTest::RunTest(
	const FString&)
{
	FSwordRhythmCueCommandRouterFixture Fixture;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	TestTrue(TEXT("ordered validation batch is ready"),
		Fixture.bReady && Fixture.TryMakeOrderedBatch(Events));
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Reversed = Events;
	Algo::Reverse(Reversed);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Invalid;
	TestTrue(TEXT("invalid command payloads fail capture"),
		!TryCaptureCreate(FGuid(0xAC100041, 0, 0, 1), {}, Invalid)
			&& !TryCaptureCreate(
				FGuid(0xAC100042, 0, 0, 1), Reversed, Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureCreate(
					FGuid(0xAC100043, 0, 0, 1),
					Events,
					VisualConsumer,
					VisualConsumer,
					Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureProcessNext(
					FGuid(0xAC110041, 0, 0, 1),
					RouterRun,
					FGuid(0xAC400041, 0, 0, 1),
					FGuid(),
					FGuid(0xAC310041, 0, 0, 1),
					Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureEnd(
					FGuid(0xAC120041, 0, 0, 1),
					RouterRun,
					FGuid(),
					Invalid));

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Create;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Conflict;
	const FGuid SharedCommandId(0xAC100044, 0, 0, 1);
	TestTrue(TEXT("scoped Create variants capture"),
		TryCaptureCreate(SharedCommandId, Events, Create)
			&& TryCaptureCreate(
				SharedCommandId, Events, Conflict, AlternateAudioConsumer)
			&& !Create.Matches(Conflict));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Router;
	FRouterFakeExecutor VisualExecutor(0xAC204000);
	FRouterFakeExecutor AudioExecutor(0xAC304000);
	const auto WrongCreateRoute = Router.TryRouteProcessNext(
		Create, VisualExecutor, AudioExecutor);
	TestTrue(TEXT("Create rejects the executor route without recording"),
		WrongCreateRoute.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				ExecutorContextMismatch
			&& Router.IsEmpty()
			&& Router.GetRecordCount() == 0
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);
	TestTrue(TEXT("Create succeeds through executor-free route"),
		Router.TryRoute(Create).IsSuccess());
	const auto IdConflict = Router.TryRoute(Conflict);
	TestTrue(TEXT("same CommandId with another Create payload conflicts"),
		IdConflict.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				CommandIdConflict
			&& Router.GetRecordCount() == 1);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Process;
	TestTrue(TEXT("valid process command captures"),
		TryCaptureProcess(
			FGuid(0xAC110044, 0, 0, 1),
			Router,
			FGuid(0xAC210044, 0, 0, 1),
			FGuid(0xAC310044, 0, 0, 1),
			Process));
	const auto WrongProcessRoute = Router.TryRoute(Process);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord Record;
	TestTrue(TEXT("Process requires executors and Create record remains durable"),
		WrongProcessRoute.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				ExecutorContextMismatch
			&& Router.GetRecordCount() == 1
			&& Router.TryGetRecord(SharedCommandId, Record)
			&& Record.Result.IsDurableRecord()
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);
	return true;
}

#endif
