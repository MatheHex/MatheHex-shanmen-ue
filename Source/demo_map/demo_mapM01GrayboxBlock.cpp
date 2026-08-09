#include "demo_mapM01GrayboxBlock.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapM01GrayboxBlock::Ademo_mapM01GrayboxBlock()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("M01GrayboxMesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded()) Mesh->SetMaterial(0, BasicMaterial.Object);
	Mesh->SetMobility(EComponentMobility::Static);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StableIdLabel"));
	Label->SetupAttachment(Mesh);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(24.0f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tags.AddUnique(TEXT("M01_GENERATED"));
}

void Ademo_mapM01GrayboxBlock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshBlock();
}

void Ademo_mapM01GrayboxBlock::Configure(Edemo_mapM01GrayboxBlockType InType, FName InStableId, FVector InDimensions, FLinearColor InColor)
{
	BlockType = InType;
	StableId = InStableId;
	Dimensions = FVector(FMath::Max(1.0f, InDimensions.X), FMath::Max(1.0f, InDimensions.Y), FMath::Max(1.0f, InDimensions.Z));
	BlockColor = InColor;
	Tags.AddUnique(TEXT("M01_GENERATED"));
	Tags.AddUnique(StableId);
	RefreshBlock();
}

void Ademo_mapM01GrayboxBlock::ConfigureGround(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01GrayboxBlockType::Ground, Id, D, C); }
void Ademo_mapM01GrayboxBlock::ConfigureRegionFloor(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01GrayboxBlockType::RegionFloor, Id, D, C); }
void Ademo_mapM01GrayboxBlock::ConfigureBoundary(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01GrayboxBlockType::Boundary, Id, D, C); }
void Ademo_mapM01GrayboxBlock::ConfigureLowBarrier(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01GrayboxBlockType::LowBarrier, Id, D, C); }
void Ademo_mapM01GrayboxBlock::ConfigureSolidWall(FName Id, FVector D, FLinearColor C) { Configure(Edemo_mapM01GrayboxBlockType::SolidWall, Id, D, C); }

bool Ademo_mapM01GrayboxBlock::HasExpectedCollisionContract() const
{
	if (!Mesh) return false;
	if (BlockType == Edemo_mapM01GrayboxBlockType::LowBarrier)
	{
		return Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
			&& Mesh->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Ignore;
	}
	if (BlockType == Edemo_mapM01GrayboxBlockType::SolidWall)
	{
		return Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
			&& Mesh->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block;
	}
	return true;
}

void Ademo_mapM01GrayboxBlock::RefreshBlock()
{
	if (!Mesh) return;
	Mesh->SetRelativeScale3D(Dimensions / 100.0f);
	const bool bRegionFloor = BlockType == Edemo_mapM01GrayboxBlockType::RegionFloor;
	Mesh->SetCollisionEnabled(bRegionFloor ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	if (BlockType == Edemo_mapM01GrayboxBlockType::LowBarrier)
	{
		// Existing flying attacks use WorldDynamic query objects. Ignoring that
		// channel preserves projectile flight while Pawns still collide.
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(!bRegionFloor);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(BlockColor));
	if (Label)
	{
		Label->SetText(FText::FromName(StableId));
		Label->SetTextRenderColor(BlockColor.ToFColor(true));
		Label->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
		Label->SetVisibility(BlockType == Edemo_mapM01GrayboxBlockType::LowBarrier
			|| BlockType == Edemo_mapM01GrayboxBlockType::SolidWall);
	}
}
