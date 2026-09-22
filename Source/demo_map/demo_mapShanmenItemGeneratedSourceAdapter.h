#pragma once

#include "ShanmenItemGeneratedSource.h"

/**
 * Fresh candidate only: resolves a registered M01/P8 slot and runs the existing
 * planner once. No caller-supplied policies, seed, definitions or reward values.
 * Callers MUST query durable history before planning: recovery/replay uses the
 * stored plan, not this entry point. The service still authorizes Owner/Run,
 * catalog admission and the cursor; success here is NOT durable acceptance.
 */
struct Fdemo_mapShanmenItemGeneratedSourceAdapter
{
	static bool BuildRequest(
		const FShanmenContentStamp& ItemContent, const FGuid& OwnerId,
		const FGuid& RunId, FName SlotId, int64 ExpectedSequence, int32 PityState,
		FShanmenItemGeneratedSourceRequest& OutRequest, FString& OutDiagnostic);
};
