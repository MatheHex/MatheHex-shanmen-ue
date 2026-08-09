#include "demo_mapRangedEnemyCharacter.h"
#include "demo_map.h"
#include "demo_mapFactionComponent.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapGameMode.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapSkillProjectile.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapEnemySkillRuntimeComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool IsV2FinalRangedDiagnosticsEnabled()
	{
#if !UE_BUILD_SHIPPING
		return FParse::Param(
			FCommandLine::Get(),
			TEXT("V2FinalAutomation"));
#else
		return false;
#endif
	}
}

Ademo_mapRangedEnemyCharacter::Ademo_mapRangedEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->InitCapsuleSize(40.0f, 88.0f);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldDynamic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 600, 0);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	SetCanBeDamaged(true);
	FactionComponent = CreateDefaultSubobject<Udemo_mapFactionComponent>(TEXT("Faction"));
	FactionComponent->SetFaction(Edemo_mapFaction::Hostile);
	EnemySkillRuntime =
		CreateDefaultSubobject<Udemo_mapEnemySkillRuntimeComponent>(
			TEXT("EnemySkillRuntime"));

	VisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangedBody"));
	VisibleMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded()) VisibleMesh->SetStaticMesh(Cylinder.Object);
	VisibleMesh->SetRelativeLocation(FVector(0,0,-38));
	VisibleMesh->SetRelativeScale3D(FVector(0.72f,0.72f,1.55f));
	VisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangedMuzzle"));
	MuzzleMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) MuzzleMesh->SetStaticMesh(Sphere.Object);
	MuzzleMesh->SetRelativeLocation(FVector(62,0,25));
	MuzzleMesh->SetRelativeScale3D(FVector(0.34f));
	MuzzleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RangedLabel"));
	Label->SetupAttachment(GetCapsuleComponent());
	Label->SetRelativeLocation(FVector(0,0,118));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(34.0f);
	Label->SetTextRenderColor(FColor(230,40,255));

	EnemyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RangedPurpleLight"));
	EnemyLight->SetupAttachment(GetCapsuleComponent());
	EnemyLight->SetLightColor(FLinearColor(0.8f,0.02f,1.0f));
	EnemyLight->SetIntensity(2400.0f);
	EnemyLight->SetAttenuationRadius(380.0f);

	ProjectileParams.Width = 50.0f;
	ProjectileParams.CollisionRadius = 25.0f;
	ProjectileParams.Speed = 800.0f;
	ProjectileParams.MaxDistance = 1800.0f;
	ProjectileParams.CommonParams.Damage = 1.0f;
	ProjectileParams.CommonParams.Cooldown = AttackCooldown;
	ProjectileParams.CommonParams.VerticalTolerance = 180.0f;
	ProjectileParams.bPierceHostiles = false;
	ProjectileParams.bPassThroughFriendlies = true;
}

void Ademo_mapRangedEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Base = VisibleMesh->GetMaterial(0))
	{
		VisibleMaterial = UMaterialInstanceDynamic::Create(Base, this);
		VisibleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.75f,0.02f,1.0f));
		VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.75f,0.02f,1.0f));
		VisibleMesh->SetMaterial(0, VisibleMaterial);
		MuzzleMesh->SetMaterial(0, VisibleMaterial);
	}
	RefreshPresentation();
	if (EnemySkillRuntime)
	{
		EnemySkillRuntime->OnResolve.AddUObject(
			this,
			&Ademo_mapRangedEnemyCharacter::HandleBackstepResolve);
	}
#if !UE_BUILD_SHIPPING
	if (IsV2FinalRangedDiagnosticsEnabled())
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("V2_RANGED_DIAG: spawned class=%s world=%s legacy=%d profile=%s state=%d distance=unavailable."),
			*GetClass()->GetName(),
			GetWorld() ? *GetWorld()->GetMapName() : TEXT("None"),
			UsesLegacyRangedBehavior(),
			*SkillProfileId.ToString(),
			static_cast<int32>(State));
	}
#endif
	GetWorldTimerManager().SetTimer(AIUpdateTimer, this, &Ademo_mapRangedEnemyCharacter::UpdateBehavior, AIUpdateInterval, true);
}

APawn* Ademo_mapRangedEnemyCharacter::GetPlayerPawn() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

bool Ademo_mapRangedEnemyCharacter::HasWorldStaticLineOfSight(const APawn* PlayerPawn) const
{
	if (!GetWorld() || !PlayerPawn) return false;
	FHitResult Hit;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(V2DRangedSight), false, this);
	Params.AddIgnoredActor(this);
	const FVector Start = GetActorLocation() + FVector(0,0,55);
	const FVector End = PlayerPawn->GetActorLocation() + FVector(0,0,45);
	return !GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Objects, Params);
}

bool Ademo_mapRangedEnemyCharacter::HasLineOfSightToPlayer() const
{
	return HasWorldStaticLineOfSight(GetPlayerPawn());
}

void Ademo_mapRangedEnemyCharacter::UpdateBehavior()
{
	if (IsDead() || bCombatSuppressed) return;
	APawn* Player = GetPlayerPawn();
	const Udemo_mapPlayerHealthComponent* Health = Player ? Player->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (!Player || (Health && Health->IsDefeated()))
	{
		if (EnemySkillRuntime)
		{
			EnemySkillRuntime->Cancel(false);
		}
		CancelWindup(); StopMovement(); State = Edemo_mapRangedEnemyState::Idle; return;
	}
	if (EnemySkillRuntime && EnemySkillRuntime->IsActive())
	{
#if !UE_BUILD_SHIPPING
		if (IsV2FinalRangedDiagnosticsEnabled()
			&& !bV2DiagLegacyLogged)
		{
			bV2DiagLegacyLogged = true;
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("V2_RANGED_DIAG: skill_runtime_preempted_legacy_flow profile=%s phase=%d distance=%.2f."),
				*SkillProfileId.ToString(),
				static_cast<int32>(
					EnemySkillRuntime->GetSnapshot().Phase),
				FVector::Dist2D(
					GetActorLocation(),
					Player->GetActorLocation()));
		}
#endif
		StopMovement();
		return;
	}
	if (State == Edemo_mapRangedEnemyState::Windup)
	{
		DrawWindupFeedback(); return;
	}
	if (State == Edemo_mapRangedEnemyState::Fire) return;
	const float Distance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	if (Distance > LoseAggroRange) { StopMovement(); State = Edemo_mapRangedEnemyState::Idle; return; }
	if (TryBeginBackstepShot(Player, Distance))
	{
		StopMovement();
		State = Edemo_mapRangedEnemyState::Idle;
		return;
	}
	const bool bCommittedRetreat = State == Edemo_mapRangedEnemyState::Retreat;
	if (IsLegacyRetreatRequired(
			Distance,
			bCommittedRetreat,
			RetreatStartRange,
			RetreatStopRange))
	{
		MoveAwayFromPlayer(Player);
		return;
	}
	if (bCommittedRetreat
		&& IsLegacySafeRangeReached(Distance, RetreatStopRange))
	{
#if !UE_BUILD_SHIPPING
		if (IsV2FinalRangedDiagnosticsEnabled()
			&& !bV2DiagSafeRangeLogged)
		{
			bV2DiagSafeRangeLogged = true;
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("V2_RANGED_DIAG: safe_range_reached distance=%.2f threshold=%.2f retreat_requests=%d los=%d."),
				Distance,
				RetreatStopRange,
				RetreatMoveRequestCount,
				HasWorldStaticLineOfSight(Player));
		}
#endif
		StopMovement();
		ResetRetreatFailure();
	}
	if (Distance > FireMaxRange && Distance <= AggroRange) { MoveTowardPlayer(Player); return; }
	if (Distance >= FireMinRange && Distance <= FireMaxRange && !HasWorldStaticLineOfSight(Player)) { MoveTowardPlayer(Player); return; }
	if (Distance >= FireMinRange && Distance <= FireMaxRange && GetWorld()->GetTimeSeconds() >= NextAttackAllowedTime)
	{
		BeginWindup(Player); return;
	}
	if (Distance >= FireMinRange && Distance <= FireMaxRange)
	{
		StopMovement(); State = Edemo_mapRangedEnemyState::Cooldown; return;
	}
	StopMovement(); State = Edemo_mapRangedEnemyState::Idle;
}

bool Ademo_mapRangedEnemyCharacter::TryBeginBackstepShot(
	APawn* PlayerPawn,
	float Distance)
{
	if (!EnemySkillRuntime || !EnemySkillRuntime->IsReady() || !PlayerPawn)
	{
		return false;
	}
	Fdemo_mapTargetFilter Filter;
	if (!Fdemo_mapCombatTargeting::CanAffect(this, PlayerPawn, Filter)
		|| !HasWorldStaticLineOfSight(PlayerPawn))
	{
		return false;
	}
	const Fdemo_mapEnemySkillDefinition* SkillDefinition =
		Fdemo_mapEnemySkillPrototypeConfig::FindProfile(SkillProfileId);
	if (!SkillDefinition
		|| SkillDefinition->Kind
			!= Edemo_mapEnemySkillKind::RangedBackstepShot)
	{
		return false;
	}
	Fdemo_mapEnemySkillActivationIntent Intent;
	Intent.Definition = *SkillDefinition;
	Intent.Target = PlayerPawn;
	Intent.InitialPlanarDirection =
		GetActorLocation() - PlayerPawn->GetActorLocation();
	Intent.CurrentTargetDistance = Distance;
	const Fdemo_mapEnemySkillStartResult Result =
		EnemySkillRuntime->TryActivate(Intent);
	if (Result.IsAccepted())
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P6_RANGED_BACKSTEP: accepted preflight=%.2f."),
			Result.ResolvedPreflightDistance);
		return true;
	}
	return false;
}

void Ademo_mapRangedEnemyCharacter::HandleBackstepResolve(
	const Fdemo_mapEnemySkillRuntimeSnapshot& Snapshot)
{
	const Fdemo_mapEnemySkillDefinition* SkillDefinition =
		Fdemo_mapEnemySkillPrototypeConfig::FindProfile(SkillProfileId);
	if (Snapshot.Kind != Edemo_mapEnemySkillKind::RangedBackstepShot
		|| !SkillDefinition
		|| !SkillDefinition->CanResolveRangedFire(
				Snapshot.ResolvedDisplacementDistance,
				true,
				true))
	{
		return;
	}
	APawn* Player = GetPlayerPawn();
	const Udemo_mapPlayerHealthComponent* Health =
		Player
			? Player->FindComponentByClass<Udemo_mapPlayerHealthComponent>()
			: nullptr;
	Fdemo_mapTargetFilter Filter;
	const bool bTargetValid =
		Player
		&& (!Health || !Health->IsDefeated())
		&& Fdemo_mapCombatTargeting::CanAffect(this, Player, Filter);
	const bool bHasLineOfSight =
		Player && HasWorldStaticLineOfSight(Player);
	if (!SkillDefinition->CanResolveRangedFire(
			Snapshot.ResolvedDisplacementDistance,
			bTargetValid,
			bHasLineOfSight))
	{
		return;
	}
	FVector FireDirection =
		Player->GetActorLocation() - GetActorLocation();
	FireDirection.Z = 0.0f;
	if (FireDirection.Normalize())
	{
		LockedDirection = FireDirection;
		FireTargetedProjectile(Player, FireDirection);
	}
}

void Ademo_mapRangedEnemyCharacter::MoveTowardPlayer(APawn* PlayerPawn)
{
	State = Edemo_mapRangedEnemyState::Approach;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastMoveRequestTime < 0.36f) return;
	if (AAIController* AI = Cast<AAIController>(GetController())) AI->MoveToActor(PlayerPawn, FireMaxRange * 0.90f, true, true, true, nullptr, true);
	LastMoveRequestTime = Now;
}

void Ademo_mapRangedEnemyCharacter::MoveAwayFromPlayer(APawn* PlayerPawn)
{
	State = Edemo_mapRangedEnemyState::Retreat;
#if !UE_BUILD_SHIPPING
	if (IsV2FinalRangedDiagnosticsEnabled()
		&& !bV2DiagRetreatLogged)
	{
		bV2DiagRetreatLogged = true;
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("V2_RANGED_DIAG: retreat_started distance=%.2f start=%.2f stop=%.2f legacy=%d profile=%s los=%d."),
			FVector::Dist2D(
				GetActorLocation(),
				PlayerPawn->GetActorLocation()),
			RetreatStartRange,
			RetreatStopRange,
			UsesLegacyRangedBehavior(),
			*SkillProfileId.ToString(),
			HasWorldStaticLineOfSight(PlayerPawn));
	}
#endif
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastMoveRequestTime < 0.30f) return;
	if (!TrySubmitRetreatMove(PlayerPawn)) HandleRetreatMoveFailure(PlayerPawn);
	LastMoveRequestTime = Now;
}

bool Ademo_mapRangedEnemyCharacter::TrySubmitRetreatMove(APawn* PlayerPawn)
{
	if (PlayerPawn == nullptr || GetWorld() == nullptr) return false;
	FVector Away = GetActorLocation() - PlayerPawn->GetActorLocation(); Away.Z = 0.0f;
	if (!Away.Normalize()) return false;
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	AAIController* AI = Cast<AAIController>(GetController());
	if (Nav == nullptr || AI == nullptr) return false;

	const float CurrentDistance = FVector::Dist2D(GetActorLocation(), PlayerPawn->GetActorLocation());
	const TArray<float> Steps = { 650.0f, 500.0f, 350.0f };
	const TArray<float> Angles = { 0.0f, 55.0f, -55.0f, 90.0f, -90.0f };
	for (const float Step : Steps)
	{
		for (const float Angle : Angles)
		{
			const FVector Direction = Away.RotateAngleAxis(Angle, FVector::UpVector);
			FNavLocation Projected;
			if (!Nav->ProjectPointToNavigation(GetActorLocation() + Direction * Step, Projected, FVector(220.0f, 220.0f, 200.0f)))
			{
				++RetreatCandidateRejectCount;
				continue;
			}
			if (FVector::Dist2D(Projected.Location, PlayerPawn->GetActorLocation()) < CurrentDistance + 80.0f)
			{
				++RetreatCandidateRejectCount;
				continue;
			}
			UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Projected.Location, this);
			if (Path == nullptr || !Path->IsValid() || Path->IsPartial())
			{
				++RetreatCandidateRejectCount;
				continue;
			}
			const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Projected.Location, 45.0f, true, true, true, false, nullptr, true);
			if (Result != EPathFollowingRequestResult::Failed)
			{
				++RetreatMoveRequestCount;
#if !UE_BUILD_SHIPPING
				if (IsV2FinalRangedDiagnosticsEnabled())
				{
					UE_LOG(
						Logdemo_map,
						Log,
						TEXT("V2_RANGED_DIAG: retreat_nav accepted=%d partial=%d current_distance=%.2f target_distance=%.2f request_count=%d."),
						Result != EPathFollowingRequestResult::Failed,
						Path->IsPartial(),
						CurrentDistance,
						FVector::Dist2D(
							Projected.Location,
							PlayerPawn->GetActorLocation()),
						RetreatMoveRequestCount);
				}
#endif
				ResetRetreatFailure();
				return true;
			}
			++RetreatCandidateRejectCount;
		}
	}
	return false;
}

void Ademo_mapRangedEnemyCharacter::HandleRetreatMoveFailure(APawn* PlayerPawn)
{
	StopMovement();
	State = Edemo_mapRangedEnemyState::Retreat;
	const float Now = GetWorld()->GetTimeSeconds();
	if (RetreatFailureStartTime < 0.0f) RetreatFailureStartTime = Now;
	if (Now - RetreatFailureStartTime < 0.85f || Now - LastRetreatFallbackTime < 1.50f) return;
	LastRetreatFallbackTime = Now;
	++RetreatFallbackCount;
	UE_LOG(Logdemo_map, Warning, TEXT("V2E: ranged retreat fallback after no legal reverse, side, or short NavMesh candidate."));
	if (HasWorldStaticLineOfSight(PlayerPawn) && Now >= NextAttackAllowedTime) BeginWindup(PlayerPawn);
}

void Ademo_mapRangedEnemyCharacter::ResetRetreatFailure() { RetreatFailureStartTime = -1.0f; }

void Ademo_mapRangedEnemyCharacter::BeginWindup(APawn* PlayerPawn)
{
	FVector Direction = PlayerPawn->GetActorLocation() - GetActorLocation(); Direction.Z = 0;
	if (!Direction.Normalize()) return;
	LockedDirection = Direction;
	State = Edemo_mapRangedEnemyState::Windup;
	StopMovement();
	SetActorRotation(LockedDirection.Rotation());
	GetWorldTimerManager().SetTimer(WindupTimer, this, &Ademo_mapRangedEnemyCharacter::CompleteWindup, AttackWindup, false);
	DrawWindupFeedback();
#if !UE_BUILD_SHIPPING
	if (IsV2FinalRangedDiagnosticsEnabled()
		&& !bV2DiagWindupLogged)
	{
		bV2DiagWindupLogged = true;
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("V2_RANGED_DIAG: windup_started time=%.3f duration=%.2f distance=%.2f los=%d target_valid=%d."),
			GetWorld()->GetTimeSeconds(),
			AttackWindup,
			FVector::Dist2D(
				GetActorLocation(),
				PlayerPawn->GetActorLocation()),
			HasWorldStaticLineOfSight(PlayerPawn),
			IsValid(PlayerPawn));
	}
#endif
	UE_LOG(Logdemo_map, Log, TEXT("V2D: ranged windup started direction=(%.2f,%.2f)."), LockedDirection.X, LockedDirection.Y);
}

void Ademo_mapRangedEnemyCharacter::CompleteWindup()
{
	if (IsDead() || bCombatSuppressed || State != Edemo_mapRangedEnemyState::Windup) return;
	APawn* Player = GetPlayerPawn();
	if (!Player || !HasWorldStaticLineOfSight(Player)) { State = Edemo_mapRangedEnemyState::Idle; return; }
#if !UE_BUILD_SHIPPING
	if (IsV2FinalRangedDiagnosticsEnabled()
		&& !bV2DiagWindupCompleteLogged)
	{
		bV2DiagWindupCompleteLogged = true;
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("V2_RANGED_DIAG: windup_completed time=%.3f distance=%.2f los=1 projectile_before=%d."),
			GetWorld()->GetTimeSeconds(),
			FVector::Dist2D(
				GetActorLocation(),
				Player->GetActorLocation()),
			TotalProjectilesFired);
	}
#endif
	FireTargetedProjectile(Player, LockedDirection);
}

bool Ademo_mapRangedEnemyCharacter::FireTargetedProjectile(
	APawn* Player,
	const FVector& Direction)
{
	if (!Player || !GetWorld())
	{
		return false;
	}
	PruneProjectiles();
	const FVector SpawnLocation = GetActorLocation() + Direction * 105.0f + FVector(0,0,55);
	FActorSpawnParameters Params; Params.Owner=this; Params.Instigator=this; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapSkillProjectile* Projectile = GetWorld()->SpawnActor<Ademo_mapSkillProjectile>(Ademo_mapSkillProjectile::StaticClass(), SpawnLocation, Direction.Rotation(), Params);
	if (Projectile)
	{
		Projectile->InitializeTargetedProjectile(this, Player, Direction, ProjectileParams, FLinearColor(0.9f,0.02f,1.0f));
		ActiveProjectiles.Add(Projectile); LastProjectile=Projectile; ++TotalProjectilesFired;
		DrawDebugSphere(GetWorld(), SpawnLocation, 55.0f, 16, FColor(245,40,255), false, 0.18f, 0, 6.0f);
#if !UE_BUILD_SHIPPING
		if (IsV2FinalRangedDiagnosticsEnabled()
			&& !bV2DiagProjectileLogged)
		{
			bV2DiagProjectileLogged = true;
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("V2_RANGED_DIAG: projectile_fired count=%d target_valid=%d los=%d state=%d."),
				TotalProjectilesFired,
				IsValid(Player),
				HasWorldStaticLineOfSight(Player),
				static_cast<int32>(State));
		}
#endif
	}
	NextAttackAllowedTime = GetWorld()->GetTimeSeconds() + AttackCooldown;
	State = Edemo_mapRangedEnemyState::Fire;
	GetWorldTimerManager().SetTimer(FireStateTimer, this, &Ademo_mapRangedEnemyCharacter::FinishFireState, 0.08f, false);
	UE_LOG(Logdemo_map, Log, TEXT("V2D: ranged projectile fired count=%d."), TotalProjectilesFired);
	return Projectile != nullptr;
}

void Ademo_mapRangedEnemyCharacter::FinishFireState() { if (!IsDead() && !bCombatSuppressed) State = Edemo_mapRangedEnemyState::Cooldown; }
void Ademo_mapRangedEnemyCharacter::StopMovement() { if (AAIController* AI=Cast<AAIController>(GetController())) AI->StopMovement(); }
void Ademo_mapRangedEnemyCharacter::CancelWindup() { GetWorldTimerManager().ClearTimer(WindupTimer); GetWorldTimerManager().ClearTimer(FireStateTimer); if(State==Edemo_mapRangedEnemyState::Windup||State==Edemo_mapRangedEnemyState::Fire) State=Edemo_mapRangedEnemyState::Idle; }

void Ademo_mapRangedEnemyCharacter::DrawWindupFeedback() const
{
#if !UE_BUILD_SHIPPING
	const FVector Start=GetActorLocation()+FVector(0,0,55);
	DrawDebugLine(GetWorld(),Start,Start+LockedDirection*ProjectileParams.MaxDistance,FColor(245,30,255),false,AIUpdateInterval+0.04f,0,5.0f);
#endif
}

void Ademo_mapRangedEnemyCharacter::SetCombatSuppressed(bool bSuppressed)
{
	bCombatSuppressed=bSuppressed;
	if(bSuppressed){if(EnemySkillRuntime)EnemySkillRuntime->Cancel(true);CancelWindup();StopMovement();CancelCombatAndProjectiles();}
}

void Ademo_mapRangedEnemyCharacter::ResetEnemySkillForNewRun()
{
	if(EnemySkillRuntime)EnemySkillRuntime->ResetForNewRun();
	CancelWindup();
	CancelCombatAndProjectiles();
	NextAttackAllowedTime=0.0f;
	State=Edemo_mapRangedEnemyState::Idle;
}

void Ademo_mapRangedEnemyCharacter::ConfigureLegacyBehavior()
{
	SkillProfileId = NAME_None;
	bEnhancedEncounter = false;
	if (EnemySkillRuntime)
	{
		EnemySkillRuntime->Cancel(true);
	}
#if !UE_BUILD_SHIPPING
	if (IsV2FinalRangedDiagnosticsEnabled())
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("V2_RANGED_DIAG: explicit_legacy_configured profile=None runtime_active=%d."),
			EnemySkillRuntime && EnemySkillRuntime->IsActive());
	}
#endif
}

bool Ademo_mapRangedEnemyCharacter::ConfigureEncounter(
	const Fdemo_mapEnemyEncounterIdentity& InIdentity,
	const Fdemo_mapEnemyCombatTuning& InTuning,
	bool bInEnhanced)
{
	const Fdemo_mapEnemySkillDefinition* SkillDefinition =
		Fdemo_mapEnemySkillPrototypeConfig::FindProfile(
			InIdentity.SkillProfileId);
	if (!InIdentity.IsValid()
		|| !InTuning.IsValid()
		|| !SkillDefinition
		|| SkillDefinition->Kind
			!= Edemo_mapEnemySkillKind::RangedBackstepShot
		|| InTuning.ProjectileWidth <= 0.0f
		|| InTuning.ProjectileCollisionRadius <= 0.0f
		|| InTuning.ProjectileSpeed <= 0.0f
		|| InTuning.ProjectileMaxDistance <= 0.0f)
	{
		return false;
	}
	EncounterIdentity = InIdentity;
	SkillProfileId = InIdentity.SkillProfileId;
	MaxHealth = InTuning.MaxHealth;
	CurrentHealth = MaxHealth;
	MovementSpeed = InTuning.MovementSpeed;
	AttackWindup = InTuning.AttackWindup;
	AttackCooldown = InTuning.AttackCooldown;
	ProjectileParams.Width = InTuning.ProjectileWidth;
	ProjectileParams.CollisionRadius = InTuning.ProjectileCollisionRadius;
	ProjectileParams.Speed = InTuning.ProjectileSpeed;
	ProjectileParams.MaxDistance = InTuning.ProjectileMaxDistance;
	ProjectileParams.CommonParams.Damage = InTuning.AttackDamage;
	ProjectileParams.CommonParams.Cooldown = AttackCooldown;
	bEnhancedEncounter = bInEnhanced;
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	RefreshPresentation();
	return true;
}

void Ademo_mapRangedEnemyCharacter::CancelCombatAndProjectiles()
{
	for(const TWeakObjectPtr<Ademo_mapSkillProjectile>& P:ActiveProjectiles) if(P.IsValid()) P->Destroy();
	ActiveProjectiles.Reset(); LastProjectile.Reset();
}

void Ademo_mapRangedEnemyCharacter::PruneProjectiles(){ActiveProjectiles.RemoveAll([](const TWeakObjectPtr<Ademo_mapSkillProjectile>& P){return !P.IsValid();});}
int32 Ademo_mapRangedEnemyCharacter::GetActiveProjectileCount() const
{
	int32 Count=0;
	for(const TWeakObjectPtr<Ademo_mapSkillProjectile>& P:ActiveProjectiles)if(P.IsValid())++Count;
	return Count;
}

float Ademo_mapRangedEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const&, AController*, AActor*)
{
	if(IsDead()||DamageAmount<=0)return 0;
	const int32 Applied=FMath::Min(CurrentHealth,FMath::Max(0,FMath::FloorToInt(DamageAmount)));if(Applied<=0)return 0;
	CurrentHealth-=Applied;RefreshPresentation();ShowDamageFeedback();UE_LOG(Logdemo_map,Log,TEXT("V2D: ranged damaged health=%d/%d."),CurrentHealth,MaxHealth);
	if(CurrentHealth==0)EnterDeadState();return static_cast<float>(Applied);
}

void Ademo_mapRangedEnemyCharacter::EnterDeadState()
{
	if(EnemySkillRuntime)EnemySkillRuntime->Cancel(true);
	if (Ademo_mapGameMode* Mode=GetWorld()?Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()):nullptr)
	{
		if(Ademo_mapV3ProgressionManager* V3=Mode->GetV3ProgressionManager())
		{
			if (const Udemo_mapM01EnemyIdentityComponent* M01 =
				FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();
				M01 && M01->IsConfigured())
			{
				M01->ProjectCorpse(V3,LootSourceId,GetActorLocation(),this);
			}
			else if (!EncounterIdentity.LootTableId.IsNone())
			{
				V3->HandleEnemyDeath(EncounterIdentity.LootTableId,LootSourceId,GetActorLocation(),this);
			}
			else
			{
				V3->HandleEnemyDeath(Edemo_mapEnemyLootArchetype::Ranged,LootSourceId,GetActorLocation(),this);
			}
		}
	}
	State=Edemo_mapRangedEnemyState::Dead;SetCanBeDamaged(false);SetActorEnableCollision(false);GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetCharacterMovement()->DisableMovement();StopMovement();CancelWindup();CancelCombatAndProjectiles();RefreshPresentation();if(VisibleMaterial){VisibleMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor(0.08f,0.01f,0.10f));VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor(0.08f,0.01f,0.10f));}
	GetWorldTimerManager().ClearTimer(AIUpdateTimer);GetWorldTimerManager().SetTimer(DestroyTimer,this,&Ademo_mapRangedEnemyCharacter::DestroyAfterDeath,1.0f,false);UE_LOG(Logdemo_map,Log,TEXT("V2D: ranged enemy died."));
}

void Ademo_mapRangedEnemyCharacter::DestroyAfterDeath(){Destroy();}
void Ademo_mapRangedEnemyCharacter::RefreshPresentation()
{
	FString Name=bEnhancedEncounter?TEXT("ENHANCED RANGED"):TEXT("RANGED");
	FLinearColor Color(0.75f,0.02f,1.0f);
	if (const Udemo_mapM01EnemyIdentityComponent* M01 =
		FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();
		M01 && M01->IsConfigured())
	{
		Name=TEXT("M01 RANGED");
		Color=FLinearColor(0.08f,0.62f,1.0f);
		if(VisibleMesh)VisibleMesh->SetRelativeScale3D(FVector(0.72f,0.72f,1.55f));
		if(MuzzleMesh)MuzzleMesh->SetRelativeScale3D(FVector(0.35f,0.85f,0.35f));
		if(Label)Label->SetTextRenderColor(FColor(70,185,255));
	}
	if(VisibleMaterial){VisibleMaterial->SetVectorParameterValue(TEXT("Color"),Color);VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),Color);}
	if(EnemyLight)EnemyLight->SetLightColor(Color);
	if(Label)
	{
		Label->SetText(FText::FromString(IsDead()?FString::Printf(TEXT("%s\nDEFEATED"),*Name):FString::Printf(TEXT("%s\n%d / %d"),*Name,CurrentHealth,MaxHealth)));
	}
}
void Ademo_mapRangedEnemyCharacter::ShowDamageFeedback(){if(VisibleMaterial){VisibleMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor::White);VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor::White);}if(EnemyLight)EnemyLight->SetLightColor(FLinearColor(1.0f,0.8f,0.2f));GetWorldTimerManager().SetTimer(DamageFeedbackTimer,this,&Ademo_mapRangedEnemyCharacter::ClearDamageFeedback,0.20f,false);}
void Ademo_mapRangedEnemyCharacter::ClearDamageFeedback(){if(IsDead())return;RefreshPresentation();}

void Ademo_mapRangedEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(EnemySkillRuntime)EnemySkillRuntime->Cancel(true);GetWorldTimerManager().ClearTimer(AIUpdateTimer);GetWorldTimerManager().ClearTimer(WindupTimer);GetWorldTimerManager().ClearTimer(FireStateTimer);GetWorldTimerManager().ClearTimer(DestroyTimer);GetWorldTimerManager().ClearTimer(DamageFeedbackTimer);CancelCombatAndProjectiles();Super::EndPlay(EndPlayReason);
}
