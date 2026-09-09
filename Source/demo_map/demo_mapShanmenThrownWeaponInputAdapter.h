#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponProductLifecycle.h"

class AActor;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Outcome of routing one physical hotbar press at the product boundary. */
enum class Edemo_mapShanmenThrownWeaponInputStatus : uint8
{
	PassThrough,
	Applied,
	InvalidSlot,
	SnapshotUnavailable,
	SnapshotStale,
	ItemEvidenceRejected,
	ProductRunMismatch,
	ProductTrajectoryMismatch,
	SourceUnavailable,
	AimUnavailable,
	TargetUnavailable,
	ApexClearanceUnavailable,
	SelectionSequenceExhausted,
	IntentCaptureRejected,
	ActionConflict,
	ProductRejected
};

/** Audit evidence for one hotbar routing decision. */
struct Fdemo_mapShanmenThrownWeaponInputResult
{
	Edemo_mapShanmenThrownWeaponInputStatus Status =
		Edemo_mapShanmenThrownWeaponInputStatus::PassThrough;
	int32 HotbarSlotNumber = INDEX_NONE;
	int32 AuthorityRevision = INDEX_NONE;
	uint64 SelectionOrdinal = 0;
	FGuid RunId;
	FGuid ItemInstanceId;
	FGuid SelectionId;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	bool bAimSampled = false;
	bool bTargetSampled = false;
	bool bApexClearanceSampled = false;
	bool bUsedSkeletalHandOrigin = false;
	Fdemo_mapShanmenPlayerActionGateResult ActionGate;
	Fdemo_mapShanmenThrownWeaponSessionResult Session;
	FString Diagnostic;

	bool ShouldPassThrough() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponInputStatus::PassThrough;
	}
	bool IsHandled() const { return !ShouldPassThrough(); }
	bool IsAccepted() const
	{
		return Status == Edemo_mapShanmenThrownWeaponInputStatus::Applied
			&& ActionGate.IsAuthorized()
			&& Session.IsAccepted();
	}
};

/**
 * The sole device-to-product bridge for active-Run thrown weapons.
 *
 * It first resolves the frozen hotbar item from durable ShanmenItems evidence.
 * Non-thrown items pass through without sampling transform or aim. A typed
 * thrown weapon captures one deterministic SelectionId and immutable intent,
 * then delegates to the already-bound P7.8 lifecycle. The adapter owns only a
 * transient input ordinal; it stores no Run, hotbar, item, aim, or world truth.
 */
class Fdemo_mapShanmenThrownWeaponInputAdapter
{
public:
	/** Shared world-up release height used by input geometry and Arc preview. */
	static constexpr double GetLaunchOriginHeight() { return 50.0; }
	/** Forward clearance moves the visible knife ahead of the source centerline. */
	static constexpr double GetLaunchOriginForwardOffset() { return 55.0; }
	/** Positive local-right offset presents the release from the weapon hand. */
	static constexpr double GetLaunchOriginRightOffset() { return 28.0; }
	/** Forward clearance keeps a hand-sourced knife center outside the palm. */
	static constexpr double GetSkeletalHandForwardClearance() { return 18.0; }
	/** Reject malformed pose samples that place the hand outside the character. */
	static constexpr double GetMaximumSkeletalHandDistance() { return 200.0; }
	/** Canonical proxy release point used when no trustworthy hand pose exists. */
	static FVector MakeLaunchOrigin(const FTransform& SourceTransform);
	/** Right-hand pose first, with the canonical proxy as a fail-safe fallback. */
	static FVector ResolveLaunchOrigin(
		AActor* SourceActor,
		bool* bOutUsedSkeletalHandOrigin = nullptr);
	/** Stable event identity; equal canonical inputs always reproduce one GUID. */
	static FGuid MakeSelectionId(
		const FGuid& CorrelationId,
		const FGuid& RunId,
		const FGuid& ItemInstanceId,
		int32 HotbarSlotNumber,
		int32 AuthorityRevision,
		uint64 SelectionOrdinal);

	Fdemo_mapShanmenThrownWeaponInputResult RouteHotbarInput(
		Udemo_mapShanmenItemAuthoritySubsystem* Authority,
		Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		AActor* SourceActor,
		int32 HotbarSlotNumber,
		TFunctionRef<FVector()> SampleAimDirection,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);
	/** Platform-neutral Arc request; target and apex are sampled at most once. */
	Fdemo_mapShanmenThrownWeaponInputResult RouteArcHotbarInput(
		Udemo_mapShanmenItemAuthoritySubsystem* Authority,
		Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		AActor* SourceActor,
		int32 HotbarSlotNumber,
		TFunctionRef<FVector()> SampleTarget,
		TFunctionRef<double()> SampleApexClearance,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

	void Reset() { NextSelectionOrdinal = 1; }
	uint64 GetNextSelectionOrdinal() const { return NextSelectionOrdinal; }

private:
	Fdemo_mapShanmenThrownWeaponInputResult RouteTypedHotbarInput(
		Udemo_mapShanmenItemAuthoritySubsystem* Authority,
		Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		AActor* SourceActor,
		int32 HotbarSlotNumber,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		TFunctionRef<FVector()> SamplePrimaryGeometry,
		TFunctionRef<double()> SampleArcApexClearance,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

	uint64 NextSelectionOrdinal = 1;
};
