#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenSwordQiProjectile.h"

struct FHitResult;

/** Failure while staging or atomically publishing one sword-qi flight. */
enum class Edemo_mapShanmenSwordQiLaunchError : uint8
{
	None,
	CoordinatorNotReady,
	RunContextMismatch,
	SourceNotRegistered,
	ActionRuntimeInvalid,
	ExecutionInvalid,
	LaunchRejected,
	EmissionRejected,
	ProjectileStageRejected,
	LaunchPathBlocked,
	PublicationRejected
};

/** Copy-on-write launch evidence; the live execution remains Ready. */
struct Fdemo_mapShanmenSwordQiLaunchPlan
{
	FShanmenSwordQiExecution ExecutionCandidate;
	FShanmenSwordQiLaunchReceipt Launch;
	FShanmenWorldHitContext Context;

	bool IsValid() const;
};

/** Auditable staging result for one sword-qi launch. */
struct Fdemo_mapShanmenSwordQiLaunchResult
{
	Edemo_mapShanmenSwordQiLaunchError Error =
		Edemo_mapShanmenSwordQiLaunchError::CoordinatorNotReady;
	Fdemo_mapShanmenSwordQiLaunchPlan Plan;

	bool IsStaged() const
	{
		return Error == Edemo_mapShanmenSwordQiLaunchError::None
			&& Plan.IsValid();
	}
};

/** Failure while converting one sword-qi hit into canonical vitality. */
enum class Edemo_mapShanmenSwordQiWorldDeliveryError : uint8
{
	None,
	CoordinatorNotReady,
	FlightNotActive,
	ContextMismatch,
	ContactNotResolved,
	TargetVitalityUnavailable,
	CandidateRejected,
	DeliveryRejected
};

/** Auditable result for one accepted sword-qi world contact. */
struct Fdemo_mapShanmenSwordQiWorldDeliveryResult
{
	Edemo_mapShanmenSwordQiWorldDeliveryError Error =
		Edemo_mapShanmenSwordQiWorldDeliveryError::CoordinatorNotReady;
	FGuid TargetEntityId;
	FShanmenSwordQiImpactReceipt Impact;
	Fdemo_mapCombatImpactDeliveryResult Delivery;

	bool IsDelivered() const
	{
		return Error == Edemo_mapShanmenSwordQiWorldDeliveryError::None
			&& TargetEntityId.IsValid()
			&& Impact.IsValid()
			&& Delivery.IsSuccess();
	}
	float GetNewlyCommittedDamage() const
	{
		return IsDelivered()
			&& Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			? Delivery.CommitResult.Receipt.GetAppliedDamage()
			: 0.0f;
	}
};

/**
 * Product bridge from immutable sword-qi launch to UE projectile evidence.
 *
 * Launch publication is copy-on-write. Contacts use WorldGameplay identity,
 * the P18.0 resolver, and the existing combat-run vitality authority. The
 * adapter never calls ApplyDamage and does not own spawning or input.
 */
struct Fdemo_mapShanmenSwordQiWorldAdapter
{
	static Fdemo_mapShanmenSwordQiLaunchResult StageLaunch(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const FVector& Origin,
		const FVector& AimDirection);

	static bool PublishStagedLaunch(
		const FShanmenActionOrchestrator& ActionRuntime,
		const Fdemo_mapShanmenSwordQiLaunchPlan& Plan,
		FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile);

	/** Successful hostile contacts do not choose a piercing policy. */
	static Fdemo_mapShanmenSwordQiWorldDeliveryResult ResolveProjectileContact(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FHitResult& Hit);

	/** Explicit terminal for range expiry or blocking world geometry. */
	static bool FinishFlight(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile);

	/** Closes a carrier after its action has already become terminal. */
	static bool EndForActionTermination(
		FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile);
};
