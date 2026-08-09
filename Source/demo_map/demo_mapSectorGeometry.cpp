#include "demo_mapSectorGeometry.h"

bool Fdemo_mapSectorGeometry::IsInsideSectorXY(
	const FVector& Origin,
	const FVector& ForwardDirection,
	const FVector& TargetLocation,
	float Radius,
	float FullAngleDegrees,
	float VerticalTolerance)
{
	FVector Forward(ForwardDirection.X, ForwardDirection.Y, 0.0f);
	if (!Forward.Normalize() || Radius <= 0.0f || FullAngleDegrees <= 0.0f || VerticalTolerance < 0.0f)
	{
		return false;
	}
	FVector ToTarget = TargetLocation - Origin;
	if (FMath::Abs(ToTarget.Z) > VerticalTolerance + KINDA_SMALL_NUMBER)
	{
		return false;
	}
	ToTarget.Z = 0.0f;
	const float DistanceXY = ToTarget.Size();
	if (DistanceXY <= KINDA_SMALL_NUMBER || DistanceXY > Radius + KINDA_SMALL_NUMBER)
	{
		return false;
	}
	ToTarget /= DistanceXY;
	const float HalfAngle = FMath::Clamp(FullAngleDegrees * 0.5f, 0.0f, 180.0f);
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngle));
	return FVector::DotProduct(Forward, ToTarget) + 0.0001f >= CosHalfAngle;
}
