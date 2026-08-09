#pragma once

#include "CoreMinimal.h"

class ACharacter;

struct Fdemo_mapCombatDisplacementResult
{
	float RequestedDistance = 0.0f;
	float ResolvedDistance = 0.0f;
	bool bBlocked = false;
	FHitResult BlockingHit;
};

/** Shared, planar, swept movement authority for P6 skills and knockback. */
class Fdemo_mapCombatDisplacement
{
public:
	static bool NormalizePlanarDirection(const FVector& Candidate, FVector& OutDirection);
	static bool ResolvePlanarDirectionWithFallback(
		const FVector& Primary,
		const FVector& Fallback,
		FVector& OutDirection);
	static float ClampPreflightDistance(float RequestedDistance, float HitDistance, bool bBlockingHit);
	static Fdemo_mapCombatDisplacementResult PreflightWorldStatic(
		const ACharacter* Character,
		const FVector& PlanarDirection,
		float RequestedDistance);
	static Fdemo_mapCombatDisplacementResult MoveCharacterSwept(
		ACharacter* Character,
		const FVector& PlanarDirection,
		float RequestedDistance);
};
