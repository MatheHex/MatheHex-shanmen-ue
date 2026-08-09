#include "demo_mapPersistentWarehouseTransaction.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"

namespace
{
	Fdemo_mapWarehouseMoveResult RejectWarehouseMove(
		Edemo_mapWarehouseMoveStatus Status,
		const FString& Diagnostic,
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapWarehouseMoveResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.PreviousGeneration = Profile.SaveGeneration;
		Result.CommittedGeneration = Profile.SaveGeneration;
		return Result;
	}
}

TArray<FGuid> Fdemo_mapWarehouseSlotProjection::Build(
	const Fdemo_mapPersistentProfile& Profile,
	bool* OutOverflow)
{
	TArray<FGuid> Slots;
	Slots.Init(
		FGuid(),
		Fdemo_mapPersistentWarehouseLayout::SlotCount);
	TSet<FGuid> OwnedIds;
	for (const Fdemo_mapPersistentItemRecord& Item :
		Profile.PermanentStash)
	{
		if (Item.ItemInstanceId.IsValid())
		{
			OwnedIds.Add(Item.ItemInstanceId);
		}
	}

	TSet<FGuid> PlacedIds;
	if (Profile.WarehouseLayout.bInitialized
		&& Profile.WarehouseLayout.SlotItemInstanceIds.Num()
			== Slots.Num())
	{
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			const FGuid Hint =
				Profile.WarehouseLayout.SlotItemInstanceIds[Index];
			if (Hint.IsValid()
				&& OwnedIds.Contains(Hint)
				&& !PlacedIds.Contains(Hint))
			{
				Slots[Index] = Hint;
				PlacedIds.Add(Hint);
			}
		}
	}

	bool bOverflow = false;
	for (const Fdemo_mapPersistentItemRecord& Item :
		Profile.PermanentStash)
	{
		if (PlacedIds.Contains(Item.ItemInstanceId))
		{
			continue;
		}
		FGuid* Empty = Slots.FindByPredicate(
			[](const FGuid& ItemId)
			{
				return !ItemId.IsValid();
			});
		if (!Empty)
		{
			bOverflow = true;
			break;
		}
		*Empty = Item.ItemInstanceId;
		PlacedIds.Add(Item.ItemInstanceId);
	}
	if (OutOverflow)
	{
		*OutOverflow = bOverflow;
	}
	return Slots;
}

Fdemo_mapWarehouseMoveResult
Fdemo_mapPersistentWarehouseTransaction::Execute(
	Fdemo_mapPersistentProfile& InOutProfile,
	const Fdemo_mapWarehouseMoveIntent& Intent,
	const Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	if (Intent.ExpectedProfileId != InOutProfile.ProfileId
		|| Intent.ExpectedSaveGeneration != InOutProfile.SaveGeneration)
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::StaleIntent,
			TEXT("Warehouse move intent has stale ProfileId or SaveGeneration."),
			InOutProfile);
	}
	if (InOutProfile.ActiveRun.bHasActiveRun)
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::SessionNotReady,
			TEXT("Warehouse position changes require ReadyForPreparation."),
			InOutProfile);
	}
	if (Intent.SourceSlotIndex < 0
		|| Intent.SourceSlotIndex
			>= Fdemo_mapPersistentWarehouseLayout::SlotCount
		|| Intent.TargetSlotIndex < 0
		|| Intent.TargetSlotIndex
			>= Fdemo_mapPersistentWarehouseLayout::SlotCount)
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::InvalidSlot,
			TEXT("Warehouse move requires source and target slots in 0..29."),
			InOutProfile);
	}
	if (Intent.SourceSlotIndex == Intent.TargetSlotIndex)
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::NoOp,
			TEXT("Warehouse source and target already match."),
			InOutProfile);
	}

	bool bOverflow = false;
	TArray<FGuid> Slots =
		Fdemo_mapWarehouseSlotProjection::Build(
			InOutProfile,
			&bOverflow);
	if (bOverflow)
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::CapacityExceeded,
			TEXT("Permanent Stash contains more than the configured 30 warehouse cells."),
			InOutProfile);
	}
	if (!Slots[Intent.SourceSlotIndex].IsValid())
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::SourceEmpty,
			TEXT("Warehouse source slot is empty."),
			InOutProfile);
	}
	Fdemo_mapPersistentProfile Candidate = InOutProfile;
	bool bMerged = false;
	const FGuid SourceId = Slots[Intent.SourceSlotIndex];
	const FGuid TargetId = Slots[Intent.TargetSlotIndex];
	const Fdemo_mapPersistentItemRecord* SourceRecord =
		InOutProfile.PermanentStash.FindByPredicate(
			[&SourceId](const Fdemo_mapPersistentItemRecord& Item)
			{ return Item.ItemInstanceId == SourceId; });
	const Fdemo_mapPersistentItemRecord* TargetRecord = TargetId.IsValid()
		? InOutProfile.PermanentStash.FindByPredicate(
			[&TargetId](const Fdemo_mapPersistentItemRecord& Item)
			{ return Item.ItemInstanceId == TargetId; })
		: nullptr;
	const Fdemo_mapItemDefinition* Definition = SourceRecord
		? Fdemo_mapItemDefinitions::Find(SourceRecord->ItemDefinitionId) : nullptr;
	const bool bCompatibleStacks = SourceRecord && TargetRecord && Definition
		&& SourceRecord->AffixSet == TargetRecord->AffixSet
		&& Fdemo_mapRewardEventRules::AreStackCompatible(
			SourceRecord->ItemDefinitionId, SourceRecord->RewardEventKind,
			SourceRecord->RewardEventId, SourceRecord->RewardValueMultiplierBps,
			SourceRecord->RewardSourceRoleId, SourceRecord->RareRewardEventId,
			SourceRecord->RareRewardPolicyId, SourceRecord->RareRewardTierId,
			SourceRecord->RareRewardBonusValue, TargetRecord->ItemDefinitionId,
			TargetRecord->RewardEventKind, TargetRecord->RewardEventId,
			TargetRecord->RewardValueMultiplierBps,
			TargetRecord->RewardSourceRoleId, TargetRecord->RareRewardEventId,
			TargetRecord->RareRewardPolicyId, TargetRecord->RareRewardTierId,
			TargetRecord->RareRewardBonusValue);
	if (bCompatibleStacks && TargetRecord->StackCount < Definition->MaxStackSize)
	{
		Fdemo_mapPersistentItemRecord* MutableSource =
			Candidate.PermanentStash.FindByPredicate(
				[&SourceId](const Fdemo_mapPersistentItemRecord& Item)
				{ return Item.ItemInstanceId == SourceId; });
		Fdemo_mapPersistentItemRecord* MutableTarget =
			Candidate.PermanentStash.FindByPredicate(
				[&TargetId](const Fdemo_mapPersistentItemRecord& Item)
				{ return Item.ItemInstanceId == TargetId; });
		const int32 Added = FMath::Min(
			MutableSource->StackCount,
			Definition->MaxStackSize - MutableTarget->StackCount);
		int64 AddedBonus = 0;
		int64 RemainingBonus = 0;
		if (!Fdemo_mapRewardEventRules::TrySplitRareBonus(
			MutableSource->StackCount, Added, MutableSource->RareRewardBonusValue,
			AddedBonus, RemainingBonus))
		{
			return RejectWarehouseMove(
				Edemo_mapWarehouseMoveStatus::RepositorySaveRejected,
				TEXT("Warehouse stack merge bonus split rejected."), InOutProfile);
		}
		MutableSource->StackCount -= Added;
		MutableSource->RareRewardBonusValue = RemainingBonus;
		MutableTarget->StackCount += Added;
		MutableTarget->RareRewardBonusValue += AddedBonus;
		if (MutableSource->StackCount == 0)
		{
			Candidate.PermanentStash.RemoveAll([&SourceId](const Fdemo_mapPersistentItemRecord& Item)
			{ return Item.ItemInstanceId == SourceId; });
			for (FGuid& Id : Slots) if (Id == SourceId) Id.Invalidate();
			auto ClearSelected = [&SourceId](FGuid& Id) { if (Id == SourceId) Id.Invalidate(); };
			ClearSelected(Candidate.PreparationLayout.WeaponItemInstanceId);
			ClearSelected(Candidate.PreparationLayout.ArmorItemInstanceId);
			ClearSelected(Candidate.PreparationLayout.AccessoryItemInstanceId);
			ClearSelected(Candidate.PreparationLayout.BackpackItemInstanceId);
			Candidate.PreparationLayout.OrderedRunInventoryItemInstanceIds.Remove(SourceId);
			for (FGuid& Id : Candidate.PreparationLayout.HotbarItemInstanceIds)
				ClearSelected(Id);
		}
		bMerged = true;
	}
	else
	{
		Slots.Swap(Intent.SourceSlotIndex, Intent.TargetSlotIndex);
	}
	Candidate.WarehouseLayout.bInitialized = true;
	Candidate.WarehouseLayout.SlotItemInstanceIds = MoveTemp(Slots);
	FString ValidationError;
	if (!Repository.ValidateProfile(Candidate, &ValidationError))
	{
		return RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::RepositorySaveRejected,
			TEXT("Warehouse candidate rejected: ") + ValidationError,
			InOutProfile);
	}

	Fdemo_mapPersistentProfile Committed = Candidate;
	const Fdemo_mapProfileSaveResult Save =
		Repository.SaveProfile(Committed, Storage);
	Fdemo_mapWarehouseMoveResult Result =
		RejectWarehouseMove(
			Edemo_mapWarehouseMoveStatus::RepositorySaveRejected,
			Save.Diagnostic,
			InOutProfile);
	Result.bDiskStateChanged = Save.bDiskStateChanged;
	if (Save.Status
		== Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Result.Status =
			Edemo_mapWarehouseMoveStatus::CommitOutcomeRequiresReload;
		return Result;
	}
	if (!Save.IsSuccess())
	{
		return Result;
	}

	InOutProfile = Committed;
	Result.Status = Edemo_mapWarehouseMoveStatus::Committed;
	Result.Diagnostic =
		bMerged
			? TEXT("Warehouse stack merge committed; target item identity was retained.")
			: TEXT("Warehouse slot move committed without changing item identity or quantity.");
	Result.CommittedGeneration = Committed.SaveGeneration;
	Result.CommittedProfile = Committed;
	return Result;
}
