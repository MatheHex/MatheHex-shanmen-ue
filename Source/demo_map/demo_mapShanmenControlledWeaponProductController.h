#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponFlightReadModel.h"
#include "demo_mapShanmenControlledWeaponWorldAdapter.h"

class AActor;
class UPrimitiveComponent;
struct FHitResult;
struct FOverlapResult;

/** Product-authored movement values frozen for one physical flying-sword session. */
struct Fdemo_mapShanmenControlledWeaponMotionCapture
{
	float DirectedSpeed = 0.0f;
	FVector OrbitCenterOffset = FVector::ZeroVector;
	FVector OrbitPlaneNormal = FVector::ZeroVector;
	FVector OrbitReferenceAxis = FVector::ZeroVector;
	float OrbitRadius = 0.0f;
	float OrbitAngularSpeedRadiansPerSecond = 0.0f;
	float InitialOrbitPhaseRadians = 0.0f;
	float MaximumStepSeconds = 0.0f;

	bool IsValid() const;
};

/** Auditable result of one swept world-movement sample. */
struct Fdemo_mapShanmenControlledWeaponMovementReceipt
{
	FGuid ActivationId;
	FGuid SourceItemInstanceId;
	int64 CommandSequence = INDEX_NONE;
	FVector Direction = FVector::ZeroVector;
	FVector StartLocation = FVector::ZeroVector;
	FVector RequestedEndLocation = FVector::ZeroVector;
	FVector ActualEndLocation = FVector::ZeroVector;
	float DeltaSeconds = 0.0f;
	bool bMoved = false;
	bool bBlockingHit = false;

	bool IsValid() const;
};

/** Auditable non-swept step from a completed flight back to its orbit anchor. */
struct Fdemo_mapShanmenControlledWeaponReturnMovementReceipt
{
	FGuid ActivationId;
	FGuid SourceItemInstanceId;
	FVector StartLocation = FVector::ZeroVector;
	FVector ReturnAnchor = FVector::ZeroVector;
	FVector RequestedEndLocation = FVector::ZeroVector;
	FVector ActualEndLocation = FVector::ZeroVector;
	float ReturnSpeed = 0.0f;
	float DeltaSeconds = 0.0f;
	bool bPlaced = false;
	bool bMoved = false;
	bool bArrived = false;

	bool IsValid() const;
};

/** Auditable, non-swept preparation-pose sample while the item is Orbiting. */
struct Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt
{
	FGuid ActivationId;
	FGuid SourceItemInstanceId;
	FVector Center = FVector::ZeroVector;
	FVector PlaneNormal = FVector::ZeroVector;
	FVector ReferenceAxis = FVector::ZeroVector;
	float Radius = 0.0f;
	float AngularSpeedRadiansPerSecond = 0.0f;
	float StartPhaseRadians = 0.0f;
	float EndPhaseRadians = 0.0f;
	FVector StartLocation = FVector::ZeroVector;
	FVector RequestedEndLocation = FVector::ZeroVector;
	FVector ActualEndLocation = FVector::ZeroVector;
	float DeltaSeconds = 0.0f;
	bool bPlaced = false;
	bool bMoved = false;

	bool IsValid() const;
};

/**
 * Exact-item proof that logical Orbiting readiness is backed by the current
 * authored world pose. It is evidence only and cannot mitigate an Impact.
 */
class Fdemo_mapShanmenControlledWeaponOrbitDefenseReadinessReceipt
{
public:
	bool IsValid() const;
	const FGuid& GetSnapshotId() const { return SnapshotId; }
	const FShanmenControlledWeaponDefenseReadinessReceipt& GetRuntime() const
	{
		return Runtime;
	}
	const FVector& GetCenter() const { return Center; }
	const FVector& GetPlaneNormal() const { return PlaneNormal; }
	const FVector& GetReferenceAxis() const { return ReferenceAxis; }
	float GetRadius() const { return Radius; }
	float GetPhaseRadians() const { return PhaseRadians; }
	const FVector& GetWeaponLocation() const { return WeaponLocation; }

private:
	friend class Fdemo_mapShanmenControlledWeaponProductController;

	FGuid SnapshotId;
	FShanmenControlledWeaponDefenseReadinessReceipt Runtime;
	FVector Center = FVector::ZeroVector;
	FVector PlaneNormal = FVector::ZeroVector;
	FVector ReferenceAxis = FVector::ZeroVector;
	float Radius = 0.0f;
	float PhaseRadians = 0.0f;
	FVector WeaponLocation = FVector::ZeroVector;
};

/** Product binding failures before a physical flying sword owns a live Session. */
enum class Edemo_mapShanmenControlledWeaponProductStartError : uint8
{
	None,
	CoordinatorNotReady,
	PreparedInvalid,
	MotionInvalid,
	ActorBindingInvalid,
	SourceNotRegistered,
	SourceMismatch,
	SessionStartRejected
};

/** Auditable result for binding one authority-approved item to one world Actor. */
struct Fdemo_mapShanmenControlledWeaponProductStartResult
{
	Edemo_mapShanmenControlledWeaponProductStartError Error =
		Edemo_mapShanmenControlledWeaponProductStartError::CoordinatorNotReady;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;

	bool IsStarted() const
	{
		return Error
			== Edemo_mapShanmenControlledWeaponProductStartError::None
			&& Startup.IsValid()
			&& Active.IsValid();
	}
};

/**
 * Product owner for one real, already-deployed controlled weapon.
 *
 * The caller supplies the source Actor, physical weapon Actor, and authored
 * movement values. This owner keeps exactly one P6.2 Session and contact
 * window, applies swept directed movement, and routes contacts only through
 * the P6.3 World adapter. It does not spawn an item, bind input, choose contact
 * cadence, mutate inventory, or write vitality directly.
 */
class Fdemo_mapShanmenControlledWeaponProductController
{
public:
	static Fdemo_mapShanmenControlledWeaponProductStartResult TryStart(
		const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		AActor* WeaponActor,
		UPrimitiveComponent* WeaponCollisionRoot,
		const Fdemo_mapShanmenControlledWeaponMotionCapture& Motion,
		Fdemo_mapShanmenControlledWeaponProductController& OutController);

	bool IsValid() const;
	bool IsActive() const;
	bool IsOrbiting() const;
	bool IsDirected() const;
	/** Existing Completed terminal state is the sole logical return trigger. */
	bool IsCompletedForReturn() const;
	FVector GetInitialOrbitLocation() const;
	bool IsAtInitialOrbitLocation(
		float Tolerance = KINDA_SMALL_NUMBER) const;
	/** Captures presentation evidence from the current authoritative state. */
	bool TryCaptureFlightReadModel(
		bool bRedeployedThisFrame,
		Fdemo_mapShanmenControlledWeaponFlightReadModel& OutReadModel) const;
	bool HasActiveContactWindow() const;
	bool HasActiveOrbitThreatWindow() const;
	bool HasActiveDirectedContactWindow() const;

	bool TryLaunch(
		int64 ExpectedSequence,
		const FVector& DesiredDirection,
		FShanmenControlledWeaponCommandReceipt& OutReceipt);
	bool TryRedirect(
		int64 ExpectedSequence,
		const FVector& DesiredDirection,
		FShanmenControlledWeaponCommandReceipt& OutReceipt);
	bool TryCaptureOrbitDefenseReadiness(
		Fdemo_mapShanmenControlledWeaponOrbitDefenseReadinessReceipt&
			OutReceipt) const;
	bool IsOrbitDefenseReadinessCurrent(
		const Fdemo_mapShanmenControlledWeaponOrbitDefenseReadinessReceipt&
			Receipt) const;

	/**
	 * Advances the explicit world-space orbit without sweep or hit emission.
	 * Near-threat, defense, and collision semantics remain separate contracts.
	 */
	bool TryAdvanceOrbiting(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt& OutReceipt);
	/** Projects externally sampled overlap evidence without resolving damage. */
	bool TryBeginOrbitThreatWindow(FShanmenWorldHitContext& OutContext);
	Fdemo_mapShanmenControlledWeaponOrbitThreatResult
	ProjectOrbitThreatOverlap(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FOverlapResult& Overlap,
		const FVector& ContactLocation,
		const FVector& ContactNormal);
	bool TryEndOrbitThreatWindow(FShanmenDetectorEmissionReceipt& OutReceipt);
	bool TryEndOrbitThreatWindow();
	bool TryEvaluateOrbitThreatReceipt(
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
		FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const;
	/** Captures canonical world evidence and evaluates the same frozen policy. */
	bool TryEvaluateOrbitThreatActors(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<AActor*>& TargetActors,
		Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult&
			OutEvidence,
		FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const;
	bool TryBuildOrbitThreatPresenceIntents(
		const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
		FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const;

	/** Moves the physical Actor with sweep enabled; contact delivery is explicit. */
	bool TryAdvanceDirected(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponMovementReceipt& OutReceipt,
		FHitResult& OutBlockingHit);
	/** Moves a Completed item toward its canonical anchor without hit emission. */
	bool TryAdvanceCompletedReturn(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponReturnMovementReceipt& OutReceipt);

	bool TryBeginContactWindow(FShanmenWorldHitContext& OutContext);
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult ResolveSweepContact(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FHitResult& Hit);
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult ResolveOverlapContact(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FOverlapResult& Overlap,
		const FVector& ContactLocation,
		const FVector& ContactNormal);
	bool TryEndContactWindow();

	bool TryRecallAndComplete(
		int64 ExpectedSequence,
		FShanmenControlledWeaponCommandReceipt& OutRecall,
		FShanmenActionTransitionReceipt& OutRecovery,
		FShanmenActionTransitionReceipt& OutCompleted);
	bool TryInterrupt(FShanmenActionTransitionReceipt& OutInterrupted);
	void Reset();

	const Fdemo_mapShanmenControlledWeaponSession& GetSession() const
	{
		return Session;
	}
	const Fdemo_mapShanmenControlledWeaponMotionCapture& GetMotion() const
	{
		return Motion;
	}
	AActor* GetSourceActor() const { return SourceActor.Get(); }
	AActor* GetWeaponActor() const { return WeaponActor.Get(); }
	UPrimitiveComponent* GetWeaponCollisionRoot() const
	{
		return WeaponCollisionRoot.Get();
	}
	float GetCurrentOrbitPhaseRadians() const
	{
		return CurrentOrbitPhaseRadians;
	}

private:
	bool CoordinatorMatches(
		const Fdemo_mapCombatRunCoordinator& Coordinator) const;
	bool ContactContextMatchesSession() const;

	FGuid RunId;
	FGuid SourceEntityId;
	TWeakObjectPtr<AActor> SourceActor;
	TWeakObjectPtr<AActor> WeaponActor;
	TWeakObjectPtr<UPrimitiveComponent> WeaponCollisionRoot;
	Fdemo_mapShanmenControlledWeaponMotionCapture Motion;
	float CurrentOrbitPhaseRadians = 0.0f;
	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenWorldHitContext ActiveContactContext;
};
