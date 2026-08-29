#include "demo_mapShanmenControlledWeaponSession.h"

namespace
{
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
}

bool Fdemo_mapShanmenControlledWeaponSession::TryStart(
	const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
	Fdemo_mapShanmenControlledWeaponSession& OutSession,
	FShanmenActionTransitionReceipt& OutStartup,
	FShanmenActionTransitionReceipt& OutActive)
{
	OutSession.Reset();
	OutStartup = FShanmenActionTransitionReceipt();
	OutActive = FShanmenActionTransitionReceipt();
	if (!Prepared.IsPrepared())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate;
	Candidate.Evidence = Prepared.Evidence;
	Candidate.Execution = Prepared.Execution;
	if (!FShanmenActionOrchestrator::TryStart(
			Prepared.Action, Candidate.ActionRuntime, OutStartup)
		|| !Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, OutActive))
	{
		OutStartup = FShanmenActionTransitionReceipt();
		OutActive = FShanmenActionTransitionReceipt();
		return false;
	}
	Candidate.State = Edemo_mapShanmenControlledWeaponSessionState::Active;
	if (!Candidate.IsValid())
	{
		OutStartup = FShanmenActionTransitionReceipt();
		OutActive = FShanmenActionTransitionReceipt();
		return false;
	}

	OutSession = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::IsValid() const
{
	if (!Evidence.IsValid()
		|| !ActionRuntime.IsValid()
		|| !Execution.IsValid()
		|| !ActionsMatch(
			ActionRuntime.GetAction(), Execution.GetAction())
		|| ActionRuntime.GetAction().GetRunId() != Evidence.ActiveRunId
		|| ActionRuntime.GetAction().GetOwnerId() != Evidence.OwnerId
		|| ActionRuntime.GetAction().GetSourceItemInstanceId()
			!= Evidence.ItemInstanceId
		|| ActionRuntime.GetAction().GetContent().Version
			!= Evidence.Content.Version
		|| ActionRuntime.GetAction().GetContent().Digest
			!= Evidence.Content.Digest)
	{
		return false;
	}

	switch (State)
	{
	case Edemo_mapShanmenControlledWeaponSessionState::Active:
		return !ActionRuntime.IsTerminal()
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Active
			&& Execution.GetState()
				!= EShanmenControlledWeaponState::Recalled;

	case Edemo_mapShanmenControlledWeaponSessionState::Completed:
		return ActionRuntime.IsTerminal()
			&& ActionRuntime.GetTerminalReason()
				== EShanmenActionTerminalReason::Completed
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Idle
			&& Execution.GetState()
				== EShanmenControlledWeaponState::Recalled
			&& !Execution.IsEmissionActive();

	case Edemo_mapShanmenControlledWeaponSessionState::Interrupted:
		return ActionRuntime.IsTerminal()
			&& ActionRuntime.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Interrupted
			&& Execution.GetState()
				!= EShanmenControlledWeaponState::Recalled
			&& !Execution.IsEmissionActive();

	case Edemo_mapShanmenControlledWeaponSessionState::Empty:
	default:
		return false;
	}
}

bool Fdemo_mapShanmenControlledWeaponSession::IsActive() const
{
	return IsValid()
		&& State == Edemo_mapShanmenControlledWeaponSessionState::Active;
}

bool Fdemo_mapShanmenControlledWeaponSession::IsTerminal() const
{
	return IsValid()
		&& (State == Edemo_mapShanmenControlledWeaponSessionState::Completed
			|| State
				== Edemo_mapShanmenControlledWeaponSessionState::Interrupted);
}

bool Fdemo_mapShanmenControlledWeaponSession::TryIssueControl(
	int64 ExpectedSequence,
	EShanmenControlledWeaponCommandKind Kind,
	const FVector& DesiredDirection,
	FShanmenControlledWeaponCommandReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponCommandReceipt();
	if (!IsActive()
		|| Kind == EShanmenControlledWeaponCommandKind::Recall)
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	if (!Candidate.Execution.TryIssueCommand(
			Candidate.ActionRuntime,
			ExpectedSequence,
			Kind,
			DesiredDirection,
			OutReceipt)
		|| !Candidate.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponCommandReceipt();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::
TryCaptureOrbitDefenseReadiness(
	FShanmenControlledWeaponDefenseReadinessReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponDefenseReadinessReceipt();
	return IsActive()
		&& Execution.TryCaptureOrbitDefenseReadiness(
			ActionRuntime, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponSession::
IsOrbitDefenseReadinessCurrent(
	const FShanmenControlledWeaponDefenseReadinessReceipt& Receipt) const
{
	return IsActive()
		&& Execution.IsOrbitDefenseReadinessCurrent(
			ActionRuntime, Receipt);
}

bool Fdemo_mapShanmenControlledWeaponSession::TryBeginContactWindow(
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	if (!Candidate.Execution.TryBeginEmission(
			Candidate.ActionRuntime, OutContext)
		|| !Candidate.IsValid())
	{
		OutContext = FShanmenWorldHitContext();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryBeginOrbitThreatWindow(
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	if (!Candidate.Execution.TryBeginOrbitThreatEmission(
			Candidate.ActionRuntime, OutContext)
		|| !Candidate.IsValid())
	{
		OutContext = FShanmenWorldHitContext();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryAcceptOrbitThreatCandidate(
	const FShanmenHitCandidate& Candidate)
{
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession SessionCandidate = *this;
	if (!SessionCandidate.Execution.TryAcceptOrbitThreatCandidate(
			SessionCandidate.ActionRuntime, Candidate)
		|| !SessionCandidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(SessionCandidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryEndOrbitThreatWindow(
	FShanmenDetectorEmissionReceipt& OutReceipt)
{
	OutReceipt = FShanmenDetectorEmissionReceipt();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	FShanmenDetectorEmissionReceipt Receipt;
	if (!Candidate.Execution.TryEndOrbitThreatEmission(
			Candidate.ActionRuntime, Receipt)
		|| !Receipt.IsValid()
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Receipt);
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryEndOrbitThreatWindow()
{
	FShanmenDetectorEmissionReceipt Ignored;
	return TryEndOrbitThreatWindow(Ignored);
}

bool Fdemo_mapShanmenControlledWeaponSession::TryEvaluateOrbitThreatReceipt(
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
	FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
	return IsActive()
		&& Execution.TryEvaluateOrbitThreatReceipt(
			ActionRuntime, Emission, TargetEvidence, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponSession::
TryBuildOrbitThreatPresenceIntents(
	const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
	FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPresenceReceipt();
	return IsActive()
		&& Execution.TryBuildOrbitThreatPresenceIntents(
			ActionRuntime, Policy, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponSession::TryResolveCandidate(
	const FShanmenHitCandidate& Candidate,
	const FShanmenTargetVitalitySnapshot& TargetVitality,
	const FShanmenDefenseSnapshot& Defense,
	FShanmenControlledWeaponImpactReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponImpactReceipt();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession SessionCandidate = *this;
	if (!SessionCandidate.Execution.TryResolveCandidate(
			SessionCandidate.ActionRuntime,
			Candidate,
			TargetVitality,
			Defense,
			OutReceipt)
		|| !SessionCandidate.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponImpactReceipt();
		return false;
	}
	*this = MoveTemp(SessionCandidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryEndContactWindow()
{
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	if (!Candidate.Execution.TryEndEmission(Candidate.ActionRuntime)
		|| !Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryRecallAndComplete(
	int64 ExpectedSequence,
	FShanmenControlledWeaponCommandReceipt& OutRecall,
	FShanmenActionTransitionReceipt& OutRecovery,
	FShanmenActionTransitionReceipt& OutCompleted)
{
	OutRecall = FShanmenControlledWeaponCommandReceipt();
	OutRecovery = FShanmenActionTransitionReceipt();
	OutCompleted = FShanmenActionTransitionReceipt();
	if (!IsActive() || Execution.IsEmissionActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	if (!Candidate.Execution.TryIssueCommand(
			Candidate.ActionRuntime,
			ExpectedSequence,
			EShanmenControlledWeaponCommandKind::Recall,
			FVector::ZeroVector,
			OutRecall)
		|| !Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active, OutRecovery)
		|| !Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery, OutCompleted))
	{
		OutRecall = FShanmenControlledWeaponCommandReceipt();
		OutRecovery = FShanmenActionTransitionReceipt();
		OutCompleted = FShanmenActionTransitionReceipt();
		return false;
	}
	Candidate.State =
		Edemo_mapShanmenControlledWeaponSessionState::Completed;
	if (!Candidate.IsValid())
	{
		OutRecall = FShanmenControlledWeaponCommandReceipt();
		OutRecovery = FShanmenActionTransitionReceipt();
		OutCompleted = FShanmenActionTransitionReceipt();
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponSession::TryInterrupt(
	FShanmenActionTransitionReceipt& OutInterrupted)
{
	OutInterrupted = FShanmenActionTransitionReceipt();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Candidate = *this;
	Candidate.Execution.EndEmissionForTermination();
	if (!Candidate.ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active, OutInterrupted))
	{
		OutInterrupted = FShanmenActionTransitionReceipt();
		return false;
	}
	Candidate.State =
		Edemo_mapShanmenControlledWeaponSessionState::Interrupted;
	if (!Candidate.IsValid())
	{
		OutInterrupted = FShanmenActionTransitionReceipt();
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

void Fdemo_mapShanmenControlledWeaponSession::Reset()
{
	*this = Fdemo_mapShanmenControlledWeaponSession();
}
