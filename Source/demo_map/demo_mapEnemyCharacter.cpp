#include "demo_mapEnemyCharacter.h"
#include "demo_map.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapGameMode.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapFactionComponent.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapEnemySkillRuntimeComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapKnockbackComponent.h"
#include "demo_mapCombatDisplacement.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapEnemyCharacter::Ademo_mapEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldDynamic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	SetCanBeDamaged(true);
	FactionComponent = CreateDefaultSubobject<Udemo_mapFactionComponent>(TEXT("Faction"));
	FactionComponent->SetFaction(Edemo_mapFaction::Hostile);
	EnemySkillRuntime =
		CreateDefaultSubobject<Udemo_mapEnemySkillRuntimeComponent>(
			TEXT("EnemySkillRuntime"));

	VisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyMesh"));
	VisibleMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisibleMesh->SetStaticMesh(CylinderMesh.Object);
	}
	VisibleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -38.0f));
	VisibleMesh->SetRelativeScale3D(FVector(0.85f, 0.85f, 1.7f));
	VisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisibleMaterial = VisibleMesh->CreateDynamicMaterialInstance(0);
	if (VisibleMaterial != nullptr)
	{
		VisibleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.02f, 0.02f));
		VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.02f, 0.02f));
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EnemyLabel"));
	Label->SetupAttachment(GetCapsuleComponent());
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	Label->SetText(FText::FromString(TEXT("MELEE\n3 / 3")));
	Label->SetTextRenderColor(FColor::Red);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(36.0f);

	EnemyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EnemyRedLight"));
	EnemyLight->SetupAttachment(GetCapsuleComponent());
	EnemyLight->SetLightColor(FLinearColor::Red);
	EnemyLight->SetIntensity(2600.0f);
	EnemyLight->SetAttenuationRadius(360.0f);
}

void Ademo_mapEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (EnemySkillRuntime)
	{
		EnemySkillRuntime->OnDisplacementSegment.AddUObject(
			this,
			&Ademo_mapEnemyCharacter::HandleDashSegment);
	}
}

void Ademo_mapEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateBehavior();
}

void Ademo_mapEnemyCharacter::UpdateBehavior()
{
	if (IsDead() || bCombatSuppressed)
	{
		return;
	}

	APawn* PlayerPawn = GetPlayerPawn();
	if (PlayerPawn == nullptr)
	{
		EnemyState = Edemo_mapEnemyState::Idle;
		return;
	}
	if (const Udemo_mapPlayerHealthComponent* Health = PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>(); Health != nullptr && Health->IsDefeated())
	{
		EnemyState = Edemo_mapEnemyState::Idle;
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		return;
	}

	const float Distance = FVector::Dist2D(GetActorLocation(), PlayerPawn->GetActorLocation());
	if (EnemySkillRuntime && EnemySkillRuntime->IsActive())
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		return;
	}
	if (Distance > LeashRange)
	{
		EnemyState = Edemo_mapEnemyState::Idle;
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		return;
	}
	if (TryBeginDash(PlayerPawn, Distance))
	{
		EnemyState = Edemo_mapEnemyState::Chase;
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		return;
	}
	if (Distance <= AttackRange)
	{
		EnemyState = Edemo_mapEnemyState::Attack;
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		AttackPlayer(PlayerPawn);
		return;
	}
	if (Distance <= AggroRange)
	{
		EnemyState = Edemo_mapEnemyState::Chase;
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastMoveRequestTime >= 0.25f)
		{
			if (AAIController* AIController = Cast<AAIController>(GetController()))
			{
				AIController->MoveToActor(PlayerPawn, AttackRange * 0.75f, true, true, true, nullptr, true);
			}
			LastMoveRequestTime = CurrentTime;
		}
		return;
	}

	EnemyState = Edemo_mapEnemyState::Idle;
}

bool Ademo_mapEnemyCharacter::TryBeginDash(
	APawn* PlayerPawn,
	float Distance)
{
	if (!EnemySkillRuntime || !EnemySkillRuntime->IsReady() || !PlayerPawn)
	{
		return false;
	}
	Fdemo_mapTargetFilter Filter;
	if (!Fdemo_mapCombatTargeting::CanAffect(this, PlayerPawn, Filter))
	{
		return false;
	}
	const Fdemo_mapEnemySkillDefinition* SkillDefinition =
		Fdemo_mapEnemySkillPrototypeConfig::FindProfile(SkillProfileId);
	if (!SkillDefinition
		|| SkillDefinition->Kind != Edemo_mapEnemySkillKind::MeleeDash)
	{
		return false;
	}
	Fdemo_mapEnemySkillActivationIntent Intent;
	Intent.Definition = *SkillDefinition;
	Intent.Target = PlayerPawn;
	Intent.InitialPlanarDirection =
		PlayerPawn->GetActorLocation() - GetActorLocation();
	Intent.CurrentTargetDistance = Distance;
	const Fdemo_mapEnemySkillStartResult Result =
		EnemySkillRuntime->TryActivate(Intent);
	if (Result.IsAccepted())
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P6_MELEE_DASH: accepted preflight=%.2f."),
			Result.ResolvedPreflightDistance);
		return true;
	}
	return false;
}

void Ademo_mapEnemyCharacter::HandleDashSegment(
	const Fdemo_mapEnemySkillDisplacementSegment& Segment)
{
	if (!EnemySkillRuntime
		|| Segment.Kind != Edemo_mapEnemySkillKind::MeleeDash
		|| EnemySkillRuntime->GetSnapshot().bFirstLegalHitConsumed
		|| !GetWorld())
	{
		return;
	}
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}
	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams Objects(
		FCollisionObjectQueryParams::AllObjects);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(P6MeleeDashHit), false, this);
	Params.AddIgnoredActor(this);
	GetWorld()->SweepMultiByObjectType(
		Hits,
		Segment.Start,
		Segment.End,
		FQuat::Identity,
		Objects,
		FCollisionShape::MakeCapsule(
			Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleHalfHeight()),
		Params);
	Hits.Sort(
		[](const FHitResult& Left, const FHitResult& Right)
		{
			return Left.Time < Right.Time;
		});
	APawn* ExpectedPlayer = GetPlayerPawn();
	TArray<AActor*> OrderedCandidates;
	if (Segment.BlockingHit.GetActor())
	{
		OrderedCandidates.AddUnique(Segment.BlockingHit.GetActor());
	}
	for (const FHitResult& Hit : Hits)
	{
		if (Hit.GetActor())
		{
			OrderedCandidates.AddUnique(Hit.GetActor());
		}
	}
	for (AActor* Candidate : OrderedCandidates)
	{
		ACharacter* HitCharacter = Cast<ACharacter>(Candidate);
		if (!HitCharacter || HitCharacter != ExpectedPlayer)
		{
			continue;
		}
		Fdemo_mapTargetFilter Filter;
		if (!Fdemo_mapCombatTargeting::CanAffect(this, HitCharacter, Filter))
		{
			continue;
		}
		Udemo_mapPlayerHealthComponent* Health =
			HitCharacter->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
		if (!Health || Health->IsDefeated())
		{
			continue;
		}
		const Fdemo_mapEnemySkillRuntimeSnapshot RuntimeSnapshot =
			EnemySkillRuntime->GetSnapshot();
		float AppliedDamage = 0.0f;
		bool bTargetDefeatedAfterDamage = false;
		bool bUsedCanonicalProduct = false;
		if (Ademo_mapGameMode* GameMode =
			GetWorld()->GetAuthGameMode<Ademo_mapGameMode>();
			GameMode && GameMode->ShouldUseM01EnemyAttackProductPath())
		{
			bUsedCanonicalProduct = true;
			const Fdemo_mapM01EnemyAttackExecutionResult Product =
				GameMode->ExecuteM01EnemyMeleeDashContact(
					this,
					HitCharacter,
					SkillProfileId,
					RuntimeSnapshot.ActivationSerial,
					AttackDamage);
			AppliedDamage = Product.GetNewlyCommittedDamage();
			bTargetDefeatedAfterDamage =
				Product.DidNewCommitDefeatTarget();
		}
		else
		{
			AppliedDamage = static_cast<float>(
				Health->ApplyIncomingDamage(AttackDamage));
			bTargetDefeatedAfterDamage = Health->IsDefeated();
		}
		LastAttackTime = GetWorld()->GetTimeSeconds();
		EnemySkillRuntime->MarkFirstLegalHitAndRecover();
		if (ShouldRequestEnemySkillKnockback(
			AppliedDamage,
			bTargetDefeatedAfterDamage))
		{
			if (Udemo_mapKnockbackComponent* Knockback =
				Udemo_mapKnockbackComponent::FindOrCreate(HitCharacter))
			{
				Fdemo_mapKnockbackIntent Intent;
				const Fdemo_mapEnemySkillPrototypeConfig& Config =
					Fdemo_mapEnemySkillPrototypeConfig::Get();
				Fdemo_mapCombatDisplacement::
					ResolvePlanarDirectionWithFallback(
						HitCharacter->GetActorLocation()
							- GetActorLocation(),
						EnemySkillRuntime->GetSnapshot()
							.LockedPlanarDirection,
						Intent.PlanarDirection);
				Intent.Distance = Config.KnockbackDistance;
				Intent.Duration = Config.KnockbackDuration;
				Knockback->TryStart(Intent);
			}
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P6_MELEE_DASH: first legal hit consumed; applied=%.3f."),
			AppliedDamage);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("0_0_10_ENEMY_MELEE Event=DashActorRoute Canonical=%d ActivationSerial=%u Applied=%.3f"),
			bUsedCanonicalProduct ? 1 : 0,
			RuntimeSnapshot.ActivationSerial,
			AppliedDamage);
		return;
	}
}

void Ademo_mapEnemyCharacter::AttackPlayer(APawn* PlayerPawn)
{
	Fdemo_mapTargetFilter EnemyFilter;
	if (!Fdemo_mapCombatTargeting::CanAffect(this, PlayerPawn, EnemyFilter))
	{
		return;
	}
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackCooldown)
	{
		return;
	}

	LastAttackTime = CurrentTime;
	if (VisibleMaterial != nullptr)
	{
		VisibleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Yellow);
		VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Yellow);
	}
	if (EnemyLight != nullptr) EnemyLight->SetLightColor(FLinearColor::Yellow);
	GetWorldTimerManager().SetTimer(AttackFeedbackTimer, this, &Ademo_mapEnemyCharacter::ClearAttackFeedback, 0.18f, false);
	if (Ademo_mapGameMode* GameMode =
		GetWorld()->GetAuthGameMode<Ademo_mapGameMode>();
		GameMode && GameMode->ShouldUseM01EnemyAttackProductPath())
	{
		const Fdemo_mapM01EnemyAttackExecutionResult Product =
			GameMode->ExecuteM01EnemyBasicMeleeStrike(
				this,
				PlayerPawn,
				AttackDamage);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("0_0_10_ENEMY_MELEE Event=ActorRoute Executed=%d Error=%d"),
			Product.IsExecuted() ? 1 : 0,
			static_cast<int32>(Product.Error));
		return;
	}
	UGameplayStatics::ApplyDamage(PlayerPawn, AttackDamage, GetController(), this, nullptr);
	UE_LOG(Logdemo_map, Log, TEXT("T7: enemy attack applied; damage=%.2f cooldown=%.2f."), AttackDamage, AttackCooldown);
	UE_LOG(Logdemo_map, Log, TEXT("T7R: enemy attack applied damage."));
}

void Ademo_mapEnemyCharacter::ClearAttackFeedback()
{
	if (!IsDead()) RefreshPresentation();
}

void Ademo_mapEnemyCharacter::SetCombatSuppressed(bool bSuppressed)
{
	bCombatSuppressed = bSuppressed;
	if (bCombatSuppressed)
	{
		if (EnemySkillRuntime)
		{
			EnemySkillRuntime->Cancel(true);
		}
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		ClearAttackFeedback();
	}
}

void Ademo_mapEnemyCharacter::ResetEnemySkillForNewRun()
{
	if (EnemySkillRuntime)
	{
		EnemySkillRuntime->ResetForNewRun();
	}
	LastAttackTime = -1000.0f;
	LastMoveRequestTime = -1000.0f;
	ClearAttackFeedback();
}

bool Ademo_mapEnemyCharacter::ConfigureEncounter(
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
		|| SkillDefinition->Kind != Edemo_mapEnemySkillKind::MeleeDash)
	{
		return false;
	}
	const float ConfiguredVitality =
		static_cast<float>(InTuning.MaxHealth);
	if (!TryCommitVitalityState(ConfiguredVitality, ConfiguredVitality))
	{
		return false;
	}
	EncounterIdentity = InIdentity;
	SkillProfileId = InIdentity.SkillProfileId;
	MovementSpeed = InTuning.MovementSpeed;
	AttackDamage = InTuning.AttackDamage;
	AttackCooldown = InTuning.AttackCooldown;
	bEnhancedEncounter = bInEnhanced;
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	RefreshPresentation();
	return true;
}

bool Ademo_mapEnemyCharacter::TryBindCombatEntity(
	const FGuid& TargetEntityId)
{
	if (!TargetEntityId.IsValid())
	{
		return false;
	}
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

bool Ademo_mapEnemyCharacter::TryEndCombatEntityBinding(
	const FGuid& ExpectedTargetEntityId)
{
	if (!ExpectedTargetEntityId.IsValid())
	{
		return false;
	}
	if (!CombatVitalityLedger.IsValid())
	{
		return true;
	}
	if (CombatVitalityLedger.GetTargetEntityId() != ExpectedTargetEntityId)
	{
		return false;
	}
	CombatVitalityLedger.Reset();
	return true;
}

bool Ademo_mapEnemyCharacter::TryCaptureCombatVitalitySnapshot(
	FShanmenTargetVitalitySnapshot& OutSnapshot) const
{
	return CombatVitalityLedger.TryCaptureSnapshot(
		CurrentVitality,
		MaximumVitality,
		OutSnapshot);
}

FShanmenVitalityCommitResult Ademo_mapEnemyCharacter::CommitCombatImpact(
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

bool Ademo_mapEnemyCharacter::TryCommitVitalityState(
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

float Ademo_mapEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsDead() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float RequestedDamage =
		static_cast<float>(FMath::Max(0, FMath::FloorToInt(DamageAmount)));
	const float AppliedDamage = FMath::Min(CurrentVitality, RequestedDamage);
	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	if (!TryCommitVitalityState(
		FMath::Max(0.0f, CurrentVitality - AppliedDamage),
		MaximumVitality))
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("0.0.10 P4.4: rejected desynchronized legacy enemy damage mutation."));
		return 0.0f;
	}
	PublishAppliedDamage(AppliedDamage);
	return AppliedDamage;
}

void Ademo_mapEnemyCharacter::PublishAppliedDamage(float AppliedDamage)
{
	if (!FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f)
	{
		return;
	}
#if WITH_DEV_AUTOMATION_TESTS
	++PositiveCombatDamageCount;
#endif
	RefreshPresentation();
	ShowDamageFeedback();
	UE_LOG(Logdemo_map, Log,
		TEXT("0.0.10 P4.4: melee enemy damaged; vitality=%.3f/%.3f."),
		CurrentVitality,
		MaximumVitality);
	if (CurrentVitality <= 0.0f)
	{
		EnterDeadState();
	}
}

void Ademo_mapEnemyCharacter::EnterDeadState()
{
	if (EnemySkillRuntime)
	{
		EnemySkillRuntime->Cancel(true);
	}
	EnemyState = Edemo_mapEnemyState::Dead;
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (Ademo_mapV3ProgressionManager* V3 = Mode->GetV3ProgressionManager())
		{
			if (const Udemo_mapM01EnemyIdentityComponent* M01 =
				FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();
				M01 && M01->IsConfigured())
			{
				M01->ProjectCorpse(V3, LootSourceId, GetActorLocation(), this);
			}
			else if (!EncounterIdentity.LootTableId.IsNone())
			{
				V3->HandleEnemyDeath(
					EncounterIdentity.LootTableId,
					LootSourceId,
					GetActorLocation(),
					this);
			}
			else
			{
				V3->HandleEnemyDeath(
					Edemo_mapEnemyLootArchetype::Melee,
					LootSourceId,
					GetActorLocation(),
					this);
			}
		}
	}
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	if (Label != nullptr)
	{
		RefreshPresentation();
	}
	if (VisibleMaterial != nullptr)
	{
		VisibleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.02f, 0.02f));
		VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.08f, 0.02f, 0.02f));
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7: enemy died."));
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &Ademo_mapEnemyCharacter::DestroyAfterDeath, 1.0f, false);
}

void Ademo_mapEnemyCharacter::DestroyAfterDeath()
{
	Destroy();
}

APawn* Ademo_mapEnemyCharacter::GetPlayerPawn() const
{
	APlayerController* PlayerController = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
}

void Ademo_mapEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CombatVitalityLedger.Reset();
	if (EnemySkillRuntime)
	{
		EnemySkillRuntime->Cancel(true);
	}
	GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackFeedbackTimer);
	GetWorldTimerManager().ClearTimer(DamageFeedbackTimer);
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapEnemyCharacter::ShowDamageFeedback()
{
	if (VisibleMaterial != nullptr)
	{
		VisibleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
		VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::White);
	}
	if (EnemyLight != nullptr) EnemyLight->SetLightColor(FLinearColor(1.0f, 0.8f, 0.15f));
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			DamageFeedbackTimer,
			this,
			&Ademo_mapEnemyCharacter::ClearDamageFeedback,
			0.20f,
			false);
	}
}

void Ademo_mapEnemyCharacter::ClearDamageFeedback()
{
	if (IsDead()) return;
	RefreshPresentation();
}

void Ademo_mapEnemyCharacter::RefreshPresentation()
{
	FString Name = bEnhancedEncounter ? TEXT("ENHANCED MELEE") : TEXT("MELEE");
	FLinearColor Color(1.0f, 0.02f, 0.02f);
	FVector Scale(0.85f, 0.85f, 1.7f);
	FColor TextColor = FColor::Red;
	if (const Udemo_mapM01EnemyIdentityComponent* M01 =
		FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();
		M01 && M01->IsConfigured())
	{
		if (M01->GetDefinition().Archetype ==
			Edemo_mapM01EnemyArchetype::EliteStalker)
		{
			Name = TEXT("M01 ELITE STALKER");
			Color = FLinearColor(0.72f, 0.06f, 1.0f);
			Scale = FVector(0.72f, 1.02f, 1.85f);
			TextColor = FColor(225, 80, 255);
		}
		else
		{
			Name = TEXT("M01 SKIRMISHER");
			Color = FLinearColor(0.95f, 0.08f, 0.18f);
			Scale = FVector(0.68f, 0.68f, 1.45f);
			TextColor = FColor(255, 70, 80);
		}
	}
	if (VisibleMesh) VisibleMesh->SetRelativeScale3D(Scale);
	if (VisibleMaterial)
	{
		VisibleMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
	if (EnemyLight) EnemyLight->SetLightColor(Color);
	if (Label != nullptr)
	{
		Label->SetTextRenderColor(TextColor);
		Label->SetText(FText::FromString(
			IsDead()
				? FString::Printf(TEXT("%s\nDEFEATED"), *Name)
				: FString::Printf(
					TEXT("%s\n%d / %d"),
					*Name,
					GetCurrentHealth(),
					GetMaxHealth())));
	}
}
