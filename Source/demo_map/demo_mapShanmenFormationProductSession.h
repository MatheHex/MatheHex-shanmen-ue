#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationMaterialAdapter.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationSessionState : uint8
{
	Empty,
	Deploying,
	Active,
	Cancelled,
	Ended
};

enum class Edemo_mapShanmenFormationSessionStatus : uint8
{
	Prepared,
	Committed,
	Replayed,
	Cancelled,
	Ended,
	SessionInvalid,
	CorrelationMismatch,
	RequestInvalid,
	PendingConflict,
	AttemptConflict,
	PendingMissing,
	MaterialRejected,
	MaterialCommitRecoveryRequired,
	MaterialCancelRecoveryRequired,
	DeploymentRecoveryRequired,
	DeploymentTransitionRejected,
	ActionTransitionRejected
};

/** Immutable audit owned after both material and deployment commits succeed. */
struct Fdemo_mapShanmenFormationAnchorAudit
{
	FName AnchorDefinitionId = NAME_None;
	FGuid AttemptId;
	Fdemo_mapShanmenFormationMaterialResult Material;
	FShanmenFormationDeploymentReceipt DeploymentReceipt;

	bool IsValid() const;
};

/** One explicit product-session operation result; no hidden product mutation. */
struct Fdemo_mapShanmenFormationSessionResult
{
	Edemo_mapShanmenFormationSessionStatus Status =
		Edemo_mapShanmenFormationSessionStatus::RequestInvalid;
	FString Diagnostic;
	FName AnchorDefinitionId = NAME_None;
	FGuid AttemptId;
	Fdemo_mapShanmenFormationMaterialResult Material;
	FShanmenFormationDeploymentReceipt DeploymentReceipt;
	FShanmenActionTransitionReceipt ActionReceipt;
	FShanmenActionTransitionReceipt CompletionReceipt;

	bool IsSuccess() const;
};

/**
 * Transient product owner for one formation activation.
 *
 * It serializes one anchor material attempt at a time and only publishes an
 * anchor audit after durable Items commit and the pure P8.0 deployment commit
 * both succeed. Actor placement, effects, authored recipes, interaction, UI,
 * timers, and legacy inventory remain outside this state machine.
 */
class Fdemo_mapShanmenFormationProductSession
{
public:
	static bool TryStart(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenFormationDiagramDefinition& Diagram,
		const FVector& Origin,
		const FVector& Forward,
		Fdemo_mapShanmenFormationProductSession& OutSession,
		FShanmenActionTransitionReceipt& OutStartup,
		FShanmenActionTransitionReceipt& OutActive,
		FShanmenFormationDeploymentReceipt& OutBegin);

	Fdemo_mapShanmenFormationSessionResult TryPrepareAnchor(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);
	Fdemo_mapShanmenFormationSessionResult TryCommitPreparedAnchor(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);
	Fdemo_mapShanmenFormationSessionResult TryCancel(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationSessionResult TryEnd(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);

	bool IsValid() const;
	bool IsTerminal() const;
	void Reset();

	Edemo_mapShanmenFormationSessionState GetState() const { return State; }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const FShanmenFormationDeployment& GetDeployment() const
	{
		return Deployment;
	}
	const TArray<Fdemo_mapShanmenFormationAnchorAudit>& GetAnchorAudits() const
	{
		return AnchorAudits;
	}
	bool HasPendingMaterial() const { return bHasPendingMaterial; }
	const Fdemo_mapShanmenFormationMaterialResult* GetPendingMaterial() const
	{
		return bHasPendingMaterial ? &PendingMaterial : nullptr;
	}

private:
	const Fdemo_mapShanmenFormationAnchorAudit* FindAudit(
		FName AnchorDefinitionId) const;
	void ClearPendingMaterial();

	Fdemo_mapShanmenRunCorrelation Correlation;
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenFormationDeployment Deployment;
	TArray<Fdemo_mapShanmenFormationAnchorAudit> AnchorAudits;
	Fdemo_mapShanmenFormationMaterialResult PendingMaterial;
	Edemo_mapShanmenFormationSessionState State =
		Edemo_mapShanmenFormationSessionState::Empty;
	bool bHasPendingMaterial = false;
};
