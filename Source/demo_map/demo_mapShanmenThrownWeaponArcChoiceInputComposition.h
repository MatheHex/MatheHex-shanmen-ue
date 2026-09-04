#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcChoiceProjection.h"
#include "demo_mapShanmenThrownWeaponInputAdapter.h"

enum class Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus : uint8
{
	Invalid,
	InputCompletedBeforeProjection,
	ProjectionRejected,
	Delegated,
	RouteProtocolRejected
};

/**
 * Audit result for one synchronous choice-to-InputAdapter composition.
 *
 * Input remains the sole product-routing result. This wrapper records only
 * whether lazy basis sampling and projection occurred before that route ended.
 */
class Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetRouteInvocationCount() const { return RouteInvocationCount; }
	int32 GetBasisSampleCount() const { return BasisSampleCount; }
	int32 GetTargetRequestCount() const { return TargetRequestCount; }
	int32 GetApexRequestCount() const { return ApexRequestCount; }
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult&
	GetProjection() const
	{
		return Projection;
	}
	const Fdemo_mapShanmenThrownWeaponInputResult& GetInput() const
	{
		return Input;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition;

	Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus Status =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus::Invalid;
	FString Diagnostic;
	int32 RouteInvocationCount = 0;
	int32 BasisSampleCount = 0;
	int32 TargetRequestCount = 0;
	int32 ApexRequestCount = 0;
	bool bRouteProtocolViolation = false;
	Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Projection;
	Fdemo_mapShanmenThrownWeaponInputResult Input;
};

/**
 * Stateless, consumer-owned bridge from a frozen Arc choice to the existing
 * synchronous Arc InputAdapter route. The route remains responsible for item,
 * Run, source, action, world-delivery, and selection-ordinal authority.
 */
class Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition
{
public:
	using FRouteProjectedArc = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputResult(
			TFunctionRef<FVector()>,
			TFunctionRef<double()>)>;

	Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult Route(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy,
		TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>
			SampleBasis,
		FRouteProjectedArc RouteProjectedArc) const;
};
