#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenItemTypes.h"
#include "demo_mapProfilePreparationTypes.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

/** Product-facing result of one authority-native preparation command. */
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
	MaterialRejected,
	SelectionLimitExceeded,
	HotbarRejected,
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
	TArray<FGuid> OrderedSelectedMaterialIds;
	Fdemo_mapHotbarBindingSnapshot HotbarBindings;
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

/** One immutable line in the authority-native prepared loadout receipt. */
struct Fdemo_mapShanmenPreparedLoadoutLine
{
	FGuid ReservationId;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	EShanmenItemResourceKind ResourceKind =
		EShanmenItemResourceKind::Quantity;
	int32 Amount = 0;
	FName PurposeId = NAME_None;
	/** Exact cell captured before a complete Quantity stack is consumed. */
	FGuid SourceContainerId;
	int32 SourceSlotIndex = INDEX_NONE;
};

/**
 * Restart-reconstructible receipt for one atomically committed preparation.
 * Every field is derived from the ShanmenItems batch ledger and reservations;
 * no retired Profile item field participates.
 */
struct Fdemo_mapShanmenPreparedLoadoutReceipt
{
	FGuid BatchRequestId;
	FGuid BatchReceiptId;
	FGuid OwnerId;
	FGuid ScopeId;
	int32 AuthorityRevision = INDEX_NONE;
	FGuid WeaponItemInstanceId;
	FGuid ArmorItemInstanceId;
	FGuid AccessoryItemInstanceId;
	FGuid SpatialRingItemInstanceId;
	FGuid BackpackItemInstanceId;
	TArray<FGuid> OrderedRunInventoryItemInstanceIds;
	TArray<FGuid> HotbarItemInstanceIds;
	TArray<Fdemo_mapShanmenPreparedLoadoutLine> OrderedLines;

	bool IsValid() const
	{
		return BatchRequestId.IsValid() && BatchReceiptId.IsValid()
			&& OwnerId.IsValid() && ScopeId.IsValid()
			&& AuthorityRevision >= 0 && !OrderedLines.IsEmpty()
			&& HotbarItemInstanceIds.Num()
				== Fdemo_mapPersistentPreparationLayout::HotbarSlotCount;
	}
};

struct Fdemo_mapShanmenPreparedLoadoutResult
{
	Edemo_mapShanmenPreparationAdapterStatus Status =
		Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority;
	FString Diagnostic;
	FShanmenItemDurableCommandResult Command;
	Fdemo_mapShanmenPreparedLoadoutReceipt Receipt;

	bool IsCommitted() const
	{
		return (Status == Edemo_mapShanmenPreparationAdapterStatus::Accepted
			|| Status == Edemo_mapShanmenPreparationAdapterStatus::NoChange)
			&& Receipt.IsValid();
	}
};

/**
 * One-way adapter from the existing preparation UI contract into ShanmenItems.
 *
 * Equipment uses a reserved DeploymentLock. Complete Material/Consumable
 * stacks use a reserved Quantity whose Purpose carries stable order and one
 * optional Hotbar slot. Existing migrated equipment containers are read only
 * as the initial baseline when a slot has no reservation history. Terminal
 * equipment history is an explicit empty tombstone. All durable state remains
 * inside the sole ShanmenItems document; the retired Profile is never written.
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

	/** Reserve or release one complete Material/Consumable stack for the next Run. */
	static Fdemo_mapShanmenPreparationAdapterResult SelectMaterial(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FGuid& ItemInstanceId,
		bool bSelected);

	/** Bind one selected Base Quick consumable to exactly one external Hotbar slot. */
	static Fdemo_mapShanmenPreparationAdapterResult SetHotbarSlot(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		int32 ExternalSlotNumber,
		const FGuid& ItemInstanceId);

	/** Validate and commit every current equipment and RunInventory intent once. */
	static Fdemo_mapShanmenPreparedLoadoutResult CommitPreparedLoadout(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority);

	/** Commit the current loadout and publish its ActiveRunId in one write. */
	static Fdemo_mapShanmenPreparedLoadoutResult StartPreparedLoadout(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority);
};
