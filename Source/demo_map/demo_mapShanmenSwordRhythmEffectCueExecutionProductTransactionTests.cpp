#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductTransactionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xAF000001, 0xAF000002, 0xAF000003, 0xAF000004);
	const FGuid TestWeapon(
		0xAF010001, 0xAF010002, 0xAF010003, 0xAF010004);

	using FProjection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection;
	using FCapture =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;
	using FCaptureResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureResult;

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

	struct FSwordRhythmCueProductTransactionFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductTransactionFixture()
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

		bool TryExecuteProjection(FProjection& OutProjection)
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
			const auto Captured =
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
					CaptureCurrent(Session);
			if (!Captured.IsCaptured())
			{
				Diagnostic = Captured.Diagnostic;
				return false;
			}
			OutProjection = Captured.Projection;
			return true;
		}

		bool TryMakeEffectProjection(FProjection& OutProjection)
		{
			FProjection Discarded;
			return TryExecuteProjection(Discarded)
				&& TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				&& TryExecuteProjection(Discarded)
				&& TryExecuteProjection(OutProjection)
				&& OutProjection.GetEvent().NumCommands() == 2;
		}

		FCaptureResult CaptureRequest(const uint32 Seed)
		{
			FProjection Projection;
			if (!bReady || !TryMakeEffectProjection(Projection))
			{
				return FCaptureResult();
			}
			return
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
					Capture(Projection, MakeIdentity(Seed));
		}
	};

	enum class EFakeMode : uint8
	{
		Success,
		RetryableFailure,
		Rejected
	};

	class FProductTransactionFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		FProductTransactionFakeExecutor(
			const uint32 InReceiptSeed,
			const EFakeMode InMode = EFakeMode::Success)
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
			if (Mode == EFakeMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Product-transaction fake rejected the invocation.");
				return Result;
			}

			const auto Outcome = Mode == EFakeMode::Success
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
					"Product-transaction fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Product-transaction fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
		EFakeMode Mode = EFakeMode::Success;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductTransactionCompletedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction.CompletedLifecycle",
	SwordRhythmCueProductTransactionFlags)

bool Fdemo_mapSwordRhythmCueProductTransactionCompletedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductTransactionFixture Fixture;
	const auto Captured = Fixture.CaptureRequest(0xAF100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductTransactionFakeExecutor Visual(0xAF200000);
	FProductTransactionFakeExecutor Audio(0xAF300000);
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(Captured.Request, Host, Visual, Audio);
	TestTrue(TEXT("single projection completes all three frozen commands"),
		Captured.IsCaptured()
			&& Completed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Completed
			&& Completed.IsCompleted()
			&& Completed.HasDurableProgress()
			&& Host.IsTerminal()
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductTransactionReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction.TerminalReplay",
	SwordRhythmCueProductTransactionFlags)

bool Fdemo_mapSwordRhythmCueProductTransactionReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductTransactionFixture Fixture;
	const auto Captured = Fixture.CaptureRequest(0xAF110000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductTransactionFakeExecutor Visual(0xAF210000);
	FProductTransactionFakeExecutor Audio(0xAF310000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(Captured.Request, Host, Visual, Audio);
	const int32 VisualBeforeReplay = Visual.InvocationCount;
	const int32 AudioBeforeReplay = Audio.InvocationCount;
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(Captured.Request, Host, Visual, Audio);
	TestTrue(TEXT("terminal replay is inert and preserves exact evidence"),
		Captured.IsCaptured()
			&& First.IsCompleted()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Replayed
			&& Replay.IsCompleted()
			&& Replay.bAllReplay
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductTransactionResumeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction.ResumeFromCreate",
	SwordRhythmCueProductTransactionFlags)

bool Fdemo_mapSwordRhythmCueProductTransactionResumeTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductTransactionFixture Fixture;
	const auto Captured = Fixture.CaptureRequest(0xAF120000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Captured.Request.GetCreateRequest(), Host);
	FProductTransactionFakeExecutor Visual(0xAF220000);
	FProductTransactionFakeExecutor Audio(0xAF320000);
	const auto Resumed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(Captured.Request, Host, Visual, Audio);
	TestTrue(TEXT("transaction resumes after its exact Create is durable"),
		Captured.IsCaptured()
			&& Created.IsSuccess()
			&& !Created.bReplay
			&& Resumed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Resumed
			&& Resumed.IsCompleted()
			&& Resumed.Create.bReplay
			&& !Resumed.Process.bReplay
			&& !Resumed.End.bReplay
			&& Host.IsTerminal()
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductTransactionFailureFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction.RetryAndRejectFence",
	SwordRhythmCueProductTransactionFlags)

bool Fdemo_mapSwordRhythmCueProductTransactionFailureFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductTransactionFixture RetryFixture;
	const auto RetryCaptured = RetryFixture.CaptureRequest(0xAF130000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RetryHost;
	FProductTransactionFakeExecutor RetryVisual(0xAF230000);
	FProductTransactionFakeExecutor RetryAudio(
		0xAF330000, EFakeMode::RetryableFailure);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(
				RetryCaptured.Request,
				RetryHost,
				RetryVisual,
				RetryAudio);
	const int32 RetryVisualBeforeReplay = RetryVisual.InvocationCount;
	const int32 RetryAudioBeforeReplay = RetryAudio.InvocationCount;
	const auto RetryReplay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(
				RetryCaptured.Request,
				RetryHost,
				RetryVisual,
				RetryAudio);
	TestTrue(TEXT("retry consumes Process sequence and never auto-runs End"),
		RetryCaptured.IsCaptured()
			&& Retry.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& Retry.HasDurableProgress()
			&& RetryReplay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& RetryReplay.Process.bReplay
			&& RetryHost.GetRecordCount() == 2
			&& RetryHost.GetNextSequence() == 2
			&& !RetryHost.IsTerminal()
			&& RetryVisual.InvocationCount == RetryVisualBeforeReplay
			&& RetryAudio.InvocationCount == RetryAudioBeforeReplay);

	FSwordRhythmCueProductTransactionFixture RejectFixture;
	const auto RejectCaptured = RejectFixture.CaptureRequest(0xAF140000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RejectHost;
	FProductTransactionFakeExecutor RejectVisual(
		0xAF240000, EFakeMode::Rejected);
	FProductTransactionFakeExecutor RejectAudio(0xAF340000);
	const auto Rejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(
				RejectCaptured.Request,
				RejectHost,
				RejectVisual,
				RejectAudio);
	const int32 RejectVisualBeforeReplay = RejectVisual.InvocationCount;
	const int32 RejectAudioBeforeReplay = RejectAudio.InvocationCount;
	const auto RejectReplay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(
				RejectCaptured.Request,
				RejectHost,
				RejectVisual,
				RejectAudio);
	TestTrue(TEXT("reject is durable and exact replay cannot re-enter executors"),
		RejectCaptured.IsCaptured()
			&& Rejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRejected
			&& Rejected.HasDurableProgress()
			&& RejectReplay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRejected
			&& RejectReplay.Process.bReplay
			&& RejectHost.GetRecordCount() == 2
			&& RejectHost.GetNextSequence() == 2
			&& !RejectHost.IsTerminal()
			&& RejectVisual.InvocationCount == RejectVisualBeforeReplay
			&& RejectAudio.InvocationCount == RejectAudioBeforeReplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductTransactionCaptureFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction.CaptureAndForeignFence",
	SwordRhythmCueProductTransactionFlags)

bool Fdemo_mapSwordRhythmCueProductTransactionCaptureFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductTransactionFixture Fixture;
	FProjection Projection;
	TestTrue(TEXT("one product projection is ready"),
		Fixture.bReady && Fixture.TryMakeEffectProjection(Projection));
	const FCapture ValidIdentity = MakeIdentity(0xAF150000);
	const auto Primary =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, ValidIdentity);
	const auto Repeated =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, ValidIdentity);
	FCapture BadSequence = ValidIdentity;
	BadSequence.ProcessSequence = 3;
	FCapture CommandAlias = ValidIdentity;
	CommandAlias.EndCommandId = CommandAlias.CreateCommandId;
	FCapture ConsumerAlias = ValidIdentity;
	ConsumerAlias.AudioConsumerId = ConsumerAlias.VisualConsumerId;
	FCapture AttemptAlias = ValidIdentity;
	AttemptAlias.AudioAttemptId = AttemptAlias.VisualAttemptId;
	const auto BadSequenceResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, BadSequence);
	const auto CommandAliasResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, CommandAlias);
	const auto ConsumerAliasResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, ConsumerAlias);
	const auto AttemptAliasResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, AttemptAlias);
	const auto EmptyProjectionResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(FProjection(), ValidIdentity);
	TestTrue(TEXT("capture freezes deterministic exact identities and fences aliases"),
		Primary.IsCaptured()
			&& Repeated.IsCaptured()
			&& Primary.Request.Matches(Repeated.Request)
			&& BadSequenceResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
					IdentityInvalid
			&& CommandAliasResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
					IdentityInvalid
			&& ConsumerAliasResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
					IdentityInvalid
			&& AttemptAliasResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
					IdentityInvalid
			&& EmptyProjectionResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
					ProjectionInvalid);

	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(Projection, MakeIdentity(0xAF160000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto ForeignCreated =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Foreign.Request.GetCreateRequest(), Host);
	FProductTransactionFakeExecutor Visual(0xAF250000);
	FProductTransactionFakeExecutor Audio(0xAF350000);
	const auto Rejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(Primary.Request, Host, Visual, Audio);
	TestTrue(TEXT("foreign sequence-zero evidence rejects before executor entry"),
		Foreign.IsCaptured()
			&& ForeignCreated.IsSuccess()
			&& Rejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					CreateRejected
			&& Host.GetRecordCount() == 1
			&& Visual.InvocationCount == 0
			&& Audio.InvocationCount == 0);
	return true;
}

#endif
