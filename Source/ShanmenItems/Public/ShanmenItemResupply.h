#pragma once

#include "ShanmenItemTypes.h"

/** Product-owned fixed policy, never an arbitrary list supplied by a UI. */
struct SHANMENITEMS_API FShanmenItemBasicSupplyLine
{
	FName DefinitionId;
	FGuid DestinationContainerId;
	int32 Quantity = 1;
};

/** One finite assistance opportunity per durably finalized Death, not a new genesis. */
struct SHANMENITEMS_API FShanmenItemBasicSupplyRequest
{
	FShanmenOperationContext Context;
	FGuid DeathRequestId;
	FName PolicyId;
	int32 ExpectedAuthorityRevision = INDEX_NONE;
	TArray<FShanmenItemBasicSupplyLine> Lines;

	static FGuid MakeRequestId(const FGuid& OwnerId, const FGuid& ScopeId,
		const FGuid& DeathRequestId, FName PolicyId);
	static FGuid MakeItemId(const FGuid& OwnerId, const FGuid& ScopeId,
		const FGuid& DeathRequestId, FName DefinitionId);
};
