#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapShanmenThrownWeaponArcChoiceProjection.h"

namespace
{
	using EProjectionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoice(
		const FVector2D& TargetIntent,
		const double ApexAdjustment)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Command));
		Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(1, TargetIntent, Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		if (ApexAdjustment != 0.0)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcApexAdjustment(2, ApexAdjustment, Command));
			Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
			check(Reduced.DidChange());
			State = Reduced.State;
		}
		return State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcWithoutTarget()
	{
		const Fdemo_mapShanmenThrownWeaponInputChoiceState Initial =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Command));
		const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Initial, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	Fdemo_mapShanmenThrownWeaponArcChoiceBasis MakeBasis(
		const FVector& Origin = FVector(100.0, 200.0, 50.0),
		const FVector& Forward = FVector(1.0, 0.0, 0.0),
		const FVector& Right = FVector(0.0, 1.0, 0.0))
	{
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
		check(Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			Origin, Forward, Right, Basis));
		return Basis;
	}

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy MakePolicy(
		const double MaximumForwardDistance = 1200.0)
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
		check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, MaximumForwardDistance, 300.0, 100.0, 500.0, Policy));
		return Policy;
	}

	bool IsCleanFailure(
		const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult& Result,
		const EProjectionStatus ExpectedStatus)
	{
		return !Result.IsValid()
			&& Result.GetStatus() == ExpectedStatus
			&& !Result.GetDiagnostic().IsEmpty()
			&& !Result.GetProjectionId().IsValid()
			&& !Result.GetChoiceStateId().IsValid()
			&& Result.GetChoiceRevision() == MAX_uint64
			&& Result.GetTarget().IsZero()
			&& Result.GetApexClearance() == 0.0
			&& Result.GetForwardDistance() == 0.0
			&& Result.GetLateralOffset() == 0.0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceProjectionCaptureContractsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceProjection.CaptureContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceProjectionCaptureContractsTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis ScaledBasis;
	TestTrue(TEXT("Finite orthogonal axes capture and normalize"),
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			FVector(100.0, 200.0, 50.0),
			FVector(2.0, 0.0, 0.0),
			FVector(0.0, 3.0, 0.0),
			Basis)
			&& Basis.IsValid()
			&& Basis.GetForward() == FVector(1.0, 0.0, 0.0)
			&& Basis.GetRight() == FVector(0.0, 1.0, 0.0));
	TestTrue(TEXT("Equivalent axis scale has canonical basis identity"),
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			FVector(100.0, 200.0, 50.0),
			FVector(8.0, 0.0, 0.0),
			FVector(0.0, 0.25, 0.0),
			ScaledBasis)
			&& Basis.Matches(ScaledBasis));
	TestFalse(TEXT("Parallel axes fail closed and clear the output"),
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			FVector::ZeroVector,
			FVector::ForwardVector,
			FVector::ForwardVector,
			Basis));
	TestFalse(TEXT("Failed basis capture publishes no stale identity"),
		Basis.IsValid() || Basis.GetBasisId().IsValid());

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy SamePolicy;
	TestTrue(TEXT("Finite ordered policy captures"),
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 100.0, 500.0, Policy)
			&& Policy.IsValid());
	TestTrue(TEXT("Equivalent policy has canonical identity"),
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 100.0, 500.0, SamePolicy)
			&& Policy.Matches(SamePolicy));
	TestFalse(TEXT("Reversed forward range fails closed"),
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			1200.0, 400.0, 300.0, 100.0, 500.0, Policy));
	TestFalse(TEXT("Failed policy capture publishes no stale identity"),
		Policy.IsValid() || Policy.GetPolicyId().IsValid());
	TestFalse(TEXT("Non-positive apex policy is rejected"),
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 0.0, 500.0, Policy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceProjectionMappingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceProjection.CanonicalMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceProjectionMappingTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Choice =
		MakeArcChoice(FVector2D(0.5, 0.5), 0.25);
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = MakeBasis();
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy = MakePolicy();
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Result =
		Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
			Choice, Basis, Policy);
	TestTrue(TEXT("Canonical choice projects one immutable output"),
		Result.IsProjected()
			&& Result.GetStatus() == EProjectionStatus::Projected
			&& Result.GetProjectionId().IsValid()
			&& Result.GetChoiceStateId() == Choice.GetStateId()
			&& Result.GetChoiceRevision() == Choice.GetRevision()
			&& Result.GetBasisId() == Basis.GetBasisId()
			&& Result.GetPolicyId() == Policy.GetPolicyId()
			&& Result.GetTarget().Equals(FVector(1100.0, 350.0, 50.0))
			&& Result.GetForwardDistance() == 1000.0
			&& Result.GetLateralOffset() == 150.0
			&& Result.GetApexClearance() == 350.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceProjectionBoundariesTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceProjection.BoundaryMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceProjectionBoundariesTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = MakeBasis();
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy = MakePolicy();
	const auto Project = [&Basis, &Policy](
		const FVector2D& Target, const double Apex)
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
			MakeArcChoice(Target, Apex), Basis, Policy);
	};
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult NearLow =
		Project(FVector2D(0.0, -1.0), -1.0);
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult FarHigh =
		Project(FVector2D(0.0, 1.0), 1.0);
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Right =
		Project(FVector2D(1.0, 0.0), 0.0);
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Left =
		Project(FVector2D(-1.0, 0.0), 0.0);
	TestTrue(TEXT("Negative forward/apex extrema map to policy minima"),
		NearLow.IsValid()
			&& NearLow.GetForwardDistance() == 400.0
			&& NearLow.GetApexClearance() == 100.0
			&& NearLow.GetTarget().Equals(FVector(500.0, 200.0, 50.0)));
	TestTrue(TEXT("Positive forward/apex extrema map to policy maxima"),
		FarHigh.IsValid()
			&& FarHigh.GetForwardDistance() == 1200.0
			&& FarHigh.GetApexClearance() == 500.0
			&& FarHigh.GetTarget().Equals(FVector(1300.0, 200.0, 50.0)));
	TestTrue(TEXT("Lateral extrema preserve sign at midpoint distance"),
		Right.IsValid() && Left.IsValid()
			&& Right.GetForwardDistance() == 800.0
			&& Left.GetForwardDistance() == 800.0
			&& Right.GetLateralOffset() == 300.0
			&& Left.GetLateralOffset() == -300.0
			&& Right.GetTarget().Equals(FVector(900.0, 500.0, 50.0))
			&& Left.GetTarget().Equals(FVector(900.0, -100.0, 50.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceProjectionDeterminismTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceProjection.DeterminismAndSensitivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceProjectionDeterminismTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceState FirstChoice =
		MakeArcChoice(FVector2D(3.0, 4.0), 0.5);
	const Fdemo_mapShanmenThrownWeaponInputChoiceState ReplayChoice =
		MakeArcChoice(FVector2D(0.6, 0.8), 0.5);
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis FirstBasis = MakeBasis(
		FVector(100.0, 200.0, 50.0),
		FVector(2.0, 0.0, 0.0),
		FVector(0.0, 3.0, 0.0));
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis ReplayBasis = MakeBasis(
		FVector(100.0, 200.0, 50.0),
		FVector(8.0, 0.0, 0.0),
		FVector(0.0, 0.25, 0.0));
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy = MakePolicy();
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult First =
		Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
			FirstChoice, FirstBasis, Policy);
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Replay =
		Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
			ReplayChoice, ReplayBasis, Policy);
	TestTrue(TEXT("Equivalent normalized inputs reproduce one projection"),
		FirstChoice.Matches(ReplayChoice)
			&& FirstBasis.Matches(ReplayBasis)
			&& First.Matches(Replay));

	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Moved =
		Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
			FirstChoice,
			MakeBasis(FVector(101.0, 200.0, 50.0)),
			Policy);
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Retuned =
		Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
			FirstChoice, FirstBasis, MakePolicy(1201.0));
	TestTrue(TEXT("Basis and policy changes alter projection identity"),
		Moved.IsValid() && Retuned.IsValid()
			&& !First.Matches(Moved)
			&& !First.Matches(Retuned)
			&& First.GetTarget() != Moved.GetTarget()
			&& First.GetForwardDistance() != Retuned.GetForwardDistance());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceProjectionFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceProjection.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceProjectionFailureTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = MakeBasis();
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy = MakePolicy();
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Arc =
		MakeArcChoice(FVector2D(0.5, 0.5), 0.25);
	TestTrue(TEXT("Invalid choice state fails cleanly"),
		IsCleanFailure(
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				Fdemo_mapShanmenThrownWeaponInputChoiceState(), Basis, Policy),
			EProjectionStatus::ChoiceStateInvalid));
	TestTrue(TEXT("Straight state fails before target/basis policy"),
		IsCleanFailure(
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
				Fdemo_mapShanmenThrownWeaponArcChoiceBasis(),
				Fdemo_mapShanmenThrownWeaponArcChoicePolicy()),
			EProjectionStatus::TrajectoryNotArc));
	TestTrue(TEXT("Arc state without target intent fails closed"),
		IsCleanFailure(
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				MakeArcWithoutTarget(), Basis, Policy),
			EProjectionStatus::TargetIntentMissing));
	TestTrue(TEXT("Invalid basis is distinguished"),
		IsCleanFailure(
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				Arc,
				Fdemo_mapShanmenThrownWeaponArcChoiceBasis(),
				Policy),
			EProjectionStatus::BasisInvalid));
	TestTrue(TEXT("Invalid policy is distinguished"),
		IsCleanFailure(
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				Arc,
				Basis,
				Fdemo_mapShanmenThrownWeaponArcChoicePolicy()),
			EProjectionStatus::PolicyInvalid));

	const double Huge = TNumericLimits<double>::Max() * 0.75;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy HugePolicy;
	check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
		Huge, Huge, 0.0, 100.0, 500.0, HugePolicy));
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis HugeBasis = MakeBasis(
		FVector(Huge, 0.0, 0.0));
	TestTrue(TEXT("Finite inputs that overflow derived geometry fail closed"),
		IsCleanFailure(
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				Arc, HugeBasis, HugePolicy),
			EProjectionStatus::OutputRejected));
	return true;
}

#endif
