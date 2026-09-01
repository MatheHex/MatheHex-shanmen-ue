#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.h"
#include "demo_mapShanmenSwordRhythmEffectCuePresentationRunController.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCuePresentationAckFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid AckRun(
		0xC1320001, 0xC1320002, 0xC1320003, 0xC1320004);
	const FGuid AckWeapon(
		0xC1321001, 0xC1321002, 0xC1321003, 0xC1321004);
	const FName VisualPresenter(
		TEXT("Presentation.Blueprint.SwordRhythm.Visual.r1"));
	const FName AudioPresenter(
		TEXT("Presentation.Blueprint.SwordRhythm.Audio.r1"));
	const FName AlternatePresenter(
		TEXT("Presentation.Blueprint.SwordRhythm.Alternate.r1"));

	using FController =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController;
	using FAcknowledgement =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement;

	struct FSwordRhythmCuePresentationAckFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCuePresentationAckFixture()
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
					AckRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(AckRun, Diagnostic)
				&& Session.TryBegin(AckRun, Diagnostic);
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

		bool TryObserveAndPublish(FController& Controller)
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				AckWeapon, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			if (!bReady || !Action.IsExecuted() || !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic))
			{
				return false;
			}
			return Controller.TryPublishCurrent(Session).IsSuccess();
		}

		bool TryReachFirstCue(FController& Controller)
		{
			if (!bReady || !Controller.TryBegin(AckRun, Diagnostic)
				|| !TryObserveAndPublish(Controller))
			{
				return false;
			}
			if (Controller.HasPendingVisualHandoff()
				&& Controller.HasPendingAudioHandoff())
			{
				return true;
			}
			if (!TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks()))
			{
				return false;
			}
			for (int32 Attempt = 0; Attempt < 8; ++Attempt)
			{
				if (!TryObserveAndPublish(Controller))
				{
					return false;
				}
				if (Controller.HasPendingVisualHandoff()
					&& Controller.HasPendingAudioHandoff())
				{
					return true;
				}
			}
			return false;
		}

		bool TryGetPair(
			FController& Controller,
			Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutVisual,
			Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutAudio)
		{
			return TryReachFirstCue(Controller)
				&& Controller.TryGetPendingVisualHandoff(OutVisual)
				&& Controller.TryGetPendingAudioHandoff(OutAudio);
		}
	};

	bool TryMakeAcknowledgement(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff,
		const FName PresenterDefinitionId,
		FAcknowledgement& OutAcknowledgement,
		FString& OutDiagnostic)
	{
		return
			Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
				TryAcknowledge(
					Handoff,
					PresenterDefinitionId,
					Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
						GetOrderedCommandIds(Handoff),
					OutAcknowledgement,
					OutDiagnostic);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationVisualAckTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationAcknowledgement.VisualExactBatch",
	SwordRhythmCuePresentationAckFlags)

bool Fdemo_mapSwordRhythmCuePresentationVisualAckTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationAckFixture Fixture;
	FController Controller;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("a real channel pair reaches the caller boundary"),
		Fixture.TryGetPair(Controller, Visual, Audio));

	FAcknowledgement Acknowledgement;
	FString Diagnostic;
	TestTrue(TEXT("exact ordered visual completion is sealed"),
		TryMakeAcknowledgement(
			Visual, VisualPresenter, Acknowledgement, Diagnostic)
			&& Acknowledgement.IsValid()
			&& Acknowledgement.Matches(Visual)
			&& Acknowledgement.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual);
	TestTrue(TEXT("visual acknowledgement releases only the visual slot"),
		Controller.TryAcknowledgeVisualHandoff(
			Acknowledgement, Diagnostic)
			&& !Controller.HasPendingVisualHandoff()
			&& Controller.HasPendingAudioHandoff());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationAudioAckTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationAcknowledgement.AudioExactBatch",
	SwordRhythmCuePresentationAckFlags)

bool Fdemo_mapSwordRhythmCuePresentationAudioAckTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationAckFixture Fixture;
	FController Controller;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("a real channel pair reaches the caller boundary"),
		Fixture.TryGetPair(Controller, Visual, Audio));

	FAcknowledgement Acknowledgement;
	FString Diagnostic;
	TestTrue(TEXT("exact ordered audio completion is sealed"),
		TryMakeAcknowledgement(
			Audio, AudioPresenter, Acknowledgement, Diagnostic)
			&& Acknowledgement.IsValid()
			&& Acknowledgement.Matches(Audio)
			&& Acknowledgement.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio);
	TestTrue(TEXT("audio acknowledgement releases only the audio slot"),
		Controller.TryAcknowledgeAudioHandoff(
			Acknowledgement, Diagnostic)
			&& Controller.HasPendingVisualHandoff()
			&& !Controller.HasPendingAudioHandoff());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationAckFencesTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationAcknowledgement.BatchAndChannelFences",
	SwordRhythmCuePresentationAckFlags)

bool Fdemo_mapSwordRhythmCuePresentationAckFencesTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationAckFixture Fixture;
	FController Controller;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("a real channel pair reaches the caller boundary"),
		Fixture.TryGetPair(Controller, Visual, Audio));

	const TArray<FGuid> Exact =
		Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
			GetOrderedCommandIds(Visual);
	FAcknowledgement Rejected;
	FString Diagnostic;
	TestTrue(TEXT("missing presenter identity fails closed"),
		!Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
			TryAcknowledge(
				Visual, NAME_None, Exact, Rejected, Diagnostic)
			&& !Rejected.IsValid());
	TestTrue(TEXT("missing completed commands fail closed"),
		!Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
			TryAcknowledge(
				Visual, VisualPresenter, {}, Rejected, Diagnostic)
			&& !Rejected.IsValid());
	TArray<FGuid> Wrong = Exact;
	if (!Wrong.IsEmpty())
	{
		Wrong[0] = FGuid(0xC132FFFF, 0, 0, 1);
	}
	TestTrue(TEXT("foreign command identity fails closed"),
		!Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
			TryAcknowledge(
				Visual, VisualPresenter, Wrong, Rejected, Diagnostic)
			&& !Rejected.IsValid());
	TArray<FGuid> Duplicate = Exact;
	Duplicate.Append(Exact);
	TestTrue(TEXT("duplicate completion cannot inflate the exact batch"),
		!Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
			TryAcknowledge(
				Visual, VisualPresenter, Duplicate, Rejected, Diagnostic)
			&& !Rejected.IsValid());

	FAcknowledgement VisualAck;
	TestTrue(TEXT("one exact visual acknowledgement is valid"),
		TryMakeAcknowledgement(
			Visual, VisualPresenter, VisualAck, Diagnostic));
	TestTrue(TEXT("a visual acknowledgement cannot release the audio slot"),
		!Controller.TryAcknowledgeAudioHandoff(VisualAck, Diagnostic)
			&& Controller.HasPendingVisualHandoff()
			&& Controller.HasPendingAudioHandoff());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationAckDeterminismTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationAcknowledgement.DeterminismAndPresenterIdentity",
	SwordRhythmCuePresentationAckFlags)

bool Fdemo_mapSwordRhythmCuePresentationAckDeterminismTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationAckFixture Fixture;
	FController Controller;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("a real channel pair reaches the caller boundary"),
		Fixture.TryGetPair(Controller, Visual, Audio));

	FAcknowledgement First;
	FAcknowledgement Replay;
	FAcknowledgement Alternate;
	FString Diagnostic;
	TestTrue(TEXT("same authored presenter and batch replay exactly"),
		TryMakeAcknowledgement(
			Visual, VisualPresenter, First, Diagnostic)
			&& TryMakeAcknowledgement(
				Visual, VisualPresenter, Replay, Diagnostic)
			&& First.Matches(Replay));
	TestTrue(TEXT("presenter identity is part of deterministic evidence"),
		TryMakeAcknowledgement(
			Visual, AlternatePresenter, Alternate, Diagnostic)
			&& Alternate.Matches(Visual)
			&& !First.Matches(Alternate)
			&& First.GetAcknowledgementId()
				!= Alternate.GetAcknowledgementId());
	TestTrue(TEXT("ordered command projection is stable and non-empty"),
		!First.GetCompletedCommandIds().IsEmpty()
			&& First.GetCompletedCommandIds()
				== Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
					GetOrderedCommandIds(Visual));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCuePresentationAckReplayTeardownTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationAcknowledgement.ReplayAndRunTeardown",
	SwordRhythmCuePresentationAckFlags)

bool Fdemo_mapSwordRhythmCuePresentationAckReplayTeardownTest::RunTest(
	const FString&)
{
	FSwordRhythmCuePresentationAckFixture Fixture;
	FController Controller;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Visual;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Audio;
	TestTrue(TEXT("a real channel pair reaches the caller boundary"),
		Fixture.TryGetPair(Controller, Visual, Audio));

	FAcknowledgement VisualAck;
	FAcknowledgement AudioAck;
	FString Diagnostic;
	TestTrue(TEXT("both channel acknowledgements are captured"),
		TryMakeAcknowledgement(
			Visual, VisualPresenter, VisualAck, Diagnostic)
			&& TryMakeAcknowledgement(
				Audio, AudioPresenter, AudioAck, Diagnostic));
	TestTrue(TEXT("exact acknowledgement replay is idempotent"),
		Controller.TryAcknowledgeVisualHandoff(VisualAck, Diagnostic)
			&& Controller.TryAcknowledgeVisualHandoff(VisualAck, Diagnostic)
			&& Controller.TryAcknowledgeAudioHandoff(AudioAck, Diagnostic)
			&& Controller.TryAcknowledgeAudioHandoff(AudioAck, Diagnostic)
			&& !Controller.HasPendingVisualHandoff()
			&& !Controller.HasPendingAudioHandoff());

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary Summary;
	TestTrue(TEXT("teardown clears controller but not immutable evidence"),
		Controller.TryEnd(AckRun, Summary, Diagnostic)
			&& Controller.IsEmpty()
			&& VisualAck.IsValid() && AudioAck.IsValid()
			&& !Controller.TryAcknowledgeVisualHandoff(
				VisualAck, Diagnostic));
	return true;
}

#endif
