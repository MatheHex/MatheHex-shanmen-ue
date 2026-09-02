#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenPlayerActionArbitration.h"
#include "demo_mapShanmenSwordQiProductAuthority.h"
#include "demo_mapShanmenSwordQiRunHost.h"

class UWorld;

enum class Edemo_mapShanmenSwordQiProductRouteStatus : uint8
{
	Applied,
	CoordinatorNotReady,
	CommandInvalid,
	RunMismatch,
	SourceMismatch,
	SessionInvalid,
	SessionRunMismatch,
	CommandIdConflict,
	HostBusy,
	ActionGateInvalid,
	ActionGateRejected,
	CarrierSpawnRejected,
	ExecutionRejected,
	ActionStartRejected,
	ActionCommitRejected,
	LaunchRejectedInterrupted,
	LaunchRecoveryRejected
};

/** Complete audit for one routed sword-qi command or exact replay. */
struct Fdemo_mapShanmenSwordQiProductRouteResult
{
	Edemo_mapShanmenSwordQiProductRouteStatus Status =
		Edemo_mapShanmenSwordQiProductRouteStatus::CoordinatorNotReady;
	bool bReplay = false;
	FGuid CommandId;
	FGuid RunId;
	FGuid SourceItemInstanceId;
	FGuid LaunchId;
	Fdemo_mapShanmenPlayerActionGateResult ActionGate;
	Fdemo_mapShanmenSwordQiSpawnResult Spawn;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;
	Fdemo_mapShanmenSwordQiHostStartResult HostStart;
	FShanmenActionTransitionReceipt Interruption;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsTerminal() const;
	bool IsReplay() const { return bReplay; }
};

/**
 * Run-scoped product owner between a frozen P18.3 command and the P18.2 Host.
 *
 * Authorization is requested exactly once and only after structural fences
 * pass. The carrier is created collision-inert before Startup crosses Active;
 * every failed path destroys it. Exact command replay returns stored receipts
 * without another authorization, sequence reservation or Actor creation.
 */
class Fdemo_mapShanmenSwordQiProductSession
{
public:
	Fdemo_mapShanmenSwordQiProductRouteResult TryRoute(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const Fdemo_mapShanmenSwordQiLaunchCommand& Command,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

	bool TryInterrupt();
	bool TryExpireRange();
	/** Copies the immutable terminal proof before making room for another shot. */
	bool TryRetireTerminal(
		Fdemo_mapShanmenSwordQiTerminalReceipt& OutReceipt);
	/** Adds the active Sword Qi as a non-preemptible player-action owner. */
	bool TryAppendOccupancy(
		Fdemo_mapShanmenPlayerActionOccupancySnapshot& InOutSnapshot) const;
	bool Reset();

	bool IsValid() const;
	bool IsEmpty() const
	{
		return Host.GetState() == Edemo_mapShanmenSwordQiHostState::Empty;
	}
	bool IsInFlight() const { return Host.IsInFlight(); }
	bool IsTerminal() const { return Host.IsTerminal(); }
	const FGuid& GetRunId() const { return RunId; }
	FGuid GetOccupancyOwnerId() const;
	int32 NumProcessedCommands() const { return ProcessedCommands.Num(); }
	const Fdemo_mapShanmenSwordQiRunHost& GetHost() const { return Host; }

private:
	struct FProcessedCommand
	{
		Fdemo_mapShanmenSwordQiLaunchCommand Command;
		Fdemo_mapShanmenSwordQiProductRouteResult Result;
	};

	void RecordTerminal(
		const Fdemo_mapShanmenSwordQiLaunchCommand& Command,
		const Fdemo_mapShanmenSwordQiProductRouteResult& Result);

	FGuid RunId;
	TMap<FGuid, FProcessedCommand> ProcessedCommands;
	Fdemo_mapShanmenSwordQiRunHost Host;
};
