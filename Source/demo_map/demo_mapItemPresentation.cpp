#include "demo_mapItemPresentation.h"

#include "demo_map.h"
#include "demo_mapItemDefinitions.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"

namespace
{
	const Fdemo_mapProfilePreparationStashRow* FindRow(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		const FGuid& ItemId)
	{
		return ItemId.IsValid()
			? Snapshot.OrderedPermanentStashRows.FindByPredicate(
				[&ItemId](
					const Fdemo_mapProfilePreparationStashRow& Candidate)
				{
					return Candidate.ItemInstanceId == ItemId;
				})
			: nullptr;
	}

	FString JoinEffects(
		const TArray<Fdemo_mapItemEffectParameter>& Effects)
	{
		FString Result;
		for (const Fdemo_mapItemEffectParameter& Effect : Effects)
		{
			if (!Result.IsEmpty()) Result += TEXT("；");
			Result += FString::Printf(
				TEXT("%s = %.2f"),
				*Effect.ParameterId.ToString(),
				Effect.Value);
		}
		return Result.IsEmpty() ? TEXT("—") : Result;
	}

	/**
	 * The grid deliberately uses a short visual token instead of the long
	 * definition/world-presentation identifiers.  Those identifiers are useful
	 * to the data layer, but wrap into illegible text in a 58 px inventory cell.
	 */
	FString GridIconForDefinition(FName DefinitionId, bool bOccupied)
	{
		if (!bOccupied)
		{
			return TEXT("＋");
		}
		const FString Id = DefinitionId.ToString();
		if (Id.Contains(TEXT(".Weapon."))) return TEXT("WPN");
		if (Id.Contains(TEXT(".Armor."))) return TEXT("ARM");
		if (Id.Contains(TEXT(".Accessory."))) return TEXT("ACC");
		if (Id.Contains(TEXT(".Backpack."))) return TEXT("BAG");
		if (Id.Contains(TEXT(".Consumable."))) return TEXT("MED");
		if (Id.Contains(TEXT(".Material."))) return TEXT("MAT");
		if (Id.Contains(TEXT("SoulBone"))) return TEXT("BNE");
		if (Id.Contains(TEXT("SpiritOre"))) return TEXT("ORE");
		return TEXT("ITM");
	}
}

Fdemo_mapWarehouseView Fdemo_mapItemPresentation::BuildWarehouseView(
	const Fdemo_mapProfilePreparationSnapshot& Snapshot)
{
	Fdemo_mapWarehouseView View;
	Fdemo_mapPersistentProfile ProjectionProfile;
	ProjectionProfile.WarehouseLayout = Snapshot.WarehouseLayout;
	for (const Fdemo_mapProfilePreparationStashRow& Row :
		Snapshot.OrderedPermanentStashRows)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = Row.ItemInstanceId;
		Item.ItemDefinitionId = Row.ItemDefinitionId;
		Item.StackCount = Row.StackCount;
		ProjectionProfile.PermanentStash.Add(Item);
	}
	bool bOverflow = false;
	const TArray<FGuid> SlotIds =
		Fdemo_mapWarehouseSlotProjection::Build(
			ProjectionProfile,
			&bOverflow);
	for (int32 Index = 0; Index < SlotIds.Num(); ++Index)
	{
		View.Slots.Add(BuildCell(
			FindRow(Snapshot, SlotIds[Index]),
			Index));
		if (SlotIds[Index].IsValid())
		{
			++View.UsedSlots;
		}
	}
	View.RemainingSlots =
		FMath::Max(0, View.CurrentCapacity - View.UsedSlots);
	View.bValid = !bOverflow;
	if (bOverflow)
	{
		View.Diagnostic =
			TEXT("仓库物品超过当前 30 格容量；请先处理溢出物品。");
	}
	return View;
}

Fdemo_mapUnifiedItemCellView Fdemo_mapItemPresentation::BuildCell(
	const Fdemo_mapProfilePreparationStashRow* Row,
	int32 SlotIndex)
{
	Fdemo_mapUnifiedItemCellView Cell;
	Cell.SlotIndex = SlotIndex;
	if (!Row)
	{
		Cell.DisplayName = TEXT("空格");
		Cell.IconLabel = TEXT("□");
		Cell.QualityLabel = TEXT("空");
		return Cell;
	}
	Cell.ItemInstanceId = Row->ItemInstanceId;
	Cell.ItemDefinitionId = Row->ItemDefinitionId;
	Cell.Quantity = Row->StackCount;
	Cell.bOccupied = Row->ItemInstanceId.IsValid();
	Cell.bSelected = Row->bSelected;
	Cell.bJackpotOrSpecial =
		Row->RewardEventKind == Edemo_mapRewardEventKind::Jackpot
		|| Row->RareRewardEventId.IsValid();
	if (const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Row->ItemDefinitionId))
	{
		Cell.DisplayName = Definition->DisplayName.ToString();
		Cell.IconLabel = Definition->WorldPresentationId.IsNone()
			? TEXT("◇")
			: Definition->WorldPresentationId.ToString().Left(2);
		Cell.LevelLabel = Definition->Level > 0
			? FString::Printf(TEXT("%d阶"), Definition->Level)
			: TEXT("基础");
		Cell.QualityLabel = Row->RareRewardTierId.IsNone()
			? (Row->AffixSet.Affixes.IsEmpty()
				? TEXT("普通")
				: TEXT("词条"))
			: Row->RareRewardTierId.ToString();
	}
	else
	{
		Cell.DisplayName = Row->ItemDefinitionId.ToString();
		Cell.IconLabel = TEXT("?");
		Cell.LevelLabel = TEXT("未知");
		Cell.QualityLabel = TEXT("缺失定义");
		Cell.bUnavailable = true;
	}
	return Cell;
}

Fdemo_mapUnifiedItemCellView Fdemo_mapItemPresentation::BuildCell(
	const Fdemo_mapEntityItemSlotView& Cell)
{
	Fdemo_mapUnifiedItemCellView Result;
	Result.SlotIndex = Cell.SlotIndex;
	Result.ItemInstanceId = Cell.ItemInstanceId;
	Result.ItemDefinitionId = Cell.ItemDefinitionId;
	Result.DisplayName =
		Cell.bOccupied ? Cell.DisplayName : TEXT("空格");
	Result.IconLabel = Cell.bOccupied ? TEXT("◇") : TEXT("□");
	Result.LevelLabel = Cell.LevelAndQualityLabel;
	Result.QualityLabel =
		Cell.bOccupied ? Cell.LevelAndQualityLabel : TEXT("空");
	Result.Quantity = Cell.Quantity;
	Result.bOccupied = Cell.bOccupied;
	Result.bSelected = Cell.bSelected;
	Result.bUnavailable = !Cell.bCanPlace;
	return Result;
}

Fdemo_mapUnifiedItemDetailView Fdemo_mapItemPresentation::BuildDetail(
	const Fdemo_mapProfilePreparationStashRow& Row)
{
	Fdemo_mapUnifiedItemDetailView Detail;
	Detail.ItemInstanceId = Row.ItemInstanceId;
	Detail.ItemDefinitionId = Row.ItemDefinitionId;
	Detail.Quantity = Row.StackCount;
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Row.ItemDefinitionId);
	if (!Definition)
	{
		Detail.DisplayName = Row.ItemDefinitionId.ToString();
		Detail.Description = TEXT("物品定义缺失，无法显示正式详情。");
		return Detail;
	}

	Detail.DisplayName = Definition->DisplayName.ToString();
	Detail.IconLabel = Definition->WorldPresentationId.IsNone()
		? TEXT("占位图")
		: Definition->WorldPresentationId.ToString();
	Detail.LevelLabel = Definition->Level > 0
		? FString::Printf(TEXT("%d阶"), Definition->Level)
		: TEXT("基础");
	Detail.TypeLabel = Definition->CategoryId.ToString();
	Detail.QualityLabel = Row.RareRewardTierId.IsNone()
		? (Row.AffixSet.Affixes.IsEmpty()
			? TEXT("普通")
			: TEXT("带随机词条"))
		: Row.RareRewardTierId.ToString();
	Detail.Description = TEXT("暂无独立描述数据");
	Detail.BaseAttributes = Definition->Modifiers.IsEmpty()
		? TEXT("—")
		: FString::Printf(
			TEXT("%d 项基础属性修正"),
			Definition->Modifiers.Num());
	Detail.RandomAffixes = Row.AffixSet.Affixes.IsEmpty()
		? TEXT("—")
		: FString::Printf(
			TEXT("%d 条（%s）"),
			Row.AffixSet.Affixes.Num(),
			*Row.AffixSet.AffixPolicyId.ToString());
	Detail.UseEffect = JoinEffects(Definition->EffectParameters);
	Detail.SellValue = Definition->bSellable
		? FString::Printf(
			TEXT("%lld"),
			Definition->SellPrice * FMath::Max(1, Row.StackCount))
		: TEXT("不可出售");
	Detail.AllowedPositions = Definition->CompatibleSlotIds.IsEmpty()
		? TEXT("仓库 / 战备物品区")
		: FString::JoinBy(
			Definition->CompatibleSlotIds,
			TEXT("、"),
			[](FName SlotId)
			{
				return SlotId.ToString();
			});
	Detail.TagsAndStatus = FString::Printf(
		TEXT("%s%s%s"),
		Row.bSafeInPermanentStash ? TEXT("永久仓库") : TEXT("非永久域"),
		Row.bSelected ? TEXT(" / 已加入战备") : TEXT(" / 仓库中"),
		Row.RewardEventKind == Edemo_mapRewardEventKind::Jackpot
			? TEXT(" / Jackpot")
			: TEXT(""));
	Detail.bValid = true;
	return Detail;
}

Fdemo_mapUnifiedItemDetailView Fdemo_mapItemPresentation::BuildDetail(
	const Fdemo_mapItemInstance& Item)
{
	Fdemo_mapProfilePreparationStashRow Row;
	Row.ItemInstanceId = Item.InstanceId;
	Row.ItemDefinitionId = Item.DefinitionId;
	Row.StackCount = Item.Quantity;
	Row.RewardEventKind = Item.RewardEventKind;
	Row.RewardEventId = Item.RewardEventId;
	Row.RewardValueMultiplierBps = Item.RewardValueMultiplierBps;
	Row.RewardSourceRoleId = Item.RewardSourceRoleId;
	Row.RareRewardEventId = Item.RareRewardEventId;
	Row.RareRewardPolicyId = Item.RareRewardPolicyId;
	Row.RareRewardTierId = Item.RareRewardTierId;
	Row.RareRewardBonusValue = Item.RareRewardBonusValue;
	Row.AffixSet = Item.AffixSet;
	Row.bSelected = Item.OwnershipState
		== Edemo_mapItemOwnershipState::Inventory
		|| Item.OwnershipState
			== Edemo_mapItemOwnershipState::Equipped;
	Fdemo_mapUnifiedItemDetailView Detail = BuildDetail(Row);
	if (Detail.bValid)
	{
		Detail.TagsAndStatus = FString::Printf(
			TEXT("Runtime / %s / %s"),
			*Item.OwnerId.ToString(),
			*Item.ContainerId.ToString());
	}
	return Detail;
}

Fdemo_mapResolvedItemActions Fdemo_mapItemPresentation::ResolveActions(
	const Fdemo_mapProfilePreparationStashRow& Row,
	Edemo_mapItemPresentationContext Context)
{
	Fdemo_mapResolvedItemActions Result;
	if (!Row.ItemInstanceId.IsValid())
	{
		return Result;
	}
	Result.Actions.Add(Edemo_mapItemContextAction::ViewDetails);
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Row.ItemDefinitionId);
	if (!Definition)
	{
		return Result;
	}
	if (Context == Edemo_mapItemPresentationContext::Warehouse
		|| Context == Edemo_mapItemPresentationContext::TeleportWarehouse)
	{
		Result.Actions.Add(Edemo_mapItemContextAction::Move);
		if (!Definition->CompatibleSlotIds.IsEmpty() && !Row.bSelected)
		{
			Result.Actions.Add(Edemo_mapItemContextAction::Equip);
		}
	}
	if (Row.bSelected)
	{
		if (!Definition->CompatibleSlotIds.IsEmpty())
		{
			Result.Actions.Add(Edemo_mapItemContextAction::Unequip);
		}
		if (Context
				== Edemo_mapItemPresentationContext::TeleportQuickItems
			&& Row.bInBaseQuickItemArea
			&& Definition->CategoryId
				== Fdemo_mapItemIds::ConsumableCategory
			&& Row.StackCount > 0)
		{
			Result.Actions.Add(
				Edemo_mapItemContextAction::EquipToHotbar);
		}
		Result.Actions.Add(
			Edemo_mapItemContextAction::ReturnToWarehouse);
	}
	return Result;
}

Fdemo_mapResolvedItemActions
Fdemo_mapItemPresentation::ResolveRuntimeActions(
	const Fdemo_mapItemInstance& Item,
	Edemo_mapItemPresentationContext Context)
{
	Fdemo_mapResolvedItemActions Result;
	if (!Item.InstanceId.IsValid()
		|| Item.OwnershipState == Edemo_mapItemOwnershipState::Destroyed)
	{
		return Result;
	}
	Result.Actions.Add(Edemo_mapItemContextAction::ViewDetails);
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Item.DefinitionId);
	if (!Definition)
	{
		return Result;
	}
	if (Context == Edemo_mapItemPresentationContext::RuntimeEquipment)
	{
		Result.Actions.Add(Edemo_mapItemContextAction::Unequip);
		return Result;
	}
	if (!Definition->CompatibleSlotIds.IsEmpty())
	{
		Result.Actions.Add(Edemo_mapItemContextAction::Equip);
	}
	if (Context
		== Edemo_mapItemPresentationContext::RuntimeBaseQuickItems
		|| Context == Edemo_mapItemPresentationContext::RuntimeRingQuickItems)
	{
		Result.Actions.Add(
			Edemo_mapItemContextAction::MoveToSpatialStorage);
		if (Definition->CategoryId
			== Fdemo_mapItemIds::ConsumableCategory)
		{
			Result.Actions.Add(
				Edemo_mapItemContextAction::EquipToHotbar);
		}
	}
	else if (Context
		== Edemo_mapItemPresentationContext::RuntimeSpatialStorage)
	{
		Result.Actions.Add(
			Edemo_mapItemContextAction::MoveToBaseQuickItems);
	}
	if (Definition->CategoryId
			== Fdemo_mapItemIds::ConsumableCategory
		&& Item.Quantity > 0)
	{
		Result.Actions.Add(Edemo_mapItemContextAction::Use);
	}
	return Result;
}

TArray<Fdemo_mapHotbarSlotView>
Fdemo_mapItemPresentation::BuildPreparationHotbar(
	const Fdemo_mapProfilePreparationSnapshot& Snapshot)
{
	TArray<Fdemo_mapHotbarSlotView> Result;
	Result.Reserve(Fdemo_mapHotbarBindingSnapshot::SlotCount);
	for (int32 Index = 0;
		Index < Fdemo_mapHotbarBindingSnapshot::SlotCount;
		++Index)
	{
		Fdemo_mapHotbarSlotView Slot;
		Slot.SlotNumber = Index + 1;
		Slot.IconLabel = TEXT("□");
		Slot.DisplayName = TEXT("空槽");
		Slot.StateLabel = TEXT("空槽");
		if (!Snapshot.HotbarBindings.SlotBindings.IsValidIndex(Index)
			|| !Snapshot.HotbarBindings.SlotBindings[Index].IsValid())
		{
			Result.Add(MoveTemp(Slot));
			continue;
		}

		Slot.ItemInstanceId =
			Snapshot.HotbarBindings.SlotBindings[Index];
		const Fdemo_mapProfilePreparationStashRow* Row =
			FindRow(Snapshot, Slot.ItemInstanceId);
		const Fdemo_mapItemDefinition* Definition = Row
			? Fdemo_mapItemDefinitions::Find(Row->ItemDefinitionId)
			: nullptr;
		if (!Row || !Definition)
		{
			Slot.IconLabel = TEXT("!");
			Slot.DisplayName = TEXT("失效绑定");
			Slot.StateLabel = TEXT("绑定失效");
			Slot.State =
				Edemo_mapHotbarSlotState::InvalidBinding;
			Result.Add(MoveTemp(Slot));
			continue;
		}

		Slot.ItemDefinitionId = Row->ItemDefinitionId;
		Slot.DisplayName = Definition->DisplayName.ToString();
		Slot.IconLabel = TEXT("◇");
		Slot.Quantity = Row->StackCount;
		const int32 CarriedIndex =
			Snapshot.OrderedSelectedMaterialIds.IndexOfByKey(
				Row->ItemInstanceId);
		const bool bEligible =
			CarriedIndex >= 0
			&& CarriedIndex
				< Fdemo_mapPersistentPreparationLayout::
					BaseQuickItemSlotCount
			&& Definition->CategoryId
				== Fdemo_mapItemIds::ConsumableCategory
			&& Row->StackCount > 0;
		Slot.State = bEligible
			? Edemo_mapHotbarSlotState::Ready
			: Edemo_mapHotbarSlotState::InvalidBinding;
		Slot.StateLabel = bEligible
			? TEXT("可使用")
			: TEXT("绑定失效");
		Result.Add(MoveTemp(Slot));
	}
	return Result;
}

TArray<Fdemo_mapHotbarSlotView>
Fdemo_mapItemPresentation::BuildRuntimeHotbar(
	const Fdemo_mapHotbarBindingSnapshot& Bindings,
	const Fdemo_mapItemAuthority& Authority,
	const Fdemo_mapItemUseCooldownSnapshot& Cooldown)
{
	TArray<Fdemo_mapHotbarSlotView> Result;
	Result.Reserve(Fdemo_mapHotbarBindingSnapshot::SlotCount);
	for (int32 Index = 0;
		Index < Fdemo_mapHotbarBindingSnapshot::SlotCount;
		++Index)
	{
		Fdemo_mapHotbarSlotView Slot;
		Slot.SlotNumber = Index + 1;
		Slot.IconLabel = TEXT("□");
		Slot.DisplayName = TEXT("空槽");
		Slot.StateLabel = TEXT("空槽");
		if (!Bindings.SlotBindings.IsValidIndex(Index)
			|| !Bindings.SlotBindings[Index].IsValid())
		{
			Result.Add(MoveTemp(Slot));
			continue;
		}

		Slot.ItemInstanceId = Bindings.SlotBindings[Index];
		const Fdemo_mapItemInstance* Item =
			Authority.FindInstance(Slot.ItemInstanceId);
		const Fdemo_mapItemDefinition* Definition = Item
			? Fdemo_mapItemDefinitions::Find(Item->DefinitionId)
			: nullptr;
		if (!Item || !Definition)
		{
			Slot.IconLabel = TEXT("!");
			Slot.DisplayName = TEXT("失效绑定");
			Slot.StateLabel = TEXT("绑定失效");
			Slot.State =
				Edemo_mapHotbarSlotState::InvalidBinding;
			Result.Add(MoveTemp(Slot));
			continue;
		}

		Slot.ItemDefinitionId = Item->DefinitionId;
		Slot.DisplayName = Definition->DisplayName.ToString();
		Slot.IconLabel = TEXT("◇");
		Slot.Quantity = Item->Quantity;
		const bool bUsable =
			Fdemo_mapItemViewRules::IsHotbarBindable(
				*Item,
				*Definition)
			&& Authority.FindInventorySlot(Item->InstanceId) >= 0
			&& Authority.FindInventorySlot(Item->InstanceId)
				< Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount;
		if (!bUsable)
		{
			Slot.State =
				Edemo_mapHotbarSlotState::Unavailable;
			Slot.StateLabel = TEXT("不可使用");
		}
		else if (Cooldown.bActive)
		{
			Slot.State = Edemo_mapHotbarSlotState::Cooldown;
			Slot.CooldownRemaining = Cooldown.RemainingSeconds;
			Slot.StateLabel = FString::Printf(
				TEXT("冷却 %.1fs"),
				Cooldown.RemainingSeconds);
		}
		else
		{
			Slot.State = Edemo_mapHotbarSlotState::Ready;
			Slot.StateLabel = TEXT("可使用");
		}
		Result.Add(MoveTemp(Slot));
	}
	return Result;
}

FString Fdemo_mapItemPresentation::BuildHotbarSlotLabel(
	const Fdemo_mapHotbarSlotView& Slot,
	const FString& KeyLabel)
{
	return Slot.IsOccupied()
		? FString::Printf(
			TEXT("[%s] %s %s\n×%d · %s"),
			*KeyLabel,
			*Slot.IconLabel,
			*Slot.DisplayName,
			Slot.Quantity,
			*Slot.StateLabel)
		: FString::Printf(
			TEXT("[%s] □ 空槽\n—"),
			*KeyLabel);
}

FString Fdemo_mapItemPresentation::ActionLabel(
	Edemo_mapItemContextAction Action)
{
	switch (Action)
	{
	case Edemo_mapItemContextAction::ViewDetails:
		return TEXT("查看详情");
	case Edemo_mapItemContextAction::Equip:
		return TEXT("装备");
	case Edemo_mapItemContextAction::Unequip:
		return TEXT("卸下");
	case Edemo_mapItemContextAction::Move:
		return TEXT("调整位置");
	case Edemo_mapItemContextAction::ReturnToWarehouse:
		return TEXT("返回仓库");
	case Edemo_mapItemContextAction::EquipToHotbar:
		return TEXT("装备到快捷使用栏");
	case Edemo_mapItemContextAction::MoveToBaseQuickItems:
		return TEXT("移至基础快捷区");
	case Edemo_mapItemContextAction::MoveToSpatialStorage:
		return TEXT("移至空间道具区");
	case Edemo_mapItemContextAction::Use:
		return TEXT("使用");
	default:
		return TEXT("操作");
	}
}

void Udemo_mapItemCellWidget::InitializeCell(
	const Fdemo_mapUnifiedItemCellView& InView,
	Fdemo_mapItemCellActivated InActivated,
	Edemo_mapItemPresentationContext InContext,
	Fdemo_mapItemCellDropped InDropped,
	Fdemo_mapItemCellContextRequested InContextRequested,
	Fdemo_mapItemCellDoubleClicked InDoubleClicked,
	bool bInOpenNestedContainerOnClick)
{
	View = InView;
	Activated = MoveTemp(InActivated);
	Context = InContext;
	Dropped = MoveTemp(InDropped);
	ContextRequested = MoveTemp(InContextRequested);
	DoubleClicked = MoveTemp(InDoubleClicked);
	bOpenNestedContainerOnClick = bInOpenNestedContainerOnClick;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		Initialize();
	}
	BuildInterface();
	RefreshText();
}

void Udemo_mapItemCellWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildInterface();
}

FReply Udemo_mapItemCellWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton
		&& View.bOccupied && !View.bUnavailable && ContextRequested.IsBound())
	{
		ContextRequested.Execute(
			View.SlotIndex,
			View.ItemInstanceId,
			Context,
			InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& !View.bUnavailable)
	{
		// Runtime equipment cells use the stable 100-based source range.  The
		// P6 player has weapon, robe, one accessory, then the spatial bag; the
		// target corpse keeps its spatial equipment entry at source slot 3.
		// Keep this identity check alongside the explicit presenter hint so a
		// rebuilt cell cannot lose its only nested-container entry gesture.
		const bool bSpatialBagEntry = bOpenNestedContainerOnClick
			|| (Context == Edemo_mapItemPresentationContext::RuntimeEquipment
				&& View.SlotIndex == 103)
			|| (Context
				== Edemo_mapItemPresentationContext::SearchContainerEquipment
				&& View.SlotIndex == 3);
		UE_LOG(Logdemo_map, Log,
			TEXT("XFIX1_BAG_INPUT left slot=%d context=%d occupied=%d hinted=%d spatial=%d double=%d"),
			View.SlotIndex,
			static_cast<int32>(Context),
			View.bOccupied ? 1 : 0,
			bOpenNestedContainerOnClick ? 1 : 0,
			bSpatialBagEntry ? 1 : 0,
			DoubleClicked.IsBound() ? 1 : 0);
		if (View.bOccupied && bSpatialBagEntry
			&& DoubleClicked.IsBound())
		{
			// A UMG drag detector owns left-button-up before an outer widget can
			// reliably observe it. Bags are entry points rather than a drop
			// target, so open their draggable nested window immediately on the
			// real icon press; every contained item still uses full drag/drop.
			DoubleClicked.Execute(View.SlotIndex);
			return FReply::Handled();
		}
		// A draggable UMG cell starts DetectDragIfPressed during its preview
		// mouse-down path.  Keep a short real-input press history here so the
		// drag detector cannot swallow the bag-opening gesture before the
		// regular NativeOnMouseButtonDoubleClick override receives it.
		const double CurrentPressSeconds = FPlatformTime::Seconds();
		const FVector2D CurrentPressPosition =
			InMouseEvent.GetScreenSpacePosition();
		const bool bRepeatedPress = LastLeftPressSeconds >= 0.0
			&& CurrentPressSeconds - LastLeftPressSeconds <= 0.8
			&& FVector2D::DistSquared(
				CurrentPressPosition,
				LastLeftPressScreenPosition) <= 64.0f;
		LastLeftPressSeconds = CurrentPressSeconds;
		LastLeftPressScreenPosition = CurrentPressPosition;
		if (bRepeatedPress && View.bOccupied && DoubleClicked.IsBound())
		{
			LastLeftPressSeconds = -1.0;
			DoubleClicked.Execute(View.SlotIndex);
			return FReply::Handled();
		}
		// Preserve the click-selection affordance even when a held mouse turns
		// into a drag before UButton can emit OnClicked.
		// A selected-cell refresh reconstructs the UMG cell tree.  Leave cells
		// that own a double-click action stable after the first press, otherwise
		// the second press lands on a new widget and cannot open its bag.
		if (Activated.IsBound() && !DoubleClicked.IsBound())
		{
			Activated.Execute(View.SlotIndex);
		}
		if (View.bOccupied && View.bAllowDrag && Dropped.IsBound())
		{
			return UWidgetBlueprintLibrary::DetectDragIfPressed(
				InMouseEvent,
				this,
				EKeys::LeftMouseButton).NativeReply;
		}
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply Udemo_mapItemCellWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	const FReply SuperReply = Super::NativeOnMouseButtonDown(
		InGeometry,
		InMouseEvent);
	return View.bOccupied && !View.bUnavailable && View.bAllowDrag
		&& Dropped.IsBound()
		? UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent,
			this,
			EKeys::LeftMouseButton).NativeReply
		: SuperReply;
}

FReply Udemo_mapItemCellWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& View.bOccupied && !View.bUnavailable && DoubleClicked.IsBound())
	{
		DoubleClicked.Execute(View.SlotIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void Udemo_mapItemCellWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	if (!View.bOccupied || View.bUnavailable || !View.bAllowDrag
		|| !Dropped.IsBound())
	{
		return;
	}
	Udemo_mapItemDragDropOperation* Operation =
		NewObject<Udemo_mapItemDragDropOperation>(this);
	Operation->SourceSlotIndex = View.SlotIndex;
	Operation->SourceItemInstanceId = View.ItemInstanceId;
	Operation->SourceContext = Context;
	Operation->Payload = this;
	USizeBox* DragSize = NewObject<USizeBox>(Operation);
	// Keep the drag visual proportional to the readable grid square below.
	// A larger visual also makes it clear that the complete item stack is
	// travelling, while the ItemAuthority still owns the actual transfer.
	DragSize->SetWidthOverride(76.0f);
	DragSize->SetHeightOverride(76.0f);
	UBorder* DragCard = NewObject<UBorder>(Operation);
	DragCard->SetBrushColor(FLinearColor(0.08f, 0.68f, 0.86f, 0.92f));
	DragCard->SetPadding(FMargin(3.0f));
	DragSize->SetContent(DragCard);
	UVerticalBox* DragColumn = NewObject<UVerticalBox>(Operation);
	DragCard->SetContent(DragColumn);
	UTextBlock* DragIcon = NewObject<UTextBlock>(Operation);
	DragIcon->SetText(FText::FromString(
		GridIconForDefinition(View.ItemDefinitionId, View.bOccupied)));
	FSlateFontInfo DragIconFont = DragIcon->GetFont();
	DragIconFont.Size = 24;
	DragIcon->SetFont(DragIconFont);
	DragIcon->SetJustification(ETextJustify::Center);
	DragColumn->AddChildToVerticalBox(DragIcon);
	UTextBlock* DragCount = NewObject<UTextBlock>(Operation);
	DragCount->SetText(FText::FromString(FString::Printf(TEXT("×%d"), View.Quantity)));
	FSlateFontInfo DragCountFont = DragCount->GetFont();
	DragCountFont.Size = 12;
	DragCount->SetFont(DragCountFont);
	DragCount->SetJustification(ETextJustify::Center);
	DragColumn->AddChildToVerticalBox(DragCount);
	Operation->DefaultDragVisual = DragSize;
	Operation->Pivot = EDragPivot::MouseDown;
	OutOperation = Operation;
	View.bDragSource = true;
	RefreshText();
}

bool Udemo_mapItemCellWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const Udemo_mapItemDragDropOperation* Operation =
		Cast<Udemo_mapItemDragDropOperation>(InOperation);
	if (!Operation || !Dropped.IsBound() || !CanAcceptDrop(Operation))
	{
		// The red rejection state is shown while the icon hovers an invalid
		// square. Once it is released, Slate cancels back to the source and the
		// destination immediately returns to its normal idle appearance.
		View.bDragRejected = false;
		RefreshText();
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}
	View.bDragTarget = false;
	View.bDragRejected = false;
	RefreshText();
	Dropped.Execute(
		Operation->SourceSlotIndex,
		Operation->SourceItemInstanceId,
		Operation->SourceContext,
		View.SlotIndex,
		Context);
	return true;
}

void Udemo_mapItemCellWidget::NativeOnDragEnter(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	if (const Udemo_mapItemDragDropOperation* Operation =
		Cast<Udemo_mapItemDragDropOperation>(InOperation))
	{
		View.bDragTarget = CanAcceptDrop(Operation);
		View.bDragRejected = !View.bDragTarget;
		RefreshText();
	}
}

void Udemo_mapItemCellWidget::NativeOnDragLeave(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	View.bDragTarget = false;
	View.bDragRejected = false;
	View.bDragSource = false;
	RefreshText();
}

void Udemo_mapItemCellWidget::NativeOnDragCancelled(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	if (const Udemo_mapItemDragDropOperation* Operation =
		Cast<Udemo_mapItemDragDropOperation>(InOperation);
		Operation && Operation->SourceItemInstanceId == View.ItemInstanceId)
	{
		View.bDragSource = false;
		View.bDragRejected = false;
		RefreshText();
	}
}

bool Udemo_mapItemCellWidget::CanAcceptDrop(
	const Udemo_mapItemDragDropOperation* Operation) const
{
	if (!Operation || View.bUnavailable
		|| (Operation->SourceContext == Context
			&& Operation->SourceSlotIndex == View.SlotIndex))
	{
		return false;
	}
	auto IsPlayerContext = [](Edemo_mapItemPresentationContext Value)
	{
		return Value == Edemo_mapItemPresentationContext::RuntimeEquipment
			|| Value == Edemo_mapItemPresentationContext::RuntimeBaseQuickItems
			|| Value == Edemo_mapItemPresentationContext::RuntimeRingQuickItems
			|| Value == Edemo_mapItemPresentationContext::RuntimeSpatialStorage
			// The Warehouse preparation panes are the same player-owned
			// destinations as their runtime equivalents. Treating them as
			// non-player panes made every Warehouse-to-loadout drop reject.
			|| Value == Edemo_mapItemPresentationContext::TeleportEquipment
			|| Value == Edemo_mapItemPresentationContext::TeleportQuickItems
			|| Value == Edemo_mapItemPresentationContext::TeleportSpatialStorage;
	};
	if (Context == Edemo_mapItemPresentationContext::RuntimeWorldDrop)
	{
		return IsPlayerContext(Operation->SourceContext);
	}
	const bool bSourcePlayer = IsPlayerContext(Operation->SourceContext);
	const bool bTargetPlayer = IsPlayerContext(Context);
	const Udemo_mapItemCellWidget* SourceWidget =
		Cast<Udemo_mapItemCellWidget>(Operation->Payload);
	if (SourceWidget && !SourceWidget->GetCellView().bAllowDrag)
	{
		return false;
	}
	const Fdemo_mapItemDefinition* SourceDefinition = SourceWidget
		? Fdemo_mapItemDefinitions::Find(SourceWidget->View.ItemDefinitionId)
		: nullptr;
	auto IsCompatibleEquipmentTarget = [this, SourceDefinition](int32 EncodedSlot)
	{
		const int32 Index = EncodedSlot - 100;
		const TArray<FName>& EquipmentSlots =
			Fdemo_mapItemDefinitions::GetEquipmentSlotIds();
		return SourceDefinition && EquipmentSlots.IsValidIndex(Index)
			&& SourceDefinition->CompatibleSlotIds.Contains(EquipmentSlots[Index]);
	};
	if ((Context == Edemo_mapItemPresentationContext::RuntimeEquipment
		|| Context == Edemo_mapItemPresentationContext::TeleportEquipment)
		&& !IsCompatibleEquipmentTarget(View.SlotIndex))
	{
		return false;
	}
	if (Context == Edemo_mapItemPresentationContext::SearchContainerEquipment)
	{
		const TArray<FName>& EquipmentSlots =
			Fdemo_mapItemDefinitions::GetEquipmentSlotIds();
		return !View.bOccupied && SourceDefinition
			&& EquipmentSlots.IsValidIndex(View.SlotIndex)
			&& SourceDefinition->CompatibleSlotIds.Contains(
				EquipmentSlots[View.SlotIndex]);
	}
	// The UI only advertises empty destination squares.  Any incompatible or
	// occupied destination rejects the drop, so Slate cancels it back to source.
	return !View.bOccupied && (bSourcePlayer != bTargetPlayer
		|| (bSourcePlayer && bTargetPlayer));
}

void Udemo_mapItemCellWidget::BuildInterface()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}
	bBuilt = true;
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	// Inventory cells remain square and compact enough for multi-column grids,
	// but the old 48 px cells made icons and drag targets unnecessarily hard to
	// read at both supported resolutions.  Containers retain wrapping and
	// scrolling, so this does not change capacity or authority semantics.
	Size->SetWidthOverride(72.0f);
	Size->SetHeightOverride(72.0f);
	WidgetTree->RootWidget = Size;
	CellButton = WidgetTree->ConstructWidget<UButton>();
	Size->SetContent(CellButton);
	CellButton->OnClicked.AddDynamic(
		this,
		&Udemo_mapItemCellWidget::ClickCell);
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
	Card->SetBrushColor(
		FLinearColor(0.055f, 0.095f, 0.13f, 1.0f));
	Card->SetPadding(FMargin(2.0f));
	CellButton->SetContent(Card);
	UVerticalBox* Column =
		WidgetTree->ConstructWidget<UVerticalBox>();
	Card->SetContent(Column);
	NameText = WidgetTree->ConstructWidget<UTextBlock>();
	MetaText = WidgetTree->ConstructWidget<UTextBlock>();
	for (UTextBlock* Text : { NameText.Get(), MetaText.Get() })
	{
		Text->SetAutoWrapText(false);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(
			FSlateColor(FLinearColor::White));
		if (UVerticalBoxSlot* TextSlot =
			Column->AddChildToVerticalBox(Text))
		{
		TextSlot->SetPadding(FMargin(1.0f));
		}
	}
	FSlateFontInfo NameFont = NameText->GetFont();
	NameFont.Size = 22;
	NameText->SetFont(NameFont);
	FSlateFontInfo MetaFont = MetaText->GetFont();
	MetaFont.Size = 12;
	MetaText->SetFont(MetaFont);
}

void Udemo_mapItemCellWidget::RefreshText()
{
	if (!bBuilt) return;
	NameText->SetText(FText::FromString(
		View.bOccupied && !View.DisplayName.IsEmpty()
			? View.DisplayName.Left(3)
			: GridIconForDefinition(View.ItemDefinitionId, View.bOccupied)));
	MetaText->SetText(FText::FromString(
		View.bOccupied
			? FString::Printf(
				TEXT("×%d"), View.Quantity)
			: TEXT("空")));
	CellButton->SetIsEnabled(!View.bUnavailable);
	CellButton->SetBackgroundColor(
		View.bDragRejected
			? FLinearColor(0.72f, 0.18f, 0.18f, 1.0f)
			: (View.bDragTarget
			? FLinearColor(0.18f, 0.62f, 0.75f, 1.0f)
			: (View.bDragSource
				? FLinearColor(0.75f, 0.52f, 0.12f, 1.0f)
				: (View.bSelected
			? FLinearColor(0.20f, 0.48f, 0.32f, 1.0f)
			: (View.bUnavailable
				? FLinearColor(0.16f, 0.16f, 0.18f, 1.0f)
				: FLinearColor(0.10f, 0.20f, 0.28f, 1.0f))))));
}

void Udemo_mapItemCellWidget::ClickCell()
{
	// UButton's Clicked event is emitted only for a completed click, not a
	// drag.  It gives spatial bags a reliable accessible fallback while the
	// native double-click path remains available for mouse users.
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(View.ItemDefinitionId);
	if (View.bOccupied && Definition
		&& Definition->CategoryId == Fdemo_mapItemIds::BackpackCategory
		&& DoubleClicked.IsBound())
	{
		DoubleClicked.Execute(View.SlotIndex);
		return;
	}
	if (Activated.IsBound() && !DoubleClicked.IsBound())
	{
		Activated.Execute(View.SlotIndex);
	}
}
