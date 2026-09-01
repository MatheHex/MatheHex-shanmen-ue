#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueRetryManifestCodecFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ManifestCodecTestRun(
		0xBB000001, 0xBB000002, 0xBB000003, 0xBB000004);
	const FGuid ManifestCodecTestWeapon(
		0xBB010001, 0xBB010002, 0xBB010003, 0xBB010004);

	using FCodec =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec;
	using FManifest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifest;
	using FEnvelope =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope;
	using FCheckpoint =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint;
	using FCheckpointService =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointService;
	using FJournal =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal;
	using FRecord =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord;
	using FCommand =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand;
	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FDecisionRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using EJournalStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;

	bool TryMakeEmptyEnvelope(FEnvelope& OutEnvelope)
	{
		OutEnvelope = FEnvelope();
		FJournal Journal;
		const auto Export = FCheckpointService::Export(Journal);
		return Export.IsSuccess()
			&& FEnvelope::TryWrap(
				FEnvelope::CurrentSchemaVersion(),
				Export.Checkpoint,
				OutEnvelope);
	}

	FDispatchSeed MakeManifestDispatchSeed(const uint32 Value)
	{
		FDispatchSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	FDecisionRequest MakeManifestDecisionRequest(
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

	struct FSwordRhythmCueRetryManifestFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueRetryManifestFixture()
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
					ManifestCodecTestRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(ManifestCodecTestRun, Diagnostic)
				&& Session.TryBegin(ManifestCodecTestRun, Diagnostic);
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
				ManifestCodecTestWeapon, 1.0f, {});
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

	enum class ERetryManifestFakeMode : uint8
	{
		Success,
		RetryableFailure
	};

	class FRetryManifestFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FRetryManifestFakeExecutor(
			const uint32 InReceiptSeed,
			const ERetryManifestFakeMode InMode =
				ERetryManifestFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		ERetryManifestFakeMode Mode = ERetryManifestFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			const auto Outcome = Mode == ERetryManifestFakeMode::Success
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
					"Retry-manifest fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Retry-manifest fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareManifestDispatch(
		FSwordRhythmCueRetryManifestFixture& Fixture,
		const uint32 Seed)
	{
		if (!Fixture.TryPrepareCurrentEffect())
		{
			return {};
		}
		return
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
				PrepareCurrent(
					Fixture.Session, MakeManifestDispatchSeed(Seed));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryManifestCodecStableTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.StableEmptyRoundTrip",
	SwordRhythmCueRetryManifestCodecFlags)

bool Fdemo_mapSwordRhythmCueRetryManifestCodecStableTest::RunTest(
	const FString&)
{
	FEnvelope Envelope;
	const bool bMadeEnvelope = TryMakeEmptyEnvelope(Envelope);
	TArray<uint8> FirstBytes;
	TArray<uint8> SecondBytes;
	const bool bEncodedFirst = FCodec::TryEncode(Envelope, FirstBytes);
	const bool bEncodedSecond = FCodec::TryEncode(Envelope, SecondBytes);
	FManifest Manifest;
	const bool bDecoded = FCodec::TryDecode(FirstBytes, Manifest);
	FEnvelope VerifiedEnvelope;
	const bool bDecodedVerified = FCodec::TryDecodeVerified(
		FirstBytes, Envelope, VerifiedEnvelope);
	FCheckpoint Checkpoint;
	const bool bUnwrapped = VerifiedEnvelope.TryUnwrap(Checkpoint);
	TestTrue(TEXT("empty envelope has one stable canonical manifest"),
		bMadeEnvelope
			&& bEncodedFirst
			&& bEncodedSecond
			&& FirstBytes.Num() == FCodec::EncodedSize()
			&& FirstBytes == SecondBytes
			&& FirstBytes[0] == static_cast<uint8>('S')
			&& FirstBytes[7] == static_cast<uint8>('1')
			&& bDecoded
			&& Manifest.GetSchemaVersion() == 1
			&& Manifest.GetEnvelopeId() == Envelope.GetEnvelopeId()
			&& Manifest.GetRecordCount() == 0
			&& FCodec::TryVerify(Manifest, Envelope)
			&& bDecodedVerified
			&& VerifiedEnvelope.GetEnvelopeId() == Envelope.GetEnvelopeId()
			&& VerifiedEnvelope.GetCheckpointId() == Envelope.GetCheckpointId()
			&& bUnwrapped
			&& Checkpoint.GetRecordCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryManifestCodecMalformedTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.MalformedAndUnsupportedFailClosed",
	SwordRhythmCueRetryManifestCodecFlags)

bool Fdemo_mapSwordRhythmCueRetryManifestCodecMalformedTest::RunTest(
	const FString&)
{
	FEnvelope Envelope;
	TArray<uint8> ValidBytes;
	const bool bPrepared = TryMakeEmptyEnvelope(Envelope)
		&& FCodec::TryEncode(Envelope, ValidBytes);
	TArray<uint8> Truncated = ValidBytes;
	Truncated.SetNum(FMath::Max(0, Truncated.Num() - 1));
	TArray<uint8> Extended = ValidBytes;
	Extended.Add(0);
	TArray<uint8> BadMagic = ValidBytes;
	BadMagic[0] ^= 0xff;
	TArray<uint8> Unsupported = ValidBytes;
	Unsupported[8] = 0;
	Unsupported[9] = 0;
	Unsupported[10] = 0;
	Unsupported[11] = 2;
	TArray<uint8> ZeroEnvelopeId = ValidBytes;
	for (int32 Index = 12; Index < 28; ++Index)
	{
		ZeroEnvelopeId[Index] = 0;
	}

	FManifest Manifest;
	const bool bTruncatedRejected = !FCodec::TryDecode(
		Truncated, Manifest) && !Manifest.IsValid();
	const bool bExtendedRejected = !FCodec::TryDecode(
		Extended, Manifest) && !Manifest.IsValid();
	const bool bMagicRejected = !FCodec::TryDecode(
		BadMagic, Manifest) && !Manifest.IsValid();
	const bool bVersionRejected = !FCodec::TryDecode(
		Unsupported, Manifest) && !Manifest.IsValid();
	const bool bZeroIdRejected = !FCodec::TryDecode(
		ZeroEnvelopeId, Manifest) && !Manifest.IsValid();
	FEnvelope Output = Envelope;
	const bool bVerifiedRejected = !FCodec::TryDecodeVerified(
		Unsupported, Envelope, Output) && !Output.IsValid();
	TestTrue(TEXT("size magic schema and required identity fail closed"),
		bPrepared
			&& bTruncatedRejected
			&& bExtendedRejected
			&& bMagicRejected
			&& bVersionRejected
			&& bZeroIdRejected
			&& bVerifiedRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryManifestCodecTamperTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.CandidateMismatchAndTamperingFailClosed",
	SwordRhythmCueRetryManifestCodecFlags)

bool Fdemo_mapSwordRhythmCueRetryManifestCodecTamperTest::RunTest(
	const FString&)
{
	FEnvelope Envelope;
	TArray<uint8> ValidBytes;
	const bool bPrepared = TryMakeEmptyEnvelope(Envelope)
		&& FCodec::TryEncode(Envelope, ValidBytes);
	TArray<uint8> EnvelopeIdTamper = ValidBytes;
	EnvelopeIdTamper[27] ^= 1;
	TArray<uint8> CheckpointIdTamper = ValidBytes;
	CheckpointIdTamper[43] ^= 1;
	TArray<uint8> CountTamper = ValidBytes;
	CountTamper[47] = 1;

	FManifest Original;
	FManifest EnvelopeIdManifest;
	FManifest CheckpointIdManifest;
	FManifest CountManifest;
	const bool bDecodedOriginal = FCodec::TryDecode(ValidBytes, Original);
	const bool bDecodedEnvelopeId = FCodec::TryDecode(
		EnvelopeIdTamper, EnvelopeIdManifest);
	const bool bDecodedCheckpointId = FCodec::TryDecode(
		CheckpointIdTamper, CheckpointIdManifest);
	const bool bDecodedCount = FCodec::TryDecode(
		CountTamper, CountManifest);
	FEnvelope Output = Envelope;
	const bool bVerifiedRejected = !FCodec::TryDecodeVerified(
		EnvelopeIdTamper, Envelope, Output) && !Output.IsValid();
	TestTrue(TEXT("structural metadata cannot impersonate the expected value"),
		bPrepared
			&& bDecodedOriginal
			&& FCodec::TryVerify(Original, Envelope)
			&& bDecodedEnvelopeId
			&& !FCodec::TryVerify(EnvelopeIdManifest, Envelope)
			&& bDecodedCheckpointId
			&& !FCodec::TryVerify(CheckpointIdManifest, Envelope)
			&& bDecodedCount
			&& !FCodec::TryVerify(CountManifest, Envelope)
			&& bVerifiedRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryManifestCodecOrderTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.RecordOrderChangesManifest",
	SwordRhythmCueRetryManifestCodecFlags)

bool Fdemo_mapSwordRhythmCueRetryManifestCodecOrderTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryManifestFixture Fixture;
	const auto Dispatch = PrepareManifestDispatch(Fixture, 0xBB100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryManifestFakeExecutor Visual(0xBB110000);
	FRetryManifestFakeExecutor Audio(
		0xBB120000, ERetryManifestFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand FirstCommand;
	FCommand SecondCommand;
	const bool bCapturedFirst = FCommand::TryCapture(
		FGuid(0xBB130001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeManifestDecisionRequest(0xBB140000, 1, 1),
		FirstCommand);
	const bool bCapturedSecond = FCommand::TryCapture(
		FGuid(0xBB150001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeManifestDecisionRequest(0xBB160000, 1, 1),
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
	TArray<uint8> ForwardBytes;
	TArray<uint8> ReversedBytes;
	FManifest ForwardManifest;
	FManifest ReversedManifest;
	const bool bEncodedForward = FCodec::TryEncode(
		ForwardEnvelope, ForwardBytes);
	const bool bEncodedReversed = FCodec::TryEncode(
		ReversedEnvelope, ReversedBytes);
	const bool bDecodedForward = FCodec::TryDecode(
		ForwardBytes, ForwardManifest);
	const bool bDecodedReversed = FCodec::TryDecode(
		ReversedBytes, ReversedManifest);
	FEnvelope CrossOutput;
	const bool bCrossRejected = !FCodec::TryDecodeVerified(
		ForwardBytes, ReversedEnvelope, CrossOutput);
	TestTrue(TEXT("record order remains visible in canonical bytes"),
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
			&& bEncodedForward
			&& bEncodedReversed
			&& ForwardBytes != ReversedBytes
			&& bDecodedForward
			&& bDecodedReversed
			&& !ForwardManifest.Matches(ReversedManifest)
			&& bCrossRejected
			&& !CrossOutput.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueRetryManifestCodecReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.VerifiedManifestPreservesLazyReplay",
	SwordRhythmCueRetryManifestCodecFlags)

bool Fdemo_mapSwordRhythmCueRetryManifestCodecReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueRetryManifestFixture Fixture;
	const auto Dispatch = PrepareManifestDispatch(Fixture, 0xBB200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FRetryManifestFakeExecutor Visual(0xBB210000);
	FRetryManifestFakeExecutor Audio(
		0xBB220000, ERetryManifestFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	FCommand Command;
	const bool bCaptured = FCommand::TryCapture(
		FGuid(0xBB230001, 0, 0, 1),
		Dispatch.Prepared,
		Host,
		MakeManifestDecisionRequest(0xBB240000, 2, 0),
		Command);
	FJournal Journal;
	const auto Recorded = Journal.TryExecute(Command, Host, Visual, Audio);
	const auto Export = FCheckpointService::Export(Journal);
	FEnvelope Envelope;
	const bool bWrapped = FEnvelope::TryWrap(
		1, Export.Checkpoint, Envelope);
	TArray<uint8> Bytes;
	const bool bEncoded = FCodec::TryEncode(Envelope, Bytes);
	FEnvelope VerifiedEnvelope;
	const bool bDecodedVerified = FCodec::TryDecodeVerified(
		Bytes, Envelope, VerifiedEnvelope);
	FCheckpoint Unwrapped;
	const bool bUnwrapped = VerifiedEnvelope.TryUnwrap(Unwrapped);
	FJournal Restored;
	const auto Restore = FCheckpointService::Restore(Unwrapped, Restored);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FRetryManifestFakeExecutor ReplayVisual(0xBB250000);
	FRetryManifestFakeExecutor ReplayAudio(0xBB260000);
	const auto Replay = Restored.TryExecute(
		Command, EmptyHost, ReplayVisual, ReplayAudio);
	TestTrue(TEXT("verified manifest preserves zero-dependency replay"),
		Dispatch.IsPrepared()
			&& !Initial.IsSuccess()
			&& bCaptured
			&& Recorded.Status == EJournalStatus::Recorded
			&& Recorded.Receipt.GetStep().Status == EStepStatus::RetryPending
			&& Export.IsSuccess()
			&& bWrapped
			&& bEncoded
			&& bDecodedVerified
			&& VerifiedEnvelope.GetEnvelopeId() == Envelope.GetEnvelopeId()
			&& VerifiedEnvelope.GetCheckpointId() == Envelope.GetCheckpointId()
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

#endif
