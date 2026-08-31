#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductPreparedDispatchFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB3000001, 0xB3000002, 0xB3000003, 0xB3000004);
	const FGuid TestWeapon(
		0xB3010001, 0xB3010002, 0xB3010003, 0xB3010004);

	using FSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;

	FSeed MakeSeed(const uint32 Value)
	{
		FSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	struct FSwordRhythmCueProductPreparedDispatchFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductPreparedDispatchFixture()
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
			return bReady
				&& TryExecute()
				&& TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				&& TryExecute()
				&& TryExecute();
		}

		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
		CaptureCurrent() const
		{
			return
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
					CaptureCurrent(Session);
		}
	};

	enum class EPreparedDispatchFakeMode : uint8
	{
		Success,
		RetryableFailure
	};

	class FProductPreparedDispatchFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		FProductPreparedDispatchFakeExecutor(
			const uint32 InReceiptSeed,
			const EPreparedDispatchFakeMode InMode =
				EPreparedDispatchFakeMode::Success)
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
			const auto Outcome = Mode == EPreparedDispatchFakeMode::Success
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
					"Prepared-dispatch fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Prepared-dispatch fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
		EPreparedDispatchFakeMode Mode = EPreparedDispatchFakeMode::Success;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedDispatchPrepareTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch.PrepareCurrentImmutableValue",
	SwordRhythmCueProductPreparedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedDispatchPrepareTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedDispatchFixture Fixture;
	const FSeed Seed = MakeSeed(0xB3100000);
	const auto Empty =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, Seed);
	TestTrue(TEXT("unobserved current state cannot produce a prepared value"),
		Empty.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
					ProjectionRejected
			&& !Empty.IsPrepared()
			&& !Empty.Prepared.IsValid());
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());

	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, Seed);
	const auto Second =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, Seed);
	const auto Request =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(
				First.Prepared.GetProjection(),
				First.Prepared.GetPlan().GetTransactionIdentity());
	TestTrue(TEXT("same current state and seed reproduce one immutable value"),
		First.IsPrepared()
			&& Second.IsPrepared()
			&& First.Prepared.Matches(Second.Prepared)
			&& First.Prepared.MatchesCurrentSession(Fixture.Session)
			&& First.Prepared.GetPlan().MatchesProjection(
				First.Prepared.GetProjection())
			&& Request.IsCaptured()
			&& First.Prepared.GetPlan().MatchesRequest(Request.Request));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedDispatchDelayedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch.DelayedAfterSourceAdvance",
	SwordRhythmCueProductPreparedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedDispatchDelayedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedDispatchFixture Fixture;
	TestTrue(TEXT("first current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Prepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeSeed(0xB3200000));
	TestTrue(TEXT("source advances after preparation"),
		Prepared.IsPrepared()
			&& Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecute());
	const auto Current = Fixture.CaptureCurrent();

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedDispatchFakeExecutor Visual(0xB3210000);
	FProductPreparedDispatchFakeExecutor Audio(0xB3220000);
	const auto Delayed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Prepared.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("prepared value is independent from later live state"),
		Current.IsCaptured()
			&& !Prepared.Prepared.MatchesCurrentSession(Fixture.Session)
			&& !Prepared.Prepared.GetProjection().Matches(Current.Projection));
	TestTrue(TEXT("delayed dispatch consumes only the frozen value"),
		Delayed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					Dispatched
			&& Delayed.IsSuccess()
			&& Delayed.Dispatch.TransactionCapture.Request.GetProjection().Matches(
				Prepared.Prepared.GetProjection())
			&& Host.IsTerminal()
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedDispatchReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch.ReplayAndForeignHostFence",
	SwordRhythmCueProductPreparedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedDispatchReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Prepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeSeed(0xB3300000));
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeSeed(0xB3300100));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedDispatchFakeExecutor Visual(0xB3310000);
	FProductPreparedDispatchFakeExecutor Audio(0xB3320000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Prepared.Prepared, Host, Visual, Audio);
	const int32 VisualBeforeReplay = Visual.InvocationCount;
	const int32 AudioBeforeReplay = Audio.InvocationCount;
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Prepared.Prepared, Host, Visual, Audio);
	const auto ForeignAttempt =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Foreign.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("exact prepared replay avoids executor re-entry"),
		Prepared.IsPrepared()
			&& Foreign.IsPrepared()
			&& First.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					Replayed
			&& Replay.IsSuccess()
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);
	TestTrue(TEXT("foreign prepared identity cannot alias a bound Host"),
		ForeignAttempt.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					DispatchIncomplete
			&& ForeignAttempt.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					CreateRejected
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedDispatchResumeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch.ResumeExistingCreate",
	SwordRhythmCueProductPreparedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedDispatchResumeTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Prepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeSeed(0xB3400000));
	const auto Transaction =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(
				Prepared.Prepared.GetProjection(),
				Prepared.Prepared.GetPlan().GetTransactionIdentity());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Transaction.Request.GetCreateRequest(), Host);
	FProductPreparedDispatchFakeExecutor Visual(0xB3410000);
	FProductPreparedDispatchFakeExecutor Audio(0xB3420000);
	const auto Resumed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Prepared.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("prepared value resumes its exact durable Create"),
		Prepared.IsPrepared()
			&& Transaction.IsCaptured()
			&& Created.IsSuccess()
			&& Resumed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					Resumed
			&& Resumed.IsSuccess()
			&& Resumed.Dispatch.Transaction.Create.bReplay
			&& Host.IsTerminal()
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedDispatchFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch.InvalidAndRetryFence",
	SwordRhythmCueProductPreparedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedDispatchFenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
		InvalidPrepared;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost InvalidHost;
	FProductPreparedDispatchFakeExecutor InvalidVisual(0xB3500000);
	FProductPreparedDispatchFakeExecutor InvalidAudio(0xB3510000);
	const auto Invalid =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				InvalidPrepared, InvalidHost, InvalidVisual, InvalidAudio);
	TestTrue(TEXT("invalid prepared value rejects before Host and executors"),
		Invalid.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					PreparedRejected
			&& InvalidHost.IsEmpty()
			&& InvalidVisual.InvocationCount == 0
			&& InvalidAudio.InvocationCount == 0);

	FSwordRhythmCueProductPreparedDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Prepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeSeed(0xB3520000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RetryHost;
	FProductPreparedDispatchFakeExecutor RetryVisual(0xB3530000);
	FProductPreparedDispatchFakeExecutor RetryAudio(
		0xB3540000, EPreparedDispatchFakeMode::RetryableFailure);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Prepared.Prepared, RetryHost, RetryVisual, RetryAudio);
	const int32 VisualBeforeRetry = RetryVisual.InvocationCount;
	const int32 AudioBeforeRetry = RetryAudio.InvocationCount;
	const auto ReplayPending =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Prepared.Prepared, RetryHost, RetryVisual, RetryAudio);
	TestTrue(TEXT("retry-pending evidence remains caller-driven and bounded"),
		Prepared.IsPrepared()
			&& First.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					DispatchIncomplete
			&& First.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& ReplayPending.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					DispatchIncomplete
			&& ReplayPending.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& RetryHost.GetRecordCount() == 2
			&& RetryHost.GetNextSequence() == 2
			&& !RetryHost.IsTerminal()
			&& RetryVisual.InvocationCount == VisualBeforeRetry
			&& RetryAudio.InvocationCount == AudioBeforeRetry);
	return true;
}

#endif
