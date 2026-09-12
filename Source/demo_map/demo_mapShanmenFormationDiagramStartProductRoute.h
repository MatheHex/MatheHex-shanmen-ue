#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationDiagramStartInputComposition.h"

class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapShanmenDivineSenseProductController;
class Fdemo_mapShanmenFormationRunLifecycle;
class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationDiagramStartProductRouteStatus : uint8
{
	Invalid,
	GameplayBlocked,
	CoordinatorUnavailable,
	LifecycleUnavailable,
	LifecycleRunMismatch,
	SpiritEnergyUnavailable,
	SpiritEnergyRunMismatch,
	SpiritEnergyOwnerMismatch,
	Routed,
	StateDesynchronized
};

/**
 * Audit proof for one concrete diagram-to-product start route.
 *
 * The captured identities come only from the supplied active product owners;
 * callers cannot inject a Run or owner identity around the shared-energy
 * formation lifecycle.
 */
class Fdemo_mapShanmenFormationDiagramStartProductRouteResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	Edemo_mapShanmenFormationDiagramStartProductRouteStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetDependencyValidationCount() const
	{
		return DependencyValidationCount;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	const FGuid& GetLifecycleRunId() const { return LifecycleRunId; }
	const FGuid& GetSpiritEnergyRunId() const
	{
		return SpiritEnergyRunId;
	}
	const FGuid& GetSpiritEnergyOwnerId() const
	{
		return SpiritEnergyOwnerId;
	}
	const Fdemo_mapShanmenFormationDiagramStartInputCompositionResult&
	GetComposition() const
	{
		return Composition;
	}

private:
	friend struct Fdemo_mapShanmenFormationDiagramStartProductRoute;

	Edemo_mapShanmenFormationDiagramStartProductRouteStatus Status =
		Edemo_mapShanmenFormationDiagramStartProductRouteStatus::Invalid;
	FString Diagnostic;
	int32 DependencyValidationCount = 0;
	bool bStateDesynchronized = false;
	FGuid RunId;
	FGuid OwnerId;
	FGuid LifecycleRunId;
	FGuid SpiritEnergyRunId;
	FGuid SpiritEnergyOwnerId;
	Fdemo_mapShanmenFormationDiagramStartInputCompositionResult Composition;
};

/**
 * Stateless final product seam for a future formation-start input surface.
 *
 * Gameplay gating happens before dependency inspection or personal knowledge
 * reads. A valid route derives Run/owner identity from the Combat Run, proves
 * that the formation lifecycle and shared SpiritEnergy controller have the
 * same binding, then delegates only through the P27.11 composition. It owns no
 * input binding, key, UI, catalog, knowledge, item, resource, Run, retry,
 * World, Actor, GameMode or product state.
 */
struct Fdemo_mapShanmenFormationDiagramStartProductRoute
{
	using FReadAuthority =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::FReadAuthority;

	static Fdemo_mapShanmenFormationDiagramStartProductRouteResult RouteStart(
		bool bGameplayInputAllowed,
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenFormationRunLifecycle& Lifecycle,
		Fdemo_mapShanmenDivineSenseProductController&
			SpiritEnergyController,
		const FGuid& InputEventId,
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		FName RequestedDiagramDefinitionId,
		const FVector& Origin,
		const FVector& Forward,
		FReadAuthority ReadAuthority);
};
