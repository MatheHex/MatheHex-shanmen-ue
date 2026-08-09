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

/** Intent only: the UI provides no mutable item graph and no direct write. */
struct Fdemo_map0909BWarehouseIntent
{
	FGuid ItemId;
	FGuid SourceContainerId;
	int32 SourceSlot = INDEX_NONE;
	FGuid TargetContainerId;
	int32 TargetSlot = INDEX_NONE;
	int32 ExpectedGraphRevision = INDEX_NONE;
};

/**
 * The P2 sect warehouse application layer.  It owns one hydrated P5 Code B
 * repository while the presentation is open; every accepted change is first
 * a P1 transaction and then a single P5 durable replacement.
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
	bool ApplyDragIntent(
		const Fdemo_map0909BWarehouseIntent& Intent,
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
	bool IsP5LayoutContainer(const FGuid& ContainerId) const;
	bool ValidateCompleteSpatialClosure(const FGuid& RootItemId, FString& OutDiagnostic) const;
	static demo_map_code_b::ECodeBOperation ResolveDragOperation(
		const demo_map_code_b::FCodeBRepository& Repository,
		const Fdemo_map0909BWarehouseIntent& Intent);

	TUniquePtr<FCodeBOutOfRaidProfileStore> Store;
	demo_map_code_b::FCodeBRepository Repository;
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
};
