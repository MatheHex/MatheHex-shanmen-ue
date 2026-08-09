#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapM01GrayboxBlock.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class Edemo_mapM01GrayboxBlockType : uint8
{
	Ground,
	RegionFloor,
	Boundary,
	LowBarrier,
	SolidWall
};

/** Persisted M01 graybox surface with an explicit collision contract. */
UCLASS()
class Ademo_mapM01GrayboxBlock : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapM01GrayboxBlock();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="M01 Graybox")
	void ConfigureGround(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Graybox")
	void ConfigureRegionFloor(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Graybox")
	void ConfigureBoundary(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Graybox")
	void ConfigureLowBarrier(FName InStableId, FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="M01 Graybox")
	void ConfigureSolidWall(FName InStableId, FVector InDimensions, FLinearColor InColor);

	FName GetStableId() const { return StableId; }
	Edemo_mapM01GrayboxBlockType GetBlockType() const { return BlockType; }
	FVector GetDimensions() const { return Dimensions; }
	bool HasExpectedCollisionContract() const;

private:
	void Configure(Edemo_mapM01GrayboxBlockType InType, FName InStableId, FVector InDimensions, FLinearColor InColor);
	void RefreshBlock();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(EditAnywhere, Category="M01 Graybox") Edemo_mapM01GrayboxBlockType BlockType = Edemo_mapM01GrayboxBlockType::Ground;
	UPROPERTY(EditAnywhere, Category="M01 Graybox") FName StableId = TEXT("M01.Block.Unconfigured");
	UPROPERTY(EditAnywhere, Category="M01 Graybox", meta=(ClampMin="1.0")) FVector Dimensions = FVector(100.0f);
	UPROPERTY(EditAnywhere, Category="M01 Graybox") FLinearColor BlockColor = FLinearColor(0.3f, 0.34f, 0.4f, 1.0f);
};
