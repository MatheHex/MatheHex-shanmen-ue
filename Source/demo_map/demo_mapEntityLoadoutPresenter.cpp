#include "demo_mapEntityLoadoutPresenter.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentWarehouseTransaction.h"

namespace
{
	const Fdemo_mapProfilePreparationStashRow* FindRow(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		const FGuid& ItemInstanceId)
	{
		return ItemInstanceId.IsValid()
			? Snapshot.OrderedPermanentStashRows.FindByPredicate(
				[&ItemInstanceId](
					const Fdemo_mapProfilePreparationStashRow& Candidate)
				{
					return Candidate.ItemInstanceId == ItemInstanceId;
				})
			: nullptr;
	}

	Fdemo_mapEntityItemSlotView MakeSlot(
		Edemo_mapEntityLoadoutRegion Region,
		int32 SlotIndex,
		const Fdemo_mapProfilePreparationStashRow* Row,
		bool bSelected)
	{
		Fdemo_mapEntityItemSlotView Slot;
		Slot.Region = Region;
		Slot.SlotIndex = SlotIndex;
		Slot.bSelected = bSelected;
		if (!Row)
		{
			return Slot;
		}

		Slot.ItemInstanceId = Row->ItemInstanceId;
		Slot.ItemDefinitionId = Row->ItemDefinitionId;
		Slot.Quantity = Row->StackCount;
		Slot.bOccupied = Row->ItemInstanceId.IsValid();
		if (const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Row->ItemDefinitionId))
		{
			Slot.DisplayName = Definition->DisplayName.ToString();
			Slot.LevelAndQualityLabel = Definition->Level > 0
				? FString::Printf(TEXT("%d阶"), Definition->Level)
				: TEXT("基础");
		}
		else
		{
			Slot.DisplayName = Row->ItemDefinitionId.ToString();
			Slot.LevelAndQualityLabel = TEXT("未知");
			Slot.bCanPlace = false;
		}
		return Slot;
	}

	Fdemo_mapEntityItemSlotView MakeRuntimeSlot(
		Edemo_mapEntityLoadoutRegion Region,
		int32 SlotIndex,
		const Fdemo_mapItemAuthority& Authority,
		const FGuid& ItemInstanceId)
	{
		Fdemo_mapEntityItemSlotView Slot;
		Slot.Region = Region;
		Slot.SlotIndex = SlotIndex;
		const Fdemo_mapItemInstance* Item =
			Authority.FindInstance(ItemInstanceId);
		if (!Item)
		{
			return Slot;
		}
		Slot.ItemInstanceId = Item->InstanceId;
		Slot.ItemDefinitionId = Item->DefinitionId;
		Slot.Quantity = Item->Quantity;
		Slot.bOccupied = Item->InstanceId.IsValid();
		if (const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Item->DefinitionId))
		{
			Slot.DisplayName = Definition->DisplayName.ToString();
			Slot.LevelAndQualityLabel = Definition->Level > 0
				? FString::Printf(TEXT("%d阶"), Definition->Level)
				: TEXT("基础");
		}
		else
		{
			Slot.DisplayName = Item->DefinitionId.ToString();
			Slot.LevelAndQualityLabel = TEXT("未知");
			Slot.bCanPlace = false;
		}
		return Slot;
	}

	void AddEquipmentSlot(
		Fdemo_mapEntityLoadoutView& View,
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		Edemo_mapEntityLoadoutRegion Region,
		int32 SlotIndex,
		const FGuid& ItemInstanceId)
	{
		View.EquipmentSlots.Add(MakeSlot(
			Region,
			SlotIndex,
			FindRow(Snapshot, ItemInstanceId),
			ItemInstanceId.IsValid()));
	}
}

bool Fdemo_mapEntityLoadoutRules::CanFitSelectedItems(
	int32 SelectedItemCount,
	FName SpatialItemDefinitionId,
	FString* OutDiagnostic,
	FName SpatialRingDefinitionId)
{
	const Fdemo_mapInventoryCapacityResult Capacity =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			SpatialItemDefinitionId,
			SpatialRingDefinitionId);
	if (!Capacity.bSuccess)
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic = Capacity.Diagnostic;
		}
		return false;
	}
	if (SelectedItemCount < 0 || SelectedItemCount > Capacity.Capacity)
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic = FString::Printf(
				TEXT("当前出战物品占用 %d 格，但目标空间道具只允许 %d 格；请先移回仓库。"),
				SelectedItemCount,
				Capacity.Capacity);
		}
		return false;
	}
	if (OutDiagnostic)
	{
		OutDiagnostic->Reset();
	}
	return true;
}

Fdemo_mapEntityLoadoutView
Fdemo_mapEntityLoadoutPresenter::BuildPlayerPreparationView(
	const Fdemo_mapProfilePreparationSnapshot& Snapshot,
	int32 AccessorySlotCount)
{
	Fdemo_mapEntityLoadoutView View;
	View.AccessorySlotCount = FMath::Max(0, AccessorySlotCount);
	View.BaseQuickItemCapacity =
		Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount;

	AddEquipmentSlot(
		View,
		Snapshot,
		Edemo_mapEntityLoadoutRegion::Weapon,
		0,
		Snapshot.SelectedWeaponId);
	AddEquipmentSlot(
		View,
		Snapshot,
		Edemo_mapEntityLoadoutRegion::Armor,
		0,
		Snapshot.SelectedArmorId);
	for (int32 Index = 0; Index < View.AccessorySlotCount; ++Index)
	{
		AddEquipmentSlot(
			View,
			Snapshot,
			Edemo_mapEntityLoadoutRegion::Accessory,
			Index,
			Index == 0 ? Snapshot.SelectedAccessoryId : FGuid());
	}
	AddEquipmentSlot(
		View,
		Snapshot,
		Edemo_mapEntityLoadoutRegion::SpatialRing,
		0,
		Snapshot.SelectedSpatialRingId);
	AddEquipmentSlot(
		View,
		Snapshot,
		Edemo_mapEntityLoadoutRegion::SpatialItem,
		0,
		Snapshot.SelectedBackpackId);

	FName SpatialItemDefinitionId = NAME_None;
	if (const Fdemo_mapProfilePreparationStashRow* SpatialItemRow =
		FindRow(Snapshot, Snapshot.SelectedBackpackId))
	{
		SpatialItemDefinitionId = SpatialItemRow->ItemDefinitionId;
	}
	FName SpatialRingDefinitionId = NAME_None;
	if (const Fdemo_mapProfilePreparationStashRow* SpatialRingRow =
		FindRow(Snapshot, Snapshot.SelectedSpatialRingId))
	{
		SpatialRingDefinitionId = SpatialRingRow->ItemDefinitionId;
	}
	const Fdemo_mapInventoryCapacityResult TotalCapacity =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			SpatialItemDefinitionId,
			SpatialRingDefinitionId);
	const Fdemo_mapSpatialStorageCapacityResult SpatialCapacity =
		Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
			SpatialItemDefinitionId);
	View.RingQuickItemCapacity =
		TotalCapacity.bSuccess ? TotalCapacity.RingQuickCapacity : 0;
	View.SpatialStorageCapacity =
		SpatialCapacity.bSuccess ? SpatialCapacity.Capacity : 0;
	View.TotalCarriedCapacity =
		TotalCapacity.bSuccess
			? TotalCapacity.Capacity
			: View.BaseQuickItemCapacity;
	View.UsedCarriedSlots =
		Snapshot.OrderedSelectedMaterialIds.Num();

	for (int32 Index = 0;
		Index < View.BaseQuickItemCapacity;
		++Index)
	{
		const FGuid ItemId =
			Snapshot.OrderedSelectedMaterialIds.IsValidIndex(Index)
				? Snapshot.OrderedSelectedMaterialIds[Index]
				: FGuid();
		View.BaseQuickItemSlots.Add(MakeSlot(
			Edemo_mapEntityLoadoutRegion::BaseQuickItem,
			Index,
			FindRow(Snapshot, ItemId),
			ItemId.IsValid()));
	}
	for (int32 Index = 0;
		Index < View.RingQuickItemCapacity;
		++Index)
	{
		const int32 SelectedIndex = View.BaseQuickItemCapacity + Index;
		const FGuid ItemId =
			Snapshot.OrderedSelectedMaterialIds.IsValidIndex(SelectedIndex)
				? Snapshot.OrderedSelectedMaterialIds[SelectedIndex]
				: FGuid();
		View.RingQuickItemSlots.Add(MakeSlot(
			Edemo_mapEntityLoadoutRegion::RingQuickItem,
			Index,
			FindRow(Snapshot, ItemId),
			ItemId.IsValid()));
	}
	for (int32 Index = 0;
		Index < View.SpatialStorageCapacity;
		++Index)
	{
		const int32 SelectedIndex =
			View.BaseQuickItemCapacity + View.RingQuickItemCapacity + Index;
		const FGuid ItemId =
			Snapshot.OrderedSelectedMaterialIds.IsValidIndex(SelectedIndex)
				? Snapshot.OrderedSelectedMaterialIds[SelectedIndex]
				: FGuid();
		View.SpatialStorageSlots.Add(MakeSlot(
			Edemo_mapEntityLoadoutRegion::SpatialStorage,
			Index,
			FindRow(Snapshot, ItemId),
			ItemId.IsValid()));
	}

	TSet<FGuid> SelectedIds;
	if (Snapshot.SelectedWeaponId.IsValid())
	{
		SelectedIds.Add(Snapshot.SelectedWeaponId);
	}
	if (Snapshot.SelectedArmorId.IsValid())
	{
		SelectedIds.Add(Snapshot.SelectedArmorId);
	}
	if (Snapshot.SelectedAccessoryId.IsValid())
	{
		SelectedIds.Add(Snapshot.SelectedAccessoryId);
	}
	if (Snapshot.SelectedSpatialRingId.IsValid())
	{
		SelectedIds.Add(Snapshot.SelectedSpatialRingId);
	}
	if (Snapshot.SelectedBackpackId.IsValid())
	{
		SelectedIds.Add(Snapshot.SelectedBackpackId);
	}
	for (const FGuid& ItemId : Snapshot.OrderedSelectedMaterialIds)
	{
		if (ItemId.IsValid())
		{
			SelectedIds.Add(ItemId);
		}
	}

	Fdemo_mapPersistentProfile WarehouseProjectionProfile;
	WarehouseProjectionProfile.WarehouseLayout =
		Snapshot.WarehouseLayout;
	for (const Fdemo_mapProfilePreparationStashRow& Row :
		Snapshot.OrderedPermanentStashRows)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = Row.ItemInstanceId;
		Item.ItemDefinitionId = Row.ItemDefinitionId;
		Item.StackCount = Row.StackCount;
		WarehouseProjectionProfile.PermanentStash.Add(Item);
	}
	const TArray<FGuid> WarehouseSlotIds =
		Fdemo_mapWarehouseSlotProjection::Build(
			WarehouseProjectionProfile);
	for (int32 WarehouseIndex = 0;
		WarehouseIndex
			< Fdemo_mapEntityLoadoutRules::WarehouseDisplayCapacity;
		++WarehouseIndex)
	{
		const FGuid ItemId =
			WarehouseSlotIds.IsValidIndex(WarehouseIndex)
				? WarehouseSlotIds[WarehouseIndex]
				: FGuid();
		const Fdemo_mapProfilePreparationStashRow* Row =
			SelectedIds.Contains(ItemId)
				? nullptr
				: FindRow(Snapshot, ItemId);
		View.WarehouseSlots.Add(MakeSlot(
			Edemo_mapEntityLoadoutRegion::Warehouse,
			WarehouseIndex,
			Row,
			false));
	}

	if (!TotalCapacity.bSuccess)
	{
		View.Diagnostic = TotalCapacity.Diagnostic;
	}
	else if (!SpatialCapacity.bSuccess)
	{
		View.Diagnostic = SpatialCapacity.Diagnostic;
	}
	else if (!Fdemo_mapEntityLoadoutRules::CanFitSelectedItems(
		View.UsedCarriedSlots,
		SpatialItemDefinitionId,
		&View.Diagnostic,
		SpatialRingDefinitionId))
	{
		// Diagnostic already describes the capacity rejection.
	}
	else
	{
		View.bValid = true;
	}
	return View;
}

Fdemo_mapEntityLoadoutView
Fdemo_mapEntityLoadoutPresenter::BuildPlayerRuntimeView(
	const Fdemo_mapItemAuthority& Authority,
	int32 AccessorySlotCount)
{
	Fdemo_mapEntityLoadoutView View;
	View.AccessorySlotCount = FMath::Max(0, AccessorySlotCount);
	View.BaseQuickItemCapacity =
		Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount;
	const Fdemo_mapInventoryCapacityResult CapacityResult =
		Authority.GetInventoryCapacityResult();
	const int32 Capacity = CapacityResult.bSuccess
		? CapacityResult.Capacity : 0;
	View.TotalCarriedCapacity = Capacity;
	View.RingQuickItemCapacity = CapacityResult.bSuccess
		? CapacityResult.RingQuickCapacity : 0;
	View.SpatialStorageCapacity = CapacityResult.bSuccess
		? CapacityResult.SpatialBagCapacity : 0;
	View.UsedCarriedSlots = Authority.GetUsedInventorySlots();

	const auto AddEquipment =
		[&View, &Authority](
			Edemo_mapEntityLoadoutRegion Region,
			int32 SlotIndex,
			FName SlotId)
		{
			View.EquipmentSlots.Add(MakeRuntimeSlot(
				Region,
				SlotIndex,
				Authority,
				Authority.GetEquippedInstance(SlotId)));
		};
	AddEquipment(
		Edemo_mapEntityLoadoutRegion::Weapon,
		0,
		Fdemo_mapItemIds::WeaponSlot);
	AddEquipment(
		Edemo_mapEntityLoadoutRegion::Armor,
		0,
		Fdemo_mapItemIds::ArmorSlot);
	for (int32 Index = 0; Index < View.AccessorySlotCount; ++Index)
	{
		View.EquipmentSlots.Add(MakeRuntimeSlot(
			Edemo_mapEntityLoadoutRegion::Accessory,
			Index,
			Authority,
			Index == 0
				? Authority.GetEquippedInstance(
					Fdemo_mapItemIds::AccessorySlot)
				: FGuid()));
	}
	AddEquipment(
		Edemo_mapEntityLoadoutRegion::SpatialRing,
		0,
		Fdemo_mapItemIds::SpatialRingSlot);
	AddEquipment(
		Edemo_mapEntityLoadoutRegion::SpatialItem,
		0,
		Fdemo_mapItemIds::BackpackSlot);

	const TArray<FGuid>& Inventory =
		Authority.GetInventorySlotSnapshot();
	for (int32 Index = 0;
		Index < View.BaseQuickItemCapacity;
		++Index)
	{
		View.BaseQuickItemSlots.Add(MakeRuntimeSlot(
			Edemo_mapEntityLoadoutRegion::BaseQuickItem,
			Index,
			Authority,
			Inventory.IsValidIndex(Index)
				? Inventory[Index]
				: FGuid()));
	}
	for (int32 Index = 0;
		Index < View.RingQuickItemCapacity;
		++Index)
	{
		const int32 AuthorityIndex = View.BaseQuickItemCapacity + Index;
		View.RingQuickItemSlots.Add(MakeRuntimeSlot(
			Edemo_mapEntityLoadoutRegion::RingQuickItem,
			Index,
			Authority,
			Inventory.IsValidIndex(AuthorityIndex)
				? Inventory[AuthorityIndex]
				: FGuid()));
	}
	for (int32 Index = 0;
		Index < View.SpatialStorageCapacity;
		++Index)
	{
		const int32 AuthorityIndex =
			View.BaseQuickItemCapacity + View.RingQuickItemCapacity + Index;
		View.SpatialStorageSlots.Add(MakeRuntimeSlot(
			Edemo_mapEntityLoadoutRegion::SpatialStorage,
			Index,
			Authority,
			Inventory.IsValidIndex(AuthorityIndex)
				? Inventory[AuthorityIndex]
				: FGuid()));
	}

	FString Error;
	View.bValid =
		Capacity >= View.BaseQuickItemCapacity
		&& Inventory.Num() == Capacity
		&& Authority.ValidateInvariants(&Error);
	if (!View.bValid)
	{
		View.Diagnostic = Error.IsEmpty()
			? TEXT("Runtime entity view capacity does not match ItemAuthority.")
			: Error;
	}
	return View;
}
