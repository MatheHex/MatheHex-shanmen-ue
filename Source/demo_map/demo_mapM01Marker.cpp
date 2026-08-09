#include "demo_mapM01Marker.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FLinearColor M01RiskColor(Edemo_mapM01Risk Tier)
	{
		switch (Tier)
		{
		case Edemo_mapM01Risk::Low: return FLinearColor(0.10f, 0.82f, 0.30f);
		case Edemo_mapM01Risk::Mid: return FLinearColor(1.0f, 0.65f, 0.08f);
		case Edemo_mapM01Risk::High: return FLinearColor(0.95f, 0.12f, 0.16f);
		}
		return FLinearColor::White;
	}
}

Ademo_mapM01Marker::Ademo_mapM01Marker()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	MarkerMesh->SetupAttachment(SceneRoot);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.65f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded()) MarkerMesh->SetStaticMesh(Cylinder.Object);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(25.0f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);
	Tags.AddUnique(TEXT("M01_GENERATED"));
}

void Ademo_mapM01Marker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void Ademo_mapM01Marker::Configure(Edemo_mapM01MarkerType InType, FName InStableId, Edemo_mapM01Risk InRiskTier)
{
	MarkerType = InType;
	StableId = InStableId;
	RiskTier = InRiskTier;
	Tags.AddUnique(TEXT("M01_GENERATED"));
	Tags.AddUnique(StableId);
	RefreshPresentation();
}

void Ademo_mapM01Marker::ConfigureRoot() { Configure(Edemo_mapM01MarkerType::MapRoot, TEXT("M01"), Edemo_mapM01Risk::Low); }
void Ademo_mapM01Marker::ConfigureRoute(FName Id, Edemo_mapM01Risk Tier) { Configure(Edemo_mapM01MarkerType::Route, Id, Tier); }
void Ademo_mapM01Marker::ConfigureEncounter(FName Id, Edemo_mapM01Risk Tier) { Configure(Edemo_mapM01MarkerType::Encounter, Id, Tier); }
void Ademo_mapM01Marker::ConfigureResourceCluster(FName Id, Edemo_mapM01Risk Tier) { Configure(Edemo_mapM01MarkerType::ResourceCluster, Id, Tier); }
void Ademo_mapM01Marker::ConfigureBoss() { Configure(Edemo_mapM01MarkerType::Boss, TEXT("M01.Boss.Main"), Edemo_mapM01Risk::High); }
void Ademo_mapM01Marker::ConfigureRetreatConnection(FName Id, Edemo_mapM01Risk Tier) { Configure(Edemo_mapM01MarkerType::RetreatConnection, Id, Tier); }

void Ademo_mapM01Marker::ConfigureLowRoute(FName Id) { ConfigureRoute(Id, Edemo_mapM01Risk::Low); }
void Ademo_mapM01Marker::ConfigureMidRoute(FName Id) { ConfigureRoute(Id, Edemo_mapM01Risk::Mid); }
void Ademo_mapM01Marker::ConfigureHighRoute(FName Id) { ConfigureRoute(Id, Edemo_mapM01Risk::High); }
void Ademo_mapM01Marker::ConfigureLowEncounter(FName Id) { ConfigureEncounter(Id, Edemo_mapM01Risk::Low); }
void Ademo_mapM01Marker::ConfigureMidEncounter(FName Id) { ConfigureEncounter(Id, Edemo_mapM01Risk::Mid); }
void Ademo_mapM01Marker::ConfigureHighEncounter(FName Id) { ConfigureEncounter(Id, Edemo_mapM01Risk::High); }
void Ademo_mapM01Marker::ConfigureLowResourceCluster(FName Id) { ConfigureResourceCluster(Id, Edemo_mapM01Risk::Low); }
void Ademo_mapM01Marker::ConfigureMidResourceCluster(FName Id) { ConfigureResourceCluster(Id, Edemo_mapM01Risk::Mid); }
void Ademo_mapM01Marker::ConfigureHighResourceCluster(FName Id) { ConfigureResourceCluster(Id, Edemo_mapM01Risk::High); }
void Ademo_mapM01Marker::ConfigureLowRetreatConnection(FName Id) { ConfigureRetreatConnection(Id, Edemo_mapM01Risk::Low); }
void Ademo_mapM01Marker::ConfigureMidRetreatConnection(FName Id) { ConfigureRetreatConnection(Id, Edemo_mapM01Risk::Mid); }
void Ademo_mapM01Marker::ConfigureHighRetreatConnection(FName Id) { ConfigureRetreatConnection(Id, Edemo_mapM01Risk::High); }

void Ademo_mapM01Marker::RefreshPresentation()
{
	const FLinearColor Color = M01RiskColor(RiskTier);
	if (MarkerMesh) MarkerMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color));
	if (Label)
	{
		Label->SetText(FText::FromName(StableId));
		Label->SetTextRenderColor(Color.ToFColor(true));
		Label->SetVisibility(MarkerType != Edemo_mapM01MarkerType::MapRoot);
	}
}
