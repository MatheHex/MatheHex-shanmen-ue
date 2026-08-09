#pragma once

#include "CoreMinimal.h"
#include "CodeB/demo_mapCodeBLoadoutSelection.h"
#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "demo_map0909BFrameworkTypes.h"

/** Read-only data rendered by the new sect warehouse UI. */
struct Fdemo_map0909BWarehousePresentation
{
	bool bOpen = false;
	bool bCanWrite = false;
	FGuid OwnerId;
	int32 PersistentRevision = INDEX_NONE;
	FString GateDiagnostic;
	demo_map_code_b::FCodeBP2Projection Projection;
	FCodeBLoadoutSelection LoadoutSelection;
};

/**
 * Read-only StartAttempt adapter. P23 moved all visible P5 writes to the
 * shared P3/P4 workspace; this service only rehydrates the durable graph and
 * captures its authoritative LoadoutSelection immediately before deployment.
 */
class Fdemo_map0909BSectWarehouseService
{
public:
	bool OpenForSect(
		const FString& StorageRoot,
		const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot,
		Edemo_map0909BTopState CoordinatorState,
		Fdemo_map0909BWarehousePresentation& OutPresentation,
		FString& OutDiagnostic);
	bool CaptureLoadoutSelection(
		FCodeBLoadoutSelection& OutSelection,
		FString& OutDiagnostic) const;
	void Reset();

private:
	bool BuildPresentation(
		Edemo_map0909BTopState CoordinatorState,
		Fdemo_map0909BWarehousePresentation& OutPresentation,
		FString& OutDiagnostic) const;
	TUniquePtr<FCodeBOutOfRaidProfileStore> Store;
	demo_map_code_b::FCodeBRepository Repository;
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
};
