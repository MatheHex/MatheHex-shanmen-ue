#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponWorldAdapter.h"

class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Result of creating one collision-inert thrown-item carrier in a World. */
enum class Edemo_mapShanmenThrownWeaponSpawnError : uint8
{
	None,
	WorldUnavailable,
	SourceInvalid,
	ClassInvalid,
	OriginInvalid,
	SpawnRejected,
	CarrierInvalid
};

struct Fdemo_mapShanmenThrownWeaponSpawnResult
{
	Edemo_mapShanmenThrownWeaponSpawnError Error =
		Edemo_mapShanmenThrownWeaponSpawnError::WorldUnavailable;
	TWeakObjectPtr<Ademo_mapShanmenThrownWeaponProjectile> Projectile;

	bool IsSpawned() const;
};

/** Product lifecycle of one host-owned, one-shot physical activation. */
enum class Edemo_mapShanmenThrownWeaponHostState : uint8
{
	Empty,
	InFlight,
	Terminal
};

enum class Edemo_mapShanmenThrownWeaponHostStartError : uint8
{
	None,
	HostBusy,
	BindingInvalid,
	RangeInvalid,
	SpawnRejected,
	LaunchRejected,
	AdoptionRejected
};

enum class Edemo_mapShanmenThrownWeaponTerminalKind : uint8
{
	None,
	Impact,
	BlockingMiss,
	RangeExpired,
	Interrupted
};

/** Immutable terminal audit retained after the transient Actor is destroyed. */
struct Fdemo_mapShanmenThrownWeaponTerminalReceipt
{
	Edemo_mapShanmenThrownWeaponTerminalKind Kind =
		Edemo_mapShanmenThrownWeaponTerminalKind::None;
	FGuid LaunchId;
	Fdemo_mapShanmenThrownWeaponWorldDeliveryResult Delivery;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completion;
	FShanmenActionTransitionReceipt Interruption;

	bool IsValid() const;
};

struct Fdemo_mapShanmenThrownWeaponHostStartResult
{
	Edemo_mapShanmenThrownWeaponHostStartError Error =
		Edemo_mapShanmenThrownWeaponHostStartError::BindingInvalid;
	Fdemo_mapShanmenThrownWeaponSpawnResult Spawn;
	Fdemo_mapShanmenThrownWeaponLaunchResult Launch;
	FGuid LaunchId;

	bool IsStarted() const
	{
		return Error == Edemo_mapShanmenThrownWeaponHostStartError::None
			&& LaunchId.IsValid();
	}
};

/**
 * Product host for one straight, one-shot thrown item.
 *
 * The host owns the action/execution pair, binds the projectile's native
 * contact and range-expiry delegates, and routes every terminal through the
 * P7.2 adapter. It stores only a non-owning coordinator pointer; the run owner
 * must keep that coordinator alive until this host is terminal or reset.
 * The host is GameThread-owned; destruction of an active host fails closed by
 * interrupting its flight and destroying an owned World carrier.
 */
class Fdemo_mapShanmenThrownWeaponRunHost
{
public:
	Fdemo_mapShanmenThrownWeaponRunHost() = default;
	~Fdemo_mapShanmenThrownWeaponRunHost();

	Fdemo_mapShanmenThrownWeaponRunHost(
		const Fdemo_mapShanmenThrownWeaponRunHost&) = delete;
	Fdemo_mapShanmenThrownWeaponRunHost& operator=(
		const Fdemo_mapShanmenThrownWeaponRunHost&) = delete;
	Fdemo_mapShanmenThrownWeaponRunHost(
		Fdemo_mapShanmenThrownWeaponRunHost&&) = delete;
	Fdemo_mapShanmenThrownWeaponRunHost& operator=(
		Fdemo_mapShanmenThrownWeaponRunHost&&) = delete;

	static Fdemo_mapShanmenThrownWeaponSpawnResult SpawnStagedCarrier(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		AActor* SourceActor,
		const FVector& Origin);

	Fdemo_mapShanmenThrownWeaponHostStartResult TrySpawnAndLaunchPrepared(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenThrownWeaponExecution& Execution,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance);

	/** Launches an already-created Empty carrier; useful for deferred spawn. */
	Fdemo_mapShanmenThrownWeaponHostStartResult TryLaunchPreparedCarrier(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenThrownWeaponExecution& Execution,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance,
		bool bDestroyCarrierOnTerminal);

	/**
	 * Reattaches after durable launch publication without issuing inventory IO.
	 * This is the recovery and deterministic automation seam.
	 */
	bool TryAdoptPublishedFlight(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenThrownWeaponExecution& Execution,
		const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
		Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		float MaximumDistance,
		bool bDestroyCarrierOnTerminal);

	bool TryExpireRange();
	bool TryInterrupt();
	bool Reset();

	bool IsValid() const;
	bool IsInFlight() const
	{
		return State == Edemo_mapShanmenThrownWeaponHostState::InFlight;
	}
	bool IsTerminal() const
	{
		return State == Edemo_mapShanmenThrownWeaponHostState::Terminal;
	}
	Edemo_mapShanmenThrownWeaponHostState GetState() const { return State; }
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const FShanmenThrownWeaponExecution& GetExecution() const
	{
		return Execution;
	}
	Ademo_mapShanmenThrownWeaponProjectile* GetProjectile() const
	{
		return Projectile.Get();
	}
	const Fdemo_mapShanmenThrownWeaponTerminalReceipt& GetTerminalReceipt() const
	{
		return TerminalReceipt;
	}
	float GetMaximumDistance() const { return MaximumDistance; }

private:
	bool ValidateBinding(
		const FShanmenActionOrchestrator& RequestedActionRuntime,
		const FShanmenThrownWeaponExecution& RequestedExecution,
		const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
		const Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
		const Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
		const AActor* RequestedSourceActor,
		float RequestedMaximumDistance) const;
	bool FinishWithoutImpact(
		Edemo_mapShanmenThrownWeaponTerminalKind Kind,
		bool bDestroyNow,
		const Fdemo_mapShanmenThrownWeaponWorldDeliveryResult*
			DeliveryDiagnostic = nullptr);
	bool PublishTerminal(
		Fdemo_mapShanmenThrownWeaponTerminalReceipt&& Receipt,
		bool bDestroyNow);
	void HandleContact(
		Ademo_mapShanmenThrownWeaponProjectile& ContactProjectile,
		const FHitResult& Hit);
	void HandleRangeExpired(
		Ademo_mapShanmenThrownWeaponProjectile& ExpiredProjectile);
	void BindProjectile();
	void UnbindProjectile();
	void DestroyOwnedProjectile();

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenThrownWeaponExecution Execution;
	Fdemo_mapShanmenThrownWeaponItemResult ItemCommit;
	TWeakObjectPtr<Ademo_mapShanmenThrownWeaponProjectile> Projectile;
	TWeakObjectPtr<AActor> SourceActor;
	Fdemo_mapCombatRunCoordinator* Coordinator = nullptr;
	FDelegateHandle ContactHandle;
	FDelegateHandle RangeExpiredHandle;
	Fdemo_mapShanmenThrownWeaponTerminalReceipt TerminalReceipt;
	float MaximumDistance = 0.0f;
	bool bDestroyCarrierOnTerminal = false;
	Edemo_mapShanmenThrownWeaponHostState State =
		Edemo_mapShanmenThrownWeaponHostState::Empty;
};
