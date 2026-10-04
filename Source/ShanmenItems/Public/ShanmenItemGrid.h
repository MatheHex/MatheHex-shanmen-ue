#pragma once

#include "CoreMinimal.h"
#include "ShanmenCoreTypes.h"
#include "ShanmenItemGrid.generated.h"

/** Geometry/policy only. Quantities and ownership remain in ItemInstance/Container. */
USTRUCT()
struct SHANMENITEMS_API FShanmenItemFootprint
{
	GENERATED_BODY()
	UPROPERTY() FName DefinitionId;
	UPROPERTY() int32 Width = 1;
	UPROPERTY() int32 Height = 1;
	UPROPERTY() FName EquipmentRole;
	UPROPERTY() bool bSecureAllowed = false;
	UPROPERTY() bool bStorageEquipment = false;
	bool operator==(const FShanmenItemFootprint& Other) const;
};

UENUM()
enum class EShanmenItemGridKind : uint8 { Stash, Carry, Secure, Equipment, World };

USTRUCT()
struct SHANMENITEMS_API FShanmenItemGridLayout
{
	GENERATED_BODY()
	UPROPERTY() FGuid ContainerId;
	UPROPERTY() int32 Width = 1;
	UPROPERTY() int32 Height = 1;
	UPROPERTY() EShanmenItemGridKind Kind = EShanmenItemGridKind::Stash;
	/** Single logical equipment slot; the item's bag footprint is not its slot size. */
	UPROPERTY() FName EquipmentRole;
	bool operator==(const FShanmenItemGridLayout& Other) const;
};

USTRUCT()
struct SHANMENITEMS_API FShanmenItemStorageDefinition
{
	GENERATED_BODY()
	UPROPERTY() FName DefinitionId;
	UPROPERTY() EShanmenItemGridKind Kind = EShanmenItemGridKind::Carry;
	UPROPERTY() int32 Width = 1;
	UPROPERTY() int32 Height = 1;
	bool operator==(const FShanmenItemStorageDefinition& Other) const;
};

USTRUCT()
struct SHANMENITEMS_API FShanmenItemGridSnapshot
{
	GENERATED_BODY()
	UPROPERTY() TArray<FShanmenItemFootprint> Footprints;
	UPROPERTY() TArray<FShanmenItemGridLayout> Layouts;
	/** Only orientation; anchor remains the existing ItemInstance.SlotIndex. */
	UPROPERTY() TArray<FGuid> RotatedItems;
	/** Frozen storage capacities, not caller-supplied resize dimensions. */
	UPROPERTY() TArray<FShanmenItemStorageDefinition> StorageDefinitions;
	bool IsEmpty() const;
	void Canonicalize();
	bool operator==(const FShanmenItemGridSnapshot& Other) const;
};

enum class EShanmenItemGridAction : uint8 { Move, Split, Merge, Equip };

struct SHANMENITEMS_API FShanmenItemGridRequest
{
	FShanmenOperationContext Context;
	EShanmenItemGridAction Action = EShanmenItemGridAction::Move;
	FGuid ItemInstanceId;
	FGuid DestinationContainerId;
	FGuid MergeTargetId;
	int32 X = 0;
	int32 Y = 0;
	bool bRotated = false;
	int32 Amount = 0;
	int32 ExpectedAuthorityRevision = 0;
	int32 ExpectedItemRevision = 0;
	int32 ExpectedTargetRevision = 0;
};

struct FShanmenItemAuthoritySnapshot;
enum class EShanmenItemTransactionError : uint8;

/** Shared by authority and read-only drag preview; does not mutate a snapshot. */
class SHANMENITEMS_API FShanmenItemGridPolicy
{
public:
	static bool Validate(const FShanmenItemAuthoritySnapshot& Snapshot);
	/** Only call on a private command candidate. Preserve coordinates, refuse overflow. */
	static EShanmenItemTransactionError ReconcileStorage(FShanmenItemAuthoritySnapshot& Candidate);
	static EShanmenItemTransactionError CanPlace(const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemId, const FGuid& ContainerId, int32 X, int32 Y, bool bRotated,
		bool bIgnoreOriginal = true);
};
