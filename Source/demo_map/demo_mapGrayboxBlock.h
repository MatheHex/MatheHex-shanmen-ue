#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapGrayboxBlock.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class Edemo_mapGrayboxBlockType : uint8
{
	Ground,
	BoundaryWall,
	RouteWall,
	LowCover,
	ProjectileBlocker,
	RegionMarker
};

/** Persisted graybox cube used by the V2-C authored level. */
UCLASS()
class Ademo_mapGrayboxBlock : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapGrayboxBlock();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="V2-C Graybox")
	void ConfigureGround(FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="V2-C Graybox")
	void ConfigureBoundaryWall(FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="V2-C Graybox")
	void ConfigureRouteWall(FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="V2-C Graybox")
	void ConfigureLowCover(FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="V2-C Graybox")
	void ConfigureProjectileBlocker(FVector InDimensions, FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="V2-C Graybox")
	void ConfigureRegionMarker(FVector InDimensions, FLinearColor InColor);

	Edemo_mapGrayboxBlockType GetBlockType() const { return BlockType; }
	FVector GetDimensions() const { return Dimensions; }

private:
	void Configure(Edemo_mapGrayboxBlockType InType, FVector InDimensions, FLinearColor InColor);
	void RefreshBlock();

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category="V2-C Graybox")
	Edemo_mapGrayboxBlockType BlockType = Edemo_mapGrayboxBlockType::RouteWall;

	UPROPERTY(EditAnywhere, Category="V2-C Graybox", meta=(ClampMin="1.0"))
	FVector Dimensions = FVector(100.0f);

	UPROPERTY(EditAnywhere, Category="V2-C Graybox")
	FLinearColor BlockColor = FLinearColor(0.35f, 0.38f, 0.42f, 1.0f);
};
