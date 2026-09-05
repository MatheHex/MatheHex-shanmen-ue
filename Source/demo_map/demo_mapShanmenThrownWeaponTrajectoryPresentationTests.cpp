#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponTrajectoryPresentation.h"

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

namespace
{
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation;

	constexpr EAutomationTestFlags PresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	Fdemo_mapShanmenThrownWeaponInputChoiceState Apply(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		const auto Result =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Result.IsSuccess());
		return Result.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState Select(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const ETrajectory Trajectory)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				State.GetRevision(), Trajectory, Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState SetTarget(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				State.GetRevision(), FVector2D(3.0, 4.0), Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState AdjustApex(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const double Delta)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(
				State.GetRevision(), Delta, Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult Read(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return true; },
			[&State]() { return State; });
	}

	bool Project(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const FString& Key,
		FPresentation& OutPresentation)
	{
		return FPresentation::TryProject(
			Read(State), Key, OutPresentation);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryPresentationStraightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation.StraightProjection",
	PresentationFlags)

bool Fdemo_mapThrownWeaponTrajectoryPresentationStraightTest::RunTest(
	const FString&)
{
	FPresentation Presentation;
	TestTrue(TEXT("initial Straight choice projects for display"),
		Project(
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			TEXT("T"),
			Presentation));
	TestTrue(TEXT("Straight projection is complete and immutable"),
		Presentation.IsValid()
			&& !Presentation.IsBallisticArc()
			&& Presentation.GetTrajectoryKind() == ETrajectory::Straight
			&& Presentation.GetModeLabel() == TEXT("STRAIGHT")
			&& Presentation.GetToggleKeyLabel() == TEXT("T")
			&& Presentation.GetDisplayText()
				== TEXT("THROWN TRAJECTORY: STRAIGHT  |  [T] Toggle"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryPresentationArcTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation.ArcProjection",
	PresentationFlags)

bool Fdemo_mapThrownWeaponTrajectoryPresentationArcTest::RunTest(
	const FString&)
{
	const auto Arc = Select(
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
		ETrajectory::BallisticArc);
	FPresentation Presentation;
	TestTrue(TEXT("Ballistic Arc choice projects for display"),
		Project(Arc, TEXT("T"), Presentation));
	TestTrue(TEXT("Arc projection carries only the visible mode and key"),
		Presentation.IsValid()
			&& Presentation.IsBallisticArc()
			&& Presentation.GetTrajectoryKind()
				== ETrajectory::BallisticArc
			&& Presentation.GetModeLabel() == TEXT("BALLISTIC ARC")
			&& Presentation.GetDisplayText()
				== TEXT(
					"THROWN TRAJECTORY: BALLISTIC ARC  |  [T] Toggle"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryPresentationChoiceDetailNeutralityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation.TargetAndApexNeutrality",
	PresentationFlags)

bool Fdemo_mapThrownWeaponTrajectoryPresentationChoiceDetailNeutralityTest::
RunTest(const FString&)
{
	const auto Arc = Select(
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
		ETrajectory::BallisticArc);
	auto DetailedArc = SetTarget(Arc);
	DetailedArc = AdjustApex(DetailedArc, 0.625);
	FPresentation Neutral;
	FPresentation Detailed;
	TestTrue(TEXT("both Arc variants project"),
		Project(Arc, TEXT("T"), Neutral)
			&& Project(DetailedArc, TEXT("T"), Detailed));
	TestTrue(TEXT("target and apex do not leak into trajectory presentation"),
		Neutral.IsValid() && Detailed.IsValid()
			&& Neutral.GetTrajectoryKind() == Detailed.GetTrajectoryKind()
			&& Neutral.GetModeLabel() == Detailed.GetModeLabel()
			&& Neutral.GetToggleKeyLabel()
				== Detailed.GetToggleKeyLabel()
			&& Neutral.GetDisplayText() == Detailed.GetDisplayText());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryPresentationRevisionNeutralityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation.RevisionNeutrality",
	PresentationFlags)

bool Fdemo_mapThrownWeaponTrajectoryPresentationRevisionNeutralityTest::
RunTest(const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	auto Later = Select(Initial, ETrajectory::BallisticArc);
	Later = Select(Later, ETrajectory::Straight);
	FPresentation First;
	FPresentation ReplayedChoice;
	TestTrue(TEXT("equal visible modes at different revisions project"),
		Initial.GetRevision() == 0 && Later.GetRevision() == 2
			&& Project(Initial, TEXT("T"), First)
			&& Project(Later, TEXT("T"), ReplayedChoice));
	TestTrue(TEXT("revision does not leak into presentation"),
		First.GetTrajectoryKind() == ReplayedChoice.GetTrajectoryKind()
			&& First.GetModeLabel() == ReplayedChoice.GetModeLabel()
			&& First.GetToggleKeyLabel()
				== ReplayedChoice.GetToggleKeyLabel()
			&& First.GetDisplayText()
				== ReplayedChoice.GetDisplayText());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryPresentationConfigurableKeyTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation.ConfigurableKeyProjection",
	PresentationFlags)

bool Fdemo_mapThrownWeaponTrajectoryPresentationConfigurableKeyTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	FPresentation DefaultKey;
	FPresentation RemappedKey;
	TestTrue(TEXT("default and remapped key labels project"),
		Project(Initial, TEXT(" T "), DefaultKey)
			&& Project(Initial, TEXT("C"), RemappedKey));
	TestTrue(TEXT("only the configured key label changes"),
		DefaultKey.GetTrajectoryKind()
				== RemappedKey.GetTrajectoryKind()
			&& DefaultKey.GetModeLabel() == RemappedKey.GetModeLabel()
			&& DefaultKey.GetToggleKeyLabel() == TEXT("T")
			&& RemappedKey.GetToggleKeyLabel() == TEXT("C")
			&& DefaultKey.GetDisplayText()
				== TEXT("THROWN TRAJECTORY: STRAIGHT  |  [T] Toggle")
			&& RemappedKey.GetDisplayText()
				== TEXT("THROWN TRAJECTORY: STRAIGHT  |  [C] Toggle"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryPresentationFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation.FailureClearsOutput",
	PresentationFlags)

bool Fdemo_mapThrownWeaponTrajectoryPresentationFailureTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto ValidRead = Read(Initial);
	const auto MissingRead =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return false; },
			[]()
			{
				return Fdemo_mapShanmenThrownWeaponInputChoiceState::
					CreateInitial();
			});
	const auto InvalidStateRead =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return true; },
			[]()
			{
				return Fdemo_mapShanmenThrownWeaponInputChoiceState();
			});

	FPresentation Reused;
	TestTrue(TEXT("fixture begins with a valid projection"),
		FPresentation::TryProject(ValidRead, TEXT("T"), Reused)
			&& Reused.IsValid());
	TestTrue(TEXT("missing source fails and clears reused output"),
		MissingRead.GetStatus() == EReadStatus::ChoiceSourceUnavailable
			&& !FPresentation::TryProject(
				MissingRead, TEXT("T"), Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("invalid source state fails and leaves output clear"),
		InvalidStateRead.GetStatus() == EReadStatus::ChoiceStateInvalid
			&& !FPresentation::TryProject(
				InvalidStateRead, TEXT("T"), Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("empty key fails closed and clears a fresh projection"),
		FPresentation::TryProject(ValidRead, TEXT("T"), Reused)
			&& Reused.IsValid()
			&& !FPresentation::TryProject(
				ValidRead, TEXT("  "), Reused)
			&& !Reused.IsValid());
	return true;
}

#endif
