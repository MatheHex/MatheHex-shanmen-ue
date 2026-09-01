#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCuePresentationRunController.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCuePresentationRunFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ControllerRun(
		0xC1310001, 0xC1310002, 0xC1310003, 0xC1310004);
	const FGuid ForeignRun(
		0xC1310011, 0xC1310012, 0xC1310013, 0xC1310014);
	const FGuid GapRun(
		0xC1310021, 0xC1310022, 0xC1310023, 0xC1310024);
	const FGuid ControllerWeapon(
		0xC1311001, 0xC1311002, 0xC1311003, 0xC1311004);
	const FGuid ForeignWeapon(
		0xC1311011, 0xC1311012, 0xC1311013, 0xC1311014);
	const FGuid GapWeapon(
		0xC1311021, 0xC1311022, 0xC1311023, 0xC1311024);

	using FController =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController;
	using FPublishResult =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult;
	using EPublishStatus =
		Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus;

	struct FSwordRhythmCuePresentationRunFixture
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

		FSwordRhythmCuePresentationRunFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
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

		bool TryObserve()
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				WeaponId, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			return bReady && Action.IsExecuted()
				&& Timeline.TryCapture(Sample)
				&& Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic);
		}

		int32 CurrentCueCommandCount() const
		{
			if (!Session.IsValid() || Session.IsEmpty())
			{
				return INDEX_NONE;
			}
			const auto Adapted =
				Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
					Session.GetPresentationState(),
					Session.GetConfig().GetEffectCuePolicy());
			return Adapted.IsAdapted()
				? Adapted.Event.NumCommands()
				: INDEX_NONE;
		}

		bool TryObserveAndPublish(
			FController& Controller,
			FPublishResult& OutResult)
		{
			if (!TryObserve())
			{
				return false;
			}
			OutResult = Controller.TryPublishCurrent(Session);
			return OutResult.IsSuccess();
		}

		bool TryReachNextCue(
			FController& Controller,
			const bool bOpenLinkWindow,
			int32& OutRevision)
		{
			OutRevision = 0;
			if (bOpenLinkWindow
				&& !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks()))
			{
				return false;
			}
			for (int32 Attempt = 0; Attempt < 8; ++Attempt)
			{
				FPublishResult Published;
				if (!TryObserveAndPublish(Controller, Published))
				{
					return false;
				}
				if (CurrentCueCommandCount() > 0)
				{
					OutRevision = Published.ObservationRevision;
					return OutRevision > 0;
				}
			}
			return false;
		}

		bool TryReachFirstCue(
			FController& Controller,
			int32& OutRevision)
		{
			FPublishResult First;
			if (!TryObserveAndPublish(Controller, First))
			{
				return false;
			}
			if (CurrentCueCommandCount() > 0)
			{
				OutRevision = First.ObservationRevision;
				return true;
			}
			return TryReachNextCue(Controller, true, OutRevision);
		}

		bool TryQueueNextRevision(
			FController& Controller,
			int32& OutRevision)
		{
			FPublishResult Published;
			if (!TryObserveAndPublish(Controller, Published))
			{
				return false;
			}
			OutRevision = Published.ObservationRevision;
			return Published.Status == EPublishStatus::Queued
				&& OutRevision > 0;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationRunFirstPublicationTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController.FirstPublication",
	SwordRhythmCuePresentationRunFlags)

bool Fdemo_mapSwordRhythmCuePresentationRunFirstPublicationTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationRunFixture Fixture(
		ControllerRun, ControllerWeapon);
	FController Controller;
	FString Diagnostic;
	int32 CueRevision = 0;
	TestTrue(TEXT("controller and first real cue are published"),
		Fixture.bReady
			&& Controller.TryBegin(ControllerRun, Diagnostic)
			&& Fixture.TryReachFirstCue(Controller, CueRevision));

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("both typed channel handoffs are caller-readable"),
		Controller.IsValid() && CueRevision > 0
			&& Controller.GetLastCapturedRevision() == CueRevision
			&& Controller.GetLastPublishedRevision() == CueRevision
			&& Controller.GetPublishedDispatchCount() == CueRevision
			&& Controller.GetQueuedDispatchCount() == 0
			&& Controller.TryGetPendingVisualHandoff(Visual)
			&& Controller.TryGetPendingAudioHandoff(Audio)
			&& Visual.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& Audio.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio
			&& Visual.GetCommands().Num() > 0
			&& Audio.GetCommands().Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationRunFifoTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController.FifoBackpressure",
	SwordRhythmCuePresentationRunFlags)

bool Fdemo_mapSwordRhythmCuePresentationRunFifoTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationRunFixture Fixture(
		ControllerRun, ControllerWeapon);
	FController Controller;
	FString Diagnostic;
	int32 FirstCueRevision = 0;
	int32 NextCueRevision = 0;
	TestTrue(TEXT("first cue occupies both presentation outboxes"),
		Fixture.bReady
			&& Controller.TryBegin(ControllerRun, Diagnostic)
			&& Fixture.TryReachFirstCue(Controller, FirstCueRevision));
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff FirstVisual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff FirstAudio;
	TestTrue(TEXT("first handoff pair is captured"),
		Controller.TryGetPendingVisualHandoff(FirstVisual)
			&& Controller.TryGetPendingAudioHandoff(FirstAudio));
	TestTrue(TEXT("the next observed revision freezes in FIFO"),
		Fixture.TryQueueNextRevision(Controller, NextCueRevision)
			&& NextCueRevision > FirstCueRevision
			&& Controller.GetLastCapturedRevision() == NextCueRevision
			&& Controller.GetLastPublishedRevision() == FirstCueRevision
			&& Controller.GetQueuedDispatchCount()
				== NextCueRevision - FirstCueRevision);

	TestTrue(TEXT("one-channel consumption cannot advance the FIFO"),
		Controller.TryConsumeVisualHandoff(
			FirstVisual.GetHandoffId(), Diagnostic)
			&& !Controller.HasPendingVisualHandoff()
			&& Controller.HasPendingAudioHandoff()
			&& Controller.GetLastPublishedRevision() == FirstCueRevision);
	TestTrue(TEXT("second-channel consumption pumps every unblocked revision"),
		Controller.TryConsumeAudioHandoff(
			FirstAudio.GetHandoffId(), Diagnostic)
			&& Controller.GetLastPublishedRevision() == NextCueRevision
			&& Controller.GetPublishedDispatchCount() == NextCueRevision
			&& Controller.GetQueuedDispatchCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationRunIdentityTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController.IdentityAndRevisionFences",
	SwordRhythmCuePresentationRunFlags)

bool Fdemo_mapSwordRhythmCuePresentationRunIdentityTest::RunTest(
	const FString&)
{
	FController Invalid;
	FString Diagnostic;
	TestTrue(TEXT("invalid Run cannot bind"),
		!Invalid.TryBegin(FGuid(), Diagnostic) && Invalid.IsEmpty());

	FSwordRhythmCuePresentationRunFixture Fixture(
		ControllerRun, ControllerWeapon);
	FController Controller;
	FPublishResult First;
	TestTrue(TEXT("first contiguous revision is accepted"),
		Fixture.bReady
			&& Controller.TryBegin(ControllerRun, Diagnostic)
			&& Fixture.TryObserveAndPublish(Controller, First));
	const auto Duplicate = Controller.TryPublishCurrent(Fixture.Session);
	TestTrue(TEXT("exact current revision is idempotent"),
		Duplicate.Status == EPublishStatus::AlreadyCaptured
			&& Duplicate.IsSuccess()
			&& Controller.GetLastCapturedRevision() == 1
			&& Controller.GetPublishedDispatchCount() == 1);

	FSwordRhythmCuePresentationRunFixture Foreign(
		ForeignRun, ForeignWeapon);
	TestTrue(TEXT("foreign fixture observes one revision"),
		Foreign.TryObserve());
	const auto WrongRun = Controller.TryPublishCurrent(Foreign.Session);
	TestTrue(TEXT("cross-Run ProductSession fails without state change"),
		WrongRun.Status == EPublishStatus::RunMismatch
			&& !WrongRun.IsSuccess()
			&& Controller.GetLastCapturedRevision() == 1);

	FSwordRhythmCuePresentationRunFixture Gap(GapRun, GapWeapon);
	FController GapController;
	TestTrue(TEXT("gap fixture advances twice before capture"),
		Gap.bReady && GapController.TryBegin(GapRun, Diagnostic)
			&& Gap.TryObserve() && Gap.TryObserve());
	const auto GapResult = GapController.TryPublishCurrent(Gap.Session);
	TestTrue(TEXT("missing revision fails closed instead of coalescing"),
		GapResult.Status == EPublishStatus::RevisionConflict
			&& !GapResult.IsSuccess()
			&& GapController.GetLastCapturedRevision() == 0
			&& GapController.GetQueuedDispatchCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationRunConsumeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController.ExactConsumption",
	SwordRhythmCuePresentationRunFlags)

bool Fdemo_mapSwordRhythmCuePresentationRunConsumeTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationRunFixture Fixture(
		ControllerRun, ControllerWeapon);
	FController Controller;
	FString Diagnostic;
	int32 CueRevision = 0;
	TestTrue(TEXT("cue handoff pair is ready"),
		Fixture.bReady
			&& Controller.TryBegin(ControllerRun, Diagnostic)
			&& Fixture.TryReachFirstCue(Controller, CueRevision));
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("pending identities are available"),
		Controller.TryGetPendingVisualHandoff(Visual)
			&& Controller.TryGetPendingAudioHandoff(Audio));
	TestTrue(TEXT("wrong identity preserves both pending handoffs"),
		!Controller.TryConsumeVisualHandoff(
			FGuid(0xC131FFFF, 0, 0, 1), Diagnostic)
			&& Controller.HasPendingVisualHandoff()
			&& Controller.HasPendingAudioHandoff());
	TestTrue(TEXT("exact visual consume and replay remain idempotent"),
		Controller.TryConsumeVisualHandoff(
			Visual.GetHandoffId(), Diagnostic)
			&& Controller.TryConsumeVisualHandoff(
				Visual.GetHandoffId(), Diagnostic)
			&& !Controller.HasPendingVisualHandoff()
			&& Controller.HasPendingAudioHandoff());
	TestTrue(TEXT("exact audio consume releases the complete pair"),
		Controller.TryConsumeAudioHandoff(
			Audio.GetHandoffId(), Diagnostic)
			&& !Controller.HasPendingVisualHandoff()
			&& !Controller.HasPendingAudioHandoff()
			&& Controller.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationRunTeardownTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController.RunTeardown",
	SwordRhythmCuePresentationRunFlags)

bool Fdemo_mapSwordRhythmCuePresentationRunTeardownTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationRunFixture Fixture(
		ControllerRun, ControllerWeapon);
	FController Controller;
	FString Diagnostic;
	int32 FirstCueRevision = 0;
	int32 NextCueRevision = 0;
	TestTrue(TEXT("pending and queued Run state is ready"),
		Fixture.bReady
			&& Controller.TryBegin(ControllerRun, Diagnostic)
			&& Fixture.TryReachFirstCue(Controller, FirstCueRevision)
			&& Fixture.TryQueueNextRevision(
				Controller, NextCueRevision));
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary Summary;
	TestTrue(TEXT("foreign teardown identity cannot clear active state"),
		!Controller.TryEnd(ForeignRun, Summary, Diagnostic)
			&& !Controller.IsEmpty());
	TestTrue(TEXT("exact teardown records and clears all presentation state"),
		Controller.TryEnd(ControllerRun, Summary, Diagnostic)
			&& Summary.RunId == ControllerRun
			&& Summary.CapturedDispatchCount == NextCueRevision
			&& Summary.PublishedDispatchCount == FirstCueRevision
			&& Summary.QueuedDispatchCount
				== NextCueRevision - FirstCueRevision
			&& Summary.bVisualPending && Summary.bAudioPending
			&& Controller.IsEmpty() && Controller.IsValid());
	TestTrue(TEXT("cleared controller can bind a later Run"),
		Controller.TryBegin(ForeignRun, Diagnostic)
			&& Controller.GetRunId() == ForeignRun
			&& Controller.GetLastCapturedRevision() == 0);
	return true;
}

#endif
