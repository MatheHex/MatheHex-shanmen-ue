#pragma once

#include "ShanmenItemTypes.h"

/** Reserve a frozen loadout and publish its prepared Run in one durable write.
 * Intermediate reservations exist only in a candidate repository. The ledger
 * retains the existing Reserve and StartPreparedRun receipts, not another schema.
 */
struct SHANMENITEMS_API FShanmenItemLoadoutStartRequest
{
	FShanmenOperationContext Context;
	int32 ExpectedAuthorityRevision = INDEX_NONE;
	TArray<FShanmenItemReserveRequest> Lines;

	static FGuid MakeRequestId(const FShanmenOperationContext& Context, int32 Revision);
	static FGuid MakeLineRequestId(const FGuid& StartRequestId, const FGuid& ItemId);
};
