#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenWeaponGuardProductRoute.h"

class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapItemAuthority;

enum class Edemo_mapShanmenWeaponGuardSessionStartStatus : uint8
{
	Rejected,
	Started,
	AlreadyActive
};

enum class Edemo_mapShanmenWeaponGuardSessionStartError : uint8
{
	None,
	ItemAuthorityUnavailable,
	AlreadyActive,
	RouteRejected,
	StateDesynchronized
};

/** Auditable result for acquiring the sole active weapon-guard Host. */
struct Fdemo_mapShanmenWeaponGuardSessionStartResult
{
	Edemo_mapShanmenWeaponGuardSessionStartStatus Status =
		Edemo_mapShanmenWeaponGuardSessionStartStatus::Rejected;
	Edemo_mapShanmenWeaponGuardSessionStartError Error =
		Edemo_mapShanmenWeaponGuardSessionStartError::
			ItemAuthorityUnavailable;
	FGuid HostId;
	FGuid SourceItemInstanceId;
	Fdemo_mapShanmenWeaponGuardProductRouteResult Route;
	FString Diagnostic;

	bool IsValid() const;
	bool IsStarted() const;
	bool IsAlreadyActive() const;
};

enum class Edemo_mapShanmenWeaponGuardSessionTransitionStatus : uint8
{
	Rejected,
	NoActiveHost,
	Completed,
	Interrupted
};

/**
 * Typed cause for retiring the sole active weapon-guard Host.
 *
 * Input release is the only orderly Active -> Recovery -> Completed path.
 * Every other cause is an immediate Active -> Interrupted path. The reason is
 * carried by the terminal receipt so lifecycle callers never have to infer it
 * from a free-form diagnostic string.
 */
enum class Edemo_mapShanmenWeaponGuardTerminationReason : uint8
{
	None,
	InputReleased,
	EffectiveDamageStagger,
	WeaponAuthorizationChanged,
	PlayerDefeated,
	PawnUnpossessed,
	ControllerEndPlay,
	RunTeardown
};

enum class Edemo_mapShanmenWeaponGuardSessionTransitionError : uint8
{
	None,
	InvalidTerminationReason,
	SessionInvalid,
	RecoveryRejected,
	CompletionRejected,
	InterruptRejected,
	StateDesynchronized
};

enum class Edemo_mapShanmenWeaponGuardSessionDefenseStatus : uint8
{
	Rejected,
	ComposedQualified,
	ComposedOutsideArc
};

enum class Edemo_mapShanmenWeaponGuardSessionDefenseError : uint8
{
	None,
	SessionInvalid,
	NoActiveHost,
	InvalidTimelineSample,
	TimelineMismatch,
	HostRejected,
	StateDesynchronized
};

/**
 * Transactional proof that the Session composed one impact's defense.
 *
 * A successful result advances the Host's monotonic observation exactly once.
 * A rejection leaves the Session and its active Host unchanged.
 */
struct Fdemo_mapShanmenWeaponGuardSessionDefenseResult
{
	Edemo_mapShanmenWeaponGuardSessionDefenseStatus Status =
		Edemo_mapShanmenWeaponGuardSessionDefenseStatus::Rejected;
	Edemo_mapShanmenWeaponGuardSessionDefenseError Error =
		Edemo_mapShanmenWeaponGuardSessionDefenseError::SessionInvalid;
	FGuid HostId;
	FGuid SourceItemInstanceId;
	FGuid TimelineId;
	int64 ObservedTick = INDEX_NONE;
	Fdemo_mapShanmenWeaponGuardHostDefenseResult Defense;
	FString Diagnostic;

	bool IsValid() const;
	bool IsSuccess() const;
	bool HasGuardLayer() const;
};

/** Ordered lifecycle proof returned before the Session forgets its Host. */
struct Fdemo_mapShanmenWeaponGuardSessionTransitionResult
{
	Edemo_mapShanmenWeaponGuardSessionTransitionStatus Status =
		Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Rejected;
	Edemo_mapShanmenWeaponGuardSessionTransitionError Error =
		Edemo_mapShanmenWeaponGuardSessionTransitionError::SessionInvalid;
	Edemo_mapShanmenWeaponGuardTerminationReason Reason =
		Edemo_mapShanmenWeaponGuardTerminationReason::None;
	FGuid HostId;
	Fdemo_mapShanmenWeaponGuardHostTransitionResult Recovery;
	Fdemo_mapShanmenWeaponGuardHostTransitionResult Terminal;
	FString Diagnostic;

	bool IsValid() const;
	bool IsSuccess() const;
	bool IsNoOp() const;
};

/**
 * Sole persistent product owner for the player's current weapon guard.
 *
 * P11.8/P11.9 remain stateless classification and composition seams. This
 * value owns their successful active Host until the sole typed termination
 * route completes or interrupts it. It owns no input binding, clock, Actor,
 * inventory or Impact.
 */
class Fdemo_mapShanmenWeaponGuardProductSession
{
public:
	Fdemo_mapShanmenWeaponGuardSessionStartResult TryStart(
		const Fdemo_mapItemAuthority* ItemAuthority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& TimelineId,
		int64 ActiveStartTick);

	/**
	 * Sole terminal command route. Input release completes in order; all other
	 * valid reasons interrupt. Empty state is an accepted idempotent no-op.
	 */
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult
	TryTerminate(Edemo_mapShanmenWeaponGuardTerminationReason Reason);
	/**
	 * Composes one hostile Impact's defense against the sole active Host.
	 * The Session commits the advanced Host only after the complete receipt is
	 * valid, so a rejected world/timeline/arc evaluation is state-atomic.
	 */
	Fdemo_mapShanmenWeaponGuardSessionDefenseResult TryComposeImpactDefense(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* DefenderActor,
		AActor* ThreatActor,
		const FGuid& TimelineId,
		int64 ObservedTick,
		const FShanmenHitCandidate& Candidate,
		const FShanmenDefenseSnapshot& BaseDefense);

	bool IsValid() const;
	bool IsEmpty() const;
	bool HasActive() const;
	bool IsCurrentAuthorization(
		const Fdemo_mapItemAuthority& ItemAuthority) const;
	const Fdemo_mapShanmenWeaponGuardProductRouteResult*
	GetActiveRoute() const;
	const Fdemo_mapShanmenWeaponGuardProductHost* GetActiveHost() const;

private:
	void Clear();

	bool bHasActiveRoute = false;
	Fdemo_mapShanmenWeaponGuardProductRouteResult ActiveRoute;
};
