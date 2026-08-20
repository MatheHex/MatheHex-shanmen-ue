#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemUseTypes.h"
#include "demo_mapProfileRunTypes.h"
#include "demo_mapSearchContainerTypes.h"
#include "demo_mapItemSubsystem.generated.h"

class Udemo_mapAttributeComponent;
class Udemo_mapPlayerHealthComponent;
class Ademo_mapWorldItem;
class Fdemo_mapRuntimeContainerAuthority;

USTRUCT()
struct Fdemo_mapWorldSpawnRequest
{
	GENERATED_BODY()

	FName DefinitionId = NAME_None;
	int32 Quantity = 0;
	FVector DesiredLocation = FVector::ZeroVector;
	FName SourceId = NAME_None;
};

struct Fdemo_mapContainerMaterializationRequest
{
	FName DefinitionId = NAME_None;
	int32 Quantity = 0;
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

/** GameInstance-scoped item authority and idempotent equipment-to-attribute bridge. */
UCLASS()
class Udemo_mapItemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	Fdemo_mapItemOperationResult AddDefinition(FName DefinitionId, int32 Quantity, TArray<FGuid>* OutAffectedInstances = nullptr);
	Fdemo_mapItemOperationResult Equip(FGuid InstanceId, FName SlotId);
	Fdemo_mapItemOperationResult Unequip(FName SlotId);
	Fdemo_mapItemOperationResult Destroy(FGuid InstanceId);
	Fdemo_mapItemOperationResult MoveInventorySlot(
		int32 SourceSlotIndex,
		int32 TargetSlotIndex);
	/** P2 Runtime drag/drop facade; also reconciles Base-6-only hotbar bindings. */
	Fdemo_mapPlayerItemDropResult ExecutePlayerItemDrop(
		const Fdemo_mapPlayerItemDropIntent& Intent);
	/** P3: one atomic drag between the player authority and an opened Container. */
	Fdemo_mapSearchContainerDropResult ExecuteSearchContainerDrop(
		const Fdemo_mapSearchContainerDropIntent& Intent,
		Fdemo_mapRuntimeContainerAuthority& Container);
	Fdemo_mapItemOperationResult BindHotbarSlot(int32 ExternalSlotNumber, FGuid InstanceId);
	Fdemo_mapItemOperationResult UnbindHotbarSlot(int32 ExternalSlotNumber);
	int32 RefreshHotbarBindings();
	void ClearHotbarBindings();
	const Fdemo_mapHotbarBindingSnapshot& GetHotbarBindingSnapshot() const { return HotbarBindings; }
	Fdemo_mapItemUseResult UseHotbarSlot(
		const Fdemo_mapItemUseIntent& Intent,
		bool bInputAllowed);
	/** Uses a legal inventory consumable through the same atomic Runtime transaction. */
	Fdemo_mapItemUseResult UseInventoryItem(
		FGuid InstanceId,
		bool bUIInputAllowed);
	Fdemo_mapItemUseCooldownSnapshot GetItemUseCooldownSnapshot() const;
	void ClearItemUseCooldown();
	void BeginWorld(UWorld* World);
	void TeardownWorld(UWorld* World);
	Fdemo_mapItemOperationResult CreateWorldItem(UWorld* World, FName DefinitionId, int32 Quantity, const FVector& DesiredLocation, Ademo_mapWorldItem*& OutActor, FName SourceId = NAME_None);
	Fdemo_mapItemOperationResult CreateWorldItemsAtomically(UWorld* World, const TArray<Fdemo_mapWorldSpawnRequest>& Requests, TArray<Ademo_mapWorldItem*>& OutActors);
	Fdemo_mapItemOperationResult CreateContainerItem(
		FGuid ContainerId,
		FName DefinitionId,
		int32 Quantity,
		FGuid& OutInstanceId);
	Fdemo_mapItemOperationResult MaterializeContainerItemsAtomically(
		FGuid ContainerId,
		const TArray<Fdemo_mapContainerMaterializationRequest>& Requests,
		TFunctionRef<bool(const TArray<FGuid>& InstanceIds, FString& OutDiagnostic)> FinalizeMaterialization,
		TArray<FGuid>& OutInstanceIds
#if WITH_DEV_AUTOMATION_TESTS
		, int32 FailureAfterMutation = INDEX_NONE
#endif
	);
	/** Projects a previously accepted Profile source using its durable GUIDs. */
	Fdemo_mapItemOperationResult MaterializeCommittedContainerItemsAtomically(
		FGuid ContainerId,
		const TArray<Fdemo_mapContainerMaterializationRequest>& Requests,
		const TArray<FGuid>& CommittedInstanceIds,
		TFunctionRef<bool(const TArray<FGuid>& InstanceIds, FString& OutDiagnostic)> FinalizeMaterialization,
		TArray<FGuid>& OutInstanceIds);
	Fdemo_mapItemOperationResult TransferContainerItemToInventory(
		FGuid ContainerId,
		FGuid InstanceId,
		bool bFailAfterMutation = false);
	Fdemo_mapItemOperationResult DestroyContainerItem(
		FGuid ContainerId,
		FGuid InstanceId);
	Fdemo_mapItemOperationResult PickupWorldItem(Ademo_mapWorldItem* Actor, APlayerController* Controller);
	Fdemo_mapItemOperationResult DropInventoryItem(FGuid InstanceId, APawn* Pawn, Ademo_mapWorldItem*& OutActor);
	/** P3 Runtime drag target: drops an inventory or non-bundled equipped item by GUID. */
	Fdemo_mapItemOperationResult DropPlayerItemToWorld(
		const Fdemo_mapPlayerItemDropIntent& Intent,
		APawn* Pawn,
		Ademo_mapWorldItem*& OutActor);
	/** Atomically projects the equipped spatial item and its storage instances into the World. */
	Fdemo_mapItemOperationResult DiscardSpatialItemBundle(
		APawn* Pawn,
		Fdemo_mapSpatialDiscardBundle& OutBundle,
		TArray<Ademo_mapWorldItem*>& OutActors);
	/** Atomically re-equips one complete discarded spatial bundle using the original GUIDs. */
	Fdemo_mapItemOperationResult RecoverSpatialItemBundle(
		FGuid BundleId,
		APlayerController* Controller);
	FGuid FindSpatialBundleId(FGuid InstanceId) const;
	int32 GetSpatialBundleMemberCount(FGuid InstanceId) const;
	Fdemo_mapItemOperationResult BeginRun();
	/**
	 * Return the transient Runtime authority to its Preparation boundary after a
	 * durable Profile settlement. Persistent stash/currency remain owned by the
	 * Profile Session and are never read or mutated here.
	 */
	Fdemo_mapItemOperationResult PrepareForPersistentRun();
	Fdemo_mapPreparedRunRuntimeResult MaterializePreparedRun(const Fdemo_mapPreparedRunRuntimeRequest& Request);
	Fdemo_mapItemOperationResult RequestSettlement(Edemo_mapRunEndReason Reason, Fdemo_mapSettlementSummary& OutSummary);
	Fdemo_mapItemOperationResult CreateEnemyLoot(UWorld* World, Edemo_mapEnemyLootArchetype Archetype, FGuid LootSourceId, const FVector& DeathLocation, const AActor* IgnoredActor, TArray<Ademo_mapWorldItem*>& OutActors, bool bEligibleHostile = true);
	Edemo_mapRunState GetRunState() const { return RunState; }
	FGuid GetActiveRunId() const { return ActiveRunId; }
	const TSet<FGuid>& GetDeployedItemIds() const { return DeployedItemIds; }
	bool IsItemAtRiskInActiveRun(FGuid InstanceId) const;
	const Fdemo_mapSettlementSummary& GetLastSettlementSummary() const { return LastSettlementSummary; }
	const TMap<FGuid, Edemo_mapLootSourceState>& GetLootSourceStates() const { return LootSourceStates; }
	int32 GetSessionStashItemCount() const;
	int32 GetSessionStashValue() const;
	bool FindSafeDropLocation(const APawn* Pawn, FVector& OutLocation) const;
	/**
	 * Resolves a grounded, reachable-looking world projection while keeping every
	 * interactive world object independently selectable.  StableSeed makes the
	 * fallback ring deterministic; ReservedLocations keeps one atomic batch from
	 * overlapping itself before any Actor has been committed.
	 */
	bool ResolveSafeWorldLocation(
		UWorld* World,
		const FVector& DesiredLocation,
		const AActor* IgnoredActor,
		FVector& OutLocation,
		FName StableSeed = NAME_None,
		const TArray<FVector>* ReservedLocations = nullptr,
		float CandidateFootprintRadius = 42.0f) const;
	void NotifyWorldActorEndPlay(FGuid InstanceId, Ademo_mapWorldItem* Actor);
	bool IsWorldActorBound(FGuid InstanceId, const Ademo_mapWorldItem* Actor) const;
	Ademo_mapWorldItem* GetWorldActor(FGuid InstanceId) const;
	int32 GetWorldActorCount() const;

	bool BindPlayerPawn(APawn* Pawn);
	bool BindAttributeComponent(Udemo_mapAttributeComponent* AttributeComponent);
	bool BindHealthComponent(Udemo_mapPlayerHealthComponent* HealthComponent);
	bool SynchronizeEquipmentModifiers();
	bool ValidateInvariants(FString* OutError = nullptr) const;

	const Fdemo_mapItemAuthority& GetAuthority() const { return Authority; }
	const TSet<FName>& GetActiveModifierSources() const { return ActiveModifierSources; }
	Udemo_mapAttributeComponent* GetBoundAttributeComponent() const { return BoundAttributeComponent.Get(); }
	static FName MakeModifierSourceId(FGuid InstanceId);

#if !UE_BUILD_SHIPPING
	void ResetForAutomation();
#endif
#if WITH_DEV_AUTOMATION_TESTS
	void SetPreparedRunFailureAfterMutationForAutomation(int32 MutationCount) { PreparedRunFailureAfterMutation = MutationCount; }
	void AdvanceItemUseTimeForAutomation(double Seconds) { ItemUseAutomationTimeOffset += FMath::Max(0.0, Seconds); }
#endif

private:
	Fdemo_mapItemOperationResult FinalizeTransaction(const Fdemo_mapItemAuthorityState& Before, const Fdemo_mapItemOperationResult& CoreResult);
	void RemoveAllEquipmentSourcesFromBoundComponent();
	bool BuildDesiredModifierSources(
		TMap<FName, TArray<Fdemo_mapModifierSpec>>& OutDesired) const;
	double GetItemUseTimeSeconds() const;

	Fdemo_mapItemAuthority Authority;
	Fdemo_mapHotbarBindingSnapshot HotbarBindings;
	TWeakObjectPtr<Udemo_mapAttributeComponent> BoundAttributeComponent;
	TWeakObjectPtr<Udemo_mapPlayerHealthComponent> BoundHealthComponent;
	TWeakObjectPtr<APawn> BoundPlayerPawn;
	TSet<FName> ActiveModifierSources;
	TWeakObjectPtr<UWorld> ActiveWorld;
	TMap<FGuid, TWeakObjectPtr<Ademo_mapWorldItem>> WorldActors;
	TMap<FGuid, Fdemo_mapSpatialDiscardBundle> SpatialDiscardBundles;
	TMap<FGuid, FGuid> SpatialBundleByInstance;
	Edemo_mapRunState RunState = Edemo_mapRunState::Inactive;
	FGuid ActiveRunId;
	TSet<FGuid> DeployedItemIds;
	Fdemo_mapSettlementSummary LastSettlementSummary;
	TMap<FGuid, Edemo_mapLootSourceState> LootSourceStates;
	double HealingPillCooldownEndTime = 0.0;

	Ademo_mapWorldItem* SpawnBoundWorldActor(UWorld* World, FGuid InstanceId, const FVector& Location);
	void RemoveWorldBinding(FGuid InstanceId, const Ademo_mapWorldItem* ExpectedActor = nullptr);
	void ClearSpatialBundleTracking();
	bool ValidateWorldBindings(FString* OutError = nullptr) const;
	Fdemo_mapItemOperationResult TagAffectedForActiveRun(const Fdemo_mapItemAuthorityState& Before, const Fdemo_mapItemOperationResult& Result, const TArray<FGuid>& Affected);
#if WITH_DEV_AUTOMATION_TESTS
	int32 PreparedRunFailureAfterMutation = INDEX_NONE;
	double ItemUseAutomationTimeOffset = 0.0;
#endif
};
