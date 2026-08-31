// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "demo_mapAttributeTypes.h"
#include "demo_mapM01Extraction.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapItemTypes.h"
#include "demo_mapProfileSessionTypes.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenControlledWeaponRunCommandRouter.h"
#include "demo_mapShanmenControlledWeaponRunLifecycle.h"
#include "demo_mapShanmenControlledWeaponThreatSampleRouter.h"
#include "demo_mapShanmenThrownWeaponInputAdapter.h"
#include "demo_mapShanmenThrownWeaponProductLifecycle.h"
#include "demo_mapShanmenRunCorrelation.h"
#include "demo_mapShanmenSpiritEvasionProductRoute.h"
#include "demo_mapShanmenWeaponGuardFixedTimeline.h"
#include "demo_mapShanmenWeaponGuardProductSession.h"
#include "demo_mapGameMode.generated.h"

class APlayerController;
class APawn;
class Ademo_mapExitZone;
class Ademo_mapEnemyCharacter;
class Ademo_mapRangedEnemyCharacter;
class Ademo_mapHeavyEnemyCharacter;
class Ademo_mapPlayerController;
class Ademo_mapTrainingTarget;
class Ademo_mapFriendlyUnit;
class Udemo_mapPlayerHealthComponent;
class Udemo_mapSkillComponent;
class Udemo_mapAttributeComponent;
class Udemo_mapItemSubsystem;
class UPrimitiveComponent;
class Ademo_mapSkillProjectile;
class Ademo_mapEncounterMarker;
class Ademo_mapV3ProgressionManager;
class Ademo_map0909BFrameworkHost;
class Ademo_mapM01ExtractionZone;
class Ademo_mapM01Marker;
class Ademo_mapM01BossCharacter;
struct Fdemo_map0909BRunStartResult;
struct FHitResult;
struct FOverlapResult;

/** Coordinates the runtime-only three-target mission loop. */
UCLASS()
class Ademo_mapGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	Ademo_mapGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;
	void HandleExtraction();
	bool IsResetPending() const { return bResetPending; }
	FString GetEndStatusText() const { return EndStatusText; }
	Ademo_mapFriendlyUnit* GetFriendlyUnit() const { return FriendlyUnit.Get(); }
	Ademo_mapEnemyCharacter* GetMeleeEnemy() const { return Enemy.Get(); }
	Ademo_mapRangedEnemyCharacter* GetRangedEnemy() const { return RangedEnemy.Get(); }
	Ademo_mapHeavyEnemyCharacter* GetHeavyEnemy() const { return HeavyEnemy.Get(); }
	Ademo_mapV3ProgressionManager* GetV3ProgressionManager() const { return V3ProgressionManager.Get(); }
	Ademo_map0909BFrameworkHost* Get0909BFrameworkHost() const { return Framework0909BHost.Get(); }
	// The only compatibility seam between the new 0.0.9B framework and the
	// retained Code A M01 runtime.  Product-facing framework code consumes
	// these operations, never the historical runtime object directly.
	bool Is0909BRuntimeReady() const;
	bool HasRetired0909BDefaultWidget() const;
	bool Prepare0909BRun(Fdemo_map0909BRunStartResult& OutResult);
	bool Activate0909BM01World(FString& OutDiagnostic);
	bool Rollback0909BPreparedRun(FString& OutDiagnostic);
	/** Durable authority identity retained after a reversible technical rollback. */
	FGuid Get0909BRecoverableRunId() const;
	bool Get0909BProfileSnapshot(Fdemo_mapProfileSessionSnapshot& OutSnapshot, FString& OutDiagnostic) const;
	void Observe0909BConfirmedRun(
		const Fdemo_mapShanmenRunCorrelation& RunCorrelation,
		FString& OutDiagnostic);
	Fdemo_mapCombatImpactDeliveryResult DeliverResolvedPlayerImpact(
		const FShanmenBasicSwordImpactReceipt& Impact);
	Fdemo_mapCombatImpactDeliveryResult DeliverResolvedM01EnemyImpact(
		const FShanmenBasicSwordImpactReceipt& Impact,
		AActor* TargetEnemy);
	/** M01 claims primary attack input only after its canonical Run is active. */
	bool ShouldUseM01BasicSwordProductPath() const;
	/** Executes one real primary-input sweep without falling through to ApplyDamage. */
	Fdemo_mapBasicSwordProductExecutionResult ExecuteM01PlayerBasicSwordSweep(
		float AttackPower,
		const TArray<FHitResult>& WorldHits);
	/** M01 claims player overlap skills only after their canonical Run is active. */
	bool ShouldUseM01PlayerShapeSkillProductPath() const;
	Fdemo_mapPlayerShapeSkillExecutionResult ExecuteM01PlayerShapeSkill(
		Edemo_mapPlayerShapeSkillFamily Family,
		float RawDamage,
		const TArray<FOverlapResult>& WorldOverlaps,
		const FVector& ContactOrigin);
	/** M01 owns player projectile routing even while its coordinator is unready. */
	bool ShouldUseM01PlayerProjectileProductPath(
		const AActor* SourcePlayer) const;
	Fdemo_mapPlayerProjectileLaunchResult PrepareM01PlayerStraightProjectile(
		AActor* SourcePlayer,
		float RawDamage);
	Fdemo_mapPlayerProjectileImpactResult
	ExecuteM01PlayerStraightProjectileImpact(
		AActor* SourcePlayer,
		AActor* TargetEnemy,
		UPrimitiveComponent* TargetComponent,
		uint64 ActivationSequence,
		const FGuid& ExpectedActivationId,
		float RawDamage,
		const FVector& ImpactLocation,
		const FVector& ImpactNormal);
	/** Attaches one externally spawned exact item to the active combat Run. */
	Fdemo_mapShanmenControlledWeaponHostAttachResult
	AttachControlledWeaponToActiveCombatRun(
		const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
		AActor* WeaponActor,
		UPrimitiveComponent* WeaponCollisionRoot,
		const Fdemo_mapShanmenControlledWeaponMotionCapture& Motion);
	/** Routes an input-independent exact-item intent through the active Run. */
	Fdemo_mapShanmenControlledWeaponRunCommandResult
	RouteControlledWeaponIntent(
		const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Intent);
	/** Routes one caller-timed, explicit threat sample through the active Run. */
	Fdemo_mapShanmenControlledWeaponThreatSampleResult
	RouteControlledWeaponThreatSampleIntent(
		const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Intent);
	/** Advances only the active Run's explicit, non-damaging Orbit poses. */
	bool AdvanceControlledWeaponOrbit(
		float DeltaSeconds,
		Fdemo_mapShanmenControlledWeaponHostOrbitBatch& OutBatch);
	/** One frame-owner pump; no fixed cadence or substep policy is implied. */
	Fdemo_mapShanmenControlledWeaponOrbitFrameResult
	AdvanceControlledWeaponOrbitFrame(float DeltaSeconds);
	const Fdemo_mapShanmenControlledWeaponRunHost&
	GetControlledWeaponRunHost() const
	{
		return ControlledWeaponRunHost;
	}
	const Fdemo_mapShanmenControlledWeaponRunCommandRouter&
	GetControlledWeaponRunCommandRouter() const
	{
		return ControlledWeaponRunCommandRouter;
	}
	const Fdemo_mapShanmenControlledWeaponThreatSampleRouter&
	GetControlledWeaponThreatSampleRouter() const
	{
		return ControlledWeaponThreatSampleRouter;
	}
	/** Routes one already-captured, device-independent hotbar trajectory. */
	Fdemo_mapShanmenThrownWeaponSessionResult RouteThrownWeaponHotbarIntent(
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent);
	/** Classifies one physical hotbar press before any transform/aim sampling. */
	Fdemo_mapShanmenThrownWeaponInputResult RouteThrownWeaponHotbarInput(
		int32 HotbarSlotNumber,
		AActor* SourceActor,
		TFunctionRef<FVector()> SampleAimDirection);
	/** Retries only the durable cancellation associated with this exact intent. */
	Fdemo_mapShanmenThrownWeaponSessionResult
	RecoverThrownWeaponCancellation(
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent);
	bool InterruptThrownWeaponFlight();
	bool ExpireThrownWeaponRange();
	const Fdemo_mapShanmenThrownWeaponProductLifecycle&
	GetThrownWeaponProductLifecycle() const
	{
		return ThrownWeaponProductLifecycle;
	}
	/** Sole product start entry from a device-independent direction intent. */
	Fdemo_mapShanmenSpiritEvasionProductRouteResult
	RouteSpiritEvasionStartIntent(const FVector& CandidateDirection);
	/** Acquires the sole active weapon-guard Host from caller-owned time. */
	Fdemo_mapShanmenWeaponGuardSessionStartResult
	RouteWeaponGuardStartIntent(
		const FGuid& TimelineId,
		int64 ActiveStartTick);
	/** Captures one opaque sample from the Run-bound 30 Hz guard timeline. */
	Fdemo_mapShanmenWeaponGuardInputTimelineSample
	CaptureWeaponGuardInputTimeline() const;
	/** Normal guard-input release; empty state is an accepted no-op. */
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult
	RouteWeaponGuardReleaseIntent();
	/** Sole typed terminal route for damage, equipment and lifecycle callers. */
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult
	RouteWeaponGuardTerminationIntent(
		Edemo_mapShanmenWeaponGuardTerminationReason Reason);
	const Fdemo_mapShanmenWeaponGuardProductSession&
	GetWeaponGuardProductSession() const
	{
		return WeaponGuardProductSession;
	}
	const Fdemo_mapShanmenWeaponGuardFixedTimeline&
	GetWeaponGuardFixedTimeline() const
	{
		return WeaponGuardFixedTimeline;
	}
	/** M01 enemy attacks never fall through to legacy damage when this is true. */
	bool ShouldUseM01EnemyAttackProductPath() const;
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyBasicMeleeStrike(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		float RawDamage);
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyMeleeDashContact(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		FName SkillProfileId,
		uint32 ActivationSerial,
		float RawDamage);
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyRangedProjectileImpact(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		FName SkillProfileId,
		uint64 ProjectileSequence,
		float RawDamage,
		const FVector& ImpactLocation,
		const FVector& ImpactNormal);
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyHeavySectorAttack(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		uint64 AttackSequence,
		float RawDamage);
	Fdemo_mapM01EnemyAttackExecutionResult ExecuteM01BossShapeAttack(
		AActor* SourceBoss,
		APawn* TargetPlayer,
		Edemo_mapM01BossAttack Attack,
		uint64 AttackSequence,
		float RawDamage);
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01BossVolleyProjectileImpact(
		AActor* SourceBoss,
		APawn* TargetPlayer,
		uint64 AttackSequence,
		int32 ProjectileOrdinal,
		float RawDamage,
		const FVector& ImpactLocation,
		const FVector& ImpactNormal);
	FString Get0909BProfileStorageRoot() const;
	bool Open0909BOutOfRaidInventory(FString& OutFeedback);
	void Set0909BOutOfRaidClosedCallback(TFunction<void()> InCallback);
	bool HasV3ProgressionFeature() const;
	bool IsM01ExpeditionMap() const;
	bool ActivateV3MissionContentForRun();
	void DeactivateV3MissionContentForPreparation();
	bool IsV3MissionContentActive() const { return bV3MissionContentActive; }
	void BindV3EnemyProjections(
		Ademo_mapEnemyCharacter* InMelee,
		Ademo_mapRangedEnemyCharacter* InRanged,
		Ademo_mapHeavyEnemyCharacter* InHeavy);
	bool IsM01ExtractionFoundationActive() const { return bM01ExtractionFoundationActive; }
	Fdemo_mapM01ExtractionSnapshot GetM01ExtractionSnapshot(Edemo_mapM01ExitType ExitType) const;
	FString GetM01ExtractionStatus(Edemo_mapM01ExitType ExitType) const;
	FString GetM01ExtractionHUDText() const;
	FString GetM01RiskHUDText() const;
	void SetM01RiskTier(FName RiskId);
	void ClearM01RiskTier(FName RiskId);
	void SetM01ExtractionInRange(Edemo_mapM01ExitType ExitType, bool bInRange);
	Fdemo_mapItemOperationResult RequestM01Extraction(Edemo_mapM01ExitType ExitType, APlayerController* Controller);
	bool NotifyM01BossDefeated(FName BossId);
	bool IsM01EnemyContentActive() const { return bM01EnemyContentActive; }
	int32 GetM01EnemyActorCount() const { return M01EnemyActors.Num(); }
	Ademo_mapM01BossCharacter* GetM01Boss() const { return M01Boss.Get(); }

private:
	void InitializeRuntimeMission();
	bool FindGroundLocation(const FVector& CandidateLocation, APawn* PlayerPawn, FVector& OutSpawnLocation) const;
	bool IsTargetLocationClear(const FVector& CandidateLocation, APawn* PlayerPawn) const;
	void SpawnMissionActors(APawn* PlayerPawn);
	bool SpawnMissionActorsFromEncounterMarkers(APawn* PlayerPawn);
	bool IsV2CMap() const;
	bool UsesPersistedEncounterMarkers() const;
	bool InitializeV3Progression(APawn* PlayerPawn, Udemo_mapItemSubsystem* Items);
	bool TryActivateCombatRun(APawn* PlayerPawn, FString& OutDiagnostic);
	bool ReleaseCombatProductRun(const TCHAR* Context);
	Fdemo_mapShanmenPlayerActionOccupancySnapshot
	CapturePlayerActionOccupancy() const;
	Fdemo_mapShanmenPlayerActionGateResult RoutePlayerActionGate(
		Edemo_mapShanmenPlayerActionKind RequestedAction);
	bool ReconcileWeaponGuardAuthorization(const TCHAR* Context);
	bool ReleasePlayerSpiritEvasion(const TCHAR* Context);
	Fdemo_mapM01EnemyAttackWeaponGuardContext
	CaptureM01EnemyAttackWeaponGuardContext();
	void PrepareV2CNavigation();
	void SpawnExit(APawn* PlayerPawn, const FVector& Forward);
	void SpawnEnemy(APawn* PlayerPawn, const FVector& Forward, const FVector& Right);
	void SpawnFriendly(APawn* PlayerPawn, const FVector& Forward, const FVector& Right);
	Udemo_mapPlayerHealthComponent* EnsurePlayerHealth(APawn* PlayerPawn) const;
	Udemo_mapSkillComponent* EnsurePlayerSkills(APawn* PlayerPawn);
	Udemo_mapShanmenSpiritEvasionComponent* EnsurePlayerSpiritEvasion(
		APawn* PlayerPawn);
	Udemo_mapAttributeComponent* EnsurePlayerAttributes(APawn* PlayerPawn);
	void StartRequestedAutomation();
	void BeginReset(const FString& StatusText, const TCHAR* LogMarker);
	void ReloadDemoLevel();
	bool InitializeM01ExtractionFoundation(APawn* PlayerPawn);
	void DestroyM01ExtractionFoundation();
	bool InitializeM01EnemyContent(APawn* PlayerPawn);
	void DestroyM01EnemyContent();
	void SuppressM01EnemyContent();
	void StartM01ExtractionVisibleSmoke();
	void RunM01IntegrationArtCaptureStep();
	void CompleteM01ExtractionVisibleSmoke(Edemo_mapM01ExitType CompletedExit);
	void StartM01EnemyVisibleSmoke();
	void RunM01EnemyVisibleSmokeStep();

	UFUNCTION()
	void HandlePlayerDefeated();

	UFUNCTION()
	void HandleM01PlayerDamaged(int32 AppliedDamage);

	UFUNCTION()
	void HandleTrainingTargetDestroyed(AActor* DestroyedActor);

	Ademo_mapTrainingTarget* GetTargetAt(int32 Index) const;
	Ademo_mapPlayerController* GetDemoPlayerController() const;
	APawn* GetDemoPawn() const;
	bool PlacePawnForTarget(Ademo_mapTrainingTarget* Target) const;
	void PlacePawnInExit(bool bInside) const;

	void StartT4Automation();
	void RunT4FirstAttack();
	void RunT4SecondAttack();
	void FinishT4Automation();
	void RunPackagedSmokeTest();
	Ademo_mapEnemyCharacter* GetEnemy() const;
	bool PlacePawnForEnemy(float Distance) const;

	void StartT5Automation();
	void VerifyLockedExit();
	void StartNextT5Target();
	void RunT5FirstAttack();
	void RunT5SecondAttack();
	void VerifyT5TargetDestroyed();
	void VerifyMissionCompletion();
	void ReenterExitForDuplicateCheck();
	void VerifyDuplicateCompletionProtection();

	void StartT7Automation();
	void VerifyT7Chase();
	void VerifyT7EnemyAttack();
	void VerifyT7EnemyCooldown();
	void RunT7EnemyKillAttack();
	void VerifyT7EnemyDefeated();
	void BeginT7PlayerDefeatCycle();
	void PollT7PlayerDefeat();
	void RunT7RVisibleInitial();
	void RunT7RVisibleDamaged();
	void PollT7RVisibleDefeat();
	void FinishT7RVisibleAcceptance();
	void EnterT7RVisibleExit();
	void CaptureT7RVisual(const FString& Filename) const;
	void StartV2AAutomation();
	void VerifyV2APlayerFiltering();
	void VerifyV2AEnemyFiltering();
	void RunV2AEnemyKillAttack();
	void VerifyV2AMissionIdentity();
	void FinishV2AMissionIdentity();
	void StartV2AVisibleAcceptance();
	void CaptureV2AInitialRelations();
	void CaptureV2AFriendlyFilter();
	void StartV2BAutomation();
	void RunV2BAutomationStep();
	void ScheduleNextV2BAutomationStep(float Delay);
	void StartV2BVisibleAcceptance();
	void RunV2BVisibleStep();
	void ScheduleNextV2BVisibleStep(float Delay);
	void StartV2CAutomation();
	void RunV2CAutomationStep();
	void ScheduleNextV2CAutomationStep(float Delay);
	void RunV2CMissionAttack();
	void PollV2CDefeat();
	void StartV2CVisibleAcceptance();
	void RunV2CVisibleStep();
	void ScheduleNextV2CVisibleStep(float Delay);
	void StartV2DAutomation();
	void RunV2DAutomationStep();
	void ScheduleNextV2DAutomationStep(float Delay);
	void RunV2DVariantKillAttack();
	void FinishV2DAutomationTargets();
	void StartV2DVisibleAcceptance();
	void RunV2DVisibleStep();
	void ScheduleNextV2DVisibleStep(float Delay);
	void StartV2EAutomation();
	void RunV2EAutomationStep();
	void ScheduleNextV2EAutomationStep(float Delay);
	void StartV2EVisibleAcceptance();
	void RunV2EVisibleStep();
	void ScheduleNextV2EVisibleStep(float Delay);
	void StartV2FinalAutomation();
	void RunV2FinalAutomationStep();
	void ScheduleNextV2FinalAutomationStep(float Delay);
	bool ValidateV2FinalResetState(const TCHAR* Context, int32& OutActorCount, int32& OutAIControllerCount);
	void StartV2FinalVisibleAcceptance();
	void RunV2FinalVisibleStep();
	void ScheduleNextV2FinalVisibleStep(float Delay);
	void StartV3AttributesGameplayAutomation();
	void RunV3AttributesGameplayAutomationStep();
	void ScheduleNextV3AttributesGameplayAutomationStep(float Delay);
	void StartV3ItemCoreGameplayAutomation();
	void RunV3ItemCoreGameplayAutomationStep();
	void ScheduleNextV3ItemCoreGameplayAutomationStep(float Delay);
	AActor* GetV2CActorWithTag(FName Tag) const;
	bool HasValidNavigationPath(const FVector& Start, const FVector& End) const;
	Ademo_mapEnemyCharacter* SpawnV2BTestEnemy(const FVector& Location);
	AActor* SpawnV2BBlockingWall(const FVector& Location);
	void MoveV2BActor(AActor* Actor, const FVector& Location) const;
	Udemo_mapSkillComponent* GetV2BSkills() const;

	void FailAutomation(const FString& FailureMessage);
	void FailPackagedSmokeTest(const FString& FailureMessage);

	TArray<TWeakObjectPtr<Ademo_mapTrainingTarget>> SpawnedTargets;
	TSet<const AActor*> CountedTargetActors;
	TWeakObjectPtr<Ademo_mapExitZone> ExitZone;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> Enemy;
	TWeakObjectPtr<Ademo_mapRangedEnemyCharacter> RangedEnemy;
	TWeakObjectPtr<Ademo_mapHeavyEnemyCharacter> HeavyEnemy;
	TWeakObjectPtr<Ademo_mapFriendlyUnit> FriendlyUnit;
	TWeakObjectPtr<Udemo_mapSkillComponent> PlayerSkillComponent;
	TWeakObjectPtr<Udemo_mapAttributeComponent> PlayerAttributeComponent;
	TWeakObjectPtr<Udemo_mapItemSubsystem> PlayerItemSubsystem;
	TWeakObjectPtr<Ademo_mapV3ProgressionManager> V3ProgressionManager;
	TWeakObjectPtr<Ademo_map0909BFrameworkHost> Framework0909BHost;
	TOptional<Fdemo_mapShanmenRunCorrelation> Prepared0909BRunCorrelation;
	Fdemo_mapCombatRunCoordinator CombatRunCoordinator;
	Fdemo_mapShanmenControlledWeaponRunHost ControlledWeaponRunHost;
	Fdemo_mapShanmenControlledWeaponRunCommandRouter
		ControlledWeaponRunCommandRouter;
	Fdemo_mapShanmenControlledWeaponThreatSampleRouter
		ControlledWeaponThreatSampleRouter;
	Fdemo_mapShanmenThrownWeaponProductLifecycle
		ThrownWeaponProductLifecycle;
	Fdemo_mapShanmenThrownWeaponInputAdapter ThrownWeaponInputAdapter;
	Fdemo_mapShanmenWeaponGuardFixedTimeline WeaponGuardFixedTimeline;
	Fdemo_mapShanmenWeaponGuardProductSession WeaponGuardProductSession;
	TArray<TWeakObjectPtr<Ademo_mapM01ExtractionZone>> M01ExtractionZones;
	TArray<TWeakObjectPtr<AActor>> M01EnemyActors;
	TWeakObjectPtr<Ademo_mapM01BossCharacter> M01Boss;
	Fdemo_mapM01ExtractionAuthority M01ExtractionAuthority;
	Fdemo_mapM01RunEnemyLedger M01RunEnemyLedger;
	FTimerHandle InitializationTimerHandle;
	FTimerHandle AutomationTimerHandle;
	bool bRuntimeMissionInitialized = false;
	bool bV3MissionContentActive = false;
	bool bM01ExtractionFoundationActive = false;
	bool bM01EnemyContentActive = false;
	FName ActiveM01RiskId = NAME_None;
	bool bM01ExtractionVisibleSmokeRequested = false;
	bool bM01IntegrationVisibleSmokeRequested = false;
	bool bM01EnemyVisibleSmokeRequested = false;
	int32 M01ExtractionVisibleSmokePhase = 0;
	int32 M01IntegrationArtCaptureStep = 0;
	bool bM01IntegrationArtCapturePending = false;
	float M01IntegrationOriginalCameraArmLength = 800.0f;
	int32 M01EnemyVisibleSmokeStep = 0;
	bool bT4AutomationRequested = false;
	bool bT5AutomationRequested = false;
	bool bT7AutomationRequested = false;
	bool bPackagedSmokeTestRequested = false;
	bool bT7RVisibleAcceptanceRequested = false;
	bool bV2AAutomationRequested = false;
	bool bV2AVisibleAcceptanceRequested = false;
	bool bV2BAutomationRequested = false;
	bool bV2BVisibleAcceptanceRequested = false;
	bool bV2CAutomationRequested = false;
	bool bV2CVisibleAcceptanceRequested = false;
	bool bV2DAutomationRequested = false;
	bool bV2DVisibleAcceptanceRequested = false;
	bool bV2EAutomationRequested = false;
	bool bV2EVisibleAcceptanceRequested = false;
	bool bV2FinalAutomationRequested = false;
	bool bV2FinalVisibleAcceptanceRequested = false;
	bool bV3AttributesGameplayAutomationRequested = false;
	bool bV3ItemCoreGameplayAutomationRequested = false;
	bool bV3WorldInteractionAutomationRequested = false;
	bool bV3InventoryUIAutomationRequested = false;
	bool bV3WorldUIVisibleAcceptanceRequested = false;
	int32 AutomationTargetIndex = 0;
	int32 T7EnemyAttackIndex = 0;
	float T7InitialEnemyDistance = 0.0f;
	float T7DefeatDeadline = 0.0f;
	FTimerHandle ResetTimerHandle;
	bool bResetPending = false;
	FString EndStatusText;
	FString T7RVisualOutputDirectory;
	int32 V2AEnemyAttackCount = 0;
	int32 V2BAutomationStep = 0;
	int32 V2BVisibleStep = 0;
	int32 V2CAutomationStep = 0;
	int32 V2CVisibleStep = 0;
	int32 V2CMissionTargetIndex = 0;
	int32 V2CMissionAttackIndex = 0;
	float V2CInitialEnemyDistance = 0.0f;
	float V2CDefeatDeadline = 0.0f;
	int32 V2DAutomationStep = 0;
	int32 V2DVisibleStep = 0;
	float V2DInitialDistance = 0.0f;
	int32 V2DInitialPlayerHealth = 0;
	TWeakObjectPtr<AActor> V2DBlockingWall;
	TWeakObjectPtr<AActor> V2EBlockingWall;
	TWeakObjectPtr<Ademo_mapSkillProjectile> V2ETestProjectile;
	TArray<TWeakObjectPtr<AActor>> V2EFallbackWalls;
	int32 V2EAutomationStep = 0;
	int32 V2EVisibleStep = 0;
	int32 V2EInitialHealth = 0;
	int32 V2EInitialFireCount = 0;
	int32 V2EInitialRetreatMoves = 0;
	int32 V2EInitialFallbackCount = 0;
	int32 V2EInitialRejectCount = 0;
	float V2EInitialDistance = 0.0f;
	float V2EOriginalCameraArmLength = 800.0f;
	int32 V2FinalAutomationStep = 0;
	int32 V2FinalVisibleStep = 0;
	int32 V2FinalInitialHealth = 0;
	int32 V2FinalInitialEnemyHealth = 0;
	int32 V2FinalInitialRangedHealth = 0;
	int32 V2FinalInitialHeavyHealth = 0;
	int32 V2FinalInitialMissionCount = 0;
	int32 V2FinalInitialRangedShots = 0;
	int32 V2FinalInitialHeavyResolves = 0;
	int32 V2FinalProjectilePeak = 0;
	float V2FinalInitialDistance = 0.0f;
	float V2FinalLoadSeconds = 0.0f;
	float V2FinalNavigationSeconds = 0.0f;
	TWeakObjectPtr<AActor> V2FinalBlockingWall;
	int32 V3AttributesAutomationStep = 0;
	Fdemo_mapModifierHandle V3AttackModifierHandle;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V3AttributeTestTarget;
	int32 V3ItemAutomationStep = 0;
	FGuid V3ItemWeaponId;
	FGuid V3ItemArmorId;
	FGuid V3ItemAccessoryId;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V3ItemTestTarget;
	int32 V2DVariantKillTarget = 0;
	int32 V2DVariantAttackCount = 0;
	FVector V2BBaseLocation = FVector::ZeroVector;
	FVector V2BProjectileStartLocation = FVector::ZeroVector;
	TArray<TWeakObjectPtr<Ademo_mapEnemyCharacter>> V2BTestEnemies;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2BBoundaryEnemy;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2BOutsideAngleEnemy;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2BBehindEnemy;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2BHighEnemy;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2BWallEnemy;
	TWeakObjectPtr<AActor> V2BBlockingWall;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2COpenLaneEnemy;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2CBlockedEnemy;
	TWeakObjectPtr<Ademo_mapEnemyCharacter> V2CSkillEnemy;
};
