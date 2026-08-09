#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapM01ArtProp.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class Edemo_mapM01ArtStyle : uint8
{
	Pine,
	TimberRuin,
	StoneRuin,
	RockSpire,
	SpiritCrystal,
	Landmark,
	BossAltar,
	ExitBeacon
};

/**
 * Non-authoritative M01 visual layer.  These props never own collision,
 * navigation, encounters, exits, or rewards; the persisted P2 graybox remains
 * the functional source of truth.
 */
UCLASS()
class Ademo_mapM01ArtProp : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapM01ArtProp();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigurePine(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureTimberRuin(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureStoneRuin(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureRockSpire(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureSpiritCrystal(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureLandmark(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureBossAltar(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Art")
	void ConfigureExitBeacon(FName InStableId, FVector InDimensions, FLinearColor InColor);

	FName GetStableId() const { return StableId; }
	Edemo_mapM01ArtStyle GetArtStyle() const { return ArtStyle; }
	UFUNCTION(BlueprintPure, Category="M01 Art")
	bool HasVisualOnlyContract() const;

private:
	void Configure(Edemo_mapM01ArtStyle InStyle, FName InStableId,
		FVector InDimensions, FLinearColor InColor);
	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MainMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> AccentMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> AccentLight;
	UPROPERTY(EditAnywhere, Category="M01 Art") FName StableId = TEXT("M01.Art.Unconfigured");
	UPROPERTY(EditAnywhere, Category="M01 Art") Edemo_mapM01ArtStyle ArtStyle = Edemo_mapM01ArtStyle::RockSpire;
	UPROPERTY(EditAnywhere, Category="M01 Art", meta=(ClampMin="1.0")) FVector Dimensions = FVector(120.0f, 120.0f, 300.0f);
	UPROPERTY(EditAnywhere, Category="M01 Art") FLinearColor ArtColor = FLinearColor(0.24f, 0.32f, 0.26f, 1.0f);
};
