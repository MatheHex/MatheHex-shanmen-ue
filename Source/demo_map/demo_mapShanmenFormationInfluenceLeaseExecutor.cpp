#include "demo_mapShanmenFormationInfluenceLeaseExecutor.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool IsKnownOperation(
		const Edemo_mapShanmenFormationInfluenceOperation Operation)
	{
		return Operation == Edemo_mapShanmenFormationInfluenceOperation::Apply
			|| Operation
				== Edemo_mapShanmenFormationInfluenceOperation::Remove;
	}

	FGuid MakeLeaseId(
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key)
	{
		if (!Key.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceLease.r1"),
			{
				GuidDigits(Key.RunId), GuidDigits(Key.OwnerId),
				GuidDigits(Key.SourceEntityId), GuidDigits(Key.DeploymentId),
				GuidDigits(Key.AreaId), GuidDigits(Key.SubjectEntityId),
				Key.PolicyDefinitionId.ToString(),
				Key.InfluenceDefinitionId.ToString(),
				Key.Content.Version.ToString(), Key.Content.Digest
			});
	}

	bool InvocationsMatch(
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Left,
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.LedgerId == Right.LedgerId
			&& Left.Intent.IntentId == Right.Intent.IntentId
			&& Left.AttemptId == Right.AttemptId;
	}

	FGuid MakeExecutorReceiptId(
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation,
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key)
	{
		if (!Invocation.IsValid() || !Key.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceLeaseExecutorReceipt.r1"),
			{
				GuidDigits(Invocation.LedgerId),
				GuidDigits(Invocation.Intent.IntentId),
				GuidDigits(Invocation.AttemptId), GuidDigits(MakeLeaseId(Key)),
				FString::FromInt(static_cast<int32>(
					Invocation.Intent.Operation))
			});
	}

	Fdemo_mapShanmenFormationInfluenceExecutorResult Completed(
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation,
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceExecutorResult Result;
		Result.Status =
			Edemo_mapShanmenFormationInfluenceExecutorStatus::Completed;
		Result.Diagnostic = Diagnostic;
		Result.Receipt.LedgerId = Invocation.LedgerId;
		Result.Receipt.IntentId = Invocation.Intent.IntentId;
		Result.Receipt.AttemptId = Invocation.AttemptId;
		Result.Receipt.ExecutorReceiptId =
			MakeExecutorReceiptId(Invocation, Key);
		Result.Receipt.Outcome =
			Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceExecutorResult Rejected(
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceExecutorResult Result;
		Result.Status =
			Edemo_mapShanmenFormationInfluenceExecutorStatus::Rejected;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
	const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
	Fdemo_mapShanmenFormationInfluenceLeaseKey& OutKey)
{
	OutKey = Fdemo_mapShanmenFormationInfluenceLeaseKey();
	if (!Intent.IsValid())
	{
		return false;
	}
	OutKey.RunId = Intent.RunId;
	OutKey.OwnerId = Intent.OwnerId;
	OutKey.SourceEntityId = Intent.SourceEntityId;
	OutKey.DeploymentId = Intent.DeploymentId;
	OutKey.AreaId = Intent.AreaId;
	OutKey.SubjectEntityId = Intent.SubjectEntityId;
	OutKey.PolicyDefinitionId = Intent.PolicyDefinitionId;
	OutKey.InfluenceDefinitionId = Intent.InfluenceDefinitionId;
	OutKey.Content = Intent.Content;
	return OutKey.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceLeaseKey::IsValid() const
{
	return RunId.IsValid() && OwnerId.IsValid() && SourceEntityId.IsValid()
		&& DeploymentId.IsValid() && AreaId.IsValid()
		&& SubjectEntityId.IsValid() && !PolicyDefinitionId.IsNone()
		&& !InfluenceDefinitionId.IsNone() && Content.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceLeaseKey::Matches(
	const Fdemo_mapShanmenFormationInfluenceLeaseKey& Other) const
{
	return IsValid() && Other.IsValid() && RunId == Other.RunId
		&& OwnerId == Other.OwnerId && SourceEntityId == Other.SourceEntityId
		&& DeploymentId == Other.DeploymentId && AreaId == Other.AreaId
		&& SubjectEntityId == Other.SubjectEntityId
		&& PolicyDefinitionId == Other.PolicyDefinitionId
		&& InfluenceDefinitionId == Other.InfluenceDefinitionId
		&& SameContent(Content, Other.Content);
}

bool Fdemo_mapShanmenFormationInfluenceLeaseSnapshot::IsValid() const
{
	return Key.IsValid() && ApplyIntentId.IsValid()
		&& LeaseId == MakeLeaseId(Key);
}

Fdemo_mapShanmenFormationInfluenceLeaseSnapshot*
Fdemo_mapShanmenFormationInfluenceLeaseExecutor::FindActiveLease(
	const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key)
{
	return ActiveLeases.FindByPredicate(
		[&Key](const auto& Lease) { return Lease.Key.Matches(Key); });
}

const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot*
Fdemo_mapShanmenFormationInfluenceLeaseExecutor::FindActiveLease(
	const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key) const
{
	return ActiveLeases.FindByPredicate(
		[&Key](const auto& Lease) { return Lease.Key.Matches(Key); });
}

const Fdemo_mapShanmenFormationInfluenceLeaseExecutor::FCompletedIntentRecord*
Fdemo_mapShanmenFormationInfluenceLeaseExecutor::FindCompletedIntent(
	const FGuid& IntentId) const
{
	return CompletedIntents.FindByPredicate(
		[&IntentId](const auto& Record)
		{
			return Record.Intent.IntentId == IntentId;
		});
}

const Fdemo_mapShanmenFormationInfluenceLeaseExecutor::FAttemptRecord*
Fdemo_mapShanmenFormationInfluenceLeaseExecutor::FindAttempt(
	const FGuid& AttemptId) const
{
	return Attempts.FindByPredicate(
		[&AttemptId](const auto& Record)
		{
			return Record.Invocation.AttemptId == AttemptId;
		});
}

bool Fdemo_mapShanmenFormationInfluenceLeaseExecutor::IsConsistent() const
{
	TArray<FGuid> LeaseIds;
	for (const auto& Lease : ActiveLeases)
	{
		if (!Lease.IsValid() || LeaseIds.Contains(Lease.LeaseId))
		{
			return false;
		}
		for (const auto& Existing : ActiveLeases)
		{
			if (&Existing != &Lease && Existing.Key.Matches(Lease.Key))
			{
				return false;
			}
		}
		const FCompletedIntentRecord* Completion =
			FindCompletedIntent(Lease.ApplyIntentId);
		if (!Completion
			|| Completion->Intent.Operation
				!= Edemo_mapShanmenFormationInfluenceOperation::Apply
			|| !Completion->Key.Matches(Lease.Key))
		{
			return false;
		}
		LeaseIds.Add(Lease.LeaseId);
	}

	TArray<FGuid> CompletedIntentIds;
	for (const auto& Completion : CompletedIntents)
	{
		Fdemo_mapShanmenFormationInfluenceLeaseKey ExpectedKey;
		if (!Completion.Intent.IsValid() || !IsKnownOperation(
				Completion.Intent.Operation)
			|| !Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
				Completion.Intent, ExpectedKey)
			|| !ExpectedKey.Matches(Completion.Key)
			|| CompletedIntentIds.Contains(Completion.Intent.IntentId))
		{
			return false;
		}
		bool bHasSuccessfulAttempt = false;
		for (const auto& Attempt : Attempts)
		{
			bHasSuccessfulAttempt |=
				Attempt.Invocation.Intent.IntentId
					== Completion.Intent.IntentId
				&& Attempt.Result.IsSuccess();
		}
		if (!bHasSuccessfulAttempt)
		{
			return false;
		}
		CompletedIntentIds.Add(Completion.Intent.IntentId);
	}

	TArray<FGuid> AttemptIds;
	for (const auto& Attempt : Attempts)
	{
		if (!Attempt.Invocation.IsValid()
			|| AttemptIds.Contains(Attempt.Invocation.AttemptId))
		{
			return false;
		}
		if (Attempt.Result.Status
			== Edemo_mapShanmenFormationInfluenceExecutorStatus::Completed)
		{
			if (!Attempt.Result.IsSuccess()
				|| !Attempt.Result.Receipt.Matches(Attempt.Invocation)
				|| Attempt.Result.Receipt.Outcome
					!= Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded
				|| !FindCompletedIntent(Attempt.Invocation.Intent.IntentId))
			{
				return false;
			}
		}
		else if (Attempt.Result.Status
				!= Edemo_mapShanmenFormationInfluenceExecutorStatus::Rejected
			|| Attempt.Result.Receipt.IsValid())
		{
			return false;
		}
		AttemptIds.Add(Attempt.Invocation.AttemptId);
	}
	return true;
}

Fdemo_mapShanmenFormationInfluenceExecutorResult
Fdemo_mapShanmenFormationInfluenceLeaseExecutor::Execute(
	const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation)
{
	if (!IsConsistent())
	{
		return Rejected(TEXT("Influence lease executor is internally inconsistent."));
	}
	if (!Invocation.IsValid())
	{
		return Rejected(TEXT("Influence lease execution requires one valid invocation."));
	}
	if (const FAttemptRecord* Existing = FindAttempt(Invocation.AttemptId))
	{
		return InvocationsMatch(Existing->Invocation, Invocation)
			? Existing->Result
			: Rejected(TEXT("AttemptId is already bound to different invocation evidence."));
	}

	Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
	if (!Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
			Invocation.Intent, Key))
	{
		return Rejected(TEXT("Influence intent could not form one valid lease key."));
	}

	const auto Before = *this;
	auto Finish = [this, &Before, &Invocation](
		Fdemo_mapShanmenFormationInfluenceExecutorResult Result)
	{
		FAttemptRecord& Record = Attempts.AddDefaulted_GetRef();
		Record.Invocation = Invocation;
		Record.Result = Result;
		if (!IsConsistent())
		{
			*this = Before;
			return Rejected(TEXT("Influence lease attempt failed executor self-validation."));
		}
		return Result;
	};

	if (const FCompletedIntentRecord* Existing =
			FindCompletedIntent(Invocation.Intent.IntentId))
	{
		if (!Existing->Key.Matches(Key)
			|| Existing->Intent.Operation != Invocation.Intent.Operation)
		{
			return Rejected(TEXT("Completed intent identity conflicts with lease evidence."));
		}
		return Finish(Completed(
			Invocation, Key,
			TEXT("The completed intent recovered without repeating its lease mutation.")));
	}

	switch (Invocation.Intent.Operation)
	{
	case Edemo_mapShanmenFormationInfluenceOperation::Apply:
		if (FindActiveLease(Key))
		{
			return Finish(Rejected(
				TEXT("A different Apply intent already owns the active lease.")));
		}
		{
			Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& Lease =
				ActiveLeases.AddDefaulted_GetRef();
			Lease.LeaseId = MakeLeaseId(Key);
			Lease.Key = Key;
			Lease.ApplyIntentId = Invocation.Intent.IntentId;
			FCompletedIntentRecord& Completion =
				CompletedIntents.AddDefaulted_GetRef();
			Completion.Intent = Invocation.Intent;
			Completion.Key = Key;
		}
		return Finish(Completed(
			Invocation, Key, TEXT("Influence lease became active.")));
	case Edemo_mapShanmenFormationInfluenceOperation::Remove:
		{
			Fdemo_mapShanmenFormationInfluenceLeaseSnapshot* Lease =
				FindActiveLease(Key);
			if (!Lease)
			{
				return Finish(Rejected(
					TEXT("Remove requires one matching active influence lease.")));
			}
			const int32 LeaseIndex = ActiveLeases.IndexOfByPredicate(
				[&Key](const auto& Candidate)
				{
					return Candidate.Key.Matches(Key);
				});
			if (LeaseIndex == INDEX_NONE)
			{
				return Rejected(TEXT("Active lease lookup became inconsistent."));
			}
			ActiveLeases.RemoveAt(LeaseIndex);
			FCompletedIntentRecord& Completion =
				CompletedIntents.AddDefaulted_GetRef();
			Completion.Intent = Invocation.Intent;
			Completion.Key = Key;
		}
		return Finish(Completed(
			Invocation, Key, TEXT("Influence lease was removed.")));
	default:
		return Rejected(TEXT("Unknown influence lease operation."));
	}
}

bool Fdemo_mapShanmenFormationInfluenceLeaseExecutor::TryGetActiveLease(
	const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key,
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& OutLease) const
{
	OutLease = Fdemo_mapShanmenFormationInfluenceLeaseSnapshot();
	if (!IsConsistent() || !Key.IsValid())
	{
		return false;
	}
	const auto* Lease = FindActiveLease(Key);
	if (!Lease)
	{
		return false;
	}
	OutLease = *Lease;
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceLeaseExecutor::TryGetAttemptResult(
	const FGuid& AttemptId,
	Fdemo_mapShanmenFormationInfluenceExecutorResult& OutResult) const
{
	OutResult = Fdemo_mapShanmenFormationInfluenceExecutorResult();
	if (!IsConsistent() || !AttemptId.IsValid())
	{
		return false;
	}
	const FAttemptRecord* Record = FindAttempt(AttemptId);
	if (!Record)
	{
		return false;
	}
	OutResult = Record->Result;
	return true;
}
