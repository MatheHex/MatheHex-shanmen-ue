#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductRetryStepFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB6000001, 0xB6000002, 0xB6000003, 0xB6000004);
	const FGuid TestWeapon(
		0xB6010001, 0xB6010002, 0xB6010003, 0xB6010004);

	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;

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

	struct FSwordRhythmCueProductRetryStepFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductRetryStepFixture()
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

	enum class ERetryStepFakeMode : uint8
	{
		Success,
		RetryableFailure,
		Rejected
	};

	class FProductRetryStepFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductRetryStepFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryStepFakeMode InMode = ERetryStepFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryStepFakeMode Mode = ERetryStepFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ERetryStepFakeMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Retry-step fake rejected the invocation.");
				return Result;
			}
			const auto Outcome = Mode == ERetryStepFakeMode::Success
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
					"Retry-step fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-step fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareDispatch(
		FSwordRhythmCueProductRetryStepFixture& Fixture,
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
	Fdemo_mapSwordRhythmCueProductRetryStepCompleteTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStep.CompleteOneShot",
	SwordRhythmCueProductRetryStepFlags)

bool Fdemo_mapSwordRhythmCueProductRetryStepCompleteTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryStepFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB6100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryStepFakeExecutor Visual(0xB6110000);
	FProductRetryStepFakeExecutor Audio(
		0xB6120000, ERetryStepFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 VisualBefore = Visual.InvocationCount;
	const int32 AudioBefore = Audio.InvocationCount;
	Audio.Mode = ERetryStepFakeMode::Success;
	const auto Step =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6130000, 2, 0));
	TestTrue(TEXT("one step decides once and completes one continuation"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& Step.Status == EStepStatus::Completed
			&& Step.IsHandled()
			&& Step.DidExecuteRetry()
			&& !Step.CanRequestAnotherStep()
			&& Step.NextRenewalsUsed == 1
			&& Step.FinalRecordCount == RecordsBefore + 2
			&& Step.FinalNextSequence == Step.FinalRecordCount
			&& Step.bFinalTerminal
			&& Host.IsTerminal()
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryStepPendingTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStep.PendingRequiresCallerNextStep",
	SwordRhythmCueProductRetryStepFlags)

bool Fdemo_mapSwordRhythmCueProductRetryStepPendingTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryStepFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB6200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryStepFakeExecutor Visual(0xB6210000);
	FProductRetryStepFakeExecutor Audio(
		0xB6220000, ERetryStepFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 AudioBefore = Audio.InvocationCount;
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6230000, 2, 0));
	const int32 RecordsAfterFirst = Host.GetRecordCount();
	const int32 AudioAfterFirst = Audio.InvocationCount;
	Audio.Mode = ERetryStepFakeMode::Success;
	const auto Second =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(
					0xB6230000, 2, First.NextRenewalsUsed));
	TestTrue(TEXT("pending work advances only after a second caller request"),
		!Initial.IsSuccess()
			&& First.Status == EStepStatus::RetryPending
			&& First.IsHandled()
			&& First.DidExecuteRetry()
			&& First.CanRequestAnotherStep()
			&& First.NextRenewalsUsed == 1
			&& !First.bFinalTerminal
			&& AudioAfterFirst == AudioBefore + 1
			&& Second.Status == EStepStatus::Completed
			&& Second.IsHandled()
			&& Second.NextRenewalsUsed == 2
			&& Second.bFinalTerminal
			&& Host.GetRecordCount() == RecordsAfterFirst + 2
			&& Audio.InvocationCount == AudioAfterFirst + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryStepStopTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStep.StopPathsAreInert",
	SwordRhythmCueProductRetryStepFlags)

bool Fdemo_mapSwordRhythmCueProductRetryStepStopTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryStepFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB6300000);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost BudgetHost;
	FProductRetryStepFakeExecutor BudgetVisual(0xB6310000);
	FProductRetryStepFakeExecutor BudgetAudio(
		0xB6320000, ERetryStepFakeMode::RetryableFailure);
	const auto BudgetInitial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared, BudgetHost, BudgetVisual, BudgetAudio);
	const int32 BudgetRecords = BudgetHost.GetRecordCount();
	const int32 BudgetCalls = BudgetAudio.InvocationCount;
	const auto BudgetStop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				BudgetHost,
				BudgetVisual,
				BudgetAudio,
				MakeDecisionRequest(0xB6330000, 1, 1));

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost CompleteHost;
	FProductRetryStepFakeExecutor CompleteVisual(0xB6340000);
	FProductRetryStepFakeExecutor CompleteAudio(0xB6350000);
	const auto CompleteInitial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared, CompleteHost, CompleteVisual, CompleteAudio);
	const int32 CompleteRecords = CompleteHost.GetRecordCount();
	const int32 CompleteCalls = CompleteAudio.InvocationCount;
	const auto CompleteStop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				CompleteHost,
				CompleteVisual,
				CompleteAudio,
				MakeDecisionRequest(0xB6360000, 2, 0));

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RejectedHost;
	FProductRetryStepFakeExecutor RejectedVisual(
		0xB6370000, ERetryStepFakeMode::Rejected);
	FProductRetryStepFakeExecutor RejectedAudio(0xB6380000);
	const auto RejectedInitial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared, RejectedHost, RejectedVisual, RejectedAudio);
	const int32 RejectedRecords = RejectedHost.GetRecordCount();
	const int32 RejectedCalls = RejectedVisual.InvocationCount;
	const auto RejectedStop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				RejectedHost,
				RejectedVisual,
				RejectedAudio,
				MakeDecisionRequest(0xB6390000, 2, 0));

	TestTrue(TEXT("all stop reasons preserve Host and executor state"),
		!BudgetInitial.IsSuccess()
			&& BudgetStop.Status == EStepStatus::StoppedBudgetExhausted
			&& BudgetStop.IsHandled()
			&& !BudgetStop.DidExecuteRetry()
			&& BudgetHost.GetRecordCount() == BudgetRecords
			&& BudgetAudio.InvocationCount == BudgetCalls
			&& CompleteInitial.IsSuccess()
			&& CompleteStop.Status == EStepStatus::StoppedCompleted
			&& CompleteStop.IsHandled()
			&& CompleteHost.GetRecordCount() == CompleteRecords
			&& CompleteAudio.InvocationCount == CompleteCalls
			&& !RejectedInitial.IsSuccess()
			&& RejectedStop.Status == EStepStatus::StoppedNotRetryable
			&& RejectedStop.IsHandled()
			&& RejectedHost.GetRecordCount() == RejectedRecords
			&& RejectedVisual.InvocationCount == RejectedCalls);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryStepRejectionTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStep.HardRejectionProducesDurableStop",
	SwordRhythmCueProductRetryStepFlags)

bool Fdemo_mapSwordRhythmCueProductRetryStepRejectionTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryStepFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB6400000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryStepFakeExecutor Visual(0xB6410000);
	FProductRetryStepFakeExecutor Audio(
		0xB6420000, ERetryStepFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 AudioBefore = Audio.InvocationCount;
	Audio.Mode = ERetryStepFakeMode::Rejected;
	const auto Rejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6430000, 2, 0));
	const int32 RecordsAfter = Host.GetRecordCount();
	const int32 AudioAfter = Audio.InvocationCount;
	const auto Stop =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(
					0xB6430000, 2, Rejected.NextRenewalsUsed));
	TestTrue(TEXT("hard rejection is durable and the next step stops"),
		!Initial.IsSuccess()
			&& Rejected.Status == EStepStatus::ExecutionRejected
			&& !Rejected.IsHandled()
			&& Rejected.DidExecuteRetry()
			&& Rejected.NextRenewalsUsed == 1
			&& Rejected.Execution.HasDurableProgress()
			&& RecordsAfter == RecordsBefore + 1
			&& AudioAfter == AudioBefore + 1
			&& Stop.Status == EStepStatus::StoppedNotRetryable
			&& Stop.IsHandled()
			&& Stop.NextRenewalsUsed == 1
			&& Host.GetRecordCount() == RecordsAfter
			&& Audio.InvocationCount == AudioAfter);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryStepFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStep.InvalidAndForeignFences",
	SwordRhythmCueProductRetryStepFlags)

bool Fdemo_mapSwordRhythmCueProductRetryStepFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryStepFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB6500000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB6510000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryStepFakeExecutor Visual(0xB6520000);
	FProductRetryStepFakeExecutor Audio(
		0xB6530000, ERetryStepFakeMode::RetryableFailure);
	const auto Pending =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 VisualBefore = Visual.InvocationCount;
	const int32 AudioBefore = Audio.InvocationCount;
	const auto InvalidRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				FDecisionRequest());
	const auto OverBudget =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6540000, 1, 2));
	const auto InvalidPrepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				{},
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6550000, 1, 0));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	const auto InvalidHost =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Dispatch.Prepared,
				EmptyHost,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6560000, 1, 0));
	const auto ForeignRoot =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Foreign.Prepared,
				Host,
				Visual,
				Audio,
				MakeDecisionRequest(0xB6570000, 1, 0));
	TestTrue(TEXT("invalid and foreign inputs reject before execution"),
		!Pending.IsSuccess()
			&& InvalidRequest.Status == EStepStatus::DecisionRejected
			&& OverBudget.Status == EStepStatus::DecisionRejected
			&& InvalidPrepared.Status == EStepStatus::DecisionRejected
			&& InvalidHost.Status == EStepStatus::DecisionRejected
			&& ForeignRoot.Status == EStepStatus::DecisionRejected
			&& !InvalidRequest.DidExecuteRetry()
			&& !ForeignRoot.DidExecuteRetry()
			&& Host.GetRecordCount() == RecordsBefore
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore);
	return true;
}

#endif
