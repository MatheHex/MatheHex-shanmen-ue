#include "demo_mapM01RiskZone.h"

#include "demo_mapGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	FColor M01RiskTextColor(Edemo_mapM01Risk Tier)
	{
		switch (Tier)
		{
		case Edemo_mapM01Risk::Low: return FColor(40, 230, 85);
		case Edemo_mapM01Risk::Mid: return FColor(255, 175, 25);
		case Edemo_mapM01Risk::High: return FColor(245, 40, 50);
		}
		return FColor::White;
	}
}

Ademo_mapM01RiskZone::Ademo_mapM01RiskZone()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("RiskVolume"));
	Volume->SetupAttachment(SceneRoot);
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionObjectType(ECC_WorldDynamic);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Volume->SetGenerateOverlapEvents(true);
	Volume->SetCanEverAffectNavigation(false);
	Volume->OnComponentBeginOverlap.AddDynamic(this, &Ademo_mapM01RiskZone::HandleBeginOverlap);
	Volume->OnComponentEndOverlap.AddDynamic(this, &Ademo_mapM01RiskZone::HandleEndOverlap);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RiskLabel"));
	Label->SetupAttachment(SceneRoot);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(54.0f);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tags.AddUnique(TEXT("M01_GENERATED"));
}

void Ademo_mapM01RiskZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void Ademo_mapM01RiskZone::Configure(FName InStableId, Edemo_mapM01Risk InRiskTier, FVector InExtent)
{
	StableId = InStableId;
	RiskTier = InRiskTier;
	Extent = FVector(FMath::Max(1.0f, InExtent.X), FMath::Max(1.0f, InExtent.Y), FMath::Max(1.0f, InExtent.Z));
	Tags.AddUnique(TEXT("M01_GENERATED"));
	Tags.AddUnique(StableId);
	RefreshPresentation();
}

void Ademo_mapM01RiskZone::ConfigureLow(FVector InExtent) { Configure(TEXT("M01.Risk.LOW"), Edemo_mapM01Risk::Low, InExtent); }
void Ademo_mapM01RiskZone::ConfigureMid(FVector InExtent) { Configure(TEXT("M01.Risk.MID"), Edemo_mapM01Risk::Mid, InExtent); }
void Ademo_mapM01RiskZone::ConfigureHigh(FVector InExtent) { Configure(TEXT("M01.Risk.HIGH"), Edemo_mapM01Risk::High, InExtent); }

FVector Ademo_mapM01RiskZone::GetExtent() const
{
	return Volume ? Volume->GetUnscaledBoxExtent() : Extent;
}

void Ademo_mapM01RiskZone::RefreshPresentation()
{
	if (Volume) Volume->SetBoxExtent(Extent, true);
	if (Label)
	{
		Label->SetText(FText::FromName(StableId));
		Label->SetTextRenderColor(M01RiskTextColor(RiskTier));
	}
}

void Ademo_mapM01RiskZone::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!Cast<APawn>(OtherActor)) return;
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		Mode->SetM01RiskTier(StableId);
	}
}

void Ademo_mapM01RiskZone::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!Cast<APawn>(OtherActor)) return;
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		Mode->ClearM01RiskTier(StableId);
	}
}
