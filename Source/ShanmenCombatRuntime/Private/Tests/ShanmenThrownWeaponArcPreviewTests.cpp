#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenThrownWeaponArcPreview.h"

namespace
{
	const FGuid PreviewRunId(0x20290001, 0, 0, 1);
	const FGuid PreviewOwnerId(0x20290002, 0, 0, 1);
	const FGuid PreviewSourceEntityId(0x20290003, 0, 0, 1);
	const FGuid PreviewItemId(0x20290004, 0, 0, 1);
	const FGuid PreviewActivationId(0x20290005, 0, 0, 1);

	constexpr EAutomationTestFlags PreviewFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	FShanmenCombatActionSnapshot MakeAction(const FString& Digest)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = PreviewRunId;
		Capture.OwnerId = PreviewOwnerId;
		Capture.ActivationId = PreviewActivationId;
		Capture.SourceEntityId = PreviewSourceEntityId;
		Capture.SourceItemInstanceId = PreviewItemId;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponArcRequest::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P20.29");
		Capture.Content.Digest = Digest;

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponArcPlan MakePlan(
		const FVector& Origin = FVector::ZeroVector,
		const FVector& Target = FVector(1000.0, 0.0, 0.0),
		double ApexClearance = 200.0,
		const FString& Digest = TEXT("PREVIEW-P20.29"))
	{
		FShanmenThrownWeaponArcRequestCapture Capture;
		Capture.Action = MakeAction(Digest);
		Capture.TechniqueTier =
			EShanmenThrownWeaponTechniqueTier::Intermediate;
		Capture.Origin = Origin;
		Capture.Target = Target;
		Capture.GravityMagnitude = 980.0;
		Capture.ApexClearance = ApexClearance;
		Capture.MaximumLaunchSpeed = 4000.0;
		Capture.MaximumFlightTime = 10.0;

		const FShanmenThrownWeaponArcPlanResult Result =
			FShanmenThrownWeaponArcPlanner::Plan(Capture);
		check(Result.IsPlanned());
		return Result.Plan;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcPreviewEndpointsTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc.CanonicalEndpoints",
	PreviewFlags)

bool FShanmenThrownWeaponArcPreviewEndpointsTest::RunTest(const FString&)
{
	const FShanmenThrownWeaponArcPlan Plan = MakePlan();
	FShanmenThrownWeaponArcPreview Preview;
	const bool bProjected =
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan, 8, Preview);
	TestTrue(TEXT("valid plan produces a self-validating preview"),
		bProjected && Preview.IsValid());
	if (!bProjected)
	{
		return false;
	}
	TestTrue(TEXT("metadata remains bound to the exact plan"),
		Preview.GetPlan().Matches(Plan)
			&& Preview.GetSegmentCount() == 8
			&& Preview.GetPositions().Num() == 9
			&& Preview.GetApexPosition() == Plan.GetApexPosition()
			&& Preview.GetPlannedLandingPosition()
				== Plan.GetRequest().GetTarget()
			&& Preview.GetFlightTimeSeconds()
				== Plan.GetFlightTimeSeconds());
	TestTrue(TEXT("preview endpoints are exact rather than approximate"),
		Preview.GetPositions()[0] == Plan.GetRequest().GetOrigin()
			&& Preview.GetPositions().Last()
				== Plan.GetRequest().GetTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcPreviewUniformSamplesTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc.UniformSamples",
	PreviewFlags)

bool FShanmenThrownWeaponArcPreviewUniformSamplesTest::RunTest(
	const FString&)
{
	const FShanmenThrownWeaponArcPlan Plan = MakePlan();
	FShanmenThrownWeaponArcPreview Preview;
	const bool bProjected =
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan, 4, Preview);
	TestTrue(TEXT("four-segment preview is valid"),
		bProjected);
	if (!bProjected)
	{
		return false;
	}

	bool bAllSamplesMatch = Preview.GetPositions().Num() == 5;
	for (int32 Index = 0; bAllSamplesMatch && Index <= 4; ++Index)
	{
		FVector Expected;
		const double Elapsed = Plan.GetFlightTimeSeconds()
			* static_cast<double>(Index) / 4.0;
		bAllSamplesMatch = Plan.TrySamplePosition(Elapsed, Expected)
			&& Preview.GetPositions()[Index].Equals(Expected, 1.0e-6);
	}
	TestTrue(TEXT("every point uses uniform plan time sampling"),
		bAllSamplesMatch);
	TestTrue(TEXT("symmetric level-target midpoint is the exact apex"),
		Preview.GetPositions()[2].Equals(
			Plan.GetApexPosition(), 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcPreviewDeterminismTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc.DeterministicIdentity",
	PreviewFlags)

bool FShanmenThrownWeaponArcPreviewDeterminismTest::RunTest(const FString&)
{
	const FShanmenThrownWeaponArcPlan Plan = MakePlan();
	FShanmenThrownWeaponArcPreview First;
	FShanmenThrownWeaponArcPreview Replay;
	FShanmenThrownWeaponArcPreview HigherResolution;
	const bool bReplayProjected =
		FShanmenThrownWeaponArcPreviewSampler::TrySample(Plan, 8, First)
		&& FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan, 8, Replay);
	TestTrue(TEXT("same plan and resolution replay exactly"),
		bReplayProjected && First.Matches(Replay)
			&& First.GetPreviewId() == Replay.GetPreviewId());
	if (!bReplayProjected)
	{
		return false;
	}
	TestTrue(TEXT("resolution participates in preview identity"),
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan, 16, HigherResolution)
			&& HigherResolution.IsValid()
			&& First.GetPreviewId() != HigherResolution.GetPreviewId()
			&& First.GetPositions().Num()
				!= HigherResolution.GetPositions().Num()
			&& First.GetPlannedLandingPosition()
				== HigherResolution.GetPlannedLandingPosition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcPreviewHeightVariantsTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc.HeightVariants",
	PreviewFlags)

bool FShanmenThrownWeaponArcPreviewHeightVariantsTest::RunTest(
	const FString&)
{
	const FShanmenThrownWeaponArcPlan Elevated = MakePlan(
		FVector(10.0, 20.0, 50.0),
		FVector(900.0, 250.0, 300.0),
		200.0,
		TEXT("PREVIEW-ELEVATED"));
	const FShanmenThrownWeaponArcPlan Lowered = MakePlan(
		FVector(10.0, 20.0, 50.0),
		FVector(900.0, -250.0, -250.0),
		200.0,
		TEXT("PREVIEW-LOWERED"));
	FShanmenThrownWeaponArcPreview ElevatedPreview;
	FShanmenThrownWeaponArcPreview LoweredPreview;
	const bool bProjected =
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Elevated, 16, ElevatedPreview)
		&& FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Lowered, 16, LoweredPreview);
	TestTrue(TEXT("unequal endpoint heights remain previewable"),
		bProjected);
	if (!bProjected)
	{
		return false;
	}
	TestTrue(TEXT("each height variant preserves exact landing metadata"),
		ElevatedPreview.IsValid()
			&& LoweredPreview.IsValid()
			&& ElevatedPreview.GetPositions().Last()
				== Elevated.GetRequest().GetTarget()
			&& LoweredPreview.GetPositions().Last()
				== Lowered.GetRequest().GetTarget()
			&& FMath::IsNearlyEqual(
				ElevatedPreview.GetApexPosition().Z, 500.0, 1.0e-6)
			&& FMath::IsNearlyEqual(
				LoweredPreview.GetApexPosition().Z, 250.0, 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcPreviewSegmentFenceTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc.SegmentFences",
	PreviewFlags)

bool FShanmenThrownWeaponArcPreviewSegmentFenceTest::RunTest(
	const FString&)
{
	const FShanmenThrownWeaponArcPlan Plan = MakePlan();
	FShanmenThrownWeaponArcPreview Reused;
	const bool bFixtureReady =
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan, 8, Reused);
	TestTrue(TEXT("fixture begins valid"),
		bFixtureReady);
	if (!bFixtureReady)
	{
		return false;
	}
	TestTrue(TEXT("below-minimum segments fail and clear reused output"),
		!FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan,
			FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount - 1,
			Reused)
			&& !Reused.IsValid()
			&& Reused.GetPositions().IsEmpty());
	TestTrue(TEXT("above-maximum segments fail closed"),
		!FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan,
			FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount + 1,
			Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("both inclusive segment boundaries are valid"),
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Plan,
			FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount,
			Reused)
			&& Reused.GetPositions().Num()
				== FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount + 1
			&& FShanmenThrownWeaponArcPreviewSampler::TrySample(
				Plan,
				FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount,
				Reused)
			&& Reused.GetPositions().Num()
				== FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcPreviewInvalidPlanTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc.InvalidPlanFailsClosed",
	PreviewFlags)

bool FShanmenThrownWeaponArcPreviewInvalidPlanTest::RunTest(const FString&)
{
	FShanmenThrownWeaponArcPreview Reused;
	const bool bFixtureReady =
		FShanmenThrownWeaponArcPreviewSampler::TrySample(
			MakePlan(), 8, Reused);
	TestTrue(TEXT("fixture begins valid"),
		bFixtureReady);
	if (!bFixtureReady)
	{
		return false;
	}
	TestTrue(TEXT("invalid plan fails and clears every output field"),
		!FShanmenThrownWeaponArcPreviewSampler::TrySample(
			FShanmenThrownWeaponArcPlan(), 8, Reused)
			&& !Reused.IsValid()
			&& !Reused.GetPreviewId().IsValid()
			&& Reused.GetSegmentCount() == 0
			&& Reused.GetPositions().IsEmpty()
			&& Reused.GetApexPosition().IsZero()
			&& Reused.GetPlannedLandingPosition().IsZero()
			&& Reused.GetFlightTimeSeconds() == 0.0);
	return true;
}

#endif
