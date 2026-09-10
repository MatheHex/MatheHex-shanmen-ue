#include "demo_mapShanmenSwordQiWorldAdapter.h"

#include "Engine/HitResult.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "demo_mapCombatVitalityHost.h"

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
		const FShanmenSwordQiExecution& Execution)
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

	bool SourceMatchesRun(
		const FShanmenCombatActionSnapshot& Action,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const AActor* SourceActor)
	{
		FGuid SourceEntityId;
		return ::IsValid(SourceActor)
			&& Action.IsValid()
			&& Action.GetRunId() == Coordinator.GetRunId()
			&& Action.GetSourceEntityId() == Coordinator.GetPlayerEntityId()
			&& Coordinator.GetEntityRegistry().TryResolveObject(
				Coordinator.GetRunId(),
				SourceActor,
				INDEX_NONE,
				SourceEntityId)
			&& SourceEntityId == Action.GetSourceEntityId();
	}

	bool CanPublish(
		const FShanmenActionOrchestrator& ActionRuntime,
		const Fdemo_mapShanmenSwordQiLaunchPlan& Plan,
		const FShanmenSwordQiExecution& Execution,
		const Ademo_mapShanmenSwordQiProjectile& Projectile)
	{
		return ActionRuntime.IsValid()
			&& ActionRuntime.CanEmitCandidates()
			&& Plan.IsValid()
			&& Execution.IsValid()
			&& Execution.GetState() == EShanmenSwordQiState::Ready
			&& !Execution.IsEmissionActive()
			&& ActionsMatch(ActionRuntime.GetAction(), Execution.GetAction())
			&& ActionsMatch(Execution.GetAction(), Plan.Launch.GetAction())
			&& Projectile.IsStagedFor(Plan.Launch, Plan.Context);
	}

	bool FlightMatches(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenSwordQiExecution& Execution,
		const Ademo_mapShanmenSwordQiProjectile& Projectile)
	{
		return ActionRuntime.IsValid()
			&& ActionRuntime.CanEmitCandidates()
			&& Execution.IsValid()
			&& Execution.GetState() == EShanmenSwordQiState::InFlight
			&& Execution.IsEmissionActive()
			&& ActionsMatch(ActionRuntime.GetAction(), Execution.GetAction())
			&& ContextMatchesExecution(
				Projectile.GetHitContext(), Execution)
			&& Projectile.IsInFlightFor(
				Execution.GetLaunchReceipt(), Projectile.GetHitContext());
	}

	bool CarrierMatchesExecution(
		const FShanmenSwordQiExecution& Execution,
		const Ademo_mapShanmenSwordQiProjectile& Projectile)
	{
		return Execution.IsValid()
			&& Execution.GetState() == EShanmenSwordQiState::InFlight
			&& Execution.IsEmissionActive()
			&& ContextMatchesExecution(
				Projectile.GetHitContext(), Execution)
			&& Projectile.IsInFlightFor(
				Execution.GetLaunchReceipt(), Projectile.GetHitContext());
	}
}

bool Fdemo_mapShanmenSwordQiLaunchPlan::IsValid() const
{
	return ExecutionCandidate.IsValid()
		&& ExecutionCandidate.GetState() == EShanmenSwordQiState::InFlight
		&& ExecutionCandidate.IsEmissionActive()
		&& Launch.IsValid()
		&& ExecutionCandidate.GetLaunchReceipt().GetLaunchId()
			== Launch.GetLaunchId()
		&& ActionsMatch(ExecutionCandidate.GetAction(), Launch.GetAction())
		&& ContextMatchesExecution(Context, ExecutionCandidate);
}

Fdemo_mapShanmenSwordQiLaunchResult
Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenSwordQiExecution& Execution,
	Ademo_mapShanmenSwordQiProjectile& Projectile,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* SourceActor,
	const FVector& Origin,
	const FVector& AimDirection)
{
	Fdemo_mapShanmenSwordQiLaunchResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!ActionRuntime.IsValid()
		|| ActionRuntime.GetAction().GetRunId() != Coordinator.GetRunId()
		|| ActionRuntime.GetAction().GetSourceEntityId()
			!= Coordinator.GetPlayerEntityId())
	{
		Result.Error = Edemo_mapShanmenSwordQiLaunchError::RunContextMismatch;
		return Result;
	}
	if (!SourceMatchesRun(
		ActionRuntime.GetAction(), Coordinator, SourceActor))
	{
		Result.Error = Edemo_mapShanmenSwordQiLaunchError::SourceNotRegistered;
		return Result;
	}
	if (!ActionRuntime.CanEmitCandidates()
		|| !ActionsMatch(ActionRuntime.GetAction(), Execution.GetAction()))
	{
		Result.Error =
			Edemo_mapShanmenSwordQiLaunchError::ActionRuntimeInvalid;
		return Result;
	}
	if (!Execution.IsValid()
		|| Execution.GetState() != EShanmenSwordQiState::Ready
		|| Execution.IsEmissionActive())
	{
		Result.Error = Edemo_mapShanmenSwordQiLaunchError::ExecutionInvalid;
		return Result;
	}

	Result.Plan.ExecutionCandidate = Execution;
	if (!Result.Plan.ExecutionCandidate.TryLaunch(
		ActionRuntime, Origin, AimDirection, Result.Plan.Launch))
	{
		Result.Error = Edemo_mapShanmenSwordQiLaunchError::LaunchRejected;
		return Result;
	}
	if (!Result.Plan.ExecutionCandidate.TryBeginEmission(
		ActionRuntime, Result.Plan.Context))
	{
		Result.Error = Edemo_mapShanmenSwordQiLaunchError::EmissionRejected;
		return Result;
	}
	Edemo_mapShanmenSwordQiProjectileStageError StageError =
		Edemo_mapShanmenSwordQiProjectileStageError::ContractRejected;
	if (!Projectile.TryStageLaunch(
		Result.Plan.Launch,
		Result.Plan.Context,
		SourceActor,
		&StageError))
	{
		Result.Error = StageError
			== Edemo_mapShanmenSwordQiProjectileStageError::LaunchPathBlocked
			? Edemo_mapShanmenSwordQiLaunchError::LaunchPathBlocked
			: Edemo_mapShanmenSwordQiLaunchError::ProjectileStageRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenSwordQiLaunchError::None;
	if (!Result.IsStaged())
	{
		Projectile.CancelStagedLaunch();
		return Fdemo_mapShanmenSwordQiLaunchResult();
	}
	return Result;
}

bool Fdemo_mapShanmenSwordQiWorldAdapter::PublishStagedLaunch(
	const FShanmenActionOrchestrator& ActionRuntime,
	const Fdemo_mapShanmenSwordQiLaunchPlan& Plan,
	FShanmenSwordQiExecution& Execution,
	Ademo_mapShanmenSwordQiProjectile& Projectile)
{
	if (!CanPublish(ActionRuntime, Plan, Execution, Projectile))
	{
		return false;
	}
	Execution = Plan.ExecutionCandidate;
	Projectile.ActivateStagedLaunch();
	return FlightMatches(ActionRuntime, Execution, Projectile);
}

Fdemo_mapShanmenSwordQiWorldDeliveryResult
Fdemo_mapShanmenSwordQiWorldAdapter::ResolveProjectileContact(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenSwordQiExecution& Execution,
	Ademo_mapShanmenSwordQiProjectile& Projectile,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FHitResult& Hit)
{
	Fdemo_mapShanmenSwordQiWorldDeliveryResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!FlightMatches(ActionRuntime, Execution, Projectile))
	{
		Result.Error =
			Edemo_mapShanmenSwordQiWorldDeliveryError::FlightNotActive;
		return Result;
	}
	if (Execution.GetAction().GetRunId() != Coordinator.GetRunId()
		|| Execution.GetAction().GetSourceEntityId()
			!= Coordinator.GetPlayerEntityId())
	{
		Result.Error =
			Edemo_mapShanmenSwordQiWorldDeliveryError::ContextMismatch;
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
			Edemo_mapShanmenSwordQiWorldDeliveryError::ContactNotResolved;
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
		Result.Error = Edemo_mapShanmenSwordQiWorldDeliveryError::
			TargetVitalityUnavailable;
		return Result;
	}

	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenSwordQiExecution ExecutionCandidate = Execution;
	if (!ExecutionCandidate.TryResolveCandidate(
		ActionRuntime,
		Candidate,
		Vitality,
		Defense,
		Result.Impact))
	{
		Result.Error =
			Edemo_mapShanmenSwordQiWorldDeliveryError::CandidateRejected;
		return Result;
	}

	Result.Delivery = Coordinator.DeliverSwordQiImpactToM01Enemy(
		Result.Impact, TargetActor);
	if (!Result.Delivery.IsSuccess())
	{
		Result.Error =
			Edemo_mapShanmenSwordQiWorldDeliveryError::DeliveryRejected;
		return Result;
	}

	Execution = MoveTemp(ExecutionCandidate);
	Result.Error = Edemo_mapShanmenSwordQiWorldDeliveryError::None;
	return Result;
}

bool Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenSwordQiExecution& Execution,
	Ademo_mapShanmenSwordQiProjectile& Projectile)
{
	if (!FlightMatches(ActionRuntime, Execution, Projectile))
	{
		return false;
	}
	FShanmenSwordQiExecution Candidate = Execution;
	if (!Candidate.TryEndEmission(ActionRuntime)
		|| !Candidate.TryDissipate(ActionRuntime)
		|| !Projectile.MarkDissipated())
	{
		return false;
	}
	Execution = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordQiWorldAdapter::EndForActionTermination(
	FShanmenSwordQiExecution& Execution,
	Ademo_mapShanmenSwordQiProjectile& Projectile)
{
	if (!CarrierMatchesExecution(Execution, Projectile))
	{
		return false;
	}
	FShanmenSwordQiExecution Candidate = Execution;
	Candidate.EndForActionTermination();
	if (!Candidate.IsValid()
		|| Candidate.GetState() != EShanmenSwordQiState::Dissipated
		|| Candidate.IsEmissionActive()
		|| !Projectile.MarkDissipated())
	{
		return false;
	}
	Execution = MoveTemp(Candidate);
	return true;
}
