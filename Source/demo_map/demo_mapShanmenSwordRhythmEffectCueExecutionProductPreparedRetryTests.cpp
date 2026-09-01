#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductPreparedRetryFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB4000001, 0xB4000002, 0xB4000003, 0xB4000004);
	const FGuid TestWeapon(
		0xB4010001, 0xB4010002, 0xB4010003, 0xB4010004);

	using FDispatchSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FRetrySeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed;
	using EPreparedStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus;
	using EPrepareStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus;
	using EChannels =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryChannels;

	FDispatchSeed MakeDispatchSeed(const uint32 Value)
	{
		FDispatchSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	FRetrySeed MakeRetrySeed(const uint32 Value)
	{
		FRetrySeed Seed;
		Seed.RetrySeed = FGuid(Value, 0, 0, 1);
		return Seed;
	}

	struct FSwordRhythmCueProductPreparedRetryFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductPreparedRetryFixture()
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

	enum class EPreparedRetryFakeMode : uint8
	{
		Success,
		RetryableFailure
	};

	class FProductPreparedRetryFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductPreparedRetryFakeExecutor(
			const uint32 InReceiptSeed,
			const EPreparedRetryFakeMode InMode =
				EPreparedRetryFakeMode::Success)
			: Mode(InMode)
			, ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;
		EPreparedRetryFakeMode Mode = EPreparedRetryFakeMode::Success;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			const auto Outcome = Mode == EPreparedRetryFakeMode::Success
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
					"Prepared-retry fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Prepared-retry fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareDispatch(
		FSwordRhythmCueProductPreparedRetryFixture& Fixture,
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
	Fdemo_mapSwordRhythmCueProductPreparedRetryAudioTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry.RenewAudioPreserveVisual",
	SwordRhythmCueProductPreparedRetryFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedRetryAudioTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedRetryFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB4100000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedRetryFakeExecutor Visual(0xB4110000);
	FProductPreparedRetryFakeExecutor Audio(
		0xB4120000, EPreparedRetryFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4130000));
	const auto& InitialIdentity =
		Dispatch.Prepared.GetPlan().GetTransactionIdentity();
	const auto& RetryCommand = Retry.Prepared.GetRetryProcessEnvelope().GetCommand();
	TestTrue(TEXT("only retryable Audio receives a fresh attempt identity"),
		Dispatch.IsPrepared()
			&& Initial.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& Retry.IsPrepared()
			&& Retry.Prepared.GetRenewedChannels() == EChannels::Audio
			&& !Retry.Prepared.RenewsVisual()
			&& Retry.Prepared.RenewsAudio()
			&& Retry.Prepared.GetSourceProcessEnvelope().GetSequence() == 1
			&& Retry.Prepared.GetRetryProcessEnvelope().GetSequence() == 2
			&& Retry.Prepared.GetEndEnvelope().GetSequence() == 3
			&& RetryCommand.GetVisualAttemptId()
				== InitialIdentity.VisualAttemptId
			&& RetryCommand.GetAudioAttemptId()
				!= InitialIdentity.AudioAttemptId);

	Audio.Mode = EPreparedRetryFakeMode::Success;
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				Retry.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("Audio renewal completes without Visual executor re-entry"),
		Completed.Status == EPreparedStatus::Completed
			&& Completed.IsCompleted()
			&& Completed.Process.Router.Session.Host.Visual.IsAcknowledged()
			&& Completed.Process.Router.Session.Host.Audio.IsAcknowledged()
			&& Host.IsTerminal()
			&& Host.GetRecordCount() == 4
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedRetryRepeatTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry.RepeatPendingAndReplay",
	SwordRhythmCueProductPreparedRetryFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedRetryRepeatTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedRetryFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB4200000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedRetryFakeExecutor Visual(0xB4210000);
	FProductPreparedRetryFakeExecutor Audio(
		0xB4220000, EPreparedRetryFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto FirstRetry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4230000));
	const auto StillPending =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				FirstRetry.Prepared, Host, Visual, Audio);
	const int32 VisualBeforeReplay = Visual.InvocationCount;
	const int32 AudioBeforeReplay = Audio.InvocationCount;
	const auto PendingReplay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				FirstRetry.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("same retry receipt replays without another executor call"),
		Initial.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					DispatchIncomplete
			&& FirstRetry.IsPrepared()
			&& StillPending.Status == EPreparedStatus::RetryPending
			&& PendingReplay.Status == EPreparedStatus::RetryPending
			&& PendingReplay.Process.IsReplay()
			&& Host.GetNextSequence() == 3
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);

	const auto SecondRetry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4240000));
	Audio.Mode = EPreparedRetryFakeMode::Success;
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				SecondRetry.Prepared, Host, Visual, Audio);
	const int32 VisualBeforeCompleteReplay = Visual.InvocationCount;
	const int32 AudioBeforeCompleteReplay = Audio.InvocationCount;
	const auto CompleteReplay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				SecondRetry.Prepared, Host, Visual, Audio);
	const auto Stale =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				FirstRetry.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("new seed renews the latest failed attempt and terminal replay is inert"),
		SecondRetry.IsPrepared()
			&& SecondRetry.Prepared.GetSourceProcessEnvelope().GetSequence() == 2
			&& SecondRetry.Prepared.GetRetryProcessEnvelope().GetSequence() == 3
			&& SecondRetry.Prepared.GetRetryProcessEnvelope().GetCommand().
				GetAudioAttemptId()
				!= FirstRetry.Prepared.GetRetryProcessEnvelope().GetCommand().
					GetAudioAttemptId()
			&& Completed.IsCompleted()
			&& Host.GetRecordCount() == 5
			&& Visual.InvocationCount == 1
			&& Audio.InvocationCount == 3
			&& CompleteReplay.Status == EPreparedStatus::Replayed
			&& CompleteReplay.IsCompleted()
			&& Visual.InvocationCount == VisualBeforeCompleteReplay
			&& Audio.InvocationCount == AudioBeforeCompleteReplay
			&& Stale.Status == EPreparedStatus::HostStateMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedRetryVisualTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry.DeterministicVisualRenewal",
	SwordRhythmCueProductPreparedRetryFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedRetryVisualTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedRetryFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB4300000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedRetryFakeExecutor Visual(
		0xB4310000, EPreparedRetryFakeMode::RetryableFailure);
	FProductPreparedRetryFakeExecutor Audio(0xB4320000);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4330000));
	const auto Same =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4330000));
	const auto Different =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4340000));
	const auto& InitialIdentity =
		Dispatch.Prepared.GetPlan().GetTransactionIdentity();
	TestTrue(TEXT("same source and seed reproduce one Visual-only renewal"),
		Initial.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& First.IsPrepared()
			&& Same.IsPrepared()
			&& Different.IsPrepared()
			&& First.Prepared.Matches(Same.Prepared)
			&& !First.Prepared.Matches(Different.Prepared)
			&& First.Prepared.GetRenewedChannels() == EChannels::Visual
			&& First.Prepared.GetRetryProcessEnvelope().GetCommand().
				GetVisualAttemptId()
				!= InitialIdentity.VisualAttemptId
			&& First.Prepared.GetRetryProcessEnvelope().GetCommand().
				GetAudioAttemptId()
				== InitialIdentity.AudioAttemptId);

	Visual.Mode = EPreparedRetryFakeMode::Success;
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				First.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("Visual renewal preserves Audio acknowledgement evidence"),
		Completed.IsCompleted()
			&& Visual.InvocationCount == 2
			&& Audio.InvocationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedRetryResumeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry.ResumeEndAfterDurableProcess",
	SwordRhythmCueProductPreparedRetryFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedRetryResumeTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedRetryFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB4400000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedRetryFakeExecutor Visual(0xB4410000);
	FProductPreparedRetryFakeExecutor Audio(
		0xB4420000, EPreparedRetryFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4430000));
	Audio.Mode = EPreparedRetryFakeMode::Success;
	const auto Process = Host.TryRouteProcessNext(
		Retry.Prepared.GetRetryProcessEnvelope(), Visual, Audio);
	const int32 VisualBeforeResume = Visual.InvocationCount;
	const int32 AudioBeforeResume = Audio.InvocationCount;
	const auto Resumed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				Retry.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("durable completed Process resumes only the End fence"),
		Initial.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& Retry.IsPrepared()
			&& Process.IsSuccess()
			&& Process.Router.Session.IsBatchComplete()
			&& !Host.IsEmpty()
			&& Resumed.Status == EPreparedStatus::Resumed
			&& Resumed.IsCompleted()
			&& Resumed.Process.IsReplay()
			&& !Resumed.End.IsReplay()
			&& Visual.InvocationCount == VisualBeforeResume
			&& Audio.InvocationCount == AudioBeforeResume);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedRetryBothTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry.RenewBothPendingChannels",
	SwordRhythmCueProductPreparedRetryFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedRetryBothTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductPreparedRetryFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB4500000);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductPreparedRetryFakeExecutor Visual(
		0xB4510000, EPreparedRetryFakeMode::RetryableFailure);
	FProductPreparedRetryFakeExecutor Audio(
		0xB4520000, EPreparedRetryFakeMode::RetryableFailure);
	const auto Initial =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(Dispatch.Prepared, Host, Visual, Audio);
	const auto Retry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, Host, MakeRetrySeed(0xB4530000));
	const auto& InitialCommand =
		Initial.Dispatch.Transaction.Process.Router.Session.Event;
	const auto& RetryCommand = Retry.Prepared.GetRetryProcessEnvelope().GetCommand();
	const auto& Identity = Dispatch.Prepared.GetPlan().GetTransactionIdentity();
	TestTrue(TEXT("both pending channels receive isolated renewed attempts"),
		InitialCommand.IsValid()
			&& Retry.IsPrepared()
			&& Retry.Prepared.GetRenewedChannels() == EChannels::VisualAndAudio
			&& RetryCommand.GetVisualAttemptId() != Identity.VisualAttemptId
			&& RetryCommand.GetAudioAttemptId() != Identity.AudioAttemptId
			&& RetryCommand.GetVisualAttemptId()
				!= RetryCommand.GetAudioAttemptId());
	Visual.Mode = EPreparedRetryFakeMode::Success;
	Audio.Mode = EPreparedRetryFakeMode::Success;
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				Retry.Prepared, Host, Visual, Audio);
	TestTrue(TEXT("both renewed channels complete in one caller pass"),
		Completed.IsCompleted()
			&& Visual.InvocationCount == 2
			&& Audio.InvocationCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductPreparedRetryFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry.InvalidForeignAndTerminalFences",
	SwordRhythmCueProductPreparedRetryFlags)

bool Fdemo_mapSwordRhythmCueProductPreparedRetryFenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry Invalid;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost EmptyHost;
	FProductPreparedRetryFakeExecutor EmptyVisual(0xB4600000);
	FProductPreparedRetryFakeExecutor EmptyAudio(0xB4610000);
	const auto InvalidExecution =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				Invalid, EmptyHost, EmptyVisual, EmptyAudio);
	TestTrue(TEXT("invalid value rejects before Host or executor mutation"),
		InvalidExecution.Status == EPreparedStatus::PreparedRejected
			&& EmptyHost.IsEmpty()
			&& EmptyVisual.InvocationCount == 0
			&& EmptyAudio.InvocationCount == 0);

	FSwordRhythmCueProductPreparedRetryFixture Fixture;
	const auto Dispatch = PrepareDispatch(Fixture, 0xB4620000);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Fixture.Session, MakeDispatchSeed(0xB4630000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost RetryHost;
	FProductPreparedRetryFakeExecutor RetryVisual(0xB4640000);
	FProductPreparedRetryFakeExecutor RetryAudio(
		0xB4650000, EPreparedRetryFakeMode::RetryableFailure);
	const auto Pending =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared, RetryHost, RetryVisual, RetryAudio);
	const auto InvalidSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared, RetryHost, FRetrySeed());
	const auto ForeignRoot =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Foreign.Prepared, RetryHost, MakeRetrySeed(0xB4660000));
	TestTrue(TEXT("invalid seed and foreign prepared root cannot continue Host"),
		Pending.Dispatch.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					ProcessRetryPending
			&& InvalidSeed.Status == EPrepareStatus::SeedInvalid
			&& ForeignRoot.Status == EPrepareStatus::PreparedRootMismatch
			&& RetryHost.GetRecordCount() == 2
			&& RetryVisual.InvocationCount == 1
			&& RetryAudio.InvocationCount == 1);

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost TerminalHost;
	FProductPreparedRetryFakeExecutor TerminalVisual(0xB4670000);
	FProductPreparedRetryFakeExecutor TerminalAudio(0xB4680000);
	const auto Completed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Dispatch.Prepared,
				TerminalHost,
				TerminalVisual,
				TerminalAudio);
	const auto TerminalRetry =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(
				Dispatch.Prepared,
				TerminalHost,
				MakeRetrySeed(0xB4690000));
	TestTrue(TEXT("completed Host exposes no retry continuation"),
		Completed.IsSuccess()
			&& TerminalHost.IsTerminal()
			&& TerminalRetry.Status == EPrepareStatus::RetryStateUnavailable);
	return true;
}

#endif
