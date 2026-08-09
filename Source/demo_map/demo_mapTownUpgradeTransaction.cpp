#include "demo_mapTownUpgradeTransaction.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapTownProgressionRules.h"

namespace
{
	bool IsSpiritWood(const FName DefinitionId)
	{
		return DefinitionId == Fdemo_mapItemIds::SpiritWoodLevel1
			|| DefinitionId == Fdemo_mapItemIds::SpiritWoodLevel2
			|| DefinitionId == Fdemo_mapItemIds::SpiritWoodLevel3;
	}

	bool IsSpiritOre(const FName DefinitionId)
	{
		return DefinitionId == Fdemo_mapItemIds::SpiritOreLevel1
			|| DefinitionId == Fdemo_mapItemIds::SpiritOreLevel2
			|| DefinitionId == Fdemo_mapItemIds::SpiritOreLevel3;
	}

	int32 CountMaterial(const Fdemo_mapPersistentProfile& Profile, bool bWood)
	{
		int32 Total = 0;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
		{
			if (Item.PersistentDomain == Edemo_mapPersistentDomain::PermanentStash
				&& (bWood ? IsSpiritWood(Item.ItemDefinitionId) : IsSpiritOre(Item.ItemDefinitionId))
				&& Item.StackCount > 0)
			{
				Total += Item.StackCount;
			}
		}
		return Total;
	}

	void RemovePreparationReferences(
		Fdemo_mapPersistentProfile& Profile,
		const TSet<FGuid>& RemovedItemIds)
	{
		auto ClearIfRemoved = [&RemovedItemIds](FGuid& ItemId)
		{
			if (RemovedItemIds.Contains(ItemId))
			{
				ItemId.Invalidate();
			}
		};
		ClearIfRemoved(Profile.PreparationLayout.WeaponItemInstanceId);
		ClearIfRemoved(Profile.PreparationLayout.ArmorItemInstanceId);
		ClearIfRemoved(Profile.PreparationLayout.AccessoryItemInstanceId);
		ClearIfRemoved(Profile.PreparationLayout.BackpackItemInstanceId);
		Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds.RemoveAll(
			[&RemovedItemIds](const FGuid& ItemId)
			{
				return RemovedItemIds.Contains(ItemId);
			});
		for (FGuid& ItemId : Profile.PreparationLayout.HotbarItemInstanceIds)
		{
			ClearIfRemoved(ItemId);
		}
		if (Profile.WarehouseLayout.bInitialized)
		{
			for (FGuid& ItemId : Profile.WarehouseLayout.SlotItemInstanceIds)
			{
				ClearIfRemoved(ItemId);
			}
		}
	}

	bool IsPreparationReferenced(
		const Fdemo_mapPersistentPreparationLayout& Layout,
		const FGuid& ItemId)
	{
		return Layout.WeaponItemInstanceId == ItemId
			|| Layout.ArmorItemInstanceId == ItemId
			|| Layout.AccessoryItemInstanceId == ItemId
			|| Layout.BackpackItemInstanceId == ItemId
			|| Layout.OrderedRunInventoryItemInstanceIds.Contains(ItemId)
			|| Layout.HotbarItemInstanceIds.Contains(ItemId);
	}

	Fdemo_mapTownUpgradeResult Reject(
		Edemo_mapTownUpgradeStatus Status,
		const FString& Diagnostic,
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapTownUpgradeResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.PreviousGeneration = Profile.SaveGeneration;
		Result.CommittedGeneration = Profile.SaveGeneration;
		Result.PreviousTownLevel = Profile.TownLevel;
		Result.TownLevelAfter = Profile.TownLevel;
		Result.SpiritWoodBefore = Result.SpiritWoodAfter = CountMaterial(Profile, true);
		Result.SpiritOreBefore = Result.SpiritOreAfter = CountMaterial(Profile, false);
		Result.PersistentSpiritStonesBefore = Result.PersistentSpiritStonesAfter =
			Profile.PersistentSpiritStones;
		return Result;
	}
}

Fdemo_mapTownUpgradeResult Fdemo_mapTownUpgradeTransaction::Execute(
	Fdemo_mapPersistentProfile& InOutProfile,
	const Fdemo_mapTownUpgradeIntent& Intent,
	const Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	FString ValidationError;
	if (!Repository.ValidateProfile(InOutProfile, &ValidationError))
		return Reject(Edemo_mapTownUpgradeStatus::ProfileValidationRejected, ValidationError, InOutProfile);
	if (InOutProfile.ActiveRun.bHasActiveRun)
		return Reject(Edemo_mapTownUpgradeStatus::SessionNotReady, TEXT("Town construction is only allowed during Preparation."), InOutProfile);
	if (Intent.ExpectedProfileId != InOutProfile.ProfileId)
		return Reject(Edemo_mapTownUpgradeStatus::ProfileIdentityMismatch, TEXT("Town upgrade ProfileId is stale."), InOutProfile);
	if (Intent.ExpectedSaveGeneration != InOutProfile.SaveGeneration)
		return Reject(Edemo_mapTownUpgradeStatus::ProfileGenerationMismatch, TEXT("Town upgrade SaveGeneration is stale."), InOutProfile);
	if (Intent.ExpectedTownLevel != InOutProfile.TownLevel)
		return Reject(Edemo_mapTownUpgradeStatus::TownLevelMismatch, TEXT("Town upgrade TownLevel is stale."), InOutProfile);

	Fdemo_mapTownUpgradeCost Cost;
	if (!Fdemo_mapTownProgressionRules::TryGetNextLevelCost(InOutProfile.TownLevel, Cost))
		return Reject(Edemo_mapTownUpgradeStatus::MaxTownLevel, TEXT("Town construction is complete at level 5."), InOutProfile);
	if (Intent.RequestedNextLevel != InOutProfile.TownLevel + 1
		|| Intent.ExpectedSpiritWoodCost != Cost.SpiritWood
		|| Intent.ExpectedSpiritOreCost != Cost.SpiritOre
		|| Intent.ExpectedSpiritStoneCost != Cost.SpiritStones)
		return Reject(Edemo_mapTownUpgradeStatus::CostMismatch, TEXT("Town upgrade intent does not match the frozen cost table."), InOutProfile);

	const int32 WoodBefore = CountMaterial(InOutProfile, true);
	const int32 OreBefore = CountMaterial(InOutProfile, false);
	if (WoodBefore < Cost.SpiritWood)
		return Reject(Edemo_mapTownUpgradeStatus::InsufficientSpiritWood, TEXT("Town upgrade requires more Spirit Wood."), InOutProfile);
	if (OreBefore < Cost.SpiritOre)
		return Reject(Edemo_mapTownUpgradeStatus::InsufficientSpiritOre, TEXT("Town upgrade requires more Spirit Ore."), InOutProfile);
	if (InOutProfile.PersistentSpiritStones < Cost.SpiritStones)
		return Reject(Edemo_mapTownUpgradeStatus::InsufficientSpiritStones, TEXT("Town upgrade requires more Persistent Spirit Stones."), InOutProfile);

	Fdemo_mapPersistentProfile Candidate = InOutProfile;
	int32 WoodRemaining = Cost.SpiritWood;
	int32 OreRemaining = Cost.SpiritOre;
	TSet<FGuid> RemovedItemIds;
	auto ConsumeMaterialPass = [&Candidate, &WoodRemaining, &OreRemaining, &RemovedItemIds](
		bool bConsumePreparationReferenced)
	{
		for (Fdemo_mapPersistentItemRecord& Item : Candidate.PermanentStash)
		{
			if (IsPreparationReferenced(Candidate.PreparationLayout, Item.ItemInstanceId)
				!= bConsumePreparationReferenced)
			{
				continue;
			}
			int32* Remaining = Item.PersistentDomain != Edemo_mapPersistentDomain::PermanentStash
				? nullptr
				: (IsSpiritWood(Item.ItemDefinitionId) ? &WoodRemaining
					: (IsSpiritOre(Item.ItemDefinitionId) ? &OreRemaining : nullptr));
			if (!Remaining || *Remaining <= 0 || Item.StackCount <= 0)
			{
				continue;
			}
			const int32 Taken = FMath::Min(*Remaining, Item.StackCount);
			Item.StackCount -= Taken;
			*Remaining -= Taken;
			if (Item.StackCount == 0)
			{
				RemovedItemIds.Add(Item.ItemInstanceId);
			}
		}
	};
	// Keep equipped/carried material stacks intact whenever ordinary permanent
	// stacks can pay the fixed cost; each pass preserves saved array order.
	ConsumeMaterialPass(false);
	ConsumeMaterialPass(true);
	if (WoodRemaining != 0 || OreRemaining != 0)
		return Reject(Edemo_mapTownUpgradeStatus::ProfileValidationRejected, TEXT("Town material counting changed during construction."), InOutProfile);
	Candidate.PermanentStash.RemoveAll([](const Fdemo_mapPersistentItemRecord& Item)
	{
		return Item.StackCount <= 0;
	});
	RemovePreparationReferences(Candidate, RemovedItemIds);
	Candidate.PersistentSpiritStones -= Cost.SpiritStones;
	++Candidate.TownLevel;
	if (!Repository.ValidateProfile(Candidate, &ValidationError))
		return Reject(Edemo_mapTownUpgradeStatus::ProfileValidationRejected, TEXT("Town upgrade candidate rejected: ") + ValidationError, InOutProfile);

	Fdemo_mapPersistentProfile Committed = Candidate;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Committed, Storage);
	Fdemo_mapTownUpgradeResult Result = Reject(Edemo_mapTownUpgradeStatus::RepositorySaveRejected, Save.Diagnostic, InOutProfile);
	Result.bDiskStateChanged = Save.bDiskStateChanged;
	Result.RepositorySaveStatus = Save.Status;
	if (Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Result.Status = Edemo_mapTownUpgradeStatus::CommitOutcomeRequiresReload;
		return Result;
	}
	if (!Save.IsSuccess()) return Result;

	InOutProfile = Committed;
	Result.Status = Edemo_mapTownUpgradeStatus::Committed;
	Result.Diagnostic = FString::Printf(TEXT("Town upgraded to level %d with one atomic Profile save."), Committed.TownLevel);
	Result.CommittedGeneration = Committed.SaveGeneration;
	Result.TownLevelAfter = Committed.TownLevel;
	Result.SpiritWoodAfter = CountMaterial(Committed, true);
	Result.SpiritOreAfter = CountMaterial(Committed, false);
	Result.PersistentSpiritStonesAfter = Committed.PersistentSpiritStones;
	Result.CommittedProfile = Committed;
	return Result;
}
