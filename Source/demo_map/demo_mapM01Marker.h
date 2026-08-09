#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapM01Extraction.h"
#include "demo_mapM01Marker.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class Edemo_mapM01MarkerType : uint8
{
	MapRoot,
	Route,
	Encounter,
	ResourceCluster,
	Boss,
	RetreatConnection
};

/** Persisted stable M01 topology/content anchor. It does not spawn content. */
UCLASS()
class Ademo_mapM01Marker : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapM01Marker();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureRoot();
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureRoute(FName InStableId, Edemo_mapM01Risk InRiskTier);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureEncounter(FName InStableId, Edemo_mapM01Risk InRiskTier);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureResourceCluster(FName InStableId, Edemo_mapM01Risk InRiskTier);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureBoss();
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureRetreatConnection(FName InStableId, Edemo_mapM01Risk InRiskTier);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureLowRoute(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureMidRoute(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureHighRoute(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureLowEncounter(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureMidEncounter(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureHighEncounter(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureLowResourceCluster(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureMidResourceCluster(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureHighResourceCluster(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureLowRetreatConnection(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureMidRetreatConnection(FName InStableId);
	UFUNCTION(BlueprintCallable, Category="M01 Marker") void ConfigureHighRetreatConnection(FName InStableId);

	FName GetStableId() const { return StableId; }
	Edemo_mapM01MarkerType GetMarkerType() const { return MarkerType; }
	Edemo_mapM01Risk GetRiskTier() const { return RiskTier; }

private:
	void Configure(Edemo_mapM01MarkerType InType, FName InStableId, Edemo_mapM01Risk InRiskTier);
	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MarkerMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(EditAnywhere, Category="M01 Marker") Edemo_mapM01MarkerType MarkerType = Edemo_mapM01MarkerType::Route;
	UPROPERTY(EditAnywhere, Category="M01 Marker") FName StableId = TEXT("M01.Marker.Unconfigured");
	UPROPERTY(EditAnywhere, Category="M01 Marker") Edemo_mapM01Risk RiskTier = Edemo_mapM01Risk::Low;
};
