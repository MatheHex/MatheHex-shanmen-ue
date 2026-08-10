// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeB/demo_mapCodeBP3UI.h"

#include "demo_mapItemDefinitions.h"

#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Layout/WidgetPath.h"
#include "Styling/CoreStyle.h"
#include "UnrealEngine.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SWindow.h"

#include "demo_mapSectNavigationWidget.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapV3ProgressionManager.h"

namespace
{
	using namespace demo_map_code_b;

	constexpr float PageWidth = 1740.0f;
	const FLinearColor PanelColor(0.025f, 0.045f, 0.075f, 0.96f);
	const FLinearColor HeaderColor(0.08f, 0.16f, 0.24f, 1.0f);
	const FLinearColor EmptyColor(0.055f, 0.075f, 0.105f, 1.0f);
	const FLinearColor ItemColor(0.10f, 0.18f, 0.27f, 1.0f);
	const FLinearColor SelectedColor(0.08f, 0.38f, 0.67f, 1.0f);
	const FLinearColor PendingColor(0.70f, 0.37f, 0.06f, 1.0f);
	const FLinearColor DragAllowedColor(0.08f, 0.52f, 0.28f, 1.0f);
	const FLinearColor DragMergeColor(0.20f, 0.38f, 0.75f, 1.0f);
	const FLinearColor DragReplacementColor(0.62f, 0.26f, 0.68f, 1.0f);
	const FLinearColor DragRejectedColor(0.68f, 0.16f, 0.16f, 1.0f);

	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, const int32 FontSize = 16, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Result = Tree->ConstructWidget<UTextBlock>();
		Result->SetText(FText::FromString(Text));
		Result->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FontSize));
		Result->SetColorAndOpacity(FSlateColor(Color));
		Result->SetAutoWrapText(true);
		return Result;
	}

	FString ItemTypeToChinese(const ECodeBItemType ItemType)
	{
		switch (ItemType)
		{
		case ECodeBItemType::Weapon: return TEXT("兵器");
		case ECodeBItemType::Armor: return TEXT("道袍");
		case ECodeBItemType::Accessory: return TEXT("饰品");
		case ECodeBItemType::SpatialItem: return TEXT("空间道具");
		case ECodeBItemType::Backpack: return TEXT("空间储物囊");
		case ECodeBItemType::Material: return TEXT("材料");
		case ECodeBItemType::Consumable: return TEXT("消耗品");
		default: return TEXT("基础物品");
		}
	}

	UCodeBP3UIHostSubsystem* FindP3Host()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = Context.OwningGameInstance;
			if (!GameInstance && Context.World())
			{
				GameInstance = Context.World()->GetGameInstance();
			}
			if (GameInstance)
			{
				return GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>();
			}
		}
		return nullptr;
	}

	void OpenP3Host()
	{
		if (UCodeBP3UIHostSubsystem* Host = FindP3Host())
		{
			Host->OpenPage();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CodeB.P3.Open could not find a game instance host."));
		}
	}

	void CloseP3Host()
	{
		if (UCodeBP3UIHostSubsystem* Host = FindP3Host())
		{
			Host->ClosePage();
		}
	}

	void ResetP3Host()
	{
		if (UCodeBP3UIHostSubsystem* Host = FindP3Host())
		{
			Host->ResetDevelopmentFixture();
		}
	}

	void CaptureP3Frame(const TCHAR* FileName)
	{
		const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("P3Screenshots"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, FileName), true, false);
	}

	void CaptureP4Frame(const TCHAR* FileName)
	{
		const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("P4Screenshots"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, FileName), true, false);
	}

	void CaptureP4xFrame(const FString& FileName)
	{
		const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("P4xScreenshots"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, FileName), true, false);
	}

	void CaptureP5Frame(const FString& Label)
	{
		const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("P5Screenshots"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FString FileName = FString::Printf(
			TEXT("Dev.D.UE.0.0.9B.P5.0.r0_%s_%dx%d_%lld.png"),
			*Label, GSystemResolution.ResX, GSystemResolution.ResY, FDateTime::UtcNow().GetTicks());
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, FileName), true, false);
		UE_LOG(LogTemp, Display, TEXT("P5.RealProfileTrace Screenshot=%s"), *FileName);
	}

	bool PrepareP3Capture(UCodeBP3UIHostSubsystem*& OutHost, FCodeBP3UIController*& OutController)
	{
		OutHost = FindP3Host();
		if (!OutHost || !OutHost->OpenPage())
		{
			return false;
		}
		OutHost->ResetDevelopmentFixture();
		OutController = OutHost->GetController();
		return OutController != nullptr && OutController->IsOpen();
	}

	bool MakeP3Address(FCodeBP3UIController& Controller, const FGuid& ContainerId, const int32 SlotIndex, FCodeBP3SlotAddress& OutAddress)
	{
		return Controller.MakeAddress(ContainerId, SlotIndex, OutAddress);
	}

	void CaptureP3Initial()
	{
		UCodeBP3UIHostSubsystem* Host = nullptr;
		FCodeBP3UIController* Controller = nullptr;
		if (PrepareP3Capture(Host, Controller))
		{
			Host->RefreshActivePage();
			CaptureP3Frame(TEXT("P3_Initial.png"));
		}
	}

	void CaptureP3Selected()
	{
		UCodeBP3UIHostSubsystem* Host = nullptr;
		FCodeBP3UIController* Controller = nullptr;
		if (PrepareP3Capture(Host, Controller) && Controller->GetFixtureIds())
		{
			FCodeBP3SlotAddress Address;
			if (MakeP3Address(*Controller, Controller->GetFixtureIds()->WarehouseContainerId, 0, Address))
			{
				Controller->ActivateAddress(Address);
				Host->RefreshActivePage();
				CaptureP3Frame(TEXT("P3_SelectedDetail.png"));
			}
		}
	}

	void CaptureP4Initial()
	{
		UCodeBP3UIHostSubsystem* Host = nullptr;
		FCodeBP3UIController* Controller = nullptr;
		if (PrepareP3Capture(Host, Controller))
		{
			Host->RefreshActivePage();
			CaptureP4Frame(TEXT("P4_Initial.png"));
		}
	}

	void CaptureP4DragHighlight()
	{
		UCodeBP3UIHostSubsystem* Host = nullptr;
		FCodeBP3UIController* Controller = nullptr;
		if (PrepareP3Capture(Host, Controller) && Controller->GetFixtureIds())
		{
			const FCodeBP2FixtureIds& Ids = *Controller->GetFixtureIds();
			FCodeBP3SlotAddress Source;
			FCodeBP3SlotAddress Target;
			FCodeBP4InteractionController Interaction(*Controller);
			FCodeBP4DragPayload Payload;
			if (MakeP3Address(*Controller, Ids.WarehouseContainerId, 6, Source)
				&& MakeP3Address(*Controller, Ids.BasicContainerId, 0, Target)
				&& Interaction.BeginDrag(Source, Payload))
			{
				Host->SetP4PreviewCapture(Payload, Target);
				CaptureP4Frame(TEXT("P4_DragLegalHighlight.png"));
			}
		}
	}

	/**
	 * Drives the mounted production UMG page through FSlateApplication hit testing.
	 * It deliberately reads cells and projections only for stable-address assertions;
	 * no test step invokes a NativeOn... handler, P3/P4 controller command, or P2 service.
	 */
	class FP4RealInputTraceHarness : public TSharedFromThis<FP4RealInputTraceHarness>
	{
	public:
		FP4RealInputTraceHarness(const int32 InExpectedWidth, const int32 InExpectedHeight, const bool bInExitWhenFinished, const bool bInCaptureEvidence, const FString& InTraceTag)
			: ExpectedWidth(InExpectedWidth)
			, ExpectedHeight(InExpectedHeight)
			, bExitWhenFinished(bInExitWhenFinished)
			, bCaptureEvidence(bInCaptureEvidence)
			, TraceTag(InTraceTag)
		{
		}

		void Start()
		{
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(AsShared(), &FP4RealInputTraceHarness::Tick), 0.05f);
		}

	private:
		enum class EStage : uint8
		{
			WaitForHost,
			SelectWarehouseItem,
			VerifySelection,
			PrepareMove,
			MoveWarehouseToBasic,
			VerifyMove,
			PrepareMerge,
			MoveFirstMergeStack,
			MergeSecondStack,
			VerifyMerge,
			PrepareSwap,
			SwapWarehouseItems,
			VerifySwap,
			PrepareEquipment,
			EquipWeapon,
			EquipWeaponAfterDetail,
			ReplaceWeapon,
			UnequipWeapon,
			UnequipWeaponAfterDetail,
			VerifyEquipment,
			PrepareSpatial,
			EquipSpatialItem,
			MoveToSpatialInternal,
			MoveToSpatialInternalAfterDetail,
			AttemptLoadedSpatialMove,
			VerifySpatial,
			PreparePouch,
			MovePouchToBasic,
			MoveDustToPouch,
			MoveDustToPouchAfterDetail,
			AttemptLoadedPouchMove,
			VerifyPouch,
			PrepareRejects,
			RejectInvalidEquip,
			RejectSameSource,
			VerifyRejects,
			PrepareDoubleClickNoWrite,
			FirstNoWriteClick,
			SecondNoWriteDoubleClick,
			VerifyDoubleClickNoWrite,
			PrepareContextNoWrite,
			OpenContextMenu,
			VerifyContextNoWrite,
			PrepareEscapeCancellation,
			BeginDragForEscape,
			EscapeMountedDrag,
			VerifyEscapeAndReopen,
			EscapeMountedClose,
			VerifyCloseAndReopen,
			Complete
		};

		bool Tick(const float DeltaSeconds)
		{
			if (++TickCount > 800)
			{
				Record(false, TEXT("Timed out waiting for mounted real-input stages."));
				return Finish();
			}
			if (DelayTicks > 0)
			{
				--DelayTicks;
				return true;
			}

			switch (Stage)
			{
			case EStage::WaitForHost:
				Host = FindP3Host();
				if (!Host.IsValid())
				{
					DelayTicks = 2;
					return true;
				}
				Record(Host->OpenPage(), TEXT("Opened the production P3/P4 viewport host."));
				Host->ResetDevelopmentFixture();
				Host->RefreshActivePage();
				if (const FCodeBP2FixtureIds* FixtureIds = Host->GetController() ? Host->GetController()->GetFixtureIds() : nullptr)
				{
					Ids = *FixtureIds;
				}
				else
				{
					Record(false, TEXT("P3/P4 host did not expose its normal fixture after Open."));
				}
				UE_LOG(LogTemp, Display, TEXT("%s Begin Resolution=%dx%d Expected=%dx%d Route=SlateHitTest->UMGCell->P4->P3->P2->P1"),
					*TraceTag, GSystemResolution.ResX, GSystemResolution.ResY, ExpectedWidth, ExpectedHeight);
				Record(ExpectedWidth <= 0 || (GSystemResolution.ResX == ExpectedWidth && GSystemResolution.ResY == ExpectedHeight), TEXT("The requested viewport resolution is active."));
				CaptureEvidence(TEXT("Initial"));
				Stage = EStage::SelectWarehouseItem;
				DelayTicks = 5;
				return true;

			case EStage::SelectWarehouseItem:
				SelectionRevision = GetRevision();
				Record(InjectCellClick(Ids.WarehouseContainerId, 0), TEXT("Slate hit-tested a mounted occupied warehouse cell for P3 selection."));
				Stage = EStage::VerifySelection;
				DelayTicks = 2;
				return true;

			case EStage::VerifySelection:
				Record(SelectedIs(Ids.WarehouseContainerId, 0), TEXT("Actual left click reached P3 selection/detail without changing repository revision."));
				Record(GetRevision() == SelectionRevision, TEXT("Ordinary selection submitted no item command."));
				CaptureEvidence(TEXT("DetailNonEquipable"));
				Stage = EStage::PrepareMove;
				return true;

			case EStage::PrepareMove:
				ResetFixture(EStage::MoveWarehouseToBasic);
				return true;

			case EStage::MoveWarehouseToBasic:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 6, Ids.BasicContainerId, 0), TEXT("Slate threshold created a production DragOperation and dropped it on a real basic-slot cell."));
				Stage = EStage::VerifyMove;
				DelayTicks = 3;
				return true;

			case EStage::VerifyMove:
				Record(AddressHasItem(Ids.BasicContainerId, 0, Ids.DustAItemId), TEXT("Real drop committed the expected item through the authoritative projection."));
				Record(GetRevision() == OperationRevision + 1, TEXT("The real drag committed exactly one P2/P1 transaction."));
				Record(LastGestureHasCounts(1, 1, OperationRevision), TEXT("The accepted physical move logged exactly one P2 command/call and its revision boundary."));
				Stage = EStage::PrepareMerge;
				return true;

			case EStage::PrepareMerge:
				ResetFixture(EStage::MoveFirstMergeStack);
				return true;

			case EStage::MoveFirstMergeStack:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 6, Ids.BasicContainerId, 0), TEXT("Actual Slate drag moved the first compatible stack before merge."));
				Stage = EStage::MergeSecondStack;
				DelayTicks = 3;
				return true;

			case EStage::MergeSecondStack:
				Record(InjectCellDrag(Ids.WarehouseContainerId, 7, Ids.BasicContainerId, 0), TEXT("Actual Slate drag entered and dropped on the compatible occupied merge target."));
				Stage = EStage::VerifyMerge;
				DelayTicks = 3;
				return true;

			case EStage::VerifyMerge:
				Record(AddressHasItemQuantity(Ids.BasicContainerId, 0, Ids.DustAItemId, 20), TEXT("Real merge preserved the target ItemId and authoritative final quantity."));
				Record(AddressIsEmpty(Ids.WarehouseContainerId, 7), TEXT("Merged source item no longer occupies its original warehouse slot."));
				Record(GetRevision() == OperationRevision + 2, TEXT("The two physical merge setup/merge gestures committed exactly two transactions."));
				Record(LastGestureHasCounts(1, 1, OperationRevision + 1), TEXT("The final accepted merge gesture logged exactly one P2 command/call and its revision boundary."));
				CaptureEvidence(TEXT("Merge"));
				Stage = EStage::PrepareSwap;
				return true;

			case EStage::PrepareSwap:
				ResetFixture(EStage::SwapWarehouseItems);
				return true;

			case EStage::SwapWarehouseItems:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 0, Ids.WarehouseContainerId, 1), TEXT("Actual Slate drag used the occupied storage target's Swap preview and drop."));
				Stage = EStage::VerifySwap;
				DelayTicks = 3;
				return true;

			case EStage::VerifySwap:
				Record(AddressHasItem(Ids.WarehouseContainerId, 0, Ids.WeaponBItemId) && AddressHasItem(Ids.WarehouseContainerId, 1, Ids.WeaponAItemId), TEXT("Real Swap exchanged the two stable ItemIds in the authoritative projection."));
				Record(GetRevision() == OperationRevision + 1, TEXT("Real Swap committed exactly one transaction."));
				Record(LastGestureHasCounts(1, 1, OperationRevision), TEXT("The accepted physical swap logged exactly one P2 command/call and its revision boundary."));
				Stage = EStage::PrepareEquipment;
				return true;

			case EStage::PrepareEquipment:
				ResetFixture(EStage::EquipWeapon);
				return true;

			case EStage::EquipWeapon:
				Record(InjectCellClick(Ids.WarehouseContainerId, 0), TEXT("Actual left click displayed the equipable item detail before equipment drag."));
				CaptureEvidence(TEXT("DetailEquipable"));
				Stage = EStage::EquipWeaponAfterDetail;
				DelayTicks = 3;
				return true;

			case EStage::EquipWeaponAfterDetail:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 0, Ids.WeaponContainerId, 0), TEXT("Actual Slate drag equipped a compatible weapon."));
				Stage = EStage::ReplaceWeapon;
				DelayTicks = 3;
				return true;

			case EStage::ReplaceWeapon:
				Record(InjectCellDrag(Ids.WarehouseContainerId, 1, Ids.WeaponContainerId, 0), TEXT("Actual Slate drag invoked equipment replacement on an occupied target."));
				Stage = EStage::UnequipWeapon;
				DelayTicks = 3;
				return true;

			case EStage::UnequipWeapon:
				Record(InjectCellClick(Ids.WeaponContainerId, 0), TEXT("Actual left click displayed the currently equipped item detail before unequip drag."));
				CaptureEvidence(TEXT("DetailEquipped"));
				Stage = EStage::UnequipWeaponAfterDetail;
				DelayTicks = 3;
				return true;

			case EStage::UnequipWeaponAfterDetail:
				Record(InjectCellDrag(Ids.WeaponContainerId, 0, Ids.BasicContainerId, 0), TEXT("Actual Slate drag unequipped the replacement weapon to a real storage target."));
				Stage = EStage::VerifyEquipment;
				DelayTicks = 3;
				return true;

			case EStage::VerifyEquipment:
				Record(AddressHasItem(Ids.BasicContainerId, 0, Ids.WeaponBItemId) && AddressIsEmpty(Ids.WeaponContainerId, 0), TEXT("Real Equip, Replacement and Unequip produced the expected final equipment projection."));
				Record(GetRevision() == OperationRevision + 3, TEXT("Equip, replacement and unequip each committed exactly once."));
				Record(LastGestureHasCounts(1, 1, OperationRevision + 2), TEXT("The accepted physical unequip logged exactly one P2 command/call and its revision boundary."));
				Stage = EStage::PrepareSpatial;
				return true;

			case EStage::PrepareSpatial:
				ResetFixture(EStage::EquipSpatialItem);
				return true;

			case EStage::EquipSpatialItem:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 5, Ids.SpatialContainerId, 0), TEXT("Actual Slate drag equipped the spatial item and exposed its production internal storage."));
				Stage = EStage::MoveToSpatialInternal;
				DelayTicks = 3;
				return true;

			case EStage::MoveToSpatialInternal:
				Record(InjectCellClick(Ids.SpatialContainerId, 0), TEXT("Actual left click displayed the equipped spatial ring and quick-space state."));
				CaptureEvidence(TEXT("RingEquippedQuickSpace"));
				Stage = EStage::MoveToSpatialInternalAfterDetail;
				DelayTicks = 3;
				return true;

			case EStage::MoveToSpatialInternalAfterDetail:
				Record(InjectCellDrag(Ids.WarehouseContainerId, 6, Ids.SpatialInternalContainerId, 0), TEXT("Actual Slate drag moved an item into the now-visible spatial internal storage."));
				Stage = EStage::AttemptLoadedSpatialMove;
				DelayTicks = 3;
				return true;

			case EStage::AttemptLoadedSpatialMove:
				SpatialRejectRevision = GetRevision();
				Record(InjectCellDrag(Ids.SpatialContainerId, 0, Ids.WarehouseContainerId, 10), TEXT("Actual Slate drag attempted the explicitly rejected loaded-spatial move."));
				Stage = EStage::VerifySpatial;
				DelayTicks = 3;
				return true;

			case EStage::VerifySpatial:
				Record(AddressHasItem(Ids.SpatialInternalContainerId, 0, Ids.DustAItemId), TEXT("Real spatial-internal move used the same P4/P3/P2/P1 chain."));
				Record(AddressHasItem(Ids.SpatialContainerId, 0, Ids.SpatialItemId) && GetRevision() == SpatialRejectRevision, TEXT("Loaded spatial rejection preserved item location and revision."));
				Record(GetFeedbackContains(TEXT("空间道具已装载物品")), TEXT("Loaded spatial rejection returned production Chinese feedback through the mounted page."));
				Record(LastGestureHasCounts(0, 0, SpatialRejectRevision), TEXT("The rejected loaded-ring drag logged zero P2 commands/calls and an unchanged revision."));
				CaptureEvidence(TEXT("RejectLoadedRing"));
				Stage = EStage::PreparePouch;
				return true;

			case EStage::PreparePouch:
				ResetFixture(EStage::MovePouchToBasic);
				return true;

			case EStage::MovePouchToBasic:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 10, Ids.BasicContainerId, 0), TEXT("Actual Slate drag moved an empty non-quick spatial pouch as an ordinary storage item."));
				Stage = EStage::MoveDustToPouch;
				DelayTicks = 3;
				return true;

			case EStage::MoveDustToPouch:
				Record(InjectCellClick(Ids.BasicContainerId, 0), TEXT("Actual left click displayed the moved non-quick pouch before its child-container drag."));
				CaptureEvidence(TEXT("PouchNonQuick"));
				Stage = EStage::MoveDustToPouchAfterDetail;
				DelayTicks = 3;
				return true;

			case EStage::MoveDustToPouchAfterDetail:
				Record(InjectCellDrag(Ids.WarehouseContainerId, 6, Ids.PouchInternalContainerId, 0), TEXT("Actual Slate drag moved an item into the non-quick pouch child container."));
				Stage = EStage::AttemptLoadedPouchMove;
				DelayTicks = 3;
				return true;

			case EStage::AttemptLoadedPouchMove:
				SpatialRejectRevision = GetRevision();
				Record(InjectCellDrag(Ids.BasicContainerId, 0, Ids.WarehouseContainerId, 10), TEXT("Actual Slate drag attempted the explicitly rejected loaded-pouch move."));
				Stage = EStage::VerifyPouch;
				DelayTicks = 3;
				return true;

			case EStage::VerifyPouch:
				Record(AddressHasItem(Ids.PouchInternalContainerId, 0, Ids.DustAItemId), TEXT("Non-quick pouch internal storage retains its one authoritative item instance."));
				Record(AddressHasItem(Ids.BasicContainerId, 0, Ids.SpatialPouchItemId) && GetRevision() == SpatialRejectRevision, TEXT("Loaded pouch rejection preserved parent location and revision."));
				Record(GetFeedbackContains(TEXT("空间道具已装载物品")), TEXT("Loaded pouch rejection returned production Chinese feedback through the mounted page."));
				Record(LastGestureHasCounts(0, 0, SpatialRejectRevision), TEXT("The rejected loaded-pouch drag logged zero P2 commands/calls and an unchanged revision."));
				Stage = EStage::PrepareRejects;
				return true;

			case EStage::PrepareRejects:
				ResetFixture(EStage::RejectInvalidEquip);
				return true;

			case EStage::RejectInvalidEquip:
				OperationRevision = GetRevision();
				Record(InjectCellDrag(Ids.WarehouseContainerId, 9, Ids.WeaponContainerId, 0), TEXT("Actual Slate drag attempted an incompatible equipment target."));
				Stage = EStage::RejectSameSource;
				DelayTicks = 3;
				return true;

			case EStage::RejectSameSource:
				Record(InjectCellDrag(Ids.WarehouseContainerId, 6, Ids.WarehouseContainerId, 6), TEXT("Actual Slate drag attempted the same source cell."));
				Stage = EStage::VerifyRejects;
				DelayTicks = 3;
				return true;

			case EStage::VerifyRejects:
				Record(GetRevision() == OperationRevision, TEXT("Incompatible and same-source real drops submitted no transaction."));
				Record(GetFeedbackContains(TEXT("不能拖放到同一来源格")), TEXT("Final real rejected drop supplied the expected Chinese feedback."));
				Record(LastGestureHasCounts(0, 0, OperationRevision), TEXT("The same-source rejection logged zero P2 commands/calls and an unchanged revision."));
				Stage = EStage::PrepareDoubleClickNoWrite;
				return true;

			case EStage::PrepareDoubleClickNoWrite:
				ResetFixture(EStage::FirstNoWriteClick);
				return true;

			case EStage::FirstNoWriteClick:
				OperationRevision = GetRevision();
				Record(InjectCellClick(Ids.WarehouseContainerId, 6), TEXT("First physical click used the mounted cell path without submitting a move."));
				Stage = EStage::SecondNoWriteDoubleClick;
				DelayTicks = 2;
				return true;

			case EStage::SecondNoWriteDoubleClick:
				Record(InjectCellDoubleClick(Ids.WarehouseContainerId, 6), TEXT("Slate double-click reached the refreshed mounted cell without a QuickMove route."));
				Stage = EStage::VerifyDoubleClickNoWrite;
				DelayTicks = 3;
				return true;

			case EStage::VerifyDoubleClickNoWrite:
				Record(AddressHasItem(Ids.WarehouseContainerId, 6, Ids.DustAItemId), TEXT("Real double-click left the item in its original authoritative slot."));
				Record(GetRevision() == OperationRevision, TEXT("Double-click submitted no item command."));
				Record(LastGestureHasCounts(0, 0, OperationRevision), TEXT("True double-click logged zero P2 commands/calls and an unchanged revision."));
				Stage = EStage::PrepareContextNoWrite;
				return true;

			case EStage::PrepareContextNoWrite:
				ResetFixture(EStage::OpenContextMenu);
				return true;

			case EStage::OpenContextMenu:
				OperationRevision = GetRevision();
				Record(InjectCellClick(Ids.WarehouseContainerId, 6, EKeys::RightMouseButton), TEXT("Actual right-click opened the mounted P4 context menu."));
				Stage = EStage::VerifyContextNoWrite;
				DelayTicks = 2;
				return true;

			case EStage::VerifyContextNoWrite:
				Record(AddressHasItem(Ids.WarehouseContainerId, 6, Ids.DustAItemId), TEXT("Right-click context opening left the authoritative item in place."));
				Record(GetRevision() == OperationRevision, TEXT("Right-click context menu submitted no item transaction."));
				Record(GetFeedbackContains(TEXT("仅用于查看详情")), TEXT("Mounted context menu reports its no-write rule in Chinese."));
				Record(LastGestureHasCounts(0, 0, OperationRevision), TEXT("True right-click logged zero P2 commands/calls and an unchanged revision."));
				Stage = EStage::PrepareEscapeCancellation;
				return true;

			case EStage::PrepareEscapeCancellation:
				ResetFixture(EStage::BeginDragForEscape);
				return true;

			case EStage::BeginDragForEscape:
				OperationRevision = GetRevision();
				Record(InjectCellDragBegin(Ids.WarehouseContainerId, 6, Ids.BasicContainerId, 0), TEXT("Actual mounted drag began before Escape cancellation."));
				CaptureEvidence(TEXT("DragHighlight"));
				Stage = EStage::EscapeMountedDrag;
				DelayTicks = 1;
				return true;

			case EStage::EscapeMountedDrag:
				Record(InjectEscape(), TEXT("Slate keyboard routing delivered Escape to the mounted page."));
				Stage = EStage::VerifyEscapeAndReopen;
				DelayTicks = 3;
				return true;

			case EStage::VerifyEscapeAndReopen:
				Record(GetRevision() == OperationRevision, TEXT("Escape-cancelled drag did not change the repository revision."));
				Record(Host.IsValid() && Host->GetActiveWidget() != nullptr && !Host->GetActiveWidget()->HasP4Preview(), TEXT("Escape released the Slate drag and cleared mounted preview state without closing the page."));
				Stage = EStage::EscapeMountedClose;
				DelayTicks = 4;
				return true;

			case EStage::EscapeMountedClose:
				Record(InjectEscape(), TEXT("A second actual Slate Escape invoked the mounted page Close route after cancellation."));
				Stage = EStage::VerifyCloseAndReopen;
				DelayTicks = 3;
				return true;

			case EStage::VerifyCloseAndReopen:
				Record(!Host.IsValid() || Host->GetActiveWidget() == nullptr || !Host->GetActiveWidget()->IsInViewport(), TEXT("Close control released the mounted page from the viewport."));
				Record(Host.IsValid() && Host->OpenPage(), TEXT("Host reopened with its existing repository instance."));
				Stage = EStage::Complete;
				DelayTicks = 3;
				return true;

			case EStage::Complete:
				Record(Host.IsValid() && Host->GetActiveWidget() != nullptr, TEXT("Reopened mounted page remains input-ready."));
				if (Host.IsValid())
				{
					Host->ClosePage();
				}
				return Finish();
			}
			return Finish();
		}

		void ResetFixture(const EStage NextStage)
		{
			Record(Host.IsValid() && Host->IsHostEnabled(), TEXT("The production host remained explicitly enabled during real-input reset."));
			if (Host.IsValid())
			{
				Host->ResetDevelopmentFixture();
				Host->RefreshActivePage();
			}
			Stage = NextStage;
			DelayTicks = 5;
		}

		int32 GetRevision() const
		{
			return Host.IsValid() && Host->GetController() ? Host->GetController()->GetProjection().Revision : INDEX_NONE;
		}

		bool SelectedIs(const FGuid& ContainerId, const int32 SlotIndex) const
		{
			if (!Host.IsValid() || !Host->GetController() || !Host->GetController()->GetSelectedAddress().IsSet())
			{
				return false;
			}
			const FCodeBP3SlotAddress& Address = Host->GetController()->GetSelectedAddress().GetValue();
			return Address.ContainerId == ContainerId && Address.SlotIndex == SlotIndex;
		}

		bool AddressHasItem(const FGuid& ContainerId, const int32 SlotIndex, const FGuid& ItemId) const
		{
			FCodeBP3SlotAddress Address;
			return Host.IsValid() && Host->GetController()
				&& Host->GetController()->MakeAddress(ContainerId, SlotIndex, Address)
				&& Address.bOccupied && Address.ItemId == ItemId;
		}

		bool AddressIsEmpty(const FGuid& ContainerId, const int32 SlotIndex) const
		{
			FCodeBP3SlotAddress Address;
			return Host.IsValid() && Host->GetController()
				&& Host->GetController()->MakeAddress(ContainerId, SlotIndex, Address)
				&& !Address.bOccupied;
		}

		bool AddressHasItemQuantity(const FGuid& ContainerId, const int32 SlotIndex, const FGuid& ItemId, const int32 Quantity) const
		{
			if (!Host.IsValid() || !Host->GetController())
			{
				return false;
			}
			const FCodeBP2ContainerView* Container = Host->GetController()->GetProjection().Containers.FindByPredicate([&ContainerId](const FCodeBP2ContainerView& Candidate)
			{
				return Candidate.ContainerId == ContainerId;
			});
			return Container && Container->Slots.IsValidIndex(SlotIndex)
				&& Container->Slots[SlotIndex].bOccupied && Container->Slots[SlotIndex].ItemId == ItemId && Container->Slots[SlotIndex].Quantity == Quantity;
		}

		bool LastGestureHasCounts(const int32 ExpectedCommandCount, const int32 ExpectedCallCount, const int32 ExpectedRevisionBefore) const
		{
			const UCodeBP3InventoryWidget* Page = Host.IsValid() ? Host->GetActiveWidget() : nullptr;
			return Page
				&& Page->GetP4CommandCount() == ExpectedCommandCount
				&& Page->GetP4CallCount() == ExpectedCallCount
				&& Page->GetP4RevisionBefore() == ExpectedRevisionBefore;
		}

		void CaptureEvidence(const TCHAR* Label) const
		{
			if (!bCaptureEvidence)
			{
				return;
			}
			const FString FileName = FString::Printf(TEXT("P4x.0.r2_%s_%dx%d.png"), Label, GSystemResolution.ResX, GSystemResolution.ResY);
			CaptureP4xFrame(FileName);
			UE_LOG(LogTemp, Display, TEXT("P4X.EVIDENCE_CAPTURE File=%s Route=SlateHitTest->MountedP3P4"), *FileName);
		}

		bool GetFeedbackContains(const FString& ExpectedText) const
		{
			return Host.IsValid() && Host->GetController() && Host->GetController()->GetFeedback().Contains(ExpectedText);
		}

		bool ResolveMountedWidget(UWidget* Widget, FVector2D& OutPosition, TSharedPtr<FGenericWindow>& OutWindow)
		{
			if (!Widget || !FSlateApplication::IsInitialized())
			{
				return false;
			}
			const FGeometry& Geometry = Widget->GetCachedGeometry();
			if (Geometry.GetLocalSize().X <= 1.0f || Geometry.GetLocalSize().Y <= 1.0f)
			{
				return false;
			}
			OutPosition = Geometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
			TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
			FSlateApplication& Slate = FSlateApplication::Get();
			const FWidgetPath HitPath = Slate.LocateWindowUnderMouse(OutPosition, Slate.GetInteractiveTopLevelWindows(), false);
			if (!HitPath.IsValid() || !HitPath.ContainsWidget(&SlateWidget.Get()))
			{
				UE_LOG(LogTemp, Error, TEXT("%s failed Slate hit-test validation for mounted widget %s at %.1f,%.1f."), *TraceTag, *Widget->GetName(), OutPosition.X, OutPosition.Y);
				return false;
			}
			OutWindow = HitPath.GetWindow()->GetNativeWindow();
			return true;
		}

		FPointerEvent MakePointerEvent(const FVector2D& Position, const FVector2D& LastPosition, const FKey& EffectingButton) const
		{
			FSlateApplication& Slate = FSlateApplication::Get();
			return FPointerEvent(FSlateApplication::CursorPointerIndex, Position, LastPosition, Slate.GetPressedMouseButtons(), EffectingButton, 0.0f, Slate.GetModifierKeys());
		}

		bool InjectWidgetClick(UWidget* Widget, const FKey Button = EKeys::LeftMouseButton, const bool bDoubleClick = false)
		{
			FVector2D Position;
			TSharedPtr<FGenericWindow> Window;
			if (!ResolveMountedWidget(Widget, Position, Window))
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			Slate.ProcessMouseMoveEvent(MakePointerEvent(Position, Position, EKeys::Invalid));
			const bool bDownHandled = bDoubleClick
				? Slate.ProcessMouseButtonDoubleClickEvent(Window, MakePointerEvent(Position, Position, Button))
				: Slate.ProcessMouseButtonDownEvent(Window, MakePointerEvent(Position, Position, Button));
			const bool bUpHandled = Slate.ProcessMouseButtonUpEvent(MakePointerEvent(Position, Position, Button));
			UE_LOG(LogTemp, Display, TEXT("%s Pointer Button=%s Double=%d DownHandled=%d UpHandled=%d Position=%.1f,%.1f"), *TraceTag, *Button.ToString(), bDoubleClick ? 1 : 0, bDownHandled ? 1 : 0, bUpHandled ? 1 : 0, Position.X, Position.Y);
			return bDownHandled || bUpHandled;
		}

		bool InjectCellClick(const FGuid& ContainerId, const int32 SlotIndex, const FKey Button = EKeys::LeftMouseButton)
		{
			return InjectWidgetClick(Host.IsValid() && Host->GetActiveWidget() ? Host->GetActiveWidget()->FindMountedCell(ContainerId, SlotIndex) : nullptr, Button);
		}

		bool InjectCellDoubleClick(const FGuid& ContainerId, const int32 SlotIndex)
		{
			return InjectWidgetClick(Host.IsValid() && Host->GetActiveWidget() ? Host->GetActiveWidget()->FindMountedCell(ContainerId, SlotIndex) : nullptr, EKeys::LeftMouseButton, true);
		}

		bool InjectCellDrag(const FGuid& SourceContainerId, const int32 SourceSlotIndex, const FGuid& TargetContainerId, const int32 TargetSlotIndex)
		{
			return InjectCellDragInternal(SourceContainerId, SourceSlotIndex, TargetContainerId, TargetSlotIndex, true);
		}

		bool InjectCellDragBegin(const FGuid& SourceContainerId, const int32 SourceSlotIndex, const FGuid& TargetContainerId, const int32 TargetSlotIndex)
		{
			return InjectCellDragInternal(SourceContainerId, SourceSlotIndex, TargetContainerId, TargetSlotIndex, false);
		}

		bool InjectCellDragInternal(const FGuid& SourceContainerId, const int32 SourceSlotIndex, const FGuid& TargetContainerId, const int32 TargetSlotIndex, const bool bReleaseOnTarget)
		{
			UCodeBP3InventoryWidget* Page = Host.IsValid() ? Host->GetActiveWidget() : nullptr;
			UCodeBP3CellButton* Source = Page ? Page->FindMountedCell(SourceContainerId, SourceSlotIndex) : nullptr;
			UCodeBP3CellButton* Target = Page ? Page->FindMountedCell(TargetContainerId, TargetSlotIndex) : nullptr;
			FVector2D SourcePosition;
			FVector2D TargetPosition;
			TSharedPtr<FGenericWindow> SourceWindow;
			TSharedPtr<FGenericWindow> TargetWindow;
			if (!ResolveMountedWidget(Source, SourcePosition, SourceWindow) || !ResolveMountedWidget(Target, TargetPosition, TargetWindow))
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			Slate.ProcessMouseMoveEvent(MakePointerEvent(SourcePosition, SourcePosition, EKeys::Invalid));
			const bool bDownHandled = Slate.ProcessMouseButtonDownEvent(SourceWindow, MakePointerEvent(SourcePosition, SourcePosition, EKeys::LeftMouseButton));
			const FVector2D Delta = TargetPosition - SourcePosition;
			const FVector2D ThresholdPosition = SourcePosition + (Delta.IsNearlyZero() ? FVector2D(24.0f, 0.0f) : Delta.GetSafeNormal() * 24.0f);
			const bool bThresholdHandled = Slate.ProcessMouseMoveEvent(MakePointerEvent(ThresholdPosition, SourcePosition, EKeys::Invalid));
			const bool bTargetHandled = Slate.ProcessMouseMoveEvent(MakePointerEvent(TargetPosition, ThresholdPosition, EKeys::Invalid));
			const bool bReleaseHandled = bReleaseOnTarget ? Slate.ProcessMouseButtonUpEvent(MakePointerEvent(TargetPosition, TargetPosition, EKeys::LeftMouseButton)) : true;
			UE_LOG(LogTemp, Display, TEXT("%s Drag Source=%s:%d Target=%s:%d Release=%d Down=%d Threshold=%d TargetMove=%d Up=%d"),
				*TraceTag, *SourceContainerId.ToString(EGuidFormats::DigitsWithHyphens), SourceSlotIndex, *TargetContainerId.ToString(EGuidFormats::DigitsWithHyphens), TargetSlotIndex, bReleaseOnTarget ? 1 : 0,
				bDownHandled ? 1 : 0, bThresholdHandled ? 1 : 0, bTargetHandled ? 1 : 0, bReleaseHandled ? 1 : 0);
			// Move routing may intentionally report unhandled while Slate is still carrying
			// the captured drag. The mounted cell's trace above is the authoritative proof
			// of DragOperation/Enter/Leave; down and release must still be handled.
			return bDownHandled && bReleaseHandled;
		}

		bool InjectEscape() const
		{
			if (!FSlateApplication::IsInitialized())
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			if (Host.IsValid() && Host->GetActiveWidget())
			{
				// Restore the same mounted page focus that OpenPage's UI-only input mode owns
				// before sending a real Slate key event; no page handler is called directly.
				Host->GetActiveWidget()->SetKeyboardFocus();
			}
			return Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Escape, Slate.GetModifierKeys(), 0, false, 0, 0));
		}

		void Record(const bool bCondition, const TCHAR* Message)
		{
			if (bCondition)
			{
				UE_LOG(LogTemp, Display, TEXT("%s PASS %s"), *TraceTag, Message);
			}
			else
			{
				bSucceeded = false;
				UE_LOG(LogTemp, Error, TEXT("%s FAIL %s"), *TraceTag, Message);
			}
		}

		bool Finish()
		{
			const bool bResult = bSucceeded;
			if (bResult)
			{
				UE_LOG(LogTemp, Display, TEXT("%s PASS Resolution=%dx%d"), *TraceTag, GSystemResolution.ResX, GSystemResolution.ResY);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("%s FAIL Resolution=%dx%d"), *TraceTag, GSystemResolution.ResX, GSystemResolution.ResY);
			}
			TSharedPtr<FP4RealInputTraceHarness> KeepAlive = AsShared();
			ActiveHarness.Reset();
			if (bExitWhenFinished)
			{
				FPlatformMisc::RequestExitWithStatus(bResult ? 0 : 1, false);
			}
			return false;
		}

		int32 ExpectedWidth = 0;
		int32 ExpectedHeight = 0;
		bool bExitWhenFinished = false;
		bool bCaptureEvidence = false;
		FString TraceTag;
		bool bSucceeded = true;
		int32 TickCount = 0;
		int32 DelayTicks = 0;
		int32 SelectionRevision = INDEX_NONE;
		int32 OperationRevision = INDEX_NONE;
		int32 SpatialRejectRevision = INDEX_NONE;
		EStage Stage = EStage::WaitForHost;
		TWeakObjectPtr<UCodeBP3UIHostSubsystem> Host;
		FCodeBP2FixtureIds Ids;
		FTSTicker::FDelegateHandle TickerHandle;

	public:
		static TSharedPtr<FP4RealInputTraceHarness> ActiveHarness;
	};

	TSharedPtr<FP4RealInputTraceHarness> FP4RealInputTraceHarness::ActiveHarness;

	void RunRealInputTrace(const TArray<FString>& Args, const bool bCaptureEvidence, const TCHAR* TraceTag)
	{
		if (FP4RealInputTraceHarness::ActiveHarness.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s is already running."), TraceTag);
			return;
		}
		const int32 ExpectedWidth = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		const int32 ExpectedHeight = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		const bool bQuitWhenDone = Args.Contains(TEXT("Quit"));
		FP4RealInputTraceHarness::ActiveHarness = MakeShared<FP4RealInputTraceHarness>(ExpectedWidth, ExpectedHeight, bQuitWhenDone, bCaptureEvidence, TraceTag);
		FP4RealInputTraceHarness::ActiveHarness->Start();
	}

	void RunP4RealInputTrace(const TArray<FString>& Args)
	{
		RunRealInputTrace(Args, false, TEXT("P4.RealInputTrace"));
	}

	void RunP4xRealInputTrace(const TArray<FString>& Args)
	{
		RunRealInputTrace(Args, true, TEXT("P4X.RealInputTrace"));
	}

#if !UE_BUILD_SHIPPING
	/**
	 * P5's product trace intentionally starts at the visible Sect building button.
	 * It never calls the Code B host, manager entry, controller, P2, P1, or store
	 * to write state.  The only mutations below are native Slate mouse gestures on
	 * the mounted navigation and inventory widgets; projection/store reads are used
	 * solely to assert the result of those gestures.
	 */
	class FP5RealProfileTraceHarness : public TSharedFromThis<FP5RealProfileTraceHarness>
	{
	public:
		FP5RealProfileTraceHarness(
			FString InProfileCase,
			const int32 InExpectedWidth,
			const int32 InExpectedHeight,
			const bool bInExitWhenFinished)
			: ProfileCase(MoveTemp(InProfileCase))
			, ExpectedWidth(InExpectedWidth)
			, ExpectedHeight(InExpectedHeight)
			, bExitWhenFinished(bInExitWhenFinished)
		{
		}

		void Start()
		{
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateSP(AsShared(), &FP5RealProfileTraceHarness::Tick), 0.05f);
		}

	private:
		enum class EStage : uint8
		{
			WaitForSect,
			ClickNormalWarehouseEntry,
			WaitForProfilePage,
			EmptyProfileNoWrite,
			VerifyEmptyProfileNoWrite,
			VerifyRestartState,
			MoveWarehouseToBasic,
			VerifyWarehouseToBasicMove,
			MergeWarehouseMaterial,
			VerifyWarehouseMaterialMerge,
			SwapWarehouseItems,
			VerifyWarehouseSwap,
			EquipRole,
			VerifyEquipRole,
			ReplaceRole,
			VerifyReplaceRole,
			UnequipRole,
			VerifyUnequipRole,
			ReequipSpatialRing,
			ReequipBackpack,
			VerifyChildContainers,
			MoveToQuickSpatial,
			VerifyQuickSpatialMove,
			MoveToPouch,
			VerifyPouchMove,
			RejectLoadedRing,
			VerifyLoadedRingReject,
			RejectLoadedBackpack,
			VerifyLoadedBackpackReject,
			RejectRingInWeaponSlot,
			VerifyRejectedDrop,
			DetailNoWrite,
			VerifyDetailNoWrite,
			DoubleClickNoWrite,
			VerifyDoubleClickNoWrite,
			BeginEscapeCancellation,
			SendEscapeCancellation,
			VerifyEscapeCancellation,
			RightClickNoWrite,
			VerifyRightClickNoWrite,
			CloseProfilePage,
			ReopenNormalWarehouseEntry,
			VerifyReopen,
			Complete
		};

		bool Tick(const float DeltaSeconds)
		{
			if (++TickCount > 500)
			{
				Record(false, TEXT("Timed out waiting for the real Profile trace."));
				return Finish();
			}
			if (DelayTicks > 0)
			{
				--DelayTicks;
				return true;
			}

			switch (Stage)
			{
			case EStage::WaitForSect:
				Manager = FindProgressionManager();
				if (!Manager.IsValid() || !Manager->GetSectNavigationWidget())
				{
					DelayTicks = 2;
					return true;
				}
				Record(!Manager->HasCodeBOutOfRaidProfileForAutomation(), TEXT("Default map has no Code B Profile host before the normal entrance is clicked."));
				Record(Manager->GetSectNavigationWidget()->GetCurrentPage() == Edemo_mapSectPage::Home, TEXT("Trace begins on the visible Sect home navigation page."));
				Record(ExpectedWidth <= 0 || (GSystemResolution.ResX == ExpectedWidth && GSystemResolution.ResY == ExpectedHeight), TEXT("Requested trace resolution is active."));
				CaptureP5Frame(TEXT("DefaultMap_NoCodeBHost"));
				Stage = EStage::ClickNormalWarehouseEntry;
				return true;

			case EStage::ClickNormalWarehouseEntry:
				Record(ClickVisibleWarehouseEntry(), TEXT("Physical Slate click reached the normal Sect warehouse/person-configuration entrance."));
				Stage = EStage::WaitForProfilePage;
				DelayTicks = 5;
				return true;

			case EStage::WaitForProfilePage:
				Host = FindProfileHostForManager();
				if (!Host.IsValid() || !Host->GetActiveWidget() || !Host->GetController() || !Host->GetController()->IsProfileBacked())
				{
					if (Manager.IsValid() && !Manager->IsCodeBOutOfRaidInventoryEntryAvailable())
					{
						Record(!Manager->HasCodeBOutOfRaidProfileForAutomation(), TEXT("Active Run normal-entry rejection created no Code B Repository or persistent record."));
						Record(!Host->GetActiveWidget(), TEXT("Active Run normal-entry rejection did not create a Code B page."));
						UE_LOG(LogTemp, Display, TEXT("P5.RealProfileTrace ActiveRunRejected EntryAvailable=0 RepositoryCreated=0 PersistentWrites=0 Route=SectSlate->Manager"));
						CaptureP5Frame(TEXT("ActiveRunRejected_NoCodeBWrite"));
						Stage = EStage::Complete;
						return true;
					}
					DelayTicks = 2;
					return true;
				}
				Record(Manager.IsValid() && Manager->HasCodeBOutOfRaidProfileForAutomation(), TEXT("Normal entrance lazily created the OwnerId-isolated Code B Profile host."));
				Record(Host->GetController()->GetFixtureIds() == nullptr, TEXT("Normal entrance did not initialize the development fixture."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidOwnerIdForAutomation().IsValid(), TEXT("Profile trace has a stable formal OwnerId."));
				InitialPersistentRevision = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				PersistentBeforeGesture = InitialPersistentRevision;
				UE_LOG(LogTemp, Display, TEXT("P5.RealProfileTrace Begin Case=%s OwnerId=%s HandoffState=%d RepositoryRevision=%d PersistentRevision=%d Route=SectSlate->Manager->ProfileHost->P4->P3->P2->P1"),
					*ProfileCase,
					Manager.IsValid() ? *Manager->GetCodeBOutOfRaidOwnerIdForAutomation().ToString(EGuidFormats::DigitsWithHyphens) : TEXT("Invalid"),
					Manager.IsValid() ? Manager->GetCodeBOutOfRaidHandoffStateForAutomation() : INDEX_NONE,
					GetRepositoryRevision(), InitialPersistentRevision);
				CaptureP5Frame(TEXT("NormalEntry_RealProfile"));
				Stage = IsRestartVerification()
					? EStage::VerifyRestartState
					: (IsEmptyProfile() ? EStage::EmptyProfileNoWrite : EStage::MoveWarehouseToBasic);
				return true;

			case EStage::VerifyRestartState:
				Record(RoleContainsItemType(FName(TEXT("SpatialRing")), ECodeBItemType::SpatialItem)
					&& RoleContainsItemType(FName(TEXT("Backpack")), ECodeBItemType::Backpack)
					&& RoleContainsItemType(FName(TEXT("QuickSpatial")), ECodeBItemType::Material)
					&& RoleContainsItemType(FName(TEXT("PouchInternal")), ECodeBItemType::Material),
					TEXT("A new process restored the committed Profile owners and child contents before any item gesture."));
				Record(GetRepositoryRevision() >= 0 && Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == InitialPersistentRevision,
					TEXT("Restart verification performed no Repository or persistent write."));
				TraceGesture(TEXT("RestartReadOnly"));
				Stage = EStage::CloseProfilePage;
				return true;

			case EStage::EmptyProfileNoWrite:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(ClickFirstEmptyRoleSlot(FName(TEXT("Warehouse"))), TEXT("Physical Slate click reached an empty real-Profile warehouse cell."));
				Stage = EStage::VerifyEmptyProfileNoWrite;
				DelayTicks = 2;
				return true;

			case EStage::VerifyEmptyProfileNoWrite:
				Record(ProjectionHasNoItems(), TEXT("EmptyProfile remains empty after the mounted empty-cell interaction."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("EmptyProfile interaction submitted no Repository transaction."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("EmptyProfile interaction submitted no persistent write."));
				TraceGesture(TEXT("EmptyProfileNoWrite"));
				Stage = EStage::CloseProfilePage;
				return true;

			case EStage::MoveWarehouseToBasic:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragFirstItemOfTypeToRole(ECodeBItemType::Material, FName(TEXT("Basic6"))), TEXT("Physical Slate drag moved a real Profile material from warehouse to basic storage."));
				Stage = EStage::VerifyWarehouseToBasicMove;
				DelayTicks = 3;
				return true;

			case EStage::VerifyWarehouseToBasicMove:
				Record(RoleContainsItemType(FName(TEXT("Basic6")), ECodeBItemType::Material), TEXT("Basic storage contains the physically moved Profile material."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Warehouse-to-basic move advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Warehouse-to-basic move advanced the persistent revision exactly once."));
				TraceGesture(TEXT("MoveWarehouseToBasic6"));
				Stage = EStage::MergeWarehouseMaterial;
				return true;

			case EStage::MergeWarehouseMaterial:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragRoleItemToRole(FName(TEXT("Warehouse")), ECodeBItemType::Material, FName(TEXT("Basic6"))), TEXT("Physical Slate drag merged a compatible Profile material stack into basic storage."));
				Stage = EStage::VerifyWarehouseMaterialMerge;
				DelayTicks = 3;
				return true;

			case EStage::VerifyWarehouseMaterialMerge:
				Record(RoleContainsItemType(FName(TEXT("Basic6")), ECodeBItemType::Material), TEXT("Basic storage retained the authoritative merged material stack."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Material merge advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Material merge advanced the persistent revision exactly once."));
				TraceGesture(TEXT("MergeWarehouseMaterial"));
				Stage = EStage::SwapWarehouseItems;
				return true;

			case EStage::SwapWarehouseItems:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragTwoWarehouseItemsForSwap(), TEXT("Physical Slate drag swapped two occupied real-Profile warehouse cells."));
				Stage = EStage::VerifyWarehouseSwap;
				DelayTicks = 3;
				return true;

			case EStage::VerifyWarehouseSwap:
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Warehouse swap advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Warehouse swap advanced the persistent revision exactly once."));
				TraceGesture(TEXT("SwapWarehouseItems"));
				Stage = EStage::EquipRole;
				return true;

			case EStage::EquipRole:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragFirstItemOfTypeToRole(CurrentEquipmentType(), CurrentEquipmentRole()), TEXT("Physical Slate drag equipped the current real-Profile equipment role."));
				Stage = EStage::VerifyEquipRole;
				DelayTicks = 3;
				return true;

			case EStage::VerifyEquipRole:
				Record(RoleContainsItemType(CurrentEquipmentRole(), CurrentEquipmentType()), TEXT("Physical drop populated the current equipment role."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Accepted equipment drag advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Accepted equipment drag advanced the persistent revision exactly once."));
				TraceGesture(TEXT("EquipRole"));
				Stage = EStage::ReplaceRole;
				return true;

			case EStage::ReplaceRole:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragRoleItemToRole(FName(TEXT("Warehouse")), CurrentEquipmentType(), CurrentEquipmentRole()), TEXT("Physical Slate drag replaced the occupied real-Profile equipment role."));
				Stage = EStage::VerifyReplaceRole;
				DelayTicks = 3;
				return true;

			case EStage::VerifyReplaceRole:
				Record(RoleContainsItemType(CurrentEquipmentRole(), CurrentEquipmentType()), TEXT("Replacement retained a valid current equipment projection."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Accepted equipment replacement advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Accepted equipment replacement advanced the persistent revision exactly once."));
				TraceGesture(TEXT("ReplaceRole"));
				Stage = EStage::UnequipRole;
				return true;

			case EStage::UnequipRole:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragRoleItemToEmptyRole(CurrentEquipmentRole(), CurrentEquipmentType(), FName(TEXT("Warehouse"))), TEXT("Physical Slate drag unequipped the current role into warehouse storage."));
				Stage = EStage::VerifyUnequipRole;
				DelayTicks = 3;
				return true;

			case EStage::VerifyUnequipRole:
				Record(!RoleContainsItemType(CurrentEquipmentRole(), CurrentEquipmentType()), TEXT("Unequip cleared the current real-Profile equipment role."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Accepted unequip advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Accepted unequip advanced the persistent revision exactly once."));
				TraceGesture(TEXT("UnequipRole"));
				++EquipmentRoleIndex;
				Stage = EquipmentRoleIndex < 5 ? EStage::EquipRole : EStage::ReequipSpatialRing;
				return true;

			case EStage::ReequipSpatialRing:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragFirstItemOfTypeToRole(ECodeBItemType::SpatialItem, FName(TEXT("SpatialRing"))), TEXT("Physical Slate drag re-equipped an empty ring for its real child-container check."));
				Stage = EStage::ReequipBackpack;
				DelayTicks = 3;
				return true;

			case EStage::ReequipBackpack:
				Record(RoleContainsItemType(FName(TEXT("SpatialRing")), ECodeBItemType::SpatialItem), TEXT("Real ring re-equip populated the SpatialRing role."));
				Record(DragFirstItemOfTypeToRole(ECodeBItemType::Backpack, FName(TEXT("Backpack"))), TEXT("Physical Slate drag re-equipped an empty backpack for its real child-container check."));
				Stage = EStage::VerifyChildContainers;
				DelayTicks = 3;
				return true;

			case EStage::VerifyChildContainers:
				Record(RoleContainsItemType(FName(TEXT("Backpack")), ECodeBItemType::Backpack), TEXT("Real backpack re-equip populated the Backpack role."));
				Record(FindRole(FName(TEXT("QuickSpatial"))) != nullptr && FindRole(FName(TEXT("PouchInternal"))) != nullptr, TEXT("Real equipped ring and backpack expose their owned child containers."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 2, TEXT("Ring and backpack re-equips committed exactly two Repository transactions."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 2, TEXT("Ring and backpack re-equips committed exactly two persistent writes."));
				TraceGesture(TEXT("ReequipSpatialOwners"));
				CaptureP5Frame(TEXT("RingAndBackpackEquipped_ChildrenVisible"));
				Stage = EStage::MoveToQuickSpatial;
				return true;

			case EStage::MoveToQuickSpatial:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragFirstItemOfTypeToRole(ECodeBItemType::Material, FName(TEXT("QuickSpatial"))), TEXT("Physical Slate drag moved a real Profile material into the ring child container."));
				Stage = EStage::VerifyQuickSpatialMove;
				DelayTicks = 3;
				return true;

			case EStage::VerifyQuickSpatialMove:
				Record(RoleContainsItemType(FName(TEXT("QuickSpatial")), ECodeBItemType::Material), TEXT("Ring child container retained the physically moved material."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Ring-child move advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Ring-child move advanced the persistent revision exactly once."));
				TraceGesture(TEXT("MoveToQuickSpatial"));
				Stage = EStage::MoveToPouch;
				return true;

			case EStage::MoveToPouch:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragFirstItemOfTypeToRole(ECodeBItemType::Material, FName(TEXT("PouchInternal"))), TEXT("Physical Slate drag moved a real Profile material into the backpack child container."));
				Stage = EStage::VerifyPouchMove;
				DelayTicks = 3;
				return true;

			case EStage::VerifyPouchMove:
				Record(RoleContainsItemType(FName(TEXT("PouchInternal")), ECodeBItemType::Material), TEXT("Backpack child container retained the physically moved material."));
				Record(GetRepositoryRevision() == RevisionBeforeGesture + 1, TEXT("Backpack-child move advanced the Repository revision exactly once."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture + 1, TEXT("Backpack-child move advanced the persistent revision exactly once."));
				TraceGesture(TEXT("MoveToPouchInternal"));
				Stage = EStage::RejectLoadedRing;
				return true;

			case EStage::RejectLoadedRing:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragRoleItemToRole(FName(TEXT("SpatialRing")), ECodeBItemType::SpatialItem, FName(TEXT("Warehouse"))), TEXT("Physical Slate drag attempted to move a loaded real Profile ring."));
				Stage = EStage::VerifyLoadedRingReject;
				DelayTicks = 3;
				return true;

			case EStage::VerifyLoadedRingReject:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Loaded ring rejection retained the Repository revision."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Loaded ring rejection retained the persistent revision."));
				TraceGesture(TEXT("RejectLoadedRing"));
				Stage = EStage::RejectLoadedBackpack;
				return true;

			case EStage::RejectLoadedBackpack:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragRoleItemToRole(FName(TEXT("Backpack")), ECodeBItemType::Backpack, FName(TEXT("Warehouse"))), TEXT("Physical Slate drag attempted to move a loaded real Profile backpack."));
				Stage = EStage::VerifyLoadedBackpackReject;
				DelayTicks = 3;
				return true;

			case EStage::VerifyLoadedBackpackReject:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Loaded backpack rejection retained the Repository revision."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Loaded backpack rejection retained the persistent revision."));
				TraceGesture(TEXT("RejectLoadedBackpack"));
				Stage = EStage::RejectRingInWeaponSlot;
				return true;

			case EStage::RejectRingInWeaponSlot:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DragRoleItemToRole(FName(TEXT("SpatialRing")), ECodeBItemType::SpatialItem, FName(TEXT("Weapon"))), TEXT("Physical Slate drag attempted an incompatible spatial-ring-to-weapon drop."));
				Stage = EStage::VerifyRejectedDrop;
				DelayTicks = 3;
				return true;

			case EStage::VerifyRejectedDrop:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Incompatible equipped target rejected without a Repository write."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Incompatible equipped target rejected without persistent revision drift."));
				TraceGesture(TEXT("RejectIncompatible"));
				Stage = EStage::DetailNoWrite;
				return true;

			case EStage::DetailNoWrite:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(ClickRoleItem(FName(TEXT("SpatialRing")), EKeys::LeftMouseButton), TEXT("Physical Slate click opened the real Profile item detail."));
				Stage = EStage::VerifyDetailNoWrite;
				DelayTicks = 2;
				return true;

			case EStage::VerifyDetailNoWrite:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Item detail interaction retained zero Repository writes."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Item detail interaction retained zero persistent writes."));
				TraceGesture(TEXT("DetailNoWrite"));
				Stage = EStage::DoubleClickNoWrite;
				return true;

			case EStage::DoubleClickNoWrite:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(DoubleClickRoleItem(FName(TEXT("SpatialRing"))), TEXT("Physical Slate double-click reached the real Profile item cell."));
				Stage = EStage::VerifyDoubleClickNoWrite;
				DelayTicks = 2;
				return true;

			case EStage::VerifyDoubleClickNoWrite:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Double-click retained zero Repository writes."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Double-click retained zero persistent writes."));
				TraceGesture(TEXT("DoubleClickNoWrite"));
				Stage = EStage::BeginEscapeCancellation;
				return true;

			case EStage::BeginEscapeCancellation:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(BeginRoleItemDragToRole(FName(TEXT("QuickSpatial")), ECodeBItemType::Material, FName(TEXT("Warehouse"))), TEXT("Physical Slate drag began from the real ring child before Escape cancellation."));
				Stage = EStage::SendEscapeCancellation;
				DelayTicks = 1;
				return true;

			case EStage::SendEscapeCancellation:
				Record(InjectEscape(), TEXT("Real Slate Escape reached the mounted Profile page during an active drag."));
				Stage = EStage::VerifyEscapeCancellation;
				DelayTicks = 2;
				return true;

			case EStage::VerifyEscapeCancellation:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Escape-cancelled drag retained zero Repository writes."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Escape-cancelled drag retained zero persistent writes."));
				Record(Host.IsValid() && Host->GetActiveWidget() && !Host->GetActiveWidget()->HasP4Preview(), TEXT("Escape cleared the real Profile drag preview without closing the page."));
				TraceGesture(TEXT("EscapeCancelNoWrite"));
				Stage = EStage::RightClickNoWrite;
				return true;

			case EStage::RightClickNoWrite:
				RevisionBeforeGesture = GetRepositoryRevision();
				PersistentBeforeGesture = Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE;
				Record(ClickRoleItem(FName(TEXT("SpatialRing")), EKeys::RightMouseButton), TEXT("Physical Slate right-click reached the real Profile item context route."));
				Stage = EStage::VerifyRightClickNoWrite;
				DelayTicks = 2;
				return true;

			case EStage::VerifyRightClickNoWrite:
				Record(GetRepositoryRevision() == RevisionBeforeGesture, TEXT("Right-click retained zero Repository writes."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Right-click retained zero persistent writes."));
				TraceGesture(TEXT("RightClickNoWrite"));
				Stage = EStage::CloseProfilePage;
				return true;

			case EStage::CloseProfilePage:
				Record(ClickMountedWidget(Host.IsValid() && Host->GetActiveWidget() ? Host->GetActiveWidget()->GetMountedCloseButton() : nullptr), TEXT("Physical Slate click closed the real Profile page."));
				Stage = EStage::ReopenNormalWarehouseEntry;
				DelayTicks = 4;
				return true;

			case EStage::ReopenNormalWarehouseEntry:
				Record(ClickVisibleWarehouseEntry(), TEXT("Physical Slate click reopened the same normal Sect warehouse entrance."));
				Stage = EStage::VerifyReopen;
				DelayTicks = 5;
				return true;

			case EStage::VerifyReopen:
			Host = FindProfileHostForManager();
				if (!Host.IsValid() || !Host->GetActiveWidget() || !Host->GetController() || !Host->GetController()->IsProfileBacked())
				{
					DelayTicks = 2;
					return true;
				}
				Record(IsEmptyProfile()
					? ProjectionHasNoItems()
					: (RoleContainsItemType(FName(TEXT("SpatialRing")), ECodeBItemType::SpatialItem)
						&& RoleContainsItemType(FName(TEXT("Backpack")), ECodeBItemType::Backpack)
						&& RoleContainsItemType(FName(TEXT("QuickSpatial")), ECodeBItemType::Material)
						&& RoleContainsItemType(FName(TEXT("PouchInternal")), ECodeBItemType::Material)),
					IsEmptyProfile()
						? TEXT("Close/reopen preserved the empty real Profile without creating an item.")
						: TEXT("Close/reopen restored the committed Profile spatial owners and child contents."));
				Record(Manager.IsValid() && Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() == PersistentBeforeGesture, TEXT("Close/reopen introduced no extra persistent revision."));
				CaptureP5Frame(TEXT("CloseReopen_PersistentProfile"));
				Stage = EStage::Complete;
				return true;

			case EStage::Complete:
				if (Host.IsValid())
				{
					Host->ClosePage();
				}
				return Finish();
			}
			return Finish();
		}

		Ademo_mapV3ProgressionManager* FindProgressionManager() const
		{
			if (!GEngine)
			{
				return nullptr;
			}
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (!World || !World->IsGameWorld())
				{
					continue;
				}
				for (TActorIterator<Ademo_mapV3ProgressionManager> It(World); It; ++It)
				{
					return *It;
				}
			}
			return nullptr;
		}

		UCodeBP3UIHostSubsystem* FindProfileHostForManager() const
		{
			UWorld* World = Manager.IsValid() ? Manager->GetWorld() : nullptr;
			UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			return GameInstance ? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
		}

		const FCodeBP2ContainerView* FindRole(const FName Role) const
		{
			const FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			return Controller ? Controller->GetProjection().Containers.FindByPredicate([Role](const FCodeBP2ContainerView& Candidate)
			{
				return Candidate.Role == Role;
			}) : nullptr;
		}

		bool IsEmptyProfile() const
		{
			return ProfileCase.Equals(TEXT("EmptyProfile"), ESearchCase::IgnoreCase);
		}

		bool IsRestartVerification() const
		{
			return ProfileCase.Equals(TEXT("RestartVerify"), ESearchCase::IgnoreCase);
		}

		FName CurrentEquipmentRole() const
		{
			switch (EquipmentRoleIndex)
			{
			case 0: return FName(TEXT("Weapon"));
			case 1: return FName(TEXT("Armor"));
			case 2: return FName(TEXT("Accessory0"));
			case 3: return FName(TEXT("SpatialRing"));
			default: return FName(TEXT("Backpack"));
			}
		}

		ECodeBItemType CurrentEquipmentType() const
		{
			switch (EquipmentRoleIndex)
			{
			case 0: return ECodeBItemType::Weapon;
			case 1: return ECodeBItemType::Armor;
			case 2: return ECodeBItemType::Accessory;
			case 3: return ECodeBItemType::SpatialItem;
			default: return ECodeBItemType::Backpack;
			}
		}

		bool ProjectionHasNoItems() const
		{
			const FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			if (!Controller)
			{
				return false;
			}
			for (const FCodeBP2ContainerView& Container : Controller->GetProjection().Containers)
			{
				if (Container.Slots.ContainsByPredicate([](const FCodeBP2SlotView& Slot) { return Slot.bOccupied; }))
				{
					return false;
				}
			}
			return true;
		}

		bool FindAddress(const FName Role, const ECodeBItemType ItemType, const bool bRequireOccupied, FCodeBP3SlotAddress& OutAddress) const
		{
			const FCodeBP2ContainerView* Container = FindRole(Role);
			FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			if (!Container || !Controller)
			{
				return false;
			}
			for (int32 SlotIndex = 0; SlotIndex < Container->Slots.Num(); ++SlotIndex)
			{
				const FCodeBP2SlotView& Slot = Container->Slots[SlotIndex];
				if (Slot.bOccupied != bRequireOccupied || (bRequireOccupied && Slot.ItemType != ItemType))
				{
					continue;
				}
				return Controller->MakeAddress(Container->ContainerId, SlotIndex, OutAddress);
			}
			return false;
		}

		bool RoleContainsItemType(const FName Role, const ECodeBItemType ItemType) const
		{
			const FCodeBP2ContainerView* Container = FindRole(Role);
			return Container && Container->Slots.ContainsByPredicate([ItemType](const FCodeBP2SlotView& Slot)
			{
				return Slot.bOccupied && Slot.ItemType == ItemType;
			});
		}

		bool ResolveMountedWidget(UWidget* Widget, FVector2D& OutPosition, TSharedPtr<FGenericWindow>& OutWindow) const
		{
			if (!Widget || !FSlateApplication::IsInitialized())
			{
				return false;
			}
			const FGeometry& Geometry = Widget->GetCachedGeometry();
			if (Geometry.GetLocalSize().X <= 1.0f || Geometry.GetLocalSize().Y <= 1.0f)
			{
				return false;
			}
			OutPosition = Geometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
			TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
			FSlateApplication& Slate = FSlateApplication::Get();
			const FWidgetPath HitPath = Slate.LocateWindowUnderMouse(OutPosition, Slate.GetInteractiveTopLevelWindows(), false);
			if (!HitPath.IsValid() || !HitPath.ContainsWidget(&SlateWidget.Get()))
			{
				return false;
			}
			OutWindow = HitPath.GetWindow()->GetNativeWindow();
			return OutWindow.IsValid();
		}

		FPointerEvent MakePointerEvent(const FVector2D& Position, const FVector2D& LastPosition, const FKey& Button) const
		{
			return FPointerEvent(FSlateApplication::CursorPointerIndex, Position, LastPosition,
				FSlateApplication::Get().GetPressedMouseButtons(), Button, 0.0f,
				FSlateApplication::Get().GetModifierKeys());
		}

		bool ClickMountedWidget(UWidget* Widget, const FKey Button = EKeys::LeftMouseButton) const
		{
			FVector2D Position;
			TSharedPtr<FGenericWindow> Window;
			if (!ResolveMountedWidget(Widget, Position, Window))
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			Slate.ProcessMouseMoveEvent(MakePointerEvent(Position, Position, EKeys::Invalid));
			const bool bDown = Slate.ProcessMouseButtonDownEvent(Window, MakePointerEvent(Position, Position, Button));
			const bool bUp = Slate.ProcessMouseButtonUpEvent(MakePointerEvent(Position, Position, Button));
			return bDown || bUp;
		}

		bool ClickVisibleWarehouseEntry() const
		{
			return Manager.IsValid() && Manager->GetSectNavigationWidget()
				&& ClickMountedWidget(Manager->GetSectNavigationWidget()->GetBuildingEntryButtonForAutomation(Edemo_mapSectPage::Warehouse));
		}

		bool ClickRoleItem(const FName Role, const FKey Button) const
		{
			FCodeBP3SlotAddress Address;
			const FCodeBP2ContainerView* Container = FindRole(Role);
			if (!Container || !Host.IsValid() || !Host->GetActiveWidget())
			{
				return false;
			}
			for (const FCodeBP2SlotView& Slot : Container->Slots)
			{
				if (Slot.bOccupied && Host->GetController()->MakeAddress(Container->ContainerId, Slot.SlotIndex, Address))
				{
					return ClickMountedWidget(Host->GetActiveWidget()->FindMountedCell(Address.ContainerId, Address.SlotIndex), Button);
				}
			}
			return false;
		}

		bool DoubleClickRoleItem(const FName Role) const
		{
			FCodeBP3SlotAddress Address;
			const FCodeBP2ContainerView* Container = FindRole(Role);
			if (!Container || !Host.IsValid() || !Host->GetActiveWidget())
			{
				return false;
			}
			for (const FCodeBP2SlotView& Slot : Container->Slots)
			{
				if (Slot.bOccupied && Host->GetController()->MakeAddress(Container->ContainerId, Slot.SlotIndex, Address))
				{
					return DoubleClickMountedWidget(Host->GetActiveWidget()->FindMountedCell(Address.ContainerId, Address.SlotIndex));
				}
			}
			return false;
		}

		bool ClickFirstEmptyRoleSlot(const FName Role) const
		{
			FCodeBP3SlotAddress Address;
			FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			return FindAddress(Role, ECodeBItemType::Generic, false, Address)
				&& Controller
				&& Host.IsValid()
				&& Host->GetActiveWidget()
				&& ClickMountedWidget(Host->GetActiveWidget()->FindMountedCell(Address.ContainerId, Address.SlotIndex));
		}

		bool DoubleClickMountedWidget(UWidget* Widget) const
		{
			FVector2D Position;
			TSharedPtr<FGenericWindow> Window;
			if (!ResolveMountedWidget(Widget, Position, Window))
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			Slate.ProcessMouseMoveEvent(MakePointerEvent(Position, Position, EKeys::Invalid));
			const bool bDoubleClick = Slate.ProcessMouseButtonDoubleClickEvent(Window, MakePointerEvent(Position, Position, EKeys::LeftMouseButton));
			const bool bUp = Slate.ProcessMouseButtonUpEvent(MakePointerEvent(Position, Position, EKeys::LeftMouseButton));
			return bDoubleClick || bUp;
		}

		bool DragFirstItemOfTypeToRole(const ECodeBItemType ItemType, const FName TargetRole) const
		{
			FCodeBP3SlotAddress Source;
			FCodeBP3SlotAddress Target;
			return FindAddress(FName(TEXT("Warehouse")), ItemType, true, Source)
				&& FindAddress(TargetRole, ECodeBItemType::Generic, false, Target)
				&& DragMountedCells(Source, Target);
		}

		bool DragRoleItemToRole(const FName SourceRole, const ECodeBItemType SourceType, const FName TargetRole) const
		{
			FCodeBP3SlotAddress Source;
			FCodeBP3SlotAddress Target;
			const FCodeBP2ContainerView* TargetContainer = FindRole(TargetRole);
			FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			// Rejection coverage deliberately drops onto the occupied weapon slot.
			// Selecting the address by role (instead of asking for an empty Generic
			// item) makes this a real incompatible Slate drop rather than a failed
			// test setup after the weapon equip gesture.
			return FindAddress(SourceRole, SourceType, true, Source)
				&& TargetContainer
				&& !TargetContainer->Slots.IsEmpty()
				&& Controller
				&& Controller->MakeAddress(TargetContainer->ContainerId, 0, Target)
				&& DragMountedCells(Source, Target);
		}

		bool DragRoleItemToEmptyRole(const FName SourceRole, const ECodeBItemType SourceType, const FName TargetRole) const
		{
			FCodeBP3SlotAddress Source;
			FCodeBP3SlotAddress Target;
			return FindAddress(SourceRole, SourceType, true, Source)
				&& FindAddress(TargetRole, ECodeBItemType::Generic, false, Target)
				&& DragMountedCells(Source, Target);
		}

		bool BeginRoleItemDragToRole(const FName SourceRole, const ECodeBItemType SourceType, const FName TargetRole) const
		{
			FCodeBP3SlotAddress Source;
			const FCodeBP2ContainerView* TargetContainer = FindRole(TargetRole);
			FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			FCodeBP3SlotAddress Target;
			return FindAddress(SourceRole, SourceType, true, Source)
				&& TargetContainer
				&& !TargetContainer->Slots.IsEmpty()
				&& Controller
				&& Controller->MakeAddress(TargetContainer->ContainerId, 0, Target)
				&& BeginMountedDrag(Source, Target);
		}

		bool DragTwoWarehouseItemsForSwap() const
		{
			const FCodeBP2ContainerView* Warehouse = FindRole(FName(TEXT("Warehouse")));
			FCodeBP3UIController* Controller = Host.IsValid() ? Host->GetController() : nullptr;
			if (!Warehouse || !Controller)
			{
				return false;
			}
			int32 FirstSlot = INDEX_NONE;
			int32 SecondSlot = INDEX_NONE;
			for (const FCodeBP2SlotView& Slot : Warehouse->Slots)
			{
				if (!Slot.bOccupied)
				{
					continue;
				}
				if (FirstSlot == INDEX_NONE)
				{
					FirstSlot = Slot.SlotIndex;
				}
				else
				{
					SecondSlot = Slot.SlotIndex;
					break;
				}
			}
			FCodeBP3SlotAddress First;
			FCodeBP3SlotAddress Second;
			return FirstSlot != INDEX_NONE
				&& SecondSlot != INDEX_NONE
				&& Controller->MakeAddress(Warehouse->ContainerId, FirstSlot, First)
				&& Controller->MakeAddress(Warehouse->ContainerId, SecondSlot, Second)
				&& DragMountedCells(First, Second);
		}


		bool DragMountedCells(const FCodeBP3SlotAddress& SourceAddress, const FCodeBP3SlotAddress& TargetAddress) const
		{
			UCodeBP3InventoryWidget* Page = Host.IsValid() ? Host->GetActiveWidget() : nullptr;
			UCodeBP3CellButton* Source = Page ? Page->FindMountedCell(SourceAddress.ContainerId, SourceAddress.SlotIndex) : nullptr;
			UCodeBP3CellButton* Target = Page ? Page->FindMountedCell(TargetAddress.ContainerId, TargetAddress.SlotIndex) : nullptr;
			FVector2D SourcePosition;
			FVector2D TargetPosition;
			TSharedPtr<FGenericWindow> SourceWindow;
			TSharedPtr<FGenericWindow> TargetWindow;
			if (!ResolveMountedWidget(Source, SourcePosition, SourceWindow) || !ResolveMountedWidget(Target, TargetPosition, TargetWindow))
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			Slate.ProcessMouseMoveEvent(MakePointerEvent(SourcePosition, SourcePosition, EKeys::Invalid));
			const bool bDown = Slate.ProcessMouseButtonDownEvent(SourceWindow, MakePointerEvent(SourcePosition, SourcePosition, EKeys::LeftMouseButton));
			const FVector2D Delta = TargetPosition - SourcePosition;
			const FVector2D Threshold = SourcePosition + (Delta.IsNearlyZero() ? FVector2D(24.0f, 0.0f) : Delta.GetSafeNormal() * 24.0f);
			Slate.ProcessMouseMoveEvent(MakePointerEvent(Threshold, SourcePosition, EKeys::Invalid));
			Slate.ProcessMouseMoveEvent(MakePointerEvent(TargetPosition, Threshold, EKeys::Invalid));
			const bool bUp = Slate.ProcessMouseButtonUpEvent(MakePointerEvent(TargetPosition, TargetPosition, EKeys::LeftMouseButton));
			return bDown && bUp;
		}

		bool BeginMountedDrag(const FCodeBP3SlotAddress& SourceAddress, const FCodeBP3SlotAddress& TargetAddress) const
		{
			UCodeBP3InventoryWidget* Page = Host.IsValid() ? Host->GetActiveWidget() : nullptr;
			UCodeBP3CellButton* Source = Page ? Page->FindMountedCell(SourceAddress.ContainerId, SourceAddress.SlotIndex) : nullptr;
			UCodeBP3CellButton* Target = Page ? Page->FindMountedCell(TargetAddress.ContainerId, TargetAddress.SlotIndex) : nullptr;
			FVector2D SourcePosition;
			FVector2D TargetPosition;
			TSharedPtr<FGenericWindow> SourceWindow;
			TSharedPtr<FGenericWindow> TargetWindow;
			if (!ResolveMountedWidget(Source, SourcePosition, SourceWindow) || !ResolveMountedWidget(Target, TargetPosition, TargetWindow))
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			Slate.ProcessMouseMoveEvent(MakePointerEvent(SourcePosition, SourcePosition, EKeys::Invalid));
			const bool bDown = Slate.ProcessMouseButtonDownEvent(SourceWindow, MakePointerEvent(SourcePosition, SourcePosition, EKeys::LeftMouseButton));
			const FVector2D Delta = TargetPosition - SourcePosition;
			const FVector2D Threshold = SourcePosition + (Delta.IsNearlyZero() ? FVector2D(24.0f, 0.0f) : Delta.GetSafeNormal() * 24.0f);
			Slate.ProcessMouseMoveEvent(MakePointerEvent(Threshold, SourcePosition, EKeys::Invalid));
			Slate.ProcessMouseMoveEvent(MakePointerEvent(TargetPosition, Threshold, EKeys::Invalid));
			return bDown;
		}

		bool InjectEscape() const
		{
			if (!FSlateApplication::IsInitialized())
			{
				return false;
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			if (Host.IsValid() && Host->GetActiveWidget())
			{
				Host->GetActiveWidget()->SetKeyboardFocus();
			}
			return Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Escape, Slate.GetModifierKeys(), 0, false, 0, 0));
		}

		int32 GetRepositoryRevision() const
		{
			return Host.IsValid() && Host->GetController() ? Host->GetController()->GetProjection().Revision : INDEX_NONE;
		}

		void TraceGesture(const TCHAR* Gesture) const
		{
			const UCodeBP3InventoryWidget* Page = Host.IsValid() ? Host->GetActiveWidget() : nullptr;
			UE_LOG(LogTemp, Display, TEXT("P5.RealProfileTrace OwnerId=%s HandoffState=%d Gesture=%s DragOperation=%d Preview=%s P2CommandCount=%d P2CallCount=%d RepositoryRevisionBefore=%d RepositoryRevisionAfter=%d PersistentRevisionBefore=%d PersistentRevisionAfter=%d"),
				Manager.IsValid() ? *Manager->GetCodeBOutOfRaidOwnerIdForAutomation().ToString(EGuidFormats::DigitsWithHyphens) : TEXT("Invalid"),
				Manager.IsValid() ? Manager->GetCodeBOutOfRaidHandoffStateForAutomation() : INDEX_NONE,
				Gesture,
				Page && Page->GetP4CommandCount() + Page->GetP4CallCount() > 0 ? 1 : 0,
				Page && Page->HasP4Preview() ? TEXT("Visible") : TEXT("Cleared"),
				Page ? Page->GetP4CommandCount() : INDEX_NONE,
				Page ? Page->GetP4CallCount() : INDEX_NONE,
				RevisionBeforeGesture, GetRepositoryRevision(), PersistentBeforeGesture,
				Manager.IsValid() ? Manager->GetCodeBOutOfRaidPersistentRevisionForAutomation() : INDEX_NONE);
		}

		void Record(const bool bCondition, const TCHAR* Message)
		{
			if (bCondition)
			{
				UE_LOG(LogTemp, Display, TEXT("P5.RealProfileTrace PASS %s"), Message);
			}
			else
			{
				bSucceeded = false;
				UE_LOG(LogTemp, Error, TEXT("P5.RealProfileTrace FAIL %s"), Message);
			}
		}

		bool Finish()
		{
			const TCHAR* Verdict = bSucceeded ? TEXT("PASS") : TEXT("FAIL");
			if (bSucceeded)
			{
				UE_LOG(LogTemp, Display, TEXT("P5.RealProfileTrace %s Case=%s Resolution=%dx%d"), Verdict, *ProfileCase, GSystemResolution.ResX, GSystemResolution.ResY);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("P5.RealProfileTrace %s Case=%s Resolution=%dx%d"), Verdict, *ProfileCase, GSystemResolution.ResX, GSystemResolution.ResY);
			}
			TSharedPtr<FP5RealProfileTraceHarness> KeepAlive = AsShared();
			ActiveHarness.Reset();
			if (bExitWhenFinished)
			{
				FPlatformMisc::RequestExitWithStatus(bSucceeded ? 0 : 1, false);
			}
			return false;
		}

		FString ProfileCase;
		int32 ExpectedWidth = 0;
		int32 ExpectedHeight = 0;
		bool bExitWhenFinished = false;
		bool bSucceeded = true;
		int32 TickCount = 0;
		int32 DelayTicks = 0;
		int32 InitialPersistentRevision = INDEX_NONE;
		int32 RevisionBeforeGesture = INDEX_NONE;
		int32 PersistentBeforeGesture = INDEX_NONE;
		int32 EquipmentRoleIndex = 0;
		bool bWeaponWasAlreadyEquipped = false;
		bool bRingWasAlreadyEquipped = false;
		EStage Stage = EStage::WaitForSect;
		TWeakObjectPtr<Ademo_mapV3ProgressionManager> Manager;
		TWeakObjectPtr<UCodeBP3UIHostSubsystem> Host;
		FTSTicker::FDelegateHandle TickerHandle;

	public:
		static TSharedPtr<FP5RealProfileTraceHarness> ActiveHarness;
	};

	TSharedPtr<FP5RealProfileTraceHarness> FP5RealProfileTraceHarness::ActiveHarness;

	void RunP5RealProfileTrace(const TArray<FString>& Args)
	{
		if (FP5RealProfileTraceHarness::ActiveHarness.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("P5.RealProfileTrace is already running."));
			return;
		}
		const FString ProfileCase = Args.Num() > 0 ? Args[0] : TEXT("SelectedProfile");
		const int32 ExpectedWidth = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		const int32 ExpectedHeight = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 0;
		FP5RealProfileTraceHarness::ActiveHarness = MakeShared<FP5RealProfileTraceHarness>(
			ProfileCase, ExpectedWidth, ExpectedHeight, Args.Contains(TEXT("Quit")));
		FP5RealProfileTraceHarness::ActiveHarness->Start();
	}

	FAutoConsoleCommand OpenP3HostCommand(
		TEXT("CodeB.P3.Open"),
		TEXT("Open the default-off Code B P3 out-of-raid stash UI host."),
		FConsoleCommandDelegate::CreateStatic(&OpenP3Host));
	FAutoConsoleCommand CloseP3HostCommand(
		TEXT("CodeB.P3.Close"),
		TEXT("Close the Code B P3 UI page and preserve its isolated in-memory fixture."),
		FConsoleCommandDelegate::CreateStatic(&CloseP3Host));
	FAutoConsoleCommand ResetP3HostCommand(
		TEXT("CodeB.P3.Reset"),
		TEXT("Explicitly rebuild the isolated Code B P3 development fixture."),
		FConsoleCommandDelegate::CreateStatic(&ResetP3Host));
	FAutoConsoleCommand CaptureP3InitialCommand(TEXT("CodeB.P3.CaptureInitial"), TEXT("Capture the initial P3 Host page with Slate UI."), FConsoleCommandDelegate::CreateStatic(&CaptureP3Initial));
	FAutoConsoleCommand CaptureP3SelectedCommand(TEXT("CodeB.P3.CaptureSelected"), TEXT("Capture selected item and detail state."), FConsoleCommandDelegate::CreateStatic(&CaptureP3Selected));
	FAutoConsoleCommand CaptureP4InitialCommand(TEXT("CodeB.P4.CaptureInitial"), TEXT("Capture the initial P4 development Host page."), FConsoleCommandDelegate::CreateStatic(&CaptureP4Initial));
	FAutoConsoleCommand CaptureP4HighlightCommand(TEXT("CodeB.P4.CaptureHighlight"), TEXT("Capture P4's legal drag target highlight."), FConsoleCommandDelegate::CreateStatic(&CaptureP4DragHighlight));
	FAutoConsoleCommand RunP4RealInputTraceCommand(
		TEXT("CodeB.P4.RunRealInputTrace"),
		TEXT("Run real Slate hit-test input on the mounted P3/P4 page. Args: [expected width] [expected height] [Quit]."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RunP4RealInputTrace));
	FAutoConsoleCommand RunP4xRealInputTraceCommand(
		TEXT("CodeB.P4x.RunRealInputTrace"),
		TEXT("Run P4x r2 real Slate hit-test input and capture current P4x evidence. Args: [expected width] [expected height] [Quit]."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RunP4xRealInputTrace));
	FAutoConsoleCommand RunP5RealProfileTraceCommand(
		TEXT("CodeB.P5.RunRealProfileTrace"),
		TEXT("Run the normal Sect-entry, real Profile Slate trace. Args: [ProfileCase] [expected width] [expected height] [Quit]."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RunP5RealProfileTrace));
#endif
}

void UCodeBP4DragOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);
	if (OwnerWidget.IsValid())
	{
		// Slate invokes the operation itself even when Escape consumes the key before
		// the page sees NativeOnKeyDown. This is the durable cancellation owner for the
		// mounted production drag and only clears non-authoritative UI state.
		OwnerWidget->HandleP4DragCancelled(this);
	}
}

void UCodeBP3CellButton::Configure(UCodeBP3InventoryWidget* InOwnerWidget, const FCodeBP3SlotAddress& InAddress)
{
	OwnerWidget = InOwnerWidget;
	Address = InAddress;
	BuildButton();
	// UUserWidget defaults to SelfHitTestInvisible. With the visual UButton also
	// non-interactive, the previous cell had no Slate hit-test target. The cell is
	// now the only pointer owner; its child remains a visual surface only.
	SetVisibility(ESlateVisibility::Visible);
	if (InnerButton)
	{
		// The cell owns pointer routing so it can distinguish a native drag from a
		// click and receive Slate's double-click callback.  The UButton stays as the
		// visual surface but deliberately does not consume pointer input itself.
		InnerButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UCodeBP3CellButton::BuildButton()
{
	if (!InnerButton && WidgetTree)
	{
		InnerButton = WidgetTree->ConstructWidget<UButton>();
		WidgetTree->RootWidget = InnerButton;
	}
}

void UCodeBP3CellButton::SetCellContent(UWidget* InContent)
{
	BuildButton();
	if (InnerButton)
	{
		InnerButton->SetContent(InContent);
	}
}

void UCodeBP3CellButton::SetCellColor(const FLinearColor& InColor)
{
	BuildButton();
	if (InnerButton)
	{
		InnerButton->SetBackgroundColor(InColor);
	}
}

FReply UCodeBP3CellButton::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	bConsumedQuickTransfer = false;
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& InMouseEvent.IsControlDown() && Address.IsRevealed() && OwnerWidget.IsValid())
	{
		bConsumedQuickTransfer = true;
		OwnerWidget->BeginP4PointerGesture(Address);
		OwnerWidget->HandleQuickTransfer(this);
		return FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& Address.IsValid()
		&& (Address.CellState == ECodeBP3CellState::Empty || Address.IsRevealed())
		&& OwnerWidget.IsValid())
	{
		OwnerWidget->BeginP4PointerGesture(Address);
		// Empty child cells can become P23's explicit quick-transfer destination,
		// but only a revealed root may register a drag threshold.
		return Address.IsRevealed()
			? FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton)
			: FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && Address.IsRevealed() && OwnerWidget.IsValid())
	{
		OwnerWidget->BeginP4PointerGesture(Address);
		OwnerWidget->TraceP4Input(TEXT("RightMouseDown"), Address);
		OwnerWidget->HandleP4ContextMenu(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCodeBP3CellButton::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OwnerWidget.IsValid())
	{
		if (bConsumedQuickTransfer)
		{
			bConsumedQuickTransfer = false;
			return FReply::Handled();
		}
		OwnerWidget->TraceP4Input(TEXT("LeftMouseUp"), Address);
		// P4x deliberately leaves a click as read-only selection. Position changes are
		// committed exclusively by a later real DragOperation Drop on an explicit target.
		OwnerWidget->HandleCellActivated(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCodeBP3CellButton::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleCellHover(Address, true);
	}
}

void UCodeBP3CellButton::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleCellHover(Address, false);
	}
}

FReply UCodeBP3CellButton::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Address.IsRevealed() && OwnerWidget.IsValid())
	{
		OwnerWidget->TraceP4Input(TEXT("NativeDoubleClick"), Address);
		// Double-click is selection-only; the retired QuickMove command is no longer
		// reachable from any mounted P4 input path.
		OwnerWidget->HandleCellActivated(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UCodeBP3CellButton::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InPointerEvent, UDragDropOperation*& OutOperation)
{
	if (!OwnerWidget.IsValid())
	{
		return;
	}
	FCodeBP4DragPayload Payload;
	if (!OwnerWidget->BeginP4Drag(this, Payload))
	{
		return;
	}
	UCodeBP4DragOperation* Operation = NewObject<UCodeBP4DragOperation>(this);
	Operation->Configure(Payload, OwnerWidget.Get());
	UTextBlock* DragVisual = NewObject<UTextBlock>(Operation);
	const FString DragLabel = Payload.bSplitIntent
		? (Payload.QuantityDraftKind == ECodeBP3QuantityDraftKind::WorldPickup
			? FString::Printf(TEXT("地面数量拾回：%s  x%d"),
				*Payload.DefinitionId.ToString(), Payload.RequestedMergeQuantity)
			: FString::Printf(TEXT("数量拖拽：%s  x%d"),
				*Payload.DefinitionId.ToString(), Payload.RequestedMergeQuantity))
		: FString::Printf(TEXT("拖拽：%s  x%d"), *Payload.DefinitionId.ToString(), Payload.Quantity);
	DragVisual->SetText(FText::FromString(DragLabel));
	DragVisual->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
	DragVisual->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.95f, 1.0f)));
	Operation->DefaultDragVisual = DragVisual;
	Operation->Pivot = EDragPivot::MouseDown;
	OutOperation = Operation;
	OwnerWidget->TraceP4Input(TEXT("DragOperationCreated"), Address, FString::Printf(TEXT("Session=%u ExpectedRevision=%d"), Payload.SessionId, Payload.ExpectedRevision));
}

void UCodeBP3CellButton::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleP4DragEnter(this, Cast<UCodeBP4DragOperation>(InOperation));
	}
}

void UCodeBP3CellButton::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleP4DragLeave(this, Cast<UCodeBP4DragOperation>(InOperation));
	}
}

bool UCodeBP3CellButton::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (OwnerWidget.IsValid() && OwnerWidget->HandleP4Drop(this, Cast<UCodeBP4DragOperation>(InOperation)))
	{
		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UCodeBP3GroundDropZone::Configure(UCodeBP3InventoryWidget* InOwnerWidget)
{
	OwnerWidget = InOwnerWidget;
	BuildZone();
}

void UCodeBP3GroundDropZone::BuildZone()
{
	if (!InnerBorder && WidgetTree)
	{
		InnerBorder = WidgetTree->ConstructWidget<UBorder>();
		InnerBorder->SetPadding(FMargin(12.0f));
		InnerBorder->SetBrushColor(FLinearColor(0.30f, 0.12f, 0.08f, 1.0f));
		UTextBlock* Label = MakeText(
			WidgetTree,
			TEXT("丢到地面 · 可拖整件基础物品，或两种正式空间父项的完整图"),
			14,
			FLinearColor(1.0f, 0.74f, 0.48f));
		Label->SetJustification(ETextJustify::Center);
		InnerBorder->SetContent(Label);
		WidgetTree->RootWidget = InnerBorder;
	}
}

bool UCodeBP3GroundDropZone::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (UCodeBP4DragOperation* Operation = Cast<UCodeBP4DragOperation>(InOperation);
		Operation && OwnerWidget.IsValid())
	{
		return OwnerWidget->HandleGroundDropZoneDrop(Operation);
	}
	return false;
}

UCodeBP3InventoryWidget::UCodeBP3InventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UCodeBP3InventoryWidget::InitializeForHost(UCodeBP3UIHostSubsystem* InHost)
{
	Host = InHost;
	BuildLayout();
	RefreshFromController();
}

void UCodeBP3InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	RefreshFromController();
	SetKeyboardFocus();
}

void UCodeBP3InventoryWidget::NativeDestruct()
{
	TraceP4Input(TEXT("PageDestruct"), FCodeBP3SlotAddress(), TEXT("P4 temporary state and pointer callbacks are being released"));
	if (P4Controller.IsValid())
	{
		P4Controller->CancelInteraction(TEXT("页面关闭，已清理拖拽临时状态"));
		P4Controller.Reset();
	}
	ContextMenuAddress.Reset();
	P4PreviewAddress.Reset();
	P4Preview.Reset();
	HoveredAddress.Reset();
	bSplitQuantityInputOpen = false;
	SplitInputItemId.Invalidate();
	ContextMenuRevision = INDEX_NONE;
	if (Host.IsValid())
	{
		Host->ClearWorkspaceTransientState();
	}
	Super::NativeDestruct();
}

FReply UCodeBP3InventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey NumberKeys[] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	int32 NumberSlot = INDEX_NONE;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
	{
		if (InKeyEvent.GetKey() == NumberKeys[Index])
		{
			NumberSlot = Index + 1;
			break;
		}
	}
	if (NumberSlot != INDEX_NONE)
	{
		// UI focus owns all number keys. Shift is the P13 binding chord; plain
		// numbers are consumed here so they can never fall through to P15 Use.
		if (InKeyEvent.IsShiftDown() && !InKeyEvent.IsRepeat()
			&& Host.IsValid() && Host->GetController())
		{
			const TOptional<FCodeBP3SlotAddress>& Selected = Host->GetController()->GetSelectedAddress();
			const FCodeBP3SlotAddress* Candidate = HoveredAddress.IsSet() && HoveredAddress->IsRevealed()
				? &HoveredAddress.GetValue()
				: (Selected.IsSet() ? &Selected.GetValue() : nullptr);
			FString Error;
			const bool bBound = Candidate
				&& Host->RequestHotbarBindFromAddress(*Candidate, NumberSlot, Error);
			Host->GetController()->SetP4Feedback(bBound
				? FString::Printf(TEXT("Shift+%d 已通过 P13 绑定；未触发 P15 使用。"), NumberSlot)
				: (Error.IsEmpty() ? TEXT("没有可绑定的 BaseQuick QuickUsable 物品。") : Error));
			RefreshFromController();
		}
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::I
		&& Host.IsValid() && Host->GetController() && Host->GetController()->IsActiveRunBacked())
	{
		// P7's existing inventory key only owns the Code B host visibility while
		// this UI-only page has focus. It cannot issue an inventory transaction.
		Host->ClosePage();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		TraceP4Input(TEXT("EscapeCancel"), FCodeBP3SlotAddress(), TEXT("Esc routed through the mounted page"));
		if (P4Controller.IsValid())
		{
			P4Controller->CancelInteraction(TEXT("已按 Esc 取消拖拽／临时预览，未写入任何物品状态"));
		}
		ContextMenuAddress.Reset();
		ContextMenuRevision = INDEX_NONE;
		if (bSplitQuantityInputOpen)
		{
			bSplitQuantityInputOpen = false;
			SplitInputItemId.Invalidate();
			if (Host.IsValid() && Host->GetController())
			{
				Host->GetController()->SetP4Feedback(TEXT("拆分数量输入已取消，未写入物品状态"));
			}
			RefreshFromController();
		}
		else if (Host.IsValid() && Host->CancelSplitDraft())
		{
			RefreshFromController();
		}
		else if (Host.IsValid() && Host->GetController() && Host->GetController()->GetOperationMode() != ECodeBP3OperationMode::None)
		{
			Host->GetController()->CancelOperation(TEXT("已取消当前操作"));
			RefreshFromController();
		}
		else if (Host.IsValid())
		{
			Host->ClosePage();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCodeBP3InventoryWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	// Child controls take focus during normal quantity entry.  Only an actual
	// page/window blur clears the transient intent; internal mouse focus does not.
	if (InFocusEvent.GetCause() == EFocusCause::WindowActivate
		|| InFocusEvent.GetCause() == EFocusCause::Cleared)
	{
		bSplitQuantityInputOpen = false;
		SplitInputItemId.Invalidate();
		if (Host.IsValid())
		{
			Host->CancelSplitDraft(TEXT("页面失去焦点，拆分草稿已清除且未写入"));
			Host->ClearWorkspaceTransientState();
		}
	}
	Super::NativeOnFocusLost(InFocusEvent);
}

void UCodeBP3InventoryWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	if (UCodeBP4DragOperation* Operation = Cast<UCodeBP4DragOperation>(InOperation))
	{
		HandleP4DragCancelled(Operation);
	}
}

void UCodeBP3InventoryWidget::BuildLayout()
{
	if (bLayoutBuilt || !WidgetTree)
	{
		return;
	}

	UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>();
	ScaleBox->SetStretch(EStretch::ScaleToFit);
	ScaleBox->SetStretchDirection(EStretchDirection::DownOnly);
	USizeBox* PageSize = WidgetTree->ConstructWidget<USizeBox>();
	PageSize->SetWidthOverride(PageWidth);
	PageSize->SetHeightOverride(960.0f);
	ScaleBox->SetContent(PageSize);

	PageContents = WidgetTree->ConstructWidget<UVerticalBox>();
	PageSize->SetContent(PageContents);
	WidgetTree->RootWidget = ScaleBox;
	bLayoutBuilt = true;
}

UVerticalBox* UCodeBP3InventoryWidget::AddPanel(UVerticalBox* Parent, const FString& Title)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(PanelColor);
	Border->SetPadding(FMargin(12.0f));
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Border->SetContent(Content);
	UVerticalBoxSlot* BorderSlot = Parent->AddChildToVerticalBox(Border);
	BorderSlot->SetPadding(FMargin(4.0f));
	BorderSlot->SetHorizontalAlignment(HAlign_Fill);
	Content->AddChildToVerticalBox(MakeText(WidgetTree, Title, 20, FLinearColor(0.65f, 0.86f, 1.0f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 8.0f));
	return Content;
}

const FCodeBP2ContainerView* UCodeBP3InventoryWidget::FindRole(const FName Role) const
{
	if (!Host.IsValid() || !Host->GetController())
	{
		return nullptr;
	}
	return Host->GetController()->GetProjection().Containers.FindByPredicate([Role](const FCodeBP2ContainerView& Candidate)
	{
		return Candidate.Role == Role;
	});
}

FCodeBP4InteractionController* UCodeBP3InventoryWidget::GetP4Controller()
{
	if (!P4Controller.IsValid() && Host.IsValid() && Host->GetController())
	{
		P4Controller = MakeUnique<FCodeBP4InteractionController>(*Host->GetController());
	}
	return P4Controller.Get();
}

const FCodeBP2SlotView* UCodeBP3InventoryWidget::FindSlot(const FCodeBP3SlotAddress& Address) const
{
	if (!Host.IsValid() || !Host->GetController()) return nullptr;
	const FCodeBP2ContainerView* Container = Host->GetController()->GetProjection().Containers.FindByPredicate(
		[&Address](const FCodeBP2ContainerView& Candidate) { return Candidate.ContainerId == Address.ContainerId; });
	return Container ? Container->Slots.FindByPredicate(
		[&Address](const FCodeBP2SlotView& Candidate) { return Candidate.SlotIndex == Address.SlotIndex; }) : nullptr;
}

const FCodeBP2SlotView* UCodeBP3InventoryWidget::FindSpatialParent(const FCodeBP2ContainerView& ChildContainer) const
{
	if (!Host.IsValid() || !Host->GetController()) return nullptr;
	for (const FCodeBP2ContainerView& Container : Host->GetController()->GetProjection().Containers)
	{
		if (const FCodeBP2SlotView* Parent = Container.Slots.FindByPredicate(
			[&ChildContainer](const FCodeBP2SlotView& CellSlot)
			{
				return CellSlot.bOccupied && CellSlot.ChildContainerId == ChildContainer.ContainerId;
			}))
		{
			return Parent;
		}
	}
	return nullptr;
}

FCodeBP4DropPreview UCodeBP3InventoryWidget::PreviewInventoryTransfer(
	const FCodeBP4DragPayload& Payload,
	const FCodeBP3SlotAddress& Target) const
{
	FCodeBP4DropPreview Rejected;
	if (!Host.IsValid() || !Host->GetController())
	{
		Rejected.Message = TEXT("物品工作台已关闭。");
		return Rejected;
	}
	FString ContextError;
	if (!Host->ValidateTransferContext(Payload, ContextError))
	{
		Rejected.Message = ContextError;
		return Rejected;
	}
	if (Host->IsNormalContainerSlotProtected(Target.ContainerId, Target.SlotIndex)
		|| Host->IsBodyContainerSlotProtected(Target.ContainerId, Target.SlotIndex))
	{
		Rejected.Message = TEXT("Hidden／Searching 格只能接收搜索点击，不能接收移动。");
		return Rejected;
	}
	const bool bSourceExternal = Host->IsExternalTargetContainer(Payload.Source.ContainerId);
	const bool bTargetExternal = Host->IsExternalTargetContainer(Target.ContainerId);
	if (bSourceExternal && bTargetExternal)
	{
		Rejected.Message = TEXT("外部目标面板不提供内部整理；请在玩家与目标之间移动。");
		return Rejected;
	}
	if (Host->IsBodyEquipmentContainerPresentation(Target.ContainerId))
	{
		Rejected.Message = TEXT("尸体固定装备位不是玩家物品写入目标。");
		return Rejected;
	}
	const bool bSourceWorldDrop = Host->IsWorldDropPresentation(Payload.Source.ContainerId);
	const bool bTargetWorldDrop = Host->IsWorldDropPresentation(Target.ContainerId);
	if ((bSourceWorldDrop || bTargetWorldDrop)
		&& !Host->ValidateWorldDropTransferContext(Payload, Target, ContextError))
	{
		Rejected.Message = ContextError;
		return Rejected;
	}
	FCodeBP4InteractionController* Interaction = const_cast<UCodeBP3InventoryWidget*>(this)->GetP4Controller();
	if (!Interaction)
	{
		Rejected.Message = TEXT("统一移动路由不可用。");
		return Rejected;
	}
	FCodeBP4DropPreview Preview = Interaction->PreviewDrop(Payload, Target);
	if (bTargetWorldDrop)
	{
		const FCodeBP2Projection& Projection = Host->GetController()->GetProjection();
		const FCodeBP2ContainerView* SourceContainer = Projection.Containers.FindByPredicate(
			[&Payload](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Payload.Source.ContainerId; });
		const FCodeBP2SlotView* SourceSlot = SourceContainer ? SourceContainer->Slots.FindByPredicate(
			[&Payload](const FCodeBP2SlotView& Value) { return Value.SlotIndex == Payload.Source.SlotIndex; }) : nullptr;
		const FCodeBP2ContainerView* TargetContainer = Projection.Containers.FindByPredicate(
			[&Target](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Target.ContainerId; });
		const FCodeBP2SlotView* TargetSlot = TargetContainer ? TargetContainer->Slots.FindByPredicate(
			[&Target](const FCodeBP2SlotView& Value) { return Value.SlotIndex == Target.SlotIndex; }) : nullptr;
		const bool bExactP29Merge = Payload.bQuickTransferIntent && !Payload.bSplitIntent
			&& Host->IsP29PlayerQuickTransferSourceContainer(Payload.Source.ContainerId)
			&& SourceSlot && TargetSlot && SourceSlot->bOccupied && TargetSlot->bOccupied
			&& SourceSlot->bStackable && TargetSlot->bStackable
			&& SourceSlot->MaxStack > 1 && TargetSlot->MaxStack == SourceSlot->MaxStack
			&& SourceSlot->Quantity > 0 && TargetSlot->Quantity > 0
			&& TargetSlot->Quantity < TargetSlot->MaxStack
			&& !SourceSlot->ChildContainerId.IsValid() && !TargetSlot->ChildContainerId.IsValid()
			&& SourceSlot->DefinitionId == TargetSlot->DefinitionId
			&& Preview.bAllowed && Preview.Kind == ECodeBP4DropKind::Merge
			&& Preview.Operation == ECodeBOperation::Merge && Preview.Quantity == 0;
		if (!bExactP29Merge)
		{
			Rejected.Message = TEXT("地面目标仅接受 P29 Ctrl 快转从 BaseQuick／当前 active child 执行一次兼容 Merge(0)。");
			return Rejected;
		}
		return Preview;
	}
	if (bSourceWorldDrop
		&& Preview.bAllowed)
	{
		const FCodeBP2Projection& Projection = Host->GetController()->GetProjection();
		const FCodeBP2ContainerView* SourceContainer = Projection.Containers.FindByPredicate(
			[&Payload](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Payload.Source.ContainerId; });
		const FCodeBP2SlotView* SourceSlot = SourceContainer ? SourceContainer->Slots.FindByPredicate(
			[&Payload](const FCodeBP2SlotView& Value) { return Value.SlotIndex == Payload.Source.SlotIndex; }) : nullptr;
		const FCodeBP2ContainerView* TargetContainer = Projection.Containers.FindByPredicate(
			[&Target](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Target.ContainerId; });
		const bool bSimpleStack = SourceSlot && !SourceSlot->ChildContainerId.IsValid()
			&& SourceSlot->bStackable && SourceSlot->MaxStack > 1;
		const bool bSimpleRoot = SourceSlot && !SourceSlot->ChildContainerId.IsValid();
		const bool bP32StandardEquipmentRoot = SourceSlot
			&& !SourceSlot->ChildContainerId.IsValid()
			&& !SourceSlot->bStackable && SourceSlot->MaxStack == 1 && SourceSlot->Quantity == 1
			&& ((SourceSlot->ItemType == ECodeBItemType::Weapon
					&& SourceSlot->EquipSlot == ECodeBEquipSlot::Weapon)
				|| (SourceSlot->ItemType == ECodeBItemType::Armor
					&& SourceSlot->EquipSlot == ECodeBEquipSlot::Armor)
				|| (SourceSlot->ItemType == ECodeBItemType::Accessory
					&& SourceSlot->EquipSlot == ECodeBEquipSlot::Accessory));
		const bool bP26PlayerStorage = TargetContainer
			&& (TargetContainer->Role == FName(TEXT("Basic6"))
				|| TargetContainer->Role == FName(TEXT("QuickSpatial"))
				|| TargetContainer->Role == FName(TEXT("PouchInternal")));
		const bool bP29QuickDestination = !Payload.bQuickTransferIntent
			|| Host->IsP29PlayerQuickTransferSourceContainer(Target.ContainerId);
		const bool bP30CompleteGraphQuickTransfer = Payload.bQuickTransferIntent
			&& SourceSlot && SourceSlot->ChildContainerId.IsValid()
			&& (SourceSlot->DefinitionId == Fdemo_mapItemIds::WindTalisman
				|| SourceSlot->DefinitionId == Fdemo_mapItemIds::BackpackLevel1);
		const bool bP34StandardEquipmentQuickTransfer = Payload.bQuickTransferIntent
			&& bP32StandardEquipmentRoot
			&& Host->IsP34StandardEquipmentWorldDropSource(Payload.Source.ContainerId);
		const bool bP35ChildStandardRecord = !Payload.bQuickTransferIntent
			&& bP32StandardEquipmentRoot
			&& Host->IsP35ChildStandardEquipmentWorldDropSource(Payload.Source.ContainerId);
		if (bP30CompleteGraphQuickTransfer)
		{
			if (!TargetContainer || TargetContainer->ContainerId != Projection.BasicContainerId
				|| TargetContainer->Role != FName(TEXT("Basic6"))
				|| Target.bOccupied || Preview.Operation != ECodeBOperation::Move
				|| Preview.Quantity != 0)
			{
				Rejected.Message = TEXT("P30 完整空间图 Ctrl 快转只接受明确 BaseQuick 空格的一次 whole-graph Move。");
				return Rejected;
			}
			return Preview;
		}
		if (bP34StandardEquipmentQuickTransfer)
		{
			if (!TargetContainer || TargetContainer->ContainerId != Projection.BasicContainerId
				|| TargetContainer->Role != FName(TEXT("Basic6"))
				|| Target.bOccupied || Preview.Operation != ECodeBOperation::Move
				|| Preview.Quantity != 1)
			{
				Rejected.Message = TEXT("P34 标准装备 Ctrl 快转只接受首个空 BaseQuick 格的一次 whole-root Move(1)。");
				return Rejected;
			}
			return Preview;
		}
		const bool bP27WorldPickup = Payload.bSplitIntent
			&& Payload.QuantityDraftKind == ECodeBP3QuantityDraftKind::WorldPickup;
		if (bP27WorldPickup)
		{
			const bool bP27EmptySplit = !Target.bOccupied
				&& Preview.Operation == ECodeBOperation::Split
				&& Preview.Quantity == Payload.RequestedMergeQuantity;
			const bool bP28OccupiedMerge = Target.bOccupied
				&& Preview.Operation == ECodeBOperation::Merge
				&& Preview.Quantity == Payload.RequestedMergeQuantity
				&& Preview.ProjectedAcceptedQuantity == Payload.RequestedMergeQuantity
				&& !Preview.bPartialAcceptance;
			if (!bSimpleStack || !bP26PlayerStorage || !bP29QuickDestination
					|| (!bP27EmptySplit && !bP28OccupiedMerge))
			{
				Rejected.Message = TEXT("地面数量拾回只接受 simple stack 到明确空格 Split(N) 或兼容未满堆叠 Merge(N)。");
				return Rejected;
			}
			return Preview;
		}
		if (Payload.bSplitIntent)
		{
			Rejected.Message = TEXT("地面来源只接受明确的 WorldPickup 数量草稿。");
			return Rejected;
		}
		if (bSimpleStack)
		{
			if (!bP26PlayerStorage || !bP29QuickDestination
					|| (Target.bOccupied && Preview.Operation != ECodeBOperation::Merge)
				|| (!Target.bOccupied && Preview.Operation != ECodeBOperation::Move))
			{
				Rejected.Message = TEXT("地面简单堆叠只能拖到明确的 BaseQuick／当前空间 child 空格或兼容未满堆叠。");
				return Rejected;
			}
		}
		else if (bP32StandardEquipmentRoot)
		{
			const bool bExplicitBaseQuick = TargetContainer
				&& TargetContainer->Role == FName(TEXT("Basic6"))
				&& !Target.bOccupied && Preview.Operation == ECodeBOperation::Move;
			const FCodeBP3InventoryWorkspaceContext* Context = Host->GetWorkspaceContext();
			const bool bExplicitCurrentChild = bP35ChildStandardRecord && TargetContainer
				&& (TargetContainer->Role == FName(TEXT("QuickSpatial"))
					|| TargetContainer->Role == FName(TEXT("PouchInternal")))
				&& !Target.bOccupied && Preview.Operation == ECodeBOperation::Move
				&& Host->IsCurrentActiveP17ChildContainer(Target.ContainerId)
				&& Payload.QuickTransferActivePlayerContainerId == Target.ContainerId
				&& Payload.ActivePlayerChildOpenGeneration != 0 && Context
				&& Payload.ActivePlayerChildOpenGeneration == Context->ActiveDestinationOpenGeneration;
			const bool bExplicitCompatibleEquipment = TargetContainer
				&& !Target.bOccupied && Preview.Operation == ECodeBOperation::Equip;
			if (Payload.bQuickTransferIntent
				|| (!bExplicitBaseQuick && !bExplicitCurrentChild && !bExplicitCompatibleEquipment))
			{
				Rejected.Message = TEXT("标准装备只接受 normal Drag 到明确空 BaseQuick、P35 当前 child 空格或明确空兼容装备位；不自动选槽或替换。");
				return Rejected;
			}
		}
		else if (bSimpleRoot)
		{
			if (!TargetContainer || TargetContainer->Role != FName(TEXT("Basic6"))
				|| Target.bOccupied || Preview.Operation != ECodeBOperation::Move)
			{
				Rejected.Message = TEXT("既有 P14 简单非堆叠地面物品仍只可拖回明确的 BaseQuick 空格。");
				return Rejected;
			}
		}
		else if (Target.bOccupied
			|| (Preview.Operation != ECodeBOperation::Move && Preview.Operation != ECodeBOperation::Equip))
		{
			Rejected.Message = TEXT("地面完整空间图只能进入合法空储物格或匹配空装备位。");
			return Rejected;
		}
	}
	return Preview;
}

bool UCodeBP3InventoryWidget::CommitInventoryTransfer(
	const FCodeBP4DragPayload& Payload,
	const FCodeBP3SlotAddress& Target,
	const TCHAR* InputLabel)
{
	const FCodeBP4DropPreview Preview = PreviewInventoryTransfer(Payload, Target);
	if (!Preview.bAllowed)
	{
		if (Host.IsValid() && Host->GetController()) Host->GetController()->SetP4Feedback(Preview.Message);
		return false;
	}
	FCodeBP4InteractionController* Interaction = GetP4Controller();
	FString GateError;
	const bool bCommitted = Host.IsValid() && Host->CanWriteWorkspace(GateError)
		&& Interaction && Interaction->CommitDrop(Payload, Target);
	if (!bCommitted && !GateError.IsEmpty() && Host.IsValid() && Host->GetController())
	{
		Host->GetController()->SetP4Feedback(GateError);
	}
	TraceP4Input(InputLabel, Target,
		FString::Printf(TEXT("Allowed=1 CommitSucceeded=%d Kind=%s"), bCommitted ? 1 : 0,
			*FCodeBP4InteractionController::GetDropKindLabel(Preview.Kind)), true);
	return bCommitted;
}

const FCodeBP2ContainerView* UCodeBP3InventoryWidget::ResolveQuickTransferDestination(
	const FCodeBP4DragPayload& Payload) const
{
	if (!Host.IsValid() || !Host->GetController()) return nullptr;
	const FCodeBP2Projection& Projection = Host->GetController()->GetProjection();
	auto FindContainer = [&Projection](const FGuid& ContainerId)
	{
		return Projection.Containers.FindByPredicate(
			[ContainerId](const FCodeBP2ContainerView& Candidate) { return Candidate.ContainerId == ContainerId; });
	};
	if (Host->IsWorldDropPresentation(Payload.Source.ContainerId))
	{
		if (const FCodeBP3InventoryWorkspaceContext* Context = Host->GetWorkspaceContext();
			Context && Context->ActiveDestinationContainerId.IsSet())
		{
			if (const FCodeBP2ContainerView* Active = FindContainer(Context->ActiveDestinationContainerId.GetValue());
				Active && (Active->Role == FName(TEXT("QuickSpatial"))
					|| Active->Role == FName(TEXT("PouchInternal"))))
			{
				return Active;
			}
		}
		return FindContainer(Projection.BasicContainerId);
	}
	if (Host->HasWorldDropPresentation())
	{
		return Host->IsP29PlayerQuickTransferSourceContainer(Payload.Source.ContainerId)
			? Projection.Containers.FindByPredicate(
				[](const FCodeBP2ContainerView& Candidate)
				{
					return Candidate.Role == FName(TEXT("WorldDropTarget"));
				})
			: nullptr;
	}
	if (Host->IsOutOfRaidWorkspace())
	{
		// P23 uses an explicit two-pane policy. Warehouse roots enter the active
		// player child only when the user selected that exact child; otherwise
		// BaseQuick is the deterministic default. Every player placement returns
		// to the warehouse and never guesses an equipment target.
		if (Payload.Source.ContainerId == Projection.WarehouseContainerId)
		{
			if (const FCodeBP3InventoryWorkspaceContext* Context = Host->GetWorkspaceContext();
				Context && Context->ActiveDestinationContainerId.IsSet())
			{
				if (const FCodeBP2ContainerView* Active = FindContainer(Context->ActiveDestinationContainerId.GetValue()))
				{
					if (Active->Role == FName(TEXT("QuickSpatial"))
						|| Active->Role == FName(TEXT("PouchInternal")))
					{
						return Active;
					}
				}
			}
			return FindContainer(Projection.BasicContainerId);
		}
		return FindContainer(Projection.WarehouseContainerId);
	}
	if (Host->IsExternalTargetContainer(Payload.Source.ContainerId))
	{
		if (Host->GetController()->GetSelectedAddress().IsSet())
		{
			const FCodeBP3SlotAddress& Selected = Host->GetController()->GetSelectedAddress().GetValue();
			if (!Host->IsExternalTargetContainer(Selected.ContainerId))
			{
				if (const FCodeBP2ContainerView* SelectedContainer = FindContainer(Selected.ContainerId))
				{
					const FString Role = SelectedContainer->Role.ToString();
					if (Role == TEXT("Basic6") || Role == TEXT("QuickSpatial") || Role == TEXT("PouchInternal"))
					{
						return SelectedContainer;
					}
				}
			}
		}
		return FindContainer(Projection.BasicContainerId);
	}
	if (const FCodeBP2ContainerView* External = Projection.Containers.FindByPredicate(
		[](const FCodeBP2ContainerView& Candidate)
		{
			return Candidate.Role == FName(TEXT("NormalContainerTarget"))
				|| Candidate.Role == FName(TEXT("BodyContainerTarget"));
		}))
	{
		return External;
	}
	if (Payload.Source.ContainerId != Projection.BasicContainerId)
	{
		return FindContainer(Projection.BasicContainerId);
	}
	if (const FCodeBP2ContainerView* Quick = Projection.Containers.FindByPredicate(
		[](const FCodeBP2ContainerView& Candidate) { return Candidate.Role == FName(TEXT("QuickSpatial")); }))
	{
		return Quick;
	}
	return Projection.Containers.FindByPredicate(
		[](const FCodeBP2ContainerView& Candidate) { return Candidate.Role == FName(TEXT("PouchInternal")); });
}

FLinearColor UCodeBP3InventoryWidget::GetNormalCellColor(const FCodeBP3SlotAddress& Address) const
{
	if (P4PreviewAddress.IsSet() && P4Preview.IsSet()
		&& P4PreviewAddress->ContainerId == Address.ContainerId && P4PreviewAddress->SlotIndex == Address.SlotIndex)
	{
		if (!P4Preview->bAllowed) return DragRejectedColor;
		return P4Preview->Kind == ECodeBP4DropKind::Merge ? DragMergeColor
			: (P4Preview->Kind == ECodeBP4DropKind::Replacement || P4Preview->Kind == ECodeBP4DropKind::Swap ? DragReplacementColor : DragAllowedColor);
	}
	if (!Host.IsValid() || !Host->GetController())
	{
		return Address.IsRevealed() ? ItemColor : EmptyColor;
	}
	const TOptional<FCodeBP3SlotAddress>& Pending = Host->GetController()->GetPendingSource();
	if (Pending.IsSet() && Pending->ContainerId == Address.ContainerId && Pending->SlotIndex == Address.SlotIndex)
	{
		return PendingColor;
	}
	if (const FCodeBP3SplitDraft* Draft = Host->GetSplitDraft();
		Draft && Draft->Source.ContainerId == Address.ContainerId && Draft->Source.SlotIndex == Address.SlotIndex)
	{
		return PendingColor;
	}
	const TOptional<FCodeBP3SlotAddress>& Selected = Host->GetController()->GetSelectedAddress();
	if (Selected.IsSet() && Selected->ContainerId == Address.ContainerId && Selected->SlotIndex == Address.SlotIndex)
	{
		return SelectedColor;
	}
	return Address.IsRevealed() ? ItemColor : EmptyColor;
}

bool UCodeBP3InventoryWidget::BeginP4Drag(UCodeBP3CellButton* CellButton, FCodeBP4DragPayload& OutPayload)
{
	if (!CellButton)
	{
		return false;
	}
	ContextMenuAddress.Reset();
	ContextMenuRevision = INDEX_NONE;
	P4PreviewAddress.Reset();
	P4Preview.Reset();
	FString GateError;
	if (!Host.IsValid() || !Host->CanWriteWorkspace(GateError))
	{
		if (Host.IsValid() && Host->GetController()) Host->GetController()->SetP4Feedback(GateError);
		return false;
	}
	if (FCodeBP4InteractionController* Interaction = GetP4Controller())
	{
		const bool bStarted = Host->BeginInventoryDrag(
			*Interaction, CellButton->GetAddress(), OutPayload, GateError);
		if (bStarted && Host.IsValid()) Host->PopulateTransferContext(OutPayload);
		else if (!GateError.IsEmpty() && Host.IsValid() && Host->GetController())
		{
			Host->GetController()->SetP4Feedback(GateError);
		}
		return bStarted;
	}
	return false;
}

void UCodeBP3InventoryWidget::HandleP4DragEnter(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation)
{
	if (!CellButton || !Operation)
	{
		return;
	}
	const FCodeBP4DropPreview Preview = PreviewInventoryTransfer(
		Operation->GetPayload(), CellButton->GetAddress());
	TraceP4Input(TEXT("DragEnter"), CellButton->GetAddress(),
		FString::Printf(TEXT("Allowed=%d Kind=%s"), Preview.bAllowed ? 1 : 0,
			*FCodeBP4InteractionController::GetDropKindLabel(Preview.Kind)));
	P4PreviewAddress = CellButton->GetAddress();
	P4Preview = Preview;
	CellButton->SetCellColor(GetNormalCellColor(CellButton->GetAddress()));
}

void UCodeBP3InventoryWidget::HandleP4DragLeave(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation)
{
	if (CellButton && Operation)
	{
		TraceP4Input(TEXT("DragLeave"), CellButton->GetAddress());
		if (P4PreviewAddress.IsSet() && P4PreviewAddress->ContainerId == CellButton->GetAddress().ContainerId && P4PreviewAddress->SlotIndex == CellButton->GetAddress().SlotIndex)
		{
			P4PreviewAddress.Reset();
			P4Preview.Reset();
		}
		CellButton->SetCellColor(GetNormalCellColor(CellButton->GetAddress()));
	}
}

void UCodeBP3InventoryWidget::HandleP4DragCancelled(UCodeBP4DragOperation* Operation)
{
	if (!Operation)
	{
		return;
	}
	TraceP4Input(TEXT("DragCancelled"), Operation->GetPayload().Source, TEXT("Slate cancelled the production drag operation; preview and payload state cleared"));
	if (P4Controller.IsValid())
	{
		P4Controller->CancelInteraction(TEXT("拖拽已取消，未写入任何物品状态"));
	}
	ContextMenuAddress.Reset();
	ContextMenuRevision = INDEX_NONE;
	P4PreviewAddress.Reset();
	P4Preview.Reset();
	RefreshFromController();
}

bool UCodeBP3InventoryWidget::HandleP4Drop(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation)
{
	if (!CellButton || !Operation)
	{
		return false;
	}
	const FCodeBP4DropPreview Preview = PreviewInventoryTransfer(
		Operation->GetPayload(), CellButton->GetAddress());
	if (Preview.bAllowed)
	{
		++CurrentP4SubmittedCommandCount;
		++CurrentP4P2CallCount;
	}
	const bool bCommitted = CommitInventoryTransfer(
		Operation->GetPayload(), CellButton->GetAddress(), TEXT("Drop"));
	TraceP4Input(TEXT("DropResult"), CellButton->GetAddress(),
		FString::Printf(TEXT("Preview=%s CommitSucceeded=%d Kind=%s"),
			Preview.bAllowed ? TEXT("Allowed") : TEXT("Rejected"), bCommitted ? 1 : 0,
			*FCodeBP4InteractionController::GetDropKindLabel(Preview.Kind)));
	ContextMenuAddress.Reset();
	ContextMenuRevision = INDEX_NONE;
	P4PreviewAddress.Reset();
	P4Preview.Reset();
	RefreshFromController();
	return true;
}

bool UCodeBP3InventoryWidget::HandleGroundDropZoneDrop(UCodeBP4DragOperation* Operation)
{
	if (!Operation || !Host.IsValid() || !Host->GetController() || !Host->HasGroundDropPresentation())
	{
		return false;
	}
	const FCodeBP4DragPayload& Payload = Operation->GetPayload();
	FString Error;
	const bool bCommitted = Host->RequestGroundDrop(Payload, Error);
	if (bCommitted && !Payload.bSplitIntent)
	{
		// The Store replacement removed this root (and possibly its P17 child
		// closure) from the live player projection. P26 partial drop deliberately
		// retains its source identity and stable cell instead.
		Host->GetController()->ClearTransientSelection(TEXT("完整物品图已转入地面真值。"));
	}
	Host->GetController()->SetP4Feedback(bCommitted
		? (Payload.bSplitIntent
			? TEXT("已提交明确数量的地面丢弃；来源余量与世界新堆均来自 P1 Split。")
			: TEXT("已提交地面丢弃；地面物品由 P6 持久化。"))
		: (Error.IsEmpty() ? TEXT("地面丢弃未完成。") : Error));
	TraceP4Input(TEXT("Drop"), Payload.Source,
		FString::Printf(TEXT("GroundDrop NativeOnDrop CommitSucceeded=%d"), bCommitted ? 1 : 0));
	ContextMenuAddress.Reset();
	ContextMenuRevision = INDEX_NONE;
	P4PreviewAddress.Reset();
	P4Preview.Reset();
	RefreshFromController();
	return true;
}

void UCodeBP3InventoryWidget::SetP4PreviewCapture(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target)
{
	if (FCodeBP4InteractionController* Interaction = GetP4Controller())
	{
		P4PreviewAddress = Target;
		P4Preview = Interaction->PreviewDrop(Payload, Target);
		RefreshFromController();
	}
}

void UCodeBP3InventoryWidget::HandleP4ContextMenu(UCodeBP3CellButton* CellButton)
{
	if (!CellButton || !Host.IsValid() || !Host->GetController())
	{
		return;
	}
	Host->GetController()->ActivateAddress(CellButton->GetAddress());
	TraceP4Input(TEXT("ContextMenuOpened"), CellButton->GetAddress());
	ContextMenuAddress = CellButton->GetAddress();
	ContextMenuRevision = Host->GetController()->GetProjection().Revision;
	Host->GetController()->SetP4Feedback(TEXT("右键菜单仅用于查看详情；物品移动请拖到明确目标"));
	RefreshFromController();
}

void UCodeBP3InventoryWidget::AddInventorySection(
	UVerticalBox* Parent,
	const FString& Title,
	const FCodeBP2ContainerView& Container,
	const int32 Columns,
	const EInventorySectionKind Kind)
{
	UVerticalBox* Section = AddPanel(Parent, FString::Printf(TEXT("%s  ·  %d 格"), *Title, Container.Capacity));
	if (Kind != EInventorySectionKind::Plain)
	{
		Section->AddChildToVerticalBox(MakeText(
			WidgetTree,
			TEXT("未揭示格可点击搜索但不暴露数量、定义、ItemId 或拖拽；揭示后与玩家格共用同一 Cell 和事务路由。"),
			12, FLinearColor(0.72f, 0.82f, 0.92f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 4.0f));
	}
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(3.0f));
	Section->AddChildToVerticalBox(Grid)->SetHorizontalAlignment(HAlign_Fill);

	const int32 SafeColumns = FMath::Max(1, Columns);
	for (int32 SlotIndex = 0; SlotIndex < Container.Capacity; ++SlotIndex)
	{
		const FCodeBP2SlotView* SlotView = Container.Slots.FindByPredicate(
			[SlotIndex](const FCodeBP2SlotView& Candidate) { return Candidate.SlotIndex == SlotIndex; });
		ECodeBP3CellState State = SlotView && SlotView->bOccupied
			? ECodeBP3CellState::Revealed : ECodeBP3CellState::Empty;
		FCodeBP3SearchLocator SearchLocator;
		if (Kind != EInventorySectionKind::Plain && Host.IsValid())
		{
			Host->ResolveExternalCellState(Container.ContainerId, SlotIndex, State, SearchLocator);
		}

		FCodeBP3SlotAddress Address;
		Address.ContainerId = Container.ContainerId;
		Address.SlotIndex = SlotIndex;
		Address.SlotId = SlotView && !SlotView->SlotId.IsNone()
			? SlotView->SlotId : FName(*FString::Printf(TEXT("Slot.%d"), SlotIndex));
		Address.CellState = State;
		Address.SearchLocator = SearchLocator;
		Address.bOccupied = State == ECodeBP3CellState::Revealed && SlotView && SlotView->bOccupied;
		Address.ItemId = Address.bOccupied ? SlotView->ItemId : FGuid();
		if (Host.IsValid()) Host->PopulateAddressContext(Address);

		UCodeBP3CellButton* Cell = WidgetTree->ConstructWidget<UCodeBP3CellButton>();
		Cell->Configure(this, Address);
		MountedCells.Add(Cell);
		Cell->SetCellColor(State == ECodeBP3CellState::Searching
			? FLinearColor(0.42f, 0.24f, 0.08f, 1.0f)
			: State == ECodeBP3CellState::Hidden
				? FLinearColor(0.16f, 0.18f, 0.22f, 1.0f)
				: GetNormalCellColor(Address));
		const FString Label = State == ECodeBP3CellState::Searching
			? TEXT("正在搜索\n内容保密")
			: State == ECodeBP3CellState::Hidden
				? TEXT("未搜索\n点击搜索")
				: Address.IsRevealed()
					? FString::Printf(TEXT("%s\nx%d  L%d/Q%d"), *SlotView->DefinitionId.ToString(), SlotView->Quantity, SlotView->Level, SlotView->Quality)
					: FString::Printf(TEXT("空格\n%s"), *Address.SlotId.ToString());
		Cell->SetCellContent(MakeText(
			WidgetTree, Label, 12,
			Address.IsRevealed() ? FLinearColor::White : FLinearColor(0.62f, 0.68f, 0.74f)));
		UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(
			Cell, SlotIndex / SafeColumns, SlotIndex % SafeColumns);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UCodeBP3InventoryWidget::AddContainerSection(UVerticalBox* Parent, const FString& Title, const FCodeBP2ContainerView& Container, const int32 Columns)
{
	AddInventorySection(Parent, Title, Container, Columns, EInventorySectionKind::Plain);
}

void UCodeBP3InventoryWidget::AddNormalContainerSection(UVerticalBox* Parent, const FCodeBP2ContainerView& Container)
{
	AddInventorySection(Parent, TEXT("普通容器"), Container, 2, EInventorySectionKind::NormalTarget);
}

void UCodeBP3InventoryWidget::AddBodyContainerSection(UVerticalBox* Parent, const FCodeBP2ContainerView& Container)
{
	AddInventorySection(Parent, TEXT("尸体"), Container, 2, EInventorySectionKind::BodyTarget);
}

void UCodeBP3InventoryWidget::AddBodyEquipmentContainerSection(
	UVerticalBox* Parent,
	const FCodeBP2ContainerView& Container,
	const FName SlotSemantic)
{
	AddInventorySection(
		Parent,
		FString::Printf(TEXT("尸体装备  ·  %s"), *SlotSemantic.ToString()),
		Container,
		1,
		EInventorySectionKind::BodyTarget);
}

void UCodeBP3InventoryWidget::AddSpatialContainerSection(
	UVerticalBox* Parent,
	const FString& EmptyTitle,
	const FCodeBP2ContainerView* Container,
	const FCodeBP2SlotView* ParentSlot)
{
	if (!Container || !ParentSlot || !ParentSlot->bOccupied || !ParentSlot->ChildContainerId.IsValid())
	{
		UVerticalBox* Empty = AddPanel(Parent, EmptyTitle);
		Empty->AddChildToVerticalBox(MakeText(
			WidgetTree, TEXT("当前没有可访问的真实空间 parent；未创建 0 格假容器。"),
			13, FLinearColor(0.65f, 0.70f, 0.76f)));
		return;
	}
	int32 DefinitionCapacity = INDEX_NONE;
	FString CapacityDiagnostic;
	if (ParentSlot->EquipSlot == ECodeBEquipSlot::SpatialItem)
	{
		const Fdemo_mapSpatialRingCapacityResult Result =
			Fdemo_mapItemDefinitions::ResolveSpatialRingCapacity(ParentSlot->DefinitionId);
		DefinitionCapacity = Result.bSuccess ? Result.Capacity : INDEX_NONE;
		CapacityDiagnostic = Result.Diagnostic;
	}
	else
	{
		const Fdemo_mapSpatialStorageCapacityResult Result =
			Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(ParentSlot->DefinitionId);
		DefinitionCapacity = Result.bSuccess ? Result.Capacity : INDEX_NONE;
		CapacityDiagnostic = Result.Diagnostic;
	}
	if (DefinitionCapacity != Container->Capacity)
	{
		UVerticalBox* Invalid = AddPanel(Parent, EmptyTitle);
		Invalid->AddChildToVerticalBox(MakeText(
			WidgetTree,
			FString::Printf(TEXT("空间容量 provenance 冲突：Definition=%d，ChildContainer=%d。%s"),
				DefinitionCapacity, Container->Capacity, *CapacityDiagnostic),
			13, FLinearColor(1.0f, 0.42f, 0.35f)));
		return;
	}
	int32 Used = 0;
	for (const FCodeBP2SlotView& CellSlot : Container->Slots)
	{
		if (CellSlot.bOccupied) ++Used;
	}
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(ParentSlot->DefinitionId);
	const FString DisplayName = Definition
		? Definition->DisplayName.ToString() : ParentSlot->DefinitionId.ToString();
	AddContainerSection(
		Parent,
		FString::Printf(TEXT("%s · 品质 %d · 已用 %d/%d"),
			*DisplayName, ParentSlot->Quality, Used, Container->Capacity),
		*Container,
		4);
}

void UCodeBP3InventoryWidget::AddHotbarPlaceholders(UVerticalBox* Parent)
{
	UVerticalBox* Section = AddPanel(Parent, TEXT("1—9 快捷栏引用（Shift+数字绑定；UI 内不使用）"));
	if (!Host.IsValid() || !Host->GetController() || !Host->GetHotbarPresentation())
	{
		Section->AddChildToVerticalBox(MakeText(
			WidgetTree,
			TEXT("此入口没有 P13 真实 Profile／活动 Run 快捷栏投影；不会显示或写入任何引用。"),
			13, FLinearColor(0.65f, 0.70f, 0.76f)));
		return;
	}

	const FCodeBP3HotbarPresentation* Presentation = Host->GetHotbarPresentation();
	Section->AddChildToVerticalBox(MakeText(
		WidgetTree,
		TEXT("鼠标悬停优先、否则使用当前选择；Shift+1—9 仅通过 P13 绑定 BaseQuick QuickUsable 引用，绝不移动、消耗或触发 P15 使用。"),
		13, FLinearColor(0.72f, 0.82f, 0.92f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 4.0f));

	UHorizontalBox* References = WidgetTree->ConstructWidget<UHorizontalBox>();
	Section->AddChildToVerticalBox(References);
	for (int32 Index = 1; Index <= FCodeBHotbarBindings::SlotCount; ++Index)
	{
		const FCodeBHotbarSlotProjection* HotbarSlot = Presentation->Projection.Slots.IsValidIndex(Index - 1)
			? &Presentation->Projection.Slots[Index - 1] : nullptr;
		const FString ReferenceLabel = HotbarSlot && HotbarSlot->bHasReference
			? FString::Printf(TEXT("%d\n%s x%d"), Index, *HotbarSlot->DefinitionId.ToString(), HotbarSlot->Quantity)
			: FString::Printf(TEXT("%d\n空"), Index);
		References->AddChildToHorizontalBox(MakeText(
			WidgetTree, ReferenceLabel, 12,
			HotbarSlot && HotbarSlot->bHasReference ? FLinearColor::White : FLinearColor(0.55f, 0.60f, 0.66f)))
			->SetPadding(FMargin(6.0f, 2.0f));
	}
}

void UCodeBP3InventoryWidget::AddGroundDropZone(UVerticalBox* Parent)
{
	if (!Host.IsValid() || !Host->HasGroundDropPresentation()) return;
	UVerticalBox* Section = AddPanel(Parent, TEXT("地面丢弃"));
	UCodeBP3GroundDropZone* Zone = WidgetTree->ConstructWidget<UCodeBP3GroundDropZone>();
	Zone->Configure(this);
	Section->AddChildToVerticalBox(Zone)->SetPadding(FMargin(2.0f));
}

void UCodeBP3InventoryWidget::AddDetailAndActions(UVerticalBox* Parent)
{
	UVerticalBox* Detail = AddPanel(Parent, TEXT("物品详情"));
	FString DetailText = TEXT("点击一个物品查看只读 Projection 详情。\n\n此页面不保存物品实例或数量真值。");
	if (Host.IsValid() && Host->GetController() && Host->GetController()->GetSelectedAddress().IsSet())
	{
		const FCodeBP3SlotAddress& Selected = Host->GetController()->GetSelectedAddress().GetValue();
		const FCodeBP2ContainerView* Container = Host->GetController()->GetProjection().Containers.FindByPredicate([&Selected](const FCodeBP2ContainerView& Candidate)
		{
			return Candidate.ContainerId == Selected.ContainerId;
		});
		const FCodeBP2SlotView* SlotView = Container
			? Container->Slots.FindByPredicate([&Selected](const FCodeBP2SlotView& Candidate)
				{ return Candidate.SlotIndex == Selected.SlotIndex; })
			: nullptr;
		if (SlotView)
		{
			const TCHAR* SourceDescription = Host->GetController()->IsProfileBacked()
				? TEXT("真实 Profile Code B 持久化实例。")
				: TEXT("Code B Fixture 最小可审计展示数据。");
			DetailText = FString::Printf(TEXT("定义：%s\n类型：%s\n数量：%d\n等级／品质：%d / %d\nItemId：%s\n容器／槽位：%s / %s\n随机种子：%d\n描述：%s"),
				*SlotView->DefinitionId.ToString(), *ItemTypeToChinese(SlotView->ItemType), SlotView->Quantity, SlotView->Level, SlotView->Quality,
				*SlotView->ItemId.ToString(EGuidFormats::DigitsWithHyphens), *Container->Role.ToString(), *SlotView->SlotId.ToString(), SlotView->RandomSeed, SourceDescription);
		}
	}
	Detail->AddChildToVerticalBox(MakeText(WidgetTree, DetailText, 14, FLinearColor(0.86f, 0.90f, 0.95f)))->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 10.0f));

	UVerticalBox* Actions = AddPanel(Parent, TEXT("物品操作规则"));
	Actions->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("拖拽使用明确目标；Ctrl+左键只按稳定候选顺序合并或进入空槽，不交换、不拆分、不自动装备；右键只读。"), 13, FLinearColor(0.72f, 0.82f, 0.92f)));
	if (Host.IsValid() && Host->GetController() && Host->GetController()->GetSelectedAddress().IsSet())
	{
		const FCodeBP3SlotAddress& Selected = Host->GetController()->GetSelectedAddress().GetValue();
		FString SplitEligibilityError;
		if (Host->GetController()->ValidateSplitSource(Selected, 1, SplitEligibilityError))
		{
			const FCodeBP3SplitDraft* Draft = Host->GetSplitDraft();
			const bool bMatchingDraft = Draft
				&& Draft->SourceItemId == Selected.ItemId
				&& Draft->Source.ContainerId == Selected.ContainerId
				&& Draft->Source.SlotIndex == Selected.SlotIndex;
			if (bMatchingDraft)
			{
				bSplitQuantityInputOpen = false;
				SplitInputItemId.Invalidate();
				const FString DraftDescription = Draft->Kind == ECodeBP3QuantityDraftKind::WorldPickup
					? FString::Printf(TEXT("地面拾回草稿：%d 个。拖到明确空玩家格；新 ItemId 只在 Drop 成功时由 P1 创建。"),
						Draft->RequestedQuantity)
					: FString::Printf(TEXT("拆分草稿：%d 个。拖动此来源堆到明确空储物格；新 ItemId 只在 Drop 成功时由 P1 创建。"),
						Draft->RequestedQuantity);
				Actions->AddChildToVerticalBox(MakeText(
					WidgetTree, DraftDescription,
					13, FLinearColor(0.78f, 0.88f, 0.98f)))->SetPadding(FMargin(2.0f, 8.0f, 2.0f, 2.0f));
				UButton* CancelButton = WidgetTree->ConstructWidget<UButton>();
				CancelButton->SetBackgroundColor(FLinearColor(0.30f, 0.12f, 0.10f, 1.0f));
				CancelButton->SetContent(MakeText(WidgetTree,
					Draft->Kind == ECodeBP3QuantityDraftKind::WorldPickup
						? TEXT("取消地面拾回草稿") : TEXT("取消拆分草稿"), 15));
				CancelButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnCancelSplitClicked);
				Actions->AddChildToVerticalBox(CancelButton)->SetPadding(FMargin(2.0f));
			}
			else if (bSplitQuantityInputOpen && SplitInputItemId == Selected.ItemId)
			{
				Actions->AddChildToVerticalBox(MakeText(
					WidgetTree, TEXT("输入要拆出的数量；此输入仍是临时 UI 状态，不会修改权威数量。"),
					13, FLinearColor(0.78f, 0.88f, 0.98f)))->SetPadding(FMargin(2.0f, 8.0f, 2.0f, 2.0f));
				SplitQuantityBox = WidgetTree->ConstructWidget<UEditableTextBox>();
				SplitQuantityBox->SetText(FText::AsNumber(1));
				SplitQuantityBox->SetHintText(FText::FromString(TEXT("拆分数量")));
				Actions->AddChildToVerticalBox(SplitQuantityBox)->SetPadding(FMargin(2.0f));
				UButton* ConfirmButton = WidgetTree->ConstructWidget<UButton>();
				ConfirmButton->SetBackgroundColor(HeaderColor);
				ConfirmButton->SetContent(MakeText(WidgetTree, TEXT("确认拆分草稿"), 15));
				ConfirmButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnBeginSplitClicked);
				Actions->AddChildToVerticalBox(ConfirmButton)->SetPadding(FMargin(2.0f));
				UButton* CancelButton = WidgetTree->ConstructWidget<UButton>();
				CancelButton->SetBackgroundColor(FLinearColor(0.30f, 0.12f, 0.10f, 1.0f));
				CancelButton->SetContent(MakeText(WidgetTree, TEXT("取消数量输入"), 15));
				CancelButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnCancelSplitClicked);
				Actions->AddChildToVerticalBox(CancelButton)->SetPadding(FMargin(2.0f));
			}
			else
			{
				bSplitQuantityInputOpen = false;
				SplitInputItemId.Invalidate();
				UButton* OpenSplitButton = WidgetTree->ConstructWidget<UButton>();
				OpenSplitButton->SetBackgroundColor(HeaderColor);
				OpenSplitButton->SetContent(MakeText(WidgetTree,
					Host->IsWorldDropPresentation(Selected.ContainerId)
						? TEXT("按数量拾回") : TEXT("拆分"), 15));
				OpenSplitButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnOpenSplitClicked);
				Actions->AddChildToVerticalBox(OpenSplitButton)->SetPadding(FMargin(2.0f, 8.0f, 2.0f, 2.0f));
			}
		}
		else if (SplitInputItemId == Selected.ItemId || bSplitQuantityInputOpen)
		{
			bSplitQuantityInputOpen = false;
			SplitInputItemId.Invalidate();
		}
		const bool bActiveRunBacked = Host->GetController()->IsActiveRunBacked();
		UButton* GuidanceButton = WidgetTree->ConstructWidget<UButton>();
		GuidanceButton->SetBackgroundColor(HeaderColor);
		if (IsSelectedItemInEquipmentSlot())
		{
			GuidanceButton->SetContent(MakeText(WidgetTree, TEXT("卸下（拖到明确储物目标）"), 15));
			if (bActiveRunBacked) GuidanceButton->SetIsEnabled(false);
			else GuidanceButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnUnequipGuidanceClicked);
			Actions->AddChildToVerticalBox(GuidanceButton)->SetPadding(FMargin(2.0f, 8.0f, 2.0f, 2.0f));
		}
		else if (IsSelectedItemEquipable())
		{
			GuidanceButton->SetContent(MakeText(WidgetTree, TEXT("装备（拖到明确装备栏）"), 15));
			if (bActiveRunBacked) GuidanceButton->SetIsEnabled(false);
			else GuidanceButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnEquipGuidanceClicked);
			Actions->AddChildToVerticalBox(GuidanceButton)->SetPadding(FMargin(2.0f, 8.0f, 2.0f, 2.0f));
		}
		else
		{
			Actions->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("当前物品不能装备；可拖到任一普通储物位置。"), 13, FLinearColor(0.65f, 0.70f, 0.76f)))->SetPadding(FMargin(2.0f, 8.0f, 2.0f, 2.0f));
		}
	}
	AddP4ContextActions(Parent);
}

void UCodeBP3InventoryWidget::AddP4ContextActions(UVerticalBox* Parent)
{
	if (!Parent || !Host.IsValid() || !Host->GetController() || !ContextMenuAddress.IsSet())
	{
		return;
	}
	if (ContextMenuRevision != Host->GetController()->GetProjection().Revision)
	{
		ContextMenuAddress.Reset();
		ContextMenuRevision = INDEX_NONE;
		return;
	}

	FCodeBP3SlotAddress AuthoritativeAddress;
	if (!Host->GetController()->MakeAddress(ContextMenuAddress->ContainerId, ContextMenuAddress->SlotIndex, AuthoritativeAddress)
		|| !AuthoritativeAddress.bOccupied
		|| AuthoritativeAddress.ItemId != ContextMenuAddress->ItemId)
	{
		ContextMenuAddress.Reset();
		ContextMenuRevision = INDEX_NONE;
		return;
	}

	UVerticalBox* Menu = AddPanel(Parent, TEXT("右键上下文菜单"));
	Menu->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("菜单只保存稳定格位地址；仅可查看详情，绝不写入物品位置。"), 13, FLinearColor(0.72f, 0.82f, 0.92f)));
	UButton* DetailButton = WidgetTree->ConstructWidget<UButton>();
	DetailButton->SetBackgroundColor(HeaderColor);
	DetailButton->SetContent(MakeText(WidgetTree, TEXT("查看详情"), 15));
	DetailButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnContextDetailClicked);
	Menu->AddChildToVerticalBox(DetailButton)->SetPadding(FMargin(2.0f));
}

void UCodeBP3InventoryWidget::BuildPageContents()
{
	if (!PageContents || !Host.IsValid() || !Host->GetController())
	{
		return;
	}
	if (PlayerScrollBox) PlayerScrollOffset = PlayerScrollBox->GetScrollOffset();
	if (TargetScrollBox) TargetScrollOffset = TargetScrollBox->GetScrollOffset();
	if (Host.IsValid())
	{
		Host->UpdateWorkspaceScroll(PlayerScrollOffset, TargetScrollOffset);
		if (const FCodeBP3InventoryWorkspaceContext* Context = Host->GetWorkspaceContext())
		{
			PlayerScrollOffset = Context->PlayerScrollOffset;
			TargetScrollOffset = Context->TargetScrollOffset;
		}
	}
	PageContents->ClearChildren();
	MountedCells.Reset();
	SplitQuantityBox = nullptr;
	CloseButton = nullptr;
	PlayerScrollBox = nullptr;
	TargetScrollBox = nullptr;
	FCodeBP3UIController* Controller = Host->GetController();
	const FCodeBP2Projection& Projection = Controller->GetProjection();

	const bool bProfileBacked = Controller->IsProfileBacked();
	const bool bActiveRunBacked = Controller->IsActiveRunBacked();
	UVerticalBox* Header = AddPanel(
		PageContents,
		bActiveRunBacked
			? TEXT("Code B · 真实活动 Run 个人背包")
			: bProfileBacked
			? TEXT("Code B · 真实局外仓库／人物配置")
			: TEXT("Code B · 局外仓库／人物配置 · P3 开发 Host"));
	Header->AddChildToVerticalBox(MakeText(
		WidgetTree,
		bActiveRunBacked
			? FString::Printf(TEXT("P6 活动会话   Revision：%d   仅真实拖拽 Drop 可提交"), Projection.Revision)
			: bProfileBacked
			? FString::Printf(TEXT("真实 Profile 持久化状态   Revision：%d   模式：%s"), Projection.Revision, *FCodeBP3UIController::GetModeLabel(Controller->GetOperationMode()))
			: FString::Printf(TEXT("Host：已启用（默认关闭）   Revision：%d   模式：%s"), Projection.Revision, *FCodeBP3UIController::GetModeLabel(Controller->GetOperationMode())),
		16,
		FLinearColor(0.70f, 0.92f, 0.78f)));
	Header->AddChildToVerticalBox(MakeText(WidgetTree, Controller->GetFeedback(), 14, FLinearColor(1.0f, 0.82f, 0.43f)))->SetPadding(FMargin(0.0f, 4.0f));
	CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetBackgroundColor(FLinearColor(0.32f, 0.10f, 0.10f, 1.0f));
	UTextBlock* CloseLabel = MakeText(WidgetTree, TEXT("关闭页面（Esc）"), 14);
	CloseLabel->SetAutoWrapText(false);
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseLabel);
	CloseButton->OnClicked.AddDynamic(this, &UCodeBP3InventoryWidget::OnCloseClicked);
	UVerticalBoxSlot* CloseSlot = Header->AddChildToVerticalBox(CloseButton);
	CloseSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	CloseSlot->SetHorizontalAlignment(HAlign_Fill);

	UHorizontalBox* Main = WidgetTree->ConstructWidget<UHorizontalBox>();
	PageContents->AddChildToVerticalBox(Main)->SetPadding(FMargin(4.0f));
	UVerticalBox* PlayerColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UVerticalBox* TargetColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	USizeBox* PlayerViewport = WidgetTree->ConstructWidget<USizeBox>();
	USizeBox* TargetViewport = WidgetTree->ConstructWidget<USizeBox>();
	PlayerViewport->SetHeightOverride(710.0f);
	TargetViewport->SetHeightOverride(710.0f);
	PlayerScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	TargetScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	PlayerScrollBox->SetOrientation(Orient_Vertical);
	TargetScrollBox->SetOrientation(Orient_Vertical);
	PlayerScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	TargetScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	PlayerScrollBox->AddChild(PlayerColumn);
	TargetScrollBox->AddChild(TargetColumn);
	PlayerViewport->SetContent(PlayerScrollBox);
	TargetViewport->SetContent(TargetScrollBox);
	FSlateChildSize PlayerSize; PlayerSize.SizeRule = ESlateSizeRule::Fill; PlayerSize.Value = 1.0f;
	FSlateChildSize TargetSize; TargetSize.SizeRule = ESlateSizeRule::Fill; TargetSize.Value = 1.0f;
	Main->AddChildToHorizontalBox(PlayerViewport)->SetSize(PlayerSize);
	Main->AddChildToHorizontalBox(TargetViewport)->SetSize(TargetSize);

	if (const FCodeBP2ContainerView* Weapon = FindRole(FName(TEXT("Weapon")))) AddContainerSection(PlayerColumn, TEXT("兵器"), *Weapon, 1);
	if (const FCodeBP2ContainerView* Armor = FindRole(FName(TEXT("Armor")))) AddContainerSection(PlayerColumn, TEXT("道袍"), *Armor, 1);
	if (const FCodeBP2ContainerView* Accessory0 = FindRole(FName(TEXT("Accessory0")))) AddContainerSection(PlayerColumn, TEXT("饰品 I"), *Accessory0, 1);
	if (const FCodeBP2ContainerView* Accessory1 = FindRole(FName(TEXT("Accessory1")))) AddContainerSection(PlayerColumn, TEXT("饰品 II"), *Accessory1, 1);
	if (const FCodeBP2ContainerView* SpatialRing = FindRole(FName(TEXT("SpatialRing")))) AddContainerSection(PlayerColumn, TEXT("空间戒指（仅此栏可装备）"), *SpatialRing, 1);
	if (const FCodeBP2ContainerView* Backpack = FindRole(FName(TEXT("Backpack")))) AddContainerSection(PlayerColumn, TEXT("空间储物囊"), *Backpack, 1);
	if (const FCodeBP2ContainerView* Basic = FindRole(FName(TEXT("Basic6")))) AddContainerSection(PlayerColumn, TEXT("基础物品"), *Basic, 3);
	const FCodeBP2ContainerView* QuickSpatial = FindRole(FName(TEXT("QuickSpatial")));
	AddSpatialContainerSection(PlayerColumn, TEXT("空间戒指内部"), QuickSpatial,
		QuickSpatial ? FindSpatialParent(*QuickSpatial) : nullptr);
	const FCodeBP2ContainerView* PouchInternal = FindRole(FName(TEXT("PouchInternal")));
	AddSpatialContainerSection(PlayerColumn, TEXT("空间储物囊内部"), PouchInternal,
		PouchInternal ? FindSpatialParent(*PouchInternal) : nullptr);
	AddHotbarPlaceholders(PlayerColumn);

	if (!bActiveRunBacked)
	{
		if (const FCodeBP2ContainerView* Stash = FindRole(FName(TEXT("Warehouse")))) AddContainerSection(TargetColumn, TEXT("局外仓库"), *Stash, 5);
	}
	if (bActiveRunBacked)
	{
		if (const FCodeBP2ContainerView* NormalTarget = FindRole(FName(TEXT("NormalContainerTarget"))))
		{
			AddNormalContainerSection(TargetColumn, *NormalTarget);
		}
		if (const FCodeBP2ContainerView* BodyTarget = FindRole(FName(TEXT("BodyContainerTarget"))))
		{
			AddBodyContainerSection(TargetColumn, *BodyTarget);
		}
		for (const FName BodyEquipmentRole : {
			FName(TEXT("Body.Weapon")), FName(TEXT("Body.ArmorRobe")), FName(TEXT("Body.Accessory0")) })
		{
			if (const FCodeBP2ContainerView* EquipmentContainer = FindRole(BodyEquipmentRole))
			{
				AddBodyEquipmentContainerSection(TargetColumn, *EquipmentContainer, BodyEquipmentRole);
			}
		}
		if (const FCodeBP2ContainerView* WorldTarget = FindRole(FName(TEXT("WorldDropTarget"))))
		{
			// P19 intentionally mounts only the P14 root cell. A spatial parent's
			// child graph remains Store-owned and cannot become a ground sub-item UI.
			AddContainerSection(TargetColumn, TEXT("地面单根（拖回明确空格或兼容未满堆叠）"), *WorldTarget, 1);
		}
	}
	if (bActiveRunBacked) AddGroundDropZone(TargetColumn);
	AddDetailAndActions(TargetColumn);
	PlayerScrollBox->SetScrollOffset(PlayerScrollOffset);
	TargetScrollBox->SetScrollOffset(TargetScrollOffset);
}

void UCodeBP3InventoryWidget::RefreshFromController()
{
	if (Host.IsValid())
	{
		Host->RefreshHotbarProjection();
	}
	BuildLayout();
	BuildPageContents();
}

UCodeBP3CellButton* UCodeBP3InventoryWidget::FindMountedCell(const FGuid& ContainerId, const int32 SlotIndex) const
{
	for (const TWeakObjectPtr<UCodeBP3CellButton>& Candidate : MountedCells)
	{
		if (UCodeBP3CellButton* Cell = Candidate.Get())
		{
			const FCodeBP3SlotAddress& Address = Cell->GetAddress();
			if (Address.ContainerId == ContainerId && Address.SlotIndex == SlotIndex)
			{
				return Cell;
			}
		}
	}
	return nullptr;
}

void UCodeBP3InventoryWidget::HandleCellActivated(UCodeBP3CellButton* CellButton)
{
	if (Host.IsValid() && Host->GetController() && CellButton)
	{
		const bool bActivatedDestination = Host->ActivateQuickTransferDestination(CellButton->GetAddress());
		if (bActivatedDestination && !CellButton->GetAddress().IsRevealed())
		{
			Host->GetController()->SetP4Feedback(Host->HasWorldDropPresentation()
				? TEXT("已激活该玩家空间区；地面 Ctrl+左键将优先进入此处。")
				: TEXT("已激活该玩家空间区；仓库 Ctrl+左键将优先进入此处。"));
			RefreshFromController();
			return;
		}
		FString SearchError;
		if (Host->IsNormalContainerPresentation(CellButton->GetAddress().ContainerId)
			&& Host->IsNormalContainerSlotProtected(
				CellButton->GetAddress().ContainerId, CellButton->GetAddress().SlotIndex))
		{
			// The normal-container cell intentionally carries no hidden ItemId in
			// its widget address. The Host resolves the current projection only to
			// start a P10 search; it never opens details or an item write path here.
			if (!Host->RequestNormalContainerItemSearch(
				CellButton->GetAddress().ContainerId,
				CellButton->GetAddress().SlotIndex,
				SearchError))
			{
				Host->GetController()->SetP4Feedback(SearchError.IsEmpty()
					? TEXT("该普通容器格当前不能搜索")
					: SearchError);
				RefreshFromController();
			}
			return;
		}
		if (Host->IsBodyContainerPresentation(CellButton->GetAddress().ContainerId)
			&& Host->IsBodyContainerSlotProtected(
				CellButton->GetAddress().ContainerId, CellButton->GetAddress().SlotIndex))
		{
			if (!Host->RequestBodyContainerItemSearch(
				CellButton->GetAddress().ContainerId,
				CellButton->GetAddress().SlotIndex,
				SearchError))
			{
				Host->GetController()->SetP4Feedback(SearchError.IsEmpty()
					? TEXT("该尸体格当前不能搜查") : SearchError);
				RefreshFromController();
			}
			return;
		}
		TraceP4Input(TEXT("P3Selection"), CellButton->GetAddress());
		if (Host->GetController()->ActivateAddress(CellButton->GetAddress()))
		{
			Host->UpdateWorkspaceSelection(CellButton->GetAddress());
		}
		RefreshFromController();
	}
}

void UCodeBP3InventoryWidget::HandleCellHover(const FCodeBP3SlotAddress& Address, const bool bHovered)
{
	if (bHovered)
	{
		HoveredAddress = Address;
	}
	else if (HoveredAddress.IsSet()
		&& HoveredAddress->ContainerId == Address.ContainerId
		&& HoveredAddress->SlotIndex == Address.SlotIndex)
	{
		HoveredAddress.Reset();
	}
	if (Host.IsValid())
	{
		Host->UpdateWorkspaceHover(Address, bHovered);
	}
}

void UCodeBP3InventoryWidget::HandleQuickTransfer(UCodeBP3CellButton* CellButton)
{
	if (!CellButton || !CellButton->GetAddress().IsRevealed()
		|| !Host.IsValid() || !Host->GetController())
	{
		return;
	}
	const FCodeBP3SlotAddress& SourceAddress = CellButton->GetAddress();
	// Ctrl+左键始终保留原有完整堆 Quick Transfer；不消费拆分数量。
	Host->CancelSplitDraft(TEXT("Ctrl+左键保持完整堆 Quick Transfer；拆分草稿已清除"));
	if (Host->IsNormalContainerSlotProtected(SourceAddress.ContainerId, SourceAddress.SlotIndex)
		|| Host->IsBodyContainerSlotProtected(SourceAddress.ContainerId, SourceAddress.SlotIndex))
	{
		Host->GetController()->SetP4Feedback(TEXT("Hidden／Searching 格不能 Quick Transfer。"));
		return;
	}
	FCodeBP4DragPayload Payload;
	if (!BeginP4Drag(CellButton, Payload))
	{
		return;
	}
	Payload.bQuickTransferIntent = true;
	const FCodeBP2SlotView* SourceSlot = FindSlot(SourceAddress);
	const bool bSimpleStack = SourceSlot && SourceSlot->bStackable && SourceSlot->MaxStack > 1
		&& SourceSlot->Quantity > 0 && !SourceSlot->ChildContainerId.IsValid();
	const bool bWorldSource = Host->IsWorldDropPresentation(SourceAddress.ContainerId);
	const bool bP30CompleteGraphRoot = bWorldSource && SourceSlot
		&& SourceSlot->ChildContainerId.IsValid()
		&& (SourceSlot->DefinitionId == Fdemo_mapItemIds::WindTalisman
			|| SourceSlot->DefinitionId == Fdemo_mapItemIds::BackpackLevel1);
	const bool bP34StandardEquipmentRoot = bWorldSource && SourceSlot
		&& !SourceSlot->ChildContainerId.IsValid()
		&& !SourceSlot->bStackable && SourceSlot->MaxStack == 1 && SourceSlot->Quantity == 1
		&& ((SourceSlot->ItemType == ECodeBItemType::Weapon
				&& SourceSlot->EquipSlot == ECodeBEquipSlot::Weapon)
			|| (SourceSlot->ItemType == ECodeBItemType::Armor
				&& SourceSlot->EquipSlot == ECodeBEquipSlot::Armor)
			|| (SourceSlot->ItemType == ECodeBItemType::Accessory
				&& SourceSlot->EquipSlot == ECodeBEquipSlot::Accessory))
		&& Host->IsP34StandardEquipmentWorldDropSource(SourceAddress.ContainerId);
	if ((bWorldSource && !bSimpleStack && !bP30CompleteGraphRoot && !bP34StandardEquipmentRoot)
		|| (!bWorldSource && Host->HasWorldDropPresentation()
			&& Host->IsP29PlayerQuickTransferSourceContainer(SourceAddress.ContainerId)
			&& !bSimpleStack))
	{
		Host->GetController()->SetP4Feedback(
			TEXT("WorldDrop Ctrl 快转只接受 simple stack、正式 P19 完整空间图，或 P32/P33 标准装备 root。"));
		return;
	}
	if (bP30CompleteGraphRoot || bP34StandardEquipmentRoot)
	{
		// P30/P34 never inherit P29's active-child preference. Their only
		// deterministic quick target is the first empty BaseQuick slot.
		Payload.QuickTransferActivePlayerContainerId.Invalidate();
		Payload.ActivePlayerChildOpenGeneration = 0;
	}
	const FCodeBP2ContainerView* Destination = (bP30CompleteGraphRoot || bP34StandardEquipmentRoot)
		? Host->GetController()->GetProjection().Containers.FindByPredicate(
			[this](const FCodeBP2ContainerView& Candidate)
			{
				return Candidate.ContainerId == Host->GetController()->GetProjection().BasicContainerId
					&& Candidate.Role == FName(TEXT("Basic6"));
			})
		: ResolveQuickTransferDestination(Payload);
	if (!Destination || Destination->ContainerId == Payload.Source.ContainerId)
	{
		Host->GetController()->SetP4Feedback(TEXT("当前没有明确且合法的 Quick Transfer 目标容器。"));
		return;
	}
	auto MakeTarget = [this, Destination](const FCodeBP2SlotView& CellSlot)
	{
		FCodeBP3SlotAddress Target;
		Target.ContainerId = Destination->ContainerId;
		Target.SlotIndex = CellSlot.SlotIndex;
		Target.SlotId = CellSlot.SlotId;
		Target.ItemId = CellSlot.ItemId;
		Target.bOccupied = CellSlot.bOccupied;
		Target.CellState = CellSlot.bOccupied ? ECodeBP3CellState::Revealed : ECodeBP3CellState::Empty;
		if (Host.IsValid()) Host->PopulateAddressContext(Target);
		return Target;
	};
	for (int32 SlotIndex = 0;
		!bP30CompleteGraphRoot && !bP34StandardEquipmentRoot && SlotIndex < Destination->Capacity;
		++SlotIndex)
	{
		const FCodeBP2SlotView* CandidateSlot = Destination->Slots.FindByPredicate(
			[SlotIndex](const FCodeBP2SlotView& Candidate) { return Candidate.SlotIndex == SlotIndex; });
		if (!CandidateSlot || !CandidateSlot->bOccupied || CandidateSlot->DefinitionId != Payload.DefinitionId) continue;
		const FCodeBP3SlotAddress Target = MakeTarget(*CandidateSlot);
		const FCodeBP4DropPreview Preview = PreviewInventoryTransfer(Payload, Target);
		if (Preview.bAllowed && Preview.Kind == ECodeBP4DropKind::Merge)
		{
			CommitInventoryTransfer(Payload, Target, TEXT("CtrlLeftQuickTransfer"));
			RefreshFromController();
			return;
		}
	}
	for (int32 SlotIndex = 0; SlotIndex < Destination->Capacity; ++SlotIndex)
	{
		const FCodeBP2SlotView* CandidateSlot = Destination->Slots.FindByPredicate(
			[SlotIndex](const FCodeBP2SlotView& Candidate) { return Candidate.SlotIndex == SlotIndex; });
		if (!CandidateSlot || CandidateSlot->bOccupied) continue;
		const FCodeBP3SlotAddress Target = MakeTarget(*CandidateSlot);
		const FCodeBP4DropPreview Preview = PreviewInventoryTransfer(Payload, Target);
		if (Preview.bAllowed && Preview.Kind != ECodeBP4DropKind::Swap)
		{
			CommitInventoryTransfer(Payload, Target, TEXT("CtrlLeftQuickTransfer"));
			RefreshFromController();
			return;
		}
	}
	Host->GetController()->SetP4Feedback(TEXT("Quick Transfer 未找到合法合并或空槽；未写入。"));
	RefreshFromController();
}

void UCodeBP3InventoryWidget::BeginP4PointerGesture(const FCodeBP3SlotAddress& Address)
{
	CurrentP4GestureId = ++NextP4GestureId;
	CurrentP4SubmittedCommandCount = 0;
	CurrentP4P2CallCount = 0;
	CurrentP4RevisionBefore = Host.IsValid() && Host->GetController() ? Host->GetController()->GetProjection().Revision : INDEX_NONE;
	TraceP4Input(TEXT("PointerDown"), Address);
}

void UCodeBP3InventoryWidget::TraceP4Input(const FString& EventName, const FCodeBP3SlotAddress& Address, const FString& Detail, const bool bSubmittedCommand)
{
	// Retained parameter keeps the r0 call signature source-compatible. P4x owns its
	// accounting at the sole physical drop write boundary above.
	(void)bSubmittedCommand;
	const int32 RevisionAfter = Host.IsValid() && Host->GetController() ? Host->GetController()->GetProjection().Revision : INDEX_NONE;
	const int32 bDragOperation = EventName.Contains(TEXT("Drag")) || EventName.Equals(TEXT("Drop")) ? 1 : 0;
	const FString Result = EventName.Equals(TEXT("Drop"))
		? (Detail.Contains(TEXT("CommitSucceeded=1")) ? TEXT("Accepted") : TEXT("Rejected"))
		: TEXT("NoWrite");
	UE_LOG(LogTemp, Display, TEXT("P4X.UITrace Gesture=%llu Event=%s ActualHitContainer=%s Slot=%d ItemId=%s DragOperation=%d Preview=%s P2CommandCount=%d P2CallCount=%d RevisionBefore=%d RevisionAfter=%d Result=%s Detail=%s"),
		static_cast<unsigned long long>(CurrentP4GestureId), *EventName, *Address.ContainerId.ToString(EGuidFormats::DigitsWithHyphens),
		Address.SlotIndex, *Address.ItemId.ToString(EGuidFormats::DigitsWithHyphens), bDragOperation,
		Detail.Contains(TEXT("Preview=Allowed")) ? TEXT("Allowed") : (Detail.Contains(TEXT("Preview=Rejected")) ? TEXT("Rejected") : TEXT("N/A")),
		CurrentP4SubmittedCommandCount, CurrentP4P2CallCount, CurrentP4RevisionBefore, RevisionAfter, *Result, *Detail);
}

bool UCodeBP3InventoryWidget::IsSelectedItemInEquipmentSlot() const
{
	if (!Host.IsValid() || !Host->GetController() || !Host->GetController()->GetSelectedAddress().IsSet())
	{
		return false;
	}
	const FGuid ContainerId = Host->GetController()->GetSelectedAddress()->ContainerId;
	const FCodeBP2ContainerView* Container = Host->GetController()->GetProjection().Containers.FindByPredicate([ContainerId](const FCodeBP2ContainerView& Candidate)
	{
		return Candidate.ContainerId == ContainerId;
	});
	if (!Container)
	{
		return false;
	}

	// Use the P2 projection role rather than fixture IDs.  The same code path
	// now correctly presents read-only equip/unequip guidance for a formal
	// Profile and for the retained development fixture, without granting the
	// guidance button any write authority.
	return Container->Role == FName(TEXT("Weapon"))
		|| Container->Role == FName(TEXT("Armor"))
		|| Container->Role == FName(TEXT("SpatialRing"))
		|| Container->Role == FName(TEXT("Backpack"))
		|| Container->Role.ToString().StartsWith(TEXT("Accessory"));
}

bool UCodeBP3InventoryWidget::IsSelectedItemEquipable() const
{
	if (!Host.IsValid() || !Host->GetController() || !Host->GetController()->GetSelectedAddress().IsSet())
	{
		return false;
	}
	const FCodeBP3SlotAddress& Selected = Host->GetController()->GetSelectedAddress().GetValue();
	const FCodeBP2ContainerView* Container = Host->GetController()->GetProjection().Containers.FindByPredicate([&Selected](const FCodeBP2ContainerView& Candidate)
	{
		return Candidate.ContainerId == Selected.ContainerId;
	});
	const FCodeBP2SlotView* SelectedSlot = Container
		? Container->Slots.FindByPredicate([&Selected](const FCodeBP2SlotView& Candidate)
			{ return Candidate.SlotIndex == Selected.SlotIndex; })
		: nullptr;
	if (!SelectedSlot)
	{
		return false;
	}
	return SelectedSlot->ItemType == ECodeBItemType::Weapon
		|| SelectedSlot->ItemType == ECodeBItemType::Armor
		|| SelectedSlot->ItemType == ECodeBItemType::Accessory
		|| SelectedSlot->ItemType == ECodeBItemType::SpatialItem
		|| SelectedSlot->ItemType == ECodeBItemType::Backpack;
}

void UCodeBP3InventoryWidget::OnEquipGuidanceClicked()
{
	if (Host.IsValid() && Host->GetController())
	{
		Host->GetController()->SetP4Feedback(TEXT("请将该物品拖到明确且兼容的装备栏；本按钮不提交位置事务"));
		RefreshFromController();
	}
}

void UCodeBP3InventoryWidget::OnUnequipGuidanceClicked()
{
	if (Host.IsValid() && Host->GetController())
	{
		Host->GetController()->SetP4Feedback(TEXT("请将已装备物品拖到明确的普通储物目标；本按钮不会自动放回或提交事务"));
		RefreshFromController();
	}
}

void UCodeBP3InventoryWidget::OnOpenSplitClicked()
{
	if (!Host.IsValid() || !Host->GetController()
		|| !Host->GetController()->GetSelectedAddress().IsSet())
	{
		return;
	}
	const FCodeBP3SlotAddress& Selected = Host->GetController()->GetSelectedAddress().GetValue();
	FString Error;
	if (!Host->GetController()->ValidateSplitSource(Selected, 1, Error))
	{
		Host->GetController()->SetP4Feedback(Error);
		return;
	}
	Host->CancelSplitDraft(TEXT("已用新的数量输入替换旧拆分草稿"));
	bSplitQuantityInputOpen = true;
	SplitInputItemId = Selected.ItemId;
	Host->GetController()->SetP4Feedback(TEXT("拆分数量输入已打开；尚未写入任何物品状态。"));
	RefreshFromController();
}

void UCodeBP3InventoryWidget::OnBeginSplitClicked()
{
	if (!Host.IsValid() || !Host->GetController() || !SplitQuantityBox
		|| !Host->GetController()->GetSelectedAddress().IsSet())
	{
		return;
	}
	const FString QuantityText = SplitQuantityBox->GetText().ToString().TrimStartAndEnd();
	bool bIntegerText = !QuantityText.IsEmpty();
	for (const TCHAR Character : QuantityText)
	{
		bIntegerText = bIntegerText && FChar::IsDigit(Character);
	}
	const int32 RequestedQuantity = FCString::Atoi(*QuantityText);
	FString Error;
	if (!bIntegerText || !Host->CreateSplitDraft(
		Host->GetController()->GetSelectedAddress().GetValue(), RequestedQuantity, Error))
	{
		Host->GetController()->SetP4Feedback(Error.IsEmpty()
			? TEXT("请输入合法的整数拆分数量。") : Error);
	}
	else
	{
		bSplitQuantityInputOpen = false;
		SplitInputItemId.Invalidate();
	}
	RefreshFromController();
}

void UCodeBP3InventoryWidget::OnCancelSplitClicked()
{
	if (Host.IsValid())
	{
		Host->CancelSplitDraft();
	}
	bSplitQuantityInputOpen = false;
	SplitInputItemId.Invalidate();
	RefreshFromController();
}

void UCodeBP3InventoryWidget::OnContextDetailClicked()
{
	if (ContextMenuAddress.IsSet() && Host.IsValid() && Host->GetController())
	{
		Host->GetController()->ActivateAddress(ContextMenuAddress.GetValue());
		Host->GetController()->SetP4Feedback(TEXT("已查看详情；未写入物品状态"));
	}
	ContextMenuAddress.Reset();
	ContextMenuRevision = INDEX_NONE;
	RefreshFromController();
}
void UCodeBP3InventoryWidget::OnCloseClicked()
{
	TraceP4Input(TEXT("CloseButton"), FCodeBP3SlotAddress(), TEXT("Mounted close button routed to host lifecycle"));
	if (Host.IsValid())
	{
		Host->ClosePage();
	}
}

void UCodeBP3UIHostSubsystem::Deinitialize()
{
	ClosePage();
	if (Controller.IsValid())
	{
		Controller->Shutdown();
		Controller.Reset();
	}
	Super::Deinitialize();
}

APlayerController* UCodeBP3UIHostSubsystem::GetPlayerController() const
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

void UCodeBP3UIHostSubsystem::CaptureInput(APlayerController* PlayerController, UCodeBP3InventoryWidget* Widget)
{
	if (!PlayerController || !Widget || bInputCaptured)
	{
		return;
	}
	bPreviousShowMouseCursor = PlayerController->bShowMouseCursor;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	bInputCaptured = true;
}

void UCodeBP3UIHostSubsystem::RestoreInput(APlayerController* PlayerController)
{
	if (!PlayerController || !bInputCaptured)
	{
		return;
	}
	if (bPreviousShowMouseCursor)
	{
		FInputModeGameAndUI InputMode;
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
	PlayerController->SetShowMouseCursor(bPreviousShowMouseCursor);
	bInputCaptured = false;
}

bool UCodeBP3UIHostSubsystem::OpenPage()
{
	if (!Controller.IsValid())
	{
		Controller = MakeUnique<FCodeBP3UIController>();
	}
	FString Error;
	if (!Controller->Open(&Error))
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
		return false;
	}
	if (ActiveWidget.IsValid())
	{
		ActiveWidget->RefreshFromController();
		ActiveWidget->SetKeyboardFocus();
		return true;
	}
	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("CodeB.P3.Open requires a player controller."));
		return false;
	}
	UCodeBP3InventoryWidget* Page = CreateWidget<UCodeBP3InventoryWidget>(PlayerController);
	if (!Page)
	{
		UE_LOG(LogTemp, Error, TEXT("Code B P3 could not create its root widget."));
		return false;
	}
	Page->InitializeForHost(this);
	// Formal Code A menus may already occupy the default viewport layer.  The isolated P3 host must remain visibly
	// on top when explicitly opened, without changing any default-page ordering while it is disabled.
	Page->AddToViewport(10000);
	ActiveWidget = Page;
	CaptureInput(PlayerController, Page);
	return true;
}

bool UCodeBP3UIHostSubsystem::OpenProfilePage(
	FCodeBRepository& Repository,
	const FCodeBP2PlayerLayout& Layout,
	FCodeBP3UIController::FProfileCommit Commit,
	TFunction<void()> OnClosed,
	const bool bActiveRunPresentation,
	const FCodeBP3NormalContainerPresentation* InNormalContainerPresentation,
	const FCodeBP3BodyContainerPresentation* InBodyContainerPresentation,
	const FCodeBP3HotbarPresentation* InHotbarPresentation,
	const FCodeBP3GroundDropPresentation* InGroundDropPresentation,
	const FCodeBP3WorldDropPresentation* InWorldDropPresentation,
	const FCodeBP3WorkspacePresentation* InWorkspacePresentation)
{
	if (!Controller.IsValid())
	{
		Controller = MakeUnique<FCodeBP3UIController>();
	}
	FCodeBP2PlayerLayout EffectiveLayout = Layout;
	if (InBodyContainerPresentation)
	{
		for (const FCodeBBodyContainerEquipmentSlotProjection& EquipmentSlot : InBodyContainerPresentation->Projection.EquipmentSlots)
		{
			if (!EquipmentSlot.SlotSemantic.IsNone() && EquipmentSlot.ContainerId.IsValid())
			{
				EffectiveLayout.TransientPresentationContainers.Add(
					TPair<FName, FGuid>(EquipmentSlot.SlotSemantic, EquipmentSlot.ContainerId));
			}
		}
	}
	FString Error;
	const bool bOpenedController = bActiveRunPresentation
		? Controller->OpenActiveRun(Repository, EffectiveLayout, MoveTemp(Commit), &Error)
		: Controller->OpenProfile(Repository, EffectiveLayout, MoveTemp(Commit), &Error);
	if (!bOpenedController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
		return false;
	}
	NormalContainerPresentation = InNormalContainerPresentation
		? TOptional<FCodeBP3NormalContainerPresentation>(*InNormalContainerPresentation)
		: TOptional<FCodeBP3NormalContainerPresentation>();
	BodyContainerPresentation = InBodyContainerPresentation
		? TOptional<FCodeBP3BodyContainerPresentation>(*InBodyContainerPresentation)
		: TOptional<FCodeBP3BodyContainerPresentation>();
	HotbarPresentation = InHotbarPresentation
		? TOptional<FCodeBP3HotbarPresentation>(*InHotbarPresentation)
		: TOptional<FCodeBP3HotbarPresentation>();
	GroundDropPresentation = InGroundDropPresentation
		? TOptional<FCodeBP3GroundDropPresentation>(*InGroundDropPresentation)
		: TOptional<FCodeBP3GroundDropPresentation>();
	WorldDropPresentation = InWorldDropPresentation
		? TOptional<FCodeBP3WorldDropPresentation>(*InWorldDropPresentation)
		: TOptional<FCodeBP3WorldDropPresentation>();
	if (InWorkspacePresentation)
	{
		WorkspacePresentation = *InWorkspacePresentation;
	}
	else
	{
		FCodeBP3WorkspacePresentation Derived;
		Derived.Context.Scope = bActiveRunPresentation
			? ECodeBP3WorkspaceScope::InRunP6 : ECodeBP3WorkspaceScope::OutOfRaidP5;
		Derived.Context.WriteGate = bActiveRunPresentation
			? ECodeBP3WorkspaceWriteGate::InRun : ECodeBP3WorkspaceWriteGate::AtSect;
		Derived.Context.PlayerPaneId = bActiveRunPresentation
			? FName(TEXT("InRun.Player")) : FName(TEXT("OutOfRaid.Player"));
		Derived.Context.TargetPaneId = bActiveRunPresentation
			? FName(TEXT("InRun.External")) : FName(TEXT("OutOfRaid.Warehouse"));
		if (InHotbarPresentation)
		{
			Derived.Context.OwnerId = InHotbarPresentation->OwnerId;
			Derived.Context.RunInstanceId = InHotbarPresentation->RunInstanceId;
			Derived.Context.SessionRevision = InHotbarPresentation->Projection.DurableRevision;
		}
		else if (InNormalContainerPresentation)
		{
			Derived.Context.OwnerId = InNormalContainerPresentation->Projection.OwnerId;
			Derived.Context.RunInstanceId = InNormalContainerPresentation->Projection.RunInstanceId;
			Derived.Context.SessionRevision = InNormalContainerPresentation->Projection.Revision;
		}
		else if (InBodyContainerPresentation)
		{
			Derived.Context.OwnerId = InBodyContainerPresentation->Projection.OwnerId;
			Derived.Context.RunInstanceId = InBodyContainerPresentation->Projection.RunInstanceId;
			Derived.Context.SessionRevision = InBodyContainerPresentation->Projection.Revision;
		}
		else if (InWorldDropPresentation)
		{
			// P27 callers normally provide a live resolver-backed workspace. Keep
			// this fallback fail-closed: a world page without exact Owner/Run cannot
			// create a WorldPickupDraft.
			Derived.Context.SessionRevision = Controller->GetProjection().Revision;
		}
		WorkspacePresentation = MoveTemp(Derived);
	}
	ProfilePageClosed = MoveTemp(OnClosed);
	if (ActiveWidget.IsValid())
	{
		ActiveWidget->RefreshFromController();
		ActiveWidget->SetKeyboardFocus();
		return true;
	}
	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("Code B Profile page requires a player controller."));
		return false;
	}
	UCodeBP3InventoryWidget* Page = CreateWidget<UCodeBP3InventoryWidget>(PlayerController);
	if (!Page)
	{
		UE_LOG(LogTemp, Error, TEXT("Code B Profile page could not create its root widget."));
		return false;
	}
	Page->InitializeForHost(this);
	Page->AddToViewport(10000);
	ActiveWidget = Page;
	CaptureInput(PlayerController, Page);
	return true;
}

void UCodeBP3UIHostSubsystem::ClosePage()
{
	if (ActiveWidget.IsValid())
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget.Reset();
	}
	RestoreInput(GetPlayerController());
	if (Controller.IsValid() && Controller->IsOpen())
	{
		Controller->Close();
	}
	if (ProfilePageClosed)
	{
		TFunction<void()> Completion = MoveTemp(ProfilePageClosed);
		Completion();
	}
	NormalContainerPresentation.Reset();
	BodyContainerPresentation.Reset();
	HotbarPresentation.Reset();
	GroundDropPresentation.Reset();
	WorldDropPresentation.Reset();
	WorkspacePresentation.Reset();
}

bool UCodeBP3UIHostSubsystem::IsNormalContainerPresentation(const FGuid& ContainerId) const
{
	return NormalContainerPresentation.IsSet()
		&& NormalContainerPresentation->TargetContainerId == ContainerId;
}

bool UCodeBP3UIHostSubsystem::IsNormalContainerItemHidden(const FGuid& ItemId) const
{
	if (!NormalContainerPresentation.IsSet() || !ItemId.IsValid()) return false;
	const FCodeBNormalContainerItemProjection* Item =
		NormalContainerPresentation->Projection.Items.FindByPredicate(
			[ItemId](const FCodeBNormalContainerItemProjection& Value)
			{ return Value.ItemId == ItemId; });
	return Item && Item->RevealState == ECodeBNormalContainerRevealState::Hidden;
}

bool UCodeBP3UIHostSubsystem::IsNormalContainerItemSearching(const FGuid& ItemId) const
{
	if (!NormalContainerPresentation.IsSet() || !ItemId.IsValid()) return false;
	const FCodeBNormalContainerItemProjection* Item =
		NormalContainerPresentation->Projection.Items.FindByPredicate(
			[ItemId](const FCodeBNormalContainerItemProjection& Value)
			{ return Value.ItemId == ItemId; });
	return Item && Item->RevealState == ECodeBNormalContainerRevealState::Searching;
}

bool UCodeBP3UIHostSubsystem::IsNormalContainerSlotProtected(
	const FGuid& ContainerId,
	const int32 SlotIndex) const
{
	ECodeBP3CellState State = ECodeBP3CellState::Empty;
	FCodeBP3SearchLocator Locator;
	return ResolveExternalCellState(ContainerId, SlotIndex, State, Locator)
		&& (State == ECodeBP3CellState::Hidden || State == ECodeBP3CellState::Searching);
}

bool UCodeBP3UIHostSubsystem::RequestNormalContainerItemSearch(
	const FGuid& ContainerId,
	const int32 SlotIndex,
	FString& OutError)
{
	OutError.Reset();
	if (!NormalContainerPresentation.IsSet() || !Controller.IsValid()
		|| NormalContainerPresentation->TargetContainerId != ContainerId
		|| !NormalContainerPresentation->BeginItemSearch)
	{
		OutError = TEXT("普通容器搜索入口当前不可用。");
		return false;
	}
	ECodeBP3CellState State = ECodeBP3CellState::Empty;
	FCodeBP3SearchLocator Locator;
	if (!ResolveExternalCellState(ContainerId, SlotIndex, State, Locator)
		|| !Locator.IsValid())
	{
		OutError = TEXT("该普通容器格没有可搜索的物品。");
		return false;
	}
	if (State != ECodeBP3CellState::Hidden)
	{
		OutError = State == ECodeBP3CellState::Searching
			? TEXT("该物品正在搜索中。")
			: TEXT("该物品已揭示，不能再次搜索。");
		return false;
	}
	FCodeBNormalContainerProjection UpdatedProjection;
	if (!NormalContainerPresentation->BeginItemSearch(Locator, UpdatedProjection, OutError))
	{
		return false;
	}
	UpdateNormalContainerProjection(UpdatedProjection);
	return true;
}

void UCodeBP3UIHostSubsystem::UpdateNormalContainerProjection(
	const FCodeBNormalContainerProjection& Projection)
{
	if (!NormalContainerPresentation.IsSet()
		|| Projection.ContainerId != NormalContainerPresentation->TargetContainerId)
	{
		return;
	}
	NormalContainerPresentation->Projection = Projection;
	RefreshActivePage();
}

bool UCodeBP3UIHostSubsystem::IsBodyContainerPresentation(const FGuid& ContainerId) const
{
	return BodyContainerPresentation.IsSet()
		&& (BodyContainerPresentation->TargetContainerId == ContainerId
			|| IsBodyEquipmentContainerPresentation(ContainerId));
}

bool UCodeBP3UIHostSubsystem::IsBodyEquipmentContainerPresentation(const FGuid& ContainerId) const
{
	if (!BodyContainerPresentation.IsSet() || !ContainerId.IsValid()) return false;
	return BodyContainerPresentation->Projection.EquipmentSlots.ContainsByPredicate(
		[ContainerId](const FCodeBBodyContainerEquipmentSlotProjection& Value)
		{
			return Value.ContainerId == ContainerId;
		});
}

bool UCodeBP3UIHostSubsystem::IsBodyContainerItemHidden(const FGuid& ItemId) const
{
	if (!BodyContainerPresentation.IsSet() || !ItemId.IsValid()) return false;
	const FCodeBBodyContainerItemProjection* Item =
		BodyContainerPresentation->Projection.Items.FindByPredicate(
			[ItemId](const FCodeBBodyContainerItemProjection& Value) { return Value.ItemId == ItemId; });
	return Item && Item->Visibility == ECodeBBodyContainerVisibility::Hidden;
}

bool UCodeBP3UIHostSubsystem::IsBodyContainerItemSearching(const FGuid& ItemId) const
{
	if (!BodyContainerPresentation.IsSet() || !ItemId.IsValid()) return false;
	const FCodeBBodyContainerItemProjection* Item =
		BodyContainerPresentation->Projection.Items.FindByPredicate(
			[ItemId](const FCodeBBodyContainerItemProjection& Value) { return Value.ItemId == ItemId; });
	return Item && Item->Visibility == ECodeBBodyContainerVisibility::Searching;
}

bool UCodeBP3UIHostSubsystem::IsBodyContainerSlotProtected(
	const FGuid& ContainerId,
	const int32 SlotIndex) const
{
	ECodeBP3CellState State = ECodeBP3CellState::Empty;
	FCodeBP3SearchLocator Locator;
	return ResolveExternalCellState(ContainerId, SlotIndex, State, Locator)
		&& (State == ECodeBP3CellState::Hidden || State == ECodeBP3CellState::Searching);
}

bool UCodeBP3UIHostSubsystem::RequestBodyContainerItemSearch(
	const FGuid& ContainerId,
	const int32 SlotIndex,
	FString& OutError)
{
	OutError.Reset();
	if (!BodyContainerPresentation.IsSet() || !Controller.IsValid()
		|| !IsBodyContainerPresentation(ContainerId)
		|| !BodyContainerPresentation->BeginItemSearch)
	{
		OutError = TEXT("尸体搜查入口当前不可用。");
		return false;
	}
	ECodeBP3CellState State = ECodeBP3CellState::Empty;
	FCodeBP3SearchLocator Locator;
	if (!ResolveExternalCellState(ContainerId, SlotIndex, State, Locator)
		|| !Locator.IsValid())
	{
		OutError = TEXT("该尸体格没有可搜查的物品。");
		return false;
	}
	if (State != ECodeBP3CellState::Hidden)
	{
		OutError = State == ECodeBP3CellState::Searching
			? TEXT("该尸体物品正在搜查中。") : TEXT("该尸体物品已揭示，不能再次搜查。");
		return false;
	}
	FCodeBBodyContainerProjection UpdatedProjection;
	if (!BodyContainerPresentation->BeginItemSearch(Locator, UpdatedProjection, OutError))
	{
		return false;
	}
	UpdateBodyContainerProjection(UpdatedProjection);
	return true;
}

bool UCodeBP3UIHostSubsystem::ResolveExternalCellState(
	const FGuid& ContainerId,
	const int32 SlotIndex,
	ECodeBP3CellState& OutState,
	FCodeBP3SearchLocator& OutLocator) const
{
	OutState = ECodeBP3CellState::Empty;
	OutLocator = FCodeBP3SearchLocator();
	if (NormalContainerPresentation.IsSet()
		&& NormalContainerPresentation->TargetContainerId == ContainerId)
	{
		const FCodeBNormalContainerProjection& Projection = NormalContainerPresentation->Projection;
		const FCodeBNormalContainerItemProjection* Item = Projection.Items.FindByPredicate(
			[ContainerId, SlotIndex](const FCodeBNormalContainerItemProjection& Candidate)
			{
				return Candidate.ParentContainerId == ContainerId && Candidate.SlotIndex == SlotIndex;
			});
		if (!Item) return true;
		OutState = Item->RevealState == ECodeBNormalContainerRevealState::Hidden
			? ECodeBP3CellState::Hidden
			: Item->RevealState == ECodeBNormalContainerRevealState::Searching
				? ECodeBP3CellState::Searching : ECodeBP3CellState::Revealed;
		OutLocator.TargetKind = ECodeBP3SearchTargetKind::NormalContainer;
		OutLocator.OwnerId = Projection.OwnerId;
		OutLocator.RunInstanceId = Projection.RunInstanceId;
		OutLocator.TargetId = Projection.SearchTargetId;
		OutLocator.ContainerId = ContainerId;
		OutLocator.SlotIndex = SlotIndex;
		OutLocator.TargetRevision = Projection.Revision;
		OutLocator.ActiveActionId = Projection.ActiveActionId;
		return true;
	}
	if (BodyContainerPresentation.IsSet() && IsBodyContainerPresentation(ContainerId))
	{
		const FCodeBBodyContainerProjection& Projection = BodyContainerPresentation->Projection;
		const FCodeBBodyContainerItemProjection* Item = Projection.Items.FindByPredicate(
			[ContainerId, SlotIndex](const FCodeBBodyContainerItemProjection& Candidate)
			{
				return Candidate.ParentContainerId == ContainerId && Candidate.SlotIndex == SlotIndex;
			});
		if (!Item) return true;
		OutState = Item->Visibility == ECodeBBodyContainerVisibility::Hidden
			? ECodeBP3CellState::Hidden
			: Item->Visibility == ECodeBBodyContainerVisibility::Searching
				? ECodeBP3CellState::Searching : ECodeBP3CellState::Revealed;
		OutLocator.TargetKind = ECodeBP3SearchTargetKind::BodyContainer;
		OutLocator.OwnerId = Projection.OwnerId;
		OutLocator.RunInstanceId = Projection.RunInstanceId;
		OutLocator.TargetId = Projection.BodyTargetId;
		OutLocator.ContainerId = ContainerId;
		OutLocator.SlotIndex = SlotIndex;
		OutLocator.TargetRevision = Projection.Revision;
		OutLocator.ActiveActionId = Projection.ActiveActionId;
		return true;
	}
	return false;
}

bool UCodeBP3UIHostSubsystem::IsExternalTargetContainer(const FGuid& ContainerId) const
{
	return IsNormalContainerPresentation(ContainerId)
		|| IsBodyContainerPresentation(ContainerId)
		|| IsWorldDropPresentation(ContainerId);
}

void UCodeBP3UIHostSubsystem::PopulateAddressContext(FCodeBP3SlotAddress& Address) const
{
	const bool bExternal = IsExternalTargetContainer(Address.ContainerId);
	if (WorkspacePresentation.IsSet())
	{
		const FCodeBP3InventoryWorkspaceContext& Context = WorkspacePresentation->Context;
		Address.Scope = bExternal
			? ECodeBP3InventoryScope::ExternalTarget
			: Context.IsOutOfRaidP5()
				? (Controller.IsValid() && Address.ContainerId == Controller->GetProjection().WarehouseContainerId
					? ECodeBP3InventoryScope::OutOfRaidWarehouse
					: ECodeBP3InventoryScope::OutOfRaidPlayer)
				: ECodeBP3InventoryScope::InRunPlayer;
		Address.OwnerId = Context.OwnerId;
		Address.RunInstanceId = Context.RunInstanceId;
		return;
	}
	Address.Scope = bExternal
		? ECodeBP3InventoryScope::ExternalTarget : ECodeBP3InventoryScope::InRunPlayer;
	if (HotbarPresentation.IsSet())
	{
		Address.OwnerId = HotbarPresentation->OwnerId;
		Address.RunInstanceId = HotbarPresentation->RunInstanceId;
	}
	else if (NormalContainerPresentation.IsSet())
	{
		Address.OwnerId = NormalContainerPresentation->Projection.OwnerId;
		Address.RunInstanceId = NormalContainerPresentation->Projection.RunInstanceId;
	}
	else if (BodyContainerPresentation.IsSet())
	{
		Address.OwnerId = BodyContainerPresentation->Projection.OwnerId;
		Address.RunInstanceId = BodyContainerPresentation->Projection.RunInstanceId;
	}
}

void UCodeBP3UIHostSubsystem::PopulateTransferContext(FCodeBP4DragPayload& Payload) const
{
	PopulateAddressContext(Payload.Source);
	Payload.SourceScope = Payload.Source.Scope;
	if (WorkspacePresentation.IsSet())
	{
		Payload.OwnerId = WorkspacePresentation->Context.OwnerId;
		Payload.RunInstanceId = WorkspacePresentation->Context.RunInstanceId;
		if (WorkspacePresentation->Context.ActiveDestinationContainerId.IsSet())
		{
			Payload.QuickTransferActivePlayerContainerId =
				WorkspacePresentation->Context.ActiveDestinationContainerId.GetValue();
			Payload.ActivePlayerChildOpenGeneration =
				WorkspacePresentation->Context.ActiveDestinationOpenGeneration;
		}
	}
	else if (HotbarPresentation.IsSet())
	{
		Payload.OwnerId = HotbarPresentation->OwnerId;
		Payload.RunInstanceId = HotbarPresentation->RunInstanceId;
	}
	else if (NormalContainerPresentation.IsSet())
	{
		Payload.OwnerId = NormalContainerPresentation->Projection.OwnerId;
		Payload.RunInstanceId = NormalContainerPresentation->Projection.RunInstanceId;
	}
	else if (BodyContainerPresentation.IsSet())
	{
		Payload.OwnerId = BodyContainerPresentation->Projection.OwnerId;
		Payload.RunInstanceId = BodyContainerPresentation->Projection.RunInstanceId;
	}
	if (WorldDropPresentation.IsSet())
	{
		Payload.WorldDropId = WorldDropPresentation->WorldDropId;
		Payload.WorldDropOrdinal = WorldDropPresentation->Ordinal;
		Payload.WorldDropRecordRevision = WorldDropPresentation->RecordRevision;
		Payload.WorldDropTargetOpenGeneration = WorldDropPresentation->TargetOpenGeneration;
		Payload.WorldDropMapRoute = WorldDropPresentation->MapRoute;
		Payload.GraphIdentity = WorldDropPresentation->WorldDropId;
	}
}

bool UCodeBP3UIHostSubsystem::CreateSplitDraft(
	const FCodeBP3SlotAddress& Source,
	const int32 RequestedQuantity,
	FString& OutError)
{
	OutError.Reset();
	if (!CanWriteWorkspace(OutError) || !Controller.IsValid())
	{
		SplitDraft.Reset();
		return false;
	}
	FCodeBP3SlotAddress AuthoritativeSource;
	if (!Controller->MakeAddress(Source.ContainerId, Source.SlotIndex, AuthoritativeSource)
		|| !AuthoritativeSource.IsRevealed() || AuthoritativeSource.ItemId != Source.ItemId)
	{
		OutError = TEXT("拆分来源已变化或当前不可见。");
		SplitDraft.Reset();
		return false;
	}
	PopulateAddressContext(AuthoritativeSource);
	const bool bWorldPickup = IsWorldDropPresentation(AuthoritativeSource.ContainerId);
	if (bWorldPickup && (!WorldDropPresentation.IsSet()
		|| !WorldDropPresentation->WorldDropId.IsValid()
		|| WorldDropPresentation->TargetContainerId != AuthoritativeSource.ContainerId))
	{
		OutError = TEXT("地面数量拾回缺少精确 P14 record/root 身份。");
		SplitDraft.Reset();
		return false;
	}
	if (!Controller->ValidateSplitSource(AuthoritativeSource, RequestedQuantity, OutError))
	{
		SplitDraft.Reset();
		return false;
	}

	FCodeBP4DragPayload IdentityProbe;
	IdentityProbe.Source = AuthoritativeSource;
	PopulateTransferContext(IdentityProbe);
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	FGuid GraphIdentity;
	if (bWorldPickup) GraphIdentity = WorldDropPresentation->WorldDropId;
	else if (NormalContainerPresentation.IsSet()) GraphIdentity = NormalContainerPresentation->TargetContainerId;
	else if (BodyContainerPresentation.IsSet()) GraphIdentity = BodyContainerPresentation->TargetContainerId;
	else if (WorkspacePresentation.IsSet() && WorkspacePresentation->Context.IsOutOfRaidP5())
		GraphIdentity = Projection.WarehouseContainerId;
	else GraphIdentity = Projection.BasicContainerId;
	if (!GraphIdentity.IsValid())
	{
		OutError = TEXT("当前共享工作台没有稳定图身份，拆分草稿未创建。");
		SplitDraft.Reset();
		return false;
	}

	FCodeBP3SplitDraft Draft;
	Draft.Kind = bWorldPickup
		? ECodeBP3QuantityDraftKind::WorldPickup
		: ECodeBP3QuantityDraftKind::PlayerSplit;
	Draft.WorkspaceScope = WorkspacePresentation.IsSet()
		? WorkspacePresentation->Context.Scope
		: (Controller->IsActiveRunBacked() ? ECodeBP3WorkspaceScope::InRunP6 : ECodeBP3WorkspaceScope::OutOfRaidP5);
	Draft.SourceScope = AuthoritativeSource.Scope;
	Draft.OwnerId = IdentityProbe.OwnerId;
	Draft.RunInstanceId = IdentityProbe.RunInstanceId;
	Draft.GraphIdentity = GraphIdentity;
	Draft.WorldDropId = bWorldPickup ? WorldDropPresentation->WorldDropId : FGuid();
	Draft.WorldDropOrdinal = bWorldPickup ? WorldDropPresentation->Ordinal : 0;
	Draft.WorldDropRecordRevision = bWorldPickup ? WorldDropPresentation->RecordRevision : INDEX_NONE;
	Draft.WorldDropTargetOpenGeneration = bWorldPickup ? WorldDropPresentation->TargetOpenGeneration : 0;
	Draft.WorldDropMapRoute = bWorldPickup ? WorldDropPresentation->MapRoute : NAME_None;
	Draft.Source = AuthoritativeSource;
	Draft.SourceItemId = AuthoritativeSource.ItemId;
	Draft.ExpectedRevision = Controller->GetProjection().Revision;
	Draft.RequestedQuantity = RequestedQuantity;
	if (!Draft.IsValid() || !Draft.OwnerId.IsValid()
		|| (Draft.WorkspaceScope == ECodeBP3WorkspaceScope::InRunP6 && !Draft.RunInstanceId.IsValid()))
	{
		OutError = TEXT("拆分草稿缺少 Owner／Run／图身份，未创建。");
		SplitDraft.Reset();
		return false;
	}
	SplitDraft = MoveTemp(Draft);
	Controller->SetP4Feedback(bWorldPickup
		? FString::Printf(TEXT("已确认从地面拾回 %d 个；请拖到明确空格或兼容未满玩家堆叠，Drop 前零写入。"),
			RequestedQuantity)
		: FString::Printf(TEXT("已准备 %d 个；请拖动同一来源堆到明确空格或兼容未满堆叠。"),
			RequestedQuantity));
	return true;
}

bool UCodeBP3UIHostSubsystem::BeginInventoryDrag(
	FCodeBP4InteractionController& Interaction,
	const FCodeBP3SlotAddress& Source,
	FCodeBP4DragPayload& OutPayload,
	FString& OutError)
{
	OutError.Reset();
	if (!SplitDraft.IsSet())
	{
		return Interaction.BeginDrag(Source, OutPayload);
	}
	const FCodeBP3SplitDraft Draft = SplitDraft.GetValue();
	if (Draft.Source.ContainerId != Source.ContainerId
		|| Draft.Source.SlotIndex != Source.SlotIndex
		|| Draft.SourceItemId != Source.ItemId)
	{
		CancelSplitDraft(TEXT("开始了另一项拖拽，旧拆分草稿已清除"));
		return Interaction.BeginDrag(Source, OutPayload);
	}
	SplitDraft.Reset();
	if (!CanWriteWorkspace(OutError) || !Controller.IsValid())
	{
		return false;
	}
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	const bool bWorkspaceIdentityCurrent = !WorkspacePresentation.IsSet()
		|| (Draft.OwnerId == WorkspacePresentation->Context.OwnerId
			&& Draft.RunInstanceId == WorkspacePresentation->Context.RunInstanceId);
	const bool bWorldIdentityCurrent = Draft.Kind != ECodeBP3QuantityDraftKind::WorldPickup
		|| (WorldDropPresentation.IsSet()
			&& Draft.WorldDropId == WorldDropPresentation->WorldDropId
			&& Draft.WorldDropOrdinal == WorldDropPresentation->Ordinal
			&& Draft.WorldDropRecordRevision == WorldDropPresentation->RecordRevision
			&& Draft.WorldDropTargetOpenGeneration == WorldDropPresentation->TargetOpenGeneration
			&& Draft.WorldDropMapRoute == WorldDropPresentation->MapRoute
			&& Draft.Source.ContainerId == WorldDropPresentation->TargetContainerId
			&& (!WorkspacePresentation.IsSet()
				|| WorkspacePresentation->Context.SessionRevision == Projection.Revision));
	if (!bWorkspaceIdentityCurrent || !bWorldIdentityCurrent
		|| Draft.ExpectedRevision != Controller->GetProjection().Revision
		|| !Controller->ValidateSplitSource(Source, Draft.RequestedQuantity, OutError))
	{
		if (OutError.IsEmpty()) OutError = TEXT("拆分草稿已过期，未开始拖拽。");
		return false;
	}
	const bool bStarted = Interaction.BeginSplitDrag(
		Source, Draft.RequestedQuantity, OutPayload, Draft.Kind);
	if (bStarted)
	{
		OutPayload.GraphIdentity = Draft.GraphIdentity;
		OutPayload.WorldDropId = Draft.WorldDropId;
		OutPayload.WorldDropOrdinal = Draft.WorldDropOrdinal;
		OutPayload.WorldDropRecordRevision = Draft.WorldDropRecordRevision;
		OutPayload.WorldDropTargetOpenGeneration = Draft.WorldDropTargetOpenGeneration;
		OutPayload.WorldDropMapRoute = Draft.WorldDropMapRoute;
	}
	return bStarted;
}

bool UCodeBP3UIHostSubsystem::CancelSplitDraft(const FString& Reason)
{
	if (!SplitDraft.IsSet()) return false;
	SplitDraft.Reset();
	if (Controller.IsValid()) Controller->SetP4Feedback(Reason);
	return true;
}

bool UCodeBP3UIHostSubsystem::CanWriteWorkspace(FString& OutError)
{
	OutError.Reset();
	if (!Controller.IsValid() || !Controller->IsOpen())
	{
		SplitDraft.Reset();
		OutError = TEXT("物品工作台已关闭。");
		return false;
	}
	if (!WorkspacePresentation.IsSet())
	{
		if (SplitDraft.IsSet())
		{
			FString SplitError;
			const FCodeBP3SplitDraft& Draft = SplitDraft.GetValue();
			if (Draft.ExpectedRevision != Controller->GetProjection().Revision
				|| !Controller->ValidateSplitSource(Draft.Source, Draft.RequestedQuantity, SplitError))
			{
				SplitDraft.Reset();
			}
		}
		return true;
	}
	FCodeBP3InventoryWorkspaceContext& Context = WorkspacePresentation->Context;
	if (WorkspacePresentation->ResolveWriteGate)
	{
		Context.WriteGate = WorkspacePresentation->ResolveWriteGate();
	}
	if (WorkspacePresentation->ResolveSessionRevision)
	{
		Context.SessionRevision = WorkspacePresentation->ResolveSessionRevision();
	}
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	auto IsCurrentAddress = [&Projection](const FCodeBP3SlotAddress& Address)
	{
		const FCodeBP2ContainerView* Container = Projection.Containers.FindByPredicate(
			[&Address](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Address.ContainerId; });
		const FCodeBP2SlotView* Slot = Container ? Container->Slots.FindByPredicate(
			[&Address](const FCodeBP2SlotView& Value) { return Value.SlotIndex == Address.SlotIndex; }) : nullptr;
		return Slot && (!Address.ItemId.IsValid() || Slot->ItemId == Address.ItemId);
	};
	if (Context.HoveredAddress.IsSet() && !IsCurrentAddress(Context.HoveredAddress.GetValue()))
	{
		Context.HoveredAddress.Reset();
	}
	if (Context.SelectedAddress.IsSet() && !IsCurrentAddress(Context.SelectedAddress.GetValue()))
	{
		Context.SelectedAddress.Reset();
	}
	if (Context.ActiveDestinationContainerId.IsSet())
	{
		const FGuid ActiveId = Context.ActiveDestinationContainerId.GetValue();
		const FCodeBP2ContainerView* Active = Projection.Containers.FindByPredicate(
			[ActiveId](const FCodeBP2ContainerView& Value) { return Value.ContainerId == ActiveId; });
		if (!Active || (Active->Role != FName(TEXT("QuickSpatial"))
			&& Active->Role != FName(TEXT("PouchInternal"))))
		{
			Context.ActiveDestinationContainerId.Reset();
			Context.ActiveDestinationOpenGeneration = 0;
		}
	}
	else
	{
		Context.ActiveDestinationOpenGeneration = 0;
	}
	if (SplitDraft.IsSet())
	{
		FString SplitError;
		const FCodeBP3SplitDraft& Draft = SplitDraft.GetValue();
		if (Draft.OwnerId != Context.OwnerId || Draft.RunInstanceId != Context.RunInstanceId
			|| Draft.ExpectedRevision != Projection.Revision
			|| (Draft.Kind == ECodeBP3QuantityDraftKind::WorldPickup
				&& (Context.SessionRevision != Projection.Revision
					|| !WorldDropPresentation.IsSet()
					|| Draft.WorldDropId != WorldDropPresentation->WorldDropId
					|| Draft.WorldDropOrdinal != WorldDropPresentation->Ordinal
					|| Draft.WorldDropRecordRevision != WorldDropPresentation->RecordRevision
					|| Draft.WorldDropTargetOpenGeneration != WorldDropPresentation->TargetOpenGeneration
					|| Draft.WorldDropMapRoute != WorldDropPresentation->MapRoute
					|| Draft.Source.ContainerId != WorldDropPresentation->TargetContainerId))
			|| !Controller->ValidateSplitSource(Draft.Source, Draft.RequestedQuantity, SplitError))
		{
			SplitDraft.Reset();
		}
	}
	if (Context.IsOutOfRaidP5())
	{
		if (Context.WriteGate == ECodeBP3WorkspaceWriteGate::AtSect)
		{
			if (Context.OwnerId.IsValid() && !Context.RunInstanceId.IsValid())
			{
				return true;
			}
			OutError = TEXT("P5 Owner 或 Run scope 无效；未提交写入。");
			SplitDraft.Reset();
			return false;
		}
		OutError = Context.WriteGate == ECodeBP3WorkspaceWriteGate::StartAttemptPending
			? TEXT("StartAttemptPending：出战尝试处理中，P5 写入暂时拒绝。")
			: TEXT("当前不在 AtSect；P5 工作台只读且不会写入物品位置。");
		SplitDraft.Reset();
		return false;
	}
	if (Context.IsInRun())
	{
		if (Context.WriteGate == ECodeBP3WorkspaceWriteGate::InRun
			&& Context.OwnerId.IsValid() && Context.RunInstanceId.IsValid())
		{
			return true;
		}
		OutError = TEXT("活动 Run 身份或生命周期已变化；当前工作台写入被拒绝。");
		SplitDraft.Reset();
		return false;
	}
	return true;
}

bool UCodeBP3UIHostSubsystem::ValidateTransferContext(
	const FCodeBP4DragPayload& Payload,
	FString& OutError)
{
	if (!Payload.IsValid() || !Controller.IsValid())
	{
		OutError = TEXT("物品移动描述无效。");
		return false;
	}
	if (!CanWriteWorkspace(OutError))
	{
		return false;
	}
	if (Payload.bSplitIntent)
	{
		const FCodeBP2Projection& Projection = Controller->GetProjection();
		FGuid ExpectedGraphIdentity;
		if (Payload.QuantityDraftKind == ECodeBP3QuantityDraftKind::WorldPickup)
		{
			if (!WorldDropPresentation.IsSet()
				|| Payload.WorldDropId != WorldDropPresentation->WorldDropId
				|| Payload.WorldDropOrdinal != WorldDropPresentation->Ordinal
				|| Payload.WorldDropRecordRevision != WorldDropPresentation->RecordRevision
				|| Payload.WorldDropTargetOpenGeneration != WorldDropPresentation->TargetOpenGeneration
				|| Payload.WorldDropMapRoute != WorldDropPresentation->MapRoute
				|| Payload.Source.ContainerId != WorldDropPresentation->TargetContainerId
				|| Payload.SourceScope != ECodeBP3InventoryScope::ExternalTarget
				|| (WorkspacePresentation.IsSet()
					&& WorkspacePresentation->Context.SessionRevision != Projection.Revision))
			{
				OutError = TEXT("WorldPickup 数量 payload 的 P14 record/root 身份已变化；未写入。");
				return false;
			}
			ExpectedGraphIdentity = WorldDropPresentation->WorldDropId;
		}
		else if (Payload.QuantityDraftKind != ECodeBP3QuantityDraftKind::PlayerSplit)
		{
			OutError = TEXT("数量 payload 缺少明确 PlayerSplit／WorldPickup 种类；未写入。");
			return false;
		}
		else if (NormalContainerPresentation.IsSet()) ExpectedGraphIdentity = NormalContainerPresentation->TargetContainerId;
		else if (BodyContainerPresentation.IsSet()) ExpectedGraphIdentity = BodyContainerPresentation->TargetContainerId;
		else if (WorkspacePresentation.IsSet() && WorkspacePresentation->Context.IsOutOfRaidP5())
			ExpectedGraphIdentity = Projection.WarehouseContainerId;
		else ExpectedGraphIdentity = Projection.BasicContainerId;
		if (!ExpectedGraphIdentity.IsValid() || Payload.GraphIdentity != ExpectedGraphIdentity)
		{
			OutError = TEXT("数量草稿的仓库／Run／外部目标图身份已变化；未写入。");
			return false;
		}
	}
	if (!WorkspacePresentation.IsSet())
	{
		FCodeBP3SlotAddress ExpectedSource = Payload.Source;
		PopulateAddressContext(ExpectedSource);
		if (Payload.ExpectedRevision != Controller->GetProjection().Revision
			|| Payload.SourceScope != ExpectedSource.Scope
			|| (ExpectedSource.OwnerId.IsValid() && (Payload.OwnerId != ExpectedSource.OwnerId
				|| Payload.Source.OwnerId != ExpectedSource.OwnerId))
			|| (ExpectedSource.RunInstanceId.IsValid() && (Payload.RunInstanceId != ExpectedSource.RunInstanceId
				|| Payload.Source.RunInstanceId != ExpectedSource.RunInstanceId)))
		{
			OutError = TEXT("Owner／Run／scope／revision 已变化；stale payload 未写入。");
			return false;
		}
		return true;
	}
	FCodeBP3SlotAddress ExpectedSource = Payload.Source;
	PopulateAddressContext(ExpectedSource);
	const FCodeBP3InventoryWorkspaceContext& Context = WorkspacePresentation->Context;
	const bool bPayloadCarriesActiveChild = Payload.QuickTransferActivePlayerContainerId.IsValid();
	const bool bActiveChildIdentityCurrent = bPayloadCarriesActiveChild
		? (Context.ActiveDestinationContainerId.IsSet()
			&& Context.ActiveDestinationContainerId.GetValue() == Payload.QuickTransferActivePlayerContainerId
			&& Context.ActiveDestinationOpenGeneration != 0
			&& Context.ActiveDestinationOpenGeneration == Payload.ActivePlayerChildOpenGeneration)
		: Payload.ActivePlayerChildOpenGeneration == 0;
	if (Payload.OwnerId != Context.OwnerId
		|| Payload.RunInstanceId != Context.RunInstanceId
		|| Payload.SourceScope != ExpectedSource.Scope
		|| Payload.Source.OwnerId != Context.OwnerId
		|| Payload.Source.RunInstanceId != Context.RunInstanceId
		|| Payload.ExpectedRevision != Controller->GetProjection().Revision
		|| !bActiveChildIdentityCurrent)
	{
		OutError = TEXT("Owner／Run／scope／revision 已变化；stale payload 未写入。");
		return false;
	}
	return true;
}

bool UCodeBP3UIHostSubsystem::IsP29PlayerQuickTransferSourceContainer(const FGuid& ContainerId) const
{
	if (!ContainerId.IsValid() || !Controller.IsValid() || !WorldDropPresentation.IsSet()
		|| !WorkspacePresentation.IsSet() || !WorkspacePresentation->Context.IsInRun())
	{
		return false;
	}
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	if (ContainerId == Projection.BasicContainerId)
	{
		return true;
	}
	return IsCurrentActiveP17ChildContainer(ContainerId);
}

bool UCodeBP3UIHostSubsystem::IsCurrentActiveP17ChildContainer(const FGuid& ContainerId) const
{
	if (!ContainerId.IsValid() || !Controller.IsValid() || !WorkspacePresentation.IsSet()
		|| !WorkspacePresentation->Context.ActiveDestinationContainerId.IsSet()
		|| WorkspacePresentation->Context.ActiveDestinationContainerId.GetValue() != ContainerId
		|| WorkspacePresentation->Context.ActiveDestinationOpenGeneration == 0)
	{
		return false;
	}
	const FCodeBP2ContainerView* Container = Controller->GetProjection().Containers.FindByPredicate(
		[ContainerId](const FCodeBP2ContainerView& Value) { return Value.ContainerId == ContainerId; });
	return Container && (Container->Role == FName(TEXT("QuickSpatial"))
		|| Container->Role == FName(TEXT("PouchInternal")));
}

bool UCodeBP3UIHostSubsystem::IsP34StandardEquipmentWorldDropSource(const FGuid& ContainerId) const
{
	if (!IsWorldDropPresentation(ContainerId) || !WorldDropPresentation.IsSet())
	{
		return false;
	}
	const FString& Provenance = WorldDropPresentation->Provenance;
	return Provenance == TEXT("P32.AcceptedGroundDrop.StandardEquipment")
		|| Provenance == TEXT("P33.AcceptedGroundDrop.BaseQuickStandardEquipment");
}

bool UCodeBP3UIHostSubsystem::IsP35ChildStandardEquipmentWorldDropSource(const FGuid& ContainerId) const
{
	return IsWorldDropPresentation(ContainerId) && WorldDropPresentation.IsSet()
		&& WorldDropPresentation->Provenance == TEXT("P35.AcceptedGroundDrop.ChildStandardEquipment");
}

bool UCodeBP3UIHostSubsystem::ValidateWorldDropTransferContext(
	const FCodeBP4DragPayload& Payload,
	const FCodeBP3SlotAddress& Target,
	FString& OutError) const
{
	OutError.Reset();
	if (!Controller.IsValid() || !WorldDropPresentation.IsSet() || !WorkspacePresentation.IsSet())
	{
		OutError = TEXT("P29 WorldDrop 工作台身份不可用。");
		return false;
	}
	const FCodeBP3WorldDropPresentation& WorldDrop = WorldDropPresentation.GetValue();
	const FCodeBP3InventoryWorkspaceContext& Context = WorkspacePresentation->Context;
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	const bool bSourceWorld = Payload.Source.ContainerId == WorldDrop.TargetContainerId;
	const bool bTargetWorld = Target.ContainerId == WorldDrop.TargetContainerId;
	if (bSourceWorld == bTargetWorld || !Context.IsInRun()
		|| !WorldDrop.OwnerId.IsValid() || !WorldDrop.RunInstanceId.IsValid()
		|| !WorldDrop.WorldDropId.IsValid() || !WorldDrop.TargetContainerId.IsValid()
		|| WorldDrop.Ordinal < 1 || !WorldDrop.RootItemId.IsValid()
		|| WorldDrop.RecordRevision < 1 || WorldDrop.TargetOpenGeneration == 0
		|| WorldDrop.MapRoute.IsNone()
		|| Payload.OwnerId != WorldDrop.OwnerId || Payload.RunInstanceId != WorldDrop.RunInstanceId
		|| Payload.Source.OwnerId != WorldDrop.OwnerId || Payload.Source.RunInstanceId != WorldDrop.RunInstanceId
		|| Context.OwnerId != WorldDrop.OwnerId || Context.RunInstanceId != WorldDrop.RunInstanceId
		|| Context.SessionRevision != Projection.Revision || Payload.ExpectedRevision != Projection.Revision
		|| Payload.WorldDropId != WorldDrop.WorldDropId
		|| Payload.WorldDropOrdinal != WorldDrop.Ordinal
		|| Payload.WorldDropRecordRevision != WorldDrop.RecordRevision
		|| Payload.WorldDropTargetOpenGeneration != WorldDrop.TargetOpenGeneration
		|| Payload.WorldDropMapRoute != WorldDrop.MapRoute
		|| Payload.GraphIdentity != WorldDrop.WorldDropId)
	{
		OutError = TEXT("P29 WorldDrop Owner／Run／record／revision 身份已变化；未写入。");
		return false;
	}
	const FCodeBP3SlotAddress& WorldAddress = bSourceWorld ? Payload.Source : Target;
	const FCodeBP2ContainerView* WorldContainer = Projection.Containers.FindByPredicate(
		[&WorldDrop](const FCodeBP2ContainerView& Value)
		{
			return Value.ContainerId == WorldDrop.TargetContainerId;
		});
	const FCodeBP2SlotView* WorldSlot = WorldContainer ? WorldContainer->Slots.FindByPredicate(
		[](const FCodeBP2SlotView& Value) { return Value.SlotIndex == 0; }) : nullptr;
	if (!WorldContainer || WorldContainer->Role != FName(TEXT("WorldDropTarget"))
		|| WorldContainer->Capacity != 1 || !WorldSlot || !WorldSlot->bOccupied
		|| WorldSlot->ItemId != WorldDrop.RootItemId
		|| WorldAddress.SlotIndex != 0 || !WorldAddress.IsRevealed()
		|| WorldAddress.ItemId != WorldDrop.RootItemId
		|| WorldAddress.OwnerId != WorldDrop.OwnerId
		|| WorldAddress.RunInstanceId != WorldDrop.RunInstanceId
		|| WorldAddress.Scope != ECodeBP3InventoryScope::ExternalTarget)
	{
		OutError = TEXT("P29 WorldDrop exact record/container/root 地址已变化；未写入。");
		return false;
	}
	return true;
}

bool UCodeBP3UIHostSubsystem::IsOutOfRaidWorkspace() const
{
	return WorkspacePresentation.IsSet()
		&& WorkspacePresentation->Context.IsOutOfRaidP5();
}

bool UCodeBP3UIHostSubsystem::ActivateQuickTransferDestination(const FCodeBP3SlotAddress& Address)
{
	const bool bSupportedWorkspace = IsOutOfRaidWorkspace()
		|| (WorkspacePresentation.IsSet() && WorkspacePresentation->Context.IsInRun()
			&& (WorldDropPresentation.IsSet() || GroundDropPresentation.IsSet()));
	if (!bSupportedWorkspace || !Controller.IsValid() || !Address.IsValid())
	{
		return false;
	}
	const FCodeBP2ContainerView* Container = Controller->GetProjection().Containers.FindByPredicate(
		[&Address](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Address.ContainerId; });
	if (!Container || (Container->Role != FName(TEXT("QuickSpatial"))
		&& Container->Role != FName(TEXT("PouchInternal"))))
	{
		return false;
	}
	WorkspacePresentation->Context.ActiveDestinationContainerId = Container->ContainerId;
	if (NextActiveDestinationOpenGeneration == 0) NextActiveDestinationOpenGeneration = 1;
	WorkspacePresentation->Context.ActiveDestinationOpenGeneration = NextActiveDestinationOpenGeneration++;
	return true;
}

void UCodeBP3UIHostSubsystem::UpdateWorkspaceHover(
	const FCodeBP3SlotAddress& Address,
	const bool bHovered)
{
	if (!WorkspacePresentation.IsSet()) return;
	if (bHovered)
	{
		WorkspacePresentation->Context.HoveredAddress = Address;
	}
	else if (WorkspacePresentation->Context.HoveredAddress.IsSet()
		&& WorkspacePresentation->Context.HoveredAddress->ContainerId == Address.ContainerId
		&& WorkspacePresentation->Context.HoveredAddress->SlotIndex == Address.SlotIndex)
	{
		WorkspacePresentation->Context.HoveredAddress.Reset();
	}
}

void UCodeBP3UIHostSubsystem::UpdateWorkspaceSelection(const FCodeBP3SlotAddress& Address)
{
	if (WorkspacePresentation.IsSet())
	{
		WorkspacePresentation->Context.SelectedAddress = Address;
	}
}

void UCodeBP3UIHostSubsystem::UpdateWorkspaceScroll(
	const float PlayerOffset,
	const float TargetOffset)
{
	if (!WorkspacePresentation.IsSet()) return;
	WorkspacePresentation->Context.PlayerScrollOffset = FMath::Max(0.0f, PlayerOffset);
	WorkspacePresentation->Context.TargetScrollOffset = FMath::Max(0.0f, TargetOffset);
}

void UCodeBP3UIHostSubsystem::ClearWorkspaceTransientState()
{
	SplitDraft.Reset();
	if (!WorkspacePresentation.IsSet()) return;
	WorkspacePresentation->Context.ActiveDestinationContainerId.Reset();
	WorkspacePresentation->Context.ActiveDestinationOpenGeneration = 0;
	WorkspacePresentation->Context.HoveredAddress.Reset();
	WorkspacePresentation->Context.SelectedAddress.Reset();
}

void UCodeBP3UIHostSubsystem::UpdateBodyContainerProjection(
	const FCodeBBodyContainerProjection& Projection)
{
	if (!BodyContainerPresentation.IsSet()
		|| Projection.ContainerId != BodyContainerPresentation->TargetContainerId)
	{
		return;
	}
	BodyContainerPresentation->Projection = Projection;
	RefreshActivePage();
}

bool UCodeBP3UIHostSubsystem::RequestGroundDrop(
	const FCodeBP4DragPayload& Payload,
	FString& OutError)
{
	OutError.Reset();
	if (!GroundDropPresentation.IsSet() || !GroundDropPresentation->RequestDrop || !Payload.IsValid())
	{
		OutError = TEXT("地面丢弃入口当前不可用。");
		return false;
	}
	if (!ValidateTransferContext(Payload, OutError))
	{
		return false;
	}
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	const FCodeBP2ContainerView* SourceContainer = Projection.Containers.FindByPredicate(
		[&Payload](const FCodeBP2ContainerView& Value) { return Value.ContainerId == Payload.Source.ContainerId; });
	const FCodeBP2SlotView* SourceSlot = SourceContainer ? SourceContainer->Slots.FindByPredicate(
		[&Payload](const FCodeBP2SlotView& Value) { return Value.SlotIndex == Payload.Source.SlotIndex; }) : nullptr;
	const bool bChildStandardRoot = SourceContainer && SourceSlot
		&& (SourceContainer->Role == FName(TEXT("QuickSpatial"))
			|| SourceContainer->Role == FName(TEXT("PouchInternal")))
		&& !SourceSlot->ChildContainerId.IsValid() && !SourceSlot->bStackable
		&& SourceSlot->MaxStack == 1 && SourceSlot->Quantity == 1
		&& ((SourceSlot->ItemType == ECodeBItemType::Weapon && SourceSlot->EquipSlot == ECodeBEquipSlot::Weapon)
			|| (SourceSlot->ItemType == ECodeBItemType::Armor && SourceSlot->EquipSlot == ECodeBEquipSlot::Armor)
			|| (SourceSlot->ItemType == ECodeBItemType::Accessory && SourceSlot->EquipSlot == ECodeBEquipSlot::Accessory));
	if (bChildStandardRoot && (!IsCurrentActiveP17ChildContainer(Payload.Source.ContainerId)
		|| Payload.QuickTransferActivePlayerContainerId != Payload.Source.ContainerId
		|| Payload.ActivePlayerChildOpenGeneration == 0))
	{
		OutError = TEXT("P35 只接受当前已打开 P17 child 内的 direct standard root。");
		return false;
	}
	const bool bCommitted = GroundDropPresentation->RequestDrop(Payload, OutError);
	if (bCommitted && Controller.IsValid())
	{
		FString ProjectionError;
		if (!Controller->RefreshProjection(&ProjectionError))
		{
			OutError = ProjectionError;
			return false;
		}
	}
	return bCommitted;
}

bool UCodeBP3UIHostSubsystem::RequestHotbarBindFromAddress(
	const FCodeBP3SlotAddress& Selected,
	const int32 SlotIndex,
	FString& OutError)
{
	OutError.Reset();
	if (!HotbarPresentation.IsSet() || !HotbarPresentation->Projection.bEditable
		|| !HotbarPresentation->OwnerId.IsValid()
		|| !HotbarPresentation->Bind || !Controller.IsValid())
	{
		OutError = TEXT("Shift+数字绑定当前不可用或只读。");
		return false;
	}
	const bool bActiveRunBinding = HotbarPresentation->Projection.bActiveRunScope;
	if ((bActiveRunBinding && (!Controller->IsActiveRunBacked()
			|| !HotbarPresentation->RunInstanceId.IsValid()))
		|| (!bActiveRunBinding && (Controller->IsActiveRunBacked()
			|| HotbarPresentation->RunInstanceId.IsValid()
			|| !IsOutOfRaidWorkspace())))
	{
		OutError = TEXT("P5／P6 快捷栏 scope 或 Run 身份不匹配。");
		return false;
	}
	if (!CanWriteWorkspace(OutError))
	{
		return false;
	}
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	const FCodeBP2ContainerView* BasicContainer = Projection.Containers.FindByPredicate(
		[&Projection](const FCodeBP2ContainerView& Candidate)
		{ return Candidate.ContainerId == Projection.BasicContainerId; });
	const FCodeBP2SlotView* Slot = BasicContainer
		? BasicContainer->Slots.FindByPredicate([&Selected](const FCodeBP2SlotView& Candidate)
			{ return Candidate.SlotIndex == Selected.SlotIndex; })
		: nullptr;
	if (!Slot || Selected.ContainerId != Projection.BasicContainerId)
	{
		OutError = TEXT("快捷栏只能引用基础快捷物品区中的物品。");
		return false;
	}
	if (!Selected.IsRevealed() || !Slot->bOccupied || Slot->ItemId != Selected.ItemId
		|| Slot->Quantity <= 0 || !Slot->bQuickUsable)
	{
		OutError = TEXT("当前选择不是可绑定的 QuickUsable 基础快捷物品。");
		return false;
	}
	FCodeBHotbarProjection UpdatedProjection;
	if (!HotbarPresentation->Bind(Selected.ItemId, SlotIndex, UpdatedProjection, OutError))
	{
		return false;
	}
	HotbarPresentation->Projection = MoveTemp(UpdatedProjection);
	return true;
}

void UCodeBP3UIHostSubsystem::RefreshHotbarProjection()
{
	if (!HotbarPresentation.IsSet() || !HotbarPresentation->Refresh)
	{
		return;
	}
	FCodeBHotbarProjection UpdatedProjection;
	FString IgnoredError;
	if (HotbarPresentation->Refresh(UpdatedProjection, IgnoredError))
	{
		HotbarPresentation->Projection = MoveTemp(UpdatedProjection);
	}
}

void UCodeBP3UIHostSubsystem::ResetDevelopmentFixture()
{
	if (!Controller.IsValid())
	{
		Controller = MakeUnique<FCodeBP3UIController>();
	}
	const bool bWasOpen = Controller->IsOpen();
	Controller->Shutdown();
	if (bWasOpen || ActiveWidget.IsValid())
	{
		Controller->Open();
		RefreshActivePage();
	}
}

void UCodeBP3UIHostSubsystem::RefreshActivePage()
{
	if (ActiveWidget.IsValid())
	{
		ActiveWidget->RefreshFromController();
	}
}

void UCodeBP3UIHostSubsystem::SetP4PreviewCapture(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target)
{
	if (ActiveWidget.IsValid())
	{
		ActiveWidget->SetP4PreviewCapture(Payload, Target);
	}
}

bool UCodeBP3UIHostSubsystem::IsHostEnabled() const
{
	return Controller.IsValid() && Controller->IsOpen();
}
