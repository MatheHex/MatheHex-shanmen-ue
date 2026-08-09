#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

class Fdemo_mapProfileRepository;

enum class Edemo_mapWarehouseMoveStatus : uint8
{
	Committed,
	NoOp,
	SessionNotReady,
	InvalidSlot,
	SourceEmpty,
	CapacityExceeded,
	StaleIntent,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapWarehouseMoveIntent
{
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	int32 SourceSlotIndex = INDEX_NONE;
	int32 TargetSlotIndex = INDEX_NONE;
};

struct Fdemo_mapWarehouseMoveResult
{
	Edemo_mapWarehouseMoveStatus Status =
		Edemo_mapWarehouseMoveStatus::SessionNotReady;
	FString Diagnostic;
	int32 PreviousGeneration = 0;
	int32 CommittedGeneration = 0;
	bool bDiskStateChanged = false;
	TOptional<Fdemo_mapPersistentProfile> CommittedProfile;

	bool IsSuccess() const
	{
		return Status == Edemo_mapWarehouseMoveStatus::Committed
			|| Status == Edemo_mapWarehouseMoveStatus::NoOp;
	}
};

struct Fdemo_mapWarehouseSlotProjection
{
	static TArray<FGuid> Build(
		const Fdemo_mapPersistentProfile& Profile,
		bool* OutOverflow = nullptr);
};

class Fdemo_mapPersistentWarehouseTransaction
{
public:
	Fdemo_mapWarehouseMoveResult Execute(
		Fdemo_mapPersistentProfile& InOutProfile,
		const Fdemo_mapWarehouseMoveIntent& Intent,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
