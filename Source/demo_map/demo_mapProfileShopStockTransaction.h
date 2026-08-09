#pragma once

#include "CoreMinimal.h"
#include "demo_mapProfileRepository.h"

enum class Edemo_mapProfileShopStockEnsureStatus : uint8
{
	Unchanged,
	Initialized,
	Refreshed,
	GenerationRejected,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapProfileShopStockEnsureResult
{
	Edemo_mapProfileShopStockEnsureStatus Status =
		Edemo_mapProfileShopStockEnsureStatus::GenerationRejected;
	FString Diagnostic;
	Fdemo_mapPersistentProfile Before;
	Fdemo_mapPersistentProfile IntendedAfter;
	bool bHasIntendedAfter = false;

	bool IsSuccess() const
	{
		return Status == Edemo_mapProfileShopStockEnsureStatus::Unchanged
			|| Status == Edemo_mapProfileShopStockEnsureStatus::Initialized
			|| Status == Edemo_mapProfileShopStockEnsureStatus::Refreshed;
	}
};

/**
 * Applies initial Generation 0 or one pending committed terminal atomically.
 * It never mutates the caller on generation/validation/repository failure.
 */
struct Fdemo_mapProfileShopStockTransaction
{
	Fdemo_mapProfileShopStockEnsureResult EnsureForPreparation(
		Fdemo_mapPersistentProfile& Profile,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
