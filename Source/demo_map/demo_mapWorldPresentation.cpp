#include "demo_mapWorldPresentation.h"

#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

bool Fdemo_mapWorldPresentation::FaceLabelToCamera(UTextRenderComponent* Label)
{
	if (!Label) return false;
	FVector DirectionToCamera(-1.0f, 0.0f, 1.4f);
	bool bUsedLiveCamera = false;
	if (const UWorld* World = Label->GetWorld())
	{
		if (const APlayerController* Controller = World->GetFirstPlayerController())
		{
			if (const APlayerCameraManager* Camera = Controller->PlayerCameraManager)
			{
				// The V3 camera follows the player but keeps a fixed view rotation.
				// Using the inverse view direction keeps labels stable during camera translation.
				DirectionToCamera = -Camera->GetCameraRotation().Vector();
				bUsedLiveCamera = true;
			}
		}
	}
	if (!DirectionToCamera.Normalize()) DirectionToCamera = FVector(-1.0f, 0.0f, 1.4f).GetSafeNormal();
	Label->SetWorldRotation(FRotationMatrix::MakeFromX(DirectionToCamera).Rotator());
	const FVector Scale = Label->GetRelativeScale3D();
	Label->SetRelativeScale3D(FVector(FMath::Abs(Scale.X), FMath::Abs(Scale.Y), FMath::Abs(Scale.Z)));
	return bUsedLiveCamera;
}

FText Fdemo_mapWorldPresentation::MakeItemWorldLabel(const FString& StableWorldName, int32 Quantity)
{
	return FText::FromString(FString::Printf(TEXT("%s x%d"), *StableWorldName, Quantity));
}
