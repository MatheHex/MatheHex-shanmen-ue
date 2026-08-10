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
		ECodeBItemType ItemType = ECodeBItemType::Generic;
		/** Projection-only Code B definition metadata; no UI cache is a hotbar truth. */
		bool bQuickUsable = false;
		/** Read-only P1 definition semantic used by P4 to preview the exact equipment target. */
		ECodeBEquipSlot EquipSlot = ECodeBEquipSlot::None;
		/** Projection-only parent/child relation used by P4 to reject loaded spatial moves without reading P1. */
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
	 * P38/P39 transient proof for one normal drag or frozen-target QuickTransfer
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
		FString EquipmentCandidateSetDigest;
		FName WorkspaceTargetPaneId;
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
				&& !LootProfileDigest.IsEmpty() && !EquipmentCandidateSetDigest.IsEmpty()
				&& !WorkspaceTargetPaneId.IsNone();
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
	 * P41 transient proof for one revealed P20 spatial parent in the exact opened
	 * P12 ordinary body root.  It freezes the canonical source closure and the
	 * input-time BaseQuick domain; Preview later fills one exact first-empty
	 * candidate and Commit may only revalidate that same address.
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
		/** P41 ordinary P12 complete spatial graph and exact BaseQuick candidate proof. */
		FCodeBP41BodySpatialGraphQuickTransferProof P41BodySpatialGraphProof;
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
