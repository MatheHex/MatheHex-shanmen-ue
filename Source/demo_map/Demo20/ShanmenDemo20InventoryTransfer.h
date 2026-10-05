#pragma once
#include "ShanmenItemTypes.h"

/** One player click -> one existing durable grid intent. No quantities are held
 * or written here; even a partial merge is decided by the item authority. */
struct FShanmenDemo20InventoryTransfer
{
	static bool Build(const FShanmenItemAuthoritySnapshot& Actual, const FGuid& Item,
		const FGuid& Destination, const FGuid& ActiveRun, FShanmenItemGridRequest& Out, FString& Reason);
};
