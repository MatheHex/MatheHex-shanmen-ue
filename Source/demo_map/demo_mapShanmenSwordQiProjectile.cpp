#include "demo_mapShanmenSwordQiProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Asset-free prototype dimensions in Unreal centimetres. */
	const FVector SwordQiEnergyBladeFullSize(72.0f, 12.0f, 3.0f);
	const FLinearColor SwordQiEnergyColor(0.22f, 0.82f, 1.0f);
	constexpr float EngineCubeSideLength = 100.0f;
	constexpr float SwordQiFlightCueIntensity = 2200.0f;
	constexpr float SwordQiFlightCueAttenuationRadius = 170.0f;

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

	bool IsLaunchCorridorClear(
		const Ademo_mapShanmenSwordQiProjectile& Projectile,
		const FShanmenSwordQiLaunchReceipt& Launch,
		const AActor* SourceActor)
	{
		const UWorld* World = Projectile.GetWorld();
		const USphereComponent* Collision = Projectile.GetCollisionComponent();
		if (!World || !Collision)
		{
			// Pure/headless contract tests intentionally have no physical scene.
			return true;
		}

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(&Projectile);
		QueryParams.AddIgnoredActor(SourceActor);
		const FCollisionResponseParams ResponseParams(
			Collision->GetCollisionResponseToChannels());
		const FVector LaunchOrigin = Launch.GetOrigin();
		const FQuat LaunchRotation =
			Launch.GetDirection().Rotation().Quaternion();
		const FCollisionShape LaunchShape = FCollisionShape::MakeSphere(
			Collision->GetScaledSphereRadius());
		if (World->OverlapBlockingTestByChannel(
			LaunchOrigin,
			LaunchRotation,
			Collision->GetCollisionObjectType(),
			LaunchShape,
			QueryParams,
			ResponseParams))
		{
			return false;
		}

		const FVector SourceOrigin = SourceActor->GetActorLocation();
		return SourceOrigin.Equals(LaunchOrigin, KINDA_SMALL_NUMBER)
			|| !World->SweepTestByChannel(
				SourceOrigin,
				LaunchOrigin,
				LaunchRotation,
				Collision->GetCollisionObjectType(),
				LaunchShape,
				QueryParams,
				ResponseParams);
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

	EnergyBladeVisual = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("SwordQiEnergyBladeVisual"));
	EnergyBladeVisual->SetupAttachment(Collision);
	EnergyBladeVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnergyBladeVisual->SetGenerateOverlapEvents(false);
	EnergyBladeVisual->SetRelativeScale3D(
		SwordQiEnergyBladeFullSize / EngineCubeSideLength);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		EnergyBladeVisual->SetStaticMesh(CubeMesh.Object);
	}
	EnergyBladeMaterial =
		EnergyBladeVisual->CreateDynamicMaterialInstance(0);
	if (EnergyBladeMaterial)
	{
		EnergyBladeMaterial->SetVectorParameterValue(
			TEXT("Color"), SwordQiEnergyColor);
		EnergyBladeMaterial->SetVectorParameterValue(
			TEXT("BaseColor"), SwordQiEnergyColor);
	}

	FlightCueLight = CreateDefaultSubobject<UPointLightComponent>(
		TEXT("SwordQiFlightCueLight"));
	FlightCueLight->SetupAttachment(Collision);
	FlightCueLight->SetIntensity(SwordQiFlightCueIntensity);
	FlightCueLight->SetAttenuationRadius(
		SwordQiFlightCueAttenuationRadius);
	FlightCueLight->SetCastShadows(false);
	FlightCueLight->SetLightColor(SwordQiEnergyColor);
	SetPresentationActive(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("SwordQiMovement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bShouldBounce = false;
	Movement->bIsHomingProjectile = false;
	Movement->bRotationFollowsVelocity = true;
	Movement->bAutoActivate = false;
}

bool Ademo_mapShanmenSwordQiProjectile::IsPresentationVisible() const
{
	return IsPresentationGeometryValid()
		&& EnergyBladeVisual->IsVisible();
}

bool Ademo_mapShanmenSwordQiProjectile::
HasPresentationMaterialColor() const
{
	return EnergyBladeMaterial
		&& EnergyBladeVisual
		&& EnergyBladeVisual->GetMaterial(0) == EnergyBladeMaterial
		&& EnergyBladeMaterial->K2_GetVectorParameterValue(TEXT("Color"))
			.Equals(SwordQiEnergyColor, KINDA_SMALL_NUMBER);
}

bool Ademo_mapShanmenSwordQiProjectile::IsFlightCueVisible() const
{
	return FlightCueLight && FlightCueLight->IsVisible();
}

FLinearColor Ademo_mapShanmenSwordQiProjectile::GetFlightCueColor() const
{
	return FlightCueLight
		? FlightCueLight->GetLightColor()
		: FLinearColor::Transparent;
}

bool Ademo_mapShanmenSwordQiProjectile::
IsPresentationGeometryValid() const
{
	return Collision
		&& EnergyBladeVisual
		&& EnergyBladeVisual->GetAttachParent() == Collision
		&& EnergyBladeVisual->GetStaticMesh()
		&& EnergyBladeVisual->GetCollisionEnabled()
			== ECollisionEnabled::NoCollision
		&& !EnergyBladeVisual->GetGenerateOverlapEvents()
		&& EnergyBladeVisual->GetRelativeLocation().IsNearlyZero()
		&& EnergyBladeVisual->GetRelativeRotation().Equals(
			FRotator::ZeroRotator, KINDA_SMALL_NUMBER)
		&& EnergyBladeVisual->GetRelativeScale3D().Equals(
			SwordQiEnergyBladeFullSize / EngineCubeSideLength,
			KINDA_SMALL_NUMBER)
		&& HasPresentationMaterialColor();
}

void Ademo_mapShanmenSwordQiProjectile::
SetPresentationActive(bool bActive)
{
	if (EnergyBladeVisual)
	{
		EnergyBladeVisual->SetVisibility(bActive, false);
	}
	if (FlightCueLight)
	{
		FlightCueLight->SetVisibility(bActive);
	}
}

bool Ademo_mapShanmenSwordQiProjectile::TryStageLaunch(
	const FShanmenSwordQiLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext,
	AActor* InSourceActor,
	Edemo_mapShanmenSwordQiProjectileStageError* OutError)
{
	if (OutError)
	{
		*OutError =
			Edemo_mapShanmenSwordQiProjectileStageError::ContractRejected;
	}
	if (State == Edemo_mapShanmenSwordQiProjectileState::Staged)
	{
		const bool bExactReplay = SourceActor == InSourceActor
			&& IsStagedFor(InLaunch, InContext);
		if (bExactReplay && OutError)
		{
			*OutError = Edemo_mapShanmenSwordQiProjectileStageError::None;
		}
		return bExactReplay;
	}
	if (State != Edemo_mapShanmenSwordQiProjectileState::Empty
		|| !InLaunch.IsValid()
		|| !InContext.IsValid()
		|| !::IsValid(InSourceActor)
		|| InSourceActor == this
		|| InSourceActor->GetWorld() != GetWorld()
		|| InContext.GetDetectorKind()
			!= EShanmenHitDetectorKind::Projectile
		|| InContext.GetHitOrdinal() != 0
		|| !ActionsMatch(InLaunch.GetAction(), InContext.GetAction()))
	{
		return false;
	}
	if (!IsLaunchCorridorClear(*this, InLaunch, InSourceActor))
	{
		if (OutError)
		{
			*OutError = Edemo_mapShanmenSwordQiProjectileStageError::
				LaunchPathBlocked;
		}
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
	SetPresentationActive(false);
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
	const bool bStaged = IsStagedFor(InLaunch, InContext);
	if (bStaged && OutError)
	{
		*OutError = Edemo_mapShanmenSwordQiProjectileStageError::None;
	}
	return bStaged;
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
	SetPresentationActive(true);
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
	SetPresentationActive(false);
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
	SetPresentationActive(false);
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
