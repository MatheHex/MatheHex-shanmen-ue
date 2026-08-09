#include "demo_mapExitZone.h"
#include "demo_map.h"
#include "demo_mapGameState.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapExitZone::Ademo_mapExitZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitTrigger"));
	SetRootComponent(TriggerComponent);
	TriggerComponent->SetBoxExtent(FVector(125.0f, 125.0f, 120.0f));
	TriggerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerComponent->SetGenerateOverlapEvents(true);
	TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &Ademo_mapExitZone::OnTriggerBeginOverlap);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExitMarker"));
	MarkerMesh->SetupAttachment(TriggerComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CylinderMesh.Object);
	}
	MarkerMesh->SetRelativeScale3D(FVector(3.2f, 3.2f, 0.18f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMaterial = MarkerMesh->CreateDynamicMaterialInstance(0);

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ExitStatusText"));
	StatusText->SetupAttachment(TriggerComponent);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(58.0f);
	RefreshPresentation();
}

void Ademo_mapExitZone::SetExitProgress(int32 Destroyed, int32 Required)
{
	DestroyedTargets = Destroyed;
	RequiredTargets = Required;
	RefreshPresentation();
}

void Ademo_mapExitZone::SetExitUnlocked(bool bInUnlocked)
{
	if (bExitUnlocked == bInUnlocked)
	{
		return;
	}

	bExitUnlocked = bInUnlocked;
	RefreshPresentation();
}

void Ademo_mapExitZone::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	if (OtherActor == nullptr || OtherActor != PlayerPawn)
	{
		return;
	}
	if (const Udemo_mapPlayerHealthComponent* Health = PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>(); Health != nullptr && Health->IsDefeated())
	{
		UE_LOG(Logdemo_map, Log, TEXT("T7: exit overlap ignored because player is defeated."));
		return;
	}

	Ademo_mapGameState* MissionState = World != nullptr ? Cast<Ademo_mapGameState>(World->GetGameState()) : nullptr;
	if (!bExitUnlocked || MissionState == nullptr || !MissionState->CanUseExit())
	{
		UE_LOG(Logdemo_map, Log, TEXT("T5: exit overlap ignored; targets remain."));
		return;
	}

	if (bCompletionHandled)
	{
		return;
	}

	if (MissionState->TryCompleteMission())
	{
		bCompletionHandled = true;
		UE_LOG(Logdemo_map, Log, TEXT("T5: player entered unlocked exit."));
		if (Ademo_mapGameMode* GameMode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode()))
		{
			GameMode->HandleExtraction();
		}
	}
}

void Ademo_mapExitZone::RefreshPresentation()
{
	if (StatusText == nullptr)
	{
		return;
	}

	const FLinearColor Color = bExitUnlocked ? FLinearColor(0.0f, 1.0f, 0.08f) : FLinearColor(0.9f, 0.02f, 0.02f);
	StatusText->SetText(FText::FromString(bExitUnlocked ? TEXT("EXIT OPEN") : FString::Printf(TEXT("EXIT LOCKED\nDestroy Targets: %d / %d"), DestroyedTargets, RequiredTargets)));
	StatusText->SetTextRenderColor(Color.ToFColor(true));
	if (MarkerMaterial != nullptr)
	{
		MarkerMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		MarkerMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}
