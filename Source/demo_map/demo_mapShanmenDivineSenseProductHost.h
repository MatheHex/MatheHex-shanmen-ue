#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSensePulseCoordinator.h"

class AActor;
class UWorld;

/** Immutable product command for one explicitly bounded Divine Sense pulse. */
struct Fdemo_mapShanmenDivineSensePulseCommand
{
public:
	static bool TryCapture(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal,
		int32 SubjectActorBudget,
		Fdemo_mapShanmenDivineSensePulseCommand& OutCommand);

	bool IsValid() const;
	const FGuid& GetCommandId() const { return CommandId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenDivineSenseDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenActionResourceCost& GetCost() const { return Cost; }
	int32 GetScanOrdinal() const { return ScanOrdinal; }
	int32 GetSubjectActorBudget() const { return SubjectActorBudget; }

private:
	FGuid CommandId;
	FShanmenCombatActionSnapshot Action;
	FShanmenDivineSenseDefinition Definition;
	FShanmenActionResourceCost Cost;
	int32 ScanOrdinal = INDEX_NONE;
	int32 SubjectActorBudget = INDEX_NONE;
};

enum class Edemo_mapShanmenDivineSenseHostPulseStatus : uint8
{
	Invalid,
	Applied,
	AlreadyApplied,
	Rejected
};

enum class Edemo_mapShanmenDivineSenseHostPulseError : uint8
{
	None,
	HostNotReady,
	InvalidCommand,
	RunMismatch,
	SourceMismatch,
	PulseRejected,
	ResourceSnapshotRejected,
	StateDesynchronized
};

/** Product-level result with resource state on both sides of one pulse. */
struct Fdemo_mapShanmenDivineSenseHostPulseResult
{
	Edemo_mapShanmenDivineSenseHostPulseStatus Status =
		Edemo_mapShanmenDivineSenseHostPulseStatus::Invalid;
	Edemo_mapShanmenDivineSenseHostPulseError Error =
		Edemo_mapShanmenDivineSenseHostPulseError::None;
	FString Diagnostic;
	FGuid HostId;
	FGuid RunId;
	Fdemo_mapShanmenDivineSensePulseCommand Command;
	FShanmenActionResourceSnapshot ResourceBefore;
	FShanmenActionResourceSnapshot ResourceAfter;
	Fdemo_mapShanmenDivineSensePulseResult Pulse;

	bool IsValid() const;
	bool IsSuccess() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSenseHostPulseStatus::AlreadyApplied;
	}
};

/**
 * Sole Run-scoped product owner for Divine Sense pulse resource state.
 *
 * The host owns exactly one SpiritEnergy authority and the P19.2 atomic pulse
 * coordinator. New pulses are staged on a host copy and published together;
 * exact replay returns stored proof without World or evidence-provider I/O.
 * Subject Actors remain an explicit bounded caller-owned input.
 */
class Fdemo_mapShanmenDivineSenseProductHost
{
public:
	static bool TryOpen(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		float CurrentSpiritEnergy,
		float MaximumSpiritEnergy,
		int64 AuthorityRevision,
		int32 ProcessedPulseCapacity,
		Fdemo_mapShanmenDivineSenseProductHost& OutHost);

	Fdemo_mapShanmenDivineSenseHostPulseResult ExecutePulse(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* SourceActor,
		const Fdemo_mapShanmenDivineSensePulseCommand& Command,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	bool IsValid() const;
	void Reset();
	bool TryCaptureResourceSnapshot(
		FShanmenActionResourceSnapshot& OutSnapshot) const;

	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetRunId() const { return Coordinator.GetRunId(); }
	const FGuid& GetSourceEntityId() const
	{
		return ResourceAuthority.GetOwnerEntityId();
	}
	float GetCurrentSpiritEnergy() const
	{
		return ResourceAuthority.GetCurrentAmount();
	}
	float GetMaximumSpiritEnergy() const
	{
		return ResourceAuthority.GetMaximumAmount();
	}
	int64 GetResourceRevision() const
	{
		return ResourceAuthority.GetAuthorityRevision();
	}
	int32 NumProcessedPulses() const
	{
		return Coordinator.NumProcessedPulses();
	}
	int32 GetProcessedPulseCapacity() const
	{
		return Coordinator.GetProcessedPulseCapacity();
	}
	const FShanmenActionResourceSnapshot& GetOpeningResourceSnapshot() const
	{
		return OpeningResourceSnapshot;
	}
	const Fdemo_mapShanmenDivineSensePulseCoordinator& GetCoordinator() const
	{
		return Coordinator;
	}

private:
	FGuid HostId;
	FShanmenActionResourceSnapshot OpeningResourceSnapshot;
	FShanmenActionResourceAuthority ResourceAuthority;
	Fdemo_mapShanmenDivineSensePulseCoordinator Coordinator;
	bool bInitialized = false;
};
