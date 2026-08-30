#include "demo_mapShanmenSpiritEvasionActionCoordinator.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
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
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	const FShanmenSpiritEvasionWindowReceipt& PlanWindow(
		const Fdemo_mapShanmenSpiritEvasionMotionPlan& Plan)
	{
		return Plan.GetMovementPlan().GetRequest().GetWindow();
	}

	FGuid MakeCoordinatorId(
		const Fdemo_mapShanmenSpiritEvasionMotionSession& Session,
		const FShanmenSpiritEvasionWindow& Window)
	{
		if (!Session.IsValid() || !Window.IsValid()
			|| PlanWindow(Session.GetMotionPlan()).GetReceiptId()
				!= Window.GetOpenReceipt().GetReceiptId())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.ActionCoordinator.r1"),
			{
				GuidDigits(Session.GetSessionId()),
				GuidDigits(Window.GetOpenReceipt().GetReceiptId())
			});
	}

	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Reject(
		Edemo_mapShanmenSpiritEvasionCoordinatorStepError Error,
		const FGuid& CoordinatorId = FGuid())
	{
		Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Result;
		Result.Status =
			Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Rejected;
		Result.Error = Error;
		Result.CoordinatorId = CoordinatorId;
		return Result;
	}

	bool TryMapInactiveRuntime(
		const FShanmenActionOrchestrator& Runtime,
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason& OutReason)
	{
		OutReason =
			Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None;
		if (!Runtime.IsValid())
		{
			return false;
		}
		if (Runtime.GetTerminalReason()
			== EShanmenActionTerminalReason::Interrupted
			|| Runtime.GetPhase() == EShanmenCombatActionPhase::Interrupted)
		{
			OutReason =
				Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ActionInterrupted;
			return true;
		}
		if (Runtime.GetPhase() == EShanmenCombatActionPhase::Recovery
			|| Runtime.GetPhase() == EShanmenCombatActionPhase::Idle
			|| Runtime.GetPhase() == EShanmenCombatActionPhase::Cancelled)
		{
			OutReason =
				Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ActionEnded;
			return true;
		}
		return false;
	}
}

Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult
Fdemo_mapShanmenSpiritEvasionCharacterExecutionPort::Execute(
	const Fdemo_mapShanmenSpiritEvasionSegmentCommand& Command)
{
	return Fdemo_mapShanmenSpiritEvasionSegmentExecutor::ExecuteSwept(
		Character, Command);
}

bool Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult::IsValid() const
{
	if (Status == Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Rejected)
	{
		return Error
			!= Edemo_mapShanmenSpiritEvasionCoordinatorStepError::None
			&& !Termination.IsValid();
	}
	if (!CoordinatorId.IsValid()
		|| Error != Edemo_mapShanmenSpiritEvasionCoordinatorStepError::None)
	{
		return false;
	}
	if (Status == Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Waiting)
	{
		return !Command.IsValid() && !Execution.HasReceipt()
			&& !Termination.IsValid();
	}
	if (Status
			== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::
			SegmentCommitted
		|| Status
			== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Completed
		|| Status
			== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Blocked)
	{
		return Command.IsValid()
			&& Execution.HasReceipt()
			&& Execution.CommandId == Command.GetCommandId()
			&& Execution.Receipt.GetCommandId() == Command.GetCommandId()
			&& (Execution.Receipt.WasBlocked()
				== (Status
					== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::
					Blocked))
			&& !Termination.IsValid();
	}
	if (Status
		== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Terminated)
	{
		return Termination.IsValid()
			&& (Command.IsValid()
				? Termination.GetPendingCommandId()
					== Command.GetCommandId()
				: !Termination.GetPendingCommandId().IsValid());
	}
	return false;
}

bool Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult::IsSuccess() const
{
	return IsValid()
		&& Status
			!= Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Rejected;
}

bool Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryOpen(
	const Fdemo_mapShanmenSpiritEvasionMotionPlan& MotionPlan,
	const Fdemo_mapShanmenSpiritEvasionMovementPreflightResult& Preflight,
	const FShanmenSpiritEvasionWindow& InWindow,
	const FShanmenActionOrchestrator& ActionRuntime,
	Fdemo_mapShanmenSpiritEvasionActionCoordinator& OutCoordinator)
{
	OutCoordinator.Reset();
	if (!MotionPlan.IsValid()
		|| !InWindow.IsActiveFor(ActionRuntime)
		|| PlanWindow(MotionPlan).GetReceiptId()
			!= InWindow.GetOpenReceipt().GetReceiptId())
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionActionCoordinator Candidate;
	Candidate.Window = InWindow;
	if (!Fdemo_mapShanmenSpiritEvasionMotionSession::TryStart(
			MotionPlan, Preflight, Candidate.MotionSession))
	{
		return false;
	}
	Candidate.CoordinatorId = MakeCoordinatorId(
		Candidate.MotionSession, Candidate.Window);
	Candidate.bOpen = true;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCoordinator = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionActionCoordinator::IsValid() const
{
	if (!bOpen || !CoordinatorId.IsValid()
		|| !Window.IsValid() || !MotionSession.IsValid()
		|| CoordinatorId != MakeCoordinatorId(MotionSession, Window))
	{
		return false;
	}
	const FShanmenSpiritEvasionWindowReceipt& ExpectedWindow =
		PlanWindow(MotionSession.GetMotionPlan());
	return ExpectedWindow.GetReceiptId()
			== Window.GetOpenReceipt().GetReceiptId()
		&& ActionsMatch(
			ExpectedWindow.GetAction(),
			Window.GetOpenReceipt().GetAction());
}

bool Fdemo_mapShanmenSpiritEvasionActionCoordinator::IsActive() const
{
	return IsValid()
		&& MotionSession.GetState()
			== Edemo_mapShanmenSpiritEvasionMotionState::Active;
}

Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult
Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryAdvance(
	const FShanmenActionOrchestrator& ActionRuntime,
	double ElapsedSeconds,
	Idemo_mapShanmenSpiritEvasionSegmentExecutionPort& ExecutionPort)
{
	if (!IsActive())
	{
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			CoordinatorNotReady,
			CoordinatorId);
	}
	const FShanmenCombatActionSnapshot& ExpectedAction =
		Window.GetOpenReceipt().GetAction();
	if (!ActionRuntime.IsValid()
		|| !ActionsMatch(ActionRuntime.GetAction(), ExpectedAction))
	{
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			ActionMismatch,
			CoordinatorId);
	}
	if (!Window.IsActiveFor(ActionRuntime))
	{
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason;
		if (!TryMapInactiveRuntime(ActionRuntime, Reason))
		{
			return Reject(
				Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
				StateDesynchronized,
				CoordinatorId);
		}
		return TryTerminate(Reason);
	}
	if (!FMath::IsFinite(ElapsedSeconds)
		|| ElapsedSeconds < MotionSession.GetLastElapsedSeconds())
	{
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			InvalidElapsed,
			CoordinatorId);
	}

	Fdemo_mapShanmenSpiritEvasionActionCoordinator Candidate = *this;
	Fdemo_mapShanmenSpiritEvasionSegmentCommand Command;
	if (!Candidate.MotionSession.TryIssueNextCommand(
			ElapsedSeconds, Command))
	{
		if (!Candidate.IsActive()
			|| Candidate.MotionSession.HasPendingCommand())
		{
			return Reject(
				Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
				StateDesynchronized,
				CoordinatorId);
		}
		*this = MoveTemp(Candidate);
		Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Waiting;
		Waiting.Status =
			Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Waiting;
		Waiting.CoordinatorId = CoordinatorId;
		return Waiting;
	}

	const Fdemo_mapShanmenSpiritEvasionSegmentExecutionResult Execution =
		ExecutionPort.Execute(Command);
	if (!Execution.HasReceipt()
		|| Execution.CommandId != Command.GetCommandId()
		|| Execution.Receipt.GetCommandId() != Command.GetCommandId()
		|| !Candidate.MotionSession.TryAcceptReceipt(Execution.Receipt))
	{
		const Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason =
			Execution.Status
				== Edemo_mapShanmenSpiritEvasionSegmentExecutionStatus::
				CharacterUnavailable
			? Edemo_mapShanmenSpiritEvasionMotionTerminationReason::OwnerEnded
			: Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExecutionUnavailable;
		Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt Termination;
		if (!Candidate.MotionSession.TryTerminate(Reason, Termination)
			|| !Candidate.IsValid())
		{
			return Reject(
				Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
				ExecutionRejected,
				CoordinatorId);
		}
		*this = MoveTemp(Candidate);
		Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Terminated;
		Terminated.Status =
			Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Terminated;
		Terminated.CoordinatorId = CoordinatorId;
		Terminated.Command = Command;
		Terminated.Execution = Execution;
		Terminated.Termination = Termination;
		return Terminated;
	}
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			StateDesynchronized,
			CoordinatorId);
	}

	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Result;
	Result.CoordinatorId = CoordinatorId;
	Result.Command = Command;
	Result.Execution = Execution;
	switch (Candidate.MotionSession.GetState())
	{
	case Edemo_mapShanmenSpiritEvasionMotionState::Active:
		Result.Status =
			Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::
			SegmentCommitted;
		break;
	case Edemo_mapShanmenSpiritEvasionMotionState::Completed:
		Result.Status =
			Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Completed;
		break;
	case Edemo_mapShanmenSpiritEvasionMotionState::Blocked:
		Result.Status =
			Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Blocked;
		break;
	default:
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			StateDesynchronized,
			CoordinatorId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult
Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryTerminate(
	Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason)
{
	if (!IsActive()
		|| Reason
			== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::None)
	{
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			CoordinatorNotReady,
			CoordinatorId);
	}
	Fdemo_mapShanmenSpiritEvasionActionCoordinator Candidate = *this;
	Fdemo_mapShanmenSpiritEvasionMotionTerminationReceipt Termination;
	if (!Candidate.MotionSession.TryTerminate(Reason, Termination)
		|| !Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSpiritEvasionCoordinatorStepError::
			StateDesynchronized,
			CoordinatorId);
	}
	*this = MoveTemp(Candidate);
	Fdemo_mapShanmenSpiritEvasionCoordinatorStepResult Result;
	Result.Status =
		Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Terminated;
	Result.CoordinatorId = CoordinatorId;
	Result.Termination = Termination;
	return Result;
}

void Fdemo_mapShanmenSpiritEvasionActionCoordinator::Reset()
{
	*this = Fdemo_mapShanmenSpiritEvasionActionCoordinator();
}
