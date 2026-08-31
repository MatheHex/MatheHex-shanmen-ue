#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenWeaponGuardItemAdapter.h"
#include "demo_mapShanmenWeaponGuardProductAuthority.h"

class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapItemAuthority;

enum class Edemo_mapShanmenWeaponGuardProductRouteStatus : uint8
{
	Ready,
	ItemAuthorizationRejected,
	ProductStartRejected,
	ItemAuthorizationStale,
	BindingRejected
};

/** Complete synchronous proof from equipped-item truth to one active host. */
struct Fdemo_mapShanmenWeaponGuardProductRouteResult
{
	Edemo_mapShanmenWeaponGuardProductRouteStatus Status =
		Edemo_mapShanmenWeaponGuardProductRouteStatus::
			ItemAuthorizationRejected;
	Fdemo_mapShanmenWeaponGuardItemResult ItemAuthorization;
	Fdemo_mapShanmenWeaponGuardProductStartResult ProductStart;
	FString Diagnostic;

	bool IsReady() const;
};

/**
 * Stateless bridge from the existing equipment authority to P11.6.
 *
 * No item identity is accepted from the caller. The route resolves and
 * authorizes the current WeaponSlot occupant, passes only that exact identity
 * into the canonical product authority, and verifies the resulting binding.
 * It owns no inventory, input, clock, Run, host, Actor or Impact state.
 */
struct Fdemo_mapShanmenWeaponGuardProductRoute
{
	static Fdemo_mapShanmenWeaponGuardProductRouteResult TryStart(
		const Fdemo_mapItemAuthority& ItemAuthority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& TimelineId,
		int64 ActiveStartTick);
};
