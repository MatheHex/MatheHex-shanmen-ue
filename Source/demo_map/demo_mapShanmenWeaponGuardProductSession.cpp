#include "demo_mapShanmenWeaponGuardProductSession.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapShanmenWeaponGuardItemAdapter.h"

namespace
{
	bool MatchesRouteIdentity(
		const Fdemo_mapShanmenWeaponGuardProductRouteResult& Route,
		const FGuid& HostId,
		const FGuid& SourceItemInstanceId)
	{
		return Route.IsReady()
			&& HostId.IsValid()
			&& SourceItemInstanceId.IsValid()
			&& Route.ProductStart.Host.GetHostId() == HostId
			&& Route.ItemAuthorization.Authorization
				.GetSourceItemInstanceId() == SourceItemInstanceId
			&& Route.ProductStart.Reservation
				.GetSourceItemInstanceId() == SourceItemInstanceId;
	}

	Fdemo_mapShanmenWeaponGuardSessionStartResult RejectStart(
		Edemo_mapShanmenWeaponGuardSessionStartError Error,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenWeaponGuardSessionStartResult Result;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenWeaponGuardSessionTransitionResult RejectTransition(
		Edemo_mapShanmenWeaponGuardSessionTransitionError Error,
		Edemo_mapShanmenWeaponGuardTerminationReason Reason,
		const FGuid& HostId,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
		Result.Error = Error;
		Result.Reason = Reason;
		Result.HostId = HostId;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	bool IsValidTerminationReason(
		Edemo_mapShanmenWeaponGuardTerminationReason Reason)
	{
		switch (Reason)
		{
		case Edemo_mapShanmenWeaponGuardTerminationReason::InputReleased:
		case Edemo_mapShanmenWeaponGuardTerminationReason::
			EffectiveDamageStagger:
		case Edemo_mapShanmenWeaponGuardTerminationReason::
			WeaponAuthorizationChanged:
		case Edemo_mapShanmenWeaponGuardTerminationReason::PlayerDefeated:
		case Edemo_mapShanmenWeaponGuardTerminationReason::PawnUnpossessed:
		case Edemo_mapShanmenWeaponGuardTerminationReason::ControllerEndPlay:
		case Edemo_mapShanmenWeaponGuardTerminationReason::RunTeardown:
			return true;
		default:
			return false;
		}
	}

	Fdemo_mapShanmenWeaponGuardSessionDefenseResult RejectDefense(
		Edemo_mapShanmenWeaponGuardSessionDefenseError Error,
		const TCHAR* Diagnostic,
		const FGuid& HostId = FGuid(),
		const FGuid& SourceItemInstanceId = FGuid(),
		const FGuid& TimelineId = FGuid(),
		int64 ObservedTick = INDEX_NONE,
		const Fdemo_mapShanmenWeaponGuardHostDefenseResult& Defense =
			Fdemo_mapShanmenWeaponGuardHostDefenseResult())
	{
		Fdemo_mapShanmenWeaponGuardSessionDefenseResult Result;
		Result.Error = Error;
		Result.HostId = HostId;
		Result.SourceItemInstanceId = SourceItemInstanceId;
		Result.TimelineId = TimelineId;
		Result.ObservedTick = ObservedTick;
		Result.Defense = Defense;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenWeaponGuardSessionStartResult::IsValid() const
{
	switch (Status)
	{
	case Edemo_mapShanmenWeaponGuardSessionStartStatus::Rejected:
		return Error
				!= Edemo_mapShanmenWeaponGuardSessionStartError::None
			&& Error
				!= Edemo_mapShanmenWeaponGuardSessionStartError::AlreadyActive
			&& !HostId.IsValid()
			&& !SourceItemInstanceId.IsValid()
			&& !Route.IsReady();
	case Edemo_mapShanmenWeaponGuardSessionStartStatus::Started:
		return Error == Edemo_mapShanmenWeaponGuardSessionStartError::None
			&& MatchesRouteIdentity(
				Route, HostId, SourceItemInstanceId);
	case Edemo_mapShanmenWeaponGuardSessionStartStatus::AlreadyActive:
		return Error
				== Edemo_mapShanmenWeaponGuardSessionStartError::AlreadyActive
			&& MatchesRouteIdentity(
				Route, HostId, SourceItemInstanceId);
	default:
		return false;
	}
}

bool Fdemo_mapShanmenWeaponGuardSessionStartResult::IsStarted() const
{
	return IsValid()
		&& Status == Edemo_mapShanmenWeaponGuardSessionStartStatus::Started;
}

bool Fdemo_mapShanmenWeaponGuardSessionStartResult::IsAlreadyActive() const
{
	return IsValid()
		&& Status
			== Edemo_mapShanmenWeaponGuardSessionStartStatus::AlreadyActive;
}

bool Fdemo_mapShanmenWeaponGuardSessionTransitionResult::IsValid() const
{
	switch (Status)
	{
	case Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Rejected:
		return Error
			!= Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
	case Edemo_mapShanmenWeaponGuardSessionTransitionStatus::NoActiveHost:
		return Error
				== Edemo_mapShanmenWeaponGuardSessionTransitionError::None
			&& IsValidTerminationReason(Reason)
			&& !HostId.IsValid();
	case Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Completed:
		return Error
				== Edemo_mapShanmenWeaponGuardSessionTransitionError::None
			&& Reason
				== Edemo_mapShanmenWeaponGuardTerminationReason::InputReleased
			&& HostId.IsValid()
			&& Recovery.IsSuccess()
			&& Recovery.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::
					EnteredRecovery
			&& Terminal.IsSuccess()
			&& Terminal.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::Completed
			&& Recovery.HostId == HostId
			&& Terminal.HostId == HostId;
	case Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Interrupted:
		return Error
				== Edemo_mapShanmenWeaponGuardSessionTransitionError::None
			&& IsValidTerminationReason(Reason)
			&& Reason
				!= Edemo_mapShanmenWeaponGuardTerminationReason::InputReleased
			&& HostId.IsValid()
			&& Terminal.IsSuccess()
			&& Terminal.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::Interrupted
			&& Terminal.HostId == HostId;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenWeaponGuardSessionTransitionResult::IsSuccess() const
{
	return IsValid()
		&& Status
			!= Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Rejected;
}

bool Fdemo_mapShanmenWeaponGuardSessionTransitionResult::IsNoOp() const
{
	return IsValid()
		&& Status
			== Edemo_mapShanmenWeaponGuardSessionTransitionStatus::NoActiveHost;
}

bool Fdemo_mapShanmenWeaponGuardSessionDefenseResult::IsValid() const
{
	if (Status == Edemo_mapShanmenWeaponGuardSessionDefenseStatus::Rejected)
	{
		return Error != Edemo_mapShanmenWeaponGuardSessionDefenseError::None
			&& !Defense.IsSuccess();
	}
	if (Error != Edemo_mapShanmenWeaponGuardSessionDefenseError::None
		|| !HostId.IsValid()
		|| !SourceItemInstanceId.IsValid()
		|| !TimelineId.IsValid()
		|| ObservedTick < 0
		|| !Defense.IsSuccess()
		|| Defense.HostId != HostId
		|| Defense.Observation.GetTimelineId() != TimelineId
		|| Defense.Observation.GetObservedTick() != ObservedTick)
	{
		return false;
	}
	const bool bQualified = Status
		== Edemo_mapShanmenWeaponGuardSessionDefenseStatus::ComposedQualified;
	return (bQualified
			|| Status
				== Edemo_mapShanmenWeaponGuardSessionDefenseStatus::
					ComposedOutsideArc)
		&& bQualified == Defense.HasGuardLayer();
}

bool Fdemo_mapShanmenWeaponGuardSessionDefenseResult::IsSuccess() const
{
	return IsValid()
		&& Status
			!= Edemo_mapShanmenWeaponGuardSessionDefenseStatus::Rejected;
}

bool Fdemo_mapShanmenWeaponGuardSessionDefenseResult::HasGuardLayer() const
{
	return IsSuccess()
		&& Status
			== Edemo_mapShanmenWeaponGuardSessionDefenseStatus::
				ComposedQualified;
}

Fdemo_mapShanmenWeaponGuardSessionStartResult
Fdemo_mapShanmenWeaponGuardProductSession::TryStart(
	const Fdemo_mapItemAuthority* ItemAuthority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FGuid& TimelineId,
	int64 ActiveStartTick)
{
	if (!IsValid())
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardSessionStartError::
				StateDesynchronized,
			TEXT("Weapon-guard Session is structurally invalid."));
	}
	if (bHasActiveRoute)
	{
		Fdemo_mapShanmenWeaponGuardSessionStartResult Result;
		Result.Status =
			Edemo_mapShanmenWeaponGuardSessionStartStatus::AlreadyActive;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionStartError::AlreadyActive;
		Result.Route = ActiveRoute;
		Result.HostId = ActiveRoute.ProductStart.Host.GetHostId();
		Result.SourceItemInstanceId = ActiveRoute.ItemAuthorization
			.Authorization.GetSourceItemInstanceId();
		Result.Diagnostic =
			TEXT("Weapon guard already owns the sole active product Host.");
		return Result;
	}
	if (!ItemAuthority)
	{
		return RejectStart(
			Edemo_mapShanmenWeaponGuardSessionStartError::
				ItemAuthorityUnavailable,
			TEXT("Weapon-guard Session requires the existing item authority."));
	}

	Fdemo_mapShanmenWeaponGuardProductRouteResult Route =
		Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
			*ItemAuthority,
			Coordinator,
			TimelineId,
			ActiveStartTick);
	if (!Route.IsReady())
	{
		if (Route.ProductStart.IsReady())
		{
			const Fdemo_mapShanmenWeaponGuardHostTransitionResult Cleanup =
				Route.ProductStart.Host.TryInterrupt();
			if (!Cleanup.IsSuccess())
			{
				return RejectStart(
					Edemo_mapShanmenWeaponGuardSessionStartError::
						StateDesynchronized,
					TEXT("Rejected product route could not terminate its local Host proof."));
			}
		}
		Fdemo_mapShanmenWeaponGuardSessionStartResult Result = RejectStart(
			Edemo_mapShanmenWeaponGuardSessionStartError::RouteRejected,
			Route.Diagnostic.IsEmpty()
				? TEXT("Weapon-guard product route rejected Session start.")
				: *Route.Diagnostic);
		Result.Route = MoveTemp(Route);
		return Result;
	}

	ActiveRoute = MoveTemp(Route);
	bHasActiveRoute = true;
	if (!IsValid())
	{
		ActiveRoute.ProductStart.Host.TryInterrupt();
		Clear();
		return RejectStart(
			Edemo_mapShanmenWeaponGuardSessionStartError::
				StateDesynchronized,
			TEXT("Weapon-guard Session rejected a desynchronized active route."));
	}

	Fdemo_mapShanmenWeaponGuardSessionStartResult Result;
	Result.Status = Edemo_mapShanmenWeaponGuardSessionStartStatus::Started;
	Result.Error = Edemo_mapShanmenWeaponGuardSessionStartError::None;
	Result.Route = ActiveRoute;
	Result.HostId = ActiveRoute.ProductStart.Host.GetHostId();
	Result.SourceItemInstanceId = ActiveRoute.ItemAuthorization.Authorization
		.GetSourceItemInstanceId();
	Result.Diagnostic =
		TEXT("Weapon-guard Session acquired the sole active product Host.");
	return Result;
}

Fdemo_mapShanmenWeaponGuardSessionTransitionResult
Fdemo_mapShanmenWeaponGuardProductSession::TryTerminate(
	Edemo_mapShanmenWeaponGuardTerminationReason Reason)
{
	if (!IsValidTerminationReason(Reason))
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				InvalidTerminationReason,
			Reason,
			FGuid(),
			TEXT("Weapon-guard termination requires a typed reason."));
	}
	if (!IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::SessionInvalid,
			Reason,
			FGuid(),
			TEXT("Weapon-guard termination requires a valid Session."));
	}
	if (!bHasActiveRoute)
	{
		Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
		Result.Status =
			Edemo_mapShanmenWeaponGuardSessionTransitionStatus::NoActiveHost;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
		Result.Reason = Reason;
		Result.Diagnostic =
			TEXT("Weapon-guard termination found no active Host.");
		return Result;
	}

	Fdemo_mapShanmenWeaponGuardProductRouteResult Candidate = ActiveRoute;
	const FGuid HostId = Candidate.ProductStart.Host.GetHostId();
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
	Result.Reason = Reason;
	Result.HostId = HostId;
	if (Reason
		== Edemo_mapShanmenWeaponGuardTerminationReason::InputReleased)
	{
		Result.Recovery = Candidate.ProductStart.Host.TryEnterRecovery();
		if (!Result.Recovery.IsSuccess())
		{
			return RejectTransition(
				Edemo_mapShanmenWeaponGuardSessionTransitionError::
					RecoveryRejected,
				Reason,
				HostId,
				TEXT("Weapon-guard Host rejected Active-to-Recovery release."));
		}
		Result.Terminal = Candidate.ProductStart.Host.TryComplete();
		if (!Result.Terminal.IsSuccess())
		{
			return RejectTransition(
				Edemo_mapShanmenWeaponGuardSessionTransitionError::
					CompletionRejected,
				Reason,
				HostId,
				TEXT("Weapon-guard Host rejected Recovery-to-Completed release."));
		}
		Result.Status =
			Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Completed;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
		Result.Diagnostic =
			TEXT("Weapon-guard Host completed ordered input release and Session retired it.");
	}
	else
	{
		Result.Terminal = Candidate.ProductStart.Host.TryInterrupt();
		if (!Result.Terminal.IsSuccess())
		{
			return RejectTransition(
				Edemo_mapShanmenWeaponGuardSessionTransitionError::
					InterruptRejected,
				Reason,
				HostId,
				TEXT("Weapon-guard Host rejected typed interruption."));
		}
		Result.Status =
			Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Interrupted;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
		Result.Diagnostic =
			TEXT("Weapon-guard Host accepted typed interruption and Session retired it.");
	}
	if (!Candidate.ProductStart.Host.IsTerminal() || !Result.IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				StateDesynchronized,
			Reason,
			HostId,
			TEXT("Weapon-guard typed termination proof failed closed."));
	}
	Clear();
	return Result;
}

Fdemo_mapShanmenWeaponGuardSessionDefenseResult
Fdemo_mapShanmenWeaponGuardProductSession::TryComposeImpactDefense(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* DefenderActor,
	AActor* ThreatActor,
	const FGuid& TimelineId,
	int64 ObservedTick,
	const FShanmenHitCandidate& Candidate,
	const FShanmenDefenseSnapshot& BaseDefense)
{
	if (!IsValid())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::SessionInvalid,
			TEXT("Weapon-guard impact defense requires a valid Session."));
	}
	if (!bHasActiveRoute)
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::NoActiveHost,
			TEXT("Weapon-guard impact defense requires one active Host."));
	}

	const Fdemo_mapShanmenWeaponGuardProductHost& ActiveHost =
		ActiveRoute.ProductStart.Host;
	const FGuid HostId = ActiveHost.GetHostId();
	const FGuid SourceItemInstanceId = ActiveRoute.ItemAuthorization
		.Authorization.GetSourceItemInstanceId();
	if (!TimelineId.IsValid() || ObservedTick < 0)
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::
				InvalidTimelineSample,
			TEXT("Weapon-guard impact defense requires a valid timeline sample."),
			HostId,
			SourceItemInstanceId,
			TimelineId,
			ObservedTick);
	}
	if (ActiveHost.GetTimingPolicy().GetTimelineId() != TimelineId)
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::TimelineMismatch,
			TEXT("Weapon-guard impact sample does not belong to the active Host timeline."),
			HostId,
			SourceItemInstanceId,
			TimelineId,
			ObservedTick);
	}

	Fdemo_mapShanmenWeaponGuardProductRouteResult CandidateRoute =
		ActiveRoute;
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult HostDefense =
		CandidateRoute.ProductStart.Host.TryComposeDefense(
			World,
			EntityRegistry,
			DefenderActor,
			ThreatActor,
			ObservedTick,
			Candidate,
			BaseDefense);
	if (!HostDefense.IsSuccess())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::HostRejected,
			TEXT("Active weapon-guard Host rejected impact defense composition."),
			HostId,
			SourceItemInstanceId,
			TimelineId,
			ObservedTick,
			HostDefense);
	}

	Fdemo_mapShanmenWeaponGuardSessionDefenseResult Result;
	Result.Status = HostDefense.HasGuardLayer()
		? Edemo_mapShanmenWeaponGuardSessionDefenseStatus::ComposedQualified
		: Edemo_mapShanmenWeaponGuardSessionDefenseStatus::ComposedOutsideArc;
	Result.Error = Edemo_mapShanmenWeaponGuardSessionDefenseError::None;
	Result.HostId = HostId;
	Result.SourceItemInstanceId = SourceItemInstanceId;
	Result.TimelineId = TimelineId;
	Result.ObservedTick = ObservedTick;
	Result.Defense = HostDefense;
	Result.Diagnostic = HostDefense.HasGuardLayer()
		? TEXT("Active weapon guard composed a qualified defense layer.")
		: TEXT("Active weapon guard inspected an outside-arc impact without adding a layer.");
	if (!CandidateRoute.IsReady()
		|| !CandidateRoute.ProductStart.Host.IsActive()
		|| !Result.IsValid())
	{
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::
				StateDesynchronized,
			TEXT("Weapon-guard impact defense proof failed closed."),
			HostId,
			SourceItemInstanceId,
			TimelineId,
			ObservedTick,
			HostDefense);
	}

	const Fdemo_mapShanmenWeaponGuardProductRouteResult PreviousRoute =
		ActiveRoute;
	ActiveRoute = MoveTemp(CandidateRoute);
	if (!IsValid())
	{
		ActiveRoute = PreviousRoute;
		return RejectDefense(
			Edemo_mapShanmenWeaponGuardSessionDefenseError::
				StateDesynchronized,
			TEXT("Weapon-guard Session rejected the advanced Host state."),
			HostId,
			SourceItemInstanceId,
			TimelineId,
			ObservedTick,
			HostDefense);
	}
	return Result;
}

bool Fdemo_mapShanmenWeaponGuardProductSession::IsValid() const
{
	if (!bHasActiveRoute)
	{
		return !ActiveRoute.IsReady()
			&& !ActiveRoute.ProductStart.Host.IsValid();
	}
	return ActiveRoute.IsReady()
		&& ActiveRoute.ProductStart.Host.IsActive();
}

bool Fdemo_mapShanmenWeaponGuardProductSession::IsEmpty() const
{
	return IsValid() && !bHasActiveRoute;
}

bool Fdemo_mapShanmenWeaponGuardProductSession::HasActive() const
{
	return IsValid() && bHasActiveRoute;
}

bool Fdemo_mapShanmenWeaponGuardProductSession::IsCurrentAuthorization(
	const Fdemo_mapItemAuthority& ItemAuthority) const
{
	return HasActive()
		&& Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			ItemAuthority,
			ActiveRoute.ItemAuthorization.Authorization);
}

const Fdemo_mapShanmenWeaponGuardProductRouteResult*
Fdemo_mapShanmenWeaponGuardProductSession::GetActiveRoute() const
{
	return HasActive() ? &ActiveRoute : nullptr;
}

const Fdemo_mapShanmenWeaponGuardProductHost*
Fdemo_mapShanmenWeaponGuardProductSession::GetActiveHost() const
{
	const Fdemo_mapShanmenWeaponGuardProductRouteResult* Route =
		GetActiveRoute();
	return Route ? &Route->ProductStart.Host : nullptr;
}

void Fdemo_mapShanmenWeaponGuardProductSession::Clear()
{
	bHasActiveRoute = false;
	ActiveRoute = Fdemo_mapShanmenWeaponGuardProductRouteResult();
}
