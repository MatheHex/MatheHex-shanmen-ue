#include "demo_mapShanmenThrownWeaponProjectile.h"

#include "Components/SphereComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
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

	bool ContextsMatch(
		const FShanmenWorldHitContext& Left,
		const FShanmenWorldHitContext& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& ActionsMatch(Left.GetAction(), Right.GetAction())
			&& Left.GetDetectorId() == Right.GetDetectorId()
			&& Left.GetDetectorKind() == Right.GetDetectorKind()
			&& Left.GetHitOrdinal() == Right.GetHitOrdinal();
	}

	struct FThrownWeaponMotionConfig
	{
		FVector InitialVelocity = FVector::ZeroVector;
		float InitialSpeed = 0.0f;
		float MaximumSpeed = 0.0f;
		float GravityScale = 0.0f;
	};

	bool TryBuildMotionConfig(
		const Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		const FShanmenThrownWeaponLaunchReceipt& Launch,
		FThrownWeaponMotionConfig& OutConfig)
	{
		OutConfig = FThrownWeaponMotionConfig();
		if (!Launch.IsValid())
		{
			return false;
		}

		const FVector InitialVelocity = Launch.GetInitialVelocity();
		if (InitialVelocity.ContainsNaN()
			|| !FMath::IsFinite(InitialVelocity.X)
			|| !FMath::IsFinite(InitialVelocity.Y)
			|| !FMath::IsFinite(InitialVelocity.Z)
			|| InitialVelocity.IsNearlyZero()
			|| !FMath::IsFinite(Launch.GetSpeed())
			|| Launch.GetSpeed() <= 0.0f)
		{
			return false;
		}

		OutConfig.InitialVelocity = InitialVelocity;
		OutConfig.InitialSpeed = Launch.GetSpeed();
		if (Launch.GetTrajectoryKind()
			== EShanmenThrownWeaponTrajectoryKind::Straight)
		{
			OutConfig.MaximumSpeed = Launch.GetSpeed();
			return Launch.GetGravityAcceleration().IsNearlyZero();
		}

		const FVector Gravity = Launch.GetGravityAcceleration();
		const UWorld* World = Projectile.GetWorld();
		if (!World
			|| Gravity.ContainsNaN()
			|| !FMath::IsFinite(Gravity.X)
			|| !FMath::IsFinite(Gravity.Y)
			|| !FMath::IsFinite(Gravity.Z)
			|| !FMath::IsNearlyZero(Gravity.X, UE_DOUBLE_SMALL_NUMBER)
			|| !FMath::IsNearlyZero(Gravity.Y, UE_DOUBLE_SMALL_NUMBER)
			|| Gravity.Z >= -UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}

		const double WorldGravityZ = static_cast<double>(World->GetGravityZ());
		const double GravityScale = Gravity.Z / WorldGravityZ;
		if (!FMath::IsFinite(WorldGravityZ)
			|| WorldGravityZ >= -UE_DOUBLE_SMALL_NUMBER
			|| !FMath::IsFinite(GravityScale)
			|| GravityScale <= 0.0
			|| GravityScale > static_cast<double>(MAX_flt))
		{
			return false;
		}

		// Arc speed may increase while descending; zero disables MaxSpeed clamping.
		OutConfig.MaximumSpeed = 0.0f;
		OutConfig.GravityScale = static_cast<float>(GravityScale);
		return true;
	}

	bool MotionMatches(
		const Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		const UProjectileMovementComponent& Movement,
		const FShanmenThrownWeaponLaunchReceipt& Launch,
		bool bRequireInitialVelocity)
	{
		FThrownWeaponMotionConfig Expected;
		return TryBuildMotionConfig(Projectile, Launch, Expected)
			&& FMath::IsNearlyEqual(
				Movement.InitialSpeed, Expected.InitialSpeed)
			&& FMath::IsNearlyEqual(
				Movement.MaxSpeed, Expected.MaximumSpeed)
			&& FMath::IsNearlyEqual(
				Movement.ProjectileGravityScale, Expected.GravityScale)
			&& (!bRequireInitialVelocity
				|| Movement.Velocity.Equals(
					Expected.InitialVelocity, KINDA_SMALL_NUMBER));
	}
}

Ademo_mapShanmenThrownWeaponProjectile::
Ademo_mapShanmenThrownWeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(
		TEXT("ThrownWeaponCollision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(12.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->OnComponentHit.AddDynamic(
		this, &Ademo_mapShanmenThrownWeaponProjectile::HandleHit);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ThrownWeaponMovement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bShouldBounce = false;
	Movement->bIsHomingProjectile = false;
	Movement->bRotationFollowsVelocity = true;
	Movement->bAutoActivate = false;
}

bool Ademo_mapShanmenThrownWeaponProjectile::TryStageLaunch(
	const FShanmenThrownWeaponLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext,
	AActor* InSourceActor)
{
	if (State == Edemo_mapShanmenThrownWeaponProjectileState::Staged)
	{
		return SourceActor == InSourceActor
			&& IsStagedFor(InLaunch, InContext);
	}
	FThrownWeaponMotionConfig Motion;
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::Empty
		|| !InLaunch.IsValid()
		|| !InContext.IsValid()
		|| !IsValid(InSourceActor)
		|| InSourceActor == this
		|| InSourceActor->GetWorld() != GetWorld()
		|| InContext.GetDetectorKind()
			!= EShanmenHitDetectorKind::Projectile
		|| InContext.GetHitOrdinal() != 0
		|| !ActionsMatch(InLaunch.GetAction(), InContext.GetAction())
		|| !TryBuildMotionConfig(*this, InLaunch, Motion))
	{
		return false;
	}

	LaunchReceipt = InLaunch;
	HitContext = InContext;
	SourceActor = InSourceActor;
	SetOwner(InSourceActor);
	if (APawn* SourcePawn = Cast<APawn>(InSourceActor))
	{
		SetInstigator(SourcePawn);
	}
	Collision->IgnoreActorWhenMoving(InSourceActor, true);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->Deactivate();
	Movement->StopMovementImmediately();
	Movement->InitialSpeed = Motion.InitialSpeed;
	Movement->MaxSpeed = Motion.MaximumSpeed;
	Movement->ProjectileGravityScale = Motion.GravityScale;
	Movement->Velocity = Motion.InitialVelocity;
	SetActorLocationAndRotation(
		InLaunch.GetOrigin(),
		Motion.InitialVelocity.Rotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	State = Edemo_mapShanmenThrownWeaponProjectileState::Staged;
	return IsStagedFor(InLaunch, InContext);
}

bool Ademo_mapShanmenThrownWeaponProjectile::IsStagedFor(
	const FShanmenThrownWeaponLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext) const
{
	return State == Edemo_mapShanmenThrownWeaponProjectileState::Staged
		&& LaunchReceipt.IsValid()
		&& InLaunch.IsValid()
		&& LaunchReceipt.GetLaunchId() == InLaunch.GetLaunchId()
		&& ContextsMatch(HitContext, InContext)
		&& Collision
		&& Movement
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !Movement->IsActive()
		&& MotionMatches(*this, *Movement, InLaunch, true);
}

bool Ademo_mapShanmenThrownWeaponProjectile::IsInFlightFor(
	const FShanmenThrownWeaponLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext) const
{
	return State == Edemo_mapShanmenThrownWeaponProjectileState::InFlight
		&& LaunchReceipt.IsValid()
		&& InLaunch.IsValid()
		&& LaunchReceipt.GetLaunchId() == InLaunch.GetLaunchId()
		&& ContextsMatch(HitContext, InContext)
		&& Collision
		&& Movement
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& Movement->IsActive()
		&& MotionMatches(*this, *Movement, InLaunch, false);
}

void Ademo_mapShanmenThrownWeaponProjectile::ActivateCommittedLaunch()
{
	check(State == Edemo_mapShanmenThrownWeaponProjectileState::Staged);
	check(LaunchReceipt.IsValid() && HitContext.IsValid());
	FThrownWeaponMotionConfig Motion;
	check(TryBuildMotionConfig(*this, LaunchReceipt, Motion));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Movement->InitialSpeed = Motion.InitialSpeed;
	Movement->MaxSpeed = Motion.MaximumSpeed;
	Movement->ProjectileGravityScale = Motion.GravityScale;
	Movement->Velocity = Motion.InitialVelocity;
	Movement->Activate(true);
	State = Edemo_mapShanmenThrownWeaponProjectileState::InFlight;
}

bool Ademo_mapShanmenThrownWeaponProjectile::CancelStagedLaunch()
{
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::Staged)
	{
		return false;
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SourceActor)
	{
		Collision->IgnoreActorWhenMoving(SourceActor, false);
	}
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	Movement->InitialSpeed = 0.0f;
	Movement->MaxSpeed = 0.0f;
	Movement->ProjectileGravityScale = 0.0f;
	SetOwner(nullptr);
	SetInstigator(nullptr);
	SourceActor = nullptr;
	LaunchReceipt = FShanmenThrownWeaponLaunchReceipt();
	HitContext = FShanmenWorldHitContext();
	State = Edemo_mapShanmenThrownWeaponProjectileState::Empty;
	return true;
}

bool Ademo_mapShanmenThrownWeaponProjectile::MarkSpent()
{
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::InFlight)
	{
		return false;
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	State = Edemo_mapShanmenThrownWeaponProjectileState::Spent;
	return true;
}

void Ademo_mapShanmenThrownWeaponProjectile::LifeSpanExpired()
{
	if (State == Edemo_mapShanmenThrownWeaponProjectileState::InFlight)
	{
		RangeExpiredEvent.Broadcast(*this);
	}
	if (!IsActorBeingDestroyed())
	{
		Super::LifeSpanExpired();
	}
}

void Ademo_mapShanmenThrownWeaponProjectile::HandleHit(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	FVector,
	const FHitResult& Hit)
{
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::InFlight
		|| !IsValid(OtherActor)
		|| OtherActor == this
		|| OtherActor == SourceActor)
	{
		return;
	}
	ContactEvent.Broadcast(*this, Hit);
}
