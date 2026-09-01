#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapRewardSourceProjection.h"

namespace demo_mapRewardProjectionTestSupport
{
	/**
	 * Registry entries are policy prototypes. Planner unit tests must attach
	 * explicit distribution/slot identity before planning, exactly as the M01
	 * and full-map product distributors do at runtime.
	 */
	inline Fdemo_mapRewardSourceProjection BindPrototype(
		const Fdemo_mapRewardSourceProjection& Prototype)
	{
		Fdemo_mapRewardSourceProjection Result = Prototype;
		const FString ProjectionIdentity = Prototype.ProjectionId.ToString();
		Result.DistributionProfileId = FName(*FString::Printf(
			TEXT("Automation.Distribution.%s"),
			*ProjectionIdentity));
		Result.SlotId = FName(*FString::Printf(
			TEXT("Automation.Slot.%s"),
			*ProjectionIdentity));
		return Result;
	}

	inline Fdemo_mapRewardSourceProjection FindBound(FName ProjectionId)
	{
		const Fdemo_mapRewardSourceProjection* Prototype =
			Fdemo_mapRewardSourceProjectionRegistry::Find(ProjectionId);
		return Prototype
			? BindPrototype(*Prototype)
			: Fdemo_mapRewardSourceProjection();
	}
}

#endif
