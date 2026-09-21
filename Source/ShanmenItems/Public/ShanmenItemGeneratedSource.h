#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

/** Resolved facts only. Generator policy, catalog lookup and RNG stay outside this module. */
struct SHANMENITEMS_API FShanmenItemGeneratedSourceEntry
{
	FShanmenItemDefinition Definition;
	int32 Quantity = 0;
	FName SectionId = NAME_None;
	int32 SlotIndex = INDEX_NONE;
	int64 UnitValue = 0;
	int64 TotalValue = 0;
	FShanmenItemRewardMetadata RewardMetadata;
	FName ChildContainerType = NAME_None;
	int32 ChildContainerCapacity = 0;

	bool IsValid() const;
	bool operator==(const FShanmenItemGeneratedSourceEntry& Other) const;
};

/**
 * Complete ordered resolved plan, not a seed-only regeneration instruction.
 * Content is the source manifest, not necessarily the authority's item catalog stamp.
 * ExpectedSequence/PityStateBefore are a per-Run compare-and-swap cursor. For a
 * non-pity source the adapter must carry the current pity through unchanged.
 */
struct SHANMENITEMS_API FShanmenItemGeneratedSourcePlan
{
	static constexpr int32 MaxEntries = 1024;
	static constexpr int32 MaxSlots = 4096;
	static constexpr int32 MaxTagsPerDefinition = 64;
	static constexpr int32 MaxDigestLength = 1024;

	FGuid OwnerId;
	FGuid RunId;
	FName SourceRoleId = NAME_None;
	FShanmenContentStamp Content;
	FName SlotId = NAME_None;
	FName ProjectionId = NAME_None;
	FName DistributionProfileId = NAME_None;
	FName BudgetProfileId = NAME_None;
	FName MarkerId = NAME_None;
	FName EncounterId = NAME_None;
	FName JackpotPolicyId = NAME_None;
	FName RareExtremePolicyId = NAME_None;
	FName AffixPolicyId = NAME_None;
	uint64 EffectiveSeed = 0;
	int64 RandomizedBudget = 0;
	int64 GeneratedTotalValue = 0;
	int64 ResidualValue = 0;
	int64 ExpectedSequence = 0;
	int32 PityStateBefore = 0;
	int32 PityStateAfter = 0;
	bool bPityCommitRequired = false;
	bool bFallbackUsed = false;
	bool bLegacyCompatibilityView = false;
	TArray<FShanmenItemGeneratedSourceEntry> Entries;

	bool IsValid() const;
	bool operator==(const FShanmenItemGeneratedSourcePlan& Other) const;
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
