#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapEncounterMarker.generated.h"

class USceneComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class Edemo_mapEncounterMarkerType : uint8
{
	FriendlySpawn,
	MeleeEnemySpawn,
	RangedEnemySpawn,
	HeavyEnemySpawn,
	TrainingTargetSpawn,
	ExitSpawn
};

/** Editor-authored location marker. GameMode owns all runtime spawning and state. */
UCLASS()
class Ademo_mapEncounterMarker : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapEncounterMarker();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="V2-C Marker")
	void ConfigureFriendlySpawn();

	UFUNCTION(BlueprintCallable, Category="V2-C Marker")
	void ConfigureMeleeEnemySpawn();

	UFUNCTION(BlueprintCallable, Category="V2-D Marker")
	void ConfigureRangedEnemySpawn();

	UFUNCTION(BlueprintCallable, Category="V2-D Marker")
	void ConfigureHeavyEnemySpawn();

	UFUNCTION(BlueprintCallable, Category="V2-C Marker")
	void ConfigureTrainingTargetSpawn(int32 InMarkerIndex);

	UFUNCTION(BlueprintCallable, Category="V2-C Marker")
	void ConfigureExitSpawn();

	Edemo_mapEncounterMarkerType GetMarkerType() const { return MarkerType; }
	int32 GetMarkerIndex() const { return MarkerIndex; }
	const FString& GetMarkerId() const { return MarkerId; }

private:
	void RefreshEditorLabel();
	void Configure(Edemo_mapEncounterMarkerType InType, int32 InIndex, const TCHAR* InId);

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UTextRenderComponent> EditorLabel;

	UPROPERTY(EditAnywhere, Category="V2-C Marker")
	Edemo_mapEncounterMarkerType MarkerType = Edemo_mapEncounterMarkerType::FriendlySpawn;

	UPROPERTY(EditAnywhere, Category="V2-C Marker")
	int32 MarkerIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category="V2-C Marker")
	FString MarkerId = TEXT("FriendlySpawn");
};
