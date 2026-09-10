#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritShieldSession.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"
#include "demo_mapShanmenDivineSenseProductController.h"
#include "demo_mapShanmenPlayerActionArbitration.h"

class Fdemo_mapCombatRunCoordinator;

/** Run-issued identity for one canonical Spirit Shield activation. */
class Fdemo_mapPlayerSpiritShieldActionReservation
{
public:
	bool IsValid() const;
	uint64 GetActivationSequence() const { return ActivationSequence; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FGuid& GetActivationId() const { return Action.GetActivationId(); }

private:
	friend class Fdemo_mapCombatRunCoordinator;

	uint64 ActivationSequence = 0;
	FShanmenCombatActionSnapshot Action;
};

/** P25.1 prototype policy. These are versioned tuning values, not final balance. */
struct Fdemo_mapShanmenSpiritShieldProductAuthority
{
	static FName CanonicalContentVersion();
	static FString CanonicalContentDigest();
	static FName CanonicalRuleId();
	static FName CanonicalCostRuleId();
	static float CanonicalMaximumCapacity();
	static float CanonicalSpiritEnergyCost();
	static int64 CanonicalDurationTicks();
	static bool TryCreateCanonicalDefinition(
		FShanmenSpiritShieldDefinition& OutDefinition);
	static bool TryCreateCanonicalCost(FShanmenActionResourceCost& OutCost);
	static FGuid MakeTransactionId(
		const Fdemo_mapPlayerSpiritShieldActionReservation& Reservation);
	static FGuid MakeCommandId(
		const Fdemo_mapPlayerSpiritShieldActionReservation& Reservation);
};

enum class Edemo_mapShanmenSpiritShieldProductActivationStatus : uint8
{
	Rejected,
	Activated
};

enum class Edemo_mapShanmenSpiritShieldProductActivationError : uint8
{
	None,
	AlreadyActive,
	CoordinatorNotReady,
	TimelineUnavailable,
	ActionConflict,
	ReservationRejected,
	PolicyConstructionFailed,
	ScheduleRejected,
	SharedResourceRejected,
	SessionRejected,
	StateDesynchronized
};

/** Atomic proof that input, Run identity, shared energy and shield state agree. */
struct Fdemo_mapShanmenSpiritShieldProductActivationResult
{
	Edemo_mapShanmenSpiritShieldProductActivationStatus Status =
		Edemo_mapShanmenSpiritShieldProductActivationStatus::Rejected;
	Edemo_mapShanmenSpiritShieldProductActivationError Error =
		Edemo_mapShanmenSpiritShieldProductActivationError::
			CoordinatorNotReady;
	FString Diagnostic;
	Fdemo_mapShanmenPlayerActionGateResult ActionGate;
	Fdemo_mapPlayerSpiritShieldActionReservation Reservation;
	FShanmenSpiritShieldSchedule Schedule;
	FShanmenSpiritShieldActionResult Begin;
	FShanmenSpiritShieldActionResult Commit;
	Fdemo_mapShanmenSharedSpiritEnergyTransactionResult SharedResource;

	bool IsValid() const;
	bool IsAccepted() const;
};

enum class Edemo_mapShanmenSpiritShieldProductTimelineStatus : uint8
{
	Rejected,
	Waiting,
	Closed,
	AlreadyClosed
};

enum class Edemo_mapShanmenSpiritShieldProductTimelineError : uint8
{
	None,
	SessionNotActive,
	TimelineMismatch,
	ObservationRejected,
	StateDesynchronized
};

enum class Edemo_mapShanmenSpiritShieldImpactDefenseStatus : uint8
{
	Rejected,
	Composed
};

enum class Edemo_mapShanmenSpiritShieldImpactDefenseError : uint8
{
	None,
	SessionNotActive,
	InvalidImpact,
	InvalidTimelineSample,
	TimelineMismatch,
	OutsideActiveWindow,
	BaseDefenseInvalid,
	ProjectionRejected,
	LayerConflict,
	SnapshotCompositionRejected
};

/** Append-only proof that the active shield projected into one Impact. */
struct Fdemo_mapShanmenSpiritShieldImpactDefenseResult
{
	Edemo_mapShanmenSpiritShieldImpactDefenseStatus Status =
		Edemo_mapShanmenSpiritShieldImpactDefenseStatus::Rejected;
	Edemo_mapShanmenSpiritShieldImpactDefenseError Error =
		Edemo_mapShanmenSpiritShieldImpactDefenseError::SessionNotActive;
	FString Diagnostic;
	FGuid ImpactId;
	Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
	FShanmenSpiritShieldProjectionReceipt Projection;
	FShanmenDefenseSnapshot Defense;

	bool IsValid() const;
	bool IsSuccess() const;
};

enum class Edemo_mapShanmenSpiritShieldImpactCommitStatus : uint8
{
	Rejected,
	NotTriggered,
	Committed,
	AlreadyCommitted
};

enum class Edemo_mapShanmenSpiritShieldImpactCommitError : uint8
{
	None,
	SessionNotActive,
	InvalidDefenseProof,
	ImpactMismatch,
	ResolutionRejected,
	CapacityRejected,
	DeliveryRejected,
	StateDesynchronized
};

/** Atomic product proof for shield capacity and the caller's Impact delivery. */
struct Fdemo_mapShanmenSpiritShieldImpactCommitResult
{
	Edemo_mapShanmenSpiritShieldImpactCommitStatus Status =
		Edemo_mapShanmenSpiritShieldImpactCommitStatus::Rejected;
	Edemo_mapShanmenSpiritShieldImpactCommitError Error =
		Edemo_mapShanmenSpiritShieldImpactCommitError::SessionNotActive;
	FString Diagnostic;
	Fdemo_mapShanmenSpiritShieldImpactDefenseResult Defense;
	FShanmenSpiritShieldCapacityCommitResult Capacity;
	bool bDeliveryCommitted = false;

	bool IsValid() const;
	bool IsSuccess() const;
	bool DidConsumeCapacity() const;
};

/** Product interpretation of one fixed-Run-timeline sample. */
struct Fdemo_mapShanmenSpiritShieldProductTimelineResult
{
	Edemo_mapShanmenSpiritShieldProductTimelineStatus Status =
		Edemo_mapShanmenSpiritShieldProductTimelineStatus::Rejected;
	Edemo_mapShanmenSpiritShieldProductTimelineError Error =
		Edemo_mapShanmenSpiritShieldProductTimelineError::SessionNotActive;
	FString Diagnostic;
	FShanmenSpiritShieldTimelineObservation Observation;
	FShanmenSpiritShieldDeadlineClosureResult Closure;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Sole product owner for the currently active short Spirit Shield.
 *
 * Activation spends the same Run-scoped SpiritEnergy authority used by Divine
 * Sense. The mutation is staged on copies and publishes the resource ledger
 * and shield session together. Time remains caller-owned through the fixed
 * Combat Run timeline; no Actor, World, Tick or second balance is retained.
 */
class Fdemo_mapShanmenSpiritShieldProductSession
{
public:
	Fdemo_mapShanmenSpiritShieldProductActivationResult TryActivate(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductController& SpiritEnergyController,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

	Fdemo_mapShanmenSpiritShieldProductTimelineResult ObserveTimeline(
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample);
	Fdemo_mapShanmenSpiritShieldImpactDefenseResult TryComposeImpactDefense(
		const FGuid& ImpactId,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		const FShanmenDefenseSnapshot& BaseDefense) const;
	/**
	 * Commits capacity on a candidate Session, invokes the caller-owned delivery,
	 * and publishes the candidate only after both operations succeed.
	 */
	Fdemo_mapShanmenSpiritShieldImpactCommitResult CommitImpact(
		const Fdemo_mapShanmenSpiritShieldImpactDefenseResult& Defense,
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Result,
		TFunctionRef<bool()> CommitDelivery);
	bool TryReleaseOwner(FString& OutDiagnostic);
	bool Reset();

	bool IsValid() const;
	bool IsEmpty() const { return !bHasActivation; }
	bool IsActive() const;
	bool IsClosed() const;
	const Fdemo_mapPlayerSpiritShieldActionReservation& GetReservation() const
	{
		return Reservation;
	}
	const FShanmenSpiritShieldSession& GetSession() const { return Session; }
	const Fdemo_mapShanmenSharedSpiritEnergyTransactionReceipt&
	GetSharedResourceReceipt() const
	{
		return SharedResourceReceipt;
	}
	int64 GetDeadlineTick() const
	{
		return bHasActivation ? Session.GetSchedule().GetDeadlineTick() : INDEX_NONE;
	}
	float GetAvailableCapacity() const
	{
		return IsActive()
			? Session.GetCapacityAuthority().GetAvailableCapacity()
			: 0.0f;
	}

private:
	Fdemo_mapPlayerSpiritShieldActionReservation Reservation;
	FShanmenSpiritShieldSession Session;
	Fdemo_mapShanmenSharedSpiritEnergyTransactionReceipt
		SharedResourceReceipt;
	bool bHasActivation = false;
};
