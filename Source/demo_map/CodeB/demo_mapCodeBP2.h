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
		/** P29 transient proof of the exact P17 child selected by the shared workspace. */
		FGuid QuickTransferActivePlayerContainerId;
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
