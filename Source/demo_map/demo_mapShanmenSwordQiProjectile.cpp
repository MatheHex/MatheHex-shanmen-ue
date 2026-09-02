#include "demo_mapShanmenSwordQiProjectile.h"

#include "Components/SphereComponent.h"
#include "Engine/HitResult.h"
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

Ademo_mapShanmenSwordQiProjectile::Ademo_mapShanmenSwordQiProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(
		TEXT("SwordQiCollision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(16.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->OnComponentHit.AddDynamic(
		this, &Ademo_mapShanmenSwordQiProjectile::HandleHit);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("SwordQiMovement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bShouldBounce = false;
	Movement->bIsHomingProjectile = false;
	Movement->bRotationFollowsVelocity = true;
	Movement->bAutoActivate = false;
}

bool Ademo_mapShanmenSwordQiProjectile::TryStageLaunch(
	const FShanmenSwordQiLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext,
	AActor* InSourceActor)
{
	if (State == Edemo_mapShanmenSwordQiProjectileState::Staged)
	{
		return SourceActor == InSourceActor
			&& IsStagedFor(InLaunch, InContext);
	}
	if (State != Edemo_mapShanmenSwordQiProjectileState::Empty
		|| !InLaunch.IsValid()
		|| !InContext.IsValid()
		|| !::IsValid(InSourceActor)
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
	State = Edemo_mapShanmenSwordQiProjectileState::Staged;
	return IsStagedFor(InLaunch, InContext);
}

bool Ademo_mapShanmenSwordQiProjectile::IsStagedFor(
	const FShanmenSwordQiLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext) const
{
	return State == Edemo_mapShanmenSwordQiProjectileState::Staged
		&& LaunchReceipt.IsValid()
		&& InLaunch.IsValid()
		&& LaunchReceipt.GetLaunchId() == InLaunch.GetLaunchId()
		&& ContextsMatch(HitContext, InContext)
		&& Collision
		&& Movement
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !Movement->IsActive();
}

bool Ademo_mapShanmenSwordQiProjectile::IsInFlightFor(
	const FShanmenSwordQiLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext) const
{
	return State == Edemo_mapShanmenSwordQiProjectileState::InFlight
		&& LaunchReceipt.IsValid()
		&& InLaunch.IsValid()
		&& LaunchReceipt.GetLaunchId() == InLaunch.GetLaunchId()
		&& ContextsMatch(HitContext, InContext)
		&& Collision
		&& Movement
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& Movement->IsActive();
}

void Ademo_mapShanmenSwordQiProjectile::ActivateStagedLaunch()
{
	check(State == Edemo_mapShanmenSwordQiProjectileState::Staged);
	check(LaunchReceipt.IsValid() && HitContext.IsValid());
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Movement->Velocity =
		LaunchReceipt.GetDirection() * LaunchReceipt.GetSpeed();
	Movement->Activate(true);
	State = Edemo_mapShanmenSwordQiProjectileState::InFlight;
}

bool Ademo_mapShanmenSwordQiProjectile::CancelStagedLaunch()
{
	if (State != Edemo_mapShanmenSwordQiProjectileState::Staged)
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
	LaunchReceipt = FShanmenSwordQiLaunchReceipt();
	HitContext = FShanmenWorldHitContext();
	State = Edemo_mapShanmenSwordQiProjectileState::Empty;
	return true;
}

bool Ademo_mapShanmenSwordQiProjectile::MarkDissipated()
{
	if (State != Edemo_mapShanmenSwordQiProjectileState::InFlight)
	{
		return false;
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	State = Edemo_mapShanmenSwordQiProjectileState::Dissipated;
	return true;
}

void Ademo_mapShanmenSwordQiProjectile::LifeSpanExpired()
{
	if (State == Edemo_mapShanmenSwordQiProjectileState::InFlight)
	{
		RangeExpiredEvent.Broadcast(*this);
	}
	if (!IsActorBeingDestroyed())
	{
		Super::LifeSpanExpired();
	}
}

void Ademo_mapShanmenSwordQiProjectile::HandleHit(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	FVector,
	const FHitResult& Hit)
{
	if (State != Edemo_mapShanmenSwordQiProjectileState::InFlight
		|| !::IsValid(OtherActor)
		|| OtherActor == this
		|| OtherActor == SourceActor)
	{
		return;
	}
	ContactEvent.Broadcast(*this, Hit);
}
