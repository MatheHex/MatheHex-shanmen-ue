#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationScatterResourcePreparation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationScatterResourceCommitStatus : uint8
{
	Invalid,
	Committed,
	Replayed,
	AuthorityNotReady,
	PlanInvalid,
	PreparationRejected,
	AttemptCancelled,
	SnapshotUnavailable,
	EvidenceInvalid,
	CommitRetryRequired,
	ForwardRecoveryRequired
};

/** One exact P27.18 allocation slice backed by one committed stack receipt. */
class Fdemo_mapShanmenFormationScatterResourceFulfillmentLine
{
public:
	bool IsValid() const;
	const FGuid& GetLineId() const { return LineId; }
	const FGuid& GetPlanId() const { return PlanId; }
	const FGuid& GetSliceId() const { return SliceId; }
	int32 GetSliceOrder() const { return SliceOrder; }
	const FGuid& GetAnchorIntentId() const { return AnchorIntentId; }
	const FGuid& GetMaterialIntentId() const { return MaterialIntentId; }
	int32 GetAnchorOrder() const { return AnchorOrder; }
	int32 GetRequirementOrder() const { return RequirementOrder; }
	FName GetMaterialDefinitionId() const { return MaterialDefinitionId; }
	const FGuid& GetReservationIntentId() const
	{
		return ReservationIntentId;
	}
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	int32 GetQuantity() const { return Quantity; }
	const FGuid& GetCommitReceiptId() const { return CommitReceiptId; }

	bool operator==(
		const Fdemo_mapShanmenFormationScatterResourceFulfillmentLine&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterResourceCommitter;

	static FGuid BuildLineId(
		const Fdemo_mapShanmenFormationScatterResourceFulfillmentLine& Line);

	FGuid LineId;
	FGuid PlanId;
	FGuid SliceId;
	int32 SliceOrder = INDEX_NONE;
	FGuid AnchorIntentId;
	FGuid MaterialIntentId;
	int32 AnchorOrder = INDEX_NONE;
	int32 RequirementOrder = INDEX_NONE;
	FName MaterialDefinitionId = NAME_None;
	FGuid ReservationIntentId;
	FGuid ItemInstanceId;
	int32 Quantity = 0;
	FGuid CommitReceiptId;
};

/** Complete committed material attribution for one exact scatter anchor. */
class Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment
{
public:
	bool IsValid() const;
	const FGuid& GetFulfillmentId() const { return FulfillmentId; }
	const FGuid& GetPlanId() const { return PlanId; }
	const FGuid& GetBatchIntentId() const { return BatchIntentId; }
	const FGuid& GetAnchorIntentId() const { return AnchorIntentId; }
	int32 GetAnchorOrder() const { return AnchorOrder; }
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetAnchorInstanceId() const { return AnchorInstanceId; }
	const TArray<
		Fdemo_mapShanmenFormationScatterResourceFulfillmentLine>&
	GetLines() const
	{
		return Lines;
	}
	int32 GetRequirementCount() const { return RequirementCount; }
	int64 GetTotalCommittedQuantity() const
	{
		return TotalCommittedQuantity;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterResourceCommitter;

	static FGuid BuildFulfillmentId(
		const Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment&
			Fulfillment);

	FGuid FulfillmentId;
	FGuid PlanId;
	FGuid BatchIntentId;
	FGuid AnchorIntentId;
	int32 AnchorOrder = INDEX_NONE;
	FName AnchorDefinitionId = NAME_None;
	FGuid AnchorInstanceId;
	TArray<Fdemo_mapShanmenFormationScatterResourceFulfillmentLine> Lines;
	int32 RequirementCount = 0;
	int64 TotalCommittedQuantity = 0;
};

/**
 * Self-validating proof that every stack reservation in one immutable scatter
 * plan reached Committed and that every source slice is attributed once to
 * its authored anchor and material requirement.
 */
class Fdemo_mapShanmenFormationScatterResourceCommitEvidence
{
public:
	bool IsValid() const;
	const FGuid& GetEvidenceId() const { return EvidenceId; }
	const Fdemo_mapShanmenFormationScatterResourcePlan& GetPlan() const
	{
		return Plan;
	}
	const TArray<FShanmenItemTransactionReceipt>& GetCommitReceipts() const
	{
		return CommitReceipts;
	}
	const TArray<
		Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment>&
	GetAnchorFulfillments() const
	{
		return AnchorFulfillments;
	}
	int32 GetRequirementCount() const { return RequirementCount; }
	int64 GetTotalCommittedQuantity() const
	{
		return TotalCommittedQuantity;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterResourceCommitter;

	static FGuid BuildEvidenceId(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			Evidence);

	FGuid EvidenceId;
	Fdemo_mapShanmenFormationScatterResourcePlan Plan;
	TArray<FShanmenItemTransactionReceipt> CommitReceipts;
	TArray<Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment>
		AnchorFulfillments;
	int32 RequirementCount = 0;
	int64 TotalCommittedQuantity = 0;
};

/** Complete evidence from one bounded forward-only commit pass. */
struct Fdemo_mapShanmenFormationScatterResourceCommitResult
{
	Edemo_mapShanmenFormationScatterResourceCommitStatus Status =
		Edemo_mapShanmenFormationScatterResourceCommitStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterResourcePlan Plan;
	Fdemo_mapShanmenFormationScatterResourcePreparationResult Preparation;
	TArray<FShanmenItemDurableCommandResult> CommitCommands;
	TArray<FShanmenItemTransactionReceipt> PrepareReceipts;
	TArray<FShanmenItemTransactionReceipt> CommitReceipts;
	Fdemo_mapShanmenFormationScatterResourceCommitEvidence Evidence;

	bool IsValid() const;
	bool IsCommitted() const;
	bool RequiresRecovery() const;
};

/**
 * Irreversible resource terminal seam for one P27.18 scatter plan.
 *
 * It first delegates to P27.19 and refuses to issue a commit until the whole
 * preparation set is durably present. Commits then advance in canonical plan
 * order. Once any line is committed, every later pass is forward-only. Each
 * pass uses an authority-revision-scoped request identity, so a persisted
 * rejected attempt cannot poison a later retry while the item authority still
 * prevents a successful terminal decision from being reversed.
 */
struct Fdemo_mapShanmenFormationScatterResourceCommitter
{
	static Fdemo_mapShanmenFormationScatterResourceCommitResult Commit(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);

	static Fdemo_mapShanmenFormationScatterResourceCommitResult
	CommitWithAuthority(
		Idemo_mapShanmenFormationScatterResourceAuthority& Authority,
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);

	static bool BuildCommitRequest(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		int32 ReservationIndex,
		const FGuid& CommitPassId,
		FShanmenItemRunQuantityIntentFinalizeRequest& OutRequest);

private:
	static bool BuildEvidence(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		const TArray<FShanmenItemTransactionReceipt>& CommitReceipts,
		Fdemo_mapShanmenFormationScatterResourceCommitEvidence& OutEvidence);
};
