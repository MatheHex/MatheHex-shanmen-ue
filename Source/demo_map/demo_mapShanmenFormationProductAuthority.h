#pragma once

#include "CoreMinimal.h"
#include "ShanmenFormationDeployment.h"
#include "demo_mapShanmenRunCorrelation.h"

class Fdemo_mapCombatRunCoordinator;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Run-issued identity and frozen action for one exact formation deployment. */
class Fdemo_mapPlayerFormationActionReservation
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

/**
 * Frozen, authority-correlated formation deployment command.
 *
 * The command joins the durable item ActiveRun, the combat Run's player
 * entity, the authored diagram and one sampled transform. It performs no item
 * or World mutation and can be passed directly to the existing ProductHost.
 */
class Fdemo_mapShanmenFormationDeploymentCommand
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapPlayerFormationActionReservation& Reservation,
		const FShanmenFormationDiagramDefinition& Diagram,
		const FVector& Origin,
		const FVector& Forward,
		Fdemo_mapShanmenFormationDeploymentCommand& OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationDeploymentCommand& Other) const;
	const FGuid& GetCommandId() const { return Reservation.GetActivationId(); }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	const Fdemo_mapPlayerFormationActionReservation& GetReservation() const
	{
		return Reservation;
	}
	const FShanmenCombatActionSnapshot& GetAction() const
	{
		return Reservation.GetAction();
	}
	const FShanmenFormationDiagramDefinition& GetDiagram() const
	{
		return Diagram;
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetForward() const { return Forward; }

private:
	Fdemo_mapShanmenRunCorrelation Correlation;
	Fdemo_mapPlayerFormationActionReservation Reservation;
	FShanmenFormationDiagramDefinition Diagram;
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ZeroVector;
};

enum class Edemo_mapShanmenFormationProductPreparationStatus : uint8
{
	Ready,
	InvalidDiagram,
	InvalidOrigin,
	InvalidDirection,
	AuthorityNotReady,
	CorrelationUnavailable,
	SnapshotUnavailable,
	AuthorityCorrelationMismatch,
	CoordinatorNotReady,
	ReservationRejected,
	CommandRejected
};

/** Complete read-only proof joining Items authority, combat Run and deployment. */
struct Fdemo_mapShanmenFormationProductPreparationResult
{
	Edemo_mapShanmenFormationProductPreparationStatus Status =
		Edemo_mapShanmenFormationProductPreparationStatus::AuthorityNotReady;
	FString Diagnostic;
	int32 AuthorityRevision = INDEX_NONE;
	Fdemo_mapShanmenRunCorrelation Correlation;
	Fdemo_mapPlayerFormationActionReservation Reservation;
	Fdemo_mapShanmenFormationDeploymentCommand Command;

	bool IsReady() const;
};

/**
 * Product composition boundary before formation ProductHost ownership.
 *
 * All externally sampled geometry and durable correlation are validated before
 * the combat Run sequence is consumed. This boundary never scans the World,
 * mutates inventory, places Actors or invents formation tuning values.
 */
struct Fdemo_mapShanmenFormationProductAuthority
{
	static FGuid MakeActivationEnergyTransactionId(
		const Fdemo_mapShanmenFormationDeploymentCommand& Command);
	static FGuid MakeActivationEnergyCommandId(
		const Fdemo_mapShanmenFormationDeploymentCommand& Command);

	static Fdemo_mapShanmenFormationProductPreparationResult PrepareDeployment(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenFormationDiagramDefinition& Diagram,
		const FVector& Origin,
		const FVector& Forward);
};
