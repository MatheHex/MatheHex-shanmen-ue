#include "demo_mapShanmenFormationScatterResourcePlan.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"

namespace
{
	using EPlanStatus =
		Edemo_mapShanmenFormationScatterResourcePlanStatus;

	FName ScatterMaterialPurpose()
	{
		return TEXT("Shanmen.Formation.ScatterBatchMaterial.r1");
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString CanonicalName(const FName Value)
	{
		FString Result = Value.ToString();
		Result.ToLowerInline();
		return Result;
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool SameRequest(
		const FShanmenItemRunQuantityIntentRequest& Left,
		const FShanmenItemRunQuantityIntentRequest& Right)
	{
		return Left.Context.RunId == Right.Context.RunId
			&& Left.Context.OwnerId == Right.Context.OwnerId
			&& Left.Context.RequestId == Right.Context.RequestId
			&& SameContent(Left.Context.Content, Right.Context.Content)
			&& Left.ActiveRunId == Right.ActiveRunId
			&& Left.IntentId == Right.IntentId
			&& Left.ItemInstanceId == Right.ItemInstanceId
			&& Left.Amount == Right.Amount
			&& Left.ExpectedQuantityBefore
				== Right.ExpectedQuantityBefore
			&& Left.PurposeId == Right.PurposeId;
	}

	template <typename TValue, typename TPredicate>
	const TValue* FindBy(
		const TArray<TValue>& Values,
		TPredicate Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	Fdemo_mapShanmenFormationScatterResourcePlanResult Reject(
		const EPlanStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterResourcePlanResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
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
		const FShanmenItemTransactionReceipt& Receipt =
			Lifecycle->Receipt;
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
		bool& bOutDuplicate)
	{
		bOutDuplicate = false;
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
			if (!Candidate
				|| Candidate->ItemInstanceId != ItemInstanceId
				|| Candidate->ResourceKind
					!= EShanmenItemResourceKind::Quantity)
			{
				continue;
			}
			if (Found)
			{
				bOutDuplicate = true;
				return nullptr;
			}
			Found = Candidate;
		}
		return Found;
	}

	bool IsRunFinalized(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ActiveRunId)
	{
		return Snapshot.ProcessedRequests.ContainsByPredicate(
			[&ActiveRunId](
				const FShanmenItemProcessedRequestSnapshot& Processed)
			{
				return Processed.Receipt.IsSuccess()
					&& Processed.Receipt.Operation
						== EShanmenItemTransactionOperation::
							FinalizePreparedRun
					&& Processed.Receipt.ReservationId == ActiveRunId;
			});
	}

	struct FInventoryBalance
	{
		int32 ItemOrder = INDEX_NONE;
		FGuid ItemInstanceId;
		FName MaterialDefinitionId = NAME_None;
		int32 QuantityBefore = 0;
		int32 QuantityRemaining = 0;
		bool bHasPendingIntent = false;
	};
}

FGuid Fdemo_mapShanmenFormationScatterResourceSlice::BuildSliceId(
	const Fdemo_mapShanmenFormationScatterResourceSlice& Slice)
{
	if (!Slice.PlanningScopeId.IsValid()
		|| Slice.SliceOrder < 0
		|| !Slice.AnchorIntentId.IsValid()
		|| !Slice.MaterialIntentId.IsValid()
		|| Slice.AnchorOrder < 0
		|| Slice.RequirementOrder < 0
		|| Slice.MaterialDefinitionId.IsNone()
		|| !Slice.ItemInstanceId.IsValid()
		|| Slice.Quantity <= 0
		|| Slice.SourceQuantityBefore < Slice.Quantity
		|| Slice.SourceQuantityAfter
			!= Slice.SourceQuantityBefore - Slice.Quantity)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourceSlice.r1"),
		{
			GuidDigits(Slice.PlanningScopeId),
			FString::FromInt(Slice.SliceOrder),
			GuidDigits(Slice.AnchorIntentId),
			GuidDigits(Slice.MaterialIntentId),
			FString::FromInt(Slice.AnchorOrder),
			FString::FromInt(Slice.RequirementOrder),
			CanonicalName(Slice.MaterialDefinitionId),
			GuidDigits(Slice.ItemInstanceId),
			FString::FromInt(Slice.Quantity),
			FString::FromInt(Slice.SourceQuantityBefore),
			FString::FromInt(Slice.SourceQuantityAfter)
		});
}

bool Fdemo_mapShanmenFormationScatterResourceSlice::IsValid() const
{
	return SliceId.IsValid() && SliceId == BuildSliceId(*this);
}

bool Fdemo_mapShanmenFormationScatterResourceSlice::operator==(
	const Fdemo_mapShanmenFormationScatterResourceSlice& Other) const
{
	return SliceId == Other.SliceId
		&& PlanningScopeId == Other.PlanningScopeId
		&& SliceOrder == Other.SliceOrder
		&& AnchorIntentId == Other.AnchorIntentId
		&& MaterialIntentId == Other.MaterialIntentId
		&& AnchorOrder == Other.AnchorOrder
		&& RequirementOrder == Other.RequirementOrder
		&& MaterialDefinitionId == Other.MaterialDefinitionId
		&& ItemInstanceId == Other.ItemInstanceId
		&& Quantity == Other.Quantity
		&& SourceQuantityBefore == Other.SourceQuantityBefore
		&& SourceQuantityAfter == Other.SourceQuantityAfter;
}

FGuid
Fdemo_mapShanmenFormationScatterResourceReservationIntent::BuildIntentId(
	const Fdemo_mapShanmenFormationScatterResourceReservationIntent& Intent)
{
	if (!Intent.PlanningScopeId.IsValid()
		|| Intent.ItemOrder < 0
		|| Intent.MaterialDefinitionId.IsNone()
		|| !Intent.ItemInstanceId.IsValid()
		|| Intent.ExpectedQuantityBefore < Intent.Quantity
		|| Intent.Quantity <= 0
		|| Intent.QuantityAfter
			!= Intent.ExpectedQuantityBefore - Intent.Quantity
		|| Intent.SliceIds.IsEmpty())
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Intent.PlanningScopeId),
		FString::FromInt(Intent.ItemOrder),
		CanonicalName(Intent.MaterialDefinitionId),
		GuidDigits(Intent.ItemInstanceId),
		FString::FromInt(Intent.ExpectedQuantityBefore),
		FString::FromInt(Intent.Quantity),
		FString::FromInt(Intent.QuantityAfter),
		FString::FromInt(Intent.SliceIds.Num())
	};
	for (const FGuid& SliceId : Intent.SliceIds)
	{
		if (!SliceId.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(SliceId));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourceReservationIntent.r1"),
		Parts);
}

FGuid
Fdemo_mapShanmenFormationScatterResourceReservationIntent::BuildRequestId(
	const Fdemo_mapShanmenFormationScatterResourceReservationIntent& Intent)
{
	if (!Intent.IntentId.IsValid()
		|| !Intent.PlanningScopeId.IsValid()
		|| !Intent.ItemInstanceId.IsValid()
		|| Intent.Quantity <= 0
		|| Intent.ExpectedQuantityBefore < Intent.Quantity)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourcePrepare.r1"),
		{
			GuidDigits(Intent.PlanningScopeId),
			GuidDigits(Intent.IntentId),
			GuidDigits(Intent.ItemInstanceId),
			FString::FromInt(Intent.Quantity),
			FString::FromInt(Intent.ExpectedQuantityBefore)
		});
}

bool
Fdemo_mapShanmenFormationScatterResourceReservationIntent::IsValid() const
{
	if (!IntentId.IsValid() || IntentId != BuildIntentId(*this)
		|| !PrepareRequest.IsValid()
		|| PrepareRequest.Context.RequestId != BuildRequestId(*this)
		|| PrepareRequest.IntentId != IntentId
		|| PrepareRequest.ItemInstanceId != ItemInstanceId
		|| PrepareRequest.Amount != Quantity
		|| PrepareRequest.ExpectedQuantityBefore
			!= ExpectedQuantityBefore
		|| PrepareRequest.PurposeId != ScatterMaterialPurpose())
	{
		return false;
	}
	TSet<FGuid> UniqueSliceIds;
	for (const FGuid& SliceId : SliceIds)
	{
		if (!SliceId.IsValid() || UniqueSliceIds.Contains(SliceId))
		{
			return false;
		}
		UniqueSliceIds.Add(SliceId);
	}
	return true;
}

bool
Fdemo_mapShanmenFormationScatterResourceReservationIntent::operator==(
	const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
		Other) const
{
	return IntentId == Other.IntentId
		&& PlanningScopeId == Other.PlanningScopeId
		&& ItemOrder == Other.ItemOrder
		&& MaterialDefinitionId == Other.MaterialDefinitionId
		&& ItemInstanceId == Other.ItemInstanceId
		&& ExpectedQuantityBefore == Other.ExpectedQuantityBefore
		&& Quantity == Other.Quantity
		&& QuantityAfter == Other.QuantityAfter
		&& SliceIds == Other.SliceIds
		&& SameRequest(PrepareRequest, Other.PrepareRequest);
}

FGuid Fdemo_mapShanmenFormationScatterResourcePlan::BuildPlanningScopeId(
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	if (!Plan.Batch.IsValid()
		|| !Plan.CorrelationId.IsValid()
		|| !Plan.OwnerId.IsValid()
		|| !Plan.ScopeId.IsValid()
		|| !Plan.ActiveRunId.IsValid()
		|| !Plan.LifecycleReceiptId.IsValid()
		|| Plan.AuthorityRevision < 0
		|| !Plan.Content.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourcePlanningScope.r1"),
		{
			GuidDigits(Plan.Batch.GetBatchIntentId()),
			GuidDigits(Plan.CorrelationId),
			GuidDigits(Plan.OwnerId),
			GuidDigits(Plan.ScopeId),
			GuidDigits(Plan.ActiveRunId),
			GuidDigits(Plan.LifecycleReceiptId),
			FString::FromInt(Plan.AuthorityRevision),
			CanonicalName(Plan.Content.Version),
			Plan.Content.Digest
		});
}

FGuid Fdemo_mapShanmenFormationScatterResourcePlan::BuildPlanId(
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	if (!Plan.PlanningScopeId.IsValid()
		|| !Plan.Batch.IsValid()
		|| Plan.Slices.IsEmpty()
		|| Plan.Reservations.IsEmpty()
		|| Plan.TotalRequirementCount <= 0
		|| Plan.TotalAllocatedQuantity <= 0)
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Plan.PlanningScopeId),
		GuidDigits(Plan.Batch.GetBatchIntentId()),
		FString::FromInt(Plan.TotalRequirementCount),
		LexToString(Plan.TotalAllocatedQuantity),
		FString::FromInt(Plan.Slices.Num()),
		FString::FromInt(Plan.Reservations.Num())
	};
	for (const Fdemo_mapShanmenFormationScatterResourceSlice& Slice :
		Plan.Slices)
	{
		if (!Slice.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Slice.GetSliceId()));
	}
	for (const auto& Reservation : Plan.Reservations)
	{
		if (!Reservation.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Reservation.GetIntentId()));
		Parts.Add(GuidDigits(
			Reservation.GetPrepareRequest().Context.RequestId));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourcePlan.r1"), Parts);
}

bool Fdemo_mapShanmenFormationScatterResourcePlan::IsValid() const
{
	if (!PlanId.IsValid()
		|| !PlanningScopeId.IsValid()
		|| !Batch.IsValid()
		|| !CorrelationId.IsValid()
		|| !OwnerId.IsValid()
		|| !ScopeId.IsValid()
		|| !ActiveRunId.IsValid()
		|| !LifecycleReceiptId.IsValid()
		|| AuthorityRevision < 0
		|| !Content.IsValid()
		|| Slices.IsEmpty()
		|| Reservations.IsEmpty()
		|| TotalRequirementCount
			!= Batch.GetTotalRequirementCount()
		|| TotalAllocatedQuantity
			!= Batch.GetTotalMaterialQuantity()
		|| PlanningScopeId != BuildPlanningScopeId(*this))
	{
		return false;
	}

	TMap<FGuid, int64> AllocatedByMaterialIntent;
	TMap<FGuid, const Fdemo_mapShanmenFormationScatterResourceSlice*>
		SlicesById;
	for (int32 Index = 0; Index < Slices.Num(); ++Index)
	{
		const auto& Slice = Slices[Index];
		if (!Slice.IsValid()
			|| Slice.GetPlanningScopeId() != PlanningScopeId
			|| Slice.GetSliceOrder() != Index
			|| SlicesById.Contains(Slice.GetSliceId())
			|| !Batch.GetAnchorIntents().IsValidIndex(
				Slice.GetAnchorOrder()))
		{
			return false;
		}
		const auto& Anchor = Batch.GetAnchorIntents()[
			Slice.GetAnchorOrder()];
		if (Anchor.GetIntentId() != Slice.GetAnchorIntentId()
			|| !Anchor.GetMaterialIntents().IsValidIndex(
				Slice.GetRequirementOrder()))
		{
			return false;
		}
		const auto& Material = Anchor.GetMaterialIntents()[
			Slice.GetRequirementOrder()];
		if (Material.GetIntentId() != Slice.GetMaterialIntentId()
			|| Material.GetMaterialDefinitionId()
				!= Slice.GetMaterialDefinitionId())
		{
			return false;
		}
		SlicesById.Add(Slice.GetSliceId(), &Slice);
		AllocatedByMaterialIntent.FindOrAdd(
			Slice.GetMaterialIntentId()) += Slice.GetQuantity();
	}

	for (const auto& Anchor : Batch.GetAnchorIntents())
	{
		for (const auto& Material : Anchor.GetMaterialIntents())
		{
			const int64* Allocated = AllocatedByMaterialIntent.Find(
				Material.GetIntentId());
			if (!Allocated || *Allocated != Material.GetQuantity())
			{
				return false;
			}
		}
	}

	TSet<FGuid> CoveredSliceIds;
	TSet<FGuid> ItemIds;
	TSet<FGuid> IntentIds;
	TSet<FGuid> RequestIds;
	int32 PreviousItemOrder = INDEX_NONE;
	int64 ReservationQuantity = 0;
	for (const auto& Reservation : Reservations)
	{
		if (!Reservation.IsValid()
			|| Reservation.GetPlanningScopeId() != PlanningScopeId
			|| Reservation.GetItemOrder() <= PreviousItemOrder
			|| ItemIds.Contains(Reservation.GetItemInstanceId())
			|| IntentIds.Contains(Reservation.GetIntentId())
			|| RequestIds.Contains(
				Reservation.GetPrepareRequest().Context.RequestId))
		{
			return false;
		}
		PreviousItemOrder = Reservation.GetItemOrder();
		ItemIds.Add(Reservation.GetItemInstanceId());
		IntentIds.Add(Reservation.GetIntentId());
		RequestIds.Add(
			Reservation.GetPrepareRequest().Context.RequestId);
		const auto& Request = Reservation.GetPrepareRequest();
		if (Request.Context.RunId != ScopeId
			|| Request.Context.OwnerId != OwnerId
			|| !SameContent(Request.Context.Content, Content)
			|| Request.ActiveRunId != ActiveRunId)
		{
			return false;
		}

		int32 SliceQuantity = 0;
		int32 ExpectedBefore = Reservation.GetExpectedQuantityBefore();
		for (const FGuid& SliceId : Reservation.GetSliceIds())
		{
			const auto* const* Found = SlicesById.Find(SliceId);
			if (!Found || !*Found || CoveredSliceIds.Contains(SliceId))
			{
				return false;
			}
			const auto& Slice = **Found;
			if (Slice.GetItemInstanceId()
					!= Reservation.GetItemInstanceId()
				|| Slice.GetMaterialDefinitionId()
					!= Reservation.GetMaterialDefinitionId()
				|| Slice.GetSourceQuantityBefore() != ExpectedBefore)
			{
				return false;
			}
			CoveredSliceIds.Add(SliceId);
			SliceQuantity += Slice.GetQuantity();
			ExpectedBefore = Slice.GetSourceQuantityAfter();
		}
		if (SliceQuantity != Reservation.GetQuantity()
			|| ExpectedBefore != Reservation.GetQuantityAfter())
		{
			return false;
		}
		ReservationQuantity += Reservation.GetQuantity();
	}
	return CoveredSliceIds.Num() == Slices.Num()
		&& ReservationQuantity == TotalAllocatedQuantity
		&& PlanId == BuildPlanId(*this);
}

bool Fdemo_mapShanmenFormationScatterResourcePlan::operator==(
	const Fdemo_mapShanmenFormationScatterResourcePlan& Other) const
{
	return PlanId == Other.PlanId
		&& PlanningScopeId == Other.PlanningScopeId
		&& Batch == Other.Batch
		&& CorrelationId == Other.CorrelationId
		&& OwnerId == Other.OwnerId
		&& ScopeId == Other.ScopeId
		&& ActiveRunId == Other.ActiveRunId
		&& LifecycleReceiptId == Other.LifecycleReceiptId
		&& AuthorityRevision == Other.AuthorityRevision
		&& SameContent(Content, Other.Content)
		&& Slices == Other.Slices
		&& Reservations == Other.Reservations
		&& TotalRequirementCount == Other.TotalRequirementCount
		&& TotalAllocatedQuantity == Other.TotalAllocatedQuantity;
}

bool Fdemo_mapShanmenFormationScatterResourcePlanResult::IsValid() const
{
	if (Status == EPlanStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	return Status == EPlanStatus::Planned
		? Plan.IsValid()
		: !Plan.IsValid();
}

bool Fdemo_mapShanmenFormationScatterResourcePlanResult::IsPlanned() const
{
	return Status == EPlanStatus::Planned && IsValid();
}

Fdemo_mapShanmenFormationScatterResourcePlanResult
Fdemo_mapShanmenFormationScatterResourcePlanner::Plan(
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenFormationScatterBatchIntent& Batch)
{
	if (!Batch.IsValid())
	{
		return Reject(
			EPlanStatus::BatchInvalid,
			TEXT("Scatter resource planning requires one valid P27.17 batch."));
	}
	if (!Projection.IsProjected())
	{
		return Reject(
			EPlanStatus::MasteryProjectionRejected,
			TEXT("Scatter resource planning requires projected mastery evidence."));
	}
	if (!Deployment.IsValid())
	{
		return Reject(
			EPlanStatus::DeploymentInvalid,
			TEXT("Scatter resource planning requires one valid deployment."));
	}
	if (!Fdemo_mapShanmenFormationScatterBatchPlanner::IsCurrentBatch(
			Projection, Deployment, Batch))
	{
		return Reject(
			EPlanStatus::BatchStale,
			TEXT("The P27.17 scatter batch is stale for current mastery or deployment evidence."));
	}
	if (!Correlation.IsValid())
	{
		return Reject(
			EPlanStatus::RunCorrelationInvalid,
			TEXT("Scatter resource planning requires one valid immutable Run correlation."));
	}

	const FShanmenCombatActionSnapshot& Action = Deployment.GetAction();
	if (Action.GetRunId() != Correlation.ActiveRunId
		|| Action.GetOwnerId() != Correlation.OwnerId)
	{
		return Reject(
			EPlanStatus::RunMismatch,
			TEXT("Formation deployment and item authority correlation identify different Runs."));
	}
	if (!Snapshot.Content.IsValid())
	{
		return Reject(
			EPlanStatus::SnapshotInvalid,
			TEXT("The item authority snapshot has no valid content identity."));
	}
	if (!SameContent(Action.GetContent(), Snapshot.Content))
	{
		return Reject(
			EPlanStatus::ContentMismatch,
			TEXT("Formation and item authority content identities do not match."));
	}

	FShanmenItemRepository SnapshotValidator;
	EShanmenItemTransactionError SnapshotError =
		EShanmenItemTransactionError::None;
	if (!SnapshotValidator.TryLoadSnapshot(Snapshot, &SnapshotError))
	{
		return Reject(
			EPlanStatus::SnapshotInvalid,
			TEXT("The item authority snapshot failed repository invariant validation."));
	}
	if (Snapshot.AuthorityRevision
		< Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			EPlanStatus::SnapshotStale,
			TEXT("The item authority snapshot predates the correlated active Run."));
	}

	const FShanmenItemProcessedRequestSnapshot* Lifecycle =
		FindLifecycle(Snapshot, Correlation);
	if (!IsLifecycleValid(Lifecycle, Correlation)
		|| IsRunFinalized(Snapshot, Correlation.ActiveRunId))
	{
		return Reject(
			EPlanStatus::LifecycleInvalid,
			TEXT("Scatter resources require the exact non-terminal prepared Run lifecycle."));
	}

	TSet<FGuid> FinalizedPrepareRequestIds;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		const FShanmenItemTransactionReceipt& Receipt =
			Processed.Receipt;
		if (Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.ReservationIds.Num() == 2
			&& Receipt.ReservationIds[0] == Correlation.ActiveRunId)
		{
			FinalizedPrepareRequestIds.Add(Receipt.ReservationIds[1]);
		}
	}

	TArray<FInventoryBalance> Balances;
	Balances.Reserve(
		Correlation.OrderedRunInventoryItemInstanceIds.Num());
	TSet<FGuid> CorrelatedItemIds;
	for (int32 ItemIndex = 0;
		ItemIndex < Correlation.OrderedRunInventoryItemInstanceIds.Num();
		++ItemIndex)
	{
		const FGuid& ItemId =
			Correlation.OrderedRunInventoryItemInstanceIds[ItemIndex];
		if (!ItemId.IsValid() || CorrelatedItemIds.Contains(ItemId))
		{
			return Reject(
				EPlanStatus::ItemAuthorityInvalid,
				TEXT("Run correlation contains an invalid or duplicate inventory identity."));
		}
		CorrelatedItemIds.Add(ItemId);

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
		const FShanmenItemReservationSnapshot* QuantityReservation =
			FindQuantityReservation(
				Snapshot, Lifecycle->Receipt, ItemId,
				bDuplicateReservation);
		if (!Item
			|| Item->OwnerId != Correlation.OwnerId
			|| Item->RunId != Correlation.ScopeId
			|| !Definition
			|| !Definition->IsValid()
			|| !Definition->Supports(
				EShanmenItemResourceKind::Quantity)
			|| bDuplicateReservation
			|| !QuantityReservation
			|| !QuantityReservation->IsValid()
			|| QuantityReservation->State
				!= EShanmenItemReservationState::Committed
			|| QuantityReservation->OwnerId != Correlation.OwnerId
			|| QuantityReservation->RunId != Correlation.ScopeId)
		{
			return Reject(
				EPlanStatus::ItemAuthorityInvalid,
				TEXT("Prepared Run inventory has missing, duplicate, or invalid Quantity authority."));
		}

		int64 Consumed = 0;
		bool bHasPendingIntent = false;
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
					== EShanmenItemTransactionOperation::
						ConsumePreparedRunItem
				&& Receipt.ReservationId == Correlation.ActiveRunId
				&& Receipt.ItemInstanceId == ItemId)
			{
				Consumed += Receipt.Amount;
			}
			else if (Receipt.Operation
					== EShanmenItemTransactionOperation::
						FinalizePreparedRunQuantityIntent
				&& Receipt.Phase
					== EShanmenItemTransactionPhase::Committed
				&& Receipt.ItemInstanceId == ItemId
				&& Receipt.ReservationIds.Num() == 2
				&& Receipt.ReservationIds[0]
					== Correlation.ActiveRunId)
			{
				Consumed += Receipt.Amount;
			}
			else if (Receipt.Operation
					== EShanmenItemTransactionOperation::
						PreparePreparedRunQuantityIntent
				&& Receipt.ItemInstanceId == ItemId
				&& Receipt.ReservationIds
					== TArray<FGuid>({ Correlation.ActiveRunId })
				&& !FinalizedPrepareRequestIds.Contains(
					Receipt.RequestId))
			{
				bHasPendingIntent = true;
			}
		}
		if (Consumed < 0
			|| Consumed > QuantityReservation->Amount)
		{
			return Reject(
				EPlanStatus::ItemAuthorityInvalid,
				TEXT("Active-Run consumption exceeds one frozen inventory stack."));
		}

		FInventoryBalance& Balance = Balances.AddDefaulted_GetRef();
		Balance.ItemOrder = ItemIndex;
		Balance.ItemInstanceId = ItemId;
		Balance.MaterialDefinitionId = Definition->DefinitionId;
		Balance.QuantityBefore =
			QuantityReservation->Amount - static_cast<int32>(Consumed);
		Balance.QuantityRemaining = Balance.QuantityBefore;
		Balance.bHasPendingIntent = bHasPendingIntent;
	}

	Fdemo_mapShanmenFormationScatterResourcePlanResult Result;
	Result.Plan.Batch = Batch;
	Result.Plan.CorrelationId = Correlation.CorrelationId;
	Result.Plan.OwnerId = Correlation.OwnerId;
	Result.Plan.ScopeId = Correlation.ScopeId;
	Result.Plan.ActiveRunId = Correlation.ActiveRunId;
	Result.Plan.LifecycleReceiptId = Correlation.LifecycleReceiptId;
	Result.Plan.AuthorityRevision = Snapshot.AuthorityRevision;
	Result.Plan.Content = Snapshot.Content;
	Result.Plan.TotalRequirementCount = Batch.GetTotalRequirementCount();
	Result.Plan.TotalAllocatedQuantity = Batch.GetTotalMaterialQuantity();
	Result.Plan.PlanningScopeId =
		Fdemo_mapShanmenFormationScatterResourcePlan::
			BuildPlanningScopeId(Result.Plan);
	if (!Result.Plan.PlanningScopeId.IsValid())
	{
		return Reject(
			EPlanStatus::PlanInvalid,
			TEXT("Validated scatter evidence did not produce a planning scope identity."));
	}

	for (const auto& Anchor : Batch.GetAnchorIntents())
	{
		for (const auto& Material : Anchor.GetMaterialIntents())
		{
			int32 Remaining = Material.GetQuantity();
			bool bBlockedByPending = false;
			for (FInventoryBalance& Balance : Balances)
			{
				if (Balance.MaterialDefinitionId
						!= Material.GetMaterialDefinitionId()
					|| Balance.QuantityRemaining <= 0)
				{
					continue;
				}
				if (Balance.bHasPendingIntent)
				{
					bBlockedByPending = true;
					continue;
				}

				const int32 Quantity = FMath::Min(
					Remaining, Balance.QuantityRemaining);
				Fdemo_mapShanmenFormationScatterResourceSlice& Slice =
					Result.Plan.Slices.AddDefaulted_GetRef();
				Slice.PlanningScopeId = Result.Plan.PlanningScopeId;
				Slice.SliceOrder = Result.Plan.Slices.Num() - 1;
				Slice.AnchorIntentId = Anchor.GetIntentId();
				Slice.MaterialIntentId = Material.GetIntentId();
				Slice.AnchorOrder = Anchor.GetAnchorOrder();
				Slice.RequirementOrder =
					Material.GetRequirementOrder();
				Slice.MaterialDefinitionId =
					Material.GetMaterialDefinitionId();
				Slice.ItemInstanceId = Balance.ItemInstanceId;
				Slice.Quantity = Quantity;
				Slice.SourceQuantityBefore =
					Balance.QuantityRemaining;
				Balance.QuantityRemaining -= Quantity;
				Slice.SourceQuantityAfter =
					Balance.QuantityRemaining;
				Slice.SliceId =
					Fdemo_mapShanmenFormationScatterResourceSlice::
						BuildSliceId(Slice);
				if (!Slice.IsValid())
				{
					return Reject(
						EPlanStatus::PlanInvalid,
						TEXT("One deterministic scatter allocation slice is invalid."));
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
					bBlockedByPending
						? EPlanStatus::ConflictingIntent
						: EPlanStatus::QuantityUnavailable,
					bBlockedByPending
						? TEXT("A still-pending Quantity intent blocks one required material stack.")
						: TEXT("Prepared Run inventory cannot satisfy the complete scatter batch."));
			}
		}
	}

	for (const FInventoryBalance& Balance : Balances)
	{
		TArray<const Fdemo_mapShanmenFormationScatterResourceSlice*>
			ItemSlices;
		for (const auto& Slice : Result.Plan.Slices)
		{
			if (Slice.GetItemInstanceId() == Balance.ItemInstanceId)
			{
				ItemSlices.Add(&Slice);
			}
		}
		if (ItemSlices.IsEmpty())
		{
			continue;
		}

		auto& Reservation =
			Result.Plan.Reservations.AddDefaulted_GetRef();
		Reservation.PlanningScopeId = Result.Plan.PlanningScopeId;
		Reservation.ItemOrder = Balance.ItemOrder;
		Reservation.MaterialDefinitionId =
			Balance.MaterialDefinitionId;
		Reservation.ItemInstanceId = Balance.ItemInstanceId;
		Reservation.ExpectedQuantityBefore = Balance.QuantityBefore;
		Reservation.QuantityAfter = Balance.QuantityRemaining;
		for (const auto* Slice : ItemSlices)
		{
			Reservation.Quantity += Slice->GetQuantity();
			Reservation.SliceIds.Add(Slice->GetSliceId());
		}
		Reservation.IntentId =
			Fdemo_mapShanmenFormationScatterResourceReservationIntent::
				BuildIntentId(Reservation);
		Reservation.PrepareRequest.Context.RunId = Correlation.ScopeId;
		Reservation.PrepareRequest.Context.OwnerId = Correlation.OwnerId;
		Reservation.PrepareRequest.Context.Content = Snapshot.Content;
		Reservation.PrepareRequest.Context.RequestId =
			Fdemo_mapShanmenFormationScatterResourceReservationIntent::
				BuildRequestId(Reservation);
		Reservation.PrepareRequest.ActiveRunId = Correlation.ActiveRunId;
		Reservation.PrepareRequest.IntentId = Reservation.IntentId;
		Reservation.PrepareRequest.ItemInstanceId = Balance.ItemInstanceId;
		Reservation.PrepareRequest.Amount = Reservation.Quantity;
		Reservation.PrepareRequest.ExpectedQuantityBefore =
			Reservation.ExpectedQuantityBefore;
		Reservation.PrepareRequest.PurposeId = ScatterMaterialPurpose();
		if (!Reservation.IsValid())
		{
			return Reject(
				EPlanStatus::PlanInvalid,
				TEXT("One aggregated scatter material reservation is invalid."));
		}
	}

	Result.Plan.PlanId =
		Fdemo_mapShanmenFormationScatterResourcePlan::BuildPlanId(
			Result.Plan);
	if (!Result.Plan.IsValid())
	{
		return Reject(
			EPlanStatus::PlanInvalid,
			TEXT("The cumulative scatter resource plan failed self-validation."));
	}
	Result.Status = EPlanStatus::Planned;
	Result.Diagnostic =
		TEXT("The complete scatter batch was allocated cumulatively into one prepare command per physical stack.");
	return Result;
}

bool Fdemo_mapShanmenFormationScatterResourcePlanner::IsCurrentPlan(
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	if (!Plan.IsValid())
	{
		return false;
	}
	const auto Current = Fdemo_mapShanmenFormationScatterResourcePlanner::
		Plan(Projection, Deployment, Snapshot, Correlation, Plan.GetBatch());
	return Current.IsPlanned() && Current.Plan == Plan;
}
