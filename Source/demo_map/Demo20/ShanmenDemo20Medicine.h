#pragma once
#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenItemAuthorityService.h"

/** Read-only medicine access reconstructed from the sole item authority. */
struct FShanmenDemo20MedicineLine
{
	FGuid ItemId;
	int32 Quantity=0;
	int64 ItemRevision=0;
	EShanmenDemo20MedicineOrigin Origin=EShanmenDemo20MedicineOrigin::PreparedCarry;
};

/** Synchronous bindings to the existing GameInstance owner (or native service in tests).
 * No inventory implementation lives here. */
struct FShanmenDemo20MedicinePorts
{
	TFunction<bool(FShanmenItemAuthoritySnapshot&)> Capture;
	TFunction<FShanmenItemDurableCommandResult(const FShanmenItemRunQuantityIntentRequest&)> PrepareRun;
	TFunction<FShanmenItemDurableCommandResult(const FShanmenItemRunQuantityIntentFinalizeRequest&)> FinalizeRun;
	TFunction<FShanmenItemDurableCommandResult(const FShanmenItemReserveRequest&)> Reserve;
	TFunction<FShanmenItemDurableCommandResult(const FShanmenItemReservationActionRequest&)> Commit;
};

struct FShanmenDemo20Medicine
{
	static bool Capture(const FShanmenItemAuthoritySnapshot& Items,const FGuid& Run,
		TArray<FShanmenDemo20MedicineLine>& Out,FString& Reason);
	static bool BuildIntent(const FShanmenDemo20WorldCheckpoint& Current,
		const FShanmenItemAuthoritySnapshot& Items,FShanmenDemo20WorldCheckpoint& Out,FString& Reason);
	/** Requires an already durable world intent. Commits exactly one item then
	 * confirms the healed checkpoint. Failures leave that same intent for recovery. */
	static bool Recover(const FString& Root,FShanmenDemo20WorldCheckpoint& Confirmed,
		const FShanmenDemo20MedicinePorts& Ports,FString& Reason);
	static FGuid IntentId(const FShanmenDemo20WorldCheckpoint& Intent);
};
