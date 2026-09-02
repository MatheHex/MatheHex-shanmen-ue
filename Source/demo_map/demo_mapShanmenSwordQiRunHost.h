#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordQiWorldAdapter.h"

class UWorld;

/** Result of creating one collision-inert sword-qi carrier in a World. */
enum class Edemo_mapShanmenSwordQiSpawnError : uint8
{
	None,
	WorldUnavailable,
	SourceInvalid,
	ClassInvalid,
	OriginInvalid,
	SpawnRejected,
	CarrierInvalid
};

struct Fdemo_mapShanmenSwordQiSpawnResult
{
	Edemo_mapShanmenSwordQiSpawnError Error =
		Edemo_mapShanmenSwordQiSpawnError::WorldUnavailable;
	TWeakObjectPtr<Ademo_mapShanmenSwordQiProjectile> Projectile;

	bool IsSpawned() const;
};

/** Product lifecycle of one host-owned sword-qi flight. */
enum class Edemo_mapShanmenSwordQiHostState : uint8
{
	Empty,
	InFlight,
	Terminal
};

enum class Edemo_mapShanmenSwordQiHostStartError : uint8
{
	None,
	HostBusy,
	BindingInvalid,
	SpawnRejected,
	LaunchRejected,
	AdoptionRejected
};

/** P18.2 uses a first-blocking-contact terminal policy. */
enum class Edemo_mapShanmenSwordQiTerminalKind : uint8
{
	None,
	Impact,
	BlockingMiss,
	RangeExpired,
	Interrupted
};

/** Immutable terminal audit retained after the transient Actor is destroyed. */
struct Fdemo_mapShanmenSwordQiTerminalReceipt
{
	Edemo_mapShanmenSwordQiTerminalKind Kind =
		Edemo_mapShanmenSwordQiTerminalKind::None;
	FGuid LaunchId;
	Fdemo_mapShanmenSwordQiWorldDeliveryResult Delivery;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completion;
	FShanmenActionTransitionReceipt Interruption;

	bool IsValid() const;
};

struct Fdemo_mapShanmenSwordQiHostStartResult
{
	Edemo_mapShanmenSwordQiHostStartError Error =
		Edemo_mapShanmenSwordQiHostStartError::BindingInvalid;
	Fdemo_mapShanmenSwordQiSpawnResult Spawn;
	Fdemo_mapShanmenSwordQiLaunchResult Launch;
	FGuid LaunchId;

	bool IsStarted() const
	{
		return Error == Edemo_mapShanmenSwordQiHostStartError::None
			&& LaunchId.IsValid();
	}
};

/**
 * Product owner for one straight sword-qi flight.
 *
 * The host binds the carrier delegates and applies one explicit policy: the
 * first blocking contact ends the flight, whether it delivers one canonical
 * impact or becomes a blocking miss. It stores a non-owning coordinator; the
 * Run owner must keep that coordinator alive until this host is terminal or
 * reset. Destruction of an active host fails closed on the GameThread.
 */
class Fdemo_mapShanmenSwordQiRunHost
{
public:
	Fdemo_mapShanmenSwordQiRunHost() = default;
	~Fdemo_mapShanmenSwordQiRunHost();

	Fdemo_mapShanmenSwordQiRunHost(
		const Fdemo_mapShanmenSwordQiRunHost&) = delete;
	Fdemo_mapShanmenSwordQiRunHost& operator=(
		const Fdemo_mapShanmenSwordQiRunHost&) = delete;
	Fdemo_mapShanmenSwordQiRunHost(
		Fdemo_mapShanmenSwordQiRunHost&&) = delete;
	Fdemo_mapShanmenSwordQiRunHost& operator=(
		Fdemo_mapShanmenSwordQiRunHost&&) = delete;

	static Fdemo_mapShanmenSwordQiSpawnResult SpawnStagedCarrier(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
		AActor* SourceActor,
		const FVector& Origin);

	Fdemo_mapShanmenSwordQiHostStartResult TrySpawnAndLaunch(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenSwordQiExecution& Execution,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const FVector& Origin,
		const FVector& AimDirection);

	/** Launches an already-created Empty carrier; useful for deferred spawn. */
	Fdemo_mapShanmenSwordQiHostStartResult TryLaunchCarrier(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const FVector& Origin,
		const FVector& AimDirection,
		bool bDestroyCarrierOnTerminal);

	/** Reattaches to an already-published flight without launching again. */
	bool TryAdoptPublishedFlight(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenSwordQiExecution& Execution,
		Ademo_mapShanmenSwordQiProjectile& Projectile,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		bool bDestroyCarrierOnTerminal);

	bool TryExpireRange();
	bool TryInterrupt();
	bool Reset();

	bool IsValid() const;
	bool IsInFlight() const
	{
		return State == Edemo_mapShanmenSwordQiHostState::InFlight;
	}
	bool IsTerminal() const
	{
		return State == Edemo_mapShanmenSwordQiHostState::Terminal;
	}
	Edemo_mapShanmenSwordQiHostState GetState() const { return State; }
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const FShanmenSwordQiExecution& GetExecution() const { return Execution; }
	Ademo_mapShanmenSwordQiProjectile* GetProjectile() const
	{
		return Projectile.Get();
	}
	const Fdemo_mapShanmenSwordQiTerminalReceipt& GetTerminalReceipt() const
	{
		return TerminalReceipt;
	}
	float GetMaximumDistance() const { return MaximumDistance; }

private:
	bool ValidateBinding(
		const FShanmenActionOrchestrator& RequestedActionRuntime,
		const FShanmenSwordQiExecution& RequestedExecution,
		const Ademo_mapShanmenSwordQiProjectile& RequestedProjectile,
		const Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
		const AActor* RequestedSourceActor) const;
	bool FinishWithoutImpact(
		Edemo_mapShanmenSwordQiTerminalKind Kind,
		bool bDestroyNow,
		const Fdemo_mapShanmenSwordQiWorldDeliveryResult*
			DeliveryDiagnostic = nullptr);
	bool PublishTerminal(
		Fdemo_mapShanmenSwordQiTerminalReceipt&& Receipt,
		bool bDestroyNow);
	void HandleContact(
		Ademo_mapShanmenSwordQiProjectile& ContactProjectile,
		const FHitResult& Hit);
	void HandleRangeExpired(
		Ademo_mapShanmenSwordQiProjectile& ExpiredProjectile);
	void BindProjectile();
	void UnbindProjectile();
	void DestroyOwnedProjectile();

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSwordQiExecution Execution;
	TWeakObjectPtr<Ademo_mapShanmenSwordQiProjectile> Projectile;
	TWeakObjectPtr<AActor> SourceActor;
	Fdemo_mapCombatRunCoordinator* Coordinator = nullptr;
	FDelegateHandle ContactHandle;
	FDelegateHandle RangeExpiredHandle;
	Fdemo_mapShanmenSwordQiTerminalReceipt TerminalReceipt;
	float MaximumDistance = 0.0f;
	bool bDestroyCarrierOnTerminal = false;
	Edemo_mapShanmenSwordQiHostState State =
		Edemo_mapShanmenSwordQiHostState::Empty;
};
