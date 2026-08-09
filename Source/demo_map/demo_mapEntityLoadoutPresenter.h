#pragma once

#include "CoreMinimal.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapProfilePreparationTypes.h"

enum class Edemo_mapEntityLoadoutRegion : uint8
{
	Weapon,
	Armor,
	Accessory,
	SpatialRing,
	SpatialItem,
	BaseQuickItem,
	RingQuickItem,
	SpatialStorage,
	Warehouse
};

struct Fdemo_mapEntityItemSlotView
{
	Edemo_mapEntityLoadoutRegion Region = Edemo_mapEntityLoadoutRegion::Warehouse;
	int32 SlotIndex = INDEX_NONE;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FString LevelAndQualityLabel;
	int32 Quantity = 0;
	bool bOccupied = false;
	bool bSelected = false;
	bool bCanPlace = true;
};

/**
 * Rebuildable entity equipment/storage projection. It owns no item state and
 * can be reused for the player, corpses, or any later searchable entity.
 */
struct Fdemo_mapEntityLoadoutView
{
	TArray<Fdemo_mapEntityItemSlotView> EquipmentSlots;
	TArray<Fdemo_mapEntityItemSlotView> BaseQuickItemSlots;
	TArray<Fdemo_mapEntityItemSlotView> RingQuickItemSlots;
	TArray<Fdemo_mapEntityItemSlotView> SpatialStorageSlots;
	TArray<Fdemo_mapEntityItemSlotView> WarehouseSlots;
	int32 AccessorySlotCount = 0;
	int32 BaseQuickItemCapacity = 0;
	int32 RingQuickItemCapacity = 0;
	int32 SpatialStorageCapacity = 0;
	int32 TotalCarriedCapacity = 0;
	int32 UsedCarriedSlots = 0;
	bool bValid = false;
	FString Diagnostic;
};

struct Fdemo_mapEntityLoadoutRules
{
	static constexpr int32 BaseQuickItemSlotCount = 6;
	static constexpr int32 DefaultAccessorySlotCount = 1;
	static constexpr int32 MinimumSpatialStorageSlotCount = 6;
	static constexpr int32 MaximumRingQuickItemSlotCount = 12;
	static constexpr int32 MaximumSpatialStorageSlotCount = 36;
	static constexpr int32 WarehouseDisplayCapacity = 30;

	static bool CanFitSelectedItems(
		int32 SelectedItemCount,
		FName SpatialItemDefinitionId,
		FString* OutDiagnostic = nullptr,
		FName SpatialRingDefinitionId = NAME_None);
};

struct Fdemo_mapEntityLoadoutPresenter
{
	static Fdemo_mapEntityLoadoutView BuildPlayerPreparationView(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		int32 AccessorySlotCount =
			Fdemo_mapEntityLoadoutRules::DefaultAccessorySlotCount);
	/** Rebuilds the in-run player page directly from Runtime item authority. */
	static Fdemo_mapEntityLoadoutView BuildPlayerRuntimeView(
		const Fdemo_mapItemAuthority& Authority,
		int32 AccessorySlotCount =
			Fdemo_mapEntityLoadoutRules::DefaultAccessorySlotCount);
};
