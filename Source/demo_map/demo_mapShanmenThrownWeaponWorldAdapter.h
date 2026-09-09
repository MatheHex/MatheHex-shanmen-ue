#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionOrchestrator.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenThrownWeaponItemAdapter.h"
#include "demo_mapShanmenThrownWeaponProjectile.h"

class Udemo_mapShanmenItemAuthoritySubsystem;
struct FHitResult;

/** Failure while freezing or durably publishing one physical launch. */
enum class Edemo_mapShanmenThrownWeaponLaunchError : uint8
{
	None,
	RunEvidenceInvalid,
	PreparationInvalid,
	ActionRuntimeInvalid,
	ExecutionInvalid,
	LaunchRejected,
	EmissionRejected,
	ItemCommitRequestRejected,
	ProjectileStageRejected,
	ReleasePathBlocked,
	AuthorityCommitRejected,
	PublicationRejected
};

/**
 * Copy-on-write launch candidate. The live execution remains Ready while this
 * evidence is staged and the physical Actor remains collision-inert.
 */
struct Fdemo_mapShanmenThrownWeaponLaunchPlan
{
	Fdemo_mapShanmenRunCorrelation Correlation;
	Fdemo_mapShanmenThrownWeaponItemResult Preparation;
	Fdemo_mapShanmenThrownWeaponItemResult ItemCommitRequest;
	FShanmenThrownWeaponExecution ExecutionCandidate;
	FShanmenThrownWeaponLaunchReceipt Launch;
	FShanmenWorldHitContext Context;

	bool IsValid() const;
};

/** Auditable result for stage, authority commit, or publication. */
struct Fdemo_mapShanmenThrownWeaponLaunchResult
{
	Edemo_mapShanmenThrownWeaponLaunchError Error =
		Edemo_mapShanmenThrownWeaponLaunchError::RunEvidenceInvalid;
	Fdemo_mapShanmenThrownWeaponLaunchPlan Plan;
	Fdemo_mapShanmenThrownWeaponItemResult ItemCommit;

	bool IsStaged() const
	{
		return Error == Edemo_mapShanmenThrownWeaponLaunchError::None
			&& Plan.IsValid();
	}
	bool IsCommitted() const
	{
		return IsStaged()
			&& ItemCommit.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::Committed
			&& ItemCommit.IsFinalized()
			&& ItemCommit.FinalizeRequest.bCommit;
	}
};

/** Failure while converting one projectile callback into canonical vitality. */
enum class Edemo_mapShanmenThrownWeaponWorldDeliveryError : uint8
{
	None,
	CoordinatorNotReady,
	FlightNotActive,
	ContextMismatch,
	ContactNotResolved,
	TargetVitalityUnavailable,
	CandidateRejected,
	FlightTerminationRejected,
	DeliveryRejected
};

/** Auditable result for one thrown-item projectile contact. */
struct Fdemo_mapShanmenThrownWeaponWorldDeliveryResult
{
	Edemo_mapShanmenThrownWeaponWorldDeliveryError Error =
		Edemo_mapShanmenThrownWeaponWorldDeliveryError::CoordinatorNotReady;
	FGuid TargetEntityId;
	FShanmenThrownWeaponImpactReceipt Impact;
	Fdemo_mapCombatImpactDeliveryResult Delivery;

	bool IsDelivered() const
	{
		return Error
			== Edemo_mapShanmenThrownWeaponWorldDeliveryError::None
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
 * Product bridge for P7 one-shot thrown-item delivery.
 *
 * The launch is copy-on-write until ShanmenItems confirms the exact durable
 * Quantity commit. World contacts then pass through WorldGameplay, the pure
 * thrown resolver, and the existing combat-run vitality authority. No path in
 * this adapter calls ApplyDamage or modifies legacy inventory.
 */
struct Fdemo_mapShanmenThrownWeaponWorldAdapter
{
	/** Backward-compatible P7 straight-flight staging entrypoint. */
	static Fdemo_mapShanmenThrownWeaponLaunchResult StagePreparedLaunch(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		AActor* SourceActor,
		const FVector& Origin,
		const FVector& AimDirection);

	/** Stages one P20 self-validating ballistic plan through the same gate. */
	static Fdemo_mapShanmenThrownWeaponLaunchResult StagePreparedArcLaunch(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		AActor* SourceActor,
		const FShanmenThrownWeaponArcPlan& ArcPlan);

	/** Executes the P7.1 durable commit and publishes only after success. */
	static Fdemo_mapShanmenThrownWeaponLaunchResult CommitStagedLaunch(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenActionOrchestrator& ActionRuntime,
		const Fdemo_mapShanmenThrownWeaponLaunchPlan& Plan,
		FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile);

	/**
	 * Publishes already-durable evidence. This supports recovery/replay without
	 * issuing a second inventory command and is the deterministic test seam.
	 */
	static bool PublishCommittedLaunch(
		const FShanmenActionOrchestrator& ActionRuntime,
		const Fdemo_mapShanmenThrownWeaponLaunchPlan& Plan,
		const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
		FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile);

	static Fdemo_mapShanmenThrownWeaponWorldDeliveryResult
	ResolveProjectileContact(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FHitResult& Hit);

	/** Terminal no-impact path for range expiry or blocking world geometry. */
	static bool FinishFlightWithoutImpact(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile);
};
