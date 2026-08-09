#include "demo_mapInventoryWidget.h"

#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapV3ProgressionManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

namespace
{
	UTextBlock* RuntimeText(
		UWidgetTree* Tree,
		const FString& Value,
		int32 Size = 16)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		return Text;
	}

	void AddPadded(
		UVerticalBox* Box,
		UWidget* Widget,
		const FMargin& Padding = FMargin(3.0f))
	{
		if (UVerticalBoxSlot* Slot =
			Box->AddChildToVerticalBox(Widget))
		{
			Slot->SetPadding(Padding);
		}
	}

	FName EquipmentSlotForRegion(
		Edemo_mapEntityLoadoutRegion Region)
	{
		switch (Region)
		{
		case Edemo_mapEntityLoadoutRegion::Weapon:
			return Fdemo_mapItemIds::WeaponSlot;
		case Edemo_mapEntityLoadoutRegion::Armor:
			return Fdemo_mapItemIds::ArmorSlot;
		case Edemo_mapEntityLoadoutRegion::Accessory:
			return Fdemo_mapItemIds::AccessorySlot;
		case Edemo_mapEntityLoadoutRegion::SpatialRing:
			return Fdemo_mapItemIds::SpatialRingSlot;
		case Edemo_mapEntityLoadoutRegion::SpatialItem:
			return Fdemo_mapItemIds::BackpackSlot;
		default:
			return NAME_None;
		}
	}

	FString EquipmentRegionLabel(
		Edemo_mapEntityLoadoutRegion Region)
	{
		switch (Region)
		{
		case Edemo_mapEntityLoadoutRegion::Weapon:
			return TEXT("武器");
		case Edemo_mapEntityLoadoutRegion::Armor:
			return TEXT("防具");
		case Edemo_mapEntityLoadoutRegion::Accessory:
			return TEXT("饰品");
		case Edemo_mapEntityLoadoutRegion::SpatialRing:
			return TEXT("空间戒指");
		case Edemo_mapEntityLoadoutRegion::SpatialItem:
			return TEXT("空间道具");
		default:
			return TEXT("装备");
		}
	}
}

void Udemo_mapInventoryWidget::InitializeForManager(
	Ademo_mapV3ProgressionManager* InManager)
{
	Manager = InManager;
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::ClearTransientDragState()
{
	PendingSpatialBundleDrop = Fdemo_mapPlayerItemDropIntent();
	if (ConfirmSpatialBundleDropButton)
	{
		ConfirmSpatialBundleDropButton->SetVisibility(
			ESlateVisibility::Collapsed);
	}
	if (CancelSpatialBundleDropButton)
	{
		CancelSpatialBundleDropButton->SetVisibility(
			ESlateVisibility::Collapsed);
	}
}

void Udemo_mapInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_mapInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildInterface();
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::BuildInterface()
{
	if (!WidgetTree || bBuilt)
	{
		return;
	}
	bBuilt = true;
	UCanvasPanel* Canvas =
		WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(
		FLinearColor(0.018f, 0.028f, 0.047f, 0.985f));
	Panel->SetPadding(FMargin(12.0f));
	Canvas->AddChild(Panel);
	if (UCanvasPanelSlot* CanvasSlot =
		Cast<UCanvasPanelSlot>(Panel->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.025f, 0.025f, 0.975f, 0.975f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	UHorizontalBox* Columns =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	Panel->SetContent(Columns);

	UVerticalBox* LoadoutColumn =
		WidgetTree->ConstructWidget<UVerticalBox>();
	if (UHorizontalBoxSlot* ColumnSlot =
		Columns->AddChildToHorizontalBox(LoadoutColumn))
	{
		ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ColumnSlot->SetPadding(FMargin(8.0f));
	}
	InventoryHeaderText = RuntimeText(
		WidgetTree,
		TEXT("局内物品 / RUNTIME INVENTORY"),
		24);
	AddPadded(LoadoutColumn, InventoryHeaderText, FMargin(4.0f));
	CapacityText = RuntimeText(
		WidgetTree,
		TEXT("权威容量读取中"),
		14);
	CapacityText->SetColorAndOpacity(
		FSlateColor(FLinearColor(0.45f, 0.85f, 1.0f)));
	AddPadded(LoadoutColumn, CapacityText);

	AddPadded(
		LoadoutColumn,
		RuntimeText(
			WidgetTree,
			TEXT("人物装备 / EQUIPMENT"),
			19),
		FMargin(3.0f, 8.0f, 3.0f, 3.0f));
	EquipmentPanel = WidgetTree->ConstructWidget<UWrapBox>();
	EquipmentPanel->SetInnerSlotPadding(FVector2D(5.0f, 5.0f));
	AddPadded(LoadoutColumn, EquipmentPanel);

	AddPadded(
		LoadoutColumn,
		RuntimeText(
			WidgetTree,
			TEXT("基础快捷区 / FIXED BASE QUICK (6)"),
			19),
		FMargin(3.0f, 8.0f, 3.0f, 3.0f));
	BaseQuickPanel =
		WidgetTree->ConstructWidget<UUniformGridPanel>();
	BaseQuickPanel->SetSlotPadding(FMargin(3.0f));
	AddPadded(LoadoutColumn, BaseQuickPanel);

	AddPadded(
		LoadoutColumn,
		RuntimeText(
			WidgetTree,
			TEXT("空间储物区 / SPATIAL STORAGE"),
			19),
		FMargin(3.0f, 8.0f, 3.0f, 3.0f));
	UScrollBox* SpatialScroll =
		WidgetTree->ConstructWidget<UScrollBox>();
	SpatialStoragePanel =
		WidgetTree->ConstructWidget<UUniformGridPanel>();
	SpatialStoragePanel->SetSlotPadding(FMargin(3.0f));
	SpatialScroll->AddChild(SpatialStoragePanel);
	if (UVerticalBoxSlot* ScrollSlot =
		LoadoutColumn->AddChildToVerticalBox(SpatialScroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetPadding(FMargin(3.0f));
	}

	UVerticalBox* DetailColumn =
		WidgetTree->ConstructWidget<UVerticalBox>();
	if (UHorizontalBoxSlot* ColumnSlot =
		Columns->AddChildToHorizontalBox(DetailColumn))
	{
		ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ColumnSlot->SetPadding(FMargin(8.0f));
	}
	AddPadded(
		DetailColumn,
		RuntimeText(
			WidgetTree,
			TEXT("物品详情 / UNIFIED ITEM DETAILS"),
			22));
	DetailsText = RuntimeText(
		WidgetTree,
		TEXT("选择装备或携带物品。"),
		16);
	DetailsText->SetAutoWrapText(true);
	AddPadded(
		DetailColumn,
		DetailsText,
		FMargin(4.0f, 6.0f, 4.0f, 10.0f));

	AddPadded(
		DetailColumn,
		RuntimeText(
			WidgetTree,
			TEXT("快捷绑定 / HOTBAR 1–9"),
			20));
	UHorizontalBox* Hotbar =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	AddPadded(DetailColumn, Hotbar);
	for (int32 SlotNumber = 1; SlotNumber <= 9; ++SlotNumber)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* Text = RuntimeText(
			WidgetTree,
			FString::Printf(TEXT("[%d]\n空"), SlotNumber),
			12);
		Button->SetContent(Text);
		if (UHorizontalBoxSlot* HotbarSlot =
			Hotbar->AddChildToHorizontalBox(Button))
		{
			HotbarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HotbarSlot->SetPadding(FMargin(2.0f));
		}
		HotbarButtons.Add(Button);
		HotbarTexts.Add(Text);
	}
	HotbarButtons[0]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar1);
	HotbarButtons[1]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar2);
	HotbarButtons[2]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar3);
	HotbarButtons[3]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar4);
	HotbarButtons[4]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar5);
	HotbarButtons[5]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar6);
	HotbarButtons[6]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar7);
	HotbarButtons[7]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar8);
	HotbarButtons[8]->OnClicked.AddDynamic(this, &Udemo_mapInventoryWidget::ClickHotbar9);

	UHorizontalBox* Actions =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	AddPadded(
		DetailColumn,
		Actions,
		FMargin(3.0f, 12.0f, 3.0f, 3.0f));
	auto MakeAction =
		[this, Actions](
			const FString& Label,
			TObjectPtr<UButton>& Target)
		{
			Target = WidgetTree->ConstructWidget<UButton>();
			Target->SetContent(RuntimeText(WidgetTree, Label, 15));
			if (UHorizontalBoxSlot* ActionSlot =
				Actions->AddChildToHorizontalBox(Target))
			{
				ActionSlot->SetPadding(FMargin(3.0f));
			}
		};
	MakeAction(TEXT("装备"), EquipButton);
	MakeAction(TEXT("卸下"), UnequipButton);
	MakeAction(TEXT("换区"), MoveButton);
	MakeAction(TEXT("使用"), UseButton);
	MakeAction(TEXT("丢弃"), DropButton);
	MakeAction(TEXT("关闭"), CloseButton);
	EquipButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickEquip);
	UnequipButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickUnequip);
	MoveButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickMove);
	UseButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickUse);
	DropButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickDrop);
	CloseButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickClose);

	AddPadded(
		DetailColumn,
		RuntimeText(
			WidgetTree,
			TEXT("局内世界丢弃 / DRAG TO WORLD"),
			18),
		FMargin(3.0f, 10.0f, 3.0f, 2.0f));
	Fdemo_mapUnifiedItemCellView WorldDropView;
	WorldDropView.SlotIndex = INDEX_NONE;
	WorldDropView.DisplayName = TEXT("拖到这里丢弃");
	WorldDropView.IconLabel = TEXT("↓");
	WorldDropView.LevelLabel = TEXT("安全落点");
	WorldDropView.QualityLabel = TEXT("丢弃目标");
	WorldDropView.bDragTarget = true;
	WorldDropCell = CreateWidget<Udemo_mapItemCellWidget>(
		GetOwningPlayer(),
		Udemo_mapItemCellWidget::StaticClass());
	WorldDropCell->InitializeCell(
		WorldDropView,
		Fdemo_mapItemCellActivated(),
		Edemo_mapItemPresentationContext::RuntimeWorldDrop,
		Fdemo_mapItemCellDropped::CreateUObject(
			this,
			&Udemo_mapInventoryWidget::HandleItemDrop));
	AddPadded(DetailColumn, WorldDropCell, FMargin(3.0f));

	UHorizontalBox* BundleActions =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	ConfirmSpatialBundleDropButton =
		WidgetTree->ConstructWidget<UButton>();
	ConfirmSpatialBundleDropButton->SetContent(RuntimeText(
		WidgetTree,
		TEXT("确认丢弃空间 Bundle"),
		14));
	CancelSpatialBundleDropButton =
		WidgetTree->ConstructWidget<UButton>();
	CancelSpatialBundleDropButton->SetContent(RuntimeText(
		WidgetTree,
		TEXT("取消 Bundle"),
		14));
	BundleActions->AddChildToHorizontalBox(ConfirmSpatialBundleDropButton);
	BundleActions->AddChildToHorizontalBox(CancelSpatialBundleDropButton);
	ConfirmSpatialBundleDropButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickConfirmSpatialBundleDrop);
	CancelSpatialBundleDropButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapInventoryWidget::ClickCancelSpatialBundleDrop);
	ConfirmSpatialBundleDropButton->SetVisibility(ESlateVisibility::Collapsed);
	CancelSpatialBundleDropButton->SetVisibility(ESlateVisibility::Collapsed);
	AddPadded(DetailColumn, BundleActions, FMargin(3.0f));

	FeedbackText = RuntimeText(
		WidgetTree,
		TEXT("选择物品；快捷栏占用时需再次点击确认覆盖。"),
		15);
	FeedbackText->SetAutoWrapText(true);
	AddPadded(
		DetailColumn,
		FeedbackText,
		FMargin(4.0f, 10.0f, 4.0f, 4.0f));
}

void Udemo_mapInventoryWidget::RefreshFromAuthority()
{
	if (!bBuilt || !Manager.IsValid()
		|| !Manager->GetItemSubsystem())
	{
		return;
	}
	const Udemo_mapItemSubsystem* Items =
		Manager->GetItemSubsystem();
	const Fdemo_mapItemAuthority& Authority =
		Items->GetAuthority();
	const Fdemo_mapEntityLoadoutView View =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerRuntimeView(
			Authority);
	RuntimeSlotCount = View.TotalCarriedCapacity;
	if (InventoryHeaderText)
	{
		const FString Key =
			Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Inventory)
				.GetDisplayName().ToString();
		InventoryHeaderText->SetText(FText::FromString(
			FString::Printf(
				TEXT("局内物品 / RUNTIME INVENTORY  [%s 关闭]"),
				*Key)));
	}
	if (CloseButton)
	{
		if (UTextBlock* CloseLabel =
			Cast<UTextBlock>(CloseButton->GetContent()))
		{
			CloseLabel->SetText(FText::FromString(FString::Printf(
				TEXT("关闭 [%s]"),
				*Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::Back)
					.GetDisplayName().ToString())));
		}
	}
	if (CapacityText)
	{
		CapacityText->SetText(FText::FromString(FString::Printf(
			TEXT("权威容量 %d / %d  ·  基础 6  ·  空间 %d"),
			View.UsedCarriedSlots,
			View.TotalCarriedCapacity,
			View.SpatialStorageCapacity)));
	}
	RebuildCellPanels(View);

	const TArray<Fdemo_mapHotbarSlotView> Hotbar =
		Fdemo_mapItemPresentation::BuildRuntimeHotbar(
			Items->GetHotbarBindingSnapshot(),
			Authority,
			Items->GetItemUseCooldownSnapshot());
	for (int32 Index = 0;
		Index < HotbarTexts.Num();
		++Index)
	{
		if (!Hotbar.IsValidIndex(Index))
		{
			continue;
		}
		const Fdemo_mapHotbarSlotView& HotbarView = Hotbar[Index];
		HotbarTexts[Index]->SetText(FText::FromString(
			HotbarView.IsOccupied()
				? FString::Printf(
					TEXT("[%d]\n%s ×%d"),
					HotbarView.SlotNumber,
					*HotbarView.DisplayName.Left(6),
					HotbarView.Quantity)
				: FString::Printf(
					TEXT("[%d]\n空"),
					HotbarView.SlotNumber)));
		HotbarButtons[Index]->SetBackgroundColor(
			HotbarView.State == Edemo_mapHotbarSlotState::Ready
				? FLinearColor(0.12f, 0.45f, 0.23f, 1.0f)
				: (HotbarView.State
						== Edemo_mapHotbarSlotState::InvalidBinding
					? FLinearColor(0.52f, 0.12f, 0.10f, 1.0f)
					: FLinearColor(0.10f, 0.19f, 0.28f, 1.0f)));
	}
	RefreshDetailsAndActions(Authority);
}

void Udemo_mapInventoryWidget::RebuildCellPanels(
	const Fdemo_mapEntityLoadoutView& View)
{
	if (!EquipmentPanel || !BaseQuickPanel || !SpatialStoragePanel)
	{
		return;
	}
	EquipmentPanel->ClearChildren();
	BaseQuickPanel->ClearChildren();
	SpatialStoragePanel->ClearChildren();
	RuntimeCells.Reset();

	for (int32 Index = 0;
		Index < View.EquipmentSlots.Num();
		++Index)
	{
		const Fdemo_mapEntityItemSlotView& ItemSlot =
			View.EquipmentSlots[Index];
		Fdemo_mapUnifiedItemCellView Cell =
			Fdemo_mapItemPresentation::BuildCell(ItemSlot);
		Cell.SlotIndex = 100 + Index;
		Cell.DisplayName = EquipmentRegionLabel(ItemSlot.Region)
			+ TEXT(" · ")
			+ Cell.DisplayName;
		Cell.bSelected =
			SelectedEquipmentSlot
			== EquipmentSlotForRegion(ItemSlot.Region);
		Udemo_mapItemCellWidget* Widget =
			CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(),
				Udemo_mapItemCellWidget::StaticClass());
		Widget->InitializeCell(
			Cell,
			Fdemo_mapItemCellActivated::CreateUObject(
				this,
				&Udemo_mapInventoryWidget::SelectEquipmentCell),
			Edemo_mapItemPresentationContext::RuntimeEquipment,
			Fdemo_mapItemCellDropped::CreateUObject(
				this,
				&Udemo_mapInventoryWidget::HandleItemDrop));
		EquipmentPanel->AddChildToWrapBox(Widget);
		RuntimeCells.Add(Widget);
	}
	for (const Fdemo_mapEntityItemSlotView& ItemSlot :
		View.BaseQuickItemSlots)
	{
		Fdemo_mapUnifiedItemCellView Cell =
			Fdemo_mapItemPresentation::BuildCell(ItemSlot);
		Cell.bSelected =
			SelectedInventorySlotIndex == ItemSlot.SlotIndex;
		Cell.ShortcutLabel =
			FString::Printf(TEXT("B%d"), ItemSlot.SlotIndex + 1);
		Udemo_mapItemCellWidget* Widget =
			CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(),
				Udemo_mapItemCellWidget::StaticClass());
		Widget->InitializeCell(
			Cell,
			Fdemo_mapItemCellActivated::CreateUObject(
				this,
				&Udemo_mapInventoryWidget::SelectInventorySlot),
			Edemo_mapItemPresentationContext::RuntimeBaseQuickItems,
			Fdemo_mapItemCellDropped::CreateUObject(
				this,
				&Udemo_mapInventoryWidget::HandleItemDrop));
		BaseQuickPanel->AddChildToUniformGrid(
			Widget,
			ItemSlot.SlotIndex / 3,
			ItemSlot.SlotIndex % 3);
		RuntimeCells.Add(Widget);
	}
	for (const Fdemo_mapEntityItemSlotView& ItemSlot :
		View.SpatialStorageSlots)
	{
		Fdemo_mapUnifiedItemCellView Cell =
			Fdemo_mapItemPresentation::BuildCell(ItemSlot);
		Cell.SlotIndex =
			Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			+ ItemSlot.SlotIndex;
		Cell.bSelected =
			SelectedInventorySlotIndex == Cell.SlotIndex;
		Udemo_mapItemCellWidget* Widget =
			CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(),
				Udemo_mapItemCellWidget::StaticClass());
		Widget->InitializeCell(
			Cell,
			Fdemo_mapItemCellActivated::CreateUObject(
				this,
				&Udemo_mapInventoryWidget::SelectInventorySlot),
			Edemo_mapItemPresentationContext::RuntimeSpatialStorage,
			Fdemo_mapItemCellDropped::CreateUObject(
				this,
				&Udemo_mapInventoryWidget::HandleItemDrop));
		SpatialStoragePanel->AddChildToUniformGrid(
			Widget,
			ItemSlot.SlotIndex / 3,
			ItemSlot.SlotIndex % 3);
		RuntimeCells.Add(Widget);
	}
}

void Udemo_mapInventoryWidget::HandleItemDrop(
	int32 SourceSlotIndex,
	FGuid SourceItemId,
	Edemo_mapItemPresentationContext SourceContext,
	int32 TargetSlotIndex,
	Edemo_mapItemPresentationContext TargetContext)
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem())
	{
		return;
	}
	auto AreaForContext = [](Edemo_mapItemPresentationContext Context)
	{
		switch (Context)
		{
		case Edemo_mapItemPresentationContext::RuntimeEquipment:
			return Edemo_mapPlayerItemArea::Equipment;
		case Edemo_mapItemPresentationContext::RuntimeBaseQuickItems:
			return Edemo_mapPlayerItemArea::BaseQuickItems;
		case Edemo_mapItemPresentationContext::RuntimeSpatialStorage:
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
	auto VisualSlotForContext = [](int32 SlotIndex, Edemo_mapItemPresentationContext Context)
	{
		return Context == Edemo_mapItemPresentationContext::RuntimeSpatialStorage
			? SlotIndex - Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			: SlotIndex;
	};
	if (TargetContext == Edemo_mapItemPresentationContext::RuntimeWorldDrop)
	{
		const Edemo_mapPlayerItemArea SourceArea = AreaForContext(SourceContext);
		if (SourceArea == Edemo_mapPlayerItemArea::Invalid)
		{
			SetFeedback(TEXT("只有已识别的玩家物品可以拖到局内世界。"), false);
			return;
		}
		Fdemo_mapPlayerItemDropIntent WorldDropIntent;
		WorldDropIntent.ExpectedAuthorityRevision =
			Manager->GetItemSubsystem()->GetAuthority().GetAuthorityRevision();
		WorldDropIntent.ExpectedSourceItemInstanceId = SourceItemId;
		WorldDropIntent.SourceArea = SourceArea;
		WorldDropIntent.SourceSlotIndex =
			VisualSlotForContext(SourceSlotIndex, SourceContext);
		WorldDropIntent.SourceEquipmentSlotId =
			SourceContext == Edemo_mapItemPresentationContext::RuntimeEquipment
				? EquipmentForEncodedSlot(SourceSlotIndex) : NAME_None;
		const bool bSpatialEquipment =
			WorldDropIntent.SourceArea == Edemo_mapPlayerItemArea::Equipment
			&& WorldDropIntent.SourceEquipmentSlotId
				== Fdemo_mapItemIds::BackpackSlot;
		if (bSpatialEquipment
			&& Manager->GetItemSubsystem()->GetAuthority().GetUsedInventorySlots()
				> Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack)
		{
			PendingSpatialBundleDrop = WorldDropIntent;
			SetFeedback(TEXT("空间道具包含内部物品：请确认原子 Bundle 丢弃，或取消。"), false);
			RefreshFromAuthority();
			return;
		}
		ShowOperationResult(Manager->RequestDropPlayerItem(WorldDropIntent));
		SelectedInventoryInstance.Invalidate();
		SelectedInventorySlotIndex = INDEX_NONE;
		SelectedEquipmentSlot = NAME_None;
		RefreshFromAuthority();
		return;
	}

	Fdemo_mapPlayerItemDropIntent Intent;
	Intent.ExpectedAuthorityRevision =
		Manager->GetItemSubsystem()->GetAuthority().GetAuthorityRevision();
	Intent.ExpectedSourceItemInstanceId = SourceItemId;
	Intent.SourceArea = AreaForContext(SourceContext);
	Intent.TargetArea = AreaForContext(TargetContext);
	Intent.SourceSlotIndex = VisualSlotForContext(SourceSlotIndex, SourceContext);
	Intent.TargetSlotIndex = VisualSlotForContext(TargetSlotIndex, TargetContext);
	Intent.SourceEquipmentSlotId =
		SourceContext == Edemo_mapItemPresentationContext::RuntimeEquipment
			? EquipmentForEncodedSlot(SourceSlotIndex) : NAME_None;
	Intent.TargetEquipmentSlotId =
		TargetContext == Edemo_mapItemPresentationContext::RuntimeEquipment
			? EquipmentForEncodedSlot(TargetSlotIndex) : NAME_None;
	const Fdemo_mapPlayerItemDropResult Result =
		Manager->GetItemSubsystem()->ExecutePlayerItemDrop(Intent);
	ShowOperationResult(Result.Operation);
	SelectedInventoryInstance.Invalidate();
	SelectedInventorySlotIndex = INDEX_NONE;
	SelectedEquipmentSlot = NAME_None;
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::RefreshDetailsAndActions(
	const Fdemo_mapItemAuthority& Authority)
{
	const bool bHasPendingSpatialBundle =
		PendingSpatialBundleDrop.ExpectedSourceItemInstanceId.IsValid();
	if (ConfirmSpatialBundleDropButton)
	{
		ConfirmSpatialBundleDropButton->SetVisibility(
			bHasPendingSpatialBundle
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (CancelSpatialBundleDropButton)
	{
		CancelSpatialBundleDropButton->SetVisibility(
			bHasPendingSpatialBundle
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	const Fdemo_mapItemInstance* Item =
		Authority.FindInstance(SelectedInventoryInstance);
	if (!Item && !SelectedEquipmentSlot.IsNone())
	{
		Item = Authority.FindInstance(
			Authority.GetEquippedInstance(SelectedEquipmentSlot));
	}
	if (!Item)
	{
		DetailsText->SetText(FText::FromString(
			TEXT("选择装备或携带物品。\n页面不持有 ItemInstance 副本。")));
		EquipButton->SetIsEnabled(false);
		UnequipButton->SetIsEnabled(false);
		MoveButton->SetIsEnabled(false);
		UseButton->SetIsEnabled(false);
		DropButton->SetIsEnabled(false);
		return;
	}

	const Fdemo_mapUnifiedItemDetailView Detail =
		Fdemo_mapItemPresentation::BuildDetail(*Item);
	DetailsText->SetText(FText::FromString(FString::Printf(
		TEXT("%s  ×%d\n%s · %s · %s\n属性：%s\n词条：%s\n使用：%s\n位置：%s\n状态：%s\nID：%s"),
		*Detail.DisplayName,
		Detail.Quantity,
		*Detail.TypeLabel,
		*Detail.LevelLabel,
		*Detail.QualityLabel,
		*Detail.BaseAttributes,
		*Detail.RandomAffixes,
		*Detail.UseEffect,
		*Detail.AllowedPositions,
		*Detail.TagsAndStatus,
		*Detail.ItemInstanceId.ToString(EGuidFormats::Digits))));

	Edemo_mapItemPresentationContext Context =
		Edemo_mapItemPresentationContext::RuntimeEquipment;
	if (SelectedInventorySlotIndex >= 0)
	{
		Context = SelectedInventorySlotIndex
				< Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			? Edemo_mapItemPresentationContext::RuntimeBaseQuickItems
			: Edemo_mapItemPresentationContext::RuntimeSpatialStorage;
	}
	const Fdemo_mapResolvedItemActions Actions =
		Fdemo_mapItemPresentation::ResolveRuntimeActions(
			*Item,
			Context);
	EquipButton->SetIsEnabled(
		Actions.Contains(Edemo_mapItemContextAction::Equip));
	UnequipButton->SetIsEnabled(
		Actions.Contains(Edemo_mapItemContextAction::Unequip));
	MoveButton->SetIsEnabled(
		Actions.Contains(
			Edemo_mapItemContextAction::MoveToBaseQuickItems)
		|| Actions.Contains(
			Edemo_mapItemContextAction::MoveToSpatialStorage));
	UseButton->SetIsEnabled(
		Actions.Contains(Edemo_mapItemContextAction::Use));
	DropButton->SetIsEnabled(true);
}

void Udemo_mapInventoryWidget::SelectInventorySlot(int32 SlotIndex)
{
	PendingHotbarSlotNumber = INDEX_NONE;
	PendingHotbarInstance.Invalidate();
	SelectedEquipmentSlot = NAME_None;
	SelectedInventorySlotIndex = SlotIndex;
	SelectedInventoryInstance.Invalidate();
	if (Manager.IsValid() && Manager->GetItemSubsystem())
	{
		const TArray<FGuid>& Slots =
			Manager->GetItemSubsystem()->GetAuthority()
				.GetInventorySlotSnapshot();
		if (Slots.IsValidIndex(SlotIndex))
		{
			SelectedInventoryInstance = Slots[SlotIndex];
		}
	}
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::SelectEquipmentCell(
	int32 EncodedIndex)
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem())
	{
		return;
	}
	const Fdemo_mapEntityLoadoutView View =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerRuntimeView(
			Manager->GetItemSubsystem()->GetAuthority());
	const int32 Index = EncodedIndex - 100;
	if (View.EquipmentSlots.IsValidIndex(Index))
	{
		SelectEquipmentSlot(
			EquipmentSlotForRegion(
				View.EquipmentSlots[Index].Region));
	}
}

void Udemo_mapInventoryWidget::SelectEquipmentSlot(FName SlotId)
{
	PendingHotbarSlotNumber = INDEX_NONE;
	PendingHotbarInstance.Invalidate();
	SelectedInventoryInstance.Invalidate();
	SelectedInventorySlotIndex = INDEX_NONE;
	SelectedEquipmentSlot = SlotId;
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleEquip()
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem()
		|| !SelectedInventoryInstance.IsValid())
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("Select an inventory equipment item.")));
		return;
	}
	const Fdemo_mapItemInstance* Item =
		Manager->GetItemSubsystem()->GetAuthority().FindInstance(
			SelectedInventoryInstance);
	const Fdemo_mapItemDefinition* Definition = Item
		? Fdemo_mapItemDefinitions::Find(Item->DefinitionId)
		: nullptr;
	if (!Definition || Definition->CompatibleSlotIds.Num() != 1)
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::NotEquipable,
			TEXT("Selected item has no single compatible slot."),
			SelectedInventoryInstance));
		return;
	}
	ShowOperationResult(Manager->GetItemSubsystem()->Equip(
		SelectedInventoryInstance,
		Definition->CompatibleSlotIds[0]));
	SelectedInventoryInstance.Invalidate();
	SelectedInventorySlotIndex = INDEX_NONE;
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleUnequip()
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem()
		|| SelectedEquipmentSlot.IsNone())
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("Select an equipment slot.")));
		return;
	}
	ShowOperationResult(
		Manager->GetItemSubsystem()->Unequip(
			SelectedEquipmentSlot));
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleDrop()
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem())
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("Select a Runtime item to drop.")));
		return;
	}
	Fdemo_mapPlayerItemDropIntent Intent;
	Intent.ExpectedAuthorityRevision = Manager->GetItemSubsystem()
		->GetAuthority().GetAuthorityRevision();
	if (!SelectedEquipmentSlot.IsNone())
	{
		Intent.SourceArea = Edemo_mapPlayerItemArea::Equipment;
		Intent.SourceEquipmentSlotId = SelectedEquipmentSlot;
		Intent.ExpectedSourceItemInstanceId = Manager->GetItemSubsystem()
			->GetAuthority().GetEquippedInstance(SelectedEquipmentSlot);
	}
	else if (SelectedInventorySlotIndex >= 0
		&& SelectedInventoryInstance.IsValid())
	{
		Intent.SourceArea = SelectedInventorySlotIndex
			< Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			? Edemo_mapPlayerItemArea::BaseQuickItems
			: Edemo_mapPlayerItemArea::SpatialStorage;
		Intent.SourceSlotIndex = Intent.SourceArea
			== Edemo_mapPlayerItemArea::SpatialStorage
			? SelectedInventorySlotIndex
				- Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			: SelectedInventorySlotIndex;
		Intent.ExpectedSourceItemInstanceId = SelectedInventoryInstance;
	}
	if (!Intent.ExpectedSourceItemInstanceId.IsValid())
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("Select an occupied equipment or Runtime inventory cell.")));
		return;
	}
	const bool bSpatialEquipment = Intent.SourceArea
		== Edemo_mapPlayerItemArea::Equipment
		&& Intent.SourceEquipmentSlotId == Fdemo_mapItemIds::BackpackSlot;
	if (bSpatialEquipment && Manager->GetItemSubsystem()->GetAuthority()
		.GetUsedInventorySlots()
		> Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack)
	{
		PendingSpatialBundleDrop = Intent;
		SetFeedback(TEXT("空间道具包含内部物品：请确认原子 Bundle 丢弃，或取消。"), false);
		RefreshFromAuthority();
		return;
	}
	ShowOperationResult(Manager->RequestDropPlayerItem(Intent));
	if (LastResultCode == Edemo_mapItemResultCode::Success)
	{
		SelectedInventoryInstance.Invalidate();
		SelectedInventorySlotIndex = INDEX_NONE;
		SelectedEquipmentSlot = NAME_None;
	}
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleConfirmSpatialBundleDrop()
{
	if (!Manager.IsValid()
		|| !PendingSpatialBundleDrop.ExpectedSourceItemInstanceId.IsValid())
	{
		return;
	}
	ShowOperationResult(Manager->RequestDropPlayerItem(
		PendingSpatialBundleDrop,
		true));
	PendingSpatialBundleDrop = Fdemo_mapPlayerItemDropIntent();
	SelectedInventoryInstance.Invalidate();
	SelectedInventorySlotIndex = INDEX_NONE;
	SelectedEquipmentSlot = NAME_None;
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleCancelSpatialBundleDrop()
{
	PendingSpatialBundleDrop = Fdemo_mapPlayerItemDropIntent();
	SetFeedback(TEXT("空间道具 Bundle 丢弃已取消；未改变物品权威。"), true);
	RefreshFromAuthority();
}

int32 Udemo_mapInventoryWidget::FindFirstEmptyInRange(
	const TArray<FGuid>& Slots,
	int32 BeginIndex,
	int32 EndIndex) const
{
	for (int32 Index = BeginIndex;
		Index < EndIndex && Slots.IsValidIndex(Index);
		++Index)
	{
		if (!Slots[Index].IsValid())
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void Udemo_mapInventoryWidget::HandleMove()
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem()
		|| SelectedInventorySlotIndex < 0
		|| !SelectedInventoryInstance.IsValid())
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("Select an occupied Runtime inventory cell.")));
		return;
	}
	Udemo_mapItemSubsystem* Items = Manager->GetItemSubsystem();
	const TArray<FGuid>& Slots =
		Items->GetAuthority().GetInventorySlotSnapshot();
	const bool bFromBase =
		SelectedInventorySlotIndex
		< Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount;
	const int32 TargetIndex = bFromBase
		? FindFirstEmptyInRange(
			Slots,
			Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount,
			Slots.Num())
		: FindFirstEmptyInRange(
			Slots,
			0,
			Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount);
	if (TargetIndex == INDEX_NONE)
	{
		ShowOperationResult(Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			TEXT("The target Runtime storage region has no empty cell."),
			SelectedInventoryInstance));
		return;
	}
	const Fdemo_mapItemOperationResult Result =
		Items->MoveInventorySlot(
			SelectedInventorySlotIndex,
			TargetIndex);
	ShowOperationResult(Result);
	if (Result.bSuccess)
	{
		SelectedInventorySlotIndex = TargetIndex;
	}
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleUse()
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem()
		|| !SelectedInventoryInstance.IsValid())
	{
		SetFeedback(TEXT("请选择可直接使用的消耗品。"), false);
		return;
	}
	const Fdemo_mapItemUseResult Result =
		Manager->GetItemSubsystem()->UseInventoryItem(
			SelectedInventoryInstance,
			true);
	SetFeedback(
		Result.IsSuccess()
			? FString::Printf(
				TEXT("使用成功：生命 %d → %d，剩余 %d。"),
				Result.BeforeHealth,
				Result.AfterHealth,
				Result.AfterStack)
			: Result.Diagnostic,
		Result.IsSuccess());
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleHotbarSlot(int32 SlotNumber)
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem()
		|| SelectedInventorySlotIndex < 0
		|| SelectedInventorySlotIndex
			>= Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
		|| !SelectedInventoryInstance.IsValid())
	{
		SetFeedback(
			TEXT("快捷栏只接受基础六格中的消耗品。"),
			false);
		return;
	}
	Udemo_mapItemSubsystem* Items = Manager->GetItemSubsystem();
	const TArray<FGuid>& Bindings =
		Items->GetHotbarBindingSnapshot().SlotBindings;
	const FGuid Existing =
		Bindings.IsValidIndex(SlotNumber - 1)
			? Bindings[SlotNumber - 1]
			: FGuid();
	if (Existing == SelectedInventoryInstance)
	{
		ShowOperationResult(Items->UnbindHotbarSlot(SlotNumber));
		PendingHotbarSlotNumber = INDEX_NONE;
		PendingHotbarInstance.Invalidate();
		RefreshFromAuthority();
		return;
	}
	if (Existing.IsValid()
		&& (PendingHotbarSlotNumber != SlotNumber
			|| PendingHotbarInstance
				!= SelectedInventoryInstance))
	{
		PendingHotbarSlotNumber = SlotNumber;
		PendingHotbarInstance = SelectedInventoryInstance;
		SetFeedback(
			FString::Printf(
				TEXT("%d 号快捷栏已占用；再次点击确认覆盖，选择其他物品或关闭可取消。"),
				SlotNumber),
			false);
		return;
	}
	ShowOperationResult(Items->BindHotbarSlot(
		SlotNumber,
		SelectedInventoryInstance));
	PendingHotbarSlotNumber = INDEX_NONE;
	PendingHotbarInstance.Invalidate();
	RefreshFromAuthority();
}

void Udemo_mapInventoryWidget::HandleClose()
{
	PendingHotbarSlotNumber = INDEX_NONE;
	PendingHotbarInstance.Invalidate();
	ClearTransientDragState();
	if (Manager.IsValid())
	{
		Manager->CloseInventory();
	}
}

void Udemo_mapInventoryWidget::SetFeedback(
	const FString& Message,
	bool bSuccess)
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::FromString(Message));
		FeedbackText->SetColorAndOpacity(FSlateColor(
			bSuccess
				? FLinearColor(0.25f, 1.0f, 0.35f)
				: FLinearColor(1.0f, 0.55f, 0.20f)));
	}
}

void Udemo_mapInventoryWidget::ShowOperationResult(
	const Fdemo_mapItemOperationResult& Result)
{
	LastResultCode = Result.Code;
	SetFeedback(
		MakeResultText(Result).ToString(),
		Result.bSuccess);
}

FText Udemo_mapInventoryWidget::MakeResultText(
	const Fdemo_mapItemOperationResult& Result) const
{
	FString Text;
	switch (Result.Code)
	{
	case Edemo_mapItemResultCode::Success:
		Text = TEXT("Success / 操作成功；已从权威数据重建。");
		break;
	case Edemo_mapItemResultCode::InventoryFull:
		Text = TEXT("Inventory Full / 目标区域已满");
		break;
	case Edemo_mapItemResultCode::IncompatibleSlot:
		Text = TEXT("Incompatible Slot / 装备槽不兼容");
		break;
	case Edemo_mapItemResultCode::UnsafeDropLocation:
		Text = TEXT("Unsafe Drop Location / 没有安全丢弃位置");
		break;
	case Edemo_mapItemResultCode::UIInvalidSelection:
		Text = TEXT("Invalid Selection / 请选择有效物品或装备槽");
		break;
	default:
		Text = Result.Diagnostic.IsEmpty()
			? FString::Printf(
				TEXT("Request Failed (%d)"),
				static_cast<int32>(Result.Code))
			: Result.Diagnostic;
		break;
	}
	return FText::FromString(Text);
}

void Udemo_mapInventoryWidget::ClickEquip() { HandleEquip(); }
void Udemo_mapInventoryWidget::ClickUnequip() { HandleUnequip(); }
void Udemo_mapInventoryWidget::ClickDrop() { HandleDrop(); }
void Udemo_mapInventoryWidget::ClickConfirmSpatialBundleDrop()
{
	HandleConfirmSpatialBundleDrop();
}
void Udemo_mapInventoryWidget::ClickCancelSpatialBundleDrop()
{
	HandleCancelSpatialBundleDrop();
}
void Udemo_mapInventoryWidget::ClickMove() { HandleMove(); }
void Udemo_mapInventoryWidget::ClickUse() { HandleUse(); }
void Udemo_mapInventoryWidget::ClickClose() { HandleClose(); }
void Udemo_mapInventoryWidget::ClickHotbar1() { HandleHotbarSlot(1); }
void Udemo_mapInventoryWidget::ClickHotbar2() { HandleHotbarSlot(2); }
void Udemo_mapInventoryWidget::ClickHotbar3() { HandleHotbarSlot(3); }
void Udemo_mapInventoryWidget::ClickHotbar4() { HandleHotbarSlot(4); }
void Udemo_mapInventoryWidget::ClickHotbar5() { HandleHotbarSlot(5); }
void Udemo_mapInventoryWidget::ClickHotbar6() { HandleHotbarSlot(6); }
void Udemo_mapInventoryWidget::ClickHotbar7() { HandleHotbarSlot(7); }
void Udemo_mapInventoryWidget::ClickHotbar8() { HandleHotbarSlot(8); }
void Udemo_mapInventoryWidget::ClickHotbar9() { HandleHotbarSlot(9); }

#if !UE_BUILD_SHIPPING
bool Udemo_mapInventoryWidget::AutomationClickInventorySlot(
	int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= RuntimeSlotCount)
	{
		return false;
	}
	SelectInventorySlot(SlotIndex);
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickEquipmentSlot(
	FName SlotId)
{
	if (!Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Contains(SlotId))
	{
		return false;
	}
	SelectEquipmentSlot(SlotId);
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickEquip()
{
	if (!EquipButton) return false;
	HandleEquip();
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickUnequip()
{
	if (!UnequipButton) return false;
	HandleUnequip();
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickDrop()
{
	if (!DropButton) return false;
	HandleDrop();
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickMove()
{
	if (!MoveButton) return false;
	HandleMove();
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickUse()
{
	if (!UseButton) return false;
	HandleUse();
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickHotbarSlot(
	int32 SlotNumber)
{
	if (SlotNumber < 1 || SlotNumber > 9)
	{
		return false;
	}
	HandleHotbarSlot(SlotNumber);
	return true;
}

bool Udemo_mapInventoryWidget::AutomationClickClose()
{
	if (!CloseButton) return false;
	HandleClose();
	return true;
}
#endif
