#pragma once

#include "CoreMinimal.h"
#include "ShanmenSwordRhythmContribution.h"

#include "ShanmenSwordRhythmContributionBinding.generated.h"

/** Immutable Run/Owner/timeline boundary for one pending-contribution ledger. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmContributionBindingScope
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FGuid& RunId,
		const FGuid& OwnerId,
		const FGuid& TimelineId,
		FShanmenSwordRhythmContributionBindingScope& OutScope);

	bool IsValid() const;
	const FGuid& GetScopeId() const { return ScopeId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	const FGuid& GetTimelineId() const { return TimelineId; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid ScopeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid OwnerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;
};

/**
 * Replay-stable proof that pending facts were bound once to one later
 * BasicSword activation. It deliberately contains no strength or damage value.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmContributionBindingReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSwordRhythmContributionBindingScope& GetScope() const
	{
		return Scope;
	}
	const FShanmenSwordRhythmObservation& GetTargetObservation() const
	{
		return TargetObservation;
	}
	const TArray<FShanmenSwordRhythmContribution>& GetContributions() const
	{
		return Contributions;
	}
	int32 NumContributions() const { return Contributions.Num(); }

private:
	friend class FShanmenSwordRhythmContributionBindingLedger;

	static bool TryCreate(
		const FShanmenSwordRhythmContributionBindingScope& Scope,
		const FShanmenSwordRhythmObservation& TargetObservation,
		TArray<FShanmenSwordRhythmContribution> Contributions,
		FShanmenSwordRhythmContributionBindingReceipt& OutReceipt);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmContributionBindingScope Scope;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmObservation TargetObservation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	TArray<FShanmenSwordRhythmContribution> Contributions;
};

/**
 * Pure, caller-clocked next-BasicSword binding authority.
 *
 * Every BasicSword observation is retained, including observations made while
 * no contribution is pending. Therefore a late contribution cannot be applied
 * retroactively. Exact replays are idempotent; conflicting activation reuse,
 * stale delivery and cross-scope evidence fail closed. A contribution produced
 * by a precise link cannot bind to the same activation that produced it.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmContributionBindingLedger
{
public:
	static bool TryCreate(
		const FShanmenSwordRhythmContributionBindingScope& Scope,
		FShanmenSwordRhythmContributionBindingLedger& OutLedger);

	bool IsValid() const;
	bool TryRecordContribution(
		const FShanmenSwordRhythmContribution& Contribution);
	/**
	 * Returns true for a newly accepted or exact-replayed observation.
	 * OutReceipt is valid only when this observation binds pending evidence.
	 */
	bool TryObserveBasicSword(
		const FShanmenSwordRhythmObservation& Observation,
		FShanmenSwordRhythmContributionBindingReceipt& OutReceipt);
	void Reset();

	const FShanmenSwordRhythmContributionBindingScope& GetScope() const
	{
		return Scope;
	}
	const FShanmenSwordRhythmObservation& GetLastObservation() const
	{
		return LastObservation;
	}
	bool ContainsPending(const FGuid& ContributionId) const;
	bool IsBound(const FGuid& ContributionId) const;
	int32 NumPending() const { return PendingById.Num(); }
	int32 NumBoundContributions() const { return BoundContributionIds.Num(); }
	int32 NumObservedActions() const { return ObservationsByActivation.Num(); }
	int32 NumBindingReceipts() const { return BindingsByActivation.Num(); }

private:
	FShanmenSwordRhythmContributionBindingScope Scope;
	FShanmenSwordRhythmObservation LastObservation;
	TMap<FGuid, FShanmenSwordRhythmContribution> PendingById;
	TSet<FGuid> BoundContributionIds;
	TMap<FGuid, FShanmenSwordRhythmObservation> ObservationsByActivation;
	TMap<FGuid, FShanmenSwordRhythmContributionBindingReceipt>
		BindingsByActivation;
};
