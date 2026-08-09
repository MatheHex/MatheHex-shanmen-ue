#include "demo_mapProfileSettlementTransaction.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"

namespace
{
	Fdemo_mapProfileSettlementResult Reject(
		Edemo_mapProfileSettlementStatus Status,
		const FString& Diagnostic,
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapProfileSettlementResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.ProfileId = Profile.ProfileId;
		Result.PreviousGeneration = Profile.SaveGeneration;
		return Result;
	}

	bool IsTerminalState(Edemo_mapPersistentActiveRunState State)
	{
		return State == Edemo_mapPersistentActiveRunState::Extraction
			|| State == Edemo_mapPersistentActiveRunState::Death
			|| State == Edemo_mapPersistentActiveRunState::Abandon
			|| State == Edemo_mapPersistentActiveRunState::RecoveredAbandon
			|| State == Edemo_mapPersistentActiveRunState::ActivationFailure;
	}

	Edemo_mapRunEndReason ReasonForState(Edemo_mapPersistentActiveRunState State)
	{
		switch (State)
		{
		case Edemo_mapPersistentActiveRunState::Extraction: return Edemo_mapRunEndReason::Extraction;
		case Edemo_mapPersistentActiveRunState::Death: return Edemo_mapRunEndReason::Death;
		case Edemo_mapPersistentActiveRunState::Abandon: return Edemo_mapRunEndReason::Abandon;
		case Edemo_mapPersistentActiveRunState::RecoveredAbandon: return Edemo_mapRunEndReason::RecoveredAbandon;
		case Edemo_mapPersistentActiveRunState::ActivationFailure: return Edemo_mapRunEndReason::ActivationFailure;
		default: return Edemo_mapRunEndReason::None;
		}
	}

	Edemo_mapPersistentActiveRunState StateForReason(Edemo_mapRunEndReason Reason)
	{
		switch (Reason)
		{
		case Edemo_mapRunEndReason::Extraction: return Edemo_mapPersistentActiveRunState::Extraction;
		case Edemo_mapRunEndReason::Death: return Edemo_mapPersistentActiveRunState::Death;
		case Edemo_mapRunEndReason::Abandon: return Edemo_mapPersistentActiveRunState::Abandon;
		case Edemo_mapRunEndReason::RecoveredAbandon: return Edemo_mapPersistentActiveRunState::RecoveredAbandon;
		case Edemo_mapRunEndReason::ActivationFailure: return Edemo_mapPersistentActiveRunState::ActivationFailure;
		default: return Edemo_mapPersistentActiveRunState::None;
		}
	}

	FGuid GenerateSettlementId(const Fdemo_mapPersistentProfile& Profile, const Fdemo_mapRuntimeSettlementSnapshot* Snapshot)
	{
		TSet<FGuid> Forbidden;
		Forbidden.Add(Profile.ProfileId);
		Forbidden.Add(Profile.ActiveRun.ActiveRunId);
		if (Profile.LastSettlementId.IsValid()) Forbidden.Add(Profile.LastSettlementId);
		if (Profile.ActiveRun.CommittedSettlementId.IsValid()) Forbidden.Add(Profile.ActiveRun.CommittedSettlementId);
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash) Forbidden.Add(Item.ItemInstanceId);
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.ActiveRun.ActiveRunItems) Forbidden.Add(Item.ItemInstanceId);
		if (Snapshot) for (const Fdemo_mapRuntimeSettlementItem& Item : Snapshot->OrderedSecuredItems) Forbidden.Add(Item.ItemInstanceId);
		for (int32 Attempt = 0; Attempt < 16; ++Attempt)
		{
			const FGuid Candidate = FGuid::NewGuid();
			if (Candidate.IsValid() && !Forbidden.Contains(Candidate)) return Candidate;
		}
		return FGuid();
	}

	void ReconcileLayout(
		Fdemo_mapPersistentPreparationLayout& Layout,
		const TSet<FGuid>& SurvivingIds,
		TArray<FGuid>& OutClearedIds)
	{
		auto ClearIfLost = [&SurvivingIds, &OutClearedIds](FGuid& Id)
		{
			if (!Id.IsValid() || SurvivingIds.Contains(Id)) return;
			OutClearedIds.AddUnique(Id);
			Id.Invalidate();
		};
		ClearIfLost(Layout.WeaponItemInstanceId);
		ClearIfLost(Layout.ArmorItemInstanceId);
		ClearIfLost(Layout.AccessoryItemInstanceId);
		ClearIfLost(Layout.SpatialRingItemInstanceId);
		ClearIfLost(Layout.BackpackItemInstanceId);
		Layout.OrderedRunInventoryItemInstanceIds.RemoveAll(
			[&SurvivingIds, &OutClearedIds](const FGuid& Id)
			{
				if (SurvivingIds.Contains(Id)) return false;
				if (Id.IsValid()) OutClearedIds.AddUnique(Id);
				return true;
			});
		for (FGuid& Id : Layout.HotbarItemInstanceIds)
		{
			ClearIfLost(Id);
		}
	}
}

Fdemo_mapProfileSettlementResult Fdemo_mapProfileSettlementTransaction::Execute(
	Fdemo_mapPersistentProfile& InOutProfile,
	const Fdemo_mapProfileSettlementRequest& Request,
	const Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	FString ValidationError;
	if (!Repository.ValidateProfile(InOutProfile, &ValidationError))
		return Reject(Edemo_mapProfileSettlementStatus::ProfileValidationRejected, ValidationError, InOutProfile);
	if (Request.ExpectedProfileId != InOutProfile.ProfileId)
		return Reject(Edemo_mapProfileSettlementStatus::ProfileIdentityMismatch, TEXT("ExpectedProfileId does not match the caller Profile."), InOutProfile);

	const Fdemo_mapPersistentActiveRunRecord& ExistingRun = InOutProfile.ActiveRun;
	if (!ExistingRun.bHasActiveRun && IsTerminalState(ExistingRun.ActiveRunState) && Request.ExpectedActiveRunId == ExistingRun.ActiveRunId)
	{
		Fdemo_mapProfileSettlementResult Result = Reject(Edemo_mapProfileSettlementStatus::AlreadyCommitted, TEXT("The same RunId already has a durable first-event-wins tombstone."), InOutProfile);
		Result.CommittedGeneration = InOutProfile.SaveGeneration;
		Result.ActiveRunId = ExistingRun.ActiveRunId;
		Result.SettlementId = ExistingRun.CommittedSettlementId;
		Result.CommittedEndReason = ReasonForState(ExistingRun.ActiveRunState);
		Result.RepositorySaveStatus = Edemo_mapProfileSaveStatus::Saved;
		return Result;
	}

	if (Request.ExpectedSaveGeneration != InOutProfile.SaveGeneration)
		return Reject(Edemo_mapProfileSettlementStatus::ProfileGenerationMismatch, TEXT("ExpectedSaveGeneration is stale."), InOutProfile);
	if (Request.ExpectedActiveRunId != ExistingRun.ActiveRunId)
		return Reject(Edemo_mapProfileSettlementStatus::ActiveRunIdMismatch, TEXT("ExpectedActiveRunId does not match the caller Profile."), InOutProfile);
	if (!ExistingRun.bHasActiveRun)
		return Reject(Edemo_mapProfileSettlementStatus::NoPreparedRun, TEXT("Profile has no Prepared ActiveRun."), InOutProfile);
	if (ExistingRun.ActiveRunState != Edemo_mapPersistentActiveRunState::Prepared)
		return Reject(Edemo_mapProfileSettlementStatus::InvalidActiveRunState, TEXT("First Settlement only accepts a Prepared ActiveRun."), InOutProfile);
	if (StateForReason(Request.RequestedEndReason) == Edemo_mapPersistentActiveRunState::None)
		return Reject(Edemo_mapProfileSettlementStatus::InvalidEndReason, TEXT("RequestedEndReason is not a supported terminal reason."), InOutProfile);

	const Fdemo_mapRuntimeSettlementSnapshot* Snapshot = Request.RuntimeSnapshot.IsSet() ? &Request.RuntimeSnapshot.GetValue() : nullptr;
	if (Request.RequestedEndReason == Edemo_mapRunEndReason::Extraction && !Snapshot)
		return Reject(Edemo_mapProfileSettlementStatus::SnapshotMissing, TEXT("Extraction requires a Runtime Settlement Snapshot."), InOutProfile);
	if (Request.RequestedEndReason == Edemo_mapRunEndReason::RecoveredAbandon && Snapshot)
		return Reject(Edemo_mapProfileSettlementStatus::SnapshotUnexpected, TEXT("RecoveredAbandon must not depend on Runtime state."), InOutProfile);
	if (Snapshot)
	{
		if (!Snapshot->bValid)
			return Reject(Edemo_mapProfileSettlementStatus::SnapshotInvalid, TEXT("Runtime Settlement Snapshot is not marked valid."), InOutProfile);
		if (Snapshot->ActiveRunId != ExistingRun.ActiveRunId)
			return Reject(Edemo_mapProfileSettlementStatus::SnapshotRunIdMismatch, TEXT("Runtime Snapshot RunId does not match the Prepared Run."), InOutProfile);
		if (Snapshot->CommittedEndReason != Request.RequestedEndReason)
			return Reject(Edemo_mapProfileSettlementStatus::SnapshotReasonMismatch, TEXT("Runtime Snapshot committed a different first terminal reason."), InOutProfile);
		if (Request.RequestedEndReason != Edemo_mapRunEndReason::Extraction && !Snapshot->OrderedSecuredItems.IsEmpty())
			return Reject(Edemo_mapProfileSettlementStatus::SnapshotUnexpected, TEXT("Non-extraction Snapshot must not secure items."), InOutProfile);
	}

	TSet<FGuid> PermanentIds;
	for (const Fdemo_mapPersistentItemRecord& Item : InOutProfile.PermanentStash) PermanentIds.Add(Item.ItemInstanceId);
	TMap<FGuid, const Fdemo_mapPersistentItemRecord*> DeployedById;
	for (const Fdemo_mapPersistentItemRecord& Item : ExistingRun.ActiveRunItems) DeployedById.Add(Item.ItemInstanceId, &Item);
	TSet<FGuid> SnapshotIds;
	if (Snapshot)
	{
		for (const Fdemo_mapRuntimeSettlementItem& Item : Snapshot->OrderedSecuredItems)
		{
			if (!Item.ItemInstanceId.IsValid() || SnapshotIds.Contains(Item.ItemInstanceId))
				return Reject(Edemo_mapProfileSettlementStatus::ItemIdentityDuplicate, TEXT("Snapshot contains an invalid or duplicate ItemInstanceId."), InOutProfile);
			SnapshotIds.Add(Item.ItemInstanceId);
			if (PermanentIds.Contains(Item.ItemInstanceId) || Item.ItemInstanceId == InOutProfile.ProfileId
				|| Item.ItemInstanceId == ExistingRun.ActiveRunId || Item.ItemInstanceId == InOutProfile.LastSettlementId)
				return Reject(Edemo_mapProfileSettlementStatus::ItemIdentityConflict, TEXT("Snapshot ItemInstanceId conflicts with persistent identity."), InOutProfile);
			const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
			if (!Definition)
				return Reject(Edemo_mapProfileSettlementStatus::UnknownItemDefinition, TEXT("Snapshot contains an unknown ItemDefinitionId."), InOutProfile);
			if (Item.StackCount <= 0 || Item.StackCount > Definition->MaxStackSize
				|| (!Definition->CompatibleSlotIds.IsEmpty() && Item.StackCount != 1))
				return Reject(Edemo_mapProfileSettlementStatus::InvalidItemQuantity, TEXT("Snapshot StackCount violates its Definition."), InOutProfile);
			if (const Fdemo_mapPersistentItemRecord* const* Deployed = DeployedById.Find(Item.ItemInstanceId))
			{
				if ((*Deployed)->ItemDefinitionId != Item.ItemDefinitionId
					|| (*Deployed)->StackCount != Item.StackCount
					|| (*Deployed)->RewardEventKind
						!= Item.RewardEventKind
					|| (*Deployed)->RewardEventId
						!= Item.RewardEventId
					|| (*Deployed)->RewardValueMultiplierBps
						!= Item.RewardValueMultiplierBps
					|| (*Deployed)->RewardSourceRoleId
						!= Item.RewardSourceRoleId
					|| (*Deployed)->RareRewardEventId
						!= Item.RareRewardEventId
					|| (*Deployed)->RareRewardPolicyId
						!= Item.RareRewardPolicyId
					|| (*Deployed)->RareRewardTierId
						!= Item.RareRewardTierId
					|| (*Deployed)->RareRewardBonusValue
						!= Item.RareRewardBonusValue
					|| !((*Deployed)->AffixSet == Item.AffixSet)
					|| Item.OriginRunId.IsValid())
					return Reject(Edemo_mapProfileSettlementStatus::DeployedIdentityMismatch, TEXT("A deployed original no longer matches its persistent identity."), InOutProfile);
			}
			else if (Item.OriginRunId != ExistingRun.ActiveRunId)
			{
				return Reject(Edemo_mapProfileSettlementStatus::AcquiredOriginMismatch, TEXT("A non-deployed secured item does not originate from the active Run."), InOutProfile);
			}
		}
	}

	Fdemo_mapPersistentProfile Candidate = InOutProfile;
	const int64 RiskBefore = ExistingRun.RiskSpiritStones;
	const int64 PersistentBefore = InOutProfile.PersistentSpiritStones;
	const bool bRestoresPreparation =
		Request.RequestedEndReason == Edemo_mapRunEndReason::ActivationFailure;
	if ((Request.RequestedEndReason == Edemo_mapRunEndReason::Extraction
		|| bRestoresPreparation)
		&& RiskBefore > MAX_int64 - PersistentBefore)
		return Reject(Edemo_mapProfileSettlementStatus::CurrencyOverflow, TEXT("Extraction Spirit Stone transfer would overflow int64."), InOutProfile);
	const FGuid SettlementId = GenerateSettlementId(Candidate, Snapshot);
	if (!SettlementId.IsValid())
		return Reject(Edemo_mapProfileSettlementStatus::ProfileValidationRejected, TEXT("A unique SettlementId could not be generated."), InOutProfile);
	TArray<FGuid> OrderedSecuredItemIds;
	if (Request.RequestedEndReason == Edemo_mapRunEndReason::Extraction)
	{
		for (const Fdemo_mapRuntimeSettlementItem& RuntimeItem : Snapshot->OrderedSecuredItems)
		{
			Fdemo_mapPersistentItemRecord Item;
			Item.ItemInstanceId = RuntimeItem.ItemInstanceId;
			Item.ItemDefinitionId = RuntimeItem.ItemDefinitionId;
			Item.StackCount = RuntimeItem.StackCount;
			Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			Item.EquipmentSlotId = NAME_None;
			Item.OriginRunId.Invalidate();
			Item.RewardEventKind = RuntimeItem.RewardEventKind;
			Item.RewardEventId = RuntimeItem.RewardEventId;
			Item.RewardValueMultiplierBps =
				RuntimeItem.RewardValueMultiplierBps;
			Item.RewardSourceRoleId =
				RuntimeItem.RewardSourceRoleId;
			Item.RareRewardEventId =
				RuntimeItem.RareRewardEventId;
			Item.RareRewardPolicyId =
				RuntimeItem.RareRewardPolicyId;
			Item.RareRewardTierId =
				RuntimeItem.RareRewardTierId;
			Item.RareRewardBonusValue =
				RuntimeItem.RareRewardBonusValue;
			Item.AffixSet = RuntimeItem.AffixSet;
			Candidate.PermanentStash.Add(MoveTemp(Item));
			OrderedSecuredItemIds.Add(RuntimeItem.ItemInstanceId);
		}
		Candidate.PersistentSpiritStones += RiskBefore;
	}
	TSet<FGuid> SurvivingIds = SnapshotIds;
	if (bRestoresPreparation)
	{
		// No world content was accepted, so preserve the exact preparation
		// loadout and risk currency instead of treating a technical failure as
		// player loss. The terminal tombstone records why the run was undone.
		for (const Fdemo_mapPersistentItemRecord& ActiveItem : ExistingRun.ActiveRunItems)
		{
			Fdemo_mapPersistentItemRecord RestoredItem = ActiveItem;
			RestoredItem.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			RestoredItem.EquipmentSlotId = NAME_None;
			RestoredItem.OriginRunId.Invalidate();
			Candidate.PermanentStash.Add(MoveTemp(RestoredItem));
			SurvivingIds.Add(ActiveItem.ItemInstanceId);
		}
		Candidate.PersistentSpiritStones += RiskBefore;
	}
	TArray<FGuid> ClearedPreparationItemIds;
	ReconcileLayout(Candidate.PreparationLayout, SurvivingIds, ClearedPreparationItemIds);
	Candidate.ActiveRun.bHasActiveRun = false;
	Candidate.ActiveRun.ActiveRunState = StateForReason(Request.RequestedEndReason);
	Candidate.ActiveRun.DeployedItemIds.Reset();
	Candidate.ActiveRun.ActiveRunItems.Reset();
	Candidate.ActiveRun.RiskSpiritStones = 0;
	Candidate.ActiveRun.ConsumedSpiritStoneSourceIds.Reset();
	Candidate.ActiveRun.CommittedSettlementId = SettlementId;
	Candidate.LastSettlementId = SettlementId;
	if (!Repository.ValidateProfile(Candidate, &ValidationError))
		return Reject(Edemo_mapProfileSettlementStatus::ProfileValidationRejected, TEXT("Settlement candidate validation failed: ") + ValidationError, InOutProfile);

	Fdemo_mapPersistentProfile Committed = Candidate;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Committed, Storage);
	Fdemo_mapProfileSettlementResult Result = Reject(Edemo_mapProfileSettlementStatus::RepositorySaveRejected, Save.Diagnostic, InOutProfile);
	Result.ActiveRunId = ExistingRun.ActiveRunId;
	Result.SettlementId = SettlementId;
	Result.CommittedEndReason = Request.RequestedEndReason;
	Result.bDiskStateChanged = Save.bDiskStateChanged;
	Result.RepositorySaveStatus = Save.Status;
	Result.RiskBefore = RiskBefore;
	Result.RiskTransferred = (Request.RequestedEndReason == Edemo_mapRunEndReason::Extraction
		|| bRestoresPreparation) ? RiskBefore : 0;
	Result.RiskLost = (Request.RequestedEndReason == Edemo_mapRunEndReason::Extraction
		|| bRestoresPreparation) ? 0 : RiskBefore;
	Result.PersistentBefore = PersistentBefore;
	Result.PersistentAfter = Candidate.PersistentSpiritStones;
	Result.ClearedPreparationItemIds = ClearedPreparationItemIds;
	if (Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Result.Status = Edemo_mapProfileSettlementStatus::CommitOutcomeRequiresReload;
		return Result;
	}
	if (!Save.IsSuccess()) return Result;

	InOutProfile = Committed;
	Result.Status = Edemo_mapProfileSettlementStatus::Committed;
	Result.Diagnostic = TEXT("Settlement committed and verified.");
	Result.CommittedGeneration = Committed.SaveGeneration;
	Result.SettlementId = Committed.ActiveRun.CommittedSettlementId;
	Result.CommittedProfile = Committed;
	Result.OrderedSecuredItemIds = MoveTemp(OrderedSecuredItemIds);
	return Result;
}
