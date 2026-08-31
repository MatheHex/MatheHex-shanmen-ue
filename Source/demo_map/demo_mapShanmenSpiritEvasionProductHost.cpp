#include "demo_mapShanmenSpiritEvasionProductHost.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		FPlatformMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
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

	FGuid MakeHostId(
		const Fdemo_mapShanmenSpiritEvasionActionCoordinator& Coordinator,
		const FShanmenActionOrchestrator& ActionRuntime,
		double StartTimeSeconds)
	{
		if (!Coordinator.IsValid()
			|| !ActionRuntime.IsValid()
			|| !FMath::IsFinite(StartTimeSeconds)
			|| StartTimeSeconds < 0.0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.ProductHost.r1"),
			{
				GuidDigits(Coordinator.GetCoordinatorId()),
				GuidDigits(ActionRuntime.GetAction().GetActivationId()),
				DoubleBits(StartTimeSeconds)
			});
	}

	Fdemo_mapShanmenSpiritEvasionHostStartResult RejectStart(
		Edemo_mapShanmenSpiritEvasionHostStartError Error)
	{
		Fdemo_mapShanmenSpiritEvasionHostStartResult Result;
		Result.Error = Error;
		return Result;
	}

	Fdemo_mapShanmenSpiritEvasionHostStepResult RejectStep(
		Edemo_mapShanmenSpiritEvasionHostStepError Error,
		const FGuid& HostId = FGuid())
	{
		Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
		Result.Error = Error;
		Result.HostId = HostId;
		return Result;
	}

	bool IsInterruptedTransition(
		const FShanmenActionTransitionReceipt& Transition)
	{
		return Transition.IsValid()
			&& Transition.GetToPhase()
				== EShanmenCombatActionPhase::Interrupted
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted;
	}

	bool IsRecoveryTransition(
		const FShanmenActionTransitionReceipt& Transition)
	{
		return Transition.IsValid()
			&& Transition.GetFromPhase()
				== EShanmenCombatActionPhase::Active
			&& Transition.GetToPhase()
				== EShanmenCombatActionPhase::Recovery
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::None;
	}

	bool IsCompletionTransition(
		const FShanmenActionTransitionReceipt& Transition)
	{
		return Transition.IsValid()
			&& Transition.GetFromPhase()
				== EShanmenCombatActionPhase::Recovery
			&& Transition.GetToPhase()
				== EShanmenCombatActionPhase::Idle
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::Completed;
	}
}

Fdemo_mapShanmenSpiritEvasionMovementPreflightResult
Fdemo_mapShanmenSpiritEvasionCharacterPreflightPort::Evaluate(
	const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan)
{
	return Fdemo_mapShanmenSpiritEvasionMovementAdapter::PreflightWorldStatic(
		Character, Plan);
}

bool Fdemo_mapShanmenSpiritEvasionHostStartResult::IsValid() const
{
	if (Status == Edemo_mapShanmenSpiritEvasionHostStartStatus::Rejected)
	{
		return Error
			!= Edemo_mapShanmenSpiritEvasionHostStartError::None
			&& !HostId.IsValid();
	}
	return Status == Edemo_mapShanmenSpiritEvasionHostStartStatus::Started
		&& Error == Edemo_mapShanmenSpiritEvasionHostStartError::None
		&& HostId.IsValid()
		&& Startup.IsValid()
		&& Startup.GetFromPhase() == EShanmenCombatActionPhase::Idle
		&& Startup.GetToPhase() == EShanmenCombatActionPhase::Startup
		&& ActiveCommit.IsValid()
		&& ActiveCommit.GetFromPhase()
			== EShanmenCombatActionPhase::Startup
		&& ActiveCommit.GetToPhase() == EShanmenCombatActionPhase::Active
		&& ActiveCommit.CrossedCommitPointNow()
		&& Window.IsValid()
		&& Window.GetCommitTransition().GetSequence()
			== ActiveCommit.GetSequence()
		&& Preflight.IsReady();
}

bool Fdemo_mapShanmenSpiritEvasionHostStartResult::IsSuccess() const
{
	return IsValid()
		&& Status == Edemo_mapShanmenSpiritEvasionHostStartStatus::Started;
}

bool Fdemo_mapShanmenSpiritEvasionHostStepResult::IsValid() const
{
	if (Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::Rejected)
	{
		return Error != Edemo_mapShanmenSpiritEvasionHostStepError::None;
	}
	if (!HostId.IsValid()
		|| Error != Edemo_mapShanmenSpiritEvasionHostStepError::None)
	{
		return false;
	}
	if (Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::Waiting)
	{
		return Motion.IsSuccess()
			&& Motion.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Waiting
			&& !ActionTransition.IsValid();
	}
	if (Status
		== Edemo_mapShanmenSpiritEvasionHostStepStatus::SegmentCommitted)
	{
		return Motion.IsSuccess()
			&& Motion.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::
				SegmentCommitted
			&& !ActionTransition.IsValid();
	}
	if (Status
		== Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryCompleted)
	{
		return Motion.IsSuccess()
			&& Motion.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Completed
			&& IsRecoveryTransition(ActionTransition);
	}
	if (Status
		== Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryBlocked)
	{
		return Motion.IsSuccess()
			&& Motion.Status
				== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Blocked
			&& IsRecoveryTransition(ActionTransition);
	}
	if (Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::Completed)
	{
		return !Motion.IsValid()
			&& IsCompletionTransition(ActionTransition);
	}
	if (Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::AlreadyTerminal)
	{
		return !Motion.IsValid() && !ActionTransition.IsValid();
	}
	if (Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::Cancelled
		|| Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::Interrupted
		|| Status == Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded
		|| Status
			== Edemo_mapShanmenSpiritEvasionHostStepStatus::
			ExecutionUnavailable)
	{
		if (!IsInterruptedTransition(ActionTransition))
		{
			return false;
		}
		if (!Motion.IsValid())
		{
			return ActionTransition.GetFromPhase()
				== EShanmenCombatActionPhase::Recovery
				&& Status
					!= Edemo_mapShanmenSpiritEvasionHostStepStatus::
					ExecutionUnavailable;
		}
		if (!Motion.IsSuccess()
			|| Motion.Status
				!= Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Terminated)
		{
			return false;
		}
		const Edemo_mapShanmenSpiritEvasionMotionTerminationReason Reason =
			Motion.Termination.GetReason();
		switch (Status)
		{
		case Edemo_mapShanmenSpiritEvasionHostStepStatus::Cancelled:
			return Reason
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExplicitCancel;
		case Edemo_mapShanmenSpiritEvasionHostStepStatus::Interrupted:
			return Reason
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ActionInterrupted;
		case Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded:
			return Reason
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::OwnerEnded;
		case Edemo_mapShanmenSpiritEvasionHostStepStatus::ExecutionUnavailable:
			return Reason
				== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
				ExecutionUnavailable;
		default:
			return false;
		}
	}
	return false;
}

bool Fdemo_mapShanmenSpiritEvasionHostStepResult::IsSuccess() const
{
	return IsValid()
		&& Status != Edemo_mapShanmenSpiritEvasionHostStepStatus::Rejected;
}

Fdemo_mapShanmenSpiritEvasionHostStartResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryStart(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritEvasionDefinition& Definition,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
	const FVector& CandidateDirection,
	double InStartTimeSeconds,
	Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort,
	Fdemo_mapShanmenSpiritEvasionProductHost& OutHost)
{
	OutHost = Fdemo_mapShanmenSpiritEvasionProductHost();
	if (!Action.IsValid()
		|| !Definition.IsValid()
		|| !Policy.IsValid()
		|| !Trajectory.IsValid()
		|| Action.GetActionDefinitionId()
			!= FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
		|| Definition.GetActionDefinitionId()
			!= Action.GetActionDefinitionId()
		|| !FMath::IsFinite(InStartTimeSeconds)
		|| InStartTimeSeconds < 0.0)
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::InvalidInput);
	}

	Fdemo_mapShanmenSpiritEvasionProductHost Candidate;
	Fdemo_mapShanmenSpiritEvasionHostStartResult Result;
	if (!FShanmenActionOrchestrator::TryStart(
			Action, Candidate.ActionRuntime, Result.Startup))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::ActionStartRejected);
	}
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Result.ActiveCommit))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::ActionCommitRejected);
	}

	FShanmenSpiritEvasionWindow Window;
	if (!FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			Definition,
			Result.ActiveCommit,
			Candidate.ActionRuntime,
			Window,
			Result.Window))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::WindowRejected);
	}

	FShanmenSpiritEvasionMovementIntentCapture IntentCapture;
	IntentCapture.Action = Action;
	IntentCapture.MovementPolicyId = Policy.GetMovementPolicyId();
	IntentCapture.CandidateDirection = CandidateDirection;
	FShanmenSpiritEvasionMovementIntent Intent;
	if (!FShanmenSpiritEvasionMovementIntent::TryCapture(
			IntentCapture, Intent))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::IntentRejected);
	}

	FShanmenSpiritEvasionMovementRequest Request;
	if (!FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
			Intent, Window, Candidate.ActionRuntime, Request))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::
			MovementRequestRejected);
	}

	Fdemo_mapShanmenSpiritEvasionMovementPlan MovementPlan;
	if (!Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
			Request, Policy, MovementPlan))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::MovementPlanRejected);
	}
	Result.Preflight = PreflightPort.Evaluate(MovementPlan);
	if (!Result.Preflight.IsReady()
		|| Result.Preflight.PlanId != MovementPlan.GetPlanId()
		|| !Fdemo_mapShanmenSpiritEvasionMovementAdapter::
		IsResolvedDistanceAccepted(
			MovementPlan, Result.Preflight.Displacement.ResolvedDistance))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::PreflightRejected);
	}

	Fdemo_mapShanmenSpiritEvasionMotionPlan MotionPlan;
	if (!Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan(
			MovementPlan, Trajectory, MotionPlan))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::MotionPlanRejected);
	}
	if (!Fdemo_mapShanmenSpiritEvasionActionCoordinator::TryOpen(
			MotionPlan,
			Result.Preflight,
			Window,
			Candidate.ActionRuntime,
			Candidate.Coordinator))
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::CoordinatorRejected);
	}

	Candidate.StartTimeSeconds = InStartTimeSeconds;
	Candidate.HostId = MakeHostId(
		Candidate.Coordinator,
		Candidate.ActionRuntime,
		Candidate.StartTimeSeconds);
	Candidate.bStarted = true;
	if (!Candidate.IsValid())
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::StateDesynchronized);
	}

	Result.Status = Edemo_mapShanmenSpiritEvasionHostStartStatus::Started;
	Result.Error = Edemo_mapShanmenSpiritEvasionHostStartError::None;
	Result.HostId = Candidate.HostId;
	if (!Result.IsValid())
	{
		return RejectStart(
			Edemo_mapShanmenSpiritEvasionHostStartError::StateDesynchronized);
	}
	OutHost = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenSpiritEvasionHostStartResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryStartCharacter(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritEvasionDefinition& Definition,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
	const FVector& CandidateDirection,
	double InStartTimeSeconds,
	const ACharacter* Character,
	Fdemo_mapShanmenSpiritEvasionProductHost& OutHost)
{
	Fdemo_mapShanmenSpiritEvasionCharacterPreflightPort Port(Character);
	return TryStart(
		Action,
		Definition,
		Policy,
		Trajectory,
		CandidateDirection,
		InStartTimeSeconds,
		Port,
		OutHost);
}

bool Fdemo_mapShanmenSpiritEvasionProductHost::IsValid() const
{
	if (!bStarted
		|| !HostId.IsValid()
		|| !FMath::IsFinite(StartTimeSeconds)
		|| StartTimeSeconds < 0.0
		|| !ActionRuntime.IsValid()
		|| !Coordinator.IsValid()
		|| HostId != MakeHostId(Coordinator, ActionRuntime, StartTimeSeconds)
		|| !ActionsMatch(
			ActionRuntime.GetAction(),
			Coordinator.GetWindow().GetOpenReceipt().GetAction()))
	{
		return false;
	}

	const Edemo_mapShanmenSpiritEvasionMotionState MotionState =
		Coordinator.GetMotionSession().GetState();
	const EShanmenCombatActionPhase ActionPhase = ActionRuntime.GetPhase();
	const EShanmenActionTerminalReason TerminalReason =
		ActionRuntime.GetTerminalReason();
	if (MotionState == Edemo_mapShanmenSpiritEvasionMotionState::Active)
	{
		return Coordinator.IsActive()
			&& ActionPhase == EShanmenCombatActionPhase::Active
			&& TerminalReason == EShanmenActionTerminalReason::None;
	}
	if (MotionState == Edemo_mapShanmenSpiritEvasionMotionState::Completed
		|| MotionState == Edemo_mapShanmenSpiritEvasionMotionState::Blocked)
	{
		return (ActionPhase == EShanmenCombatActionPhase::Recovery
				&& TerminalReason == EShanmenActionTerminalReason::None)
			|| (ActionPhase == EShanmenCombatActionPhase::Idle
				&& TerminalReason == EShanmenActionTerminalReason::Completed)
			|| (ActionPhase == EShanmenCombatActionPhase::Interrupted
				&& TerminalReason == EShanmenActionTerminalReason::Interrupted);
	}
	if (MotionState == Edemo_mapShanmenSpiritEvasionMotionState::Terminated)
	{
		return ActionPhase == EShanmenCombatActionPhase::Interrupted
			&& TerminalReason == EShanmenActionTerminalReason::Interrupted;
	}
	return false;
}

bool Fdemo_mapShanmenSpiritEvasionProductHost::IsActive() const
{
	return IsValid()
		&& Coordinator.IsActive()
		&& ActionRuntime.GetPhase() == EShanmenCombatActionPhase::Active;
}

bool Fdemo_mapShanmenSpiritEvasionProductHost::IsRecovery() const
{
	if (!IsValid()
		|| ActionRuntime.GetPhase() != EShanmenCombatActionPhase::Recovery)
	{
		return false;
	}
	const Edemo_mapShanmenSpiritEvasionMotionState State =
		Coordinator.GetMotionSession().GetState();
	return State == Edemo_mapShanmenSpiritEvasionMotionState::Completed
		|| State == Edemo_mapShanmenSpiritEvasionMotionState::Blocked;
}

bool Fdemo_mapShanmenSpiritEvasionProductHost::IsTerminal() const
{
	return IsValid() && ActionRuntime.IsTerminal();
}

double Fdemo_mapShanmenSpiritEvasionProductHost::
GetLastObservedTimeSeconds() const
{
	return IsValid()
		? StartTimeSeconds
			+ Coordinator.GetMotionSession().GetLastElapsedSeconds()
		: 0.0;
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryAdvance(
	double NowSeconds,
	Idemo_mapShanmenSpiritEvasionSegmentExecutionPort& ExecutionPort)
{
	if (!IsValid())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::HostNotReady,
			HostId);
	}
	if (!IsActive())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::HostNotActive,
			HostId);
	}
	if (!FMath::IsFinite(NowSeconds)
		|| NowSeconds < StartTimeSeconds
		|| NowSeconds < GetLastObservedTimeSeconds())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::InvalidTime,
			HostId);
	}

	Fdemo_mapShanmenSpiritEvasionProductHost Candidate = *this;
	Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
	Result.HostId = HostId;
	Result.Motion = Candidate.Coordinator.TryAdvance(
		Candidate.ActionRuntime,
		NowSeconds - Candidate.StartTimeSeconds,
		ExecutionPort);
	if (!Result.Motion.IsSuccess())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::CoordinatorRejected,
			HostId);
	}

	switch (Result.Motion.Status)
	{
	case Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Waiting:
		Result.Status = Edemo_mapShanmenSpiritEvasionHostStepStatus::Waiting;
		break;
	case Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::SegmentCommitted:
		Result.Status =
			Edemo_mapShanmenSpiritEvasionHostStepStatus::SegmentCommitted;
		break;
	case Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Completed:
	case Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Blocked:
		if (!Candidate.ActionRuntime.TryAdvance(
				EShanmenCombatActionPhase::Active,
				Result.ActionTransition))
		{
			return RejectStep(
				Edemo_mapShanmenSpiritEvasionHostStepError::
				ActionTransitionRejected,
				HostId);
		}
		Result.Status = Result.Motion.Status
			== Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Completed
			? Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryCompleted
			: Edemo_mapShanmenSpiritEvasionHostStepStatus::RecoveryBlocked;
		break;
	case Edemo_mapShanmenSpiritEvasionCoordinatorStepStatus::Terminated:
		if (!Candidate.ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Result.ActionTransition))
		{
			return RejectStep(
				Edemo_mapShanmenSpiritEvasionHostStepError::
				ActionTransitionRejected,
				HostId);
		}
		if (Result.Motion.Termination.GetReason()
			== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::OwnerEnded)
		{
			Result.Status =
				Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded;
		}
		else if (Result.Motion.Termination.GetReason()
			== Edemo_mapShanmenSpiritEvasionMotionTerminationReason::
			ExecutionUnavailable)
		{
			Result.Status = Edemo_mapShanmenSpiritEvasionHostStepStatus::
				ExecutionUnavailable;
		}
		else
		{
			return RejectStep(
				Edemo_mapShanmenSpiritEvasionHostStepError::
				StateDesynchronized,
				HostId);
		}
		break;
	default:
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::StateDesynchronized,
			HostId);
	}

	Result.Error = Edemo_mapShanmenSpiritEvasionHostStepError::None;
	if (!Candidate.IsValid() || !Result.IsValid())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::StateDesynchronized,
			HostId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryAdvanceCharacter(
	double NowSeconds,
	ACharacter* Character)
{
	Fdemo_mapShanmenSpiritEvasionCharacterExecutionPort Port(Character);
	return TryAdvance(NowSeconds, Port);
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::Terminate(
	Edemo_mapShanmenSpiritEvasionMotionTerminationReason MotionReason,
	Edemo_mapShanmenSpiritEvasionHostStepStatus ResultStatus)
{
	if (!IsValid())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::HostNotReady,
			HostId);
	}
	if (IsTerminal())
	{
		Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
		Result.Status =
			Edemo_mapShanmenSpiritEvasionHostStepStatus::AlreadyTerminal;
		Result.Error = Edemo_mapShanmenSpiritEvasionHostStepError::None;
		Result.HostId = HostId;
		return Result;
	}
	if (!IsActive() && !IsRecovery())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::HostNotActive,
			HostId);
	}

	Fdemo_mapShanmenSpiritEvasionProductHost Candidate = *this;
	Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
	Result.Status = ResultStatus;
	Result.Error = Edemo_mapShanmenSpiritEvasionHostStepError::None;
	Result.HostId = HostId;
	const EShanmenCombatActionPhase Phase = Candidate.ActionRuntime.GetPhase();
	if (Phase == EShanmenCombatActionPhase::Active)
	{
		Result.Motion = Candidate.Coordinator.TryTerminate(MotionReason);
		if (!Result.Motion.IsSuccess())
		{
			return RejectStep(
				Edemo_mapShanmenSpiritEvasionHostStepError::CoordinatorRejected,
				HostId);
		}
	}
	if (!Candidate.ActionRuntime.TryInterrupt(
			Phase, Result.ActionTransition))
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::
			ActionTransitionRejected,
			HostId);
	}
	if (!Candidate.IsValid() || !Result.IsValid())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::StateDesynchronized,
			HostId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryCancel()
{
	return Terminate(
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason::ExplicitCancel,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Cancelled);
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryInterrupt()
{
	return Terminate(
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason::ActionInterrupted,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::Interrupted);
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryOwnerEnd()
{
	return Terminate(
		Edemo_mapShanmenSpiritEvasionMotionTerminationReason::OwnerEnded,
		Edemo_mapShanmenSpiritEvasionHostStepStatus::OwnerEnded);
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Fdemo_mapShanmenSpiritEvasionProductHost::TryFinishRecovery()
{
	if (!IsValid())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::HostNotReady,
			HostId);
	}
	if (IsTerminal())
	{
		Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
		Result.Status =
			Edemo_mapShanmenSpiritEvasionHostStepStatus::AlreadyTerminal;
		Result.Error = Edemo_mapShanmenSpiritEvasionHostStepError::None;
		Result.HostId = HostId;
		return Result;
	}
	if (!IsRecovery())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::HostNotActive,
			HostId);
	}

	Fdemo_mapShanmenSpiritEvasionProductHost Candidate = *this;
	Fdemo_mapShanmenSpiritEvasionHostStepResult Result;
	Result.Status = Edemo_mapShanmenSpiritEvasionHostStepStatus::Completed;
	Result.Error = Edemo_mapShanmenSpiritEvasionHostStepError::None;
	Result.HostId = HostId;
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery,
			Result.ActionTransition))
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::
			ActionTransitionRejected,
			HostId);
	}
	if (!Candidate.IsValid() || !Result.IsValid())
	{
		return RejectStep(
			Edemo_mapShanmenSpiritEvasionHostStepError::StateDesynchronized,
			HostId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenSpiritEvasionProductHost::TryProjectDefenseLayer(
	FShanmenSpiritEvasionProjectionReceipt& OutReceipt) const
{
	OutReceipt = FShanmenSpiritEvasionProjectionReceipt();
	return IsActive()
		&& Coordinator.GetWindow().TryProjectDefenseLayer(
			ActionRuntime,
			OutReceipt);
}
