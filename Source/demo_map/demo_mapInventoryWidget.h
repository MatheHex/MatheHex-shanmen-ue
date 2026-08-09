#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapItemTypes.h"
#include "demo_mapInventoryWidget.generated.h"

class UButton;
class UTextBlock;
class UUniformGridPanel;
class UWrapBox;
class Ademo_mapV3ProgressionManager;

/**
 * P5 Runtime player inventory. The widget owns selection/confirmation only;
 * every cell, detail and action is rebuilt from ItemAuthority after mutation.
 */
UCLASS()
class Udemo_mapInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForManager(Ademo_mapV3ProgressionManager* InManager);
	void RefreshFromAuthority();
	/** Clears only uncommitted UI drag/Bundle state. ItemAuthority is untouched. */
	void ClearTransientDragState();
	void ShowOperationResult(const Fdemo_mapItemOperationResult& Result);
	int32 GetInventoryButtonCount() const { return RuntimeSlotCount; }
	Edemo_mapItemResultCode GetLastResultCode() const { return LastResultCode; }
	FGuid GetSelectedInventoryInstance() const { return SelectedInventoryInstance; }
	FName GetSelectedEquipmentSlot() const { return SelectedEquipmentSlot; }
	int32 GetSelectedInventorySlotIndex() const
	{
		return SelectedInventorySlotIndex;
	}

#if !UE_BUILD_SHIPPING
	bool AutomationClickInventorySlot(int32 SlotIndex);
	bool AutomationClickEquipmentSlot(FName SlotId);
	bool AutomationClickEquip();
	bool AutomationClickUnequip();
	bool AutomationClickDrop();
	bool AutomationClickMove();
	bool AutomationClickUse();
	bool AutomationClickHotbarSlot(int32 SlotNumber);
	bool AutomationClickClose();
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

private:
	void BuildInterface();
	void SelectInventorySlot(int32 SlotIndex);
	void SelectEquipmentCell(int32 EncodedIndex);
	void HandleItemDrop(
		int32 SourceSlotIndex,
		FGuid SourceItemId,
		Edemo_mapItemPresentationContext SourceContext,
		int32 TargetSlotIndex,
		Edemo_mapItemPresentationContext TargetContext);
	void SelectEquipmentSlot(FName SlotId);
	void HandleEquip();
	void HandleUnequip();
	void HandleDrop();
	void HandleConfirmSpatialBundleDrop();
	void HandleCancelSpatialBundleDrop();
	void HandleMove();
	void HandleUse();
	void HandleHotbarSlot(int32 SlotNumber);
	void HandleClose();
	void SetFeedback(const FString& Message, bool bSuccess);
	void RebuildCellPanels(
		const Fdemo_mapEntityLoadoutView& View);
	void RefreshDetailsAndActions(
		const Fdemo_mapItemAuthority& Authority);
	FText MakeResultText(
		const Fdemo_mapItemOperationResult& Result) const;
	int32 FindFirstEmptyInRange(
		const TArray<FGuid>& Slots,
		int32 BeginIndex,
		int32 EndIndex) const;

	UFUNCTION() void ClickEquip();
	UFUNCTION() void ClickUnequip();
	UFUNCTION() void ClickDrop();
	UFUNCTION() void ClickConfirmSpatialBundleDrop();
	UFUNCTION() void ClickCancelSpatialBundleDrop();
	UFUNCTION() void ClickMove();
	UFUNCTION() void ClickUse();
	UFUNCTION() void ClickClose();
	UFUNCTION() void ClickHotbar1();
	UFUNCTION() void ClickHotbar2();
	UFUNCTION() void ClickHotbar3();
	UFUNCTION() void ClickHotbar4();
	UFUNCTION() void ClickHotbar5();
	UFUNCTION() void ClickHotbar6();
	UFUNCTION() void ClickHotbar7();
	UFUNCTION() void ClickHotbar8();
	UFUNCTION() void ClickHotbar9();

	TWeakObjectPtr<Ademo_mapV3ProgressionManager> Manager;
	UPROPERTY(Transient)
	TArray<TObjectPtr<Udemo_mapItemCellWidget>> RuntimeCells;
	UPROPERTY(Transient) TObjectPtr<UWrapBox> EquipmentPanel;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> BaseQuickPanel;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> SpatialStoragePanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InventoryHeaderText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CapacityText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FeedbackText;
	UPROPERTY(Transient) TObjectPtr<UButton> EquipButton;
	UPROPERTY(Transient) TObjectPtr<UButton> UnequipButton;
	UPROPERTY(Transient) TObjectPtr<UButton> DropButton;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapItemCellWidget> WorldDropCell;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmSpatialBundleDropButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelSpatialBundleDropButton;
	UPROPERTY(Transient) TObjectPtr<UButton> MoveButton;
	UPROPERTY(Transient) TObjectPtr<UButton> UseButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> HotbarButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> HotbarTexts;
	FGuid SelectedInventoryInstance;
	FName SelectedEquipmentSlot = NAME_None;
	int32 SelectedInventorySlotIndex = INDEX_NONE;
	int32 PendingHotbarSlotNumber = INDEX_NONE;
	FGuid PendingHotbarInstance;
	Fdemo_mapPlayerItemDropIntent PendingSpatialBundleDrop;
	int32 RuntimeSlotCount = 0;
	Edemo_mapItemResultCode LastResultCode =
		Edemo_mapItemResultCode::Success;
	bool bBuilt = false;
};
