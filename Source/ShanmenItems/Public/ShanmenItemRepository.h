#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

/**
 * The sole mutable item authority for 0.0.10. It owns the item graph,
 * reservations, terminal deployment state, and request idempotency ledger.
 */
class SHANMENITEMS_API FShanmenItemRepository
{
public:
	bool TryLoadSnapshot(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		EShanmenItemTransactionError* OutError = nullptr);

	FShanmenItemTransactionReceipt Reserve(const FShanmenItemReserveRequest& Request);
	FShanmenItemTransactionReceipt Commit(const FShanmenItemReservationActionRequest& Request);
	FShanmenItemTransactionReceipt CommitBatch(const FShanmenItemReservationBatchRequest& Request);
	FShanmenItemTransactionReceipt StartPreparedRun(
		const FShanmenItemRunStartRequest& Request);
	FShanmenItemTransactionReceipt AmendReservationPurpose(
		const FShanmenItemReservationAmendRequest& Request);
	FShanmenItemTransactionReceipt Cancel(const FShanmenItemReservationActionRequest& Request);
	FShanmenItemTransactionReceipt ReleaseDeployment(const FShanmenItemReservationActionRequest& Request);
	FShanmenItemTransactionReceipt ClaimPreparedRun(
		const FShanmenItemRunClaimRequest& Request);
	FShanmenItemTransactionReceipt ConsumePreparedRunItem(
		const FShanmenItemRunConsumeRequest& Request);
	FShanmenItemTransactionReceipt CommitPreparedRunResources(
		const FShanmenItemRunResourceCommitRequest& Request);
	FShanmenItemTransactionReceipt FinalizePreparedRun(
		const FShanmenItemRunFinalizeRequest& Request);

	FShanmenItemAuthoritySnapshot CaptureSnapshot() const;
	bool ValidateInvariants(EShanmenItemTransactionError* OutError = nullptr) const;

	const FShanmenItemDefinition* FindDefinition(FName DefinitionId) const;
	const FShanmenItemContainer* FindContainer(const FGuid& ContainerId) const;
	const FShanmenItemInstance* FindItem(const FGuid& ItemInstanceId) const;
	const FShanmenItemReservationSnapshot* FindReservation(const FGuid& ReservationId) const;

	int32 GetAvailableResource(const FGuid& ItemInstanceId, EShanmenItemResourceKind Kind) const;
	int32 GetAuthorityRevision() const { return State.AuthorityRevision; }
	int32 NumActiveReservations() const;
	bool IsInitialized() const { return bInitialized; }

private:
	struct FState
	{
		int32 AuthorityRevision = 0;
		FShanmenContentStamp Content;
		TMap<FName, FShanmenItemDefinition> Definitions;
		TMap<FGuid, FShanmenItemContainer> Containers;
		TMap<FGuid, FShanmenItemInstance> Items;
		TMap<FGuid, FShanmenItemReservationSnapshot> Reservations;
		TMap<FGuid, FShanmenItemProcessedRequestSnapshot> ProcessedRequests;
	};

	FState State;
	bool bInitialized = false;

	static bool TryBuildState(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FState& OutState,
		EShanmenItemTransactionError* OutError);
	static bool ValidateState(const FState& Candidate, EShanmenItemTransactionError* OutError);

	static FGuid Fingerprint(const FShanmenItemReserveRequest& Request);
	static FGuid Fingerprint(
		EShanmenItemTransactionOperation Operation,
		const FShanmenItemReservationActionRequest& Request);
	static FGuid Fingerprint(const FShanmenItemReservationBatchRequest& Request);
	static FGuid Fingerprint(const FShanmenItemRunStartRequest& Request);
	static FGuid Fingerprint(const FShanmenItemReservationAmendRequest& Request);
	static FGuid Fingerprint(const FShanmenItemRunClaimRequest& Request);
	static FGuid Fingerprint(const FShanmenItemRunConsumeRequest& Request);
	static FGuid Fingerprint(
		const FShanmenItemRunResourceCommitRequest& Request);
	static FGuid Fingerprint(const FShanmenItemRunFinalizeRequest& Request);
	static FGuid MakeReservationId(const FShanmenItemReserveRequest& Request);
	static FGuid MakeActiveRunId(
		const FGuid& OwnerId,
		const FGuid& ScopeId,
		const FGuid& PreparedBatchRequestId);
	static FGuid MakeAcquiredChildContainerId(
		const FGuid& ActiveRunId,
		const FGuid& ItemInstanceId);
	static FGuid MakeReceiptId(
		const FGuid& RequestId,
		const FGuid& Fingerprint,
		EShanmenItemTransactionPhase Phase,
		EShanmenItemTransactionError Error);

	bool TryReplay(
		const FGuid& RequestId,
		const FGuid& RequestFingerprint,
		EShanmenItemTransactionOperation Operation,
		FShanmenItemTransactionReceipt& OutReceipt) const;
	void RecordProcessed(
		FState& Candidate,
		const FGuid& RequestId,
		const FGuid& RequestFingerprint,
		const FShanmenItemTransactionReceipt& Receipt) const;

	FShanmenItemTransactionReceipt MakeRejected(
		EShanmenItemTransactionOperation Operation,
		const FGuid& RequestId,
		const FGuid& RequestFingerprint,
		EShanmenItemTransactionError Error) const;
	FShanmenItemTransactionReceipt ResolveReservation(
		EShanmenItemTransactionOperation Operation,
		const FShanmenItemReservationActionRequest& Request);
	static bool ApplyCommit(
		FState& Candidate,
		const FGuid& ReservationId,
		EShanmenItemTransactionError& OutError);

	static int32 GetResourceTotal(
		const FShanmenItemInstance& Item,
		EShanmenItemResourceKind Kind);
	static int32 GetReservedAmount(
		const FState& Candidate,
		const FGuid& ItemInstanceId,
		EShanmenItemResourceKind Kind,
		const FGuid& ExcludedReservationId = FGuid());
	static int32 GetAvailableResource(
		const FState& Candidate,
		const FGuid& ItemInstanceId,
		EShanmenItemResourceKind Kind);
	static bool IsSameContent(const FShanmenContentStamp& Left, const FShanmenContentStamp& Right);
};
