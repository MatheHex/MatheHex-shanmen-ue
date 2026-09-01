#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductRetryDecisionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB5000001, 0xB5000002, 0xB5000003, 0xB5000004);
	const FGuid TestWeapon(
		0xB5010001, 0xB5010002, 0xB5010003, 0xB5010004);

	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionPolicy =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionPolicy;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using EDecisionOutcome =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome;
	using EDecisionStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionStatus;
	using EPrepareStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus;

	FDispatchSeed MakeDispatchSeed(const uint32 Value)
	{
		FDispatchSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	FDecisionRequest MakeDecisionRequest(
		const uint32 Value,
		const int32 MaxRenewals,
		const int32 RenewalsUsed)
	{
		FDecisionRequest Request;
		Request.Policy.PolicySeed = FGuid(Value, 0, 0, 1);
		Request.Policy.MaxRenewals = MaxRenewals;
		Request.RenewalsUsed = RenewalsUsed;
		return Request;
	}

	struct FSwordRhythmCueProductRetryDecisionFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductRetryDecisionFixture()
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
	};

	enum class ERetryDecisionFakeMode : uint8
	{
		Success,
		RetryableFailure,
		Rejected
	};

	class FProductRetryDecisionFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductRetryDecisionFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryDecisionFakeMode InMode =
				ERetryDecisionFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryDecisionFakeMode Mode = ERetryDecisionFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ERetryDecisionFakeMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Retry-decision fake rejected the invocation.");
				return Result;
			}
			const auto Outcome = Mode == ERetryDecisionFakeMode::Success
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
					"Retry-decision fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-decision fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareDispatch(
		FSwordRhythmCueProductRetryDecisionFixture& Fixture,
		const uint32 Seed)
	{
		if (!Fixture.TryPrepareCurrentEffect())
		{
			return {};
		}
		return
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
				PrepareCurrent(Fixture.Session, MakeDispatchSeed(Seed));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryDecisionRetryTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryDecision.RetryIsDeterministicAndCallerExecuted",
	SwordRhythmCueProductRetryDecisionFlags)

bool Fdemo_mapSwordRhythmCueProductRetryDecisionRetryTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryDecisionFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB5100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryDecisionFakeExecutor Visual(0xB5110000);
	FProductRetryDecisionFakeExecutor Audio(
		0xB5120000, ERetryDecisionFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto Request = MakeDecisionRequest(0xB5130000, 2, 0);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 VisualBefore = Visual.InvocationCount;
	const int32 AudioBefore = Audio.InvocationCount;
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(Dispatch.Prepared, Host, Request);
	const auto Same =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(Dispatch.Prepared, Host, Request);
	TestTrue(TEXT("same receipt and budget produce one inert Retry value"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& First.IsDecided()
			&& Same.IsDecided()
			&& First.Decision.Matches(Same.Decision)
			&& First.Decision.ShouldRetry()
			&& First.Decision.GetOutcome() == EDecisionOutcome::Retry
			&& First.Decision.GetNextRenewalsUsed() == 1
			&& First.Decision.GetPreparationStatus()
				== EPrepareStatus::Prepared
			&& First.Decision.GetPreparedRetry().IsValid()
			&& Host.GetRecordCount() == RecordsBefore
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore);

	Audio.Mode = ERetryDecisionFakeMode::Success;
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				First.Decision.GetPreparedRetry(), Host, Visual, Audio);
	TestTrue(TEXT("caller may explicitly execute the returned continuation"),
		Completed.IsCompleted()
			&& Host.IsTerminal()
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryDecisionBudgetTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryDecision.StopAtBudgetWithoutContinuation",
	SwordRhythmCueProductRetryDecisionFlags)

bool Fdemo_mapSwordRhythmCueProductRetryDecisionBudgetTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryDecisionFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB5200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryDecisionFakeExecutor Visual(0xB5210000);
	FProductRetryDecisionFakeExecutor Audio(
		0xB5220000, ERetryDecisionFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 AudioBefore = Audio.InvocationCount;
	const auto Stopped =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Dispatch.Prepared,
				Host,
				MakeDecisionRequest(0xB5230000, 1, 1));
	TestTrue(TEXT("valid retryable state stops without leaking executable work"),
		!Initial.IsSuccess()
			&& Stopped.IsDecided()
			&& !Stopped.Decision.ShouldRetry()
			&& Stopped.Decision.GetOutcome()
				== EDecisionOutcome::StopBudgetExhausted
			&& Stopped.Decision.GetPreparationStatus()
				== EPrepareStatus::Prepared
			&& Stopped.Decision.GetNextRenewalsUsed() == 1
			&& !Stopped.Decision.GetPreparedRetry().IsValid()
			&& Host.GetRecordCount() == RecordsBefore
			&& Audio.InvocationCount == AudioBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryDecisionRepeatedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryDecision.RepeatedCallerBudget",
	SwordRhythmCueProductRetryDecisionFlags)

bool Fdemo_mapSwordRhythmCueProductRetryDecisionRepeatedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryDecisionFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB5300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryDecisionFakeExecutor Visual(0xB5310000);
	FProductRetryDecisionFakeExecutor Audio(
		0xB5320000, ERetryDecisionFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Dispatch.Prepared,
				Host,
				MakeDecisionRequest(0xB5330000, 2, 0));
	const auto FirstExecution =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				First.Decision.GetPreparedRetry(), Host, Visual, Audio);
	const auto Second =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Dispatch.Prepared,
				Host,
				MakeDecisionRequest(
					0xB5330000,
					2,
					First.Decision.GetNextRenewalsUsed()));
	const auto SecondExecution =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				Second.Decision.GetPreparedRetry(), Host, Visual, Audio);
	const int32 RecordsBeforeStop = Host.GetRecordCount();
	const int32 AudioBeforeStop = Audio.InvocationCount;
	const auto Stop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Dispatch.Prepared,
				Host,
				MakeDecisionRequest(
					0xB5330000,
					2,
					Second.Decision.GetNextRenewalsUsed()));
	TestTrue(TEXT("caller advances explicit count and receives distinct renewals"),
		!Initial.IsSuccess()
			&& First.IsDecided()
			&& First.Decision.ShouldRetry()
			&& FirstExecution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
					RetryPending
			&& Second.IsDecided()
			&& Second.Decision.ShouldRetry()
			&& Second.Decision.GetRetrySeed().RetrySeed
				!= First.Decision.GetRetrySeed().RetrySeed
			&& Second.Decision.GetPreparedRetry().GetSourceProcessEnvelope().
				GetSequence() == 2
			&& SecondExecution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
					RetryPending
			&& Stop.IsDecided()
			&& Stop.Decision.GetOutcome()
				== EDecisionOutcome::StopBudgetExhausted
			&& Stop.Decision.GetNextRenewalsUsed() == 2
			&& !Stop.Decision.GetPreparedRetry().IsValid()
			&& Host.GetRecordCount() == RecordsBeforeStop
			&& Audio.InvocationCount == AudioBeforeStop);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryDecisionStopReasonsTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryDecision.CompletedAndNotRetryableStops",
	SwordRhythmCueProductRetryDecisionFlags)

bool Fdemo_mapSwordRhythmCueProductRetryDecisionStopReasonsTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryDecisionFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB5400000);
	const auto Request = MakeDecisionRequest(0xB5410000, 2, 0);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost TerminalHost;
	FProductRetryDecisionFakeExecutor TerminalVisual(0xB5420000);
	FProductRetryDecisionFakeExecutor TerminalAudio(0xB5430000);
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared,
				TerminalHost,
				TerminalVisual,
				TerminalAudio);
	const auto CompletedStop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(Dispatch.Prepared, TerminalHost, Request);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RejectedHost;
	FProductRetryDecisionFakeExecutor RejectedVisual(
		0xB5440000, ERetryDecisionFakeMode::Rejected);
	FProductRetryDecisionFakeExecutor RejectedAudio(0xB5450000);
	const auto Rejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared,
				RejectedHost,
				RejectedVisual,
				RejectedAudio);
	const auto RejectedStop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(Dispatch.Prepared, RejectedHost, Request);
	TestTrue(TEXT("terminal and durable non-retryable receipts have distinct Stops"),
		Completed.IsSuccess()
			&& TerminalHost.IsTerminal()
			&& CompletedStop.IsDecided()
			&& CompletedStop.Decision.GetOutcome()
				== EDecisionOutcome::StopCompleted
			&& CompletedStop.Decision.WasHostTerminal()
			&& !CompletedStop.Decision.GetPreparedRetry().IsValid()
			&& !Rejected.IsSuccess()
			&& RejectedHost.IsValid()
			&& !RejectedHost.IsTerminal()
			&& RejectedStop.IsDecided()
			&& RejectedStop.Decision.GetOutcome()
				== EDecisionOutcome::StopNotRetryable
			&& !RejectedStop.Decision.WasHostTerminal()
			&& !RejectedStop.Decision.GetPreparedRetry().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryDecisionFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryDecision.InvalidAndForeignFences",
	SwordRhythmCueProductRetryDecisionFlags)

bool Fdemo_mapSwordRhythmCueProductRetryDecisionFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryDecisionFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB5500000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB5510000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryDecisionFakeExecutor Visual(0xB5520000);
	FProductRetryDecisionFakeExecutor Audio(
		0xB5530000, ERetryDecisionFakeMode::RetryableFailure);
	const auto Pending =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 VisualBefore = Visual.InvocationCount;
	const int32 AudioBefore = Audio.InvocationCount;
	const auto InvalidRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(Dispatch.Prepared, Host, FDecisionRequest());
	const auto OverBudgetRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Dispatch.Prepared,
				Host,
				MakeDecisionRequest(0xB5540000, 1, 2));
	const auto InvalidPrepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				{}, Host, MakeDecisionRequest(0xB5550000, 1, 0));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	const auto InvalidHost =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Dispatch.Prepared,
				EmptyHost,
				MakeDecisionRequest(0xB5560000, 1, 0));
	const auto ForeignRoot =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(
				Foreign.Prepared,
				Host,
				MakeDecisionRequest(0xB5570000, 1, 0));
	TestTrue(TEXT("invalid inputs and foreign root reject before side effects"),
		!Pending.IsSuccess()
			&& InvalidRequest.Status == EDecisionStatus::RequestInvalid
			&& OverBudgetRequest.Status == EDecisionStatus::RequestInvalid
			&& InvalidPrepared.Status
				== EDecisionStatus::PreparedDispatchInvalid
			&& InvalidHost.Status == EDecisionStatus::HostInvalid
			&& ForeignRoot.Status == EDecisionStatus::PreparedRootMismatch
			&& !ForeignRoot.IsDecided()
			&& Host.GetRecordCount() == RecordsBefore
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore);
	return true;
}

#endif
