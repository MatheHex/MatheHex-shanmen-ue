#include "demo_mapShanmenMeridianShockTreatmentAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	FName TreatmentPurpose()
	{
		return TEXT("Shanmen.Condition.MeridianShock.Treatment.r1");
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	template <typename TValue, typename TPredicate>
	const TValue* FindBy(const TArray<TValue>& Values, TPredicate Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	Fdemo_mapShanmenMeridianShockTreatmentItemResult Reject(
		const Edemo_mapShanmenMeridianShockTreatmentStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenMeridianShockTreatmentItemResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	FGuid MakePrepareRequestId(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent,
		const FShanmenContentStamp& Content,
		const int32 Quantity)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.MeridianShockTreatment.QuantityPrepare.r1"),
			{
				GuidDigits(Correlation.CorrelationId),
				GuidDigits(Correlation.ActiveRunId),
				GuidDigits(Intent.GetTreatmentId()),
				GuidDigits(Intent.GetItemInstanceId()),
				FString::FromInt(Quantity),
				Content.Version.ToString(),
				Content.Digest
			});
	}

	FGuid MakeFinalizeRequestId(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenItemRunQuantityIntentRequest& Prepare)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.MeridianShockTreatment.QuantityFinalize.r1"),
			{
				GuidDigits(Correlation.CorrelationId),
				GuidDigits(Prepare.ActiveRunId),
				GuidDigits(Prepare.Context.RequestId),
				GuidDigits(Prepare.IntentId),
				GuidDigits(Prepare.ItemInstanceId)
			});
	}

	bool IsPreparedStatus(
		const Edemo_mapShanmenMeridianShockTreatmentStatus Status)
	{
		return Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::Prepared
			|| Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::Replayed;
	}

	Fdemo_mapShanmenMeridianShockTreatmentItemResult BuildFinalizeRequest(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
		const bool bCommit)
	{
		if (!Correlation.IsValid())
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::
					RunCorrelationInvalid,
				TEXT("Treatment finalization requires one valid immutable Run correlation."));
		}
		if (!Preparation.IsPrepared())
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::
					PreparationInvalid,
				TEXT("Treatment finalization requires one durable prepared Quantity intent."));
		}
		const FShanmenItemRunQuantityIntentRequest& Prepare =
			Preparation.PrepareRequest;
		const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent =
			Preparation.TreatmentIntent;
		if (Intent.GetRunId() != Correlation.ActiveRunId
			|| Prepare.Context.RunId != Correlation.ScopeId
			|| Prepare.Context.OwnerId != Correlation.OwnerId
			|| Prepare.ActiveRunId != Correlation.ActiveRunId
			|| Prepare.IntentId != Intent.GetTreatmentId()
			|| Prepare.ItemInstanceId != Intent.GetItemInstanceId())
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::
					ConditionMismatch,
				TEXT("Prepared treatment identity does not belong to this active Run."));
		}

		Fdemo_mapShanmenMeridianShockTreatmentItemResult Result = Preparation;
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady;
		Result.Diagnostic = bCommit
			? TEXT("Exact treatment proof authorizes one durable Quantity commit request.")
			: TEXT("Pre-treatment cancellation authorizes one durable Quantity release request.");
		Result.FinalizeCommand = FShanmenItemDurableCommandResult();
		Result.FinalizeRequest = FShanmenItemRunQuantityIntentFinalizeRequest();
		Result.FinalizeRequest.Context.RunId = Correlation.ScopeId;
		Result.FinalizeRequest.Context.OwnerId = Correlation.OwnerId;
		Result.FinalizeRequest.Context.RequestId =
			MakeFinalizeRequestId(Correlation, Prepare);
		Result.FinalizeRequest.Context.Content = Prepare.Context.Content;
		Result.FinalizeRequest.ActiveRunId = Prepare.ActiveRunId;
		Result.FinalizeRequest.PrepareRequestId = Prepare.Context.RequestId;
		Result.FinalizeRequest.IntentId = Prepare.IntentId;
		Result.FinalizeRequest.ItemInstanceId = Prepare.ItemInstanceId;
		Result.FinalizeRequest.bCommit = bCommit;
		if (!Result.FinalizeRequest.IsValid())
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::RequestInvalid,
				TEXT("Validated treatment evidence produced an invalid finalize request."));
		}
		return Result;
	}

	Fdemo_mapShanmenMeridianShockTreatmentItemResult ExecuteFinalize(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapShanmenMeridianShockTreatmentItemResult Result)
	{
		if (!IsInGameThread()
			|| Authority.GetLifecycleState()
				!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::AuthorityNotReady,
				TEXT("Treatment finalization requires the ready item authority on the Game Thread."));
		}
		Result.FinalizeCommand =
			Authority.FinalizePreparedRunQuantityIntentDurable(
				Result.FinalizeRequest);
		if (!Result.FinalizeCommand.IsCommandSuccess())
		{
			Result.Status =
				Edemo_mapShanmenMeridianShockTreatmentStatus::FinalizeRejected;
			Result.Diagnostic = Result.FinalizeCommand.Diagnostic;
			return Result;
		}
		Result.Status = Result.FinalizeRequest.bCommit
			? Edemo_mapShanmenMeridianShockTreatmentStatus::Committed
			: Edemo_mapShanmenMeridianShockTreatmentStatus::Cancelled;
		Result.Diagnostic = Result.FinalizeRequest.bCommit
			? TEXT("The exact treatment item Quantity was durably consumed.")
			: TEXT("The pre-treatment item Quantity intent was durably cancelled.");
		return Result;
	}
}

bool Fdemo_mapShanmenMeridianShockTreatmentItemResult::
	HasPrepareRequest() const
{
	return TreatmentIntent.IsValid()
		&& PrepareRequest.IsValid()
		&& PrepareRequest.IntentId == TreatmentIntent.GetTreatmentId()
		&& PrepareRequest.ItemInstanceId
			== TreatmentIntent.GetItemInstanceId()
		&& PrepareRequest.PurposeId == TreatmentPurpose();
}

bool Fdemo_mapShanmenMeridianShockTreatmentItemResult::IsPrepared() const
{
	return IsPreparedStatus(Status)
		&& HasPrepareRequest()
		&& PrepareCommand.IsCommandSuccess()
		&& PrepareCommand.Receipt.Operation
			== EShanmenItemTransactionOperation::
				PreparePreparedRunQuantityIntent
		&& PrepareCommand.Receipt.RequestId
			== PrepareRequest.Context.RequestId
		&& PrepareCommand.Receipt.ReservationId
			== PrepareRequest.IntentId;
}

bool Fdemo_mapShanmenMeridianShockTreatmentItemResult::IsFinalized() const
{
	return (Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::Committed
			|| Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::Cancelled)
		&& HasPrepareRequest()
		&& PrepareCommand.IsCommandSuccess()
		&& FinalizeRequest.IsValid()
		&& FinalizeCommand.IsCommandSuccess()
		&& FinalizeCommand.Receipt.Operation
			== EShanmenItemTransactionOperation::
				FinalizePreparedRunQuantityIntent
		&& FinalizeCommand.Receipt.RequestId
			== FinalizeRequest.Context.RequestId;
}

Fdemo_mapShanmenMeridianShockTreatmentItemResult
Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildPrepareRequest(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& ConditionStatus,
	const FGuid& ItemInstanceId,
	const int32 Quantity)
{
	if (!Correlation.IsValid())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				RunCorrelationInvalid,
			TEXT("Treatment preparation requires one valid immutable Run correlation."));
	}
	if (!ConditionStatus.IsValid() || !ConditionStatus.IsActive()
		|| ConditionStatus.GetDefinitionId()
			!= Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		|| ConditionStatus.GetConditionRevision() <= 0)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				ConditionStatusInvalid,
			TEXT("Treatment preparation requires one active canonical Meridian Shock status."));
	}
	if (ConditionStatus.GetRunId() != Correlation.ActiveRunId)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::ConditionMismatch,
			TEXT("Condition status does not belong to the active item Run."));
	}
	const FShanmenContentStamp AuthorityContent =
		Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
	if (!Snapshot.Content.IsValid()
		|| Snapshot.Content.Version != AuthorityContent.Version
		|| Snapshot.Content.Digest != AuthorityContent.Digest
		|| Snapshot.AuthorityRevision
			< Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::SnapshotStale,
			TEXT("Treatment item evidence is stale or not from the product item authority."));
	}
	if (!ItemInstanceId.IsValid()
		|| !Correlation.OrderedRunInventoryItemInstanceIds.Contains(
			ItemInstanceId))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::SourceItemMismatch,
			TEXT("Only an exact prepared Run-inventory item may treat this condition."));
	}

	const FShanmenItemInstance* Item = FindBy(
		Snapshot.Items,
		[&ItemInstanceId](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == ItemInstanceId;
		});
	if (!Item || Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::ItemNotFound,
			TEXT("The exact treatment item is absent from this authority scope."));
	}
	const FShanmenItemDefinition* Definition = FindBy(
		Snapshot.Definitions,
		[Item](const FShanmenItemDefinition& Candidate)
		{
			return Candidate.DefinitionId == Item->DefinitionId;
		});
	const Fdemo_mapItemDefinition* ProductDefinition =
		Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
	if (!Definition || !Definition->IsValid()
		|| !Definition->Supports(EShanmenItemResourceKind::Quantity)
		|| !ProductDefinition
		|| !Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			ProductDefinition->ContentVersionId,
			ProductDefinition->ContentDigest)
		|| ProductDefinition->MaxStackSize != Definition->MaxStack
		|| !ProductDefinition->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::MeridianShockTreatment))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				DefinitionNotTreatment,
			TEXT("A consumable stack cannot treat Meridian Shock without the exact product semantic."));
	}

	const FShanmenItemProcessedRequestSnapshot* Lifecycle = FindBy(
		Snapshot.ProcessedRequests,
		[&Correlation](const FShanmenItemProcessedRequestSnapshot& Processed)
		{
			return Processed.RequestId == Correlation.LifecycleRequestId;
		});
	if (!Lifecycle || !Lifecycle->Receipt.IsSuccess()
		|| (Lifecycle->Receipt.Operation
				!= EShanmenItemTransactionOperation::StartPreparedRun
			&& Lifecycle->Receipt.Operation
				!= EShanmenItemTransactionOperation::ClaimPreparedRun)
		|| Lifecycle->Receipt.ReceiptId != Correlation.LifecycleReceiptId
		|| Lifecycle->Receipt.ReservationId != Correlation.ActiveRunId
		|| Lifecycle->Receipt.AuthorityRevision
			!= Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				QuantityReservationInvalid,
			TEXT("The active Run lifecycle receipt does not match its immutable correlation."));
	}
	if (Snapshot.ProcessedRequests.ContainsByPredicate(
			[&Correlation](const FShanmenItemProcessedRequestSnapshot& Processed)
			{
				return Processed.Receipt.IsSuccess()
					&& Processed.Receipt.Operation
						== EShanmenItemTransactionOperation::FinalizePreparedRun
					&& Processed.Receipt.ReservationId
						== Correlation.ActiveRunId;
			}))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::QuantityUnavailable,
			TEXT("A finalized Run cannot prepare another treatment item."));
	}

	const FShanmenItemReservationSnapshot* QuantityReservation = nullptr;
	for (const FGuid& ReservationId : Lifecycle->Receipt.ReservationIds)
	{
		const FShanmenItemReservationSnapshot* Candidate = FindBy(
			Snapshot.Reservations,
			[&ReservationId](const FShanmenItemReservationSnapshot& Value)
			{
				return Value.ReservationId == ReservationId;
			});
		if (!Candidate || Candidate->ItemInstanceId != ItemInstanceId
			|| Candidate->ResourceKind
				!= EShanmenItemResourceKind::Quantity)
		{
			continue;
		}
		if (QuantityReservation)
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::
					QuantityReservationInvalid,
				TEXT("The active Run contains duplicate Quantity authority for one treatment item."));
		}
		QuantityReservation = Candidate;
	}
	if (!QuantityReservation || !QuantityReservation->IsValid()
		|| QuantityReservation->State
			!= EShanmenItemReservationState::Committed
		|| QuantityReservation->OwnerId != Correlation.OwnerId
		|| QuantityReservation->RunId != Correlation.ScopeId)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				QuantityReservationInvalid,
			TEXT("The treatment item has no exact committed active-Run Quantity reservation."));
	}

	Fdemo_mapShanmenCombatConditionTreatmentIntent Intent;
	if (!Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			ConditionStatus.GetRunId(),
			ConditionStatus.GetTargetEntityId(),
			ConditionStatus.GetTimelineId(),
			ItemInstanceId,
			Item->DefinitionId,
			ConditionStatus.GetConditionRevision(),
			Intent))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::RequestInvalid,
			TEXT("Validated condition and item evidence could not capture a treatment intent."));
	}
	const FGuid PrepareRequestId = MakePrepareRequestId(
		Correlation, Intent, Snapshot.Content, Quantity);
	const FShanmenItemTransactionReceipt* ExistingPrepare = nullptr;
	TSet<FGuid> FinalizedPrepareRequestIds;
	int32 Consumed = 0;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		const FShanmenItemTransactionReceipt& Receipt = Processed.Receipt;
		if (!Receipt.IsSuccess())
		{
			continue;
		}
		if (Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.ReservationIds.Num() == 2)
		{
			FinalizedPrepareRequestIds.Add(Receipt.ReservationIds[1]);
			if (Receipt.Phase == EShanmenItemTransactionPhase::Committed
				&& Receipt.ReservationIds[0] == Correlation.ActiveRunId
				&& Receipt.ItemInstanceId == ItemInstanceId)
			{
				Consumed += Receipt.Amount;
			}
		}
		else if (Receipt.Operation
				== EShanmenItemTransactionOperation::ConsumePreparedRunItem
			&& Receipt.ReservationId == Correlation.ActiveRunId
			&& Receipt.ItemInstanceId == ItemInstanceId)
		{
			Consumed += Receipt.Amount;
		}
		else if (Receipt.Operation
				== EShanmenItemTransactionOperation::
					PreparePreparedRunQuantityIntent
			&& Receipt.ReservationId == Intent.GetTreatmentId())
		{
			if (ExistingPrepare)
			{
				return Reject(
					Edemo_mapShanmenMeridianShockTreatmentStatus::
						QuantityReservationInvalid,
					TEXT("The treatment identity has duplicate prepare receipts."));
			}
			ExistingPrepare = &Receipt;
		}
	}
	const int32 CurrentQuantity = QuantityReservation->Amount - Consumed;
	if (CurrentQuantity < 0)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				QuantityReservationInvalid,
			TEXT("Committed active-Run consumption exceeds its frozen Quantity."));
	}

	int32 ExpectedQuantityBefore = CurrentQuantity;
	if (ExistingPrepare)
	{
		if (ExistingPrepare->RequestId != PrepareRequestId
			|| ExistingPrepare->ItemInstanceId != ItemInstanceId
			|| ExistingPrepare->ReservationIds
				!= TArray<FGuid>({ Correlation.ActiveRunId })
			|| ExistingPrepare->Amount != Quantity
			|| ExistingPrepare->PurposeId != TreatmentPurpose())
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::
					ConditionMismatch,
				TEXT("Existing condition intent does not match the canonical treatment request."));
		}
		ExpectedQuantityBefore = ExistingPrepare->ResourceBefore;
	}
	else
	{
		const bool bOtherPendingIntent =
			Snapshot.ProcessedRequests.ContainsByPredicate(
				[&Correlation, &ItemInstanceId, &FinalizedPrepareRequestIds](
					const FShanmenItemProcessedRequestSnapshot& Processed)
				{
					const FShanmenItemTransactionReceipt& Receipt =
						Processed.Receipt;
					return Receipt.IsSuccess()
						&& Receipt.Operation
							== EShanmenItemTransactionOperation::
								PreparePreparedRunQuantityIntent
						&& Receipt.ItemInstanceId == ItemInstanceId
						&& Receipt.ReservationIds
							== TArray<FGuid>({ Correlation.ActiveRunId })
						&& !FinalizedPrepareRequestIds.Contains(
							Receipt.RequestId);
				});
		if (bOtherPendingIntent)
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentStatus::
					QuantityUnavailable,
				TEXT("Another action already owns the pending intent for this item."));
		}
	}
	if (Quantity <= 0 || ExpectedQuantityBefore < Quantity)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::QuantityUnavailable,
			TEXT("The active Run has insufficient Quantity for this treatment."));
	}

	Fdemo_mapShanmenMeridianShockTreatmentItemResult Result;
	Result.Status =
		Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady;
	Result.Diagnostic = ExistingPrepare
		? TEXT("The deterministic treatment prepare request can replay exactly.")
		: TEXT("Exact active-Run treatment Quantity is ready to prepare.");
	Result.TreatmentIntent = Intent;
	Result.PrepareRequest.Context.RunId = Correlation.ScopeId;
	Result.PrepareRequest.Context.OwnerId = Correlation.OwnerId;
	Result.PrepareRequest.Context.RequestId = PrepareRequestId;
	Result.PrepareRequest.Context.Content = Snapshot.Content;
	Result.PrepareRequest.ActiveRunId = Correlation.ActiveRunId;
	Result.PrepareRequest.IntentId = Intent.GetTreatmentId();
	Result.PrepareRequest.ItemInstanceId = ItemInstanceId;
	Result.PrepareRequest.Amount = Quantity;
	Result.PrepareRequest.ExpectedQuantityBefore = ExpectedQuantityBefore;
	Result.PrepareRequest.PurposeId = TreatmentPurpose();
	if (!Result.HasPrepareRequest())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::RequestInvalid,
			TEXT("Validated treatment evidence produced an invalid prepare request."));
	}
	return Result;
}

Fdemo_mapShanmenMeridianShockTreatmentItemResult
Fdemo_mapShanmenMeridianShockTreatmentAdapter::PrepareActiveRun(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& ConditionStatus,
	const FGuid& ItemInstanceId,
	const int32 Quantity)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::AuthorityNotReady,
			TEXT("Treatment preparation requires the ready item authority on the Game Thread."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::SnapshotUnavailable,
			TEXT("The ready item authority could not provide a read-only snapshot."));
	}
	Fdemo_mapShanmenMeridianShockTreatmentItemResult Result =
		BuildPrepareRequest(
			Snapshot,
			Correlation,
			ConditionStatus,
			ItemInstanceId,
			Quantity);
	if (Result.Status
		!= Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady)
	{
		return Result;
	}
	Result.PrepareCommand =
		Authority.PreparePreparedRunQuantityIntentDurable(
			Result.PrepareRequest);
	if (!Result.PrepareCommand.IsCommandSuccess())
	{
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentStatus::PrepareRejected;
		Result.Diagnostic = Result.PrepareCommand.Diagnostic;
		return Result;
	}
	Result.Status = Result.PrepareCommand.Status
		== EShanmenItemDurableCommandStatus::Replayed
		? Edemo_mapShanmenMeridianShockTreatmentStatus::Replayed
		: Edemo_mapShanmenMeridianShockTreatmentStatus::Prepared;
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenMeridianShockTreatmentStatus::Replayed
		? TEXT("The exact treatment Quantity intent was durably replayed.")
		: TEXT("The exact treatment Quantity intent was durably prepared.");
	return Result;
}

Fdemo_mapShanmenMeridianShockTreatmentItemResult
Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildCommitRequest(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
	const Fdemo_mapShanmenCombatConditionTreatmentReceipt& TreatmentReceipt)
{
	if (!TreatmentReceipt.IsValid())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				TreatmentReceiptInvalid,
			TEXT("Treatment commit requires one valid immutable condition receipt."));
	}
	if (!TreatmentReceipt.Matches(Preparation.TreatmentIntent))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::TreatmentMismatch,
			TEXT("A different condition treatment cannot consume this prepared item."));
	}
	return BuildFinalizeRequest(Correlation, Preparation, true);
}

Fdemo_mapShanmenMeridianShockTreatmentItemResult
Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildCancelRequest(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& LiveConditionStatus)
{
	const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent =
		Preparation.TreatmentIntent;
	if (!LiveConditionStatus.IsValid()
		|| !LiveConditionStatus.IsActive()
		|| !Intent.IsValid()
		|| LiveConditionStatus.GetDefinitionId()
			!= Intent.GetConditionDefinitionId()
		|| LiveConditionStatus.GetRunId() != Correlation.ActiveRunId
		|| LiveConditionStatus.GetRunId() != Intent.GetRunId()
		|| LiveConditionStatus.GetTargetEntityId()
			!= Intent.GetTargetEntityId()
		|| LiveConditionStatus.GetTimelineId() != Intent.GetTimelineId()
		|| LiveConditionStatus.GetConditionRevision()
			!= Intent.GetExpectedConditionRevision())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentStatus::
				ConditionStatusInvalid,
			TEXT("Cancellation requires the same still-active condition revision captured before treatment."));
	}
	return BuildFinalizeRequest(Correlation, Preparation, false);
}

Fdemo_mapShanmenMeridianShockTreatmentItemResult
Fdemo_mapShanmenMeridianShockTreatmentAdapter::CommitTreated(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
	const Fdemo_mapShanmenCombatConditionTreatmentReceipt& TreatmentReceipt)
{
	Fdemo_mapShanmenMeridianShockTreatmentItemResult Result =
		BuildCommitRequest(
			Correlation,
			Preparation,
			TreatmentReceipt);
	return Result.Status
		== Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady
		? ExecuteFinalize(Authority, MoveTemp(Result)) : Result;
}

Fdemo_mapShanmenMeridianShockTreatmentItemResult
Fdemo_mapShanmenMeridianShockTreatmentAdapter::CancelBeforeTreatment(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& LiveConditionStatus)
{
	Fdemo_mapShanmenMeridianShockTreatmentItemResult Result =
		BuildCancelRequest(
			Correlation,
			Preparation,
			LiveConditionStatus);
	return Result.Status
		== Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady
		? ExecuteFinalize(Authority, MoveTemp(Result)) : Result;
}
