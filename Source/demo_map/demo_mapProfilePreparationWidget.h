#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Commands/InputChord.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapProfilePreparationWidget.generated.h"

class UButton;
class UInputKeySelector;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class Udemo_mapItemCellWidget;
class Udemo_mapProfilePreparationWidget;
class Ademo_mapV3ProgressionManager;

UCLASS()
class Udemo_mapProfilePreparationRowClickProxy : public UObject
{
	GENERATED_BODY()

public:
	void InitializeProxy(Udemo_mapProfilePreparationWidget* InOwner, int32 InRowIndex);
	UFUNCTION() void HandleClicked();

private:
	UPROPERTY(Transient) TObjectPtr<Udemo_mapProfilePreparationWidget> Owner;
	int32 RowIndex = INDEX_NONE;
};

UCLASS()
class Udemo_mapProfileTradeClickProxy : public UObject
{
	GENERATED_BODY()

public:
	void InitializeProxy(Udemo_mapProfilePreparationWidget* InOwner, int32 InRowIndex, bool bInBuy);
	UFUNCTION() void HandleClicked();

private:
	UPROPERTY(Transient) TObjectPtr<Udemo_mapProfilePreparationWidget> Owner;
	int32 RowIndex = INDEX_NONE;
	bool bBuy = false;
};

UCLASS()
class Udemo_mapInputBindingSelectProxy : public UObject
{
	GENERATED_BODY()

public:
	void InitializeProxy(
		Udemo_mapProfilePreparationWidget* InOwner,
		FName InActionId);
	UFUNCTION()
	void HandleSelected(FInputChord SelectedKey);

private:
	UPROPERTY(Transient)
	TObjectPtr<Udemo_mapProfilePreparationWidget> Owner;
	FName ActionId = NAME_None;
};

/** Programmatic C++ Preparation panel. It displays value state and forwards only existing Subsystem intents. */
UCLASS()
class Udemo_mapProfilePreparationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForSession(Udemo_mapProfileSessionSubsystem* InSession);
	void InitializeForLifecycle(Udemo_mapProfileSessionSubsystem* InSession, Ademo_mapV3ProgressionManager* InManager);
	void RefreshFromSession();
	Fdemo_mapProfilePreparationSelectionResult SelectEquipment(FName SlotId, const FGuid& ItemInstanceId);
	Fdemo_mapProfilePreparationSelectionResult SelectMaterial(const FGuid& ItemInstanceId, bool bSelected);
	Fdemo_mapProfilePreparationSelectionResult ActivateStashRow(int32 RowIndex);
	Fdemo_mapProfilePreparationSelectionResult ClearEquipmentSlot(FName SlotId);
	Fdemo_mapProfilePreparationSelectionResult ClearAllSelection();
	Fdemo_mapProfileSessionBeginResult RequestStartRun();
	Fdemo_mapProfileSessionSettlementResult RequestRetrySettlement();
	/** Accepts a Shop SlotId; DefinitionId fallback is retained for migrated tests only. */
	Fdemo_mapProfileTradeResult RequestBuy(FName ShopSlotId);
	Fdemo_mapProfileTradeResult RequestSell(const FGuid& ItemInstanceId);
	void HandleRowButton(int32 RowIndex);
	void HandleTradeButton(int32 RowIndex, bool bBuy);
	void HandleShopCellActivated(int32 RowIndex);
	void HandleSellCellActivated(int32 RowIndex);
	void HandleInputKeySelected(FName ActionId, const FKey& Key);

	const Fdemo_mapProfilePreparationViewState& GetViewState() const { return ViewState; }
	FGuid GetFocusedItemId() const { return FocusedItemId; }
	const FString& GetLastInteractionDiagnostic() const { return LastInteractionDiagnostic; }
	Edemo_mapProfilePreparationSelectionStatus GetLastSelectionStatus() const { return LastSelectionStatus; }
	Edemo_mapProfileSessionBeginStatus GetLastBeginStatus() const { return LastBeginStatus; }
	Edemo_mapProfileSessionSettlementStatus GetLastSettlementRetryStatus() const { return LastSettlementRetryStatus; }
	int32 GetRowButtonCount() const { return RowButtons.Num(); }
	int32 GetShopButtonCount() const { return ShopCellWidgets.Num(); }
	int32 GetShopSellCellCount() const
	{
		return ShopSellCellWidgets.Num();
	}
	int32 GetSellButtonCount() const { return SellButtons.Num(); }
	int32 GetInputSelectorCount() const { return InputSelectors.Num(); }
	int32 GetShopGridColumnCount() const { return ShopGridColumnCount; }
	int32 GetSelectedShopRowIndex() const { return SelectedShopRowIndex; }
	int32 GetSelectedSellRowIndex() const { return SelectedSellRowIndex; }
	FString GetShopDetailText() const;
	const FString& GetInputBindingDiagnostic() const
	{
		return InputBindingDiagnostic;
	}
	int32 GetBaseQuickSlotCount() const
	{
		return ViewState.EntityLoadout.BaseQuickItemSlots.Num();
	}
	int32 GetSpatialStorageSlotCount() const
	{
		return ViewState.EntityLoadout.SpatialStorageSlots.Num();
	}
	int32 GetWarehouseGridSlotCount() const
	{
		return ViewState.EntityLoadout.WarehouseSlots.Num();
	}
	bool IsInterfaceBuilt() const { return bBuilt; }

#if !UE_BUILD_SHIPPING
	/** Broadcasts the real Start Run button event for product-path automation. */
	bool AutomationClickStartRun();
	void AutomationShowShopTab();
	void AutomationShowInputSettingsTab();
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

private:
	void RetireSettlementForPageTransition(const TCHAR* Reason);
	void BuildInterface();
	void RebuildStashRows();
	void RebuildShopRows();
	void RebuildInputRows();
	void RefreshInputRows();
	void RefreshShopDetails();
	void RefreshInternal(bool bClearInteractionFeedback);
	void ApplySelectionResult(const Fdemo_mapProfilePreparationSelectionResult& Result);
	void ApplyBeginResult(const Fdemo_mapProfileSessionBeginResult& Result);
	void ApplySettlementResult(const Fdemo_mapProfileSessionSettlementResult& Result);
	void ApplyTradeResult(const Fdemo_mapProfileTradeResult& Result);
	void RefreshText();
	void ApplyTabVisibility();
	Fdemo_mapProfilePreparationSelectionResult MakeUnavailableSelectionResult(const FString& Diagnostic) const;
	Fdemo_mapProfileSessionBeginResult MakeUnavailableBeginResult(const FString& Diagnostic) const;

	UFUNCTION() void ClickClearWeapon();
	UFUNCTION() void ClickClearArmor();
	UFUNCTION() void ClickClearAccessory();
	UFUNCTION() void ClickClearSpatialRing();
	UFUNCTION() void ClickClearBackpack();
	UFUNCTION() void ClickRemapInteractH();
	UFUNCTION() void ClickRestoreInputDefaults();
	UFUNCTION() void ClickClearAll();
	UFUNCTION() void ClickRefresh();
	UFUNCTION() void ClickStartRun();
	UFUNCTION() void ClickRetrySettlement();
	UFUNCTION() void ClickWarehouseTab();
	UFUNCTION() void ClickShopTab();
	UFUNCTION() void ClickInputSettingsTab();
	UFUNCTION() void ClickBuySelected();
	UFUNCTION() void ClickSellSelected();
	UFUNCTION() void ClickReturnToSect();

	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem> Session;
	TWeakObjectPtr<Ademo_mapV3ProgressionManager> LifecycleManager;
	Fdemo_mapProfilePreparationViewState ViewState;
	FGuid FocusedItemId;
	FString LastInteractionDiagnostic;
	Edemo_mapProfilePreparationSelectionStatus LastSelectionStatus = Edemo_mapProfilePreparationSelectionStatus::SessionNotReady;
	Edemo_mapProfileSessionBeginStatus LastBeginStatus = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
	Edemo_mapProfileSessionSettlementStatus LastSettlementRetryStatus = Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> RowButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapProfilePreparationRowClickProxy>> RowClickProxies;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ShopButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> SellButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapProfileTradeClickProxy>> TradeClickProxies;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapItemCellWidget>> ShopCellWidgets;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapItemCellWidget>> ShopSellCellWidgets;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputKeySelector>> InputSelectors;
	UPROPERTY(Transient) TArray<TObjectPtr<Udemo_mapInputBindingSelectProxy>> InputSelectProxies;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StashRowsBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ShopRowsBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> InputRowsBox;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> ShopGridPanel;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> ShopSellGridPanel;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> WarehouseSection;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ShopSection;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> InputSettingsSection;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SessionText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RiskText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WarningText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquipmentText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MaterialText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GridText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HotbarText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ControlsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DiagnosticText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SettlementText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BalanceText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TradeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ShopDetailText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InputDiagnosticText;
	UPROPERTY(Transient) TObjectPtr<UButton> ClearWeaponButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ClearArmorButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ClearAccessoryButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ClearSpatialRingButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ClearBackpackButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RemapInteractHButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RestoreInputDefaultsButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ClearAllButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RefreshButton;
	UPROPERTY(Transient) TObjectPtr<UButton> StartRunButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RetrySettlementButton;
	UPROPERTY(Transient) TObjectPtr<UButton> WarehouseTabButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ShopTabButton;
	UPROPERTY(Transient) TObjectPtr<UButton> InputSettingsTabButton;
	UPROPERTY(Transient) TObjectPtr<UButton> BuySelectedButton;
	UPROPERTY(Transient) TObjectPtr<UButton> SellSelectedButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ReturnToSectButton;
	bool bShopTabActive = false;
	bool bInputSettingsTabActive = false;
	int32 SelectedShopRowIndex = INDEX_NONE;
	int32 SelectedSellRowIndex = INDEX_NONE;
	int32 ShopGridColumnCount = 5;
	FString InputBindingDiagnostic;
	bool bBuilt = false;
};
