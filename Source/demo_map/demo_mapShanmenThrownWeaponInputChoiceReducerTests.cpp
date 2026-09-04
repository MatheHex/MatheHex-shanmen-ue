#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

#include "Misc/AutomationTest.h"

#include <limits>

namespace
{
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand SelectTrajectory(
		const uint64 Revision,
		const Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind Kind)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(Revision, Kind, Command));
		return Command;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand SetTarget(
		const uint64 Revision,
		const FVector2D& Target)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(Revision, Target, Command));
		return Command;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand AdjustApex(
		const uint64 Revision,
		const double Delta)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(Revision, Delta, Command));
		return Command;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand ClearTarget(
		const uint64 Revision)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetClear(Revision, Command));
		return Command;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState ReduceChecked(
		FAutomationTestBase& Test,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Previous,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Result =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Previous, Command);
		Test.TestTrue(TEXT("Reduction succeeds and advances"),
			Result.IsSuccess() && Result.DidChange());
		return Result.State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceCommandNormalizationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoice.CommandNormalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceCommandNormalizationTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	TestTrue(TEXT("Initial state is canonical Straight revision zero"),
		Initial.IsValid() && Initial.GetRevision() == 0
			&& Initial.GetTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
			&& !Initial.HasArcTargetIntent()
			&& Initial.GetArcApexAdjustment() == 0.0);

	const auto Oversized = SetTarget(0, FVector2D(3.0, 4.0));
	const auto Unit = SetTarget(0, FVector2D(0.6, 0.8));
	TestEqual(TEXT("Radial overflow has the canonical unit-disc command ID"),
		Oversized.GetCommandId(), Unit.GetCommandId());
	TestTrue(TEXT("Radial overflow canonicalizes X"),
		FMath::IsNearlyEqual(
			Oversized.GetArcTargetIntent().X, 0.6, 1.0e-6));
	TestTrue(TEXT("Radial overflow canonicalizes Y"),
		FMath::IsNearlyEqual(
			Oversized.GetArcTargetIntent().Y, 0.8, 1.0e-6));
	const auto Analog = SetTarget(0, FVector2D(0.3, -0.4));
	TestTrue(TEXT("In-disc analog magnitude is retained"),
		FMath::IsNearlyEqual(
			Analog.GetArcTargetIntent().Size(), 0.5, 1.0e-6));
	const auto NegativeZero = SetTarget(0, FVector2D(1.0, -0.0));
	const auto PositiveZero = SetTarget(0, FVector2D(1.0, 0.0));
	TestEqual(TEXT("Signed zero has one command identity"),
		NegativeZero.GetCommandId(), PositiveZero.GetCommandId());

	const auto HighDelta = AdjustApex(0, 5.0);
	const auto UnitDelta = AdjustApex(0, 1.0);
	TestTrue(TEXT("Apex delta clamps before identity"),
		HighDelta.GetCommandId() == UnitDelta.GetCommandId()
			&& HighDelta.GetArcApexAdjustmentDelta() == 1.0);
	TestEqual(TEXT("Negative apex delta clamps"),
		AdjustApex(0, -5.0).GetArcApexAdjustmentDelta(), -1.0);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Reusable = Unit;
	TestFalse(TEXT("Zero target is rejected and clears reusable output"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(0, FVector2D::ZeroVector, Reusable));
	TestFalse(TEXT("Rejected target leaves invalid output"), Reusable.IsValid());
	TestFalse(TEXT("Non-finite apex delta is rejected"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(
				0, std::numeric_limits<double>::quiet_NaN(), Reusable));
	TestFalse(TEXT("Zero apex delta is rejected"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(0, 0.0, Reusable));
	TestFalse(TEXT("Invalid trajectory is rejected"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				0,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid,
				Reusable));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceModeAndTargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoice.ModeAndTargetTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceModeAndTargetTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto SelectArc = SelectTrajectory(
		0,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc);
	const auto Arc = ReduceChecked(*this, Initial, SelectArc);
	TestTrue(TEXT("Arc selection advances one neutral Arc state"),
		Arc.GetRevision() == 1
			&& Arc.GetTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
			&& !Arc.HasArcTargetIntent()
			&& Arc.GetArcApexAdjustment() == 0.0);

	const auto Replay = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
		Arc, SelectArc);
	TestTrue(TEXT("Exact command replay returns the existing state"),
		Replay.IsSuccess() && Replay.IsReplay()
			&& Replay.State.Matches(Arc));
	const auto ArcNoChange =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Arc,
			SelectTrajectory(
				1,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc));
	TestTrue(TEXT("Fresh repeated mode selection is a no-op"),
		ArcNoChange.IsSuccess() && !ArcNoChange.DidChange()
			&& ArcNoChange.State.Matches(Arc));

	const auto Targeted = ReduceChecked(
		*this, Arc, SetTarget(1, FVector2D(0.25, -0.5)));
	TestTrue(TEXT("Arc target advances and is visible"),
		Targeted.GetRevision() == 2 && Targeted.HasArcTargetIntent()
			&& Targeted.GetArcTargetIntent()
				.Equals(FVector2D(0.25, -0.5)));
	const auto TargetNoChange =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Targeted, SetTarget(2, FVector2D(0.25, -0.5)));
	TestTrue(TEXT("Same canonical target is a no-op"),
		TargetNoChange.IsSuccess() && !TargetNoChange.DidChange()
			&& TargetNoChange.State.Matches(Targeted));

	const auto Cleared = ReduceChecked(*this, Targeted, ClearTarget(2));
	TestTrue(TEXT("Target clear advances to canonical empty Arc"),
		Cleared.GetRevision() == 3 && !Cleared.HasArcTargetIntent()
			&& Cleared.GetArcTargetIntent().IsZero());
	const auto ClearNoChange =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Cleared, ClearTarget(3));
	TestTrue(TEXT("Repeated clear is a no-op"),
		ClearNoChange.IsSuccess() && !ClearNoChange.DidChange());

	const auto Straight = ReduceChecked(
		*this,
		Cleared,
		SelectTrajectory(
			3,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight));
	const auto ArcAgain = ReduceChecked(
		*this,
		Straight,
		SelectTrajectory(
			4,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc));
	TestTrue(TEXT("Mode round trip starts one neutral Arc choice"),
		ArcAgain.GetRevision() == 5 && !ArcAgain.HasArcTargetIntent()
			&& ArcAgain.GetArcApexAdjustment() == 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceApexSaturationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoice.ApexSaturationAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceApexSaturationTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto Arc = ReduceChecked(
		*this,
		Initial,
		SelectTrajectory(
			0,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc));
	const auto Raised = ReduceChecked(*this, Arc, AdjustApex(1, 0.75));
	const auto Maximum = ReduceChecked(*this, Raised, AdjustApex(2, 0.5));
	TestEqual(TEXT("Positive accumulation saturates at one"),
		Maximum.GetArcApexAdjustment(), 1.0);
	const auto Saturated =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Maximum, AdjustApex(3, 0.25));
	TestTrue(TEXT("Input beyond upper limit is a no-op"),
		Saturated.IsSuccess() && !Saturated.DidChange()
			&& Saturated.State.Matches(Maximum));

	const auto Neutral = ReduceChecked(*this, Maximum, AdjustApex(3, -1.0));
	const auto Minimum = ReduceChecked(*this, Neutral, AdjustApex(4, -1.0));
	TestEqual(TEXT("Negative accumulation saturates at minus one"),
		Minimum.GetArcApexAdjustment(), -1.0);
	const auto LowerSaturated =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Minimum, AdjustApex(5, -1.0));
	TestTrue(TEXT("Input beyond lower limit is a no-op"),
		LowerSaturated.IsSuccess() && !LowerSaturated.DidChange());

	const auto Straight = ReduceChecked(
		*this,
		Minimum,
		SelectTrajectory(
			5,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight));
	TestTrue(TEXT("Straight mode clears Arc-only adjustment"),
		Straight.GetArcApexAdjustment() == 0.0
			&& !Straight.HasArcTargetIntent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoice.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceFailClosedTest::RunTest(const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto TargetInStraight =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Initial, SetTarget(0, FVector2D(0.5, 0.5)));
	const auto ApexInStraight =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Initial, AdjustApex(0, 0.5));
	TestTrue(TEXT("Arc-only edits fail closed in Straight mode"),
		TargetInStraight.Status
				== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::ModeMismatch
			&& ApexInStraight.Status
				== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::ModeMismatch
			&& !TargetInStraight.State.IsValid()
			&& !ApexInStraight.State.IsValid()
			&& Initial.GetRevision() == 0);

	const auto Arc = ReduceChecked(
		*this,
		Initial,
		SelectTrajectory(
			0,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc));
	const auto Stale = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
		Arc, SetTarget(0, FVector2D(0.5, 0.5)));
	TestTrue(TEXT("Stale command cannot advance the Arc state"),
		Stale.Status
				== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::
					RevisionMismatch
			&& !Stale.State.IsValid() && Arc.GetRevision() == 1);

	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand InvalidCommand;
	const auto CommandRejected =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Arc, InvalidCommand);
	TestEqual(TEXT("Default command is rejected first"),
		CommandRejected.Status,
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::CommandInvalid);
	const Fdemo_mapShanmenThrownWeaponInputChoiceState InvalidState;
	const auto StateRejected =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			InvalidState, SetTarget(0, FVector2D(0.5, 0.5)));
	TestEqual(TEXT("Default state is rejected before command sequencing"),
		StateRejected.Status,
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::StateInvalid);
	TestTrue(TEXT("Rejected results never publish replacement state"),
		!CommandRejected.State.IsValid() && !StateRejected.State.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceDeterministicReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoice.DeterministicSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceDeterministicReplayTest::RunTest(
	const FString&)
{
	auto RunSequence = [this]()
	{
		auto State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		State = ReduceChecked(
			*this,
			State,
			SelectTrajectory(
				0,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc));
		State = ReduceChecked(
			*this, State, SetTarget(1, FVector2D(2.0, -1.0)));
		State = ReduceChecked(*this, State, AdjustApex(2, 0.4));
		return State;
	};
	const auto First = RunSequence();
	const auto Replay = RunSequence();
	TestTrue(TEXT("Independent canonical sequences produce one sealed state"),
		First.Matches(Replay) && First.GetRevision() == 3
			&& First.HasArcTargetIntent()
			&& FMath::IsNearlyEqual(
				First.GetArcTargetIntent().Size(), 1.0, 1.0e-6)
			&& FMath::IsNearlyEqual(
				First.GetArcApexAdjustment(), 0.4, 1.0e-6));

	const auto DifferentTarget = ReduceChecked(
		*this,
		ReduceChecked(
			*this,
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			SelectTrajectory(
				0,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc)),
		SetTarget(1, FVector2D(-1.0, 0.0)));
	TestTrue(TEXT("Different canonical target yields a different state"),
		DifferentTarget.GetStateId() != First.GetStateId());
	TestTrue(TEXT("Expected revision participates in command identity"),
		SetTarget(1, FVector2D(0.5, 0.5)).GetCommandId()
			!= SetTarget(2, FVector2D(0.5, 0.5)).GetCommandId());
	return true;
}

#endif
