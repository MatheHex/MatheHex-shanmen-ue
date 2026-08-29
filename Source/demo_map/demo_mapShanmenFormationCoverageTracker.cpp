#include "demo_mapShanmenFormationCoverageTracker.h"

namespace
{
	Fdemo_mapShanmenFormationCoverageTrackerResult MakeResult(
		const Edemo_mapShanmenFormationCoverageTrackerStatus Status,
		const TCHAR* Diagnostic,
		const FGuid& PreviousBaselineReceiptId = FGuid(),
		const FGuid& CurrentBaselineReceiptId = FGuid(),
		const Fdemo_mapShanmenFormationCoverageTransitionResult& Transition =
			Fdemo_mapShanmenFormationCoverageTransitionResult())
	{
		Fdemo_mapShanmenFormationCoverageTrackerResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.PreviousBaselineReceiptId = PreviousBaselineReceiptId;
		Result.CurrentBaselineReceiptId = CurrentBaselineReceiptId;
		Result.Transition = Transition;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationCoverageTrackerResult::IsSuccess() const
{
	switch (Status)
	{
	case Edemo_mapShanmenFormationCoverageTrackerStatus::Primed:
		return !PreviousBaselineReceiptId.IsValid()
			&& CurrentBaselineReceiptId.IsValid()
			&& !Transition.IsSuccess();
	case Edemo_mapShanmenFormationCoverageTrackerStatus::PrimeReplayed:
		return PreviousBaselineReceiptId.IsValid()
			&& PreviousBaselineReceiptId == CurrentBaselineReceiptId
			&& !Transition.IsSuccess();
	case Edemo_mapShanmenFormationCoverageTrackerStatus::Rebased:
		return PreviousBaselineReceiptId.IsValid()
			&& CurrentBaselineReceiptId.IsValid()
			&& !Transition.IsSuccess();
	case Edemo_mapShanmenFormationCoverageTrackerStatus::Advanced:
	case Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed:
		return PreviousBaselineReceiptId.IsValid()
			&& CurrentBaselineReceiptId.IsValid()
			&& Transition.IsSuccess()
			&& Transition.Receipt.PreviousCoverage.ReceiptId
				== PreviousBaselineReceiptId
			&& Transition.Receipt.CurrentCoverage.ReceiptId
				== CurrentBaselineReceiptId;
	case Edemo_mapShanmenFormationCoverageTrackerStatus::Cleared:
		return !CurrentBaselineReceiptId.IsValid()
			&& !Transition.IsSuccess();
	default:
		return false;
	}
}

void Fdemo_mapShanmenFormationCoverageTracker::ClearReplay()
{
	bHasReplay = false;
	LastExpectedBaselineReceiptId.Invalidate();
	LastCurrentCoverageReceiptId.Invalidate();
	LastTransition = Fdemo_mapShanmenFormationCoverageTransitionResult();
}

bool Fdemo_mapShanmenFormationCoverageTracker::IsConsistent() const
{
	if (!bPrimed)
	{
		return !Baseline.IsValid() && !bHasReplay
			&& !LastExpectedBaselineReceiptId.IsValid()
			&& !LastCurrentCoverageReceiptId.IsValid()
			&& !LastTransition.IsSuccess();
	}
	if (!Baseline.IsValid())
	{
		return false;
	}
	if (!bHasReplay)
	{
		return !LastExpectedBaselineReceiptId.IsValid()
			&& !LastCurrentCoverageReceiptId.IsValid()
			&& !LastTransition.IsSuccess();
	}
	return LastExpectedBaselineReceiptId.IsValid()
		&& LastCurrentCoverageReceiptId.IsValid()
		&& LastTransition.IsSuccess()
		&& LastTransition.Receipt.PreviousCoverage.ReceiptId
			== LastExpectedBaselineReceiptId
		&& LastTransition.Receipt.CurrentCoverage.ReceiptId
			== LastCurrentCoverageReceiptId
		&& Baseline.ReceiptId == LastCurrentCoverageReceiptId;
}

bool Fdemo_mapShanmenFormationCoverageTracker::TryGetBaseline(
	Fdemo_mapShanmenFormationCoverageReceipt& OutBaseline) const
{
	OutBaseline = Fdemo_mapShanmenFormationCoverageReceipt();
	if (!bPrimed || !Baseline.IsValid())
	{
		return false;
	}
	OutBaseline = Baseline;
	return true;
}

Fdemo_mapShanmenFormationCoverageTrackerResult
Fdemo_mapShanmenFormationCoverageTracker::Prime(
	const Fdemo_mapShanmenFormationCoverageReceipt& Coverage)
{
	if (!Coverage.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::CoverageInvalid,
			TEXT("Prime requires one valid immutable coverage receipt."),
			FGuid(), bPrimed ? Baseline.ReceiptId : FGuid());
	}
	if (bPrimed)
	{
		if (Baseline.ReceiptId == Coverage.ReceiptId)
		{
			return MakeResult(
				Edemo_mapShanmenFormationCoverageTrackerStatus::PrimeReplayed,
				TEXT("The exact coverage baseline is already primed."),
				Baseline.ReceiptId, Baseline.ReceiptId);
		}
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::AlreadyPrimed,
			TEXT("A different active baseline requires explicit Rebase."),
			Baseline.ReceiptId, Baseline.ReceiptId);
	}

	Baseline = Coverage;
	bPrimed = true;
	ClearReplay();
	return MakeResult(
		Edemo_mapShanmenFormationCoverageTrackerStatus::Primed,
		TEXT("The first immutable coverage baseline was established."),
		FGuid(), Baseline.ReceiptId);
}

Fdemo_mapShanmenFormationCoverageTrackerResult
Fdemo_mapShanmenFormationCoverageTracker::Rebase(
	const Fdemo_mapShanmenFormationCoverageReceipt& Coverage)
{
	if (!Coverage.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::CoverageInvalid,
			TEXT("Rebase requires one valid immutable coverage receipt."),
			bPrimed ? Baseline.ReceiptId : FGuid(),
			bPrimed ? Baseline.ReceiptId : FGuid());
	}
	if (!bPrimed)
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::NotPrimed,
			TEXT("Prime must establish the first baseline before Rebase."));
	}
	const FGuid PreviousId = Baseline.ReceiptId;
	Baseline = Coverage;
	ClearReplay();
	return MakeResult(
		Edemo_mapShanmenFormationCoverageTrackerStatus::Rebased,
		TEXT("Explicit lifecycle handling replaced the coverage baseline."),
		PreviousId, Baseline.ReceiptId);
}

Fdemo_mapShanmenFormationCoverageTrackerResult
Fdemo_mapShanmenFormationCoverageTracker::Advance(
	const FGuid& ExpectedBaselineReceiptId,
	const Fdemo_mapShanmenFormationCoverageReceipt& CurrentCoverage)
{
	if (!ExpectedBaselineReceiptId.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::ExpectedBaselineInvalid,
			TEXT("Advance requires the caller's expected baseline receipt identity."),
			ExpectedBaselineReceiptId,
			bPrimed ? Baseline.ReceiptId : FGuid());
	}
	if (!CurrentCoverage.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::CoverageInvalid,
			TEXT("Advance requires one valid current coverage receipt."),
			ExpectedBaselineReceiptId,
			bPrimed ? Baseline.ReceiptId : FGuid());
	}
	if (!bPrimed)
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::NotPrimed,
			TEXT("Prime must establish a baseline before Advance."),
			ExpectedBaselineReceiptId);
	}
	if (bHasReplay
		&& LastExpectedBaselineReceiptId == ExpectedBaselineReceiptId
		&& LastCurrentCoverageReceiptId == CurrentCoverage.ReceiptId
		&& Baseline.ReceiptId == CurrentCoverage.ReceiptId)
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed,
			TEXT("The latest successful Advance command replayed its exact receipt."),
			LastExpectedBaselineReceiptId,
			LastCurrentCoverageReceiptId,
			LastTransition);
	}
	if (Baseline.ReceiptId != ExpectedBaselineReceiptId)
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::BaselineConflict,
			TEXT("The expected baseline is stale or belongs to another tracker state."),
			ExpectedBaselineReceiptId, Baseline.ReceiptId);
	}

	const Fdemo_mapShanmenFormationCoverageTransitionResult Reduced =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Baseline, CurrentCoverage);
	if (!Reduced.IsSuccess())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageTrackerStatus::TransitionRejected,
			TEXT("P8.7 rejected the current coverage; baseline remains unchanged."),
			Baseline.ReceiptId, Baseline.ReceiptId, Reduced);
	}

	const FGuid PreviousId = Baseline.ReceiptId;
	Baseline = CurrentCoverage;
	bHasReplay = true;
	LastExpectedBaselineReceiptId = ExpectedBaselineReceiptId;
	LastCurrentCoverageReceiptId = CurrentCoverage.ReceiptId;
	LastTransition = Reduced;
	return MakeResult(
		Edemo_mapShanmenFormationCoverageTrackerStatus::Advanced,
		TEXT("Coverage transition committed and atomically replaced the baseline."),
		PreviousId, Baseline.ReceiptId, LastTransition);
}

Fdemo_mapShanmenFormationCoverageTrackerResult
Fdemo_mapShanmenFormationCoverageTracker::Reset()
{
	const FGuid PreviousId = bPrimed ? Baseline.ReceiptId : FGuid();
	bPrimed = false;
	Baseline = Fdemo_mapShanmenFormationCoverageReceipt();
	ClearReplay();
	return MakeResult(
		Edemo_mapShanmenFormationCoverageTrackerStatus::Cleared,
		TEXT("Coverage baseline and latest replay evidence were cleared."),
		PreviousId);
}
