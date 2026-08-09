#pragma once

#include "CoreMinimal.h"
#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapSearchContainerTypes.h"

struct Fdemo_mapSearchContainerViewRow
{
	FGuid EntryId;
	FGuid ItemInstanceId;
	int32 SlotIndex = INDEX_NONE;
	FString Text;
	bool bActionEnabled = false;
	Edemo_mapRuntimeContainerEntryState State =
		Edemo_mapRuntimeContainerEntryState::Hidden;
};

struct Fdemo_mapSearchContainerSectionView
{
	Edemo_mapRuntimeContainerSection Section =
		Edemo_mapRuntimeContainerSection::Chest;
	FString Label;
	TArray<Fdemo_mapSearchContainerViewRow> Rows;
};

enum class Edemo_mapDualLootTargetRegion : uint8
{
	ContainerGrid,
	Weapon,
	Armor,
	Accessory,
	SpatialRing,
	SpatialItem,
	BaseQuickItems,
	SpatialStorage,
	Body
};

struct Fdemo_mapDualLootTargetCellView
{
	FGuid EntryId;
	Edemo_mapRuntimeContainerSection SourceSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 SourceSlotIndex = INDEX_NONE;
	Edemo_mapRuntimeContainerEntryState State =
		Edemo_mapRuntimeContainerEntryState::Taken;
	Fdemo_mapUnifiedItemCellView Cell;
	FString StatusLabel;
	float Progress01 = 0.0f;
	bool bCanSearch = false;
	bool bCanTake = false;
};

struct Fdemo_mapDualLootTargetRegionView
{
	Edemo_mapDualLootTargetRegion Region =
		Edemo_mapDualLootTargetRegion::ContainerGrid;
	FString Label;
	Edemo_mapRuntimeContainerSection SourceSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 FirstSourceSlotIndex = 0;
	int32 Capacity = 0;
	bool bDirectlyIdentified = false;
	TArray<Fdemo_mapDualLootTargetCellView> Cells;
};

struct Fdemo_mapSearchContainerViewState
{
	FString Header;
	FString StateText;
	FString InventoryText;
	FString DiagnosticText;
	TArray<Fdemo_mapSearchContainerSectionView> Sections;
	bool bDualSided = false;
	bool bOrdinaryContainer = false;
	bool bCorpse = false;
	Fdemo_mapEntityLoadoutView PlayerLoadout;
	TArray<Fdemo_mapHotbarSlotView> Hotbar;
	TArray<Fdemo_mapDualLootTargetRegionView> TargetRegions;
	FString PlayerLayoutSummary;
	FString TargetLayoutSummary;
};

/** Pure snapshot-to-UI projection. It owns no Runtime or Item state. */
struct Fdemo_mapSearchContainerPresenter
{
	static Fdemo_mapSearchContainerViewState Build(
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot);
	static Fdemo_mapSearchContainerViewState BuildRuntime(
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		const Fdemo_mapItemAuthority& PlayerAuthority,
		const Fdemo_mapHotbarBindingSnapshot& HotbarBindings,
		const Fdemo_mapItemUseCooldownSnapshot& Cooldown);
	static FString SectionLabel(Edemo_mapRuntimeContainerSection Section);
};
