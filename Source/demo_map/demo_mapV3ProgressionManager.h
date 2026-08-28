#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineBaseTypes.h"
#include "TimerManager.h"
#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "CodeB/demo_mapCodeBP4.h"
#include "demo_mapItemTypes.h"
#include "demo_mapProfilePreparationFlow.h"
#include "demo_mapProfileSessionTypes.h"
#include "demo_mapProfileStartupMode.h"
#include "demo_mapRewardGenerator.h"
#include "demo_mapRewardAffix.h"
#if !UE_BUILD_SHIPPING
#include "demo_mapInputRestoreTrace.h"
#include "demo_mapInputConsumptionTrace.h"
#endif
#include "demo_mapV3ProgressionManager.generated.h"

struct FCodeBActivePlayerInteractionCommitResult;
struct FCodeBNormalContainerInteractionCommitResult;
struct FCodeBBodyContainerInteractionCommitResult;
struct FCodeBWorldDropInteractionCommitResult;

class UCharacterMovementComponent;
class APawn;
class UWorld;

#if !UE_BUILD_SHIPPING
enum class Edemo_mapProfileFlowAutomationPhase : uint8
{
	Preparation,
	Extract,
	Death,
	Crash,
	Recovered
};

enum class Edemo_mapProfileTradeAutomationPhase : uint8
{
	Trade,
	Reload
};

enum class Edemo_mapFullSystemLoopAutomationPhase : uint8
{
	Loop,
	Reload,
	Death,
	DeathReload,
	Crash,
	Recovered
};
#endif

class Ademo_mapLootChest;
class Ademo_mapSearchContainerActor;
class Ademo_mapCorpseContainerActor;
class Ademo_mapCodeBNormalContainerActor;
class Ademo_mapCodeBWorldDropActor;
class Ademo_mapWorldItem;
class Ademo_mapSpiritStonePickup;
class Ademo_mapV3ProgressionMarker;
class Ademo_mapPlayerController;
class Udemo_mapInventoryWidget;
class Udemo_mapProfilePreparationWidget;
class Udemo_mapSectNavigationWidget;
class Udemo_mapSettlementWidget;
class Udemo_mapSearchContainerWidget;
class Udemo_mapItemSubsystem;
class Udemo_mapPlayerHealthComponent;
class Udemo_mapProfileSessionSubsystem;
struct Fdemo_mapM01EnemyDefinition;
struct Fdemo_mapRuntimeContainerIntent;
struct Fdemo_mapRuntimeContainerResult;
struct Fdemo_mapSearchContainerDropIntent;
struct Fdemo_mapSearchContainerDropResult;
struct Fdemo_mapRewardSourceProjectionResult;

#if !UE_BUILD_SHIPPING
/**
 * Single-use formatter for the authoritative InputRestore terminal record.
 * It is compiled only for non-Shipping builds and is called only by the
 * explicit -InputRestoreAutomation terminal path.
 */
struct Fdemo_mapInputRestoreTerminalStateEmitter
{
	void Reset() { bEmitted = false; }

	bool TryBuildLine(
		const FString& Phase,
		const FString& Boundary,
		bool bPassed,
		const Ademo_mapPlayerController* Controller,
		const APawn* Pawn,
		const UWorld* World,
		float DistanceUU,
		double LatencySeconds,
		FString& OutLine);

private:
	bool bEmitted = false;
};
#endif

/** V3 root-gated world projection, focus, UI and automation coordinator. */
UCLASS()
class Ademo_mapV3ProgressionManager : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapV3ProgressionManager();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool Initialize(
		APawn* InPlayerPawn,
		Udemo_mapItemSubsystem* InItems,
		bool bUse0909BFrameworkHost = false);
	bool IsInitialized() const { return bInitialized; }
	bool IsInventoryOpen() const { return bInventoryOpen || bCodeBActiveRunInventoryOpen || bCodeBNormalContainerOpen || bCodeBBodyContainerOpen || bCodeBWorldDropOpen; }
	bool IsSearchContainerOpen() const { return bSearchContainerOpen; }
	Udemo_mapItemSubsystem* GetItemSubsystem() const { return Items.Get(); }
	Udemo_mapInventoryWidget* GetInventoryWidget() const { return InventoryWidget.Get(); }
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }
	FText GetInteractionPrompt() const;
	Fdemo_mapItemOperationResult GetLastOperationResult() const { return LastOperationResult; }
	const TArray<TWeakObjectPtr<Ademo_mapLootChest>>& GetChests() const { return Chests; }
	const TArray<TWeakObjectPtr<Ademo_mapCorpseContainerActor>>& GetCorpses() const { return Corpses; }
	const TArray<TWeakObjectPtr<AActor>>& GetEnemyActors() const { return EnemyActors; }
	bool HasNavigableEnemySpawnMarker(FName MarkerId) const
	{
		return NavigableEnemySpawnMarkerIds.Contains(MarkerId);
	}
	Udemo_mapSearchContainerWidget* GetSearchContainerWidget() const { return SearchContainerWidget.Get(); }
	Ademo_mapSearchContainerActor* GetActiveSearchContainer() const { return ActiveSearchContainer.Get(); }

	Fdemo_mapItemOperationResult RequestInteractFocused();
	/** P10 exact-target adapter entry called only by its map-placeable Actor. */
	Fdemo_mapItemOperationResult RequestCodeBNormalContainerInteract(Ademo_mapCodeBNormalContainerActor* Container);
	/** P14 actor adapter entry. The actor forwards only context/range interaction into P6. */
	Fdemo_mapItemOperationResult RequestCodeBWorldDropInteract(Ademo_mapCodeBWorldDropActor* DropActor);
	void NotifyCodeBWorldDropActorEndPlay(Ademo_mapCodeBWorldDropActor* DropActor);
	void CompleteCodeBNormalContainerAction(
		Ademo_mapCodeBNormalContainerActor* Container,
		const FGuid& ActionId,
		bool bSearchAction);
	bool IsCodeBNormalContainerActionStillValid(
		const Ademo_mapCodeBNormalContainerActor* Container,
		const FGuid& ActionId) const;
	void InterruptCodeBNormalContainerAction(
		Ademo_mapCodeBNormalContainerActor* Container,
		const FString& Reason);
	/** P12 exact-body adapter entry; the corpse Actor owns only transient route/range/timer state. */
	Fdemo_mapItemOperationResult RequestCodeBBodyContainerInteract(Ademo_mapCorpseContainerActor* Corpse);
	void CompleteCodeBBodyContainerAction(
		Ademo_mapCorpseContainerActor* Corpse,
		const FGuid& ActionId,
		bool bSearchAction);
	bool IsCodeBBodyContainerActionStillValid(
		const Ademo_mapCorpseContainerActor* Corpse,
		const FGuid& ActionId) const;
	void InterruptCodeBBodyContainerAction(
		Ademo_mapCorpseContainerActor* Corpse,
		const FString& Reason);
	void ReleaseInteractFocused();
	Fdemo_mapItemOperationResult RequestDropInventory(FGuid InstanceId);
	/** P15's one gameplay adapter for discrete 1--9 presses; Code B owns item mutation. */
	bool RequestUseBoundCodeBQuickSlot(int32 SlotIndex);
	/** Authority-selecting hotbar entry; Shanmen failures never fall back to Code B. */
	bool RequestUseBoundQuickSlot(int32 SlotIndex);
	/** Authority-selecting inventory entry; Shanmen failures never fall back to Runtime-only use. */
	Fdemo_mapItemUseResult RequestUseInventoryItem(FGuid ItemInstanceId);
	void ToggleInventory();
	void OpenInventory();
	void CloseInventory();
	void OpenSearchContainer(Ademo_mapSearchContainerActor* Container);
	void CloseSearchContainer(const FString& Reason, bool bCancelAction);
	void RefreshSearchContainerWidget();
	Fdemo_mapRuntimeContainerResult SubmitSearchContainerIntent(
		const Fdemo_mapRuntimeContainerIntent& Intent);
	Fdemo_mapSearchContainerDropResult SubmitSearchContainerDrop(
		const Fdemo_mapSearchContainerDropIntent& Intent);
	/** P3 Run-only world-drop entry used by both inventory and search surfaces. */
	Fdemo_mapItemOperationResult RequestDropPlayerItem(
		const Fdemo_mapPlayerItemDropIntent& Intent,
		bool bConfirmSpatialBundle = false);
	void NotifySearchContainerEndPlay(Ademo_mapSearchContainerActor* Container);
	void RefreshFocusNow();
	Fdemo_mapItemOperationResult RequestSettlementAndReload(Edemo_mapRunEndReason Reason);
	Fdemo_mapItemOperationResult HandleEnemyDeath(Edemo_mapEnemyLootArchetype Archetype, FGuid LootSourceId, const FVector& DeathLocation, const AActor* EnemyActor);
	Fdemo_mapItemOperationResult HandleEnemyDeath(
		FName LootTableId,
		FGuid LootSourceId,
		const FVector& DeathLocation,
		const AActor* EnemyActor);
	Fdemo_mapItemOperationResult HandleM01EnemyDeath(
		FName RewardSourceRoleId,
		FName CorpseIdentity,
		FGuid LootSourceId,
		const FVector& DeathLocation,
		const AActor* EnemyActor);
	Fdemo_mapProfileSessionBeginResult StartPreparedProfileRun();
	/** Routes the sect teleport CTA through the established Preparation/Start Run authority. */
	Fdemo_mapProfileSessionBeginResult StartPreparedProfileRunFromSect();
	/**
	 * I1 adapter entry: commits Code A's prepared Run without selecting any
	 * retired presentation surface. World activation and Code B observation are
	 * deliberately performed later by the 0.0.9B coordinator.
	 */
	Fdemo_mapProfileSessionBeginResult BeginPreparedProfileRunFor0909B();
	/** Performs only reversible M01 materialization; it never settles or routes UI. */
	bool ActivatePreparedProfileWorldFor0909B(FString& OutDiagnostic);
	/** Cancels a failed pre-success deployment as an ActivationFailure, never a player terminal. */
	bool RollbackPreparedProfileRunFor0909B(FString& OutDiagnostic);
	/** P6 is a post-success observer only. */
	void ObserveCodeBRunAfter0909BActivation(
		const Fdemo_mapProfileSessionSnapshot& Snapshot);
	void Set0909BOutOfRaidCloseCallback(TFunction<void()>&& InCallback);
	Fdemo_mapProfileSessionSettlementResult RetryPendingProfileSettlement();
	bool CanGenerateRewardSource(FGuid RunId, FName RewardSourceId) const;
	bool FindDurablyAcceptedRewardSource(
		FGuid RunId,
		FName RewardSourceId,
		Fdemo_mapPersistentGeneratedRewardSource& OutSource) const;
	Fdemo_mapProfileGeneratedRewardSourceResult PrepareGeneratedRewardSource(
		FGuid RunId,
		FName RewardSourceId,
		const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt);
	bool CommitGeneratedRewardSource(
		FGuid RunId,
		FName RewardSourceId,
		const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt);
	const Fdemo_mapRewardSourceAcceptanceReceipt*
		FindAcceptedRewardSourceReceipt(
			FGuid RunId,
			FName RewardSourceId) const;
	int32 GetRewardAffixPityState(FGuid RunId) const;
	bool CommitRewardAffixPity(
		FGuid RunId,
		const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt);
	bool IsSettlementPending() const { return bSettlementPending; }
	Udemo_mapSettlementWidget* GetSettlementWidget() const { return SettlementWidget.Get(); }
	/** Retires only the transient report and its input lock; durable settlement history is untouched. */
	void DismissSettlementPresentation(const TCHAR* Reason);
	/** User-facing dismissal always returns to the Sect product hub. */
	void ReturnToSectAfterSettlement(const TCHAR* Reason);
	bool IsProfilePreparationFlowActive() const { return Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode); }
	bool IsProfileWorldActive() const { return bProfileWorldActive; }
	Edemo_mapProfileStartupMode GetProfileStartupMode() const { return ProfileStartupMode; }
	const Fdemo_mapProfilePreparationFlow* GetProfilePreparationFlow() const { return ProfilePreparationFlow.Get(); }
	Udemo_mapProfilePreparationWidget* GetProfilePreparationWidget() const { return ProfilePreparationWidget.Get(); }
	Udemo_mapSectNavigationWidget* GetSectNavigationWidget() const { return SectNavigationWidget.Get(); }
	/** Opens the existing authoritative Preparation surface from the P1 sect router. */
	void OpenProfilePreparationFromSect(bool bReturnToTeleport = false);
	/** Read-only normal-entry gate for Code B's out-of-raid host. */
	bool IsCodeBOutOfRaidInventoryEntryAvailable() const;
	/** P5's only product-facing Code B entry; it cannot initialize Run state. */
	bool OpenCodeBOutOfRaidInventory(FString& OutFeedback);
	void RestoreSectNavigationAfterCodeBOutOfRaidClose();
	/** Returns from Preparation to the sect home without releasing the UI input lock. */
	void ReturnToSectNavigation();
#if !UE_BUILD_SHIPPING
	/** Read-only P5 smoke inspection; no Profile, Repository, or UI mutation is exposed. */
	bool HasCodeBOutOfRaidProfileForAutomation() const
	{
		return CodeBOutOfRaidRepository.IsValid() && CodeBOutOfRaidProfileStore.IsValid();
	}
	int32 GetCodeBOutOfRaidPersistentRevisionForAutomation() const
	{
		return CodeBOutOfRaidProfileStore.IsValid()
			? CodeBOutOfRaidProfileStore->GetPersistentRevision() : INDEX_NONE;
	}
	FGuid GetCodeBOutOfRaidOwnerIdForAutomation() const
	{
		return CodeBOutOfRaidProfileStore.IsValid()
			? CodeBOutOfRaidProfileStore->GetRecord().OwnerId : FGuid();
	}
	int32 GetCodeBOutOfRaidHandoffStateForAutomation() const
	{
		return CodeBOutOfRaidProfileStore.IsValid()
			? static_cast<int32>(CodeBOutOfRaidProfileStore->GetRecord().Receipt.State)
			: INDEX_NONE;
	}
	void RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent Event,
		float DistanceUU = -1.0f,
		double LatencySeconds = -1.0,
		Edemo_mapInputRestoreTraceCaller Caller =
			Edemo_mapInputRestoreTraceCaller::Manager);
	bool IsInputRestoreTraceEnabled() const
	{
		return InputRestoreTrace.IsValid() && InputRestoreTrace->IsEnabled();
	}
#endif

private:
	/** P7 input edge: reads an exact P6 active session and opens the shared P3/P4 host. */
	bool OpenCodeBActiveRunInventory(FString& OutFeedback);
	/** P14 Code A adapter: resolves a safe floor transform, then calls the sole P6 writer. */
	bool RequestCodeBGroundDrop(
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutFeedback);
	/** P70/P71 adapter for exact opened WorldDrop split or same-record placement. */
	bool RequestCodeBWorldDropGroundDrop(
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutFeedback);
	/** P49 adapter for the same GroundDropZone while the exact P10 BasicCache page is open. */
	bool RequestCodeBNormalContainerGroundDrop(
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutFeedback);
	/** P43/P44 adapter for the same GroundDropZone while the exact P12 body page is open. */
	bool RequestCodeBBodyGroundDrop(
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutFeedback);
	bool ResolveCodeBWorldDropPlacement(FName& OutMapRoute, FTransform& OutFloorTransform, FString& OutFeedback) const;
	/** Applies the explicit, read-only accepted-result contract; it never inspects P2 command or Store state. */
	void ApplyCodeBRunItemInteractionResult(const FCodeBActivePlayerInteractionCommitResult& Result);
	void ApplyCodeBRunItemInteractionResult(const FCodeBNormalContainerInteractionCommitResult& Result);
	void ApplyCodeBRunItemInteractionResult(const FCodeBBodyContainerInteractionCommitResult& Result);
	void ApplyCodeBRunItemInteractionResult(const FCodeBWorldDropInteractionCommitResult& Result);
	void RefreshCodeBWorldDropActors();
	void ClearCodeBWorldDropActors();
	bool OpenCodeBWorldDropPage(Ademo_mapCodeBWorldDropActor* DropActor, FString& OutFeedback);
	void CloseCodeBWorldDropPage();
	void CloseCodeBActiveRunInventory();
	bool DeliverPendingCodeBQuickUseReceipts();
	bool IsCodeBQuickUseDeliveryLegal(
		Fdemo_mapProfileSessionSnapshot& OutSnapshot,
		class Udemo_mapPlayerHealthComponent*& OutHealth) const;
	bool OpenCodeBNormalContainerPage(
		Ademo_mapCodeBNormalContainerActor* Container,
		const FCodeBNormalContainerProjection& Projection,
		FString& OutFeedback);
	void CloseCodeBNormalContainerPage(const FString& Reason);
	bool BeginCodeBNormalContainerItemSearch(
		const FGuid& ItemId,
		FCodeBNormalContainerProjection& OutProjection,
		FString& OutFeedback);
	bool OpenCodeBBodyContainerPage(
		Ademo_mapCorpseContainerActor* Corpse,
		const FCodeBBodyContainerProjection& Projection,
		FString& OutFeedback);
	void CloseCodeBBodyContainerPage(const FString& Reason);
	bool BeginCodeBBodyContainerItemSearch(
		const FGuid& ItemId,
		FCodeBBodyContainerProjection& OutProjection,
		FString& OutFeedback);
	/** P11 post-death observer. It forwards no actor, loot, UI, or result back into Code A. */
	void ObserveCodeBBodyContainerAfterCodeADeath(const Fdemo_mapM01EnemyDefinition& EnemyDefinition);
	/** P8 observes only an already-persisted Code A terminal decision. */
	void ObserveCodeBRunTerminalAfterCodeACommit(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		ECodeBRunInventoryTerminalState TerminalState);
	bool InitializeWorldContent();
	bool InitializeM01RewardContent();
	/** P57's static M01 targets are non-blocking Code A adapters, never Run truth. */
	bool InitializeCodeBNormalContainerTarget();
	bool IsRegisteredCodeBNormalContainerTarget(
		const Ademo_mapCodeBNormalContainerActor* Container) const;
	bool InitializeEnemyEncounterContent();
	bool SpawnM01SpiritStonePickup(
		const Fdemo_mapM01EnemyDefinition& EnemyDefinition,
		const FVector& DeathLocation,
		const AActor* EnemyActor);
	void DestroyEnemyEncounterContent();
	void SetFocusedActor(AActor* NewFocus);
	/** Returns true when a real mouse ray was available, even if it found no legal target. */
	bool ResolvePointerFocusedActor(AActor*& OutFocus) const;
	bool IsVisibleCandidate(AActor* Candidate, const FVector& InteractionLocation) const;
	Ademo_mapPlayerController* GetDemoController() const;
	void StartRequestedAutomation();
	void RunWorldInteractionAutomation();
	void RunInventoryUIAutomation();
	void RunVisibleAcceptanceStep();
	void RunP5RuntimeVisibleAcceptanceStep();
	void RunP6DualLootVisibleAcceptanceStep();
	void RunEnemyLootAutomation();
	void RunLifecycleAutomation();
	void RunSettlementUIAutomation();
	void RunFreshSessionAutomation();
	void RunCloseRangeProjectileAutomation();
	void RunLifecycleVisibleStep();
	void RunRepairVisibleAcceptanceStep();
	void RunV3FinalAutomation();
	void RunV3FinalVisibleAcceptance();
	void RunSearchContainerAutomation();
		void RunRewardGenerationAutomation();
		void RunRewardJackpotAutomation();
		void RunRewardRareExtremeAutomation();
		void RunRewardAffixPityAutomation();
		void RunRewardShopStockAutomation();
		void RunRewardBossSourceAutomation();
		void RunRewardFullMapDistributionAutomation();
	void RunInputRestoreAutomation();
	bool InitializeInputRestoreAutomation();
	void ScheduleInputRestoreAutomation(float Delay);
	void FinishInputRestoreAutomation(bool bPassed, const FString& Reason);
	void CompleteInputRestoreAutomationVerdict(
		bool bPassed,
		const FString& Reason);
	void TryFinalizeInputConsumptionTrace();
	void SetInputRestoreTraceBoundary(const TCHAR* Boundary);
	bool BeginInputRestoreMovementProbe(const TCHAR* Boundary);
	void SampleInputRestoreMovementProbe();
	void ActivateInputConsumptionTrace();
	void DeactivateInputConsumptionTrace();
	void HandleInputConsumptionPostActorTick(
		UWorld* World,
		ELevelTick TickType,
		float DeltaSeconds);
	bool PrepareInputRestoreContainer(bool bCorpse);
	bool TakeAndCloseInputRestoreContainer(bool bCorpse);
	void RunEnemySkillFrameworkAutomation();
	void RunEnemyRouteLootAutomation();
	bool ValidateFinalNewRunState(const TCHAR* RoundName, Edemo_mapRunEndReason ExpectedReason, int32 ExpectedStashQuantity, int32 ExpectedStashValue);
	void LogFinalAutomationSnapshot(const TCHAR* RoundName, const Fdemo_mapSettlementSummary& Summary) const;
	bool CompleteTrainingMissionAndEnterExit(int32 NextGlobalPhase);
	void ScheduleVisibleStep(float Delay);
	void CaptureVisible(const FString& Filename) const;
	void FailAutomation(const FString& Message) const;
	void PassAutomation(const TCHAR* Marker) const;
	void LogInputRestoreDiagnostics(const TCHAR* Phase) const;
	void ReloadAfterSettlement();
	void ShowSettlement(
		const Fdemo_mapSettlementSummary& Summary,
		FGuid SettlementId = FGuid());
	bool MovePawnNear(AActor* Target, float Distance = 120.0f);
	bool PressBoundKey(const FKey& Key);
	bool ActivatePreparedProfileWorld();
	void ShowProfilePreparation();
	void ShowSectNavigation();
	void HideProfilePreparation();
	void DeactivateProfileWorld();
	void DestroyRuntimeContainers(const FString& Reason);
	/** The sole post-activation Code B observer.  It never feeds back into Code A. */
	void ObserveCodeBRunAfterActivation(const Fdemo_mapProfileSessionSnapshot& Snapshot);
#if !UE_BUILD_SHIPPING
	bool InitializeSearchContainerAutomationProfile();
	void FinishSearchContainerAutomation(bool bPassed, const FString& Reason);
	bool InitializeRewardGenerationAutomationProfile();
	void FinishRewardGenerationAutomation(bool bPassed, const FString& Reason);
	bool InitializeEnemySkillAutomationProfile();
	bool InitializeEnemyRouteLootAutomationProfile();
	void FinishEnemySkillFrameworkAutomation(
		bool bPassed,
		const FString& Reason);
#endif
	UFUNCTION()
	void HandlePlayerDamaged(int32 AppliedDamage);
#if !UE_BUILD_SHIPPING
	void ReadNonShippingStartupFlags();
	bool IsLegacyAutomationRequested() const;
	bool InitializeExplicitProfileFlow();
	/** Seeds an isolated formal Profile for P5's visible real-Profile input trace. */
	bool InitializeP5RealProfileTraceProfile();
	/** P6 r1/r2: product-CTA trace; the bridge remains a post-activation observer. */
	bool InitializeP6ProductStartBridgeProfile();
	void RunP6ProductStartBridgeAutomation();
	/** P6 r2/r3 evidence trace; this never changes Code A or the bridge result. */
	void LogP6ProductStartBridgeR2Trace(
		const TCHAR* Event,
		const Fdemo_mapProfileSessionSnapshot& Snapshot,
		const FCodeBOutOfRaidInventoryRecord* BeforeRecord,
		const FCodeBOutOfRaidInventoryRecord* AfterRecord,
		ECodeBRunInventoryBridgeStatus BridgeStatus,
		const FString& Detail);
	void WriteP6ProductStartBridgeR3Outcome(bool bPassed, const FString& Detail) const;
	void RunProfileFlowAutomation();
	void RunProfileTradeAutomation();
	void RunP4xStartRunAutomation();
	void RunXFix1SettlementLifecycleAutomation();
	void FinishProfileFlowAutomation();
	bool InitializeFullSystemLoopAutomation();
	bool PrepareFullSystemAutomationRun();
	void RunFullSystemLoopAutomation();
	bool TakeFullSystemRequiredLoot();
	bool CollectFullSystemSpiritStone();
	bool WriteFullSystemHandoff() const;
	bool ReadFullSystemHandoff(TMap<FString, FString>& OutValues) const;
	void ScheduleFullSystemAutomation(float Delay);
#endif

	TWeakObjectPtr<APawn> PlayerPawn;
	TWeakObjectPtr<Udemo_mapItemSubsystem> Items;
	TWeakObjectPtr<AActor> FocusedActor;
	TArray<TWeakObjectPtr<Ademo_mapLootChest>> Chests;
	TArray<TWeakObjectPtr<Ademo_mapCorpseContainerActor>> Corpses;
	TArray<TWeakObjectPtr<AActor>> EnemyActors;
	TSet<FName> NavigableEnemySpawnMarkerIds;
	TSet<FGuid> CorpseLootSourceIds;
	Fdemo_mapRewardGenerationSession RewardGenerationSession;
	Fdemo_mapRewardAffixPityLedger RewardAffixPityLedger;
	FGuid M01RewardRunId;
	bool bM01RewardContentActive = false;
	TArray<TWeakObjectPtr<Ademo_mapWorldItem>> InitialWorldItems;
	TWeakObjectPtr<Ademo_mapSpiritStonePickup> SpiritStonePickup;
	TArray<TWeakObjectPtr<Ademo_mapSpiritStonePickup>> M01SpiritStonePickups;
	TSet<FName> M01SpiritStoneSpawnSourceIds;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapInventoryWidget> InventoryWidget;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapProfilePreparationWidget> ProfilePreparationWidget;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapSectNavigationWidget> SectNavigationWidget;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapSettlementWidget> SettlementWidget;
	FGuid ActiveSettlementPresentationId;
	TSet<FGuid> PresentedSettlementIds;
	UPROPERTY(Transient) TObjectPtr<Udemo_mapSearchContainerWidget> SearchContainerWidget;
	TWeakObjectPtr<Ademo_mapSearchContainerActor> ActiveSearchContainer;
	Fdemo_mapItemOperationResult LastOperationResult;
	FTimerHandle AutomationTimer;
	FTimerHandle SettlementReloadTimer;
	float FocusAccumulator = 0.0f;
	float QuickUseDeliveryAccumulator = 0.0f;
	bool bInitialized = false;
	/** The former top-level UI is retained only behind this adapter and cannot become the default product shell. */
	bool bUse0909BFrameworkHost = false;
	bool bInventoryOpen = false;
	bool bSearchContainerOpen = false;
	bool bSettlementPending = false;
	bool bSavedShowMouseCursor = true;
	bool bSavedClickEvents = false;
	bool bSavedMouseOverEvents = false;
	bool bSavedMoveIgnored = false;
	bool bSavedLookIgnored = false;
	EMouseCaptureMode SavedCaptureMode = EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
	EMouseLockMode SavedLockMode = EMouseLockMode::LockOnCapture;
	bool bWorldAutomation = false;
	bool bInventoryUIAutomation = false;
	bool bVisibleAcceptance = false;
	bool bP5RuntimeVisibleAcceptance = false;
	bool bP6DualLootVisibleAcceptance = false;
	/** Development-only physical-input fixture.  It creates no persistent data. */
	bool bP6DualLootManualFixture = false;
	bool bP6DualLootManualFixturePrepared = false;
	bool bEnemyLootAutomation = false;
	bool bRunLifecycleAutomation = false;
	bool bPostSettlementInputRestoreAutomation = false;
	bool bSettlementUIAutomation = false;
	bool bFreshSessionAutomation = false;
	bool bCloseRangeProjectileAutomation = false;
	bool bLifecycleVisibleAcceptance = false;
	bool bRepairVisibleAcceptance = false;
	bool bV3FinalAutomation = false;
	bool bV3FinalVisibleAcceptance = false;
	bool bSearchContainerAutomation = false;
	bool bEnemySkillFrameworkAutomation = false;
	bool bEnemyRouteLootAutomation = false;
	bool bRewardGenerationAutomation = false;
		bool bRewardSourceProjectionAutomation = false;
		bool bRewardJackpotAutomation = false;
		bool bRewardRareExtremeAutomation = false;
		bool bRewardAffixPityAutomation = false;
	bool bRewardShopStockAutomation = false;
	bool bRewardBossSourceAutomation = false;
	bool bRewardFullMapDistributionAutomation = false;
	bool bInputRestoreDiagnostics = false;
	TUniquePtr<Fdemo_mapProfilePreparationFlow> ProfilePreparationFlow;
	TUniquePtr<demo_map_code_b::FCodeBRepository> CodeBOutOfRaidRepository;
	TUniquePtr<FCodeBOutOfRaidProfileStore> CodeBOutOfRaidProfileStore;
	/** When the I1 host opens P5, the host (not the retired sect page) regains focus on close. */
	TFunction<void()> OutOfRaidCloseOverride;
	/** P7 owns only this in-memory P1 view over an already-committed P6 session. */
	TUniquePtr<demo_map_code_b::FCodeBRepository> CodeBActiveRunInventoryRepository;
	TUniquePtr<FCodeBOutOfRaidProfileStore> CodeBActiveRunInventoryStore;
	FGuid CodeBActiveRunInventoryOwnerId;
	FGuid CodeBActiveRunInventoryRunId;
	int32 CodeBActiveRunInventoryExpectedP6Revision = INDEX_NONE;
	bool bCodeBActiveRunInventoryOpen = false;
	TMap<FGuid, TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>> CodeBWorldDropActors;
	TUniquePtr<demo_map_code_b::FCodeBRepository> CodeBWorldDropRepository;
	FGuid CodeBWorldDropOwnerId;
	FGuid CodeBWorldDropRunId;
	FGuid CodeBWorldDropId;
	FGuid CodeBWorldDropContainerId;
	FGuid CodeBWorldDropRootItemId;
	FGuid CodeBWorldDropSpatialChildContainerId;
	FName CodeBWorldDropMapRoute = NAME_None;
	int32 CodeBWorldDropOrdinal = 0;
	int32 CodeBWorldDropRecordRevision = INDEX_NONE;
	int32 CodeBWorldDropExpectedP6Revision = INDEX_NONE;
	uint32 CodeBWorldDropOpenGeneration = 0;
	uint32 NextCodeBWorldDropOpenGeneration = 1;
	TWeakObjectPtr<Ademo_mapCodeBWorldDropActor> ActiveCodeBWorldDrop;
	bool bCodeBWorldDropOpen = false;
	/** P10 retains a single transient P1 composite view; P6/P9 remain durable truth. */
	TUniquePtr<demo_map_code_b::FCodeBRepository> CodeBNormalContainerRepository;
	FGuid CodeBNormalContainerOwnerId;
	FGuid CodeBNormalContainerRunId;
	FGuid CodeBNormalContainerTargetId;
	FName CodeBNormalContainerDefinitionId = NAME_None;
	FGuid CodeBNormalContainerActionId;
	int32 CodeBNormalContainerExpectedP6Revision = INDEX_NONE;
	int32 CodeBNormalContainerExpectedTargetRevision = INDEX_NONE;
	TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor> ActiveCodeBNormalContainer;
	/** P57 registry for the two exact map sources; Actor state remains projection-only. */
	TArray<TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>> CodeBNormalContainerTargets;
	/** Only runtime-created adapters are destroyed by the manager; map-authored targets are retained. */
	TArray<TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>> SpawnedCodeBNormalContainerTargets;
	bool bCodeBNormalContainerOpen = false;
	/** P12 retains a separate transient composite; P6/P11 remain the durable truths. */
	TUniquePtr<demo_map_code_b::FCodeBRepository> CodeBBodyContainerRepository;
	FGuid CodeBBodyContainerOwnerId;
	FGuid CodeBBodyContainerRunId;
	FGuid CodeBBodyContainerTargetId;
	FName CodeBBodyContainerDefinitionId = NAME_None;
	FGuid CodeBBodyContainerActionId;
	int32 CodeBBodyContainerExpectedP6Revision = INDEX_NONE;
	int32 CodeBBodyContainerExpectedTargetRevision = INDEX_NONE;
	TWeakObjectPtr<Ademo_mapCorpseContainerActor> ActiveCodeBBodyContainer;
	bool bCodeBBodyContainerOpen = false;
	bool bReturnToTeleportAfterPreparation = false;
	Edemo_mapProfileStartupMode ProfileStartupMode = Edemo_mapProfileStartupMode::Disconnected;
	bool bProfileWorldActive = false;
#if !UE_BUILD_SHIPPING
	bool bProfileFlowAutomation = false;
	bool bProfileTradeAutomation = false;
	/** Test startup only: the trace itself still reaches Code B through normal Sect Slate input. */
	bool bP5RealProfileTraceStartup = false;
	/** P4x r2: isolated, product-CTA Start Run evidence. Never uses the retired Preparation widget. */
	bool bP4xStartRunAutomation = false;
	/** P6 r1: isolated product Start Run evidence for the non-blocking bridge. */
	bool bP6ProductStartBridgeAutomation = false;
	/** P6 r2 adds structured evidence only; it deliberately retains the r1 product route. */
	bool bP6ProductStartBridgeR2Trace = false;
	/** P6 r3 permits only a verified Prepared receipt rebind after Code A recovery. */
	bool bP6ProductStartBridgeR3Trace = false;
	bool bXFix1SettlementLifecycleAutomation = false;
	bool bXFix1SettlementRestartAutomation = false;
	bool bFullSystemLoopAutomation = false;
	bool bLegacyStartupAutomation = false;
	bool bInputRestoreAutomation = false;
	bool bInputMovementGateTrace = false;
	TUniquePtr<Fdemo_mapInputRestoreTrace> InputRestoreTrace;
	TUniquePtr<Fdemo_mapInputConsumptionTrace> InputConsumptionTrace;
	TWeakObjectPtr<UCharacterMovementComponent>
		InputConsumptionObservedMovement;
	FDelegateHandle InputConsumptionPostActorTickHandle;
	FString InputRestorePhase;
	FString InputRestoreStorageRoot;
	FString InputRestoreUserConfigRoot;
	FString InputRestoreAutomationRoot;
	int32 InputRestoreAutomationStep = 0;
	double InputRestoreBoundarySeconds = 0.0;
	FVector InputRestoreMovementStart = FVector::ZeroVector;
	FString InputRestoreBoundary;
	FGuid InputRestoreTakenItemId;
	TWeakObjectPtr<Ademo_mapSearchContainerActor> InputRestoreContainer;
	int32 InputRestoreEntrySection = INDEX_NONE;
	int32 InputRestoreEntrySlot = INDEX_NONE;
	int32 InputRestoreAttackCount = 0;
	int32 InputRestoreContainerWaitRetries = 0;
	bool bInputRestoreTraceSawVelocity = false;
	bool bInputRestoreTraceSawDisplacement = false;
	bool bInputRestoreTraceSawTenUU = false;
	bool bInputRestoreVerdictPending = false;
	bool bInputRestoreVerdictPassed = false;
	float InputRestoreLastProbeDistance = 0.0f;
	double InputRestoreLastProbeLatency = 0.0;
	FString InputRestoreVerdictReason;
	Fdemo_mapInputRestoreTerminalStateEmitter InputRestoreTerminalStateEmitter;
	Edemo_mapProfileFlowAutomationPhase ProfileFlowAutomationPhase = Edemo_mapProfileFlowAutomationPhase::Preparation;
	Edemo_mapProfileSessionInitializeStatus ProfileFlowInitializeStatus = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
	FString ProfileFlowStorageRoot;
	FGuid ProfileFlowItemId;
	FGuid ProfileFlowExpectedItemId;
	Edemo_mapProfileTradeAutomationPhase ProfileTradeAutomationPhase = Edemo_mapProfileTradeAutomationPhase::Trade;
	FString ProfileTradeStorageRoot;
	FString ProfileTradeHandoffPath;
	FString P5RealProfileStorageRoot;
	FString P5RealProfileCase;
	/** I1-only isolated real CTA Profile. Empty always selects production storage. */
	FString I1ProfileStorageRoot;
	FString P4xStartRunStorageRoot;
	FString P4xStartRunCase;
	int32 P4xStartRunAutomationStep = 0;
	FGuid P4xStartRunFirstId;
	Fdemo_mapPersistentPreparationLayout P4xPreparationLayoutBefore;
	FString P6ProductStartBridgeStorageRoot;
	FString P6ProductStartBridgeCase;
	FString P6ProductStartBridgePhase;
	int32 P6ProductStartBridgeStep = 0;
	FGuid P6ProductOwnerId;
	FGuid P6ProductPreviousRunId;
	TArray<FGuid> P6ProductExpectedCarryItemIds;
	FGuid P6ProductWarehouseOnlyItemId;
	int32 P6ProductPersistentRevisionBefore = INDEX_NONE;
	/** Snapshot/layout captured after normal P5 open and before the real product CTA. */
	demo_map_code_b::FCodeBSnapshot P6ProductSourceSnapshotBefore;
	demo_map_code_b::FCodeBP2PlayerLayout P6ProductSourceLayoutBefore;
	int32 P6ProductTraceSequence = 0;
	bool bP6ProductBridgeObserved = false;
	FCodeBRunInventoryBridgeResult P6ProductLastBridge;
	FString XFix1StorageRoot;
	int32 XFix1AutomationStep = 0;
	FGuid XFix1Run1Id;
	FGuid XFix1PersistentSettlementId;
	Fdemo_mapSettlementSummary XFix1PresentationSummary;
	TArray<uint8> ProfileTradePrimaryBytesBeforeLoad;
	Edemo_mapFullSystemLoopAutomationPhase FullSystemLoopAutomationPhase =
		Edemo_mapFullSystemLoopAutomationPhase::Loop;
	FString FullSystemLoopStorageRoot;
	FString FullSystemLoopUserConfigRoot;
	FString FullSystemLoopHandoffPath;
	TArray<uint8> FullSystemLoopPrimaryBytesBeforeLoad;
	int32 FullSystemLoopAutomationStep = 0;
	FGuid FullSystemLoopProfileId;
	FGuid FullSystemLoopRunId;
	FGuid FullSystemLoopBackpackId;
	FGuid FullSystemLoopCoreId;
	FGuid FullSystemLoopPillId;
	FName FullSystemLoopBackpackDefinitionId;
	FName FullSystemLoopCoreDefinitionId;
	FName FullSystemLoopPillDefinitionId;
	FName FullSystemLoopRewardProjectionId;
	FName FullSystemLoopRewardSourceRoleId;
	int64 FullSystemLoopCoreSaleValue = 0;
	int64 FullSystemLoopPillBuyValue = 0;
	int64 FullSystemLoopExpectedBalance = 0;
	int32 FullSystemLoopExpectedGeneration = 0;
	int32 FullSystemLoopExpectedShopGeneration = INDEX_NONE;
	int32 FullSystemLoopAcceptedCommitCount = 0;
	FString SearchContainerStorageRoot;
	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem> SearchContainerProfileSession;
	int32 SearchContainerAutomationStep = 0;
	FGuid SearchContainerChestTakenId;
	FGuid SearchContainerWeaponId;
	FGuid SearchContainerSoulBoneId;
	FString RewardGenerationStorageRoot;
	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem>
		RewardGenerationProfileSession;
	int32 RewardGenerationAutomationStep = 0;
	FGuid RewardGenerationTakenItemId;
	FName RewardGenerationTakenDefinitionId = NAME_None;
	int32 RewardGenerationTakenQuantity = 0;
		TArray<FGuid> RewardSourceProjectionTakenIds;
	FGuid RewardJackpotTakenItemId;
	Fdemo_mapPersistentShopStockState RewardShopStockGeneration0;
	FGuid RewardShopStockInitialRunId;
	FString EnemySkillAutomationPhase;
	FString EnemySkillStorageRoot;
	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem> EnemySkillProfileSession;
	int32 EnemySkillAutomationStep = 0;
	double EnemySkillAutomationStartTime = 0.0;
	int32 EnemySkillHealthBefore = 0;
	int32 EnemySkillProjectileCountBefore = 0;
	FVector EnemySkillPlayerStart = FVector::ZeroVector;
	FVector EnemySkillActorStart = FVector::ZeroVector;
	FString EnemyRouteLootStorageRoot;
	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem> EnemyRouteLootProfileSession;
	int32 EnemyRouteLootAutomationStep = 0;
	double EnemyRouteLootAutomationStartTime = 0.0;
	int32 EnemyRouteLootWorldItemCountBefore = 0;
	int64 EnemyRouteLootPersistentBalanceBefore = 0;
	int64 EnemyRouteLootRiskBalanceBefore = 0;
#endif
	FString VisibleOutputDirectory;
	int32 VisibleStep = 0;
	int32 FinalAutomationStep = 0;
	int32 CloseRangeScenario = 0;
	int32 CloseRangePrimaryHealthBefore = 0;
	int32 CloseRangeSecondaryHealthBefore = 0;
	bool bCloseRangeAwaitingResult = false;
	TWeakObjectPtr<AActor> CloseRangePrimaryTarget;
	TWeakObjectPtr<AActor> CloseRangeSecondaryTarget;
	TWeakObjectPtr<AActor> CloseRangeBlockingActor;
};
