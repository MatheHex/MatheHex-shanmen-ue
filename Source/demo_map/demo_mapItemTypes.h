#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffixTypes.h"
#include "demo_mapAttributeTypes.h"
#include "demo_mapRewardEventTypes.h"
#include "demo_mapItemTypes.generated.h"

UENUM()
enum class Edemo_mapItemOwnershipState : uint8
{
	World,
	Container,
	Inventory,
	Equipped,
	SessionStash,
	Destroyed
};

UENUM()
enum class Edemo_mapItemResultCode : uint8
{
	Success,
	UnknownDefinition,
	InvalidQuantity,
	InstanceNotFound,
	InvalidOwnership,
	InventoryFull,
	InvalidContainer,
	InvalidSlot,
	IncompatibleSlot,
	NotEquipable,
	SlotOccupied,
	ModifierApplicationFailed,
	InvariantViolation,
	AlreadyDestroyed,
	InternalRollbackFailed,
	NoInteractionFocus,
	InteractionOutOfRange,
	InteractionBlocked,
	InvalidWorldBinding,
	WorldActorSpawnFailed,
	UnsafeDropLocation,
	AlreadyClaimed,
	AlreadyOpened,
	LootSpawnFailed,
	UIInvalidSelection,
	RunNotActive,
	RunAlreadyActive,
	SettlementInProgress,
	SettlementAlreadyCompleted,
	InvalidRunId,
	InvalidSettlementReason,
	LootSourceInvalid,
	LootSourceProcessing,
	LootSourceCompleted,
	LootTableNotFound,
	ForbiddenLootSource,
	SettlementRollbackFailed,
	SessionStashReadOnly
};

/** P2 player-owned item areas.  World and container loot intentionally do not
 * participate in this contract. */
enum class Edemo_mapPlayerItemArea : uint8
{
	Invalid,
	Warehouse,
	Equipment,
	BaseQuickItems,
	SpatialStorage
};

/** The only committed outcomes a UI drag may present. */
enum class Edemo_mapPlayerItemDropKind : uint8
{
	Move,
	Swap,
	Merge,
	Equip,
	Unequip,
	Reject
};

/**
 * UI-independent P2 input.  The caller provides both its source identity and
 * the authority revision it rendered; an authority rejects stale input before
 * it mutates anything.  Equipment uses EquipmentSlotId, all other areas use
 * their zero-based visual slot index.
 */
struct Fdemo_mapPlayerItemDropIntent
{
	int32 ExpectedAuthorityRevision = INDEX_NONE;
	FGuid ExpectedSourceItemInstanceId;
	Edemo_mapPlayerItemArea SourceArea = Edemo_mapPlayerItemArea::Invalid;
	int32 SourceSlotIndex = INDEX_NONE;
	FName SourceEquipmentSlotId = NAME_None;
	Edemo_mapPlayerItemArea TargetArea = Edemo_mapPlayerItemArea::Invalid;
	int32 TargetSlotIndex = INDEX_NONE;
	FName TargetEquipmentSlotId = NAME_None;
};

UENUM()
enum class Edemo_mapRunState : uint8
{
	Inactive,
	Active,
	Settling,
	Settled
};

UENUM()
enum class Edemo_mapRunEndReason : uint8
{
	None,
	Death,
	Extraction,
	Abandon,
	RecoveredAbandon,
	/** Startup failed before the player received control; this is never a player abandon. */
	ActivationFailure
};

UENUM()
enum class Edemo_mapEnemyLootArchetype : uint8
{
	Melee,
	Ranged,
	Heavy
};

UENUM()
enum class Edemo_mapLootSourceState : uint8
{
	Processing,
	Completed
};

/**
 * Immutable, data-only effect metadata. P1.0 publishes values for later systems
 * without adding any gameplay consumption path.
 */
USTRUCT()
struct Fdemo_mapItemEffectParameter
{
	GENERATED_BODY()

	FName ParameterId = NAME_None;
	double Value = 0.0;
};

/**
 * Stable product semantics that may project into the 0.0.10 item authority.
 * This is an extensible typed set rather than a collection of independent
 * booleans; category, display name, and DefinitionId never grant gameplay
 * capabilities by inference.
 */
UENUM()
enum class Edemo_mapItemGameplaySemantic : uint8
{
	None,
	ThrownWeapon,
	WeaponGuard,
	/** Treats only the canonical minor Meridian Shock condition. */
	MeridianShockTreatment,
	/** Passively intercepts one otherwise-lethal impact while charges remain. */
	LethalInterception,
	/** May be the exact equipped source item for the canonical Sword Qi action. */
	SwordQiSource
};

USTRUCT()
struct Fdemo_mapItemDefinition
{
	GENERATED_BODY()

	FName DefinitionId = NAME_None;
	FText DisplayName;
	FString WorldLabelName;
	FName CategoryId = NAME_None;
	int32 Level = 0;
	int32 MaxStackSize = 1;
	/** Explicit 0.0.10 authority resource; zero means durability is unsupported. */
	int32 MaxDurability = 0;
	/** Explicit 0.0.10 authority resource; zero means charges are unsupported. */
	int32 MaxCharges = 0;
	int32 GridWidth = 1;
	int32 GridHeight = 1;
	FName EquipmentSlotId = NAME_None;
	TArray<FName> CompatibleSlotIds;
	TArray<Fdemo_mapModifierSpec> Modifiers;
	TArray<Fdemo_mapItemEffectParameter> EffectParameters;
	/** Immutable product semantics consumed only by explicit authority adapters. */
	TArray<Edemo_mapItemGameplaySemantic> GameplaySemantics;
	bool bPurchasable = false;
	bool bSellable = false;
	int64 BuyPrice = 0;
	int64 SellPrice = 0;
	int32 PrototypeValue = 0;
	FName WorldPresentationId = NAME_None;
	/** P73 content manifest identity. Runtime instances retain DefinitionId only. */
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
	/** Canonical content rule; WorldDrop creation fails closed when false. */
	bool bWorldDropEligible = false;
	/** Canonical content rule consumed by the hotbar projection/validator. */
	bool bHotbarEligible = false;

	bool HasGameplaySemantic(Edemo_mapItemGameplaySemantic Semantic) const
	{
		return Semantic != Edemo_mapItemGameplaySemantic::None
			&& GameplaySemantics.Contains(Semantic);
	}
};

USTRUCT()
struct Fdemo_mapItemInstance
{
	GENERATED_BODY()

	FGuid InstanceId;
	FName DefinitionId = NAME_None;
	int32 Quantity = 0;
	Edemo_mapItemOwnershipState OwnershipState = Edemo_mapItemOwnershipState::Destroyed;
	FName OwnerId = NAME_None;
	FName ContainerId = NAME_None;
	FName EquippedSlotId = NAME_None;
	FGuid OriginRunId;
	Edemo_mapRewardEventKind RewardEventKind =
		Edemo_mapRewardEventKind::None;
	FGuid RewardEventId;
	int32 RewardValueMultiplierBps =
		Fdemo_mapRewardEventRules::NormalMultiplierBps;
	FName RewardSourceRoleId = NAME_None;
	FGuid RareRewardEventId;
	FName RareRewardPolicyId = NAME_None;
	FName RareRewardTierId = NAME_None;
	int64 RareRewardBonusValue = 0;
	Fdemo_mapRewardAffixSet AffixSet;
};

/** One deterministic 1x1 view cell. It never owns or copies an item instance. */
struct Fdemo_mapGridSlotView
{
	int32 SlotIndex = INDEX_NONE;
	FGuid ItemInstanceId;

	bool IsOccupied() const { return ItemInstanceId.IsValid(); }
	bool operator==(const Fdemo_mapGridSlotView& Other) const
	{
		return SlotIndex == Other.SlotIndex && ItemInstanceId == Other.ItemInstanceId;
	}
};

/** Rebuildable ordered grid projection; no row, column, or index is persisted. */
struct Fdemo_mapGridContainerSnapshot
{
	int32 Capacity = 0;
	int32 UsedSlots = 0;
	TArray<Fdemo_mapGridSlotView> OrderedSlots;
	bool bValid = false;
	FString Diagnostic;

	bool operator==(const Fdemo_mapGridContainerSnapshot& Other) const
	{
		return Capacity == Other.Capacity
			&& UsedSlots == Other.UsedSlots
			&& OrderedSlots == Other.OrderedSlots
			&& bValid == Other.bValid
			&& Diagnostic == Other.Diagnostic;
	}
};

/** Shared result for definition-backed capacity resolution and atomic preflight. */
struct Fdemo_mapInventoryCapacityResult
{
	bool bSuccess = false;
	bool bFits = false;
	int32 UsedSlots = 0;
	int32 Capacity = 0;
	/** Fixed six is followed by a ring quick area and an independent bag. */
	int32 RingQuickCapacity = 0;
	int32 SpatialBagCapacity = 0;
	FName BackpackDefinitionId = NAME_None;
	/** The equipped spatial-ring definition, never the ordinary accessory. */
	FName SpatialRingDefinitionId = NAME_None;
	FString Diagnostic;
};

/** Definition-backed space-item storage, separate from the fixed six quick cells. */
struct Fdemo_mapSpatialStorageCapacityResult
{
	bool bSuccess = false;
	int32 Capacity = 0;
	FName SpatialItemDefinitionId = NAME_None;
	FString Diagnostic;
};

/** Definition-backed quick cells granted by the equipped spatial ring. */
struct Fdemo_mapSpatialRingCapacityResult
{
	bool bSuccess = false;
	int32 Capacity = 0;
	FName SpatialRingDefinitionId = NAME_None;
	FString Diagnostic;
};

/** Fixed 1..9 pure-value bindings. Invalid GUID means an empty slot. */
struct Fdemo_mapHotbarBindingSnapshot
{
	static constexpr int32 SlotCount = 9;

	Fdemo_mapHotbarBindingSnapshot()
	{
		SlotBindings.Init(FGuid(), SlotCount);
	}

	TArray<FGuid> SlotBindings;

	bool operator==(const Fdemo_mapHotbarBindingSnapshot& Other) const
	{
		return SlotBindings == Other.SlotBindings;
	}
};

/** Pure projection and hotbar validation rules shared by Runtime and Automation. */
struct Fdemo_mapItemViewRules
{
	static Fdemo_mapGridContainerSnapshot BuildGridFromOccupiedOrder(
		const TArray<FGuid>& OrderedOccupiedItemIds,
		int32 Capacity);
	static bool IsHotbarBindable(
		const Fdemo_mapItemInstance& Instance,
		const Fdemo_mapItemDefinition& Definition);
};

USTRUCT()
struct Fdemo_mapSettlementItemRow
{
	GENERATED_BODY()

	FGuid InstanceId;
	FName DefinitionId = NAME_None;
	int32 Quantity = 0;
	Edemo_mapItemOwnershipState SourceOwnership = Edemo_mapItemOwnershipState::Destroyed;
	Edemo_mapItemOwnershipState FinalOwnership = Edemo_mapItemOwnershipState::Destroyed;
	int32 UnitValue = 0;
	int32 TotalValue = 0;
};

/** Pure-value handoff from the first Runtime terminal event; never persistent authority. */
struct Fdemo_mapRuntimeSettlementItem
{
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	int32 StackCount = 0;
	FGuid OriginRunId;
	Edemo_mapRewardEventKind RewardEventKind =
		Edemo_mapRewardEventKind::None;
	FGuid RewardEventId;
	int32 RewardValueMultiplierBps =
		Fdemo_mapRewardEventRules::NormalMultiplierBps;
	FName RewardSourceRoleId = NAME_None;
	FGuid RareRewardEventId;
	FName RareRewardPolicyId = NAME_None;
	FName RareRewardTierId = NAME_None;
	int64 RareRewardBonusValue = 0;
	Fdemo_mapRewardAffixSet AffixSet;

	bool operator==(const Fdemo_mapRuntimeSettlementItem& Other) const
	{
		return ItemInstanceId == Other.ItemInstanceId
			&& ItemDefinitionId == Other.ItemDefinitionId
			&& StackCount == Other.StackCount
			&& OriginRunId == Other.OriginRunId
			&& RewardEventKind == Other.RewardEventKind
			&& RewardEventId == Other.RewardEventId
			&& RewardValueMultiplierBps
				== Other.RewardValueMultiplierBps
			&& RewardSourceRoleId == Other.RewardSourceRoleId
			&& RareRewardEventId == Other.RareRewardEventId
			&& RareRewardPolicyId == Other.RareRewardPolicyId
			&& RareRewardTierId == Other.RareRewardTierId
			&& RareRewardBonusValue == Other.RareRewardBonusValue
			&& AffixSet == Other.AffixSet;
	}
};

struct Fdemo_mapRuntimeSettlementSnapshot
{
	FGuid ActiveRunId;
	Edemo_mapRunEndReason CommittedEndReason = Edemo_mapRunEndReason::None;
	TArray<Fdemo_mapRuntimeSettlementItem> OrderedSecuredItems;
	bool bValid = false;

	bool operator==(const Fdemo_mapRuntimeSettlementSnapshot& Other) const
	{
		return ActiveRunId == Other.ActiveRunId
			&& CommittedEndReason == Other.CommittedEndReason
			&& OrderedSecuredItems == Other.OrderedSecuredItems
			&& bValid == Other.bValid;
	}
};

USTRUCT()
struct Fdemo_mapSettlementSummary
{
	GENERATED_BODY()

	FGuid RunId;
	Edemo_mapRunEndReason Reason = Edemo_mapRunEndReason::None;
	TArray<Fdemo_mapSettlementItemRow> Rows;
	int32 SecuredItemCount = 0;
	int32 SecuredValue = 0;
	int32 LostItemCount = 0;
	int32 LostValue = 0;
	int32 StashItemCountAfter = 0;
	int32 StashValueAfter = 0;
	int64 RiskBefore = 0;
	int64 RiskTransferred = 0;
	int64 RiskLost = 0;
	int64 PersistentBefore = 0;
	int64 PersistentAfter = 0;
	TArray<FGuid> ClearedPreparationItemIds;
	Fdemo_mapRuntimeSettlementSnapshot RuntimeSnapshot;
	bool bValid = false;
};

USTRUCT()
struct Fdemo_mapItemOperationResult
{
	GENERATED_BODY()

	Edemo_mapItemResultCode Code = Edemo_mapItemResultCode::Success;
	bool bSuccess = true;
	FGuid RelatedInstanceId;
	FName RelatedDefinitionId = NAME_None;
	FName RelatedSlotId = NAME_None;
	FName RelatedActorId = NAME_None;
	FString Diagnostic;

	static Fdemo_mapItemOperationResult Success(FGuid InstanceId = FGuid(), FName DefinitionId = NAME_None, FName SlotId = NAME_None, FName ActorId = NAME_None);
	static Fdemo_mapItemOperationResult Failure(Edemo_mapItemResultCode ResultCode, const FString& Message, FGuid InstanceId = FGuid(), FName DefinitionId = NAME_None, FName SlotId = NAME_None, FName ActorId = NAME_None);
};

/** Atomic P2 response.  It never asks callers to infer a partial outcome. */
struct Fdemo_mapPlayerItemDropResult
{
	Edemo_mapPlayerItemDropKind Kind = Edemo_mapPlayerItemDropKind::Reject;
	bool bCommitted = false;
	int32 CommittedAuthorityRevision = INDEX_NONE;
	FGuid SourceItemInstanceId;
	FGuid TargetItemInstanceId;
	Fdemo_mapItemOperationResult Operation;
	FString Diagnostic;

	bool IsSuccess() const { return bCommitted; }
};
