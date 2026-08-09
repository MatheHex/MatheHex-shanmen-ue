#include "demo_mapSearchContainerWidget.h"
#include "demo_map.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

namespace
{
	constexpr int32 ContextActionSearch = 0;
	constexpr int32 ContextActionTake = 1;
	constexpr int32 ContextActionEquip = 2;
	constexpr int32 ContextActionDrop = 3;
	constexpr int32 ContextActionReturn = 4;
	constexpr int32 ContextActionCancel = 5;
	constexpr int32 ContextActionUnequip = 6;
	constexpr int32 ContextActionBindFirst = 100;
	constexpr int32 ContextActionConfirmBundleDrop = 200;
	constexpr int32 ContextActionCancelBundleDrop = 201;

	UTextBlock* MakeContainerText(
		UWidgetTree* Tree,
		const FString& Value,
		int32 Size = 18)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetAutoWrapText(true);
		return Text;
	}

	void AddContainerPadded(
		UVerticalBox* Box,
		UWidget* Widget,
		float Padding = 3.0f)
	{
		if (UVerticalBoxSlot* LayoutSlot = Box->AddChildToVerticalBox(Widget))
		{
			LayoutSlot->SetPadding(FMargin(Padding));
		}
	}
}

void Udemo_mapSearchContextActionWidget::InitializeAction(
	const FString& Label,
	int32 InActionCode,
	Fdemo_mapSearchContextActionClicked InActivated)
{
	ActionLabel = Label;
	ActionCode = InActionCode;
	Activated = MoveTemp(InActivated);
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
}

void Udemo_mapSearchContextActionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_mapSearchContextActionWidget::BuildInterface()
{
	if (!WidgetTree)
	{
		return;
	}
	if (bBuilt)
	{
		if (Button)
		{
			if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
			{
				Label->SetText(FText::FromString(ActionLabel));
			}
		}
		return;
	}
	bBuilt = true;
	Button = WidgetTree->ConstructWidget<UButton>();
	WidgetTree->RootWidget = Button;
	Button->SetBackgroundColor(FLinearColor(0.08f, 0.16f, 0.21f, 1.0f));
	Button->SetContent(MakeContainerText(WidgetTree, ActionLabel, 14));
	Button->OnClicked.AddDynamic(this, &Udemo_mapSearchContextActionWidget::ClickAction);
}

void Udemo_mapSearchContextActionWidget::ClickAction()
{
	if (Activated.IsBound())
	{
		Activated.Execute(ActionCode);
	}
}

void Udemo_mapNestedContainerWindowWidget::InitializeWindow(
	const FString& InTitle,
	const TArray<Fdemo_mapEntityItemSlotView>& InSlots,
	Edemo_mapItemPresentationContext InContext,
	int32 InInventoryOffset,
	Fdemo_mapItemCellActivated InActivated,
	Fdemo_mapItemCellDropped InDropped,
	Fdemo_mapItemCellContextRequested InContextRequested,
	Fdemo_mapItemCellDoubleClicked InDoubleClicked,
	TFunction<void()> InClosed)
{
	Title = InTitle;
	Slots = InSlots;
	Context = InContext;
	InventoryOffset = InInventoryOffset;
	Activated = MoveTemp(InActivated);
	Dropped = MoveTemp(InDropped);
	ContextRequested = MoveTemp(InContextRequested);
	DoubleClicked = MoveTemp(InDoubleClicked);
	Closed = MoveTemp(InClosed);
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
	RefreshSlots();
}

void Udemo_mapNestedContainerWindowWidget::BuildInterface()
{
	if (!WidgetTree || bBuilt)
	{
		return;
	}
	bBuilt = true;
	SetIsFocusable(true);
	USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>();
	// The window keeps a multi-column, scrollable grid, while allowing the
	// shared readable item squares to fit without feeling compressed.
	Frame->SetWidthOverride(430.0f);
	Frame->SetHeightOverride(520.0f);
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(FLinearColor(0.025f, 0.075f, 0.11f, 0.98f));
	Border->SetPadding(FMargin(6.0f));
	Frame->SetContent(Border);
	WidgetTree->RootWidget = Frame;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Border->SetContent(Column);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Header);
	TitleText = MakeContainerText(WidgetTree, Title, 16);
	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetContent(MakeContainerText(WidgetTree, TEXT("×"), 18));
	CloseButton->OnClicked.AddDynamic(this, &Udemo_mapNestedContainerWindowWidget::ClickClose);
	Header->AddChildToHorizontalBox(CloseButton);
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	if (UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetPadding(FMargin(2.0f, 5.0f));
	}
	SlotGrid = WidgetTree->ConstructWidget<UWrapBox>();
	SlotGrid->SetInnerSlotPadding(FVector2D(3.0f, 3.0f));
	Scroll->AddChild(SlotGrid);
}

void Udemo_mapNestedContainerWindowWidget::RefreshSlots()
{
	if (!SlotGrid)
	{
		return;
	}
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(Title));
	}
	SlotGrid->ClearChildren();
	for (const Fdemo_mapEntityItemSlotView& ItemSlot : Slots)
	{
		Fdemo_mapUnifiedItemCellView Cell =
			Fdemo_mapItemPresentation::BuildCell(ItemSlot);
		Cell.SlotIndex = InventoryOffset + ItemSlot.SlotIndex;
		Cell.ShortcutLabel = TEXT("袋");
		Udemo_mapItemCellWidget* ItemCell =
			CreateWidget<Udemo_mapItemCellWidget>(
				GetOwningPlayer(), Udemo_mapItemCellWidget::StaticClass());
		ItemCell->InitializeCell(
			Cell,
			Activated,
			Context,
			Dropped,
			ContextRequested,
			DoubleClicked);
		SlotGrid->AddChildToWrapBox(ItemCell);
	}
}

void Udemo_mapNestedContainerWindowWidget::SetWindowPosition(
	const FVector2D& InPosition,
	int32 InZOrder)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetPosition(InPosition);
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetZOrder(InZOrder);
		return;
	}
	// The loot overlay has its own full-screen canvas.  A native child window
	// added to that canvas can be clipped behind the panel at some DPI scales;
	// retain the same local position as a viewport overlay in that case.
	SetPositionInViewport(InPosition, false);
}

FReply Udemo_mapNestedContainerWindowWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			CanvasSlot->SetZOrder(120);
		}
		else
		{
			// The viewport z-order is supplied by AddToViewport at construction.
		}
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()).Y <= 30.0f)
	{
		bMoving = true;
		GrabOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply Udemo_mapNestedContainerWindowWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!bMoving)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetParent()))
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			const FVector2D CanvasPoint = Canvas->GetCachedGeometry().AbsoluteToLocal(
				InMouseEvent.GetScreenSpacePosition());
			const FVector2D CanvasSize = Canvas->GetCachedGeometry().GetLocalSize();
			CanvasSlot->SetPosition(FVector2D(
				FMath::Clamp(CanvasPoint.X - GrabOffset.X, 0.0f, FMath::Max(0.0f, CanvasSize.X - 360.0f)),
				FMath::Clamp(CanvasPoint.Y - GrabOffset.Y, 0.0f, FMath::Max(0.0f, CanvasSize.Y - 440.0f))));
		}
	}
	else
	{
		SetPositionInViewport(
			InMouseEvent.GetScreenSpacePosition() - GrabOffset,
			true);
	}
	return FReply::Handled();
}

FReply Udemo_mapNestedContainerWindowWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bMoving && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bMoving = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void Udemo_mapNestedContainerWindowWidget::ClickClose()
{
	if (Closed)
	{
		Closed();
	}
}

void Udemo_mapSearchContainerWidget::InitializeForManager(
	Ademo_mapV3ProgressionManager* InManager)
{
	Manager = InManager;
}

void Udemo_mapSearchContainerWidget::ClearTransientDragState()
{
	PendingSpatialBundleDrop = Fdemo_mapPlayerItemDropIntent();
	DismissContextMenu();
	ClosePlayerBagWindow();
	CloseTargetBagWindow();
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

void Udemo_mapSearchContainerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!bBuilt)
	{
		BuildInterface();
	}
}

void Udemo_mapSearchContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!bBuilt)
	{
		BuildInterface();
	}
	RefreshVisuals();
}

FReply Udemo_mapSearchContainerWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (ContextMenu)
		{
			DismissContextMenu();
			return FReply::Handled();
		}
		if (TargetBagWindow)
		{
			CloseTargetBagWindow();
			return FReply::Handled();
		}
		if (PlayerBagWindow)
		{
			ClosePlayerBagWindow();
			return FReply::Handled();
		}
		HandleClose();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

const Fdemo_mapRuntimeContainerEntrySnapshot*
Udemo_mapSearchContainerWidget::FindContainerEntry(
	Edemo_mapItemPresentationContext Context,
	int32 SlotIndex) const
{
	Edemo_mapRuntimeContainerSection Section =
		Edemo_mapRuntimeContainerSection::Chest;
	switch (Context)
	{
	case Edemo_mapItemPresentationContext::SearchContainerGrid:
		Section = Edemo_mapRuntimeContainerSection::Chest;
		break;
	case Edemo_mapItemPresentationContext::SearchContainerEquipment:
		Section = Edemo_mapRuntimeContainerSection::Equipment;
		break;
	case Edemo_mapItemPresentationContext::SearchContainerBaseQuickItems:
	case Edemo_mapItemPresentationContext::SearchContainerSpatialStorage:
		Section = Edemo_mapRuntimeContainerSection::Backpack;
		break;
	case Edemo_mapItemPresentationContext::SearchContainerBody:
		Section = Edemo_mapRuntimeContainerSection::Body;
		break;
	default:
		return nullptr;
	}
	const Fdemo_mapRuntimeContainerSectionSnapshot* SnapshotSection =
		Snapshot.Sections.FindByPredicate(
			[Section](const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
			{
				return Candidate.Section == Section;
			});
	return SnapshotSection
		? SnapshotSection->OrderedOccupiedEntries.FindByPredicate(
			[SlotIndex](const Fdemo_mapRuntimeContainerEntrySnapshot& Candidate)
			{
				return Candidate.SlotIndex == SlotIndex;
			})
		: nullptr;
}

void Udemo_mapSearchContainerWidget::DismissContextMenu()
{
	if (ContextMenu)
	{
		ContextMenu->RemoveFromParent();
		ContextMenu = nullptr;
	}
}

void Udemo_mapSearchContainerWidget::HandleItemCellContextRequested(
	int32 SourceSlotIndex,
	FGuid SourceItemId,
	Edemo_mapItemPresentationContext SourceContext,
	FVector2D ScreenPosition)
{
	if (!RootCanvas || !SourceItemId.IsValid())
	{
		return;
	}
	DismissContextMenu();
	ContextSourceSlotIndex = SourceSlotIndex;
	ContextSourceItemId = SourceItemId;
	ContextSourceContext = SourceContext;
	ContextScreenPosition = ScreenPosition;

	ContextMenu = WidgetTree->ConstructWidget<UBorder>();
	ContextMenu->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.98f));
	ContextMenu->SetPadding(FMargin(5.0f));
	RootCanvas->AddChild(ContextMenu);
	if (UCanvasPanelSlot* MenuSlot = Cast<UCanvasPanelSlot>(ContextMenu->Slot))
	{
		const FVector2D LocalPoint = GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
		MenuSlot->SetPosition(FVector2D(
			FMath::Clamp(LocalPoint.X, 8.0f, 900.0f),
			FMath::Clamp(LocalPoint.Y, 8.0f, 620.0f)));
		MenuSlot->SetAutoSize(true);
	}
	UVerticalBox* MenuColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	ContextMenu->SetContent(MenuColumn);
	auto AddAction = [this, MenuColumn](const FString& Label, int32 ActionCode)
	{
		Udemo_mapSearchContextActionWidget* Entry =
			CreateWidget<Udemo_mapSearchContextActionWidget>(
				GetOwningPlayer(), Udemo_mapSearchContextActionWidget::StaticClass());
		Entry->InitializeAction(
			Label,
			ActionCode,
			Fdemo_mapSearchContextActionClicked::CreateUObject(
				this, &Udemo_mapSearchContainerWidget::ExecuteContextAction));
		if (UVerticalBoxSlot* EntrySlot = MenuColumn->AddChildToVerticalBox(Entry))
		{
			EntrySlot->SetPadding(FMargin(1.0f));
		}
	};

	if (const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
		FindContainerEntry(SourceContext, SourceSlotIndex))
	{
		if (Entry->State == Edemo_mapRuntimeContainerEntryState::Hidden)
		{
			AddAction(TEXT("搜索 / SEARCH"), ContextActionSearch);
		}
		if (Entry->State == Edemo_mapRuntimeContainerEntryState::Identified
			&& Entry->bCanTake)
		{
			AddAction(TEXT("拿取 / TAKE"), ContextActionTake);
		}
	}
	else if (Manager.IsValid() && Manager->GetItemSubsystem())
	{
		const Fdemo_mapItemInstance* Item = Manager->GetItemSubsystem()
			->GetAuthority().FindInstance(SourceItemId);
		if (Item)
		{
			const Fdemo_mapResolvedItemActions Actions =
				Fdemo_mapItemPresentation::ResolveRuntimeActions(*Item, SourceContext);
			if (Actions.Contains(Edemo_mapItemContextAction::Equip))
			{
				AddAction(TEXT("装备 / EQUIP"), ContextActionEquip);
			}
			if (Actions.Contains(Edemo_mapItemContextAction::Unequip))
			{
				AddAction(TEXT("卸下 / UNEQUIP"), ContextActionUnequip);
			}
			if (Actions.Contains(Edemo_mapItemContextAction::EquipToHotbar))
			{
				for (int32 HotbarSlot = 1; HotbarSlot <= Fdemo_mapHotbarBindingSnapshot::SlotCount; ++HotbarSlot)
				{
					AddAction(FString::Printf(TEXT("设为快捷键 %d"), HotbarSlot),
						ContextActionBindFirst + HotbarSlot - 1);
				}
			}
			if (Snapshot.bPlayerDepositAllowed)
			{
				AddAction(TEXT("放回 / RETURN"), ContextActionReturn);
			}
			AddAction(TEXT("丢弃 / DROP"), ContextActionDrop);
			AddAction(TEXT("取消 / CANCEL"), ContextActionCancel);
		}
	}
}

void Udemo_mapSearchContainerWidget::HandleItemCellDoubleClicked(
	int32 SourceSlotIndex,
	Edemo_mapItemPresentationContext SourceContext)
{
	UE_LOG(Logdemo_map, Log,
		TEXT("XFIX1_BAG_ACTION double slot=%d context=%d"),
		SourceSlotIndex,
		static_cast<int32>(SourceContext));
	if (SourceContext == Edemo_mapItemPresentationContext::RuntimeEquipment)
	{
		const int32 EquipmentIndex = SourceSlotIndex - 100;
		if (ViewState.PlayerLoadout.EquipmentSlots.IsValidIndex(EquipmentIndex)
			&& ViewState.PlayerLoadout.EquipmentSlots[EquipmentIndex].Region
				== Edemo_mapEntityLoadoutRegion::SpatialItem
			&& ViewState.PlayerLoadout.EquipmentSlots[EquipmentIndex].bOccupied)
		{
			OpenPlayerBagWindow();
		}
		return;
	}
	if (SourceContext == Edemo_mapItemPresentationContext::SearchContainerEquipment)
	{
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
			FindContainerEntry(SourceContext, SourceSlotIndex);
		const Fdemo_mapItemDefinition* Definition = Entry && Manager.IsValid()
			&& Manager->GetItemSubsystem()
			? Fdemo_mapItemDefinitions::Find(
				Manager->GetItemSubsystem()->GetAuthority()
					.FindInstance(Entry->ItemInstanceId)
					? Manager->GetItemSubsystem()->GetAuthority()
						.FindInstance(Entry->ItemInstanceId)->DefinitionId
						: NAME_None)
			: nullptr;
		if (Definition
			&& Definition->CategoryId == Fdemo_mapItemIds::BackpackCategory)
		{
			OpenTargetBagWindow();
		}
	}
}

void Udemo_mapSearchContainerWidget::OpenPlayerBagWindow()
{
	UE_LOG(Logdemo_map, Log,
		TEXT("XFIX1_BAG_OPEN player root=%d slots=%d capacity=%d"),
		RootCanvas ? 1 : 0,
		ViewState.PlayerLoadout.SpatialStorageSlots.Num(),
		ViewState.PlayerLoadout.SpatialStorageCapacity);
	if (!RootCanvas || ViewState.PlayerLoadout.SpatialStorageSlots.IsEmpty())
	{
		return;
	}
	const bool bCreatePlayerBagWindow = !PlayerBagWindow;
	if (bCreatePlayerBagWindow)
	{
		PlayerBagWindow = CreateWidget<Udemo_mapNestedContainerWindowWidget>(
			GetOwningPlayer(), Udemo_mapNestedContainerWindowWidget::StaticClass());
	}
	PlayerBagWindow->InitializeWindow(
		FString::Printf(TEXT("玩家吞天袋 / PLAYER BAG  %d / %d"),
			ViewState.PlayerLoadout.UsedCarriedSlots,
			ViewState.PlayerLoadout.SpatialStorageCapacity),
		ViewState.PlayerLoadout.SpatialStorageSlots,
		Edemo_mapItemPresentationContext::RuntimeSpatialStorage,
		Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			+ ViewState.PlayerLoadout.RingQuickItemCapacity,
		Fdemo_mapItemCellActivated(),
		Fdemo_mapItemCellDropped::CreateUObject(
			this, &Udemo_mapSearchContainerWidget::HandleItemCellDrop),
		Fdemo_mapItemCellContextRequested::CreateUObject(
			this, &Udemo_mapSearchContainerWidget::HandleItemCellContextRequested),
		Fdemo_mapItemCellDoubleClicked::CreateLambda(
			[this](int32 SlotIndex)
			{
				HandleItemCellDoubleClicked(
					SlotIndex,
					Edemo_mapItemPresentationContext::RuntimeSpatialStorage);
			}),
		[this]() { ClosePlayerBagWindow(); });
	if (bCreatePlayerBagWindow)
	{
		// Build the native widget tree before it enters the viewport; adding an
		// empty UUserWidget first leaves no Slate content to paint in packaged UI.
		// The main corpse-search overlay occupies viewport layer 550.
		PlayerBagWindow->AddToViewport(560);
		PlayerBagWindow->SetWindowPosition(FVector2D(84.0f, 110.0f), 110);
	}
}

void Udemo_mapSearchContainerWidget::OpenTargetBagWindow()
{
	const Fdemo_mapDualLootTargetRegionView* Region =
		ViewState.TargetRegions.FindByPredicate(
			[](const Fdemo_mapDualLootTargetRegionView& Candidate)
			{
				return Candidate.Region
					== Edemo_mapDualLootTargetRegion::SpatialStorage;
			});
	UE_LOG(Logdemo_map, Log,
		TEXT("XFIX1_BAG_OPEN target root=%d region=%d slots=%d"),
		RootCanvas ? 1 : 0,
		Region ? 1 : 0,
		Region ? Region->Cells.Num() : 0);
	if (!RootCanvas || !Region || Region->Cells.IsEmpty())
	{
		return;
	}
	TArray<Fdemo_mapEntityItemSlotView> Slots;
	Slots.Reserve(Region->Cells.Num());
	for (const Fdemo_mapDualLootTargetCellView& TargetCell : Region->Cells)
	{
		Fdemo_mapEntityItemSlotView ItemSlot;
		ItemSlot.Region = Edemo_mapEntityLoadoutRegion::SpatialStorage;
		ItemSlot.SlotIndex = TargetCell.SourceSlotIndex;
		ItemSlot.ItemInstanceId = TargetCell.Cell.ItemInstanceId;
		ItemSlot.ItemDefinitionId = TargetCell.Cell.ItemDefinitionId;
		ItemSlot.DisplayName = TargetCell.Cell.DisplayName;
		ItemSlot.LevelAndQualityLabel = TargetCell.Cell.LevelLabel;
		ItemSlot.Quantity = TargetCell.Cell.Quantity;
		ItemSlot.bOccupied = TargetCell.Cell.bOccupied;
		ItemSlot.bCanPlace = TargetCell.State
			!= Edemo_mapRuntimeContainerEntryState::Searching;
		Slots.Add(MoveTemp(ItemSlot));
	}
	const bool bCreateTargetBagWindow = !TargetBagWindow;
	if (bCreateTargetBagWindow)
	{
		TargetBagWindow = CreateWidget<Udemo_mapNestedContainerWindowWidget>(
			GetOwningPlayer(), Udemo_mapNestedContainerWindowWidget::StaticClass());
	}
	TargetBagWindow->InitializeWindow(
		FString::Printf(TEXT("目标空间袋 / TARGET BAG  %d 格"), Region->Capacity),
		Slots,
		Edemo_mapItemPresentationContext::SearchContainerSpatialStorage,
		0,
		Fdemo_mapItemCellActivated::CreateLambda(
			[this](int32 SlotIndex)
			{
				HandleTargetItemCellActivated(
					SlotIndex,
					Edemo_mapItemPresentationContext::SearchContainerSpatialStorage);
			}),
		Fdemo_mapItemCellDropped::CreateUObject(
			this, &Udemo_mapSearchContainerWidget::HandleItemCellDrop),
		Fdemo_mapItemCellContextRequested::CreateUObject(
			this, &Udemo_mapSearchContainerWidget::HandleItemCellContextRequested),
		Fdemo_mapItemCellDoubleClicked::CreateLambda(
			[this](int32 SlotIndex)
			{
				HandleItemCellDoubleClicked(
					SlotIndex,
					Edemo_mapItemPresentationContext::SearchContainerSpatialStorage);
			}),
		[this]() { CloseTargetBagWindow(); });
	if (bCreateTargetBagWindow)
	{
		TargetBagWindow->AddToViewport(561);
		TargetBagWindow->SetWindowPosition(FVector2D(470.0f, 110.0f), 111);
	}
}

void Udemo_mapSearchContainerWidget::ClosePlayerBagWindow()
{
	if (PlayerBagWindow)
	{
		PlayerBagWindow->RemoveFromParent();
		PlayerBagWindow = nullptr;
	}
}

void Udemo_mapSearchContainerWidget::CloseTargetBagWindow()
{
	if (TargetBagWindow)
	{
		TargetBagWindow->RemoveFromParent();
		TargetBagWindow = nullptr;
	}
}

void Udemo_mapSearchContainerWidget::HandleTargetItemCellActivated(
	int32 SourceSlotIndex,
	Edemo_mapItemPresentationContext SourceContext)
{
	if (!Manager.IsValid())
	{
		return;
	}
	const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
		FindContainerEntry(SourceContext, SourceSlotIndex);
	if (!Entry)
	{
		return;
	}
	if (Entry->State == Edemo_mapRuntimeContainerEntryState::Hidden)
	{
		Fdemo_mapRuntimeContainerIntent Intent;
		Intent.ExpectedRunId = Snapshot.OwningRunId;
		Intent.ContainerId = Snapshot.ContainerId;
		Intent.ExpectedRevision = Snapshot.Revision;
		Intent.EntryId = Entry->EntryId;
		Intent.Action = Edemo_mapRuntimeContainerActionKind::BeginSearch;
		LastResult = Manager->SubmitSearchContainerIntent(Intent);
		return;
	}
	if (Entry->State == Edemo_mapRuntimeContainerEntryState::Identified
		&& DiagnosticText)
	{
		DiagnosticText->SetText(FText::FromString(
			TEXT("物品已识别：右键选择拿取。")));
	}
}

void Udemo_mapSearchContainerWidget::ShowSpatialBundleConfirmation()
{
	if (!RootCanvas || !PendingSpatialBundleDrop.ExpectedSourceItemInstanceId.IsValid())
	{
		return;
	}
	DismissContextMenu();
	ContextMenu = WidgetTree->ConstructWidget<UBorder>();
	ContextMenu->SetBrushColor(FLinearColor(0.28f, 0.08f, 0.05f, 0.98f));
	ContextMenu->SetPadding(FMargin(5.0f));
	RootCanvas->AddChild(ContextMenu);
	if (UCanvasPanelSlot* MenuSlot = Cast<UCanvasPanelSlot>(ContextMenu->Slot))
	{
		const FVector2D LocalPoint = GetCachedGeometry().AbsoluteToLocal(ContextScreenPosition);
		MenuSlot->SetPosition(LocalPoint);
		MenuSlot->SetAutoSize(true);
	}
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	ContextMenu->SetContent(Column);
	auto AddConfirmation = [this, Column](const FString& Label, int32 Code)
	{
		Udemo_mapSearchContextActionWidget* Entry =
			CreateWidget<Udemo_mapSearchContextActionWidget>(
				GetOwningPlayer(), Udemo_mapSearchContextActionWidget::StaticClass());
		Entry->InitializeAction(Label, Code,
			Fdemo_mapSearchContextActionClicked::CreateUObject(
				this, &Udemo_mapSearchContainerWidget::ExecuteContextAction));
		Column->AddChildToVerticalBox(Entry);
	};
	AddConfirmation(TEXT("背包内有物品：确认整体丢弃"), ContextActionConfirmBundleDrop);
	AddConfirmation(TEXT("取消"), ContextActionCancelBundleDrop);
}

void Udemo_mapSearchContainerWidget::ExecuteContextAction(int32 ActionCode)
{
	if (!Manager.IsValid() || !Manager->GetItemSubsystem())
	{
		DismissContextMenu();
		return;
	}
	Udemo_mapItemSubsystem* Items = Manager->GetItemSubsystem();
	if (ActionCode == ContextActionSearch)
	{
		HandleTargetItemCellActivated(
			ContextSourceSlotIndex, ContextSourceContext);
		DismissContextMenu();
		return;
	}
	if (ActionCode == ContextActionConfirmBundleDrop)
	{
		DismissContextMenu();
		HandleConfirmSpatialBundleDrop();
		Manager->RefreshSearchContainerWidget();
		return;
	}
	if (ActionCode == ContextActionCancelBundleDrop)
	{
		DismissContextMenu();
		HandleCancelSpatialBundleDrop();
		return;
	}
	if (ActionCode == ContextActionTake)
	{
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
			FindContainerEntry(ContextSourceContext, ContextSourceSlotIndex);
		if (Entry && Entry->State == Edemo_mapRuntimeContainerEntryState::Identified
			&& Entry->bCanTake)
		{
			Fdemo_mapRuntimeContainerIntent Intent;
			Intent.ExpectedRunId = Snapshot.OwningRunId;
			Intent.ContainerId = Snapshot.ContainerId;
			Intent.ExpectedRevision = Snapshot.Revision;
			Intent.EntryId = Entry->EntryId;
			Intent.Action = Edemo_mapRuntimeContainerActionKind::Take;
			LastResult = Manager->SubmitSearchContainerIntent(Intent);
		}
		DismissContextMenu();
		return;
	}
	if (ActionCode == ContextActionEquip)
	{
		const Fdemo_mapItemInstance* Item = Items->GetAuthority().FindInstance(ContextSourceItemId);
		const Fdemo_mapItemDefinition* Definition = Item
			? Fdemo_mapItemDefinitions::Find(Item->DefinitionId) : nullptr;
		if (Definition && !Definition->CompatibleSlotIds.IsEmpty())
		{
			const Fdemo_mapItemOperationResult Result = Items->Equip(
				ContextSourceItemId,
				Definition->CompatibleSlotIds[0]);
			if (DiagnosticText) DiagnosticText->SetText(FText::FromString(Result.Diagnostic));
			Manager->RefreshSearchContainerWidget();
		}
		DismissContextMenu();
		return;
	}
	if (ActionCode == ContextActionUnequip)
	{
		const int32 EquipmentIndex = ContextSourceSlotIndex - 100;
		const TArray<FName>& EquipmentSlots =
			Fdemo_mapItemDefinitions::GetEquipmentSlotIds();
		const FName EquipmentSlotId = EquipmentSlots.IsValidIndex(EquipmentIndex)
			? EquipmentSlots[EquipmentIndex]
			: NAME_None;
		if (!EquipmentSlotId.IsNone())
		{
			const Fdemo_mapItemOperationResult Result =
				Items->Unequip(EquipmentSlotId);
			if (DiagnosticText)
			{
				DiagnosticText->SetText(FText::FromString(Result.Diagnostic));
			}
			Manager->RefreshSearchContainerWidget();
		}
		DismissContextMenu();
		return;
	}
	if (ActionCode >= ContextActionBindFirst
		&& ActionCode < ContextActionBindFirst + Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		const Fdemo_mapItemOperationResult Result = Items->BindHotbarSlot(
			ActionCode - ContextActionBindFirst + 1,
			ContextSourceItemId);
		if (DiagnosticText) DiagnosticText->SetText(FText::FromString(Result.Diagnostic));
		Manager->RefreshSearchContainerWidget();
		DismissContextMenu();
		return;
	}
	if (ActionCode == ContextActionDrop)
	{
		HandleItemCellDrop(
			ContextSourceSlotIndex,
			ContextSourceItemId,
			ContextSourceContext,
			0,
			Edemo_mapItemPresentationContext::RuntimeWorldDrop);
		if (!PendingSpatialBundleDrop.ExpectedSourceItemInstanceId.IsValid())
		{
			DismissContextMenu();
			Manager->RefreshSearchContainerWidget();
		}
	}
	if (ActionCode == ContextActionReturn)
	{
		const Fdemo_mapRuntimeContainerSectionSnapshot* Chest =
			Snapshot.Sections.FindByPredicate(
				[](const Fdemo_mapRuntimeContainerSectionSnapshot& Section)
				{
					return Section.Section == Edemo_mapRuntimeContainerSection::Chest;
				});
		if (!Chest || !Snapshot.bPlayerDepositAllowed)
		{
			if (DiagnosticText) DiagnosticText->SetText(FText::FromString(
				TEXT("当前目标不接受放回。")));
			DismissContextMenu();
			return;
		}
		int32 EmptySlot = INDEX_NONE;
		for (int32 SlotIndex = 0; SlotIndex < Chest->Capacity; ++SlotIndex)
		{
			if (!Chest->OrderedOccupiedEntries.ContainsByPredicate(
				[SlotIndex](const Fdemo_mapRuntimeContainerEntrySnapshot& Entry)
				{
					return Entry.SlotIndex == SlotIndex
						&& Entry.State != Edemo_mapRuntimeContainerEntryState::Taken;
				}))
			{
				EmptySlot = SlotIndex;
				break;
			}
		}
		if (EmptySlot == INDEX_NONE)
		{
			if (DiagnosticText) DiagnosticText->SetText(FText::FromString(
				TEXT("没有可放回的空格。")));
		}
		else
		{
			HandleItemCellDrop(
				ContextSourceSlotIndex,
				ContextSourceItemId,
				ContextSourceContext,
				EmptySlot,
				Edemo_mapItemPresentationContext::SearchContainerGrid);
		}
		DismissContextMenu();
		Manager->RefreshSearchContainerWidget();
		return;
	}
	if (ActionCode == ContextActionCancel)
	{
		DismissContextMenu();
		return;
	}
}

void Udemo_mapSearchContainerWidget::BuildInterface()
{
	if (!WidgetTree || bBuilt)
	{
		return;
	}
	bBuilt = true;
	SetIsFocusable(true);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	RootCanvas = Canvas;
	WidgetTree->RootWidget = Canvas;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.025f, 0.025f, 0.035f, 0.96f));
	Canvas->AddChild(Panel);
	if (UCanvasPanelSlot* PanelLayout = Cast<UCanvasPanelSlot>(Panel->Slot))
	{
		// Give the interaction page enough horizontal and vertical room for the
		// enlarged multi-column cells at 1280x720 as well as 1600x900.
		PanelLayout->SetAnchors(FAnchors(0.05f, 0.06f, 0.95f, 0.94f));
		PanelLayout->SetOffsets(FMargin(0.0f));
	}
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Root);
	HeaderText = MakeContainerText(
		WidgetTree,
		TEXT("物品交互 / LOOT GRID  ·  ESC 关闭"),
		22);
	StateText = MakeContainerText(WidgetTree, TEXT("CLOSED"), 20);
	InventoryText = MakeContainerText(WidgetTree, TEXT("Run Inventory 0 / 0"), 19);
	AddContainerPadded(Root, HeaderText, 10.0f);
	AddContainerPadded(Root, StateText, 4.0f);
	AddContainerPadded(Root, InventoryText, 4.0f);

	UHorizontalBox* DualColumns =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* ColumnsSlot =
		Root->AddChildToVerticalBox(DualColumns))
	{
		ColumnsSlot->SetPadding(FMargin(7.0f));
		ColumnsSlot->SetSize(
			FSlateChildSize(ESlateSizeRule::Fill));
	}
	UBorder* PlayerPanel = WidgetTree->ConstructWidget<UBorder>();
	PlayerPanel->SetBrushColor(
		FLinearColor(0.035f, 0.09f, 0.12f, 0.96f));
	UVerticalBox* PlayerBox =
		WidgetTree->ConstructWidget<UVerticalBox>();
	PlayerPanel->SetContent(PlayerBox);
	if (UHorizontalBoxSlot* PlayerSlot =
		DualColumns->AddChildToHorizontalBox(PlayerPanel))
	{
		PlayerSlot->SetPadding(FMargin(3.0f));
		// The player column is the active loadout surface, so it deliberately
		// receives the larger share of the dual-panel layout.
		FSlateChildSize PlayerColumnSize(ESlateSizeRule::Fill);
		PlayerColumnSize.Value = 1.2f;
		PlayerSlot->SetSize(PlayerColumnSize);
	}
	PlayerSummaryText = MakeContainerText(
		WidgetTree,
		TEXT("玩家实体 / PLAYER"),
		18);
	AddContainerPadded(PlayerBox, PlayerSummaryText, 10.0f);
	AddContainerPadded(PlayerBox, MakeContainerText(
		WidgetTree, TEXT("拖拽物品到空格；右键操作"), 14), 3.0f);
	UScrollBox* PlayerScroll = WidgetTree->ConstructWidget<UScrollBox>();
	if (UVerticalBoxSlot* PlayerScrollSlot =
		PlayerBox->AddChildToVerticalBox(PlayerScroll))
	{
		PlayerScrollSlot->SetPadding(FMargin(2.0f));
		PlayerScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	PlayerDragBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PlayerScroll->AddChild(PlayerDragBox);
	Fdemo_mapUnifiedItemCellView WorldDropView;
	WorldDropView.SlotIndex = 0;
	WorldDropView.DisplayName = TEXT("丢弃到世界 / DROP TO WORLD");
	WorldDropView.IconLabel = TEXT("WORLD");
	WorldDropView.LevelLabel = TEXT("局内整件／整堆");
	WorldDropView.QualityLabel = TEXT("拖入此格");
	WorldDropView.bUnavailable = false;
	WorldDropCell = CreateWidget<Udemo_mapItemCellWidget>(
		GetOwningPlayer(), Udemo_mapItemCellWidget::StaticClass());
	WorldDropCell->InitializeCell(
		WorldDropView,
		Fdemo_mapItemCellActivated(),
		Edemo_mapItemPresentationContext::RuntimeWorldDrop,
		Fdemo_mapItemCellDropped::CreateUObject(
			this, &Udemo_mapSearchContainerWidget::HandleItemCellDrop));
	AddContainerPadded(PlayerBox, WorldDropCell, 3.0f);
	// This is a first-class Loot-page drag destination, not an invisible
	// fallback.  Dropping there delegates to the same ItemAuthority world path.
	WorldDropCell->SetVisibility(ESlateVisibility::Visible);
	UHorizontalBox* BundleActions = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddContainerPadded(PlayerBox, BundleActions, 2.0f);
	BundleActions->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmSpatialBundleDropButton = WidgetTree->ConstructWidget<UButton>();
	ConfirmSpatialBundleDropButton->SetContent(MakeContainerText(
		WidgetTree, TEXT("确认丢弃空间 Bundle"), 14));
	ConfirmSpatialBundleDropButton->OnClicked.AddDynamic(
		this, &Udemo_mapSearchContainerWidget::ClickConfirmSpatialBundleDrop);
	if (UHorizontalBoxSlot* ConfirmSlot =
		BundleActions->AddChildToHorizontalBox(ConfirmSpatialBundleDropButton))
	{
		ConfirmSlot->SetPadding(FMargin(2.0f));
	}
	CancelSpatialBundleDropButton = WidgetTree->ConstructWidget<UButton>();
	CancelSpatialBundleDropButton->SetContent(MakeContainerText(
		WidgetTree, TEXT("取消 Bundle"), 14));
	CancelSpatialBundleDropButton->OnClicked.AddDynamic(
		this, &Udemo_mapSearchContainerWidget::ClickCancelSpatialBundleDrop);
	if (UHorizontalBoxSlot* CancelSlot =
		BundleActions->AddChildToHorizontalBox(CancelSpatialBundleDropButton))
	{
		CancelSlot->SetPadding(FMargin(2.0f));
	}

	UBorder* TargetPanel = WidgetTree->ConstructWidget<UBorder>();
	TargetPanel->SetBrushColor(
		FLinearColor(0.12f, 0.055f, 0.035f, 0.96f));
	UVerticalBox* TargetBox =
		WidgetTree->ConstructWidget<UVerticalBox>();
	TargetPanel->SetContent(TargetBox);
	if (UHorizontalBoxSlot* TargetSlot =
		DualColumns->AddChildToHorizontalBox(TargetPanel))
	{
		TargetSlot->SetPadding(FMargin(3.0f));
		FSlateChildSize TargetColumnSize(ESlateSizeRule::Fill);
		TargetColumnSize.Value = 0.8f;
		TargetSlot->SetSize(TargetColumnSize);
	}
	TargetSummaryText = MakeContainerText(
		WidgetTree,
		TEXT("当前目标 / TARGET"),
		18);
	AddContainerPadded(TargetBox, TargetSummaryText, 10.0f);
	AddContainerPadded(TargetBox, MakeContainerText(
		WidgetTree, TEXT("左键搜索未知物品；右键拿取；拖回玩家空格"), 14), 3.0f);
	UScrollBox* TargetScroll = WidgetTree->ConstructWidget<UScrollBox>();
	if (UVerticalBoxSlot* TargetScrollSlot =
		TargetBox->AddChildToVerticalBox(TargetScroll))
	{
		TargetScrollSlot->SetPadding(FMargin(2.0f));
		TargetScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UVerticalBox* TargetContent = WidgetTree->ConstructWidget<UVerticalBox>();
	TargetScroll->AddChild(TargetContent);
	TargetDragBox = WidgetTree->ConstructWidget<UVerticalBox>();
	AddContainerPadded(TargetContent, TargetDragBox, 2.0f);

	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddContainerPadded(TargetContent, Tabs, 7.0f);
	Tabs->SetVisibility(ESlateVisibility::Collapsed);
	auto AddTab = [this, Tabs](
		Edemo_mapRuntimeContainerSection Section,
		const FString& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->SetContent(MakeContainerText(WidgetTree, Label, 17));
		if (UHorizontalBoxSlot* LayoutSlot = Tabs->AddChildToHorizontalBox(Button))
		{
			LayoutSlot->SetPadding(FMargin(3.0f));
		}
		SectionButtons.Add(Section, Button);
	};
	AddTab(Edemo_mapRuntimeContainerSection::Chest, TEXT("Chest"));
	AddTab(Edemo_mapRuntimeContainerSection::Equipment, TEXT("Equipment"));
	AddTab(Edemo_mapRuntimeContainerSection::Backpack, TEXT("Backpack"));
	AddTab(Edemo_mapRuntimeContainerSection::Body, TEXT("Body"));
	SectionButtons.FindChecked(Edemo_mapRuntimeContainerSection::Chest)->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickChestSection);
	SectionButtons.FindChecked(Edemo_mapRuntimeContainerSection::Equipment)->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickEquipmentSection);
	SectionButtons.FindChecked(Edemo_mapRuntimeContainerSection::Backpack)->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickBackpackSection);
	SectionButtons.FindChecked(Edemo_mapRuntimeContainerSection::Body)->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickBodySection);

	for (int32 Index = 0; Index < 6; ++Index)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* Text = MakeContainerText(
			WidgetTree,
			FString::Printf(TEXT("%02d — EMPTY —"), Index + 1),
			17);
		Button->SetContent(Text);
		AddContainerPadded(TargetContent, Button, 2.0f);
		RowButtons.Add(Button);
		RowTexts.Add(Text);
		Button->SetVisibility(ESlateVisibility::Collapsed);
	}
	RowButtons[0]->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickRow00);
	RowButtons[1]->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickRow01);
	RowButtons[2]->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickRow02);
	RowButtons[3]->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickRow03);
	RowButtons[4]->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickRow04);
	RowButtons[5]->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickRow05);

	UHorizontalBox* PageNavigation =
		WidgetTree->ConstructWidget<UHorizontalBox>();
	AddContainerPadded(TargetContent, PageNavigation, 3.0f);
	PageNavigation->SetVisibility(ESlateVisibility::Collapsed);
	PreviousPageButton = WidgetTree->ConstructWidget<UButton>();
	PreviousPageButton->SetContent(MakeContainerText(
		WidgetTree,
		TEXT("上一组 / PREV"),
		15));
	PreviousPageButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickPreviousPage);
	if (UHorizontalBoxSlot* PreviousSlot =
		PageNavigation->AddChildToHorizontalBox(PreviousPageButton))
	{
		PreviousSlot->SetPadding(FMargin(3.0f));
	}
	PageText = MakeContainerText(WidgetTree, TEXT("1 / 1"), 15);
	if (UHorizontalBoxSlot* PageSlot =
		PageNavigation->AddChildToHorizontalBox(PageText))
	{
		PageSlot->SetPadding(FMargin(10.0f, 5.0f));
	}
	NextPageButton = WidgetTree->ConstructWidget<UButton>();
	NextPageButton->SetContent(MakeContainerText(
		WidgetTree,
		TEXT("下一组 / NEXT"),
		15));
	NextPageButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickNextPage);
	if (UHorizontalBoxSlot* NextSlot =
		PageNavigation->AddChildToHorizontalBox(NextPageButton))
	{
		NextSlot->SetPadding(FMargin(3.0f));
	}

	DetailText = MakeContainerText(
		WidgetTree,
		TEXT("选择已识别物品查看详情"),
		16);
	AddContainerPadded(TargetContent, DetailText, 8.0f);
	DetailText->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddContainerPadded(TargetBox, Actions, 4.0f);
	Actions->SetVisibility(ESlateVisibility::Collapsed);
	TakeButton = WidgetTree->ConstructWidget<UButton>();
	TakeButton->SetContent(MakeContainerText(
		WidgetTree,
		TEXT("取出 / TAKE"),
		17));
	TakeButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickTake);
	if (UHorizontalBoxSlot* TakeSlot =
		Actions->AddChildToHorizontalBox(TakeButton))
	{
		TakeSlot->SetPadding(FMargin(3.0f));
	}
	ReturnButton = WidgetTree->ConstructWidget<UButton>();
	ReturnButton->SetContent(MakeContainerText(
		WidgetTree,
		TEXT("放回 / RETURN (拖拽)"),
		17));
	ReturnButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickReturn);
	if (UHorizontalBoxSlot* ReturnSlot =
		Actions->AddChildToHorizontalBox(ReturnButton))
	{
		ReturnSlot->SetPadding(FMargin(3.0f));
	}
	CancelSearchButton = WidgetTree->ConstructWidget<UButton>();
	CancelSearchButton->SetContent(MakeContainerText(
		WidgetTree,
		TEXT("取消读取 / CANCEL SEARCH"),
		17));
	CancelSearchButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSearchContainerWidget::ClickCancelSearch);
	if (UHorizontalBoxSlot* CancelSlot =
		Actions->AddChildToHorizontalBox(CancelSearchButton))
	{
		CancelSlot->SetPadding(FMargin(3.0f));
	}

	HotbarText = MakeContainerText(
		WidgetTree,
		TEXT("1—9 快捷栏"),
		16);
	AddContainerPadded(Root, HotbarText, 5.0f);
	DiagnosticText = MakeContainerText(WidgetTree, TEXT("Ready"), 16);
	AddContainerPadded(Root, DiagnosticText, 5.0f);
	CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetContent(MakeContainerText(WidgetTree, TEXT("关闭 / CLOSE"), 19));
	CloseButton->OnClicked.AddDynamic(this, &Udemo_mapSearchContainerWidget::ClickClose);
	AddContainerPadded(Root, CloseButton, 5.0f);
	CloseButton->SetVisibility(ESlateVisibility::Collapsed);
}

void Udemo_mapSearchContainerWidget::RefreshFromSnapshot(
	const Fdemo_mapRuntimeContainerSnapshot& InSnapshot)
{
	Snapshot = InSnapshot;
	if (Manager.IsValid())
	{
		if (const Udemo_mapItemSubsystem* ItemSubsystem =
			Manager->GetItemSubsystem())
		{
			ViewState = Fdemo_mapSearchContainerPresenter::BuildRuntime(
				Snapshot,
				ItemSubsystem->GetAuthority(),
				ItemSubsystem->GetHotbarBindingSnapshot(),
				ItemSubsystem->GetItemUseCooldownSnapshot());
		}
		else
		{
			ViewState = Fdemo_mapSearchContainerPresenter::Build(Snapshot);
		}
	}
	else
	{
		ViewState = Fdemo_mapSearchContainerPresenter::Build(Snapshot);
	}
	const Fdemo_mapSearchContainerViewRow* SelectedRow =
		FindRowByEntryId(SelectedEntryId);
	if (!SelectedRow
		|| SelectedRow->State
			!= Edemo_mapRuntimeContainerEntryState::Identified)
	{
		SelectedEntryId.Invalidate();
	}
	if (!ViewState.Sections.ContainsByPredicate(
		[this](const Fdemo_mapSearchContainerSectionView& Section)
		{
			return Section.Section == SelectedSection;
		})
		&& !ViewState.Sections.IsEmpty())
	{
		SelectedSection = ViewState.Sections[0].Section;
		VisibleRowOffset = 0;
	}
	RefreshVisuals();
	// The main panel is rebuilt from the new snapshot above.  Keep either
	// already-open nested bag on the same live projection so a successful drag
	// immediately changes both its source and destination cells.
	if (PlayerBagWindow)
	{
		OpenPlayerBagWindow();
	}
	if (TargetBagWindow)
	{
		OpenTargetBagWindow();
	}
}

const Fdemo_mapSearchContainerSectionView*
Udemo_mapSearchContainerWidget::FindSelectedSectionView() const
{
	return ViewState.Sections.FindByPredicate(
		[this](const Fdemo_mapSearchContainerSectionView& Section)
		{
			return Section.Section == SelectedSection;
		});
}

int32 Udemo_mapSearchContainerWidget::GetSelectedVisibleRowCount() const
{
	const Fdemo_mapSearchContainerSectionView* SectionView =
		FindSelectedSectionView();
	if (!SectionView)
	{
		return 0;
	}
	if (!ViewState.bCorpse
		|| SelectedSection
			!= Edemo_mapRuntimeContainerSection::Backpack)
	{
		return SectionView->Rows.Num();
	}
	int32 VisibleCapacity = 0;
	for (const Fdemo_mapDualLootTargetRegionView& Region :
		ViewState.TargetRegions)
	{
		if (Region.Region
				== Edemo_mapDualLootTargetRegion::BaseQuickItems
			|| Region.Region
				== Edemo_mapDualLootTargetRegion::SpatialStorage)
		{
			VisibleCapacity += Region.Capacity;
		}
	}
	return FMath::Min(SectionView->Rows.Num(), VisibleCapacity);
}

const Fdemo_mapSearchContainerViewRow*
Udemo_mapSearchContainerWidget::FindRowByEntryId(FGuid EntryId) const
{
	if (!EntryId.IsValid())
	{
		return nullptr;
	}
	for (const Fdemo_mapSearchContainerSectionView& Section :
		ViewState.Sections)
	{
		if (const Fdemo_mapSearchContainerViewRow* Row =
			Section.Rows.FindByPredicate(
				[EntryId](const Fdemo_mapSearchContainerViewRow& Candidate)
				{
					return Candidate.EntryId == EntryId;
				}))
		{
			return Row;
		}
	}
	return nullptr;
}

FString Udemo_mapSearchContainerWidget::BuildSelectedDetailText() const
{
	if (!SelectedEntryId.IsValid())
	{
		return TEXT("选择已识别物品查看详情；再次点击“取出”执行同 GUID 单件转移。");
	}
	const Udemo_mapItemSubsystem* ItemSubsystem =
		Manager.IsValid() ? Manager->GetItemSubsystem() : nullptr;
	const Fdemo_mapSearchContainerViewRow* SelectedRow =
		FindRowByEntryId(SelectedEntryId);
	const Fdemo_mapItemInstance* Item =
		ItemSubsystem && SelectedRow
			? ItemSubsystem->GetAuthority().FindInstance(
				SelectedRow->ItemInstanceId)
			: nullptr;
	if (!Item)
	{
		return TEXT("所选物品已失效；页面将从真实权威刷新。");
	}
	const Fdemo_mapUnifiedItemDetailView Detail =
		Fdemo_mapItemPresentation::BuildDetail(*Item);
	if (!Detail.bValid)
	{
		return TEXT("物品详情不可用。");
	}
	return FString::Printf(
		TEXT("%s  %s  ×%d\n%s | %s | %s\n%s\n%s\n%s\n%s"),
		*Detail.DisplayName,
		*Detail.LevelLabel,
		Detail.Quantity,
		*Detail.TypeLabel,
		*Detail.QualityLabel,
		*Detail.SellValue,
		*Detail.Description,
		*Detail.BaseAttributes,
		*Detail.RandomAffixes,
		*Detail.AllowedPositions);
}

void Udemo_mapSearchContainerWidget::RefreshVisuals()
{
	if (!bBuilt)
	{
		return;
	}
	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(ViewState.Header));
	}
	if (StateText)
	{
		StateText->SetText(FText::FromString(ViewState.StateText));
	}
	if (InventoryText)
	{
		InventoryText->SetText(FText::FromString(ViewState.InventoryText));
	}
	if (PlayerSummaryText)
	{
		PlayerSummaryText->SetText(
			FText::FromString(ViewState.PlayerLayoutSummary));
	}
	if (TargetSummaryText)
	{
		TargetSummaryText->SetText(
			FText::FromString(ViewState.TargetLayoutSummary));
	}
	if (PlayerDragBox && TargetDragBox)
	{
		PlayerDragBox->ClearChildren();
		TargetDragBox->ClearChildren();
		PlayerDragCells.Reset();
		TargetDragCells.Reset();
		auto EquipmentSlotIndex = [this](
			const Fdemo_mapEntityItemSlotView& ItemSlot)
		{
			switch (ItemSlot.Region)
			{
			case Edemo_mapEntityLoadoutRegion::Weapon:
				return 0;
			case Edemo_mapEntityLoadoutRegion::Armor:
				return 1;
			case Edemo_mapEntityLoadoutRegion::Accessory:
				return 2;
			case Edemo_mapEntityLoadoutRegion::SpatialRing:
				return 3;
			case Edemo_mapEntityLoadoutRegion::SpatialItem:
				return 4;
			default:
				return ItemSlot.SlotIndex;
			}
		};
		auto EquipmentSlotLabel = [](Edemo_mapEntityLoadoutRegion Region)
		{
			switch (Region)
			{
			case Edemo_mapEntityLoadoutRegion::Weapon:
				return FString(TEXT("兵器\nWEAPON"));
			case Edemo_mapEntityLoadoutRegion::Armor:
				return FString(TEXT("道袍\nROBE"));
			case Edemo_mapEntityLoadoutRegion::Accessory:
				return FString(TEXT("饰品\nACCESSORY"));
			case Edemo_mapEntityLoadoutRegion::SpatialRing:
				return FString(TEXT("空间戒指\nSPATIAL RING"));
			case Edemo_mapEntityLoadoutRegion::SpatialItem:
				return FString(TEXT("空间袋\nSPATIAL BAG"));
			default:
				return FString(TEXT("装备\nEQUIPMENT"));
			}
		};
		auto AddPlayerCells = [this, &EquipmentSlotIndex, &EquipmentSlotLabel](
			const FString& Label,
			const TArray<Fdemo_mapEntityItemSlotView>& Slots,
			Edemo_mapItemPresentationContext Context,
			bool bEquipment,
			int32 InventoryOffset,
			const FString& AreaLabel)
		{
			if (Slots.IsEmpty()) return;
			AddContainerPadded(PlayerDragBox,
				MakeContainerText(WidgetTree, Label, 14), 1.0f);
			UWrapBox* Grid = WidgetTree->ConstructWidget<UWrapBox>();
			Grid->SetInnerSlotPadding(FVector2D(3.0f, 3.0f));
			AddContainerPadded(PlayerDragBox, Grid, 1.0f);
			for (int32 Index = 0; Index < Slots.Num(); ++Index)
			{
				Fdemo_mapUnifiedItemCellView Cell =
					Fdemo_mapItemPresentation::BuildCell(Slots[Index]);
				Cell.SlotIndex = bEquipment
					? 100 + EquipmentSlotIndex(Slots[Index])
					: InventoryOffset + Slots[Index].SlotIndex;
				Cell.ShortcutLabel = bEquipment
					? TEXT("装备")
					: AreaLabel;
				Udemo_mapItemCellWidget* CellWidget =
					CreateWidget<Udemo_mapItemCellWidget>(
						GetOwningPlayer(), Udemo_mapItemCellWidget::StaticClass());
				CellWidget->InitializeCell(
					Cell,
					Fdemo_mapItemCellActivated(),
					Context,
					Fdemo_mapItemCellDropped::CreateUObject(
						this,
						&Udemo_mapSearchContainerWidget::HandleItemCellDrop),
				Fdemo_mapItemCellContextRequested::CreateUObject(
					this,
					&Udemo_mapSearchContainerWidget::HandleItemCellContextRequested),
				Fdemo_mapItemCellDoubleClicked::CreateLambda(
					[this, Context](int32 SlotIndex)
					{
						HandleItemCellDoubleClicked(SlotIndex, Context);
					}),
				bEquipment && Slots[Index].Region
					== Edemo_mapEntityLoadoutRegion::SpatialItem);
				if (bEquipment)
				{
					// Explicit equipment labels make the spatial ring observable as
					// its own main slot instead of an anonymous accessory icon.
					USizeBox* EquipmentCell =
						WidgetTree->ConstructWidget<USizeBox>();
					EquipmentCell->SetWidthOverride(78.0f);
					UVerticalBox* EquipmentColumn =
						WidgetTree->ConstructWidget<UVerticalBox>();
					EquipmentCell->SetContent(EquipmentColumn);
					UTextBlock* SlotTitle = MakeContainerText(
						WidgetTree,
						EquipmentSlotLabel(Slots[Index].Region),
						11);
					SlotTitle->SetJustification(ETextJustify::Center);
					EquipmentColumn->AddChildToVerticalBox(SlotTitle);
					EquipmentColumn->AddChildToVerticalBox(CellWidget);
					Grid->AddChildToWrapBox(EquipmentCell);
				}
				else
				{
					Grid->AddChildToWrapBox(CellWidget);
				}
				PlayerDragCells.Add(CellWidget);
			}
		};
		TArray<Fdemo_mapEntityItemSlotView> MainEquipmentSlots;
		for (const Fdemo_mapEntityItemSlotView& ItemSlot :
			ViewState.PlayerLoadout.EquipmentSlots)
		{
			MainEquipmentSlots.Add(ItemSlot);
		}
		AddPlayerCells(TEXT("大型装备栏 / EQUIPMENT"), MainEquipmentSlots,
			Edemo_mapItemPresentationContext::RuntimeEquipment, true, 0, TEXT("装备"));
		AddPlayerCells(FString::Printf(TEXT("纳物戒快捷 %d 格"),
			ViewState.PlayerLoadout.RingQuickItemCapacity),
			ViewState.PlayerLoadout.RingQuickItemSlots,
			Edemo_mapItemPresentationContext::RuntimeRingQuickItems,
			false,
			Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount,
			TEXT("戒"));
		AddPlayerCells(TEXT("身体背囊 / BASE QUICK（固定 6 格）"),
			ViewState.PlayerLoadout.BaseQuickItemSlots,
			Edemo_mapItemPresentationContext::RuntimeBaseQuickItems, false, 0, TEXT("背"));
		AddContainerPadded(
			PlayerDragBox,
			MakeContainerText(
				WidgetTree,
				FString::Printf(
					TEXT("空间袋入口 / SPATIAL BAG（%d 格；双击上方空间袋图标打开独立窗口）"),
					ViewState.PlayerLoadout.SpatialStorageCapacity),
				14),
			1.0f);

		auto ContextForTargetRegion = [](Edemo_mapDualLootTargetRegion Region)
		{
			switch (Region)
			{
			case Edemo_mapDualLootTargetRegion::ContainerGrid:
				return Edemo_mapItemPresentationContext::SearchContainerGrid;
			case Edemo_mapDualLootTargetRegion::Weapon:
			case Edemo_mapDualLootTargetRegion::Armor:
			case Edemo_mapDualLootTargetRegion::Accessory:
			case Edemo_mapDualLootTargetRegion::SpatialRing:
			case Edemo_mapDualLootTargetRegion::SpatialItem:
				return Edemo_mapItemPresentationContext::SearchContainerEquipment;
			case Edemo_mapDualLootTargetRegion::BaseQuickItems:
				return Edemo_mapItemPresentationContext::SearchContainerBaseQuickItems;
			case Edemo_mapDualLootTargetRegion::SpatialStorage:
				return Edemo_mapItemPresentationContext::SearchContainerSpatialStorage;
			default:
				return Edemo_mapItemPresentationContext::SearchContainerBody;
			}
		};
		for (const Fdemo_mapDualLootTargetRegionView& Region :
			ViewState.TargetRegions)
		{
			if (Region.Region == Edemo_mapDualLootTargetRegion::SpatialStorage)
			{
				// Its bag icon remains in the equipment group; contents are exposed
				// only by its independent movable window, never as a 36-row page.
				continue;
			}
			AddContainerPadded(TargetDragBox,
				MakeContainerText(WidgetTree, Region.Label, 14), 1.0f);
			UWrapBox* Grid = WidgetTree->ConstructWidget<UWrapBox>();
			Grid->SetInnerSlotPadding(FVector2D(3.0f, 3.0f));
			AddContainerPadded(TargetDragBox, Grid, 1.0f);
			for (const Fdemo_mapDualLootTargetCellView& TargetCell : Region.Cells)
			{
				Fdemo_mapUnifiedItemCellView Cell = TargetCell.Cell;
				Cell.SlotIndex = TargetCell.SourceSlotIndex;
				Cell.ShortcutLabel = TargetCell.StatusLabel;
				// Hidden loot remains clickable so the player can begin the
				// identification search.  Only a running search is locked.
				Cell.bUnavailable = TargetCell.State
					== Edemo_mapRuntimeContainerEntryState::Searching;
				Cell.bAllowDrag = TargetCell.State
					== Edemo_mapRuntimeContainerEntryState::Identified;
				const Edemo_mapItemPresentationContext TargetContext =
					ContextForTargetRegion(Region.Region);
				Udemo_mapItemCellWidget* CellWidget =
					CreateWidget<Udemo_mapItemCellWidget>(
						GetOwningPlayer(), Udemo_mapItemCellWidget::StaticClass());
				CellWidget->InitializeCell(
					Cell,
					Fdemo_mapItemCellActivated::CreateLambda(
						[this, TargetContext](int32 SlotIndex)
						{
							HandleTargetItemCellActivated(SlotIndex, TargetContext);
						}),
					TargetContext,
					Fdemo_mapItemCellDropped::CreateUObject(
						this,
						&Udemo_mapSearchContainerWidget::HandleItemCellDrop),
				Fdemo_mapItemCellContextRequested::CreateUObject(
					this,
					&Udemo_mapSearchContainerWidget::HandleItemCellContextRequested),
				Fdemo_mapItemCellDoubleClicked::CreateLambda(
					[this, TargetContext](int32 SlotIndex)
					{
						HandleItemCellDoubleClicked(SlotIndex, TargetContext);
					}),
				Region.Region == Edemo_mapDualLootTargetRegion::SpatialItem);
				Grid->AddChildToWrapBox(CellWidget);
				TargetDragCells.Add(CellWidget);
			}
		}
	}
	const bool bBundleConfirmationPending =
		PendingSpatialBundleDrop.ExpectedSourceItemInstanceId.IsValid();
	if (ConfirmSpatialBundleDropButton)
	{
		ConfirmSpatialBundleDropButton->SetVisibility(
			bBundleConfirmationPending
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (CancelSpatialBundleDropButton)
	{
		CancelSpatialBundleDropButton->SetVisibility(
			bBundleConfirmationPending
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DetailText)
	{
		DetailText->SetText(
			FText::FromString(BuildSelectedDetailText()));
	}
	if (HotbarText)
	{
		TArray<FString> HotbarParts;
		for (const Fdemo_mapHotbarSlotView& HotbarSlot : ViewState.Hotbar)
		{
			const FKey Key =
				Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionRegistry::HotbarActionId(
						HotbarSlot.SlotNumber));
			FString Label =
				Fdemo_mapItemPresentation::BuildHotbarSlotLabel(
					HotbarSlot,
					Key.GetDisplayName().ToString());
			Label.ReplaceInline(TEXT("\r"), TEXT(" "));
			Label.ReplaceInline(TEXT("\n"), TEXT(" "));
			HotbarParts.Add(MoveTemp(Label));
		}
		HotbarText->SetText(FText::FromString(FString::Printf(
			TEXT("快捷栏 / HOTBAR\n%s"),
			*FString::Join(HotbarParts, TEXT("  |  ")))));
	}
	if (DiagnosticText)
	{
		DiagnosticText->SetText(FText::FromString(ViewState.DiagnosticText.IsEmpty()
			? TEXT("Ready")
			: ViewState.DiagnosticText));
	}
	// Legacy row, pager and footer controls remain allocated only for existing
	// automation entry points.  The runtime surface is exclusively the compact
	// grid plus the item right-click menu.
	for (TPair<Edemo_mapRuntimeContainerSection, TObjectPtr<UButton>>& Pair : SectionButtons)
	{
		Pair.Value->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UButton* Button : RowButtons)
	{
		if (Button) Button->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PreviousPageButton) PreviousPageButton->SetVisibility(ESlateVisibility::Collapsed);
	if (NextPageButton) NextPageButton->SetVisibility(ESlateVisibility::Collapsed);
	if (PageText) PageText->SetVisibility(ESlateVisibility::Collapsed);
	if (TakeButton) TakeButton->SetVisibility(ESlateVisibility::Collapsed);
	if (ReturnButton) ReturnButton->SetVisibility(ESlateVisibility::Collapsed);
	if (CancelSearchButton) CancelSearchButton->SetVisibility(ESlateVisibility::Collapsed);
	if (CloseButton) CloseButton->SetVisibility(ESlateVisibility::Collapsed);
	return;
	for (TPair<Edemo_mapRuntimeContainerSection, TObjectPtr<UButton>>& Pair :
		SectionButtons)
	{
		const bool bVisibleSection = ViewState.Sections.ContainsByPredicate(
			[&Pair](const Fdemo_mapSearchContainerSectionView& Section)
			{
				return Section.Section == Pair.Key;
			});
		Pair.Value->SetVisibility(
			bVisibleSection
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	const Fdemo_mapSearchContainerSectionView* SectionView =
		FindSelectedSectionView();
	constexpr int32 RowsPerPage = 6;
	const int32 RowCount = GetSelectedVisibleRowCount();
	const int32 MaximumOffset =
		RowCount > 0
			? ((RowCount - 1) / RowsPerPage) * RowsPerPage
			: 0;
	VisibleRowOffset = FMath::Clamp(VisibleRowOffset, 0, MaximumOffset);
	for (int32 Index = 0; Index < RowButtons.Num(); ++Index)
	{
		const int32 RowIndex = VisibleRowOffset + Index;
		if (SectionView
			&& RowIndex < RowCount
			&& SectionView->Rows.IsValidIndex(RowIndex))
		{
			const Fdemo_mapSearchContainerViewRow& Row =
				SectionView->Rows[RowIndex];
			RowButtons[Index]->SetVisibility(ESlateVisibility::Visible);
			RowButtons[Index]->SetIsEnabled(Row.bActionEnabled);
			FString RegionPrefix;
			if (ViewState.bCorpse
				&& SelectedSection
					== Edemo_mapRuntimeContainerSection::Backpack)
			{
				RegionPrefix =
					Row.SlotIndex
						< Fdemo_mapSearchContainerPrototypeConfig::
							CorpseBaseQuickItemCapacity
					? TEXT("基础快捷 / BASE QUICK  ")
					: TEXT("空间储物 / SPATIAL  ");
			}
			FString CompactRow = Row.Text;
			const Fdemo_mapRuntimeContainerSectionSnapshot* SnapshotSection =
				Snapshot.Sections.FindByPredicate(
					[this](
						const Fdemo_mapRuntimeContainerSectionSnapshot&
							Candidate)
					{
						return Candidate.Section == SelectedSection;
					});
			const Fdemo_mapRuntimeContainerEntrySnapshot* SnapshotEntry =
				SnapshotSection
					? SnapshotSection->OrderedOccupiedEntries.FindByPredicate(
						[&Row](
							const Fdemo_mapRuntimeContainerEntrySnapshot&
								Candidate)
						{
							return Candidate.SlotIndex == Row.SlotIndex;
						})
					: nullptr;
			if (SnapshotEntry
				&& Row.State
					== Edemo_mapRuntimeContainerEntryState::Identified)
			{
				CompactRow = FString::Printf(
					TEXT("%02d  %s  Lv.%d  ×%d  [详情 / 取出]"),
					Row.SlotIndex + 1,
					*SnapshotEntry->DisplayName.ToString(),
					SnapshotEntry->Level,
					SnapshotEntry->StackCount);
			}
			RowTexts[Index]->SetText(FText::FromString(
				RegionPrefix + CompactRow));
		}
		else
		{
			RowButtons[Index]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	const int32 PageCount =
		FMath::Max(1, FMath::DivideAndRoundUp(RowCount, RowsPerPage));
	const int32 CurrentPage = VisibleRowOffset / RowsPerPage + 1;
	if (PageText)
	{
		PageText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"),
			CurrentPage,
			PageCount)));
	}
	if (PreviousPageButton)
	{
		PreviousPageButton->SetVisibility(
			PageCount > 1
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		PreviousPageButton->SetIsEnabled(VisibleRowOffset > 0);
	}
	if (NextPageButton)
	{
		NextPageButton->SetVisibility(
			PageCount > 1
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		NextPageButton->SetIsEnabled(
			VisibleRowOffset + RowsPerPage < RowCount);
	}
	if (PageText)
	{
		PageText->SetVisibility(
			PageCount > 1
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	if (TakeButton)
	{
		const Fdemo_mapSearchContainerViewRow* SelectedRow =
			FindRowByEntryId(SelectedEntryId);
		TakeButton->SetIsEnabled(
			SelectedRow
			&& SelectedRow->State
				== Edemo_mapRuntimeContainerEntryState::Identified
			&& SelectedRow->bActionEnabled);
	}
	if (CancelSearchButton)
	{
		CancelSearchButton->SetIsEnabled(
			Snapshot.ActiveAction
				== Edemo_mapRuntimeContainerActionKind::BeginSearch);
	}
	if (CloseButton)
	{
		if (UTextBlock* CloseLabel =
			Cast<UTextBlock>(CloseButton->GetContent()))
		{
			CloseLabel->SetText(FText::FromString(FString::Printf(
				TEXT("关闭 / CLOSE [%s]"),
				*Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::Back)
					.GetDisplayName().ToString())));
		}
	}
}

void Udemo_mapSearchContainerWidget::SelectSection(
	Edemo_mapRuntimeContainerSection Section)
{
	if (ViewState.Sections.ContainsByPredicate(
		[Section](const Fdemo_mapSearchContainerSectionView& Candidate)
		{
			return Candidate.Section == Section;
		}))
	{
		SelectedSection = Section;
		VisibleRowOffset = 0;
		RefreshVisuals();
	}
}

void Udemo_mapSearchContainerWidget::ActivateRow(int32 VisualRowIndex)
{
	const Fdemo_mapSearchContainerSectionView* SectionView =
		FindSelectedSectionView();
	if (!Manager.IsValid()
		|| !SectionView
		|| !SectionView->Rows.IsValidIndex(VisualRowIndex))
	{
		return;
	}
	const int32 RowIndex = VisibleRowOffset + VisualRowIndex;
	if (!SectionView->Rows.IsValidIndex(RowIndex))
	{
		return;
	}
	const Fdemo_mapSearchContainerViewRow& Row = SectionView->Rows[RowIndex];
	if (!Row.EntryId.IsValid() || !Row.bActionEnabled)
	{
		return;
	}
	Fdemo_mapRuntimeContainerIntent Intent;
	Intent.ExpectedRunId = Snapshot.OwningRunId;
	Intent.ContainerId = Snapshot.ContainerId;
	Intent.ExpectedRevision = Snapshot.Revision;
	Intent.EntryId = Row.EntryId;
	if (Row.State == Edemo_mapRuntimeContainerEntryState::Hidden)
	{
		SelectedEntryId.Invalidate();
		Intent.Action = Edemo_mapRuntimeContainerActionKind::BeginSearch;
		LastResult = Manager->SubmitSearchContainerIntent(Intent);
	}
	else if (Row.State
		== Edemo_mapRuntimeContainerEntryState::Identified)
	{
		SelectedEntryId = Row.EntryId;
		LastResult = Fdemo_mapRuntimeContainerResult::Success(
			Snapshot.ContainerId,
			Snapshot.Revision,
			Snapshot.Revision,
			Row.EntryId,
			Row.ItemInstanceId);
		RefreshVisuals();
	}
}

void Udemo_mapSearchContainerWidget::HandleItemCellDrop(
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
	auto IsPlayerContext = [](Edemo_mapItemPresentationContext Context)
	{
		return Context == Edemo_mapItemPresentationContext::RuntimeEquipment
			|| Context == Edemo_mapItemPresentationContext::RuntimeBaseQuickItems
			|| Context == Edemo_mapItemPresentationContext::RuntimeRingQuickItems
			|| Context == Edemo_mapItemPresentationContext::RuntimeSpatialStorage;
	};
	auto PlayerAreaFor = [](Edemo_mapItemPresentationContext Context)
	{
		switch (Context)
		{
		case Edemo_mapItemPresentationContext::RuntimeEquipment:
			return Edemo_mapPlayerItemArea::Equipment;
		case Edemo_mapItemPresentationContext::RuntimeBaseQuickItems:
			return Edemo_mapPlayerItemArea::BaseQuickItems;
		case Edemo_mapItemPresentationContext::RuntimeRingQuickItems:
		case Edemo_mapItemPresentationContext::RuntimeSpatialStorage:
			return Edemo_mapPlayerItemArea::SpatialStorage;
		default:
			return Edemo_mapPlayerItemArea::Invalid;
		}
	};
	auto EquipmentForEncodedSlot = [](int32 SlotIndex)
	{
		const int32 Index = SlotIndex - 100;
		const TArray<FName>& Slots =
			Fdemo_mapItemDefinitions::GetEquipmentSlotIds();
		return Slots.IsValidIndex(Index) ? Slots[Index] : NAME_None;
	};
	auto PlayerVisualSlot = [](int32 SlotIndex,
		Edemo_mapItemPresentationContext Context)
	{
		return (Context == Edemo_mapItemPresentationContext::RuntimeRingQuickItems
			|| Context == Edemo_mapItemPresentationContext::RuntimeSpatialStorage)
			? SlotIndex - Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
			: SlotIndex;
	};
	auto ResolveContainerSlot = [](Edemo_mapItemPresentationContext Context,
		int32 SlotIndex,
		Edemo_mapRuntimeContainerSection& OutSection,
		int32& OutSlot) -> bool
	{
		OutSlot = SlotIndex;
		switch (Context)
		{
		case Edemo_mapItemPresentationContext::SearchContainerGrid:
			OutSection = Edemo_mapRuntimeContainerSection::Chest;
			return true;
		case Edemo_mapItemPresentationContext::SearchContainerEquipment:
			OutSection = Edemo_mapRuntimeContainerSection::Equipment;
			return true;
		case Edemo_mapItemPresentationContext::SearchContainerBaseQuickItems:
		case Edemo_mapItemPresentationContext::SearchContainerSpatialStorage:
			OutSection = Edemo_mapRuntimeContainerSection::Backpack;
			return true;
		case Edemo_mapItemPresentationContext::SearchContainerBody:
			OutSection = Edemo_mapRuntimeContainerSection::Body;
			return true;
		default:
			return false;
		}
	};
	auto FindSnapshotEntry = [this](
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex) -> const Fdemo_mapRuntimeContainerEntrySnapshot*
	{
		const Fdemo_mapRuntimeContainerSectionSnapshot* SnapshotSection =
			Snapshot.Sections.FindByPredicate(
				[Section](const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
				{
					return Candidate.Section == Section;
				});
		return SnapshotSection
			? SnapshotSection->OrderedOccupiedEntries.FindByPredicate(
				[SlotIndex](const Fdemo_mapRuntimeContainerEntrySnapshot& Candidate)
				{
					return Candidate.SlotIndex == SlotIndex;
				})
			: nullptr;
	};
	auto BuildPlayerSourceIntent = [&](int32 SlotIndex,
		Edemo_mapItemPresentationContext Context)
	{
		Fdemo_mapPlayerItemDropIntent Result;
		Result.ExpectedAuthorityRevision = Manager->GetItemSubsystem()
			->GetAuthority().GetAuthorityRevision();
		Result.ExpectedSourceItemInstanceId = SourceItemId;
		Result.SourceArea = PlayerAreaFor(Context);
		Result.SourceSlotIndex = PlayerVisualSlot(SlotIndex, Context);
		Result.SourceEquipmentSlotId = Context
			== Edemo_mapItemPresentationContext::RuntimeEquipment
			? EquipmentForEncodedSlot(SlotIndex) : NAME_None;
		return Result;
	};

	if (TargetContext == Edemo_mapItemPresentationContext::RuntimeWorldDrop)
	{
		if (!IsPlayerContext(SourceContext))
		{
			return;
		}
		Fdemo_mapPlayerItemDropIntent DropIntent =
			BuildPlayerSourceIntent(SourceSlotIndex, SourceContext);
		const bool bSpatialEquipment = DropIntent.SourceArea
			== Edemo_mapPlayerItemArea::Equipment
			&& DropIntent.SourceEquipmentSlotId == Fdemo_mapItemIds::BackpackSlot;
		if (bSpatialEquipment
			&& Manager->GetItemSubsystem()->GetAuthority().GetUsedInventorySlots()
				> Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack)
		{
			PendingSpatialBundleDrop = DropIntent;
			if (DiagnosticText)
			{
				DiagnosticText->SetText(FText::FromString(
					TEXT("背包含内部物品：请在右键菜单确认整体丢弃。")));
			}
			ShowSpatialBundleConfirmation();
			return;
		}
		const Fdemo_mapItemOperationResult Operation =
			Manager->RequestDropPlayerItem(DropIntent, false);
		if (DiagnosticText)
		{
			DiagnosticText->SetText(FText::FromString(Operation.Diagnostic));
		}
		return;
	}

	const bool bSourcePlayer = IsPlayerContext(SourceContext);
	const bool bTargetPlayer = IsPlayerContext(TargetContext);
	if (bSourcePlayer && bTargetPlayer)
	{
		Fdemo_mapPlayerItemDropIntent Intent =
			BuildPlayerSourceIntent(SourceSlotIndex, SourceContext);
		Intent.TargetArea = PlayerAreaFor(TargetContext);
		Intent.TargetSlotIndex = PlayerVisualSlot(TargetSlotIndex, TargetContext);
		Intent.TargetEquipmentSlotId = TargetContext
			== Edemo_mapItemPresentationContext::RuntimeEquipment
			? EquipmentForEncodedSlot(TargetSlotIndex) : NAME_None;
		const Fdemo_mapPlayerItemDropResult PlayerResult =
			Manager->GetItemSubsystem()->ExecutePlayerItemDrop(Intent);
		if (DiagnosticText)
		{
			DiagnosticText->SetText(FText::FromString(PlayerResult.Diagnostic));
		}
		RefreshFromSnapshot(Snapshot);
		return;
	}

	Edemo_mapRuntimeContainerSection ContainerSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 ContainerSlotIndex = INDEX_NONE;
	if ((!bSourcePlayer
		&& !ResolveContainerSlot(SourceContext, SourceSlotIndex,
			ContainerSection, ContainerSlotIndex))
		|| (!bTargetPlayer
			&& !ResolveContainerSlot(TargetContext, TargetSlotIndex,
				ContainerSection, ContainerSlotIndex)))
	{
		return;
	}

	Fdemo_mapSearchContainerDropIntent Intent;
	Intent.ExpectedRunId = Snapshot.OwningRunId;
	Intent.ContainerId = Snapshot.ContainerId;
	Intent.ExpectedContainerRevision = Snapshot.Revision;
	Intent.ExpectedAuthorityRevision = Manager->GetItemSubsystem()
		->GetAuthority().GetAuthorityRevision();
	Intent.ExpectedSourceItemInstanceId = SourceItemId;
	Intent.bSourceIsContainer = !bSourcePlayer;
	if (Intent.bSourceIsContainer)
	{
		const Fdemo_mapRuntimeContainerEntrySnapshot* SourceEntry =
			FindSnapshotEntry(ContainerSection, ContainerSlotIndex);
		if (!SourceEntry)
		{
			return;
		}
		Intent.SourceEntryId = SourceEntry->EntryId;
		Intent.TargetPlayerArea = PlayerAreaFor(TargetContext);
		Intent.TargetPlayerSlotIndex = PlayerVisualSlot(
			TargetSlotIndex, TargetContext);
		Intent.TargetPlayerEquipmentSlotId = TargetContext
			== Edemo_mapItemPresentationContext::RuntimeEquipment
			? EquipmentForEncodedSlot(TargetSlotIndex) : NAME_None;
	}
	else
	{
		const Fdemo_mapRuntimeContainerEntrySnapshot* TargetEntry =
			FindSnapshotEntry(ContainerSection, ContainerSlotIndex);
		Intent.TargetEntryId = TargetEntry ? TargetEntry->EntryId : FGuid();
		Intent.TargetSection = ContainerSection;
		Intent.TargetContainerSlotIndex = ContainerSlotIndex;
		Intent.SourcePlayerArea = PlayerAreaFor(SourceContext);
		Intent.SourcePlayerSlotIndex = PlayerVisualSlot(
			SourceSlotIndex, SourceContext);
		Intent.SourcePlayerEquipmentSlotId = SourceContext
			== Edemo_mapItemPresentationContext::RuntimeEquipment
			? EquipmentForEncodedSlot(SourceSlotIndex) : NAME_None;
	}
	const Fdemo_mapSearchContainerDropResult DropResult =
		Manager->SubmitSearchContainerDrop(Intent);
	if (DiagnosticText)
	{
		DiagnosticText->SetText(FText::FromString(DropResult.Diagnostic));
	}
}

void Udemo_mapSearchContainerWidget::HandleConfirmSpatialBundleDrop()
{
	if (!Manager.IsValid()
		|| !PendingSpatialBundleDrop.ExpectedSourceItemInstanceId.IsValid())
	{
		return;
	}
	const Fdemo_mapItemOperationResult Operation =
		Manager->RequestDropPlayerItem(PendingSpatialBundleDrop, true);
	PendingSpatialBundleDrop = Fdemo_mapPlayerItemDropIntent();
	if (DiagnosticText)
	{
		DiagnosticText->SetText(FText::FromString(Operation.Diagnostic));
	}
	RefreshVisuals();
}

void Udemo_mapSearchContainerWidget::HandleCancelSpatialBundleDrop()
{
	PendingSpatialBundleDrop = Fdemo_mapPlayerItemDropIntent();
	if (DiagnosticText)
	{
		DiagnosticText->SetText(FText::FromString(
			TEXT("空间道具 Bundle 丢弃已取消；未改变物品权威。")));
	}
	RefreshVisuals();
}

void Udemo_mapSearchContainerWidget::HandleTakeSelected()
{
	if (!Manager.IsValid())
	{
		return;
	}
	const Fdemo_mapSearchContainerViewRow* Row =
		FindRowByEntryId(SelectedEntryId);
	if (!Row
		|| Row->State != Edemo_mapRuntimeContainerEntryState::Identified
		|| !Row->bActionEnabled)
	{
		return;
	}
	Fdemo_mapRuntimeContainerIntent Intent;
	Intent.ExpectedRunId = Snapshot.OwningRunId;
	Intent.ContainerId = Snapshot.ContainerId;
	Intent.ExpectedRevision = Snapshot.Revision;
	Intent.EntryId = Row->EntryId;
	Intent.Action = Edemo_mapRuntimeContainerActionKind::Take;
	LastResult = Manager->SubmitSearchContainerIntent(Intent);
}

void Udemo_mapSearchContainerWidget::HandleCancelSearch()
{
	if (!Manager.IsValid()
		|| Snapshot.ActiveAction
			!= Edemo_mapRuntimeContainerActionKind::BeginSearch)
	{
		return;
	}
	Fdemo_mapRuntimeContainerIntent Intent;
	Intent.ExpectedRunId = Snapshot.OwningRunId;
	Intent.ContainerId = Snapshot.ContainerId;
	Intent.ExpectedRevision = Snapshot.Revision;
	Intent.Action = Edemo_mapRuntimeContainerActionKind::CancelSearch;
	LastResult = Manager->SubmitSearchContainerIntent(Intent);
}

void Udemo_mapSearchContainerWidget::HandleClose()
{
	ClearTransientDragState();
	if (Manager.IsValid())
	{
#if !UE_BUILD_SHIPPING
		Manager->RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::CloseIntentReceived,
			-1.0f,
			-1.0,
			Edemo_mapInputRestoreTraceCaller::Widget);
#endif
		Manager->CloseSearchContainer(TEXT("UserClose"), true);
	}
}

void Udemo_mapSearchContainerWidget::ClickChestSection(){SelectSection(Edemo_mapRuntimeContainerSection::Chest);}
void Udemo_mapSearchContainerWidget::ClickEquipmentSection(){SelectSection(Edemo_mapRuntimeContainerSection::Equipment);}
void Udemo_mapSearchContainerWidget::ClickBackpackSection(){SelectSection(Edemo_mapRuntimeContainerSection::Backpack);}
void Udemo_mapSearchContainerWidget::ClickBodySection(){SelectSection(Edemo_mapRuntimeContainerSection::Body);}
void Udemo_mapSearchContainerWidget::ClickRow00(){ActivateRow(0);}
void Udemo_mapSearchContainerWidget::ClickRow01(){ActivateRow(1);}
void Udemo_mapSearchContainerWidget::ClickRow02(){ActivateRow(2);}
void Udemo_mapSearchContainerWidget::ClickRow03(){ActivateRow(3);}
void Udemo_mapSearchContainerWidget::ClickRow04(){ActivateRow(4);}
void Udemo_mapSearchContainerWidget::ClickRow05(){ActivateRow(5);}
void Udemo_mapSearchContainerWidget::ClickPreviousPage()
{
	VisibleRowOffset = FMath::Max(0, VisibleRowOffset - 6);
	RefreshVisuals();
}
void Udemo_mapSearchContainerWidget::ClickNextPage()
{
	const Fdemo_mapSearchContainerSectionView* SectionView =
		FindSelectedSectionView();
	if (SectionView
		&& VisibleRowOffset + 6 < GetSelectedVisibleRowCount())
	{
		VisibleRowOffset += 6;
		RefreshVisuals();
	}
}
void Udemo_mapSearchContainerWidget::ClickTake(){HandleTakeSelected();}
void Udemo_mapSearchContainerWidget::ClickReturn()
{
	if (DiagnosticText)
	{
		DiagnosticText->SetText(FText::FromString(
			TEXT("放回使用真实鼠标拖拽：从左侧玩家物品拖至右侧目标格。")));
	}
}
void Udemo_mapSearchContainerWidget::ClickCancelSearch(){HandleCancelSearch();}
void Udemo_mapSearchContainerWidget::ClickClose(){HandleClose();}
void Udemo_mapSearchContainerWidget::ClickConfirmSpatialBundleDrop()
{
	HandleConfirmSpatialBundleDrop();
}
void Udemo_mapSearchContainerWidget::ClickCancelSpatialBundleDrop()
{
	HandleCancelSpatialBundleDrop();
}

#if !UE_BUILD_SHIPPING
bool Udemo_mapSearchContainerWidget::AutomationSelectSection(
	Edemo_mapRuntimeContainerSection Section)
{
	if (!ViewState.Sections.ContainsByPredicate(
		[Section](const Fdemo_mapSearchContainerSectionView& Candidate)
		{
			return Candidate.Section == Section;
		}))
	{
		return false;
	}
	SelectSection(Section);
	return SelectedSection == Section;
}

bool Udemo_mapSearchContainerWidget::AutomationClickEntry(
	Edemo_mapRuntimeContainerSection Section,
	int32 SlotIndex)
{
	if (!AutomationSelectSection(Section))
	{
		return false;
	}
	const Fdemo_mapSearchContainerSectionView* SectionView =
		FindSelectedSectionView();
	if (!SectionView)
	{
		return false;
	}
	const int32 RowIndex = SectionView->Rows.IndexOfByPredicate(
		[SlotIndex](const Fdemo_mapSearchContainerViewRow& Row)
		{
			return Row.SlotIndex == SlotIndex;
		});
	if (!RowButtons.IsValidIndex(RowIndex)
		|| !SectionView->Rows[RowIndex].bActionEnabled)
	{
		if (RowIndex == INDEX_NONE
			|| !SectionView->Rows.IsValidIndex(RowIndex)
			|| !SectionView->Rows[RowIndex].bActionEnabled)
		{
			return false;
		}
	}
	VisibleRowOffset = (RowIndex / 6) * 6;
	RefreshVisuals();
	const int32 VisualRowIndex = RowIndex - VisibleRowOffset;
	if (!RowButtons.IsValidIndex(VisualRowIndex))
	{
		return false;
	}
	const Edemo_mapRuntimeContainerEntryState StateBefore =
		SectionView->Rows[RowIndex].State;
	RowButtons[VisualRowIndex]->OnClicked.Broadcast();
	if (StateBefore == Edemo_mapRuntimeContainerEntryState::Identified)
	{
		HandleTakeSelected();
	}
	return LastResult.bSuccess;
}

bool Udemo_mapSearchContainerWidget::AutomationClickTake()
{
	const Fdemo_mapSearchContainerViewRow* Row =
		FindRowByEntryId(SelectedEntryId);
	if (!Row
		|| Row->State != Edemo_mapRuntimeContainerEntryState::Identified)
	{
		return false;
	}
	HandleTakeSelected();
	return LastResult.bSuccess;
}

bool Udemo_mapSearchContainerWidget::AutomationClickCancelSearch()
{
	if (Snapshot.ActiveAction
		!= Edemo_mapRuntimeContainerActionKind::BeginSearch)
	{
		return false;
	}
	HandleCancelSearch();
	return LastResult.bSuccess;
}

bool Udemo_mapSearchContainerWidget::AutomationClickClose()
{
	if (!CloseButton)
	{
		return false;
	}
	CloseButton->OnClicked.Broadcast();
	return true;
}
#endif
