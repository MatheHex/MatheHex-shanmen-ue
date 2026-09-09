#include "demo_mapShanmenThrownWeaponRunHost.h"

#include "Components/BoxComponent.h"
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

	bool IsUsableActorLifeSpan(double Seconds)
	{
		return FMath::IsFinite(Seconds)
			&& Seconds > 0.0
			&& Seconds <= static_cast<double>(MAX_flt)
			&& FMath::IsFinite(static_cast<float>(Seconds))
			&& static_cast<float>(Seconds) > 0.0f;
	}

	template <typename TLaunchCarrier>
	Fdemo_mapShanmenThrownWeaponHostStartResult SpawnAndLaunchPreparedImpl(
		Fdemo_mapShanmenThrownWeaponRunHost& Host,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		AActor* SourceActor,
		const FVector& Origin,
		TLaunchCarrier&& LaunchCarrier)
	{
		Fdemo_mapShanmenThrownWeaponHostStartResult Result;
		if (Host.GetState()
			!= Edemo_mapShanmenThrownWeaponHostState::Empty)
		{
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::HostBusy;
			return Result;
		}
		Result.Spawn =
			Fdemo_mapShanmenThrownWeaponRunHost::SpawnStagedCarrier(
				World, ProjectileClass, SourceActor, Origin);
		if (!Result.Spawn.IsSpawned())
		{
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::SpawnRejected;
			return Result;
		}
		Ademo_mapShanmenThrownWeaponProjectile* Spawned =
			Result.Spawn.Projectile.Get();
		Result = LaunchCarrier(*Spawned);
		Result.Spawn.Error = Edemo_mapShanmenThrownWeaponSpawnError::None;
		Result.Spawn.Projectile = Spawned;
		if (!Result.IsStarted() && ::IsValid(Spawned))
		{
			Spawned->Destroy();
		}
		return Result;
	}

	template <typename TStage, typename TAdopt>
	Fdemo_mapShanmenThrownWeaponHostStartResult LaunchPreparedCarrierImpl(
		Fdemo_mapShanmenThrownWeaponRunHost& Host,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenActionOrchestrator& RequestedActionRuntime,
		const FShanmenThrownWeaponExecution& RequestedExecution,
		Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
		Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
		AActor* RequestedSourceActor,
		TStage&& Stage,
		TAdopt&& Adopt)
	{
		Fdemo_mapShanmenThrownWeaponHostStartResult Result;
		if (Host.GetState()
			!= Edemo_mapShanmenThrownWeaponHostState::Empty)
		{
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::HostBusy;
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
				RequestedActionRuntime.GetAction(),
				RequestedExecution.GetAction())
			|| !ActionsMatch(
				RequestedActionRuntime.GetAction(), Preparation.Action)
			|| RequestedProjectile.GetProjectileState()
				!= Edemo_mapShanmenThrownWeaponProjectileState::Empty
			|| !CoordinatorMatches(
				RequestedCoordinator,
				RequestedActionRuntime.GetAction(),
				RequestedSourceActor))
		{
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::BindingInvalid;
			return Result;
		}

		FShanmenThrownWeaponExecution ExecutionCandidate =
			RequestedExecution;
		const Fdemo_mapShanmenThrownWeaponLaunchResult Staged =
			Stage(ExecutionCandidate);
		if (!Staged.IsStaged())
		{
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected;
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
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected;
			return Result;
		}
		if (!Adopt(ExecutionCandidate, Result.Launch.ItemCommit))
		{
			Result.Error =
				Edemo_mapShanmenThrownWeaponHostStartError::AdoptionRejected;
			return Result;
		}
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::None;
		Result.LaunchId = Host.GetExecution().GetLaunchReceipt().GetLaunchId();
		return Result;
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

bool Fdemo_mapShanmenThrownWeaponHostLifetime::TryCreateRange(
	const FShanmenThrownWeaponLaunchReceipt& Launch,
	float RequestedMaximumDistance,
	Fdemo_mapShanmenThrownWeaponHostLifetime& OutLifetime)
{
	OutLifetime = Fdemo_mapShanmenThrownWeaponHostLifetime();
	if (!Launch.IsValid()
		|| Launch.GetTrajectoryKind()
			!= EShanmenThrownWeaponTrajectoryKind::Straight
		|| !FMath::IsFinite(RequestedMaximumDistance)
		|| RequestedMaximumDistance <= KINDA_SMALL_NUMBER
		|| !FMath::IsFinite(Launch.GetSpeed())
		|| Launch.GetSpeed() <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const double ActorLifeSpan =
		static_cast<double>(RequestedMaximumDistance)
		/ static_cast<double>(Launch.GetSpeed());
	if (!IsUsableActorLifeSpan(ActorLifeSpan))
	{
		return false;
	}

	OutLifetime.Kind =
		Edemo_mapShanmenThrownWeaponHostLifetimeKind::RangeDistance;
	OutLifetime.MaximumDistance = RequestedMaximumDistance;
	OutLifetime.ActorLifeSpanSeconds = static_cast<float>(ActorLifeSpan);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponHostLifetime::TryCreateArc(
	const FShanmenThrownWeaponLaunchReceipt& Launch,
	Fdemo_mapShanmenThrownWeaponHostLifetime& OutLifetime)
{
	OutLifetime = Fdemo_mapShanmenThrownWeaponHostLifetime();
	const double FlightTime = Launch.GetFlightTimeSeconds();
	if (!Launch.IsValid()
		|| Launch.GetTrajectoryKind()
			!= EShanmenThrownWeaponTrajectoryKind::BallisticArc
		|| !IsUsableActorLifeSpan(FlightTime))
	{
		return false;
	}

	OutLifetime.Kind =
		Edemo_mapShanmenThrownWeaponHostLifetimeKind::ArcFlightTime;
	OutLifetime.FlightTimeSeconds = FlightTime;
	OutLifetime.ActorLifeSpanSeconds = static_cast<float>(FlightTime);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponHostLifetime::IsValidFor(
	const FShanmenThrownWeaponLaunchReceipt& Launch) const
{
	Fdemo_mapShanmenThrownWeaponHostLifetime Expected;
	const bool bCreated = Kind
		== Edemo_mapShanmenThrownWeaponHostLifetimeKind::RangeDistance
		? TryCreateRange(Launch, MaximumDistance, Expected)
		: Kind == Edemo_mapShanmenThrownWeaponHostLifetimeKind::ArcFlightTime
			&& TryCreateArc(Launch, Expected);
	return bCreated
		&& Kind == Expected.Kind
		&& MaximumDistance == Expected.MaximumDistance
		&& FlightTimeSeconds == Expected.FlightTimeSeconds
		&& ActorLifeSpanSeconds == Expected.ActorLifeSpanSeconds;
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
	if (Kind == Edemo_mapShanmenThrownWeaponTerminalKind::Impact)
	{
		return Delivery.IsDelivered();
	}
	return (Kind == Edemo_mapShanmenThrownWeaponTerminalKind::BlockingMiss
			|| Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::RangeExpired
			|| Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::FlightTimeExpired)
		&& !Delivery.IsDelivered();
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
	return SpawnAndLaunchPreparedImpl(
		*this,
		World,
		ProjectileClass,
		RequestedSourceActor,
		Origin,
		[this,
			&Authority,
			&Correlation,
			&Preparation,
			&RequestedActionRuntime,
			&RequestedExecution,
			&RequestedCoordinator,
			RequestedSourceActor,
			&Origin,
			&AimDirection,
			RequestedMaximumDistance](
			Ademo_mapShanmenThrownWeaponProjectile& Spawned)
		{
			return TryLaunchPreparedCarrier(
				Authority,
				Correlation,
				Preparation,
				RequestedActionRuntime,
				RequestedExecution,
				Spawned,
				RequestedCoordinator,
				RequestedSourceActor,
				Origin,
				AimDirection,
				RequestedMaximumDistance,
				true);
		});
}

Fdemo_mapShanmenThrownWeaponHostStartResult
Fdemo_mapShanmenThrownWeaponRunHost::TrySpawnAndLaunchPreparedArc(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const FShanmenThrownWeaponArcPlan& ArcPlan)
{
	Fdemo_mapShanmenThrownWeaponHostStartResult Invalid;
	if (!ArcPlan.IsValid()
		|| !IsUsableActorLifeSpan(ArcPlan.GetFlightTimeSeconds()))
	{
		Invalid.Error =
			Edemo_mapShanmenThrownWeaponHostStartError::FlightTimeInvalid;
		return Invalid;
	}
	return SpawnAndLaunchPreparedImpl(
		*this,
		World,
		ProjectileClass,
		RequestedSourceActor,
		ArcPlan.GetRequest().GetOrigin(),
		[this,
			&Authority,
			&Correlation,
			&Preparation,
			&RequestedActionRuntime,
			&RequestedExecution,
			&RequestedCoordinator,
			RequestedSourceActor,
			&ArcPlan](
			Ademo_mapShanmenThrownWeaponProjectile& Spawned)
		{
			return TryLaunchPreparedArcCarrier(
				Authority,
				Correlation,
				Preparation,
				RequestedActionRuntime,
				RequestedExecution,
				Spawned,
				RequestedCoordinator,
				RequestedSourceActor,
				ArcPlan,
				true);
		});
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
	return LaunchPreparedCarrierImpl(
		*this,
		Authority,
		Correlation,
		Preparation,
		RequestedActionRuntime,
		RequestedExecution,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		[&Correlation,
			&Preparation,
			&RequestedActionRuntime,
			&RequestedProjectile,
			RequestedSourceActor,
			&Origin,
			&AimDirection](FShanmenThrownWeaponExecution& Candidate)
		{
			return Fdemo_mapShanmenThrownWeaponWorldAdapter::
				StagePreparedLaunch(
					Correlation,
					Preparation,
					RequestedActionRuntime,
					Candidate,
					RequestedProjectile,
					RequestedSourceActor,
					Origin,
					AimDirection);
		},
		[this,
			&RequestedActionRuntime,
			&RequestedProjectile,
			&RequestedCoordinator,
			RequestedSourceActor,
			RequestedMaximumDistance,
			bDestroyOnTerminal](
			const FShanmenThrownWeaponExecution& Candidate,
			const Fdemo_mapShanmenThrownWeaponItemResult& Committed)
		{
			return TryAdoptPublishedFlight(
				RequestedActionRuntime,
				Candidate,
				Committed,
				RequestedProjectile,
				RequestedCoordinator,
				RequestedSourceActor,
				RequestedMaximumDistance,
				bDestroyOnTerminal);
		});
}

Fdemo_mapShanmenThrownWeaponHostStartResult
Fdemo_mapShanmenThrownWeaponRunHost::TryLaunchPreparedArcCarrier(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const FShanmenThrownWeaponArcPlan& ArcPlan,
	bool bDestroyOnTerminal)
{
	Fdemo_mapShanmenThrownWeaponHostStartResult Result;
	if (State != Edemo_mapShanmenThrownWeaponHostState::Empty)
	{
		Result.Error = Edemo_mapShanmenThrownWeaponHostStartError::HostBusy;
		return Result;
	}
	if (!ArcPlan.IsValid()
		|| !IsUsableActorLifeSpan(ArcPlan.GetFlightTimeSeconds()))
	{
		Result.Error =
			Edemo_mapShanmenThrownWeaponHostStartError::FlightTimeInvalid;
		return Result;
	}
	return LaunchPreparedCarrierImpl(
		*this,
		Authority,
		Correlation,
		Preparation,
		RequestedActionRuntime,
		RequestedExecution,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		[&Correlation,
			&Preparation,
			&RequestedActionRuntime,
			&RequestedProjectile,
			RequestedSourceActor,
			&ArcPlan](FShanmenThrownWeaponExecution& Candidate)
		{
			return Fdemo_mapShanmenThrownWeaponWorldAdapter::
				StagePreparedArcLaunch(
					Correlation,
					Preparation,
					RequestedActionRuntime,
					Candidate,
					RequestedProjectile,
					RequestedSourceActor,
					ArcPlan);
		},
		[this,
			&RequestedActionRuntime,
			&RequestedProjectile,
			&RequestedCoordinator,
			RequestedSourceActor,
			bDestroyOnTerminal](
			const FShanmenThrownWeaponExecution& Candidate,
			const Fdemo_mapShanmenThrownWeaponItemResult& Committed)
		{
			return TryAdoptPublishedArcFlight(
				RequestedActionRuntime,
				Candidate,
				Committed,
				RequestedProjectile,
				RequestedCoordinator,
				RequestedSourceActor,
				bDestroyOnTerminal);
		});
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
	Fdemo_mapShanmenThrownWeaponHostLifetime RequestedLifetime;
	return Fdemo_mapShanmenThrownWeaponHostLifetime::TryCreateRange(
			RequestedExecution.GetLaunchReceipt(),
			RequestedMaximumDistance,
			RequestedLifetime)
		&& TryAdoptPublishedFlightWithLifetime(
		RequestedActionRuntime,
		RequestedExecution,
		CommittedItem,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		RequestedLifetime,
		bDestroyOnTerminal);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::TryAdoptPublishedArcFlight(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
	Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	bool bDestroyOnTerminal)
{
	Fdemo_mapShanmenThrownWeaponHostLifetime RequestedLifetime;
	return Fdemo_mapShanmenThrownWeaponHostLifetime::TryCreateArc(
			RequestedExecution.GetLaunchReceipt(), RequestedLifetime)
		&& TryAdoptPublishedFlightWithLifetime(
			RequestedActionRuntime,
			RequestedExecution,
			CommittedItem,
			RequestedProjectile,
			RequestedCoordinator,
			RequestedSourceActor,
			RequestedLifetime,
			bDestroyOnTerminal);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::
TryAdoptPublishedFlightWithLifetime(
	const FShanmenActionOrchestrator& RequestedActionRuntime,
	const FShanmenThrownWeaponExecution& RequestedExecution,
	const Fdemo_mapShanmenThrownWeaponItemResult& CommittedItem,
	Ademo_mapShanmenThrownWeaponProjectile& RequestedProjectile,
	Fdemo_mapCombatRunCoordinator& RequestedCoordinator,
	AActor* RequestedSourceActor,
	const Fdemo_mapShanmenThrownWeaponHostLifetime& RequestedLifetime,
	bool bDestroyOnTerminal)
{
	if (!ValidateBinding(
		RequestedActionRuntime,
		RequestedExecution,
		CommittedItem,
		RequestedProjectile,
		RequestedCoordinator,
		RequestedSourceActor,
		RequestedLifetime))
	{
		return false;
	}

	ActionRuntime = RequestedActionRuntime;
	Execution = RequestedExecution;
	ItemCommit = CommittedItem;
	Projectile = &RequestedProjectile;
	SourceActor = RequestedSourceActor;
	Coordinator = &RequestedCoordinator;
	Lifetime = RequestedLifetime;
	bDestroyCarrierOnTerminal = bDestroyOnTerminal;
	State = Edemo_mapShanmenThrownWeaponHostState::InFlight;
	BindProjectile();
	if (bDestroyCarrierOnTerminal && RequestedProjectile.GetWorld())
	{
		RequestedProjectile.SetLifeSpan(
			RequestedLifetime.GetActorLifeSpanSeconds());
	}
	return IsValid();
}

bool Fdemo_mapShanmenThrownWeaponRunHost::TryExpireRange()
{
	return Lifetime.GetKind()
			== Edemo_mapShanmenThrownWeaponHostLifetimeKind::RangeDistance
		&& FinishWithoutImpact(
		Edemo_mapShanmenThrownWeaponTerminalKind::RangeExpired, true);
}

bool Fdemo_mapShanmenThrownWeaponRunHost::TryExpireFlightTime()
{
	return Lifetime.GetKind()
			== Edemo_mapShanmenThrownWeaponHostLifetimeKind::ArcFlightTime
		&& FinishWithoutImpact(
			Edemo_mapShanmenThrownWeaponTerminalKind::FlightTimeExpired,
			true);
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
	Lifetime = Fdemo_mapShanmenThrownWeaponHostLifetime();
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
		|| !Execution.IsValid()
		|| !Lifetime.IsValidFor(Execution.GetLaunchReceipt()))
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
	const Fdemo_mapShanmenThrownWeaponHostLifetime& RequestedLifetime) const
{
	return State == Edemo_mapShanmenThrownWeaponHostState::Empty
		&& RequestedActionRuntime.IsValid()
		&& RequestedActionRuntime.CanEmitCandidates()
		&& RequestedExecution.IsValid()
		&& RequestedLifetime.IsValidFor(
			RequestedExecution.GetLaunchReceipt())
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
		const Edemo_mapShanmenThrownWeaponTerminalKind Kind =
			Lifetime.GetKind()
				== Edemo_mapShanmenThrownWeaponHostLifetimeKind::ArcFlightTime
			? Edemo_mapShanmenThrownWeaponTerminalKind::FlightTimeExpired
			: Edemo_mapShanmenThrownWeaponTerminalKind::RangeExpired;
		FinishWithoutImpact(Kind, false);
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
