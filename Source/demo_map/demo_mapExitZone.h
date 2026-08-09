#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapExitZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPrimitiveComponent;
class UMaterialInstanceDynamic;

/** Runtime-only mission exit. It completes only after the GameState unlocks it. */
UCLASS()
class Ademo_mapExitZone : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapExitZone();
	void SetExitUnlocked(bool bInUnlocked);
	void SetExitProgress(int32 Destroyed, int32 Required);
	bool IsExitUnlocked() const { return bExitUnlocked; }
	UBoxComponent* GetTriggerComponent() const { return TriggerComponent; }

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere, Category = "Exit")
	TObjectPtr<UBoxComponent> TriggerComponent;

	UPROPERTY(VisibleAnywhere, Category = "Exit")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Exit")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MarkerMaterial;

	bool bExitUnlocked = false;
	bool bCompletionHandled = false;
	int32 DestroyedTargets = 0;
	int32 RequiredTargets = 3;
};
