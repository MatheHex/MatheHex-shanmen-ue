#include "demo_mapSettlementWidget.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapV3ProgressionManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"

namespace
{
	UTextBlock* SettlementText(UWidgetTree* Tree, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont(); Font.Size = Size; Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color)); Text->SetAutoWrapText(true); return Text;
	}
}

void Udemo_mapSettlementWidget::InitializeForManager(
	Ademo_mapV3ProgressionManager* InManager)
{
	Manager = InManager;
}

void Udemo_mapSettlementWidget::SetSummary(
	const Fdemo_mapSettlementSummary& InSummary,
	FGuid InSettlementId)
{
	Summary = InSummary;
	SettlementId = InSettlementId;
	RefreshText();
}

void Udemo_mapSettlementWidget::NativeOnInitialized() { Super::NativeOnInitialized(); BuildInterface(); }
void Udemo_mapSettlementWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildInterface();
	RefreshText();
}

void Udemo_mapSettlementWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree) return;
	bBuilt = true;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(); Panel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.97f)); Canvas->AddChild(Panel);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Panel->Slot)) { CanvasSlot->SetAnchors(FAnchors(0.18f, 0.12f, 0.82f, 0.88f)); CanvasSlot->SetOffsets(FMargin(0)); }
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(Box);
	TitleText = SettlementText(WidgetTree, 34, FLinearColor(0.95f, 0.78f, 0.20f));
	RowsText = SettlementText(WidgetTree, 19, FLinearColor::White);
	TotalsText = SettlementText(WidgetTree, 23, FLinearColor(0.30f, 1.0f, 0.45f));
	if (UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(22, 14, 22, 8));
	}
	RowsScroll = WidgetTree->ConstructWidget<UScrollBox>();
	RowsScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	RowsScroll->AddChild(RowsText);
	if (UVerticalBoxSlot* RowsSlot = Box->AddChildToVerticalBox(RowsScroll))
	{
		RowsSlot->SetPadding(FMargin(22, 4, 22, 8));
		RowsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UVerticalBoxSlot* TotalsSlot = Box->AddChildToVerticalBox(TotalsText))
	{
		TotalsSlot->SetPadding(FMargin(22, 6));
	}
	ContinueButton = WidgetTree->ConstructWidget<UButton>();
	ContinueButton->SetVisibility(ESlateVisibility::Visible);
	ContinueButton->SetIsEnabled(true);
	UTextBlock* ContinueText = SettlementText(WidgetTree, 22, FLinearColor::White);
	ContinueText->SetText(FText::FromString(TEXT("继续 / RETURN TO PREPARATION")));
	ContinueButton->SetContent(ContinueText);
	ContinueButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapSettlementWidget::HandleContinueClicked);
	USizeBox* ContinueSize = WidgetTree->ConstructWidget<USizeBox>();
	ContinueSize->SetMinDesiredHeight(56.0f);
	ContinueSize->SetContent(ContinueButton);
	if (UVerticalBoxSlot* ButtonSlot = Box->AddChildToVerticalBox(ContinueSize))
	{
		ButtonSlot->SetPadding(FMargin(22, 14, 22, 22));
	}
}

FReply Udemo_mapSettlementWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (DismissForNavigationKey(InKeyEvent.GetKey()))
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool Udemo_mapSettlementWidget::DismissForNavigationKey(const FKey& Key)
{
	if (Key != EKeys::Escape
		&& Key != EKeys::Virtual_Gamepad_Back.GetVirtualKey()
		&& Key != EKeys::Gamepad_FaceButton_Right)
	{
		return false;
	}
	if (Manager.IsValid())
	{
		Manager->ReturnToSectAfterSettlement(
			Key == EKeys::Escape ? TEXT("Escape") : TEXT("Back"));
	}
	return true;
}

void Udemo_mapSettlementWidget::RefreshText()
{
	if (!bBuilt || !TitleText || !RowsText || !TotalsText || !Summary.bValid) return;
	const TCHAR* Title = Summary.Reason == Edemo_mapRunEndReason::Extraction ? TEXT("撤离成功  EXTRACTION SECURED") : Summary.Reason == Edemo_mapRunEndReason::Death ? TEXT("行动失败  DEATH LOSS") : TEXT("放弃行动  RUN ABANDONED");
	TitleText->SetText(FText::FromString(
		SettlementId.IsValid()
			? FString::Printf(
				TEXT("%s\nSettlement %s"),
				Title,
				*SettlementId.ToString(EGuidFormats::DigitsWithHyphens))
			: FString(Title)));
	FString Rows;
	for (const Fdemo_mapSettlementItemRow& Row : Summary.Rows)
	{
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Row.DefinitionId);
		Rows += FString::Printf(TEXT("%s  ×%d   %s   value %d\n"), Definition ? *Definition->DisplayName.ToString() : *Row.DefinitionId.ToString(), Row.Quantity, Row.FinalOwnership == Edemo_mapItemOwnershipState::SessionStash ? TEXT("SECURED") : TEXT("LOST"), Row.TotalValue);
	}
	if (Rows.IsEmpty()) Rows = TEXT("No run items / 本次行动无物品");
	RowsText->SetText(FText::FromString(Rows));
	TotalsText->SetText(FText::FromString(FString::Printf(
		TEXT("Secured %d / value %d    Lost %d / value %d\nSession Stash %d / value %d\nRisk before %lld  transferred %lld  lost %lld\nPersistent %lld → %lld   cleared refs %d\nRun %s"),
		Summary.SecuredItemCount,
		Summary.SecuredValue,
		Summary.LostItemCount,
		Summary.LostValue,
		Summary.StashItemCountAfter,
		Summary.StashValueAfter,
		static_cast<long long>(Summary.RiskBefore),
		static_cast<long long>(Summary.RiskTransferred),
		static_cast<long long>(Summary.RiskLost),
		static_cast<long long>(Summary.PersistentBefore),
		static_cast<long long>(Summary.PersistentAfter),
		Summary.ClearedPreparationItemIds.Num(),
		*Summary.RunId.ToString(EGuidFormats::DigitsWithHyphens))));
}

void Udemo_mapSettlementWidget::HandleContinueClicked()
{
	if (Manager.IsValid())
	{
		Manager->ReturnToSectAfterSettlement(TEXT("ContinueButton"));
	}
}

#if !UE_BUILD_SHIPPING
bool Udemo_mapSettlementWidget::AutomationValidateContinuePath(
	FString& OutDiagnostic) const
{
	if (!ContinueButton || !RowsScroll || !IsInViewport()
		|| ContinueButton->GetVisibility() != ESlateVisibility::Visible
		|| !ContinueButton->GetIsEnabled())
	{
		OutDiagnostic = TEXT("button/scroll visibility, enablement, or viewport precondition failed");
		return false;
	}
	const FGeometry RootGeometry = GetCachedGeometry();
	const FGeometry ButtonGeometry = ContinueButton->GetCachedGeometry();
	const FVector2D RootPosition = RootGeometry.GetAbsolutePosition();
	const FVector2D RootSize = RootGeometry.GetAbsoluteSize();
	const FVector2D ButtonPosition = ButtonGeometry.GetAbsolutePosition();
	const FVector2D ButtonSize = ButtonGeometry.GetAbsoluteSize();
	const bool bMeaningfulGeometry = RootSize.X > 1.0f
		&& RootSize.Y > 1.0f
		&& ButtonSize.X >= 80.0f
		&& ButtonSize.Y >= 32.0f;
	const bool bInsideViewport = ButtonPosition.X >= RootPosition.X - 1.0f
		&& ButtonPosition.Y >= RootPosition.Y - 1.0f
		&& ButtonPosition.X + ButtonSize.X <= RootPosition.X + RootSize.X + 1.0f
		&& ButtonPosition.Y + ButtonSize.Y <= RootPosition.Y + RootSize.Y + 1.0f;
	OutDiagnostic = FString::Printf(
		TEXT("root=(%.1f,%.1f %.1fx%.1f) button=(%.1f,%.1f %.1fx%.1f) visible=%d enabled=%d inside=%d"),
		RootPosition.X, RootPosition.Y, RootSize.X, RootSize.Y,
		ButtonPosition.X, ButtonPosition.Y, ButtonSize.X, ButtonSize.Y,
		ContinueButton->GetVisibility() == ESlateVisibility::Visible,
		ContinueButton->GetIsEnabled(), bInsideViewport);
	return bMeaningfulGeometry && bInsideViewport;
}

bool Udemo_mapSettlementWidget::AutomationClickContinue()
{
	FString Diagnostic;
	if (!AutomationValidateContinuePath(Diagnostic))
	{
		return false;
	}
	ContinueButton->OnClicked.Broadcast();
	return true;
}

bool Udemo_mapSettlementWidget::AutomationPressBack(const FKey& Key)
{
	const FKeyEvent KeyEvent(
		Key,
		FModifierKeysState(),
		0,
		false,
		0,
		0);
	return NativeOnKeyDown(GetCachedGeometry(), KeyEvent).IsEventHandled();
}
#endif
