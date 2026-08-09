#pragma once

#include "demo_mapSpiritStoneTypes.h"

class Fdemo_mapProfileRepository;

class Fdemo_mapSpiritStoneTransaction
{
public:
	Fdemo_mapSpiritStonePickupResult Execute(
		Fdemo_mapPersistentProfile& InOutProfile,
		const Fdemo_mapSpiritStonePickupIntent& Intent,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
