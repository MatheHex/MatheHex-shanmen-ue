#pragma once

#include "CoreMinimal.h"

/** Shared, stateless XY-sector definition used by player and heavy enemy attacks. */
class Fdemo_mapSectorGeometry
{
public:
	static bool IsInsideSectorXY(
		const FVector& Origin,
		const FVector& ForwardDirection,
		const FVector& TargetLocation,
		float Radius,
		float FullAngleDegrees,
		float VerticalTolerance);
};
