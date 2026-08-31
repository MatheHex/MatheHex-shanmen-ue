#include "demo_mapShanmenWeaponGuardProductHost.h"

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

	bool WindowsMatch(
		const FShanmenWeaponGuardWindowReceipt& Left,
		const FShanmenWeaponGuardWindowReceipt& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetReceiptId() == Right.GetReceiptId()
			&& Left.GetWindowId() == Right.GetWindowId();
	}

	FGuid MakeHostId(
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenWeaponPerfectGuardPolicy& TimingPolicy,
		const FShanmenWeaponGuardArcPolicy& ArcPolicy)
	{
		if (!Window.IsValid()
			|| !TimingPolicy.IsValid()
			|| !ArcPolicy.IsValid()
			|| !WindowsMatch(
				Window.GetOpenReceipt(), TimingPolicy.GetWindow())
			|| !WindowsMatch(
				Window.GetOpenReceipt(), ArcPolicy.GetWindow()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Sword.WeaponGuard.ProductHost.r1"),
			{
				GuidDigits(Window.GetOpenReceipt().GetReceiptId()),
				GuidDigits(TimingPolicy.GetPolicyId()),
				GuidDigits(ArcPolicy.GetPolicyId())
			});
	}

	FGuid MakeDefenseReceiptId(
		const FGuid& HostId,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		const Fdemo_mapShanmenWeaponGuardDefenseResult& Composition)
	{
		if (!HostId.IsValid()
			|| !Observation.IsValid()
			|| !Composition.IsSuccess())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Sword.WeaponGuard.ProductHost.Defense.r1"),
			{
				GuidDigits(HostId),
				GuidDigits(Observation.GetObservationId()),
				GuidDigits(Composition.ReceiptId)
			});
	}

	Fdemo_mapShanmenWeaponGuardHostStartResult RejectStart(
		Edemo_mapShanmenWeaponGuardHostStartError Error)
	{
		Fdemo_mapShanmenWeaponGuardHostStartResult Result;
		Result.Error = Error;
		return Result;
	}

	Fdemo_mapShanmenWeaponGuardHostDefenseResult RejectDefense(
		Edemo_mapShanmenWeaponGuardHostDefenseError Error,
		const FGuid& HostId = FGuid(),
		const FShanmenWeaponGuardTimelineObservation& Observation =
			FShanmenWeaponGuardTimelineObservation(),
		const Fdemo_mapShanmenWeaponGuardDefenseResult& Composition =
			Fdemo_mapShanmenWeaponGuardDefenseResult())
	{
		Fdemo_mapShanmenWeaponGuardHostDefenseResult Result;
		Result.Error = Error;
		Result.HostId = HostId;
		Result.Observation = Observation;
		Result.Composition = Composition;
		return Result;
	}

	Fdemo_mapShanmenWeaponGuardHostTransitionResult RejectTransition(
		Edemo_mapShanmenWeaponGuardHostTransitionError Error,
		const FGuid& HostId = FGuid())
	{
		Fdemo_mapShanmenWeaponGuardHostTransitionResult Result;
		Result.Error = Error;
		Result.HostId = HostId;
		return Result;
	}
}

bool Fdemo_mapShanmenWeaponGuardHostStartResult::IsValid() const
{
	if (Status == Edemo_mapShanmenWeaponGuardHostStartStatus::Rejected)
	{
		return Error != Edemo_mapShanmenWeaponGuardHostStartError::None
			&& !HostId.IsValid();
	}
	return Status == Edemo_mapShanmenWeaponGuardHostStartStatus::Started
		&& Error == Edemo_mapShanmenWeaponGuardHostStartError::None
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
		&& TimingPolicy.IsValid()
		&& ArcPolicy.IsValid()
		&& WindowsMatch(Window, TimingPolicy.GetWindow())
		&& WindowsMatch(Window, ArcPolicy.GetWindow());
}

bool Fdemo_mapShanmenWeaponGuardHostStartResult::IsSuccess() const
{
	return IsValid()
		&& Status == Edemo_mapShanmenWeaponGuardHostStartStatus::Started;
}

bool Fdemo_mapShanmenWeaponGuardHostDefenseResult::IsValid() const
{
	if (Status == Edemo_mapShanmenWeaponGuardHostDefenseStatus::Rejected)
	{
		return Error != Edemo_mapShanmenWeaponGuardHostDefenseError::None
			&& !ReceiptId.IsValid();
	}
	if (Error != Edemo_mapShanmenWeaponGuardHostDefenseError::None
		|| !HostId.IsValid()
		|| !ReceiptId.IsValid()
		|| !Observation.IsValid()
		|| !Composition.IsSuccess())
	{
		return false;
	}
	const bool bQualified =
		Status
			== Edemo_mapShanmenWeaponGuardHostDefenseStatus::
			ComposedQualified;
	if (!bQualified
		&& Status
			!= Edemo_mapShanmenWeaponGuardHostDefenseStatus::
			ComposedOutsideArc)
	{
		return false;
	}
	return bQualified == Composition.HasGuardLayer()
		&& ReceiptId
			== MakeDefenseReceiptId(HostId, Observation, Composition);
}

bool Fdemo_mapShanmenWeaponGuardHostDefenseResult::IsSuccess() const
{
	return IsValid()
		&& Status != Edemo_mapShanmenWeaponGuardHostDefenseStatus::Rejected;
}

bool Fdemo_mapShanmenWeaponGuardHostTransitionResult::IsValid() const
{
	if (Status == Edemo_mapShanmenWeaponGuardHostTransitionStatus::Rejected)
	{
		return Error
			!= Edemo_mapShanmenWeaponGuardHostTransitionError::None
			&& !Transition.IsValid();
	}
	if (Error != Edemo_mapShanmenWeaponGuardHostTransitionError::None
		|| !HostId.IsValid())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenWeaponGuardHostTransitionStatus::AlreadyTerminal)
	{
		return !Transition.IsValid();
	}
	if (!Transition.IsValid())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenWeaponGuardHostTransitionStatus::EnteredRecovery:
		return Transition.GetFromPhase() == EShanmenCombatActionPhase::Active
			&& Transition.GetToPhase()
				== EShanmenCombatActionPhase::Recovery
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::None;
	case Edemo_mapShanmenWeaponGuardHostTransitionStatus::Completed:
		return Transition.GetFromPhase()
				== EShanmenCombatActionPhase::Recovery
			&& Transition.GetToPhase() == EShanmenCombatActionPhase::Idle
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::Completed;
	case Edemo_mapShanmenWeaponGuardHostTransitionStatus::Interrupted:
		return (Transition.GetFromPhase()
				== EShanmenCombatActionPhase::Active
				|| Transition.GetFromPhase()
					== EShanmenCombatActionPhase::Recovery)
			&& Transition.GetToPhase()
				== EShanmenCombatActionPhase::Interrupted
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenWeaponGuardHostTransitionResult::IsSuccess() const
{
	return IsValid()
		&& Status
			!= Edemo_mapShanmenWeaponGuardHostTransitionStatus::Rejected;
}

Fdemo_mapShanmenWeaponGuardHostStartResult
Fdemo_mapShanmenWeaponGuardProductHost::TryStart(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenWeaponGuardDefinition& Definition,
	const FGuid& TimelineId,
	int64 ActiveStartTick,
	int64 PerfectEndTick,
	FName PerfectRuleId,
	FName ArcRuleId,
	double MinimumFacingDot,
	Fdemo_mapShanmenWeaponGuardProductHost& OutHost)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenWeaponGuardDefinition FrozenDefinition = Definition;
	OutHost = Fdemo_mapShanmenWeaponGuardProductHost();
	if (!FrozenAction.IsValid()
		|| !FrozenDefinition.IsValid()
		|| FrozenAction.GetActionDefinitionId()
			!= FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId()
		|| FrozenDefinition.GetActionDefinitionId()
			!= FrozenAction.GetActionDefinitionId()
		|| !TimelineId.IsValid()
		|| ActiveStartTick < 0
		|| PerfectEndTick <= ActiveStartTick
		|| PerfectRuleId.IsNone()
		|| ArcRuleId.IsNone()
		|| !FMath::IsFinite(MinimumFacingDot)
		|| MinimumFacingDot < -1.0
		|| MinimumFacingDot > 1.0)
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::InvalidInput);
	}

	Fdemo_mapShanmenWeaponGuardProductHost Candidate;
	Fdemo_mapShanmenWeaponGuardHostStartResult Result;
	if (!FShanmenActionOrchestrator::TryStart(
			FrozenAction, Candidate.ActionRuntime, Result.Startup))
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::ActionStartRejected);
	}
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Result.ActiveCommit))
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::ActionCommitRejected);
	}
	if (!FShanmenWeaponGuardWindow::TryOpen(
			FrozenAction,
			FrozenDefinition,
			Result.ActiveCommit,
			Candidate.ActionRuntime,
			Candidate.Window,
			Result.Window))
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::WindowRejected);
	}
	if (!FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Result.Window,
			TimelineId,
			ActiveStartTick,
			PerfectEndTick,
			PerfectRuleId,
			Candidate.TimingPolicy))
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::
			TimingPolicyRejected);
	}
	if (!FShanmenWeaponGuardArcPolicy::TryCapture(
			Result.Window,
			ArcRuleId,
			MinimumFacingDot,
			Candidate.ArcPolicy))
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::ArcPolicyRejected);
	}

	Candidate.HostId = MakeHostId(
		Candidate.Window, Candidate.TimingPolicy, Candidate.ArcPolicy);
	Candidate.LastObservedTick = ActiveStartTick;
	Candidate.bStarted = true;
	if (!Candidate.IsValid())
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::StateDesynchronized);
	}

	Result.Status = Edemo_mapShanmenWeaponGuardHostStartStatus::Started;
	Result.Error = Edemo_mapShanmenWeaponGuardHostStartError::None;
	Result.HostId = Candidate.HostId;
	Result.TimingPolicy = Candidate.TimingPolicy;
	Result.ArcPolicy = Candidate.ArcPolicy;
	if (!Result.IsValid())
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardHostStartError::StateDesynchronized);
	}
	OutHost = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenWeaponGuardProductHost::IsValid() const
{
	if (!bStarted
		|| !HostId.IsValid()
		|| LastObservedTick < 0
		|| !ActionRuntime.IsValid()
		|| !Window.IsValid()
		|| !TimingPolicy.IsValid()
		|| !ArcPolicy.IsValid()
		|| LastObservedTick < TimingPolicy.GetActiveStartTick()
		|| HostId != MakeHostId(Window, TimingPolicy, ArcPolicy)
		|| !ActionsMatch(
			ActionRuntime.GetAction(), Window.GetOpenReceipt().GetAction()))
	{
		return false;
	}

	const EShanmenCombatActionPhase Phase = ActionRuntime.GetPhase();
	const EShanmenActionTerminalReason Reason =
		ActionRuntime.GetTerminalReason();
	if (Phase == EShanmenCombatActionPhase::Active)
	{
		return Reason == EShanmenActionTerminalReason::None
			&& Window.IsActiveFor(ActionRuntime);
	}
	if (Phase == EShanmenCombatActionPhase::Recovery)
	{
		return Reason == EShanmenActionTerminalReason::None
			&& !Window.IsActiveFor(ActionRuntime);
	}
	if (Phase == EShanmenCombatActionPhase::Idle)
	{
		return Reason == EShanmenActionTerminalReason::Completed
			&& !Window.IsActiveFor(ActionRuntime);
	}
	return Phase == EShanmenCombatActionPhase::Interrupted
		&& Reason == EShanmenActionTerminalReason::Interrupted
		&& !Window.IsActiveFor(ActionRuntime);
}

bool Fdemo_mapShanmenWeaponGuardProductHost::IsActive() const
{
	return IsValid()
		&& ActionRuntime.GetPhase() == EShanmenCombatActionPhase::Active
		&& !ActionRuntime.IsTerminal();
}

bool Fdemo_mapShanmenWeaponGuardProductHost::IsRecovery() const
{
	return IsValid()
		&& ActionRuntime.GetPhase() == EShanmenCombatActionPhase::Recovery
		&& !ActionRuntime.IsTerminal();
}

bool Fdemo_mapShanmenWeaponGuardProductHost::IsTerminal() const
{
	return IsValid() && ActionRuntime.IsTerminal();
}

Fdemo_mapShanmenWeaponGuardHostDefenseResult
Fdemo_mapShanmenWeaponGuardProductHost::TryComposeDefense(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* DefenderActor,
	AActor* ThreatActor,
	int64 ObservedTick,
	const FShanmenHitCandidate& Candidate,
	const FShanmenDefenseSnapshot& BaseDefense)
{
	if (!IsValid())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardHostDefenseError::HostNotReady);
	}
	if (!IsActive())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardHostDefenseError::HostNotActive,
			HostId);
	}
	if (ObservedTick < LastObservedTick)
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardHostDefenseError::
			NonMonotonicObservation,
			HostId);
	}

	FShanmenWeaponGuardTimelineObservation Observation;
	if (!FShanmenWeaponGuardTimelineObservation::TryCapture(
			TimingPolicy.GetTimelineId(), ObservedTick, Observation))
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardHostDefenseError::ObservationRejected,
			HostId);
	}
	const Fdemo_mapShanmenWeaponGuardDefenseResult Composition =
		Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose(
			World,
			EntityRegistry,
			DefenderActor,
			ThreatActor,
			Window,
			ActionRuntime,
			TimingPolicy,
			Observation,
			ArcPolicy,
			Candidate,
			BaseDefense);
	if (!Composition.IsSuccess())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardHostDefenseError::CompositionRejected,
			HostId,
			Observation,
			Composition);
	}

	Fdemo_mapShanmenWeaponGuardProductHost HostCandidate = *this;
	HostCandidate.LastObservedTick = ObservedTick;
	Fdemo_mapShanmenWeaponGuardHostDefenseResult Result;
	Result.Status = Composition.HasGuardLayer()
		? Edemo_mapShanmenWeaponGuardHostDefenseStatus::ComposedQualified
		: Edemo_mapShanmenWeaponGuardHostDefenseStatus::ComposedOutsideArc;
	Result.Error = Edemo_mapShanmenWeaponGuardHostDefenseError::None;
	Result.HostId = HostId;
	Result.Observation = Observation;
	Result.Composition = Composition;
	Result.ReceiptId = MakeDefenseReceiptId(
		Result.HostId, Result.Observation, Result.Composition);
	if (!HostCandidate.IsValid() || !Result.IsValid())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardHostDefenseError::StateDesynchronized,
			HostId,
			Observation,
			Composition);
	}
	*this = MoveTemp(HostCandidate);
	return Result;
}

Fdemo_mapShanmenWeaponGuardHostTransitionResult
Fdemo_mapShanmenWeaponGuardProductHost::TryEnterRecovery()
{
	if (!IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotReady);
	}
	if (IsTerminal() || !IsActive())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotActive,
			HostId);
	}

	Fdemo_mapShanmenWeaponGuardProductHost Candidate = *this;
	Fdemo_mapShanmenWeaponGuardHostTransitionResult Result;
	Result.Status =
		Edemo_mapShanmenWeaponGuardHostTransitionStatus::EnteredRecovery;
	Result.Error = Edemo_mapShanmenWeaponGuardHostTransitionError::None;
	Result.HostId = HostId;
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active, Result.Transition))
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::
			ActionTransitionRejected,
			HostId);
	}
	if (!Candidate.IsValid() || !Candidate.IsRecovery() || !Result.IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::
			StateDesynchronized,
			HostId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenWeaponGuardHostTransitionResult
Fdemo_mapShanmenWeaponGuardProductHost::TryComplete()
{
	if (!IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotReady);
	}
	if (IsTerminal())
	{
		Fdemo_mapShanmenWeaponGuardHostTransitionResult Result;
		Result.Status =
			Edemo_mapShanmenWeaponGuardHostTransitionStatus::AlreadyTerminal;
		Result.Error = Edemo_mapShanmenWeaponGuardHostTransitionError::None;
		Result.HostId = HostId;
		return Result;
	}
	if (!IsRecovery())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotActive,
			HostId);
	}

	Fdemo_mapShanmenWeaponGuardProductHost Candidate = *this;
	Fdemo_mapShanmenWeaponGuardHostTransitionResult Result;
	Result.Status = Edemo_mapShanmenWeaponGuardHostTransitionStatus::Completed;
	Result.Error = Edemo_mapShanmenWeaponGuardHostTransitionError::None;
	Result.HostId = HostId;
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery, Result.Transition))
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::
			ActionTransitionRejected,
			HostId);
	}
	if (!Candidate.IsValid() || !Candidate.IsTerminal() || !Result.IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::
			StateDesynchronized,
			HostId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenWeaponGuardHostTransitionResult
Fdemo_mapShanmenWeaponGuardProductHost::TryInterrupt()
{
	if (!IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotReady);
	}
	if (IsTerminal())
	{
		Fdemo_mapShanmenWeaponGuardHostTransitionResult Result;
		Result.Status =
			Edemo_mapShanmenWeaponGuardHostTransitionStatus::AlreadyTerminal;
		Result.Error = Edemo_mapShanmenWeaponGuardHostTransitionError::None;
		Result.HostId = HostId;
		return Result;
	}
	if (!IsActive() && !IsRecovery())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotActive,
			HostId);
	}

	Fdemo_mapShanmenWeaponGuardProductHost Candidate = *this;
	Fdemo_mapShanmenWeaponGuardHostTransitionResult Result;
	Result.Status =
		Edemo_mapShanmenWeaponGuardHostTransitionStatus::Interrupted;
	Result.Error = Edemo_mapShanmenWeaponGuardHostTransitionError::None;
	Result.HostId = HostId;
	const EShanmenCombatActionPhase Phase =
		Candidate.ActionRuntime.GetPhase();
	if (!Candidate.ActionRuntime.TryInterrupt(Phase, Result.Transition))
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::
			ActionTransitionRejected,
			HostId);
	}
	if (!Candidate.IsValid() || !Candidate.IsTerminal() || !Result.IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardHostTransitionError::
			StateDesynchronized,
			HostId);
	}
	*this = MoveTemp(Candidate);
	return Result;
}
