#include "demo_mapItemAuthority.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapItemDefinitions.h"

Fdemo_mapItemAuthority::Fdemo_mapItemAuthority()
{
	Reset();
}

void Fdemo_mapItemAuthority::Reset()
{
	Instances.Reset();
	InventorySlots.Init(FGuid(), Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack);
	EquipmentSlots.Reset();
	SessionStash.Reset();
	AuthorityRevision = 0;
	for (FName SlotId : Fdemo_mapItemDefinitions::GetEquipmentSlotIds()) EquipmentSlots.Add(SlotId, FGuid());
}

void Fdemo_mapItemAuthority::AdvanceAuthorityRevision()
{
	AuthorityRevision = AuthorityRevision == MAX_int32
		? 1
		: AuthorityRevision + 1;
}

FName Fdemo_mapItemAuthority::GetEquippedBackpackDefinitionId() const
{
	const FGuid BackpackId = EquipmentSlots.FindRef(Fdemo_mapItemIds::BackpackSlot);
	const Fdemo_mapItemInstance* Backpack = BackpackId.IsValid() ? Instances.Find(BackpackId) : nullptr;
	return Backpack ? Backpack->DefinitionId : NAME_None;
}

FName Fdemo_mapItemAuthority::GetEquippedSpatialRingDefinitionId() const
{
	const FGuid RingId = EquipmentSlots.FindRef(Fdemo_mapItemIds::SpatialRingSlot);
	const Fdemo_mapItemInstance* Ring =
		RingId.IsValid() ? Instances.Find(RingId) : nullptr;
	return Ring ? Ring->DefinitionId : NAME_None;
}

Fdemo_mapInventoryCapacityResult Fdemo_mapItemAuthority::GetInventoryCapacityResult() const
{
	const FGuid BackpackId = EquipmentSlots.FindRef(Fdemo_mapItemIds::BackpackSlot);
	if (BackpackId.IsValid())
	{
		const Fdemo_mapItemInstance* Backpack = Instances.Find(BackpackId);
		if (!Backpack
			|| Backpack->OwnershipState != Edemo_mapItemOwnershipState::Equipped
			|| Backpack->EquippedSlotId != Fdemo_mapItemIds::BackpackSlot
			|| Backpack->Quantity != 1)
		{
			Fdemo_mapInventoryCapacityResult Result;
			Result.Diagnostic = TEXT("Backpack slot does not reference one valid equipped instance.");
			return Result;
		}
	}
	return Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
		GetEquippedBackpackDefinitionId(),
		GetEquippedSpatialRingDefinitionId());
}

Fdemo_mapInventoryCapacityResult Fdemo_mapItemAuthority::PreflightInventoryCapacity(
	int32 ProjectedUsedSlots,
	FName BackpackDefinitionId) const
{
	return PreflightInventoryCapacity(
		ProjectedUsedSlots,
		BackpackDefinitionId,
		GetEquippedSpatialRingDefinitionId());
}

Fdemo_mapInventoryCapacityResult Fdemo_mapItemAuthority::PreflightInventoryCapacity(
	int32 ProjectedUsedSlots,
	FName BackpackDefinitionId,
	FName SpatialRingDefinitionId) const
{
	Fdemo_mapInventoryCapacityResult Result =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			BackpackDefinitionId,
			SpatialRingDefinitionId);
	Result.UsedSlots = ProjectedUsedSlots;
	if (!Result.bSuccess)
	{
		return Result;
	}
	if (ProjectedUsedSlots < 0)
	{
		Result.bSuccess = false;
		Result.bFits = false;
		Result.Diagnostic = TEXT("Projected inventory usage cannot be negative.");
		return Result;
	}
	Result.bFits = ProjectedUsedSlots <= Result.Capacity;
	if (!Result.bFits)
	{
		Result.Diagnostic = FString::Printf(
			TEXT("Projected inventory usage %d exceeds capacity %d."),
			ProjectedUsedSlots,
			Result.Capacity);
	}
	return Result;
}

int32 Fdemo_mapItemAuthority::GetRingQuickCapacity() const
{
	const Fdemo_mapInventoryCapacityResult Result = GetInventoryCapacityResult();
	return Result.bSuccess ? Result.RingQuickCapacity : 0;
}

int32 Fdemo_mapItemAuthority::GetSpatialBagCapacity() const
{
	const Fdemo_mapInventoryCapacityResult Result = GetInventoryCapacityResult();
	return Result.bSuccess ? Result.SpatialBagCapacity : 0;
}

int32 Fdemo_mapItemAuthority::GetInventoryCapacity() const
{
	const Fdemo_mapInventoryCapacityResult Result = GetInventoryCapacityResult();
	return Result.bSuccess ? Result.Capacity : 0;
}

int32 Fdemo_mapItemAuthority::GetUsedInventorySlots() const
{
	int32 Count = 0;
	for (const FGuid& InstanceId : InventorySlots) if (InstanceId.IsValid()) ++Count;
	return Count;
}

int32 Fdemo_mapItemAuthority::GetFreeInventorySlots() const
{
	const int32 Capacity = GetInventoryCapacity();
	return Capacity > 0 ? FMath::Max(0, Capacity - GetUsedInventorySlots()) : 0;
}

Fdemo_mapGridContainerSnapshot Fdemo_mapItemAuthority::BuildRunInventoryGridSnapshot() const
{
	const Fdemo_mapInventoryCapacityResult CapacityResult = GetInventoryCapacityResult();
	if (!CapacityResult.bSuccess)
	{
		Fdemo_mapGridContainerSnapshot Snapshot;
		Snapshot.Diagnostic = CapacityResult.Diagnostic;
		return Snapshot;
	}

	TArray<FGuid> OrderedOccupied;
	bool bReachedEmpty = false;
	for (const FGuid& InstanceId : InventorySlots)
	{
		if (!InstanceId.IsValid())
		{
			bReachedEmpty = true;
			continue;
		}
		const Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
		if (bReachedEmpty
			|| !Instance
			|| Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory)
		{
			Fdemo_mapGridContainerSnapshot Snapshot;
			Snapshot.Diagnostic = TEXT("Run Inventory is not in canonical ordered form.");
			return Snapshot;
		}
		OrderedOccupied.Add(InstanceId);
	}
	return Fdemo_mapItemViewRules::BuildGridFromOccupiedOrder(OrderedOccupied, CapacityResult.Capacity);
}

int32 Fdemo_mapItemAuthority::FindInventorySlot(FGuid InstanceId) const
{
	return InstanceId.IsValid() ? InventorySlots.IndexOfByKey(InstanceId) : INDEX_NONE;
}

int32 Fdemo_mapItemAuthority::FindFirstEmptyInventorySlot() const
{
	return InventorySlots.IndexOfByPredicate([](const FGuid& Id) { return !Id.IsValid(); });
}

bool Fdemo_mapItemAuthority::NormalizeInventorySlots(int32 TargetCapacity)
{
	if (TargetCapacity < 0)
	{
		return false;
	}
	TArray<FGuid> OrderedOccupied;
	TSet<FGuid> Seen;
	for (const FGuid& InstanceId : InventorySlots)
	{
		if (!InstanceId.IsValid())
		{
			continue;
		}
		if (Seen.Contains(InstanceId))
		{
			return false;
		}
		Seen.Add(InstanceId);
		OrderedOccupied.Add(InstanceId);
	}
	if (OrderedOccupied.Num() > TargetCapacity)
	{
		return false;
	}
	InventorySlots.Init(FGuid(), TargetCapacity);
	for (int32 Index = 0; Index < OrderedOccupied.Num(); ++Index)
	{
		InventorySlots[Index] = OrderedOccupied[Index];
	}
	return true;
}

FGuid Fdemo_mapItemAuthority::GetEquippedInstance(FName SlotId) const
{
	if (const FGuid* InstanceId = EquipmentSlots.Find(SlotId)) return *InstanceId;
	return FGuid();
}

TArray<FGuid> Fdemo_mapItemAuthority::FindInventoryInstancesByDefinition(FName DefinitionId) const
{
	TArray<FGuid> Result;
	for (const FGuid& InstanceId : InventorySlots)
	{
		const Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
		if (Instance != nullptr && Instance->DefinitionId == DefinitionId) Result.Add(InstanceId);
	}
	return Result;
}

bool Fdemo_mapItemAuthority::IsKnownEquipmentSlot(FName SlotId) const
{
	return EquipmentSlots.Contains(SlotId);
}

bool Fdemo_mapItemAuthority::ResolveInventoryAreaSlot(
	Edemo_mapPlayerItemArea Area,
	int32 VisualSlotIndex,
	int32& OutInventorySlotIndex) const
{
	OutInventorySlotIndex = INDEX_NONE;
	if (VisualSlotIndex < 0)
	{
		return false;
	}
	if (Area == Edemo_mapPlayerItemArea::BaseQuickItems)
	{
		if (VisualSlotIndex >= Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack)
		{
			return false;
		}
		OutInventorySlotIndex = VisualSlotIndex;
	}
	else if (Area == Edemo_mapPlayerItemArea::SpatialStorage)
	{
		OutInventorySlotIndex = Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
			+ VisualSlotIndex;
	}
	else
	{
		return false;
	}
	return InventorySlots.IsValidIndex(OutInventorySlotIndex);
}

FGuid Fdemo_mapItemAuthority::GenerateUniqueInstanceId() const
{
	FGuid Candidate;
	do Candidate = FGuid::NewGuid(); while (!Candidate.IsValid() || Instances.Contains(Candidate));
	return Candidate;
}

Fdemo_mapItemAuthorityState Fdemo_mapItemAuthority::CaptureState() const
{
	return { Instances, InventorySlots, EquipmentSlots, SessionStash };
}

void Fdemo_mapItemAuthority::RestoreState(const Fdemo_mapItemAuthorityState& State)
{
	Instances = State.Instances;
	InventorySlots = State.InventorySlots;
	EquipmentSlots = State.EquipmentSlots;
	SessionStash = State.SessionStash;
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::MaterializeDeployedInstance(
	FGuid InstanceId,
	FName DefinitionId,
	int32 Quantity,
	FName EquipmentSlotId,
	FGuid OriginRunId,
	Edemo_mapRewardEventKind RewardEventKind,
	FGuid RewardEventId,
	int32 RewardValueMultiplierBps,
	FName RewardSourceRoleId,
	FGuid RareRewardEventId,
	FName RareRewardPolicyId,
	FName RareRewardTierId,
	int64 RareRewardBonusValue,
	const Fdemo_mapRewardAffixSet& AffixSet)
{
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (!InstanceId.IsValid() || Instances.Contains(InstanceId)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, TEXT("A deployed instance ID is invalid or already registered."), InstanceId, DefinitionId, EquipmentSlotId);
	if (!Definition) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("A deployed item definition is not registered."), InstanceId, DefinitionId, EquipmentSlotId);
	if (Quantity <= 0 || Quantity > Definition->MaxStackSize) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidQuantity, TEXT("A deployed item quantity violates its definition."), InstanceId, DefinitionId, EquipmentSlotId);
	FString RewardError;
	if (!Fdemo_mapRewardEventRules::IsValid(
		RewardEventKind,
		RewardEventId,
		RewardValueMultiplierBps,
		RewardSourceRoleId,
		RareRewardEventId,
		RareRewardPolicyId,
		RareRewardTierId,
		RareRewardBonusValue,
		&RewardError))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			RewardError,
			InstanceId,
			DefinitionId,
			EquipmentSlotId);
	}
	if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
		DefinitionId, Quantity, AffixSet, &RewardError))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			RewardError,
			InstanceId,
			DefinitionId,
			EquipmentSlotId);
	}

	Fdemo_mapItemInstance Instance;
	Instance.InstanceId = InstanceId;
	Instance.DefinitionId = DefinitionId;
	Instance.Quantity = Quantity;
	Instance.OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
	Instance.OriginRunId = OriginRunId;
	Instance.RewardEventKind = RewardEventKind;
	Instance.RewardEventId = RewardEventId;
	Instance.RewardValueMultiplierBps = RewardValueMultiplierBps;
	Instance.RewardSourceRoleId = RewardSourceRoleId;
	Instance.RareRewardEventId = RareRewardEventId;
	Instance.RareRewardPolicyId = RareRewardPolicyId;
	Instance.RareRewardTierId = RareRewardTierId;
	Instance.RareRewardBonusValue = RareRewardBonusValue;
	Instance.AffixSet = AffixSet;
	if (EquipmentSlotId.IsNone())
	{
		const int32 InventoryIndex = FindFirstEmptyInventorySlot();
		if (InventoryIndex == INDEX_NONE) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InventoryFull, TEXT("Runtime inventory cannot accept the deployed item."), InstanceId, DefinitionId);
		Instance.OwnershipState = Edemo_mapItemOwnershipState::Inventory;
		Instance.ContainerId = Fdemo_mapItemIds::InventoryContainer;
		Instances.Add(InstanceId, Instance);
		InventorySlots[InventoryIndex] = InstanceId;
	}
	else
	{
		const FGuid* Occupant = EquipmentSlots.Find(EquipmentSlotId);
		if (!Occupant) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidSlot, TEXT("A deployed equipment slot is unknown."), InstanceId, DefinitionId, EquipmentSlotId);
		if (Occupant->IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SlotOccupied, TEXT("A deployed equipment slot is occupied."), InstanceId, DefinitionId, EquipmentSlotId);
		if (Quantity != 1 || !Definition->CompatibleSlotIds.Contains(EquipmentSlotId)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::IncompatibleSlot, TEXT("A deployed item is incompatible with its equipment slot."), InstanceId, DefinitionId, EquipmentSlotId);
		Instance.OwnershipState = Edemo_mapItemOwnershipState::Equipped;
		Instance.ContainerId = Fdemo_mapItemIds::EquipmentContainer;
		Instance.EquippedSlotId = EquipmentSlotId;
		Instances.Add(InstanceId, Instance);
		EquipmentSlots[EquipmentSlotId] = InstanceId;
	}
	const Fdemo_mapInventoryCapacityResult CapacityResult = GetInventoryCapacityResult();
	if (!CapacityResult.bSuccess || !NormalizeInventorySlots(CapacityResult.Capacity))
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			CapacityResult.Diagnostic.IsEmpty() ? TEXT("Deployed equipment would violate inventory capacity.") : CapacityResult.Diagnostic,
			InstanceId,
			DefinitionId,
			EquipmentSlotId);
	}
	return CommitOrRollback(Before, InstanceId, DefinitionId, EquipmentSlotId);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::TagInstancesForRun(const TArray<FGuid>& InstanceIds, FGuid RunId)
{
	if (!RunId.IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidRunId, TEXT("A valid active run ID is required."));
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	for (const FGuid& InstanceId : InstanceIds)
	{
		Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
		if (!Instance || Instance->OwnershipState == Edemo_mapItemOwnershipState::SessionStash || Instance->OwnershipState == Edemo_mapItemOwnershipState::Destroyed)
		{
			RestoreState(Before);
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidOwnership, TEXT("Only active run-owned instances can receive OriginRunId."), InstanceId);
		}
		if (Instance->OriginRunId.IsValid() && Instance->OriginRunId != RunId)
		{
			RestoreState(Before);
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, TEXT("An instance cannot cross run identities."), InstanceId, Instance->DefinitionId);
		}
		Instance->OriginRunId = RunId;
	}
	return CommitOrRollback(Before, InstanceIds.IsEmpty() ? FGuid() : InstanceIds[0], NAME_None, NAME_None);
}

bool Fdemo_mapItemAuthority::IsItemAtRiskInRun(FGuid InstanceId, FGuid RunId, const TSet<FGuid>& DeployedItemIds) const
{
	const Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	return Instance != nullptr && (DeployedItemIds.Contains(InstanceId) || Instance->OriginRunId == RunId);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::SettleRunItems(FGuid RunId, const TSet<FGuid>& DeployedItemIds, bool bSecureCarriedItems, TArray<Fdemo_mapSettlementItemRow>& OutRows)
{
	OutRows.Reset();
	if (!RunId.IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidRunId, TEXT("Settlement requires a valid run ID."));
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	for (TPair<FGuid, Fdemo_mapItemInstance>& Pair : Instances)
	{
		Fdemo_mapItemInstance& Instance = Pair.Value;
		if (!IsItemAtRiskInRun(Pair.Key, RunId, DeployedItemIds) || Instance.OwnershipState == Edemo_mapItemOwnershipState::SessionStash || Instance.OwnershipState == Edemo_mapItemOwnershipState::Destroyed) continue;
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Instance.DefinitionId);
		if (!Definition) { RestoreState(Before); OutRows.Reset(); return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("Settlement encountered an unknown definition."), Instance.InstanceId, Instance.DefinitionId); }
		Fdemo_mapSettlementItemRow Row;
		Row.InstanceId = Instance.InstanceId;
		Row.DefinitionId = Instance.DefinitionId;
		Row.Quantity = Instance.Quantity;
		Row.SourceOwnership = Instance.OwnershipState;
		Row.UnitValue = Definition->PrototypeValue;
		Row.TotalValue = Row.Quantity * Row.UnitValue;
		for (FGuid& SlotId : InventorySlots) if (SlotId == Instance.InstanceId) SlotId.Invalidate();
		for (TPair<FName, FGuid>& Slot : EquipmentSlots) if (Slot.Value == Instance.InstanceId) Slot.Value.Invalidate();
		const bool bSecure = bSecureCarriedItems && (Instance.OwnershipState == Edemo_mapItemOwnershipState::Inventory || Instance.OwnershipState == Edemo_mapItemOwnershipState::Equipped);
		if (bSecure)
		{
			Instance.OwnershipState = Edemo_mapItemOwnershipState::SessionStash;
			Instance.OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
			Instance.ContainerId = Fdemo_mapItemIds::SessionStashContainer;
			Instance.EquippedSlotId = NAME_None;
			SessionStash.Add(Instance.InstanceId);
		}
		else
		{
			Instance.OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
			Instance.OwnerId = NAME_None;
			Instance.ContainerId = NAME_None;
			Instance.EquippedSlotId = NAME_None;
		}
		Row.FinalOwnership = Instance.OwnershipState;
		OutRows.Add(Row);
	}
	OutRows.Sort([](const Fdemo_mapSettlementItemRow& A, const Fdemo_mapSettlementItemRow& B){ return A.InstanceId.ToString(EGuidFormats::Digits) < B.InstanceId.ToString(EGuidFormats::Digits); });
	const Fdemo_mapInventoryCapacityResult CapacityResult = GetInventoryCapacityResult();
	if (!CapacityResult.bSuccess || !NormalizeInventorySlots(CapacityResult.Capacity))
	{
		RestoreState(Before);
		OutRows.Reset();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::SettlementRollbackFailed,
			TEXT("Settlement could not restore the post-run inventory capacity."));
	}
	Fdemo_mapItemOperationResult Result = CommitOrRollback(Before, FGuid(), NAME_None, NAME_None);
	if (!Result.bSuccess) OutRows.Reset();
	return Result;
}

int32 Fdemo_mapItemAuthority::CompactDestroyedRun(FGuid RunId, const TSet<FGuid>& DeployedItemIds)
{
	if (!RunId.IsValid()) return 0;
	TArray<FGuid> RemoveIds;
	for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair : Instances)
	{
		if (IsItemAtRiskInRun(Pair.Key, RunId, DeployedItemIds) && Pair.Value.OwnershipState == Edemo_mapItemOwnershipState::Destroyed) RemoveIds.Add(Pair.Key);
	}
	for (const FGuid& Id : RemoveIds) Instances.Remove(Id);
	return RemoveIds.Num();
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::CommitOrRollback(const Fdemo_mapItemAuthorityState& Before, FGuid InstanceId, FName DefinitionId, FName SlotId)
{
	FString Error;
	if (!ValidateInvariants(&Error))
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, Error, InstanceId, DefinitionId, SlotId);
	}
	AdvanceAuthorityRevision();
	return Fdemo_mapItemOperationResult::Success(InstanceId, DefinitionId, SlotId);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::AddDefinition(FName DefinitionId, int32 Quantity, TArray<FGuid>* OutAffectedInstances)
{
	if (OutAffectedInstances) OutAffectedInstances->Reset();
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (Definition == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("Definition ID is not registered."), FGuid(), DefinitionId);
	if (Quantity <= 0) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidQuantity, TEXT("Quantity must be greater than zero."), FGuid(), DefinitionId);

	int64 AvailableUnits = static_cast<int64>(GetFreeInventorySlots()) * Definition->MaxStackSize;
	if (Definition->MaxStackSize > 1)
	{
		for (const FGuid& SlotId : InventorySlots)
		{
			const Fdemo_mapItemInstance* Instance = Instances.Find(SlotId);
			if (Instance != nullptr
				&& Instance->OwnershipState == Edemo_mapItemOwnershipState::Inventory
				&& Fdemo_mapRewardEventRules::AreStackCompatible(
					Instance->DefinitionId,
					Instance->RewardEventKind,
					Instance->RewardEventId,
					Instance->RewardValueMultiplierBps,
					Instance->RewardSourceRoleId,
					DefinitionId,
					Edemo_mapRewardEventKind::None,
					FGuid(),
					Fdemo_mapRewardEventRules::NormalMultiplierBps,
					NAME_None))
			{
				AvailableUnits += Definition->MaxStackSize - Instance->Quantity;
			}
		}
	}
	if (Quantity > AvailableUnits) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InventoryFull, TEXT("Inventory cannot accept the entire request."), FGuid(), DefinitionId);

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	int32 Remaining = Quantity;
	FGuid FirstAffected;
	if (Definition->MaxStackSize > 1)
	{
		for (const FGuid& SlotId : InventorySlots)
		{
			Fdemo_mapItemInstance* Instance = Instances.Find(SlotId);
			if (Instance == nullptr
				|| Instance->Quantity >= Definition->MaxStackSize
				|| !Fdemo_mapRewardEventRules::AreStackCompatible(
					Instance->DefinitionId,
					Instance->RewardEventKind,
					Instance->RewardEventId,
					Instance->RewardValueMultiplierBps,
					Instance->RewardSourceRoleId,
					DefinitionId,
					Edemo_mapRewardEventKind::None,
					FGuid(),
					Fdemo_mapRewardEventRules::NormalMultiplierBps,
					NAME_None))
			{
				continue;
			}
			const int32 Added = FMath::Min(Remaining, Definition->MaxStackSize - Instance->Quantity);
			Instance->Quantity += Added;
			Remaining -= Added;
			if (!FirstAffected.IsValid()) FirstAffected = Instance->InstanceId;
			if (OutAffectedInstances) OutAffectedInstances->AddUnique(Instance->InstanceId);
			if (Remaining == 0) break;
		}
	}
	while (Remaining > 0)
	{
		const int32 SlotIndex = FindFirstEmptyInventorySlot();
		if (SlotIndex == INDEX_NONE)
		{
			RestoreState(Before);
			if (OutAffectedInstances) OutAffectedInstances->Reset();
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InternalRollbackFailed, TEXT("Preflight succeeded but no inventory slot was available."), FGuid(), DefinitionId);
		}
		Fdemo_mapItemInstance Instance;
		Instance.InstanceId = GenerateUniqueInstanceId();
		Instance.DefinitionId = DefinitionId;
		Instance.Quantity = FMath::Min(Remaining, Definition->MaxStackSize);
		Instance.OwnershipState = Edemo_mapItemOwnershipState::Inventory;
		Instance.OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
		Instance.ContainerId = Fdemo_mapItemIds::InventoryContainer;
		InventorySlots[SlotIndex] = Instance.InstanceId;
		Instances.Add(Instance.InstanceId, Instance);
		Remaining -= Instance.Quantity;
		if (!FirstAffected.IsValid()) FirstAffected = Instance.InstanceId;
		if (OutAffectedInstances) OutAffectedInstances->Add(Instance.InstanceId);
	}
	Fdemo_mapItemOperationResult Result = CommitOrRollback(Before, FirstAffected, DefinitionId, NAME_None);
	if (!Result.bSuccess && OutAffectedInstances) OutAffectedInstances->Reset();
	return Result;
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::CreateWorldDefinition(FName DefinitionId, int32 Quantity, FGuid& OutInstanceId)
{
	OutInstanceId.Invalidate();
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (Definition == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("Definition ID is not registered."), FGuid(), DefinitionId);
	if (Quantity <= 0 || Quantity > Definition->MaxStackSize) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidQuantity, TEXT("World quantity must fit one definition stack."), FGuid(), DefinitionId);

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	Fdemo_mapItemInstance Instance;
	Instance.InstanceId = GenerateUniqueInstanceId();
	Instance.DefinitionId = DefinitionId;
	Instance.Quantity = Quantity;
	Instance.OwnershipState = Edemo_mapItemOwnershipState::World;
	Instance.ContainerId = Fdemo_mapItemIds::WorldContainer;
	Instances.Add(Instance.InstanceId, Instance);
	OutInstanceId = Instance.InstanceId;
	Fdemo_mapItemOperationResult Result = CommitOrRollback(Before, Instance.InstanceId, DefinitionId, NAME_None);
	if (!Result.bSuccess) OutInstanceId.Invalidate();
	return Result;
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::PickupWorld(FGuid InstanceId, TArray<FGuid>* OutAffectedInstances)
{
	if (OutAffectedInstances) OutAffectedInstances->Reset();
	Fdemo_mapItemInstance* WorldInstance = Instances.Find(InstanceId);
	if (WorldInstance == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InstanceNotFound, TEXT("World instance does not exist."), InstanceId);
	if (WorldInstance->OwnershipState != Edemo_mapItemOwnershipState::World) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::AlreadyClaimed, TEXT("World instance is no longer claimable."), InstanceId, WorldInstance->DefinitionId);
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(WorldInstance->DefinitionId);
	if (Definition == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("World instance definition is missing."), InstanceId, WorldInstance->DefinitionId);

	int64 AvailableUnits = static_cast<int64>(GetFreeInventorySlots()) * Definition->MaxStackSize;
	for (const FGuid& SlotId : InventorySlots)
	{
		const Fdemo_mapItemInstance* Existing = Instances.Find(SlotId);
		if (Existing != nullptr
			&& Fdemo_mapRewardEventRules::AreStackCompatible(
				Existing->DefinitionId,
				Existing->RewardEventKind,
				Existing->RewardEventId,
				Existing->RewardValueMultiplierBps,
				Existing->RewardSourceRoleId,
				Existing->RareRewardEventId,
				Existing->RareRewardPolicyId,
				Existing->RareRewardTierId,
				Existing->RareRewardBonusValue,
				WorldInstance->DefinitionId,
				WorldInstance->RewardEventKind,
				WorldInstance->RewardEventId,
				WorldInstance->RewardValueMultiplierBps,
				WorldInstance->RewardSourceRoleId,
				WorldInstance->RareRewardEventId,
				WorldInstance->RareRewardPolicyId,
				WorldInstance->RareRewardTierId,
				WorldInstance->RareRewardBonusValue))
		{
			AvailableUnits += Definition->MaxStackSize - Existing->Quantity;
		}
	}
	if (WorldInstance->Quantity > AvailableUnits) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InventoryFull, TEXT("Inventory cannot accept the entire world stack."), InstanceId, Definition->DefinitionId);

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	int32 Remaining = WorldInstance->Quantity;
	if (Definition->MaxStackSize > 1)
	{
		for (const FGuid& SlotId : InventorySlots)
		{
			Fdemo_mapItemInstance* Existing = Instances.Find(SlotId);
			if (Existing == nullptr
				|| Existing->Quantity >= Definition->MaxStackSize
				|| !Fdemo_mapRewardEventRules::AreStackCompatible(
					Existing->DefinitionId,
					Existing->RewardEventKind,
					Existing->RewardEventId,
					Existing->RewardValueMultiplierBps,
					Existing->RewardSourceRoleId,
					Existing->RareRewardEventId,
					Existing->RareRewardPolicyId,
					Existing->RareRewardTierId,
					Existing->RareRewardBonusValue,
					WorldInstance->DefinitionId,
					WorldInstance->RewardEventKind,
					WorldInstance->RewardEventId,
					WorldInstance->RewardValueMultiplierBps,
					WorldInstance->RewardSourceRoleId,
					WorldInstance->RareRewardEventId,
					WorldInstance->RareRewardPolicyId,
					WorldInstance->RareRewardTierId,
					WorldInstance->RareRewardBonusValue))
			{
				continue;
			}
			const int32 Added = FMath::Min(Remaining, Definition->MaxStackSize - Existing->Quantity);
			int64 AddedRareBonus = 0;
			int64 RemainingRareBonus =
				WorldInstance->RareRewardBonusValue;
			if (!Fdemo_mapRewardEventRules::TrySplitRareBonus(
				Remaining,
				Added,
				WorldInstance->RareRewardBonusValue,
				AddedRareBonus,
				RemainingRareBonus))
			{
				RestoreState(Before);
				return Fdemo_mapItemOperationResult::Failure(
					Edemo_mapItemResultCode::InvariantViolation,
					TEXT("World stack rare bonus split failed."),
					InstanceId,
					Definition->DefinitionId);
			}
			Existing->Quantity += Added;
			Existing->RareRewardBonusValue += AddedRareBonus;
			WorldInstance->RareRewardBonusValue = RemainingRareBonus;
			Remaining -= Added;
			if (OutAffectedInstances) OutAffectedInstances->AddUnique(Existing->InstanceId);
			if (Remaining == 0) break;
		}
	}
	WorldInstance = Instances.Find(InstanceId);
	if (Remaining > 0)
	{
		const int32 SlotIndex = FindFirstEmptyInventorySlot();
		if (SlotIndex == INDEX_NONE)
		{
			RestoreState(Before);
			if (OutAffectedInstances) OutAffectedInstances->Reset();
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InternalRollbackFailed, TEXT("Pickup preflight succeeded but no slot remained."), InstanceId, Definition->DefinitionId);
		}
		WorldInstance->Quantity = Remaining;
		WorldInstance->OwnershipState = Edemo_mapItemOwnershipState::Inventory;
		WorldInstance->OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
		WorldInstance->ContainerId = Fdemo_mapItemIds::InventoryContainer;
		InventorySlots[SlotIndex] = InstanceId;
		if (OutAffectedInstances) OutAffectedInstances->AddUnique(InstanceId);
	}
	else
	{
		WorldInstance->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
		WorldInstance->OwnerId = NAME_None;
		WorldInstance->ContainerId = NAME_None;
		WorldInstance->EquippedSlotId = NAME_None;
		WorldInstance->RareRewardEventId.Invalidate();
		WorldInstance->RareRewardPolicyId = NAME_None;
		WorldInstance->RareRewardTierId = NAME_None;
		WorldInstance->RareRewardBonusValue = 0;
		if (WorldInstance->RewardEventKind
			== Edemo_mapRewardEventKind::None)
		{
			WorldInstance->RewardSourceRoleId = NAME_None;
		}
	}
	Fdemo_mapItemOperationResult Result = CommitOrRollback(Before, InstanceId, Definition->DefinitionId, NAME_None);
	if (!Result.bSuccess && OutAffectedInstances) OutAffectedInstances->Reset();
	return Result;
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::CreateContainerDefinition(
	FName DefinitionId,
	int32 Quantity,
	FGuid ContainerId,
	FGuid OriginRunId,
	FGuid& OutInstanceId,
	Edemo_mapRewardEventKind RewardEventKind,
	FGuid RewardEventId,
	int32 RewardValueMultiplierBps,
	FName RewardSourceRoleId,
	FGuid RareRewardEventId,
	FName RareRewardPolicyId,
	FName RareRewardTierId,
	int64 RareRewardBonusValue,
	const Fdemo_mapRewardAffixSet& AffixSet,
	FGuid RequestedInstanceId)
{
	OutInstanceId.Invalidate();
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (!Definition)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnknownDefinition,
			TEXT("Container seed definition is not registered."),
			FGuid(),
			DefinitionId);
	}
	if (Quantity <= 0 || Quantity > Definition->MaxStackSize)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidQuantity,
			TEXT("Container seed quantity must fit one complete stack."),
			FGuid(),
			DefinitionId);
	}
	if (!ContainerId.IsValid() || !OriginRunId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidContainer,
			TEXT("Container seed requires valid Container and ActiveRun identities."),
			FGuid(),
			DefinitionId);
	}
	FString RewardError;
	if (!Fdemo_mapRewardEventRules::IsValid(
		RewardEventKind,
		RewardEventId,
		RewardValueMultiplierBps,
		RewardSourceRoleId,
		RareRewardEventId,
		RareRewardPolicyId,
		RareRewardTierId,
		RareRewardBonusValue,
		&RewardError))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			RewardError,
			FGuid(),
			DefinitionId);
	}
	if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
		DefinitionId, Quantity, AffixSet, &RewardError))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			RewardError,
			FGuid(),
			DefinitionId);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	Fdemo_mapItemInstance Instance;
	if (RequestedInstanceId.IsValid()
		&& Instances.Contains(RequestedInstanceId))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			TEXT("Committed Container ItemInstance identity already exists."),
			RequestedInstanceId,
			DefinitionId);
	}
	Instance.InstanceId = RequestedInstanceId.IsValid()
		? RequestedInstanceId
		: GenerateUniqueInstanceId();
	Instance.DefinitionId = DefinitionId;
	Instance.Quantity = Quantity;
	Instance.OwnershipState = Edemo_mapItemOwnershipState::Container;
	Instance.ContainerId = FName(*ContainerId.ToString(EGuidFormats::Digits));
	Instance.OriginRunId = OriginRunId;
	Instance.RewardEventKind = RewardEventKind;
	Instance.RewardEventId = RewardEventId;
	Instance.RewardValueMultiplierBps = RewardValueMultiplierBps;
	Instance.RewardSourceRoleId = RewardSourceRoleId;
	Instance.RareRewardEventId = RareRewardEventId;
	Instance.RareRewardPolicyId = RareRewardPolicyId;
	Instance.RareRewardTierId = RareRewardTierId;
	Instance.RareRewardBonusValue = RareRewardBonusValue;
	Instance.AffixSet = AffixSet;
	Instances.Add(Instance.InstanceId, Instance);
	OutInstanceId = Instance.InstanceId;
	Fdemo_mapItemOperationResult Result = CommitOrRollback(
		Before,
		Instance.InstanceId,
		DefinitionId,
		NAME_None);
	if (!Result.bSuccess)
	{
		OutInstanceId.Invalidate();
	}
	return Result;
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::TransferContainerToInventoryWhole(
	FGuid InstanceId,
	FGuid ExpectedContainerId,
	bool bFailAfterMutation)
{
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (!Instance)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InstanceNotFound,
			TEXT("Container ItemInstance does not exist."),
			InstanceId);
	}
	const FName ExpectedContainerName = ExpectedContainerId.IsValid()
		? FName(*ExpectedContainerId.ToString(EGuidFormats::Digits))
		: NAME_None;
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::Container
		|| Instance->ContainerId != ExpectedContainerName)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("Container Take found an ownership or ContainerId mismatch."),
			InstanceId,
			Instance->DefinitionId);
	}
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Instance->DefinitionId);
	if (!Definition)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnknownDefinition,
			TEXT("Container ItemInstance definition is not registered."),
			InstanceId,
			Instance->DefinitionId);
	}
	if (Instance->Quantity <= 0 || Instance->Quantity > Definition->MaxStackSize)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidQuantity,
			TEXT("Container ItemInstance has an invalid complete stack."),
			InstanceId,
			Instance->DefinitionId);
	}
	const Fdemo_mapInventoryCapacityResult Capacity =
		PreflightInventoryCapacity(
			GetUsedInventorySlots() + 1,
			GetEquippedBackpackDefinitionId());
	if (!Capacity.bSuccess || !Capacity.bFits)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			Capacity.Diagnostic.IsEmpty()
				? TEXT("Run Inventory cannot accept the complete Container ItemInstance.")
				: Capacity.Diagnostic,
			InstanceId,
			Instance->DefinitionId);
	}
	const int32 EmptySlot = FindFirstEmptyInventorySlot();
	if (EmptySlot == INDEX_NONE)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			TEXT("Run Inventory has no empty whole-instance slot."),
			InstanceId,
			Instance->DefinitionId);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName DefinitionId = Instance->DefinitionId;
	Instance->OwnershipState = Edemo_mapItemOwnershipState::Inventory;
	Instance->OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
	Instance->ContainerId = Fdemo_mapItemIds::InventoryContainer;
	Instance->EquippedSlotId = NAME_None;
	InventorySlots[EmptySlot] = InstanceId;
	if (bFailAfterMutation)
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InternalRollbackFailed,
			TEXT("Automation-injected Container Take failure rolled back the mutation."),
			InstanceId,
			DefinitionId);
	}
	return CommitOrRollback(Before, InstanceId, DefinitionId, NAME_None);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::ConsumeInventoryUnit(
	FGuid InstanceId
#if WITH_DEV_AUTOMATION_TESTS
	, bool bFailAfterMutation
#endif
)
{
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (!Instance)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InstanceNotFound,
			TEXT("Item use requires a registered ItemInstance."),
			InstanceId);
	}
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Instance->DefinitionId);
	if (!Definition)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UnknownDefinition,
			TEXT("Item use encountered an unknown Definition."),
			InstanceId,
			Instance->DefinitionId);
	}
	const int32 InventoryIndex = FindInventorySlot(InstanceId);
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory
		|| Instance->OwnerId != Fdemo_mapItemIds::LocalPlayerOwner
		|| Instance->ContainerId != Fdemo_mapItemIds::InventoryContainer
		|| InventoryIndex == INDEX_NONE)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("Item use may consume only the local player's Inventory-owned instance."),
			InstanceId,
			Instance->DefinitionId);
	}
	if (Instance->Quantity <= 0 || Instance->Quantity > Definition->MaxStackSize)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidQuantity,
			TEXT("Item use encountered an invalid StackCount."),
			InstanceId,
			Instance->DefinitionId);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName DefinitionId = Instance->DefinitionId;
	if (Instance->Quantity > 1)
	{
		--Instance->Quantity;
	}
	else
	{
		InventorySlots[InventoryIndex].Invalidate();
		Instance->Quantity = 0;
		Instance->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
		Instance->OwnerId = NAME_None;
		Instance->ContainerId = NAME_None;
		Instance->EquippedSlotId = NAME_None;
		if (!NormalizeInventorySlots(GetInventoryCapacity()))
		{
			RestoreState(Before);
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InternalRollbackFailed,
				TEXT("Item use could not restore canonical Inventory order."),
				InstanceId,
				DefinitionId);
		}
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailAfterMutation)
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InternalRollbackFailed,
			TEXT("Automation-injected item mutation failure rolled back the transaction."),
			InstanceId,
			DefinitionId);
	}
#endif
	return CommitOrRollback(Before, InstanceId, DefinitionId, NAME_None);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::MoveInventorySlot(
	int32 SourceSlotIndex,
	int32 TargetSlotIndex)
{
	if (!InventorySlots.IsValidIndex(SourceSlotIndex)
		|| !InventorySlots.IsValidIndex(TargetSlotIndex))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidSlot,
			TEXT("Runtime inventory move requires two cells inside the current definition-backed capacity."));
	}
	const FGuid InstanceId = InventorySlots[SourceSlotIndex];
	if (!InstanceId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("Runtime inventory move requires an occupied source cell."));
	}
	const Fdemo_mapItemInstance* Instance = FindInstance(InstanceId);
	if (!Instance
		|| Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory
		|| Instance->OwnerId != Fdemo_mapItemIds::LocalPlayerOwner
		|| Instance->ContainerId != Fdemo_mapItemIds::InventoryContainer)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("Only current local Inventory ownership may be rearranged."),
			InstanceId);
	}
	if (SourceSlotIndex == TargetSlotIndex)
	{
		return Fdemo_mapItemOperationResult::Success(
			InstanceId,
			Instance->DefinitionId,
			FName(*FString::Printf(TEXT("Inventory.%d"), TargetSlotIndex)));
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	InventorySlots.Swap(SourceSlotIndex, TargetSlotIndex);
	return CommitOrRollback(
		Before,
		InstanceId,
		Instance->DefinitionId,
		FName(*FString::Printf(TEXT("Inventory.%d"), TargetSlotIndex)));
}

Fdemo_mapPlayerItemDropResult Fdemo_mapItemAuthority::ExecutePlayerItemDrop(
	const Fdemo_mapPlayerItemDropIntent& Intent)
{
	Fdemo_mapPlayerItemDropResult Result;
	Result.SourceItemInstanceId = Intent.ExpectedSourceItemInstanceId;
	Result.CommittedAuthorityRevision = AuthorityRevision;
	auto Reject = [&Result, this](Edemo_mapItemResultCode Code, const FString& Message)
	{
		Result.Kind = Edemo_mapPlayerItemDropKind::Reject;
		Result.bCommitted = false;
		Result.CommittedAuthorityRevision = AuthorityRevision;
		Result.Operation = Fdemo_mapItemOperationResult::Failure(Code, Message, Result.SourceItemInstanceId);
		Result.Diagnostic = Message;
		return Result;
	};
	auto Commit = [&Result, this](Edemo_mapPlayerItemDropKind Kind, const Fdemo_mapItemOperationResult& Operation, FGuid TargetId = FGuid())
	{
		Result.Kind = Operation.bSuccess ? Kind : Edemo_mapPlayerItemDropKind::Reject;
		Result.bCommitted = Operation.bSuccess;
		Result.CommittedAuthorityRevision = AuthorityRevision;
		Result.TargetItemInstanceId = TargetId;
		Result.Operation = Operation;
		Result.Diagnostic = Operation.Diagnostic;
		return Result;
	};

	if (Intent.ExpectedAuthorityRevision != AuthorityRevision)
	{
		return Reject(Edemo_mapItemResultCode::UIInvalidSelection, TEXT("拖拽已过期：物品权威版本已变化。"));
	}

	FGuid SourceId;
	FName SourceEquipmentSlot = NAME_None;
	int32 SourceInventorySlot = INDEX_NONE;
	if (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment)
	{
		SourceEquipmentSlot = Intent.SourceEquipmentSlotId;
		if (!IsKnownEquipmentSlot(SourceEquipmentSlot))
			return Reject(Edemo_mapItemResultCode::InvalidSlot, TEXT("拖拽源装备槽无效。"));
		SourceId = EquipmentSlots.FindRef(SourceEquipmentSlot);
	}
	else if (!ResolveInventoryAreaSlot(Intent.SourceArea, Intent.SourceSlotIndex, SourceInventorySlot))
	{
		return Reject(Edemo_mapItemResultCode::InvalidSlot, TEXT("拖拽源物品格不在当前容量内。"));
	}
	else
	{
		SourceId = InventorySlots[SourceInventorySlot];
	}
	if (!SourceId.IsValid() || SourceId != Intent.ExpectedSourceItemInstanceId)
	{
		return Reject(Edemo_mapItemResultCode::UIInvalidSelection, TEXT("拖拽源物品已移动、为空或不再匹配。"));
	}

	FName TargetEquipmentSlot = NAME_None;
	int32 TargetInventorySlot = INDEX_NONE;
	if (Intent.TargetArea == Edemo_mapPlayerItemArea::Equipment)
	{
		TargetEquipmentSlot = Intent.TargetEquipmentSlotId;
		if (!IsKnownEquipmentSlot(TargetEquipmentSlot))
			return Reject(Edemo_mapItemResultCode::InvalidSlot, TEXT("拖拽目标装备槽无效。"));
	}
	else if (!ResolveInventoryAreaSlot(Intent.TargetArea, Intent.TargetSlotIndex, TargetInventorySlot))
	{
		return Reject(Edemo_mapItemResultCode::InvalidSlot, TEXT("拖拽目标物品格不在当前容量内。"));
	}

	if (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment)
	{
		if (Intent.TargetArea == Edemo_mapPlayerItemArea::Equipment)
		{
			if (SourceEquipmentSlot == TargetEquipmentSlot)
			{
				return Commit(Edemo_mapPlayerItemDropKind::Move, Fdemo_mapItemOperationResult::Success(SourceId));
			}
			const Fdemo_mapItemInstance* Source = FindInstance(SourceId);
			const Fdemo_mapItemDefinition* SourceDefinition = Source ? Fdemo_mapItemDefinitions::Find(Source->DefinitionId) : nullptr;
			if (!SourceDefinition || !SourceDefinition->CompatibleSlotIds.Contains(TargetEquipmentSlot))
				return Reject(Edemo_mapItemResultCode::IncompatibleSlot, TEXT("物品不能装备到目标槽。"));
			const FGuid TargetId = EquipmentSlots.FindRef(TargetEquipmentSlot);
			if (TargetId.IsValid())
			{
				const Fdemo_mapItemInstance* Target = FindInstance(TargetId);
				const Fdemo_mapItemDefinition* TargetDefinition = Target ? Fdemo_mapItemDefinitions::Find(Target->DefinitionId) : nullptr;
				if (!TargetDefinition || !TargetDefinition->CompatibleSlotIds.Contains(SourceEquipmentSlot))
					return Reject(Edemo_mapItemResultCode::IncompatibleSlot, TEXT("目标装备不能回填到源槽，交换已拒绝。"));
			}
			const Fdemo_mapItemAuthorityState Before = CaptureState();
			EquipmentSlots.FindChecked(SourceEquipmentSlot) = TargetId;
			EquipmentSlots.FindChecked(TargetEquipmentSlot) = SourceId;
			Instances.FindChecked(SourceId).EquippedSlotId = TargetEquipmentSlot;
			if (TargetId.IsValid()) Instances.FindChecked(TargetId).EquippedSlotId = SourceEquipmentSlot;
			return Commit(TargetId.IsValid() ? Edemo_mapPlayerItemDropKind::Swap : Edemo_mapPlayerItemDropKind::Equip,
				CommitOrRollback(Before, SourceId, Source->DefinitionId, TargetEquipmentSlot), TargetId);
		}

		const FGuid TargetId = InventorySlots[TargetInventorySlot];
		if (TargetId.IsValid())
		{
			const Fdemo_mapItemOperationResult Operation = Equip(TargetId, SourceEquipmentSlot);
			return Commit(Operation.bSuccess ? Edemo_mapPlayerItemDropKind::Swap : Edemo_mapPlayerItemDropKind::Reject, Operation, TargetId);
		}
		const Fdemo_mapItemAuthorityState Before = CaptureState();
		const int32 RevisionBefore = AuthorityRevision;
		const Fdemo_mapItemOperationResult Unequipped = Unequip(SourceEquipmentSlot);
		if (!Unequipped.bSuccess)
			return Commit(Edemo_mapPlayerItemDropKind::Reject, Unequipped);
		const int32 ProducedSlot = FindInventorySlot(SourceId);
		const Fdemo_mapItemOperationResult Moved = MoveInventorySlot(ProducedSlot, TargetInventorySlot);
		if (!Moved.bSuccess)
		{
			RestoreState(Before);
			AuthorityRevision = RevisionBefore;
			return Commit(Edemo_mapPlayerItemDropKind::Reject, Moved);
		}
		return Commit(Edemo_mapPlayerItemDropKind::Unequip, Moved);
	}

	// A runtime inventory item may equip, or remain within the two inventory areas.
	if (Intent.TargetArea == Edemo_mapPlayerItemArea::Equipment)
	{
		const Fdemo_mapItemOperationResult Operation = Equip(SourceId, TargetEquipmentSlot);
		return Commit(Operation.bSuccess ? Edemo_mapPlayerItemDropKind::Equip : Edemo_mapPlayerItemDropKind::Reject, Operation,
			EquipmentSlots.FindRef(TargetEquipmentSlot));
	}
	if (SourceInventorySlot == TargetInventorySlot)
	{
		return Commit(Edemo_mapPlayerItemDropKind::Move, Fdemo_mapItemOperationResult::Success(SourceId));
	}

	const FGuid TargetId = InventorySlots[TargetInventorySlot];
	if (!TargetId.IsValid())
	{
		return Commit(Edemo_mapPlayerItemDropKind::Move, MoveInventorySlot(SourceInventorySlot, TargetInventorySlot));
	}
	const Fdemo_mapItemInstance* Source = FindInstance(SourceId);
	const Fdemo_mapItemInstance* Target = FindInstance(TargetId);
	const Fdemo_mapItemDefinition* Definition = Source ? Fdemo_mapItemDefinitions::Find(Source->DefinitionId) : nullptr;
	if (!Source || !Target || !Definition)
		return Reject(Edemo_mapItemResultCode::InvariantViolation, TEXT("拖拽物品记录或定义缺失。"));
	const bool bCompatibleStacks =
		Source->AffixSet == Target->AffixSet
		&& Fdemo_mapRewardEventRules::AreStackCompatible(
			Source->DefinitionId, Source->RewardEventKind, Source->RewardEventId,
			Source->RewardValueMultiplierBps, Source->RewardSourceRoleId,
			Source->RareRewardEventId, Source->RareRewardPolicyId, Source->RareRewardTierId, Source->RareRewardBonusValue,
			Target->DefinitionId, Target->RewardEventKind, Target->RewardEventId,
			Target->RewardValueMultiplierBps, Target->RewardSourceRoleId,
			Target->RareRewardEventId, Target->RareRewardPolicyId, Target->RareRewardTierId, Target->RareRewardBonusValue);
	if (bCompatibleStacks && Target->Quantity < Definition->MaxStackSize)
	{
		const Fdemo_mapItemAuthorityState Before = CaptureState();
		const int32 Added = FMath::Min(Source->Quantity, Definition->MaxStackSize - Target->Quantity);
		int64 AddedBonus = 0;
		int64 RemainingBonus = 0;
		if (!Fdemo_mapRewardEventRules::TrySplitRareBonus(Source->Quantity, Added, Source->RareRewardBonusValue, AddedBonus, RemainingBonus))
			return Reject(Edemo_mapItemResultCode::InvariantViolation, TEXT("堆叠奖励数值分配失败。"));
		Fdemo_mapItemInstance& MutableSource = Instances.FindChecked(SourceId);
		Fdemo_mapItemInstance& MutableTarget = Instances.FindChecked(TargetId);
		MutableSource.Quantity -= Added;
		MutableSource.RareRewardBonusValue = RemainingBonus;
		MutableTarget.Quantity += Added;
		MutableTarget.RareRewardBonusValue += AddedBonus;
		if (MutableSource.Quantity == 0)
		{
			InventorySlots[SourceInventorySlot].Invalidate();
			MutableSource.OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
			MutableSource.OwnerId = NAME_None;
			MutableSource.ContainerId = NAME_None;
			MutableSource.EquippedSlotId = NAME_None;
		}
		return Commit(Edemo_mapPlayerItemDropKind::Merge,
			CommitOrRollback(Before, TargetId, MutableTarget.DefinitionId,
				FName(*FString::Printf(TEXT("Inventory.%d"), TargetInventorySlot))), TargetId);
	}
	return Commit(Edemo_mapPlayerItemDropKind::Swap, MoveInventorySlot(SourceInventorySlot, TargetInventorySlot), TargetId);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::DestroyContainer(
	FGuid InstanceId,
	FGuid ExpectedContainerId)
{
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (!Instance)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InstanceNotFound,
			TEXT("Container cleanup ItemInstance does not exist."),
			InstanceId);
	}
	const FName ExpectedContainerName = ExpectedContainerId.IsValid()
		? FName(*ExpectedContainerId.ToString(EGuidFormats::Digits))
		: NAME_None;
	if (Instance->OwnershipState == Edemo_mapItemOwnershipState::Destroyed)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::AlreadyDestroyed,
			TEXT("Container cleanup ItemInstance is already destroyed."),
			InstanceId,
			Instance->DefinitionId);
	}
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::Container
		|| Instance->ContainerId != ExpectedContainerName)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("Container cleanup cannot destroy an item it no longer owns."),
			InstanceId,
			Instance->DefinitionId);
	}
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName DefinitionId = Instance->DefinitionId;
	Instance->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
	Instance->OwnerId = NAME_None;
	Instance->ContainerId = NAME_None;
	Instance->EquippedSlotId = NAME_None;
	return CommitOrRollback(Before, InstanceId, DefinitionId, NAME_None);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::MoveInventoryToWorld(FGuid InstanceId)
{
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (Instance == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InstanceNotFound, TEXT("Inventory instance does not exist."), InstanceId);
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidOwnership, TEXT("Drop requires Inventory ownership."), InstanceId, Instance->DefinitionId);
	const int32 SlotIndex = FindInventorySlot(InstanceId);
	if (SlotIndex == INDEX_NONE) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, TEXT("Inventory-owned instance has no slot."), InstanceId, Instance->DefinitionId);
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName DefinitionId = Instance->DefinitionId;
	InventorySlots[SlotIndex].Invalidate();
	Instance = Instances.Find(InstanceId);
	Instance->OwnershipState = Edemo_mapItemOwnershipState::World;
	Instance->OwnerId = NAME_None;
	Instance->ContainerId = Fdemo_mapItemIds::WorldContainer;
	Instance->EquippedSlotId = NAME_None;
	if (!NormalizeInventorySlots(GetInventoryCapacity()))
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InternalRollbackFailed, TEXT("Drop could not normalize Run Inventory."), InstanceId, DefinitionId);
	}
	return CommitOrRollback(Before, InstanceId, DefinitionId, NAME_None);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::DiscardSpatialBundle(
	Fdemo_mapSpatialDiscardBundle& OutBundle,
	bool bFailAfterMutation)
{
	OutBundle = Fdemo_mapSpatialDiscardBundle();
	const FGuid SpatialItemId = EquipmentSlots.FindRef(Fdemo_mapItemIds::BackpackSlot);
	Fdemo_mapItemInstance* SpatialItem = Instances.Find(SpatialItemId);
	if (!SpatialItemId.IsValid() || !SpatialItem)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InstanceNotFound,
			TEXT("Spatial discard requires one equipped spatial item."),
			SpatialItemId,
			NAME_None,
			Fdemo_mapItemIds::BackpackSlot);
	}
	if (SpatialItem->OwnershipState != Edemo_mapItemOwnershipState::Equipped
		|| SpatialItem->EquippedSlotId != Fdemo_mapItemIds::BackpackSlot
		|| SpatialItem->Quantity != 1)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("Spatial discard found an inconsistent equipped spatial item."),
			SpatialItemId,
			SpatialItem->DefinitionId,
			Fdemo_mapItemIds::BackpackSlot);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName SpatialDefinitionId = SpatialItem->DefinitionId;
	OutBundle.BundleId = FGuid::NewGuid();
	OutBundle.SpatialItemInstanceId = SpatialItemId;
	const int32 BagStartIndex =
		Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
		+ GetRingQuickCapacity();
	for (int32 SlotIndex = BagStartIndex;
		SlotIndex < InventorySlots.Num();
		++SlotIndex)
	{
		const FGuid StoredId = InventorySlots[SlotIndex];
		if (!StoredId.IsValid()) continue;
		Fdemo_mapItemInstance* Stored = Instances.Find(StoredId);
		if (!Stored || Stored->OwnershipState != Edemo_mapItemOwnershipState::Inventory)
		{
			RestoreState(Before);
			OutBundle = Fdemo_mapSpatialDiscardBundle();
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvariantViolation,
				TEXT("A spatial storage cell did not contain a valid Inventory instance."),
				StoredId);
		}
		OutBundle.StoredItemInstanceIds.Add(StoredId);
		Stored->OwnershipState = Edemo_mapItemOwnershipState::World;
		Stored->OwnerId = NAME_None;
		Stored->ContainerId = Fdemo_mapItemIds::WorldContainer;
		Stored->EquippedSlotId = NAME_None;
		InventorySlots[SlotIndex].Invalidate();
	}

	EquipmentSlots.FindChecked(Fdemo_mapItemIds::BackpackSlot).Invalidate();
	SpatialItem = Instances.Find(SpatialItemId);
	SpatialItem->OwnershipState = Edemo_mapItemOwnershipState::World;
	SpatialItem->OwnerId = NAME_None;
	SpatialItem->ContainerId = Fdemo_mapItemIds::WorldContainer;
	SpatialItem->EquippedSlotId = NAME_None;
	InventorySlots.SetNum(BagStartIndex);

	if (bFailAfterMutation)
	{
		RestoreState(Before);
		OutBundle = Fdemo_mapSpatialDiscardBundle();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InternalRollbackFailed,
			TEXT("Automation-injected spatial discard failure restored the complete bundle."),
			SpatialItemId,
			SpatialDefinitionId,
			Fdemo_mapItemIds::BackpackSlot);
	}

	Fdemo_mapItemOperationResult Result = CommitOrRollback(
		Before,
		SpatialItemId,
		SpatialDefinitionId,
		Fdemo_mapItemIds::BackpackSlot);
	if (!Result.bSuccess) OutBundle = Fdemo_mapSpatialDiscardBundle();
	return Result;
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::RecoverSpatialBundle(
	const Fdemo_mapSpatialDiscardBundle& Bundle,
	bool bFailAfterMutation)
{
	if (!Bundle.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidWorldBinding,
			TEXT("Spatial recovery requires a valid bundle identity."));
	}
	if (EquipmentSlots.FindRef(Fdemo_mapItemIds::BackpackSlot).IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::SlotOccupied,
			TEXT("Spatial recovery requires an empty spatial-item equipment slot."),
			Bundle.SpatialItemInstanceId,
			NAME_None,
			Fdemo_mapItemIds::BackpackSlot);
	}
	Fdemo_mapItemInstance* SpatialItem = Instances.Find(Bundle.SpatialItemInstanceId);
	const Fdemo_mapItemDefinition* SpatialDefinition = SpatialItem
		? Fdemo_mapItemDefinitions::Find(SpatialItem->DefinitionId)
		: nullptr;
	if (!SpatialItem || !SpatialDefinition
		|| SpatialItem->OwnershipState != Edemo_mapItemOwnershipState::World
		|| SpatialDefinition->CategoryId != Fdemo_mapItemIds::BackpackCategory
		|| !SpatialDefinition->CompatibleSlotIds.Contains(Fdemo_mapItemIds::BackpackSlot))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidOwnership,
			TEXT("The discarded spatial item is missing or no longer World-owned."),
			Bundle.SpatialItemInstanceId,
			SpatialItem ? SpatialItem->DefinitionId : NAME_None,
			Fdemo_mapItemIds::BackpackSlot);
	}

	TSet<FGuid> UniqueIds;
	UniqueIds.Add(Bundle.SpatialItemInstanceId);
	for (const FGuid StoredId : Bundle.StoredItemInstanceIds)
	{
		const Fdemo_mapItemInstance* Stored = Instances.Find(StoredId);
		if (!Stored || Stored->OwnershipState != Edemo_mapItemOwnershipState::World || UniqueIds.Contains(StoredId))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InvalidWorldBinding,
				TEXT("Spatial recovery requires every original stored instance exactly once."),
				StoredId);
		}
		UniqueIds.Add(StoredId);
	}

	const Fdemo_mapInventoryCapacityResult Capacity = PreflightInventoryCapacity(
		GetUsedInventorySlots() + Bundle.StoredItemInstanceIds.Num(),
		SpatialItem->DefinitionId);
	if (!Capacity.bSuccess || !Capacity.bFits)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			Capacity.Diagnostic,
			Bundle.SpatialItemInstanceId,
			SpatialItem->DefinitionId,
			Fdemo_mapItemIds::BackpackSlot);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	TArray<FGuid> OrderedInventory;
	for (const FGuid Id : InventorySlots) if (Id.IsValid()) OrderedInventory.Add(Id);
	OrderedInventory.Append(Bundle.StoredItemInstanceIds);
	InventorySlots.Init(FGuid(), Capacity.Capacity);
	for (int32 Index = 0; Index < OrderedInventory.Num(); ++Index)
	{
		const FGuid Id = OrderedInventory[Index];
		InventorySlots[Index] = Id;
		Fdemo_mapItemInstance* Stored = Instances.Find(Id);
		Stored->OwnershipState = Edemo_mapItemOwnershipState::Inventory;
		Stored->OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
		Stored->ContainerId = Fdemo_mapItemIds::InventoryContainer;
		Stored->EquippedSlotId = NAME_None;
	}
	EquipmentSlots.FindChecked(Fdemo_mapItemIds::BackpackSlot) = Bundle.SpatialItemInstanceId;
	SpatialItem = Instances.Find(Bundle.SpatialItemInstanceId);
	SpatialItem->OwnershipState = Edemo_mapItemOwnershipState::Equipped;
	SpatialItem->OwnerId = Fdemo_mapItemIds::LocalPlayerOwner;
	SpatialItem->ContainerId = Fdemo_mapItemIds::EquipmentContainer;
	SpatialItem->EquippedSlotId = Fdemo_mapItemIds::BackpackSlot;

	if (bFailAfterMutation)
	{
		const FName DefinitionId = SpatialItem->DefinitionId;
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InternalRollbackFailed,
			TEXT("Automation-injected spatial recovery failure restored World ownership."),
			Bundle.SpatialItemInstanceId,
			DefinitionId,
			Fdemo_mapItemIds::BackpackSlot);
	}
	return CommitOrRollback(
		Before,
		Bundle.SpatialItemInstanceId,
		SpatialItem->DefinitionId,
		Fdemo_mapItemIds::BackpackSlot);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::DestroyWorld(FGuid InstanceId)
{
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (Instance == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InstanceNotFound, TEXT("World instance does not exist."), InstanceId);
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::World) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidOwnership, TEXT("World cleanup requires World ownership."), InstanceId, Instance->DefinitionId);
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName DefinitionId = Instance->DefinitionId;
	Instance->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
	Instance->OwnerId = NAME_None;
	Instance->ContainerId = NAME_None;
	Instance->EquippedSlotId = NAME_None;
	return CommitOrRollback(Before, InstanceId, DefinitionId, NAME_None);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::Equip(FGuid InstanceId, FName SlotId)
{
	if (!IsKnownEquipmentSlot(SlotId)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidSlot, TEXT("Equipment slot is not registered."), InstanceId, NAME_None, SlotId);
	Fdemo_mapItemInstance* NewInstance = Instances.Find(InstanceId);
	if (NewInstance == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InstanceNotFound, TEXT("Instance does not exist."), InstanceId, NAME_None, SlotId);
	if (NewInstance->OwnershipState == Edemo_mapItemOwnershipState::Destroyed) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::AlreadyDestroyed, TEXT("Destroyed instances cannot be equipped."), InstanceId, NewInstance->DefinitionId, SlotId);
	if (NewInstance->OwnershipState != Edemo_mapItemOwnershipState::Inventory || FindInventorySlot(InstanceId) == INDEX_NONE) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidOwnership, TEXT("Equip requires an Inventory instance."), InstanceId, NewInstance->DefinitionId, SlotId);
	if (NewInstance->Quantity != 1) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidQuantity, TEXT("Equipped instance quantity must equal one."), InstanceId, NewInstance->DefinitionId, SlotId);
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(NewInstance->DefinitionId);
	if (Definition == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::UnknownDefinition, TEXT("Instance definition is missing."), InstanceId, NewInstance->DefinitionId, SlotId);
	if (Definition->CompatibleSlotIds.IsEmpty()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::NotEquipable, TEXT("Definition has no compatible equipment slot."), InstanceId, Definition->DefinitionId, SlotId);
	if (!Definition->CompatibleSlotIds.Contains(SlotId)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::IncompatibleSlot, TEXT("Definition is incompatible with the requested slot."), InstanceId, Definition->DefinitionId, SlotId);

	const int32 UsedBefore = GetUsedInventorySlots();
	const FGuid OldInstanceId = EquipmentSlots.FindRef(SlotId);
	const FName TargetBackpackDefinitionId = SlotId == Fdemo_mapItemIds::BackpackSlot
		? Definition->DefinitionId
		: GetEquippedBackpackDefinitionId();
	const FName TargetSpatialRingDefinitionId = SlotId == Fdemo_mapItemIds::SpatialRingSlot
		? Definition->DefinitionId
		: GetEquippedSpatialRingDefinitionId();
	const int32 ProjectedUsed = OldInstanceId.IsValid() ? UsedBefore : UsedBefore - 1;
	const Fdemo_mapInventoryCapacityResult CapacityPreflight = PreflightInventoryCapacity(
		ProjectedUsed,
		TargetBackpackDefinitionId,
		TargetSpatialRingDefinitionId);
	if (!CapacityPreflight.bSuccess || !CapacityPreflight.bFits)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			CapacityPreflight.Diagnostic,
			InstanceId,
			Definition->DefinitionId,
			SlotId);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const int32 ReleasedInventorySlot = FindInventorySlot(InstanceId);
	InventorySlots[ReleasedInventorySlot].Invalidate();
	if (OldInstanceId.IsValid())
	{
		Fdemo_mapItemInstance* OldInstance = Instances.Find(OldInstanceId);
		if (OldInstance == nullptr || OldInstance->OwnershipState != Edemo_mapItemOwnershipState::Equipped)
		{
			RestoreState(Before);
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, TEXT("Occupied slot did not reference a valid equipped instance."), InstanceId, Definition->DefinitionId, SlotId);
		}
		InventorySlots[ReleasedInventorySlot] = OldInstanceId;
		OldInstance->OwnershipState = Edemo_mapItemOwnershipState::Inventory;
		OldInstance->ContainerId = Fdemo_mapItemIds::InventoryContainer;
		OldInstance->EquippedSlotId = NAME_None;
	}
	EquipmentSlots.FindChecked(SlotId) = InstanceId;
	NewInstance = Instances.Find(InstanceId);
	NewInstance->OwnershipState = Edemo_mapItemOwnershipState::Equipped;
	NewInstance->ContainerId = Fdemo_mapItemIds::EquipmentContainer;
	NewInstance->EquippedSlotId = SlotId;
	if (!NormalizeInventorySlots(CapacityPreflight.Capacity))
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InternalRollbackFailed,
			TEXT("Equipment preflight succeeded but inventory normalization failed."),
			InstanceId,
			Definition->DefinitionId,
			SlotId);
	}
	return CommitOrRollback(Before, InstanceId, Definition->DefinitionId, SlotId);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::Unequip(FName SlotId)
{
	if (!IsKnownEquipmentSlot(SlotId)) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidSlot, TEXT("Equipment slot is not registered."), FGuid(), NAME_None, SlotId);
	const FGuid InstanceId = EquipmentSlots.FindRef(SlotId);
	if (!InstanceId.IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InstanceNotFound, TEXT("Equipment slot is empty."), FGuid(), NAME_None, SlotId);
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (Instance == nullptr || Instance->OwnershipState != Edemo_mapItemOwnershipState::Equipped) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidOwnership, TEXT("Equipment slot ownership is inconsistent."), InstanceId, NAME_None, SlotId);

	const FName TargetBackpackDefinitionId = SlotId == Fdemo_mapItemIds::BackpackSlot
		? NAME_None
		: GetEquippedBackpackDefinitionId();
	const FName TargetSpatialRingDefinitionId = SlotId == Fdemo_mapItemIds::SpatialRingSlot
		? NAME_None
		: GetEquippedSpatialRingDefinitionId();
	const Fdemo_mapInventoryCapacityResult CapacityPreflight = PreflightInventoryCapacity(
		GetUsedInventorySlots() + 1,
		TargetBackpackDefinitionId,
		TargetSpatialRingDefinitionId);
	if (!CapacityPreflight.bSuccess || !CapacityPreflight.bFits)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InventoryFull,
			CapacityPreflight.Diagnostic,
			InstanceId,
			Instance->DefinitionId,
			SlotId);
	}

	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const int32 InventorySlot = FindFirstEmptyInventorySlot();
	EquipmentSlots.FindChecked(SlotId).Invalidate();
	InventorySlots[InventorySlot] = InstanceId;
	Instance = Instances.Find(InstanceId);
	Instance->OwnershipState = Edemo_mapItemOwnershipState::Inventory;
	Instance->ContainerId = Fdemo_mapItemIds::InventoryContainer;
	Instance->EquippedSlotId = NAME_None;
	if (!NormalizeInventorySlots(CapacityPreflight.Capacity))
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InternalRollbackFailed,
			TEXT("Unequip preflight succeeded but inventory normalization failed."),
			InstanceId,
			Instance->DefinitionId,
			SlotId);
	}
	return CommitOrRollback(Before, InstanceId, Instance->DefinitionId, SlotId);
}

Fdemo_mapItemOperationResult Fdemo_mapItemAuthority::Destroy(FGuid InstanceId)
{
	Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
	if (Instance == nullptr) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InstanceNotFound, TEXT("Instance does not exist."), InstanceId);
	if (Instance->OwnershipState == Edemo_mapItemOwnershipState::Destroyed) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::AlreadyDestroyed, TEXT("Instance is already destroyed."), InstanceId, Instance->DefinitionId);
	if (Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidOwnership, TEXT("Normal destroy requires Inventory ownership."), InstanceId, Instance->DefinitionId);
	const int32 SlotIndex = FindInventorySlot(InstanceId);
	if (SlotIndex == INDEX_NONE) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, TEXT("Inventory-owned instance is missing from inventory."), InstanceId, Instance->DefinitionId);
	const Fdemo_mapItemAuthorityState Before = CaptureState();
	const FName DefinitionId = Instance->DefinitionId;
	InventorySlots[SlotIndex].Invalidate();
	Instance = Instances.Find(InstanceId);
	Instance->OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
	Instance->OwnerId = NAME_None;
	Instance->ContainerId = NAME_None;
	Instance->EquippedSlotId = NAME_None;
	if (!NormalizeInventorySlots(GetInventoryCapacity()))
	{
		RestoreState(Before);
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InternalRollbackFailed, TEXT("Destroy could not normalize Run Inventory."), InstanceId, DefinitionId);
	}
	return CommitOrRollback(Before, InstanceId, DefinitionId, NAME_None);
}

TArray<FGuid> Fdemo_mapItemAuthority::FindWorldInstances() const
{
	TArray<FGuid> Result;
	for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair : Instances)
	{
		if (Pair.Value.OwnershipState == Edemo_mapItemOwnershipState::World) Result.Add(Pair.Key);
	}
	Result.Sort([](const FGuid& A, const FGuid& B) { return A.ToString(EGuidFormats::Digits) < B.ToString(EGuidFormats::Digits); });
	return Result;
}

bool Fdemo_mapItemAuthority::ValidateInvariants(FString* OutError) const
{
	auto Fail = [OutError](const FString& Message) { if (OutError) *OutError = Message; return false; };
	FString DefinitionError;
	if (!Fdemo_mapItemDefinitions::Validate(&DefinitionError)) return Fail(DefinitionError);
	const Fdemo_mapInventoryCapacityResult CapacityResult = GetInventoryCapacityResult();
	if (!CapacityResult.bSuccess) return Fail(CapacityResult.Diagnostic);
	if (InventorySlots.Num() != CapacityResult.Capacity) return Fail(TEXT("Inventory slot count does not match its definition-backed capacity."));
	if (EquipmentSlots.Num() != Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num()) return Fail(TEXT("Equipment slot count is invalid."));

	TSet<FGuid> ActiveIds;
	TMap<FGuid, int32> InventoryOccurrences;
	TMap<FGuid, int32> EquipmentOccurrences;
	TMap<FGuid, int32> StashOccurrences;
	TMap<FGuid, const Fdemo_mapItemInstance*> RewardEvents;
	TMap<FGuid, const Fdemo_mapItemInstance*> RareRewardEvents;
	for (const FGuid& InstanceId : InventorySlots)
	{
		if (!InstanceId.IsValid())
		{
			continue;
		}
		if (ActiveIds.Contains(InstanceId)) return Fail(TEXT("Duplicate active instance ID in inventory."));
		ActiveIds.Add(InstanceId);
		InventoryOccurrences.FindOrAdd(InstanceId)++;
		const Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
		if (Instance == nullptr || Instance->OwnershipState != Edemo_mapItemOwnershipState::Inventory || Instance->ContainerId != Fdemo_mapItemIds::InventoryContainer || !Instance->EquippedSlotId.IsNone()) return Fail(TEXT("Inventory ownership metadata is inconsistent."));
	}
	for (FName RequiredSlot : Fdemo_mapItemDefinitions::GetEquipmentSlotIds())
	{
		const FGuid* InstanceId = EquipmentSlots.Find(RequiredSlot);
		if (InstanceId == nullptr) return Fail(TEXT("Required equipment slot is missing."));
		if (!InstanceId->IsValid()) continue;
		if (ActiveIds.Contains(*InstanceId)) return Fail(TEXT("Instance is present in multiple active containers."));
		ActiveIds.Add(*InstanceId);
		EquipmentOccurrences.FindOrAdd(*InstanceId)++;
		const Fdemo_mapItemInstance* Instance = Instances.Find(*InstanceId);
		const Fdemo_mapItemDefinition* Definition = Instance ? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId) : nullptr;
		if (Instance == nullptr || Definition == nullptr || Instance->OwnershipState != Edemo_mapItemOwnershipState::Equipped || Instance->ContainerId != Fdemo_mapItemIds::EquipmentContainer || Instance->EquippedSlotId != RequiredSlot || Instance->Quantity != 1 || !Definition->CompatibleSlotIds.Contains(RequiredSlot)) return Fail(TEXT("Equipment ownership or compatibility is inconsistent."));
	}
	for (const FGuid& InstanceId : SessionStash)
	{
		if (!InstanceId.IsValid() || ActiveIds.Contains(InstanceId)) return Fail(TEXT("Invalid or duplicate active instance ID in Session Stash."));
		ActiveIds.Add(InstanceId);
		StashOccurrences.FindOrAdd(InstanceId)++;
		const Fdemo_mapItemInstance* Instance = Instances.Find(InstanceId);
		if (!Instance || Instance->OwnershipState != Edemo_mapItemOwnershipState::SessionStash || Instance->ContainerId != Fdemo_mapItemIds::SessionStashContainer || !Instance->EquippedSlotId.IsNone()) return Fail(TEXT("Session Stash ownership metadata is inconsistent."));
	}
	for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair : Instances)
	{
		const Fdemo_mapItemInstance& Instance = Pair.Value;
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Instance.DefinitionId);
		const bool bQuantityValid = Definition != nullptr
			&& Instance.Quantity <= Definition->MaxStackSize
			&& (Instance.OwnershipState == Edemo_mapItemOwnershipState::Destroyed
				? Instance.Quantity >= 0
				: Instance.Quantity > 0);
		if (!Pair.Key.IsValid() || Pair.Key != Instance.InstanceId || !bQuantityValid) return Fail(TEXT("Instance identity, definition, or quantity is invalid."));
		FString RewardError;
		if (!Fdemo_mapRewardEventRules::IsValid(
			Instance.RewardEventKind,
			Instance.RewardEventId,
			Instance.RewardValueMultiplierBps,
			Instance.RewardSourceRoleId,
			Instance.RareRewardEventId,
			Instance.RareRewardPolicyId,
			Instance.RareRewardTierId,
			Instance.RareRewardBonusValue,
			&RewardError))
		{
			return Fail(RewardError);
		}
		if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
			Instance.DefinitionId,
			Instance.Quantity,
			Instance.AffixSet,
			&RewardError))
		{
			return Fail(RewardError);
		}
		if (Instance.RewardEventId.IsValid())
		{
			if (const Fdemo_mapItemInstance* const* Existing =
				RewardEvents.Find(Instance.RewardEventId))
			{
				if (!Fdemo_mapRewardEventRules::AreStackCompatible(
					(*Existing)->DefinitionId,
					(*Existing)->RewardEventKind,
					(*Existing)->RewardEventId,
					(*Existing)->RewardValueMultiplierBps,
					(*Existing)->RewardSourceRoleId,
					Instance.DefinitionId,
					Instance.RewardEventKind,
					Instance.RewardEventId,
					Instance.RewardValueMultiplierBps,
					Instance.RewardSourceRoleId))
				{
					return Fail(TEXT("RewardEventId conflicts across incompatible instances."));
				}
			}
			else
			{
				RewardEvents.Add(Instance.RewardEventId, &Instance);
			}
		}
		if (Instance.RareRewardEventId.IsValid())
		{
			if (const Fdemo_mapItemInstance* const* Existing =
				RareRewardEvents.Find(Instance.RareRewardEventId))
			{
				if ((*Existing)->RareRewardPolicyId
						!= Instance.RareRewardPolicyId
					|| (*Existing)->RareRewardTierId
						!= Instance.RareRewardTierId
					|| (*Existing)->RewardSourceRoleId
						!= Instance.RewardSourceRoleId)
				{
					return Fail(TEXT("RareRewardEventId conflicts across carrier provenance."));
				}
			}
			else
			{
				RareRewardEvents.Add(
					Instance.RareRewardEventId,
					&Instance);
			}
		}
		const int32 InventoryCount = InventoryOccurrences.FindRef(Pair.Key);
		const int32 EquipmentCount = EquipmentOccurrences.FindRef(Pair.Key);
		const int32 StashCount = StashOccurrences.FindRef(Pair.Key);
		switch (Instance.OwnershipState)
		{
		case Edemo_mapItemOwnershipState::Inventory:
			if (InventoryCount != 1 || EquipmentCount != 0 || StashCount != 0) return Fail(TEXT("Inventory instance occurrence count is invalid."));
			break;
		case Edemo_mapItemOwnershipState::Equipped:
			if (InventoryCount != 0 || EquipmentCount != 1 || StashCount != 0) return Fail(TEXT("Equipped instance occurrence count is invalid."));
			break;
		case Edemo_mapItemOwnershipState::World:
			if (InventoryCount != 0 || EquipmentCount != 0 || StashCount != 0 || Instance.ContainerId != Fdemo_mapItemIds::WorldContainer || !Instance.EquippedSlotId.IsNone()) return Fail(TEXT("World ownership metadata is inconsistent."));
			break;
		case Edemo_mapItemOwnershipState::Container:
			if (InventoryCount != 0
				|| EquipmentCount != 0
				|| StashCount != 0
				|| Instance.ContainerId.IsNone()
				|| !Instance.EquippedSlotId.IsNone()
				|| !Instance.OriginRunId.IsValid())
			{
				return Fail(TEXT("Runtime Container ownership metadata is inconsistent."));
			}
			break;
		case Edemo_mapItemOwnershipState::Destroyed:
			if (InventoryCount != 0 || EquipmentCount != 0 || StashCount != 0 || !Instance.ContainerId.IsNone() || !Instance.EquippedSlotId.IsNone()) return Fail(TEXT("Destroyed instance remains active."));
			break;
		case Edemo_mapItemOwnershipState::SessionStash:
			if (InventoryCount != 0 || EquipmentCount != 0 || StashCount != 1 || Instance.ContainerId != Fdemo_mapItemIds::SessionStashContainer || !Instance.EquippedSlotId.IsNone()) return Fail(TEXT("Session Stash occurrence count is invalid."));
			break;
		default: return Fail(TEXT("Unknown ownership state."));
		}
	}
	return true;
}
