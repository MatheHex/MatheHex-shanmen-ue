#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueHandoffFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid HandoffRun(
		0xC1300001, 0xC1300002, 0xC1300003, 0xC1300004);
	const FGuid ForeignRun(
		0xC1300011, 0xC1300012, 0xC1300013, 0xC1300014);
	const FGuid FirstWeapon(
		0xC1310001, 0xC1310002, 0xC1310003, 0xC1310004);
	const FGuid SecondWeapon(
		0xC1310011, 0xC1310012, 0xC1310013, 0xC1310014);
	const FGuid VisualConsumer(
		0xC1320001, 0xC1320002, 0xC1320003, 0xC1320004);
	const FGuid AudioConsumer(
		0xC1330001, 0xC1330002, 0xC1330003, 0xC1330004);

	struct FSwordRhythmCueHandoffFixture
	{
		FGuid RunId;
		FGuid WeaponId;
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueHandoffFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("CueHandoffPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("CueHandoffPlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(RunId, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(RunId, Diagnostic)
				&& Session.TryBegin(RunId, Diagnostic);
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

		bool TryExecute(
			Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				WeaponId, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			if (!Action.IsExecuted() || !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic))
			{
				return false;
			}
			const auto Adapted =
				Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
					Session.GetPresentationState(),
					Session.GetConfig().GetEffectCuePolicy());
			if (!Adapted.IsAdapted())
			{
				Diagnostic = Adapted.Diagnostic;
				return false;
			}
			OutEvent = Adapted.Event;
			return true;
		}

		bool TryMakeInvocation(
			const Edemo_mapShanmenSwordRhythmEffectCueChannel Channel,
			const FGuid& AttemptId,
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				OutInvocation)
		{
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Discarded;
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
			if (!bReady || !TryExecute(Discarded)
				|| !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				|| !TryExecute(Discarded) || !TryExecute(Event)
				|| !Event.IsValid() || Event.NumCommands() != 2)
			{
				return false;
			}

			const bool bVisual = Channel
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual;
			const bool bAudio = Channel
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio;
			if (!bVisual && !bAudio)
			{
				return false;
			}
			Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Scope;
			Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Consumer;
			const FName RoleId = bVisual
				? Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					VisualConsumerRoleId()
				: Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
					AudioConsumerRoleId();
			const FGuid& ConsumerId = bVisual
				? VisualConsumer : AudioConsumer;
			if (!Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
					RunId, ConsumerId, RoleId, Scope)
				|| !Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::
					TryCreate(Scope, Consumer))
			{
				return false;
			}
			const auto Prepared = Consumer.Prepare(Event);
			return Prepared.IsRouted() && Prepared.Route.NumCommands() == 1
				&& Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::
					TryCreate(Prepared.Route, AttemptId, OutInvocation);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationHandoffVisualPublicationTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationHandoffExecutor.VisualPublication",
	SwordRhythmCueHandoffFlags)

bool Fdemo_mapSwordRhythmCuePresentationHandoffVisualPublicationTest::RunTest(
	const FString&)
{
	FSwordRhythmCueHandoffFixture Fixture(HandoffRun, FirstWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Invocation;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor Executor;
	TestTrue(TEXT("real visual invocation and executor are ready"),
		Fixture.TryMakeInvocation(
			Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
			FGuid(0xC1340001, 0, 0, 1), Invocation)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					HandoffRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
					Executor));

	const auto Result = Executor.Execute(Invocation);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Pending;
	TestTrue(TEXT("all-or-nothing publication returns succeeded evidence"),
		Result.IsSuccess()
			&& Result.Receipt.Matches(Invocation)
			&& Result.Receipt.GetOutcome()
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded
			&& Executor.TryGetPendingHandoff(Pending));
	TestTrue(TEXT("pending read model preserves the exact visual batch"),
		Pending.IsValid()
			&& Pending.GetInvocation().Matches(Invocation)
			&& Pending.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& Pending.GetCommands().Num() == 1
			&& Pending.GetCommands()[0].GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& Executor.GetAcceptedInvocationCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationHandoffScopeFencesTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationHandoffExecutor.ScopeFences",
	SwordRhythmCueHandoffFlags)

bool Fdemo_mapSwordRhythmCuePresentationHandoffScopeFencesTest::RunTest(
	const FString&)
{
	FSwordRhythmCueHandoffFixture Fixture(HandoffRun, FirstWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation VisualInvocation;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor
		AudioExecutor;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor
		ForeignRunExecutor;
	TestTrue(TEXT("scope fence fixtures are ready"),
		Fixture.TryMakeInvocation(
			Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
			FGuid(0xC1340011, 0, 0, 1), VisualInvocation)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					HandoffRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio,
					AudioExecutor)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					ForeignRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
					ForeignRunExecutor));

	const auto WrongChannel = AudioExecutor.Execute(VisualInvocation);
	const auto WrongRun = ForeignRunExecutor.Execute(VisualInvocation);
	TestTrue(TEXT("cross-channel and cross-Run invocations fail closed"),
		!WrongChannel.IsSuccess() && !WrongRun.IsSuccess()
			&& !AudioExecutor.HasPendingHandoff()
			&& !ForeignRunExecutor.HasPendingHandoff()
			&& AudioExecutor.GetAcceptedInvocationCount() == 0
			&& ForeignRunExecutor.GetAcceptedInvocationCount() == 0);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor Invalid;
	TestTrue(TEXT("invalid Run and invalid channel cannot create executors"),
		!Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
			TryCreate(
				FGuid(),
				Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
				Invalid)
			&& !Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					HandoffRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid,
					Invalid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationHandoffBackpressureTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationHandoffExecutor.Backpressure",
	SwordRhythmCueHandoffFlags)

bool Fdemo_mapSwordRhythmCuePresentationHandoffBackpressureTest::RunTest(
	const FString&)
{
	FSwordRhythmCueHandoffFixture FirstFixture(HandoffRun, FirstWeapon);
	FSwordRhythmCueHandoffFixture SecondFixture(HandoffRun, SecondWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation First;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Second;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor Executor;
	TestTrue(TEXT("two distinct visual batches are ready"),
		FirstFixture.TryMakeInvocation(
			Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
			FGuid(0xC1340021, 0, 0, 1), First)
			&& SecondFixture.TryMakeInvocation(
				Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
				FGuid(0xC1340022, 0, 0, 1), Second)
			&& !First.Matches(Second)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					HandoffRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
					Executor));

	const auto FirstResult = Executor.Execute(First);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Pending;
	TestTrue(TEXT("first batch occupies the bounded outbox"),
		FirstResult.IsSuccess()
			&& Executor.TryGetPendingHandoff(Pending));
	const auto Blocked = Executor.Execute(Second);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff StillPending;
	TestTrue(TEXT("a different batch cannot overwrite pending evidence"),
		!Blocked.IsSuccess()
			&& Executor.TryGetPendingHandoff(StillPending)
			&& StillPending.Matches(Pending)
			&& Executor.GetAcceptedInvocationCount() == 1);

	const auto Consumed = Executor.Consume(Pending.GetHandoffId());
	const auto SecondResult = Executor.Execute(Second);
	TestTrue(TEXT("explicit consumption releases capacity for the next batch"),
		Consumed.IsSuccess() && SecondResult.IsSuccess()
			&& Executor.HasPendingHandoff()
			&& Executor.GetAcceptedInvocationCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationHandoffReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationHandoffExecutor.ExactReplay",
	SwordRhythmCueHandoffFlags)

bool Fdemo_mapSwordRhythmCuePresentationHandoffReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueHandoffFixture Fixture(HandoffRun, FirstWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Invocation;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor Executor;
	TestTrue(TEXT("replay fixture is ready"),
		Fixture.TryMakeInvocation(
			Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
			FGuid(0xC1340031, 0, 0, 1), Invocation)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					HandoffRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
					Executor));

	const auto First = Executor.Execute(Invocation);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Pending;
	TestTrue(TEXT("first handoff is published"),
		First.IsSuccess() && Executor.TryGetPendingHandoff(Pending));
	const auto Consumed = Executor.Consume(Pending.GetHandoffId());
	const auto Replayed = Executor.Execute(Invocation);
	const auto ConsumeReplayed = Executor.Consume(Pending.GetHandoffId());
	TestTrue(TEXT("exact invocation and consume replay preserve evidence"),
		Consumed.Status
				== Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
					Consumed
			&& ConsumeReplayed.Status
				== Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
					AlreadyConsumed
			&& Replayed.IsSuccess()
			&& Replayed.Receipt.GetReceiptId()
				== First.Receipt.GetReceiptId()
			&& Replayed.Receipt.GetExecutorReceiptId()
				== First.Receipt.GetExecutorReceiptId()
			&& Executor.GetAcceptedInvocationCount() == 1
			&& !Executor.HasPendingHandoff());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationHandoffAudioResetTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationHandoffExecutor.AudioAndReset",
	SwordRhythmCueHandoffFlags)

bool Fdemo_mapSwordRhythmCuePresentationHandoffAudioResetTest::RunTest(
	const FString&)
{
	FSwordRhythmCueHandoffFixture Fixture(HandoffRun, FirstWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Invocation;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor Executor;
	TestTrue(TEXT("real audio invocation and executor are ready"),
		Fixture.TryMakeInvocation(
			Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio,
			FGuid(0xC1340041, 0, 0, 1), Invocation)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
				TryCreate(
					HandoffRun,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio,
					Executor));
	const auto Published = Executor.Execute(Invocation);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Pending;
	TestTrue(TEXT("audio batch remains typed and caller-readable"),
		Published.IsSuccess() && Executor.TryGetPendingHandoff(Pending)
			&& Pending.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio
			&& Pending.GetCommands().Num() == 1
			&& Pending.GetCommands()[0].GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio);
	const auto WrongConsume =
		Executor.Consume(FGuid(0xC13FFFFF, 0, 0, 1));
	TestTrue(TEXT("wrong consumption identity preserves the pending batch"),
		!WrongConsume.IsSuccess() && Executor.HasPendingHandoff());
	Executor.Reset();
	TestTrue(TEXT("Run teardown reset removes all executor state"),
		!Executor.IsValid() && !Executor.HasPendingHandoff()
			&& Executor.GetAcceptedInvocationCount() == 0
			&& !Executor.Execute(Invocation).IsSuccess());
	return true;
}

#endif
