#include "demo_mapShanmenThrownWeaponWorldAdapter.h"

#include "Engine/HitResult.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

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

	bool ContextMatchesExecution(
		const FShanmenWorldHitContext& Context,
		const FShanmenThrownWeaponExecution& Execution)
	{
		return Context.IsValid()
			&& Execution.IsValid()
			&& ActionsMatch(Context.GetAction(), Execution.GetAction())
			&& Context.GetDetectorKind()
				== EShanmenHitDetectorKind::Projectile
			&& Context.GetDetectorId()
				== Execution.GetDefinition().GetDetectorId()
			&& Context.GetHitOrdinal() == 0;
	}

	bool FinalizeRequestsMatch(
		const FShanmenItemRunQuantityIntentFinalizeRequest& Left,
		const FShanmenItemRunQuantityIntentFinalizeRequest& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Context.RunId == Right.Context.RunId
			&& Left.Context.OwnerId == Right.Context.OwnerId
			&& Left.Context.RequestId == Right.Context.RequestId
			&& Left.Context.Content.Version
				== Right.Context.Content.Version
			&& Left.Context.Content.Digest == Right.Context.Content.Digest
			&& Left.ActiveRunId == Right.ActiveRunId
			&& Left.PrepareRequestId == Right.PrepareRequestId
			&& Left.IntentId == Right.IntentId
			&& Left.ItemInstanceId == Right.ItemInstanceId
			&& Left.bCommit == Right.bCommit;
	}

	bool CanPublish(
		const FShanmenActionOrchestrator& ActionRuntime,
		const Fdemo_mapShanmenThrownWeaponLaunchPlan& Plan,
		const FShanmenThrownWeaponExecution& Execution,
		const Ademo_mapShanmenThrownWeaponProjectile& Projectile)
	{
		return ActionRuntime.IsValid()
			&& ActionRuntime.CanEmitCandidates()
			&& Plan.IsValid()
			&& Execution.IsValid()
			&& Execution.GetState() == EShanmenThrownWeaponState::Ready
			&& !Execution.IsEmissionActive()
			&& ActionsMatch(ActionRuntime.GetAction(), Execution.GetAction())
			&& ActionsMatch(Execution.GetAction(), Plan.Launch.GetAction())
			&& Projectile.IsStagedFor(Plan.Launch, Plan.Context);
	}

	bool FlightMatches(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenThrownWeaponExecution& Execution,
		const Ademo_mapShanmenThrownWeaponProjectile& Projectile)
	{
		return ActionRuntime.IsValid()
			&& ActionRuntime.CanEmitCandidates()
			&& Execution.IsValid()
			&& Execution.GetState() == EShanmenThrownWeaponState::InFlight
			&& Execution.IsEmissionActive()
			&& ActionsMatch(ActionRuntime.GetAction(), Execution.GetAction())
			&& ContextMatchesExecution(
				Projectile.GetHitContext(), Execution)
			&& Projectile.IsInFlightFor(
				Execution.GetLaunchReceipt(), Projectile.GetHitContext());
	}
}

bool Fdemo_mapShanmenThrownWeaponLaunchPlan::IsValid() const
{
	return Correlation.IsValid()
		&& Preparation.IsPrepared()
		&& ItemCommitRequest.Status
			== Edemo_mapShanmenThrownWeaponItemStatus::RequestReady
		&& ItemCommitRequest.FinalizeRequest.IsValid()
		&& ItemCommitRequest.FinalizeRequest.bCommit
		&& ExecutionCandidate.IsValid()
		&& ExecutionCandidate.GetState()
			== EShanmenThrownWeaponState::InFlight
		&& ExecutionCandidate.IsEmissionActive()
		&& Launch.IsValid()
		&& ExecutionCandidate.GetLaunchReceipt().GetLaunchId()
			== Launch.GetLaunchId()
		&& ActionsMatch(Preparation.Action, Launch.GetAction())
		&& ContextMatchesExecution(Context, ExecutionCandidate);
}

Fdemo_mapShanmenThrownWeaponLaunchResult
Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenThrownWeaponExecution& Execution,
	Ademo_mapShanmenThrownWeaponProjectile& Projectile,
	AActor* SourceActor,
	const FVector& Origin,
	const FVector& AimDirection)
{
	Fdemo_mapShanmenThrownWeaponLaunchResult Result;
	if (!Correlation.IsValid())
	{
		return Result;
	}
	if (!Preparation.IsPrepared())
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::PreparationInvalid;
		return Result;
	}
	if (!ActionRuntime.IsValid()
		|| !ActionRuntime.CanEmitCandidates()
		|| !ActionsMatch(ActionRuntime.GetAction(), Preparation.Action))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::ActionRuntimeInvalid;
		return Result;
	}
	if (!Execution.IsValid()
		|| Execution.GetState() != EShanmenThrownWeaponState::Ready
		|| Execution.IsEmissionActive()
		|| !ActionsMatch(Execution.GetAction(), Preparation.Action))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::ExecutionInvalid;
		return Result;
	}

	Result.Plan.Correlation = Correlation;
	Result.Plan.Preparation = Preparation;
	Result.Plan.ExecutionCandidate = Execution;
	if (!Result.Plan.ExecutionCandidate.TryLaunchStraight(
		ActionRuntime, Origin, AimDirection, Result.Plan.Launch))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponLaunchError::LaunchRejected;
		return Result;
	}
	if (!Result.Plan.ExecutionCandidate.TryBeginEmission(
		ActionRuntime, Result.Plan.Context))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponLaunchError::EmissionRejected;
		return Result;
	}
	Result.Plan.ItemCommitRequest =
		Fdemo_mapShanmenThrownWeaponItemAdapter::BuildCommitRequest(
			Correlation, Preparation, Result.Plan.Launch);
	if (Result.Plan.ItemCommitRequest.Status
		!= Edemo_mapShanmenThrownWeaponItemStatus::RequestReady)
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::ItemCommitRequestRejected;
		return Result;
	}
	if (!Projectile.TryStageLaunch(
		Result.Plan.Launch, Result.Plan.Context, SourceActor))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::ProjectileStageRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenThrownWeaponLaunchError::None;
	if (!Result.IsStaged())
	{
		Projectile.CancelStagedLaunch();
		return Fdemo_mapShanmenThrownWeaponLaunchResult();
	}
	return Result;
}

Fdemo_mapShanmenThrownWeaponLaunchResult
Fdemo_mapShanmenThrownWeaponWorldAdapter::CommitStagedLaunch(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FShanmenActionOrchestrator& ActionRuntime,
	const Fdemo_mapShanmenThrownWeaponLaunchPlan& Plan,
	FShanmenThrownWeaponExecution& Execution,
	Ademo_mapShanmenThrownWeaponProjectile& Projectile)
{
	Fdemo_mapShanmenThrownWeaponLaunchResult Result;
	Result.Plan = Plan;
	if (!CanPublish(ActionRuntime, Plan, Execution, Projectile))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::PublicationRejected;
		return Result;
	}

	Result.ItemCommit =
		Fdemo_mapShanmenThrownWeaponItemAdapter::CommitLaunched(
			Authority, Plan.Correlation, Plan.Preparation, Plan.Launch);
	if (Result.ItemCommit.Status
		!= Edemo_mapShanmenThrownWeaponItemStatus::Committed
		|| !Result.ItemCommit.IsFinalized())
	{
		Projectile.CancelStagedLaunch();
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::AuthorityCommitRejected;
		return Result;
	}
	if (!PublishCommittedLaunch(
		ActionRuntime,
		Plan,
		Result.ItemCommit,
		Execution,
		Projectile))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::PublicationRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenThrownWeaponLaunchError::None;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponWorldAdapter::PublishCommittedLaunch(
	const FShanmenActionOrchestrator& ActionRuntime,
	const Fdemo_mapShanmenThrownWeaponLaunchPlan& Plan,
	const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
	FShanmenThrownWeaponExecution& Execution,
	Ademo_mapShanmenThrownWeaponProjectile& Projectile)
{
	if (!CanPublish(ActionRuntime, Plan, Execution, Projectile)
		|| CommittedItem.Status
			!= Edemo_mapShanmenThrownWeaponItemStatus::Committed
		|| !CommittedItem.IsFinalized()
		|| !CommittedItem.FinalizeRequest.bCommit
		|| !ActionsMatch(CommittedItem.Action, Plan.Preparation.Action)
		|| !FinalizeRequestsMatch(
			CommittedItem.FinalizeRequest,
			Plan.ItemCommitRequest.FinalizeRequest))
	{
		return false;
	}

	Execution = Plan.ExecutionCandidate;
	Projectile.ActivateCommittedLaunch();
	return FlightMatches(ActionRuntime, Execution, Projectile);
}

Fdemo_mapShanmenThrownWeaponWorldDeliveryResult
Fdemo_mapShanmenThrownWeaponWorldAdapter::ResolveProjectileContact(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenThrownWeaponExecution& Execution,
	Ademo_mapShanmenThrownWeaponProjectile& Projectile,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FHitResult& Hit)
{
	Fdemo_mapShanmenThrownWeaponWorldDeliveryResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!FlightMatches(ActionRuntime, Execution, Projectile))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::FlightNotActive;
		return Result;
	}
	if (Execution.GetAction().GetRunId() != Coordinator.GetRunId())
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::ContextMismatch;
		return Result;
	}

	FShanmenHitCandidate Candidate;
	if (!FShanmenWorldHitAdapter::TryFromProjectile(
		Projectile.GetHitContext(),
		Hit,
		Coordinator.GetEntityRegistry(),
		Candidate))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::ContactNotResolved;
		return Result;
	}
	Result.TargetEntityId = Candidate.TargetEntityId;
	AActor* TargetActor = Hit.GetActor();
	Idemo_mapCombatVitalityHost* VitalityHost = TargetActor
		? Cast<Idemo_mapCombatVitalityHost>(TargetActor)
		: nullptr;
	FShanmenTargetVitalitySnapshot Vitality;
	if (!VitalityHost
		|| !VitalityHost->IsCombatEntityBound()
		|| VitalityHost->GetCombatEntityId() != Candidate.TargetEntityId
		|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Vitality))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::TargetVitalityUnavailable;
		return Result;
	}

	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenThrownWeaponExecution ExecutionCandidate = Execution;
	if (!ExecutionCandidate.TryResolveCandidate(
		ActionRuntime,
		Candidate,
		Vitality,
		Defense,
		Result.Impact))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::CandidateRejected;
		return Result;
	}
	if (!ExecutionCandidate.TryEndEmission(ActionRuntime)
		|| !ExecutionCandidate.TryFinishFlight(ActionRuntime))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::FlightTerminationRejected;
		return Result;
	}

	Result.Delivery = Coordinator.DeliverThrownWeaponImpactToM01Enemy(
		Result.Impact, TargetActor);
	if (!Result.Delivery.IsSuccess())
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponWorldDeliveryError::DeliveryRejected;
		return Result;
	}

	Execution = MoveTemp(ExecutionCandidate);
	check(Projectile.MarkSpent());
	Result.Error = Edemo_mapShanmenThrownWeaponWorldDeliveryError::None;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponWorldAdapter::FinishFlightWithoutImpact(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenThrownWeaponExecution& Execution,
	Ademo_mapShanmenThrownWeaponProjectile& Projectile)
{
	if (!FlightMatches(ActionRuntime, Execution, Projectile))
	{
		return false;
	}
	FShanmenThrownWeaponExecution Candidate = Execution;
	if (!Candidate.TryEndEmission(ActionRuntime)
		|| !Candidate.TryFinishFlight(ActionRuntime))
	{
		return false;
	}
	Execution = MoveTemp(Candidate);
	return Projectile.MarkSpent();
}
