#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationCoverageTransitionReducer.h"

enum class Edemo_mapShanmenFormationCoverageTrackerStatus : uint8
{
	Primed,
	PrimeReplayed,
	Rebased,
	Advanced,
	AdvanceReplayed,
	Cleared,
	CoverageInvalid,
	ExpectedBaselineInvalid,
	NotPrimed,
	AlreadyPrimed,
	BaselineConflict,
	TransitionRejected
};

struct Fdemo_mapShanmenFormationCoverageTrackerResult
{
	Edemo_mapShanmenFormationCoverageTrackerStatus Status =
		Edemo_mapShanmenFormationCoverageTrackerStatus::NotPrimed;
	FString Diagnostic;
	FGuid PreviousBaselineReceiptId;
	FGuid CurrentBaselineReceiptId;
	Fdemo_mapShanmenFormationCoverageTransitionResult Transition;

	bool IsSuccess() const;
};

/**
 * Explicitly driven, non-UObject owner for one immutable coverage baseline.
 *
 * Prime establishes the first baseline. Advance is compare-and-swap guarded by
 * the expected baseline receipt identity and retains only the latest successful
 * command for idempotent replay. Area/subject lifecycle changes require Rebase.
 * One caller must serialize commands; baseline reads are returned as value copies.
 */
class Fdemo_mapShanmenFormationCoverageTracker
{
public:
	Fdemo_mapShanmenFormationCoverageTrackerResult Prime(
		const Fdemo_mapShanmenFormationCoverageReceipt& Coverage);
	Fdemo_mapShanmenFormationCoverageTrackerResult Rebase(
		const Fdemo_mapShanmenFormationCoverageReceipt& Coverage);
	Fdemo_mapShanmenFormationCoverageTrackerResult Advance(
		const FGuid& ExpectedBaselineReceiptId,
		const Fdemo_mapShanmenFormationCoverageReceipt& CurrentCoverage);
	Fdemo_mapShanmenFormationCoverageTrackerResult Reset();

	bool IsPrimed() const { return bPrimed; }
	bool IsConsistent() const;
	bool TryGetBaseline(
		Fdemo_mapShanmenFormationCoverageReceipt& OutBaseline) const;

private:
	void ClearReplay();

	bool bPrimed = false;
	Fdemo_mapShanmenFormationCoverageReceipt Baseline;
	bool bHasReplay = false;
	FGuid LastExpectedBaselineReceiptId;
	FGuid LastCurrentCoverageReceiptId;
	Fdemo_mapShanmenFormationCoverageTransitionResult LastTransition;
};
