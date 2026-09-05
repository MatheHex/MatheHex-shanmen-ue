#include "ShanmenThrownWeaponArcPreview.h"

#include "ShanmenDeterministicId.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

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

	FGuid MakePreviewId(
		const FShanmenThrownWeaponArcPlan& Plan,
		int32 SegmentCount)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ThrownWeapon.ArcPreview.r1"),
			{
				Plan.GetPlanId().ToString(EGuidFormats::Digits),
				FString::FromInt(SegmentCount)
			});
	}

	bool TryExpectedPosition(
		const FShanmenThrownWeaponArcPlan& Plan,
		int32 SegmentCount,
		int32 Index,
		FVector& OutPosition)
	{
		OutPosition = FVector::ZeroVector;
		if (!Plan.IsValid()
			|| SegmentCount
				< FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
			|| SegmentCount
				> FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount
			|| Index < 0
			|| Index > SegmentCount)
		{
			return false;
		}

		if (Index == 0)
		{
			OutPosition = Plan.GetRequest().GetOrigin();
			return true;
		}
		if (Index == SegmentCount)
		{
			OutPosition = Plan.GetRequest().GetTarget();
			return true;
		}

		const double NormalizedTime =
			static_cast<double>(Index) / static_cast<double>(SegmentCount);
		const double ElapsedSeconds =
			Plan.GetFlightTimeSeconds() * NormalizedTime;
		return FMath::IsFinite(ElapsedSeconds)
			&& Plan.TrySamplePosition(ElapsedSeconds, OutPosition)
			&& IsFiniteVector(OutPosition);
	}
}

bool FShanmenThrownWeaponArcPreview::IsValid() const
{
	if (!PreviewId.IsValid()
		|| !Plan.IsValid()
		|| SegmentCount
			< FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
		|| SegmentCount
			> FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount
		|| Positions.Num() != SegmentCount + 1
		|| !IsFiniteVector(ApexPosition)
		|| !IsFiniteVector(PlannedLandingPosition)
		|| ApexPosition != Plan.GetApexPosition()
		|| PlannedLandingPosition != Plan.GetRequest().GetTarget()
		|| !NearlyEqual(FlightTimeSeconds, Plan.GetFlightTimeSeconds())
		|| Positions[0] != Plan.GetRequest().GetOrigin()
		|| Positions.Last() != Plan.GetRequest().GetTarget()
		|| PreviewId != MakePreviewId(Plan, SegmentCount))
	{
		return false;
	}

	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		FVector Expected;
		if (!TryExpectedPosition(Plan, SegmentCount, Index, Expected)
			|| !VectorsNearlyEqual(Positions[Index], Expected))
		{
			return false;
		}
	}
	return true;
}

bool FShanmenThrownWeaponArcPreview::Matches(
	const FShanmenThrownWeaponArcPreview& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& PreviewId == Other.PreviewId
		&& Plan.Matches(Other.Plan)
		&& SegmentCount == Other.SegmentCount
		&& Positions == Other.Positions
		&& ApexPosition == Other.ApexPosition
		&& PlannedLandingPosition == Other.PlannedLandingPosition
		&& FlightTimeSeconds == Other.FlightTimeSeconds;
}

bool FShanmenThrownWeaponArcPreviewSampler::TrySample(
	const FShanmenThrownWeaponArcPlan& Plan,
	int32 SegmentCount,
	FShanmenThrownWeaponArcPreview& OutPreview)
{
	OutPreview = FShanmenThrownWeaponArcPreview();
	if (!Plan.IsValid()
		|| SegmentCount < MinimumSegmentCount
		|| SegmentCount > MaximumSegmentCount)
	{
		return false;
	}

	FShanmenThrownWeaponArcPreview Candidate;
	Candidate.Plan = Plan;
	Candidate.SegmentCount = SegmentCount;
	Candidate.Positions.Reserve(SegmentCount + 1);
	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		FVector Position;
		if (!TryExpectedPosition(Plan, SegmentCount, Index, Position))
		{
			return false;
		}
		Candidate.Positions.Add(Position);
	}

	Candidate.ApexPosition = Plan.GetApexPosition();
	Candidate.PlannedLandingPosition = Plan.GetRequest().GetTarget();
	Candidate.FlightTimeSeconds = Plan.GetFlightTimeSeconds();
	Candidate.PreviewId = MakePreviewId(Plan, SegmentCount);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPreview = MoveTemp(Candidate);
	return true;
}
