#pragma once

#include "CoreMinimal.h"

/**
 * A transient, Code B-owned description of the exact P5 carry roots selected
 * for a future run.  It never owns item state and is deliberately not a P6
 * preview or a persisted second inventory.
 */
struct FCodeBLoadoutSelectionRoot
{
	FGuid RootItemId;
	FGuid ContainerId;
	int32 SlotIndex = INDEX_NONE;
	FName SlotSemantic;
	/** Includes RootItemId and every existing child-graph item below it. */
	TArray<FGuid> GraphClosureItemIds;
};

/**
 * Read-only P5 evidence captured at StartAttempt creation and revalidated by
 * the post-world-confirmed P5 -> P6 bridge.  bEnrolled is false only when no
 * P5 sidecar exists; it never authorizes a bridge to create one.
 */
struct FCodeBLoadoutSelection
{
	bool bEnrolled = false;
	FGuid OwnerId;
	int32 PersistentRevision = INDEX_NONE;
	int32 GraphRevision = INDEX_NONE;
	TArray<FCodeBLoadoutSelectionRoot> Roots;
	FString Digest;

	bool IsUsable() const
	{
		return bEnrolled && OwnerId.IsValid() && PersistentRevision >= 1
			&& GraphRevision >= 0 && !Digest.IsEmpty();
	}
};
