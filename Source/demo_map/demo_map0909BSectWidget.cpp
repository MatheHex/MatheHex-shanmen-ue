#include "demo_map0909BSectWidget.h"

#include "demo_map0909BFramework.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	UTextBlock* SectText(
		UWidgetTree* Tree,
		const FString& Value,
		const int32 Size,
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

	void AddSectPadded(UVerticalBox* Parent, UWidget* Child, const FMargin Padding)
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

void Udemo_map0909BSectWidget::InitializeForFramework(
	Ademo_map0909BFrameworkHost* InHost)
{
	FrameworkHost = InHost;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
	RefreshText();
}

void Udemo_map0909BSectWidget::RefreshPresentation(
	const Edemo_map0909BTopState InState,
	const FString& InFeedback,
	const Fdemo_map0909BStartDiagnostic& InDiagnostic)
{
	State = InState;
	Feedback = InFeedback;
	Diagnostic = InDiagnostic;
	RefreshText();
}

void Udemo_map0909BSectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

void Udemo_map0909BSectWidget::BuildInterface()
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
	Background->SetBrushColor(FLinearColor(0.014f, 0.026f, 0.042f, 0.985f));
	RootCanvas->AddChild(Background);
	if (UCanvasPanelSlot* BackgroundCanvasSlot = Cast<UCanvasPanelSlot>(Background->Slot))
	{
		BackgroundCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundCanvasSlot->SetOffsets(FMargin(0.0f));
	}
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Background->SetContent(Scroll);
	Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Column);

	AddSectPadded(Column, SectText(WidgetTree, TEXT("归山宗门 · 0.0.9B"), 34,
		FLinearColor(0.97f, 0.80f, 0.25f)), FMargin(42.0f, 34.0f, 42.0f, 8.0f));
	AddSectPadded(Column, SectText(WidgetTree,
		TEXT("仓库／战备保持同一 P5 图；只有经 M01 世界、玩家与输入确认的出战才进入真实局内状态。"),
		18, FLinearColor(0.78f, 0.84f, 0.92f)), FMargin(42.0f, 0.0f, 42.0f, 18.0f));
	StateText = SectText(WidgetTree, TEXT("状态：AtSect"), 22,
		FLinearColor(0.50f, 0.90f, 1.0f));
	FeedbackText = SectText(WidgetTree, FString(), 18,
		FLinearColor(0.92f, 0.94f, 0.97f));
	DiagnosticText = SectText(WidgetTree, FString(), 14,
		FLinearColor(0.60f, 0.70f, 0.80f));
	AddSectPadded(Column, StateText, FMargin(42.0f, 8.0f, 42.0f, 4.0f));
	AddSectPadded(Column, FeedbackText, FMargin(42.0f, 4.0f, 42.0f, 4.0f));
	AddSectPadded(Column, DiagnosticText, FMargin(42.0f, 4.0f, 42.0f, 22.0f));
	StartM01Button = AddActionButton(TEXT("部署 M01 荒山灵矿遗迹"));
	WarehouseButton = AddActionButton(TEXT("打开仓库 / 人物装备"));
}

UButton* Udemo_map0909BSectWidget::AddActionButton(const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(FLinearColor(0.10f, 0.25f, 0.34f, 1.0f));
	UTextBlock* Text = SectText(WidgetTree, Label, 22, FLinearColor::White);
	Button->SetContent(Text);
	AddSectPadded(Column, Button, FMargin(42.0f, 8.0f, 42.0f, 8.0f));
	if (!StartM01Button)
	{
		Button->OnClicked.AddDynamic(this, &Udemo_map0909BSectWidget::ClickStartM01);
	}
	else
	{
		Button->OnClicked.AddDynamic(this, &Udemo_map0909BSectWidget::ClickOpenWarehouse);
	}
	return Button;
}

void Udemo_map0909BSectWidget::RefreshText()
{
	if (!bBuilt)
	{
		return;
	}
	StateText->SetText(FText::FromString(FString::Printf(
		TEXT("状态：%s"), demo_map0909BTopStateName(State))));
	FeedbackText->SetText(FText::FromString(Feedback));
	DiagnosticText->SetText(FText::FromString(
		Diagnostic.StartAttemptId.IsValid()
			? FString::Printf(TEXT("尝试：%s · 回执：%s · 地图：%s · 原因：%s"),
				*Diagnostic.StartAttemptId.ToString(EGuidFormats::Digits),
				*Diagnostic.RuntimeReceiptClass,
				*Diagnostic.M01MapIdentity,
				*Diagnostic.FailureClass)
			: TEXT("启动诊断：等待真实部署请求。")));
	const bool bAtSect = State == Edemo_map0909BTopState::AtSect;
	StartM01Button->SetIsEnabled(bAtSect);
	WarehouseButton->SetIsEnabled(bAtSect);
}

void Udemo_map0909BSectWidget::ClickStartM01()
{
	if (FrameworkHost.IsValid())
	{
		FrameworkHost->RequestStartM01FromUI();
	}
}

void Udemo_map0909BSectWidget::ClickOpenWarehouse()
{
	if (FrameworkHost.IsValid())
	{
		FrameworkHost->RequestOpenWarehouseFromUI();
	}
}
