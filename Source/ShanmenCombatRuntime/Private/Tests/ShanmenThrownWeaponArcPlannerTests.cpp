#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenThrownWeaponArcPlanner.h"

#include <limits>

namespace
{
	const FGuid ArcRunId(0x20000001, 0, 0, 1);
	const FGuid ArcOwnerId(0x20000002, 0, 0, 1);
	const FGuid ArcSourceEntityId(0x20000003, 0, 0, 1);
	const FGuid ArcItemId(0x20000004, 0, 0, 1);
	const FGuid ArcActivationId(0x20000005, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeArcAction(
		bool bWithItem = true,
		FName ActionDefinitionId =
			FShanmenThrownWeaponArcRequest::CanonicalActionDefinitionId(),
		const FString& Digest = TEXT("TEST-DIGEST-P20.0"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ArcRunId;
		Capture.OwnerId = ArcOwnerId;
		Capture.ActivationId = ArcActivationId;
		Capture.SourceEntityId = ArcSourceEntityId;
		Capture.SourceItemInstanceId = bWithItem ? ArcItemId : FGuid();
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P20.0");
		Capture.Content.Digest = Digest;

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponArcRequestCapture MakeArcCapture(
		EShanmenThrownWeaponTechniqueTier Tier =
			EShanmenThrownWeaponTechniqueTier::Intermediate)
	{
		FShanmenThrownWeaponArcRequestCapture Capture;
		Capture.Action = MakeArcAction();
		Capture.TechniqueTier = Tier;
		Capture.Origin = FVector::ZeroVector;
		Capture.Target = FVector(1000.0, 0.0, 0.0);
		Capture.GravityMagnitude = 980.0;
		Capture.ApexClearance = 200.0;
		Capture.MaximumLaunchSpeed = 2000.0;
		Capture.MaximumFlightTime = 10.0;
		return Capture;
	}

	bool NearlyEqual(double Left, double Right, double Tolerance = 1.0e-6)
	{
		return FMath::Abs(Left - Right) <= Tolerance;
	}

	bool NearlyEqualVector(
		const FVector& Left,
		const FVector& Right,
		double Tolerance = 1.0e-6)
	{
		return Left.Equals(Right, Tolerance);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcTechniqueGateTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.TechniqueGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcTechniqueGateTest::RunTest(const FString&)
{
	const FShanmenThrownWeaponArcPlanResult Invalid =
		FShanmenThrownWeaponArcPlanner::Plan(
			FShanmenThrownWeaponArcRequestCapture());
	TestTrue(TEXT("Default capture yields an auditable typed rejection"),
		Invalid.IsValid()
			&& Invalid.Status
				== EShanmenThrownWeaponArcPlanStatus::RequestRejected
			&& !Invalid.IsPlanned());

	FShanmenThrownWeaponArcRequestCapture BeginnerCapture = MakeArcCapture(
		EShanmenThrownWeaponTechniqueTier::Beginner);
	FShanmenThrownWeaponArcRequest BeginnerRequest;
	TestTrue(TEXT("A beginner request remains valid identity evidence"),
		FShanmenThrownWeaponArcRequest::TryCapture(
			BeginnerCapture, BeginnerRequest)
			&& BeginnerRequest.IsValid()
			&& !BeginnerRequest.IsArcUnlocked());
	const FShanmenThrownWeaponArcPlanResult Locked =
		FShanmenThrownWeaponArcPlanner::Plan(BeginnerRequest);
	TestTrue(TEXT("Beginner mastery cannot plan an arc"),
		Locked.IsValid()
			&& Locked.Status
				== EShanmenThrownWeaponArcPlanStatus::TechniqueLocked
			&& Locked.Request.Matches(BeginnerRequest)
			&& !Locked.Plan.IsValid());

	const FShanmenThrownWeaponArcPlanResult Intermediate =
		FShanmenThrownWeaponArcPlanner::Plan(MakeArcCapture());
	TestTrue(TEXT("Intermediate mastery unlocks manual arc planning"),
		Intermediate.IsPlanned());
	const FShanmenThrownWeaponArcPlanResult Master =
		FShanmenThrownWeaponArcPlanner::Plan(MakeArcCapture(
			EShanmenThrownWeaponTechniqueTier::Master));
	TestTrue(TEXT("Master mastery retains the intermediate operation"),
		Master.IsPlanned());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcLevelTargetTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.LevelTargetPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcLevelTargetTest::RunTest(const FString&)
{
	const FShanmenThrownWeaponArcRequestCapture Capture = MakeArcCapture();
	const FShanmenThrownWeaponArcPlanResult Result =
		FShanmenThrownWeaponArcPlanner::Plan(Capture);
	TestTrue(TEXT("A level target produces a self-validating plan"),
		Result.IsPlanned()
			&& Result.Plan.GetRequest().Matches(Result.Request)
			&& Result.Plan.GetLaunchSpeed()
				<= Capture.MaximumLaunchSpeed
			&& Result.Plan.GetFlightTimeSeconds()
				<= Capture.MaximumFlightTime);

	FVector Start = FVector::ZeroVector;
	FVector Apex = FVector::ZeroVector;
	FVector Target = FVector::ZeroVector;
	TestTrue(TEXT("The plan samples start apex and target"),
		Result.Plan.TrySamplePosition(0.0, Start)
			&& Result.Plan.TrySamplePosition(
				Result.Plan.GetTimeToApexSeconds(), Apex)
			&& Result.Plan.TrySamplePosition(
				Result.Plan.GetFlightTimeSeconds(), Target));
	TestTrue(TEXT("Start sample is exact"),
		NearlyEqualVector(Start, Capture.Origin));
	TestTrue(TEXT("Apex is the requested clearance"),
		NearlyEqual(Apex.Z, 200.0)
			&& NearlyEqualVector(Apex, Result.Plan.GetApexPosition()));
	TestTrue(TEXT("Terminal sample reconstructs the target"),
		NearlyEqualVector(Target, Capture.Target));
	TestTrue(TEXT("Vertical velocity is zero at the apex"),
		NearlyEqual(
			Result.Plan.GetInitialVelocity().Z
				+ Result.Plan.GetGravityAcceleration().Z
					* Result.Plan.GetTimeToApexSeconds(),
			0.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcHeightVariantsTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.HeightVariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcHeightVariantsTest::RunTest(const FString&)
{
	FShanmenThrownWeaponArcRequestCapture Elevated = MakeArcCapture();
	Elevated.Origin = FVector(100.0, -50.0, 20.0);
	Elevated.Target = FVector(850.0, 300.0, 250.0);
	Elevated.ApexClearance = 150.0;
	const FShanmenThrownWeaponArcPlanResult ElevatedResult =
		FShanmenThrownWeaponArcPlanner::Plan(Elevated);
	FVector ElevatedTarget = FVector::ZeroVector;
	TestTrue(TEXT("An elevated target has a valid asymmetric arc"),
		ElevatedResult.IsPlanned()
			&& NearlyEqual(ElevatedResult.Plan.GetApexPosition().Z, 400.0)
			&& ElevatedResult.Plan.TrySamplePosition(
				ElevatedResult.Plan.GetFlightTimeSeconds(),
				ElevatedTarget)
			&& NearlyEqualVector(ElevatedTarget, Elevated.Target));

	FShanmenThrownWeaponArcRequestCapture Depressed = MakeArcCapture();
	Depressed.Origin = FVector(-250.0, 80.0, 100.0);
	Depressed.Target = FVector(700.0, -400.0, -300.0);
	Depressed.ApexClearance = 175.0;
	const FShanmenThrownWeaponArcPlanResult DepressedResult =
		FShanmenThrownWeaponArcPlanner::Plan(Depressed);
	FVector DepressedTarget = FVector::ZeroVector;
	TestTrue(TEXT("A depressed target has a valid asymmetric arc"),
		DepressedResult.IsPlanned()
			&& NearlyEqual(DepressedResult.Plan.GetApexPosition().Z, 275.0)
			&& DepressedResult.Plan.TrySamplePosition(
				DepressedResult.Plan.GetFlightTimeSeconds(),
				DepressedTarget)
			&& NearlyEqualVector(DepressedTarget, Depressed.Target));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcEnvelopeTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.EnvelopeFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcEnvelopeTest::RunTest(const FString&)
{
	FShanmenThrownWeaponArcRequestCapture SpeedLimited = MakeArcCapture();
	SpeedLimited.MaximumLaunchSpeed = 100.0;
	const FShanmenThrownWeaponArcPlanResult SpeedRejected =
		FShanmenThrownWeaponArcPlanner::Plan(SpeedLimited);
	TestTrue(TEXT("A valid request can exceed its speed envelope"),
		SpeedRejected.IsValid()
			&& SpeedRejected.Status
				== EShanmenThrownWeaponArcPlanStatus::Unreachable
			&& SpeedRejected.Request.IsValid()
			&& !SpeedRejected.Plan.IsValid());

	FShanmenThrownWeaponArcRequestCapture TimeLimited = MakeArcCapture();
	TimeLimited.MaximumFlightTime = 0.25;
	const FShanmenThrownWeaponArcPlanResult TimeRejected =
		FShanmenThrownWeaponArcPlanner::Plan(TimeLimited);
	TestTrue(TEXT("A valid request can exceed its time envelope"),
		TimeRejected.IsValid()
			&& TimeRejected.Status
				== EShanmenThrownWeaponArcPlanStatus::Unreachable
			&& TimeRejected.Request.IsValid());

	FShanmenThrownWeaponArcRequestCapture Taller = MakeArcCapture();
	Taller.ApexClearance = 500.0;
	const FShanmenThrownWeaponArcPlanResult TallerResult =
		FShanmenThrownWeaponArcPlanner::Plan(Taller);
	TestTrue(TEXT("A content envelope can admit a taller manual arc"),
		TallerResult.IsPlanned()
			&& NearlyEqual(TallerResult.Plan.GetApexPosition().Z, 500.0)
			&& TallerResult.Plan.GetFlightTimeSeconds()
				> FShanmenThrownWeaponArcPlanner::Plan(
					MakeArcCapture()).Plan.GetFlightTimeSeconds());

	FShanmenThrownWeaponArcPlanResult ForgedUnreachable;
	ForgedUnreachable.Status =
		EShanmenThrownWeaponArcPlanStatus::Unreachable;
	ForgedUnreachable.Diagnostic = TEXT("Forged unreachable result.");
	ForgedUnreachable.Request = TallerResult.Request;
	TestFalse(TEXT("A reachable request cannot masquerade as unreachable"),
		ForgedUnreachable.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcDeterminismTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.DeterminismAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcDeterminismTest::RunTest(const FString&)
{
	const FShanmenThrownWeaponArcRequestCapture Capture = MakeArcCapture();
	const FShanmenThrownWeaponArcPlanResult First =
		FShanmenThrownWeaponArcPlanner::Plan(Capture);
	const FShanmenThrownWeaponArcPlanResult Replay =
		FShanmenThrownWeaponArcPlanner::Plan(Capture);
	TestTrue(TEXT("Equivalent inputs reproduce request and plan identity"),
		First.IsPlanned()
			&& Replay.IsPlanned()
			&& First.Request.Matches(Replay.Request)
			&& First.Plan.Matches(Replay.Plan));

	FShanmenThrownWeaponArcRequestCapture NegativeZero = Capture;
	NegativeZero.Origin.X = -0.0;
	const FShanmenThrownWeaponArcPlanResult CanonicalReplay =
		FShanmenThrownWeaponArcPlanner::Plan(NegativeZero);
	TestTrue(TEXT("Signed zero cannot split deterministic identity"),
		CanonicalReplay.IsPlanned()
			&& First.Request.Matches(CanonicalReplay.Request)
			&& First.Plan.Matches(CanonicalReplay.Plan));

	FShanmenThrownWeaponArcRequestCapture OtherApex = Capture;
	OtherApex.ApexClearance += 50.0;
	const FShanmenThrownWeaponArcPlanResult OtherApexResult =
		FShanmenThrownWeaponArcPlanner::Plan(OtherApex);
	TestTrue(TEXT("A different manual arc selection has new identity"),
		OtherApexResult.IsPlanned()
			&& OtherApexResult.Request.GetRequestId()
				!= First.Request.GetRequestId()
			&& OtherApexResult.Plan.GetPlanId()
				!= First.Plan.GetPlanId());

	FShanmenThrownWeaponArcRequestCapture OtherContent = Capture;
	OtherContent.Action = MakeArcAction(
		true,
		FShanmenThrownWeaponArcRequest::CanonicalActionDefinitionId(),
		TEXT("TEST-DIGEST-P20.0-B"));
	const FShanmenThrownWeaponArcPlanResult OtherContentResult =
		FShanmenThrownWeaponArcPlanner::Plan(OtherContent);
	TestTrue(TEXT("Content provenance participates in identity"),
		OtherContentResult.IsPlanned()
			&& OtherContentResult.Request.GetRequestId()
				!= First.Request.GetRequestId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenThrownWeaponArcFailureFenceTest,
	"Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.FailureAndSamplingFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenThrownWeaponArcFailureFenceTest::RunTest(const FString&)
{
	FShanmenThrownWeaponArcRequestCapture Invalid = MakeArcCapture();
	Invalid.Target = Invalid.Origin;
	TestTrue(TEXT("Zero-displacement requests are rejected"),
		FShanmenThrownWeaponArcPlanner::Plan(Invalid).Status
			== EShanmenThrownWeaponArcPlanStatus::RequestRejected);
	Invalid = MakeArcCapture();
	Invalid.GravityMagnitude = std::numeric_limits<double>::quiet_NaN();
	TestTrue(TEXT("Non-finite gravity is rejected"),
		FShanmenThrownWeaponArcPlanner::Plan(Invalid).Status
			== EShanmenThrownWeaponArcPlanStatus::RequestRejected);
	Invalid = MakeArcCapture();
	Invalid.Action = MakeArcAction(false);
	TestTrue(TEXT("Arc planning requires one exact physical item"),
		FShanmenThrownWeaponArcPlanner::Plan(Invalid).Status
			== EShanmenThrownWeaponArcPlanStatus::RequestRejected);
	Invalid = MakeArcCapture();
	Invalid.Action = MakeArcAction(
		true, TEXT("Combat.Action.Projectile.Generic"));
	TestTrue(TEXT("A generic projectile cannot enter the arc contract"),
		FShanmenThrownWeaponArcPlanner::Plan(Invalid).Status
			== EShanmenThrownWeaponArcPlanStatus::RequestRejected);

	const FShanmenThrownWeaponArcPlanResult Planned =
		FShanmenThrownWeaponArcPlanner::Plan(MakeArcCapture());
	FVector Sample(1.0, 1.0, 1.0);
	TestFalse(TEXT("Negative sample time fails closed"),
		Planned.Plan.TrySamplePosition(-0.01, Sample));
	TestTrue(TEXT("Rejected sampling clears output"),
		Sample == FVector::ZeroVector);
	TestFalse(TEXT("Non-finite sample time fails closed"),
		Planned.Plan.TrySamplePosition(
			std::numeric_limits<double>::quiet_NaN(), Sample));
	TestFalse(TEXT("Post-flight sample time fails closed"),
		Planned.Plan.TrySamplePosition(
			Planned.Plan.GetFlightTimeSeconds() + 0.01, Sample));
	TestTrue(TEXT("An in-range midpoint remains finite"),
		Planned.Plan.TrySamplePosition(
			Planned.Plan.GetFlightTimeSeconds() * 0.5, Sample)
			&& FMath::IsFinite(Sample.X)
			&& FMath::IsFinite(Sample.Y)
			&& FMath::IsFinite(Sample.Z));
	return true;
}

#endif
