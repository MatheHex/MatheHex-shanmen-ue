#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductRetryJournalFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB8000001, 0xB8000002, 0xB8000003, 0xB8000004);
	const FGuid TestWeapon(
		0xB8010001, 0xB8010002, 0xB8010003, 0xB8010004);

	using FCommand =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand;
	using FJournal =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal;
	using FJournalRecord =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord;
	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using EAdapterStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus;
	using EJournalStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus;
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

	struct FSwordRhythmCueProductRetryJournalFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductRetryJournalFixture()
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

	enum class ERetryJournalFakeMode : uint8
	{
		Success,
		RetryableFailure,
		Rejected
	};

	class FProductRetryJournalFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductRetryJournalFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryJournalFakeMode InMode =
				ERetryJournalFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryJournalFakeMode Mode = ERetryJournalFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ERetryJournalFakeMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Retry-journal fake rejected the invocation.");
				return Result;
			}
			const auto Outcome = Mode == ERetryJournalFakeMode::Success
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
					"Retry-journal fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-journal fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareDispatch(
		FSwordRhythmCueProductRetryJournalFixture& Fixture,
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
	Fdemo_mapSwordRhythmCueProductRetryJournalReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournal.HandledCommandRecordsAndReplays",
	SwordRhythmCueProductRetryJournalFlags)

bool Fdemo_mapSwordRhythmCueProductRetryJournalReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryJournalFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB8100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryJournalFakeExecutor Visual(0xB8110000);
	FProductRetryJournalFakeExecutor Audio(
		0xB8120000, ERetryJournalFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xB8130001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB8140000, 2, 0),
		Command);
	Audio.Mode = ERetryJournalFakeMode::Success;
	FJournal Journal;
	const auto Recorded = Journal.TryExecute(Command, Host, Visual, Audio);
	const int32 HostRecords = Host.GetRecordCount();
	const int32 VisualCalls = Visual.InvocationCount;
	const int32 AudioCalls = Audio.InvocationCount;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FProductRetryJournalFakeExecutor ReplayVisual(0xB8150000);
	FProductRetryJournalFakeExecutor ReplayAudio(0xB8160000);
	const auto Replay = Journal.TryExecute(
		Command, EmptyHost, ReplayVisual, ReplayAudio);
	FJournalRecord Stored;
	const bool bFound = Journal.TryGetRecord(Command.GetCommandId(), Stored);
	TestTrue(TEXT("exact replay returns the stored receipt without dependencies"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCaptured
			&& Recorded.Status == EJournalStatus::Recorded
			&& Recorded.IsSuccess()
			&& !Recorded.IsReplay()
			&& Recorded.AdapterStatus == EAdapterStatus::RecordedHandled
			&& Recorded.Receipt.GetStep().Status == EStepStatus::Completed
			&& Host.IsTerminal()
			&& Replay.Status == EJournalStatus::Replayed
			&& Replay.IsSuccess()
			&& Replay.IsReplay()
			&& Replay.Receipt.GetReceiptId()
				== Recorded.Receipt.GetReceiptId()
			&& Host.GetRecordCount() == HostRecords
			&& Visual.InvocationCount == VisualCalls
			&& Audio.InvocationCount == AudioCalls
			&& ReplayVisual.InvocationCount == 0
			&& ReplayAudio.InvocationCount == 0
			&& Journal.IsValid()
			&& Journal.GetRecordCount() == 1
			&& bFound
			&& Stored.GetReceipt().GetReceiptId()
				== Recorded.Receipt.GetReceiptId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryJournalPendingTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournal.PendingRequiresSecondRecordedCommand",
	SwordRhythmCueProductRetryJournalFlags)

bool Fdemo_mapSwordRhythmCueProductRetryJournalPendingTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryJournalFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB8200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryJournalFakeExecutor Visual(0xB8210000);
	FProductRetryJournalFakeExecutor Audio(
		0xB8220000, ERetryJournalFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FJournal Journal;
	FCommand FirstCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xB8230001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB8240000, 2, 0),
		FirstCommand);
	const auto First = Journal.TryExecute(
		FirstCommand, Host, Visual, Audio);
	const int32 RecordsAfterFirst = Host.GetRecordCount();
	const int32 AudioCallsAfterFirst = Audio.InvocationCount;
	const auto FirstReplay = Journal.TryExecute(
		FirstCommand, Host, Visual, Audio);

	FCommand SecondCommand;
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xB8250001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(
			0xB8240000,
			2,
			First.Receipt.GetStep().NextRenewalsUsed),
		SecondCommand);
	Audio.Mode = ERetryJournalFakeMode::Success;
	const auto Second = Journal.TryExecute(
		SecondCommand, Host, Visual, Audio);
	TestTrue(TEXT("pending progress requires a fresh command and receipt"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedFirst
			&& First.Status == EJournalStatus::Recorded
			&& First.Receipt.GetStep().Status == EStepStatus::RetryPending
			&& First.Receipt.CanCaptureNextCommand()
			&& FirstReplay.IsReplay()
			&& Host.GetRecordCount() == RecordsAfterFirst + 2
			&& Audio.InvocationCount == AudioCallsAfterFirst + 1
			&& bCapturedSecond
			&& Second.Status == EJournalStatus::Recorded
			&& Second.Receipt.GetStep().Status == EStepStatus::Completed
			&& Second.Receipt.GetStep().NextRenewalsUsed == 2
			&& First.Receipt.GetReceiptId()
				!= Second.Receipt.GetReceiptId()
			&& Journal.IsValid()
			&& Journal.GetRecordCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryJournalConflictTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournal.CommandIdConflictFailsClosed",
	SwordRhythmCueProductRetryJournalFlags)

bool Fdemo_mapSwordRhythmCueProductRetryJournalConflictTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryJournalFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB8300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryJournalFakeExecutor Visual(0xB8310000);
	FProductRetryJournalFakeExecutor Audio(
		0xB8320000, ERetryJournalFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const FGuid SharedCommandId(0xB8330001, 0, 0, 1);
	FCommand Original;
	FCommand Conflict;
	const bool bCapturedOriginal = FCommand::TryCapture(
		SharedCommandId,
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB8340000, 1, 1),
		Original);
	const bool bCapturedConflict = FCommand::TryCapture(
		SharedCommandId,
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB8350000, 1, 1),
		Conflict);
	FJournal Journal;
	const auto Recorded = Journal.TryExecute(Original, Host, Visual, Audio);
	const int32 RecordsBeforeConflict = Host.GetRecordCount();
	const int32 VisualBeforeConflict = Visual.InvocationCount;
	const int32 AudioBeforeConflict = Audio.InvocationCount;
	const auto Rejected = Journal.TryExecute(
		Conflict, Host, Visual, Audio);
	TestTrue(TEXT("one CommandId cannot alias a different immutable command"),
		!Initial.IsSuccess()
			&& bCapturedOriginal
			&& bCapturedConflict
			&& !Original.Matches(Conflict)
			&& Recorded.IsSuccess()
			&& Recorded.Receipt.GetStep().Status
				== EStepStatus::StoppedBudgetExhausted
			&& Rejected.Status == EJournalStatus::CommandReplayConflict
			&& !Rejected.IsSuccess()
			&& Host.GetRecordCount() == RecordsBeforeConflict
			&& Visual.InvocationCount == VisualBeforeConflict
			&& Audio.InvocationCount == AudioBeforeConflict
			&& Journal.IsValid()
			&& Journal.GetRecordCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryJournalRejectedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournal.RejectedReceiptsAreReplayable",
	SwordRhythmCueProductRetryJournalFlags)

bool Fdemo_mapSwordRhythmCueProductRetryJournalRejectedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryJournalFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB8400000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB8410000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryJournalFakeExecutor Visual(0xB8420000);
	FProductRetryJournalFakeExecutor Audio(
		0xB8430000, ERetryJournalFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FJournal Journal;

	FCommand ForeignCommand;
	const bool bCapturedForeign = FCommand::TryCapture(
		FGuid(0xB8440001, 0, 0, 1),
		Foreign.Prepared,
		Host,
		MakeDecisionRequest(0xB8450000, 2, 0),
		ForeignCommand);
	const auto DecisionRejected = Journal.TryExecute(
		ForeignCommand, Host, Visual, Audio);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FProductRetryJournalFakeExecutor ReplayVisual(0xB8460000);
	FProductRetryJournalFakeExecutor ReplayAudio(0xB8470000);
	const auto DecisionReplay = Journal.TryExecute(
		ForeignCommand, EmptyHost, ReplayVisual, ReplayAudio);

	FCommand ExecutionCommand;
	const bool bCapturedExecution = FCommand::TryCapture(
		FGuid(0xB8480001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB8490000, 2, 0),
		ExecutionCommand);
	Audio.Mode = ERetryJournalFakeMode::Rejected;
	const auto ExecutionRejected = Journal.TryExecute(
		ExecutionCommand, Host, Visual, Audio);
	const int32 RecordsAfterExecution = Host.GetRecordCount();
	const int32 AudioCallsAfterExecution = Audio.InvocationCount;
	const auto ExecutionReplay = Journal.TryExecute(
		ExecutionCommand, Host, Visual, Audio);
	TestTrue(TEXT("durable rejection receipts replay without re-execution"),
		Dispatch.IsPrepared()
			&& Foreign.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedForeign
			&& DecisionRejected.Status == EJournalStatus::Recorded
			&& DecisionRejected.AdapterStatus
				== EAdapterStatus::RecordedDecisionRejected
			&& DecisionRejected.Receipt.GetStep().Status
				== EStepStatus::DecisionRejected
			&& DecisionReplay.IsReplay()
			&& bCapturedExecution
			&& ExecutionRejected.Status == EJournalStatus::Recorded
			&& ExecutionRejected.AdapterStatus
				== EAdapterStatus::RecordedExecutionRejected
			&& ExecutionRejected.Receipt.GetStep().Status
				== EStepStatus::ExecutionRejected
			&& ExecutionRejected.Receipt.DidExecuteRetry()
			&& ExecutionReplay.IsReplay()
			&& Host.GetRecordCount() == RecordsAfterExecution
			&& Audio.InvocationCount == AudioCallsAfterExecution
			&& ReplayVisual.InvocationCount == 0
			&& ReplayAudio.InvocationCount == 0
			&& Journal.IsValid()
			&& Journal.GetRecordCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRetryJournalFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournal.UnreceiptedFailureIsNotJournaled",
	SwordRhythmCueProductRetryJournalFlags)

bool Fdemo_mapSwordRhythmCueProductRetryJournalFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRetryJournalFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB8500000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB8510000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductRetryJournalFakeExecutor Visual(0xB8520000);
	FProductRetryJournalFakeExecutor Audio(
		0xB8530000, ERetryJournalFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost ForeignHost;
	FProductRetryJournalFakeExecutor ForeignVisual(0xB8540000);
	FProductRetryJournalFakeExecutor ForeignAudio(
		0xB8550000, ERetryJournalFakeMode::RetryableFailure);
	const auto ForeignInitial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Foreign.Prepared, ForeignHost, ForeignVisual, ForeignAudio);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xB8560001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeDecisionRequest(0xB8570000, 2, 0),
		Command);
	FJournal Journal;
	const int32 ForeignRecords = ForeignHost.GetRecordCount();
	const int32 ForeignVisualCalls = ForeignVisual.InvocationCount;
	const int32 ForeignAudioCalls = ForeignAudio.InvocationCount;
	const auto WrongHost = Journal.TryExecute(
		Command, ForeignHost, ForeignVisual, ForeignAudio);
	const auto Invalid = Journal.TryExecute(
		FCommand(), ForeignHost, ForeignVisual, ForeignAudio);
	Audio.Mode = ERetryJournalFakeMode::Success;
	const auto CorrectHost = Journal.TryExecute(
		Command, Host, Visual, Audio);
	TestTrue(TEXT("unreceipted adapter failures remain safely retryable"),
		Dispatch.IsPrepared()
			&& Foreign.IsPrepared()
			&& !Initial.IsSuccess()
			&& !ForeignInitial.IsSuccess()
			&& bCaptured
			&& WrongHost.Status == EJournalStatus::AdapterRejected
			&& WrongHost.AdapterStatus == EAdapterStatus::HostCursorMismatch
			&& Journal.GetRecordCount() == 1
			&& ForeignHost.GetRecordCount() == ForeignRecords
			&& ForeignVisual.InvocationCount == ForeignVisualCalls
			&& ForeignAudio.InvocationCount == ForeignAudioCalls
			&& Invalid.Status == EJournalStatus::CommandInvalid
			&& CorrectHost.Status == EJournalStatus::Recorded
			&& CorrectHost.IsSuccess()
			&& CorrectHost.Receipt.GetStep().Status == EStepStatus::Completed
			&& Journal.IsValid());
	return true;
}

#endif
