#include "demo_mapShanmenSwordQiRunHost.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

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

bool Fdemo_mapShanmenSwordQiSpawnResult::IsSpawned() const
{
	const Ademo_mapShanmenSwordQiProjectile* Spawned = Projectile.Get();
	return Error == Edemo_mapShanmenSwordQiSpawnError::None
		&& ::IsValid(Spawned)
		&& Spawned->GetProjectileState()
			== Edemo_mapShanmenSwordQiProjectileState::Empty
		&& Spawned->GetCollisionComponent()
		&& Spawned->GetMovementComponent()
		&& Spawned->GetCollisionComponent()->GetCollisionEnabled()
			== ECollisionEnabled::NoCollision
		&& !Spawned->GetMovementComponent()->IsActive();
}

bool Fdemo_mapShanmenSwordQiTerminalReceipt::IsValid() const
{
	if (!LaunchId.IsValid()
		|| Kind == Edemo_mapShanmenSwordQiTerminalKind::None)
	{
		return false;
	}
	if (Kind == Edemo_mapShanmenSwordQiTerminalKind::Interrupted)
	{
		return Interruption.IsValid()
			&& Interruption.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& !Recovery.IsValid()
			&& !Completion.IsValid()
			&& !Delivery.IsDelivered();
	}
	if (!Recovery.IsValid()
		|| !Completion.IsValid()
		|| Completion.GetTerminalReason()
			!= EShanmenActionTerminalReason::Completed
		|| Interruption.IsValid())
	{
		return false;
	}
	if (Kind == Edemo_mapShanmenSwordQiTerminalKind::Impact)
	{
		return Delivery.IsDelivered();
	}
	if (Delivery.IsDelivered())
	{
		return false;
	}
	return Kind == Edemo_mapShanmenSwordQiTerminalKind::RangeExpired
		|| (Kind == Edemo_mapShanmenSwordQiTerminalKind::BlockingMiss
			&& Delivery.Error
				!= Edemo_mapShanmenSwordQiWorldDeliveryError::None);
}

Fdemo_mapShanmenSwordQiRunHost::~Fdemo_mapShanmenSwordQiRunHost()
{
	if (IsInGameThread() && IsInFlight() && Projectile.IsValid())
	{
		TryInterrupt();
	}
	UnbindProjectile();
}

Fdemo_mapShanmenSwordQiSpawnResult
Fdemo_mapShanmenSwordQiRunHost::SpawnStagedCarrier(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
	AActor* SourceActor,
	const FVector& Origin)
{
	Fdemo_mapShanmenSwordQiSpawnResult Result;
	if (!::IsValid(World))
	{
		return Result;
	}
	if (!::IsValid(SourceActor) || SourceActor->GetWorld() != World)
	{
		Result.Error = Edemo_mapShanmenSwordQiSpawnError::SourceInvalid;
		return Result;
	}
	if (!ProjectileClass
		|| !ProjectileClass->IsChildOf(
			Ademo_mapShanmenSwordQiProjectile::StaticClass()))
	{
		Result.Error = Edemo_mapShanmenSwordQiSpawnError::ClassInvalid;
		return Result;
	}
	if (!IsFiniteVector(Origin))
	{
		Result.Error = Edemo_mapShanmenSwordQiSpawnError::OriginInvalid;
		return Result;
	}

	FActorSpawnParameters Parameters;
	Parameters.Owner = SourceActor;
	Parameters.Instigator = Cast<APawn>(SourceActor);
	Parameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapShanmenSwordQiProjectile* Spawned =
		World->SpawnActor<Ademo_mapShanmenSwordQiProjectile>(
			ProjectileClass,
			Origin,
			FRotator::ZeroRotator,
			Parameters);
	if (!::IsValid(Spawned))
	{
		Result.Error = Edemo_mapShanmenSwordQiSpawnError::SpawnRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenSwordQiSpawnError::None;
	Result.Projectile = Spawned;
	if (!Result.IsSpawned())
	{
		Spawned->Destroy();
		return Fdemo_mapShanmenSwordQiSpawnResult{
			Edemo_mapShanmenSwordQiSpawnError::CarrierInvalid, nullptr};
	}
	return Result;
}

Fdemo_mapShanmenSwordQiHostStartResult
Fdemo_mapShanmenSwordQiRunHost::TrySpawnAndLaunch(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenSwordQiExecution& RequestedExecution,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const FVector& Origin,
	const FVector& AimDirection)
{
	Fdemo_mapShanmenSwordQiHostStartResult Result;
	if (State != Edemo_mapShanmenSwordQiHostState::Empty)
	{
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::HostBusy;
		return Result;
	}
	const Fdemo_mapShanmenSwordQiSpawnResult Spawn = SpawnStagedCarrier(
		World, ProjectileClass, RequestedSourceActor, Origin);
	if (!Spawn.IsSpawned())
	{
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::SpawnRejected;
		Result.Spawn = Spawn;
		return Result;
	}
	Ademo_mapShanmenSwordQiProjectile* Spawned = Spawn.Projectile.Get();
	Result = TryLaunchCarrier(
		RequestedActionRuntime,
		RequestedExecution,
		*Spawned,
		RequestedCoordinator,
		RequestedSourceActor,
		Origin,
		AimDirection,
		true);
	Result.Spawn = Spawn;
	if (!Result.IsStarted() && ::IsValid(Spawned))
	{
		Spawned->Destroy();
	}
	return Result;
}

Fdemo_mapShanmenSwordQiHostStartResult
Fdemo_mapShanmenSwordQiRunHost::TryLaunchCarrier(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenSwordQiExecution& RequestedExecution,
	Ademo_mapShanmenSwordQiProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const FVector& Origin,
	const FVector& AimDirection,
	bool bDestroyOnTerminal)
{
	Fdemo_mapShanmenSwordQiHostStartResult Result;
	if (State != Edemo_mapShanmenSwordQiHostState::Empty)
	{
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::HostBusy;
		return Result;
	}
	if (!RequestedActionRuntime.IsValid()
		|| !RequestedActionRuntime.CanEmitCandidates()
		|| !RequestedExecution.IsValid()
		|| RequestedExecution.GetState() != EShanmenSwordQiState::Ready
		|| RequestedExecution.IsEmissionActive()
		|| !ActionsMatch(
			RequestedActionRuntime.GetAction(), RequestedExecution.GetAction())
		|| RequestedProjectile.GetProjectileState()
			!= Edemo_mapShanmenSwordQiProjectileState::Empty
		|| !CoordinatorMatches(
			RequestedCoordinator,
			RequestedActionRuntime.GetAction(),
			RequestedSourceActor))
	{
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::BindingInvalid;
		return Result;
	}

	FShanmenSwordQiExecution ExecutionCandidate = RequestedExecution;
	Result.Launch = Fdemo_mapShanmenSwordQiWorldAdapter::StageLaunch(
		RequestedActionRuntime,
		ExecutionCandidate,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		Origin,
		AimDirection);
	if (!Result.Launch.IsStaged())
	{
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::LaunchRejected;
		return Result;
	}
	if (!Fdemo_mapShanmenSwordQiWorldAdapter::PublishStagedLaunch(
		RequestedActionRuntime,
		Result.Launch.Plan,
		ExecutionCandidate,
		RequestedProjectile))
	{
		RequestedProjectile.CancelStagedLaunch();
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::LaunchRejected;
		return Result;
	}
	if (!TryAdoptPublishedFlight(
		RequestedActionRuntime,
		ExecutionCandidate,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		bDestroyOnTerminal))
	{
		Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
			RequestedActionRuntime, ExecutionCandidate, RequestedProjectile);
		Result.Error = Edemo_mapShanmenSwordQiHostStartError::AdoptionRejected;
		return Result;
	}
	Result.Error = Edemo_mapShanmenSwordQiHostStartError::None;
	Result.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	return Result;
}

bool Fdemo_mapShanmenSwordQiRunHost::TryAdoptPublishedFlight(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenSwordQiExecution& RequestedExecution,
	Ademo_mapShanmenSwordQiProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	bool bDestroyOnTerminal)
{
	if (!ValidateBinding(
		RequestedActionRuntime,
		RequestedExecution,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor))
	{
		return false;
	}

	ActionRuntime = RequestedActionRuntime;
	Execution = RequestedExecution;
	Projectile = &RequestedProjectile;
	SourceActor = RequestedSourceActor;
	Coordinator = &RequestedCoordinator;
	MaximumDistance = RequestedExecution.GetLaunchReceipt().GetMaximumRange();
	bDestroyCarrierOnTerminal = bDestroyOnTerminal;
	State = Edemo_mapShanmenSwordQiHostState::InFlight;
	BindProjectile();
	if (bDestroyCarrierOnTerminal && RequestedProjectile.GetWorld())
	{
		RequestedProjectile.SetLifeSpan(
			MaximumDistance
			/ RequestedExecution.GetLaunchReceipt().GetSpeed());
	}
	return IsValid();
}

bool Fdemo_mapShanmenSwordQiRunHost::TryExpireRange()
{
	return FinishWithoutImpact(
		Edemo_mapShanmenSwordQiTerminalKind::RangeExpired, true);
}

bool Fdemo_mapShanmenSwordQiRunHost::TryInterrupt()
{
	Ademo_mapShanmenSwordQiProjectile* ActiveProjectile = Projectile.Get();
	if (!IsInFlight() || !ActiveProjectile)
	{
		return false;
	}
	FShanmenActionOrchestrator InterruptedAction = ActionRuntime;
	FShanmenSwordQiExecution InterruptedExecution = Execution;
	FShanmenActionTransitionReceipt Interrupted;
	if (!InterruptedAction.TryInterrupt(
			EShanmenCombatActionPhase::Active, Interrupted)
		|| !Fdemo_mapShanmenSwordQiWorldAdapter::EndForActionTermination(
			InterruptedExecution, *ActiveProjectile))
	{
		return false;
	}
	ActionRuntime = MoveTemp(InterruptedAction);
	Execution = MoveTemp(InterruptedExecution);
	Fdemo_mapShanmenSwordQiTerminalReceipt Receipt;
	Receipt.Kind = Edemo_mapShanmenSwordQiTerminalKind::Interrupted;
	Receipt.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	Receipt.Interruption = Interrupted;
	return PublishTerminal(MoveTemp(Receipt), true);
}

bool Fdemo_mapShanmenSwordQiRunHost::Reset()
{
	if (State == Edemo_mapShanmenSwordQiHostState::InFlight)
	{
		return false;
	}
	UnbindProjectile();
	DestroyOwnedProjectile();
	ActionRuntime.Reset();
	Execution = FShanmenSwordQiExecution();
	Projectile.Reset();
	SourceActor.Reset();
	Coordinator = nullptr;
	TerminalReceipt = Fdemo_mapShanmenSwordQiTerminalReceipt();
	MaximumDistance = 0.0f;
	bDestroyCarrierOnTerminal = false;
	State = Edemo_mapShanmenSwordQiHostState::Empty;
	return true;
}

bool Fdemo_mapShanmenSwordQiRunHost::IsValid() const
{
	if (!FMath::IsFinite(MaximumDistance)
		|| MaximumDistance <= KINDA_SMALL_NUMBER
		|| !Execution.IsValid()
		|| !Execution.GetLaunchReceipt().IsValid()
		|| !FMath::IsNearlyEqual(
			MaximumDistance,
			Execution.GetLaunchReceipt().GetMaximumRange()))
	{
		return false;
	}
	if (State == Edemo_mapShanmenSwordQiHostState::Terminal)
	{
		const int32 ExpectedImpactCount =
			TerminalReceipt.Kind
				== Edemo_mapShanmenSwordQiTerminalKind::Impact
			? 1
			: 0;
		return ActionRuntime.IsValid()
			&& ActionRuntime.IsTerminal()
			&& Execution.GetState() == EShanmenSwordQiState::Dissipated
			&& !Execution.IsEmissionActive()
			&& Execution.NumAcceptedImpacts() == ExpectedImpactCount
			&& TerminalReceipt.IsValid()
			&& TerminalReceipt.LaunchId
				== Execution.GetLaunchReceipt().GetLaunchId();
	}
	const Ademo_mapShanmenSwordQiProjectile* ActiveProjectile = Projectile.Get();
	return State == Edemo_mapShanmenSwordQiHostState::InFlight
		&& ActionRuntime.IsValid()
		&& ActionRuntime.CanEmitCandidates()
		&& Execution.GetState() == EShanmenSwordQiState::InFlight
		&& Execution.IsEmissionActive()
		&& Execution.NumAcceptedImpacts() == 0
		&& ::IsValid(ActiveProjectile)
		&& ActiveProjectile->IsInFlightFor(
			Execution.GetLaunchReceipt(), ActiveProjectile->GetHitContext())
		&& Coordinator
		&& CoordinatorMatches(
			*Coordinator, ActionRuntime.GetAction(), SourceActor.Get())
		&& ContactHandle.IsValid()
		&& RangeExpiredHandle.IsValid();
}

bool Fdemo_mapShanmenSwordQiRunHost::ValidateBinding(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenSwordQiExecution& RequestedExecution,
	const Ademo_mapShanmenSwordQiProjectile& RequestedProjectile,
	const Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	const AActor* RequestedSourceActor) const
{
	const FShanmenSwordQiLaunchReceipt& Launch =
		RequestedExecution.GetLaunchReceipt();
	return State == Edemo_mapShanmenSwordQiHostState::Empty
		&& RequestedActionRuntime.IsValid()
		&& RequestedActionRuntime.CanEmitCandidates()
		&& RequestedExecution.IsValid()
		&& RequestedExecution.GetState() == EShanmenSwordQiState::InFlight
		&& RequestedExecution.IsEmissionActive()
		&& RequestedExecution.NumAcceptedImpacts() == 0
		&& Launch.IsValid()
		&& FMath::IsFinite(Launch.GetSpeed())
		&& Launch.GetSpeed() > KINDA_SMALL_NUMBER
		&& FMath::IsFinite(Launch.GetMaximumRange())
		&& Launch.GetMaximumRange() > KINDA_SMALL_NUMBER
		&& ActionsMatch(
			RequestedActionRuntime.GetAction(), RequestedExecution.GetAction())
		&& RequestedProjectile.GetOwner() == RequestedSourceActor
		&& RequestedProjectile.IsInFlightFor(
			Launch, RequestedProjectile.GetHitContext())
		&& CoordinatorMatches(
			RequestedCoordinator,
			RequestedActionRuntime.GetAction(),
			RequestedSourceActor);
}

bool Fdemo_mapShanmenSwordQiRunHost::FinishWithoutImpact(
	Edemo_mapShanmenSwordQiTerminalKind Kind,
	bool bDestroyNow,
	const Fdemo_mapShanmenSwordQiWorldDeliveryResult* DeliveryDiagnostic)
{
	Ademo_mapShanmenSwordQiProjectile* ActiveProjectile = Projectile.Get();
	if (!IsInFlight()
		|| Kind == Edemo_mapShanmenSwordQiTerminalKind::None
		|| Kind == Edemo_mapShanmenSwordQiTerminalKind::Impact
		|| Kind == Edemo_mapShanmenSwordQiTerminalKind::Interrupted
		|| !ActiveProjectile)
	{
		return false;
	}
	FShanmenSwordQiExecution TerminalExecution = Execution;
	FShanmenActionOrchestrator CompletedAction;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completion;
	if (!BuildCompletedAction(
			ActionRuntime, CompletedAction, Recovery, Completion)
		|| !Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
			ActionRuntime, TerminalExecution, *ActiveProjectile))
	{
		return false;
	}
	Execution = MoveTemp(TerminalExecution);
	ActionRuntime = MoveTemp(CompletedAction);
	Fdemo_mapShanmenSwordQiTerminalReceipt Receipt;
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

bool Fdemo_mapShanmenSwordQiRunHost::PublishTerminal(
	Fdemo_mapShanmenSwordQiTerminalReceipt&& Receipt,
	bool bDestroyNow)
{
	if (!Receipt.IsValid()
		|| !ActionRuntime.IsTerminal()
		|| Execution.GetState() != EShanmenSwordQiState::Dissipated)
	{
		return false;
	}
	TerminalReceipt = MoveTemp(Receipt);
	State = Edemo_mapShanmenSwordQiHostState::Terminal;
	Coordinator = nullptr;
	UnbindProjectile();
	if (Ademo_mapShanmenSwordQiProjectile* TerminalProjectile =
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

void Fdemo_mapShanmenSwordQiRunHost::HandleContact(
	Ademo_mapShanmenSwordQiProjectile& ContactProjectile,
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
	FShanmenSwordQiExecution ContactExecution = Execution;
	const Fdemo_mapShanmenSwordQiWorldDeliveryResult Delivery =
		Fdemo_mapShanmenSwordQiWorldAdapter::ResolveProjectileContact(
			ActionRuntime,
			ContactExecution,
			ContactProjectile,
			*Coordinator,
			Hit);
	if (!Delivery.IsDelivered())
	{
		FinishWithoutImpact(
			Edemo_mapShanmenSwordQiTerminalKind::BlockingMiss,
			true,
			&Delivery);
		return;
	}
	if (!Fdemo_mapShanmenSwordQiWorldAdapter::FinishFlight(
		ActionRuntime, ContactExecution, ContactProjectile))
	{
		// Vitality already committed; retain the ledger candidate so any later
		// callback remains idempotent even if an impossible carrier race occurs.
		Execution = MoveTemp(ContactExecution);
		return;
	}

	Execution = MoveTemp(ContactExecution);
	ActionRuntime = MoveTemp(CompletedAction);
	Fdemo_mapShanmenSwordQiTerminalReceipt Receipt;
	Receipt.Kind = Edemo_mapShanmenSwordQiTerminalKind::Impact;
	Receipt.LaunchId = Execution.GetLaunchReceipt().GetLaunchId();
	Receipt.Delivery = Delivery;
	Receipt.Recovery = Recovery;
	Receipt.Completion = Completion;
	PublishTerminal(MoveTemp(Receipt), true);
}

void Fdemo_mapShanmenSwordQiRunHost::HandleRangeExpired(
	Ademo_mapShanmenSwordQiProjectile& ExpiredProjectile)
{
	if (Projectile.Get() == &ExpiredProjectile)
	{
		FinishWithoutImpact(
			Edemo_mapShanmenSwordQiTerminalKind::RangeExpired, false);
	}
}

void Fdemo_mapShanmenSwordQiRunHost::BindProjectile()
{
	Ademo_mapShanmenSwordQiProjectile* BoundProjectile = Projectile.Get();
	check(BoundProjectile && !ContactHandle.IsValid()
		&& !RangeExpiredHandle.IsValid());
	ContactHandle = BoundProjectile->OnContact().AddRaw(
		this, &Fdemo_mapShanmenSwordQiRunHost::HandleContact);
	RangeExpiredHandle = BoundProjectile->OnRangeExpired().AddRaw(
		this, &Fdemo_mapShanmenSwordQiRunHost::HandleRangeExpired);
}

void Fdemo_mapShanmenSwordQiRunHost::UnbindProjectile()
{
	if (Ademo_mapShanmenSwordQiProjectile* BoundProjectile = Projectile.Get())
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

void Fdemo_mapShanmenSwordQiRunHost::DestroyOwnedProjectile()
{
	Ademo_mapShanmenSwordQiProjectile* OwnedProjectile = Projectile.Get();
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
