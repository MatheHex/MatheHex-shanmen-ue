#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueRetryEnvelopeFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid EnvelopeTestRun(
		0xBA000001, 0xBA000002, 0xBA000003, 0xBA000004);
	const FGuid EnvelopeTestWeapon(
		0xBA010001, 0xBA010002, 0xBA010003, 0xBA010004);

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
	using FEnvelope =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope;
	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using EJournalStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;

	FDispatchSeed MakeEnvelopeDispatchSeed(const uint32 Value)
	{
		FDispatchSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	FDecisionRequest MakeEnvelopeDecisionRequest(
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

	struct FSwordRhythmCueRetryEnvelopeFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueRetryEnvelopeFixture()
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
					EnvelopeTestRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(EnvelopeTestRun, Diagnostic)
				&& Session.TryBegin(EnvelopeTestRun, Diagnostic);
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
				EnvelopeTestWeapon, 1.0f, {});
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

	enum class ERetryEnvelopeFakeMode : uint8
	{
		Success,
		RetryableFailure
	};

	class FRetryEnvelopeFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FRetryEnvelopeFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryEnvelopeFakeMode InMode =
				ERetryEnvelopeFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryEnvelopeFakeMode Mode = ERetryEnvelopeFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			const auto Outcome = Mode == ERetryEnvelopeFakeMode::Success
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
					"Retry-envelope fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-envelope fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareEnvelopeDispatch(
		FSwordRhythmCueRetryEnvelopeFixture& Fixture,
		const uint32 Seed)
	{
		if (!Fixture.TryPrepareCurrentEffect())
		{
			return {};
		}
		return
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
				PrepareCurrent(
					Fixture.Session, MakeEnvelopeDispatchSeed(Seed));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryEnvelopeEmptyTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.EmptyCheckpointStableRoundTrip",
	SwordRhythmCueRetryEnvelopeFlags)

bool Fdemo_mapSwordRhythmCueRetryEnvelopeEmptyTest::RunTest(const FString&)
{
	FJournal Journal;
	const auto Export = FCheckpointService::Export(Journal);
	FEnvelope First;
	FEnvelope Second;
	const bool bWrappedFirst = FEnvelope::TryWrap(
		FEnvelope::CurrentSchemaVersion(), Export.Checkpoint, First);
	const bool bWrappedSecond = FEnvelope::TryWrap(
		FEnvelope::CurrentSchemaVersion(), Export.Checkpoint, Second);
	FCheckpoint Unwrapped;
	const bool bUnwrapped = First.TryUnwrap(Unwrapped);
	TestTrue(TEXT("empty checkpoint has one stable versioned envelope"),
		Export.IsSuccess()
			&& bWrappedFirst
			&& bWrappedSecond
			&& First.IsValid()
			&& First.GetSchemaVersion() == 1
			&& First.GetRecordCount() == 0
			&& First.GetEnvelopeId() == Second.GetEnvelopeId()
			&& First.Matches(Second)
			&& bUnwrapped
			&& Unwrapped.Matches(Export.Checkpoint));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryEnvelopeVersionTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.UnsupportedVersionAndInvalidCheckpointFailClosed",
	SwordRhythmCueRetryEnvelopeFlags)

bool Fdemo_mapSwordRhythmCueRetryEnvelopeVersionTest::RunTest(
	const FString&)
{
	FJournal Journal;
	const auto Export = FCheckpointService::Export(Journal);
	FEnvelope ZeroVersion;
	FEnvelope FutureVersion;
	FEnvelope InvalidCheckpoint;
	const bool bWrappedZero = FEnvelope::TryWrap(
		0, Export.Checkpoint, ZeroVersion);
	const bool bWrappedFuture = FEnvelope::TryWrap(
		FEnvelope::CurrentSchemaVersion() + 1,
		Export.Checkpoint,
		FutureVersion);
	const bool bWrappedInvalid = FEnvelope::TryWrap(
		FEnvelope::CurrentSchemaVersion(),
		FCheckpoint(),
		InvalidCheckpoint);
	FCheckpoint Unwrapped;
	TestTrue(TEXT("only the current schema and a valid checkpoint can wrap"),
		Export.IsSuccess()
			&& !bWrappedZero
			&& !ZeroVersion.IsValid()
			&& !bWrappedFuture
			&& !FutureVersion.IsValid()
			&& !bWrappedInvalid
			&& !InvalidCheckpoint.IsValid()
			&& !InvalidCheckpoint.TryUnwrap(Unwrapped)
			&& !Unwrapped.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryEnvelopeOrderTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.RecordOrderChangesEnvelopeIdentity",
	SwordRhythmCueRetryEnvelopeFlags)

bool Fdemo_mapSwordRhythmCueRetryEnvelopeOrderTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryEnvelopeFixture Fixture;
	const auto Dispatch = PrepareEnvelopeDispatch(Fixture, 0xBA100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryEnvelopeFakeExecutor Visual(0xBA110000);
	FRetryEnvelopeFakeExecutor Audio(
		0xBA120000, ERetryEnvelopeFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand FirstCommand;
	FCommand SecondCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xBA130001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeEnvelopeDecisionRequest(0xBA140000, 1, 1),
		FirstCommand);
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xBA150001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeEnvelopeDecisionRequest(0xBA160000, 1, 1),
		SecondCommand);
	FJournal Journal;
	const auto First = Journal.TryExecute(
		FirstCommand, Host, Visual, Audio);
	const auto Second = Journal.TryExecute(
		SecondCommand, Host, Visual, Audio);
	const auto ForwardCheckpoint = FCheckpointService::Export(Journal);
	FRecord FirstRecord;
	FRecord SecondRecord;
	const bool bReadFirst = ForwardCheckpoint.Checkpoint.TryGetRecordAt(
		0, FirstRecord);
	const bool bReadSecond = ForwardCheckpoint.Checkpoint.TryGetRecordAt(
		1, SecondRecord);
	TArray<FRecord> ReversedRecords;
	ReversedRecords.Add(SecondRecord);
	ReversedRecords.Add(FirstRecord);
	FCheckpoint ReversedCheckpoint;
	const bool bCreatedReversed = FCheckpoint::TryCreate(
		ReversedRecords, ReversedCheckpoint);
	FEnvelope ForwardEnvelope;
	FEnvelope ReversedEnvelope;
	const bool bWrappedForward = FEnvelope::TryWrap(
		1, ForwardCheckpoint.Checkpoint, ForwardEnvelope);
	const bool bWrappedReversed = FEnvelope::TryWrap(
		1, ReversedCheckpoint, ReversedEnvelope);
	TestTrue(TEXT("checkpoint order remains visible at the envelope boundary"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedFirst
			&& bCapturedSecond
			&& First.IsSuccess()
			&& Second.IsSuccess()
			&& ForwardCheckpoint.IsSuccess()
			&& bReadFirst
			&& bReadSecond
			&& bCreatedReversed
			&& bWrappedForward
			&& bWrappedReversed
			&& ForwardEnvelope.GetRecordCount() == 2
			&& ReversedEnvelope.GetRecordCount() == 2
			&& ForwardEnvelope.GetEnvelopeId()
				!= ReversedEnvelope.GetEnvelopeId()
			&& !ForwardEnvelope.Matches(ReversedEnvelope));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryEnvelopeReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.UnwrappedCheckpointRestoresLazyReplay",
	SwordRhythmCueRetryEnvelopeFlags)

bool Fdemo_mapSwordRhythmCueRetryEnvelopeReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryEnvelopeFixture Fixture;
	const auto Dispatch = PrepareEnvelopeDispatch(Fixture, 0xBA200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryEnvelopeFakeExecutor Visual(0xBA210000);
	FRetryEnvelopeFakeExecutor Audio(
		0xBA220000, ERetryEnvelopeFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xBA230001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeEnvelopeDecisionRequest(0xBA240000, 2, 0),
		Command);
	FJournal Journal;
	const auto Recorded = Journal.TryExecute(Command, Host, Visual, Audio);
	const auto Export = FCheckpointService::Export(Journal);
	FEnvelope Envelope;
	const bool bWrapped = FEnvelope::TryWrap(1, Export.Checkpoint, Envelope);
	FCheckpoint Unwrapped;
	const bool bUnwrapped = Envelope.TryUnwrap(Unwrapped);
	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(Unwrapped, Restored);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FRetryEnvelopeFakeExecutor ReplayVisual(0xBA250000);
	FRetryEnvelopeFakeExecutor ReplayAudio(0xBA260000);
	const auto Replay = Restored.TryExecute(
		Command, EmptyHost, ReplayVisual, ReplayAudio);
	TestTrue(TEXT("envelope round-trip preserves zero-dependency replay"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCaptured
			&& Recorded.Status == EJournalStatus::Recorded
			&& Recorded.Receipt.GetStep().Status == EStepStatus::RetryPending
			&& Export.IsSuccess()
			&& bWrapped
			&& bUnwrapped
			&& Restore.IsSuccess()
			&& Replay.IsReplay()
			&& Replay.Receipt.GetReceiptId()
				== Recorded.Receipt.GetReceiptId()
			&& ReplayVisual.InvocationCount == 0
			&& ReplayAudio.InvocationCount == 0
			&& Restored.IsValid()
			&& Restored.GetRecordCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryEnvelopeContinueTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.RestoredJournalAcceptsOneFreshCommand",
	SwordRhythmCueRetryEnvelopeFlags)

bool Fdemo_mapSwordRhythmCueRetryEnvelopeContinueTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryEnvelopeFixture Fixture;
	const auto Dispatch = PrepareEnvelopeDispatch(Fixture, 0xBA300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryEnvelopeFakeExecutor Visual(0xBA310000);
	FRetryEnvelopeFakeExecutor Audio(
		0xBA320000, ERetryEnvelopeFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand FirstCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xBA330001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeEnvelopeDecisionRequest(0xBA340000, 2, 0),
		FirstCommand);
	FJournal Source;
	const auto First = Source.TryExecute(
		FirstCommand, Host, Visual, Audio);
	const auto Export = FCheckpointService::Export(Source);
	FEnvelope Envelope;
	const bool bWrapped = FEnvelope::TryWrap(1, Export.Checkpoint, Envelope);
	FCheckpoint Unwrapped;
	const bool bUnwrapped = Envelope.TryUnwrap(Unwrapped);
	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(Unwrapped, Restored);
	FCommand SecondCommand;
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xBA350001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeEnvelopeDecisionRequest(
			0xBA340000,
			2,
			First.Receipt.GetStep().NextRenewalsUsed),
		SecondCommand);
	const int32 VisualCallsBeforeSecond = Visual.InvocationCount;
	const int32 AudioCallsBeforeSecond = Audio.InvocationCount;
	Audio.Mode = ERetryEnvelopeFakeMode::Success;
	const auto Second = Restored.TryExecute(
		SecondCommand, Host, Visual, Audio);
	TestTrue(TEXT("unwrapped journal continues only through a fresh command"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCapturedFirst
			&& First.Receipt.GetStep().Status == EStepStatus::RetryPending
			&& Export.IsSuccess()
			&& bWrapped
			&& bUnwrapped
			&& Restore.IsSuccess()
			&& bCapturedSecond
			&& Second.Status == EJournalStatus::Recorded
			&& Second.Receipt.GetStep().Status == EStepStatus::Completed
			&& Visual.InvocationCount == VisualCallsBeforeSecond
			&& Audio.InvocationCount == AudioCallsBeforeSecond + 1
			&& Source.GetRecordCount() == 1
			&& Restored.IsValid()
			&& Restored.GetRecordCount() == 2);
	return true;
}

#endif
