// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeB/demo_mapCodeBP3.h"

#include "demo_mapItemDefinitions.h"

namespace
{
	using namespace demo_map_code_b;

	FString ResultCodeToChinese(const ECodeBResultCode Code)
	{
		switch (Code)
		{
		case ECodeBResultCode::TargetFull: return TEXT("目标容器已满");
		case ECodeBResultCode::TargetOccupied: return TEXT("目标格已被占用");
		case ECodeBResultCode::IncompatibleSlot:
		case ECodeBResultCode::NotEquipable: return TEXT("物品类型与目标栏位不匹配");
		case ECodeBResultCode::SourceMismatch:
		case ECodeBResultCode::ItemNotFound: return TEXT("来源物品状态无效");
		case ECodeBResultCode::StaleRevision: return TEXT("物品状态已变化，请重新操作");
		case ECodeBResultCode::InvalidQuantity: return TEXT("拆分数量无效");
		case ECodeBResultCode::StackMismatch: return TEXT("目标物品无法合并");
		case ECodeBResultCode::StackFull: return TEXT("目标堆叠已满");
		default: return TEXT("操作未通过物品规则校验");
		}
	}

	ECodeBSpatialContainerSemantic ResolveP17SpatialContainerSemantic(const FCodeBItemDefinition& Definition)
	{
		if (Definition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None)
		{
			return Definition.SpatialContainerSemantic;
		}

		// Legacy P5/P6 snapshots can predate the persisted semantic fields. Read
		// only the immutable product definition to identify their real item type;
		// do not infer from localized display text or modify the old graph.
		const Fdemo_mapItemDefinition* Source = Fdemo_mapItemDefinitions::Find(Definition.DefinitionId);
		if (!Source)
		{
			return ECodeBSpatialContainerSemantic::None;
		}
		if (Source->CategoryId == Fdemo_mapItemIds::SpatialRingCategory
			&& Definition.ItemType == ECodeBItemType::SpatialItem
			&& Definition.EquipSlot == ECodeBEquipSlot::SpatialItem)
		{
			const Fdemo_mapSpatialRingCapacityResult Ring =
				Fdemo_mapItemDefinitions::ResolveSpatialRingCapacity(Definition.DefinitionId);
			return Ring.bSuccess && Ring.Capacity > 0
				? ECodeBSpatialContainerSemantic::QuickRing
				: ECodeBSpatialContainerSemantic::None;
		}
		if (Source->CategoryId == Fdemo_mapItemIds::BackpackCategory
			&& Definition.ItemType == ECodeBItemType::Backpack
			&& Definition.EquipSlot == ECodeBEquipSlot::Backpack)
		{
			const Fdemo_mapSpatialStorageCapacityResult Pouch =
				Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(Definition.DefinitionId);
			return Pouch.bSuccess && Pouch.Capacity > 0
				? ECodeBSpatialContainerSemantic::StoragePouch
				: ECodeBSpatialContainerSemantic::None;
		}
		return ECodeBSpatialContainerSemantic::None;
	}
}

namespace demo_map_code_b
{
	bool FCodeBP3UIController::Open(FString* OutError)
	{
		if (bOpen)
		{
			Feedback = TEXT("开发 Host 已打开，已复用当前页面状态");
			return true;
		}

		if (!Fixture.IsValid())
		{
			ProfileRepository = nullptr;
			ProfileCommit = nullptr;
			PresentationScope = ECodeBP3PresentationScope::OutOfRaidProfile;
			Fixture = MakeUnique<FCodeBP2Fixture>();
			FString BuildError;
			if (!FCodeBP2Fixture::Build(*Fixture, &BuildError))
			{
				Fixture.Reset();
				Feedback = FString::Printf(TEXT("无法创建 P3 Fixture：%s"), *BuildError);
				if (OutError) *OutError = Feedback;
				return false;
			}
			Service = MakeUnique<FCodeBP2ApplicationService>(Fixture->MakeApplicationService());
		}

		if (!RefreshProjection(OutError))
		{
			return false;
		}
		bOpen = true;
		Feedback = FString::Printf(TEXT("Code B 开发 Host 已启动（Revision %d）"), Projection.Revision);
		return true;
	}

	bool FCodeBP3UIController::OpenProfile(
		FCodeBRepository& InRepository,
		const FCodeBP2PlayerLayout& InLayout,
		FProfileCommit InCommit,
		FString* OutError)
	{
		return OpenRepository(
			InRepository,
			InLayout,
			MoveTemp(InCommit),
			ECodeBP3PresentationScope::OutOfRaidProfile,
			OutError);
	}

	bool FCodeBP3UIController::OpenActiveRun(
		FCodeBRepository& InRepository,
		const FCodeBP2PlayerLayout& InLayout,
		FProfileCommit InCommit,
		FString* OutError)
	{
		return OpenRepository(
			InRepository,
			InLayout,
			MoveTemp(InCommit),
			ECodeBP3PresentationScope::ActiveRun,
			OutError);
	}

	bool FCodeBP3UIController::OpenRepository(
		FCodeBRepository& InRepository,
		const FCodeBP2PlayerLayout& InLayout,
		FProfileCommit InCommit,
		const ECodeBP3PresentationScope InPresentationScope,
		FString* OutError)
	{
		if (bOpen)
		{
			if (ProfileRepository == &InRepository && PresentationScope == InPresentationScope)
			{
				ActiveLayout = InLayout;
				return RefreshProjection(OutError);
			}
			if (OutError) *OutError = TEXT("Code B Profile Host is already open for another repository.");
			return false;
		}
		FString InvariantError;
		if (!InRepository.ValidateInvariants(&InvariantError))
		{
			if (OutError) *OutError = InvariantError;
			return false;
		}
		Fixture.Reset();
		ProfileRepository = &InRepository;
		ProfileCommit = MoveTemp(InCommit);
		PresentationScope = InPresentationScope;
		ActiveLayout = InLayout;
		ActiveRunSpatialContainer.Reset();
		Service = MakeUnique<FCodeBP2ApplicationService>(InRepository, InLayout);
		if (!RefreshProjection(OutError))
		{
			Service.Reset();
			ProfileRepository = nullptr;
			ProfileCommit = nullptr;
			ActiveLayout = FCodeBP2PlayerLayout();
			return false;
		}
		bOpen = true;
		Feedback = InPresentationScope == ECodeBP3PresentationScope::ActiveRun
			? FString::Printf(TEXT("Code B 真实活动 Run 个人背包已打开（Revision %d）"), Projection.Revision)
			: FString::Printf(TEXT("Code B 真实 Profile 仓库已打开（Revision %d）"), Projection.Revision);
		return true;
	}

	void FCodeBP3UIController::Close()
	{
		const bool bClosingProfile = IsProfileBacked();
		CancelOperation(TEXT("页面已关闭，未提交任何待定操作"));
		ActiveRunSpatialContainer.Reset();
		bOpen = false;
		Feedback = bClosingProfile
			? TEXT("页面已关闭；真实 Profile 持久化状态将在重开时恢复")
			: TEXT("页面已关闭；Fixture 与 Revision 将在重开时保留");
	}

	void FCodeBP3UIController::Shutdown()
	{
		bOpen = false;
		SelectedAddress.Reset();
		PendingSource.Reset();
		OperationMode = ECodeBP3OperationMode::None;
		Service.Reset();
		Fixture.Reset();
		ProfileRepository = nullptr;
		ProfileCommit = nullptr;
		PresentationScope = ECodeBP3PresentationScope::OutOfRaidProfile;
		Projection = FCodeBP2Projection();
		ActiveLayout = FCodeBP2PlayerLayout();
		ActiveRunSpatialContainer.Reset();
		Feedback = TEXT("开发 Host 已退出并释放隔离 Fixture");
		SplitQuantity = 1;
		TransactionSerial = 1;
	}

	const FCodeBP2FixtureIds* FCodeBP3UIController::GetFixtureIds() const
	{
		return Fixture.IsValid() ? &Fixture->GetIds() : nullptr;
	}

	bool FCodeBP3UIController::RefreshProjection(FString* OutError)
	{
		if (!Service.IsValid())
		{
			const FString Error = TEXT("P3 Application Service 尚未启动。");
			if (OutError) *OutError = Error;
			Feedback = Error;
			return false;
		}
		FString Error;
		if (!Service->BuildCurrentProjection(Projection, &Error))
		{
			Feedback = FString::Printf(TEXT("无法刷新权威 Projection：%s"), *Error);
			if (OutError) *OutError = Feedback;
			return false;
		}
		if (ActiveRunSpatialContainer.IsSet())
		{
			FCodeBP7SpatialContainerProjection RefreshedSpatialProjection;
			if (BuildActiveRunSpatialContainerProjection(
				ActiveRunSpatialContainer->ParentItemId, RefreshedSpatialProjection, nullptr))
			{
				ActiveRunSpatialContainer = MoveTemp(RefreshedSpatialProjection);
			}
			else
			{
				// Unequip, recovery, stale refresh, or an invalidated child graph
				// closes the transient P7 entry instead of caching a writable mirror.
				ActiveRunSpatialContainer.Reset();
			}
		}
		return true;
	}

	bool FCodeBP3UIController::ValidateSplitSource(
		const FCodeBP3SlotAddress& Address,
		const int32 RequestedQuantity,
		FString& OutError) const
	{
		OutError.Reset();
		if (!bOpen || !ProfileRepository || !Address.IsValid() || !Address.ItemId.IsValid())
		{
			OutError = TEXT("拆分只接受当前真实 Profile／Run 图中的已显示物品。");
			return false;
		}
		const FCodeBItemInstance* Item = ProfileRepository->FindItem(Address.ItemId);
		const FCodeBItemDefinition* Definition = Item
			? ProfileRepository->FindDefinition(Item->DefinitionId) : nullptr;
		const FCodeBContainer* Container = Item
			? ProfileRepository->FindContainer(Item->ParentContainerId) : nullptr;
		if (!Item || !Definition || !Container
			|| Item->ParentContainerId != Address.ContainerId
			|| Item->SlotIndex != Address.SlotIndex
			|| !Container->Slots.IsValidIndex(Address.SlotIndex)
			|| Container->Slots[Address.SlotIndex] != Item->ItemId)
		{
			OutError = TEXT("拆分来源已变化，请重新选择。");
			return false;
		}
		if (Container->IsEquipment() || Item->ChildContainerId.IsValid()
			|| !Definition->bStackable || Definition->MaxStack <= 1
			|| Item->Quantity <= 1)
		{
			OutError = TEXT("只有普通储物格中的无子容器可堆叠物品才能拆分。");
			return false;
		}
		if (RequestedQuantity < 1 || RequestedQuantity >= Item->Quantity)
		{
			OutError = FString::Printf(TEXT("拆分数量必须在 1—%d 之间。"), Item->Quantity - 1);
			return false;
		}
		return true;
	}

	const FCodeBP2ContainerView* FCodeBP3UIController::FindContainerView(const FGuid& ContainerId) const
	{
		return Projection.Containers.FindByPredicate([&ContainerId](const FCodeBP2ContainerView& Candidate)
		{
			return Candidate.ContainerId == ContainerId;
		});
	}

	bool FCodeBP3UIController::BuildActiveRunSpatialContainerProjection(
		const FGuid& ParentItemId,
		FCodeBP7SpatialContainerProjection& OutProjection,
		FString* OutError) const
	{
		OutProjection = FCodeBP7SpatialContainerProjection();
		if (!IsActiveRunBacked() || !ProfileRepository || !ParentItemId.IsValid())
		{
			if (OutError) *OutError = TEXT("P7 requires an exact active Run repository and parent item.");
			return false;
		}

		const FCodeBItemInstance* ParentItem = ProfileRepository->FindItem(ParentItemId);
		const FCodeBItemDefinition* ParentDefinition = ParentItem
			? ProfileRepository->FindDefinition(ParentItem->DefinitionId)
			: nullptr;
		const FCodeBContainer* ChildContainer = ParentItem && ParentItem->ChildContainerId.IsValid()
			? ProfileRepository->FindContainer(ParentItem->ChildContainerId)
			: nullptr;
		const ECodeBSpatialContainerSemantic Semantic = ParentDefinition
			? ResolveP17SpatialContainerSemantic(*ParentDefinition)
			: ECodeBSpatialContainerSemantic::None;
		if (!ParentItem || !ParentDefinition || !ChildContainer || ChildContainer->IsEquipment()
			|| Semantic == ECodeBSpatialContainerSemantic::None)
		{
			if (OutError) *OutError = TEXT("Selected P6 item has no valid real spatial child container.");
			return false;
		}
		if (Semantic == ECodeBSpatialContainerSemantic::QuickRing
			&& ParentItem->ParentContainerId != ActiveLayout.SpatialContainerId)
		{
			if (OutError) *OutError = TEXT("Quick spatial entry requires the item to remain in the current spatial-ring slot.");
			return false;
		}
		if (Semantic == ECodeBSpatialContainerSemantic::StoragePouch && !ParentItem->IsPlaced())
		{
			if (OutError) *OutError = TEXT("Storage pouch entry requires its real placed P6 item.");
			return false;
		}

		OutProjection.ParentItemId = ParentItem->ItemId;
		OutProjection.ParentDefinitionId = ParentItem->DefinitionId;
		OutProjection.ContainerId = ChildContainer->ContainerId;
		OutProjection.Semantic = Semantic;
		OutProjection.bQuickEntry = Semantic == ECodeBSpatialContainerSemantic::QuickRing;
		OutProjection.Container.Role = OutProjection.bQuickEntry
			? FName(TEXT("QuickSpatial"))
			: FName(TEXT("PouchInternal"));
		OutProjection.Container.ContainerId = ChildContainer->ContainerId;
		OutProjection.Container.Capacity = ChildContainer->Slots.Num();
		OutProjection.Container.Slots.Reserve(ChildContainer->Slots.Num());
		for (int32 SlotIndex = 0; SlotIndex < ChildContainer->Slots.Num(); ++SlotIndex)
		{
			FCodeBP2SlotView Slot;
			Slot.SlotId = FName(*FString::Printf(TEXT("%s.%d"), *OutProjection.Container.Role.ToString(), SlotIndex));
			Slot.SlotIndex = SlotIndex;
			const FGuid ItemId = ChildContainer->Slots[SlotIndex];
			if (ItemId.IsValid())
			{
				const FCodeBItemInstance* Item = ProfileRepository->FindItem(ItemId);
				const FCodeBItemDefinition* Definition = Item
					? ProfileRepository->FindDefinition(Item->DefinitionId)
					: nullptr;
				if (!Item || !Definition)
				{
					if (OutError) *OutError = TEXT("P7 spatial child container has an invalid P1 item reference.");
					return false;
				}
				Slot.bOccupied = true;
				Slot.ItemId = Item->ItemId;
				Slot.DefinitionId = Item->DefinitionId;
				Slot.Quantity = Item->Quantity;
				Slot.Level = Item->Level;
				Slot.Quality = Item->Quality;
				Slot.RandomSeed = Item->RandomSeed;
				Slot.ItemType = Definition->ItemType;
				Slot.bQuickUsable = Definition->bQuickUsable;
				Slot.EquipSlot = Definition->EquipSlot;
				Slot.ChildContainerId = Item->ChildContainerId;
			}
			OutProjection.Container.Slots.Add(MoveTemp(Slot));
		}
		return true;
	}

	bool FCodeBP3UIController::EnterActiveRunSpatialContainer(
		const FGuid& ParentItemId,
		FCodeBP7SpatialContainerProjection& OutProjection,
		FString* OutError)
	{
		if (!BuildActiveRunSpatialContainerProjection(ParentItemId, OutProjection, OutError))
		{
			Feedback = OutError && !OutError->IsEmpty()
				? *OutError
				: TEXT("无法进入当前空间道具。");
			return false;
		}
		ActiveRunSpatialContainer = OutProjection;
		Feedback = OutProjection.bQuickEntry
			? TEXT("已进入当前装备纳物戒的只读快速空间")
			: TEXT("已进入当前 P6 纳物袋的只读存储空间");
		return true;
	}

	void FCodeBP3UIController::ReturnFromActiveRunSpatialContainer()
	{
		ActiveRunSpatialContainer.Reset();
		if (IsActiveRunBacked())
		{
			Feedback = TEXT("已返回活动 Run 背包主视图");
		}
	}

	bool FCodeBP3UIController::MakeAddress(const FGuid& ContainerId, const int32 SlotIndex, FCodeBP3SlotAddress& OutAddress) const
	{
		const FCodeBP2ContainerView* Container = FindContainerView(ContainerId);
		const FCodeBP2SlotView* Slot = Container
			? Container->Slots.FindByPredicate([SlotIndex](const FCodeBP2SlotView& Candidate)
				{ return Candidate.SlotIndex == SlotIndex; })
			: nullptr;
		if (!Slot)
		{
			return false;
		}
		OutAddress.ContainerId = ContainerId;
		OutAddress.SlotIndex = Slot->SlotIndex;
		OutAddress.SlotId = Slot->SlotId;
		OutAddress.ItemId = Slot->ItemId;
		OutAddress.bOccupied = Slot->bOccupied;
		OutAddress.CellState = Slot->bOccupied ? ECodeBP3CellState::Revealed : ECodeBP3CellState::Empty;
		OutAddress.SearchLocator = FCodeBP3SearchLocator();
		return true;
	}

	bool FCodeBP3UIController::FindAddressForItem(const FGuid& ItemId, FCodeBP3SlotAddress& OutAddress) const
	{
		for (const FCodeBP2ContainerView& Container : Projection.Containers)
		{
			for (const FCodeBP2SlotView& Slot : Container.Slots)
			{
				if (Slot.bOccupied && Slot.ItemId == ItemId)
				{
					OutAddress.ContainerId = Container.ContainerId;
					OutAddress.SlotIndex = Slot.SlotIndex;
					OutAddress.SlotId = Slot.SlotId;
					OutAddress.ItemId = Slot.ItemId;
					OutAddress.bOccupied = true;
					OutAddress.CellState = ECodeBP3CellState::Revealed;
					OutAddress.SearchLocator = FCodeBP3SearchLocator();
					return true;
				}
			}
		}
		return false;
	}

	bool FCodeBP3UIController::SelectAddress(const FCodeBP3SlotAddress& Address)
	{
		FCodeBP3SlotAddress AuthoritativeAddress;
		if (!MakeAddress(Address.ContainerId, Address.SlotIndex, AuthoritativeAddress))
		{
			Feedback = TEXT("该格位不在当前权威 Projection 中");
			return false;
		}
		if (!AuthoritativeAddress.bOccupied)
		{
			Feedback = TEXT("空格仅可在已选择操作目标时使用");
			return false;
		}
		SelectedAddress = AuthoritativeAddress;
		Feedback = FString::Printf(TEXT("已选择 %s（%s）"), *AuthoritativeAddress.ItemId.ToString(EGuidFormats::DigitsWithHyphens), *AuthoritativeAddress.SlotId.ToString());
		return true;
	}

	bool FCodeBP3UIController::BeginOperation(const ECodeBP3OperationMode InOperationMode)
	{
		if (IsActiveRunBacked() && InOperationMode != ECodeBP3OperationMode::None)
		{
			// P7 deliberately exposes no click/menu/hotkey command path. Its mounted
			// P4 DragOperation and explicit Drop remain the only write authority.
			Feedback = TEXT("活动 Run 仅支持拖拽到明确目标格；未提交任何物品状态");
			return false;
		}
		if (InOperationMode == ECodeBP3OperationMode::None)
		{
			CancelOperation();
			return true;
		}
		if (!bOpen || !SelectedAddress.IsSet() || !SelectedAddress->bOccupied)
		{
			Feedback = TEXT("请先选择一个物品作为操作来源");
			return false;
		}
		PendingSource = SelectedAddress;
		OperationMode = InOperationMode;
		if (OperationMode == ECodeBP3OperationMode::Split && SplitQuantity <= 0)
		{
			SplitQuantity = 1;
		}
		Feedback = FString::Printf(TEXT("%s：请选择明确目标格"), *GetModeLabel(OperationMode));
		return true;
	}

	void FCodeBP3UIController::CancelOperation(const FString& Reason)
	{
		PendingSource.Reset();
		OperationMode = ECodeBP3OperationMode::None;
		Feedback = Reason;
	}

	void FCodeBP3UIController::ClearTransientSelection(const FString& Reason)
	{
		SelectedAddress.Reset();
		CancelOperation(Reason);
	}

	FGuid FCodeBP3UIController::MakeTransactionId(const uint32 Serial)
	{
		return FGuid(0xB3000000u + Serial, 0x00000003u, 0x00000009u, 0x000000B3u);
	}

	FCodeBP2Command FCodeBP3UIController::MakeCommand(const FCodeBP3SlotAddress& Source, const FCodeBP3SlotAddress& Target, const int32 ExpectedRevision) const
	{
		FCodeBP2Command Command;
		Command.TransactionId = MakeTransactionId(TransactionSerial);
		Command.ItemId = Source.ItemId;
		Command.SourceContainerId = Source.ContainerId;
		Command.SourceSlot = Source.SlotIndex;
		Command.TargetContainerId = Target.ContainerId;
		Command.TargetSlot = Target.SlotIndex;
		Command.ExpectedRevision = ExpectedRevision;
		switch (OperationMode)
		{
		case ECodeBP3OperationMode::Move: Command.Operation = ECodeBOperation::Move; break;
		case ECodeBP3OperationMode::Equip: Command.Operation = ECodeBOperation::Equip; break;
		case ECodeBP3OperationMode::Unequip: Command.Operation = ECodeBOperation::Unequip; break;
		case ECodeBP3OperationMode::Split: Command.Operation = ECodeBOperation::Split; Command.Quantity = SplitQuantity; break;
		case ECodeBP3OperationMode::Merge: Command.Operation = ECodeBOperation::Merge; Command.Quantity = SplitQuantity; break;
		default: break;
		}
		return Command;
	}

	void FCodeBP3UIController::RestoreSelectionAfterResult(const FCodeBP2ApplicationResult& Result, const FGuid& PreferredItemId)
	{
		FCodeBP3SlotAddress RestoredAddress;
		if (PreferredItemId.IsValid() && FindAddressForItem(PreferredItemId, RestoredAddress))
		{
			SelectedAddress = RestoredAddress;
			return;
		}
		if (Result.Command.Operation == ECodeBOperation::Merge && FindAddressForItem(Result.Command.ItemId, RestoredAddress))
		{
			SelectedAddress = RestoredAddress;
			return;
		}
		SelectedAddress.Reset();
	}

	bool FCodeBP3UIController::PersistProfileSnapshotAfterAcceptedP1(
		const FCodeBSnapshot& BeforeSnapshot,
		const FCodeBP2Command& AcceptedCommand,
		FString& OutError)
	{
		if (!ProfileRepository || !ProfileCommit)
		{
			return true;
		}
		if (ProfileCommit(ProfileRepository->CaptureSnapshot(), AcceptedCommand, OutError))
		{
			return true;
		}
		FString RollbackError;
		if (!ProfileRepository->LoadPersistedSnapshot(BeforeSnapshot, &RollbackError))
		{
			OutError = FString::Printf(TEXT("%s；P1 回滚失败：%s"), *OutError, *RollbackError);
		}
		RefreshProjection();
		return false;
	}

	bool FCodeBP3UIController::ActivateAddress(const FCodeBP3SlotAddress& TargetAddress, const int32 ExpectedRevisionOverride)
	{
		if (!bOpen)
		{
			Feedback = TEXT("开发 Host 尚未启动");
			return false;
		}
		FCodeBP3SlotAddress AuthoritativeTarget;
		if (!MakeAddress(TargetAddress.ContainerId, TargetAddress.SlotIndex, AuthoritativeTarget))
		{
			Feedback = TEXT("目标格不在当前权威 Projection 中");
			return false;
		}
		if (OperationMode == ECodeBP3OperationMode::None || !PendingSource.IsSet())
		{
			return SelectAddress(AuthoritativeTarget);
		}
		if (IsActiveRunBacked())
		{
			CancelOperation(TEXT("活动 Run 的点击操作已拒绝；请使用真实拖拽 Drop"));
			return false;
		}

		FCodeBP3SlotAddress AuthoritativeSource;
		if (!MakeAddress(PendingSource->ContainerId, PendingSource->SlotIndex, AuthoritativeSource) || !AuthoritativeSource.bOccupied || AuthoritativeSource.ItemId != PendingSource->ItemId)
		{
			SelectedAddress.Reset();
			CancelOperation(TEXT("来源物品状态已变化，请重新操作"));
			return false;
		}

		const int32 ExpectedRevision = ExpectedRevisionOverride == INDEX_NONE ? Projection.Revision : ExpectedRevisionOverride;
		const FCodeBP2Command Command = MakeCommand(AuthoritativeSource, AuthoritativeTarget, ExpectedRevision);
		const FString CompletedModeLabel = GetModeLabel(OperationMode);
		const FGuid PreferredSelectionItem = Command.Operation == ECodeBOperation::Merge && AuthoritativeTarget.bOccupied ? AuthoritativeTarget.ItemId : AuthoritativeSource.ItemId;
		const FCodeBSnapshot BeforeSnapshot = ProfileRepository
			? ProfileRepository->CaptureSnapshot() : FCodeBSnapshot();
		const FCodeBP2ApplicationResult Result = Service->Apply(Command);
		++TransactionSerial;
		PendingSource.Reset();
		OperationMode = ECodeBP3OperationMode::None;

		if (Result.IsSuccess())
		{
			FString PersistenceError;
			if (!PersistProfileSnapshotAfterAcceptedP1(BeforeSnapshot, Command, PersistenceError))
			{
				SelectedAddress.Reset();
				Feedback = FString::Printf(TEXT("保存失败，未提交本次操作：%s"), *PersistenceError);
				return false;
			}
			Projection = Result.Projection;
			RestoreSelectionAfterResult(Result, PreferredSelectionItem);
			Feedback = FString::Printf(TEXT("操作成功：%s（Revision %d）"), *CompletedModeLabel, Projection.Revision);
			return true;
		}

		Projection = Result.Projection;
		if (Result.P1Result.Code == ECodeBResultCode::StaleRevision)
		{
			RefreshProjection();
			SelectedAddress.Reset();
			Feedback = TEXT("物品状态已变化，请重新操作");
		}
		else
		{
			RestoreSelectionAfterResult(Result, AuthoritativeSource.ItemId);
			Feedback = GetResultFeedback(Result);
		}
		return false;
	}

	bool FCodeBP3UIController::CommitP4Operation(
		const ECodeBOperation Operation,
		const FCodeBP3SlotAddress& Source,
		const FCodeBP3SlotAddress& Target,
		const int32 ExpectedRevision,
		const int32 Quantity,
		const FString& OperationLabel,
		const ECodeBP2CommandIntent Intent,
		const FGuid& QuickTransferActivePlayerContainerId,
		const uint32 ActivePlayerChildOpenGeneration,
		const ECodeBQuickTransferTargetMode QuickTransferTargetMode,
		const FGuid& QuickTransferActivePlayerParentItemId,
		const FCodeBP38BodyEquipmentTransferProof& P38BodyEquipmentProof,
		const FCodeBP40BodySimpleStackQuickTransferProof& P40BodySimpleStackProof,
		const FCodeBP46NormalContainerSimpleStackQuickTransferProof& P46NormalContainerSimpleStackProof,
		const FCodeBP41BodySpatialGraphQuickTransferProof& P41BodySpatialGraphProof,
		const FCodeBP42BodySpatialGraphEquipmentTransferProof& P42BodySpatialGraphEquipmentProof)
	{
		if (!bOpen || !Service.IsValid())
		{
			Feedback = TEXT("开发 Host 尚未启动");
			return false;
		}

		FCodeBP3SlotAddress AuthoritativeSource;
		FCodeBP3SlotAddress AuthoritativeTarget;
		if (!MakeAddress(Source.ContainerId, Source.SlotIndex, AuthoritativeSource)
			|| !MakeAddress(Target.ContainerId, Target.SlotIndex, AuthoritativeTarget)
			|| !AuthoritativeSource.bOccupied
			|| AuthoritativeSource.ItemId != Source.ItemId)
		{
			SelectedAddress.Reset();
			PendingSource.Reset();
			OperationMode = ECodeBP3OperationMode::None;
			Feedback = TEXT("来源物品状态已变化，请重新操作");
			return false;
		}

		FCodeBP2Command Command;
		Command.TransactionId = MakeTransactionId(TransactionSerial);
		Command.Operation = Operation;
		Command.ItemId = AuthoritativeSource.ItemId;
		Command.SourceContainerId = AuthoritativeSource.ContainerId;
		Command.SourceSlot = AuthoritativeSource.SlotIndex;
		Command.TargetContainerId = AuthoritativeTarget.ContainerId;
		Command.TargetSlot = AuthoritativeTarget.SlotIndex;
		Command.Quantity = Quantity;
		Command.ExpectedRevision = ExpectedRevision;
		Command.Intent = Intent;
		Command.QuickTransferActivePlayerContainerId = QuickTransferActivePlayerContainerId;
		Command.ActivePlayerChildOpenGeneration = ActivePlayerChildOpenGeneration;
		Command.QuickTransferTargetMode = QuickTransferTargetMode;
		Command.QuickTransferActivePlayerParentItemId = QuickTransferActivePlayerParentItemId;
		Command.P38BodyEquipmentProof = P38BodyEquipmentProof;
		Command.P40BodySimpleStackProof = P40BodySimpleStackProof;
		Command.P46NormalContainerSimpleStackProof = P46NormalContainerSimpleStackProof;
		Command.P41BodySpatialGraphProof = P41BodySpatialGraphProof;
		Command.P42BodySpatialGraphEquipmentProof = P42BodySpatialGraphEquipmentProof;

		const FGuid PreferredSelectionItem = Operation == ECodeBOperation::Merge && AuthoritativeTarget.bOccupied
			? AuthoritativeTarget.ItemId
			: AuthoritativeSource.ItemId;
		const FCodeBSnapshot BeforeSnapshot = ProfileRepository
			? ProfileRepository->CaptureSnapshot() : FCodeBSnapshot();
		const FCodeBP2ApplicationResult Result = Service->Apply(Command);
		++TransactionSerial;
		PendingSource.Reset();
		OperationMode = ECodeBP3OperationMode::None;

		if (Result.IsSuccess())
		{
			FString PersistenceError;
			if (!PersistProfileSnapshotAfterAcceptedP1(BeforeSnapshot, Command, PersistenceError))
			{
				SelectedAddress.Reset();
				Feedback = FString::Printf(TEXT("保存失败，未提交本次拖拽：%s"), *PersistenceError);
				return false;
			}
			Projection = Result.Projection;
			RestoreSelectionAfterResult(Result, PreferredSelectionItem);
			Feedback = FString::Printf(TEXT("拖拽成功：%s（Revision %d）"), *OperationLabel, Projection.Revision);
			return true;
		}

		Projection = Result.Projection;
		if (Result.P1Result.Code == ECodeBResultCode::StaleRevision)
		{
			RefreshProjection();
			SelectedAddress.Reset();
			Feedback = TEXT("物品状态已变化，请重新操作");
		}
		else
		{
			RestoreSelectionAfterResult(Result, AuthoritativeSource.ItemId);
			Feedback = GetResultFeedback(Result);
		}
		return false;
	}

	FString FCodeBP3UIController::GetModeLabel(const ECodeBP3OperationMode InMode)
	{
		switch (InMode)
		{
		case ECodeBP3OperationMode::Move: return TEXT("移动");
		case ECodeBP3OperationMode::Equip: return TEXT("装备");
		case ECodeBP3OperationMode::Unequip: return TEXT("卸下");
		case ECodeBP3OperationMode::Split: return TEXT("拆分");
		case ECodeBP3OperationMode::Merge: return TEXT("合并");
		default: return TEXT("普通选择");
		}
	}

	FString FCodeBP3UIController::GetResultFeedback(const FCodeBP2ApplicationResult& Result)
	{
		if (Result.P1Result.Code == ECodeBResultCode::StaleRevision)
		{
			return TEXT("物品状态已变化，请重新操作");
		}
		return FString::Printf(TEXT("操作失败：%s"), *ResultCodeToChinese(Result.P1Result.Code));
	}
}
