#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

struct Fdemo_mapPersistentItemRecord;
struct Fdemo_mapRuntimeSettlementItem;
struct Fdemo_mapRewardPlannedStack;

/**
 * One product-boundary codec for resolved reward metadata. It validates the
 * current demo_map policy before producing registry-independent authority data.
 */
struct Fdemo_mapShanmenItemMetadataAdapter
{
	static bool FromPlannedStack(
		const Fdemo_mapRewardPlannedStack& Source,
		FShanmenItemRewardMetadata& OutMetadata,
		FString& OutDiagnostic);

	static bool FromPersistentItem(
		const Fdemo_mapPersistentItemRecord& Source,
		FShanmenItemRewardMetadata& OutMetadata,
		FString& OutDiagnostic);

	static bool FromRuntimeItem(
		const Fdemo_mapRuntimeSettlementItem& Source,
		FShanmenItemRewardMetadata& OutMetadata,
		FString& OutDiagnostic);
};
