#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "ShanmenControlledWeaponThreatPresenceAuthority.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"
#include "demo_mapShanmenControlledWeaponProductController.h"

/** Run-host failure before another physical controlled weapon can be attached. */
enum class Edemo_mapShanmenControlledWeaponHostAttachError : uint8
{
	None,
	CoordinatorNotReady,
	PreparedInvalid,
	HostBindingMismatch,
	ItemAlreadyBound,
	ActivationAlreadyBound,
	WeaponActorAlreadyBound,
	ProductStartRejected
};

/** Auditable attachment result; Product retains the exact P6.4 start evidence. */
struct Fdemo_mapShanmenControlledWeaponHostAttachResult
{
	Edemo_mapShanmenControlledWeaponHostAttachError Error =
		Edemo_mapShanmenControlledWeaponHostAttachError::CoordinatorNotReady;
	FGuid ItemInstanceId;
	FGuid ActivationId;
	Fdemo_mapShanmenControlledWeaponProductStartResult Product;

	bool IsAttached() const
	{
		return Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::None
			&& ItemInstanceId.IsValid()
			&& ActivationId.IsValid()
			&& Product.IsStarted();
	}
};

/** One deterministic entry from a best-effort physical multi-weapon step. */
struct Fdemo_mapShanmenControlledWeaponHostMovementEntry
{
	FGuid ItemInstanceId;
	bool bAdvanced = false;
	Fdemo_mapShanmenControlledWeaponMovementReceipt Movement;
	FHitResult BlockingHit;

	bool IsSuccessful() const
	{
		return ItemInstanceId.IsValid()
			&& bAdvanced
			&& Movement.IsValid()
			&& Movement.SourceItemInstanceId == ItemInstanceId;
	}
};

/** Ordered receipt for one frame's directed physical weapons. */
struct Fdemo_mapShanmenControlledWeaponHostMovementBatch
{
	int32 AttemptedCount = 0;
	int32 AdvancedCount = 0;
	TArray<Fdemo_mapShanmenControlledWeaponHostMovementEntry> Entries;

	bool IsFullyAdvanced() const;
};

/** Why one fixed-timeline directed-flight pump failed before safe convergence. */
enum class Edemo_mapShanmenControlledWeaponDirectedTimelineError : uint8
{
	None,
	InputInvalid,
	TickBudgetExceeded,
	CoordinatorMismatch,
	HostInvalid,
	MovementRejected,
	ContactLifecycleRejected
};

/** Compact audit for one GameMode-owned batch of canonical 30 Hz flight ticks. */
struct Fdemo_mapShanmenControlledWeaponDirectedTimelineResult
{
	Edemo_mapShanmenControlledWeaponDirectedTimelineError Error =
		Edemo_mapShanmenControlledWeaponDirectedTimelineError::InputInvalid;
	FGuid RunId;
	FGuid TimelineId;
	int64 StartTick = INDEX_NONE;
	int64 EndTick = INDEX_NONE;
	int64 RequestedTickCount = 0;
	int64 MovementTickCount = 0;
	int64 MovementCount = 0;
	int32 BlockingContactCount = 0;
	int32 DeliveredImpactCount = 0;
	int32 TerminalizedCount = 0;
	int32 FallbackInterruptedCount = 0;
	FGuid FailedItemInstanceId;
	FString Diagnostic;

	bool IsSuccess() const;
	bool IsAdvanced() const
	{
		return IsSuccess() && MovementTickCount > 0;
	}
	bool IsNoOp() const
	{
		return IsSuccess() && MovementTickCount == 0;
	}
};

/** One deterministic entry from a best-effort multi-weapon orbit step. */
struct Fdemo_mapShanmenControlledWeaponHostOrbitEntry
{
	FGuid ItemInstanceId;
	bool bAdvanced = false;
	Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt Movement;

	bool IsSuccessful() const
	{
		return ItemInstanceId.IsValid()
			&& bAdvanced
			&& Movement.IsValid()
			&& Movement.SourceItemInstanceId == ItemInstanceId;
	}
};

/** Ordered receipt for one explicit sample of every Orbiting exact item. */
struct Fdemo_mapShanmenControlledWeaponHostOrbitBatch
{
	int32 AttemptedCount = 0;
	int32 AdvancedCount = 0;
	TArray<Fdemo_mapShanmenControlledWeaponHostOrbitEntry> Entries;

	bool IsFullyAdvanced() const;
};

/** One stable-order exact item in a caller-selected defense-readiness set. */
struct Fdemo_mapShanmenControlledWeaponDefenseReadinessEntry
{
	FGuid ItemInstanceId;
	Fdemo_mapShanmenControlledWeaponOrbitDefenseReadinessReceipt Readiness;

	bool IsValid() const
	{
		return ItemInstanceId.IsValid()
			&& Readiness.IsValid()
			&& Readiness.GetRuntime().GetAction()
				.GetSourceItemInstanceId() == ItemInstanceId;
	}
};

/**
 * Atomic read-only snapshot of an explicit item subset in stable GUID order.
 * The caller, not the Host, owns which flying swords participate in defense.
 */
struct Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch
{
	FGuid RunId;
	FGuid SourceEntityId;
	int32 RequestedCount = 0;
	int32 CapturedCount = 0;
	TArray<Fdemo_mapShanmenControlledWeaponDefenseReadinessEntry> Entries;

	bool IsFullyCaptured() const;
};

/** Why one owner-supplied frame delta did or did not advance Orbiting items. */
enum class Edemo_mapShanmenControlledWeaponOrbitFrameStatus : uint8
{
	NoOrbitingItems,
	Advanced,
	DeltaInvalid,
	HostInvalid,
	MovementRejected
};

/** Self-checking owner receipt for one frame-level Orbit pump. */
struct Fdemo_mapShanmenControlledWeaponOrbitFrameResult
{
	Edemo_mapShanmenControlledWeaponOrbitFrameStatus Status =
		Edemo_mapShanmenControlledWeaponOrbitFrameStatus::DeltaInvalid;
	FGuid RunId;
	float DeltaSeconds = 0.0f;
	int32 BoundCount = 0;
	int32 OrbitingCount = 0;
	Fdemo_mapShanmenControlledWeaponHostOrbitBatch Batch;

	bool IsValid() const;
	bool IsAdvanced() const
	{
		return IsValid()
			&& Status
				== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::Advanced;
	}
	bool IsNoOp() const
	{
		return IsValid()
			&& Status
				== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::NoOrbitingItems;
	}
};

/** One caller-owned overlap observation in an explicit Orbit threat sample. */
struct Fdemo_mapShanmenControlledWeaponOrbitThreatContact
{
	FOverlapResult Overlap;
	FVector ContactLocation = FVector::ZeroVector;
	FVector ContactNormal = FVector::ZeroVector;
};

/** Caller-owned contacts for one exact item in a multi-item sample batch. */
struct Fdemo_mapShanmenControlledWeaponThreatSampleRequest
{
	FGuid ItemInstanceId;
	TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact> Contacts;

	bool IsValid() const { return ItemInstanceId.IsValid(); }
};

/**
 * Complete audit for one caller-owned, already-ended Orbit threat sample.
 * No field implies a time cadence or applies a gameplay effect.
 */
class Fdemo_mapShanmenControlledWeaponThreatFinalizationResult
{
public:
	bool IsFinalized() const;
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult&
	GetEvidence() const
	{
		return Evidence;
	}
	const FShanmenControlledWeaponThreatPresenceReceipt& GetPresence() const
	{
		return Presence;
	}
	const FShanmenControlledWeaponThreatPresenceConsumeResult&
	GetConsumption() const
	{
		return Consumption;
	}

private:
	friend class Fdemo_mapShanmenControlledWeaponRunHost;

	FGuid ItemInstanceId;
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Evidence;
	FShanmenControlledWeaponThreatPresenceReceipt Presence;
	FShanmenControlledWeaponThreatPresenceConsumeResult Consumption;
};

/** One stable-order item receipt from an atomic multi-item threat sample. */
struct Fdemo_mapShanmenControlledWeaponThreatSampleBatchEntry
{
	FGuid ItemInstanceId;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Finalization;

	bool IsSuccessful() const;
};

/**
 * Whole-host receipt for an explicit caller-supplied subset of exact items.
 * Entries are strictly ordered by item GUID and commit all-or-nothing.
 */
struct Fdemo_mapShanmenControlledWeaponThreatSampleBatch
{
	FGuid RunId;
	int32 AttemptedCount = 0;
	int32 FinalizedCount = 0;
	TArray<Fdemo_mapShanmenControlledWeaponThreatSampleBatchEntry> Entries;

	bool IsFullyFinalized() const;
};

/** One item-scoped terminal receipt from an atomic host-wide interrupt. */
struct Fdemo_mapShanmenControlledWeaponHostInterruptReceipt
{
	FGuid ItemInstanceId;
	FShanmenActionTransitionReceipt Interrupted;

	bool IsValid() const
	{
		return ItemInstanceId.IsValid()
			&& Interrupted.IsValid()
			&& Interrupted.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted;
	}
};

/**
 * Run-scoped product owner for several real controlled-weapon items.
 *
 * Every exact ItemInstanceId owns one P6.4 Controller and one physical Actor.
 * Stable GUID ordering, rather than attach order or pointer address, controls
 * batch movement and host-wide interruption. Each sword keeps independent
 * commands, contact windows, impact ledger, and terminal lifecycle. The host
 * owns one Run-scoped threat-presence consumption authority, but does not
 * create items or Actors and does not merge their damage identities.
 */
class Fdemo_mapShanmenControlledWeaponRunHost
{
public:
	Fdemo_mapShanmenControlledWeaponHostAttachResult TryAttach(
		const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		AActor* WeaponActor,
		UPrimitiveComponent* WeaponCollisionRoot,
		const Fdemo_mapShanmenControlledWeaponMotionCapture& Motion);

	bool IsEmpty() const { return Controllers.IsEmpty(); }
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	int32 NumBound() const { return Controllers.Num(); }
	int32 NumActive() const;
	int32 NumOrbiting() const;
	TArray<FGuid> GetOrderedItemInstanceIds() const;

	const Fdemo_mapShanmenControlledWeaponProductController* FindController(
		const FGuid& ItemInstanceId) const;

	bool TryLaunch(
		const FGuid& ItemInstanceId,
		int64 ExpectedSequence,
		const FVector& DesiredDirection,
		FShanmenControlledWeaponCommandReceipt& OutReceipt);
	bool TryRedirect(
		const FGuid& ItemInstanceId,
		int64 ExpectedSequence,
		const FVector& DesiredDirection,
		FShanmenControlledWeaponCommandReceipt& OutReceipt);

	/** Preflights every Orbiting item, then places each in stable item order. */
	bool TryAdvanceOrbitingInOrder(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponHostOrbitBatch& OutBatch);
	/** Captures only the explicit exact-item subset; no participation policy is inferred. */
	bool TryCaptureOrbitDefenseReadinessInOrder(
		const TArray<FGuid>& ItemInstanceIds,
		Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch& OutBatch) const;
	/** Revalidates logical state, command checkpoint, source anchor, and pose. */
	bool IsOrbitDefenseReadinessCurrent(
		const Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch& Batch) const;
	/** Adapts an owner frame delta into an auditable Orbit advance/no-op. */
	Fdemo_mapShanmenControlledWeaponOrbitFrameResult AdvanceOrbitingFrame(
		float DeltaSeconds);
	bool TryBeginOrbitThreatWindow(
		const FGuid& ItemInstanceId,
		FShanmenWorldHitContext& OutContext);
	Fdemo_mapShanmenControlledWeaponOrbitThreatResult
	ProjectOrbitThreatOverlap(
		const FGuid& ItemInstanceId,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FOverlapResult& Overlap,
		const FVector& ContactLocation,
		const FVector& ContactNormal);
	bool TryEndOrbitThreatWindow(
		const FGuid& ItemInstanceId,
		FShanmenDetectorEmissionReceipt& OutReceipt);
	bool TryEndOrbitThreatWindow(const FGuid& ItemInstanceId);
	bool TryEvaluateOrbitThreatReceipt(
		const FGuid& ItemInstanceId,
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
		FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const;
	bool TryEvaluateOrbitThreatActors(
		const FGuid& ItemInstanceId,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<AActor*>& TargetActors,
		Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult&
			OutEvidence,
		FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const;
	bool TryBuildOrbitThreatPresenceIntents(
		const FGuid& ItemInstanceId,
		const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
		FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const;
	bool TryConsumeOrbitThreatPresence(
		const FGuid& ItemInstanceId,
		const FShanmenControlledWeaponThreatPresenceReceipt& Presence,
		FShanmenControlledWeaponThreatPresenceConsumeResult& OutResult);
	/**
	 * Runs Begin/Project/End/Finalize on a complete Host candidate. Any rejected
	 * contact or evidence step rolls back the sample ordinal and authority.
	 * The caller remains the sole owner of cadence and overlap collection.
	 */
	bool TrySampleOrbitThreat(
		const FGuid& ItemInstanceId,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>&
			Contacts,
		Fdemo_mapShanmenControlledWeaponThreatFinalizationResult& OutResult);
	/**
	 * Samples an explicit item subset in stable GUID order on one Host candidate.
	 * Any failed item discards every ordinal, checkpoint, and authority mutation.
	 */
	bool TrySampleOrbitThreatsInOrder(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>&
			Requests,
		Fdemo_mapShanmenControlledWeaponThreatSampleBatch& OutBatch);
	/**
	 * Atomically closes evidence, policy, presence, and consumption for one
	 * explicit completed sample. The caller remains the sole cadence owner.
	 */
	bool TryFinalizeOrbitThreatSample(
		const FGuid& ItemInstanceId,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<AActor*>& TargetActors,
		Fdemo_mapShanmenControlledWeaponThreatFinalizationResult& OutResult);
	int64 NumConsumedThreatPresenceIntents() const
	{
		return ThreatPresenceAuthority.NumConsumedIntents();
	}
	const FShanmenControlledWeaponThreatPresenceAuthority&
	GetThreatPresenceAuthority() const
	{
		return ThreatPresenceAuthority;
	}

	/** Preflights every directed sword, then advances each in stable item order. */
	bool TryAdvanceDirectedInOrder(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponHostMovementBatch& OutBatch);
	/**
	 * Consumes one bounded set of already-advanced canonical Run ticks.
	 * Blocking sweeps use the existing contact/vitality path, then converge the
	 * exact item to its existing terminal lifecycle. No independent clock lives
	 * here and terminal items remain bound for normal Run teardown.
	 */
	Fdemo_mapShanmenControlledWeaponDirectedTimelineResult
	AdvanceDirectedFixedTicks(
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		int64 AdvancedTicks,
		Fdemo_mapCombatRunCoordinator& Coordinator);

	bool TryBeginContactWindow(
		const FGuid& ItemInstanceId,
		FShanmenWorldHitContext& OutContext);
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult ResolveSweepContact(
		const FGuid& ItemInstanceId,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FHitResult& Hit);
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult ResolveOverlapContact(
		const FGuid& ItemInstanceId,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FOverlapResult& Overlap,
		const FVector& ContactLocation,
		const FVector& ContactNormal);
	bool TryEndContactWindow(const FGuid& ItemInstanceId);

	bool TryRecallAndComplete(
		const FGuid& ItemInstanceId,
		int64 ExpectedSequence,
		FShanmenControlledWeaponCommandReceipt& OutRecall,
		FShanmenActionTransitionReceipt& OutRecovery,
		FShanmenActionTransitionReceipt& OutCompleted);
	bool TryInterrupt(
		const FGuid& ItemInstanceId,
		FShanmenActionTransitionReceipt& OutInterrupted);
	/** Interrupts every active item atomically in stable item order. */
	bool TryInterruptAll(
		TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt>& OutReceipts);
	/** Removes only a terminal item; removing the last item resets the Run binding. */
	bool TryRemoveTerminal(const FGuid& ItemInstanceId);
	void Reset();

private:
	bool CoordinatorMatches(
		const Fdemo_mapCombatRunCoordinator& Coordinator) const;
	bool BindingMatches(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const AActor* RequestedSourceActor,
		const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared) const;
	bool HasWeaponBinding(
		const AActor* WeaponActor,
		const UPrimitiveComponent* CollisionRoot) const;
	bool PresenceMatchesController(
		const FGuid& ItemInstanceId,
		const FShanmenControlledWeaponThreatPresenceReceipt& Presence) const;
	TArray<FGuid> GetOrderedActiveItemInstanceIds() const;

	FGuid RunId;
	FGuid SourceEntityId;
	TWeakObjectPtr<AActor> SourceActor;
	TMap<FGuid, Fdemo_mapShanmenControlledWeaponProductController> Controllers;
	FShanmenControlledWeaponThreatPresenceAuthority ThreatPresenceAuthority;
};
