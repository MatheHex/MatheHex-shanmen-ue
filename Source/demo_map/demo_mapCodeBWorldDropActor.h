#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "demo_mapInteractable.h"
#include "demo_mapCodeBWorldDropActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UTextRenderComponent;
class Ademo_mapV3ProgressionManager;

/** P14 Code A projection only. Destroying this actor never mutates P6 WorldDrops. */
UCLASS()
class DEMO_MAP_API Ademo_mapCodeBWorldDropActor : public AActor, public Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	Ademo_mapCodeBWorldDropActor();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ConfigureCodeBWorldDrop(const FCodeBWorldDropProjection& Projection);
	const FGuid& GetOwnerId() const { return OwnerId; }
	const FGuid& GetRunInstanceId() const { return RunInstanceId; }
	const FGuid& GetWorldDropId() const { return WorldDropId; }

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;
	virtual FVector GetInteractionLocation() const override { return GetActorLocation(); }
	virtual int32 GetInteractionPriority() const override { return 26; }
	virtual void FocusChanged(bool bFocused) override;

private:
	Ademo_mapV3ProgressionManager* ResolveManager() const;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
};
