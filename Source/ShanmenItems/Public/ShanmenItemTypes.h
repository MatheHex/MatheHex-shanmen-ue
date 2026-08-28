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
	Depleted,
	/** Terminal audit tombstone for an identity lost with a Run. */
	Destroyed
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
	FinalizePreparedRun,
	/** Atomically commits all prepared reservations and publishes one active Run. */
	StartPreparedRun,
	/** Durably consumes one Quantity unit from an already-started prepared Run. */
	ConsumePreparedRunItem,
	/** Atomically commits triggered Durability/Charges reservations for one active Run. */
	CommitPreparedRunResources,
	/** Durably freezes one external-impact decision over pending Run resources. */
	PreparePreparedRunResourceIntent,
	/** Atomically commits triggered lines and cancels every other prepared line. */
	FinalizePreparedRunResourceIntent
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
	SourcePlacementUnavailable,
	AcquiredItemMismatch,
	ImportPlacementUnavailable,
	/** Runtime's expected prepared-Run quantity no longer matches the durable ledger. */
	RunItemQuantityConflict,
	/** A resource intent cannot overlap another still-pending external mutation. */
	ResourceIntentConflict,
	/** The requested durable resource intent does not exist or has the wrong identity. */
	ResourceIntentNotFound
};

/** Authority-independent terminal reason for one claimed prepared Run. */
UENUM(BlueprintType)
enum class EShanmenItemRunTerminalReason : uint8
{
	None,
	Extraction,
	Death,
	Abandon
};

/** Authority-owned reward provenance. Product adapters validate policy semantics. */
UENUM(BlueprintType)
enum class EShanmenItemRewardEventKind : uint8
{
	None,
	Jackpot
};

UENUM(BlueprintType)
enum class EShanmenItemRewardAffixTier : uint8
{
	None = 0,
	Tier1 = 1,
	Tier2 = 2,
	Tier3 = 3
};

UENUM(BlueprintType)
enum class EShanmenItemRewardAffixAcquisition : uint8
{
	None,
	Natural,
	PityGuaranteed
};

/** One resolved, immutable affix value retained independently of product registries. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemResolvedRewardAffix
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FName AffixId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	EShanmenItemRewardAffixTier Tier = EShanmenItemRewardAffixTier::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	int32 ResolvedMagnitudeScaled = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward", meta = (ClampMin = "1"))
	int64 ResolvedValue = 0;

	bool IsValid() const;
	bool operator==(const FShanmenItemResolvedRewardAffix& Other) const;
};

/**
 * Canonical item provenance persisted by ShanmenItems. It carries only resolved
 * facts; product policy/category validation remains at the demo_map boundary.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRewardMetadata
{
	GENERATED_BODY()

	static constexpr int32 NormalMultiplierBps = 10000;
	static constexpr int32 JackpotMultiplierBps = 60000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	EShanmenItemRewardEventKind RewardEventKind =
		EShanmenItemRewardEventKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FGuid RewardEventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	int32 RewardValueMultiplierBps = NormalMultiplierBps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FName RewardSourceRoleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FGuid RareRewardEventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FName RareRewardPolicyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FName RareRewardTierId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward", meta = (ClampMin = "0"))
	int64 RareRewardBonusValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FGuid AffixSetEventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	FName AffixPolicyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	EShanmenItemRewardAffixAcquisition AffixAcquisition =
		EShanmenItemRewardAffixAcquisition::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items|Reward")
	TArray<FShanmenItemResolvedRewardAffix> Affixes;

	bool IsEmpty() const;
	bool IsValid() const;
	bool operator==(const FShanmenItemRewardMetadata& Other) const;
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

	/** Immutable acquisition provenance retained through storage and tombstones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenItemRewardMetadata RewardMetadata;

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

/**
 * One ordered command that consumes every pending preparation reservation and
 * publishes the only active Run in the same authority revision. RequestId is
 * also the stable prepared-batch identity used to derive ActiveRunId.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunStartRequest
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

/** Legacy-compatible idempotent claim of one successful CommitBatch entry. */
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

/**
 * Idempotent in-Run consumption against one prepared Quantity reservation.
 * The persistent item remains a preparation tombstone; active-Run quantity is
 * reconstructed from the start receipt minus these ordered consumption receipts.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunConsumeRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ActiveRunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "1"))
	int32 Amount = 1;

	/** Compare-and-swap guard from the transient Runtime projection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "1"))
	int32 ExpectedQuantityBefore = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName PurposeId = NAME_None;

	bool IsValid() const;
};

/** One triggered defense source mapped back to its exact pending item reservation. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunResourceCommitLine
{
	GENERATED_BODY()

	/** The defense LayerId captured before resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ReservationId;

	/** The defense SourceInstanceId captured by CombatCore. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	bool IsValid() const;
	bool operator==(const FShanmenItemRunResourceCommitLine& Other) const;
};

/**
 * One idempotent commit point for every resource-backed defense layer triggered
 * by a single accepted impact. The repository accepts only pending Durability
 * or Charges reservations owned by equipment deployed in the exact ActiveRun.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunResourceCommitRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ActiveRunId;

	/** Resolver trigger order is canonical and therefore part of the fingerprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemRunResourceCommitLine> OrderedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName PurposeId = NAME_None;

	bool IsValid() const;
};

/**
 * Durable prepare half of one external-impact saga.
 *
 * OrderedLines always stores triggered lines first, followed by non-triggered
 * lines. TriggeredLineCount freezes the split. The repository changes no
 * resource totals here; it only pins every still-pending reservation to the
 * opaque, product-authored IntentMetadata before the external mutation occurs.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunResourceIntentRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ActiveRunId;

	/** Stable external operation identity; Combat uses the exact ImpactId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid IntentId;

	/** Triggered prefix followed by the ordered non-triggered resource layers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemRunResourceCommitLine> OrderedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 TriggeredLineCount = 0;

	/** Opaque immutable payload required to reconstruct the external CAS command. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName IntentMetadata = NAME_None;

	bool IsValid() const;
};

/** Durable terminal decision for one exact prepared external-impact intent. */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunResourceIntentFinalizeRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenOperationContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ActiveRunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid PrepareRequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid IntentId;

	/** True only after the external authority committed or replayed this intent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	bool bExternalCommitSucceeded = false;

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

/**
 * One Runtime-created identity accepted into the persistent authority at
 * extraction. Definition is carried in full so loot can introduce content
 * that the one-time legacy migration did not already own.
 */
USTRUCT(BlueprintType)
struct SHANMENITEMS_API FShanmenItemRunAcquiredItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenItemDefinition Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FShanmenItemRewardMetadata RewardMetadata;

	/** Optional empty item-owned container created with the acquired identity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	FName ChildContainerType = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items", meta = (ClampMin = "0"))
	int32 ChildContainerCapacity = 0;

	bool IsValid() const;
	bool operator==(const FShanmenItemRunAcquiredItem& Other) const;
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

	/** Runtime-created identities imported only by Extraction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Items")
	TArray<FShanmenItemRunAcquiredItem> AcquiredItems;

	bool IsValid() const;
};

/**
 * Backward-compatible placement codec for complete-stack reservations.
 * Placement lives inside PurposeId, so pre-metadata snapshot JSON and its SHA stay
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
	static FName Death();
	static FName Abandon();
	static FName RecoveredStorage();
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
