#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritEvasionProductRoute.h"

/** Outcome of adapting one dedicated Spirit Evasion start input. */
enum class Edemo_mapShanmenSpiritEvasionInputStatus : uint8
{
	Applied,
	GameplayBlocked,
	ProductRouteUnavailable,
	ProductRejected
};

/** Audit proof that the device boundary sampled and delegated at most once. */
struct Fdemo_mapShanmenSpiritEvasionInputResult
{
	Edemo_mapShanmenSpiritEvasionInputStatus Status =
		Edemo_mapShanmenSpiritEvasionInputStatus::GameplayBlocked;
	bool bDirectionSampled = false;
	bool bProductRouteInvoked = false;
	FVector SampledDirection = FVector::ZeroVector;
	Fdemo_mapShanmenSpiritEvasionProductRouteResult ProductRoute;
	FString Diagnostic;

	bool IsAccepted() const;
};

/**
 * Stateless device-to-product seam for a future physical Spirit Evasion key.
 * It checks the existing PlayerController gameplay surface and product-route
 * availability before sampling. An eligible input samples one direction and
 * invokes the P10.9 product route exactly once; it never retries, normalizes,
 * chooses product identity, or falls back to a legacy skill path.
 */
struct Fdemo_mapShanmenSpiritEvasionInputAdapter
{
	static Fdemo_mapShanmenSpiritEvasionInputResult RouteStartInput(
		bool bGameplayInputAllowed,
		bool bProductRouteAvailable,
		TFunctionRef<FVector()> SampleDirection,
		TFunctionRef<Fdemo_mapShanmenSpiritEvasionProductRouteResult(
			const FVector&)> RouteStartIntent);
};
