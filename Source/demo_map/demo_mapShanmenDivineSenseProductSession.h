#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseCommandRouter.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class UWorld;

enum class Edemo_mapShanmenDivineSenseProductSessionState : uint8
{
	Empty,
	Active,
	Ended
};

/**
 * Pointer-free read-only view of current Divine Sense product capacity.
 *
 * The projection owns no resource authority. Its identity changes whenever a
 * pulse advances the Host snapshot or consumes one Router ledger slot.
 */
class Fdemo_mapShanmenDivineSenseAvailabilityProjection
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseAvailabilityProjection& Other) const;
	bool HasRouteCapacity() const
	{
		return IsValid()
			&& ProcessedCommandCount < ProcessedCommandCapacity;
	}
	bool CanAfford(const FShanmenActionResourceCost& Cost) const;

	const FGuid& GetProjectionId() const { return ProjectionId; }
	const FGuid& GetSessionId() const { return SessionId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetRouterId() const { return RouterId; }
	const FShanmenActionResourceSnapshot& GetResourceSnapshot() const
	{
		return ResourceSnapshot;
	}
	int32 GetProcessedCommandCount() const
	{
		return ProcessedCommandCount;
	}
	int32 GetProcessedCommandCapacity() const
	{
		return ProcessedCommandCapacity;
	}
	int32 GetRemainingCommandCapacity() const
	{
		return IsValid()
			? ProcessedCommandCapacity - ProcessedCommandCount
			: 0;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseProductSession;

	FGuid ProjectionId;
	FGuid SessionId;
	FGuid RunId;
	FGuid SourceEntityId;
	FGuid HostId;
	FGuid RouterId;
	FShanmenActionResourceSnapshot ResourceSnapshot;
	int32 ProcessedCommandCount = INDEX_NONE;
	int32 ProcessedCommandCapacity = INDEX_NONE;
};

enum class Edemo_mapShanmenDivineSenseSessionRouteStatus : uint8
{
	Invalid,
	Applied,
	AlreadyApplied,
	SessionNotActive,
	SessionInvalid,
	CoordinatorNotReady,
	RunMismatch,
	SourceMismatch,
	CommandInvalid,
	RouteRejected,
	StateDesynchronized
};

/** Session-level route proof with availability on both sides. */
struct Fdemo_mapShanmenDivineSenseSessionRouteResult
{
	Edemo_mapShanmenDivineSenseSessionRouteStatus Status =
		Edemo_mapShanmenDivineSenseSessionRouteStatus::Invalid;
	FString Diagnostic;
	FGuid SessionId;
	Fdemo_mapShanmenDivineSenseAvailabilityProjection AvailabilityBefore;
	Fdemo_mapShanmenDivineSenseAvailabilityProjection AvailabilityAfter;
	Fdemo_mapShanmenDivineSenseCommandRouteResult Route;

	bool IsValid() const;
	bool IsAccepted() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSenseSessionRouteStatus::AlreadyApplied;
	}
};

/** Immutable proof retained after the Session releases its active lifecycle. */
class Fdemo_mapShanmenDivineSenseSessionEndReceipt
{
public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetSessionId() const { return SessionId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetRouterId() const { return RouterId; }
	const FGuid& GetOpeningResourceSnapshotId() const
	{
		return OpeningResourceSnapshotId;
	}
	const FGuid& GetFinalResourceSnapshotId() const
	{
		return FinalResourceSnapshotId;
	}
	int32 GetProcessedCommandCount() const
	{
		return ProcessedCommandCount;
	}
	int32 GetProcessedCommandCapacity() const
	{
		return ProcessedCommandCapacity;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseProductSession;

	FGuid ReceiptId;
	FGuid SessionId;
	FGuid RunId;
	FGuid SourceEntityId;
	FGuid HostId;
	FGuid RouterId;
	FGuid OpeningResourceSnapshotId;
	FGuid FinalResourceSnapshotId;
	int32 ProcessedCommandCount = INDEX_NONE;
	int32 ProcessedCommandCapacity = INDEX_NONE;
};

enum class Edemo_mapShanmenDivineSenseSessionEndStatus : uint8
{
	Invalid,
	Ended,
	AlreadyEnded,
	SessionNotActive,
	SessionInvalid,
	CoordinatorNotReady,
	RunMismatch,
	SourceMismatch,
	SessionMismatch,
	StateDesynchronized
};

/** Typed teardown outcome. Exact replay returns the retained receipt. */
struct Fdemo_mapShanmenDivineSenseSessionEndResult
{
	Edemo_mapShanmenDivineSenseSessionEndStatus Status =
		Edemo_mapShanmenDivineSenseSessionEndStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenDivineSenseSessionEndReceipt Receipt;

	bool IsValid() const;
	bool IsSuccess() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSenseSessionEndStatus::AlreadyEnded;
	}
};

/**
 * Sole Run-lifecycle owner of the P19.3 Host and P19.4 Router pair.
 *
 * Begin consumes one caller-explicit, unreserved SpiritEnergy snapshot and
 * binds it to the ready Combat Run's player identity. Every command and route
 * revalidates that same Coordinator without retaining Actor or Coordinator
 * pointers. End must occur against the same live Run and seals an immutable
 * terminal receipt over the retained value-only Host and Router proof. Input,
 * Actor discovery, UI and balance policy remain outside this value object.
 */
class Fdemo_mapShanmenDivineSenseProductSession
{
public:
	bool TryBegin(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenActionResourceSnapshot& OpeningSpiritEnergy,
		int32 ProcessedPulseCapacity,
		FString& OutDiagnostic);

	bool TryCaptureAvailability(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseAvailabilityProjection& OutProjection,
		FString& OutDiagnostic) const;
	bool IsAvailabilityCurrent(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenDivineSenseAvailabilityProjection& Projection)
		const;

	bool TryCaptureCommand(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal,
		int32 SubjectActorBudget,
		const TArray<AActor*>& SubjectActors,
		Fdemo_mapShanmenDivineSenseRouteCommand& OutCommand,
		FString& OutDiagnostic) const;

	Fdemo_mapShanmenDivineSenseSessionRouteResult TryRoute(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const Fdemo_mapShanmenDivineSenseRouteCommand& Command,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	Fdemo_mapShanmenDivineSenseSessionEndResult TryEnd(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ExpectedSessionId);

	/** Clears only empty or already-ended state; active state must TryEnd. */
	bool Reset();
	bool IsValid() const;
	bool IsEmpty() const
	{
		return State == Edemo_mapShanmenDivineSenseProductSessionState::Empty;
	}
	bool IsActive() const
	{
		return State == Edemo_mapShanmenDivineSenseProductSessionState::Active;
	}
	bool IsEnded() const
	{
		return State == Edemo_mapShanmenDivineSenseProductSessionState::Ended;
	}
	bool IsConsistentWithCoordinator(
		const Fdemo_mapCombatRunCoordinator& Coordinator) const;

	Edemo_mapShanmenDivineSenseProductSessionState GetState() const
	{
		return State;
	}
	const FGuid& GetSessionId() const { return SessionId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const FShanmenActionResourceSnapshot& GetOpeningResourceSnapshot() const
	{
		return OpeningResourceSnapshot;
	}
	const Fdemo_mapShanmenDivineSenseProductHost& GetHost() const
	{
		return Host;
	}
	const Fdemo_mapShanmenDivineSenseCommandRouter& GetRouter() const
	{
		return Router;
	}
	const Fdemo_mapShanmenDivineSenseSessionEndReceipt& GetEndReceipt() const
	{
		return EndReceipt;
	}

private:
	bool TryBuildAvailability(
		Fdemo_mapShanmenDivineSenseAvailabilityProjection& OutProjection) const;

	FGuid SessionId;
	FGuid RunId;
	FGuid SourceEntityId;
	FShanmenActionResourceSnapshot OpeningResourceSnapshot;
	Fdemo_mapShanmenDivineSenseProductHost Host;
	Fdemo_mapShanmenDivineSenseCommandRouter Router;
	Fdemo_mapShanmenDivineSenseSessionEndReceipt EndReceipt;
	Edemo_mapShanmenDivineSenseProductSessionState State =
		Edemo_mapShanmenDivineSenseProductSessionState::Empty;
};
