#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductRetryCommandFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB7000001, 0xB7000002, 0xB7000003, 0xB7000004);
	const FGuid TestWeapon(
		0xB7010001, 0xB7010002, 0xB7010003, 0xB7010004);

	using FCommand =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand;
	using FCommandAdapter =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandAdapter;
	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using ECommandStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus;
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

	struct FSwordRhythmCueProductRetryCommandFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductRetryCommandFixture()
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

	enum class ERetryCommandFakeMode : uint8
	{
		Success,
		RetryableFailure,
		Rejected
	};

	class FProductRetryCommandFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductRetryCommandFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryCommandFakeMode InMode =
				ERetryCommandFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryCommandFakeMode Mode = ERetryCommandFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ERetryCommandFakeMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Retry-command fake rejected the invocation.");
				return Result;
			}
			const auto Outcome = Mode == ERetryCommandFakeMode::Success
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
					"Retry-command fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-command fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareDispatch(
		FSwordRhythmCueProductRetryCommandFixture& Fixture,
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
	Fdemo_mapSwordRhythmCueProductRetryCommandCompleteTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommand.HandledReceiptComplete",
	SwordRhythmCueProductRetryCommandFlags)

bool Fdemo_mapSwordRhythmCueProductRetryCommandCompleteTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryCommandFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB7100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryCommandFakeExecutor Visual(0xB7110000);
	FProductRetryCommandFakeExecutor Audio(
		0xB7120000, ERetryCommandFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand Command;
	const auto Request = MakeDecisionRequest(0xB7130000, 2, 0);
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xB7140001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		Request,
		Command);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 VisualBefore = Visual.InvocationCount;
	const int32 AudioBefore = Audio.InvocationCount;
	Audio.Mode = ERetryCommandFakeMode::Success;
	const auto Result = FCommandAdapter::TryExecute(
		Command, Host, Visual, Audio);
	const auto& Step = Result.Receipt.GetStep();
	TestTrue(TEXT("one command records one completed retry receipt"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCaptured
			&& Command.IsValid()
			&& Result.Status == ECommandStatus::RecordedHandled
			&& Result.HasReceipt()
			&& Result.Receipt.IsHandled()
			&& Result.Receipt.DidExecuteRetry()
			&& !Result.Receipt.CanCaptureNextCommand()
			&& Step.Status == EStepStatus::Completed
			&& Step.NextRenewalsUsed == 1
			&& Host.IsTerminal()
			&& Host.GetRecordCount() == RecordsBefore + 2
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryCommandFreshCursorTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommand.PendingRequiresFreshCommand",
	SwordRhythmCueProductRetryCommandFlags)

bool Fdemo_mapSwordRhythmCueProductRetryCommandFreshCursorTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryCommandFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB7200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryCommandFakeExecutor Visual(0xB7210000);
	FProductRetryCommandFakeExecutor Audio(
		0xB7220000, ERetryCommandFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand FirstCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xB7230001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB7240000, 2, 0),
		FirstCommand);
	const auto First = FCommandAdapter::TryExecute(
		FirstCommand, Host, Visual, Audio);
	const int32 RecordsAfterFirst = Host.GetRecordCount();
	const int32 CallsAfterFirst = Audio.InvocationCount;
	const auto Stale = FCommandAdapter::TryExecute(
		FirstCommand, Host, Visual, Audio);

	FCommand SecondCommand;
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xB7250001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(
			0xB7240000,
			2,
			First.Receipt.GetStep().NextRenewalsUsed),
		SecondCommand);
	Audio.Mode = ERetryCommandFakeMode::Success;
	const auto Second = FCommandAdapter::TryExecute(
		SecondCommand, Host, Visual, Audio);
	TestTrue(TEXT("a mutating retry step invalidates its exact command cursor"),
		!Initial.IsSuccess()
			&& bCapturedFirst
			&& First.Status == ECommandStatus::RecordedHandled
			&& First.HasReceipt()
			&& First.Receipt.GetStep().Status == EStepStatus::RetryPending
			&& First.Receipt.CanCaptureNextCommand()
			&& Stale.Status == ECommandStatus::HostCursorMismatch
			&& !Stale.HasReceipt()
			&& Host.GetRecordCount() == RecordsAfterFirst + 2
			&& Audio.InvocationCount == CallsAfterFirst + 1
			&& bCapturedSecond
			&& Second.Status == ECommandStatus::RecordedHandled
			&& Second.HasReceipt()
			&& Second.Receipt.GetStep().Status == EStepStatus::Completed
			&& Second.Receipt.GetStep().NextRenewalsUsed == 2
			&& First.Receipt.GetReceiptId()
				!= Second.Receipt.GetReceiptId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryCommandStopReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommand.StopReplayIsDeterministicAndInert",
	SwordRhythmCueProductRetryCommandFlags)

bool Fdemo_mapSwordRhythmCueProductRetryCommandStopReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryCommandFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB7300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryCommandFakeExecutor Visual(0xB7310000);
	FProductRetryCommandFakeExecutor Audio(
		0xB7320000, ERetryCommandFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xB7330001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB7340000, 1, 1),
		Command);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 VisualBefore = Visual.InvocationCount;
	const int32 AudioBefore = Audio.InvocationCount;
	const auto First = FCommandAdapter::TryExecute(
		Command, Host, Visual, Audio);
	const auto Replay = FCommandAdapter::TryExecute(
		Command, Host, Visual, Audio);
	TestTrue(TEXT("non-mutating stop replay derives identical inert evidence"),
		!Initial.IsSuccess()
			&& bCaptured
			&& First.Status == ECommandStatus::RecordedHandled
			&& Replay.Status == ECommandStatus::RecordedHandled
			&& First.HasReceipt()
			&& Replay.HasReceipt()
			&& First.Receipt.GetStep().Status
				== EStepStatus::StoppedBudgetExhausted
			&& !First.Receipt.DidExecuteRetry()
			&& !First.Receipt.CanCaptureNextCommand()
			&& First.Receipt.GetReceiptId()
				== Replay.Receipt.GetReceiptId()
			&& Host.GetRecordCount() == RecordsBefore
			&& Visual.InvocationCount == VisualBefore
			&& Audio.InvocationCount == AudioBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryCommandRejectionTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommand.RejectedAttemptsAreReceipted",
	SwordRhythmCueProductRetryCommandFlags)

bool Fdemo_mapSwordRhythmCueProductRetryCommandRejectionTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryCommandFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB7400000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB7410000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryCommandFakeExecutor Visual(0xB7420000);
	FProductRetryCommandFakeExecutor Audio(
		0xB7430000, ERetryCommandFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const int32 RecordsBefore = Host.GetRecordCount();
	const int32 CallsBefore = Audio.InvocationCount;

	FCommand ForeignCommand;
	const bool bCapturedForeign = FCommand::TryCapture(
		FGuid(0xB7440001, 0, 0, 1),
		Foreign.Prepared,
		Host,
		MakeDecisionRequest(0xB7450000, 2, 0),
		ForeignCommand);
	const auto DecisionRejected = FCommandAdapter::TryExecute(
		ForeignCommand, Host, Visual, Audio);

	FCommand ExecutionCommand;
	const bool bCapturedExecution = FCommand::TryCapture(
		FGuid(0xB7460001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB7470000, 2, 0),
		ExecutionCommand);
	Audio.Mode = ERetryCommandFakeMode::Rejected;
	const auto ExecutionRejected = FCommandAdapter::TryExecute(
		ExecutionCommand, Host, Visual, Audio);
	TestTrue(TEXT("decision and durable execution rejections remain auditable"),
		!Initial.IsSuccess()
			&& Foreign.IsPrepared()
			&& bCapturedForeign
			&& DecisionRejected.Status
				== ECommandStatus::RecordedDecisionRejected
			&& DecisionRejected.HasReceipt()
			&& DecisionRejected.Receipt.GetStep().Status
				== EStepStatus::DecisionRejected
			&& Host.GetRecordCount() == RecordsBefore + 1
			&& bCapturedExecution
			&& ExecutionRejected.Status
				== ECommandStatus::RecordedExecutionRejected
			&& ExecutionRejected.HasReceipt()
			&& ExecutionRejected.Receipt.DidExecuteRetry()
			&& ExecutionRejected.Receipt.GetStep().Execution.HasDurableProgress()
			&& Audio.InvocationCount == CallsBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryCommandFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommand.CaptureAndCursorFences",
	SwordRhythmCueProductRetryCommandFlags)

bool Fdemo_mapSwordRhythmCueProductRetryCommandFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryCommandFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB7500000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB7510000));
	FProductRetryCommandFakeExecutor Visual(0xB7520000);
	FProductRetryCommandFakeExecutor Audio(
		0xB7530000, ERetryCommandFakeMode::RetryableFailure);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto Request = MakeDecisionRequest(0xB7540000, 2, 0);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xB7550001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		Request,
		Command);
	FCommand Scratch = Command;
	const bool bInvalidId = FCommand::TryCapture(
		FGuid(), Dispatch.Prepared, Host, Request, Scratch);
	const bool bInvalidPrepared = FCommand::TryCapture(
		FGuid(0xB7560001, 0, 0, 1), {}, Host, Request, Scratch);
	const bool bInvalidRequest = FCommand::TryCapture(
		FGuid(0xB7570001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		FDecisionRequest(),
		Scratch);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	const bool bEmptyHost = FCommand::TryCapture(
		FGuid(0xB7580001, 0, 0, 1),
		Dispatch.Prepared,
		EmptyHost,
		Request,
		Scratch);
	const auto InvalidCommand = FCommandAdapter::TryExecute(
		FCommand(), Host, Visual, Audio);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost ForeignHost;
	FProductRetryCommandFakeExecutor ForeignVisual(0xB7590000);
	FProductRetryCommandFakeExecutor ForeignAudio(
		0xB75A0000, ERetryCommandFakeMode::RetryableFailure);
	const auto ForeignInitial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Foreign.Prepared, ForeignHost, ForeignVisual, ForeignAudio);
	const int32 ForeignRecords = ForeignHost.GetRecordCount();
	const int32 ForeignVisualCalls = ForeignVisual.InvocationCount;
	const int32 ForeignAudioCalls = ForeignAudio.InvocationCount;
	const auto CursorMismatch = FCommandAdapter::TryExecute(
		Command, ForeignHost, ForeignVisual, ForeignAudio);
	TestTrue(TEXT("capture and exact Host cursor fences fail before execution"),
		!Initial.IsSuccess()
			&& bCaptured
			&& Command.IsValid()
			&& !bInvalidId
			&& !bInvalidPrepared
			&& !bInvalidRequest
			&& !bEmptyHost
			&& !Scratch.IsValid()
			&& InvalidCommand.Status == ECommandStatus::CommandInvalid
			&& !InvalidCommand.HasReceipt()
			&& Foreign.IsPrepared()
			&& !ForeignInitial.IsSuccess()
			&& CursorMismatch.Status
				== ECommandStatus::HostCursorMismatch
			&& !CursorMismatch.HasReceipt()
			&& ForeignHost.GetRecordCount() == ForeignRecords
			&& ForeignVisual.InvocationCount == ForeignVisualCalls
			&& ForeignAudio.InvocationCount == ForeignAudioCalls);
	return true;
}

#endif
