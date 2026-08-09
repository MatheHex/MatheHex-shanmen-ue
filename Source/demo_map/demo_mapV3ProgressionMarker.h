#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapV3ProgressionMarker.generated.h"

class USceneComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class Edemo_mapV3ProgressionMarkerType : uint8
{
	ProgressionRoot,
	Chest,
	WorldItem,
	LootDisplayArea,
	EnemyDropValidation
};

/** Persisted V3-only authoring marker; runtime behavior is enabled by exactly one root marker. */
UCLASS()
class Ademo_mapV3ProgressionMarker : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapV3ProgressionMarker();
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="V3 Progression") void ConfigureRoot();
	UFUNCTION(BlueprintCallable, Category="V3 Progression") void ConfigureChest(int32 InIndex);
	UFUNCTION(BlueprintCallable, Category="V3 Progression") void ConfigureWorldItem(int32 InIndex, FName InDefinitionId, int32 InQuantity);
	UFUNCTION(BlueprintCallable, Category="V3 Progression") void ConfigureLootDisplayArea();
	UFUNCTION(BlueprintCallable, Category="V3 Progression") void ConfigureEnemyDropValidation();

	Edemo_mapV3ProgressionMarkerType GetMarkerType() const { return MarkerType; }
	int32 GetMarkerIndex() const { return MarkerIndex; }
	FName GetStableId() const { return StableId; }
	FName GetDefinitionId() const { return DefinitionId; }
	int32 GetQuantity() const { return Quantity; }

private:
	void Configure(Edemo_mapV3ProgressionMarkerType InType, int32 InIndex, FName InStableId, FName InDefinitionId = NAME_None, int32 InQuantity = 0);
	void RefreshLabel();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(EditAnywhere, Category="V3 Progression") Edemo_mapV3ProgressionMarkerType MarkerType = Edemo_mapV3ProgressionMarkerType::ProgressionRoot;
	UPROPERTY(EditAnywhere, Category="V3 Progression") int32 MarkerIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, Category="V3 Progression") FName StableId = TEXT("Feature.V3.Progression");
	UPROPERTY(EditAnywhere, Category="V3 Progression") FName DefinitionId = NAME_None;
	UPROPERTY(EditAnywhere, Category="V3 Progression") int32 Quantity = 0;
};
