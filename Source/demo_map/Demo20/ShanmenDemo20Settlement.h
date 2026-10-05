#pragma once
#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

/** Presentation only: never computes inventory balances or issues item writes. */
struct FShanmenDemo20Settlement
{
	static FString Describe(const FShanmenItemAuthoritySnapshot& Authority, const FGuid& Run);
	/** Most recent complete saved item report; empty if there is none. Never loads a World. */
	static FString DescribeLatestSaved(const FShanmenItemAuthoritySnapshot& Authority);
};
