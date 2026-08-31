#include "ShanmenSwordRhythmContributionBinding.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeScopeId(
		const FGuid& RunId,
		const FGuid& OwnerId,
		const FGuid& TimelineId)
	{
		if (!RunId.IsValid() || !OwnerId.IsValid() || !TimelineId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.ContributionBinding.Scope.r1"),
			{ GuidDigits(RunId), GuidDigits(OwnerId), GuidDigits(TimelineId) });
	}

	bool ScopeMatches(
		const FShanmenSwordRhythmContributionBindingScope& Scope,
		const FShanmenSwordRhythmContribution& Contribution)
	{
		return Scope.IsValid()
			&& Contribution.IsValid()
			&& Contribution.GetAction().GetRunId() == Scope.GetRunId()
			&& Contribution.GetAction().GetOwnerId() == Scope.GetOwnerId()
			&& Contribution.GetTimelineId() == Scope.GetTimelineId();
	}

	bool ScopeMatches(
		const FShanmenSwordRhythmContributionBindingScope& Scope,
		const FShanmenSwordRhythmObservation& Observation)
	{
		return Scope.IsValid()
			&& Observation.IsValid()
			&& Observation.GetAction().GetRunId() == Scope.GetRunId()
			&& Observation.GetAction().GetOwnerId() == Scope.GetOwnerId()
			&& Observation.GetTimelineId() == Scope.GetTimelineId();
	}

	bool ObservationsMatch(
		const FShanmenSwordRhythmObservation& Left,
		const FShanmenSwordRhythmObservation& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetObservationId() == Right.GetObservationId()
			&& Left.GetAction().GetActivationId()
				== Right.GetAction().GetActivationId()
			&& Left.GetTimelineId() == Right.GetTimelineId()
			&& Left.GetInputTick() == Right.GetInputTick();
	}

	bool ContributionsMatch(
		const FShanmenSwordRhythmContribution& Left,
		const FShanmenSwordRhythmContribution& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetContributionId() == Right.GetContributionId()
			&& Left.GetKind() == Right.GetKind()
			&& Left.GetAction().GetActivationId()
				== Right.GetAction().GetActivationId()
			&& Left.GetSourceReceiptId() == Right.GetSourceReceiptId()
			&& Left.GetTimelineId() == Right.GetTimelineId()
			&& Left.GetObservedTick() == Right.GetObservedTick();
	}

	bool ContributionComesBefore(
		const FShanmenSwordRhythmContribution& Left,
		const FShanmenSwordRhythmContribution& Right)
	{
		if (Left.GetObservedTick() != Right.GetObservedTick())
		{
			return Left.GetObservedTick() < Right.GetObservedTick();
		}
		return GuidDigits(Left.GetContributionId())
			< GuidDigits(Right.GetContributionId());
	}

	FGuid MakeReceiptId(
		const FShanmenSwordRhythmContributionBindingScope& Scope,
		const FShanmenSwordRhythmObservation& Target,
		const TArray<FShanmenSwordRhythmContribution>& Contributions)
	{
		if (!ScopeMatches(Scope, Target) || Contributions.IsEmpty())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Scope.GetScopeId()),
			GuidDigits(Target.GetObservationId()),
			FString::FromInt(Contributions.Num())
		};
		for (const FShanmenSwordRhythmContribution& Contribution
			: Contributions)
		{
			Parts.Add(GuidDigits(Contribution.GetContributionId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.ContributionBinding.Receipt.r1"),
			Parts);
	}
}

bool FShanmenSwordRhythmContributionBindingScope::TryCapture(
	const FGuid& InRunId,
	const FGuid& InOwnerId,
	const FGuid& InTimelineId,
	FShanmenSwordRhythmContributionBindingScope& OutScope)
{
	OutScope = FShanmenSwordRhythmContributionBindingScope();
	FShanmenSwordRhythmContributionBindingScope Candidate;
	Candidate.RunId = InRunId;
	Candidate.OwnerId = InOwnerId;
	Candidate.TimelineId = InTimelineId;
	Candidate.ScopeId = MakeScopeId(
		Candidate.RunId, Candidate.OwnerId, Candidate.TimelineId);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutScope = Candidate;
	return true;
}

bool FShanmenSwordRhythmContributionBindingScope::IsValid() const
{
	return ScopeId.IsValid()
		&& RunId.IsValid()
		&& OwnerId.IsValid()
		&& TimelineId.IsValid()
		&& ScopeId == MakeScopeId(RunId, OwnerId, TimelineId);
}

bool FShanmenSwordRhythmContributionBindingReceipt::TryCreate(
	const FShanmenSwordRhythmContributionBindingScope& InScope,
	const FShanmenSwordRhythmObservation& InTargetObservation,
	TArray<FShanmenSwordRhythmContribution> InContributions,
	FShanmenSwordRhythmContributionBindingReceipt& OutReceipt)
{
	OutReceipt = FShanmenSwordRhythmContributionBindingReceipt();
	InContributions.Sort(ContributionComesBefore);
	FShanmenSwordRhythmContributionBindingReceipt Candidate;
	Candidate.Scope = InScope;
	Candidate.TargetObservation = InTargetObservation;
	Candidate.Contributions = MoveTemp(InContributions);
	Candidate.ReceiptId = MakeReceiptId(
		Candidate.Scope,
		Candidate.TargetObservation,
		Candidate.Contributions);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmContributionBindingReceipt::IsValid() const
{
	if (!ReceiptId.IsValid()
		|| !ScopeMatches(Scope, TargetObservation)
		|| Contributions.IsEmpty())
	{
		return false;
	}

	TSet<FGuid> ContributionIds;
	for (int32 Index = 0; Index < Contributions.Num(); ++Index)
	{
		const FShanmenSwordRhythmContribution& Contribution =
			Contributions[Index];
		if (!ScopeMatches(Scope, Contribution)
			|| Contribution.GetObservedTick()
				> TargetObservation.GetInputTick()
			|| Contribution.GetAction().GetActivationId()
				== TargetObservation.GetAction().GetActivationId()
			|| ContributionIds.Contains(Contribution.GetContributionId())
			|| (Index > 0
				&& ContributionComesBefore(
					Contribution, Contributions[Index - 1])))
		{
			return false;
		}
		ContributionIds.Add(Contribution.GetContributionId());
	}

	return ReceiptId == MakeReceiptId(
		Scope, TargetObservation, Contributions);
}

bool FShanmenSwordRhythmContributionBindingLedger::TryCreate(
	const FShanmenSwordRhythmContributionBindingScope& InScope,
	FShanmenSwordRhythmContributionBindingLedger& OutLedger)
{
	OutLedger.Reset();
	if (!InScope.IsValid())
	{
		return false;
	}
	FShanmenSwordRhythmContributionBindingLedger Candidate;
	Candidate.Scope = InScope;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutLedger = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmContributionBindingLedger::IsValid() const
{
	if (!Scope.IsValid())
	{
		return false;
	}

	if (ObservationsByActivation.IsEmpty())
	{
		if (LastObservation.IsValid()
			|| !BindingsByActivation.IsEmpty()
			|| !BoundContributionIds.IsEmpty())
		{
			return false;
		}
	}
	else
	{
		if (!ScopeMatches(Scope, LastObservation))
		{
			return false;
		}
		const FShanmenSwordRhythmObservation* StoredLast =
			ObservationsByActivation.Find(
				LastObservation.GetAction().GetActivationId());
		if (StoredLast == nullptr
			|| !ObservationsMatch(*StoredLast, LastObservation))
		{
			return false;
		}
	}

	for (const TPair<FGuid, FShanmenSwordRhythmObservation>& Pair
		: ObservationsByActivation)
	{
		if (!Pair.Key.IsValid()
			|| !ScopeMatches(Scope, Pair.Value)
			|| Pair.Key != Pair.Value.GetAction().GetActivationId()
			|| (LastObservation.IsValid()
				&& Pair.Value.GetInputTick()
					> LastObservation.GetInputTick()))
		{
			return false;
		}
	}

	TSet<FGuid> ReceiptContributionIds;
	for (const TPair<FGuid, FShanmenSwordRhythmContributionBindingReceipt>& Pair
		: BindingsByActivation)
	{
		const FShanmenSwordRhythmContributionBindingReceipt& Receipt =
			Pair.Value;
		const FShanmenSwordRhythmObservation* StoredObservation =
			ObservationsByActivation.Find(Pair.Key);
		if (!Pair.Key.IsValid()
			|| !Receipt.IsValid()
			|| Receipt.GetScope().GetScopeId() != Scope.GetScopeId()
			|| Pair.Key
				!= Receipt.GetTargetObservation().GetAction().GetActivationId()
			|| StoredObservation == nullptr
			|| !ObservationsMatch(
				*StoredObservation, Receipt.GetTargetObservation()))
		{
			return false;
		}
		for (const FShanmenSwordRhythmContribution& Contribution
			: Receipt.GetContributions())
		{
			if (ReceiptContributionIds.Contains(
					Contribution.GetContributionId())
				|| !BoundContributionIds.Contains(
					Contribution.GetContributionId()))
			{
				return false;
			}
			ReceiptContributionIds.Add(Contribution.GetContributionId());
		}
	}
	if (ReceiptContributionIds.Num() != BoundContributionIds.Num())
	{
		return false;
	}
	for (const FGuid& BoundId : BoundContributionIds)
	{
		if (!BoundId.IsValid() || !ReceiptContributionIds.Contains(BoundId))
		{
			return false;
		}
	}

	for (const TPair<FGuid, FShanmenSwordRhythmContribution>& Pair
		: PendingById)
	{
		const FShanmenSwordRhythmContribution& Contribution = Pair.Value;
		if (!Pair.Key.IsValid()
			|| Pair.Key != Contribution.GetContributionId()
			|| !ScopeMatches(Scope, Contribution)
			|| BoundContributionIds.Contains(Pair.Key))
		{
			return false;
		}
		if (LastObservation.IsValid()
			&& Contribution.GetObservedTick()
				< LastObservation.GetInputTick()
			&& (Contribution.GetAction().GetActivationId()
					!= LastObservation.GetAction().GetActivationId()
				|| Contribution.GetObservedTick()
					!= LastObservation.GetInputTick()))
		{
			return false;
		}
	}
	return true;
}

bool FShanmenSwordRhythmContributionBindingLedger::TryRecordContribution(
	const FShanmenSwordRhythmContribution& Contribution)
{
	if (!IsValid() || !ScopeMatches(Scope, Contribution))
	{
		return false;
	}
	if (const FShanmenSwordRhythmContribution* Existing =
		PendingById.Find(Contribution.GetContributionId()))
	{
		return ContributionsMatch(*Existing, Contribution);
	}
	if (BoundContributionIds.Contains(Contribution.GetContributionId()))
	{
		return true;
	}
	if (LastObservation.IsValid()
			&& Contribution.GetObservedTick()
				< LastObservation.GetInputTick())
	{
		return false;
	}

	FShanmenSwordRhythmContributionBindingLedger Candidate = *this;
	Candidate.PendingById.Add(
		Contribution.GetContributionId(), Contribution);
	if (!Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmContributionBindingLedger::TryObserveBasicSword(
	const FShanmenSwordRhythmObservation& Observation,
	FShanmenSwordRhythmContributionBindingReceipt& OutReceipt)
{
	OutReceipt = FShanmenSwordRhythmContributionBindingReceipt();
	if (!IsValid() || !ScopeMatches(Scope, Observation))
	{
		return false;
	}

	const FGuid ActivationId = Observation.GetAction().GetActivationId();
	if (const FShanmenSwordRhythmObservation* Existing =
		ObservationsByActivation.Find(ActivationId))
	{
		if (!ObservationsMatch(*Existing, Observation))
		{
			return false;
		}
		if (const FShanmenSwordRhythmContributionBindingReceipt* Binding =
			BindingsByActivation.Find(ActivationId))
		{
			OutReceipt = *Binding;
		}
		return true;
	}
	if (LastObservation.IsValid()
		&& Observation.GetInputTick() < LastObservation.GetInputTick())
	{
		return false;
	}

	for (const TPair<FGuid, FShanmenSwordRhythmContribution>& Pair
		: PendingById)
	{
		const FShanmenSwordRhythmContribution& Contribution = Pair.Value;
		if (Contribution.GetAction().GetActivationId() == ActivationId
			&& Contribution.GetObservedTick() != Observation.GetInputTick())
		{
			return false;
		}
	}

	FShanmenSwordRhythmContributionBindingLedger Candidate = *this;
	Candidate.ObservationsByActivation.Add(ActivationId, Observation);
	Candidate.LastObservation = Observation;

	TArray<FShanmenSwordRhythmContribution> Eligible;
	for (const TPair<FGuid, FShanmenSwordRhythmContribution>& Pair
		: Candidate.PendingById)
	{
		const FShanmenSwordRhythmContribution& Contribution = Pair.Value;
		if (Contribution.GetObservedTick() <= Observation.GetInputTick()
			&& Contribution.GetAction().GetActivationId() != ActivationId)
		{
			Eligible.Add(Contribution);
		}
	}

	FShanmenSwordRhythmContributionBindingReceipt CandidateReceipt;
	if (!Eligible.IsEmpty())
	{
		if (!FShanmenSwordRhythmContributionBindingReceipt::TryCreate(
				Candidate.Scope,
				Observation,
				Eligible,
				CandidateReceipt))
		{
			return false;
		}
		for (const FShanmenSwordRhythmContribution& Contribution
			: CandidateReceipt.GetContributions())
		{
			Candidate.PendingById.Remove(Contribution.GetContributionId());
			Candidate.BoundContributionIds.Add(
				Contribution.GetContributionId());
		}
		Candidate.BindingsByActivation.Add(
			ActivationId, CandidateReceipt);
	}

	if (!Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	OutReceipt = CandidateReceipt;
	return true;
}

void FShanmenSwordRhythmContributionBindingLedger::Reset()
{
	*this = FShanmenSwordRhythmContributionBindingLedger();
}

bool FShanmenSwordRhythmContributionBindingLedger::ContainsPending(
	const FGuid& ContributionId) const
{
	return ContributionId.IsValid() && PendingById.Contains(ContributionId);
}

bool FShanmenSwordRhythmContributionBindingLedger::IsBound(
	const FGuid& ContributionId) const
{
	return ContributionId.IsValid()
		&& BoundContributionIds.Contains(ContributionId);
}
