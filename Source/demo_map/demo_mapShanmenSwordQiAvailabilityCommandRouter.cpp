#include "demo_mapShanmenSwordQiAvailabilityCommandRouter.h"

namespace
{
	bool IsKnownKind(
		const Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind)
	{
		switch (Kind)
		{
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue:
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry:
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Cancel:
			return true;
		default:
			return false;
		}
	}

	bool IsAvailable(
		const Fdemo_mapShanmenSwordQiCommandAvailabilityProjection&
			Projection,
		const Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind)
	{
		switch (Kind)
		{
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue:
			return Projection.CanIssue();
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry:
			return Projection.CanRetry();
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Cancel:
			return Projection.CanCancel();
		default:
			return false;
		}
	}

	bool IsExpectedCommandEvent(
		const Fdemo_mapShanmenSwordQiCommandEventResult& Event,
		const Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind)
	{
		const bool bNormalStatus =
			Event.Status == Edemo_mapShanmenSwordQiCommandEventStatus::Applied
			|| Event.Status
				== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected;
		if (!bNormalStatus || !Event.Event.IsValid())
		{
			return false;
		}
		if (Kind == Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue)
		{
			return Event.bNewEvent && !Event.bPendingRetryAttempt;
		}
		if (Kind == Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry)
		{
			return !Event.bNewEvent
				&& Event.bEventCommitted
				&& Event.bPendingRetryAttempt
				&& Event.Request.IsValid();
		}
		return false;
	}

	bool HasValidTransition(
		const Fdemo_mapShanmenSwordQiAvailabilityCommandResult& Result)
	{
		if (!Result.Before.IsValid()
			|| !Result.After.IsValid()
			|| Result.Before.GetRunId() != Result.After.GetRunId())
		{
			return false;
		}

		switch (Result.Kind)
		{
		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue:
			if (!IsExpectedCommandEvent(Result.CommandEvent, Result.Kind))
			{
				return false;
			}
			if (!Result.CommandEvent.bEventCommitted)
			{
				return Result.After.GetProjectionId()
					== Result.Before.GetProjectionId();
			}
			if (Result.After.GetNextEventSequence()
				!= Result.Before.GetNextEventSequence() + 1)
			{
				return false;
			}
			return Result.CommandEvent.bPendingRetryStored
				? Result.After.CanRetry()
				: Result.After.CanIssue();

		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry:
			if (!IsExpectedCommandEvent(Result.CommandEvent, Result.Kind)
				|| Result.After.GetNextEventSequence()
					!= Result.Before.GetNextEventSequence())
			{
				return false;
			}
			return Result.CommandEvent.bPendingRetryStored
				? Result.After.CanRetry()
					&& Result.After.GetProjectionId()
						== Result.Before.GetProjectionId()
				: Result.After.CanIssue();

		case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Cancel:
			return Result.Cancellation.IsValid()
				&& Result.After.CanIssue()
				&& Result.After.GetNextEventSequence()
					== Result.Before.GetNextEventSequence()
				&& Result.After.GetProjectionId()
					!= Result.Before.GetProjectionId();

		default:
			return false;
		}
	}
}

bool Fdemo_mapShanmenSwordQiAvailabilityCommand::TryCapture(
	const FGuid& RequestedProjectionId,
	const Edemo_mapShanmenSwordQiAvailabilityCommandKind RequestedKind,
	Fdemo_mapShanmenSwordQiAvailabilityCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordQiAvailabilityCommand();
	if (!RequestedProjectionId.IsValid() || !IsKnownKind(RequestedKind))
	{
		return false;
	}

	OutCommand.ExpectedProjectionId = RequestedProjectionId;
	OutCommand.Kind = RequestedKind;
	if (!OutCommand.IsValid())
	{
		OutCommand = Fdemo_mapShanmenSwordQiAvailabilityCommand();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordQiAvailabilityCommand::IsValid() const
{
	return ExpectedProjectionId.IsValid() && IsKnownKind(Kind);
}

Fdemo_mapShanmenSwordQiAvailabilityCommandResult
Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
	Fdemo_mapShanmenSwordQiCommandEventOwner& Owner,
	const Fdemo_mapShanmenSwordQiAvailabilityCommand& Command,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
		RouteIssueInput,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
		const FGuid&,
		const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput)
{
	Fdemo_mapShanmenSwordQiAvailabilityCommandResult Result;
	Result.Kind = Command.GetKind();
	Result.ExpectedProjectionId = Command.GetExpectedProjectionId();
	if (!Command.IsValid())
	{
		Result.Diagnostic = TEXT("Sword Qi availability command is invalid.");
		return Result;
	}

	FString ProjectionDiagnostic;
	if (!Owner.TryProjectAvailability(Result.Before, ProjectionDiagnostic))
	{
		Result.Status = Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
			ProjectionUnavailable;
		Result.Diagnostic = ProjectionDiagnostic;
		return Result;
	}
	if (Result.Before.GetProjectionId()
		!= Command.GetExpectedProjectionId())
	{
		Result.Status = Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
			ProjectionStale;
		Result.Diagnostic =
			TEXT("Sword Qi availability changed before command dispatch.");
		return Result;
	}
	if (!IsAvailable(Result.Before, Command.GetKind()))
	{
		Result.Status = Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
			CommandUnavailable;
		Result.Diagnostic =
			TEXT("Sword Qi decision is incompatible with current availability.");
		return Result;
	}

	bool bOwnerDispatchValid = false;
	switch (Command.GetKind())
	{
	case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue:
		Result.CommandEvent = Owner.TryIssue(RouteIssueInput);
		bOwnerDispatchValid =
			IsExpectedCommandEvent(Result.CommandEvent, Command.GetKind());
		break;
	case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry:
		Result.CommandEvent = Owner.TryRetryPending(RouteFrozenInput);
		bOwnerDispatchValid =
			IsExpectedCommandEvent(Result.CommandEvent, Command.GetKind());
		break;
	case Edemo_mapShanmenSwordQiAvailabilityCommandKind::Cancel:
		Result.Diagnostic.Reset();
		bOwnerDispatchValid = Owner.TryCancelPending(
			Result.Cancellation,
			Result.Diagnostic);
		break;
	default:
		break;
	}

	if (!bOwnerDispatchValid)
	{
		Result.Status =
			Edemo_mapShanmenSwordQiAvailabilityCommandStatus::OwnerRejected;
		if (Command.GetKind()
			!= Edemo_mapShanmenSwordQiAvailabilityCommandKind::Cancel)
		{
			Result.Diagnostic = Result.CommandEvent.Diagnostic;
		}
		if (Result.Diagnostic.IsEmpty())
		{
			Result.Diagnostic =
				TEXT("Sword Qi command-event owner rejected the decision.");
		}
		return Result;
	}

	FString AfterDiagnostic;
	if (!Owner.TryProjectAvailability(Result.After, AfterDiagnostic))
	{
		Result.Status = Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
			ProjectionPostconditionFailed;
		Result.Diagnostic = AfterDiagnostic;
		return Result;
	}
	if (!HasValidTransition(Result))
	{
		Result.Status = Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
			ProjectionPostconditionFailed;
		Result.Diagnostic =
			TEXT("Sword Qi availability transition failed closed.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordQiAvailabilityCommandStatus::Dispatched;
	Result.Diagnostic =
		TEXT("Sword Qi availability decision reached the command-event owner.");
	return Result;
}
