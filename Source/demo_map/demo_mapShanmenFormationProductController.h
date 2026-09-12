#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseProductController.h"
#include "demo_mapShanmenFormationProductAuthority.h"
#include "demo_mapShanmenFormationProductHost.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapShanmenFormationRunLifecycle;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Stable device-independent request captured before formation authority. */
class Fdemo_mapShanmenFormationIntent
{
public:
	static bool TryCapture(
		const FGuid& RequestedIntentId,
		const FGuid& RequestedRunId,
		const FShanmenFormationDiagramDefinition& RequestedDiagram,
		const FVector& RequestedOrigin,
		const FVector& RequestedForward,
		Fdemo_mapShanmenFormationIntent& OutIntent);

	bool IsValid() const;
	bool Matches(const Fdemo_mapShanmenFormationIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetRunId() const { return RunId; }
	const FShanmenFormationDiagramDefinition& GetDiagram() const
	{
		return Diagram;
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetForward() const { return Forward; }

private:
	FGuid IntentId;
	FGuid RunId;
	FShanmenFormationDiagramDefinition Diagram;
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ZeroVector;
};

enum class Edemo_mapShanmenFormationControllerStatus : uint8
{
	Started,
	ControllerInactive,
	ControllerInvalid,
	IntentInvalid,
	RunMismatch,
	IntentIdConflict,
	HostBusy,
	PreparationRejected,
	HostStartRejected,
	SharedResourceRejected,
	StateDesynchronized
};

/** Frozen proof from durable item Run through formation ProductHost start. */
struct Fdemo_mapShanmenFormationControllerResult
{
	Edemo_mapShanmenFormationControllerStatus Status =
		Edemo_mapShanmenFormationControllerStatus::ControllerInactive;
	bool bReusedIntent = false;
	FGuid IntentId;
	FGuid RunId;
	Fdemo_mapShanmenFormationProductPreparationResult Preparation;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;
	FShanmenFormationDeploymentReceipt Begin;
	FShanmenActionResourceTransactionResult ResourceReserve;
	FShanmenActionResourceTransactionResult ResourceCommit;
	Fdemo_mapShanmenSharedSpiritEnergyTransactionResult SharedResource;
	FString Diagnostic;

	bool IsAccepted() const;
};

/** Device-independent identity for one explicit anchor operation. */
class Fdemo_mapShanmenFormationAnchorOperation
{
public:
	static bool TryCapture(
		const FGuid& RequestedRunId,
		FName RequestedAnchorDefinitionId,
		const FGuid& RequestedAttemptId,
		Fdemo_mapShanmenFormationAnchorOperation& OutOperation);

	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetAttemptId() const { return AttemptId; }

private:
	FGuid RunId;
	FName AnchorDefinitionId = NAME_None;
	FGuid AttemptId;
};

enum class Edemo_mapShanmenFormationAnchorOperationStatus : uint8
{
	Placed,
	Replayed,
	LifecycleInactive,
	LifecycleInvalid,
	TeardownPending,
	ControllerInactive,
	ControllerInvalid,
	OperationInvalid,
	RunMismatch,
	CoordinatorNotReady,
	ProductHostMissing,
	WorldBindingInvalid,
	PendingPlacementConflict,
	PreparationRejected,
	CommitRejected,
	PlacementRejected
};

/** Full proof for one explicit prepare -> commit -> World placement gateway. */
struct Fdemo_mapShanmenFormationAnchorOperationResult
{
	Edemo_mapShanmenFormationAnchorOperationStatus Status =
		Edemo_mapShanmenFormationAnchorOperationStatus::LifecycleInactive;
	bool bResumedCommittedPlacement = false;
	FString ActorClassPath;
	Fdemo_mapShanmenFormationAnchorOperation Operation;
	Fdemo_mapShanmenFormationHostResult Prepared;
	Fdemo_mapShanmenFormationHostResult Committed;
	Fdemo_mapShanmenFormationHostResult Placement;
	FString Diagnostic;

	bool IsSuccess() const;
};

/** Audit retained while one Run-scoped controller closes its owned Host. */
struct Fdemo_mapShanmenFormationControllerEndSummary
{
	FGuid RunId;
	int32 CapturedIntentCount = 0;
	bool bHadProductHost = false;
	bool bEndedCompletedFormation = false;
	bool bDiscardedUnstartedCommand = false;
	Fdemo_mapShanmenFormationHostResult Terminal;

	bool IsValid() const;
};

/**
 * Sole Run-scoped owner for one player formation product.
 *
 * The first valid IntentId freezes durable item authority and one combat Run
 * sequence, then atomically starts the existing ProductHost and commits the
 * diagram's authored cost through the Run's shared SpiritEnergy authority.
 * Exact replay returns the same product/resource proof without another
 * sequence or spend; a changed or second intent fails before authority is
 * sampled. World placement enters only through the explicit RunLifecycle
 * anchor gateway rather than hidden polling.
 */
class Fdemo_mapShanmenFormationProductController
{
public:
	bool TryBegin(const FGuid& RequestedRunId, FString& OutDiagnostic);

	Fdemo_mapShanmenFormationControllerResult TrySubmit(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductController&
			SpiritEnergyController,
		const Fdemo_mapShanmenFormationIntent& Intent);

	bool TryTerminateAndEnd(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		UWorld* World,
		const FGuid& ExpectedRunId,
		Fdemo_mapShanmenFormationControllerEndSummary& OutSummary,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const;
	bool HasProductHost() const { return bHasProductHost; }
	const FGuid& GetRunId() const { return RunId; }
	int32 NumCapturedIntents() const { return CapturedIntent.IsSet() ? 1 : 0; }
	const Fdemo_mapShanmenFormationProductHost* GetProductHost() const
	{
		return bHasProductHost ? &ProductHost : nullptr;
	}
	const Fdemo_mapShanmenFormationDeploymentCommand* FindCapturedCommand(
		const FGuid& IntentId) const;

private:
	friend class Fdemo_mapShanmenFormationRunLifecycle;

	struct FCapturedIntent
	{
		Fdemo_mapShanmenFormationIntent Intent;
		Fdemo_mapShanmenFormationProductPreparationResult Preparation;
		Fdemo_mapShanmenFormationControllerResult LastResult;
	};

	Fdemo_mapShanmenFormationControllerResult StartCaptured(
		FCapturedIntent& Captured,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductController&
			SpiritEnergyController,
		bool bReusedIntent);
	Fdemo_mapShanmenFormationAnchorOperationResult TryExecuteAnchorOperation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		TSubclassOf<AActor> ActorClass,
		const Fdemo_mapShanmenFormationAnchorOperation& Operation);
	void Clear();

	FGuid RunId;
	TOptional<FCapturedIntent> CapturedIntent;
	Fdemo_mapShanmenFormationProductHost ProductHost;
	bool bHasProductHost = false;
};
