#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationDiagramAccessAdapter.h"
#include "demo_mapShanmenFormationInputAdapter.h"

enum class Edemo_mapShanmenFormationDiagramStartInputCompositionStatus : uint8
{
	Invalid,
	InputCompletedBeforeAccess,
	DiagramAccessRejected,
	SpatialSampleRejected,
	Delegated,
	RouteProtocolRejected
};

/**
 * Audit result for one synchronous diagram-access-to-start-input composition.
 *
 * Input remains the sole product-routing result. This wrapper records whether
 * the existing input route required one personal knowledge read, obtained one
 * owner-bound selection, and captured one spatial sample before delegation.
 */
class Fdemo_mapShanmenFormationDiagramStartInputCompositionResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	Edemo_mapShanmenFormationDiagramStartInputCompositionStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetAccessInvocationCount() const { return AccessInvocationCount; }
	int32 GetSpatialSampleCaptureCount() const
	{
		return SpatialSampleCaptureCount;
	}
	const Fdemo_mapShanmenFormationDiagramAccessResult& GetAccess() const
	{
		return Access;
	}
	const Fdemo_mapShanmenFormationStartInputResult& GetInput() const
	{
		return Input;
	}

private:
	friend struct Fdemo_mapShanmenFormationDiagramStartInputComposition;

	Edemo_mapShanmenFormationDiagramStartInputCompositionStatus Status =
		Edemo_mapShanmenFormationDiagramStartInputCompositionStatus::Invalid;
	FString Diagnostic;
	int32 AccessInvocationCount = 0;
	int32 SpatialSampleCaptureCount = 0;
	bool bRouteProtocolViolation = false;
	Fdemo_mapShanmenFormationDiagramAccessResult Access;
	Fdemo_mapShanmenFormationStartInputResult Input;
};

/**
 * Stateless, consumer-owned bridge from an authored diagram request and one
 * progression/save knowledge read to the existing formation start input route.
 *
 * The input adapter preserves gameplay/lifecycle/identity preflight order and
 * invokes the access callback lazily at most once. The caller still owns the
 * concrete lifecycle route, including the P27.10 shared SpiritEnergy parameter.
 * This composition owns no catalog, knowledge, unlock, input, resource, Run,
 * retry, World, Actor, GameMode, UI, or product state.
 */
struct Fdemo_mapShanmenFormationDiagramStartInputComposition
{
	using FReadAuthority =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::FReadAuthority;
	using FRouteStart = Fdemo_mapShanmenFormationInputAdapter::FRouteStart;

	static Fdemo_mapShanmenFormationDiagramStartInputCompositionResult Route(
		bool bGameplayInputAllowed,
		bool bLifecycleAvailable,
		const FGuid& RunId,
		const FGuid& OwnerId,
		const FGuid& InputEventId,
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		FName RequestedDiagramDefinitionId,
		const FVector& Origin,
		const FVector& Forward,
		FReadAuthority ReadAuthority,
		FRouteStart RouteLifecycle);
};
