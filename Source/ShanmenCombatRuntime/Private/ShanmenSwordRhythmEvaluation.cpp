#include "ShanmenSwordRhythmEvaluation.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
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

	bool BindingMatchesRhythm(
		const FShanmenSwordRhythmReceipt& Rhythm,
		const FShanmenSwordRhythmContributionBindingReceipt& Binding)
	{
		if (!Binding.IsValid())
		{
			return !Binding.GetReceiptId().IsValid();
		}
		const FShanmenSwordRhythmObservation& Target =
			Binding.GetTargetObservation();
		const FShanmenSwordRhythmObservation& Current =
			Rhythm.GetCurrentObservation();
		return ObservationsMatch(Target, Current)
			&& Binding.GetScope().GetRunId()
				== Current.GetAction().GetRunId()
			&& Binding.GetScope().GetOwnerId()
				== Current.GetAction().GetOwnerId()
			&& Binding.GetScope().GetTimelineId()
				== Current.GetTimelineId();
	}

	FGuid MakeInputId(
		const FShanmenSwordRhythmReceipt& Rhythm,
		const FShanmenSwordRhythmContributionBindingReceipt& Binding)
	{
		if (!Rhythm.IsValid() || !BindingMatchesRhythm(Rhythm, Binding))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.EvaluationInput.r1"),
			{
				GuidDigits(Rhythm.GetReceiptId()),
				Binding.IsValid()
					? GuidDigits(Binding.GetReceiptId())
					: TEXT("NO_CONTRIBUTION_BINDING")
			});
	}
}

bool FShanmenSwordRhythmEvaluationInput::TryCapture(
	const FShanmenSwordRhythmReceipt& InRhythmReceipt,
	const FShanmenSwordRhythmContributionBindingReceipt&
		InContributionBinding,
	FShanmenSwordRhythmEvaluationInput& OutInput)
{
	OutInput = FShanmenSwordRhythmEvaluationInput();
	FShanmenSwordRhythmEvaluationInput Candidate;
	Candidate.RhythmReceipt = InRhythmReceipt;
	Candidate.ContributionBinding = InContributionBinding;
	Candidate.InputId = MakeInputId(
		Candidate.RhythmReceipt,
		Candidate.ContributionBinding);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutInput = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmEvaluationInput::IsValid() const
{
	return InputId.IsValid()
		&& RhythmReceipt.IsValid()
		&& BindingMatchesRhythm(RhythmReceipt, ContributionBinding)
		&& InputId == MakeInputId(RhythmReceipt, ContributionBinding);
}
