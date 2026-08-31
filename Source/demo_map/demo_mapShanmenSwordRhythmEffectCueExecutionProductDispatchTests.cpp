#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductDispatchFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB0000001, 0xB0000002, 0xB0000003, 0xB0000004);
	const FGuid TestWeapon(
		0xB0010001, 0xB0010002, 0xB0010003, 0xB0010004);

	using FCapture =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;

	FCapture MakeIdentity(const uint32 Seed)
	{
		FCapture Identity;
		Identity.HostId = FGuid(Seed + 1, 0, 0, 1);
		Identity.CreateCommandId = FGuid(Seed + 2, 0, 0, 1);
		Identity.ProcessCommandId = FGuid(Seed + 3, 0, 0, 1);
		Identity.EndCommandId = FGuid(Seed + 4, 0, 0, 1);
		Identity.VisualConsumerId = FGuid(Seed + 5, 0, 0, 1);
		Identity.AudioConsumerId = FGuid(Seed + 6, 0, 0, 1);
		Identity.VisualAttemptId = FGuid(Seed + 7, 0, 0, 1);
		Identity.AudioAttemptId = FGuid(Seed + 8, 0, 0, 1);
		return Identity;
	}

	struct FSwordRhythmCueProductDispatchFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductDispatchFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn ? NewObject<UBoxComponent>(Pawn) : nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(Pawn)
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					TestRun, Pawn, Health, Diagnostic)
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

		bool TryExecute()
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				TestWeapon, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			return Action.IsExecuted()
				&& Timeline.TryCapture(Sample)
				&& Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic);
		}

		bool TryPrepareCurrentEffect()
		{
			if (!bReady
				|| !TryExecute()
				|| !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				|| !TryExecute()
				|| !TryExecute())
			{
				return false;
			}
			const auto Projection =
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
					CaptureCurrent(Session);
			return Projection.IsCaptured()
				&& Projection.Projection.GetEvent().NumCommands() == 2;
		}
	};

	enum class EDispatchFakeMode : uint8
	{
		Success,
		RetryableFailure
	};

	class FProductDispatchFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		FProductDispatchFakeExecutor(
			const uint32 InReceiptSeed,
			const EDispatchFakeMode InMode = EDispatchFakeMode::Success)
			: ReceiptSeed(InReceiptSeed)
			, Mode(InMode)
		{
		}

		int32 InvocationCount = 0;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			const auto Outcome = Mode == EDispatchFakeMode::Success
				? Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded
				: Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure;
			if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
					Invocation,
					FGuid(ReceiptSeed + InvocationCount, 0, 0, 1),
					Outcome,
					Result.Receipt))
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Product-dispatch fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Product-dispatch fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
		EDispatchFakeMode Mode = EDispatchFakeMode::Success;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchCompletedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch.CompletedCurrent",
	SwordRhythmCueProductDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchCompletedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchFixture Fixture;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductDispatchFakeExecutor Visual(0xB0200000);
	FProductDispatchFakeExecutor Audio(0xB0300000);
	const auto Dispatched =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				MakeIdentity(0xB0100000),
				Host,
				Visual,
				Audio);
	TestTrue(TEXT("unobserved current state rejects before transaction"),
		Dispatched.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					ProjectionRejected
			&& Host.IsEmpty()
			&& Visual.InvocationCount == 0
			&& Audio.InvocationCount == 0);
	TestTrue(TEXT("current effect becomes dispatchable"),
		Fixture.TryPrepareCurrentEffect());

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost ReadyHost;
	FProductDispatchFakeExecutor ReadyVisual(0xB0210000);
	FProductDispatchFakeExecutor ReadyAudio(0xB0310000);
	const auto Ready =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				MakeIdentity(0xB0110000),
				ReadyHost,
				ReadyVisual,
				ReadyAudio);
	TestTrue(TEXT("prepared current state dispatches through exact transaction"),
		Ready.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Dispatched
			&& Ready.IsSuccess()
			&& Ready.TransactionCapture.IsCaptured()
			&& ReadyHost.IsTerminal()
			&& ReadyHost.GetRecordCount() == 3
			&& ReadyVisual.InvocationCount == 1
			&& ReadyAudio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch.IdempotentCurrentReplay",
	SwordRhythmCueProductDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FCapture Identity = MakeIdentity(0xB0120000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductDispatchFakeExecutor Visual(0xB0220000);
	FProductDispatchFakeExecutor Audio(0xB0320000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session, Identity, Host, Visual, Audio);
	const int32 VisualBeforeReplay = Visual.InvocationCount;
	const int32 AudioBeforeReplay = Audio.InvocationCount;
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session, Identity, Host, Visual, Audio);
	TestTrue(TEXT("unchanged current state captures the exact terminal replay"),
		First.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Replayed
			&& Replay.IsSuccess()
			&& First.TransactionCapture.Request.Matches(
				Replay.TransactionCapture.Request)
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchResumeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch.ResumeExistingCreate",
	SwordRhythmCueProductDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchResumeTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FCapture Identity = MakeIdentity(0xB0130000);
	const auto Projection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
			CaptureCurrent(Fixture.Session);
	const auto Captured =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection.Projection, Identity);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Captured.Request.GetCreateRequest(), Host);
	FProductDispatchFakeExecutor Visual(0xB0230000);
	FProductDispatchFakeExecutor Audio(0xB0330000);
	const auto Resumed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session, Identity, Host, Visual, Audio);
	TestTrue(TEXT("current dispatch resumes from exact durable Create"),
		Projection.IsCaptured()
			&& Captured.IsCaptured()
			&& Created.IsSuccess()
			&& Resumed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Resumed
			&& Resumed.IsSuccess()
			&& Resumed.Transaction.Create.bReplay
			&& Host.IsTerminal()
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchAdvanceFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch.SourceAdvanceFence",
	SwordRhythmCueProductDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchAdvanceFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FCapture Identity = MakeIdentity(0xB0140000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductDispatchFakeExecutor Visual(0xB0240000);
	FProductDispatchFakeExecutor Audio(0xB0340000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session, Identity, Host, Visual, Audio);
	const int32 VisualBeforeAdvance = Visual.InvocationCount;
	const int32 AudioBeforeAdvance = Audio.InvocationCount;
	TestTrue(TEXT("source ProductSession advances after frozen dispatch"),
		Fixture.TryAdvanceTicks(1) && Fixture.TryExecute());
	const auto Advanced =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session, Identity, Host, Visual, Audio);
	const auto Historical =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(
				First.TransactionCapture.Request,
				Host,
				Visual,
				Audio);
	TestTrue(TEXT("new current payload conflicts while retained request replays"),
		First.IsSuccess()
			&& Advanced.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					TransactionIncomplete
			&& Advanced.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					CreateRejected
			&& Historical.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Replayed
			&& Historical.IsCompleted()
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeAdvance
			&& Audio.InvocationCount == AudioBeforeAdvance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchFailureFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch.ProjectionCaptureAndProcessFence",
	SwordRhythmCueProductDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchFailureFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchFixture EmptyFixture;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FProductDispatchFakeExecutor EmptyVisual(0xB0250000);
	FProductDispatchFakeExecutor EmptyAudio(0xB0350000);
	const auto Empty =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				EmptyFixture.Session,
				MakeIdentity(0xB0150000),
				EmptyHost,
				EmptyVisual,
				EmptyAudio);
	TestTrue(TEXT("unobserved ProductSession rejects before capture"),
		Empty.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					ProjectionRejected
			&& EmptyHost.IsEmpty()
			&& EmptyVisual.InvocationCount == 0
			&& EmptyAudio.InvocationCount == 0);

	FSwordRhythmCueProductDispatchFixture CaptureFixture;
	TestTrue(TEXT("capture fixture current effect is prepared"),
		CaptureFixture.TryPrepareCurrentEffect());
	FCapture InvalidIdentity = MakeIdentity(0xB0160000);
	InvalidIdentity.EndCommandId = InvalidIdentity.CreateCommandId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost CaptureHost;
	FProductDispatchFakeExecutor CaptureVisual(0xB0260000);
	FProductDispatchFakeExecutor CaptureAudio(0xB0360000);
	const auto CaptureRejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				CaptureFixture.Session,
				InvalidIdentity,
				CaptureHost,
				CaptureVisual,
				CaptureAudio);
	TestTrue(TEXT("invalid transaction identity rejects before Host/executors"),
		CaptureRejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					CaptureRejected
			&& CaptureHost.IsEmpty()
			&& CaptureVisual.InvocationCount == 0
			&& CaptureAudio.InvocationCount == 0);

	FSwordRhythmCueProductDispatchFixture RetryFixture;
	TestTrue(TEXT("retry fixture current effect is prepared"),
		RetryFixture.TryPrepareCurrentEffect());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RetryHost;
	FProductDispatchFakeExecutor RetryVisual(0xB0270000);
	FProductDispatchFakeExecutor RetryAudio(
		0xB0370000, EDispatchFakeMode::RetryableFailure);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				RetryFixture.Session,
				MakeIdentity(0xB0170000),
				RetryHost,
				RetryVisual,
				RetryAudio);
	TestTrue(TEXT("retry propagates without hidden End or retry loop"),
		Retry.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					TransactionIncomplete
			&& Retry.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& Retry.Transaction.HasDurableProgress()
			&& RetryHost.GetRecordCount() == 2
			&& RetryHost.GetNextSequence() == 2
			&& !RetryHost.IsTerminal()
			&& RetryVisual.InvocationCount == 1
			&& RetryAudio.InvocationCount == 1);
	return true;
}

#endif
