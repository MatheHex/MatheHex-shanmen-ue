#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueRetryCheckpointFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid CheckpointTestRun(
		0xB9000001, 0xB9000002, 0xB9000003, 0xB9000004);
	const FGuid CheckpointTestWeapon(
		0xB9010001, 0xB9010002, 0xB9010003, 0xB9010004);

	using FCommand =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand;
	using FJournal =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal;
	using FRecord =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord;
	using FCheckpoint =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint;
	using FCheckpointService =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointService;
	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using ECheckpointStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointStatus;
	using EAdapterStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus;
	using EJournalStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;

	FDispatchSeed MakeCheckpointDispatchSeed(const uint32 Value)
	{
		FDispatchSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	FDecisionRequest MakeCheckpointDecisionRequest(
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

	struct FSwordRhythmCueRetryCheckpointFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueRetryCheckpointFixture()
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
					CheckpointTestRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(CheckpointTestRun, Diagnostic)
				&& Session.TryBegin(CheckpointTestRun, Diagnostic);
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
				CheckpointTestWeapon, 1.0f, {});
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

	enum class ERetryCheckpointFakeMode : uint8
	{
		Success,
		RetryableFailure,
		Rejected
	};

	class FRetryCheckpointFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FRetryCheckpointFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryCheckpointFakeMode InMode =
				ERetryCheckpointFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryCheckpointFakeMode Mode = ERetryCheckpointFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (Mode == ERetryCheckpointFakeMode::Rejected)
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Retry-checkpoint fake rejected the invocation.");
				return Result;
			}
			const auto Outcome = Mode == ERetryCheckpointFakeMode::Success
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
					"Retry-checkpoint fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-checkpoint fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareCheckpointDispatch(
		FSwordRhythmCueRetryCheckpointFixture& Fixture,
		const uint32 Seed)
	{
		if (!Fixture.TryPrepareCurrentEffect())
		{
			return {};
		}
		return
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
				PrepareCurrent(
					Fixture.Session, MakeCheckpointDispatchSeed(Seed));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryCheckpointEmptyTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.EmptyJournalRoundTrips",
	SwordRhythmCueRetryCheckpointFlags)

bool Fdemo_mapSwordRhythmCueRetryCheckpointEmptyTest::RunTest(const FString&)
{
	FJournal Source;
	const auto FirstExport = FCheckpointService::Export(Source);
	const auto SecondExport = FCheckpointService::Export(Source);
	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(
		FirstExport.Checkpoint, Restored);
	TestTrue(TEXT("empty journal has a stable restorable identity"),
		Source.IsValid()
			&& FirstExport.Status == ECheckpointStatus::Exported
			&& FirstExport.IsSuccess()
			&& FirstExport.Checkpoint.GetRecordCount() == 0
			&& SecondExport.IsSuccess()
			&& FirstExport.Checkpoint.Matches(SecondExport.Checkpoint)
			&& FirstExport.Checkpoint.GetCheckpointId()
				== SecondExport.Checkpoint.GetCheckpointId()
			&& Restore.Status == ECheckpointStatus::Restored
			&& Restore.IsSuccess()
			&& Restored.IsValid()
			&& Restored.GetRecordCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryCheckpointPendingTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.RestoredReplayAndFreshCommand",
	SwordRhythmCueRetryCheckpointFlags)

bool Fdemo_mapSwordRhythmCueRetryCheckpointPendingTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryCheckpointFixture Fixture;
	const auto Dispatch = PrepareCheckpointDispatch(Fixture, 0xB9100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryCheckpointFakeExecutor Visual(0xB9110000);
	FRetryCheckpointFakeExecutor Audio(
		0xB9120000, ERetryCheckpointFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand FirstCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xB9130001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeCheckpointDecisionRequest(0xB9140000, 2, 0),
		FirstCommand);
	FJournal Source;
	const auto First = Source.TryExecute(
		FirstCommand, Host, Visual, Audio);
	const auto Export = FCheckpointService::Export(Source);
	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(
		Export.Checkpoint, Restored);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FRetryCheckpointFakeExecutor ReplayVisual(0xB9150000);
	FRetryCheckpointFakeExecutor ReplayAudio(0xB9160000);
	const auto Replay = Restored.TryExecute(
		FirstCommand, EmptyHost, ReplayVisual, ReplayAudio);

	FCommand SecondCommand;
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xB9170001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeCheckpointDecisionRequest(
			0xB9140000,
			2,
			First.Receipt.GetStep().NextRenewalsUsed),
		SecondCommand);
	Audio.Mode = ERetryCheckpointFakeMode::Success;
	const auto Second = Restored.TryExecute(
		SecondCommand, Host, Visual, Audio);
	TestTrue(TEXT("restore replays old work and accepts one fresh command"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedFirst
			&& First.Status == EJournalStatus::Recorded
			&& First.Receipt.GetStep().Status == EStepStatus::RetryPending
			&& Export.IsSuccess()
			&& Restore.IsSuccess()
			&& Replay.Status == EJournalStatus::Replayed
			&& Replay.IsReplay()
			&& Replay.Receipt.GetReceiptId()
				== First.Receipt.GetReceiptId()
			&& ReplayVisual.InvocationCount == 0
			&& ReplayAudio.InvocationCount == 0
			&& bCapturedSecond
			&& Second.Status == EJournalStatus::Recorded
			&& Second.Receipt.GetStep().Status == EStepStatus::Completed
			&& Restored.IsValid()
			&& Restored.GetRecordCount() == 2
			&& Source.GetRecordCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryCheckpointRejectedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.RejectedReceiptsRoundTrip",
	SwordRhythmCueRetryCheckpointFlags)

bool Fdemo_mapSwordRhythmCueRetryCheckpointRejectedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryCheckpointFixture Fixture;
	const auto Dispatch = PrepareCheckpointDispatch(Fixture, 0xB9200000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(
				Fixture.Session, MakeCheckpointDispatchSeed(0xB9210000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryCheckpointFakeExecutor Visual(0xB9220000);
	FRetryCheckpointFakeExecutor Audio(
		0xB9230000, ERetryCheckpointFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FJournal Source;

	FCommand ForeignCommand;
	const bool bCapturedForeign = FCommand::TryCapture(
		FGuid(0xB9240001, 0, 0, 1),
		Foreign.Prepared,
		Host,
		MakeCheckpointDecisionRequest(0xB9250000, 2, 0),
		ForeignCommand);
	const auto DecisionRejected = Source.TryExecute(
		ForeignCommand, Host, Visual, Audio);

	FCommand ExecutionCommand;
	const bool bCapturedExecution = FCommand::TryCapture(
		FGuid(0xB9260001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeCheckpointDecisionRequest(0xB9270000, 2, 0),
		ExecutionCommand);
	Audio.Mode = ERetryCheckpointFakeMode::Rejected;
	const auto ExecutionRejected = Source.TryExecute(
		ExecutionCommand, Host, Visual, Audio);
	const auto Export = FCheckpointService::Export(Source);
	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(
		Export.Checkpoint, Restored);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FRetryCheckpointFakeExecutor ReplayVisual(0xB9280000);
	FRetryCheckpointFakeExecutor ReplayAudio(0xB9290000);
	const auto DecisionReplay = Restored.TryExecute(
		ForeignCommand, EmptyHost, ReplayVisual, ReplayAudio);
	const auto ExecutionReplay = Restored.TryExecute(
		ExecutionCommand, EmptyHost, ReplayVisual, ReplayAudio);
	TestTrue(TEXT("both durable rejection classes survive restore"),
		Dispatch.IsPrepared()
			&& Foreign.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedForeign
			&& DecisionRejected.AdapterStatus
				== EAdapterStatus::RecordedDecisionRejected
			&& bCapturedExecution
			&& ExecutionRejected.AdapterStatus
				== EAdapterStatus::RecordedExecutionRejected
			&& Export.IsSuccess()
			&& Restore.IsSuccess()
			&& DecisionReplay.IsReplay()
			&& DecisionReplay.AdapterStatus
				== EAdapterStatus::RecordedDecisionRejected
			&& DecisionReplay.Receipt.GetReceiptId()
				== DecisionRejected.Receipt.GetReceiptId()
			&& ExecutionReplay.IsReplay()
			&& ExecutionReplay.AdapterStatus
				== EAdapterStatus::RecordedExecutionRejected
			&& ExecutionReplay.Receipt.GetReceiptId()
				== ExecutionRejected.Receipt.GetReceiptId()
			&& ReplayVisual.InvocationCount == 0
			&& ReplayAudio.InvocationCount == 0
			&& Restored.IsValid()
			&& Restored.GetRecordCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryCheckpointMalformedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.MalformedAndDuplicateRecordsFailClosed",
	SwordRhythmCueRetryCheckpointFlags)

bool Fdemo_mapSwordRhythmCueRetryCheckpointMalformedTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryCheckpointFixture Fixture;
	const auto Dispatch = PrepareCheckpointDispatch(Fixture, 0xB9300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryCheckpointFakeExecutor Visual(0xB9310000);
	FRetryCheckpointFakeExecutor Audio(
		0xB9320000, ERetryCheckpointFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xB9330001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeCheckpointDecisionRequest(0xB9340000, 1, 1),
		Command);
	FJournal Source;
	const auto Recorded = Source.TryExecute(Command, Host, Visual, Audio);
	FRecord ValidRecord;
	const bool bFound = Source.TryGetRecord(
		Command.GetCommandId(), ValidRecord);

	FCheckpoint InvalidRecordCheckpoint;
	TArray<FRecord> InvalidRecords;
	InvalidRecords.Add(FRecord());
	const bool bCreatedInvalid = FCheckpoint::TryCreate(
		InvalidRecords, InvalidRecordCheckpoint);
	FCheckpoint DuplicateCheckpoint;
	TArray<FRecord> DuplicateRecords;
	DuplicateRecords.Add(ValidRecord);
	DuplicateRecords.Add(ValidRecord);
	const bool bCreatedDuplicate = FCheckpoint::TryCreate(
		DuplicateRecords, DuplicateCheckpoint);
	FJournal Target;
	const auto RestoreInvalid = FCheckpointService::Restore(
		FCheckpoint(), Target);
	TestTrue(TEXT("malformed identities cannot become restore state"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCaptured
			&& Recorded.IsSuccess()
			&& Recorded.Receipt.GetStep().Status
				== EStepStatus::StoppedBudgetExhausted
			&& bFound
			&& ValidRecord.IsValid()
			&& !bCreatedInvalid
			&& !InvalidRecordCheckpoint.IsValid()
			&& !bCreatedDuplicate
			&& !DuplicateCheckpoint.IsValid()
			&& RestoreInvalid.Status == ECheckpointStatus::CheckpointInvalid
			&& !RestoreInvalid.IsSuccess()
			&& Target.IsValid()
			&& Target.GetRecordCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryCheckpointOrderTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.OrderIdentityAndNonEmptyFence",
	SwordRhythmCueRetryCheckpointFlags)

bool Fdemo_mapSwordRhythmCueRetryCheckpointOrderTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryCheckpointFixture Fixture;
	const auto Dispatch = PrepareCheckpointDispatch(Fixture, 0xB9400000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryCheckpointFakeExecutor Visual(0xB9410000);
	FRetryCheckpointFakeExecutor Audio(
		0xB9420000, ERetryCheckpointFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand FirstCommand;
	FCommand SecondCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xB9430001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeCheckpointDecisionRequest(0xB9440000, 1, 1),
		FirstCommand);
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xB9450001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeCheckpointDecisionRequest(0xB9460000, 1, 1),
		SecondCommand);
	FJournal Source;
	const auto First = Source.TryExecute(
		FirstCommand, Host, Visual, Audio);
	const auto Second = Source.TryExecute(
		SecondCommand, Host, Visual, Audio);
	const auto Forward = FCheckpointService::Export(Source);
	FRecord FirstRecord;
	FRecord SecondRecord;
	const bool bReadFirst = Forward.Checkpoint.TryGetRecordAt(
		0, FirstRecord);
	const bool bReadSecond = Forward.Checkpoint.TryGetRecordAt(
		1, SecondRecord);
	TArray<FRecord> ReversedRecords;
	ReversedRecords.Add(SecondRecord);
	ReversedRecords.Add(FirstRecord);
	FCheckpoint Reversed;
	const bool bCreatedReversed = FCheckpoint::TryCreate(
		ReversedRecords, Reversed);

	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(
		Forward.Checkpoint, Restored);
	FRecord RestoredFirst;
	FRecord RestoredSecond;
	const bool bFoundFirst = Restored.TryGetRecord(
		FirstCommand.GetCommandId(), RestoredFirst);
	const bool bFoundSecond = Restored.TryGetRecord(
		SecondCommand.GetCommandId(), RestoredSecond);
	const auto RefusedOverwrite = FCheckpointService::Restore(
		Reversed, Restored);
	TestTrue(TEXT("record order is identity and restore never overwrites"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedFirst
			&& bCapturedSecond
			&& First.IsSuccess()
			&& Second.IsSuccess()
			&& Source.IsValid()
			&& Source.GetRecordCount() == 2
			&& Forward.IsSuccess()
			&& bReadFirst
			&& bReadSecond
			&& bCreatedReversed
			&& Reversed.IsValid()
			&& Forward.Checkpoint.GetCheckpointId()
				!= Reversed.GetCheckpointId()
			&& !Forward.Checkpoint.Matches(Reversed)
			&& Restore.IsSuccess()
			&& bFoundFirst
			&& bFoundSecond
			&& RestoredFirst.GetReceipt().GetReceiptId()
				== FirstRecord.GetReceipt().GetReceiptId()
			&& RestoredSecond.GetReceipt().GetReceiptId()
				== SecondRecord.GetReceipt().GetReceiptId()
			&& RefusedOverwrite.Status == ECheckpointStatus::TargetNotEmpty
			&& !RefusedOverwrite.IsSuccess()
			&& Restored.IsValid()
			&& Restored.GetRecordCount() == 2);
	return true;
}

#endif
