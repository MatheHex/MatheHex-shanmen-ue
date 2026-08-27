#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"
#include "demo_mapProfilePreparationTypes.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

/** Product-facing result of one authority-native preparation equipment command. */
enum class Edemo_mapShanmenPreparationAdapterStatus : uint8
{
	Accepted,
	NoChange,
	CleanupPending,
	AuthorityNotReady,
	InvalidAuthority,
	InvalidSlot,
	ItemNotFound,
	DuplicateSelection,
	SlotRejected,
	CommandRejected,
	DeployedSelectionLocked
};

/** Read-only preparation projection derived from the sole ShanmenItems authority. */
struct Fdemo_mapShanmenPreparationAuthorityProjection
{
	FGuid OwnerId;
	FGuid ScopeId;
	int32 AuthorityRevision = INDEX_NONE;
	TArray<Fdemo_mapProfilePreparationStashRow> OrderedRows;
	Fdemo_mapPersistentWarehouseLayout WarehouseLayout;
	FGuid SelectedWeaponId;
	FGuid SelectedArmorId;
	FGuid SelectedAccessoryId;
	FGuid SelectedSpatialRingId;
	FGuid SelectedBackpackId;
	FString Diagnostic;
};

struct Fdemo_mapShanmenPreparationAdapterResult
{
	Edemo_mapShanmenPreparationAdapterStatus Status =
		Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority;
	FString Diagnostic;
	Fdemo_mapShanmenPreparationAuthorityProjection Projection;

	bool IsAccepted() const
	{
		return Status == Edemo_mapShanmenPreparationAdapterStatus::Accepted
			|| Status == Edemo_mapShanmenPreparationAdapterStatus::NoChange;
	}
};

/**
 * One-way adapter from the existing preparation UI contract into ShanmenItems.
 *
 * A Reserved DeploymentLock is the durable selection intent. Existing migrated
 * equipment containers are read only as the initial baseline when a slot has
 * no reservation history. A terminal latest reservation is an explicit empty
 * tombstone, so clearing a migrated slot survives process restart without a
 * second persistence schema or a legacy Profile write.
 */
struct Fdemo_mapShanmenPreparationAdapter
{
	static bool BuildProjection(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ExpectedOwnerId,
		Fdemo_mapShanmenPreparationAuthorityProjection& OutProjection,
		FString* OutDiagnostic = nullptr);

	static Fdemo_mapShanmenPreparationAdapterResult SelectEquipment(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		FName SlotId,
		const FGuid& ItemInstanceId);
};
