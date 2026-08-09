#pragma once

#include "CoreMinimal.h"
#include "demo_mapRewardAffixTypes.h"
#include "demo_mapItemTypes.h"

enum class Edemo_mapRuntimeContainerKind : uint8
{
	Chest,
	Corpse
};

enum class Edemo_mapRuntimeContainerState : uint8
{
	Closed,
	Opening,
	Opened
};

enum class Edemo_mapRuntimeContainerSection : uint8
{
	Chest,
	Equipment,
	Backpack,
	Body
};

enum class Edemo_mapRuntimeContainerEntryState : uint8
{
	Hidden,
	Searching,
	Identified,
	Taken
};

enum class Edemo_mapRuntimeContainerActionKind : uint8
{
	None,
	BeginOpen,
	CancelOpen,
	BeginSearch,
	CancelSearch,
	Take,
	Close
};

enum class Edemo_mapRuntimeContainerResultCode : uint8
{
	Success,
	NoOp,
	NotInitialized,
	InvalidRun,
	InvalidContainer,
	StaleRevision,
	InvalidState,
	InvalidEntry,
	ConcurrentAction,
	PlayerUnavailable,
	OutOfRange,
	ItemAuthorityRejected,
	CleanupRejected
};

struct Fdemo_mapRuntimeContainerSeedEntry
{
	Edemo_mapRuntimeContainerSection Section = Edemo_mapRuntimeContainerSection::Chest;
	int32 SlotIndex = INDEX_NONE;
	FName DefinitionId = NAME_None;
	int32 StackCount = 0;
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

struct Fdemo_mapRuntimeContainerResolvedSeedEntry
{
	Edemo_mapRuntimeContainerSection Section = Edemo_mapRuntimeContainerSection::Chest;
	int32 SlotIndex = INDEX_NONE;
	FGuid ItemInstanceId;
	FName DefinitionId = NAME_None;
	int32 StackCount = 0;
	float SearchDurationSeconds = 0.0f;
};

struct Fdemo_mapRuntimeContainerEntrySnapshot
{
	FGuid EntryId;
	Edemo_mapRuntimeContainerSection Section = Edemo_mapRuntimeContainerSection::Chest;
	int32 SlotIndex = INDEX_NONE;
	Edemo_mapRuntimeContainerEntryState State = Edemo_mapRuntimeContainerEntryState::Hidden;
	float Progress01 = 0.0f;
	bool bCanSearch = false;
	bool bCanTake = false;
	bool bOccupied = false;

	// These identity fields are populated only for Identified entries.
	FGuid ItemInstanceId;
	FName DefinitionId = NAME_None;
	FText DisplayName;
	FName CategoryId = NAME_None;
	int32 Level = 0;
	int32 StackCount = 0;
	int64 UnitSellPrice = 0;
	int64 EffectiveStackSellValue = 0;
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

struct Fdemo_mapRuntimeContainerSectionSnapshot
{
	Edemo_mapRuntimeContainerSection Section = Edemo_mapRuntimeContainerSection::Chest;
	int32 Capacity = 0;
	TArray<Fdemo_mapRuntimeContainerEntrySnapshot> OrderedOccupiedEntries;
};

struct Fdemo_mapRuntimeContainerSnapshot
{
	FGuid ContainerId;
	FGuid OwningRunId;
	Edemo_mapRuntimeContainerKind Kind = Edemo_mapRuntimeContainerKind::Chest;
	Edemo_mapRuntimeContainerState State = Edemo_mapRuntimeContainerState::Closed;
	Edemo_mapRuntimeContainerActionKind ActiveAction = Edemo_mapRuntimeContainerActionKind::None;
	int32 Revision = 0;
	float ActionProgress01 = 0.0f;
	int32 InventoryUsedSlots = 0;
	int32 InventoryCapacity = 0;
	bool bPlayerDepositAllowed = true;
	bool bEmpty = false;
	FString SourceDisplayLabel;
	TArray<Fdemo_mapRuntimeContainerSectionSnapshot> Sections;
	FString Diagnostic;
};

struct Fdemo_mapRuntimeContainerIntent
{
	FGuid ExpectedRunId;
	FGuid ContainerId;
	int32 ExpectedRevision = 0;
	FGuid EntryId;
	Edemo_mapRuntimeContainerActionKind Action = Edemo_mapRuntimeContainerActionKind::None;
};

/**
 * P3's UI-independent bridge between one active Runtime Container and the
 * player's existing ItemAuthority.  The Container Revision and ItemAuthority
 * Revision are both optimistic-concurrency tokens; stale drags never mutate
 * either side.  A Container source uses SourceEntryId and player Target fields;
 * a player source uses player Source fields and Container Target fields.
 */
struct Fdemo_mapSearchContainerDropIntent
{
	FGuid ExpectedRunId;
	FGuid ContainerId;
	int32 ExpectedContainerRevision = INDEX_NONE;
	int32 ExpectedAuthorityRevision = INDEX_NONE;
	bool bSourceIsContainer = false;
	FGuid ExpectedSourceItemInstanceId;
	FGuid SourceEntryId;
	FGuid TargetEntryId;
	Edemo_mapRuntimeContainerSection TargetSection =
		Edemo_mapRuntimeContainerSection::Chest;
	int32 TargetContainerSlotIndex = INDEX_NONE;
	Edemo_mapPlayerItemArea SourcePlayerArea =
		Edemo_mapPlayerItemArea::Invalid;
	int32 SourcePlayerSlotIndex = INDEX_NONE;
	FName SourcePlayerEquipmentSlotId = NAME_None;
	Edemo_mapPlayerItemArea TargetPlayerArea =
		Edemo_mapPlayerItemArea::Invalid;
	int32 TargetPlayerSlotIndex = INDEX_NONE;
	FName TargetPlayerEquipmentSlotId = NAME_None;
};

/** One committed P3 drag has one atomic outcome across Container and player. */
struct Fdemo_mapSearchContainerDropResult
{
	Edemo_mapPlayerItemDropKind Kind = Edemo_mapPlayerItemDropKind::Reject;
	bool bCommitted = false;
	int32 CommittedContainerRevision = INDEX_NONE;
	int32 CommittedAuthorityRevision = INDEX_NONE;
	FGuid SourceItemInstanceId;
	FGuid TargetItemInstanceId;
	Fdemo_mapItemOperationResult Operation;
	FString Diagnostic;

	bool IsSuccess() const { return bCommitted; }
};

struct Fdemo_mapRuntimeContainerResult
{
	Edemo_mapRuntimeContainerResultCode Code = Edemo_mapRuntimeContainerResultCode::Success;
	bool bSuccess = true;
	FGuid ContainerId;
	FGuid EntryId;
	FGuid ItemInstanceId;
	int32 RevisionBefore = 0;
	int32 RevisionAfter = 0;
	FString Diagnostic;

	static Fdemo_mapRuntimeContainerResult Success(
		FGuid InContainerId,
		int32 InRevisionBefore,
		int32 InRevisionAfter,
		FGuid InEntryId = FGuid(),
		FGuid InItemInstanceId = FGuid());
	static Fdemo_mapRuntimeContainerResult Failure(
		Edemo_mapRuntimeContainerResultCode InCode,
		const FString& InDiagnostic,
		FGuid InContainerId = FGuid(),
		int32 InRevision = 0,
		FGuid InEntryId = FGuid());
};

/** P4.0 immutable prototype values and deterministic seed provider. */
struct Fdemo_mapSearchContainerPrototypeConfig
{
	static constexpr float ChestOpenSeconds = 1.00f;
	static constexpr float CorpseOpenSeconds = 1.00f;
	static constexpr float ChestEntrySearchSeconds = 0.75f;
	static constexpr float CorpseEquipmentSearchSeconds = 0.00f;
	static constexpr float CorpseBackpackEntrySearchSeconds = 1.00f;
	static constexpr float CorpseBodyEntrySearchSeconds = 1.50f;
	static constexpr float FallbackInteractionRangeUU = 300.00f;
	static constexpr int32 ChestPrototypeCapacity = 6;
	static constexpr int32 CorpseEquipmentCapacity = 5;
	static constexpr int32 CorpseBaseQuickItemCapacity = 6;
	static constexpr int32 CorpseMaximumSpatialStorageCapacity = 36;
	static constexpr int32 CorpseRewardBackpackCapacity = 1;
	static constexpr int32 CorpseRuntimeBackpackCapacity =
		CorpseBaseQuickItemCapacity
		+ CorpseMaximumSpatialStorageCapacity;
	static constexpr int32 CorpseBodyCapacity = 2;
	static constexpr int32 MaxConcurrentContainerAction = 1;

	static float GetOpenSeconds(Edemo_mapRuntimeContainerKind Kind);
	static float GetSearchSeconds(
		Edemo_mapRuntimeContainerKind Kind,
		Edemo_mapRuntimeContainerSection Section);
	static int32 GetSectionCapacity(
		Edemo_mapRuntimeContainerKind Kind,
		Edemo_mapRuntimeContainerSection Section);
	static TArray<Edemo_mapRuntimeContainerSection> GetSectionOrder(
		Edemo_mapRuntimeContainerKind Kind);
	static TArray<Fdemo_mapRuntimeContainerSeedEntry> BuildChestSeed(int32 ChestIndex);
	static TArray<Fdemo_mapRuntimeContainerSeedEntry> BuildPrototypeCorpseSeed();
};
