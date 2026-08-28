#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_map.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapFactionComponent.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapGameMode.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapSectorGeometry.h"
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
#include "UObject/ConstructorHelpers.h"

Ademo_mapHeavyEnemyCharacter::Ademo_mapHeavyEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick=false;
	GetCapsuleComponent()->InitCapsuleSize(58.0f,105.0f);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldDynamic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed=MovementSpeed;
	GetCharacterMovement()->bOrientRotationToMovement=true;
	GetCharacterMovement()->RotationRate=FRotator(0,420,0);
	AIControllerClass=AAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;SetCanBeDamaged(true);
	FactionComponent=CreateDefaultSubobject<Udemo_mapFactionComponent>(TEXT("Faction"));FactionComponent->SetFaction(Edemo_mapFaction::Hostile);

	VisibleMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeavyBody"));VisibleMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));if(Cube.Succeeded())VisibleMesh->SetStaticMesh(Cube.Object);
	VisibleMesh->SetRelativeLocation(FVector(0,0,-42));VisibleMesh->SetRelativeScale3D(FVector(1.25f,1.05f,1.65f));VisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoulderMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeavyShoulders"));ShoulderMesh->SetupAttachment(GetCapsuleComponent());if(Cube.Succeeded())ShoulderMesh->SetStaticMesh(Cube.Object);
	ShoulderMesh->SetRelativeLocation(FVector(0,0,32));ShoulderMesh->SetRelativeScale3D(FVector(1.65f,1.25f,0.40f));ShoulderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("HeavyLabel"));Label->SetupAttachment(GetCapsuleComponent());Label->SetRelativeLocation(FVector(0,0,145));Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(38);Label->SetTextRenderColor(FColor(255,115,15));
	EnemyLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("HeavyOrangeLight"));EnemyLight->SetupAttachment(GetCapsuleComponent());EnemyLight->SetLightColor(FLinearColor(1.0f,0.22f,0.01f));EnemyLight->SetIntensity(3000);EnemyLight->SetAttenuationRadius(440);
}

void Ademo_mapHeavyEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(UMaterialInterface* Base=VisibleMesh->GetMaterial(0)){VisibleMaterial=UMaterialInstanceDynamic::Create(Base,this);VisibleMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor(1.0f,0.20f,0.01f));VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor(1.0f,0.20f,0.01f));VisibleMesh->SetMaterial(0,VisibleMaterial);ShoulderMesh->SetMaterial(0,VisibleMaterial);}
	RefreshPresentation();GetWorldTimerManager().SetTimer(AIUpdateTimer,this,&Ademo_mapHeavyEnemyCharacter::UpdateBehavior,AIUpdateInterval,true);
}

APawn* Ademo_mapHeavyEnemyCharacter::GetPlayerPawn() const{APlayerController* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;return PC?PC->GetPawn():nullptr;}
void Ademo_mapHeavyEnemyCharacter::StopMovement(){if(AAIController* AI=Cast<AAIController>(GetController()))AI->StopMovement();}

void Ademo_mapHeavyEnemyCharacter::UpdateBehavior()
{
	if(IsDead()||bCombatSuppressed)return;
	APawn* Player=GetPlayerPawn();const Udemo_mapPlayerHealthComponent* Health=Player?Player->FindComponentByClass<Udemo_mapPlayerHealthComponent>():nullptr;
	if(!Player||(Health&&Health->IsDefeated())){CancelPendingAttack();StopMovement();State=Edemo_mapHeavyEnemyState::Idle;return;}
	if(State==Edemo_mapHeavyEnemyState::Windup){DrawSectorFeedback(FColor(255,70,0),AIUpdateInterval+0.04f,7.0f);return;}
	if(State==Edemo_mapHeavyEnemyState::Resolve||State==Edemo_mapHeavyEnemyState::Recovery)return;
	const float Distance=FVector::Dist2D(GetActorLocation(),Player->GetActorLocation());
	if(Distance>LoseAggroRange){StopMovement();State=Edemo_mapHeavyEnemyState::Idle;return;}
	if(Distance<=AttackRange&&GetWorld()->GetTimeSeconds()>=NextAttackAllowedTime){BeginWindup(Player);return;}
	if(Distance<=AggroRange&&Distance>AttackRange)
	{
		State=Edemo_mapHeavyEnemyState::Chase;const float Now=GetWorld()->GetTimeSeconds();
		if(Now-LastMoveRequestTime>=0.36f){if(AAIController* AI=Cast<AAIController>(GetController()))AI->MoveToActor(Player,AttackRange*0.82f,true,true,true,nullptr,true);LastMoveRequestTime=Now;}
		return;
	}
	StopMovement();State=Edemo_mapHeavyEnemyState::Idle;
}

void Ademo_mapHeavyEnemyCharacter::BeginWindup(APawn* PlayerPawn)
{
	FVector Direction=PlayerPawn->GetActorLocation()-GetActorLocation();Direction.Z=0;if(!Direction.Normalize())return;
	if(NextAttackSequence==0||NextAttackSequence==MAX_uint64)
	{
		if(GetWorld())NextAttackAllowedTime=GetWorld()->GetTimeSeconds()+AttackCooldown;
		UE_LOG(Logdemo_map,Error,TEXT("0_0_10_ENEMY_HEAVY Event=SequenceExhausted Next=%llu"),static_cast<unsigned long long>(NextAttackSequence));
		return;
	}
	ActiveAttackSequence=NextAttackSequence;++NextAttackSequence;
	LockedDirection=Direction;State=Edemo_mapHeavyEnemyState::Windup;StopMovement();GetCharacterMovement()->bOrientRotationToMovement=false;SetActorRotation(LockedDirection.Rotation());
	GetWorldTimerManager().SetTimer(WindupTimer,this,&Ademo_mapHeavyEnemyCharacter::ResolveAttack,WindupDuration,false);DrawSectorFeedback(FColor(255,70,0),AIUpdateInterval+0.04f,7.0f);UE_LOG(Logdemo_map,Log,TEXT("V2D: heavy telegraph started."));
}

bool Ademo_mapHeavyEnemyCharacter::HasWorldStaticLineOfSight(const APawn* PlayerPawn) const
{
	if(!GetWorld()||!PlayerPawn)return false;FHitResult Hit;FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);FCollisionQueryParams Params(SCENE_QUERY_STAT(V2DHeavySight),false,this);Params.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByObjectType(Hit,GetActorLocation()+FVector(0,0,55),PlayerPawn->GetActorLocation()+FVector(0,0,45),Objects,Params);
}

void Ademo_mapHeavyEnemyCharacter::ResolveAttack()
{
	if(IsDead()||bCombatSuppressed||State!=Edemo_mapHeavyEnemyState::Windup)return;
	State=Edemo_mapHeavyEnemyState::Resolve;++ResolveCount;APawn* Player=GetPlayerPawn();
	DrawSectorFeedback(FColor::Yellow,0.22f,12.0f);
	Fdemo_mapTargetFilter Filter;
	const bool bInside=Player&&Fdemo_mapSectorGeometry::IsInsideSectorXY(GetActorLocation(),LockedDirection,Player->GetActorLocation(),SectorRadius,FullAngleDegrees,VerticalTolerance);
	if(bInside&&HasWorldStaticLineOfSight(Player)&&Fdemo_mapCombatTargeting::CanAffect(this,Player,Filter))
	{
		float AppliedDamage=0.0f;bool bUsedCanonicalProduct=false;
		if(Ademo_mapGameMode* GameMode=GetWorld()?GetWorld()->GetAuthGameMode<Ademo_mapGameMode>():nullptr;GameMode&&GameMode->ShouldUseM01EnemyAttackProductPath())
		{
			bUsedCanonicalProduct=true;
			const Fdemo_mapM01EnemyAttackExecutionResult Product=GameMode->ExecuteM01EnemyHeavySectorAttack(this,Player,ActiveAttackSequence,AttackDamage);
			AppliedDamage=Product.GetNewlyCommittedDamage();
		}
		else
		{
			AppliedDamage=UGameplayStatics::ApplyDamage(Player,AttackDamage,GetController(),this,nullptr);
		}
		UE_LOG(Logdemo_map,Log,TEXT("0_0_10_ENEMY_HEAVY Event=SectorActorRoute Canonical=%d Sequence=%llu Applied=%.3f"),bUsedCanonicalProduct?1:0,static_cast<unsigned long long>(ActiveAttackSequence),AppliedDamage);
	}
	ActiveAttackSequence=0;NextAttackAllowedTime=GetWorld()->GetTimeSeconds()+AttackCooldown;State=Edemo_mapHeavyEnemyState::Recovery;GetWorldTimerManager().SetTimer(RecoveryTimer,this,&Ademo_mapHeavyEnemyCharacter::FinishRecovery,RecoveryDuration,false);
}

void Ademo_mapHeavyEnemyCharacter::FinishRecovery(){ActiveAttackSequence=0;if(!IsDead()&&!bCombatSuppressed){State=Edemo_mapHeavyEnemyState::Idle;GetCharacterMovement()->bOrientRotationToMovement=true;}}
void Ademo_mapHeavyEnemyCharacter::CancelPendingAttack(){if(GetWorld()){GetWorldTimerManager().ClearTimer(WindupTimer);GetWorldTimerManager().ClearTimer(RecoveryTimer);}ActiveAttackSequence=0;if(State==Edemo_mapHeavyEnemyState::Windup||State==Edemo_mapHeavyEnemyState::Resolve||State==Edemo_mapHeavyEnemyState::Recovery)State=Edemo_mapHeavyEnemyState::Idle;GetCharacterMovement()->bOrientRotationToMovement=true;}
void Ademo_mapHeavyEnemyCharacter::SetCombatSuppressed(bool bSuppressed){bCombatSuppressed=bSuppressed;if(bSuppressed){CancelPendingAttack();StopMovement();}}
void Ademo_mapHeavyEnemyCharacter::ResetHeavyAttackForNewRun(){CancelPendingAttack();NextAttackAllowedTime=0.0f;NextAttackSequence=1;ActiveAttackSequence=0;}

bool Ademo_mapHeavyEnemyCharacter::ConfigureEncounter(
	const Fdemo_mapEnemyEncounterIdentity& InIdentity,
	const Fdemo_mapEnemyCombatTuning& InTuning)
{
	if(!InIdentity.IsValid()||!InIdentity.SkillProfileId.IsNone()||!InTuning.IsValid())return false;
	const float ConfiguredVitality=static_cast<float>(InTuning.MaxHealth);if(!TryCommitVitalityState(ConfiguredVitality,ConfiguredVitality))return false;EncounterIdentity=InIdentity;MovementSpeed=InTuning.MovementSpeed;AttackDamage=InTuning.AttackDamage;WindupDuration=InTuning.AttackWindup;AttackCooldown=InTuning.AttackCooldown;NextAttackSequence=1;ActiveAttackSequence=0;GetCharacterMovement()->MaxWalkSpeed=MovementSpeed;RefreshPresentation();return true;
}

void Ademo_mapHeavyEnemyCharacter::DrawSectorFeedback(const FColor& Color,float Duration,float Thickness) const
{
#if !UE_BUILD_SHIPPING
	const FVector Center=GetActorLocation()+FVector(0,0,18);const float Half=FullAngleDegrees*0.5f;const FVector Left=LockedDirection.RotateAngleAxis(-Half,FVector::UpVector);const FVector Right=LockedDirection.RotateAngleAxis(Half,FVector::UpVector);
	DrawDebugLine(GetWorld(),Center,Center+Left*SectorRadius,Color,false,Duration,0,Thickness);DrawDebugLine(GetWorld(),Center,Center+Right*SectorRadius,Color,false,Duration,0,Thickness);
	FVector Previous=Center+Left*SectorRadius;for(int32 Segment=1;Segment<=24;++Segment){const float Angle=-Half+FullAngleDegrees*(static_cast<float>(Segment)/24.0f);const FVector Current=Center+LockedDirection.RotateAngleAxis(Angle,FVector::UpVector)*SectorRadius;DrawDebugLine(GetWorld(),Previous,Current,Color,false,Duration,0,Thickness);Previous=Current;}
#endif
}

bool Ademo_mapHeavyEnemyCharacter::TryBindCombatEntity(
	const FGuid& TargetEntityId)
{
	if(!TargetEntityId.IsValid())return false;
	if(CombatVitalityLedger.IsValid())return CombatVitalityLedger.GetTargetEntityId()==TargetEntityId&&CombatVitalityLedger.IsSynchronized(CurrentVitality,MaximumVitality);
	FShanmenVitalityCommitLedger NewLedger;if(!FShanmenVitalityCommitLedger::TryCreate(TargetEntityId,CurrentVitality,MaximumVitality,0,NewLedger))return false;CombatVitalityLedger=MoveTemp(NewLedger);return true;
}

bool Ademo_mapHeavyEnemyCharacter::TryEndCombatEntityBinding(
	const FGuid& ExpectedTargetEntityId)
{
	if(!ExpectedTargetEntityId.IsValid())return false;if(!CombatVitalityLedger.IsValid())return true;if(CombatVitalityLedger.GetTargetEntityId()!=ExpectedTargetEntityId)return false;CombatVitalityLedger.Reset();return true;
}

bool Ademo_mapHeavyEnemyCharacter::TryCaptureCombatVitalitySnapshot(
	FShanmenTargetVitalitySnapshot& OutSnapshot) const
{
	return CombatVitalityLedger.TryCaptureSnapshot(CurrentVitality,MaximumVitality,OutSnapshot);
}

FShanmenVitalityCommitResult Ademo_mapHeavyEnemyCharacter::CommitCombatImpact(
	const FShanmenVitalityCommitCommand& Command)
{
	FShanmenVitalityCommitResult Result=CombatVitalityLedger.Commit(Command,CurrentVitality,MaximumVitality);if(Result.Status==EShanmenVitalityCommitStatus::Committed&&Result.Receipt.GetAppliedDamage()>0.0f)PublishAppliedDamage(Result.Receipt.GetAppliedDamage());return Result;
}

bool Ademo_mapHeavyEnemyCharacter::TryCommitVitalityState(
	float NewCurrentVitality,
	float NewMaximumVitality)
{
	if(!FMath::IsFinite(NewCurrentVitality)||!FMath::IsFinite(NewMaximumVitality)||NewMaximumVitality<=0.0f||NewCurrentVitality<0.0f||NewCurrentVitality>NewMaximumVitality)return false;if(CombatVitalityLedger.IsValid())return CombatVitalityLedger.TryCommitExternalMutation(CurrentVitality,MaximumVitality,NewCurrentVitality,NewMaximumVitality);CurrentVitality=NewCurrentVitality;MaximumVitality=NewMaximumVitality;return true;
}

float Ademo_mapHeavyEnemyCharacter::TakeDamage(float DamageAmount,FDamageEvent const&,AController*,AActor*)
{
	if(IsDead()||!FMath::IsFinite(DamageAmount)||DamageAmount<=0.0f)return 0.0f;const float RequestedDamage=static_cast<float>(FMath::Max(0,FMath::FloorToInt(DamageAmount)));const float AppliedDamage=FMath::Min(CurrentVitality,RequestedDamage);if(AppliedDamage<=0.0f||!TryCommitVitalityState(FMath::Max(0.0f,CurrentVitality-AppliedDamage),MaximumVitality))return 0.0f;PublishAppliedDamage(AppliedDamage);return AppliedDamage;
}

void Ademo_mapHeavyEnemyCharacter::PublishAppliedDamage(float AppliedDamage)
{
	if(!FMath::IsFinite(AppliedDamage)||AppliedDamage<=0.0f)return;
#if WITH_DEV_AUTOMATION_TESTS
	++PositiveCombatDamageCount;
#endif
	RefreshPresentation();ShowDamageFeedback();UE_LOG(Logdemo_map,Log,TEXT("0.0.10 P4.6: heavy enemy damaged; vitality=%.3f/%.3f."),CurrentVitality,MaximumVitality);if(CurrentVitality<=0.0f)EnterDeadState();
}

void Ademo_mapHeavyEnemyCharacter::EnterDeadState()
{
	if(Ademo_mapGameMode* Mode=GetWorld()?Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()):nullptr)
	{
		if(Ademo_mapV3ProgressionManager* V3=Mode->GetV3ProgressionManager())
		{
			if(const Udemo_mapM01EnemyIdentityComponent* M01=FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();M01&&M01->IsConfigured())M01->ProjectCorpse(V3,LootSourceId,GetActorLocation(),this);
			else if(!EncounterIdentity.LootTableId.IsNone())V3->HandleEnemyDeath(EncounterIdentity.LootTableId,LootSourceId,GetActorLocation(),this);
			else V3->HandleEnemyDeath(Edemo_mapEnemyLootArchetype::Heavy,LootSourceId,GetActorLocation(),this);
		}
	}
	State=Edemo_mapHeavyEnemyState::Dead;SetCanBeDamaged(false);SetActorEnableCollision(false);GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetCharacterMovement()->DisableMovement();StopMovement();CancelPendingAttack();GetWorldTimerManager().ClearTimer(AIUpdateTimer);RefreshPresentation();if(VisibleMaterial){VisibleMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor(0.10f,0.04f,0.01f));VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor(0.10f,0.04f,0.01f));}GetWorldTimerManager().SetTimer(DestroyTimer,this,&Ademo_mapHeavyEnemyCharacter::DestroyAfterDeath,1.0f,false);UE_LOG(Logdemo_map,Log,TEXT("V2D: heavy enemy died."));
}
void Ademo_mapHeavyEnemyCharacter::DestroyAfterDeath(){Destroy();}
void Ademo_mapHeavyEnemyCharacter::RefreshPresentation(){FString Name=TEXT("HEAVY");FLinearColor Color(1.0f,0.20f,0.01f);if(const Udemo_mapM01EnemyIdentityComponent* M01=FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();M01&&M01->IsConfigured()){const bool bElite=M01->GetDefinition().Archetype==Edemo_mapM01EnemyArchetype::EliteBulwark;Name=bElite?TEXT("M01 ELITE BULWARK"):TEXT("M01 BRUISER");Color=bElite?FLinearColor(1.0f,0.70f,0.04f):FLinearColor(1.0f,0.28f,0.03f);if(VisibleMesh)VisibleMesh->SetRelativeScale3D(bElite?FVector(1.55f,1.35f,1.95f):FVector(1.20f,1.05f,1.65f));if(ShoulderMesh)ShoulderMesh->SetRelativeScale3D(bElite?FVector(2.05f,1.55f,0.52f):FVector(1.65f,1.25f,0.40f));if(Label)Label->SetTextRenderColor(bElite?FColor(255,205,45):FColor(255,115,15));}if(VisibleMaterial){VisibleMaterial->SetVectorParameterValue(TEXT("Color"),Color);VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),Color);}if(EnemyLight)EnemyLight->SetLightColor(Color);if(Label)Label->SetText(FText::FromString(IsDead()?FString::Printf(TEXT("%s\nDEFEATED"),*Name):FString::Printf(TEXT("%s\n%d / %d"),*Name,GetCurrentHealth(),GetMaxHealth())));}
void Ademo_mapHeavyEnemyCharacter::ShowDamageFeedback(){if(VisibleMaterial){VisibleMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor::White);VisibleMaterial->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor::White);}if(EnemyLight)EnemyLight->SetLightColor(FLinearColor(1.0f,0.85f,0.20f));if(GetWorld())GetWorldTimerManager().SetTimer(DamageFeedbackTimer,this,&Ademo_mapHeavyEnemyCharacter::ClearDamageFeedback,0.20f,false);}
void Ademo_mapHeavyEnemyCharacter::ClearDamageFeedback(){if(IsDead())return;RefreshPresentation();}
void Ademo_mapHeavyEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason){GetWorldTimerManager().ClearTimer(AIUpdateTimer);GetWorldTimerManager().ClearTimer(WindupTimer);GetWorldTimerManager().ClearTimer(RecoveryTimer);GetWorldTimerManager().ClearTimer(DestroyTimer);GetWorldTimerManager().ClearTimer(DamageFeedbackTimer);CombatVitalityLedger.Reset();Super::EndPlay(EndPlayReason);}
