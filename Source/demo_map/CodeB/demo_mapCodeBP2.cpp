#include "CodeB/demo_mapCodeBP2.h"

namespace demo_map_code_b
{
	namespace
	{
		void SetError(FString* OutError, const FString& Message)
		{
			if (OutError)
			{
				*OutError = Message;
			}
		}

		FGuid MakeFixtureGuid(uint32 Number)
		{
			return FGuid(0xB2000000u + Number, 0x00000002u, 0x00000009u, 0x000000B0u);
		}

		FName MakeSlotId(FName Role, int32 SlotIndex)
		{
			return FName(*FString::Printf(TEXT("%s.%02d"), *Role.ToString(), SlotIndex));
		}

		bool AddDefinition(
			FCodeBRepository& Repository,
			FName DefinitionId,
			ECodeBItemType ItemType,
			bool bStackable,
			int32 MaxStack,
			ECodeBEquipSlot EquipSlot,
			FString* OutError)
		{
			FCodeBItemDefinition Definition;
			Definition.DefinitionId = DefinitionId;
			Definition.ItemType = ItemType;
			Definition.bStackable = bStackable;
			Definition.bQuickUsable = ItemType == ECodeBItemType::Consumable;
			Definition.MaxStack = MaxStack;
			Definition.EquipSlot = EquipSlot;
			return Repository.RegisterDefinition(Definition, OutError);
		}

		bool IsEmptyContainer(const FCodeBRepository& Repository, const FGuid& ContainerId)
		{
			const FCodeBContainer* Container = Repository.FindContainer(ContainerId);
			if (!Container)
			{
				return false;
			}
			for (const FGuid& ItemId : Container->Slots)
			{
				if (ItemId.IsValid())
				{
					return false;
				}
			}
			return true;
		}
	}

	TArray<TPair<FName, FGuid>> FCodeBP2PlayerLayout::GetOrderedContainers() const
	{
		TArray<TPair<FName, FGuid>> Result;
		Result.Add(TPair<FName, FGuid>(FName(TEXT("Warehouse")), WarehouseContainerId));
		Result.Add(TPair<FName, FGuid>(FName(TEXT("Basic6")), BasicContainerId));
		Result.Add(TPair<FName, FGuid>(FName(TEXT("Weapon")), WeaponContainerId));
		Result.Add(TPair<FName, FGuid>(FName(TEXT("Armor")), ArmorContainerId));
		Result.Add(TPair<FName, FGuid>(FName(TEXT("SpatialRing")), SpatialContainerId));
		if (BackpackContainerId.IsValid())
		{
			Result.Add(TPair<FName, FGuid>(FName(TEXT("Backpack")), BackpackContainerId));
		}
		for (int32 Index = 0; Index < AccessoryContainerIds.Num(); ++Index)
		{
			Result.Add(TPair<FName, FGuid>(
				FName(*FString::Printf(TEXT("Accessory%d"), Index)),
				AccessoryContainerIds[Index]));
		}
		Result.Add(TPair<FName, FGuid>(FName(TEXT("QuickSpatial")), SpatialInternalContainerId));
		Result.Add(TPair<FName, FGuid>(FName(TEXT("PouchInternal")), PouchInternalContainerId));
		for (const TPair<FName, FGuid>& TransientRef : TransientPresentationContainers)
		{
			if (!TransientRef.Key.IsNone() && TransientRef.Value.IsValid())
			{
				Result.Add(TransientRef);
			}
		}
		return Result;
	}

	bool FCodeBP2SlotView::operator==(const FCodeBP2SlotView& Other) const
	{
		return SlotId == Other.SlotId
			&& SlotIndex == Other.SlotIndex
			&& bOccupied == Other.bOccupied
			&& ItemId == Other.ItemId
			&& DefinitionId == Other.DefinitionId
			&& Quantity == Other.Quantity
			&& Level == Other.Level
			&& Quality == Other.Quality
			&& RandomSeed == Other.RandomSeed
			&& ItemType == Other.ItemType
			&& bQuickUsable == Other.bQuickUsable
			&& EquipSlot == Other.EquipSlot
			&& ChildContainerId == Other.ChildContainerId;
	}

	bool FCodeBP2ContainerView::operator==(const FCodeBP2ContainerView& Other) const
	{
		return Role == Other.Role
			&& ContainerId == Other.ContainerId
			&& Capacity == Other.Capacity
			&& Slots == Other.Slots;
	}

	bool FCodeBP2Projection::operator==(const FCodeBP2Projection& Other) const
	{
		return Revision == Other.Revision
			&& LayoutId == Other.LayoutId
			&& WarehouseContainerId == Other.WarehouseContainerId
			&& BasicContainerId == Other.BasicContainerId
			&& Containers == Other.Containers
			&& LastTransactionId == Other.LastTransactionId
			&& AffectedItemIds == Other.AffectedItemIds
			&& AffectedContainerIds == Other.AffectedContainerIds;
	}

	bool FCodeBP2ProjectionBuilder::Build(
		const FCodeBRepository& Repository,
		const FCodeBP2PlayerLayout& Layout,
		FCodeBP2Projection& OutProjection,
		const FCodeBTransactionResult* LastTransaction,
		FString* OutError)
	{
		OutProjection = FCodeBP2Projection();
		FString InvariantError;
		if (!Repository.ValidateInvariants(&InvariantError))
		{
			SetError(OutError, InvariantError);
			return false;
		}
		if (!Layout.LayoutId.IsValid())
		{
			SetError(OutError, TEXT("P2 layout id is invalid."));
			return false;
		}

		OutProjection.Revision = Repository.GetRevision();
		OutProjection.LayoutId = Layout.LayoutId;
		OutProjection.WarehouseContainerId = Layout.WarehouseContainerId;
		OutProjection.BasicContainerId = Layout.BasicContainerId;
		const auto ResolveConditionalChildContainer = [&Repository, &Layout](const FName Role, const FGuid& DefaultContainerId)
		{
			if (!Layout.bUseConditionalSpatialContainers)
			{
				return DefaultContainerId;
			}

			// A space item's child container belongs to the equipped item itself,
			// rather than to the old preparation-layout reference that happened to
			// exist when the Profile was migrated.  This lets a later P1/P2 equip,
			// replacement, or unequip immediately change what P3 can expose.
			const FGuid EquipmentContainerId = Role == FName(TEXT("QuickSpatial"))
				? Layout.SpatialContainerId
				: Role == FName(TEXT("PouchInternal"))
					? Layout.BackpackContainerId
					: FGuid();
			if (!EquipmentContainerId.IsValid())
			{
				return DefaultContainerId;
			}
			const FCodeBContainer* Equipment = Repository.FindContainer(EquipmentContainerId);
			const FGuid EquippedItemId = Equipment && !Equipment->Slots.IsEmpty()
				? Equipment->Slots[0]
				: FGuid();
			const FCodeBItemInstance* EquippedItem = Repository.FindItem(EquippedItemId);
			return EquippedItem && EquippedItem->ChildContainerId.IsValid()
				? EquippedItem->ChildContainerId
				: FGuid();
		};

		TSet<FGuid> SeenItemIds;
		for (const TPair<FName, FGuid>& Ref : Layout.GetOrderedContainers())
		{
			const FGuid EffectiveContainerId = ResolveConditionalChildContainer(Ref.Key, Ref.Value);
			if (!EffectiveContainerId.IsValid())
			{
				continue;
			}
			const FCodeBContainer* Container = Repository.FindContainer(EffectiveContainerId);
			if (!Container)
			{
				SetError(OutError, FString::Printf(TEXT("Projection container is missing for role %s."), *Ref.Key.ToString()));
				return false;
			}

			FCodeBP2ContainerView ContainerView;
			ContainerView.Role = Ref.Key;
			ContainerView.ContainerId = Container->ContainerId;
			ContainerView.Capacity = Container->Slots.Num();
			ContainerView.Slots.Reserve(Container->Slots.Num());
			for (int32 SlotIndex = 0; SlotIndex < Container->Slots.Num(); ++SlotIndex)
			{
				FCodeBP2SlotView SlotView;
				SlotView.SlotId = MakeSlotId(Ref.Key, SlotIndex);
				SlotView.SlotIndex = SlotIndex;
				const FGuid ItemId = Container->Slots[SlotIndex];
				if (ItemId.IsValid())
				{
					const FCodeBItemInstance* Item = Repository.FindItem(ItemId);
					const FCodeBItemDefinition* Definition = Item ? Repository.FindDefinition(Item->DefinitionId) : nullptr;
					if (!Item || !Definition || SeenItemIds.Contains(ItemId))
					{
						SetError(OutError, TEXT("Projection found a missing or duplicate item."));
						return false;
					}
					SeenItemIds.Add(ItemId);
					SlotView.bOccupied = true;
					SlotView.ItemId = Item->ItemId;
					SlotView.DefinitionId = Item->DefinitionId;
					SlotView.Quantity = Item->Quantity;
					SlotView.Level = Item->Level;
					SlotView.Quality = Item->Quality;
					SlotView.RandomSeed = Item->RandomSeed;
					SlotView.ItemType = Definition->ItemType;
					SlotView.bQuickUsable = Definition->bQuickUsable;
					SlotView.EquipSlot = Definition->EquipSlot;
					SlotView.ChildContainerId = Item->ChildContainerId;
				}
				ContainerView.Slots.Add(SlotView);
			}
			OutProjection.Containers.Add(MoveTemp(ContainerView));
		}

		if (LastTransaction)
		{
			OutProjection.LastTransactionId = LastTransaction->TransactionId;
			OutProjection.AffectedItemIds = LastTransaction->AffectedItemIds;
			OutProjection.AffectedContainerIds = LastTransaction->AffectedContainerIds;
		}
		return true;
	}

	bool FCodeBP2Fixture::Build(FCodeBP2Fixture& OutFixture, FString* OutError)
	{
		OutFixture = FCodeBP2Fixture();
		FString Error;
		FCodeBP2FixtureIds& Ids = OutFixture.Ids;
		Ids.WarehouseContainerId = MakeFixtureGuid(1);
		Ids.BasicContainerId = MakeFixtureGuid(2);
		Ids.WeaponContainerId = MakeFixtureGuid(3);
		Ids.ArmorContainerId = MakeFixtureGuid(4);
		Ids.SpatialContainerId = MakeFixtureGuid(5);
		Ids.Accessory0ContainerId = MakeFixtureGuid(6);
		Ids.Accessory1ContainerId = MakeFixtureGuid(7);
		Ids.SpatialInternalContainerId = MakeFixtureGuid(8);
		Ids.PouchInternalContainerId = MakeFixtureGuid(9);
		Ids.WeaponAItemId = MakeFixtureGuid(101);
		Ids.WeaponBItemId = MakeFixtureGuid(102);
		Ids.ArmorItemId = MakeFixtureGuid(103);
		Ids.AccessoryAItemId = MakeFixtureGuid(104);
		Ids.AccessoryBItemId = MakeFixtureGuid(105);
		Ids.SpatialItemId = MakeFixtureGuid(106);
		Ids.DustAItemId = MakeFixtureGuid(107);
		Ids.DustBItemId = MakeFixtureGuid(108);
		Ids.PotionItemId = MakeFixtureGuid(109);
		Ids.InvalidEquipItemId = MakeFixtureGuid(110);
		Ids.SpatialPouchItemId = MakeFixtureGuid(111);

		FCodeBRepository& Repository = OutFixture.Repository;
		if (!AddDefinition(Repository, FName(TEXT("Weapon.A")), ECodeBItemType::Weapon, false, 1, ECodeBEquipSlot::Weapon, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Weapon.B")), ECodeBItemType::Weapon, false, 1, ECodeBEquipSlot::Weapon, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Armor.Robe")), ECodeBItemType::Armor, false, 1, ECodeBEquipSlot::Armor, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Accessory.A")), ECodeBItemType::Accessory, false, 1, ECodeBEquipSlot::Accessory, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Accessory.B")), ECodeBItemType::Accessory, false, 1, ECodeBEquipSlot::Accessory, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Spatial.Ring")), ECodeBItemType::SpatialItem, false, 1, ECodeBEquipSlot::SpatialItem, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Spatial.Pouch")), ECodeBItemType::SpatialItem, false, 1, ECodeBEquipSlot::None, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Material.Dust")), ECodeBItemType::Material, true, 20, ECodeBEquipSlot::None, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Consumable.Potion")), ECodeBItemType::Consumable, true, 10, ECodeBEquipSlot::None, &Error)
			|| !AddDefinition(Repository, FName(TEXT("Generic.InvalidEquip")), ECodeBItemType::Generic, false, 1, ECodeBEquipSlot::None, &Error))
		{
			SetError(OutError, Error);
			return false;
		}

		if (!Repository.CreateContainer(FName(TEXT("Stash")), 30, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, Ids.WarehouseContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("Basic6")), 6, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, Ids.BasicContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("WeaponSlot")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Weapon, &Error, Ids.WeaponContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("ArmorSlot")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Armor, &Error, Ids.ArmorContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("SpatialSlot")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::SpatialItem, &Error, Ids.SpatialContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("AccessorySlot0")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Accessory, &Error, Ids.Accessory0ContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("AccessorySlot1")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Accessory, &Error, Ids.Accessory1ContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("QuickSpatialInternal")), 4, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, Ids.SpatialInternalContainerId).IsValid()
			|| !Repository.CreateContainer(FName(TEXT("PouchInternal")), 4, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, Ids.PouchInternalContainerId).IsValid())
		{
			SetError(OutError, Error);
			return false;
		}

		const auto AddItem = [&](FName DefinitionId, int32 Quantity, int32 Slot, const FGuid& ItemId) -> bool
		{
			return Repository.CreateItem(DefinitionId, Quantity, Ids.WarehouseContainerId, Slot, ItemId, &Error).IsValid();
		};
		if (!AddItem(FName(TEXT("Weapon.A")), 1, 0, Ids.WeaponAItemId)
			|| !AddItem(FName(TEXT("Weapon.B")), 1, 1, Ids.WeaponBItemId)
			|| !AddItem(FName(TEXT("Armor.Robe")), 1, 2, Ids.ArmorItemId)
			|| !AddItem(FName(TEXT("Accessory.A")), 1, 3, Ids.AccessoryAItemId)
			|| !AddItem(FName(TEXT("Accessory.B")), 1, 4, Ids.AccessoryBItemId)
			|| !AddItem(FName(TEXT("Spatial.Ring")), 1, 5, Ids.SpatialItemId)
			|| !AddItem(FName(TEXT("Material.Dust")), 12, 6, Ids.DustAItemId)
			|| !AddItem(FName(TEXT("Material.Dust")), 8, 7, Ids.DustBItemId)
			|| !AddItem(FName(TEXT("Consumable.Potion")), 5, 8, Ids.PotionItemId)
			|| !AddItem(FName(TEXT("Generic.InvalidEquip")), 1, 9, Ids.InvalidEquipItemId)
			|| !AddItem(FName(TEXT("Spatial.Pouch")), 1, 10, Ids.SpatialPouchItemId)
			|| !Repository.AssociateChildContainer(Ids.SpatialItemId, Ids.SpatialInternalContainerId, &Error)
			|| !Repository.AssociateChildContainer(Ids.SpatialPouchItemId, Ids.PouchInternalContainerId, &Error))
		{
			SetError(OutError, Error);
			return false;
		}

		FCodeBP2PlayerLayout& Layout = OutFixture.Layout;
		Layout.LayoutId = FName(TEXT("P2.PlayerStashFixture"));
		Layout.WarehouseContainerId = Ids.WarehouseContainerId;
		Layout.BasicContainerId = Ids.BasicContainerId;
		Layout.WeaponContainerId = Ids.WeaponContainerId;
		Layout.ArmorContainerId = Ids.ArmorContainerId;
		Layout.SpatialContainerId = Ids.SpatialContainerId;
		Layout.AccessoryContainerIds = { Ids.Accessory0ContainerId, Ids.Accessory1ContainerId };
		Layout.SpatialInternalContainerId = Ids.SpatialInternalContainerId;
		Layout.PouchInternalContainerId = Ids.PouchInternalContainerId;

		if (!OutFixture.Validate(&Error))
		{
			SetError(OutError, Error);
			return false;
		}
		return true;
	}

	bool FCodeBP2Fixture::Validate(FString* OutError) const
	{
		if (!Repository.ValidateInvariants(OutError))
		{
			return false;
		}
		if (Layout.AccessoryContainerIds.Num() < 2
			|| !Layout.WarehouseContainerId.IsValid()
			|| !Layout.BasicContainerId.IsValid()
			|| !Layout.SpatialInternalContainerId.IsValid()
			|| !Layout.PouchInternalContainerId.IsValid())
		{
			SetError(OutError, TEXT("P2 layout is incomplete."));
			return false;
		}
		const FCodeBContainer* Warehouse = Repository.FindContainer(Layout.WarehouseContainerId);
		const FCodeBContainer* Basic = Repository.FindContainer(Layout.BasicContainerId);
		const FCodeBContainer* SpatialInternal = Repository.FindContainer(Layout.SpatialInternalContainerId);
		const FCodeBContainer* PouchInternal = Repository.FindContainer(Layout.PouchInternalContainerId);
		const FCodeBItemInstance* SpatialItem = Repository.FindItem(Ids.SpatialItemId);
		const FCodeBItemInstance* PouchItem = Repository.FindItem(Ids.SpatialPouchItemId);
		if (!Warehouse || Warehouse->Slots.Num() < 30 || !Basic || Basic->Slots.Num() != 6
			|| !SpatialInternal || SpatialInternal->Slots.Num() <= 0 || !PouchInternal || PouchInternal->Slots.Num() <= 0
			|| !SpatialItem || !PouchItem
			|| SpatialItem->ChildContainerId != Layout.SpatialInternalContainerId
			|| PouchItem->ChildContainerId != Layout.PouchInternalContainerId)
		{
			SetError(OutError, TEXT("P2 Fixture does not meet the player/stash minimums."));
			return false;
		}
		return true;
	}

	FCodeBP2ApplicationService FCodeBP2Fixture::MakeApplicationService()
	{
		return FCodeBP2ApplicationService(Repository, Layout);
	}

	FCodeBP2ApplicationService::FCodeBP2ApplicationService(FCodeBRepository& InRepository, const FCodeBP2PlayerLayout& InLayout)
		: Repository(InRepository)
		, Layout(InLayout)
	{
	}

	bool FCodeBP2ApplicationService::BuildCurrentProjection(FCodeBP2Projection& OutProjection, FString* OutError) const
	{
		return FCodeBP2ProjectionBuilder::Build(Repository, Layout, OutProjection, nullptr, OutError);
	}

	bool FCodeBP2ApplicationService::IsLoadedSpatialItemMoveUnsupported(const FCodeBP2Command& Command, FString& OutMessage) const
	{
		if (Command.Operation != ECodeBOperation::Move
			&& Command.Operation != ECodeBOperation::Swap
			&& Command.Operation != ECodeBOperation::Equip
			&& Command.Operation != ECodeBOperation::Unequip)
		{
			return false;
		}
		const FCodeBItemInstance* Item = Repository.FindItem(Command.ItemId);
		if (!Item || !Item->ChildContainerId.IsValid() || Command.SourceContainerId == Command.TargetContainerId)
		{
			return false;
		}
		if (!IsEmptyContainer(Repository, Item->ChildContainerId))
		{
			OutMessage = TEXT("Loaded spatial-item overall movement is outside P2 scope.");
			return true;
		}
		return false;
	}

	FCodeBP2ApplicationResult FCodeBP2ApplicationService::Apply(const FCodeBP2Command& Command)
	{
		FCodeBP2ApplicationResult Result;
		Result.Command = Command;
		if (!Command.ItemId.IsValid())
		{
			Result.Code = ECodeBP2ResultCode::InvalidCommand;
			Result.Message = TEXT("P2 command requires a valid ItemId.");
			return Result;
		}

		FCodeBP2Command EffectiveCommand = Command;
		if (!EffectiveCommand.TransactionId.IsValid())
		{
			EffectiveCommand.TransactionId = FGuid::NewGuid();
		}
		Result.Command = EffectiveCommand;

		FString UnsupportedMessage;
		if (IsLoadedSpatialItemMoveUnsupported(EffectiveCommand, UnsupportedMessage))
		{
			Result.Code = ECodeBP2ResultCode::LoadedSpatialItemMoveUnsupported;
			Result.Message = UnsupportedMessage;
			BuildCurrentProjection(Result.Projection, nullptr);
			return Result;
		}

		FCodeBTransactionRequest Request;
		Request.TransactionId = EffectiveCommand.TransactionId;
		Request.Operation = EffectiveCommand.Operation;
		Request.ItemId = EffectiveCommand.ItemId;
		Request.SourceContainerId = EffectiveCommand.SourceContainerId;
		Request.SourceSlot = EffectiveCommand.SourceSlot;
		Request.TargetContainerId = EffectiveCommand.TargetContainerId;
		Request.TargetSlot = EffectiveCommand.TargetSlot;
		Request.Quantity = EffectiveCommand.Quantity;
		Request.ExpectedRevision = EffectiveCommand.ExpectedRevision;
		Result.P1Result = Repository.ExecuteTransaction(Request);

		FString ProjectionError;
		if (!FCodeBP2ProjectionBuilder::Build(Repository, Layout, Result.Projection, &Result.P1Result, &ProjectionError))
		{
			Result.Code = ECodeBP2ResultCode::ProjectionFailure;
			Result.Message = ProjectionError;
			return Result;
		}
		if (!Result.P1Result.IsSuccess())
		{
			Result.Code = ECodeBP2ResultCode::P1Failure;
			Result.Message = Result.P1Result.Message;
			return Result;
		}

		Result.bSuccess = true;
		Result.Code = ECodeBP2ResultCode::Success;
		Result.Message = TEXT("P2 application command committed through the P1 transaction core.");
		return Result;
	}
}
