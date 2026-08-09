#pragma once

#include "demo_mapTownUpgradeTypes.h"

class Fdemo_mapProfileRepository;

/**
 * One-save town construction transaction. It never owns UI state and changes
 * a Profile only after every resource and optimistic-concurrency check passes.
 */
class Fdemo_mapTownUpgradeTransaction
{
public:
	Fdemo_mapTownUpgradeResult Execute(
		Fdemo_mapPersistentProfile& InOutProfile,
		const Fdemo_mapTownUpgradeIntent& Intent,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
