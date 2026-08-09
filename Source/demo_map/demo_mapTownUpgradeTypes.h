#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

/** UI-independent, optimistic-concurrency request for one town level. */
struct Fdemo_mapTownUpgradeIntent
{
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	int32 ExpectedTownLevel = 0;
	int32 RequestedNextLevel = 1;
	int32 ExpectedSpiritWoodCost = 0;
	int32 ExpectedSpiritOreCost = 0;
	int64 ExpectedSpiritStoneCost = 0;
};

enum class Edemo_mapTownUpgradeStatus : uint8
{
	Committed,
	SessionNotReady,
	ProfileValidationRejected,
	ProfileIdentityMismatch,
	ProfileGenerationMismatch,
	TownLevelMismatch,
	MaxTownLevel,
	CostMismatch,
	InsufficientSpiritWood,
	InsufficientSpiritOre,
	InsufficientSpiritStones,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapTownUpgradeResult
{
	Edemo_mapTownUpgradeStatus Status = Edemo_mapTownUpgradeStatus::SessionNotReady;
	FString Diagnostic;
	int32 PreviousGeneration = 0;
	int32 CommittedGeneration = 0;
	int32 PreviousTownLevel = 0;
	int32 TownLevelAfter = 0;
	int32 SpiritWoodBefore = 0;
	int32 SpiritWoodAfter = 0;
	int32 SpiritOreBefore = 0;
	int32 SpiritOreAfter = 0;
	int64 PersistentSpiritStonesBefore = 0;
	int64 PersistentSpiritStonesAfter = 0;
	bool bDiskStateChanged = false;
	Edemo_mapProfileSaveStatus RepositorySaveStatus =
		Edemo_mapProfileSaveStatus::ValidationRejected;
	TOptional<Fdemo_mapPersistentProfile> CommittedProfile;

	bool IsCommitted() const
	{
		return Status == Edemo_mapTownUpgradeStatus::Committed;
	}
};
