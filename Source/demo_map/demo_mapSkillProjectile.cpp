#include "demo_mapSkillProjectile.h"
#include "demo_map.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapGameMode.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapM01BossCharacter.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "demo_mapTrainingTarget.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapSkillProjectile::Ademo_mapSkillProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(35.0f);
	// Source filtering and parameters must be configured before collision starts.
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	// WorldStatic is queried as overlap so the projectile can travel horizontally
	// above the floor while still consuming itself on walls and blocking geometry.
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &Ademo_mapSkillProjectile::HandleOverlap);
	Collision->OnComponentHit.AddDynamic(this, &Ademo_mapSkillProjectile::HandleHit);

	VisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileVisual"));
	VisibleMesh->SetupAttachment(Collision);
	VisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisibleMesh->SetStaticMesh(SphereMesh.Object);
	}
	VisibleMesh->SetRelativeScale3D(FVector(0.70f));

	ProjectileLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ProjectileLight"));
	ProjectileLight->SetupAttachment(Collision);
	ProjectileLight->SetLightColor(FLinearColor(0.0f, 0.75f, 1.0f));
	ProjectileLight->SetIntensity(2200.0f);
	ProjectileLight->SetAttenuationRadius(240.0f);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 1000.0f;
	Movement->MaxSpeed = 1000.0f;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bShouldBounce = false;
	Movement->bAutoActivate = false;
}

void Ademo_mapSkillProjectile::InitializeProjectile(AActor* InSourceActor, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams)
{
	bIntendedTargetOnly = false;
	bBossVolleyProjectile = false;
	IntendedTarget.Reset();
	SourceSkillProfileId = NAME_None;
	ProjectileSequence = 0;
	ProjectileOrdinal = INDEX_NONE;
	ApplyConfiguration(InSourceActor, Direction, InParams, FLinearColor(0.0f, 0.75f, 1.0f));
	ActivateForFlight();
}

void Ademo_mapSkillProjectile::InitializeProjectileWithLaunchSegment(AActor* InSourceActor, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams, const FVector& AttackOrigin)
{
	bIntendedTargetOnly = false;
	bBossVolleyProjectile = false;
	IntendedTarget.Reset();
	SourceSkillProfileId = NAME_None;
	ProjectileSequence = 0;
	ProjectileOrdinal = INDEX_NONE;
	ApplyConfiguration(InSourceActor, Direction, InParams, FLinearColor(0.0f, 0.75f, 1.0f));
	const FVector SpawnLocation = GetActorLocation();
	InitialLocation = AttackOrigin;
	const float RemainingDistance = FMath::Max(0.0f, ProjectileParams.MaxDistance - FVector::Dist(AttackOrigin, SpawnLocation));
	SetLifeSpan(ProjectileParams.Speed > KINDA_SMALL_NUMBER ? RemainingDistance / ProjectileParams.Speed : 0.01f);
	if (!ResolveLaunchSegment(AttackOrigin, SpawnLocation)) ActivateForFlight();
}

void Ademo_mapSkillProjectile::InitializeTargetedProjectile(AActor* InSourceActor, AActor* InIntendedTarget, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams, const FLinearColor& InVisualColor)
{
	bIntendedTargetOnly = true;
	bBossVolleyProjectile = false;
	IntendedTarget = InIntendedTarget;
	SourceSkillProfileId = NAME_None;
	ProjectileSequence = 0;
	ProjectileOrdinal = INDEX_NONE;
	ApplyConfiguration(InSourceActor, Direction, InParams, InVisualColor);
	ActivateForFlight();
}

void Ademo_mapSkillProjectile::InitializeTargetedEnemyProjectile(
	AActor* InSourceActor,
	AActor* InIntendedTarget,
	const FVector& Direction,
	const Fdemo_mapProjectileSkillParams& InParams,
	const FLinearColor& InVisualColor,
	FName InSkillProfileId,
	uint64 InProjectileSequence)
{
	bIntendedTargetOnly = true;
	bBossVolleyProjectile = false;
	IntendedTarget = InIntendedTarget;
	SourceSkillProfileId = InSkillProfileId;
	ProjectileSequence = InProjectileSequence;
	ProjectileOrdinal = 0;
	ApplyConfiguration(InSourceActor, Direction, InParams, InVisualColor);
	ActivateForFlight();
}

void Ademo_mapSkillProjectile::InitializeTargetedBossProjectile(
	AActor* InSourceActor,
	AActor* InIntendedTarget,
	const FVector& Direction,
	const Fdemo_mapProjectileSkillParams& InParams,
	const FLinearColor& InVisualColor,
	uint64 InAttackSequence,
	int32 InProjectileOrdinal)
{
	bIntendedTargetOnly = true;
	bBossVolleyProjectile = true;
	IntendedTarget = InIntendedTarget;
	SourceSkillProfileId = NAME_None;
	ProjectileSequence = InAttackSequence;
	ProjectileOrdinal = InProjectileOrdinal;
	ApplyConfiguration(InSourceActor, Direction, InParams, InVisualColor);
	ActivateForFlight();
}

void Ademo_mapSkillProjectile::ApplyConfiguration(AActor* InSourceActor, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams, const FLinearColor& InVisualColor)
{
	SourceActor = InSourceActor;
	ProjectileParams = InParams;
	VisualColor = InVisualColor;
	ProjectileParams.Width = ProjectileParams.CollisionRadius * 2.0f;
	SetOwner(InSourceActor);
	if (APawn* SourcePawn = Cast<APawn>(InSourceActor))
	{
		SetInstigator(SourcePawn);
	}
	Collision->SetSphereRadius(ProjectileParams.CollisionRadius, true);
	Collision->IgnoreActorWhenMoving(InSourceActor, true);
	VisibleMesh->SetRelativeScale3D(FVector(ProjectileParams.Width / 100.0f));
	if (UMaterialInterface* BaseMaterial = VisibleMesh->GetMaterial(0))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), VisualColor);
		Material->SetVectorParameterValue(TEXT("BaseColor"), VisualColor);
		VisibleMesh->SetMaterial(0, Material);
	}
	ProjectileLight->SetLightColor(VisualColor);
	const FVector SafeDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	Movement->InitialSpeed = ProjectileParams.Speed;
	Movement->MaxSpeed = ProjectileParams.Speed;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->Velocity = SafeDirection * ProjectileParams.Speed;
	InitialLocation = GetActorLocation();
	const float MaximumLifetime = ProjectileParams.Speed > KINDA_SMALL_NUMBER ? ProjectileParams.MaxDistance / ProjectileParams.Speed : 0.05f;
	SetLifeSpan(MaximumLifetime);
}

void Ademo_mapSkillProjectile::ActivateForFlight()
{
	if (bConsumed) return;
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Movement->Activate(true);
}

bool Ademo_mapSkillProjectile::ResolveLaunchSegment(const FVector& AttackOrigin, const FVector& SpawnLocation)
{
	UWorld* World = GetWorld();
	if (!World || bConsumed) return bConsumed;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(V3CloseRangeProjectileLaunch), false, SourceActor);
	QueryParams.AddIgnoredActor(this);
	if (SourceActor) QueryParams.AddIgnoredActor(SourceActor);
	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, AttackOrigin, SpawnLocation, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(ProjectileParams.CollisionRadius), QueryParams);
	Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
	for (const FHitResult& Hit : Hits)
	{
		if (HandleProjectileContact(Hit.GetActor(), Hit.GetComponent(), &Hit, Hit.bBlockingHit)) return true;
	}
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, SpawnLocation, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(ProjectileParams.CollisionRadius), QueryParams);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (HandleProjectileContact(Overlap.GetActor(), Overlap.GetComponent(), nullptr, false)) return true;
	}
	return bConsumed;
}

bool Ademo_mapSkillProjectile::IsAlreadyDefeated(const AActor* OtherActor) const
{
	if (OtherActor == nullptr || !OtherActor->CanBeDamaged()) return true;
	if (const Ademo_mapEnemyCharacter* Enemy = Cast<Ademo_mapEnemyCharacter>(OtherActor)) return Enemy->IsDead();
	if (const Ademo_mapRangedEnemyCharacter* Enemy = Cast<Ademo_mapRangedEnemyCharacter>(OtherActor)) return Enemy->IsDead();
	if (const Ademo_mapHeavyEnemyCharacter* Enemy = Cast<Ademo_mapHeavyEnemyCharacter>(OtherActor)) return Enemy->IsDead();
	if (const Ademo_mapTrainingTarget* Target = Cast<Ademo_mapTrainingTarget>(OtherActor)) return Target->WasDestroyedByDamage();
	if (const Udemo_mapPlayerHealthComponent* Health = OtherActor->FindComponentByClass<Udemo_mapPlayerHealthComponent>()) return Health->IsDefeated();
	return false;
}

void Ademo_mapSkillProjectile::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32, bool, const FHitResult& SweepResult)
{
	HandleProjectileContact(OtherActor, OtherComponent, &SweepResult, false);
}

void Ademo_mapSkillProjectile::HandleHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector, const FHitResult& Hit)
{
	HandleProjectileContact(OtherActor, OtherComponent, &Hit, true);
}

bool Ademo_mapSkillProjectile::HandleProjectileContact(AActor* OtherActor, UPrimitiveComponent* OtherComponent, const FHitResult* HitResult, bool)
{
	if (bConsumed || !IsValid(OtherActor) || OtherActor == this || OtherActor == SourceActor || OtherActor == GetOwner() || OtherActor == GetInstigator()) return bConsumed;
	if (ContactedActors.Contains(OtherActor)) return bConsumed;

	const FVector ImpactLocation = HitResult != nullptr && !HitResult->ImpactPoint.IsNearlyZero() ? FVector(HitResult->ImpactPoint) : GetActorLocation();
	if (OtherComponent != nullptr && OtherComponent->GetCollisionObjectType() == ECC_WorldStatic)
	{
		ConsumeAt(ImpactLocation, FColor::White);
		return true;
	}
	if (IsAlreadyDefeated(OtherActor)) return false;
	if (bIntendedTargetOnly && OtherActor != IntendedTarget.Get()) return false;

	const Edemo_mapTargetRelation Relation = Fdemo_mapCombatTargeting::ResolveRelation(SourceActor, OtherActor);
	if (Relation == Edemo_mapTargetRelation::Self) return false;
	if (Relation == Edemo_mapTargetRelation::Friendly && ProjectileParams.bPassThroughFriendlies) return false;
	if (Relation != Edemo_mapTargetRelation::Hostile || !Fdemo_mapCombatTargeting::CanAffect(SourceActor, OtherActor, ProjectileParams.CommonParams.TargetFilter)) return false;

	ContactedActors.Add(OtherActor);
	bool bUsedCanonicalProduct = false;
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	Ademo_mapGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<Ademo_mapGameMode>()
		: nullptr;
	if (Ademo_mapM01BossCharacter* BossSource =
		Cast<Ademo_mapM01BossCharacter>(SourceActor))
	{
		if (GameMode && GameMode->ShouldUseM01EnemyAttackProductPath())
		{
			bUsedCanonicalProduct = true;
			const FVector ImpactNormal = HitResult
				? FVector(HitResult->ImpactNormal)
				: -Movement->Velocity.GetSafeNormal();
			ProductResult =
				GameMode->ExecuteM01BossVolleyProjectileImpact(
					BossSource,
					Cast<APawn>(OtherActor),
					ProjectileSequence,
					ProjectileOrdinal,
					ProjectileParams.CommonParams.Damage,
					ImpactLocation,
					ImpactNormal);
		}
	}
	else if (Ademo_mapRangedEnemyCharacter* RangedSource =
		Cast<Ademo_mapRangedEnemyCharacter>(SourceActor))
	{
		if (GameMode && GameMode->ShouldUseM01EnemyAttackProductPath())
		{
			bUsedCanonicalProduct = true;
			const FVector ImpactNormal = HitResult
				? FVector(HitResult->ImpactNormal)
				: -Movement->Velocity.GetSafeNormal();
			ProductResult =
				GameMode->ExecuteM01EnemyRangedProjectileImpact(
					RangedSource,
					Cast<APawn>(OtherActor),
					SourceSkillProfileId,
					ProjectileSequence,
					ProjectileParams.CommonParams.Damage,
					ImpactLocation,
					ImpactNormal);
		}
	}
	MarkConsumed();
	if (!bUsedCanonicalProduct)
	{
		UGameplayStatics::ApplyDamage(OtherActor, ProjectileParams.CommonParams.Damage, SourceActor != nullptr ? SourceActor->GetInstigatorController() : nullptr, SourceActor, nullptr);
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_ENEMY_PROJECTILE Event=ActorContact Canonical=%d BossVolley=%d Sequence=%llu Ordinal=%d Error=%d Applied=%.3f"),
		bUsedCanonicalProduct ? 1 : 0,
		bBossVolleyProjectile ? 1 : 0,
		static_cast<unsigned long long>(ProjectileSequence),
		ProjectileOrdinal,
		static_cast<int32>(ProductResult.Error),
		ProductResult.GetNewlyCommittedDamage());
	DrawDebugSphere(GetWorld(), ImpactLocation, ProjectileParams.CollisionRadius * 1.8f, 16, bIntendedTargetOnly ? FColor(245, 40, 255) : FColor::Cyan, false, 0.22f, 0, 4.0f);
	Destroy();
	return true;
}

void Ademo_mapSkillProjectile::MarkConsumed()
{
	if (bConsumed) return;
	bConsumed = true;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();
}

void Ademo_mapSkillProjectile::ConsumeAt(const FVector& Location, const FColor& Color)
{
	if (bConsumed)
	{
		return;
	}
	MarkConsumed();
	DrawDebugSphere(GetWorld(), Location, ProjectileParams.CollisionRadius * 1.8f, 16, Color, false, 0.22f, 0, 4.0f);
	Destroy();
}
