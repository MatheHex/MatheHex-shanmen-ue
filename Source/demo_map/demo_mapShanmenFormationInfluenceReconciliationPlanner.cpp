#include "demo_mapShanmenFormationInfluenceReconciliationPlanner.h"

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

	bool HasModeShape(
		const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch)
	{
		switch (Batch.Mode)
		{
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Prime:
			return !Batch.Previous.IsSet() && Batch.Current.IsSet();
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Rebase:
			return Batch.Previous.IsSet() && Batch.Current.IsSet();
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset:
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Terminal:
			return Batch.Previous.IsSet() && !Batch.Current.IsSet();
		default:
			return false;
		}
	}

	bool HasCompatibleAuthority(
		const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch)
	{
		if (Batch.Mode
			!= Edemo_mapShanmenFormationInfluenceReconciliationMode::Rebase)
		{
			return true;
		}
		return Batch.Previous.IsSet() && Batch.Current.IsSet()
			&& Batch.Previous->Area.RunId == Batch.Current->Area.RunId
			&& Batch.Previous->Area.OwnerId == Batch.Current->Area.OwnerId
			&& Batch.Previous->Area.DeploymentId
				== Batch.Current->Area.DeploymentId;
	}

	void AddScopeIdentity(
		TArray<FString>& Parts,
		const Fdemo_mapShanmenFormationInfluenceScope& Scope)
	{
		Parts.Append({
			GuidDigits(Scope.Area.RunId), GuidDigits(Scope.Area.OwnerId),
			GuidDigits(Scope.Area.DeploymentId), GuidDigits(Scope.Area.AreaId),
			Scope.Policy.PolicyDefinitionId.ToString(),
			Scope.Policy.InfluenceDefinitionId.ToString(),
			Scope.Policy.Content.Version.ToString(),
			Scope.Policy.Content.Digest,
			GuidDigits(Scope.Coverage.ReceiptId)
		});
	}

	bool HasSameEffectScope(
		const Fdemo_mapShanmenFormationInfluenceScope& Previous,
		const Fdemo_mapShanmenFormationInfluenceScope& Current)
	{
		return Previous.Area.RunId == Current.Area.RunId
			&& Previous.Area.OwnerId == Current.Area.OwnerId
			&& Previous.Area.DeploymentId == Current.Area.DeploymentId
			&& Previous.Area.AreaId == Current.Area.AreaId
			&& Previous.Policy.PolicyDefinitionId
				== Current.Policy.PolicyDefinitionId
			&& Previous.Policy.InfluenceDefinitionId
				== Current.Policy.InfluenceDefinitionId
			&& ContentMatches(
				Previous.Policy.Content, Current.Policy.Content);
	}

	TSet<FGuid> CoveredSubjects(
		const Fdemo_mapShanmenFormationInfluenceScope& Scope)
	{
		TSet<FGuid> Subjects;
		for (const Fdemo_mapShanmenFormationMembershipReceipt& Membership :
			Scope.Coverage.Memberships)
		{
			if (Membership.IsCovered())
			{
				Subjects.Add(Membership.SubjectEntityId);
			}
		}
		return Subjects;
	}

	FGuid MakeReconciliationId(
		const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch)
	{
		if (!Batch.SourceEntityId.IsValid() || !HasModeShape(Batch)
			|| !HasCompatibleAuthority(Batch)
			|| (Batch.Previous.IsSet() && !Batch.Previous->IsValid())
			|| (Batch.Current.IsSet() && !Batch.Current->IsValid()))
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			FString::FromInt(static_cast<int32>(Batch.Mode)),
			GuidDigits(Batch.SourceEntityId),
			Batch.Previous.IsSet() ? TEXT("Previous") : TEXT("NoPrevious")
		};
		if (Batch.Previous.IsSet())
		{
			AddScopeIdentity(Parts, Batch.Previous.GetValue());
		}
		Parts.Add(Batch.Current.IsSet() ? TEXT("Current") : TEXT("NoCurrent"));
		if (Batch.Current.IsSet())
		{
			AddScopeIdentity(Parts, Batch.Current.GetValue());
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceReconciliationEvidence.r1"),
			Parts);
	}

	void AddIntents(
		const Fdemo_mapShanmenFormationInfluenceScope& Scope,
		const FGuid& SourceEntityId,
		const FGuid& CauseId,
		const Edemo_mapShanmenFormationInfluenceOperation Operation,
		const TSet<FGuid>* ExcludedSubjects,
		TArray<Fdemo_mapShanmenFormationInfluenceIntent>& OutIntents)
	{
		for (const Fdemo_mapShanmenFormationMembershipReceipt& Membership :
			Scope.Coverage.Memberships)
		{
			if (!Membership.IsCovered()
				|| (ExcludedSubjects
					&& ExcludedSubjects->Contains(Membership.SubjectEntityId)))
			{
				continue;
			}
			OutIntents.Add(
				Fdemo_mapShanmenFormationInfluenceIntent::Make(
					Scope.Area, SourceEntityId, Scope.Policy,
					Membership.SubjectEntityId, Operation, CauseId));
		}
	}

	bool BuildExpectedIntents(
		const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch,
		TArray<Fdemo_mapShanmenFormationInfluenceIntent>& OutIntents)
	{
		OutIntents.Reset();
		if (!Batch.ReconciliationId.IsValid() || !HasModeShape(Batch))
		{
			return false;
		}
		switch (Batch.Mode)
		{
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Prime:
			AddIntents(
				Batch.Current.GetValue(), Batch.SourceEntityId,
				Batch.ReconciliationId,
				Edemo_mapShanmenFormationInfluenceOperation::Apply,
				nullptr, OutIntents);
			break;
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Rebase:
		{
			const auto& Previous = Batch.Previous.GetValue();
			const auto& Current = Batch.Current.GetValue();
			if (HasSameEffectScope(Previous, Current))
			{
				const TSet<FGuid> PreviousCovered = CoveredSubjects(Previous);
				const TSet<FGuid> CurrentCovered = CoveredSubjects(Current);
				AddIntents(
					Previous, Batch.SourceEntityId, Batch.ReconciliationId,
					Edemo_mapShanmenFormationInfluenceOperation::Remove,
					&CurrentCovered, OutIntents);
				AddIntents(
					Current, Batch.SourceEntityId, Batch.ReconciliationId,
					Edemo_mapShanmenFormationInfluenceOperation::Apply,
					&PreviousCovered, OutIntents);
			}
			else
			{
				AddIntents(
					Previous, Batch.SourceEntityId, Batch.ReconciliationId,
					Edemo_mapShanmenFormationInfluenceOperation::Remove,
					nullptr, OutIntents);
				AddIntents(
					Current, Batch.SourceEntityId, Batch.ReconciliationId,
					Edemo_mapShanmenFormationInfluenceOperation::Apply,
					nullptr, OutIntents);
			}
			break;
		}
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset:
		case Edemo_mapShanmenFormationInfluenceReconciliationMode::Terminal:
			AddIntents(
				Batch.Previous.GetValue(), Batch.SourceEntityId,
				Batch.ReconciliationId,
				Edemo_mapShanmenFormationInfluenceOperation::Remove,
				nullptr, OutIntents);
			break;
		default:
			return false;
		}
		for (const Fdemo_mapShanmenFormationInfluenceIntent& Intent : OutIntents)
		{
			if (!Intent.IsValid())
			{
				return false;
			}
		}
		return true;
	}

	bool IntentMatches(
		const Fdemo_mapShanmenFormationInfluenceIntent& Left,
		const Fdemo_mapShanmenFormationInfluenceIntent& Right)
	{
		return Left.IntentId == Right.IntentId && Left.RunId == Right.RunId
			&& Left.OwnerId == Right.OwnerId
			&& Left.SourceEntityId == Right.SourceEntityId
			&& Left.DeploymentId == Right.DeploymentId
			&& Left.AreaId == Right.AreaId
			&& Left.SubjectEntityId == Right.SubjectEntityId
			&& Left.PolicyDefinitionId == Right.PolicyDefinitionId
			&& Left.InfluenceDefinitionId == Right.InfluenceDefinitionId
			&& Left.Operation == Right.Operation
			&& Left.CauseId == Right.CauseId
			&& ContentMatches(Left.Content, Right.Content);
	}

	FGuid MakeBatchId(
		const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch)
	{
		if (!Batch.ReconciliationId.IsValid() || Batch.ApplyCount < 0
			|| Batch.RemoveCount < 0)
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Batch.ReconciliationId),
			FString::FromInt(Batch.RemoveCount),
			FString::FromInt(Batch.ApplyCount),
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
			TEXT("Shanmen.Formation.InfluenceReconciliationBatch.r1"), Parts);
	}

	Fdemo_mapShanmenFormationInfluenceReconciliationResult Reject(
		const Edemo_mapShanmenFormationInfluenceReconciliationStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceReconciliationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceReconciliationResult Plan(
		const Edemo_mapShanmenFormationInfluenceReconciliationMode Mode,
		const FGuid& SourceEntityId,
		const TOptional<Fdemo_mapShanmenFormationInfluenceScope>& Previous,
		const TOptional<Fdemo_mapShanmenFormationInfluenceScope>& Current)
	{
		if (!SourceEntityId.IsValid())
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::SourceInvalid,
				TEXT("Influence reconciliation requires one stable source EntityId."));
		}
		if (Previous.IsSet() && !Previous->IsValid())
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::PreviousScopeInvalid,
				TEXT("Previous influence scope is invalid or internally mismatched."));
		}
		if (Current.IsSet() && !Current->IsValid())
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::CurrentScopeInvalid,
				TEXT("Current influence scope is invalid or internally mismatched."));
		}
		Fdemo_mapShanmenFormationInfluenceReconciliationBatch Shape;
		Shape.Mode = Mode;
		Shape.Previous = Previous;
		Shape.Current = Current;
		if (!HasCompatibleAuthority(Shape))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::ScopeMismatch,
				TEXT("Rebase cannot cross Run, owner, or deployment authority."));
		}

		Fdemo_mapShanmenFormationInfluenceReconciliationResult Result;
		Result.Batch.Mode = Mode;
		Result.Batch.SourceEntityId = SourceEntityId;
		Result.Batch.Previous = Previous;
		Result.Batch.Current = Current;
		if (!HasModeShape(Result.Batch))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::ModeInvalid,
				TEXT("Reconciliation mode does not match its old/new scope shape."));
		}
		Result.Batch.ReconciliationId = MakeReconciliationId(Result.Batch);
		if (!BuildExpectedIntents(Result.Batch, Result.Batch.Intents))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::ReceiptRejected,
				TEXT("Influence reconciliation intents could not be derived."));
		}
		for (const Fdemo_mapShanmenFormationInfluenceIntent& Intent :
			Result.Batch.Intents)
		{
			if (Intent.Operation
				== Edemo_mapShanmenFormationInfluenceOperation::Apply)
			{
				++Result.Batch.ApplyCount;
			}
			else
			{
				++Result.Batch.RemoveCount;
			}
		}
		Result.Batch.BatchId = MakeBatchId(Result.Batch);
		if (!Result.Batch.IsValid())
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceReconciliationStatus::ReceiptRejected,
				TEXT("Lifecycle reconciliation failed deterministic self-validation."));
		}
		Result.Status =
			Edemo_mapShanmenFormationInfluenceReconciliationStatus::Planned;
		Result.Diagnostic = Result.Batch.IsNoOp()
			? TEXT("Lifecycle evidence produced a sealed no-op reconciliation.")
			: TEXT("Lifecycle evidence produced canonical influence reconciliation intents.");
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceScope::IsValid() const
{
	return Area.IsValid() && Policy.IsValid() && Coverage.IsValid()
		&& Coverage.AreaId == Area.AreaId
		&& ContentMatches(Area.Content, Policy.Content);
}

bool Fdemo_mapShanmenFormationInfluenceReconciliationBatch::IsValid() const
{
	if (!BatchId.IsValid() || !SourceEntityId.IsValid() || !HasModeShape(*this)
		|| !HasCompatibleAuthority(*this)
		|| (Previous.IsSet() && !Previous->IsValid())
		|| (Current.IsSet() && !Current->IsValid())
		|| ReconciliationId != MakeReconciliationId(*this)
		|| ApplyCount < 0 || RemoveCount < 0
		|| Intents.Num() != ApplyCount + RemoveCount)
	{
		return false;
	}
	TArray<Fdemo_mapShanmenFormationInfluenceIntent> Expected;
	if (!BuildExpectedIntents(*this, Expected)
		|| Expected.Num() != Intents.Num())
	{
		return false;
	}
	int32 ExpectedApply = 0;
	int32 ExpectedRemove = 0;
	TSet<FGuid> IntentIds;
	for (int32 Index = 0; Index < Expected.Num(); ++Index)
	{
		if (!IntentMatches(Intents[Index], Expected[Index])
			|| IntentIds.Contains(Intents[Index].IntentId))
		{
			return false;
		}
		IntentIds.Add(Intents[Index].IntentId);
		if (Intents[Index].Operation
			== Edemo_mapShanmenFormationInfluenceOperation::Apply)
		{
			++ExpectedApply;
		}
		else
		{
			++ExpectedRemove;
		}
	}
	return ApplyCount == ExpectedApply && RemoveCount == ExpectedRemove
		&& BatchId == MakeBatchId(*this);
}

bool Fdemo_mapShanmenFormationInfluenceReconciliationResult::IsSuccess() const
{
	return Status
		== Edemo_mapShanmenFormationInfluenceReconciliationStatus::Planned
		&& Batch.IsValid();
}

Fdemo_mapShanmenFormationInfluenceReconciliationResult
Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanPrime(
	const FGuid& SourceEntityId,
	const Fdemo_mapShanmenFormationInfluenceScope& Current)
{
	return Plan(
		Edemo_mapShanmenFormationInfluenceReconciliationMode::Prime,
		SourceEntityId, {}, Current);
}

Fdemo_mapShanmenFormationInfluenceReconciliationResult
Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanRebase(
	const FGuid& SourceEntityId,
	const Fdemo_mapShanmenFormationInfluenceScope& Previous,
	const Fdemo_mapShanmenFormationInfluenceScope& Current)
{
	return Plan(
		Edemo_mapShanmenFormationInfluenceReconciliationMode::Rebase,
		SourceEntityId, Previous, Current);
}

Fdemo_mapShanmenFormationInfluenceReconciliationResult
Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
	const Edemo_mapShanmenFormationInfluenceReconciliationMode Mode,
	const FGuid& SourceEntityId,
	const Fdemo_mapShanmenFormationInfluenceScope& Previous)
{
	if (Mode != Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset
		&& Mode
			!= Edemo_mapShanmenFormationInfluenceReconciliationMode::Terminal)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceReconciliationStatus::ModeInvalid,
			TEXT("Clear planning only accepts Reset or Terminal mode."));
	}
	return Plan(Mode, SourceEntityId, Previous, {});
}
