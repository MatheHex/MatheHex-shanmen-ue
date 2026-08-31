#include "ShanmenSwordRhythmEvaluation.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EContributionKind = EShanmenSwordRhythmContributionKind;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	const TArray<EContributionKind>& KnownContributionKinds()
	{
		static const TArray<EContributionKind> Kinds = {
			EContributionKind::PreciseSwordLink,
			EContributionKind::PerfectWeaponGuard,
			EContributionKind::SpiritEvasion
		};
		return Kinds;
	}

	bool IsKnownContributionKind(EContributionKind Kind)
	{
		return KnownContributionKinds().Contains(Kind);
	}

	bool SpecificationComesBefore(
		const FShanmenSwordRhythmEffectSpecification& Left,
		const FShanmenSwordRhythmEffectSpecification& Right)
	{
		return static_cast<uint8>(Left.GetContributionKind())
			< static_cast<uint8>(Right.GetContributionKind());
	}

	FGuid MakeSpecificationId(
		EContributionKind Kind,
		FName EffectDefinitionId,
		const FShanmenContentStamp& Content)
	{
		if (!IsKnownContributionKind(Kind)
			|| EffectDefinitionId.IsNone()
			|| !Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.EffectSpecification.r1"),
			{
				FString::FromInt(static_cast<uint8>(Kind)),
				EffectDefinitionId.ToString(),
				Content.Version.ToString(),
				Content.Digest
			});
	}

	FGuid MakePolicyId(
		FName PolicyDefinitionId,
		const FShanmenContentStamp& Content,
		const TArray<FShanmenSwordRhythmEffectSpecification>& Specifications)
	{
		if (PolicyDefinitionId.IsNone()
			|| !Content.IsValid()
			|| Specifications.Num() != KnownContributionKinds().Num())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			PolicyDefinitionId.ToString(),
			Content.Version.ToString(),
			Content.Digest,
			FString::FromInt(Specifications.Num())
		};
		for (const FShanmenSwordRhythmEffectSpecification& Specification
			: Specifications)
		{
			if (!Specification.IsValid()
				|| !ContentMatches(Content, Specification.GetContent()))
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Specification.GetSpecificationId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.EvaluationPolicy.r1"), Parts);
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

	FGuid MakeEffectId(
		const FGuid& EvaluationInputId,
		const FShanmenSwordRhythmEffectSpecification& Specification,
		const FShanmenSwordRhythmContribution& Contribution)
	{
		if (!EvaluationInputId.IsValid()
			|| !Specification.IsValid()
			|| !Contribution.IsValid()
			|| Specification.GetContributionKind() != Contribution.GetKind())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.EvaluatedEffect.r1"),
			{
				GuidDigits(EvaluationInputId),
				GuidDigits(Specification.GetSpecificationId()),
				GuidDigits(Contribution.GetContributionId())
			});
	}

	bool InputContainsContribution(
		const FShanmenSwordRhythmEvaluationInput& Input,
		const FShanmenSwordRhythmContribution& Contribution)
	{
		if (!Input.IsValid()
			|| !Input.HasContributionBinding()
			|| !Contribution.IsValid())
		{
			return false;
		}
		for (const FShanmenSwordRhythmContribution& Bound
			: Input.GetContributionBinding().GetContributions())
		{
			if (Bound.GetContributionId() == Contribution.GetContributionId())
			{
				return true;
			}
		}
		return false;
	}

	bool EffectsMatch(
		const FShanmenSwordRhythmEvaluatedEffect& Left,
		const FShanmenSwordRhythmEvaluatedEffect& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetEffectId() == Right.GetEffectId()
			&& Left.GetEvaluationInputId() == Right.GetEvaluationInputId()
			&& Left.GetSpecification().GetSpecificationId()
				== Right.GetSpecification().GetSpecificationId()
			&& Left.GetContribution().GetContributionId()
				== Right.GetContribution().GetContributionId();
	}

	bool BuildCanonicalEffects(
		const FShanmenSwordRhythmEvaluationInput& Input,
		const FShanmenSwordRhythmEvaluationPolicy& Policy,
		TArray<FShanmenSwordRhythmEvaluatedEffect>& OutEffects)
	{
		OutEffects.Reset();
		if (!Input.IsValid() || !Policy.IsValid())
		{
			return false;
		}
		if (!Input.HasContributionBinding())
		{
			return true;
		}

		const TArray<FShanmenSwordRhythmContribution>& Contributions =
			Input.GetContributionBinding().GetContributions();
		OutEffects.Reserve(Contributions.Num());
		for (const FShanmenSwordRhythmContribution& Contribution
			: Contributions)
		{
			FShanmenSwordRhythmEffectSpecification Specification;
			FShanmenSwordRhythmEvaluatedEffect Effect;
			if (!Policy.TryFindSpecification(
					Contribution.GetKind(), Specification)
				|| !FShanmenSwordRhythmEvaluatedEffect::TryCreate(
					Input, Specification, Contribution, Effect))
			{
				OutEffects.Reset();
				return false;
			}
			OutEffects.Add(MoveTemp(Effect));
		}
		return true;
	}

	FGuid MakeEvaluationReceiptId(
		const FShanmenSwordRhythmEvaluationPolicy& Policy,
		const FShanmenSwordRhythmEvaluationInput& Input,
		const TArray<FShanmenSwordRhythmEvaluatedEffect>& Effects)
	{
		if (!Policy.IsValid() || !Input.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Policy.GetPolicyId()),
			GuidDigits(Input.GetInputId()),
			FString::FromInt(Effects.Num())
		};
		for (const FShanmenSwordRhythmEvaluatedEffect& Effect : Effects)
		{
			if (!Effect.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Effect.GetEffectId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.EvaluationReceipt.r1"), Parts);
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

bool FShanmenSwordRhythmEffectSpecification::TryCapture(
	const FShanmenSwordRhythmEffectSpecificationCapture& InCapture,
	const FShanmenContentStamp& InContent,
	FShanmenSwordRhythmEffectSpecification& OutSpecification)
{
	OutSpecification = FShanmenSwordRhythmEffectSpecification();
	FShanmenSwordRhythmEffectSpecification Candidate;
	Candidate.ContributionKind = InCapture.ContributionKind;
	Candidate.EffectDefinitionId = InCapture.EffectDefinitionId;
	Candidate.Content = InContent;
	Candidate.SpecificationId = MakeSpecificationId(
		Candidate.ContributionKind,
		Candidate.EffectDefinitionId,
		Candidate.Content);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSpecification = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmEffectSpecification::IsValid() const
{
	return SpecificationId.IsValid()
		&& SpecificationId == MakeSpecificationId(
			ContributionKind, EffectDefinitionId, Content);
}

bool FShanmenSwordRhythmEvaluationPolicy::TryCapture(
	const FShanmenSwordRhythmEvaluationPolicyCapture& InCapture,
	FShanmenSwordRhythmEvaluationPolicy& OutPolicy)
{
	OutPolicy = FShanmenSwordRhythmEvaluationPolicy();
	if (InCapture.PolicyDefinitionId.IsNone()
		|| !InCapture.Content.IsValid()
		|| InCapture.Specifications.Num() != KnownContributionKinds().Num())
	{
		return false;
	}

	FShanmenSwordRhythmEvaluationPolicy Candidate;
	Candidate.PolicyDefinitionId = InCapture.PolicyDefinitionId;
	Candidate.Content = InCapture.Content;
	Candidate.Specifications.Reserve(InCapture.Specifications.Num());
	for (const FShanmenSwordRhythmEffectSpecificationCapture& Capture
		: InCapture.Specifications)
	{
		FShanmenSwordRhythmEffectSpecification Specification;
		if (!FShanmenSwordRhythmEffectSpecification::TryCapture(
				Capture, Candidate.Content, Specification))
		{
			return false;
		}
		Candidate.Specifications.Add(MoveTemp(Specification));
	}
	Candidate.Specifications.Sort(SpecificationComesBefore);
	Candidate.PolicyId = MakePolicyId(
		Candidate.PolicyDefinitionId,
		Candidate.Content,
		Candidate.Specifications);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPolicy = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmEvaluationPolicy::IsValid() const
{
	if (!PolicyId.IsValid()
		|| PolicyDefinitionId.IsNone()
		|| !Content.IsValid()
		|| Specifications.Num() != KnownContributionKinds().Num())
	{
		return false;
	}

	TSet<EContributionKind> Kinds;
	TSet<FName> EffectDefinitionIds;
	for (int32 Index = 0; Index < Specifications.Num(); ++Index)
	{
		const FShanmenSwordRhythmEffectSpecification& Specification =
			Specifications[Index];
		if (!Specification.IsValid()
			|| !ContentMatches(Content, Specification.GetContent())
			|| Kinds.Contains(Specification.GetContributionKind())
			|| EffectDefinitionIds.Contains(
				Specification.GetEffectDefinitionId())
			|| (Index > 0
				&& !SpecificationComesBefore(
					Specifications[Index - 1], Specification)))
		{
			return false;
		}
		Kinds.Add(Specification.GetContributionKind());
		EffectDefinitionIds.Add(Specification.GetEffectDefinitionId());
	}
	for (const EContributionKind Kind : KnownContributionKinds())
	{
		if (!Kinds.Contains(Kind))
		{
			return false;
		}
	}
	return PolicyId == MakePolicyId(
		PolicyDefinitionId, Content, Specifications);
}

bool FShanmenSwordRhythmEvaluationPolicy::TryFindSpecification(
	EShanmenSwordRhythmContributionKind ContributionKind,
	FShanmenSwordRhythmEffectSpecification& OutSpecification) const
{
	OutSpecification = FShanmenSwordRhythmEffectSpecification();
	if (!IsValid() || !IsKnownContributionKind(ContributionKind))
	{
		return false;
	}
	for (const FShanmenSwordRhythmEffectSpecification& Specification
		: Specifications)
	{
		if (Specification.GetContributionKind() == ContributionKind)
		{
			OutSpecification = Specification;
			return true;
		}
	}
	return false;
}

bool FShanmenSwordRhythmEvaluatedEffect::TryCreate(
	const FShanmenSwordRhythmEvaluationInput& InInput,
	const FShanmenSwordRhythmEffectSpecification& InSpecification,
	const FShanmenSwordRhythmContribution& InContribution,
	FShanmenSwordRhythmEvaluatedEffect& OutEffect)
{
	OutEffect = FShanmenSwordRhythmEvaluatedEffect();
	if (!InputContainsContribution(InInput, InContribution))
	{
		return false;
	}
	FShanmenSwordRhythmEvaluatedEffect Candidate;
	Candidate.EvaluationInputId = InInput.GetInputId();
	Candidate.Specification = InSpecification;
	Candidate.Contribution = InContribution;
	Candidate.EffectId = MakeEffectId(
		Candidate.EvaluationInputId,
		Candidate.Specification,
		Candidate.Contribution);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutEffect = MoveTemp(Candidate);
	return true;
}

bool FShanmenSwordRhythmEvaluatedEffect::IsValid() const
{
	return EffectId.IsValid()
		&& EvaluationInputId.IsValid()
		&& Specification.IsValid()
		&& Contribution.IsValid()
		&& Specification.GetContributionKind() == Contribution.GetKind()
		&& EffectId == MakeEffectId(
			EvaluationInputId, Specification, Contribution);
}

bool FShanmenSwordRhythmEvaluationReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !Policy.IsValid() || !Input.IsValid())
	{
		return false;
	}
	TArray<FShanmenSwordRhythmEvaluatedEffect> ExpectedEffects;
	if (!BuildCanonicalEffects(Input, Policy, ExpectedEffects)
		|| ExpectedEffects.Num() != Effects.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Effects.Num(); ++Index)
	{
		if (!EffectsMatch(Effects[Index], ExpectedEffects[Index]))
		{
			return false;
		}
	}
	return ReceiptId == MakeEvaluationReceiptId(Policy, Input, Effects);
}

bool FShanmenSwordRhythmEvaluationResult::IsSuccess() const
{
	return Status == EShanmenSwordRhythmEvaluationStatus::Evaluated
		&& Receipt.IsValid();
}

FShanmenSwordRhythmEvaluationResult FShanmenSwordRhythmEvaluator::Evaluate(
	const FShanmenSwordRhythmEvaluationInput& InInput,
	const FShanmenSwordRhythmEvaluationPolicy& InPolicy)
{
	FShanmenSwordRhythmEvaluationResult Result;
	if (!InInput.IsValid())
	{
		Result.Status = EShanmenSwordRhythmEvaluationStatus::InputInvalid;
		Result.Diagnostic =
			TEXT("Sword-rhythm evaluation requires one valid immutable input.");
		return Result;
	}
	if (!InPolicy.IsValid())
	{
		Result.Status = EShanmenSwordRhythmEvaluationStatus::PolicyInvalid;
		Result.Diagnostic =
			TEXT("Sword-rhythm evaluation requires one complete authored policy.");
		return Result;
	}

	FShanmenSwordRhythmEvaluationReceipt Candidate;
	Candidate.Policy = InPolicy;
	Candidate.Input = InInput;
	if (!BuildCanonicalEffects(
			Candidate.Input, Candidate.Policy, Candidate.Effects))
	{
		Result.Status =
			EShanmenSwordRhythmEvaluationStatus::EffectMappingRejected;
		Result.Diagnostic =
			TEXT("Sword-rhythm contribution could not map to an authored effect.");
		return Result;
	}
	Candidate.ReceiptId = MakeEvaluationReceiptId(
		Candidate.Policy, Candidate.Input, Candidate.Effects);
	if (!Candidate.IsValid())
	{
		Result.Status = EShanmenSwordRhythmEvaluationStatus::ReceiptRejected;
		Result.Diagnostic =
			TEXT("Sword-rhythm evaluator rejected its candidate receipt.");
		return Result;
	}

	Result.Status = EShanmenSwordRhythmEvaluationStatus::Evaluated;
	Result.Diagnostic = Candidate.Effects.IsEmpty()
		? TEXT("Sword-rhythm input produced a valid empty effect receipt.")
		: TEXT("Sword-rhythm contributions mapped to authored effect tokens.");
	Result.Receipt = MoveTemp(Candidate);
	return Result;
}
