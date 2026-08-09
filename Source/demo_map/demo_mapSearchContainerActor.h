#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapInteractable.h"
#include "demo_mapRuntimeContainer.h"
#include "demo_mapSearchContainerActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class Udemo_mapItemSubsystem;
class Ademo_mapV3ProgressionManager;

/** Shared real Actor host for Chest and Corpse Runtime Containers. */
UCLASS()
class Ademo_mapSearchContainerActor : public AActor, public Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	Ademo_mapSearchContainerActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool InitializeSearchContainer(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId,
		Edemo_mapRuntimeContainerKind InKind,
		FName InStableSourceId,
		const TArray<Fdemo_mapRuntimeContainerSeedEntry>& Seed);
	Fdemo_mapRuntimeContainerResult SubmitContainerIntent(
		const Fdemo_mapRuntimeContainerIntent& Intent,
		bool bPlayerAlive,
		bool bInRange);
	Fdemo_mapSearchContainerDropResult SubmitPlayerDrop(
		const Fdemo_mapSearchContainerDropIntent& Intent);
	Fdemo_mapRuntimeContainerResult ReleaseOpeningHold();
	Fdemo_mapRuntimeContainerResult CancelContainerAction(const FString& Diagnostic);
	Fdemo_mapRuntimeContainerSnapshot GetContainerSnapshot() const;

	bool IsContainerInitialized() const { return ContainerAuthority.IsInitialized(); }
	bool IsContainerActionActive() const { return ContainerAuthority.IsActionActive(); }
	bool IsContainerOpening() const { return ContainerAuthority.IsOpening(); }
	bool IsContainerSearching() const { return ContainerAuthority.IsSearching(); }
	bool IsContainerOpened() const { return ContainerAuthority.GetState() == Edemo_mapRuntimeContainerState::Opened; }
	bool IsContainerEmpty() const { return ContainerAuthority.IsEmpty(); }
	FGuid GetContainerId() const { return ContainerAuthority.GetContainerId(); }
	FGuid GetOwningRunId() const { return ContainerAuthority.GetOwningRunId(); }
	int32 GetContainerRevision() const { return ContainerAuthority.GetRevision(); }
	FName GetStableSourceId() const { return StableSourceId; }
	float GetCurrentActionProgress01() const;
	FString GetWorldLabelText() const;
	bool HasActiveTimer() const;
	const FString& GetLastContainerDiagnostic() const { return LastDiagnostic; }

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;
	virtual FVector GetInteractionLocation() const override;
	virtual int32 GetInteractionPriority() const override { return 25; }
	virtual void FocusChanged(bool bFocused) override;

#if !UE_BUILD_SHIPPING
	Fdemo_mapRuntimeContainerResult CompleteActionForAutomation();
#endif

protected:
	void SetContainerDisplayLabel(const FString& Label);
	void SetContainerMeshColor(const FLinearColor& Color);
	void SetPlayerDepositAllowed(bool bAllowed)
	{
		bPlayerDepositAllowed = bAllowed;
	}
	void RefreshContainerPresentation();
	UStaticMeshComponent* GetContainerMesh() const { return Mesh; }

private:
	void CompleteTimedAction();
	bool IsPlayerInRange(const APlayerController* Controller) const;
	bool IsPlayerAlive(const APlayerController* Controller) const;
	void ScheduleActiveAction();
	void CleanupUnclaimedItems();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	TWeakObjectPtr<Ademo_mapV3ProgressionManager> Manager;
	TWeakObjectPtr<Udemo_mapItemSubsystem> Items;
	Fdemo_mapRuntimeContainerAuthority ContainerAuthority;
	FName StableSourceId = NAME_None;
	FString DisplayLabel = TEXT("CONTAINER");
	FTimerHandle ActiveActionTimer;
	double ActionStartSeconds = 0.0;
	float ActionDurationSeconds = 0.0f;
	FString LastDiagnostic;
	bool bCleanupComplete = false;
	bool bPlayerDepositAllowed = true;
};
