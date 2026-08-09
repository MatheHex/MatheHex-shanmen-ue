#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapM01Extraction.h"
#include "demo_mapM01RiskZone.generated.h"

class UBoxComponent;
class USceneComponent;
class UTextRenderComponent;

/** Non-blocking authored risk volume that projects the current M01 tier to HUD. */
UCLASS()
class Ademo_mapM01RiskZone : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapM01RiskZone();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="M01 Risk") void ConfigureLow(FVector InExtent);
	UFUNCTION(BlueprintCallable, Category="M01 Risk") void ConfigureMid(FVector InExtent);
	UFUNCTION(BlueprintCallable, Category="M01 Risk") void ConfigureHigh(FVector InExtent);

	FName GetStableId() const { return StableId; }
	Edemo_mapM01Risk GetRiskTier() const { return RiskTier; }
	FVector GetExtent() const;

private:
	void Configure(FName InStableId, Edemo_mapM01Risk InRiskTier, FVector InExtent);
	void RefreshPresentation();

	UFUNCTION() void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION() void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Volume;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(EditAnywhere, Category="M01 Risk") FName StableId = TEXT("M01.Risk.LOW");
	UPROPERTY(EditAnywhere, Category="M01 Risk") Edemo_mapM01Risk RiskTier = Edemo_mapM01Risk::Low;
	UPROPERTY(EditAnywhere, Category="M01 Risk") FVector Extent = FVector(1000.0f, 1000.0f, 300.0f);
};
