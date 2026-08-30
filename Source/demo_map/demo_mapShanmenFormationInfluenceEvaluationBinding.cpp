#include "demo_mapShanmenFormationInfluenceEvaluationBinding.h"

namespace
{
	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool IsCanonicalEmptyReceipt(
		const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Receipt)
	{
		return !Receipt.ReceiptId.IsValid()
			&& !Receipt.Context.RunId.IsValid()
			&& !Receipt.Context.SubjectEntityId.IsValid()
			&& !Receipt.Context.Channel.IsValid()
			&& Receipt.Context.SubjectTags.IsEmpty()
			&& Receipt.Context.Content.Version.IsNone()
			&& Receipt.Context.Content.Digest.IsEmpty()
			&& Receipt.Decisions.IsEmpty()
			&& Receipt.FinalMagnitudeUnits == 0;
	}
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationBinding::TryCaptureApply(
	const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& RequestedReceipt,
	Fdemo_mapShanmenFormationInfluenceEvaluationBinding& OutBinding)
{
	OutBinding = Fdemo_mapShanmenFormationInfluenceEvaluationBinding();
	if (!RequestedReceipt.IsValid() || RequestedReceipt.Decisions.IsEmpty())
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceEvaluationBinding Candidate;
	Candidate.bHasReceipt = true;
	Candidate.Receipt = RequestedReceipt;
	if (!Candidate.IsStructurallyValid())
	{
		return false;
	}
	OutBinding = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenFormationInfluenceEvaluationBinding
Fdemo_mapShanmenFormationInfluenceEvaluationBinding::MakeRemove()
{
	return Fdemo_mapShanmenFormationInfluenceEvaluationBinding();
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
IsStructurallyValid() const
{
	return bHasReceipt
		? Receipt.IsValid() && !Receipt.Decisions.IsEmpty()
		: IsCanonicalEmptyReceipt(Receipt);
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
ReceiptMatchesInfluenceIdentity(
	const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Candidate,
	const Fdemo_mapShanmenFormationInfluenceIntent& Intent)
{
	if (!Candidate.IsValid() || Candidate.Decisions.IsEmpty()
		|| !Intent.IsValid()
		|| Candidate.Context.RunId != Intent.RunId
		|| Candidate.Context.SubjectEntityId != Intent.SubjectEntityId
		|| !SameContent(Candidate.Context.Content, Intent.Content))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluencePolicy Policy;
	Policy.PolicyDefinitionId = Intent.PolicyDefinitionId;
	Policy.InfluenceDefinitionId = Intent.InfluenceDefinitionId;
	Policy.Content = Intent.Content;
	if (!Policy.IsValid())
	{
		return false;
	}
	for (const auto& Decision : Candidate.Decisions)
	{
		if (!Decision.Specification.MatchesPolicy(Policy))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationBinding::MatchesIntent(
	const Fdemo_mapShanmenFormationInfluenceIntent& Intent) const
{
	if (!IsStructurallyValid() || !Intent.IsValid())
	{
		return false;
	}
	switch (Intent.Operation)
	{
	case Edemo_mapShanmenFormationInfluenceOperation::Apply:
		return bHasReceipt
			&& ReceiptMatchesInfluenceIdentity(Receipt, Intent);
	case Edemo_mapShanmenFormationInfluenceOperation::Remove:
		return !bHasReceipt;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationInfluenceEvaluationBinding::Matches(
	const Fdemo_mapShanmenFormationInfluenceEvaluationBinding& Other) const
{
	if (!IsStructurallyValid() || !Other.IsStructurallyValid()
		|| bHasReceipt != Other.bHasReceipt)
	{
		return false;
	}
	return !bHasReceipt || Receipt.Matches(Other.Receipt);
}
