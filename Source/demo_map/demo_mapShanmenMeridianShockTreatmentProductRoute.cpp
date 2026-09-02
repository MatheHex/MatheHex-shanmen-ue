#include "demo_mapShanmenMeridianShockTreatmentProductRoute.h"

#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	bool CommandBelongsToCorrelation(
		const Fdemo_mapShanmenMeridianShockTreatmentCommand& Command,
		const Fdemo_mapShanmenRunCorrelation& Correlation)
	{
		return Command.IsValid()
			&& Correlation.IsValid()
			&& Command.GetCorrelation() == Correlation;
	}
}

bool Fdemo_mapShanmenMeridianShockTreatmentCommand::TryCapture(
	const FGuid& RequestedRequestId,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& RequestedStatus,
	const FGuid& RequestedItemInstanceId,
	const int32 RequestedQuantity,
	const Fdemo_mapShanmenCombatRunTimelineSample& RequestedTimelineSample,
	Fdemo_mapShanmenMeridianShockTreatmentCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenMeridianShockTreatmentCommand();
	Fdemo_mapShanmenMeridianShockTreatmentCommand Candidate;
	Candidate.RequestId = RequestedRequestId;
	Candidate.Correlation = RequestedCorrelation;
	Candidate.ConditionStatus = RequestedStatus;
	Candidate.ItemInstanceId = RequestedItemInstanceId;
	Candidate.Quantity = RequestedQuantity;
	Candidate.TimelineSample = RequestedTimelineSample;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCommand = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentCommand::IsValid() const
{
	return RequestId.IsValid()
		&& Correlation.IsValid()
		&& ConditionStatus.IsValid()
		&& ConditionStatus.IsActive()
		&& ConditionStatus.GetDefinitionId()
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& ConditionStatus.GetRunId() == Correlation.ActiveRunId
		&& ConditionStatus.GetTimelineId() == TimelineSample.GetTimelineId()
		&& TimelineSample.IsValid()
		&& TimelineSample.GetCurrentTick()
			>= ConditionStatus.GetObservedTick()
		&& ItemInstanceId.IsValid()
		&& Correlation.OrderedRunInventoryItemInstanceIds.Contains(
			ItemInstanceId)
		&& Quantity > 0;
}

bool Fdemo_mapShanmenMeridianShockTreatmentCommand::Matches(
	const Fdemo_mapShanmenMeridianShockTreatmentCommand& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& RequestId == Other.RequestId
		&& Correlation == Other.Correlation
		&& ConditionStatus.Matches(Other.ConditionStatus)
		&& ItemInstanceId == Other.ItemInstanceId
		&& Quantity == Other.Quantity
		&& TimelineSample.GetSampleId()
			== Other.TimelineSample.GetSampleId();
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::
	FJournalEntry::IsValid() const
{
	if (!Command.IsValid()
		|| (bCommitted && bCancelled)
		|| bTreated && !bPrepared
		|| (bCommitted || bCancelled) && !bPrepared
		|| bCancelled && bTreated)
	{
		return false;
	}
	if (bPrepared
		&& (!Preparation.IsPrepared()
			|| Preparation.TreatmentIntent.GetItemInstanceId()
				!= Command.GetItemInstanceId()
			|| Preparation.TreatmentIntent.GetRunId()
				!= Command.GetCorrelation().ActiveRunId
			|| Preparation.TreatmentIntent.GetExpectedConditionRevision()
				!= Command.GetConditionStatus().GetConditionRevision()))
	{
		return false;
	}
	if (bTreated
		&& (!Treatment.IsSuccess()
			|| !Treatment.Receipt.Matches(
				Preparation.TreatmentIntent)))
	{
		return false;
	}
	if ((bCommitted || bCancelled)
		&& (!Finalization.IsFinalized()
			|| Finalization.FinalizeRequest.bCommit != bCommitted))
	{
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::TryBegin(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	Udemo_mapShanmenCombatConditionComponent* RequestedConditionComponent,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsInGameThread())
	{
		OutDiagnostic = TEXT("Treatment route binding is restricted to the Game Thread.");
		return false;
	}
	if (bActive)
	{
		if (IsValid()
			&& Correlation == RequestedCorrelation
			&& ConditionComponent.Get() == RequestedConditionComponent)
		{
			OutDiagnostic = TEXT("Meridian Shock treatment route is already bound to this Run.");
			return true;
		}
		OutDiagnostic = TEXT("An active treatment route cannot change Run or condition authority.");
		return false;
	}
	if (!IsEmpty()
		|| !RequestedCorrelation.IsValid()
		|| !::IsValid(RequestedConditionComponent)
		|| !RequestedConditionComponent->IsValid()
		|| RequestedConditionComponent->IsEmpty()
		|| RequestedConditionComponent->GetRunId()
			!= RequestedCorrelation.ActiveRunId)
	{
		OutDiagnostic = TEXT("Treatment route begin requires empty state and matching active-Run authorities.");
		return false;
	}
	Correlation = RequestedCorrelation;
	ConditionComponent = RequestedConditionComponent;
	bActive = true;
	if (!IsValid())
	{
		Clear();
		OutDiagnostic = TEXT("Treatment route failed closed while binding the active Run.");
		return false;
	}
	OutDiagnostic = TEXT("Treatment route owns the active Run treatment sequence.");
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::
	TryCaptureCommand(
		const FGuid& RequestId,
		const FGuid& ItemInstanceId,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		Fdemo_mapShanmenMeridianShockTreatmentCommand& OutCommand,
		FString& OutDiagnostic,
		const int32 Quantity) const
{
	OutCommand = Fdemo_mapShanmenMeridianShockTreatmentCommand();
	OutDiagnostic.Reset();
	if (!IsInGameThread()
		|| !bActive || !IsValid() || !ConditionComponent.IsValid())
	{
		OutDiagnostic = TEXT("Treatment command capture requires one valid active product route.");
		return false;
	}
	Fdemo_mapShanmenCombatConditionStatusSnapshot Status;
	if (!ConditionComponent->TryCaptureMeridianShockStatus(Status))
	{
		OutDiagnostic = TEXT("Condition authority could not provide a treatment status snapshot.");
		return false;
	}
	if (!Fdemo_mapShanmenMeridianShockTreatmentCommand::TryCapture(
			RequestId,
			Correlation,
			Status,
			ItemInstanceId,
			Quantity,
			TimelineSample,
			OutCommand))
	{
		OutDiagnostic = TEXT("Live Run, condition, item and timeline evidence cannot form a treatment command.");
		return false;
	}
	OutDiagnostic = TEXT("Exact Meridian Shock treatment command captured before mutation.");
	return true;
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductRoute::Reject(
	const Edemo_mapShanmenMeridianShockTreatmentRouteError Error,
	const Fdemo_mapShanmenMeridianShockTreatmentCommand* Command,
	const TCHAR* Diagnostic,
	const bool bReusedRequest) const
{
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
	Result.Error = Error;
	Result.bReusedRequest = bReusedRequest;
	Result.Diagnostic = Diagnostic;
	if (Command)
	{
		Result.RequestId = Command->GetRequestId();
	}
	return Result;
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductRoute::TryExecute(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenMeridianShockTreatmentCommand& Command)
{
	if (!IsInGameThread())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInvalid,
			&Command,
			TEXT("Treatment execution is restricted to the Game Thread."));
	}
	if (!bActive)
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInactive,
			&Command,
			TEXT("Treatment execution requires one active product route."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInvalid,
			&Command,
			TEXT("Treatment product route invariants are invalid."));
	}
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RequestInvalid,
			&Command,
			TEXT("Treatment product route rejected an invalid command."));
	}
	if (!CommandBelongsToCorrelation(Command, Correlation))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RunMismatch,
			&Command,
			TEXT("Treatment command does not belong to the bound active Run."));
	}
	if (!ConditionComponent.IsValid()
		|| Command.GetConditionStatus().GetTargetEntityId()
			!= ConditionComponent->GetTargetEntityId()
		|| Command.GetConditionStatus().GetTimelineId()
			!= ConditionComponent->GetTimelineId())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RunMismatch,
			&Command,
			TEXT("Treatment command does not belong to the bound target and timeline."));
	}

	if (FJournalEntry* Existing = Journal.Find(Command.GetRequestId()))
	{
		if (!Existing->Command.Matches(Command))
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::
					RequestIdConflict,
				&Command,
				TEXT("RequestId was reused with another treatment payload."),
				true);
		}
		return Resume(Authority, *Existing, true);
	}

	FJournalEntry Entry;
	Entry.Command = Command;
	FJournalEntry& Stored = Journal.Add(Command.GetRequestId(), MoveTemp(Entry));
	return Resume(Authority, Stored, false);
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductRoute::Resume(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	FJournalEntry& Entry,
	const bool bReusedRequest)
{
	const Fdemo_mapShanmenMeridianShockTreatmentCommand& Command =
		Entry.Command;
	if (Entry.bCommitted)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Replayed;
		Result.Error = Edemo_mapShanmenMeridianShockTreatmentRouteError::None;
		Result.bReusedRequest = true;
		Result.RequestId = Command.GetRequestId();
		Result.TreatmentId =
			Entry.Preparation.TreatmentIntent.GetTreatmentId();
		Result.Item = Entry.Finalization;
		Result.Treatment = Entry.Treatment;
		Result.Diagnostic = TEXT("Exact committed treatment command replayed without mutation.");
		return Result;
	}
	if (Entry.bCancelled)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Cancelled;
		Result.Error =
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				TreatmentRejected;
		Result.bReusedRequest = true;
		Result.RequestId = Command.GetRequestId();
		Result.TreatmentId =
			Entry.Preparation.TreatmentIntent.GetTreatmentId();
		Result.Item = Entry.Finalization;
		Result.Treatment = Entry.Treatment;
		Result.Diagnostic = TEXT("Exact pre-treatment cancellation replayed without mutation.");
		return Result;
	}

	if (!Entry.bPrepared)
	{
		Entry.Preparation =
			Fdemo_mapShanmenMeridianShockTreatmentAdapter::PrepareActiveRun(
				Authority,
				Command.GetCorrelation(),
				Command.GetConditionStatus(),
				Command.GetItemInstanceId(),
				Command.GetQuantity());
		if (!Entry.Preparation.IsPrepared())
		{
			Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::PrepareRejected,
				&Command,
				TEXT("Durable treatment item preparation was rejected."),
				bReusedRequest);
			Result.Item = Entry.Preparation;
			Result.Diagnostic = Entry.Preparation.Diagnostic;
			if (Entry.Preparation.PrepareCommand.Status
				== EShanmenItemDurableCommandStatus::RecoveryRequired)
			{
				Result.Status =
					Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
						RecoveryRequired;
			}
			return Result;
		}
		Entry.bPrepared = true;
	}

	if (!Entry.bTreated)
	{
		if (!ConditionComponent.IsValid())
		{
			Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::
					ConditionUnavailable,
				&Command,
				TEXT("Prepared treatment lost its bound condition authority."),
				bReusedRequest);
			Result.Status =
				Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
					RecoveryRequired;
			Result.Item = Entry.Preparation;
			Result.TreatmentId =
				Entry.Preparation.TreatmentIntent.GetTreatmentId();
			return Result;
		}
		Entry.Treatment = ConditionComponent->TryTreatMeridianShock(
			Entry.Preparation.TreatmentIntent,
			Command.GetTimelineSample());
		if (!Entry.Treatment.IsSuccess())
		{
			Fdemo_mapShanmenCombatConditionStatusSnapshot LiveStatus;
			if (ConditionComponent->TryCaptureMeridianShockStatus(LiveStatus))
			{
				Entry.Finalization =
					Fdemo_mapShanmenMeridianShockTreatmentAdapter::
						CancelBeforeTreatment(
							Authority,
							Command.GetCorrelation(),
							Entry.Preparation,
							LiveStatus);
				if (Entry.Finalization.IsFinalized()
					&& Entry.Finalization.Status
						== Edemo_mapShanmenMeridianShockTreatmentStatus::
							Cancelled)
				{
					Entry.bCancelled = true;
					Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
					Result.Status =
						Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
							Cancelled;
					Result.Error =
						Edemo_mapShanmenMeridianShockTreatmentRouteError::
							TreatmentRejected;
					Result.bReusedRequest = bReusedRequest;
					Result.RequestId = Command.GetRequestId();
					Result.TreatmentId = Entry.Preparation.TreatmentIntent.
						GetTreatmentId();
					Result.Item = Entry.Finalization;
					Result.Treatment = Entry.Treatment;
					Result.Diagnostic = TEXT("Condition treatment rejected; exact pre-treatment Quantity intent was durably cancelled.");
					return Result;
				}
			}
			Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::
					CancellationRejected,
				&Command,
				TEXT("Treatment failed and its prepared Quantity intent could not be safely cancelled."),
				bReusedRequest);
			Result.Status =
				Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
					RecoveryRequired;
			Result.TreatmentId =
				Entry.Preparation.TreatmentIntent.GetTreatmentId();
			Result.Item = Entry.Finalization.HasPrepareRequest()
				? Entry.Finalization : Entry.Preparation;
			Result.Treatment = Entry.Treatment;
			return Result;
		}
		Entry.bTreated = true;
	}

#if WITH_DEV_AUTOMATION_TESTS
	if (bInterruptAfterTreatmentForAutomation)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				InterruptedAfterTreatment,
			&Command,
			TEXT("Injected interruption retained treatment proof for commit-only recovery."),
			bReusedRequest);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
				RecoveryRequired;
		Result.TreatmentId =
			Entry.Preparation.TreatmentIntent.GetTreatmentId();
		Result.Item = Entry.Preparation;
		Result.Treatment = Entry.Treatment;
		return Result;
	}
#endif

	Entry.Finalization =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::CommitTreated(
			Authority,
			Command.GetCorrelation(),
			Entry.Preparation,
			Entry.Treatment.Receipt);
	if (!Entry.Finalization.IsFinalized()
		|| Entry.Finalization.Status
			!= Edemo_mapShanmenMeridianShockTreatmentStatus::Committed)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::CommitRejected,
			&Command,
			TEXT("Condition treatment is committed; item consumption requires commit-only recovery."),
			bReusedRequest);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
				RecoveryRequired;
		Result.TreatmentId =
			Entry.Preparation.TreatmentIntent.GetTreatmentId();
		Result.Item = Entry.Finalization;
		Result.Treatment = Entry.Treatment;
		Result.Diagnostic = Entry.Finalization.Diagnostic;
		return Result;
	}
	Entry.bCommitted = true;

	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
	Result.Status =
		Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Committed;
	Result.Error = Edemo_mapShanmenMeridianShockTreatmentRouteError::None;
	Result.bReusedRequest = bReusedRequest;
	Result.RequestId = Command.GetRequestId();
	Result.TreatmentId =
		Entry.Preparation.TreatmentIntent.GetTreatmentId();
	Result.Item = Entry.Finalization;
	Result.Treatment = Entry.Treatment;
	Result.Diagnostic = TEXT("Meridian Shock treatment and exact item consumption committed in order.");
	return Result;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::
	TryRecoverDurablePreparation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		int32& OutRecoveredCount,
		FString& OutDiagnostic)
{
	OutRecoveredCount = 0;
	OutDiagnostic.Reset();
	if (!IsInGameThread() || !bActive || !IsValid()
		|| !ConditionComponent.IsValid()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		OutDiagnostic =
			TEXT("Durable treatment recovery requires matching ready product authorities on the Game Thread.");
		return false;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		OutDiagnostic =
			TEXT("Durable treatment recovery could not capture the item authority ledger.");
		return false;
	}
	TSet<FGuid> FinalizedPrepareRequestIds;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		const FShanmenItemTransactionReceipt& Receipt = Processed.Receipt;
		if (Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.ReservationIds.Num() == 2)
		{
			FinalizedPrepareRequestIds.Add(Receipt.ReservationIds[1]);
		}
	}
	TArray<FShanmenItemTransactionReceipt> Pending;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		const FShanmenItemTransactionReceipt& Receipt = Processed.Receipt;
		if (Receipt.IsSuccess() && Receipt.IsValid()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					PreparePreparedRunQuantityIntent
			&& Receipt.Phase == EShanmenItemTransactionPhase::Reserved
			&& Receipt.PurposeId
				== Fdemo_mapShanmenMeridianShockTreatmentAdapter::
					TreatmentPurposeId()
			&& Receipt.ReservationIds
				== TArray<FGuid>({ Correlation.ActiveRunId })
			&& !FinalizedPrepareRequestIds.Contains(Receipt.RequestId))
		{
			Pending.Add(Receipt);
		}
	}
	Pending.Sort([](
		const FShanmenItemTransactionReceipt& Left,
		const FShanmenItemTransactionReceipt& Right)
	{
		return Left.RequestId.ToString(EGuidFormats::Digits)
			< Right.RequestId.ToString(EGuidFormats::Digits);
	});
	if (Pending.IsEmpty())
	{
		OutDiagnostic =
			TEXT("The durable item ledger contains no pending Meridian Shock treatment.");
		return true;
	}
	if (Pending.Num() != 1)
	{
		OutDiagnostic =
			TEXT("Multiple pending Meridian Shock prepares are ambiguous for one condition authority.");
		return false;
	}

	const FShanmenItemTransactionReceipt& PrepareReceipt = Pending[0];
	Fdemo_mapShanmenCombatConditionTreatmentReceipt TreatmentReceipt;
	const bool bTreatmentAlreadyProcessed =
		ConditionComponent->TryGetProcessedMeridianShockTreatment(
			PrepareReceipt.ReservationId,
			TreatmentReceipt);
	Fdemo_mapShanmenCombatConditionTreatmentIntent TreatmentIntent;
	Fdemo_mapShanmenCombatConditionStatusSnapshot LiveStatus;
	Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
	if (bTreatmentAlreadyProcessed)
	{
		if (!Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
				TreatmentReceipt.GetRunId(),
				TreatmentReceipt.GetTargetEntityId(),
				TreatmentReceipt.GetTimelineId(),
				TreatmentReceipt.GetItemInstanceId(),
				TreatmentReceipt.GetItemDefinitionId(),
				TreatmentReceipt.GetConditionRevisionBefore(),
				TreatmentIntent)
			|| !TreatmentReceipt.Matches(TreatmentIntent)
			|| TreatmentReceipt.GetTreatmentId()
				!= PrepareReceipt.ReservationId)
		{
			OutDiagnostic =
				TEXT("Runtime treatment proof cannot reconstruct the durable prepare identity.");
			return false;
		}
	}
	else
	{
		const FShanmenItemInstance* Item =
			Snapshot.Items.FindByPredicate(
				[&PrepareReceipt](const FShanmenItemInstance& Value)
				{
					return Value.ItemInstanceId
						== PrepareReceipt.ItemInstanceId;
				});
		if (!Item
			|| !ConditionComponent->TryCaptureMeridianShockStatus(
				LiveStatus)
			|| !LiveStatus.IsActive()
			|| !Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
				LiveStatus.GetRunId(),
				LiveStatus.GetTargetEntityId(),
				LiveStatus.GetTimelineId(),
				PrepareReceipt.ItemInstanceId,
				Item->DefinitionId,
				LiveStatus.GetConditionRevision(),
				TreatmentIntent)
			|| TreatmentIntent.GetTreatmentId()
				!= PrepareReceipt.ReservationId
			|| !Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
				LiveStatus.GetTimelineId(),
				LiveStatus.GetObservedTick(),
				TimelineSample))
		{
			OutDiagnostic =
				TEXT("Pending item prepare has neither exact treatment proof nor its still-active condition revision.");
			return false;
		}
	}

	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Preparation =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::
			RestorePreparedFromLedger(
				Authority,
				Correlation,
				PrepareReceipt,
				TreatmentIntent);
	if (!Preparation.IsPrepared())
	{
		OutDiagnostic = Preparation.Diagnostic;
		return false;
	}

	Fdemo_mapShanmenCombatConditionTreatmentResult Treatment;
	if (bTreatmentAlreadyProcessed)
	{
		Treatment.Status =
			Edemo_mapShanmenCombatConditionTreatmentStatus::AlreadyTreated;
		Treatment.Error = Edemo_mapShanmenCombatConditionTreatmentError::None;
		Treatment.Receipt = TreatmentReceipt;
	}
	else
	{
		Treatment = ConditionComponent->TryTreatMeridianShock(
			TreatmentIntent,
			TimelineSample);
	}
	if (!Treatment.IsSuccess())
	{
		OutDiagnostic =
			TEXT("Durable prepare was restored, but exact condition treatment proof could not be established.");
		return false;
	}

	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::CommitTreated(
			Authority,
			Correlation,
			Preparation,
			Treatment.Receipt);
	if (!Finalization.IsFinalized()
		|| Finalization.Status
			!= Edemo_mapShanmenMeridianShockTreatmentStatus::Committed)
	{
		OutDiagnostic = Finalization.Diagnostic;
		return false;
	}
	OutRecoveredCount = 1;
	OutDiagnostic = bTreatmentAlreadyProcessed
		? TEXT("Recovered one durable prepare from runtime treatment proof and committed only its item Quantity.")
		: TEXT("Recovered one durable prepare from the still-active condition, then treated and committed in order.");
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsInGameThread()
		|| !bActive || !IsValid()
		|| ExpectedRunId != Correlation.ActiveRunId)
	{
		OutDiagnostic = TEXT("Treatment route end requires its exact valid active Run.");
		return false;
	}
	if (HasUnresolvedRecovery())
	{
		OutDiagnostic = TEXT("Treatment route cannot forget prepared or treated recovery work.");
		return false;
	}
	Clear();
	OutDiagnostic = TEXT("Treatment route ended with no unresolved item transaction.");
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::IsValid() const
{
	if (!bActive)
	{
		return IsEmpty();
	}
	if (!Correlation.IsValid()
		|| !ConditionComponent.IsValid()
		|| !ConditionComponent->IsValid()
		|| ConditionComponent->IsEmpty()
		|| ConditionComponent->GetRunId() != Correlation.ActiveRunId)
	{
		return false;
	}
	for (const TPair<FGuid, FJournalEntry>& Pair : Journal)
	{
		if (Pair.Key != Pair.Value.Command.GetRequestId()
			|| !Pair.Value.IsValid()
			|| Pair.Value.Command.GetCorrelation() != Correlation)
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::IsEmpty() const
{
	return !bActive
		&& !Correlation.IsValid()
		&& !ConditionComponent.IsValid()
		&& Journal.IsEmpty();
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductRoute::
	HasUnresolvedRecovery() const
{
	for (const TPair<FGuid, FJournalEntry>& Pair : Journal)
	{
		const FJournalEntry& Entry = Pair.Value;
		const bool bAmbiguousPrepare = !Entry.bPrepared
			&& Entry.Preparation.PrepareCommand.Status
				== EShanmenItemDurableCommandStatus::RecoveryRequired;
		if (bAmbiguousPrepare
			|| (Entry.bPrepared && !Entry.IsResolved()))
		{
			return true;
		}
	}
	return false;
}

void Fdemo_mapShanmenMeridianShockTreatmentProductRoute::Clear()
{
	bActive = false;
	Correlation = Fdemo_mapShanmenRunCorrelation();
	ConditionComponent.Reset();
	Journal.Reset();
#if WITH_DEV_AUTOMATION_TESTS
	bInterruptAfterTreatmentForAutomation = false;
#endif
}
