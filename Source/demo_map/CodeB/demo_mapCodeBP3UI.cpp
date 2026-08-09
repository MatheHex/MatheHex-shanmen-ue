// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeB/demo_mapCodeBP3UI.h"

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
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Address.bOccupied && OwnerWidget.IsValid())
	{
		OwnerWidget->BeginP4PointerGesture(Address);
		// This threshold registration is owned by the same cell that owns MouseUp,
		// double-click, enter/leave and drop. It performs no item write by itself.
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && Address.bOccupied && OwnerWidget.IsValid())
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
		OwnerWidget->TraceP4Input(TEXT("LeftMouseUp"), Address);
		// P4x deliberately leaves a click as read-only selection. Position changes are
		// committed exclusively by a later real DragOperation Drop on an explicit target.
		OwnerWidget->HandleCellActivated(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCodeBP3CellButton::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Address.bOccupied && OwnerWidget.IsValid())
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
	DragVisual->SetText(FText::FromString(FString::Printf(TEXT("拖拽：%s  x%d"), *Payload.DefinitionId.ToString(), Payload.Quantity)));
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

void UCodeBP3HotbarActionButton::Configure(
	UCodeBP3InventoryWidget* InOwnerWidget,
	const int32 InSlotIndex,
	const bool bInUnbindAction)
{
	OwnerWidget = InOwnerWidget;
	SlotIndex = InSlotIndex;
	bUnbindAction = bInUnbindAction;
	BuildButton();
}

void UCodeBP3HotbarActionButton::SetLabel(const FString& InLabel, const bool bEnabled)
{
	BuildButton();
	if (InnerButton)
	{
		InnerButton->SetIsEnabled(bEnabled);
		InnerButton->SetContent(MakeText(
			WidgetTree, InLabel, 12,
			bEnabled ? FLinearColor::White : FLinearColor(0.55f, 0.55f, 0.55f)));
	}
}

void UCodeBP3HotbarActionButton::BuildButton()
{
	if (!InnerButton && WidgetTree)
	{
		InnerButton = WidgetTree->ConstructWidget<UButton>();
		InnerButton->OnClicked.AddDynamic(this, &UCodeBP3HotbarActionButton::OnClicked);
		WidgetTree->RootWidget = InnerButton;
	}
}

void UCodeBP3HotbarActionButton::OnClicked()
{
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleHotbarSlotAction(SlotIndex, bUnbindAction);
	}
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
	ContextMenuRevision = INDEX_NONE;
	Super::NativeDestruct();
}

FReply UCodeBP3InventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
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
		if (Host.IsValid() && Host->GetController() && Host->GetController()->GetOperationMode() != ECodeBP3OperationMode::None)
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

	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	ScrollBox->SetOrientation(Orient_Vertical);
	ScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	PageSize->SetContent(ScrollBox);

	PageContents = WidgetTree->ConstructWidget<UVerticalBox>();
	ScrollBox->AddChild(PageContents);
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
		return Address.bOccupied ? ItemColor : EmptyColor;
	}
	const TOptional<FCodeBP3SlotAddress>& Pending = Host->GetController()->GetPendingSource();
	if (Pending.IsSet() && Pending->ContainerId == Address.ContainerId && Pending->SlotIndex == Address.SlotIndex)
	{
		return PendingColor;
	}
	const TOptional<FCodeBP3SlotAddress>& Selected = Host->GetController()->GetSelectedAddress();
	if (Selected.IsSet() && Selected->ContainerId == Address.ContainerId && Selected->SlotIndex == Address.SlotIndex)
	{
		return SelectedColor;
	}
	return Address.bOccupied ? ItemColor : EmptyColor;
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
	if (FCodeBP4InteractionController* Interaction = GetP4Controller())
	{
		return Interaction->BeginDrag(CellButton->GetAddress(), OutPayload);
	}
	return false;
}

void UCodeBP3InventoryWidget::HandleP4DragEnter(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation)
{
	if (!CellButton || !Operation)
	{
		return;
	}
	if (Host.IsValid() && (Host->IsNormalContainerSlotProtected(
		CellButton->GetAddress().ContainerId, CellButton->GetAddress().SlotIndex)
		|| Host->IsBodyContainerSlotProtected(
			CellButton->GetAddress().ContainerId, CellButton->GetAddress().SlotIndex)))
	{
		FCodeBP4DropPreview Rejected;
		Rejected.Message = TEXT("未知或正在搜索的容器物品不能作为拖拽目标。");
		TraceP4Input(TEXT("DragEnter"), CellButton->GetAddress(), TEXT("Preview=Rejected HiddenOrSearchingTarget"));
		P4PreviewAddress = CellButton->GetAddress();
		P4Preview = Rejected;
		CellButton->SetCellColor(GetNormalCellColor(CellButton->GetAddress()));
		return;
	}
	if (Host.IsValid())
	{
		const bool bBodySource = Host->IsBodyContainerPresentation(Operation->GetPayload().Source.ContainerId);
		const bool bBodyDestination = Host->IsBodyContainerPresentation(CellButton->GetAddress().ContainerId);
		const bool bP21BodyEquipmentSource = Host->IsBodyEquipmentContainerPresentation(Operation->GetPayload().Source.ContainerId);
		const bool bP21BodyEquipmentDestination = Host->IsBodyEquipmentContainerPresentation(CellButton->GetAddress().ContainerId);
		const bool bP20SpatialParent = Operation->GetPayload().DefinitionId == Fdemo_mapItemIds::WindTalisman
			|| Operation->GetPayload().DefinitionId == Fdemo_mapItemIds::BackpackLevel1;
		const bool bEmptyBaseQuickDestination = CellButton->GetAddress().ContainerId == Host->GetController()->GetProjection().BasicContainerId
			&& !CellButton->GetAddress().bOccupied;
		if ((bP21BodyEquipmentSource && !bEmptyBaseQuickDestination)
			|| bP21BodyEquipmentDestination
			|| (bBodySource && bP20SpatialParent && !bEmptyBaseQuickDestination)
			|| (bBodyDestination && bP20SpatialParent))
		{
			FCodeBP4DropPreview Rejected;
			Rejected.Message = bP21BodyEquipmentDestination
				? TEXT("玩家物品不能回存尸体装备位。")
				: bP21BodyEquipmentSource
					? TEXT("尸体装备只能拖到空基础物品栏。")
				: bBodyDestination
				? TEXT("空间道具不能从玩家放回尸体。")
				: TEXT("尸体空间根节点只能拖到空基础物品栏。");
			TraceP4Input(TEXT("DragEnter"), CellButton->GetAddress(), TEXT("Preview=Rejected P20BodySpatialRootOnly"));
			P4PreviewAddress = CellButton->GetAddress();
			P4Preview = Rejected;
			CellButton->SetCellColor(GetNormalCellColor(CellButton->GetAddress()));
			return;
		}
	}
	if (Host.IsValid() && (Host->HasNormalContainerPresentation() || Host->HasBodyContainerPresentation() || Host->IsWorldDropPresentation(Operation->GetPayload().Source.ContainerId) || Host->IsWorldDropPresentation(CellButton->GetAddress().ContainerId)))
	{
		const bool bSourceIsTarget = Host->IsNormalContainerPresentation(
			Operation->GetPayload().Source.ContainerId)
			|| Host->IsBodyContainerPresentation(Operation->GetPayload().Source.ContainerId)
			|| Host->IsWorldDropPresentation(Operation->GetPayload().Source.ContainerId);
		const bool bDestinationIsTarget = Host->IsNormalContainerPresentation(
			CellButton->GetAddress().ContainerId)
			|| Host->IsBodyContainerPresentation(CellButton->GetAddress().ContainerId)
			|| Host->IsWorldDropPresentation(CellButton->GetAddress().ContainerId);
		if (!bSourceIsTarget && !bDestinationIsTarget)
		{
			FCodeBP4DropPreview Rejected;
			Rejected.Message = TEXT("打开容器时，拖拽必须跨越玩家与目标两栏。");
			TraceP4Input(TEXT("DragEnter"), CellButton->GetAddress(), TEXT("Preview=Rejected NotTwoColumnTransfer"));
			P4PreviewAddress = CellButton->GetAddress();
			P4Preview = Rejected;
			CellButton->SetCellColor(GetNormalCellColor(CellButton->GetAddress()));
			return;
		}
	}
	if (FCodeBP4InteractionController* Interaction = GetP4Controller())
	{
		const FCodeBP4DropPreview Preview = Interaction->PreviewDrop(Operation->GetPayload(), CellButton->GetAddress());
		TraceP4Input(TEXT("DragEnter"), CellButton->GetAddress(), FString::Printf(TEXT("Allowed=%d Kind=%s"), Preview.bAllowed ? 1 : 0, *FCodeBP4InteractionController::GetDropKindLabel(Preview.Kind)));
		P4PreviewAddress = CellButton->GetAddress();
		P4Preview = Preview;
		CellButton->SetCellColor(GetNormalCellColor(CellButton->GetAddress()));
	}
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
	if (Host.IsValid() && (Host->IsNormalContainerSlotProtected(
		CellButton->GetAddress().ContainerId, CellButton->GetAddress().SlotIndex)
		|| Host->IsBodyContainerSlotProtected(
			CellButton->GetAddress().ContainerId, CellButton->GetAddress().SlotIndex)))
	{
		Host->GetController()->SetP4Feedback(TEXT("未知或正在搜索的容器物品不能作为拖拽目标。"));
		TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), TEXT("Preview=Rejected HiddenOrSearchingTarget"));
		return true;
	}
	if (Host.IsValid())
	{
		const bool bBodySource = Host->IsBodyContainerPresentation(Operation->GetPayload().Source.ContainerId);
		const bool bBodyDestination = Host->IsBodyContainerPresentation(CellButton->GetAddress().ContainerId);
		const bool bP21BodyEquipmentSource = Host->IsBodyEquipmentContainerPresentation(Operation->GetPayload().Source.ContainerId);
		const bool bP21BodyEquipmentDestination = Host->IsBodyEquipmentContainerPresentation(CellButton->GetAddress().ContainerId);
		const bool bP20SpatialParent = Operation->GetPayload().DefinitionId == Fdemo_mapItemIds::WindTalisman
			|| Operation->GetPayload().DefinitionId == Fdemo_mapItemIds::BackpackLevel1;
		const bool bEmptyBaseQuickDestination = CellButton->GetAddress().ContainerId == Host->GetController()->GetProjection().BasicContainerId
			&& !CellButton->GetAddress().bOccupied;
		if ((bP21BodyEquipmentSource && !bEmptyBaseQuickDestination)
			|| bP21BodyEquipmentDestination
			|| (bBodySource && bP20SpatialParent && !bEmptyBaseQuickDestination)
			|| (bBodyDestination && bP20SpatialParent))
		{
			Host->GetController()->SetP4Feedback(bP21BodyEquipmentDestination
				? TEXT("玩家物品不能回存尸体装备位。")
				: bP21BodyEquipmentSource
					? TEXT("尸体装备只能拖到空基础物品栏。")
				: bBodyDestination
				? TEXT("空间道具不能从玩家放回尸体。")
				: TEXT("尸体空间根节点只能拖到空基础物品栏。"));
			TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), TEXT("Preview=Rejected P20BodySpatialRootOnly"));
			return true;
		}
	}
	if (Host.IsValid() && (Host->HasNormalContainerPresentation() || Host->HasBodyContainerPresentation() || Host->IsWorldDropPresentation(Operation->GetPayload().Source.ContainerId) || Host->IsWorldDropPresentation(CellButton->GetAddress().ContainerId)))
	{
		const bool bSourceIsTarget = Host->IsNormalContainerPresentation(
			Operation->GetPayload().Source.ContainerId)
			|| Host->IsBodyContainerPresentation(Operation->GetPayload().Source.ContainerId)
			|| Host->IsWorldDropPresentation(Operation->GetPayload().Source.ContainerId);
		const bool bDestinationIsTarget = Host->IsNormalContainerPresentation(
			CellButton->GetAddress().ContainerId)
			|| Host->IsBodyContainerPresentation(CellButton->GetAddress().ContainerId)
			|| Host->IsWorldDropPresentation(CellButton->GetAddress().ContainerId);
		if (!bSourceIsTarget && !bDestinationIsTarget)
		{
			Host->GetController()->SetP4Feedback(TEXT("打开容器时，拖拽必须跨越玩家与目标两栏。"));
			TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), TEXT("Preview=Rejected NotTwoColumnTransfer"));
			return true;
		}
	}
	if (FCodeBP4InteractionController* Interaction = GetP4Controller())
	{
		const FCodeBP4DropPreview Preview = Interaction->PreviewDrop(Operation->GetPayload(), CellButton->GetAddress());
		const bool bContainerTargetIsSource = Host.IsValid()
			&& (Host->IsNormalContainerPresentation(Operation->GetPayload().Source.ContainerId)
				|| Host->IsBodyContainerPresentation(Operation->GetPayload().Source.ContainerId)
				|| Host->IsWorldDropPresentation(Operation->GetPayload().Source.ContainerId));
		const bool bWorldDropIsSource = Host.IsValid()
			&& Host->IsWorldDropPresentation(Operation->GetPayload().Source.ContainerId);
		const FCodeBP2ContainerView* WorldDropTarget = Host.IsValid()
			? Host->GetController()->GetProjection().Containers.FindByPredicate(
				[CellButton](const FCodeBP2ContainerView& Container)
				{
					return Container.ContainerId == CellButton->GetAddress().ContainerId;
				})
			: nullptr;
		const bool bEmptyBaseQuickTarget = Host.IsValid()
			&& CellButton->GetAddress().ContainerId == Host->GetController()->GetProjection().BasicContainerId
			&& Preview.bAllowed && Preview.Operation == ECodeBOperation::Move;
		const bool bWindTalismanEquipmentTarget = Preview.bAllowed
			&& Preview.Operation == ECodeBOperation::Equip
			&& Operation->GetPayload().DefinitionId == Fdemo_mapItemIds::WindTalisman
			&& WorldDropTarget && WorldDropTarget->Role == FName(TEXT("SpatialRing"));
		const bool bBackpackEquipmentTarget = Preview.bAllowed
			&& Preview.Operation == ECodeBOperation::Equip
			&& Operation->GetPayload().DefinitionId == Fdemo_mapItemIds::BackpackLevel1
			&& WorldDropTarget && WorldDropTarget->Role == FName(TEXT("Backpack"));
		if (bWorldDropIsSource
			&& !bEmptyBaseQuickTarget && !bWindTalismanEquipmentTarget && !bBackpackEquipmentTarget)
		{
			Host->GetController()->SetP4Feedback(TEXT("地面根节点只能拖到空基础物品栏，或其匹配的空空间装备栏；不允许合并、交换或拆分。"));
			TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), TEXT("Preview=Rejected WorldDropRootOnlyLegalEmptyDestination"));
			return true;
		}
		if (Host.IsValid() && Host->IsWorldDropPresentation(CellButton->GetAddress().ContainerId))
		{
			Host->GetController()->SetP4Feedback(TEXT("不能将物品拖入地面目标；请使用明确的地面丢弃区域。"));
			TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), TEXT("Preview=Rejected NoDropIntoWorldTarget"));
			return true;
		}
		if (bContainerTargetIsSource
			&& Preview.bAllowed
			&& Preview.Operation != ECodeBOperation::Move
			&& Preview.Operation != ECodeBOperation::Merge
			&& Preview.Operation != ECodeBOperation::Swap)
		{
			Host->GetController()->SetP4Feedback(TEXT("容器仅允许已揭示物品与玩家储物格之间的移动、合并或交换。"));
			TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), TEXT("Preview=Rejected NormalContainerNonTransferOperation"));
			return true;
		}
		// P4x counts only a preview-approved physical drop as a P2 submission/call.
		// Rejected previews return feedback locally and must prove zero writes.
		if (Preview.bAllowed)
		{
			++CurrentP4SubmittedCommandCount;
			++CurrentP4P2CallCount;
		}
		const bool bCommitted = Interaction->CommitDrop(Operation->GetPayload(), CellButton->GetAddress());
		TraceP4Input(TEXT("Drop"), CellButton->GetAddress(), FString::Printf(TEXT("Preview=%s CommitSucceeded=%d Kind=%s"), Preview.bAllowed ? TEXT("Allowed") : TEXT("Rejected"), bCommitted ? 1 : 0, *FCodeBP4InteractionController::GetDropKindLabel(Preview.Kind)));
		ContextMenuAddress.Reset();
		ContextMenuRevision = INDEX_NONE;
		P4PreviewAddress.Reset();
		P4Preview.Reset();
		RefreshFromController();
		return true;
	}
	return false;
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
	if (bCommitted)
	{
		// The Store replacement removed this root (and possibly its P17 child
		// closure) from the live player projection. Do not retain a cell selection
		// or pending source that could point at the now-world-owned graph.
		Host->GetController()->ClearTransientSelection(TEXT("完整物品图已转入地面真值。"));
	}
	Host->GetController()->SetP4Feedback(bCommitted
		? TEXT("已提交地面丢弃；地面物品由 P6 持久化。")
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

void UCodeBP3InventoryWidget::AddContainerSection(UVerticalBox* Parent, const FString& Title, const FCodeBP2ContainerView& Container, const int32 Columns)
{
	UVerticalBox* Section = AddPanel(Parent, FString::Printf(TEXT("%s  ·  %d 格"), *Title, Container.Capacity));
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(3.0f));
	Section->AddChildToVerticalBox(Grid)->SetHorizontalAlignment(HAlign_Fill);

	for (int32 Index = 0; Index < Container.Slots.Num(); ++Index)
	{
		const FCodeBP2SlotView& SlotView = Container.Slots[Index];
		FCodeBP3SlotAddress Address;
		Address.ContainerId = Container.ContainerId;
		Address.SlotIndex = SlotView.SlotIndex;
		Address.SlotId = SlotView.SlotId;
		Address.ItemId = SlotView.ItemId;
		Address.bOccupied = SlotView.bOccupied;

		UCodeBP3CellButton* Cell = WidgetTree->ConstructWidget<UCodeBP3CellButton>();
		Cell->Configure(this, Address);
		MountedCells.Add(Cell);
		Cell->SetCellColor(GetNormalCellColor(Address));
		FString Label;
		if (SlotView.bOccupied)
		{
			Label = FString::Printf(TEXT("%s\nx%d  L%d/Q%d"), *SlotView.DefinitionId.ToString(), SlotView.Quantity, SlotView.Level, SlotView.Quality);
		}
		else
		{
			Label = FString::Printf(TEXT("空格\n%s"), *SlotView.SlotId.ToString());
		}
		Cell->SetCellContent(MakeText(WidgetTree, Label, 12, SlotView.bOccupied ? FLinearColor::White : FLinearColor(0.52f, 0.58f, 0.64f)));
		UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Cell, Index / Columns, Index % Columns);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UCodeBP3InventoryWidget::AddNormalContainerSection(
	UVerticalBox* Parent,
	const FCodeBP2ContainerView& Container)
{
	UVerticalBox* Section = AddPanel(Parent, FString::Printf(TEXT("普通容器  ·  %d 格"), Container.Capacity));
	Section->AddChildToVerticalBox(MakeText(
		WidgetTree,
		TEXT("未知物品只能逐件搜索；正在搜索、未知物品与详情都不提供拖拽或转移。"),
		12,
		FLinearColor(0.72f, 0.82f, 0.92f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 4.0f));
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(3.0f));
	Section->AddChildToVerticalBox(Grid)->SetHorizontalAlignment(HAlign_Fill);

	for (int32 Index = 0; Index < Container.Slots.Num(); ++Index)
	{
		const FCodeBP2SlotView& SlotView = Container.Slots[Index];
		const bool bHidden = SlotView.bOccupied && Host.IsValid()
			&& Host->IsNormalContainerItemHidden(SlotView.ItemId);
		const bool bSearching = SlotView.bOccupied && Host.IsValid()
			&& Host->IsNormalContainerItemSearching(SlotView.ItemId);
		FCodeBP3SlotAddress Address;
		Address.ContainerId = Container.ContainerId;
		Address.SlotIndex = SlotView.SlotIndex;
		Address.SlotId = SlotView.SlotId;
		Address.ItemId = bHidden || bSearching ? FGuid() : SlotView.ItemId;
		Address.bOccupied = SlotView.bOccupied && !bHidden && !bSearching;

		UCodeBP3CellButton* Cell = WidgetTree->ConstructWidget<UCodeBP3CellButton>();
		Cell->Configure(this, Address);
		MountedCells.Add(Cell);
		Cell->SetCellColor(
			bSearching ? FLinearColor(0.42f, 0.28f, 0.08f, 1.0f)
			: bHidden ? FLinearColor(0.16f, 0.20f, 0.24f, 1.0f)
			: GetNormalCellColor(Address));
		const FString Label = bSearching
			? TEXT("正在搜索\n内容保密")
			: bHidden
				? TEXT("未搜索\n点击搜索")
				: SlotView.bOccupied
					? FString::Printf(TEXT("%s\nx%d  L%d/Q%d"), *SlotView.DefinitionId.ToString(), SlotView.Quantity, SlotView.Level, SlotView.Quality)
					: FString::Printf(TEXT("空格\n%s"), *SlotView.SlotId.ToString());
		Cell->SetCellContent(MakeText(
			WidgetTree,
			Label,
			12,
			bHidden || bSearching || !SlotView.bOccupied
				? FLinearColor(0.62f, 0.68f, 0.74f)
				: FLinearColor::White));
		UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Cell, Index / 2, Index % 2);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UCodeBP3InventoryWidget::AddBodyContainerSection(
	UVerticalBox* Parent,
	const FCodeBP2ContainerView& Container)
{
	UVerticalBox* Section = AddPanel(Parent, FString::Printf(TEXT("尸体  ·  %d 格"), Container.Capacity));
	Section->AddChildToVerticalBox(MakeText(
		WidgetTree,
		TEXT("未知物品只能逐件搜查；搜查中和未知物品不显示信息、详情或拖拽 payload。"),
		12,
		FLinearColor(0.82f, 0.70f, 0.70f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 4.0f));
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(3.0f));
	Section->AddChildToVerticalBox(Grid)->SetHorizontalAlignment(HAlign_Fill);
	for (int32 Index = 0; Index < Container.Slots.Num(); ++Index)
	{
		const FCodeBP2SlotView& SlotView = Container.Slots[Index];
		const bool bHidden = SlotView.bOccupied && Host.IsValid()
			&& Host->IsBodyContainerItemHidden(SlotView.ItemId);
		const bool bSearching = SlotView.bOccupied && Host.IsValid()
			&& Host->IsBodyContainerItemSearching(SlotView.ItemId);
		FCodeBP3SlotAddress Address;
		Address.ContainerId = Container.ContainerId;
		Address.SlotIndex = SlotView.SlotIndex;
		Address.SlotId = SlotView.SlotId;
		Address.ItemId = bHidden || bSearching ? FGuid() : SlotView.ItemId;
		Address.bOccupied = SlotView.bOccupied && !bHidden && !bSearching;
		UCodeBP3CellButton* Cell = WidgetTree->ConstructWidget<UCodeBP3CellButton>();
		Cell->Configure(this, Address);
		MountedCells.Add(Cell);
		Cell->SetCellColor(
			bSearching ? FLinearColor(0.42f, 0.20f, 0.08f, 1.0f)
			: bHidden ? FLinearColor(0.22f, 0.13f, 0.13f, 1.0f)
			: GetNormalCellColor(Address));
		const FString Label = bSearching
			? TEXT("正在搜查\n内容保密")
			: bHidden
				? TEXT("未搜查\n点击搜查")
				: SlotView.bOccupied
					? FString::Printf(TEXT("%s\nx%d  L%d/Q%d"), *SlotView.DefinitionId.ToString(), SlotView.Quantity, SlotView.Level, SlotView.Quality)
					: FString::Printf(TEXT("空格\n%s"), *SlotView.SlotId.ToString());
		Cell->SetCellContent(MakeText(
			WidgetTree, Label, 12,
			bHidden || bSearching || !SlotView.bOccupied
				? FLinearColor(0.70f, 0.64f, 0.64f) : FLinearColor::White));
		UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Cell, Index / 2, Index % 2);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UCodeBP3InventoryWidget::AddBodyEquipmentContainerSection(
	UVerticalBox* Parent,
	const FCodeBP2ContainerView& Container,
	const FName SlotSemantic)
{
	UVerticalBox* Section = AddPanel(Parent, FString::Printf(TEXT("尸体装备  ·  %s"), *SlotSemantic.ToString()));
	Section->AddChildToVerticalBox(MakeText(
		WidgetTree,
		TEXT("仅在既有尸体揭示后可见；只能拖到空基础物品栏，不能回存或直接装备。"),
		12, FLinearColor(0.82f, 0.70f, 0.70f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 4.0f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Section->AddChildToVerticalBox(Row);
	for (const FCodeBP2SlotView& SlotView : Container.Slots)
	{
		const bool bHidden = SlotView.bOccupied && Host.IsValid() && Host->IsBodyContainerItemHidden(SlotView.ItemId);
		const bool bSearching = SlotView.bOccupied && Host.IsValid() && Host->IsBodyContainerItemSearching(SlotView.ItemId);
		FCodeBP3SlotAddress Address;
		Address.ContainerId = Container.ContainerId;
		Address.SlotIndex = SlotView.SlotIndex;
		Address.SlotId = SlotView.SlotId;
		Address.ItemId = bHidden || bSearching ? FGuid() : SlotView.ItemId;
		Address.bOccupied = SlotView.bOccupied && !bHidden && !bSearching;
		UCodeBP3CellButton* Cell = WidgetTree->ConstructWidget<UCodeBP3CellButton>();
		Cell->Configure(this, Address);
		MountedCells.Add(Cell);
		Cell->SetCellColor(bSearching ? FLinearColor(0.42f, 0.20f, 0.08f, 1.0f)
			: bHidden ? FLinearColor(0.22f, 0.13f, 0.13f, 1.0f) : GetNormalCellColor(Address));
		const FString Label = bSearching ? TEXT("正在搜查\n内容保密")
			: bHidden ? TEXT("未搜查\n点击搜查")
			: SlotView.bOccupied ? FString::Printf(TEXT("%s\nx%d"), *SlotView.DefinitionId.ToString(), SlotView.Quantity)
			: TEXT("空装备位");
		Cell->SetCellContent(MakeText(WidgetTree, Label, 12,
			bHidden || bSearching || !SlotView.bOccupied ? FLinearColor(0.70f, 0.64f, 0.64f) : FLinearColor::White));
		Row->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(2.0f));
	}
}

void UCodeBP3InventoryWidget::AddHotbarPlaceholders(UVerticalBox* Parent)
{
	UVerticalBox* Section = AddPanel(Parent, TEXT("1—9 快捷栏引用（仅绑定／解绑；未启用使用）"));
	if (!Host.IsValid() || !Host->GetController() || !Host->GetHotbarPresentation())
	{
		Section->AddChildToVerticalBox(MakeText(
			WidgetTree,
			TEXT("此入口没有 P13 真实 Profile／活动 Run 快捷栏投影；不会显示或写入任何引用。"),
			13, FLinearColor(0.65f, 0.70f, 0.76f)));
		return;
	}

	const FCodeBP3HotbarPresentation* Presentation = Host->GetHotbarPresentation();
	const FCodeBP2Projection& Projection = Host->GetController()->GetProjection();
	bool bCanBindSelected = false;
	if (Host->GetController()->GetSelectedAddress().IsSet())
	{
		const FCodeBP3SlotAddress& Selected = Host->GetController()->GetSelectedAddress().GetValue();
		const FCodeBP2ContainerView* BasicContainer = Projection.Containers.FindByPredicate(
			[&Projection](const FCodeBP2ContainerView& Candidate)
			{ return Candidate.ContainerId == Projection.BasicContainerId; });
		if (BasicContainer && Selected.ContainerId == Projection.BasicContainerId
			&& BasicContainer->Slots.IsValidIndex(Selected.SlotIndex))
		{
			const FCodeBP2SlotView& SelectedSlot = BasicContainer->Slots[Selected.SlotIndex];
			bCanBindSelected = Selected.bOccupied && SelectedSlot.bOccupied
				&& Selected.ItemId == SelectedSlot.ItemId && SelectedSlot.bQuickUsable;
		}
	}
	Section->AddChildToVerticalBox(MakeText(
		WidgetTree,
		TEXT("先选中基础快捷物品区的 QuickUsable 物品，再点“绑定”指定槽。清空只移除 ItemId 引用，绝不移动、消耗或使用物品。"),
		13, FLinearColor(0.72f, 0.82f, 0.92f)))->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 4.0f));

	UHorizontalBox* BindActions = WidgetTree->ConstructWidget<UHorizontalBox>();
	Section->AddChildToVerticalBox(BindActions);
	UHorizontalBox* UnbindActions = WidgetTree->ConstructWidget<UHorizontalBox>();
	Section->AddChildToVerticalBox(UnbindActions)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
	for (int32 Index = 1; Index <= FCodeBHotbarBindings::SlotCount; ++Index)
	{
		const FCodeBHotbarSlotProjection* HotbarSlot = Presentation->Projection.Slots.IsValidIndex(Index - 1)
			? &Presentation->Projection.Slots[Index - 1] : nullptr;
		const FString ReferenceLabel = HotbarSlot && HotbarSlot->bHasReference
			? FString::Printf(TEXT("%d\n%s x%d\n绑定"), Index, *HotbarSlot->DefinitionId.ToString(), HotbarSlot->Quantity)
			: FString::Printf(TEXT("%d\n空\n绑定"), Index);
		UCodeBP3HotbarActionButton* BindButton = WidgetTree->ConstructWidget<UCodeBP3HotbarActionButton>();
		BindButton->Configure(this, Index, false);
		BindButton->SetLabel(ReferenceLabel, Presentation->Projection.bEditable && bCanBindSelected);
		BindActions->AddChildToHorizontalBox(BindButton)->SetPadding(FMargin(2.0f));

		UCodeBP3HotbarActionButton* UnbindButton = WidgetTree->ConstructWidget<UCodeBP3HotbarActionButton>();
		UnbindButton->Configure(this, Index, true);
		UnbindButton->SetLabel(
			FString::Printf(TEXT("清空 %d"), Index),
			Presentation->Projection.bEditable && HotbarSlot && HotbarSlot->bHasReference);
		UnbindActions->AddChildToHorizontalBox(UnbindButton)->SetPadding(FMargin(2.0f));
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
		if (Container && Container->Slots.IsValidIndex(Selected.SlotIndex))
		{
			const FCodeBP2SlotView& SlotView = Container->Slots[Selected.SlotIndex];
			const TCHAR* SourceDescription = Host->GetController()->IsProfileBacked()
				? TEXT("真实 Profile Code B 持久化实例。")
				: TEXT("Code B Fixture 最小可审计展示数据。");
			DetailText = FString::Printf(TEXT("定义：%s\n类型：%s\n数量：%d\n等级／品质：%d / %d\nItemId：%s\n容器／槽位：%s / %s\n随机种子：%d\n描述：%s"),
				*SlotView.DefinitionId.ToString(), *ItemTypeToChinese(SlotView.ItemType), SlotView.Quantity, SlotView.Level, SlotView.Quality,
				*SlotView.ItemId.ToString(EGuidFormats::DigitsWithHyphens), *Container->Role.ToString(), *SlotView.SlotId.ToString(), SlotView.RandomSeed, SourceDescription);
		}
	}
	Detail->AddChildToVerticalBox(MakeText(WidgetTree, DetailText, 14, FLinearColor(0.86f, 0.90f, 0.95f)))->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 10.0f));

	UVerticalBox* Actions = AddPanel(Parent, TEXT("物品操作规则"));
	Actions->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("移动、交换、合并、装备替换和卸下都必须拖到明确目标格；本页没有自动转移、合并或放回。"), 13, FLinearColor(0.72f, 0.82f, 0.92f)));
	if (Host.IsValid() && Host->GetController() && Host->GetController()->GetSelectedAddress().IsSet())
	{
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
	PageContents->ClearChildren();
	MountedCells.Reset();
	CloseButton = nullptr;
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
	UVerticalBox* StashColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	UVerticalBox* DetailColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	FSlateChildSize PlayerSize; PlayerSize.SizeRule = ESlateSizeRule::Fill; PlayerSize.Value = 1.05f;
	FSlateChildSize StashSize; StashSize.SizeRule = ESlateSizeRule::Fill; StashSize.Value = 1.25f;
	FSlateChildSize DetailSize; DetailSize.SizeRule = ESlateSizeRule::Fill; DetailSize.Value = 0.95f;
	Main->AddChildToHorizontalBox(PlayerColumn)->SetSize(PlayerSize);
	Main->AddChildToHorizontalBox(StashColumn)->SetSize(StashSize);
	Main->AddChildToHorizontalBox(DetailColumn)->SetSize(DetailSize);

	if (const FCodeBP2ContainerView* Weapon = FindRole(FName(TEXT("Weapon")))) AddContainerSection(PlayerColumn, TEXT("兵器"), *Weapon, 1);
	if (const FCodeBP2ContainerView* Armor = FindRole(FName(TEXT("Armor")))) AddContainerSection(PlayerColumn, TEXT("道袍"), *Armor, 1);
	if (const FCodeBP2ContainerView* Accessory0 = FindRole(FName(TEXT("Accessory0")))) AddContainerSection(PlayerColumn, TEXT("饰品 I"), *Accessory0, 1);
	if (const FCodeBP2ContainerView* Accessory1 = FindRole(FName(TEXT("Accessory1")))) AddContainerSection(PlayerColumn, TEXT("饰品 II"), *Accessory1, 1);
	if (const FCodeBP2ContainerView* SpatialRing = FindRole(FName(TEXT("SpatialRing")))) AddContainerSection(PlayerColumn, TEXT("空间戒指（仅此栏可装备）"), *SpatialRing, 1);
	if (const FCodeBP2ContainerView* Backpack = FindRole(FName(TEXT("Backpack")))) AddContainerSection(PlayerColumn, TEXT("空间储物囊"), *Backpack, 1);
	if (const FCodeBP2ContainerView* Basic = FindRole(FName(TEXT("Basic6")))) AddContainerSection(PlayerColumn, TEXT("基础物品"), *Basic, 3);

	if (!bActiveRunBacked)
	{
		if (const FCodeBP2ContainerView* Stash = FindRole(FName(TEXT("Warehouse")))) AddContainerSection(StashColumn, TEXT("局外仓库"), *Stash, 5);
	}
	if (bActiveRunBacked)
	{
		if (const FCodeBP2ContainerView* NormalTarget = FindRole(FName(TEXT("NormalContainerTarget"))))
		{
			AddNormalContainerSection(StashColumn, *NormalTarget);
		}
		if (const FCodeBP2ContainerView* BodyTarget = FindRole(FName(TEXT("BodyContainerTarget"))))
		{
			AddBodyContainerSection(StashColumn, *BodyTarget);
		}
		for (const FName BodyEquipmentRole : {
			FName(TEXT("Body.Weapon")), FName(TEXT("Body.ArmorRobe")), FName(TEXT("Body.Accessory0")) })
		{
			if (const FCodeBP2ContainerView* EquipmentContainer = FindRole(BodyEquipmentRole))
			{
				AddBodyEquipmentContainerSection(StashColumn, *EquipmentContainer, BodyEquipmentRole);
			}
		}
		if (const FCodeBP2ContainerView* WorldTarget = FindRole(FName(TEXT("WorldDropTarget"))))
		{
			// P19 intentionally mounts only the P14 root cell. A spatial parent's
			// child graph remains Store-owned and cannot become a ground sub-item UI.
			AddContainerSection(StashColumn, TEXT("地面完整图根节点（仅拖回空基础格或匹配空装备栏）"), *WorldTarget, 1);
		}
	}
	if (const FCodeBP2ContainerView* SpatialRing = FindRole(FName(TEXT("SpatialRing"))))
	{
		if (SpatialRing->Slots.Num() > 0 && SpatialRing->Slots[0].bOccupied)
		{
			if (const FCodeBP2ContainerView* QuickSpatial = FindRole(FName(TEXT("QuickSpatial")))) AddContainerSection(StashColumn, TEXT("快捷空间（空间戒指）"), *QuickSpatial, 4);
		}
		else
		{
			UVerticalBox* EmptySpatial = AddPanel(StashColumn, TEXT("快捷空间（空间戒指）"));
			EmptySpatial->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("空间戒指未装备；快捷空间不会显示或提供访问。"), 14, FLinearColor(0.65f, 0.70f, 0.76f)));
		}
	}
	if (const FCodeBP2ContainerView* PouchInternal = FindRole(FName(TEXT("PouchInternal")))) AddContainerSection(StashColumn, TEXT("非快捷储物囊（普通空间物品）"), *PouchInternal, 4);
	if (bActiveRunBacked) AddGroundDropZone(StashColumn);
	AddHotbarPlaceholders(StashColumn);
	AddDetailAndActions(DetailColumn);
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
		Host->GetController()->ActivateAddress(CellButton->GetAddress());
		RefreshFromController();
	}
}

void UCodeBP3InventoryWidget::HandleHotbarSlotAction(const int32 SlotIndex, const bool bUnbindAction)
{
	if (!Host.IsValid() || !Host->GetController())
	{
		return;
	}
	FString Error;
	const bool bAccepted = bUnbindAction
		? Host->RequestHotbarUnbind(SlotIndex, Error)
		: Host->RequestHotbarBindFromSelectedItem(SlotIndex, Error);
	Host->GetController()->SetP4Feedback(bAccepted
		? (bUnbindAction
			? FString::Printf(TEXT("已清空快捷栏 %d 的引用。"), SlotIndex)
			: FString::Printf(TEXT("已绑定快捷栏 %d；未启用按键使用或消耗。"), SlotIndex))
		: (Error.IsEmpty() ? TEXT("快捷栏引用操作未完成。") : Error));
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
	if (!Container || !Container->Slots.IsValidIndex(Selected.SlotIndex))
	{
		return false;
	}
	const FCodeBP2SlotView& SelectedSlot = Container->Slots[Selected.SlotIndex];
	return SelectedSlot.ItemType == ECodeBItemType::Weapon
		|| SelectedSlot.ItemType == ECodeBItemType::Armor
		|| SelectedSlot.ItemType == ECodeBItemType::Accessory
		|| SelectedSlot.ItemType == ECodeBItemType::SpatialItem
		|| SelectedSlot.ItemType == ECodeBItemType::Backpack;
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
	const FCodeBP3WorldDropPresentation* InWorldDropPresentation)
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
	if (!IsNormalContainerPresentation(ContainerId) || !Controller.IsValid()) return false;
	const FCodeBP2ContainerView* Container = Controller->GetProjection().Containers.FindByPredicate(
		[ContainerId](const FCodeBP2ContainerView& Value) { return Value.ContainerId == ContainerId; });
	if (!Container || !Container->Slots.IsValidIndex(SlotIndex) || !Container->Slots[SlotIndex].bOccupied)
	{
		return false;
	}
	const FGuid ItemId = Container->Slots[SlotIndex].ItemId;
	return IsNormalContainerItemHidden(ItemId) || IsNormalContainerItemSearching(ItemId);
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
	const FCodeBP2ContainerView* Container = Controller->GetProjection().Containers.FindByPredicate(
		[ContainerId](const FCodeBP2ContainerView& Value) { return Value.ContainerId == ContainerId; });
	if (!Container || !Container->Slots.IsValidIndex(SlotIndex)
		|| !Container->Slots[SlotIndex].bOccupied)
	{
		OutError = TEXT("该普通容器格没有可搜索的物品。");
		return false;
	}
	const FGuid ItemId = Container->Slots[SlotIndex].ItemId;
	if (!IsNormalContainerItemHidden(ItemId))
	{
		OutError = IsNormalContainerItemSearching(ItemId)
			? TEXT("该物品正在搜索中。")
			: TEXT("该物品已揭示，不能再次搜索。");
		return false;
	}
	FCodeBNormalContainerProjection UpdatedProjection;
	if (!NormalContainerPresentation->BeginItemSearch(ItemId, UpdatedProjection, OutError))
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
	if (!IsBodyContainerPresentation(ContainerId) || !Controller.IsValid()) return false;
	const FCodeBP2ContainerView* Container = Controller->GetProjection().Containers.FindByPredicate(
		[ContainerId](const FCodeBP2ContainerView& Value) { return Value.ContainerId == ContainerId; });
	if (!Container || !Container->Slots.IsValidIndex(SlotIndex) || !Container->Slots[SlotIndex].bOccupied)
	{
		return false;
	}
	const FGuid ItemId = Container->Slots[SlotIndex].ItemId;
	return IsBodyContainerItemHidden(ItemId) || IsBodyContainerItemSearching(ItemId);
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
	const FCodeBP2ContainerView* Container = Controller->GetProjection().Containers.FindByPredicate(
		[ContainerId](const FCodeBP2ContainerView& Value) { return Value.ContainerId == ContainerId; });
	if (!Container || !Container->Slots.IsValidIndex(SlotIndex) || !Container->Slots[SlotIndex].bOccupied)
	{
		OutError = TEXT("该尸体格没有可搜查的物品。");
		return false;
	}
	const FGuid ItemId = Container->Slots[SlotIndex].ItemId;
	if (!IsBodyContainerItemHidden(ItemId))
	{
		OutError = IsBodyContainerItemSearching(ItemId)
			? TEXT("该尸体物品正在搜查中。") : TEXT("该尸体物品已揭示，不能再次搜查。");
		return false;
	}
	FCodeBBodyContainerProjection UpdatedProjection;
	if (!BodyContainerPresentation->BeginItemSearch(ItemId, UpdatedProjection, OutError))
	{
		return false;
	}
	UpdateBodyContainerProjection(UpdatedProjection);
	return true;
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

bool UCodeBP3UIHostSubsystem::RequestHotbarBindFromSelectedItem(const int32 SlotIndex, FString& OutError)
{
	OutError.Reset();
	if (!HotbarPresentation.IsSet() || !HotbarPresentation->Projection.bEditable
		|| !HotbarPresentation->Bind || !Controller.IsValid()
		|| !Controller->GetSelectedAddress().IsSet())
	{
		OutError = TEXT("快捷栏绑定入口当前不可用。请选择基础快捷物品区中的 QuickUsable 物品。");
		return false;
	}
	const FCodeBP3SlotAddress& Selected = Controller->GetSelectedAddress().GetValue();
	const FCodeBP2Projection& Projection = Controller->GetProjection();
	const FCodeBP2ContainerView* BasicContainer = Projection.Containers.FindByPredicate(
		[&Projection](const FCodeBP2ContainerView& Candidate)
		{ return Candidate.ContainerId == Projection.BasicContainerId; });
	if (!BasicContainer || Selected.ContainerId != Projection.BasicContainerId
		|| !BasicContainer->Slots.IsValidIndex(Selected.SlotIndex))
	{
		OutError = TEXT("快捷栏只能引用基础快捷物品区中的物品。");
		return false;
	}
	const FCodeBP2SlotView& Slot = BasicContainer->Slots[Selected.SlotIndex];
	if (!Selected.bOccupied || !Slot.bOccupied || Slot.ItemId != Selected.ItemId || !Slot.bQuickUsable)
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

bool UCodeBP3UIHostSubsystem::RequestHotbarUnbind(const int32 SlotIndex, FString& OutError)
{
	OutError.Reset();
	if (!HotbarPresentation.IsSet() || !HotbarPresentation->Projection.bEditable || !HotbarPresentation->Unbind)
	{
		OutError = TEXT("快捷栏解绑入口当前不可用。");
		return false;
	}
	FCodeBHotbarProjection UpdatedProjection;
	if (!HotbarPresentation->Unbind(SlotIndex, UpdatedProjection, OutError))
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
