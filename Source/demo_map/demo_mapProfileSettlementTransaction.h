#pragma once

#include "CoreMinimal.h"
#include "demo_mapProfileSettlementTypes.h"

class Fdemo_mapProfileRepository;

/** Stateless coordinator for one optimistic, durable Profile Settlement commit. */
class Fdemo_mapProfileSettlementTransaction
{
public:
	Fdemo_mapProfileSettlementResult Execute(
		Fdemo_mapPersistentProfile& InOutProfile,
		const Fdemo_mapProfileSettlementRequest& Request,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
