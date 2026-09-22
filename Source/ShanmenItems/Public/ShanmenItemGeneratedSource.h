#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

/** Catalog identity is the authority's stamp; Plan.Content is the source manifest. */
struct SHANMENITEMS_API FShanmenItemGeneratedSourceRequest
{
	FShanmenContentStamp ItemContent;
	FShanmenItemGeneratedSourcePlan Plan;
};

enum class EShanmenItemGeneratedSourceRunState : uint8
{
	Unavailable,
	Active,
	Finalized
};

/** Borrowed authority facts. Not a mutable ledger or a new Run owner. */
struct SHANMENITEMS_API FShanmenItemGeneratedSourceView
{
	FGuid OwnerId;
	FGuid RunId;
	FShanmenContentStamp SourceContent;
	EShanmenItemGeneratedSourceRunState State = EShanmenItemGeneratedSourceRunState::Unavailable;
	int64 AcceptedSequence = 0;
	int32 PityState = 0;

	bool IsValid() const;
};

/**
 * Read-only value returned by the contract. Existence/IsValid does NOT prove a
 * durable write: only the sole AuthorityService may publish it after persistence.
 * No Blueprint write surface. Rebuilding from stored data must validate the same contract.
 */
class SHANMENITEMS_API FShanmenItemGeneratedSourceReceipt
{
public:
	const FShanmenItemGeneratedSourcePlan& GetPlan() const { return Plan; }
	const FGuid& GetSourceId() const { return SourceId; }
	const FGuid& GetContainerId() const { return ContainerId; }
	const TArray<FGuid>& GetItemIds() const { return ItemIds; }
	int64 GetAcceptedSequence() const { return AcceptedSequence; }
	bool IsValid() const;
	bool operator==(const FShanmenItemGeneratedSourceReceipt& Other) const;

private:
	friend struct FShanmenItemGeneratedSourceContract;
	FShanmenItemGeneratedSourcePlan Plan;
	FGuid SourceId;
	FGuid ContainerId;
	TArray<FGuid> ItemIds;
	int64 AcceptedSequence = 0;
};

enum class EShanmenItemGeneratedSourceReadStatus : uint8
{
	Unavailable,
	Absent,
	Accepted
};

/**
 * One coherent read, not a reservation or a second ledger. Only the Ready service
 * publishes durable facts. Absent is proven absence in a known Run, not permission
 * to generate: RunState must still be Active and a later commit rechecks the cursor.
 * SourceContent is empty until the Run's first source binds it; ItemContent is not.
 * At most the requested source plan is copied, never the entire inventory/history.
 */
struct SHANMENITEMS_API FShanmenItemGeneratedSourceReadResult
{
	EShanmenItemGeneratedSourceReadStatus Status = EShanmenItemGeneratedSourceReadStatus::Unavailable;
	EShanmenItemGeneratedSourceRunState RunState = EShanmenItemGeneratedSourceRunState::Unavailable;
	FGuid OwnerId;
	FGuid RunId;
	FName SourceRoleId;
	FShanmenContentStamp ItemContent;
	FShanmenContentStamp SourceContent;
	int32 AuthorityRevision = INDEX_NONE;
	int64 AcceptedSequence = 0;
	int32 PityState = 0;
	FShanmenItemGeneratedSourceReceipt Receipt;
};

enum class EShanmenItemGeneratedSourceDecision : uint8
{
	Rejected,
	CandidatePrepared,
	ExactReplay
};

enum class EShanmenItemGeneratedSourceError : uint8
{
	None,
	InvalidPlan,
	InvalidView,
	ScopeMismatch,
	ContentMismatch,
	InvalidExistingReceipt,
	SourceConflict,
	RunClosed,
	CursorConflict
};

struct SHANMENITEMS_API FShanmenItemGeneratedSourceEvaluation
{
	EShanmenItemGeneratedSourceDecision Decision = EShanmenItemGeneratedSourceDecision::Rejected;
	EShanmenItemGeneratedSourceError Error = EShanmenItemGeneratedSourceError::None;
	FShanmenItemGeneratedSourceReceipt Receipt;
};

/**
 * Stateless admission calculation; performs no IO, item insertion or cursor mutation.
 * The caller must obtain View and Existing from ONE locked authority snapshot,
 * looking up Existing by OwnerId/RunId/SourceRoleId. Null means proven absent,
 * not "lookup failed". Catalog and cross-ledger collision checks remain that owner's job.
 */
struct SHANMENITEMS_API FShanmenItemGeneratedSourceContract
{
	static FGuid MakeSourceId(const FGuid& OwnerId, const FGuid& RunId, FName SourceRoleId);
	static FShanmenItemGeneratedSourceEvaluation Evaluate(
		const FShanmenItemGeneratedSourceView& View,
		const FShanmenItemGeneratedSourcePlan& Plan,
		const FShanmenItemGeneratedSourceReceipt* Existing = nullptr);
};
