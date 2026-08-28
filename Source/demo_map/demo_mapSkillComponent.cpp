#include "demo_mapSkillComponent.h"
#include "demo_map.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapSkillProjectile.h"
#include "demo_mapPlayerCombat.h"
#include "demo_mapSectorGeometry.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

Udemo_mapSkillComponent::Udemo_mapSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	CircleParams.CastRange = 900.0f;
	CircleParams.Radius = 250.0f;
	CircleParams.CommonParams.Cooldown = 1.50f;
	CircleParams.CommonParams.VerticalTolerance = 180.0f;

	ConeParams.Radius = 350.0f;
	ConeParams.FullAngleDegrees = 90.0f;
	ConeParams.CommonParams.Cooldown = 1.00f;
	ConeParams.CommonParams.VerticalTolerance = 180.0f;

	ProjectileParams.Width = 70.0f;
	ProjectileParams.CollisionRadius = 35.0f;
	ProjectileParams.Speed = 1000.0f;
	ProjectileParams.MaxDistance = 1400.0f;
	ProjectileParams.CommonParams.Cooldown = 0.65f;
	ProjectileParams.CommonParams.VerticalTolerance = 180.0f;
	ProjectileParams.bPierceHostiles = false;
	ProjectileParams.bPassThroughFriendlies = true;
}

void Udemo_mapSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
	UE_LOG(Logdemo_map, Log, TEXT("V2B: player SkillComponent initialized exactly once."));
}

void Udemo_mapSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelAllSkillState();
	ActiveProjectiles.Reset();
	LastSpawnedProjectile.Reset();
	Super::EndPlay(EndPlayReason);
}

void Udemo_mapSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bGroundCircleTargeting || !CanUseSkills())
	{
		CancelGroundCircleTargeting();
		return;
	}
	DrawTargetingPreview();
}

float Udemo_mapSkillComponent::GetWorldTime() const
{
	return GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
}

bool Udemo_mapSkillComponent::CanUseSkills() const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || GetWorld() == nullptr || GetWorld()->bIsTearingDown)
	{
		return false;
	}
	if (const Udemo_mapPlayerHealthComponent* Health = OwnerActor->FindComponentByClass<Udemo_mapPlayerHealthComponent>(); Health != nullptr && Health->IsDefeated())
	{
		return false;
	}
	const Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode());
	return Mode == nullptr || !Mode->IsResetPending();
}

bool Udemo_mapSkillComponent::IsCircleReady() const { return GetCircleCooldownRemaining() <= 0.0f; }
bool Udemo_mapSkillComponent::IsConeReady() const { return GetConeCooldownRemaining() <= 0.0f; }
bool Udemo_mapSkillComponent::IsProjectileReady() const { return GetProjectileCooldownRemaining() <= 0.0f; }
float Udemo_mapSkillComponent::GetCircleCooldownRemaining() const { return FMath::Max(0.0f, CircleReadyTime - GetWorldTime()); }
float Udemo_mapSkillComponent::GetConeCooldownRemaining() const { return FMath::Max(0.0f, ConeReadyTime - GetWorldTime()); }
float Udemo_mapSkillComponent::GetProjectileCooldownRemaining() const { return FMath::Max(0.0f, ProjectileReadyTime - GetWorldTime()); }

bool Udemo_mapSkillComponent::ToggleGroundCircleTargeting()
{
	if (bGroundCircleTargeting)
	{
		CancelGroundCircleTargeting();
		return true;
	}
	return BeginGroundCircleTargeting();
}

bool Udemo_mapSkillComponent::BeginGroundCircleTargeting()
{
	if (!CanUseSkills() || !IsCircleReady())
	{
		return false;
	}
	bGroundCircleTargeting = true;
	bPreviewHasGroundPoint = false;
	bPreviewInRange = false;
	SetComponentTickEnabled(true);
	return true;
}

void Udemo_mapSkillComponent::CancelGroundCircleTargeting()
{
	bGroundCircleTargeting = false;
	bPreviewHasGroundPoint = false;
	bPreviewInRange = false;
	bExternalPreviewControl = false;
	SetComponentTickEnabled(false);
}

void Udemo_mapSkillComponent::SetAutomationGroundTargetPreview(bool bHasGroundPoint, const FVector& GroundPoint)
{
	bExternalPreviewControl = true;
	UpdateGroundTargetPreview(bHasGroundPoint, GroundPoint);
}

void Udemo_mapSkillComponent::UpdateGroundTargetPreview(bool bHasGroundPoint, const FVector& GroundPoint)
{
	if (!bGroundCircleTargeting)
	{
		return;
	}
	bPreviewHasGroundPoint = bHasGroundPoint;
	PreviewGroundPoint = GroundPoint;
	const AActor* OwnerActor = GetOwner();
	bPreviewInRange = bHasGroundPoint && OwnerActor != nullptr && FVector::Dist2D(OwnerActor->GetActorLocation(), GroundPoint) <= CircleParams.CastRange + KINDA_SMALL_NUMBER;
}

bool Udemo_mapSkillComponent::ConfirmGroundCircle()
{
	return TryCastGroundCircleAt(PreviewGroundPoint, bPreviewHasGroundPoint);
}

bool Udemo_mapSkillComponent::TryCastGroundCircleAt(const FVector& GroundPoint, bool bHasGroundPoint)
{
	AActor* OwnerActor = GetOwner();
	if (!CanUseSkills() || !IsCircleReady() || !bHasGroundPoint || OwnerActor == nullptr)
	{
		return false;
	}
	const float DistanceXY = FVector::Dist2D(OwnerActor->GetActorLocation(), GroundPoint);
	if (DistanceXY > CircleParams.CastRange + KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float DamageSnapshot = Fdemo_mapPlayerCombat::CaptureOutgoingDamage(OwnerActor, 1.0f);
	if (ApplyCircleDamage(GroundPoint, DamageSnapshot) == INDEX_NONE)
	{
		return false;
	}
	CircleReadyTime = GetWorldTime()
		+ Fdemo_mapPlayerCombat::CaptureEffectiveCooldown(
			OwnerActor,
			CircleParams.CommonParams.Cooldown);
	DrawCircleCastVisual(GroundPoint);
	CancelGroundCircleTargeting();
	return true;
}

int32 Udemo_mapSkillComponent::ApplyCircleDamage(const FVector& Center, float DamageSnapshot)
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (World == nullptr || OwnerActor == nullptr)
	{
		return 0;
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(V2BGroundCircle), false, OwnerActor);
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectTypes, FCollisionShape::MakeCapsule(CircleParams.Radius, CircleParams.CommonParams.VerticalTolerance), QueryParams);
	TSet<AActor*> DamagedActors;
	TArray<FOverlapResult> AuthorizedOverlaps;
	Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode());
	const bool bUseM01ProductPath = Mode
		&& Mode->ShouldUseM01PlayerShapeSkillProductPath();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target == nullptr || Target == OwnerActor || DamagedActors.Contains(Target) || !Target->CanBeDamaged()) continue;
		if (FVector::Dist2D(Target->GetActorLocation(), Center) > CircleParams.Radius + KINDA_SMALL_NUMBER) continue;
		if (FMath::Abs(Target->GetActorLocation().Z - Center.Z) > CircleParams.CommonParams.VerticalTolerance + KINDA_SMALL_NUMBER) continue;
		if (!Fdemo_mapCombatTargeting::CanAffect(OwnerActor, Target, CircleParams.CommonParams.TargetFilter)) continue;
		DamagedActors.Add(Target);
		AuthorizedOverlaps.Add(Overlap);
		if (!bUseM01ProductPath)
		{
			UGameplayStatics::ApplyDamage(Target, DamageSnapshot, OwnerActor->GetInstigatorController(), OwnerActor, nullptr);
			DrawHitFeedback(Target, FColor::Purple);
		}
	}
	if (bUseM01ProductPath)
	{
		const Fdemo_mapPlayerShapeSkillExecutionResult Result =
			Mode->ExecuteM01PlayerShapeSkill(
				Edemo_mapPlayerShapeSkillFamily::GroundCircle,
				DamageSnapshot,
				AuthorizedOverlaps,
				Center);
		if (!Result.IsExecuted())
		{
			return INDEX_NONE;
		}
		for (const FOverlapResult& Overlap : AuthorizedOverlaps)
		{
			DrawHitFeedback(Overlap.GetActor(), FColor::Purple);
		}
		return Result.DeliveredImpactCount;
	}
	return DamagedActors.Num();
}

bool Udemo_mapSkillComponent::TryCastSelfSector(const FVector& AimDirection)
{
	FVector Direction(AimDirection.X, AimDirection.Y, 0.0f);
	if (!CanUseSkills() || !IsConeReady() || !Direction.Normalize())
	{
		return false;
	}
	const float DamageSnapshot = Fdemo_mapPlayerCombat::CaptureOutgoingDamage(GetOwner(), 1.0f);
	if (ApplySectorDamage(Direction, DamageSnapshot) == INDEX_NONE)
	{
		return false;
	}
	ConeReadyTime = GetWorldTime()
		+ Fdemo_mapPlayerCombat::CaptureEffectiveCooldown(
			GetOwner(),
			ConeParams.CommonParams.Cooldown);
	DrawSectorCastVisual(Direction);
	return true;
}

int32 Udemo_mapSkillComponent::ApplySectorDamage(const FVector& Direction, float DamageSnapshot)
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (World == nullptr || OwnerActor == nullptr) return 0;
	const FVector Center = OwnerActor->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(V2BSelfSector), false, OwnerActor);
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectTypes, FCollisionShape::MakeCapsule(ConeParams.Radius, ConeParams.CommonParams.VerticalTolerance), QueryParams);
	TSet<AActor*> DamagedActors;
	TArray<FOverlapResult> AuthorizedOverlaps;
	Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode());
	const bool bUseM01ProductPath = Mode
		&& Mode->ShouldUseM01PlayerShapeSkillProductPath();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target == nullptr || Target == OwnerActor || DamagedActors.Contains(Target) || !Target->CanBeDamaged()) continue;
		if (!Fdemo_mapSectorGeometry::IsInsideSectorXY(Center, Direction, Target->GetActorLocation(), ConeParams.Radius, ConeParams.FullAngleDegrees, ConeParams.CommonParams.VerticalTolerance)) continue;
		if (!Fdemo_mapCombatTargeting::CanAffect(OwnerActor, Target, ConeParams.CommonParams.TargetFilter)) continue;
		DamagedActors.Add(Target);
		AuthorizedOverlaps.Add(Overlap);
		if (!bUseM01ProductPath)
		{
			UGameplayStatics::ApplyDamage(Target, DamageSnapshot, OwnerActor->GetInstigatorController(), OwnerActor, nullptr);
			DrawHitFeedback(Target, FColor::Orange);
		}
	}
	if (bUseM01ProductPath)
	{
		const Fdemo_mapPlayerShapeSkillExecutionResult Result =
			Mode->ExecuteM01PlayerShapeSkill(
				Edemo_mapPlayerShapeSkillFamily::SelfSector,
				DamageSnapshot,
				AuthorizedOverlaps,
				Center);
		if (!Result.IsExecuted())
		{
			return INDEX_NONE;
		}
		for (const FOverlapResult& Overlap : AuthorizedOverlaps)
		{
			DrawHitFeedback(Overlap.GetActor(), FColor::Orange);
		}
		return Result.DeliveredImpactCount;
	}
	return DamagedActors.Num();
}

Ademo_mapSkillProjectile* Udemo_mapSkillComponent::TryFireStraightProjectile(const FVector& AimDirection)
{
	FVector Direction(AimDirection.X, AimDirection.Y, 0.0f);
	if (!CanUseSkills() || !IsProjectileReady() || !Direction.Normalize())
	{
		return nullptr;
	}
	Ademo_mapSkillProjectile* Projectile = SpawnProjectile(Direction);
	if (Projectile != nullptr)
	{
		ProjectileReadyTime = GetWorldTime()
			+ Fdemo_mapPlayerCombat::CaptureEffectiveCooldown(
				GetOwner(),
				ProjectileParams.CommonParams.Cooldown);
	}
	return Projectile;
}

Ademo_mapSkillProjectile* Udemo_mapSkillComponent::SpawnProjectile(const FVector& AimDirection)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	FVector Direction(AimDirection.X, AimDirection.Y, 0.0f);
	if (OwnerActor == nullptr || World == nullptr || !Direction.Normalize()) return nullptr;
	PruneProjectiles();
	const FVector SpawnLocation = OwnerActor->GetActorLocation() + Direction * 120.0f + FVector(0.0f, 0.0f, 55.0f);
	Fdemo_mapProjectileSkillParams ProjectileSnapshot = ProjectileParams;
	ProjectileSnapshot.CommonParams.Damage =
		Fdemo_mapPlayerCombat::CaptureOutgoingDamage(OwnerActor, 1.0f);
	Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(
		World->GetAuthGameMode());
	const bool bUseM01ProductPath = Mode
		&& Mode->ShouldUseM01PlayerProjectileProductPath(OwnerActor);
	FActorSpawnParameters Params;
	Params.Owner = OwnerActor;
	Params.Instigator = Cast<APawn>(OwnerActor);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapSkillProjectile* Projectile = World->SpawnActor<Ademo_mapSkillProjectile>(Ademo_mapSkillProjectile::StaticClass(), SpawnLocation, Direction.Rotation(), Params);
	if (Projectile != nullptr)
	{
		const FVector AttackOrigin = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
		if (bUseM01ProductPath)
		{
			const Fdemo_mapPlayerProjectileLaunchResult Launch =
				Mode->PrepareM01PlayerStraightProjectile(
					OwnerActor,
					ProjectileSnapshot.CommonParams.Damage);
			if (!Launch.IsPrepared())
			{
				Projectile->Destroy();
				return nullptr;
			}
			Projectile->
				InitializeCanonicalPlayerProjectileWithLaunchSegment(
					OwnerActor,
					Direction,
					ProjectileSnapshot,
					AttackOrigin,
					Launch.ActivationSequence,
					Launch.ActivationId);
		}
		else
		{
			Projectile->InitializeProjectileWithLaunchSegment(
				OwnerActor,
				Direction,
				ProjectileSnapshot,
				AttackOrigin);
		}
		ActiveProjectiles.Add(Projectile);
		LastSpawnedProjectile = Projectile;
	}
	return Projectile;
}

void Udemo_mapSkillComponent::CancelAllSkillState()
{
	CancelGroundCircleTargeting();
	for (const TWeakObjectPtr<Ademo_mapSkillProjectile>& Projectile : ActiveProjectiles)
	{
		if (Projectile.IsValid()) Projectile->Destroy();
	}
	ActiveProjectiles.Reset();
	LastSpawnedProjectile.Reset();
}

void Udemo_mapSkillComponent::PruneProjectiles()
{
	ActiveProjectiles.RemoveAll([](const TWeakObjectPtr<Ademo_mapSkillProjectile>& Projectile){ return !Projectile.IsValid(); });
}

void Udemo_mapSkillComponent::DrawTargetingPreview() const
{
	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || GetWorld() == nullptr) return;
	const FVector OwnerCenter = OwnerActor->GetActorLocation() + FVector(0,0,8);
	DrawDebugCircle(GetWorld(), OwnerCenter, CircleParams.CastRange, 72, FColor(40,130,255), false, 0.0f, 0, 2.5f, FVector(1,0,0), FVector(0,1,0), false);
	if (bPreviewHasGroundPoint)
	{
		const FColor Color = bPreviewInRange ? FColor::Cyan : FColor::Red;
		DrawDebugCircle(GetWorld(), PreviewGroundPoint + FVector(0,0,12), CircleParams.Radius, 48, Color, false, 0.0f, 0, 7.0f, FVector(1,0,0), FVector(0,1,0), false);
		DrawDebugLine(GetWorld(), PreviewGroundPoint + FVector(0,0,10), PreviewGroundPoint + FVector(0,0,120), Color, false, 0.0f, 0, 5.0f);
	}
}

void Udemo_mapSkillComponent::DrawCircleCastVisual(const FVector& Center) const
{
	DrawDebugCircle(GetWorld(), Center + FVector(0,0,15), CircleParams.Radius, 64, FColor::Cyan, false, 0.27f, 0, 10.0f, FVector(1,0,0), FVector(0,1,0), false);
	DrawDebugSphere(GetWorld(), Center + FVector(0,0,35), 45.0f, 16, FColor::White, false, 0.24f, 0, 4.0f);
}

void Udemo_mapSkillComponent::DrawSectorCastVisual(const FVector& Direction) const
{
	const FVector Center = GetOwner()->GetActorLocation() + FVector(0,0,18);
	const float HalfAngle = ConeParams.FullAngleDegrees * 0.5f;
	const FVector Left = Direction.RotateAngleAxis(-HalfAngle, FVector::UpVector);
	const FVector Right = Direction.RotateAngleAxis(HalfAngle, FVector::UpVector);
	DrawDebugLine(GetWorld(), Center, Center + Left * ConeParams.Radius, FColor::Orange, false, 0.27f, 0, 8.0f);
	DrawDebugLine(GetWorld(), Center, Center + Right * ConeParams.Radius, FColor::Orange, false, 0.27f, 0, 8.0f);
	FVector Previous = Center + Left * ConeParams.Radius;
	for (int32 Segment=1; Segment<=18; ++Segment)
	{
		const float Angle = -HalfAngle + ConeParams.FullAngleDegrees * (static_cast<float>(Segment) / 18.0f);
		const FVector Current = Center + Direction.RotateAngleAxis(Angle, FVector::UpVector) * ConeParams.Radius;
		DrawDebugLine(GetWorld(), Previous, Current, FColor::Yellow, false, 0.27f, 0, 7.0f);
		Previous = Current;
	}
}

void Udemo_mapSkillComponent::DrawHitFeedback(const AActor* Target, const FColor& Color) const
{
	if (Target != nullptr)
	{
		DrawDebugSphere(GetWorld(), Target->GetActorLocation() + FVector(0,0,40), 65.0f, 14, Color, false, 0.24f, 0, 5.0f);
	}
}
