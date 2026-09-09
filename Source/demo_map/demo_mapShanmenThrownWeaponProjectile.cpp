#include "demo_mapShanmenThrownWeaponProjectile.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Engine Cube prototype dimensions in Unreal centimetres. */
	const FVector ThrownWeaponPrototypeFullSize(30.0f, 4.5f, 1.2f);
	constexpr float EngineCubeSideLength = 100.0f;
	const FLinearColor StraightFlightCueColor(1.0f, 0.48f, 0.08f);
	const FLinearColor ArcFlightCueColor(0.20f, 0.72f, 1.0f);
	constexpr float FlightCueIntensity = 1600.0f;
	constexpr float FlightCueAttenuationRadius = 140.0f;

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

	Collision = CreateDefaultSubobject<UBoxComponent>(
		TEXT("ThrownWeaponCollision"));
	SetRootComponent(Collision);
	Collision->InitBoxExtent(ThrownWeaponPrototypeFullSize * 0.5f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Collision->SetGenerateOverlapEvents(false);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ThrownWeaponVisual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetRelativeScale3D(
		ThrownWeaponPrototypeFullSize / EngineCubeSideLength);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Visual->SetStaticMesh(CubeMesh.Object);
	}
	Visual->SetVisibility(false, true);

	FlightCueLight = CreateDefaultSubobject<UPointLightComponent>(
		TEXT("ThrownWeaponFlightCueLight"));
	FlightCueLight->SetupAttachment(Collision);
	FlightCueLight->SetIntensity(FlightCueIntensity);
	FlightCueLight->SetAttenuationRadius(FlightCueAttenuationRadius);
	FlightCueLight->SetCastShadows(false);
	FlightCueLight->SetLightColor(StraightFlightCueColor);
	FlightCueLight->SetVisibility(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ThrownWeaponMovement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bShouldBounce = false;
	Movement->bIsHomingProjectile = false;
	Movement->bRotationFollowsVelocity = true;
	Movement->bAutoActivate = false;
}

void Ademo_mapShanmenThrownWeaponProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	check(Movement);
	Movement->OnProjectileStop.AddUniqueDynamic(
		this,
		&Ademo_mapShanmenThrownWeaponProjectile::HandleProjectileStop);
}

bool Ademo_mapShanmenThrownWeaponProjectile::IsPresentationVisible() const
{
	return Visual
		&& Visual->GetStaticMesh()
		&& Visual->IsVisible();
}

FVector Ademo_mapShanmenThrownWeaponProjectile::
GetPresentationForwardDirection() const
{
	return Visual
		? Visual->GetForwardVector().GetSafeNormal()
		: FVector::ZeroVector;
}

bool Ademo_mapShanmenThrownWeaponProjectile::IsFlightCueVisible() const
{
	return FlightCueLight && FlightCueLight->IsVisible();
}

FLinearColor Ademo_mapShanmenThrownWeaponProjectile::
GetFlightCueColor() const
{
	return FlightCueLight
		? FlightCueLight->GetLightColor()
		: FLinearColor::Transparent;
}

void Ademo_mapShanmenThrownWeaponProjectile::RefreshFlightCue()
{
	if (!FlightCueLight)
	{
		return;
	}
	if (LaunchReceipt.IsValid())
	{
		FlightCueLight->SetLightColor(
			LaunchReceipt.GetTrajectoryKind()
				== EShanmenThrownWeaponTrajectoryKind::BallisticArc
				? ArcFlightCueColor
				: StraightFlightCueColor);
	}
	FlightCueLight->SetVisibility(
		State == Edemo_mapShanmenThrownWeaponProjectileState::InFlight);
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
		|| !Visual
		|| !Visual->GetStaticMesh()
		|| !FlightCueLight
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
	Visual->SetVisibility(false, true);
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
	RefreshFlightCue();
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
		&& Visual
		&& FlightCueLight
		&& Visual->GetStaticMesh()
		&& FlightCueLight->GetAttachParent() == Collision
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !IsPresentationVisible()
		&& !IsFlightCueVisible()
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
		&& Visual
		&& FlightCueLight
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& IsPresentationVisible()
		&& IsFlightCueVisible()
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
	Visual->SetVisibility(true, true);
	State = Edemo_mapShanmenThrownWeaponProjectileState::InFlight;
	RefreshFlightCue();
}

bool Ademo_mapShanmenThrownWeaponProjectile::CancelStagedLaunch()
{
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::Staged)
	{
		return false;
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetVisibility(false, true);
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
	RefreshFlightCue();
	return true;
}

bool Ademo_mapShanmenThrownWeaponProjectile::MarkSpent()
{
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::InFlight)
	{
		return false;
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetVisibility(false, true);
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	State = Edemo_mapShanmenThrownWeaponProjectileState::Spent;
	RefreshFlightCue();
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

void Ademo_mapShanmenThrownWeaponProjectile::HandleProjectileStop(
	const FHitResult& Hit)
{
	AActor* OtherActor = Hit.GetActor();
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::InFlight
		|| !IsValid(OtherActor)
		|| OtherActor == this
		|| OtherActor == SourceActor)
	{
		return;
	}
	ContactEvent.Broadcast(*this, Hit);
}
