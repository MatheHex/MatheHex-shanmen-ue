#include "demo_mapSectNavigationWidget.h"

#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapTownProgressionRules.h"
#include "demo_mapV3ProgressionManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"

namespace
{
	UTextBlock* SectText(
		UWidgetTree* Tree,
		const FString& Value,
		int32 Size,
		const FLinearColor& Color = FLinearColor::White)
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

	void AddSectPadded(
		UVerticalBox* Parent,
		UWidget* Child,
		const FMargin& Padding = FMargin(12.0f, 6.0f))
	{
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child))
		{
			Slot->SetPadding(Padding);
		}
	}
}

void Udemo_mapSectBuildingEntryWidget::InitializeEntry(
	Udemo_mapSectNavigationWidget* InOwner,
	const Fdemo_mapSectBuildingDescriptor& InDescriptor)
{
	Owner = InOwner;
	Descriptor = InDescriptor;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
	RefreshText();
}

void Udemo_mapSectBuildingEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_mapSectBuildingEntryWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}
	bBuilt = true;
	SetIsFocusable(true);

	USizeBox* TileSize = WidgetTree->ConstructWidget<USizeBox>();
	TileSize->SetMinDesiredWidth(330.0f);
	TileSize->SetMinDesiredHeight(225.0f);
	WidgetTree->RootWidget = TileSize;

	EntryButton = WidgetTree->ConstructWidget<UButton>();
	TileSize->SetContent(EntryButton);
	EntryButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectBuildingEntryWidget::ClickEntry);

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
	Card->SetBrushColor(FLinearColor(0.055f, 0.095f, 0.13f, 0.98f));
	EntryButton->SetContent(Card);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Card->SetContent(Column);

	NameText = SectText(
		WidgetTree,
		TEXT("建筑"),
		27,
		FLinearColor(0.95f, 0.78f, 0.22f));
	LevelText = SectText(WidgetTree, TEXT("等级"), 17);
	StatusText = SectText(
		WidgetTree,
		TEXT("状态"),
		18,
		FLinearColor(0.55f, 0.88f, 1.0f));
	UpgradeText = SectText(WidgetTree, TEXT("升级"), 15, FLinearColor(0.72f, 0.76f, 0.82f));
	ReminderText = SectText(WidgetTree, FString(), 14, FLinearColor(1.0f, 0.65f, 0.2f));
	AddSectPadded(Column, NameText, FMargin(14.0f, 14.0f, 14.0f, 6.0f));
	AddSectPadded(Column, LevelText);
	AddSectPadded(Column, StatusText);
	AddSectPadded(Column, UpgradeText);
	AddSectPadded(Column, ReminderText);
}

void Udemo_mapSectBuildingEntryWidget::RefreshText()
{
	if (!bBuilt)
	{
		return;
	}
	NameText->SetText(FText::FromString(Descriptor.Name));
	LevelText->SetText(FText::FromString(Descriptor.LevelLabel));
	StatusText->SetText(FText::FromString(Descriptor.StatusLabel));
	UpgradeText->SetText(FText::FromString(Descriptor.UpgradeLabel));
	ReminderText->SetText(FText::FromString(Descriptor.ReminderLabel));
	EntryButton->SetBackgroundColor(
		Descriptor.bAvailable
			? FLinearColor(0.16f, 0.29f, 0.36f, 1.0f)
			: FLinearColor(0.13f, 0.13f, 0.15f, 1.0f));
}

void Udemo_mapSectBuildingEntryWidget::ClickEntry()
{
	if (Owner.IsValid())
	{
		Owner->HandleBuildingEntry(Descriptor.Page);
	}
}

void Udemo_mapSectNavigationWidget::InitializeForLifecycle(
	Udemo_mapProfileSessionSubsystem* InSession,
	Ademo_mapV3ProgressionManager* InManager)
{
	Session = InSession;
	LifecycleManager = InManager;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
	ShowPage(CurrentPage);
}

void Udemo_mapSectNavigationWidget::RefreshFromSession()
{
	if (CurrentPage == Edemo_mapSectPage::Home
		|| CurrentPage == Edemo_mapSectPage::Warehouse
		|| CurrentPage == Edemo_mapSectPage::TeleportArray
		|| CurrentPage == Edemo_mapSectPage::Town)
	{
		RebuildCurrentPage();
	}
}

void Udemo_mapSectNavigationWidget::HandleBuildingEntry(
	Edemo_mapSectPage Page)
{
	if (Page == Edemo_mapSectPage::Warehouse)
	{
		bReturnToTeleportAfterWarehouse = false;
		if (LifecycleManager.IsValid())
		{
			FString Feedback;
			if (LifecycleManager->OpenCodeBOutOfRaidInventory(Feedback))
			{
				UE_LOG(LogTemp, Display, TEXT("P5.OutOfRaidEntry SectWarehouse=OpenedProfileHost"));
				return;
			}
			UE_LOG(LogTemp, Warning, TEXT("P5.OutOfRaidEntry SectWarehouse=Rejected Diagnostic=%s"), *Feedback);
			PreparationActionFeedback = Feedback.IsEmpty()
				? TEXT("仓库／人物配置未能打开。") : Feedback;
			RebuildCurrentPage();
			return;
		}
	}
	ShowPage(Page);
}

void Udemo_mapSectNavigationWidget::ShowHomePage()
{
	ShowPage(Edemo_mapSectPage::Home);
}

void Udemo_mapSectNavigationWidget::ShowTeleportPage()
{
	ShowPage(Edemo_mapSectPage::TeleportArray);
}

#if !UE_BUILD_SHIPPING
UButton* Udemo_mapSectNavigationWidget::GetBuildingEntryButtonForAutomation(
	Edemo_mapSectPage Page) const
{
	for (const Udemo_mapSectBuildingEntryWidget* Entry : BuildingEntries)
	{
		if (Entry && Entry->GetDescriptor().Page == Page)
		{
			return Entry->GetEntryButtonForAutomation();
		}
	}
	return nullptr;
}

void Udemo_mapSectNavigationWidget::AutomationScrollToPageEnd()
{
	if (PageScroll)
	{
		PageScroll->SetScrollOffset(100000.0f);
		PageScroll->ScrollToEnd();
	}
}
#endif

TArray<Fdemo_mapSectBuildingDescriptor>
Udemo_mapSectNavigationWidget::BuildDefaultBuildingDescriptors()
{
	return {
		{ Edemo_mapSectPage::TeleportArray, TEXT("传送阵"), TEXT("建筑等级 1"),
			TEXT("可进入 · 地图选择与远征入口"), TEXT("当前地图：M01 荒山灵矿遗迹"), TEXT("可快捷前往仓库"), true },
		{ Edemo_mapSectPage::Warehouse, TEXT("仓库"), TEXT("建筑等级 1"),
			TEXT("可进入 · 永久仓库"), TEXT("下一等级容量 42"), TEXT(""), true },
		{ Edemo_mapSectPage::Town, TEXT("城镇"), TEXT("城镇等级由真实 Profile 保存"),
			TEXT("可进入 · 资源与升级需求概览"), TEXT("实际升级将在后续任务接入"), TEXT(""), true },
		{ Edemo_mapSectPage::ClosedBuildingOne, TEXT("占位建筑一"), TEXT("等级 —"),
			TEXT("未开放"), TEXT("升级接口：未开放"), TEXT("后续提醒接口"), false },
		{ Edemo_mapSectPage::ClosedBuildingTwo, TEXT("占位建筑二"), TEXT("等级 —"),
			TEXT("未开放"), TEXT("升级接口：未开放"), TEXT("后续提醒接口"), false },
		{ Edemo_mapSectPage::ClosedBuildingThree, TEXT("占位建筑三"), TEXT("等级 —"),
			TEXT("未开放"), TEXT("升级接口：未开放"), TEXT("后续提醒接口"), false }
	};
}

FString Udemo_mapSectNavigationWidget::PageTitle(Edemo_mapSectPage Page)
{
	switch (Page)
	{
	case Edemo_mapSectPage::Home: return TEXT("宗门主界面");
	case Edemo_mapSectPage::TeleportArray: return TEXT("传送阵");
	case Edemo_mapSectPage::Warehouse: return TEXT("仓库");
	case Edemo_mapSectPage::Town: return TEXT("城镇");
	case Edemo_mapSectPage::ClosedBuildingOne: return TEXT("占位建筑一");
	case Edemo_mapSectPage::ClosedBuildingTwo: return TEXT("占位建筑二");
	case Edemo_mapSectPage::ClosedBuildingThree: return TEXT("占位建筑三");
	default: return TEXT("宗门");
	}
}

void Udemo_mapSectNavigationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_mapSectNavigationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildInterface();
	RebuildCurrentPage();
}

FReply Udemo_mapSectNavigationWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && ItemContextMenu)
	{
		DismissItemContextMenu();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void Udemo_mapSectNavigationWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}
	bBuilt = true;
	SetIsFocusable(true);

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = RootCanvas;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.012f, 0.022f, 0.035f, 0.985f));
	RootCanvas->AddChild(Background);
	if (UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(Background->Slot))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}
	PageScroll = WidgetTree->ConstructWidget<UScrollBox>();
	Background->SetContent(PageScroll);
	PageColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	PageScroll->AddChild(PageColumn);
}

void Udemo_mapSectNavigationWidget::ShowPage(Edemo_mapSectPage Page)
{
	if (LifecycleManager.IsValid()
		&& LifecycleManager->GetSettlementWidget())
	{
		LifecycleManager->DismissSettlementPresentation(
			TEXT("SectPageTransition"));
	}
	CurrentPage = Page;
	RebuildCurrentPage();
}

void Udemo_mapSectNavigationWidget::RebuildCurrentPage()
{
	if (!PageColumn)
	{
		return;
	}
	PageColumn->ClearChildren();
	DismissItemContextMenu();
	ReturnHomeButton = nullptr;
	OpenPreparationButton = nullptr;
	ItemDetailText = nullptr;
	MoveItemButton = nullptr;
	EquipItemButton = nullptr;
	ReturnItemButton = nullptr;
	BindHotbarButton = nullptr;
	CancelHotbarButton = nullptr;
	HotbarFeedbackText = nullptr;
	HotbarSlotButtons.Reset();
	ItemCells.Reset();
	BuildingEntries.Reset();
	FocusedItemId.Invalidate();
	FocusedWarehouseSlot = INDEX_NONE;
	bWarehouseMoveArmed = false;
	PendingHotbarItemId.Invalidate();
	PendingHotbarReplacementSlot = INDEX_NONE;
	bHotbarPlacementArmed = false;

	switch (CurrentPage)
	{
	case Edemo_mapSectPage::Home:
		BuildHomePage();
		break;
	case Edemo_mapSectPage::TeleportArray:
		BuildTeleportPage();
		break;
	case Edemo_mapSectPage::Warehouse:
		BuildWarehousePage();
		break;
	case Edemo_mapSectPage::Town:
		BuildTownPage();
		break;
	default:
		BuildClosedPage(CurrentPage);
		break;
	}
}

void Udemo_mapSectNavigationWidget::BuildHomePage()
{
	AddPageHeader(
		TEXT("归山宗门 · SECT HOME"),
		TEXT("选择建筑进入对应页面。宗门路由只负责页面导航，物品、商店、仓库与存档继续使用现有权威。"));
	const Fdemo_mapProfileSessionSnapshot Snapshot = Session.IsValid()
		? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	auto CountMaterial = [&Snapshot](bool bWood)
	{
		int32 Total = 0;
		for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
		{
			const FString Id = Item.ItemDefinitionId.ToString();
			if ((bWood && Id.Contains(TEXT("SpiritWood")))
				|| (!bWood && Id.Contains(TEXT("SpiritOre")))) Total += Item.StackCount;
		}
		return Total;
	};
	AddBodyText(FString::Printf(TEXT("宗门资源概览：灵石 %lld    灵木 %d    灵矿 %d    城镇等级 %d / 5"), Snapshot.PersistentSpiritStones, CountMaterial(true), CountMaterial(false), Snapshot.TownLevel), 20);
	if (!PreparationActionFeedback.IsEmpty())
	{
		AddBodyText(PreparationActionFeedback, 17);
	}

	TArray<Fdemo_mapSectBuildingDescriptor> Descriptors =
		BuildDefaultBuildingDescriptors();
	if (Descriptors.IsValidIndex(2))
	{
		Descriptors[2].LevelLabel = FString::Printf(
			TEXT("城镇等级 %d / %d"),
			Snapshot.TownLevel,
			Fdemo_mapTownProgressionRules::MaxTownLevel);
		Descriptors[2].StatusLabel = Snapshot.TownLevel
			>= Fdemo_mapTownProgressionRules::MaxTownLevel
			? TEXT("建设完成 · 当前暂无实际数值效果")
			: TEXT("可进入 · 资源消耗与真实升级");
		Descriptors[2].UpgradeLabel = TEXT("使用灵木、灵矿与持久灵石升级");
	}
	for (int32 RowIndex = 0; RowIndex < 2; ++RowIndex)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		AddSectPadded(PageColumn, Row, FMargin(18.0f, 8.0f));
		for (int32 ColumnIndex = 0; ColumnIndex < 3; ++ColumnIndex)
		{
			const int32 DescriptorIndex = RowIndex * 3 + ColumnIndex;
			Udemo_mapSectBuildingEntryWidget* Entry = GetOwningPlayer()
				? CreateWidget<Udemo_mapSectBuildingEntryWidget>(
					GetOwningPlayer(),
					Udemo_mapSectBuildingEntryWidget::StaticClass())
				: NewObject<Udemo_mapSectBuildingEntryWidget>(this);
			if (!Entry)
			{
				continue;
			}
			Entry->InitializeEntry(this, Descriptors[DescriptorIndex]);
			BuildingEntries.Add(Entry);
			if (UHorizontalBoxSlot* EntrySlot = Row->AddChildToHorizontalBox(Entry))
			{
				EntrySlot->SetPadding(FMargin(8.0f));
				EntrySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	}
	AddBodyText(TEXT("P1 导航框架 · 正式美术、装备、消耗品与快捷栏将在后续任务中扩展。"), 15);
}

void Udemo_mapSectNavigationWidget::BuildTeleportPage()
{
	AddPageHeader(
		TEXT("传送阵 · TELEPORT ARRAY"),
		TEXT("地图选择与远征入口；M01 选择在本会话的仓库往返中保持，战备整理统一前往仓库。"));
	AddBodyText(TEXT("当前选择：M01 · 荒山灵矿遗迹"), 24);
	AddBodyText(TEXT("风险：LOW / MID / HIGH    资源：灵木 / 灵矿 / 高价值容器"), 18);
	AddBodyText(TEXT("已知敌人：普通敌人 / 精英 / M01.Boss.Main    撤离：常规 / Boss / 弃置空间道具"), 17);
	if (!TeleportFeedback.IsEmpty())
	{
		AddBodyText(TeleportFeedback, 17);
	}
	OpenPreparationButton = AddActionButton(TEXT("前往仓库整理 · OPEN WAREHOUSE"));
	OpenPreparationButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickOpenWarehouseFromTeleport);
	StartRunButton = AddActionButton(TEXT("进入远征 · START M01 RUN"));
	StartRunButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickStartRunFromTeleport);
	ReturnHomeButton = AddActionButton(TEXT("返回宗门主界面"));
	ReturnHomeButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickReturnHome);
}

void Udemo_mapSectNavigationWidget::BuildWarehousePage()
{
	AddPageHeader(
		TEXT("仓库 · WAREHOUSE"),
		TEXT("真实 30 格永久仓库；统一方格只转发输入，位置、装备与存档规则仍由 Profile Session 权威处理。"));
	Fdemo_mapWarehouseView Warehouse;
	if (Session.IsValid())
	{
		ItemPresentationSnapshot =
			Session->GetPreparationSnapshot();
	}
	Warehouse =
		Fdemo_mapItemPresentation::BuildWarehouseView(
			ItemPresentationSnapshot);
	AddBodyText(FString::Printf(
		TEXT("当前等级：%d    当前容量：%d"),
		Fdemo_mapWarehouseView::CurrentLevel,
		Fdemo_mapWarehouseView::CurrentCapacity), 22);
	AddBodyText(FString::Printf(
		TEXT("已使用：%d    剩余：%d"),
		Warehouse.UsedSlots,
		Warehouse.RemainingSlots), 20);
	AddBodyText(FString::Printf(
		TEXT("下一等级容量：%d    升级条件：%s"),
		Fdemo_mapWarehouseView::NextLevelCapacity,
		*Warehouse.NextLevelRequirement), 17);
	if (!Warehouse.Diagnostic.IsEmpty())
	{
		AddBodyText(Warehouse.Diagnostic, 17);
	}
	if (!PreparationActionFeedback.IsEmpty())
	{
		AddBodyText(PreparationActionFeedback, 17);
	}
	BuildUnifiedItemGrid(
		TEXT("永久仓库 · 点击查看详情 / 拖到战备区整理"),
		Warehouse.Slots,
		Edemo_mapItemPresentationContext::Warehouse,
		5);
	const Fdemo_mapEntityLoadoutView Preparation =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerPreparationView(
			ItemPresentationSnapshot);
	TArray<Fdemo_mapUnifiedItemCellView> EquipmentCells;
	for (int32 Index = 0; Index < Preparation.EquipmentSlots.Num(); ++Index)
	{
		Fdemo_mapUnifiedItemCellView Cell =
			Fdemo_mapItemPresentation::BuildCell(
				Preparation.EquipmentSlots[Index]);
		Cell.SlotIndex = 100 + Index;
		EquipmentCells.Add(MoveTemp(Cell));
	}
	TArray<Fdemo_mapUnifiedItemCellView> BaseQuickCells;
	for (const Fdemo_mapEntityItemSlotView& ItemSlot : Preparation.BaseQuickItemSlots)
	{
		BaseQuickCells.Add(Fdemo_mapItemPresentation::BuildCell(ItemSlot));
	}
	TArray<Fdemo_mapUnifiedItemCellView> SpatialCells;
	for (const Fdemo_mapEntityItemSlotView& ItemSlot : Preparation.SpatialStorageSlots)
	{
		SpatialCells.Add(Fdemo_mapItemPresentation::BuildCell(ItemSlot));
	}
	BuildUnifiedItemGrid(
		TEXT("人物装备 / PLAYER EQUIPMENT · 从仓库拖入；替换会原子返回旧装备"),
		EquipmentCells,
		Edemo_mapItemPresentationContext::TeleportEquipment,
		4);
	BuildUnifiedItemGrid(
		TEXT("基础快捷物品区（6 格）· 仅材料 / 消耗品"),
		BaseQuickCells,
		Edemo_mapItemPresentationContext::TeleportQuickItems,
		3);
	BuildUnifiedItemGrid(
		TEXT("空间道具区 · 容量随已装备空间道具变化"),
		SpatialCells,
		Edemo_mapItemPresentationContext::TeleportSpatialStorage,
		3);
	if (!FocusedItemId.IsValid())
	{
		if (const Fdemo_mapUnifiedItemCellView* FirstOccupied =
			Warehouse.Slots.FindByPredicate(
				[](const Fdemo_mapUnifiedItemCellView& Cell)
				{
					return Cell.bOccupied;
				}))
		{
			FocusedItemId = FirstOccupied->ItemInstanceId;
			FocusedWarehouseSlot = FirstOccupied->SlotIndex;
			FocusedItemContext =
				Edemo_mapItemPresentationContext::Warehouse;
		}
	}
	RebuildItemDetails();
	// P6: the Warehouse can be entered from the Teleport page.  Keep the
	// visible back action truthful so the player does not appear to lose the
	// selected M01 route while preparing equipment.
	ReturnHomeButton = AddActionButton(
		bReturnToTeleportAfterWarehouse
			? TEXT("返回传送阵 · RETURN TO M01")
			: TEXT("返回宗门主界面"));
	ReturnHomeButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickReturnHome);
}

void Udemo_mapSectNavigationWidget::BuildUnifiedItemGrid(
	const FString& Title,
	const TArray<Fdemo_mapUnifiedItemCellView>& Cells,
	Edemo_mapItemPresentationContext Context,
	int32 ColumnCount)
{
	AddBodyText(Title, 20);
	if (!WidgetTree || !PageColumn)
	{
		return;
	}
	UWrapBox* Grid = WidgetTree->ConstructWidget<UWrapBox>();
	Grid->SetInnerSlotPadding(FVector2D(4.0f, 4.0f));
	AddSectPadded(PageColumn, Grid, FMargin(20.0f, 5.0f));
	// This is intentionally a compact wrap grid rather than UUniformGridPanel:
	// the latter gives every column an equal share of the whole page and turns
	// a 48 px item icon into a visually sparse, full-width inventory.
	static_cast<void>(ColumnCount);
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		const Fdemo_mapUnifiedItemCellView Cell = Cells[Index];
		Udemo_mapItemCellWidget* CellWidget = GetOwningPlayer()
			? CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(),
				Udemo_mapItemCellWidget::StaticClass())
			: NewObject<Udemo_mapItemCellWidget>(this);
		if (!CellWidget) continue;
		Fdemo_mapItemCellActivated Activated;
		Activated.BindWeakLambda(
			this,
			[this, ItemId = Cell.ItemInstanceId, Context](
				int32 SlotIndex)
			{
				HandleUnifiedItemCell(
					SlotIndex,
					ItemId,
					Context);
			});
		CellWidget->InitializeCell(
			Cell,
			MoveTemp(Activated),
			Context,
			Fdemo_mapItemCellDropped::CreateUObject(
				this,
				&Udemo_mapSectNavigationWidget::HandleUnifiedItemDrop),
			Fdemo_mapItemCellContextRequested::CreateUObject(
				this,
				&Udemo_mapSectNavigationWidget::HandleUnifiedItemContextRequested));
		ItemCells.Add(CellWidget);
		Grid->AddChildToWrapBox(CellWidget);
	}
}

void Udemo_mapSectNavigationWidget::HandleUnifiedItemDrop(
	int32 SourceSlotIndex,
	FGuid SourceItemId,
	Edemo_mapItemPresentationContext SourceContext,
	int32 TargetSlotIndex,
	Edemo_mapItemPresentationContext TargetContext)
{
	const bool bWarehouseDrag =
		SourceContext == Edemo_mapItemPresentationContext::Warehouse
		&& TargetContext == Edemo_mapItemPresentationContext::Warehouse;
	if (!Session.IsValid() || SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}
	if (bWarehouseDrag)
	{
		const Fdemo_mapWarehouseMoveResult Move = Session->MoveWarehouseItem(
			SourceSlotIndex,
			TargetSlotIndex);
		PreparationActionFeedback = Move.IsSuccess()
			? FString::Printf(TEXT("仓库拖拽已提交：%s。"),
				*SourceItemId.ToString(EGuidFormats::Digits).Left(8))
			: (Move.Diagnostic.IsEmpty() ? TEXT("仓库拖拽未提交。") : Move.Diagnostic);
	}
	else
	{
		auto AreaForContext = [](Edemo_mapItemPresentationContext Context)
		{
			switch (Context)
			{
			case Edemo_mapItemPresentationContext::Warehouse:
				return Edemo_mapPlayerItemArea::Warehouse;
			case Edemo_mapItemPresentationContext::TeleportEquipment:
				return Edemo_mapPlayerItemArea::Equipment;
			case Edemo_mapItemPresentationContext::TeleportQuickItems:
				return Edemo_mapPlayerItemArea::BaseQuickItems;
			case Edemo_mapItemPresentationContext::TeleportSpatialStorage:
				return Edemo_mapPlayerItemArea::SpatialStorage;
			default:
				return Edemo_mapPlayerItemArea::Invalid;
			}
		};
		auto EquipmentForEncodedSlot = [](int32 SlotIndex)
		{
			const int32 Index = SlotIndex - 100;
			const TArray<FName>& Slots = Fdemo_mapItemDefinitions::GetEquipmentSlotIds();
			return Slots.IsValidIndex(Index) ? Slots[Index] : NAME_None;
		};
		Fdemo_mapPlayerItemDropIntent Intent;
		Intent.ExpectedAuthorityRevision =
			Session->GetPreparationSnapshot().SaveGeneration;
		Intent.ExpectedSourceItemInstanceId = SourceItemId;
		Intent.SourceArea = AreaForContext(SourceContext);
		Intent.TargetArea = AreaForContext(TargetContext);
		Intent.SourceSlotIndex = SourceSlotIndex;
		Intent.TargetSlotIndex = TargetSlotIndex;
		Intent.SourceEquipmentSlotId =
			Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment
				? EquipmentForEncodedSlot(SourceSlotIndex) : NAME_None;
		Intent.TargetEquipmentSlotId =
			Intent.TargetArea == Edemo_mapPlayerItemArea::Equipment
				? EquipmentForEncodedSlot(TargetSlotIndex) : NAME_None;
		const Fdemo_mapPlayerItemDropResult Drop =
			Session->ExecutePreparationItemDrop(Intent);
		PreparationActionFeedback = Drop.IsSuccess()
			? TEXT("战备拖拽已持久化提交。")
			: (Drop.Diagnostic.IsEmpty() ? TEXT("战备拖拽未提交。") : Drop.Diagnostic);
	}
	RebuildCurrentPage();
}

void Udemo_mapSectNavigationWidget::HandleUnifiedItemCell(
	int32 SlotIndex,
	const FGuid& ItemInstanceId,
	Edemo_mapItemPresentationContext Context)
{
	if (bWarehouseMoveArmed
		&& FocusedWarehouseSlot != INDEX_NONE
		&& (Context == Edemo_mapItemPresentationContext::Warehouse
			|| Context
				== Edemo_mapItemPresentationContext::TeleportWarehouse))
	{
		if (Session.IsValid())
		{
			Session->MoveWarehouseItem(
				FocusedWarehouseSlot,
				SlotIndex);
		}
		bWarehouseMoveArmed = false;
		RebuildCurrentPage();
		return;
	}

	FocusedItemId = ItemInstanceId;
	FocusedWarehouseSlot =
		(Context == Edemo_mapItemPresentationContext::Warehouse
			|| Context
				== Edemo_mapItemPresentationContext::TeleportWarehouse)
			? SlotIndex
			: INDEX_NONE;
	FocusedItemContext = Context;
	RebuildItemDetails();
}

void Udemo_mapSectNavigationWidget::DismissItemContextMenu()
{
	if (ItemContextMenu)
	{
		ItemContextMenu->RemoveFromParent();
		ItemContextMenu = nullptr;
	}
}

void Udemo_mapSectNavigationWidget::HandleUnifiedItemContextRequested(
	int32 SlotIndex,
	FGuid ItemInstanceId,
	Edemo_mapItemPresentationContext Context,
	FVector2D ScreenPosition)
{
	if (!RootCanvas || !ItemInstanceId.IsValid())
	{
		return;
	}
	HandleUnifiedItemCell(SlotIndex, ItemInstanceId, Context);
	DismissItemContextMenu();
	const Fdemo_mapProfilePreparationStashRow* Row = FindFocusedRow();
	if (!Row)
	{
		return;
	}
	const Fdemo_mapResolvedItemActions Actions =
		Fdemo_mapItemPresentation::ResolveActions(*Row, Context);
	ItemContextMenu = WidgetTree->ConstructWidget<UBorder>();
	ItemContextMenu->SetBrushColor(FLinearColor(0.015f, 0.035f, 0.06f, 0.985f));
	ItemContextMenu->SetPadding(FMargin(6.0f));
	RootCanvas->AddChild(ItemContextMenu);
	if (UCanvasPanelSlot* MenuSlot = Cast<UCanvasPanelSlot>(ItemContextMenu->Slot))
	{
		const FVector2D LocalPoint = GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
		MenuSlot->SetPosition(FVector2D(
			FMath::Clamp(LocalPoint.X, 8.0f, 900.0f),
			FMath::Clamp(LocalPoint.Y, 8.0f, 600.0f)));
		MenuSlot->SetAutoSize(true);
		MenuSlot->SetZOrder(20);
	}
	UVerticalBox* MenuColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	ItemContextMenu->SetContent(MenuColumn);
	auto AddContextAction = [this, MenuColumn](const FString& Label)
	{
		UButton* ActionButton = WidgetTree->ConstructWidget<UButton>();
		ActionButton->SetBackgroundColor(FLinearColor(0.08f, 0.18f, 0.26f, 1.0f));
		ActionButton->SetContent(SectText(WidgetTree, Label, 15));
		if (UVerticalBoxSlot* Slot = MenuColumn->AddChildToVerticalBox(ActionButton))
		{
			Slot->SetPadding(FMargin(1.0f));
		}
		return ActionButton;
	};
	if (Actions.Contains(Edemo_mapItemContextAction::Move))
	{
		AddContextAction(TEXT("调整位置"))->OnClicked.AddDynamic(
			this, &Udemo_mapSectNavigationWidget::ClickContextMove);
	}
	if (Actions.Contains(Edemo_mapItemContextAction::Equip))
	{
		AddContextAction(TEXT("装备"))->OnClicked.AddDynamic(
			this, &Udemo_mapSectNavigationWidget::ClickContextEquip);
	}
	if (Actions.Contains(Edemo_mapItemContextAction::Unequip)
		|| Actions.Contains(Edemo_mapItemContextAction::ReturnToWarehouse))
	{
		AddContextAction(TEXT("卸下 / 返回仓库"))->OnClicked.AddDynamic(
			this, &Udemo_mapSectNavigationWidget::ClickContextReturn);
	}
	if (Actions.Contains(Edemo_mapItemContextAction::EquipToHotbar))
	{
		AddContextAction(TEXT("设置快捷键"))->OnClicked.AddDynamic(
			this, &Udemo_mapSectNavigationWidget::ClickContextBindHotbar);
	}
	SetKeyboardFocus();
}

void Udemo_mapSectNavigationWidget::ClickContextMove()
{
	DismissItemContextMenu();
	ClickArmWarehouseMove();
}

void Udemo_mapSectNavigationWidget::ClickContextEquip()
{
	DismissItemContextMenu();
	ClickEquipFocusedItem();
}

void Udemo_mapSectNavigationWidget::ClickContextReturn()
{
	DismissItemContextMenu();
	ClickReturnFocusedItem();
}

void Udemo_mapSectNavigationWidget::ClickContextBindHotbar()
{
	DismissItemContextMenu();
	ClickArmHotbarPlacement();
}

const Fdemo_mapProfilePreparationStashRow*
Udemo_mapSectNavigationWidget::FindFocusedRow() const
{
	return FocusedItemId.IsValid()
		? ItemPresentationSnapshot.OrderedPermanentStashRows.FindByPredicate(
			[this](
				const Fdemo_mapProfilePreparationStashRow& Candidate)
			{
				return Candidate.ItemInstanceId == FocusedItemId;
			})
		: nullptr;
}

void Udemo_mapSectNavigationWidget::RebuildItemDetails()
{
	if (!ItemDetailText)
	{
		ItemDetailText = SectText(
			WidgetTree,
			TEXT("选择非空方格查看正式物品详情与当前合法操作。"),
			16,
			FLinearColor(0.82f, 0.88f, 0.96f));
		AddSectPadded(
			PageColumn,
			ItemDetailText,
			FMargin(20.0f, 10.0f));
		MoveItemButton = AddActionButton(TEXT("调整位置：再点目标格"));
		MoveItemButton->OnClicked.AddDynamic(
			this,
			&Udemo_mapSectNavigationWidget::ClickArmWarehouseMove);
		EquipItemButton = AddActionButton(TEXT("装备"));
		EquipItemButton->OnClicked.AddDynamic(
			this,
			&Udemo_mapSectNavigationWidget::ClickEquipFocusedItem);
		ReturnItemButton = AddActionButton(TEXT("卸下 / 返回仓库"));
		ReturnItemButton->OnClicked.AddDynamic(
			this,
			&Udemo_mapSectNavigationWidget::ClickReturnFocusedItem);
		BindHotbarButton = AddActionButton(
			Fdemo_mapItemPresentation::ActionLabel(
				Edemo_mapItemContextAction::EquipToHotbar));
		BindHotbarButton->OnClicked.AddDynamic(
			this,
			&Udemo_mapSectNavigationWidget::ClickArmHotbarPlacement);
	}

	const Fdemo_mapProfilePreparationStashRow* Row =
		FindFocusedRow();
	if (!Row)
	{
		ItemDetailText->SetText(FText::FromString(
			TEXT("选择非空方格查看正式物品详情与当前合法操作。")));
		MoveItemButton->SetVisibility(ESlateVisibility::Collapsed);
		EquipItemButton->SetVisibility(ESlateVisibility::Collapsed);
		ReturnItemButton->SetVisibility(ESlateVisibility::Collapsed);
		BindHotbarButton->SetVisibility(
			ESlateVisibility::Collapsed);
		return;
	}
	const Fdemo_mapUnifiedItemDetailView Detail =
		Fdemo_mapItemPresentation::BuildDetail(*Row);
	ItemDetailText->SetText(FText::FromString(FString::Printf(
		TEXT("%s [%s]\n图像：%s    等级：%s    类型：%s    品质：%s    数量：%d\n描述：%s\n基础属性：%s\n随机词条：%s\n使用效果：%s\n出售价值：%s\n允许位置：%s\n标签 / 状态：%s"),
		*Detail.DisplayName,
		*Detail.ItemDefinitionId.ToString(),
		*Detail.IconLabel,
		*Detail.LevelLabel,
		*Detail.TypeLabel,
		*Detail.QualityLabel,
		Detail.Quantity,
		*Detail.Description,
		*Detail.BaseAttributes,
		*Detail.RandomAffixes,
		*Detail.UseEffect,
		*Detail.SellValue,
		*Detail.AllowedPositions,
		*Detail.TagsAndStatus)));

	const Fdemo_mapResolvedItemActions Actions =
		Fdemo_mapItemPresentation::ResolveActions(
			*Row,
			FocusedItemContext);
	MoveItemButton->SetVisibility(
		Actions.Contains(Edemo_mapItemContextAction::Move)
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	EquipItemButton->SetVisibility(
		Actions.Contains(Edemo_mapItemContextAction::Equip)
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	ReturnItemButton->SetVisibility(
		Actions.Contains(Edemo_mapItemContextAction::Unequip)
			|| Actions.Contains(
				Edemo_mapItemContextAction::ReturnToWarehouse)
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	BindHotbarButton->SetVisibility(
		Actions.Contains(
			Edemo_mapItemContextAction::EquipToHotbar)
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
}

void Udemo_mapSectNavigationWidget::BuildPreparationHotbarSection()
{
	if (!WidgetTree || !PageColumn)
	{
		return;
	}
	AddBodyText(
		TEXT("1—9 快捷使用栏 · 绑定只引用基础快捷物品区中的真实丹药"),
		21);
	HotbarFeedbackText = SectText(
		WidgetTree,
		TEXT("选择基础快捷区中的丹药，在物品详情中点击“装备到快捷使用栏”，再选择位置。"),
		15,
		FLinearColor(0.65f, 0.82f, 1.0f));
	AddSectPadded(
		PageColumn,
		HotbarFeedbackText,
		FMargin(20.0f, 4.0f, 20.0f, 8.0f));

	const TArray<Fdemo_mapHotbarSlotView> Slots =
		Fdemo_mapItemPresentation::BuildPreparationHotbar(
			ItemPresentationSnapshot);
	const Fdemo_mapInputBindingSettings& Settings =
		Fdemo_mapInputBindingSettings::Get();
	UHorizontalBox* Row =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	AddSectPadded(PageColumn, Row, FMargin(18.0f, 4.0f));
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const Fdemo_mapHotbarSlotView& HotbarSlot = Slots[Index];
		USizeBox* SlotSize =
			WidgetTree->ConstructWidget<USizeBox>();
		SlotSize->SetMinDesiredWidth(145.0f);
		SlotSize->SetMinDesiredHeight(88.0f);
		UButton* SlotButton =
			WidgetTree->ConstructWidget<UButton>();
		SlotSize->SetContent(SlotButton);
		const FString KeyLabel = Settings.GetKey(
			Fdemo_mapInputActionRegistry::HotbarActionId(
				HotbarSlot.SlotNumber)).GetDisplayName().ToString();
		SlotButton->SetContent(SectText(
			WidgetTree,
			Fdemo_mapItemPresentation::BuildHotbarSlotLabel(
				HotbarSlot,
				KeyLabel),
			13));
		switch (HotbarSlot.SlotNumber)
		{
		case 1:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot1);
			break;
		case 2:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot2);
			break;
		case 3:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot3);
			break;
		case 4:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot4);
			break;
		case 5:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot5);
			break;
		case 6:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot6);
			break;
		case 7:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot7);
			break;
		case 8:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot8);
			break;
		case 9:
			SlotButton->OnClicked.AddDynamic(
				this,
				&Udemo_mapSectNavigationWidget::ClickHotbarSlot9);
			break;
		default:
			break;
		}
		SlotButton->SetBackgroundColor(
			HotbarSlot.State == Edemo_mapHotbarSlotState::Ready
				? FLinearColor(0.16f, 0.42f, 0.26f, 1.0f)
				: (HotbarSlot.State
						== Edemo_mapHotbarSlotState::
							InvalidBinding
					? FLinearColor(0.45f, 0.14f, 0.12f, 1.0f)
					: FLinearColor(
						0.10f,
						0.20f,
						0.28f,
						1.0f)));
		HotbarSlotButtons.Add(SlotButton);
		if (UHorizontalBoxSlot* RowSlot =
			Row->AddChildToHorizontalBox(SlotSize))
		{
			RowSlot->SetPadding(FMargin(3.0f));
			RowSlot->SetSize(
				FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	CancelHotbarButton = AddActionButton(
		TEXT("取消快捷栏位置选择"));
	CancelHotbarButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickCancelHotbarPlacement);
	CancelHotbarButton->SetVisibility(
		ESlateVisibility::Collapsed);
}

void Udemo_mapSectNavigationWidget::SetHotbarFeedback(
	const FString& Message)
{
	if (HotbarFeedbackText)
	{
		HotbarFeedbackText->SetText(
			FText::FromString(Message));
	}
}

void Udemo_mapSectNavigationWidget::ClickArmHotbarPlacement()
{
	const Fdemo_mapProfilePreparationStashRow* Row =
		FindFocusedRow();
	if (!Row
		|| !Row->bInBaseQuickItemArea
		|| Row->ItemCategoryId
			!= Fdemo_mapItemIds::ConsumableCategory)
	{
		SetHotbarFeedback(
			TEXT("只有基础快捷物品区中的有效丹药可以绑定。"));
		return;
	}
	PendingHotbarItemId = Row->ItemInstanceId;
	PendingHotbarReplacementSlot = INDEX_NONE;
	bHotbarPlacementArmed = true;
	if (CancelHotbarButton)
	{
		CancelHotbarButton->SetVisibility(
			ESlateVisibility::Visible);
	}
	SetHotbarFeedback(FString::Printf(
		TEXT("正在绑定 %s：请选择 1—9 位置。占用位置需再次确认替换，也可取消。"),
		*Row->ItemDefinitionId.ToString()));
}

void Udemo_mapSectNavigationWidget::ChoosePreparationHotbarSlot(
	int32 SlotNumber)
{
	if (!bHotbarPlacementArmed
		|| !PendingHotbarItemId.IsValid())
	{
		SetHotbarFeedback(
			TEXT("请先从物品详情选择“装备到快捷使用栏”。"));
		return;
	}
	if (!Session.IsValid()
		|| SlotNumber < 1
		|| SlotNumber
			> Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		SetHotbarFeedback(TEXT("快捷栏位置当前不可用。"));
		return;
	}

	ItemPresentationSnapshot =
		Session->GetPreparationSnapshot();
	const FGuid Existing =
		ItemPresentationSnapshot.HotbarBindings.SlotBindings
			.IsValidIndex(SlotNumber - 1)
		? ItemPresentationSnapshot.HotbarBindings
			.SlotBindings[SlotNumber - 1]
		: FGuid();
	if (Existing.IsValid()
		&& Existing != PendingHotbarItemId
		&& PendingHotbarReplacementSlot != SlotNumber)
	{
		PendingHotbarReplacementSlot = SlotNumber;
		SetHotbarFeedback(FString::Printf(
			TEXT("位置 %d 已占用。再次点击该位置确认“替换”，或点击“取消快捷栏位置选择”。"),
			SlotNumber));
		return;
	}

	const Fdemo_mapProfilePreparationSelectionResult Result =
		Session->SetPreparationHotbarSlot(
			SlotNumber,
			PendingHotbarItemId);
	if (!Result.IsAccepted())
	{
		SetHotbarFeedback(Result.Diagnostic);
		return;
	}
	RebuildCurrentPage();
}

void Udemo_mapSectNavigationWidget::ClickCancelHotbarPlacement()
{
	bHotbarPlacementArmed = false;
	PendingHotbarItemId.Invalidate();
	PendingHotbarReplacementSlot = INDEX_NONE;
	if (CancelHotbarButton)
	{
		CancelHotbarButton->SetVisibility(
			ESlateVisibility::Collapsed);
	}
	SetHotbarFeedback(
		TEXT("已取消快捷栏位置选择。"));
}

void Udemo_mapSectNavigationWidget::ClickHotbarSlot1()
{
	ChoosePreparationHotbarSlot(1);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot2()
{
	ChoosePreparationHotbarSlot(2);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot3()
{
	ChoosePreparationHotbarSlot(3);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot4()
{
	ChoosePreparationHotbarSlot(4);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot5()
{
	ChoosePreparationHotbarSlot(5);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot6()
{
	ChoosePreparationHotbarSlot(6);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot7()
{
	ChoosePreparationHotbarSlot(7);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot8()
{
	ChoosePreparationHotbarSlot(8);
}
void Udemo_mapSectNavigationWidget::ClickHotbarSlot9()
{
	ChoosePreparationHotbarSlot(9);
}

void Udemo_mapSectNavigationWidget::ClickArmWarehouseMove()
{
	bWarehouseMoveArmed =
		FocusedWarehouseSlot != INDEX_NONE
		&& FindFocusedRow() != nullptr;
	if (bWarehouseMoveArmed && ItemDetailText)
	{
		ItemDetailText->SetText(FText::FromString(
			TEXT("位置调整已激活：点击任意目标格；空格为移动，非空格为交换。")));
	}
}

void Udemo_mapSectNavigationWidget::ClickEquipFocusedItem()
{
	const Fdemo_mapProfilePreparationStashRow* Row =
		FindFocusedRow();
	if (Session.IsValid()
		&& Row
		&& !Row->CompatibleEquipmentSlotId.IsNone())
	{
		const Fdemo_mapProfilePreparationSelectionResult Result =
			Session->SetPreparationEquipment(
			Row->CompatibleEquipmentSlotId,
			Row->ItemInstanceId);
		PreparationActionFeedback = Result.IsAccepted()
			? TEXT("已装备到兼容槽位。")
			: (Result.Diagnostic.IsEmpty()
				? TEXT("装备未提交。") : Result.Diagnostic);
	}
	else
	{
		PreparationActionFeedback = TEXT("当前物品不能装备到人物槽位。");
	}
	RebuildCurrentPage();
}

void Udemo_mapSectNavigationWidget::ClickReturnFocusedItem()
{
	const Fdemo_mapProfilePreparationStashRow* Row =
		FindFocusedRow();
	if (Session.IsValid() && Row)
	{
		if (!Row->CompatibleEquipmentSlotId.IsNone())
		{
			const Fdemo_mapProfilePreparationSelectionResult Result =
				Session->SetPreparationEquipment(
				Row->CompatibleEquipmentSlotId,
				FGuid());
			PreparationActionFeedback = Result.IsAccepted()
				? TEXT("已卸下并返回仓库。")
				: (Result.Diagnostic.IsEmpty()
					? TEXT("卸下未提交。") : Result.Diagnostic);
		}
		else
		{
			const Fdemo_mapProfilePreparationSelectionResult Result =
				Session->SetPreparationMaterial(
				Row->ItemInstanceId,
				false);
			PreparationActionFeedback = Result.IsAccepted()
				? TEXT("已返回仓库。")
				: (Result.Diagnostic.IsEmpty()
					? TEXT("返回仓库未提交。") : Result.Diagnostic);
		}
	}
	else
	{
		PreparationActionFeedback = TEXT("当前没有可返回的战备物品。");
	}
	RebuildCurrentPage();
}

void Udemo_mapSectNavigationWidget::BuildTownPage()
{
	AddPageHeader(
		TEXT("城镇 · TOWN"),
		TEXT("城镇升级使用永久仓库灵木／灵矿与持久灵石；本任务不提供战斗、商店或弟子数值效果。"));
	const int32 TownLevel = Session.IsValid() ? Session->GetSnapshot().TownLevel : 0;
	const Fdemo_mapProfileSessionSnapshot Snapshot = Session.IsValid()
		? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	int32 SpiritWood = 0;
	int32 SpiritOre = 0;
	for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
	{
		const FString Id = Item.ItemDefinitionId.ToString();
		if (Id.Contains(TEXT("SpiritWood"))) SpiritWood += Item.StackCount;
		if (Id.Contains(TEXT("SpiritOre"))) SpiritOre += Item.StackCount;
	}
	const int32 ClampedTownLevel = FMath::Clamp(
		TownLevel,
		0,
		Fdemo_mapTownProgressionRules::MaxTownLevel);
	const int32 NextLevel = FMath::Min(
		ClampedTownLevel + 1,
		Fdemo_mapTownProgressionRules::MaxTownLevel);
	AddBodyText(FString::Printf(TEXT("当前城镇等级：%d / 5    下一等级：%d"), ClampedTownLevel, NextLevel), 22);
	AddBodyText(FString::Printf(TEXT("当前真实资源：灵木 %d    灵矿 %d    灵石 %lld"), SpiritWood, SpiritOre, Snapshot.PersistentSpiritStones), 18);
	Fdemo_mapTownUpgradeCost RequiredCost;
	if (Fdemo_mapTownProgressionRules::TryGetNextLevelCost(
		ClampedTownLevel,
		RequiredCost))
	{
		AddBodyText(FString::Printf(
			TEXT("升至 %d 级需求：灵木 %d（缺 %d）、灵矿 %d（缺 %d）、灵石 %lld（缺 %lld）。"),
			NextLevel,
			RequiredCost.SpiritWood,
			FMath::Max(0, RequiredCost.SpiritWood - SpiritWood),
			RequiredCost.SpiritOre,
			FMath::Max(0, RequiredCost.SpiritOre - SpiritOre),
			RequiredCost.SpiritStones,
			FMath::Max<int64>(0, RequiredCost.SpiritStones - Snapshot.PersistentSpiritStones)), 17);
		const bool bCanUpgrade = SpiritWood >= RequiredCost.SpiritWood
			&& SpiritOre >= RequiredCost.SpiritOre
			&& Snapshot.PersistentSpiritStones >= RequiredCost.SpiritStones;
		UButton* UpgradeButton = AddActionButton(
			bCanUpgrade
				? FString::Printf(TEXT("升级城镇至 %d 级 · CONSTRUCT"), NextLevel)
				: TEXT("资源不足 · 无法升级"));
		UpgradeButton->SetIsEnabled(bCanUpgrade && Session.IsValid());
		UpgradeButton->OnClicked.AddDynamic(
			this,
			&Udemo_mapSectNavigationWidget::ClickTownUpgrade);
	}
	else
	{
		AddBodyText(TEXT("当前已达 TownLevel 5；后续等级需求暂未开放。"), 17);
	}
	AddBodyText(TEXT("当前暂无实际数值效果；后续可接入宗门建设、弟子、商店与招募。"), 19);
	if (!TownFeedback.IsEmpty())
	{
		AddBodyText(TownFeedback, 17);
	}
	ReturnHomeButton = AddActionButton(TEXT("返回宗门主界面"));
	ReturnHomeButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickReturnHome);
}

void Udemo_mapSectNavigationWidget::BuildClosedPage(Edemo_mapSectPage Page)
{
	AddPageHeader(
		FString::Printf(TEXT("%s · NOT OPEN"), *PageTitle(Page)),
		TEXT("该建筑已进入正式导航结构，但本版本尚未开放实际功能。"));
	AddBodyText(TEXT("状态：未开放"), 26);
	AddBodyText(TEXT("这里不是空白按钮；后续任务可以复用同一页面路由和建筑入口元数据。"), 18);
	ReturnHomeButton = AddActionButton(TEXT("返回宗门主界面"));
	ReturnHomeButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSectNavigationWidget::ClickReturnHome);
}

void Udemo_mapSectNavigationWidget::AddPageHeader(
	const FString& Title,
	const FString& Subtitle)
{
	AddSectPadded(
		PageColumn,
		SectText(
			WidgetTree,
			Title,
			34,
			FLinearColor(0.95f, 0.78f, 0.22f)),
		FMargin(20.0f, 20.0f, 20.0f, 6.0f));
	AddSectPadded(
		PageColumn,
		SectText(
			WidgetTree,
			Subtitle,
			17,
			FLinearColor(0.68f, 0.76f, 0.86f)),
		FMargin(20.0f, 4.0f, 20.0f, 16.0f));
}

UButton* Udemo_mapSectNavigationWidget::AddActionButton(
	const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetContent(SectText(WidgetTree, Label, 20));
	AddSectPadded(PageColumn, Button, FMargin(20.0f, 8.0f));
	return Button;
}

void Udemo_mapSectNavigationWidget::AddBodyText(
	const FString& Value,
	int32 FontSize)
{
	AddSectPadded(
		PageColumn,
		SectText(WidgetTree, Value, FontSize),
		FMargin(24.0f, 7.0f));
}

void Udemo_mapSectNavigationWidget::ClickReturnHome()
{
	if (CurrentPage == Edemo_mapSectPage::Warehouse
		&& bReturnToTeleportAfterWarehouse)
	{
		bReturnToTeleportAfterWarehouse = false;
		ShowPage(Edemo_mapSectPage::TeleportArray);
		return;
	}
	ShowPage(Edemo_mapSectPage::Home);
}

void Udemo_mapSectNavigationWidget::ClickOpenWarehouseFromTeleport()
{
	bReturnToTeleportAfterWarehouse = true;
	if (LifecycleManager.IsValid())
	{
		FString Feedback;
		if (LifecycleManager->OpenCodeBOutOfRaidInventory(Feedback))
		{
			return;
		}
		TeleportFeedback = Feedback.IsEmpty()
			? TEXT("仓库／人物配置未能打开。") : Feedback;
		RebuildCurrentPage();
		return;
	}
	TeleportFeedback = TEXT("仓库／人物配置不可用：Profile 会话未初始化。");
	RebuildCurrentPage();
}

void Udemo_mapSectNavigationWidget::ClickOpenPreparation()
{
	if (LifecycleManager.IsValid())
	{
		LifecycleManager->OpenProfilePreparationFromSect(
			CurrentPage == Edemo_mapSectPage::TeleportArray
				|| (CurrentPage == Edemo_mapSectPage::Warehouse
					&& bReturnToTeleportAfterWarehouse));
	}
}

void Udemo_mapSectNavigationWidget::ClickStartRunFromTeleport()
{
	if (!LifecycleManager.IsValid())
	{
		TeleportFeedback = TEXT("远征启动不可用：战备会话未初始化。");
		RebuildCurrentPage();
		return;
	}

	// This is intentionally the existing Prepared Run authority, not a new
	// teleport-specific transaction or parallel Run entrypoint.
	const Fdemo_mapProfileSessionBeginResult Result = LifecycleManager->StartPreparedProfileRunFromSect();
	if (!Result.IsRunActive())
	{
		TeleportFeedback = Result.Diagnostic.IsEmpty()
			? TEXT("远征启动未提交。") : Result.Diagnostic;
		RebuildCurrentPage();
	}
}

void Udemo_mapSectNavigationWidget::ClickTownUpgrade()
{
	if (!Session.IsValid())
	{
		TownFeedback = TEXT("城镇升级不可用：Profile 会话未初始化。");
		RebuildCurrentPage();
		return;
	}
	const Fdemo_mapTownUpgradeResult Result = Session->RequestTownUpgrade();
	TownFeedback = Result.Diagnostic;
	if (Result.IsCommitted())
	{
		TownFeedback = FString::Printf(
			TEXT("建设完成：城镇已升至 %d 级，资源已原子扣除并保存。"),
			Result.TownLevelAfter);
	}
	RefreshFromSession();
}

#if !UE_BUILD_SHIPPING
bool Udemo_mapSectNavigationWidget::AutomationClickStartRunFromTeleport()
{
	// Automation deliberately enters through the same visible Teleport CTA rather
	// than calling the manager or Profile flow directly.  This preserves the
	// product's UI -> Sect -> lifecycle authority boundary for P4x evidence.
	ShowPage(Edemo_mapSectPage::TeleportArray);
	if (!StartRunButton || !StartRunButton->GetIsEnabled())
	{
		return false;
	}
	StartRunButton->OnClicked.Broadcast();
	return true;
}

bool Udemo_mapSectNavigationWidget::AutomationActivateWarehouseCell(
	int32 SlotIndex)
{
	if (!Session.IsValid())
	{
		const Fdemo_mapWarehouseView Warehouse =
			Fdemo_mapItemPresentation::BuildWarehouseView(
				ItemPresentationSnapshot);
		if (!Warehouse.Slots.IsValidIndex(SlotIndex))
		{
			return false;
		}
		HandleUnifiedItemCell(
			SlotIndex,
			Warehouse.Slots[SlotIndex].ItemInstanceId,
			Edemo_mapItemPresentationContext::Warehouse);
		return true;
	}
	const Fdemo_mapWarehouseView Warehouse =
		Fdemo_mapItemPresentation::BuildWarehouseView(
			Session->GetPreparationSnapshot());
	if (!Warehouse.Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}
	HandleUnifiedItemCell(
		SlotIndex,
		Warehouse.Slots[SlotIndex].ItemInstanceId,
		Edemo_mapItemPresentationContext::Warehouse);
	return true;
}
#endif
