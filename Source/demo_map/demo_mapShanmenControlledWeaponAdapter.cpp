#include "demo_mapShanmenControlledWeaponAdapter.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenItemTags.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool DefinitionsMatch(
		const FShanmenControlledWeaponDefinition& Left,
		const FShanmenControlledWeaponDefinition& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetDetectorId() == Right.GetDetectorId()
			&& Left.GetFormulaId() == Right.GetFormulaId()
			&& Left.GetBaseDamage() == Right.GetBaseDamage()
			&& Left.GetControlPowerCoefficient()
				== Right.GetControlPowerCoefficient()
			&& Left.GetDamageTags() == Right.GetDamageTags()
			&& Left.GetRequiredTargetTags()
				== Right.GetRequiredTargetTags()
			&& Left.RejectsSelf() == Right.RejectsSelf();
	}

	template <typename TValue>
	const TValue* FindById(
		const TArray<TValue>& Values,
		TFunctionRef<bool(const TValue&)> Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult Reject(
		const Edemo_mapShanmenControlledWeaponPrepareStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenControlledWeaponPrepareResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenControlledWeaponPrepareRequest::IsValid() const
{
	return SourceEntityId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& FMath::IsFinite(ControlPower)
		&& ControlPower >= 0.0f;
}

bool Fdemo_mapShanmenControlledWeaponAuthorityEvidence::IsValid() const
{
	return CorrelationId.IsValid()
		&& ActiveRunId.IsValid()
		&& OwnerId.IsValid()
		&& ItemInstanceId.IsValid()
		&& !ItemDefinitionId.IsNone()
		&& DeploymentReservationId.IsValid()
		&& AuthorityRevision >= 0
		&& ItemRevision >= 0
		&& Content.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponPrepareResult::IsPrepared() const
{
	return Status == Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared
		&& Evidence.IsValid()
		&& Action.IsValid()
		&& Definition.IsValid()
		&& Offense.IsValid()
		&& Execution.IsValid()
		&& Action.GetRunId() == Evidence.ActiveRunId
		&& Action.GetOwnerId() == Evidence.OwnerId
		&& Action.GetSourceItemInstanceId() == Evidence.ItemInstanceId
		&& Action.GetContent().Version == Evidence.Content.Version
		&& Action.GetContent().Digest == Evidence.Content.Digest
		&& Action.GetSourceTags().HasTagExact(
			FShanmenItemNativeTags::ItemWeaponFlyingSword())
		&& Action.GetActionDefinitionId()
			== Definition.GetActionDefinitionId()
		&& ActionsMatch(Action, Execution.GetAction())
		&& DefinitionsMatch(Definition, Execution.GetDefinition())
		&& Offense.GetControlPower()
			== Execution.GetOffense().GetControlPower();
}

Fdemo_mapShanmenControlledWeaponPrepareResult
Fdemo_mapShanmenControlledWeaponAdapter::PrepareActiveRun(
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Udemo_mapItemSubsystem& Runtime,
	const Fdemo_mapShanmenControlledWeaponPrepareRequest& Request)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::AuthorityNotReady,
			TEXT("Controlled-weapon preparation requires the ready item authority on the Game Thread."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FString Diagnostic;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Diagnostic))
	{
		Fdemo_mapShanmenControlledWeaponPrepareResult Result = Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::RunCorrelationInvalid,
			TEXT("Controlled-weapon preparation could not reconstruct the durable active Run."));
		Result.Diagnostic = Diagnostic.IsEmpty()
			? Result.Diagnostic : Diagnostic;
		return Result;
	}
	if (Runtime.GetRunState() != Edemo_mapRunState::Active
		|| Runtime.GetActiveRunId() != Correlation.ActiveRunId)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::RuntimeRunMismatch,
			TEXT("Transient Runtime does not match the durable active-Run correlation."));
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::SnapshotUnavailable,
			TEXT("The ready item authority could not provide a read-only snapshot."));
	}
	return PrepareFromEvidence(Snapshot, Correlation, Request);
}

Fdemo_mapShanmenControlledWeaponPrepareResult
Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenControlledWeaponPrepareRequest& Request)
{
	if (!Request.IsValid())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::RequestInvalid,
			TEXT("Controlled-weapon activation input is incomplete or non-finite."));
	}
	if (!Correlation.IsValid())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::RunCorrelationInvalid,
			TEXT("Controlled-weapon activation requires one valid immutable Run correlation."));
	}
	if (!Snapshot.Content.IsValid()
		|| Snapshot.AuthorityRevision < Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::SnapshotStale,
			TEXT("Item evidence predates the active-Run lifecycle or has no content identity."));
	}
	if (Request.SourceItemInstanceId != Correlation.WeaponItemInstanceId
		|| !Correlation.OrderedPreparedItemInstanceIds.Contains(
			Request.SourceItemInstanceId))
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::SourceItemMismatch,
			TEXT("Only the exact prepared weapon-slot identity may source this controlled action."));
	}

	const FShanmenItemInstance* Item = FindById<FShanmenItemInstance>(
		Snapshot.Items,
		[&Request](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == Request.SourceItemInstanceId;
		});
	if (!Item)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::ItemNotFound,
			TEXT("The prepared weapon identity is absent from the current authority snapshot."));
	}
	if (Item->State != EShanmenItemInstanceState::Deployed
		|| Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId
		|| Item->Quantity != 1
		|| !Item->DeploymentReservationId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::ItemNotDeployed,
			TEXT("The exact weapon is not a live deployed singleton in this Run scope."));
	}

	const FShanmenItemDefinition* ItemDefinition =
		FindById<FShanmenItemDefinition>(
			Snapshot.Definitions,
			[Item](const FShanmenItemDefinition& Candidate)
			{
				return Candidate.DefinitionId == Item->DefinitionId;
			});
	if (!ItemDefinition || !ItemDefinition->IsValid()
		|| !ItemDefinition->Supports(
			EShanmenItemResourceKind::DeploymentLock)
		|| !ItemDefinition->ItemTags.HasTagExact(
			FShanmenItemNativeTags::ItemWeaponFlyingSword()))
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::DefinitionNotFlyingSword,
			TEXT("A deployment-capable item is not a flying sword unless its authority definition says so."));
	}

	const FShanmenItemReservationSnapshot* Deployment =
		FindById<FShanmenItemReservationSnapshot>(
			Snapshot.Reservations,
			[Item](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId
					== Item->DeploymentReservationId;
			});
	if (!Deployment || !Deployment->IsValid()
		|| Deployment->State != EShanmenItemReservationState::Committed
		|| Deployment->ResourceKind
			!= EShanmenItemResourceKind::DeploymentLock
		|| Deployment->ItemInstanceId != Item->ItemInstanceId
		|| Deployment->RunId != Correlation.ScopeId
		|| Deployment->OwnerId != Correlation.OwnerId
		|| Deployment->Amount != 1
		|| Item->Revision <= Deployment->ItemRevisionAtReserve)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::DeploymentEvidenceInvalid,
			TEXT("The flying sword has no exact committed DeploymentLock evidence."));
	}

	FShanmenControlledWeaponDefinition FrozenDefinition;
	FShanmenControlledWeaponOffenseSnapshot FrozenOffense;
	if (!FShanmenControlledWeaponDefinition::TryCapture(
			Request.Definition, FrozenDefinition)
		|| !FShanmenControlledWeaponOffenseSnapshot::TryCapture(
			Request.ControlPower, FrozenOffense))
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::RequestInvalid,
			TEXT("Product-authored controlled-weapon values failed immutable capture."));
	}

	FShanmenCombatActionCapture ActionCapture;
	ActionCapture.RunId = Correlation.ActiveRunId;
	ActionCapture.OwnerId = Correlation.OwnerId;
	ActionCapture.SourceEntityId = Request.SourceEntityId;
	ActionCapture.SourceItemInstanceId = Request.SourceItemInstanceId;
	ActionCapture.ActionDefinitionId =
		FrozenDefinition.GetActionDefinitionId();
	ActionCapture.Content = Snapshot.Content;
	ActionCapture.SourceTags = Request.SourceTags;
	ActionCapture.SourceTags.AppendTags(ItemDefinition->ItemTags);
	ActionCapture.ActivationId =
		FShanmenCombatIdFactory::MakeActivationId(
			ActionCapture.RunId,
			ActionCapture.SourceEntityId,
			ActionCapture.ActionDefinitionId,
			Request.ActivationSequence);

	FShanmenCombatActionSnapshot Action;
	FShanmenControlledWeaponExecution Execution;
	if (!FShanmenCombatActionSnapshot::TryCapture(ActionCapture, Action)
		|| !FShanmenControlledWeaponExecution::TryCreate(
			Action, FrozenDefinition, FrozenOffense, Execution))
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::CombatCaptureRejected,
			TEXT("Validated flying-sword evidence could not enter the deterministic combat contract."));
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult Result;
	Result.Status = Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
	Result.Diagnostic =
		TEXT("Exact deployed flying-sword authority evidence was captured without mutation.");
	Result.Evidence.CorrelationId = Correlation.CorrelationId;
	Result.Evidence.ActiveRunId = Correlation.ActiveRunId;
	Result.Evidence.OwnerId = Correlation.OwnerId;
	Result.Evidence.ItemInstanceId = Item->ItemInstanceId;
	Result.Evidence.ItemDefinitionId = Item->DefinitionId;
	Result.Evidence.DeploymentReservationId =
		Item->DeploymentReservationId;
	Result.Evidence.AuthorityRevision = Snapshot.AuthorityRevision;
	Result.Evidence.ItemRevision = Item->Revision;
	Result.Evidence.Content = Snapshot.Content;
	Result.Action = Action;
	Result.Definition = FrozenDefinition;
	Result.Offense = FrozenOffense;
	Result.Execution = MoveTemp(Execution);
	if (!Result.IsPrepared())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponPrepareStatus::CombatCaptureRejected,
			TEXT("Prepared controlled-weapon result failed its final cross-boundary invariants."));
	}
	return Result;
}
