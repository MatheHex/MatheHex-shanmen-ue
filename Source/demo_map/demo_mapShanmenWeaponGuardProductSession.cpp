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
		const FGuid& HostId,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
		Result.Error = Error;
		Result.HostId = HostId;
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
			&& !HostId.IsValid();
	case Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Completed:
		return Error
				== Edemo_mapShanmenWeaponGuardSessionTransitionError::None
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
Fdemo_mapShanmenWeaponGuardProductSession::TryRelease()
{
	if (!IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::SessionInvalid,
			FGuid(),
			TEXT("Weapon-guard release requires a valid Session."));
	}
	if (!bHasActiveRoute)
	{
		Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
		Result.Status =
			Edemo_mapShanmenWeaponGuardSessionTransitionStatus::NoActiveHost;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
		Result.Diagnostic =
			TEXT("Weapon-guard release found no active Host.");
		return Result;
	}

	Fdemo_mapShanmenWeaponGuardProductRouteResult Candidate = ActiveRoute;
	const FGuid HostId = Candidate.ProductStart.Host.GetHostId();
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
	Result.HostId = HostId;
	Result.Recovery = Candidate.ProductStart.Host.TryEnterRecovery();
	if (!Result.Recovery.IsSuccess())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				RecoveryRejected,
			HostId,
			TEXT("Weapon-guard Host rejected Active-to-Recovery release."));
	}
	Result.Terminal = Candidate.ProductStart.Host.TryComplete();
	if (!Result.Terminal.IsSuccess())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				CompletionRejected,
			HostId,
			TEXT("Weapon-guard Host rejected Recovery-to-Completed release."));
	}
	Result.Status =
		Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Completed;
	Result.Error = Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
	Result.Diagnostic =
		TEXT("Weapon-guard Host completed ordered release and Session retired it.");
	if (!Candidate.ProductStart.Host.IsTerminal() || !Result.IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				StateDesynchronized,
			HostId,
			TEXT("Weapon-guard release proof failed closed."));
	}
	Clear();
	return Result;
}

Fdemo_mapShanmenWeaponGuardSessionTransitionResult
Fdemo_mapShanmenWeaponGuardProductSession::TryInterruptAndReset()
{
	if (!IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::SessionInvalid,
			FGuid(),
			TEXT("Weapon-guard teardown requires a valid Session."));
	}
	if (!bHasActiveRoute)
	{
		Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
		Result.Status =
			Edemo_mapShanmenWeaponGuardSessionTransitionStatus::NoActiveHost;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
		Result.Diagnostic =
			TEXT("Weapon-guard teardown found no active Host.");
		return Result;
	}

	Fdemo_mapShanmenWeaponGuardProductRouteResult Candidate = ActiveRoute;
	const FGuid HostId = Candidate.ProductStart.Host.GetHostId();
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result;
	Result.HostId = HostId;
	Result.Terminal = Candidate.ProductStart.Host.TryInterrupt();
	if (!Result.Terminal.IsSuccess())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				InterruptRejected,
			HostId,
			TEXT("Weapon-guard Host rejected Run-teardown interruption."));
	}
	Result.Status =
		Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Interrupted;
	Result.Error = Edemo_mapShanmenWeaponGuardSessionTransitionError::None;
	Result.Diagnostic =
		TEXT("Weapon-guard Host interrupted before Session teardown.");
	if (!Candidate.ProductStart.Host.IsTerminal() || !Result.IsValid())
	{
		return RejectTransition(
			Edemo_mapShanmenWeaponGuardSessionTransitionError::
				StateDesynchronized,
			HostId,
			TEXT("Weapon-guard interruption proof failed closed."));
	}
	Clear();
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
