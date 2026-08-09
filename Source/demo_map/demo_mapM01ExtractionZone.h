#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapInteractable.h"
#include "demo_mapM01Extraction.h"
#include "demo_mapM01ExtractionZone.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Temporary reusable M01 exit projection; the GameMode-owned authority holds all state. */
UCLASS()
class Ademo_mapM01ExtractionZone : public AActor, public Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	Ademo_mapM01ExtractionZone();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Configure(Edemo_mapM01ExitType InExitType);
	UFUNCTION(BlueprintCallable, Category="M01 Extraction")
	void ConfigureAuthored(Edemo_mapM01ExitType InExitType);
	UFUNCTION(BlueprintCallable, Category="M01 Extraction") void ConfigureAuthoredRegular();
	UFUNCTION(BlueprintCallable, Category="M01 Extraction") void ConfigureAuthoredBoss();
	UFUNCTION(BlueprintCallable, Category="M01 Extraction") void ConfigureAuthoredDiscardSpatial();
	Edemo_mapM01ExitType GetExitType() const { return ExitType; }
	FName GetStableId() const { return Fdemo_mapM01ExtractionAuthority::ExitId(ExitType); }
	bool IsAuthoredForM01() const { return bAuthoredForM01; }
	void SetProjectionActive(bool bActive);
	void RefreshPresentation();

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;
	virtual FVector GetInteractionLocation() const override;
	virtual int32 GetInteractionPriority() const override { return 70; }
	virtual void FocusChanged(bool bFocused) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> InteractionSphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MarkerMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> StatusText;
	UPROPERTY(EditAnywhere, Category="M01 Extraction")
	Edemo_mapM01ExitType ExitType = Edemo_mapM01ExitType::Regular;
	UPROPERTY(EditAnywhere, Category="M01 Extraction")
	bool bAuthoredForM01 = false;
	bool bLastInRange = false;
	bool bProjectionActive = true;
};
