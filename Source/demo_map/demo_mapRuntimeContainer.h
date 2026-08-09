#pragma once

#include "CoreMinimal.h"
#include "demo_mapSearchContainerTypes.h"

class Fdemo_mapItemAuthority;

struct Fdemo_mapRuntimeContainerEntryRecord
{
	FGuid EntryId;
	Edemo_mapRuntimeContainerSection Section = Edemo_mapRuntimeContainerSection::Chest;
	int32 SlotIndex = INDEX_NONE;
	Edemo_mapRuntimeContainerEntryState State = Edemo_mapRuntimeContainerEntryState::Hidden;
	FGuid InternalItemInstanceId;
	FName ExpectedDefinitionId = NAME_None;
	int32 ExpectedStackCount = 0;
	float SearchDurationSeconds = 0.0f;
};

/** Value-only rollback/commit state for the active Runtime Container. */
struct Fdemo_mapRuntimeContainerAuthorityState
{
	FGuid ContainerId;
	FGuid OwningRunId;
	Edemo_mapRuntimeContainerKind Kind = Edemo_mapRuntimeContainerKind::Chest;
	Edemo_mapRuntimeContainerState State =
		Edemo_mapRuntimeContainerState::Closed;
	Edemo_mapRuntimeContainerActionKind ActiveAction =
		Edemo_mapRuntimeContainerActionKind::None;
	FGuid ActiveEntryId;
	int32 Revision = 0;
	TArray<Fdemo_mapRuntimeContainerEntryRecord> Entries;
	bool bPlayerDepositAllowed = true;
	bool bInitialized = false;
};

/**
 * Single value authority for one Runtime Container. It stores no ItemInstance
 * record; atomic item mutations are supplied by the sole Item Authority.
 */
class Fdemo_mapRuntimeContainerAuthority
{
public:
	bool Initialize(
		FGuid InContainerId,
		FGuid InOwningRunId,
		Edemo_mapRuntimeContainerKind InKind,
		const TArray<Fdemo_mapRuntimeContainerResolvedSeedEntry>& ResolvedEntries,
		FString& OutDiagnostic,
		bool bInPlayerDepositAllowed = true);

	Fdemo_mapRuntimeContainerResult SubmitIntent(
		const Fdemo_mapRuntimeContainerIntent& Intent,
		bool bPlayerAlive,
		bool bInRange,
		TFunctionRef<Fdemo_mapItemOperationResult(FGuid ItemInstanceId)> TransferWholeItem);
	Fdemo_mapRuntimeContainerResult CompleteActiveAction();
	Fdemo_mapRuntimeContainerResult CancelActiveAction(const FString& Diagnostic);
	Fdemo_mapRuntimeContainerSnapshot BuildSnapshot(
		const Fdemo_mapItemAuthority& ItemAuthority,
		float ActionProgress01,
		int32 InventoryUsedSlots,
		int32 InventoryCapacity,
		const FString& Diagnostic = FString()) const;
	int32 CleanupUnclaimed(
		TFunctionRef<Fdemo_mapItemOperationResult(FGuid ItemInstanceId)> DestroyContainerItem);

	bool IsInitialized() const { return bInitialized; }
	bool IsActionActive() const { return ActiveAction != Edemo_mapRuntimeContainerActionKind::None; }
	bool IsOpening() const { return ActiveAction == Edemo_mapRuntimeContainerActionKind::BeginOpen; }
	bool IsSearching() const { return ActiveAction == Edemo_mapRuntimeContainerActionKind::BeginSearch; }
	bool IsEmpty() const;
	float GetActiveActionDuration() const;
	FGuid GetActiveEntryId() const { return ActiveEntryId; }
	FGuid GetContainerId() const { return ContainerId; }
	FGuid GetOwningRunId() const { return OwningRunId; }
	int32 GetRevision() const { return Revision; }
	Edemo_mapRuntimeContainerKind GetKind() const { return Kind; }
	Edemo_mapRuntimeContainerState GetState() const { return State; }
	Edemo_mapRuntimeContainerActionKind GetActiveAction() const { return ActiveAction; }
	bool AllowsPlayerDeposit() const { return bPlayerDepositAllowed; }
	const TArray<Fdemo_mapRuntimeContainerEntryRecord>& GetEntriesForAudit() const { return Entries; }
	const Fdemo_mapRuntimeContainerEntryRecord* FindEntry(FGuid EntryId) const;
	Fdemo_mapRuntimeContainerAuthorityState CaptureState() const;
	/**
	 * Commits a prevalidated P3 entry projection.  Item ownership is owned by
	 * ItemAuthority; this method writes only the corresponding Container slots.
	 */
	Fdemo_mapRuntimeContainerResult CommitPlayerDropState(
		const Fdemo_mapRuntimeContainerAuthorityState& InState,
		int32 ExpectedRevision,
		FGuid RelatedEntryId = FGuid(),
		FGuid RelatedItemId = FGuid());

private:
	Fdemo_mapRuntimeContainerResult ValidateIntent(
		const Fdemo_mapRuntimeContainerIntent& Intent,
		bool bPlayerAlive,
		bool bInRange) const;
	Fdemo_mapRuntimeContainerResult BeginOpen();
	Fdemo_mapRuntimeContainerResult BeginSearch(FGuid EntryId);
	Fdemo_mapRuntimeContainerResult Take(
		FGuid EntryId,
		TFunctionRef<Fdemo_mapItemOperationResult(FGuid ItemInstanceId)> TransferWholeItem);
	Fdemo_mapRuntimeContainerEntryRecord* FindMutableEntry(FGuid EntryId);

	FGuid ContainerId;
	FGuid OwningRunId;
	Edemo_mapRuntimeContainerKind Kind = Edemo_mapRuntimeContainerKind::Chest;
	Edemo_mapRuntimeContainerState State = Edemo_mapRuntimeContainerState::Closed;
	Edemo_mapRuntimeContainerActionKind ActiveAction = Edemo_mapRuntimeContainerActionKind::None;
	FGuid ActiveEntryId;
	int32 Revision = 0;
	TArray<Fdemo_mapRuntimeContainerEntryRecord> Entries;
	bool bPlayerDepositAllowed = true;
	bool bInitialized = false;
};
