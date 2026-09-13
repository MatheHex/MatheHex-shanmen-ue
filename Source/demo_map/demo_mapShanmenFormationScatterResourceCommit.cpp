#include "demo_mapShanmenFormationScatterResourceCommit.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	using ECommitStatus =
		Edemo_mapShanmenFormationScatterResourceCommitStatus;
	using EPreparationStatus =
		Edemo_mapShanmenFormationScatterResourcePreparationStatus;

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

	bool IsPrepareReceiptForReservation(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Reservation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		const FShanmenItemRunQuantityIntentRequest& Request =
			Reservation.GetPrepareRequest();
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					PreparePreparedRunQuantityIntent
			&& Receipt.Phase == EShanmenItemTransactionPhase::Reserved
			&& Receipt.RequestId == Request.Context.RequestId
			&& Receipt.ReservationId == Reservation.GetIntentId()
			&& Receipt.ItemInstanceId == Reservation.GetItemInstanceId()
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Reservation.GetQuantity()
			&& Receipt.ResourceBefore
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.ResourceAfter
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.AvailableAfter == Reservation.GetQuantityAfter()
			&& Receipt.PurposeId == Request.PurposeId
			&& Receipt.ReservationIds
				== TArray<FGuid>({ Plan.GetActiveRunId() });
	}

	bool IsCommitReceiptForReservation(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Reservation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		const FGuid* ExpectedRequestId = nullptr)
	{
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.Phase == EShanmenItemTransactionPhase::Committed
			&& (!ExpectedRequestId
				|| Receipt.RequestId == *ExpectedRequestId)
			&& Receipt.ReservationId == Reservation.GetIntentId()
			&& Receipt.ItemInstanceId == Reservation.GetItemInstanceId()
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Reservation.GetQuantity()
			&& Receipt.ResourceBefore
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.ResourceAfter == Reservation.GetQuantityAfter()
			&& Receipt.AvailableAfter == Reservation.GetQuantityAfter()
			&& Receipt.PurposeId
				== Reservation.GetPrepareRequest().PurposeId
			&& Receipt.ReservationIds
				== TArray<FGuid>({
					Plan.GetActiveRunId(),
					Reservation.GetPrepareRequest().Context.RequestId });
	}

	bool IsCancelledReceiptForReservation(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Reservation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.Phase == EShanmenItemTransactionPhase::Cancelled
			&& Receipt.ReservationId == Reservation.GetIntentId()
			&& Receipt.ItemInstanceId == Reservation.GetItemInstanceId()
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Reservation.GetQuantity()
			&& Receipt.ResourceBefore
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.ResourceAfter
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.AvailableAfter
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.PurposeId
				== Reservation.GetPrepareRequest().PurposeId
			&& Receipt.ReservationIds
				== TArray<FGuid>({
					Plan.GetActiveRunId(),
					Reservation.GetPrepareRequest().Context.RequestId });
	}

	struct FCommitState
	{
		bool bValid = false;
		FString Diagnostic;
		TArray<FShanmenItemTransactionReceipt> PrepareReceipts;
		TArray<FShanmenItemTransactionReceipt> CommitReceipts;
		int32 PendingCount = 0;
		int32 CommittedCount = 0;
		int32 CancelledCount = 0;
	};

	FCommitState CollectCommitState(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		FCommitState State;
		if (!Plan.IsValid())
		{
			State.Diagnostic = TEXT("Resource plan is invalid.");
			return State;
		}
		const auto& Reservations = Plan.GetReservations();
		State.PrepareReceipts.SetNum(Reservations.Num());
		State.CommitReceipts.SetNum(Reservations.Num());
		for (int32 Index = 0; Index < Reservations.Num(); ++Index)
		{
			const auto& Reservation = Reservations[Index];
			const FGuid PrepareRequestId =
				Reservation.GetPrepareRequest().Context.RequestId;
			const FShanmenItemTransactionReceipt* Prepare = nullptr;
			const FShanmenItemTransactionReceipt* Terminal = nullptr;
			int32 PrepareCount = 0;
			int32 TerminalCount = 0;
			for (const FShanmenItemProcessedRequestSnapshot& Processed :
				Snapshot.ProcessedRequests)
			{
				const FShanmenItemTransactionReceipt& Receipt =
					Processed.Receipt;
				if (Processed.RequestId == PrepareRequestId)
				{
					Prepare = &Receipt;
					++PrepareCount;
				}
				if (Receipt.IsSuccess()
					&& Receipt.Operation
						== EShanmenItemTransactionOperation::
							FinalizePreparedRunQuantityIntent
					&& Receipt.ReservationIds.Num() == 2
					&& Receipt.ReservationIds[1] == PrepareRequestId)
				{
					Terminal = &Receipt;
					++TerminalCount;
				}
			}
			if (PrepareCount != 1 || TerminalCount > 1 || !Prepare
				|| !IsPrepareReceiptForReservation(
					*Prepare, Reservation, Plan))
			{
				State.Diagnostic = FString::Printf(
					TEXT("Reservation %d lacks one exact durable prepare."),
					Index);
				return State;
			}
			State.PrepareReceipts[Index] = *Prepare;
			if (!Terminal)
			{
				++State.PendingCount;
				continue;
			}
			if (Terminal->Phase == EShanmenItemTransactionPhase::Committed
				&& IsCommitReceiptForReservation(
					*Terminal, Reservation, Plan))
			{
				State.CommitReceipts[Index] = *Terminal;
				++State.CommittedCount;
				continue;
			}
			if (Terminal->Phase == EShanmenItemTransactionPhase::Cancelled
				&& IsCancelledReceiptForReservation(
					*Terminal, Reservation, Plan))
			{
				++State.CancelledCount;
				continue;
			}
			State.Diagnostic = FString::Printf(
				TEXT("Reservation %d has terminal evidence that does not match the frozen plan."),
				Index);
			return State;
		}
		if (State.CommittedCount > 0 && State.CancelledCount > 0)
		{
			State.Diagnostic =
				TEXT("One scatter plan mixes successful commit and cancellation terminals.");
			return State;
		}
		State.bValid = true;
		State.Diagnostic = TEXT("Exact commit authority state reconstructed.");
		return State;
	}

	FGuid BuildCommitPassId(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FCommitState& State)
	{
		if (!Plan.IsValid() || !State.bValid
			|| Snapshot.AuthorityRevision < 0
			|| State.CommitReceipts.Num()
				!= Plan.GetReservations().Num())
		{
			return FGuid();
		}
		TArray<FString> Parts =
		{
			GuidDigits(Plan.GetPlanId()),
			FString::FromInt(Snapshot.AuthorityRevision),
			FString::FromInt(State.CommittedCount),
			FString::FromInt(State.PendingCount)
		};
		for (const FShanmenItemTransactionReceipt& Receipt :
			State.CommitReceipts)
		{
			Parts.Add(Receipt.IsSuccess()
				? GuidDigits(Receipt.ReceiptId)
				: TEXT("pending"));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Formation.ScatterResourceCommitPass.r1"),
			Parts);
	}

	Fdemo_mapShanmenFormationScatterResourceCommitResult MakeResult(
		const ECommitStatus Status,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterResourceCommitResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Plan = Plan;
		if (Plan.IsValid())
		{
			Result.PrepareReceipts.SetNum(Plan.GetReservations().Num());
			Result.CommitReceipts.SetNum(Plan.GetReservations().Num());
		}
		return Result;
	}

	void ApplyState(
		const FCommitState& State,
		Fdemo_mapShanmenFormationScatterResourceCommitResult& Result)
	{
		Result.PrepareReceipts = State.PrepareReceipts;
		Result.CommitReceipts = State.CommitReceipts;
	}

	class FProductAuthority final
		: public Idemo_mapShanmenFormationScatterResourceAuthority
	{
	public:
		explicit FProductAuthority(
			Udemo_mapShanmenItemAuthoritySubsystem& InAuthority)
			: Authority(InAuthority)
		{
		}

		virtual bool IsReady() const override
		{
			return Authority.GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
		}

		virtual bool TryCaptureSnapshot(
			FShanmenItemAuthoritySnapshot& OutSnapshot) const override
		{
			return Authority.TryCaptureSnapshot(OutSnapshot);
		}

		virtual FShanmenItemDurableCommandResult PrepareQuantity(
			const FShanmenItemRunQuantityIntentRequest& Request) override
		{
			return Authority.PreparePreparedRunQuantityIntentDurable(Request);
		}

		virtual FShanmenItemDurableCommandResult FinalizeQuantity(
			const FShanmenItemRunQuantityIntentFinalizeRequest& Request)
			override
		{
			return Authority.FinalizePreparedRunQuantityIntentDurable(Request);
		}

	private:
		Udemo_mapShanmenItemAuthoritySubsystem& Authority;
	};
}

FGuid Fdemo_mapShanmenFormationScatterResourceFulfillmentLine::BuildLineId(
	const Fdemo_mapShanmenFormationScatterResourceFulfillmentLine& Line)
{
	if (!Line.PlanId.IsValid() || !Line.SliceId.IsValid()
		|| Line.SliceOrder < 0 || !Line.AnchorIntentId.IsValid()
		|| !Line.MaterialIntentId.IsValid() || Line.AnchorOrder < 0
		|| Line.RequirementOrder < 0
		|| Line.MaterialDefinitionId.IsNone()
		|| !Line.ReservationIntentId.IsValid()
		|| !Line.ItemInstanceId.IsValid() || Line.Quantity <= 0
		|| !Line.CommitReceiptId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourceFulfillmentLine.r1"),
		{
			GuidDigits(Line.PlanId),
			GuidDigits(Line.SliceId),
			FString::FromInt(Line.SliceOrder),
			GuidDigits(Line.AnchorIntentId),
			GuidDigits(Line.MaterialIntentId),
			FString::FromInt(Line.AnchorOrder),
			FString::FromInt(Line.RequirementOrder),
			CanonicalName(Line.MaterialDefinitionId),
			GuidDigits(Line.ReservationIntentId),
			GuidDigits(Line.ItemInstanceId),
			FString::FromInt(Line.Quantity),
			GuidDigits(Line.CommitReceiptId)
		});
}

bool Fdemo_mapShanmenFormationScatterResourceFulfillmentLine::IsValid()
	const
{
	return LineId.IsValid() && LineId == BuildLineId(*this);
}

bool Fdemo_mapShanmenFormationScatterResourceFulfillmentLine::operator==(
	const Fdemo_mapShanmenFormationScatterResourceFulfillmentLine& Other)
	const
{
	return LineId == Other.LineId && PlanId == Other.PlanId
		&& SliceId == Other.SliceId && SliceOrder == Other.SliceOrder
		&& AnchorIntentId == Other.AnchorIntentId
		&& MaterialIntentId == Other.MaterialIntentId
		&& AnchorOrder == Other.AnchorOrder
		&& RequirementOrder == Other.RequirementOrder
		&& MaterialDefinitionId == Other.MaterialDefinitionId
		&& ReservationIntentId == Other.ReservationIntentId
		&& ItemInstanceId == Other.ItemInstanceId
		&& Quantity == Other.Quantity
		&& CommitReceiptId == Other.CommitReceiptId;
}

FGuid Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment::
BuildFulfillmentId(
	const Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment&
		Fulfillment)
{
	if (!Fulfillment.PlanId.IsValid()
		|| !Fulfillment.BatchIntentId.IsValid()
		|| !Fulfillment.AnchorIntentId.IsValid()
		|| Fulfillment.AnchorOrder < 0
		|| Fulfillment.AnchorDefinitionId.IsNone()
		|| !Fulfillment.AnchorInstanceId.IsValid()
		|| Fulfillment.Lines.IsEmpty()
		|| Fulfillment.RequirementCount <= 0
		|| Fulfillment.TotalCommittedQuantity <= 0)
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Fulfillment.PlanId),
		GuidDigits(Fulfillment.BatchIntentId),
		GuidDigits(Fulfillment.AnchorIntentId),
		FString::FromInt(Fulfillment.AnchorOrder),
		CanonicalName(Fulfillment.AnchorDefinitionId),
		GuidDigits(Fulfillment.AnchorInstanceId),
		FString::FromInt(Fulfillment.RequirementCount),
		LexToString(Fulfillment.TotalCommittedQuantity),
		FString::FromInt(Fulfillment.Lines.Num())
	};
	for (const auto& Line : Fulfillment.Lines)
	{
		if (!Line.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Line.GetLineId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterAnchorResourceFulfillment.r1"),
		Parts);
}

bool Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment::IsValid()
	const
{
	if (!FulfillmentId.IsValid()
		|| FulfillmentId != BuildFulfillmentId(*this))
	{
		return false;
	}
	TSet<FGuid> LineIds;
	TSet<FGuid> SliceIds;
	TSet<FGuid> MaterialIntentIds;
	int32 PreviousSliceOrder = INDEX_NONE;
	int64 Quantity = 0;
	for (const auto& Line : Lines)
	{
		if (!Line.IsValid() || Line.GetPlanId() != PlanId
			|| Line.GetAnchorIntentId() != AnchorIntentId
			|| Line.GetAnchorOrder() != AnchorOrder
			|| Line.GetSliceOrder() <= PreviousSliceOrder
			|| LineIds.Contains(Line.GetLineId())
			|| SliceIds.Contains(Line.GetSliceId()))
		{
			return false;
		}
		PreviousSliceOrder = Line.GetSliceOrder();
		LineIds.Add(Line.GetLineId());
		SliceIds.Add(Line.GetSliceId());
		MaterialIntentIds.Add(Line.GetMaterialIntentId());
		Quantity += Line.GetQuantity();
	}
	return MaterialIntentIds.Num() == RequirementCount
		&& Quantity == TotalCommittedQuantity;
}

bool Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment::operator==(
	const Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment& Other)
	const
{
	return FulfillmentId == Other.FulfillmentId
		&& PlanId == Other.PlanId
		&& BatchIntentId == Other.BatchIntentId
		&& AnchorIntentId == Other.AnchorIntentId
		&& AnchorOrder == Other.AnchorOrder
		&& AnchorDefinitionId == Other.AnchorDefinitionId
		&& AnchorInstanceId == Other.AnchorInstanceId
		&& Lines == Other.Lines
		&& RequirementCount == Other.RequirementCount
		&& TotalCommittedQuantity == Other.TotalCommittedQuantity;
}

FGuid Fdemo_mapShanmenFormationScatterResourceCommitEvidence::BuildEvidenceId(
	const Fdemo_mapShanmenFormationScatterResourceCommitEvidence& Evidence)
{
	if (!Evidence.Plan.IsValid()
		|| Evidence.CommitReceipts.IsEmpty()
		|| Evidence.AnchorFulfillments.IsEmpty()
		|| Evidence.RequirementCount <= 0
		|| Evidence.TotalCommittedQuantity <= 0)
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Evidence.Plan.GetPlanId()),
		FString::FromInt(Evidence.RequirementCount),
		LexToString(Evidence.TotalCommittedQuantity),
		FString::FromInt(Evidence.CommitReceipts.Num()),
		FString::FromInt(Evidence.AnchorFulfillments.Num())
	};
	for (const FShanmenItemTransactionReceipt& Receipt :
		Evidence.CommitReceipts)
	{
		if (!Receipt.IsSuccess())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Receipt.ReceiptId));
		Parts.Add(GuidDigits(Receipt.RequestId));
	}
	for (const auto& Fulfillment : Evidence.AnchorFulfillments)
	{
		if (!Fulfillment.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Fulfillment.GetFulfillmentId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterResourceCommitEvidence.r1"),
		Parts);
}

bool Fdemo_mapShanmenFormationScatterResourceCommitEvidence::IsValid()
	const
{
	if (!EvidenceId.IsValid() || !Plan.IsValid()
		|| CommitReceipts.Num() != Plan.GetReservations().Num()
		|| AnchorFulfillments.Num()
			!= Plan.GetBatch().GetAnchorIntents().Num()
		|| RequirementCount != Plan.GetTotalRequirementCount()
		|| TotalCommittedQuantity != Plan.GetTotalAllocatedQuantity())
	{
		return false;
	}

	TSet<FGuid> ReceiptIds;
	for (int32 Index = 0; Index < CommitReceipts.Num(); ++Index)
	{
		const FShanmenItemTransactionReceipt& Receipt =
			CommitReceipts[Index];
		if (!IsCommitReceiptForReservation(
				Receipt, Plan.GetReservations()[Index], Plan)
			|| ReceiptIds.Contains(Receipt.ReceiptId))
		{
			return false;
		}
		ReceiptIds.Add(Receipt.ReceiptId);
	}

	TSet<FGuid> CoveredSlices;
	TMap<FGuid, int64> QuantityByMaterialIntent;
	int64 TotalLineQuantity = 0;
	for (int32 AnchorIndex = 0;
		AnchorIndex < AnchorFulfillments.Num(); ++AnchorIndex)
	{
		const auto& Fulfillment = AnchorFulfillments[AnchorIndex];
		const auto& Anchor =
			Plan.GetBatch().GetAnchorIntents()[AnchorIndex];
		if (!Fulfillment.IsValid()
			|| Fulfillment.GetPlanId() != Plan.GetPlanId()
			|| Fulfillment.GetBatchIntentId()
				!= Plan.GetBatch().GetBatchIntentId()
			|| Fulfillment.GetAnchorIntentId() != Anchor.GetIntentId()
			|| Fulfillment.GetAnchorOrder() != Anchor.GetAnchorOrder()
			|| Fulfillment.GetAnchorDefinitionId()
				!= Anchor.GetAnchorDefinitionId()
			|| Fulfillment.GetAnchorInstanceId()
				!= Anchor.GetAnchorInstanceId()
			|| Fulfillment.GetRequirementCount()
				!= Anchor.GetMaterialIntents().Num()
			|| Fulfillment.GetTotalCommittedQuantity()
				!= Anchor.GetTotalMaterialQuantity())
		{
			return false;
		}

		for (const auto& Line : Fulfillment.GetLines())
		{
			if (!Plan.GetSlices().IsValidIndex(Line.GetSliceOrder()))
			{
				return false;
			}
			const auto& Slice = Plan.GetSlices()[Line.GetSliceOrder()];
			if (CoveredSlices.Contains(Line.GetSliceId())
				|| Line.GetSliceId() != Slice.GetSliceId()
				|| Line.GetAnchorIntentId() != Slice.GetAnchorIntentId()
				|| Line.GetMaterialIntentId()
					!= Slice.GetMaterialIntentId()
				|| Line.GetAnchorOrder() != Slice.GetAnchorOrder()
				|| Line.GetRequirementOrder()
					!= Slice.GetRequirementOrder()
				|| Line.GetMaterialDefinitionId()
					!= Slice.GetMaterialDefinitionId()
				|| Line.GetItemInstanceId()
					!= Slice.GetItemInstanceId()
				|| Line.GetQuantity() != Slice.GetQuantity())
			{
				return false;
			}

			int32 ReservationIndex = INDEX_NONE;
			for (int32 Candidate = 0;
				Candidate < Plan.GetReservations().Num(); ++Candidate)
			{
				if (Plan.GetReservations()[Candidate].GetSliceIds().Contains(
						Slice.GetSliceId()))
				{
					if (ReservationIndex != INDEX_NONE)
					{
						return false;
					}
					ReservationIndex = Candidate;
				}
			}
			if (ReservationIndex == INDEX_NONE
				|| Line.GetReservationIntentId()
					!= Plan.GetReservations()[ReservationIndex].GetIntentId()
				|| Line.GetCommitReceiptId()
					!= CommitReceipts[ReservationIndex].ReceiptId)
			{
				return false;
			}
			CoveredSlices.Add(Line.GetSliceId());
			QuantityByMaterialIntent.FindOrAdd(
				Line.GetMaterialIntentId()) += Line.GetQuantity();
			TotalLineQuantity += Line.GetQuantity();
		}
	}

	for (const auto& Anchor : Plan.GetBatch().GetAnchorIntents())
	{
		for (const auto& Material : Anchor.GetMaterialIntents())
		{
			const int64* Quantity = QuantityByMaterialIntent.Find(
				Material.GetIntentId());
			if (!Quantity || *Quantity != Material.GetQuantity())
			{
				return false;
			}
		}
	}
	return CoveredSlices.Num() == Plan.GetSlices().Num()
		&& TotalLineQuantity == TotalCommittedQuantity
		&& EvidenceId == BuildEvidenceId(*this);
}

bool Fdemo_mapShanmenFormationScatterResourceCommitEvidence::operator==(
	const Fdemo_mapShanmenFormationScatterResourceCommitEvidence& Other)
	const
{
	return EvidenceId == Other.EvidenceId && Plan == Other.Plan
		&& CommitReceipts == Other.CommitReceipts
		&& AnchorFulfillments == Other.AnchorFulfillments
		&& RequirementCount == Other.RequirementCount
		&& TotalCommittedQuantity == Other.TotalCommittedQuantity;
}

bool Fdemo_mapShanmenFormationScatterResourceCommitResult::IsValid() const
{
	if (Status == ECommitStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (!Plan.IsValid())
	{
		return Status == ECommitStatus::PlanInvalid;
	}
	const int32 ReservationCount = Plan.GetReservations().Num();
	if ((PrepareReceipts.Num() != 0
			&& PrepareReceipts.Num() != ReservationCount)
		|| (CommitReceipts.Num() != 0
			&& CommitReceipts.Num() != ReservationCount))
	{
		return false;
	}
	if (Status == ECommitStatus::Committed
		|| Status == ECommitStatus::Replayed)
	{
		return IsCommitted();
	}
	if (Status == ECommitStatus::AttemptCancelled)
	{
		return Preparation.IsValid() && Preparation.IsCancelled();
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterResourceCommitResult::IsCommitted()
	const
{
	if ((Status != ECommitStatus::Committed
			&& Status != ECommitStatus::Replayed)
		|| !Plan.IsValid() || !Evidence.IsValid()
		|| PrepareReceipts.Num() != Plan.GetReservations().Num()
		|| CommitReceipts.Num() != Plan.GetReservations().Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Plan.GetReservations().Num(); ++Index)
	{
		if (!IsPrepareReceiptForReservation(
				PrepareReceipts[Index], Plan.GetReservations()[Index], Plan)
			|| !IsCommitReceiptForReservation(
				CommitReceipts[Index], Plan.GetReservations()[Index], Plan))
		{
			return false;
		}
	}
	return Evidence.GetCommitReceipts() == CommitReceipts;
}

bool Fdemo_mapShanmenFormationScatterResourceCommitResult::RequiresRecovery()
	const
{
	return Status == ECommitStatus::CommitRetryRequired
		|| Status == ECommitStatus::ForwardRecoveryRequired;
}

bool Fdemo_mapShanmenFormationScatterResourceCommitter::BuildCommitRequest(
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
	const int32 ReservationIndex,
	const FGuid& CommitPassId,
	FShanmenItemRunQuantityIntentFinalizeRequest& OutRequest)
{
	OutRequest = FShanmenItemRunQuantityIntentFinalizeRequest();
	if (!Plan.IsValid() || !CommitPassId.IsValid()
		|| !Plan.GetReservations().IsValidIndex(ReservationIndex))
	{
		return false;
	}
	const auto& Reservation = Plan.GetReservations()[ReservationIndex];
	OutRequest.Context.RunId = Plan.GetScopeId();
	OutRequest.Context.OwnerId = Plan.GetOwnerId();
	OutRequest.Context.RequestId =
		FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Formation.ScatterResourceCommitRequest.r1"),
			{
				GuidDigits(CommitPassId),
				GuidDigits(Plan.GetPlanId()),
				GuidDigits(Reservation.GetIntentId()),
				GuidDigits(
					Reservation.GetPrepareRequest().Context.RequestId),
				GuidDigits(Reservation.GetItemInstanceId())
			});
	OutRequest.Context.Content = Plan.GetContent();
	OutRequest.ActiveRunId = Plan.GetActiveRunId();
	OutRequest.PrepareRequestId =
		Reservation.GetPrepareRequest().Context.RequestId;
	OutRequest.IntentId = Reservation.GetIntentId();
	OutRequest.ItemInstanceId = Reservation.GetItemInstanceId();
	OutRequest.bCommit = true;
	return OutRequest.IsValid();
}

bool Fdemo_mapShanmenFormationScatterResourceCommitter::BuildEvidence(
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
	const TArray<FShanmenItemTransactionReceipt>& CommitReceipts,
	Fdemo_mapShanmenFormationScatterResourceCommitEvidence& OutEvidence)
{
	OutEvidence =
		Fdemo_mapShanmenFormationScatterResourceCommitEvidence();
	if (!Plan.IsValid()
		|| CommitReceipts.Num() != Plan.GetReservations().Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < CommitReceipts.Num(); ++Index)
	{
		if (!IsCommitReceiptForReservation(
				CommitReceipts[Index], Plan.GetReservations()[Index], Plan))
		{
			return false;
		}
	}

	OutEvidence.Plan = Plan;
	OutEvidence.CommitReceipts = CommitReceipts;
	OutEvidence.RequirementCount = Plan.GetTotalRequirementCount();
	OutEvidence.TotalCommittedQuantity = Plan.GetTotalAllocatedQuantity();
	for (const auto& Anchor : Plan.GetBatch().GetAnchorIntents())
	{
		Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment
			Fulfillment;
		Fulfillment.PlanId = Plan.GetPlanId();
		Fulfillment.BatchIntentId = Plan.GetBatch().GetBatchIntentId();
		Fulfillment.AnchorIntentId = Anchor.GetIntentId();
		Fulfillment.AnchorOrder = Anchor.GetAnchorOrder();
		Fulfillment.AnchorDefinitionId = Anchor.GetAnchorDefinitionId();
		Fulfillment.AnchorInstanceId = Anchor.GetAnchorInstanceId();
		Fulfillment.RequirementCount = Anchor.GetMaterialIntents().Num();
		Fulfillment.TotalCommittedQuantity =
			Anchor.GetTotalMaterialQuantity();

		for (const auto& Slice : Plan.GetSlices())
		{
			if (Slice.GetAnchorIntentId() != Anchor.GetIntentId())
			{
				continue;
			}
			int32 ReservationIndex = INDEX_NONE;
			for (int32 Candidate = 0;
				Candidate < Plan.GetReservations().Num(); ++Candidate)
			{
				if (Plan.GetReservations()[Candidate].GetSliceIds().Contains(
						Slice.GetSliceId()))
				{
					if (ReservationIndex != INDEX_NONE)
					{
						return false;
					}
					ReservationIndex = Candidate;
				}
			}
			if (ReservationIndex == INDEX_NONE)
			{
				return false;
			}

			Fdemo_mapShanmenFormationScatterResourceFulfillmentLine Line;
			Line.PlanId = Plan.GetPlanId();
			Line.SliceId = Slice.GetSliceId();
			Line.SliceOrder = Slice.GetSliceOrder();
			Line.AnchorIntentId = Slice.GetAnchorIntentId();
			Line.MaterialIntentId = Slice.GetMaterialIntentId();
			Line.AnchorOrder = Slice.GetAnchorOrder();
			Line.RequirementOrder = Slice.GetRequirementOrder();
			Line.MaterialDefinitionId = Slice.GetMaterialDefinitionId();
			Line.ReservationIntentId =
				Plan.GetReservations()[ReservationIndex].GetIntentId();
			Line.ItemInstanceId = Slice.GetItemInstanceId();
			Line.Quantity = Slice.GetQuantity();
			Line.CommitReceiptId =
				CommitReceipts[ReservationIndex].ReceiptId;
			Line.LineId =
				Fdemo_mapShanmenFormationScatterResourceFulfillmentLine::
					BuildLineId(Line);
			if (!Line.IsValid())
			{
				return false;
			}
			Fulfillment.Lines.Add(Line);
		}
		Fulfillment.FulfillmentId =
			Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment::
				BuildFulfillmentId(Fulfillment);
		if (!Fulfillment.IsValid())
		{
			return false;
		}
		OutEvidence.AnchorFulfillments.Add(Fulfillment);
	}
	OutEvidence.EvidenceId =
		Fdemo_mapShanmenFormationScatterResourceCommitEvidence::
			BuildEvidenceId(OutEvidence);
	return OutEvidence.IsValid();
}

Fdemo_mapShanmenFormationScatterResourceCommitResult
Fdemo_mapShanmenFormationScatterResourceCommitter::Commit(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	FProductAuthority ProductAuthority(Authority);
	return CommitWithAuthority(
		ProductAuthority, Projection, Deployment, Correlation, Plan);
}

Fdemo_mapShanmenFormationScatterResourceCommitResult
Fdemo_mapShanmenFormationScatterResourceCommitter::CommitWithAuthority(
	Idemo_mapShanmenFormationScatterResourceAuthority& Authority,
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	if (!Plan.IsValid())
	{
		return MakeResult(
			ECommitStatus::PlanInvalid, Plan,
			TEXT("Scatter resource commit requires a valid immutable P27.18 plan."));
	}
	if (!IsInGameThread() || !Authority.IsReady())
	{
		return MakeResult(
			ECommitStatus::AuthorityNotReady, Plan,
			TEXT("Scatter resource commit requires the ready serialized item authority on the Game Thread."));
	}

	const auto Preparation =
		Fdemo_mapShanmenFormationScatterResourcePreparation::
			PrepareWithAuthority(
				Authority, Projection, Deployment, Correlation, Plan);
	Fdemo_mapShanmenFormationScatterResourceCommitResult Result =
		MakeResult(ECommitStatus::Invalid, Plan, Preparation.Diagnostic);
	Result.Preparation = Preparation;
	Result.PrepareReceipts = Preparation.PrepareReceipts;
	if (Preparation.Status == EPreparationStatus::AttemptCancelled
		|| Preparation.Status == EPreparationStatus::PlanStaleRolledBack
		|| Preparation.Status
			== EPreparationStatus::PrepareRejectedRolledBack)
	{
		Result.Status = ECommitStatus::AttemptCancelled;
		Result.Diagnostic =
			TEXT("The scatter resource attempt is terminally cancelled; no commit was issued.");
		return Result;
	}
	if (Preparation.Status != EPreparationStatus::Prepared
		&& Preparation.Status != EPreparationStatus::Replayed
		&& Preparation.Status
			!= EPreparationStatus::ForwardRecoveryRequired)
	{
		Result.Status = ECommitStatus::PreparationRejected;
		Result.Diagnostic = FString::Printf(
			TEXT("Whole-batch preparation did not reach a commit-safe state: %s"),
			*Preparation.Diagnostic);
		return Result;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		Result.Status = ECommitStatus::SnapshotUnavailable;
		Result.Diagnostic =
			TEXT("The ready item authority could not provide a pre-commit snapshot.");
		return Result;
	}
	FCommitState State = CollectCommitState(Snapshot, Plan);
	if (!State.bValid)
	{
		Result.Status = ECommitStatus::EvidenceInvalid;
		Result.Diagnostic = State.Diagnostic;
		return Result;
	}
	ApplyState(State, Result);
	if (State.CancelledCount > 0)
	{
		Result.Status = ECommitStatus::AttemptCancelled;
		Result.Diagnostic =
			TEXT("A prior terminal cancellation prevents resource commit.");
		return Result;
	}
	if (State.CommittedCount == Plan.GetReservations().Num())
	{
		if (!BuildEvidence(Plan, State.CommitReceipts, Result.Evidence))
		{
			Result.Status = ECommitStatus::EvidenceInvalid;
			Result.Diagnostic =
				TEXT("All resources are committed, but exact anchor fulfillment evidence is invalid.");
			return Result;
		}
		Result.Status = ECommitStatus::Replayed;
		Result.Diagnostic =
			TEXT("The complete committed resource result replayed from durable authority evidence.");
		return Result;
	}
	if (State.PendingCount <= 0)
	{
		Result.Status = ECommitStatus::EvidenceInvalid;
		Result.Diagnostic =
			TEXT("Commit state is incomplete without a pending or committed reservation.");
		return Result;
	}

	const FGuid CommitPassId = BuildCommitPassId(Plan, Snapshot, State);
	if (!CommitPassId.IsValid())
	{
		Result.Status = ECommitStatus::EvidenceInvalid;
		Result.Diagnostic =
			TEXT("A deterministic forward commit pass identity could not be derived.");
		return Result;
	}

	for (int32 Index = 0; Index < Plan.GetReservations().Num(); ++Index)
	{
		if (Result.CommitReceipts[Index].IsSuccess())
		{
			continue;
		}
		FShanmenItemRunQuantityIntentFinalizeRequest Request;
		if (!BuildCommitRequest(Plan, Index, CommitPassId, Request))
		{
			Result.Status = State.CommittedCount > 0
				? ECommitStatus::ForwardRecoveryRequired
				: ECommitStatus::CommitRetryRequired;
			Result.Diagnostic = FString::Printf(
				TEXT("Commit request %d could not be built."), Index);
			return Result;
		}

		const FShanmenItemDurableCommandResult Command =
			Authority.FinalizeQuantity(Request);
		Result.CommitCommands.Add(Command);
		if (!Command.IsCommandSuccess()
			|| !IsCommitReceiptForReservation(
				Command.Receipt, Plan.GetReservations()[Index], Plan,
				&Request.Context.RequestId))
		{
			FShanmenItemAuthoritySnapshot FailedSnapshot;
			if (Authority.IsReady()
				&& Authority.TryCaptureSnapshot(FailedSnapshot))
			{
				const FCommitState FailedState =
					CollectCommitState(FailedSnapshot, Plan);
				if (FailedState.bValid)
				{
					ApplyState(FailedState, Result);
					State = FailedState;
				}
			}
			Result.Status = State.CommittedCount > 0
				? ECommitStatus::ForwardRecoveryRequired
				: ECommitStatus::CommitRetryRequired;
			Result.Diagnostic = FString::Printf(
				TEXT("Commit line %d was not durably accepted; %s"),
				Index,
				State.CommittedCount > 0
					? TEXT("the irreversible prefix must continue forward")
					: TEXT("all prepares remain pending for a later retry"));
			return Result;
		}
		Result.CommitReceipts[Index] = Command.Receipt;
		++State.CommittedCount;
		--State.PendingCount;
	}

	FShanmenItemAuthoritySnapshot FinalSnapshot;
	if (!Authority.IsReady()
		|| !Authority.TryCaptureSnapshot(FinalSnapshot))
	{
		Result.Status = ECommitStatus::ForwardRecoveryRequired;
		Result.Diagnostic =
			TEXT("Commit commands completed, but final durable evidence could not be captured.");
		return Result;
	}
	const FCommitState Final = CollectCommitState(FinalSnapshot, Plan);
	if (!Final.bValid)
	{
		Result.Status = ECommitStatus::EvidenceInvalid;
		Result.Diagnostic = Final.Diagnostic;
		return Result;
	}
	ApplyState(Final, Result);
	if (Final.CancelledCount > 0)
	{
		Result.Status = ECommitStatus::EvidenceInvalid;
		Result.Diagnostic =
			TEXT("A cancellation appeared during the forward-only commit pass.");
		return Result;
	}
	if (Final.CommittedCount != Plan.GetReservations().Num()
		|| Final.PendingCount != 0)
	{
		Result.Status = Final.CommittedCount > 0
			? ECommitStatus::ForwardRecoveryRequired
			: ECommitStatus::CommitRetryRequired;
		Result.Diagnostic =
			TEXT("The bounded commit pass ended without a complete durable terminal set.");
		return Result;
	}
	if (!BuildEvidence(Plan, Final.CommitReceipts, Result.Evidence))
	{
		Result.Status = ECommitStatus::EvidenceInvalid;
		Result.Diagnostic =
			TEXT("Committed receipts could not be attributed exactly to every authored anchor requirement.");
		return Result;
	}
	Result.Status = ECommitStatus::Committed;
	Result.Diagnostic =
		TEXT("Every prepared physical stack is durably committed and attributed to its authored scatter anchors.");
	return Result;
}
