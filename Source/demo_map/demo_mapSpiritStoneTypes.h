#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

struct Fdemo_mapSpiritStonePickupContract
{
	static const FName PickupId;
	static const FName SourceId;
	static constexpr int64 Value = 20;
};

enum class Edemo_mapSpiritStonePickupStatus : uint8
{
	Committed,
	ProfileValidationRejected,
	ProfileIdentityMismatch,
	ProfileGenerationMismatch,
	RunIdMismatch,
	RunNotActive,
	SourceRejected,
	DuplicateSource,
	ValueRejected,
	CurrencyOverflow,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapSpiritStonePickupIntent
{
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	FGuid ExpectedActiveRunId;
	FName PickupId = Fdemo_mapSpiritStonePickupContract::PickupId;
	FName SourceId = Fdemo_mapSpiritStonePickupContract::SourceId;
	int64 Value = Fdemo_mapSpiritStonePickupContract::Value;
};

struct Fdemo_mapSpiritStonePickupResult
{
	Edemo_mapSpiritStonePickupStatus Status = Edemo_mapSpiritStonePickupStatus::ProfileValidationRejected;
	FString Diagnostic;
	int32 PreviousGeneration = 0;
	int32 CommittedGeneration = 0;
	int64 RiskBefore = 0;
	int64 RiskAfter = 0;
	bool bDiskStateChanged = false;
	Edemo_mapProfileSaveStatus RepositorySaveStatus = Edemo_mapProfileSaveStatus::ValidationRejected;
	TOptional<Fdemo_mapPersistentProfile> CommittedProfile;

	bool IsCommitted() const { return Status == Edemo_mapSpiritStonePickupStatus::Committed; }
};
