#include "demo_mapProfileShopStockTransaction.h"

#include "demo_mapRewardShopStock.h"

Fdemo_mapProfileShopStockEnsureResult
Fdemo_mapProfileShopStockTransaction::EnsureForPreparation(
	Fdemo_mapPersistentProfile& Profile,
	Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	Fdemo_mapProfileShopStockEnsureResult Result;
	Result.Before = Profile;
	const bool bInitialize = !Profile.ShopStock.bInitialized;
	const bool bRefresh = !bInitialize
		&& Profile.LastSettlementId.IsValid()
		&& Profile.ShopStock.LastAppliedTerminalId
			!= Profile.LastSettlementId;
	if (!bInitialize && !bRefresh)
	{
		Result.Status = Edemo_mapProfileShopStockEnsureStatus::Unchanged;
		Result.Diagnostic =
			TEXT("ShopStock already matches the current Preparation terminal identity.");
		return Result;
	}
	if (!bInitialize && Profile.ShopStock.Generation == MAX_int32)
	{
		Result.Status =
			Edemo_mapProfileShopStockEnsureStatus::GenerationRejected;
		Result.Diagnostic = TEXT("ShopStock generation cannot be incremented.");
		return Result;
	}

	Result.IntendedAfter = Profile;
	const int32 Generation = bInitialize
		? Fdemo_mapRewardShopStock::GetDefaultPolicy().InitialGeneration
		: Profile.ShopStock.Generation + 1;
	const FGuid AppliedTerminal = Profile.LastSettlementId;
	FString GenerationError;
	if (!Fdemo_mapRewardShopStock::Generate(
			Profile.ProfileId,
			Generation,
			AppliedTerminal,
			Result.IntendedAfter.ShopStock,
			&GenerationError))
	{
		Result.Status =
			Edemo_mapProfileShopStockEnsureStatus::GenerationRejected;
		Result.Diagnostic = GenerationError;
		return Result;
	}
	Result.bHasIntendedAfter = true;

	Fdemo_mapPersistentProfile Candidate = Result.IntendedAfter;
	Candidate.SaveGeneration = Profile.SaveGeneration;
	const Fdemo_mapProfileSaveResult Save =
		Repository.SaveProfile(Candidate, Storage);
	if (Save.IsSuccess())
	{
		Profile = Candidate;
		Result.IntendedAfter = Candidate;
		Result.Status = bInitialize
			? Edemo_mapProfileShopStockEnsureStatus::Initialized
			: Edemo_mapProfileShopStockEnsureStatus::Refreshed;
		Result.Diagnostic = bInitialize
			? TEXT("ShopStock Generation 0 initialized atomically.")
			: TEXT("Exactly one pending terminal restock committed atomically.");
		return Result;
	}
	Result.Status =
		Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed
			? Edemo_mapProfileShopStockEnsureStatus::CommitOutcomeRequiresReload
			: Edemo_mapProfileShopStockEnsureStatus::RepositorySaveRejected;
	Result.Diagnostic = Save.Diagnostic;
	return Result;
}
