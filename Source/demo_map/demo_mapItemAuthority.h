#pragma once

#include "CoreMinimal.h"
#include "demo_mapItemTypes.h"

struct Fdemo_mapItemAuthorityState
{
	TMap<FGuid, Fdemo_mapItemInstance> Instances;
	TArray<FGuid> InventorySlots;
	TMap<FName, FGuid> EquipmentSlots;
	TArray<FGuid> SessionStash;
};

/** One atomic world-lost spatial item plus the instances that occupied its storage cells. */
struct Fdemo_mapSpatialDiscardBundle
{
	FGuid BundleId;
	FGuid SpatialItemInstanceId;
	TArray<FGuid> StoredItemInstanceIds;

	bool IsValid() const
	{
		return BundleId.IsValid() && SpatialItemInstanceId.IsValid();
	}

	TArray<FGuid> GetAllInstanceIds() const
	{
		TArray<FGuid> Result;
		if (SpatialItemInstanceId.IsValid()) Result.Add(SpatialItemInstanceId);
		Result.Append(StoredItemInstanceIds);
		return Result;
	}
};

/** Sole writer for item instances, inventory, equipment and ownership transitions. */
class Fdemo_mapItemAuthority
{
public:
	Fdemo_mapItemAuthority();
	void Reset();

	Fdemo_mapItemOperationResult AddDefinition(FName DefinitionId, int32 Quantity, TArray<FGuid>* OutAffectedInstances = nullptr);
	Fdemo_mapItemOperationResult Equip(FGuid InstanceId, FName SlotId);
	Fdemo_mapItemOperationResult Unequip(FName SlotId);
	Fdemo_mapItemOperationResult Destroy(FGuid InstanceId);
	Fdemo_mapItemOperationResult CreateWorldDefinition(FName DefinitionId, int32 Quantity, FGuid& OutInstanceId);
	Fdemo_mapItemOperationResult PickupWorld(FGuid InstanceId, TArray<FGuid>* OutAffectedInstances = nullptr);
	Fdemo_mapItemOperationResult CreateContainerDefinition(
		FName DefinitionId,
		int32 Quantity,
		FGuid ContainerId,
		FGuid OriginRunId,
		FGuid& OutInstanceId,
		Edemo_mapRewardEventKind RewardEventKind =
			Edemo_mapRewardEventKind::None,
		FGuid RewardEventId = FGuid(),
		int32 RewardValueMultiplierBps =
			Fdemo_mapRewardEventRules::NormalMultiplierBps,
		FName RewardSourceRoleId = NAME_None,
		FGuid RareRewardEventId = FGuid(),
		FName RareRewardPolicyId = NAME_None,
		FName RareRewardTierId = NAME_None,
		int64 RareRewardBonusValue = 0,
		const Fdemo_mapRewardAffixSet& AffixSet = {});
	Fdemo_mapItemOperationResult TransferContainerToInventoryWhole(
		FGuid InstanceId,
		FGuid ExpectedContainerId,
		bool bFailAfterMutation = false);
	Fdemo_mapItemOperationResult ConsumeInventoryUnit(
		FGuid InstanceId
#if WITH_DEV_AUTOMATION_TESTS
		, bool bFailAfterMutation = false
#endif
	);
	/** Atomically swaps two Runtime inventory cells without changing item identity. */
	Fdemo_mapItemOperationResult MoveInventorySlot(
		int32 SourceSlotIndex,
		int32 TargetSlotIndex);
	/**
	 * P2's sole Runtime player-item drag/drop writer.  It resolves visual base
	 * six, spatial and equipment targets against this authority and commits a
	 * whole Move/Swap/Merge/Equip/Unequip transaction or rejects unchanged.
	 */
	Fdemo_mapPlayerItemDropResult ExecutePlayerItemDrop(
		const Fdemo_mapPlayerItemDropIntent& Intent);
	Fdemo_mapItemOperationResult DestroyContainer(
		FGuid InstanceId,
		FGuid ExpectedContainerId);
	Fdemo_mapItemOperationResult MoveInventoryToWorld(FGuid InstanceId);
	/** Atomically moves the equipped spatial item and only its storage cells to World ownership. */
	Fdemo_mapItemOperationResult DiscardSpatialBundle(
		Fdemo_mapSpatialDiscardBundle& OutBundle,
		bool bFailAfterMutation = false);
	/** Atomically restores a complete discarded bundle with identical ItemInstance GUIDs. */
	Fdemo_mapItemOperationResult RecoverSpatialBundle(
		const Fdemo_mapSpatialDiscardBundle& Bundle,
		bool bFailAfterMutation = false);
	Fdemo_mapItemOperationResult DestroyWorld(FGuid InstanceId);
	Fdemo_mapItemOperationResult MaterializeDeployedInstance(
		FGuid InstanceId,
		FName DefinitionId,
		int32 Quantity,
		FName EquipmentSlotId,
		FGuid OriginRunId = FGuid(),
		Edemo_mapRewardEventKind RewardEventKind =
			Edemo_mapRewardEventKind::None,
		FGuid RewardEventId = FGuid(),
		int32 RewardValueMultiplierBps =
			Fdemo_mapRewardEventRules::NormalMultiplierBps,
		FName RewardSourceRoleId = NAME_None,
		FGuid RareRewardEventId = FGuid(),
		FName RareRewardPolicyId = NAME_None,
		FName RareRewardTierId = NAME_None,
		int64 RareRewardBonusValue = 0,
		const Fdemo_mapRewardAffixSet& AffixSet = {});
	Fdemo_mapItemOperationResult TagInstancesForRun(const TArray<FGuid>& InstanceIds, FGuid RunId);
	Fdemo_mapItemOperationResult SettleRunItems(FGuid RunId, const TSet<FGuid>& DeployedItemIds, bool bSecureCarriedItems, TArray<Fdemo_mapSettlementItemRow>& OutRows);
	int32 CompactDestroyedRun(FGuid RunId, const TSet<FGuid>& DeployedItemIds);
	bool IsItemAtRiskInRun(FGuid InstanceId, FGuid RunId, const TSet<FGuid>& DeployedItemIds) const;

	Fdemo_mapInventoryCapacityResult GetInventoryCapacityResult() const;
	Fdemo_mapInventoryCapacityResult PreflightInventoryCapacity(
		int32 ProjectedUsedSlots,
		FName BackpackDefinitionId) const;
	Fdemo_mapInventoryCapacityResult PreflightInventoryCapacity(
		int32 ProjectedUsedSlots,
		FName BackpackDefinitionId,
		FName SpatialRingDefinitionId) const;
	int32 GetRingQuickCapacity() const;
	int32 GetSpatialBagCapacity() const;
	Fdemo_mapGridContainerSnapshot BuildRunInventoryGridSnapshot() const;
	int32 GetInventoryCapacity() const;
	int32 GetUsedInventorySlots() const;
	int32 GetFreeInventorySlots() const;
	int32 GetAuthorityRevision() const { return AuthorityRevision; }
	int32 FindInventorySlot(FGuid InstanceId) const;
	bool CanAcceptUnequippedItem() const { return GetFreeInventorySlots() > 0; }
	const Fdemo_mapItemInstance* FindInstance(FGuid InstanceId) const { return Instances.Find(InstanceId); }
	const TMap<FGuid, Fdemo_mapItemInstance>& GetInstanceSnapshot() const { return Instances; }
	const TArray<FGuid>& GetInventorySlotSnapshot() const { return InventorySlots; }
	const TMap<FName, FGuid>& GetEquipmentSlotSnapshot() const { return EquipmentSlots; }
	const TArray<FGuid>& GetSessionStashSnapshot() const { return SessionStash; }
	FGuid GetEquippedInstance(FName SlotId) const;
	TArray<FGuid> FindInventoryInstancesByDefinition(FName DefinitionId) const;
	TArray<FGuid> FindWorldInstances() const;
	bool ValidateInvariants(FString* OutError = nullptr) const;

	Fdemo_mapItemAuthorityState CaptureState() const;

private:
	friend class Udemo_mapItemSubsystem;
	void RestoreState(const Fdemo_mapItemAuthorityState& State);
	FGuid GenerateUniqueInstanceId() const;
	int32 FindFirstEmptyInventorySlot() const;
	bool IsKnownEquipmentSlot(FName SlotId) const;
	bool ResolveInventoryAreaSlot(
		Edemo_mapPlayerItemArea Area,
		int32 VisualSlotIndex,
		int32& OutInventorySlotIndex) const;
	void AdvanceAuthorityRevision();
	FName GetEquippedBackpackDefinitionId() const;
	FName GetEquippedSpatialRingDefinitionId() const;
	bool NormalizeInventorySlots(int32 TargetCapacity);
	Fdemo_mapItemOperationResult CommitOrRollback(const Fdemo_mapItemAuthorityState& Before, FGuid InstanceId, FName DefinitionId, FName SlotId);

	TMap<FGuid, Fdemo_mapItemInstance> Instances;
	TArray<FGuid> InventorySlots;
	TMap<FName, FGuid> EquipmentSlots;
	TArray<FGuid> SessionStash;
	int32 AuthorityRevision = 0;
};
