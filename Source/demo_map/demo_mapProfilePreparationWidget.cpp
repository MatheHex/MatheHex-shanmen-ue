#include "demo_mapProfilePreparationWidget.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapUILayoutPolicy.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapPlayerController.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/InputKeySelector.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

namespace
{
	UTextBlock* PreparationText(UWidgetTree* Tree, const FString& Value, int32 Size, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetAutoWrapText(true);
		return Text;
	}

	void AddPreparationPadded(UVerticalBox* Parent, UWidget* Child, float Vertical = 4.0f)
	{
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child))
		{
			Slot->SetPadding(FMargin(10.0f, Vertical));
		}
	}
}

void Udemo_mapProfilePreparationRowClickProxy::InitializeProxy(
	Udemo_mapProfilePreparationWidget* InOwner,
	int32 InRowIndex)
{
	Owner = InOwner;
	RowIndex = InRowIndex;
}

void Udemo_mapProfilePreparationRowClickProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->HandleRowButton(RowIndex);
	}
}

void Udemo_mapProfileTradeClickProxy::InitializeProxy(
	Udemo_mapProfilePreparationWidget* InOwner,
	int32 InRowIndex,
	bool bInBuy)
{
	Owner = InOwner;
	RowIndex = InRowIndex;
	bBuy = bInBuy;
}

void Udemo_mapProfileTradeClickProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->HandleTradeButton(RowIndex, bBuy);
	}
}

void Udemo_mapInputBindingSelectProxy::InitializeProxy(
	Udemo_mapProfilePreparationWidget* InOwner,
	FName InActionId)
{
	Owner = InOwner;
	ActionId = InActionId;
}

void Udemo_mapInputBindingSelectProxy::HandleSelected(
	FInputChord SelectedKey)
{
	if (Owner)
	{
		Owner->HandleInputKeySelected(ActionId, SelectedKey.Key);
	}
}

void Udemo_mapProfilePreparationWidget::InitializeForSession(
	Udemo_mapProfileSessionSubsystem* InSession)
{
	Session = InSession;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
	RefreshInternal(true);
}

void Udemo_mapProfilePreparationWidget::InitializeForLifecycle(
	Udemo_mapProfileSessionSubsystem* InSession,
	Ademo_mapV3ProgressionManager* InManager)
{
	LifecycleManager = InManager;
	InitializeForSession(InSession);
}

void Udemo_mapProfilePreparationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_mapProfilePreparationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildInterface();
	RefreshInternal(false);
}

void Udemo_mapProfilePreparationWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}
	SetIsFocusable(true);
	bBuilt = true;

	UScrollBox* Root = WidgetTree->ConstructWidget<UScrollBox>();
	WidgetTree->RootWidget = Root;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.045f, 0.98f));
	Root->AddChild(Panel);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	AddPreparationPadded(Column, PreparationText(WidgetTree, TEXT("传送阵战备  TELEPORT LOADOUT"), 30, FLinearColor(0.95f, 0.78f, 0.20f)), 12);
	ReturnToSectButton = WidgetTree->ConstructWidget<UButton>();
	ReturnToSectButton->SetContent(
		PreparationText(
			WidgetTree,
			TEXT("返回宗门 · RETURN TO SECT"),
			18));
	ReturnToSectButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapProfilePreparationWidget::ClickReturnToSect);
	AddPreparationPadded(Column, ReturnToSectButton, 6);
	SessionText = PreparationText(WidgetTree, TEXT("Session: Uninitialized"), 19);
	RiskText = PreparationText(WidgetTree, TEXT("SAFE"), 24, FLinearColor(0.30f, 1.0f, 0.45f));
	WarningText = PreparationText(WidgetTree, FString(), 17, FLinearColor(1.0f, 0.72f, 0.22f));
	AddPreparationPadded(Column, SessionText);
	AddPreparationPadded(Column, RiskText);
	AddPreparationPadded(Column, WarningText);

	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddPreparationPadded(Column, Tabs, 8);
	WarehouseTabButton = WidgetTree->ConstructWidget<UButton>();
	WarehouseTabButton->SetContent(PreparationText(WidgetTree, TEXT("WAREHOUSE / 永久仓库"), 18));
	ShopTabButton = WidgetTree->ConstructWidget<UButton>();
	ShopTabButton->SetContent(PreparationText(WidgetTree, TEXT("SHOP / 商店"), 18));
	InputSettingsTabButton = WidgetTree->ConstructWidget<UButton>();
	InputSettingsTabButton->SetContent(PreparationText(
		WidgetTree,
		TEXT("INPUT / 改键"),
		18));
	if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(WarehouseTabButton)) TabSlot->SetPadding(FMargin(4.0f));
	if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(ShopTabButton)) TabSlot->SetPadding(FMargin(4.0f));
	if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(InputSettingsTabButton)) TabSlot->SetPadding(FMargin(4.0f));
	WarehouseTabButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickWarehouseTab);
	ShopTabButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickShopTab);
	InputSettingsTabButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapProfilePreparationWidget::ClickInputSettingsTab);

	WarehouseSection = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPreparationPadded(Column, WarehouseSection, 0);
	AddPreparationPadded(WarehouseSection, PreparationText(WidgetTree, TEXT("局外仓库物品 / WAREHOUSE ITEMS"), 23), 10);
	StashRowsBox = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPreparationPadded(WarehouseSection, StashRowsBox);

	AddPreparationPadded(WarehouseSection, PreparationText(WidgetTree, TEXT("人物轮廓与装备槽 / ENTITY LOADOUT"), 23), 10);
	EquipmentText = PreparationText(WidgetTree, FString(), 18);
	AddPreparationPadded(WarehouseSection, EquipmentText);
	UHorizontalBox* SlotActions = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddPreparationPadded(WarehouseSection, SlotActions);
	auto AddSlotButton = [this, SlotActions](const FString& Label, TObjectPtr<UButton>& Target)
	{
		Target = WidgetTree->ConstructWidget<UButton>();
		Target->SetContent(PreparationText(WidgetTree, Label, 16));
		if (UHorizontalBoxSlot* Slot = SlotActions->AddChildToHorizontalBox(Target))
		{
			Slot->SetPadding(FMargin(3.0f));
		}
	};
	AddSlotButton(TEXT("卸下兵器"), ClearWeaponButton);
	AddSlotButton(TEXT("卸下道袍"), ClearArmorButton);
	AddSlotButton(TEXT("卸下饰品"), ClearAccessoryButton);
	AddSlotButton(TEXT("卸下空间戒指"), ClearSpatialRingButton);
	AddSlotButton(TEXT("卸下空间道具"), ClearBackpackButton);
	ClearWeaponButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickClearWeapon);
	ClearArmorButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickClearArmor);
	ClearAccessoryButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickClearAccessory);
	ClearSpatialRingButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickClearSpatialRing);
	ClearBackpackButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickClearBackpack);

	AddPreparationPadded(WarehouseSection, PreparationText(WidgetTree, TEXT("基础快捷物品区（固定 6 格）"), 23), 10);
	MaterialText = PreparationText(WidgetTree, FString(), 18);
	AddPreparationPadded(WarehouseSection, MaterialText);
	AddPreparationPadded(WarehouseSection, PreparationText(WidgetTree, TEXT("空间道具储物区（动态 6—30 格）"), 23), 10);
	GridText = PreparationText(WidgetTree, FString(), 17);
	AddPreparationPadded(WarehouseSection, GridText);
	AddPreparationPadded(WarehouseSection, PreparationText(WidgetTree, TEXT("右侧局外仓库方格区（30 格投影）"), 23), 10);
	HotbarText = PreparationText(WidgetTree, FString(), 17);
	AddPreparationPadded(WarehouseSection, HotbarText);
	AddPreparationPadded(WarehouseSection, PreparationText(WidgetTree, TEXT("CONTROLS / 输入映射"), 23), 10);
	ControlsText = PreparationText(WidgetTree, FString(), 16);
	AddPreparationPadded(WarehouseSection, ControlsText);
	DetailText = PreparationText(WidgetTree, TEXT("Select a Stash row for details."), 17);
	AddPreparationPadded(WarehouseSection, DetailText, 10);

	ShopSection = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPreparationPadded(Column, ShopSection, 0);
	BalanceText = PreparationText(WidgetTree, FString(), 24, FLinearColor(0.95f, 0.78f, 0.20f));
	AddPreparationPadded(ShopSection, BalanceText, 10);
	AddPreparationPadded(ShopSection, PreparationText(WidgetTree, TEXT("PREPARATION SHOP / 整备商店"), 23), 10);
	ShopRowsBox = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPreparationPadded(ShopSection, ShopRowsBox);
	ShopGridPanel = WidgetTree->ConstructWidget<UUniformGridPanel>();
	AddPreparationPadded(ShopRowsBox, ShopGridPanel, 3);
	ShopDetailText = PreparationText(
		WidgetTree,
		TEXT("选择商品或仓库物品查看统一详情。"),
		16);
	AddPreparationPadded(ShopSection, ShopDetailText, 8);
	UHorizontalBox* ShopActions =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	AddPreparationPadded(ShopSection, ShopActions, 5);
	BuySelectedButton = WidgetTree->ConstructWidget<UButton>();
	BuySelectedButton->SetContent(PreparationText(
		WidgetTree,
		TEXT("购买所选 / BUY"),
		17));
	SellSelectedButton = WidgetTree->ConstructWidget<UButton>();
	SellSelectedButton->SetContent(PreparationText(
		WidgetTree,
		TEXT("出售所选 / SELL"),
		17));
	if (UHorizontalBoxSlot* BuySlot =
		ShopActions->AddChildToHorizontalBox(BuySelectedButton))
	{
		BuySlot->SetPadding(FMargin(3.0f));
	}
	if (UHorizontalBoxSlot* SellSlot =
		ShopActions->AddChildToHorizontalBox(SellSelectedButton))
	{
		SellSlot->SetPadding(FMargin(3.0f));
	}
	BuySelectedButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapProfilePreparationWidget::ClickBuySelected);
	SellSelectedButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapProfilePreparationWidget::ClickSellSelected);
	AddPreparationPadded(ShopSection, PreparationText(
		WidgetTree,
		TEXT("可出售仓库物品 / PLAYER SELL GRID"),
		20), 8);
	ShopSellGridPanel =
		WidgetTree->ConstructWidget<UUniformGridPanel>();
	AddPreparationPadded(ShopSection, ShopSellGridPanel, 3);
	TradeText = PreparationText(WidgetTree, FString(), 16, FLinearColor(0.65f, 0.90f, 1.0f));
	AddPreparationPadded(ShopSection, TradeText, 10);

	InputSettingsSection = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPreparationPadded(Column, InputSettingsSection, 0);
	AddPreparationPadded(InputSettingsSection, PreparationText(
		WidgetTree,
		TEXT("统一输入设置 / INPUT BINDINGS"),
		23), 10);
	AddPreparationPadded(InputSettingsSection, PreparationText(
		WidgetTree,
		TEXT("点击按键框后按下新键；冲突时稳定交换，Escape 可取消捕获。"),
		15), 4);
	InputRowsBox = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPreparationPadded(InputSettingsSection, InputRowsBox, 4);
	RestoreInputDefaultsButton = WidgetTree->ConstructWidget<UButton>();
	RestoreInputDefaultsButton->SetContent(PreparationText(
		WidgetTree,
		TEXT("恢复默认 / RESTORE DEFAULTS"),
		17));
	RestoreInputDefaultsButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapProfilePreparationWidget::ClickRestoreInputDefaults);
	AddPreparationPadded(InputSettingsSection, RestoreInputDefaultsButton, 5);
	InputDiagnosticText = PreparationText(
		WidgetTree,
		TEXT("输入设置已从注册表载入。"),
		15,
		FLinearColor(0.65f, 0.90f, 1.0f));
	AddPreparationPadded(InputSettingsSection, InputDiagnosticText, 5);
	RebuildInputRows();

	DiagnosticText = PreparationText(WidgetTree, FString(), 17, FLinearColor(1.0f, 0.35f, 0.24f));
	SettlementText = PreparationText(WidgetTree, FString(), 16, FLinearColor(0.65f, 0.78f, 1.0f));
	AddPreparationPadded(Column, DiagnosticText);
	AddPreparationPadded(Column, SettlementText);

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddPreparationPadded(Column, Actions, 12);
	auto AddActionButton = [this, Actions](const FString& Label, TObjectPtr<UButton>& Target)
	{
		Target = WidgetTree->ConstructWidget<UButton>();
		Target->SetContent(PreparationText(WidgetTree, Label, 18));
		if (UHorizontalBoxSlot* Slot = Actions->AddChildToHorizontalBox(Target))
		{
			Slot->SetPadding(FMargin(5.0f));
		}
	};
	AddActionButton(TEXT("清空战备"), ClearAllButton);
	AddActionButton(TEXT("刷新"), RefreshButton);
	AddActionButton(TEXT("Start Run / 开始出战"), StartRunButton);
	AddActionButton(TEXT("Retry Settlement"), RetrySettlementButton);
	ClearAllButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickClearAll);
	RefreshButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickRefresh);
	StartRunButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickStartRun);
	RetrySettlementButton->OnClicked.AddDynamic(this, &Udemo_mapProfilePreparationWidget::ClickRetrySettlement);
	ApplyTabVisibility();
}

void Udemo_mapProfilePreparationWidget::RefreshFromSession()
{
	RefreshInternal(true);
}

void Udemo_mapProfilePreparationWidget::RefreshInternal(bool bClearInteractionFeedback)
{
	if (bClearInteractionFeedback)
	{
		LastInteractionDiagnostic.Reset();
	}
	Fdemo_mapProfilePreparationSnapshot Snapshot = Session.IsValid()
		? Session->GetPreparationSnapshot()
		: Fdemo_mapProfilePreparationSnapshot();
	if (LifecycleManager.IsValid())
	{
		if (Udemo_mapItemSubsystem* Items = LifecycleManager->GetItemSubsystem())
		{
			Items->RefreshHotbarBindings();
			Snapshot.HotbarBindings = Items->GetHotbarBindingSnapshot();
		}
	}
	ViewState = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
	if (!LastInteractionDiagnostic.IsEmpty())
	{
		ViewState.VisibleDiagnostic = LastInteractionDiagnostic;
	}
	if (FocusedItemId.IsValid() && !ViewState.OrderedPermanentStashRows.ContainsByPredicate(
		[this](const Fdemo_mapProfilePreparationRowView& Row)
		{
			return Row.ItemInstanceId == FocusedItemId;
		}))
	{
		FocusedItemId.Invalidate();
	}
	if (!ViewState.OrderedShopRows.IsValidIndex(
		SelectedShopRowIndex))
	{
		SelectedShopRowIndex = INDEX_NONE;
	}
	if (!ViewState.OrderedPermanentStashRows.IsValidIndex(
		SelectedSellRowIndex))
	{
		SelectedSellRowIndex = INDEX_NONE;
	}
	RebuildStashRows();
	RebuildShopRows();
	RefreshText();
}

void Udemo_mapProfilePreparationWidget::RebuildStashRows()
{
	RowButtons.Reset();
	RowClickProxies.Reset();
	SellButtons.Reset();
	TradeClickProxies.Reset();
	if (!StashRowsBox || !WidgetTree)
	{
		return;
	}
	StashRowsBox->ClearChildren();
	for (int32 Index = 0; Index < ViewState.OrderedPermanentStashRows.Num(); ++Index)
	{
		const Fdemo_mapProfilePreparationRowView& Row = ViewState.OrderedPermanentStashRows[Index];
		const FString Selectable = Row.bStashOnly ? TEXT("STASH-ONLY") : (Row.bSelected ? TEXT("SELECTED") : TEXT("SELECT"));
		const FString ResourceSuffix = Row.ResourceLabel.IsEmpty()
			? FString()
			: TEXT("  ") + Row.ResourceLabel;
		const FString Line = FString::Printf(
			TEXT("%02d  %s [%s] ×%d  %s  %s%s"),
			Index + 1,
			*Row.DisplayName,
			*Row.ItemDefinitionId.ToString(),
			Row.StackCount,
			*Row.RiskLabel,
			*Selectable,
			*ResourceSuffix);
		UHorizontalBox* LineBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->SetContent(PreparationText(WidgetTree, Line, 16));
		Button->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
		Udemo_mapProfilePreparationRowClickProxy* Proxy = NewObject<Udemo_mapProfilePreparationRowClickProxy>(this);
		Proxy->InitializeProxy(this, Index);
		Button->OnClicked.AddDynamic(Proxy, &Udemo_mapProfilePreparationRowClickProxy::HandleClicked);
		RowButtons.Add(Button);
		RowClickProxies.Add(Proxy);
		if (UHorizontalBoxSlot* RowSlot = LineBox->AddChildToHorizontalBox(Button))
		{
			RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RowSlot->SetPadding(FMargin(2.0f));
		}
		UButton* SellButton = WidgetTree->ConstructWidget<UButton>();
		SellButton->SetContent(PreparationText(WidgetTree,
			FString::Printf(TEXT("SELL +%lld"), Row.TotalSellPrice), 15));
		SellButton->SetIsEnabled(Row.bCanSell);
		Udemo_mapProfileTradeClickProxy* SellProxy = NewObject<Udemo_mapProfileTradeClickProxy>(this);
		SellProxy->InitializeProxy(this, Index, false);
		SellButton->OnClicked.AddDynamic(SellProxy, &Udemo_mapProfileTradeClickProxy::HandleClicked);
		SellButtons.Add(SellButton);
		TradeClickProxies.Add(SellProxy);
		if (UHorizontalBoxSlot* SellSlot = LineBox->AddChildToHorizontalBox(SellButton)) SellSlot->SetPadding(FMargin(2.0f));
		AddPreparationPadded(StashRowsBox, LineBox, 2.0f);
	}
}

void Udemo_mapProfilePreparationWidget::RebuildShopRows()
{
	ShopButtons.Reset();
	ShopCellWidgets.Reset();
	ShopSellCellWidgets.Reset();
	if (!ShopGridPanel || !ShopSellGridPanel || !WidgetTree) return;
	ShopGridPanel->ClearChildren();
	ShopSellGridPanel->ClearChildren();
	// Compact is the fail-safe when no live viewport is available (off-screen
	// render, automation, early construction). A real viewport can opt into
	// the wider five-column layout.
	FIntPoint ViewportSize(1280, 720);
	if (GEngine && GEngine->GameViewport
		&& GEngine->GameViewport->Viewport)
	{
		ViewportSize =
			GEngine->GameViewport->Viewport->GetSizeXY();
	}
	ShopGridColumnCount =
		Fdemo_mapUILayoutPolicy::Resolve(ViewportSize)
			.UnifiedGridColumns;
	for (int32 Index = 0; Index < ViewState.OrderedShopRows.Num(); ++Index)
	{
		const Fdemo_mapProfileShopRowView& Row = ViewState.OrderedShopRows[Index];
		Fdemo_mapUnifiedItemCellView Cell;
		Cell.SlotIndex = Index;
		Cell.ItemInstanceId = Row.ItemInstanceId;
		Cell.ItemDefinitionId = Row.ItemDefinitionId;
		Cell.DisplayName = Row.DisplayName;
		Cell.IconLabel = TEXT("◇");
		if (const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(
				Row.ItemDefinitionId))
		{
			Cell.LevelLabel = Definition->Level > 0
				? FString::Printf(
					TEXT("%d阶"),
					Definition->Level)
				: TEXT("基础");
		}
		else
		{
			Cell.LevelLabel = TEXT("未知");
		}
		Cell.QualityLabel = FString::Printf(
			TEXT("%lld 灵石 · %s"),
			Row.BuyPrice,
			*Row.StockLabel);
		Cell.Quantity = 1;
		Cell.bOccupied = true;
		Cell.bSelected = SelectedShopRowIndex == Index;
		Udemo_mapItemCellWidget* CellWidget = GetOwningPlayer()
			? CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(),
				Udemo_mapItemCellWidget::StaticClass())
			: NewObject<Udemo_mapItemCellWidget>(this);
		if (!CellWidget) continue;
		CellWidget->InitializeCell(
			Cell,
			Fdemo_mapItemCellActivated::CreateUObject(
				this,
				&Udemo_mapProfilePreparationWidget::
					HandleShopCellActivated));
		ShopGridPanel->AddChildToUniformGrid(
			CellWidget,
			Index / ShopGridColumnCount,
			Index % ShopGridColumnCount);
		ShopCellWidgets.Add(CellWidget);
	}
	for (int32 Index = 0;
		Index < ViewState.OrderedPermanentStashRows.Num();
		++Index)
	{
		const Fdemo_mapProfilePreparationRowView& Row =
			ViewState.OrderedPermanentStashRows[Index];
		Fdemo_mapUnifiedItemCellView Cell;
		Cell.SlotIndex = Index;
		Cell.ItemInstanceId = Row.ItemInstanceId;
		Cell.ItemDefinitionId = Row.ItemDefinitionId;
		Cell.DisplayName = Row.DisplayName;
		Cell.IconLabel = TEXT("◇");
		Cell.LevelLabel = Row.SlotOrCategoryLabel;
		Cell.QualityLabel = Row.bCanSell
			? FString::Printf(
				TEXT("出售 +%lld"),
				Row.TotalSellPrice)
			: Row.SellDiagnostic;
		Cell.Quantity = Row.StackCount;
		Cell.bOccupied = true;
		Cell.bSelected = SelectedSellRowIndex == Index;
		Udemo_mapItemCellWidget* CellWidget = GetOwningPlayer()
			? CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(),
				Udemo_mapItemCellWidget::StaticClass())
			: NewObject<Udemo_mapItemCellWidget>(this);
		if (!CellWidget) continue;
		CellWidget->InitializeCell(
			Cell,
			Fdemo_mapItemCellActivated::CreateUObject(
				this,
				&Udemo_mapProfilePreparationWidget::
					HandleSellCellActivated));
		ShopSellGridPanel->AddChildToUniformGrid(
			CellWidget,
			Index / ShopGridColumnCount,
			Index % ShopGridColumnCount);
		ShopSellCellWidgets.Add(CellWidget);
	}
	RefreshShopDetails();
}

void Udemo_mapProfilePreparationWidget::RefreshShopDetails()
{
	FString Details =
		TEXT("选择商品或仓库物品查看统一详情。");
	bool bCanBuy = false;
	bool bCanSell = false;
	if (ViewState.OrderedShopRows.IsValidIndex(
		SelectedShopRowIndex))
	{
		const Fdemo_mapProfileShopRowView& Row =
			ViewState.OrderedShopRows[SelectedShopRowIndex];
		Details = FString::Printf(
			TEXT("%s\n%s\n品质 / 词条：%s\n购买：%lld 灵石\n出售参考：%lld 灵石\n库存：%s\n%s"),
			*Row.DisplayName,
			*Row.CategoryAndLevel,
			*Row.AffixLabel,
			Row.BuyPrice,
			Row.SellPrice,
			*Row.StockLabel,
			*Row.BuyDiagnostic);
		bCanBuy = Row.bCanBuy;
	}
	else if (ViewState.OrderedPermanentStashRows.IsValidIndex(
		SelectedSellRowIndex))
	{
		const Fdemo_mapProfilePreparationRowView& Row =
			ViewState.OrderedPermanentStashRows[SelectedSellRowIndex];
		Details = FString::Printf(
			TEXT("%s\n%s\n数量：%d\n真实出售价值：%lld 灵石\n%s"),
			*Row.DisplayName,
			*Row.SlotOrCategoryLabel,
			Row.StackCount,
			Row.TotalSellPrice,
			*Row.SellDiagnostic);
		bCanSell = Row.bCanSell;
	}
	if (ShopDetailText)
	{
		ShopDetailText->SetText(FText::FromString(Details));
	}
	if (BuySelectedButton) BuySelectedButton->SetIsEnabled(bCanBuy);
	if (SellSelectedButton) SellSelectedButton->SetIsEnabled(bCanSell);
}

FString Udemo_mapProfilePreparationWidget::GetShopDetailText() const
{
	return ShopDetailText
		? ShopDetailText->GetText().ToString()
		: FString();
}

void Udemo_mapProfilePreparationWidget::RebuildInputRows()
{
	InputSelectors.Reset();
	InputSelectProxies.Reset();
	if (!InputRowsBox || !WidgetTree) return;
	InputRowsBox->ClearChildren();
	const Fdemo_mapInputBindingSettings& Settings =
		Fdemo_mapInputBindingSettings::Get();
	for (const Fdemo_mapInputActionDefinition& Action :
		Fdemo_mapInputActionRegistry::GetExactDefaultActions())
	{
		UHorizontalBox* Row =
			WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* Label = PreparationText(
			WidgetTree,
			FString::Printf(
				TEXT("[%s] %s"),
				*Action.CategoryLabel,
				*Action.DisplayLabel),
			15);
		if (UHorizontalBoxSlot* LabelSlot =
			Row->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetPadding(FMargin(3.0f));
		}
		UInputKeySelector* Selector =
			WidgetTree->ConstructWidget<UInputKeySelector>();
		Selector->SetAllowModifierKeys(false);
		Selector->SetAllowGamepadKeys(false);
		Selector->SetEscapeKeys({ EKeys::Escape });
		Selector->SetKeySelectionText(
			FText::FromString(TEXT("按下新键…")));
		Selector->SetSelectedKey(
			FInputChord(Settings.GetKey(Action.ActionId)));
		Udemo_mapInputBindingSelectProxy* Proxy =
			NewObject<Udemo_mapInputBindingSelectProxy>(this);
		Proxy->InitializeProxy(this, Action.ActionId);
		Selector->OnKeySelected.AddDynamic(
			Proxy,
			&Udemo_mapInputBindingSelectProxy::HandleSelected);
		if (UHorizontalBoxSlot* SelectorSlot =
			Row->AddChildToHorizontalBox(Selector))
		{
			SelectorSlot->SetPadding(FMargin(3.0f));
		}
		AddPreparationPadded(InputRowsBox, Row, 2.0f);
		InputSelectors.Add(Selector);
		InputSelectProxies.Add(Proxy);
	}
	RefreshInputRows();
}

void Udemo_mapProfilePreparationWidget::RefreshInputRows()
{
	const TArray<Fdemo_mapInputActionDefinition>& Actions =
		Fdemo_mapInputActionRegistry::GetExactDefaultActions();
	const Fdemo_mapInputBindingSettings& Settings =
		Fdemo_mapInputBindingSettings::Get();
	for (int32 Index = 0;
		Index < InputSelectors.Num() && Actions.IsValidIndex(Index);
		++Index)
	{
		if (UInputKeySelector* Selector = InputSelectors[Index])
		{
			Selector->SetSelectedKey(
				FInputChord(Settings.GetKey(
					Actions[Index].ActionId)));
			Selector->SetIsEnabled(
				ViewState.bPreparationOperationsEnabled);
		}
	}
	if (InputDiagnosticText)
	{
		InputDiagnosticText->SetText(FText::FromString(
			InputBindingDiagnostic.IsEmpty()
				? TEXT("选择任意动作后按新键；冲突将自动交换。")
				: InputBindingDiagnostic));
	}
}

void Udemo_mapProfilePreparationWidget::RefreshText()
{
	if (!bBuilt)
	{
		return;
	}
	if (SessionText)
	{
		SessionText->SetText(FText::FromString(FString::Printf(
			TEXT("Session: %s   Profile: %s   Generation: %d"),
			*ViewState.SessionLabel,
			ViewState.ProfileId.IsValid() ? *ViewState.ProfileId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("—"),
			ViewState.SaveGeneration)));
	}
	if (RiskText)
	{
		RiskText->SetText(FText::FromString(ViewState.RiskLabel));
		const FLinearColor Color = ViewState.RiskPhase == Edemo_mapProfilePreparationRiskPhase::Safe
			? FLinearColor(0.30f, 1.0f, 0.45f)
			: (ViewState.RiskPhase == Edemo_mapProfilePreparationRiskPhase::WillBeAtRisk
				? FLinearColor(1.0f, 0.72f, 0.22f)
				: FLinearColor(1.0f, 0.20f, 0.12f));
		RiskText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (WarningText) WarningText->SetText(FText::FromString(ViewState.RiskWarning));
	if (BalanceText) BalanceText->SetText(FText::FromString(FString::Printf(
		TEXT("Persistent Spirit Stones: %lld"), ViewState.PersistentSpiritStones)));
	if (TradeText) TradeText->SetText(FText::FromString(ViewState.LastTradeSummary));

	auto SlotContents = [](
		const Fdemo_mapEntityItemSlotView& Cell)
	{
		return Cell.bOccupied
			? FString::Printf(
				TEXT("%s ×%d · %s"),
				*Cell.DisplayName,
				Cell.Quantity,
				*Cell.LevelAndQualityLabel)
			: FString(TEXT("— 空槽 —"));
	};
	FString EquipmentLines = TEXT("        [人物轮廓占位]\n");
	for (const Fdemo_mapEntityItemSlotView& Cell :
		ViewState.EntityLoadout.EquipmentSlots)
	{
		FString Label;
		switch (Cell.Region)
		{
		case Edemo_mapEntityLoadoutRegion::Weapon:
			Label = TEXT("兵器");
			break;
		case Edemo_mapEntityLoadoutRegion::Armor:
			Label = TEXT("道袍");
			break;
		case Edemo_mapEntityLoadoutRegion::Accessory:
			Label = FString::Printf(
				TEXT("饰品 %d"),
				Cell.SlotIndex + 1);
			break;
		case Edemo_mapEntityLoadoutRegion::SpatialRing:
			Label = TEXT("空间戒指");
			break;
		case Edemo_mapEntityLoadoutRegion::SpatialItem:
			Label = TEXT("空间道具");
			break;
		default:
			Label = TEXT("装备");
			break;
		}
		EquipmentLines += FString::Printf(
			TEXT("[%s] %s\n"),
			*Label,
			*SlotContents(Cell));
	}
	if (EquipmentText) EquipmentText->SetText(FText::FromString(EquipmentLines));

	FString Materials;
	for (const Fdemo_mapEntityItemSlotView& Cell :
		ViewState.EntityLoadout.BaseQuickItemSlots)
	{
		Materials += FString::Printf(
			TEXT("[%d] %s\n"),
			Cell.SlotIndex + 1,
			*SlotContents(Cell));
	}
	if (MaterialText)
	{
		MaterialText->SetText(FText::FromString(Materials));
	}

	FString GridLines = FString::Printf(
		TEXT("已使用：%d / 总携带：%d（基础 6 + 空间道具 %d）\n"),
		ViewState.EntityLoadout.UsedCarriedSlots,
		ViewState.EntityLoadout.TotalCarriedCapacity,
		ViewState.EntityLoadout.SpatialStorageCapacity);
	if (ViewState.EntityLoadout.SpatialStorageSlots.IsEmpty())
	{
		GridLines += TEXT("未装备空间道具；额外储物区未启用。");
	}
	else
	{
		for (const Fdemo_mapEntityItemSlotView& Cell :
			ViewState.EntityLoadout.SpatialStorageSlots)
		{
			GridLines += FString::Printf(
				TEXT("[%02d] %s%s"),
				Cell.SlotIndex + 1,
				*SlotContents(Cell),
				(Cell.SlotIndex + 1) % 3 == 0
					? TEXT("\n")
					: TEXT("    "));
		}
	}
	if (GridText) GridText->SetText(FText::FromString(GridLines));

	FString WarehouseLines;
	for (const Fdemo_mapEntityItemSlotView& Cell :
		ViewState.EntityLoadout.WarehouseSlots)
	{
		WarehouseLines += FString::Printf(
			TEXT("[%02d] %s%s"),
			Cell.SlotIndex + 1,
			*SlotContents(Cell),
			(Cell.SlotIndex + 1) % 3 == 0
				? TEXT("\n")
				: TEXT("    "));
	}
	if (HotbarText)
	{
		HotbarText->SetText(FText::FromString(WarehouseLines));
	}
	if (ControlsText)
	{
		FString Controls;
		const Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
		for (const Fdemo_mapInputActionDefinition& Action : Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			Controls += FString::Printf(
				TEXT("%s = %s\n"),
				*Action.DisplayLabel,
				*Settings.GetKey(Action.ActionId)
					.GetDisplayName().ToString());
		}
		ControlsText->SetText(FText::FromString(Controls));
	}

	FString Details = TEXT("Select a Stash row for details.");
	if (const Fdemo_mapProfilePreparationRowView* Row = ViewState.OrderedPermanentStashRows.FindByPredicate(
		[this](const Fdemo_mapProfilePreparationRowView& Candidate)
		{
			return Candidate.ItemInstanceId == FocusedItemId;
		}))
	{
		Details = FString::Printf(
			TEXT("%s\nDefinition: %s\nStackCount: %d\nResources: %s\nItemInstanceId: %s\nGridIndex: %d\nSlot / Category: %s\nRisk: %s\nSelection: %s"),
			*Row->DisplayName,
			*Row->ItemDefinitionId.ToString(),
			Row->StackCount,
			Row->ResourceLabel.IsEmpty() ? TEXT("NONE") : *Row->ResourceLabel,
			*Row->ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			Row->GridIndex,
			*Row->SlotOrCategoryLabel,
			*Row->RiskLabel,
			Row->bStashOnly ? TEXT("STASH-ONLY") : (Row->bSelected ? TEXT("SELECTED") : TEXT("AVAILABLE")));
	}
	if (DetailText) DetailText->SetText(FText::FromString(Details));
	if (DiagnosticText) DiagnosticText->SetText(FText::FromString(ViewState.VisibleDiagnostic.IsEmpty() ? TEXT("Diagnostic: —") : TEXT("Diagnostic: ") + ViewState.VisibleDiagnostic));
	if (SettlementText) SettlementText->SetText(FText::FromString(ViewState.LastSettlementSummary));
	if (StartRunButton) StartRunButton->SetIsEnabled(ViewState.bCanStartRun);
	if (RetrySettlementButton)
	{
		RetrySettlementButton->SetVisibility(ViewState.bCanRetrySettlement
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		RetrySettlementButton->SetIsEnabled(ViewState.bCanRetrySettlement);
	}
	if (ClearAllButton) ClearAllButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (ClearWeaponButton) ClearWeaponButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (ClearArmorButton) ClearArmorButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (ClearAccessoryButton) ClearAccessoryButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (ClearSpatialRingButton) ClearSpatialRingButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (ClearBackpackButton) ClearBackpackButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (RemapInteractHButton) RemapInteractHButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	if (RestoreInputDefaultsButton) RestoreInputDefaultsButton->SetIsEnabled(ViewState.bPreparationOperationsEnabled);
	RefreshInputRows();
	RefreshShopDetails();
	ApplyTabVisibility();
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfilePreparationWidget::SelectEquipment(
	FName SlotId,
	const FGuid& ItemInstanceId)
{
	const Fdemo_mapProfilePreparationSelectionResult Result = Session.IsValid()
		? Session->SetPreparationEquipment(SlotId, ItemInstanceId)
		: MakeUnavailableSelectionResult(TEXT("Preparation UI has no ProfileSession Subsystem."));
	ApplySelectionResult(Result);
	return Result;
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfilePreparationWidget::SelectMaterial(
	const FGuid& ItemInstanceId,
	bool bSelected)
{
	const Fdemo_mapProfilePreparationSelectionResult Result = Session.IsValid()
		? Session->SetPreparationMaterial(ItemInstanceId, bSelected)
		: MakeUnavailableSelectionResult(TEXT("Preparation UI has no ProfileSession Subsystem."));
	ApplySelectionResult(Result);
	return Result;
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfilePreparationWidget::ActivateStashRow(int32 RowIndex)
{
	if (!ViewState.OrderedPermanentStashRows.IsValidIndex(RowIndex))
	{
		const Fdemo_mapProfilePreparationSelectionResult Result = MakeUnavailableSelectionResult(TEXT("Preparation UI row is missing or stale."));
		ApplySelectionResult(Result);
		return Result;
	}
	const Fdemo_mapProfilePreparationRowView Row = ViewState.OrderedPermanentStashRows[RowIndex];
	FocusedItemId = Row.ItemInstanceId;
	if (!Row.CompatibleEquipmentSlotId.IsNone())
	{
		return SelectEquipment(Row.CompatibleEquipmentSlotId, Row.bSelected ? FGuid() : Row.ItemInstanceId);
	}
	return SelectMaterial(Row.ItemInstanceId, !Row.bSelected);
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfilePreparationWidget::ClearEquipmentSlot(FName SlotId)
{
	return SelectEquipment(SlotId, FGuid());
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfilePreparationWidget::ClearAllSelection()
{
	const Fdemo_mapProfilePreparationSelectionResult Result = Session.IsValid()
		? Session->ClearPreparationSelection()
		: MakeUnavailableSelectionResult(TEXT("Preparation UI has no ProfileSession Subsystem."));
	ApplySelectionResult(Result);
	return Result;
}

Fdemo_mapProfileSessionBeginResult Udemo_mapProfilePreparationWidget::RequestStartRun()
{
	const Fdemo_mapProfileSessionBeginResult Result = Session.IsValid()
		? Session->StartPreparedRun()
		: MakeUnavailableBeginResult(TEXT("Preparation UI has no ProfileSession Subsystem."));
	ApplyBeginResult(Result);
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Udemo_mapProfilePreparationWidget::RequestRetrySettlement()
{
	const Fdemo_mapProfileSessionSettlementResult Result = LifecycleManager.IsValid()
		? LifecycleManager->RetryPendingProfileSettlement()
		: (Session.IsValid()
			? Session->RetryPendingSettlement()
			: Fdemo_mapProfileSessionSettlementResult());
	ApplySettlementResult(Result);
	return Result;
}

Fdemo_mapProfileTradeResult Udemo_mapProfilePreparationWidget::RequestBuy(
	FName ShopSlotId)
{
	Fdemo_mapProfileTradeIntent Intent;
	Intent.Kind = Edemo_mapProfileTradeKind::Buy;
	Intent.ExpectedProfileId = ViewState.ProfileId;
	Intent.ExpectedSaveGeneration = ViewState.SaveGeneration;
	const Fdemo_mapProfileShopRowView* Row =
		ViewState.OrderedShopRows.FindByPredicate(
			[ShopSlotId](const Fdemo_mapProfileShopRowView& Candidate)
			{
				return Candidate.SlotId == ShopSlotId;
			});
	if (!Row)
	{
		Row = ViewState.OrderedShopRows.FindByPredicate(
			[ShopSlotId](const Fdemo_mapProfileShopRowView& Candidate)
			{
				return Candidate.ItemDefinitionId == ShopSlotId
					&& Candidate.State
						== Edemo_mapPersistentShopStockEntryState::Available;
			});
	}
	if (Row)
	{
		Intent.ItemInstanceId = Row->ItemInstanceId;
		Intent.ShopStockPolicyId = ViewState.ShopStockPolicyId;
		Intent.ShopStockGeneration = ViewState.ShopStockGeneration;
		Intent.ShopStockEventId = ViewState.ShopStockEventId;
		Intent.ShopSlotId = Row->SlotId;
		Intent.ExpectedBuyValue = Row->BuyPrice;
	}
	Fdemo_mapProfileTradeResult Result;
	if (Session.IsValid())
	{
		Result = Session->SubmitTradeIntent(Intent);
	}
	else
	{
		Result.Status = Edemo_mapProfileTradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Preparation UI has no ProfileSession Subsystem.");
	}
	ApplyTradeResult(Result);
	return Result;
}

Fdemo_mapProfileTradeResult Udemo_mapProfilePreparationWidget::RequestSell(const FGuid& ItemInstanceId)
{
	Fdemo_mapProfileTradeIntent Intent;
	Intent.Kind = Edemo_mapProfileTradeKind::Sell;
	Intent.ExpectedProfileId = ViewState.ProfileId;
	Intent.ExpectedSaveGeneration = ViewState.SaveGeneration;
	Intent.ItemInstanceId = ItemInstanceId;
	Fdemo_mapProfileTradeResult Result;
	if (Session.IsValid())
	{
		Result = Session->SubmitTradeIntent(Intent);
	}
	else
	{
		Result.Status = Edemo_mapProfileTradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Preparation UI has no ProfileSession Subsystem.");
	}
	ApplyTradeResult(Result);
	return Result;
}

void Udemo_mapProfilePreparationWidget::HandleRowButton(int32 RowIndex)
{
	ActivateStashRow(RowIndex);
}

void Udemo_mapProfilePreparationWidget::HandleTradeButton(int32 RowIndex, bool bBuy)
{
	if (bBuy)
	{
		if (ViewState.OrderedShopRows.IsValidIndex(RowIndex))
		{
			RequestBuy(ViewState.OrderedShopRows[RowIndex].SlotId);
		}
		return;
	}
	if (ViewState.OrderedPermanentStashRows.IsValidIndex(RowIndex))
	{
		RequestSell(ViewState.OrderedPermanentStashRows[RowIndex].ItemInstanceId);
	}
}

void Udemo_mapProfilePreparationWidget::HandleShopCellActivated(
	int32 RowIndex)
{
	if (!ViewState.OrderedShopRows.IsValidIndex(RowIndex))
	{
		return;
	}
	SelectedShopRowIndex = RowIndex;
	SelectedSellRowIndex = INDEX_NONE;
	RefreshShopDetails();
}

void Udemo_mapProfilePreparationWidget::HandleSellCellActivated(
	int32 RowIndex)
{
	if (!ViewState.OrderedPermanentStashRows.IsValidIndex(RowIndex))
	{
		return;
	}
	SelectedSellRowIndex = RowIndex;
	SelectedShopRowIndex = INDEX_NONE;
	FocusedItemId =
		ViewState.OrderedPermanentStashRows[RowIndex]
			.ItemInstanceId;
	RefreshShopDetails();
}

void Udemo_mapProfilePreparationWidget::HandleInputKeySelected(
	FName ActionId,
	const FKey& Key)
{
	Fdemo_mapInputBindingResult Result;
	if (Ademo_mapPlayerController* Controller =
		Cast<Ademo_mapPlayerController>(GetOwningPlayer()))
	{
		Result = Controller->ApplyInputBindingOverrideWithSwap(
			ActionId,
			Key);
	}
	else if (ViewState.bPreparationOperationsEnabled)
	{
		Result = Fdemo_mapInputBindingSettings::Get()
			.ApplyOverrideWithSwap(ActionId, Key);
	}
	else
	{
		Result.Status = Edemo_mapInputBindingStatus::LoadRejected;
		Result.Diagnostic =
			TEXT("Input settings are available only before starting a run.");
	}
	InputBindingDiagnostic = Result.Diagnostic;
	LastInteractionDiagnostic = Result.Diagnostic;
	RefreshInputRows();
	RefreshText();
}

void Udemo_mapProfilePreparationWidget::ApplySelectionResult(
	const Fdemo_mapProfilePreparationSelectionResult& Result)
{
	LastSelectionStatus = Result.Status;
	LastInteractionDiagnostic = Result.Diagnostic;
	RefreshInternal(false);
}

void Udemo_mapProfilePreparationWidget::ApplyBeginResult(
	const Fdemo_mapProfileSessionBeginResult& Result)
{
	LastBeginStatus = Result.Status;
	LastInteractionDiagnostic = Result.Diagnostic;
	RefreshInternal(false);
}

void Udemo_mapProfilePreparationWidget::ApplySettlementResult(
	const Fdemo_mapProfileSessionSettlementResult& Result)
{
	LastSettlementRetryStatus = Result.Status;
	LastInteractionDiagnostic = Result.Diagnostic;
	RefreshInternal(false);
}

void Udemo_mapProfilePreparationWidget::ApplyTradeResult(
	const Fdemo_mapProfileTradeResult& Result)
{
	LastInteractionDiagnostic = Result.Diagnostic;
	if (Result.IsCommitted())
	{
		SelectedShopRowIndex = INDEX_NONE;
		SelectedSellRowIndex = INDEX_NONE;
	}
	RefreshInternal(false);
}

void Udemo_mapProfilePreparationWidget::ApplyTabVisibility()
{
	if (WarehouseSection) WarehouseSection->SetVisibility(
		(bShopTabActive || bInputSettingsTabActive)
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	if (ShopSection) ShopSection->SetVisibility(
		bShopTabActive && !bInputSettingsTabActive
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	if (InputSettingsSection) InputSettingsSection->SetVisibility(
		bInputSettingsTabActive
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfilePreparationWidget::MakeUnavailableSelectionResult(
	const FString& Diagnostic) const
{
	Fdemo_mapProfilePreparationSelectionResult Result;
	Result.Status = Edemo_mapProfilePreparationSelectionStatus::SessionNotReady;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = Session.IsValid() ? Session->GetPreparationSnapshot() : Fdemo_mapProfilePreparationSnapshot();
	Result.Snapshot.VisibleDiagnostic = Diagnostic;
	return Result;
}

Fdemo_mapProfileSessionBeginResult Udemo_mapProfilePreparationWidget::MakeUnavailableBeginResult(
	const FString& Diagnostic) const
{
	Fdemo_mapProfileSessionBeginResult Result;
	Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = Session.IsValid() ? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	return Result;
}

void Udemo_mapProfilePreparationWidget::ClickClearWeapon() { ClearEquipmentSlot(Fdemo_mapItemIds::WeaponSlot); }
void Udemo_mapProfilePreparationWidget::ClickClearArmor() { ClearEquipmentSlot(Fdemo_mapItemIds::ArmorSlot); }
void Udemo_mapProfilePreparationWidget::ClickClearAccessory() { ClearEquipmentSlot(Fdemo_mapItemIds::AccessorySlot); }
void Udemo_mapProfilePreparationWidget::ClickClearSpatialRing() { ClearEquipmentSlot(Fdemo_mapItemIds::SpatialRingSlot); }
void Udemo_mapProfilePreparationWidget::ClickClearBackpack() { ClearEquipmentSlot(Fdemo_mapItemIds::BackpackSlot); }
void Udemo_mapProfilePreparationWidget::ClickRemapInteractH()
{
	if (Ademo_mapPlayerController* Controller = Cast<Ademo_mapPlayerController>(GetOwningPlayer()))
	{
		const Fdemo_mapInputBindingResult Result = Controller->ApplyInputBindingOverride(Fdemo_mapInputActionIds::Interact, EKeys::H);
		LastInteractionDiagnostic = Result.Diagnostic;
		RefreshInternal(false);
	}
}
void Udemo_mapProfilePreparationWidget::ClickRestoreInputDefaults()
{
	Fdemo_mapInputBindingResult Result;
	if (Ademo_mapPlayerController* Controller = Cast<Ademo_mapPlayerController>(GetOwningPlayer()))
	{
		Result = Controller->RestoreDefaultInputBindings();
	}
	else if (ViewState.bPreparationOperationsEnabled)
	{
		Result = Fdemo_mapInputBindingSettings::Get()
			.RestoreDefaults();
	}
	else
	{
		Result.Status = Edemo_mapInputBindingStatus::LoadRejected;
		Result.Diagnostic =
			TEXT("Default restore is available only before starting a run.");
	}
	InputBindingDiagnostic = Result.Diagnostic;
	LastInteractionDiagnostic = Result.Diagnostic;
	RefreshInternal(false);
}
void Udemo_mapProfilePreparationWidget::ClickClearAll() { ClearAllSelection(); }
void Udemo_mapProfilePreparationWidget::ClickRefresh() { RefreshFromSession(); }
void Udemo_mapProfilePreparationWidget::ClickStartRun()
{
	if (LifecycleManager.IsValid())
	{
		ApplyBeginResult(LifecycleManager->StartPreparedProfileRun());
	}
	else
	{
		RequestStartRun();
	}
}
void Udemo_mapProfilePreparationWidget::ClickRetrySettlement() { RequestRetrySettlement(); }
void Udemo_mapProfilePreparationWidget::RetireSettlementForPageTransition(
	const TCHAR* Reason)
{
	if (LifecycleManager.IsValid()
		&& LifecycleManager->GetSettlementWidget())
	{
		LifecycleManager->DismissSettlementPresentation(Reason);
	}
}
void Udemo_mapProfilePreparationWidget::ClickWarehouseTab()
{
	RetireSettlementForPageTransition(TEXT("PreparationWarehouse"));
	bShopTabActive = false;
	bInputSettingsTabActive = false;
	ApplyTabVisibility();
}
void Udemo_mapProfilePreparationWidget::ClickShopTab()
{
	RetireSettlementForPageTransition(TEXT("PreparationShop"));
	bShopTabActive = true;
	bInputSettingsTabActive = false;
	ApplyTabVisibility();
}
void Udemo_mapProfilePreparationWidget::ClickInputSettingsTab()
{
	RetireSettlementForPageTransition(TEXT("PreparationInputSettings"));
	bShopTabActive = false;
	bInputSettingsTabActive = true;
	RefreshInputRows();
	ApplyTabVisibility();
}
void Udemo_mapProfilePreparationWidget::ClickBuySelected()
{
	if (ViewState.OrderedShopRows.IsValidIndex(
		SelectedShopRowIndex))
	{
		RequestBuy(
			ViewState.OrderedShopRows[SelectedShopRowIndex]
				.SlotId);
	}
}
void Udemo_mapProfilePreparationWidget::ClickSellSelected()
{
	if (ViewState.OrderedPermanentStashRows.IsValidIndex(
		SelectedSellRowIndex))
	{
		RequestSell(
			ViewState.OrderedPermanentStashRows[
				SelectedSellRowIndex].ItemInstanceId);
	}
}
void Udemo_mapProfilePreparationWidget::ClickReturnToSect()
{
	if (LifecycleManager.IsValid())
	{
		LifecycleManager->ReturnToSectNavigation();
	}
}

#if !UE_BUILD_SHIPPING
bool Udemo_mapProfilePreparationWidget::AutomationClickStartRun()
{
	if (!StartRunButton || !StartRunButton->GetIsEnabled())
	{
		return false;
	}
	StartRunButton->OnClicked.Broadcast();
	return LastBeginStatus
		== Edemo_mapProfileSessionBeginStatus::CommittedAndMaterialized;
}

void Udemo_mapProfilePreparationWidget::AutomationShowShopTab()
{
	ClickShopTab();
}

void Udemo_mapProfilePreparationWidget::
AutomationShowInputSettingsTab()
{
	ClickInputSettingsTab();
}
#endif
