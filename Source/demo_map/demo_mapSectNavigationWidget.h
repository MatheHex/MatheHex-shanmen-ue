#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapSectNavigationWidget.generated.h"

class Ademo_mapV3ProgressionManager;
class UButton;
class UBorder;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class Udemo_mapProfileSessionSubsystem;
class Udemo_mapSectNavigationWidget;

enum class Edemo_mapSectPage : uint8
{
	Home,
	TeleportArray,
	Warehouse,
	Town,
	ClosedBuildingOne,
	ClosedBuildingTwo,
	ClosedBuildingThree
};

struct Fdemo_mapSectBuildingDescriptor
{
	Edemo_mapSectPage Page = Edemo_mapSectPage::Home;
	FString Name;
	FString LevelLabel;
	FString StatusLabel;
	FString UpgradeLabel;
	FString ReminderLabel;
	bool bAvailable = false;
};

/**
 * Reusable, metadata-driven building tile used by the sect home page.
 * It is presentation-only and never owns gameplay or persistence state.
 */
UCLASS()
class Udemo_mapSectBuildingEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeEntry(
		Udemo_mapSectNavigationWidget* InOwner,
		const Fdemo_mapSectBuildingDescriptor& InDescriptor);

	const Fdemo_mapSectBuildingDescriptor& GetDescriptor() const
	{
		return Descriptor;
	}
	/** Read-only mounted control lookup for Slate-driven product smoke only. */
	UButton* GetEntryButtonForAutomation() const { return EntryButton; }

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildInterface();
	void RefreshText();

	UFUNCTION()
	void ClickEntry();

	TWeakObjectPtr<Udemo_mapSectNavigationWidget> Owner;
	Fdemo_mapSectBuildingDescriptor Descriptor;
	UPROPERTY(Transient) TObjectPtr<UButton> EntryButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> UpgradeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ReminderText;
	bool bBuilt = false;
};

/**
 * Single out-of-run UI root and page router for the 0.0.7 sect experience.
 * Existing Preparation/Shop/Warehouse transactions remain in their existing
 * widget and Profile Session subsystem; this widget only routes to them.
 */
UCLASS()
class Udemo_mapSectNavigationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForLifecycle(
		Udemo_mapProfileSessionSubsystem* InSession,
		Ademo_mapV3ProgressionManager* InManager);
	void RefreshFromSession();
	void HandleBuildingEntry(Edemo_mapSectPage Page);
	void ShowHomePage();
	void ShowTeleportPage();

	static TArray<Fdemo_mapSectBuildingDescriptor> BuildDefaultBuildingDescriptors();
	static FString PageTitle(Edemo_mapSectPage Page);

	Edemo_mapSectPage GetCurrentPage() const { return CurrentPage; }
	int32 GetBuildingEntryCount() const { return BuildingEntries.Num(); }
	bool IsInterfaceBuilt() const { return bBuilt; }

#if !UE_BUILD_SHIPPING
	void AutomationOpenPage(Edemo_mapSectPage Page) { ShowPage(Page); }
	void AutomationOpenWarehouseFromTeleport()
	{
		ShowPage(Edemo_mapSectPage::TeleportArray);
		ClickOpenWarehouseFromTeleport();
	}
	void AutomationOpenWarehouseFromHome()
	{
		ShowPage(Edemo_mapSectPage::Home);
		HandleBuildingEntry(Edemo_mapSectPage::Warehouse);
	}
	void AutomationReturnFromCurrentPage() { ClickReturnHome(); }
	void AutomationSetItemPresentationSnapshot(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot)
	{
		ItemPresentationSnapshot = Snapshot;
	}
	bool AutomationActivateWarehouseCell(int32 SlotIndex);
	void AutomationArmWarehouseMove()
	{
		ClickArmWarehouseMove();
	}
	/** Drives the same visible Teleport CTA that a player uses; it never calls the lifecycle transaction directly. */
	bool AutomationClickStartRunFromTeleport();
	const FString& GetTeleportFeedbackForAutomation() const { return TeleportFeedback; }
	int32 GetUnifiedItemCellCount() const
	{
		return ItemCells.Num();
	}
	FGuid GetFocusedItemId() const { return FocusedItemId; }
	void AutomationScrollToPageEnd();
	/** Returns a visible normal-navigation target; callers still must use Slate input. */
	UButton* GetBuildingEntryButtonForAutomation(Edemo_mapSectPage Page) const;
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

private:
	void BuildInterface();
	void ShowPage(Edemo_mapSectPage Page);
	void RebuildCurrentPage();
	void BuildHomePage();
	void BuildTeleportPage();
	void BuildWarehousePage();
	void BuildTownPage();
	void BuildClosedPage(Edemo_mapSectPage Page);
	void BuildUnifiedItemGrid(
		const FString& Title,
		const TArray<Fdemo_mapUnifiedItemCellView>& Cells,
		Edemo_mapItemPresentationContext Context,
		int32 ColumnCount = 5);
	void HandleUnifiedItemCell(
		int32 SlotIndex,
		const FGuid& ItemInstanceId,
		Edemo_mapItemPresentationContext Context);
	void HandleUnifiedItemContextRequested(
		int32 SlotIndex,
		FGuid ItemInstanceId,
		Edemo_mapItemPresentationContext Context,
		FVector2D ScreenPosition);
	void DismissItemContextMenu();
	void HandleUnifiedItemDrop(
		int32 SourceSlotIndex,
		FGuid SourceItemId,
		Edemo_mapItemPresentationContext SourceContext,
		int32 TargetSlotIndex,
		Edemo_mapItemPresentationContext TargetContext);
	void RebuildItemDetails();
	void BuildPreparationHotbarSection();
	void ChoosePreparationHotbarSlot(int32 SlotNumber);
	void SetHotbarFeedback(const FString& Message);
	const Fdemo_mapProfilePreparationStashRow* FindFocusedRow() const;
	void AddPageHeader(const FString& Title, const FString& Subtitle);
	UButton* AddActionButton(const FString& Label);
	void AddBodyText(const FString& Value, int32 FontSize = 18);

	UFUNCTION()
	void ClickReturnHome();
	UFUNCTION()
	void ClickOpenWarehouseFromTeleport();
	UFUNCTION()
	void ClickOpenPreparation();
	UFUNCTION()
	void ClickStartRunFromTeleport();
	UFUNCTION()
	void ClickTownUpgrade();
	UFUNCTION()
	void ClickArmWarehouseMove();
	UFUNCTION()
	void ClickContextMove();
	UFUNCTION()
	void ClickContextEquip();
	UFUNCTION()
	void ClickContextReturn();
	UFUNCTION()
	void ClickContextBindHotbar();
	UFUNCTION()
	void ClickEquipFocusedItem();
	UFUNCTION()
	void ClickReturnFocusedItem();
	UFUNCTION()
	void ClickArmHotbarPlacement();
	UFUNCTION()
	void ClickCancelHotbarPlacement();
	UFUNCTION()
	void ClickHotbarSlot1();
	UFUNCTION()
	void ClickHotbarSlot2();
	UFUNCTION()
	void ClickHotbarSlot3();
	UFUNCTION()
	void ClickHotbarSlot4();
	UFUNCTION()
	void ClickHotbarSlot5();
	UFUNCTION()
	void ClickHotbarSlot6();
	UFUNCTION()
	void ClickHotbarSlot7();
	UFUNCTION()
	void ClickHotbarSlot8();
	UFUNCTION()
	void ClickHotbarSlot9();

	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem> Session;
	TWeakObjectPtr<Ademo_mapV3ProgressionManager> LifecycleManager;
	Edemo_mapSectPage CurrentPage = Edemo_mapSectPage::Home;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> PageColumn;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> PageScroll;
	UPROPERTY(Transient) TObjectPtr<UBorder> ItemContextMenu;
	UPROPERTY(Transient) TObjectPtr<UButton> ReturnHomeButton;
	UPROPERTY(Transient) TObjectPtr<UButton> OpenPreparationButton;
	UPROPERTY(Transient) TObjectPtr<UButton> StartRunButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ItemDetailText;
	UPROPERTY(Transient) TObjectPtr<UButton> MoveItemButton;
	UPROPERTY(Transient) TObjectPtr<UButton> EquipItemButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ReturnItemButton;
	UPROPERTY(Transient) TObjectPtr<UButton> BindHotbarButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelHotbarButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HotbarFeedbackText;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> HotbarSlotButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapItemCellWidget>> ItemCells;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapSectBuildingEntryWidget>> BuildingEntries;
	Fdemo_mapProfilePreparationSnapshot ItemPresentationSnapshot;
	FGuid FocusedItemId;
	int32 FocusedWarehouseSlot = INDEX_NONE;
	Edemo_mapItemPresentationContext FocusedItemContext =
		Edemo_mapItemPresentationContext::Warehouse;
	bool bWarehouseMoveArmed = false;
	// P1 keeps the only selected map (M01) when Teleport routes through Warehouse.
	bool bReturnToTeleportAfterWarehouse = false;
	FGuid PendingHotbarItemId;
	int32 PendingHotbarReplacementSlot = INDEX_NONE;
	bool bHotbarPlacementArmed = false;
	// Keep the last committed-or-rejected preparation action visible on the
	// Warehouse page.  The context menu intentionally closes after an action,
	// so without this the player cannot tell whether a drag/equip was accepted.
	FString PreparationActionFeedback;
	FString TeleportFeedback;
	FString TownFeedback;
	bool bBuilt = false;
};
