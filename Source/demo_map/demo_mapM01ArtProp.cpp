#include "demo_mapM01ArtProp.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* M01ArtLabel(Edemo_mapM01ArtStyle Style, FName StableId)
	{
		switch (Style)
		{
		case Edemo_mapM01ArtStyle::Landmark:
			if (StableId.ToString().Contains(TEXT("Start"))) return TEXT("LOW · WOODLAND TRAIL");
			if (StableId.ToString().Contains(TEXT("Mid"))) return TEXT("MID · RUINED COURTYARD");
			if (StableId.ToString().Contains(TEXT("Elite"))) return TEXT("HIGH · SPIRIT RIFT");
			return TEXT("M01 · DESOLATE SPIRIT MINE");
		case Edemo_mapM01ArtStyle::BossAltar: return TEXT("BOSS · RELIC CORE");
		case Edemo_mapM01ArtStyle::ExitBeacon:
			if (StableId.ToString().Contains(TEXT("Regular"))) return TEXT("REGULAR EXTRACTION");
			if (StableId.ToString().Contains(TEXT("Discard"))) return TEXT("DISCARD EXTRACTION");
			return TEXT("BOSS EXTRACTION");
		default: return TEXT("");
		}
	}
}

Ademo_mapM01ArtProp::Ademo_mapM01ArtProp()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainVisual"));
	MainMesh->SetupAttachment(SceneRoot);
	MainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MainMesh->SetCanEverAffectNavigation(false);
	AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccentVisual"));
	AccentMesh->SetupAttachment(SceneRoot);
	AccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AccentMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Cube.Succeeded()) MainMesh->SetStaticMesh(Cube.Object);
	if (Cone.Succeeded()) AccentMesh->SetStaticMesh(Cone.Object);
	if (BasicMaterial.Succeeded())
	{
		MainMesh->SetMaterial(0, BasicMaterial.Object);
		AccentMesh->SetMaterial(0, BasicMaterial.Object);
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ArtLabel"));
	Label->SetupAttachment(SceneRoot);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(34.0f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight"));
	AccentLight->SetupAttachment(SceneRoot);
	AccentLight->SetIntensity(0.0f);
	AccentLight->SetAttenuationRadius(620.0f);
	AccentLight->SetCastShadows(false);

	SetActorEnableCollision(false);
	Tags.AddUnique(TEXT("M01_GENERATED"));
	Tags.AddUnique(TEXT("M01_ART"));
}

void Ademo_mapM01ArtProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void Ademo_mapM01ArtProp::Configure(Edemo_mapM01ArtStyle InStyle,
	FName InStableId, FVector InDimensions, FLinearColor InColor)
{
	ArtStyle = InStyle;
	StableId = InStableId;
	Dimensions = FVector(
		FMath::Max(1.0f, InDimensions.X),
		FMath::Max(1.0f, InDimensions.Y),
		FMath::Max(1.0f, InDimensions.Z));
	ArtColor = InColor;
	Tags.AddUnique(TEXT("M01_GENERATED"));
	Tags.AddUnique(TEXT("M01_ART"));
	Tags.AddUnique(StableId);
	RefreshPresentation();
}

void Ademo_mapM01ArtProp::ConfigurePine(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::Pine, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureTimberRuin(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::TimberRuin, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureStoneRuin(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::StoneRuin, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureRockSpire(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::RockSpire, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureSpiritCrystal(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::SpiritCrystal, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureLandmark(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::Landmark, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureBossAltar(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::BossAltar, Id, D, C); }
void Ademo_mapM01ArtProp::ConfigureExitBeacon(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01ArtStyle::ExitBeacon, Id, D, C); }

bool Ademo_mapM01ArtProp::HasVisualOnlyContract() const
{
	return MainMesh && AccentMesh
		&& MainMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& AccentMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !MainMesh->CanEverAffectNavigation()
		&& !AccentMesh->CanEverAffectNavigation();
}

void Ademo_mapM01ArtProp::RefreshPresentation()
{
	if (!MainMesh || !AccentMesh || !Label || !AccentLight) return;

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	MainMesh->SetVisibility(true);
	AccentMesh->SetVisibility(true);
	MainMesh->SetRelativeRotation(FRotator::ZeroRotator);
	AccentMesh->SetRelativeRotation(FRotator::ZeroRotator);

	const FVector Unit = Dimensions / 100.0f;
	switch (ArtStyle)
	{
	case Edemo_mapM01ArtStyle::Pine:
		MainMesh->SetStaticMesh(Cylinder);
		MainMesh->SetRelativeScale3D(FVector(Unit.X * 0.26f, Unit.Y * 0.26f, Unit.Z * 0.48f));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.24f));
		AccentMesh->SetStaticMesh(Cone);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y, Unit.Z * 0.72f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.62f));
		break;
	case Edemo_mapM01ArtStyle::TimberRuin:
		MainMesh->SetStaticMesh(Cube);
		MainMesh->SetRelativeScale3D(FVector(Unit.X * 0.22f, Unit.Y * 0.22f, Unit.Z));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.50f));
		AccentMesh->SetStaticMesh(Cube);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y * 0.20f, Unit.Z * 0.18f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.84f));
		break;
	case Edemo_mapM01ArtStyle::StoneRuin:
		MainMesh->SetStaticMesh(Cube);
		MainMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y, Unit.Z));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.50f));
		AccentMesh->SetStaticMesh(Cube);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X * 0.72f, Unit.Y * 0.72f, Unit.Z * 0.12f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 1.02f));
		break;
	case Edemo_mapM01ArtStyle::RockSpire:
		MainMesh->SetStaticMesh(Cone);
		MainMesh->SetRelativeScale3D(Unit);
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.50f));
		AccentMesh->SetStaticMesh(Sphere);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X * 0.28f, Unit.Y * 0.28f, Unit.Z * 0.10f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.72f));
		break;
	case Edemo_mapM01ArtStyle::SpiritCrystal:
		MainMesh->SetStaticMesh(Cone);
		MainMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y, Unit.Z));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.50f));
		AccentMesh->SetStaticMesh(Sphere);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X * 0.55f, Unit.Y * 0.55f, Unit.X * 0.55f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.82f));
		break;
	case Edemo_mapM01ArtStyle::Landmark:
		MainMesh->SetStaticMesh(Cube);
		MainMesh->SetRelativeScale3D(FVector(Unit.X * 0.18f, Unit.Y, Unit.Z));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.50f));
		AccentMesh->SetStaticMesh(Cube);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y, Unit.Z * 0.16f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.94f));
		break;
	case Edemo_mapM01ArtStyle::BossAltar:
		MainMesh->SetStaticMesh(Cylinder);
		MainMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y, Unit.Z * 0.24f));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.12f));
		AccentMesh->SetStaticMesh(Cone);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X * 0.34f, Unit.Y * 0.34f, Unit.Z));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.65f));
		break;
	case Edemo_mapM01ArtStyle::ExitBeacon:
		MainMesh->SetStaticMesh(Cylinder);
		MainMesh->SetRelativeScale3D(FVector(Unit.X, Unit.Y, Unit.Z * 0.16f));
		MainMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.08f));
		AccentMesh->SetStaticMesh(Sphere);
		AccentMesh->SetRelativeScale3D(FVector(Unit.X * 0.34f, Unit.Y * 0.34f, Unit.X * 0.34f));
		AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.82f));
		break;
	}

	MainMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(ArtColor * 0.56f));
	MainMesh->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(ArtColor * 0.56f));
	AccentMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(ArtColor));
	AccentMesh->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(ArtColor));

	const bool bLit = ArtStyle == Edemo_mapM01ArtStyle::SpiritCrystal
		|| ArtStyle == Edemo_mapM01ArtStyle::BossAltar
		|| ArtStyle == Edemo_mapM01ArtStyle::ExitBeacon;
	AccentLight->SetIntensity(bLit ? (ArtStyle == Edemo_mapM01ArtStyle::BossAltar ? 7200.0f : 2600.0f) : 0.0f);
	AccentLight->SetLightColor(ArtColor);
	AccentLight->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z * 0.78f));

	const FString Display = M01ArtLabel(ArtStyle, StableId);
	Label->SetText(FText::FromString(Display));
	Label->SetTextRenderColor(ArtColor.ToFColor(true));
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, Dimensions.Z + 80.0f));
	Label->SetVisibility(!Display.IsEmpty());
	SetActorEnableCollision(false);
}

