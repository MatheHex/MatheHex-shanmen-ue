#pragma once

#include "CodeB/demo_mapCodeBInventory.h"

namespace demo_map_code_b
{
	/** Application-layer result codes; P1 remains the source of transaction legality. */
	enum class ECodeBP2ResultCode : uint8
	{
		Success,
		FixtureInvalid,
		InvalidCommand,
		ProjectionFailure,
		P1Failure
	};

	struct FCodeBP2PlayerLayout
	{
		FName LayoutId;
		FGuid WarehouseContainerId;
		FGuid BasicContainerId;
		FGuid WeaponContainerId;
		FGuid ArmorContainerId;
		FGuid SpatialContainerId;
		/** Optional formal Profile backpack slot.  The accepted fixture leaves it unset. */
		FGuid BackpackContainerId;
		TArray<FGuid> AccessoryContainerIds;
		/** Child storage exposed only while the equipped spatial ring is present. */
		FGuid SpatialInternalContainerId;
		/** Child storage owned by the ordinary non-quick spatial pouch fixture. */
		FGuid PouchInternalContainerId;
		/**
		 * P5 Profile layouts hide the ring child until its owner is equipped and
		 * only expose the pouch child while a matching spatial item exists.  The
		 * development fixture keeps its accepted fixed projection contract.
		 */
		bool bUseConditionalSpatialContainers = false;
		/** P10-only, non-persisted presentation references. They never become P5/P6 layout truth. */
		TArray<TPair<FName, FGuid>> TransientPresentationContainers;

		TArray<TPair<FName, FGuid>> GetOrderedContainers() const;
	};

	struct FCodeBP2FixtureIds
	{
		FGuid WarehouseContainerId;
		FGuid BasicContainerId;
		FGuid WeaponContainerId;
		FGuid ArmorContainerId;
		FGuid SpatialContainerId;
		FGuid Accessory0ContainerId;
		FGuid Accessory1ContainerId;
		/** The dedicated equipment container for a Spatial.Ring. */
		FGuid SpatialInternalContainerId;
		/** An ordinary storage child container belonging to Spatial.Pouch. */
		FGuid PouchInternalContainerId;

		FGuid WeaponAItemId;
		FGuid WeaponBItemId;
		FGuid ArmorItemId;
		FGuid AccessoryAItemId;
		FGuid AccessoryBItemId;
		/** Compatibility field retained for existing P1--P4 tests; it is now Spatial.Ring. */
		FGuid SpatialItemId;
		/** Ordinary, non-equipable spatial storage pouch. */
		FGuid SpatialPouchItemId;
		FGuid DustAItemId;
		FGuid DustBItemId;
		FGuid PotionItemId;
		FGuid InvalidEquipItemId;
	};

	struct FCodeBP2SlotView
	{
		FName SlotId;
		int32 SlotIndex = INDEX_NONE;
		bool bOccupied = false;
		FGuid ItemId;
		FName DefinitionId;
		int32 Quantity = 0;
		/** Read-only P1 stack metadata used by the shared P4 planner. */
		bool bStackable = false;
		int32 MaxStack = 1;
		int32 Level = 0;
		int32 Quality = 0;
		int32 RandomSeed = 0;
		/** Immutable P1 instance metadata; projected read-only for identity-bound transfers. */
		FString LegacyAffixDigest;
		ECodeBItemType ItemType = ECodeBItemType::Generic;
		/** Projection-only Code B definition metadata; no UI cache is a hotbar truth. */
		bool bQuickUsable = false;
		/** Read-only P1 definition semantic used by P4 to preview the exact equipment target. */
		ECodeBEquipSlot EquipSlot = ECodeBEquipSlot::None;
		/** Projection-only parent/child relation used by P4 to retain whole-graph identity without reading P1. */
		FGuid ChildContainerId;

		bool operator==(const FCodeBP2SlotView& Other) const;
	};

	struct FCodeBP2ContainerView
	{
		FName Role;
		FGuid ContainerId;
		int32 Capacity = 0;
		TArray<FCodeBP2SlotView> Slots;

		bool operator==(const FCodeBP2ContainerView& Other) const;
	};

	struct FCodeBP2Projection
	{
		int32 Revision = 0;
		FName LayoutId;
		FGuid WarehouseContainerId;
		FGuid BasicContainerId;
		TArray<FCodeBP2ContainerView> Containers;
		FGuid LastTransactionId;
		TArray<FGuid> AffectedItemIds;
		TArray<FGuid> AffectedContainerIds;

		bool operator==(const FCodeBP2Projection& Other) const;
		bool operator!=(const FCodeBP2Projection& Other) const { return !(*this == Other); }
	};

	/** Transient audit identity for the one P2 command; never persisted as item state. */
	enum class ECodeBP2CommandIntent : uint8
	{
		Standard,
		QuickTransfer
	};

	/** P36/P37 freeze the one automatic target family at pointer-down time. Legacy remains for P29/P30. */
	enum class ECodeBQuickTransferTargetMode : uint8
	{
		Legacy,
		CurrentP17Child,
		BaseQuickNoChildAtInput,
		/** P41 complete spatial graphs never target a P17 child, even when one is open. */
		BaseQuickOnly
	};

	/**
	 * P38/P39/P44 transient proof for one normal drag, frozen-target QuickTransfer,
	 * or normal GroundDrop
	 * from an exact P21 corpse-equipment
	 * slot. It carries only immutable identities/lifecycle revisions into the
	 * existing P3 -> P2 -> P1 -> P11/P6 commit callback; it is never persisted as
	 * item or container truth.
	 */
	struct FCodeBP38BodyEquipmentTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid BodyTargetId;
		FGuid DeathReceiptId;
		int32 BodyRecordRevision = INDEX_NONE;
		uint32 BodyTargetOpenGeneration = 0;
		FName BodyDefinitionId;
		FName SourceSlotSemantic;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootResultDigest;
		FString MaterializationDigest;
		FString EquipmentCandidateSetDigest;
		FName WorkspaceTargetPaneId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		int32 CompositeRevision = INDEX_NONE;
		FGuid ActivePlayerChildContainerId;
		FGuid ActivePlayerChildParentItemId;
		uint32 ActivePlayerChildOpenGeneration = 0;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& BodyTargetId.IsValid() && DeathReceiptId.IsValid()
				&& BodyRecordRevision > 0 && BodyTargetOpenGeneration != 0
				&& !BodyDefinitionId.IsNone() && !SourceSlotSemantic.IsNone()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootResultDigest.IsEmpty()
				&& !MaterializationDigest.IsEmpty() && !EquipmentCandidateSetDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot == 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& CompositeRevision >= 0;
		}
	};

	/**
	 * P40 transient proof for one revealed ordinary P12 simple-stack source.
	 * It freezes only durable body/source identities and the input-time P17 target
	 * identity; P1 remains the sole placement and quantity authority.
	 */
	struct FCodeBP40BodySimpleStackQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid BodyTargetId;
		FGuid DeathReceiptId;
		int32 BodyRecordRevision = INDEX_NONE;
		uint32 BodyTargetOpenGeneration = 0;
		FName BodyDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		FGuid ActivePlayerChildContainerId;
		FGuid ActivePlayerChildParentItemId;
		uint32 ActivePlayerChildOpenGeneration = 0;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& BodyTargetId.IsValid() && DeathReceiptId.IsValid()
				&& BodyRecordRevision > 0 && BodyTargetOpenGeneration != 0
				&& !BodyDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootResultDigest.IsEmpty()
				&& !MaterializationDigest.IsEmpty() && !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0;
		}
	};

	/**
	 * P46 transient proof for one exact opened/revealed P10 BasicCache simple
	 * stack.  The source receipt and the input-time P17/P6 target choice are
	 * frozen here; P1 remains the sole placement and quantity authority.
	 */
	struct FCodeBP46NormalContainerSimpleStackQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		FGuid ActivePlayerChildContainerId;
		FGuid ActivePlayerChildParentItemId;
		uint32 ActivePlayerChildOpenGeneration = 0;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootResultDigest.IsEmpty()
				&& !MaterializationDigest.IsEmpty() && !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}
	};

	/**
	 * P62 transient proof for one player-deposited canonical non-spatial
	 * standard-equipment root in the exact opened BasicCache.  It records the
	 * full immutable root identity and the input-time P17/P6 target choice; it
	 * is never persisted as a second provenance truth.
	 */
	struct FCodeBP62NormalContainerStandardEquipmentQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		ECodeBEquipSlot SourceEquipSlot = ECodeBEquipSlot::None;
		int32 SourceLevel = 0;
		int32 SourceQuality = 0;
		int32 SourceRandomSeed = 0;
		FString SourceLegacyAffixDigest;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		FGuid ActivePlayerChildContainerId;
		FGuid ActivePlayerChildParentItemId;
		uint32 ActivePlayerChildOpenGeneration = 0;
		FGuid FrozenTargetContainerId;
		int32 FrozenTargetSlot = INDEX_NONE;
		int32 FrozenTargetCapacity = 0;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceEquipSlot != ECodeBEquipSlot::None && SourceLevel > 0
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return HasSourceIdentity() && FrozenTargetContainerId.IsValid()
				&& FrozenTargetSlot >= 0 && FrozenTargetCapacity > 0;
		}
	};

	/**
	 * P63 transient proof for one player-deposited generic simple stack in the
	 * exact opened BasicCache.  It is intentionally disjoint from P46's
	 * durable-materialization family and freezes one merge-first / real-empty-
	 * second P6 target at input time.
	 */
	struct FCodeBP63NormalContainerPlayerSimpleStackQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FName SourceStackKey;
		int32 SourceQuantity = 0;
		int32 SourceMaxStack = 0;
		ECodeBItemType SourceItemType = ECodeBItemType::Material;
		ECodeBEquipSlot SourceEquipSlot = ECodeBEquipSlot::None;
		int32 SourceLevel = 0;
		int32 SourceQuality = 0;
		int32 SourceRandomSeed = 0;
		FString SourceLegacyAffixDigest;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		FGuid ActivePlayerChildContainerId;
		FGuid ActivePlayerChildParentItemId;
		uint32 ActivePlayerChildOpenGeneration = 0;
		FGuid FrozenTargetContainerId;
		int32 FrozenTargetSlot = INDEX_NONE;
		int32 FrozenTargetCapacity = 0;
		bool bFrozenMergeCandidate = false;
		FGuid FrozenTargetItemId;
		int32 FrozenTargetQuantity = 0;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& !SourceStackKey.IsNone() && SourceQuantity > 0 && SourceMaxStack > 0
				&& SourceEquipSlot == ECodeBEquipSlot::None
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return HasSourceIdentity() && FrozenTargetContainerId.IsValid()
				&& FrozenTargetSlot >= 0 && FrozenTargetCapacity > 0
				&& (bFrozenMergeCandidate
					? FrozenTargetItemId.IsValid() && FrozenTargetQuantity > 0
					: !FrozenTargetItemId.IsValid() && FrozenTargetQuantity == 0);
		}
	};

	/**
	 * P58 transient proof for one player-side ordinary simple stack deposited into
	 * the exact opened P10 BasicCache.  It freezes both the P6 source mode and the
	 * one visible merge-first / real-empty-second P9 candidate selected at input.
	 */
	struct FCodeBP58PlayerSimpleStackToNormalContainerQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid TargetContainerId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FName SourceStackKey;
		int32 SourceQuantity = 0;
		int32 SourceMaxStack = 0;
		bool bSourceIsBaseQuick = false;
		FGuid SourceActiveChildContainerId;
		FGuid SourceActiveChildParentItemId;
		uint32 SourceActiveChildOpenGeneration = 0;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		int32 FrozenTargetSlot = INDEX_NONE;
		bool bFrozenMergeCandidate = false;
		FGuid FrozenTargetItemId;
		int32 FrozenTargetQuantity = 0;

		bool HasSourceIdentity() const
		{
			const bool bValidSourceMode = bSourceIsBaseQuick
				? (!SourceActiveChildContainerId.IsValid()
					&& !SourceActiveChildParentItemId.IsValid()
					&& SourceActiveChildOpenGeneration == 0)
				: (SourceActiveChildContainerId == SourceContainerId
					&& SourceActiveChildParentItemId.IsValid()
					&& SourceActiveChildOpenGeneration != 0);
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && TargetContainerId.IsValid()
				&& SourceContainerId.IsValid() && SourceSlot >= 0 && SourceItemId.IsValid()
				&& !SourceDefinitionId.IsNone() && !SourceStackKey.IsNone()
				&& SourceQuantity > 0 && SourceMaxStack > 1 && bValidSourceMode
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return HasSourceIdentity() && FrozenTargetSlot >= 0
				&& (bFrozenMergeCandidate
					? FrozenTargetItemId.IsValid() && FrozenTargetQuantity > 0
					: !FrozenTargetItemId.IsValid() && FrozenTargetQuantity == 0);
		}
	};

	/**
	 * P59 transient proof for one canonical standard-equipment root deposited from
	 * an exact P6 equipment/BaseQuick/current-P17-child source into the exact
	 * opened BasicCache. The first real empty target is frozen once at input.
	 */
	struct FCodeBP59PlayerStandardEquipmentToNormalContainerQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid TargetContainerId;
		FGuid SourceContainerId;
		FName SourceContainerRole;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		ECodeBEquipSlot SourceEquipSlot = ECodeBEquipSlot::None;
		int32 SourceLevel = 0;
		int32 SourceQuality = 0;
		int32 SourceRandomSeed = 0;
		FString SourceLegacyAffixDigest;
		bool bSourceIsFormalEquipment = false;
		bool bSourceIsBaseQuick = false;
		FGuid SourceActiveChildContainerId;
		FGuid SourceActiveChildParentItemId;
		uint32 SourceActiveChildOpenGeneration = 0;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		int32 FrozenTargetSlot = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			const bool bFormalMode = bSourceIsFormalEquipment && !bSourceIsBaseQuick
				&& !SourceActiveChildContainerId.IsValid()
				&& !SourceActiveChildParentItemId.IsValid()
				&& SourceActiveChildOpenGeneration == 0;
			const bool bBaseQuickMode = !bSourceIsFormalEquipment && bSourceIsBaseQuick
				&& !SourceActiveChildContainerId.IsValid()
				&& !SourceActiveChildParentItemId.IsValid()
				&& SourceActiveChildOpenGeneration == 0;
			const bool bCurrentChildMode = !bSourceIsFormalEquipment && !bSourceIsBaseQuick
				&& SourceActiveChildContainerId == SourceContainerId
				&& SourceActiveChildParentItemId.IsValid()
				&& SourceActiveChildOpenGeneration != 0;
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && TargetContainerId.IsValid()
				&& SourceContainerId.IsValid() && !SourceContainerRole.IsNone()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceEquipSlot != ECodeBEquipSlot::None && SourceLevel > 0
				&& (bFormalMode || bBaseQuickMode || bCurrentChildMode)
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return HasSourceIdentity() && FrozenTargetSlot >= 0;
		}
	};

	/**
	 * P60 transient proof for one canonical P19 complete spatial graph deposited
	 * from P6 BaseQuick or its exact formal equipment slot into the currently
	 * opened BasicCache.  Source closure and the first real empty target are
	 * frozen once at input; a P17 child cell is never a source.
	 */
	struct FCodeBP60PlayerSpatialGraphToNormalContainerQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid TargetContainerId;
		FGuid SourceContainerId;
		FName SourceContainerRole;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		ECodeBEquipSlot SourceEquipSlot = ECodeBEquipSlot::None;
		FGuid SourceChildContainerId;
		FGuid StableSpatialChildGuid;
		int32 SourceChildCapacity = 0;
		FString SourceClosureDigest;
		bool bSourceIsFormalEquipment = false;
		bool bSourceIsBaseQuick = false;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		int32 FrozenTargetSlot = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			const bool bFormalMode = bSourceIsFormalEquipment && !bSourceIsBaseQuick;
			const bool bBaseQuickMode = !bSourceIsFormalEquipment && bSourceIsBaseQuick;
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && TargetContainerId.IsValid()
				&& SourceContainerId.IsValid() && !SourceContainerRole.IsNone()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceEquipSlot != ECodeBEquipSlot::None
				&& SourceChildContainerId.IsValid() && StableSpatialChildGuid.IsValid()
				&& SourceChildContainerId == StableSpatialChildGuid
				&& SourceChildCapacity > 0 && !SourceClosureDigest.IsEmpty()
				&& (bFormalMode || bBaseQuickMode)
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return HasSourceIdentity() && FrozenTargetSlot >= 0;
		}
	};

	/**
	 * P49 transient proof for one normal Drag of an exact opened/revealed P10
	 * BasicCache ordinary simple-stack root to the existing GroundDropZone.
	 * It freezes only the P9 source and receipt identity; WorldDrop identity and
	 * ordinal remain Store-derived after durable revalidation.
	 */
	struct FCodeBP49NormalContainerSimpleStackGroundDropProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FName SourceStackKey;
		int32 SourceQuantity = 0;
		int32 SourceMaxStack = 0;
		ECodeBItemType SourceItemType = ECodeBItemType::Material;
		ECodeBEquipSlot SourceEquipSlot = ECodeBEquipSlot::None;
		int32 SourceLevel = 0;
		int32 SourceQuality = 0;
		int32 SourceRandomSeed = 0;
		FString SourceLegacyAffixDigest;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& !SourceStackKey.IsNone() && SourceQuantity > 0 && SourceMaxStack > 1
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}
	};

	/**
	 * P64 transient proof for one player-deposited generic simple stack in the
	 * exact opened BasicCache, dragged normally to GroundDrop.  The dedicated
	 * family stays disjoint from P49's two materialization definitions while
	 * preserving the complete root metadata needed for durable revalidation.
	 */
	struct FCodeBP64NormalContainerPlayerSimpleStackGroundDropProof
		: FCodeBP49NormalContainerSimpleStackGroundDropProof
	{
		bool HasSourceIdentity() const
		{
			return FCodeBP49NormalContainerSimpleStackGroundDropProof::HasSourceIdentity()
				&& SourceEquipSlot == ECodeBEquipSlot::None;
		}
	};

	/**
	 * P65 transient proof for one unclaimed player-deposited canonical standard
	 * non-spatial root in the exact opened BasicCache, dragged normally to the
	 * existing GroundDropZone. The shared P49 identity surface is reused only as
	 * immutable source metadata; the dedicated shape gate keeps P49/P64/P65
	 * mutually exclusive.
	 */
	struct FCodeBP65NormalContainerPlayerStandardEquipmentGroundDropProof
		: FCodeBP49NormalContainerSimpleStackGroundDropProof
	{
		bool HasSourceIdentity() const
		{
			return FCodeBP49NormalContainerSimpleStackGroundDropProof::HasSourceIdentity()
				&& SourceQuantity == 1 && SourceMaxStack == 1
				&& SourceEquipSlot != ECodeBEquipSlot::None;
		}
	};

	/**
	 * P47/P55/P61 transient proof for one exact opened/revealed BasicCache
	 * spatial parent. It preserves the two receipt-claimed materialization
	 * families and adds only the receipt-derived player-deposited family; the
	 * one input-time BaseQuick candidate and P1 remain authoritative.
	 */
	struct FCodeBP47NormalContainerSpatialGraphQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FGuid SourceChildContainerId;
		FGuid StableSpatialChildGuid;
		bool bPlayerDepositedSource = false;
		int32 SourceChildCapacity = 0;
		FString SourceClosureDigest;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		FGuid BaseQuickContainerId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		FGuid FrozenTargetContainerId;
		int32 FrozenTargetSlot = INDEX_NONE;
		int32 FrozenTargetCompositeRevision = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceChildContainerId.IsValid() && StableSpatialChildGuid.IsValid()
				&& SourceChildContainerId == StableSpatialChildGuid
				&& (!bPlayerDepositedSource
					|| (SourceChildCapacity > 0 && !SourceClosureDigest.IsEmpty()))
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone() && BaseQuickContainerId.IsValid()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return FrozenTargetContainerId.IsValid() && FrozenTargetSlot >= 0
				&& FrozenTargetCompositeRevision >= 0;
		}
	};

	/**
	 * P48/P66 transient proof for one normal P10 Drag of a revealed canonical
	 * spatial graph. P48 freezes an explicit formal-equipment target; P50/P56/P66
	 * leave the target empty for GroundDrop. Player-deposited graphs carry their
	 * exact closure identity so the durable writer can reject source-family drift.
	 */
	struct FCodeBP48NormalContainerSpatialGraphEquipmentTransferProof
	{
		bool bSourceProof = false;
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid SearchTargetId;
		FGuid ReceiptId;
		int32 NormalContainerRevision = INDEX_NONE;
		uint32 TargetOpenGeneration = 0;
		FName NormalContainerDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FGuid SourceChildContainerId;
		FGuid StableSpatialChildGuid;
		bool bPlayerDepositedSource = false;
		int32 SourceChildCapacity = 0;
		FString SourceClosureDigest;
		int32 DefinitionContentRevision = 0;
		FString DefinitionDigest;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootAlgorithmVersion;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		int32 P6SnapshotRevision = INDEX_NONE;
		FGuid FrozenTargetContainerId;
		int32 FrozenTargetSlot = INDEX_NONE;
		FName FrozenTargetSlotSemantic;
		int32 FrozenTargetCompositeRevision = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			return bSourceProof && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& SearchTargetId.IsValid() && ReceiptId.IsValid()
				&& NormalContainerRevision > 0 && TargetOpenGeneration != 0
				&& !NormalContainerDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceChildContainerId.IsValid() && StableSpatialChildGuid.IsValid()
				&& SourceChildContainerId == StableSpatialChildGuid
				&& (!bPlayerDepositedSource
					|| (SourceChildCapacity > 0 && !SourceClosureDigest.IsEmpty()))
				&& DefinitionContentRevision > 0 && !DefinitionDigest.IsEmpty()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootAlgorithmVersion.IsEmpty()
				&& !LootResultDigest.IsEmpty() && !MaterializationDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0 && P6SnapshotRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return bIntent && FrozenTargetContainerId.IsValid() && FrozenTargetSlot >= 0
				&& !FrozenTargetSlotSemantic.IsNone() && FrozenTargetCompositeRevision >= 0;
		}
	};

	/**
	 * P43/P69 transient proof for one normal Drag of an exact Revealed ordinary
	 * P12 simple-stack root, whole or confirmed split, to the existing GroundDropZone.
	 * It deliberately carries no player target, WorldDrop identity, or ordinal;
	 * the durable Store derives the new P31 identity only after revalidation.
	 */
	struct FCodeBP43BodySimpleStackGroundDropProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid BodyTargetId;
		FGuid DeathReceiptId;
		int32 BodyRecordRevision = INDEX_NONE;
		uint32 BodyTargetOpenGeneration = 0;
		FName BodyDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		int32 SourceQuantity = 0;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& BodyTargetId.IsValid() && DeathReceiptId.IsValid()
				&& BodyRecordRevision > 0 && BodyTargetOpenGeneration != 0
				&& !BodyDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceQuantity > 0 && !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootResultDigest.IsEmpty()
				&& !MaterializationDigest.IsEmpty() && !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0;
		}
	};

	/**
	 * P41/P52 transient proof for one revealed P20 empty or exact P51-loaded
	 * spatial parent in the exact opened P12 ordinary body root. It freezes the
	 * canonical source identity and input-time BaseQuick domain; durable Commit
	 * independently validates the receipt-bound full closure and may only
	 * revalidate Preview's one exact first-empty address.
	 */
	struct FCodeBP41BodySpatialGraphQuickTransferProof
	{
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid BodyTargetId;
		FGuid DeathReceiptId;
		int32 BodyRecordRevision = INDEX_NONE;
		uint32 BodyTargetOpenGeneration = 0;
		FName BodyDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FGuid SourceChildContainerId;
		/** P17's stable child identity is the canonical SpatialChildGuid for SourceItemId. */
		FGuid StableSpatialChildGuid;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		FGuid BaseQuickContainerId;
		int32 CompositeRevision = INDEX_NONE;
		FGuid FrozenTargetContainerId;
		int32 FrozenTargetSlot = INDEX_NONE;
		int32 FrozenTargetCompositeRevision = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			return bIntent && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& BodyTargetId.IsValid() && DeathReceiptId.IsValid()
				&& BodyRecordRevision > 0 && BodyTargetOpenGeneration != 0
				&& !BodyDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceChildContainerId.IsValid() && StableSpatialChildGuid.IsValid()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootResultDigest.IsEmpty()
				&& !MaterializationDigest.IsEmpty() && !WorkspaceTargetPaneId.IsNone()
				&& BaseQuickContainerId.IsValid() && CompositeRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return FrozenTargetContainerId.IsValid() && FrozenTargetSlot >= 0
				&& FrozenTargetCompositeRevision >= 0;
		}
	};

	/**
	 * P42 transient proof for one normal P12 Drag of a revealed P20 spatial
	 * graph into its exact empty P6 formal equipment slot.  Source identity is
	 * captured at drag start; the user's concrete drop address is frozen later.
	 */
	struct FCodeBP42BodySpatialGraphEquipmentTransferProof
	{
		bool bSourceProof = false;
		bool bIntent = false;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid BodyTargetId;
		FGuid DeathReceiptId;
		int32 BodyRecordRevision = INDEX_NONE;
		uint32 BodyTargetOpenGeneration = 0;
		FName BodyDefinitionId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid SourceItemId;
		FName SourceDefinitionId;
		FGuid SourceChildContainerId;
		FGuid StableSpatialChildGuid;
		FName LootProfileId;
		int32 LootProfileVersion = 0;
		FString LootProfileDigest;
		FString LootResultDigest;
		FString MaterializationDigest;
		FName WorkspaceTargetPaneId;
		int32 CompositeRevision = INDEX_NONE;
		FGuid FrozenTargetContainerId;
		int32 FrozenTargetSlot = INDEX_NONE;
		FName FrozenTargetSlotSemantic;
		int32 FrozenTargetCompositeRevision = INDEX_NONE;

		bool HasSourceIdentity() const
		{
			return bSourceProof && OwnerId.IsValid() && RunInstanceId.IsValid()
				&& BodyTargetId.IsValid() && DeathReceiptId.IsValid()
				&& BodyRecordRevision > 0 && BodyTargetOpenGeneration != 0
				&& !BodyDefinitionId.IsNone() && SourceContainerId.IsValid()
				&& SourceSlot >= 0 && SourceItemId.IsValid() && !SourceDefinitionId.IsNone()
				&& SourceChildContainerId.IsValid() && StableSpatialChildGuid.IsValid()
				&& !LootProfileId.IsNone() && LootProfileVersion > 0
				&& !LootProfileDigest.IsEmpty() && !LootResultDigest.IsEmpty()
				&& !MaterializationDigest.IsEmpty() && !WorkspaceTargetPaneId.IsNone()
				&& CompositeRevision >= 0;
		}

		bool HasFrozenTarget() const
		{
			return bIntent && FrozenTargetContainerId.IsValid() && FrozenTargetSlot >= 0
				&& !FrozenTargetSlotSemantic.IsNone() && FrozenTargetCompositeRevision >= 0;
		}
	};

	struct FCodeBP2Command
	{
		FGuid TransactionId;
		ECodeBOperation Operation = ECodeBOperation::Move;
		FGuid ItemId;
		FGuid SourceContainerId;
		int32 SourceSlot = INDEX_NONE;
		FGuid TargetContainerId;
		int32 TargetSlot = INDEX_NONE;
		int32 Quantity = 0;
		int32 ExpectedRevision = INDEX_NONE;
		ECodeBP2CommandIntent Intent = ECodeBP2CommandIntent::Standard;
		/** P29/P35 transient proof of the exact P17 child selected by the shared workspace. */
		FGuid QuickTransferActivePlayerContainerId;
		/** P35 rejects a drag captured before the current child was reopened or switched. */
		uint32 ActivePlayerChildOpenGeneration = 0;
		/** P36/P37 one-shot target decision; never persisted and never recomputed at commit. */
		ECodeBQuickTransferTargetMode QuickTransferTargetMode = ECodeBQuickTransferTargetMode::Legacy;
		/** P36/P37 exact canonical spatial parent paired with CurrentP17Child mode. */
		FGuid QuickTransferActivePlayerParentItemId;
		/** P38/P39 body-equipment source/target lifecycle proof; absent for every earlier family. */
		FCodeBP38BodyEquipmentTransferProof P38BodyEquipmentProof;
		/** P40 ordinary P12 simple-stack source and frozen-target proof. */
		FCodeBP40BodySimpleStackQuickTransferProof P40BodySimpleStackProof;
		/** P46 opened/revealed P10 BasicCache simple-stack and frozen-target proof. */
		FCodeBP46NormalContainerSimpleStackQuickTransferProof P46NormalContainerSimpleStackProof;
		/** P62 player-deposited BasicCache standard-equipment and frozen-target proof. */
		FCodeBP62NormalContainerStandardEquipmentQuickTransferProof P62NormalContainerStandardEquipmentProof;
		FCodeBP63NormalContainerPlayerSimpleStackQuickTransferProof P63NormalContainerPlayerSimpleStackProof;
		/** P58 player ordinary simple-stack and exact opened BasicCache target proof. */
		FCodeBP58PlayerSimpleStackToNormalContainerQuickTransferProof P58PlayerToNormalContainerSimpleStackProof;
		/** P59 player canonical standard-equipment and exact opened BasicCache target proof. */
		FCodeBP59PlayerStandardEquipmentToNormalContainerQuickTransferProof P59PlayerToNormalContainerStandardEquipmentProof;
		/** P60 player canonical complete spatial graph and exact opened BasicCache target. */
		FCodeBP60PlayerSpatialGraphToNormalContainerQuickTransferProof P60PlayerToNormalContainerSpatialGraphProof;
		/** P47 opened/revealed P10 BasicCache spatial graph and frozen BaseQuick proof. */
		FCodeBP47NormalContainerSpatialGraphQuickTransferProof P47NormalContainerSpatialGraphProof;
		/** P48 opened/revealed P10 BasicCache spatial graph and explicit formal equipment target. */
		FCodeBP48NormalContainerSpatialGraphEquipmentTransferProof P48NormalContainerSpatialGraphEquipmentProof;
		/** P41 ordinary P12 complete spatial graph and exact BaseQuick candidate proof. */
		FCodeBP41BodySpatialGraphQuickTransferProof P41BodySpatialGraphProof;
		/** P42 normal P12 Drag of one complete spatial graph to an exact formal P6 slot. */
		FCodeBP42BodySpatialGraphEquipmentTransferProof P42BodySpatialGraphEquipmentProof;
	};

	struct FCodeBP2ApplicationResult
	{
		bool bSuccess = false;
		ECodeBP2ResultCode Code = ECodeBP2ResultCode::InvalidCommand;
		FString Message;
		FCodeBP2Command Command;
		FCodeBTransactionResult P1Result;
		FCodeBP2Projection Projection;

		bool IsSuccess() const { return bSuccess && Code == ECodeBP2ResultCode::Success; }
	};

	class FCodeBP2ProjectionBuilder
	{
	public:
		static bool Build(
			const FCodeBRepository& Repository,
			const FCodeBP2PlayerLayout& Layout,
			FCodeBP2Projection& OutProjection,
			const FCodeBTransactionResult* LastTransaction = nullptr,
			FString* OutError = nullptr);
	};

	class FCodeBP2ApplicationService;

	class FCodeBP2Fixture
	{
	public:
		static bool Build(FCodeBP2Fixture& OutFixture, FString* OutError = nullptr);

		const FCodeBRepository& GetRepository() const { return Repository; }
		const FCodeBP2PlayerLayout& GetLayout() const { return Layout; }
		const FCodeBP2FixtureIds& GetIds() const { return Ids; }
		bool Validate(FString* OutError = nullptr) const;
		FCodeBP2ApplicationService MakeApplicationService();

	private:
		FCodeBRepository Repository;
		FCodeBP2PlayerLayout Layout;
		FCodeBP2FixtureIds Ids;
	};

	class FCodeBP2ApplicationService
	{
	public:
		FCodeBP2ApplicationService(FCodeBRepository& InRepository, const FCodeBP2PlayerLayout& InLayout);

		bool BuildCurrentProjection(FCodeBP2Projection& OutProjection, FString* OutError = nullptr) const;
		FCodeBP2ApplicationResult Apply(const FCodeBP2Command& Command);

	private:
		FCodeBRepository& Repository;
		FCodeBP2PlayerLayout Layout;
	};
}
