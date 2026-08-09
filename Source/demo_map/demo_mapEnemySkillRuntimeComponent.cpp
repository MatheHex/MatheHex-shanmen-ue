#include "demo_mapEnemySkillRuntimeComponent.h"
#include "demo_mapCombatDisplacement.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

Udemo_mapEnemySkillRuntimeComponent::Udemo_mapEnemySkillRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

double Udemo_mapEnemySkillRuntimeComponent::GetNow() const
{
	return GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
}

bool Udemo_mapEnemySkillRuntimeComponent::IsReady() const
{
	return !IsActive() && GetNow() >= CooldownUntil;
}

Fdemo_mapEnemySkillStartResult
Udemo_mapEnemySkillRuntimeComponent::TryActivate(
	const Fdemo_mapEnemySkillActivationIntent& Intent)
{
	Fdemo_mapEnemySkillStartResult Result;
	if (!Intent.Definition.IsValid())
	{
		Result.Status = Edemo_mapEnemySkillStartStatus::InvalidDefinition;
		return Result;
	}
	if (IsActive())
	{
		Result.Status = Edemo_mapEnemySkillStartStatus::Busy;
		return Result;
	}
	if (GetNow() < CooldownUntil)
	{
		Result.Status = Edemo_mapEnemySkillStartStatus::Cooldown;
		return Result;
	}
	if (!Intent.Target.IsValid())
	{
		Result.Status = Edemo_mapEnemySkillStartStatus::InvalidTarget;
		return Result;
	}
	if (!Intent.Definition.IsInsideTriggerRange(Intent.CurrentTargetDistance))
	{
		Result.Status = Edemo_mapEnemySkillStartStatus::OutsideTriggerRange;
		return Result;
	}
	FVector Direction;
	if (!Fdemo_mapCombatDisplacement::NormalizePlanarDirection(
		Intent.InitialPlanarDirection,
		Direction))
	{
		Result.Status = Edemo_mapEnemySkillStartStatus::InvalidDirection;
		return Result;
	}
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const Fdemo_mapCombatDisplacementResult Preflight =
		Fdemo_mapCombatDisplacement::PreflightWorldStatic(
			Character,
			Direction,
			Intent.Definition.DisplacementDistance);
	Result.ResolvedPreflightDistance = Preflight.ResolvedDistance;
	if (Preflight.ResolvedDistance
		+ KINDA_SMALL_NUMBER
		< Intent.Definition.MinimumResolvedDistance)
	{
		Result.Status =
			Edemo_mapEnemySkillStartStatus::InsufficientResolvedDistance;
		return Result;
	}

	Definition = Intent.Definition;
	Target = Intent.Target;
	LockedDirection = Direction;
	ResolvedPreflightDistance = Preflight.ResolvedDistance;
	ResolvedDisplacementDistance = 0.0f;
	bFirstLegalHitConsumed = false;
	bResolveBroadcast = false;
	Phase = Edemo_mapEnemySkillPhase::Windup;
	PhaseStartTime = GetNow();
	CooldownUntil = PhaseStartTime + Definition.CooldownDuration;
	++ActivationSerial;
	++ActivationCount;
	Result.Status = Edemo_mapEnemySkillStartStatus::Accepted;
	return Result;
}

FVector Udemo_mapEnemySkillRuntimeComponent::ResolveDirectionAtEndOfWindup() const
{
	if (!Definition.bLockDirectionAtEndOfWindup || !Target.IsValid() || !GetOwner())
	{
		return LockedDirection;
	}
	const FVector Between = Definition.bDirectionAwayFromTarget
		? GetOwner()->GetActorLocation() - Target->GetActorLocation()
		: Target->GetActorLocation() - GetOwner()->GetActorLocation();
	FVector Direction;
	return Fdemo_mapCombatDisplacement::NormalizePlanarDirection(
		Between,
		Direction)
		? Direction
		: FVector::ZeroVector;
}

void Udemo_mapEnemySkillRuntimeComponent::EnterDisplacement()
{
	LockedDirection = ResolveDirectionAtEndOfWindup();
	if (LockedDirection.IsNearlyZero())
	{
		EnterRecovery();
		return;
	}
	const Fdemo_mapCombatDisplacementResult Preflight =
		Fdemo_mapCombatDisplacement::PreflightWorldStatic(
			Cast<ACharacter>(GetOwner()),
			LockedDirection,
			Definition.DisplacementDistance);
	ResolvedPreflightDistance = Preflight.ResolvedDistance;
	if (ResolvedPreflightDistance + KINDA_SMALL_NUMBER
		< Definition.MinimumResolvedDistance)
	{
		EnterRecovery();
		return;
	}
	Phase = Edemo_mapEnemySkillPhase::Displacing;
	PhaseStartTime = GetNow();
	ResolvedDisplacementDistance = 0.0f;
}

void Udemo_mapEnemySkillRuntimeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Phase == Edemo_mapEnemySkillPhase::Idle)
	{
		return;
	}
	const double Now = GetNow();
	if (Phase == Edemo_mapEnemySkillPhase::Windup)
	{
		if (Now - PhaseStartTime >= Definition.WindupDuration)
		{
			EnterDisplacement();
		}
		return;
	}
	if (Phase == Edemo_mapEnemySkillPhase::Displacing)
	{
		ACharacter* Character = Cast<ACharacter>(GetOwner());
		if (!Character)
		{
			EnterRecovery();
			return;
		}
		const float Alpha = FMath::Clamp(
			static_cast<float>(
				(Now - PhaseStartTime)
				/ FMath::Max(
					static_cast<double>(Definition.DisplacementDuration),
					UE_DOUBLE_SMALL_NUMBER)),
			0.0f,
			1.0f);
		const float Desired = FMath::Min(
			Definition.DisplacementDistance * Alpha,
			ResolvedPreflightDistance);
		const float Step = FMath::Max(
			0.0f,
			Desired - ResolvedDisplacementDistance);
		if (Step > KINDA_SMALL_NUMBER)
		{
			const FVector Start = Character->GetActorLocation();
			const Fdemo_mapCombatDisplacementResult Move =
				Fdemo_mapCombatDisplacement::MoveCharacterSwept(
					Character,
					LockedDirection,
					Step);
			ResolvedDisplacementDistance += Move.ResolvedDistance;
			if (Move.bBlocked)
			{
				++BlockedDisplacementCount;
			}
			Fdemo_mapEnemySkillDisplacementSegment Segment;
			Segment.Kind = Definition.Kind;
			Segment.Start = Start;
			Segment.End = Character->GetActorLocation();
			Segment.BlockingHit = Move.BlockingHit;
			OnDisplacementSegment.Broadcast(Segment);
			if (Phase != Edemo_mapEnemySkillPhase::Displacing)
			{
				return;
			}
			if (Move.bBlocked)
			{
				ResolveAndRecover();
				return;
			}
		}
		if (Alpha >= 1.0f
			|| ResolvedDisplacementDistance + KINDA_SMALL_NUMBER
				>= ResolvedPreflightDistance)
		{
			ResolveAndRecover();
		}
		return;
	}
	if (Phase == Edemo_mapEnemySkillPhase::Recovery
		&& Now - PhaseStartTime >= Definition.RecoveryDuration)
	{
		Phase = Edemo_mapEnemySkillPhase::Idle;
		Definition = Fdemo_mapEnemySkillDefinition();
		Target.Reset();
		LockedDirection = FVector::ZeroVector;
		ResolvedPreflightDistance = 0.0f;
		ResolvedDisplacementDistance = 0.0f;
		bResolveBroadcast = false;
	}
}

void Udemo_mapEnemySkillRuntimeComponent::ResolveAndRecover()
{
	if (Phase != Edemo_mapEnemySkillPhase::Displacing || bResolveBroadcast)
	{
		return;
	}
	Phase = Edemo_mapEnemySkillPhase::Resolving;
	bResolveBroadcast = true;
	++ResolveCount;
	const Fdemo_mapEnemySkillRuntimeSnapshot Snapshot = GetSnapshot();
	OnResolve.Broadcast(Snapshot);
	if (Phase == Edemo_mapEnemySkillPhase::Resolving)
	{
		EnterRecovery();
	}
}

void Udemo_mapEnemySkillRuntimeComponent::EnterRecovery()
{
	Phase = Edemo_mapEnemySkillPhase::Recovery;
	PhaseStartTime = GetNow();
}

void Udemo_mapEnemySkillRuntimeComponent::MarkFirstLegalHitAndRecover()
{
	if (bFirstLegalHitConsumed
		|| Phase != Edemo_mapEnemySkillPhase::Displacing)
	{
		return;
	}
	bFirstLegalHitConsumed = true;
	++LegalHitCount;
	EnterRecovery();
}

void Udemo_mapEnemySkillRuntimeComponent::Cancel(bool bResetCooldown)
{
	Phase = Edemo_mapEnemySkillPhase::Idle;
	Definition = Fdemo_mapEnemySkillDefinition();
	Target.Reset();
	LockedDirection = FVector::ZeroVector;
	PhaseStartTime = 0.0;
	ResolvedPreflightDistance = 0.0f;
	ResolvedDisplacementDistance = 0.0f;
	bFirstLegalHitConsumed = false;
	bResolveBroadcast = false;
	if (bResetCooldown)
	{
		CooldownUntil = 0.0;
	}
}

void Udemo_mapEnemySkillRuntimeComponent::ResetForNewRun()
{
	Cancel(true);
	ActivationCount = 0;
	ResolveCount = 0;
	LegalHitCount = 0;
	BlockedDisplacementCount = 0;
	ActivationSerial = 0;
}

Fdemo_mapEnemySkillRuntimeSnapshot
Udemo_mapEnemySkillRuntimeComponent::GetSnapshot() const
{
	Fdemo_mapEnemySkillRuntimeSnapshot Snapshot;
	Snapshot.Kind = Definition.Kind;
	Snapshot.Phase = Phase;
	Snapshot.LockedPlanarDirection = LockedDirection;
	Snapshot.ResolvedPreflightDistance = ResolvedPreflightDistance;
	Snapshot.ResolvedDisplacementDistance = ResolvedDisplacementDistance;
	Snapshot.CooldownRemaining =
		FMath::Max(0.0f, static_cast<float>(CooldownUntil - GetNow()));
	Snapshot.ActivationSerial = ActivationSerial;
	Snapshot.bFirstLegalHitConsumed = bFirstLegalHitConsumed;
	return Snapshot;
}

void Udemo_mapEnemySkillRuntimeComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Cancel(true);
	OnResolve.Clear();
	OnDisplacementSegment.Clear();
	Super::EndPlay(EndPlayReason);
}

