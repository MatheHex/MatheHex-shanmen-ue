#pragma once

#include "CoreMinimal.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileTradeTypes.h"

struct Fdemo_mapProfileTradeTransactionResult
{
	Fdemo_mapProfileTradeResult Result;
	Fdemo_mapPersistentProfile Before;
	Fdemo_mapPersistentProfile IntendedAfter;
	bool bHasIntendedAfter = false;
};

/** Pure Profile trade transaction; Runtime inventory and risk-domain state are never consulted or mutated. */
struct Fdemo_mapProfileTradeTransaction
{
	static TArray<Fdemo_mapProfileShopCatalogRow> BuildCatalog();
	static bool IsSessionStateAllowed(uint8 SessionStateValue);
	static bool MatchesIntendedCommit(
		const Fdemo_mapPersistentProfile& Reloaded,
		const Fdemo_mapPersistentProfile& IntendedAfter);

	Fdemo_mapProfileTradeTransactionResult Execute(
		Fdemo_mapPersistentProfile& Profile,
		const Fdemo_mapProfileTradeIntent& Intent,
		const TSet<FGuid>& SelectedForDeployment,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
