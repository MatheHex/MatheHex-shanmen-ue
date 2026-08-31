#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenPlayerActionArbitration.h"
#include "demo_mapShanmenSpiritEvasionProductAuthority.h"

class ACharacter;
class Fdemo_mapCombatRunCoordinator;
class Idemo_mapShanmenSpiritEvasionPreflightPort;
class Udemo_mapShanmenSpiritEvasionComponent;

enum class Edemo_mapShanmenSpiritEvasionProductRouteStatus : uint8
{
	Applied,
	OwnerUnavailable,
	WorldUnavailable,
	ComponentUnavailable,
	ComponentOwnerMismatch,
	ComponentBusy,
	CoordinatorUnavailable,
	OwnerNotRegistered,
	ProductRejected,
	ActionConflict,
	CommandRouteRejected
};

/**
 * End-to-end product proof from one direction intent to the exact component
 * start receipt. No field is reconstructed by GameMode or an input adapter.
 */
struct Fdemo_mapShanmenSpiritEvasionProductRouteResult
{
	Edemo_mapShanmenSpiritEvasionProductRouteStatus Status =
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::OwnerUnavailable;
	Fdemo_mapShanmenSpiritEvasionProductStartResult ProductStart;
	Fdemo_mapShanmenPlayerActionGateResult ActionGate;
	Fdemo_mapShanmenSpiritEvasionCommandResult CommandRoute;
	FString Diagnostic;

	bool IsAccepted() const;
};

/**
 * Stateless composition seam used by GameMode's sole Spirit Evasion start
 * entry. It rejects every observable owner/component/Run fence before asking
 * P10.8 to consume an activation sequence, then delegates the frozen command
 * only to P10.7. It owns no input, resource, clock, movement or lifecycle
 * state.
 */
struct Fdemo_mapShanmenSpiritEvasionProductRoute
{
	static Fdemo_mapShanmenSpiritEvasionProductRouteResult TryRoute(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const FVector& CandidateDirection,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

#if WITH_DEV_AUTOMATION_TESTS
	static Fdemo_mapShanmenSpiritEvasionProductRouteResult
	TryRouteAtForAutomation(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const FVector& CandidateDirection,
		double StartTimeSeconds,
		Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort);
	static Fdemo_mapShanmenSpiritEvasionProductRouteResult
	TryRouteAtForAutomation(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const FVector& CandidateDirection,
		double StartTimeSeconds,
		Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);
#endif
};
