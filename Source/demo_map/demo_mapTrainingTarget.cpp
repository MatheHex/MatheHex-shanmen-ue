#include "demo_mapTrainingTarget.h"
#include "demo_map.h"
#include "demo_mapFactionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapTrainingTarget::Ademo_mapTrainingTarget()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(true);
	FactionComponent = CreateDefaultSubobject<Udemo_mapFactionComponent>(TEXT("Faction"));
	FactionComponent->SetFaction(Edemo_mapFaction::Hostile);

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainingTargetMesh"));
	SetRootComponent(TargetMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		TargetMesh->SetStaticMesh(CubeMesh.Object);
	}

	TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetMesh->SetCollisionObjectType(ECC_WorldDynamic);
	TargetMesh->SetCollisionResponseToAllChannels(ECR_Block);
	TargetMesh->SetGenerateOverlapEvents(true);
	TargetMesh->SetCanEverAffectNavigation(false);
	ProjectileContact = CreateDefaultSubobject<UBoxComponent>(TEXT("ProjectileContact"));
	ProjectileContact->SetupAttachment(TargetMesh);
	ProjectileContact->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	ProjectileContact->SetBoxExtent(FVector(50.0f, 50.0f, 75.0f));
	ProjectileContact->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileContact->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileContact->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileContact->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	ProjectileContact->SetGenerateOverlapEvents(true);
	ProjectileContact->SetCanEverAffectNavigation(false);
	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TargetStatus"));
	StatusText->SetupAttachment(TargetMesh);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(34.0f);
	StatusText->SetTextRenderColor(FColor::Yellow);
	StatusText->SetText(FText::FromString(TEXT("TARGET\n2 / 2")));
}

void Ademo_mapTrainingTarget::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Base = TargetMesh->GetMaterial(0))
	{
		TargetMaterial = UMaterialInstanceDynamic::Create(Base, this);
		TargetMesh->SetMaterial(0, TargetMaterial);
	}
	RefreshPresentation();
}

float Ademo_mapTrainingTarget::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDestroyedByDamage || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const int32 AppliedDamage = FMath::Min(Health, FMath::Max(0, FMath::FloorToInt(DamageAmount)));
	if (AppliedDamage == 0)
	{
		return 0.0f;
	}

	Health -= AppliedDamage;
	RefreshPresentation();
	if (TargetMaterial != nullptr)
	{
		TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
		TargetMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::White);
		GetWorldTimerManager().SetTimer(DamageFeedbackTimer, this, &Ademo_mapTrainingTarget::ClearDamageFeedback, 0.20f, false);
	}
	UE_LOG(Logdemo_map, Log, TEXT("T5: TrainingTarget damage=%d health=%d"), AppliedDamage, Health);

	if (Health <= 0)
	{
		bDestroyedByDamage = true;
		SetCanBeDamaged(false);
		TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileContact->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (TargetMaterial != nullptr)
		{
			TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.08f, 0.08f));
			TargetMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.08f, 0.08f, 0.08f));
		}
		RefreshPresentation();
		UE_LOG(Logdemo_map, Log, TEXT("T5: TrainingTarget destroyed."));
		GetWorldTimerManager().SetTimer(DeathTimer, this, &Ademo_mapTrainingTarget::FinishDeath, 0.18f, false);
	}

	return static_cast<float>(AppliedDamage);
}

void Ademo_mapTrainingTarget::RefreshPresentation()
{
	if (StatusText != nullptr) StatusText->SetText(FText::FromString(bDestroyedByDamage ? TEXT("TARGET\nDEFEATED") : FString::Printf(TEXT("TARGET\n%d / 2"), Health)));
}

void Ademo_mapTrainingTarget::ClearDamageFeedback()
{
	if (TargetMaterial != nullptr && !bDestroyedByDamage)
	{
		TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.65f, 0.02f));
		TargetMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.65f, 0.02f));
	}
}

void Ademo_mapTrainingTarget::FinishDeath() { Destroy(); }

void Ademo_mapTrainingTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageFeedbackTimer);
	GetWorldTimerManager().ClearTimer(DeathTimer);
	Super::EndPlay(EndPlayReason);
}
