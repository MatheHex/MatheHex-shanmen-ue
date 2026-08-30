#include "demo_mapShanmenFormationInfluenceModifierEvaluator.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FSpecification =
		Fdemo_mapShanmenFormationInfluenceModifierSpecification;
	using FContext = Fdemo_mapShanmenFormationInfluenceEvaluationContext;
	using FDecision =
		Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord;
	using EDecision = Edemo_mapShanmenFormationInfluenceModifierDecision;
	using EStackPolicy = Edemo_mapShanmenFormationInfluenceStackPolicy;
	using EStatus = Edemo_mapShanmenFormationInfluenceEvaluationStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool IsKnownStackPolicy(const EStackPolicy Policy)
	{
		return Policy == EStackPolicy::Additive
			|| Policy == EStackPolicy::StrongestMagnitude
			|| Policy == EStackPolicy::HighestPriority;
	}

	void AppendSortedTags(
		const FGameplayTagContainer& Tags,
		TArray<FString>& InOutParts)
	{
		TArray<FGameplayTag> Sorted;
		Tags.GetGameplayTagArray(Sorted);
		Sorted.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});
		InOutParts.Add(FString::FromInt(Sorted.Num()));
		for (const FGameplayTag& Tag : Sorted)
		{
			InOutParts.Add(Tag.ToString());
		}
	}

	bool TagsAreValid(const FGameplayTagContainer& Tags)
	{
		for (const FGameplayTag& Tag : Tags.GetGameplayTagArray())
		{
			if (!Tag.IsValid())
			{
				return false;
			}
		}
		return true;
	}

	bool HasValidSpecificationFields(const FSpecification& Specification)
	{
		return !Specification.GetPolicyDefinitionId().IsNone()
			&& !Specification.GetInfluenceDefinitionId().IsNone()
			&& !Specification.GetModifierDefinitionId().IsNone()
			&& Specification.GetChannel().IsValid()
			&& TagsAreValid(Specification.GetRequiredSubjectTags())
			&& TagsAreValid(Specification.GetBlockedSubjectTags())
			&& !Specification.GetRequiredSubjectTags().HasAnyExact(
				Specification.GetBlockedSubjectTags())
			&& Specification.GetMagnitudeUnits() != 0
			&& !Specification.GetStackGroupId().IsNone()
			&& IsKnownStackPolicy(Specification.GetStackPolicy())
			&& Specification.GetContent().IsValid();
	}

	FGuid MakeSpecificationId(const FSpecification& Specification)
	{
		if (!HasValidSpecificationFields(Specification))
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			Specification.GetPolicyDefinitionId().ToString(),
			Specification.GetInfluenceDefinitionId().ToString(),
			Specification.GetModifierDefinitionId().ToString(),
			Specification.GetChannel().ToString(),
			FString::FromInt(Specification.GetMagnitudeUnits()),
			Specification.GetStackGroupId().ToString(),
			FString::FromInt(static_cast<int32>(
				Specification.GetStackPolicy())),
			FString::FromInt(Specification.GetPriority()),
			Specification.GetContent().Version.ToString(),
			Specification.GetContent().Digest
		};
		AppendSortedTags(Specification.GetRequiredSubjectTags(), Parts);
		AppendSortedTags(Specification.GetBlockedSubjectTags(), Parts);
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceModifierSpecification.r1"), Parts);
	}

	bool HasSameStackDomain(
		const FSpecification& Left,
		const FSpecification& Right)
	{
		return Left.GetChannel().MatchesTagExact(Right.GetChannel())
			&& Left.GetStackGroupId() == Right.GetStackGroupId();
	}

	int64 AbsoluteMagnitude(const int32 Magnitude)
	{
		const int64 Wide = Magnitude;
		return Wide < 0 ? -Wide : Wide;
	}

	bool CandidateOutranks(
		const FSpecification& Candidate,
		const FSpecification& Current)
	{
		switch (Candidate.GetStackPolicy())
		{
		case EStackPolicy::StrongestMagnitude:
			if (AbsoluteMagnitude(Candidate.GetMagnitudeUnits())
				!= AbsoluteMagnitude(Current.GetMagnitudeUnits()))
			{
				return AbsoluteMagnitude(Candidate.GetMagnitudeUnits())
					> AbsoluteMagnitude(Current.GetMagnitudeUnits());
			}
			return Candidate.GetPriority() > Current.GetPriority();
		case EStackPolicy::HighestPriority:
			if (Candidate.GetPriority() != Current.GetPriority())
			{
				return Candidate.GetPriority() > Current.GetPriority();
			}
			return AbsoluteMagnitude(Candidate.GetMagnitudeUnits())
				> AbsoluteMagnitude(Current.GetMagnitudeUnits());
		default:
			return false;
		}
	}

	bool BuildCanonicalEvaluation(
		const FContext& Context,
		const TArray<FSpecification>& InputSpecifications,
		TArray<FDecision>& OutDecisions,
		int64& OutFinalMagnitude,
		EStatus& OutStatus,
		FString& OutDiagnostic)
	{
		OutDecisions.Reset();
		OutFinalMagnitude = 0;
		OutStatus = EStatus::ContextInvalid;
		OutDiagnostic.Reset();
		if (!Context.IsValid())
		{
			OutDiagnostic = TEXT("Influence evaluation requires one valid context.");
			return false;
		}

		TSet<FGuid> SpecificationIds;
		TSet<FName> ModifierDefinitionIds;
		for (const FSpecification& Specification : InputSpecifications)
		{
			if (!Specification.IsValid())
			{
				OutStatus = EStatus::SpecificationInvalid;
				OutDiagnostic = TEXT("Influence evaluation rejected an invalid specification.");
				return false;
			}
			if (!ContentMatches(Context.Content, Specification.GetContent()))
			{
				OutStatus = EStatus::ContentMismatch;
				OutDiagnostic = TEXT("Influence context and specification content identities differ.");
				return false;
			}
			if (SpecificationIds.Contains(Specification.GetSpecificationId())
				|| ModifierDefinitionIds.Contains(
					Specification.GetModifierDefinitionId()))
			{
				OutStatus = EStatus::DuplicateSpecification;
				OutDiagnostic = TEXT("Influence evaluation rejected duplicate authored identity.");
				return false;
			}
			SpecificationIds.Add(Specification.GetSpecificationId());
			ModifierDefinitionIds.Add(
				Specification.GetModifierDefinitionId());
		}

		TArray<FSpecification> Specifications = InputSpecifications;
		Specifications.Sort([](
			const FSpecification& Left,
			const FSpecification& Right)
		{
			return GuidDigits(Left.GetSpecificationId())
				< GuidDigits(Right.GetSpecificationId());
		});

		for (int32 LeftIndex = 0; LeftIndex < Specifications.Num(); ++LeftIndex)
		{
			for (int32 RightIndex = LeftIndex + 1;
				RightIndex < Specifications.Num(); ++RightIndex)
			{
				if (HasSameStackDomain(
						Specifications[LeftIndex], Specifications[RightIndex])
					&& Specifications[LeftIndex].GetStackPolicy()
						!= Specifications[RightIndex].GetStackPolicy())
				{
					OutStatus = EStatus::StackPolicyConflict;
					OutDiagnostic = TEXT("One channel/stack group contains conflicting policies.");
					return false;
				}
			}
		}

		OutDecisions.Reserve(Specifications.Num());
		for (const FSpecification& Specification : Specifications)
		{
			FDecision& Decision = OutDecisions.AddDefaulted_GetRef();
			Decision.Specification = Specification;
			if (!Specification.GetChannel().MatchesTagExact(Context.Channel))
			{
				Decision.Decision = EDecision::ChannelMismatch;
			}
			else if (!Context.SubjectTags.HasAll(
					Specification.GetRequiredSubjectTags()))
			{
				Decision.Decision = EDecision::RequiredTagsMissing;
			}
			else if (Context.SubjectTags.HasAny(
					Specification.GetBlockedSubjectTags()))
			{
				Decision.Decision = EDecision::BlockedByTags;
			}
			else
			{
				Decision.Decision = EDecision::Applied;
				Decision.ContributionMagnitudeUnits =
					Specification.GetMagnitudeUnits();
			}
		}

		TSet<FString> ResolvedStackDomains;
		for (int32 Index = 0; Index < OutDecisions.Num(); ++Index)
		{
			FDecision& Decision = OutDecisions[Index];
			if (Decision.Decision != EDecision::Applied)
			{
				continue;
			}
			const FString StackDomain =
				Decision.Specification.GetChannel().ToString()
				+ TEXT("|")
				+ Decision.Specification.GetStackGroupId().ToString();
			if (ResolvedStackDomains.Contains(StackDomain))
			{
				continue;
			}
			ResolvedStackDomains.Add(StackDomain);
			if (Decision.Specification.GetStackPolicy()
				== EStackPolicy::Additive)
			{
				continue;
			}

			int32 WinnerIndex = Index;
			for (int32 CandidateIndex = Index + 1;
				CandidateIndex < OutDecisions.Num(); ++CandidateIndex)
			{
				const FDecision& Candidate = OutDecisions[CandidateIndex];
				if (Candidate.Decision == EDecision::Applied
					&& HasSameStackDomain(
						Decision.Specification, Candidate.Specification)
					&& CandidateOutranks(
						Candidate.Specification,
						OutDecisions[WinnerIndex].Specification))
				{
					WinnerIndex = CandidateIndex;
				}
			}

			const FGuid WinnerId = OutDecisions[WinnerIndex]
				.Specification.GetSpecificationId();
			for (int32 CandidateIndex = Index;
				CandidateIndex < OutDecisions.Num(); ++CandidateIndex)
			{
				FDecision& Candidate = OutDecisions[CandidateIndex];
				if (CandidateIndex != WinnerIndex
					&& Candidate.Decision == EDecision::Applied
					&& HasSameStackDomain(
						Decision.Specification, Candidate.Specification))
				{
					Candidate.Decision = EDecision::StackSuppressed;
					Candidate.ContributionMagnitudeUnits = 0;
					Candidate.WinningSpecificationId = WinnerId;
				}
			}
		}

		for (const FDecision& Decision : OutDecisions)
		{
			OutFinalMagnitude += Decision.ContributionMagnitudeUnits;
		}
		OutStatus = EStatus::Evaluated;
		OutDiagnostic = OutDecisions.IsEmpty()
			? TEXT("Influence evaluation produced a valid empty receipt.")
			: TEXT("Influence specifications produced canonical modifier decisions.");
		return true;
	}

	FGuid MakeReceiptId(
		const FContext& Context,
		const TArray<FDecision>& Decisions,
		const int64 FinalMagnitude)
	{
		if (!Context.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Context.RunId), GuidDigits(Context.SubjectEntityId),
			Context.Channel.ToString(), Context.Content.Version.ToString(),
			Context.Content.Digest, LexToString(FinalMagnitude),
			FString::FromInt(Decisions.Num())
		};
		AppendSortedTags(Context.SubjectTags, Parts);
		for (const FDecision& Decision : Decisions)
		{
			Parts.Add(GuidDigits(
				Decision.Specification.GetSpecificationId()));
			Parts.Add(FString::FromInt(static_cast<int32>(Decision.Decision)));
			Parts.Add(FString::FromInt(
				Decision.ContributionMagnitudeUnits));
			Parts.Add(GuidDigits(Decision.WinningSpecificationId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceEvaluationReceipt.r1"), Parts);
	}
}

bool Fdemo_mapShanmenFormationInfluenceModifierSpecification::TryCreate(
	const Fdemo_mapShanmenFormationInfluencePolicy& Policy,
	const FName ModifierDefinitionId,
	const FGameplayTag Channel,
	const FGameplayTagContainer& RequiredSubjectTags,
	const FGameplayTagContainer& BlockedSubjectTags,
	const int32 MagnitudeUnits,
	const FName StackGroupId,
	const Edemo_mapShanmenFormationInfluenceStackPolicy StackPolicy,
	const int32 Priority,
	Fdemo_mapShanmenFormationInfluenceModifierSpecification& OutSpecification)
{
	OutSpecification = Fdemo_mapShanmenFormationInfluenceModifierSpecification();
	if (!Policy.IsValid())
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceModifierSpecification Candidate;
	Candidate.PolicyDefinitionId = Policy.PolicyDefinitionId;
	Candidate.InfluenceDefinitionId = Policy.InfluenceDefinitionId;
	Candidate.ModifierDefinitionId = ModifierDefinitionId;
	Candidate.Channel = Channel;
	Candidate.RequiredSubjectTags = RequiredSubjectTags;
	Candidate.BlockedSubjectTags = BlockedSubjectTags;
	Candidate.MagnitudeUnits = MagnitudeUnits;
	Candidate.StackGroupId = StackGroupId;
	Candidate.StackPolicy = StackPolicy;
	Candidate.Priority = Priority;
	Candidate.Content = Policy.Content;
	Candidate.SpecificationId = MakeSpecificationId(Candidate);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSpecification = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceModifierSpecification::IsValid() const
{
	return SpecificationId.IsValid()
		&& MakeSpecificationId(*this) == SpecificationId;
}

bool Fdemo_mapShanmenFormationInfluenceModifierSpecification::Matches(
	const Fdemo_mapShanmenFormationInfluenceModifierSpecification& Other) const
{
	return IsValid() && Other.IsValid()
		&& SpecificationId == Other.SpecificationId
		&& PolicyDefinitionId == Other.PolicyDefinitionId
		&& InfluenceDefinitionId == Other.InfluenceDefinitionId
		&& ModifierDefinitionId == Other.ModifierDefinitionId
		&& Channel.MatchesTagExact(Other.Channel)
		&& RequiredSubjectTags == Other.RequiredSubjectTags
		&& BlockedSubjectTags == Other.BlockedSubjectTags
		&& MagnitudeUnits == Other.MagnitudeUnits
		&& StackGroupId == Other.StackGroupId
		&& StackPolicy == Other.StackPolicy && Priority == Other.Priority
		&& ContentMatches(Content, Other.Content);
}

bool Fdemo_mapShanmenFormationInfluenceModifierSpecification::MatchesPolicy(
	const Fdemo_mapShanmenFormationInfluencePolicy& Policy) const
{
	return IsValid() && Policy.IsValid()
		&& PolicyDefinitionId == Policy.PolicyDefinitionId
		&& InfluenceDefinitionId == Policy.InfluenceDefinitionId
		&& ContentMatches(Content, Policy.Content);
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationContext::IsValid() const
{
	return RunId.IsValid() && SubjectEntityId.IsValid() && Channel.IsValid()
		&& TagsAreValid(SubjectTags) && Content.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationContext::Matches(
	const Fdemo_mapShanmenFormationInfluenceEvaluationContext& Other) const
{
	return IsValid() && Other.IsValid() && RunId == Other.RunId
		&& SubjectEntityId == Other.SubjectEntityId
		&& Channel.MatchesTagExact(Other.Channel)
		&& SubjectTags == Other.SubjectTags
		&& ContentMatches(Content, Other.Content);
}

bool Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord::Matches(
	const Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord& Other) const
{
	return Specification.Matches(Other.Specification)
		&& Decision == Other.Decision
		&& ContributionMagnitudeUnits == Other.ContributionMagnitudeUnits
		&& WinningSpecificationId == Other.WinningSpecificationId;
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !Context.IsValid())
	{
		return false;
	}
	TArray<FSpecification> Specifications;
	Specifications.Reserve(Decisions.Num());
	for (const FDecision& Decision : Decisions)
	{
		Specifications.Add(Decision.Specification);
	}
	TArray<FDecision> ExpectedDecisions;
	int64 ExpectedMagnitude = 0;
	EStatus Status = EStatus::ContextInvalid;
	FString Diagnostic;
	if (!BuildCanonicalEvaluation(
			Context, Specifications, ExpectedDecisions, ExpectedMagnitude,
			Status, Diagnostic)
		|| Status != EStatus::Evaluated
		|| ExpectedMagnitude != FinalMagnitudeUnits
		|| ExpectedDecisions.Num() != Decisions.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Decisions.Num(); ++Index)
	{
		if (!Decisions[Index].Matches(ExpectedDecisions[Index]))
		{
			return false;
		}
	}
	return ReceiptId == MakeReceiptId(Context, Decisions, FinalMagnitudeUnits);
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationReceipt::Matches(
	const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Other) const
{
	if (!IsValid() || !Other.IsValid() || ReceiptId != Other.ReceiptId
		|| !Context.Matches(Other.Context)
		|| FinalMagnitudeUnits != Other.FinalMagnitudeUnits
		|| Decisions.Num() != Other.Decisions.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Decisions.Num(); ++Index)
	{
		if (!Decisions[Index].Matches(Other.Decisions[Index]))
		{
			return false;
		}
	}
	return true;
}

int32 Fdemo_mapShanmenFormationInfluenceEvaluationReceipt::GetAppliedCount()
	const
{
	int32 Count = 0;
	for (const FDecision& Decision : Decisions)
	{
		Count += Decision.Decision == EDecision::Applied ? 1 : 0;
	}
	return Count;
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationInfluenceEvaluationStatus::Evaluated
		&& Receipt.IsValid();
}

Fdemo_mapShanmenFormationInfluenceEvaluationResult
Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
	const Fdemo_mapShanmenFormationInfluenceEvaluationContext& Context,
	const TArray<Fdemo_mapShanmenFormationInfluenceModifierSpecification>&
		Specifications)
{
	Fdemo_mapShanmenFormationInfluenceEvaluationResult Result;
	Result.Receipt.Context = Context;
	if (!BuildCanonicalEvaluation(
			Context, Specifications, Result.Receipt.Decisions,
			Result.Receipt.FinalMagnitudeUnits, Result.Status,
			Result.Diagnostic))
	{
		Result.Receipt = Fdemo_mapShanmenFormationInfluenceEvaluationReceipt();
		return Result;
	}
	Result.Receipt.ReceiptId = MakeReceiptId(
		Result.Receipt.Context, Result.Receipt.Decisions,
		Result.Receipt.FinalMagnitudeUnits);
	if (!Result.Receipt.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceEvaluationStatus::
			ReceiptRejected;
		Result.Diagnostic = TEXT("Influence evaluation receipt failed self-validation.");
		Result.Receipt = Fdemo_mapShanmenFormationInfluenceEvaluationReceipt();
	}
	return Result;
}
