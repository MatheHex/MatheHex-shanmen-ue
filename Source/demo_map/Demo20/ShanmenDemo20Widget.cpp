#include "ShanmenDemo20Widget.h"
#include "ShanmenDemo20World.h"
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
	Refresh();
}
void UShanmenDemo20Widget::NativeOnInitialized() { Super::NativeOnInitialized(); Build(); }

void UShanmenDemo20Widget::Build()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	auto* Status = WidgetTree->ConstructWidget<UBorder>();
	Status->SetBrushColor(Ink);
	Status->SetPadding(FMargin(22,16));
	Status->SetVisibility(ESlateVisibility::HitTestInvisible);
	auto* StatusSlot = Canvas->AddChildToCanvas(Status);
	StatusSlot->SetOffsets(FMargin(24,24,330,205));
	auto* StatusBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Status->SetContent(StatusBox);
	Add(StatusBox, Text(WidgetTree, TEXT("山 门  /  归尘试炼"), 23, Gold));
	HealthLabel = Text(WidgetTree, TEXT("生命"), 16, Paper);
	Add(StatusBox, HealthLabel, 6);
	Health = WidgetTree->ConstructWidget<UProgressBar>();
	Health->SetFillColorAndOpacity(Jade);
	auto* BarSize = WidgetTree->ConstructWidget<USizeBox>();
	BarSize->SetHeightOverride(8); BarSize->SetContent(Health); Add(StatusBox, BarSize);
	Objective = Text(WidgetTree, TEXT("守阵石卫  0 / 3"), 17, Paper);
	Add(StatusBox, Objective);
	Defense = Text(WidgetTree, FString(), 14, Gold); Add(StatusBox, Defense, 0);

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
	const FString Controls = FString::Printf(TEXT("%s/%s/%s/%s 移动    %s 出剑    %s 闪身    按住 %s 格挡    %s 归阵    Esc 暂停"),
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
	Add(CardBox, Text(WidgetTree, TEXT("本地试炼 · 不打包 · 不读取旧档\n首个战斗切片，暂不包含背包、治疗与持久奖励。"), 12, FLinearColor(.50f,.61f,.56f)), 0);
}

void UShanmenDemo20Widget::Refresh()
{
	if (!Host.IsValid() || !Modal) return;
	const auto& Session = Host->GetSession();
	const auto Phase = Session.GetPhase();
	Set(HealthLabel, FString::Printf(TEXT("生命  %.0f / 100"), Session.GetHealth()));
	Health->SetPercent(Session.GetHealth() / 100.f);
	Set(Objective, FString::Printf(TEXT("守阵石卫  %d / 3"), Session.NumDefeated()));
	Set(Defense, Session.IsGuarding() ? TEXT("格挡中 · 无法出剑") : Session.IsEvading() ? TEXT("闪身中") : Session.GetEvadeCooldown() > 0.f ? FString::Printf(TEXT("闪身恢复  %.1f 秒"), Session.GetEvadeCooldown()) : TEXT("闪身就绪"));
	Set(Notice, Host->GetNotice());
	Modal->SetVisibility(Host->IsPlaying() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	PrimaryButton->SetIsEnabled(Host->IsWorldReady());
	SecondaryButton->SetVisibility(Host->IsPaused() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Phase == EShanmenDemo20Phase::Preparation)
	{
		Set(Heading, TEXT("归尘试炼"));
		Set(Body, Host->IsWorldReady() ? TEXT("进入石庭，以剑破阵。\n\n靠近三座石卫，鼠标指向目标出剑。\n赤光蓄势时，闪身避开或持剑格挡。\n击破全部石卫后，回到青色归阵撤出。") : Host->GetNotice());
		Set(PrimaryLabel, TEXT("进入试炼"));
	}
	else if (Host->IsPaused())
	{
		Set(Heading, TEXT("片刻静心"));
		Set(Body, TEXT("试炼已暂停，计时与石卫攻击已停止。\n\n继续后按新的键位输入行动；不会保留失焦前的移动或格挡。"));
		Set(PrimaryLabel, TEXT("继续试炼"));
		Set(SecondaryLabel, TEXT("结束本次试炼"));
	}
	else
	{
		Set(Heading, Phase == EShanmenDemo20Phase::Extracted ? TEXT("破阵归来") : Phase == EShanmenDemo20Phase::Defeated ? TEXT("力竭止步") : TEXT("试炼已结束"));
		Set(Body, FString::Printf(TEXT("击破石卫  %d / 3\n本次用时  %.1f 秒\n\n这是独立试炼结果，不代表持久奖励入库。\n返回后可开启全新一局。"), Session.NumDefeated(), Session.GetElapsed()));
		Set(PrimaryLabel, TEXT("返回试炼入口"));
	}
}

void UShanmenDemo20Widget::Primary()
{
	if (!Host.IsValid()) return;
	if (Host->IsPaused()) Host->TogglePause();
	else if (Host->GetSession().GetPhase() == EShanmenDemo20Phase::Preparation) Host->StartTrial();
	else Host->ReturnToPreparation();
}
void UShanmenDemo20Widget::Secondary() { if (Host.IsValid() && Host->IsPaused()) Host->LeaveTrial(); }
