#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordQiProductController.h"

/** One immutable origin/aim sample from a caller-owned input surface. */
class Fdemo_mapShanmenSwordQiInputSample
{
public:
	static bool TryCapture(
		const FVector& RequestedOrigin,
		const FVector& RequestedAimDirection,
		Fdemo_mapShanmenSwordQiInputSample& OutSample);

	bool IsValid() const;
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetAimDirection() const { return AimDirection; }

private:
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ZeroVector;
};

enum class Edemo_mapShanmenSwordQiInputStatus : uint8
{
	Applied,
	GameplayBlocked,
	ProductRouteUnavailable,
	RunUnavailable,
	EventIdentityInvalid,
	SpatialSampleRejected,
	IntentCaptureRejected,
	ProductRejected
};

/** Audit proof that one input event samples and delegates at most once. */
struct Fdemo_mapShanmenSwordQiInputResult
{
	Edemo_mapShanmenSwordQiInputStatus Status =
		Edemo_mapShanmenSwordQiInputStatus::GameplayBlocked;
	bool bSpatialSampled = false;
	bool bProductRouteInvoked = false;
	FGuid InputEventId;
	FGuid RunId;
	FGuid IntentId;
	Fdemo_mapShanmenSwordQiInputSample Sample;
	Fdemo_mapShanmenSwordQiIntent Intent;
	Fdemo_mapShanmenSwordQiControllerResult Product;
	FString Diagnostic;

	bool IsAccepted() const;
};

/**
 * Stateless seam between a future physical command and the P18.4 controller.
 *
 * The caller owns gameplay gating, stable input-event identity and one
 * origin/aim sample. This adapter namespaces the event into one Run intent,
 * then invokes the sole product route exactly once. It owns no key, retry,
 * equipment, attribute, sequence, Actor, inventory or damage authority.
 */
struct Fdemo_mapShanmenSwordQiInputAdapter
{
	static FGuid MakeIntentId(
		const FGuid& RunId,
		const FGuid& InputEventId);

	static Fdemo_mapShanmenSwordQiInputResult RouteStartInput(
		bool bGameplayInputAllowed,
		bool bProductRouteAvailable,
		const FGuid& RunId,
		const FGuid& InputEventId,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputSample()> SampleSpatialInput,
		TFunctionRef<Fdemo_mapShanmenSwordQiControllerResult(
			const Fdemo_mapShanmenSwordQiIntent&)> RouteStartIntent);
};
