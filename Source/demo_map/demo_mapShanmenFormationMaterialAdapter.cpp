#include "demo_mapShanmenFormationMaterialAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	FName FormationMaterialPurpose()
	{
		return TEXT("Shanmen.Formation.AnchorMaterial.r1");
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	template <typename TValue, typename TPredicate>
	const TValue* FindBy(const TArray<TValue>& Values, TPredicate Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	Fdemo_mapShanmenFormationMaterialResult Reject(
		const Edemo_mapShanmenFormationMaterialStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationMaterialResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	FGuid MakeTransactionId(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenFormationDeployment& Deployment,
		const FName AnchorDefinitionId,
		const FGuid& AttemptId)
	{
		if (!Correlation.IsValid() || !Deployment.IsValid()
			|| AnchorDefinitionId.IsNone() || !AttemptId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.Formation.MaterialTransaction.r1"),
			{
				GuidDigits(Correlation.CorrelationId),
				GuidDigits(Correlation.ActiveRunId),
				GuidDigits(Deployment.GetDeploymentId()),
				AnchorDefinitionId.ToString(),
				GuidDigits(AttemptId),
				Deployment.GetAction().GetContent().Version.ToString(),
				Deployment.GetAction().GetContent().Digest
			});
	}

	FGuid MakeIntentId(
		const FGuid& TransactionId,
		const FGuid& ItemInstanceId)
	{
		if (!TransactionId.IsValid() || !ItemInstanceId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.Formation.MaterialLineIntent.r1"),
			{ GuidDigits(TransactionId), GuidDigits(ItemInstanceId) });
	}

	FGuid MakePrepareRequestId(
		const FGuid& TransactionId,
		const FGuid& ItemInstanceId,
		const int32 Quantity,
		const int32 ExpectedQuantityBefore)
	{
		if (!TransactionId.IsValid() || !ItemInstanceId.IsValid()
			|| Quantity <= 0 || ExpectedQuantityBefore < Quantity)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.Formation.MaterialPrepare.r1"),
			{
				GuidDigits(TransactionId),
				GuidDigits(ItemInstanceId),
				FString::FromInt(Quantity),
				FString::FromInt(ExpectedQuantityBefore)
			});
	}

	FGuid MakeFinalizeRequestId(
		const FGuid& TransactionId,
		const FGuid& PrepareRequestId,
		const FGuid& ItemInstanceId)
	{
		if (!TransactionId.IsValid() || !PrepareRequestId.IsValid()
			|| !ItemInstanceId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.Formation.MaterialFinalize.r1"),
			{
				GuidDigits(TransactionId),
				GuidDigits(PrepareRequestId),
				GuidDigits(ItemInstanceId)
			});
	}

	bool IsPrepareReceiptForLine(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationMaterialPlanLine& Line,
		const FGuid& ActiveRunId)
	{
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
			&& Receipt.Phase == EShanmenItemTransactionPhase::Reserved
			&& Receipt.RequestId == Line.PrepareRequest.Context.RequestId
			&& Receipt.ReservationId == Line.IntentId
			&& Receipt.ItemInstanceId == Line.ItemInstanceId
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Line.Quantity
			&& Receipt.ResourceBefore == Line.ExpectedQuantityBefore
			&& Receipt.ResourceAfter == Line.ExpectedQuantityBefore
			&& Receipt.PurposeId == FormationMaterialPurpose()
			&& Receipt.ReservationIds == TArray<FGuid>({ ActiveRunId });
	}

	bool IsFinalizeReceiptForLine(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationMaterialPlanLine& Line,
		const FGuid& ActiveRunId,
		const FGuid& FinalizeRequestId,
		const EShanmenItemTransactionPhase Phase)
	{
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
			&& Receipt.Phase == Phase
			&& Receipt.RequestId == FinalizeRequestId
			&& Receipt.ReservationId == Line.IntentId
			&& Receipt.ItemInstanceId == Line.ItemInstanceId
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Line.Quantity
			&& Receipt.PurposeId == FormationMaterialPurpose()
			&& Receipt.ReservationIds
				== TArray<FGuid>({
					ActiveRunId,
					Line.PrepareRequest.Context.RequestId });
	}

	const FShanmenItemProcessedRequestSnapshot* FindLifecycle(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation)
	{
		return FindBy(
			Snapshot.ProcessedRequests,
			[&Correlation](
				const FShanmenItemProcessedRequestSnapshot& Processed)
			{
				return Processed.RequestId
					== Correlation.LifecycleRequestId;
			});
	}

	bool IsLifecycleValid(
		const FShanmenItemProcessedRequestSnapshot* Lifecycle,
		const Fdemo_mapShanmenRunCorrelation& Correlation)
	{
		if (!Lifecycle || !Lifecycle->Receipt.IsSuccess())
		{
			return false;
		}
		const FShanmenItemTransactionReceipt& Receipt = Lifecycle->Receipt;
		return (Receipt.Operation
				== EShanmenItemTransactionOperation::StartPreparedRun
			|| Receipt.Operation
				== EShanmenItemTransactionOperation::ClaimPreparedRun)
			&& Receipt.ReceiptId == Correlation.LifecycleReceiptId
			&& Receipt.ReservationId == Correlation.ActiveRunId
			&& Receipt.AuthorityRevision
				== Correlation.LifecycleAuthorityRevision;
	}

	const FShanmenItemReservationSnapshot* FindQuantityReservation(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemTransactionReceipt& Lifecycle,
		const FGuid& ItemInstanceId,
		bool& bDuplicate)
	{
		bDuplicate = false;
		const FShanmenItemReservationSnapshot* Found = nullptr;
		for (const FGuid& ReservationId : Lifecycle.ReservationIds)
		{
			const FShanmenItemReservationSnapshot* Candidate = FindBy(
				Snapshot.Reservations,
				[&ReservationId](
					const FShanmenItemReservationSnapshot& Value)
				{
					return Value.ReservationId == ReservationId;
				});
			if (!Candidate || Candidate->ItemInstanceId != ItemInstanceId
				|| Candidate->ResourceKind
					!= EShanmenItemResourceKind::Quantity)
			{
				continue;
			}
			if (Found)
			{
				bDuplicate = true;
				return nullptr;
			}
			Found = Candidate;
		}
		return Found;
	}

	int32 ConsumedQuantity(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ActiveRunId,
		const FGuid& ItemInstanceId)
	{
		int32 Consumed = 0;
		for (const FShanmenItemProcessedRequestSnapshot& Processed :
			Snapshot.ProcessedRequests)
		{
			const FShanmenItemTransactionReceipt& Receipt =
				Processed.Receipt;
			if (!Receipt.IsSuccess())
			{
				continue;
			}
			if (Receipt.Operation
					== EShanmenItemTransactionOperation::ConsumePreparedRunItem
				&& Receipt.ReservationId == ActiveRunId
				&& Receipt.ItemInstanceId == ItemInstanceId)
			{
				Consumed += Receipt.Amount;
			}
			else if (Receipt.Operation
					== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
				&& Receipt.Phase == EShanmenItemTransactionPhase::Committed
				&& Receipt.ItemInstanceId == ItemInstanceId
				&& Receipt.ReservationIds.Num() == 2
				&& Receipt.ReservationIds[0] == ActiveRunId)
			{
				Consumed += Receipt.Amount;
			}
		}
		return Consumed;
	}

	bool PlansMatch(
		const Fdemo_mapShanmenFormationMaterialResult& Left,
		const Fdemo_mapShanmenFormationMaterialResult& Right)
	{
		if (!Left.HasPlan() || !Right.HasPlan()
			|| Left.AttemptId != Right.AttemptId
			|| Left.TransactionId != Right.TransactionId
			|| Left.ScopeId != Right.ScopeId
			|| Left.OwnerId != Right.OwnerId
			|| Left.ActiveRunId != Right.ActiveRunId
			|| Left.DeploymentId != Right.DeploymentId
			|| Left.AnchorDefinitionId != Right.AnchorDefinitionId
			|| !SameContent(Left.Content, Right.Content)
			|| Left.Requirements.Num() != Right.Requirements.Num()
			|| Left.Lines.Num() != Right.Lines.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Requirements.Num(); ++Index)
		{
			if (Left.Requirements[Index].MaterialDefinitionId
					!= Right.Requirements[Index].MaterialDefinitionId
				|| Left.Requirements[Index].Quantity
					!= Right.Requirements[Index].Quantity)
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < Left.Lines.Num(); ++Index)
		{
			const Fdemo_mapShanmenFormationMaterialPlanLine& A =
				Left.Lines[Index];
			const Fdemo_mapShanmenFormationMaterialPlanLine& B =
				Right.Lines[Index];
			if (A.MaterialDefinitionId != B.MaterialDefinitionId
				|| A.ItemInstanceId != B.ItemInstanceId
				|| A.Quantity != B.Quantity
				|| A.ExpectedQuantityBefore != B.ExpectedQuantityBefore
				|| A.IntentId != B.IntentId
				|| A.PrepareRequest.Context.RequestId
					!= B.PrepareRequest.Context.RequestId)
			{
				return false;
			}
		}
		return true;
	}

	bool HasExistingPhase(
		const Fdemo_mapShanmenFormationMaterialResult& Result,
		const EShanmenItemTransactionPhase Phase)
	{
		return Result.Lines.ContainsByPredicate(
			[Phase](
				const Fdemo_mapShanmenFormationMaterialPlanLine& Line)
			{
				return Line.ExistingFinalizeReceipt.IsSuccess()
					&& Line.ExistingFinalizeReceipt.Phase == Phase;
			});
	}

	bool AllLinesHaveExistingPhase(
		const Fdemo_mapShanmenFormationMaterialResult& Result,
		const EShanmenItemTransactionPhase Phase)
	{
		return !Result.Lines.IsEmpty()
			&& Result.Lines.ContainsByPredicate(
				[Phase](
					const Fdemo_mapShanmenFormationMaterialPlanLine& Line)
				{
					return !Line.ExistingFinalizeReceipt.IsSuccess()
						|| Line.ExistingFinalizeReceipt.Phase != Phase;
				}) == false;
	}

	bool AuthorityReady(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority)
	{
		return IsInGameThread()
			&& Authority.GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	}
}

bool Fdemo_mapShanmenFormationMaterialPlanLine::IsValid() const
{
	return !MaterialDefinitionId.IsNone() && ItemInstanceId.IsValid()
		&& Quantity > 0 && ExpectedQuantityBefore >= Quantity
		&& IntentId.IsValid() && PrepareRequest.IsValid()
		&& PrepareRequest.IntentId == IntentId
		&& PrepareRequest.ItemInstanceId == ItemInstanceId
		&& PrepareRequest.Amount == Quantity
		&& PrepareRequest.ExpectedQuantityBefore == ExpectedQuantityBefore
		&& PrepareRequest.PurposeId == FormationMaterialPurpose();
}

bool Fdemo_mapShanmenFormationMaterialResult::HasPlan() const
{
	if (!AttemptId.IsValid() || !TransactionId.IsValid()
		|| !ScopeId.IsValid() || !OwnerId.IsValid()
		|| !ActiveRunId.IsValid() || !DeploymentId.IsValid()
		|| AnchorDefinitionId.IsNone() || !Content.IsValid()
		|| Requirements.IsEmpty() || Lines.IsEmpty())
	{
		return false;
	}

	TMap<FName, int64> Required;
	for (const Fdemo_mapShanmenFormationMaterialRequirement& Requirement :
		Requirements)
	{
		if (!Requirement.IsValid()
			|| Required.Contains(Requirement.MaterialDefinitionId))
		{
			return false;
		}
		Required.Add(Requirement.MaterialDefinitionId, Requirement.Quantity);
	}

	TMap<FName, int64> Allocated;
	TSet<FGuid> ItemIds;
	TSet<FGuid> IntentIds;
	TSet<FGuid> RequestIds;
	for (const Fdemo_mapShanmenFormationMaterialPlanLine& Line : Lines)
	{
		if (!Line.IsValid() || ItemIds.Contains(Line.ItemInstanceId)
			|| IntentIds.Contains(Line.IntentId)
			|| RequestIds.Contains(Line.PrepareRequest.Context.RequestId)
			|| Line.PrepareRequest.Context.RunId != ScopeId
			|| Line.PrepareRequest.Context.OwnerId != OwnerId
			|| Line.PrepareRequest.ActiveRunId != ActiveRunId
			|| !SameContent(Line.PrepareRequest.Context.Content, Content))
		{
			return false;
		}
		ItemIds.Add(Line.ItemInstanceId);
		IntentIds.Add(Line.IntentId);
		RequestIds.Add(Line.PrepareRequest.Context.RequestId);
		Allocated.FindOrAdd(Line.MaterialDefinitionId) += Line.Quantity;
	}
	return Allocated.OrderIndependentCompareEqual(Required);
}

bool Fdemo_mapShanmenFormationMaterialResult::IsPrepared() const
{
	if ((Status != Edemo_mapShanmenFormationMaterialStatus::Prepared
			&& Status != Edemo_mapShanmenFormationMaterialStatus::Replayed)
		|| !HasPlan() || PrepareReceipts.Num() != Lines.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (!IsPrepareReceiptForLine(
			PrepareReceipts[Index], Lines[Index], ActiveRunId))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationMaterialResult::IsCommitted() const
{
	if (Status != Edemo_mapShanmenFormationMaterialStatus::Committed
		|| !HasPlan() || PrepareReceipts.Num() != Lines.Num()
		|| FinalizeReceipts.Num() != Lines.Num() || !Evidence.IsValid()
		|| Evidence.DeploymentId != DeploymentId
		|| Evidence.AnchorDefinitionId != AnchorDefinitionId
		|| Evidence.RunId != ActiveRunId || Evidence.OwnerId != OwnerId
		|| !SameContent(Evidence.Content, Content))
	{
		return false;
	}
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		if (!IsPrepareReceiptForLine(
				PrepareReceipts[Index], Lines[Index], ActiveRunId)
			|| !Fdemo_mapShanmenFormationMaterialAdapter::BuildFinalizeRequest(
				*this, Index, true, Request)
			|| !IsFinalizeReceiptForLine(
				FinalizeReceipts[Index], Lines[Index], ActiveRunId,
				Request.Context.RequestId,
				EShanmenItemTransactionPhase::Committed))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationMaterialResult::IsCancelled() const
{
	if (Status != Edemo_mapShanmenFormationMaterialStatus::Cancelled
		|| !HasPlan() || PrepareReceipts.Num() != Lines.Num()
		|| FinalizeReceipts.Num() != Lines.Num() || Evidence.IsValid())
	{
		return false;
	}
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		if (!IsPrepareReceiptForLine(
				PrepareReceipts[Index], Lines[Index], ActiveRunId)
			|| !Fdemo_mapShanmenFormationMaterialAdapter::BuildFinalizeRequest(
				*this, Index, false, Request)
			|| !IsFinalizeReceiptForLine(
				FinalizeReceipts[Index], Lines[Index], ActiveRunId,
				Request.Context.RequestId,
				EShanmenItemTransactionPhase::Cancelled))
		{
			return false;
		}
	}
	return true;
}

Fdemo_mapShanmenFormationMaterialResult
Fdemo_mapShanmenFormationMaterialAdapter::BuildPlan(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenFormationDeployment& Deployment,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!Correlation.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::RunCorrelationInvalid,
			TEXT("Formation materials require one valid immutable Run correlation."));
	}
	if (!Deployment.IsValid()
		|| Deployment.GetState()
			!= EShanmenFormationDeploymentState::Deploying
		|| !AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::DeploymentInvalid,
			TEXT("Formation materials require one deploying P8.0 formation and stable attempt identity."));
	}
	const FShanmenCombatActionSnapshot& Action = Deployment.GetAction();
	if (Action.GetActionDefinitionId()
			!= FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId()
		|| Action.GetRunId() != Correlation.ActiveRunId
		|| Action.GetOwnerId() != Correlation.OwnerId
		|| !SameContent(Action.GetContent(), Snapshot.Content))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::DeploymentMismatch,
			TEXT("Formation deployment identity or content does not match the active item authority Run."));
	}
	if (Snapshot.AuthorityRevision < Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::SnapshotStale,
			TEXT("Formation material evidence predates the active Run lifecycle."));
	}
	const FShanmenFormationAnchorDefinition* Anchor =
		Deployment.GetDiagram().FindAnchor(AnchorDefinitionId);
	const FShanmenFormationAnchorProgress* Progress =
		Deployment.GetAnchors().FindByPredicate(
			[AnchorDefinitionId](
				const FShanmenFormationAnchorProgress& Candidate)
			{
				return Candidate.GetAnchorDefinitionId()
					== AnchorDefinitionId;
			});
	if (!Anchor || !Anchor->IsValid() || !Progress || Progress->IsCommitted())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::AnchorInvalid,
			TEXT("Formation material planning requires one known uncommitted anchor."));
	}

	const FShanmenItemProcessedRequestSnapshot* Lifecycle =
		FindLifecycle(Snapshot, Correlation);
	if (!IsLifecycleValid(Lifecycle, Correlation)
		|| Snapshot.ProcessedRequests.ContainsByPredicate(
			[&Correlation](
				const FShanmenItemProcessedRequestSnapshot& Processed)
			{
				return Processed.Receipt.IsSuccess()
					&& Processed.Receipt.Operation
						== EShanmenItemTransactionOperation::FinalizePreparedRun
					&& Processed.Receipt.ReservationId
						== Correlation.ActiveRunId;
			}))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::LifecycleInvalid,
			TEXT("Formation materials require the exact non-terminal prepared Run lifecycle receipt."));
	}

	Fdemo_mapShanmenFormationMaterialResult Result;
	Result.Status = Edemo_mapShanmenFormationMaterialStatus::PlanReady;
	Result.AttemptId = AttemptId;
	Result.TransactionId = MakeTransactionId(
		Correlation, Deployment, AnchorDefinitionId, AttemptId);
	Result.ScopeId = Correlation.ScopeId;
	Result.OwnerId = Correlation.OwnerId;
	Result.ActiveRunId = Correlation.ActiveRunId;
	Result.DeploymentId = Deployment.GetDeploymentId();
	Result.AnchorDefinitionId = AnchorDefinitionId;
	Result.Content = Snapshot.Content;
	for (const FShanmenFormationMaterialRequirement& Requirement :
		Anchor->GetRequirements())
	{
		Fdemo_mapShanmenFormationMaterialRequirement& Copy =
			Result.Requirements.AddDefaulted_GetRef();
		Copy.MaterialDefinitionId = Requirement.GetMaterialDefinitionId();
		Copy.Quantity = Requirement.GetQuantity();
	}
	if (!Result.TransactionId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::RequestInvalid,
			TEXT("Validated formation identities did not produce a transaction identity."));
	}

	TMap<FGuid, const FShanmenItemDefinition*> DefinitionsByItem;
	TMap<FGuid, int32> CurrentQuantityByItem;
	for (const FGuid& ItemId : Correlation.OrderedRunInventoryItemInstanceIds)
	{
		const FShanmenItemInstance* Item = FindBy(
			Snapshot.Items,
			[&ItemId](const FShanmenItemInstance& Candidate)
			{
				return Candidate.ItemInstanceId == ItemId;
			});
		const FShanmenItemDefinition* Definition = Item
			? FindBy(
				Snapshot.Definitions,
				[Item](const FShanmenItemDefinition& Candidate)
				{
					return Candidate.DefinitionId == Item->DefinitionId;
				})
			: nullptr;
		bool bDuplicateReservation = false;
		const FShanmenItemReservationSnapshot* Reservation =
			FindQuantityReservation(
				Snapshot, Lifecycle->Receipt, ItemId,
				bDuplicateReservation);
		if (!Item || Item->OwnerId != Correlation.OwnerId
			|| Item->RunId != Correlation.ScopeId
			|| !Definition || !Definition->IsValid()
			|| !Definition->Supports(EShanmenItemResourceKind::Quantity)
			|| bDuplicateReservation || !Reservation
			|| !Reservation->IsValid()
			|| Reservation->State
				!= EShanmenItemReservationState::Committed
			|| Reservation->OwnerId != Correlation.OwnerId
			|| Reservation->RunId != Correlation.ScopeId)
		{
			return Reject(
				Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
				TEXT("Prepared Run inventory has missing, duplicate, or invalid Quantity authority."));
		}
		const int32 Current = Reservation->Amount
			- ConsumedQuantity(Snapshot, Correlation.ActiveRunId, ItemId);
		if (Current < 0)
		{
			return Reject(
				Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
				TEXT("Active-Run consumption exceeds one frozen material stack."));
		}
		DefinitionsByItem.Add(ItemId, Definition);
		CurrentQuantityByItem.Add(ItemId, Current);
	}

	TSet<FGuid> FinalizedPrepareRequestIds;
	TMap<FGuid, const FShanmenItemTransactionReceipt*> PrepareByIntent;
	TMap<FGuid, const FShanmenItemTransactionReceipt*> FinalizeByPrepare;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		const FShanmenItemTransactionReceipt& Receipt = Processed.Receipt;
		if (!Receipt.IsSuccess())
		{
			continue;
		}
		if (Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
			&& Receipt.ReservationIds.Num() == 2)
		{
			if (FinalizeByPrepare.Contains(Receipt.ReservationIds[1]))
			{
				return Reject(
					Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
					TEXT("One Quantity prepare identity has duplicate terminal receipts."));
			}
			FinalizedPrepareRequestIds.Add(Receipt.ReservationIds[1]);
			FinalizeByPrepare.Add(Receipt.ReservationIds[1], &Receipt);
		}
		else if (Receipt.Operation
				== EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
			&& Receipt.ReservationIds
				== TArray<FGuid>({ Correlation.ActiveRunId }))
		{
			if (PrepareByIntent.Contains(Receipt.ReservationId))
			{
				return Reject(
					Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
					TEXT("One material intent has duplicate prepare receipts."));
			}
			PrepareByIntent.Add(Receipt.ReservationId, &Receipt);
		}
	}

	bool bHasCommittedTerminal = false;
	bool bHasCancelledTerminal = false;
	TSet<FGuid> ExistingPlanIntentIds;
	for (const FGuid& ItemId : Correlation.OrderedRunInventoryItemInstanceIds)
	{
		const FGuid IntentId = MakeIntentId(Result.TransactionId, ItemId);
		const FShanmenItemTransactionReceipt* const* ExistingPrepare =
			PrepareByIntent.Find(IntentId);
		if (!ExistingPrepare || !*ExistingPrepare)
		{
			continue;
		}
		ExistingPlanIntentIds.Add(IntentId);
		const FShanmenItemTransactionReceipt* const* ExistingFinalize =
			FinalizeByPrepare.Find((*ExistingPrepare)->RequestId);
		if (ExistingFinalize && *ExistingFinalize)
		{
			bHasCommittedTerminal |= (*ExistingFinalize)->Phase
				== EShanmenItemTransactionPhase::Committed;
			bHasCancelledTerminal |= (*ExistingFinalize)->Phase
				== EShanmenItemTransactionPhase::Cancelled;
		}
	}
	if (bHasCommittedTerminal && bHasCancelledTerminal)
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
			TEXT("One formation material attempt cannot mix committed and cancelled terminals."));
	}

	auto AddPlanLine = [
		&Result, &PrepareByIntent, &FinalizeByPrepare,
		&Correlation](
		const FName DefinitionId,
		const FGuid& ItemId,
		const int32 Quantity,
		const int32 ExpectedBefore) -> bool
	{
		Fdemo_mapShanmenFormationMaterialPlanLine& Line =
			Result.Lines.AddDefaulted_GetRef();
		Line.MaterialDefinitionId = DefinitionId;
		Line.ItemInstanceId = ItemId;
		Line.Quantity = Quantity;
		Line.ExpectedQuantityBefore = ExpectedBefore;
		Line.IntentId = MakeIntentId(Result.TransactionId, ItemId);
		Line.PrepareRequest.Context.RunId = Result.ScopeId;
		Line.PrepareRequest.Context.OwnerId = Result.OwnerId;
		Line.PrepareRequest.Context.RequestId = MakePrepareRequestId(
			Result.TransactionId, ItemId, Quantity, ExpectedBefore);
		Line.PrepareRequest.Context.Content = Result.Content;
		Line.PrepareRequest.ActiveRunId = Result.ActiveRunId;
		Line.PrepareRequest.IntentId = Line.IntentId;
		Line.PrepareRequest.ItemInstanceId = ItemId;
		Line.PrepareRequest.Amount = Quantity;
		Line.PrepareRequest.ExpectedQuantityBefore = ExpectedBefore;
		Line.PrepareRequest.PurposeId = FormationMaterialPurpose();
		const FShanmenItemTransactionReceipt* const* ExistingPrepare =
			PrepareByIntent.Find(Line.IntentId);
		if (ExistingPrepare && *ExistingPrepare)
		{
			Line.ExistingPrepareReceipt = **ExistingPrepare;
			if (!IsPrepareReceiptForLine(
				Line.ExistingPrepareReceipt, Line, Correlation.ActiveRunId))
			{
				return false;
			}
			const FShanmenItemTransactionReceipt* const* ExistingFinalize =
				FinalizeByPrepare.Find(Line.PrepareRequest.Context.RequestId);
			if (ExistingFinalize && *ExistingFinalize)
			{
				Line.ExistingFinalizeReceipt = **ExistingFinalize;
				const FGuid ExpectedFinalizeRequestId =
					MakeFinalizeRequestId(
						Result.TransactionId,
						Line.PrepareRequest.Context.RequestId,
						Line.ItemInstanceId);
				if (!ExpectedFinalizeRequestId.IsValid()
					|| !IsFinalizeReceiptForLine(
						Line.ExistingFinalizeReceipt, Line,
						Correlation.ActiveRunId,
						ExpectedFinalizeRequestId,
						Line.ExistingFinalizeReceipt.Phase))
				{
					return false;
				}
			}
		}
		return Line.IsValid();
	};

	// A committed line changes the available quantity, so a forward-only
	// attempt must be rebuilt strictly from its original receipts. Cancellation
	// does not consume quantity; rebuilding it through the normal deterministic
	// allocator preserves missing lines after a partial prepare rollback and
	// lets the same attempt finish releasing every still-pending line.
	if (bHasCommittedTerminal)
	{
		for (const Fdemo_mapShanmenFormationMaterialRequirement& Requirement :
			Result.Requirements)
		{
			int32 Remaining = Requirement.Quantity;
			for (const FGuid& ItemId :
				Correlation.OrderedRunInventoryItemInstanceIds)
			{
				const FShanmenItemDefinition* const* Definition =
					DefinitionsByItem.Find(ItemId);
				const FGuid IntentId = MakeIntentId(
					Result.TransactionId, ItemId);
				const FShanmenItemTransactionReceipt* const* Existing =
					PrepareByIntent.Find(IntentId);
				if (!Definition || !*Definition || !Existing || !*Existing
					|| (*Definition)->DefinitionId
						!= Requirement.MaterialDefinitionId)
				{
					continue;
				}
				if (!AddPlanLine(
						Requirement.MaterialDefinitionId,
						ItemId, (*Existing)->Amount,
						(*Existing)->ResourceBefore))
				{
					return Reject(
						Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
						TEXT("Recovered formation material receipts do not match their deterministic plan."));
				}
				Remaining -= (*Existing)->Amount;
			}
			if (Remaining != 0)
			{
				return Reject(
					Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
					TEXT("A terminal formation material attempt is missing prepared requirement lines."));
			}
		}
	}
	else
	{
		TSet<FGuid> MatchedExistingIntents;
		for (const Fdemo_mapShanmenFormationMaterialRequirement& Requirement :
			Result.Requirements)
		{
			int32 Remaining = Requirement.Quantity;
			for (const FGuid& ItemId :
				Correlation.OrderedRunInventoryItemInstanceIds)
			{
				const FShanmenItemDefinition* const* Definition =
					DefinitionsByItem.Find(ItemId);
				const int32* Current = CurrentQuantityByItem.Find(ItemId);
				if (!Definition || !*Definition || !Current || *Current <= 0
					|| (*Definition)->DefinitionId
						!= Requirement.MaterialDefinitionId)
				{
					continue;
				}
				const FGuid IntentId = MakeIntentId(
					Result.TransactionId, ItemId);
				const FShanmenItemTransactionReceipt* const* Existing =
					PrepareByIntent.Find(IntentId);
				if (!Existing || !*Existing)
				{
					const bool bOtherPending =
						Snapshot.ProcessedRequests.ContainsByPredicate(
							[&ItemId, &Correlation,
								&FinalizedPrepareRequestIds](
								const FShanmenItemProcessedRequestSnapshot& Processed)
							{
								const FShanmenItemTransactionReceipt& Receipt =
									Processed.Receipt;
								return Receipt.IsSuccess()
									&& Receipt.Operation
										== EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
									&& Receipt.ItemInstanceId == ItemId
									&& Receipt.ReservationIds
										== TArray<FGuid>({ Correlation.ActiveRunId })
									&& !FinalizedPrepareRequestIds.Contains(
										Receipt.RequestId);
							});
					if (bOtherPending)
					{
						return Reject(
							Edemo_mapShanmenFormationMaterialStatus::ConflictingIntent,
							TEXT("Another action already owns a pending intent for one selected material stack."));
					}
				}
				const int32 Quantity = Existing && *Existing
					? (*Existing)->Amount
					: FMath::Min(Remaining, *Current);
				if (Quantity <= 0 || Quantity > Remaining
					|| !AddPlanLine(
						Requirement.MaterialDefinitionId, ItemId,
						Quantity,
						Existing && *Existing
							? (*Existing)->ResourceBefore : *Current))
				{
					return Reject(
						Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
						TEXT("Existing formation material prepare does not match canonical allocation."));
				}
				if (Existing && *Existing)
				{
					MatchedExistingIntents.Add(IntentId);
				}
				Remaining -= Quantity;
				if (Remaining == 0)
				{
					break;
				}
			}
			if (Remaining != 0)
			{
				return Reject(
					Edemo_mapShanmenFormationMaterialStatus::QuantityUnavailable,
					TEXT("Prepared Run inventory cannot satisfy the exact formation anchor requirements."));
			}
		}
		bool bExistingSetsMatch =
			MatchedExistingIntents.Num() == ExistingPlanIntentIds.Num();
		for (const FGuid& IntentId : MatchedExistingIntents)
		{
			bExistingSetsMatch &= ExistingPlanIntentIds.Contains(IntentId);
		}
		if (!bExistingSetsMatch)
		{
			return Reject(
				Edemo_mapShanmenFormationMaterialStatus::ItemAuthorityInvalid,
				TEXT("Formation material attempt contains an unexpected prepared item line."));
		}
	}

	if (!Result.HasPlan())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::RequestInvalid,
			TEXT("Validated material allocation did not produce a self-consistent plan."));
	}
	Result.PrepareReceipts.SetNum(Result.Lines.Num());
	Result.FinalizeReceipts.SetNum(Result.Lines.Num());
	for (int32 Index = 0; Index < Result.Lines.Num(); ++Index)
	{
		Result.PrepareReceipts[Index] =
			Result.Lines[Index].ExistingPrepareReceipt;
		Result.FinalizeReceipts[Index] =
			Result.Lines[Index].ExistingFinalizeReceipt;
	}
	Result.Diagnostic = ExistingPlanIntentIds.IsEmpty()
		? TEXT("Exact formation requirements were allocated across active-Run stacks deterministically.")
		: TEXT("Existing formation material receipts reconstructed the same deterministic attempt.");
	return Result;
}

bool Fdemo_mapShanmenFormationMaterialAdapter::BuildFinalizeRequest(
	const Fdemo_mapShanmenFormationMaterialResult& Preparation,
	const int32 LineIndex,
	const bool bCommit,
	FShanmenItemRunQuantityIntentFinalizeRequest& OutRequest)
{
	OutRequest = FShanmenItemRunQuantityIntentFinalizeRequest();
	if (!Preparation.HasPlan()
		|| !Preparation.Lines.IsValidIndex(LineIndex))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationMaterialPlanLine& Line =
		Preparation.Lines[LineIndex];
	OutRequest.Context.RunId = Preparation.ScopeId;
	OutRequest.Context.OwnerId = Preparation.OwnerId;
	OutRequest.Context.RequestId = MakeFinalizeRequestId(
		Preparation.TransactionId,
		Line.PrepareRequest.Context.RequestId,
		Line.ItemInstanceId);
	OutRequest.Context.Content = Preparation.Content;
	OutRequest.ActiveRunId = Preparation.ActiveRunId;
	OutRequest.PrepareRequestId = Line.PrepareRequest.Context.RequestId;
	OutRequest.IntentId = Line.IntentId;
	OutRequest.ItemInstanceId = Line.ItemInstanceId;
	OutRequest.bCommit = bCommit;
	return OutRequest.IsValid();
}

Fdemo_mapShanmenFormationMaterialResult
Fdemo_mapShanmenFormationMaterialAdapter::BuildCommittedEvidence(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenFormationMaterialResult& Preparation,
	const TArray<FShanmenItemTransactionReceipt>& CommitReceipts)
{
	if (!Correlation.IsValid() || !Preparation.HasPlan()
		|| !Deployment.IsValid()
		|| Deployment.GetState()
			!= EShanmenFormationDeploymentState::Deploying
		|| Deployment.GetDeploymentId() != Preparation.DeploymentId
		|| Deployment.GetAction().GetRunId() != Preparation.ActiveRunId
		|| Deployment.GetAction().GetOwnerId() != Preparation.OwnerId
		|| Correlation.ActiveRunId != Preparation.ActiveRunId
		|| Correlation.OwnerId != Preparation.OwnerId
		|| !SameContent(
			Deployment.GetAction().GetContent(), Preparation.Content)
		|| CommitReceipts.Num() != Preparation.Lines.Num())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::PreparationInvalid,
			TEXT("Committed formation evidence requires the exact deploying plan and Run correlation."));
	}

	Fdemo_mapShanmenFormationMaterialResult Result = Preparation;
	Result.Status = Edemo_mapShanmenFormationMaterialStatus::Committed;
	Result.FinalizeReceipts = CommitReceipts;
	Result.Evidence = FShanmenFormationAnchorFulfillmentEvidence();
	Result.Evidence.RunId = Preparation.ActiveRunId;
	Result.Evidence.OwnerId = Preparation.OwnerId;
	Result.Evidence.DeploymentId = Preparation.DeploymentId;
	Result.Evidence.AnchorDefinitionId = Preparation.AnchorDefinitionId;
	Result.Evidence.Content = Preparation.Content;
	TArray<FString> FulfillmentParts =
	{
		GuidDigits(Preparation.TransactionId),
		GuidDigits(Preparation.AttemptId),
		GuidDigits(Preparation.DeploymentId),
		Preparation.AnchorDefinitionId.ToString()
	};
	int32 FinalAuthorityRevision = INDEX_NONE;
	for (int32 Index = 0; Index < Preparation.Lines.Num(); ++Index)
	{
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		if (!BuildFinalizeRequest(Preparation, Index, true, Request)
			|| !IsFinalizeReceiptForLine(
				CommitReceipts[Index], Preparation.Lines[Index],
				Preparation.ActiveRunId, Request.Context.RequestId,
				EShanmenItemTransactionPhase::Committed))
		{
			return Reject(
				Edemo_mapShanmenFormationMaterialStatus::EvidenceInvalid,
				TEXT("One material terminal receipt is not the exact committed plan line."));
		}
		FinalAuthorityRevision = FMath::Max(
			FinalAuthorityRevision, CommitReceipts[Index].AuthorityRevision);
		FulfillmentParts.Add(GuidDigits(CommitReceipts[Index].ReceiptId));
		FShanmenFormationMaterialFulfillmentLine& EvidenceLine =
			Result.Evidence.Lines.AddDefaulted_GetRef();
		EvidenceLine.ItemInstanceId = Preparation.Lines[Index].ItemInstanceId;
		EvidenceLine.MaterialDefinitionId =
			Preparation.Lines[Index].MaterialDefinitionId;
		EvidenceLine.Quantity = Preparation.Lines[Index].Quantity;
	}
	Result.Evidence.AuthorityRevision = FinalAuthorityRevision;
	FulfillmentParts.Add(FString::FromInt(FinalAuthorityRevision));
	Result.Evidence.FulfillmentId =
		FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.Formation.MaterialFulfillment.r1"),
			FulfillmentParts);
	if (!Result.Evidence.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::EvidenceInvalid,
			TEXT("Committed material receipts did not produce valid P8.0 fulfillment evidence."));
	}
	Result.Diagnostic =
		TEXT("Every exact formation material line is durable; P8.0 fulfillment evidence is ready.");
	return Result;
}

Fdemo_mapShanmenFormationMaterialResult
Fdemo_mapShanmenFormationMaterialAdapter::PrepareMaterials(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenFormationDeployment& Deployment,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!AuthorityReady(Authority))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::AuthorityNotReady,
			TEXT("Formation material preparation requires the ready item authority on the Game Thread."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::SnapshotUnavailable,
			TEXT("The ready authority could not provide a material-planning snapshot."));
	}
	Fdemo_mapShanmenFormationMaterialResult Result = BuildPlan(
		Snapshot, Correlation, Deployment, AnchorDefinitionId, AttemptId);
	if (!Result.HasPlan())
	{
		return Result;
	}
	if (AllLinesHaveExistingPhase(
			Result, EShanmenItemTransactionPhase::Cancelled))
	{
		Result.Status =
			Edemo_mapShanmenFormationMaterialStatus::AttemptCancelled;
		Result.Diagnostic =
			TEXT("This formation material attempt is already cancelled; use a new AttemptId.");
		return Result;
	}
	const bool bResumingPartialCancellation = HasExistingPhase(
		Result, EShanmenItemTransactionPhase::Cancelled);

	Result.PrepareCommands.SetNum(Result.Lines.Num());
	Result.PrepareReceipts.SetNum(Result.Lines.Num());
	Result.FinalizeCommands.SetNum(Result.Lines.Num());
	Result.FinalizeReceipts.SetNum(Result.Lines.Num());
	bool bAllReplayed = true;
	for (int32 Index = 0; Index < Result.Lines.Num(); ++Index)
	{
		Result.PrepareCommands[Index] =
			Authority.PreparePreparedRunQuantityIntentDurable(
				Result.Lines[Index].PrepareRequest);
		if (!Result.PrepareCommands[Index].IsCommandSuccess())
		{
			bool bRollbackComplete = true;
			for (int32 RollbackIndex = 0;
				RollbackIndex < Index;
				++RollbackIndex)
			{
				FShanmenItemRunQuantityIntentFinalizeRequest Cancel;
				if (!BuildFinalizeRequest(
						Result, RollbackIndex, false, Cancel))
				{
					bRollbackComplete = false;
					continue;
				}
				Result.FinalizeCommands[RollbackIndex] =
					Authority.FinalizePreparedRunQuantityIntentDurable(Cancel);
				if (Result.FinalizeCommands[RollbackIndex].IsCommandSuccess())
				{
					Result.FinalizeReceipts[RollbackIndex] =
						Result.FinalizeCommands[RollbackIndex].Receipt;
				}
				else
				{
					bRollbackComplete = false;
				}
			}
			Result.Status = bRollbackComplete
				? Edemo_mapShanmenFormationMaterialStatus::PrepareRejectedRolledBack
				: Edemo_mapShanmenFormationMaterialStatus::RollbackRecoveryRequired;
			Result.Diagnostic = bRollbackComplete
				? TEXT("A material prepare failed; every earlier line was durably cancelled.")
				: TEXT("A material prepare failed and at least one durable cancellation requires exact replay recovery.");
			return Result;
		}
		Result.PrepareReceipts[Index] =
			Result.PrepareCommands[Index].Receipt;
		bAllReplayed &= Result.PrepareCommands[Index].Status
			== EShanmenItemDurableCommandStatus::Replayed;
	}
	Result.Status = bAllReplayed
		? Edemo_mapShanmenFormationMaterialStatus::Replayed
		: Edemo_mapShanmenFormationMaterialStatus::Prepared;
	Result.Diagnostic = bResumingPartialCancellation
		? TEXT("A partially cancelled material attempt was reconstructed; replay cancellation to release every remaining line.")
		: bAllReplayed
			? TEXT("Every formation material prepare replayed its durable receipt.")
			: TEXT("Every exact formation material stack is durably prepared.");
	return Result;
}

Fdemo_mapShanmenFormationMaterialResult
Fdemo_mapShanmenFormationMaterialAdapter::CommitMaterials(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenFormationMaterialResult& Preparation)
{
	if (!AuthorityReady(Authority))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::AuthorityNotReady,
			TEXT("Formation material commit requires the ready item authority on the Game Thread."));
	}
	if (!Preparation.IsPrepared())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::PreparationInvalid,
			TEXT("Formation material commit requires every exact line to be durably prepared."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::SnapshotUnavailable,
			TEXT("Authority disappeared before formation material commit."));
	}
	const Fdemo_mapShanmenFormationMaterialResult Current = BuildPlan(
		Snapshot, Correlation, Deployment, Preparation.AnchorDefinitionId,
		Preparation.AttemptId);
	if (!PlansMatch(Current, Preparation))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::PreparationInvalid,
			TEXT("Durable material state no longer reconstructs the prepared plan."));
	}
	if (HasExistingPhase(Current, EShanmenItemTransactionPhase::Cancelled))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::AttemptCancelled,
			TEXT("A cancelled material attempt cannot later commit."));
	}

	Fdemo_mapShanmenFormationMaterialResult Result = Preparation;
	Result.FinalizeCommands.SetNum(Result.Lines.Num());
	Result.FinalizeReceipts.SetNum(Result.Lines.Num());
	for (int32 Index = 0; Index < Result.Lines.Num(); ++Index)
	{
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		if (!BuildFinalizeRequest(Result, Index, true, Request))
		{
			return Reject(
				Edemo_mapShanmenFormationMaterialStatus::RequestInvalid,
				TEXT("Prepared line did not produce a valid commit request."));
		}
		Result.FinalizeCommands[Index] =
			Authority.FinalizePreparedRunQuantityIntentDurable(Request);
		if (!Result.FinalizeCommands[Index].IsCommandSuccess())
		{
			Result.Status =
				Edemo_mapShanmenFormationMaterialStatus::CommitRecoveryRequired;
			Result.Diagnostic =
				TEXT("Material commit is forward-only; replay this exact attempt to complete remaining lines.");
			return Result;
		}
		Result.FinalizeReceipts[Index] =
			Result.FinalizeCommands[Index].Receipt;
	}
	Fdemo_mapShanmenFormationMaterialResult Committed =
		BuildCommittedEvidence(
			Correlation, Deployment, Result, Result.FinalizeReceipts);
	Committed.PrepareCommands = MoveTemp(Result.PrepareCommands);
	Committed.FinalizeCommands = MoveTemp(Result.FinalizeCommands);
	return Committed;
}

Fdemo_mapShanmenFormationMaterialResult
Fdemo_mapShanmenFormationMaterialAdapter::CancelMaterials(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenFormationMaterialResult& Preparation)
{
	if (!AuthorityReady(Authority))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::AuthorityNotReady,
			TEXT("Formation material cancellation requires the ready item authority on the Game Thread."));
	}
	if (!Preparation.IsPrepared())
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::PreparationInvalid,
			TEXT("Formation material cancellation requires one complete prepared attempt."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::SnapshotUnavailable,
			TEXT("Authority disappeared before formation material cancellation."));
	}
	const Fdemo_mapShanmenFormationMaterialResult Current = BuildPlan(
		Snapshot, Correlation, Deployment, Preparation.AnchorDefinitionId,
		Preparation.AttemptId);
	if (!PlansMatch(Current, Preparation))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::PreparationInvalid,
			TEXT("Durable material state no longer reconstructs the prepared plan."));
	}
	if (HasExistingPhase(Current, EShanmenItemTransactionPhase::Committed))
	{
		return Reject(
			Edemo_mapShanmenFormationMaterialStatus::CommitRecoveryRequired,
			TEXT("At least one material line already committed; the attempt must complete forward."));
	}

	Fdemo_mapShanmenFormationMaterialResult Result = Preparation;
	Result.FinalizeCommands.SetNum(Result.Lines.Num());
	Result.FinalizeReceipts.SetNum(Result.Lines.Num());
	for (int32 Index = 0; Index < Result.Lines.Num(); ++Index)
	{
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		if (!BuildFinalizeRequest(Result, Index, false, Request))
		{
			return Reject(
				Edemo_mapShanmenFormationMaterialStatus::RequestInvalid,
				TEXT("Prepared line did not produce a valid cancellation request."));
		}
		Result.FinalizeCommands[Index] =
			Authority.FinalizePreparedRunQuantityIntentDurable(Request);
		if (!Result.FinalizeCommands[Index].IsCommandSuccess())
		{
			Result.Status =
				Edemo_mapShanmenFormationMaterialStatus::CancelRecoveryRequired;
			Result.Diagnostic =
				TEXT("Material cancellation requires exact replay to release every remaining line.");
			return Result;
		}
		Result.FinalizeReceipts[Index] =
			Result.FinalizeCommands[Index].Receipt;
	}
	Result.Status = Edemo_mapShanmenFormationMaterialStatus::Cancelled;
	Result.Evidence = FShanmenFormationAnchorFulfillmentEvidence();
	Result.Diagnostic =
		TEXT("Every prepared formation material line was durably cancelled without consumption.");
	return Result;
}
