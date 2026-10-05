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

/** Active-Run edit capability; never widens the preparation edit port. */
struct SHANMENITEMS_API FShanmenItemRunGridRequest
{
	FShanmenItemGridRequest Grid;
	FGuid ActiveRunId;
};

/** Materializes only the already accepted immutable plan, never caller loot. */
struct SHANMENITEMS_API FShanmenItemSourceMaterializeRequest
{
	FShanmenOperationContext Context;
	FGuid ActiveRunId;
	FName SourceRoleId;
	static FGuid MakeRequestId(const FGuid& Owner, const FGuid& Run, FName Role);
};

struct FShanmenItemAuthoritySnapshot;
enum class EShanmenItemTransactionError : uint8;

/** Whole-instance transfer, not generation. Split first to discard part of a stack.
 * Integer centimetres are frozen in the durable receipt, never in an Actor ledger. */
struct SHANMENITEMS_API FShanmenItemGroundDropRequest
{
	FShanmenOperationContext Context;
	FGuid ActiveRunId, ItemInstanceId;
	FIntVector Position = FIntVector::ZeroValue;
	int32 ExpectedAuthorityRevision = 0, ExpectedItemRevision = 0;
	static FGuid MakeRequestId(const FGuid& Owner, const FGuid& Scope, const FGuid& Run, const FGuid& Item, int32 ItemRevision);
	FGuid ContainerId() const;
	FGuid Fingerprint() const;
};

/** Read-only world presentation. Empty is derived from the real container slots. */
struct SHANMENITEMS_API FShanmenItemGroundDropView
{
	FGuid ContainerId;
	FIntVector Position = FIntVector::ZeroValue;
	bool bEmpty = false;
};

class SHANMENITEMS_API FShanmenItemGroundDropPolicy
{
public:
	static bool IsPositionValid(const FIntVector& Position);
	static FName EncodePosition(const FIntVector& Position);
	static bool DecodePosition(FName Encoded, FIntVector& Out);
	static bool Read(const FShanmenItemAuthoritySnapshot& Authority, const FGuid& Owner, const FGuid& Scope,
		const FGuid& ActiveRun, TArray<FShanmenItemGroundDropView>& Out);
};

/** No caller quantities: the authority reconstructs the exact remaining balance. */
struct SHANMENITEMS_API FShanmenItemRunInventoryRequest
{
	FShanmenOperationContext Context;
	FGuid ActiveRunId;
	static FGuid MakeRequestId(const FGuid& Owner, const FGuid& Scope, const FGuid& Run);
	FGuid Fingerprint() const;
};

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

/** Read-only projection of the SAME authority item graph. Before one-way Run
 * transfer, prepared balances are overlaid exclusively from existing receipts;
 * afterwards the projection is the actual graph, without a second balance.
 * Never pass a display snapshot to authority loading or saving: writes accept
 * command requests, not this projection. */
class SHANMENITEMS_API FShanmenItemRunGridPolicy
{
public:
	static bool IsMaterialized(const FShanmenItemAuthoritySnapshot& Authority, const FGuid& Run);
	static bool IsTransferred(const FShanmenItemAuthoritySnapshot& Authority, const FGuid& Run, const FGuid& Reservation);
	static bool Project(const FShanmenItemAuthoritySnapshot& Authority, const FGuid& Owner,
		const FGuid& Scope, const FGuid& Run, FShanmenItemAuthoritySnapshot& Out);
};
