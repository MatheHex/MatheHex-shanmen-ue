#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponActiveRunRoute.h"

class AActor;
class Ademo_mapShanmenControlledWeaponActor;
class UWorld;
class Udemo_mapItemSubsystem;
class Udemo_mapShanmenItemAuthoritySubsystem;
class Fdemo_mapCombatRunCoordinator;

/** Product outcome for projecting the exact prepared weapon into the World. */
enum class Edemo_mapShanmenControlledWeaponWorldStartStatus : uint8
{
	NotApplicable,
	Started,
	LifecycleInvalid,
	LifecycleBusy,
	DependenciesUnavailable,
	CoordinatorNotReady,
	CorrelationUnavailable,
	CorrelationMismatch,
	EvidenceUnavailable,
	WeaponEvidenceInvalid,
	SpawnRejected,
	ActorBindingRejected,
	RouteRejected,
	ResultInvalid
};

/** Auditable start result; NotApplicable is a successful non-flying-sword no-op. */
struct Fdemo_mapShanmenControlledWeaponWorldStartResult
{
	Edemo_mapShanmenControlledWeaponWorldStartStatus Status =
		Edemo_mapShanmenControlledWeaponWorldStartStatus::
			DependenciesUnavailable;
	FGuid RunId;
	FGuid ItemInstanceId;
	TWeakObjectPtr<Ademo_mapShanmenControlledWeaponActor> WeaponActor;
	Fdemo_mapShanmenControlledWeaponActiveRunResult Route;
	FString Diagnostic;

	bool IsAccepted() const
	{
		return Status
				== Edemo_mapShanmenControlledWeaponWorldStartStatus::
					NotApplicable
			|| IsStarted();
	}
	bool IsStarted() const;
};

/**
 * World projection owned by one product combat Run.
 *
 * It reconstructs the existing durable correlation, spawns only the canonical
 * training flying sword, and delegates all combat binding to the P21.1 route.
 * Actor existence is never inventory or Run authority.
 */
struct Fdemo_mapShanmenControlledWeaponWorldLifecycle
{
	Fdemo_mapShanmenControlledWeaponWorldStartResult TryBegin(
		UWorld* World,
		const Udemo_mapShanmenItemAuthoritySubsystem* Authority,
		const Udemo_mapItemSubsystem* Runtime,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		AActor* SourceActor,
		uint64 ActivationSequence);

	/** Physical retirement follows the atomic logical Host -> Coordinator end. */
	bool TryEndAfterRun(
		const FGuid& ExpectedRunId,
		const Fdemo_mapShanmenControlledWeaponRunHost& Host,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsActive() const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	Ademo_mapShanmenControlledWeaponActor* GetWeaponActor() const
	{
		return WeaponActor.Get();
	}
	void Reset();

private:
	void Clear();

	bool bOwnsSpawnedActor = false;
	FGuid RunId;
	FGuid ItemInstanceId;
	TWeakObjectPtr<Ademo_mapShanmenControlledWeaponActor> WeaponActor;
};
