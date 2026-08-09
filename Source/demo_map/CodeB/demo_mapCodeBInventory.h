// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "CoreMinimal.h"

namespace demo_map_code_b
{
	/** Code B intentionally uses plain value types so it can be tested without creating a second runtime authority. */
	enum class ECodeBItemType : uint8
	{
		Generic,
		Material,
		Consumable,
		Weapon,
		Armor,
		Accessory,
		SpatialItem,
		Backpack
	};

	/** Stable Code B provenance for an item-owned, one-level storage container. */
	enum class ECodeBSpatialContainerSemantic : uint8
	{
		None,
		QuickRing,
		StoragePouch
	};

	/** Immutable descriptor carried by a Code B definition, never by Code A. */
	enum class ECodeBQuickUseEffectKind : uint8
	{
		None,
		RestoreHealth
	};

	enum class ECodeBEquipSlot : uint8
	{
		None,
		Weapon,
		Armor,
		Accessory,
		SpatialItem,
		Backpack
	};

	enum class ECodeBContainerKind : uint8
	{
		Storage,
		Equipment
	};

	enum class ECodeBOperation : uint8
	{
		Move,
		Swap,
		Merge,
		Split,
		Equip,
		Unequip
	};

	enum class ECodeBResultCode : uint8
	{
		Success,
		ItemNotFound,
		SourceMismatch,
		TargetNotFound,
		TargetOccupied,
		TargetFull,
		InvalidSlot,
		InvalidQuantity,
		InvalidDefinition,
		NotEquipable,
		IncompatibleSlot,
		StackMismatch,
		StackFull,
		DuplicateItem,
		DuplicatePlacement,
		ContainerCycle,
		StaleRevision,
		InvariantViolation,
		InternalCommitFailure
	};

	struct FCodeBItemDefinition
	{
		FName DefinitionId;
		ECodeBItemType ItemType = ECodeBItemType::Generic;
		bool bStackable = false;
		/** Explicit Code B eligibility metadata for P13's reference-only 1--9 hotbar. */
		bool bQuickUsable = false;
		/** P15 permits only this data-defined effect on a bound BaseQuick item. */
		ECodeBQuickUseEffectKind QuickUseEffect = ECodeBQuickUseEffectKind::None;
		int32 QuickUseRestoreAmount = 0;
		int32 MaxStack = 1;
		ECodeBEquipSlot EquipSlot = ECodeBEquipSlot::None;
		int32 GridWidth = 1;
		int32 GridHeight = 1;
		/** Canonical product-definition semantic; legacy snapshots retain None. */
		ECodeBSpatialContainerSemantic SpatialContainerSemantic = ECodeBSpatialContainerSemantic::None;
		/** Exact capacity required by a newly canonical item-owned child container. */
		int32 ChildContainerCapacity = 0;

		bool operator==(const FCodeBItemDefinition& Other) const;
	};

	struct FCodeBItemInstance
	{
		FGuid ItemId;
		FName DefinitionId;
		int32 Quantity = 1;
		int32 Level = 1;
		int32 Quality = 0;
		int32 RandomSeed = 0;
		/**
		 * Immutable import metadata kept with the Code B instance.  P5 uses this
		 * to retain an auditable digest of legacy affixes without introducing a
		 * second editable Code A inventory mirror.
		 */
		FString LegacyAffixDigest;
		FGuid ParentContainerId;
		int32 SlotIndex = INDEX_NONE;
		/** Optional Code B owned internal storage for a spatial item. */
		FGuid ChildContainerId;

		bool IsPlaced() const { return ParentContainerId.IsValid() && SlotIndex != INDEX_NONE; }
		bool operator==(const FCodeBItemInstance& Other) const;
	};

	struct FCodeBContainer
	{
		FGuid ContainerId;
		FName ContainerType;
		ECodeBContainerKind Kind = ECodeBContainerKind::Storage;
		ECodeBEquipSlot EquipmentSlot = ECodeBEquipSlot::None;
		TArray<FGuid> Slots;

		bool IsEquipment() const { return Kind == ECodeBContainerKind::Equipment; }
		bool operator==(const FCodeBContainer& Other) const;
	};

	struct FCodeBTransactionRequest
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
};

	struct FCodeBTransactionResult
	{
		bool bSuccess = false;
		ECodeBResultCode Code = ECodeBResultCode::InternalCommitFailure;
		FString Message;
		FGuid TransactionId;
		int32 NewRevision = 0;
		TArray<FGuid> AffectedItemIds;
		TArray<FGuid> AffectedContainerIds;
		FGuid CreatedItemId;

		bool IsSuccess() const { return bSuccess && Code == ECodeBResultCode::Success; }
	};

	struct FCodeBSnapshot
	{
		int32 Revision = 0;
		TMap<FName, FCodeBItemDefinition> Definitions;
		TMap<FGuid, FCodeBItemInstance> Items;
		TMap<FGuid, FCodeBContainer> Containers;

		bool operator==(const FCodeBSnapshot& Other) const;
		bool operator!=(const FCodeBSnapshot& Other) const { return !(*this == Other); }
	};

	/**
	 * The only Code B mutable authority. Setup methods are seed/fixture APIs; all
	 * post-seed item movement goes through ExecuteTransaction.
	 */
	class FCodeBRepository
	{
	public:
		bool RegisterDefinition(const FCodeBItemDefinition& Definition, FString* OutError = nullptr);
		FGuid CreateContainer(
			FName ContainerType,
			int32 Capacity,
			ECodeBContainerKind Kind = ECodeBContainerKind::Storage,
			ECodeBEquipSlot EquipmentSlot = ECodeBEquipSlot::None,
			FString* OutError = nullptr,
			const FGuid& ForcedContainerId = FGuid());
		FGuid CreateItem(
			FName DefinitionId,
			int32 Quantity,
			const FGuid& ParentContainerId = FGuid(),
			int32 TargetSlot = INDEX_NONE,
			const FGuid& ForcedItemId = FGuid(),
			FString* OutError = nullptr);
		/** Setup-only relationship for a spatial item and its internal storage container. */
		bool AssociateChildContainer(const FGuid& SpatialItemId, const FGuid& ChildContainerId, FString* OutError = nullptr);

		FCodeBTransactionResult ExecuteTransaction(const FCodeBTransactionRequest& Request);

		FCodeBSnapshot CaptureSnapshot() const;
		/**
		 * Persistence-only snapshot hydration/rollback boundary.  This validates
		 * the complete candidate before replacing the in-memory repository, so a
		 * failed profile commit can restore the prior P1 state without inventing
		 * setup writes or replaying a transaction.
		 */
		bool LoadPersistedSnapshot(const FCodeBSnapshot& Snapshot, FString* OutError = nullptr);
		bool ValidateInvariants(FString* OutError = nullptr) const;
		int32 GetRevision() const { return State.Revision; }
		const FCodeBItemDefinition* FindDefinition(FName DefinitionId) const;
		const FCodeBItemInstance* FindItem(const FGuid& ItemId) const;
		const FCodeBContainer* FindContainer(const FGuid& ContainerId) const;

	public:
		struct FState
		{
			int32 Revision = 0;
			TMap<FName, FCodeBItemDefinition> Definitions;
			TMap<FGuid, FCodeBItemInstance> Items;
			TMap<FGuid, FCodeBContainer> Containers;
		};

	private:
		FState State;

		static bool ValidateState(const FState& Candidate, FString* OutError);
		static FCodeBTransactionResult MakeFailure(
			const FCodeBTransactionRequest& Request,
			int32 CurrentRevision,
			ECodeBResultCode Code,
			const TCHAR* Message);
		static bool IsValidStorageTarget(
			const FState& Candidate,
			const FGuid& ContainerId,
			int32 Slot,
			FCodeBTransactionResult& OutResult,
			int32* OutResolvedSlot = nullptr);
		static bool ValidateSource(
			FState& Candidate,
			const FCodeBTransactionRequest& Request,
			FCodeBItemInstance*& OutItem,
			FCodeBContainer*& OutSource,
			FCodeBTransactionResult& OutResult);
		static void AddAffected(FCodeBTransactionResult& Result, const FGuid& ItemId, const FGuid& ContainerId);
		static bool ExecuteMove(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult);
		static bool ExecuteSwap(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult);
		static bool ExecuteMerge(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult);
		static bool ExecuteSplit(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult);
		static bool ExecuteEquip(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult);
		static bool ExecuteUnequip(FState& Candidate, const FCodeBTransactionRequest& Request, FCodeBTransactionResult& OutResult);
	};
}
