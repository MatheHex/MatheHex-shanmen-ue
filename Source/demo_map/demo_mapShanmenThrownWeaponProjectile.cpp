#include "demo_mapShanmenThrownWeaponProjectile.h"

#include "Components/SphereComponent.h"
#include "Engine/HitResult.h"
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
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::Empty
		|| !InLaunch.IsValid()
		|| !InContext.IsValid()
		|| !IsValid(InSourceActor)
		|| InSourceActor == this
		|| InContext.GetDetectorKind()
			!= EShanmenHitDetectorKind::Projectile
		|| InContext.GetHitOrdinal() != 0
		|| !ActionsMatch(InLaunch.GetAction(), InContext.GetAction()))
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
	Movement->InitialSpeed = InLaunch.GetSpeed();
	Movement->MaxSpeed = InLaunch.GetSpeed();
	Movement->Velocity = InLaunch.GetDirection() * InLaunch.GetSpeed();
	SetActorLocationAndRotation(
		InLaunch.GetOrigin(),
		InLaunch.GetDirection().Rotation(),
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
		&& !Movement->IsActive();
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
		&& Movement->IsActive();
}

void Ademo_mapShanmenThrownWeaponProjectile::ActivateCommittedLaunch()
{
	check(State == Edemo_mapShanmenThrownWeaponProjectileState::Staged);
	check(LaunchReceipt.IsValid() && HitContext.IsValid());
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Movement->Velocity =
		LaunchReceipt.GetDirection() * LaunchReceipt.GetSpeed();
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
