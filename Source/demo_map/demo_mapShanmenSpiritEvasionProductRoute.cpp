#include "demo_mapShanmenSpiritEvasionProductRoute.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenSpiritEvasionComponent.h"
#include "GameFramework/Character.h"

namespace
{
	bool ValidateEntry(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		Fdemo_mapShanmenSpiritEvasionProductRouteResult& OutResult)
	{
		if (!::IsValid(Owner))
		{
			OutResult.Diagnostic =
				TEXT("Spirit Evasion product route requires the player Character.");
			return false;
		}
		if (!Owner->GetWorld())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::
					WorldUnavailable;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion product route requires the player's World.");
			return false;
		}
		if (!::IsValid(Component) || !Component->IsRegistered())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::
					ComponentUnavailable;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion product component is unavailable.");
			return false;
		}
		if (Component->GetOwner() != Owner)
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::
					ComponentOwnerMismatch;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion product component belongs to another owner.");
			return false;
		}
		if (!Component->CanStart())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::ComponentBusy;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion product component already owns a nonterminal action.");
			return false;
		}
		if (!Coordinator.IsReady())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::
					CoordinatorUnavailable;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion product route requires one active combat Run.");
			return false;
		}

		FGuid ResolvedOwnerId;
		if (!Coordinator.GetEntityRegistry().TryResolveObject(
				Coordinator.GetRunId(),
				Owner,
				INDEX_NONE,
				ResolvedOwnerId)
			|| ResolvedOwnerId != Coordinator.GetPlayerEntityId())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::
					OwnerNotRegistered;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion route owner is not the active Run player entity.");
			return false;
		}
		return true;
	}

	template <typename AuthorizeType, typename DispatchType>
	Fdemo_mapShanmenSpiritEvasionProductRouteResult RouteIntent(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const FVector& CandidateDirection,
		AuthorizeType&& AuthorizeAction,
		DispatchType&& Dispatch)
	{
		Fdemo_mapShanmenSpiritEvasionProductRouteResult Result;
		if (!ValidateEntry(Component, Coordinator, Owner, Result))
		{
			return Result;
		}

		Result.ProductStart =
			Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
				Coordinator,
				CandidateDirection);
		if (!Result.ProductStart.IsReady())
		{
			Result.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::ProductRejected;
			Result.Diagnostic = Result.ProductStart.Diagnostic;
			return Result;
		}

		Result.ActionGate = AuthorizeAction();
		if (!Result.ActionGate.IsAuthorized())
		{
			Result.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::ActionConflict;
			Result.Diagnostic = Result.ActionGate.Diagnostic;
			return Result;
		}

		Result.CommandRoute = Dispatch(Result.ProductStart.Command);
		if (!Result.CommandRoute.IsAccepted())
		{
			Result.Status =
				Edemo_mapShanmenSpiritEvasionProductRouteStatus::
					CommandRouteRejected;
			Result.Diagnostic = Result.CommandRoute.Diagnostic;
			return Result;
		}

		Result.Status =
			Edemo_mapShanmenSpiritEvasionProductRouteStatus::Applied;
		Result.Diagnostic =
			TEXT("Spirit Evasion direction intent reserved and applied one canonical action.");
		return Result;
	}
}

bool Fdemo_mapShanmenSpiritEvasionProductRouteResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSpiritEvasionProductRouteStatus::Applied
		&& ProductStart.IsReady()
		&& ActionGate.IsAuthorized()
		&& CommandRoute.IsAccepted()
		&& CommandRoute.Kind
			== Edemo_mapShanmenSpiritEvasionCommandKind::Start
		&& CommandRoute.ActivationId
			== ProductStart.Reservation.GetActivationId()
		&& CommandRoute.ActivationId
			== ProductStart.Command.GetActivationId();
}

Fdemo_mapShanmenSpiritEvasionProductRouteResult
Fdemo_mapShanmenSpiritEvasionProductRoute::TryRoute(
	Udemo_mapShanmenSpiritEvasionComponent* Component,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	ACharacter* Owner,
	const FVector& CandidateDirection,
	TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()> AuthorizeAction)
{
	return RouteIntent(
		Component,
		Coordinator,
		Owner,
		CandidateDirection,
		AuthorizeAction,
		[Component, &Coordinator, Owner](
			const Fdemo_mapShanmenSpiritEvasionCommand& Command)
		{
			return Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
				Component,
				Coordinator,
				Owner,
				Command);
		});
}

#if WITH_DEV_AUTOMATION_TESTS
Fdemo_mapShanmenSpiritEvasionProductRouteResult
Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
	Udemo_mapShanmenSpiritEvasionComponent* Component,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	ACharacter* Owner,
	const FVector& CandidateDirection,
	double StartTimeSeconds,
	Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort)
{
	return TryRouteAtForAutomation(
		Component,
		Coordinator,
		Owner,
		CandidateDirection,
		StartTimeSeconds,
		PreflightPort,
		[&Coordinator]()
		{
			const Fdemo_mapShanmenPlayerActionArbitrationReceipt Receipt =
				Coordinator.TryAuthorizePlayerAction(
					Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
					Fdemo_mapShanmenPlayerActionOccupancySnapshot());
			return Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
				Receipt);
		});
}

Fdemo_mapShanmenSpiritEvasionProductRouteResult
Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
	Udemo_mapShanmenSpiritEvasionComponent* Component,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	ACharacter* Owner,
	const FVector& CandidateDirection,
	double StartTimeSeconds,
	Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort,
	TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()> AuthorizeAction)
{
	return RouteIntent(
		Component,
		Coordinator,
		Owner,
		CandidateDirection,
		AuthorizeAction,
		[Component, &Coordinator, Owner, StartTimeSeconds, &PreflightPort](
			const Fdemo_mapShanmenSpiritEvasionCommand& Command)
		{
			return Fdemo_mapShanmenSpiritEvasionCommandRouter::
				TryRouteAtForAutomation(
					Component,
					Coordinator,
					Owner,
					Command,
					StartTimeSeconds,
					PreflightPort);
		});
}
#endif
