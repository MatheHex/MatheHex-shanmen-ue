#pragma once

#include "CoreMinimal.h"
#include "demo_mapProfileRunTypes.h"

class Fdemo_mapProfileRepository;

/** Stateless coordinator for one optimistic, durable Profile BeginRun commit. */
class Fdemo_mapProfileBeginRunTransaction
{
public:
	Fdemo_mapBeginRunResult Execute(
		Fdemo_mapPersistentProfile& InOutProfile,
		const Fdemo_mapBeginRunRequest& Request,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
