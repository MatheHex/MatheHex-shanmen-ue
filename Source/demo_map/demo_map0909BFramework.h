#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_map0909BFrameworkTypes.h"
#include "demo_map0909BRunStartCoordinator.h"
#include "demo_map0909BFramework.generated.h"

class Ademo_mapGameMode;
class Ademo_mapPlayerController;
class Udemo_map0909BSectWidget;
class Udemo_map0909BSectWarehouseWidget;
class Fdemo_map0909BSectWarehouseService;

/**
 * New default 0.0.9B product shell. It owns the top-level state, the visible
 * Sect UI, and the M01 start coordinator; retained runtime code is an adapter.
 */
UCLASS()
class Ademo_map0909BFrameworkHost : public AActor
{
	GENERATED_BODY()

public:
	Ademo_map0909BFrameworkHost();
	bool InitializeForGame(
		Ademo_mapGameMode* InGameMode,
		Ademo_mapPlayerController* InController);
	void RequestStartM01FromUI();
	void RequestOpenWarehouseFromUI();
	void RequestWarehouseDragDrop(
		const FGuid& ItemId,
		const FGuid& SourceContainerId,
		int32 SourceSlot,
		const FGuid& TargetContainerId,
		int32 TargetSlot,
		int32 ExpectedGraphRevision);
	void RequestCloseWarehouseFromUI();
	Edemo_map0909BTopState GetTopState() const;
	const Fdemo_map0909BStartDiagnostic& GetLastStartDiagnostic() const;
	bool IsInitialized() const { return bInitialized; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ShowSect(const FString& InFeedback);
	void RefreshSect(const FString& InFeedback);
	bool OpenWarehouseService(FString& OutDiagnostic);
	void ShowWarehouse(const struct Fdemo_map0909BWarehousePresentation& Presentation);

	TWeakObjectPtr<Ademo_mapGameMode> GameMode;
	TWeakObjectPtr<Ademo_mapPlayerController> Controller;
	TUniquePtr<Fdemo_map0909BRunStartCoordinator> StartCoordinator;
	TUniquePtr<Fdemo_map0909BSectWarehouseService> WarehouseService;
	UPROPERTY(Transient) TObjectPtr<Udemo_map0909BSectWidget> SectWidget;
	UPROPERTY(Transient) TObjectPtr<Udemo_map0909BSectWarehouseWidget> WarehouseWidget;
	bool bInitialized = false;
};
