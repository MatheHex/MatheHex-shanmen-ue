#include "demo_mapHUD.h"
#include "demo_map.h"
#include "demo_mapGameState.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapFriendlyUnit.h"
#include "demo_mapSkillComponent.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapPlayerController.h"
#include "demo_mapShanmenControlledWeaponActor.h"
#include "demo_mapShanmenThrownWeaponArcEditingInputHintPresentation.h"
#include "demo_mapShanmenThrownWeaponArcEditingPresentation.h"
#include "demo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation.h"
#include "demo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPolicy.h"
#include "demo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation.h"
#include "demo_mapShanmenThrownWeaponTrajectoryPresentation.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CanvasItem.h"

namespace
{
	void DrawReadableText(UCanvas* Canvas, UFont* Font, const FString& Text, const FVector2D& Position, const FLinearColor& Color, float Scale = 1.0f)
	{
		FCanvasTextItem Item(Position, FText::FromString(Text), Font, Color);
		Item.Scale = FVector2D(Scale, Scale);
		Item.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Item);
	}

	void DrawHUDPanel(
		UCanvas* Canvas,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& Color =
			FLinearColor(0.018f, 0.04f, 0.065f, 0.82f))
	{
		FCanvasTileItem Tile(Position, Size, Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}

	void DrawControlledWeaponThreatReadout(
		UCanvas* Canvas,
		APlayerController* PlayerController,
		const Ademo_mapShanmenControlledWeaponActor* Weapon)
	{
		if (Canvas == nullptr || PlayerController == nullptr
			|| Weapon == nullptr || !Weapon->IsThreatPresenceCueActive()
			|| Weapon->GetThreatPresenceCueContactCount() <= 0)
		{
			return;
		}

		FVector2D ScreenPosition;
		if (!PlayerController->ProjectWorldLocationToScreen(
				Weapon->GetActorLocation() + FVector(0.0f, 0.0f, 72.0f),
				ScreenPosition,
				false))
		{
			return;
		}

		const FVector2D PanelSize(230.0f, 30.0f);
		const FVector2D PanelPosition(
			FMath::Clamp(
				ScreenPosition.X - PanelSize.X * 0.5f,
				8.0,
				FMath::Max(8.0, Canvas->SizeX - PanelSize.X - 8.0)),
			FMath::Clamp(
				ScreenPosition.Y - 46.0f,
				54.0,
				FMath::Max(54.0, Canvas->SizeY - PanelSize.Y - 92.0)));
		DrawHUDPanel(
			Canvas,
			PanelPosition,
			PanelSize,
			FLinearColor(0.02f, 0.18f, 0.14f, 0.88f));
		DrawReadableText(
			Canvas,
			GEngine->GetSmallFont(),
			FString::Printf(
				TEXT("飞剑警戒 · 近身目标 %d"),
				Weapon->GetThreatPresenceCueContactCount()),
			PanelPosition + FVector2D(10.0f, 6.0f),
			FLinearColor(0.2f, 1.0f, 0.72f),
			0.82f);
	}

	void DrawArcPreviewMarker(
		UCanvas* Canvas,
		APlayerController* PlayerController,
		const FVector& WorldPosition,
		const FLinearColor& Color,
		const float Radius)
	{
		FVector2D ScreenPosition;
		if (Canvas == nullptr || PlayerController == nullptr
			|| !PlayerController->ProjectWorldLocationToScreen(
				WorldPosition, ScreenPosition, false))
		{
			return;
		}
		FCanvasLineItem Horizontal(
			ScreenPosition - FVector2D(Radius, 0.0f),
			ScreenPosition + FVector2D(Radius, 0.0f));
		Horizontal.SetColor(Color);
		Horizontal.LineThickness = 2.0f;
		Canvas->DrawItem(Horizontal);
		FCanvasLineItem Vertical(
			ScreenPosition - FVector2D(0.0f, Radius),
			ScreenPosition + FVector2D(0.0f, Radius));
		Vertical.SetColor(Color);
		Vertical.LineThickness = 2.0f;
		Canvas->DrawItem(Vertical);
	}

	void DrawThrownWeaponArcPreview(
		UCanvas* Canvas,
		APlayerController* PlayerController,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State)
	{
		if (Canvas == nullptr || PlayerController == nullptr
			|| !State.IsVisible())
		{
			return;
		}
		const FLinearColor ArcColor(0.18f, 0.92f, 1.0f, 0.96f);
		for (const auto& Segment : State.GetSegments())
		{
			FVector2D Start;
			FVector2D End;
			if (!PlayerController->ProjectWorldLocationToScreen(
					Segment.GetStart(), Start, false)
				|| !PlayerController->ProjectWorldLocationToScreen(
					Segment.GetEnd(), End, false))
			{
				continue;
			}
			FCanvasLineItem Line(Start, End);
			Line.SetColor(ArcColor);
			Line.LineThickness = 2.5f;
			Canvas->DrawItem(Line);
		}
		DrawArcPreviewMarker(
			Canvas,
			PlayerController,
			State.GetApexPosition(),
			FLinearColor(1.0f, 0.78f, 0.18f, 1.0f),
			5.0f);
		DrawArcPreviewMarker(
			Canvas,
			PlayerController,
			State.GetPlannedLandingPosition(),
			FLinearColor(0.25f, 1.0f, 0.38f, 1.0f),
			7.0f);
	}

	FLinearColor GetThrownWeaponCombatHintColor(
		const Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone Tone)
	{
		using ETone =
			Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone;
		switch (Tone)
		{
		case ETone::StraightMode:
			return FLinearColor(0.25f, 0.9f, 1.0f);
		case ETone::ArcMode:
		case ETone::TargetRequired:
			return FLinearColor(1.0f, 0.72f, 0.18f);
		case ETone::ArcApex:
			return FLinearColor(1.0f, 0.82f, 0.38f);
		case ETone::ArcTarget:
			return FLinearColor(0.76f, 0.88f, 1.0f);
		case ETone::ArcInput:
			return FLinearColor(0.58f, 0.92f, 1.0f);
		case ETone::ReadyToConfirm:
			return FLinearColor(0.35f, 1.0f, 0.48f);
		default:
			return FLinearColor::White;
		}
	}

	float GetThrownWeaponCombatHintScale(
		const Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone Tone)
	{
		using ETone =
			Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone;
		switch (Tone)
		{
		case ETone::StraightMode:
		case ETone::ArcMode:
			return 0.92f;
		case ETone::ArcApex:
		case ETone::ArcTarget:
			return 0.78f;
		case ETone::ArcInput:
			return 0.72f;
		case ETone::TargetRequired:
		case ETone::ReadyToConfirm:
			return 0.76f;
		default:
			return 1.0f;
		}
	}
}

void Ademo_mapHUD::BeginPlay()
{
	Super::BeginPlay();
	FString Diagnostic;
	if (!ThrownWeaponArcPreviewRendererAdapter.TryInitialize(
			FGuid::NewGuid(), Diagnostic))
	{
		UE_LOG(
			Logdemo_map,
			Warning,
			TEXT("Arc preview MainHUD renderer initialization rejected: %s"),
			*Diagnostic);
	}
	else if (Ademo_mapGameMode* ActiveMode = GetWorld() != nullptr
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
		ActiveMode
		&& !ActiveMode->TryAttachThrownWeaponArcPreviewHUD(
			ThrownWeaponArcPreviewRendererAdapter, Diagnostic))
	{
		UE_LOG(
			Logdemo_map,
			Warning,
			TEXT("Arc preview MainHUD runtime attach rejected: %s"),
			*Diagnostic);
	}
}

void Ademo_mapHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ThrownWeaponArcPreviewRendererAdapter.IsInitialized())
	{
		if (Ademo_mapGameMode* ActiveMode = GetWorld() != nullptr
			? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
			: nullptr)
		{
			FString Diagnostic;
			if (!ActiveMode->TryDetachThrownWeaponArcPreviewHUD(
					ThrownWeaponArcPreviewRendererAdapter, Diagnostic))
			{
				UE_LOG(
					Logdemo_map,
					Error,
					TEXT("Arc preview MainHUD runtime detach rejected: %s"),
					*Diagnostic);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapHUD::DrawHUD()
{
	Super::DrawHUD();
	if (Canvas == nullptr || GEngine == nullptr)
	{
		return;
	}

	const Ademo_mapGameMode* ActiveMode = GetWorld() != nullptr ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	const Ademo_mapV3ProgressionManager* V3 = ActiveMode ? ActiveMode->GetV3ProgressionManager() : nullptr;
	const Fdemo_mapInputBindingSettings& InputSettings = Fdemo_mapInputBindingSettings::Get();
	APlayerController* PlayerController =
		GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	DrawThrownWeaponArcPreview(
		Canvas,
		PlayerController,
		ThrownWeaponArcPreviewRendererAdapter.GetSurfaceCursor());
	DrawControlledWeaponThreatReadout(
		Canvas,
		PlayerController,
		ActiveMode
			? ActiveMode->GetControlledWeaponWorldLifecycle().GetWeaponActor()
			: nullptr);
	DrawHUDPanel(
		Canvas,
		FVector2D(18.0f, 14.0f),
		FVector2D(Canvas->SizeX - 36.0f, 36.0f));
	DrawHUDPanel(
		Canvas,
		FVector2D(18.0f, 56.0f),
		FVector2D(430.0f, 250.0f));
	DrawHUDPanel(
		Canvas,
		FVector2D(Canvas->SizeX - 350.0f, 14.0f),
		FVector2D(332.0f, 235.0f));
	DrawReadableText(
		Canvas,
		GEngine->GetSmallFont(),
		FString::Printf(
			TEXT("%s/%s/%s/%s Move | %s Attack | %s/%s/%s Skills | %s Interact/Search | %s Inventory | %s Back | %s Restart"),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::MoveForward).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::MoveLeft).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::MoveBackward).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::MoveRight).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::PrimaryAttack).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::SkillGroundCircle).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::SkillSelfSector).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::SkillStraightProjectile).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::Interact).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::Inventory).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::Back).GetDisplayName().ToString(),
			*InputSettings.GetKey(Fdemo_mapInputActionIds::ResetRun).GetDisplayName().ToString()),
		FVector2D(28.0f, 24.0f),
		FLinearColor::White,
		1.05f);
	if (V3 && !V3->IsInventoryOpen())
	{
		const FVector2D ProgressPosition(
			Canvas->SizeX * 0.30f,
			Canvas->SizeY - 156.0f);
		DrawHUDPanel(
			Canvas,
			ProgressPosition - FVector2D(12.0f, 8.0f),
			FVector2D(Canvas->SizeX * 0.40f, 96.0f));
		DrawReadableText(
			Canvas,
			GEngine->GetLargeFont(),
			V3->GetInteractionPrompt().ToString(),
			ProgressPosition,
			FLinearColor(1.0f, 0.92f, 0.22f),
			0.92f);
		const bool bSearching = V3->IsSearchContainerOpen();
		DrawReadableText(
			Canvas,
			GEngine->GetSmallFont(),
			bSearching
				? TEXT("SEARCH / USE PROGRESS  ■■■■■■■■■■")
				: TEXT("SEARCH / USE PROGRESS  READY"),
			ProgressPosition + FVector2D(0.0f, 28.0f),
			bSearching
				? FLinearColor(0.25f, 0.9f, 1.0f)
				: FLinearColor(0.65f, 0.74f, 0.82f),
			0.78f);
		if (ActiveMode)
		{
			const FString M01RiskText = ActiveMode->GetM01RiskHUDText();
			if (!M01RiskText.IsEmpty())
			{
				DrawReadableText(
					Canvas,
					GEngine->GetSmallFont(),
					M01RiskText,
					ProgressPosition + FVector2D(0.0f, 50.0f),
					FLinearColor(0.35f, 0.9f, 1.0f),
					0.86f);
			}
			const FString M01ExtractionText = ActiveMode->GetM01ExtractionHUDText();
			if (!M01ExtractionText.IsEmpty())
			{
				DrawReadableText(
					Canvas,
					GEngine->GetSmallFont(),
					M01ExtractionText,
					ProgressPosition + FVector2D(0.0f, 70.0f),
					FLinearColor(0.98f, 0.80f, 0.18f),
					0.82f);
			}
		}
	}
	if (Ademo_mapPlayerController* DemoController =
		Cast<Ademo_mapPlayerController>(PlayerController))
	{
		const auto Read =
			DemoController->ReadThrownWeaponInputChoiceInteraction();
		Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation
			Feedback;
		if (ActiveMode)
		{
			const auto& PreLaunchContext =
				ActiveMode->GetThrownWeaponArcPreLaunchPreviewContext();
			if (PreLaunchContext.HasArmedHotbarSlot())
			{
				const int32 ArmedSlot =
					PreLaunchContext.GetArmedHotbarSlotNumber();
				const FString HotbarKeyLabel = InputSettings.GetKey(
					Fdemo_mapInputActionRegistry::HotbarActionId(ArmedSlot))
					.GetDisplayName().ToString();
				Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation::
					TryProject(
						PreLaunchContext,
						Read,
						ThrownWeaponArcPreviewRendererAdapter.
							GetSurfaceCursor(),
						HotbarKeyLabel,
						Feedback);
			}
		}
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation Presentation;
		const FString ToggleKeyLabel = InputSettings.GetKey(
			Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle)
			.GetDisplayName().ToString();
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation::TryProject(
			Read, ToggleKeyLabel, Presentation);
		Fdemo_mapShanmenThrownWeaponArcEditingPresentation ArcPresentation;
		Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation
			ArcInputHintPresentation;
		if (Fdemo_mapShanmenThrownWeaponArcEditingPresentation::TryProject(
			Read, ArcPresentation))
		{
			Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation::
				TryProject(
					ArcPresentation,
					InputSettings.GetKey(
						Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet)
						.GetDisplayName().ToString(),
					InputSettings.GetKey(
						Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease)
						.GetDisplayName().ToString(),
					InputSettings.GetKey(
						Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease)
						.GetDisplayName().ToString(),
					InputSettings.GetKey(
						Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear)
						.GetDisplayName().ToString(),
					ArcInputHintPresentation);
		}
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation Stack;
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan Layout;
		if (Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation::
			TryCompose(
				Presentation,
				ArcPresentation,
				ArcInputHintPresentation,
				Feedback,
				Stack)
			&& Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan::TryPlan(
				FVector2D(Canvas->SizeX, Canvas->SizeY),
				Stack.NumLines(),
				Layout))
		{
			const auto& Lines = Stack.GetLines();
			for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
			{
				const auto& Line = Lines[LineIndex];
				FVector2D LinePosition;
				if (!Layout.TryGetLinePosition(LineIndex, LinePosition))
				{
					continue;
				}
				DrawReadableText(
					Canvas,
					GEngine->GetSmallFont(),
					Line.GetDisplayText(),
					LinePosition,
					GetThrownWeaponCombatHintColor(Line.GetTone()),
					GetThrownWeaponCombatHintScale(Line.GetTone())
						* Layout.GetScaleMultiplier());
			}
		}
	}
	APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	const Udemo_mapPlayerHealthComponent* Health = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (Health != nullptr)
	{
		DrawReadableText(Canvas, GEngine->GetLargeFont(), FString::Printf(TEXT("Health: %d / %d"), Health->GetCurrentHealth(), Health->GetMaxHealth()), FVector2D(28.0f, 62.0f), Health->IsDefeated() ? FLinearColor::Red : FLinearColor(0.15f, 1.0f, 0.18f), 1.25f);
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const Udemo_mapProfileSessionSubsystem* ProfileSession = GameInstance ? GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>() : nullptr;
	if (ProfileSession)
	{
		const Fdemo_mapProfileSessionSnapshot Profile = ProfileSession->GetSnapshot();
		DrawReadableText(
			Canvas,
			GEngine->GetLargeFont(),
			FString::Printf(TEXT("RISK SPIRIT STONES: %lld  (撤离后转为持久灵石)"), static_cast<long long>(Profile.RiskSpiritStones)),
			FVector2D(28.0f, 88.0f),
			FLinearColor(0.2f, 0.9f, 1.0f),
			1.0f);
		const int64 RecentGain = ProfileSession->GetRecentSpiritStoneGain();
		if (RecentGain > 0)
		{
			DrawReadableText(
				Canvas,
				GEngine->GetLargeFont(),
				FString::Printf(TEXT("+%lld 灵石"), static_cast<long long>(RecentGain)),
				FVector2D(28.0f, 116.0f),
				FLinearColor(1.0f, 0.82f, 0.2f),
				1.05f);
		}
	}
	if (const Udemo_mapItemSubsystem* Items = GameInstance ? GameInstance->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr)
	{
		const Fdemo_mapItemAuthority& Authority =
			Items->GetAuthority();
		const FGuid WeaponId = Authority.GetEquippedInstance(
			Fdemo_mapItemIds::WeaponSlot);
		const Fdemo_mapItemInstance* Weapon =
			Authority.FindInstance(WeaponId);
		const Fdemo_mapItemDefinition* WeaponDefinition = Weapon
			? Fdemo_mapItemDefinitions::Find(Weapon->DefinitionId)
			: nullptr;
		const FString InventoryKey =
			InputSettings.GetKey(Fdemo_mapInputActionIds::Inventory)
				.GetDisplayName().ToString();
		DrawReadableText(
			Canvas,
			GEngine->GetSmallFont(),
			FString::Printf(
				TEXT("CURRENT WEAPON: %s  |  [%s] 人物物品  %d/%d"),
				WeaponDefinition
					? *WeaponDefinition->DisplayName.ToString()
					: TEXT("UNARMED"),
				*InventoryKey,
				Authority.GetUsedInventorySlots(),
				Authority.GetInventoryCapacity()),
			FVector2D(28.0f, 146.0f),
			FLinearColor(0.45f, 0.86f, 1.0f),
			0.92f);
		const TArray<Fdemo_mapHotbarSlotView> Hotbar =
			Fdemo_mapItemPresentation::BuildRuntimeHotbar(
				Items->GetHotbarBindingSnapshot(),
				Items->GetAuthority(),
				Items->GetItemUseCooldownSnapshot());
		const float OuterMargin = 18.0f;
		const float Gap = 4.0f;
		const float SlotWidth =
			(Canvas->SizeX
				- OuterMargin * 2.0f
				- Gap * 8.0f)
			/ 9.0f;
		const float SlotY = Canvas->SizeY - 82.0f;
		for (const Fdemo_mapHotbarSlotView& Slot : Hotbar)
		{
			const float SlotX =
				OuterMargin
				+ (Slot.SlotNumber - 1)
					* (SlotWidth + Gap);
			const FLinearColor StateColor =
				Slot.State == Edemo_mapHotbarSlotState::Ready
					? FLinearColor(0.18f, 0.95f, 0.38f)
					: (Slot.State
							== Edemo_mapHotbarSlotState::Cooldown
						? FLinearColor(1.0f, 0.75f, 0.15f)
						: (Slot.State
								== Edemo_mapHotbarSlotState::
									InvalidBinding
							? FLinearColor(1.0f, 0.22f, 0.18f)
							: FLinearColor(
								0.72f,
								0.78f,
								0.84f)));
			FCanvasTileItem Background(
				FVector2D(SlotX, SlotY),
				FVector2D(SlotWidth, 60.0f),
				FLinearColor(0.025f, 0.055f, 0.08f, 0.92f));
			Background.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Background);
			const FString KeyLabel = InputSettings.GetKey(
				Fdemo_mapInputActionRegistry::HotbarActionId(
					Slot.SlotNumber)).GetDisplayName().ToString();
			DrawReadableText(
				Canvas,
				GEngine->GetSmallFont(),
				FString::Printf(
					TEXT("[%s] %s %s"),
					*KeyLabel,
					*Slot.IconLabel,
					*Slot.DisplayName),
				FVector2D(SlotX + 6.0f, SlotY + 7.0f),
				FLinearColor::White,
				0.72f);
			DrawReadableText(
				Canvas,
				GEngine->GetSmallFont(),
				Slot.IsOccupied()
					? FString::Printf(
						TEXT("×%d · %s"),
						Slot.Quantity,
						*Slot.StateLabel)
					: TEXT("— · 空槽"),
				FVector2D(SlotX + 6.0f, SlotY + 33.0f),
				StateColor,
				0.72f);
		}
	}
	if (const Udemo_mapAttributeComponent* Attributes = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<Udemo_mapAttributeComponent>() : nullptr)
	{
		const Fdemo_mapAttributeSnapshot& Snapshot = Attributes->GetFinalSnapshot();
		const float PanelX = Canvas->SizeX - 330.0f;
		float PanelY = 24.0f;
		DrawReadableText(Canvas, GEngine->GetSmallFont(), TEXT("PROTOTYPE ATTRIBUTES"), FVector2D(PanelX, PanelY), FLinearColor(0.65f, 0.9f, 1.0f), 1.05f);
		PanelY += 24.0f;
		for (FName Id : { Fdemo_mapAttributeIds::Primary01, Fdemo_mapAttributeIds::Primary02, Fdemo_mapAttributeIds::Primary03 })
		{
			float Value = 0.0f;
			const Fdemo_mapAttributeDefinition* Definition = Fdemo_mapAttributeDefinitions::Find(Id);
			if (Definition != nullptr && Snapshot.TryGetValue(Id, Value))
			{
				DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("%s: %.0f"), *Definition->DisplayName.ToString(), Value), FVector2D(PanelX, PanelY), FLinearColor::White, 0.95f);
				PanelY += 20.0f;
			}
		}
		if (Health != nullptr)
		{
			DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("Current Health: %d / %d"), Health->GetCurrentHealth(), Health->GetMaxHealth()), FVector2D(PanelX, PanelY), FLinearColor(0.3f, 1.0f, 0.35f), 0.95f);
			PanelY += 20.0f;
		}
		for (FName Id : { Fdemo_mapAttributeIds::MoveSpeed, Fdemo_mapAttributeIds::AttackPower, Fdemo_mapAttributeIds::DodgeChance })
		{
			float Value = 0.0f;
			const Fdemo_mapAttributeDefinition* Definition = Fdemo_mapAttributeDefinitions::Find(Id);
			if (Definition != nullptr && Snapshot.TryGetValue(Id, Value))
			{
				const FString ValueText = Id == Fdemo_mapAttributeIds::DodgeChance ? FString::Printf(TEXT("%.1f%%"), Value * 100.0f) : FString::Printf(TEXT("%.2f"), Value);
				DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("%s: %s"), *Definition->DisplayName.ToString(), *ValueText), FVector2D(PanelX, PanelY), FLinearColor::White, 0.95f);
				PanelY += 20.0f;
			}
		}
	}
	const Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (MissionState == nullptr)
	{
		return;
	}

	const FString MissionText = MissionState->GetMissionPhase() == Edemo_mapMissionPhase::ReachExit
		? FString::Printf(TEXT("Destroy Targets: %d / %d"), MissionState->GetDestroyedTargets(), MissionState->GetRequiredTargets())
		: MissionState->GetMissionText();
	DrawReadableText(Canvas, GEngine->GetLargeFont(), MissionText, FVector2D(28.0f, 148.0f), FLinearColor(1.0f, 0.85f, 0.1f), 0.9f);
	if (MissionState->GetMissionPhase() == Edemo_mapMissionPhase::ReachExit || MissionState->GetMissionPhase() == Edemo_mapMissionPhase::Complete)
	{
		DrawReadableText(Canvas, GEngine->GetLargeFont(), TEXT("EXIT OPEN"), FVector2D(Canvas->SizeX * 0.34f, 145.0f), FLinearColor::Green, 1.20f);
		DrawReadableText(Canvas, GEngine->GetLargeFont(), TEXT("Extraction Available"), FVector2D(Canvas->SizeX * 0.34f, 172.0f), FLinearColor::Green, 1.15f);
	}
	if (const Ademo_mapGameMode* Mode = GetWorld() != nullptr ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (const Ademo_mapFriendlyUnit* Ally = Mode->GetFriendlyUnit())
		{
			DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("ALLY: %d / %d"), Ally->GetCurrentHealth(), Ally->GetMaxHealth()), FVector2D(28.0f, 180.0f), FLinearColor::Green, 1.1f);
		}
	}
	if (const Udemo_mapSkillComponent* Skills = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<Udemo_mapSkillComponent>() : nullptr)
	{
		auto SkillStatus = [](bool bReady, float Remaining){ return bReady ? FString(TEXT("READY")) : FString::Printf(TEXT("%.1fs"), Remaining); };
		DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("Q  Ground Circle     %s"), *SkillStatus(Skills->IsCircleReady(), Skills->GetCircleCooldownRemaining())), FVector2D(28.0f, 210.0f), FLinearColor(0.0f, 1.0f, 1.0f), 1.0f);
		DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("E  Self Sector       %s"), *SkillStatus(Skills->IsConeReady(), Skills->GetConeCooldownRemaining())), FVector2D(28.0f, 236.0f), FLinearColor(1.0f, 0.65f, 0.1f), 1.0f);
		DrawReadableText(Canvas, GEngine->GetSmallFont(), FString::Printf(TEXT("F  Projectile        %s"), *SkillStatus(Skills->IsProjectileReady(), Skills->GetProjectileCooldownRemaining())), FVector2D(28.0f, 262.0f), FLinearColor(0.15f, 0.75f, 1.0f), 1.0f);
		if (Skills->IsGroundCircleTargeting())
		{
			DrawReadableText(Canvas, GEngine->GetLargeFont(), TEXT("TARGETING GROUND CIRCLE"), FVector2D(Canvas->SizeX * 0.35f, 62.0f), FLinearColor(0.0f, 1.0f, 1.0f), 1.0f);
			DrawReadableText(Canvas, GEngine->GetSmallFont(), TEXT("LMB Confirm    RMB / ESC Cancel"), FVector2D(Canvas->SizeX * 0.38f, 98.0f), FLinearColor::White, 1.1f);
			if (!Skills->HasValidGroundPoint())
			{
				DrawReadableText(Canvas, GEngine->GetLargeFont(), TEXT("INVALID TARGET"), FVector2D(Canvas->SizeX * 0.42f, 128.0f), FLinearColor::Red, 1.0f);
			}
			else if (!Skills->IsGroundPointInRange())
			{
				DrawReadableText(Canvas, GEngine->GetLargeFont(), TEXT("OUT OF RANGE"), FVector2D(Canvas->SizeX * 0.43f, 128.0f), FLinearColor::Red, 1.0f);
			}
		}
	}
	DrawReadableText(Canvas, GEngine->GetSmallFont(), TEXT("MELEE / RANGED / HEAVY"), FVector2D(Canvas->SizeX - 310.0f, Canvas->SizeY - 68.0f), FLinearColor(1.0f, 0.55f, 0.25f), 1.0f);
	DrawReadableText(Canvas, GEngine->GetSmallFont(), V3 ? TEXT("DEMO BUILD: 0.5.0.0") : TEXT("DEMO BUILD: 0.3.2.0"), FVector2D(Canvas->SizeX - 320.0f, Canvas->SizeY - 42.0f), FLinearColor(0.3f, 0.8f, 1.0f), 1.15f);
	if (const Ademo_mapGameMode* GameMode = GetWorld() != nullptr ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr; GameMode != nullptr && GameMode->IsResetPending())
	{
		DrawReadableText(Canvas, GEngine->GetLargeFont(), GameMode->GetEndStatusText(), FVector2D(Canvas->SizeX * 0.36f, Canvas->SizeY * 0.40f), FLinearColor::White, 1.6f);
	}
}
