#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationProductAuthority.h"
#include "demo_mapShanmenFormationProductHost.h"

class Fdemo_mapCombatRunCoordinator;
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
	HostStartRejected
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
	FString Diagnostic;

	bool IsAccepted() const;
};

/** Audit retained while one Run-scoped controller closes its owned Host. */
struct Fdemo_mapShanmenFormationControllerEndSummary
{
	FGuid RunId;
	int32 CapturedIntentCount = 0;
	bool bHadProductHost = false;
	bool bDiscardedUnstartedCommand = false;
	Fdemo_mapShanmenFormationHostResult Terminal;

	bool IsValid() const;
};

/**
 * Sole Run-scoped owner for one player formation product.
 *
 * The first valid IntentId freezes durable item authority and one combat Run
 * sequence, then starts the existing ProductHost. Exact replay returns the
 * same start proof without another sequence; a changed or second intent fails
 * before authority is sampled. World placement and anchor cadence remain
 * explicit later controller operations rather than hidden polling.
 */
class Fdemo_mapShanmenFormationProductController
{
public:
	bool TryBegin(const FGuid& RequestedRunId, FString& OutDiagnostic);

	Fdemo_mapShanmenFormationControllerResult TrySubmit(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenFormationIntent& Intent);

	bool TryCancelAndEnd(
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
	struct FCapturedIntent
	{
		Fdemo_mapShanmenFormationIntent Intent;
		Fdemo_mapShanmenFormationProductPreparationResult Preparation;
		Fdemo_mapShanmenFormationControllerResult LastResult;
	};

	Fdemo_mapShanmenFormationControllerResult StartCaptured(
		FCapturedIntent& Captured,
		bool bReusedIntent);
	void Clear();

	FGuid RunId;
	TOptional<FCapturedIntent> CapturedIntent;
	Fdemo_mapShanmenFormationProductHost ProductHost;
	bool bHasProductHost = false;
};
