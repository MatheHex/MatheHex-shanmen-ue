#pragma once

#include "CoreMinimal.h"

/** Player actions that require the sole local action lane in 0.0.10. */
enum class Edemo_mapShanmenPlayerActionKind : uint8
{
	None,
	BasicSword,
	ThrownWeapon,
	SwordQi,
	SpiritEvasion,
	SpiritShield,
	WeaponGuard
};

/** Whether a registered lane owner may be retired by a later action. */
enum class Edemo_mapShanmenPlayerActionClaimPreemption : uint8
{
	None,
	ExactOwner
};

/** One typed, identity-bearing claim projected from an existing product Host. */
struct Fdemo_mapShanmenPlayerActionClaim
{
	Edemo_mapShanmenPlayerActionKind OwningAction =
		Edemo_mapShanmenPlayerActionKind::None;
	FGuid OwnerId;
	Edemo_mapShanmenPlayerActionClaimPreemption Preemption =
		Edemo_mapShanmenPlayerActionClaimPreemption::None;

	static bool TryCreate(
		Edemo_mapShanmenPlayerActionKind OwningAction,
		const FGuid& OwnerId,
		Edemo_mapShanmenPlayerActionClaimPreemption Preemption,
		Fdemo_mapShanmenPlayerActionClaim& OutClaim);

	bool IsValid() const;
	bool RequiresExactOwnerPreemption() const;
};

/**
 * Read-only projection assembled from existing product Hosts at one route
 * boundary. It stores no lifecycle state and rejects duplicate product kinds.
 */
struct Fdemo_mapShanmenPlayerActionOccupancySnapshot
{
	bool TryRegisterClaim(
		Edemo_mapShanmenPlayerActionKind OwningAction,
		const FGuid& OwnerId,
		Edemo_mapShanmenPlayerActionClaimPreemption Preemption);
	void Invalidate();

	bool IsValid() const;
	int32 NumOccupiedProducts() const;
	const Fdemo_mapShanmenPlayerActionClaim* GetSoleClaim() const;

private:
	TArray<Fdemo_mapShanmenPlayerActionClaim> Claims;
	bool bProjectionValid = true;
};

enum class Edemo_mapShanmenPlayerActionArbitrationStatus : uint8
{
	Rejected,
	Granted,
	WeaponGuardPreemptionRequired,
	AlreadyActive
};

enum class Edemo_mapShanmenPlayerActionArbitrationError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidRequestedAction,
	InvalidOccupancySnapshot,
	SequenceExhausted,
	IdentityConstructionFailed,
	MultipleActiveProducts,
	ConflictingProductActive
};

/**
 * Run-scoped typed decision for one product start attempt.
 *
 * It owns no Host state. A policy rejection is still assigned deterministic
 * identity after all structural preconditions pass, so conflict telemetry is
 * replayable without pretending that an action started.
 */
struct Fdemo_mapShanmenPlayerActionArbitrationReceipt
{
	Edemo_mapShanmenPlayerActionArbitrationStatus Status =
		Edemo_mapShanmenPlayerActionArbitrationStatus::Rejected;
	Edemo_mapShanmenPlayerActionArbitrationError Error =
		Edemo_mapShanmenPlayerActionArbitrationError::None;
	Edemo_mapShanmenPlayerActionKind RequestedAction =
		Edemo_mapShanmenPlayerActionKind::None;
	uint64 CommandSequence = 0;
	FGuid RunId;
	FGuid PlayerEntityId;
	FGuid CommandId;
	Edemo_mapShanmenPlayerActionKind OccupyingAction =
		Edemo_mapShanmenPlayerActionKind::None;
	FGuid OccupyingOwnerId;
	FString Diagnostic;

	bool IsValid() const;
	bool IsAuthorized() const;
	bool RequiresWeaponGuardPreemption() const;
};

enum class Edemo_mapShanmenPlayerActionGateStatus : uint8
{
	Rejected,
	Authorized,
	WeaponGuardPreempted
};

enum class Edemo_mapShanmenPlayerActionGateError : uint8
{
	None,
	ArbitrationRejected,
	WeaponGuardPreemptionRejected,
	PostPreemptionProjectionInvalid,
	PostPreemptionLaneOccupied,
	StateDesynchronized
};

/** What the route observed after an exact Guard Host was retired. */
enum class Edemo_mapShanmenPlayerActionPostPreemptionObservation : uint8
{
	NotObserved,
	Empty,
	Occupied,
	Invalid
};

/** Product-route proof that any required Guard preemption really completed. */
struct Fdemo_mapShanmenPlayerActionGateResult
{
	Edemo_mapShanmenPlayerActionGateStatus Status =
		Edemo_mapShanmenPlayerActionGateStatus::Rejected;
	Edemo_mapShanmenPlayerActionGateError Error =
		Edemo_mapShanmenPlayerActionGateError::ArbitrationRejected;
	Fdemo_mapShanmenPlayerActionArbitrationReceipt Arbitration;
	FGuid RetiredWeaponGuardHostId;
	Edemo_mapShanmenPlayerActionPostPreemptionObservation
		PostPreemptionObservation =
			Edemo_mapShanmenPlayerActionPostPreemptionObservation::NotObserved;
	FString Diagnostic;

	static Fdemo_mapShanmenPlayerActionGateResult FromArbitration(
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt);
	static Fdemo_mapShanmenPlayerActionGateResult FromGuardPreemption(
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt,
		const FGuid& RetiredHostId,
		const Fdemo_mapShanmenPlayerActionOccupancySnapshot&
			PostPreemptionOccupancy);
	static Fdemo_mapShanmenPlayerActionGateResult RejectGuardPreemption(
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt,
		const TCHAR* Diagnostic);

	bool IsValid() const;
	bool IsAuthorized() const;
};

/** Pure arbitration policy; the Combat Run supplies identity and sequencing. */
struct Fdemo_mapShanmenPlayerActionArbitrationPolicy
{
	static FName CanonicalCommandDefinitionId();
	static Fdemo_mapShanmenPlayerActionArbitrationReceipt Evaluate(
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		uint64 CommandSequence,
		Edemo_mapShanmenPlayerActionKind RequestedAction,
		const Fdemo_mapShanmenPlayerActionOccupancySnapshot& Occupancy);
};
