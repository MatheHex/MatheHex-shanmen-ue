#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionResourceAuthority.h"
#include "demo_mapShanmenDivineSenseWorldObservationAdapter.h"

class AActor;
class UWorld;

enum class Edemo_mapShanmenDivineSensePulseStatus : uint8
{
	Invalid,
	Applied,
	AlreadyApplied,
	Rejected
};

enum class Edemo_mapShanmenDivineSensePulseError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidInput,
	RunMismatch,
	ActivationConflict,
	ProcessedCapacityExceeded,
	ResourceAuthorityNotReady,
	ResourceOwnerMismatch,
	ResourceChannelMismatch,
	ActionStartRejected,
	ResourceSnapshotRejected,
	ReservationRequestRejected,
	ResourceReservationRejected,
	ActionCommitRejected,
	FinalizationRequestRejected,
	ResourceFinalizationRejected,
	WorldObservationRejected,
	ActionRecoveryRejected,
	ActionCompletionRejected,
	StateDesynchronized
};

/** Immutable proof that one Divine Sense pulse completed atomically. */
struct Fdemo_mapShanmenDivineSensePulseReceipt
{
public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetCoordinatorId() const { return CoordinatorId; }
	const FShanmenCombatActionSnapshot& GetAction() const;
	const FShanmenDivineSenseDefinition& GetDefinition() const;
	const FShanmenActionResourceCost& GetCost() const;
	int32 GetScanOrdinal() const;
	int32 GetSubjectActorBudget() const
	{
		return WorldObservation.SubjectActorBudget;
	}
	int32 GetObservedSubjectCount() const
	{
		return WorldObservation.ObservedSubjectCount;
	}
	const FShanmenActionTransitionReceipt& GetStartup() const
	{
		return Startup;
	}
	const FShanmenActionResourceReservationReceipt& GetReservation() const
	{
		return Reservation;
	}
	const FShanmenActionTransitionReceipt& GetActiveCommit() const
	{
		return ActiveCommit;
	}
	const FShanmenActionResourceFinalizationReceipt& GetResourceCommit() const
	{
		return ResourceCommit;
	}
	const Fdemo_mapShanmenDivineSenseWorldObservationResult&
	GetWorldObservation() const
	{
		return WorldObservation;
	}
	const FShanmenActionTransitionReceipt& GetRecovery() const
	{
		return Recovery;
	}
	const FShanmenActionTransitionReceipt& GetCompletion() const
	{
		return Completion;
	}

private:
	friend class Fdemo_mapShanmenDivineSensePulseCoordinator;

	FGuid ReceiptId;
	FGuid CoordinatorId;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionResourceReservationReceipt Reservation;
	FShanmenActionTransitionReceipt ActiveCommit;
	FShanmenActionResourceFinalizationReceipt ResourceCommit;
	Fdemo_mapShanmenDivineSenseWorldObservationResult WorldObservation;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completion;
};

/** Structured result; rejection never carries a committed pulse receipt. */
struct Fdemo_mapShanmenDivineSensePulseResult
{
	Edemo_mapShanmenDivineSensePulseStatus Status =
		Edemo_mapShanmenDivineSensePulseStatus::Invalid;
	Edemo_mapShanmenDivineSensePulseError Error =
		Edemo_mapShanmenDivineSensePulseError::None;
	EShanmenActionResourceTransactionError ResourceError =
		EShanmenActionResourceTransactionError::None;
	FString Diagnostic;
	Fdemo_mapShanmenDivineSenseWorldObservationResult WorldFailure;
	Fdemo_mapShanmenDivineSensePulseReceipt Receipt;

	bool IsValid() const;
	bool IsSuccess() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSensePulseStatus::AlreadyApplied;
	}
};

/**
 * Bounded Run-scoped owner for atomic Divine Sense pulse composition.
 *
 * A new activation is staged on private action/resource copies. Resource state
 * and a replay receipt are published only after World observation and action
 * completion both succeed. Exact replay performs no World or provider I/O.
 */
class Fdemo_mapShanmenDivineSensePulseCoordinator
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		int32 ProcessedPulseCapacity,
		Fdemo_mapShanmenDivineSensePulseCoordinator& OutCoordinator);

	Fdemo_mapShanmenDivineSensePulseResult Execute(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* SourceActor,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal,
		int32 SubjectActorBudget,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider,
		FShanmenActionResourceAuthority& ResourceAuthority);

	bool IsValid() const;
	void Reset();

	const FGuid& GetCoordinatorId() const { return CoordinatorId; }
	const FGuid& GetRunId() const { return RunId; }
	int32 GetProcessedPulseCapacity() const
	{
		return ProcessedPulseCapacity;
	}
	int32 NumProcessedPulses() const { return ProcessedPulses.Num(); }

private:
	FGuid CoordinatorId;
	FGuid RunId;
	int32 ProcessedPulseCapacity = 0;
	bool bInitialized = false;
	TMap<FGuid, Fdemo_mapShanmenDivineSensePulseReceipt> ProcessedPulses;
};
