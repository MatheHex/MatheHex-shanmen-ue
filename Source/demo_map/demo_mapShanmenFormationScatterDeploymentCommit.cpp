#include "demo_mapShanmenFormationScatterDeploymentCommit.h"

#include "ShanmenActionOrchestrator.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenFormationScatterDeploymentCommitStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& SameContent(Left.GetContent(), Right.GetContent())
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool SameMaterialLine(
		const FShanmenFormationMaterialFulfillmentLine& Left,
		const FShanmenFormationMaterialFulfillmentLine& Right)
	{
		return Left.ItemInstanceId == Right.ItemInstanceId
			&& Left.MaterialDefinitionId == Right.MaterialDefinitionId
			&& Left.Quantity == Right.Quantity;
	}

	bool SameDeploymentEvidence(
		const FShanmenFormationAnchorFulfillmentEvidence& Left,
		const FShanmenFormationAnchorFulfillmentEvidence& Right)
	{
		if (Left.FulfillmentId != Right.FulfillmentId
			|| Left.RunId != Right.RunId
			|| Left.OwnerId != Right.OwnerId
			|| Left.DeploymentId != Right.DeploymentId
			|| Left.AnchorDefinitionId != Right.AnchorDefinitionId
			|| !SameContent(Left.Content, Right.Content)
			|| Left.AuthorityRevision != Right.AuthorityRevision
			|| Left.Lines.Num() != Right.Lines.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Lines.Num(); ++Index)
		{
			if (!SameMaterialLine(Left.Lines[Index], Right.Lines[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool SameDeploymentReceipt(
		const FShanmenFormationDeploymentReceipt& Left,
		const FShanmenFormationDeploymentReceipt& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetReceiptId() == Right.GetReceiptId()
			&& Left.GetDeploymentId() == Right.GetDeploymentId()
			&& Left.GetSequence() == Right.GetSequence()
			&& Left.GetEventKind() == Right.GetEventKind()
			&& Left.GetStateBefore() == Right.GetStateBefore()
			&& Left.GetStateAfter() == Right.GetStateAfter()
			&& Left.GetAnchorDefinitionId()
				== Right.GetAnchorDefinitionId()
			&& Left.GetFulfillmentId() == Right.GetFulfillmentId()
			&& Left.GetAuthorityRevision()
				== Right.GetAuthorityRevision()
			&& Left.GetCommittedAnchorCount()
				== Right.GetCommittedAnchorCount();
	}

	const FShanmenItemTransactionReceipt* FindResourceCommitReceipt(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			Evidence,
		const FGuid& ReceiptId)
	{
		return ReceiptId.IsValid()
			? Evidence.GetCommitReceipts().FindByPredicate(
				[&ReceiptId](
					const FShanmenItemTransactionReceipt& Candidate)
				{
					return Candidate.ReceiptId == ReceiptId;
				})
			: nullptr;
	}

	const FShanmenFormationDeploymentReceipt* FindAnchorCommitReceipt(
		const FShanmenFormationDeployment& Deployment,
		const FName AnchorDefinitionId)
	{
		return Deployment.GetReceipts().FindByPredicate(
			[AnchorDefinitionId](
				const FShanmenFormationDeploymentReceipt& Candidate)
			{
				return Candidate.GetEventKind()
						== EShanmenFormationDeploymentEventKind::CommitAnchor
					&& Candidate.GetAnchorDefinitionId()
						== AnchorDefinitionId;
			});
	}

	Fdemo_mapShanmenFormationScatterDeploymentCommitResult Reject(
		const EStatus Status,
		const FString& Diagnostic,
		const int32 InitialCommittedAnchorCount = INDEX_NONE)
	{
		Fdemo_mapShanmenFormationScatterDeploymentCommitResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.InitialCommittedAnchorCount = InitialCommittedAnchorCount;
		return Result;
	}

	bool MatchesResourceIdentity(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			ResourceEvidence,
		const FShanmenFormationDeployment& Deployment)
	{
		const auto& Plan = ResourceEvidence.GetPlan();
		const auto& Authorization =
			Plan.GetBatch().GetAuthorization();
		const auto& Action = Deployment.GetAction();
		return Deployment.GetDeploymentId()
				== Authorization.GetDeploymentId()
			&& Action.GetRunId() == Plan.GetActiveRunId()
			&& Action.GetRunId() == Authorization.GetRunId()
			&& Action.GetOwnerId() == Plan.GetOwnerId()
			&& Action.GetOwnerId() == Authorization.GetOwnerId()
			&& Action.GetActivationId()
				== Authorization.GetActivationId()
			&& Deployment.GetDiagram().GetDiagramDefinitionId()
				== Authorization.GetDiagramDefinitionId()
			&& SameContent(Action.GetContent(), Plan.GetContent())
			&& SameContent(Action.GetContent(), Authorization.GetContent());
	}

	bool MatchesAnchorShape(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			ResourceEvidence,
		const FShanmenFormationDeployment& Deployment)
	{
		const auto& ResourceAnchors =
			ResourceEvidence.GetAnchorFulfillments();
		const auto& BatchAnchors =
			ResourceEvidence.GetPlan().GetBatch().GetAnchorIntents();
		const auto& DeploymentAnchors = Deployment.GetAnchors();
		if (ResourceAnchors.Num() != BatchAnchors.Num()
			|| ResourceAnchors.Num() != DeploymentAnchors.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < ResourceAnchors.Num(); ++Index)
		{
			const auto& Resource = ResourceAnchors[Index];
			const auto& Batch = BatchAnchors[Index];
			const auto& Runtime = DeploymentAnchors[Index];
			if (Resource.GetAnchorOrder() != Index
				|| Batch.GetAnchorOrder() != Index
				|| Resource.GetAnchorIntentId() != Batch.GetIntentId()
				|| Resource.GetAnchorDefinitionId()
					!= Batch.GetAnchorDefinitionId()
				|| Resource.GetAnchorInstanceId()
					!= Batch.GetAnchorInstanceId()
				|| Runtime.GetAnchorDefinitionId()
					!= Resource.GetAnchorDefinitionId()
				|| Runtime.GetAnchorInstanceId()
					!= Resource.GetAnchorInstanceId()
				|| Runtime.GetWorldLocation() != Batch.GetWorldLocation())
			{
				return false;
			}
		}
		return true;
	}

	bool MatchesCanonicalCommittedPrefix(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			ResourceEvidence,
		const FShanmenFormationDeployment& Deployment)
	{
		const int32 CommittedCount = Deployment.GetCommittedAnchorCount();
		const auto& Anchors = Deployment.GetAnchors();
		if (CommittedCount < 0 || CommittedCount > Anchors.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Anchors.Num(); ++Index)
		{
			const bool bShouldBeCommitted = Index < CommittedCount;
			if (Anchors[Index].IsCommitted() != bShouldBeCommitted)
			{
				return false;
			}
			if (!bShouldBeCommitted)
			{
				continue;
			}

			FShanmenFormationAnchorFulfillmentEvidence Expected;
			const FShanmenFormationDeploymentReceipt* Receipt =
				FindAnchorCommitReceipt(
					Deployment, Anchors[Index].GetAnchorDefinitionId());
			if (!Fdemo_mapShanmenFormationScatterDeploymentCommitter::
					BuildAnchorEvidence(ResourceEvidence, Index, Expected)
				|| !SameDeploymentEvidence(
					Anchors[Index].GetFulfillment(), Expected)
				|| !Receipt
				|| Receipt->GetSequence() != Index + 1
				|| Receipt->GetCommittedAnchorCount() != Index + 1
				|| Receipt->GetFulfillmentId() != Expected.FulfillmentId
				|| Receipt->GetAuthorityRevision()
					!= Expected.AuthorityRevision)
			{
				return false;
			}
		}
		return true;
	}
}

FGuid Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff::
BuildHandoffId(
	const Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff& Handoff)
{
	if (!Handoff.ResourceEvidenceId.IsValid()
		|| !Handoff.ResourceFulfillment.IsValid()
		|| !Handoff.DeploymentEvidence.IsValid()
		|| !Handoff.DeploymentReceipt.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterAnchorDeploymentHandoff.r1"),
		{
			GuidDigits(Handoff.ResourceEvidenceId),
			GuidDigits(Handoff.ResourceFulfillment.GetFulfillmentId()),
			GuidDigits(Handoff.DeploymentReceipt.GetReceiptId()),
			FString::FromInt(
				Handoff.ResourceFulfillment.GetAnchorOrder())
		});
}

bool Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff::IsValid()
	const
{
	return HandoffId.IsValid()
		&& ResourceEvidenceId.IsValid()
		&& ResourceFulfillment.IsValid()
		&& DeploymentEvidence.IsValid()
		&& DeploymentReceipt.IsValid()
		&& DeploymentReceipt.GetEventKind()
			== EShanmenFormationDeploymentEventKind::CommitAnchor
		&& ResourceFulfillment.GetFulfillmentId()
			== DeploymentEvidence.FulfillmentId
		&& ResourceFulfillment.GetAnchorDefinitionId()
			== DeploymentEvidence.AnchorDefinitionId
		&& DeploymentReceipt.GetDeploymentId()
			== DeploymentEvidence.DeploymentId
		&& DeploymentReceipt.GetAnchorDefinitionId()
			== DeploymentEvidence.AnchorDefinitionId
		&& DeploymentReceipt.GetFulfillmentId()
			== DeploymentEvidence.FulfillmentId
		&& DeploymentReceipt.GetAuthorityRevision()
			== DeploymentEvidence.AuthorityRevision
		&& DeploymentReceipt.GetSequence()
			== ResourceFulfillment.GetAnchorOrder() + 1
		&& HandoffId == BuildHandoffId(*this);
}

bool Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff::operator==(
	const Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff& Other)
	const
{
	return HandoffId == Other.HandoffId
		&& ResourceEvidenceId == Other.ResourceEvidenceId
		&& ResourceFulfillment == Other.ResourceFulfillment
		&& SameDeploymentEvidence(
			DeploymentEvidence, Other.DeploymentEvidence)
		&& SameDeploymentReceipt(
			DeploymentReceipt, Other.DeploymentReceipt);
}

FGuid Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence::
BuildEvidenceId(
	const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence& Evidence)
{
	if (!Evidence.ResourceEvidence.IsValid()
		|| !Evidence.Deployment.IsValid()
		|| Evidence.Handoffs.IsEmpty()
		|| Evidence.TotalCommittedQuantity <= 0)
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Evidence.ResourceEvidence.GetEvidenceId()),
		GuidDigits(Evidence.Deployment.GetDeploymentId()),
		GuidDigits(Evidence.Deployment.GetAction().GetActivationId()),
		FString::FromInt(Evidence.Handoffs.Num()),
		FString::Printf(TEXT("%lld"), Evidence.TotalCommittedQuantity)
	};
	for (const auto& Handoff : Evidence.Handoffs)
	{
		if (!Handoff.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Handoff.GetHandoffId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterDeploymentCommitEvidence.r1"),
		Parts);
}

bool Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence::IsValid()
	const
{
	if (!EvidenceId.IsValid()
		|| !ResourceEvidence.IsValid()
		|| !Deployment.IsValid()
		|| Deployment.GetState()
			!= EShanmenFormationDeploymentState::Active
		|| !MatchesResourceIdentity(ResourceEvidence, Deployment)
		|| !MatchesAnchorShape(ResourceEvidence, Deployment)
		|| !MatchesCanonicalCommittedPrefix(ResourceEvidence, Deployment)
		|| Handoffs.Num() != Deployment.GetAnchors().Num()
		|| Handoffs.Num()
			!= ResourceEvidence.GetAnchorFulfillments().Num()
		|| TotalCommittedQuantity
			!= ResourceEvidence.GetTotalCommittedQuantity())
	{
		return false;
	}

	const auto& Receipts = Deployment.GetReceipts();
	if (Receipts.Num() != Handoffs.Num() + 1
		|| !Receipts[0].IsValid()
		|| Receipts[0].GetSequence() != 0
		|| Receipts[0].GetEventKind()
			!= EShanmenFormationDeploymentEventKind::Begin)
	{
		return false;
	}

	int64 CountedQuantity = 0;
	for (int32 Index = 0; Index < Handoffs.Num(); ++Index)
	{
		const auto& Handoff = Handoffs[Index];
		const auto& Resource =
			ResourceEvidence.GetAnchorFulfillments()[Index];
		FShanmenFormationAnchorFulfillmentEvidence Expected;
		if (!Handoff.IsValid()
			|| !(Handoff.GetResourceFulfillment() == Resource)
			|| !Fdemo_mapShanmenFormationScatterDeploymentCommitter::
				BuildAnchorEvidence(ResourceEvidence, Index, Expected)
			|| !SameDeploymentEvidence(
				Handoff.GetDeploymentEvidence(), Expected)
			|| !SameDeploymentReceipt(
				Handoff.GetDeploymentReceipt(), Receipts[Index + 1])
			|| Receipts[Index + 1].GetCommittedAnchorCount() != Index + 1
			|| Receipts[Index + 1].GetStateAfter()
				!= (Index + 1 == Handoffs.Num()
					? EShanmenFormationDeploymentState::Active
					: EShanmenFormationDeploymentState::Deploying))
		{
			return false;
		}
		CountedQuantity += Resource.GetTotalCommittedQuantity();
	}
	return CountedQuantity == TotalCommittedQuantity
		&& EvidenceId == BuildEvidenceId(*this);
}

bool Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence::operator==(
	const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence& Other)
	const
{
	return EvidenceId == Other.EvidenceId
		&& ResourceEvidence == Other.ResourceEvidence
		&& Handoffs == Other.Handoffs
		&& TotalCommittedQuantity == Other.TotalCommittedQuantity;
}

bool Fdemo_mapShanmenFormationScatterDeploymentCommitResult::IsValid() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status != EStatus::Committed && Status != EStatus::Replayed)
	{
		return !Evidence.IsValid() && NewCommitReceipts.IsEmpty();
	}
	if (!Evidence.IsValid())
	{
		return false;
	}
	const int32 AnchorCount = Evidence.GetHandoffs().Num();
	if (InitialCommittedAnchorCount < 0
		|| InitialCommittedAnchorCount > AnchorCount)
	{
		return false;
	}
	if (Status == EStatus::Replayed)
	{
		return InitialCommittedAnchorCount == AnchorCount
			&& NewCommitReceipts.IsEmpty();
	}
	if (NewCommitReceipts.Num()
		!= AnchorCount - InitialCommittedAnchorCount
		|| NewCommitReceipts.IsEmpty())
	{
		return false;
	}
	const auto& Receipts = Evidence.GetDeployment().GetReceipts();
	for (int32 Index = 0; Index < NewCommitReceipts.Num(); ++Index)
	{
		if (!SameDeploymentReceipt(
				NewCommitReceipts[Index],
				Receipts[InitialCommittedAnchorCount + Index + 1]))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterDeploymentCommitResult::IsCommitted()
	const
{
	return Status == EStatus::Committed || Status == EStatus::Replayed;
}

bool Fdemo_mapShanmenFormationScatterDeploymentCommitter::
BuildAnchorEvidence(
	const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
		ResourceEvidence,
	const int32 AnchorIndex,
	FShanmenFormationAnchorFulfillmentEvidence& OutEvidence)
{
	OutEvidence = FShanmenFormationAnchorFulfillmentEvidence();
	if (!ResourceEvidence.IsValid()
		|| !ResourceEvidence.GetAnchorFulfillments().IsValidIndex(
			AnchorIndex))
	{
		return false;
	}

	const auto& Plan = ResourceEvidence.GetPlan();
	const auto& Resource =
		ResourceEvidence.GetAnchorFulfillments()[AnchorIndex];
	if (Resource.GetAnchorOrder() != AnchorIndex)
	{
		return false;
	}

	OutEvidence.FulfillmentId = Resource.GetFulfillmentId();
	OutEvidence.RunId = Plan.GetActiveRunId();
	OutEvidence.OwnerId = Plan.GetOwnerId();
	OutEvidence.DeploymentId =
		Plan.GetBatch().GetAuthorization().GetDeploymentId();
	OutEvidence.AnchorDefinitionId = Resource.GetAnchorDefinitionId();
	OutEvidence.Content = Plan.GetContent();
	OutEvidence.AuthorityRevision = INDEX_NONE;

	for (const auto& SourceLine : Resource.GetLines())
	{
		const FShanmenItemTransactionReceipt* CommitReceipt =
			FindResourceCommitReceipt(
				ResourceEvidence, SourceLine.GetCommitReceiptId());
		if (!CommitReceipt || !CommitReceipt->IsSuccess()
			|| CommitReceipt->Phase
				!= EShanmenItemTransactionPhase::Committed)
		{
			OutEvidence =
				FShanmenFormationAnchorFulfillmentEvidence();
			return false;
		}
		OutEvidence.AuthorityRevision = FMath::Max(
			OutEvidence.AuthorityRevision,
			CommitReceipt->AuthorityRevision);

		FShanmenFormationMaterialFulfillmentLine* Existing =
			OutEvidence.Lines.FindByPredicate(
				[&SourceLine](
					FShanmenFormationMaterialFulfillmentLine& Candidate)
				{
					return Candidate.ItemInstanceId
						== SourceLine.GetItemInstanceId();
				});
		if (Existing)
		{
			const int64 Combined =
				static_cast<int64>(Existing->Quantity)
				+ SourceLine.GetQuantity();
			if (Existing->MaterialDefinitionId
					!= SourceLine.GetMaterialDefinitionId()
				|| Combined > MAX_int32)
			{
				OutEvidence =
					FShanmenFormationAnchorFulfillmentEvidence();
				return false;
			}
			Existing->Quantity = static_cast<int32>(Combined);
			continue;
		}

		FShanmenFormationMaterialFulfillmentLine& Line =
			OutEvidence.Lines.AddDefaulted_GetRef();
		Line.ItemInstanceId = SourceLine.GetItemInstanceId();
		Line.MaterialDefinitionId =
			SourceLine.GetMaterialDefinitionId();
		Line.Quantity = SourceLine.GetQuantity();
	}
	if (!OutEvidence.IsValid())
	{
		OutEvidence = FShanmenFormationAnchorFulfillmentEvidence();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterDeploymentCommitter::
BuildCompletionEvidence(
	const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
		ResourceEvidence,
	const FShanmenFormationDeployment& Deployment,
	Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence& OutEvidence)
{
	OutEvidence =
		Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence();
	if (!ResourceEvidence.IsValid()
		|| !Deployment.IsValid()
		|| Deployment.GetState()
			!= EShanmenFormationDeploymentState::Active)
	{
		return false;
	}

	OutEvidence.ResourceEvidence = ResourceEvidence;
	OutEvidence.Deployment = Deployment;
	OutEvidence.TotalCommittedQuantity =
		ResourceEvidence.GetTotalCommittedQuantity();
	for (int32 Index = 0;
		Index < ResourceEvidence.GetAnchorFulfillments().Num(); ++Index)
	{
		Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff Handoff;
		Handoff.ResourceEvidenceId = ResourceEvidence.GetEvidenceId();
		Handoff.ResourceFulfillment =
			ResourceEvidence.GetAnchorFulfillments()[Index];
		if (!BuildAnchorEvidence(
				ResourceEvidence, Index, Handoff.DeploymentEvidence))
		{
			return false;
		}
		const FShanmenFormationDeploymentReceipt* Receipt =
			FindAnchorCommitReceipt(
				Deployment,
				Handoff.ResourceFulfillment.GetAnchorDefinitionId());
		if (!Receipt)
		{
			return false;
		}
		Handoff.DeploymentReceipt = *Receipt;
		Handoff.HandoffId =
			Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff::
				BuildHandoffId(Handoff);
		if (!Handoff.IsValid())
		{
			return false;
		}
		OutEvidence.Handoffs.Add(Handoff);
	}
	OutEvidence.EvidenceId =
		Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence::
			BuildEvidenceId(OutEvidence);
	return OutEvidence.IsValid();
}

Fdemo_mapShanmenFormationScatterDeploymentCommitResult
Fdemo_mapShanmenFormationScatterDeploymentCommitter::Commit(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
		ResourceEvidence)
{
	if (!ResourceEvidence.IsValid())
	{
		return Reject(
			EStatus::ResourceEvidenceInvalid,
			TEXT("Scatter deployment commit requires complete P27.20 resource evidence."));
	}
	if (!ActionRuntime.IsValid() || ActionRuntime.IsTerminal()
		|| !ActionRuntime.CanEmitCandidates())
	{
		return Reject(
			EStatus::ActionRuntimeInvalid,
			TEXT("Scatter deployment commit requires the matching active action runtime."));
	}
	if (!Deployment.IsValid())
	{
		return Reject(
			EStatus::DeploymentInvalid,
			TEXT("Scatter deployment commit requires a valid deployment snapshot."));
	}
	const int32 InitialCommitted = Deployment.GetCommittedAnchorCount();
	if (!ActionsMatch(ActionRuntime.GetAction(), Deployment.GetAction())
		|| !MatchesResourceIdentity(ResourceEvidence, Deployment))
	{
		return Reject(
			EStatus::IdentityMismatch,
			TEXT("Resource, action and deployment identities must describe the same activation."),
			InitialCommitted);
	}
	if (Deployment.GetState()
			!= EShanmenFormationDeploymentState::Deploying
		&& Deployment.GetState()
			!= EShanmenFormationDeploymentState::Active)
	{
		return Reject(
			EStatus::DeploymentTerminal,
			TEXT("A cancelled or ended deployment cannot accept committed scatter resources."),
			InitialCommitted);
	}
	if (!MatchesAnchorShape(ResourceEvidence, Deployment))
	{
		return Reject(
			EStatus::AnchorShapeInvalid,
			TEXT("Resource fulfillments do not exactly cover the deployment anchor shape."),
			InitialCommitted);
	}
	if (!MatchesCanonicalCommittedPrefix(ResourceEvidence, Deployment))
	{
		return Reject(
			EStatus::ExistingAnchorConflict,
			TEXT("Existing deployment commits are not the exact canonical resource prefix."),
			InitialCommitted);
	}

	FShanmenFormationDeployment Candidate = Deployment;
	TArray<FShanmenFormationDeploymentReceipt> NewReceipts;
	for (int32 Index = InitialCommitted;
		Index < ResourceEvidence.GetAnchorFulfillments().Num(); ++Index)
	{
		FShanmenFormationAnchorFulfillmentEvidence Evidence;
		FShanmenFormationDeploymentReceipt Receipt;
		if (!BuildAnchorEvidence(ResourceEvidence, Index, Evidence))
		{
			return Reject(
				EStatus::AnchorShapeInvalid,
				TEXT("One resource fulfillment cannot form deployment evidence."),
				InitialCommitted);
		}
		if (!Candidate.TryCommitAnchor(ActionRuntime, Evidence, Receipt))
		{
			return Reject(
				EStatus::AnchorCommitRejected,
				TEXT("The deployment kernel rejected one canonical scatter anchor."),
				InitialCommitted);
		}
		NewReceipts.Add(Receipt);
	}

	Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence Completion;
	if (!BuildCompletionEvidence(
			ResourceEvidence, Candidate, Completion))
	{
		return Reject(
			EStatus::CompletionEvidenceInvalid,
			TEXT("The candidate deployment did not produce complete self-validating evidence."),
			InitialCommitted);
	}

	Fdemo_mapShanmenFormationScatterDeploymentCommitResult Result;
	Result.Status = NewReceipts.IsEmpty()
		? EStatus::Replayed
		: EStatus::Committed;
	Result.Diagnostic = NewReceipts.IsEmpty()
		? TEXT("Every scatter anchor already replays from the exact committed deployment evidence.")
		: TEXT("Every committed scatter resource fulfillment is published to its canonical deployment anchor.");
	Result.InitialCommittedAnchorCount = InitialCommitted;
	Result.NewCommitReceipts = NewReceipts;
	Result.Evidence = Completion;
	if (!Result.IsValid())
	{
		return Reject(
			EStatus::CompletionEvidenceInvalid,
			TEXT("Scatter deployment commit result failed its final invariant check."),
			InitialCommitted);
	}
	Deployment = Candidate;
	return Result;
}
