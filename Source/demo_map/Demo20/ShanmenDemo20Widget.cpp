#include "ShanmenDemo20Widget.h"
#include "ShanmenDemo20World.h"
#include "ShanmenDemo20InventoryWidget.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	const FLinearColor Ink(.022f,.043f,.040f,.97f), Jade(.21f,.63f,.49f), Gold(.76f,.63f,.36f), Paper(.88f,.88f,.80f);
	UTextBlock* Text(UWidgetTree* Tree, const FString& Value, int32 Size, FLinearColor Color)
	{
		auto* Result = Tree->ConstructWidget<UTextBlock>();
		auto Font = Result->GetFont(); Font.Size = Size;
		Result->SetFont(Font);
		Result->SetColorAndOpacity(Color);
		Result->SetAutoWrapText(true);
		Result->SetText(FText::FromString(Value));
		return Result;
	}
	void Add(UVerticalBox* Box, UWidget* Widget, float Bottom = 12.f)
	{
		auto* Slot = Box->AddChildToVerticalBox(Widget);
		Slot->SetPadding(FMargin(0,0,0,Bottom));
	}
	void Set(UTextBlock* Target, const FString& Value)
	{
		if (Target && Target->GetText().ToString() != Value) Target->SetText(FText::FromString(Value));
	}
	FString KeyLabel(FName Action) { return Fdemo_mapInputBindingSettings::Get().GetKey(Action).GetDisplayName().ToString(); }
}

void UShanmenDemo20Widget::InitializeForDemo(AShanmenDemo20GameMode* InHost)
{
	Host = InHost;
	Build();
	if (InventoryView) InventoryView->InitializeForDemo(InHost);
	Refresh();
}
void UShanmenDemo20Widget::NativeOnInitialized() { Super::NativeOnInitialized(); Build(); }

void UShanmenDemo20Widget::Build()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	auto* Status = WidgetTree->ConstructWidget<UBorder>();
	Status->SetBrushColor(Ink);
	Status->SetPadding(FMargin(22,16));
	Status->SetVisibility(ESlateVisibility::HitTestInvisible);
	auto* StatusSlot = Canvas->AddChildToCanvas(Status);
	StatusSlot->SetOffsets(FMargin(24,24,330,245));
	auto* StatusBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Status->SetContent(StatusBox);
	Add(StatusBox, Text(WidgetTree, TEXT("山 门  /  Demo 2.0"), 23, Gold));
	HealthLabel = Text(WidgetTree, TEXT("生命"), 16, Paper);
	Add(StatusBox, HealthLabel, 6);
	Health = WidgetTree->ConstructWidget<UProgressBar>();
	Health->SetFillColorAndOpacity(Jade);
	auto* BarSize = WidgetTree->ConstructWidget<USizeBox>();
	BarSize->SetHeightOverride(8); BarSize->SetContent(Health); Add(StatusBox, BarSize);
	Objective = Text(WidgetTree, TEXT("守阵石卫  0 / 3"), 17, Paper);
	Add(StatusBox, Objective);
	Defense = Text(WidgetTree, FString(), 14, Gold); Add(StatusBox, Defense, 0);
	Medicine = Text(WidgetTree, FString(), 14, Paper); Add(StatusBox, Medicine, 4);

	auto* Footer = WidgetTree->ConstructWidget<UBorder>();
	Footer->SetBrushColor(Ink);
	Footer->SetPadding(FMargin(22,12));
	Footer->SetVisibility(ESlateVisibility::HitTestInvisible);
	auto* FooterSlot = Canvas->AddChildToCanvas(Footer);
	FooterSlot->SetAnchors(FAnchors(0,1,1,1));
	FooterSlot->SetOffsets(FMargin(24,-134,24,110));
	auto* FooterBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Footer->SetContent(FooterBox);
	Notice = Text(WidgetTree, FString(), 17, Gold); Add(FooterBox, Notice, 9);
	const FString Controls = FString::Printf(TEXT("%s/%s/%s/%s 移动    %s 出剑    %s 闪身    按住 %s 格挡    %s 搜索/归阵    Esc 返回/暂停"),
		*KeyLabel(Fdemo_mapInputActionIds::MoveForward), *KeyLabel(Fdemo_mapInputActionIds::MoveLeft),
		*KeyLabel(Fdemo_mapInputActionIds::MoveBackward), *KeyLabel(Fdemo_mapInputActionIds::MoveRight),
		*KeyLabel(Fdemo_mapInputActionIds::PrimaryAttack), *KeyLabel(Fdemo_mapInputActionIds::SpiritEvasion),
		*KeyLabel(Fdemo_mapInputActionIds::WeaponGuard), *KeyLabel(Fdemo_mapInputActionIds::Interact));
	Add(FooterBox, Text(WidgetTree, Controls, 13, Paper), 0);

	Modal = WidgetTree->ConstructWidget<UBorder>();
	Modal->SetBrushColor(FLinearColor(.005f,.015f,.012f,.78f));
	auto* ModalSlot = Canvas->AddChildToCanvas(Modal);
	ModalSlot->SetAnchors(FAnchors(0,0,1,1)); ModalSlot->SetOffsets(FMargin(0));
	Modal->SetHorizontalAlignment(HAlign_Center); Modal->SetVerticalAlignment(VAlign_Center);
	auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
	Scale->SetStretch(EStretch::ScaleToFit); Scale->SetStretchDirection(EStretchDirection::DownOnly);
	Modal->SetContent(Scale);
	auto* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(550); Scale->SetContent(Size);
	auto* Card = WidgetTree->ConstructWidget<UBorder>();
	Card->SetBrushColor(Ink); Card->SetPadding(FMargin(36,28)); Size->SetContent(Card);
	auto* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(); Card->SetContent(CardBox);
	Add(CardBox, Text(WidgetTree, TEXT("SHANMEN    /    DEMO 2.0"), 13, Gold), 20);
	Heading = Text(WidgetTree, TEXT("归尘试炼"), 34, Paper); Add(CardBox, Heading, 16);
	Body = Text(WidgetTree, FString(), 17, Paper); Add(CardBox, Body, 24);
	auto Button = [&](TObjectPtr<UButton>& OutButton, TObjectPtr<UTextBlock>& OutLabel, const FLinearColor& Color)
	{
		OutButton = WidgetTree->ConstructWidget<UButton>();
		OutButton->SetBackgroundColor(Color);
		OutLabel = Text(WidgetTree, FString(), 19, Paper);
		OutLabel->SetAutoWrapText(false);
		OutLabel->SetJustification(ETextJustify::Center);
		OutButton->SetContent(OutLabel);
		auto* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
		ButtonSize->SetMinDesiredHeight(52); ButtonSize->SetContent(OutButton); Add(CardBox, ButtonSize, 10);
	};
	Button(PrimaryButton, PrimaryLabel, FLinearColor(.10f,.38f,.30f));
	Button(SecondaryButton, SecondaryLabel, FLinearColor(.12f,.16f,.15f));
	PrimaryButton->OnClicked.AddDynamic(this, &UShanmenDemo20Widget::Primary);
	SecondaryButton->OnClicked.AddDynamic(this, &UShanmenDemo20Widget::Secondary);
	InventoryButton = WidgetTree->ConstructWidget<UButton>();
	InventoryButton->SetBackgroundColor(FLinearColor(.12f,.28f,.22f));
	auto* InventoryLabel = Text(WidgetTree, TEXT("仓库与整备"), 20, Paper);
	InventoryLabel->SetAutoWrapText(false);
	InventoryLabel->SetJustification(ETextJustify::Center);
	InventoryButton->SetContent(InventoryLabel);
	InventoryButton->OnClicked.AddDynamic(this, &UShanmenDemo20Widget::Inventory);
	Add(CardBox, InventoryButton, 14);
	Add(CardBox, Text(WidgetTree, TEXT("本地 Demo · 隔离档 · 不读取旧档\n正式探索按携带规则结算；练习不改物品。"), 12, FLinearColor(.50f,.61f,.56f)), 0);
	InventorySurface = WidgetTree->ConstructWidget<UBorder>();
	InventorySurface->SetBrushColor(FLinearColor(.005f,.015f,.012f,.9f));
	InventorySurface->SetHorizontalAlignment(HAlign_Center); InventorySurface->SetVerticalAlignment(VAlign_Center);
	auto* SurfaceSlot = Canvas->AddChildToCanvas(InventorySurface);
	SurfaceSlot->SetAnchors(FAnchors(0,0,1,1));
	SurfaceSlot->SetOffsets(FMargin(12));
	auto* InventoryScale = WidgetTree->ConstructWidget<UScaleBox>(); InventoryScale->SetStretch(EStretch::ScaleToFit);
	InventoryScale->SetStretchDirection(EStretchDirection::DownOnly); InventorySurface->SetContent(InventoryScale);
	auto* InventorySize = WidgetTree->ConstructWidget<USizeBox>(); InventorySize->SetWidthOverride(960); InventorySize->SetHeightOverride(640);
	InventoryScale->SetContent(InventorySize);
	InventoryView = CreateWidget<UShanmenDemo20InventoryWidget>(GetOwningPlayer(), UShanmenDemo20InventoryWidget::StaticClass());
	InventorySize->SetContent(InventoryView);
}

void UShanmenDemo20Widget::Refresh()
{
	if (!Host.IsValid() || !Modal) return;
	const auto& Session = Host->GetSession();
	const auto Phase = Session.GetPhase();
	InventorySurface->SetVisibility(Host->IsInventoryOpen() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	InventoryButton->SetVisibility(Phase == EShanmenDemo20Phase::Preparation ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	InventoryButton->SetIsEnabled(Host->IsProfileReady());
	const bool Preparing = Phase == EShanmenDemo20Phase::Preparation;
	Set(HealthLabel, Preparing ? (Host->IsExpedition() ? TEXT("整备入口 · 携带状态见下方") : TEXT("整备阶段 · 尚未出发"))
		: FString::Printf(TEXT("生命  %.0f / 100"), Session.GetHealth()));
	Health->SetPercent(Session.GetHealth() / 100.f);
	Set(Objective, Preparing ? TEXT("仓库 / 背包 / 装备") : Host->IsExpedition() ? Host->GetExplorationArea()
		: FString::Printf(TEXT("守阵石卫  %d / 3"), Session.NumDefeated()));
	Set(Defense, Preparing ? (Host->IsExpedition()?TEXT("携带确认后出发 · 开局可撤离"):TEXT("石庭练习 · 不结算"))
		: Session.IsGuarding() ? TEXT("格挡中 · 无法出剑") : Session.IsEvading() ? TEXT("闪身中") : Session.GetEvadeCooldown() > 0.f ? FString::Printf(TEXT("闪身恢复  %.1f 秒"), Session.GetEvadeCooldown()) : TEXT("闪身就绪"));
	Set(Notice, Host->GetNotice());
	Set(Medicine, Phase==EShanmenDemo20Phase::Active && Host->IsExpedition() ? FString::Printf(TEXT("[%s] 回春丹 · 普通 %d / 安全格 %d\n非满生命使用 · 仓库不可局内使用"),
		*KeyLabel(Fdemo_mapInputActionIds::Hotbar1),Host->GetCarryMedicine(),Host->GetSecureMedicine()) : FString());
	if (Host->IsMedicinePending()) Set(Medicine,TEXT("丹药使用正在恢复确认\n操作已暂停 · 请重试或重启恢复"));
	Modal->SetVisibility(Host->IsPlaying() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	PrimaryButton->SetIsEnabled(Host->IsWorldReady() && (!Host->IsExpedition() || Host->IsProfileReady()));
	SecondaryButton->SetVisibility(Host->IsPaused() && Phase==EShanmenDemo20Phase::Active ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Host->IsSourceSurfaceOpen() && !Host->IsPaused() && Phase==EShanmenDemo20Phase::Active)
	{
		Set(Heading,Host->GetSourceHeading()); Set(Body,Host->GetSourceBody());
		Set(PrimaryLabel,Host->IsSearchingSource()?TEXT("取消搜索"):TEXT("关闭预览"));
		SecondaryButton->SetVisibility(ESlateVisibility::Collapsed); return;
	}
	if (Phase == EShanmenDemo20Phase::Preparation)
	{
		if (Host->IsExpedition())
		{
			Set(Heading,TEXT("青岚关探索"));
			Set(Body,TEXT("整备武器、护具与行囊后出发。\n石径 → 竹林 → 遗坛，相连区域可自由探索。\n归阵从开局可用，交互后等待 3 秒撤离。\n普通携带死亡损失；安全格与仓库保留。\n\n")+Host->GetNotice());
			Set(PrimaryLabel,TEXT("出发 / 继续原局")); return;
		}
		Set(Heading, TEXT("归尘试炼"));
		Set(Body, Host->IsWorldReady() ? TEXT("进入石庭，以剑破阵。\n\n靠近三座石卫，鼠标指向目标出剑。\n赤光蓄势时，闪身避开或持剑格挡。\n击破全部石卫后，回到青色归阵撤出。") : Host->GetNotice());
		Set(PrimaryLabel, TEXT("石庭练习（不结算）"));
	}
	else if (Host->IsPaused() && Phase==EShanmenDemo20Phase::Active)
	{
		Set(Heading, TEXT("片刻静心"));
		Set(Body, Host->IsExpedition()?TEXT("世界已暂停，移动与敌人计时停止。\n继续或重试保存后行动；失焦前的输入不会遗留。\n\n")+Host->GetNotice():
			TEXT("试炼已暂停，计时与石卫攻击已停止。\n\n继续后按新的键位输入行动；不会保留失焦前的移动或格挡。"));
		Set(PrimaryLabel, Host->IsExpedition()?TEXT("继续 / 重试保存"):TEXT("继续试炼"));
		Set(SecondaryLabel, Host->IsExpedition()?TEXT("保存并返回入口（原局保留）"):TEXT("结束本次试炼"));
	}
	else
	{
		if (Host->IsExpedition())
		{
			Set(Heading,Host->IsTerminalConfirmed()?(Phase==EShanmenDemo20Phase::Extracted?TEXT("归阵撤离"):TEXT("力竭止步")):TEXT("结算待确认"));
			Set(Body,FString::Printf(TEXT("击败守卫  %d / 3\n本局用时  %.1f 秒\n\n"),Session.NumDefeated(),Session.GetElapsed())+Host->GetNotice());
			Set(PrimaryLabel,Host->IsTerminalConfirmed()?TEXT("返回仓库与整备"):TEXT("重试确认结算")); return;
		}
		Set(Heading, Phase == EShanmenDemo20Phase::Extracted ? TEXT("破阵归来") : Phase == EShanmenDemo20Phase::Defeated ? TEXT("力竭止步") : TEXT("试炼已结束"));
		Set(Body, FString::Printf(TEXT("击破石卫  %d / 3\n本次用时  %.1f 秒\n\n这是独立试炼结果，不代表持久奖励入库。\n返回后可开启全新一局。"), Session.NumDefeated(), Session.GetElapsed()));
		Set(PrimaryLabel, TEXT("返回试炼入口"));
	}
}

void UShanmenDemo20Widget::Primary()
{
	if (!Host.IsValid()) return;
	if (Host->IsSourceSurfaceOpen() && !Host->IsPaused()) Host->CloseSourceSurface();
	else if (Host->IsPaused() && Host->GetSession().GetPhase()==EShanmenDemo20Phase::Active) Host->TogglePause();
	else if (Host->GetSession().GetPhase() == EShanmenDemo20Phase::Preparation) Host->StartTrial();
	else Host->ReturnToPreparation();
}
void UShanmenDemo20Widget::Secondary() { if (Host.IsValid() && Host->IsPaused()) Host->LeaveTrial(); }
void UShanmenDemo20Widget::Inventory() { if (Host.IsValid()) Host->ToggleInventory(); }
void UShanmenDemo20Widget::RefreshInventory() { if (InventoryView) InventoryView->RefreshProjection(); }

void UShanmenDemo20Widget::FocusActiveSurface()
{
	if (!Host.IsValid() || Host->IsPlaying()) return;
	if (Host->IsInventoryOpen() && InventoryView) InventoryView->SetKeyboardFocus();
	else SetKeyboardFocus();
}

FReply UShanmenDemo20Widget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Host.IsValid() && Host->IsSourceSurfaceOpen() && Event.GetKey()==EKeys::Escape)
	{ Host->TogglePause(); return FReply::Handled().ReleaseMouseCapture(); }
	if (Host.IsValid() && (Event.GetKey() == Fdemo_mapInputBindingSettings::Get().GetKey(Fdemo_mapInputActionIds::Inventory)
		|| (Event.GetKey() == EKeys::Escape && Host->IsInventoryOpen())))
	{
		Host->ToggleInventory();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
