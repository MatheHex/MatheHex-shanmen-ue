#include "demo_mapShanmenFormationProductSession.h"

#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& SameContent(Left.GetContent(), Right.GetContent())
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool EvidenceMatches(
		const FShanmenFormationAnchorFulfillmentEvidence& Left,
		const FShanmenFormationAnchorFulfillmentEvidence& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.FulfillmentId != Right.FulfillmentId
			|| Left.RunId != Right.RunId || Left.OwnerId != Right.OwnerId
			|| Left.DeploymentId != Right.DeploymentId
			|| Left.AnchorDefinitionId != Right.AnchorDefinitionId
			|| !SameContent(Left.Content, Right.Content)
			|| Left.AuthorityRevision != Right.AuthorityRevision
			|| Left.Lines.Num() != Right.Lines.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Lines.Num(); ++Index)
		{
			const FShanmenFormationMaterialFulfillmentLine& LeftLine =
				Left.Lines[Index];
			const FShanmenFormationMaterialFulfillmentLine& RightLine =
				Right.Lines[Index];
			if (LeftLine.ItemInstanceId != RightLine.ItemInstanceId
				|| LeftLine.MaterialDefinitionId
					!= RightLine.MaterialDefinitionId
				|| LeftLine.Quantity != RightLine.Quantity)
			{
				return false;
			}
		}
		return true;
	}

	Fdemo_mapShanmenFormationSessionResult Reject(
		const Edemo_mapShanmenFormationSessionStatus Status,
		const FName AnchorDefinitionId,
		const FGuid& AttemptId,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationSessionResult Result;
		Result.Status = Status;
		Result.AnchorDefinitionId = AnchorDefinitionId;
		Result.AttemptId = AttemptId;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	bool IsPendingMaterialValid(
		const Fdemo_mapShanmenFormationMaterialResult& Material)
	{
		return Material.HasPlan()
			&& (Material.IsPrepared() || Material.IsCommitted());
	}
}

bool Fdemo_mapShanmenFormationAnchorAudit::IsValid() const
{
	return !AnchorDefinitionId.IsNone() && AttemptId.IsValid()
		&& Material.IsCommitted()
		&& Material.AnchorDefinitionId == AnchorDefinitionId
		&& Material.AttemptId == AttemptId
		&& DeploymentReceipt.IsValid()
		&& DeploymentReceipt.GetEventKind()
			== EShanmenFormationDeploymentEventKind::CommitAnchor
		&& DeploymentReceipt.GetDeploymentId() == Material.DeploymentId
		&& DeploymentReceipt.GetAnchorDefinitionId() == AnchorDefinitionId
		&& DeploymentReceipt.GetFulfillmentId()
			== Material.Evidence.FulfillmentId
		&& DeploymentReceipt.GetAuthorityRevision()
			== Material.Evidence.AuthorityRevision;
}

bool Fdemo_mapShanmenFormationSessionResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationSessionStatus::Prepared
		|| Status == Edemo_mapShanmenFormationSessionStatus::Committed
		|| Status == Edemo_mapShanmenFormationSessionStatus::Replayed
		|| Status == Edemo_mapShanmenFormationSessionStatus::Cancelled
		|| Status == Edemo_mapShanmenFormationSessionStatus::Ended;
}

bool Fdemo_mapShanmenFormationProductSession::TryStart(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenFormationDiagramDefinition& Diagram,
	const FVector& Origin,
	const FVector& Forward,
	Fdemo_mapShanmenFormationProductSession& OutSession,
	FShanmenActionTransitionReceipt& OutStartup,
	FShanmenActionTransitionReceipt& OutActive,
	FShanmenFormationDeploymentReceipt& OutBegin)
{
	OutSession.Reset();
	OutStartup = FShanmenActionTransitionReceipt();
	OutActive = FShanmenActionTransitionReceipt();
	OutBegin = FShanmenFormationDeploymentReceipt();
	if (!RequestedCorrelation.IsValid() || !Action.IsValid()
		|| !Diagram.IsValid()
		|| RequestedCorrelation.ActiveRunId != Action.GetRunId()
		|| RequestedCorrelation.OwnerId != Action.GetOwnerId())
	{
		return false;
	}

	Fdemo_mapShanmenFormationProductSession Candidate;
	Candidate.Correlation = RequestedCorrelation;
	if (!FShanmenActionOrchestrator::TryStart(
			Action, Candidate.ActionRuntime, OutStartup)
		|| !Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, OutActive)
		|| !FShanmenFormationDeployment::TryCreate(
			Action, Diagram, Origin, Forward, Candidate.Deployment)
		|| !Candidate.Deployment.TryBeginDeployment(
			Candidate.ActionRuntime, OutBegin))
	{
		OutStartup = FShanmenActionTransitionReceipt();
		OutActive = FShanmenActionTransitionReceipt();
		OutBegin = FShanmenFormationDeploymentReceipt();
		return false;
	}
	Candidate.State = Edemo_mapShanmenFormationSessionState::Deploying;
	if (!Candidate.IsValid())
	{
		OutStartup = FShanmenActionTransitionReceipt();
		OutActive = FShanmenActionTransitionReceipt();
		OutBegin = FShanmenFormationDeploymentReceipt();
		return false;
	}
	OutSession = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenFormationSessionResult
Fdemo_mapShanmenFormationProductSession::TryPrepareAnchor(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::SessionInvalid,
			AnchorDefinitionId, AttemptId,
			TEXT("Formation material preparation requires one valid product session."));
	}
	if (RequestedCorrelation != Correlation)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::CorrelationMismatch,
			AnchorDefinitionId, AttemptId,
			TEXT("Formation material preparation rejected a stale or foreign Run correlation."));
	}
	if (AnchorDefinitionId.IsNone() || !AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::RequestInvalid,
			AnchorDefinitionId, AttemptId,
			TEXT("Formation material preparation requires an anchor and explicit AttemptId."));
	}
	if (const Fdemo_mapShanmenFormationAnchorAudit* Existing =
		FindAudit(AnchorDefinitionId))
	{
		if (Existing->AttemptId != AttemptId)
		{
			return Reject(
				Edemo_mapShanmenFormationSessionStatus::AttemptConflict,
				AnchorDefinitionId, AttemptId,
				TEXT("A committed anchor cannot be rebound to another material attempt."));
		}
		Fdemo_mapShanmenFormationSessionResult Result;
		Result.Status = Edemo_mapShanmenFormationSessionStatus::Replayed;
		Result.AnchorDefinitionId = AnchorDefinitionId;
		Result.AttemptId = AttemptId;
		Result.Material = Existing->Material;
		Result.DeploymentReceipt = Existing->DeploymentReceipt;
		Result.Diagnostic =
			TEXT("The exact committed anchor replayed its immutable audit.");
		return Result;
	}
	if (AnchorAudits.ContainsByPredicate(
		[AttemptId](const Fdemo_mapShanmenFormationAnchorAudit& Audit)
		{
			return Audit.AttemptId == AttemptId;
		}))
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::AttemptConflict,
			AnchorDefinitionId,
			AttemptId,
			TEXT("A committed material attempt cannot be rebound to another anchor."));
	}
	if (State != Edemo_mapShanmenFormationSessionState::Deploying)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::SessionInvalid,
			AnchorDefinitionId, AttemptId,
			TEXT("Only a deploying formation may prepare another anchor."));
	}
	if (bHasPendingMaterial
		&& (PendingMaterial.AnchorDefinitionId != AnchorDefinitionId
			|| PendingMaterial.AttemptId != AttemptId))
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::PendingConflict,
			AnchorDefinitionId, AttemptId,
			TEXT("Another exact anchor attempt must commit or cancel before a new prepare."));
	}

	Fdemo_mapShanmenFormationMaterialResult Prepared;
	if (bHasPendingMaterial && PendingMaterial.IsCommitted())
	{
		Prepared = PendingMaterial;
	}
	else
	{
		Prepared = Fdemo_mapShanmenFormationMaterialAdapter::PrepareMaterials(
			Authority, Correlation, Deployment, AnchorDefinitionId, AttemptId);
	}
	if (!IsPendingMaterialValid(Prepared))
	{
		Fdemo_mapShanmenFormationSessionResult Result = Reject(
			Edemo_mapShanmenFormationSessionStatus::MaterialRejected,
			AnchorDefinitionId, AttemptId, Prepared.Diagnostic);
		Result.Material = MoveTemp(Prepared);
		return Result;
	}

	const bool bReplay = bHasPendingMaterial
		|| Prepared.Status == Edemo_mapShanmenFormationMaterialStatus::Replayed;
	PendingMaterial = Prepared;
	bHasPendingMaterial = true;
	Fdemo_mapShanmenFormationSessionResult Result;
	Result.Status = bReplay
		? Edemo_mapShanmenFormationSessionStatus::Replayed
		: Edemo_mapShanmenFormationSessionStatus::Prepared;
	Result.AnchorDefinitionId = AnchorDefinitionId;
	Result.AttemptId = AttemptId;
	Result.Material = PendingMaterial;
	Result.Diagnostic = bReplay
		? TEXT("The exact pending material attempt replayed without a second reservation.")
		: TEXT("One exact anchor material attempt is durably prepared.");
	return Result;
}

Fdemo_mapShanmenFormationSessionResult
Fdemo_mapShanmenFormationProductSession::TryCommitPreparedAnchor(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::SessionInvalid,
			AnchorDefinitionId, AttemptId,
			TEXT("Formation anchor commit requires one valid product session."));
	}
	if (RequestedCorrelation != Correlation)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::CorrelationMismatch,
			AnchorDefinitionId, AttemptId,
			TEXT("Formation anchor commit rejected a stale or foreign Run correlation."));
	}
	if (AnchorDefinitionId.IsNone() || !AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::RequestInvalid,
			AnchorDefinitionId, AttemptId,
			TEXT("Formation anchor commit requires an anchor and explicit AttemptId."));
	}
	if (const Fdemo_mapShanmenFormationAnchorAudit* Existing =
		FindAudit(AnchorDefinitionId))
	{
		if (Existing->AttemptId != AttemptId)
		{
			return Reject(
				Edemo_mapShanmenFormationSessionStatus::AttemptConflict,
				AnchorDefinitionId, AttemptId,
				TEXT("A committed anchor cannot accept a different AttemptId."));
		}
		Fdemo_mapShanmenFormationSessionResult Result;
		Result.Status = Edemo_mapShanmenFormationSessionStatus::Replayed;
		Result.AnchorDefinitionId = AnchorDefinitionId;
		Result.AttemptId = AttemptId;
		Result.Material = Existing->Material;
		Result.DeploymentReceipt = Existing->DeploymentReceipt;
		Result.Diagnostic =
			TEXT("The exact anchor commit replayed its immutable audit.");
		return Result;
	}
	if (State != Edemo_mapShanmenFormationSessionState::Deploying)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::SessionInvalid,
			AnchorDefinitionId, AttemptId,
			TEXT("Only a deploying formation may commit another anchor."));
	}
	if (!bHasPendingMaterial)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::PendingMissing,
			AnchorDefinitionId, AttemptId,
			TEXT("Anchor commit requires its explicit prepared material attempt."));
	}
	if (PendingMaterial.AnchorDefinitionId != AnchorDefinitionId
		|| PendingMaterial.AttemptId != AttemptId)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::PendingConflict,
			AnchorDefinitionId, AttemptId,
			TEXT("Anchor commit does not match the sole pending material attempt."));
	}

	Fdemo_mapShanmenFormationMaterialResult Committed = PendingMaterial;
	if (!Committed.IsCommitted())
	{
		Fdemo_mapShanmenFormationMaterialResult Reconstructed =
			Fdemo_mapShanmenFormationMaterialAdapter::PrepareMaterials(
				Authority, Correlation, Deployment,
				AnchorDefinitionId, AttemptId);
		if (!Reconstructed.IsPrepared())
		{
			Fdemo_mapShanmenFormationSessionResult Result = Reject(
				Edemo_mapShanmenFormationSessionStatus::MaterialRejected,
				AnchorDefinitionId, AttemptId, Reconstructed.Diagnostic);
			Result.Material = MoveTemp(Reconstructed);
			return Result;
		}
		Committed = Fdemo_mapShanmenFormationMaterialAdapter::CommitMaterials(
			Authority, Correlation, Deployment, Reconstructed);
		if (!Committed.IsCommitted())
		{
			Fdemo_mapShanmenFormationSessionResult Result = Reject(
				Committed.Status
					== Edemo_mapShanmenFormationMaterialStatus::CommitRecoveryRequired
					? Edemo_mapShanmenFormationSessionStatus::MaterialCommitRecoveryRequired
					: Edemo_mapShanmenFormationSessionStatus::MaterialRejected,
				AnchorDefinitionId, AttemptId, Committed.Diagnostic);
			Result.Material = MoveTemp(Committed);
			return Result;
		}
	}

	Fdemo_mapShanmenFormationProductSession Candidate = *this;
	Candidate.PendingMaterial = Committed;
	Candidate.bHasPendingMaterial = true;
	FShanmenFormationDeploymentReceipt DeploymentReceipt;
	if (!Candidate.Deployment.TryCommitAnchor(
			Candidate.ActionRuntime, Committed.Evidence, DeploymentReceipt))
	{
		// Material is already durable. Preserve it as the only pending value so
		// exact replay can finish the pure deployment edge without re-consuming.
		PendingMaterial = Committed;
		bHasPendingMaterial = true;
		Fdemo_mapShanmenFormationSessionResult Result = Reject(
			Edemo_mapShanmenFormationSessionStatus::DeploymentRecoveryRequired,
			AnchorDefinitionId, AttemptId,
			TEXT("Materials committed; replay this exact attempt to finish deployment commit."));
		Result.Material = Committed;
		return Result;
	}

	Fdemo_mapShanmenFormationAnchorAudit& Audit =
		Candidate.AnchorAudits.AddDefaulted_GetRef();
	Audit.AnchorDefinitionId = AnchorDefinitionId;
	Audit.AttemptId = AttemptId;
	Audit.Material = Committed;
	Audit.DeploymentReceipt = DeploymentReceipt;
	Candidate.ClearPendingMaterial();
	Candidate.State = Candidate.Deployment.GetState()
		== EShanmenFormationDeploymentState::Active
		? Edemo_mapShanmenFormationSessionState::Active
		: Edemo_mapShanmenFormationSessionState::Deploying;
	if (!Candidate.IsValid())
	{
		PendingMaterial = Committed;
		bHasPendingMaterial = true;
		Fdemo_mapShanmenFormationSessionResult Result = Reject(
			Edemo_mapShanmenFormationSessionStatus::DeploymentRecoveryRequired,
			AnchorDefinitionId, AttemptId,
			TEXT("Materials committed but post-commit session invariants require exact replay."));
		Result.Material = Committed;
		return Result;
	}

	*this = MoveTemp(Candidate);
	Fdemo_mapShanmenFormationSessionResult Result;
	Result.Status = Edemo_mapShanmenFormationSessionStatus::Committed;
	Result.AnchorDefinitionId = AnchorDefinitionId;
	Result.AttemptId = AttemptId;
	Result.Material = Committed;
	Result.DeploymentReceipt = DeploymentReceipt;
	Result.Diagnostic =
		TEXT("Durable materials and the exact formation anchor committed atomically at the product boundary.");
	return Result;
}

Fdemo_mapShanmenFormationSessionResult
Fdemo_mapShanmenFormationProductSession::TryCancel(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	if (!IsValid() || State != Edemo_mapShanmenFormationSessionState::Deploying)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::SessionInvalid,
			NAME_None, FGuid(),
			TEXT("Only a valid deploying formation may cancel."));
	}
	if (RequestedCorrelation != Correlation)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::CorrelationMismatch,
			NAME_None, FGuid(),
			TEXT("Formation cancellation rejected a stale or foreign Run correlation."));
	}

	Fdemo_mapShanmenFormationProductSession Candidate = *this;
	FShanmenFormationDeploymentReceipt DeploymentReceipt;
	FShanmenActionTransitionReceipt Interrupted;
	if (!Candidate.Deployment.TryCancel(
			Candidate.ActionRuntime, DeploymentReceipt))
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::DeploymentTransitionRejected,
			NAME_None, FGuid(),
			TEXT("Pure formation deployment rejected cancellation."));
	}
	if (!Candidate.ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active, Interrupted))
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::ActionTransitionRejected,
			NAME_None, FGuid(),
			TEXT("Formation action rejected cancellation interruption."));
	}

	Fdemo_mapShanmenFormationMaterialResult CancelledMaterial;
	if (bHasPendingMaterial)
	{
		if (PendingMaterial.IsCommitted())
		{
			Fdemo_mapShanmenFormationSessionResult Result = Reject(
				Edemo_mapShanmenFormationSessionStatus::MaterialCommitRecoveryRequired,
				PendingMaterial.AnchorDefinitionId,
				PendingMaterial.AttemptId,
				TEXT("Committed material cannot roll back; finish the pending anchor commit first."));
			Result.Material = PendingMaterial;
			return Result;
		}
		const Fdemo_mapShanmenFormationMaterialResult Reconstructed =
			Fdemo_mapShanmenFormationMaterialAdapter::PrepareMaterials(
				Authority, Correlation, Deployment,
				PendingMaterial.AnchorDefinitionId,
				PendingMaterial.AttemptId);
		if (!Reconstructed.IsPrepared())
		{
			Fdemo_mapShanmenFormationSessionResult Result = Reject(
				Edemo_mapShanmenFormationSessionStatus::MaterialRejected,
				PendingMaterial.AnchorDefinitionId,
				PendingMaterial.AttemptId,
				Reconstructed.Diagnostic);
			Result.Material = Reconstructed;
			return Result;
		}
		CancelledMaterial =
			Fdemo_mapShanmenFormationMaterialAdapter::CancelMaterials(
				Authority, Correlation, Deployment, Reconstructed);
		if (!CancelledMaterial.IsCancelled())
		{
			const bool bForwardOnly = CancelledMaterial.Status
				== Edemo_mapShanmenFormationMaterialStatus::CommitRecoveryRequired;
			Fdemo_mapShanmenFormationSessionResult Result = Reject(
				bForwardOnly
					? Edemo_mapShanmenFormationSessionStatus::MaterialCommitRecoveryRequired
					: Edemo_mapShanmenFormationSessionStatus::MaterialCancelRecoveryRequired,
				PendingMaterial.AnchorDefinitionId,
				PendingMaterial.AttemptId,
				CancelledMaterial.Diagnostic);
			Result.Material = CancelledMaterial;
			return Result;
		}
		Candidate.ClearPendingMaterial();
	}

	Candidate.State = Edemo_mapShanmenFormationSessionState::Cancelled;
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::ActionTransitionRejected,
			NAME_None, FGuid(),
			TEXT("Formation cancellation failed post-transition invariants."));
	}
	*this = MoveTemp(Candidate);
	Fdemo_mapShanmenFormationSessionResult Result;
	Result.Status = Edemo_mapShanmenFormationSessionStatus::Cancelled;
	Result.Material = MoveTemp(CancelledMaterial);
	Result.DeploymentReceipt = DeploymentReceipt;
	Result.ActionReceipt = Interrupted;
	Result.Diagnostic =
		TEXT("Pending materials were released before deployment and action cancellation.");
	return Result;
}

Fdemo_mapShanmenFormationSessionResult
Fdemo_mapShanmenFormationProductSession::TryEnd(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	if (!IsValid() || State != Edemo_mapShanmenFormationSessionState::Active
		|| bHasPendingMaterial)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::SessionInvalid,
			NAME_None, FGuid(),
			TEXT("Only a complete active formation with no pending material may end."));
	}
	if (RequestedCorrelation != Correlation)
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::CorrelationMismatch,
			NAME_None, FGuid(),
			TEXT("Formation end rejected a stale or foreign Run correlation."));
	}

	Fdemo_mapShanmenFormationProductSession Candidate = *this;
	FShanmenFormationDeploymentReceipt DeploymentReceipt;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	if (!Candidate.Deployment.TryEnd(
			Candidate.ActionRuntime, DeploymentReceipt))
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::DeploymentTransitionRejected,
			NAME_None, FGuid(),
			TEXT("Pure formation deployment rejected end."));
	}
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active, Recovery)
		|| !Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery, Completed))
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::ActionTransitionRejected,
			NAME_None, FGuid(),
			TEXT("Formation action rejected Recovery -> Completed end."));
	}
	Candidate.State = Edemo_mapShanmenFormationSessionState::Ended;
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationSessionStatus::ActionTransitionRejected,
			NAME_None, FGuid(),
			TEXT("Formation end failed post-transition invariants."));
	}
	*this = MoveTemp(Candidate);
	Fdemo_mapShanmenFormationSessionResult Result;
	Result.Status = Edemo_mapShanmenFormationSessionStatus::Ended;
	Result.DeploymentReceipt = DeploymentReceipt;
	Result.ActionReceipt = Recovery;
	Result.CompletionReceipt = Completed;
	Result.Diagnostic =
		TEXT("Active formation ended with its action fully completed.");
	return Result;
}

bool Fdemo_mapShanmenFormationProductSession::IsValid() const
{
	if (State == Edemo_mapShanmenFormationSessionState::Empty)
	{
		return false;
	}
	if (!Correlation.IsValid() || !ActionRuntime.IsValid()
		|| !Deployment.IsValid()
		|| Correlation.ActiveRunId
			!= ActionRuntime.GetAction().GetRunId()
		|| Correlation.OwnerId != ActionRuntime.GetAction().GetOwnerId()
		|| !ActionsMatch(
			ActionRuntime.GetAction(), Deployment.GetAction())
		|| AnchorAudits.Num() != Deployment.GetCommittedAnchorCount())
	{
		return false;
	}

	TSet<FName> AuditedAnchors;
	TSet<FGuid> AttemptIds;
	for (const Fdemo_mapShanmenFormationAnchorAudit& Audit : AnchorAudits)
	{
		if (!Audit.IsValid()
			|| AuditedAnchors.Contains(Audit.AnchorDefinitionId)
			|| AttemptIds.Contains(Audit.AttemptId)
			|| Audit.Material.ActiveRunId != Correlation.ActiveRunId
			|| Audit.Material.OwnerId != Correlation.OwnerId
			|| Audit.Material.DeploymentId != Deployment.GetDeploymentId())
		{
			return false;
		}
		const FShanmenFormationAnchorProgress* Progress =
			Deployment.GetAnchors().FindByPredicate(
				[&Audit](const FShanmenFormationAnchorProgress& Candidate)
				{
					return Candidate.GetAnchorDefinitionId()
						== Audit.AnchorDefinitionId;
				});
		if (!Progress || !Progress->IsCommitted()
			|| !EvidenceMatches(
				Progress->GetFulfillment(), Audit.Material.Evidence))
		{
			return false;
		}
		AuditedAnchors.Add(Audit.AnchorDefinitionId);
		AttemptIds.Add(Audit.AttemptId);
	}

	if (bHasPendingMaterial)
	{
		const FShanmenFormationAnchorProgress* Progress =
			Deployment.GetAnchors().FindByPredicate(
				[this](const FShanmenFormationAnchorProgress& Candidate)
				{
					return Candidate.GetAnchorDefinitionId()
						== PendingMaterial.AnchorDefinitionId;
				});
		if (State != Edemo_mapShanmenFormationSessionState::Deploying
			|| !IsPendingMaterialValid(PendingMaterial)
			|| PendingMaterial.ActiveRunId != Correlation.ActiveRunId
			|| PendingMaterial.OwnerId != Correlation.OwnerId
			|| PendingMaterial.DeploymentId != Deployment.GetDeploymentId()
			|| AuditedAnchors.Contains(PendingMaterial.AnchorDefinitionId)
			|| AttemptIds.Contains(PendingMaterial.AttemptId)
			|| !Progress || Progress->IsCommitted())
		{
			return false;
		}
	}
	else if (PendingMaterial.HasPlan())
	{
		return false;
	}

	switch (State)
	{
	case Edemo_mapShanmenFormationSessionState::Deploying:
		return !ActionRuntime.IsTerminal()
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Active
			&& Deployment.GetState()
				== EShanmenFormationDeploymentState::Deploying;

	case Edemo_mapShanmenFormationSessionState::Active:
		return !bHasPendingMaterial && !ActionRuntime.IsTerminal()
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Active
			&& Deployment.GetState()
				== EShanmenFormationDeploymentState::Active
			&& AnchorAudits.Num() == Deployment.GetAnchors().Num();

	case Edemo_mapShanmenFormationSessionState::Cancelled:
		return !bHasPendingMaterial && ActionRuntime.IsTerminal()
			&& ActionRuntime.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& Deployment.GetState()
				== EShanmenFormationDeploymentState::Cancelled;

	case Edemo_mapShanmenFormationSessionState::Ended:
		return !bHasPendingMaterial && ActionRuntime.IsTerminal()
			&& ActionRuntime.GetTerminalReason()
				== EShanmenActionTerminalReason::Completed
			&& Deployment.GetState()
				== EShanmenFormationDeploymentState::Ended;

	case Edemo_mapShanmenFormationSessionState::Empty:
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationProductSession::IsTerminal() const
{
	return IsValid()
		&& (State == Edemo_mapShanmenFormationSessionState::Cancelled
			|| State == Edemo_mapShanmenFormationSessionState::Ended);
}

void Fdemo_mapShanmenFormationProductSession::Reset()
{
	*this = Fdemo_mapShanmenFormationProductSession();
}

const Fdemo_mapShanmenFormationAnchorAudit*
Fdemo_mapShanmenFormationProductSession::FindAudit(
	const FName AnchorDefinitionId) const
{
	return AnchorAudits.FindByPredicate(
		[AnchorDefinitionId](
			const Fdemo_mapShanmenFormationAnchorAudit& Candidate)
		{
			return Candidate.AnchorDefinitionId == AnchorDefinitionId;
		});
}

void Fdemo_mapShanmenFormationProductSession::ClearPendingMaterial()
{
	bHasPendingMaterial = false;
	PendingMaterial = Fdemo_mapShanmenFormationMaterialResult();
}
