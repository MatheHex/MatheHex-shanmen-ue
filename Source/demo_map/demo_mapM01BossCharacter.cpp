#include "demo_mapM01BossCharacter.h"

#include "demo_map.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapFactionComponent.h"
#include "demo_mapGameMode.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapSectorGeometry.h"
#include "demo_mapSkillProjectile.h"
#include "demo_mapV3ProgressionManager.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapM01BossCharacter::Ademo_mapM01BossCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(76.0f, 128.0f);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldDynamic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 320.0f, 0.0f);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	SetCanBeDamaged(true);

	FactionComponent = CreateDefaultSubobject<Udemo_mapFactionComponent>(TEXT("Faction"));
	FactionComponent->SetFaction(Edemo_mapFaction::Hostile);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossBody"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	if (Cylinder.Succeeded()) BodyMesh->SetStaticMesh(Cylinder.Object);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -48.0f));
	BodyMesh->SetRelativeScale3D(FVector(1.65f, 1.65f, 2.35f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CrownMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossCrown"));
	CrownMesh->SetupAttachment(GetCapsuleComponent());
	if (Cone.Succeeded()) CrownMesh->SetStaticMesh(Cone.Object);
	CrownMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 98.0f));
	CrownMesh->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.65f));
	CrownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossCore"));
	CoreMesh->SetupAttachment(GetCapsuleComponent());
	if (Sphere.Succeeded()) CoreMesh->SetStaticMesh(Sphere.Object);
	CoreMesh->SetRelativeLocation(FVector(82.0f, 0.0f, 30.0f));
	CoreMesh->SetRelativeScale3D(FVector(0.38f));
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BossLabel"));
	Label->SetupAttachment(GetCapsuleComponent());
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 185.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(48.0f);
	Label->SetTextRenderColor(FColor(235, 80, 255));

	BossLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BossLight"));
	BossLight->SetupAttachment(GetCapsuleComponent());
	BossLight->SetLightColor(FLinearColor(0.72f, 0.04f, 1.0f));
	BossLight->SetIntensity(5600.0f);
	BossLight->SetAttenuationRadius(720.0f);

	ProjectileParams.Width = 82.0f;
	ProjectileParams.CollisionRadius = 34.0f;
	ProjectileParams.Speed = 920.0f;
	ProjectileParams.MaxDistance = 2100.0f;
	ProjectileParams.CommonParams.Damage = 2.0f;
	ProjectileParams.CommonParams.Cooldown = AttackCooldown;
}

void Ademo_mapM01BossCharacter::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	if (UMaterialInterface* Base = BodyMesh ? BodyMesh->GetMaterial(0) : nullptr)
	{
		BodyMaterial = UMaterialInstanceDynamic::Create(Base, this);
		BodyMesh->SetMaterial(0, BodyMaterial);
		CrownMesh->SetMaterial(0, BodyMaterial);
	}
	RefreshPresentation();
}

void Ademo_mapM01BossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateBehavior();
	if (State == Edemo_mapM01BossState::Windup)
	{
		DrawTelegraph(FColor(230, 45, 255), DeltaSeconds + 0.05f, 9.0f);
	}
}

bool Ademo_mapM01BossCharacter::ConfigureBoss(
	const Fdemo_mapM01EnemyDefinition& InDefinition)
{
	if (!InDefinition.IsValid()
		|| InDefinition.Archetype != Edemo_mapM01EnemyArchetype::BossMain)
	{
		return false;
	}
	const float ConfiguredVitality =
		static_cast<float>(InDefinition.Tuning.MaxHealth);
	if (!TryCommitVitalityState(ConfiguredVitality, ConfiguredVitality))
	{
		return false;
	}
	Definition = InDefinition;
	MovementSpeed = InDefinition.Tuning.MovementSpeed;
	AttackDamage = InDefinition.Tuning.AttackDamage;
	AttackCooldown = InDefinition.Tuning.AttackCooldown;
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	RefreshPresentation();
	return true;
}

APawn* Ademo_mapM01BossCharacter::GetPlayerPawn() const
{
	APlayerController* FirstPlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;
	return FirstPlayerController ? FirstPlayerController->GetPawn() : nullptr;
}

void Ademo_mapM01BossCharacter::StopMovement()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
}

void Ademo_mapM01BossCharacter::UpdateBehavior()
{
	if (IsDead() || bCombatSuppressed || !GetWorld()) return;
	APawn* Player = GetPlayerPawn();
	const Udemo_mapPlayerHealthComponent* Health = Player
		? Player->FindComponentByClass<Udemo_mapPlayerHealthComponent>()
		: nullptr;
	if (!Player || (Health && Health->IsDefeated()))
	{
		StopMovement();
		State = Edemo_mapM01BossState::Idle;
		return;
	}
	if (State == Edemo_mapM01BossState::Windup
		|| State == Edemo_mapM01BossState::Recovery)
	{
		return;
	}
	const float HomeDistance = FVector::Dist2D(GetActorLocation(), HomeLocation);
	const float PlayerDistance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	const float Now = GetWorld()->GetTimeSeconds();
	if (HomeDistance > 2250.0f || PlayerDistance > 2050.0f)
	{
		State = Edemo_mapM01BossState::Idle;
		if (HomeDistance > 120.0f && Now - LastMoveRequestTime >= 0.35f)
		{
			if (AAIController* AI = Cast<AAIController>(GetController()))
			{
				AI->MoveToLocation(HomeLocation, 80.0f, true, true, true, false, nullptr, true);
			}
			LastMoveRequestTime = Now;
		}
		return;
	}
	if (PlayerDistance <= 1300.0f && Now >= NextAttackAllowedTime)
	{
		BeginAttack(Fdemo_mapM01BossCombatPlanner::SelectAttack(PlayerDistance), Player);
		return;
	}
	if (PlayerDistance > 245.0f && Now - LastMoveRequestTime >= 0.25f)
	{
		State = Edemo_mapM01BossState::Chase;
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->MoveToActor(Player, 220.0f, true, true, true, nullptr, true);
		}
		LastMoveRequestTime = Now;
	}
}

void Ademo_mapM01BossCharacter::BeginAttack(
	Edemo_mapM01BossAttack Attack,
	APawn* PlayerPawn)
{
	if (!PlayerPawn || State == Edemo_mapM01BossState::Windup) return;
	LockedDirection = PlayerPawn->GetActorLocation() - GetActorLocation();
	LockedDirection.Z = 0.0f;
	if (!LockedDirection.Normalize()) return;
	PendingAttack = Attack;
	LastAttack = Attack;
	State = Edemo_mapM01BossState::Windup;
	StopMovement();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	SetActorRotation(LockedDirection.Rotation());
	const float Windup = Attack == Edemo_mapM01BossAttack::Charge ? 0.82f : 0.68f;
	DrawTelegraph(FColor(230, 45, 255), Windup, 9.0f);
	GetWorldTimerManager().SetTimer(
		WindupTimer,
		this,
		&Ademo_mapM01BossCharacter::ResolveAttack,
		Windup,
		false);
	UE_LOG(Logdemo_map, Log, TEXT("M01_BOSS_WINDUP attack=%d."), static_cast<int32>(Attack));
}

bool Ademo_mapM01BossCharacter::HasWorldStaticLineOfSight(
	const APawn* PlayerPawn) const
{
	if (!GetWorld() || !PlayerPawn) return false;
	FHitResult Hit;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(M01BossSight), false, this);
	Params.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByObjectType(
		Hit,
		GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
		PlayerPawn->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f),
		Objects,
		Params);
}

void Ademo_mapM01BossCharacter::ResolveAttack()
{
	if (IsDead() || bCombatSuppressed
		|| State != Edemo_mapM01BossState::Windup || !GetWorld())
	{
		return;
	}
	APawn* Player = GetPlayerPawn();
	Fdemo_mapTargetFilter Filter;
	const bool bCanAffect = Player
		&& Fdemo_mapCombatTargeting::CanAffect(this, Player, Filter);
	if (PendingAttack == Edemo_mapM01BossAttack::Sweep)
	{
		++SweepResolveCount;
		DrawTelegraph(FColor::Yellow, 0.22f, 14.0f);
		if (bCanAffect
			&& HasWorldStaticLineOfSight(Player)
			&& Fdemo_mapSectorGeometry::IsInsideSectorXY(
				GetActorLocation(), LockedDirection, Player->GetActorLocation(),
				340.0f, 118.0f, 180.0f))
		{
			UGameplayStatics::ApplyDamage(Player, AttackDamage, GetController(), this, nullptr);
		}
	}
	else if (PendingAttack == Edemo_mapM01BossAttack::Charge)
	{
		++ChargeResolveCount;
		FHitResult Hit;
		AddActorWorldOffset(LockedDirection * 620.0f, true, &Hit, ETeleportType::None);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() - LockedDirection * 620.0f,
			FColor::Yellow, false, 0.28f, 0, 15.0f);
		if (bCanAffect && FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) <= 190.0f)
		{
			UGameplayStatics::ApplyDamage(Player, AttackDamage, GetController(), this, nullptr);
		}
	}
	else
	{
		++VolleyResolveCount;
		if (Player)
		{
			for (const float Angle : { -11.0f, 0.0f, 11.0f })
			{
				const FVector Direction = LockedDirection.RotateAngleAxis(Angle, FVector::UpVector);
				const FVector SpawnLocation = GetActorLocation()
					+ Direction * 130.0f + FVector(0.0f, 0.0f, 78.0f);
				FActorSpawnParameters Params;
				Params.Owner = this;
				Params.Instigator = this;
				Params.SpawnCollisionHandlingOverride =
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Ademo_mapSkillProjectile* Projectile =
					GetWorld()->SpawnActor<Ademo_mapSkillProjectile>(
						Ademo_mapSkillProjectile::StaticClass(),
						SpawnLocation,
						Direction.Rotation(),
						Params);
				if (Projectile)
				{
					Projectile->InitializeTargetedProjectile(
						this, Player, Direction, ProjectileParams,
						FLinearColor(0.85f, 0.05f, 1.0f));
					ActiveProjectiles.Add(Projectile);
				}
			}
		}
	}
	UE_LOG(Logdemo_map, Log, TEXT("M01_BOSS_RESOLVE attack=%d sweep=%d charge=%d volley=%d."),
		static_cast<int32>(PendingAttack), SweepResolveCount, ChargeResolveCount, VolleyResolveCount);
	NextAttackAllowedTime = GetWorld()->GetTimeSeconds() + AttackCooldown;
	State = Edemo_mapM01BossState::Recovery;
	GetWorldTimerManager().SetTimer(
		RecoveryTimer,
		this,
		&Ademo_mapM01BossCharacter::FinishRecovery,
		0.52f,
		false);
}

void Ademo_mapM01BossCharacter::FinishRecovery()
{
	if (!IsDead() && !bCombatSuppressed)
	{
		State = Edemo_mapM01BossState::Idle;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

bool Ademo_mapM01BossCharacter::TryBindCombatEntity(
	const FGuid& TargetEntityId)
{
	if (!TargetEntityId.IsValid()) return false;
	if (CombatVitalityLedger.IsValid())
	{
		return CombatVitalityLedger.GetTargetEntityId() == TargetEntityId
			&& CombatVitalityLedger.IsSynchronized(
				CurrentVitality,
				MaximumVitality);
	}
	FShanmenVitalityCommitLedger NewLedger;
	if (!FShanmenVitalityCommitLedger::TryCreate(
			TargetEntityId,
			CurrentVitality,
			MaximumVitality,
			0,
			NewLedger))
	{
		return false;
	}
	CombatVitalityLedger = MoveTemp(NewLedger);
	return true;
}

bool Ademo_mapM01BossCharacter::TryEndCombatEntityBinding(
	const FGuid& ExpectedTargetEntityId)
{
	if (!ExpectedTargetEntityId.IsValid()) return false;
	if (!CombatVitalityLedger.IsValid()) return true;
	if (CombatVitalityLedger.GetTargetEntityId() != ExpectedTargetEntityId)
	{
		return false;
	}
	CombatVitalityLedger.Reset();
	return true;
}

bool Ademo_mapM01BossCharacter::TryCaptureCombatVitalitySnapshot(
	FShanmenTargetVitalitySnapshot& OutSnapshot) const
{
	return CombatVitalityLedger.TryCaptureSnapshot(
		CurrentVitality,
		MaximumVitality,
		OutSnapshot);
}

FShanmenVitalityCommitResult Ademo_mapM01BossCharacter::CommitCombatImpact(
	const FShanmenVitalityCommitCommand& Command)
{
	FShanmenVitalityCommitResult Result = CombatVitalityLedger.Commit(
		Command,
		CurrentVitality,
		MaximumVitality);
	if (Result.Status == EShanmenVitalityCommitStatus::Committed
		&& Result.Receipt.GetAppliedDamage() > 0.0f)
	{
		PublishAppliedDamage(Result.Receipt.GetAppliedDamage());
	}
	return Result;
}

bool Ademo_mapM01BossCharacter::TryCommitVitalityState(
	float NewCurrentVitality,
	float NewMaximumVitality)
{
	if (!FMath::IsFinite(NewCurrentVitality)
		|| !FMath::IsFinite(NewMaximumVitality)
		|| NewMaximumVitality <= 0.0f
		|| NewCurrentVitality < 0.0f
		|| NewCurrentVitality > NewMaximumVitality)
	{
		return false;
	}
	if (CombatVitalityLedger.IsValid())
	{
		return CombatVitalityLedger.TryCommitExternalMutation(
			CurrentVitality,
			MaximumVitality,
			NewCurrentVitality,
			NewMaximumVitality);
	}
	CurrentVitality = NewCurrentVitality;
	MaximumVitality = NewMaximumVitality;
	return true;
}

float Ademo_mapM01BossCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const&,
	AController*,
	AActor*)
{
	if (IsDead() || !FMath::IsFinite(DamageAmount)
		|| DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	const float RequestedDamage = static_cast<float>(
		FMath::Max(0, FMath::FloorToInt(DamageAmount)));
	const float AppliedDamage = FMath::Min(CurrentVitality, RequestedDamage);
	if (AppliedDamage <= 0.0f
		|| !TryCommitVitalityState(
			FMath::Max(0.0f, CurrentVitality - AppliedDamage),
			MaximumVitality))
	{
		return 0.0f;
	}
	PublishAppliedDamage(AppliedDamage);
	return AppliedDamage;
}

void Ademo_mapM01BossCharacter::PublishAppliedDamage(float AppliedDamage)
{
	if (!FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f) return;
#if WITH_DEV_AUTOMATION_TESTS
	++PositiveCombatDamageCount;
#endif
	RefreshPresentation();
	ShowDamageFeedback();
	UE_LOG(Logdemo_map, Log,
		TEXT("0.0.10 P4.6: boss damaged; vitality=%.3f/%.3f."),
		CurrentVitality,
		MaximumVitality);
	if (CurrentVitality <= 0.0f) EnterDeadState();
}

void Ademo_mapM01BossCharacter::EnterDeadState()
{
	if (bDeathCommitted) return;
	bDeathCommitted = true;
	State = Edemo_mapM01BossState::Dead;
	CancelCombat();
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	if (Mode)
	{
		if (Ademo_mapV3ProgressionManager* V3 = Mode->GetV3ProgressionManager())
		{
			if (const Udemo_mapM01EnemyIdentityComponent* Identity =
				FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>())
			{
				Identity->ProjectCorpse(V3, LootSourceId, GetActorLocation(), this);
			}
		}
		Mode->NotifyM01BossDefeated(TEXT("M01.Boss.Main"));
	}
	RefreshPresentation();
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.10f, 0.01f, 0.12f));
		BodyMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.10f, 0.01f, 0.12f));
	}
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&Ademo_mapM01BossCharacter::DestroyAfterDeath,
		1.5f,
		false);
	UE_LOG(Logdemo_map, Log, TEXT("M01_BOSS_DEATH committed=1 corpse=1."));
}

void Ademo_mapM01BossCharacter::DestroyAfterDeath()
{
	Destroy();
}

void Ademo_mapM01BossCharacter::SetCombatSuppressed(bool bSuppressed)
{
	bCombatSuppressed = bSuppressed;
	if (bSuppressed) CancelCombat();
}

void Ademo_mapM01BossCharacter::CancelCombat()
{
	GetWorldTimerManager().ClearTimer(WindupTimer);
	GetWorldTimerManager().ClearTimer(RecoveryTimer);
	StopMovement();
	PruneProjectiles();
	for (TWeakObjectPtr<Ademo_mapSkillProjectile>& Projectile : ActiveProjectiles)
	{
		if (Projectile.IsValid()) Projectile->Destroy();
	}
	ActiveProjectiles.Reset();
	if (!IsDead()) State = Edemo_mapM01BossState::Idle;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void Ademo_mapM01BossCharacter::PruneProjectiles()
{
	ActiveProjectiles.RemoveAll(
		[](const TWeakObjectPtr<Ademo_mapSkillProjectile>& Projectile)
		{
			return !Projectile.IsValid() || Projectile->HasBeenConsumed();
		});
}

void Ademo_mapM01BossCharacter::RefreshPresentation()
{
	const FLinearColor Color(0.72f, 0.04f, 1.0f);
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		BodyMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
	if (BossLight) BossLight->SetLightColor(Color);
	if (Label)
	{
		const TCHAR* StateLabel = State == Edemo_mapM01BossState::Windup
			? (PendingAttack == Edemo_mapM01BossAttack::Sweep
				? TEXT("SWEEP WINDUP")
				: (PendingAttack == Edemo_mapM01BossAttack::Charge
					? TEXT("CHARGE WINDUP") : TEXT("VOLLEY WINDUP")))
			: TEXT("LOCKED");
		Label->SetText(FText::FromString(
			IsDead()
				? TEXT("M01 BOSS MAIN\nDEFEATED")
				: FString::Printf(TEXT("M01 BOSS MAIN\n%d / %d\n%s"),
					GetCurrentHealth(), GetMaxHealth(), StateLabel)));
	}
}

void Ademo_mapM01BossCharacter::ShowDamageFeedback()
{
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
		BodyMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::White);
	}
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			DamageFeedbackTimer,
			this,
			&Ademo_mapM01BossCharacter::ClearDamageFeedback,
			0.20f,
			false);
	}
}

void Ademo_mapM01BossCharacter::ClearDamageFeedback()
{
	if (!IsDead()) RefreshPresentation();
}

void Ademo_mapM01BossCharacter::DrawTelegraph(
	FColor Color,
	float Duration,
	float Thickness) const
{
#if !UE_BUILD_SHIPPING
	if (!GetWorld()) return;
	const FVector Center = GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
	if (PendingAttack == Edemo_mapM01BossAttack::Sweep)
	{
		const float HalfAngle = 59.0f;
		FVector Previous = Center + LockedDirection.RotateAngleAxis(-HalfAngle, FVector::UpVector) * 340.0f;
		DrawDebugLine(GetWorld(), Center, Previous, Color, false, Duration, 0, Thickness);
		for (int32 Segment = 1; Segment <= 24; ++Segment)
		{
			const float Angle = -HalfAngle + (2.0f * HalfAngle * Segment / 24.0f);
			const FVector Current = Center
				+ LockedDirection.RotateAngleAxis(Angle, FVector::UpVector) * 340.0f;
			DrawDebugLine(GetWorld(), Previous, Current, Color, false, Duration, 0, Thickness);
			Previous = Current;
		}
		DrawDebugLine(GetWorld(), Center, Previous, Color, false, Duration, 0, Thickness);
	}
	else if (PendingAttack == Edemo_mapM01BossAttack::Charge)
	{
		DrawDebugDirectionalArrow(GetWorld(), Center, Center + LockedDirection * 620.0f,
			70.0f, Color, false, Duration, 0, Thickness);
	}
	else
	{
		for (const float Angle : { -11.0f, 0.0f, 11.0f })
		{
			DrawDebugLine(GetWorld(), Center,
				Center + LockedDirection.RotateAngleAxis(Angle, FVector::UpVector) * 1050.0f,
				Color, false, Duration, 0, Thickness);
		}
	}
#endif
}

void Ademo_mapM01BossCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WindupTimer);
	GetWorldTimerManager().ClearTimer(RecoveryTimer);
	GetWorldTimerManager().ClearTimer(DestroyTimer);
	GetWorldTimerManager().ClearTimer(DamageFeedbackTimer);
	CancelCombat();
	CombatVitalityLedger.Reset();
	Super::EndPlay(EndPlayReason);
}
