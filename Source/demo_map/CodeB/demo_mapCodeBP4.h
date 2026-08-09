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
		/** Authoritative source quantity at drag start; never a writable copy. */
		int32 Quantity = 0;
		/** P25's one-shot exact merge/split quantity; zero for a normal full-stack drag. */
		int32 RequestedMergeQuantity = 0;
		int32 Quality = 0;
		bool bSplitIntent = false;
		ECodeBP3InventoryScope SourceScope = ECodeBP3InventoryScope::Unknown;
		FGuid OwnerId;
		FGuid RunInstanceId;
		/** Stable warehouse/run/external graph identity captured by a confirmed quantity draft. */
		FGuid GraphIdentity;
		uint32 SessionId = 0;

		bool IsValid() const
		{
			return Source.IsValid() && ItemId.IsValid() && ExpectedRevision != INDEX_NONE && SessionId != 0
				&& Quantity > 0 && (!bSplitIntent || (GraphIdentity.IsValid()
					&& RequestedMergeQuantity > 0 && RequestedMergeQuantity < Quantity));
		}
	};

	enum class ECodeBP4DropKind : uint8
	{
		Reject,
		Split,
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
		/** Zero delegates a normal full-stack merge amount to P1; positive is an exact requested amount. */
		int32 Quantity = 0;
		/** Transient read-only preview of the amount P1 can accept from the current snapshot. */
		int32 ProjectedAcceptedQuantity = 0;
		bool bPartialAcceptance = false;
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
		/** Consumes a confirmed P24 draft into a one-shot split drag descriptor. */
		bool BeginSplitDrag(const FCodeBP3SlotAddress& Source, int32 RequestedQuantity, FCodeBP4DragPayload& OutPayload);
		FCodeBP4DropPreview PreviewDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target) const;
		bool CommitDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target);
		void CancelInteraction(const FString& Reason = TEXT("已取消拖拽，未写入任何物品状态"));

		static FString GetDropKindLabel(ECodeBP4DropKind Kind);

	private:
		const FCodeBP2ContainerView* FindContainer(const FGuid& ContainerId) const;
		const FCodeBP2SlotView* FindSlot(const FCodeBP3SlotAddress& Address) const;
		bool IsEquipmentContainer(const FGuid& ContainerId) const;
		bool IsCompatibleEquipmentTarget(const FCodeBP2SlotView& Source, const FGuid& TargetContainerId) const;
		static FCodeBP4DropPreview Reject(const FString& Message);
		bool CommitPreview(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target, const FCodeBP4DropPreview& Preview);

		FCodeBP3UIController& Controller;
		uint32 NextSessionId = 1;
	};
}
