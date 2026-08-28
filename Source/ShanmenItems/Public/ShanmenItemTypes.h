#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenCoreTypes.h"

#include "ShanmenItemTypes.generated.h"

UENUM(BlueprintType)
enum class EShanmenItemResourceKind : uint8
{
	Quantity,
	DeploymentLock,
	Durability,
	Charges
};

UENUM(BlueprintType)
enum class EShanmenItemInstanceState : uint8
{
	Stored,
	Deployed,
	Depleted
};

UENUM(BlueprintType)
enum class EShanmenItemReservationState : uint8
{
	Reserved,
	Committed,
	Cancelled,
	Released
};

UENUM(BlueprintType)
enum class EShanmenItemTransactionOperation : uint8
{
	Reserve,
	Commit,
	Cancel,
	ReleaseDeployment,
	/** Atomically commits an ordered set of already-reserved resources. */
	CommitBatch,
	/** Atomically replaces metadata on one still-pending reservation. */
	AmendReservationPurpose,
	/** Durably claims one committed preparation batch as the only active Run. */
	ClaimPreparedRun,
	/** Atomically reconciles one claimed Run and publishes its terminal marker. */
	FinalizePreparedRun
};

UENUM(BlueprintType)
enum class EShanmenItemTransactionPhase : uint8
{
	Rejected,
	Reserved,
	Committed,
	Cancelled,
	Released
};

UENUM(BlueprintType)
enum class EShanmenItemTransactionError : uint8
{
	None,
	NotInitialized,
	InvalidRequest,
	InvalidSnapshot,
	ContentMismatch,
	RequestIdConflict,
	DefinitionNotFound,
	ContainerNotFound,
	ItemNotFound,
	ScopeMismatch,
	StaleItemRevision,
	ResourceUnsupported,
	InsufficientResource,
	ReservationNotFound,
	ReservationScopeMismatch,
	ReservationAlreadyCommitted,
	ReservationAlreadyCancelled,
	ReservationNotCommitted,
	DeploymentMismatch,
	InvariantViolation,
	ReservationPurposeMismatch,
	PreparedBatchNotFound,
	PreparedBatchAlreadyClaimed,
	ActiveRunConflict,
	RunNotFound,
	RunAlreadyFinalized,
	RunTerminalReasonUnsupported,
	SecuredItemMismatch,
	SourcePlacementUnavailable
};

/** Authority-independent terminal reason; P1.9 deliberately admits extraction only. */
UENUM(BlueprintType)
enum class EShanmenItemRunTerminalReason : uint8
{
	None,
	Extraction,
	Death,
	Abandon
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName DefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGameplayTagContainer ItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 MaxDurability = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 MaxCharges = 0;

	bool IsValid() const;
	bool Supports(EShanmenItemResourceKind Kind) const;
	bool operator==(const FShanmenItemDefinition& Other) const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ContainerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid RunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName ContainerType = NAME_None;

	/** Fixed-width logical slots. Invalid GUID means empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FGuid> Slots;

	bool IsValid() const;
	bool operator==(const FShanmenItemContainer& Other) const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName DefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid RunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ParentContainerId;

	/** Optional item-owned storage. P1.1 retains the legacy one-level container graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ChildContainerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 Durability = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 Charges = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 Revision = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	EShanmenItemInstanceState State = EShanmenItemInstanceState::Stored;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid DeploymentReservationId;

	bool operator==(const FShanmenItemInstance& Other) const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemReserveRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	EShanmenItemResourceKind ResourceKind = EShanmenItemResourceKind::Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "1"))
	int32 Amount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 ExpectedItemRevision = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName PurposeId = NAME_None;

	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemReservationActionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ReservationId;

	bool IsValid() const;
};

/**
 * One ordered, idempotent authority command that commits all reservations or
 * none of them. The order is part of the command fingerprint and is retained
 * by the receipt so product adapters can rebuild an immutable loadout after a
 * process restart.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemReservationBatchRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FGuid> ReservationIds;

	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemReservationAmendRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ReservationId;

	/** Compare-and-swap guard against overwriting newer reservation metadata. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName ExpectedPurposeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName PurposeId = NAME_None;

	bool IsValid() const;
};

/** Idempotent claim of one successful CommitBatch ledger entry. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunClaimRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	/** RequestId of the successful preparation CommitBatch to consume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid PreparedBatchRequestId;

	bool IsValid() const;
};

/** Remaining quantity of one prepared original observed in Runtime settlement. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunSecuredOriginal
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "1"))
	int32 RemainingQuantity = 1;

	bool IsValid() const;
	bool operator==(const FShanmenItemRunSecuredOriginal& Other) const;
};

/** Atomic terminal reconciliation for one already-claimed prepared Run. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunFinalizeRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ActiveRunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	EShanmenItemRunTerminalReason TerminalReason =
		EShanmenItemRunTerminalReason::None;

	/** Prepared originals that survived; absence means zero remaining. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemRunSecuredOriginal> SecuredOriginals;

	bool IsValid() const;
};

/**
 * Backward-compatible placement codec for complete-stack reservations.
 * Placement lives inside PurposeId, so schema-1 snapshot JSON and its SHA stay
 * byte-compatible while a committed Quantity can still return to its exact
 * source cell after Runtime extraction.
 */
struct SHANMENITEMS_API FShanmenItemReservationPlacement
{
	static FName Encode(
		FName LogicalPurposeId,
		const FGuid& SourceContainerId,
		int32 SourceSlotIndex);
	static bool Decode(
		FName EncodedPurposeId,
		FName& OutLogicalPurposeId,
		FGuid& OutSourceContainerId,
		int32& OutSourceSlotIndex);
};

/** Stable semantic values stored in lifecycle receipts' existing PurposeId. */
struct SHANMENITEMS_API FShanmenItemRunLifecyclePurpose
{
	static FName Active();
	static FName Extraction();
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemTransactionReceipt
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	bool bSuccess = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	EShanmenItemTransactionOperation Operation = EShanmenItemTransactionOperation::Reserve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	EShanmenItemTransactionPhase Phase = EShanmenItemTransactionPhase::Rejected;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	EShanmenItemTransactionError Error = EShanmenItemTransactionError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	FGuid RequestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	FGuid ReservationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	EShanmenItemResourceKind ResourceKind = EShanmenItemResourceKind::Quantity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	int32 Amount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	int32 ResourceBefore = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	int32 ResourceAfter = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	int32 AvailableAfter = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	int32 ItemRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	int32 AuthorityRevision = INDEX_NONE;

	/** Effective metadata after Reserve or AmendReservationPurpose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	FName PurposeId = NAME_None;

	/** Populated only by CommitBatch; ordered identity of every committed line. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Items")
	TArray<FGuid> ReservationIds;

	bool IsSuccess() const;
	bool IsValid() const;
	bool operator==(const FShanmenItemTransactionReceipt& Other) const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemReservationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ReservationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ReserveRequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid RunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	EShanmenItemResourceKind ResourceKind = EShanmenItemResourceKind::Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	int32 ItemRevisionAtReserve = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName PurposeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	EShanmenItemReservationState State = EShanmenItemReservationState::Reserved;

	bool IsValid() const;
	bool operator==(const FShanmenItemReservationSnapshot& Other) const;
};

USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemProcessedRequestSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid Fingerprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenItemTransactionReceipt Receipt;

	bool IsValid() const;
	bool operator==(const FShanmenItemProcessedRequestSnapshot& Other) const;
};

/** Complete persistence boundary for item graph, reservations, and idempotency ledger. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemAuthoritySnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	int32 AuthorityRevision = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenContentStamp Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemDefinition> Definitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemContainer> Containers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemInstance> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemReservationSnapshot> Reservations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemProcessedRequestSnapshot> ProcessedRequests;

	bool operator==(const FShanmenItemAuthoritySnapshot& Other) const;
};
