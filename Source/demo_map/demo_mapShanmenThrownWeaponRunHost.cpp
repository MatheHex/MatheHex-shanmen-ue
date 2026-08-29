#include "demo_mapShanmenThrownWeaponRunHost.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
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

	bool IsFiniteVector(const FVector& Value)
	{
		return !Value.ContainsNaN()
			&& FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool CoordinatorMatches(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenCombatActionSnapshot& Action,
		const AActor* SourceActor)
	{
		if (!Coordinator.IsReady()
			|| !Action.IsValid()
			|| !::IsValid(SourceActor)
			|| Coordinator.GetRunId() != Action.GetRunId()
			|| Coordinator.GetPlayerEntityId()
				!= Action.GetSourceEntityId())
		{
			return false;
		}
		FGuid ResolvedSourceEntityId;
		return Coordinator.GetEntityRegistry().TryResolveObject(
			Action.GetRunId(),
			const_cast<AActor*>(SourceActor),
			INDEX_NONE,
			ResolvedSourceEntityId)
			&& ResolvedSourceEntityId == Action.GetSourceEntityId();
	}

	bool BuildCompletedAction(
		const FShanmenActionOrchestrator& Source,
		FShanmenActionOrchestrator& OutCompleted,
		FShanmenActionTransitionReceipt& OutRecovery,
		FShanmenActionTransitionReceipt& OutCompletion)
	{
		OutRecovery = FShanmenActionTransitionReceipt();
		OutCompletion = FShanmenActionTransitionReceipt();
		OutCompleted = Source;
		return Source.IsValid()
			&& Source.CanEmitCandidates()
			&& OutCompleted.TryAdvance(
				EShanmenCombatActionPhase::Active, OutRecovery)
			&& OutCompleted.TryAdvance(
				EShanmenCombatActionPhase::Recovery, OutCompletion)
			&& OutCompleted.IsTerminal()
			&& OutCompleted.GetTerminalReason()
				== EShanmenActionTerminalReason::Completed;
	}
}

bool Fdemo_mapShanmenThrownWeaponSpawnResult::IsSpawned() const
{
	const Ademo_mapShanmenThrownWeaponProjectile* Spawned = Projectile.Get();
	return Error == Edemo_mapShanmenThrownWeaponSpawnError::None
		&& ::IsValid(Spawned)
		&& Spawned->GetProjectileState()
			== Edemo_mapShanmenThrownWeaponProjectileState::Empty
		&& Spawned->GetCollisionComponent()
		&& Spawned->GetMovementComponent()
		&& Spawned->GetCollisionComponent()->GetCollisionEnabled()
			== ECollisionEnabled::NoCollision
		&& !Spawned->GetMovementComponent()->IsActive();
}

bool Fdemo_mapShanmenThrownWeaponTerminalReceipt::IsValid() const
{
	if (!LaunchId.IsValid()
		|| Kind == Edemo_mapShanmenThrownWeaponTerminalKind::None)
	{
		return false;
	}
	if (Kind == Edemo_mapShanmenThrownWeaponTerminalKind::Interrupted)
	{
		return Interruption.IsValid()
			&& Interruption.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& !Recovery.IsValid()
			&& !Completion.IsValid();
	}
	if (!Recovery.IsValid()
		|| !Completion.IsValid()
		|| Completion.GetTerminalReason()
			!= EShanmenActionTerminalReason::Completed
		|| Interruption.IsValid())
	{
		return false;
	}
	return Kind == Edemo_mapShanmenThrownWeaponTerminalKind::Impact
		? Delivery.IsDelivered()
		: !Delivery.IsDelivered();
}

Fdemo_mapShanmenThrownWeaponRunHost::~Fdemo_mapShanmenThrownWeaponRunHost()
{
	if (IsInGameThread() && IsInFlight() && Projectile.IsValid())
	{
		TryInterrupt();
	}
	UnbindProjectile();
}

Fdemo_mapShanmenThrownWeaponSpawnResult
Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	AActor* SourceActor,
	const FVector& Origin)
{
	Fdemo_mapShanmenThrownWeaponSpawnResult Result;
	if (!::IsValid(World))
	{
		return Result;
	}
	if (!::IsValid(SourceActor) || SourceActor->GetWorld() != World)
	{
		Result.Error = Edemo_mapShanmenThrownWeaponSpawnError::SourceInvalid;
		return Result;
	}
	if (!ProjectileClass
		|| !ProjectileClass->IsChildOf(
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass()))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponSpawnError::ClassInvalid;
		return Result;
	}
	if (!IsFiniteVector(Origin))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponSpawnError::OriginInvalid;
		return Result;
	}

	FActorSpawnParameters Parameters;
	Parameters.Owner = SourceActor;
	Parameters.Instigator = Cast<APawn>(SourceActor);
	Parameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapShanmenThrownWeaponProjectile* Spawned =
		World->SpawnActor<Ademo_mapShanmenThrownWeaponProjectile>(
			ProjectileClass,
			Origin,
			FRotator::ZeroRotator,
			Parameters);
	if (!::IsValid(Spawned))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponSpawnError::SpawnRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenThrownWeaponSpawnError::None;
	Result.Projectile = Spawned;
	if (!Result.IsSpawned())
	{
		Spawned->Destroy();
		return Fdemo_mapShanmenThrownWeaponSpawnResult{
			Edemo_mapShanmenThrownWeaponSpawnError::CarrierInvalid, nullptr};
	}
	return Result;
}

Fdemo_mapShanmenThrownWeaponHostStartResult
Fdemo_mapShanmenThrownWeaponRunHost::TrySpawnAndLaunchPrepared(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const FVector& Origin,
	const FVector& AimDirection,
	float RequestedMaximumDistance)
{
	Fdemo_mapShanmenThrownWeaponHostStartResult Result;
	if (State != Edemo_mapShanmenThrownWeaponHostState::Empty)
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::HostBusy;
		return Result;
	}
	Result.Spawn = SpawnStagedCarrier(
		World, ProjectileClass, RequestedSourceActor, Origin);
	if (!Result.Spawn.IsSpawned())
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::SpawnRejected;
		return Result;
	}
	Ademo_mapShanmenThrownWeaponProjectile* Spawned =
		Result.Spawn.Projectile.Get();
	Result = TryLaunchPreparedCarrier(
		Authority,
		Correlation,
		Preparation,
		RequestedActionRuntime,
		RequestedExecution,
		*Spawned,
		RequestedCoordinator,
		RequestedSourceActor,
		Origin,
		AimDirection,
		RequestedMaximumDistance,
		true);
	Result.Spawn.Error = Edemo_mapShanmenThrownWeaponSpawnError::None;
	Result.Spawn.Projectile = Spawned;
	if (!Result.IsStarted() && ::IsValid(Spawned))
	{
		Spawned->Destroy();
	}
	return Result;
}

Fdemo_mapShanmenThrownWeaponHostStartResult
Fdemo_mapShanmenThrownWeaponRunHost::TryLaunchPreparedCarrier(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const FVector& Origin,
	const FVector& AimDirection,
	float RequestedMaximumDistance,
	bool bDestroyOnTerminal)
{
	Fdemo_mapShanmenThrownWeaponHostStartResult Result;
	if (State != Edemo_mapShanmenThrownWeaponHostState::Empty)
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::HostBusy;
		return Result;
	}
	if (!FMath::IsFinite(RequestedMaximumDistance)
		|| RequestedMaximumDistance <= KINDA_SMALL_NUMBER)
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::RangeInvalid;
		return Result;
	}
	if (!RequestedActionRuntime.IsValid()
		|| !RequestedActionRuntime.CanEmitCandidates()
		|| !RequestedExecution.IsValid()
		|| RequestedExecution.GetState()
			!= EShanmenThrownWeaponState::Ready
		|| RequestedExecution.IsEmissionActive()
		|| !Preparation.IsPrepared()
		|| !ActionsMatch(
			RequestedActionRuntime.GetAction(), RequestedExecution.GetAction())
		|| !ActionsMatch(
			RequestedActionRuntime.GetAction(), Preparation.Action)
		|| RequestedProjectile.GetProjectileState()
			!= Edemo_mapShanmenThrownWeaponProjectileState::Empty
		|| !CoordinatorMatches(
			RequestedCoordinator,
			RequestedActionRuntime.GetAction(),
			RequestedSourceActor))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::BindingInvalid;
		return Result;
	}

	FShanmenThrownWeaponExecution ExecutionCandidate = RequestedExecution;
	const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::StagePreparedLaunch(
			Correlation,
			Preparation,
			RequestedActionRuntime,
			ExecutionCandidate,
			RequestedProjectile,
			RequestedSourceActor,
			Origin,
			AimDirection);
	if (!Staged.IsStaged())
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected;
		Result.Launch = Staged;
		return Result;
	}
	Result.Launch =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::CommitStagedLaunch(
			Authority,
			RequestedActionRuntime,
			Staged.Plan,
			ExecutionCandidate,
			RequestedProjectile);
	if (!Result.Launch.IsCommitted())
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected;
		return Result;
	}
	if (!TryAdoptPublishedFlight(
		RequestedActionRuntime,
		ExecutionCandidate,
		Result.Launch.ItemCommit,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		RequestedMaximumDistance,
		bDestroyOnTerminal))
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::AdoptionRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::None;
	Result.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponRunHost::TryAdoptPublishedFlight(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
	Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	float RequestedMaximumDistance,
	bool bDestroyOnTerminal)
{
	if (!ValidateBinding(
		RequestedActionRuntime,
		RequestedExecution,
		CommittedItem,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		RequestedMaximumDistance))
	{
		return false;
	}

	ActionRuntime = RequestedActionRuntime;
	Execution = RequestedExecution;
	ItemCommit = CommittedItem;
	Projectile = &RequestedProjectile;
	SourceActor = RequestedSourceActor;
	Coordinator = &RequestedCoordinator;
	MaximumDistance = RequestedMaximumDistance;
	bDestroyCarrierOnTerminal = bDestroyOnTerminal;
	State = Edemo_mapShanmenThrownWeaponHostState::InFlight;
	BindProjectile();
	if (bDestroyCarrierOnTerminal && RequestedProjectile.GetWorld())
	{
		RequestedProjectile.SetLifeSpan(
			RequestedMaximumDistance
			/ RequestedExecution.GetLaunchReceipt().GetSpeed());
	}
	return IsValid();
}

bool Fdemo_mapShanmenThrownWeaponRunHost::TryExpireRange()
{
	return FinishWithoutImpact(
		Edemo_mapShanmenThrownWeaponTerminalKind::RangeExpired, true);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::TryInterrupt()
{
	Ademo_mapShanmenThrownWeaponProjectile* ActiveProjectile =
		Projectile.Get();
	if (!IsInFlight()
		|| !ActiveProjectile)
	{
		return false;
	}
	FShanmenActionOrchestrator InterruptedAction = ActionRuntime;
	FShanmenThrownWeaponExecution InterruptedExecution = Execution;
	FShanmenActionTransitionReceipt Interrupted;
	if (!InterruptedAction.TryInterrupt(
			EShanmenCombatActionPhase::Active, Interrupted)
		|| !Fdemo_mapShanmenThrownWeaponWorldAdapter::
			FinishFlightWithoutImpact(
				ActionRuntime, InterruptedExecution, *ActiveProjectile))
	{
		return false;
	}
	ActionRuntime = MoveTemp(InterruptedAction);
	Execution = MoveTemp(InterruptedExecution);
	Fdemo_mapShanmenThrownWeaponTerminalReceipt Receipt;
	Receipt.Kind = Edemo_mapShanmenThrownWeaponTerminalKind::Interrupted;
	Receipt.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	Receipt.Interruption = Interrupted;
	return PublishTerminal(MoveTemp(Receipt), true);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::Reset()
{
	if (State == Edemo_mapShanmenThrownWeaponHostState::InFlight)
	{
		return false;
	}
	UnbindProjectile();
	DestroyOwnedProjectile();
	ActionRuntime.Reset();
	Execution = FShanmenThrownWeaponExecution();
	ItemCommit = Fdemo_mapShanmenThrownWeaponItemResult();
	Projectile.Reset();
	SourceActor.Reset();
	Coordinator = nullptr;
	TerminalReceipt = Fdemo_mapShanmenThrownWeaponTerminalReceipt();
	MaximumDistance = 0.0f;
	bDestroyCarrierOnTerminal = false;
	State = Edemo_mapShanmenThrownWeaponHostState::Empty;
	return true;
}

bool Fdemo_mapShanmenThrownWeaponRunHost::IsValid() const
{
	if (!ItemCommit.IsFinalized()
		|| ItemCommit.Status
			!= Edemo_mapShanmenThrownWeaponItemStatus::Committed
		|| !ItemCommit.FinalizeRequest.bCommit
		|| !FMath::IsFinite(MaximumDistance)
		|| MaximumDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	if (State == Edemo_mapShanmenThrownWeaponHostState::Terminal)
	{
		return ActionRuntime.IsValid()
			&& ActionRuntime.IsTerminal()
			&& Execution.IsValid()
			&& Execution.GetState() == EShanmenThrownWeaponState::Spent
			&& !Execution.IsEmissionActive()
			&& TerminalReceipt.IsValid();
	}
	const Ademo_mapShanmenThrownWeaponProjectile* ActiveProjectile =
		Projectile.Get();
	return State == Edemo_mapShanmenThrownWeaponHostState::InFlight
		&& ActionRuntime.IsValid()
		&& ActionRuntime.CanEmitCandidates()
		&& Execution.IsValid()
		&& Execution.GetState() == EShanmenThrownWeaponState::InFlight
		&& Execution.IsEmissionActive()
		&& ::IsValid(ActiveProjectile)
		&& ActiveProjectile->IsInFlightFor(
			Execution.GetLaunchReceipt(), ActiveProjectile->GetHitContext())
		&& Coordinator
		&& CoordinatorMatches(
			*Coordinator, ActionRuntime.GetAction(), SourceActor.Get());
}

bool Fdemo_mapShanmenThrownWeaponRunHost::ValidateBinding(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
	const Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
	const Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	const AActor* RequestedSourceActor,
	float RequestedMaximumDistance) const
{
	return State == Edemo_mapShanmenThrownWeaponHostState::Empty
		&& FMath::IsFinite(RequestedMaximumDistance)
		&& RequestedMaximumDistance > KINDA_SMALL_NUMBER
		&& RequestedActionRuntime.IsValid()
		&& RequestedActionRuntime.CanEmitCandidates()
		&& RequestedExecution.IsValid()
		&& RequestedExecution.GetState()
			== EShanmenThrownWeaponState::InFlight
		&& RequestedExecution.IsEmissionActive()
		&& ActionsMatch(
			RequestedActionRuntime.GetAction(), RequestedExecution.GetAction())
		&& CommittedItem.Status
			== Edemo_mapShanmenThrownWeaponItemStatus::Committed
		&& CommittedItem.IsFinalized()
		&& CommittedItem.FinalizeRequest.bCommit
		&& ActionsMatch(
			RequestedActionRuntime.GetAction(), CommittedItem.Action)
		&& RequestedProjectile.GetOwner() == RequestedSourceActor
		&& RequestedProjectile.IsInFlightFor(
			RequestedExecution.GetLaunchReceipt(),
			RequestedProjectile.GetHitContext())
		&& CoordinatorMatches(
			RequestedCoordinator,
			RequestedActionRuntime.GetAction(),
			RequestedSourceActor);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::FinishWithoutImpact(
	Edemo_mapShanmenThrownWeaponTerminalKind Kind,
	bool bDestroyNow,
	const Fdemo_mapShanmenThrownWeaponWorldDeliveryResult* DeliveryDiagnostic)
{
	Ademo_mapShanmenThrownWeaponProjectile* ActiveProjectile =
		Projectile.Get();
	if (!IsInFlight()
		|| Kind == Edemo_mapShanmenThrownWeaponTerminalKind::None
		|| Kind == Edemo_mapShanmenThrownWeaponTerminalKind::Impact
		|| Kind == Edemo_mapShanmenThrownWeaponTerminalKind::Interrupted
		|| !ActiveProjectile)
	{
		return false;
	}
	FShanmenThrownWeaponExecution TerminalExecution = Execution;
	FShanmenActionOrchestrator CompletedAction;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completion;
	if (!BuildCompletedAction(
			ActionRuntime, CompletedAction, Recovery, Completion)
		|| !Fdemo_mapShanmenThrownWeaponWorldAdapter::
			FinishFlightWithoutImpact(
				ActionRuntime, TerminalExecution, *ActiveProjectile))
	{
		return false;
	}
	Execution = MoveTemp(TerminalExecution);
	ActionRuntime = MoveTemp(CompletedAction);
	Fdemo_mapShanmenThrownWeaponTerminalReceipt Receipt;
	Receipt.Kind = Kind;
	Receipt.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	Receipt.Recovery = Recovery;
	Receipt.Completion = Completion;
	if (DeliveryDiagnostic)
	{
		Receipt.Delivery = *DeliveryDiagnostic;
	}
	return PublishTerminal(MoveTemp(Receipt), bDestroyNow);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::PublishTerminal(
	Fdemo_mapShanmenThrownWeaponTerminalReceipt&& Receipt,
	bool bDestroyNow)
{
	if (!Receipt.IsValid()
		|| !ActionRuntime.IsTerminal()
		|| Execution.GetState() != EShanmenThrownWeaponState::Spent)
	{
		return false;
	}
	TerminalReceipt = MoveTemp(Receipt);
	State = Edemo_mapShanmenThrownWeaponHostState::Terminal;
	Coordinator = nullptr;
	UnbindProjectile();
	if (Ademo_mapShanmenThrownWeaponProjectile* TerminalProjectile =
		Projectile.Get())
	{
		if (TerminalProjectile->GetWorld())
		{
			TerminalProjectile->SetLifeSpan(0.0f);
		}
	}
	if (bDestroyNow)
	{
		DestroyOwnedProjectile();
	}
	return IsValid();
}

void Fdemo_mapShanmenThrownWeaponRunHost::HandleContact(
	Ademo_mapShanmenThrownWeaponProjectile& ContactProjectile,
	const FHitResult& Hit)
{
	if (!IsInFlight()
		|| Projectile.Get() != &ContactProjectile
		|| !Coordinator)
	{
		return;
	}

	FShanmenActionOrchestrator CompletedAction;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completion;
	if (!BuildCompletedAction(
		ActionRuntime, CompletedAction, Recovery, Completion))
	{
		return;
	}
	FShanmenThrownWeaponExecution ContactExecution = Execution;
	const Fdemo_mapShanmenThrownWeaponWorldDeliveryResult Delivery =
		Fdemo_mapShanmenThrownWeaponWorldAdapter::ResolveProjectileContact(
			ActionRuntime,
			ContactExecution,
			ContactProjectile,
			*Coordinator,
			Hit);
	if (!Delivery.IsDelivered())
	{
		FinishWithoutImpact(
			Edemo_mapShanmenThrownWeaponTerminalKind::BlockingMiss,
			true,
			&Delivery);
		return;
	}

	Execution = MoveTemp(ContactExecution);
	ActionRuntime = MoveTemp(CompletedAction);
	Fdemo_mapShanmenThrownWeaponTerminalReceipt Receipt;
	Receipt.Kind = Edemo_mapShanmenThrownWeaponTerminalKind::Impact;
	Receipt.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	Receipt.Delivery = Delivery;
	Receipt.Recovery = Recovery;
	Receipt.Completion = Completion;
	PublishTerminal(MoveTemp(Receipt), true);
}

void Fdemo_mapShanmenThrownWeaponRunHost::HandleRangeExpired(
	Ademo_mapShanmenThrownWeaponProjectile& ExpiredProjectile)
{
	if (Projectile.Get() == &ExpiredProjectile)
	{
		FinishWithoutImpact(
			Edemo_mapShanmenThrownWeaponTerminalKind::RangeExpired, false);
	}
}

void Fdemo_mapShanmenThrownWeaponRunHost::BindProjectile()
{
	Ademo_mapShanmenThrownWeaponProjectile* BoundProjectile = Projectile.Get();
	check(BoundProjectile && !ContactHandle.IsValid()
		&& !RangeExpiredHandle.IsValid());
	ContactHandle = BoundProjectile->OnContact().AddRaw(
		this, &Fdemo_mapShanmenThrownWeaponRunHost::HandleContact);
	RangeExpiredHandle = BoundProjectile->OnRangeExpired().AddRaw(
		this, &Fdemo_mapShanmenThrownWeaponRunHost::HandleRangeExpired);
}

void Fdemo_mapShanmenThrownWeaponRunHost::UnbindProjectile()
{
	if (Ademo_mapShanmenThrownWeaponProjectile* BoundProjectile =
		Projectile.Get())
	{
		if (ContactHandle.IsValid())
		{
			BoundProjectile->OnContact().Remove(ContactHandle);
		}
		if (RangeExpiredHandle.IsValid())
		{
			BoundProjectile->OnRangeExpired().Remove(RangeExpiredHandle);
		}
	}
	ContactHandle.Reset();
	RangeExpiredHandle.Reset();
}

void Fdemo_mapShanmenThrownWeaponRunHost::DestroyOwnedProjectile()
{
	Ademo_mapShanmenThrownWeaponProjectile* OwnedProjectile = Projectile.Get();
	if (!bDestroyCarrierOnTerminal
		|| !::IsValid(OwnedProjectile)
		|| !OwnedProjectile->GetWorld()
		|| OwnedProjectile->IsActorBeingDestroyed())
	{
		return;
	}
	OwnedProjectile->SetLifeSpan(0.0f);
	OwnedProjectile->Destroy();
}
