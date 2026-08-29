#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponRunHost.h"

class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/**
 * Frozen, input-device-independent request for one exact physical throw.
 *
 * Intent identity is the action ActivationId itself. This prevents callers
 * from aliasing one durable item intent behind multiple external command ids.
 * Product configuration (World, projectile class, authority, and source Actor)
 * is supplied to the router and is deliberately not owned by the input intent.
 */
class Fdemo_mapShanmenThrownWeaponRunCommandIntent
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenThrownWeaponDefinition& Definition,
		const FShanmenThrownWeaponOffenseSnapshot& Offense,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance,
		Fdemo_mapShanmenThrownWeaponRunCommandIntent& OutIntent);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Other) const;
	const FGuid& GetIntentId() const { return Action.GetActivationId(); }
	const FGuid& GetRunId() const { return Action.GetRunId(); }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenThrownWeaponDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenThrownWeaponOffenseSnapshot& GetOffense() const
	{
		return Offense;
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetAimDirection() const { return AimDirection; }
	float GetMaximumDistance() const { return MaximumDistance; }

private:
	Fdemo_mapShanmenRunCorrelation Correlation;
	FShanmenCombatActionSnapshot Action;
	FShanmenThrownWeaponDefinition Definition;
	FShanmenThrownWeaponOffenseSnapshot Offense;
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ZeroVector;
	float MaximumDistance = 0.0f;
};

enum class Edemo_mapShanmenThrownWeaponRunCommandStatus : uint8
{
	Applied,
	CoordinatorNotReady,
	IntentInvalid,
	RunMismatch,
	SourceMismatch,
	HostBusy,
	RouterInvalid,
	RouterRunMismatch,
	IntentIdConflict,
	ActionStartRejected,
	ExecutionRejected,
	PrepareRejected,
	ActionCommitRejected,
	LaunchRejectedCancelled,
	RecoveryRequired,
	RecoveryNotFound,
	RecoveryNotApplicable
};

/** Complete audit for one routed throw or one exact replay. */
struct Fdemo_mapShanmenThrownWeaponRunCommandResult
{
	Edemo_mapShanmenThrownWeaponRunCommandStatus Status =
		Edemo_mapShanmenThrownWeaponRunCommandStatus::CoordinatorNotReady;
	bool bReplay = false;
	FGuid IntentId;
	FGuid RunId;
	FGuid ItemInstanceId;
	FGuid LaunchId;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;
	Fdemo_mapShanmenThrownWeaponItemResult Preparation;
	Fdemo_mapShanmenThrownWeaponHostStartResult HostStart;
	Fdemo_mapShanmenThrownWeaponItemResult Cancellation;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsDurableTerminal() const;
	bool IsReplay() const { return bReplay; }
	bool RequiresRecovery() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryRequired;
	}
};

/**
 * Run-scoped command boundary between future selection/input and P7.1/P7.3.
 *
 * One call constructs the pure action runtime, durably prepares one Quantity,
 * crosses the action commit point, and asks the P7.3 Host to spawn and publish
 * the exact straight flight. Exact replay never repeats inventory I/O or Actor
 * creation. A failed pre-launch path is durably cancelled; if cancellation I/O
 * fails, the terminal record blocks unsafe retry until TryRecoverCancellation
 * succeeds. No input binding, inventory mutation, Actor subclass policy, or
 * vitality authority is owned by this router.
 */
class Fdemo_mapShanmenThrownWeaponRunCommandRouter
{
public:
	Fdemo_mapShanmenThrownWeaponRunCommandResult TryRoute(
		Fdemo_mapShanmenThrownWeaponRunHost& Host,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Intent);

	/** Retries only a failed durable pre-launch cancellation; never launches. */
	Fdemo_mapShanmenThrownWeaponRunCommandResult TryRecoverCancellation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Intent);

	bool IsEmpty() const { return ProcessedIntents.IsEmpty(); }
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	int32 NumProcessedIntents() const { return ProcessedIntents.Num(); }
	void Reset();

private:
	struct FProcessedIntent
	{
		Fdemo_mapShanmenThrownWeaponRunCommandIntent Intent;
		Fdemo_mapShanmenThrownWeaponRunCommandResult Result;
	};

	void RecordTerminal(
		const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Intent,
		const Fdemo_mapShanmenThrownWeaponRunCommandResult& Result);

	FGuid RunId;
	TMap<FGuid, FProcessedIntent> ProcessedIntents;
};
