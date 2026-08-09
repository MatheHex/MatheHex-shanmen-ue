#include "demo_mapV3ProgressionMarker.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

Ademo_mapV3ProgressionMarker::Ademo_mapV3ProgressionMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetWorldSize(28.0f);
	Label->SetRelativeLocation(FVector(0, 0, 80));
	SetActorEnableCollision(false);
}

void Ademo_mapV3ProgressionMarker::BeginPlay()
{
	Super::BeginPlay();
	// Marker IDs are editor diagnostics, not player-facing world labels.
	if (Label) Label->SetVisibility(false);
}

void Ademo_mapV3ProgressionMarker::Configure(Edemo_mapV3ProgressionMarkerType InType, int32 InIndex, FName InStableId, FName InDefinitionId, int32 InQuantity)
{
	MarkerType = InType;
	MarkerIndex = InIndex;
	StableId = InStableId;
	DefinitionId = InDefinitionId;
	Quantity = InQuantity;
	RefreshLabel();
}

void Ademo_mapV3ProgressionMarker::ConfigureRoot() { Configure(Edemo_mapV3ProgressionMarkerType::ProgressionRoot, INDEX_NONE, TEXT("Feature.V3.Progression")); }
void Ademo_mapV3ProgressionMarker::ConfigureChest(int32 InIndex) { Configure(Edemo_mapV3ProgressionMarkerType::Chest, InIndex, FName(*FString::Printf(TEXT("V3.Chest.%02d"), InIndex + 1))); }
void Ademo_mapV3ProgressionMarker::ConfigureWorldItem(int32 InIndex, FName InDefinitionId, int32 InQuantity) { Configure(Edemo_mapV3ProgressionMarkerType::WorldItem, InIndex, FName(*FString::Printf(TEXT("V3.WorldItem.%02d"), InIndex + 1)), InDefinitionId, InQuantity); }
void Ademo_mapV3ProgressionMarker::ConfigureLootDisplayArea() { Configure(Edemo_mapV3ProgressionMarkerType::LootDisplayArea, INDEX_NONE, TEXT("V3.LootDisplayArea")); }
void Ademo_mapV3ProgressionMarker::ConfigureEnemyDropValidation() { Configure(Edemo_mapV3ProgressionMarkerType::EnemyDropValidation, INDEX_NONE, TEXT("V3.EnemyDropValidation")); }

void Ademo_mapV3ProgressionMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshLabel();
}

void Ademo_mapV3ProgressionMarker::RefreshLabel()
{
	if (!Label) return;
	FString Text = StableId.ToString();
	if (MarkerType == Edemo_mapV3ProgressionMarkerType::WorldItem) Text += FString::Printf(TEXT("\n%s x%d"), *DefinitionId.ToString(), Quantity);
	Label->SetText(FText::FromString(Text));
	const FColor Color = MarkerType == Edemo_mapV3ProgressionMarkerType::ProgressionRoot ? FColor::Cyan :
		MarkerType == Edemo_mapV3ProgressionMarkerType::Chest ? FColor::Yellow :
		MarkerType == Edemo_mapV3ProgressionMarkerType::WorldItem ? FColor::Green : FColor(180, 120, 255);
	Label->SetTextRenderColor(Color);
}
