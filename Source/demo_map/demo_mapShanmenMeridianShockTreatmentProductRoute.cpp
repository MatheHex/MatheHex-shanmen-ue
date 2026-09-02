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

	bool RecoveryStorageMatches(
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Left,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Right)
	{
		return Left.OwnerId == Right.OwnerId
			&& Left.RunId == Right.RunId
			&& Left.RootDirectory.Equals(
				Right.RootDirectory,
				ESearchCase::IgnoreCase);
	}

	bool IsIntentForgetSuccess(
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult& Result)
	{
		return Result.Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentForgotten
			|| Result.Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					IntentAlreadyAbsent;
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
		|| bIntentRecorded && !bPrepared
		|| bTreated && (!bPrepared || !RecoveryIntent.IsValid())
		|| bProofRecorded && (!bTreated || bIntentRecorded)
		|| bCommitted && (!bPrepared || !bTreated || !bProofRecorded)
		|| bIntentCleanupPending && (!bCancelled || !bIntentRecorded)
		|| bCancelled && bIntentRecorded && !bIntentCleanupPending
		|| bProofCleanupPending && !bCommitted
		|| bCancelled
			&& (bTreated || bProofRecorded || bCommitted
				|| bProofCleanupPending))
	{
		return false;
	}
	if (RecoveryIntent.IsValid())
	{
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent RuntimeIntent;
		if (!bPrepared
			|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::
				TryCapture(
					Preparation.TreatmentIntent,
					Command.GetTimelineSample(),
					RuntimeIntent)
			|| !RuntimeIntent.Matches(RecoveryIntent))
		{
			return false;
		}
	}
	else if (bIntentRecorded || bTreated || bProofRecorded || bCommitted
		|| bIntentCleanupPending || bProofCleanupPending || bCancelled)
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
	if (RecoveryProof.IsValid())
	{
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof RuntimeProof;
		if (!bTreated
			|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::
				TryCapture(Treatment.Receipt, RuntimeProof)
			|| !RuntimeProof.Matches(RecoveryProof))
		{
			return false;
		}
	}
	else if (bProofRecorded || bCommitted || bProofCleanupPending)
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
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext&
		RequestedRecoveryStorage,
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
			&& ConditionComponent.Get() == RequestedConditionComponent
			&& RecoveryStorageMatches(
				RecoveryStorage,
				RequestedRecoveryStorage))
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
			!= RequestedCorrelation.ActiveRunId
		|| !RequestedRecoveryStorage.IsValid()
		|| RequestedRecoveryStorage.OwnerId != RequestedCorrelation.OwnerId
		|| RequestedRecoveryStorage.RunId
			!= RequestedCorrelation.ActiveRunId)
	{
		OutDiagnostic = TEXT("Treatment route begin requires empty state and matching active-Run authorities.");
		return false;
	}
	Correlation = RequestedCorrelation;
	RecoveryStorage = RequestedRecoveryStorage;
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
		if (Entry.bProofCleanupPending)
		{
			const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
				RecoveryStore.ForgetProof(
					Entry.RecoveryProof.GetTreatmentId(),
					RecoveryStorage);
			if (!Cleanup.IsSuccess())
			{
				Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result =
					Reject(
						Edemo_mapShanmenMeridianShockTreatmentRouteError::
							ProofCleanupRejected,
						&Command,
						TEXT("Committed item consumption retained a durable condition proof that still requires cleanup."),
						true);
				Result.Status =
					Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
						RecoveryRequired;
				Result.TreatmentId = Entry.RecoveryProof.GetTreatmentId();
				Result.Item = Entry.Finalization;
				Result.Treatment = Entry.Treatment;
				Result.Diagnostic = Cleanup.Diagnostic;
				return Result;
			}
			Entry.bProofCleanupPending = false;
		}
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
		if (Entry.bIntentCleanupPending)
		{
			const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
				RecoveryStore.ForgetIntent(
					Entry.RecoveryIntent.GetTreatmentId(),
					RecoveryStorage);
			if (!IsIntentForgetSuccess(Cleanup))
			{
				Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result =
					Reject(
						Edemo_mapShanmenMeridianShockTreatmentRouteError::
							IntentCleanupRejected,
						&Command,
						TEXT("Cancelled item reservation retained a durable pre-treatment intent that still requires cleanup."),
						true);
				Result.Status =
					Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
						RecoveryRequired;
				Result.TreatmentId = Entry.RecoveryIntent.GetTreatmentId();
				Result.Item = Entry.Finalization;
				Result.Treatment = Entry.Treatment;
				Result.Diagnostic = Cleanup.Diagnostic;
				return Result;
			}
			Entry.bIntentRecorded = false;
			Entry.bIntentCleanupPending = false;
		}
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

	if (!Entry.RecoveryIntent.IsValid()
		&& !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			Entry.Preparation.TreatmentIntent,
			Command.GetTimelineSample(),
			Entry.RecoveryIntent))
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				IntentCaptureRejected,
			&Command,
			TEXT("Durable item preparation succeeded, but its canonical write-ahead treatment intent could not be captured."),
			bReusedRequest);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::RecoveryRequired;
		Result.TreatmentId =
			Entry.Preparation.TreatmentIntent.GetTreatmentId();
		Result.Item = Entry.Preparation;
		return Result;
	}
	if (!Entry.bIntentRecorded && !Entry.bTreated && !Entry.bProofRecorded)
	{
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Persistence =
			RecoveryStore.RecordIntent(
				Entry.RecoveryIntent,
				RecoveryStorage);
		const bool bIntentRecorded = Persistence.Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentRecorded
			|| Persistence.Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					IntentAlreadyRecorded;
		if (!bIntentRecorded)
		{
			Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::
					IntentPersistenceRejected,
				&Command,
				TEXT("Durable item preparation is fenced from condition mutation until its exact write-ahead treatment intent is durable."),
				bReusedRequest);
			Result.Status =
				Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
					RecoveryRequired;
			Result.TreatmentId = Entry.RecoveryIntent.GetTreatmentId();
			Result.Item = Entry.Preparation;
			Result.Diagnostic = Persistence.Diagnostic;
			return Result;
		}
		Entry.bIntentRecorded = true;
	}

#if WITH_DEV_AUTOMATION_TESTS
	if (bInterruptAfterIntentPersistenceForAutomation && !Entry.bTreated)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				InterruptedAfterIntentPersistence,
			&Command,
			TEXT("Injected interruption retained a durable pre-treatment intent before condition mutation."),
			bReusedRequest);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::RecoveryRequired;
		Result.TreatmentId = Entry.RecoveryIntent.GetTreatmentId();
		Result.Item = Entry.Preparation;
		return Result;
	}
#endif

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
					Entry.bIntentCleanupPending = true;
					const Fdemo_mapShanmenTreatmentRecoveryMutationResult
						Cleanup = RecoveryStore.ForgetIntent(
							Entry.RecoveryIntent.GetTreatmentId(),
							RecoveryStorage);
					if (!IsIntentForgetSuccess(Cleanup))
					{
						Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result =
							Reject(
								Edemo_mapShanmenMeridianShockTreatmentRouteError::
									IntentCleanupRejected,
								&Command,
								TEXT("Condition treatment was rejected and item cancellation committed, but durable intent cleanup requires recovery."),
								bReusedRequest);
						Result.Status =
							Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
								RecoveryRequired;
						Result.TreatmentId =
							Entry.RecoveryIntent.GetTreatmentId();
						Result.Item = Entry.Finalization;
						Result.Treatment = Entry.Treatment;
						Result.Diagnostic = Cleanup.Diagnostic;
						return Result;
					}
					Entry.bIntentRecorded = false;
					Entry.bIntentCleanupPending = false;
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
	if (bInterruptAfterConditionMutationForAutomation
		&& !Entry.bProofRecorded)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				InterruptedAfterConditionMutation,
			&Command,
			TEXT("Injected interruption retained the durable intent after condition mutation and before proof promotion."),
			bReusedRequest);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::RecoveryRequired;
		Result.TreatmentId = Entry.RecoveryIntent.GetTreatmentId();
		Result.Item = Entry.Preparation;
		Result.Treatment = Entry.Treatment;
		return Result;
	}
#endif

	if (!Entry.RecoveryProof.IsValid()
		&& !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryCapture(
			Entry.Treatment.Receipt,
			Entry.RecoveryProof))
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				ProofCaptureRejected,
			&Command,
			TEXT("Condition treatment committed in memory, but its canonical recovery proof could not be captured."),
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
	if (!Entry.bProofRecorded)
	{
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Persistence =
			RecoveryStore.PromoteIntentToProof(
				Entry.RecoveryIntent,
				Entry.RecoveryProof,
				RecoveryStorage);
		if (!Persistence.IsSuccess())
		{
			Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::
					ProofPersistenceRejected,
				&Command,
				TEXT("Condition treatment committed in memory, but atomic intent-to-proof promotion failed before item commit."),
				bReusedRequest);
			Result.Status =
				Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
					RecoveryRequired;
			Result.TreatmentId = Entry.RecoveryProof.GetTreatmentId();
			Result.Item = Entry.Preparation;
			Result.Treatment = Entry.Treatment;
			Result.Diagnostic = Persistence.Diagnostic;
			return Result;
		}
		Entry.bIntentRecorded = false;
		Entry.bProofRecorded = true;
	}

#if WITH_DEV_AUTOMATION_TESTS
	if (bInterruptAfterProofPersistenceForAutomation)
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				InterruptedAfterProofPersistence,
			&Command,
			TEXT("Injected interruption retained a durable treatment proof for commit-only recovery."),
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
	Entry.bProofCleanupPending = true;
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
		RecoveryStore.ForgetProof(
			Entry.RecoveryProof.GetTreatmentId(),
			RecoveryStorage);
	if (!Cleanup.IsSuccess())
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::
				ProofCleanupRejected,
			&Command,
			TEXT("Item consumption committed, but durable treatment proof cleanup requires recovery."),
			bReusedRequest);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
				RecoveryRequired;
		Result.TreatmentId = Entry.RecoveryProof.GetTreatmentId();
		Result.Item = Entry.Finalization;
		Result.Treatment = Entry.Treatment;
		Result.Diagnostic = Cleanup.Diagnostic;
		return Result;
	}
	Entry.bProofCleanupPending = false;

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
	Result.Diagnostic = TEXT("Meridian Shock item prepare, durable intent, condition treatment, atomic proof promotion, item consumption and proof cleanup committed in order.");
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
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult RecoveryLoad =
		RecoveryStore.LoadExisting(RecoveryStorage);
	const bool bRecoveryStoreMissing = RecoveryLoad.Status
		== Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing;
	if (!RecoveryLoad.IsSuccess() && !bRecoveryStoreMissing)
	{
		OutDiagnostic = RecoveryLoad.Diagnostic;
		return false;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		OutDiagnostic =
			TEXT("Durable treatment recovery could not capture the item authority ledger.");
		return false;
	}
	TMap<FGuid, const FShanmenItemTransactionReceipt*> PreparesByTreatmentId;
	TMap<FGuid, const FShanmenItemTransactionReceipt*>
		FinalizationsByPrepareRequestId;
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
				== TArray<FGuid>({ Correlation.ActiveRunId }))
		{
			if (PreparesByTreatmentId.Contains(Receipt.ReservationId))
			{
				OutDiagnostic =
					TEXT("The durable item ledger contains duplicate treatment prepares for one TreatmentId.");
				return false;
			}
			PreparesByTreatmentId.Add(Receipt.ReservationId, &Receipt);
		}
		if (Receipt.IsSuccess()
			&& Receipt.IsValid()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.ReservationIds.Num() == 2)
		{
			const FGuid PrepareRequestId = Receipt.ReservationIds[1];
			if (FinalizationsByPrepareRequestId.Contains(PrepareRequestId))
			{
				OutDiagnostic =
					TEXT("The durable item ledger contains multiple terminal receipts for one treatment prepare.");
				return false;
			}
			FinalizationsByPrepareRequestId.Add(PrepareRequestId, &Receipt);
		}
	}

	auto FindPreparedItem = [&Snapshot](const FGuid& ItemInstanceId)
		-> const FShanmenItemInstance*
	{
		return Snapshot.Items.FindByPredicate(
			[&ItemInstanceId](const FShanmenItemInstance& Value)
			{
				return Value.ItemInstanceId == ItemInstanceId;
			});
	};
	auto TerminalMatchesPrepare = [this](
		const FShanmenItemTransactionReceipt& Finalization,
		const FShanmenItemTransactionReceipt& Prepare,
		const FGuid& TreatmentId,
		const FGuid& ItemInstanceId)
	{
		return Finalization.ReservationId == TreatmentId
			&& Finalization.ItemInstanceId == ItemInstanceId
			&& Finalization.Amount == Prepare.Amount
			&& Finalization.PurposeId == Prepare.PurposeId
			&& Finalization.ReservationIds
				== TArray<FGuid>({ Correlation.ActiveRunId, Prepare.RequestId });
	};

	TArray<FGuid> CommittedProofsToForget;
	TArray<FGuid> CancelledIntentsToForget;
	if (!bRecoveryStoreMissing)
	{
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
			RecoveryLoad.Document.Proofs)
		{
			if (!Proof.IsValid()
				|| Proof.GetRunId() != Correlation.ActiveRunId
				|| Proof.GetTargetEntityId()
					!= ConditionComponent->GetTargetEntityId()
				|| Proof.GetTimelineId() != ConditionComponent->GetTimelineId()
				|| Proof.GetConditionDefinitionId()
					!= Udemo_mapShanmenCombatConditionComponent::
						MeridianShockDefinitionId())
			{
				OutDiagnostic =
					TEXT("The condition proof store contains proof outside the bound Run, target or timeline.");
				return false;
			}
			const FShanmenItemTransactionReceipt* const* PreparePointer =
				PreparesByTreatmentId.Find(Proof.GetTreatmentId());
			if (!PreparePointer || !*PreparePointer)
			{
				OutDiagnostic =
					TEXT("The condition proof store contains an orphan TreatmentId with no exact item prepare.");
				return false;
			}
			const FShanmenItemTransactionReceipt& Prepare = **PreparePointer;
			if (Prepare.ItemInstanceId != Proof.GetItemInstanceId())
			{
				OutDiagnostic =
					TEXT("The condition proof store conflicts with its item prepare identity.");
				return false;
			}

			const FShanmenItemTransactionReceipt* const* FinalizationPointer =
				FinalizationsByPrepareRequestId.Find(Prepare.RequestId);
			if (FinalizationPointer && *FinalizationPointer)
			{
				const FShanmenItemTransactionReceipt& Finalization =
					**FinalizationPointer;
				if (Finalization.Phase
						!= EShanmenItemTransactionPhase::Committed
					|| !TerminalMatchesPrepare(
						Finalization,
						Prepare,
						Proof.GetTreatmentId(),
						Proof.GetItemInstanceId()))
				{
					OutDiagnostic =
						TEXT("A stored condition proof conflicts with the terminal item receipt.");
					return false;
				}
				CommittedProofsToForget.Add(Proof.GetTreatmentId());
				continue;
			}
			const FShanmenItemInstance* PreparedItem =
				FindPreparedItem(Proof.GetItemInstanceId());
			if (!PreparedItem
				|| PreparedItem->DefinitionId
					!= Proof.GetItemDefinitionId())
			{
				OutDiagnostic =
					TEXT("A pending condition proof conflicts with the current item definition.");
				return false;
			}
		}

		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent :
			RecoveryLoad.Document.Intents)
		{
			if (!Intent.IsValid()
				|| Intent.GetRunId() != Correlation.ActiveRunId
				|| Intent.GetTargetEntityId()
					!= ConditionComponent->GetTargetEntityId()
				|| Intent.GetTimelineId()
					!= ConditionComponent->GetTimelineId()
				|| Intent.GetConditionDefinitionId()
					!= Udemo_mapShanmenCombatConditionComponent::
						MeridianShockDefinitionId())
			{
				OutDiagnostic =
					TEXT("The condition intent store contains an intent outside the bound Run, target or timeline.");
				return false;
			}
			const FShanmenItemTransactionReceipt* const* PreparePointer =
				PreparesByTreatmentId.Find(Intent.GetTreatmentId());
			if (!PreparePointer || !*PreparePointer)
			{
				OutDiagnostic =
					TEXT("The condition intent store contains an orphan TreatmentId with no exact item prepare.");
				return false;
			}
			const FShanmenItemTransactionReceipt& Prepare = **PreparePointer;
			if (Prepare.ItemInstanceId != Intent.GetItemInstanceId())
			{
				OutDiagnostic =
					TEXT("The condition intent store conflicts with its item prepare identity.");
				return false;
			}
			const FShanmenItemTransactionReceipt* const* FinalizationPointer =
				FinalizationsByPrepareRequestId.Find(Prepare.RequestId);
			if (FinalizationPointer && *FinalizationPointer)
			{
				const FShanmenItemTransactionReceipt& Finalization =
					**FinalizationPointer;
				if (Finalization.Phase
						!= EShanmenItemTransactionPhase::Cancelled
					|| !TerminalMatchesPrepare(
						Finalization,
						Prepare,
						Intent.GetTreatmentId(),
						Intent.GetItemInstanceId()))
				{
					OutDiagnostic =
						TEXT("A stored pre-treatment intent conflicts with the terminal item receipt.");
					return false;
				}
				CancelledIntentsToForget.Add(Intent.GetTreatmentId());
				continue;
			}
			const FShanmenItemInstance* PreparedItem =
				FindPreparedItem(Intent.GetItemInstanceId());
			if (!PreparedItem
				|| PreparedItem->DefinitionId
					!= Intent.GetItemDefinitionId())
			{
				OutDiagnostic =
					TEXT("A pending condition intent conflicts with the current item definition.");
				return false;
			}
		}
	}
	for (const FGuid& TreatmentId : CommittedProofsToForget)
	{
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
			RecoveryStore.ForgetProof(TreatmentId, RecoveryStorage);
		if (!Cleanup.IsSuccess())
		{
			OutDiagnostic = Cleanup.Diagnostic;
			return false;
		}
	}
	for (const FGuid& TreatmentId : CancelledIntentsToForget)
	{
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
			RecoveryStore.ForgetIntent(TreatmentId, RecoveryStorage);
		if (!IsIntentForgetSuccess(Cleanup))
		{
			OutDiagnostic = Cleanup.Diagnostic;
			return false;
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
			&& !FinalizationsByPrepareRequestId.Contains(Receipt.RequestId))
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
		OutDiagnostic = CommittedProofsToForget.IsEmpty()
			&& CancelledIntentsToForget.IsEmpty()
			? TEXT("The durable item ledger contains no pending Meridian Shock treatment.")
			: TEXT("Terminal item receipts were verified and their stale condition recovery evidence was removed.");
		return true;
	}
	if (Pending.Num() != 1)
	{
		OutDiagnostic =
			TEXT("Multiple pending Meridian Shock prepares are ambiguous for one condition authority.");
		return false;
	}

	const FShanmenItemTransactionReceipt& PrepareReceipt = Pending[0];
	const FShanmenItemInstance* PreparedItem =
		FindPreparedItem(PrepareReceipt.ItemInstanceId);
	if (!PreparedItem)
	{
		OutDiagnostic =
			TEXT("Pending treatment prepare no longer identifies one item authority instance.");
		return false;
	}
	for (const TPair<FGuid, FJournalEntry>& Pair : Journal)
	{
		const FJournalEntry& Entry = Pair.Value;
		if (Entry.bPrepared
			&& !Entry.IsResolved()
			&& Entry.Preparation.TreatmentIntent.GetTreatmentId()
				== PrepareReceipt.ReservationId)
		{
			OutDiagnostic =
				TEXT("The pending durable treatment remains owned by its live transient journal.");
			return true;
		}
	}

	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent*
		DurableTreatmentIntent = nullptr;
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof*
		DurableTreatmentProof = nullptr;
	if (!bRecoveryStoreMissing)
	{
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent :
			RecoveryLoad.Document.Intents)
		{
			if (Intent.GetTreatmentId() == PrepareReceipt.ReservationId)
			{
				DurableTreatmentIntent = &Intent;
				break;
			}
		}
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
			RecoveryLoad.Document.Proofs)
		{
			if (Proof.GetTreatmentId() == PrepareReceipt.ReservationId)
			{
				DurableTreatmentProof = &Proof;
				break;
			}
		}
	}
	if (DurableTreatmentIntent && DurableTreatmentProof)
	{
		OutDiagnostic =
			TEXT("One TreatmentId cannot be both a durable intent and a durable proof.");
		return false;
	}
	if (DurableTreatmentIntent
		&& (DurableTreatmentIntent->GetRunId()
				!= Correlation.ActiveRunId
			|| DurableTreatmentIntent->GetItemInstanceId()
				!= PrepareReceipt.ItemInstanceId
			|| DurableTreatmentIntent->GetItemDefinitionId()
				!= PreparedItem->DefinitionId))
	{
		OutDiagnostic =
			TEXT("Durable treatment intent does not match the pending item prepare.");
		return false;
	}
	if (DurableTreatmentProof
		&& (DurableTreatmentProof->GetRunId()
				!= Correlation.ActiveRunId
			|| DurableTreatmentProof->GetItemInstanceId()
				!= PrepareReceipt.ItemInstanceId
			|| DurableTreatmentProof->GetItemDefinitionId()
				!= PreparedItem->DefinitionId))
	{
		OutDiagnostic =
			TEXT("Durable treatment proof does not match the pending item prepare.");
		return false;
	}

	Fdemo_mapShanmenCombatConditionTreatmentReceipt TreatmentReceipt;
	const bool bTreatmentAlreadyProcessed =
		ConditionComponent->TryGetProcessedMeridianShockTreatment(
			PrepareReceipt.ReservationId,
			TreatmentReceipt);

	if (DurableTreatmentIntent)
	{
		Fdemo_mapShanmenCombatConditionTreatmentIntent TreatmentIntent;
		Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
		if (!DurableTreatmentIntent->TryRestore(
				TreatmentIntent,
				TimelineSample)
			|| TreatmentIntent.GetTreatmentId()
				!= PrepareReceipt.ReservationId)
		{
			OutDiagnostic =
				TEXT("Durable treatment intent cannot restore the pending prepare identity.");
			return false;
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

		if (bTreatmentAlreadyProcessed)
		{
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof RuntimeProof;
			if (!TreatmentReceipt.Matches(TreatmentIntent)
				|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::
					TryCapture(TreatmentReceipt, RuntimeProof))
			{
				OutDiagnostic =
					TEXT("Processed condition receipt conflicts with its durable write-ahead intent.");
				return false;
			}
			const Fdemo_mapShanmenTreatmentRecoveryMutationResult Promotion =
				RecoveryStore.PromoteIntentToProof(
					*DurableTreatmentIntent,
					RuntimeProof,
					RecoveryStorage);
			if (!Promotion.IsSuccess())
			{
				OutDiagnostic = Promotion.Diagnostic;
				return false;
			}
			const Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization =
				Fdemo_mapShanmenMeridianShockTreatmentAdapter::CommitTreated(
					Authority,
					Correlation,
					Preparation,
					TreatmentReceipt);
			if (!Finalization.IsFinalized()
				|| Finalization.Status
					!= Edemo_mapShanmenMeridianShockTreatmentStatus::Committed)
			{
				OutDiagnostic = Finalization.Diagnostic;
				return false;
			}
			const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
				RecoveryStore.ForgetProof(
					RuntimeProof.GetTreatmentId(),
					RecoveryStorage);
			if (!Cleanup.IsSuccess())
			{
				OutDiagnostic = Cleanup.Diagnostic;
				return false;
			}
			OutRecoveredCount = 1;
			OutDiagnostic =
				TEXT("Recovered a post-mutation durable intent by exact receipt, promoted its proof and committed item Quantity once.");
			return true;
		}

		Fdemo_mapShanmenCombatConditionStatusSnapshot LiveStatus;
		if (!ConditionComponent->TryCaptureMeridianShockStatus(LiveStatus)
			|| !LiveStatus.IsActive()
			|| LiveStatus.GetRunId() != TreatmentIntent.GetRunId()
			|| LiveStatus.GetTargetEntityId()
				!= TreatmentIntent.GetTargetEntityId()
			|| LiveStatus.GetTimelineId() != TreatmentIntent.GetTimelineId()
			|| LiveStatus.GetDefinitionId()
				!= TreatmentIntent.GetConditionDefinitionId()
			|| LiveStatus.GetConditionRevision()
				!= TreatmentIntent.GetExpectedConditionRevision())
		{
			OutDiagnostic =
				TEXT("Durable treatment intent is ambiguous: neither its exact processed receipt nor its unchanged active condition revision exists.");
			return false;
		}
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization =
			Fdemo_mapShanmenMeridianShockTreatmentAdapter::CancelBeforeTreatment(
				Authority,
				Correlation,
				Preparation,
				LiveStatus);
		if (!Finalization.IsFinalized()
			|| Finalization.Status
				!= Edemo_mapShanmenMeridianShockTreatmentStatus::Cancelled)
		{
			OutDiagnostic = Finalization.Diagnostic;
			return false;
		}
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
			RecoveryStore.ForgetIntent(
				DurableTreatmentIntent->GetTreatmentId(),
				RecoveryStorage);
		if (!IsIntentForgetSuccess(Cleanup))
		{
			OutDiagnostic = Cleanup.Diagnostic;
			return false;
		}
		OutRecoveredCount = 1;
		OutDiagnostic =
			TEXT("Recovered a pre-mutation durable intent by cancelling its item reservation at the exact unchanged condition revision.");
		return true;
	}

	if (DurableTreatmentProof)
	{
		Fdemo_mapShanmenCombatConditionTreatmentIntent TreatmentIntent;
		Fdemo_mapShanmenCombatConditionTreatmentReceipt ProofReceipt;
		if (!DurableTreatmentProof->TryRestore(
				TreatmentIntent,
				ProofReceipt))
		{
			OutDiagnostic =
				TEXT("Durable treatment proof could not restore canonical condition evidence.");
			return false;
		}
		if (bTreatmentAlreadyProcessed)
		{
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof RuntimeProof;
			if (!TreatmentReceipt.Matches(TreatmentIntent)
				|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::
					TryCapture(TreatmentReceipt, RuntimeProof)
				|| !RuntimeProof.Matches(*DurableTreatmentProof))
			{
				OutDiagnostic =
					TEXT("Runtime and durable condition treatment proofs conflict.");
				return false;
			}
		}
		else
		{
			FString RestoreDiagnostic;
			if (!ConditionComponent->TryRestoreProcessedMeridianShockTreatment(
					*DurableTreatmentProof,
					RestoreDiagnostic)
				|| !ConditionComponent->TryGetProcessedMeridianShockTreatment(
					PrepareReceipt.ReservationId,
					TreatmentReceipt))
			{
				OutDiagnostic = RestoreDiagnostic.IsEmpty()
					? TEXT("Condition authority rejected the durable treatment proof.")
					: RestoreDiagnostic;
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
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization =
			Fdemo_mapShanmenMeridianShockTreatmentAdapter::CommitTreated(
				Authority,
				Correlation,
				Preparation,
				TreatmentReceipt);
		if (!Finalization.IsFinalized()
			|| Finalization.Status
				!= Edemo_mapShanmenMeridianShockTreatmentStatus::Committed)
		{
			OutDiagnostic = Finalization.Diagnostic;
			return false;
		}
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
			RecoveryStore.ForgetProof(
				DurableTreatmentProof->GetTreatmentId(),
				RecoveryStorage);
		if (!Cleanup.IsSuccess())
		{
			OutDiagnostic = Cleanup.Diagnostic;
			return false;
		}
		OutRecoveredCount = 1;
		OutDiagnostic =
			TEXT("Recovered one durable prepare from canonical condition proof and committed only its item Quantity.");
		return true;
	}

	if (bTreatmentAlreadyProcessed)
	{
		Fdemo_mapShanmenCombatConditionTreatmentIntent TreatmentIntent;
		if (!Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
				TreatmentReceipt.GetRunId(),
				TreatmentReceipt.GetTargetEntityId(),
				TreatmentReceipt.GetTimelineId(),
				TreatmentReceipt.GetItemInstanceId(),
				TreatmentReceipt.GetItemDefinitionId(),
				TreatmentReceipt.GetConditionRevisionBefore(),
				TreatmentIntent)
			|| !TreatmentReceipt.Matches(TreatmentIntent))
		{
			OutDiagnostic =
				TEXT("Legacy runtime treatment proof cannot reconstruct the pending prepare identity.");
			return false;
		}
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult Preparation =
			Fdemo_mapShanmenMeridianShockTreatmentAdapter::
				RestorePreparedFromLedger(
					Authority,
					Correlation,
					PrepareReceipt,
					TreatmentIntent);
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof RuntimeProof;
		if (!Preparation.IsPrepared()
			|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryCapture(
				TreatmentReceipt,
				RuntimeProof))
		{
			OutDiagnostic = Preparation.IsPrepared()
				? TEXT("Legacy runtime treatment receipt could not form canonical proof.")
				: Preparation.Diagnostic;
			return false;
		}
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Persistence =
			RecoveryStore.RecordProof(RuntimeProof, RecoveryStorage);
		if (!Persistence.IsSuccess())
		{
			OutDiagnostic = Persistence.Diagnostic;
			return false;
		}
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization =
			Fdemo_mapShanmenMeridianShockTreatmentAdapter::CommitTreated(
				Authority,
				Correlation,
				Preparation,
				TreatmentReceipt);
		if (!Finalization.IsFinalized()
			|| Finalization.Status
				!= Edemo_mapShanmenMeridianShockTreatmentStatus::Committed)
		{
			OutDiagnostic = Finalization.Diagnostic;
			return false;
		}
		const Fdemo_mapShanmenTreatmentRecoveryMutationResult Cleanup =
			RecoveryStore.ForgetProof(RuntimeProof.GetTreatmentId(), RecoveryStorage);
		if (!Cleanup.IsSuccess())
		{
			OutDiagnostic = Cleanup.Diagnostic;
			return false;
		}
		OutRecoveredCount = 1;
		OutDiagnostic =
			TEXT("Recovered one legacy post-mutation prepare from exact runtime proof.");
		return true;
	}

	Fdemo_mapShanmenCombatConditionStatusSnapshot LiveStatus;
	Fdemo_mapShanmenCombatConditionTreatmentIntent TreatmentIntent;
	if (!ConditionComponent->TryCaptureMeridianShockStatus(LiveStatus)
		|| !LiveStatus.IsActive()
		|| !Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			LiveStatus.GetRunId(),
			LiveStatus.GetTargetEntityId(),
			LiveStatus.GetTimelineId(),
			PrepareReceipt.ItemInstanceId,
			PreparedItem->DefinitionId,
			LiveStatus.GetConditionRevision(),
			TreatmentIntent)
		|| TreatmentIntent.GetTreatmentId() != PrepareReceipt.ReservationId)
	{
		OutDiagnostic =
			TEXT("Pending legacy prepare has neither durable evidence, exact runtime proof nor an unchanged active condition revision.");
		return false;
	}
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Preparation =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::RestorePreparedFromLedger(
			Authority,
			Correlation,
			PrepareReceipt,
			TreatmentIntent);
	if (!Preparation.IsPrepared())
	{
		OutDiagnostic = Preparation.Diagnostic;
		return false;
	}
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::CancelBeforeTreatment(
			Authority,
			Correlation,
			Preparation,
			LiveStatus);
	if (!Finalization.IsFinalized()
		|| Finalization.Status
			!= Edemo_mapShanmenMeridianShockTreatmentStatus::Cancelled)
	{
		OutDiagnostic = Finalization.Diagnostic;
		return false;
	}
	OutRecoveredCount = 1;
	OutDiagnostic =
		TEXT("Recovered one legacy pre-mutation prepare by safe cancellation; no condition mutation was replayed.");
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
		|| !RecoveryStorage.IsValid()
		|| RecoveryStorage.OwnerId != Correlation.OwnerId
		|| RecoveryStorage.RunId != Correlation.ActiveRunId
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
		&& RecoveryStorage.RootDirectory.IsEmpty()
		&& !RecoveryStorage.OwnerId.IsValid()
		&& !RecoveryStorage.RunId.IsValid()
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
	RecoveryStorage = Fdemo_mapShanmenTreatmentRecoveryStorageContext();
	ConditionComponent.Reset();
	Journal.Reset();
#if WITH_DEV_AUTOMATION_TESTS
	bInterruptAfterIntentPersistenceForAutomation = false;
	bInterruptAfterConditionMutationForAutomation = false;
	bInterruptAfterProofPersistenceForAutomation = false;
#endif
}
