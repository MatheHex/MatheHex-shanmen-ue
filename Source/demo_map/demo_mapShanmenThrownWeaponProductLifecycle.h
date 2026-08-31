#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponProductSession.h"

class AActor;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/**
 * Product-lifetime owner for the P7.6 thrown-weapon session.
 *
 * The durable ShanmenItems ledger remains the sole Run/loadout authority. This
 * adapter reconstructs that correlation at begin, proves the source Actor is
 * the CombatRunCoordinator's player, and then exposes only device-independent
 * intents. It owns no Tick, timer, key binding, UI state, or duplicate Run id.
 */
class Fdemo_mapShanmenThrownWeaponProductLifecycle
{
public:
	Fdemo_mapShanmenThrownWeaponProductLifecycle() = default;
	~Fdemo_mapShanmenThrownWeaponProductLifecycle() = default;

	Fdemo_mapShanmenThrownWeaponProductLifecycle(
		const Fdemo_mapShanmenThrownWeaponProductLifecycle&) = delete;
	Fdemo_mapShanmenThrownWeaponProductLifecycle& operator=(
		const Fdemo_mapShanmenThrownWeaponProductLifecycle&) = delete;
	Fdemo_mapShanmenThrownWeaponProductLifecycle(
		Fdemo_mapShanmenThrownWeaponProductLifecycle&&) = delete;
	Fdemo_mapShanmenThrownWeaponProductLifecycle& operator=(
		Fdemo_mapShanmenThrownWeaponProductLifecycle&&) = delete;

	/** Captures the first real product policy for TrainingThrowingKnife. */
	static bool TryCaptureTrainingThrowingKnifeConfig(
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig,
		FString& OutDiagnostic);

	bool TryBegin(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		AActor& SourceActor,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponSessionResult TrySubmitHotbar(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent);

	Fdemo_mapShanmenThrownWeaponSessionResult TryRecoverCancellation(
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent);

	bool TryInterruptFlight();
	bool TryExpireRange();
	/** Interrupts an active flight, but preserves unresolved durable recovery. */
	bool TryEnd(FString& OutDiagnostic);

	bool IsActive() const { return Session.IsActive(); }
	bool IsValid() const;
	bool IsEmpty() const { return !IsActive() && IsValid(); }
	const FGuid& GetRunId() const { return Session.GetRunId(); }
	int32 NumCapturedSelections() const
	{
		return Session.NumCapturedSelections();
	}
	Edemo_mapShanmenThrownWeaponHostState GetHostState() const
	{
		return Session.GetHostState();
	}
	const FGuid& GetOccupancyOwnerId() const
	{
		return Session.GetOccupancyOwnerId();
	}

private:
	Fdemo_mapShanmenThrownWeaponSessionResult RejectUnavailable(
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent,
		const TCHAR* Diagnostic) const;

	TWeakObjectPtr<Udemo_mapShanmenItemAuthoritySubsystem> BoundAuthority;
	Fdemo_mapShanmenThrownWeaponProductSession Session;
};
