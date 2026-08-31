#include "demo_mapShanmenWeaponGuardProductRoute.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"

namespace
{
	bool HasExactBinding(
		const Fdemo_mapShanmenWeaponGuardItemAuthorization& Authorization,
		const Fdemo_mapShanmenWeaponGuardProductStartResult& ProductStart)
	{
		if (!Authorization.IsValid() || !ProductStart.IsReady())
		{
			return false;
		}

		const FGuid& ItemId = Authorization.GetSourceItemInstanceId();
		return ProductStart.Reservation.GetSourceItemInstanceId() == ItemId
			&& ProductStart.Reservation.GetAction()
				.GetSourceItemInstanceId() == ItemId
			&& ProductStart.Host.GetActionRuntime().GetAction()
				.GetSourceItemInstanceId() == ItemId;
	}
}

bool Fdemo_mapShanmenWeaponGuardProductRouteResult::IsReady() const
{
	return Status == Edemo_mapShanmenWeaponGuardProductRouteStatus::Ready
		&& ItemAuthorization.IsAuthorized()
		&& ProductStart.IsReady()
		&& HasExactBinding(
			ItemAuthorization.Authorization,
			ProductStart);
}

Fdemo_mapShanmenWeaponGuardProductRouteResult
Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
	const Fdemo_mapItemAuthority& ItemAuthority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FGuid& TimelineId,
	int64 ActiveStartTick)
{
	Fdemo_mapShanmenWeaponGuardProductRouteResult Result;
	Result.ItemAuthorization =
		Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon(
			ItemAuthority);
	if (!Result.ItemAuthorization.IsAuthorized())
	{
		Result.Diagnostic = Result.ItemAuthorization.Diagnostic;
		return Result;
	}

	const Fdemo_mapShanmenWeaponGuardItemAuthorization& Authorization =
		Result.ItemAuthorization.Authorization;
	const int32 AuthorizedRevision = Authorization.GetAuthorityRevision();
	if (ItemAuthority.GetAuthorityRevision() != AuthorizedRevision
		|| !Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			ItemAuthority,
			Authorization))
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductRouteStatus::
				ItemAuthorizationStale;
		Result.Diagnostic =
			TEXT("Equipped weapon authorization changed before product start.");
		return Result;
	}

	Result.ProductStart =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Coordinator,
			Authorization.GetSourceItemInstanceId(),
			TimelineId,
			ActiveStartTick);
	if (!Result.ProductStart.IsReady())
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductRouteStatus::
				ProductStartRejected;
		Result.Diagnostic = Result.ProductStart.Diagnostic;
		return Result;
	}

	if (ItemAuthority.GetAuthorityRevision() != AuthorizedRevision
		|| !Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			ItemAuthority,
			Authorization))
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductRouteStatus::
				ItemAuthorizationStale;
		Result.Diagnostic =
			TEXT("Equipped weapon authorization changed during product start; the reserved action remains consumed.");
		return Result;
	}

	if (!HasExactBinding(Authorization, Result.ProductStart))
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductRouteStatus::BindingRejected;
		Result.Diagnostic =
			TEXT("Weapon-guard product start did not retain the authorized item identity.");
		return Result;
	}

	Result.Status = Edemo_mapShanmenWeaponGuardProductRouteStatus::Ready;
	Result.Diagnostic =
		TEXT("Current equipped weapon authorized and started one canonical guard host.");
	return Result;
}
