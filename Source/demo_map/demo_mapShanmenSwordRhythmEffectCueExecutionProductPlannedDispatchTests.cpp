#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductPlannedDispatchFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB2000001, 0xB2000002, 0xB2000003, 0xB2000004);
	const FGuid TestWeapon(
		0xB2010001, 0xB2010002, 0xB2010003, 0xB2010004);

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

	struct FSwordRhythmCueProductPlannedDispatchFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductPlannedDispatchFixture()
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

	enum class EPlannedDispatchFakeMode : uint8
	{
		Success,
		RetryableFailure
	};

	class FProductPlannedDispatchFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		FProductPlannedDispatchFakeExecutor(
			const uint32 InReceiptSeed,
			const EPlannedDispatchFakeMode InMode =
				EPlannedDispatchFakeMode::Success)
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
			const auto Outcome = Mode == EPlannedDispatchFakeMode::Success
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
					"Planned-dispatch fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Planned-dispatch fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
		EPlannedDispatchFakeMode Mode = EPlannedDispatchFakeMode::Success;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPlannedDispatchCompletedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch.CompletedSingleRead",
	SwordRhythmCueProductPlannedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPlannedDispatchCompletedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPlannedDispatchFixture Fixture;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FProductPlannedDispatchFakeExecutor EmptyVisual(0xB2100000);
	FProductPlannedDispatchFakeExecutor EmptyAudio(0xB2110000);
	const auto Empty =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				MakeSeed(0xB2120000),
				EmptyHost,
				EmptyVisual,
				EmptyAudio);
	TestTrue(TEXT("unobserved current state rejects before plan and dispatch"),
		Empty.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					ProjectionRejected
			&& EmptyHost.IsEmpty()
			&& EmptyVisual.InvocationCount == 0
			&& EmptyAudio.InvocationCount == 0);
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPlannedDispatchFakeExecutor Visual(0xB2130000);
	FProductPlannedDispatchFakeExecutor Audio(0xB2140000);
	const auto Ready =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				MakeSeed(0xB2150000),
				Host,
				Visual,
				Audio);
	TestTrue(TEXT("one current projection becomes one planned dispatch"),
		Ready.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					Dispatched
			&& Ready.IsSuccess()
			&& Ready.PlanCapture.Plan.MatchesProjection(
				Ready.Projection.Projection)
			&& Ready.Dispatch.Projection.Projection.Matches(
				Ready.Projection.Projection)
			&& Host.IsTerminal()
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPlannedDispatchReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch.DeterministicReplay",
	SwordRhythmCueProductPlannedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPlannedDispatchReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPlannedDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FSeed Seed = MakeSeed(0xB2200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPlannedDispatchFakeExecutor Visual(0xB2210000);
	FProductPlannedDispatchFakeExecutor Audio(0xB2220000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session, Seed, Host, Visual, Audio);
	const int32 VisualBeforeReplay = Visual.InvocationCount;
	const int32 AudioBeforeReplay = Audio.InvocationCount;
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session, Seed, Host, Visual, Audio);
	TestTrue(TEXT("same current state and seed replay exact terminal plan"),
		First.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					Replayed
			&& Replay.IsSuccess()
			&& First.PlanCapture.Plan.Matches(Replay.PlanCapture.Plan)
			&& First.Dispatch.TransactionCapture.Request.Matches(
				Replay.Dispatch.TransactionCapture.Request)
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPlannedDispatchResumeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch.ResumeExistingCreate",
	SwordRhythmCueProductPlannedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPlannedDispatchResumeTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPlannedDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FSeed Seed = MakeSeed(0xB2300000);
	const auto Projection = Fixture.CaptureCurrent();
	const auto Plan =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, Seed);
	const auto Transaction =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(
				Projection.Projection,
				Plan.Plan.GetTransactionIdentity());
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Transaction.Request.GetCreateRequest(), Host);
	FProductPlannedDispatchFakeExecutor Visual(0xB2310000);
	FProductPlannedDispatchFakeExecutor Audio(0xB2320000);
	const auto Resumed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session, Seed, Host, Visual, Audio);
	TestTrue(TEXT("planned current dispatch resumes exact durable Create"),
		Projection.IsCaptured()
			&& Plan.IsCaptured()
			&& Transaction.IsCaptured()
			&& Created.IsSuccess()
			&& Resumed.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					Resumed
			&& Resumed.IsSuccess()
			&& Resumed.Dispatch.Transaction.Create.bReplay
			&& Host.IsTerminal()
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPlannedDispatchAdvanceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch.SourceAdvancePlanSeparation",
	SwordRhythmCueProductPlannedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPlannedDispatchAdvanceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPlannedDispatchFixture Fixture;
	TestTrue(TEXT("first current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FSeed Seed = MakeSeed(0xB2400000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost FirstHost;
	FProductPlannedDispatchFakeExecutor Visual(0xB2410000);
	FProductPlannedDispatchFakeExecutor Audio(0xB2420000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session, Seed, FirstHost, Visual, Audio);
	const int32 VisualBeforeAdvance = Visual.InvocationCount;
	const int32 AudioBeforeAdvance = Audio.InvocationCount;
	TestTrue(TEXT("source advances to a new presentation revision"),
		Fixture.TryAdvanceTicks(1) && Fixture.TryExecute());
	const auto ForeignForOldHost =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session, Seed, FirstHost, Visual, Audio);
	const int32 VisualAfterForeign = Visual.InvocationCount;
	const int32 AudioAfterForeign = Audio.InvocationCount;
	const auto Historical =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
			TryExecute(
				First.Dispatch.TransactionCapture.Request,
				FirstHost,
				Visual,
				Audio);
	const int32 VisualAfterHistorical = Visual.InvocationCount;
	const int32 AudioAfterHistorical = Audio.InvocationCount;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost SecondHost;
	const auto Second =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session, Seed, SecondHost, Visual, Audio);
	TestTrue(TEXT("first revision dispatch succeeds"), First.IsSuccess());
	TestTrue(TEXT("new revision is rejected by the completed old host"),
		ForeignForOldHost.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				DispatchIncomplete
			&& ForeignForOldHost.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					CreateRejected);
	TestTrue(TEXT("old host rejection does not invoke executors"),
		VisualBeforeAdvance == VisualAfterForeign
			&& AudioBeforeAdvance == AudioAfterForeign);
	TestTrue(TEXT("new revision derives a distinct plan"),
		!First.PlanCapture.Plan.Matches(
			ForeignForOldHost.PlanCapture.Plan));
	TestTrue(TEXT("retained request replays without executor invocation"),
		Historical.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Replayed
			&& Historical.IsCompleted()
			&& VisualAfterForeign == VisualAfterHistorical
			&& AudioAfterForeign == AudioAfterHistorical);
	TestTrue(TEXT("new revision dispatch succeeds on a fresh host"),
		Second.IsSuccess() && SecondHost.IsTerminal());
	TestTrue(TEXT("fresh-host revision retains explicit no-op evidence"),
		Second.Dispatch.Transaction.Process.Router.Session.Host.Visual.Execution.
			Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
				NoOpSucceeded
			&& Second.Dispatch.Transaction.Process.Router.Session.Host.Audio.
				Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					NoOpSucceeded);
	TestEqual(TEXT("no-op revision does not invoke visual executor"),
		Visual.InvocationCount, VisualBeforeAdvance);
	TestEqual(TEXT("no-op revision does not invoke audio executor"),
		Audio.InvocationCount, AudioBeforeAdvance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPlannedDispatchFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch.ProjectionSeedAndRetryFence",
	SwordRhythmCueProductPlannedDispatchFlags)

bool Fdemo_mapSwordRhythmCueProductPlannedDispatchFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPlannedDispatchFixture Fixture;
	TestTrue(TEXT("current effect is prepared"),
		Fixture.TryPrepareCurrentEffect());
	FSeed InvalidSeed = MakeSeed(0xB2500000);
	InvalidSeed.AudioConsumerScopeId = InvalidSeed.VisualConsumerScopeId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost InvalidSeedHost;
	FProductPlannedDispatchFakeExecutor InvalidSeedVisual(0xB2510000);
	FProductPlannedDispatchFakeExecutor InvalidSeedAudio(0xB2520000);
	const auto InvalidSeedResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				InvalidSeed,
				InvalidSeedHost,
				InvalidSeedVisual,
				InvalidSeedAudio);
	TestTrue(TEXT("invalid seed rejects before Host and executors"),
		InvalidSeedResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					PlanRejected
			&& InvalidSeedHost.IsEmpty()
			&& InvalidSeedVisual.InvocationCount == 0
			&& InvalidSeedAudio.InvocationCount == 0);

	const auto Projection = Fixture.CaptureCurrent();
	const auto Plan =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, MakeSeed(0xB2530000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost ProjectionHost;
	FProductPlannedDispatchFakeExecutor ProjectionVisual(0xB2540000);
	FProductPlannedDispatchFakeExecutor ProjectionAudio(0xB2550000);
	const auto InvalidProjection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchProjection(
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection(),
				Plan.Plan.GetTransactionIdentity(),
				ProjectionHost,
				ProjectionVisual,
				ProjectionAudio);
	TestTrue(TEXT("frozen dispatch rejects invalid projection before capture"),
		Projection.IsCaptured()
			&& Plan.IsCaptured()
			&& InvalidProjection.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					ProjectionRejected
			&& ProjectionHost.IsEmpty()
			&& ProjectionVisual.InvocationCount == 0
			&& ProjectionAudio.InvocationCount == 0);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RetryHost;
	FProductPlannedDispatchFakeExecutor RetryVisual(0xB2560000);
	FProductPlannedDispatchFakeExecutor RetryAudio(
		0xB2570000, EPlannedDispatchFakeMode::RetryableFailure);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				MakeSeed(0xB2580000),
				RetryHost,
				RetryVisual,
				RetryAudio);
	TestTrue(TEXT("retry propagates without hidden End or retry loop"),
		Retry.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					DispatchIncomplete
			&& Retry.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& RetryHost.GetRecordCount() == 2
			&& RetryHost.GetNextSequence() == 2
			&& !RetryHost.IsTerminal()
			&& RetryVisual.InvocationCount == 1
			&& RetryAudio.InvocationCount == 1);
	return true;
}

#endif
