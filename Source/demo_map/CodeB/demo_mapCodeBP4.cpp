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

	int32 FCodeBP4InteractionController::GetMaxStack(const FName& DefinitionId)
	{
		if (const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId))
		{
			return Definition->MaxStackSize;
		}
		if (DefinitionId == FName(TEXT("Material.Dust"))) return 20;
		if (DefinitionId == FName(TEXT("Consumable.Potion"))) return 10;
		return 1;
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
		OutPayload.Quality = SourceSlot->Quality;
		OutPayload.SourceScope = ECodeBP3InventoryScope::Unknown;
		OutPayload.SessionId = NextSessionId++;
		Controller.SetP4Feedback(FString::Printf(TEXT("正在拖拽 %s；放下时才会提交事务"), *SourceSlot->DefinitionId.ToString()));
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
		if (!SourceSlot || !TargetSlot || !SourceSlot->bOccupied || SourceSlot->ItemId != Payload.ItemId)
		{
			return Reject(TEXT("来源或目标已变化，请重新操作"));
		}
		const bool bSourceEquipment = IsEquipmentContainer(Payload.Source.ContainerId);
		const bool bTargetEquipment = IsEquipmentContainer(Target.ContainerId);
		FCodeBP4DropPreview Preview;
		Preview.bAllowed = true;
		Preview.Quantity = Payload.Quantity;
		if (bTargetEquipment)
		{
			if (bSourceEquipment) return Reject(TEXT("装备栏之间不能直接拖拽"));
			if (!IsCompatibleEquipmentTarget(*SourceSlot, Target.ContainerId)) return Reject(TEXT("物品类型与目标栏位不匹配"));
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
			Preview.Message = TEXT("可移动到空储物格");
			return Preview;
		}
		if (SourceSlot->DefinitionId == TargetSlot->DefinitionId && GetMaxStack(SourceSlot->DefinitionId) > 1)
		{
			const int32 Available = GetMaxStack(SourceSlot->DefinitionId) - TargetSlot->Quantity;
			if (Available < Payload.Quantity) return Reject(TEXT("目标堆叠空间不足；请使用拆分后合并"));
			Preview.Kind = ECodeBP4DropKind::Merge;
			Preview.Operation = ECodeBOperation::Merge;
			Preview.Message = TEXT("可合并完整堆叠");
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
		return Controller.CommitP4Operation(Preview.Operation, Payload.Source, Target, Payload.ExpectedRevision, Preview.Quantity, GetDropKindLabel(Preview.Kind));
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
