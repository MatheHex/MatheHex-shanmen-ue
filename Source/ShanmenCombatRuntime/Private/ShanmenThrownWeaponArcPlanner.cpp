#include "ShanmenThrownWeaponArcPlanner.h"

#include "ShanmenDeterministicId.h"

namespace
{
	double CanonicalZero(double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FVector CanonicalVector(FVector Value)
	{
		Value.X = CanonicalZero(Value.X);
		Value.Y = CanonicalZero(Value.Y);
		Value.Z = CanonicalZero(Value.Z);
		return Value;
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool IsTechniqueTierValid(EShanmenThrownWeaponTechniqueTier Tier)
	{
		return Tier == EShanmenThrownWeaponTechniqueTier::Beginner
			|| Tier == EShanmenThrownWeaponTechniqueTier::Intermediate
			|| Tier == EShanmenThrownWeaponTechniqueTier::Master;
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	FGuid MakeRequestId(const FShanmenThrownWeaponArcRequest& Request)
	{
		const FShanmenCombatActionSnapshot& Action = Request.GetAction();
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ThrownWeapon.ArcRequest.r1"),
			{
				GuidDigits(Action.GetRunId()),
				GuidDigits(Action.GetOwnerId()),
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				GuidDigits(Action.GetSourceItemInstanceId()),
				Action.GetActionDefinitionId().ToString(),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest,
				FString::FromInt(
					static_cast<int32>(Request.GetTechniqueTier())),
				DoubleBits(Request.GetOrigin().X),
				DoubleBits(Request.GetOrigin().Y),
				DoubleBits(Request.GetOrigin().Z),
				DoubleBits(Request.GetTarget().X),
				DoubleBits(Request.GetTarget().Y),
				DoubleBits(Request.GetTarget().Z),
				DoubleBits(Request.GetGravityMagnitude()),
				DoubleBits(Request.GetApexClearance()),
				DoubleBits(Request.GetMaximumLaunchSpeed()),
				DoubleBits(Request.GetMaximumFlightTime())
			});
	}

	struct FSolvedArc
	{
		FVector InitialVelocity = FVector::ZeroVector;
		FVector GravityAcceleration = FVector::ZeroVector;
		FVector ApexPosition = FVector::ZeroVector;
		double LaunchSpeed = 0.0;
		double TimeToApexSeconds = 0.0;
		double FlightTimeSeconds = 0.0;
	};

	bool NearlyEqual(double Left, double Right)
	{
		const double Scale = FMath::Max(
			1.0,
			FMath::Max(FMath::Abs(Left), FMath::Abs(Right)));
		return FMath::Abs(Left - Right) <= Scale * 1.0e-9;
	}

	bool VectorsNearlyEqual(const FVector& Left, const FVector& Right)
	{
		return NearlyEqual(Left.X, Right.X)
			&& NearlyEqual(Left.Y, Right.Y)
			&& NearlyEqual(Left.Z, Right.Z);
	}

	bool TrySolve(
		const FShanmenThrownWeaponArcRequest& Request,
		FSolvedArc& OutSolution)
	{
		OutSolution = FSolvedArc();
		if (!Request.IsValid() || !Request.IsArcUnlocked())
		{
			return false;
		}

		const FVector Origin = Request.GetOrigin();
		const FVector Target = Request.GetTarget();
		const double Gravity = Request.GetGravityMagnitude();
		const double ApexZ = FMath::Max(Origin.Z, Target.Z)
			+ Request.GetApexClearance();
		const double Rise = ApexZ - Origin.Z;
		const double Fall = ApexZ - Target.Z;
		if (!FMath::IsFinite(ApexZ)
			|| !FMath::IsFinite(Rise)
			|| !FMath::IsFinite(Fall)
			|| Rise <= 0.0
			|| Fall <= 0.0)
		{
			return false;
		}

		const double VerticalSpeedSquared = 2.0 * Gravity * Rise;
		const double DownTimeSquared = 2.0 * Fall / Gravity;
		if (!FMath::IsFinite(VerticalSpeedSquared)
			|| !FMath::IsFinite(DownTimeSquared)
			|| VerticalSpeedSquared <= 0.0
			|| DownTimeSquared <= 0.0)
		{
			return false;
		}

		const double VerticalSpeed = FMath::Sqrt(VerticalSpeedSquared);
		const double TimeToApex = VerticalSpeed / Gravity;
		const double TimeFromApex = FMath::Sqrt(DownTimeSquared);
		const double FlightTime = TimeToApex + TimeFromApex;
		if (!FMath::IsFinite(VerticalSpeed)
			|| !FMath::IsFinite(TimeToApex)
			|| !FMath::IsFinite(TimeFromApex)
			|| !FMath::IsFinite(FlightTime)
			|| FlightTime <= 0.0
			|| FlightTime > Request.GetMaximumFlightTime())
		{
			return false;
		}

		FVector InitialVelocity(
			(Target.X - Origin.X) / FlightTime,
			(Target.Y - Origin.Y) / FlightTime,
			VerticalSpeed);
		InitialVelocity = CanonicalVector(InitialVelocity);
		const FVector GravityAcceleration(0.0, 0.0, -Gravity);
		const double LaunchSpeed = InitialVelocity.Size();
		if (!IsFiniteVector(InitialVelocity)
			|| InitialVelocity.IsNearlyZero()
			|| !FMath::IsFinite(LaunchSpeed)
			|| LaunchSpeed <= 0.0
			|| LaunchSpeed > Request.GetMaximumLaunchSpeed())
		{
			return false;
		}

		const FVector ApexPosition = CanonicalVector(
			Origin
			+ InitialVelocity * TimeToApex
			+ 0.5 * GravityAcceleration
				* TimeToApex * TimeToApex);
		const FVector ReconstructedTarget = CanonicalVector(
			Origin
			+ InitialVelocity * FlightTime
			+ 0.5 * GravityAcceleration
				* FlightTime * FlightTime);
		if (!IsFiniteVector(ApexPosition)
			|| !IsFiniteVector(ReconstructedTarget)
			|| !NearlyEqual(ApexPosition.Z, ApexZ)
			|| !VectorsNearlyEqual(ReconstructedTarget, Target))
		{
			return false;
		}

		OutSolution.InitialVelocity = InitialVelocity;
		OutSolution.GravityAcceleration = GravityAcceleration;
		OutSolution.ApexPosition = ApexPosition;
		OutSolution.LaunchSpeed = CanonicalZero(LaunchSpeed);
		OutSolution.TimeToApexSeconds = CanonicalZero(TimeToApex);
		OutSolution.FlightTimeSeconds = CanonicalZero(FlightTime);
		return true;
	}

	FGuid MakePlanId(
		const FShanmenThrownWeaponArcRequest& Request,
		const FSolvedArc& Solution)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ThrownWeapon.ArcPlan.r1"),
			{
				GuidDigits(Request.GetRequestId()),
				DoubleBits(Solution.InitialVelocity.X),
				DoubleBits(Solution.InitialVelocity.Y),
				DoubleBits(Solution.InitialVelocity.Z),
				DoubleBits(Solution.GravityAcceleration.Z),
				DoubleBits(Solution.ApexPosition.X),
				DoubleBits(Solution.ApexPosition.Y),
				DoubleBits(Solution.ApexPosition.Z),
				DoubleBits(Solution.LaunchSpeed),
				DoubleBits(Solution.TimeToApexSeconds),
				DoubleBits(Solution.FlightTimeSeconds)
			});
	}

	bool PlanMatchesSolution(
		const FShanmenThrownWeaponArcPlan& Plan,
		const FSolvedArc& Solution)
	{
		return VectorsNearlyEqual(
				Plan.GetInitialVelocity(), Solution.InitialVelocity)
			&& VectorsNearlyEqual(
				Plan.GetGravityAcceleration(),
				Solution.GravityAcceleration)
			&& VectorsNearlyEqual(
				Plan.GetApexPosition(), Solution.ApexPosition)
			&& NearlyEqual(Plan.GetLaunchSpeed(), Solution.LaunchSpeed)
			&& NearlyEqual(
				Plan.GetTimeToApexSeconds(),
				Solution.TimeToApexSeconds)
			&& NearlyEqual(
				Plan.GetFlightTimeSeconds(),
				Solution.FlightTimeSeconds);
	}
}

FName FShanmenThrownWeaponArcRequest::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.ThrownWeapon.Arc01");
}

bool FShanmenThrownWeaponArcRequest::TryCapture(
	const FShanmenThrownWeaponArcRequestCapture& Capture,
	FShanmenThrownWeaponArcRequest& OutRequest)
{
	OutRequest = FShanmenThrownWeaponArcRequest();
	if (!Capture.Action.IsValid()
		|| !Capture.Action.GetSourceItemInstanceId().IsValid()
		|| Capture.Action.GetActionDefinitionId()
			!= CanonicalActionDefinitionId()
		|| !IsTechniqueTierValid(Capture.TechniqueTier)
		|| !IsFiniteVector(Capture.Origin)
		|| !IsFiniteVector(Capture.Target)
		|| Capture.Origin.Equals(Capture.Target, UE_DOUBLE_SMALL_NUMBER)
		|| !FMath::IsFinite(Capture.GravityMagnitude)
		|| Capture.GravityMagnitude <= 0.0
		|| !FMath::IsFinite(Capture.ApexClearance)
		|| Capture.ApexClearance <= 0.0
		|| !FMath::IsFinite(Capture.MaximumLaunchSpeed)
		|| Capture.MaximumLaunchSpeed <= 0.0
		|| !FMath::IsFinite(Capture.MaximumFlightTime)
		|| Capture.MaximumFlightTime <= 0.0)
	{
		return false;
	}

	FShanmenThrownWeaponArcRequest Candidate;
	Candidate.Action = Capture.Action;
	Candidate.TechniqueTier = Capture.TechniqueTier;
	Candidate.Origin = CanonicalVector(Capture.Origin);
	Candidate.Target = CanonicalVector(Capture.Target);
	Candidate.GravityMagnitude = CanonicalZero(Capture.GravityMagnitude);
	Candidate.ApexClearance = CanonicalZero(Capture.ApexClearance);
	Candidate.MaximumLaunchSpeed =
		CanonicalZero(Capture.MaximumLaunchSpeed);
	Candidate.MaximumFlightTime =
		CanonicalZero(Capture.MaximumFlightTime);
	Candidate.RequestId = MakeRequestId(Candidate);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutRequest = MoveTemp(Candidate);
	return true;
}

bool FShanmenThrownWeaponArcRequest::IsValid() const
{
	return RequestId.IsValid()
		&& Action.IsValid()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId() == CanonicalActionDefinitionId()
		&& IsTechniqueTierValid(TechniqueTier)
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(Target)
		&& !Origin.Equals(Target, UE_DOUBLE_SMALL_NUMBER)
		&& FMath::IsFinite(GravityMagnitude)
		&& GravityMagnitude > 0.0
		&& FMath::IsFinite(ApexClearance)
		&& ApexClearance > 0.0
		&& FMath::IsFinite(MaximumLaunchSpeed)
		&& MaximumLaunchSpeed > 0.0
		&& FMath::IsFinite(MaximumFlightTime)
		&& MaximumFlightTime > 0.0
		&& RequestId == MakeRequestId(*this);
}

bool FShanmenThrownWeaponArcRequest::Matches(
	const FShanmenThrownWeaponArcRequest& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& RequestId == Other.RequestId
		&& ActionsMatch(Action, Other.Action)
		&& TechniqueTier == Other.TechniqueTier
		&& Origin == Other.Origin
		&& Target == Other.Target
		&& GravityMagnitude == Other.GravityMagnitude
		&& ApexClearance == Other.ApexClearance
		&& MaximumLaunchSpeed == Other.MaximumLaunchSpeed
		&& MaximumFlightTime == Other.MaximumFlightTime;
}

bool FShanmenThrownWeaponArcPlan::IsValid() const
{
	FSolvedArc Solution;
	return PlanId.IsValid()
		&& Request.IsValid()
		&& Request.IsArcUnlocked()
		&& TrySolve(Request, Solution)
		&& PlanMatchesSolution(*this, Solution)
		&& PlanId == MakePlanId(Request, Solution);
}

bool FShanmenThrownWeaponArcPlan::Matches(
	const FShanmenThrownWeaponArcPlan& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& PlanId == Other.PlanId
		&& Request.Matches(Other.Request)
		&& InitialVelocity == Other.InitialVelocity
		&& GravityAcceleration == Other.GravityAcceleration
		&& ApexPosition == Other.ApexPosition
		&& LaunchSpeed == Other.LaunchSpeed
		&& TimeToApexSeconds == Other.TimeToApexSeconds
		&& FlightTimeSeconds == Other.FlightTimeSeconds;
}

bool FShanmenThrownWeaponArcPlan::TrySamplePosition(
	double ElapsedSeconds,
	FVector& OutPosition) const
{
	OutPosition = FVector::ZeroVector;
	if (!IsValid()
		|| !FMath::IsFinite(ElapsedSeconds)
		|| ElapsedSeconds < 0.0
		|| ElapsedSeconds > FlightTimeSeconds)
	{
		return false;
	}

	OutPosition = CanonicalVector(
		Request.GetOrigin()
		+ InitialVelocity * ElapsedSeconds
		+ 0.5 * GravityAcceleration
			* ElapsedSeconds * ElapsedSeconds);
	return IsFiniteVector(OutPosition);
}

bool FShanmenThrownWeaponArcPlanResult::IsValid() const
{
	if (Status == EShanmenThrownWeaponArcPlanStatus::Invalid
		|| Diagnostic.IsEmpty())
	{
		return false;
	}

	switch (Status)
	{
	case EShanmenThrownWeaponArcPlanStatus::Planned:
		return Request.IsValid()
			&& Request.IsArcUnlocked()
			&& Plan.IsValid()
			&& Plan.GetRequest().Matches(Request);
	case EShanmenThrownWeaponArcPlanStatus::RequestRejected:
		return !Request.IsValid() && !Plan.IsValid();
	case EShanmenThrownWeaponArcPlanStatus::TechniqueLocked:
		return Request.IsValid()
			&& Request.GetTechniqueTier()
				== EShanmenThrownWeaponTechniqueTier::Beginner
			&& !Plan.IsValid();
	case EShanmenThrownWeaponArcPlanStatus::Unreachable:
	{
		FSolvedArc Solution;
		return Request.IsValid()
			&& Request.IsArcUnlocked()
			&& !Plan.IsValid()
			&& !TrySolve(Request, Solution);
	}
	default:
		return false;
	}
}

FShanmenThrownWeaponArcPlanResult FShanmenThrownWeaponArcPlanner::Plan(
	const FShanmenThrownWeaponArcRequestCapture& Capture)
{
	FShanmenThrownWeaponArcRequest Request;
	if (!FShanmenThrownWeaponArcRequest::TryCapture(Capture, Request))
	{
		FShanmenThrownWeaponArcPlanResult Result;
		Result.Status = EShanmenThrownWeaponArcPlanStatus::RequestRejected;
		Result.Diagnostic =
			TEXT("Thrown weapon arc request capture is invalid.");
		return Result;
	}
	return Plan(Request);
}

FShanmenThrownWeaponArcPlanResult FShanmenThrownWeaponArcPlanner::Plan(
	const FShanmenThrownWeaponArcRequest& Request)
{
	FShanmenThrownWeaponArcPlanResult Result;
	if (!Request.IsValid())
	{
		Result.Status = EShanmenThrownWeaponArcPlanStatus::RequestRejected;
		Result.Diagnostic = TEXT("Thrown weapon arc request is invalid.");
		return Result;
	}

	Result.Request = Request;
	if (!Request.IsArcUnlocked())
	{
		Result.Status = EShanmenThrownWeaponArcPlanStatus::TechniqueLocked;
		Result.Diagnostic =
			TEXT("Intermediate Hidden Weapon Mastery is required for arc planning.");
		return Result;
	}

	FSolvedArc Solution;
	if (!TrySolve(Request, Solution))
	{
		Result.Status = EShanmenThrownWeaponArcPlanStatus::Unreachable;
		Result.Diagnostic =
			TEXT("Requested arc exceeds its launch-speed or flight-time envelope.");
		return Result;
	}

	Result.Plan.Request = Request;
	Result.Plan.InitialVelocity = Solution.InitialVelocity;
	Result.Plan.GravityAcceleration = Solution.GravityAcceleration;
	Result.Plan.ApexPosition = Solution.ApexPosition;
	Result.Plan.LaunchSpeed = Solution.LaunchSpeed;
	Result.Plan.TimeToApexSeconds = Solution.TimeToApexSeconds;
	Result.Plan.FlightTimeSeconds = Solution.FlightTimeSeconds;
	Result.Plan.PlanId = MakePlanId(Request, Solution);
	Result.Status = EShanmenThrownWeaponArcPlanStatus::Planned;
	Result.Diagnostic = TEXT("Planned one deterministic thrown weapon arc.");
	if (!Result.IsValid())
	{
		return FShanmenThrownWeaponArcPlanResult();
	}
	return Result;
}
