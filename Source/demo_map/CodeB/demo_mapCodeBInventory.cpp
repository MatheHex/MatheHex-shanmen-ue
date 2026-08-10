#include "CodeB/demo_mapCodeBInventory.h"

#include "demo_mapItemDefinitions.h"

namespace demo_map_code_b
{
	namespace
	{
		void SetError(FString* OutError, const TCHAR* Message)
		{
			if (OutError)
			{
				*OutError = Message;
			}
		}

		bool IsSlotInRange(const FCodeBContainer& Container, int32 Slot)
		{
			return Container.Slots.IsValidIndex(Slot);
		}

		FGuid GetItemAt(const FCodeBRepository::FState& State, const FGuid& ContainerId, int32 Slot)
		{
			const FCodeBContainer* Container = State.Containers.Find(ContainerId);
			return Container && Container->Slots.IsValidIndex(Slot) ? Container->Slots[Slot] : FGuid();
		}

		bool IsEquipSlotCompatible(const FCodeBItemDefinition& Definition, const FCodeBContainer& Container)
		{
			return Container.IsEquipment() && Definition.EquipSlot != ECodeBEquipSlot::None && Definition.EquipSlot == Container.EquipmentSlot;
		}

		// P21 has read-only corpse equipment source cells.  They are still formal
		// P1 equipment containers for invariant purposes, but their one-way P12
		// extraction is modelled as a Move and is later narrowed by the durable
		// body-transfer commit to one explicit P38 P6 target.
		bool IsP21BodyEquipmentContainer(const FCodeBContainer& Container)
		{
			return Container.IsEquipment()
				&& Container.ContainerType.ToString().StartsWith(TEXT("CodeB.Body."));
		}

		bool IsDefinitionValidForItem(const FCodeBItemDefinition& Definition, int32 Quantity)
		{
			return Definition.GridWidth == 1
				&& Definition.GridHeight == 1
				&& Definition.MaxStack >= 1
				&& Quantity >= 1
				&& Quantity <= Definition.MaxStack
				&& (Definition.bStackable || Quantity == 1)
				&& (Definition.bStackable || Definition.MaxStack == 1)
				&& ((Definition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None
					&& Definition.ChildContainerCapacity == 0)
					|| (Definition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
						&& Definition.ItemType == ECodeBItemType::SpatialItem
						&& Definition.EquipSlot == ECodeBEquipSlot::SpatialItem
						&& Definition.ChildContainerCapacity > 0)
					|| (Definition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::StoragePouch
						&& Definition.ItemType == ECodeBItemType::Backpack
						&& Definition.EquipSlot == ECodeBEquipSlot::Backpack
						&& Definition.ChildContainerCapacity > 0));
		}

		bool ContainsGuid(const TArray<FGuid>& Values, const FGuid& Value)
		{
			return Values.Contains(Value);
		}
	}

	bool FCodeBItemDefinition::operator==(const FCodeBItemDefinition& Other) const
	{
	return DefinitionId == Other.DefinitionId
		&& ItemType == Other.ItemType
		&& bStackable == Other.bStackable
		&& bQuickUsable == Other.bQuickUsable
		&& QuickUseEffect == Other.QuickUseEffect
		&& QuickUseRestoreAmount == Other.QuickUseRestoreAmount
		&& MaxStack == Other.MaxStack
			&& EquipSlot == Other.EquipSlot
			&& GridWidth == Other.GridWidth
			&& GridHeight == Other.GridHeight
			&& SpatialContainerSemantic == Other.SpatialContainerSemantic
			&& ChildContainerCapacity == Other.ChildContainerCapacity;
	}

	bool FCodeBItemInstance::operator==(const FCodeBItemInstance& Other) const
	{
		return ItemId == Other.ItemId
			&& DefinitionId == Other.DefinitionId
			&& Quantity == Other.Quantity
			&& Level == Other.Level
			&& Quality == Other.Quality
			&& RandomSeed == Other.RandomSeed
			&& LegacyAffixDigest == Other.LegacyAffixDigest
			&& ParentContainerId == Other.ParentContainerId
			&& SlotIndex == Other.SlotIndex
			&& ChildContainerId == Other.ChildContainerId;
	}

	bool FCodeBContainer::operator==(const FCodeBContainer& Other) const
	{
		return ContainerId == Other.ContainerId
			&& ContainerType == Other.ContainerType
			&& Kind == Other.Kind
			&& EquipmentSlot == Other.EquipmentSlot
			&& Slots == Other.Slots;
	}

	bool FCodeBSnapshot::operator==(const FCodeBSnapshot& Other) const
	{
		if (Revision != Other.Revision
			|| Definitions.Num() != Other.Definitions.Num()
			|| Items.Num() != Other.Items.Num()
			|| Containers.Num() != Other.Containers.Num())
		{
			return false;
		}

		for (const TPair<FName, FCodeBItemDefinition>& Pair : Definitions)
		{
			const FCodeBItemDefinition* OtherValue = Other.Definitions.Find(Pair.Key);
			if (!OtherValue || !(*OtherValue == Pair.Value))
			{
				return false;
			}
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Items)
		{
			const FCodeBItemInstance* OtherValue = Other.Items.Find(Pair.Key);
			if (!OtherValue || !(*OtherValue == Pair.Value))
			{
				return false;
			}
		}
		for (const TPair<FGuid, FCodeBContainer>& Pair : Containers)
		{
			const FCodeBContainer* OtherValue = Other.Containers.Find(Pair.Key);
			if (!OtherValue || !(*OtherValue == Pair.Value))
			{
				return false;
			}
		}
		return true;
	}

	bool FCodeBRepository::RegisterDefinition(const FCodeBItemDefinition& Definition, FString* OutError)
	{
		if (!Definition.DefinitionId.IsValid())
		{
			SetError(OutError, TEXT("Definition ID is invalid."));
			return false;
		}
		if (Definition.GridWidth != 1 || Definition.GridHeight != 1)
		{
			SetError(OutError, TEXT("Code B P1 only accepts 1x1 definitions."));
			return false;
		}
		if (Definition.MaxStack < 1 || (!Definition.bStackable && Definition.MaxStack != 1))
		{
			SetError(OutError, TEXT("Definition stack rules are invalid."));
			return false;
		}
		if (!IsDefinitionValidForItem(Definition, 1))
		{
			SetError(OutError, TEXT("Definition spatial-container provenance is invalid."));
			return false;
		}
		if ((Definition.QuickUseEffect == ECodeBQuickUseEffectKind::None && Definition.QuickUseRestoreAmount != 0)
			|| (Definition.QuickUseEffect == ECodeBQuickUseEffectKind::RestoreHealth
				&& (!Definition.bQuickUsable || Definition.QuickUseRestoreAmount <= 0))
			|| (Definition.QuickUseEffect != ECodeBQuickUseEffectKind::None
				&& Definition.QuickUseEffect != ECodeBQuickUseEffectKind::RestoreHealth))
		{
			SetError(OutError, TEXT("Definition quick-use descriptor is invalid."));
			return false;
		}
		if (State.Definitions.Contains(Definition.DefinitionId))
		{
			SetError(OutError, TEXT("Duplicate Definition ID."));
			return false;
		}
		State.Definitions.Add(Definition.DefinitionId, Definition);
		++State.Revision;
		return true;
	}

	FGuid FCodeBRepository::CreateContainer(
		FName ContainerType,
		int32 Capacity,
		ECodeBContainerKind Kind,
		ECodeBEquipSlot EquipmentSlot,
		FString* OutError,
		const FGuid& ForcedContainerId)
	{
		if (!ContainerType.IsValid() || Capacity <= 0)
		{
			SetError(OutError, TEXT("Container type or capacity is invalid."));
			return FGuid();
		}
		if (Kind == ECodeBContainerKind::Equipment && (Capacity != 1 || EquipmentSlot == ECodeBEquipSlot::None))
		{
			SetError(OutError, TEXT("Equipment containers must have one slot and a concrete equipment slot."));
			return FGuid();
		}
		if (Kind == ECodeBContainerKind::Storage && EquipmentSlot != ECodeBEquipSlot::None)
		{
			SetError(OutError, TEXT("Storage containers cannot declare an equipment slot."));
			return FGuid();
		}

		FCodeBContainer Container;
		Container.ContainerId = ForcedContainerId.IsValid() ? ForcedContainerId : FGuid::NewGuid();
		if (!Container.ContainerId.IsValid() || State.Containers.Contains(Container.ContainerId))
		{
			SetError(OutError, TEXT("Duplicate or invalid Container ID."));
			return FGuid();
		}
		Container.ContainerType = ContainerType;
		Container.Kind = Kind;
		Container.EquipmentSlot = EquipmentSlot;
		Container.Slots.Init(FGuid(), Capacity);
		State.Containers.Add(Container.ContainerId, Container);
		++State.Revision;
		return Container.ContainerId;
	}

	FGuid FCodeBRepository::CreateItem(
		FName DefinitionId,
		int32 Quantity,
		const FGuid& ParentContainerId,
		int32 TargetSlot,
		const FGuid& ForcedItemId,
		FString* OutError)
	{
		const FCodeBItemDefinition* Definition = State.Definitions.Find(DefinitionId);
		if (!Definition || !IsDefinitionValidForItem(*Definition, Quantity))
		{
			SetError(OutError, TEXT("Definition or quantity is invalid."));
			return FGuid();
		}

		const FGuid ItemId = ForcedItemId.IsValid() ? ForcedItemId : FGuid::NewGuid();
		if (!ItemId.IsValid() || State.Items.Contains(ItemId))
		{
			SetError(OutError, TEXT("Duplicate or invalid Item ID."));
			return FGuid();
		}

		FCodeBContainer* Container = ParentContainerId.IsValid() ? State.Containers.Find(ParentContainerId) : nullptr;
		if (ParentContainerId.IsValid() && !Container)
		{
			SetError(OutError, TEXT("Parent container not found."));
			return FGuid();
		}
		if (Container)
		{
			if (TargetSlot == INDEX_NONE)
			{
				TargetSlot = Container->Slots.IndexOfByPredicate([](const FGuid& Value) { return !Value.IsValid(); });
			}
			if (!IsSlotInRange(*Container, TargetSlot) || Container->Slots[TargetSlot].IsValid())
			{
				SetError(OutError, TEXT("Parent container slot is invalid or occupied."));
				return FGuid();
			}
			if (Container->IsEquipment() && !IsEquipSlotCompatible(*Definition, *Container))
			{
				SetError(OutError, TEXT("Item does not match equipment container."));
				return FGuid();
			}
		}

		FCodeBItemInstance Item;
		Item.ItemId = ItemId;
		Item.DefinitionId = DefinitionId;
		Item.Quantity = Quantity;
		Item.ParentContainerId = ParentContainerId;
		Item.SlotIndex = Container ? TargetSlot : INDEX_NONE;
		State.Items.Add(ItemId, Item);
		if (Container)
		{
			Container->Slots[TargetSlot] = ItemId;
		}
		++State.Revision;
		return ItemId;
	}

	bool FCodeBRepository::AssociateChildContainer(const FGuid& SpatialItemId, const FGuid& ChildContainerId, FString* OutError)
	{
		FCodeBItemInstance* Item = State.Items.Find(SpatialItemId);
		const FCodeBContainer* Child = State.Containers.Find(ChildContainerId);
		const FCodeBItemDefinition* Definition = Item ? State.Definitions.Find(Item->DefinitionId) : nullptr;
		if (!Item || !Definition
			|| (Definition->ItemType != ECodeBItemType::SpatialItem && Definition->ItemType != ECodeBItemType::Backpack))
		{
			SetError(OutError, TEXT("Only a valid spatial item can own an internal container."));
			return false;
		}
		if (!Child || Child->IsEquipment() || Item->ParentContainerId == ChildContainerId || Child->Slots.Contains(SpatialItemId))
		{
			SetError(OutError, TEXT("Spatial item child container is invalid."));
			return false;
		}
		if (Definition->SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
			&& Child->Slots.Num() != Definition->ChildContainerCapacity)
		{
			SetError(OutError, TEXT("Spatial item child container capacity does not match its product definition."));
			return false;
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : State.Items)
		{
			if (Pair.Key != SpatialItemId && Pair.Value.ChildContainerId == ChildContainerId)
			{
				SetError(OutError, TEXT("Internal container is already owned by another item."));
				return false;
			}
		}

		const FGuid PreviousChildContainerId = Item->ChildContainerId;
		Item->ChildContainerId = ChildContainerId;
		FString InvariantError;
		if (!ValidateState(State, &InvariantError))
		{
			Item->ChildContainerId = PreviousChildContainerId;
			SetError(OutError, *InvariantError);
			return false;
		}
		++State.Revision;
		return true;
	}

	const FCodeBItemDefinition* FCodeBRepository::FindDefinition(FName DefinitionId) const
	{
		return State.Definitions.Find(DefinitionId);
	}

	const FCodeBItemInstance* FCodeBRepository::FindItem(const FGuid& ItemId) const
	{
		return State.Items.Find(ItemId);
	}

	const FCodeBContainer* FCodeBRepository::FindContainer(const FGuid& ContainerId) const
	{
		return State.Containers.Find(ContainerId);
	}

	FCodeBSnapshot FCodeBRepository::CaptureSnapshot() const
	{
		FCodeBSnapshot Snapshot;
		Snapshot.Revision = State.Revision;
		Snapshot.Definitions = State.Definitions;
		Snapshot.Items = State.Items;
		Snapshot.Containers = State.Containers;
		return Snapshot;
	}

	bool FCodeBRepository::LoadPersistedSnapshot(const FCodeBSnapshot& Snapshot, FString* OutError)
	{
		if (Snapshot.Revision < 0)
		{
			SetError(OutError, TEXT("Persisted Code B revision is invalid."));
			return false;
		}

		FState Candidate;
		Candidate.Revision = Snapshot.Revision;
		Candidate.Definitions = Snapshot.Definitions;
		Candidate.Items = Snapshot.Items;
		Candidate.Containers = Snapshot.Containers;
		FString ValidationError;
		if (!ValidateState(Candidate, &ValidationError))
		{
			SetError(OutError, *ValidationError);
			return false;
		}

		State = MoveTemp(Candidate);
		return true;
	}

	bool FCodeBRepository::ValidateState(const FState& Candidate, FString* OutError)
	{
		TSet<FGuid> SeenPlacedItems;
		TSet<FGuid> OwnedChildContainers;
		for (const TPair<FName, FCodeBItemDefinition>& DefinitionPair : Candidate.Definitions)
		{
			const FCodeBItemDefinition& Definition = DefinitionPair.Value;
			if (DefinitionPair.Key != Definition.DefinitionId
				|| !Definition.DefinitionId.IsValid()
				|| Definition.GridWidth != 1
				|| Definition.GridHeight != 1
				|| !IsDefinitionValidForItem(Definition, 1))
			{
				SetError(OutError, TEXT("Definition invariant failed."));
				return false;
			}
		}

		for (const TPair<FGuid, FCodeBContainer>& ContainerPair : Candidate.Containers)
		{
			const FCodeBContainer& Container = ContainerPair.Value;
			if (ContainerPair.Key != Container.ContainerId
				|| !Container.ContainerId.IsValid()
				|| !Container.ContainerType.IsValid()
				|| Container.Slots.Num() <= 0
				|| (Container.IsEquipment() && (Container.Slots.Num() != 1 || Container.EquipmentSlot == ECodeBEquipSlot::None))
				|| (!Container.IsEquipment() && Container.EquipmentSlot != ECodeBEquipSlot::None))
			{
				SetError(OutError, TEXT("Container invariant failed."));
				return false;
			}

			for (int32 SlotIndex = 0; SlotIndex < Container.Slots.Num(); ++SlotIndex)
			{
				const FGuid ItemId = Container.Slots[SlotIndex];
				if (!ItemId.IsValid())
				{
					continue;
				}
				if (SeenPlacedItems.Contains(ItemId))
				{
					SetError(OutError, TEXT("Duplicate item placement detected."));
					return false;
				}
				SeenPlacedItems.Add(ItemId);
				const FCodeBItemInstance* Item = Candidate.Items.Find(ItemId);
				if (!Item
					|| Item->ItemId != ItemId
					|| Item->ParentContainerId != Container.ContainerId
					|| Item->SlotIndex != SlotIndex)
				{
					SetError(OutError, TEXT("Container-to-item placement index is inconsistent."));
					return false;
				}
			}
		}

		for (const TPair<FGuid, FCodeBItemInstance>& ItemPair : Candidate.Items)
		{
			const FCodeBItemInstance& Item = ItemPair.Value;
			const FCodeBItemDefinition* Definition = Candidate.Definitions.Find(Item.DefinitionId);
			if (ItemPair.Key != Item.ItemId
				|| !Item.ItemId.IsValid()
				|| !Definition
				|| !IsDefinitionValidForItem(*Definition, Item.Quantity))
			{
				SetError(OutError, TEXT("Item identity, definition, or quantity invariant failed."));
				return false;
			}

			if (Item.ChildContainerId.IsValid())
			{
				if ((Definition->ItemType != ECodeBItemType::SpatialItem && Definition->ItemType != ECodeBItemType::Backpack)
					|| Item.ChildContainerId == Item.ParentContainerId
					|| OwnedChildContainers.Contains(Item.ChildContainerId))
				{
					SetError(OutError, TEXT("Spatial item child-container ownership invariant failed."));
					return false;
				}
				const FCodeBContainer* ChildContainer = Candidate.Containers.Find(Item.ChildContainerId);
				if (!ChildContainer || ChildContainer->IsEquipment() || ChildContainer->Slots.Contains(Item.ItemId)
					|| (Definition->SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
						&& ChildContainer->Slots.Num() != Definition->ChildContainerCapacity))
				{
					SetError(OutError, TEXT("Spatial item child-container relationship is invalid."));
					return false;
				}
				for (const FGuid& ChildItemId : ChildContainer->Slots)
				{
					if (!ChildItemId.IsValid()) continue;
					const FCodeBItemInstance* ChildItem = Candidate.Items.Find(ChildItemId);
					const FCodeBItemDefinition* ChildDefinition = ChildItem
						? Candidate.Definitions.Find(ChildItem->DefinitionId)
						: nullptr;
					if (!ChildItem || !ChildDefinition
						|| ChildItem->ChildContainerId.IsValid()
						|| ChildDefinition->ItemType == ECodeBItemType::SpatialItem
						|| ChildDefinition->ItemType == ECodeBItemType::Backpack)
					{
						SetError(OutError, TEXT("Spatial item child containers permit only one storage level."));
						return false;
					}
				}
				OwnedChildContainers.Add(Item.ChildContainerId);
			}

			if (Item.IsPlaced())
			{
				const FCodeBContainer* Container = Candidate.Containers.Find(Item.ParentContainerId);
				if (!Container || !IsSlotInRange(*Container, Item.SlotIndex) || Container->Slots[Item.SlotIndex] != Item.ItemId)
				{
					SetError(OutError, TEXT("Item-to-container placement index is inconsistent."));
					return false;
				}
				if (Container->IsEquipment() && !IsEquipSlotCompatible(*Definition, *Container))
				{
					SetError(OutError, TEXT("Equipped item does not match its slot."));
					return false;
				}
			}
		}

		return true;
	}

	bool FCodeBRepository::ValidateInvariants(FString* OutError) const
	{
		return ValidateState(State, OutError);
	}

	FCodeBTransactionResult FCodeBRepository::MakeFailure(
		const FCodeBTransactionRequest& Request,
		int32 CurrentRevision,
		ECodeBResultCode Code,
		const TCHAR* Message)
	{
		FCodeBTransactionResult Result;
		Result.TransactionId = Request.TransactionId.IsValid() ? Request.TransactionId : FGuid::NewGuid();
		Result.Code = Code;
		Result.Message = Message;
		Result.NewRevision = CurrentRevision;
		return Result;
	}

	void FCodeBRepository::AddAffected(FCodeBTransactionResult& Result, const FGuid& ItemId, const FGuid& ContainerId)
	{
		if (ItemId.IsValid())
		{
			Result.AffectedItemIds.AddUnique(ItemId);
		}
		if (ContainerId.IsValid())
		{
			Result.AffectedContainerIds.AddUnique(ContainerId);
		}
	}

	bool FCodeBRepository::IsValidStorageTarget(
		const FState& Candidate,
		const FGuid& ContainerId,
		int32 Slot,
		FCodeBTransactionResult& OutResult,
		int32* OutResolvedSlot)
	{
		const FCodeBContainer* Container = Candidate.Containers.Find(ContainerId);
		if (!Container)
		{
			OutResult.Code = ECodeBResultCode::TargetNotFound;
			OutResult.Message = TEXT("Target container was not found.");
			return false;
		}
		if (Container->IsEquipment())
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("This operation requires a storage target.");
			return false;
		}
		if (Slot == INDEX_NONE)
		{
			const int32 FirstEmptySlot = Container->Slots.IndexOfByPredicate([](const FGuid& Value) { return !Value.IsValid(); });
			if (FirstEmptySlot == INDEX_NONE)
			{
				OutResult.Code = ECodeBResultCode::TargetFull;
				OutResult.Message = TEXT("Target storage container is full.");
				return false;
			}
			if (OutResolvedSlot)
			{
				*OutResolvedSlot = FirstEmptySlot;
			}
			return true;
		}
		if (!IsSlotInRange(*Container, Slot))
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Target slot is invalid.");
			return false;
		}
		if (OutResolvedSlot)
		{
			*OutResolvedSlot = Slot;
		}
		if (Container->Slots[Slot].IsValid())
		{
			OutResult.Code = ECodeBResultCode::TargetOccupied;
			OutResult.Message = TEXT("Target slot is occupied.");
			return false;
		}
		return true;
	}

	bool FCodeBRepository::ValidateSource(
		FState& Candidate,
		const FCodeBTransactionRequest& Request,
		FCodeBItemInstance*& OutItem,
		FCodeBContainer*& OutSource,
		FCodeBTransactionResult& OutResult)
	{
		OutItem = Candidate.Items.Find(Request.ItemId);
		if (!OutItem)
		{
			OutResult.Code = ECodeBResultCode::ItemNotFound;
			OutResult.Message = TEXT("Source item was not found.");
			return false;
		}
		if (!OutItem->IsPlaced())
		{
			OutResult.Code = ECodeBResultCode::SourceMismatch;
			OutResult.Message = TEXT("Source item is not placed.");
			return false;
		}
		OutSource = Candidate.Containers.Find(OutItem->ParentContainerId);
		if (!OutSource || !IsSlotInRange(*OutSource, OutItem->SlotIndex))
		{
			OutResult.Code = ECodeBResultCode::InvariantViolation;
			OutResult.Message = TEXT("Source placement index is invalid.");
			return false;
		}
		if (Request.SourceContainerId.IsValid() && Request.SourceContainerId != OutItem->ParentContainerId)
		{
			OutResult.Code = ECodeBResultCode::SourceMismatch;
			OutResult.Message = TEXT("Source container does not match the item.");
			return false;
		}
		if (Request.SourceSlot != INDEX_NONE && Request.SourceSlot != OutItem->SlotIndex)
		{
			OutResult.Code = ECodeBResultCode::SourceMismatch;
			OutResult.Message = TEXT("Source slot does not match the item.");
			return false;
		}
		if ((*OutSource).Slots[OutItem->SlotIndex] != OutItem->ItemId)
		{
			OutResult.Code = ECodeBResultCode::InvariantViolation;
			OutResult.Message = TEXT("Source container index does not match the item.");
			return false;
		}
		return true;
	}

	bool FCodeBRepository::ExecuteMove(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult)
	{
		FCodeBItemInstance* Item = nullptr;
		FCodeBContainer* Source = nullptr;
		if (!ValidateSource(Candidate, Request, Item, Source, OutResult))
		{
			return false;
		}
		const bool bP21BodyEquipmentSource = IsP21BodyEquipmentContainer(*Source);
		if (Source->IsEquipment() && !bP21BodyEquipmentSource)
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Use Unequip for equipment items.");
			return false;
		}
		FCodeBContainer* Target = Candidate.Containers.Find(Request.TargetContainerId);
		int32 TargetSlot = Request.TargetSlot;
		const bool bP42BodySpatialSource = Source->ContainerType == FName(TEXT("CodeB.BodyContainer.BasicCorpse"));
		if (bP42BodySpatialSource && Target && Target->IsEquipment())
		{
			const FCodeBItemDefinition* Definition = Candidate.Definitions.Find(Item->DefinitionId);
			const bool bWindTalisman = Item->DefinitionId == Fdemo_mapItemIds::WindTalisman;
			const bool bBackpackLevel1 = Item->DefinitionId == Fdemo_mapItemIds::BackpackLevel1;
			const bool bExactTarget = (bWindTalisman
					&& Target->ContainerType == FName(TEXT("SpatialRing"))
					&& Target->EquipmentSlot == ECodeBEquipSlot::SpatialItem)
				|| (bBackpackLevel1
					&& Target->ContainerType == FName(TEXT("Backpack"))
					&& Target->EquipmentSlot == ECodeBEquipSlot::Backpack);
			if (!Definition || !bExactTarget || !IsEquipSlotCompatible(*Definition, *Target)
				|| Definition->bStackable || Definition->MaxStack != 1
				|| Definition->ChildContainerCapacity <= 0
				|| !Item->ChildContainerId.IsValid() || Item->Quantity != 1
				|| Request.Quantity != 1 || Target->Slots.Num() != 1
				|| Request.TargetSlot != 0)
			{
				OutResult.Code = ECodeBResultCode::IncompatibleSlot;
				OutResult.Message = TEXT("P42 corpse spatial Move requires its one compatible formal empty player equipment slot.");
				return false;
			}
			TargetSlot = 0;
			if (Target->Slots[0].IsValid())
			{
				OutResult.Code = ECodeBResultCode::TargetOccupied;
				OutResult.Message = TEXT("P42 corpse spatial Move does not replace an occupied equipment slot.");
				return false;
			}
		}
		else if (bP21BodyEquipmentSource && Target && Target->IsEquipment())
		{
			const FCodeBItemDefinition* Definition = Candidate.Definitions.Find(Item->DefinitionId);
			const FString TargetType = Target->ContainerType.ToString();
			const bool bFormalP38Target = TargetType == TEXT("Weapon")
				|| TargetType == TEXT("Armor") || TargetType.StartsWith(TEXT("Accessory"));
			if (!Definition || !bFormalP38Target || IsP21BodyEquipmentContainer(*Target)
				|| !IsEquipSlotCompatible(*Definition, *Target)
				|| Definition->bStackable || Definition->MaxStack != 1
				|| Definition->SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
				|| Definition->ChildContainerCapacity != 0
				|| Item->Quantity != 1 || Item->ChildContainerId.IsValid()
				|| Request.Quantity != 1 || Target->Slots.Num() != 1
				|| (Request.TargetSlot != INDEX_NONE && Request.TargetSlot != 0))
			{
				OutResult.Code = ECodeBResultCode::IncompatibleSlot;
				OutResult.Message = TEXT("P38 corpse equipment Move requires one compatible formal empty player equipment slot.");
				return false;
			}
			TargetSlot = 0;
			if (Target->Slots[0].IsValid())
			{
				OutResult.Code = ECodeBResultCode::TargetOccupied;
				OutResult.Message = TEXT("P38 corpse equipment Move does not replace an occupied equipment slot.");
				return false;
			}
		}
		else
		{
			if (bP21BodyEquipmentSource && Request.Quantity != 1)
			{
				OutResult.Code = ECodeBResultCode::InvalidQuantity;
				OutResult.Message = TEXT("P38 corpse equipment Move requires the one whole root.");
				return false;
			}
			if (!IsValidStorageTarget(Candidate, Request.TargetContainerId, Request.TargetSlot, OutResult, &TargetSlot))
			{
				return false;
			}
			Target = Candidate.Containers.Find(Request.TargetContainerId);
		}
		const int32 OldSlot = Item->SlotIndex;
		const FGuid OldContainerId = Item->ParentContainerId;
		Source->Slots[OldSlot] = FGuid();
		Target->Slots[TargetSlot] = Item->ItemId;
		Item->ParentContainerId = Target->ContainerId;
		Item->SlotIndex = TargetSlot;
		AddAffected(OutResult, Item->ItemId, OldContainerId);
		AddAffected(OutResult, Item->ItemId, Target->ContainerId);
		return true;
	}

	bool FCodeBRepository::ExecuteSwap(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult)
	{
		FCodeBItemInstance* Item = nullptr;
		FCodeBContainer* Source = nullptr;
		if (!ValidateSource(Candidate, Request, Item, Source, OutResult))
		{
			return false;
		}
		if (Source->IsEquipment())
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Equipment swaps must use Equip with a storage source.");
			return false;
		}
		FCodeBContainer* Target = Candidate.Containers.Find(Request.TargetContainerId);
		if (!Target)
		{
			OutResult.Code = ECodeBResultCode::TargetNotFound;
			OutResult.Message = TEXT("Target container was not found.");
			return false;
		}
		if (Target->IsEquipment() || !IsSlotInRange(*Target, Request.TargetSlot))
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Swap target must be a valid storage slot.");
			return false;
		}
		const FGuid TargetItemId = Target->Slots[Request.TargetSlot];
		FCodeBItemInstance* TargetItem = Candidate.Items.Find(TargetItemId);
		if (!TargetItem)
		{
			OutResult.Code = ECodeBResultCode::TargetOccupied;
			OutResult.Message = TEXT("Swap target is empty.");
			return false;
		}
		if (TargetItemId == Item->ItemId || TargetItem->ParentContainerId != Target->ContainerId || TargetItem->SlotIndex != Request.TargetSlot)
		{
			OutResult.Code = ECodeBResultCode::InvariantViolation;
			OutResult.Message = TEXT("Swap target placement is invalid.");
			return false;
		}

		const int32 SourceSlot = Item->SlotIndex;
		const FGuid SourceContainerId = Source->ContainerId;
		Source->Slots[SourceSlot] = TargetItem->ItemId;
		Target->Slots[Request.TargetSlot] = Item->ItemId;
		Item->ParentContainerId = Target->ContainerId;
		Item->SlotIndex = Request.TargetSlot;
		TargetItem->ParentContainerId = Source->ContainerId;
		TargetItem->SlotIndex = SourceSlot;
		AddAffected(OutResult, Item->ItemId, SourceContainerId);
		AddAffected(OutResult, Item->ItemId, Target->ContainerId);
		AddAffected(OutResult, TargetItem->ItemId, SourceContainerId);
		AddAffected(OutResult, TargetItem->ItemId, Target->ContainerId);
		return true;
	}

	bool FCodeBRepository::ExecuteMerge(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult)
	{
		FCodeBItemInstance* SourceItem = nullptr;
		FCodeBContainer* Source = nullptr;
		if (!ValidateSource(Candidate, Request, SourceItem, Source, OutResult))
		{
			return false;
		}
		if (Source->IsEquipment())
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Equipment items cannot be merged.");
			return false;
		}
		FCodeBContainer* Target = Candidate.Containers.Find(Request.TargetContainerId);
		if (!Target || Target->IsEquipment() || !IsSlotInRange(*Target, Request.TargetSlot))
		{
			OutResult.Code = Target ? ECodeBResultCode::InvalidSlot : ECodeBResultCode::TargetNotFound;
			OutResult.Message = TEXT("Merge target is invalid.");
			return false;
		}
		const FGuid TargetItemId = Target->Slots[Request.TargetSlot];
		FCodeBItemInstance* TargetItem = Candidate.Items.Find(TargetItemId);
		if (!TargetItem || TargetItemId == SourceItem->ItemId)
		{
			OutResult.Code = ECodeBResultCode::TargetOccupied;
			OutResult.Message = TEXT("Merge target must contain a different item.");
			return false;
		}
		const FCodeBItemDefinition* Definition = Candidate.Definitions.Find(SourceItem->DefinitionId);
		const FCodeBItemDefinition* TargetDefinition = Candidate.Definitions.Find(TargetItem->DefinitionId);
		if (!Definition || !TargetDefinition || SourceItem->DefinitionId != TargetItem->DefinitionId || !Definition->bStackable)
		{
			OutResult.Code = ECodeBResultCode::StackMismatch;
			OutResult.Message = TEXT("Only equal stackable definitions can merge.");
			return false;
		}
		const int32 Available = Definition->MaxStack - TargetItem->Quantity;
		if (Available <= 0)
		{
			OutResult.Code = ECodeBResultCode::StackFull;
			OutResult.Message = TEXT("Target stack is full.");
			return false;
		}
		const int32 Amount = Request.Quantity > 0 ? Request.Quantity : FMath::Min(SourceItem->Quantity, Available);
		if (Amount <= 0 || Amount > SourceItem->Quantity || Amount > Available)
		{
			OutResult.Code = Amount > Available ? ECodeBResultCode::StackFull : ECodeBResultCode::InvalidQuantity;
			OutResult.Message = TEXT("Merge quantity is invalid.");
			return false;
		}

		const FGuid SourceItemId = SourceItem->ItemId;
		const FGuid SourceContainerId = Source->ContainerId;
		TargetItem->Quantity += Amount;
		SourceItem->Quantity -= Amount;
		AddAffected(OutResult, SourceItemId, SourceContainerId);
		AddAffected(OutResult, TargetItem->ItemId, Target->ContainerId);
		if (SourceItem->Quantity == 0)
		{
			Source->Slots[SourceItem->SlotIndex] = FGuid();
			Candidate.Items.Remove(SourceItemId);
		}
		return true;
	}

	bool FCodeBRepository::ExecuteSplit(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult)
	{
		FCodeBItemInstance* SourceItem = nullptr;
		FCodeBContainer* Source = nullptr;
		if (!ValidateSource(Candidate, Request, SourceItem, Source, OutResult))
		{
			return false;
		}
		if (Source->IsEquipment())
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Equipment items cannot be split.");
			return false;
		}
		const FCodeBItemDefinition* Definition = Candidate.Definitions.Find(SourceItem->DefinitionId);
		if (!Definition || !Definition->bStackable)
		{
			OutResult.Code = ECodeBResultCode::StackMismatch;
			OutResult.Message = TEXT("Only stackable items can be split.");
			return false;
		}
		if (Request.Quantity <= 0 || Request.Quantity >= SourceItem->Quantity)
		{
			OutResult.Code = ECodeBResultCode::InvalidQuantity;
			OutResult.Message = TEXT("Split quantity must leave a non-empty source stack.");
			return false;
		}
		int32 TargetSlot = Request.TargetSlot;
		if (!IsValidStorageTarget(Candidate, Request.TargetContainerId, Request.TargetSlot, OutResult, &TargetSlot))
		{
			return false;
		}
		FCodeBContainer* Target = Candidate.Containers.Find(Request.TargetContainerId);
		const FGuid NewItemId = FGuid::NewGuid();
		if (!NewItemId.IsValid() || Candidate.Items.Contains(NewItemId))
		{
			OutResult.Code = ECodeBResultCode::DuplicateItem;
			OutResult.Message = TEXT("Generated split Item ID was not unique.");
			return false;
		}

		const int32 OriginalQuantity = SourceItem->Quantity;
		SourceItem->Quantity = OriginalQuantity - Request.Quantity;
		FCodeBItemInstance NewItem = *SourceItem;
		NewItem.ItemId = NewItemId;
		NewItem.Quantity = Request.Quantity;
		NewItem.ParentContainerId = Target->ContainerId;
		NewItem.SlotIndex = TargetSlot;
		Candidate.Items.Add(NewItemId, NewItem);
		Target->Slots[TargetSlot] = NewItemId;
		OutResult.CreatedItemId = NewItemId;
		AddAffected(OutResult, SourceItem->ItemId, Source->ContainerId);
		AddAffected(OutResult, NewItemId, Target->ContainerId);
		return true;
	}

	bool FCodeBRepository::ExecuteEquip(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult)
	{
		FCodeBItemInstance* Item = nullptr;
		FCodeBContainer* Source = nullptr;
		if (!ValidateSource(Candidate, Request, Item, Source, OutResult))
		{
			return false;
		}
		if (Source->IsEquipment())
		{
			OutResult.Code = ECodeBResultCode::SourceMismatch;
			OutResult.Message = TEXT("Equip source must be a storage container.");
			return false;
		}
		const FCodeBItemDefinition* Definition = Candidate.Definitions.Find(Item->DefinitionId);
		FCodeBContainer* Target = Candidate.Containers.Find(Request.TargetContainerId);
		if (!Definition || !Target)
		{
			OutResult.Code = Target ? ECodeBResultCode::InvalidDefinition : ECodeBResultCode::TargetNotFound;
			OutResult.Message = TEXT("Equip definition or target was not found.");
			return false;
		}
		if (!Target->IsEquipment() || !IsEquipSlotCompatible(*Definition, *Target))
		{
			OutResult.Code = ECodeBResultCode::IncompatibleSlot;
			OutResult.Message = TEXT("Item type does not match the equipment slot.");
			return false;
		}
		if (!Definition->bStackable && Item->Quantity != 1)
		{
			OutResult.Code = ECodeBResultCode::InvalidQuantity;
			OutResult.Message = TEXT("Equipment quantity must be one.");
			return false;
		}
		if (Request.TargetSlot != INDEX_NONE && Request.TargetSlot != 0)
		{
			OutResult.Code = ECodeBResultCode::InvalidSlot;
			OutResult.Message = TEXT("Equipment target slot must be zero.");
			return false;
		}

		const int32 SourceSlot = Item->SlotIndex;
		const FGuid SourceContainerId = Source->ContainerId;
		const FGuid ExistingItemId = Target->Slots[0];
		FCodeBItemInstance* ExistingItem = ExistingItemId.IsValid() ? Candidate.Items.Find(ExistingItemId) : nullptr;
		if (ExistingItemId.IsValid() && !ExistingItem)
		{
			OutResult.Code = ECodeBResultCode::InvariantViolation;
			OutResult.Message = TEXT("Occupied equipment slot points to no item.");
			return false;
		}

		Source->Slots[SourceSlot] = ExistingItemId;
		Target->Slots[0] = Item->ItemId;
		Item->ParentContainerId = Target->ContainerId;
		Item->SlotIndex = 0;
		if (ExistingItem)
		{
			ExistingItem->ParentContainerId = Source->ContainerId;
			ExistingItem->SlotIndex = SourceSlot;
		}
		AddAffected(OutResult, Item->ItemId, SourceContainerId);
		AddAffected(OutResult, Item->ItemId, Target->ContainerId);
		AddAffected(OutResult, ExistingItemId, SourceContainerId);
		AddAffected(OutResult, ExistingItemId, Target->ContainerId);
		return true;
	}

	bool FCodeBRepository::ExecuteUnequip(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult)
	{
		FCodeBItemInstance* Item = nullptr;
		FCodeBContainer* Source = nullptr;
		if (!ValidateSource(Candidate, Request, Item, Source, OutResult))
		{
			return false;
		}
		if (!Source->IsEquipment())
		{
			OutResult.Code = ECodeBResultCode::SourceMismatch;
			OutResult.Message = TEXT("Unequip source must be an equipment container.");
			return false;
		}
		int32 TargetSlot = Request.TargetSlot;
		if (!IsValidStorageTarget(Candidate, Request.TargetContainerId, Request.TargetSlot, OutResult, &TargetSlot))
		{
			return false;
		}
		FCodeBContainer* Target = Candidate.Containers.Find(Request.TargetContainerId);
		Source->Slots[0] = FGuid();
		Target->Slots[TargetSlot] = Item->ItemId;
		Item->ParentContainerId = Target->ContainerId;
		Item->SlotIndex = TargetSlot;
		AddAffected(OutResult, Item->ItemId, Source->ContainerId);
		AddAffected(OutResult, Item->ItemId, Target->ContainerId);
		return true;
	}

	FCodeBTransactionResult FCodeBRepository::ExecuteTransaction(const FCodeBTransactionRequest& Request)
	{
		FCodeBTransactionResult Result;
		Result.TransactionId = Request.TransactionId.IsValid() ? Request.TransactionId : FGuid::NewGuid();
		Result.NewRevision = State.Revision;
		if (Request.ExpectedRevision != INDEX_NONE && Request.ExpectedRevision != State.Revision)
		{
			return MakeFailure(Request, State.Revision, ECodeBResultCode::StaleRevision, TEXT("Repository revision is stale."));
		}

		FState Candidate = State;
		bool bOperationSucceeded = false;
		switch (Request.Operation)
		{
		case ECodeBOperation::Move:
			bOperationSucceeded = ExecuteMove(Candidate, Request, Result);
			break;
		case ECodeBOperation::Swap:
			bOperationSucceeded = ExecuteSwap(Candidate, Request, Result);
			break;
		case ECodeBOperation::Merge:
			bOperationSucceeded = ExecuteMerge(Candidate, Request, Result);
			break;
		case ECodeBOperation::Split:
			bOperationSucceeded = ExecuteSplit(Candidate, Request, Result);
			break;
		case ECodeBOperation::Equip:
			bOperationSucceeded = ExecuteEquip(Candidate, Request, Result);
			break;
		case ECodeBOperation::Unequip:
			bOperationSucceeded = ExecuteUnequip(Candidate, Request, Result);
			break;
		default:
			Result.Code = ECodeBResultCode::InternalCommitFailure;
			Result.Message = TEXT("Unknown Code B operation.");
			break;
		}

		if (!bOperationSucceeded)
		{
			Result.bSuccess = false;
			Result.NewRevision = State.Revision;
			return Result;
		}

		FString InvariantError;
		if (!ValidateState(Candidate, &InvariantError))
		{
			return MakeFailure(Request, State.Revision, ECodeBResultCode::InvariantViolation, *InvariantError);
		}
		Candidate.Revision = State.Revision + 1;
		State = MoveTemp(Candidate);
		Result.bSuccess = true;
		Result.Code = ECodeBResultCode::Success;
		Result.Message = TEXT("Code B transaction committed atomically.");
		Result.NewRevision = State.Revision;
		return Result;
	}
}
