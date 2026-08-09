#include "demo_mapEncounterMarker.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

Ademo_mapEncounterMarker::Ademo_mapEncounterMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	EditorLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EditorLabel"));
	EditorLabel->SetupAttachment(SceneRoot);
	EditorLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorLabel->SetHiddenInGame(true);
	EditorLabel->SetHorizontalAlignment(EHTA_Center);
	EditorLabel->SetVerticalAlignment(EVRTA_TextCenter);
	EditorLabel->SetWorldSize(80.0f);
	EditorLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	EditorLabel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	Tags.AddUnique(TEXT("V2C_ENCOUNTER_MARKER"));
}

void Ademo_mapEncounterMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEditorLabel();
}

void Ademo_mapEncounterMarker::Configure(Edemo_mapEncounterMarkerType InType, int32 InIndex, const TCHAR* InId)
{
	MarkerType = InType;
	MarkerIndex = InIndex;
	MarkerId = InId;
	RefreshEditorLabel();
}

void Ademo_mapEncounterMarker::ConfigureFriendlySpawn()
{
	Configure(Edemo_mapEncounterMarkerType::FriendlySpawn, INDEX_NONE, TEXT("FriendlySpawn"));
}

void Ademo_mapEncounterMarker::ConfigureMeleeEnemySpawn()
{
	Configure(Edemo_mapEncounterMarkerType::MeleeEnemySpawn, INDEX_NONE, TEXT("MeleeEnemySpawn"));
}

void Ademo_mapEncounterMarker::ConfigureRangedEnemySpawn()
{
	Configure(Edemo_mapEncounterMarkerType::RangedEnemySpawn, INDEX_NONE, TEXT("RangedEnemySpawn"));
}

void Ademo_mapEncounterMarker::ConfigureHeavyEnemySpawn()
{
	Configure(Edemo_mapEncounterMarkerType::HeavyEnemySpawn, INDEX_NONE, TEXT("HeavyEnemySpawn"));
}

void Ademo_mapEncounterMarker::ConfigureTrainingTargetSpawn(int32 InMarkerIndex)
{
	Configure(Edemo_mapEncounterMarkerType::TrainingTargetSpawn, InMarkerIndex, *FString::Printf(TEXT("TrainingTargetSpawn_%d"), InMarkerIndex));
}

void Ademo_mapEncounterMarker::ConfigureExitSpawn()
{
	Configure(Edemo_mapEncounterMarkerType::ExitSpawn, INDEX_NONE, TEXT("ExitSpawn"));
}

void Ademo_mapEncounterMarker::RefreshEditorLabel()
{
	if (EditorLabel == nullptr)
	{
		return;
	}
	FLinearColor Color = FLinearColor::Green;
	switch (MarkerType)
	{
	case Edemo_mapEncounterMarkerType::FriendlySpawn: Color = FLinearColor::Green; break;
	case Edemo_mapEncounterMarkerType::MeleeEnemySpawn: Color = FLinearColor::Red; break;
	case Edemo_mapEncounterMarkerType::RangedEnemySpawn: Color = FLinearColor(0.85f, 0.02f, 1.0f); break;
	case Edemo_mapEncounterMarkerType::HeavyEnemySpawn: Color = FLinearColor(1.0f, 0.25f, 0.01f); break;
	case Edemo_mapEncounterMarkerType::TrainingTargetSpawn: Color = FLinearColor::Yellow; break;
	case Edemo_mapEncounterMarkerType::ExitSpawn: Color = FLinearColor(0.0f, 1.0f, 1.0f); break;
	}
	EditorLabel->SetText(FText::FromString(MarkerId));
	EditorLabel->SetTextRenderColor(Color.ToFColor(true));
}
