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
		/** P29: set only by the shared Ctrl+left router for this one transient gesture. */
		bool bQuickTransferIntent = false;
		/** P29/P35: exact active P17 player child at gesture time; empty means no child proof. */
		FGuid QuickTransferActivePlayerContainerId;
		/** P35: exact transient activation generation paired with the active child identity. */
		uint32 ActivePlayerChildOpenGeneration = 0;
		/** P36/P37: target family frozen at Ctrl+left input; Legacy preserves P29/P30. */
		ECodeBQuickTransferTargetMode QuickTransferTargetMode = ECodeBQuickTransferTargetMode::Legacy;
		/** P36/P37: exact P17 spatial parent identity paired with CurrentP17Child mode. */
		FGuid QuickTransferActivePlayerParentItemId;
		/** P38/P39: exact P21 body-source and input-time current P17-child proof. */
		FCodeBP38BodyEquipmentTransferProof P38BodyEquipmentProof;
		/** P40: exact revealed ordinary P12 simple-stack and frozen-target proof. */
		FCodeBP40BodySimpleStackQuickTransferProof P40BodySimpleStackProof;
		/** P41: exact revealed ordinary P20 spatial graph and frozen BaseQuick target. */
		FCodeBP41BodySpatialGraphQuickTransferProof P41BodySpatialGraphProof;
		/** P42: normal Drag source proof plus the exact user-selected formal P6 target. */
		FCodeBP42BodySpatialGraphEquipmentTransferProof P42BodySpatialGraphEquipmentProof;
		/** Explicit P24 player split versus P27 world partial-pickup intent. */
		ECodeBP3QuantityDraftKind QuantityDraftKind = ECodeBP3QuantityDraftKind::None;
		/** P27 transient record identity. It is never used to derive an ItemId. */
		FGuid WorldDropId;
		/** P31 exact opened-record lifecycle identity; transient and never authoritative. */
		int32 WorldDropOrdinal = 0;
		int32 WorldDropRecordRevision = INDEX_NONE;
		uint32 WorldDropTargetOpenGeneration = 0;
		FName WorldDropMapRoute = NAME_None;
		ECodeBP3InventoryScope SourceScope = ECodeBP3InventoryScope::Unknown;
		FGuid OwnerId;
		FGuid RunInstanceId;
		/** Stable warehouse/run/external graph identity captured by a confirmed quantity draft. */
		FGuid GraphIdentity;
		uint32 SessionId = 0;

		bool IsValid() const
		{
			return Source.IsValid() && ItemId.IsValid() && ExpectedRevision != INDEX_NONE && SessionId != 0
				&& Quantity > 0 && (!bSplitIntent || (QuantityDraftKind != ECodeBP3QuantityDraftKind::None
					&& GraphIdentity.IsValid() && RequestedMergeQuantity > 0 && RequestedMergeQuantity < Quantity
					&& (QuantityDraftKind != ECodeBP3QuantityDraftKind::WorldPickup
						|| (WorldDropId.IsValid() && WorldDropOrdinal > 0
							&& WorldDropRecordRevision > 0 && WorldDropTargetOpenGeneration != 0
							&& !WorldDropMapRoute.IsNone()))));
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
		bool BeginSplitDrag(
			const FCodeBP3SlotAddress& Source,
			int32 RequestedQuantity,
			FCodeBP4DragPayload& OutPayload,
			ECodeBP3QuantityDraftKind DraftKind = ECodeBP3QuantityDraftKind::PlayerSplit);
		FCodeBP4DropPreview PreviewDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target) const;
		bool CommitDrop(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target);
		void CancelInteraction(const FString& Reason = TEXT("已取消拖拽，未写入任何物品状态"));

		static FString GetDropKindLabel(ECodeBP4DropKind Kind);

	private:
		const FCodeBP2ContainerView* FindContainer(const FGuid& ContainerId) const;
		const FCodeBP2SlotView* FindSlot(const FCodeBP3SlotAddress& Address) const;
		bool IsEquipmentContainer(const FGuid& ContainerId) const;
		bool IsP21BodyEquipmentContainer(const FGuid& ContainerId) const;
		bool IsCompatibleEquipmentTarget(const FCodeBP2SlotView& Source, const FGuid& TargetContainerId) const;
		static FCodeBP4DropPreview Reject(const FString& Message);
		bool CommitPreview(const FCodeBP4DragPayload& Payload, const FCodeBP3SlotAddress& Target, const FCodeBP4DropPreview& Preview);

		FCodeBP3UIController& Controller;
		uint32 NextSessionId = 1;
	};
}
