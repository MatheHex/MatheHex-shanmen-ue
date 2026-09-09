#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation.h"

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction;
	using EKind =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind;
	using ECommand = Edemo_mapShanmenThrownWeaponRunCommandStatus;
	using EMode =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode;
	using ETone =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FArc =
		Fdemo_mapShanmenThrownWeaponArcEditingPresentation;
	using FArcInput =
		Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation;
	using FChoice =
		Fdemo_mapShanmenThrownWeaponInputChoiceState;
	using FFeedback =
		Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation;
	using FLaunchRejection =
		Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation;
	using FStack =
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation;
	using FTrajectory =
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation;

	constexpr EAutomationTestFlags StackFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	FChoice Apply(
		const FChoice& State,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		const auto Result =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Result.DidChange());
		return Result.State;
	}

	FChoice MakeChoice(
		const ETrajectory Trajectory,
		const bool bWithTarget = false,
		const double ApexAdjustment = 0.0)
	{
		FChoice State = FChoice::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		if (Trajectory == ETrajectory::BallisticArc)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureTrajectorySelection(
					State.GetRevision(), Trajectory, Command));
			State = Apply(State, Command);
		}
		if (bWithTarget)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcTargetIntent(
					State.GetRevision(), FVector2D(0.6, 0.8), Command));
			State = Apply(State, Command);
		}
		if (ApexAdjustment != 0.0)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcApexAdjustment(
					State.GetRevision(), ApexAdjustment, Command));
			State = Apply(State, Command);
		}
		return State;
	}

	auto Read(const FChoice& Choice)
	{
		return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return true; },
			[&Choice]() { return Choice; });
	}

	FTrajectory ProjectTrajectory(const FChoice& Choice)
	{
		FTrajectory Presentation;
		check(FTrajectory::TryProject(
			Read(Choice), TEXT("V"), Presentation));
		return Presentation;
	}

	FArc ProjectArc(const FChoice& Choice)
	{
		FArc Presentation;
		check(FArc::TryProject(Read(Choice), Presentation));
		return Presentation;
	}

	FArcInput ProjectArcInput(const FArc& Arc)
	{
		FArcInput Presentation;
		check(FArcInput::TryProject(
			Arc,
			TEXT("Middle Mouse"),
			TEXT("Right Bracket"),
			TEXT("Left Bracket"),
			TEXT("Delete"),
			Presentation));
		return Presentation;
	}

	FFeedback ProjectTargetRequiredFeedback(
		const FChoice& Choice,
		const FGuid& RunId)
	{
		Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext Context;
		FString Diagnostic;
		EAction Action = EAction::Invalid;
		check(Context.TryBegin(RunId, Diagnostic));
		check(Context.RouteEligibleHotbarPress(
			RunId, 2, Action, Diagnostic));
		check(Action == EAction::Armed);
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState EmptySurface;
		FFeedback Presentation;
		check(FFeedback::TryProject(
			Context,
			Read(Choice),
			EmptySurface,
			TEXT("2"),
			Presentation));
		return Presentation;
	}

	FLaunchRejection ProjectReleasePathBlocked()
	{
		const FGuid SelectionId(
			0xF4650201, 0xF4650202, 0xF4650203, 0xF4650204);
		const FGuid RunId(
			0xF4650211, 0xF4650212, 0xF4650213, 0xF4650214);
		const FGuid ItemId(
			0xF4650221, 0xF4650222, 0xF4650223, 0xF4650224);
		const FGuid ActivationId(
			0xF4650231, 0xF4650232, 0xF4650233, 0xF4650234);

		Fdemo_mapShanmenThrownWeaponSessionResult Result;
		Result.Status =
			Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected;
		Result.SelectionId = SelectionId;
		Result.RunId = RunId;
		Result.HotbarSlotNumber = 2;
		Result.ItemInstanceId = ItemId;
		Result.Diagnostic = TEXT("Exact release path was blocked.");
		Result.Product.Status =
			Edemo_mapShanmenThrownWeaponProductStatus::RouterRejected;
		Result.Product.SelectionId = SelectionId;
		Result.Product.RunId = RunId;
		Result.Product.SourceItemInstanceId = ItemId;
		Result.Product.ActivationSequence = 11;
		Result.Product.ActivationId = ActivationId;
		Result.Product.Command.Status = ECommand::LaunchRejectedCancelled;
		Result.Product.Command.IntentId = ActivationId;
		Result.Product.Command.RunId = RunId;
		Result.Product.Command.ItemInstanceId = ItemId;
		Result.Product.Command.HostStart.Error =
			Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected;
		Result.Product.Command.HostStart.Launch.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::ReleasePathBlocked;

		FLaunchRejection Presentation;
		check(FLaunchRejection::TryProject(Result, Presentation));
		return Presentation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponMainHUDCombatHintStackOrderTest,
	"Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation.DeterministicOrder",
	StackFlags)

bool Fdemo_mapThrownWeaponMainHUDCombatHintStackOrderTest::RunTest(
	const FString&)
{
	const FChoice StraightChoice = MakeChoice(ETrajectory::Straight);
	const FTrajectory Straight = ProjectTrajectory(StraightChoice);
	FArc NoArc;
	FArcInput NoInput;
	FFeedback NoFeedback;
	FStack StraightStack;
	TestTrue(TEXT("straight mode composes one canonical line"),
		FStack::TryCompose(
			Straight, NoArc, NoInput, NoFeedback, StraightStack));
	TestTrue(TEXT("straight line is bottom-most and cool-toned"),
		StraightStack.IsValid()
			&& StraightStack.GetMode() == EMode::Straight
			&& !StraightStack.IsBallisticArc()
			&& StraightStack.NumLines() == 1
			&& StraightStack.GetLines()[0].GetKind()
				== EKind::TrajectoryMode
			&& StraightStack.GetLines()[0].GetTone()
				== ETone::StraightMode);

	Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation NoTerminal;
	const FLaunchRejection LaunchRejection = ProjectReleasePathBlocked();
	FStack BlockedStack;
	TestTrue(TEXT("a typed pre-launch block appends one authoritative outcome"),
		FStack::TryCompose(
			Straight,
			NoArc,
			NoInput,
			NoFeedback,
			NoTerminal,
			LaunchRejection,
			BlockedStack));
	TestTrue(TEXT("release-path rejection is visible as one blocked HUD line"),
		BlockedStack.IsValid()
			&& BlockedStack.NumLines() == 2
			&& BlockedStack.GetLines()[1].GetKind()
				== EKind::LaunchRejection
			&& BlockedStack.GetLines()[1].GetTone() == ETone::Blocked
			&& BlockedStack.GetLines()[1].GetDisplayText()
				== TEXT("飞刀 · 释放路径受阻"));

	const FChoice ArcChoice = MakeChoice(ETrajectory::BallisticArc);
	const FTrajectory ArcTrajectory = ProjectTrajectory(ArcChoice);
	const FArc Arc = ProjectArc(ArcChoice);
	const FArcInput ArcInput = ProjectArcInput(Arc);
	const FFeedback Feedback = ProjectTargetRequiredFeedback(
		ArcChoice,
		FGuid(0xF4650001, 0xF4650002, 0xF4650003, 0xF4650004));
	FStack ArcStack;
	FStack Replay;
	TestTrue(TEXT("targetless Arc composes all five available sources"),
		FStack::TryCompose(
			ArcTrajectory, Arc, ArcInput, Feedback, ArcStack)
			&& FStack::TryCompose(
				ArcTrajectory, Arc, ArcInput, Feedback, Replay));
	const auto& Lines = ArcStack.GetLines();
	TestTrue(TEXT("Arc stack is deterministic and bottom-to-top"),
		ArcStack.IsValid()
			&& ArcStack.Matches(Replay)
			&& ArcStack.IsBallisticArc()
			&& ArcStack.HasArcPresentation()
			&& !ArcStack.HasArcTargetIntent()
			&& Lines.Num() == 5
			&& Lines[0].GetKind() == EKind::TrajectoryMode
			&& Lines[0].GetTone() == ETone::ArcMode
			&& Lines[1].GetKind() == EKind::ArcApex
			&& Lines[2].GetKind() == EKind::ArcTarget
			&& Lines[3].GetKind() == EKind::ArcInput
			&& Lines[4].GetKind() == EKind::ArcPreLaunchGesture
			&& Lines[4].GetTone() == ETone::TargetRequired);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponMainHUDCombatHintStackPartialTest,
	"Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation.PartialSources",
	StackFlags)

bool Fdemo_mapThrownWeaponMainHUDCombatHintStackPartialTest::RunTest(
	const FString&)
{
	const FChoice ArcChoice = MakeChoice(ETrajectory::BallisticArc);
	const FTrajectory Trajectory = ProjectTrajectory(ArcChoice);
	const FArc Arc = ProjectArc(ArcChoice);
	FArcInput NoInput;
	FFeedback NoFeedback;
	FStack Stack;
	TestTrue(TEXT("an Arc trajectory remains visible without edit sources"),
		FStack::TryCompose(
			Trajectory, FArc(), NoInput, NoFeedback, Stack));
	TestTrue(TEXT("trajectory-only Arc has one warm mode line"),
		Stack.IsValid()
			&& Stack.IsBallisticArc()
			&& !Stack.HasArcPresentation()
			&& Stack.NumLines() == 1
			&& Stack.GetLines()[0].GetTone() == ETone::ArcMode);

	FTrajectory NoTrajectory;
	TestTrue(TEXT("Arc values survive an unavailable toggle label"),
		FStack::TryCompose(
			NoTrajectory, Arc, NoInput, NoFeedback, Stack));
	TestTrue(TEXT("partial Arc stack preserves apex then target order"),
		Stack.IsValid()
			&& Stack.HasArcPresentation()
			&& Stack.NumLines() == 2
			&& Stack.GetLines()[0].GetKind() == EKind::ArcApex
			&& Stack.GetLines()[1].GetKind() == EKind::ArcTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponMainHUDCombatHintStackFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation.SourceFences",
	StackFlags)

bool Fdemo_mapThrownWeaponMainHUDCombatHintStackFenceTest::RunTest(
	const FString&)
{
	const FChoice TargetlessChoice =
		MakeChoice(ETrajectory::BallisticArc);
	const FChoice TargetedChoice =
		MakeChoice(ETrajectory::BallisticArc, true);
	const FChoice ApexLimitChoice =
		MakeChoice(ETrajectory::BallisticArc, false, 1.0);
	const FTrajectory ArcTrajectory = ProjectTrajectory(TargetlessChoice);
	const FArc TargetlessArc = ProjectArc(TargetlessChoice);
	const FArc TargetedArc = ProjectArc(TargetedChoice);
	const FArcInput StaleLimitInput =
		ProjectArcInput(ProjectArc(ApexLimitChoice));
	const FFeedback TargetRequired = ProjectTargetRequiredFeedback(
		TargetlessChoice,
		FGuid(0xF4650101, 0xF4650102, 0xF4650103, 0xF4650104));
	FArcInput NoInput;
	FFeedback NoFeedback;
	FStack Reused;
	check(FStack::TryCompose(
		ArcTrajectory, TargetlessArc, NoInput, TargetRequired, Reused));

	TestFalse(TEXT("a stale Arc input hint cannot describe current limits"),
		FStack::TryCompose(
			ArcTrajectory,
			TargetlessArc,
			StaleLimitInput,
			TargetRequired,
			Reused));
	TestTrue(TEXT("failed composition clears reusable output"),
		!Reused.IsValid() && Reused.NumLines() == 0);

	const FTrajectory Straight = ProjectTrajectory(
		MakeChoice(ETrajectory::Straight));
	TestFalse(TEXT("straight mode rejects stale Arc presentation"),
		FStack::TryCompose(
			Straight, TargetlessArc, NoInput, NoFeedback, Reused));
	TestFalse(TEXT("target-required feedback rejects a targeted Arc"),
		FStack::TryCompose(
			ProjectTrajectory(TargetedChoice),
			TargetedArc,
			ProjectArcInput(TargetedArc),
			TargetRequired,
			Reused));
	TestFalse(TEXT("orphan input and gesture sources fail closed"),
		FStack::TryCompose(
			FTrajectory(), FArc(), StaleLimitInput, TargetRequired, Reused));
	return true;
}

#endif
