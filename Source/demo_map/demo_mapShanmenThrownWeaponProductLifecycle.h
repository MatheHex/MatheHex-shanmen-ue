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

	/** Straight compatibility capture for the real TrainingThrowingKnife. */
	static bool TryCaptureTrainingThrowingKnifeConfig(
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig,
		FString& OutDiagnostic);
	/** Captures canonical Straight or Arc content; callers cannot inject stats. */
	static bool TryCaptureTrainingThrowingKnifeConfig(
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig,
		FString& OutDiagnostic);

	/** Straight compatibility bind. */
	bool TryBegin(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		AActor& SourceActor,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);
	/** Explicit typed bind using only canonical lifecycle-owned content. */
	bool TryBegin(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		AActor& SourceActor,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
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
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const
	{
		return Session.GetTrajectoryKind();
	}
	int32 NumCapturedSelections() const
	{
		return Session.NumCapturedSelections();
	}
	Edemo_mapShanmenThrownWeaponHostState GetHostState() const
	{
		return Session.GetHostState();
	}
	/** Read-only terminal evidence retained after the transient carrier ends. */
	const Fdemo_mapShanmenThrownWeaponTerminalReceipt& GetTerminalReceipt() const
	{
		return Session.GetTerminalReceipt();
	}
	/** Latest immutable hotbar/recovery audit; never a second gameplay owner. */
	const Fdemo_mapShanmenThrownWeaponSessionResult&
	GetLastHotbarRouteResult() const
	{
		return LastHotbarRouteResult;
	}
	const FGuid& GetOccupancyOwnerId() const
	{
		return Session.GetOccupancyOwnerId();
	}
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* FindCapturedCommand(
		const FGuid& SelectionId) const
	{
		return Session.FindCapturedCommand(SelectionId);
	}
	/** Copies one immutable hotbar/config snapshot without product mutation. */
	bool TryCaptureReadOnlyHotbarBinding(
		int32 HotbarSlotNumber,
		FGuid& OutItemInstanceId,
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig) const;

private:
	Fdemo_mapShanmenThrownWeaponSessionResult RejectUnavailable(
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent,
		const TCHAR* Diagnostic) const;

	TWeakObjectPtr<Udemo_mapShanmenItemAuthoritySubsystem> BoundAuthority;
	Fdemo_mapShanmenThrownWeaponProductSession Session;
	Fdemo_mapShanmenThrownWeaponSessionResult LastHotbarRouteResult;
};
