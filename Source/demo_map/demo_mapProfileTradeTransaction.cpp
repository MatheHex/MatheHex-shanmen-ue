#include "demo_mapProfileTradeTransaction.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardEventTypes.h"
#include "demo_mapRewardShopStock.h"
#include "demo_mapProfileSessionTypes.h"

namespace
{
	Fdemo_mapProfileTradeTransactionResult Reject(
		const Fdemo_mapPersistentProfile& Profile,
		const Fdemo_mapProfileTradeIntent& Intent,
		Edemo_mapProfileTradeStatus Status,
		const FString& Diagnostic)
	{
		Fdemo_mapProfileTradeTransactionResult Out;
		Out.Before = Profile;
		Out.Result.Status = Status;
		Out.Result.Kind = Intent.Kind;
		Out.Result.ProfileId = Profile.ProfileId;
		Out.Result.SaveGenerationBefore = Profile.SaveGeneration;
		Out.Result.SaveGenerationAfter = Profile.SaveGeneration;
		Out.Result.BalanceBefore = Profile.PersistentSpiritStones;
		Out.Result.BalanceAfter = Profile.PersistentSpiritStones;
		Out.Result.ItemDefinitionId = Intent.ItemDefinitionId;
		Out.Result.ItemInstanceId = Intent.ItemInstanceId;
		Out.Result.Diagnostic = Diagnostic;
		return Out;
	}

	bool IsUniqueProfileItemId(const Fdemo_mapPersistentProfile& Profile, const FGuid& Candidate)
	{
		if (!Candidate.IsValid()) return false;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
		{
			if (Item.ItemInstanceId == Candidate) return false;
		}
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.ActiveRun.ActiveRunItems)
		{
			if (Item.ItemInstanceId == Candidate) return false;
		}
		return true;
	}

	bool SameMetadataExceptLastSaved(
		const Fdemo_mapPersistentProfileMetadata& A,
		const Fdemo_mapPersistentProfileMetadata& B)
	{
		return A.ProfileName == B.ProfileName && A.CreatedUtc == B.CreatedUtc;
	}
}

TArray<Fdemo_mapProfileShopCatalogRow> Fdemo_mapProfileTradeTransaction::BuildCatalog()
{
	TArray<Fdemo_mapProfileShopCatalogRow> Rows;
	for (const FName DefinitionId : Fdemo_mapItemDefinitions::GetPurchasableDefinitionIds())
	{
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
		if (!Definition) continue;
		Fdemo_mapProfileShopCatalogRow Row;
		Row.ItemDefinitionId = Definition->DefinitionId;
		Row.DisplayName = Definition->DisplayName.ToString();
		Row.ItemCategoryId = Definition->CategoryId;
		Row.Level = Definition->Level;
		Row.MaxStackSize = Definition->MaxStackSize;
		Row.BuyPrice = Definition->BuyPrice;
		Row.SellPrice = Definition->SellPrice;
		Rows.Add(MoveTemp(Row));
	}
	return Rows;
}

bool Fdemo_mapProfileTradeTransaction::IsSessionStateAllowed(uint8 SessionStateValue)
{
	return SessionStateValue == static_cast<uint8>(Edemo_mapProfileSessionState::ReadyForPreparation);
}

bool Fdemo_mapProfileTradeTransaction::MatchesIntendedCommit(
	const Fdemo_mapPersistentProfile& Reloaded,
	const Fdemo_mapPersistentProfile& IntendedAfter)
{
	return Reloaded.SchemaVersion == IntendedAfter.SchemaVersion
		&& Reloaded.ProfileId == IntendedAfter.ProfileId
		&& Reloaded.SaveGeneration == IntendedAfter.SaveGeneration
		&& SameMetadataExceptLastSaved(Reloaded.ProfileMetadata, IntendedAfter.ProfileMetadata)
		&& Reloaded.PersistentSpiritStones == IntendedAfter.PersistentSpiritStones
		&& Reloaded.PermanentStash == IntendedAfter.PermanentStash
		&& Reloaded.ShopStock == IntendedAfter.ShopStock
		&& Reloaded.PreparationLayout == IntendedAfter.PreparationLayout
		&& Reloaded.ActiveRun == IntendedAfter.ActiveRun
		&& Reloaded.LastSettlementId == IntendedAfter.LastSettlementId;
}

Fdemo_mapProfileTradeTransactionResult Fdemo_mapProfileTradeTransaction::Execute(
	Fdemo_mapPersistentProfile& Profile,
	const Fdemo_mapProfileTradeIntent& Intent,
	const TSet<FGuid>& SelectedForDeployment,
	Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	if (Intent.ExpectedProfileId != Profile.ProfileId
		|| Intent.ExpectedSaveGeneration != Profile.SaveGeneration)
	{
		return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::StaleIntent,
			TEXT("Trade intent has a stale ProfileId or SaveGeneration."));
	}

	Fdemo_mapProfileTradeTransactionResult Out = Reject(
		Profile, Intent, Edemo_mapProfileTradeStatus::InvalidIntent, TEXT("Trade intent was not applied."));
	Out.IntendedAfter = Profile;
	Fdemo_mapPersistentProfile& Candidate = Out.IntendedAfter;

	if (Intent.Kind == Edemo_mapProfileTradeKind::Buy)
	{
		if (!Intent.ItemDefinitionId.IsNone()
			|| !Intent.ItemInstanceId.IsValid()
			|| Intent.ShopStockPolicyId.IsNone()
			|| Intent.ShopStockGeneration < 0
			|| !Intent.ShopStockEventId.IsValid()
			|| Intent.ShopSlotId.IsNone()
			|| Intent.ExpectedBuyValue <= 0)
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::InvalidIntent,
				TEXT("Buy requires one complete instance-bound ShopStock quote."));
		}
		if (!Profile.ShopStock.bInitialized)
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ShopStockUnavailable,
				TEXT("ShopStock has not been initialized."));
		}
		if (Profile.ShopStock.PolicyId != Intent.ShopStockPolicyId
			|| Profile.ShopStock.Generation != Intent.ShopStockGeneration
			|| Profile.ShopStock.ShopStockEventId
				!= Intent.ShopStockEventId)
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ShopStockStale,
				TEXT("ShopStock policy, generation, or event identity is stale."));
		}
		const int32 EntryIndex =
			Profile.ShopStock.Entries.IndexOfByPredicate(
				[&Intent](const Fdemo_mapPersistentShopStockEntry& Entry)
				{
					return Entry.SlotId == Intent.ShopSlotId;
				});
		if (EntryIndex == INDEX_NONE)
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ShopSlotNotFound,
				TEXT("ShopStock SlotId is absent."));
		}
		const Fdemo_mapPersistentShopStockEntry& Entry =
			Profile.ShopStock.Entries[EntryIndex];
		if (Entry.State == Edemo_mapPersistentShopStockEntryState::Sold)
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ShopSlotSold,
				TEXT("ShopStock slot is already SOLD."));
		}
		if (Entry.Item.ItemInstanceId != Intent.ItemInstanceId)
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ShopStockStale,
				TEXT("ShopStock ItemInstanceId is stale."));
		}
		FString StockError;
		if (!Fdemo_mapRewardShopStock::ValidateState(
				Profile.ProfileId,
				Profile.ShopStock,
				&StockError))
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::InvalidStack,
				StockError);
		}
		int64 EffectiveBuyValue = 0;
		if (!Fdemo_mapRewardShopStock::TryComputeBuyValue(
				Entry.Item,
				EffectiveBuyValue,
				&StockError))
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ArithmeticOverflow,
				StockError);
		}
		if (EffectiveBuyValue != Entry.QuotedBuyValue
			|| EffectiveBuyValue != Intent.ExpectedBuyValue)
		{
			return Reject(Profile, Intent,
				Edemo_mapProfileTradeStatus::ShopQuoteMismatch,
				TEXT("ShopStock buy quote no longer matches the single valuation authority."));
		}
		if (Profile.PersistentSpiritStones < EffectiveBuyValue)
		{
			Fdemo_mapProfileTradeTransactionResult Insufficient = Reject(
				Profile, Intent, Edemo_mapProfileTradeStatus::InsufficientFunds,
				TEXT("Persistent Spirit Stone balance is insufficient for this buy."));
			Insufficient.Result.ItemDefinitionId =
				Entry.Item.ItemDefinitionId;
			Insufficient.Result.ItemInstanceId =
				Entry.Item.ItemInstanceId;
			Insufficient.Result.UnitPrice = EffectiveBuyValue;
			Insufficient.Result.Quantity = 1;
			Insufficient.Result.TotalPrice = EffectiveBuyValue;
			return Insufficient;
		}
		if (!IsUniqueProfileItemId(Profile, Entry.Item.ItemInstanceId))
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::InvalidIntent,
				TEXT("ShopStock ItemInstanceId already exists in a player-owned domain."));
		}

		Candidate.PersistentSpiritStones -= EffectiveBuyValue;
		Fdemo_mapPersistentItemRecord PurchasedItem = Entry.Item;
		PurchasedItem.PersistentDomain =
			Edemo_mapPersistentDomain::PermanentStash;
		Candidate.PermanentStash.Add(PurchasedItem);
		Fdemo_mapPersistentShopStockEntry& SoldEntry =
			Candidate.ShopStock.Entries[EntryIndex];
		SoldEntry.State = Edemo_mapPersistentShopStockEntryState::Sold;
		SoldEntry.SoldItemInstanceId = PurchasedItem.ItemInstanceId;
		SoldEntry.Item = Fdemo_mapPersistentItemRecord();
		SoldEntry.QuotedBuyValue = 0;
		Out.Result.ItemDefinitionId = PurchasedItem.ItemDefinitionId;
		Out.Result.ItemInstanceId = PurchasedItem.ItemInstanceId;
		Out.Result.UnitPrice = EffectiveBuyValue;
		Out.Result.Quantity = 1;
		Out.Result.TotalPrice = EffectiveBuyValue;
	}
	else
	{
		if (!Intent.ItemInstanceId.IsValid() || !Intent.ItemDefinitionId.IsNone())
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::InvalidIntent,
				TEXT("Sell requires one ItemInstanceId and no caller-supplied DefinitionId."));
		}
		const int32 ItemIndex = Profile.PermanentStash.IndexOfByPredicate(
			[&Intent](const Fdemo_mapPersistentItemRecord& Item)
			{
				return Item.ItemInstanceId == Intent.ItemInstanceId;
			});
		if (ItemIndex == INDEX_NONE)
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::ItemNotFound,
				TEXT("Sell ItemInstanceId is absent from the ordered Permanent Stash."));
		}
		if (SelectedForDeployment.Contains(Intent.ItemInstanceId))
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::ItemSelectedForDeployment,
				TEXT("Selected deployment items must be deselected before selling."));
		}
		const Fdemo_mapPersistentItemRecord& Item = Profile.PermanentStash[ItemIndex];
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
		if (!Definition)
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::UnknownDefinition,
				TEXT("Sell item DefinitionId is absent from the immutable item registry."));
		}
		if (!Definition->bSellable || Definition->SellPrice <= 0)
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::NotSellable,
				TEXT("The immutable item definition is not sellable."));
		}
		if (Item.StackCount <= 0 || Item.StackCount > Definition->MaxStackSize
			|| Item.PersistentDomain != Edemo_mapPersistentDomain::PermanentStash)
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::InvalidStack,
				TEXT("Sell requires one complete valid Permanent Stash record."));
		}
			int64 Total = 0;
			FString SellValueDiagnostic;
			if (!Fdemo_mapItemSellValueRules::TryCompute(
				Item.ItemDefinitionId,
				Item.StackCount,
				Item.RewardValueMultiplierBps,
				Item.AffixSet.TotalResolvedValue(),
				Item.RareRewardBonusValue,
				Total,
				&SellValueDiagnostic))
			{
				return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::ArithmeticOverflow,
					SellValueDiagnostic.IsEmpty() ? TEXT("Sell total overflowed int64.") : SellValueDiagnostic);
			}
			if (Profile.PersistentSpiritStones > MAX_int64 - Total)
		{
			return Reject(Profile, Intent, Edemo_mapProfileTradeStatus::ArithmeticOverflow,
				TEXT("Persistent Spirit Stone credit overflowed int64."));
		}
		Candidate.PersistentSpiritStones += Total;
		Candidate.PermanentStash.RemoveAt(ItemIndex, 1, EAllowShrinking::No);
		Out.Result.ItemDefinitionId = Item.ItemDefinitionId;
		Out.Result.ItemInstanceId = Item.ItemInstanceId;
		Out.Result.UnitPrice = Definition->SellPrice;
		Out.Result.Quantity = Item.StackCount;
		Out.Result.TotalPrice = Total;
	}

	Out.bHasIntendedAfter = true;
	Out.IntendedAfter.SaveGeneration = Profile.SaveGeneration + 1;
	Out.Result.BalanceAfter = Candidate.PersistentSpiritStones;
	Out.Result.SaveGenerationAfter = Out.IntendedAfter.SaveGeneration;

	Fdemo_mapPersistentProfile CommitCandidate = Candidate;
	CommitCandidate.SaveGeneration = Profile.SaveGeneration;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(CommitCandidate, Storage);
	if (Save.IsSuccess())
	{
		Profile = CommitCandidate;
		Out.IntendedAfter = CommitCandidate;
		Out.Result.Status = Edemo_mapProfileTradeStatus::Committed;
		Out.Result.SaveGenerationAfter = Profile.SaveGeneration;
		Out.Result.BalanceAfter = Profile.PersistentSpiritStones;
		Out.Result.Diagnostic = Intent.Kind == Edemo_mapProfileTradeKind::Buy
			? TEXT("ShopStock buy moved the same ItemInstance GUID to Permanent Stash and committed one SOLD tombstone.")
			: TEXT("Sell committed exactly one whole Permanent Stash record.");
		return Out;
	}
	if (Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Out.Result.Status = Edemo_mapProfileTradeStatus::CommitOutcomeRequiresReload;
		Out.Result.Diagnostic = TEXT("Trade commit outcome requires exactly one deterministic reload.");
		return Out;
	}
	Out.Result.Status = Edemo_mapProfileTradeStatus::PersistentCommitRejected;
	Out.Result.SaveGenerationAfter = Profile.SaveGeneration;
	Out.Result.BalanceAfter = Profile.PersistentSpiritStones;
	Out.Result.Diagnostic = Save.Diagnostic;
	return Out;
}
