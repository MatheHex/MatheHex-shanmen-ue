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
	SourceUnavailable,
	AimUnavailable,
	SelectionSequenceExhausted,
	IntentCaptureRejected,
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
	bool bAimSampled = false;
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
		TFunctionRef<FVector()> SampleAimDirection);

	void Reset() { NextSelectionOrdinal = 1; }
	uint64 GetNextSelectionOrdinal() const { return NextSelectionOrdinal; }

private:
	uint64 NextSelectionOrdinal = 1;
};
