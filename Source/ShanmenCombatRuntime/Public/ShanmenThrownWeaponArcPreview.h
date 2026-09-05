#pragma once

#include "CoreMinimal.h"
#include "ShanmenThrownWeaponArcPlanner.h"

/**
 * Immutable, deterministic spatial preview of one validated ballistic plan.
 *
 * Positions are uniformly sampled in flight time. The first and final points
 * are pinned to the plan's exact origin and requested target. This value owns
 * no World, trace, collision, Actor, component, renderer, timer, or UI state.
 */
class SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcPreview
{
public:
	bool IsValid() const;
	bool Matches(const FShanmenThrownWeaponArcPreview& Other) const;

	const FGuid& GetPreviewId() const { return PreviewId; }
	const FShanmenThrownWeaponArcPlan& GetPlan() const { return Plan; }
	int32 GetSegmentCount() const { return SegmentCount; }
	const TArray<FVector>& GetPositions() const { return Positions; }
	const FVector& GetApexPosition() const { return ApexPosition; }
	const FVector& GetPlannedLandingPosition() const
	{
		return PlannedLandingPosition;
	}
	double GetFlightTimeSeconds() const { return FlightTimeSeconds; }

private:
	friend class FShanmenThrownWeaponArcPreviewSampler;

	FGuid PreviewId;
	FShanmenThrownWeaponArcPlan Plan;
	int32 SegmentCount = 0;
	TArray<FVector> Positions;
	FVector ApexPosition = FVector::ZeroVector;
	FVector PlannedLandingPosition = FVector::ZeroVector;
	double FlightTimeSeconds = 0.0;
};

/** Pure, bounded sampler built on FShanmenThrownWeaponArcPlan. */
class SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcPreviewSampler
{
public:
	static constexpr int32 MinimumSegmentCount = 2;
	static constexpr int32 MaximumSegmentCount = 128;

	static bool TrySample(
		const FShanmenThrownWeaponArcPlan& Plan,
		int32 SegmentCount,
		FShanmenThrownWeaponArcPreview& OutPreview);
};
