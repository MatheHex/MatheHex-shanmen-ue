#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation.h"

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction;
	using EMode =
		Edemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackMode;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FContext =
		Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation;

	constexpr EAutomationTestFlags FeedbackFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeChoice(
		const bool bWithTarget)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				State.GetRevision(), ETrajectory::BallisticArc, Command));
		auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		if (bWithTarget)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcTargetIntent(
					State.GetRevision(), FVector2D(0.0, -1.0), Command));
			Reduced =
				Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
					State, Command);
			check(Reduced.DidChange());
			State = Reduced.State;
		}
		return State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult Read(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice)
	{
		return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return true; },
			[&Choice]() { return Choice; });
	}

	FContext MakeArmedContext(const FGuid& RunId, const int32 Slot)
	{
		FContext Context;
		FString Diagnostic;
		EAction Action = EAction::Invalid;
		check(Context.TryBegin(RunId, Diagnostic));
		check(Context.RouteEligibleHotbarPress(
			RunId, Slot, Action, Diagnostic));
		check(Action == EAction::Armed);
		return Context;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreLaunchFeedbackTargetRequiredTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation.TargetRequired",
	FeedbackFlags)

bool Fdemo_mapThrownWeaponArcPreLaunchFeedbackTargetRequiredTest::RunTest(
	const FString&)
{
	const FGuid RunId(
		0xF4640001, 0xF4640002, 0xF4640003, 0xF4640004);
	const FContext Context = MakeArmedContext(RunId, 2);
	const auto Choice = MakeChoice(false);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState EmptySurface;
	FPresentation Feedback;
	TestTrue(TEXT("an armed targetless Arc projects one actionable hint"),
		FPresentation::TryProject(
			Context,
			Read(Choice),
			EmptySurface,
			TEXT("  2  "),
			Feedback));
	TestTrue(TEXT("the hint identifies the sole slot and target requirement"),
		Feedback.IsValid()
			&& Feedback.GetMode() == EMode::TargetRequired
			&& Feedback.NeedsArcTarget()
			&& !Feedback.IsReadyToConfirm()
			&& Feedback.GetRunId() == RunId
			&& Feedback.GetArmedHotbarSlotNumber() == 2
			&& Feedback.GetContextRevision() == 1
			&& Feedback.GetHotbarKeyLabel() == TEXT("2")
			&& Feedback.GetDisplayText() == TEXT(
				"ARC PREVIEW: SLOT 2 [2] ARMED | SET TARGET TO SHOW ARC"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreLaunchFeedbackFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation.SourceFences",
	FeedbackFlags)

bool Fdemo_mapThrownWeaponArcPreLaunchFeedbackFenceTest::RunTest(
	const FString&)
{
	const FGuid RunId(
		0xF4640101, 0xF4640102, 0xF4640103, 0xF4640104);
	const FContext Armed = MakeArmedContext(RunId, 7);
	const auto TargetlessChoice = MakeChoice(false);
	const auto TargetedChoice = MakeChoice(true);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState EmptySurface;
	FPresentation Reused;
	check(FPresentation::TryProject(
		Armed,
		Read(TargetlessChoice),
		EmptySurface,
		TEXT("7"),
		Reused));

	FContext Inactive;
	TestFalse(TEXT("inactive context fails and clears reused output"),
		FPresentation::TryProject(
			Inactive,
			Read(TargetlessChoice),
			EmptySurface,
			TEXT("7"),
			Reused));
	TestTrue(TEXT("failed projection leaves a canonical invalid value"),
		!Reused.IsValid() && Reused.GetDisplayText().IsEmpty());
	TestFalse(TEXT("blank physical key labels fail closed"),
		FPresentation::TryProject(
			Armed,
			Read(TargetlessChoice),
			EmptySurface,
			TEXT(" "),
			Reused));
	TestFalse(TEXT("a targeted choice requires a current visible surface"),
		FPresentation::TryProject(
			Armed,
			Read(TargetedChoice),
			EmptySurface,
			TEXT("7"),
			Reused));
	return true;
}

#endif
