#include "demo_map0909BSectWarehouseWidget.h"

#include "demo_map0909BFramework.h"
#include "demo_map0909BSectWarehouseService.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

namespace
{
	void AddPadded(UVerticalBox* Parent, UWidget* Child, const FMargin Padding)
	{
		if (Parent && Child)
		{
			if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child))
			{
				Slot->SetPadding(Padding);
			}
		}
	}
}

void Udemo_map0909BWarehouseSlotWidget::InitializeSlot(
	Ademo_map0909BFrameworkHost* InHost,
	const FGuid& InItemId,
	const FGuid& InContainerId,
	const int32 InSlotIndex,
	const int32 InExpectedGraphRevision,
	const FString& InLabel)
{
	FrameworkHost = InHost;
	ItemId = InItemId;
	ContainerId = InContainerId;
	SlotIndex = InSlotIndex;
	ExpectedGraphRevision = InExpectedGraphRevision;
	BuildInterface();
	if (LabelText)
	{
		LabelText->SetText(FText::FromString(InLabel));
	}
}

FReply Udemo_map0909BWarehouseSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (ItemId.IsValid())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void Udemo_map0909BWarehouseSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (!ItemId.IsValid()) return;
	Udemo_map0909BWarehouseDragOperation* Operation = NewObject<Udemo_map0909BWarehouseDragOperation>(this);
	Operation->ItemId = ItemId;
	Operation->SourceContainerId = ContainerId;
	Operation->SourceSlot = SlotIndex;
	Operation->ExpectedGraphRevision = ExpectedGraphRevision;
	OutOperation = Operation;
}

bool Udemo_map0909BWarehouseSlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (const Udemo_map0909BWarehouseDragOperation* Operation = Cast<Udemo_map0909BWarehouseDragOperation>(InOperation))
	{
		if (FrameworkHost.IsValid())
		{
			FrameworkHost->RequestWarehouseDragDrop(
				Operation->ItemId, Operation->SourceContainerId, Operation->SourceSlot,
				ContainerId, SlotIndex, Operation->ExpectedGraphRevision);
			return true;
		}
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void Udemo_map0909BWarehouseSlotWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree) return;
	bBuilt = true;
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(FLinearColor(0.11f, 0.22f, 0.30f, 1.0f));
	Border->SetPadding(FMargin(8.0f));
	LabelText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = LabelText->GetFont();
	Font.Size = 14;
	LabelText->SetFont(Font);
	LabelText->SetAutoWrapText(true);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Border->SetContent(LabelText);
	WidgetTree->RootWidget = Border;
}

void Udemo_map0909BSectWarehouseWidget::InitializeForFramework(Ademo_map0909BFrameworkHost* InHost)
{
	FrameworkHost = InHost;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
}

void Udemo_map0909BSectWarehouseWidget::RefreshPresentation(
	const Fdemo_map0909BWarehousePresentation& InPresentation)
{
	Presentation = InPresentation;
	BuildInterface();
	if (!bBuilt) return;
	GateText->SetText(FText::FromString(Presentation.GateDiagnostic));
	SelectionText->SetText(FText::FromString(FString::Printf(
		TEXT("P5 Owner: %s · Persistent Revision: %d · Graph Revision: %d · Loadout Digest: %s"),
		*Presentation.OwnerId.ToString(EGuidFormats::Digits), Presentation.PersistentRevision,
		Presentation.LoadoutSelection.GraphRevision, *Presentation.LoadoutSelection.Digest)));
	RebuildProjection();
}

void Udemo_map0909BSectWarehouseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_map0909BSectWarehouseWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree) return;
	bBuilt = true;
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = RootCanvas;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.014f, 0.026f, 0.042f, 0.985f));
	RootCanvas->AddChild(Background);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Background->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Background->SetContent(Scroll);
	Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Column);
	AddPadded(Column, AddText(TEXT("宗门仓库 / Sect Warehouse"), 32, FLinearColor(0.97f, 0.80f, 0.25f)), FMargin(32.0f, 28.0f, 32.0f, 6.0f));
	AddPadded(Column, AddText(TEXT("仓库、战备与空间内容是同一 P5 Code B 图的不同位置。拖拽仅提交意图；快照过期或状态不符会被拒绝。"), 17, FLinearColor(0.78f, 0.84f, 0.92f)), FMargin(32.0f, 0.0f, 32.0f, 8.0f));
	GateText = AddText(FString(), 17, FLinearColor(0.92f, 0.94f, 0.97f));
	SelectionText = AddText(FString(), 14, FLinearColor(0.58f, 0.72f, 0.86f));
	AddPadded(Column, GateText, FMargin(32.0f, 4.0f, 32.0f, 4.0f));
	AddPadded(Column, SelectionText, FMargin(32.0f, 4.0f, 32.0f, 16.0f));
	ProjectionColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPadded(Column, ProjectionColumn, FMargin(32.0f, 0.0f, 32.0f, 16.0f));
	ReturnButton = WidgetTree->ConstructWidget<UButton>();
	ReturnButton->SetBackgroundColor(FLinearColor(0.10f, 0.25f, 0.34f, 1.0f));
	UTextBlock* ReturnText = AddText(TEXT("返回宗门主页"), 20);
	ReturnButton->SetContent(ReturnText);
	ReturnButton->OnClicked.AddDynamic(this, &Udemo_map0909BSectWarehouseWidget::ClickReturnToSect);
	AddPadded(Column, ReturnButton, FMargin(32.0f, 6.0f, 32.0f, 28.0f));
}

void Udemo_map0909BSectWarehouseWidget::RebuildProjection()
{
	if (!ProjectionColumn || !WidgetTree) return;
	ProjectionColumn->ClearChildren();
	for (const demo_map_code_b::FCodeBP2ContainerView& Container : Presentation.Projection.Containers)
	{
		const FString Heading = FString::Printf(TEXT("%s · %d 格"), *Container.Role.ToString(), Container.Capacity);
		AddPadded(ProjectionColumn, AddText(Heading, 20, FLinearColor(0.52f, 0.89f, 1.0f)), FMargin(0.0f, 8.0f, 0.0f, 4.0f));
		UWrapBox* Slots = WidgetTree->ConstructWidget<UWrapBox>();
		for (int32 SlotIndex = 0; SlotIndex < Container.Slots.Num(); ++SlotIndex)
		{
			const demo_map_code_b::FCodeBP2SlotView& SlotView = Container.Slots[SlotIndex];
			const FString ItemLabel = SlotView.ItemId.IsValid()
				? FString::Printf(TEXT("%d · %s%s"), SlotIndex + 1, *SlotView.DefinitionId.ToString(),
					SlotView.ChildContainerId.IsValid() ? TEXT(" [空间闭包]") : TEXT(""))
				: FString::Printf(TEXT("%d · 空"), SlotIndex + 1);
			Udemo_map0909BWarehouseSlotWidget* Cell = WidgetTree->ConstructWidget<Udemo_map0909BWarehouseSlotWidget>();
			Cell->InitializeSlot(FrameworkHost.Get(), SlotView.ItemId, Container.ContainerId, SlotIndex,
				Presentation.Projection.Revision, ItemLabel);
			if (UWrapBoxSlot* SlotLayout = Slots->AddChildToWrapBox(Cell))
			{
				SlotLayout->SetPadding(FMargin(3.0f));
			}
		}
		AddPadded(ProjectionColumn, Slots, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}
}

UTextBlock* Udemo_map0909BSectWarehouseWidget::AddText(
	const FString& Value,
	const int32 Size,
	const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
	Text->SetText(FText::FromString(Value));
	Text->SetAutoWrapText(true);
	Text->SetColorAndOpacity(FSlateColor(Color));
	return Text;
}

void Udemo_map0909BSectWarehouseWidget::ClickReturnToSect()
{
	if (FrameworkHost.IsValid())
	{
		FrameworkHost->RequestCloseWarehouseFromUI();
	}
}
