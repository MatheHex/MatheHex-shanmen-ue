#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseProductHost.h"

class AActor;
class UWorld;

/**
 * Pointer-free optimistic command for one explicit Divine Sense Actor batch.
 *
 * The command freezes the P19.3 pulse payload, the exact canonical subject
 * entity set, and the Host resource projection observed at capture time.
 */
class Fdemo_mapShanmenDivineSenseRouteCommand
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseRouteCommand& Other) const;

	const FGuid& GetRouteCommandId() const { return RouteCommandId; }
	const FGuid& GetExpectedRouterId() const { return ExpectedRouterId; }
	const FGuid& GetExpectedHostId() const { return ExpectedHostId; }
	const FGuid& GetExpectedResourceSnapshotId() const
	{
		return ExpectedResourceSnapshotId;
	}
	const Fdemo_mapShanmenDivineSensePulseCommand& GetPulseCommand() const
	{
		return PulseCommand;
	}
	const TArray<FGuid>& GetSubjectEntityIds() const
	{
		return SubjectEntityIds;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseCommandRouter;

	FGuid RouteCommandId;
	FGuid ExpectedRouterId;
	FGuid ExpectedHostId;
	FGuid ExpectedResourceSnapshotId;
	Fdemo_mapShanmenDivineSensePulseCommand PulseCommand;
	TArray<FGuid> SubjectEntityIds;
};

enum class Edemo_mapShanmenDivineSenseCommandRouteStatus : uint8
{
	Invalid,
	Applied,
	AlreadyApplied,
	RouterNotReady,
	HostNotReady,
	CommandInvalid,
	HostMismatch,
	RunMismatch,
	SourceMismatch,
	RegistryMismatch,
	ResourceProjectionStale,
	SourceActorMismatch,
	SubjectActorMismatch,
	ProcessedCapacityExceeded,
	ActivationConflict,
	HostRejected,
	StateDesynchronized
};

/** Typed route outcome retaining the complete P19.3 Host proof. */
struct Fdemo_mapShanmenDivineSenseCommandRouteResult
{
	Edemo_mapShanmenDivineSenseCommandRouteStatus Status =
		Edemo_mapShanmenDivineSenseCommandRouteStatus::Invalid;
	FString Diagnostic;
	FGuid RouterId;
	FGuid RunId;
	FGuid SourceEntityId;
	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	Fdemo_mapShanmenDivineSenseHostPulseResult HostPulse;

	bool IsValid() const;
	bool IsAccepted() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSenseCommandRouteStatus::AlreadyApplied;
	}
};

/**
 * Sole Run-scoped optimistic router in front of one P19.3 Product Host.
 *
 * Capture converts caller-selected Actors to stable registry identities and
 * freezes the current resource projection. Route revalidates those identities,
 * stages Host and replay-ledger state together, and commits only one valid
 * product result. Exact accepted replay bypasses all live Actor and evidence
 * reads and asks the Host only for its stored proof.
 */
class Fdemo_mapShanmenDivineSenseCommandRouter
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenDivineSenseProductHost& Host,
		Fdemo_mapShanmenDivineSenseCommandRouter& OutRouter);

	bool TryCaptureCommand(
		const Fdemo_mapShanmenDivineSenseProductHost& Host,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal,
		int32 SubjectActorBudget,
		const TArray<AActor*>& SubjectActors,
		Fdemo_mapShanmenDivineSenseRouteCommand& OutCommand) const;

	Fdemo_mapShanmenDivineSenseCommandRouteResult TryRoute(
		Fdemo_mapShanmenDivineSenseProductHost& Host,
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* SourceActor,
		const Fdemo_mapShanmenDivineSenseRouteCommand& Command,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	bool IsValid() const;
	bool IsConsistentWithHost(
		const Fdemo_mapShanmenDivineSenseProductHost& Host) const;
	void Reset();

	const FGuid& GetRouterId() const { return RouterId; }
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	int32 NumProcessedCommands() const { return ProcessedCommands.Num(); }
	int32 GetProcessedCommandCapacity() const
	{
		return ProcessedCommandCapacity;
	}
	const FShanmenActionResourceSnapshot& GetCurrentResourceSnapshot() const
	{
		return CurrentResourceSnapshot;
	}

private:
	struct FProcessedCommand
	{
		int32 Sequence = INDEX_NONE;
		Fdemo_mapShanmenDivineSenseRouteCommand Command;
		Fdemo_mapShanmenDivineSenseCommandRouteResult Result;
	};

	FGuid RouterId;
	FGuid HostId;
	FGuid RunId;
	FGuid SourceEntityId;
	FShanmenActionResourceSnapshot OpeningResourceSnapshot;
	FShanmenActionResourceSnapshot CurrentResourceSnapshot;
	int32 ProcessedCommandCapacity = 0;
	bool bInitialized = false;
	TMap<FGuid, FProcessedCommand> ProcessedCommands;
};
