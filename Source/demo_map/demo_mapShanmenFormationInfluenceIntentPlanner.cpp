#include "demo_mapShanmenFormationInfluenceIntentPlanner.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool IsKnownOperation(
		const Edemo_mapShanmenFormationInfluenceOperation Operation)
	{
		return Operation
				== Edemo_mapShanmenFormationInfluenceOperation::Apply
			|| Operation
				== Edemo_mapShanmenFormationInfluenceOperation::Remove;
	}

	FGuid MakeIntentId(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent)
	{
		if (!Intent.RunId.IsValid() || !Intent.OwnerId.IsValid()
			|| !Intent.SourceEntityId.IsValid()
			|| !Intent.DeploymentId.IsValid() || !Intent.AreaId.IsValid()
			|| !Intent.SubjectEntityId.IsValid()
			|| Intent.PolicyDefinitionId.IsNone()
			|| Intent.InfluenceDefinitionId.IsNone()
			|| !IsKnownOperation(Intent.Operation)
			|| !Intent.CauseId.IsValid() || !Intent.Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceIntent.r1"),
			{
				GuidDigits(Intent.RunId), GuidDigits(Intent.OwnerId),
				GuidDigits(Intent.SourceEntityId),
				GuidDigits(Intent.DeploymentId), GuidDigits(Intent.AreaId),
				GuidDigits(Intent.SubjectEntityId),
				Intent.PolicyDefinitionId.ToString(),
				Intent.InfluenceDefinitionId.ToString(),
				FString::FromInt(static_cast<int32>(Intent.Operation)),
				GuidDigits(Intent.CauseId),
				Intent.Content.Version.ToString(), Intent.Content.Digest
			});
	}

	bool IntentMatchesFact(
		const Fdemo_mapShanmenFormationInfluenceTransitionBatch& Batch,
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		const Fdemo_mapShanmenFormationMembershipTransitionFact& Fact,
		const Edemo_mapShanmenFormationInfluenceOperation Operation)
	{
		return Intent.IsValid()
			&& Intent.RunId == Batch.Area.RunId
			&& Intent.OwnerId == Batch.Area.OwnerId
			&& Intent.SourceEntityId == Batch.SourceEntityId
			&& Intent.DeploymentId == Batch.Area.DeploymentId
			&& Intent.AreaId == Batch.Area.AreaId
			&& Intent.SubjectEntityId == Fact.SubjectEntityId
			&& Intent.PolicyDefinitionId
				== Batch.Policy.PolicyDefinitionId
			&& Intent.InfluenceDefinitionId
				== Batch.Policy.InfluenceDefinitionId
			&& Intent.Operation == Operation
			&& Intent.CauseId == Fact.FactId
			&& ContentMatches(Intent.Content, Batch.Policy.Content);
	}

	FGuid MakeBatchId(
		const Fdemo_mapShanmenFormationInfluenceTransitionBatch& Batch)
	{
		if (!Batch.Area.IsValid() || !Batch.SourceEntityId.IsValid()
			|| !Batch.Policy.IsValid() || !Batch.Transition.IsValid()
			|| Batch.ApplyCount < 0 || Batch.RemoveCount < 0)
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Batch.Area.RunId), GuidDigits(Batch.Area.OwnerId),
			GuidDigits(Batch.SourceEntityId),
			GuidDigits(Batch.Area.DeploymentId),
			GuidDigits(Batch.Area.AreaId),
			Batch.Policy.PolicyDefinitionId.ToString(),
			Batch.Policy.InfluenceDefinitionId.ToString(),
			Batch.Policy.Content.Version.ToString(), Batch.Policy.Content.Digest,
			GuidDigits(Batch.Transition.ReceiptId),
			FString::FromInt(Batch.ApplyCount),
			FString::FromInt(Batch.RemoveCount),
			FString::FromInt(Batch.Intents.Num())
		};
		for (const Fdemo_mapShanmenFormationInfluenceIntent& Intent :
			Batch.Intents)
		{
			if (!Intent.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Intent.IntentId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceTransitionBatch.r1"), Parts);
	}

	Fdemo_mapShanmenFormationInfluencePlanResult Reject(
		const Edemo_mapShanmenFormationInfluencePlanStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluencePlanResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluencePolicy::IsValid() const
{
	return !PolicyDefinitionId.IsNone() && !InfluenceDefinitionId.IsNone()
		&& Content.IsValid();
}

Fdemo_mapShanmenFormationInfluenceIntent
Fdemo_mapShanmenFormationInfluenceIntent::Make(
	const Fdemo_mapShanmenFormationAreaSnapshot& Area,
	const FGuid& SourceEntityId,
	const Fdemo_mapShanmenFormationInfluencePolicy& Policy,
	const FGuid& SubjectEntityId,
	const Edemo_mapShanmenFormationInfluenceOperation Operation,
	const FGuid& CauseId)
{
	Fdemo_mapShanmenFormationInfluenceIntent Intent;
	if (!Area.IsValid() || !SourceEntityId.IsValid() || !Policy.IsValid()
		|| !SubjectEntityId.IsValid() || !IsKnownOperation(Operation)
		|| !CauseId.IsValid() || !ContentMatches(Area.Content, Policy.Content))
	{
		return Intent;
	}
	Intent.RunId = Area.RunId;
	Intent.OwnerId = Area.OwnerId;
	Intent.SourceEntityId = SourceEntityId;
	Intent.DeploymentId = Area.DeploymentId;
	Intent.AreaId = Area.AreaId;
	Intent.SubjectEntityId = SubjectEntityId;
	Intent.PolicyDefinitionId = Policy.PolicyDefinitionId;
	Intent.InfluenceDefinitionId = Policy.InfluenceDefinitionId;
	Intent.Operation = Operation;
	Intent.CauseId = CauseId;
	Intent.Content = Policy.Content;
	Intent.IntentId = MakeIntentId(Intent);
	return Intent;
}

bool Fdemo_mapShanmenFormationInfluenceIntent::IsValid() const
{
	return IntentId.IsValid() && MakeIntentId(*this) == IntentId;
}

bool Fdemo_mapShanmenFormationInfluenceTransitionBatch::IsValid() const
{
	if (!BatchId.IsValid() || !Area.IsValid() || !SourceEntityId.IsValid()
		|| !Policy.IsValid() || !Transition.IsValid()
		|| !ContentMatches(Area.Content, Policy.Content)
		|| Transition.PreviousCoverage.AreaId != Area.AreaId
		|| Transition.CurrentCoverage.AreaId != Area.AreaId
		|| ApplyCount != Transition.EnteredCount
		|| RemoveCount != Transition.LeftCount
		|| Intents.Num() != ApplyCount + RemoveCount)
	{
		return false;
	}

	int32 IntentIndex = 0;
	TSet<FGuid> IntentIds;
	for (const Fdemo_mapShanmenFormationMembershipTransitionFact& Fact :
		Transition.Facts)
	{
		Edemo_mapShanmenFormationInfluenceOperation ExpectedOperation =
			Edemo_mapShanmenFormationInfluenceOperation::Apply;
		bool bRequiresIntent = true;
		switch (Fact.Kind)
		{
		case Edemo_mapShanmenFormationCoverageTransitionKind::Entered:
			ExpectedOperation =
				Edemo_mapShanmenFormationInfluenceOperation::Apply;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::Left:
			ExpectedOperation =
				Edemo_mapShanmenFormationInfluenceOperation::Remove;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered:
		case Edemo_mapShanmenFormationCoverageTransitionKind::RemainedOutside:
			bRequiresIntent = false;
			break;
		default:
			return false;
		}
		if (!bRequiresIntent)
		{
			continue;
		}
		if (!Intents.IsValidIndex(IntentIndex)
			|| !IntentMatchesFact(
				*this, Intents[IntentIndex], Fact, ExpectedOperation)
			|| IntentIds.Contains(Intents[IntentIndex].IntentId))
		{
			return false;
		}
		IntentIds.Add(Intents[IntentIndex].IntentId);
		++IntentIndex;
	}
	return IntentIndex == Intents.Num() && IntentIds.Num() == Intents.Num()
		&& BatchId == MakeBatchId(*this);
}

bool Fdemo_mapShanmenFormationInfluencePlanResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationInfluencePlanStatus::Planned
		&& Batch.IsValid();
}

Fdemo_mapShanmenFormationInfluencePlanResult
Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
	const Fdemo_mapShanmenFormationAreaSnapshot& Area,
	const FGuid& SourceEntityId,
	const Fdemo_mapShanmenFormationInfluencePolicy& Policy,
	const Fdemo_mapShanmenFormationCoverageTransitionReceipt& Transition)
{
	if (!Area.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::AreaInvalid,
			TEXT("Influence planning requires one valid immutable formation area."));
	}
	if (!SourceEntityId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::SourceInvalid,
			TEXT("Influence planning requires one stable source EntityId."));
	}
	if (!Policy.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::PolicyInvalid,
			TEXT("Influence planning requires one named, versioned policy."));
	}
	if (!Transition.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::TransitionInvalid,
			TEXT("Influence planning requires one valid coverage transition receipt."));
	}
	if (Transition.PreviousCoverage.AreaId != Area.AreaId
		|| Transition.CurrentCoverage.AreaId != Area.AreaId)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::AreaMismatch,
			TEXT("Coverage transition evidence belongs to another formation area."));
	}
	if (!ContentMatches(Area.Content, Policy.Content))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::ContentMismatch,
			TEXT("Formation area and influence policy content identities differ."));
	}

	Fdemo_mapShanmenFormationInfluencePlanResult Result;
	Result.Batch.Area = Area;
	Result.Batch.SourceEntityId = SourceEntityId;
	Result.Batch.Policy = Policy;
	Result.Batch.Transition = Transition;
	Result.Batch.Intents.Reserve(
		Transition.EnteredCount + Transition.LeftCount);
	for (const Fdemo_mapShanmenFormationMembershipTransitionFact& Fact :
		Transition.Facts)
	{
		Edemo_mapShanmenFormationInfluenceOperation Operation =
			Edemo_mapShanmenFormationInfluenceOperation::Apply;
		if (Fact.Kind
			== Edemo_mapShanmenFormationCoverageTransitionKind::Entered)
		{
			++Result.Batch.ApplyCount;
		}
		else if (Fact.Kind
			== Edemo_mapShanmenFormationCoverageTransitionKind::Left)
		{
			Operation = Edemo_mapShanmenFormationInfluenceOperation::Remove;
			++Result.Batch.RemoveCount;
		}
		else
		{
			continue;
		}

		Result.Batch.Intents.Add(
			Fdemo_mapShanmenFormationInfluenceIntent::Make(
				Area, SourceEntityId, Policy, Fact.SubjectEntityId,
				Operation, Fact.FactId));
	}
	Result.Batch.BatchId = MakeBatchId(Result.Batch);
	if (!Result.Batch.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluencePlanStatus::ReceiptRejected,
			TEXT("Influence intent evidence failed deterministic self-validation."));
	}
	Result.Status = Edemo_mapShanmenFormationInfluencePlanStatus::Planned;
	Result.Diagnostic = Result.Batch.Intents.IsEmpty()
		? TEXT("Coverage transition produced a valid no-op influence batch.")
		: TEXT("Coverage transition produced canonical influence delta intents.");
	return Result;
}
