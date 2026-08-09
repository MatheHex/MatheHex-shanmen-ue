#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_map0909BSectWarehouseWidget.generated.h"

class Ademo_map0909BFrameworkHost;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/** Transient drag intent only; Code B remains the sole item authority. */
UCLASS()
class Udemo_map0909BWarehouseDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	FGuid ItemId;
	FGuid SourceContainerId;
	int32 SourceSlot = INDEX_NONE;
	int32 ExpectedGraphRevision = INDEX_NONE;
};

/** One rendered Code B snapshot cell; it does not own an inventory array. */
UCLASS()
class Udemo_map0909BWarehouseSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeSlot(
		Ademo_map0909BFrameworkHost* InHost,
		const FGuid& InItemId,
		const FGuid& InContainerId,
		int32 InSlotIndex,
		int32 InExpectedGraphRevision,
		const FString& InLabel);

protected:
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

private:
	void BuildInterface();

	TWeakObjectPtr<Ademo_map0909BFrameworkHost> FrameworkHost;
	FGuid ItemId;
	FGuid ContainerId;
	int32 SlotIndex = INDEX_NONE;
	int32 ExpectedGraphRevision = INDEX_NONE;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LabelText;
	bool bBuilt = false;
};

/** New P2 Warehouse / Loadout projection hosted exclusively by the P1 shell. */
UCLASS()
class Udemo_map0909BSectWarehouseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForFramework(Ademo_map0909BFrameworkHost* InHost);
	void RefreshPresentation(const Fdemo_map0909BWarehousePresentation& InPresentation);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildInterface();
	void RebuildProjection();
	UTextBlock* AddText(const FString& Value, int32 Size, const FLinearColor& Color = FLinearColor::White);

	UFUNCTION()
	void ClickReturnToSect();

	TWeakObjectPtr<Ademo_map0909BFrameworkHost> FrameworkHost;
	Fdemo_map0909BWarehousePresentation Presentation;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Column;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ProjectionColumn;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GateText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectionText;
	UPROPERTY(Transient) TObjectPtr<UButton> ReturnButton;
	bool bBuilt = false;
};
