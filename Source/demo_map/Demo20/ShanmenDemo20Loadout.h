#pragma once
#include "ShanmenItemLoadout.h"

/** Frozen read model from the sole item ledger, never a second inventory. */
struct FShanmenDemo20ActiveLoadout
{
	FGuid RunId;
	FGuid StartRequestId;
	uint64 RunSeed = 0;
	TArray<FShanmenItemRunSecuredOriginal> RemainingOriginals;
};

struct FShanmenDemo20Loadout
{
	/** Ordinary equipment and Carry only. Secure equipment/content stay owned in
	 * the same document and must be accessed through Run-specific ports later. */
	static bool Build(const FShanmenItemAuthoritySnapshot& Snapshot,
		FShanmenItemLoadoutStartRequest& Out, FString& Reason);
	static bool InspectActive(const FShanmenItemAuthoritySnapshot& Snapshot,
		FShanmenDemo20ActiveLoadout& Out, FString& Reason);
	static FString Summary(const FShanmenItemAuthoritySnapshot& Snapshot);
};
