#include "ShanmenSpiritShieldSession.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeScheduleId(
		const FGuid& TimelineId,
		int64 StartTick,
		int64 DeadlineTick)
	{
		if (!TimelineId.IsValid() || StartTick < 0
			|| DeadlineTick <= StartTick)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Schedule.r1"),
			{
				GuidDigits(TimelineId),
				FString::Printf(TEXT("%lld"), StartTick),
				FString::Printf(TEXT("%lld"), DeadlineTick)
			});
	}

	bool SchedulesMatch(
		const FShanmenSpiritShieldSchedule& Left,
		const FShanmenSpiritShieldSchedule& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetScheduleId() == Right.GetScheduleId()
			&& Left.GetTimelineId() == Right.GetTimelineId()
			&& Left.GetStartTick() == Right.GetStartTick()
			&& Left.GetDeadlineTick() == Right.GetDeadlineTick();
	}

	bool ActivationsMatch(
		const FShanmenSpiritShieldActivationReceipt& Left,
		const FShanmenSpiritShieldActivationReceipt& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetReceiptId() == Right.GetReceiptId()
			&& Left.GetShieldInstanceId() == Right.GetShieldInstanceId();
	}

	FShanmenSpiritShieldActionResult RejectAction(
		EShanmenSpiritShieldActionError Error,
		EShanmenActionResourceTransactionError ResourceError =
			EShanmenActionResourceTransactionError::None)
	{
		FShanmenSpiritShieldActionResult Result;
		Result.Status = EShanmenSpiritShieldActionStatus::Rejected;
		Result.Error = Error;
		Result.ResourceError = ResourceError;
		return Result;
	}

	FShanmenSpiritShieldCapacityCommitResult RejectCapacity(
		EShanmenSpiritShieldCapacityCommitError Error)
	{
		FShanmenSpiritShieldCapacityCommitResult Result;
		Result.Status = EShanmenSpiritShieldCapacityCommitStatus::Rejected;
		Result.Error = Error;
		return Result;
	}

	FShanmenSpiritShieldDeadlineClosureResult RejectDeadlineClosure(
		EShanmenSpiritShieldDeadlineClosureError Error,
		EShanmenSpiritShieldDeadlineError DeadlineError =
			EShanmenSpiritShieldDeadlineError::None,
		EShanmenSpiritShieldActionError ActionError =
			EShanmenSpiritShieldActionError::None)
	{
		FShanmenSpiritShieldDeadlineClosureResult Result;
		Result.Status =
			EShanmenSpiritShieldDeadlineClosureStatus::Rejected;
		Result.Error = Error;
		Result.DeadlineError = DeadlineError;
		Result.ActionError = ActionError;
		return Result;
	}
}

bool FShanmenSpiritShieldSchedule::TryCapture(
	const FGuid& InTimelineId,
	int64 InStartTick,
	int64 InDeadlineTick,
	FShanmenSpiritShieldSchedule& OutSchedule)
{
	OutSchedule = FShanmenSpiritShieldSchedule();
	FShanmenSpiritShieldSchedule Candidate;
	Candidate.TimelineId = InTimelineId;
	Candidate.StartTick = InStartTick;
	Candidate.DeadlineTick = InDeadlineTick;
	Candidate.ScheduleId = MakeScheduleId(
		InTimelineId, InStartTick, InDeadlineTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSchedule = Candidate;
	return true;
}

bool FShanmenSpiritShieldSchedule::IsValid() const
{
	return ScheduleId.IsValid() && TimelineId.IsValid()
		&& StartTick >= 0 && DeadlineTick > StartTick
		&& ScheduleId == MakeScheduleId(
			TimelineId, StartTick, DeadlineTick);
}

bool FShanmenSpiritShieldDeadlineClosureResult::IsValid() const
{
	if (Status == EShanmenSpiritShieldDeadlineClosureStatus::Closed
		|| Status
			== EShanmenSpiritShieldDeadlineClosureStatus::AlreadyClosed)
	{
		return Error == EShanmenSpiritShieldDeadlineClosureError::None
			&& DeadlineError == EShanmenSpiritShieldDeadlineError::None
			&& ActionError == EShanmenSpiritShieldActionError::None
			&& DeadlineReceipt.IsValid() && ClosureReceipt.IsValid()
			&& DeadlineReceipt.GetDeactivation().GetReceiptId()
				== ClosureReceipt.GetDeactivation().GetReceiptId()
			&& ClosureReceipt.GetOutcome()
				== EShanmenSpiritShieldActionClosureOutcome::Completed;
	}
	if (Status != EShanmenSpiritShieldDeadlineClosureStatus::Rejected
		|| Error == EShanmenSpiritShieldDeadlineClosureError::None
		|| DeadlineReceipt.IsValid() || ClosureReceipt.IsValid())
	{
		return false;
	}
	if (Error
		== EShanmenSpiritShieldDeadlineClosureError::DeadlineRejected)
	{
		return DeadlineError != EShanmenSpiritShieldDeadlineError::None
			&& ActionError == EShanmenSpiritShieldActionError::None;
	}
	if (Error
		== EShanmenSpiritShieldDeadlineClosureError::ActionClosureRejected)
	{
		return DeadlineError == EShanmenSpiritShieldDeadlineError::None
			&& ActionError != EShanmenSpiritShieldActionError::None;
	}
	return DeadlineError == EShanmenSpiritShieldDeadlineError::None
		&& ActionError == EShanmenSpiritShieldActionError::None;
}

bool FShanmenSpiritShieldDeadlineClosureResult::IsSuccess() const
{
	return IsValid()
		&& (Status == EShanmenSpiritShieldDeadlineClosureStatus::Closed
			|| Status
				== EShanmenSpiritShieldDeadlineClosureStatus::AlreadyClosed);
}

FShanmenSpiritShieldActionResult FShanmenSpiritShieldSession::Begin(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritShieldDefinition& Definition,
	const FShanmenActionResourceCost& Cost,
	const FShanmenSpiritShieldSchedule& InSchedule,
	FShanmenActionResourceAuthority& ResourceAuthority,
	FShanmenSpiritShieldSession& OutSession)
{
	if (!InSchedule.IsValid())
	{
		return RejectAction(EShanmenSpiritShieldActionError::InvalidInput);
	}

	if (OutSession.bInitialized)
	{
		if (!OutSession.IsValid())
		{
			return RejectAction(
				EShanmenSpiritShieldActionError::CoordinatorNotReady);
		}
		if (!SchedulesMatch(InSchedule, OutSession.Schedule))
		{
			return RejectAction(
				EShanmenSpiritShieldActionError::CoordinatorConflict);
		}
	}

	FShanmenSpiritShieldSession Candidate = OutSession;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	if (!Candidate.bInitialized)
	{
		Candidate.Schedule = InSchedule;
	}
	const FShanmenSpiritShieldActionResult Begun =
		FShanmenSpiritShieldActionCoordinator::Begin(
			Action,
			Definition,
			Cost,
			AuthorityCandidate,
			Candidate.ActionCoordinator);
	if (!Begun.IsSuccess())
	{
		return Begun;
	}
	Candidate.bInitialized = true;
	if (!Candidate.IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}

	ResourceAuthority = MoveTemp(AuthorityCandidate);
	OutSession = MoveTemp(Candidate);
	return Begun;
}

bool FShanmenSpiritShieldSession::HasActivatedAuthorities() const
{
	return CapacityAuthority.IsValid()
		&& DeadlineContract.IsValid()
		&& DeadlineGate.IsValid();
}

bool FShanmenSpiritShieldSession::IsValid() const
{
	if (!bInitialized || !Schedule.IsValid()
		|| !ActionCoordinator.IsValid())
	{
		return false;
	}

	const EShanmenSpiritShieldActionState State =
		ActionCoordinator.GetState();
	if (State == EShanmenSpiritShieldActionState::Reserved
		|| State == EShanmenSpiritShieldActionState::Aborted)
	{
		return !CapacityAuthority.IsValid()
			&& !DeadlineContract.IsValid()
			&& !DeadlineGate.IsValid();
	}
	if (State != EShanmenSpiritShieldActionState::Activated
		&& State != EShanmenSpiritShieldActionState::Completed
		&& State != EShanmenSpiritShieldActionState::Interrupted)
	{
		return false;
	}
	if (!HasActivatedAuthorities())
	{
		return false;
	}

	const FShanmenSpiritShieldActivationReceipt& Activation =
		ActionCoordinator.GetTerminalReceipt().GetShieldActivation();
	if (!ActivationsMatch(Activation, CapacityAuthority.GetActivation())
		|| !ActivationsMatch(
			Activation, DeadlineContract.GetActivation())
		|| DeadlineContract.GetTimelineId() != Schedule.GetTimelineId()
		|| DeadlineContract.GetStartTick() != Schedule.GetStartTick()
		|| DeadlineContract.GetDeadlineTick() != Schedule.GetDeadlineTick()
		|| DeadlineGate.GetContract().GetContractId()
			!= DeadlineContract.GetContractId())
	{
		return false;
	}

	if (State == EShanmenSpiritShieldActionState::Activated)
	{
		return ActionCoordinator.GetShieldRuntime().GetState()
				== EShanmenSpiritShieldState::Active
			&& !DeadlineGate.HasElapsed();
	}

	const FShanmenSpiritShieldActionClosureReceipt& Closure =
		ActionCoordinator.GetClosureReceipt();
	const EShanmenSpiritShieldDeactivationReason Reason =
		Closure.GetDeactivation().GetReason();
	if (Reason == EShanmenSpiritShieldDeactivationReason::DurationElapsed)
	{
		return State == EShanmenSpiritShieldActionState::Completed
			&& DeadlineGate.HasElapsed()
			&& DeadlineGate.GetElapsedReceipt().GetDeactivation()
				.GetReceiptId()
				== Closure.GetDeactivation().GetReceiptId();
	}
	return !DeadlineGate.HasElapsed()
		&& ((State == EShanmenSpiritShieldActionState::Completed
				&& Reason
					== EShanmenSpiritShieldDeactivationReason::Explicit)
			|| (State == EShanmenSpiritShieldActionState::Interrupted
				&& (Reason
						== EShanmenSpiritShieldDeactivationReason::Interrupted
					|| Reason
						== EShanmenSpiritShieldDeactivationReason::OwnerEnded)));
}

FShanmenSpiritShieldActionResult FShanmenSpiritShieldSession::Commit(
	FShanmenActionResourceAuthority& ResourceAuthority)
{
	if (!IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::CoordinatorNotReady);
	}

	FShanmenSpiritShieldSession Candidate = *this;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	const FShanmenSpiritShieldActionResult Activated =
		Candidate.ActionCoordinator.Commit(AuthorityCandidate);
	if (!Activated.IsSuccess())
	{
		return Activated;
	}
	if (Activated.Status == EShanmenSpiritShieldActionStatus::Activated)
	{
		const FShanmenSpiritShieldActivationReceipt& Activation =
			Activated.Terminal.GetShieldActivation();
		if (!FShanmenSpiritShieldCapacityAuthority::TryCreate(
				Activation, Candidate.CapacityAuthority)
			|| !FShanmenSpiritShieldDeadlineContract::TryCapture(
				Activation,
				Candidate.Schedule.GetTimelineId(),
				Candidate.Schedule.GetStartTick(),
				Candidate.Schedule.GetDeadlineTick(),
				Candidate.DeadlineContract)
			|| !FShanmenSpiritShieldDeadlineGate::TryCreate(
				Candidate.DeadlineContract, Candidate.DeadlineGate))
		{
			return RejectAction(
				EShanmenSpiritShieldActionError::StateDesynchronized);
		}
	}
	if (!Candidate.IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}

	ResourceAuthority = MoveTemp(AuthorityCandidate);
	*this = MoveTemp(Candidate);
	return Activated;
}

FShanmenSpiritShieldActionResult FShanmenSpiritShieldSession::Abort(
	EShanmenActionTerminalReason Reason,
	FShanmenActionResourceAuthority& ResourceAuthority)
{
	if (!IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::CoordinatorNotReady);
	}
	FShanmenSpiritShieldSession Candidate = *this;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	const FShanmenSpiritShieldActionResult Aborted =
		Candidate.ActionCoordinator.Abort(Reason, AuthorityCandidate);
	if (!Aborted.IsSuccess())
	{
		return Aborted;
	}
	if (!Candidate.IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}
	ResourceAuthority = MoveTemp(AuthorityCandidate);
	*this = MoveTemp(Candidate);
	return Aborted;
}

bool FShanmenSpiritShieldSession::TryProjectDefenseLayer(
	FShanmenSpiritShieldProjectionReceipt& OutProjection)
{
	OutProjection = FShanmenSpiritShieldProjectionReceipt();
	if (!IsValid()
		|| ActionCoordinator.GetState()
			!= EShanmenSpiritShieldActionState::Activated)
	{
		return false;
	}
	FShanmenSpiritShieldSession Candidate = *this;
	FShanmenSpiritShieldProjectionReceipt Projection;
	if (!Candidate.CapacityAuthority.TryProjectDefenseLayer(
			Candidate.ActionCoordinator.GetShieldRuntime(), Projection)
		|| !Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	OutProjection = Projection;
	return true;
}

FShanmenSpiritShieldCapacityCommitResult
FShanmenSpiritShieldSession::CommitCapacity(
	const FShanmenSpiritShieldCapacityCommitCommand& Command)
{
	if (!IsValid() || !HasActivatedAuthorities())
	{
		return RejectCapacity(
			EShanmenSpiritShieldCapacityCommitError::AuthorityNotReady);
	}

	FShanmenSpiritShieldSession Candidate = *this;
	const FShanmenSpiritShieldCapacityCommitResult Committed =
		Candidate.CapacityAuthority.Commit(Command);
	const bool bActive = ActionCoordinator.GetState()
		== EShanmenSpiritShieldActionState::Activated;
	if (!bActive)
	{
		return Committed.IsSuccess()
			&& Committed.Status
				== EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted
			? Committed
			: RejectCapacity(
				EShanmenSpiritShieldCapacityCommitError::AuthorityNotReady);
	}
	if (!Committed.IsSuccess())
	{
		return Committed;
	}
	if (!Candidate.IsValid())
	{
		return RejectCapacity(
			EShanmenSpiritShieldCapacityCommitError::AuthorityNotReady);
	}
	*this = MoveTemp(Candidate);
	return Committed;
}

FShanmenSpiritShieldDeadlineClosureResult
FShanmenSpiritShieldSession::ObserveDeadline(
	const FShanmenSpiritShieldTimelineObservation& Observation)
{
	if (!Observation.IsValid())
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::InvalidObservation);
	}
	if (!IsValid())
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::SessionNotReady);
	}

	const EShanmenSpiritShieldActionState State =
		ActionCoordinator.GetState();
	const bool bDeadlineReplay =
		State == EShanmenSpiritShieldActionState::Completed
		&& ActionCoordinator.GetClosureReceipt().GetDeactivation().GetReason()
			== EShanmenSpiritShieldDeactivationReason::DurationElapsed;
	if (State != EShanmenSpiritShieldActionState::Activated
		&& !bDeadlineReplay)
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::SessionNotReady);
	}

	FShanmenSpiritShieldSession Candidate = *this;
	const FShanmenSpiritShieldDeadlineResult Elapsed =
		Candidate.DeadlineGate.TryElapse(
			Candidate.ActionCoordinator.GetShieldRuntime(), Observation);
	if (!Elapsed.IsSuccess())
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::DeadlineRejected,
			Elapsed.Error);
	}
	const FShanmenSpiritShieldActionResult Closed =
		Candidate.ActionCoordinator.Close(
			EShanmenSpiritShieldDeactivationReason::DurationElapsed);
	if (!Closed.IsSuccess())
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::ActionClosureRejected,
			EShanmenSpiritShieldDeadlineError::None,
			Closed.Error);
	}
	if (Elapsed.Receipt.GetDeactivation().GetReceiptId()
			!= Closed.Closure.GetDeactivation().GetReceiptId()
		|| !Candidate.IsValid())
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::StateDesynchronized);
	}

	FShanmenSpiritShieldDeadlineClosureResult Result;
	Result.Status = Elapsed.Status
		== EShanmenSpiritShieldDeadlineStatus::Elapsed
		? EShanmenSpiritShieldDeadlineClosureStatus::Closed
		: EShanmenSpiritShieldDeadlineClosureStatus::AlreadyClosed;
	Result.DeadlineReceipt = Elapsed.Receipt;
	Result.ClosureReceipt = Closed.Closure;
	if (!Result.IsValid())
	{
		return RejectDeadlineClosure(
			EShanmenSpiritShieldDeadlineClosureError::StateDesynchronized);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

FShanmenSpiritShieldActionResult FShanmenSpiritShieldSession::Close(
	EShanmenSpiritShieldDeactivationReason Reason)
{
	if (Reason == EShanmenSpiritShieldDeactivationReason::DurationElapsed)
	{
		return RejectAction(EShanmenSpiritShieldActionError::InvalidInput);
	}
	if (!IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::CoordinatorNotReady);
	}
	FShanmenSpiritShieldSession Candidate = *this;
	const FShanmenSpiritShieldActionResult Closed =
		Candidate.ActionCoordinator.Close(Reason);
	if (!Closed.IsSuccess())
	{
		return Closed;
	}
	if (!Candidate.IsValid())
	{
		return RejectAction(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}
	*this = MoveTemp(Candidate);
	return Closed;
}

void FShanmenSpiritShieldSession::Reset()
{
	*this = FShanmenSpiritShieldSession();
}
