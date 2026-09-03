#include "demo_mapGameMode.h"
#include "demo_map.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapExitZone.h"
#include "demo_mapGameState.h"
#include "demo_mapHUD.h"
#include "demo_mapPlayerController.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapTrainingTarget.h"
#include "demo_mapFriendlyUnit.h"
#include "demo_mapFactionComponent.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapSkillComponent.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapSkillProjectile.h"
#include "demo_mapEncounterMarker.h"
#include "demo_mapGrayboxBlock.h"
#include "demo_mapV3ProgressionMarker.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_map0909BFramework.h"
#include "demo_mapProfilePreparationFlow.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapM01ExtractionZone.h"
#include "demo_mapM01Marker.h"
#include "demo_mapM01GrayboxBlock.h"
#include "demo_mapM01RiskZone.h"
#include "demo_mapM01BossCharacter.h"
#include "demo_mapM01ArtProp.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenSpiritEvasionComponent.h"
#include "demo_mapShanmenThrownWeaponProjectile.h"
#include "demo_mapShanmenSwordQiProjectile.h"
#include "demo_mapLootChest.h"
#include "demo_mapCorpseContainerActor.h"
#include "Algo/AllOf.h"
#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

#if !UE_BUILD_SHIPPING
namespace
{
	bool GT7DefeatCyclePending = false;
	int32 GT7RVisiblePhase = 0;
	int32 GV2BAutomationPhase = 0;
	int32 GV2CAutomationPhase = 0;
	int32 GV2DAutomationPhase = 0;
	int32 GV2EAutomationPhase = 0;
	int32 GV2FinalAutomationPhase = 0;
	int32 GV2FinalVisiblePhase = 0;
	int32 GV2FinalInitialActorCount = 0;
	int32 GV2FinalInitialAIControllerCount = 0;
	int32 GV2FinalResetActorCounts[3] = { 0, 0, 0 };
	double GV2FinalProcessStartSeconds = FPlatformTime::Seconds();
	float GV2FinalMeasuredLoadSeconds = 0.0f;
	float GV2FinalMeasuredNavigationSeconds = 0.0f;
}
#endif

Ademo_mapGameMode::Ademo_mapGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<APawn> TopDownCharacterClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (TopDownCharacterClass.Succeeded())
	{
		DefaultPawnClass = TopDownCharacterClass.Class;
	}

	PlayerControllerClass = Ademo_mapPlayerController::StaticClass();
	GameStateClass = Ademo_mapGameState::StaticClass();
	HUDClass = Ademo_mapHUD::StaticClass();

#if !UE_BUILD_SHIPPING
	bT4AutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("T4Automation"));
	bT5AutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("T5Automation"));
	bT7AutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("T7Automation"));
	bPackagedSmokeTestRequested = FParse::Param(FCommandLine::Get(), TEXT("PackagedSmokeTest"));
	bT7RVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("T7RVisibleAcceptance"));
	bV2AAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V2AAutomation"));
	bV2AVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V2AVisibleAcceptance"));
	bV2BAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V2BAutomation"));
	bV2BVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V2BVisibleAcceptance"));
	bV2CAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V2CAutomation"));
	bV2CVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V2CVisibleAcceptance"));
	bV2DAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V2DAutomation"));
	bV2DVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V2DVisibleAcceptance"));
	bV2EAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V2EAutomation"));
	bV2EVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V2EVisibleAcceptance"));
	bV2FinalAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V2FinalAutomation"));
	bV2FinalVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V2FinalVisibleAcceptance"));
	bV3AttributesGameplayAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V3AttributesGameplayAutomation"));
	bV3ItemCoreGameplayAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V3ItemCoreGameplayAutomation"));
	bV3WorldInteractionAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V3WorldInteractionAutomation"));
	bV3InventoryUIAutomationRequested = FParse::Param(FCommandLine::Get(), TEXT("V3InventoryUIAutomation"));
	bV3WorldUIVisibleAcceptanceRequested = FParse::Param(FCommandLine::Get(), TEXT("V3WorldUIVisibleAcceptance"));
	bM01IntegrationVisibleSmokeRequested = FParse::Param(FCommandLine::Get(), TEXT("M01IntegrationVisibleSmoke"));
	bM01ExtractionVisibleSmokeRequested = bM01IntegrationVisibleSmokeRequested
		|| FParse::Param(FCommandLine::Get(), TEXT("M01ExtractionVisibleSmoke"));
	bM01EnemyVisibleSmokeRequested = FParse::Param(FCommandLine::Get(), TEXT("M01EnemyVisibleSmoke"));
	FParse::Value(FCommandLine::Get(), TEXT("T7RVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V2AVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V2BVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V2CVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V2DVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V2EVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V2FinalVisualOutput="), T7RVisualOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("M01VisualOutput="), T7RVisualOutputDirectory);
#endif
}

bool Ademo_mapGameMode::Is0909BRuntimeReady() const
{
	return V3ProgressionManager.IsValid()
		&& V3ProgressionManager->IsInitialized();
}

bool Ademo_mapGameMode::HasRetired0909BDefaultWidget() const
{
	return V3ProgressionManager.IsValid()
		&& V3ProgressionManager->GetSectNavigationWidget() != nullptr;
}

bool Ademo_mapGameMode::Prepare0909BRun(
	Fdemo_map0909BRunStartResult& OutResult)
{
	OutResult = Fdemo_map0909BRunStartResult();
	Prepared0909BRunCorrelation.Reset();
	if (!Is0909BRuntimeReady())
	{
		OutResult.Diagnostic = TEXT("The retained M01 runtime is not initialized.");
		return false;
	}

	const Fdemo_mapProfileSessionBeginResult Begin =
		V3ProgressionManager->BeginPreparedProfileRunFor0909B();
	OutResult.OwnerId = Begin.Snapshot.ProfileId;
	OutResult.RunInstanceId = Begin.Snapshot.ActiveRunId;
	OutResult.Diagnostic = Begin.Diagnostic;
	OutResult.bRunActive = Begin.IsRunActive()
		&& OutResult.OwnerId.IsValid() && OutResult.RunInstanceId.IsValid();
	if (OutResult.bRunActive)
	{
		const Fdemo_mapProfilePreparationFlow* Flow =
			V3ProgressionManager->GetProfilePreparationFlow();
		FString CorrelationDiagnostic;
		if (!Flow || !Flow->TryGetActiveShanmenRunCorrelation(
				OutResult.RunCorrelation, &CorrelationDiagnostic)
			|| OutResult.RunCorrelation.OwnerId != OutResult.OwnerId
			|| OutResult.RunCorrelation.ActiveRunId != OutResult.RunInstanceId)
		{
			OutResult.bRunActive = false;
			OutResult.Diagnostic = CorrelationDiagnostic.IsEmpty()
				? TEXT("Prepared Run lacks matching Shanmen authority correlation.")
				: CorrelationDiagnostic;
			return false;
		}
		Prepared0909BRunCorrelation = OutResult.RunCorrelation;
	}
	return OutResult.bRunActive;
}

bool Ademo_mapGameMode::Activate0909BM01World(FString& OutDiagnostic)
{
	if (!Is0909BRuntimeReady())
	{
		OutDiagnostic = TEXT("The retained M01 runtime is not initialized.");
		return false;
	}
	return V3ProgressionManager->ActivatePreparedProfileWorldFor0909B(OutDiagnostic);
}

bool Ademo_mapGameMode::Rollback0909BPreparedRun(FString& OutDiagnostic)
{
	if (!Is0909BRuntimeReady())
	{
		OutDiagnostic = TEXT("The retained M01 runtime is not initialized.");
		return false;
	}
	const bool bRolledBack =
		V3ProgressionManager->RollbackPreparedProfileRunFor0909B(OutDiagnostic);
	if (bRolledBack)
	{
		Prepared0909BRunCorrelation.Reset();
	}
	return bRolledBack;
}

FGuid Ademo_mapGameMode::Get0909BRecoverableRunId() const
{
	const Fdemo_mapProfilePreparationFlow* Flow =
		V3ProgressionManager.IsValid()
			? V3ProgressionManager->GetProfilePreparationFlow()
			: nullptr;
	return Flow ? Flow->GetRecoverableShanmenRunId() : FGuid();
}

bool Ademo_mapGameMode::Get0909BProfileSnapshot(
	Fdemo_mapProfileSessionSnapshot& OutSnapshot,
	FString& OutDiagnostic) const
{
	OutSnapshot = Fdemo_mapProfileSessionSnapshot();
	OutDiagnostic.Reset();
	if (!Is0909BRuntimeReady())
	{
		OutDiagnostic = TEXT("The retained Profile adapter is not initialized.");
		return false;
	}
	const Fdemo_mapProfilePreparationFlow* Flow = V3ProgressionManager->GetProfilePreparationFlow();
	if (!Flow || !Flow->GetSession())
	{
		OutDiagnostic = TEXT("The retained Profile adapter has no readable P5 session.");
		return false;
	}
	OutSnapshot = Flow->GetPresentationSnapshot();
	if (!OutSnapshot.ProfileId.IsValid())
	{
		OutDiagnostic = TEXT("The retained Profile adapter returned an invalid OwnerId.");
		return false;
	}
	return true;
}

void Ademo_mapGameMode::Observe0909BConfirmedRun(
	const Fdemo_mapShanmenRunCorrelation& RunCorrelation,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Is0909BRuntimeReady() || !RunCorrelation.IsValid()
		|| !Prepared0909BRunCorrelation.IsSet()
		|| Prepared0909BRunCorrelation.GetValue() != RunCorrelation)
	{
		OutDiagnostic = TEXT("RejectedMissingPreparedIdentity");
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_RUN_AUTHORITY Event=RejectedMissingPreparedIdentity %s"),
			*RunCorrelation.ToLogString());
		return;
	}

	const Fdemo_mapProfilePreparationFlow* Flow =
		V3ProgressionManager->GetProfilePreparationFlow();
	Fdemo_mapShanmenRunCorrelation Current;
	FString Diagnostic;
	if (!Flow || !Flow->TryGetActiveShanmenRunCorrelation(
			Current, &Diagnostic) || Current != RunCorrelation)
	{
		OutDiagnostic = Diagnostic.IsEmpty()
			? TEXT("RejectedAuthorityCorrelationChanged") : Diagnostic;
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_RUN_AUTHORITY Event=WorldConfirmationRejected Expected={%s} Actual={%s} Diagnostic=%s"),
			*RunCorrelation.ToLogString(), *Current.ToLogString(),
			*OutDiagnostic);
		return;
	}
	OutDiagnostic =
		TEXT("World confirmation matched the durable ShanmenItems Run correlation; no Code B write was issued.");
	UE_LOG(Logdemo_map, Log,
		TEXT("0_0_10_RUN_AUTHORITY Event=WorldConfirmedNoLegacyWrite %s"),
		*RunCorrelation.ToLogString());
}

Fdemo_mapCombatImpactDeliveryResult
Ademo_mapGameMode::DeliverResolvedPlayerImpact(
	const FShanmenBasicSwordImpactReceipt& Impact)
{
	return CombatRunCoordinator.DeliverBasicSwordImpactToPlayer(Impact);
}

Fdemo_mapCombatImpactDeliveryResult
Ademo_mapGameMode::DeliverResolvedM01EnemyImpact(
	const FShanmenBasicSwordImpactReceipt& Impact,
	AActor* TargetEnemy)
{
	return CombatRunCoordinator.DeliverBasicSwordImpactToM01Enemy(
		Impact,
		TargetEnemy);
}

Fdemo_mapShanmenPlayerActionOccupancySnapshot
Ademo_mapGameMode::CapturePlayerActionOccupancy() const
{
	Fdemo_mapShanmenPlayerActionOccupancySnapshot Snapshot;
	if (!WeaponGuardProductSession.IsValid())
	{
		Snapshot.Invalidate();
		return Snapshot;
	}
	if (WeaponGuardProductSession.HasActive())
	{
		const Fdemo_mapShanmenWeaponGuardProductHost* Host =
			WeaponGuardProductSession.GetActiveHost();
		if (Host == nullptr
			|| !Snapshot.TryRegisterClaim(
				Edemo_mapShanmenPlayerActionKind::WeaponGuard,
				Host->GetHostId(),
				Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner))
		{
			Snapshot.Invalidate();
			return Snapshot;
		}
	}
	if (!ThrownWeaponProductLifecycle.IsValid())
	{
		Snapshot.Invalidate();
		return Snapshot;
	}
	if (!SwordQiProductController.TryAppendOccupancy(Snapshot))
	{
		Snapshot.Invalidate();
		return Snapshot;
	}
	const bool bThrownWeaponInFlight =
		ThrownWeaponProductLifecycle.IsActive()
		&& ThrownWeaponProductLifecycle.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::InFlight;
	if (bThrownWeaponInFlight
		&& !Snapshot.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			ThrownWeaponProductLifecycle.GetOccupancyOwnerId(),
			Edemo_mapShanmenPlayerActionClaimPreemption::None))
	{
		Snapshot.Invalidate();
		return Snapshot;
	}

	const ACharacter* PlayerCharacter = Cast<ACharacter>(GetDemoPawn());
	const Udemo_mapShanmenSpiritEvasionComponent* SpiritComponent =
		PlayerCharacter
			? PlayerCharacter->FindComponentByClass<
				Udemo_mapShanmenSpiritEvasionComponent>()
			: nullptr;
	if (SpiritComponent && !SpiritComponent->CanStart())
	{
		if (!SpiritComponent->HasHost()
			|| !SpiritComponent->GetHost().IsValid()
			|| !Snapshot.TryRegisterClaim(
				Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
				SpiritComponent->GetHost().GetHostId(),
				Edemo_mapShanmenPlayerActionClaimPreemption::None))
		{
			Snapshot.Invalidate();
		}
	}
	return Snapshot;
}

Fdemo_mapShanmenPlayerActionGateResult
Ademo_mapGameMode::RoutePlayerActionGate(
	const Edemo_mapShanmenPlayerActionKind RequestedAction)
{
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt Arbitration =
		CombatRunCoordinator.TryAuthorizePlayerAction(
			RequestedAction,
			CapturePlayerActionOccupancy());
	if (!Arbitration.RequiresWeaponGuardPreemption())
	{
		const Fdemo_mapShanmenPlayerActionGateResult Result =
			Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
				Arbitration);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("0_0_10_PLAYER_ACTION Event=Arbitrated Action=%d Status=%d Error=%d Sequence=%llu CommandId=%s Diagnostic=%s"),
			static_cast<int32>(RequestedAction),
			static_cast<int32>(Arbitration.Status),
			static_cast<int32>(Arbitration.Error),
			static_cast<unsigned long long>(Arbitration.CommandSequence),
			*Arbitration.CommandId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.Diagnostic);
		return Result;
	}

	const FGuid ExpectedHostId = Arbitration.OccupyingOwnerId;
	const Fdemo_mapShanmenWeaponGuardSessionTransitionResult Transition =
		RouteWeaponGuardTerminationIntent(
			Edemo_mapShanmenWeaponGuardTerminationReason::
				PlayerActionPreempted);
	if (!Transition.IsSuccess()
		|| Transition.Status
			!= Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Interrupted
		|| Transition.HostId != ExpectedHostId
		|| !WeaponGuardProductSession.IsEmpty())
	{
		return Fdemo_mapShanmenPlayerActionGateResult::
			RejectGuardPreemption(
				Arbitration,
				TEXT("The exact weapon-guard Host rejected typed player-action preemption."));
	}
	return Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
		Arbitration,
		Transition.HostId,
		CapturePlayerActionOccupancy());
}

bool Ademo_mapGameMode::ShouldUseM01BasicSwordProductPath() const
{
	// M01 never falls back to the legacy damage writer. Before its Combat Run
	// is ready, input is rejected by the product execution gate instead.
	return IsM01ExpeditionMap();
}

Fdemo_mapBasicSwordProductExecutionResult
Ademo_mapGameMode::ExecuteM01PlayerBasicSwordSweep(
	float AttackPower,
	const TArray<FHitResult>& WorldHits)
{
	Fdemo_mapBasicSwordProductExecutionResult Result;
	if (!ShouldUseM01BasicSwordProductPath()
		|| !CombatRunCoordinator.IsReady()
		|| !PlayerItemSubsystem.IsValid())
	{
		return Result;
	}

	const FGuid WeaponInstanceId = PlayerItemSubsystem->GetAuthority()
		.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot);
	FShanmenBasicSwordOffenseSnapshot Offense;
	if (!WeaponInstanceId.IsValid())
	{
		Result.Error =
			Edemo_mapBasicSwordProductExecutionError::InvalidSourceItem;
		return Result;
	}
	if (!FShanmenBasicSwordOffenseSnapshot::TryCapture(
			AttackPower,
			Offense))
	{
		Result.Error = Edemo_mapBasicSwordProductExecutionError::InvalidOffense;
		return Result;
	}
	Result.ActionGate = RoutePlayerActionGate(
		Edemo_mapShanmenPlayerActionKind::BasicSword);
	if (!Result.ActionGate.IsAuthorized())
	{
		Result.Error = Edemo_mapBasicSwordProductExecutionError::ActionConflict;
		return Result;
	}
	const Fdemo_mapShanmenPlayerActionGateResult ActionGate =
		Result.ActionGate;
	Result = CombatRunCoordinator.ExecutePlayerBasicSwordSweep(
		WeaponInstanceId,
		AttackPower,
		WorldHits);
	Result.ActionGate = ActionGate;
	if (Result.IsExecuted())
	{
		Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
		FShanmenSwordRhythmReceipt RhythmReceipt;
		FShanmenSwordRhythmContributionBindingReceipt BindingReceipt;
		FShanmenSwordRhythmEvaluationInput EvaluationInput;
		FString RhythmDiagnostic;
		bool bRhythmObserved = false;
		if (!CombatRunFixedTimeline.TryCapture(TimelineSample))
		{
			RhythmDiagnostic =
				TEXT("Canonical Run timeline could not capture the completed BasicSword action.");
		}
		else
		{
			bRhythmObserved =
				SwordRhythmProductSession.TryObserveExecutedBasicSword(
					Result,
					TimelineSample,
					RhythmReceipt,
					BindingReceipt,
					EvaluationInput,
					RhythmDiagnostic);
		}
		if (bRhythmObserved)
		{
			const auto& PresentationState =
				SwordRhythmProductSession.GetPresentationState();
			UE_LOG(Logdemo_map,
				Log,
				TEXT("0_0_10_SWORD_RHYTHM Event=Observed ActivationId=%s ReceiptId=%s PresentationStateId=%s EvaluationInputId=%s BindingReceiptId=%s Contributions=%d Revision=%d InputTick=%lld Band=%d PreviousCount=%d ResultingCount=%d"),
				*Result.ActivationId.ToString(
					EGuidFormats::DigitsWithHyphens),
				*RhythmReceipt.GetReceiptId().ToString(
					EGuidFormats::DigitsWithHyphens),
				*PresentationState.GetPresentationStateId().ToString(
					EGuidFormats::DigitsWithHyphens),
				*EvaluationInput.GetInputId().ToString(
					EGuidFormats::DigitsWithHyphens),
				*BindingReceipt.GetReceiptId().ToString(
					EGuidFormats::DigitsWithHyphens),
				EvaluationInput.NumContributions(),
				PresentationState.GetObservationRevision(),
				static_cast<long long>(
					RhythmReceipt.GetCurrentObservation().GetInputTick()),
				static_cast<int32>(RhythmReceipt.GetBand()),
				RhythmReceipt.GetPreviousChainCount(),
				RhythmReceipt.GetResultingChainCount());
			PublishCurrentSwordRhythmPresentation(Result.ActivationId);
		}
		else
		{
			UE_LOG(Logdemo_map,
				Error,
				TEXT("0_0_10_SWORD_RHYTHM Event=ObservationRejected ActivationId=%s Diagnostic=%s"),
				*Result.ActivationId.ToString(
					EGuidFormats::DigitsWithHyphens),
				*RhythmDiagnostic);
		}
	}
	UE_LOG(Logdemo_map,
		Log,
		TEXT("0_0_10_BASIC_SWORD Event=ProductSweep Error=%d ActivationId=%s Contacts=%d Candidates=%d Delivered=%d Committed=%d Replayed=%d"),
		static_cast<int32>(Result.Error),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		Result.WorldContactCount,
		Result.ResolvedCandidateCount,
		Result.DeliveredImpactCount,
		Result.CommittedImpactCount,
		Result.AlreadyCommittedImpactCount);
	return Result;
}

bool Ademo_mapGameMode::TryGetSwordRhythmPresentationState(
	Fdemo_mapShanmenSwordRhythmPresentationState& OutState) const
{
	OutState = Fdemo_mapShanmenSwordRhythmPresentationState();
	if (!SwordRhythmProductSession.IsValid()
		|| SwordRhythmProductSession.IsEmpty()
		|| !SwordRhythmProductSession.GetPresentationState().IsValid())
	{
		return false;
	}
	OutState = SwordRhythmProductSession.GetPresentationState();
	return true;
}

bool Ademo_mapGameMode::TryGetMeridianShockStatus(
	Fdemo_mapShanmenCombatConditionStatusSnapshot& OutStatus) const
{
	OutStatus = Fdemo_mapShanmenCombatConditionStatusSnapshot();
	return PlayerCombatConditionComponent.IsValid()
		&& PlayerCombatConditionComponent->TryCaptureMeridianShockStatus(
			OutStatus);
}

bool Ademo_mapGameMode::TryGetSwordRhythmPresentationEvent(
	Fdemo_mapShanmenSwordRhythmPresentationEvent& OutEvent) const
{
	OutEvent = Fdemo_mapShanmenSwordRhythmPresentationEvent();
	Fdemo_mapShanmenSwordRhythmPresentationState State;
	if (!TryGetSwordRhythmPresentationState(State))
	{
		return false;
	}
	const auto Adapted =
		Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt(State);
	if (!Adapted.IsAdapted())
	{
		return false;
	}
	OutEvent = Adapted.Event;
	return true;
}

void Ademo_mapGameMode::PublishCurrentSwordRhythmPresentation(
	const FGuid& ActivationId)
{
	const auto Published =
		SwordRhythmPresentationRunController.TryPublishCurrent(
			SwordRhythmProductSession);
	if (!Published.IsSuccess())
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=PresentationHandoffRejected ActivationId=%s Status=%d Revision=%d Published=%d Queued=%d Diagnostic=%s"),
			*ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
			static_cast<int32>(Published.Status),
			Published.ObservationRevision,
			Published.PublishedDispatchCount,
			Published.QueuedDispatchCount,
			*Published.Diagnostic);
		return;
	}
	UE_LOG(Logdemo_map,
		Log,
		TEXT("0_0_10_SWORD_RHYTHM Event=PresentationHandoffCaptured ActivationId=%s Status=%d Revision=%d Published=%d Queued=%d VisualPending=%d AudioPending=%d"),
		*ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		static_cast<int32>(Published.Status),
		Published.ObservationRevision,
		Published.PublishedDispatchCount,
		Published.QueuedDispatchCount,
		SwordRhythmPresentationRunController.HasPendingVisualHandoff()
			? 1
			: 0,
		SwordRhythmPresentationRunController.HasPendingAudioHandoff()
			? 1
			: 0);
}

bool Ademo_mapGameMode::TryGetSwordRhythmVisualHandoff(
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff) const
{
	return SwordRhythmPresentationRunController.
		TryGetPendingVisualHandoff(OutHandoff);
}

bool Ademo_mapGameMode::TryGetSwordRhythmAudioHandoff(
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff) const
{
	return SwordRhythmPresentationRunController.
		TryGetPendingAudioHandoff(OutHandoff);
}

bool Ademo_mapGameMode::AcknowledgeSwordRhythmVisualHandoff(
	const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement&
		Acknowledgement,
	FString& OutDiagnostic)
{
	const bool bAcknowledged = SwordRhythmPresentationRunController.
		TryAcknowledgeVisualHandoff(Acknowledgement, OutDiagnostic);
	if (bAcknowledged)
	{
		UE_LOG(Logdemo_map,
			Log,
			TEXT("0_0_10_SWORD_RHYTHM Event=VisualHandoffAcknowledge AcknowledgementId=%s HandoffId=%s Accepted=1 Published=%d Queued=%d Diagnostic=%s"),
			*Acknowledgement.GetAcknowledgementId().ToString(
				EGuidFormats::DigitsWithHyphens),
			*Acknowledgement.GetHandoffId().ToString(
				EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=VisualHandoffAcknowledge AcknowledgementId=%s HandoffId=%s Accepted=0 Published=%d Queued=%d Diagnostic=%s"),
			*Acknowledgement.GetAcknowledgementId().ToString(
				EGuidFormats::DigitsWithHyphens),
			*Acknowledgement.GetHandoffId().ToString(
				EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	return bAcknowledged;
}

bool Ademo_mapGameMode::AcknowledgeSwordRhythmAudioHandoff(
	const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement&
		Acknowledgement,
	FString& OutDiagnostic)
{
	const bool bAcknowledged = SwordRhythmPresentationRunController.
		TryAcknowledgeAudioHandoff(Acknowledgement, OutDiagnostic);
	if (bAcknowledged)
	{
		UE_LOG(Logdemo_map,
			Log,
			TEXT("0_0_10_SWORD_RHYTHM Event=AudioHandoffAcknowledge AcknowledgementId=%s HandoffId=%s Accepted=1 Published=%d Queued=%d Diagnostic=%s"),
			*Acknowledgement.GetAcknowledgementId().ToString(
				EGuidFormats::DigitsWithHyphens),
			*Acknowledgement.GetHandoffId().ToString(
				EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=AudioHandoffAcknowledge AcknowledgementId=%s HandoffId=%s Accepted=0 Published=%d Queued=%d Diagnostic=%s"),
			*Acknowledgement.GetAcknowledgementId().ToString(
				EGuidFormats::DigitsWithHyphens),
			*Acknowledgement.GetHandoffId().ToString(
				EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	return bAcknowledged;
}

bool Ademo_mapGameMode::ConsumeSwordRhythmVisualHandoff(
	const FGuid HandoffId,
	FString& OutDiagnostic)
{
	const bool bConsumed =
		SwordRhythmPresentationRunController.TryConsumeVisualHandoff(
			HandoffId, OutDiagnostic);
	if (bConsumed)
	{
		UE_LOG(Logdemo_map,
			Log,
			TEXT("0_0_10_SWORD_RHYTHM Event=VisualHandoffConsume HandoffId=%s Consumed=1 Published=%d Queued=%d Diagnostic=%s"),
			*HandoffId.ToString(EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=VisualHandoffConsume HandoffId=%s Consumed=0 Published=%d Queued=%d Diagnostic=%s"),
			*HandoffId.ToString(EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	return bConsumed;
}

bool Ademo_mapGameMode::ConsumeSwordRhythmAudioHandoff(
	const FGuid HandoffId,
	FString& OutDiagnostic)
{
	const bool bConsumed =
		SwordRhythmPresentationRunController.TryConsumeAudioHandoff(
			HandoffId, OutDiagnostic);
	if (bConsumed)
	{
		UE_LOG(Logdemo_map,
			Log,
			TEXT("0_0_10_SWORD_RHYTHM Event=AudioHandoffConsume HandoffId=%s Consumed=1 Published=%d Queued=%d Diagnostic=%s"),
			*HandoffId.ToString(EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=AudioHandoffConsume HandoffId=%s Consumed=0 Published=%d Queued=%d Diagnostic=%s"),
			*HandoffId.ToString(EGuidFormats::DigitsWithHyphens),
			SwordRhythmPresentationRunController.GetPublishedDispatchCount(),
			SwordRhythmPresentationRunController.GetQueuedDispatchCount(),
			*OutDiagnostic);
	}
	return bConsumed;
}

bool Ademo_mapGameMode::ShouldUseM01PlayerShapeSkillProductPath() const
{
	// M01 owns the routing decision even before the coordinator is ready. A
	// canonical failure must never fall through to the retained V2 damage path.
	return IsM01ExpeditionMap();
}

Fdemo_mapPlayerShapeSkillExecutionResult
Ademo_mapGameMode::ExecuteM01PlayerShapeSkill(
	Edemo_mapPlayerShapeSkillFamily Family,
	float RawDamage,
	const TArray<FOverlapResult>& WorldOverlaps,
	const FVector& ContactOrigin)
{
	Fdemo_mapPlayerShapeSkillExecutionResult Result;
	Result.Family = Family;
	if (!ShouldUseM01PlayerShapeSkillProductPath())
	{
		return Result;
	}
	Result = CombatRunCoordinator.ExecutePlayerShapeSkill(
		Family,
		RawDamage,
		WorldOverlaps,
		ContactOrigin);
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_PLAYER_SHAPE_SKILL Event=ProductOverlap Family=%d Error=%d ActivationId=%s Contacts=%d Candidates=%d Delivered=%d Committed=%d Replayed=%d"),
		static_cast<int32>(Result.Family),
		static_cast<int32>(Result.Error),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		Result.WorldContactCount,
		Result.ResolvedCandidateCount,
		Result.DeliveredImpactCount,
		Result.CommittedImpactCount,
		Result.AlreadyCommittedImpactCount);
	return Result;
}

bool Ademo_mapGameMode::ShouldUseM01PlayerProjectileProductPath(
	const AActor* SourcePlayer) const
{
	// Source identity is checked against the live local product Pawn so M01
	// retains ownership even if CombatRunCoordinator is not ready yet.
	return IsM01ExpeditionMap()
		&& SourcePlayer != nullptr
		&& SourcePlayer == GetDemoPawn();
}

Fdemo_mapPlayerProjectileLaunchResult
Ademo_mapGameMode::PrepareM01PlayerStraightProjectile(
	AActor* SourcePlayer,
	float RawDamage)
{
	Fdemo_mapPlayerProjectileLaunchResult Result;
	if (!ShouldUseM01PlayerProjectileProductPath(SourcePlayer))
	{
		return Result;
	}
	Result = CombatRunCoordinator.PreparePlayerStraightProjectile(
		SourcePlayer,
		RawDamage);
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_PLAYER_PROJECTILE Event=LaunchPrepared Error=%d Sequence=%llu ActivationId=%s Damage=%.3f"),
		static_cast<int32>(Result.Error),
		static_cast<unsigned long long>(Result.ActivationSequence),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		Result.RawDamage);
	return Result;
}

Fdemo_mapPlayerProjectileImpactResult
Ademo_mapGameMode::ExecuteM01PlayerStraightProjectileImpact(
	AActor* SourcePlayer,
	AActor* TargetEnemy,
	UPrimitiveComponent* TargetComponent,
	uint64 ActivationSequence,
	const FGuid& ExpectedActivationId,
	float RawDamage,
	const FVector& ImpactLocation,
	const FVector& ImpactNormal)
{
	Fdemo_mapPlayerProjectileImpactResult Result;
	Result.ActivationSequence = ActivationSequence;
	if (!ShouldUseM01PlayerProjectileProductPath(SourcePlayer))
	{
		return Result;
	}
	Result = CombatRunCoordinator.ExecutePlayerStraightProjectileImpact(
		SourcePlayer,
		TargetEnemy,
		TargetComponent,
		ActivationSequence,
		ExpectedActivationId,
		RawDamage,
		ImpactLocation,
		ImpactNormal);
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_PLAYER_PROJECTILE Event=ProductContact Error=%d Sequence=%llu ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Result.Error),
		static_cast<unsigned long long>(Result.ActivationSequence),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

Fdemo_mapShanmenControlledWeaponHostAttachResult
Ademo_mapGameMode::AttachControlledWeaponToActiveCombatRun(
	const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
	AActor* WeaponActor,
	UPrimitiveComponent* WeaponCollisionRoot,
	const Fdemo_mapShanmenControlledWeaponMotionCapture& Motion)
{
	return ControlledWeaponRunHost.TryAttach(
		Prepared,
		CombatRunCoordinator,
		GetDemoPawn(),
		WeaponActor,
		WeaponCollisionRoot,
		Motion);
}

Fdemo_mapShanmenControlledWeaponRunCommandResult
Ademo_mapGameMode::RouteControlledWeaponIntent(
	const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Intent)
{
	return ControlledWeaponRunCommandRouter.TryRoute(
		ControlledWeaponRunHost,
		CombatRunCoordinator,
		Intent);
}

Fdemo_mapShanmenControlledWeaponThreatSampleResult
Ademo_mapGameMode::RouteControlledWeaponThreatSampleIntent(
	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Intent)
{
	return ControlledWeaponThreatSampleRouter.TryRoute(
		ControlledWeaponRunHost,
		CombatRunCoordinator,
		Intent);
}

Fdemo_mapShanmenSpiritEvasionProductRouteResult
Ademo_mapGameMode::RouteSpiritEvasionStartIntent(
	const FVector& CandidateDirection)
{
	ACharacter* PlayerCharacter = Cast<ACharacter>(GetDemoPawn());
	Udemo_mapShanmenSpiritEvasionComponent* Component =
		PlayerCharacter
			? EnsurePlayerSpiritEvasion(PlayerCharacter)
			: nullptr;
	Fdemo_mapShanmenSpiritEvasionProductRouteResult Result =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRoute(
		Component,
		CombatRunCoordinator,
		PlayerCharacter,
		CandidateDirection,
		[this]()
		{
			return RoutePlayerActionGate(
				Edemo_mapShanmenPlayerActionKind::SpiritEvasion);
		});
	if (!Result.IsAccepted())
	{
		return Result;
	}

	FShanmenSpiritEvasionProjectionReceipt Projection;
	Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
	FShanmenSwordRhythmContribution Contribution;
	FString ContributionDiagnostic;
	if (!Component
		|| !Component->TryProjectDefenseLayer(Projection)
		|| !CombatRunFixedTimeline.TryCapture(TimelineSample)
		|| !SwordRhythmProductSession.TryRecordSpiritEvasionContribution(
			Projection,
			TimelineSample,
			Contribution,
			ContributionDiagnostic))
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=SpiritEvasionContributionRejected ActivationId=%s Diagnostic=%s"),
			*Result.CommandRoute.ActivationId.ToString(
				EGuidFormats::DigitsWithHyphens),
			ContributionDiagnostic.IsEmpty()
				? TEXT("Active SpiritEvasion projection or Run timeline was unavailable.")
				: *ContributionDiagnostic);
		return Result;
	}
	UE_LOG(Logdemo_map,
		Log,
		TEXT("0_0_10_SWORD_RHYTHM Event=SpiritEvasionContributionRecorded ActivationId=%s ProjectionId=%s ContributionId=%s Tick=%lld"),
		*Result.CommandRoute.ActivationId.ToString(
			EGuidFormats::DigitsWithHyphens),
		*Projection.GetProjectionId().ToString(
			EGuidFormats::DigitsWithHyphens),
		*Contribution.GetContributionId().ToString(
			EGuidFormats::DigitsWithHyphens),
		static_cast<long long>(TimelineSample.GetCurrentTick()));
	return Result;
}

Fdemo_mapShanmenWeaponGuardSessionStartResult
Ademo_mapGameMode::RouteWeaponGuardStartIntent(
	const FGuid& TimelineId,
	int64 ActiveStartTick)
{
	const Fdemo_mapItemAuthority* ItemAuthority =
		PlayerItemSubsystem.IsValid()
			? &PlayerItemSubsystem->GetAuthority()
			: nullptr;
	if (!ItemAuthority)
	{
		return WeaponGuardProductSession.TryStart(
			ItemAuthority,
			CombatRunCoordinator,
			TimelineId,
			ActiveStartTick);
	}
	const Fdemo_mapShanmenPlayerActionGateResult ActionGate =
		RoutePlayerActionGate(
			Edemo_mapShanmenPlayerActionKind::WeaponGuard);
	if (!ActionGate.IsAuthorized())
	{
		Fdemo_mapShanmenWeaponGuardSessionStartResult Result;
		Result.Error =
			Edemo_mapShanmenWeaponGuardSessionStartError::ActionConflict;
		Result.ActionGate = ActionGate;
		Result.Diagnostic = ActionGate.Diagnostic;
		return Result;
	}
	Fdemo_mapShanmenWeaponGuardSessionStartResult Result =
		WeaponGuardProductSession.TryStart(
		ItemAuthority,
		CombatRunCoordinator,
		TimelineId,
		ActiveStartTick);
	Result.ActionGate = ActionGate;
	return Result;
}

Fdemo_mapShanmenWeaponGuardInputTimelineSample
Ademo_mapGameMode::CaptureWeaponGuardInputTimeline() const
{
	Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample Sample;
	if (CombatRunFixedTimeline.TryCapture(TimelineSample))
	{
		Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
			TimelineSample.GetTimelineId(),
			TimelineSample.GetCurrentTick(),
			Sample);
	}
	return Sample;
}

Fdemo_mapShanmenWeaponGuardSessionTransitionResult
Ademo_mapGameMode::RouteWeaponGuardReleaseIntent()
{
	return RouteWeaponGuardTerminationIntent(
		Edemo_mapShanmenWeaponGuardTerminationReason::InputReleased);
}

Fdemo_mapShanmenWeaponGuardSessionTransitionResult
Ademo_mapGameMode::RouteWeaponGuardTerminationIntent(
	Edemo_mapShanmenWeaponGuardTerminationReason Reason)
{
	const Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result =
		WeaponGuardProductSession.TryTerminate(Reason);
	if (!Result.IsSuccess())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("0_0_10_WEAPON_GUARD Event=TerminationRejected Reason=%d Status=%d Error=%d HostId=%s Diagnostic=%s"),
			static_cast<int32>(Result.Reason),
			static_cast<int32>(Result.Status),
			static_cast<int32>(Result.Error),
			*Result.HostId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.Diagnostic);
	}
	else if (!Result.IsNoOp())
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("0_0_10_WEAPON_GUARD Event=Terminated Reason=%d Status=%d HostId=%s"),
			static_cast<int32>(Result.Reason),
			static_cast<int32>(Result.Status),
			*Result.HostId.ToString(EGuidFormats::DigitsWithHyphens));
	}
	return Result;
}

bool Ademo_mapGameMode::AdvanceControlledWeaponOrbit(
	float DeltaSeconds,
	Fdemo_mapShanmenControlledWeaponHostOrbitBatch& OutBatch)
{
	return ControlledWeaponRunHost.TryAdvanceOrbitingInOrder(
		DeltaSeconds, OutBatch);
}

Fdemo_mapShanmenControlledWeaponOrbitFrameResult
Ademo_mapGameMode::AdvanceControlledWeaponOrbitFrame(float DeltaSeconds)
{
	return ControlledWeaponRunHost.AdvanceOrbitingFrame(DeltaSeconds);
}

Fdemo_mapShanmenThrownWeaponSessionResult
Ademo_mapGameMode::RouteThrownWeaponHotbarIntent(
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent)
{
	if (!Intent.IsValid()
		|| !ThrownWeaponProductLifecycle.IsValid()
		|| !ThrownWeaponProductLifecycle.IsActive()
		|| !CombatRunCoordinator.IsReady()
		|| ThrownWeaponProductLifecycle.GetRunId()
			!= CombatRunCoordinator.GetRunId())
	{
		return ThrownWeaponProductLifecycle.TrySubmitHotbar(
			GetWorld(),
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			CombatRunCoordinator,
			Intent);
	}
	const Fdemo_mapShanmenPlayerActionGateResult ActionGate =
		RoutePlayerActionGate(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon);
	if (!ActionGate.IsAuthorized())
	{
		Fdemo_mapShanmenThrownWeaponSessionResult Result;
		Result.Status =
			Edemo_mapShanmenThrownWeaponSessionStatus::ActionConflict;
		Result.SelectionId = Intent.GetSelectionId();
		Result.RunId = CombatRunCoordinator.GetRunId();
		Result.HotbarSlotNumber = Intent.GetHotbarSlotNumber();
		Result.ActionGate = ActionGate;
		Result.Diagnostic = ActionGate.Diagnostic;
		return Result;
	}
	Fdemo_mapShanmenThrownWeaponSessionResult Result =
		ThrownWeaponProductLifecycle.TrySubmitHotbar(
		GetWorld(),
		Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
		CombatRunCoordinator,
		Intent);
	Result.ActionGate = ActionGate;
	return Result;
}

Fdemo_mapShanmenThrownWeaponInputResult
Ademo_mapGameMode::RouteThrownWeaponHotbarInput(
	const int32 HotbarSlotNumber,
	AActor* SourceActor,
	TFunctionRef<FVector()> SampleAimDirection)
{
	Udemo_mapShanmenItemAuthoritySubsystem* Authority = GetGameInstance()
		? GetGameInstance()->GetSubsystem<
			Udemo_mapShanmenItemAuthoritySubsystem>()
		: nullptr;
	return ThrownWeaponInputAdapter.RouteHotbarInput(
		Authority,
		ThrownWeaponProductLifecycle,
		CombatRunCoordinator,
		GetWorld(),
		Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
		SourceActor,
		HotbarSlotNumber,
		SampleAimDirection,
		[this]()
		{
			return RoutePlayerActionGate(
				Edemo_mapShanmenPlayerActionKind::ThrownWeapon);
		});
}

Fdemo_mapShanmenMeridianShockTreatmentInputResult
Ademo_mapGameMode::RouteMeridianShockTreatmentHotbarInput(
	const int32 HotbarSlotNumber)
{
	Udemo_mapShanmenItemAuthoritySubsystem* Authority = GetGameInstance()
		? GetGameInstance()->GetSubsystem<
			Udemo_mapShanmenItemAuthoritySubsystem>()
		: nullptr;
	return MeridianShockTreatmentInputAdapter.RouteHotbarInput(
		Authority,
		MeridianShockTreatmentProductLifecycle,
		CombatRunFixedTimeline,
		HotbarSlotNumber);
}

Fdemo_mapShanmenThrownWeaponSessionResult
Ademo_mapGameMode::RecoverThrownWeaponCancellation(
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent)
{
	return ThrownWeaponProductLifecycle.TryRecoverCancellation(Intent);
}

bool Ademo_mapGameMode::InterruptThrownWeaponFlight()
{
	return ThrownWeaponProductLifecycle.TryInterruptFlight();
}

bool Ademo_mapGameMode::ExpireThrownWeaponRange()
{
	return ThrownWeaponProductLifecycle.TryExpireRange();
}

Fdemo_mapShanmenSwordQiControllerResult
Ademo_mapGameMode::RouteSwordQiIntent(
	const Fdemo_mapShanmenSwordQiIntent& Intent)
{
	APawn* PlayerPawn = GetDemoPawn();
	Udemo_mapAttributeComponent* Attributes =
		PlayerAttributeComponent.Get();
	if (!PlayerItemSubsystem.IsValid() || !PlayerPawn || !Attributes)
	{
		Fdemo_mapShanmenSwordQiControllerResult Result;
		Result.Status = Edemo_mapShanmenSwordQiControllerStatus::
			ProductDependenciesUnavailable;
		Result.IntentId = Intent.GetIntentId();
		Result.RunId = CombatRunCoordinator.GetRunId();
		Result.Diagnostic =
			TEXT("Sword Qi product route requires the active player, item authority and attribute authority.");
		return Result;
	}
	return SwordQiProductController.TrySubmit(
		GetWorld(),
		Ademo_mapShanmenSwordQiProjectile::StaticClass(),
		PlayerItemSubsystem->GetAuthority(),
		*Attributes,
		CombatRunCoordinator,
		PlayerPawn,
		Intent,
		[this]()
		{
			return RoutePlayerActionGate(
				Edemo_mapShanmenPlayerActionKind::SwordQi);
		});
}

Fdemo_mapShanmenSwordQiInputResult
Ademo_mapGameMode::RouteSwordQiStartInput(
	const bool bGameplayInputAllowed,
	const FGuid& InputEventId,
	TFunctionRef<FVector()> SampleOrigin,
	TFunctionRef<FVector()> SampleAimDirection)
{
	const bool bProductRouteAvailable =
		SwordQiProductController.IsActive()
		&& SwordQiProductController.IsValid()
		&& CombatRunCoordinator.IsReady()
		&& SwordQiProductController.GetRunId()
			== CombatRunCoordinator.GetRunId()
		&& PlayerItemSubsystem.IsValid()
		&& PlayerAttributeComponent.IsValid()
		&& GetDemoPawn() != nullptr;
	const FGuid RunId = CombatRunCoordinator.IsReady()
		? CombatRunCoordinator.GetRunId()
		: FGuid();

	return Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
		bGameplayInputAllowed,
		bProductRouteAvailable,
		RunId,
		InputEventId,
		[&SampleOrigin, &SampleAimDirection]()
		{
			Fdemo_mapShanmenSwordQiInputSample Sample;
			Fdemo_mapShanmenSwordQiInputSample::TryCapture(
				SampleOrigin(),
				SampleAimDirection(),
				Sample);
			return Sample;
		},
		[this](const Fdemo_mapShanmenSwordQiIntent& Intent)
		{
			return RouteSwordQiIntent(Intent);
		});
}

Fdemo_mapShanmenSwordQiCommandEventResult
Ademo_mapGameMode::IssueSwordQiStartCommand(
	const bool bGameplayInputAllowed,
	TFunctionRef<FVector()> SampleOrigin,
	TFunctionRef<FVector()> SampleAimDirection)
{
	return SwordQiCommandEventOwner.TryIssue(
		[this,
		 bGameplayInputAllowed,
		 &SampleOrigin,
		 &SampleAimDirection](const FGuid& InputEventId)
		{
			return RouteSwordQiStartInput(
				bGameplayInputAllowed,
				InputEventId,
				SampleOrigin,
				SampleAimDirection);
		});
}

Fdemo_mapShanmenSwordQiCommandEventResult
Ademo_mapGameMode::ReplaySwordQiStartCommand(
	const Fdemo_mapShanmenSwordQiCommandEvent& Event,
	const bool bGameplayInputAllowed,
	TFunctionRef<FVector()> SampleOrigin,
	TFunctionRef<FVector()> SampleAimDirection)
{
	return SwordQiCommandEventOwner.TryReplay(
		Event,
		[this,
		 bGameplayInputAllowed,
		 &SampleOrigin,
		 &SampleAimDirection](const FGuid& InputEventId)
		{
			return RouteSwordQiStartInput(
				bGameplayInputAllowed,
				InputEventId,
				SampleOrigin,
				SampleAimDirection);
		});
}

bool Ademo_mapGameMode::InterruptSwordQiFlight()
{
	return SwordQiProductController.TryInterrupt();
}

bool Ademo_mapGameMode::ExpireSwordQiRange()
{
	return SwordQiProductController.TryExpireRange();
}

bool Ademo_mapGameMode::RetireSwordQiTerminal(
	Fdemo_mapShanmenSwordQiTerminalReceipt& OutReceipt)
{
	return SwordQiProductController.TryRetireTerminal(OutReceipt);
}

bool Ademo_mapGameMode::ShouldUseM01EnemyAttackProductPath() const
{
	// M01 owns this routing decision even while the Run is still preparing:
	// an unready canonical coordinator fails closed instead of double-writing
	// through the retained legacy damage delegate.
	return IsM01ExpeditionMap();
}

Fdemo_mapM01EnemyAttackWeaponGuardContext
Ademo_mapGameMode::CaptureM01EnemyAttackWeaponGuardContext()
{
	Fdemo_mapM01EnemyAttackWeaponGuardContext Context;
	if (!ReconcileWeaponGuardAuthorization(TEXT("HostileImpactCapture")))
	{
		// Preserve fail-closed behavior: a rejected stale-authorization
		// termination is carried as an enabled invalid context, so the
		// Coordinator rejects before vitality resolution.
		Context.Session = &WeaponGuardProductSession;
		return Context;
	}
	if (WeaponGuardProductSession.IsEmpty())
	{
		return Context;
	}

	// A non-empty or structurally invalid Session must never silently bypass
	// guard composition. An invalid timeline is therefore carried as an invalid
	// enabled context and rejected by the Coordinator before damage resolution.
	Context.Session = &WeaponGuardProductSession;
	if (CombatRunFixedTimeline.IsValid()
		&& !CombatRunFixedTimeline.IsEmpty())
	{
		Context.TimelineId = CombatRunFixedTimeline.GetTimelineId();
		Context.ObservedTick = CombatRunFixedTimeline.GetCurrentTick();
	}
	return Context;
}

void Ademo_mapGameMode::ObserveSwordRhythmWeaponGuardContribution(
	const Fdemo_mapM01EnemyAttackExecutionResult& AttackResult)
{
	if (!AttackResult.IsExecuted()
		|| !AttackResult.bWeaponGuardInspected
		|| !AttackResult.WeaponGuardDefense.HasGuardLayer())
	{
		return;
	}
	const FShanmenWeaponGuardTimingProjectionReceipt& Projection =
		AttackResult.WeaponGuardDefense.Defense.Composition.TimingProjection;
	if (!Projection.IsValid()
		|| Projection.GetBand()
			!= EShanmenWeaponGuardTimingBand::Perfect)
	{
		return;
	}

	FShanmenSwordRhythmContribution Contribution;
	FString Diagnostic;
	if (!SwordRhythmProductSession.TryRecordPerfectWeaponGuardContribution(
			Projection,
			Contribution,
			Diagnostic))
	{
		UE_LOG(Logdemo_map,
			Error,
			TEXT("0_0_10_SWORD_RHYTHM Event=PerfectGuardContributionRejected ImpactId=%s ProjectionId=%s Diagnostic=%s"),
			*AttackResult.Impact.GetRequest().ImpactId.ToString(
				EGuidFormats::DigitsWithHyphens),
			*Projection.GetReceiptId().ToString(
				EGuidFormats::DigitsWithHyphens),
			*Diagnostic);
		return;
	}
	UE_LOG(Logdemo_map,
		Log,
		TEXT("0_0_10_SWORD_RHYTHM Event=PerfectGuardContributionRecorded ImpactId=%s ProjectionId=%s ContributionId=%s Tick=%lld"),
		*AttackResult.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		*Projection.GetReceiptId().ToString(
			EGuidFormats::DigitsWithHyphens),
		*Contribution.GetContributionId().ToString(
			EGuidFormats::DigitsWithHyphens),
		static_cast<long long>(Contribution.GetObservedTick()));
}

Fdemo_mapM01EnemyAttackExecutionResult
Ademo_mapGameMode::ExecuteM01EnemyBasicMeleeStrike(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult Result;
	if (!ShouldUseM01EnemyAttackProductPath())
	{
		return Result;
	}
	Fdemo_mapM01EnemyAttackWeaponGuardContext GuardContext =
		CaptureM01EnemyAttackWeaponGuardContext();
	Result = CombatRunCoordinator.ExecuteM01EnemyBasicMeleeStrike(
		SourceEnemy,
		TargetPlayer,
		RawDamage,
		GuardContext.IsEnabled() ? &GuardContext : nullptr);
	ObserveSwordRhythmWeaponGuardContribution(Result);
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_ENEMY_MELEE Event=ProductStrike Error=%d ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Result.Error),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

Fdemo_mapM01EnemyAttackExecutionResult
Ademo_mapGameMode::ExecuteM01EnemyMeleeDashContact(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	FName SkillProfileId,
	uint32 ActivationSerial,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult Result;
	if (!ShouldUseM01EnemyAttackProductPath())
	{
		return Result;
	}
	Fdemo_mapM01EnemyAttackWeaponGuardContext GuardContext =
		CaptureM01EnemyAttackWeaponGuardContext();
	Result = CombatRunCoordinator.ExecuteM01EnemyMeleeDashContact(
		SourceEnemy,
		TargetPlayer,
		SkillProfileId,
		ActivationSerial,
		RawDamage,
		GuardContext.IsEnabled() ? &GuardContext : nullptr);
	ObserveSwordRhythmWeaponGuardContribution(Result);
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_ENEMY_MELEE Event=ProductDashContact Family=%d Error=%d ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Result.Impact.GetFamily()),
		static_cast<int32>(Result.Error),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

Fdemo_mapM01EnemyAttackExecutionResult
Ademo_mapGameMode::ExecuteM01EnemyRangedProjectileImpact(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	FName SkillProfileId,
	uint64 ProjectileSequence,
	float RawDamage,
	const FVector& ImpactLocation,
	const FVector& ImpactNormal)
{
	Fdemo_mapM01EnemyAttackExecutionResult Result;
	if (!ShouldUseM01EnemyAttackProductPath())
	{
		return Result;
	}
	Fdemo_mapM01EnemyAttackWeaponGuardContext GuardContext =
		CaptureM01EnemyAttackWeaponGuardContext();
	Result = CombatRunCoordinator.ExecuteM01EnemyRangedProjectileImpact(
		SourceEnemy,
		TargetPlayer,
		SkillProfileId,
		ProjectileSequence,
		RawDamage,
		ImpactLocation,
		ImpactNormal,
		GuardContext.IsEnabled() ? &GuardContext : nullptr);
	ObserveSwordRhythmWeaponGuardContribution(Result);
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_ENEMY_PROJECTILE Event=ProductContact Family=%d Error=%d Sequence=%llu ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Result.Impact.GetFamily()),
		static_cast<int32>(Result.Error),
		static_cast<unsigned long long>(ProjectileSequence),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

Fdemo_mapM01EnemyAttackExecutionResult
Ademo_mapGameMode::ExecuteM01EnemyHeavySectorAttack(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	uint64 AttackSequence,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult Result;
	if (!ShouldUseM01EnemyAttackProductPath())
	{
		return Result;
	}
	Fdemo_mapM01EnemyAttackWeaponGuardContext GuardContext =
		CaptureM01EnemyAttackWeaponGuardContext();
	Result = CombatRunCoordinator.ExecuteM01EnemyHeavySectorAttack(
		SourceEnemy,
		TargetPlayer,
		AttackSequence,
		RawDamage,
		GuardContext.IsEnabled() ? &GuardContext : nullptr);
	ObserveSwordRhythmWeaponGuardContribution(Result);
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_ENEMY_HEAVY Event=ProductSector Family=%d Error=%d Sequence=%llu ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Result.Impact.GetFamily()),
		static_cast<int32>(Result.Error),
		static_cast<unsigned long long>(AttackSequence),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

Fdemo_mapM01EnemyAttackExecutionResult
Ademo_mapGameMode::ExecuteM01BossShapeAttack(
	AActor* SourceBoss,
	APawn* TargetPlayer,
	Edemo_mapM01BossAttack Attack,
	uint64 AttackSequence,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult Result;
	if (!ShouldUseM01EnemyAttackProductPath())
	{
		return Result;
	}
	Fdemo_mapM01EnemyAttackWeaponGuardContext GuardContext =
		CaptureM01EnemyAttackWeaponGuardContext();
	Result = CombatRunCoordinator.ExecuteM01BossShapeAttack(
		SourceBoss,
		TargetPlayer,
		Attack,
		AttackSequence,
		RawDamage,
		GuardContext.IsEnabled() ? &GuardContext : nullptr);
	ObserveSwordRhythmWeaponGuardContribution(Result);
	if (Result.IsExecuted()
		&& Result.Impact.GetFamily()
			== Edemo_mapM01EnemyAttackFamily::BossCharge
		&& Result.Delivery.CommitResult.IsSuccess()
		&& Result.Delivery.CommitResult.Receipt.GetAppliedDamage() > 0.0f)
	{
		Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
		if (!PlayerCombatConditionComponent.IsValid()
			|| !CombatRunFixedTimeline.TryCapture(TimelineSample))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("0_0_10_COMBAT_CONDITION Event=BossChargeProjectionUnavailable ImpactId=%s Conditions=%d Timeline=%d"),
				*Result.Delivery.CommitResult.Receipt.GetImpactId().ToString(
					EGuidFormats::DigitsWithHyphens),
				PlayerCombatConditionComponent.IsValid() ? 1 : 0,
				TimelineSample.IsValid() ? 1 : 0);
		}
		else
		{
			const Fdemo_mapShanmenCombatConditionApplicationResult
				ConditionResult =
					PlayerCombatConditionComponent->TryApplyMeridianShock(
						Result.Delivery.CommitResult.Receipt,
						TimelineSample);
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("0_0_10_COMBAT_CONDITION Event=BossChargeMeridianShock ImpactId=%s Status=%d Error=%d Tick=%lld Expiry=%lld Revision=%lld"),
				*Result.Delivery.CommitResult.Receipt.GetImpactId().ToString(
					EGuidFormats::DigitsWithHyphens),
				static_cast<int32>(ConditionResult.Status),
				static_cast<int32>(ConditionResult.Error),
				static_cast<long long>(TimelineSample.GetCurrentTick()),
				static_cast<long long>(
					PlayerCombatConditionComponent->
						GetMeridianShockExpiryTick()),
				static_cast<long long>(
					PlayerCombatConditionComponent->GetConditionRevision()));
			if (!ConditionResult.IsSuccess())
			{
				UE_LOG(
					Logdemo_map,
					Error,
					TEXT("0_0_10_COMBAT_CONDITION Event=BossChargeMeridianShockRejected ImpactId=%s Error=%d"),
					*Result.Delivery.CommitResult.Receipt.GetImpactId().ToString(
						EGuidFormats::DigitsWithHyphens),
					static_cast<int32>(ConditionResult.Error));
			}
		}
	}
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_BOSS_ATTACK Event=ProductShape Attack=%d Family=%d Error=%d Sequence=%llu ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Attack),
		static_cast<int32>(Result.Impact.GetFamily()),
		static_cast<int32>(Result.Error),
		static_cast<unsigned long long>(AttackSequence),
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

Fdemo_mapM01EnemyAttackExecutionResult
Ademo_mapGameMode::ExecuteM01BossVolleyProjectileImpact(
	AActor* SourceBoss,
	APawn* TargetPlayer,
	uint64 AttackSequence,
	int32 ProjectileOrdinal,
	float RawDamage,
	const FVector& ImpactLocation,
	const FVector& ImpactNormal)
{
	Fdemo_mapM01EnemyAttackExecutionResult Result;
	if (!ShouldUseM01EnemyAttackProductPath())
	{
		return Result;
	}
	Fdemo_mapM01EnemyAttackWeaponGuardContext GuardContext =
		CaptureM01EnemyAttackWeaponGuardContext();
	Result = CombatRunCoordinator.ExecuteM01BossVolleyProjectileImpact(
		SourceBoss,
		TargetPlayer,
		AttackSequence,
		ProjectileOrdinal,
		RawDamage,
		ImpactLocation,
		ImpactNormal,
		GuardContext.IsEnabled() ? &GuardContext : nullptr);
	ObserveSwordRhythmWeaponGuardContribution(Result);
	const FShanmenImpactResult& Resolution = Result.Impact.GetResult();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_BOSS_ATTACK Event=ProductVolley Family=%d Error=%d Sequence=%llu Ordinal=%d ActivationId=%s ImpactId=%s Raw=%.3f Prevented=%.3f Final=%.3f Commit=%d"),
		static_cast<int32>(Result.Impact.GetFamily()),
		static_cast<int32>(Result.Error),
		static_cast<unsigned long long>(AttackSequence),
		ProjectileOrdinal,
		*Result.ActivationId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Impact.GetRequest().ImpactId.ToString(
			EGuidFormats::DigitsWithHyphens),
		Resolution.RawDamage,
		Resolution.PreventedDamage,
		Resolution.FinalDamage,
		static_cast<int32>(Result.Delivery.CommitResult.Status));
	return Result;
}

FString Ademo_mapGameMode::Get0909BProfileStorageRoot() const
{
	if (!Is0909BRuntimeReady())
	{
		return FString();
	}
	if (const Fdemo_mapProfilePreparationFlow* Flow =
		V3ProgressionManager->GetProfilePreparationFlow())
	{
		return Flow->GetStorageRoot();
	}
	return FString();
}

bool Ademo_mapGameMode::Open0909BOutOfRaidInventory(FString& OutFeedback)
{
	if (!Is0909BRuntimeReady())
	{
		OutFeedback = TEXT("宗门物品服务尚未初始化。");
		return false;
	}
	return V3ProgressionManager->OpenCodeBOutOfRaidInventory(OutFeedback);
}

void Ademo_mapGameMode::Set0909BOutOfRaidClosedCallback(
	TFunction<void()> InCallback)
{
	if (Is0909BRuntimeReady())
	{
		V3ProgressionManager->Set0909BOutOfRaidCloseCallback(MoveTemp(InCallback));
	}
}

void Ademo_mapGameMode::BeginPlay()
{
	Super::BeginPlay();
	PrepareV2CNavigation();
	InitializeRuntimeMission();
}

void Ademo_mapGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
	GetWorldTimerManager().ClearTimer(AutomationTimerHandle);
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	if (APawn* PlayerPawn = GetDemoPawn())
	{
		if (Udemo_mapPlayerHealthComponent* Health = PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>())
		{
			Health->OnPlayerDefeated.RemoveDynamic(this, &Ademo_mapGameMode::HandlePlayerDefeated);
			Health->OnPlayerDamaged.RemoveDynamic(this, &Ademo_mapGameMode::HandleM01PlayerDamaged);
		}
	}
	for (const TWeakObjectPtr<Ademo_mapTrainingTarget>& Target : SpawnedTargets)
	{
		if (Target.IsValid())
		{
			Target->OnDestroyed.RemoveDynamic(this, &Ademo_mapGameMode::HandleTrainingTargetDestroyed);
		}
	}
	ReleaseCombatProductRun(TEXT("EndPlay"));
	DestroyM01EnemyContent();
	DestroyM01ExtractionFoundation();
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ReconcileWeaponGuardAuthorization(TEXT("GameModeTick"));
	if (!CombatRunFixedTimeline.IsEmpty())
	{
		int64 AdvancedTicks = 0;
		FString TimelineDiagnostic;
		if (!CombatRunFixedTimeline.TryAdvance(
				static_cast<double>(DeltaSeconds),
				AdvancedTicks,
				TimelineDiagnostic))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=TimelineAdvanceRejected RunId=%s Delta=%.9f Diagnostic=%s"),
				*CombatRunFixedTimeline.GetRunId().ToString(
					EGuidFormats::DigitsWithHyphens),
				DeltaSeconds,
				*TimelineDiagnostic);
		}
		else if (PlayerCombatConditionComponent.IsValid()
			&& !PlayerCombatConditionComponent->IsEmpty())
		{
			Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
			if (!CombatRunFixedTimeline.TryCapture(TimelineSample))
			{
				UE_LOG(
					Logdemo_map,
					Error,
					TEXT("0_0_10_COMBAT_CONDITION Event=TimelineCaptureRejected RunId=%s"),
					*CombatRunFixedTimeline.GetRunId().ToString(
						EGuidFormats::DigitsWithHyphens));
			}
			else
			{
				const Fdemo_mapShanmenCombatConditionAdvanceResult
					ConditionAdvance =
						PlayerCombatConditionComponent->TryAdvance(
							TimelineSample);
				if (!ConditionAdvance.IsSuccess())
				{
					UE_LOG(
						Logdemo_map,
						Error,
						TEXT("0_0_10_COMBAT_CONDITION Event=AdvanceRejected RunId=%s Tick=%lld Error=%d"),
						*CombatRunFixedTimeline.GetRunId().ToString(
							EGuidFormats::DigitsWithHyphens),
						static_cast<long long>(
							TimelineSample.GetCurrentTick()),
						static_cast<int32>(ConditionAdvance.Error));
				}
				else if (ConditionAdvance.Status
					== Edemo_mapShanmenCombatConditionAdvanceStatus::Expired)
				{
					UE_LOG(
						Logdemo_map,
						Log,
						TEXT("0_0_10_COMBAT_CONDITION Event=MeridianShockExpired RunId=%s Tick=%lld Revision=%lld"),
						*CombatRunFixedTimeline.GetRunId().ToString(
							EGuidFormats::DigitsWithHyphens),
						static_cast<long long>(
							ConditionAdvance.ObservedTick),
						static_cast<long long>(
							ConditionAdvance.ConditionRevision));
				}
			}
		}
	}
	if (!ControlledWeaponRunHost.IsEmpty())
	{
		const Fdemo_mapShanmenControlledWeaponOrbitFrameResult OrbitFrame =
			AdvanceControlledWeaponOrbitFrame(DeltaSeconds);
		if (!OrbitFrame.IsValid()
			|| OrbitFrame.Status
				== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::HostInvalid)
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_CONTROLLED_WEAPON Event=OrbitFrameInvalid RunId=%s Status=%d Delta=%.6f Bound=%d Orbiting=%d Attempted=%d Advanced=%d"),
				*OrbitFrame.RunId.ToString(EGuidFormats::DigitsWithHyphens),
				static_cast<int32>(OrbitFrame.Status),
				DeltaSeconds,
				OrbitFrame.BoundCount,
				OrbitFrame.OrbitingCount,
				OrbitFrame.Batch.AttemptedCount,
				OrbitFrame.Batch.AdvancedCount);
		}
		else if (OrbitFrame.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::DeltaInvalid
			|| OrbitFrame.Status
				== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::MovementRejected)
		{
			UE_LOG(Logdemo_map, Warning,
				TEXT("0_0_10_CONTROLLED_WEAPON Event=OrbitFrameRejected RunId=%s Status=%d Delta=%.6f Bound=%d Orbiting=%d Attempted=%d Advanced=%d"),
				*OrbitFrame.RunId.ToString(EGuidFormats::DigitsWithHyphens),
				static_cast<int32>(OrbitFrame.Status),
				DeltaSeconds,
				OrbitFrame.BoundCount,
				OrbitFrame.OrbitingCount,
				OrbitFrame.Batch.AttemptedCount,
				OrbitFrame.Batch.AdvancedCount);
		}
	}
	if (!bM01ExtractionFoundationActive || M01ExtractionAuthority.IsRunTerminal()) return;
	if (PlayerItemSubsystem.IsValid())
	{
		M01ExtractionAuthority.SetSpatialItemEquipped(
			PlayerItemSubsystem->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot).IsValid());
	}
	Edemo_mapM01ExitType CompletedExit = Edemo_mapM01ExitType::Regular;
	if (M01ExtractionAuthority.Advance(DeltaSeconds, CompletedExit))
	{
		UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_COMPLETE exit=%s"),
			*Fdemo_mapM01ExtractionAuthority::ExitId(CompletedExit).ToString());
#if !UE_BUILD_SHIPPING
		if (bM01ExtractionVisibleSmokeRequested)
		{
			CompleteM01ExtractionVisibleSmoke(CompletedExit);
			return;
		}
#endif
		M01ExtractionAuthority.NotifyRunTerminal();
		HandleExtraction();
	}
}

void Ademo_mapGameMode::PrepareV2CNavigation()
{
	if ((!UsesPersistedEncounterMarkers() && !IsM01ExpeditionMap()) || GetWorld() == nullptr)
	{
		return;
	}
	const double NavigationStartSeconds = FPlatformTime::Seconds();
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavigationSystem == nullptr)
	{
		UE_LOG(Logdemo_map, Error, TEXT("V2C: NavigationSystemV1 is unavailable."));
		return;
	}
	int32 BoundsCount = 0;
	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		++BoundsCount;
		NavigationSystem->OnNavigationBoundsUpdated(*It);
		const FBox Bounds = It->GetComponentsBoundingBox(true);
		UE_LOG(Logdemo_map, Log, TEXT("V2C: NavMeshBounds extent=(%.1f, %.1f, %.1f)."), Bounds.GetExtent().X, Bounds.GetExtent().Y, Bounds.GetExtent().Z);
	}
	NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::Create);
	NavigationSystem->Build();
	V2FinalNavigationSeconds = static_cast<float>(FPlatformTime::Seconds() - NavigationStartSeconds);
	UE_LOG(Logdemo_map, Log, TEXT("V2C: requested navigation build for %d persisted bounds volume(s)."), BoundsCount);
}

void Ademo_mapGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	InitializeRuntimeMission();
}

void Ademo_mapGameMode::InitializeRuntimeMission()
{
	if (bRuntimeMissionInitialized)
	{
		return;
	}

	APawn* PlayerPawn = GetDemoPawn();
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (PlayerPawn == nullptr || MissionState == nullptr)
	{
		GetWorldTimerManager().SetTimer(InitializationTimerHandle, this, &Ademo_mapGameMode::InitializeRuntimeMission, 0.10f, false);
		return;
	}

	Udemo_mapAttributeComponent* PlayerAttributes = EnsurePlayerAttributes(PlayerPawn);
	Udemo_mapPlayerHealthComponent* PlayerHealth = EnsurePlayerHealth(PlayerPawn);
	if (PlayerHealth != nullptr)
	{
		PlayerHealth->OnPlayerDefeated.AddUniqueDynamic(this, &Ademo_mapGameMode::HandlePlayerDefeated);
		PlayerHealth->OnPlayerDamaged.AddUniqueDynamic(this, &Ademo_mapGameMode::HandleM01PlayerDamaged);
	}
	Udemo_mapFactionComponent* PlayerFaction = PlayerPawn->FindComponentByClass<Udemo_mapFactionComponent>();
	if (PlayerFaction == nullptr)
	{
		PlayerFaction = NewObject<Udemo_mapFactionComponent>(PlayerPawn, TEXT("RuntimePlayerFaction"));
		PlayerFaction->SetFaction(Edemo_mapFaction::Player);
		PlayerFaction->RegisterComponent();
	}
	Udemo_mapSkillComponent* Skills = EnsurePlayerSkills(PlayerPawn);
	Udemo_mapShanmenSpiritEvasionComponent* SpiritEvasion =
		EnsurePlayerSpiritEvasion(PlayerPawn);
	Udemo_mapItemSubsystem* Items = GetGameInstance() ? GetGameInstance()->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr;
	const bool bItemsReady = Items != nullptr && Items->BindPlayerPawn(PlayerPawn);
	PlayerItemSubsystem = Items;
	const bool bV3World = HasV3ProgressionFeature();
	MissionState->InitializeMission(3);
	if (!bV3World)
	{
		SpawnMissionActors(PlayerPawn);
	}
	const bool bV3Ready = !HasV3ProgressionFeature() || InitializeV3Progression(PlayerPawn, Items);
	const bool bMissionProjectionReady = bV3World
		? bV3Ready
		: (SpawnedTargets.Num() == MissionState->GetRequiredTargets()
			&& ExitZone.IsValid()
			&& Enemy.IsValid()
			&& FriendlyUnit.IsValid()
			&& (!UsesPersistedEncounterMarkers() || (RangedEnemy.IsValid() && HeavyEnemy.IsValid())));
	if (!bMissionProjectionReady
		|| PlayerAttributes == nullptr
		|| PlayerHealth == nullptr
		|| PlayerFaction == nullptr
		|| Skills == nullptr
		|| SpiritEvasion == nullptr
		|| !bItemsReady)
	{
		const FString Failure = FString::Printf(TEXT("T7: mission initialization failed. Targets=%d Exit=%d Enemy=%d Health=%d"), SpawnedTargets.Num(), ExitZone.IsValid() ? 1 : 0, Enemy.IsValid() ? 1 : 0, PlayerHealth != nullptr ? 1 : 0);
		if (bT4AutomationRequested || bT5AutomationRequested || bT7AutomationRequested || bV2AAutomationRequested || bV2BAutomationRequested || bV2BVisibleAcceptanceRequested || bV2CAutomationRequested || bV2CVisibleAcceptanceRequested || bV2DAutomationRequested || bV2DVisibleAcceptanceRequested || bV2EAutomationRequested || bV2EVisibleAcceptanceRequested || bV2FinalAutomationRequested || bV2FinalVisibleAcceptanceRequested || bV3AttributesGameplayAutomationRequested || bV3ItemCoreGameplayAutomationRequested || bV3WorldInteractionAutomationRequested || bV3InventoryUIAutomationRequested || bV3WorldUIVisibleAcceptanceRequested || bM01ExtractionVisibleSmokeRequested || bM01EnemyVisibleSmokeRequested)
		{
			FailAutomation(Failure);
		}
		else
		{
			UE_LOG(Logdemo_map, Error, TEXT("%s"), *Failure);
		}
		return;
	}

	bRuntimeMissionInitialized = true;
	V2FinalLoadSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	UE_LOG(Logdemo_map, Log, TEXT("T7R: runtime GameMode verified."));
	StartRequestedAutomation();
}

bool Ademo_mapGameMode::TryActivateCombatRun(
	APawn* PlayerPawn,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	Udemo_mapShanmenSpiritEvasionComponent* SpiritEvasion =
		PlayerPawn
		? PlayerPawn->FindComponentByClass<
			Udemo_mapShanmenSpiritEvasionComponent>()
		: nullptr;
	Udemo_mapShanmenCombatConditionComponent* ExistingConditions =
		PlayerPawn
			? PlayerPawn->FindComponentByClass<
				Udemo_mapShanmenCombatConditionComponent>()
			: nullptr;
	if (!ControlledWeaponRunHost.IsEmpty()
		|| !ControlledWeaponRunCommandRouter.IsEmpty()
		|| !ControlledWeaponThreatSampleRouter.IsEmpty()
		|| !ThrownWeaponProductLifecycle.IsEmpty()
		|| !MeridianShockTreatmentProductLifecycle.IsEmpty()
		|| !WeaponGuardProductSession.IsEmpty()
		|| !CombatRunFixedTimeline.IsEmpty()
		|| !SwordRhythmProductSession.IsEmpty()
		|| !SwordRhythmPresentationRunController.IsEmpty()
		|| !SwordQiProductController.IsEmpty()
		|| !SwordQiCommandEventOwner.IsEmpty()
		|| (ExistingConditions && !ExistingConditions->IsEmpty())
		|| (SpiritEvasion
			&& SpiritEvasion->HasHost()
			&& !SpiritEvasion->IsTerminal()))
	{
		OutDiagnostic =
			TEXT("Player combat Run binding rejected stale product-lifecycle state.");
		return false;
	}
	if (!PlayerPawn || !PlayerItemSubsystem.IsValid())
	{
		OutDiagnostic =
			TEXT("Player combat Run binding requires the product Pawn and ItemAuthority.");
		return false;
	}
	const FGuid ActiveRunId = PlayerItemSubsystem->GetActiveRunId();
	Udemo_mapPlayerHealthComponent* PlayerHealth =
		EnsurePlayerHealth(PlayerPawn);
	if (!ActiveRunId.IsValid() || !PlayerHealth)
	{
		OutDiagnostic =
			TEXT("Player combat Run binding requires an active authority Run and health host.");
		return false;
	}
	if (!CombatRunCoordinator.TryBeginRun(
			ActiveRunId,
			PlayerPawn,
			PlayerHealth,
			OutDiagnostic))
	{
		return false;
	}
	if (IsM01ExpeditionMap()
		&& CombatRunCoordinator.NumRegisteredM01Enemies() == 0)
	{
		for (const TWeakObjectPtr<AActor>& EnemyActor : M01EnemyActors)
		{
			if (!EnemyActor.IsValid()
				|| !CombatRunCoordinator.TryRegisterM01Enemy(
					EnemyActor.Get(),
					OutDiagnostic))
			{
				FString ReleaseDiagnostic;
				CombatRunCoordinator.TryEndRun(
					ActiveRunId,
					ReleaseDiagnostic);
				return false;
			}
		}
	}
	if (IsM01ExpeditionMap()
		&& CombatRunCoordinator.NumRegisteredM01Enemies()
			!= M01EnemyActors.Num())
	{
		OutDiagnostic =
			TEXT("Combat Run did not register the complete authored M01 enemy set.");
		FString ReleaseDiagnostic;
		CombatRunCoordinator.TryEndRun(
			ActiveRunId,
			ReleaseDiagnostic);
		return false;
	}
	if (UGameInstance* GameInstance = PlayerPawn->GetGameInstance())
	{
		if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
			GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			Authority
			&& Authority->GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready
			&& !ThrownWeaponProductLifecycle.TryBegin(
				*Authority,
				*PlayerPawn,
				CombatRunCoordinator,
				OutDiagnostic))
		{
			FString ReleaseDiagnostic;
			CombatRunCoordinator.TryEndRun(
				ActiveRunId,
				ReleaseDiagnostic);
			return false;
		}
	}
	ThrownWeaponInputAdapter.Reset();
	FString TimelineDiagnostic;
	if (!CombatRunFixedTimeline.TryBegin(
			ActiveRunId,
			TimelineDiagnostic))
	{
		FString ThrownDiagnostic;
		ThrownWeaponProductLifecycle.TryEnd(ThrownDiagnostic);
		FString ReleaseDiagnostic;
		CombatRunCoordinator.TryEndRun(
			ActiveRunId,
			ReleaseDiagnostic);
		OutDiagnostic = TimelineDiagnostic;
		return false;
	}
	Udemo_mapAttributeComponent* PlayerAttributes =
		EnsurePlayerAttributes(PlayerPawn);
	Udemo_mapShanmenCombatConditionComponent* CombatConditions =
		EnsurePlayerCombatConditions(PlayerPawn);
	FString ConditionDiagnostic;
	if (!PlayerAttributes
		|| !CombatConditions
		|| !CombatConditions->TryBegin(
			ActiveRunId,
			CombatRunCoordinator.GetPlayerEntityId(),
			CombatRunFixedTimeline.GetTimelineId(),
			PlayerAttributes,
			ConditionDiagnostic))
	{
		FString TimelineReleaseDiagnostic;
		CombatRunFixedTimeline.TryEnd(
			ActiveRunId,
			TimelineReleaseDiagnostic);
		FString ThrownDiagnostic;
		ThrownWeaponProductLifecycle.TryEnd(ThrownDiagnostic);
		FString ReleaseDiagnostic;
		CombatRunCoordinator.TryEndRun(
			ActiveRunId,
			ReleaseDiagnostic);
		OutDiagnostic = PlayerAttributes && CombatConditions
			? ConditionDiagnostic
			: TEXT("Combat condition binding requires player attribute and condition components.");
		return false;
	}
	if (UGameInstance* GameInstance = PlayerPawn->GetGameInstance())
	{
		if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
			GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			Authority
			&& Authority->GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready
			&& !MeridianShockTreatmentProductLifecycle.TryBegin(
				*Authority,
				CombatConditions,
				OutDiagnostic))
		{
			FString ConditionReleaseDiagnostic;
			if (!CombatConditions->TryEnd(
				ActiveRunId, ConditionReleaseDiagnostic))
			{
				CombatConditions->Reset();
			}
			FString TimelineReleaseDiagnostic;
			CombatRunFixedTimeline.TryEnd(
				ActiveRunId, TimelineReleaseDiagnostic);
			FString ThrownDiagnostic;
			ThrownWeaponProductLifecycle.TryEnd(ThrownDiagnostic);
			FString ReleaseDiagnostic;
			CombatRunCoordinator.TryEndRun(
				ActiveRunId, ReleaseDiagnostic);
			return false;
		}
	}
	MeridianShockTreatmentInputAdapter.Reset();
	FString SwordRhythmDiagnostic;
	if (!SwordRhythmProductSession.TryBegin(
			ActiveRunId,
			SwordRhythmDiagnostic))
	{
		FString TreatmentDiagnostic;
		MeridianShockTreatmentProductLifecycle.TryEnd(
			TreatmentDiagnostic);
		MeridianShockTreatmentInputAdapter.Reset();
		FString ConditionReleaseDiagnostic;
		if (!CombatConditions->TryEnd(
			ActiveRunId,
				ConditionReleaseDiagnostic))
		{
			CombatConditions->Reset();
		}
		FString TimelineReleaseDiagnostic;
		CombatRunFixedTimeline.TryEnd(
			ActiveRunId,
			TimelineReleaseDiagnostic);
		FString ThrownDiagnostic;
		ThrownWeaponProductLifecycle.TryEnd(ThrownDiagnostic);
		FString ReleaseDiagnostic;
		CombatRunCoordinator.TryEndRun(
			ActiveRunId,
			ReleaseDiagnostic);
		OutDiagnostic = SwordRhythmDiagnostic;
		return false;
	}
	FString SwordRhythmPresentationDiagnostic;
	if (!SwordRhythmPresentationRunController.TryBegin(
			ActiveRunId,
			SwordRhythmPresentationDiagnostic))
	{
		FString SwordRhythmReleaseDiagnostic;
		if (!SwordRhythmProductSession.TryEnd(
				ActiveRunId,
				SwordRhythmReleaseDiagnostic))
		{
			SwordRhythmProductSession.Reset();
		}
		FString TreatmentDiagnostic;
		MeridianShockTreatmentProductLifecycle.TryEnd(
			TreatmentDiagnostic);
		MeridianShockTreatmentInputAdapter.Reset();
		FString ConditionReleaseDiagnostic;
		if (!CombatConditions->TryEnd(
				ActiveRunId,
				ConditionReleaseDiagnostic))
		{
			CombatConditions->Reset();
		}
		FString TimelineReleaseDiagnostic;
		CombatRunFixedTimeline.TryEnd(
			ActiveRunId,
			TimelineReleaseDiagnostic);
		FString ThrownDiagnostic;
		ThrownWeaponProductLifecycle.TryEnd(ThrownDiagnostic);
		FString ReleaseDiagnostic;
		CombatRunCoordinator.TryEndRun(
			ActiveRunId,
			ReleaseDiagnostic);
		OutDiagnostic = SwordRhythmPresentationDiagnostic;
		return false;
	}
	FString SwordQiDiagnostic;
	if (!SwordQiProductController.TryBegin(
			ActiveRunId,
			SwordQiDiagnostic))
	{
		const bool bReleased =
			ReleaseCombatProductRun(TEXT("SwordQiControllerBindFailure"));
		OutDiagnostic = SwordQiDiagnostic;
		if (!bReleased)
		{
			OutDiagnostic +=
				TEXT(" Existing combat products also rejected activation rollback.");
		}
		return false;
	}
	FString SwordQiCommandDiagnostic;
	if (!SwordQiCommandEventOwner.TryBegin(
			ActiveRunId,
			SwordQiCommandDiagnostic))
	{
		const bool bReleased =
			ReleaseCombatProductRun(TEXT("SwordQiCommandOwnerBindFailure"));
		OutDiagnostic = SwordQiCommandDiagnostic;
		if (!bReleased)
		{
			OutDiagnostic +=
				TEXT(" Existing combat products also rejected activation rollback.");
		}
		return false;
	}
	UE_LOG(Logdemo_map, Log,
		TEXT("0_0_10_COMBAT_RUN Event=RunBound RunId=%s PlayerEntityId=%s M01Entities=%d M01VitalityHosts=%d ThrownWeaponLifecycle=%d TreatmentLifecycle=%d SwordQiController=%d SwordQiCommandOwner=%d RunTimelineId=%s RunTickRate=%lld ConditionDefinition=%s ConditionDurationTicks=%lld SwordRhythmConfigId=%s SwordRhythmWindow=[%lld,%lld)"),
		*ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*CombatRunCoordinator.GetPlayerEntityId().ToString(
			EGuidFormats::DigitsWithHyphens),
		CombatRunCoordinator.NumRegisteredM01Enemies(),
		CombatRunCoordinator.NumVitalityBoundM01Enemies(),
		ThrownWeaponProductLifecycle.IsActive() ? 1 : 0,
		MeridianShockTreatmentProductLifecycle.IsActive() ? 1 : 0,
		SwordQiProductController.IsActive() ? 1 : 0,
		SwordQiCommandEventOwner.IsActive() ? 1 : 0,
		*CombatRunFixedTimeline.GetTimelineId().ToString(
			EGuidFormats::DigitsWithHyphens),
		static_cast<long long>(
			Fdemo_mapShanmenCombatRunFixedTimeline::
				CanonicalTicksPerSecond()),
		*Udemo_mapShanmenCombatConditionComponent::
			MeridianShockDefinitionId().ToString(),
		static_cast<long long>(
			Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDurationTicks()),
		*SwordRhythmProductSession.GetConfig().GetConfigId().ToString(
			EGuidFormats::DigitsWithHyphens),
		static_cast<long long>(
			SwordRhythmProductSession.GetConfig().GetDefinition()
				.GetLinkOpenOffsetTicks()),
		static_cast<long long>(
			SwordRhythmProductSession.GetConfig().GetDefinition()
				.GetLinkCloseOffsetTicks()));
	return true;
}

bool Ademo_mapGameMode::ReleaseCombatProductRun(
	const TCHAR* Context)
{
	const TCHAR* SafeContext = Context ? Context : TEXT("Unknown");
	Fdemo_mapShanmenSwordQiCommandEventEndSummary SwordQiCommandSummary;
	Fdemo_mapShanmenSwordQiControllerEndSummary SwordQiSummary;
	// Treatment is the only product path that may own an incomplete durable
	// commit. Recover it before dismantling any other Run authority.
	const int32 TreatmentRequestCount =
		MeridianShockTreatmentProductLifecycle.NumCapturedRequests();
	const int32 TreatmentPendingCount =
		MeridianShockTreatmentProductLifecycle.NumPendingRecovery();
	FString TreatmentDiagnostic;
	if (!MeridianShockTreatmentProductLifecycle.TryEnd(
			TreatmentDiagnostic))
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_COMBAT_RUN Event=TreatmentRunReleaseRejected Context=%s CapturedRequests=%d PendingRecovery=%d Diagnostic=%s"),
			SafeContext,
			TreatmentRequestCount,
			TreatmentPendingCount,
			*TreatmentDiagnostic);
		return false;
	}
	MeridianShockTreatmentInputAdapter.Reset();
	if (!SwordQiCommandEventOwner.IsEmpty())
	{
		const FGuid ExpectedSwordQiCommandRunId =
			CombatRunCoordinator.IsActive()
				? CombatRunCoordinator.GetRunId()
				: SwordQiCommandEventOwner.GetRunId();
		FString SwordQiCommandDiagnostic;
		if (!SwordQiCommandEventOwner.TryEnd(
				ExpectedSwordQiCommandRunId,
				SwordQiCommandSummary,
				SwordQiCommandDiagnostic))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("0_0_10_COMBAT_RUN Event=SwordQiCommandRunReleaseRejected Context=%s RunId=%s CommittedEvents=%llu Diagnostic=%s"),
				SafeContext,
				*ExpectedSwordQiCommandRunId.ToString(
					EGuidFormats::DigitsWithHyphens),
				static_cast<unsigned long long>(
					SwordQiCommandEventOwner.NumCommittedEvents()),
				*SwordQiCommandDiagnostic);
			return false;
		}
	}
	if (!SwordQiProductController.IsEmpty())
	{
		const FGuid ExpectedSwordQiRunId = CombatRunCoordinator.IsActive()
			? CombatRunCoordinator.GetRunId()
			: SwordQiProductController.GetRunId();
		FString SwordQiDiagnostic;
		if (!SwordQiProductController.TryEnd(
				ExpectedSwordQiRunId,
				SwordQiSummary,
				SwordQiDiagnostic))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("0_0_10_COMBAT_RUN Event=SwordQiRunReleaseRejected Context=%s RunId=%s CapturedIntents=%d ProcessedCommands=%d Diagnostic=%s"),
				SafeContext,
				*ExpectedSwordQiRunId.ToString(
					EGuidFormats::DigitsWithHyphens),
				SwordQiProductController.NumCapturedIntents(),
				SwordQiProductController.GetSession().NumProcessedCommands(),
				*SwordQiDiagnostic);
			return false;
		}
	}
	// Attempt independent active-product cleanup before evaluating either
	// result. A SpiritEvasion teardown fault must not strand WeaponGuard, and
	// a WeaponGuard fault must not strand SpiritEvasion.
	const bool bSpiritEvasionReleased =
		ReleasePlayerSpiritEvasion(SafeContext);
	const Fdemo_mapShanmenWeaponGuardSessionTransitionResult GuardRelease =
		RouteWeaponGuardTerminationIntent(
			Edemo_mapShanmenWeaponGuardTerminationReason::RunTeardown);
	if (!GuardRelease.IsSuccess())
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_COMBAT_RUN Event=WeaponGuardRunReleaseRejected Context=%s Status=%d Error=%d Diagnostic=%s"),
			SafeContext,
			static_cast<int32>(GuardRelease.Status),
			static_cast<int32>(GuardRelease.Error),
			*GuardRelease.Diagnostic);
		return false;
	}
	if (!bSpiritEvasionReleased)
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary
		SwordRhythmPresentationSummary;
	bool bSwordRhythmPresentationReleased = true;
	if (!SwordRhythmPresentationRunController.IsEmpty())
	{
		const FGuid ExpectedPresentationRunId =
			CombatRunCoordinator.IsActive()
				? CombatRunCoordinator.GetRunId()
				: SwordRhythmPresentationRunController.GetRunId();
		FString PresentationDiagnostic;
		bSwordRhythmPresentationReleased =
			SwordRhythmPresentationRunController.TryEnd(
				ExpectedPresentationRunId,
				SwordRhythmPresentationSummary,
				PresentationDiagnostic);
		if (!bSwordRhythmPresentationReleased)
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=SwordRhythmPresentationReleaseRejected Context=%s RunId=%s Captured=%d Published=%d Queued=%d VisualPending=%d AudioPending=%d Diagnostic=%s"),
				SafeContext,
				*SwordRhythmPresentationSummary.RunId.ToString(
					EGuidFormats::DigitsWithHyphens),
				SwordRhythmPresentationSummary.CapturedDispatchCount,
				SwordRhythmPresentationSummary.PublishedDispatchCount,
				SwordRhythmPresentationSummary.QueuedDispatchCount,
				SwordRhythmPresentationSummary.bVisualPending ? 1 : 0,
				SwordRhythmPresentationSummary.bAudioPending ? 1 : 0,
				*PresentationDiagnostic);
			SwordRhythmPresentationRunController.Reset();
		}
	}
	if (!SwordRhythmProductSession.IsValid())
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_COMBAT_RUN Event=SwordRhythmInvalidOnRelease Context=%s"),
			SafeContext);
		SwordRhythmProductSession.Reset();
		return false;
	}
	const int32 SwordRhythmObservationCount =
		SwordRhythmProductSession.NumRecordedObservations();
	if (!SwordRhythmProductSession.IsEmpty())
	{
		if (!CombatRunCoordinator.IsActive()
			|| SwordRhythmProductSession.GetRunId()
				!= CombatRunCoordinator.GetRunId())
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=SwordRhythmRunMismatchOnRelease Context=%s Observations=%d"),
				SafeContext,
				SwordRhythmObservationCount);
			SwordRhythmProductSession.Reset();
			return false;
		}
		FString SwordRhythmDiagnostic;
		if (!SwordRhythmProductSession.TryEnd(
				CombatRunCoordinator.GetRunId(),
				SwordRhythmDiagnostic))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=SwordRhythmReleaseRejected Context=%s Observations=%d Diagnostic=%s"),
				SafeContext,
				SwordRhythmObservationCount,
				*SwordRhythmDiagnostic);
			return false;
		}
	}
	const int32 ThrownSelectionCount =
		ThrownWeaponProductLifecycle.NumCapturedSelections();
	FString ThrownDiagnostic;
	if (!ThrownWeaponProductLifecycle.TryEnd(ThrownDiagnostic))
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_COMBAT_RUN Event=ThrownWeaponRunReleaseRejected Context=%s CapturedSelections=%d Diagnostic=%s"),
			SafeContext,
			ThrownSelectionCount,
			*ThrownDiagnostic);
		return false;
	}
	ThrownWeaponInputAdapter.Reset();
	const int32 ConditionApplicationCount =
		PlayerCombatConditionComponent.IsValid()
			? PlayerCombatConditionComponent->NumProcessedApplications()
			: 0;
	const int64 ConditionRevision =
		PlayerCombatConditionComponent.IsValid()
			? PlayerCombatConditionComponent->GetConditionRevision()
			: 0;
	if (PlayerCombatConditionComponent.IsValid()
		&& !PlayerCombatConditionComponent->IsEmpty())
	{
		const FGuid ExpectedConditionRunId = CombatRunCoordinator.IsActive()
			? CombatRunCoordinator.GetRunId()
			: PlayerCombatConditionComponent->GetRunId();
		FString ConditionDiagnostic;
		if (!PlayerCombatConditionComponent->TryEnd(
				ExpectedConditionRunId,
				ConditionDiagnostic))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("0_0_10_COMBAT_RUN Event=ConditionRunReleaseRejected Context=%s RunId=%s Applications=%d Revision=%lld Diagnostic=%s"),
				SafeContext,
				*ExpectedConditionRunId.ToString(
					EGuidFormats::DigitsWithHyphens),
				ConditionApplicationCount,
				static_cast<long long>(ConditionRevision),
				*ConditionDiagnostic);
			return false;
		}
	}
	if (!CombatRunCoordinator.IsActive())
	{
		if (ControlledWeaponRunHost.IsEmpty()
			&& ControlledWeaponRunCommandRouter.IsEmpty()
			&& ControlledWeaponThreatSampleRouter.IsEmpty()
			&& MeridianShockTreatmentProductLifecycle.IsEmpty()
			&& CombatRunFixedTimeline.IsEmpty()
			&& (!PlayerCombatConditionComponent.IsValid()
				|| PlayerCombatConditionComponent->IsEmpty())
			&& SwordRhythmProductSession.IsEmpty()
			&& SwordRhythmPresentationRunController.IsEmpty()
			&& SwordQiProductController.IsEmpty()
			&& SwordQiCommandEventOwner.IsEmpty())
		{
			return bSwordRhythmPresentationReleased;
		}
		UE_LOG(Logdemo_map, Error,
			TEXT("0_0_10_COMBAT_RUN Event=OrphanedControlledWeaponState Context=%s BoundItems=%d RoutedIntents=%d ThreatSamples=%lld"),
			SafeContext,
			ControlledWeaponRunHost.NumBound(),
			ControlledWeaponRunCommandRouter.NumProcessedIntents(),
			static_cast<long long>(
				ControlledWeaponThreatSampleRouter.NumAcceptedSamples()));
		ControlledWeaponRunHost.Reset();
		ControlledWeaponRunCommandRouter.Reset();
		ControlledWeaponThreatSampleRouter.Reset();
		if (PlayerCombatConditionComponent.IsValid())
		{
			PlayerCombatConditionComponent->Reset();
		}
		CombatRunFixedTimeline.Reset();
		SwordRhythmProductSession.Reset();
		SwordRhythmPresentationRunController.Reset();
		SwordQiCommandEventOwner.Reset();
		SwordQiProductController.Reset();
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponRunEndResult Result =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			ControlledWeaponRunHost,
			CombatRunCoordinator);
	if (Result.IsEnded())
	{
		FString TimelineDiagnostic;
		if (!CombatRunFixedTimeline.TryEnd(
				Result.RunId,
				TimelineDiagnostic))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=RunTimelineReleaseRejected RunId=%s Context=%s Diagnostic=%s"),
				*Result.RunId.ToString(EGuidFormats::DigitsWithHyphens),
				SafeContext,
				*TimelineDiagnostic);
			CombatRunFixedTimeline.Reset();
			return false;
		}
		const int32 RoutedIntentCount =
			ControlledWeaponRunCommandRouter.NumProcessedIntents();
		const int64 ThreatSampleCount =
			ControlledWeaponThreatSampleRouter.NumAcceptedSamples();
		ControlledWeaponRunCommandRouter.Reset();
		ControlledWeaponThreatSampleRouter.Reset();
		UE_LOG(Logdemo_map, Log,
			TEXT("0_0_10_COMBAT_RUN Event=RunReleased RunId=%s Context=%s ControlledBound=%d ControlledInterrupted=%d RoutedIntents=%d ThreatSamples=%lld ThrownSelections=%d TreatmentRequests=%d TreatmentPendingAtTeardown=%d SwordQiCommandEvents=%llu SwordQiIntents=%d SwordQiCommands=%d SwordQiInterrupted=%d ConditionApplications=%d ConditionRevision=%lld SwordRhythmObservations=%d SwordRhythmPresentationPublished=%d SwordRhythmPresentationQueuedAtTeardown=%d WeaponGuardInterrupted=%d"),
			*Result.RunId.ToString(EGuidFormats::DigitsWithHyphens),
			SafeContext,
			Result.BoundItemCount,
			Result.InterruptedItemCount,
			RoutedIntentCount,
			static_cast<long long>(ThreatSampleCount),
			ThrownSelectionCount,
			TreatmentRequestCount,
			TreatmentPendingCount,
			static_cast<unsigned long long>(
				SwordQiCommandSummary.CommittedEventCount),
			SwordQiSummary.CapturedIntentCount,
			SwordQiSummary.ProcessedCommandCount,
			SwordQiSummary.bInterruptedFlight ? 1 : 0,
			ConditionApplicationCount,
			static_cast<long long>(ConditionRevision),
			SwordRhythmObservationCount,
			SwordRhythmPresentationSummary.PublishedDispatchCount,
			SwordRhythmPresentationSummary.QueuedDispatchCount,
			GuardRelease.Status
				== Edemo_mapShanmenWeaponGuardSessionTransitionStatus::Interrupted
				? 1
				: 0);
		return bSwordRhythmPresentationReleased;
	}

	UE_LOG(Logdemo_map, Error,
		TEXT("0_0_10_COMBAT_RUN Event=RunReleaseRejected Context=%s Status=%d Diagnostic=%s"),
		SafeContext,
		static_cast<int32>(Result.Status),
		*Result.Diagnostic);
	ControlledWeaponRunCommandRouter.Reset();
	ControlledWeaponThreatSampleRouter.Reset();
	ControlledWeaponRunHost.Reset();
	if (PlayerCombatConditionComponent.IsValid())
	{
		PlayerCombatConditionComponent->Reset();
	}
	CombatRunFixedTimeline.Reset();
	SwordRhythmProductSession.Reset();
	SwordRhythmPresentationRunController.Reset();
	SwordQiCommandEventOwner.Reset();
	SwordQiProductController.Reset();
	CombatRunCoordinator.Reset();
	return false;
}

bool Ademo_mapGameMode::ReconcileWeaponGuardAuthorization(
	const TCHAR* Context)
{
	if (!WeaponGuardProductSession.HasActive())
	{
		return true;
	}
	if (PlayerItemSubsystem.IsValid()
		&& WeaponGuardProductSession.IsCurrentAuthorization(
			PlayerItemSubsystem->GetAuthority()))
	{
		return true;
	}

	const Fdemo_mapShanmenWeaponGuardSessionTransitionResult Result =
		RouteWeaponGuardTerminationIntent(
			Edemo_mapShanmenWeaponGuardTerminationReason::
				WeaponAuthorizationChanged);
	if (!Result.IsSuccess() || !WeaponGuardProductSession.IsEmpty())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("0_0_10_WEAPON_GUARD Event=AuthorizationReconcileRejected Context=%s Status=%d Error=%d Active=%d"),
			Context ? Context : TEXT("Unknown"),
			static_cast<int32>(Result.Status),
			static_cast<int32>(Result.Error),
			WeaponGuardProductSession.HasActive() ? 1 : 0);
		return false;
	}
	return true;
}

bool Ademo_mapGameMode::ReleasePlayerSpiritEvasion(const TCHAR* Context)
{
	APawn* PlayerPawn = GetDemoPawn();
	if (!PlayerPawn)
	{
		return true;
	}
	TArray<Udemo_mapShanmenSpiritEvasionComponent*> Components;
	PlayerPawn->GetComponents<Udemo_mapShanmenSpiritEvasionComponent>(
		Components);
	if (Components.Num() > 1)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("0_0_10_SPIRIT_EVASION Event=RunReleaseRejected Context=%s Components=%d"),
			Context ? Context : TEXT("Unknown"),
			Components.Num());
		return false;
	}
	if (Components.IsEmpty()
		|| !Components[0]->HasHost()
		|| Components[0]->IsTerminal())
	{
		return true;
	}

	const Fdemo_mapShanmenSpiritEvasionCommandResult Interrupted =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
			Components[0],
			CombatRunCoordinator,
			Cast<ACharacter>(PlayerPawn),
			Fdemo_mapShanmenSpiritEvasionCommand::MakeInterrupt());
	if (!Interrupted.IsAccepted())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("0_0_10_SPIRIT_EVASION Event=RunReleaseRejected Context=%s Status=%d Diagnostic=%s"),
			Context ? Context : TEXT("Unknown"),
			static_cast<int32>(Interrupted.Status),
			*Interrupted.Diagnostic);
		return false;
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0_0_10_SPIRIT_EVASION Event=RunReleased Context=%s HostId=%s"),
		Context ? Context : TEXT("Unknown"),
		*Interrupted.Step.HostId.ToString(EGuidFormats::DigitsWithHyphens));
	return true;
}

bool Ademo_mapGameMode::ActivateV3MissionContentForRun()
{
	if (!HasV3ProgressionFeature())
	{
		return false;
	}
	if (bV3MissionContentActive)
	{
		bool bProjectionReady = false;
		if (IsM01ExpeditionMap())
		{
			bProjectionReady = bM01ExtractionFoundationActive
				&& M01ExtractionZones.Num() == 3
				&& bM01EnemyContentActive
				&& M01EnemyActors.Num() == 14;
		}
		else
		{
			bProjectionReady = SpawnedTargets.Num() == 3
				&& ExitZone.IsValid()
				&& FriendlyUnit.IsValid();
		}
		FString CombatDiagnostic;
		return bProjectionReady
			&& TryActivateCombatRun(GetDemoPawn(), CombatDiagnostic);
	}
	APawn* PlayerPawn = GetDemoPawn();
	Ademo_mapGameState* MissionState = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!PlayerPawn || !MissionState)
	{
		return false;
	}
	if (IsM01ExpeditionMap())
	{
		int32 PlayerStartCount = 0;
		APlayerStart* M01PlayerStart = nullptr;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			++PlayerStartCount;
			M01PlayerStart = *It;
		}
		if (PlayerStartCount != 1 || !M01PlayerStart)
		{
			UE_LOG(Logdemo_map, Error, TEXT("M01_MAP: expected one PlayerStart; found=%d."), PlayerStartCount);
			return false;
		}
		PlayerPawn->SetActorLocationAndRotation(
			M01PlayerStart->GetActorLocation(),
			M01PlayerStart->GetActorRotation(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		PlayerPawn->UpdateOverlaps();
		const bool bExtractionReady = InitializeM01ExtractionFoundation(PlayerPawn);
		const bool bEnemyReady = bExtractionReady && InitializeM01EnemyContent(PlayerPawn);
		bV3MissionContentActive = bExtractionReady && bEnemyReady;
		UE_LOG(Logdemo_map, Log, TEXT("M01_MAP: prepared Run entered authored M01 PlayerStart=%s exits=%d enemies=%d."),
			*M01PlayerStart->GetName(), M01ExtractionZones.Num(), M01EnemyActors.Num());
		FString CombatDiagnostic;
		if (bV3MissionContentActive
			&& !TryActivateCombatRun(PlayerPawn, CombatDiagnostic))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=RunBindingRejected Diagnostic=%s"),
				*CombatDiagnostic);
			DeactivateV3MissionContentForPreparation();
		}
		return bV3MissionContentActive;
	}
	MissionState->InitializeMission(3);
	SpawnMissionActors(PlayerPawn);
	bV3MissionContentActive = SpawnedTargets.Num() == MissionState->GetRequiredTargets()
		&& ExitZone.IsValid()
		&& FriendlyUnit.IsValid();
	if (bV3MissionContentActive)
	{
		bV3MissionContentActive = InitializeM01ExtractionFoundation(PlayerPawn);
		FString CombatDiagnostic;
		if (bV3MissionContentActive
			&& !TryActivateCombatRun(PlayerPawn, CombatDiagnostic))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("0_0_10_COMBAT_RUN Event=RunBindingRejected Diagnostic=%s"),
				*CombatDiagnostic);
			bV3MissionContentActive = false;
		}
	}
	if (!bV3MissionContentActive)
	{
		DeactivateV3MissionContentForPreparation();
	}
	return bV3MissionContentActive;
}

void Ademo_mapGameMode::BindV3EnemyProjections(
	Ademo_mapEnemyCharacter* InMelee,
	Ademo_mapRangedEnemyCharacter* InRanged,
	Ademo_mapHeavyEnemyCharacter* InHeavy)
{
	if (!HasV3ProgressionFeature())
	{
		return;
	}
	Enemy = InMelee;
	RangedEnemy = InRanged;
	HeavyEnemy = InHeavy;
}

void Ademo_mapGameMode::DeactivateV3MissionContentForPreparation()
{
	ReleaseCombatProductRun(TEXT("PreparationDeactivation"));
	DestroyM01EnemyContent();
	DestroyM01ExtractionFoundation();
	if (IsM01ExpeditionMap())
	{
		bV3MissionContentActive = false;
		return;
	}
	auto DestroyActor = [](TWeakObjectPtr<AActor>& Actor)
	{
		if (Actor.IsValid()) Actor->Destroy();
		Actor.Reset();
	};
	for (TWeakObjectPtr<Ademo_mapTrainingTarget>& Target : SpawnedTargets)
	{
		if (Target.IsValid()) Target->Destroy();
	}
	SpawnedTargets.Reset();
	CountedTargetActors.Reset();
	TWeakObjectPtr<AActor> Exit = ExitZone;
	TWeakObjectPtr<AActor> Melee = Enemy;
	TWeakObjectPtr<AActor> Ranged = RangedEnemy;
	TWeakObjectPtr<AActor> Heavy = HeavyEnemy;
	TWeakObjectPtr<AActor> Friendly = FriendlyUnit;
	DestroyActor(Exit);
	DestroyActor(Melee);
	DestroyActor(Ranged);
	DestroyActor(Heavy);
	DestroyActor(Friendly);
	ExitZone.Reset();
	Enemy.Reset();
	RangedEnemy.Reset();
	HeavyEnemy.Reset();
	FriendlyUnit.Reset();
	bV3MissionContentActive = false;
}

bool Ademo_mapGameMode::InitializeM01EnemyContent(APawn* PlayerPawn)
{
	if (!IsM01ExpeditionMap() || !PlayerPawn || !GetWorld()) return false;
	DestroyM01EnemyContent();
	FString ValidationError;
	if (!Fdemo_mapM01EnemyConfig::Validate(&ValidationError))
	{
		UE_LOG(Logdemo_map, Error, TEXT("M01_ENEMY_CONFIG_INVALID %s"), *ValidationError);
		return false;
	}
	TMap<FName, Ademo_mapM01Marker*> ParentMarkers;
	for (TActorIterator<Ademo_mapM01Marker> It(GetWorld()); It; ++It)
	{
		if (It->GetMarkerType() == Edemo_mapM01MarkerType::Encounter
			|| It->GetMarkerType() == Edemo_mapM01MarkerType::Boss)
		{
			ParentMarkers.Add(It->GetStableId(), *It);
		}
	}
	for (const FName Required : {
		FName(TEXT("M01.Encounter.Normal.01")),
		FName(TEXT("M01.Encounter.Normal.02")),
		FName(TEXT("M01.Encounter.Elite.01")),
		FName(TEXT("M01.Boss.Main")) })
	{
		if (!ParentMarkers.Contains(Required))
		{
			UE_LOG(Logdemo_map, Error, TEXT("M01_ENEMY_MARKER_MISSING id=%s"), *Required.ToString());
			return false;
		}
	}
	const FGuid ActiveRunId = PlayerItemSubsystem.IsValid()
		? PlayerItemSubsystem->GetActiveRunId()
		: FGuid::NewGuid();
	M01RunEnemyLedger.ResetForNewRun(ActiveRunId);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (const Fdemo_mapM01EnemyDefinition& Definition :
		Fdemo_mapM01EnemyConfig::GetDefinitions())
	{
		Ademo_mapM01Marker* Parent = ParentMarkers.FindRef(Definition.ParentMarkerId);
		if (!Parent || !M01RunEnemyLedger.TryClaimEncounter(Definition.EncounterId))
		{
			DestroyM01EnemyContent();
			return false;
		}
		const FVector SpawnLocation = Parent->GetActorTransform().TransformPositionNoScale(
			Definition.LocalOffset) + FVector(0.0f, 0.0f, 45.0f);
		const FRotator SpawnRotation =
			(PlayerPawn->GetActorLocation() - SpawnLocation).Rotation();
		UClass* SpawnClass = nullptr;
		switch (Definition.Archetype)
		{
		case Edemo_mapM01EnemyArchetype::StandardSkirmisher:
		case Edemo_mapM01EnemyArchetype::EliteStalker:
			SpawnClass = Ademo_mapEnemyCharacter::StaticClass();
			break;
		case Edemo_mapM01EnemyArchetype::StandardRanged:
			SpawnClass = Ademo_mapRangedEnemyCharacter::StaticClass();
			break;
		case Edemo_mapM01EnemyArchetype::StandardBruiser:
		case Edemo_mapM01EnemyArchetype::EliteBulwark:
			SpawnClass = Ademo_mapHeavyEnemyCharacter::StaticClass();
			break;
		case Edemo_mapM01EnemyArchetype::BossMain:
			SpawnClass = Ademo_mapM01BossCharacter::StaticClass();
			break;
		}
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(
			SpawnClass, SpawnLocation, SpawnRotation, Params);
		if (!Spawned)
		{
			DestroyM01EnemyContent();
			return false;
		}
		Udemo_mapM01EnemyIdentityComponent* Identity =
			NewObject<Udemo_mapM01EnemyIdentityComponent>(Spawned);
		Spawned->AddInstanceComponent(Identity);
		Identity->RegisterComponent();
		if (!Identity->Configure(Definition))
		{
			Spawned->Destroy();
			DestroyM01EnemyContent();
			return false;
		}
		Fdemo_mapEnemyEncounterIdentity LegacyIdentity;
		LegacyIdentity.EncounterId = Definition.EncounterId;
		LegacyIdentity.RouteId = Definition.RouteId;
		LegacyIdentity.SpawnMarkerId = Definition.SpawnMarkerId;
		LegacyIdentity.LootTableId = Definition.CorpseIdentity;
		LegacyIdentity.SkillProfileId = Definition.SkillProfileId;
		bool bConfigured = false;
		if (Ademo_mapEnemyCharacter* Melee = Cast<Ademo_mapEnemyCharacter>(Spawned))
		{
			bConfigured = Melee->ConfigureEncounter(
				LegacyIdentity, Definition.Tuning, Definition.IsElite());
		}
		else if (Ademo_mapRangedEnemyCharacter* Ranged =
			Cast<Ademo_mapRangedEnemyCharacter>(Spawned))
		{
			bConfigured = Ranged->ConfigureEncounter(
				LegacyIdentity, Definition.Tuning, false);
		}
		else if (Ademo_mapHeavyEnemyCharacter* Heavy =
			Cast<Ademo_mapHeavyEnemyCharacter>(Spawned))
		{
			bConfigured = Heavy->ConfigureEncounter(LegacyIdentity, Definition.Tuning);
		}
		else if (Ademo_mapM01BossCharacter* Boss =
			Cast<Ademo_mapM01BossCharacter>(Spawned))
		{
			bConfigured = Boss->ConfigureBoss(Definition);
			M01Boss = Boss;
		}
		if (!bConfigured)
		{
			Spawned->Destroy();
			DestroyM01EnemyContent();
			return false;
		}
		Spawned->Tags.AddUnique(TEXT("M01_ENEMY"));
		Spawned->Tags.AddUnique(Definition.EncounterId);
		Spawned->Tags.AddUnique(Definition.EnemyArchetypeId);
		Spawned->Tags.AddUnique(Definition.RewardSourceRoleId);
		Spawned->Tags.AddUnique(Definition.RiskTierId);
		if (Definition.IsElite()) Spawned->Tags.AddUnique(TEXT("M01_ELITE"));
		if (Definition.IsBoss()) Spawned->Tags.AddUnique(TEXT("M01_BOSS"));
		M01EnemyActors.Add(Spawned);
	}
	bM01EnemyContentActive = M01EnemyActors.Num() == 14
		&& M01RunEnemyLedger.GetClaimedEncounterCount() == 14
		&& M01Boss.IsValid();
	UE_LOG(Logdemo_map, Log,
		TEXT("M01_ENEMY_CONTENT_READY standard=10 elite=3 boss=1 encounters=%d run=%s."),
		M01RunEnemyLedger.GetClaimedEncounterCount(),
		*M01RunEnemyLedger.GetRunId().ToString(EGuidFormats::DigitsWithHyphens));
	return bM01EnemyContentActive;
}

void Ademo_mapGameMode::SuppressM01EnemyContent()
{
	for (TWeakObjectPtr<AActor>& Actor : M01EnemyActors)
	{
		if (Ademo_mapEnemyCharacter* Melee = Cast<Ademo_mapEnemyCharacter>(Actor.Get()))
			Melee->SetCombatSuppressed(true);
		else if (Ademo_mapRangedEnemyCharacter* Ranged =
			Cast<Ademo_mapRangedEnemyCharacter>(Actor.Get()))
			Ranged->SetCombatSuppressed(true);
		else if (Ademo_mapHeavyEnemyCharacter* Heavy =
			Cast<Ademo_mapHeavyEnemyCharacter>(Actor.Get()))
			Heavy->SetCombatSuppressed(true);
		else if (Ademo_mapM01BossCharacter* Boss =
			Cast<Ademo_mapM01BossCharacter>(Actor.Get()))
			Boss->SetCombatSuppressed(true);
	}
}

void Ademo_mapGameMode::DestroyM01EnemyContent()
{
	M01RunEnemyLedger.MarkTerminal();
	bM01EnemyContentActive = false;
	SuppressM01EnemyContent();
	const TArray<TWeakObjectPtr<AActor>> ActorsToDestroy = M01EnemyActors;
	M01EnemyActors.Reset();
	M01Boss.Reset();
	for (const TWeakObjectPtr<AActor>& Actor : ActorsToDestroy)
	{
		if (Actor.IsValid()) Actor->Destroy();
	}
}

bool Ademo_mapGameMode::InitializeM01ExtractionFoundation(APawn* PlayerPawn)
{
	if (!PlayerPawn || !GetWorld() || !PlayerItemSubsystem.IsValid()) return false;
	DestroyM01ExtractionFoundation();
	const bool bSpatialEquipped = PlayerItemSubsystem->GetAuthority()
		.GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot).IsValid();
	M01ExtractionAuthority.ResetForNewRun(bSpatialEquipped);
	ActiveM01RiskId = NAME_None;

	if (IsM01ExpeditionMap())
	{
		TSet<Edemo_mapM01ExitType> AuthoredTypes;
		for (TActorIterator<Ademo_mapM01ExtractionZone> It(GetWorld()); It; ++It)
		{
			Ademo_mapM01ExtractionZone* Zone = *It;
			if (!Zone || !Zone->IsAuthoredForM01() || AuthoredTypes.Contains(Zone->GetExitType()))
			{
				continue;
			}
			AuthoredTypes.Add(Zone->GetExitType());
			Zone->SetProjectionActive(true);
			Zone->RefreshPresentation();
			M01ExtractionZones.Add(Zone);
		}
		bM01ExtractionFoundationActive = M01ExtractionZones.Num() == 3
			&& AuthoredTypes.Contains(Edemo_mapM01ExitType::Regular)
			&& AuthoredTypes.Contains(Edemo_mapM01ExitType::Boss)
			&& AuthoredTypes.Contains(Edemo_mapM01ExitType::DiscardSpatial);
		if (!bM01ExtractionFoundationActive)
		{
			UE_LOG(Logdemo_map, Error, TEXT("M01_MAP: authored extraction validation failed zones=%d unique_types=%d."),
				M01ExtractionZones.Num(), AuthoredTypes.Num());
			DestroyM01ExtractionFoundation();
			return false;
		}
		UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_FOUNDATION_READY authored=1 regular=1 boss=1 discard_spatial=1 countdown=3.0"));
		return true;
	}

	FVector Forward = PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal2D();
	const TArray<Edemo_mapM01ExitType> Types = {
		Edemo_mapM01ExitType::Regular,
		Edemo_mapM01ExitType::Boss,
		Edemo_mapM01ExitType::DiscardSpatial
	};
	const TArray<FVector> Offsets = {
		-Forward * 650.0f,
		(-Forward * 520.0f) + (Right * 430.0f),
		(-Forward * 520.0f) - (Right * 430.0f)
	};
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		FVector Location;
		if (!FindGroundLocation(PlayerPawn->GetActorLocation() + Offsets[Index], PlayerPawn, Location))
		{
			DestroyM01ExtractionFoundation();
			return false;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ademo_mapM01ExtractionZone* Zone = GetWorld()->SpawnActor<Ademo_mapM01ExtractionZone>(
			Ademo_mapM01ExtractionZone::StaticClass(),
			Location,
			FRotator::ZeroRotator,
			Params);
		if (!Zone)
		{
			DestroyM01ExtractionFoundation();
			return false;
		}
		Zone->Configure(Types[Index]);
		M01ExtractionZones.Add(Zone);
	}
	bM01ExtractionFoundationActive = M01ExtractionZones.Num() == 3;
	// M01 owns the Run extraction surface. Keep the inherited training exit as a
	// compatibility actor, but prevent its legacy overlap callback from bypassing
	// the shared three-second interaction state machine.
	if (bM01ExtractionFoundationActive && ExitZone.IsValid())
	{
		ExitZone->SetActorEnableCollision(false);
		ExitZone->SetActorHiddenInGame(true);
	}
	UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_FOUNDATION_READY regular=1 boss=1 discard_spatial=1 countdown=3.0"));
	return bM01ExtractionFoundationActive;
}

void Ademo_mapGameMode::DestroyM01ExtractionFoundation()
{
	bM01ExtractionFoundationActive = false;
	ActiveM01RiskId = NAME_None;
	for (TWeakObjectPtr<Ademo_mapM01ExtractionZone>& Zone : M01ExtractionZones)
	{
		if (!Zone.IsValid()) continue;
		if (Zone->IsAuthoredForM01()) Zone->SetProjectionActive(false);
		else Zone->Destroy();
	}
	M01ExtractionZones.Reset();
}

Fdemo_mapM01ExtractionSnapshot Ademo_mapGameMode::GetM01ExtractionSnapshot(Edemo_mapM01ExitType ExitType) const
{
	if (bM01ExtractionFoundationActive) return M01ExtractionAuthority.GetSnapshot(ExitType);
	Fdemo_mapM01ExtractionSnapshot Snapshot;
	Snapshot.ExitType = ExitType;
	Snapshot.ExitId = Fdemo_mapM01ExtractionAuthority::ExitId(ExitType);
	return Snapshot;
}

FString Ademo_mapGameMode::GetM01ExtractionStatus(Edemo_mapM01ExitType ExitType) const
{
	return bM01ExtractionFoundationActive
		? M01ExtractionAuthority.FormatStatus(ExitType)
		: TEXT("M01撤离未激活");
}

FString Ademo_mapGameMode::GetM01ExtractionHUDText() const
{
	if (!bM01ExtractionFoundationActive) return FString();
	for (const Edemo_mapM01ExitType Type : {
		Edemo_mapM01ExitType::Regular,
		Edemo_mapM01ExitType::Boss,
		Edemo_mapM01ExitType::DiscardSpatial })
	{
		const Fdemo_mapM01ExtractionSnapshot Snapshot = M01ExtractionAuthority.GetSnapshot(Type);
		if (Snapshot.State == Edemo_mapM01ExtractionState::CountingDown || Snapshot.bInRange)
		{
			return M01ExtractionAuthority.FormatStatus(Type);
		}
	}
	return FString::Printf(
		TEXT("M01撤离 · 常规可用 | Boss%s | 空间道具%s"),
		M01ExtractionAuthority.IsBossUnlocked() ? TEXT("已解锁") : TEXT("锁定"),
		M01ExtractionAuthority.GetSnapshot(Edemo_mapM01ExitType::DiscardSpatial).bConditionSatisfied
			? TEXT("可撤离")
			: TEXT("需弃置"));
}

FString Ademo_mapGameMode::GetM01RiskHUDText() const
{
	if (!IsM01ExpeditionMap() || !bV3MissionContentActive)
	{
		return FString();
	}
	return ActiveM01RiskId.IsNone()
		? TEXT("M01区域 · 安全出生区")
		: FString::Printf(TEXT("M01区域 · %s"), *ActiveM01RiskId.ToString());
}

void Ademo_mapGameMode::SetM01RiskTier(FName RiskId)
{
	if (!bV3MissionContentActive || !IsM01ExpeditionMap()) return;
	if (RiskId != TEXT("M01.Risk.LOW")
		&& RiskId != TEXT("M01.Risk.MID")
		&& RiskId != TEXT("M01.Risk.HIGH"))
	{
		return;
	}
	if (ActiveM01RiskId != RiskId)
	{
		ActiveM01RiskId = RiskId;
		UE_LOG(Logdemo_map, Log, TEXT("M01_RISK_ENTER id=%s"), *RiskId.ToString());
	}
}

void Ademo_mapGameMode::ClearM01RiskTier(FName RiskId)
{
	if (ActiveM01RiskId == RiskId)
	{
		ActiveM01RiskId = NAME_None;
		UE_LOG(Logdemo_map, Log, TEXT("M01_RISK_EXIT id=%s"), *RiskId.ToString());
	}
}

void Ademo_mapGameMode::SetM01ExtractionInRange(Edemo_mapM01ExitType ExitType, bool bInRange)
{
	if (bM01ExtractionFoundationActive)
	{
		M01ExtractionAuthority.SetInRange(ExitType, bInRange);
	}
}

Fdemo_mapItemOperationResult Ademo_mapGameMode::RequestM01Extraction(
	Edemo_mapM01ExitType ExitType,
	APlayerController* Controller)
{
	if (!bM01ExtractionFoundationActive || !Controller || M01ExtractionAuthority.IsRunTerminal())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			TEXT("M01 extraction is not active for the current Run."),
			FGuid(),
			NAME_None,
			NAME_None,
			Fdemo_mapM01ExtractionAuthority::ExitId(ExitType));
	}
	const Fdemo_mapM01ExtractionSnapshot Before = M01ExtractionAuthority.GetSnapshot(ExitType);
	if (!Before.bInRange)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionOutOfRange,
			TEXT("Enter the M01 extraction range before interacting."));
	}

	if (ExitType == Edemo_mapM01ExitType::DiscardSpatial
		&& !Before.bConditionSatisfied)
	{
		Udemo_mapItemSubsystem* Items = PlayerItemSubsystem.Get();
		APawn* Pawn = Controller->GetPawn();
		Fdemo_mapSpatialDiscardBundle Bundle;
		TArray<Ademo_mapWorldItem*> BundleActors;
		Fdemo_mapItemOperationResult DiscardResult = Items
			? Items->DiscardSpatialItemBundle(Pawn, Bundle, BundleActors)
			: Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InteractionBlocked,
				TEXT("ItemAuthority is unavailable for spatial discard."));
		if (!DiscardResult.bSuccess) return DiscardResult;
		M01ExtractionAuthority.SetSpatialItemEquipped(false);
		UE_LOG(Logdemo_map, Log, TEXT("M01_SPATIAL_BUNDLE_DISCARDED bundle=%s members=%d"),
			*Bundle.BundleId.ToString(EGuidFormats::DigitsWithHyphens), Bundle.GetAllInstanceIds().Num());
	}

	FString Diagnostic;
	if (!M01ExtractionAuthority.BeginInteraction(ExitType, &Diagnostic))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			Diagnostic,
			FGuid(),
			NAME_None,
			NAME_None,
			Fdemo_mapM01ExtractionAuthority::ExitId(ExitType));
	}
	UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_COUNTDOWN_STARTED exit=%s seconds=3.0"),
		*Fdemo_mapM01ExtractionAuthority::ExitId(ExitType).ToString());
	return Fdemo_mapItemOperationResult::Success(
		FGuid(),
		NAME_None,
		NAME_None,
		Fdemo_mapM01ExtractionAuthority::ExitId(ExitType));
}

bool Ademo_mapGameMode::NotifyM01BossDefeated(FName BossId)
{
	const bool bFirstCommit = M01RunEnemyLedger.TryCommitBossDeath(BossId);
	const bool bUnlocked = bFirstCommit && bM01ExtractionFoundationActive
		&& M01ExtractionAuthority.NotifyBossDefeated(BossId);
	if (bUnlocked)
	{
		UE_LOG(Logdemo_map, Log, TEXT("M01_BOSS_EXIT_UNLOCKED boss=%s"), *BossId.ToString());
	}
	return bUnlocked;
}

void Ademo_mapGameMode::StartM01EnemyVisibleSmoke()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	if (!Pawn || !IsM01ExpeditionMap() || !V3ProgressionManager.IsValid()
		|| !PlayerItemSubsystem.IsValid())
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: runtime dependencies."));
		return;
	}
	if (PlayerItemSubsystem->GetRunState() != Edemo_mapRunState::Active)
	{
		if (!V3ProgressionManager->GetProfilePreparationWidget())
		{
			V3ProgressionManager->OpenProfilePreparationFromSect();
		}
		const Fdemo_mapProfileSessionBeginResult Begin =
			V3ProgressionManager->StartPreparedProfileRun();
		if (!Begin.IsRunActive())
		{
			FailAutomation(FString::Printf(
				TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: prepared Run start: %s"),
				*Begin.Diagnostic));
			return;
		}
	}
	if (!ActivateV3MissionContentForRun()
		|| !bM01EnemyContentActive
		|| M01EnemyActors.Num() != 14
		|| V3ProgressionManager->GetChests().Num() != 135)
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: runtime composition."));
		return;
	}
	int32 WoodCount = 0;
	int32 OreCount = 0;
	int32 HighValueCount = 0;
	int32 Tier1Count = 0;
	int32 Tier2Count = 0;
	int32 Tier3ResourceCount = 0;
	Ademo_mapLootChest* RepresentativeChest = nullptr;
	for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate :
		V3ProgressionManager->GetChests())
	{
		Ademo_mapLootChest* Chest = Candidate.Get();
		if (!Chest) continue;
		const bool bWood = Chest->HasRewardSourceTag(
			Fdemo_mapRewardProjectionTagIds::SourceContainerWood);
		const bool bOre = Chest->HasRewardSourceTag(
			Fdemo_mapRewardProjectionTagIds::SourceContainerOre);
		const bool bHigh = Chest->HasRewardSourceTag(
			Fdemo_mapRewardProjectionTagIds::ValueHigh);
		if (bWood) ++WoodCount;
		if (bOre) ++OreCount;
		if (bHigh) ++HighValueCount;
		if (Chest->HasRewardSourceTag(Fdemo_mapRewardProjectionTagIds::Tier1))
		{
			if (!bHigh) ++Tier1Count;
			if (bWood && !RepresentativeChest) RepresentativeChest = Chest;
		}
		if (Chest->HasRewardSourceTag(Fdemo_mapRewardProjectionTagIds::Tier2)
			&& !bHigh) ++Tier2Count;
		if (Chest->HasRewardSourceTag(Fdemo_mapRewardProjectionTagIds::Tier3)
			&& !bHigh) ++Tier3ResourceCount;
	}
	if (WoodCount != 60 || OreCount != 60 || HighValueCount != 15
		|| Tier1Count != 48 || Tier2Count != 48 || Tier3ResourceCount != 24
		|| !RepresentativeChest)
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: reward source distribution."));
		return;
	}
	APlayerController* Controller = GetDemoPlayerController();
	Pawn->SetActorLocation(
		RepresentativeChest->GetActorLocation() + FVector(-100.0f, 0.0f, 10.0f));
	const Fdemo_mapItemOperationResult OpenBegin =
		RepresentativeChest->RequestInteract(Controller);
	const Fdemo_mapRuntimeContainerResult OpenComplete =
		RepresentativeChest->CompleteActionForAutomation();
	Fdemo_mapRuntimeContainerSnapshot Snapshot =
		RepresentativeChest->GetContainerSnapshot();
	const Fdemo_mapRuntimeContainerEntrySnapshot* Entry = nullptr;
	for (const Fdemo_mapRuntimeContainerSectionSnapshot& Section : Snapshot.Sections)
	{
		if (!Section.OrderedOccupiedEntries.IsEmpty())
		{
			Entry = &Section.OrderedOccupiedEntries[0];
			break;
		}
	}
	if (!OpenBegin.bSuccess || !OpenComplete.bSuccess || !Entry)
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: representative source open."));
		return;
	}
	Fdemo_mapRuntimeContainerIntent Intent;
	Intent.ExpectedRunId = PlayerItemSubsystem->GetActiveRunId();
	Intent.ContainerId = Snapshot.ContainerId;
	Intent.ExpectedRevision = Snapshot.Revision;
	Intent.EntryId = Entry->EntryId;
	Intent.Action = Edemo_mapRuntimeContainerActionKind::BeginSearch;
	if (!V3ProgressionManager->SubmitSearchContainerIntent(Intent).bSuccess
		|| !RepresentativeChest->CompleteActionForAutomation().bSuccess)
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: representative source search."));
		return;
	}
	Snapshot = RepresentativeChest->GetContainerSnapshot();
	const Fdemo_mapRuntimeContainerEntrySnapshot* Identified = nullptr;
	for (const Fdemo_mapRuntimeContainerSectionSnapshot& Section : Snapshot.Sections)
	{
		Identified = Section.OrderedOccupiedEntries.FindByPredicate(
			[&Intent](const auto& Candidate) { return Candidate.EntryId == Intent.EntryId; });
		if (Identified) break;
	}
	const FGuid IdentifiedGuid = Identified ? Identified->ItemInstanceId : FGuid();
	CaptureT7RVisual(TEXT("01_M01_REWARD_SEARCH_IDENTIFIED.png"));
	Intent.ExpectedRevision = Snapshot.Revision;
	Intent.Action = Edemo_mapRuntimeContainerActionKind::Take;
	const Fdemo_mapRuntimeContainerResult Take =
		V3ProgressionManager->SubmitSearchContainerIntent(Intent);
	V3ProgressionManager->CloseSearchContainer(TEXT("M01RewardVisibleSmoke"), true);
	if (!IdentifiedGuid.IsValid() || !Take.bSuccess
		|| Take.ItemInstanceId != IdentifiedGuid)
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: same-GUID take."));
		return;
	}
	int32 StandardCount = 0;
	int32 EliteCount = 0;
	int32 BossCount = 0;
	TSet<FName> Archetypes;
	TSet<FName> Encounters;
	for (const TWeakObjectPtr<AActor>& Actor : M01EnemyActors)
	{
		const Udemo_mapM01EnemyIdentityComponent* Identity = Actor.IsValid()
			? Actor->FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>()
			: nullptr;
		if (!Identity || !Identity->IsConfigured())
		{
			FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: stable identity."));
			return;
		}
		const Fdemo_mapM01EnemyDefinition& Definition = Identity->GetDefinition();
		Archetypes.Add(Definition.EnemyArchetypeId);
		Encounters.Add(Definition.EncounterId);
		if (Definition.IsBoss()) ++BossCount;
		else if (Definition.IsElite()) ++EliteCount;
		else ++StandardCount;
	}
	if (StandardCount != 10 || EliteCount != 3 || BossCount != 1
		|| Archetypes.Num() != 6 || Encounters.Num() != 14)
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: composition counts."));
		return;
	}
	CaptureT7RVisual(TEXT("02_M01_ENEMY_COMPOSITION.png"));
	UE_LOG(Logdemo_map, Log,
		TEXT("M01_ENEMY_VISIBLE_SMOKE: composition=PASS standard=10 elite=3 boss=1 archetypes=6 encounters=14 sources=149 containers=135 tier1=48 tier2=48 tier3=24 high=15 wood=60 ore=60 same_guid_take=PASS."));
	M01EnemyVisibleSmokeStep = 1;
	GetWorldTimerManager().SetTimer(
		AutomationTimerHandle,
		this,
		&Ademo_mapGameMode::RunM01EnemyVisibleSmokeStep,
		0.45f,
		false);
#endif
}

void Ademo_mapGameMode::RunM01EnemyVisibleSmokeStep()
{
#if !UE_BUILD_SHIPPING
	if (!bM01EnemyContentActive || !V3ProgressionManager.IsValid())
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: content disappeared."));
		return;
	}
	if (M01EnemyVisibleSmokeStep == 1)
	{
		AActor* Skirmisher = nullptr;
		AActor* Stalker = nullptr;
		for (const TWeakObjectPtr<AActor>& Actor : M01EnemyActors)
		{
			const Udemo_mapM01EnemyIdentityComponent* Identity = Actor.IsValid()
				? Actor->FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>()
				: nullptr;
			if (!Identity) continue;
			if (!Skirmisher && Identity->GetDefinition().Archetype
				== Edemo_mapM01EnemyArchetype::StandardSkirmisher)
				Skirmisher = Actor.Get();
			if (!Stalker && Identity->GetDefinition().Archetype
				== Edemo_mapM01EnemyArchetype::EliteStalker)
				Stalker = Actor.Get();
		}
		if (!Skirmisher || !Stalker)
		{
			FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: representative enemies."));
			return;
		}
		UGameplayStatics::ApplyDamage(Skirmisher, 999.0f, nullptr, GetDemoPawn(), nullptr);
		UGameplayStatics::ApplyDamage(Stalker, 999.0f, nullptr, GetDemoPawn(), nullptr);
		M01EnemyVisibleSmokeStep = 2;
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this,
			&Ademo_mapGameMode::RunM01EnemyVisibleSmokeStep, 0.35f, false);
		return;
	}
	if (M01EnemyVisibleSmokeStep == 2)
	{
		if (V3ProgressionManager->GetCorpses().Num() < 2 || !M01Boss.IsValid()
			|| !Algo::AllOf(
				V3ProgressionManager->GetCorpses(),
				[](const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Corpse)
				{
					return Corpse.IsValid() && Corpse->UsesGeneratedReward()
						&& !Corpse->GetCorpseIdentity().IsNone();
				}))
		{
			FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: standard/elite Corpse projection."));
			return;
		}
		if (APawn* Pawn = GetDemoPawn())
		{
			Pawn->SetActorLocation(M01Boss->GetActorLocation() + FVector(-520.0f, 0.0f, 10.0f));
		}
		CaptureT7RVisual(TEXT("03_M01_ELITE_AND_BOSS.png"));
		UGameplayStatics::ApplyDamage(M01Boss.Get(), 999.0f, nullptr, GetDemoPawn(), nullptr);
		M01EnemyVisibleSmokeStep = 3;
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this,
			&Ademo_mapGameMode::RunM01EnemyVisibleSmokeStep, 0.35f, false);
		return;
	}
	const Fdemo_mapM01ExtractionSnapshot BossExit =
		GetM01ExtractionSnapshot(Edemo_mapM01ExitType::Boss);
	if (M01RunEnemyLedger.TryCommitBossDeath(TEXT("M01.Boss.Main"))
		|| !BossExit.bConditionSatisfied
		|| BossExit.State != Edemo_mapM01ExtractionState::Available
		|| V3ProgressionManager->GetCorpses().Num() < 3
		|| !Algo::AllOf(
			V3ProgressionManager->GetCorpses(),
			[](const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Corpse)
			{
				return Corpse.IsValid() && Corpse->UsesGeneratedReward()
					&& !Corpse->GetCorpseIdentity().IsNone();
			}))
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: Boss one-shot, Corpse, or extraction unlock."));
		return;
	}
	CaptureT7RVisual(TEXT("04_M01_BOSS_DEFEATED_EXIT_UNLOCKED.png"));
	const Fdemo_mapItemOperationResult Settlement =
		V3ProgressionManager->RequestSettlementAndReload(Edemo_mapRunEndReason::Abandon);
	if (!Settlement.bSuccess
		|| bM01EnemyContentActive
		|| !M01EnemyActors.IsEmpty()
		|| !V3ProgressionManager->GetCorpses().IsEmpty()
		|| !V3ProgressionManager->GetChests().IsEmpty())
	{
		FailAutomation(TEXT("M01_ENEMY_VISIBLE_SMOKE: FAIL: terminal cleanup."));
		return;
	}
	UE_LOG(Logdemo_map, Log,
		TEXT("M01_ENEMY_VISIBLE_SMOKE: PASS sources=149 generated_containers=135 search_same_guid_take=PASS generated_corpse_types=3 boss_equipment=required boss_notify_once=1 boss_exit=unlocked carry_settlement=PASS terminal_cleanup=PASS."));
	FPlatformMisc::RequestExitWithStatus(false, 0);
#endif
}

void Ademo_mapGameMode::StartM01ExtractionVisibleSmoke()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	if (!Pawn || !Controller || !PlayerItemSubsystem.IsValid()
		|| !(IsM01ExpeditionMap() ? ActivateV3MissionContentForRun() : InitializeM01ExtractionFoundation(Pawn)))
	{
		FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: runtime foundation."));
		return;
	}
	if (IsM01ExpeditionMap())
	{
		int32 PlayerStartCount = 0;
		int32 RiskZoneCount = 0;
		int32 AuthoredExitCount = 0;
		int32 LowBarrierCount = 0;
		int32 SolidWallCount = 0;
		int32 ArtPropCount = 0;
		int32 ForbiddenContentCount = 0;
		TSet<FName> ExitIds;
		TSet<FName> RiskIds;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) ++PlayerStartCount;
		for (TActorIterator<Ademo_mapM01RiskZone> It(GetWorld()); It; ++It)
		{
			++RiskZoneCount;
			RiskIds.Add(It->GetStableId());
		}
		for (TActorIterator<Ademo_mapM01ExtractionZone> It(GetWorld()); It; ++It)
		{
			if (It->IsAuthoredForM01())
			{
				++AuthoredExitCount;
				ExitIds.Add(It->GetStableId());
			}
		}
		for (TActorIterator<Ademo_mapM01GrayboxBlock> It(GetWorld()); It; ++It)
		{
			if (!It->HasExpectedCollisionContract())
			{
				FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: graybox collision contract."));
				return;
			}
			if (It->GetBlockType() == Edemo_mapM01GrayboxBlockType::LowBarrier) ++LowBarrierCount;
			if (It->GetBlockType() == Edemo_mapM01GrayboxBlockType::SolidWall) ++SolidWallCount;
		}
		for (TActorIterator<Ademo_mapM01ArtProp> It(GetWorld()); It; ++It)
		{
			++ArtPropCount;
			if (!It->HasVisualOnlyContract())
			{
				FailAutomation(TEXT("M01_INTEGRATION_VISIBLE_SMOKE: FAIL: art prop collision/navigation ownership."));
				return;
			}
		}
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("M01_ENEMY"))) continue;
			const FString ClassName = It->GetClass()->GetName();
			if (ClassName.Contains(TEXT("demo_mapV3ProgressionMarker"))
				|| ClassName.Contains(TEXT("demo_mapEncounterMarker"))
				|| ClassName.Contains(TEXT("demo_mapTrainingTarget"))
				|| ClassName.Contains(TEXT("demo_mapEnemyCharacter"))
				|| ClassName.Contains(TEXT("demo_mapRangedEnemyCharacter"))
				|| ClassName.Contains(TEXT("demo_mapHeavyEnemyCharacter"))
				|| ClassName.Contains(TEXT("demo_mapSpiritStonePickup"))
				|| ClassName.Contains(TEXT("demo_mapExitZone")))
			{
				++ForbiddenContentCount;
			}
		}
		if (PlayerStartCount != 1 || RiskZoneCount != 3 || RiskIds.Num() != 3
			|| AuthoredExitCount != 3 || ExitIds.Num() != 3
			|| LowBarrierCount < 6 || SolidWallCount < 6 || ForbiddenContentCount != 0
			|| (bM01IntegrationVisibleSmokeRequested && ArtPropCount < 40))
		{
			FailAutomation(FString::Printf(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: map contract starts=%d risks=%d/%d exits=%d/%d low_walls=%d solid_walls=%d art=%d forbidden=%d."),
				PlayerStartCount, RiskZoneCount, RiskIds.Num(), AuthoredExitCount, ExitIds.Num(), LowBarrierCount, SolidWallCount, ArtPropCount, ForbiddenContentCount));
			return;
		}
		for (const FName RiskId : RiskIds)
		{
			SetM01RiskTier(RiskId);
			if (!GetM01RiskHUDText().Contains(RiskId.ToString()))
			{
				FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: risk HUD projection."));
				return;
			}
		}
		if (bM01IntegrationVisibleSmokeRequested)
		{
			// This smoke validates the integrated routes and presentation.  Combat
			// behavior has its own visible slice; suppress it here so four staged
			// camera placements cannot be interrupted by an unrelated death/reload.
			SuppressM01EnemyContent();
			M01IntegrationArtCaptureStep = 0;
			bM01IntegrationArtCapturePending = false;
			if (USpringArmComponent* CameraBoom = Pawn->FindComponentByClass<USpringArmComponent>())
			{
				M01IntegrationOriginalCameraArmLength = CameraBoom->TargetArmLength;
				CameraBoom->TargetArmLength = 2400.0f;
			}
			RunM01IntegrationArtCaptureStep();
			return;
		}
		for (TActorIterator<Ademo_mapM01Marker> It(GetWorld()); It; ++It)
		{
			if (It->GetMarkerType() == Edemo_mapM01MarkerType::Boss)
			{
				Pawn->SetActorLocation(It->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
				break;
			}
		}
		CaptureT7RVisual(TEXT("01_M01_HIGH_BOSS_ROUTE.png"));
		UE_LOG(Logdemo_map, Log, TEXT("M01_MAP_VISIBLE_SMOKE: map=PASS risk_hud=LOW/MID/HIGH collision=PASS content_isolation=PASS."));
	}
	M01ExtractionVisibleSmokePhase = 1;
	SetM01ExtractionInRange(Edemo_mapM01ExitType::Regular, true);
	if (!RequestM01Extraction(Edemo_mapM01ExitType::Regular, Controller).bSuccess)
	{
		FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: regular flow did not start."));
	}
#endif
}

void Ademo_mapGameMode::RunM01IntegrationArtCaptureStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	if (!Pawn || !Controller || !bM01IntegrationVisibleSmokeRequested)
	{
		FailAutomation(TEXT("M01_INTEGRATION_VISIBLE_SMOKE: FAIL: capture runtime."));
		return;
	}
	struct FM01Capture
	{
		FName MarkerId;
		FName RiskId;
		FVector CameraOffset;
		float CameraYaw;
		const TCHAR* Filename;
	};
	static const FM01Capture Captures[] = {
		{ TEXT("M01.Route.Low.North"), TEXT("M01.Risk.LOW"), FVector(-1150.0f, -1730.0f, 100.0f), 0.0f, TEXT("01_M01_LOW_WOODLAND_HUD.png") },
		{ TEXT("M01.Route.Mid.Loop.North"), TEXT("M01.Risk.MID"), FVector(-580.0f, -1220.0f, 100.0f), 0.0f, TEXT("02_M01_MID_RUIN_HUD.png") },
		{ TEXT("M01.Encounter.Elite.01"), TEXT("M01.Risk.HIGH"), FVector(0.0f, -5700.0f, 100.0f), 0.0f, TEXT("03_M01_HIGH_SPIRIT_RIFT_HUD.png") },
		{ TEXT("M01.Boss.Main"), TEXT("M01.Risk.HIGH"), FVector(0.0f, 0.0f, 100.0f), 0.0f, TEXT("04_M01_BOSS_ALTAR_HUD.png") },
	};
	if (M01IntegrationArtCaptureStep < UE_ARRAY_COUNT(Captures))
	{
		const FM01Capture& Capture = Captures[M01IntegrationArtCaptureStep];
		if (bM01IntegrationArtCapturePending)
		{
			CaptureT7RVisual(Capture.Filename);
			bM01IntegrationArtCapturePending = false;
			++M01IntegrationArtCaptureStep;
			GetWorldTimerManager().SetTimer(
				AutomationTimerHandle, this,
				&Ademo_mapGameMode::RunM01IntegrationArtCaptureStep, 0.32f, false);
			return;
		}
		Ademo_mapM01Marker* TargetMarker = nullptr;
		for (TActorIterator<Ademo_mapM01Marker> It(GetWorld()); It; ++It)
		{
			if (It->GetStableId() == Capture.MarkerId)
			{
				TargetMarker = *It;
				break;
			}
		}
		if (!TargetMarker)
		{
			FailAutomation(FString::Printf(TEXT("M01_INTEGRATION_VISIBLE_SMOKE: FAIL: missing capture marker %s."), *Capture.MarkerId.ToString()));
			return;
		}
		SetM01RiskTier(Capture.RiskId);
		Pawn->SetActorLocationAndRotation(
			TargetMarker->GetActorLocation() + Capture.CameraOffset,
			FRotator(0.0f, Capture.CameraYaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Pawn->UpdateOverlaps();
		bM01IntegrationArtCapturePending = true;
		GetWorldTimerManager().SetTimer(
			AutomationTimerHandle, this,
			&Ademo_mapGameMode::RunM01IntegrationArtCaptureStep, 0.35f, false);
		return;
	}
	if (USpringArmComponent* CameraBoom = Pawn->FindComponentByClass<USpringArmComponent>())
	{
		CameraBoom->TargetArmLength = M01IntegrationOriginalCameraArmLength;
	}

	UE_LOG(Logdemo_map, Log,
		TEXT("M01_INTEGRATION_VISIBLE_SMOKE: art=PASS theme=DESOLATE_SPIRIT_MINE regions=LOW/MID/HIGH/BOSS hud=PASS."));
	M01ExtractionVisibleSmokePhase = 1;
	SetM01ExtractionInRange(Edemo_mapM01ExitType::Regular, true);
	if (!RequestM01Extraction(Edemo_mapM01ExitType::Regular, Controller).bSuccess)
	{
		FailAutomation(TEXT("M01_INTEGRATION_VISIBLE_SMOKE: FAIL: regular flow did not start."));
	}
#endif
}

void Ademo_mapGameMode::CompleteM01ExtractionVisibleSmoke(Edemo_mapM01ExitType CompletedExit)
{
#if !UE_BUILD_SHIPPING
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	APawn* Pawn = GetDemoPawn();
	if (!Controller || !Pawn || !PlayerItemSubsystem.IsValid())
	{
		FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: runtime disappeared."));
		return;
	}

	const Edemo_mapM01ExitType Expected = M01ExtractionVisibleSmokePhase == 1
		? Edemo_mapM01ExitType::Regular
		: (M01ExtractionVisibleSmokePhase == 2
			? Edemo_mapM01ExitType::Boss
			: Edemo_mapM01ExitType::DiscardSpatial);
	if (CompletedExit != Expected)
	{
		FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: wrong completion token."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_VISIBLE_SMOKE: phase=%d exit=%s PASS."),
		M01ExtractionVisibleSmokePhase,
		*Fdemo_mapM01ExtractionAuthority::ExitId(CompletedExit).ToString());
	if (IsM01ExpeditionMap())
	{
		CaptureT7RVisual(FString::Printf(TEXT("0%d_M01_EXIT_%s.png"),
			M01ExtractionVisibleSmokePhase + 1,
			*Fdemo_mapM01ExtractionAuthority::ExitLabel(CompletedExit)));
		if (M01ExtractionVisibleSmokePhase == 1 && !bM01IntegrationVisibleSmokeRequested)
		{
			DestroyM01ExtractionFoundation();
			if (!InitializeM01ExtractionFoundation(Pawn))
			{
				FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: new Run foundation reset."));
				return;
			}
			const Fdemo_mapM01ExtractionSnapshot Regular = GetM01ExtractionSnapshot(Edemo_mapM01ExitType::Regular);
			const Fdemo_mapM01ExtractionSnapshot Boss = GetM01ExtractionSnapshot(Edemo_mapM01ExitType::Boss);
			if (!Regular.bConditionSatisfied || Regular.State != Edemo_mapM01ExtractionState::Available
				|| Boss.bConditionSatisfied || Boss.State != Edemo_mapM01ExtractionState::Locked)
			{
				FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: new Run exit reset state."));
				return;
			}
			CaptureT7RVisual(TEXT("03_M01_NEW_RUN_RESET.png"));
			DeactivateV3MissionContentForPreparation();
			if (!Controller->RestoreGameplayControlForNewRun())
			{
				FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: post-settlement input restore."));
				return;
			}
			UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_VISIBLE_SMOKE: PASS regular=1 boss_locked=1 discard_condition_reused=1 return_ready=1 new_run_ready=1 repeated_p1_flows=0."));
			FPlatformMisc::RequestExitWithStatus(false, 0);
			return;
		}
	}

	DestroyM01ExtractionFoundation();
	if (M01ExtractionVisibleSmokePhase == 1)
	{
		M01ExtractionVisibleSmokePhase = 2;
		if (!InitializeM01ExtractionFoundation(Pawn))
		{
			FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: Boss flow setup."));
			return;
		}
		const Fdemo_mapM01ExtractionSnapshot ResetRegular =
			GetM01ExtractionSnapshot(Edemo_mapM01ExitType::Regular);
		const Fdemo_mapM01ExtractionSnapshot ResetBoss =
			GetM01ExtractionSnapshot(Edemo_mapM01ExitType::Boss);
		if (!ResetRegular.bConditionSatisfied
			|| ResetRegular.State != Edemo_mapM01ExtractionState::Available
			|| ResetBoss.bConditionSatisfied
			|| ResetBoss.State != Edemo_mapM01ExtractionState::Locked
			|| !NotifyM01BossDefeated(Fdemo_mapM01Ids::MainBoss))
		{
			FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: new Run reset or Boss flow setup."));
			return;
		}
		if (bM01IntegrationVisibleSmokeRequested)
		{
			UE_LOG(Logdemo_map, Log, TEXT("M01_INTEGRATION_VISIBLE_SMOKE: new_run_reset=PASS boss_unlock=PASS."));
		}
		SetM01ExtractionInRange(Edemo_mapM01ExitType::Boss, true);
		if (!RequestM01Extraction(Edemo_mapM01ExitType::Boss, Controller).bSuccess)
		{
			FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: Boss flow did not start."));
		}
		return;
	}

	if (M01ExtractionVisibleSmokePhase == 2)
	{
		M01ExtractionVisibleSmokePhase = 3;
		if (IsM01ExpeditionMap())
		{
			for (TActorIterator<Ademo_mapM01ExtractionZone> It(GetWorld()); It; ++It)
			{
				if (It->IsAuthoredForM01()
					&& It->GetExitType() == Edemo_mapM01ExitType::DiscardSpatial)
				{
					Pawn->SetActorLocation(
						It->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f),
						false, nullptr, ETeleportType::TeleportPhysics);
					Pawn->UpdateOverlaps();
					break;
				}
			}
		}
		Udemo_mapItemSubsystem* Items = PlayerItemSubsystem.Get();
		Items->ResetForAutomation();
		if (!Items->BindPlayerPawn(Pawn) || !Items->BeginRun().bSuccess)
		{
			FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: active Run item authority binding."));
			return;
		}
		TArray<FGuid> Added;
		if (!Items->AddDefinition(Fdemo_mapItemIds::BackpackLevel1, 1, &Added).bSuccess
			|| Added.Num() != 1
			|| !Items->Equip(Added[0], Fdemo_mapItemIds::BackpackSlot).bSuccess)
		{
			FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: spatial item setup."));
			return;
		}
		for (int32 Index = 0; Index < 7; ++Index)
		{
			if (!Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade, 1).bSuccess)
			{
				FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: spatial storage setup."));
				return;
			}
		}
		if (!InitializeM01ExtractionFoundation(Pawn))
		{
			FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: discard flow foundation."));
			return;
		}
		SetM01ExtractionInRange(Edemo_mapM01ExitType::DiscardSpatial, true);
		const Fdemo_mapItemOperationResult DiscardStart =
			RequestM01Extraction(Edemo_mapM01ExitType::DiscardSpatial, Controller);
		if (!DiscardStart.bSuccess)
		{
			FailAutomation(FString::Printf(
				TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: discard flow did not start: %s"),
				*DiscardStart.Diagnostic));
		}
		return;
	}

	if (bM01IntegrationVisibleSmokeRequested)
	{
		if (!V3ProgressionManager.IsValid())
		{
			FailAutomation(TEXT("M01_INTEGRATION_VISIBLE_SMOKE: FAIL: settlement authority missing."));
			return;
		}
		const Fdemo_mapItemOperationResult Settlement =
			V3ProgressionManager->RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction);
		if (!Settlement.bSuccess || bM01EnemyContentActive
			|| !M01EnemyActors.IsEmpty()
			|| !V3ProgressionManager->GetCorpses().IsEmpty()
			|| !V3ProgressionManager->GetChests().IsEmpty())
		{
			FailAutomation(TEXT("M01_INTEGRATION_VISIBLE_SMOKE: FAIL: extraction settlement cleanup."));
			return;
		}
		UE_LOG(Logdemo_map, Log,
			TEXT("M01_INTEGRATION_VISIBLE_SMOKE: PASS theme=1 low_regular=1 high_boss=1 discard_spatial=1 settlement_once=1 new_run_reset=1 duplicates=0 input_lock=0 sources=149 base_value=115500."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
		return;
	}
	DeactivateV3MissionContentForPreparation();
	if (!Controller->RestoreGameplayControlForNewRun())
	{
		FailAutomation(TEXT("M01_EXTRACTION_VISIBLE_SMOKE: FAIL: post-settlement input restore."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("M01_EXTRACTION_VISIBLE_SMOKE: PASS regular=1 boss=1 discard_spatial=1 return_ready=1 new_run_ready=1."));
	FPlatformMisc::RequestExitWithStatus(false, 0);
#endif
}

Udemo_mapSkillComponent* Ademo_mapGameMode::EnsurePlayerSkills(APawn* PlayerPawn)
{
	if (PlayerPawn == nullptr) return nullptr;
	TArray<Udemo_mapSkillComponent*> ExistingComponents;
	PlayerPawn->GetComponents<Udemo_mapSkillComponent>(ExistingComponents);
	if (ExistingComponents.Num() > 1)
	{
		UE_LOG(Logdemo_map, Error, TEXT("V2B: duplicate SkillComponents detected: %d."), ExistingComponents.Num());
		return nullptr;
	}
	Udemo_mapSkillComponent* Skills = ExistingComponents.Num() == 1 ? ExistingComponents[0] : NewObject<Udemo_mapSkillComponent>(PlayerPawn, TEXT("RuntimePlayerSkills"));
	if (Skills != nullptr && !Skills->IsRegistered()) Skills->RegisterComponent();
	PlayerSkillComponent = Skills;
	return Skills;
}

Udemo_mapShanmenSpiritEvasionComponent*
Ademo_mapGameMode::EnsurePlayerSpiritEvasion(APawn* PlayerPawn)
{
	const Fdemo_mapShanmenSpiritEvasionInstallationResult Installation =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
			Cast<ACharacter>(PlayerPawn));
	if (!Installation.IsSuccess())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("0.0.10 P10.7: Spirit Evasion installation rejected; status=%d existing=%d."),
			static_cast<int32>(Installation.Status),
			Installation.ExistingComponentCount);
		return nullptr;
	}
	return Installation.Component;
}

Udemo_mapAttributeComponent* Ademo_mapGameMode::EnsurePlayerAttributes(APawn* PlayerPawn)
{
	if (PlayerPawn == nullptr) return nullptr;
	TArray<Udemo_mapAttributeComponent*> ExistingComponents;
	PlayerPawn->GetComponents<Udemo_mapAttributeComponent>(ExistingComponents);
	if (ExistingComponents.Num() > 1)
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.3.1.0: duplicate AttributeComponents detected: %d."), ExistingComponents.Num());
		return nullptr;
	}
	Udemo_mapAttributeComponent* Attributes = ExistingComponents.Num() == 1 ? ExistingComponents[0] : NewObject<Udemo_mapAttributeComponent>(PlayerPawn, TEXT("RuntimePlayerAttributes"));
	if (Attributes != nullptr && !Attributes->IsRegistered()) Attributes->RegisterComponent();
	PlayerAttributeComponent = Attributes;
	return Attributes;
}

Udemo_mapShanmenCombatConditionComponent*
Ademo_mapGameMode::EnsurePlayerCombatConditions(APawn* PlayerPawn)
{
	if (PlayerPawn == nullptr)
	{
		return nullptr;
	}
	TArray<Udemo_mapShanmenCombatConditionComponent*> ExistingComponents;
	PlayerPawn->GetComponents<Udemo_mapShanmenCombatConditionComponent>(
		ExistingComponents);
	if (ExistingComponents.Num() > 1)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("0.0.10 P14.0: duplicate CombatConditionComponents detected: %d."),
			ExistingComponents.Num());
		return nullptr;
	}
	Udemo_mapShanmenCombatConditionComponent* Conditions =
		ExistingComponents.Num() == 1
			? ExistingComponents[0]
			: NewObject<Udemo_mapShanmenCombatConditionComponent>(
				PlayerPawn,
				TEXT("RuntimePlayerCombatConditions"));
	if (Conditions != nullptr && !Conditions->IsRegistered())
	{
		Conditions->RegisterComponent();
	}
	PlayerCombatConditionComponent = Conditions;
	return Conditions;
}

Udemo_mapPlayerHealthComponent* Ademo_mapGameMode::EnsurePlayerHealth(APawn* PlayerPawn) const
{
	if (PlayerPawn == nullptr)
	{
		return nullptr;
	}
	if (Udemo_mapPlayerHealthComponent* Existing = PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>())
	{
		return Existing;
	}
	Udemo_mapPlayerHealthComponent* Health = NewObject<Udemo_mapPlayerHealthComponent>(PlayerPawn, TEXT("RuntimePlayerHealth"));
	if (Health != nullptr)
	{
		Health->RegisterComponent();
		UE_LOG(Logdemo_map, Log, TEXT("T7: player health component initialized; health=%d/%d."), Health->GetCurrentHealth(), Health->GetMaxHealth());
	}
	return Health;
}

bool Ademo_mapGameMode::FindGroundLocation(const FVector& CandidateLocation, APawn* PlayerPawn, FVector& OutSpawnLocation) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(T5GroundTrace), false, PlayerPawn);
	QueryParams.AddIgnoredActor(PlayerPawn);
	const FVector TraceStart = CandidateLocation + FVector(0.0f, 0.0f, 1200.0f);
	const FVector TraceEnd = CandidateLocation - FVector(0.0f, 0.0f, 1800.0f);
	if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) || !GroundHit.bBlockingHit)
	{
		return false;
	}

	OutSpawnLocation = GroundHit.Location + FVector(0.0f, 0.0f, 50.0f);
	return true;
}

bool Ademo_mapGameMode::IsTargetLocationClear(const FVector& CandidateLocation, APawn* PlayerPawn) const
{
	if (PlayerPawn == nullptr || FVector::Dist2D(CandidateLocation, PlayerPawn->GetActorLocation()) < 200.0f)
	{
		return false;
	}

	for (const TWeakObjectPtr<Ademo_mapTrainingTarget>& ExistingTarget : SpawnedTargets)
	{
		if (ExistingTarget.IsValid() && FVector::Dist2D(CandidateLocation, ExistingTarget->GetActorLocation()) < 250.0f)
		{
			return false;
		}
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(T5TargetClearance), false, PlayerPawn);
	QueryParams.AddIgnoredActor(PlayerPawn);
	return !World->OverlapAnyTestByObjectType(CandidateLocation, FQuat::Identity, ObjectQueryParams, FCollisionShape::MakeSphere(120.0f), QueryParams);
}

void Ademo_mapGameMode::SpawnMissionActors(APawn* PlayerPawn)
{
	if (UsesPersistedEncounterMarkers())
	{
		if (!SpawnMissionActorsFromEncounterMarkers(PlayerPawn))
		{
			UE_LOG(Logdemo_map, Error, TEXT("V2C: required encounter markers are missing, duplicated, overlapping, or invalid; fallback spawning is forbidden on L_V2_CombatDemo."));
		}
		return;
	}

	FVector Forward = PlayerPawn->GetActorForwardVector();
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const TArray<FVector> CandidateOffsets = {
		Forward * 350.0f,
		(Forward * 500.0f) + (Right * 300.0f),
		(Forward * 500.0f) - (Right * 300.0f),
		(Forward * 700.0f) + (Right * 150.0f),
		(Forward * 700.0f) - (Right * 150.0f),
		Forward * 850.0f
	};

	for (const FVector& Offset : CandidateOffsets)
	{
		if (SpawnedTargets.Num() >= 3)
		{
			break;
		}

		FVector SpawnLocation;
		const FVector CandidateLocation = PlayerLocation + Offset;
		if (!FindGroundLocation(CandidateLocation, PlayerPawn, SpawnLocation) || !IsTargetLocationClear(SpawnLocation, PlayerPawn))
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FRotator FacingRotation = (PlayerPawn->GetActorLocation() - SpawnLocation).Rotation();
		Ademo_mapTrainingTarget* Target = GetWorld()->SpawnActor<Ademo_mapTrainingTarget>(Ademo_mapTrainingTarget::StaticClass(), SpawnLocation, FacingRotation, SpawnParameters);
		if (Target != nullptr)
		{
			Target->OnDestroyed.AddDynamic(this, &Ademo_mapGameMode::HandleTrainingTargetDestroyed);
			SpawnedTargets.Add(Target);
			UE_LOG(Logdemo_map, Log, TEXT("T5: TrainingTarget spawned; health=%d at (%.1f, %.1f, %.1f)."), Target->GetHealth(), SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
		}
	}

	SpawnExit(PlayerPawn, Forward);
	SpawnEnemy(PlayerPawn, Forward, Right);
	SpawnFriendly(PlayerPawn, Forward, Right);
}

bool Ademo_mapGameMode::IsV2CMap() const
{
	return GetWorld() != nullptr && GetWorld()->GetMapName().EndsWith(TEXT("L_V2_CombatDemo"));
}

bool Ademo_mapGameMode::IsM01ExpeditionMap() const
{
	if (GetWorld() == nullptr || !GetWorld()->GetMapName().EndsWith(TEXT("L_M01_Expedition"))) return false;
	int32 RootCount = 0;
	for (TActorIterator<Ademo_mapM01Marker> It(GetWorld()); It; ++It)
	{
		if (It->GetMarkerType() == Edemo_mapM01MarkerType::MapRoot
			&& It->GetStableId() == TEXT("M01"))
		{
			++RootCount;
		}
	}
	return RootCount == 1;
}

bool Ademo_mapGameMode::HasV3ProgressionFeature() const
{
	if (GetWorld() == nullptr) return false;
	if (IsM01ExpeditionMap()) return true;
	int32 RootCount = 0;
	for (TActorIterator<Ademo_mapV3ProgressionMarker> It(GetWorld()); It; ++It)
	{
		if (It->GetMarkerType() == Edemo_mapV3ProgressionMarkerType::ProgressionRoot) ++RootCount;
	}
	return RootCount == 1;
}

bool Ademo_mapGameMode::UsesPersistedEncounterMarkers() const
{
	return IsV2CMap() || (HasV3ProgressionFeature() && !IsM01ExpeditionMap());
}

bool Ademo_mapGameMode::InitializeV3Progression(APawn* PlayerPawn, Udemo_mapItemSubsystem* Items)
{
	if (!HasV3ProgressionFeature()) return true;
	if (V3ProgressionManager.IsValid())
	{
		return V3ProgressionManager->IsInitialized()
			&& Framework0909BHost.IsValid()
			&& Framework0909BHost->IsInitialized();
	}
	if (!GetWorld() || !PlayerPawn || !Items) return false;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapV3ProgressionManager* Manager = GetWorld()->SpawnActor<Ademo_mapV3ProgressionManager>(Ademo_mapV3ProgressionManager::StaticClass(), FTransform::Identity, Params);
	if (!Manager || !Manager->Initialize(PlayerPawn, Items, true))
	{
		if (Manager) Manager->Destroy();
		return false;
	}
	// Keep the retained Code A runtime behind this GameMode before the new
	// product host validates its narrow adapter contract.
	V3ProgressionManager = Manager;
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_map0909BFrameworkHost* Framework = GetWorld()->SpawnActor<Ademo_map0909BFrameworkHost>(
		Ademo_map0909BFrameworkHost::StaticClass(), FTransform::Identity, Params);
	if (!Framework || !Controller || !Framework->InitializeForGame(this, Controller))
	{
		if (Framework) Framework->Destroy();
		V3ProgressionManager.Reset();
		Manager->Destroy();
		return false;
	}
	Framework0909BHost = Framework;
	return true;
}

bool Ademo_mapGameMode::SpawnMissionActorsFromEncounterMarkers(APawn* PlayerPawn)
{
	if (PlayerPawn == nullptr || GetWorld() == nullptr)
	{
		return false;
	}

	TArray<AActor*> RawMarkers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapEncounterMarker::StaticClass(), RawMarkers);
	Ademo_mapEncounterMarker* FriendlyMarker = nullptr;
	Ademo_mapEncounterMarker* EnemyMarker = nullptr;
	Ademo_mapEncounterMarker* RangedMarker = nullptr;
	Ademo_mapEncounterMarker* HeavyMarker = nullptr;
	Ademo_mapEncounterMarker* ExitMarker = nullptr;
	TMap<int32, Ademo_mapEncounterMarker*> TargetMarkers;
	bool bInvalid = false;
	for (AActor* RawMarker : RawMarkers)
	{
		Ademo_mapEncounterMarker* Marker = Cast<Ademo_mapEncounterMarker>(RawMarker);
		if (Marker == nullptr)
		{
			continue;
		}
		switch (Marker->GetMarkerType())
		{
		case Edemo_mapEncounterMarkerType::FriendlySpawn:
			bInvalid |= FriendlyMarker != nullptr;
			FriendlyMarker = Marker;
			break;
		case Edemo_mapEncounterMarkerType::MeleeEnemySpawn:
			bInvalid |= EnemyMarker != nullptr;
			EnemyMarker = Marker;
			break;
		case Edemo_mapEncounterMarkerType::RangedEnemySpawn:
			bInvalid |= RangedMarker != nullptr;
			RangedMarker = Marker;
			break;
		case Edemo_mapEncounterMarkerType::HeavyEnemySpawn:
			bInvalid |= HeavyMarker != nullptr;
			HeavyMarker = Marker;
			break;
		case Edemo_mapEncounterMarkerType::ExitSpawn:
			bInvalid |= ExitMarker != nullptr;
			ExitMarker = Marker;
			break;
		case Edemo_mapEncounterMarkerType::TrainingTargetSpawn:
			bInvalid |= Marker->GetMarkerIndex() < 0 || Marker->GetMarkerIndex() > 2 || TargetMarkers.Contains(Marker->GetMarkerIndex());
			TargetMarkers.Add(Marker->GetMarkerIndex(), Marker);
			break;
		}
	}
	if (bInvalid || RawMarkers.Num() != 8 || FriendlyMarker == nullptr || EnemyMarker == nullptr || RangedMarker == nullptr || HeavyMarker == nullptr || ExitMarker == nullptr || TargetMarkers.Num() != 3 || !TargetMarkers.Contains(0) || !TargetMarkers.Contains(1) || !TargetMarkers.Contains(2))
	{
		UE_LOG(Logdemo_map, Error, TEXT("V2D: marker validation failed. Total=%d Friendly=%d Melee=%d Ranged=%d Heavy=%d Targets=%d Exit=%d Invalid=%d"), RawMarkers.Num(), FriendlyMarker != nullptr, EnemyMarker != nullptr, RangedMarker != nullptr, HeavyMarker != nullptr, TargetMarkers.Num(), ExitMarker != nullptr, bInvalid);
		return false;
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FVector Location = TargetMarkers[Index]->GetActorLocation();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ademo_mapTrainingTarget* Target = GetWorld()->SpawnActor<Ademo_mapTrainingTarget>(Ademo_mapTrainingTarget::StaticClass(), Location, (PlayerPawn->GetActorLocation() - Location).Rotation(), Params);
		if (Target == nullptr)
		{
			return false;
		}
		Target->OnDestroyed.AddDynamic(this, &Ademo_mapGameMode::HandleTrainingTargetDestroyed);
		SpawnedTargets.Add(Target);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FriendlyUnit = GetWorld()->SpawnActor<Ademo_mapFriendlyUnit>(Ademo_mapFriendlyUnit::StaticClass(), FriendlyMarker->GetActorTransform(), Params);
	const bool bV3World = HasV3ProgressionFeature();
	if (!bV3World)
	{
		Enemy = GetWorld()->SpawnActor<Ademo_mapEnemyCharacter>(Ademo_mapEnemyCharacter::StaticClass(), EnemyMarker->GetActorTransform(), Params);
		RangedEnemy = GetWorld()->SpawnActor<Ademo_mapRangedEnemyCharacter>(Ademo_mapRangedEnemyCharacter::StaticClass(), RangedMarker->GetActorTransform(), Params);
		if (RangedEnemy.IsValid())
		{
			RangedEnemy->ConfigureLegacyBehavior();
		}
		HeavyEnemy = GetWorld()->SpawnActor<Ademo_mapHeavyEnemyCharacter>(Ademo_mapHeavyEnemyCharacter::StaticClass(), HeavyMarker->GetActorTransform(), Params);
	}
	ExitZone = GetWorld()->SpawnActor<Ademo_mapExitZone>(Ademo_mapExitZone::StaticClass(), ExitMarker->GetActorTransform(), Params);
	if (!FriendlyUnit.IsValid()
		|| !ExitZone.IsValid()
		|| (!bV3World
			&& (!Enemy.IsValid()
				|| !RangedEnemy.IsValid()
				|| !HeavyEnemy.IsValid())))
	{
		return false;
	}
	ExitZone->SetExitUnlocked(false);
	ExitZone->SetExitProgress(0, 3);
	if (bV3World)
	{
		UE_LOG(Logdemo_map, Log, TEXT("P7: GameMode projected friendly, training targets, and exit; V3 Manager owns all hostile projections."));
	}
	else
	{
		UE_LOG(Logdemo_map, Log, TEXT("V2D: GameMode spawned 1 friendly, 1 melee, 1 ranged, 1 heavy, 3 training targets, and 1 exit from persisted encounter markers."));
	}
	return true;
}

void Ademo_mapGameMode::SpawnFriendly(APawn* PlayerPawn, const FVector& Forward, const FVector& Right)
{
	if (PlayerPawn == nullptr || GetWorld() == nullptr || FriendlyUnit.IsValid()) return;
	const TArray<FVector> CandidateOffsets = { Right * 350.0f, -Right * 350.0f, (Right * 300.0f) - (Forward * 250.0f), (-Right * 300.0f) - (Forward * 250.0f) };
	for (const FVector& Offset : CandidateOffsets)
	{
		FVector Location;
		if (!FindGroundLocation(PlayerPawn->GetActorLocation() + Offset, PlayerPawn, Location) || !IsTargetLocationClear(Location, PlayerPawn)) continue;
		if ((ExitZone.IsValid() && FVector::Dist2D(Location, ExitZone->GetActorLocation()) < 250.0f) || (Enemy.IsValid() && FVector::Dist2D(Location, Enemy->GetActorLocation()) < 250.0f)) continue;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FRotator FacingRotation = (PlayerPawn->GetActorLocation() - Location).Rotation();
		FriendlyUnit = GetWorld()->SpawnActor<Ademo_mapFriendlyUnit>(Ademo_mapFriendlyUnit::StaticClass(), Location, FacingRotation, Params);
		if (FriendlyUnit.IsValid())
		{
			UE_LOG(Logdemo_map, Log, TEXT("V2A: friendly spawned at (%.1f, %.1f, %.1f)."), Location.X, Location.Y, Location.Z);
			return;
		}
	}
}

void Ademo_mapGameMode::SpawnEnemy(APawn* PlayerPawn, const FVector& Forward, const FVector& Right)
{
	if (PlayerPawn == nullptr || GetWorld() == nullptr || Enemy.IsValid())
	{
		return;
	}
	const TArray<FVector> CandidateOffsets = {
		(Forward * 700.0f) + (Right * 150.0f),
		(Forward * 700.0f) - (Right * 150.0f),
		Forward * 800.0f,
		(Forward * 900.0f) + (Right * 420.0f),
		(Forward * 1000.0f) - (Right * 420.0f),
		(Forward * 800.0f) - (Right * 500.0f),
		(Forward * 1100.0f) + (Right * 250.0f)
	};
	for (const FVector& Offset : CandidateOffsets)
	{
		FVector SpawnLocation;
		if (!FindGroundLocation(PlayerPawn->GetActorLocation() + Offset, PlayerPawn, SpawnLocation) || !IsTargetLocationClear(SpawnLocation, PlayerPawn))
		{
			continue;
		}
		if (ExitZone.IsValid() && FVector::Dist2D(SpawnLocation, ExitZone->GetActorLocation()) < 350.0f)
		{
			continue;
		}
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Enemy = GetWorld()->SpawnActor<Ademo_mapEnemyCharacter>(Ademo_mapEnemyCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
		if (Enemy.IsValid())
		{
			UE_LOG(Logdemo_map, Log, TEXT("T7: enemy spawned; health=3 aggro=850 attack=135 at (%.1f, %.1f, %.1f)."), SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
			return;
		}
	}
}

void Ademo_mapGameMode::SpawnExit(APawn* PlayerPawn, const FVector& Forward)
{
	FVector SpawnLocation;
	const FVector CandidateLocation = PlayerPawn->GetActorLocation() - (Forward * 500.0f);
	if (!FindGroundLocation(CandidateLocation, PlayerPawn, SpawnLocation))
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ExitZone = GetWorld()->SpawnActor<Ademo_mapExitZone>(Ademo_mapExitZone::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
	if (ExitZone.IsValid())
	{
		ExitZone->SetExitUnlocked(false);
		ExitZone->SetExitProgress(0, 3);
		UE_LOG(Logdemo_map, Log, TEXT("T5: exit spawned locked."));
	}
}

void Ademo_mapGameMode::HandleTrainingTargetDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == nullptr || CountedTargetActors.Contains(DestroyedActor) || GetWorld() == nullptr || GetWorld()->bIsTearingDown)
	{
		return;
	}

	Ademo_mapTrainingTarget* DestroyedTarget = Cast<Ademo_mapTrainingTarget>(DestroyedActor);
	const bool bWasSpawnedByThisMode = SpawnedTargets.ContainsByPredicate([DestroyedTarget](const TWeakObjectPtr<Ademo_mapTrainingTarget>& Target)
	{
		return Target.Get() == DestroyedTarget;
	});
	if (DestroyedTarget == nullptr || !DestroyedTarget->WasDestroyedByDamage() || !bWasSpawnedByThisMode)
	{
		return;
	}

	CountedTargetActors.Add(DestroyedActor);
	Ademo_mapGameState* MissionState = Cast<Ademo_mapGameState>(GetWorld()->GetGameState());
	if (MissionState != nullptr && MissionState->RegisterTargetDestroyed() && ExitZone.IsValid())
	{
		ExitZone->SetExitProgress(MissionState->GetDestroyedTargets(), MissionState->GetRequiredTargets());
		ExitZone->SetExitUnlocked(true);
		UE_LOG(Logdemo_map, Log, TEXT("T5: exit unlocked."));
		UE_LOG(Logdemo_map, Log, TEXT("T7R: exit unlocked."));
	}
	else if (MissionState != nullptr && ExitZone.IsValid())
	{
		ExitZone->SetExitProgress(MissionState->GetDestroyedTargets(), MissionState->GetRequiredTargets());
	}
}

void Ademo_mapGameMode::HandlePlayerDefeated()
{
	RouteWeaponGuardTerminationIntent(
		Edemo_mapShanmenWeaponGuardTerminationReason::PlayerDefeated);
	M01ExtractionAuthority.NotifyPlayerDefeated();
	M01ExtractionAuthority.NotifyRunTerminal();
	UE_LOG(Logdemo_map, Log, TEXT("T7R: player defeated."));
	if (V3ProgressionManager.IsValid())
	{
		V3ProgressionManager->RequestSettlementAndReload(Edemo_mapRunEndReason::Death);
		return;
	}
	BeginReset(TEXT("DEFEATED\nRestarting..."), TEXT("T7R: defeat reset passed."));
}

void Ademo_mapGameMode::HandleExtraction()
{
	if (bM01ExtractionFoundationActive)
	{
		M01ExtractionAuthority.NotifyRunTerminal();
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7R: extraction triggered."));
	if (V3ProgressionManager.IsValid())
	{
		V3ProgressionManager->RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction);
		return;
	}
	if (bT7RVisibleAcceptanceRequested)
	{
		CaptureT7RVisual(TEXT("05_EXTRACTED.png"));
	}
	BeginReset(TEXT("EXTRACTED\nRestarting..."), TEXT("T7R: extraction reset passed."));
}

void Ademo_mapGameMode::HandleM01PlayerDamaged(int32 AppliedDamage)
{
	if (AppliedDamage <= 0)
	{
		return;
	}
	Edemo_mapShanmenWeaponGuardTerminationReason Reason =
		Edemo_mapShanmenWeaponGuardTerminationReason::EffectiveDamageStagger;
	if (const APawn* PlayerPawn = GetDemoPawn())
	{
		if (const Udemo_mapPlayerHealthComponent* Health =
			PlayerPawn->FindComponentByClass<
				Udemo_mapPlayerHealthComponent>();
			Health && Health->GetCurrentVitality() <= 0.0f)
		{
			Reason =
				Edemo_mapShanmenWeaponGuardTerminationReason::PlayerDefeated;
		}
	}
	RouteWeaponGuardTerminationIntent(Reason);
	if (bM01ExtractionFoundationActive)
	{
		M01ExtractionAuthority.NotifyEffectiveDamage();
	}
}

void Ademo_mapGameMode::BeginReset(const FString& StatusText, const TCHAR* LogMarker)
{
	if (bResetPending || GetWorld() == nullptr)
	{
		return;
	}
	bResetPending = true;
	EndStatusText = StatusText;
	if (Ademo_mapPlayerController* Controller = GetDemoPlayerController())
	{
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
	}
	if (PlayerSkillComponent.IsValid())
	{
		PlayerSkillComponent->CancelAllSkillState();
	}
	if (Enemy.IsValid())
	{
		Enemy->SetCombatSuppressed(true);
	}
	if (RangedEnemy.IsValid())
	{
		RangedEnemy->SetCombatSuppressed(true);
	}
	if (HeavyEnemy.IsValid())
	{
		HeavyEnemy->SetCombatSuppressed(true);
	}
	SuppressM01EnemyContent();
	UE_LOG(Logdemo_map, Log, TEXT("%s"), LogMarker);
	GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &Ademo_mapGameMode::ReloadDemoLevel, 1.75f, false);
}

void Ademo_mapGameMode::ReloadDemoLevel()
{
	if (GetWorld() != nullptr)
	{
		UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true);
	}
}

void Ademo_mapGameMode::StartRequestedAutomation()
{
#if !UE_BUILD_SHIPPING
	const bool bAnyLegacyAutomation = bV2CAutomationRequested || bV2CVisibleAcceptanceRequested || bV2BAutomationRequested || bV2BVisibleAcceptanceRequested || bV2AAutomationRequested || bV2AVisibleAcceptanceRequested || bT7AutomationRequested || bT7RVisibleAcceptanceRequested || bT5AutomationRequested || bT4AutomationRequested;
	if (bAnyLegacyAutomation)
	{
		if (RangedEnemy.IsValid()) RangedEnemy->SetCombatSuppressed(true);
		if (HeavyEnemy.IsValid()) HeavyEnemy->SetCombatSuppressed(true);
	}
	if (Ademo_mapPlayerController* Controller = GetDemoPlayerController())
	{
		if (Cast<Ademo_mapHUD>(Controller->GetHUD()) != nullptr)
		{
			UE_LOG(Logdemo_map, Log, TEXT("T7R: runtime HUD verified."));
			UE_LOG(Logdemo_map, Log, TEXT("T7R: health HUD visible."));
		}
	}
	if (bV3ItemCoreGameplayAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV3ItemCoreGameplayAutomation, 1.50f, false);
	}
	else if (bM01EnemyVisibleSmokeRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartM01EnemyVisibleSmoke, 1.50f, false);
	}
	else if (bM01ExtractionVisibleSmokeRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartM01ExtractionVisibleSmoke, 1.50f, false);
	}
	else if (bV3AttributesGameplayAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV3AttributesGameplayAutomation, 1.50f, false);
	}
	else if (bV2FinalAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2FinalAutomation, 1.50f, false);
	}
	else if (bV2FinalVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2FinalVisibleAcceptance, 2.00f, false);
	}
	else if (bV2EAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2EAutomation, 1.50f, false);
	}
	else if (bV2EVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2EVisibleAcceptance, 2.00f, false);
	}
	else if (bV2DAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2DAutomation, 1.50f, false);
	}
	else if (bV2DVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2DVisibleAcceptance, 2.00f, false);
	}
	else if (bV2CAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2CAutomation, 1.50f, false);
	}
	else if (bV2CVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2CVisibleAcceptance, 2.00f, false);
	}
	else if (bV2BAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2BAutomation, 0.25f, false);
	}
	else if (bV2BVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2BVisibleAcceptance, 2.00f, false);
	}
	else if (bV2AAutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2AAutomation, 0.25f, false);
	}
	else if (bV2AVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartV2AVisibleAcceptance, 2.00f, false);
	}
	else if (bT7AutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartT7Automation, 0.20f, false);
	}
	else if (bT7RVisibleAcceptanceRequested)
	{
		if (GT7RVisiblePhase == 0)
		{
			GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunT7RVisibleInitial, 0.80f, false);
		}
		else
		{
			StartT5Automation();
		}
	}
	else if (bT5AutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartT5Automation, 0.15f, false);
	}
	else if (bT4AutomationRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartT4Automation, 0.15f, false);
	}
	else if (bPackagedSmokeTestRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunPackagedSmokeTest, 0.15f, false);
	}
#endif
}

void Ademo_mapGameMode::ScheduleNextV2EAutomationStep(float Delay)
{
	++V2EAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2EAutomationStep, Delay, false);
}

void Ademo_mapGameMode::StartV2EAutomation()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	Udemo_mapPlayerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Ademo_mapGameState* Mission = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!IsV2CMap() || !Pawn || !Skills || !Health || !Mission || !Enemy.IsValid() || !RangedEnemy.IsValid() || !HeavyEnemy.IsValid() || !FriendlyUnit.IsValid() || !ExitZone.IsValid())
	{
		FailAutomation(TEXT("V2E_AUTOMATION: FAIL: lifecycle initialization or roster.")); return;
	}
	if (GV2EAutomationPhase == 1)
	{
		TArray<AActor*> Projectiles; UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapSkillProjectile::StaticClass(), Projectiles);
		if (Health->GetCurrentHealth()!=5 || Mission->GetDestroyedTargets()!=0 || ExitZone->IsExitUnlocked() || Projectiles.Num()!=0) { FailAutomation(TEXT("V2E_AUTOMATION: FAIL: extraction reset state.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V2E_AUTOMATION: extraction reset passed."));
		GV2EAutomationPhase = 2;
		UGameplayStatics::ApplyDamage(Pawn, 5.0f, Enemy->GetController(), Enemy.Get(), nullptr);
		return;
	}
	if (GV2EAutomationPhase == 2)
	{
		TArray<AActor*> Projectiles; UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapSkillProjectile::StaticClass(), Projectiles);
		if (Health->GetCurrentHealth()!=5 || Mission->GetDestroyedTargets()!=0 || ExitZone->IsExitUnlocked() || Projectiles.Num()!=0 || !RangedEnemy.IsValid() || !HeavyEnemy.IsValid()) { FailAutomation(TEXT("V2E_AUTOMATION: FAIL: defeat reset state.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V2E_AUTOMATION: lifecycle regression passed."));
		UE_LOG(Logdemo_map, Log, TEXT("V2E_AUTOMATION: PASS."));
		GV2EAutomationPhase = 0;
		FPlatformMisc::RequestExitWithStatus(false, 0);
		return;
	}

	AActor* Ground = GetV2CActorWithTag(TEXT("V2C_GROUND"));
	TArray<AActor*> Markers; UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapEncounterMarker::StaticClass(), Markers);
	const FVector GroundSize = Ground ? Ground->GetComponentsBoundingBox(true).GetSize() : FVector::ZeroVector;
	bool bAllMarkerPathsValid = Markers.Num() == 8;
	for (AActor* Marker : Markers) bAllMarkerPathsValid &= Marker != nullptr && HasValidNavigationPath(Pawn->GetActorLocation(), Marker->GetActorLocation());
	if (!Ground || !FMath::IsNearlyEqual(GroundSize.X,5040.0f,35.0f) || !FMath::IsNearlyEqual(GroundSize.Y,6580.0f,35.0f) || !bAllMarkerPathsValid)
	{
		FailAutomation(FString::Printf(TEXT("V2E_AUTOMATION: FAIL: compact map or navigation. size=(%.1f,%.1f) markers=%d paths=%d"),GroundSize.X,GroundSize.Y,Markers.Num(),bAllMarkerPathsValid?1:0)); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2E_AUTOMATION: compact map passed."));

	Enemy->SetCombatSuppressed(true); RangedEnemy->SetCombatSuppressed(true); HeavyEnemy->SetCombatSuppressed(true);
	const FVector Base(700.0f, 700.0f, Pawn->GetActorLocation().Z);
	Ademo_mapTrainingTarget* Target = GetTargetAt(0);
	if (!Target) { FailAutomation(TEXT("V2E_AUTOMATION: FAIL: first Training Target missing.")); return; }
	MoveV2BActor(Pawn, Base); Pawn->SetActorRotation(FRotator::ZeroRotator);
	MoveV2BActor(Target, FVector(Base.X+650.0f,Base.Y,Target->GetActorLocation().Z));
	MoveV2BActor(FriendlyUnit.Get(), Base+FVector(0,600,0));
	if (!Skills->TryFireStraightProjectile(FVector::ForwardVector)) { FailAutomation(TEXT("V2E_AUTOMATION: FAIL: first real player projectile did not spawn.")); return; }
	V2EAutomationStep = 0;
	ScheduleNextV2EAutomationStep(0.80f);
#endif
}

void Ademo_mapGameMode::RunV2EAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn(); Udemo_mapSkillComponent* Skills=GetV2BSkills(); Udemo_mapPlayerHealthComponent* Health=Pawn?Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>():nullptr;
	Ademo_mapGameState* Mission=GetWorld()?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr; Ademo_mapPlayerController* Controller=GetDemoPlayerController();
	if(!Pawn||!Skills||!Health||!Mission||!Controller||!RangedEnemy.IsValid()||!HeavyEnemy.IsValid()||!FriendlyUnit.IsValid()){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: runtime state disappeared."));return;}
	const FVector Base(700.0f,700.0f,Pawn->GetActorLocation().Z); const FVector RangedBase(-800.0f,1300.0f,Pawn->GetActorLocation().Z);
	auto PlaceProjectileTarget=[this,Pawn,&Base](AActor* Actor,float Distance){if(Actor)MoveV2BActor(Actor,FVector(Base.X+Distance,Base.Y,Actor->GetActorLocation().Z));MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator::ZeroRotator);};
	auto StartRangedDistance=[this,Pawn,&RangedBase](float Distance){RangedEnemy->SetCombatSuppressed(true);MoveV2BActor(RangedEnemy.Get(),FVector(RangedBase.X,RangedBase.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,FVector(RangedBase.X+Distance,RangedBase.Y,Pawn->GetActorLocation().Z));RangedEnemy->SetCombatSuppressed(false);};
	switch(V2EAutomationStep)
	{
	case 1:
		if(!GetTargetAt(0)||GetTargetAt(0)->GetHealth()!=1||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: Training Target first projectile contact."));return;}
		if(!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: Training Target second projectile spawn."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 2:
		if(GetTargetAt(0)!=nullptr||Mission->GetDestroyedTargets()!=1){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: Training Target death or mission single-count."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: player projectile training target passed."));
		PlaceProjectileTarget(Enemy.Get(),650.0f);if(!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: melee matrix spawn."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 3:
		if(!Enemy.IsValid()||Enemy->GetCurrentHealth()!=2||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: melee matrix result."));return;}
		MoveV2BActor(Enemy.Get(),Base+FVector(-900,0,0));PlaceProjectileTarget(RangedEnemy.Get(),650.0f);if(!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged matrix spawn."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 4:
		if(RangedEnemy->GetCurrentHealth()!=2||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged matrix result."));return;}
		MoveV2BActor(RangedEnemy.Get(),Base+FVector(-900,300,0));PlaceProjectileTarget(HeavyEnemy.Get(),650.0f);if(!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: heavy matrix spawn."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 5:
		if(HeavyEnemy->GetCurrentHealth()!=4||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: heavy matrix result."));return;}
		MoveV2BActor(HeavyEnemy.Get(),Base+FVector(-900,-300,0));if(!GetTargetAt(1)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: matrix Training Target missing."));return;}PlaceProjectileTarget(GetTargetAt(1),650.0f);if(!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: target matrix spawn."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 6:
		if(!GetTargetAt(1)||GetTargetAt(1)->GetHealth()!=1||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: target matrix result."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: player projectile hostile matrix passed."));
		PlaceProjectileTarget(GetTargetAt(1),650.0f);MoveV2BActor(FriendlyUnit.Get(),FVector(Base.X+330.0f,Base.Y,FriendlyUnit->GetActorLocation().Z));if(!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: friendly pass spawn."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 7:
		if(FriendlyUnit->GetCurrentHealth()!=5||GetTargetAt(1)!=nullptr||Mission->GetDestroyedTargets()!=2||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: friendly pass-through result."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: projectile friendly pass-through passed."));
		if(!GetTargetAt(2)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: world block target missing."));return;}PlaceProjectileTarget(GetTargetAt(2),650.0f);V2EBlockingWall=SpawnV2BBlockingWall(Base+FVector(400,0,55));if(!V2EBlockingWall.IsValid()||!Skills->TryFireStraightProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: world block setup."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 8:
		if(!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=2||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: projectile crossed WorldStatic."));return;}if(V2EBlockingWall.IsValid())V2EBlockingWall->Destroy();
		UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: projectile world blocking passed."));
		MoveV2BActor(RangedEnemy.Get(),FVector(Base.X,Base.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(800,0,0));MoveV2BActor(FriendlyUnit.Get(),FVector(Base.X+200,Base.Y,FriendlyUnit->GetActorLocation().Z));MoveV2BActor(GetTargetAt(2),FVector(Base.X+350,Base.Y,GetTargetAt(2)->GetActorLocation().Z));MoveV2BActor(Enemy.Get(),FVector(Base.X+470,Base.Y,Enemy->GetActorLocation().Z));MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X+610,Base.Y,HeavyEnemy->GetActorLocation().Z));V2EInitialHealth=Health->GetCurrentHealth();V2EInitialFireCount=RangedEnemy->GetTotalProjectilesFired();RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2EAutomationStep(1.80f);return;
	case 9:
		if(Health->GetCurrentHealth()!=V2EInitialHealth-1||RangedEnemy->GetTotalProjectilesFired()<=V2EInitialFireCount||FriendlyUnit->GetCurrentHealth()!=5||Enemy->GetCurrentHealth()!=2||HeavyEnemy->GetCurrentHealth()!=4||!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=2){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: enemy projectile filtering or single damage."));return;}
		RangedEnemy->SetCombatSuppressed(true);V2EInitialHealth=Health->GetCurrentHealth();MoveV2BActor(RangedEnemy.Get(),FVector(Base.X,Base.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(800,0,0));V2EBlockingWall=SpawnV2BBlockingWall(Base+FVector(400,0,55));
		{Fdemo_mapProjectileSkillParams P;P.Width=50;P.CollisionRadius=25;P.Speed=800;P.MaxDistance=1800;P.CommonParams.Damage=1;P.bPassThroughFriendlies=true;FActorSpawnParameters S;S.Owner=RangedEnemy.Get();S.Instigator=RangedEnemy.Get();S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;V2ETestProjectile=GetWorld()->SpawnActor<Ademo_mapSkillProjectile>(Ademo_mapSkillProjectile::StaticClass(),FVector(Base.X+105,Base.Y,RangedEnemy->GetActorLocation().Z+55),FRotator::ZeroRotator,S);if(V2ETestProjectile.IsValid())V2ETestProjectile->InitializeTargetedProjectile(RangedEnemy.Get(),Pawn,FVector::ForwardVector,P,FLinearColor(0.9f,0.02f,1.0f));}
		if(!V2EBlockingWall.IsValid()||!V2ETestProjectile.IsValid()){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: enemy projectile wall setup."));return;}ScheduleNextV2EAutomationStep(1.00f);return;
	case 10:
		if(Health->GetCurrentHealth()!=V2EInitialHealth||V2ETestProjectile.IsValid()){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: enemy projectile WorldStatic regression."));return;}if(V2EBlockingWall.IsValid())V2EBlockingWall->Destroy();UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: enemy projectile regression passed."));
		MoveV2BActor(FriendlyUnit.Get(),Base+FVector(0,600,0));MoveV2BActor(RangedEnemy.Get(),Base+FVector(-900,300,0));MoveV2BActor(HeavyEnemy.Get(),Base+FVector(-900,-300,0));MoveV2BActor(GetTargetAt(2),FVector(Base.X-1000,Base.Y+500,GetTargetAt(2)->GetActorLocation().Z));PlaceProjectileTarget(Enemy.Get(),150.0f);if(!Controller->TryBasicAttack()||Enemy->GetCurrentHealth()!=1){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: basic melee damage path."));return;}MoveV2BActor(Enemy.Get(),Base+FVector(-900,0,0));MoveV2BActor(GetTargetAt(2),FVector(Base.X,Base.Y+300,GetTargetAt(2)->GetActorLocation().Z));if(!Skills->TryCastGroundCircleAt(GetTargetAt(2)->GetActorLocation())||GetTargetAt(2)->GetHealth()!=1){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ground circle damage path."));return;}PlaceProjectileTarget(HeavyEnemy.Get(),200.0f);if(!Skills->TryCastSelfSector(FVector::ForwardVector)||HeavyEnemy->GetCurrentHealth()!=3){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: self sector damage path."));return;}MoveV2BActor(Enemy.Get(),FVector(Base.X+100,Base.Y,Enemy->GetActorLocation().Z));V2EInitialHealth=Health->GetCurrentHealth();Enemy->SetCombatSuppressed(false);ScheduleNextV2EAutomationStep(0.35f);return;
	case 11:
		if(Health->GetCurrentHealth()!=V2EInitialHealth-1){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: melee enemy damage path."));return;}Enemy->SetCombatSuppressed(true);MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X+300,Base.Y,HeavyEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base);V2EInitialHealth=Health->GetCurrentHealth();HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2EAutomationStep(1.15f);return;
	case 12:
		if(Health->GetCurrentHealth()!=V2EInitialHealth-1){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: heavy sector damage path."));return;}HeavyEnemy->SetCombatSuppressed(true);UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: unified damage paths passed."));Pawn->SetActorEnableCollision(false);StartRangedDistance(400.0f);V2EInitialDistance=400.0f;ScheduleNextV2EAutomationStep(0.55f);return;
	case 13:
		if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())<=V2EInitialDistance+55.0f||RangedEnemy->GetRangedState()!=Edemo_mapRangedEnemyState::Retreat){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 400."));return;}StartRangedDistance(500.0f);V2EInitialDistance=500.0f;ScheduleNextV2EAutomationStep(0.55f);return;
	case 14:
		if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())<=V2EInitialDistance+40.0f||RangedEnemy->GetRangedState()!=Edemo_mapRangedEnemyState::Retreat){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 500."));return;}StartRangedDistance(600.0f);ScheduleNextV2EAutomationStep(0.35f);return;
	case 15:
		if(RangedEnemy->GetRangedState()==Edemo_mapRangedEnemyState::Idle){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 600 dead zone."));return;}StartRangedDistance(700.0f);ScheduleNextV2EAutomationStep(0.35f);return;
	case 16:
		if(RangedEnemy->GetRangedState()==Edemo_mapRangedEnemyState::Idle){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 700."));return;}StartRangedDistance(900.0f);ScheduleNextV2EAutomationStep(0.35f);return;
	case 17:
		if(RangedEnemy->GetRangedState()==Edemo_mapRangedEnemyState::Idle){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 900."));return;}StartRangedDistance(1200.0f);V2EInitialDistance=1200.0f;ScheduleNextV2EAutomationStep(1.00f);return;
	case 18:
		if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())>=V2EInitialDistance-70.0f){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 1200 did not approach."));return;}StartRangedDistance(1600.0f);V2EInitialFireCount=RangedEnemy->GetTotalProjectilesFired();ScheduleNextV2EAutomationStep(0.35f);return;
	case 19:
		if(RangedEnemy->GetRangedState()!=Edemo_mapRangedEnemyState::Idle||RangedEnemy->GetTotalProjectilesFired()!=V2EInitialFireCount){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged distance 1600."));return;}UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: ranged distance state matrix passed."));StartRangedDistance(400.0f);V2EInitialRetreatMoves=RangedEnemy->GetRetreatMoveRequestCount();V2EInitialFireCount=RangedEnemy->GetTotalProjectilesFired();ScheduleNextV2EAutomationStep(0.65f);return;
	case 20:
		{FVector ToPlayer=Pawn->GetActorLocation()-RangedEnemy->GetActorLocation();ToPlayer.Z=0;ToPlayer.Normalize();MoveV2BActor(Pawn,RangedEnemy->GetActorLocation()+ToPlayer*350.0f);}ScheduleNextV2EAutomationStep(1.80f);return;
	case 21:
		if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())<650.0f||RangedEnemy->GetRetreatMoveRequestCount()<V2EInitialRetreatMoves+2){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: sustained retreat did not reach safe range or continue requests."));return;}ScheduleNextV2EAutomationStep(0.80f);return;
	case 22:
		if(RangedEnemy->GetTotalProjectilesFired()<=V2EInitialFireCount){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged did not fire after reaching safe range."));return;}UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: ranged sustained retreat passed."));UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: ranged fires after retreat passed."));RangedEnemy->SetCombatSuppressed(true);V2EFallbackWalls.Reset();
		{const FVector C(1800,2600,100);AActor* N=SpawnV2BBlockingWall(C+FVector(0,200,0));AActor* S=SpawnV2BBlockingWall(C+FVector(0,-200,0));AActor* E=SpawnV2BBlockingWall(C+FVector(200,0,0));AActor* W=SpawnV2BBlockingWall(C+FVector(-200,0,0));if(E)E->SetActorRotation(FRotator(0,90,0));if(W)W->SetActorRotation(FRotator(0,90,0));V2EFallbackWalls={N,S,E,W};MoveV2BActor(RangedEnemy.Get(),FVector(C.X,C.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,FVector(C.X-400,C.Y,Pawn->GetActorLocation().Z));if(UNavigationSystemV1* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->Build();}
		V2EInitialFallbackCount=RangedEnemy->GetRetreatFallbackCount();V2EInitialRejectCount=RangedEnemy->GetRetreatCandidateRejectCount();RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2EAutomationStep(2.00f);return;
	case 23:
		if(RangedEnemy->GetRetreatFallbackCount()<=V2EInitialFallbackCount||RangedEnemy->GetRetreatCandidateRejectCount()<=V2EInitialRejectCount||RangedEnemy->GetRangedState()==Edemo_mapRangedEnemyState::Idle){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: ranged retreat fallback."));return;}UE_LOG(Logdemo_map,Log,TEXT("V2E_AUTOMATION: ranged retreat fallback passed."));RangedEnemy->SetCombatSuppressed(true);for(const TWeakObjectPtr<AActor>& W:V2EFallbackWalls)if(W.IsValid())W->Destroy();V2EFallbackWalls.Reset();Pawn->SetActorEnableCollision(true);if(!GetTargetAt(2)){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: final mission target missing."));return;}PlaceProjectileTarget(GetTargetAt(2),150.0f);if(!Controller->TryBasicAttack()){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: final mission basic attack."));return;}ScheduleNextV2EAutomationStep(0.35f);return;
	case 24:
		if(GetTargetAt(2)!=nullptr||Mission->GetDestroyedTargets()!=3||!ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2E_AUTOMATION: FAIL: task completion or exit unlock."));return;}GV2EAutomationPhase=1;PlacePawnInExit(true);return;
	default:FailAutomation(TEXT("V2E_AUTOMATION: FAIL: invalid step."));return;
	}
#endif
}

void Ademo_mapGameMode::ScheduleNextV2EVisibleStep(float Delay)
{
	++V2EVisibleStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2EVisibleStep, Delay, false);
}

void Ademo_mapGameMode::StartV2EVisibleAcceptance()
{
#if !UE_BUILD_SHIPPING
	if(!IsV2CMap()||!GetDemoPawn()||!GetV2BSkills()||!RangedEnemy.IsValid()||!HeavyEnemy.IsValid()||!FriendlyUnit.IsValid()||SpawnedTargets.Num()!=3){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: roster."));return;}
	Enemy->SetCombatSuppressed(true);RangedEnemy->SetCombatSuppressed(true);HeavyEnemy->SetCombatSuppressed(true);
	MoveV2BActor(Enemy.Get(),FVector(-1800.0f,-2600.0f,Enemy->GetActorLocation().Z));
	MoveV2BActor(RangedEnemy.Get(),FVector(-2050.0f,-2600.0f,RangedEnemy->GetActorLocation().Z));
	MoveV2BActor(HeavyEnemy.Get(),FVector(-2250.0f,-2600.0f,HeavyEnemy->GetActorLocation().Z));
	V2EVisibleStep=0;RunV2EVisibleStep();
#endif
}

void Ademo_mapGameMode::RunV2EVisibleStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn();Udemo_mapSkillComponent* Skills=GetV2BSkills();if(!Pawn||!Skills){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: runtime."));return;}
	const FVector Base(700,700,Pawn->GetActorLocation().Z);const FVector RetreatBase(-800,1300,Pawn->GetActorLocation().Z);
	switch(V2EVisibleStep)
	{
	case 0:
	{
		USpringArmComponent* CameraBoom=Pawn->FindComponentByClass<USpringArmComponent>();
		if(!CameraBoom){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: overview camera boom."));return;}
		V2EOriginalCameraArmLength=CameraBoom->TargetArmLength;
		CameraBoom->TargetArmLength=5600.0f;
		MoveV2BActor(Pawn,FVector(0,0,Pawn->GetActorLocation().Z));ScheduleNextV2EVisibleStep(1.00f);return;
	}
	case 1:CaptureT7RVisual(TEXT("01_V2E_COMPACT_MAP_OVERVIEW.png"));ScheduleNextV2EVisibleStep(0.40f);return;
	case 2:{Ademo_mapTrainingTarget* T=GetTargetAt(0);USpringArmComponent* CameraBoom=Pawn->FindComponentByClass<USpringArmComponent>();if(!T||!CameraBoom){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: projectile target or camera restore."));return;}CameraBoom->TargetArmLength=V2EOriginalCameraArmLength;MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator(0,90,0));MoveV2BActor(T,FVector(Base.X,Base.Y+500,T->GetActorLocation().Z));MoveV2BActor(FriendlyUnit.Get(),Base+FVector(-400,0,0));if(!Skills->TryFireStraightProjectile(FVector::RightVector)){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: projectile spawn."));return;}ScheduleNextV2EVisibleStep(0.55f);return;}
	case 3:if(!GetTargetAt(0)||GetTargetAt(0)->GetHealth()!=1){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: projectile target result."));return;}CaptureT7RVisual(TEXT("02_V2E_PROJECTILE_HITS_TARGET.png"));ScheduleNextV2EVisibleStep(0.35f);return;
	case 4:{Ademo_mapTrainingTarget* T=GetTargetAt(1);if(!T){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: friendly target."));return;}MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator(0,90,0));MoveV2BActor(FriendlyUnit.Get(),FVector(Base.X,Base.Y+250,FriendlyUnit->GetActorLocation().Z));MoveV2BActor(T,FVector(Base.X,Base.Y+500,T->GetActorLocation().Z));if(!Skills->TryFireStraightProjectile(FVector::RightVector)){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: friendly pass spawn."));return;}ScheduleNextV2EVisibleStep(0.55f);return;}
	case 5:if(FriendlyUnit->GetCurrentHealth()!=5||!GetTargetAt(1)||GetTargetAt(1)->GetHealth()!=1){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: friendly pass result."));return;}CaptureT7RVisual(TEXT("03_V2E_PROJECTILE_FRIENDLY_PASS.png"));ScheduleNextV2EVisibleStep(0.35f);return;
	case 6:{USpringArmComponent* CameraBoom=Pawn->FindComponentByClass<USpringArmComponent>();if(!CameraBoom){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: ranged camera boom."));return;}CameraBoom->TargetArmLength=1200.0f;RangedEnemy->SetCombatSuppressed(true);MoveV2BActor(RangedEnemy.Get(),FVector(RetreatBase.X,RetreatBase.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,RetreatBase+FVector(0,-400,0));RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2EVisibleStep(0.60f);return;}
	case 7:if(RangedEnemy->GetRangedState()!=Edemo_mapRangedEnemyState::Retreat){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: retreat start."));return;}CaptureT7RVisual(TEXT("04_V2E_RANGED_RETREAT_START.png"));ScheduleNextV2EVisibleStep(1.80f);return;
	case 8:if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())<650){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: safe range."));return;}CaptureT7RVisual(TEXT("05_V2E_RANGED_REACHES_SAFE_RANGE.png"));ScheduleNextV2EVisibleStep(0.20f);return;
	case 9:if(RangedEnemy->GetTotalProjectilesFired()<1&&!RangedEnemy->HasActiveWindup()){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: fire after retreat."));return;}CaptureT7RVisual(TEXT("06_V2E_RANGED_FIRES_AFTER_RETREAT.png"));ScheduleNextV2EVisibleStep(0.35f);return;
	case 10:
	{
		RangedEnemy->SetCombatSuppressed(true);
		if(USpringArmComponent* CameraBoom=Pawn->FindComponentByClass<USpringArmComponent>())CameraBoom->TargetArmLength=V2EOriginalCameraArmLength;
		const FVector FeedbackBase(0.0f, 2200.0f, Pawn->GetActorLocation().Z);
		Ademo_mapTrainingTarget* FeedbackTarget = GetTargetAt(2);
		if (!FeedbackTarget) { FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: hit feedback target.")); return; }
		MoveV2BActor(Pawn, FeedbackBase);
		Pawn->SetActorRotation(FRotator::ZeroRotator);
		MoveV2BActor(HeavyEnemy.Get(), FVector(FeedbackBase.X+230.0f, FeedbackBase.Y-120.0f, HeavyEnemy->GetActorLocation().Z));
		MoveV2BActor(FeedbackTarget, FVector(FeedbackBase.X+230.0f, FeedbackBase.Y+120.0f, FeedbackTarget->GetActorLocation().Z));
		ScheduleNextV2EVisibleStep(0.60f);
		return;
	}
	case 11:
		if(!Skills->TryCastSelfSector(FVector::ForwardVector)){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: hit feedback cast."));return;}
		ScheduleNextV2EVisibleStep(0.08f);return;
	case 12:
		if(!HeavyEnemy.IsValid()||HeavyEnemy->GetCurrentHealth()!=4||!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=1){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: unified hit feedback result."));return;}
		CaptureT7RVisual(TEXT("07_V2E_UNIFIED_HIT_FEEDBACK.png"));ScheduleNextV2EVisibleStep(0.35f);return;
	case 13:for(int32 I=0;I<3;++I)if(Ademo_mapTrainingTarget* T=GetTargetAt(I))UGameplayStatics::ApplyDamage(T,2.0f,GetDemoPlayerController(),Pawn,nullptr);ScheduleNextV2EVisibleStep(0.40f);return;
	case 14:
	{
		const Ademo_mapGameState* Mission=GetWorld()?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr;
		if(!Mission||Mission->GetDestroyedTargets()!=3||!ExitZone.IsValid()||!ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: exit open or target count."));return;}
		MoveV2BActor(Pawn,ExitZone->GetActorLocation()+FVector(320,0,68));ScheduleNextV2EVisibleStep(0.45f);return;
	}
	case 15:CaptureT7RVisual(TEXT("08_V2E_EXIT_OPEN.png"));ScheduleNextV2EVisibleStep(0.60f);return;
	case 16:UE_LOG(Logdemo_map,Log,TEXT("V2E_VISIBLE_ACCEPTANCE: PASS."));FPlatformMisc::RequestExitWithStatus(false,0);return;
	default:FailAutomation(TEXT("V2E_VISIBLE_ACCEPTANCE: FAIL: invalid step."));return;
	}
#endif
}

void Ademo_mapGameMode::StartV2AAutomation()
{
	APawn* Pawn = GetDemoPawn();
	Ademo_mapGameState* State = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	Udemo_mapPlayerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	auto HasFaction = [](const AActor* Actor, Edemo_mapFaction ExpectedFaction)
	{
		Edemo_mapFaction ActualFaction = Edemo_mapFaction::Neutral;
		return Fdemo_mapCombatTargeting::TryGetFaction(Actor, ActualFaction) && ActualFaction == ExpectedFaction;
	};
	bool bAllTargetsHostile = SpawnedTargets.Num() == 3;
	for (const TWeakObjectPtr<Ademo_mapTrainingTarget>& Target : SpawnedTargets)
	{
		bAllTargetsHostile = bAllTargetsHostile && Target.IsValid() && HasFaction(Target.Get(), Edemo_mapFaction::Hostile);
	}
	if (!Pawn || !FriendlyUnit.IsValid() || !Enemy.IsValid() || !HasFaction(Pawn,Edemo_mapFaction::Player) || !HasFaction(FriendlyUnit.Get(),Edemo_mapFaction::Friendly) || !HasFaction(Enemy.Get(),Edemo_mapFaction::Hostile) || !bAllTargetsHostile || FriendlyUnit->GetCurrentHealth()!=5 || !State || State->GetDestroyedTargets()!=0 || !ExitZone.IsValid() || ExitZone->IsExitUnlocked() || !Health || Health->GetCurrentHealth()!=5)
	{
		FailAutomation(TEXT("V2A_AUTOMATION: FAIL: initial state.")); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2A_AUTOMATION: initial state passed."));
	AActor* NeutralActor = GetWorld()->SpawnActor<AActor>();
	Udemo_mapFactionComponent* NeutralFaction = NeutralActor ? NewObject<Udemo_mapFactionComponent>(NeutralActor, TEXT("V2AAutomationNeutralFaction")) : nullptr;
	if (NeutralFaction)
	{
		NeutralFaction->SetFaction(Edemo_mapFaction::Neutral);
		NeutralFaction->RegisterComponent();
	}
	const bool bNeutralRelationPassed = NeutralActor && NeutralFaction && Fdemo_mapCombatTargeting::ResolveRelation(Pawn,NeutralActor)==Edemo_mapTargetRelation::Neutral;
	if (NeutralActor)
	{
		NeutralActor->Destroy();
	}
	if (Fdemo_mapCombatTargeting::ResolveRelation(Pawn,Pawn)!=Edemo_mapTargetRelation::Self || Fdemo_mapCombatTargeting::ResolveRelation(Pawn,FriendlyUnit.Get())!=Edemo_mapTargetRelation::Friendly || Fdemo_mapCombatTargeting::ResolveRelation(FriendlyUnit.Get(),Pawn)!=Edemo_mapTargetRelation::Friendly || Fdemo_mapCombatTargeting::ResolveRelation(Pawn,Enemy.Get())!=Edemo_mapTargetRelation::Hostile || Fdemo_mapCombatTargeting::ResolveRelation(Enemy.Get(),Pawn)!=Edemo_mapTargetRelation::Hostile || Fdemo_mapCombatTargeting::ResolveRelation(Enemy.Get(),GetTargetAt(0))!=Edemo_mapTargetRelation::Friendly || !bNeutralRelationPassed || Fdemo_mapCombatTargeting::ResolveRelation(Pawn,ExitZone.Get())!=Edemo_mapTargetRelation::Invalid)
	{
		FailAutomation(TEXT("V2A_AUTOMATION: FAIL: relation matrix.")); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2A_AUTOMATION: relation matrix passed."));
	Ademo_mapTrainingTarget* Target=GetTargetAt(0);
	if(!PlacePawnForTarget(Target)){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: player filter placement."));return;}
	FriendlyUnit->SetActorLocation(Target->GetActorLocation()+FVector(0,65,0),false,nullptr,ETeleportType::TeleportPhysics);
	FriendlyUnit->SetActorRotation((GetDemoPawn()->GetActorLocation()-FriendlyUnit->GetActorLocation()).Rotation());
	if(!GetDemoPlayerController()->TryBasicAttack()){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: player attack."));return;}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::VerifyV2APlayerFiltering,0.10f,false);
}

void Ademo_mapGameMode::VerifyV2APlayerFiltering()
{
	if(!GetTargetAt(0) || GetTargetAt(0)->GetHealth()!=1 || !FriendlyUnit.IsValid() || FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: player target filtering."));return;}
	UE_LOG(Logdemo_map, Log, TEXT("V2A_AUTOMATION: player target filtering passed."));
	PlacePawnForEnemy(100.0f);
	FriendlyUnit->SetActorLocation(Enemy->GetActorLocation()+FVector(0,70,0),false,nullptr,ETeleportType::TeleportPhysics);
	FriendlyUnit->SetActorRotation((GetDemoPawn()->GetActorLocation()-FriendlyUnit->GetActorLocation()).Rotation());
	GetTargetAt(0)->SetActorLocation(Enemy->GetActorLocation()+FVector(0,-70,0),false,nullptr,ETeleportType::TeleportPhysics);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::VerifyV2AEnemyFiltering,0.35f,false);
}

void Ademo_mapGameMode::VerifyV2AEnemyFiltering()
{
	Udemo_mapPlayerHealthComponent* Health=GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
	if(!Health || Health->GetCurrentHealth()!=4 || FriendlyUnit->GetCurrentHealth()!=5 || Enemy->GetCurrentHealth()!=3 || !GetTargetAt(0) || GetTargetAt(0)->GetHealth()!=1){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: enemy target filtering."));return;}
	UE_LOG(Logdemo_map, Log, TEXT("V2A_AUTOMATION: enemy target filtering passed."));
	GetTargetAt(0)->SetActorLocation(Enemy->GetActorLocation()+FVector(1000,0,0),false,nullptr,ETeleportType::TeleportPhysics);
	PlacePawnForEnemy(180.0f); V2AEnemyAttackCount=0;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::RunV2AEnemyKillAttack,0.55f,false);
}

void Ademo_mapGameMode::RunV2AEnemyKillAttack()
{
	if(!Enemy.IsValid() || !GetDemoPlayerController()->TryBasicAttack()){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: hostile kill path."));return;}
	if(++V2AEnemyAttackCount<3){GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::RunV2AEnemyKillAttack,0.55f,false);return;}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::VerifyV2AMissionIdentity,0.15f,false);
}

void Ademo_mapGameMode::VerifyV2AMissionIdentity()
{
	Ademo_mapGameState* State=Cast<Ademo_mapGameState>(GetWorld()->GetGameState());
	if(!Enemy.IsValid() || !Enemy->IsDead() || !State || State->GetDestroyedTargets()!=0 || ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: ordinary hostile changed mission."));return;}
	if(!PlacePawnForTarget(GetTargetAt(0))){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: mission target placement."));return;}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::FinishV2AMissionIdentity,0.55f,false);
}

void Ademo_mapGameMode::FinishV2AMissionIdentity()
{
	if(!GetDemoPlayerController()->TryBasicAttack()){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: mission target attack."));return;}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,FTimerDelegate::CreateLambda([this](){Ademo_mapGameState* State=Cast<Ademo_mapGameState>(GetWorld()->GetGameState()); if(!State || State->GetDestroyedTargets()!=1 || FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2A_AUTOMATION: FAIL: mission identity separation."));return;} UE_LOG(Logdemo_map,Log,TEXT("V2A_AUTOMATION: mission identity separation passed.")); UE_LOG(Logdemo_map,Log,TEXT("V2A_AUTOMATION: PASS.")); FPlatformMisc::RequestExitWithStatus(false,0);}),0.30f,false);
}

void Ademo_mapGameMode::StartV2AVisibleAcceptance()
{
	Ademo_mapTrainingTarget* Target=GetTargetAt(0);
	if(!PlacePawnForTarget(Target)){FailAutomation(TEXT("V2A_VISIBLE_ACCEPTANCE: FAIL placement."));return;}
	FriendlyUnit->SetActorLocation(Target->GetActorLocation()+FVector(0,65,0),false,nullptr,ETeleportType::TeleportPhysics);
	FriendlyUnit->SetActorRotation((GetDemoPawn()->GetActorLocation()-FriendlyUnit->GetActorLocation()).Rotation());
	Enemy->SetCombatSuppressed(true);
	Enemy->SetActorLocation(Target->GetActorLocation()+FVector(0,-230,0),false,nullptr,ETeleportType::TeleportPhysics);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::CaptureV2AInitialRelations,1.20f,false);
}

void Ademo_mapGameMode::CaptureV2AInitialRelations()
{
	CaptureT7RVisual(TEXT("01_V2A_INITIAL_RELATIONS.png"));
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::CaptureV2AFriendlyFilter,0.55f,false);
}

void Ademo_mapGameMode::CaptureV2AFriendlyFilter()
{
	if(!PlacePawnForTarget(GetTargetAt(0)) || !GetDemoPlayerController()->TryBasicAttack() || FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2A_VISIBLE_ACCEPTANCE: FAIL filter."));return;}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,FTimerDelegate::CreateLambda([this]()
	{
		if (!GetTargetAt(0) || GetTargetAt(0)->GetHealth()!=1 || !FriendlyUnit.IsValid() || FriendlyUnit->GetCurrentHealth()!=5)
		{
			FailAutomation(TEXT("V2A_VISIBLE_ACCEPTANCE: FAIL post-attack state."));
			return;
		}
		CaptureT7RVisual(TEXT("02_V2A_FRIENDLY_FILTER.png"));
		GetWorldTimerManager().SetTimer(AutomationTimerHandle,FTimerDelegate::CreateLambda([](){UE_LOG(Logdemo_map,Log,TEXT("V2A_VISIBLE_ACCEPTANCE: PASS.")); FPlatformMisc::RequestExitWithStatus(false,0);}),0.50f,false);
	}),0.20f,false);
}

void Ademo_mapGameMode::CaptureT7RVisual(const FString& Filename) const
{
	if (!T7RVisualOutputDirectory.IsEmpty())
	{
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(T7RVisualOutputDirectory, Filename), false, false);
	}
}

void Ademo_mapGameMode::RunT7RVisibleInitial()
{
	CaptureT7RVisual(TEXT("01_INITIAL.png"));
	if (!PlacePawnForEnemy(100.0f))
	{
		FailAutomation(TEXT("T7R_VISIBLE_ACCEPTANCE: FAIL initial placement."));
		return;
	}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunT7RVisibleDamaged, 0.55f, false);
}

void Ademo_mapGameMode::RunT7RVisibleDamaged()
{
	CaptureT7RVisual(TEXT("02_DAMAGED.png"));
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::PollT7RVisibleDefeat, 0.20f, true);
}

void Ademo_mapGameMode::PollT7RVisibleDefeat()
{
#if !UE_BUILD_SHIPPING
	Udemo_mapPlayerHealthComponent* Health = GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (Health != nullptr && Health->IsDefeated())
	{
		GetWorldTimerManager().ClearTimer(AutomationTimerHandle);
		CaptureT7RVisual(TEXT("03_DEFEATED.png"));
		GT7RVisiblePhase = 1;
	}
#endif
}

void Ademo_mapGameMode::FinishT7RVisibleAcceptance()
{
	UE_LOG(Logdemo_map, Log, TEXT("T7R_VISIBLE_ACCEPTANCE: PASS."));
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

Ademo_mapTrainingTarget* Ademo_mapGameMode::GetTargetAt(int32 Index) const
{
	Ademo_mapTrainingTarget* Target = SpawnedTargets.IsValidIndex(Index) ? SpawnedTargets[Index].Get() : nullptr;
	return Target != nullptr && !Target->WasDestroyedByDamage() && !Target->IsActorBeingDestroyed() ? Target : nullptr;
}

Ademo_mapPlayerController* Ademo_mapGameMode::GetDemoPlayerController() const
{
	return GetWorld() != nullptr ? Cast<Ademo_mapPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
}

APawn* Ademo_mapGameMode::GetDemoPawn() const
{
	APlayerController* PlayerController = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
}

Ademo_mapEnemyCharacter* Ademo_mapGameMode::GetEnemy() const
{
	return Enemy.Get();
}

bool Ademo_mapGameMode::PlacePawnForEnemy(float Distance) const
{
	APawn* PlayerPawn = GetDemoPawn();
	Ademo_mapEnemyCharacter* Hostile = GetEnemy();
	if (PlayerPawn == nullptr || Hostile == nullptr)
	{
		return false;
	}
	const FVector EnemyLocation = Hostile->GetActorLocation();
	const FVector PawnLocation = EnemyLocation - (FVector::ForwardVector * Distance);
	const FRotator PawnRotation = (EnemyLocation - PawnLocation).Rotation();
	PlayerPawn->SetActorLocationAndRotation(PawnLocation, PawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->UpdateOverlaps();
	return true;
}

bool Ademo_mapGameMode::PlacePawnForTarget(Ademo_mapTrainingTarget* Target) const
{
	APawn* PlayerPawn = GetDemoPawn();
	if (PlayerPawn == nullptr || Target == nullptr)
	{
		return false;
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const FVector PawnLocation = TargetLocation - (FVector::ForwardVector * 180.0f);
	const FRotator PawnRotation = (TargetLocation - PawnLocation).Rotation();
	PlayerPawn->SetActorLocationAndRotation(PawnLocation, PawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->UpdateOverlaps();
	return true;
}

void Ademo_mapGameMode::PlacePawnInExit(bool bInside) const
{
	APawn* PlayerPawn = GetDemoPawn();
	if (PlayerPawn == nullptr || !ExitZone.IsValid())
	{
		return;
	}

	const FVector ExitLocation = ExitZone->GetActorLocation();
	const FVector Destination = bInside ? ExitLocation : ExitLocation + FVector(450.0f, 0.0f, 0.0f);
	PlayerPawn->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->UpdateOverlaps();
	ExitZone->GetTriggerComponent()->UpdateOverlaps();
}

void Ademo_mapGameMode::StartT4Automation()
{
	if (!PlacePawnForTarget(GetTargetAt(0)))
	{
		FailAutomation(TEXT("T4_AUTOMATION: FAIL at setup. First target or player pawn was unavailable."));
		return;
	}
	RunT4FirstAttack();
}

void Ademo_mapGameMode::RunT4FirstAttack()
{
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_mapTrainingTarget* Target = GetTargetAt(0);
	if (Controller == nullptr || Target == nullptr)
	{
		FailAutomation(TEXT("T4_AUTOMATION: FAIL at first attack. Controller or target was unavailable."));
		return;
	}

	const bool bFirstAttackApplied = Controller->TryBasicAttack();
	if (!bFirstAttackApplied || Target->GetHealth() != 1)
	{
		FailAutomation(FString::Printf(TEXT("T4_AUTOMATION: FAIL at first attack. Applied=%d Health=%d"), bFirstAttackApplied ? 1 : 0, Target->GetHealth()));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T4_AUTOMATION: first attack passed."));

	const bool bCooldownAttackApplied = Controller->TryBasicAttack();
	if (bCooldownAttackApplied || Target->GetHealth() != 1)
	{
		FailAutomation(FString::Printf(TEXT("T4_AUTOMATION: FAIL at cooldown rejection. Applied=%d Health=%d"), bCooldownAttackApplied ? 1 : 0, Target->GetHealth()));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T4_AUTOMATION: cooldown rejection passed."));
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunT4SecondAttack, 0.55f, false);
}

void Ademo_mapGameMode::RunT4SecondAttack()
{
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	if (Controller == nullptr || GetTargetAt(0) == nullptr || !Controller->TryBasicAttack())
	{
		FailAutomation(TEXT("T4_AUTOMATION: FAIL at second valid attack."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T4_AUTOMATION: second valid attack passed."));
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::FinishT4Automation, 0.05f, false);
}

void Ademo_mapGameMode::FinishT4Automation()
{
	if (GetTargetAt(0) != nullptr)
	{
		FailAutomation(TEXT("T4_AUTOMATION: FAIL at target destruction. Target still exists."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T4_AUTOMATION: target destruction passed."));
	UE_LOG(Logdemo_map, Log, TEXT("T4_AUTOMATION: PASS."));
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

void Ademo_mapGameMode::RunPackagedSmokeTest()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	const bool bExpectedInitialState = MissionState != nullptr
		&& MissionState->GetMissionPhase() == Edemo_mapMissionPhase::EliminateTargets
		&& MissionState->GetRequiredTargets() == 3
		&& MissionState->GetDestroyedTargets() == 0;
	const bool bTargetsReady = SpawnedTargets.Num() == 3
		&& SpawnedTargets[0].IsValid()
		&& SpawnedTargets[1].IsValid()
		&& SpawnedTargets[2].IsValid();
	const bool bExitLocked = ExitZone.IsValid() && !ExitZone->IsExitUnlocked();
	const Udemo_mapPlayerHealthComponent* Health = GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	const bool bEnemyReady = Enemy.IsValid() && Enemy->GetCurrentHealth() == 3 && !Enemy->IsDead();
	const bool bVariantsReady = !IsV2CMap() || (RangedEnemy.IsValid() && RangedEnemy->GetCurrentHealth() == 3 && !RangedEnemy->IsDead() && HeavyEnemy.IsValid() && HeavyEnemy->GetCurrentHealth() == 5 && !HeavyEnemy->IsDead());
	Ademo_mapPlayerController* DemoController = GetDemoPlayerController();
	const bool bHudReady = DemoController != nullptr && Cast<Ademo_mapHUD>(DemoController->GetHUD()) != nullptr;
	if (GetDemoPawn() == nullptr || DemoController == nullptr || !bExpectedInitialState || !bTargetsReady || !bExitLocked || !bHudReady || Health == nullptr || Health->GetCurrentHealth() != 5 || !bEnemyReady || !bVariantsReady)
	{
		FailPackagedSmokeTest(FString::Printf(TEXT("T6_PACKAGED_SMOKE: FAIL: Pawn=%d Controller=%d HUD=%d State=%d Targets=%d ExitLocked=%d Health=%d Enemy=%d"), GetDemoPawn() != nullptr ? 1 : 0, DemoController != nullptr ? 1 : 0, bHudReady ? 1 : 0, bExpectedInitialState ? 1 : 0, bTargetsReady ? 1 : 0, bExitLocked ? 1 : 0, Health != nullptr ? Health->GetCurrentHealth() : -1, bEnemyReady ? 1 : 0));
		return;
	}

	UE_LOG(Logdemo_map, Log, TEXT("T6_PACKAGED_SMOKE: initial runtime state passed."));
	UE_LOG(Logdemo_map, Log, TEXT("T6_PACKAGED_SMOKE: PASS."));
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

void Ademo_mapGameMode::StartT5Automation()
{
	if (Enemy.IsValid())
	{
		Enemy->SetCombatSuppressed(true);
		if (APawn* Pawn = GetDemoPawn())
		{
			Enemy->SetActorLocation(Pawn->GetActorLocation() - FVector(1000.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (MissionState == nullptr || MissionState->GetMissionPhase() != Edemo_mapMissionPhase::EliminateTargets || MissionState->GetRequiredTargets() != 3 || MissionState->GetDestroyedTargets() != 0 || SpawnedTargets.Num() != 3 || !ExitZone.IsValid() || ExitZone->IsExitUnlocked())
	{
		FailAutomation(TEXT("T5_AUTOMATION: FAIL: initial state. GameState, targets, or locked exit did not match expected values."));
		return;
	}

	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: initial state passed."));
	PlacePawnInExit(true);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyLockedExit, 0.10f, false);
}

void Ademo_mapGameMode::VerifyLockedExit()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (MissionState == nullptr || MissionState->GetMissionPhase() != Edemo_mapMissionPhase::EliminateTargets)
	{
		FailAutomation(TEXT("T5_AUTOMATION: FAIL: locked exit rejection. Mission advanced before targets were destroyed."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: locked exit rejection passed."));
	AutomationTargetIndex = 0;
	StartNextT5Target();
}

void Ademo_mapGameMode::StartNextT5Target()
{
	Ademo_mapTrainingTarget* Target = GetTargetAt(AutomationTargetIndex);
	if (!PlacePawnForTarget(Target))
	{
		FailAutomation(FString::Printf(TEXT("T5_AUTOMATION: FAIL: target %d setup."), AutomationTargetIndex + 1));
		return;
	}
	RunT5FirstAttack();
}

void Ademo_mapGameMode::RunT5FirstAttack()
{
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_mapTrainingTarget* Target = GetTargetAt(AutomationTargetIndex);
	if (Controller == nullptr || Target == nullptr || !Controller->TryBasicAttack() || Target->GetHealth() != 1)
	{
		FailAutomation(FString::Printf(TEXT("T5_AUTOMATION: FAIL: target %d first attack."), AutomationTargetIndex + 1));
		return;
	}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunT5SecondAttack, 0.55f, false);
}

void Ademo_mapGameMode::RunT5SecondAttack()
{
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_mapTrainingTarget* Target = GetTargetAt(AutomationTargetIndex);
	if (Controller == nullptr || Target == nullptr || !PlacePawnForTarget(Target) || !Controller->TryBasicAttack())
	{
		FailAutomation(FString::Printf(TEXT("T5_AUTOMATION: FAIL: target %d second attack."), AutomationTargetIndex + 1));
		return;
	}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyT5TargetDestroyed, 0.25f, false);
}

void Ademo_mapGameMode::VerifyT5TargetDestroyed()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	const int32 ExpectedCount = AutomationTargetIndex + 1;
	if (MissionState == nullptr || GetTargetAt(AutomationTargetIndex) != nullptr || MissionState->GetDestroyedTargets() != ExpectedCount)
	{
		FailAutomation(FString::Printf(TEXT("T5_AUTOMATION: FAIL: target %d count. Actual=%d Expected=%d"), ExpectedCount, MissionState != nullptr ? MissionState->GetDestroyedTargets() : -1, ExpectedCount));
		return;
	}

	if (ExpectedCount < 3)
	{
		if (MissionState->GetMissionPhase() != Edemo_mapMissionPhase::EliminateTargets)
		{
			FailAutomation(FString::Printf(TEXT("T5_AUTOMATION: FAIL: target %d changed phase too early."), ExpectedCount));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: target %d passed; count=%d."), ExpectedCount, ExpectedCount);
		++AutomationTargetIndex;
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::StartNextT5Target, 0.55f, false);
		return;
	}

	if (MissionState->GetMissionPhase() != Edemo_mapMissionPhase::ReachExit || !ExitZone.IsValid() || !ExitZone->IsExitUnlocked() || MissionState->GetDestroyedTargets() > MissionState->GetRequiredTargets())
	{
		FailAutomation(TEXT("T5_AUTOMATION: FAIL: target 3 did not unlock the exit correctly."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: target 3 passed; count=3."));
	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: exit unlocked passed."));
	if (bT7RVisibleAcceptanceRequested)
	{
		CaptureT7RVisual(TEXT("04_EXIT_OPEN.png"));
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::EnterT7RVisibleExit, 0.60f, false);
		return;
	}
	PlacePawnInExit(true);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyMissionCompletion, 0.10f, false);
}

void Ademo_mapGameMode::EnterT7RVisibleExit()
{
	PlacePawnInExit(true);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyMissionCompletion, 0.15f, false);
}

void Ademo_mapGameMode::VerifyMissionCompletion()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (MissionState == nullptr || MissionState->GetMissionPhase() != Edemo_mapMissionPhase::Complete)
	{
		FailAutomation(TEXT("T5_AUTOMATION: FAIL: mission completion. Unlocked exit overlap did not complete the mission."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: mission completion passed."));
	if (bT7RVisibleAcceptanceRequested)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::FinishT7RVisibleAcceptance, 1.00f, false);
		return;
	}
	PlacePawnInExit(false);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::ReenterExitForDuplicateCheck, 0.05f, false);
}

void Ademo_mapGameMode::ReenterExitForDuplicateCheck()
{
	PlacePawnInExit(true);
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyDuplicateCompletionProtection, 0.10f, false);
}

void Ademo_mapGameMode::VerifyDuplicateCompletionProtection()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (MissionState == nullptr || MissionState->GetMissionPhase() != Edemo_mapMissionPhase::Complete || MissionState->GetDestroyedTargets() != 3)
	{
		FailAutomation(TEXT("T5_AUTOMATION: FAIL: duplicate completion protection."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: duplicate completion protection passed."));
	UE_LOG(Logdemo_map, Log, TEXT("T5_AUTOMATION: PASS."));
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

void Ademo_mapGameMode::StartT7Automation()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	Udemo_mapPlayerHealthComponent* Health = GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (MissionState == nullptr || Health == nullptr || Health->GetCurrentHealth() != 5 || Health->IsDefeated() || !Enemy.IsValid() || Enemy->GetCurrentHealth() != 3 || SpawnedTargets.Num() != 3 || !ExitZone.IsValid() || ExitZone->IsExitUnlocked())
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: initial state did not contain a healthy player, hostile enemy, three targets, and a locked exit."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: initial state passed."));
#if !UE_BUILD_SHIPPING
	if (GT7DefeatCyclePending)
	{
		BeginT7PlayerDefeatCycle();
		return;
	}
#endif
	if (!PlacePawnForEnemy(700.0f))
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: could not place pawn for chase verification."));
		return;
	}
	T7InitialEnemyDistance = FVector::Dist2D(GetDemoPawn()->GetActorLocation(), Enemy->GetActorLocation());
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyT7Chase, 1.20f, false);
}

void Ademo_mapGameMode::VerifyT7Chase()
{
	Ademo_mapEnemyCharacter* Hostile = GetEnemy();
	APawn* PlayerPawn = GetDemoPawn();
	if (Hostile == nullptr || PlayerPawn == nullptr || Hostile->GetEnemyState() != Edemo_mapEnemyState::Chase || FVector::Dist2D(PlayerPawn->GetActorLocation(), Hostile->GetActorLocation()) >= T7InitialEnemyDistance)
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: NavMesh chase did not close distance."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: enemy chase passed."));
	if (!PlacePawnForEnemy(100.0f))
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: could not place pawn for enemy attack."));
		return;
	}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyT7EnemyAttack, 0.30f, false);
}

void Ademo_mapGameMode::VerifyT7EnemyAttack()
{
	Udemo_mapPlayerHealthComponent* Health = GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (Health == nullptr || Health->GetCurrentHealth() != 4)
	{
		FailAutomation(FString::Printf(TEXT("T7_AUTOMATION: FAIL: enemy attack. Health=%d"), Health != nullptr ? Health->GetCurrentHealth() : -1));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: enemy attack passed."));
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyT7EnemyCooldown, 0.35f, false);
}

void Ademo_mapGameMode::VerifyT7EnemyCooldown()
{
	Udemo_mapPlayerHealthComponent* Health = GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (Health == nullptr || Health->GetCurrentHealth() != 4)
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: enemy cooldown was not enforced."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: enemy cooldown passed."));
	if (!PlacePawnForEnemy(180.0f))
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: could not place pawn for enemy defeat."));
		return;
	}
	T7EnemyAttackIndex = 0;
	RunT7EnemyKillAttack();
}

void Ademo_mapGameMode::RunT7EnemyKillAttack()
{
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_mapEnemyCharacter* Hostile = GetEnemy();
	if (Controller == nullptr || Hostile == nullptr || !Controller->TryBasicAttack())
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: player attack did not damage enemy."));
		return;
	}
	++T7EnemyAttackIndex;
	if (T7EnemyAttackIndex < 3)
	{
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunT7EnemyKillAttack, 0.55f, false);
		return;
	}
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::VerifyT7EnemyDefeated, 0.10f, false);
}

void Ademo_mapGameMode::VerifyT7EnemyDefeated()
{
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!Enemy.IsValid() || !Enemy->IsDead() || MissionState == nullptr || MissionState->GetDestroyedTargets() != 0 || MissionState->GetMissionPhase() != Edemo_mapMissionPhase::EliminateTargets || !ExitZone.IsValid() || ExitZone->IsExitUnlocked())
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: enemy defeat changed mission state or enemy did not die."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: player damage and enemy death passed."));
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: mission independence passed."));
#if !UE_BUILD_SHIPPING
	GT7DefeatCyclePending = true;
#endif
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true);
}

void Ademo_mapGameMode::BeginT7PlayerDefeatCycle()
{
	if (!PlacePawnForEnemy(100.0f))
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: could not start player defeat cycle."));
		return;
	}
	T7DefeatDeadline = GetWorld()->GetTimeSeconds() + 8.0f;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::PollT7PlayerDefeat, 0.20f, true);
}

void Ademo_mapGameMode::PollT7PlayerDefeat()
{
	APawn* PlayerPawn = GetDemoPawn();
	Udemo_mapPlayerHealthComponent* Health = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (Health == nullptr)
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: player health component disappeared."));
		return;
	}
	if (!Health->IsDefeated())
	{
		if (GetWorld()->GetTimeSeconds() > T7DefeatDeadline)
		{
			FailAutomation(TEXT("T7_AUTOMATION: FAIL: player was not defeated after five hostile attacks."));
		}
		return;
	}
	GetWorldTimerManager().ClearTimer(AutomationTimerHandle);
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_mapGameState* MissionState = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	PlacePawnInExit(true);
	if (Health->GetCurrentHealth() != 0 || Controller == nullptr || Controller->IsGameplayInputAllowed() || Controller->TryBasicAttack() || MissionState == nullptr || MissionState->GetMissionPhase() != Edemo_mapMissionPhase::EliminateTargets || MissionState->GetDestroyedTargets() != 0)
	{
		FailAutomation(TEXT("T7_AUTOMATION: FAIL: defeated input, exit, or mission protections failed."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: player defeat, input rejection, and health clamp passed."));
	UE_LOG(Logdemo_map, Log, TEXT("T7_AUTOMATION: PASS."));
#if !UE_BUILD_SHIPPING
	GT7DefeatCyclePending = false;
#endif
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

Udemo_mapSkillComponent* Ademo_mapGameMode::GetV2BSkills() const
{
	return GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapSkillComponent>() : nullptr;
}

void Ademo_mapGameMode::MoveV2BActor(AActor* Actor, const FVector& Location) const
{
	if (Actor == nullptr) return;
	Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	Actor->UpdateOverlaps();
}

Ademo_mapEnemyCharacter* Ademo_mapGameMode::SpawnV2BTestEnemy(const FVector& Location)
{
	if (GetWorld() == nullptr) return nullptr;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapEnemyCharacter* TestEnemy = GetWorld()->SpawnActor<Ademo_mapEnemyCharacter>(Ademo_mapEnemyCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (TestEnemy != nullptr)
	{
		TestEnemy->SetCombatSuppressed(true);
		V2BTestEnemies.Add(TestEnemy);
	}
	return TestEnemy;
}

AActor* Ademo_mapGameMode::SpawnV2BBlockingWall(const FVector& Location)
{
	if (GetWorld() == nullptr) return nullptr;
	AActor* Wall = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (Wall == nullptr) return nullptr;
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall, TEXT("V2BWorldStaticWall"));
	Wall->SetRootComponent(Box);
	Wall->AddInstanceComponent(Box);
	Box->SetBoxExtent(FVector(150.0f, 35.0f, 150.0f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->SetGenerateOverlapEvents(true);
	Box->RegisterComponent();
	Wall->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	Box->UpdateOverlaps();
	V2BBlockingWall = Wall;
	return Wall;
}

void Ademo_mapGameMode::ScheduleNextV2BAutomationStep(float Delay)
{
	++V2BAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2BAutomationStep, Delay, false);
}

void Ademo_mapGameMode::StartV2BAutomation()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	Udemo_mapPlayerHealthComponent* Health = Pawn != nullptr ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Ademo_mapGameState* State = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (Pawn == nullptr || Skills == nullptr || Health == nullptr || State == nullptr)
	{
		FailAutomation(TEXT("V2B_AUTOMATION: FAIL: lifecycle initialization."));
		return;
	}

	if (GV2BAutomationPhase == 1)
	{
		if (!Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady() || Skills->IsGroundCircleTargeting() || Health->GetCurrentHealth()!=5 || State->GetDestroyedTargets()!=0)
		{
			FailAutomation(TEXT("V2B_AUTOMATION: FAIL: extraction reset did not restore READY state.")); return;
		}
		GV2BAutomationPhase = 2;
		for (int32 Hit=0; Hit<5; ++Hit)
		{
			UGameplayStatics::ApplyDamage(Pawn, 1.0f, Enemy.IsValid() ? Enemy->GetController() : nullptr, Enemy.Get(), nullptr);
		}
		const bool bRejected = Health->IsDefeated() && !Skills->BeginGroundCircleTargeting() && !Skills->TryCastGroundCircleAt(Pawn->GetActorLocation()+FVector(300,0,0)) && !Skills->TryCastSelfSector(FVector::ForwardVector) && Skills->TryFireStraightProjectile(FVector::ForwardVector)==nullptr && !Skills->IsGroundCircleTargeting();
		if (!bRejected)
		{
			FailAutomation(TEXT("V2B_AUTOMATION: FAIL: defeated skill rejection.")); return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V2B_AUTOMATION: defeated input rejection passed."));
		return;
	}
	if (GV2BAutomationPhase == 2)
	{
		if (!Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady() || Skills->IsGroundCircleTargeting())
		{
			FailAutomation(TEXT("V2B_AUTOMATION: FAIL: defeat reset did not restore READY state.")); return;
		}
		GV2BAutomationPhase = 3;
		GetDemoPlayerController()->ReloadCurrentLevel();
		return;
	}
	if (GV2BAutomationPhase == 3)
	{
		if (!Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady() || Skills->IsGroundCircleTargeting() || State->GetDestroyedTargets()!=0)
		{
			FailAutomation(TEXT("V2B_AUTOMATION: FAIL: R reload state.")); return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V2B_AUTOMATION: lifecycle rejection and reset passed."));
		UE_LOG(Logdemo_map, Log, TEXT("V2B_AUTOMATION: PASS."));
		GV2BAutomationPhase = 0;
		FPlatformMisc::RequestExitWithStatus(false,0);
		return;
	}

	TArray<Udemo_mapSkillComponent*> SkillComponents;
	Pawn->GetComponents<Udemo_mapSkillComponent>(SkillComponents);
	TArray<AActor*> Allies;
	UGameplayStatics::GetAllActorsOfClass(this, Ademo_mapFriendlyUnit::StaticClass(), Allies);
	const Fdemo_mapCircleSkillParams& Circle = Skills->GetCircleParams();
	const Fdemo_mapConeSkillParams& Cone = Skills->GetConeParams();
	const Fdemo_mapProjectileSkillParams& Projectile = Skills->GetProjectileParams();
	const bool bParametersValid = FMath::IsNearlyEqual(Circle.CastRange,900.0f) && FMath::IsNearlyEqual(Circle.Radius,250.0f) && FMath::IsNearlyEqual(Circle.CommonParams.Cooldown,1.5f) && FMath::IsNearlyEqual(Circle.CommonParams.VerticalTolerance,180.0f) && FMath::IsNearlyEqual(Cone.Radius,350.0f) && FMath::IsNearlyEqual(Cone.FullAngleDegrees,90.0f) && FMath::IsNearlyEqual(Cone.CommonParams.Cooldown,1.0f) && FMath::IsNearlyEqual(Cone.CommonParams.VerticalTolerance,180.0f) && FMath::IsNearlyEqual(Projectile.Width,70.0f) && FMath::IsNearlyEqual(Projectile.CollisionRadius,35.0f) && FMath::IsNearlyEqual(Projectile.Speed,1000.0f) && FMath::IsNearlyEqual(Projectile.MaxDistance,1400.0f) && FMath::IsNearlyEqual(Projectile.CommonParams.Cooldown,0.65f) && FMath::IsNearlyEqual(Projectile.CommonParams.VerticalTolerance,180.0f) && !Projectile.bPierceHostiles && Projectile.bPassThroughFriendlies;
	if (SkillComponents.Num()!=1 || Allies.Num()!=1 || !FriendlyUnit.IsValid() || FriendlyUnit->GetCurrentHealth()!=5 || !Enemy.IsValid() || SpawnedTargets.Num()!=3 || !GetTargetAt(0) || !GetTargetAt(1) || !GetTargetAt(2) || !bParametersValid || !Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady() || State->GetDestroyedTargets()!=0 || !ExitZone.IsValid() || ExitZone->IsExitUnlocked() || Health->GetCurrentHealth()!=5)
	{
		FailAutomation(TEXT("V2B_AUTOMATION: FAIL: initial state or parameters.")); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2B_AUTOMATION: initial state passed."));

	V2BBaseLocation = Pawn->GetActorLocation();
	Enemy->SetCombatSuppressed(true);
	MoveV2BActor(Enemy.Get(), V2BBaseLocation+FVector(-1200,0,0));
	MoveV2BActor(FriendlyUnit.Get(), V2BBaseLocation+FVector(-900,500,0));
	for (int32 Index=0; Index<3; ++Index) MoveV2BActor(GetTargetAt(Index), V2BBaseLocation+FVector(-1000, (Index-1)*350.0f, 0));
	const FVector OverRangePoint = V2BBaseLocation + FVector(1000,0,0);
	if (Skills->TryCastGroundCircleAt(OverRangePoint) || !Skills->IsCircleReady())
	{
		FailAutomation(TEXT("V2B_AUTOMATION: FAIL: over-range circle was accepted or started cooldown.")); return;
	}
	const FVector LegalEmptyPoint = V2BBaseLocation + FVector(0,-650,0);
	if (!Skills->TryCastGroundCircleAt(LegalEmptyPoint) || Skills->GetCircleCooldownRemaining()<1.3f || FVector::Dist2D(V2BBaseLocation,OverRangePoint)<=Circle.CastRange)
	{
		FailAutomation(TEXT("V2B_AUTOMATION: FAIL: legal circle range or no-clamp rule.")); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2B_AUTOMATION: circle cast range passed."));
	V2BAutomationStep = 0;
	ScheduleNextV2BAutomationStep(1.55f);
#endif
}

void Ademo_mapGameMode::RunV2BAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn(); Udemo_mapSkillComponent* Skills=GetV2BSkills(); Ademo_mapGameState* State=GetWorld()!=nullptr?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr; Udemo_mapPlayerHealthComponent* Health=Pawn?Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>():nullptr;
	if(!Pawn||!Skills||!State||!Health||!FriendlyUnit.IsValid()){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: runtime state disappeared."));return;}
	const FVector X=FVector::ForwardVector; const FVector Y=FVector::RightVector;
	switch(V2BAutomationStep)
	{
	case 1:
	{
		const FVector Center=V2BBaseLocation+X*400.0f;
		MoveV2BActor(GetTargetAt(0),Center); MoveV2BActor(GetTargetAt(1),Center+X*400.0f); MoveV2BActor(GetTargetAt(2),V2BBaseLocation-X*1000.0f); MoveV2BActor(FriendlyUnit.Get(),Center+Y*80.0f);
		V2BHighEnemy=SpawnV2BTestEnemy(Center+FVector(0,0,250)); if(V2BHighEnemy.IsValid()) V2BHighEnemy->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		if(!V2BHighEnemy.IsValid()||!Skills->TryCastGroundCircleAt(Center)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: circle filtering cast."));return;}
		ScheduleNextV2BAutomationStep(0.15f); return;
	}
	case 2:
		if(!GetTargetAt(0)||GetTargetAt(0)->GetHealth()!=1||!GetTargetAt(1)||GetTargetAt(1)->GetHealth()!=2||FriendlyUnit->GetCurrentHealth()!=5||Health->GetCurrentHealth()!=5||!V2BHighEnemy.IsValid()||V2BHighEnemy->GetCurrentHealth()!=3||Skills->GetCircleCooldownRemaining()<1.1f){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: circle targeting, filtering, single-hit, or vertical tolerance."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: circle targeting and filtering passed."));
		MoveV2BActor(Enemy.Get(),V2BBaseLocation+X*200.0f); V2BBoundaryEnemy=SpawnV2BTestEnemy(V2BBaseLocation+FVector(141.421f,141.421f,0)); V2BOutsideAngleEnemy=SpawnV2BTestEnemy(V2BBaseLocation+FVector(100.0f,173.205f,0)); V2BBehindEnemy=SpawnV2BTestEnemy(V2BBaseLocation-X*200.0f); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+FVector(140,70,0));
		for(int32 I=0;I<3;++I) MoveV2BActor(GetTargetAt(I),V2BBaseLocation+FVector(-1000,I*300.0f,0)); if(V2BHighEnemy.IsValid()) MoveV2BActor(V2BHighEnemy.Get(),V2BBaseLocation+FVector(-1200,-500,0));
		if(!V2BBoundaryEnemy.IsValid()||!V2BOutsideAngleEnemy.IsValid()||!V2BBehindEnemy.IsValid()||!Skills->TryCastSelfSector(X)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: cone cast."));return;}
		ScheduleNextV2BAutomationStep(0.15f); return;
	case 3:
		if(!Enemy.IsValid()||Enemy->GetCurrentHealth()!=2||V2BBoundaryEnemy->GetCurrentHealth()!=2||V2BOutsideAngleEnemy->GetCurrentHealth()!=3||V2BBehindEnemy->GetCurrentHealth()!=3||FriendlyUnit->GetCurrentHealth()!=5||Health->GetCurrentHealth()!=5||Skills->GetConeCooldownRemaining()<0.7f){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: cone geometry, 45-degree boundary, or filtering."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: cone geometry and filtering passed."));
		for(int32 I=0;I<V2BTestEnemies.Num();++I) if(V2BTestEnemies[I].IsValid()) MoveV2BActor(V2BTestEnemies[I].Get(),V2BBaseLocation+FVector(-1400,I*250.0f,0)); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+FVector(-900,500,0)); MoveV2BActor(Enemy.Get(),V2BBaseLocation+X*200.0f); ScheduleNextV2BAutomationStep(0.90f); return;
	case 4:
		if(!Skills->TryCastSelfSector(X)||!Enemy.IsValid()||Enemy->GetCurrentHealth()!=1){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: ordinary hostile sector damage 2."));return;} ScheduleNextV2BAutomationStep(1.05f); return;
	case 5:
		if(!Skills->TryCastSelfSector(X)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: ordinary hostile sector damage 3."));return;} ScheduleNextV2BAutomationStep(0.20f); return;
	case 6:
	{
		if(!Enemy.IsValid()||!Enemy->IsDead()||State->GetDestroyedTargets()!=0||ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: ordinary hostile changed mission."));return;}
		Pawn->SetActorEnableCollision(false); MoveV2BActor(Pawn,V2BBaseLocation+FVector(0,0,800)); if(ACharacter* Character=Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Ademo_mapSkillProjectile* Projectile=Skills->TryFireStraightProjectile(X); if(!Projectile){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: projectile range spawn."));return;} V2BProjectileStartLocation=Projectile->GetActorLocation(); ScheduleNextV2BAutomationStep(0.45f); return;
	}
	case 7:
	{
		Ademo_mapSkillProjectile* Projectile=Skills->GetLastSpawnedProjectile(); const float Distance=Projectile?FVector::Dist(Projectile->GetActorLocation(),V2BProjectileStartLocation):0.0f;
		if(!Projectile||Distance<320.0f||Distance>620.0f||!FMath::IsNearlyEqual(Projectile->GetConfiguredSpeed(),1000.0f)||!FMath::IsNearlyEqual(Projectile->GetConfiguredWidth(),70.0f)||!FMath::IsNearlyEqual(Projectile->GetConfiguredCollisionRadius(),35.0f)||!FMath::IsNearlyEqual(Projectile->GetConfiguredMaxDistance(),1400.0f)||Health->GetCurrentHealth()!=5){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: projectile speed, width, or owner immunity."));return;} ScheduleNextV2BAutomationStep(1.10f); return;
	}
	case 8:
		if(Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: projectile exceeded maximum lifetime."));return;} MoveV2BActor(Pawn,V2BBaseLocation); Pawn->SetActorEnableCollision(true); if(ACharacter* Character=Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking); UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: projectile speed and range passed.")); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+X*330.0f); MoveV2BActor(GetTargetAt(2),V2BBaseLocation+X*650.0f); if(!Skills->TryFireStraightProjectile(X)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: friendly pass projectile spawn."));return;} ScheduleNextV2BAutomationStep(0.80f); return;
	case 9:
		if(!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=1||FriendlyUnit->GetCurrentHealth()!=5||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: projectile friendly pass-through."));return;} UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: projectile friendly pass-through passed.")); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+FVector(-900,500,0)); MoveV2BActor(GetTargetAt(2),V2BBaseLocation+FVector(-1000,-500,0)); V2BBlockingWall=SpawnV2BBlockingWall(V2BBaseLocation+Y*400.0f+FVector(0,0,55)); V2BWallEnemy=SpawnV2BTestEnemy(V2BBaseLocation+Y*650.0f); if(!V2BBlockingWall.IsValid()||!V2BWallEnemy.IsValid()||!Skills->TryFireStraightProjectile(Y)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: world collision setup."));return;} ScheduleNextV2BAutomationStep(0.80f); return;
	case 10:
		if(Skills->GetLastSpawnedProjectile()!=nullptr||!V2BWallEnemy.IsValid()||V2BWallEnemy->GetCurrentHealth()!=3){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: projectile crossed WorldStatic."));return;} UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: projectile world collision passed.")); MoveV2BActor(V2BWallEnemy.Get(),V2BBaseLocation+FVector(-1500,-800,0)); MoveV2BActor(GetTargetAt(0),V2BBaseLocation+Y*500.0f); if(!Skills->TryCastGroundCircleAt(GetTargetAt(0)->GetActorLocation())){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission circle cast."));return;} ScheduleNextV2BAutomationStep(0.20f); return;
	case 11:
		if(GetTargetAt(0)!=nullptr||State->GetDestroyedTargets()!=1){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission circle target."));return;} MoveV2BActor(GetTargetAt(1),V2BBaseLocation+X*200.0f); if(!Skills->TryCastSelfSector(X)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission sector first cast."));return;} ScheduleNextV2BAutomationStep(1.05f); return;
	case 12:
		if(!GetTargetAt(1)||GetTargetAt(1)->GetHealth()!=1||!Skills->TryCastSelfSector(X)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission sector second cast."));return;} ScheduleNextV2BAutomationStep(0.20f); return;
	case 13:
		if(GetTargetAt(1)!=nullptr||State->GetDestroyedTargets()!=2){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission sector target."));return;} MoveV2BActor(GetTargetAt(2),V2BBaseLocation+X*650.0f); if(!Skills->TryFireStraightProjectile(X)){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission projectile cast."));return;} ScheduleNextV2BAutomationStep(0.80f); return;
	case 14:
		if(GetTargetAt(2)!=nullptr||State->GetDestroyedTargets()!=3||!ExitZone.IsValid()||!ExitZone->IsExitUnlocked()||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: mission integration or exit unlock."));return;} UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: mission integration passed.")); PlacePawnInExit(true); ScheduleNextV2BAutomationStep(0.20f); return;
	case 15:
	{
		const bool bRejected=State->GetMissionPhase()==Edemo_mapMissionPhase::Complete&&bResetPending&&!Skills->BeginGroundCircleTargeting()&&!Skills->TryCastGroundCircleAt(V2BBaseLocation+X*300.0f)&&!Skills->TryCastSelfSector(X)&&Skills->TryFireStraightProjectile(X)==nullptr&&!Skills->IsGroundCircleTargeting(); if(!bRejected){FailAutomation(TEXT("V2B_AUTOMATION: FAIL: extraction skill rejection."));return;} UE_LOG(Logdemo_map,Log,TEXT("V2B_AUTOMATION: extraction input rejection passed.")); GV2BAutomationPhase=1; return;
	}
	default: FailAutomation(TEXT("V2B_AUTOMATION: FAIL: invalid automation step.")); return;
	}
#endif
}

void Ademo_mapGameMode::ScheduleNextV2BVisibleStep(float Delay)
{
	++V2BVisibleStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::RunV2BVisibleStep,Delay,false);
}

void Ademo_mapGameMode::StartV2BVisibleAcceptance()
{
	APawn* Pawn=GetDemoPawn(); Udemo_mapSkillComponent* Skills=GetV2BSkills();
	if(!Pawn||!Skills||!FriendlyUnit.IsValid()||!Enemy.IsValid()||!GetTargetAt(0)||!GetTargetAt(1)||!GetTargetAt(2)){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: initial state."));return;}
	V2BBaseLocation=Pawn->GetActorLocation(); Enemy->SetCombatSuppressed(true); MoveV2BActor(Enemy.Get(),V2BBaseLocation+FVector(-350,-250,0));
	MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+FVector(0,350,0)); MoveV2BActor(GetTargetAt(0),V2BBaseLocation+FVector(420,0,0)); MoveV2BActor(GetTargetAt(1),V2BBaseLocation+FVector(500,300,0)); MoveV2BActor(GetTargetAt(2),V2BBaseLocation+FVector(500,-300,0));
	V2BVisibleStep=0; RunV2BVisibleStep();
}

void Ademo_mapGameMode::RunV2BVisibleStep()
{
	APawn* Pawn=GetDemoPawn(); Udemo_mapSkillComponent* Skills=GetV2BSkills(); Ademo_mapGameState* State=GetWorld()!=nullptr?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr; if(!Pawn||!Skills||!State||!FriendlyUnit.IsValid()){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: runtime state."));return;}
	const FVector X=FVector::ForwardVector; const FVector Y=FVector::RightVector;
	switch(V2BVisibleStep)
	{
	case 0: CaptureT7RVisual(TEXT("01_V2B_SKILL_HUD.png")); ScheduleNextV2BVisibleStep(0.50f); return;
	case 1: if(!Skills->BeginGroundCircleTargeting()){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: valid targeting start."));return;} Skills->SetAutomationGroundTargetPreview(true,V2BBaseLocation+X*500.0f); ScheduleNextV2BVisibleStep(0.40f); return;
	case 2: CaptureT7RVisual(TEXT("02_V2B_CIRCLE_TARGETING_VALID.png")); ScheduleNextV2BVisibleStep(0.20f); return;
	case 3: Skills->SetAutomationGroundTargetPreview(true,V2BBaseLocation+X*1000.0f); ScheduleNextV2BVisibleStep(0.40f); return;
	case 4: CaptureT7RVisual(TEXT("03_V2B_CIRCLE_TARGETING_OUT_OF_RANGE.png")); ScheduleNextV2BVisibleStep(0.20f); return;
	case 5: Skills->CancelGroundCircleTargeting(); MoveV2BActor(GetTargetAt(0),V2BBaseLocation+X*400.0f); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+X*400.0f+Y*80.0f); MoveV2BActor(GetTargetAt(1),V2BBaseLocation-X*900.0f); MoveV2BActor(GetTargetAt(2),V2BBaseLocation-X*1000.0f); if(!Skills->TryCastGroundCircleAt(V2BBaseLocation+X*400.0f)){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: circle cast."));return;} ScheduleNextV2BVisibleStep(0.10f); return;
	case 6: if(!GetTargetAt(0)||GetTargetAt(0)->GetHealth()!=1||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: circle filter."));return;} CaptureT7RVisual(TEXT("04_V2B_CIRCLE_FRIENDLY_FILTER.png")); ScheduleNextV2BVisibleStep(0.20f); return;
	case 7: MoveV2BActor(GetTargetAt(1),V2BBaseLocation+X*200.0f); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+FVector(140,70,0)); if(!Skills->TryCastSelfSector(X)){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: sector cast."));return;} ScheduleNextV2BVisibleStep(0.10f); return;
	case 8: if(!GetTargetAt(1)||GetTargetAt(1)->GetHealth()!=1||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: sector filter."));return;} CaptureT7RVisual(TEXT("05_V2B_SELF_SECTOR.png")); ScheduleNextV2BVisibleStep(0.20f); return;
	case 9: MoveV2BActor(GetTargetAt(0),V2BBaseLocation+FVector(-900,-500,0)); MoveV2BActor(GetTargetAt(1),V2BBaseLocation+FVector(-1000,500,0)); MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+X*330.0f); MoveV2BActor(GetTargetAt(2),V2BBaseLocation+X*700.0f); if(!Skills->TryFireStraightProjectile(X)){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: projectile spawn."));return;} ScheduleNextV2BVisibleStep(0.28f); return;
	case 10: CaptureT7RVisual(TEXT("06_V2B_PROJECTILE_FRIENDLY_PASS.png")); ScheduleNextV2BVisibleStep(0.55f); return;
	case 11: if(!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=1||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: projectile filter."));return;} MoveV2BActor(FriendlyUnit.Get(),V2BBaseLocation+FVector(-800,500,0)); MoveV2BActor(GetTargetAt(0),V2BBaseLocation+Y*500.0f); MoveV2BActor(GetTargetAt(1),V2BBaseLocation+X*200.0f); MoveV2BActor(GetTargetAt(2),V2BBaseLocation+X*650.0f); ScheduleNextV2BVisibleStep(0.75f); return;
	case 12: if(!Skills->TryCastGroundCircleAt(GetTargetAt(0)->GetActorLocation())||!Skills->TryCastSelfSector(X)||!Skills->TryFireStraightProjectile(X)){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: final skill mission casts."));return;} ScheduleNextV2BVisibleStep(0.80f); return;
	case 13: if(State->GetDestroyedTargets()!=3||!ExitZone.IsValid()||!ExitZone->IsExitUnlocked()||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: final mission state."));return;} CaptureT7RVisual(TEXT("07_V2B_EXIT_OPEN_AFTER_SKILLS.png")); ScheduleNextV2BVisibleStep(0.60f); return;
	case 14: UE_LOG(Logdemo_map,Log,TEXT("V2B_VISIBLE_ACCEPTANCE: PASS.")); FPlatformMisc::RequestExitWithStatus(false,0); return;
	default: FailAutomation(TEXT("V2B_VISIBLE_ACCEPTANCE: FAIL: invalid step.")); return;
	}
}

AActor* Ademo_mapGameMode::GetV2CActorWithTag(FName Tag) const
{
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(Tag))
		{
			return *It;
		}
	}
	return nullptr;
}

bool Ademo_mapGameMode::HasValidNavigationPath(const FVector& Start, const FVector& End) const
{
	if (GetWorld() == nullptr)
	{
		return false;
	}
	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Start, End);
	const bool bValid = Path != nullptr && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num() >= 2;
	UE_LOG(Logdemo_map, Log, TEXT("V2C: nav query (%.0f,%.0f)->(%.0f,%.0f) valid=%d partial=%d points=%d"), Start.X, Start.Y, End.X, End.Y, bValid ? 1 : 0, Path != nullptr && Path->IsPartial() ? 1 : 0, Path != nullptr ? Path->PathPoints.Num() : 0);
	return bValid;
}

void Ademo_mapGameMode::ScheduleNextV2CAutomationStep(float Delay)
{
	++V2CAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2CAutomationStep, Delay, false);
}

void Ademo_mapGameMode::StartV2CAutomation()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	Udemo_mapPlayerHealthComponent* Health = Pawn != nullptr ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Ademo_mapGameState* State = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!IsV2CMap() || Pawn == nullptr || Skills == nullptr || Health == nullptr || State == nullptr || !Enemy.IsValid() || !FriendlyUnit.IsValid() || !ExitZone.IsValid())
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: lifecycle initialization or map identity."));
		return;
	}

	if (GV2CAutomationPhase == 1)
	{
		if (State->GetDestroyedTargets() != 0 || ExitZone->IsExitUnlocked() || Health->GetCurrentHealth() != 5 || !Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady() || SpawnedTargets.Num() != 3)
		{
			FailAutomation(TEXT("V2C_AUTOMATION: FAIL: extraction reset state."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: mission extraction reset passed."));
		GV2CAutomationPhase = 2;
		if (!PlacePawnForEnemy(100.0f))
		{
			FailAutomation(TEXT("V2C_AUTOMATION: FAIL: could not place player for real enemy defeat cycle."));
			return;
		}
		Enemy->SetCombatSuppressed(false);
		V2CDefeatDeadline = GetWorld()->GetTimeSeconds() + 9.0f;
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::PollV2CDefeat, 0.20f, true);
		return;
	}

	if (GV2CAutomationPhase == 2)
	{
		int32 EnemyCount = 0, FriendlyCount = 0, TargetCount = 0, ExitCount = 0, ProjectileCount = 0;
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) ++EnemyCount;
		for (TActorIterator<Ademo_mapFriendlyUnit> It(GetWorld()); It; ++It) ++FriendlyCount;
		for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) ++TargetCount;
		for (TActorIterator<Ademo_mapExitZone> It(GetWorld()); It; ++It) ++ExitCount;
		for (TActorIterator<Ademo_mapSkillProjectile> It(GetWorld()); It; ++It) ++ProjectileCount;
		if (Health->GetCurrentHealth() != 5 || State->GetDestroyedTargets() != 0 || ExitZone->IsExitUnlocked() || !Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady() || EnemyCount != 1 || FriendlyCount != 1 || TargetCount != 3 || ExitCount != 1 || ProjectileCount != 0)
		{
			FailAutomation(TEXT("V2C_AUTOMATION: FAIL: defeat reset state or actor counts."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: defeat reset passed."));
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: PASS."));
		GV2CAutomationPhase = 0;
		FPlatformMisc::RequestExitWithStatus(false, 0);
		return;
	}

	int32 PlayerStartCount = 0;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) ++PlayerStartCount;
	if (!GetWorld()->GetMapName().EndsWith(TEXT("L_V2_CombatDemo")) || Cast<Ademo_mapPlayerController>(GetWorld()->GetFirstPlayerController()) == nullptr || Cast<Ademo_mapHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()) == nullptr || PlayerStartCount != 1)
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: map identity, controller, HUD, or PlayerStart."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: map identity passed."));

	TArray<AActor*> RawMarkers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapEncounterMarker::StaticClass(), RawMarkers);
	int32 FriendlyMarkers = 0, EnemyMarkers = 0, ExitMarkers = 0, TargetMarkers = 0;
	TSet<int32> TargetIndices;
	int32 RangedMarkers = 0, HeavyMarkers = 0;
	bool bMarkerGeometryValid = RawMarkers.Num() == 8;
	for (AActor* Raw : RawMarkers)
	{
		Ademo_mapEncounterMarker* Marker = Cast<Ademo_mapEncounterMarker>(Raw);
		const FVector P = Marker != nullptr ? Marker->GetActorLocation() : FVector(99999.0f);
		bMarkerGeometryValid &= Marker != nullptr && FMath::Abs(P.X) < 3400.0f && FMath::Abs(P.Y) < 4500.0f;
		for (AActor* OtherRaw : RawMarkers)
		{
			if (OtherRaw != Raw) bMarkerGeometryValid &= FVector::Dist2D(P, OtherRaw->GetActorLocation()) > 100.0f;
		}
		if (Marker == nullptr) continue;
		switch (Marker->GetMarkerType())
		{
		case Edemo_mapEncounterMarkerType::FriendlySpawn: ++FriendlyMarkers; break;
		case Edemo_mapEncounterMarkerType::MeleeEnemySpawn: ++EnemyMarkers; break;
		case Edemo_mapEncounterMarkerType::RangedEnemySpawn: ++RangedMarkers; break;
		case Edemo_mapEncounterMarkerType::HeavyEnemySpawn: ++HeavyMarkers; break;
		case Edemo_mapEncounterMarkerType::ExitSpawn: ++ExitMarkers; break;
		case Edemo_mapEncounterMarkerType::TrainingTargetSpawn: ++TargetMarkers; TargetIndices.Add(Marker->GetMarkerIndex()); break;
		}
	}
	if (!bMarkerGeometryValid || FriendlyMarkers != 1 || EnemyMarkers != 1 || RangedMarkers != 1 || HeavyMarkers != 1 || TargetMarkers != 3 || TargetIndices.Num() != 3 || !TargetIndices.Contains(0) || !TargetIndices.Contains(1) || !TargetIndices.Contains(2) || ExitMarkers != 1)
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: encounter marker counts, indices, bounds, or overlap."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: encounter markers passed."));

	int32 SkillCount = 0;
	Pawn->ForEachComponent<Udemo_mapSkillComponent>(false, [&SkillCount](Udemo_mapSkillComponent*) { ++SkillCount; });
	if (Health->GetCurrentHealth() != 5 || FriendlyUnit->GetCurrentHealth() != 5 || Enemy->GetCurrentHealth() != 3 || SpawnedTargets.Num() != 3 || ExitZone->IsExitUnlocked() || State->GetDestroyedTargets() != 0 || SkillCount != 1 || !Skills->IsCircleReady() || !Skills->IsConeReady() || !Skills->IsProjectileReady())
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: initial gameplay state."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: initial gameplay state passed."));

	int32 GroundCount = 0, BoundaryCount = 0;
	for (TActorIterator<Ademo_mapGrayboxBlock> It(GetWorld()); It; ++It)
	{
		GroundCount += It->GetBlockType() == Edemo_mapGrayboxBlockType::Ground ? 1 : 0;
		BoundaryCount += It->GetBlockType() == Edemo_mapGrayboxBlockType::BoundaryWall ? 1 : 0;
	}
	FHitResult GroundHit;
	const FVector PawnLocation = Pawn->GetActorLocation();
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(V2CGround), false, Pawn);
	const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(GroundHit, PawnLocation + FVector(0,0,100), PawnLocation - FVector(0,0,500), ECC_Visibility, GroundParams);
	auto BoundarySweep = [this](const FVector& Start, const FVector& End)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(V2CBoundary), false, GetDemoPawn());
		return GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(42.0f), Params) && Hit.GetActor() != nullptr && Hit.GetActor()->ActorHasTag(TEXT("V2C_BOUNDARY"));
	};
	const bool bBoundariesBlock = BoundarySweep(FVector(2200,0,88), FVector(2600,0,88)) && BoundarySweep(FVector(-2200,0,88), FVector(-2600,0,88)) && BoundarySweep(FVector(0,3000,88), FVector(0,3400,88)) && BoundarySweep(FVector(0,-3000,88), FVector(0,-3400,88));
	if (GroundCount != 1 || BoundaryCount != 4 || !bGroundHit || GroundHit.GetActor() == nullptr || !GroundHit.GetActor()->ActorHasTag(TEXT("V2C_GROUND")) || !bBoundariesBlock)
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: ground or boundary collision."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: ground and boundaries passed."));

	AActor* Narrow = GetV2CActorWithTag(TEXT("V2C_REGION_NARROW"));
	AActor* Cover = GetV2CActorWithTag(TEXT("V2C_REGION_COVER"));
	AActor* Core = GetV2CActorWithTag(TEXT("V2C_REGION_CORE"));
	AActor* ExitRegion = GetV2CActorWithTag(TEXT("V2C_REGION_EXIT"));
	bool bPaths = Narrow && Cover && Core && ExitRegion && HasValidNavigationPath(PawnLocation, Narrow->GetActorLocation()) && HasValidNavigationPath(Narrow->GetActorLocation(), Cover->GetActorLocation()) && HasValidNavigationPath(Cover->GetActorLocation(), GetTargetAt(0)->GetActorLocation()) && HasValidNavigationPath(Cover->GetActorLocation(), GetTargetAt(1)->GetActorLocation()) && HasValidNavigationPath(Cover->GetActorLocation(), GetTargetAt(2)->GetActorLocation()) && HasValidNavigationPath(Core->GetActorLocation(), ExitRegion->GetActorLocation()) && HasValidNavigationPath(Enemy->GetActorLocation(), PawnLocation);
	if (!bPaths)
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: navigation connectivity."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: navigation connectivity passed."));

	AActor* CentralBlocker = GetV2CActorWithTag(TEXT("V2C_CENTRAL_BLOCKER"));
	if (CentralBlocker == nullptr)
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: central blocker missing."));
		return;
	}
	const FVector Center = CentralBlocker->GetActorLocation();
	MoveV2BActor(Pawn, FVector(Center.X - 410.0f, Center.Y, 88.0f));
	MoveV2BActor(Enemy.Get(), FVector(Center.X + 410.0f, Center.Y, 88.0f));
	Enemy->SetCombatSuppressed(false);
	V2CInitialEnemyDistance = FVector::Dist2D(Pawn->GetActorLocation(), Enemy->GetActorLocation());
	V2CAutomationStep = 0;
	ScheduleNextV2CAutomationStep(4.0f);
#endif
}

void Ademo_mapGameMode::RunV2CAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	if (Pawn == nullptr || Skills == nullptr || !FriendlyUnit.IsValid() || !Enemy.IsValid())
	{
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: runtime state disappeared.")); return;
	}
	const FVector X = FVector::ForwardVector;
	switch (V2CAutomationStep)
	{
	case 1:
	{
		const float NewDistance = FVector::Dist2D(Pawn->GetActorLocation(), Enemy->GetActorLocation());
		if (NewDistance >= V2CInitialEnemyDistance - 100.0f || !HasValidNavigationPath(Enemy->GetActorLocation(), Pawn->GetActorLocation())) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: obstacle chase did not use a valid detour or close distance.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: obstacle chase passed."));
		Enemy->SetCombatSuppressed(true);
		AActor* Lane = GetV2CActorWithTag(TEXT("V2C_OPEN_PROJECTILE_LANE"));
		if (!Lane) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: open projectile lane marker missing.")); return; }
		const FVector Base = Lane->GetActorLocation() + FVector(0,0,84);
		MoveV2BActor(Pawn, Base); MoveV2BActor(FriendlyUnit.Get(), Base + FVector(320,90,0));
		V2COpenLaneEnemy = SpawnV2BTestEnemy(Base + X * 700.0f);
		if (!V2COpenLaneEnemy.IsValid() || Skills->TryFireStraightProjectile(X) == nullptr) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: open projectile setup.")); return; }
		ScheduleNextV2CAutomationStep(0.85f); return;
	}
	case 2:
	{
		if (!V2COpenLaneEnemy.IsValid() || V2COpenLaneEnemy->GetCurrentHealth() != 2 || FriendlyUnit->GetCurrentHealth() != 5 || Skills->GetLastSpawnedProjectile() != nullptr) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: open projectile lane damage, friendly filter, or cleanup.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: open projectile lane passed."));
		AActor* Wall = GetV2CActorWithTag(TEXT("V2C_PROJECTILE_BLOCKER"));
		if (!Wall) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: persisted projectile blocker missing.")); return; }
		const FVector Base = Wall->GetActorLocation();
		MoveV2BActor(Pawn, FVector(Base.X - 430, Base.Y, 88));
		V2CBlockedEnemy = SpawnV2BTestEnemy(FVector(Base.X + 430, Base.Y, 88));
		if (!V2CBlockedEnemy.IsValid() || Skills->TryFireStraightProjectile(X) == nullptr) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: map blocker projectile setup.")); return; }
		ScheduleNextV2CAutomationStep(0.85f); return;
	}
	case 3:
	{
		if (!V2CBlockedEnemy.IsValid() || V2CBlockedEnemy->GetCurrentHealth() != 3 || Skills->GetLastSpawnedProjectile() != nullptr) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: projectile crossed persisted WorldStatic blocker.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: map projectile blocker passed."));
		const auto& C = Skills->GetCircleParams(); const auto& S = Skills->GetConeParams(); const auto& P = Skills->GetProjectileParams();
		if (!FMath::IsNearlyEqual(C.CastRange,900.0f) || !FMath::IsNearlyEqual(C.Radius,250.0f) || !FMath::IsNearlyEqual(S.Radius,350.0f) || !FMath::IsNearlyEqual(S.FullAngleDegrees,90.0f) || !FMath::IsNearlyEqual(P.Speed,1000.0f) || !FMath::IsNearlyEqual(P.MaxDistance,1400.0f) || Skills->TryCastGroundCircleAt(Pawn->GetActorLocation()+X*1000.0f)) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: V2-B parameters or circle range rejection.")); return; }
		const FVector Base = GetV2CActorWithTag(TEXT("V2C_OPEN_PROJECTILE_LANE"))->GetActorLocation()+FVector(0,0,84);
		MoveV2BActor(Pawn, Base); MoveV2BActor(FriendlyUnit.Get(), Base+X*350.0f+FVector(0,80,0));
		V2CSkillEnemy = SpawnV2BTestEnemy(Base+X*350.0f);
		if (!V2CSkillEnemy.IsValid() || !Skills->TryCastGroundCircleAt(V2CSkillEnemy->GetActorLocation())) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: legal ground circle in new map.")); return; }
		ScheduleNextV2CAutomationStep(0.20f); return;
	}
	case 4:
		if (!V2CSkillEnemy.IsValid() || V2CSkillEnemy->GetCurrentHealth()!=2 || FriendlyUnit->GetCurrentHealth()!=5) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: circle result or friendly filter.")); return; }
		MoveV2BActor(V2CSkillEnemy.Get(), Pawn->GetActorLocation()+X*200.0f);
		MoveV2BActor(FriendlyUnit.Get(), Pawn->GetActorLocation()+X*200.0f+FVector(0,80,0));
		if (!Skills->TryCastSelfSector(X)) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: sector cast.")); return; }
		ScheduleNextV2CAutomationStep(0.20f); return;
	case 5:
		if (!V2CSkillEnemy.IsValid() || V2CSkillEnemy->GetCurrentHealth()!=1 || FriendlyUnit->GetCurrentHealth()!=5) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: sector spatial result or friendly filter.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V2C_AUTOMATION: skill spatial compatibility passed."));
		V2CMissionTargetIndex = 0; V2CMissionAttackIndex = 0; RunV2CMissionAttack(); return;
	default: FailAutomation(TEXT("V2C_AUTOMATION: FAIL: invalid V2-C automation step.")); return;
	}
#endif
}

void Ademo_mapGameMode::RunV2CMissionAttack()
{
#if !UE_BUILD_SHIPPING
	Ademo_mapGameState* State = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (State == nullptr) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: mission state missing.")); return; }
	if (V2CMissionTargetIndex >= 3)
	{
		if (State->GetDestroyedTargets()!=3 || !ExitZone.IsValid() || !ExitZone->IsExitUnlocked()) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: mission did not reach 3/3 and unlock exit.")); return; }
		GV2CAutomationPhase = 1;
		PlacePawnInExit(true);
		return;
	}
	Ademo_mapTrainingTarget* Target = GetTargetAt(V2CMissionTargetIndex);
	if (V2CMissionAttackIndex < 2)
	{
		if (Target == nullptr || !PlacePawnForTarget(Target) || GetDemoPlayerController() == nullptr || !GetDemoPlayerController()->TryBasicAttack()) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: real basic attack mission step.")); return; }
		++V2CMissionAttackIndex;
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2CMissionAttack, 0.58f, false);
		return;
	}
	if (GetTargetAt(V2CMissionTargetIndex) != nullptr || State->GetDestroyedTargets() != V2CMissionTargetIndex + 1) { FailAutomation(TEXT("V2C_AUTOMATION: FAIL: training target damage identity or mission progress.")); return; }
	++V2CMissionTargetIndex; V2CMissionAttackIndex = 0;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2CMissionAttack, 0.10f, false);
#endif
}

void Ademo_mapGameMode::PollV2CDefeat()
{
	Udemo_mapPlayerHealthComponent* Health = GetDemoPawn() != nullptr ? GetDemoPawn()->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	if (Health != nullptr && Health->IsDefeated())
	{
		GetWorldTimerManager().ClearTimer(AutomationTimerHandle);
		return;
	}
	if (GetWorld() == nullptr || GetWorld()->GetTimeSeconds() > V2CDefeatDeadline)
	{
		GetWorldTimerManager().ClearTimer(AutomationTimerHandle);
		FailAutomation(TEXT("V2C_AUTOMATION: FAIL: real enemy attacks did not defeat player before deadline."));
	}
}

void Ademo_mapGameMode::ScheduleNextV2CVisibleStep(float Delay)
{
	++V2CVisibleStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2CVisibleStep, Delay, false);
}

void Ademo_mapGameMode::StartV2CVisibleAcceptance()
{
	if (!IsV2CMap() || GetDemoPawn()==nullptr || !Enemy.IsValid() || !FriendlyUnit.IsValid() || !GetTargetAt(0) || !GetTargetAt(1) || !GetTargetAt(2) || !ExitZone.IsValid()) { FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL: initial state.")); return; }
	Enemy->SetCombatSuppressed(true);
	V2CVisibleStep = 0;
	RunV2CVisibleStep();
}

void Ademo_mapGameMode::RunV2CVisibleStep()
{
	APawn* Pawn = GetDemoPawn(); Udemo_mapSkillComponent* Skills = GetV2BSkills();
	if (!Pawn || !Skills) { FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL: runtime state.")); return; }
	auto RegionLocation = [this](FName Tag) { AActor* A = GetV2CActorWithTag(Tag); return A ? A->GetActorLocation()+FVector(0,0,84) : FVector::ZeroVector; };
	switch (V2CVisibleStep)
	{
	case 0: CaptureT7RVisual(TEXT("01_V2C_START_COURTYARD.png")); ScheduleNextV2CVisibleStep(0.60f); return;
	case 1: MoveV2BActor(Pawn,RegionLocation(TEXT("V2C_REGION_NARROW"))); MoveV2BActor(Enemy.Get(),RegionLocation(TEXT("V2C_REGION_NARROW"))+FVector(330,180,0)); ScheduleNextV2CVisibleStep(1.20f); return;
	case 2: CaptureT7RVisual(TEXT("02_V2C_NARROW_PASS.png")); ScheduleNextV2CVisibleStep(0.60f); return;
	case 3:
	{
		AActor* Central=GetV2CActorWithTag(TEXT("V2C_CENTRAL_BLOCKER")); if(!Central){FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL central blocker."));return;}
		MoveV2BActor(Pawn,Central->GetActorLocation()+FVector(0,-850,-92)); ScheduleNextV2CVisibleStep(1.20f); return;
	}
	case 4: CaptureT7RVisual(TEXT("03_V2C_COVER_YARD.png")); ScheduleNextV2CVisibleStep(0.50f); return;
	case 5:
	{
		AActor* Wall=GetV2CActorWithTag(TEXT("V2C_PROJECTILE_BLOCKER")); if(!Wall){FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL blocker."));return;}
		MoveV2BActor(Pawn,Wall->GetActorLocation()+FVector(-300,0,-72)); MoveV2BActor(GetTargetAt(0),Wall->GetActorLocation()+FVector(300,0,-110)); Skills->TryFireStraightProjectile(FVector::ForwardVector); ScheduleNextV2CVisibleStep(1.00f); return;
	}
	case 6: CaptureT7RVisual(TEXT("04_V2C_PROJECTILE_BLOCKED.png")); ScheduleNextV2CVisibleStep(0.60f); return;
	case 7:
	{
		AActor* Target0Marker=GetV2CActorWithTag(TEXT("V2C_MARKER_TARGET_0")); if(!Target0Marker){FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL target marker."));return;}
		MoveV2BActor(GetTargetAt(0),Target0Marker->GetActorLocation()); MoveV2BActor(Pawn,RegionLocation(TEXT("V2C_REGION_CORE"))); Skills->BeginGroundCircleTargeting(); Skills->SetAutomationGroundTargetPreview(true,GetTargetAt(1)->GetActorLocation()); ScheduleNextV2CVisibleStep(1.20f); return;
	}
	case 8: CaptureT7RVisual(TEXT("05_V2C_CORE_TARGET_AREA.png")); ScheduleNextV2CVisibleStep(0.50f); return;
	case 9: Skills->CancelGroundCircleTargeting(); MoveV2BActor(Pawn,ExitZone->GetActorLocation()+FVector(320,0,68)); ScheduleNextV2CVisibleStep(1.20f); return;
	case 10: CaptureT7RVisual(TEXT("06_V2C_EXIT_LOCKED.png")); ScheduleNextV2CVisibleStep(0.50f); return;
	case 11:
		for(int32 Index=0;Index<3;++Index) if(Ademo_mapTrainingTarget* Target=GetTargetAt(Index)) UGameplayStatics::ApplyDamage(Target,2.0f,GetDemoPlayerController(),Pawn,nullptr);
		ScheduleNextV2CVisibleStep(0.70f); return;
	case 12:
		if(!ExitZone.IsValid() || !ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL: exit did not unlock."));return;}
		CaptureT7RVisual(TEXT("07_V2C_EXIT_OPEN.png")); ScheduleNextV2CVisibleStep(0.80f); return;
	case 13: UE_LOG(Logdemo_map,Log,TEXT("V2C_VISIBLE_ACCEPTANCE: PASS.")); FPlatformMisc::RequestExitWithStatus(false,0); return;
	default: FailAutomation(TEXT("V2C_VISIBLE_ACCEPTANCE: FAIL: invalid step.")); return;
	}
}

void Ademo_mapGameMode::ScheduleNextV2DAutomationStep(float Delay)
{
	++V2DAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2DAutomationStep, Delay, false);
}

void Ademo_mapGameMode::StartV2DAutomation()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapPlayerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Ademo_mapGameState* State = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!IsV2CMap() || !Pawn || !Health || !State || !Enemy.IsValid() || !RangedEnemy.IsValid() || !HeavyEnemy.IsValid() || !FriendlyUnit.IsValid() || !ExitZone.IsValid() || SpawnedTargets.Num() != 3)
	{
		FailAutomation(TEXT("V2D_AUTOMATION: FAIL: lifecycle initialization or roster.")); return;
	}
	if (GV2DAutomationPhase == 1)
	{
		if (Health->GetCurrentHealth()!=5 || State->GetDestroyedTargets()!=0 || ExitZone->IsExitUnlocked()) { FailAutomation(TEXT("V2D_AUTOMATION: FAIL: extraction reset state.")); return; }
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: extraction reset passed."));
		GV2DAutomationPhase=2;
		UGameplayStatics::ApplyDamage(Pawn,5.0f,Enemy->GetController(),Enemy.Get(),nullptr);
		return;
	}
	if (GV2DAutomationPhase == 2)
	{
		if (Health->GetCurrentHealth()!=5 || State->GetDestroyedTargets()!=0 || ExitZone->IsExitUnlocked() || !RangedEnemy.IsValid() || !HeavyEnemy.IsValid()) { FailAutomation(TEXT("V2D_AUTOMATION: FAIL: defeat reset state.")); return; }
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: defeat reset passed."));
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: PASS."));
		GV2DAutomationPhase=0; FPlatformMisc::RequestExitWithStatus(false,0); return;
	}
	if (Enemy->GetCurrentHealth()!=3 || RangedEnemy->GetCurrentHealth()!=3 || HeavyEnemy->GetCurrentHealth()!=5 || FriendlyUnit->GetCurrentHealth()!=5 || Health->GetCurrentHealth()!=5)
	{
		FailAutomation(TEXT("V2D_AUTOMATION: FAIL: initial health parameters.")); return;
	}
	TArray<AActor*> Markers; UGameplayStatics::GetAllActorsOfClass(GetWorld(),Ademo_mapEncounterMarker::StaticClass(),Markers);
	if (Markers.Num()!=8 || !FMath::IsNearlyEqual(RangedEnemy->GetMovementSpeed(),240.0f) || !FMath::IsNearlyEqual(HeavyEnemy->GetMovementSpeed(),180.0f) || !FMath::IsNearlyEqual(HeavyEnemy->GetSectorRadius(),430.0f) || !FMath::IsNearlyEqual(HeavyEnemy->GetFullAngleDegrees(),100.0f))
	{
		FailAutomation(TEXT("V2D_AUTOMATION: FAIL: roster, markers, or public parameters.")); return;
	}
	UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: initial roster passed."));
	Enemy->SetCombatSuppressed(true); HeavyEnemy->SetCombatSuppressed(true); RangedEnemy->SetCombatSuppressed(false);
	const FVector Base(-800.0f,1300.0f,88.0f);
	MoveV2BActor(RangedEnemy.Get(),Base); MoveV2BActor(Pawn,Base+FVector(1300,0,0));
	V2DInitialDistance=FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation()); V2DAutomationStep=0;
	ScheduleNextV2DAutomationStep(1.20f);
#endif
}

void Ademo_mapGameMode::RunV2DAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn(); Udemo_mapPlayerHealthComponent* Health=Pawn?Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>():nullptr;
	Ademo_mapGameState* Mission=GetWorld()?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr;
	if(!Pawn||!Health||!Mission||!RangedEnemy.IsValid()||!HeavyEnemy.IsValid()||!FriendlyUnit.IsValid()){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: runtime state disappeared."));return;}
	const FVector Base(-800.0f,1300.0f,88.0f);
	switch(V2DAutomationStep)
	{
	case 1:
		if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())>=V2DInitialDistance-80.0f){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: ranged approach."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: ranged approach passed."));
		MoveV2BActor(RangedEnemy.Get(),Base); MoveV2BActor(Pawn,Base+FVector(800,0,0)); MoveV2BActor(FriendlyUnit.Get(),Base+FVector(300,0,0)); MoveV2BActor(Enemy.Get(),Base+FVector(500,0,0));
		ScheduleNextV2DAutomationStep(0.30f);return;
	case 2:
		if(!RangedEnemy->HasLineOfSightToPlayer()||!RangedEnemy->HasActiveWindup()){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: ranged line of sight or windup."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: ranged line of sight passed."));
		V2DInitialPlayerHealth=Health->GetCurrentHealth(); ScheduleNextV2DAutomationStep(1.15f);return;
	case 3:
		if(RangedEnemy->GetTotalProjectilesFired()<1||Health->GetCurrentHealth()!=V2DInitialPlayerHealth-1||FriendlyUnit->GetCurrentHealth()!=5||Enemy->GetCurrentHealth()!=3){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: enemy projectile filtering or damage."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: ranged windup and projectile passed.")); UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: enemy projectile target filtering passed."));
		MoveV2BActor(RangedEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(100,0,0));V2DInitialDistance=FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation());ScheduleNextV2DAutomationStep(1.20f);return;
	case 4:
		if(FVector::Dist2D(RangedEnemy->GetActorLocation(),Pawn->GetActorLocation())<=V2DInitialDistance+60.0f){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: ranged retreat."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: ranged retreat passed."));RangedEnemy->SetCombatSuppressed(true);
		MoveV2BActor(HeavyEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(800,0,0));V2DInitialDistance=800;HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2DAutomationStep(1.20f);return;
	case 5:
		if(FVector::Dist2D(HeavyEnemy->GetActorLocation(),Pawn->GetActorLocation())>=V2DInitialDistance-70.0f){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: heavy chase."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: heavy chase passed."));MoveV2BActor(HeavyEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(300,0,0));ScheduleNextV2DAutomationStep(0.30f);return;
	case 6:
		if(!HeavyEnemy->HasActiveTelegraph()){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: heavy telegraph."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: heavy telegraph passed."));V2DInitialPlayerHealth=Health->GetCurrentHealth();ScheduleNextV2DAutomationStep(0.90f);return;
	case 7:
		if(Health->GetCurrentHealth()!=V2DInitialPlayerHealth-1){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: heavy sector hit."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: heavy sector hit passed."));HeavyEnemy->SetCombatSuppressed(true);ScheduleNextV2DAutomationStep(2.50f);return;
	case 8:
		MoveV2BActor(HeavyEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(300,0,0));HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2DAutomationStep(0.30f);return;
	case 9:
		if(!HeavyEnemy->HasActiveTelegraph()){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: dodge telegraph."));return;}
		V2DInitialPlayerHealth=Health->GetCurrentHealth();MoveV2BActor(Pawn,Base+FVector(0,600,0));ScheduleNextV2DAutomationStep(0.90f);return;
	case 10:
		if(Health->GetCurrentHealth()!=V2DInitialPlayerHealth){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: heavy sector dodge."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: heavy sector dodge passed."));HeavyEnemy->SetCombatSuppressed(true);ScheduleNextV2DAutomationStep(2.50f);return;
	case 11:
		MoveV2BActor(HeavyEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(350,0,0));V2DBlockingWall=SpawnV2BBlockingWall(Base+FVector(175,0,0));V2DInitialPlayerHealth=Health->GetCurrentHealth();HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2DAutomationStep(1.20f);return;
	case 12:
		if(Health->GetCurrentHealth()!=V2DInitialPlayerHealth){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: heavy world obstruction."));return;}
		UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: heavy world obstruction passed."));HeavyEnemy->SetCombatSuppressed(true);if(V2DBlockingWall.IsValid())V2DBlockingWall->Destroy();
		MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator(0,0,0));MoveV2BActor(RangedEnemy.Get(),Base+FVector(170,0,0));MoveV2BActor(HeavyEnemy.Get(),Base+FVector(900,0,0));
		V2DVariantKillTarget=0;V2DVariantAttackCount=0;RunV2DVariantKillAttack();return;
	default:FailAutomation(TEXT("V2D_AUTOMATION: FAIL: invalid step."));return;
	}
#endif
}

void Ademo_mapGameMode::RunV2DVariantKillAttack()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn();Ademo_mapPlayerController* Controller=GetDemoPlayerController();Ademo_mapGameState* Mission=GetWorld()?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr;
	if(!Pawn||!Controller||!Mission||(V2DVariantKillTarget==0&&!RangedEnemy.IsValid())||(V2DVariantKillTarget==1&&!HeavyEnemy.IsValid())){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: variant kill runtime state."));return;}
	const int32 RequiredAttacks=V2DVariantKillTarget==0?3:5;
	if(V2DVariantAttackCount<RequiredAttacks)
	{
		if(!Controller->TryBasicAttack()){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: real player attack on enemy variant."));return;}
		++V2DVariantAttackCount;GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::RunV2DVariantKillAttack,0.58f,false);return;
	}
	const bool bDead=V2DVariantKillTarget==0?RangedEnemy->IsDead():HeavyEnemy->IsDead();
	if(!bDead||Mission->GetDestroyedTargets()!=0||ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2D_AUTOMATION: FAIL: variant death or mission identity."));return;}
	if(V2DVariantKillTarget==0)
	{
		V2DVariantKillTarget=1;V2DVariantAttackCount=0;MoveV2BActor(RangedEnemy.Get(),Pawn->GetActorLocation()+FVector(900,0,0));MoveV2BActor(HeavyEnemy.Get(),Pawn->GetActorLocation()+FVector(170,0,0));GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::RunV2DVariantKillAttack,0.20f,false);return;
	}
	UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: player attacks enemy variants passed."));UE_LOG(Logdemo_map,Log,TEXT("V2D_AUTOMATION: mission identity remained separate."));
	for(int32 Index=0;Index<3;++Index)if(Ademo_mapTrainingTarget* Training=GetTargetAt(Index))UGameplayStatics::ApplyDamage(Training,2.0f,Controller,Pawn,nullptr);
	GV2DAutomationPhase=1;GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::FinishV2DAutomationTargets,0.30f,false);
#endif
}

void Ademo_mapGameMode::FinishV2DAutomationTargets(){PlacePawnInExit(true);}

void Ademo_mapGameMode::ScheduleNextV2DVisibleStep(float Delay)
{
	++V2DVisibleStep; GetWorldTimerManager().SetTimer(AutomationTimerHandle,this,&Ademo_mapGameMode::RunV2DVisibleStep,Delay,false);
}

void Ademo_mapGameMode::StartV2DVisibleAcceptance()
{
	if(!IsV2CMap()||!GetDemoPawn()||!RangedEnemy.IsValid()||!HeavyEnemy.IsValid()||!Enemy.IsValid()){FailAutomation(TEXT("V2D_VISIBLE_ACCEPTANCE: FAIL: roster."));return;}
	Enemy->SetCombatSuppressed(true);RangedEnemy->SetCombatSuppressed(true);HeavyEnemy->SetCombatSuppressed(true);V2DVisibleStep=0;RunV2DVisibleStep();
}

void Ademo_mapGameMode::RunV2DVisibleStep()
{
	APawn* Pawn=GetDemoPawn();if(!Pawn){FailAutomation(TEXT("V2D_VISIBLE_ACCEPTANCE: FAIL: pawn."));return;}const FVector Base(700,700,88);const FVector StartArea(0,-2555,88);
	switch(V2DVisibleStep)
	{
	case 0:MoveV2BActor(Pawn,StartArea);MoveV2BActor(Enemy.Get(),StartArea+FVector(260,-260,0));MoveV2BActor(RangedEnemy.Get(),StartArea+FVector(420,0,0));MoveV2BActor(HeavyEnemy.Get(),StartArea+FVector(260,280,0));ScheduleNextV2DVisibleStep(0.60f);return;
	case 1:CaptureT7RVisual(TEXT("01_V2D_ENEMY_ROSTER.png"));ScheduleNextV2DVisibleStep(0.30f);return;
	case 2:MoveV2BActor(RangedEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(800,0,0));RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2DVisibleStep(0.30f);return;
	case 3:CaptureT7RVisual(TEXT("02_V2D_RANGED_WINDUP.png"));ScheduleNextV2DVisibleStep(0.35f);return;
	case 4:CaptureT7RVisual(TEXT("03_V2D_RANGED_PROJECTILE.png"));ScheduleNextV2DVisibleStep(0.30f);return;
	case 5:{RangedEnemy->SetCombatSuppressed(true);AActor* Wall=GetV2CActorWithTag(TEXT("V2C_PROJECTILE_BLOCKER"));if(!Wall){FailAutomation(TEXT("V2D_VISIBLE_ACCEPTANCE: FAIL: blocker."));return;}MoveV2BActor(RangedEnemy.Get(),Wall->GetActorLocation()+FVector(-430,0,-72));MoveV2BActor(Pawn,Wall->GetActorLocation()+FVector(430,0,-72));RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2DVisibleStep(0.60f);return;}
	case 6:CaptureT7RVisual(TEXT("04_V2D_RANGED_BLOCKED_BY_COVER.png"));ScheduleNextV2DVisibleStep(0.30f);return;
	case 7:RangedEnemy->SetCombatSuppressed(true);MoveV2BActor(HeavyEnemy.Get(),Base);MoveV2BActor(Pawn,Base+FVector(300,0,0));HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2DVisibleStep(0.30f);return;
	case 8:CaptureT7RVisual(TEXT("05_V2D_HEAVY_TELEGRAPH.png"));ScheduleNextV2DVisibleStep(0.25f);return;
	case 9:MoveV2BActor(Pawn,Base+FVector(0,600,0));ScheduleNextV2DVisibleStep(0.35f);return;
	case 10:CaptureT7RVisual(TEXT("06_V2D_HEAVY_DODGED.png"));ScheduleNextV2DVisibleStep(0.30f);return;
	case 11:HeavyEnemy->SetCombatSuppressed(true);MoveV2BActor(Pawn,Base);MoveV2BActor(RangedEnemy.Get(),Base+FVector(280,-210,0));MoveV2BActor(HeavyEnemy.Get(),Base+FVector(280,210,0));UGameplayStatics::ApplyDamage(RangedEnemy.Get(),1.0f,GetDemoPlayerController(),Pawn,nullptr);UGameplayStatics::ApplyDamage(HeavyEnemy.Get(),1.0f,GetDemoPlayerController(),Pawn,nullptr);ScheduleNextV2DVisibleStep(0.40f);return;
	case 12:CaptureT7RVisual(TEXT("07_V2D_PLAYER_DAMAGES_VARIANTS.png"));ScheduleNextV2DVisibleStep(0.30f);return;
	case 13:for(int32 Index=0;Index<3;++Index)if(Ademo_mapTrainingTarget* Target=GetTargetAt(Index))UGameplayStatics::ApplyDamage(Target,2.0f,GetDemoPlayerController(),Pawn,nullptr);MoveV2BActor(Pawn,ExitZone->GetActorLocation()+FVector(320,0,68));ScheduleNextV2DVisibleStep(0.70f);return;
	case 14:if(!ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2D_VISIBLE_ACCEPTANCE: FAIL: exit."));return;}CaptureT7RVisual(TEXT("08_V2D_EXIT_OPEN.png"));ScheduleNextV2DVisibleStep(0.60f);return;
	case 15:UE_LOG(Logdemo_map,Log,TEXT("V2D_VISIBLE_ACCEPTANCE: PASS."));FPlatformMisc::RequestExitWithStatus(false,0);return;
	default:FailAutomation(TEXT("V2D_VISIBLE_ACCEPTANCE: FAIL: invalid step."));return;
	}
}

void Ademo_mapGameMode::ScheduleNextV2FinalAutomationStep(float Delay)
{
	++V2FinalAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2FinalAutomationStep, Delay, false);
}

bool Ademo_mapGameMode::ValidateV2FinalResetState(const TCHAR* Context, int32& OutActorCount, int32& OutAIControllerCount)
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	Udemo_mapPlayerHealthComponent* Health = Pawn != nullptr ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Ademo_mapGameState* Mission = GetWorld() != nullptr ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (Enemy.IsValid()) Enemy->SetCombatSuppressed(true);
	if (RangedEnemy.IsValid()) RangedEnemy->SetCombatSuppressed(true);
	if (HeavyEnemy.IsValid()) HeavyEnemy->SetCombatSuppressed(true);

	int32 PlayerCount = 0, FriendlyCount = 0, MeleeCount = 0, RangedCount = 0, HeavyCount = 0, TargetCount = 0, ExitCount = 0, ProjectileCount = 0, SkillCount = 0;
	OutActorCount = 0;
	OutAIControllerCount = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It) ++OutActorCount;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It) if (It->IsPlayerControlled()) ++PlayerCount;
	for (TActorIterator<Ademo_mapFriendlyUnit> It(GetWorld()); It; ++It) ++FriendlyCount;
	for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) ++MeleeCount;
	for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) ++RangedCount;
	for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) ++HeavyCount;
	for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) ++TargetCount;
	for (TActorIterator<Ademo_mapExitZone> It(GetWorld()); It; ++It) ++ExitCount;
	for (TActorIterator<Ademo_mapSkillProjectile> It(GetWorld()); It; ++It) ++ProjectileCount;
	for (TActorIterator<AAIController> It(GetWorld()); It; ++It) ++OutAIControllerCount;
	if (Pawn != nullptr) Pawn->ForEachComponent<Udemo_mapSkillComponent>(false, [&SkillCount](Udemo_mapSkillComponent*) { ++SkillCount; });

	const bool bReady = IsV2CMap() && Pawn != nullptr && Health != nullptr && Mission != nullptr && Skills != nullptr
		&& PlayerCount == 1 && FriendlyCount == 1 && MeleeCount == 1 && RangedCount == 1 && HeavyCount == 1 && TargetCount == 3 && ExitCount == 1
		&& SkillCount == 1 && Health->GetCurrentHealth() == 5 && FriendlyUnit.IsValid() && FriendlyUnit->GetCurrentHealth() == 5
		&& Mission->GetDestroyedTargets() == 0 && ExitZone.IsValid() && !ExitZone->IsExitUnlocked()
		&& Skills->IsCircleReady() && Skills->IsConeReady() && Skills->IsProjectileReady() && ProjectileCount == 0
		&& RangedEnemy.IsValid() && !RangedEnemy->HasActiveWindup() && HeavyEnemy.IsValid() && !HeavyEnemy->HasActiveTelegraph()
		&& OutAIControllerCount == 3;
	if (!bReady)
	{
		FailAutomation(FString::Printf(TEXT("V2FINAL_AUTOMATION: FAIL: %s reset state. actors=%d ai=%d player=%d ally=%d melee=%d ranged=%d heavy=%d targets=%d exit=%d skills=%d projectiles=%d health=%d mission=%d."),
			Context, OutActorCount, OutAIControllerCount, PlayerCount, FriendlyCount, MeleeCount, RangedCount, HeavyCount, TargetCount, ExitCount, SkillCount, ProjectileCount,
			Health != nullptr ? Health->GetCurrentHealth() : -1, Mission != nullptr ? Mission->GetDestroyedTargets() : -1));
	}
	return bReady;
#else
	return false;
#endif
}

void Ademo_mapGameMode::StartV2FinalAutomation()
{
#if !UE_BUILD_SHIPPING
	int32 ActorCount = 0, AIControllerCount = 0;
	if (!ValidateV2FinalResetState(TEXT("initial"), ActorCount, AIControllerCount)) return;
	APawn* Pawn = GetDemoPawn();
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	Udemo_mapPlayerHealthComponent* Health = Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
	Ademo_mapGameState* Mission = Cast<Ademo_mapGameState>(GetWorld()->GetGameState());

	if (GV2FinalAutomationPhase == 1)
	{
		GV2FinalResetActorCounts[0] = ActorCount;
		if (ActorCount != GV2FinalInitialActorCount || AIControllerCount != GV2FinalInitialAIControllerCount)
		{
			FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: extraction reload actor count changed.")); return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V2FINAL_AUTOMATION: mission and extraction passed."));
		GV2FinalAutomationPhase = 2;
		UGameplayStatics::ApplyDamage(Pawn, 5.0f, Enemy->GetController(), Enemy.Get(), nullptr);
		return;
	}
	if (GV2FinalAutomationPhase == 2)
	{
		GV2FinalResetActorCounts[1] = ActorCount;
		if (ActorCount != GV2FinalInitialActorCount || AIControllerCount != GV2FinalInitialAIControllerCount)
		{
			FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: death reload actor count changed.")); return;
		}
		GV2FinalAutomationPhase = 3;
		if (Ademo_mapPlayerController* Controller = GetDemoPlayerController()) Controller->ReloadCurrentLevel();
		else FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: manual R reload controller missing."));
		return;
	}
	if (GV2FinalAutomationPhase == 3)
	{
		GV2FinalResetActorCounts[2] = ActorCount;
		if (ActorCount != GV2FinalInitialActorCount || AIControllerCount != GV2FinalInitialAIControllerCount || GV2FinalResetActorCounts[0] != ActorCount || GV2FinalResetActorCounts[1] != ActorCount)
		{
			FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: repeated reset actor or AIController count growth.")); return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V2FINAL_AUTOMATION: repeated reset integrity passed."));
		const FVector StressBase(700.0f, 700.0f, Pawn->GetActorLocation().Z);
		MoveV2BActor(Pawn, StressBase);
		Pawn->SetActorRotation(FRotator::ZeroRotator);
		V2FinalBlockingWall = SpawnV2BBlockingWall(StressBase + FVector(400.0f, 0.0f, 55.0f));
		for (int32 Index = 0; Index < 10; ++Index) Skills->SpawnProjectile(FVector::ForwardVector);
		for (int32 Index = 0; Index < 10; ++Index) Skills->SpawnProjectile(FVector::RightVector);
		TArray<AActor*> Projectiles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapSkillProjectile::StaticClass(), Projectiles);
		V2FinalProjectilePeak = Projectiles.Num();
		if (V2FinalProjectilePeak != 20 || !V2FinalBlockingWall.IsValid())
		{
			FailAutomation(FString::Printf(TEXT("V2FINAL_AUTOMATION: FAIL: bounded projectile spawn peak=%d."), V2FinalProjectilePeak)); return;
		}
		V2FinalAutomationStep = 100;
		GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2FinalAutomationStep, 2.10f, false);
		return;
	}

	GV2FinalInitialActorCount = ActorCount;
	GV2FinalInitialAIControllerCount = AIControllerCount;
	GV2FinalMeasuredLoadSeconds = static_cast<float>(FPlatformTime::Seconds() - GV2FinalProcessStartSeconds);
	GV2FinalMeasuredNavigationSeconds = V2FinalNavigationSeconds;
	int32 PlayerStartCount = 0, NavBoundsCount = 0;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) ++PlayerStartCount;
	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It) ++NavBoundsCount;
	TArray<AActor*> Markers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), Ademo_mapEncounterMarker::StaticClass(), Markers);
	int32 FriendlyMarkers=0, MeleeMarkers=0, RangedMarkers=0, HeavyMarkers=0, TargetMarkers=0, ExitMarkers=0;
	TSet<int32> TargetIndices;
	bool bMarkerPaths = Markers.Num() == 8;
	for (AActor* Raw : Markers)
	{
		Ademo_mapEncounterMarker* Marker = Cast<Ademo_mapEncounterMarker>(Raw);
		bMarkerPaths &= Marker != nullptr && HasValidNavigationPath(Pawn->GetActorLocation(), Raw->GetActorLocation());
		if (!Marker) continue;
		switch (Marker->GetMarkerType())
		{
		case Edemo_mapEncounterMarkerType::FriendlySpawn: ++FriendlyMarkers; break;
		case Edemo_mapEncounterMarkerType::MeleeEnemySpawn: ++MeleeMarkers; break;
		case Edemo_mapEncounterMarkerType::RangedEnemySpawn: ++RangedMarkers; break;
		case Edemo_mapEncounterMarkerType::HeavyEnemySpawn: ++HeavyMarkers; break;
		case Edemo_mapEncounterMarkerType::TrainingTargetSpawn: ++TargetMarkers; TargetIndices.Add(Marker->GetMarkerIndex()); break;
		case Edemo_mapEncounterMarkerType::ExitSpawn: ++ExitMarkers; break;
		}
	}
	AActor* Ground = GetV2CActorWithTag(TEXT("V2C_GROUND"));
	const FVector GroundSize = Ground != nullptr ? Ground->GetComponentsBoundingBox(true).GetSize() : FVector::ZeroVector;
	const bool bRegions = GetV2CActorWithTag(TEXT("V2C_REGION_START")) && GetV2CActorWithTag(TEXT("V2C_REGION_NARROW")) && GetV2CActorWithTag(TEXT("V2C_REGION_COVER")) && GetV2CActorWithTag(TEXT("V2C_REGION_CORE")) && GetV2CActorWithTag(TEXT("V2C_REGION_EXIT"));
	if (!GetWorld()->GetMapName().EndsWith(TEXT("L_V2_CombatDemo")) || GetWorld()->GetAuthGameMode() != this || !GetDemoPlayerController() || !Cast<Ademo_mapHUD>(GetDemoPlayerController()->GetHUD()) || PlayerStartCount != 1
		|| !Ground || !FMath::IsNearlyEqual(GroundSize.X, 5040.0f, 35.0f) || !FMath::IsNearlyEqual(GroundSize.Y, 6580.0f, 35.0f) || !bRegions || NavBoundsCount != 1
		|| FriendlyMarkers!=1 || MeleeMarkers!=1 || RangedMarkers!=1 || HeavyMarkers!=1 || TargetMarkers!=3 || ExitMarkers!=1 || TargetIndices.Num()!=3 || !bMarkerPaths)
	{
		FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: startup, map, regions, markers, or NavMesh.")); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2FINAL_AUTOMATION: startup and map passed."));
	UE_LOG(Logdemo_map, Log, TEXT("V2FINAL_AUTOMATION: initial roster passed."));

	int32 GroundCount=0, BoundaryCount=0;
	for (TActorIterator<Ademo_mapGrayboxBlock> It(GetWorld()); It; ++It)
	{
		GroundCount += It->GetBlockType()==Edemo_mapGrayboxBlockType::Ground ? 1 : 0;
		BoundaryCount += It->GetBlockType()==Edemo_mapGrayboxBlockType::BoundaryWall ? 1 : 0;
	}
	FHitResult GroundHit;
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(V2FinalGround), false, Pawn);
	const FVector PawnLocation = Pawn->GetActorLocation();
	const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(GroundHit, PawnLocation+FVector(0,0,100), PawnLocation-FVector(0,0,500), ECC_Visibility, GroundParams);
	auto BoundarySweep = [this](const FVector& Start, const FVector& End)
	{
		FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(V2FinalBoundary), false, GetDemoPawn());
		return GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(42.0f), Params) && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("V2C_BOUNDARY"));
	};
	AActor* Narrow=GetV2CActorWithTag(TEXT("V2C_REGION_NARROW")); AActor* Cover=GetV2CActorWithTag(TEXT("V2C_REGION_COVER")); AActor* Core=GetV2CActorWithTag(TEXT("V2C_REGION_CORE")); AActor* ExitRegion=GetV2CActorWithTag(TEXT("V2C_REGION_EXIT"));
	const bool bRoutes = Narrow && Cover && Core && ExitRegion && HasValidNavigationPath(PawnLocation,Narrow->GetActorLocation()) && HasValidNavigationPath(Narrow->GetActorLocation(),Cover->GetActorLocation()) && HasValidNavigationPath(Cover->GetActorLocation(),Core->GetActorLocation()) && HasValidNavigationPath(Core->GetActorLocation(),ExitRegion->GetActorLocation());
	if (GroundCount!=1 || BoundaryCount!=4 || !bGroundHit || !GroundHit.GetActor() || !GroundHit.GetActor()->ActorHasTag(TEXT("V2C_GROUND")) || !BoundarySweep(FVector(2200,0,88),FVector(2600,0,88)) || !BoundarySweep(FVector(-2200,0,88),FVector(-2600,0,88)) || !BoundarySweep(FVector(0,3000,88),FVector(0,3400,88)) || !BoundarySweep(FVector(0,-3000,88),FVector(0,-3400,88)) || !bRoutes)
	{
		FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: movement, ground, boundary, wall, or route collision.")); return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("V2FINAL_AUTOMATION: movement and collision passed."));

	AActor* CentralBlocker = GetV2CActorWithTag(TEXT("V2C_CENTRAL_BLOCKER"));
	if (!CentralBlocker) { FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: central blocker missing for enemy navigation.")); return; }
	const FVector ChaseBase = CentralBlocker->GetActorLocation();
	MoveV2BActor(Pawn, FVector(ChaseBase.X-410.0f,ChaseBase.Y,Pawn->GetActorLocation().Z)); MoveV2BActor(Enemy.Get(), FVector(ChaseBase.X+410.0f,ChaseBase.Y,Enemy->GetActorLocation().Z));
	V2FinalInitialDistance=FVector::Dist2D(Pawn->GetActorLocation(),Enemy->GetActorLocation()); Enemy->SetCombatSuppressed(false);
	V2FinalAutomationStep=0; ScheduleNextV2FinalAutomationStep(4.00f);
#endif
}

void Ademo_mapGameMode::RunV2FinalAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn(); Udemo_mapSkillComponent* Skills=GetV2BSkills(); Ademo_mapPlayerController* Controller=GetDemoPlayerController();
	Udemo_mapPlayerHealthComponent* Health=Pawn?Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>():nullptr; Ademo_mapGameState* Mission=GetWorld()?Cast<Ademo_mapGameState>(GetWorld()->GetGameState()):nullptr;
	if(!Pawn||!Skills||!Controller||!Health||!Mission||!Enemy.IsValid()||!RangedEnemy.IsValid()||!HeavyEnemy.IsValid()||!FriendlyUnit.IsValid()){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: runtime state disappeared."));return;}
	const FVector Base(700.0f,700.0f,Pawn->GetActorLocation().Z);
	auto PlaceTarget=[this,Pawn,&Base](AActor* Actor,float Distance){if(Actor)MoveV2BActor(Actor,FVector(Base.X+Distance,Base.Y,Actor->GetActorLocation().Z));MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator::ZeroRotator);};
	switch(V2FinalAutomationStep)
	{
	case 1:
		if(FVector::Dist2D(Pawn->GetActorLocation(),Enemy->GetActorLocation())>=V2FinalInitialDistance-80.0f){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: melee NavMesh chase."));return;} Enemy->SetCombatSuppressed(true);
		Pawn->SetActorEnableCollision(false); MoveV2BActor(RangedEnemy.Get(),FVector(Base.X,Base.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(400,0,0));V2FinalInitialDistance=400;V2FinalInitialRangedShots=RangedEnemy->GetTotalProjectilesFired();RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalAutomationStep(2.60f);return;
	case 2:
		{
			const float RangedDistance =
				FVector::Dist2D(
					Pawn->GetActorLocation(),
					RangedEnemy->GetActorLocation());
			const int32 RetreatMoveRequests =
				RangedEnemy->GetRetreatMoveRequestCount();
			const int32 CurrentShots =
				RangedEnemy->GetTotalProjectilesFired();
			const bool bWindup = RangedEnemy->HasActiveWindup();
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("V2_RANGED_DIAG: final_stage distance=%.2f retreat_requests=%d initial_projectiles=%d current_projectiles=%d windup=%d legacy=%d profile=%s."),
				RangedDistance,
				RetreatMoveRequests,
				V2FinalInitialRangedShots,
				CurrentShots,
				bWindup,
				RangedEnemy->UsesLegacyRangedBehavior(),
				*RangedEnemy->GetSkillProfileId().ToString());
			if (!Ademo_mapRangedEnemyCharacter::IsV2FinalRangedStageComplete(
					RangedDistance,
					RetreatMoveRequests,
					V2FinalInitialRangedShots,
					CurrentShots,
					bWindup))
			{
				FailAutomation(
					FString::Printf(
						TEXT("V2FINAL_AUTOMATION: FAIL: ranged retreat, safe range, windup, or fire. retreat_not_started=%d safe_range_not_reached=%d windup_or_fire_missing=%d."),
						RetreatMoveRequests < 1,
						RangedDistance < 650.0f,
						CurrentShots <= V2FinalInitialRangedShots
							&& !bWindup));
				return;
			}
			RangedEnemy->SetCombatSuppressed(true);
			Pawn->SetActorEnableCollision(true);
		}
		MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X,Base.Y,HeavyEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(800,0,0));V2FinalInitialDistance=800;HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalAutomationStep(1.20f);return;
	case 3:
		if(FVector::Dist2D(Pawn->GetActorLocation(),HeavyEnemy->GetActorLocation())>=V2FinalInitialDistance-70.0f||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: heavy chase or target filtering."));return;}HeavyEnemy->SetCombatSuppressed(true);UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: enemy navigation and behavior passed."));MoveV2BActor(Enemy.Get(),Base+FVector(-1100,0,0));MoveV2BActor(RangedEnemy.Get(),Base+FVector(-1100,300,0));MoveV2BActor(HeavyEnemy.Get(),Base+FVector(-1100,-300,0));
		PlaceTarget(GetTargetAt(0),170);MoveV2BActor(FriendlyUnit.Get(),Base+FVector(150,60,0));if(!Controller->TryBasicAttack()||!GetTargetAt(0)||GetTargetAt(0)->GetHealth()!=1||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: basic melee matrix."));return;}ScheduleNextV2FinalAutomationStep(0.55f);return;
	case 4:
		MoveV2BActor(FriendlyUnit.Get(),Base+FVector(0,600,0));MoveV2BActor(GetTargetAt(0),Base+FVector(-900,0,0));MoveV2BActor(GetTargetAt(1),Base+FVector(300,0,0));if(!Skills->TryCastGroundCircleAt(GetTargetAt(1)->GetActorLocation())||GetTargetAt(1)->GetHealth()!=1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: Ground Circle matrix."));return;}ScheduleNextV2FinalAutomationStep(0.20f);return;
	case 5:
		MoveV2BActor(GetTargetAt(1),Base+FVector(-900,300,0));PlaceTarget(GetTargetAt(2),200);if(!Skills->TryCastSelfSector(FVector::ForwardVector)||GetTargetAt(2)->GetHealth()!=1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: Self Sector matrix."));return;}ScheduleNextV2FinalAutomationStep(0.20f);return;
	case 6:
		MoveV2BActor(GetTargetAt(2),Base+FVector(-900,-300,0));PlaceTarget(GetTargetAt(0),650);if(!Skills->SpawnProjectile(FVector::ForwardVector)){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: Straight Projectile spawn."));return;}ScheduleNextV2FinalAutomationStep(0.85f);return;
	case 7:
		if(GetTargetAt(0)!=nullptr||Mission->GetDestroyedTargets()!=1||FriendlyUnit->GetCurrentHealth()!=5){FailAutomation(FString::Printf(TEXT("V2FINAL_AUTOMATION: FAIL: four player attacks or mission first target. target_valid=%d target_defeated=%d mission=%d friendly_health=%d"),GetTargetAt(0)!=nullptr,GetTargetAt(0)?GetTargetAt(0)->WasDestroyedByDamage():0,Mission->GetDestroyedTargets(),FriendlyUnit->GetCurrentHealth()));return;}UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: player attack matrix passed."));
		PlaceTarget(Enemy.Get(),650);V2FinalInitialEnemyHealth=Enemy->GetCurrentHealth();Skills->SpawnProjectile(FVector::ForwardVector);ScheduleNextV2FinalAutomationStep(0.85f);return;
	case 8:
		if(!Enemy.IsValid()||Enemy->GetCurrentHealth()!=V2FinalInitialEnemyHealth-1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: projectile melee contact."));return;}MoveV2BActor(Enemy.Get(),Base+FVector(-900,0,0));PlaceTarget(RangedEnemy.Get(),650);V2FinalInitialRangedHealth=RangedEnemy->GetCurrentHealth();Skills->SpawnProjectile(FVector::ForwardVector);ScheduleNextV2FinalAutomationStep(0.85f);return;
	case 9:
		if(RangedEnemy->GetCurrentHealth()!=V2FinalInitialRangedHealth-1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: projectile ranged contact."));return;}MoveV2BActor(RangedEnemy.Get(),Base+FVector(-900,300,0));PlaceTarget(HeavyEnemy.Get(),650);V2FinalInitialHeavyHealth=HeavyEnemy->GetCurrentHealth();Skills->SpawnProjectile(FVector::ForwardVector);ScheduleNextV2FinalAutomationStep(0.85f);return;
	case 10:
		if(HeavyEnemy->GetCurrentHealth()!=V2FinalInitialHeavyHealth-1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: projectile heavy contact."));return;}MoveV2BActor(HeavyEnemy.Get(),Base+FVector(-900,-300,0));PlaceTarget(GetTargetAt(1),650);MoveV2BActor(FriendlyUnit.Get(),Base+FVector(330,0,0));Skills->SpawnProjectile(FVector::ForwardVector);ScheduleNextV2FinalAutomationStep(0.85f);return;
	case 11:
		if(FriendlyUnit->GetCurrentHealth()!=5||GetTargetAt(1)!=nullptr||Mission->GetDestroyedTargets()!=2){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: projectile friendly pass or target contact."));return;}PlaceTarget(GetTargetAt(2),650);V2FinalBlockingWall=SpawnV2BBlockingWall(Base+FVector(400,0,55));Skills->SpawnProjectile(FVector::ForwardVector);ScheduleNextV2FinalAutomationStep(0.85f);return;
	case 12:
		if(!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: player projectile WorldStatic block."));return;}if(V2FinalBlockingWall.IsValid())V2FinalBlockingWall->Destroy();
		MoveV2BActor(RangedEnemy.Get(),FVector(Base.X,Base.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(800,0,0));MoveV2BActor(FriendlyUnit.Get(),Base+FVector(200,0,0));MoveV2BActor(Enemy.Get(),Base+FVector(360,0,0));MoveV2BActor(HeavyEnemy.Get(),Base+FVector(520,0,0));V2FinalInitialHealth=Health->GetCurrentHealth();
		{Fdemo_mapProjectileSkillParams P;P.Width=50;P.CollisionRadius=25;P.Speed=800;P.MaxDistance=1800;P.CommonParams.Damage=1;P.bPassThroughFriendlies=true;FActorSpawnParameters S;S.Owner=RangedEnemy.Get();S.Instigator=RangedEnemy.Get();S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;Ademo_mapSkillProjectile* Projectile=GetWorld()->SpawnActor<Ademo_mapSkillProjectile>(Ademo_mapSkillProjectile::StaticClass(),FVector(Base.X+105,Base.Y,RangedEnemy->GetActorLocation().Z+55),FRotator::ZeroRotator,S);if(Projectile)Projectile->InitializeTargetedProjectile(RangedEnemy.Get(),Pawn,FVector::ForwardVector,P,FLinearColor(0.9f,0.02f,1.0f));else{FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: enemy projectile spawn."));return;}}
		ScheduleNextV2FinalAutomationStep(1.20f);return;
	case 13:
		if(Health->GetCurrentHealth()!=V2FinalInitialHealth-1||FriendlyUnit->GetCurrentHealth()!=5||Enemy->GetCurrentHealth()!=V2FinalInitialEnemyHealth-1||HeavyEnemy->GetCurrentHealth()!=V2FinalInitialHeavyHealth-1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: enemy projectile target-only integrity."));return;}UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: projectile integrity passed."));
		MoveV2BActor(FriendlyUnit.Get(),Base+FVector(0,700,0));MoveV2BActor(Enemy.Get(),Base+FVector(-900,0,0));MoveV2BActor(RangedEnemy.Get(),Base+FVector(-900,300,0));MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X,Base.Y,HeavyEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(300,0,0));V2FinalInitialHealth=Health->GetCurrentHealth();V2FinalInitialHeavyResolves=HeavyEnemy->GetResolveCount();HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalAutomationStep(1.15f);return;
	case 14:
		if(Health->GetCurrentHealth()!=V2FinalInitialHealth-1||HeavyEnemy->GetResolveCount()!=V2FinalInitialHeavyResolves+1){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: heavy telegraph or single resolve damage."));return;}HeavyEnemy->SetCombatSuppressed(true);ScheduleNextV2FinalAutomationStep(2.55f);return;
	case 15:
		MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X,Base.Y,HeavyEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(300,0,0));V2FinalInitialHealth=Health->GetCurrentHealth();HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalAutomationStep(0.30f);return;
	case 16:
		if(!HeavyEnemy->HasActiveTelegraph()){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: heavy dodge telegraph."));return;}MoveV2BActor(Pawn,Base+FVector(0,600,0));ScheduleNextV2FinalAutomationStep(0.90f);return;
	case 17:
		if(Health->GetCurrentHealth()!=V2FinalInitialHealth){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: heavy dodge."));return;}HeavyEnemy->SetCombatSuppressed(true);ScheduleNextV2FinalAutomationStep(2.55f);return;
	case 18:
		MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X,Base.Y,HeavyEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(350,0,0));V2FinalBlockingWall=SpawnV2BBlockingWall(Base+FVector(175,0,55));V2FinalInitialHealth=Health->GetCurrentHealth();HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalAutomationStep(1.20f);return;
	case 19:
		if(Health->GetCurrentHealth()!=V2FinalInitialHealth){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: heavy WorldStatic obstruction."));return;}HeavyEnemy->SetCombatSuppressed(true);if(V2FinalBlockingWall.IsValid())V2FinalBlockingWall->Destroy();UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: heavy sector passed."));
		V2FinalInitialMissionCount=Mission->GetDestroyedTargets();UGameplayStatics::ApplyDamage(Enemy.Get(),10.0f,Controller,Pawn,nullptr);ScheduleNextV2FinalAutomationStep(0.20f);return;
	case 20:
		if(!Enemy.IsValid()||!Enemy->IsDead()||Enemy->GetCurrentHealth()!=0||Mission->GetDestroyedTargets()!=V2FinalInitialMissionCount){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: damage, death, or ordinary enemy mission identity."));return;}UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: damage and death passed."));
		if(!PlacePawnForTarget(GetTargetAt(2))||!Controller->TryBasicAttack()){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: final mission target real damage."));return;}ScheduleNextV2FinalAutomationStep(0.30f);return;
	case 21:
		if(GetTargetAt(2)!=nullptr||Mission->GetDestroyedTargets()!=3||!ExitZone.IsValid()||!ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: mission progression or exit unlock."));return;}GV2FinalAutomationPhase=1;PlacePawnInExit(true);return;
	case 100:
	{
		TArray<AActor*> Projectiles;UGameplayStatics::GetAllActorsOfClass(GetWorld(),Ademo_mapSkillProjectile::StaticClass(),Projectiles);if(Projectiles.Num()!=0){FailAutomation(FString::Printf(TEXT("V2FINAL_AUTOMATION: FAIL: bounded projectile lifecycle final=%d."),Projectiles.Num()));return;}if(V2FinalBlockingWall.IsValid())V2FinalBlockingWall->Destroy();
		UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: bounded projectile lifecycle passed."));
		UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_METRIC: map_load_seconds=%.3f nav_init_seconds=%.3f initial_actor_count=%d reload_actor_counts=%d,%d,%d initial_ai_controllers=%d projectile_peak=%d projectile_final=0."),GV2FinalMeasuredLoadSeconds,GV2FinalMeasuredNavigationSeconds,GV2FinalInitialActorCount,GV2FinalResetActorCounts[0],GV2FinalResetActorCounts[1],GV2FinalResetActorCounts[2],GV2FinalInitialAIControllerCount,V2FinalProjectilePeak);
		UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_AUTOMATION: PASS."));GV2FinalAutomationPhase=0;FPlatformMisc::RequestExitWithStatus(false,0);return;
	}
	default:FailAutomation(TEXT("V2FINAL_AUTOMATION: FAIL: invalid step."));return;
	}
#endif
}

void Ademo_mapGameMode::ScheduleNextV2FinalVisibleStep(float Delay)
{
	++V2FinalVisibleStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV2FinalVisibleStep, Delay, false);
}

void Ademo_mapGameMode::StartV2FinalVisibleAcceptance()
{
#if !UE_BUILD_SHIPPING
	if(!IsV2CMap()||!GetDemoPawn()||!GetV2BSkills()||!Enemy.IsValid()||!RangedEnemy.IsValid()||!HeavyEnemy.IsValid()||!FriendlyUnit.IsValid()||SpawnedTargets.Num()!=3||T7RVisualOutputDirectory.IsEmpty()){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: roster or output directory."));return;}
	Enemy->SetCombatSuppressed(true);RangedEnemy->SetCombatSuppressed(true);HeavyEnemy->SetCombatSuppressed(true);
	if(GV2FinalVisiblePhase==1)
	{
		V2FinalVisibleStep=20;RunV2FinalVisibleStep();return;
	}
	V2FinalVisibleStep=0;RunV2FinalVisibleStep();
#endif
}

void Ademo_mapGameMode::RunV2FinalVisibleStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn=GetDemoPawn();Udemo_mapSkillComponent* Skills=GetV2BSkills();Udemo_mapPlayerHealthComponent* Health=Pawn?Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>():nullptr;Ademo_mapPlayerController* Controller=GetDemoPlayerController();
	if(!Pawn||!Skills||!Health||!Controller){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: runtime."));return;}
	const FVector Base(700.0f,700.0f,Pawn->GetActorLocation().Z);
	switch(V2FinalVisibleStep)
	{
	case 0:MoveV2BActor(Pawn,FVector(0,-2555,Pawn->GetActorLocation().Z));MoveV2BActor(FriendlyUnit.Get(),FVector(-315,-2555,FriendlyUnit->GetActorLocation().Z));MoveV2BActor(Enemy.Get(),FVector(280,-2350,Enemy->GetActorLocation().Z));MoveV2BActor(RangedEnemy.Get(),FVector(520,-2555,RangedEnemy->GetActorLocation().Z));MoveV2BActor(HeavyEnemy.Get(),FVector(300,-2750,HeavyEnemy->GetActorLocation().Z));ScheduleNextV2FinalVisibleStep(0.70f);return;
	case 1:CaptureT7RVisual(TEXT("01_V2FINAL_INITIAL.png"));ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 2:{Ademo_mapTrainingTarget* T=GetTargetAt(0);if(!PlacePawnForTarget(T)){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: ally setup."));return;}MoveV2BActor(FriendlyUnit.Get(),T->GetActorLocation()+FVector(0,65,0));if(!Controller->TryBasicAttack()){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: ally attack."));return;}ScheduleNextV2FinalVisibleStep(0.20f);return;}
	case 3:if(FriendlyUnit->GetCurrentHealth()!=5||!GetTargetAt(0)||GetTargetAt(0)->GetHealth()!=1){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: ally immunity."));return;}CaptureT7RVisual(TEXT("02_V2FINAL_ALLY_IMMUNITY.png"));ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 4:MoveV2BActor(Pawn,Base);MoveV2BActor(GetTargetAt(1),Base+FVector(260,0,0));Skills->TryCastGroundCircleAt(GetTargetAt(1)->GetActorLocation());ScheduleNextV2FinalVisibleStep(0.08f);return;
	case 5:CaptureT7RVisual(TEXT("03_V2FINAL_GROUND_CIRCLE.png"));ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 6:MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator::ZeroRotator);MoveV2BActor(HeavyEnemy.Get(),Base+FVector(250,0,0));Skills->TryCastSelfSector(FVector::ForwardVector);ScheduleNextV2FinalVisibleStep(0.08f);return;
	case 7:CaptureT7RVisual(TEXT("04_V2FINAL_SELF_SECTOR.png"));ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 8:MoveV2BActor(Pawn,Base);Pawn->SetActorRotation(FRotator::ZeroRotator);MoveV2BActor(HeavyEnemy.Get(),Base+FVector(-900,-300,0));if(GetTargetAt(1))MoveV2BActor(GetTargetAt(1),Base+FVector(-900,300,0));MoveV2BActor(FriendlyUnit.Get(),Base+FVector(0,600,0));MoveV2BActor(GetTargetAt(2),FVector(Base.X+500,Base.Y,GetTargetAt(2)->GetActorLocation().Z));Skills->SpawnProjectile(FVector::ForwardVector);ScheduleNextV2FinalVisibleStep(0.55f);return;
	case 9:if(!GetTargetAt(2)||GetTargetAt(2)->GetHealth()!=1||Skills->GetLastSpawnedProjectile()!=nullptr){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: projectile target hit."));return;}CaptureT7RVisual(TEXT("05_V2FINAL_PROJECTILE_TARGET_HIT.png"));ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 10:MoveV2BActor(RangedEnemy.Get(),FVector(Base.X,Base.Y,RangedEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(800,0,0));RangedEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalVisibleStep(0.35f);return;
	case 11:if(RangedEnemy->GetRangedState()==Edemo_mapRangedEnemyState::Idle){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: ranged behavior."));return;}CaptureT7RVisual(TEXT("06_V2FINAL_RANGED_BEHAVIOR.png"));RangedEnemy->SetCombatSuppressed(true);ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 12:MoveV2BActor(HeavyEnemy.Get(),FVector(Base.X,Base.Y,HeavyEnemy->GetActorLocation().Z));MoveV2BActor(Pawn,Base+FVector(300,0,0));HeavyEnemy->SetCombatSuppressed(false);ScheduleNextV2FinalVisibleStep(0.30f);return;
	case 13:if(!HeavyEnemy->HasActiveTelegraph()){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: heavy telegraph."));return;}CaptureT7RVisual(TEXT("07_V2FINAL_HEAVY_TELEGRAPH.png"));ScheduleNextV2FinalVisibleStep(0.90f);return;
	case 14:if(Health->GetCurrentHealth()>=5){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: player damage."));return;}CaptureT7RVisual(TEXT("08_V2FINAL_PLAYER_DAMAGED.png"));HeavyEnemy->SetCombatSuppressed(true);ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 15:GV2FinalVisiblePhase=1;UGameplayStatics::ApplyDamage(Pawn,10.0f,Enemy->GetController(),Enemy.Get(),nullptr);ScheduleNextV2FinalVisibleStep(0.15f);return;
	case 16:if(!Health->IsDefeated()||!GetEndStatusText().StartsWith(TEXT("DEFEATED"))){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: defeated state."));return;}CaptureT7RVisual(TEXT("09_V2FINAL_DEFEATED.png"));return;
	case 20:for(int32 Index=0;Index<3;++Index)if(Ademo_mapTrainingTarget* T=GetTargetAt(Index))UGameplayStatics::ApplyDamage(T,2.0f,Controller,Pawn,nullptr);ScheduleNextV2FinalVisibleStep(0.40f);return;
	case 21:if(!ExitZone.IsValid()||!ExitZone->IsExitUnlocked()){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: exit open."));return;}CaptureT7RVisual(TEXT("10_V2FINAL_EXIT_OPEN.png"));ScheduleNextV2FinalVisibleStep(0.45f);return;
	case 22:PlacePawnInExit(true);ScheduleNextV2FinalVisibleStep(0.15f);return;
	case 23:if(!GetEndStatusText().StartsWith(TEXT("EXTRACTED"))){FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: extracted state."));return;}CaptureT7RVisual(TEXT("11_V2FINAL_EXTRACTED.png"));ScheduleNextV2FinalVisibleStep(0.55f);return;
	case 24:UE_LOG(Logdemo_map,Log,TEXT("V2FINAL_VISIBLE_ACCEPTANCE: PASS."));GV2FinalVisiblePhase=0;FPlatformMisc::RequestExitWithStatus(false,0);return;
	default:FailAutomation(TEXT("V2FINAL_VISIBLE_ACCEPTANCE: FAIL: invalid step."));return;
	}
#endif
}

void Ademo_mapGameMode::StartV3AttributesGameplayAutomation()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapAttributeComponent* Attributes = Pawn ? Pawn->FindComponentByClass<Udemo_mapAttributeComponent>() : nullptr;
	Udemo_mapPlayerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Udemo_mapSkillComponent* Skills = GetV2BSkills();
	ACharacter* Character = Cast<ACharacter>(Pawn);
	Ademo_mapGameState* Mission = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!Pawn || !Attributes || !Health || !Skills || !Character || !Mission || !Enemy.IsValid() || !FriendlyUnit.IsValid() || !ExitZone.IsValid())
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: runtime foundation missing.")); return;
	}
	if (RangedEnemy.IsValid()) RangedEnemy->SetCombatSuppressed(true);
	if (HeavyEnemy.IsValid()) HeavyEnemy->SetCombatSuppressed(true);
	Enemy->SetCombatSuppressed(true);

	auto GetValue = [Attributes](FName Id){ float Value = 0.0f; Attributes->GetFinalValue(Id, Value); return Value; };
	if (!FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::Primary01), 0.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::Primary02), 0.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::Primary03), 0.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::MaxHealth), 5.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::MoveSpeed), 600.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::AttackPower), 1.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::DodgeChance), 0.0f)
		|| Attributes->GetActiveModifierCount() != 0 || Health->GetCurrentHealth() != 5 || Health->GetMaxHealth() != 5)
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: default values.")); return;
	}

	Fdemo_mapModifierSpec Spec;
	Spec.SourceId = TEXT("Automation.HealthPolicy"); Spec.AttributeId = Fdemo_mapAttributeIds::Primary03; Spec.Operation = Edemo_mapModifierOperation::Add; Spec.Value = 2.0f;
	Fdemo_mapModifierHandle Handle;
	if (!Attributes->AddModifier(Spec, Handle) || Health->GetMaxHealth() != 7 || Health->GetCurrentHealth() != 5)
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: max-health increase policy.")); return;
	}
	Health->SetCurrentHealthForAutomation(6);
	if (!Attributes->RemoveModifier(Handle) || Health->GetMaxHealth() != 5 || Health->GetCurrentHealth() != 5)
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: max-health decrease clamp.")); return;
	}
	Attributes->ForceRecalculate();
	if (Health->GetCurrentHealth() != 5) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: repeated health recalc.")); return; }

	Spec.SourceId = TEXT("Automation.Move"); Spec.AttributeId = Fdemo_mapAttributeIds::Primary02; Spec.Value = 3.0f;
	if (!Attributes->AddModifier(Spec, Handle) || !FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, 630.0f)
		|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::DodgeChance), 0.15f))
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: primary move/dodge integration.")); return;
	}
	if (!Attributes->RemoveModifier(Handle) || !FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, 600.0f))
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: move speed restore.")); return;
	}
	Spec.SourceId = TEXT("Automation.MoveDirect"); Spec.AttributeId = Fdemo_mapAttributeIds::MoveSpeed; Spec.Operation = Edemo_mapModifierOperation::Multiply; Spec.Value = 1.1f;
	if (!Attributes->AddModifier(Spec, Handle) || !FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, 660.0f)
		|| !Attributes->RemoveModifier(Handle) || !FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, 600.0f))
	{
		FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: direct move modifier.")); return;
	}

	Health->SetCurrentHealthForAutomation(5);
	UGameplayStatics::ApplyDamage(Pawn, 1.0f, Enemy->GetController(), Enemy.Get(), nullptr);
	if (Health->GetCurrentHealth() != 4) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: default incoming damage regression.")); return; }
	Health->SetCurrentHealthForAutomation(5);
	for (int32 Round = 0; Round < 3; ++Round)
	{
		Spec.SourceId = FName(*FString::Printf(TEXT("Automation.Residue.%d"), Round)); Spec.AttributeId = Fdemo_mapAttributeIds::Primary01; Spec.Operation = Edemo_mapModifierOperation::Add; Spec.Value = 2.0f;
		if (!Attributes->AddModifier(Spec, Handle) || !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::AttackPower), 3.0f) || !Attributes->RemoveModifier(Handle)
			|| !FMath::IsNearlyEqual(GetValue(Fdemo_mapAttributeIds::AttackPower), 1.0f))
		{
			FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: modifier residue cycle.")); return;
		}
	}
	if (Attributes->GetActiveModifierCount() != 0) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: active modifier residue.")); return; }

	const FVector Base(700.0f, 700.0f, Pawn->GetActorLocation().Z);
	MoveV2BActor(Pawn, Base); Pawn->SetActorRotation(FRotator::ZeroRotator);
	MoveV2BActor(FriendlyUnit.Get(), Base + FVector(0, 900, 0));
	MoveV2BActor(Enemy.Get(), Base + FVector(-1000, 0, 0));
	if (RangedEnemy.IsValid()) MoveV2BActor(RangedEnemy.Get(), Base + FVector(-1000, 300, 0));
	if (HeavyEnemy.IsValid()) MoveV2BActor(HeavyEnemy.Get(), Base + FVector(-1000, -300, 0));
	for (int32 Index = 0; Index < 3; ++Index) if (Ademo_mapTrainingTarget* Target = GetTargetAt(Index)) MoveV2BActor(Target, Base + FVector(-1200, Index * 250.0f, 0));
	V3AttributesAutomationStep = 0;
	UE_LOG(Logdemo_map, Log, TEXT("V3_ATTRIBUTES_GAMEPLAY: defaults, health, movement, dodge baseline, and residue passed."));
	ScheduleNextV3AttributesGameplayAutomationStep(0.10f);
#endif
}

void Ademo_mapGameMode::ScheduleNextV3AttributesGameplayAutomationStep(float Delay)
{
	++V3AttributesAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV3AttributesGameplayAutomationStep, Delay, false);
}

void Ademo_mapGameMode::RunV3AttributesGameplayAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn(); Udemo_mapAttributeComponent* Attributes = Pawn ? Pawn->FindComponentByClass<Udemo_mapAttributeComponent>() : nullptr;
	Udemo_mapSkillComponent* Skills = GetV2BSkills(); Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	Ademo_mapGameState* Mission = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!Pawn || !Attributes || !Skills || !Controller || !Mission) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: runtime disappeared.")); return; }
	const FVector Base(700.0f, 700.0f, Pawn->GetActorLocation().Z);
	auto CleanupTarget = [this]() { if (V3AttributeTestTarget.IsValid()) V3AttributeTestTarget->Destroy(); V3AttributeTestTarget.Reset(); };
	auto SpawnTarget = [this, Pawn, Base](float Distance)
	{
		if (V3AttributeTestTarget.IsValid()) V3AttributeTestTarget->Destroy();
		MoveV2BActor(Pawn, Base); Pawn->SetActorRotation(FRotator::ZeroRotator);
		V3AttributeTestTarget = SpawnV2BTestEnemy(FVector(Base.X + Distance, Base.Y, Base.Z));
		return V3AttributeTestTarget.Get();
	};
	auto AddAttackPower = [this, Attributes]()
	{
		Fdemo_mapModifierSpec Spec; Spec.SourceId = TEXT("Automation.AttackPower"); Spec.AttributeId = Fdemo_mapAttributeIds::AttackPower;
		Spec.Operation = Edemo_mapModifierOperation::Add; Spec.Value = 1.0f; Spec.Priority = 0;
		return Attributes->AddModifier(Spec, V3AttackModifierHandle);
	};
	auto RemoveAttackPower = [this, Attributes]() { const bool bRemoved = Attributes->RemoveModifier(V3AttackModifierHandle); V3AttackModifierHandle.Reset(); return bRemoved; };

	switch (V3AttributesAutomationStep)
	{
	case 1:
		if (!SpawnTarget(170.0f) || !Controller->TryBasicAttack()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: LMB default setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.10f); return;
	case 2:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 2) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: LMB default damage.")); return; }
		CleanupTarget(); if (!AddAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: LMB power modifier.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.50f); return;
	case 3:
		if (!SpawnTarget(170.0f) || !Controller->TryBasicAttack()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: LMB powered setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.10f); return;
	case 4:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 1 || !RemoveAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: LMB powered damage/restore.")); return; }
		CleanupTarget(); if (!SpawnTarget(300.0f) || !Skills->TryCastGroundCircleAt(V3AttributeTestTarget->GetActorLocation())) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: Q default setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.10f); return;
	case 5:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 2 || !AddAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: Q default damage/modifier.")); return; }
		CleanupTarget(); ScheduleNextV3AttributesGameplayAutomationStep(1.50f); return;
	case 6:
		if (!SpawnTarget(300.0f) || !Skills->TryCastGroundCircleAt(V3AttributeTestTarget->GetActorLocation())) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: Q powered setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.10f); return;
	case 7:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 1 || !RemoveAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: Q powered damage/restore.")); return; }
		CleanupTarget(); if (!SpawnTarget(200.0f) || !Skills->TryCastSelfSector(FVector::ForwardVector)) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: E default setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.10f); return;
	case 8:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 2 || !AddAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: E default damage/modifier.")); return; }
		CleanupTarget(); ScheduleNextV3AttributesGameplayAutomationStep(1.00f); return;
	case 9:
		if (!SpawnTarget(200.0f) || !Skills->TryCastSelfSector(FVector::ForwardVector)) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: E powered setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.10f); return;
	case 10:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 1 || !RemoveAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: E powered damage/restore.")); return; }
		CleanupTarget(); if (!SpawnTarget(650.0f) || !Skills->TryFireStraightProjectile(FVector::ForwardVector)) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: F default setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.85f); return;
	case 11:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 2 || Skills->GetLastSpawnedProjectile() != nullptr || !AddAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: F default damage/modifier.")); return; }
		CleanupTarget(); if (!SpawnTarget(650.0f) || !Skills->TryFireStraightProjectile(FVector::ForwardVector)) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: F powered setup.")); return; }
		ScheduleNextV3AttributesGameplayAutomationStep(0.85f); return;
	case 12:
		if (!V3AttributeTestTarget.IsValid() || V3AttributeTestTarget->GetCurrentHealth() != 1 || Skills->GetLastSpawnedProjectile() != nullptr || !RemoveAttackPower()) { FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: F powered damage/restore.")); return; }
		CleanupTarget();
		if (Attributes->GetActiveModifierCount() != 0 || FriendlyUnit->GetCurrentHealth() != 5 || Mission->GetDestroyedTargets() != 0 || ExitZone->IsExitUnlocked())
		{
			FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: final residue or V2 mission/friendly regression.")); return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V3_ATTRIBUTES_GAMEPLAY: LMB/Q/E/F default=1 powered=2 restored=1 passed."));
		UE_LOG(Logdemo_map, Log, TEXT("V3_ATTRIBUTES_GAMEPLAY: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0); return;
	default: FailAutomation(TEXT("V3_ATTRIBUTES_GAMEPLAY: FAIL: invalid step.")); return;
	}
#endif
}

void Ademo_mapGameMode::StartV3ItemCoreGameplayAutomation()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn();
	Udemo_mapAttributeComponent* Attributes = Pawn ? Pawn->FindComponentByClass<Udemo_mapAttributeComponent>() : nullptr;
	Udemo_mapPlayerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Udemo_mapItemSubsystem* Items = PlayerItemSubsystem.Get();
	ACharacter* Character = Cast<ACharacter>(Pawn);
	if (!Pawn || !Attributes || !Health || !Items || !Character || !Enemy.IsValid())
	{
		FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: runtime foundation missing.")); return;
	}
	if (RangedEnemy.IsValid()) RangedEnemy->SetCombatSuppressed(true);
	if (HeavyEnemy.IsValid()) HeavyEnemy->SetCombatSuppressed(true);
	Enemy->SetCombatSuppressed(true);
	Items->ResetForAutomation();
	if (!Items->BindPlayerPawn(Pawn)) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: initial attribute binding.")); return; }

	auto AddOne = [Items](FName DefinitionId, FGuid& OutId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result = Items->AddDefinition(DefinitionId, 1, &Affected);
		if (!Result.bSuccess || Affected.Num() != 1) return false;
		OutId = Affected[0];
		return OutId.IsValid();
	};
	auto GetAttribute = [Attributes](FName Id) { float Value = 0.0f; Attributes->GetFinalValue(Id, Value); return Value; };
	if (!AddOne(Fdemo_mapItemIds::TrainingBlade, V3ItemWeaponId)
		|| !AddOne(Fdemo_mapItemIds::TrainingVest, V3ItemArmorId)
		|| !AddOne(Fdemo_mapItemIds::WindTalisman, V3ItemAccessoryId)
		|| Items->GetAuthority().GetUsedInventorySlots() != 3
		|| !Items->Equip(V3ItemWeaponId, Fdemo_mapItemIds::WeaponSlot).bSuccess
		|| !FMath::IsNearlyEqual(GetAttribute(Fdemo_mapAttributeIds::AttackPower), 2.0f)
		|| !Items->BindPlayerPawn(Pawn) || !Items->SynchronizeEquipmentModifiers()
		|| !FMath::IsNearlyEqual(GetAttribute(Fdemo_mapAttributeIds::AttackPower), 2.0f)
		|| Items->GetActiveModifierSources().Num() != 1)
	{
		FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: item creation, weapon equip, or idempotent binding.")); return;
	}

	const FVector Base(700.0f, 700.0f, Pawn->GetActorLocation().Z);
	MoveV2BActor(Pawn, Base); Pawn->SetActorRotation(FRotator::ZeroRotator);
	MoveV2BActor(FriendlyUnit.Get(), Base + FVector(0, 900, 0));
	MoveV2BActor(Enemy.Get(), Base + FVector(-1000, 0, 0));
	V3ItemTestTarget = SpawnV2BTestEnemy(Base + FVector(170, 0, 0));
	Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	if (!V3ItemTestTarget.IsValid() || !Controller || !Controller->TryBasicAttack())
	{
		FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: powered LMB setup.")); return;
	}
	V3ItemAutomationStep = 0;
	UE_LOG(Logdemo_map, Log, TEXT("V3_ITEM_CORE_GAMEPLAY: authority, inventory, weapon modifier, and duplicate sync passed."));
	ScheduleNextV3ItemCoreGameplayAutomationStep(0.15f);
#endif
}

void Ademo_mapGameMode::ScheduleNextV3ItemCoreGameplayAutomationStep(float Delay)
{
	++V3ItemAutomationStep;
	GetWorldTimerManager().SetTimer(AutomationTimerHandle, this, &Ademo_mapGameMode::RunV3ItemCoreGameplayAutomationStep, Delay, false);
}

void Ademo_mapGameMode::RunV3ItemCoreGameplayAutomationStep()
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetDemoPawn(); Udemo_mapAttributeComponent* Attributes = Pawn ? Pawn->FindComponentByClass<Udemo_mapAttributeComponent>() : nullptr;
	Udemo_mapPlayerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	Udemo_mapItemSubsystem* Items = PlayerItemSubsystem.Get(); ACharacter* Character = Cast<ACharacter>(Pawn); Ademo_mapPlayerController* Controller = GetDemoPlayerController();
	if (!Pawn || !Attributes || !Health || !Items || !Character || !Controller) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: runtime disappeared.")); return; }
	auto GetAttribute = [Attributes](FName Id) { float Value = 0.0f; Attributes->GetFinalValue(Id, Value); return Value; };
	const FVector Base(700.0f, 700.0f, Pawn->GetActorLocation().Z);
	switch (V3ItemAutomationStep)
	{
	case 1:
		if (!V3ItemTestTarget.IsValid() || V3ItemTestTarget->GetCurrentHealth() != 1) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: equipped weapon LMB damage was not 2.")); return; }
		V3ItemTestTarget->Destroy(); V3ItemTestTarget.Reset();
		if (!Items->Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess || !FMath::IsNearlyEqual(GetAttribute(Fdemo_mapAttributeIds::AttackPower), 1.0f)) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: weapon unequip restore.")); return; }
		ScheduleNextV3ItemCoreGameplayAutomationStep(0.50f); return;
	case 2:
		MoveV2BActor(Pawn, Base); Pawn->SetActorRotation(FRotator::ZeroRotator);
		V3ItemTestTarget = SpawnV2BTestEnemy(Base + FVector(170, 0, 0));
		if (!V3ItemTestTarget.IsValid() || !Controller->TryBasicAttack()) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: restored LMB setup.")); return; }
		ScheduleNextV3ItemCoreGameplayAutomationStep(0.15f); return;
	case 3:
	{
		if (!V3ItemTestTarget.IsValid() || V3ItemTestTarget->GetCurrentHealth() != 2) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: unequipped LMB damage was not 1.")); return; }
		V3ItemTestTarget->Destroy(); V3ItemTestTarget.Reset();
		if (!Items->Equip(V3ItemArmorId, Fdemo_mapItemIds::ArmorSlot).bSuccess || Health->GetMaxHealth() != 7 || Health->GetCurrentHealth() != 5) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: armor max-health policy.")); return; }
		Health->SetCurrentHealthForAutomation(6);
		if (!Items->Unequip(Fdemo_mapItemIds::ArmorSlot).bSuccess || Health->GetMaxHealth() != 5 || Health->GetCurrentHealth() != 5) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: armor clamp policy.")); return; }
		const float EnemySpeed = Enemy.IsValid() ? Enemy->GetCharacterMovement()->MaxWalkSpeed : -1.0f;
		if (!Items->Equip(V3ItemAccessoryId, Fdemo_mapItemIds::SpatialRingSlot).bSuccess || !FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, 660.0f) || !Enemy.IsValid() || !FMath::IsNearlyEqual(Enemy->GetCharacterMovement()->MaxWalkSpeed, EnemySpeed)) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: spatial ring movement isolation.")); return; }
		Fdemo_mapModifierSpec External; External.SourceId = TEXT("Automation.NonEquipment"); External.AttributeId = Fdemo_mapAttributeIds::AttackPower; External.Operation = Edemo_mapModifierOperation::Add; External.Value = 5.0f;
		Fdemo_mapModifierHandle ExternalHandle;
		if (!Attributes->AddModifier(External, ExternalHandle) || !Items->Unequip(Fdemo_mapItemIds::SpatialRingSlot).bSuccess || Attributes->GetModifierCountBySource(External.SourceId) != 1 || !FMath::IsNearlyEqual(GetAttribute(Fdemo_mapAttributeIds::AttackPower), 6.0f) || !FMath::IsNearlyEqual(Character->GetCharacterMovement()->MaxWalkSpeed, 600.0f)) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: source isolation or spatial-ring restore.")); return; }
		if (!Attributes->RemoveModifier(ExternalHandle) || !FMath::IsNearlyEqual(GetAttribute(Fdemo_mapAttributeIds::AttackPower), 1.0f)) { FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: non-equipment source cleanup.")); return; }
		FString InvariantError;
		if (!Items->BindPlayerPawn(Pawn) || !Items->ValidateInvariants(&InvariantError) || Items->GetActiveModifierSources().Num() != 0)
		{
			FailAutomation(FString::Printf(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: final invariants: %s"), *InvariantError)); return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V3_ITEM_CORE_GAMEPLAY: real LMB 2->1, armor 5/7 policy, accessory 600->660->600, and source isolation passed."));
		UE_LOG(Logdemo_map, Log, TEXT("V3_ITEM_CORE_GAMEPLAY: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0); return;
	}
	default: FailAutomation(TEXT("V3_ITEM_CORE_GAMEPLAY: FAIL: invalid step.")); return;
	}
#endif
}

void Ademo_mapGameMode::FailAutomation(const FString& FailureMessage)
{
	UE_LOG(Logdemo_map, Error, TEXT("%s"), *FailureMessage);
	FPlatformMisc::RequestExitWithStatus(false, 1);
}

void Ademo_mapGameMode::FailPackagedSmokeTest(const FString& FailureMessage)
{
	UE_LOG(Logdemo_map, Error, TEXT("%s"), *FailureMessage);
	FPlatformMisc::RequestExitWithStatus(false, 1);
}
