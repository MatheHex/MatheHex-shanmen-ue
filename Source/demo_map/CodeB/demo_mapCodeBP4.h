// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CodeB/demo_mapCodeBP3.h"

namespace demo_map_code_b
{
	/** Pure, stable drag description.  It deliberately has no widget, repository, container or item-instance reference. */
	struct FCodeBP4DragPayload
	{
		FCodeBP3SlotAddress Source;
		FGuid ItemId;
		int32 ExpectedRevision = INDEX_NONE;
		FName DefinitionId;
		int32 Quantity = 0;
		int32 Quality = 0;
		uint32 SessionId = 0;

		bool IsValid() const { return Source.IsValid() && ItemId.IsValid() && ExpectedRevision != INDEX_NONE && SessionId != 0; }
	};

	enum class ECodeBP4DropKind : uint8
	{
		Reject,
		Move,
		Merge,
		Swap,
		Equip,
		Replacement,
		Unequip
	};

	/** Read-only prediction only.  P2/P1 revalidate all of its suggested operations on commit. */
	struct FCodeBP4DropPreview
	{
		bool bAllowed = false;
		ECodeBP4DropKind Kind = ECodeBP4DropKind::Reject;
		FString Message;
		ECodeBOperation Operation = ECodeBOperation::Move;
		int32 Quantity = 0;
	};

	/**
	 * P4's non-authoritative gesture planner.  It reads only the P2 projection and can only submit through the
	 * P3 controller bridge, which in turn calls the P2 application service and P1 transaction core.
	 */
	class FCodeBP4InteractionController
	{
	public:
		explicit FCodeBP4InteractionController(FCodeBP3UIController& InController)
			: Controller(InController)
		{
		}

		bool BeginDrag(const FCodeBP3SlotAddress& Source, FCodeBP4DragPayload& OutPayload);
		FCodeBP4DropPreview PreviewDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target) const;
		bool CommitDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target);
		void CancelInteraction(const FString& Reason = TEXT("已取消拖拽，未写入任何物品状态"));

		static FString GetDropKindLabel(ECodeBP4DropKind Kind);

	private:
		const FCodeBP2ContainerView* FindContainer(const FGuid& ContainerId) const;
		const FCodeBP2SlotView* FindSlot(const FCodeBP3SlotAddress& Address) const;
		bool IsEquipmentContainer(const FGuid& ContainerId) const;
		bool IsLoadedSpatialItem(const FCodeBP4DragPayload& Payload) const;
		bool IsCompatibleEquipmentTarget(const FCodeBP2SlotView& Source, const FGuid& TargetContainerId) const;
		static int32 GetMaxStack(const FName& DefinitionId);
		static FCodeBP4DropPreview Reject(const FString& Message);
		bool CommitPreview(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target, const FCodeBP4DropPreview& Preview);

		FCodeBP3UIController& Controller;
		uint32 NextSessionId = 1;
	};
}
