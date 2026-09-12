#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInputAdapter.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapShanmenFormationRunLifecycle;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationAnchorProductRouteStatus : uint8
{
	Invalid,
	GameplayBlocked,
	CoordinatorUnavailable,
	LifecycleUnavailable,
	LifecycleRunMismatch,
	WorldUnavailable,
	ActorClassUnavailable,
	Routed,
	StateDesynchronized
};

/** Audit proof for one concrete anchor-input-to-product route. */
class Fdemo_mapShanmenFormationAnchorProductRouteResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	Edemo_mapShanmenFormationAnchorProductRouteStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetDependencyValidationCount() const
	{
		return DependencyValidationCount;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	const FGuid& GetLifecycleRunId() const { return LifecycleRunId; }
	const FString& GetActorClassPath() const { return ActorClassPath; }
	const Fdemo_mapShanmenFormationAnchorInputResult& GetInput() const
	{
		return Input;
	}

private:
	friend struct Fdemo_mapShanmenFormationAnchorProductRoute;

	Edemo_mapShanmenFormationAnchorProductRouteStatus Status =
		Edemo_mapShanmenFormationAnchorProductRouteStatus::Invalid;
	FString Diagnostic;
	int32 DependencyValidationCount = 0;
	bool bStateDesynchronized = false;
	FGuid RunId;
	FGuid PlayerEntityId;
	FGuid LifecycleRunId;
	FString ActorClassPath;
	Fdemo_mapShanmenFormationAnchorInputResult Input;
};

/**
 * Stateless concrete product seam for one future formation anchor input.
 *
 * Gameplay gating happens before dependency inspection or anchor sampling.
 * Run identity comes only from the ready Combat Run, and the lifecycle
 * callback is fixed to the sole formation Run lifecycle. This route owns no
 * key, UI, World, Actor, inventory, sequence, retry, cache or product state.
 */
struct Fdemo_mapShanmenFormationAnchorProductRoute
{
	static Fdemo_mapShanmenFormationAnchorProductRouteResult RouteAnchor(
		bool bGameplayInputAllowed,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenFormationRunLifecycle& Lifecycle,
		UWorld* World,
		TSubclassOf<AActor> ActorClass,
		const FGuid& InputEventId,
		FName RequestedAnchorDefinitionId);
};
