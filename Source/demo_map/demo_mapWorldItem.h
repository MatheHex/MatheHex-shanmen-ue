#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapInteractable.h"
#include "demo_mapWorldItem.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Non-authoritative world projection. InstanceId is its only item state. */
UCLASS()
class Ademo_mapWorldItem : public AActor, public Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	Ademo_mapWorldItem();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetInstanceId(FGuid InInstanceId) { InstanceId = InInstanceId; }
	FGuid GetInstanceId() const { return InstanceId; }
	void RefreshPresentation();
	FString GetWorldLabelText() const;
	FVector GetWorldLabelForwardVector() const;

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;
	virtual FVector GetInteractionLocation() const override;
	virtual int32 GetInteractionPriority() const override { return 30; }
	virtual void FocusChanged(bool bFocused) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> InteractionSphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleInstanceOnly, Category="World Item") FGuid InstanceId;
};
