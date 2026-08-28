#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

struct Fdemo_mapPersistentItemRecord;
struct Fdemo_mapRuntimeSettlementItem;

/**
 * One product-boundary codec for resolved reward metadata. It validates the
 * current demo_map policy before producing registry-independent authority data.
 */
struct Fdemo_mapShanmenItemMetadataAdapter
{
	static bool FromPersistentItem(
		const Fdemo_mapPersistentItemRecord& Source,
		FShanmenItemRewardMetadata& OutMetadata,
		FString& OutDiagnostic);

	static bool FromRuntimeItem(
		const Fdemo_mapRuntimeSettlementItem& Source,
		FShanmenItemRewardMetadata& OutMetadata,
		FString& OutDiagnostic);
};
