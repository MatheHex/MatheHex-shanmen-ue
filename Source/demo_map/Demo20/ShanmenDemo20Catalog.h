#pragma once
#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"
#include "ShanmenItemResupply.h"

/** Immutable product definitions. Quantities/placements exist only in ShanmenItems. */
struct FShanmenDemo20Catalog
{
	static FName ProductId() { return TEXT("Demo20.Expedition"); }
	static FGuid OwnerId();
	static FGuid ScopeId();
	static FGuid ContainerId(FName Role);
	static FShanmenItemAuthoritySnapshot Initial(int32 TestMoney = 1000000);
	static FShanmenContentStamp ContentStamp();
	static bool Definition(FName Id, FShanmenItemDefinition& Out);
	static FString ItemName(FName DefinitionId);
	static FString ItemPurpose(FName DefinitionId);
	static FString ContainerName(FName Role);
	static FShanmenItemBasicSupplyRequest BasicSupply(const FShanmenItemAuthoritySnapshot& Snapshot);
};
