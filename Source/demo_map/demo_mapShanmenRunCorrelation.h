#pragma once

#include "CoreMinimal.h"

struct FShanmenItemTransactionReceipt;
struct Fdemo_mapShanmenPreparedLoadoutReceipt;

/**
 * Immutable product correlation rebuilt from the ShanmenItems durable ledger.
 * It is transient evidence only: the authority receipts remain the sole source
 * of truth and no duplicate Run/loadout document is persisted here.
 */
struct Fdemo_mapShanmenRunCorrelation
{
	FGuid CorrelationId;
	FGuid OwnerId;
	FGuid ScopeId;
	FGuid ActiveRunId;
	FGuid PreparedRequestId;
	FGuid PreparedReceiptId;
	FGuid LifecycleRequestId;
	FGuid LifecycleReceiptId;
	int32 PreparedAuthorityRevision = INDEX_NONE;
	int32 LifecycleAuthorityRevision = INDEX_NONE;
	FGuid WeaponItemInstanceId;
	FGuid ArmorItemInstanceId;
	FGuid AccessoryItemInstanceId;
	FGuid SpatialRingItemInstanceId;
	FGuid BackpackItemInstanceId;
	TArray<FGuid> OrderedPreparedItemInstanceIds;
	TArray<FGuid> OrderedRunInventoryItemInstanceIds;
	TArray<FGuid> HotbarItemInstanceIds;

	bool IsValid() const;
	bool operator==(const Fdemo_mapShanmenRunCorrelation& Other) const;
	bool operator!=(const Fdemo_mapShanmenRunCorrelation& Other) const
	{
		return !(*this == Other);
	}
	FString ToLogString() const;

	static bool Build(
		const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared,
		const FShanmenItemTransactionReceipt& Lifecycle,
		Fdemo_mapShanmenRunCorrelation& OutCorrelation,
		FString& OutDiagnostic);
};
