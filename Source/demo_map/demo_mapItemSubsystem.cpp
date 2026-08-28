#include "demo_mapItemSubsystem.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapEquipmentEffectResolver.h"
#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRuntimeContainer.h"
#include "demo_mapWorldItem.h"
#include "demo_mapInteractable.h"
#include "demo_mapSearchContainerActor.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"
#include "Misc/Crc.h"

namespace
{
	constexpr float WorldItemFootprintRadius = 42.0f;
	constexpr float ContainerFootprintRadius = 66.0f;
	constexpr float WorldObjectInteractionPadding = 28.0f;

	uint32 StablePlacementHash(FName StableSeed, const FVector& DesiredLocation)
	{
		const FString SeedText = StableSeed.IsNone()
			? FString::Printf(TEXT("%.0f:%.0f"), DesiredLocation.X, DesiredLocation.Y)
			: StableSeed.ToString();
		return FCrc::StrCrc32(*SeedText);
	}

	bool IsWorldPlacementClear(
		UWorld* World,
		const FVector& Candidate,
		const AActor* IgnoredActor,
		const TArray<FVector>* ReservedLocations,
		float CandidateFootprintRadius)
	{
		const float ItemMinimumDistance =
			CandidateFootprintRadius + WorldItemFootprintRadius +
			WorldObjectInteractionPadding;
		const float ContainerMinimumDistance =
			CandidateFootprintRadius + ContainerFootprintRadius +
			WorldObjectInteractionPadding;
		if (ReservedLocations)
		{
			for (const FVector& Reserved : *ReservedLocations)
			{
				if (FVector::DistSquared2D(Candidate, Reserved)
					< FMath::Square(ItemMinimumDistance))
				{
					return false;
				}
			}
		}
		for (TActorIterator<Ademo_mapWorldItem> It(World); It; ++It)
		{
			const Ademo_mapWorldItem* Existing = *It;
			if (!IsValid(Existing) || Existing == IgnoredActor)
			{
				continue;
			}
			if (FVector::DistSquared2D(Candidate, Existing->GetActorLocation())
				< FMath::Square(ItemMinimumDistance))
			{
				return false;
			}
		}
		for (TActorIterator<Ademo_mapSearchContainerActor> It(World); It; ++It)
		{
			const Ademo_mapSearchContainerActor* Existing = *It;
			if (!IsValid(Existing) || Existing == IgnoredActor)
			{
				continue;
			}
			if (FVector::DistSquared2D(Candidate, Existing->GetInteractionLocation())
				< FMath::Square(ContainerMinimumDistance))
			{
				return false;
			}
		}
		return true;
	}

	bool IsFrozenPreparedRunEquipmentSlot(FName SlotId)
	{
		return SlotId == Fdemo_mapItemIds::WeaponSlot
			|| SlotId == Fdemo_mapItemIds::ArmorSlot
			|| SlotId == Fdemo_mapItemIds::AccessorySlot
			|| SlotId == Fdemo_mapItemIds::SpatialRingSlot
			|| SlotId == Fdemo_mapItemIds::BackpackSlot;
	}
}

void Udemo_mapItemSubsystem::Deinitialize()
{
	TeardownWorld(ActiveWorld.Get());
	RemoveAllEquipmentSourcesFromBoundComponent();
	ClearHotbarBindings();
	ClearItemUseCooldown();
	BoundAttributeComponent.Reset();
	BoundHealthComponent.Reset();
	BoundPlayerPawn.Reset();
	Super::Deinitialize();
}

FName Udemo_mapItemSubsystem::MakeModifierSourceId(FGuid InstanceId)
{
	return InstanceId.IsValid() ? FName(*FString::Printf(TEXT("Equipment.%s"), *InstanceId.ToString(EGuidFormats::Digits))) : NAME_None;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::AddDefinition(FName DefinitionId, int32 Quantity, TArray<FGuid>* OutAffectedInstances)
{
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	TArray<FGuid> Affected;
	Fdemo_mapItemOperationResult Result = Authority.AddDefinition(DefinitionId, Quantity, &Affected);
	Result = TagAffectedForActiveRun(Before, Result, Affected);
	if (OutAffectedInstances) *OutAffectedInstances = Result.bSuccess ? Affected : TArray<FGuid>();
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::BindHotbarSlot(
	int32 ExternalSlotNumber,
	FGuid InstanceId)
{
	if (ExternalSlotNumber < 1 || ExternalSlotNumber > Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidSlot,
			TEXT("Hotbar slot number must be in the external 1..9 range."),
			InstanceId);
	}
	const Fdemo_mapItemInstance* Instance = Authority.FindInstance(InstanceId);
	if (!Instance)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InstanceNotFound,
			TEXT("Hotbar binding requires a registered ItemInstanceId."),
			InstanceId);
	}
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Instance->DefinitionId);
	if (!Definition)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnknownDefinition,
			TEXT("Hotbar binding encountered an unknown Definition."),
			InstanceId,
			Instance->DefinitionId);
	}
	if (!Fdemo_mapItemViewRules::IsHotbarBindable(*Instance, *Definition))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("Hotbar binding accepts only accessible, positive-quantity Consumable inventory instances."),
			InstanceId,
			Instance->DefinitionId);
	}
	const int32 InventoryIndex = Authority.FindInventorySlot(InstanceId);
	if (InventoryIndex < 0
		|| InventoryIndex
			>= Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
				+ Authority.GetRingQuickCapacity())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidSlot,
			TEXT("Runtime Hotbar may reference only Consumables in the base or equipped ring quick area."),
			InstanceId,
			Instance->DefinitionId);
	}

	if (HotbarBindings.SlotBindings.Num() != Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		HotbarBindings = Fdemo_mapHotbarBindingSnapshot();
	}
	for (FGuid& BoundId : HotbarBindings.SlotBindings)
	{
		if (BoundId == InstanceId)
		{
			BoundId.Invalidate();
		}
	}
	HotbarBindings.SlotBindings[ExternalSlotNumber - 1] = InstanceId;
	return Fdemo_mapItemOperationResult::Success(
		InstanceId,
		Instance->DefinitionId,
		FName(*FString::Printf(TEXT("Hotbar.%d"), ExternalSlotNumber)));
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::UnbindHotbarSlot(int32 ExternalSlotNumber)
{
	if (ExternalSlotNumber < 1 || ExternalSlotNumber > Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidSlot,
			TEXT("Hotbar slot number must be in the external 1..9 range."));
	}
	if (HotbarBindings.SlotBindings.Num() != Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		HotbarBindings = Fdemo_mapHotbarBindingSnapshot();
	}
	const FGuid Previous = HotbarBindings.SlotBindings[ExternalSlotNumber - 1];
	HotbarBindings.SlotBindings[ExternalSlotNumber - 1].Invalidate();
	return Fdemo_mapItemOperationResult::Success(
		Previous,
		NAME_None,
		FName(*FString::Printf(TEXT("Hotbar.%d"), ExternalSlotNumber)));
}

int32 Udemo_mapItemSubsystem::RefreshHotbarBindings()
{
	if (HotbarBindings.SlotBindings.Num() != Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		const int32 Cleared = HotbarBindings.SlotBindings.Num();
		HotbarBindings = Fdemo_mapHotbarBindingSnapshot();
		return Cleared;
	}

	int32 Cleared = 0;
	TSet<FGuid> Seen;
	for (FGuid& BoundId : HotbarBindings.SlotBindings)
	{
		if (!BoundId.IsValid())
		{
			continue;
		}
		const Fdemo_mapItemInstance* Instance = Authority.FindInstance(BoundId);
		const Fdemo_mapItemDefinition* Definition = Instance
			? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId)
			: nullptr;
		if (Seen.Contains(BoundId)
			|| !Instance
			|| !Definition
			|| !Fdemo_mapItemViewRules::IsHotbarBindable(*Instance, *Definition)
			|| Authority.FindInventorySlot(BoundId) < 0
			|| Authority.FindInventorySlot(BoundId)
				>= Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
					+ Authority.GetRingQuickCapacity())
		{
			BoundId.Invalidate();
			++Cleared;
			continue;
		}
		Seen.Add(BoundId);
	}
	return Cleared;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::MoveInventorySlot(
	int32 SourceSlotIndex,
	int32 TargetSlotIndex)
{
	const Fdemo_mapItemOperationResult Result =
		Authority.MoveInventorySlot(SourceSlotIndex, TargetSlotIndex);
	if (Result.bSuccess)
	{
		RefreshHotbarBindings();
	}
	return Result;
}

Fdemo_mapPlayerItemDropResult Udemo_mapItemSubsystem::ExecutePlayerItemDrop(
	const Fdemo_mapPlayerItemDropIntent& Intent)
{
	const Fdemo_mapPlayerItemDropResult Result =
		Authority.ExecutePlayerItemDrop(Intent);
	if (Result.IsSuccess())
	{
		// Leaving Base Quick Items (or consuming a merged source) may make a
		// hotbar reference illegal.  This pure reconciliation never changes the
		// authority transaction or creates an item copy.
		RefreshHotbarBindings();
	}
	return Result;
}

Fdemo_mapSearchContainerDropResult
Udemo_mapItemSubsystem::ExecuteSearchContainerDrop(
	const Fdemo_mapSearchContainerDropIntent& Intent,
	Fdemo_mapRuntimeContainerAuthority& Container)
{
	Fdemo_mapSearchContainerDropResult Result;
	Result.SourceItemInstanceId = Intent.ExpectedSourceItemInstanceId;
	Result.CommittedAuthorityRevision = Authority.GetAuthorityRevision();
	Result.CommittedContainerRevision = Container.GetRevision();
	auto Reject = [&Result, this, &Container](
		Edemo_mapItemResultCode Code,
		const FString& Message,
		FGuid RelatedId = FGuid())
	{
		Result.Kind = Edemo_mapPlayerItemDropKind::Reject;
		Result.bCommitted = false;
		Result.CommittedAuthorityRevision = Authority.GetAuthorityRevision();
		Result.CommittedContainerRevision = Container.GetRevision();
		Result.Operation = Fdemo_mapItemOperationResult::Failure(
			Code,
			Message,
			RelatedId.IsValid() ? RelatedId : Result.SourceItemInstanceId);
		Result.Diagnostic = Message;
		return Result;
	};

	if (RunState != Edemo_mapRunState::Active
		|| !ActiveRunId.IsValid()
		|| Intent.ExpectedRunId != ActiveRunId)
	{
		return Reject(Edemo_mapItemResultCode::RunNotActive,
			TEXT("搜索拖拽要求当前 Active Run。"));
	}
	if (Intent.ExpectedAuthorityRevision != Authority.GetAuthorityRevision()
		|| Intent.ExpectedContainerRevision != Container.GetRevision()
		|| Intent.ContainerId != Container.GetContainerId()
		|| !Container.IsInitialized()
		|| Container.GetState() != Edemo_mapRuntimeContainerState::Opened
		|| Container.IsActionActive())
	{
		return Reject(Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("拖拽已过期，或搜索目标已关闭／正在操作。"));
	}

	Fdemo_mapRuntimeContainerAuthorityState ContainerState =
		Container.CaptureState();
	const Fdemo_mapItemAuthorityState AuthorityBefore = Authority.CaptureState();
	const FName ContainerName = FName(*Intent.ContainerId.ToString(
		EGuidFormats::Digits));
	auto FindEntryBySlot = [&ContainerState](
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex) -> Fdemo_mapRuntimeContainerEntryRecord*
	{
		return ContainerState.Entries.FindByPredicate(
			[Section, SlotIndex](const Fdemo_mapRuntimeContainerEntryRecord& Entry)
			{
				return Entry.Section == Section && Entry.SlotIndex == SlotIndex;
			});
	};
	auto FindEntryById = [&ContainerState](FGuid EntryId)
		-> Fdemo_mapRuntimeContainerEntryRecord*
	{
		return ContainerState.Entries.FindByPredicate(
			[EntryId](const Fdemo_mapRuntimeContainerEntryRecord& Entry)
			{
				return Entry.EntryId == EntryId;
			});
	};
	auto ResolvePlayerSlot = [this](
		Edemo_mapPlayerItemArea Area,
		int32 VisualSlot,
		FName EquipmentSlot,
		int32& OutInventorySlot,
		FName& OutEquipmentSlot) -> bool
	{
		OutInventorySlot = INDEX_NONE;
		OutEquipmentSlot = NAME_None;
		if (Area == Edemo_mapPlayerItemArea::Equipment)
		{
			if (!Authority.IsKnownEquipmentSlot(EquipmentSlot))
			{
				return false;
			}
			OutEquipmentSlot = EquipmentSlot;
			return true;
		}
		return Authority.ResolveInventoryAreaSlot(
			Area,
			VisualSlot,
			OutInventorySlot);
	};
	auto IsCompatibleStack = [](const Fdemo_mapItemInstance& A,
		const Fdemo_mapItemInstance& B) -> bool
	{
		return A.AffixSet == B.AffixSet
			&& Fdemo_mapRewardEventRules::AreStackCompatible(
				A.DefinitionId, A.RewardEventKind, A.RewardEventId,
				A.RewardValueMultiplierBps, A.RewardSourceRoleId,
				A.RareRewardEventId, A.RareRewardPolicyId,
				A.RareRewardTierId, A.RareRewardBonusValue,
				B.DefinitionId, B.RewardEventKind, B.RewardEventId,
				B.RewardValueMultiplierBps, B.RewardSourceRoleId,
				B.RareRewardEventId, B.RareRewardPolicyId,
				B.RareRewardTierId, B.RareRewardBonusValue);
	};
	auto CanPlaceInContainer = [&ContainerState](
		const Fdemo_mapItemDefinition& Definition,
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex) -> bool
	{
		const int32 Capacity = Fdemo_mapSearchContainerPrototypeConfig::
			GetSectionCapacity(ContainerState.Kind, Section);
		if (SlotIndex < 0 || SlotIndex >= Capacity)
		{
			return false;
		}
		if (ContainerState.Kind == Edemo_mapRuntimeContainerKind::Chest)
		{
			return Section == Edemo_mapRuntimeContainerSection::Chest;
		}
		if (Section == Edemo_mapRuntimeContainerSection::Equipment)
		{
			const TArray<FName> SlotIds = {
				Fdemo_mapItemIds::WeaponSlot,
				Fdemo_mapItemIds::ArmorSlot,
				Fdemo_mapItemIds::AccessorySlot,
				Fdemo_mapItemIds::SpatialRingSlot,
				Fdemo_mapItemIds::BackpackSlot };
			return SlotIds.IsValidIndex(SlotIndex)
				&& Definition.CompatibleSlotIds.Contains(SlotIds[SlotIndex]);
		}
		// Corpse carried and body regions are storage regions, not equipment.
		return Section == Edemo_mapRuntimeContainerSection::Backpack
			|| Section == Edemo_mapRuntimeContainerSection::Body;
	};
	auto CanPlaceInPlayer = [this](
		const Fdemo_mapItemDefinition& Definition,
		Edemo_mapPlayerItemArea Area,
		FName EquipmentSlot) -> bool
	{
		return Area != Edemo_mapPlayerItemArea::Equipment
			|| (Authority.IsKnownEquipmentSlot(EquipmentSlot)
				&& Definition.CompatibleSlotIds.Contains(EquipmentSlot));
	};
	auto SetContainerOwnership = [&ContainerName, this](FGuid ItemId)
	{
		Fdemo_mapItemInstance& Item = Authority.Instances.FindChecked(ItemId);
		Item.OwnershipState = Edemo_mapItemOwnershipState::Container;
		Item.OwnerId = NAME_None;
		Item.ContainerId = ContainerName;
		Item.EquippedSlotId = NAME_None;
	};
	auto SetPlayerOwnership = [this](
		FGuid ItemId,
		Edemo_mapPlayerItemArea Area,
		int32 InventorySlot,
		FName EquipmentSlot)
	{
		Fdemo_mapItemInstance& Item = Authority.Instances.FindChecked(ItemId);
		if (Area == Edemo_mapPlayerItemArea::Equipment)
		{
			Authority.EquipmentSlots.FindChecked(EquipmentSlot) = ItemId;
			Item.OwnershipState = Edemo_mapItemOwnershipState::Equipped;
			Item.OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
			Item.ContainerId = Fdemo_mapItemIds::EquipmentContainer;
			Item.EquippedSlotId = EquipmentSlot;
		}
		else
		{
			Authority.InventorySlots[InventorySlot] = ItemId;
			Item.OwnershipState = Edemo_mapItemOwnershipState::Inventory;
			Item.OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
			Item.ContainerId = Fdemo_mapItemIds::InventoryContainer;
			Item.EquippedSlotId = NAME_None;
		}
	};
	auto ClearPlayerOwnership = [this](
		Edemo_mapPlayerItemArea Area,
		int32 InventorySlot,
		FName EquipmentSlot)
	{
		if (Area == Edemo_mapPlayerItemArea::Equipment)
		{
			Authority.EquipmentSlots.FindChecked(EquipmentSlot).Invalidate();
		}
		else
		{
			Authority.InventorySlots[InventorySlot].Invalidate();
		}
	};
	auto RefreshEntry = [this](
		Fdemo_mapRuntimeContainerEntryRecord& Entry,
		FGuid ItemId,
		Edemo_mapRuntimeContainerEntryState State)
	{
		const Fdemo_mapItemInstance& Item = Authority.Instances.FindChecked(ItemId);
		Entry.InternalItemInstanceId = ItemId;
		Entry.ExpectedDefinitionId = Item.DefinitionId;
		Entry.ExpectedStackCount = Item.Quantity;
		Entry.State = State;
	};
	auto AddEntry = [&ContainerState](
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex) -> Fdemo_mapRuntimeContainerEntryRecord&
	{
		Fdemo_mapRuntimeContainerEntryRecord NewEntry;
		do
		{
			NewEntry.EntryId = FGuid::NewGuid();
		}
		while (!NewEntry.EntryId.IsValid()
			|| ContainerState.Entries.ContainsByPredicate(
				[&NewEntry](const Fdemo_mapRuntimeContainerEntryRecord& Existing)
				{
					return Existing.EntryId == NewEntry.EntryId;
				}));
		NewEntry.Section = Section;
		NewEntry.SlotIndex = SlotIndex;
		NewEntry.SearchDurationSeconds = 0.0f;
		NewEntry.State = Edemo_mapRuntimeContainerEntryState::Taken;
		return ContainerState.Entries.Add_GetRef(NewEntry);
	};

	Fdemo_mapRuntimeContainerEntryRecord* SourceEntry = nullptr;
	Fdemo_mapRuntimeContainerEntryRecord* TargetEntry = nullptr;
	FGuid SourceId;
	FGuid PlayerTargetId;
	int32 PlayerSourceInventorySlot = INDEX_NONE;
	int32 PlayerTargetInventorySlot = INDEX_NONE;
	FName PlayerSourceEquipmentSlot = NAME_None;
	FName PlayerTargetEquipmentSlot = NAME_None;
	Edemo_mapPlayerItemDropKind DropKind = Edemo_mapPlayerItemDropKind::Move;
	FGuid RelatedEntryId;

	if (Intent.bSourceIsContainer)
	{
		SourceEntry = FindEntryById(Intent.SourceEntryId);
		if (!SourceEntry
			|| SourceEntry->State != Edemo_mapRuntimeContainerEntryState::Identified
			|| SourceEntry->InternalItemInstanceId
				!= Intent.ExpectedSourceItemInstanceId)
		{
			return Reject(Edemo_mapItemResultCode::UIInvalidSelection,
				TEXT("搜索源物品尚未识别、已被取走或已过期。"));
		}
		SourceId = SourceEntry->InternalItemInstanceId;
		if (!ResolvePlayerSlot(
			Intent.TargetPlayerArea,
			Intent.TargetPlayerSlotIndex,
			Intent.TargetPlayerEquipmentSlotId,
			PlayerTargetInventorySlot,
			PlayerTargetEquipmentSlot))
		{
			return Reject(Edemo_mapItemResultCode::InvalidSlot,
				TEXT("玩家目标格无效。"), SourceId);
		}
		PlayerTargetId = Intent.TargetPlayerArea
			== Edemo_mapPlayerItemArea::Equipment
			? Authority.EquipmentSlots.FindRef(PlayerTargetEquipmentSlot)
			: Authority.InventorySlots[PlayerTargetInventorySlot];
	}
	else
	{
		if (!Container.AllowsPlayerDeposit())
		{
			return Reject(Edemo_mapItemResultCode::InvalidOwnership,
				TEXT("该资源搜索目标仅允许取出物品。"));
		}
		if (!ResolvePlayerSlot(
			Intent.SourcePlayerArea,
			Intent.SourcePlayerSlotIndex,
			Intent.SourcePlayerEquipmentSlotId,
			PlayerSourceInventorySlot,
			PlayerSourceEquipmentSlot))
		{
			return Reject(Edemo_mapItemResultCode::InvalidSlot,
				TEXT("玩家拖拽源格无效。"));
		}
		SourceId = Intent.SourcePlayerArea
			== Edemo_mapPlayerItemArea::Equipment
			? Authority.EquipmentSlots.FindRef(PlayerSourceEquipmentSlot)
			: Authority.InventorySlots[PlayerSourceInventorySlot];
		if (!SourceId.IsValid() || SourceId != Intent.ExpectedSourceItemInstanceId)
		{
			return Reject(Edemo_mapItemResultCode::UIInvalidSelection,
				TEXT("玩家拖拽物品已移动、为空或已过期。"), SourceId);
		}
		TargetEntry = Intent.TargetEntryId.IsValid()
			? FindEntryById(Intent.TargetEntryId)
			: FindEntryBySlot(Intent.TargetSection, Intent.TargetContainerSlotIndex);
		if (TargetEntry
			&& (TargetEntry->Section != Intent.TargetSection
				|| TargetEntry->SlotIndex != Intent.TargetContainerSlotIndex))
		{
			return Reject(Edemo_mapItemResultCode::UIInvalidSelection,
				TEXT("搜索目标格与拖拽快照不匹配。"), SourceId);
		}
	}

	Fdemo_mapItemInstance* SourceItem = Authority.Instances.Find(SourceId);
	const Fdemo_mapItemDefinition* SourceDefinition = SourceItem
		? Fdemo_mapItemDefinitions::Find(SourceItem->DefinitionId)
		: nullptr;
	if (!SourceItem || !SourceDefinition || SourceItem->Quantity <= 0)
	{
		return Reject(Edemo_mapItemResultCode::InvariantViolation,
			TEXT("拖拽源的 ItemInstance 或定义无效。"), SourceId);
	}
	if (Intent.bSourceIsContainer)
	{
		if (SourceItem->OwnershipState != Edemo_mapItemOwnershipState::Container
			|| SourceItem->ContainerId != ContainerName)
		{
			return Reject(Edemo_mapItemResultCode::InvalidOwnership,
				TEXT("搜索源不再由当前 Container 持有。"), SourceId);
		}
		if (!CanPlaceInPlayer(
			*SourceDefinition,
			Intent.TargetPlayerArea,
			PlayerTargetEquipmentSlot))
		{
			return Reject(Edemo_mapItemResultCode::IncompatibleSlot,
				TEXT("搜索物品不兼容玩家目标装备槽。"), SourceId);
		}
	}
	else
	{
		const bool bSourceOwned = Intent.SourcePlayerArea
			== Edemo_mapPlayerItemArea::Equipment
			? SourceItem->OwnershipState == Edemo_mapItemOwnershipState::Equipped
				&& SourceItem->EquippedSlotId == PlayerSourceEquipmentSlot
			: SourceItem->OwnershipState == Edemo_mapItemOwnershipState::Inventory;
		if (!bSourceOwned)
		{
			return Reject(Edemo_mapItemResultCode::InvalidOwnership,
				TEXT("玩家源物品不再属于拖拽的区域。"), SourceId);
		}
		if (Intent.SourcePlayerArea == Edemo_mapPlayerItemArea::Equipment
			&& PlayerSourceEquipmentSlot == Fdemo_mapItemIds::BackpackSlot
			&& Authority.InventorySlots.Num()
				> Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
					+ Authority.GetRingQuickCapacity()
			&& Authority.GetUsedInventorySlots()
				> Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
					+ Authority.GetRingQuickCapacity())
		{
			return Reject(Edemo_mapItemResultCode::InventoryFull,
				TEXT("含内部物品的空间道具不能直接放回搜索目标。"), SourceId);
		}
		if (!CanPlaceInContainer(
			*SourceDefinition,
			Intent.TargetSection,
			Intent.TargetContainerSlotIndex))
		{
			return Reject(Edemo_mapItemResultCode::IncompatibleSlot,
				TEXT("该搜索目标区域不接收此物品类型。"), SourceId);
		}
	}

	if (Intent.bSourceIsContainer)
	{
		if (!PlayerTargetId.IsValid())
		{
			if (Intent.TargetPlayerArea == Edemo_mapPlayerItemArea::Equipment
				&& PlayerTargetEquipmentSlot == Fdemo_mapItemIds::BackpackSlot)
			{
				const Fdemo_mapInventoryCapacityResult Capacity =
					Authority.PreflightInventoryCapacity(
						Authority.GetUsedInventorySlots(),
						SourceItem->DefinitionId);
				if (!Capacity.bSuccess || !Capacity.bFits)
				{
					return Reject(Edemo_mapItemResultCode::InventoryFull,
						Capacity.Diagnostic, SourceId);
				}
				Authority.InventorySlots.SetNum(Capacity.Capacity);
			}
			SetPlayerOwnership(
				SourceId,
				Intent.TargetPlayerArea,
				PlayerTargetInventorySlot,
				PlayerTargetEquipmentSlot);
			SourceEntry->State = Edemo_mapRuntimeContainerEntryState::Taken;
			DropKind = Intent.TargetPlayerArea == Edemo_mapPlayerItemArea::Equipment
				? Edemo_mapPlayerItemDropKind::Equip
				: Edemo_mapPlayerItemDropKind::Move;
			RelatedEntryId = SourceEntry->EntryId;
		}
		else
		{
			Fdemo_mapItemInstance* TargetItem = Authority.Instances.Find(PlayerTargetId);
			const Fdemo_mapItemDefinition* TargetDefinition = TargetItem
				? Fdemo_mapItemDefinitions::Find(TargetItem->DefinitionId) : nullptr;
			if (!TargetItem || !TargetDefinition)
			{
				return Reject(Edemo_mapItemResultCode::InvariantViolation,
					TEXT("玩家目标 ItemInstance 无效。"), PlayerTargetId);
			}
			if (Intent.TargetPlayerArea != Edemo_mapPlayerItemArea::Equipment
				&& IsCompatibleStack(*SourceItem, *TargetItem)
				&& TargetItem->Quantity < TargetDefinition->MaxStackSize)
			{
				const int32 Added = FMath::Min(
					SourceItem->Quantity,
					TargetDefinition->MaxStackSize - TargetItem->Quantity);
				int64 AddedBonus = 0;
				int64 RemainingBonus = 0;
				if (!Fdemo_mapRewardEventRules::TrySplitRareBonus(
					SourceItem->Quantity,
					Added,
					SourceItem->RareRewardBonusValue,
					AddedBonus,
					RemainingBonus))
				{
					return Reject(Edemo_mapItemResultCode::InvariantViolation,
						TEXT("搜索堆叠奖励数值分配失败。"), SourceId);
				}
				SourceItem->Quantity -= Added;
				SourceItem->RareRewardBonusValue = RemainingBonus;
				TargetItem->Quantity += Added;
				TargetItem->RareRewardBonusValue += AddedBonus;
				if (SourceItem->Quantity == 0)
				{
					SourceItem->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
					SourceItem->ContainerId = NAME_None;
					SourceItem->OwnerId = NAME_None;
					SourceEntry->State = Edemo_mapRuntimeContainerEntryState::Taken;
				}
				else
				{
					RefreshEntry(*SourceEntry, SourceId,
						Edemo_mapRuntimeContainerEntryState::Identified);
				}
				Result.TargetItemInstanceId = PlayerTargetId;
				DropKind = Edemo_mapPlayerItemDropKind::Merge;
				RelatedEntryId = SourceEntry->EntryId;
			}
			else
			{
				if (!CanPlaceInContainer(
					*TargetDefinition,
					SourceEntry->Section,
					SourceEntry->SlotIndex))
				{
					return Reject(Edemo_mapItemResultCode::IncompatibleSlot,
						TEXT("玩家目标物品不能放回该搜索源格。"), PlayerTargetId);
				}
				if (Intent.TargetPlayerArea == Edemo_mapPlayerItemArea::Equipment
					&& PlayerTargetEquipmentSlot == Fdemo_mapItemIds::BackpackSlot)
				{
					const Fdemo_mapInventoryCapacityResult Capacity =
						Authority.PreflightInventoryCapacity(
							Authority.GetUsedInventorySlots(),
							SourceItem->DefinitionId);
					if (!Capacity.bSuccess || !Capacity.bFits)
					{
						return Reject(Edemo_mapItemResultCode::InventoryFull,
							Capacity.Diagnostic, SourceId);
					}
					Authority.InventorySlots.SetNum(Capacity.Capacity);
				}
				ClearPlayerOwnership(
					Intent.TargetPlayerArea,
					PlayerTargetInventorySlot,
					PlayerTargetEquipmentSlot);
				SetPlayerOwnership(
					SourceId,
					Intent.TargetPlayerArea,
					PlayerTargetInventorySlot,
					PlayerTargetEquipmentSlot);
				SetContainerOwnership(PlayerTargetId);
				RefreshEntry(*SourceEntry, PlayerTargetId,
					Edemo_mapRuntimeContainerEntryState::Identified);
				Result.TargetItemInstanceId = PlayerTargetId;
				DropKind = Edemo_mapPlayerItemDropKind::Swap;
				RelatedEntryId = SourceEntry->EntryId;
			}
		}
	}
	else
	{
		const bool bTargetOccupied = TargetEntry
			&& TargetEntry->State == Edemo_mapRuntimeContainerEntryState::Identified;
		if (TargetEntry && TargetEntry->State != Edemo_mapRuntimeContainerEntryState::Taken
			&& !bTargetOccupied)
		{
			return Reject(Edemo_mapItemResultCode::UIInvalidSelection,
				TEXT("搜索目标格未识别或仍在读取中。"), SourceId);
		}
		if (!TargetEntry)
		{
			TargetEntry = &AddEntry(Intent.TargetSection,
				Intent.TargetContainerSlotIndex);
		}
		if (!bTargetOccupied)
		{
			ClearPlayerOwnership(Intent.SourcePlayerArea,
				PlayerSourceInventorySlot, PlayerSourceEquipmentSlot);
			if (Intent.SourcePlayerArea == Edemo_mapPlayerItemArea::Equipment
				&& PlayerSourceEquipmentSlot == Fdemo_mapItemIds::BackpackSlot)
			{
				Authority.InventorySlots.SetNum(
					Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
						+ Authority.GetRingQuickCapacity());
			}
			SetContainerOwnership(SourceId);
			RefreshEntry(*TargetEntry, SourceId,
				Edemo_mapRuntimeContainerEntryState::Identified);
			DropKind = Intent.SourcePlayerArea == Edemo_mapPlayerItemArea::Equipment
				? Edemo_mapPlayerItemDropKind::Unequip
				: Edemo_mapPlayerItemDropKind::Move;
			RelatedEntryId = TargetEntry->EntryId;
		}
		else
		{
			Fdemo_mapItemInstance* TargetItem = Authority.Instances.Find(
				TargetEntry->InternalItemInstanceId);
			const Fdemo_mapItemDefinition* TargetDefinition = TargetItem
				? Fdemo_mapItemDefinitions::Find(TargetItem->DefinitionId) : nullptr;
			if (!TargetItem || !TargetDefinition
				|| TargetItem->OwnershipState != Edemo_mapItemOwnershipState::Container
				|| TargetItem->ContainerId != ContainerName)
			{
				return Reject(Edemo_mapItemResultCode::InvalidOwnership,
					TEXT("搜索目标物品已失效。"), TargetEntry->InternalItemInstanceId);
			}
			if (Intent.SourcePlayerArea != Edemo_mapPlayerItemArea::Equipment
				&& IsCompatibleStack(*SourceItem, *TargetItem)
				&& TargetItem->Quantity < TargetDefinition->MaxStackSize)
			{
				const int32 Added = FMath::Min(SourceItem->Quantity,
					TargetDefinition->MaxStackSize - TargetItem->Quantity);
				int64 AddedBonus = 0;
				int64 RemainingBonus = 0;
				if (!Fdemo_mapRewardEventRules::TrySplitRareBonus(
					SourceItem->Quantity, Added, SourceItem->RareRewardBonusValue,
					AddedBonus, RemainingBonus))
				{
					return Reject(Edemo_mapItemResultCode::InvariantViolation,
						TEXT("放回堆叠奖励数值分配失败。"), SourceId);
				}
				SourceItem->Quantity -= Added;
				SourceItem->RareRewardBonusValue = RemainingBonus;
				TargetItem->Quantity += Added;
				TargetItem->RareRewardBonusValue += AddedBonus;
				if (SourceItem->Quantity == 0)
				{
					ClearPlayerOwnership(Intent.SourcePlayerArea,
						PlayerSourceInventorySlot, PlayerSourceEquipmentSlot);
					SourceItem->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
					SourceItem->OwnerId = NAME_None;
					SourceItem->ContainerId = NAME_None;
					SourceItem->EquippedSlotId = NAME_None;
				}
				RefreshEntry(*TargetEntry, TargetItem->InstanceId,
					Edemo_mapRuntimeContainerEntryState::Identified);
				Result.TargetItemInstanceId = TargetItem->InstanceId;
				DropKind = Edemo_mapPlayerItemDropKind::Merge;
				RelatedEntryId = TargetEntry->EntryId;
			}
			else
			{
				if (!CanPlaceInPlayer(*TargetDefinition,
					Intent.SourcePlayerArea, PlayerSourceEquipmentSlot))
				{
					return Reject(Edemo_mapItemResultCode::IncompatibleSlot,
						TEXT("搜索目标物品不能交换到玩家源格。"), TargetItem->InstanceId);
				}
				if (Intent.SourcePlayerArea == Edemo_mapPlayerItemArea::Equipment
					&& PlayerSourceEquipmentSlot == Fdemo_mapItemIds::BackpackSlot)
				{
					const Fdemo_mapInventoryCapacityResult Capacity =
						Authority.PreflightInventoryCapacity(
							Authority.GetUsedInventorySlots(), TargetItem->DefinitionId);
					if (!Capacity.bSuccess || !Capacity.bFits)
					{
						return Reject(Edemo_mapItemResultCode::InventoryFull,
							Capacity.Diagnostic, TargetItem->InstanceId);
					}
					Authority.InventorySlots.SetNum(Capacity.Capacity);
				}
				ClearPlayerOwnership(Intent.SourcePlayerArea,
					PlayerSourceInventorySlot, PlayerSourceEquipmentSlot);
				SetPlayerOwnership(TargetItem->InstanceId,
					Intent.SourcePlayerArea, PlayerSourceInventorySlot,
					PlayerSourceEquipmentSlot);
				SetContainerOwnership(SourceId);
				RefreshEntry(*TargetEntry, SourceId,
					Edemo_mapRuntimeContainerEntryState::Identified);
				Result.TargetItemInstanceId = TargetItem->InstanceId;
				DropKind = Edemo_mapPlayerItemDropKind::Swap;
				RelatedEntryId = TargetEntry->EntryId;
			}
		}
	}

	const Fdemo_mapItemInstance* RelatedItem = Authority.FindInstance(SourceId);
	const Fdemo_mapItemOperationResult Commit = Authority.CommitOrRollback(
		AuthorityBefore,
		SourceId,
		RelatedItem ? RelatedItem->DefinitionId : SourceDefinition->DefinitionId,
		NAME_None);
	if (!Commit.bSuccess)
	{
		return Reject(Commit.Code, Commit.Diagnostic, SourceId);
	}
	if (!SynchronizeEquipmentModifiers())
	{
		Authority.RestoreState(AuthorityBefore);
		SynchronizeEquipmentModifiers();
		return Reject(Edemo_mapItemResultCode::ModifierApplicationFailed,
			TEXT("装备效果同步失败，搜索拖拽已回滚。"), SourceId);
	}
	const Fdemo_mapRuntimeContainerResult ContainerCommit =
		Container.CommitPlayerDropState(
			ContainerState,
			Intent.ExpectedContainerRevision,
			RelatedEntryId,
			SourceId);
	if (!ContainerCommit.bSuccess)
	{
		Authority.RestoreState(AuthorityBefore);
		SynchronizeEquipmentModifiers();
		return Reject(Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("搜索目标版本已变化，拖拽已回滚。"), SourceId);
	}
	RefreshHotbarBindings();
	Result.Kind = DropKind;
	Result.bCommitted = true;
	Result.CommittedAuthorityRevision = Authority.GetAuthorityRevision();
	Result.CommittedContainerRevision = ContainerCommit.RevisionAfter;
	Result.Operation = Commit;
	Result.Diagnostic = TEXT("搜索目标与玩家物品权威已原子同步。");
	return Result;
}

void Udemo_mapItemSubsystem::ClearHotbarBindings()
{
	HotbarBindings = Fdemo_mapHotbarBindingSnapshot();
}

double Udemo_mapItemSubsystem::GetItemUseTimeSeconds() const
{
	const double BaseTime = ActiveWorld.IsValid()
		? static_cast<double>(ActiveWorld->GetTimeSeconds())
		: FPlatformTime::Seconds();
#if WITH_DEV_AUTOMATION_TESTS
	return BaseTime + ItemUseAutomationTimeOffset;
#else
	return BaseTime;
#endif
}

Fdemo_mapItemUseCooldownSnapshot
Udemo_mapItemSubsystem::GetItemUseCooldownSnapshot() const
{
	Fdemo_mapItemUseCooldownSnapshot Snapshot;
	Snapshot.RemainingSeconds = static_cast<float>(
		FMath::Max(0.0, HealingPillCooldownEndTime - GetItemUseTimeSeconds()));
	Snapshot.bActive = Snapshot.RemainingSeconds > 0.0f;
	return Snapshot;
}

void Udemo_mapItemSubsystem::ClearItemUseCooldown()
{
	HealingPillCooldownEndTime = 0.0;
}

Fdemo_mapItemUseResult Udemo_mapItemSubsystem::UseHotbarSlot(
	const Fdemo_mapItemUseIntent& Intent,
	bool bInputAllowed)
{
	Fdemo_mapItemUseResult Result;
	Result.HotbarSlotNumber = Intent.HotbarSlotNumber;
	Result.ItemInstanceId = Intent.ExpectedItemInstanceId;
	const Fdemo_mapItemUseCooldownSnapshot CooldownBefore =
		GetItemUseCooldownSnapshot();
	Result.CooldownBefore = CooldownBefore.RemainingSeconds;
	auto Reject = [&Result](
		Edemo_mapItemUseStatus Status,
		const TCHAR* Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.CooldownAfter = Result.CooldownBefore;
		return Result;
	};

	if (RunState != Edemo_mapRunState::Active
		|| !ActiveRunId.IsValid()
		|| Intent.ExpectedRunId != ActiveRunId)
	{
		return Reject(
			Edemo_mapItemUseStatus::NotInActiveRun,
			TEXT("Item use requires the exact current ActiveRun identity."));
	}
	Udemo_mapPlayerHealthComponent* Health = BoundHealthComponent.Get();
	if (Health == nullptr)
	{
		return Reject(
			Edemo_mapItemUseStatus::PlayerUnavailable,
			TEXT("Item use requires the bound PlayerHealth authority."));
	}
	if (Health->IsDefeated())
	{
		return Reject(
			Edemo_mapItemUseStatus::PlayerDefeatedOrTerminal,
			TEXT("Defeated or terminal players cannot use items."));
	}
	if (!bInputAllowed)
	{
		return Reject(
			Edemo_mapItemUseStatus::InputLocked,
			TEXT("Current gameplay or UI input lock rejects item use."));
	}
	if (Intent.HotbarSlotNumber < 1
		|| Intent.HotbarSlotNumber > Fdemo_mapHotbarBindingSnapshot::SlotCount
		|| HotbarBindings.SlotBindings.Num()
			!= Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		return Reject(
			Edemo_mapItemUseStatus::InvalidHotbarSlot,
			TEXT("Hotbar item use requires an external slot in the 1..9 range."));
	}
	const FGuid BoundId =
		HotbarBindings.SlotBindings[Intent.HotbarSlotNumber - 1];
	if (!BoundId.IsValid())
	{
		return Reject(
			Edemo_mapItemUseStatus::EmptyBinding,
			TEXT("Hotbar slot is empty."));
	}
	if (BoundId != Intent.ExpectedItemInstanceId)
	{
		return Reject(
			Edemo_mapItemUseStatus::ExpectedInstanceMismatch,
			TEXT("Hotbar binding no longer matches the expected ItemInstanceId."));
	}
	const Fdemo_mapItemInstance* Instance = Authority.FindInstance(BoundId);
	if (!Instance || Instance->OwnershipState == Edemo_mapItemOwnershipState::Destroyed)
	{
		return Reject(
			Edemo_mapItemUseStatus::StaleBinding,
			TEXT("Hotbar binding references a missing or destroyed ItemInstance."));
	}
	Result.DefinitionId = Instance->DefinitionId;
	Result.BeforeStack = Instance->Quantity;
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory
		|| Instance->OwnerId != Fdemo_mapItemIds::LocalPlayerOwner
		|| Instance->ContainerId != Fdemo_mapItemIds::InventoryContainer
		|| Authority.FindInventorySlot(BoundId) == INDEX_NONE
		|| Instance->OriginRunId != ActiveRunId)
	{
		return Reject(
			Edemo_mapItemUseStatus::WrongOwnership,
			TEXT("Item use requires current-run local Inventory ownership."));
	}
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Instance->DefinitionId);
	if (!Definition)
	{
		return Reject(
			Edemo_mapItemUseStatus::UnknownDefinition,
			TEXT("Item use encountered an unknown Definition."));
	}
	if (Definition->CategoryId != Fdemo_mapItemIds::ConsumableCategory)
	{
		return Reject(
			Edemo_mapItemUseStatus::NotConsumable,
			TEXT("Only Consumable definitions may be used from the Hotbar."));
	}
	if (Instance->Quantity <= 0
		|| Instance->Quantity > Definition->MaxStackSize)
	{
		return Reject(
			Edemo_mapItemUseStatus::InvalidStack,
			TEXT("Item use encountered an invalid StackCount."));
	}

	int32 HealAmount = 0;
	int32 HealEffectCount = 0;
	for (const Fdemo_mapItemEffectParameter& Effect
		: Definition->EffectParameters)
	{
		if (Effect.ParameterId != Fdemo_mapItemEffectIds::HealAmount)
		{
			return Reject(
				Edemo_mapItemUseStatus::NotConsumable,
				TEXT("Consumable contains an unsupported Effect Key."));
		}
		++HealEffectCount;
		if (!FMath::IsFinite(Effect.Value)
			|| Effect.Value <= 0.0
			|| Effect.Value > static_cast<double>(MAX_int32)
			|| FMath::TruncToDouble(Effect.Value) != Effect.Value)
		{
			return Reject(
				Edemo_mapItemUseStatus::InvalidHealAmount,
				TEXT("HealAmount must be one finite positive integer."));
		}
		HealAmount = static_cast<int32>(Effect.Value);
	}
	if (HealEffectCount != 1)
	{
		return Reject(
			Edemo_mapItemUseStatus::InvalidHealAmount,
			TEXT("Healing Pill requires exactly one HealAmount Effect."));
	}

	Result.HealRequested = HealAmount;
	const float VitalityBefore = Health->GetCurrentVitality();
	Result.BeforeHealth = Health->GetCurrentHealth();
	Result.MaxHealth = Health->GetMaxHealth();
	if (!Fdemo_mapItemUsePrototypeConfig::AllowHealingPillAtFullHealth
		&& VitalityBefore >= Health->GetMaximumVitality())
	{
		return Reject(
			Edemo_mapItemUseStatus::FullHealth,
			TEXT("Prototype policy rejects Healing Pill use at full health."));
	}
	if (CooldownBefore.bActive)
	{
		return Reject(
			Edemo_mapItemUseStatus::CooldownActive,
			TEXT("The shared Healing Pill cooldown is active."));
	}

	const Fdemo_mapItemAuthorityState AuthorityBefore =
		Authority.CaptureState();
	const Fdemo_mapHotbarBindingSnapshot HotbarBefore = HotbarBindings;
	const double CooldownEndBefore = HealingPillCooldownEndTime;
	auto Rollback = [&]()
	{
		Authority.RestoreState(AuthorityBefore);
		HotbarBindings = HotbarBefore;
		Health->RestoreCurrentVitalityAfterItemUseRollback(
			VitalityBefore);
		HealingPillCooldownEndTime = CooldownEndBefore;
		Result.AfterHealth = Result.BeforeHealth;
		Result.AfterStack = Result.BeforeStack;
		Result.CooldownAfter = Result.CooldownBefore;
		Result.bBindingCleared = false;
	};

#if WITH_DEV_AUTOMATION_TESTS
	const bool bFailItem =
		Intent.FailurePoint
			== Edemo_mapItemUseFailurePoint::AfterItemMutation;
	const Fdemo_mapItemOperationResult ConsumeResult =
		Authority.ConsumeInventoryUnit(BoundId, bFailItem);
#else
	const Fdemo_mapItemOperationResult ConsumeResult =
		Authority.ConsumeInventoryUnit(BoundId);
#endif
	if (!ConsumeResult.bSuccess)
	{
		Rollback();
		return Reject(
			Edemo_mapItemUseStatus::CommitFailed,
			TEXT("Item authority rejected or rolled back Stack consumption."));
	}

	Result.HealApplied = Health->ApplyHealing(HealAmount);
	if (Result.HealApplied <= 0)
	{
		Rollback();
		return Reject(
			Edemo_mapItemUseStatus::CommitFailed,
			TEXT("Health authority rejected healing during transaction commit."));
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (Intent.FailurePoint
		== Edemo_mapItemUseFailurePoint::AfterHealthMutation)
	{
		Rollback();
		return Reject(
			Edemo_mapItemUseStatus::CommitFailed,
			TEXT("Automation-injected Health failure rolled back item use."));
	}
	if (Intent.FailurePoint
		== Edemo_mapItemUseFailurePoint::BeforeCooldownCommit)
	{
		Rollback();
		return Reject(
			Edemo_mapItemUseStatus::CommitFailed,
			TEXT("Automation-injected cooldown failure rolled back item use."));
	}
#endif

	HealingPillCooldownEndTime =
		GetItemUseTimeSeconds()
		+ Fdemo_mapItemUsePrototypeConfig::
			HealingPillGlobalCooldownSeconds;
	RefreshHotbarBindings();
	FString InvariantError;
	if (!ValidateInvariants(&InvariantError))
	{
		Rollback();
		return Reject(
			Edemo_mapItemUseStatus::CommitFailed,
			TEXT("Post-use invariants failed; the transaction was rolled back."));
	}

	Result.Status = Edemo_mapItemUseStatus::Success;
	Result.AfterHealth = Health->GetCurrentHealth();
	const Fdemo_mapItemInstance* AfterInstance =
		Authority.FindInstance(BoundId);
	Result.AfterStack = AfterInstance
		? AfterInstance->Quantity
		: 0;
	Result.bBindingCleared =
		!HotbarBindings.SlotBindings[
			Intent.HotbarSlotNumber - 1].IsValid();
	Result.CooldownAfter =
		GetItemUseCooldownSnapshot().RemainingSeconds;
	Result.Diagnostic =
		TEXT("Healing Pill transaction committed atomically.");
	return Result;
}

Fdemo_mapItemUseResult Udemo_mapItemSubsystem::UseInventoryItem(
	FGuid InstanceId,
	bool bUIInputAllowed)
{
	const Fdemo_mapHotbarBindingSnapshot SavedBindings = HotbarBindings;
	if (HotbarBindings.SlotBindings.Num()
		!= Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		HotbarBindings = Fdemo_mapHotbarBindingSnapshot();
	}
	int32 SlotIndex =
		HotbarBindings.SlotBindings.IndexOfByKey(InstanceId);
	if (SlotIndex == INDEX_NONE)
	{
		SlotIndex = HotbarBindings.SlotBindings.IndexOfByPredicate(
			[](const FGuid& Id) { return !Id.IsValid(); });
	}
	if (SlotIndex == INDEX_NONE)
	{
		SlotIndex = 0;
	}
	HotbarBindings.SlotBindings[SlotIndex] = InstanceId;
	Fdemo_mapItemUseIntent Intent;
	Intent.ExpectedRunId = ActiveRunId;
	Intent.HotbarSlotNumber = SlotIndex + 1;
	Intent.ExpectedItemInstanceId = InstanceId;
	Fdemo_mapItemUseResult Result =
		UseHotbarSlot(Intent, bUIInputAllowed);
	HotbarBindings = SavedBindings;
	RefreshHotbarBindings();
	Result.bBindingCleared =
		!HotbarBindings.SlotBindings.Contains(InstanceId);
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::TagAffectedForActiveRun(const Fdemo_mapItemAuthorityState& Before, const Fdemo_mapItemOperationResult& Result, const TArray<FGuid>& Affected)
{
	if (!Result.bSuccess || RunState != Edemo_mapRunState::Active) return Result;
	const Fdemo_mapItemOperationResult TagResult = Authority.TagInstancesForRun(Affected, ActiveRunId);
	if (TagResult.bSuccess) return Result;
	Authority.RestoreState(Before);
	return TagResult;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::BeginRun()
{
	if (RunState == Edemo_mapRunState::Active) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::RunAlreadyActive, TEXT("A run is already active."));
	if (RunState == Edemo_mapRunState::Settling) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SettlementInProgress, TEXT("Cannot begin a run while settlement is in progress."));
	for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair : Authority.GetInstanceSnapshot())
	{
		if (Pair.Value.OwnershipState != Edemo_mapItemOwnershipState::SessionStash && Pair.Value.OwnershipState != Edemo_mapItemOwnershipState::Destroyed)
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, TEXT("A new run requires empty run inventory, equipment, and world ownership."), Pair.Key, Pair.Value.DefinitionId);
	}
	ActiveRunId = FGuid::NewGuid();
	if (!ActiveRunId.IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidRunId, TEXT("Failed to allocate a run identity."));
	DeployedItemIds.Reset();
	RunState = Edemo_mapRunState::Active;
	ClearItemUseCooldown();
	LootSourceStates.Reset();
	UE_LOG(LogTemp, Log, TEXT("0.3.4.0 RUN_BEGIN run=%s stash_items=%d stash_value=%d"), *ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens), GetSessionStashItemCount(), GetSessionStashValue());
	return Fdemo_mapItemOperationResult::Success();
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::PrepareForPersistentRun()
{
	if (RunState == Edemo_mapRunState::Active
		|| RunState == Edemo_mapRunState::Settling)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			TEXT("Preparation cannot clear an active or settling Runtime."));
	}

	const int32 ResidueCount = Authority.GetInstanceSnapshot().Num();
	const Edemo_mapRunState PreviousState = RunState;
	if (ActiveWorld.IsValid())
	{
		TeardownWorld(ActiveWorld.Get());
	}
	RemoveAllEquipmentSourcesFromBoundComponent();
	Authority.Reset();
	ClearHotbarBindings();
	ClearItemUseCooldown();
	WorldActors.Reset();
	ActiveWorld.Reset();
	RunState = Edemo_mapRunState::Inactive;
	ActiveRunId.Invalidate();
	DeployedItemIds.Reset();
	LastSettlementSummary = Fdemo_mapSettlementSummary();
	LootSourceStates.Reset();

	FString Error;
	if (!ValidateInvariants(&Error))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			TEXT("Runtime Preparation cleanup failed invariants: ") + Error);
	}
	UE_LOG(
		LogTemp,
		Log,
		TEXT("XFIX1_RUNTIME_PREPARATION_READY previous_state=%d cleared_transient_instances=%d"),
		static_cast<int32>(PreviousState),
		ResidueCount);
	return Fdemo_mapItemOperationResult::Success();
}

Fdemo_mapPreparedRunRuntimeResult Udemo_mapItemSubsystem::MaterializePreparedRun(const Fdemo_mapPreparedRunRuntimeRequest& Request)
{
	const Fdemo_mapCommittedRunLoadoutPlan& Plan = Request.CommittedPlan;
	auto Reject = [&Plan](Edemo_mapPreparedRunRuntimeStatus Status, const FString& Diagnostic)
	{
		Fdemo_mapPreparedRunRuntimeResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.ActiveRunId = Plan.ActiveRunId;
		return Result;
	};

#if WITH_DEV_AUTOMATION_TESTS
	const int32 FailureAfterMutation = PreparedRunFailureAfterMutation;
	PreparedRunFailureAfterMutation = INDEX_NONE;
#else
	const int32 FailureAfterMutation = INDEX_NONE;
#endif

	if (RunState != Edemo_mapRunState::Inactive || !Authority.GetInstanceSnapshot().IsEmpty() || !WorldActors.IsEmpty() || !ActiveModifierSources.IsEmpty())
		return Reject(Edemo_mapPreparedRunRuntimeStatus::RuntimeNotIdle, TEXT("Prepared Run materialization requires an idle Runtime with no owned item residue."));
	if (!Plan.ProfileId.IsValid() || Plan.CommittedGeneration <= 0)
		return Reject(Edemo_mapPreparedRunRuntimeStatus::CommittedPlanInvalid, TEXT("Committed Profile identity or generation is invalid."));
	if (!Plan.ActiveRunId.IsValid() || Plan.ActiveRunId == Plan.ProfileId)
		return Reject(Edemo_mapPreparedRunRuntimeStatus::RunIdInvalidOrConflicting, TEXT("Committed ActiveRunId is invalid or conflicts with ProfileId."));
	if (Plan.OrderedItems.Num()
			> 5 + Fdemo_mapPersistentPreparationLayout::MaxRunInventoryItems
		|| Plan.DeployedItemIds.Num() != Plan.OrderedItems.Num())
	{
		return Reject(
			Edemo_mapPreparedRunRuntimeStatus::DuplicateOrMissingDeployedId,
			TEXT("ActiveRunItems and DeployedItemIds must have the same bounded size."));
	}
	TArray<FGuid> EffectiveHotbar = Plan.HotbarItemInstanceIds;
	if (EffectiveHotbar.IsEmpty()) EffectiveHotbar.Init(FGuid(), Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
	if (EffectiveHotbar.Num() != Fdemo_mapPersistentPreparationLayout::HotbarSlotCount)
		return Reject(Edemo_mapPreparedRunRuntimeStatus::CommittedPlanInvalid, TEXT("Committed Hotbar must contain exactly nine slots."));

	TSet<FGuid> ItemIds;
	TSet<FGuid> DeclaredDeployedIds;
	for (const FGuid& Id : Plan.DeployedItemIds)
	{
		if (!Id.IsValid() || DeclaredDeployedIds.Contains(Id)) return Reject(Edemo_mapPreparedRunRuntimeStatus::DuplicateOrMissingDeployedId, TEXT("DeployedItemIds contains an invalid or duplicate identity."));
		DeclaredDeployedIds.Add(Id);
	}

	TMap<FName, const Fdemo_mapPersistentItemRecord*> EquipmentBySlot;
	TArray<const Fdemo_mapPersistentItemRecord*> RunInventory;
	TSet<FGuid> RunInventoryIds;
	for (const Fdemo_mapPersistentItemRecord& Item : Plan.OrderedItems)
	{
		if (!Item.ItemInstanceId.IsValid() || ItemIds.Contains(Item.ItemInstanceId) || !DeclaredDeployedIds.Contains(Item.ItemInstanceId))
			return Reject(Edemo_mapPreparedRunRuntimeStatus::DuplicateOrMissingDeployedId, TEXT("An ActiveRun item identity is invalid, duplicated, or missing from DeployedItemIds."));
		if (Item.ItemInstanceId == Plan.ProfileId || Item.ItemInstanceId == Plan.ActiveRunId)
			return Reject(Edemo_mapPreparedRunRuntimeStatus::RunIdInvalidOrConflicting, TEXT("Committed identities conflict with a deployed item identity."));
		ItemIds.Add(Item.ItemInstanceId);
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
		if (!Definition) return Reject(Edemo_mapPreparedRunRuntimeStatus::UnknownDefinition, TEXT("A committed item definition is unknown."));
		if (Item.PersistentDomain != Edemo_mapPersistentDomain::ActiveRun || Item.OriginRunId.IsValid())
			return Reject(Edemo_mapPreparedRunRuntimeStatus::CommittedPlanInvalid, TEXT("A deployed original must be in ActiveRun domain and retain an invalid OriginRunId."));
		if (Item.StackCount <= 0 || Item.StackCount > Definition->MaxStackSize)
			return Reject(Edemo_mapPreparedRunRuntimeStatus::QuantityRejected, TEXT("A committed StackCount violates its definition."));
		if (Item.EquipmentSlotId.IsNone())
		{
			if (Definition->CategoryId != Fdemo_mapItemIds::MaterialCategory
				&& Definition->CategoryId != Fdemo_mapItemIds::ConsumableCategory)
				return Reject(Edemo_mapPreparedRunRuntimeStatus::SlotCompatibilityRejected, TEXT("Only Material or Consumable definitions may enter Runtime inventory from a committed plan."));
			RunInventory.Add(&Item);
			RunInventoryIds.Add(Item.ItemInstanceId);
		}
		else
		{
			if (!IsFrozenPreparedRunEquipmentSlot(Item.EquipmentSlotId) || Item.StackCount != 1 || !Definition->CompatibleSlotIds.Contains(Item.EquipmentSlotId) || EquipmentBySlot.Contains(Item.EquipmentSlotId))
				return Reject(Edemo_mapPreparedRunRuntimeStatus::SlotCompatibilityRejected, TEXT("Committed equipment has an invalid, incompatible, or duplicate slot."));
			EquipmentBySlot.Add(Item.EquipmentSlotId, &Item);
		}
	}
	if (ItemIds.Num() != DeclaredDeployedIds.Num())
		return Reject(Edemo_mapPreparedRunRuntimeStatus::DuplicateOrMissingDeployedId, TEXT("DeployedItemIds contains an identity absent from ActiveRunItems."));
	if (EquipmentBySlot.Num() > 5
		|| RunInventory.Num()
			> Fdemo_mapPersistentPreparationLayout::MaxRunInventoryItems)
	{
		return Reject(
			Edemo_mapPreparedRunRuntimeStatus::CapacityRejected,
			TEXT("Committed plan exceeds five equipment or the carried-inventory structural maximum."));
	}
	FName BackpackDefinitionId = NAME_None;
	if (const Fdemo_mapPersistentItemRecord* const* Backpack = EquipmentBySlot.Find(Fdemo_mapItemIds::BackpackSlot))
		BackpackDefinitionId = (*Backpack)->ItemDefinitionId;
	FName SpatialRingDefinitionId = NAME_None;
	if (const Fdemo_mapPersistentItemRecord* const* SpatialRing = EquipmentBySlot.Find(Fdemo_mapItemIds::SpatialRingSlot))
		SpatialRingDefinitionId = (*SpatialRing)->ItemDefinitionId;
	const Fdemo_mapInventoryCapacityResult Capacity =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			BackpackDefinitionId,
			SpatialRingDefinitionId);
	if (!Capacity.bSuccess
		|| (Plan.RunInventoryCapacity != 0 && Plan.RunInventoryCapacity != Capacity.Capacity)
		|| RunInventory.Num() > Capacity.Capacity)
		return Reject(Edemo_mapPreparedRunRuntimeStatus::CapacityRejected, TEXT("Committed carried-inventory capacity does not match the fixed-six plus space-item contract."));
	TSet<FGuid> HotbarIds;
	for (const FGuid& Id : EffectiveHotbar)
	{
		if (!Id.IsValid()) continue;
		const Fdemo_mapPersistentItemRecord* Item = Plan.OrderedItems.FindByPredicate(
			[Id](const Fdemo_mapPersistentItemRecord& Candidate) { return Candidate.ItemInstanceId == Id; });
		const Fdemo_mapItemDefinition* Definition = Item ? Fdemo_mapItemDefinitions::Find(Item->ItemDefinitionId) : nullptr;
		if (HotbarIds.Contains(Id) || !RunInventoryIds.Contains(Id) || !Definition
			|| Definition->CategoryId != Fdemo_mapItemIds::ConsumableCategory)
			return Reject(Edemo_mapPreparedRunRuntimeStatus::CommittedPlanInvalid, TEXT("Committed Hotbar contains a duplicate or non-selected non-Consumable reference."));
		HotbarIds.Add(Id);
	}

	TArray<const Fdemo_mapPersistentItemRecord*> MaterializationOrder;
	for (FName SlotId : { Fdemo_mapItemIds::WeaponSlot, Fdemo_mapItemIds::ArmorSlot, Fdemo_mapItemIds::AccessorySlot, Fdemo_mapItemIds::SpatialRingSlot, Fdemo_mapItemIds::BackpackSlot })
	{
		if (const Fdemo_mapPersistentItemRecord* const* Item = EquipmentBySlot.Find(SlotId)) MaterializationOrder.Add(*Item);
	}
	MaterializationOrder.Append(RunInventory);

	const Fdemo_mapItemAuthorityState AuthorityBefore = Authority.CaptureState();
	const Edemo_mapRunState RunStateBefore = RunState;
	const FGuid ActiveRunIdBefore = ActiveRunId;
	const TSet<FGuid> DeployedBefore = DeployedItemIds;
	const Fdemo_mapSettlementSummary SettlementBefore = LastSettlementSummary;
	const TMap<FGuid, Edemo_mapLootSourceState> LootBefore = LootSourceStates;
	const Fdemo_mapHotbarBindingSnapshot HotbarBefore = HotbarBindings;
	auto Rollback = [this, &AuthorityBefore, RunStateBefore, ActiveRunIdBefore, &DeployedBefore, &SettlementBefore, &LootBefore, &HotbarBefore, &Reject](const FString& Diagnostic)
	{
		Authority.RestoreState(AuthorityBefore);
		RunState = RunStateBefore;
		ActiveRunId = ActiveRunIdBefore;
		DeployedItemIds = DeployedBefore;
		LastSettlementSummary = SettlementBefore;
		LootSourceStates = LootBefore;
		HotbarBindings = HotbarBefore;
		if (!SynchronizeEquipmentModifiers()) return Reject(Edemo_mapPreparedRunRuntimeStatus::RollbackFailed, TEXT("Prepared Run rollback could not restore equipment modifiers: ") + Diagnostic);
		return Reject(Edemo_mapPreparedRunRuntimeStatus::AuthorityMutationRejected, Diagnostic);
	};

	ActiveRunId = Plan.ActiveRunId;
	DeployedItemIds = DeclaredDeployedIds;
	RunState = Edemo_mapRunState::Active;
	LastSettlementSummary = Fdemo_mapSettlementSummary();
	LootSourceStates.Reset();
	int32 MutationCount = 0;
	for (const Fdemo_mapPersistentItemRecord* Item : MaterializationOrder)
	{
		const Fdemo_mapItemOperationResult Mutation =
			Authority.MaterializeDeployedInstance(
				Item->ItemInstanceId,
				Item->ItemDefinitionId,
				Item->StackCount,
				Item->EquipmentSlotId,
				Item->OriginRunId,
				Item->RewardEventKind,
				Item->RewardEventId,
				Item->RewardValueMultiplierBps,
				Item->RewardSourceRoleId,
				Item->RareRewardEventId,
				Item->RareRewardPolicyId,
				Item->RareRewardTierId,
				Item->RareRewardBonusValue,
				Item->AffixSet);
		if (!Mutation.bSuccess) return Rollback(TEXT("Authority rejected a deployed item mutation: ") + Mutation.Diagnostic);
		++MutationCount;
		if (FailureAfterMutation == MutationCount) return Rollback(TEXT("Automation-injected Prepared Run mutation failure."));
	}
	for (int32 Index = 0; Index < EffectiveHotbar.Num(); ++Index)
	{
		const FGuid Id = EffectiveHotbar[Index];
		if (!Id.IsValid()) continue;
		const Fdemo_mapItemOperationResult Binding = BindHotbarSlot(Index + 1, Id);
		if (!Binding.bSuccess) return Rollback(TEXT("Committed Hotbar materialization failed: ") + Binding.Diagnostic);
	}
	if (!SynchronizeEquipmentModifiers()) return Rollback(TEXT("Equipment modifier application rejected the committed plan."));
	FString InvariantError;
	if (!ValidateInvariants(&InvariantError)) return Rollback(TEXT("Prepared Run post-materialization invariant failed: ") + InvariantError);

	Fdemo_mapPreparedRunRuntimeResult Result;
	Result.Status = Edemo_mapPreparedRunRuntimeStatus::Materialized;
	Result.ActiveRunId = ActiveRunId;
	Result.DeployedItemIds = Plan.DeployedItemIds;
	ClearItemUseCooldown();
	return Result;
}

bool Udemo_mapItemSubsystem::IsItemAtRiskInActiveRun(FGuid InstanceId) const
{
	return (RunState == Edemo_mapRunState::Active || RunState == Edemo_mapRunState::Settling)
		&& Authority.IsItemAtRiskInRun(InstanceId, ActiveRunId, DeployedItemIds);
}

int32 Udemo_mapItemSubsystem::GetSessionStashItemCount() const
{
	int32 Count = 0;
	for (const FGuid& Id : Authority.GetSessionStashSnapshot()) if (const Fdemo_mapItemInstance* Item = Authority.FindInstance(Id)) Count += Item->Quantity;
	return Count;
}

int32 Udemo_mapItemSubsystem::GetSessionStashValue() const
{
	int32 Value = 0;
	for (const FGuid& Id : Authority.GetSessionStashSnapshot())
	{
		const Fdemo_mapItemInstance* Item = Authority.FindInstance(Id);
		const Fdemo_mapItemDefinition* Definition = Item ? Fdemo_mapItemDefinitions::Find(Item->DefinitionId) : nullptr;
		if (Item && Definition) Value += Item->Quantity * Definition->PrototypeValue;
	}
	return Value;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::RequestSettlement(Edemo_mapRunEndReason Reason, Fdemo_mapSettlementSummary& OutSummary)
{
	OutSummary = Fdemo_mapSettlementSummary();
	if (RunState == Edemo_mapRunState::Settling) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SettlementInProgress, TEXT("Settlement is already in progress."));
	if (RunState == Edemo_mapRunState::Settled)
	{
		OutSummary = LastSettlementSummary;
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SettlementAlreadyCompleted, TEXT("First legal settlement already won."));
	}
	if (RunState != Edemo_mapRunState::Active) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::RunNotActive, TEXT("No active run can be settled."));
	if (Reason == Edemo_mapRunEndReason::None) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidSettlementReason, TEXT("A terminal settlement reason is required."));

	const FGuid SettledRunId = ActiveRunId;
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	const Fdemo_mapHotbarBindingSnapshot HotbarBefore =
		HotbarBindings;
	const double CooldownEndBefore =
		HealingPillCooldownEndTime;
	RunState = Edemo_mapRunState::Settling;
	TArray<Fdemo_mapSettlementItemRow> Rows;
	Fdemo_mapItemOperationResult Result = Authority.SettleRunItems(SettledRunId, DeployedItemIds, Reason == Edemo_mapRunEndReason::Extraction, Rows);
	if (!Result.bSuccess || !SynchronizeEquipmentModifiers())
	{
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		RunState = Edemo_mapRunState::Active;
		return Result.bSuccess ? Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SettlementRollbackFailed, TEXT("Settlement modifier cleanup failed; authority state was rolled back.")) : Result;
	}
	RefreshHotbarBindings();

	ClearSpatialBundleTracking();
	for (const Fdemo_mapSettlementItemRow& Row : Rows)
	{
		if (Row.SourceOwnership != Edemo_mapItemOwnershipState::World) continue;
		TWeakObjectPtr<Ademo_mapWorldItem> Actor = WorldActors.FindRef(Row.InstanceId);
		RemoveWorldBinding(Row.InstanceId);
		if (Actor.IsValid()) Actor->Destroy();
	}
	Authority.CompactDestroyedRun(SettledRunId, DeployedItemIds);
	Fdemo_mapSettlementSummary Summary;
	Summary.RunId = SettledRunId;
	Summary.Reason = Reason;
	Summary.Rows = Rows;
	for (const Fdemo_mapSettlementItemRow& Row : Rows)
	{
		if (Row.FinalOwnership == Edemo_mapItemOwnershipState::SessionStash) { Summary.SecuredItemCount += Row.Quantity; Summary.SecuredValue += Row.TotalValue; }
		else { Summary.LostItemCount += Row.Quantity; Summary.LostValue += Row.TotalValue; }
	}
	Summary.StashItemCountAfter = GetSessionStashItemCount();
	Summary.StashValueAfter = GetSessionStashValue();
	Summary.RuntimeSnapshot.ActiveRunId = SettledRunId;
	Summary.RuntimeSnapshot.CommittedEndReason = Reason;
	if (Reason == Edemo_mapRunEndReason::Extraction)
	{
		for (const FGuid& Id : Authority.GetSessionStashSnapshot())
		{
			const Fdemo_mapItemInstance* Item = Authority.FindInstance(Id);
			if (!Item || (!DeployedItemIds.Contains(Id) && Item->OriginRunId != SettledRunId)) continue;
			Fdemo_mapRuntimeSettlementItem Secured;
			Secured.ItemInstanceId = Item->InstanceId;
			Secured.ItemDefinitionId = Item->DefinitionId;
			Secured.StackCount = Item->Quantity;
			Secured.OriginRunId = Item->OriginRunId;
			Secured.RewardEventKind = Item->RewardEventKind;
			Secured.RewardEventId = Item->RewardEventId;
			Secured.RewardValueMultiplierBps =
				Item->RewardValueMultiplierBps;
			Secured.RewardSourceRoleId =
				Item->RewardSourceRoleId;
			Secured.RareRewardEventId =
				Item->RareRewardEventId;
			Secured.RareRewardPolicyId =
				Item->RareRewardPolicyId;
			Secured.RareRewardTierId =
				Item->RareRewardTierId;
			Secured.RareRewardBonusValue =
				Item->RareRewardBonusValue;
			Secured.AffixSet = Item->AffixSet;
			Summary.RuntimeSnapshot.OrderedSecuredItems.Add(MoveTemp(Secured));
		}
	}
	Summary.RuntimeSnapshot.bValid = true;
	Summary.bValid = true;
	LastSettlementSummary = Summary;
	OutSummary = Summary;
	RunState = Edemo_mapRunState::Settled;
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		Authority.RestoreState(Before);
		HotbarBindings = HotbarBefore;
		HealingPillCooldownEndTime = CooldownEndBefore;
		SynchronizeEquipmentModifiers();
		RunState = Edemo_mapRunState::Active;
		LastSettlementSummary = Fdemo_mapSettlementSummary();
		OutSummary = Fdemo_mapSettlementSummary();
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SettlementRollbackFailed, TEXT("Post-settlement invariant failed: ") + Error);
	}
	ClearItemUseCooldown();
	UE_LOG(LogTemp, Log, TEXT("0.3.4.0 RUN_SETTLED run=%s reason=%d secured=%d/%d lost=%d/%d stash=%d/%d"), *SettledRunId.ToString(EGuidFormats::DigitsWithHyphens), static_cast<int32>(Reason), Summary.SecuredItemCount, Summary.SecuredValue, Summary.LostItemCount, Summary.LostValue, Summary.StashItemCountAfter, Summary.StashValueAfter);
	return Fdemo_mapItemOperationResult::Success();
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::CreateEnemyLoot(UWorld* World, Edemo_mapEnemyLootArchetype Archetype, FGuid LootSourceId, const FVector& DeathLocation, const AActor* IgnoredActor, TArray<Ademo_mapWorldItem*>& OutActors, bool bEligibleHostile)
{
	OutActors.Reset();
	if (RunState != Edemo_mapRunState::Active) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::RunNotActive, TEXT("Enemy loot requires an active V3 run."));
	if (!bEligibleHostile) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::ForbiddenLootSource, TEXT("Only the three V3 hostile archetypes may create loot."));
	if (!LootSourceId.IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::LootSourceInvalid, TEXT("Loot source GUID is invalid."));
	if (const Edemo_mapLootSourceState* State = LootSourceStates.Find(LootSourceId))
	{
		return Fdemo_mapItemOperationResult::Failure(*State == Edemo_mapLootSourceState::Processing ? Edemo_mapItemResultCode::LootSourceProcessing : Edemo_mapItemResultCode::LootSourceCompleted, TEXT("Loot source was already processed."));
	}
	const FName TableId = Fdemo_mapItemDefinitions::GetEnemyLootProfileId(Archetype);
	const TArray<Fdemo_mapLootTableEntry>* Entries =
		Fdemo_mapItemDefinitions::FindEnemyLootProfile(TableId);
	if (!Entries) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::LootTableNotFound, TEXT("Fixed enemy loot table was not found."), FGuid(), NAME_None, NAME_None, TableId);
	LootSourceStates.Add(LootSourceId, Edemo_mapLootSourceState::Processing);
	TArray<Fdemo_mapWorldSpawnRequest> Requests;
	for (int32 Index = 0; Index < Entries->Num(); ++Index)
	{
		Fdemo_mapWorldSpawnRequest Request;
		Request.DefinitionId = (*Entries)[Index].DefinitionId;
		Request.Quantity = (*Entries)[Index].Quantity;
		Request.DesiredLocation = DeathLocation + FVector(Index * 85.0f, 0.0f, 0.0f);
		Request.SourceId = FName(*LootSourceId.ToString(EGuidFormats::Digits));
		Requests.Add(Request);
	}
	Fdemo_mapItemOperationResult Result = CreateWorldItemsAtomically(World, Requests, OutActors);
	if (!Result.bSuccess) { LootSourceStates.Remove(LootSourceId); return Result; }
	LootSourceStates[LootSourceId] = Edemo_mapLootSourceState::Completed;
	UE_LOG(LogTemp, Log, TEXT("0.3.4.0 ENEMY_LOOT_COMPLETE source=%s table=%s actors=%d"), *LootSourceId.ToString(EGuidFormats::DigitsWithHyphens), *TableId.ToString(), OutActors.Num());
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::Equip(FGuid InstanceId, FName SlotId)
{
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	return FinalizeTransaction(Before, Authority.Equip(InstanceId, SlotId));
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::Unequip(FName SlotId)
{
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	return FinalizeTransaction(Before, Authority.Unequip(SlotId));
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::Destroy(FGuid InstanceId)
{
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	const Fdemo_mapItemOperationResult Result = FinalizeTransaction(Before, Authority.Destroy(InstanceId));
	if (Result.bSuccess)
	{
		RefreshHotbarBindings();
	}
	return Result;
}

void Udemo_mapItemSubsystem::BeginWorld(UWorld* World)
{
	if (ActiveWorld.Get() == World) return;
	if (ActiveWorld.IsValid()) TeardownWorld(ActiveWorld.Get());
	ActiveWorld = World;
}

void Udemo_mapItemSubsystem::TeardownWorld(UWorld* World)
{
	if (World == nullptr || ActiveWorld.Get() != World) return;
	ClearSpatialBundleTracking();
	const TArray<FGuid> WorldIds = Authority.FindWorldInstances();
	for (const FGuid& InstanceId : WorldIds)
	{
		TWeakObjectPtr<Ademo_mapWorldItem> Actor = WorldActors.FindRef(InstanceId);
		RemoveWorldBinding(InstanceId);
		if (Actor.IsValid()) Actor->Destroy();
		if (const Fdemo_mapItemInstance* Instance = Authority.FindInstance(InstanceId); Instance != nullptr && Instance->OwnershipState == Edemo_mapItemOwnershipState::World)
		{
			Authority.DestroyWorld(InstanceId);
		}
	}
	WorldActors.Reset();
	ActiveWorld.Reset();
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::CreateWorldItem(UWorld* World, FName DefinitionId, int32 Quantity, const FVector& DesiredLocation, Ademo_mapWorldItem*& OutActor, FName SourceId)
{
	OutActor = nullptr;
	Fdemo_mapWorldSpawnRequest Request;
	Request.DefinitionId = DefinitionId;
	Request.Quantity = Quantity;
	Request.DesiredLocation = DesiredLocation;
	Request.SourceId = SourceId;
	TArray<Ademo_mapWorldItem*> Actors;
	Fdemo_mapItemOperationResult Result = CreateWorldItemsAtomically(World, { Request }, Actors);
	if (Result.bSuccess && Actors.Num() == 1) OutActor = Actors[0];
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::CreateWorldItemsAtomically(UWorld* World, const TArray<Fdemo_mapWorldSpawnRequest>& Requests, TArray<Ademo_mapWorldItem*>& OutActors)
{
	OutActors.Reset();
	if (World == nullptr || Requests.IsEmpty()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::LootSpawnFailed, TEXT("World and at least one spawn request are required."));
	BeginWorld(World);
	TArray<FVector> SafeLocations;
	for (int32 RequestIndex = 0; RequestIndex < Requests.Num(); ++RequestIndex)
	{
		const Fdemo_mapWorldSpawnRequest& Request = Requests[RequestIndex];
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Request.DefinitionId);
		if (Definition == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("World spawn definition is not registered."), FGuid(), Request.DefinitionId, NAME_None, Request.SourceId);
		if (!Definition->bWorldDropEligible) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::ForbiddenLootSource, TEXT("World spawn definition is not WorldDrop eligible."), FGuid(), Request.DefinitionId, NAME_None, Request.SourceId);
		if (Request.Quantity <= 0 || Request.Quantity > Definition->MaxStackSize) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidQuantity, TEXT("World spawn quantity must fit one stack."), FGuid(), Request.DefinitionId, NAME_None, Request.SourceId);
		FVector SafeLocation;
		const FName StableSeed = Request.SourceId.IsNone()
			? FName(*FString::Printf(TEXT("WorldBatch.%d"), RequestIndex))
			: Request.SourceId;
		if (!ResolveSafeWorldLocation(
			World,
			Request.DesiredLocation,
			nullptr,
			SafeLocation,
			StableSeed,
			&SafeLocations))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::UnsafeDropLocation,
				TEXT("No safe, separated world location was found."),
				FGuid(), Request.DefinitionId, NAME_None, Request.SourceId);
		}
		SafeLocations.Add(SafeLocation);
	}

	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	TArray<FGuid> CreatedIds;
	auto Rollback = [this, &Before, &CreatedIds, &OutActors]()
	{
		for (int32 Index = 0; Index < CreatedIds.Num(); ++Index)
		{
			RemoveWorldBinding(CreatedIds[Index]);
			if (OutActors.IsValidIndex(Index) && IsValid(OutActors[Index])) OutActors[Index]->Destroy();
		}
		Authority.RestoreState(Before);
		OutActors.Reset();
	};

	for (int32 Index = 0; Index < Requests.Num(); ++Index)
	{
		FGuid InstanceId;
		Fdemo_mapItemOperationResult Core = Authority.CreateWorldDefinition(Requests[Index].DefinitionId, Requests[Index].Quantity, InstanceId);
		if (!Core.bSuccess)
		{
			Rollback();
			return Core;
		}
		CreatedIds.Add(InstanceId);
		if (RunState == Edemo_mapRunState::Active)
		{
			Fdemo_mapItemOperationResult TagResult = Authority.TagInstancesForRun({ InstanceId }, ActiveRunId);
			if (!TagResult.bSuccess) { Rollback(); return TagResult; }
		}
		Ademo_mapWorldItem* Actor = SpawnBoundWorldActor(World, InstanceId, SafeLocations[Index]);
		if (Actor == nullptr)
		{
			Rollback();
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::WorldActorSpawnFailed, TEXT("Deferred World Item actor spawn failed."), InstanceId, Requests[Index].DefinitionId, NAME_None, Requests[Index].SourceId);
		}
		OutActors.Add(Actor);
	}
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		Rollback();
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, Error);
	}
	return Fdemo_mapItemOperationResult::Success(CreatedIds[0], Requests[0].DefinitionId, NAME_None, Requests[0].SourceId);
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::CreateContainerItem(
	FGuid ContainerId,
	FName DefinitionId,
	int32 Quantity,
	FGuid& OutInstanceId)
{
	OutInstanceId.Invalidate();
	if (RunState != Edemo_mapRunState::Active || !ActiveRunId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Runtime Container item creation requires an ActiveRun."),
			FGuid(),
			DefinitionId);
	}
	return Authority.CreateContainerDefinition(
		DefinitionId,
		Quantity,
		ContainerId,
		ActiveRunId,
		OutInstanceId);
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::MaterializeContainerItemsAtomically(
	FGuid ContainerId,
	const TArray<Fdemo_mapContainerMaterializationRequest>& Requests,
	TFunctionRef<bool(const TArray<FGuid>& InstanceIds, FString& OutDiagnostic)> FinalizeMaterialization,
	TArray<FGuid>& OutInstanceIds
#if WITH_DEV_AUTOMATION_TESTS
	, int32 FailureAfterMutation
#endif
)
{
	OutInstanceIds.Reset();
	if (RunState != Edemo_mapRunState::Active || !ActiveRunId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Atomic Runtime Container materialization requires an ActiveRun."));
	}
	if (!ContainerId.IsValid() || Requests.IsEmpty())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidContainer,
			TEXT("Atomic Runtime Container materialization requires a valid ContainerId and at least one request."));
	}

	for (const Fdemo_mapContainerMaterializationRequest& Request : Requests)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Request.DefinitionId);
		if (!Definition)
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::UnknownDefinition,
				TEXT("Atomic Runtime Container materialization found an unknown DefinitionId."),
				FGuid(),
				Request.DefinitionId);
		}
		if (Request.Quantity <= 0 || Request.Quantity > Definition->MaxStackSize)
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvalidQuantity,
				TEXT("Atomic Runtime Container materialization quantity must fit one stack."),
				FGuid(),
				Request.DefinitionId);
		}
		FString RewardError;
		if (!Fdemo_mapRewardEventRules::IsValid(
			Request.RewardEventKind,
			Request.RewardEventId,
			Request.RewardValueMultiplierBps,
			Request.RewardSourceRoleId,
			Request.RareRewardEventId,
			Request.RareRewardPolicyId,
			Request.RareRewardTierId,
			Request.RareRewardBonusValue,
			&RewardError))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvariantViolation,
				RewardError,
				FGuid(),
				Request.DefinitionId);
		}
		if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
			Request.DefinitionId,
			Request.Quantity,
			Request.AffixSet,
			&RewardError))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvariantViolation,
				RewardError,
				FGuid(),
				Request.DefinitionId);
		}
	}

	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	auto Rollback = [this, &Before, &OutInstanceIds]()
	{
		Authority.RestoreState(Before);
		OutInstanceIds.Reset();
	};
	for (int32 Index = 0; Index < Requests.Num(); ++Index)
	{
		FGuid InstanceId;
		const Fdemo_mapContainerMaterializationRequest& Request =
			Requests[Index];
		const Fdemo_mapItemOperationResult Created =
			Authority.CreateContainerDefinition(
				Request.DefinitionId,
				Request.Quantity,
				ContainerId,
				ActiveRunId,
				InstanceId,
				Request.RewardEventKind,
				Request.RewardEventId,
				Request.RewardValueMultiplierBps,
				Request.RewardSourceRoleId,
				Request.RareRewardEventId,
				Request.RareRewardPolicyId,
				Request.RareRewardTierId,
				Request.RareRewardBonusValue,
				Request.AffixSet);
		if (!Created.bSuccess || !InstanceId.IsValid())
		{
			Rollback();
			return Created.bSuccess
				? Fdemo_mapItemOperationResult::Failure(
					Edemo_mapItemResultCode::InvariantViolation,
					TEXT("Atomic Runtime Container materialization created an invalid ItemInstance identity."),
					InstanceId,
					Request.DefinitionId)
				: Created;
		}
		OutInstanceIds.Add(InstanceId);
#if WITH_DEV_AUTOMATION_TESTS
		if (FailureAfterMutation != INDEX_NONE
			&& OutInstanceIds.Num() >= FailureAfterMutation)
		{
			Rollback();
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvariantViolation,
				TEXT("Injected atomic Runtime Container materialization failure."));
		}
#endif
	}

	FString Diagnostic;
	if (!FinalizeMaterialization(OutInstanceIds, Diagnostic))
	{
		Rollback();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			Diagnostic.IsEmpty()
				? TEXT("Atomic Runtime Container materialization finalizer rejected the batch.")
				: Diagnostic);
	}
	if (!ValidateInvariants(&Diagnostic))
	{
		Rollback();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			Diagnostic);
	}
	return Fdemo_mapItemOperationResult::Success(
		OutInstanceIds[0],
		Requests[0].DefinitionId);
}

Fdemo_mapItemOperationResult
Udemo_mapItemSubsystem::MaterializeCommittedContainerItemsAtomically(
	FGuid ContainerId,
	const TArray<Fdemo_mapContainerMaterializationRequest>& Requests,
	const TArray<FGuid>& CommittedInstanceIds,
	TFunctionRef<bool(const TArray<FGuid>& InstanceIds, FString& OutDiagnostic)>
		FinalizeMaterialization,
	TArray<FGuid>& OutInstanceIds)
{
	OutInstanceIds.Reset();
	if (RunState != Edemo_mapRunState::Active || !ActiveRunId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Committed Runtime Container projection requires an ActiveRun."));
	}
	if (!ContainerId.IsValid() || Requests.IsEmpty()
		|| Requests.Num() != CommittedInstanceIds.Num())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidContainer,
			TEXT("Committed Runtime Container projection requires one durable identity per request."));
	}
	TSet<FGuid> RequestedIds;
	for (const FGuid& Id : CommittedInstanceIds)
	{
		if (!Id.IsValid() || RequestedIds.Contains(Id))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvariantViolation,
				TEXT("Committed Runtime Container contains an invalid or duplicate item identity."),
				Id);
		}
		RequestedIds.Add(Id);
	}

	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	auto Rollback = [this, &Before, &OutInstanceIds]()
	{
		Authority.RestoreState(Before);
		OutInstanceIds.Reset();
	};
	for (int32 Index = 0; Index < Requests.Num(); ++Index)
	{
		const Fdemo_mapContainerMaterializationRequest& Request = Requests[Index];
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Request.DefinitionId);
		if (!Definition || Request.Quantity <= 0
			|| Request.Quantity > Definition->MaxStackSize)
		{
			Rollback();
			return Fdemo_mapItemOperationResult::Failure(
				Definition ? Edemo_mapItemResultCode::InvalidQuantity
					: Edemo_mapItemResultCode::UnknownDefinition,
				TEXT("Committed Runtime Container source has an invalid definition or stack."),
				CommittedInstanceIds[Index], Request.DefinitionId);
		}
		FString RewardError;
		if (!Fdemo_mapRewardEventRules::IsValid(
			Request.RewardEventKind, Request.RewardEventId,
			Request.RewardValueMultiplierBps, Request.RewardSourceRoleId,
			Request.RareRewardEventId, Request.RareRewardPolicyId,
			Request.RareRewardTierId, Request.RareRewardBonusValue,
			&RewardError)
			|| !Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
				Request.DefinitionId, Request.Quantity, Request.AffixSet,
				&RewardError))
		{
			Rollback();
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvariantViolation,
				RewardError, CommittedInstanceIds[Index], Request.DefinitionId);
		}
		FGuid CreatedId;
		const Fdemo_mapItemOperationResult Created =
			Authority.CreateContainerDefinition(
				Request.DefinitionId, Request.Quantity, ContainerId,
				ActiveRunId, CreatedId, Request.RewardEventKind,
				Request.RewardEventId, Request.RewardValueMultiplierBps,
				Request.RewardSourceRoleId, Request.RareRewardEventId,
				Request.RareRewardPolicyId, Request.RareRewardTierId,
				Request.RareRewardBonusValue, Request.AffixSet,
				CommittedInstanceIds[Index]);
		if (!Created.bSuccess || CreatedId != CommittedInstanceIds[Index])
		{
			Rollback();
			return Created.bSuccess
				? Fdemo_mapItemOperationResult::Failure(
					Edemo_mapItemResultCode::InvariantViolation,
					TEXT("Committed Runtime Container projection changed a durable item identity."),
					CreatedId, Request.DefinitionId)
				: Created;
		}
		OutInstanceIds.Add(CreatedId);
	}
	FString Diagnostic;
	if (!FinalizeMaterialization(OutInstanceIds, Diagnostic)
		|| !ValidateInvariants(&Diagnostic))
	{
		Rollback();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			Diagnostic.IsEmpty()
				? TEXT("Committed Runtime Container projection finalizer rejected the durable source.")
				: Diagnostic);
	}
	return Fdemo_mapItemOperationResult::Success(
		OutInstanceIds[0], Requests[0].DefinitionId);
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::TransferContainerItemToInventory(
	FGuid ContainerId,
	FGuid InstanceId,
	bool bFailAfterMutation)
{
	if (RunState != Edemo_mapRunState::Active || !ActiveRunId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Runtime Container Take requires an ActiveRun."),
			InstanceId);
	}
	const Fdemo_mapItemInstance* Before = Authority.FindInstance(InstanceId);
	if (!Before || Before->OriginRunId != ActiveRunId)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidRunId,
			TEXT("Runtime Container ItemInstance does not belong to the current ActiveRun."),
			InstanceId,
			Before ? Before->DefinitionId : NAME_None);
	}
	return Authority.TransferContainerToInventoryWhole(
		InstanceId,
		ContainerId,
		bFailAfterMutation);
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::DestroyContainerItem(
	FGuid ContainerId,
	FGuid InstanceId)
{
	const Fdemo_mapItemInstance* Item = Authority.FindInstance(InstanceId);
	if (!Item)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InstanceNotFound,
			TEXT("Runtime Container cleanup ItemInstance was already compacted."),
			InstanceId);
	}
	return Authority.DestroyContainer(InstanceId, ContainerId);
}

Ademo_mapWorldItem* Udemo_mapItemSubsystem::SpawnBoundWorldActor(UWorld* World, FGuid InstanceId, const FVector& Location)
{
	if (World == nullptr || !InstanceId.IsValid() || WorldActors.Contains(InstanceId)) return nullptr;
	const FTransform Transform(FRotator::ZeroRotator, Location);
	Ademo_mapWorldItem* Actor = World->SpawnActorDeferred<Ademo_mapWorldItem>(Ademo_mapWorldItem::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Actor == nullptr) return nullptr;
	for (const TPair<FGuid, TWeakObjectPtr<Ademo_mapWorldItem>>& Pair : WorldActors)
	{
		if (Pair.Value.Get() == Actor) { Actor->Destroy(); return nullptr; }
	}
	Actor->SetInstanceId(InstanceId);
	WorldActors.Add(InstanceId, Actor);
	AActor* Finished = UGameplayStatics::FinishSpawningActor(Actor, Transform);
	if (Finished == nullptr)
	{
		WorldActors.Remove(InstanceId);
		return nullptr;
	}
	Actor->RefreshPresentation();
	return Actor;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::PickupWorldItem(Ademo_mapWorldItem* Actor, APlayerController* Controller)
{
	if (!IsValid(Actor)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidWorldBinding, TEXT("World Item actor is invalid."));
	const FGuid InstanceId = Actor->GetInstanceId();
	const Fdemo_mapItemInstance* Instance = Authority.FindInstance(InstanceId);
	if (Instance == nullptr || Instance->OwnershipState != Edemo_mapItemOwnershipState::World) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::AlreadyClaimed, TEXT("World item was already claimed."), InstanceId);
	if (!IsWorldActorBound(InstanceId, Actor)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidWorldBinding, TEXT("Actor and world instance binding do not match."), InstanceId, Instance->DefinitionId);
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Pawn == nullptr || FVector::Dist(Pawn->GetActorLocation(), Actor->GetInteractionLocation()) > Fdemo_mapWorldInteractionRules::InteractionRangeUU) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InteractionOutOfRange, TEXT("World item is outside interaction distance."), InstanceId, Instance->DefinitionId);
	if (const FGuid BundleId = FindSpatialBundleId(InstanceId); BundleId.IsValid())
	{
		return RecoverSpatialItemBundle(BundleId, Controller);
	}
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	TArray<FGuid> Affected;
	Fdemo_mapItemOperationResult Result = Authority.PickupWorld(InstanceId, &Affected);
	if (!Result.bSuccess) return Result;
	Result = TagAffectedForActiveRun(Before, Result, Affected);
	if (!Result.bSuccess) return Result;
	RemoveWorldBinding(InstanceId, Actor);
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		Authority.RestoreState(Before);
		WorldActors.Add(InstanceId, Actor);
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, Error, InstanceId, Instance->DefinitionId);
	}
	Actor->Destroy();
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::DropInventoryItem(FGuid InstanceId, APawn* Pawn, Ademo_mapWorldItem*& OutActor)
{
	OutActor = nullptr;
	if (Pawn == nullptr || Pawn->GetWorld() == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnsafeDropLocation, TEXT("A live pawn is required to find a drop location."), InstanceId);
	FVector SafeLocation;
	if (!FindSafeDropLocation(Pawn, SafeLocation)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnsafeDropLocation, TEXT("No safe deterministic drop candidate was found."), InstanceId);
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	Fdemo_mapItemOperationResult Result = Authority.MoveInventoryToWorld(InstanceId);
	if (!Result.bSuccess) return Result;
	OutActor = SpawnBoundWorldActor(Pawn->GetWorld(), InstanceId, SafeLocation);
	if (OutActor == nullptr)
	{
		Authority.RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::WorldActorSpawnFailed, TEXT("Drop actor spawn failed and inventory was restored."), InstanceId, Result.RelatedDefinitionId);
	}
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		RemoveWorldBinding(InstanceId, OutActor);
		OutActor->Destroy();
		OutActor = nullptr;
		Authority.RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, Error, InstanceId, Result.RelatedDefinitionId);
	}
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::DropPlayerItemToWorld(
	const Fdemo_mapPlayerItemDropIntent& Intent,
	APawn* Pawn,
	Ademo_mapWorldItem*& OutActor)
{
	OutActor = nullptr;
	if (RunState != Edemo_mapRunState::Active || !ActiveRunId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("世界丢弃要求当前 Active Run。"),
			Intent.ExpectedSourceItemInstanceId);
	}
	if (Intent.ExpectedAuthorityRevision != Authority.GetAuthorityRevision()
		|| !Intent.ExpectedSourceItemInstanceId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("世界丢弃的物品引用已过期。"),
			Intent.ExpectedSourceItemInstanceId);
	}
	if (!Pawn || !Pawn->GetWorld())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnsafeDropLocation,
			TEXT("世界丢弃需要有效的玩家和地图。"),
			Intent.ExpectedSourceItemInstanceId);
	}

	int32 InventorySlot = INDEX_NONE;
	FName EquipmentSlot = NAME_None;
	if (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment)
	{
		EquipmentSlot = Intent.SourceEquipmentSlotId;
		if (!Authority.IsKnownEquipmentSlot(EquipmentSlot))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvalidSlot,
				TEXT("世界丢弃的装备槽无效。"),
				Intent.ExpectedSourceItemInstanceId);
		}
	}
	else if (!Authority.ResolveInventoryAreaSlot(
		Intent.SourceArea,
		Intent.SourceSlotIndex,
		InventorySlot))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidSlot,
			TEXT("世界丢弃的物品格无效。"),
			Intent.ExpectedSourceItemInstanceId);
	}
	const FGuid SourceId = Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment
		? Authority.EquipmentSlots.FindRef(EquipmentSlot)
		: Authority.InventorySlots[InventorySlot];
	if (SourceId != Intent.ExpectedSourceItemInstanceId)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("世界丢弃源物品已移动或为空。"),
			SourceId);
	}
	Fdemo_mapItemInstance* Source = Authority.Instances.Find(SourceId);
	if (!Source || (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment
		&& Source->OwnershipState != Edemo_mapItemOwnershipState::Equipped)
		|| (Intent.SourceArea != Edemo_mapPlayerItemArea::Equipment
			&& Source->OwnershipState != Edemo_mapItemOwnershipState::Inventory))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("世界丢弃源不再由玩家当前区域持有。"),
			SourceId);
	}
	if (EquipmentSlot == Fdemo_mapItemIds::BackpackSlot
		&& Authority.GetUsedInventorySlots()
			> Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
				+ Authority.GetRingQuickCapacity())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			TEXT("空间道具含内部物品；请使用 Bundle 确认丢弃。"),
			SourceId,
			Source->DefinitionId,
			EquipmentSlot);
	}

	FVector SafeLocation;
	if (!FindSafeDropLocation(Pawn, SafeLocation))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnsafeDropLocation,
			TEXT("没有合法的世界丢弃落点。"),
			SourceId,
			Source->DefinitionId);
	}
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	const FName DefinitionId = Source->DefinitionId;
	if (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment)
	{
		Authority.EquipmentSlots.FindChecked(EquipmentSlot).Invalidate();
		if (EquipmentSlot == Fdemo_mapItemIds::BackpackSlot)
		{
			Authority.InventorySlots.SetNum(
				Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
					+ Authority.GetRingQuickCapacity());
		}
	}
	else
	{
		Authority.InventorySlots[InventorySlot].Invalidate();
	}
	Source = Authority.Instances.Find(SourceId);
	Source->OwnershipState = Edemo_mapItemOwnershipState::World;
	Source->OwnerId = NAME_None;
	Source->ContainerId = Fdemo_mapItemIds::WorldContainer;
	Source->EquippedSlotId = NAME_None;
	const Fdemo_mapItemOperationResult Core = Authority.CommitOrRollback(
		Before,
		SourceId,
		DefinitionId,
		EquipmentSlot);
	if (!Core.bSuccess)
	{
		return Core;
	}
	if (!SynchronizeEquipmentModifiers())
	{
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::ModifierApplicationFailed,
			TEXT("装备效果同步失败，世界丢弃已回滚。"),
			SourceId,
			DefinitionId,
			EquipmentSlot);
	}
	OutActor = SpawnBoundWorldActor(Pawn->GetWorld(), SourceId, SafeLocation);
	if (!OutActor)
	{
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::WorldActorSpawnFailed,
			TEXT("世界物品投影失败，玩家物品已恢复。"),
			SourceId,
			DefinitionId,
			EquipmentSlot);
	}
	FString InvariantError;
	if (!ValidateInvariants(&InvariantError))
	{
		RemoveWorldBinding(SourceId, OutActor);
		OutActor->Destroy();
		OutActor = nullptr;
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			InvariantError,
			SourceId,
			DefinitionId,
			EquipmentSlot);
	}
	RefreshHotbarBindings();
	return Core;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::DiscardSpatialItemBundle(
	APawn* Pawn,
	Fdemo_mapSpatialDiscardBundle& OutBundle,
	TArray<Ademo_mapWorldItem*>& OutActors)
{
	OutBundle = Fdemo_mapSpatialDiscardBundle();
	OutActors.Reset();
	if (!Pawn || !Pawn->GetWorld() || RunState != Edemo_mapRunState::Active)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Spatial discard requires a live pawn in an Active Run."));
	}

	FVector BaseLocation;
	if (!FindSafeDropLocation(Pawn, BaseLocation))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnsafeDropLocation,
			TEXT("No safe location exists for the spatial discard bundle."));
	}
	BeginWorld(Pawn->GetWorld());
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	Fdemo_mapItemOperationResult Result = Authority.DiscardSpatialBundle(OutBundle);
	if (!Result.bSuccess) return Result;

	if (!SynchronizeEquipmentModifiers())
	{
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		OutBundle = Fdemo_mapSpatialDiscardBundle();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::ModifierApplicationFailed,
			TEXT("Spatial discard rolled back because equipment modifiers could not synchronize."));
	}

	const TArray<FGuid> InstanceIds = OutBundle.GetAllInstanceIds();
	TArray<FVector> SafeLocations;
	for (int32 Index = 0; Index < InstanceIds.Num(); ++Index)
	{
		const float Angle = InstanceIds.Num() > 1
			? (360.0f * static_cast<float>(Index) / static_cast<float>(InstanceIds.Num()))
			: 0.0f;
		const FVector Offset = Index == 0
			? FVector::ZeroVector
			: FVector(72.0f, 0.0f, 0.0f).RotateAngleAxis(Angle, FVector::UpVector);
		FVector SafeLocation;
		if (!ResolveSafeWorldLocation(
			Pawn->GetWorld(),
			BaseLocation + Offset,
			Pawn,
			SafeLocation,
			FName(*InstanceIds[Index].ToString(EGuidFormats::Digits)),
			&SafeLocations))
		{
			Authority.RestoreState(Before);
			SynchronizeEquipmentModifiers();
			OutBundle = Fdemo_mapSpatialDiscardBundle();
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::UnsafeDropLocation,
				TEXT("Spatial discard could not ground every bundle member atomically."));
		}
		SafeLocations.Add(SafeLocation);
	}

	auto Rollback = [&]()
	{
		for (int32 Index = 0; Index < OutActors.Num(); ++Index)
		{
			Ademo_mapWorldItem* Actor = OutActors[Index];
			RemoveWorldBinding(InstanceIds[Index], Actor);
			if (IsValid(Actor)) Actor->Destroy();
		}
		OutActors.Reset();
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		OutBundle = Fdemo_mapSpatialDiscardBundle();
	};

	for (int32 Index = 0; Index < InstanceIds.Num(); ++Index)
	{
		Ademo_mapWorldItem* Actor = SpawnBoundWorldActor(Pawn->GetWorld(), InstanceIds[Index], SafeLocations[Index]);
		if (!Actor)
		{
			Rollback();
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::WorldActorSpawnFailed,
				TEXT("Spatial discard actor projection failed; all ownership was restored."));
		}
		OutActors.Add(Actor);
	}

	SpatialDiscardBundles.Add(OutBundle.BundleId, OutBundle);
	for (const FGuid InstanceId : InstanceIds)
	{
		SpatialBundleByInstance.Add(InstanceId, OutBundle.BundleId);
	}
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		ClearSpatialBundleTracking();
		Rollback();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			Error);
	}
	return Result;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::RecoverSpatialItemBundle(
	FGuid BundleId,
	APlayerController* Controller)
{
	const Fdemo_mapSpatialDiscardBundle* Bundle = SpatialDiscardBundles.Find(BundleId);
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Bundle || !Pawn)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidWorldBinding,
			TEXT("Spatial recovery could not resolve the bundle or player."));
	}
	bool bAnyMemberInRange = false;
	for (const FGuid InstanceId : Bundle->GetAllInstanceIds())
	{
		const Ademo_mapWorldItem* Actor = GetWorldActor(InstanceId);
		bAnyMemberInRange |= Actor
			&& FVector::Dist(Pawn->GetActorLocation(), Actor->GetInteractionLocation())
				<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
	}
	if (!bAnyMemberInRange)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionOutOfRange,
			TEXT("The spatial discard bundle is outside interaction range."));
	}

	const Fdemo_mapSpatialDiscardBundle BundleCopy = *Bundle;
	const Fdemo_mapItemAuthorityState Before = Authority.CaptureState();
	Fdemo_mapItemOperationResult Result = Authority.RecoverSpatialBundle(BundleCopy);
	if (!Result.bSuccess) return Result;
	if (!SynchronizeEquipmentModifiers())
	{
		Authority.RestoreState(Before);
		SynchronizeEquipmentModifiers();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::ModifierApplicationFailed,
			TEXT("Spatial recovery rolled back because equipment modifiers could not synchronize."));
	}

	SpatialDiscardBundles.Remove(BundleId);
	for (const FGuid InstanceId : BundleCopy.GetAllInstanceIds())
	{
		SpatialBundleByInstance.Remove(InstanceId);
		TWeakObjectPtr<Ademo_mapWorldItem> Actor = WorldActors.FindRef(InstanceId);
		RemoveWorldBinding(InstanceId);
		if (Actor.IsValid()) Actor->Destroy();
	}
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			Error,
			BundleCopy.SpatialItemInstanceId);
	}
	return Result;
}

FGuid Udemo_mapItemSubsystem::FindSpatialBundleId(FGuid InstanceId) const
{
	return SpatialBundleByInstance.FindRef(InstanceId);
}

int32 Udemo_mapItemSubsystem::GetSpatialBundleMemberCount(FGuid InstanceId) const
{
	const FGuid BundleId = FindSpatialBundleId(InstanceId);
	const Fdemo_mapSpatialDiscardBundle* Bundle = SpatialDiscardBundles.Find(BundleId);
	return Bundle ? Bundle->GetAllInstanceIds().Num() : 0;
}

bool Udemo_mapItemSubsystem::FindSafeDropLocation(const APawn* Pawn, FVector& OutLocation) const
{
	if (Pawn == nullptr || Pawn->GetWorld() == nullptr) return false;
	FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
	const TArray<float> Radii = { 165.0f, 220.0f, 280.0f };
	const TArray<float> Angles = { 0.0f, 45.0f, -45.0f, 90.0f, -90.0f, 180.0f };
	for (float Radius : Radii)
	{
		for (float Angle : Angles)
		{
			const FVector Direction = Forward.RotateAngleAxis(Angle, FVector::UpVector);
			if (ResolveSafeWorldLocation(Pawn->GetWorld(), Pawn->GetActorLocation() + Direction * Radius, Pawn, OutLocation) && FVector::Dist2D(Pawn->GetActorLocation(), OutLocation) >= 130.0f) return true;
		}
	}
	return false;
}

bool Udemo_mapItemSubsystem::ResolveSafeWorldLocation(
	UWorld* World,
	const FVector& DesiredLocation,
	const AActor* IgnoredActor,
	FVector& OutLocation,
	FName StableSeed,
	const TArray<FVector>* ReservedLocations,
	float CandidateFootprintRadius) const
{
	if (World == nullptr) return false;
	const uint32 Hash = StablePlacementHash(StableSeed, DesiredLocation);
	const float PhaseDegrees = static_cast<float>(Hash % 360u);
	TArray<FVector> DesiredCandidates;
	DesiredCandidates.Reserve(25);
	DesiredCandidates.Add(DesiredLocation);
	for (const float Radius : { 82.0f, 128.0f, 178.0f })
	{
		for (int32 Step = 0; Step < 8; ++Step)
		{
			const FVector Offset = FVector(Radius, 0.0f, 0.0f)
				.RotateAngleAxis(PhaseDegrees + (45.0f * Step), FVector::UpVector);
			DesiredCandidates.Add(DesiredLocation + Offset);
		}
	}
	for (const FVector& DesiredCandidate : DesiredCandidates)
	{
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(V3WorldGround), false, IgnoredActor);
		if (IgnoredActor) TraceParams.AddIgnoredActor(IgnoredActor);
		FHitResult GroundHit;
		if (!World->LineTraceSingleByChannel(GroundHit, DesiredCandidate + FVector(0, 0, 500), DesiredCandidate - FVector(0, 0, 900), ECC_Visibility, TraceParams) || !GroundHit.bBlockingHit || GroundHit.ImpactNormal.Z < 0.65f) continue;
		const FVector Candidate = GroundHit.ImpactPoint + FVector(0, 0, 36.0f);
		if (!IsWorldPlacementClear(
			World,
			Candidate,
			IgnoredActor,
			ReservedLocations,
			CandidateFootprintRadius)) continue;
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
		TArray<FOverlapResult> Overlaps;
		bool bBlocked = false;
		if (World->OverlapMultiByObjectType(Overlaps, Candidate, FQuat::Identity, Objects, FCollisionShape::MakeSphere(27.0f), TraceParams))
		{
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (Overlap.GetActor() == nullptr || Overlap.GetActor() == GroundHit.GetActor()) continue;
				const UPrimitiveComponent* Component = Overlap.Component.Get();
				if (Component && Component->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block)
				{
					bBlocked = true;
					break;
				}
			}
		}
		if (bBlocked) continue;
		OutLocation = Candidate;
		return true;
	}
	return false;
}

void Udemo_mapItemSubsystem::NotifyWorldActorEndPlay(FGuid InstanceId, Ademo_mapWorldItem* Actor)
{
	if (!IsWorldActorBound(InstanceId, Actor)) return;
	RemoveWorldBinding(InstanceId, Actor);
	if (const Fdemo_mapItemInstance* Instance = Authority.FindInstance(InstanceId); Instance != nullptr && Instance->OwnershipState == Edemo_mapItemOwnershipState::World) Authority.DestroyWorld(InstanceId);
}

void Udemo_mapItemSubsystem::RemoveWorldBinding(FGuid InstanceId, const Ademo_mapWorldItem* ExpectedActor)
{
	if (const TWeakObjectPtr<Ademo_mapWorldItem>* Bound = WorldActors.Find(InstanceId); Bound != nullptr && (ExpectedActor == nullptr || Bound->Get() == ExpectedActor)) WorldActors.Remove(InstanceId);
}

void Udemo_mapItemSubsystem::ClearSpatialBundleTracking()
{
	SpatialDiscardBundles.Reset();
	SpatialBundleByInstance.Reset();
}

bool Udemo_mapItemSubsystem::IsWorldActorBound(FGuid InstanceId, const Ademo_mapWorldItem* Actor) const
{
	const TWeakObjectPtr<Ademo_mapWorldItem>* Bound = WorldActors.Find(InstanceId);
	return Bound != nullptr && Bound->IsValid() && Bound->Get() == Actor;
}

Ademo_mapWorldItem* Udemo_mapItemSubsystem::GetWorldActor(FGuid InstanceId) const { return WorldActors.FindRef(InstanceId).Get(); }
int32 Udemo_mapItemSubsystem::GetWorldActorCount() const { return WorldActors.Num(); }

bool Udemo_mapItemSubsystem::ValidateWorldBindings(FString* OutError) const
{
	auto Fail = [OutError](const FString& Message) { if (OutError) *OutError = Message; return false; };
	const TArray<FGuid> WorldIds = Authority.FindWorldInstances();
	if (WorldIds.Num() != WorldActors.Num()) return Fail(TEXT("World instance and actor binding counts differ."));
	TSet<const Ademo_mapWorldItem*> UniqueActors;
	for (const FGuid& InstanceId : WorldIds)
	{
		Ademo_mapWorldItem* Actor = GetWorldActor(InstanceId);
		if (!IsValid(Actor) || Actor->GetInstanceId() != InstanceId || Actor->GetWorld() != ActiveWorld.Get() || UniqueActors.Contains(Actor)) return Fail(TEXT("World actor binding is missing, stale, duplicated, or cross-world."));
		UniqueActors.Add(Actor);
	}
	return true;
}

Fdemo_mapItemOperationResult Udemo_mapItemSubsystem::FinalizeTransaction(const Fdemo_mapItemAuthorityState& Before, const Fdemo_mapItemOperationResult& CoreResult)
{
	if (!CoreResult.bSuccess) return CoreResult;
	Udemo_mapPlayerHealthComponent* Health =
		BoundHealthComponent.Get();
	const float VitalityBefore = Health
		? Health->GetCurrentVitality()
		: 0.0f;
	if (SynchronizeEquipmentModifiers()) return CoreResult;
	Authority.RestoreState(Before);
	if (!SynchronizeEquipmentModifiers())
	{
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InternalRollbackFailed, TEXT("Modifier synchronization and rollback both failed."), CoreResult.RelatedInstanceId, CoreResult.RelatedDefinitionId, CoreResult.RelatedSlotId);
	}
	if (Health)
	{
		Health->RestoreCurrentVitalityAfterItemUseRollback(
			VitalityBefore);
	}
	return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::ModifierApplicationFailed, TEXT("Equipment transaction rolled back because its modifiers could not be applied."), CoreResult.RelatedInstanceId, CoreResult.RelatedDefinitionId, CoreResult.RelatedSlotId);
}

bool Udemo_mapItemSubsystem::BindPlayerPawn(APawn* Pawn)
{
	BoundPlayerPawn = Pawn;
	const bool bAttributesBound = BindAttributeComponent(
		Pawn
			? Pawn->FindComponentByClass<Udemo_mapAttributeComponent>()
			: nullptr);
	const bool bHealthBound = BindHealthComponent(
		Pawn
			? Pawn->FindComponentByClass<
				Udemo_mapPlayerHealthComponent>()
			: nullptr);
	return bAttributesBound && bHealthBound;
}

bool Udemo_mapItemSubsystem::BindAttributeComponent(Udemo_mapAttributeComponent* AttributeComponent)
{
	if (BoundAttributeComponent.Get() == AttributeComponent) return SynchronizeEquipmentModifiers();
	RemoveAllEquipmentSourcesFromBoundComponent();
	BoundAttributeComponent = AttributeComponent;
	ActiveModifierSources.Reset();
	return SynchronizeEquipmentModifiers();
}

bool Udemo_mapItemSubsystem::BindHealthComponent(
	Udemo_mapPlayerHealthComponent* HealthComponent)
{
	BoundHealthComponent = HealthComponent;
	return HealthComponent != nullptr;
}

void Udemo_mapItemSubsystem::RemoveAllEquipmentSourcesFromBoundComponent()
{
	if (Udemo_mapAttributeComponent* Attributes = BoundAttributeComponent.Get())
	{
		for (FName SourceId : ActiveModifierSources) Attributes->RemoveModifiersBySource(SourceId);
	}
	ActiveModifierSources.Reset();
}

bool Udemo_mapItemSubsystem::BuildDesiredModifierSources(
	TMap<FName, TArray<Fdemo_mapModifierSpec>>& OutDesired) const
{
	OutDesired.Reset();
	for (const TPair<FName, FGuid>& Slot : Authority.GetEquipmentSlotSnapshot())
	{
		if (!Slot.Value.IsValid()) continue;
		const Fdemo_mapItemInstance* Instance = Authority.FindInstance(Slot.Value);
		const Fdemo_mapItemDefinition* Definition = Instance ? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId) : nullptr;
		if (Instance == nullptr || Definition == nullptr)
		{
			return false;
		}
		const Fdemo_mapEquipmentEffectResolution Resolution =
			Fdemo_mapEquipmentEffectResolver::Resolve(*Definition, Slot.Key);
		if (!Resolution.bSuccess)
		{
			return false;
		}
		TArray<Fdemo_mapModifierSpec> Combined = Resolution.Modifiers;
		const Fdemo_mapEquipmentEffectResolution AffixResolution =
			Fdemo_mapEquipmentEffectResolver::ResolveAffixes(
				*Definition,
				Instance->AffixSet);
		if (!AffixResolution.bSuccess)
		{
			return false;
		}
		Combined.Append(AffixResolution.Modifiers);
		if (!Combined.IsEmpty())
		{
			OutDesired.Add(
				MakeModifierSourceId(Instance->InstanceId),
				MoveTemp(Combined));
		}
	}
	return true;
}

bool Udemo_mapItemSubsystem::SynchronizeEquipmentModifiers()
{
	Udemo_mapAttributeComponent* Attributes = BoundAttributeComponent.Get();
	if (Attributes == nullptr)
	{
		ActiveModifierSources.Reset();
		return true;
	}
	Udemo_mapPlayerHealthComponent* Health =
		BoundHealthComponent.Get();
	const float CurrentVitalityBefore = Health
		? Health->GetCurrentVitality()
		: 0.0f;
	TMap<FName, TArray<Fdemo_mapModifierSpec>> Desired;
	if (!BuildDesiredModifierSources(Desired)) return false;
	for (auto It = ActiveModifierSources.CreateIterator(); It; ++It)
	{
		if (!Desired.Contains(*It))
		{
			Attributes->RemoveModifiersBySource(*It);
			It.RemoveCurrent();
		}
	}
	for (const TPair<FName, TArray<Fdemo_mapModifierSpec>>& Pair : Desired)
	{
		const int32 ExpectedCount = Pair.Value.Num();
		if (ActiveModifierSources.Contains(Pair.Key) && Attributes->GetModifierCountBySource(Pair.Key) == ExpectedCount) continue;
		Attributes->RemoveModifiersBySource(Pair.Key);
		ActiveModifierSources.Remove(Pair.Key);
		bool bApplied = true;
		for (const Fdemo_mapModifierSpec& DefinitionModifier : Pair.Value)
		{
			Fdemo_mapModifierSpec RuntimeModifier = DefinitionModifier;
			RuntimeModifier.SourceId = Pair.Key;
			Fdemo_mapModifierHandle Handle;
			if (!Attributes->AddModifier(RuntimeModifier, Handle))
			{
				bApplied = false;
				break;
			}
		}
		if (!bApplied)
		{
			Attributes->RemoveModifiersBySource(Pair.Key);
			return false;
		}
		ActiveModifierSources.Add(Pair.Key);
	}
	if (Health)
	{
		Health->RestoreCurrentVitalityAfterItemUseRollback(
			CurrentVitalityBefore);
	}
	return true;
}

bool Udemo_mapItemSubsystem::ValidateInvariants(FString* OutError) const
{
	if (!Authority.ValidateInvariants(OutError)) return false;
	if (!ValidateWorldBindings(OutError)) return false;
	if (HotbarBindings.SlotBindings.Num() != Fdemo_mapHotbarBindingSnapshot::SlotCount)
	{
		if (OutError) *OutError = TEXT("Hotbar must contain exactly nine stable value slots.");
		return false;
	}
	TSet<FGuid> HotbarIds;
	for (const FGuid& BoundId : HotbarBindings.SlotBindings)
	{
		if (!BoundId.IsValid())
		{
			continue;
		}
		const Fdemo_mapItemInstance* Instance = Authority.FindInstance(BoundId);
		const Fdemo_mapItemDefinition* Definition = Instance
			? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId)
			: nullptr;
		if (HotbarIds.Contains(BoundId)
			|| !Instance
			|| !Definition
			|| !Fdemo_mapItemViewRules::IsHotbarBindable(*Instance, *Definition)
			|| Authority.FindInventorySlot(BoundId) < 0
			|| Authority.FindInventorySlot(BoundId)
				>= Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount
					+ Authority.GetRingQuickCapacity())
		{
			if (OutError) *OutError = TEXT("Hotbar contains a duplicate, stale, or incompatible binding.");
			return false;
		}
		HotbarIds.Add(BoundId);
	}
	const Udemo_mapAttributeComponent* Attributes = BoundAttributeComponent.Get();
	if (Attributes == nullptr) return ActiveModifierSources.IsEmpty();
	TMap<FName, TArray<Fdemo_mapModifierSpec>> Desired;
	if (!BuildDesiredModifierSources(Desired))
	{
		if (OutError) *OutError = TEXT("Equipped Item Effect metadata cannot be resolved.");
		return false;
	}
	if (Desired.Num() != ActiveModifierSources.Num())
	{
		if (OutError) *OutError = TEXT("Equipment modifier source count does not match equipped items.");
		return false;
	}
	for (const TPair<FName, TArray<Fdemo_mapModifierSpec>>& Pair : Desired)
	{
		if (!ActiveModifierSources.Contains(Pair.Key) || Attributes->GetModifierCountBySource(Pair.Key) != Pair.Value.Num())
		{
			if (OutError) *OutError = FString::Printf(TEXT("Equipment modifier source is missing or duplicated: %s"), *Pair.Key.ToString());
			return false;
		}
	}
	return true;
}

#if !UE_BUILD_SHIPPING
void Udemo_mapItemSubsystem::ResetForAutomation()
{
	if (ActiveWorld.IsValid()) TeardownWorld(ActiveWorld.Get());
	RemoveAllEquipmentSourcesFromBoundComponent();
	Authority.Reset();
	ClearHotbarBindings();
	ClearItemUseCooldown();
	BoundHealthComponent.Reset();
	BoundPlayerPawn.Reset();
	RunState = Edemo_mapRunState::Inactive;
	ActiveRunId.Invalidate();
	DeployedItemIds.Reset();
	LastSettlementSummary = Fdemo_mapSettlementSummary();
	LootSourceStates.Reset();
#if WITH_DEV_AUTOMATION_TESTS
	PreparedRunFailureAfterMutation = INDEX_NONE;
	ItemUseAutomationTimeOffset = 0.0;
#endif
}
#endif
