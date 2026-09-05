#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcEditingPresentation.h"

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

namespace
{
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponArcEditingPresentation;

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
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const FVector2D& Target = FVector2D(3.0, 4.0))
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				State.GetRevision(), Target, Command));
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
		FPresentation& OutPresentation)
	{
		return FPresentation::TryProject(Read(State), OutPresentation);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState CreateArc()
	{
		return Select(
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			ETrajectory::BallisticArc);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationNeutralTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.NeutralArcProjection",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationNeutralTest::RunTest(
	const FString&)
{
	FPresentation Presentation;
	TestTrue(TEXT("neutral Arc choice projects"),
		Project(CreateArc(), Presentation));
	TestTrue(TEXT("neutral Arc projection exposes only valid edit details"),
		Presentation.IsValid()
			&& !Presentation.HasArcTargetIntent()
			&& Presentation.GetArcTargetIntent().IsZero()
			&& Presentation.GetArcApexAdjustment() == 0.0
			&& Presentation.CanSetArcTargetIntent()
			&& Presentation.CanIncreaseArcApex()
			&& Presentation.CanDecreaseArcApex()
			&& !Presentation.CanClearArcTargetIntent()
			&& Presentation.GetTargetDisplayText()
				== TEXT("ARC TARGET: UNSET  |  SET AVAILABLE")
			&& Presentation.GetApexDisplayText()
				== TEXT("ARC APEX: +0.00  |  ADJUST: +/-"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationTargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.TargetProjection",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationTargetTest::RunTest(
	const FString&)
{
	FPresentation Presentation;
	TestTrue(TEXT("canonical target projects"),
		Project(SetTarget(CreateArc()), Presentation));
	TestTrue(TEXT("target and clear availability are visible"),
		Presentation.IsValid()
			&& Presentation.HasArcTargetIntent()
			&& Presentation.GetArcTargetIntent().Equals(
				FVector2D(0.6, 0.8), 1.0e-6)
			&& Presentation.CanSetArcTargetIntent()
			&& Presentation.CanClearArcTargetIntent()
			&& Presentation.GetTargetDisplayText()
				== TEXT(
					"ARC TARGET: X +0.60  Y +0.80  |  SET / CLEAR AVAILABLE"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationApexBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.ApexCapabilityBoundaries",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationApexBoundaryTest::RunTest(
	const FString&)
{
	FPresentation Maximum;
	FPresentation Minimum;
	TestTrue(TEXT("both apex boundaries project"),
		Project(AdjustApex(CreateArc(), 1.0), Maximum)
			&& Project(AdjustApex(CreateArc(), -1.0), Minimum));
	TestTrue(TEXT("upper boundary exposes only decrease"),
		Maximum.IsValid() && Maximum.GetArcApexAdjustment() == 1.0
			&& !Maximum.CanIncreaseArcApex()
			&& Maximum.CanDecreaseArcApex()
			&& Maximum.GetApexDisplayText()
				== TEXT("ARC APEX: +1.00  |  ADJUST: -"));
	TestTrue(TEXT("lower boundary exposes only increase"),
		Minimum.IsValid() && Minimum.GetArcApexAdjustment() == -1.0
			&& Minimum.CanIncreaseArcApex()
			&& !Minimum.CanDecreaseArcApex()
			&& Minimum.GetApexDisplayText()
				== TEXT("ARC APEX: -1.00  |  ADJUST: +"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationRefreshTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.DetailRefresh",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationRefreshTest::RunTest(
	const FString&)
{
	const auto NeutralArc = CreateArc();
	auto DetailedArc = SetTarget(NeutralArc, FVector2D(-0.25, 0.5));
	DetailedArc = AdjustApex(DetailedArc, 0.5);
	FPresentation Neutral;
	FPresentation Detailed;
	TestTrue(TEXT("neutral and changed details both project"),
		Project(NeutralArc, Neutral) && Project(DetailedArc, Detailed));
	TestTrue(TEXT("a fresh read refreshes all visible Arc details"),
		Neutral.IsValid() && Detailed.IsValid()
			&& Neutral.GetTargetDisplayText()
				!= Detailed.GetTargetDisplayText()
			&& Neutral.GetApexDisplayText()
				!= Detailed.GetApexDisplayText()
			&& Detailed.HasArcTargetIntent()
			&& Detailed.GetArcTargetIntent().Equals(
				FVector2D(-0.25, 0.5))
			&& Detailed.GetArcApexAdjustment() == 0.5
			&& Detailed.GetApexDisplayText()
				== TEXT("ARC APEX: +0.50  |  ADJUST: +/-"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationRevisionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.RevisionNeutrality",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationRevisionTest::RunTest(
	const FString&)
{
	const auto FirstArc = CreateArc();
	auto ReenteredArc = Select(FirstArc, ETrajectory::Straight);
	ReenteredArc = Select(ReenteredArc, ETrajectory::BallisticArc);
	FPresentation First;
	FPresentation Reentered;
	TestTrue(TEXT("equal Arc details at different revisions project"),
		FirstArc.GetRevision() == 1 && ReenteredArc.GetRevision() == 3
			&& Project(FirstArc, First)
			&& Project(ReenteredArc, Reentered));
	TestTrue(TEXT("revision does not leak into Arc detail presentation"),
		First.IsValid() && Reentered.IsValid()
			&& First.HasArcTargetIntent()
				== Reentered.HasArcTargetIntent()
			&& First.GetArcTargetIntent().Equals(
				Reentered.GetArcTargetIntent())
			&& First.GetArcApexAdjustment()
				== Reentered.GetArcApexAdjustment()
			&& First.GetTargetDisplayText()
				== Reentered.GetTargetDisplayText()
			&& First.GetApexDisplayText()
				== Reentered.GetApexDisplayText());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationStraightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.StraightClearsOutput",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationStraightTest::RunTest(
	const FString&)
{
	FPresentation Reused;
	TestTrue(TEXT("fixture begins with a valid Arc projection"),
		Project(CreateArc(), Reused) && Reused.IsValid());
	TestTrue(TEXT("Straight mode is out of scope and clears reused output"),
		!Project(
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			Reused)
			&& !Reused.IsValid()
			&& Reused.GetTargetDisplayText().IsEmpty()
			&& Reused.GetApexDisplayText().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPresentationInvalidReadTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation.InvalidReadClearsOutput",
	PresentationFlags)

bool Fdemo_mapThrownWeaponArcEditingPresentationInvalidReadTest::RunTest(
	const FString&)
{
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
	TestTrue(TEXT("fixture begins with a valid Arc projection"),
		Project(CreateArc(), Reused) && Reused.IsValid());
	TestTrue(TEXT("missing source fails and clears reused output"),
		MissingRead.GetStatus() == EReadStatus::ChoiceSourceUnavailable
			&& !FPresentation::TryProject(MissingRead, Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("invalid source state also fails closed"),
		InvalidStateRead.GetStatus() == EReadStatus::ChoiceStateInvalid
			&& !FPresentation::TryProject(InvalidStateRead, Reused)
			&& !Reused.IsValid());
	return true;
}

#endif
