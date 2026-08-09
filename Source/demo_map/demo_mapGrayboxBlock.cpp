#include "demo_mapGrayboxBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapGrayboxBlock::Ademo_mapGrayboxBlock()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrayboxMesh"));
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(true);
	Tags.AddUnique(TEXT("V2C_GENERATED"));
}

void Ademo_mapGrayboxBlock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshBlock();
}

void Ademo_mapGrayboxBlock::BeginPlay()
{
	Super::BeginPlay();
	RefreshBlock();
}

void Ademo_mapGrayboxBlock::Configure(Edemo_mapGrayboxBlockType InType, FVector InDimensions, FLinearColor InColor)
{
	BlockType = InType;
	Dimensions = FVector(FMath::Max(1.0f, InDimensions.X), FMath::Max(1.0f, InDimensions.Y), FMath::Max(1.0f, InDimensions.Z));
	BlockColor = InColor;
	RefreshBlock();
}

void Ademo_mapGrayboxBlock::ConfigureGround(FVector D, FLinearColor C) { Configure(Edemo_mapGrayboxBlockType::Ground, D, C); }
void Ademo_mapGrayboxBlock::ConfigureBoundaryWall(FVector D, FLinearColor C) { Configure(Edemo_mapGrayboxBlockType::BoundaryWall, D, C); }
void Ademo_mapGrayboxBlock::ConfigureRouteWall(FVector D, FLinearColor C) { Configure(Edemo_mapGrayboxBlockType::RouteWall, D, C); }
void Ademo_mapGrayboxBlock::ConfigureLowCover(FVector D, FLinearColor C) { Configure(Edemo_mapGrayboxBlockType::LowCover, D, C); }
void Ademo_mapGrayboxBlock::ConfigureProjectileBlocker(FVector D, FLinearColor C) { Configure(Edemo_mapGrayboxBlockType::ProjectileBlocker, D, C); }
void Ademo_mapGrayboxBlock::ConfigureRegionMarker(FVector D, FLinearColor C) { Configure(Edemo_mapGrayboxBlockType::RegionMarker, D, C); }

void Ademo_mapGrayboxBlock::RefreshBlock()
{
	if (Mesh == nullptr)
	{
		return;
	}
	Mesh->SetRelativeScale3D(Dimensions / 100.0f);
	Mesh->SetCollisionEnabled(BlockType == Edemo_mapGrayboxBlockType::RegionMarker ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCanEverAffectNavigation(BlockType != Edemo_mapGrayboxBlockType::RegionMarker);
	if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BlockColor);
	}
}
