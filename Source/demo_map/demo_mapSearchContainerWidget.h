#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "demo_mapSearchContainerPresenter.h"
#include "demo_mapSearchContainerWidget.generated.h"

class UButton;
class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWrapBox;
class Ademo_mapV3ProgressionManager;

DECLARE_DELEGATE_OneParam(Fdemo_mapSearchContextActionClicked, int32);

/** Tiny native menu row used by the runtime item right-click menu. */
UCLASS()
class Udemo_mapSearchContextActionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeAction(
		const FString& Label,
		int32 InActionCode,
		Fdemo_mapSearchContextActionClicked InActivated);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildInterface();
	UFUNCTION() void ClickAction();

	FString ActionLabel;
	int32 ActionCode = INDEX_NONE;
	Fdemo_mapSearchContextActionClicked Activated;
	UPROPERTY(Transient) TObjectPtr<UButton> Button;
	bool bBuilt = false;
};

/**
 * A lightweight native movable container window.  The contents remain real
 * Udemo_mapItemCellWidget instances, so a window-to-window drop travels through
 * the same ItemAuthority transaction as every other loot drop.
 */
UCLASS()
class Udemo_mapNestedContainerWindowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeWindow(
		const FString& InTitle,
		const TArray<Fdemo_mapEntityItemSlotView>& InSlots,
		Edemo_mapItemPresentationContext InContext,
		int32 InInventoryOffset,
		Fdemo_mapItemCellActivated InActivated,
		Fdemo_mapItemCellDropped InDropped,
		Fdemo_mapItemCellContextRequested InContextRequested,
		Fdemo_mapItemCellDoubleClicked InDoubleClicked,
		TFunction<void()> InClosed);
	void SetWindowPosition(const FVector2D& InPosition, int32 InZOrder);

protected:
	virtual FReply NativeOnPreviewMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

private:
	void BuildInterface();
	void RefreshSlots();
	UFUNCTION() void ClickClose();

	FString Title;
	TArray<Fdemo_mapEntityItemSlotView> Slots;
	Edemo_mapItemPresentationContext Context =
		Edemo_mapItemPresentationContext::RuntimeSpatialStorage;
	int32 InventoryOffset = 0;
	Fdemo_mapItemCellActivated Activated;
	Fdemo_mapItemCellDropped Dropped;
	Fdemo_mapItemCellContextRequested ContextRequested;
	Fdemo_mapItemCellDoubleClicked DoubleClicked;
	TFunction<void()> Closed;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UWrapBox> SlotGrid;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	bool bBuilt = false;
	bool bMoving = false;
	FVector2D GrabOffset;
};

/** Snapshot-only Container UI; actions are forwarded to the V3 Manager. */
UCLASS()
class Udemo_mapSearchContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForManager(Ademo_mapV3ProgressionManager* InManager);
	void RefreshFromSnapshot(const Fdemo_mapRuntimeContainerSnapshot& InSnapshot);
	/** Clears only uncommitted UI drag/Bundle state. ItemAuthority is untouched. */
	void ClearTransientDragState();
	int32 GetRenderedSectionCount() const { return ViewState.Sections.Num(); }
	const Fdemo_mapRuntimeContainerSnapshot& GetLastSnapshot() const { return Snapshot; }
	const Fdemo_mapRuntimeContainerResult& GetLastResult() const { return LastResult; }
	Edemo_mapRuntimeContainerSection GetSelectedSection() const { return SelectedSection; }
	const Fdemo_mapSearchContainerViewState& GetViewState() const
	{
		return ViewState;
	}
	FGuid GetSelectedEntryId() const { return SelectedEntryId; }

#if !UE_BUILD_SHIPPING
	bool AutomationSelectSection(Edemo_mapRuntimeContainerSection Section);
	bool AutomationClickEntry(
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex);
	bool AutomationClickTake();
	bool AutomationClickCancelSearch();
	bool AutomationClickClose();
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

private:
	void BuildInterface();
	void RefreshVisuals();
	void SelectSection(Edemo_mapRuntimeContainerSection Section);
	void ActivateRow(int32 VisualRowIndex);
	void HandleTakeSelected();
	void HandleItemCellDrop(
		int32 SourceSlotIndex,
		FGuid SourceItemId,
		Edemo_mapItemPresentationContext SourceContext,
		int32 TargetSlotIndex,
		Edemo_mapItemPresentationContext TargetContext);
	void HandleItemCellContextRequested(
		int32 SourceSlotIndex,
		FGuid SourceItemId,
		Edemo_mapItemPresentationContext SourceContext,
		FVector2D ScreenPosition);
	void HandleItemCellDoubleClicked(
		int32 SourceSlotIndex,
		Edemo_mapItemPresentationContext SourceContext);
	void OpenPlayerBagWindow();
	void OpenTargetBagWindow();
	void ClosePlayerBagWindow();
	void CloseTargetBagWindow();
	/** Left-click on a target item only searches unknown loot; it never takes it. */
	void HandleTargetItemCellActivated(
		int32 SourceSlotIndex,
		Edemo_mapItemPresentationContext SourceContext);
	void ExecuteContextAction(int32 ActionCode);
	void ShowSpatialBundleConfirmation();
	void DismissContextMenu();
	const Fdemo_mapRuntimeContainerEntrySnapshot* FindContainerEntry(
		Edemo_mapItemPresentationContext Context,
		int32 SlotIndex) const;
	void HandleConfirmSpatialBundleDrop();
	void HandleCancelSpatialBundleDrop();
	void HandleCancelSearch();
	void HandleClose();
	FString BuildSelectedDetailText() const;
	const Fdemo_mapSearchContainerViewRow* FindRowByEntryId(
		FGuid EntryId) const;
	const Fdemo_mapSearchContainerSectionView* FindSelectedSectionView() const;
	int32 GetSelectedVisibleRowCount() const;

	UFUNCTION() void ClickChestSection();
	UFUNCTION() void ClickEquipmentSection();
	UFUNCTION() void ClickBackpackSection();
	UFUNCTION() void ClickBodySection();
	UFUNCTION() void ClickRow00();
	UFUNCTION() void ClickRow01();
	UFUNCTION() void ClickRow02();
	UFUNCTION() void ClickRow03();
	UFUNCTION() void ClickRow04();
	UFUNCTION() void ClickRow05();
	UFUNCTION() void ClickPreviousPage();
	UFUNCTION() void ClickNextPage();
	UFUNCTION() void ClickTake();
	UFUNCTION() void ClickReturn();
	UFUNCTION() void ClickConfirmSpatialBundleDrop();
	UFUNCTION() void ClickCancelSpatialBundleDrop();
	UFUNCTION() void ClickCancelSearch();
	UFUNCTION() void ClickClose();

	TWeakObjectPtr<Ademo_mapV3ProgressionManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UBorder> ContextMenu;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InventoryText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerSummaryText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TargetSummaryText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> PlayerDragBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> TargetDragBox;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapItemCellWidget> WorldDropCell;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapNestedContainerWindowWidget> PlayerBagWindow;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapNestedContainerWindowWidget> TargetBagWindow;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmSpatialBundleDropButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelSpatialBundleDropButton;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapItemCellWidget>> PlayerDragCells;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapItemCellWidget>> TargetDragCells;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HotbarText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DiagnosticText;
	TMap<Edemo_mapRuntimeContainerSection, TObjectPtr<UButton>> SectionButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> RowButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RowTexts;
	UPROPERTY(Transient) TObjectPtr<UButton> PreviousPageButton;
	UPROPERTY(Transient) TObjectPtr<UButton> NextPageButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PageText;
	UPROPERTY(Transient) TObjectPtr<UButton> TakeButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ReturnButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelSearchButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	Fdemo_mapRuntimeContainerSnapshot Snapshot;
	Fdemo_mapSearchContainerViewState ViewState;
	Fdemo_mapRuntimeContainerResult LastResult;
	Edemo_mapRuntimeContainerSection SelectedSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 VisibleRowOffset = 0;
	FGuid SelectedEntryId;
	Fdemo_mapPlayerItemDropIntent PendingSpatialBundleDrop;
	int32 ContextSourceSlotIndex = INDEX_NONE;
	FGuid ContextSourceItemId;
	Edemo_mapItemPresentationContext ContextSourceContext =
		Edemo_mapItemPresentationContext::Warehouse;
	FVector2D ContextScreenPosition;
	bool bBuilt = false;
};
