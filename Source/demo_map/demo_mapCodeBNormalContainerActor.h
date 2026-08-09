#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapInteractable.h"
#include "demo_mapCodeBNormalContainerActor.generated.h"

class UBoxComponent;
class UTextRenderComponent;
class USceneComponent;
class Ademo_mapV3ProgressionManager;

/**
 * P10's map-placeable adapter for the single normal-container target. It owns
 * only Actor existence, prompt, range surface and real timer delivery; Code B
 * owns every item/state transition and Code A's existing focus/input dispatch
 * remains the only way to reach RequestInteract.
 */
UCLASS()
class DEMO_MAP_API Ademo_mapCodeBNormalContainerActor : public AActor, public Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	Ademo_mapCodeBNormalContainerActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Map-author visible, static identity. P10 accepts only its one documented value. */
	UPROPERTY(EditInstanceOnly, Category = "Code B Normal Container")
	FName MapTargetIdentity = FName(TEXT("M01.CodeBNormalContainer.BasicCache.01"));

	FName GetMapTargetIdentity() const { return MapTargetIdentity; }
	void ScheduleOpenCompletion(const FGuid& ActionId, float DurationSeconds);
	void ScheduleSearchCompletion(const FGuid& ActionId, float DurationSeconds);
	void ClearPendingAction();

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;
	virtual FVector GetInteractionLocation() const override;
	virtual int32 GetInteractionPriority() const override { return 27; }
	virtual void FocusChanged(bool bFocused) override;

private:
	Ademo_mapV3ProgressionManager* ResolveManager() const;
	void CompletePendingAction();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	FTimerHandle PendingActionTimer;
	FGuid PendingActionId;
	bool bPendingActionIsSearch = false;
};
