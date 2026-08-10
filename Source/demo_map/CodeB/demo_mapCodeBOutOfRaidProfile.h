#pragma once

#include "CodeB/demo_mapCodeBP2.h"
#include "CodeB/demo_mapCodeBLoadoutSelection.h"
#include "demo_mapProfileSessionTypes.h"

/** The durable state of the one-way, Profile-owned Code B out-of-raid handoff. */
enum class ECodeBOutOfRaidHandoffState : uint8
{
	Prepared,
	Committed
};

struct FCodeBOutOfRaidHandoffMapping
{
	FGuid LegacyItemId;
	FGuid CodeBItemId;
};

struct FCodeBOutOfRaidHandoffReceipt
{
	ECodeBOutOfRaidHandoffState State = ECodeBOutOfRaidHandoffState::Prepared;
	FGuid SourceProfileId;
	FString SourceFingerprint;
	FString StartedUtc;
	FString CommittedUtc;
	TArray<FCodeBOutOfRaidHandoffMapping> ItemMappings;
	TArray<FString> InvalidLegacyEntries;
};

/** Immutable evidence of one Code A RecoveredAbandon-authorized P6 rebind. */
struct FCodeBRunInventoryRecoveryRebind
{
	int32 Sequence = 0;
	FGuid OldRunId;
	FGuid NewRunId;
	FString CodeATerminalCause;
	FString ReboundUtc;
};

/**
 * P13 stores a hotbar as nine stable references only.  An empty slot is
 * explicit through bHasReference; ItemId is never used as an empty sentinel.
 */
struct FCodeBHotbarBinding
{
	int32 SlotIndex = 1;
	bool bHasReference = false;
	FGuid ItemId;
};

struct FCodeBHotbarBindings
{
	static constexpr int32 SlotCount = 9;

	FCodeBHotbarBindings()
	{
		Slots.Reserve(SlotCount);
		for (int32 SlotIndex = 1; SlotIndex <= SlotCount; ++SlotIndex)
		{
			FCodeBHotbarBinding& Slot = Slots.AddDefaulted_GetRef();
			Slot.SlotIndex = SlotIndex;
		}
	}

	TArray<FCodeBHotbarBinding> Slots;
};

/** Read-only projection materialized from the current P5/P6 graph, never persisted as a second inventory. */
struct FCodeBHotbarSlotProjection
{
	int32 SlotIndex = 1;
	bool bHasReference = false;
	FGuid ItemId;
	FName DefinitionId;
	int32 Quantity = 0;
};

struct FCodeBHotbarProjection
{
	bool bEditable = false;
	bool bActiveRunScope = false;
	int32 DurableRevision = INDEX_NONE;
	TArray<FCodeBHotbarSlotProjection> Slots;
};

/**
 * A frozen description of one item at the instant P6 created its carry
 * receipt. P7 may move the live item through P4/P3/P2/P1, but it must not
 * rewrite this evidence or the receipt digest that protects it.
 */
struct FCodeBRunInventoryReceiptItem
{
	FGuid ItemId;
	FName DefinitionId;
	int32 Quantity = 0;
	FGuid ParentContainerId;
	int32 SlotIndex = INDEX_NONE;
	FGuid ChildContainerId;
};

/** Read-only Code A lifecycle context supplied only after its own terminal decision. */
struct FCodeBRunInventoryRecoveryContext
{
	FGuid RecoveredAbandonRunId;
	FString CodeATerminalCause;

	bool AuthorizesRebindFrom(const FGuid& ExpectedOldRunId) const
	{
		return ExpectedOldRunId.IsValid()
			&& RecoveredAbandonRunId == ExpectedOldRunId
			&& CodeATerminalCause == TEXT("RecoveredAbandon");
	}
};

/** The durable receipt for P6's post-success, non-blocking Run bridge. */
enum class ECodeBRunInventoryBridgeState : uint8
{
	Prepared,
	Committed
};

struct FCodeBRunInventoryBridgeReceipt
{
	ECodeBRunInventoryBridgeState State = ECodeBRunInventoryBridgeState::Prepared;
	FGuid ReceiptId;
	FGuid OwnerId;
	/** Never changes, even when a Prepared receipt is rebound to a new Code A RunId. */
	FGuid OriginRunId;
	/** Current P6 binding. It mirrors FCodeBRunInventorySession::RunInstanceId. */
	FGuid RunInstanceId;
	int32 SourceOutOfRaidRevision = INDEX_NONE;
	FString PreparedUtc;
	FString CommittedUtc;
	TArray<FGuid> MovedItemIds;
	/** Optional on older P6 documents; populated once before the first P7 commit. */
	TArray<FCodeBRunInventoryReceiptItem> ImmutablePayloadItems;
	FString PayloadDigest;
	TArray<FCodeBRunInventoryRecoveryRebind> RecoveryRebindHistory;
	FString RecoveryDiagnostic;
};

/** P14 keeps an in-run ground drop as P6-owned data; Code A actors are projections only. */
enum class ECodeBWorldDropActionState : uint8
{
	Available
};

struct FCodeBWorldDropRecord
{
	/** P31 canonical registry scope. These values must exactly match the owning P6 session. */
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
	/** Stable creation order encoded by WorldDropId; never renumbered after another record is removed. */
	int32 Ordinal = 0;
	/** Deterministically derived from WorldDropId; it is an ordinary P1 storage root. */
	FGuid WorldContainerId;
	/** Foreign key to the one whole P1 item instance held by WorldContainerId. */
	FGuid ItemId;
	/** Exact formal P19 child identity, or invalid for a simple root. */
	FGuid SpatialChildContainerId;
	FName MapRoute;
	FTransform FloorTransform = FTransform::Identity;
	ECodeBWorldDropActionState ActionState = ECodeBWorldDropActionState::Available;
	/** Per-record identity revision. Root quantity changes remain covered by the P6 snapshot revision. */
	int32 RecordRevision = 1;
	/** Durable creation/migration provenance only; never used to infer item authority. */
	FString Provenance;
};

/** Read-only P14 bridge data for the Code A placement/actor adapter. */
struct FCodeBWorldDropProjection
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
	int32 Ordinal = 0;
	FGuid WorldContainerId;
	FGuid ItemId;
	FGuid SpatialChildContainerId;
	FName DefinitionId;
	int32 Quantity = 0;
	FName MapRoute;
	FTransform FloorTransform = FTransform::Identity;
	int32 RecordRevision = INDEX_NONE;
	int32 P6SnapshotRevision = INDEX_NONE;
};

/** P15's durable Code B-to-Code A delivery ledger; it never mirrors an item graph. */
enum class ECodeBQuickUseReceiptState : uint8
{
	Pending,
	Acknowledged
};

struct FCodeBQuickUseReceipt
{
	FGuid ReceiptId;
	int32 ReceiptOrdinal = 0;
	int32 SlotIndex = INDEX_NONE;
	FGuid SourceItemId;
	demo_map_code_b::ECodeBQuickUseEffectKind EffectKind = demo_map_code_b::ECodeBQuickUseEffectKind::None;
	int32 RestoreAmount = 0;
	ECodeBQuickUseReceiptState State = ECodeBQuickUseReceiptState::Pending;
};

/**
 * P6's read-only pending Run inventory authority. It has no Code A Runtime,
 * actor, world, Loot, settlement, or Run Save reference.
 */
struct FCodeBRunInventorySession
{
	static constexpr int32 CurrentSchemaVersion = 6;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid OwnerId;
	FGuid RunInstanceId;
	int32 SessionRevision = 0;
	int32 SourceOutOfRaidRevision = INDEX_NONE;
	FString CreatedUtc;
	FString LastCommittedUtc;
	ECodeBRunInventoryBridgeState BridgeState = ECodeBRunInventoryBridgeState::Prepared;
	FCodeBRunInventoryBridgeReceipt Receipt;
	demo_map_code_b::FCodeBSnapshot RepositorySnapshot;
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
	/** P13's sole editable active-Run binding truth; it has no P1 graph ownership. */
	FCodeBHotbarBindings HotbarBindings;
	/** P14's sole durable in-world item truth. It is empty at every P5->P6 bridge. */
	TArray<FCodeBWorldDropRecord> WorldDrops;
	/** Never decremented or consumed by a rejected request. */
	int32 NextWorldDropOrdinal = 1;
	/** P15's monotonic committed receipt ordinal; rejected use never advances it. */
	TArray<FCodeBQuickUseReceipt> QuickUseReceipts;
	int32 NextQuickUseReceiptOrdinal = 1;
};

enum class ECodeBRunInventoryBridgeStatus : uint8
{
	Committed,
	AlreadyCommitted,
	RecoveredPreparedReceipt,
	NotEnrolled,
	ActiveSessionConflict,
	SelectionMismatch,
	InvalidIdentity,
	StorageFailure
};

struct FCodeBRunInventoryBridgeResult
{
	ECodeBRunInventoryBridgeStatus Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
	FString Diagnostic;
	FCodeBRunInventorySession Session;

	bool IsCommitted() const
	{
		return Status == ECodeBRunInventoryBridgeStatus::Committed
			|| Status == ECodeBRunInventoryBridgeStatus::AlreadyCommitted
			|| Status == ECodeBRunInventoryBridgeStatus::RecoveredPreparedReceipt;
	}
};

/** Code A's already-committed terminal classification, observed by P8 only. */
enum class ECodeBRunInventoryTerminalState : uint8
{
	Unknown,
	Extracted,
	Dead,
	RecoveredAbandon
};

/**
 * One immutable P8 audit record.  It freezes the P7 current session graph at
 * the same durable-record commit that either returns it to P5 or confiscates it.
 */
struct FCodeBRunInventoryTerminalReceipt
{
	FGuid ReceiptId;
	FGuid OwnerId;
	FGuid RunInstanceId;
	ECodeBRunInventoryTerminalState TerminalState = ECodeBRunInventoryTerminalState::Unknown;
	int32 SourceSessionRevision = INDEX_NONE;
	FString CommittedUtc;
	FString FrozenSnapshotDigest;
	demo_map_code_b::FCodeBSnapshot FrozenRunSnapshot;
	demo_map_code_b::FCodeBP2PlayerLayout FrozenRunLayout;
};

enum class ECodeBRunInventoryTerminalStatus : uint8
{
	Committed,
	AlreadyCommitted,
	NotEnrolled,
	NoMatchedSession,
	SessionNotCommitted,
	ConflictingTerminal,
	InvalidIdentity,
	UnknownTerminal,
	StorageFailure
};

struct FCodeBRunInventoryTerminalResult
{
	ECodeBRunInventoryTerminalStatus Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
	FString Diagnostic;
	FCodeBRunInventoryTerminalReceipt Receipt;

	bool IsCommitted() const
	{
		return Status == ECodeBRunInventoryTerminalStatus::Committed
			|| Status == ECodeBRunInventoryTerminalStatus::AlreadyCommitted;
	}
};

/** P9 reserves the lifecycle states; only Closed is materialized in P9. */
enum class ECodeBNormalContainerState : uint8
{
	Closed,
	Opening,
	Open,
	Interrupted
};

/** P9 creates every materialized item Hidden.  Search transitions arrive later. */
enum class ECodeBNormalContainerRevealState : uint8
{
	Hidden,
	Searching,
	Revealed
};

/** Named production timing contract consumed by the P10 world adapter; no UI owns timing truth. */
struct FCodeBNormalContainerInteractionTiming
{
	static constexpr float OpenSeconds = 0.85f;
	static constexpr float ItemSearchSeconds = 0.65f;
};

/** A declarative, non-player-owned item placement in a normal-container definition. */
struct FCodeBNormalContainerContentPlanEntry
{
	FName ItemDefinitionId;
	int32 Quantity = 0;
	int32 SlotIndex = INDEX_NONE;
	/** Optional empty child storage owned by the materialized item. */
	int32 ChildContainerCapacity = 0;
};

/** Immutable production definition used by the P9 exact-target materializer. */
struct FCodeBNormalContainerDefinition
{
	FName DefinitionId;
	FName ContainerType;
	int32 Capacity = 0;
	int32 ContentRevision = 0;
	/** Immutable pre-P16 recipe retained solely to validate historical materialized records. */
	TArray<FCodeBNormalContainerContentPlanEntry> ContentPlan;
};

/** Immutable evidence of one successful normal-container materialization. */
struct FCodeBNormalContainerMaterializationReceipt
{
	FGuid ReceiptId;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid SearchTargetId;
	FName DefinitionId;
	FGuid ContainerId;
	int32 DefinitionContentRevision = 0;
	FString DefinitionDigest;
	/** Empty only for an immutable P9 fixed-recipe record created before P16. */
	FName LootProfileId;
	int32 LootProfileVersion = 0;
	FString LootProfileDigest;
	FString LootAlgorithmVersion;
	FString LootResultDigest;
	FString MaterializationDigest;
	FString MaterializedUtc;
};

struct FCodeBNormalContainerItemReveal
{
	FGuid ItemId;
	ECodeBNormalContainerRevealState RevealState = ECodeBNormalContainerRevealState::Hidden;
};

/**
 * The sole P9 Run-local search-target truth.  Its snapshot is deliberately
 * separate from P5 and the P6/P7 player-carry snapshot.
 */
struct FCodeBRunLocalNormalContainerRecord
{
	/** P10 adds durable, restart-safe action identities to P9's materialized graph. */
	static constexpr int32 CurrentSchemaVersion = 4;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid SearchTargetId;
	FName DefinitionId;
	FGuid ContainerId;
	bool bMaterialized = false;
	ECodeBNormalContainerState State = ECodeBNormalContainerState::Closed;
	/** Non-zero only while the authoritative container open or item search timer is pending. */
	FGuid ActiveActionId;
	/** Non-zero only while exactly one materialized item is in Searching state. */
	FGuid ActiveSearchItemId;
	int32 Revision = 0;
	FCodeBNormalContainerMaterializationReceipt Receipt;
	demo_map_code_b::FCodeBSnapshot ContainerSnapshot;
	TArray<FCodeBNormalContainerItemReveal> ItemRevealStates;
};

/** Read-only P10/P11-facing projection; it carries no write capability. */
struct FCodeBNormalContainerItemProjection
{
	FGuid ItemId;
	FName DefinitionId;
	int32 Quantity = 0;
	FGuid ParentContainerId;
	int32 SlotIndex = INDEX_NONE;
	FGuid ChildContainerId;
	ECodeBNormalContainerRevealState RevealState = ECodeBNormalContainerRevealState::Hidden;
};

struct FCodeBNormalContainerProjection
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid SearchTargetId;
	FName DefinitionId;
	FGuid ContainerId;
	ECodeBNormalContainerState State = ECodeBNormalContainerState::Closed;
	FGuid ActiveActionId;
	FGuid ActiveSearchItemId;
	int32 Revision = 0;
	FCodeBNormalContainerMaterializationReceipt Receipt;
	TArray<FCodeBNormalContainerItemProjection> Items;
};

/** P10's exact-target, durable normal-container action result. */
enum class ECodeBNormalContainerActionStatus : uint8
{
	Committed,
	AlreadyInRequestedState,
	RecoveredInterrupted,
	NotEnrolled,
	InvalidIdentity,
	ActiveSessionNotCommitted,
	NotMaterialized,
	TargetDefinitionConflict,
	InvalidState,
	ItemNotFound,
	ItemNotHidden,
	ActionMismatch,
	StorageFailure
};

struct FCodeBNormalContainerActionResult
{
	ECodeBNormalContainerActionStatus Status = ECodeBNormalContainerActionStatus::StorageFailure;
	FString Diagnostic;
	FCodeBNormalContainerProjection Projection;

	bool IsCommitted() const
	{
		return Status == ECodeBNormalContainerActionStatus::Committed
			|| Status == ECodeBNormalContainerActionStatus::AlreadyInRequestedState;
	}
};

enum class ECodeBNormalContainerMaterializationStatus : uint8
{
	Materialized,
	AlreadyMaterialized,
	NotEnrolled,
	InvalidIdentity,
	ActiveSessionNotCommitted,
	UnknownDefinition,
	TargetDefinitionConflict,
	InvalidDefinition,
	StorageFailure
};

struct FCodeBNormalContainerMaterializationResult
{
	ECodeBNormalContainerMaterializationStatus Status = ECodeBNormalContainerMaterializationStatus::StorageFailure;
	FString Diagnostic;
	FCodeBNormalContainerProjection Projection;

	bool IsMaterialized() const
	{
		return Status == ECodeBNormalContainerMaterializationStatus::Materialized
			|| Status == ECodeBNormalContainerMaterializationStatus::AlreadyMaterialized;
	}
};

/** P12 keeps body contents opaque until the one-at-a-time durable search completes. */
enum class ECodeBBodyContainerVisibility : uint8
{
	Hidden,
	Searching,
	Revealed
};

/** P12 adds only action lifecycle; P8 remains the sole terminal discard authority. */
enum class ECodeBBodyContainerState : uint8
{
	BodyMaterialized,
	Opening,
	Open,
	Interrupted
};

/** Named production cadence shared by the one P12 corpse action surface. */
struct FCodeBBodyContainerInteractionTiming
{
	static constexpr float OpenSeconds = 0.85f;
	static constexpr float ItemSearchSeconds = 0.70f;
};

struct FCodeBBodyContainerContentPlanEntry
{
	FName ItemDefinitionId;
	int32 Quantity = 0;
	int32 SlotIndex = INDEX_NONE;
	int32 ChildContainerCapacity = 0;
};

/** Immutable Code B-only corpse recipe. It is not a P9 normal-container alias. */
struct FCodeBBodyContainerDefinition
{
	FName DefinitionId;
	FName ContainerType;
	int32 Capacity = 0;
	int32 ContentRevision = 0;
	/** Immutable pre-P16 recipe retained solely to validate historical materialized records. */
	TArray<FCodeBBodyContainerContentPlanEntry> ContentPlan;
};

/** Code A forwards this once its own death conclusion is already committed. */
struct FCodeBBodyContainerDeathReceipt
{
	FGuid DeathReceiptId;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid BodyTargetId;
	FName DefinitionId;
	FName StaticSpawnIdentity;
	int32 SpawnOrdinal = INDEX_NONE;
	FString CommittedUtc;
};

struct FCodeBBodyContainerMaterializationReceipt
{
	FGuid ReceiptId;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid BodyTargetId;
	FName DefinitionId;
	FGuid ContainerId;
	int32 DefinitionContentRevision = 0;
	FString DefinitionDigest;
	/** Empty only for an immutable P11 fixed-recipe record created before P16. */
	FName LootProfileId;
	int32 LootProfileVersion = 0;
	FString LootProfileDigest;
	FString LootAlgorithmVersion;
	FString LootResultDigest;
	/** P21 persists r3's three formal non-spatial equipment candidates separately from the Profile digest. */
	FString EquipmentCandidateSetDigest;
	FString MaterializationDigest;
	FCodeBBodyContainerDeathReceipt DeathReceipt;
	FString MaterializedUtc;
};

struct FCodeBBodyContainerItemVisibility
{
	FGuid ItemId;
	ECodeBBodyContainerVisibility Visibility = ECodeBBodyContainerVisibility::Hidden;
};

/** The sole P11 Run-local body-container truth, owned by its exact P6 session. */
struct FCodeBRunLocalBodyContainerRecord
{
	/** P21 adds persisted r3 fixed-equipment-slot provenance while preserving r1--r2 parse support. */
	static constexpr int32 CurrentSchemaVersion = 4;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid BodyTargetId;
	FName DefinitionId;
	FGuid ContainerId;
	bool bMaterialized = false;
	ECodeBBodyContainerState State = ECodeBBodyContainerState::BodyMaterialized;
	int32 Revision = 0;
	FGuid ActiveActionId;
	FGuid ActiveSearchItemId;
	FCodeBBodyContainerMaterializationReceipt Receipt;
	demo_map_code_b::FCodeBSnapshot ContainerSnapshot;
	TArray<FCodeBBodyContainerItemVisibility> ItemVisibilities;
};

/** Read-only P12-facing data. It is intentionally not a UI or drag payload. */
struct FCodeBBodyContainerItemProjection
{
	FGuid ItemId;
	FName DefinitionId;
	int32 Quantity = 0;
	FGuid ParentContainerId;
	int32 SlotIndex = INDEX_NONE;
	FGuid ChildContainerId;
	ECodeBBodyContainerVisibility Visibility = ECodeBBodyContainerVisibility::Hidden;
};

/** Read-only P21 descriptor for one fixed equipment container in the same P11 corpse graph. */
struct FCodeBBodyContainerEquipmentSlotProjection
{
	FName SlotSemantic;
	FGuid ContainerId;
	demo_map_code_b::ECodeBEquipSlot EquipmentSlot = demo_map_code_b::ECodeBEquipSlot::None;
};

struct FCodeBBodyContainerProjection
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid BodyTargetId;
	FName DefinitionId;
	FGuid ContainerId;
	ECodeBBodyContainerState State = ECodeBBodyContainerState::BodyMaterialized;
	int32 Revision = 0;
	FGuid ActiveActionId;
	FGuid ActiveSearchItemId;
	FCodeBBodyContainerMaterializationReceipt Receipt;
	TArray<FCodeBBodyContainerItemProjection> Items;
	TArray<FCodeBBodyContainerEquipmentSlotProjection> EquipmentSlots;
};

enum class ECodeBBodyContainerMaterializationStatus : uint8
{
	Materialized,
	AlreadyMaterialized,
	NotEnrolled,
	InvalidIdentity,
	ActiveSessionNotCommitted,
	UnknownDefinition,
	TargetDefinitionConflict,
	InvalidDeathReceipt,
	InvalidDefinition,
	StorageFailure
};

struct FCodeBBodyContainerMaterializationResult
{
	ECodeBBodyContainerMaterializationStatus Status = ECodeBBodyContainerMaterializationStatus::StorageFailure;
	FString Diagnostic;
	FCodeBBodyContainerProjection Projection;

	bool IsMaterialized() const
	{
		return Status == ECodeBBodyContainerMaterializationStatus::Materialized
			|| Status == ECodeBBodyContainerMaterializationStatus::AlreadyMaterialized;
	}
};

enum class ECodeBBodyContainerActionStatus : uint8
{
	Committed,
	AlreadyInRequestedState,
	NotEnrolled,
	InvalidIdentity,
	ActiveSessionNotCommitted,
	NotMaterialized,
	TargetDefinitionConflict,
	InvalidState,
	ActionMismatch,
	ItemNotFound,
	ItemNotHidden,
	StorageFailure
};

struct FCodeBBodyContainerActionResult
{
	ECodeBBodyContainerActionStatus Status = ECodeBBodyContainerActionStatus::StorageFailure;
	FString Diagnostic;
	FCodeBBodyContainerProjection Projection;

	bool IsCommitted() const
	{
		return Status == ECodeBBodyContainerActionStatus::Committed
			|| Status == ECodeBBodyContainerActionStatus::AlreadyInRequestedState;
	}
};

/**
 * A versioned, per-Profile Code B document.  It deliberately stores only the
 * authoritative Code B snapshot plus an import receipt; no editable Code A
 * inventory projection is retained after handoff.
 */
struct FCodeBOutOfRaidInventoryRecord
{
	static constexpr int32 CurrentSchemaVersion = 8;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid OwnerId;
	int32 PersistentRevision = 0;
	FString CreatedUtc;
	FString LastCommittedUtc;
	demo_map_code_b::FCodeBSnapshot RepositorySnapshot;
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
	/** P13's Profile-side reference truth while no P6 session lock is active. */
	FCodeBHotbarBindings HotbarBindings;
	FCodeBOutOfRaidHandoffReceipt Receipt;
	/** P6 keeps one unresolved Code B Run session per Owner in this atomic document. */
	bool bHasActiveRunInventorySession = false;
	FCodeBRunInventorySession ActiveRunInventorySession;
	/** P8 retains terminal audit history while the active-session lock is released. */
	TArray<FCodeBRunInventoryTerminalReceipt> TerminalReceipts;
	/** P9 Run-local normal containers; P8 discards them when it closes the session. */
	TArray<FCodeBRunLocalNormalContainerRecord> RunLocalNormalContainers;
	/** P11 Run-local body containers; P8 discards them without ever settling their items. */
	TArray<FCodeBRunLocalBodyContainerRecord> RunLocalBodyContainers;
};

struct FCodeBOutOfRaidOpenResult
{
	bool bSuccess = false;
	bool bCreated = false;
	bool bRecoveredPendingReceipt = false;
	FString Diagnostic;
	FCodeBOutOfRaidInventoryRecord Record;
};

/**
 * Sidecar persistence is tied to the formal Profile storage root and stable
 * ProfileId.  Every write is a temp-verify-backup-replace transaction.  The
 * caller owns the live P1 repository; this store only hydrates and durably
 * snapshots it after an accepted P1/P2 transaction.
 */
class FCodeBOutOfRaidProfileStore
{
public:
	FCodeBOutOfRaidProfileStore(FString InStorageRoot, FGuid InOwnerId);

	FCodeBOutOfRaidOpenResult OpenOrMigrate(
		const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot,
		demo_map_code_b::FCodeBRepository& OutRepository,
		demo_map_code_b::FCodeBP2PlayerLayout& OutLayout);
	bool CommitAcceptedSnapshot(
		const demo_map_code_b::FCodeBSnapshot& Snapshot,
		FString* OutError = nullptr);
	/** Read-only P13 projection of the currently unlocked P5 record. */
	bool TryGetOutOfRaidHotbarProjection(
		FCodeBHotbarProjection& OutProjection,
		FString* OutError = nullptr) const;
	/** P13's only Profile-side binding writer. It changes references, never P1 placement. */
	bool BindOutOfRaidHotbarSlot(
		const FGuid& ItemId,
		int32 SlotIndex,
		FCodeBHotbarProjection& OutProjection,
		FString* OutError = nullptr);
	bool UnbindOutOfRaidHotbarSlot(
		int32 SlotIndex,
		FCodeBHotbarProjection& OutProjection,
		FString* OutError = nullptr);
	/** Called only after Code A has successfully activated a stable RunInstanceId. */
	static FCodeBRunInventoryBridgeResult NotifySuccessfulRun(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FCodeBRunInventoryRecoveryContext& RecoveryContext = FCodeBRunInventoryRecoveryContext());
	/** Read-only canonical P5 carry selection. It never creates or mutates a sidecar. */
	static bool BuildLoadoutSelection(
		const FCodeBOutOfRaidInventoryRecord& Record,
		FCodeBLoadoutSelection& OutSelection,
		FString* OutError = nullptr);
	/** Revalidates a captured P5 selection before the existing durable P5 -> P6 bridge. */
	static FCodeBRunInventoryBridgeResult NotifySuccessfulRunWithLoadoutSelection(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FCodeBLoadoutSelection& Selection,
		const FCodeBRunInventoryRecoveryContext& RecoveryContext = FCodeBRunInventoryRecoveryContext());
	/**
	 * P8's post-Code-A observer. It only settles an exact committed P6 session;
	 * absent, prepared, mismatched, repeated, or conflicting notices do not
	 * create, migrate, rebind, or write a record.
	 */
	static FCodeBRunInventoryTerminalResult NotifyCommittedRunTerminal(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		ECodeBRunInventoryTerminalState TerminalState);
	/** Read-only registry lookup for the production normal-container definitions. */
	static const FCodeBNormalContainerDefinition* FindNormalContainerDefinition(FName DefinitionId);
	/** Read-only P11 registry lookup; it has no map, actor, UI, or P9 dependency. */
	static const FCodeBBodyContainerDefinition* FindBodyContainerDefinition(FName DefinitionId);
	/** P11's sole DeathReceipt-gated durable body graph materialization entry. */
	static FCodeBBodyContainerMaterializationResult MaterializeMatchedRunBodyContainerOnDeath(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		const FCodeBBodyContainerDeathReceipt& DeathReceipt);
	/** Read-only P12 preparation query. It never materializes, migrates, or repairs a record. */
	static bool TryGetMatchedRunBodyContainerProjection(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FCodeBBodyContainerProjection& OutProjection,
		FString* OutError = nullptr);
	/** P12 begins an existing materialized body's exact durable opening action. */
	static FCodeBBodyContainerActionResult BeginMatchedRunBodyContainerOpen(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId);
	/** P12 accepts only the exact action id issued by BeginMatchedRunBodyContainerOpen. */
	static FCodeBBodyContainerActionResult CompleteMatchedRunBodyContainerOpen(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		const FGuid& ActionId);
	/** P12 begins the sole legal one-at-a-time Hidden-to-Searching body reveal. */
	static FCodeBBodyContainerActionResult BeginMatchedRunBodyContainerItemSearch(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		const FGuid& ItemId);
	/** P12 completes only the matching durable body-item search action. */
	static FCodeBBodyContainerActionResult CompleteMatchedRunBodyContainerItemSearch(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		const FGuid& ActionId);
	/** P12 cancellation/recovery makes Opening Interrupted and returns only Searching to Hidden. */
	static FCodeBBodyContainerActionResult InterruptMatchedRunBodyContainerAction(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		const FString& Reason);
	/** P12's sole P6/body atomic durable transfer; composite snapshot must already be P1-valid. */
	static bool CommitAcceptedMatchedRunBodyContainerTransfer(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		int32 ExpectedP6SnapshotRevision,
		int32 ExpectedBodyContainerRevision,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CompositeSnapshot,
		FString* OutError = nullptr);
	/**
	 * P9's sole creation gate.  It accepts only an exact Owner/committed-Run/
	 * target/definition identity and writes one Run-local record atomically.
	 */
	static FCodeBNormalContainerMaterializationResult MaterializeMatchedRunNormalContainer(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId);
	/** Read-only P10/P11 facade; it never materializes, opens, reveals, or writes. */
	static bool TryGetMatchedRunNormalContainerProjection(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FCodeBNormalContainerProjection& OutProjection,
		FString* OutError = nullptr);
	/** P10: materialize if required, then begin an exact target's durable opening action. */
	static FCodeBNormalContainerActionResult BeginMatchedRunNormalContainerOpen(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId);
	/** P10: accepts only the exact action id issued by BeginMatchedRunNormalContainerOpen. */
	static FCodeBNormalContainerActionResult CompleteMatchedRunNormalContainerOpen(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		const FGuid& ActionId);
	/** P10: begins the sole legal one-at-a-time reveal for a Hidden item in an Open target. */
	static FCodeBNormalContainerActionResult BeginMatchedRunNormalContainerItemSearch(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		const FGuid& ItemId);
	/** P10: completes only the matching durable item-search action. */
	static FCodeBNormalContainerActionResult CompleteMatchedRunNormalContainerItemSearch(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		const FGuid& ActionId);
	/** P10 cancellation/recovery entry. Opening becomes Interrupted; Searching returns only that item to Hidden. */
	static FCodeBNormalContainerActionResult InterruptMatchedRunNormalContainerAction(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		const FString& Reason);
	/**
	 * P10's sole dual-owner durable commit. The input is a P1-validated composite
	 * snapshot made from the matched P6 carry graph plus this exact P9 target;
	 * it is split and saved as one Owner-document replacement or not at all.
	 */
	static bool CommitAcceptedMatchedRunNormalContainerTransfer(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		int32 ExpectedP6SnapshotRevision,
		int32 ExpectedNormalContainerRevision,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CompositeSnapshot,
		FString* OutError = nullptr);
	/** Read-only P5 entry lock probe; it never creates or migrates a sidecar. */
	static bool HasActiveRunInventorySession(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		FString* OutError = nullptr);
	bool TryGetActiveRunInventorySession(
		FCodeBRunInventorySession& OutSession,
		FString* OutError = nullptr) const;
	/**
	 * Read-only P7 facade. It loads only a pre-existing, committed P6 session
	 * for this Store Owner and exact active RunId; it never creates, migrates,
	 * rebinds, or recovers a sidecar.
	 */
	bool OpenMatchedActiveRunInventorySession(
		const FGuid& ExpectedRunInstanceId,
		FCodeBRunInventorySession& OutSession,
		FString* OutError = nullptr);
	/**
	 * P7's sole durable consumer write. The accepted P1 snapshot replaces only
	 * the active P6 session snapshot; the P5 snapshot and P6 receipt/rebind
	 * semantics remain untouched.
	 */
	bool CommitAcceptedActiveRunInventorySnapshot(
		const FGuid& ExpectedRunInstanceId,
		const demo_map_code_b::FCodeBSnapshot& Snapshot,
		FString* OutError = nullptr);
	/** P15's only consuming writer. It edits the exact P6 bound BaseQuick item and receipt ledger atomically. */
	bool UseMatchedActiveRunBoundQuickSlot(
		const FGuid& ExpectedRunInstanceId,
		int32 SlotIndex,
		FCodeBQuickUseReceipt& OutReceipt,
		FString* OutError = nullptr);
	/** Read-only delivery query. It cannot consume, bind, repair, or acknowledge. */
	bool TryGetMatchedActiveRunPendingQuickUseReceipts(
		const FGuid& ExpectedRunInstanceId,
		TArray<FCodeBQuickUseReceipt>& OutReceipts,
		FString* OutError = nullptr);
	/** Code A acknowledges only an exact saved P15 receipt after idempotent application. */
	bool AcknowledgeMatchedActiveRunQuickUseReceipt(
		const FGuid& ExpectedRunInstanceId,
		const FGuid& ReceiptId,
		FString* OutError = nullptr);
	/** Read-only P13 projection for one exact committed P6 session. */
	bool TryGetMatchedActiveRunHotbarProjection(
		const FGuid& ExpectedRunInstanceId,
		FCodeBHotbarProjection& OutProjection,
		FString* OutError = nullptr) const;
	/** P13's only active-Run binding writers; exact OwnerId + RunInstanceId is required. */
	bool BindMatchedActiveRunHotbarSlot(
		const FGuid& ExpectedRunInstanceId,
		const FGuid& ItemId,
		int32 SlotIndex,
		FCodeBHotbarProjection& OutProjection,
		FString* OutError = nullptr);
	bool UnbindMatchedActiveRunHotbarSlot(
		const FGuid& ExpectedRunInstanceId,
		int32 SlotIndex,
		FCodeBHotbarProjection& OutProjection,
		FString* OutError = nullptr);
	/** Read-only P14 actor/lifecycle bridge. It never creates records or actors. */
	bool TryGetMatchedActiveRunWorldDropProjections(
		const FGuid& ExpectedRunInstanceId,
		TArray<FCodeBWorldDropProjection>& OutProjections,
		FString* OutError = nullptr) const;
	/** P14/P19/P26's only ground-drop writer. Exact source address/revision and Code A floor transform are verified before one durable candidate. */
	bool DropMatchedActiveRunWorldDropItem(
		const FGuid& ExpectedRunInstanceId,
		const FGuid& ItemId,
		const FGuid& ExpectedSourceContainerId,
		int32 ExpectedSourceSlot,
		int32 ExpectedP6SnapshotRevision,
		int32 RequestedSplitQuantity,
		const FGuid& ExpectedActiveChildContainerId,
		uint32 ExpectedActiveChildOpenGeneration,
		FName MapRoute,
		const FTransform& FloorTransform,
		FCodeBWorldDropProjection& OutProjection,
		FString* OutError = nullptr);
	/**
	 * P43/P44/P45's sole corpse-to-world writer. It revalidates exactly one Revealed
	 * ordinary P12 simple-stack, P21 standard-equipment root, or P20 complete graph, creates one
	 * derived P31 record, partitions the accepted P1 composite back into P11/P6,
	 * and saves one Owner replacement.
	 */
	static bool DropMatchedRunBodyContainerWorldDropItem(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& BodyTargetId,
		FName DefinitionId,
		int32 ExpectedP6SnapshotRevision,
		int32 ExpectedBodyContainerRevision,
		const demo_map_code_b::FCodeBP43BodySimpleStackGroundDropProof& SourceProof,
		const demo_map_code_b::FCodeBP38BodyEquipmentTransferProof& EquipmentSourceProof,
		const demo_map_code_b::FCodeBP42BodySpatialGraphEquipmentTransferProof& SpatialSourceProof,
		FName MapRoute,
		const FTransform& FloorTransform,
		FCodeBBodyContainerProjection& OutBodyProjection,
		FCodeBWorldDropProjection& OutWorldProjection,
		FString* OutError = nullptr);
	/**
	 * P49's sole BasicCache-to-world writer. It revalidates one exact Revealed
	 * ordinary P10 simple stack, executes one P1 whole-root Move into one new P31
	 * record, partitions P9/P6, and saves one Owner replacement.
	 */
	static bool DropMatchedRunNormalContainerWorldDropItem(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		int32 ExpectedP6SnapshotRevision,
		int32 ExpectedNormalContainerRevision,
		const demo_map_code_b::FCodeBP49NormalContainerSimpleStackGroundDropProof& SourceProof,
		FName MapRoute,
		const FTransform& FloorTransform,
		FCodeBNormalContainerProjection& OutNormalProjection,
		FCodeBWorldDropProjection& OutWorldProjection,
		FString* OutError = nullptr);
	/**
	 * P50's sole BasicCache spatial-graph-to-world writer. It revalidates one
	 * exact revealed P18 parent plus its unique empty child, moves the whole
	 * graph to a new P31 record, and saves one Owner replacement.
	 */
	static bool DropMatchedRunNormalContainerSpatialWorldDropItem(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& SearchTargetId,
		FName DefinitionId,
		int32 ExpectedP6SnapshotRevision,
		int32 ExpectedNormalContainerRevision,
		const demo_map_code_b::FCodeBP48NormalContainerSpatialGraphEquipmentTransferProof& SourceProof,
		FName MapRoute,
		const FTransform& FloorTransform,
		FCodeBNormalContainerProjection& OutNormalProjection,
		FCodeBWorldDropProjection& OutWorldProjection,
		FString* OutError = nullptr);
	/** P14/P19/P26/P27/P28/P29/P30/P32/P34/P36/P37's only opened-WorldDrop writer; the accepted P2 command is transient proof context, never durable state. */
	static bool CommitAcceptedMatchedRunWorldDropPickup(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		const FGuid& InRunInstanceId,
		const FGuid& WorldDropId,
		int32 ExpectedWorldDropOrdinal,
		int32 ExpectedWorldDropRecordRevision,
		int32 ExpectedP6SnapshotRevision,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
		FString* OutError = nullptr);

	bool IsOpen() const { return bHasRecord; }
	int32 GetPersistentRevision() const { return Record.PersistentRevision; }
	const FCodeBOutOfRaidInventoryRecord& GetRecord() const { return Record; }
	FString GetPrimaryPath() const;
	FString GetBackupPath() const;
	FString GetTempPath() const;

#if WITH_DEV_AUTOMATION_TESTS
	/** Simulates process loss after the verified Prepared receipt and before finalization. */
	void SetInterruptAfterPreparedReceiptForAutomation(bool bEnabled)
	{
		bInterruptAfterPreparedReceiptForAutomation = bEnabled;
	}
	/** P6-only interruption seam; it leaves a verified Run Prepared receipt on disk. */
	void SetInterruptAfterRunPreparedReceiptForAutomation(bool bEnabled)
	{
		bInterruptAfterRunPreparedReceiptForAutomation = bEnabled;
	}
	/** Applies the P6 receipt interruption seam to the production observer's
	 * short-lived store instance.  It exists solely for lifecycle-level tests. */
	static void SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(bool bEnabled);
	/** Read-only lifecycle assertion seam; it cannot create, migrate, or write a sidecar. */
	static bool TryReadRecordForLifecycleAutomation(
		const FString& InStorageRoot,
		const FGuid& InOwnerId,
		FCodeBOutOfRaidInventoryRecord& OutRecord,
		FString* OutError = nullptr);
	FCodeBRunInventoryBridgeResult NotifySuccessfulRunForAutomation(const FGuid& RunInstanceId)
	{
		return BridgeSuccessfulRun(RunInstanceId);
	}
#endif

private:
	bool LoadRecord(FCodeBOutOfRaidInventoryRecord& OutRecord, FString& OutError);
	bool LoadRecordAtPath(
		const FString& Path,
		FCodeBOutOfRaidInventoryRecord& OutRecord,
		FString& OutError) const;
	bool SaveRecord(const FCodeBOutOfRaidInventoryRecord& InRecord, FString& OutError) const;
	bool BuildInitialRecord(
		const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot,
		FCodeBOutOfRaidInventoryRecord& OutRecord,
		FString& OutError) const;
	bool ValidateRecord(const FCodeBOutOfRaidInventoryRecord& InRecord, FString& OutError) const;
	bool FinalizePreparedReceipt(FString& OutError);
	FCodeBRunInventoryBridgeResult BridgeSuccessfulRun(
		const FGuid& RunInstanceId,
		const FCodeBRunInventoryRecoveryContext& RecoveryContext = FCodeBRunInventoryRecoveryContext());
	bool RebindPreparedRunInventoryReceipt(
		const FGuid& NewRunInstanceId,
		const FCodeBRunInventoryRecoveryContext& RecoveryContext,
		FCodeBRunInventoryBridgeResult& OutResult);
	bool FinalizePreparedRunInventoryReceipt(FCodeBRunInventoryBridgeResult& OutResult);
	FCodeBRunInventoryTerminalResult FinalizeCommittedRunTerminal(
		const FGuid& RunInstanceId,
		ECodeBRunInventoryTerminalState TerminalState);

	FString StorageRoot;
	FGuid OwnerId;
	FCodeBOutOfRaidInventoryRecord Record;
	bool bHasRecord = false;
	bool bLastLoadUsedBackup = false;
#if WITH_DEV_AUTOMATION_TESTS
	bool bInterruptAfterPreparedReceiptForAutomation = false;
	bool bInterruptAfterRunPreparedReceiptForAutomation = false;
#endif
};
