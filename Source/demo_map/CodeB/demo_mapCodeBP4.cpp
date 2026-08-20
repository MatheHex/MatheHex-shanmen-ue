// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeB/demo_mapCodeBP4.h"

#include "demo_mapItemDefinitions.h"

namespace
{
	using namespace demo_map_code_b;

	bool IsSameAddress(const FCodeBP3SlotAddress& A, const FCodeBP3SlotAddress& B)
	{
		return A.ContainerId == B.ContainerId && A.SlotIndex == B.SlotIndex;
	}
}

namespace demo_map_code_b
{
	const FCodeBP2ContainerView* FCodeBP4InteractionController::FindContainer(const FGuid& ContainerId) const
	{
		return Controller.GetProjection().Containers.FindByPredicate([&ContainerId](const FCodeBP2ContainerView& Candidate)
		{
			return Candidate.ContainerId == ContainerId;
		});
	}

	const FCodeBP2SlotView* FCodeBP4InteractionController::FindSlot(const FCodeBP3SlotAddress& Address) const
	{
		const FCodeBP2ContainerView* Container = FindContainer(Address.ContainerId);
		return Container ? Container->Slots.FindByPredicate([&Address](const FCodeBP2SlotView& Slot)
		{
			return Slot.SlotIndex == Address.SlotIndex;
		}) : nullptr;
	}

	bool FCodeBP4InteractionController::IsEquipmentContainer(const FGuid& ContainerId) const
	{
		const FCodeBP2ContainerView* Container = FindContainer(ContainerId);
		if (!Container)
		{
			return false;
		}
		return Container->Role == FName(TEXT("Weapon"))
			|| Container->Role == FName(TEXT("Armor"))
			|| Container->Role == FName(TEXT("SpatialRing"))
			|| Container->Role == FName(TEXT("Backpack"))
			|| Container->Role.ToString().StartsWith(TEXT("Accessory"));
	}

	bool FCodeBP4InteractionController::IsP21BodyEquipmentContainer(const FGuid& ContainerId) const
	{
		const FCodeBP2ContainerView* Container = FindContainer(ContainerId);
		return Container && (Container->Role == FName(TEXT("Body.Weapon"))
			|| Container->Role == FName(TEXT("Body.ArmorRobe"))
			|| Container->Role == FName(TEXT("Body.Accessory0")));
	}

	bool FCodeBP4InteractionController::IsCompatibleEquipmentTarget(const FCodeBP2SlotView& Source, const FGuid& TargetContainerId) const
	{
		const FCodeBP2ContainerView* Target = FindContainer(TargetContainerId);
		if (!Target)
		{
			return false;
		}
		// P4 remains a projection-only planner, but the P2 slot carries the
		// P1 definition's equip slot.  ItemType alone cannot distinguish an
		// ordinary unwearable spatial pouch from an equippable spatial ring.
		if (Target->Role == FName(TEXT("Weapon"))) return Source.EquipSlot == ECodeBEquipSlot::Weapon;
		if (Target->Role == FName(TEXT("Armor"))) return Source.EquipSlot == ECodeBEquipSlot::Armor;
		if (Target->Role == FName(TEXT("SpatialRing"))) return Source.EquipSlot == ECodeBEquipSlot::SpatialItem;
		if (Target->Role == FName(TEXT("Backpack"))) return Source.EquipSlot == ECodeBEquipSlot::Backpack;
		return Target->Role.ToString().StartsWith(TEXT("Accessory"))
			&& Source.EquipSlot == ECodeBEquipSlot::Accessory;
	}

	FCodeBP4DropPreview FCodeBP4InteractionController::Reject(const FString& Message)
	{
		FCodeBP4DropPreview Preview;
		Preview.Message = Message;
		return Preview;
	}

	bool FCodeBP4InteractionController::BeginDrag(const FCodeBP3SlotAddress& Source, FCodeBP4DragPayload& OutPayload)
	{
		OutPayload = FCodeBP4DragPayload();
		if (!Controller.IsOpen())
		{
			Controller.SetP4Feedback(TEXT("开发 Host 尚未启动"));
			return false;
		}

		FCodeBP3SlotAddress AuthoritativeSource;
		const FCodeBP2SlotView* SourceSlot = FindSlot(Source);
		if (!SourceSlot || !Controller.MakeAddress(Source.ContainerId, Source.SlotIndex, AuthoritativeSource) || !SourceSlot->bOccupied)
		{
			Controller.SetP4Feedback(TEXT("空格、禁用占位或已失效来源不能开始拖拽"));
			return false;
		}

		Controller.CancelOperation(TEXT("已切换到拖拽操作；未提交任何待定点击操作"));
		OutPayload.Source = AuthoritativeSource;
		OutPayload.ItemId = SourceSlot->ItemId;
		OutPayload.ExpectedRevision = Controller.GetProjection().Revision;
		OutPayload.DefinitionId = SourceSlot->DefinitionId;
		OutPayload.Quantity = SourceSlot->Quantity;
		OutPayload.bStackable = SourceSlot->bStackable;
		OutPayload.MaxStack = SourceSlot->MaxStack;
		OutPayload.Level = SourceSlot->Level;
		OutPayload.Quality = SourceSlot->Quality;
		OutPayload.RandomSeed = SourceSlot->RandomSeed;
		OutPayload.LegacyAffixDigest = SourceSlot->LegacyAffixDigest;
		OutPayload.SourceScope = ECodeBP3InventoryScope::Unknown;
		OutPayload.SessionId = NextSessionId++;
		Controller.SetP4Feedback(FString::Printf(TEXT("正在拖拽 %s；放下时才会提交事务"), *SourceSlot->DefinitionId.ToString()));
		return true;
	}

	bool FCodeBP4InteractionController::BeginSplitDrag(
		const FCodeBP3SlotAddress& Source,
		const int32 RequestedQuantity,
		FCodeBP4DragPayload& OutPayload,
		const ECodeBP3QuantityDraftKind DraftKind)
	{
		if (DraftKind == ECodeBP3QuantityDraftKind::None)
		{
			OutPayload = FCodeBP4DragPayload();
			Controller.SetP4Feedback(TEXT("数量拖拽缺少明确意图种类。"));
			return false;
		}
		FString SplitError;
		if (!Controller.ValidateSplitSource(Source, RequestedQuantity, SplitError))
		{
			OutPayload = FCodeBP4DragPayload();
			Controller.SetP4Feedback(SplitError);
			return false;
		}
		if (!BeginDrag(Source, OutPayload))
		{
			return false;
		}
		OutPayload.bSplitIntent = true;
		OutPayload.QuantityDraftKind = DraftKind;
		OutPayload.RequestedMergeQuantity = RequestedQuantity;
		Controller.SetP4Feedback(FString::Printf(
			TEXT("数量草稿已确认：拖动 %d 个到明确空格或兼容未满堆叠；Drop 前不写入。"), RequestedQuantity));
		return true;
	}

	FCodeBP4DropPreview FCodeBP4InteractionController::PreviewDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target) const
	{
		if (!Payload.IsValid()) return Reject(TEXT("拖拽 payload 无效"));
		if (!Controller.IsOpen()) return Reject(TEXT("页面已关闭"));
		if (Payload.ExpectedRevision != Controller.GetProjection().Revision) return Reject(TEXT("物品状态已变化，请重新操作"));
		if (IsSameAddress(Payload.Source, Target)) return Reject(TEXT("不能拖放到同一来源格"));

		const FCodeBP2SlotView* SourceSlot = FindSlot(Payload.Source);
		const FCodeBP2SlotView* TargetSlot = FindSlot(Target);
		if (!SourceSlot || !TargetSlot || !SourceSlot->bOccupied || SourceSlot->ItemId != Payload.ItemId
			|| SourceSlot->DefinitionId != Payload.DefinitionId || SourceSlot->Quantity != Payload.Quantity)
		{
			return Reject(TEXT("来源或目标已变化，请重新操作"));
		}
		const bool bSourceEquipment = IsEquipmentContainer(Payload.Source.ContainerId);
		const bool bP21BodyEquipmentSource = IsP21BodyEquipmentContainer(Payload.Source.ContainerId);
		const bool bTargetEquipment = IsEquipmentContainer(Target.ContainerId);
		FCodeBP4DropPreview Preview;
		Preview.bAllowed = true;
		if (Payload.bSplitIntent)
		{
			FString SplitError;
			if (!Controller.ValidateSplitSource(Payload.Source, Payload.RequestedMergeQuantity, SplitError))
			{
				return Reject(SplitError);
			}
			if (bSourceEquipment || bTargetEquipment || SourceSlot->ChildContainerId.IsValid())
			{
				return Reject(TEXT("拆分不能使用装备格、装备目标或空间父物品。"));
			}
			if (TargetSlot->bOccupied)
			{
				if (TargetSlot->ChildContainerId.IsValid()
					|| SourceSlot->DefinitionId != TargetSlot->DefinitionId
					|| !SourceSlot->bStackable || !TargetSlot->bStackable
					|| SourceSlot->MaxStack <= 1 || SourceSlot->MaxStack != TargetSlot->MaxStack)
				{
					return Reject(TEXT("数量草稿只接受同一正式堆叠定义的普通未满目标。"));
				}
				const int32 Available = TargetSlot->MaxStack - TargetSlot->Quantity;
				if (Available < Payload.RequestedMergeQuantity)
				{
					return Reject(FString::Printf(
						TEXT("目标只剩 %d 个容量，不能接受明确请求的 %d 个；数量不会自动截断。"),
						FMath::Max(0, Available), Payload.RequestedMergeQuantity));
				}
				Preview.Kind = ECodeBP4DropKind::Merge;
				Preview.Operation = ECodeBOperation::Merge;
				Preview.Quantity = Payload.RequestedMergeQuantity;
				Preview.ProjectedAcceptedQuantity = Payload.RequestedMergeQuantity;
				Preview.Message = FString::Printf(
					TEXT("可精确合并 %d 个；来源与目标 ItemId 均保持不变"), Payload.RequestedMergeQuantity);
				return Preview;
			}
			Preview.Kind = ECodeBP4DropKind::Split;
			Preview.Operation = ECodeBOperation::Split;
			Preview.Quantity = Payload.RequestedMergeQuantity;
			Preview.ProjectedAcceptedQuantity = Payload.RequestedMergeQuantity;
			Preview.Message = FString::Printf(TEXT("可在此生成 %d 个的新堆叠"), Payload.RequestedMergeQuantity);
			return Preview;
		}
		if (bTargetEquipment)
		{
			if (bSourceEquipment) return Reject(TEXT("装备栏之间不能直接拖拽"));
			if (!IsCompatibleEquipmentTarget(*SourceSlot, Target.ContainerId)) return Reject(TEXT("物品类型与目标栏位不匹配"));
			if (Payload.P48NormalContainerSpatialGraphEquipmentProof.bIntent)
			{
				if (TargetSlot->bOccupied) return Reject(TEXT("P48 BasicCache 空间图只接受明确空正式装备位，不替换"));
				Preview.Kind = ECodeBP4DropKind::Move;
				Preview.Operation = ECodeBOperation::Move;
				Preview.Quantity = 1;
				Preview.Message = TEXT("可将已揭示 BasicCache 完整空间图移动到该明确空正式装备位");
				return Preview;
			}
			if (Payload.P42BodySpatialGraphEquipmentProof.bIntent)
			{
				if (TargetSlot->bOccupied) return Reject(TEXT("P42 尸体空间图只接受明确空正式装备位，不替换"));
				Preview.Kind = ECodeBP4DropKind::Move;
				Preview.Operation = ECodeBOperation::Move;
				Preview.Quantity = 1;
				Preview.Message = TEXT("可将已揭示尸体完整空间图移动到该明确空正式装备位");
				return Preview;
			}
			if (bP21BodyEquipmentSource)
			{
				if (TargetSlot->bOccupied) return Reject(TEXT("P38 尸体装备只接受明确空装备位，不替换"));
				Preview.Kind = ECodeBP4DropKind::Move;
				Preview.Operation = ECodeBOperation::Move;
				Preview.Quantity = 1;
				Preview.Message = TEXT("可将已揭示尸体装备直接移动到该明确空装备位");
				return Preview;
			}
			Preview.Kind = TargetSlot->bOccupied ? ECodeBP4DropKind::Replacement : ECodeBP4DropKind::Equip;
			Preview.Operation = ECodeBOperation::Equip;
			Preview.Message = TargetSlot->bOccupied ? TEXT("可替换已装备物品") : TEXT("可装备到此栏位");
			return Preview;
		}
		if (bSourceEquipment)
		{
			if (TargetSlot->bOccupied) return Reject(TEXT("卸下目标格已被占用"));
			Preview.Kind = ECodeBP4DropKind::Unequip;
			Preview.Operation = ECodeBOperation::Unequip;
			Preview.Message = TEXT("可卸下到此储物格");
			return Preview;
		}
		if (!TargetSlot->bOccupied)
		{
			Preview.Kind = ECodeBP4DropKind::Move;
			Preview.Operation = ECodeBOperation::Move;
			if (bP21BodyEquipmentSource)
			{
				Preview.Quantity = 1;
				Preview.Message = TEXT("可将已揭示尸体装备移动到该明确空普通格");
				return Preview;
			}
			// P34/P36/P37/P39 standard whole-root QuickTransfer carries its one accepted
			// item explicitly. P1 Move ignores quantity, but the durable proof rejects
			// the P29/P30 Quantity=0 stack/graph semantics for this source family.
			const bool bP41WholeGraphQuickTransfer = Payload.bQuickTransferIntent
				&& Payload.P41BodySpatialGraphProof.bIntent
				&& SourceSlot->Quantity == 1 && SourceSlot->ChildContainerId.IsValid()
				&& (SourceSlot->DefinitionId == Fdemo_mapItemIds::WindTalisman
					|| SourceSlot->DefinitionId == Fdemo_mapItemIds::BackpackLevel1);
			const bool bStandardWholeRootQuickTransfer = Payload.bQuickTransferIntent
				&& !SourceSlot->bStackable && SourceSlot->MaxStack == 1
				&& SourceSlot->Quantity == 1 && !SourceSlot->ChildContainerId.IsValid()
				&& ((SourceSlot->ItemType == ECodeBItemType::Weapon
						&& SourceSlot->EquipSlot == ECodeBEquipSlot::Weapon)
					|| (SourceSlot->ItemType == ECodeBItemType::Armor
						&& SourceSlot->EquipSlot == ECodeBEquipSlot::Armor)
					|| (SourceSlot->ItemType == ECodeBItemType::Accessory
						&& SourceSlot->EquipSlot == ECodeBEquipSlot::Accessory));
			const bool bP47WholeGraphQuickTransfer = Payload.bQuickTransferIntent
				&& Payload.P47NormalContainerSpatialGraphProof.bIntent
				&& SourceSlot->Quantity == 1 && SourceSlot->ChildContainerId.IsValid()
				&& (SourceSlot->DefinitionId == Fdemo_mapItemIds::WindTalisman
					|| SourceSlot->DefinitionId == Fdemo_mapItemIds::BackpackLevel1);
			const bool bP60WholeGraphQuickTransfer = Payload.bQuickTransferIntent
				&& Payload.P60PlayerToNormalContainerSpatialGraphProof.bIntent
				&& SourceSlot->Quantity == 1 && SourceSlot->ChildContainerId.IsValid()
				&& (SourceSlot->DefinitionId == Fdemo_mapItemIds::WindTalisman
					|| SourceSlot->DefinitionId == Fdemo_mapItemIds::BackpackLevel1);
			Preview.Quantity = (bStandardWholeRootQuickTransfer || bP41WholeGraphQuickTransfer
				|| bP47WholeGraphQuickTransfer || bP60WholeGraphQuickTransfer) ? 1 : 0;
			Preview.Message = TEXT("可移动到空储物格");
			return Preview;
		}
		if (SourceSlot->DefinitionId == TargetSlot->DefinitionId
			&& SourceSlot->bStackable && TargetSlot->bStackable
			&& SourceSlot->MaxStack > 1 && SourceSlot->MaxStack == TargetSlot->MaxStack)
		{
			const int32 Available = TargetSlot->MaxStack - TargetSlot->Quantity;
			if (Available <= 0) return Reject(TEXT("目标堆叠已满"));
			const int32 Accepted = FMath::Min(SourceSlot->Quantity, Available);
			Preview.Kind = ECodeBP4DropKind::Merge;
			Preview.Operation = ECodeBOperation::Merge;
			// Quantity zero deliberately delegates the normal full-stack amount to
			// P1 ExecuteMerge. The accepted preview remains display-only.
			Preview.Quantity = 0;
			Preview.ProjectedAcceptedQuantity = Accepted;
			Preview.bPartialAcceptance = Accepted < SourceSlot->Quantity;
			Preview.Message = Preview.bPartialAcceptance
				? FString::Printf(TEXT("可部分接收 %d 个；来源保留 %d 个并保持原位"), Accepted, SourceSlot->Quantity - Accepted)
				: FString::Printf(TEXT("可完整合并 %d 个"), Accepted);
			return Preview;
		}
		Preview.Kind = ECodeBP4DropKind::Swap;
		Preview.Operation = ECodeBOperation::Swap;
		Preview.Message = TEXT("可交换两个储物格");
		return Preview;
	}

	bool FCodeBP4InteractionController::CommitPreview(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target, const FCodeBP4DropPreview& Preview)
	{
		if (!Preview.bAllowed)
		{
			Controller.SetP4Feedback(Preview.Message);
			return false;
		}
		FCodeBP3SourceAction Action;
		Action.Operation = Preview.Operation;
		Action.Source = Payload.Source;
		Action.Target = Target;
		Action.ExpectedRevision = Payload.ExpectedRevision;
		Action.Quantity = Preview.Quantity;
		Action.OperationLabel = GetDropKindLabel(Preview.Kind);
		Action.Intent = Payload.bQuickTransferIntent
			? ECodeBP2CommandIntent::QuickTransfer : ECodeBP2CommandIntent::Standard;
		Action.QuickTransferActivePlayerContainerId = Payload.QuickTransferActivePlayerContainerId;
		Action.ActivePlayerChildOpenGeneration = Payload.ActivePlayerChildOpenGeneration;
		Action.QuickTransferTargetMode = Payload.QuickTransferTargetMode;
		Action.QuickTransferActivePlayerParentItemId = Payload.QuickTransferActivePlayerParentItemId;
		Action.P38BodyEquipmentProof = Payload.P38BodyEquipmentProof;
		Action.P40BodySimpleStackProof = Payload.P40BodySimpleStackProof;
		Action.P46NormalContainerSimpleStackProof = Payload.P46NormalContainerSimpleStackProof;
		Action.P62NormalContainerStandardEquipmentProof = Payload.P62NormalContainerStandardEquipmentProof;
		Action.P63NormalContainerPlayerSimpleStackProof = Payload.P63NormalContainerPlayerSimpleStackProof;
		Action.P58PlayerToNormalContainerSimpleStackProof = Payload.P58PlayerToNormalContainerSimpleStackProof;
		Action.P59PlayerToNormalContainerStandardEquipmentProof = Payload.P59PlayerToNormalContainerStandardEquipmentProof;
		Action.P60PlayerToNormalContainerSpatialGraphProof = Payload.P60PlayerToNormalContainerSpatialGraphProof;
		Action.P47NormalContainerSpatialGraphProof = Payload.P47NormalContainerSpatialGraphProof;
		Action.P48NormalContainerSpatialGraphEquipmentProof = Payload.P48NormalContainerSpatialGraphEquipmentProof;
		Action.P41BodySpatialGraphProof = Payload.P41BodySpatialGraphProof;
		Action.P42BodySpatialGraphEquipmentProof = Payload.P42BodySpatialGraphEquipmentProof;
		return Controller.CommitSourceAction(Action);
	}

	bool FCodeBP4InteractionController::CommitDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target)
	{
		return CommitPreview(Payload, Target, PreviewDrop(Payload, Target));
	}

	void FCodeBP4InteractionController::CancelInteraction(const FString& Reason)
	{
		Controller.SetP4Feedback(Reason);
	}

	FString FCodeBP4InteractionController::GetDropKindLabel(const ECodeBP4DropKind Kind)
	{
		switch (Kind)
		{
		case ECodeBP4DropKind::Split: return TEXT("拆分");
		case ECodeBP4DropKind::Move: return TEXT("移动");
		case ECodeBP4DropKind::Merge: return TEXT("合并");
		case ECodeBP4DropKind::Swap: return TEXT("交换");
		case ECodeBP4DropKind::Equip: return TEXT("装备");
		case ECodeBP4DropKind::Replacement: return TEXT("装备替换");
		case ECodeBP4DropKind::Unequip: return TEXT("卸下");
		default: return TEXT("拒绝");
		}
	}
}
