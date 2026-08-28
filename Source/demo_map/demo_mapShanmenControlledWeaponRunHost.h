#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
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
 * does not create items or Actors and does not merge their damage identities.
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

	/** Preflights every directed sword, then advances each in stable item order. */
	bool TryAdvanceDirectedInOrder(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponHostMovementBatch& OutBatch);

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
	TArray<FGuid> GetOrderedActiveItemInstanceIds() const;

	FGuid RunId;
	FGuid SourceEntityId;
	TWeakObjectPtr<AActor> SourceActor;
	TMap<FGuid, Fdemo_mapShanmenControlledWeaponProductController> Controllers;
};
