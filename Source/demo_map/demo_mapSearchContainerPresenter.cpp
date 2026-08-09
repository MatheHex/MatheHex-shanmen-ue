#include "demo_mapSearchContainerPresenter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"

namespace
{
	const Fdemo_mapRuntimeContainerSectionSnapshot* FindSection(
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		Edemo_mapRuntimeContainerSection Section)
	{
		return Snapshot.Sections.FindByPredicate(
			[Section](
				const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
			{
				return Candidate.Section == Section;
			});
	}

	const Fdemo_mapRuntimeContainerEntrySnapshot* FindEntry(
		const Fdemo_mapRuntimeContainerSectionSnapshot* Section,
		int32 SlotIndex)
	{
		return Section
			? Section->OrderedOccupiedEntries.FindByPredicate(
				[SlotIndex](
					const Fdemo_mapRuntimeContainerEntrySnapshot& Candidate)
				{
					return Candidate.SlotIndex == SlotIndex;
				})
			: nullptr;
	}

	FString LoadoutRegionLabel(Edemo_mapEntityLoadoutRegion Region)
	{
		switch (Region)
		{
		case Edemo_mapEntityLoadoutRegion::Weapon:
			return TEXT("兵器");
		case Edemo_mapEntityLoadoutRegion::Armor:
			return TEXT("道袍");
		case Edemo_mapEntityLoadoutRegion::Accessory:
			return TEXT("饰品");
		case Edemo_mapEntityLoadoutRegion::SpatialRing:
			return TEXT("空间戒指");
		case Edemo_mapEntityLoadoutRegion::SpatialItem:
			return TEXT("空间道具");
		default:
			return TEXT("物品");
		}
	}

	FString TargetRegionLabel(Edemo_mapDualLootTargetRegion Region)
	{
		switch (Region)
		{
		case Edemo_mapDualLootTargetRegion::ContainerGrid:
			return TEXT("容器方格 / CONTAINER");
		case Edemo_mapDualLootTargetRegion::Weapon:
			return TEXT("兵器 / WEAPON");
		case Edemo_mapDualLootTargetRegion::Armor:
			return TEXT("道袍 / ROBE");
		case Edemo_mapDualLootTargetRegion::Accessory:
			return TEXT("饰品 / ACCESSORY");
		case Edemo_mapDualLootTargetRegion::SpatialRing:
			return TEXT("空间戒指 / SPATIAL RING");
		case Edemo_mapDualLootTargetRegion::SpatialItem:
			return TEXT("空间道具 / SPATIAL ITEM");
		case Edemo_mapDualLootTargetRegion::BaseQuickItems:
			return TEXT("基础快捷物品区 / BASE QUICK 6");
		case Edemo_mapDualLootTargetRegion::SpatialStorage:
			return TEXT("空间道具储物区 / SPATIAL STORAGE");
		case Edemo_mapDualLootTargetRegion::Body:
			return TEXT("身体容器 / BODY");
		default:
			return TEXT("LOOT");
		}
	}

	Fdemo_mapDualLootTargetCellView BuildTargetCell(
		const Fdemo_mapRuntimeContainerSectionSnapshot* Section,
		Edemo_mapRuntimeContainerSection SourceSection,
		int32 SourceSlotIndex,
		int32 VisualSlotIndex)
	{
		Fdemo_mapDualLootTargetCellView Result;
		Result.SourceSection = SourceSection;
		Result.SourceSlotIndex = SourceSlotIndex;
		Result.Cell.SlotIndex = VisualSlotIndex;
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
			FindEntry(Section, SourceSlotIndex);
		if (!Entry)
		{
			Result.Cell.DisplayName = TEXT("空");
			Result.Cell.IconLabel = TEXT("□");
			Result.Cell.bUnavailable = true;
			Result.StatusLabel = TEXT("EMPTY / 空");
			return Result;
		}

		Result.EntryId = Entry->EntryId;
		Result.State = Entry->State;
		Result.Progress01 = Entry->Progress01;
		Result.bCanSearch = Entry->bCanSearch;
		Result.bCanTake = Entry->bCanTake;
		Result.Cell.bOccupied =
			Entry->State != Edemo_mapRuntimeContainerEntryState::Taken;
		Result.Cell.bUnavailable =
			!Entry->bCanSearch && !Entry->bCanTake;
		switch (Entry->State)
		{
		case Edemo_mapRuntimeContainerEntryState::Hidden:
			Result.Cell.DisplayName = TEXT("未搜索");
			Result.Cell.IconLabel = TEXT("?");
			Result.StatusLabel = TEXT("UNSEARCHED / 未搜索");
			break;
		case Edemo_mapRuntimeContainerEntryState::Searching:
			Result.Cell.DisplayName = FString::Printf(
				TEXT("读取中 %.0f%%"),
				Entry->Progress01 * 100.0f);
			Result.Cell.IconLabel = TEXT("…");
			Result.StatusLabel = TEXT("SEARCHING / 搜索中");
			break;
		case Edemo_mapRuntimeContainerEntryState::Identified:
			Result.Cell.ItemInstanceId = Entry->ItemInstanceId;
			Result.Cell.ItemDefinitionId = Entry->DefinitionId;
			Result.Cell.DisplayName = Entry->DisplayName.ToString();
			Result.Cell.IconLabel = Entry->CategoryId.ToString();
			Result.Cell.LevelLabel = Entry->Level > 0
				? FString::Printf(TEXT("%d阶"), Entry->Level)
				: TEXT("基础");
			Result.Cell.Quantity = Entry->StackCount;
			Result.Cell.QualityLabel =
				Entry->RareRewardEventId.IsValid()
					? TEXT("极境")
					: Entry->RewardEventKind
						== Edemo_mapRewardEventKind::Jackpot
						? TEXT("JACKPOT")
						: TEXT("标准");
			Result.StatusLabel = TEXT("IDENTIFIED / 已识别");
			break;
		case Edemo_mapRuntimeContainerEntryState::Taken:
			Result.Cell.DisplayName = TEXT("已取走");
			Result.Cell.IconLabel = TEXT("□");
			Result.Cell.bOccupied = false;
			Result.StatusLabel = TEXT("TAKEN / 已取走");
			break;
		}
		return Result;
	}

	Fdemo_mapDualLootTargetRegionView BuildTargetRegion(
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		Edemo_mapDualLootTargetRegion Region,
		Edemo_mapRuntimeContainerSection SourceSection,
		int32 FirstSourceSlotIndex,
		int32 Capacity,
		bool bDirectlyIdentified)
	{
		Fdemo_mapDualLootTargetRegionView Result;
		Result.Region = Region;
		Result.Label = TargetRegionLabel(Region);
		Result.SourceSection = SourceSection;
		Result.FirstSourceSlotIndex = FirstSourceSlotIndex;
		Result.Capacity = FMath::Max(0, Capacity);
		Result.bDirectlyIdentified = bDirectlyIdentified;
		const Fdemo_mapRuntimeContainerSectionSnapshot* Section =
			FindSection(Snapshot, SourceSection);
		for (int32 VisualSlotIndex = 0;
			VisualSlotIndex < Result.Capacity;
			++VisualSlotIndex)
		{
			Result.Cells.Add(BuildTargetCell(
				Section,
				SourceSection,
				FirstSourceSlotIndex + VisualSlotIndex,
				VisualSlotIndex));
		}
		return Result;
	}
}

FString Fdemo_mapSearchContainerPresenter::SectionLabel(
	Edemo_mapRuntimeContainerSection Section)
{
	switch (Section)
	{
	case Edemo_mapRuntimeContainerSection::Chest:
		return TEXT("宝箱 / CHEST");
	case Edemo_mapRuntimeContainerSection::Equipment:
		return TEXT("装备 / EQUIPMENT");
	case Edemo_mapRuntimeContainerSection::Backpack:
		return TEXT("背包 / BACKPACK");
	case Edemo_mapRuntimeContainerSection::Body:
		return TEXT("躯体 / BODY");
	default:
		return TEXT("UNKNOWN");
	}
}

Fdemo_mapSearchContainerViewState Fdemo_mapSearchContainerPresenter::Build(
	const Fdemo_mapRuntimeContainerSnapshot& Snapshot)
{
	Fdemo_mapSearchContainerViewState View;
	View.Header = Snapshot.SourceDisplayLabel.Equals(
			TEXT("BOSS REWARD"),
			ESearchCase::CaseSensitive)
		? TEXT("BOSS REWARD")
		: Snapshot.Kind == Edemo_mapRuntimeContainerKind::Chest
			? TEXT("宝箱搜索 / CHEST SEARCH")
			: TEXT("尸体搜索 / CORPSE SEARCH");
	switch (Snapshot.State)
	{
	case Edemo_mapRuntimeContainerState::Closed:
		View.StateText = TEXT("CLOSED / 关闭");
		break;
	case Edemo_mapRuntimeContainerState::Opening:
		View.StateText = FString::Printf(
			TEXT("OPENING / 开启中 %.0f%%"),
			Snapshot.ActionProgress01 * 100.0f);
		break;
	case Edemo_mapRuntimeContainerState::Opened:
		View.StateText =
			Snapshot.ActiveAction
				== Edemo_mapRuntimeContainerActionKind::BeginSearch
			? FString::Printf(
				TEXT("SEARCHING / 读取中 %.0f%%"),
				Snapshot.ActionProgress01 * 100.0f)
			: Snapshot.bEmpty
				? TEXT("EMPTY / 已取空")
				: TEXT("OPENED / 已开启");
		break;
	}
	View.StateText += FString::Printf(TEXT("  REV %d"), Snapshot.Revision);
	View.InventoryText = FString::Printf(
		TEXT("Run Inventory %d / %d"),
		Snapshot.InventoryUsedSlots,
		Snapshot.InventoryCapacity);
	View.DiagnosticText = Snapshot.Diagnostic;

	for (const Fdemo_mapRuntimeContainerSectionSnapshot& SectionSnapshot :
		Snapshot.Sections)
	{
		Fdemo_mapSearchContainerSectionView SectionView;
		SectionView.Section = SectionSnapshot.Section;
		SectionView.Label = SectionLabel(SectionSnapshot.Section);
		for (int32 SlotIndex = 0; SlotIndex < SectionSnapshot.Capacity; ++SlotIndex)
		{
			const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
				SectionSnapshot.OrderedOccupiedEntries.FindByPredicate(
					[SlotIndex](const Fdemo_mapRuntimeContainerEntrySnapshot& Candidate)
					{
						return Candidate.SlotIndex == SlotIndex;
					});
			Fdemo_mapSearchContainerViewRow Row;
			Row.SlotIndex = SlotIndex;
			if (!Entry)
			{
				Row.State = Edemo_mapRuntimeContainerEntryState::Taken;
				Row.Text = FString::Printf(
					TEXT("%02d  — EMPTY / 空 —"),
					SlotIndex + 1);
			}
			else
			{
				Row.EntryId = Entry->EntryId;
				Row.ItemInstanceId = Entry->ItemInstanceId;
				Row.State = Entry->State;
				Row.bActionEnabled = Entry->bCanSearch || Entry->bCanTake;
				switch (Entry->State)
				{
				case Edemo_mapRuntimeContainerEntryState::Hidden:
					Row.Text = FString::Printf(
						TEXT("%02d  UNKNOWN / 未识别  [搜索]"),
						SlotIndex + 1);
					break;
				case Edemo_mapRuntimeContainerEntryState::Searching:
					Row.Text = FString::Printf(
						TEXT("%02d  SEARCHING / 搜索中 %.0f%%"),
						SlotIndex + 1,
						Entry->Progress01 * 100.0f);
					break;
				case Edemo_mapRuntimeContainerEntryState::Identified:
				{
					const bool bJackpot = Entry->RewardEventKind
						== Edemo_mapRewardEventKind::Jackpot;
					const bool bRare =
						Entry->RareRewardEventId.IsValid();
					TArray<FString> RewardParts;
					if (bJackpot)
					{
						RewardParts.Add(TEXT("JACKPOT ×6"));
					}
					if (bRare)
					{
						RewardParts.Add(TEXT("EXTREME VALUE"));
					}
					const FString AffixLabel =
						Fdemo_mapRewardAffixPolicyRegistry::
							BuildDisplayLabel(Entry->AffixSet);
					if (!AffixLabel.IsEmpty())
					{
						RewardParts.Add(AffixLabel);
					}
					const FString JoinedLabel =
						FString::Join(RewardParts, TEXT(" | "));
					const FString RewardLabel =
						JoinedLabel.IsEmpty()
							? FString()
							: TEXT("  ") + JoinedLabel;
					Row.Text = FString::Printf(
						TEXT("%02d  %s  Lv.%d  ×%d  [%s]%s  SELL=%lld  GUID=%s  [取出]"),
						SlotIndex + 1,
						*Entry->DisplayName.ToString(),
						Entry->Level,
						Entry->StackCount,
						*Entry->CategoryId.ToString(),
						*RewardLabel,
						Entry->EffectiveStackSellValue,
						*Entry->ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens));
					break;
				}
				case Edemo_mapRuntimeContainerEntryState::Taken:
					Row.Text = FString::Printf(
						TEXT("%02d  TAKEN / 已取走"),
						SlotIndex + 1);
					break;
				}
			}
			SectionView.Rows.Add(MoveTemp(Row));
		}
		View.Sections.Add(MoveTemp(SectionView));
	}
	return View;
}

Fdemo_mapSearchContainerViewState
Fdemo_mapSearchContainerPresenter::BuildRuntime(
	const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
	const Fdemo_mapItemAuthority& PlayerAuthority,
	const Fdemo_mapHotbarBindingSnapshot& HotbarBindings,
	const Fdemo_mapItemUseCooldownSnapshot& Cooldown)
{
	Fdemo_mapSearchContainerViewState View = Build(Snapshot);
	View.bDualSided = true;
	View.bOrdinaryContainer =
		Snapshot.Kind == Edemo_mapRuntimeContainerKind::Chest;
	View.bCorpse =
		Snapshot.Kind == Edemo_mapRuntimeContainerKind::Corpse;
	View.PlayerLoadout =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerRuntimeView(
			PlayerAuthority);
	View.Hotbar = Fdemo_mapItemPresentation::BuildRuntimeHotbar(
		HotbarBindings,
		PlayerAuthority,
		Cooldown);

	TArray<FString> EquipmentParts;
	for (const Fdemo_mapEntityItemSlotView& Slot :
		View.PlayerLoadout.EquipmentSlots)
	{
		EquipmentParts.Add(FString::Printf(
			TEXT("%s[%d] %s"),
			*LoadoutRegionLabel(Slot.Region),
			Slot.SlotIndex + 1,
			Slot.bOccupied ? *Slot.DisplayName : TEXT("空")));
	}
	TArray<FString> BaseQuickParts;
	for (const Fdemo_mapEntityItemSlotView& Slot :
		View.PlayerLoadout.BaseQuickItemSlots)
	{
		BaseQuickParts.Add(FString::Printf(
			TEXT("%d:%s"),
			Slot.SlotIndex + 1,
			Slot.bOccupied ? *Slot.DisplayName : TEXT("空")));
	}
	TArray<FString> SpatialParts;
	for (const Fdemo_mapEntityItemSlotView& Slot :
		View.PlayerLoadout.SpatialStorageSlots)
	{
		if (Slot.bOccupied)
		{
			SpatialParts.Add(FString::Printf(
				TEXT("%d:%s"),
				Slot.SlotIndex + 1,
				*Slot.DisplayName));
		}
	}
	View.PlayerLayoutSummary = FString::Printf(
		TEXT("玩家实体 / PLAYER\n%s\n基础快捷 %d 格: %s\n纳物戒快捷 %d 格\n吞天袋 %d 格: %s\n已用 %d / %d"),
		*FString::Join(EquipmentParts, TEXT("  |  ")),
		View.PlayerLoadout.BaseQuickItemCapacity,
		*FString::Join(BaseQuickParts, TEXT("  ")),
		View.PlayerLoadout.RingQuickItemCapacity,
		View.PlayerLoadout.SpatialStorageCapacity,
		SpatialParts.IsEmpty()
			? TEXT("空")
			: *FString::Join(SpatialParts, TEXT("  ")),
		View.PlayerLoadout.UsedCarriedSlots,
		View.PlayerLoadout.TotalCarriedCapacity);

	if (View.bOrdinaryContainer)
	{
		View.TargetRegions.Add(BuildTargetRegion(
			Snapshot,
			Edemo_mapDualLootTargetRegion::ContainerGrid,
			Edemo_mapRuntimeContainerSection::Chest,
			0,
			Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity,
			false));
		View.TargetLayoutSummary = FString::Printf(
			TEXT("%s / %s\n方格 %d | %s"),
			Snapshot.bPlayerDepositAllowed
				? TEXT("普通容器") : TEXT("资源容器（只可取）"),
			Snapshot.bPlayerDepositAllowed
				? TEXT("ORDINARY CONTAINER") : TEXT("TAKE-ONLY RESOURCE"),
			Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity,
			*View.StateText);
		return View;
	}

	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::Weapon,
		Edemo_mapRuntimeContainerSection::Equipment,
		0,
		1,
		true));
	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::Armor,
		Edemo_mapRuntimeContainerSection::Equipment,
		1,
		1,
		true));
	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::Accessory,
		Edemo_mapRuntimeContainerSection::Equipment,
		2,
		1,
		true));
	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::SpatialRing,
		Edemo_mapRuntimeContainerSection::Equipment,
		3,
		1,
		true));
	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::SpatialItem,
		Edemo_mapRuntimeContainerSection::Equipment,
		4,
		1,
		true));
	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::BaseQuickItems,
		Edemo_mapRuntimeContainerSection::Backpack,
		0,
		Fdemo_mapSearchContainerPrototypeConfig::
			CorpseBaseQuickItemCapacity,
		false));

	int32 SpatialStorageCapacity = 0;
	const Fdemo_mapRuntimeContainerSectionSnapshot* Equipment =
		FindSection(
			Snapshot,
			Edemo_mapRuntimeContainerSection::Equipment);
	const Fdemo_mapRuntimeContainerEntrySnapshot* SpatialItem =
		FindEntry(Equipment, 4);
	if (SpatialItem
		&& SpatialItem->State
			== Edemo_mapRuntimeContainerEntryState::Identified)
	{
		const Fdemo_mapSpatialStorageCapacityResult Capacity =
			Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
				SpatialItem->DefinitionId);
		SpatialStorageCapacity =
			Capacity.bSuccess ? Capacity.Capacity : 0;
	}
	if (SpatialStorageCapacity > 0)
	{
		View.TargetRegions.Add(BuildTargetRegion(
			Snapshot,
			Edemo_mapDualLootTargetRegion::SpatialStorage,
			Edemo_mapRuntimeContainerSection::Backpack,
			Fdemo_mapSearchContainerPrototypeConfig::
				CorpseBaseQuickItemCapacity,
			SpatialStorageCapacity,
			false));
	}
	View.TargetRegions.Add(BuildTargetRegion(
		Snapshot,
		Edemo_mapDualLootTargetRegion::Body,
		Edemo_mapRuntimeContainerSection::Body,
		0,
		Fdemo_mapSearchContainerPrototypeConfig::CorpseBodyCapacity,
		false));

	const auto OccupiedCount =
		[&View](Edemo_mapDualLootTargetRegion Region) -> int32
		{
			const Fdemo_mapDualLootTargetRegionView* Found =
				View.TargetRegions.FindByPredicate(
					[Region](
						const Fdemo_mapDualLootTargetRegionView& Candidate)
					{
						return Candidate.Region == Region;
					});
			int32 Count = 0;
			if (Found)
			{
				for (const Fdemo_mapDualLootTargetCellView& Cell :
					Found->Cells)
				{
					Count += Cell.Cell.bOccupied ? 1 : 0;
				}
			}
			return Count;
		};
	View.TargetLayoutSummary = FString::Printf(
		TEXT("敌人尸体 / ENEMY CORPSE\n兵器 %d/1 | 道袍 %d/1 | 饰品 %d/1 | 空间戒指 %d/1 | 空间道具 %d/1\n基础快捷 6 格 | 空间储物 %d 格 | 身体容器 %d 格"),
		OccupiedCount(Edemo_mapDualLootTargetRegion::Weapon),
		OccupiedCount(Edemo_mapDualLootTargetRegion::Armor),
		OccupiedCount(Edemo_mapDualLootTargetRegion::Accessory),
		OccupiedCount(Edemo_mapDualLootTargetRegion::SpatialRing),
		OccupiedCount(Edemo_mapDualLootTargetRegion::SpatialItem),
		SpatialStorageCapacity,
		Fdemo_mapSearchContainerPrototypeConfig::CorpseBodyCapacity);
	return View;
}
