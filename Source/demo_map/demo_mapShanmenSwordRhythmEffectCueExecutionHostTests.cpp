#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionHost.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueHostFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid HostRun(
		0xAA000001, 0xAA000002, 0xAA000003, 0xAA000004);
	const FGuid ForeignRun(
		0xAA000011, 0xAA000012, 0xAA000013, 0xAA000014);
	const FGuid HostWeapon(
		0xAA010001, 0xAA010002, 0xAA010003, 0xAA010004);
	const FGuid ForeignWeapon(
		0xAA010011, 0xAA010012, 0xAA010013, 0xAA010014);
	const FGuid VisualConsumer(
		0xAA020001, 0xAA020002, 0xAA020003, 0xAA020004);
	const FGuid AudioConsumer(
		0xAA030001, 0xAA030002, 0xAA030003, 0xAA030004);

	struct FSwordRhythmCueHostFixture
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

		FSwordRhythmCueHostFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueHostPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueHostPlayerHealth"))
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
		explicit FHostFakeExecutor(
			const uint32 InReceiptSeed,
			int32* InSharedSequence = nullptr)
			: ReceiptSeed(InReceiptSeed)
			, SharedSequence(InSharedSequence)
		{
		}

		EHostExecutorMode Mode = EHostExecutorMode::Succeeded;
		int32 InvocationCount = 0;
		int32 LastInvocationOrder = 0;
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation LastInvocation;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			LastInvocation = Invocation;
			if (SharedSequence)
			{
				LastInvocationOrder = ++(*SharedSequence);
			}
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == EHostExecutorMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Host fake executor rejected the batch.");
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
		int32* SharedSequence = nullptr;
	};

	bool TryMakeHost(
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost& OutHost)
	{
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryCreate(
			HostRun, VisualConsumer, AudioConsumer, OutHost);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueHostDualSuccessTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost.DualSuccessReplay",
	SwordRhythmCueHostFlags)

bool Fdemo_mapSwordRhythmCueHostDualSuccessTest::RunTest(const FString&)
{
	FSwordRhythmCueHostFixture Fixture(HostRun, HostWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Host;
	TestTrue(TEXT("real event and dual consumer host are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeHost(Host));
	int32 Sequence = 0;
	FHostFakeExecutor VisualExecutor(0xAA040000, &Sequence);
	FHostFakeExecutor AudioExecutor(0xAA050000, &Sequence);
	const FGuid VisualAttempt(0xAA060001, 0, 0, 1);
	const FGuid AudioAttempt(0xAA070001, 0, 0, 1);
	const auto Completed = Host.Process(
		Event,
		VisualAttempt,
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	const auto Replayed = Host.Process(
		Event,
		VisualAttempt,
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("host executes visual then audio and completes"),
		Completed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::Completed
			&& Completed.IsComplete()
			&& Completed.NumAcknowledgedConsumers() == 2
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1
			&& VisualExecutor.LastInvocationOrder == 1
			&& AudioExecutor.LastInvocationOrder == 2
			&& VisualExecutor.LastInvocation.GetRoute().GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& AudioExecutor.LastInvocation.GetRoute().GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio);
	TestTrue(TEXT("exact dual replay invokes neither executor"),
		Replayed.IsComplete()
			&& Replayed.Visual.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed
			&& Replayed.Audio.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1
			&& Sequence == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueHostPartialRetryTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost.PartialRetry",
	SwordRhythmCueHostFlags)

bool Fdemo_mapSwordRhythmCueHostPartialRetryTest::RunTest(const FString&)
{
	FSwordRhythmCueHostFixture Fixture(HostRun, HostWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Host;
	TestTrue(TEXT("effect event and host are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeHost(Host));
	FHostFakeExecutor VisualExecutor(0xAA041000);
	FHostFakeExecutor AudioExecutor(0xAA051000);
	AudioExecutor.Mode = EHostExecutorMode::RetryableFailure;
	const FGuid VisualAttempt(0xAA060011, 0, 0, 1);
	const auto Partial = Host.Process(
		Event,
		VisualAttempt,
		FGuid(0xAA070011, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("visual acknowledgement survives audio retry"),
		Partial.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::RetryPending
			&& Partial.IsSuccess()
			&& !Partial.IsComplete()
			&& Partial.NumAcknowledgedConsumers() == 1
			&& Partial.Visual.IsAcknowledged()
			&& !Partial.Audio.IsAcknowledged()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);
	AudioExecutor.Mode = EHostExecutorMode::Succeeded;
	const auto Recovered = Host.Process(
		Event,
		VisualAttempt,
		FGuid(0xAA070012, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("retry pass invokes only the unfinished audio executor"),
		Recovered.IsComplete()
			&& Recovered.Visual.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed
			&& Recovered.Audio.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueHostPartialRejectTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost.PartialRejectRecovery",
	SwordRhythmCueHostFlags)

bool Fdemo_mapSwordRhythmCueHostPartialRejectTest::RunTest(const FString&)
{
	FSwordRhythmCueHostFixture Fixture(HostRun, HostWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Host;
	TestTrue(TEXT("effect event and host are ready"),
		Fixture.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& TryMakeHost(Host));
	FHostFakeExecutor VisualExecutor(0xAA042000);
	FHostFakeExecutor AudioExecutor(0xAA052000);
	VisualExecutor.Mode = EHostExecutorMode::Rejected;
	const FGuid AudioAttempt(0xAA070021, 0, 0, 1);
	const auto Partial = Host.Process(
		Event,
		FGuid(0xAA060021, 0, 0, 1),
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("audio success is retained when visual executor rejects"),
		Partial.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::ConsumerRejected
			&& !Partial.IsSuccess()
			&& Partial.NumAcknowledgedConsumers() == 1
			&& !Partial.Visual.IsSuccess()
			&& Partial.Audio.IsAcknowledged()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);
	VisualExecutor.Mode = EHostExecutorMode::Succeeded;
	const auto Recovered = Host.Process(
		Event,
		FGuid(0xAA060022, 0, 0, 1),
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("recovery invokes only rejected visual executor"),
		Recovered.IsComplete()
			&& Recovered.Visual.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed
			&& Recovered.Audio.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed
			&& VisualExecutor.InvocationCount == 2
			&& AudioExecutor.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueHostNoOpTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost.DualNoOp",
	SwordRhythmCueHostFlags)

bool Fdemo_mapSwordRhythmCueHostNoOpTest::RunTest(const FString&)
{
	FSwordRhythmCueHostFixture Fixture(HostRun, HostWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent EmptyEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Host;
	TestTrue(TEXT("real zero-command event and host are ready"),
		Fixture.bReady
			&& Fixture.TryExecute(EmptyEvent)
			&& EmptyEvent.NumCommands() == 0
			&& TryMakeHost(Host));
	FHostFakeExecutor VisualExecutor(0xAA043000);
	FHostFakeExecutor AudioExecutor(0xAA053000);
	VisualExecutor.Mode = EHostExecutorMode::Rejected;
	AudioExecutor.Mode = EHostExecutorMode::Rejected;
	const FGuid VisualAttempt(0xAA060031, 0, 0, 1);
	const FGuid AudioAttempt(0xAA070031, 0, 0, 1);
	const auto NoOp = Host.Process(
		EmptyEvent,
		VisualAttempt,
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	const auto Replay = Host.Process(
		EmptyEvent,
		VisualAttempt,
		AudioAttempt,
		VisualExecutor,
		AudioExecutor);
	const auto Already = Host.Process(
		EmptyEvent,
		FGuid(0xAA060032, 0, 0, 1),
		FGuid(0xAA070032, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("both no-op consumers complete without executor calls"),
		NoOp.IsComplete()
			&& NoOp.Visual.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpExecuted
			&& NoOp.Audio.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpExecuted
			&& Replay.IsComplete()
			&& Already.IsComplete()
			&& Already.Visual.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					AlreadyAcknowledged
			&& Already.Audio.Status
				== Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
					AlreadyAcknowledged
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueHostFenceLifecycleTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost.FenceLifecycle",
	SwordRhythmCueHostFlags)

bool Fdemo_mapSwordRhythmCueHostFenceLifecycleTest::RunTest(const FString&)
{
	FSwordRhythmCueHostFixture Fixture(HostRun, HostWeapon);
	FSwordRhythmCueHostFixture Foreign(ForeignRun, ForeignWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent ForeignEvent;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Host;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Invalid;
	TestTrue(TEXT("host identity fences reject invalid creation"),
		!Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryCreate(
			FGuid(), VisualConsumer, AudioConsumer, Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryCreate(
				HostRun, VisualConsumer, VisualConsumer, Invalid));
	TestTrue(TEXT("local foreign events and host are ready"),
		Fixture.bReady
			&& Foreign.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& Foreign.TryMakeEffectEvent(ForeignEvent)
			&& TryMakeHost(Host));
	FHostFakeExecutor VisualExecutor(0xAA044000);
	FHostFakeExecutor AudioExecutor(0xAA054000);
	const auto InvalidEvent = Host.Process(
		Fdemo_mapShanmenSwordRhythmEffectCueEvent(),
		FGuid(0xAA060041, 0, 0, 1),
		FGuid(0xAA070041, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	const auto InvalidAttempt = Host.Process(
		Event,
		FGuid(),
		FGuid(0xAA070042, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	const auto CrossRun = Host.Process(
		ForeignEvent,
		FGuid(0xAA060043, 0, 0, 1),
		FGuid(0xAA070043, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("event attempt and Run fences invoke neither executor"),
		InvalidEvent.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::EventInvalid
			&& InvalidAttempt.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::AttemptInvalid
			&& CrossRun.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::ConsumerRejected
			&& CrossRun.Visual.Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::RunMismatch
			&& CrossRun.Audio.Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::RunMismatch
			&& VisualExecutor.InvocationCount == 0
			&& AudioExecutor.InvocationCount == 0);
	TestTrue(TEXT("source ProductSession teardown succeeds"),
		Fixture.Session.TryEnd(HostRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty());
	const auto AfterTeardown = Host.Process(
		Event,
		FGuid(0xAA060044, 0, 0, 1),
		FGuid(0xAA070044, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("frozen event completes after source teardown"),
		AfterTeardown.IsComplete()
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);
	FString EndDiagnostic;
	TestTrue(TEXT("wrong Run cannot end host"),
		!Host.TryEnd(ForeignRun, EndDiagnostic) && Host.IsValid());
	TestTrue(TEXT("matching Run ends and empties host"),
		Host.TryEnd(HostRun, EndDiagnostic) && Host.IsEmpty());
	const auto AfterEnd = Host.Process(
		Event,
		FGuid(0xAA060045, 0, 0, 1),
		FGuid(0xAA070045, 0, 0, 1),
		VisualExecutor,
		AudioExecutor);
	TestTrue(TEXT("ended host rejects before executor"),
		AfterEnd.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::HostInvalid
			&& VisualExecutor.InvocationCount == 1
			&& AudioExecutor.InvocationCount == 1);
	return true;
}

#endif
