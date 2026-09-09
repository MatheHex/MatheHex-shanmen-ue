#include "demo_mapShanmenThrownWeaponProjectile.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Engine Cube prototype dimensions in Unreal centimetres. */
	const FVector ThrownWeaponPrototypeFullSize(30.0f, 4.5f, 1.2f);
	const FVector ThrownWeaponBladeBodyFullSize(22.0f, 4.0f, 1.2f);
	const FVector ThrownWeaponBladeBodyOffset(4.0f, 0.25f, 0.0f);
	const FVector ThrownWeaponBladeEdgeFullSize(22.0f, 0.5f, 1.2f);
	const FVector ThrownWeaponBladeEdgeOffset(4.0f, -2.0f, 0.0f);
	const FVector ThrownWeaponGripFullSize(8.0f, 3.0f, 1.2f);
	const FVector ThrownWeaponGripOffset(-11.0f, 0.0f, 0.0f);
	const FLinearColor ThrownWeaponBladeColor(0.62f, 0.78f, 1.0f);
	const FLinearColor ThrownWeaponBladeEdgeColor(0.92f, 0.97f, 1.0f);
	const FLinearColor ThrownWeaponGripColor(0.16f, 0.045f, 0.012f);
	constexpr float EngineCubeSideLength = 100.0f;
	const FLinearColor StraightFlightCueColor(1.0f, 0.48f, 0.08f);
	const FLinearColor ArcFlightCueColor(0.20f, 0.72f, 1.0f);
	constexpr float FlightCueIntensity = 1600.0f;
	constexpr float FlightCueAttenuationRadius = 140.0f;
	constexpr float FlightRollDegreesPerSecond = 720.0f;

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

	bool IsLaunchCorridorClear(
		const Ademo_mapShanmenThrownWeaponProjectile& Projectile,
		const FShanmenThrownWeaponLaunchReceipt& Launch,
		const FThrownWeaponMotionConfig& Motion,
		const AActor* SourceActor)
	{
		const UWorld* World = Projectile.GetWorld();
		const UBoxComponent* Collision = Projectile.GetCollisionComponent();
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
			Motion.InitialVelocity.Rotation().Quaternion();
		const FCollisionShape LaunchShape =
			FCollisionShape::MakeBox(Collision->GetScaledBoxExtent());
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

	PresentationPivot = CreateDefaultSubobject<USceneComponent>(
		TEXT("ThrownWeaponPresentationPivot"));
	PresentationPivot->SetupAttachment(Collision);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ThrownWeaponVisual"));
	Visual->SetupAttachment(PresentationPivot);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetRelativeLocation(ThrownWeaponBladeBodyOffset);
	Visual->SetRelativeScale3D(
		ThrownWeaponBladeBodyFullSize / EngineCubeSideLength);

	BladeEdgeVisual = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ThrownWeaponBladeEdgeVisual"));
	BladeEdgeVisual->SetupAttachment(PresentationPivot);
	BladeEdgeVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BladeEdgeVisual->SetGenerateOverlapEvents(false);
	BladeEdgeVisual->SetRelativeLocation(ThrownWeaponBladeEdgeOffset);
	BladeEdgeVisual->SetRelativeScale3D(
		ThrownWeaponBladeEdgeFullSize / EngineCubeSideLength);

	GripVisual = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ThrownWeaponGripVisual"));
	GripVisual->SetupAttachment(PresentationPivot);
	GripVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GripVisual->SetGenerateOverlapEvents(false);
	GripVisual->SetRelativeLocation(ThrownWeaponGripOffset);
	GripVisual->SetRelativeScale3D(
		ThrownWeaponGripFullSize / EngineCubeSideLength);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Visual->SetStaticMesh(CubeMesh.Object);
		BladeEdgeVisual->SetStaticMesh(CubeMesh.Object);
		GripVisual->SetStaticMesh(CubeMesh.Object);
	}
	BladeMaterial = Visual->CreateDynamicMaterialInstance(0);
	BladeEdgeMaterial = BladeEdgeVisual->CreateDynamicMaterialInstance(0);
	GripMaterial = GripVisual->CreateDynamicMaterialInstance(0);
	if (BladeMaterial)
	{
		BladeMaterial->SetVectorParameterValue(
			TEXT("Color"), ThrownWeaponBladeColor);
		BladeMaterial->SetVectorParameterValue(
			TEXT("BaseColor"), ThrownWeaponBladeColor);
	}
	if (BladeEdgeMaterial)
	{
		BladeEdgeMaterial->SetVectorParameterValue(
			TEXT("Color"), ThrownWeaponBladeEdgeColor);
		BladeEdgeMaterial->SetVectorParameterValue(
			TEXT("BaseColor"), ThrownWeaponBladeEdgeColor);
	}
	if (GripMaterial)
	{
		GripMaterial->SetVectorParameterValue(
			TEXT("Color"), ThrownWeaponGripColor);
		GripMaterial->SetVectorParameterValue(
			TEXT("BaseColor"), ThrownWeaponGripColor);
	}
	SetPresentationVisibility(false);

	VisualRoll = CreateDefaultSubobject<URotatingMovementComponent>(
		TEXT("ThrownWeaponVisualRoll"));
	VisualRoll->SetUpdatedComponent(PresentationPivot);
	VisualRoll->RotationRate = FRotator(
		0.0f, 0.0f, FlightRollDegreesPerSecond);
	VisualRoll->PivotTranslation = FVector::ZeroVector;
	VisualRoll->bRotationInLocalSpace = true;
	VisualRoll->bAutoActivate = false;

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
	return IsPresentationGeometryValid()
		&& PresentationPivot->IsVisible()
		&& Visual->IsVisible()
		&& BladeEdgeVisual->IsVisible()
		&& GripVisual->IsVisible();
}

bool Ademo_mapShanmenThrownWeaponProjectile::
HasPresentationMaterialContrast() const
{
	return BladeMaterial
		&& BladeEdgeMaterial
		&& GripMaterial
		&& BladeMaterial != BladeEdgeMaterial
		&& BladeMaterial != GripMaterial
		&& BladeEdgeMaterial != GripMaterial
		&& Visual
		&& BladeEdgeVisual
		&& GripVisual
		&& Visual->GetMaterial(0) == BladeMaterial
		&& BladeEdgeVisual->GetMaterial(0) == BladeEdgeMaterial
		&& GripVisual->GetMaterial(0) == GripMaterial
		&& BladeMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(
			ThrownWeaponBladeColor, KINDA_SMALL_NUMBER)
		&& BladeEdgeMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(
			ThrownWeaponBladeEdgeColor, KINDA_SMALL_NUMBER)
		&& GripMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(
			ThrownWeaponGripColor, KINDA_SMALL_NUMBER);
}

FVector Ademo_mapShanmenThrownWeaponProjectile::
GetPresentationForwardDirection() const
{
	return PresentationPivot
		? PresentationPivot->GetForwardVector().GetSafeNormal()
		: FVector::ZeroVector;
}

FVector Ademo_mapShanmenThrownWeaponProjectile::
GetPresentationUpDirection() const
{
	return PresentationPivot
		? PresentationPivot->GetUpVector().GetSafeNormal()
		: FVector::ZeroVector;
}

bool Ademo_mapShanmenThrownWeaponProjectile::
IsPresentationRollActive() const
{
	return VisualRoll && VisualRoll->IsActive();
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

bool Ademo_mapShanmenThrownWeaponProjectile::
IsPresentationGeometryValid() const
{
	return PresentationPivot
		&& Visual
		&& BladeEdgeVisual
		&& GripVisual
		&& PresentationPivot->GetAttachParent() == Collision
		&& PresentationPivot->GetRelativeLocation().IsNearlyZero()
		&& PresentationPivot->GetRelativeScale3D().Equals(
			FVector::OneVector, KINDA_SMALL_NUMBER)
		&& Visual->GetAttachParent() == PresentationPivot
		&& BladeEdgeVisual->GetAttachParent() == PresentationPivot
		&& GripVisual->GetAttachParent() == PresentationPivot
		&& Visual->GetStaticMesh()
		&& BladeEdgeVisual->GetStaticMesh() == Visual->GetStaticMesh()
		&& GripVisual->GetStaticMesh() == Visual->GetStaticMesh()
		&& HasPresentationMaterialContrast()
		&& Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& BladeEdgeVisual->GetCollisionEnabled()
			== ECollisionEnabled::NoCollision
		&& GripVisual->GetCollisionEnabled()
			== ECollisionEnabled::NoCollision
		&& !Visual->GetGenerateOverlapEvents()
		&& !BladeEdgeVisual->GetGenerateOverlapEvents()
		&& !GripVisual->GetGenerateOverlapEvents()
		&& Visual->GetRelativeLocation().Equals(
			ThrownWeaponBladeBodyOffset, KINDA_SMALL_NUMBER)
		&& Visual->GetRelativeScale3D().Equals(
			ThrownWeaponBladeBodyFullSize / EngineCubeSideLength,
			KINDA_SMALL_NUMBER)
		&& Visual->GetRelativeRotation().Equals(
			FRotator::ZeroRotator, KINDA_SMALL_NUMBER)
		&& BladeEdgeVisual->GetRelativeLocation().Equals(
			ThrownWeaponBladeEdgeOffset, KINDA_SMALL_NUMBER)
		&& BladeEdgeVisual->GetRelativeScale3D().Equals(
			ThrownWeaponBladeEdgeFullSize / EngineCubeSideLength,
			KINDA_SMALL_NUMBER)
		&& BladeEdgeVisual->GetRelativeRotation().Equals(
			FRotator::ZeroRotator, KINDA_SMALL_NUMBER)
		&& GripVisual->GetRelativeLocation().Equals(
			ThrownWeaponGripOffset, KINDA_SMALL_NUMBER)
		&& GripVisual->GetRelativeScale3D().Equals(
			ThrownWeaponGripFullSize / EngineCubeSideLength,
			KINDA_SMALL_NUMBER)
		&& GripVisual->GetRelativeRotation().Equals(
			FRotator::ZeroRotator, KINDA_SMALL_NUMBER);
}

void Ademo_mapShanmenThrownWeaponProjectile::
SetPresentationVisibility(bool bVisible)
{
	if (PresentationPivot)
	{
		PresentationPivot->SetVisibility(bVisible, false);
	}
	if (Visual)
	{
		Visual->SetVisibility(bVisible, false);
	}
	if (BladeEdgeVisual)
	{
		BladeEdgeVisual->SetVisibility(bVisible, false);
	}
	if (GripVisual)
	{
		GripVisual->SetVisibility(bVisible, false);
	}
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

void Ademo_mapShanmenThrownWeaponProjectile::ResetPresentationRoll()
{
	if (VisualRoll)
	{
		VisualRoll->Deactivate();
	}
	if (PresentationPivot)
	{
		PresentationPivot->SetRelativeRotation(
			FRotator::ZeroRotator,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}
}

bool Ademo_mapShanmenThrownWeaponProjectile::TryStageLaunch(
	const FShanmenThrownWeaponLaunchReceipt& InLaunch,
	const FShanmenWorldHitContext& InContext,
	AActor* InSourceActor,
	Edemo_mapShanmenThrownWeaponProjectileStageError* OutError)
{
	if (OutError)
	{
		*OutError =
			Edemo_mapShanmenThrownWeaponProjectileStageError::ContractRejected;
	}
	if (State == Edemo_mapShanmenThrownWeaponProjectileState::Staged)
	{
		const bool bExactReplay = SourceActor == InSourceActor
			&& IsStagedFor(InLaunch, InContext);
		if (bExactReplay && OutError)
		{
			*OutError =
				Edemo_mapShanmenThrownWeaponProjectileStageError::None;
		}
		return bExactReplay;
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
		|| !IsPresentationGeometryValid()
		|| !VisualRoll
		|| VisualRoll->UpdatedComponent != PresentationPivot
		|| !FlightCueLight
		|| !TryBuildMotionConfig(*this, InLaunch, Motion))
	{
		return false;
	}
	if (!IsLaunchCorridorClear(*this, InLaunch, Motion, InSourceActor))
	{
		if (OutError)
		{
			*OutError = Edemo_mapShanmenThrownWeaponProjectileStageError::
				ReleasePathBlocked;
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
	SetPresentationVisibility(false);
	ResetPresentationRoll();
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
	const bool bStaged = IsStagedFor(InLaunch, InContext);
	if (bStaged && OutError)
	{
		*OutError = Edemo_mapShanmenThrownWeaponProjectileStageError::None;
	}
	return bStaged;
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
		&& IsPresentationGeometryValid()
		&& VisualRoll
		&& FlightCueLight
		&& VisualRoll->UpdatedComponent == PresentationPivot
		&& FlightCueLight->GetAttachParent() == Collision
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !PresentationPivot->IsVisible()
		&& !Visual->IsVisible()
		&& !BladeEdgeVisual->IsVisible()
		&& !GripVisual->IsVisible()
		&& !IsPresentationRollActive()
		&& PresentationPivot->GetRelativeRotation().Equals(
			FRotator::ZeroRotator, KINDA_SMALL_NUMBER)
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
		&& IsPresentationGeometryValid()
		&& VisualRoll
		&& FlightCueLight
		&& VisualRoll->UpdatedComponent == PresentationPivot
		&& Collision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& IsPresentationVisible()
		&& IsPresentationRollActive()
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
	SetPresentationVisibility(true);
	State = Edemo_mapShanmenThrownWeaponProjectileState::InFlight;
	RefreshFlightCue();
	VisualRoll->Activate(true);
}

bool Ademo_mapShanmenThrownWeaponProjectile::CancelStagedLaunch()
{
	if (State != Edemo_mapShanmenThrownWeaponProjectileState::Staged)
	{
		return false;
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetPresentationVisibility(false);
	ResetPresentationRoll();
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
	SetPresentationVisibility(false);
	ResetPresentationRoll();
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
