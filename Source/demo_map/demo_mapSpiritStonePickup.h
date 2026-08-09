#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapInteractable.h"
#include "demo_mapSpiritStoneTypes.h"
#include "demo_mapSpiritStonePickup.generated.h"

class USceneComponent;
class USphereComponent;
class UTextRenderComponent;

/** One pure-currency P8 pickup. It owns no item instance or inventory container. */
UCLASS()
class Ademo_mapSpiritStonePickup : public AActor, public Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	Ademo_mapSpiritStonePickup();

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;
	virtual FVector GetInteractionLocation() const override;
	virtual int32 GetInteractionPriority() const override { return 80; }
	virtual void FocusChanged(bool bFocused) override;

	FName GetPickupId() const;
	FName GetSourceId() const;
	int64 GetValue() const;
	bool InitializeRuntimePickup(
		FName InPickupId,
		FName InSourceId,
		int64 InValue,
		FGuid InExpectedRunId);

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> InteractionSphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	FName PickupId = Fdemo_mapSpiritStonePickupContract::PickupId;
	FName SourceId = Fdemo_mapSpiritStonePickupContract::SourceId;
	int64 Value = Fdemo_mapSpiritStonePickupContract::Value;
	FGuid ExpectedRunId;
	bool bCommitted = false;
};
