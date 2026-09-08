#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponRunCommandRouter.h"

struct Fdemo_mapShanmenControlledWeaponWorldLifecycle;

/** Frozen canonical flying-sword state observed for one physical input press. */
class Fdemo_mapShanmenControlledWeaponInputReadModel
{
public:
	static bool TryCapture(
		const FGuid& RunId,
		const FGuid& ItemInstanceId,
		EShanmenControlledWeaponState State,
		Fdemo_mapShanmenControlledWeaponInputReadModel& OutReadModel);

	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	EShanmenControlledWeaponState GetState() const { return State; }

private:
	FGuid RunId;
	FGuid ItemInstanceId;
	EShanmenControlledWeaponState State =
		EShanmenControlledWeaponState::Recalled;
};

/** Outcome of one rebindable Launch-or-Recall flying-sword press. */
enum class Edemo_mapShanmenControlledWeaponInputStatus : uint8
{
	Applied,
	GameplayBlocked,
	ProductRouteUnavailable,
	CanonicalWeaponUnavailable,
	IntentIdInvalid,
	IntentCaptureRejected,
	ProductRejected
};

/** Audit proof that each external input dependency was consumed at most once. */
struct Fdemo_mapShanmenControlledWeaponInputResult
{
	Edemo_mapShanmenControlledWeaponInputStatus Status =
		Edemo_mapShanmenControlledWeaponInputStatus::GameplayBlocked;
	bool bCanonicalReadInvoked = false;
	bool bIntentIdCreated = false;
	bool bDirectionSampled = false;
	bool bIntentCaptured = false;
	bool bProductRouteInvoked = false;
	Fdemo_mapShanmenControlledWeaponInputReadModel ReadModel;
	FGuid IntentId;
	FVector SampledDirection = FVector::ZeroVector;
	Fdemo_mapShanmenControlledWeaponRunCommandIntent Intent;
	Fdemo_mapShanmenControlledWeaponRunCommandResult ProductRoute;
	FString Diagnostic;

	bool IsAccepted() const;
};

/**
 * Stateless physical-input seam for the canonical TrainingFlyingSword.
 *
 * Orbiting maps to Launch with one current-aim sample. Directed maps to Recall
 * without sampling aim. Canonical identity is read once, a fresh intent id is
 * created once, and the existing P6 Run command route is invoked at most once.
 * This adapter never chooses another item, retries, moves an Actor, or resolves
 * damage.
 */
struct Fdemo_mapShanmenControlledWeaponInputAdapter
{
	static bool TryReadCanonical(
		const Fdemo_mapShanmenControlledWeaponWorldLifecycle& Lifecycle,
		const Fdemo_mapShanmenControlledWeaponRunHost& Host,
		Fdemo_mapShanmenControlledWeaponInputReadModel& OutReadModel);

	static Fdemo_mapShanmenControlledWeaponInputResult RouteToggleInput(
		bool bGameplayInputAllowed,
		bool bProductRouteAvailable,
		TFunctionRef<bool(
			Fdemo_mapShanmenControlledWeaponInputReadModel&)> ReadCanonicalWeapon,
		TFunctionRef<FGuid()> CreateIntentId,
		TFunctionRef<FVector()> SampleLaunchDirection,
		TFunctionRef<Fdemo_mapShanmenControlledWeaponRunCommandResult(
			const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)> RouteIntent);
};
