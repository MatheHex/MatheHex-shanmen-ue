#include "demo_mapItemTypes.h"
#include "demo_mapItemDefinitions.h"

Fdemo_mapGridContainerSnapshot Fdemo_mapItemViewRules::BuildGridFromOccupiedOrder(
	const TArray<FGuid>& OrderedOccupiedItemIds,
	int32 Capacity)
{
	Fdemo_mapGridContainerSnapshot Snapshot;
	Snapshot.Capacity = Capacity;
	Snapshot.UsedSlots = OrderedOccupiedItemIds.Num();
	if (Capacity < 0 || OrderedOccupiedItemIds.Num() > Capacity)
	{
		Snapshot.Diagnostic = TEXT("Grid projection exceeds its declared capacity.");
		return Snapshot;
	}

	TSet<FGuid> Seen;
	for (const FGuid& ItemInstanceId : OrderedOccupiedItemIds)
	{
		if (!ItemInstanceId.IsValid() || Seen.Contains(ItemInstanceId))
		{
			Snapshot.Diagnostic = TEXT("Grid projection contains an invalid or duplicate ItemInstanceId.");
			return Snapshot;
		}
		Seen.Add(ItemInstanceId);
	}

	Snapshot.OrderedSlots.Reserve(Capacity);
	for (int32 Index = 0; Index < Capacity; ++Index)
	{
		Fdemo_mapGridSlotView Slot;
		Slot.SlotIndex = Index;
		if (OrderedOccupiedItemIds.IsValidIndex(Index))
		{
			Slot.ItemInstanceId = OrderedOccupiedItemIds[Index];
		}
		Snapshot.OrderedSlots.Add(Slot);
	}
	Snapshot.bValid = true;
	return Snapshot;
}

bool Fdemo_mapItemViewRules::IsHotbarBindable(
	const Fdemo_mapItemInstance& Instance,
	const Fdemo_mapItemDefinition& Definition)
{
	return Instance.InstanceId.IsValid()
		&& Instance.DefinitionId == Definition.DefinitionId
		&& Instance.OwnershipState == Edemo_mapItemOwnershipState::Inventory
		&& Instance.Quantity > 0
		&& Instance.Quantity <= Definition.MaxStackSize
		&& Definition.CategoryId == Fdemo_mapItemIds::ConsumableCategory;
}

Fdemo_mapItemOperationResult Fdemo_mapItemOperationResult::Success(FGuid InstanceId, FName DefinitionId, FName SlotId, FName ActorId)
{
	Fdemo_mapItemOperationResult Result;
	Result.RelatedInstanceId = InstanceId;
	Result.RelatedDefinitionId = DefinitionId;
	Result.RelatedSlotId = SlotId;
	Result.RelatedActorId = ActorId;
	return Result;
}

Fdemo_mapItemOperationResult Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode ResultCode, const FString& Message, FGuid InstanceId, FName DefinitionId, FName SlotId, FName ActorId)
{
	Fdemo_mapItemOperationResult Result;
	Result.Code = ResultCode;
	Result.bSuccess = false;
	Result.RelatedInstanceId = InstanceId;
	Result.RelatedDefinitionId = DefinitionId;
	Result.RelatedSlotId = SlotId;
	Result.RelatedActorId = ActorId;
	Result.Diagnostic = Message;
	return Result;
}
