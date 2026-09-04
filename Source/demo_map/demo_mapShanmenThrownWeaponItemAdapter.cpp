#include "demo_mapShanmenThrownWeaponItemAdapter.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemTags.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	bool TryGetThrownLaunchPurpose(
		FName ActionDefinitionId,
		FName& OutPurposeId)
	{
		OutPurposeId = NAME_None;
		if (ActionDefinitionId
			== FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
		{
			OutPurposeId = TEXT("Shanmen.ThrownWeapon.StraightLaunch.r1");
			return true;
		}
		if (ActionDefinitionId
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId())
		{
			OutPurposeId = TEXT("Shanmen.ThrownWeapon.ArcLaunch.r1");
			return true;
		}
		return false;
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

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
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

	template <typename TValue, typename TPredicate>
	const TValue* FindBy(const TArray<TValue>& Values, TPredicate Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	Fdemo_mapShanmenThrownWeaponItemResult Reject(
		Edemo_mapShanmenThrownWeaponItemStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenThrownWeaponItemResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	FGuid MakePrepareRequestId(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenCombatActionSnapshot& Action,
		int32 Quantity)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.ThrownWeapon.QuantityPrepare.r1"),
			{
				GuidDigits(Correlation.CorrelationId),
				GuidDigits(Correlation.ActiveRunId),
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceItemInstanceId()),
				FString::FromInt(Quantity),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest
			});
	}

	FGuid MakeFinalizeRequestId(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenItemRunQuantityIntentRequest& Prepare)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.ThrownWeapon.QuantityFinalize.r1"),
			{
				GuidDigits(Correlation.CorrelationId),
				GuidDigits(Prepare.ActiveRunId),
				GuidDigits(Prepare.Context.RequestId),
				GuidDigits(Prepare.IntentId),
				GuidDigits(Prepare.ItemInstanceId)
			});
	}

	bool IsPreparedStatus(Edemo_mapShanmenThrownWeaponItemStatus Status)
	{
		return Status == Edemo_mapShanmenThrownWeaponItemStatus::Prepared
			|| Status == Edemo_mapShanmenThrownWeaponItemStatus::Replayed;
	}

	Fdemo_mapShanmenThrownWeaponItemResult BuildFinalizeRequest(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		bool bCommit)
	{
		if (!Correlation.IsValid())
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::RunCorrelationInvalid,
				TEXT("Thrown-item finalization requires one valid immutable Run correlation."));
		}
		if (!Preparation.IsPrepared())
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::PreparationInvalid,
				TEXT("Thrown-item finalization requires one durable prepared intent."));
		}
		const FShanmenItemRunQuantityIntentRequest& Prepare =
			Preparation.PrepareRequest;
		if (Preparation.Action.GetRunId() != Correlation.ActiveRunId
			|| Preparation.Action.GetOwnerId() != Correlation.OwnerId
			|| Prepare.Context.RunId != Correlation.ScopeId
			|| Prepare.Context.OwnerId != Correlation.OwnerId
			|| Prepare.ActiveRunId != Correlation.ActiveRunId
			|| Prepare.IntentId
				!= Preparation.Action.GetActivationId()
			|| Prepare.ItemInstanceId
				!= Preparation.Action.GetSourceItemInstanceId())
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::ActionMismatch,
				TEXT("Prepared thrown-item identity does not belong to this active Run."));
		}

		Fdemo_mapShanmenThrownWeaponItemResult Result = Preparation;
		Result.Status = Edemo_mapShanmenThrownWeaponItemStatus::RequestReady;
		Result.Diagnostic = bCommit
			? TEXT("Exact launch proof authorizes one durable Quantity commit request.")
			: TEXT("Pre-launch cancellation authorizes one durable Quantity release request.");
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
				Edemo_mapShanmenThrownWeaponItemStatus::RequestInvalid,
				TEXT("Validated thrown-item evidence produced an invalid finalize request."));
		}
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponItemResult ExecuteFinalize(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapShanmenThrownWeaponItemResult Result)
	{
		if (!IsInGameThread()
			|| Authority.GetLifecycleState()
				!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::AuthorityNotReady,
				TEXT("Thrown-item finalization requires the ready item authority on the Game Thread."));
		}
		Result.FinalizeCommand =
			Authority.FinalizePreparedRunQuantityIntentDurable(
				Result.FinalizeRequest);
		if (!Result.FinalizeCommand.IsCommandSuccess())
		{
			Result.Status =
				Edemo_mapShanmenThrownWeaponItemStatus::FinalizeRejected;
			Result.Diagnostic = Result.FinalizeCommand.Diagnostic;
			return Result;
		}
		Result.Status = Result.FinalizeRequest.bCommit
			? Edemo_mapShanmenThrownWeaponItemStatus::Committed
			: Edemo_mapShanmenThrownWeaponItemStatus::Cancelled;
		Result.Diagnostic = Result.FinalizeRequest.bCommit
			? TEXT("The exact launched item Quantity was durably consumed.")
			: TEXT("The pre-launch item Quantity intent was durably cancelled.");
		return Result;
	}
}

bool Fdemo_mapShanmenThrownWeaponItemResult::HasPrepareRequest() const
{
	return Action.IsValid()
		&& PrepareRequest.IsValid()
		&& PrepareRequest.IntentId == Action.GetActivationId()
		&& PrepareRequest.ItemInstanceId == Action.GetSourceItemInstanceId();
}

bool Fdemo_mapShanmenThrownWeaponItemResult::IsPrepared() const
{
	return IsPreparedStatus(Status)
		&& HasPrepareRequest()
		&& PrepareCommand.IsCommandSuccess()
		&& PrepareCommand.Receipt.Operation
			== EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
		&& PrepareCommand.Receipt.RequestId == PrepareRequest.Context.RequestId
		&& PrepareCommand.Receipt.ReservationId == PrepareRequest.IntentId;
}

bool Fdemo_mapShanmenThrownWeaponItemResult::IsFinalized() const
{
	return (Status == Edemo_mapShanmenThrownWeaponItemStatus::Committed
			|| Status == Edemo_mapShanmenThrownWeaponItemStatus::Cancelled)
		&& HasPrepareRequest()
		&& PrepareCommand.IsCommandSuccess()
		&& FinalizeRequest.IsValid()
		&& FinalizeCommand.IsCommandSuccess()
		&& FinalizeCommand.Receipt.Operation
			== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
		&& FinalizeCommand.Receipt.RequestId
			== FinalizeRequest.Context.RequestId;
}

Fdemo_mapShanmenThrownWeaponItemResult
Fdemo_mapShanmenThrownWeaponItemAdapter::BuildPrepareRequest(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenCombatActionSnapshot& Action,
	int32 Quantity)
{
	if (!Correlation.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::RunCorrelationInvalid,
			TEXT("Thrown-item preparation requires one valid immutable Run correlation."));
	}
	FName LaunchPurposeId;
	if (!Action.IsValid()
		|| !TryGetThrownLaunchPurpose(
			Action.GetActionDefinitionId(), LaunchPurposeId)
		|| Quantity <= 0)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::ActionInvalid,
			TEXT("Thrown-item preparation requires one canonical physical action and positive Quantity."));
	}
	if (Action.GetRunId() != Correlation.ActiveRunId
		|| Action.GetOwnerId() != Correlation.OwnerId
		|| !SameContent(Action.GetContent(), Snapshot.Content))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::ActionMismatch,
			TEXT("Thrown-item action identity or content does not match the active authority Run."));
	}
	if (!Snapshot.Content.IsValid()
		|| Snapshot.AuthorityRevision < Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::SnapshotStale,
			TEXT("Thrown-item authority evidence predates the active Run lifecycle."));
	}
	const FGuid ItemId = Action.GetSourceItemInstanceId();
	if (!Correlation.OrderedRunInventoryItemInstanceIds.Contains(ItemId))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::SourceItemMismatch,
			TEXT("Only an exact prepared Run-inventory item may source a thrown action."));
	}

	const FShanmenItemInstance* Item = FindBy(
		Snapshot.Items,
		[&ItemId](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == ItemId;
		});
	if (!Item || Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::ItemNotFound,
			TEXT("The exact thrown item is absent from this authority scope."));
	}
	const FShanmenItemDefinition* Definition = FindBy(
		Snapshot.Definitions,
		[Item](const FShanmenItemDefinition& Candidate)
		{
			return Candidate.DefinitionId == Item->DefinitionId;
		});
	if (!Definition || !Definition->IsValid()
		|| !Definition->Supports(EShanmenItemResourceKind::Quantity)
		|| !Definition->ItemTags.HasTagExact(
			FShanmenItemNativeTags::ItemWeaponThrown()))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::DefinitionNotThrownWeapon,
			TEXT("A consumable stack is not a thrown weapon unless its authority definition says so."));
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
			Edemo_mapShanmenThrownWeaponItemStatus::QuantityReservationInvalid,
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
			Edemo_mapShanmenThrownWeaponItemStatus::QuantityUnavailable,
			TEXT("A finalized Run cannot prepare another thrown item."));
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
		if (!Candidate || Candidate->ItemInstanceId != ItemId
			|| Candidate->ResourceKind
				!= EShanmenItemResourceKind::Quantity)
		{
			continue;
		}
		if (QuantityReservation)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::QuantityReservationInvalid,
				TEXT("The active Run contains duplicate Quantity authority for one thrown item."));
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
			Edemo_mapShanmenThrownWeaponItemStatus::QuantityReservationInvalid,
			TEXT("The thrown item has no exact committed active-Run Quantity reservation."));
	}

	const FGuid PrepareRequestId =
		MakePrepareRequestId(Correlation, Action, Quantity);
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
				== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
			&& Receipt.ReservationIds.Num() == 2)
		{
			FinalizedPrepareRequestIds.Add(Receipt.ReservationIds[1]);
			if (Receipt.Phase == EShanmenItemTransactionPhase::Committed
				&& Receipt.ReservationIds[0] == Correlation.ActiveRunId
				&& Receipt.ItemInstanceId == ItemId)
			{
				Consumed += Receipt.Amount;
			}
		}
		else if (Receipt.Operation
				== EShanmenItemTransactionOperation::ConsumePreparedRunItem
			&& Receipt.ReservationId == Correlation.ActiveRunId
			&& Receipt.ItemInstanceId == ItemId)
		{
			Consumed += Receipt.Amount;
		}
		else if (Receipt.Operation
				== EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
			&& Receipt.ReservationId == Action.GetActivationId())
		{
			if (ExistingPrepare)
			{
				return Reject(
					Edemo_mapShanmenThrownWeaponItemStatus::QuantityReservationInvalid,
					TEXT("The action identity has duplicate prepare receipts."));
			}
			ExistingPrepare = &Receipt;
		}
	}
	const int32 CurrentQuantity = QuantityReservation->Amount - Consumed;
	if (CurrentQuantity < 0)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::QuantityReservationInvalid,
			TEXT("Committed active-Run consumption exceeds its frozen Quantity."));
	}

	int32 ExpectedQuantityBefore = CurrentQuantity;
	if (ExistingPrepare)
	{
		if (ExistingPrepare->RequestId != PrepareRequestId
			|| ExistingPrepare->ItemInstanceId != ItemId
			|| ExistingPrepare->ReservationIds
				!= TArray<FGuid>({ Correlation.ActiveRunId })
			|| ExistingPrepare->Amount != Quantity
			|| ExistingPrepare->PurposeId != LaunchPurposeId)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::ActionMismatch,
				TEXT("Existing action intent does not match the canonical thrown-item request."));
		}
		ExpectedQuantityBefore = ExistingPrepare->ResourceBefore;
	}
	else
	{
		const bool bOtherPendingIntent =
			Snapshot.ProcessedRequests.ContainsByPredicate(
				[&Correlation, &ItemId, &FinalizedPrepareRequestIds](
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
						&& !FinalizedPrepareRequestIds.Contains(Receipt.RequestId);
				});
		if (bOtherPendingIntent)
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponItemStatus::QuantityUnavailable,
				TEXT("Another launch already owns the pending intent for this item."));
		}
	}
	if (ExpectedQuantityBefore < Quantity)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::QuantityUnavailable,
			TEXT("The active Run has insufficient Quantity for this thrown action."));
	}

	Fdemo_mapShanmenThrownWeaponItemResult Result;
	Result.Status = Edemo_mapShanmenThrownWeaponItemStatus::RequestReady;
	Result.Diagnostic = ExistingPrepare
		? TEXT("The deterministic thrown-item prepare request can replay exactly.")
		: TEXT("Exact active-Run thrown-item Quantity is ready to prepare.");
	Result.Action = Action;
	Result.PrepareRequest.Context.RunId = Correlation.ScopeId;
	Result.PrepareRequest.Context.OwnerId = Correlation.OwnerId;
	Result.PrepareRequest.Context.RequestId = PrepareRequestId;
	Result.PrepareRequest.Context.Content = Snapshot.Content;
	Result.PrepareRequest.ActiveRunId = Correlation.ActiveRunId;
	Result.PrepareRequest.IntentId = Action.GetActivationId();
	Result.PrepareRequest.ItemInstanceId = ItemId;
	Result.PrepareRequest.Amount = Quantity;
	Result.PrepareRequest.ExpectedQuantityBefore = ExpectedQuantityBefore;
	Result.PrepareRequest.PurposeId = LaunchPurposeId;
	if (!Result.HasPrepareRequest())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::RequestInvalid,
			TEXT("Validated thrown-item evidence produced an invalid prepare request."));
	}
	return Result;
}

Fdemo_mapShanmenThrownWeaponItemResult
Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenCombatActionSnapshot& Action,
	int32 Quantity)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::AuthorityNotReady,
			TEXT("Thrown-item preparation requires the ready item authority on the Game Thread."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::SnapshotUnavailable,
			TEXT("The ready item authority could not provide a read-only snapshot."));
	}
	Fdemo_mapShanmenThrownWeaponItemResult Result =
		BuildPrepareRequest(Snapshot, Correlation, Action, Quantity);
	if (Result.Status
		!= Edemo_mapShanmenThrownWeaponItemStatus::RequestReady)
	{
		return Result;
	}
	Result.PrepareCommand =
		Authority.PreparePreparedRunQuantityIntentDurable(
			Result.PrepareRequest);
	if (!Result.PrepareCommand.IsCommandSuccess())
	{
		Result.Status = Edemo_mapShanmenThrownWeaponItemStatus::PrepareRejected;
		Result.Diagnostic = Result.PrepareCommand.Diagnostic;
		return Result;
	}
	Result.Status = Result.PrepareCommand.Status
		== EShanmenItemDurableCommandStatus::Replayed
		? Edemo_mapShanmenThrownWeaponItemStatus::Replayed
		: Edemo_mapShanmenThrownWeaponItemStatus::Prepared;
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenThrownWeaponItemStatus::Replayed
		? TEXT("The exact thrown-item Quantity intent was durably replayed.")
		: TEXT("The exact thrown-item Quantity intent was durably prepared.");
	return Result;
}

Fdemo_mapShanmenThrownWeaponItemResult
Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCommitRequest(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenThrownWeaponLaunchReceipt& LaunchReceipt)
{
	if (!LaunchReceipt.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::LaunchReceiptInvalid,
			TEXT("Thrown-item commit requires one valid immutable launch receipt."));
	}
	if (!ActionsMatch(LaunchReceipt.GetAction(), Preparation.Action))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponItemStatus::LaunchMismatch,
			TEXT("A launch receipt cannot consume a different action's physical item."));
	}
	return BuildFinalizeRequest(Correlation, Preparation, true);
}

Fdemo_mapShanmenThrownWeaponItemResult
Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCancelRequest(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation)
{
	return BuildFinalizeRequest(Correlation, Preparation, false);
}

Fdemo_mapShanmenThrownWeaponItemResult
Fdemo_mapShanmenThrownWeaponItemAdapter::CommitLaunched(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenThrownWeaponLaunchReceipt& LaunchReceipt)
{
	Fdemo_mapShanmenThrownWeaponItemResult Result =
		BuildCommitRequest(Correlation, Preparation, LaunchReceipt);
	return Result.Status
		== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
		? ExecuteFinalize(Authority, MoveTemp(Result)) : Result;
}

Fdemo_mapShanmenThrownWeaponItemResult
Fdemo_mapShanmenThrownWeaponItemAdapter::CancelBeforeLaunch(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation)
{
	Fdemo_mapShanmenThrownWeaponItemResult Result =
		BuildCancelRequest(Correlation, Preparation);
	return Result.Status
		== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
		? ExecuteFinalize(Authority, MoveTemp(Result)) : Result;
}
