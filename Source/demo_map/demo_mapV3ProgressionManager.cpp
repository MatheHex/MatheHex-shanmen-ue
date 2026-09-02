#include "demo_mapV3ProgressionManager.h"
#include "CodeB/demo_mapCodeBP3UI.h"
#include "CodeB/demo_mapCodeBRunContainerPresentation.h"
#include "demo_map.h"
#include "demo_mapInteractable.h"
#include "demo_mapInventoryWidget.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapSectNavigationWidget.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapProfileTradeTransaction.h"
#include "demo_mapSpiritStonePickup.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapSettlementWidget.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapLootChest.h"
#include "demo_mapSearchContainerActor.h"
#include "demo_mapCodeBNormalContainerActor.h"
#include "demo_mapCodeBWorldDropActor.h"
#include "demo_mapCorpseContainerActor.h"
#include "demo_mapSearchContainerWidget.h"
#include "demo_mapSearchContainerTypes.h"
#include "demo_mapPlayerController.h"
#include "demo_mapV3ProgressionMarker.h"
#include "demo_mapWorldItem.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapEquipmentEffectResolver.h"
#include "demo_mapPlayerCombat.h"
#include "demo_mapTrainingTarget.h"
#include "demo_mapFriendlyUnit.h"
#include "demo_mapExitZone.h"
#include "demo_mapGameState.h"
#include "demo_mapGameMode.h"
#include "demo_map0909BFramework.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapSkillProjectile.h"
#include "demo_mapEnemySkillRuntimeComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardSourceProjection.h"
#include "demo_mapRewardFullMapDistribution.h"
#include "demo_mapM01RewardDistribution.h"
#include "demo_mapM01Marker.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapTownProgressionRules.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardRareExtreme.h"
#include "demo_mapRewardShopStock.h"
#include "demo_mapSearchContainerPresenter.h"
#include "Engine/GameInstance.h"
#include "demo_mapEncounterMarker.h"
#include "demo_mapKnockbackComponent.h"
#include "demo_mapCombatDisplacement.h"
#include "AIController.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "CollisionQueryParams.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"
#include "Framework/Application/SlateApplication.h"

#if !UE_BUILD_SHIPPING
namespace
{
	int32 GRunLifecycleAutomationPhase = 0;
	int32 GRunLifecycleVisiblePhase = 0;
	int32 GRepairVisiblePhase = 0;
	int32 GV3FinalAutomationPhase = 0;
	int32 GV3FinalVisiblePhase = 0;
	TSet<FGuid> GV3FinalRunIds;
	TSet<FGuid> GV3FinalSettlementIds;
	TMap<FGuid, FGuid> GV3FinalExpectedStashOrigins;
	int32 GV3FinalExpectedStashQuantity = 0;
	int32 GV3FinalExpectedStashValue = 0;

	FString CanonicalSearchContainerDirectory(const FString& Path)
	{
		FString Result = FPaths::ConvertRelativePathToFull(Path);
		FPaths::CollapseRelativeDirectories(Result);
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}

	bool IsSameOrChildSearchContainerPath(
		const FString& Candidate,
		const FString& Parent)
	{
		return Candidate.Equals(Parent, ESearchCase::IgnoreCase)
			|| Candidate.StartsWith(Parent + TEXT("/"), ESearchCase::IgnoreCase);
	}

	bool CopyRewardShopStockEvidenceProfile(
		const FString& StorageRoot,
		const TCHAR* Filename)
	{
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				StorageRoot);
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(
			Bytes,
			*Storage.PrimaryPath()))
		{
			return false;
		}
		const FString ProductSmokeRoot =
			FPaths::GetPath(FPaths::GetPath(StorageRoot));
		const FString SnapshotDirectory =
			FPaths::Combine(
				ProductSmokeRoot,
				TEXT("Snapshots"));
		return IFileManager::Get().MakeDirectory(
				*SnapshotDirectory,
				true)
			&& FFileHelper::SaveArrayToFile(
				Bytes,
				*FPaths::Combine(
					SnapshotDirectory,
					Filename));
	}

	bool TryGetP81RangedCompatibilityRoot(FString& OutRoot)
	{
		FString ExplicitRoot;
		if (!FParse::Value(
				FCommandLine::Get(),
				TEXT("P8RangedCompatibilityRoot="),
				ExplicitRoot)
			|| ExplicitRoot.TrimStartAndEnd().IsEmpty())
		{
			return false;
		}
		OutRoot = CanonicalSearchContainerDirectory(ExplicitRoot);
		const FString Parent =
			CanonicalSearchContainerDirectory(FPaths::GetPath(OutRoot));
		const FString Grandparent =
			CanonicalSearchContainerDirectory(FPaths::GetPath(Parent));
		return FPaths::GetCleanFilename(OutRoot).Equals(
				TEXT("Dev.D.UE.0.0.5.P8.1.r0"),
				ESearchCase::IgnoreCase)
			&& FPaths::GetCleanFilename(Parent).Equals(
				TEXT("Automation"),
				ESearchCase::IgnoreCase)
			&& FPaths::GetCleanFilename(Grandparent).Equals(
				TEXT("Saved"),
				ESearchCase::IgnoreCase)
			&& !(IFileManager::Get().DirectoryExists(*OutRoot)
				&& IFileManager::Get().IsSymlink(*OutRoot));
	}

	int32 GetAutomationHealth(const AActor* Actor)
	{
		if (const Ademo_mapTrainingTarget* Target = Cast<Ademo_mapTrainingTarget>(Actor)) return Target->GetHealth();
		if (const Ademo_mapEnemyCharacter* Enemy = Cast<Ademo_mapEnemyCharacter>(Actor)) return Enemy->GetCurrentHealth();
		if (const Ademo_mapRangedEnemyCharacter* Enemy = Cast<Ademo_mapRangedEnemyCharacter>(Actor)) return Enemy->GetCurrentHealth();
		if (const Ademo_mapHeavyEnemyCharacter* Enemy = Cast<Ademo_mapHeavyEnemyCharacter>(Actor)) return Enemy->GetCurrentHealth();
		if (const Ademo_mapFriendlyUnit* Friendly = Cast<Ademo_mapFriendlyUnit>(Actor)) return Friendly->GetCurrentHealth();
		return INDEX_NONE;
	}

	Edemo_mapInputRestoreTracePhase InputRestoreTracePhase(
		const FString& Phase)
	{
		if (Phase.Equals(TEXT("RunStartFresh"), ESearchCase::IgnoreCase))
		{
			return Edemo_mapInputRestoreTracePhase::RunStartFresh;
		}
		if (Phase.Equals(TEXT("RunStartLegacy"), ESearchCase::IgnoreCase))
		{
			return Edemo_mapInputRestoreTracePhase::RunStartLegacy;
		}
		if (Phase.Equals(TEXT("ChestTakeClose"), ESearchCase::IgnoreCase))
		{
			return Edemo_mapInputRestoreTracePhase::ChestTakeClose;
		}
		if (Phase.Equals(TEXT("CorpseTakeClose"), ESearchCase::IgnoreCase))
		{
			return Edemo_mapInputRestoreTracePhase::CorpseTakeClose;
		}
		if (Phase.Equals(TEXT("Reload"), ESearchCase::IgnoreCase))
		{
			return Edemo_mapInputRestoreTracePhase::Reload;
		}
		return Edemo_mapInputRestoreTracePhase::Unknown;
	}

	Edemo_mapInputRestoreTraceBoundary InputRestoreTraceBoundary(
		const TCHAR* Boundary)
	{
		if (FCString::Stricmp(Boundary, TEXT("RunStartPlayable")) == 0)
		{
			return Edemo_mapInputRestoreTraceBoundary::RunStartPlayable;
		}
		if (FCString::Stricmp(Boundary, TEXT("ChestTakeCloseCommitted")) == 0)
		{
			return Edemo_mapInputRestoreTraceBoundary::ChestTakeCloseCommitted;
		}
		if (FCString::Stricmp(Boundary, TEXT("CorpseTakeCloseCommitted")) == 0)
		{
			return Edemo_mapInputRestoreTraceBoundary::CorpseTakeCloseCommitted;
		}
		return Edemo_mapInputRestoreTraceBoundary::None;
	}
}
#endif

namespace
{
	const FName GCodeBNormalContainerDefinitionId(TEXT("CodeB.NormalContainer.BasicCache"));
	const FName GCodeBNormalContainerPrimaryMapTargetId(TEXT("M01.CodeBNormalContainer.BasicCache.01"));
	const FName GCodeBNormalContainerSecondaryMapTargetId(TEXT("M01.CodeBNormalContainer.BasicCache.02"));
	const FName GCodeBNormalContainerAnchorId(TEXT("M01.Resource.TIER_1.Cluster.01"));
	// The Tier-1 reward projection fills an 8x6 grid around the anchor at 240 uu
	// spacing. Keep both production projections outside that grid and preserve
	// P10's original .01 offset exactly; .02 is a separate future source.
	const FVector GCodeBNormalContainerPrimaryAnchorLocalOffset(0.0f, -1600.0f, 0.0f);
	const FVector GCodeBNormalContainerSecondaryAnchorLocalOffset(480.0f, -1600.0f, 0.0f);
	// P11 deliberately binds one map-authored M01 spawn. These values are static
	// content identity, never Actor/ObjectName/UI/runtime-generated identity.
	const FName GCodeBBodyContainerDefinitionId(TEXT("CodeB.BodyContainer.BasicCorpse"));
	const FName GCodeBBodyContainerEncounterId(TEXT("M01.Encounter.LOW.Skirmisher.01"));
	const FName GCodeBBodyContainerSpawnId(TEXT("M01.Spawn.LOW.Skirmisher.01"));
	const FName GCodeBBodyContainerTargetIdentity(TEXT("M01.BodyTarget.LOW.Skirmisher.01.Ordinal.0"));
	constexpr int32 GCodeBBodyContainerSpawnOrdinal = 0;

	FGuid CodeBNormalContainerSearchTargetGuid(const FName MapTargetIdentity)
	{
		const FString Seed = MapTargetIdentity.ToString();
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBNormalTarget.A"))),
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBNormalTarget.B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBNormalTarget.C"))),
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBNormalTarget.D"))));
	}

	bool IsCodeBNormalContainerMapTargetIdentity(const FName MapTargetIdentity)
	{
		return MapTargetIdentity == GCodeBNormalContainerPrimaryMapTargetId
			|| MapTargetIdentity == GCodeBNormalContainerSecondaryMapTargetId;
	}

	FVector CodeBNormalContainerAnchorLocalOffset(const FName MapTargetIdentity)
	{
		return MapTargetIdentity == GCodeBNormalContainerPrimaryMapTargetId
			? GCodeBNormalContainerPrimaryAnchorLocalOffset
			: GCodeBNormalContainerSecondaryAnchorLocalOffset;
	}

	FGuid CodeBBodyContainerTargetGuid(const FName BodyTargetIdentity)
	{
		const FString Seed = BodyTargetIdentity.ToString();
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBBodyTarget.A"))),
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBBodyTarget.B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBBodyTarget.C"))),
			FCrc::StrCrc32(*(Seed + TEXT("|CodeBBodyTarget.D"))));
	}

	FGuid CodeBBodyContainerDeathReceiptGuid(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& BodyTargetId,
		const FName DefinitionId)
	{
		const FString Seed = FString::Printf(TEXT("P11|%s|%s|%s|%s|DeathReceipt"),
			*OwnerId.ToString(EGuidFormats::DigitsWithHyphensLower),
			*RunInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
			*BodyTargetId.ToString(EGuidFormats::DigitsWithHyphensLower),
			*DefinitionId.ToString());
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))),
			FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))),
			FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

}

Ademo_mapV3ProgressionManager::Ademo_mapV3ProgressionManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
}

bool Ademo_mapV3ProgressionManager::CanGenerateRewardSource(
	FGuid RunId,
	FName RewardSourceId) const
{
	bool bDurableReadModelHydrated = true;
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const Udemo_mapProfileSessionSubsystem* Profile =
				GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>())
			{
				Ademo_mapV3ProgressionManager* MutableThis =
					const_cast<Ademo_mapV3ProgressionManager*>(this);
				for (const Fdemo_mapPersistentGeneratedRewardSource& Source :
					Profile->GetActiveGeneratedRewardSources())
				{
					if (Source.Receipt.RunId != RunId)
					{
						continue;
					}
					const bool bReceiptHydrated =
						MutableThis->RewardGenerationSession.IsProcessed(
							RunId, Source.Receipt.StableSourceRoleId)
						|| MutableThis->RewardGenerationSession.Commit(Source.Receipt);
					const bool bPityHydrated =
						MutableThis->CommitRewardAffixPity(RunId, Source.Receipt);
					bDurableReadModelHydrated = bDurableReadModelHydrated
						&& bReceiptHydrated && bPityHydrated;
				}
			}
		}
	}
	return RunId.IsValid()
		&& Items.IsValid()
		&& Items->GetRunState() == Edemo_mapRunState::Active
		&& Items->GetActiveRunId() == RunId
		&& !RewardSourceId.IsNone()
		&& bDurableReadModelHydrated
		&& !RewardGenerationSession.IsProcessed(
			RunId,
			RewardSourceId);
}

bool Ademo_mapV3ProgressionManager::FindDurablyAcceptedRewardSource(
	FGuid RunId,
	FName RewardSourceId,
	Fdemo_mapPersistentGeneratedRewardSource& OutSource) const
{
	OutSource = Fdemo_mapPersistentGeneratedRewardSource();
	if (!RunId.IsValid() || RewardSourceId.IsNone())
	{
		return false;
	}
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const Udemo_mapProfileSessionSubsystem* Profile = GameInstance
		? GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>()
		: nullptr;
	if (!Profile)
	{
		return false;
	}
	// Opening or rebinding an already-accepted source must reconstruct the
	// entire active durable read model before the source is exposed.  A caller
	// can reach this path without first asking CanGenerateRewardSource(), so do
	// not leave the transient receipt/pity ledgers empty after a reload.
	Ademo_mapV3ProgressionManager* MutableThis =
		const_cast<Ademo_mapV3ProgressionManager*>(this);
	for (const Fdemo_mapPersistentGeneratedRewardSource& Source :
		Profile->GetActiveGeneratedRewardSources())
	{
		if (Source.Receipt.RunId != RunId)
		{
			continue;
		}
		const bool bReceiptHydrated = Source.Receipt.IsValid()
			&& (MutableThis->RewardGenerationSession.IsProcessed(
				RunId, Source.Receipt.StableSourceRoleId)
				|| MutableThis->RewardGenerationSession.Commit(Source.Receipt));
		const bool bPityHydrated =
			MutableThis->CommitRewardAffixPity(RunId, Source.Receipt);
		if (!bReceiptHydrated || !bPityHydrated)
		{
			return false;
		}
	}
	for (const Fdemo_mapPersistentGeneratedRewardSource& Source :
		Profile->GetActiveGeneratedRewardSources())
	{
		if (Source.Receipt.RunId == RunId
			&& Source.Receipt.StableSourceRoleId == RewardSourceId
			&& Source.Receipt.IsValid())
		{
			OutSource = Source;
			return true;
		}
	}
	return false;
}

Fdemo_mapProfileGeneratedRewardSourceResult
Ademo_mapV3ProgressionManager::PrepareGeneratedRewardSource(
	FGuid RunId,
	FName RewardSourceId,
	const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt)
{
	Fdemo_mapProfileGeneratedRewardSourceResult Result;
	if (!CanGenerateRewardSource(RunId, RewardSourceId)
		|| Receipt.RunId != RunId
		|| Receipt.StableSourceRoleId != RewardSourceId)
	{
		Result.Diagnostic =
			TEXT("Generated reward source was rejected before its durable candidate commit.");
		return Result;
	}
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	Udemo_mapProfileSessionSubsystem* Profile = GameInstance
		? GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>()
		: nullptr;
	if (!Profile)
	{
		Result.Diagnostic =
			TEXT("Generated reward source has no Profile Session before durable candidate commit.");
		return Result;
	}
	Result =
		Profile->CommitGeneratedRewardSource(Receipt);
	if (!Result.IsDurablyCommitted())
	{
		return Result;
	}
	// This is a transient read model only.  Once the Profile candidate is
	// durable, a hydration miss is an accepted result requiring reconciliation,
	// never a retryable generation failure.
	const bool bReceiptHydrated = RewardGenerationSession.IsProcessed(
		RunId, RewardSourceId)
		|| RewardGenerationSession.Commit(Result.Source.Receipt);
	const bool bPityHydrated = CommitRewardAffixPity(
		RunId, Result.Source.Receipt);
	if (!bReceiptHydrated || !bPityHydrated)
	{
		Result.Status =
			Edemo_mapProfileGeneratedRewardSourceStatus::CommittedReconciliationRequired;
		Result.Diagnostic = TEXT("Generated reward source is durably accepted; transient receipt or pity hydration requires reconciliation without reroll.");
	}
	return Result;
}

bool Ademo_mapV3ProgressionManager::CommitGeneratedRewardSource(
	FGuid RunId,
	FName RewardSourceId,
	const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt)
{
	return PrepareGeneratedRewardSource(
		RunId, RewardSourceId, Receipt).IsDurablyCommitted();
}

const Fdemo_mapRewardSourceAcceptanceReceipt*
Ademo_mapV3ProgressionManager::FindAcceptedRewardSourceReceipt(
	FGuid RunId,
	FName RewardSourceId) const
{
	CanGenerateRewardSource(RunId, RewardSourceId);
	return RewardGenerationSession.FindAcceptedReceipt(
		RunId,
		RewardSourceId);
}

int32 Ademo_mapV3ProgressionManager::GetRewardAffixPityState(
	FGuid RunId) const
{
	return RewardAffixPityLedger.GetState(
		RunId,
		Fdemo_mapRewardAffixPolicyRegistry::PityChannelId);
}

bool Ademo_mapV3ProgressionManager::CommitRewardAffixPity(
	FGuid RunId,
	const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt)
{
	if (!Receipt.IsValid() || Receipt.RunId != RunId)
	{
		return false;
	}
	if (!Receipt.bPityCommitRequired)
	{
		return Receipt.PityStateIn == Receipt.PityStateOut
			&& RewardAffixPityLedger.GetState(
				RunId, Fdemo_mapRewardAffixPolicyRegistry::PityChannelId)
				== Receipt.PityStateIn;
	}
	return RewardAffixPityLedger.Commit(
			RunId,
			Fdemo_mapRewardAffixPolicyRegistry::PityChannelId,
			Receipt.StableSourceRoleId,
			Receipt.PityStateIn,
			Receipt.PityStateOut);
}

#if !UE_BUILD_SHIPPING
void Ademo_mapV3ProgressionManager::ReadNonShippingStartupFlags()
{
	bProfileFlowAutomation = FParse::Param(FCommandLine::Get(), TEXT("ProfileFlowAutomation"));
	bProfileTradeAutomation = FParse::Param(FCommandLine::Get(), TEXT("ProfileTradeAutomation"));
	bP5RealProfileTraceStartup = FParse::Param(FCommandLine::Get(), TEXT("P5RealProfileTraceStartup"));
	bP4xStartRunAutomation = FParse::Param(FCommandLine::Get(), TEXT("P4xStartRunAutomation"));
	bP6ProductStartBridgeR2Trace = FParse::Param(FCommandLine::Get(), TEXT("P6ProductStartBridgeR2Trace"));
	bP6ProductStartBridgeR3Trace = FParse::Param(FCommandLine::Get(), TEXT("P6ProductStartBridgeR3Trace"));
	bP6ProductStartBridgeAutomation = FParse::Param(FCommandLine::Get(), TEXT("P6ProductStartBridge"))
		|| bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace;
	bXFix1SettlementLifecycleAutomation =
		FParse::Param(FCommandLine::Get(), TEXT("XFix1SettlementLifecycleAutomation"));
	bXFix1SettlementRestartAutomation =
		FParse::Param(FCommandLine::Get(), TEXT("XFix1SettlementRestartAutomation"));
	bFullSystemLoopAutomation = FParse::Param(FCommandLine::Get(), TEXT("FullSystemLoopAutomation"));
	bInputRestoreAutomation = FParse::Param(FCommandLine::Get(), TEXT("InputRestoreAutomation"));
	const bool bInputConsumptionMicroTrace =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("InputConsumptionMicroTrace"));
	bInputMovementGateTrace =
		bInputRestoreAutomation
		&& bInputConsumptionMicroTrace
		&& FParse::Param(
			FCommandLine::Get(),
			TEXT("InputMovementGateTrace"));
	if (bInputRestoreAutomation
		&& FParse::Param(FCommandLine::Get(), TEXT("InputRestoreTrace")))
	{
		InputRestoreTrace = MakeUnique<Fdemo_mapInputRestoreTrace>();
		Setdemo_mapInputRestoreTraceRuntimeActive(true);
	}
	if (bInputRestoreAutomation
		&& bInputConsumptionMicroTrace)
	{
		InputConsumptionTrace =
			MakeUnique<Fdemo_mapInputConsumptionTrace>(
				true,
				bInputMovementGateTrace);
	}
	bInputRestoreDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("V3InputRestoreDiagnostics"));
	bWorldAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3WorldInteractionAutomation"));
	bInventoryUIAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3InventoryUIAutomation"));
	bVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3WorldUIVisibleAcceptance"));
	bP5RuntimeVisibleAcceptance = FParse::Param(
		FCommandLine::Get(),
		TEXT("P5RuntimeVisibleAcceptance"));
	bP6DualLootVisibleAcceptance = FParse::Param(
		FCommandLine::Get(),
		TEXT("P6DualLootVisibleAcceptance"));
	bP6DualLootManualFixture = FParse::Param(
		FCommandLine::Get(),
		TEXT("P6DualLootManualFixture"));
	bEnemyLootAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3EnemyLootAutomation"));
	bRunLifecycleAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3RunLifecycleAutomation"));
	bPostSettlementInputRestoreAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3PostSettlementInputRestoreAutomation"));
	bSettlementUIAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3SettlementUIAutomation"));
	bFreshSessionAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3FreshSessionAutomation"));
	bCloseRangeProjectileAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3CloseRangeProjectileAutomation"));
	bLifecycleVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3RunVisibleAcceptance"));
	bRepairVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3RepairVisibleAcceptance"));
	bV3FinalAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3FinalAutomation"));
	bV3FinalVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3FinalVisibleAcceptance"));
	bSearchContainerAutomation = FParse::Param(FCommandLine::Get(), TEXT("SearchContainerAutomation"));
	bRewardGenerationAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardGenerationAutomation"));
	bRewardSourceProjectionAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardSourceProjectionAutomation"));
	bRewardJackpotAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardJackpotAutomation"));
	bRewardRareExtremeAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardRareExtremeAutomation"));
	bRewardAffixPityAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardAffixPityAutomation"));
	bRewardShopStockAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardShopStockAutomation"));
	bRewardBossSourceAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardBossSourceAutomation"));
	bRewardFullMapDistributionAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("RewardFullMapDistributionAutomation"));
	bEnemySkillFrameworkAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("EnemySkillFrameworkAutomation"));
	bEnemyRouteLootAutomation =
		FParse::Param(
			FCommandLine::Get(),
			TEXT("EnemyRouteLootAutomation"));
	FParse::Value(
		FCommandLine::Get(),
		TEXT("SearchContainerStorageRoot="),
		SearchContainerStorageRoot);
	FParse::Value(
		FCommandLine::Get(),
		TEXT("RewardGenerationStorageRoot="),
		RewardGenerationStorageRoot);
	FParse::Value(
		FCommandLine::Get(),
		TEXT("EnemySkillPhase="),
		EnemySkillAutomationPhase);
	FParse::Value(
		FCommandLine::Get(),
		TEXT("EnemySkillStorageRoot="),
		EnemySkillStorageRoot);
	FParse::Value(
		FCommandLine::Get(),
		TEXT("EnemyRouteLootStorageRoot="),
		EnemyRouteLootStorageRoot);
	FParse::Value(FCommandLine::Get(), TEXT("InputRestorePhase="), InputRestorePhase);
	FParse::Value(FCommandLine::Get(), TEXT("InputRestoreStorageRoot="), InputRestoreStorageRoot);
	FParse::Value(FCommandLine::Get(), TEXT("InputRestoreUserConfigRoot="), InputRestoreUserConfigRoot);
	FParse::Value(FCommandLine::Get(), TEXT("InputRestoreAutomationRoot="), InputRestoreAutomationRoot);
	FParse::Value(FCommandLine::Get(), TEXT("P5RealProfileStorageRoot="), P5RealProfileStorageRoot);
	FParse::Value(FCommandLine::Get(), TEXT("P5RealProfileCase="), P5RealProfileCase);
	FParse::Value(FCommandLine::Get(), TEXT("I1ProfileStorageRoot="), I1ProfileStorageRoot);
	FParse::Value(FCommandLine::Get(), TEXT("P6ProductStartBridgeStorageRoot="), P6ProductStartBridgeStorageRoot);
	FParse::Value(FCommandLine::Get(), TEXT("P6ProductStartBridgeCase="), P6ProductStartBridgeCase);
	FParse::Value(FCommandLine::Get(), TEXT("P6ProductStartBridgePhase="), P6ProductStartBridgePhase);
	FParse::Value(FCommandLine::Get(), TEXT("XFix1StorageRoot="), XFix1StorageRoot);
	if (InputRestoreTrace)
	{
		InputRestoreTrace->BeginTransition(
			InputRestoreTracePhase(InputRestorePhase));
	}
	FParse::Value(FCommandLine::Get(), TEXT("V3WorldUIVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V3RunVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V3RepairVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V3FinalVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P5VisibleOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P6VisibleOutput="), VisibleOutputDirectory);
	bLegacyStartupAutomation = IsLegacyAutomationRequested();
}

bool Ademo_mapV3ProgressionManager::IsLegacyAutomationRequested() const
{
	return bWorldAutomation
		|| bInventoryUIAutomation
		|| bVisibleAcceptance
		|| bP5RuntimeVisibleAcceptance
		|| bP6DualLootVisibleAcceptance
		|| bP6DualLootManualFixture
		|| bEnemyLootAutomation
		|| bRunLifecycleAutomation
		|| bPostSettlementInputRestoreAutomation
		|| bSettlementUIAutomation
		|| bFreshSessionAutomation
		|| bCloseRangeProjectileAutomation
		|| bLifecycleVisibleAcceptance
		|| bRepairVisibleAcceptance
		|| bV3FinalAutomation
		|| bV3FinalVisibleAcceptance
		|| bSearchContainerAutomation
		|| bRewardGenerationAutomation
		|| bRewardSourceProjectionAutomation
		|| bRewardJackpotAutomation
		|| bRewardRareExtremeAutomation
		|| bRewardAffixPityAutomation
		|| bRewardShopStockAutomation
		|| bRewardBossSourceAutomation
		|| bRewardFullMapDistributionAutomation
		|| bEnemySkillFrameworkAutomation
		|| bEnemyRouteLootAutomation
		|| FParse::Param(FCommandLine::Get(), TEXT("V3AttributesGameplayAutomation"))
		|| FParse::Param(FCommandLine::Get(), TEXT("V3ItemCoreGameplayAutomation"));
}

bool Ademo_mapV3ProgressionManager::InitializeSearchContainerAutomationProfile()
{
	if (SearchContainerStorageRoot.TrimStartAndEnd().IsEmpty()
		|| !GetGameInstance()
		|| !Items.IsValid())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("SEARCH_CONTAINER_SMOKE: isolated Profile storage root and runtime are required."));
		return false;
	}
	const FString Canonical =
		CanonicalSearchContainerDirectory(SearchContainerStorageRoot);
	const FString Allowed = CanonicalSearchContainerDirectory(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.5.P4.0.r0"),
		TEXT("SearchContainer")));
	const FString Production = CanonicalSearchContainerDirectory(
		Fdemo_mapProfileStorageContext::Production().RootDirectory);
	const bool bExists = IFileManager::Get().DirectoryExists(*Canonical);
	if (Canonical.Equals(Allowed, ESearchCase::IgnoreCase)
		|| !IsSameOrChildSearchContainerPath(Canonical, Allowed)
		|| IsSameOrChildSearchContainerPath(Canonical, Production)
		|| IsSameOrChildSearchContainerPath(Production, Canonical)
		|| (bExists && IFileManager::Get().IsSymlink(*Canonical)))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("SEARCH_CONTAINER_SMOKE: rejected storage root outside the unique P4 boundary: %s"),
			*Canonical);
		return false;
	}
	Udemo_mapProfileSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
	if (!Session)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("SEARCH_CONTAINER_SMOKE: Profile Session subsystem is unavailable."));
		return false;
	}
	const Fdemo_mapProfileSessionInitializeResult Init =
		Session->InitializeSession(
			Fdemo_mapProfileStorageContext::ForRoot(Canonical));
	if (!Init.IsReady())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("SEARCH_CONTAINER_SMOKE: isolated Profile initialization failed: %s"),
			*Init.Diagnostic);
		return false;
	}
	const Fdemo_mapProfileSessionBeginResult Begin = Session->StartPreparedRun();
	if (!Begin.IsRunActive()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| Items->GetActiveRunId() != Begin.Snapshot.ActiveRunId)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("SEARCH_CONTAINER_SMOKE: normal prepared ActiveRun failed: %s"),
			*Begin.Diagnostic);
		return false;
	}
	SearchContainerStorageRoot = Canonical;
	SearchContainerProfileSession = Session;
	return true;
}

bool Ademo_mapV3ProgressionManager::InitializeRewardGenerationAutomationProfile()
{
	FString CommandUserDir;
	FParse::Value(
		FCommandLine::Get(),
		TEXT("UserDir="),
		CommandUserDir);
	if (RewardGenerationStorageRoot.TrimStartAndEnd().IsEmpty()
		|| CommandUserDir.TrimStartAndEnd().IsEmpty()
		|| !GetGameInstance()
		|| !Items.IsValid())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P1_REWARD_GENERATION_PRODUCT_SMOKE: FAIL: isolated Profile/UserDir and runtime are required."));
		return false;
	}
	const FString CanonicalStorage =
		CanonicalSearchContainerDirectory(
			RewardGenerationStorageRoot);
	const FString CanonicalUserDir =
		CanonicalSearchContainerDirectory(CommandUserDir);
	FString RewardAutomationTaskId;
	FParse::Value(
		FCommandLine::Get(),
		TEXT("RewardAutomationTaskId="),
		RewardAutomationTaskId);
	const bool bValidRewardAutomationTaskId =
		((RewardAutomationTaskId.Equals(
				TEXT("Dev.D.UE.0.0.6.F0.0.r2"))
			|| RewardAutomationTaskId.StartsWith(
				TEXT("Dev.D.UE.0.0.6.F0.0.r2."))
			|| RewardAutomationTaskId.Equals(
				TEXT("Dev.D.UE.0.0.6.F0.0.r4"))
			|| RewardAutomationTaskId.StartsWith(
				TEXT("Dev.D.UE.0.0.6.F0.0.r4."))
			|| (bRewardFullMapDistributionAutomation
				&& RewardAutomationTaskId.StartsWith(
					TEXT("Dev.D.UE.0.0.6.P8.")))
			|| (bRewardBossSourceAutomation
				&& RewardAutomationTaskId.StartsWith(
					TEXT("Dev.D.UE.0.0.6.P7.")))
			|| (bRewardShopStockAutomation
				&& RewardAutomationTaskId.StartsWith(
					TEXT("Dev.D.UE.0.0.6.P6."))))
		&& !RewardAutomationTaskId.Contains(TEXT("/"))
		&& !RewardAutomationTaskId.Contains(TEXT("\\"))
		&& !RewardAutomationTaskId.Contains(TEXT(" "))
		&& !RewardAutomationTaskId.Contains(TEXT("..")));
	const FString AutomationTaskId =
		bValidRewardAutomationTaskId
			? RewardAutomationTaskId
			: bRewardFullMapDistributionAutomation
				? TEXT("Dev.D.UE.0.0.6.P8.0.r0")
				: bRewardBossSourceAutomation
				? TEXT("Dev.D.UE.0.0.6.P7.0.r0")
				: bRewardShopStockAutomation
				? TEXT("Dev.D.UE.0.0.6.P6.0.r3")
				: bRewardAffixPityAutomation
					? TEXT("Dev.D.UE.0.0.6.P5.0.r0")
					: bRewardRareExtremeAutomation
						? TEXT("Dev.D.UE.0.0.6.P4.0.r0")
						: bRewardJackpotAutomation
							? TEXT("Dev.D.UE.0.0.6.P3.0.r0")
							: bRewardSourceProjectionAutomation
								? TEXT("Dev.D.UE.0.0.6.P2.0.r0")
								: TEXT("Dev.D.UE.0.0.6.P1.0.r0");
	const FString Allowed =
		CanonicalSearchContainerDirectory(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Saved"),
			TEXT("Automation"),
			AutomationTaskId,
			TEXT("ProductSmoke")));
	const FString Production =
		CanonicalSearchContainerDirectory(
			Fdemo_mapProfileStorageContext::Production().RootDirectory);
	const bool bStorageExists =
		IFileManager::Get().DirectoryExists(*CanonicalStorage);
	const bool bUserDirExists =
		IFileManager::Get().DirectoryExists(*CanonicalUserDir);
	if (CanonicalStorage.Equals(Allowed, ESearchCase::IgnoreCase)
		|| CanonicalUserDir.Equals(Allowed, ESearchCase::IgnoreCase)
		|| !IsSameOrChildSearchContainerPath(CanonicalStorage, Allowed)
		|| !IsSameOrChildSearchContainerPath(CanonicalUserDir, Allowed)
		|| IsSameOrChildSearchContainerPath(
			CanonicalStorage,
			Production)
		|| IsSameOrChildSearchContainerPath(
			Production,
			CanonicalStorage)
		|| (bStorageExists
			&& IFileManager::Get().IsSymlink(*CanonicalStorage))
		|| (bUserDirExists
			&& IFileManager::Get().IsSymlink(*CanonicalUserDir)))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P1_REWARD_GENERATION_PRODUCT_SMOKE: FAIL: isolated boundary rejected profile=%s userdir=%s."),
			*CanonicalStorage,
			*CanonicalUserDir);
		return false;
	}
	Udemo_mapProfileSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<
			Udemo_mapProfileSessionSubsystem>();
	if (!Session)
	{
		return false;
	}
	const Fdemo_mapProfileSessionInitializeResult Init =
		Session->InitializeSession(
			Fdemo_mapProfileStorageContext::ForRoot(
				CanonicalStorage));
	if (!Init.IsReady())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P1_REWARD_GENERATION_PRODUCT_SMOKE: FAIL: isolated Profile initialization failed: %s"),
			*Init.Diagnostic);
		return false;
	}
	if (bRewardShopStockAutomation)
	{
		const Fdemo_mapProfileSessionSnapshot Initial =
			Session->GetSnapshot();
		FString StockDiagnostic;
		const bool bExactShape =
			Initial.SessionState
					== Edemo_mapProfileSessionState::ReadyForPreparation
			&& Initial.ShopStock.bInitialized
			&& Initial.ShopStock.PolicyId
					== Fdemo_mapRewardShopStock::DefaultPolicyId
			&& Initial.ShopStock.Generation == 0
			&& Initial.ShopStock.Entries.Num() == 12
			&& Fdemo_mapRewardShopStock::ValidateState(
				Initial.ProfileId,
				Initial.ShopStock,
				&StockDiagnostic);
		int32 WeaponCount = 0;
		int32 RobeCount = 0;
		int32 LowMidPillCount = 0;
		int32 MidHighPillCount = 0;
		int32 BasicPillCount = 0;
		for (const Fdemo_mapPersistentShopStockEntry& Entry :
			Initial.ShopStock.Entries)
		{
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(
					Entry.Item.ItemDefinitionId);
			if (!Definition
				|| Entry.State
					!= Edemo_mapPersistentShopStockEntryState::Available
				|| Entry.Item.RewardEventKind
					!= Edemo_mapRewardEventKind::None
				|| Entry.Item.RewardEventId.IsValid()
				|| Entry.Item.RareRewardEventId.IsValid()
				|| !Entry.Item.RareRewardPolicyId.IsNone()
				|| !Entry.Item.RareRewardTierId.IsNone()
				|| Entry.Item.RareRewardBonusValue != 0)
			{
				continue;
			}
			WeaponCount += Definition->CategoryId
					== Fdemo_mapItemIds::WeaponCategory
				? 1 : 0;
			RobeCount += Definition->CategoryId
					== Fdemo_mapItemIds::ArmorCategory
				? 1 : 0;
			LowMidPillCount += Entry.SlotId.ToString().
					StartsWith(TEXT("Shop.Stock.Pill.LowMid."))
				? 1 : 0;
			MidHighPillCount += Entry.SlotId
					== FName(TEXT(
						"Shop.Stock.Pill.MidHigh.01"))
				? 1 : 0;
			BasicPillCount += Entry.Item.ItemDefinitionId
						== Fdemo_mapItemIds::HealingPillLevel1
					&& Entry.QuotedBuyValue == 30
				? 1 : 0;
		}
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				CanonicalStorage);
		const Fdemo_mapProfileLoadResult Reload =
			Repository.LoadExistingProfile(Storage);
		const Fdemo_mapPersistentShopStockState BeforeReopen =
			Session->GetSnapshot().ShopStock;
		const Fdemo_mapPersistentShopStockState AfterReopen =
			Session->GetSnapshot().ShopStock;
		if (!bExactShape
			|| WeaponCount != 3
			|| RobeCount != 3
			|| LowMidPillCount != 5
			|| MidHighPillCount != 1
			|| BasicPillCount < 1
			|| !Reload.IsSuccess()
			|| Reload.Profile.ProfileId != Initial.ProfileId
			|| Reload.Profile.ShopStock != Initial.ShopStock
			|| BeforeReopen != AfterReopen)
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P6_REWARD_SHOP_STOCK_PRODUCT_SMOKE: FAIL: StageA initial/reopen/reload stock contract rejected: %s"),
				*StockDiagnostic);
			return false;
		}
		RewardShopStockGeneration0 = Initial.ShopStock;
		if (!CopyRewardShopStockEvidenceProfile(
			CanonicalStorage,
			TEXT("ShopStockGeneration0.json")))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P6_REWARD_SHOP_STOCK_PRODUCT_SMOKE: FAIL: StageA Generation 0 evidence snapshot could not be written."));
			return false;
		}
	}
	const Fdemo_mapProfileSessionBeginResult Begin =
		Session->StartPreparedRun();
	if (!Begin.IsRunActive()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| Items->GetActiveRunId() != Begin.Snapshot.ActiveRunId)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P1_REWARD_GENERATION_PRODUCT_SMOKE: FAIL: prepared ActiveRun failed: %s"),
			*Begin.Diagnostic);
		return false;
	}
	if (bRewardShopStockAutomation
		&& (Begin.Snapshot.ShopStock
				!= RewardShopStockGeneration0
			|| Begin.Snapshot.ShopStock.Generation != 0))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P6_REWARD_SHOP_STOCK_PRODUCT_SMOKE: FAIL: StageA BeginRun rerolled ShopStock."));
		return false;
	}
	RewardGenerationStorageRoot = CanonicalStorage;
	RewardGenerationProfileSession = Session;
	if (bRewardShopStockAutomation)
	{
		RewardShopStockInitialRunId =
			Begin.Snapshot.ActiveRunId;
	}
	if (bRewardSourceProjectionAutomation)
	{
		EnemyRouteLootStorageRoot = CanonicalStorage;
		EnemyRouteLootProfileSession = Session;
	}
	return true;
}

void Ademo_mapV3ProgressionManager::RecordInputRestoreTraceEvent(
	Edemo_mapInputRestoreTraceEvent Event,
	float DistanceUU,
	double LatencySeconds,
	Edemo_mapInputRestoreTraceCaller Caller)
{
	if (!InputRestoreTrace)
	{
		return;
	}
	Fdemo_mapInputRestoreTraceSnapshot Snapshot;
	const Ademo_mapPlayerController* Controller = GetDemoController();
	const ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get());
	const UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr;
	Snapshot.RealSeconds = FPlatformTime::Seconds();
	Snapshot.WorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	Snapshot.LatencySeconds = LatencySeconds;
	Snapshot.DistanceUU = DistanceUU;
	Snapshot.VelocityUUPerSecond =
		PlayerPawn.IsValid() ? PlayerPawn->GetVelocity().Size2D() : -1.0f;
	if (PlayerPawn.IsValid())
	{
		const FVector Location = PlayerPawn->GetActorLocation();
		Snapshot.LocationX = Location.X;
		Snapshot.LocationY = Location.Y;
	}
	Snapshot.MovementMode =
		Movement ? static_cast<int32>(Movement->MovementMode) : INDEX_NONE;
	Snapshot.ControllerIdentity =
		reinterpret_cast<uint64>(Controller);
	Snapshot.PawnIdentity =
		reinterpret_cast<uint64>(PlayerPawn.Get());
	Snapshot.RunId = Items.IsValid() ? Items->GetActiveRunId() : FGuid();
	const Ademo_mapSearchContainerActor* Container =
		ActiveSearchContainer.IsValid()
			? ActiveSearchContainer.Get()
			: InputRestoreContainer.Get();
	Snapshot.ContainerId = Container ? Container->GetContainerId() : FGuid();
	Snapshot.bSearchOpen = bSearchContainerOpen;
	Snapshot.bWidgetValid = SearchContainerWidget != nullptr;
	Snapshot.bWidgetInViewport =
		SearchContainerWidget && SearchContainerWidget->IsInViewport();
	Snapshot.bWidgetFocus =
		SearchContainerWidget && SearchContainerWidget->HasKeyboardFocus();
	Snapshot.bSlateKeyboardFocus =
		FSlateApplication::IsInitialized()
			&& FSlateApplication::Get().GetKeyboardFocusedWidget().IsValid();
	if (Controller)
	{
		Snapshot.bSearchLock = Controller->IsSearchContainerInputLocked();
		Snapshot.bPreparationLock = Controller->IsProfilePreparationInputLocked();
		Snapshot.bSettlementLock =
			Controller->IsSettlementInputLockedForAutomation();
		Snapshot.bOwnedIgnore =
			Controller->IsOwnedInputIgnoreAppliedForAutomation();
		Snapshot.bMoveIgnored = Controller->IsMoveInputIgnored();
		Snapshot.bLookIgnored = Controller->IsLookInputIgnored();
		Snapshot.bGameplayAllowed = Controller->IsGameplayInputAllowed();
		Snapshot.bMoveForwardPressed =
			Controller->IsMoveForwardPressedForAutomation();
		Snapshot.bPlayerInputPresent =
			Controller->HasPlayerInputForAutomation();
		Snapshot.bInputComponentPresent =
			Controller->HasInputComponentForAutomation();
		Snapshot.bShowMouseCursor = Controller->bShowMouseCursor;
		Snapshot.bPossessed = Controller->GetPawn() == PlayerPawn.Get();
		Snapshot.InputSurface =
			Controller->GetInputSurfaceState() == TEXT("Gameplay")
				? 0
				: Controller->GetInputSurfaceState() == TEXT("SearchContainer")
					? 1
					: Controller->GetInputSurfaceState() == TEXT("ProfilePreparation")
						? 2
						: Controller->GetInputSurfaceState() == TEXT("Settlement")
							? 3
							: INDEX_NONE;
		Snapshot.InputMode =
			Controller->GetInputModeState() == TEXT("GameOnly")
				? 0
				: Controller->GetInputModeState() == TEXT("GameAndUI")
					? 1
					: Controller->GetInputModeState() == TEXT("UIOnly")
						? 2
						: INDEX_NONE;
	}
	InputRestoreTrace->Record(Event, Snapshot, Caller);
}

void Ademo_mapV3ProgressionManager::SetInputRestoreTraceBoundary(
	const TCHAR* Boundary)
{
	if (InputRestoreTrace)
	{
		InputRestoreTrace->SetBoundary(InputRestoreTraceBoundary(Boundary));
	}
}

void Ademo_mapV3ProgressionManager::ActivateInputConsumptionTrace()
{
	if (!InputConsumptionTrace || !PlayerPawn.IsValid())
	{
		return;
	}
	if (Ademo_mapPlayerController* Controller = GetDemoController())
	{
		Controller->RefreshInputConsumptionMovementHookForGate(
			Cast<ACharacter>(PlayerPawn.Get()));
	}
	// Use the manager's existing tick as the direct PostMovement hook, ordered
	// after the product CharacterMovement tick through an explicit prerequisite.
	// This creates no new tick or component.
	if (ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get()))
	{
		if (UCharacterMovementComponent* Movement =
			Character->GetCharacterMovement())
		{
			PrimaryActorTick.AddPrerequisite(
				Movement,
				Movement->PrimaryComponentTick);
			InputConsumptionObservedMovement = Movement;
		}
	}
	if (!InputConsumptionPostActorTickHandle.IsValid())
	{
		InputConsumptionPostActorTickHandle =
			FWorldDelegates::OnWorldPostActorTick.AddUObject(
			this,
			&Ademo_mapV3ProgressionManager::
					HandleInputConsumptionPostActorTick);
	}
	InputConsumptionTrace->BeginBoundary(
		PlayerPawn.Get(),
		InputRestoreContainer.Get());
	Setdemo_mapInputConsumptionTraceRuntime(InputConsumptionTrace.Get());
}

void Ademo_mapV3ProgressionManager::DeactivateInputConsumptionTrace()
{
	if (UCharacterMovementComponent* Movement =
		InputConsumptionObservedMovement.Get())
	{
		PrimaryActorTick.RemovePrerequisite(
			Movement,
			Movement->PrimaryComponentTick);
	}
	InputConsumptionObservedMovement.Reset();
	if (InputConsumptionPostActorTickHandle.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(
			InputConsumptionPostActorTickHandle);
		InputConsumptionPostActorTickHandle.Reset();
	}
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
}

void Ademo_mapV3ProgressionManager::
	HandleInputConsumptionPostActorTick(
		UWorld* World,
		ELevelTick TickType,
		float DeltaSeconds)
{
	(void)TickType;
	(void)DeltaSeconds;
	if (World == GetWorld())
	{
		// UWorld broadcasts this only after TG_PostPhysics has completed.
		Recorddemo_mapInputConsumptionTraceStage(
			Edemo_mapInputConsumptionStage::PostPhysics,
			PlayerPawn.Get(),
			GetDemoController());
	}
}

bool Ademo_mapV3ProgressionManager::InitializeEnemySkillAutomationProfile()
{
	const bool bMelee =
		EnemySkillAutomationPhase.Equals(
			TEXT("Melee"),
			ESearchCase::IgnoreCase);
	const bool bRanged =
		EnemySkillAutomationPhase.Equals(
			TEXT("Ranged"),
			ESearchCase::IgnoreCase);
	if ((!bMelee && !bRanged)
		|| EnemySkillStorageRoot.TrimStartAndEnd().IsEmpty()
		|| !GetGameInstance()
		|| !Items.IsValid())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_SKILL_SMOKE: phase, isolated storage root, and runtime are required."));
		return false;
	}
	const FString Canonical =
		CanonicalSearchContainerDirectory(EnemySkillStorageRoot);
	const FString AllowedParent =
		CanonicalSearchContainerDirectory(FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.5.P6.0.r0"),
			TEXT("Smoke")));
	const FString Expected =
		CanonicalSearchContainerDirectory(FPaths::Combine(
			AllowedParent,
			bMelee ? TEXT("Melee") : TEXT("Ranged")));
	FString P81Root;
	const bool bP81Boundary =
		TryGetP81RangedCompatibilityRoot(P81Root);
	const FString P81Expected =
		CanonicalSearchContainerDirectory(FPaths::Combine(
			P81Root,
			TEXT("ProductSmoke"),
			TEXT("P6Ranged")));
	const FString Production = CanonicalSearchContainerDirectory(
		Fdemo_mapProfileStorageContext::Production().RootDirectory);
	const bool bExists = IFileManager::Get().DirectoryExists(*Canonical);
	const bool bLegacyP6Boundary =
		Canonical.Equals(Expected, ESearchCase::IgnoreCase)
		&& IsSameOrChildSearchContainerPath(Canonical, AllowedParent);
	const bool bTaskP81Boundary =
		bP81Boundary
		&& Canonical.Equals(P81Expected, ESearchCase::IgnoreCase)
		&& IsSameOrChildSearchContainerPath(Canonical, P81Root);
	if (!(bLegacyP6Boundary || bTaskP81Boundary)
		|| IsSameOrChildSearchContainerPath(Canonical, Production)
		|| IsSameOrChildSearchContainerPath(Production, Canonical)
		|| (bExists && IFileManager::Get().IsSymlink(*Canonical)))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_SKILL_SMOKE: rejected storage root: %s"),
			*Canonical);
		return false;
	}
	Udemo_mapProfileSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
	if (!Session)
	{
		return false;
	}
	const Fdemo_mapProfileSessionInitializeResult Init =
		Session->InitializeSession(
			Fdemo_mapProfileStorageContext::ForRoot(Canonical));
	if (!Init.IsReady())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_SKILL_SMOKE: isolated Profile initialization failed: %s"),
			*Init.Diagnostic);
		return false;
	}
	const Fdemo_mapProfileSessionBeginResult Begin =
		Session->StartPreparedRun();
	if (!Begin.IsRunActive()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| Items->GetActiveRunId() != Begin.Snapshot.ActiveRunId)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_SKILL_SMOKE: normal prepared ActiveRun failed: %s"),
			*Begin.Diagnostic);
		return false;
	}
	EnemySkillStorageRoot = Canonical;
	EnemySkillProfileSession = Session;
	return true;
}

bool Ademo_mapV3ProgressionManager::InitializeEnemyRouteLootAutomationProfile()
{
	if (EnemyRouteLootStorageRoot.TrimStartAndEnd().IsEmpty()
		|| !GetGameInstance()
		|| !Items.IsValid())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_ROUTE_LOOT_SMOKE: FAIL — isolated storage root and Runtime are required."));
		return false;
	}
	const FString Canonical =
		CanonicalSearchContainerDirectory(EnemyRouteLootStorageRoot);
	const FString Expected =
		CanonicalSearchContainerDirectory(FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.5.P7.0.r0"),
			TEXT("Smoke"),
			TEXT("EnemyRouteLoot.Final5")));
	FString P81Root;
	const bool bP81Boundary =
		TryGetP81RangedCompatibilityRoot(P81Root);
	const FString P81Expected =
		CanonicalSearchContainerDirectory(FPaths::Combine(
			P81Root,
			TEXT("ProductSmoke"),
			TEXT("P7EnemyRouteLoot")));
	const FString Production = CanonicalSearchContainerDirectory(
		Fdemo_mapProfileStorageContext::Production().RootDirectory);
	const bool bExists = IFileManager::Get().DirectoryExists(*Canonical);
	if (!(Canonical.Equals(Expected, ESearchCase::IgnoreCase)
			|| (bP81Boundary
				&& Canonical.Equals(P81Expected, ESearchCase::IgnoreCase)
				&& IsSameOrChildSearchContainerPath(Canonical, P81Root)))
		|| IsSameOrChildSearchContainerPath(Canonical, Production)
		|| IsSameOrChildSearchContainerPath(Production, Canonical)
		|| (bExists && IFileManager::Get().IsSymlink(*Canonical)))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_ROUTE_LOOT_SMOKE: FAIL — rejected storage root: %s"),
			*Canonical);
		return false;
	}
	Udemo_mapProfileSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
	if (!Session)
	{
		return false;
	}
	const Fdemo_mapProfileSessionInitializeResult Init =
		Session->InitializeSession(
			Fdemo_mapProfileStorageContext::ForRoot(Canonical));
	if (!Init.IsReady())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_ROUTE_LOOT_SMOKE: FAIL — isolated Profile initialization failed: %s"),
			*Init.Diagnostic);
		return false;
	}
	const Fdemo_mapProfileSessionBeginResult Begin =
		Session->StartPreparedRun();
	if (!Begin.IsRunActive()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| Items->GetActiveRunId() != Begin.Snapshot.ActiveRunId)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_ROUTE_LOOT_SMOKE: FAIL — normal prepared ActiveRun failed: %s"),
			*Begin.Diagnostic);
		return false;
	}
	EnemyRouteLootStorageRoot = Canonical;
	EnemyRouteLootProfileSession = Session;
	EnemyRouteLootPersistentBalanceBefore =
		Begin.Snapshot.PersistentSpiritStones;
	EnemyRouteLootRiskBalanceBefore = Begin.Snapshot.RiskSpiritStones;
	return true;
}
#endif

bool Ademo_mapV3ProgressionManager::Initialize(
	APawn* InPlayerPawn,
	Udemo_mapItemSubsystem* InItems,
	const bool bInUse0909BFrameworkHost)
{
	if (bInitialized)
	{
		return PlayerPawn.Get() == InPlayerPawn
			&& Items.Get() == InItems
			&& bUse0909BFrameworkHost == bInUse0909BFrameworkHost;
	}
	if (!InPlayerPawn || !InItems || !GetWorld()) return false;
	bUse0909BFrameworkHost = bInUse0909BFrameworkHost;
	PlayerPawn = InPlayerPawn;
	Items = InItems;
	if (Udemo_mapPlayerHealthComponent* Health =
		InPlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>())
	{
		Health->OnPlayerDamaged.AddUniqueDynamic(
			this,
			&Ademo_mapV3ProgressionManager::HandlePlayerDamaged);
	}

#if !UE_BUILD_SHIPPING
	ReadNonShippingStartupFlags();
#endif
	Fdemo_mapProfileStartupInputs StartupInputs;
	StartupInputs.bIsV3World = true;
#if !UE_BUILD_SHIPPING
	StartupInputs.bLegacyAutomationRequested = bLegacyStartupAutomation;
	StartupInputs.bProfileAutomationRequested =
		bProfileFlowAutomation
		|| bProfileTradeAutomation
		|| bP5RealProfileTraceStartup
		|| bP4xStartRunAutomation
		|| bP6ProductStartBridgeAutomation
		|| bXFix1SettlementLifecycleAutomation
		|| bXFix1SettlementRestartAutomation
		|| bFullSystemLoopAutomation
		|| bInputRestoreAutomation;
#endif
	ProfileStartupMode = Fdemo_mapProfileStartupModeSelector::Select(StartupInputs);
	if (Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode))
	{
		ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
#if !UE_BUILD_SHIPPING
		if (ProfileStartupMode == Edemo_mapProfileStartupMode::ProfileAutomation)
		{
			if (!InitializeExplicitProfileFlow()) return false;
		}
		else
#endif
		{
			const Fdemo_mapProfileSessionInitializeResult Init =
#if !UE_BUILD_SHIPPING
				!I1ProfileStorageRoot.TrimStartAndEnd().IsEmpty()
					? ProfilePreparationFlow->InitializeExplicit(
						GetGameInstance(), I1ProfileStorageRoot)
					: ProfilePreparationFlow->InitializeProduction(GetGameInstance());
#else
				ProfilePreparationFlow->InitializeProduction(GetGameInstance());
#endif
			UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP: production initialization status=%d diagnostic=%s."), static_cast<int32>(Init.Status), *Init.Diagnostic);
			if (Init.IsReady()
				&& (Init.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted
					|| Init.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted)
				&& Init.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon
				&& ProfilePreparationFlow->GetRecoveredAbandonRunId().IsValid())
			{
				// Code A's restart recovery has already committed its own terminal
				// record. P8 may only observe the stable identity now; any Code B
				// refusal remains audit-only and cannot alter startup semantics.
				ObserveCodeBRunTerminalAfterCodeACommit(
					Init.Snapshot.ProfileId,
					ProfilePreparationFlow->GetRecoveredAbandonRunId(),
					ECodeBRunInventoryTerminalState::RecoveredAbandon);
			}
		}
		bInitialized = true;
#if !UE_BUILD_SHIPPING
		if (ProfileStartupMode == Edemo_mapProfileStartupMode::ProfileAutomation
			&& !bP4xStartRunAutomation
			&& !bP6ProductStartBridgeAutomation
			&& !bP5RealProfileTraceStartup)
		{
			// Existing product-path automation owns the original Preparation widget.
			ShowProfilePreparation();
		}
		else
#endif
		{
			if (!bUse0909BFrameworkHost)
			{
				ShowSectNavigation();
			}
		}
#if !UE_BUILD_SHIPPING
		if (ProfileStartupMode == Edemo_mapProfileStartupMode::ProfileAutomation
			&& (ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Extract
				|| ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Death
				|| ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Crash))
		{
			const Fdemo_mapProfilePreparationSnapshot Preparation = ProfilePreparationFlow->GetSession()->GetPreparationSnapshot();
			for (const Fdemo_mapProfilePreparationStashRow& Row : Preparation.OrderedPermanentStashRows)
			{
				if (Row.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade)
				{
					ProfileFlowItemId = Row.ItemInstanceId;
					break;
				}
			}
			if (!ProfileFlowItemId.IsValid()
				|| !ProfilePreparationWidget
				|| !ProfilePreparationWidget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, ProfileFlowItemId).IsAccepted()
				|| !StartPreparedProfileRun().IsRunActive())
			{
				UE_LOG(Logdemo_map, Error, TEXT("PROFILE_PREPARATION_V3_FLOW: automated Preparation Start failed."));
				return false;
			}
		}
		if (bFullSystemLoopAutomation
			&& (FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Loop
				|| FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Death
				|| FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Crash)
			&& !PrepareFullSystemAutomationRun())
		{
			UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: Preparation UI could not start the requested product run."));
			return false;
		}
#endif
		StartRequestedAutomation();
		return true;
	}

	Items->BeginWorld(GetWorld());
	#if !UE_BUILD_SHIPPING
	if (bEnemyRouteLootAutomation)
	{
		if (!InitializeEnemyRouteLootAutomationProfile())
		{
			Items->TeardownWorld(GetWorld());
			return false;
		}
	}
		else if (bRewardGenerationAutomation
			|| bRewardSourceProjectionAutomation
			|| bRewardJackpotAutomation
			|| bRewardRareExtremeAutomation
			|| bRewardAffixPityAutomation
			|| bRewardShopStockAutomation
			|| bRewardBossSourceAutomation
			|| bRewardFullMapDistributionAutomation)
	{
		if (!InitializeRewardGenerationAutomationProfile())
		{
			Items->TeardownWorld(GetWorld());
			return false;
		}
	}
	else if (bEnemySkillFrameworkAutomation)
	{
		if (!InitializeEnemySkillAutomationProfile())
		{
			Items->TeardownWorld(GetWorld());
			return false;
		}
	}
	else if (bSearchContainerAutomation)
	{
		if (!InitializeSearchContainerAutomationProfile())
		{
			Items->TeardownWorld(GetWorld());
			return false;
		}
	}
	else
	#endif
	{
		const Fdemo_mapItemOperationResult BeginRunResult = Items->BeginRun();
		if (!BeginRunResult.bSuccess)
		{
			UE_LOG(Logdemo_map, Error, TEXT("0.3.4.0: BeginRun failed code=%d diagnostic=%s"), static_cast<int32>(BeginRunResult.Code), *BeginRunResult.Diagnostic);
			Items->TeardownWorld(GetWorld());
			return false;
		}
	}
	Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode());
	if (!Mode || !Mode->ActivateV3MissionContentForRun() || !InitializeWorldContent())
	{
#if !UE_BUILD_SHIPPING
		if (bProfileFlowAutomation && ProfilePreparationFlow)
		{
			const Fdemo_mapProfileSessionSettlementResult RollbackResult = ProfilePreparationFlow->CancelActiveRunForActivationFailure();
			UE_LOG(Logdemo_map, Error, TEXT("PROFILE_PREPARATION_V3_FLOW: world activation failed; activation_rollback_status=%d diagnostic=%s"), static_cast<int32>(RollbackResult.Status), *RollbackResult.Diagnostic);
		}
		else
#endif
		{
			Fdemo_mapSettlementSummary CleanupSummary;
			Items->RequestSettlement(Edemo_mapRunEndReason::Abandon, CleanupSummary);
		}
		if (Mode) Mode->DeactivateV3MissionContentForPreparation();
		Items->TeardownWorld(GetWorld());
		return false;
	}
	bProfileWorldActive = false;
#if !UE_BUILD_SHIPPING
	bProfileFlowAutomation = false;
#endif
	bInitialized = true;
#if !UE_BUILD_SHIPPING
	bInputRestoreDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("V3InputRestoreDiagnostics"));
	bWorldAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3WorldInteractionAutomation"));
	bInventoryUIAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3InventoryUIAutomation"));
	bVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3WorldUIVisibleAcceptance"));
	bP5RuntimeVisibleAcceptance = FParse::Param(
		FCommandLine::Get(),
		TEXT("P5RuntimeVisibleAcceptance"));
	bP6DualLootVisibleAcceptance = FParse::Param(
		FCommandLine::Get(),
		TEXT("P6DualLootVisibleAcceptance"));
	bP6DualLootManualFixture = FParse::Param(
		FCommandLine::Get(),
		TEXT("P6DualLootManualFixture"));
	bEnemyLootAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3EnemyLootAutomation"));
	bRunLifecycleAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3RunLifecycleAutomation"));
	bPostSettlementInputRestoreAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3PostSettlementInputRestoreAutomation"));
	bSettlementUIAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3SettlementUIAutomation"));
	bFreshSessionAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3FreshSessionAutomation"));
	bCloseRangeProjectileAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3CloseRangeProjectileAutomation"));
	bLifecycleVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3RunVisibleAcceptance"));
	bRepairVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3RepairVisibleAcceptance"));
	bV3FinalAutomation = FParse::Param(FCommandLine::Get(), TEXT("V3FinalAutomation"));
	bV3FinalVisibleAcceptance = FParse::Param(FCommandLine::Get(), TEXT("V3FinalVisibleAcceptance"));
	FParse::Value(FCommandLine::Get(), TEXT("V3WorldUIVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V3RunVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V3RepairVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("V3FinalVisualOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P5VisibleOutput="), VisibleOutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P6VisibleOutput="), VisibleOutputDirectory);
#endif
	if (Ademo_mapPlayerController* Controller = GetDemoController(); Controller == nullptr || !Controller->RestoreGameplayControlForNewRun())
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.3.4.1 INPUT_RESTORE_FAIL: V3 initialization could not restore the gameplay control surface."));
		return false;
	}
	LogInputRestoreDiagnostics(TEXT("A.NewRunInitialized"));
	UE_LOG(Logdemo_map, Log, TEXT("V3_WORLD_INTERACTION: root-gated manager initialized; chests=%d world_items=%d."), Chests.Num(), InitialWorldItems.Num());
	StartRequestedAutomation();
	return true;
}

#if !UE_BUILD_SHIPPING
bool Ademo_mapV3ProgressionManager::InitializeInputRestoreAutomation()
{
	const bool bValidPhase =
		InputRestorePhase.Equals(TEXT("RunStartFresh"), ESearchCase::IgnoreCase)
		|| InputRestorePhase.Equals(TEXT("RunStartLegacy"), ESearchCase::IgnoreCase)
		|| InputRestorePhase.Equals(TEXT("ChestTakeClose"), ESearchCase::IgnoreCase)
		|| InputRestorePhase.Equals(TEXT("CorpseTakeClose"), ESearchCase::IgnoreCase)
		|| InputRestorePhase.Equals(TEXT("Reload"), ESearchCase::IgnoreCase);
	if (!bValidPhase
		|| InputRestoreStorageRoot.TrimStartAndEnd().IsEmpty()
		|| InputRestoreUserConfigRoot.TrimStartAndEnd().IsEmpty()
		|| InputRestoreAutomationRoot.TrimStartAndEnd().IsEmpty())
	{
		UE_LOG(Logdemo_map, Error, TEXT("INPUT_RESTORE_PROBE: FAIL required phase and isolated roots are missing."));
		return false;
	}

	FString CommandUserDir;
	FParse::Value(FCommandLine::Get(), TEXT("UserDir="), CommandUserDir);
	const FString GeneratedConfig =
		FPaths::GeneratedConfigDir();
	const Fdemo_mapAutomationRootBoundaryResult Boundary =
		Fdemo_mapProfilePreparationFlow::ValidateExplicitAutomationBoundary(
			InputRestoreAutomationRoot,
			InputRestoreStorageRoot,
			InputRestoreUserConfigRoot,
			CommandUserDir,
			GeneratedConfig,
			{
				InputRestoreAutomationRoot,
				InputRestoreStorageRoot,
				InputRestoreUserConfigRoot,
				GeneratedConfig
			});
	if (!Boundary.bAccepted)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("INPUT_RESTORE_PROBE: FAIL AUTOMATION_ROOT_BOUNDARY %s."),
			*Boundary.Diagnostic());
		return false;
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_RESTORE_PROBE: AUTOMATION_ROOT_BOUNDARY %s."),
		*Boundary.Diagnostic());

	InputRestoreAutomationRoot = Boundary.CanonicalRoot;
	InputRestoreStorageRoot = Boundary.CanonicalStorageRoot;
	InputRestoreUserConfigRoot = Boundary.CanonicalUserConfigRoot;
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::AutomationInitialized);
	ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
	const Fdemo_mapProfileSessionInitializeResult Init =
		ProfilePreparationFlow->InitializeExplicit(
			GetGameInstance(),
			InputRestoreStorageRoot);
	ProfileFlowInitializeStatus = Init.Status;
	if (!Init.IsReady())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("INPUT_RESTORE_PROBE: FAIL isolated session initialization status=%d diagnostic=%s."),
			static_cast<int32>(Init.Status),
			*Init.Diagnostic);
		ProfilePreparationFlow->Unbind();
		ProfilePreparationFlow.Reset();
		return false;
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_RESTORE_PROBE initialized phase=%s storage=%s user=%s init_status=%d."),
		*InputRestorePhase,
		*InputRestoreStorageRoot,
		*InputRestoreUserConfigRoot,
		static_cast<int32>(Init.Status));
	return true;
}

bool Ademo_mapV3ProgressionManager::InitializeP5RealProfileTraceProfile()
{
	if (P5RealProfileStorageRoot.TrimStartAndEnd().IsEmpty()
		|| P5RealProfileCase.TrimStartAndEnd().IsEmpty())
	{
		UE_LOG(Logdemo_map, Error, TEXT("P5_REAL_PROFILE_STARTUP: -P5RealProfileStorageRoot and -P5RealProfileCase are required."));
		return false;
	}

	FString CanonicalRoot;
	FString BoundaryDiagnostic;
	if (!Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(
			P5RealProfileStorageRoot, CanonicalRoot, BoundaryDiagnostic))
	{
		UE_LOG(Logdemo_map, Error, TEXT("P5_REAL_PROFILE_STARTUP: rejected isolated storage root: %s"), *BoundaryDiagnostic);
		return false;
	}
	P5RealProfileStorageRoot = CanonicalRoot;
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(P5RealProfileStorageRoot);
	const bool bHasExistingProfile = IFileManager::Get().FileExists(*Storage.PrimaryPath())
		|| IFileManager::Get().FileExists(*Storage.BackupPath());
	if (!bHasExistingProfile)
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
		Profile.ProfileMetadata.ProfileName = FString::Printf(TEXT("P5 %s"), *P5RealProfileCase);
		if (P5RealProfileCase.Equals(TEXT("EmptyProfile"), ESearchCase::IgnoreCase))
		{
			// CreateFreshProfile intentionally grants the normal starter stash.  P5's
			// EmptyProfile migration case instead needs a truly empty legacy source.
			Profile.PermanentStash.Reset();
		}
		auto AddLegacyStashItem = [&Profile](const FName DefinitionId, const int32 Quantity = 1, const FGuid LegacySpatialParentItemId = FGuid()) -> FGuid
		{
			Fdemo_mapPersistentItemRecord Item;
			Item.ItemInstanceId = FGuid::NewGuid();
			Item.ItemDefinitionId = DefinitionId;
			Item.StackCount = Quantity;
			Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			Item.LegacySpatialParentItemInstanceId = LegacySpatialParentItemId;
			const FGuid ItemId = Item.ItemInstanceId;
			Profile.PermanentStash.Add(MoveTemp(Item));
			return ItemId;
		};
		if (P5RealProfileCase.Equals(TEXT("LegacyPopulated"), ESearchCase::IgnoreCase)
			|| P5RealProfileCase.Equals(TEXT("LegacyInvalid"), ESearchCase::IgnoreCase))
		{
			// These are legacy Profile records, not Code B fixtures.  They are only
			// created before startup so the visible trace can enter through the
			// ordinary Sect button and perform the one-time real handoff itself.
			// The isolated formal profile deliberately contains one replacement for
			// every equipment role and two compatible material stacks.  The trace
			// still reaches them only through the normal Sect Slate entrance; this
			// seed is legacy Profile data, never a Code B fixture.
			AddLegacyStashItem(Fdemo_mapItemIds::WeaponLevel1);
			AddLegacyStashItem(Fdemo_mapItemIds::WeaponLevel2);
			AddLegacyStashItem(Fdemo_mapItemIds::ArmorRobeLevel1);
			AddLegacyStashItem(Fdemo_mapItemIds::ArmorRobeLevel2);
			AddLegacyStashItem(Fdemo_mapItemIds::EvasionCharm);
			AddLegacyStashItem(Fdemo_mapItemIds::EvasionCharm);
			const FGuid LegacyRingItemId = AddLegacyStashItem(Fdemo_mapItemIds::WindTalisman);
			AddLegacyStashItem(Fdemo_mapItemIds::AccessoryLevel1);
			AddLegacyStashItem(Fdemo_mapItemIds::BackpackLevel1);
			AddLegacyStashItem(Fdemo_mapItemIds::BackpackLevel2);
			AddLegacyStashItem(Fdemo_mapItemIds::SpiritDust, 3);
			AddLegacyStashItem(Fdemo_mapItemIds::SpiritDust, 2);
			AddLegacyStashItem(Fdemo_mapItemIds::SpiritWoodLevel1, 2, LegacyRingItemId);
			AddLegacyStashItem(Fdemo_mapItemIds::SpiritWoodLevel1, 2);
			if (P5RealProfileCase.Equals(TEXT("LegacyInvalid"), ESearchCase::IgnoreCase))
			{
				// The retired layout reference intentionally does not point at a
				// current persistent item.  Profile validation permits this opaque
				// legacy metadata, while Code B must still open its valid content.
				Profile.PreparationLayout.WeaponItemInstanceId = FGuid::NewGuid();
			}
		}
		else if (!P5RealProfileCase.Equals(TEXT("EmptyProfile"), ESearchCase::IgnoreCase))
		{
			UE_LOG(Logdemo_map, Error, TEXT("P5_REAL_PROFILE_STARTUP: case must be LegacyPopulated, EmptyProfile, or LegacyInvalid; received=%s."), *P5RealProfileCase);
			return false;
		}

		const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
		if (!Save.IsSuccess())
		{
			UE_LOG(Logdemo_map, Error, TEXT("P5_REAL_PROFILE_STARTUP: failed to seed isolated %s Profile: %s"), *P5RealProfileCase, *Save.Diagnostic);
			return false;
		}
		UE_LOG(Logdemo_map, Log, TEXT("P5_REAL_PROFILE_STARTUP: seeded formal legacy Profile case=%s owner=%s storage=%s."),
			*P5RealProfileCase, *Profile.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), *P5RealProfileStorageRoot);
	}

	const Fdemo_mapProfileSessionInitializeResult Init =
		ProfilePreparationFlow->InitializeExplicit(GetGameInstance(), P5RealProfileStorageRoot);
	ProfileFlowInitializeStatus = Init.Status;
	if (!Init.IsReady())
	{
		UE_LOG(Logdemo_map, Error, TEXT("P5_REAL_PROFILE_STARTUP: isolated session initialization failed status=%d diagnostic=%s."),
			static_cast<int32>(Init.Status), *Init.Diagnostic);
		ProfilePreparationFlow->Unbind();
		ProfilePreparationFlow.Reset();
		return false;
	}
	UE_LOG(Logdemo_map, Log, TEXT("P5_REAL_PROFILE_STARTUP: formal Profile ready case=%s owner=%s storage=%s."),
		*P5RealProfileCase, *Init.Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), *P5RealProfileStorageRoot);
	return true;
}

bool Ademo_mapV3ProgressionManager::InitializeP6ProductStartBridgeProfile()
{
	if (P6ProductStartBridgeStorageRoot.TrimStartAndEnd().IsEmpty()
		|| P6ProductStartBridgeCase.TrimStartAndEnd().IsEmpty())
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("P6R1_PRODUCT_START: isolated -P6ProductStartBridgeStorageRoot and -P6ProductStartBridgeCase are required."));
		return false;
	}
	FString CanonicalRoot;
	FString BoundaryDiagnostic;
	if (!Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(
			P6ProductStartBridgeStorageRoot, CanonicalRoot, BoundaryDiagnostic)
		|| (bP6ProductStartBridgeR3Trace
			? !CanonicalRoot.Contains(TEXT("Dev.D.UE.0.0.9B.P6.0.r3"))
			: !CanonicalRoot.Contains(TEXT("P6ProductStartBridge"))))
	{
		UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: rejected isolated storage root: %s"), *BoundaryDiagnostic);
		return false;
	}
	P6ProductStartBridgeStorageRoot = CanonicalRoot;
	if (bP6ProductStartBridgeR3Trace && P6ProductStartBridgePhase.IsEmpty())
	{
		P6ProductStartBridgePhase = TEXT("Single");
	}
	const bool bNotEnrolled = P6ProductStartBridgeCase.Equals(TEXT("NotEnrolled"), ESearchCase::IgnoreCase);
	const bool bEmpty = P6ProductStartBridgeCase.Equals(TEXT("Empty"), ESearchCase::IgnoreCase);
	const bool bBridgeFailure = P6ProductStartBridgeCase.Equals(TEXT("BridgeFailure"), ESearchCase::IgnoreCase);
	const bool bPreparedRecovery = P6ProductStartBridgeCase.Equals(TEXT("PreparedRecovery"), ESearchCase::IgnoreCase);
	const bool bConflict = P6ProductStartBridgeCase.Equals(TEXT("ActiveSessionConflict"), ESearchCase::IgnoreCase);
	const bool bLock = P6ProductStartBridgeCase.Equals(TEXT("OutOfRaidLock"), ESearchCase::IgnoreCase);
	const bool bCompleteCarry = P6ProductStartBridgeCase.Equals(TEXT("CompleteCarry"), ESearchCase::IgnoreCase);
	if (!(bNotEnrolled || bEmpty || bBridgeFailure || bPreparedRecovery || bConflict || bLock || bCompleteCarry))
	{
		UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: unsupported case=%s."), *P6ProductStartBridgeCase);
		return false;
	}

#if WITH_DEV_AUTOMATION_TESTS
	FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(false);
#else
	if (bBridgeFailure || bPreparedRecovery)
	{
		UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: receipt-failure cases require WITH_DEV_AUTOMATION_TESTS."));
		return false;
	}
#endif

	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(P6ProductStartBridgeStorageRoot);
	const bool bHasExistingProfile = IFileManager::Get().FileExists(*Storage.PrimaryPath())
		|| IFileManager::Get().FileExists(*Storage.BackupPath());
	P6ProductExpectedCarryItemIds.Reset();
	P6ProductWarehouseOnlyItemId.Invalidate();
	if (!bHasExistingProfile)
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
		Profile.ProfileMetadata.ProfileName = FString::Printf(TEXT("P6r1 %s"), *P6ProductStartBridgeCase);
		Profile.PermanentStash.Reset();
		auto AddLegacyItem = [&Profile](const FName DefinitionId, const int32 Quantity = 1, const FGuid ParentId = FGuid())
		{
			Fdemo_mapPersistentItemRecord Item;
			Item.ItemInstanceId = FGuid::NewGuid();
			Item.ItemDefinitionId = DefinitionId;
			Item.StackCount = Quantity;
			Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			Item.LegacySpatialParentItemInstanceId = ParentId;
			const FGuid ItemId = Item.ItemInstanceId;
			Profile.PermanentStash.Add(MoveTemp(Item));
			return ItemId;
		};
		if (!bEmpty && !bNotEnrolled)
		{
			const FGuid Weapon = AddLegacyItem(Fdemo_mapItemIds::WeaponLevel1);
			const FGuid Armor = AddLegacyItem(Fdemo_mapItemIds::ArmorRobeLevel1);
			const FGuid Accessory = AddLegacyItem(Fdemo_mapItemIds::EvasionCharm);
			const FGuid Ring = AddLegacyItem(Fdemo_mapItemIds::WindTalisman);
			const FGuid Backpack = AddLegacyItem(Fdemo_mapItemIds::BackpackLevel1);
			const FGuid Dust = AddLegacyItem(Fdemo_mapItemIds::SpiritDust, 3);
			const FGuid RingChild = AddLegacyItem(Fdemo_mapItemIds::SpiritWoodLevel1, 2, Ring);
			const FGuid PouchChild = AddLegacyItem(Fdemo_mapItemIds::SpiritDust, 1, Backpack);
			P6ProductWarehouseOnlyItemId = AddLegacyItem(Fdemo_mapItemIds::SpiritDust, 2);
			P6ProductExpectedCarryItemIds = { Weapon, Armor, Accessory, Ring, Backpack, Dust, RingChild, PouchChild };
			Profile.PreparationLayout.WeaponItemInstanceId = Weapon;
			Profile.PreparationLayout.ArmorItemInstanceId = Armor;
			Profile.PreparationLayout.AccessoryItemInstanceId = Accessory;
			Profile.PreparationLayout.SpatialRingItemInstanceId = Ring;
			Profile.PreparationLayout.BackpackItemInstanceId = Backpack;
			Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds = { Dust };
		}
		const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
		if (!Save.IsSuccess())
		{
			UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: isolated Profile seed failed: %s"), *Save.Diagnostic);
			return false;
		}
	}

	const Fdemo_mapProfileSessionInitializeResult Init =
		ProfilePreparationFlow->InitializeExplicit(GetGameInstance(), P6ProductStartBridgeStorageRoot);
	ProfileFlowInitializeStatus = Init.Status;
	if (!Init.IsReady())
	{
		UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: isolated Profile initialization failed: %s"), *Init.Diagnostic);
		return false;
	}
	P6ProductOwnerId = Init.Snapshot.ProfileId;
	if (bNotEnrolled)
	{
		UE_LOG(Logdemo_map, Log, TEXT("P6R1_PRODUCT_START: seeded NotEnrolled owner=%s storage=%s."),
			*P6ProductOwnerId.ToString(EGuidFormats::DigitsWithHyphens), *P6ProductStartBridgeStorageRoot);
		return true;
	}

	FCodeBOutOfRaidProfileStore Store(P6ProductStartBridgeStorageRoot, P6ProductOwnerId);
	demo_map_code_b::FCodeBRepository Repository;
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
	FCodeBOutOfRaidInventoryRecord P6RecordAtInitialization;
	const bool bP6r3RestartRecovery = bP6ProductStartBridgeR3Trace
		&& bHasExistingProfile
		&& (bPreparedRecovery || bConflict)
		&& P6ProductStartBridgePhase.Equals(TEXT("Recovery"), ESearchCase::IgnoreCase);
	if (bP6r3RestartRecovery)
	{
	#if WITH_DEV_AUTOMATION_TESTS
		FString RecoveryReadError;
		if (!FCodeBOutOfRaidProfileStore::TryReadRecordForLifecycleAutomation(
			P6ProductStartBridgeStorageRoot, P6ProductOwnerId, P6RecordAtInitialization, &RecoveryReadError))
		{
			UE_LOG(Logdemo_map, Error, TEXT("P6R3_PRODUCT_START: could not read the prior verified P6 receipt after Code A recovery: %s"), *RecoveryReadError);
			return false;
		}
	#else
		UE_LOG(Logdemo_map, Error, TEXT("P6R3_PRODUCT_START: restart recovery evidence requires WITH_DEV_AUTOMATION_TESTS."));
		return false;
	#endif
	}
	else
	{
		const FCodeBOutOfRaidOpenResult Open = Store.OpenOrMigrate(Init.Snapshot, Repository, Layout);
		if (!Open.bSuccess)
		{
			UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: P5 enrollment failed: %s"), *Open.Diagnostic);
			return false;
		}
		P6RecordAtInitialization = Store.GetRecord();
	}
	P6ProductSourceSnapshotBefore = P6RecordAtInitialization.RepositorySnapshot;
	P6ProductSourceLayoutBefore = P6RecordAtInitialization.Layout;
	P6ProductTraceSequence = 0;
	if (bConflict && !bP6ProductStartBridgeR3Trace)
	{
		P6ProductPreviousRunId = FGuid::NewGuid();
		const FCodeBRunInventoryBridgeResult Previous =
			FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(
				P6ProductStartBridgeStorageRoot, P6ProductOwnerId, P6ProductPreviousRunId);
		if (!Previous.IsCommitted())
		{
			UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: could not seed prior active session: %s"), *Previous.Diagnostic);
			return false;
		}
		const FCodeBOutOfRaidOpenResult Reload = Store.OpenOrMigrate(Init.Snapshot, Repository, Layout);
		if (!Reload.bSuccess)
		{
			UE_LOG(Logdemo_map, Error, TEXT("P6R1_PRODUCT_START: could not reload the seeded active session: %s"), *Reload.Diagnostic);
			return false;
		}
		P6RecordAtInitialization = Store.GetRecord();
	}
	if (P6RecordAtInitialization.bHasActiveRunInventorySession)
	{
		const FCodeBRunInventorySession& ExistingSession = P6RecordAtInitialization.ActiveRunInventorySession;
		if (bP6ProductStartBridgeR3Trace
			&& (bPreparedRecovery || bConflict)
			&& bHasExistingProfile)
		{
			P6ProductPreviousRunId = ExistingSession.RunInstanceId;
			P6ProductExpectedCarryItemIds = ExistingSession.Receipt.MovedItemIds;
			P6ProductSourceSnapshotBefore = P6RecordAtInitialization.RepositorySnapshot;
			for (const TPair<FGuid, demo_map_code_b::FCodeBItemInstance>& Pair : P6ProductSourceSnapshotBefore.Items)
			{
				if (!P6ProductExpectedCarryItemIds.Contains(Pair.Key))
				{
					P6ProductWarehouseOnlyItemId = Pair.Key;
					break;
				}
			}
		}
		P6ProductPersistentRevisionBefore = P6RecordAtInitialization.PersistentRevision;
	}
	else
	{
		P6ProductPersistentRevisionBefore = P6RecordAtInitialization.PersistentRevision;
	}
#if WITH_DEV_AUTOMATION_TESTS
	FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(
		bBridgeFailure || (bPreparedRecovery && !bP6ProductStartBridgeR3Trace
			? true : bPreparedRecovery && !P6ProductStartBridgePhase.Equals(TEXT("Recovery"), ESearchCase::IgnoreCase)));
#endif
	UE_LOG(Logdemo_map, Log,
		TEXT("P6R1_PRODUCT_START: seeded case=%s owner=%s storage=%s enrolled=1 expected_carry=%d."),
		*P6ProductStartBridgeCase,
		*P6ProductOwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*P6ProductStartBridgeStorageRoot,
		P6ProductExpectedCarryItemIds.Num());
	return true;
}

bool Ademo_mapV3ProgressionManager::InitializeExplicitProfileFlow()
{
	if (bP6ProductStartBridgeAutomation)
	{
		return InitializeP6ProductStartBridgeProfile();
	}
	if (bP5RealProfileTraceStartup)
	{
		return InitializeP5RealProfileTraceProfile();
	}
	if (bP4xStartRunAutomation)
	{
		if (!FParse::Value(FCommandLine::Get(), TEXT("P4xStartRunStorageRoot="), P4xStartRunStorageRoot)
			|| !FParse::Value(FCommandLine::Get(), TEXT("P4xStartRunCase="), P4xStartRunCase)
			|| !P4xStartRunStorageRoot.Contains(TEXT("P4xStartRun")))
		{
			UE_LOG(Logdemo_map, Error, TEXT("P4X_PRODUCT_START: isolated -P4xStartRunStorageRoot containing P4xStartRun and -P4xStartRunCase are required."));
			return false;
		}

		Fdemo_mapProfileRepository Repository;
		Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
		if (P4xStartRunCase.Equals(TEXT("StaleLegacy"), ESearchCase::IgnoreCase))
		{
			Profile.PreparationLayout.WeaponItemInstanceId = FGuid::NewGuid();
			Profile.PreparationLayout.SpatialRingItemInstanceId = FGuid::NewGuid();
			Profile.PreparationLayout.HotbarItemInstanceIds.SetNum(2);
		}
		else if (!P4xStartRunCase.Equals(TEXT("Normal"), ESearchCase::IgnoreCase)
			&& !P4xStartRunCase.Equals(TEXT("NoEquipment"), ESearchCase::IgnoreCase)
			&& !P4xStartRunCase.Equals(TEXT("MissingPreparation"), ESearchCase::IgnoreCase))
		{
			UE_LOG(Logdemo_map, Error, TEXT("P4X_PRODUCT_START: case must be Normal, NoEquipment, MissingPreparation, or StaleLegacy; received=%s."), *P4xStartRunCase);
			return false;
		}

		const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(P4xStartRunStorageRoot);
		const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
		if (!Save.IsSuccess())
		{
			UE_LOG(Logdemo_map, Error, TEXT("P4X_PRODUCT_START: could not seed isolated %s profile: %s."), *P4xStartRunCase, *Save.Diagnostic);
			return false;
		}

		ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
		const Fdemo_mapProfileSessionInitializeResult Init = ProfilePreparationFlow->InitializeExplicit(
			GetGameInstance(), P4xStartRunStorageRoot);
		ProfileFlowInitializeStatus = Init.Status;
		if (!Init.IsReady())
		{
			UE_LOG(Logdemo_map, Error, TEXT("P4X_PRODUCT_START: isolated %s session initialization failed: %s."), *P4xStartRunCase, *Init.Diagnostic);
			ProfilePreparationFlow->Unbind();
			ProfilePreparationFlow.Reset();
			return false;
		}
		UE_LOG(Logdemo_map, Log, TEXT("P4X_PRODUCT_START: seeded isolated case=%s storage=%s."), *P4xStartRunCase, *P4xStartRunStorageRoot);
		return true;
	}

	if (bXFix1SettlementLifecycleAutomation
		|| bXFix1SettlementRestartAutomation)
	{
		if (XFix1StorageRoot.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(Logdemo_map, Error, TEXT("XFIX1_SETTLEMENT_AUTOMATION: explicit isolated storage root is required."));
			return false;
		}
		ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
		const Fdemo_mapProfileSessionInitializeResult Init =
			ProfilePreparationFlow->InitializeExplicit(
				GetGameInstance(),
				XFix1StorageRoot);
		ProfileFlowInitializeStatus = Init.Status;
		if (!Init.IsReady())
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("XFIX1_SETTLEMENT_AUTOMATION: initialization failed status=%d diagnostic=%s."),
				static_cast<int32>(Init.Status),
				*Init.Diagnostic);
			return false;
		}
		return true;
	}
	if (bInputRestoreAutomation)
	{
		return InitializeInputRestoreAutomation();
	}
	if (bFullSystemLoopAutomation)
	{
		return InitializeFullSystemLoopAutomation();
	}
	if (bProfileTradeAutomation)
	{
		FString PhaseValue;
		if (!FParse::Value(FCommandLine::Get(), TEXT("ProfileTradePhase="), PhaseValue)
			|| !FParse::Value(FCommandLine::Get(), TEXT("ProfileTradeStorageRoot="), ProfileTradeStorageRoot)
			|| !FParse::Value(FCommandLine::Get(), TEXT("ProfileTradeHandoff="), ProfileTradeHandoffPath))
		{
			UE_LOG(Logdemo_map, Error, TEXT("PROFILE_TRADE_SMOKE: explicit phase, storage root, and handoff are required."));
			return false;
		}
		if (PhaseValue.Equals(TEXT("Trade"), ESearchCase::IgnoreCase))
		{
			ProfileTradeAutomationPhase = Edemo_mapProfileTradeAutomationPhase::Trade;
		}
		else if (PhaseValue.Equals(TEXT("Reload"), ESearchCase::IgnoreCase))
		{
			ProfileTradeAutomationPhase = Edemo_mapProfileTradeAutomationPhase::Reload;
		}
		else
		{
			UE_LOG(Logdemo_map, Error, TEXT("PROFILE_TRADE_SMOKE: phase must be Trade or Reload."));
			return false;
		}
		if (ProfileTradeAutomationPhase == Edemo_mapProfileTradeAutomationPhase::Reload)
		{
			FFileHelper::LoadFileToArray(
				ProfileTradePrimaryBytesBeforeLoad,
				*Fdemo_mapProfileStorageContext::ForRoot(ProfileTradeStorageRoot).PrimaryPath());
		}
		ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
		const Fdemo_mapProfileSessionInitializeResult Init = ProfilePreparationFlow->InitializeExplicit(
			GetGameInstance(), ProfileTradeStorageRoot);
		ProfileFlowInitializeStatus = Init.Status;
		if (!Init.IsReady())
		{
			UE_LOG(Logdemo_map, Error, TEXT("PROFILE_TRADE_SMOKE: isolated initialization failed: %s"), *Init.Diagnostic);
			ProfilePreparationFlow->Unbind();
			ProfilePreparationFlow.Reset();
			return false;
		}
		UE_LOG(Logdemo_map, Log, TEXT("PROFILE_TRADE_SMOKE: initialized phase=%s storage=%s handoff=%s."),
			*PhaseValue, *ProfilePreparationFlow->GetStorageRoot(), *ProfileTradeHandoffPath);
		return true;
	}

	FString PhaseValue;
	if (!FParse::Value(FCommandLine::Get(), TEXT("ProfileFlowPhase="), PhaseValue)
		|| !FParse::Value(FCommandLine::Get(), TEXT("ProfileFlowStorageRoot="), ProfileFlowStorageRoot))
	{
		UE_LOG(Logdemo_map, Error, TEXT("PROFILE_PREPARATION_V3_FLOW: explicit flag requires both -ProfileFlowPhase and -ProfileFlowStorageRoot; no fallback was used."));
		return false;
	}
	if (PhaseValue.Equals(TEXT("Preparation"), ESearchCase::IgnoreCase)) ProfileFlowAutomationPhase = Edemo_mapProfileFlowAutomationPhase::Preparation;
	else if (PhaseValue.Equals(TEXT("Extract"), ESearchCase::IgnoreCase)) ProfileFlowAutomationPhase = Edemo_mapProfileFlowAutomationPhase::Extract;
	else if (PhaseValue.Equals(TEXT("Death"), ESearchCase::IgnoreCase)) ProfileFlowAutomationPhase = Edemo_mapProfileFlowAutomationPhase::Death;
	else if (PhaseValue.Equals(TEXT("Crash"), ESearchCase::IgnoreCase)) ProfileFlowAutomationPhase = Edemo_mapProfileFlowAutomationPhase::Crash;
	else if (PhaseValue.Equals(TEXT("Recovered"), ESearchCase::IgnoreCase)) ProfileFlowAutomationPhase = Edemo_mapProfileFlowAutomationPhase::Recovered;
	else
	{
		UE_LOG(Logdemo_map, Error, TEXT("PROFILE_PREPARATION_V3_FLOW: phase must be Preparation, Extract, Death, Crash, or Recovered; received=%s."), *PhaseValue);
		return false;
	}
	FString ExpectedItemValue;
	if (FParse::Value(FCommandLine::Get(), TEXT("ProfileFlowExpectedItemId="), ExpectedItemValue))
	{
		FGuid::Parse(ExpectedItemValue, ProfileFlowExpectedItemId);
	}

	ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
	const Fdemo_mapProfileSessionInitializeResult Init = ProfilePreparationFlow->InitializeExplicit(
		GetGameInstance(),
		ProfileFlowStorageRoot);
	ProfileFlowInitializeStatus = Init.Status;
	if (!Init.IsReady())
	{
		UE_LOG(Logdemo_map, Error, TEXT("PROFILE_PREPARATION_V3_FLOW: session initialization rejected status=%d diagnostic=%s."), static_cast<int32>(Init.Status), *Init.Diagnostic);
		ProfilePreparationFlow->Unbind();
		ProfilePreparationFlow.Reset();
		return false;
	}

	UE_LOG(Logdemo_map, Log, TEXT("PROFILE_PREPARATION_V3_FLOW: isolated initialization status=%d phase=%s storage=%s."),
		static_cast<int32>(Init.Status),
		*PhaseValue,
		*ProfilePreparationFlow->GetStorageRoot());
	return true;
}

void Ademo_mapV3ProgressionManager::RunP4xStartRunAutomation()
{
	auto Fail = [this](const FString& Reason)
	{
		FailAutomation(TEXT("P4X_PRODUCT_START: FAIL: ") + Reason);
	};
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession() || !Items.IsValid())
	{
		Fail(TEXT("Profile lifecycle or Runtime authority is unavailable."));
		return;
	}
	Udemo_mapProfileSessionSubsystem* Session = ProfilePreparationFlow->GetSession();
	if (P4xStartRunAutomationStep == 0)
	{
		P4xPreparationLayoutBefore = Session->GetSnapshot().PreparationLayout;
		if (!SectNavigationWidget || !SectNavigationWidget->IsInViewport()
			|| ProfilePreparationWidget != nullptr
			|| Items->GetRunState() != Edemo_mapRunState::Inactive
			|| !SectNavigationWidget->AutomationClickStartRunFromTeleport())
		{
			Fail(TEXT("the visible Sect Teleport CTA could not initiate an idle product Start Run without a Preparation widget."));
			return;
		}
		P4xStartRunAutomationStep = 1;
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunP4xStartRunAutomation, 0.45f, false);
		return;
	}
	if (P4xStartRunAutomationStep == 1)
	{
		const Fdemo_mapProfileSessionSnapshot Active = Session->GetSnapshot();
		if (!Active.ActiveRunId.IsValid() || !bProfileWorldActive
			|| Items->GetRunState() != Edemo_mapRunState::Active
			|| ProfilePreparationWidget != nullptr
			|| Active.PreparationLayout != P4xPreparationLayoutBefore
			|| !Items->GetDeployedItemIds().IsEmpty())
		{
			Fail(TEXT("the first Teleport CTA Start Run did not activate the existing product world independently of legacy preparation data."));
			return;
		}
		P4xStartRunFirstId = Active.ActiveRunId;
		const FString P4xScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("P4xScreenshots"));
		IFileManager::Get().MakeDirectory(*P4xScreenshotDirectory, true);
		const FString P4xScreenshotFile = FString::Printf(TEXT("P4x.0.r2_ProductStart_%s_%dx%d.png"), *P4xStartRunCase, GSystemResolution.ResX, GSystemResolution.ResY);
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(P4xScreenshotDirectory, P4xScreenshotFile), true, false);
		UE_LOG(Logdemo_map, Log, TEXT("P4X_PRODUCT_START: screenshot=%s entry=SectTeleportCTA."), *P4xScreenshotFile);
		const Fdemo_mapItemOperationResult Settlement = RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction);
		if (!Settlement.bSuccess)
		{
			Fail(TEXT("the first product Start Run could not return through Extraction."));
			return;
		}
		P4xStartRunAutomationStep = 2;
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunP4xStartRunAutomation, 0.45f, false);
		return;
	}
	if (P4xStartRunAutomationStep == 2)
	{
		if (bProfileWorldActive || ProfilePreparationWidget != nullptr
			|| !SectNavigationWidget || !SectNavigationWidget->IsInViewport()
			|| !SectNavigationWidget->AutomationClickStartRunFromTeleport())
		{
			Fail(TEXT("post-Extraction product UI did not expose the same Teleport CTA for a second Start Run."));
			return;
		}
		P4xStartRunAutomationStep = 3;
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunP4xStartRunAutomation, 0.45f, false);
		return;
	}

	const Fdemo_mapProfileSessionSnapshot Restart = Session->GetSnapshot();
	const bool bLegacyLayoutUnchangedAfterRestart = Restart.PreparationLayout == P4xPreparationLayoutBefore;
	if (!Restart.ActiveRunId.IsValid() || Restart.ActiveRunId == P4xStartRunFirstId
		|| !bProfileWorldActive || Items->GetRunState() != Edemo_mapRunState::Active
		|| ProfilePreparationWidget != nullptr
		|| !Items->GetDeployedItemIds().IsEmpty())
	{
		Fail(TEXT("the second Teleport CTA Start Run failed to reactivate the product world without a Preparation widget or deployed legacy items."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("P4X_PRODUCT_START: PASS case=%s entry=SectTeleportCTA first_run=%s second_run=%s preparation_widget=0 legacy_layout_first_start_unchanged=1 legacy_layout_restart_unchanged=%d deployed_items=0 product_world=1."),
		*P4xStartRunCase,
		*P4xStartRunFirstId.ToString(EGuidFormats::DigitsWithHyphens),
		*Restart.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		bLegacyLayoutUnchangedAfterRestart ? 1 : 0);
	PassAutomation(TEXT("P4X_PRODUCT_START: PASS."));
}

void Ademo_mapV3ProgressionManager::RunP6ProductStartBridgeAutomation()
{
	auto Fail = [this](const FString& Reason)
	{
#if WITH_DEV_AUTOMATION_TESTS
		FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(false);
#endif
		if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
		{
			const Fdemo_mapProfileSessionSnapshot ExitSnapshot =
				(ProfilePreparationFlow && ProfilePreparationFlow->GetSession())
				? ProfilePreparationFlow->GetSession()->GetSnapshot()
				: Fdemo_mapProfileSessionSnapshot();
			LogP6ProductStartBridgeR2Trace(
				TEXT("ProcessExit"), ExitSnapshot, nullptr, nullptr,
				ECodeBRunInventoryBridgeStatus::StorageFailure,
				FString::Printf(TEXT("RequestExitWithStatus(1); Reason=%s"), *Reason));
		}
		WriteP6ProductStartBridgeR3Outcome(false, Reason);
		FailAutomation(TEXT("demo_map.CodeB.P6.ProductStartBridge: FAIL: ") + Reason);
	};
#if !WITH_DEV_AUTOMATION_TESTS
	Fail(TEXT("P6r1 lifecycle cases require WITH_DEV_AUTOMATION_TESTS."));
	return;
#else
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession() || !Items.IsValid())
	{
		Fail(TEXT("Profile lifecycle or Runtime authority is unavailable."));
		return;
	}
	Udemo_mapProfileSessionSubsystem* Session = ProfilePreparationFlow->GetSession();
	auto ReadRecord = [this](FCodeBOutOfRaidInventoryRecord& OutRecord, FString& OutError)
	{
		return FCodeBOutOfRaidProfileStore::TryReadRecordForLifecycleAutomation(
			P6ProductStartBridgeStorageRoot, P6ProductOwnerId, OutRecord, &OutError);
	};
	auto ContainsAll = [](const demo_map_code_b::FCodeBSnapshot& Snapshot, const TArray<FGuid>& ItemIds)
	{
		for (const FGuid& ItemId : ItemIds)
		{
			if (!Snapshot.Items.Contains(ItemId)) return false;
		}
		return true;
	};
	auto ContainsNone = [](const demo_map_code_b::FCodeBSnapshot& Snapshot, const TArray<FGuid>& ItemIds)
	{
		for (const FGuid& ItemId : ItemIds)
		{
			if (Snapshot.Items.Contains(ItemId)) return false;
		}
		return true;
	};

	if (P6ProductStartBridgeStep == 0)
	{
		const Fdemo_mapProfileSessionSnapshot Ready = Session->GetSnapshot();
		if (Ready.ProfileId != P6ProductOwnerId || Ready.ActiveRunId.IsValid()
			|| Ready.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
			|| !SectNavigationWidget || !SectNavigationWidget->IsInViewport()
			|| ProfilePreparationWidget != nullptr
			|| Items->GetRunState() != Edemo_mapRunState::Inactive)
		{
			Fail(TEXT("the isolated Owner was not ReadyForPreparation before the real Start Run CTA."));
			return;
		}
		bP6ProductBridgeObserved = false;
		P6ProductLastBridge = FCodeBRunInventoryBridgeResult();
		LogP6ProductStartBridgeR2Trace(
			TEXT("ProductStartRunCTA"), Ready, nullptr, nullptr,
			ECodeBRunInventoryBridgeStatus::StorageFailure,
			TEXT("Route=SectTeleportCTA; Click=AutomationClickStartRunFromTeleport"));
		UE_LOG(Logdemo_map, Log,
			TEXT("P6R1_PRODUCT_START_TRACE Event=StartRequest Route=SectTeleportCTA Case=%s OwnerId=%s CodeAState=ReadyForPreparation."),
			*P6ProductStartBridgeCase,
			*P6ProductOwnerId.ToString(EGuidFormats::DigitsWithHyphens));
		if (!SectNavigationWidget->AutomationClickStartRunFromTeleport())
		{
			Fail(TEXT("the visible Sect Teleport Start Run CTA rejected the product request."));
			return;
		}
		P6ProductStartBridgeStep = 1;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunP6ProductStartBridgeAutomation,
			0.45f,
			false);
		return;
	}

	const Fdemo_mapProfileSessionSnapshot Active = Session->GetSnapshot();
	if (Active.ProfileId != P6ProductOwnerId || !Active.ActiveRunId.IsValid()
		|| !bProfileWorldActive || Items->GetRunState() != Edemo_mapRunState::Active
		|| ProfilePreparationWidget != nullptr || !bP6ProductBridgeObserved)
	{
		Fail(TEXT("the product CTA did not reach successful Code A Run activation and its post-activation observer."));
		return;
	}
	UE_LOG(Logdemo_map, Log,
		TEXT("P6R1_PRODUCT_START_TRACE Event=ActivationSuccess Case=%s OwnerId=%s RunInstanceId=%s CodeAStart=Success WorldActivation=Success ObserverDelivered=1 BridgeStatus=%d."),
		*P6ProductStartBridgeCase,
		*Active.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*Active.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		static_cast<int32>(P6ProductLastBridge.Status));

	const bool bNotEnrolled = P6ProductStartBridgeCase.Equals(TEXT("NotEnrolled"), ESearchCase::IgnoreCase);
	const bool bEmpty = P6ProductStartBridgeCase.Equals(TEXT("Empty"), ESearchCase::IgnoreCase);
	const bool bBridgeFailure = P6ProductStartBridgeCase.Equals(TEXT("BridgeFailure"), ESearchCase::IgnoreCase);
	const bool bPreparedRecovery = P6ProductStartBridgeCase.Equals(TEXT("PreparedRecovery"), ESearchCase::IgnoreCase);
	const bool bConflict = P6ProductStartBridgeCase.Equals(TEXT("ActiveSessionConflict"), ESearchCase::IgnoreCase);
	const bool bLock = P6ProductStartBridgeCase.Equals(TEXT("OutOfRaidLock"), ESearchCase::IgnoreCase);
	const bool bCompleteCarry = P6ProductStartBridgeCase.Equals(TEXT("CompleteCarry"), ESearchCase::IgnoreCase);
	const bool bR3RecoveryPhase = bP6ProductStartBridgeR3Trace
		&& P6ProductStartBridgePhase.Equals(TEXT("Recovery"), ESearchCase::IgnoreCase);
	FCodeBOutOfRaidInventoryRecord Record;
	FString ReadError;

	if (bNotEnrolled)
	{
		FCodeBOutOfRaidProfileStore Probe(P6ProductStartBridgeStorageRoot, P6ProductOwnerId);
		if (P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::NotEnrolled
			|| IFileManager::Get().FileExists(*Probe.GetPrimaryPath())
			|| IFileManager::Get().FileExists(*Probe.GetBackupPath()))
		{
			Fail(TEXT("unenrolled Code A success created or mutated a Code B sidecar."));
			return;
		}
	}
	else if (!ReadRecord(Record, ReadError))
	{
		Fail(TEXT("the enrolled product observer left no readable Owner-isolated record: ") + ReadError);
		return;
	}
	else if (Record.OwnerId != P6ProductOwnerId)
	{
		Fail(TEXT("the product observer did not preserve OwnerId isolation."));
		return;
	}

	if (bNotEnrolled)
	{
		// The preceding probe already proves the observer preserved Code A success
		// without enrolling or writing a Code B sidecar.
	}
	else if (bCompleteCarry)
	{
		if (!P6ProductLastBridge.IsCommitted() || !Record.bHasActiveRunInventorySession
			|| Record.ActiveRunInventorySession.RunInstanceId != Active.ActiveRunId
			|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Committed
			|| !ContainsAll(Record.ActiveRunInventorySession.RepositorySnapshot, P6ProductExpectedCarryItemIds)
			|| !ContainsNone(Record.RepositorySnapshot, P6ProductExpectedCarryItemIds)
			|| !Record.RepositorySnapshot.Items.Contains(P6ProductWarehouseOnlyItemId))
		{
			Fail(TEXT("complete carry did not atomically place the prepared selection in the Run session while retaining the warehouse-only item."));
			return;
		}
		if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
		{
			auto LogItemMapping = [this, &Record](const FGuid& ItemId, const TCHAR* ItemRole)
			{
				const demo_map_code_b::FCodeBItemInstance* BeforeItem = P6ProductSourceSnapshotBefore.Items.Find(ItemId);
				const demo_map_code_b::FCodeBItemInstance* RunItem = Record.ActiveRunInventorySession.RepositorySnapshot.Items.Find(ItemId);
				const demo_map_code_b::FCodeBItemInstance* P5AfterItem = Record.RepositorySnapshot.Items.Find(ItemId);
				UE_LOG(Logdemo_map, Log,
					TEXT("P6R2_ITEM_MAPPING Case=%s Role=%s ItemId=%s Definition=%s P5BeforeParent=%s P5BeforeSlot=%d P5BeforeChild=%s RunParent=%s RunSlot=%d RunChild=%s P5AfterPresent=%d RunPresent=%d"),
					*P6ProductStartBridgeCase, ItemRole,
					*ItemId.ToString(EGuidFormats::DigitsWithHyphens),
					BeforeItem ? *BeforeItem->DefinitionId.ToString() : TEXT("<missing>"),
					BeforeItem ? *BeforeItem->ParentContainerId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("<missing>"),
					BeforeItem ? BeforeItem->SlotIndex : INDEX_NONE,
					BeforeItem ? *BeforeItem->ChildContainerId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("<missing>"),
					RunItem ? *RunItem->ParentContainerId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("<missing>"),
					RunItem ? RunItem->SlotIndex : INDEX_NONE,
					RunItem ? *RunItem->ChildContainerId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("<missing>"),
					P5AfterItem ? 1 : 0, RunItem ? 1 : 0);
			};
			static const TCHAR* Roles[] = {
				TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory0"), TEXT("SpatialRing"),
				TEXT("Pouch"), TEXT("Basic0"), TEXT("SpatialRingChild"), TEXT("PouchChild") };
			for (int32 Index = 0; Index < P6ProductExpectedCarryItemIds.Num(); ++Index)
			{
				LogItemMapping(P6ProductExpectedCarryItemIds[Index], Roles[FMath::Min<int32>(Index, static_cast<int32>(UE_ARRAY_COUNT(Roles)) - 1)]);
			}
			const demo_map_code_b::FCodeBItemInstance* WarehouseBefore =
				P6ProductSourceSnapshotBefore.Items.Find(P6ProductWarehouseOnlyItemId);
			const demo_map_code_b::FCodeBItemInstance* WarehouseAfter =
				Record.RepositorySnapshot.Items.Find(P6ProductWarehouseOnlyItemId);
			if (!WarehouseBefore || !WarehouseAfter
				|| WarehouseBefore->ParentContainerId != WarehouseAfter->ParentContainerId
				|| WarehouseBefore->SlotIndex != WarehouseAfter->SlotIndex)
			{
				Fail(TEXT("warehouse-only item moved while validating CompleteCarry."));
				return;
			}
			UE_LOG(Logdemo_map, Log,
				TEXT("P6R2_ITEM_MAPPING Case=%s Role=WarehouseUntouched ItemId=%s Definition=%s Parent=%s Slot=%d P5AfterPresent=1 RunPresent=0"),
				*P6ProductStartBridgeCase,
				*P6ProductWarehouseOnlyItemId.ToString(EGuidFormats::DigitsWithHyphens),
				*WarehouseAfter->DefinitionId.ToString(),
				*WarehouseAfter->ParentContainerId.ToString(EGuidFormats::DigitsWithHyphens), WarehouseAfter->SlotIndex);
			const demo_map_code_b::FCodeBContainer* BeforeBasic =
				P6ProductSourceSnapshotBefore.Containers.Find(P6ProductSourceLayoutBefore.BasicContainerId);
			const demo_map_code_b::FCodeBContainer* RunBasic =
				Record.ActiveRunInventorySession.RepositorySnapshot.Containers.Find(
					Record.ActiveRunInventorySession.Layout.BasicContainerId);
			for (int32 Slot = 0; Slot < 6; ++Slot)
			{
				const FGuid BeforeId = (BeforeBasic && BeforeBasic->Slots.IsValidIndex(Slot)) ? BeforeBasic->Slots[Slot] : FGuid();
				const FGuid RunId = (RunBasic && RunBasic->Slots.IsValidIndex(Slot)) ? RunBasic->Slots[Slot] : FGuid();
				UE_LOG(Logdemo_map, Log,
					TEXT("P6R2_BASIC6 Case=%s Slot=%d P5BeforeItem=%s RunItem=%s"),
					*P6ProductStartBridgeCase, Slot,
					*BeforeId.ToString(EGuidFormats::DigitsWithHyphens),
					*RunId.ToString(EGuidFormats::DigitsWithHyphens));
			}
		}
	}
	else if (bEmpty)
	{
		if (!P6ProductLastBridge.IsCommitted() || !Record.bHasActiveRunInventorySession
			|| Record.ActiveRunInventorySession.RunInstanceId != Active.ActiveRunId
			|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Committed
			|| !Record.RepositorySnapshot.Items.IsEmpty()
			|| !Record.ActiveRunInventorySession.RepositorySnapshot.Items.IsEmpty())
		{
			Fail(TEXT("empty Profile start did not produce an empty committed Code B Run session."));
			return;
		}
	}
	else if (bBridgeFailure)
	{
		if (P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::StorageFailure
			|| !Record.bHasActiveRunInventorySession
			|| Record.ActiveRunInventorySession.RunInstanceId != Active.ActiveRunId
			|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Prepared
			|| !ContainsAll(Record.RepositorySnapshot, P6ProductExpectedCarryItemIds))
		{
			Fail(TEXT("post-activation bridge fault did not retain one verified Prepared receipt with the P5 source unextracted."));
			return;
		}
	}
	else if (bPreparedRecovery)
	{
		if (bP6ProductStartBridgeR3Trace && !bR3RecoveryPhase)
		{
			if (P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::StorageFailure
				|| !Record.bHasActiveRunInventorySession
				|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Prepared
				|| !Record.ActiveRunInventorySession.Receipt.ReceiptId.IsValid()
				|| Record.ActiveRunInventorySession.Receipt.OriginRunId != Active.ActiveRunId
				|| !ContainsAll(Record.RepositorySnapshot, P6ProductExpectedCarryItemIds))
			{
				Fail(TEXT("P6r3 initial CTA did not persist one verified immutable Prepared receipt."));
				return;
			}
			LogP6ProductStartBridgeR2Trace(
				TEXT("PreparedReceiptAwaitingRestart"), Active, nullptr, &Record,
				P6ProductLastBridge.Status,
				FString::Printf(TEXT("ReceiptId=%s; OriginRunId=%s; PayloadDigest=%s; P5Unextracted=1"),
					*Record.ActiveRunInventorySession.Receipt.ReceiptId.ToString(EGuidFormats::DigitsWithHyphens),
					*Record.ActiveRunInventorySession.Receipt.OriginRunId.ToString(EGuidFormats::DigitsWithHyphens),
					*Record.ActiveRunInventorySession.Receipt.PayloadDigest));
			FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(false);
			LogP6ProductStartBridgeR2Trace(
				TEXT("ProcessExit"), Active, nullptr, &Record, P6ProductLastBridge.Status,
				TEXT("RequestExitWithStatus(0); verified Prepared receipt persisted for restart."));
			WriteP6ProductStartBridgeR3Outcome(true, TEXT("Prepared receipt persisted; restart required for Code A RecoveredAbandon and second real CTA."));
			PassAutomation(TEXT("demo_map.CodeB.P6.ProductStartBridge: PASS."));
			return;
		}
		if (bP6ProductStartBridgeR3Trace && bR3RecoveryPhase)
		{
			const FCodeBRunInventoryBridgeReceipt& Receipt = Record.ActiveRunInventorySession.Receipt;
			if (P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::RecoveredPreparedReceipt
				|| !Record.bHasActiveRunInventorySession
				|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Committed
				|| Record.ActiveRunInventorySession.RunInstanceId != Active.ActiveRunId
				|| Receipt.OriginRunId != P6ProductPreviousRunId
				|| Receipt.RecoveryRebindHistory.Num() != 1
				|| Receipt.RecoveryRebindHistory[0].OldRunId != P6ProductPreviousRunId
				|| Receipt.RecoveryRebindHistory[0].NewRunId != Active.ActiveRunId
				|| Receipt.RecoveryRebindHistory[0].CodeATerminalCause != TEXT("RecoveredAbandon")
				|| !ContainsAll(Record.ActiveRunInventorySession.RepositorySnapshot, P6ProductExpectedCarryItemIds)
				|| !ContainsNone(Record.RepositorySnapshot, P6ProductExpectedCarryItemIds)
				|| !Record.RepositorySnapshot.Items.Contains(P6ProductWarehouseOnlyItemId))
			{
				Fail(TEXT("P6r3 second real CTA did not rebind and commit the immutable Prepared receipt exactly once."));
				return;
			}
			// The normal r2 direct observer replay below is intentionally bypassed.
			FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(false);
			LogP6ProductStartBridgeR2Trace(
				TEXT("ProcessExit"), Active, nullptr, &Record, P6ProductLastBridge.Status,
				TEXT("RequestExitWithStatus(0); P6r3 rebind scenario assertions completed."));
			WriteP6ProductStartBridgeR3Outcome(true, TEXT("Second real CTA rebound and committed the verified receipt."));
			PassAutomation(TEXT("demo_map.CodeB.P6.ProductStartBridge: PASS."));
			return;
		}
		if (P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::StorageFailure
			|| !Record.bHasActiveRunInventorySession
			|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Prepared
			|| !ContainsAll(Record.RepositorySnapshot, P6ProductExpectedCarryItemIds))
		{
			Fail(TEXT("interruption setup did not leave the verified Prepared receipt required for observer recovery."));
			return;
		}
		if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
		{
			LogP6ProductStartBridgeR2Trace(
				TEXT("PreparedReceiptAwaitingSecondProductCTA"), Active, nullptr, &Record,
				P6ProductLastBridge.Status,
				TEXT("R2 forbids the r1 direct observer replay. Existing Code A keeps this Run active; a new product CTA after process recovery receives a different RunInstanceId and is rejected by the unchanged P6 active-session guard."));
			Fail(TEXT("P6r2 audit evidence stops at the Prepared receipt: no permitted second real CTA can re-enter the original RunInstanceId without changing Code A lifecycle or P6 bridge semantics."));
			return;
		}
		FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(false);
		bP6ProductBridgeObserved = false;
		ObserveCodeBRunAfterActivation(Active);
		if (!bP6ProductBridgeObserved
			|| P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::RecoveredPreparedReceipt
			|| !ReadRecord(Record, ReadError)
			|| !Record.bHasActiveRunInventorySession
			|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Committed
			|| Record.ActiveRunInventorySession.RunInstanceId != Active.ActiveRunId
			|| !ContainsAll(Record.ActiveRunInventorySession.RepositorySnapshot, P6ProductExpectedCarryItemIds)
			|| !ContainsNone(Record.RepositorySnapshot, P6ProductExpectedCarryItemIds))
		{
			Fail(TEXT("Prepared receipt recovery did not re-enter the same post-activation observer for a single committed handoff."));
			return;
		}
		UE_LOG(Logdemo_map, Log,
			TEXT("P6R1_PRODUCT_START_TRACE Event=ObserverRecovery Case=%s OwnerId=%s RunInstanceId=%s BridgeStatus=%d Route=PostActivationObserver."),
			*P6ProductStartBridgeCase,
			*Active.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Active.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			static_cast<int32>(P6ProductLastBridge.Status));
	}
	else if (bConflict)
	{
		if (bP6ProductStartBridgeR3Trace && !bR3RecoveryPhase)
		{
			if (!P6ProductLastBridge.IsCommitted() || !Record.bHasActiveRunInventorySession
				|| Record.ActiveRunInventorySession.RunInstanceId != Active.ActiveRunId
				|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Committed)
			{
				Fail(TEXT("P6r3 conflict setup CTA did not create the committed prior Run session."));
				return;
			}
			LogP6ProductStartBridgeR2Trace(
				TEXT("ProcessExit"), Active, nullptr, &Record, P6ProductLastBridge.Status,
				TEXT("RequestExitWithStatus(0); committed prior Run session persisted for restart."));
			WriteP6ProductStartBridgeR3Outcome(true, TEXT("Committed prior Run session persisted; restart required for real conflicting CTA."));
			PassAutomation(TEXT("demo_map.CodeB.P6.ProductStartBridge: PASS."));
			return;
		}
		if (P6ProductLastBridge.Status != ECodeBRunInventoryBridgeStatus::ActiveSessionConflict
			|| !Record.bHasActiveRunInventorySession
			|| Record.ActiveRunInventorySession.RunInstanceId != P6ProductPreviousRunId
			|| Record.PersistentRevision != P6ProductPersistentRevisionBefore)
		{
			Fail(TEXT("a different active RunId was not refused without changing the prior Code B Run session."));
			return;
		}
	}
	else if (bLock)
	{
		const int32 RevisionBeforeNormalEntry = Record.PersistentRevision;
		SectNavigationWidget->AutomationOpenWarehouseFromTeleport();
		if (SectNavigationWidget->GetTeleportFeedbackForAutomation() != TEXT("当前 Run 中，返回后再整理")
			|| !ReadRecord(Record, ReadError)
			|| Record.PersistentRevision != RevisionBeforeNormalEntry
			|| HasCodeBOutOfRaidProfileForAutomation())
		{
			Fail(TEXT("the normal P5 entry route did not reject the active Code B session with the required no-write lock."));
			return;
		}
		UE_LOG(Logdemo_map, Log,
			TEXT("P6R1_PRODUCT_START_TRACE Event=NormalEntryLock Case=%s OwnerId=%s RunInstanceId=%s Feedback=当前 Run 中，返回后再整理 PersistentWrite=0 Route=SectTeleportWarehouse."),
			*P6ProductStartBridgeCase,
			*Active.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Active.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens));
	}
	else
	{
		Fail(TEXT("the requested P6r1 scenario was not recognized after product activation."));
		return;
	}

	FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(false);
	if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
	{
		LogP6ProductStartBridgeR2Trace(
			TEXT("ProcessExit"), Active, nullptr, bNotEnrolled ? nullptr : &Record,
			P6ProductLastBridge.Status,
			TEXT("RequestExitWithStatus(0); Product scenario assertions completed."));
	}
	UE_LOG(Logdemo_map, Log,
		TEXT("demo_map.CodeB.P6.ProductStartBridge: PASS case=%s owner=%s run=%s code_a_start=success activation=success observer=post_activation bridge_status=%d controlled_exit=1."),
		*P6ProductStartBridgeCase,
		*Active.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*Active.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		static_cast<int32>(P6ProductLastBridge.Status));
	WriteP6ProductStartBridgeR3Outcome(true, TEXT("Product CTA scenario assertions completed."));
	PassAutomation(TEXT("demo_map.CodeB.P6.ProductStartBridge: PASS."));
#endif
}

void Ademo_mapV3ProgressionManager::LogP6ProductStartBridgeR2Trace(
	const TCHAR* Event,
	const Fdemo_mapProfileSessionSnapshot& Snapshot,
	const FCodeBOutOfRaidInventoryRecord* BeforeRecord,
	const FCodeBOutOfRaidInventoryRecord* AfterRecord,
	const ECodeBRunInventoryBridgeStatus BridgeStatus,
	const FString& Detail)
{
	if (!bP6ProductStartBridgeR2Trace && !bP6ProductStartBridgeR3Trace)
	{
		return;
	}
	auto PersistentRevision = [](const FCodeBOutOfRaidInventoryRecord* Record)
	{
		return Record ? Record->PersistentRevision : INDEX_NONE;
	};
	auto SessionRevision = [](const FCodeBOutOfRaidInventoryRecord* Record)
	{
		return (Record && Record->bHasActiveRunInventorySession)
			? Record->ActiveRunInventorySession.SessionRevision : INDEX_NONE;
	};
	auto ReceiptState = [](const FCodeBOutOfRaidInventoryRecord* Record)
	{
		return (Record && Record->bHasActiveRunInventorySession)
			? static_cast<int32>(Record->ActiveRunInventorySession.Receipt.State) : INDEX_NONE;
	};
	UE_LOG(Logdemo_map, Log,
		TEXT("%s_TRACE Sequence=%d Case=%s Phase=%s Event=%s OwnerId=%s RunInstanceId=%s OwnerDocumentRevisionBefore=%d OwnerDocumentRevisionAfter=%d RunSessionRevisionBefore=%d RunSessionRevisionAfter=%d ReceiptBefore=%d ReceiptAfter=%d BridgeStatus=%d Detail=%s"),
		bP6ProductStartBridgeR3Trace ? TEXT("P6R3") : TEXT("P6R2"),
		++P6ProductTraceSequence, *P6ProductStartBridgeCase, *P6ProductStartBridgePhase, Event,
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		PersistentRevision(BeforeRecord), PersistentRevision(AfterRecord),
		SessionRevision(BeforeRecord), SessionRevision(AfterRecord),
		ReceiptState(BeforeRecord), ReceiptState(AfterRecord),
		static_cast<int32>(BridgeStatus), *Detail);
}

void Ademo_mapV3ProgressionManager::WriteP6ProductStartBridgeR3Outcome(
	const bool bPassed,
	const FString& Detail) const
{
	if (!bP6ProductStartBridgeR3Trace || P6ProductStartBridgeStorageRoot.IsEmpty()) return;
	const FString SafeDetail = Detail.ReplaceCharWithEscapedChar();
	const FString Text = FString::Printf(
		TEXT("{\"task\":\"Dev.D.UE.0.0.9B.P6.0.r3\",\"scenario\":\"%s\",\"phase\":\"%s\",\"outcome\":\"%s\",\"trace_sequence\":%d,\"detail\":\"%s\"}"),
		*P6ProductStartBridgeCase, *P6ProductStartBridgePhase, bPassed ? TEXT("PASS") : TEXT("FAIL"),
		P6ProductTraceSequence, *SafeDetail);
	FFileHelper::SaveStringToFile(
		Text,
		*FPaths::Combine(P6ProductStartBridgeStorageRoot,
			FString::Printf(TEXT("P6r3ProductOutcome.%s.json"), *P6ProductStartBridgePhase)),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool Ademo_mapV3ProgressionManager::InitializeFullSystemLoopAutomation()
{
	FString PhaseValue;
	if (!FParse::Value(FCommandLine::Get(), TEXT("FullSystemLoopPhase="), PhaseValue)
		|| !FParse::Value(FCommandLine::Get(), TEXT("FullSystemLoopStorageRoot="), FullSystemLoopStorageRoot)
		|| !FParse::Value(FCommandLine::Get(), TEXT("FullSystemLoopUserConfigRoot="), FullSystemLoopUserConfigRoot))
	{
		UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: phase, isolated Profile root, and isolated User Config root are required."));
		return false;
	}
	if (PhaseValue.Equals(TEXT("Loop"), ESearchCase::IgnoreCase))
		FullSystemLoopAutomationPhase = Edemo_mapFullSystemLoopAutomationPhase::Loop;
	else if (PhaseValue.Equals(TEXT("Reload"), ESearchCase::IgnoreCase))
		FullSystemLoopAutomationPhase = Edemo_mapFullSystemLoopAutomationPhase::Reload;
	else if (PhaseValue.Equals(TEXT("Death"), ESearchCase::IgnoreCase))
		FullSystemLoopAutomationPhase = Edemo_mapFullSystemLoopAutomationPhase::Death;
	else if (PhaseValue.Equals(TEXT("DeathReload"), ESearchCase::IgnoreCase))
		FullSystemLoopAutomationPhase = Edemo_mapFullSystemLoopAutomationPhase::DeathReload;
	else if (PhaseValue.Equals(TEXT("Crash"), ESearchCase::IgnoreCase))
		FullSystemLoopAutomationPhase = Edemo_mapFullSystemLoopAutomationPhase::Crash;
	else if (PhaseValue.Equals(TEXT("Recovered"), ESearchCase::IgnoreCase))
		FullSystemLoopAutomationPhase = Edemo_mapFullSystemLoopAutomationPhase::Recovered;
	else
	{
		UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: phase must be Loop, Reload, Death, DeathReload, Crash, or Recovered."));
		return false;
	}

	const FString DefaultP8AllowedRoot = CanonicalSearchContainerDirectory(FPaths::Combine(
		FPaths::ProjectDir(), TEXT("Saved"), TEXT("Automation"), TEXT("Dev.D.UE.0.0.5.P8.0.r0")));
	FString ExplicitAllowedRoot;
	FParse::Value(
		FCommandLine::Get(),
		TEXT("FullSystemLoopAutomationRoot="),
		ExplicitAllowedRoot);
	FString CommandUserDir;
	FParse::Value(FCommandLine::Get(), TEXT("UserDir="), CommandUserDir);
	const FString GeneratedConfig = FPaths::GeneratedConfigDir();
	FString AllowedRoot;
	FString CanonicalStorage;
	FString CanonicalUserRoot;
	if (ExplicitAllowedRoot.IsEmpty())
	{
		AllowedRoot = DefaultP8AllowedRoot;
		CanonicalStorage = CanonicalSearchContainerDirectory(FullSystemLoopStorageRoot);
		CanonicalUserRoot = CanonicalSearchContainerDirectory(FullSystemLoopUserConfigRoot);
		const FString Production = CanonicalSearchContainerDirectory(
			Fdemo_mapProfileStorageContext::Production().RootDirectory);
		const FString CanonicalCommandUserRoot =
			CanonicalSearchContainerDirectory(CommandUserDir);
		const FString CanonicalGeneratedConfig =
			CanonicalSearchContainerDirectory(GeneratedConfig);
		if (!AllowedRoot.Equals(DefaultP8AllowedRoot, ESearchCase::IgnoreCase)
			|| !IsSameOrChildSearchContainerPath(CanonicalStorage, AllowedRoot)
			|| IsSameOrChildSearchContainerPath(CanonicalStorage, Production)
			|| IsSameOrChildSearchContainerPath(Production, CanonicalStorage)
			|| (IFileManager::Get().DirectoryExists(*CanonicalStorage)
				&& IFileManager::Get().IsSymlink(*CanonicalStorage))
			|| CommandUserDir.IsEmpty()
			|| !CanonicalCommandUserRoot.Equals(CanonicalUserRoot, ESearchCase::IgnoreCase)
			|| !IsSameOrChildSearchContainerPath(CanonicalUserRoot, AllowedRoot)
			|| (IFileManager::Get().DirectoryExists(*CanonicalUserRoot)
				&& IFileManager::Get().IsSymlink(*CanonicalUserRoot))
			|| !IsSameOrChildSearchContainerPath(CanonicalGeneratedConfig, CanonicalUserRoot))
		{
			UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: rejected default isolation boundary root=%s profile=%s user=%s generated_config=%s."),
				*AllowedRoot, *CanonicalStorage, *CanonicalUserRoot, *CanonicalGeneratedConfig);
			return false;
		}
	}
	else
	{
		const Fdemo_mapAutomationRootBoundaryResult Boundary =
			Fdemo_mapProfilePreparationFlow::ValidateExplicitAutomationBoundary(
				ExplicitAllowedRoot,
				FullSystemLoopStorageRoot,
				FullSystemLoopUserConfigRoot,
				CommandUserDir,
				GeneratedConfig,
				{
					ExplicitAllowedRoot,
					FullSystemLoopStorageRoot,
					FullSystemLoopUserConfigRoot,
					GeneratedConfig
				});
		if (!Boundary.bAccepted)
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("FULL_SYSTEM_STARTUP: FAIL: AUTOMATION_ROOT_BOUNDARY %s."),
				*Boundary.Diagnostic());
			return false;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("FULL_SYSTEM_STARTUP: AUTOMATION_ROOT_BOUNDARY %s."),
			*Boundary.Diagnostic());
		AllowedRoot = Boundary.CanonicalRoot;
		CanonicalStorage = Boundary.CanonicalStorageRoot;
		CanonicalUserRoot = Boundary.CanonicalUserConfigRoot;
	}
	FullSystemLoopStorageRoot = CanonicalStorage;
	FullSystemLoopUserConfigRoot = CanonicalUserRoot;
	FullSystemLoopHandoffPath = FPaths::Combine(FullSystemLoopStorageRoot, TEXT("FullSystemLoop.handoff"));

	const bool bFreshPhase =
		FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Loop
		|| FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Death
		|| FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Crash;
	const FString PrimaryPath =
		Fdemo_mapProfileStorageContext::ForRoot(FullSystemLoopStorageRoot).PrimaryPath();
	if (bFreshPhase && IFileManager::Get().FileExists(*PrimaryPath))
	{
		UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: fresh phase refused an existing Profile primary."));
		return false;
	}
	if (!bFreshPhase
		&& !FFileHelper::LoadFileToArray(FullSystemLoopPrimaryBytesBeforeLoad, *PrimaryPath))
	{
		UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: reload phase could not capture the existing Profile primary."));
		return false;
	}

	ProfilePreparationFlow = MakeUnique<Fdemo_mapProfilePreparationFlow>();
	const Fdemo_mapProfileSessionInitializeResult Init =
		ProfilePreparationFlow->InitializeExplicit(GetGameInstance(), FullSystemLoopStorageRoot);
	ProfileFlowInitializeStatus = Init.Status;
	if (!Init.IsReady())
	{
		UE_LOG(Logdemo_map, Error, TEXT("FULL_SYSTEM_STARTUP: FAIL: isolated session initialization rejected status=%d diagnostic=%s."),
			static_cast<int32>(Init.Status), *Init.Diagnostic);
		ProfilePreparationFlow->Unbind();
		ProfilePreparationFlow.Reset();
		return false;
	}
	UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_STARTUP: phase=%s profile_root=%s user_root=%s init_status=%d."),
		*PhaseValue, *FullSystemLoopStorageRoot, *FullSystemLoopUserConfigRoot, static_cast<int32>(Init.Status));
	return true;
}

bool Ademo_mapV3ProgressionManager::PrepareFullSystemAutomationRun()
{
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession() || !ProfilePreparationWidget)
		return false;
	Udemo_mapProfileSessionSubsystem* Session = ProfilePreparationFlow->GetSession();
	const Fdemo_mapProfilePreparationSnapshot Preparation = Session->GetPreparationSnapshot();
	if (Preparation.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
		|| Preparation.PersistentSpiritStones != 0
		|| Session->GetSnapshot().RiskSpiritStones != 0)
		return false;

	FGuid Weapon;
	FGuid Armor;
	FGuid SpatialRing;
	for (const Fdemo_mapProfilePreparationStashRow& Row : Preparation.OrderedPermanentStashRows)
	{
		if (Row.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade) Weapon = Row.ItemInstanceId;
		else if (Row.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest) Armor = Row.ItemInstanceId;
		else if (Row.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman) SpatialRing = Row.ItemInstanceId;
	}
	if (!Weapon.IsValid() || !Armor.IsValid() || !SpatialRing.IsValid()
		|| !ProfilePreparationWidget->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, Weapon).IsAccepted()
		|| !ProfilePreparationWidget->SelectEquipment(Fdemo_mapItemIds::ArmorSlot, Armor).IsAccepted()
		|| !ProfilePreparationWidget->SelectEquipment(Fdemo_mapItemIds::SpatialRingSlot, SpatialRing).IsAccepted())
		return false;

	const Fdemo_mapProfileSessionBeginResult Begin = StartPreparedProfileRun();
	if (!Begin.IsRunActive()) return false;
	FullSystemLoopProfileId = Begin.Snapshot.ProfileId;
	FullSystemLoopRunId = Begin.Snapshot.ActiveRunId;
	return FullSystemLoopProfileId.IsValid() && FullSystemLoopRunId.IsValid();
}

void Ademo_mapV3ProgressionManager::ScheduleFullSystemAutomation(float Delay)
{
	GetWorldTimerManager().SetTimer(
		AutomationTimer,
		this,
		&Ademo_mapV3ProgressionManager::RunFullSystemLoopAutomation,
		Delay,
		false);
}

bool Ademo_mapV3ProgressionManager::TakeFullSystemRequiredLoot()
{
	const Fdemo_mapFullMapRewardSlot* EliteSlot =
		Fdemo_mapRewardFullMapDistribution::FindEnemyByEncounterId(
			Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced);
	Ademo_mapCorpseContainerActor* Corpse = nullptr;
	for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Candidate : Corpses)
	{
		if (Candidate.IsValid()
			&& EliteSlot
			&& EliteSlot->RewardClass
				== Edemo_mapFullMapRewardClass::EnemyElite
			&& Candidate->UsesGeneratedReward()
			&& Candidate->GetProjectionResult().Trace.StableSourceRoleId
				== EliteSlot->StableSourceRoleId)
		{
			Corpse = Candidate.Get();
			break;
		}
	}
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Corpse || !Controller || !PlayerPawn.IsValid() || !EliteSlot)
		return false;
	const Fdemo_mapRewardSourceProjectionResult& Plan =
		Corpse->GetProjectionResult();
	const Fdemo_mapRewardPlannedStack* BackpackPlan =
		Plan.PlannedStacks.FindByPredicate(
			[](const Fdemo_mapRewardPlannedStack& Stack)
			{
				const Fdemo_mapItemDefinition* Definition =
					Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
				return Stack.Section
						== Edemo_mapRuntimeContainerSection::Equipment
					&& Definition
					&& Definition->CategoryId
						== Fdemo_mapItemIds::BackpackCategory;
			});
	const Fdemo_mapRewardPlannedStack* CorePlan =
		Plan.PlannedStacks.FindByPredicate(
			[](const Fdemo_mapRewardPlannedStack& Stack)
			{
				const Fdemo_mapItemDefinition* Definition =
					Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
				return Stack.Section
						== Edemo_mapRuntimeContainerSection::Body
					&& Definition
					&& Definition->CategoryId
						== Fdemo_mapItemIds::CoreCategory
					&& Stack.DefinitionId.ToString().StartsWith(
						TEXT("Prototype.Item.Core.Inner."));
			});
	if (!Plan.IsSuccess()
		|| Plan.Trace.bFallbackUsed
		|| Plan.Trace.bJackpotHit
		|| Plan.Trace.bRareExtremeHit
		|| Plan.Trace.StableSourceRoleId != EliteSlot->StableSourceRoleId
		|| !BackpackPlan
		|| !CorePlan)
	{
		return false;
	}
	FullSystemLoopBackpackDefinitionId = BackpackPlan->DefinitionId;
	FullSystemLoopCoreDefinitionId = CorePlan->DefinitionId;
	FullSystemLoopRewardProjectionId = Plan.Trace.ProjectionId;
	FullSystemLoopRewardSourceRoleId =
		Plan.Trace.StableSourceRoleId;
	if (!MovePawnNear(Corpse)) return false;
	Controller->SetAutomationAimDirection(Corpse->GetActorLocation() - PlayerPawn->GetActorLocation());
	SetFocusedActor(Corpse);
	const FKey InteractKey =
		Fdemo_mapInputBindingSettings::Get().GetKey(Fdemo_mapInputActionIds::Interact);
	if (!Controller->DispatchAutomationKeyPressed(InteractKey)
		|| !Corpse->IsContainerOpening()
		|| !Corpse->CompleteActionForAutomation().bSuccess
		|| !Controller->DispatchAutomationKeyReleased(InteractKey)
		|| !Corpse->IsContainerOpened()
		|| !bSearchContainerOpen
		|| !SearchContainerWidget)
		return false;

	const TArray<Fdemo_mapRuntimeContainerSeedEntry> CanonicalSeed =
		Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(Plan);
	auto TakePlannedStack = [this, Corpse, &CanonicalSeed](
		const Fdemo_mapRewardPlannedStack& Planned,
		FName RequiredCategory,
		FGuid& OutId) -> bool
	{
		const Fdemo_mapRuntimeContainerSeedEntry* CanonicalEntry =
			CanonicalSeed.FindByPredicate(
				[&Planned](
					const Fdemo_mapRuntimeContainerSeedEntry& Candidate)
				{
					return Candidate.DefinitionId == Planned.DefinitionId
						&& Candidate.StackCount == Planned.StackCount
						&& Candidate.RewardEventKind
							== Planned.RewardEventKind
						&& Candidate.RewardEventId
							== Planned.RewardEventId
						&& Candidate.RewardSourceRoleId
							== Planned.RewardSourceRoleId
						&& Candidate.RareRewardEventId
							== Planned.RareRewardEventId;
				});
		if (!CanonicalEntry)
		{
			return false;
		}
		Fdemo_mapRuntimeContainerSnapshot Snapshot = Corpse->GetContainerSnapshot();
		const Fdemo_mapRuntimeContainerSectionSnapshot* Section =
			Snapshot.Sections.FindByPredicate(
				[CanonicalEntry](
					const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
				{
					return Candidate.Section == CanonicalEntry->Section;
				});
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry = Section
			? Section->OrderedOccupiedEntries.FindByPredicate(
				[CanonicalEntry](
					const Fdemo_mapRuntimeContainerEntrySnapshot& Candidate)
				{
					return Candidate.SlotIndex
						== CanonicalEntry->SlotIndex;
				})
			: nullptr;
		if (!Entry) return false;
		const Edemo_mapRuntimeContainerEntryState FoundState = Entry->State;
		if (FoundState == Edemo_mapRuntimeContainerEntryState::Identified)
		{
			if (Entry->DefinitionId != Planned.DefinitionId
				|| Entry->CategoryId != RequiredCategory)
				return false;
			OutId = Entry->ItemInstanceId;
		}
		if (FoundState == Edemo_mapRuntimeContainerEntryState::Hidden)
		{
			if (!SearchContainerWidget->AutomationClickEntry(
					CanonicalEntry->Section,
					CanonicalEntry->SlotIndex)
				|| !Corpse->IsContainerSearching()
				|| !Corpse->CompleteActionForAutomation().bSuccess)
				return false;
			Snapshot = Corpse->GetContainerSnapshot();
			Section = Snapshot.Sections.FindByPredicate(
				[CanonicalEntry](
					const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
				{
					return Candidate.Section == CanonicalEntry->Section;
				});
			Entry = Section
				? Section->OrderedOccupiedEntries.FindByPredicate(
					[CanonicalEntry](
						const Fdemo_mapRuntimeContainerEntrySnapshot& Candidate)
					{
						return Candidate.SlotIndex
							== CanonicalEntry->SlotIndex;
					})
				: nullptr;
			if (!Entry
				|| Entry->State != Edemo_mapRuntimeContainerEntryState::Identified
				|| Entry->DefinitionId != Planned.DefinitionId
				|| Entry->CategoryId != RequiredCategory)
				return false;
			OutId = Entry->ItemInstanceId;
		}
		if (!OutId.IsValid()) return false;
		if (!SearchContainerWidget->AutomationClickEntry(
				CanonicalEntry->Section,
				CanonicalEntry->SlotIndex)
			|| !SearchContainerWidget->GetLastResult().bSuccess
			|| SearchContainerWidget->GetLastResult().ItemInstanceId != OutId)
		{
			return false;
		}
		const Fdemo_mapItemInstance* RuntimeItem = Items.IsValid()
			? Items->GetAuthority().FindInstance(OutId)
			: nullptr;
		return RuntimeItem
			&& RuntimeItem->DefinitionId == Planned.DefinitionId
			&& RuntimeItem->InstanceId == OutId;
	};

	const bool bTaken =
		TakePlannedStack(
			*BackpackPlan,
			Fdemo_mapItemIds::BackpackCategory,
			FullSystemLoopBackpackId)
		&& TakePlannedStack(
			*CorePlan,
			Fdemo_mapItemIds::CoreCategory,
			FullSystemLoopCoreId);
	const bool bClosed = SearchContainerWidget->AutomationClickClose() && !bSearchContainerOpen;
	if (bTaken && bClosed)
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("FULL_SYSTEM_GENERATED_REWARD_SOURCE projection=%s role=%s class=EnemyElite generated=1 fallback=0 jackpot=miss rare=miss backpack_definition=%s backpack=%s core_definition=%s core=%s same_guid_take=2."),
			*FullSystemLoopRewardProjectionId.ToString(),
			*FullSystemLoopRewardSourceRoleId.ToString(),
			*FullSystemLoopBackpackDefinitionId.ToString(),
			*FullSystemLoopBackpackId.ToString(
				EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopCoreDefinitionId.ToString(),
			*FullSystemLoopCoreId.ToString(
				EGuidFormats::DigitsWithHyphens));
	}
	return bTaken && bClosed;
}

bool Ademo_mapV3ProgressionManager::CollectFullSystemSpiritStone()
{
	if (!SpiritStonePickup.IsValid() || !Items.IsValid() || !PlayerPawn.IsValid())
		return false;
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !MovePawnNear(SpiritStonePickup.Get())) return false;
	const int32 ItemCountBefore = Items->GetAuthority().GetInstanceSnapshot().Num();
	Controller->SetAutomationAimDirection(
		SpiritStonePickup->GetActorLocation() - PlayerPawn->GetActorLocation());
	SetFocusedActor(SpiritStonePickup.Get());
	const FKey InteractKey =
		Fdemo_mapInputBindingSettings::Get().GetKey(Fdemo_mapInputActionIds::Interact);
	const bool bPressed = Controller->DispatchAutomationKeyPressed(InteractKey);
	const bool bReleased = Controller->DispatchAutomationKeyReleased(InteractKey);
	const Fdemo_mapProfileSessionSnapshot Snapshot =
		ProfilePreparationFlow->GetSession()->GetSnapshot();
	return bPressed && bReleased
		&& !SpiritStonePickup.IsValid()
		&& Snapshot.PersistentSpiritStones == 0
		&& Snapshot.RiskSpiritStones == 20
		&& Items->GetAuthority().GetInstanceSnapshot().Num() == ItemCountBefore;
}

bool Ademo_mapV3ProgressionManager::WriteFullSystemHandoff() const
{
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()) return false;
	const Fdemo_mapProfileSessionSnapshot Snapshot =
		ProfilePreparationFlow->GetSession()->GetSnapshot();
	const FString Text = FString::Printf(
		TEXT("ProfileId=%s\nRunId=%s\nSettlementId=%s\nBackpackId=%s\nBackpackDefinition=%s\nCoreId=%s\nCoreDefinition=%s\nPillId=%s\nPillDefinition=%s\nProjection=%s\nSourceRole=%s\nCoreSaleValue=%lld\nPillBuyValue=%lld\nBalance=%lld\nGeneration=%d\nShopGeneration=%d\nAcceptedCommitCount=%d\n"),
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*FullSystemLoopRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
		*FullSystemLoopBackpackId.ToString(EGuidFormats::DigitsWithHyphens),
		*FullSystemLoopBackpackDefinitionId.ToString(),
		*FullSystemLoopCoreId.ToString(EGuidFormats::DigitsWithHyphens),
		*FullSystemLoopCoreDefinitionId.ToString(),
		*FullSystemLoopPillId.ToString(EGuidFormats::DigitsWithHyphens),
		*FullSystemLoopPillDefinitionId.ToString(),
		*FullSystemLoopRewardProjectionId.ToString(),
		*FullSystemLoopRewardSourceRoleId.ToString(),
		FullSystemLoopCoreSaleValue,
		FullSystemLoopPillBuyValue,
		FullSystemLoopExpectedBalance,
		FullSystemLoopExpectedGeneration,
		FullSystemLoopExpectedShopGeneration,
		FullSystemLoopAcceptedCommitCount);
	return FFileHelper::SaveStringToFile(
		Text, *FullSystemLoopHandoffPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool Ademo_mapV3ProgressionManager::ReadFullSystemHandoff(
	TMap<FString, FString>& OutValues) const
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FullSystemLoopHandoffPath)) return false;
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, true);
	for (const FString& Line : Lines)
	{
		FString Key;
		FString Value;
		if (!Line.Split(TEXT("="), &Key, &Value) || Key.IsEmpty() || Value.IsEmpty())
			return false;
		OutValues.Add(Key, Value);
	}
	return OutValues.Contains(TEXT("ProfileId"))
		&& OutValues.Contains(TEXT("RunId"))
		&& OutValues.Contains(TEXT("SettlementId"))
		&& OutValues.Contains(TEXT("BackpackId"))
		&& OutValues.Contains(TEXT("BackpackDefinition"))
		&& OutValues.Contains(TEXT("CoreId"))
		&& OutValues.Contains(TEXT("CoreDefinition"))
		&& OutValues.Contains(TEXT("PillId"))
		&& OutValues.Contains(TEXT("PillDefinition"))
		&& OutValues.Contains(TEXT("Projection"))
		&& OutValues.Contains(TEXT("SourceRole"))
		&& OutValues.Contains(TEXT("CoreSaleValue"))
		&& OutValues.Contains(TEXT("PillBuyValue"))
		&& OutValues.Contains(TEXT("Balance"))
		&& OutValues.Contains(TEXT("Generation"))
		&& OutValues.Contains(TEXT("ShopGeneration"))
		&& OutValues.Contains(TEXT("AcceptedCommitCount"));
}

void Ademo_mapV3ProgressionManager::RunFullSystemLoopAutomation()
{
	auto Fail = [this](const FString& Detail)
	{
		FailAutomation(TEXT("FULL_SYSTEM_LOOP: FAIL: ") + Detail);
	};
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession())
	{
		Fail(TEXT("Profile Preparation flow or Session is unavailable."));
		return;
	}
	Udemo_mapProfileSessionSubsystem* Session = ProfilePreparationFlow->GetSession();

	const bool bReloadPhase =
		FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Reload
		|| FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::DeathReload
		|| FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Recovered;
	if (bReloadPhase)
	{
		TMap<FString, FString> Handoff;
		FGuid ExpectedProfile;
		FGuid ExpectedSettlement;
		FGuid ExpectedBackpack;
		FGuid ExpectedCore;
		FGuid ExpectedPill;
		if (!ReadFullSystemHandoff(Handoff)
			|| !FGuid::Parse(Handoff[TEXT("ProfileId")], ExpectedProfile)
			|| !FGuid::Parse(Handoff[TEXT("SettlementId")], ExpectedSettlement)
			|| !FGuid::Parse(Handoff[TEXT("BackpackId")], ExpectedBackpack)
			|| !FGuid::Parse(Handoff[TEXT("CoreId")], ExpectedCore)
			|| !FGuid::Parse(Handoff[TEXT("PillId")], ExpectedPill))
		{
			Fail(TEXT("cross-process handoff is missing or invalid."));
			return;
		}
		const FName ExpectedBackpackDefinition(
			*Handoff[TEXT("BackpackDefinition")]);
		const FName ExpectedCoreDefinition(
			*Handoff[TEXT("CoreDefinition")]);
		const FName ExpectedPillDefinition(
			*Handoff[TEXT("PillDefinition")]);
		const FName ExpectedProjection(*Handoff[TEXT("Projection")]);
		const FName ExpectedSourceRole(*Handoff[TEXT("SourceRole")]);
		const int64 ExpectedSaleValue =
			FCString::Atoi64(*Handoff[TEXT("CoreSaleValue")]);
		const int64 ExpectedBuyValue =
			FCString::Atoi64(*Handoff[TEXT("PillBuyValue")]);
		const int64 ExpectedBalance =
			FCString::Atoi64(*Handoff[TEXT("Balance")]);
		const int32 ExpectedGeneration =
			FCString::Atoi(*Handoff[TEXT("Generation")]);
		const int32 ExpectedShopGeneration =
			FCString::Atoi(*Handoff[TEXT("ShopGeneration")]);
		const int32 ExpectedCommitCount =
			FCString::Atoi(*Handoff[TEXT("AcceptedCommitCount")]);
		const Fdemo_mapProfileSessionSnapshot Snapshot = Session->GetSnapshot();
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(FullSystemLoopStorageRoot);
		const Fdemo_mapProfileLoadResult FirstLoad = Repository.LoadExistingProfile(Storage);
		TArray<uint8> BytesAfterInitialization;
		FFileHelper::LoadFileToArray(BytesAfterInitialization, *Storage.PrimaryPath());
		const bool bCommon =
			FirstLoad.IsSuccess()
			&& FirstLoad.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
			&& Snapshot.ProfileId == ExpectedProfile
			&& Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
			&& Snapshot.RiskSpiritStones == 0;
		if (!bCommon)
		{
			Fail(TEXT("Current Schema, identity, ReadyForPreparation, or zero-risk reload invariant failed."));
			return;
		}

		if (FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Reload)
		{
			const auto HasExactItem = [&Snapshot](
				const FGuid& Id,
				FName DefinitionId)
			{
				return Snapshot.OrderedPermanentStash.ContainsByPredicate(
					[&](const Fdemo_mapPersistentItemRecord& Item)
					{
						return Item.ItemInstanceId == Id
							&& Item.ItemDefinitionId == DefinitionId;
					});
			};
			const Fdemo_mapItemDefinition* BackpackDefinition =
				Fdemo_mapItemDefinitions::Find(
					ExpectedBackpackDefinition);
			const Fdemo_mapItemDefinition* CoreDefinition =
				Fdemo_mapItemDefinitions::Find(
					ExpectedCoreDefinition);
			const Fdemo_mapItemDefinition* PillDefinition =
				Fdemo_mapItemDefinitions::Find(
					ExpectedPillDefinition);
			const FKey ReloadInteract =
				Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::Interact);
			if (FullSystemLoopPrimaryBytesBeforeLoad != BytesAfterInitialization
				|| Snapshot.PersistentSpiritStones != ExpectedBalance
				|| Snapshot.SaveGeneration != ExpectedGeneration
				|| ExpectedCommitCount != ExpectedGeneration
				|| Snapshot.ShopStock.Generation
					!= ExpectedShopGeneration
				|| Snapshot.LastSettlementId != ExpectedSettlement
				|| ExpectedProjection.IsNone()
				|| ExpectedSourceRole.IsNone()
				|| ExpectedSaleValue <= 0
				|| ExpectedBuyValue <= 0
				|| !BackpackDefinition
				|| BackpackDefinition->CategoryId
					!= Fdemo_mapItemIds::BackpackCategory
				|| !CoreDefinition
				|| CoreDefinition->CategoryId
					!= Fdemo_mapItemIds::CoreCategory
				|| !ExpectedCoreDefinition.ToString().StartsWith(
					TEXT("Prototype.Item.Core.Inner."))
				|| !PillDefinition
				|| PillDefinition->CategoryId
					!= Fdemo_mapItemIds::ConsumableCategory
				|| !ExpectedPillDefinition.ToString().StartsWith(
					TEXT("Prototype.Item.Consumable.HealingPill."))
				|| !HasExactItem(
					ExpectedBackpack,
					ExpectedBackpackDefinition)
				|| !HasExactItem(
					ExpectedPill,
					ExpectedPillDefinition)
				|| Snapshot.OrderedPermanentStash.ContainsByPredicate(
					[&](const Fdemo_mapPersistentItemRecord& Item)
					{
						return Item.ItemInstanceId == ExpectedCore;
					})
				|| Snapshot.PreparationLayout.BackpackItemInstanceId != ExpectedBackpack
				|| !Snapshot.PreparationLayout.OrderedRunInventoryItemInstanceIds.Contains(ExpectedPill)
				|| Snapshot.PreparationLayout.HotbarItemInstanceIds.Num() != 9
				|| Snapshot.PreparationLayout.HotbarItemInstanceIds[0] != ExpectedPill
				|| ReloadInteract != EKeys::H
				|| ReloadInteract == EKeys::G)
			{
				Fail(TEXT("Phase B dynamic Definition/GUID, balance equation, commit count, Shop generation, terminal identity, layout, Hotbar, H-not-G mapping, or read-only bytes mismatch."));
				return;
			}
			UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_LOOP_PHASE_B_IDENTITY profile=%s projection=%s role=%s backpack_definition=%s backpack=%s core_definition=%s core_sold=%s pill_definition=%s pill=%s sale_delta=%lld buy_delta=%lld balance=%lld equation=20+%lld-%lld generation=%d accepted_commit_count=%d shop_generation=%d bytes_unchanged=1 terminal_replay=0 interact=H g_triggers_interact=0."),
				*ExpectedProfile.ToString(EGuidFormats::DigitsWithHyphens),
				*ExpectedProjection.ToString(),
				*ExpectedSourceRole.ToString(),
				*ExpectedBackpackDefinition.ToString(),
				*ExpectedBackpack.ToString(EGuidFormats::DigitsWithHyphens),
				*ExpectedCoreDefinition.ToString(),
				*ExpectedCore.ToString(EGuidFormats::DigitsWithHyphens),
				*ExpectedPillDefinition.ToString(),
				*ExpectedPill.ToString(EGuidFormats::DigitsWithHyphens),
				ExpectedSaleValue,
				ExpectedBuyValue,
				ExpectedBalance,
				ExpectedSaleValue,
				ExpectedBuyValue,
				Snapshot.SaveGeneration,
				ExpectedCommitCount,
				Snapshot.ShopStock.Generation);
			PassAutomation(TEXT("FULL_SYSTEM_LOOP_PHASE_B_RELOAD: PASS."));
			return;
		}

		const bool bExpectedDeath =
			FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::DeathReload;
		const Edemo_mapRunEndReason ExpectedReason = bExpectedDeath
			? Edemo_mapRunEndReason::Death
			: Edemo_mapRunEndReason::RecoveredAbandon;
		const bool bLootAbsent =
			!Snapshot.OrderedPermanentStash.ContainsByPredicate(
				[&](const Fdemo_mapPersistentItemRecord& Item)
				{
					return Item.ItemInstanceId == ExpectedBackpack
						|| Item.ItemInstanceId == ExpectedCore;
				});
		if (Snapshot.PersistentSpiritStones != 0
			|| Snapshot.LastTerminalReason != ExpectedReason
			|| (bExpectedDeath
				&& Snapshot.LastSettlementId != ExpectedSettlement)
			|| !bLootAbsent
			|| !Snapshot.PreparationLayout.IsEmpty())
		{
			Fail(TEXT("risk-loss terminal reload retained currency, loot, or stale layout references."));
			return;
		}
		if (bExpectedDeath)
		{
			if (FullSystemLoopPrimaryBytesBeforeLoad != BytesAfterInitialization)
			{
				Fail(TEXT("Death reload changed settled Profile bytes."));
				return;
			}
			if (Snapshot.SaveGeneration != ExpectedGeneration
				|| Snapshot.ShopStock.Generation
					!= ExpectedShopGeneration
				|| ExpectedCommitCount != ExpectedGeneration
				|| ExpectedBackpackDefinition.IsNone()
				|| ExpectedCoreDefinition.IsNone()
				|| ExpectedProjection.IsNone()
				|| ExpectedSourceRole.IsNone())
			{
				Fail(TEXT("Death reload generation, Shop generation, generated identity, or terminal replay invariant failed."));
				return;
			}
			UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_DEATH_IDENTITY profile=%s projection=%s role=%s backpack_definition=%s backpack_lost=%s core_definition=%s core_lost=%s generation=%d accepted_commit_count=%d shop_generation=%d balance=0 terminal_replay=0 bytes_unchanged=1."),
				*ExpectedProfile.ToString(EGuidFormats::DigitsWithHyphens),
				*ExpectedProjection.ToString(),
				*ExpectedSourceRole.ToString(),
				*ExpectedBackpackDefinition.ToString(),
				*ExpectedBackpack.ToString(EGuidFormats::DigitsWithHyphens),
				*ExpectedCoreDefinition.ToString(),
				*ExpectedCore.ToString(EGuidFormats::DigitsWithHyphens),
				Snapshot.SaveGeneration,
				ExpectedCommitCount,
				Snapshot.ShopStock.Generation);
			PassAutomation(TEXT("FULL_SYSTEM_DEATH_RISK: PASS."));
			return;
		}

		TArray<uint8> BeforePureReload = BytesAfterInitialization;
		const int32 RecoveredGeneration = Snapshot.SaveGeneration;
		const Fdemo_mapProfileLoadResult SecondLoad = Repository.LoadExistingProfile(Storage);
		TArray<uint8> AfterPureReload;
		FFileHelper::LoadFileToArray(AfterPureReload, *Storage.PrimaryPath());
		if (!SecondLoad.IsSuccess()
			|| SecondLoad.Profile.SaveGeneration != RecoveredGeneration
			|| BeforePureReload != AfterPureReload
			|| !(ProfileFlowInitializeStatus == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted
				|| ProfileFlowInitializeStatus == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted))
		{
			Fail(TEXT("RecoveredAbandon was not exactly once or the second pure reload changed bytes."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_RECOVERED_IDENTITY profile=%s run=%s backpack_lost=%s core_lost=%s generation=%d second_reload_unchanged=1."),
			*ExpectedProfile.ToString(EGuidFormats::DigitsWithHyphens),
			*Handoff[TEXT("RunId")],
			*ExpectedBackpack.ToString(EGuidFormats::DigitsWithHyphens),
			*ExpectedCore.ToString(EGuidFormats::DigitsWithHyphens),
			RecoveredGeneration);
		PassAutomation(TEXT("FULL_SYSTEM_RECOVERED_ABANDON: PASS."));
		return;
	}

	const bool bAwaitingSettledVerification =
		FullSystemLoopAutomationStep == 3 || FullSystemLoopAutomationStep == 5;
	if (!bAwaitingSettledVerification
		&& (!bProfileWorldActive || !Items.IsValid()
			|| Items->GetRunState() != Edemo_mapRunState::Active))
	{
		Fail(TEXT("fresh product run is not active."));
		return;
	}
	for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);

	if (FullSystemLoopAutomationStep == 0)
	{
		Ademo_mapEnemyCharacter* Enhanced = nullptr;
		for (const TWeakObjectPtr<AActor>& Actor : EnemyActors)
		{
			Ademo_mapEnemyCharacter* Candidate = Cast<Ademo_mapEnemyCharacter>(Actor.Get());
			if (Candidate
				&& Candidate->GetEncounterIdentity().EncounterId
					== Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced)
			{
				Enhanced = Candidate;
				break;
			}
		}
		if (!Enhanced)
		{
			Fail(TEXT("P7.Encounter.Side.Melee.Enhanced is missing."));
			return;
		}
		UGameplayStatics::ApplyDamage(
			Enhanced, 100.0f, GetDemoController(), PlayerPawn.Get(), nullptr);
		FullSystemLoopAutomationStep = 1;
		ScheduleFullSystemAutomation(0.2f);
		return;
	}
	if (FullSystemLoopAutomationStep == 1)
	{
		if (!TakeFullSystemRequiredLoot())
		{
			Fail(TEXT("real Corpse hold/search/widget take did not preserve Backpack/Core GUIDs."));
			return;
		}
		FullSystemLoopAutomationStep = 2;
		ScheduleFullSystemAutomation(0.1f);
		return;
	}
	if (FullSystemLoopAutomationStep == 2)
	{
		if (!CollectFullSystemSpiritStone())
		{
			Fail(TEXT("real fixed Spirit Stone interaction or non-Item risk projection failed."));
			return;
		}
		if (!WriteFullSystemHandoff())
		{
			Fail(TEXT("cross-process handoff write failed."));
			return;
		}
		if (FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Crash)
		{
			UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_RECOVERED_CRASH_PHASE: active run=%s risk=20 backpack=%s core=%s preserved without Settlement."),
				*FullSystemLoopRunId.ToString(EGuidFormats::DigitsWithHyphens),
				*FullSystemLoopBackpackId.ToString(EGuidFormats::DigitsWithHyphens),
				*FullSystemLoopCoreId.ToString(EGuidFormats::DigitsWithHyphens));
			FPlatformMisc::RequestExitWithStatus(false, 0);
			return;
		}
		if (FullSystemLoopAutomationPhase == Edemo_mapFullSystemLoopAutomationPhase::Death)
		{
			AActor* DamageSource = nullptr;
			for (const TWeakObjectPtr<AActor>& Actor : EnemyActors)
			{
				if (Actor.IsValid())
				{
					DamageSource = Actor.Get();
					break;
				}
			}
			Udemo_mapPlayerHealthComponent* Health =
				PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
			if (!DamageSource || !Health)
			{
				Fail(TEXT("real Player Damage source or health component is missing."));
				return;
			}
			UGameplayStatics::ApplyDamage(
				PlayerPawn.Get(), 100.0f, DamageSource->GetInstigatorController(), DamageSource, nullptr);
			FullSystemLoopAutomationStep = 3;
			ScheduleFullSystemAutomation(0.35f);
			return;
		}
		int32 TargetCount = 0;
		for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It)
		{
			UGameplayStatics::ApplyDamage(
				*It, 2.0f, GetDemoController(), PlayerPawn.Get(), nullptr);
			++TargetCount;
		}
		if (TargetCount != 3)
		{
			Fail(TEXT("exact three Training Targets were not damaged through the product path."));
			return;
		}
		FullSystemLoopAutomationStep = 4;
		ScheduleFullSystemAutomation(0.2f);
		return;
	}
	if (FullSystemLoopAutomationStep == 3)
	{
		const Fdemo_mapProfileSessionSnapshot Snapshot = Session->GetSnapshot();
		if (Snapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
			|| Snapshot.LastTerminalReason != Edemo_mapRunEndReason::Death
			|| Snapshot.PersistentSpiritStones != 0
			|| Snapshot.RiskSpiritStones != 0
			|| Snapshot.OrderedPermanentStash.ContainsByPredicate(
				[this](const Fdemo_mapPersistentItemRecord& Item)
				{
					return Item.ItemInstanceId == FullSystemLoopBackpackId
						|| Item.ItemInstanceId == FullSystemLoopCoreId;
				}))
		{
			Fail(TEXT("real Death did not atomically lose risk Item/currency."));
			return;
		}
		FullSystemLoopExpectedBalance = 0;
		FullSystemLoopExpectedGeneration = Snapshot.SaveGeneration;
		FullSystemLoopExpectedShopGeneration =
			Snapshot.ShopStock.Generation;
		FullSystemLoopAcceptedCommitCount = Snapshot.SaveGeneration;
		if (!Snapshot.LastSettlementId.IsValid()
			|| !WriteFullSystemHandoff())
		{
			Fail(TEXT("Death terminal identity or final cross-process handoff write failed."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_DEATH_PHASE_A_COMPLETE profile=%s run=%s settlement=%s projection=%s role=%s backpack_definition=%s backpack_lost=%s core_definition=%s core_lost=%s risk_lost=20 balance=0 generation=%d accepted_commit_count=%d shop_generation=%d."),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.LastSettlementId.ToString(
				EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopRewardProjectionId.ToString(),
			*FullSystemLoopRewardSourceRoleId.ToString(),
			*FullSystemLoopBackpackDefinitionId.ToString(),
			*FullSystemLoopBackpackId.ToString(
				EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopCoreDefinitionId.ToString(),
			*FullSystemLoopCoreId.ToString(
				EGuidFormats::DigitsWithHyphens),
			Snapshot.SaveGeneration,
			FullSystemLoopAcceptedCommitCount,
			Snapshot.ShopStock.Generation);
		FPlatformMisc::RequestExitWithStatus(false, 0);
		return;
	}
	if (FullSystemLoopAutomationStep == 4)
	{
		CompleteTrainingMissionAndEnterExit(0);
		FullSystemLoopAutomationStep = 5;
		ScheduleFullSystemAutomation(0.35f);
		return;
	}
	if (FullSystemLoopAutomationStep == 5)
	{
		const Fdemo_mapProfileSessionSnapshot BeforeTrade = Session->GetSnapshot();
		if (BeforeTrade.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
			|| BeforeTrade.LastTerminalReason != Edemo_mapRunEndReason::Extraction
			|| BeforeTrade.PersistentSpiritStones != 20
			|| BeforeTrade.RiskSpiritStones != 0)
		{
			Fail(TEXT("real Exit extraction did not transfer exact 20 risk currency."));
			return;
		}
		const Fdemo_mapProfileTradeResult Sell =
			ProfilePreparationWidget->RequestSell(FullSystemLoopCoreId);
		if (!Sell.IsCommitted()
			|| Sell.ItemInstanceId != FullSystemLoopCoreId
			|| Sell.ItemDefinitionId != FullSystemLoopCoreDefinitionId
			|| Sell.BalanceBefore != 20
			|| Sell.TotalPrice <= 0
			|| Sell.BalanceAfter
				!= Sell.BalanceBefore + Sell.TotalPrice)
		{
			Fail(TEXT("real Shop did not sell the same generated Core GUID for its actual effective value."));
			return;
		}
		const Fdemo_mapProfileSessionSnapshot AfterSell =
			Session->GetSnapshot();
		Fdemo_mapPersistentShopStockEntry AffordablePill;
		bool bFoundAffordablePill = false;
		for (const Fdemo_mapPersistentShopStockEntry& Entry :
			AfterSell.ShopStock.Entries)
		{
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(
					Entry.Item.ItemDefinitionId);
			if (Entry.State
					!= Edemo_mapPersistentShopStockEntryState::Available
				|| !Definition
				|| Definition->CategoryId
					!= Fdemo_mapItemIds::ConsumableCategory
				|| !Entry.Item.ItemDefinitionId.ToString().StartsWith(
					TEXT("Prototype.Item.Consumable.HealingPill."))
				|| Entry.QuotedBuyValue <= 0
				|| Entry.QuotedBuyValue
					> AfterSell.PersistentSpiritStones)
			{
				continue;
			}
			if (!bFoundAffordablePill
				|| Entry.SlotOrdinal < AffordablePill.SlotOrdinal)
			{
				AffordablePill = Entry;
				bFoundAffordablePill = true;
			}
		}
		if (!bFoundAffordablePill)
		{
			Fail(TEXT("current 12-slot ShopStock has no naturally available affordable Healing Pill."));
			return;
		}
		const Fdemo_mapProfileTradeResult Buy =
			ProfilePreparationWidget->RequestBuy(
				AffordablePill.SlotId);
		FullSystemLoopPillId = Buy.ItemInstanceId;
		FullSystemLoopPillDefinitionId =
			AffordablePill.Item.ItemDefinitionId;
		FullSystemLoopCoreSaleValue = Sell.TotalPrice;
		FullSystemLoopPillBuyValue = Buy.TotalPrice;
		FullSystemLoopExpectedBalance =
			20 + FullSystemLoopCoreSaleValue
				- FullSystemLoopPillBuyValue;
		if (!Buy.IsCommitted()
			|| Buy.ItemInstanceId
				!= AffordablePill.Item.ItemInstanceId
			|| Buy.ItemDefinitionId
				!= AffordablePill.Item.ItemDefinitionId
			|| Buy.UnitPrice != AffordablePill.QuotedBuyValue
			|| Buy.TotalPrice != AffordablePill.QuotedBuyValue
			|| Buy.BalanceBefore != Sell.BalanceAfter
			|| Buy.BalanceAfter
				!= Buy.BalanceBefore - Buy.TotalPrice
			|| Buy.BalanceAfter
				!= FullSystemLoopExpectedBalance
			|| !FullSystemLoopPillId.IsValid()
			|| !ProfilePreparationWidget->SelectEquipment(
				Fdemo_mapItemIds::BackpackSlot, FullSystemLoopBackpackId).IsAccepted()
			|| !ProfilePreparationWidget->SelectMaterial(
				FullSystemLoopPillId, true).IsAccepted()
			|| !Session->SetPreparationHotbarSlot(
				1, FullSystemLoopPillId).IsAccepted())
		{
			Fail(TEXT("real Shop or persistent Preparation Backpack/Pill intent failed."));
			return;
		}
		Ademo_mapPlayerController* Controller = GetDemoController();
		const Fdemo_mapInputBindingResult Remap = Controller
			? Controller->ApplyInputBindingOverride(Fdemo_mapInputActionIds::Interact, EKeys::H)
			: Fdemo_mapInputBindingResult();
		const Fdemo_mapProfileSessionSnapshot Final = Session->GetSnapshot();
		FullSystemLoopExpectedGeneration = Final.SaveGeneration;
		FullSystemLoopExpectedShopGeneration =
			Final.ShopStock.Generation;
		FullSystemLoopAcceptedCommitCount = Final.SaveGeneration;
		const bool bFinalBackpack =
			Final.OrderedPermanentStash.ContainsByPredicate(
				[this](const Fdemo_mapPersistentItemRecord& Item)
				{
					return Item.ItemInstanceId
							== FullSystemLoopBackpackId
						&& Item.ItemDefinitionId
							== FullSystemLoopBackpackDefinitionId;
				});
		const bool bFinalPill =
			Final.OrderedPermanentStash.ContainsByPredicate(
				[this](const Fdemo_mapPersistentItemRecord& Item)
				{
					return Item.ItemInstanceId
							== FullSystemLoopPillId
						&& Item.ItemDefinitionId
							== FullSystemLoopPillDefinitionId;
				});
		const bool bCoreSold =
			!Final.OrderedPermanentStash.ContainsByPredicate(
				[this](const Fdemo_mapPersistentItemRecord& Item)
				{
					return Item.ItemInstanceId
						== FullSystemLoopCoreId;
				});
		if (!Remap.IsSuccess()
			|| Final.PersistentSpiritStones
				!= FullSystemLoopExpectedBalance
			|| Final.LastSettlementId
				!= BeforeTrade.LastSettlementId
			|| Final.ShopStock.Generation
				!= AfterSell.ShopStock.Generation
			|| FullSystemLoopAcceptedCommitCount
				!= Final.SaveGeneration
			|| !bFinalBackpack
			|| !bFinalPill
			|| !bCoreSold
			|| Final.PreparationLayout.BackpackItemInstanceId != FullSystemLoopBackpackId
			|| !Final.PreparationLayout.OrderedRunInventoryItemInstanceIds.Contains(FullSystemLoopPillId)
			|| Final.PreparationLayout.HotbarItemInstanceIds.Num() != 9
			|| Final.PreparationLayout.HotbarItemInstanceIds[0] != FullSystemLoopPillId
			|| Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) != EKeys::H
			|| !WriteFullSystemHandoff())
		{
			Fail(TEXT("final dynamic GUID ownership, balance equation, commit count, Shop generation, layout, Hotbar1, Interact H persistence, terminal identity, or handoff failed."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("FULL_SYSTEM_LOOP_PHASE_A_IDENTITY profile=%s run=%s settlement=%s projection=%s role=%s generated=1 fallback=0 jackpot=miss rare=miss backpack_definition=%s backpack=%s core_definition=%s core_sold=%s pill_definition=%s pill=%s sale_delta=%lld buy_delta=%lld balance=%lld equation=20+%lld-%lld generation=%d accepted_commit_count=%d shop_generation=%d same_guid_take=2 same_guid_buy=1 interact=H."),
			*Final.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*Final.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopRewardProjectionId.ToString(),
			*FullSystemLoopRewardSourceRoleId.ToString(),
			*FullSystemLoopBackpackDefinitionId.ToString(),
			*FullSystemLoopBackpackId.ToString(EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopCoreDefinitionId.ToString(),
			*FullSystemLoopCoreId.ToString(EGuidFormats::DigitsWithHyphens),
			*FullSystemLoopPillDefinitionId.ToString(),
			*FullSystemLoopPillId.ToString(EGuidFormats::DigitsWithHyphens),
			FullSystemLoopCoreSaleValue,
			FullSystemLoopPillBuyValue,
			FullSystemLoopExpectedBalance,
			FullSystemLoopCoreSaleValue,
			FullSystemLoopPillBuyValue,
			Final.SaveGeneration,
			FullSystemLoopAcceptedCommitCount,
			Final.ShopStock.Generation);
		PassAutomation(TEXT("FULL_SYSTEM_LOOP_PHASE_A: PASS."));
		return;
	}
	Fail(TEXT("invalid fresh-phase automation step."));
}

#endif

Fdemo_mapProfileSessionBeginResult
Ademo_mapV3ProgressionManager::BeginPreparedProfileRunFor0909B()
{
	DismissSettlementPresentation(TEXT("0909BPrepareStart"));
	Fdemo_mapProfileSessionBeginResult Result;
	if (!bInitialized
		|| !Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode)
		|| !ProfilePreparationFlow)
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
		Result.Diagnostic = TEXT("0.0.9B deployment coordinator has no ready Profile lifecycle.");
		return Result;
	}
	if (!GetWorld() || !Items.IsValid() || !PlayerPawn.IsValid()
		|| !GetDemoController())
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = TEXT("0.0.9B deployment preflight has no world, item runtime, pawn, or controller.");
		Result.Snapshot = ProfilePreparationFlow->GetSession()
			? ProfilePreparationFlow->GetSession()->GetSnapshot()
			: Fdemo_mapProfileSessionSnapshot();
		return Result;
	}

	// This creates the formal Code A Run. M01 activation and the P6 observer
	// remain intentionally outside this method, under the I1 coordinator.
	Result = ProfilePreparationFlow->StartPreparedRunDirect();
	UE_LOG(Logdemo_map, Log,
		TEXT("I1_RUN_COORDINATOR Event=CodeARunPrepared Status=%d OwnerId=%s RunId=%s Diagnostic=%s"),
		static_cast<int32>(Result.Status),
		*Result.Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.Diagnostic);
	return Result;
}

bool Ademo_mapV3ProgressionManager::ActivatePreparedProfileWorldFor0909B(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (bProfileWorldActive)
	{
		OutDiagnostic = TEXT("M01 world is already active for the current prepared Run.");
		return true;
	}
	if (!ProfilePreparationFlow
		|| ProfilePreparationFlow->GetPhase()
			!= Edemo_mapProfilePreparationFlowPhase::RunActive
		|| !Items.IsValid())
	{
		OutDiagnostic = TEXT("M01 activation requires one matching active Code A Run.");
		return false;
	}

	Items->BeginWorld(GetWorld());
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	if (!Mode || !Mode->ActivateV3MissionContentForRun() || !InitializeWorldContent())
	{
		OutDiagnostic = TEXT("Authored M01 activation did not produce the required world projections.");
		return false;
	}

	if (SpiritStonePickup.IsValid())
	{
		SpiritStonePickup->Destroy();
		SpiritStonePickup.Reset();
	}
	bProfileWorldActive = true;
	bSettlementPending = false;
	OutDiagnostic = TEXT("Authored M01 world is active and awaits coordinator input restoration.");
	UE_LOG(Logdemo_map, Log,
		TEXT("I1_M01_ADAPTER Event=WorldActivated RunId=%s"),
		*ProfilePreparationFlow->GetStartedRunId().ToString(EGuidFormats::DigitsWithHyphens));
	return true;
}

bool Ademo_mapV3ProgressionManager::RollbackPreparedProfileRunFor0909B(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ProfilePreparationFlow)
	{
		OutDiagnostic = TEXT("No Profile lifecycle exists for technical-start rollback.");
		return false;
	}
	const Fdemo_mapProfileSessionSettlementResult Rollback =
		ProfilePreparationFlow->CancelActiveRunForActivationFailure();
	DeactivateProfileWorld();
	bSettlementPending = false;
	const Fdemo_mapProfileSessionSnapshot Snapshot =
		ProfilePreparationFlow->GetPresentationSnapshot();
	const bool bTechnicalRuntimeRollback =
		Rollback.Status
			== Edemo_mapProfileSessionSettlementStatus::RuntimeRollbackReady;
	const bool bAtSectReady =
		(Rollback.IsDurablySettled() || bTechnicalRuntimeRollback)
		&& Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
		&& !Snapshot.ActiveRunId.IsValid();
	OutDiagnostic = FString::Printf(
		TEXT("Activation rollback status=%d ready=%d diagnostic=%s"),
		static_cast<int32>(Rollback.Status), bAtSectReady ? 1 : 0,
		*Rollback.Diagnostic);
	if (bAtSectReady)
	{
		UE_LOG(Logdemo_map, Log,
			TEXT("I1_RUN_COORDINATOR Event=TechnicalRollback OwnerId=%s RunId=%s %s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*OutDiagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("I1_RUN_COORDINATOR Event=TechnicalRollback OwnerId=%s RunId=%s %s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*OutDiagnostic);
	}
	return bAtSectReady;
}

void Ademo_mapV3ProgressionManager::ObserveCodeBRunAfter0909BActivation(
	const Fdemo_mapProfileSessionSnapshot& Snapshot)
{
	ObserveCodeBRunAfterActivation(Snapshot);
}

void Ademo_mapV3ProgressionManager::Set0909BOutOfRaidCloseCallback(
	TFunction<void()>&& InCallback)
{
	OutOfRaidCloseOverride = MoveTemp(InCallback);
}

Fdemo_mapProfileSessionBeginResult Ademo_mapV3ProgressionManager::StartPreparedProfileRun()
{
	// Start Run is a hard presentation boundary. This also releases a stale
	// settlement focus/input lock before any new Runtime authority is committed.
	DismissSettlementPresentation(TEXT("StartPreparedProfileRun"));
	Fdemo_mapProfileSessionBeginResult Result;
	if (!Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode)
		|| !ProfilePreparationFlow)
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Profile lifecycle is not available for Start Run.");
		return Result;
	}
	// A prepared transaction must not be committed until the local world can
	// actually receive it.  This closes the stale-UI/input path without creating
	// any player terminal event for a local activation problem.
	if (!GetWorld() || !Items.IsValid() || !PlayerPawn.IsValid()
		|| !GetDemoController())
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = TEXT("Start Run was rejected before commit because the local world, player, or controller is unavailable.");
		ShowSectNavigation();
		return Result;
	}
	// The old preparation widget remains only for legacy automation. Product
	// Start Run is an atomic ShanmenItems lifecycle launched from the teleport page.
	HideProfilePreparation();
	Result = ProfilePreparationFlow->StartPreparedRunDirect();
#if !UE_BUILD_SHIPPING
	if ((bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace) && Result.IsRunActive())
	{
		LogP6ProductStartBridgeR2Trace(
			TEXT("CodeAStartRunRequested"), Result.Snapshot, nullptr, nullptr,
			ECodeBRunInventoryBridgeStatus::StorageFailure,
			TEXT("Route=StartPreparedProfileRunFromSect>StartPreparedProfileRun>ProfileFlow.AtomicAuthorityStart"));
	}
#endif
	if (!Result.IsRunActive())
	{
		ShowSectNavigation();
		return Result;
	}
	if (!ActivatePreparedProfileWorld())
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = TEXT("V3 world activation failed and was rolled back as a technical ActivationFailure, not a player Abandon.");
		Result.Snapshot = ProfilePreparationFlow->GetPresentationSnapshot();
	}
	else if (Result.Snapshot.ProfileId.IsValid() && Result.Snapshot.ActiveRunId.IsValid())
	{
#if !UE_BUILD_SHIPPING
		if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
		{
			LogP6ProductStartBridgeR2Trace(
				TEXT("CodeAWorldActivated"), Result.Snapshot, nullptr, nullptr,
				ECodeBRunInventoryBridgeStatus::StorageFailure,
				TEXT("ActivatePreparedProfileWorld=Success"));
		}
#endif
		ObserveCodeBRunAfterActivation(Result.Snapshot);
	}
#if !UE_BUILD_SHIPPING
	if ((bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace) && Result.IsRunActive())
	{
		LogP6ProductStartBridgeR2Trace(
			TEXT("CodeAStartRunReturned"), Result.Snapshot, nullptr, nullptr,
			P6ProductLastBridge.Status,
			TEXT("Start Run result returned after post-activation observer; bridge result is audit-only."));
	}
#endif
	return Result;
}

void Ademo_mapV3ProgressionManager::ObserveCodeBRunAfterActivation(
	const Fdemo_mapProfileSessionSnapshot& Snapshot)
{
	if (!ProfilePreparationFlow || !Snapshot.ProfileId.IsValid()
		|| !Snapshot.ActiveRunId.IsValid())
	{
		return;
	}
	if (ProfilePreparationFlow->UsesShanmenItemLifecycle())
	{
		UE_LOG(LogTemp, Display,
			TEXT("Shanmen.RunAuthority Event=PostActivationNoLegacyWrite OwnerId=%s RunInstanceId=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens));
		return;
	}
	// This observer is deliberately post-success and one-way: a refusal or
	// storage fault is audit-only and cannot alter Code A's Run result, player,
	// world, old inventory, or Runtime authority.
#if !UE_BUILD_SHIPPING
	FCodeBOutOfRaidInventoryRecord BeforeRecord;
	FString BeforeError;
	const bool bHasBeforeRecord = (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
		&& FCodeBOutOfRaidProfileStore::TryReadRecordForLifecycleAutomation(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, BeforeRecord, &BeforeError);
	if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
	{
		LogP6ProductStartBridgeR2Trace(
			TEXT("CodeBObserverDelivered"), Snapshot,
			bHasBeforeRecord ? &BeforeRecord : nullptr, nullptr,
			ECodeBRunInventoryBridgeStatus::StorageFailure,
			FString::Printf(TEXT("StableIdentity=1; PreBridgeRecord=%s; %s"),
				bHasBeforeRecord ? TEXT("Present") : TEXT("Absent"), *BeforeError));
	}
#endif
	FCodeBRunInventoryRecoveryContext RecoveryContext;
	if (ProfilePreparationFlow->GetRecoveredAbandonRunId().IsValid())
	{
		RecoveryContext.RecoveredAbandonRunId = ProfilePreparationFlow->GetRecoveredAbandonRunId();
		RecoveryContext.CodeATerminalCause = TEXT("RecoveredAbandon");
#if !UE_BUILD_SHIPPING
		if (bP6ProductStartBridgeR3Trace)
		{
			LogP6ProductStartBridgeR2Trace(
				TEXT("CodeARecoveredAbandonContext"), Snapshot,
				bHasBeforeRecord ? &BeforeRecord : nullptr, nullptr,
				ECodeBRunInventoryBridgeStatus::StorageFailure,
				FString::Printf(TEXT("RecoveredRunId=%s; ReadOnlyContext=1"),
					*RecoveryContext.RecoveredAbandonRunId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
#endif
	}
	const FCodeBRunInventoryBridgeResult Bridge =
		FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(
			ProfilePreparationFlow->GetStorageRoot(),
			Snapshot.ProfileId,
			Snapshot.ActiveRunId,
			RecoveryContext);
#if !UE_BUILD_SHIPPING
	if (bP6ProductStartBridgeAutomation)
	{
		bP6ProductBridgeObserved = true;
		P6ProductLastBridge = Bridge;
	}
	if (bP6ProductStartBridgeR2Trace || bP6ProductStartBridgeR3Trace)
	{
		FCodeBOutOfRaidInventoryRecord AfterRecord;
		FString AfterError;
		const bool bHasAfterRecord =
			FCodeBOutOfRaidProfileStore::TryReadRecordForLifecycleAutomation(
				ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, AfterRecord, &AfterError);
		LogP6ProductStartBridgeR2Trace(
			TEXT("CodeBBridgeFinal"), Snapshot,
			bHasBeforeRecord ? &BeforeRecord : nullptr,
			bHasAfterRecord ? &AfterRecord : nullptr, Bridge.Status,
			FString::Printf(TEXT("Diagnostic=%s; PostBridgeRecord=%s; %s"),
				*Bridge.Diagnostic, bHasAfterRecord ? TEXT("Present") : TEXT("Absent"), *AfterError));
		if (bP6ProductStartBridgeR3Trace && bHasAfterRecord
			&& AfterRecord.bHasActiveRunInventorySession
			&& !AfterRecord.ActiveRunInventorySession.Receipt.RecoveryRebindHistory.IsEmpty())
		{
			const FCodeBRunInventoryRecoveryRebind& Rebind = AfterRecord.ActiveRunInventorySession.Receipt.RecoveryRebindHistory.Last();
			LogP6ProductStartBridgeR2Trace(
				TEXT("P6PreparedRebind"), Snapshot,
				bHasBeforeRecord ? &BeforeRecord : nullptr, &AfterRecord, Bridge.Status,
				FString::Printf(TEXT("ReceiptId=%s; OriginRunId=%s; OldRunId=%s; NewRunId=%s; RecoveryCount=%d; PayloadDigest=%s"),
					*AfterRecord.ActiveRunInventorySession.Receipt.ReceiptId.ToString(EGuidFormats::DigitsWithHyphens),
					*AfterRecord.ActiveRunInventorySession.Receipt.OriginRunId.ToString(EGuidFormats::DigitsWithHyphens),
					*Rebind.OldRunId.ToString(EGuidFormats::DigitsWithHyphens),
					*Rebind.NewRunId.ToString(EGuidFormats::DigitsWithHyphens),
					AfterRecord.ActiveRunInventorySession.Receipt.RecoveryRebindHistory.Num(),
					*AfterRecord.ActiveRunInventorySession.Receipt.PayloadDigest));
		}
	}
#endif
	UE_LOG(LogTemp, Display,
		TEXT("CodeB.P6.RunBridge Event=PostActivationObserver Status=%d OwnerId=%s RunInstanceId=%s Diagnostic=%s"),
		static_cast<int32>(Bridge.Status),
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*Bridge.Diagnostic);
}

Fdemo_mapProfileSessionBeginResult Ademo_mapV3ProgressionManager::StartPreparedProfileRunFromSect()
{
	// Teleport owns map selection; it does not create the retired preparation UI.
	return StartPreparedProfileRun();
}

Fdemo_mapProfileSessionSettlementResult Ademo_mapV3ProgressionManager::RetryPendingProfileSettlement()
{
	Fdemo_mapProfileSessionSettlementResult Result;
	if (!ProfilePreparationFlow)
	{
		Result.Status = Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement;
		Result.Diagnostic = TEXT("Profile Preparation lifecycle has no pending Settlement.");
		return Result;
	}
	Result = ProfilePreparationFlow->RetryPendingSettlement();
	if (Result.IsDurablySettled())
	{
		bSettlementPending = false;
	}
	ShowSectNavigation();
	return Result;
}

bool Ademo_mapV3ProgressionManager::ActivatePreparedProfileWorld()
{
	if (bProfileWorldActive)
	{
		return true;
	}
	if (!ProfilePreparationFlow
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive
		|| !Items.IsValid())
	{
		return false;
	}
	DismissSettlementPresentation(TEXT("ActivatePreparedProfileWorld"));
	Items->BeginWorld(GetWorld());
	Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (!Mode || !Mode->ActivateV3MissionContentForRun() || !InitializeWorldContent())
	{
		const Fdemo_mapProfileSessionSettlementResult RollbackResult = ProfilePreparationFlow->CancelActiveRunForActivationFailure();
		UE_LOG(Logdemo_map, Error, TEXT("PROFILE_NORMAL_STARTUP: world activation failed; activation_rollback_status=%d diagnostic=%s."), static_cast<int32>(RollbackResult.Status), *RollbackResult.Diagnostic);
		DeactivateProfileWorld();
		ShowSectNavigation();
		return false;
	}
	if (SpiritStonePickup.IsValid())
	{
		SpiritStonePickup->Destroy();
		SpiritStonePickup.Reset();
	}
	if (!Mode->IsM01ExpeditionMap())
	{
		const FVector PickupLocation = PlayerPawn.IsValid()
			? PlayerPawn->GetActorLocation() + PlayerPawn->GetActorForwardVector().GetSafeNormal2D() * 240.0f + FVector(0, 0, 40)
			: FVector(240, 0, 80);
		FActorSpawnParameters PickupSpawn;
		PickupSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpiritStonePickup = GetWorld()->SpawnActor<Ademo_mapSpiritStonePickup>(
			Ademo_mapSpiritStonePickup::StaticClass(), PickupLocation, FRotator::ZeroRotator, PickupSpawn);
		if (!SpiritStonePickup.IsValid())
		{
			const Fdemo_mapProfileSessionSettlementResult RollbackResult = ProfilePreparationFlow->CancelActiveRunForActivationFailure();
			UE_LOG(Logdemo_map, Error, TEXT("PROFILE_NORMAL_STARTUP: fixed Spirit Stone spawn failed; activation_rollback_status=%d."), static_cast<int32>(RollbackResult.Status));
			DeactivateProfileWorld();
			ShowSectNavigation();
			return false;
		}
	}
	else
	{
		UE_LOG(Logdemo_map, Log, TEXT("M01_CONTENT_ISOLATION: fixed Spirit Stone pickup suppressed."));
	}
	bProfileWorldActive = true;
	bSettlementPending = false;
	HideProfilePreparation();
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !Controller->RestoreGameplayControlForNewRun())
	{
		const Fdemo_mapProfileSessionSettlementResult RollbackResult = ProfilePreparationFlow->CancelActiveRunForActivationFailure();
		UE_LOG(Logdemo_map, Error, TEXT("PROFILE_NORMAL_STARTUP: gameplay input activation failed; activation_rollback_status=%d diagnostic=%s."), static_cast<int32>(RollbackResult.Status), *RollbackResult.Diagnostic);
		DeactivateProfileWorld();
		ShowSectNavigation();
		return false;
	}
	UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP: prepared Runtime and V3 world activated run=%s."), *ProfilePreparationFlow->GetStartedRunId().ToString(EGuidFormats::DigitsWithHyphens));
	return true;
}

bool Ademo_mapV3ProgressionManager::IsCodeBQuickUseDeliveryLegal(
	Fdemo_mapProfileSessionSnapshot& OutSnapshot,
	Udemo_mapPlayerHealthComponent*& OutHealth) const
{
	OutSnapshot = Fdemo_mapProfileSessionSnapshot();
	OutHealth = nullptr;
	if (!bInitialized || !bProfileWorldActive || bSettlementPending || bSearchContainerOpen
		|| IsInventoryOpen() || !ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return false;
	}
	OutSnapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	APawn* Pawn = PlayerPawn.Get();
	OutHealth = Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	return OutSnapshot.SessionState == Edemo_mapProfileSessionState::RunActive
		&& OutSnapshot.ProfileId.IsValid() && OutSnapshot.ActiveRunId.IsValid()
		&& OutHealth != nullptr && !OutHealth->IsDefeated();
}

bool Ademo_mapV3ProgressionManager::DeliverPendingCodeBQuickUseReceipts()
{
	Fdemo_mapProfileSessionSnapshot Snapshot;
	Udemo_mapPlayerHealthComponent* Health = nullptr;
	if (!IsCodeBQuickUseDeliveryLegal(Snapshot, Health)) return false;
	FCodeBOutOfRaidProfileStore Store(ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId);
	TArray<FCodeBQuickUseReceipt> Pending;
	FString Error;
	if (!Store.TryGetMatchedActiveRunPendingQuickUseReceipts(Snapshot.ActiveRunId, Pending, &Error))
	{
		return false;
	}
	for (const FCodeBQuickUseReceipt& Receipt : Pending)
	{
		if (Receipt.EffectKind != demo_map_code_b::ECodeBQuickUseEffectKind::RestoreHealth)
		{
			return false;
		}
		bool bAlreadyProcessed = false;
		if (!Health->ApplyRestoreHealthReceipt(Receipt.ReceiptId, Receipt.RestoreAmount, bAlreadyProcessed)
			|| !Store.AcknowledgeMatchedActiveRunQuickUseReceipt(Snapshot.ActiveRunId, Receipt.ReceiptId, &Error))
		{
			return false;
		}
	}
	return true;
}

bool Ademo_mapV3ProgressionManager::RequestUseBoundCodeBQuickSlot(const int32 SlotIndex)
{
	Fdemo_mapProfileSessionSnapshot Snapshot;
	Udemo_mapPlayerHealthComponent* Health = nullptr;
	if (SlotIndex < 1 || SlotIndex > FCodeBHotbarBindings::SlotCount
		|| !IsCodeBQuickUseDeliveryLegal(Snapshot, Health)
		|| Health->GetCurrentHealth() >= Health->GetMaxHealth())
	{
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId);
	FCodeBQuickUseReceipt Receipt;
	FString Error;
	if (!Store.UseMatchedActiveRunBoundQuickSlot(Snapshot.ActiveRunId, SlotIndex, Receipt, &Error))
	{
		return false;
	}
	return DeliverPendingCodeBQuickUseReceipts();
}

bool Ademo_mapV3ProgressionManager::RequestUseBoundQuickSlot(
	const int32 SlotIndex)
{
	if (ProfilePreparationFlow
		&& ProfilePreparationFlow->UsesShanmenItemLifecycle())
	{
		if (!bInitialized || !bProfileWorldActive || bSettlementPending
			|| bSearchContainerOpen || IsInventoryOpen()
			|| ProfilePreparationFlow->GetPhase()
				!= Edemo_mapProfilePreparationFlowPhase::RunActive)
		{
			return false;
		}
		FString Diagnostic;
		const bool bUsed = ProfilePreparationFlow->UseActiveRunHotbarSlot(
			SlotIndex, true, &Diagnostic);
		if (!bUsed)
		{
			UE_LOG(
				Logdemo_map, Warning,
				TEXT("SHANMEN_RUN_ITEM_USE: slot=%d rejected: %s"),
				SlotIndex, *Diagnostic);
		}
		return bUsed;
	}
	return RequestUseBoundCodeBQuickSlot(SlotIndex);
}

Fdemo_mapItemUseResult Ademo_mapV3ProgressionManager::RequestUseInventoryItem(
	const FGuid ItemInstanceId)
{
	auto Reject = [ItemInstanceId](
		const Edemo_mapItemUseStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapItemUseResult Result;
		Result.Status = Status;
		Result.ItemInstanceId = ItemInstanceId;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!ItemInstanceId.IsValid())
	{
		return Reject(
			Edemo_mapItemUseStatus::StaleBinding,
			TEXT("Inventory use requires one valid ItemInstanceId."));
	}
	if (ProfilePreparationFlow
		&& ProfilePreparationFlow->UsesShanmenItemLifecycle())
	{
		if (!bInitialized || !bProfileWorldActive || bSettlementPending
			|| bSearchContainerOpen
			|| ProfilePreparationFlow->GetPhase()
				!= Edemo_mapProfilePreparationFlowPhase::RunActive)
		{
			return Reject(
				Edemo_mapItemUseStatus::InputLocked,
				TEXT("Inventory item use is unavailable outside the active Shanmen product Run."));
		}
		Fdemo_mapItemUseResult Result =
			ProfilePreparationFlow->UseActiveRunInventoryItem(
				ItemInstanceId, true);
		if (!Result.IsSuccess())
		{
			UE_LOG(
				Logdemo_map, Warning,
				TEXT("SHANMEN_RUN_INVENTORY_ITEM_USE: item=%s rejected: %s"),
				*ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
				*Result.Diagnostic);
		}
		return Result;
	}
	Udemo_mapItemSubsystem* Runtime = Items.Get();
	return Runtime
		? Runtime->UseInventoryItem(ItemInstanceId, true)
		: Reject(
			Edemo_mapItemUseStatus::PlayerUnavailable,
			TEXT("Inventory item use requires the Runtime item subsystem."));
}

void Ademo_mapV3ProgressionManager::ShowProfilePreparation()
{
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !ProfilePreparationFlow || !ProfilePreparationFlow->GetSession())
	{
		return;
	}
	if (SectNavigationWidget)
	{
		SectNavigationWidget->RemoveFromParent();
	}
	if (!ProfilePreparationWidget)
	{
		ProfilePreparationWidget = CreateWidget<Udemo_mapProfilePreparationWidget>(
			Controller,
			Udemo_mapProfilePreparationWidget::StaticClass());
	}
	if (ProfilePreparationWidget)
	{
		ProfilePreparationWidget->InitializeForLifecycle(ProfilePreparationFlow->GetSession(), this);
		ProfilePreparationWidget->RefreshFromSession();
		if (!ProfilePreparationWidget->IsInViewport())
		{
			ProfilePreparationWidget->AddToViewport(600);
		}
		Controller->BeginProfilePreparationInputLock(ProfilePreparationWidget);
	}
}

void Ademo_mapV3ProgressionManager::ShowSectNavigation()
{
	DismissSettlementPresentation(TEXT("SectNavigation"));
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !ProfilePreparationFlow || !ProfilePreparationFlow->GetSession())
	{
		return;
	}
	if (ProfilePreparationWidget)
	{
		ProfilePreparationWidget->RemoveFromParent();
	}
	if (!SectNavigationWidget)
	{
		SectNavigationWidget = CreateWidget<Udemo_mapSectNavigationWidget>(
			Controller,
			Udemo_mapSectNavigationWidget::StaticClass());
	}
	if (SectNavigationWidget)
	{
		SectNavigationWidget->InitializeForLifecycle(
			ProfilePreparationFlow->GetSession(),
			this);
		SectNavigationWidget->RefreshFromSession();
		if (!SectNavigationWidget->IsInViewport())
		{
			SectNavigationWidget->AddToViewport(600);
		}
		Controller->BeginProfilePreparationInputLock(SectNavigationWidget);
	}
}

void Ademo_mapV3ProgressionManager::OpenProfilePreparationFromSect(bool bReturnToTeleport)
{
	DismissSettlementPresentation(TEXT("OpenProfilePreparationFromSect"));
	bReturnToTeleportAfterPreparation = bReturnToTeleport;
	ShowProfilePreparation();
}

bool Ademo_mapV3ProgressionManager::IsCodeBOutOfRaidInventoryEntryAvailable() const
{
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession())
	{
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	return Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
		&& !Snapshot.ActiveRunId.IsValid();
}

bool Ademo_mapV3ProgressionManager::OpenCodeBOutOfRaidInventory(FString& OutFeedback)
{
	OutFeedback.Reset();
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession())
	{
		OutFeedback = TEXT("仓库／人物配置不可用：Profile 会话未初始化。");
		UE_LOG(LogTemp, Warning, TEXT("P5.OutOfRaidEntry Rejected Reason=ProfileSessionUnavailable"));
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	FString RunSessionLockError;
	if (FCodeBOutOfRaidProfileStore::HasActiveRunInventorySession(
		ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, &RunSessionLockError))
	{
		OutFeedback = TEXT("当前 Run 中，返回后再整理");
		UE_LOG(LogTemp, Warning, TEXT("CodeB.P6.OutOfRaidEntry Rejected Reason=ActiveRunInventorySession OwnerId=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens));
		return false;
	}
	if (!RunSessionLockError.IsEmpty())
	{
		OutFeedback = RunSessionLockError;
		UE_LOG(LogTemp, Error, TEXT("CodeB.P6.OutOfRaidEntry Rejected Reason=RunSessionProbeFailed OwnerId=%s Diagnostic=%s"),
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
		return false;
	}
	if (!IsCodeBOutOfRaidInventoryEntryAvailable())
	{
		OutFeedback = TEXT("结束当前 Run 后再整理");
		UE_LOG(LogTemp, Warning, TEXT("P5.OutOfRaidEntry Rejected Reason=RunActiveOrProfileNotReady State=%d ActiveRun=%d"), static_cast<int32>(Snapshot.SessionState), Snapshot.ActiveRunId.IsValid() ? 1 : 0);
		return false;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	if (!Host)
	{
		OutFeedback = TEXT("仓库／人物配置不可用：Code B Profile Host 未启动。");
		UE_LOG(LogTemp, Error, TEXT("P5.OutOfRaidEntry Rejected Reason=ProfileHostUnavailable"));
		return false;
	}
	if (!CodeBOutOfRaidProfileStore.IsValid()
		|| CodeBOutOfRaidProfileStore->GetRecord().OwnerId != Snapshot.ProfileId)
	{
		CodeBOutOfRaidRepository = MakeUnique<demo_map_code_b::FCodeBRepository>();
		CodeBOutOfRaidProfileStore = MakeUnique<FCodeBOutOfRaidProfileStore>(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId);
	}
	if (!CodeBOutOfRaidRepository.IsValid())
	{
		OutFeedback = TEXT("仓库／人物配置不可用：Code B Repository 未建立。");
		UE_LOG(LogTemp, Error, TEXT("P5.OutOfRaidEntry Rejected Reason=RepositoryUnavailable"));
		return false;
	}
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
	const FCodeBOutOfRaidOpenResult OpenResult =
		CodeBOutOfRaidProfileStore->OpenOrMigrate(
			Snapshot, *CodeBOutOfRaidRepository, Layout);
	if (!OpenResult.bSuccess)
	{
		OutFeedback = OpenResult.Diagnostic;
		UE_LOG(LogTemp, Error, TEXT("P5.OutOfRaidEntry Rejected Reason=StoreOpenOrMigrationFailed Diagnostic=%s"), *OutFeedback);
		return false;
	}
	FCodeBHotbarProjection OutOfRaidHotbarProjection;
	FString HotbarError;
	if (!CodeBOutOfRaidProfileStore->TryGetOutOfRaidHotbarProjection(OutOfRaidHotbarProjection, &HotbarError))
	{
		OutFeedback = HotbarError.IsEmpty()
			? TEXT("仓库／人物配置快捷栏投影不可用。") : HotbarError;
		UE_LOG(LogTemp, Error, TEXT("CodeB.P13.OutOfRaidHotbar Rejected OwnerId=%s Diagnostic=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
		return false;
	}
	const TWeakObjectPtr<Ademo_mapV3ProgressionManager> WeakManager(this);
	FCodeBP3HotbarPresentation OutOfRaidHotbarPresentation;
	OutOfRaidHotbarPresentation.OwnerId = Snapshot.ProfileId;
	OutOfRaidHotbarPresentation.Projection = MoveTemp(OutOfRaidHotbarProjection);
	OutOfRaidHotbarPresentation.Refresh = [WeakManager](FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->CodeBOutOfRaidProfileStore.IsValid())
		{
			OutError = TEXT("真实 Profile 快捷栏服务已释放。");
			return false;
		}
		return WeakManager->CodeBOutOfRaidProfileStore->TryGetOutOfRaidHotbarProjection(OutProjection, &OutError);
	};
	OutOfRaidHotbarPresentation.Bind = [WeakManager](
		const FGuid& ItemId, const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->CodeBOutOfRaidProfileStore.IsValid())
		{
			OutError = TEXT("真实 Profile 快捷栏服务已释放。");
			return false;
		}
		return WeakManager->CodeBOutOfRaidProfileStore->BindOutOfRaidHotbarSlot(
			ItemId, SlotIndex, OutProjection, &OutError);
	};
	OutOfRaidHotbarPresentation.Unbind = [WeakManager](
		const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->CodeBOutOfRaidProfileStore.IsValid())
		{
			OutError = TEXT("真实 Profile 快捷栏服务已释放。");
			return false;
		}
		return WeakManager->CodeBOutOfRaidProfileStore->UnbindOutOfRaidHotbarSlot(
			SlotIndex, OutProjection, &OutError);
	};
	FCodeBP3WorkspacePresentation OutOfRaidWorkspace;
	OutOfRaidWorkspace.Context.Scope = demo_map_code_b::ECodeBP3WorkspaceScope::OutOfRaidP5;
	OutOfRaidWorkspace.Context.OwnerId = Snapshot.ProfileId;
	OutOfRaidWorkspace.Context.SessionRevision = CodeBOutOfRaidProfileStore->GetPersistentRevision();
	OutOfRaidWorkspace.Context.WriteGate = demo_map_code_b::ECodeBP3WorkspaceWriteGate::AtSect;
	OutOfRaidWorkspace.Context.PlayerPaneId = FName(TEXT("OutOfRaidP5.PlayerLoadout"));
	OutOfRaidWorkspace.Context.TargetPaneId = FName(TEXT("OutOfRaidP5.SectWarehouse"));
	OutOfRaidWorkspace.ResolveWriteGate = [WeakManager]()
	{
		if (!WeakManager.IsValid() || !WeakManager->GetWorld())
		{
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::Unavailable;
		}
		const Ademo_mapGameMode* GameMode = WeakManager->GetWorld()->GetAuthGameMode<Ademo_mapGameMode>();
		const Ademo_map0909BFrameworkHost* Framework = GameMode
			? GameMode->Get0909BFrameworkHost() : nullptr;
		if (!Framework)
		{
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::Unavailable;
		}
		switch (Framework->GetTopState())
		{
		case Edemo_map0909BTopState::AtSect:
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::AtSect;
		case Edemo_map0909BTopState::PreparingStart:
		case Edemo_map0909BTopState::ActivatingWorld:
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::StartAttemptPending;
		case Edemo_map0909BTopState::InRun:
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::InRun;
		case Edemo_map0909BTopState::ResolvingTerminal:
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::ResolvingTerminal;
		default:
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::Unavailable;
		}
	};
	OutOfRaidWorkspace.ResolveSessionRevision = [WeakManager]()
	{
		return WeakManager.IsValid() && WeakManager->CodeBOutOfRaidProfileStore.IsValid()
			? WeakManager->CodeBOutOfRaidProfileStore->GetPersistentRevision()
			: INDEX_NONE;
	};
	const bool bOpened = Host->OpenProfilePage(
		*CodeBOutOfRaidRepository,
		Layout,
		[this](const demo_map_code_b::FCodeBSnapshot& PersistedSnapshot,
			const demo_map_code_b::FCodeBP2Command&, FString& OutError)
		{
			return CodeBOutOfRaidProfileStore.IsValid()
				&& CodeBOutOfRaidProfileStore->CommitAcceptedSnapshot(PersistedSnapshot, &OutError);
		},
		[WeakManager]()
		{
			if (WeakManager.IsValid())
			{
				if (WeakManager->OutOfRaidCloseOverride)
				{
					WeakManager->OutOfRaidCloseOverride();
				}
				else
				{
					WeakManager->RestoreSectNavigationAfterCodeBOutOfRaidClose();
				}
			}
		},
		false,
		nullptr,
		nullptr,
		&OutOfRaidHotbarPresentation,
		nullptr,
		nullptr,
		&OutOfRaidWorkspace);
	OutFeedback = bOpened
		? OpenResult.Diagnostic
		: TEXT("仓库／人物配置无法创建真实 Code B 页面。");
	if (bOpened)
	{
		UE_LOG(LogTemp, Display, TEXT("P5.OutOfRaidEntry Result=Opened OwnerId=%s Diagnostic=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("P5.OutOfRaidEntry Result=Rejected OwnerId=%s Diagnostic=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
	}
	return bOpened;
}

void Ademo_mapV3ProgressionManager::RestoreSectNavigationAfterCodeBOutOfRaidClose()
{
	if (SectNavigationWidget)
	{
		if (Ademo_mapPlayerController* Controller = GetDemoController())
		{
			Controller->BeginProfilePreparationInputLock(SectNavigationWidget);
		}
	}
}

bool Ademo_mapV3ProgressionManager::OpenCodeBActiveRunInventory(FString& OutFeedback)
{
	OutFeedback.Reset();
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		OutFeedback = TEXT("Code B 活动背包未接入当前 Run。");
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	const FGuid ExpectedRunId = ProfilePreparationFlow->GetStartedRunId();
	if (!Snapshot.ProfileId.IsValid() || !ExpectedRunId.IsValid() || Snapshot.ActiveRunId != ExpectedRunId)
	{
		OutFeedback = TEXT("Code B 活动背包身份与当前 Run 不匹配。");
		UE_LOG(LogTemp, Warning, TEXT("CodeB.P7.Entry Rejected Reason=RunIdentityMismatch OwnerId=%s SnapshotRunId=%s ExpectedRunId=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*ExpectedRunId.ToString(EGuidFormats::DigitsWithHyphens));
		return false;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	if (!Host || Host->IsHostEnabled())
	{
		OutFeedback = TEXT("Code B 活动背包 Host 当前不可用。");
		return false;
	}
	if (!CodeBActiveRunInventoryStore.IsValid()
		|| CodeBActiveRunInventoryOwnerId != Snapshot.ProfileId)
	{
		CodeBActiveRunInventoryStore = MakeUnique<FCodeBOutOfRaidProfileStore>(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId);
		CodeBActiveRunInventoryOwnerId = Snapshot.ProfileId;
	}
	FCodeBRunInventorySession ActiveSession;
	if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(ExpectedRunId, ActiveSession, &OutFeedback))
	{
		UE_LOG(LogTemp, Verbose, TEXT("CodeB.P7.Entry NonBlocking Reason=NoMatchedSession OwnerId=%s RunId=%s Diagnostic=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*ExpectedRunId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
		return false;
	}

	CodeBActiveRunInventoryRepository = MakeUnique<demo_map_code_b::FCodeBRepository>();
	if (!CodeBActiveRunInventoryRepository->LoadPersistedSnapshot(ActiveSession.RepositorySnapshot, &OutFeedback))
	{
		CodeBActiveRunInventoryRepository.Reset();
		UE_LOG(LogTemp, Error, TEXT("CodeB.P7.Entry Rejected Reason=InvalidMatchedSession OwnerId=%s RunId=%s Diagnostic=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*ExpectedRunId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
		return false;
	}
	FCodeBHotbarProjection ActiveRunHotbarProjection;
	FString HotbarError;
	if (!CodeBActiveRunInventoryStore->TryGetMatchedActiveRunHotbarProjection(
		ExpectedRunId, ActiveRunHotbarProjection, &HotbarError))
	{
		CodeBActiveRunInventoryRepository.Reset();
		OutFeedback = HotbarError.IsEmpty()
			? TEXT("Code B 活动背包快捷栏投影不可用。") : HotbarError;
		UE_LOG(LogTemp, Error, TEXT("CodeB.P13.ActiveRunHotbar Rejected OwnerId=%s RunId=%s Diagnostic=%s"),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*ExpectedRunId.ToString(EGuidFormats::DigitsWithHyphens), *OutFeedback);
		return false;
	}
	CodeBActiveRunInventoryRunId = ExpectedRunId;
	CodeBActiveRunInventoryExpectedP6Revision = ActiveSession.RepositorySnapshot.Revision;
	const TWeakObjectPtr<Ademo_mapV3ProgressionManager> WeakManager(this);
	FCodeBP3HotbarPresentation ActiveRunHotbarPresentation;
	ActiveRunHotbarPresentation.OwnerId = Snapshot.ProfileId;
	ActiveRunHotbarPresentation.RunInstanceId = ExpectedRunId;
	ActiveRunHotbarPresentation.Projection = MoveTemp(ActiveRunHotbarProjection);
	ActiveRunHotbarPresentation.Refresh = [WeakManager, ExpectedRunId](
		FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->CodeBActiveRunInventoryStore.IsValid()
			|| WeakManager->CodeBActiveRunInventoryRunId != ExpectedRunId)
		{
			OutError = TEXT("活动 Run 快捷栏服务已释放或身份不匹配。");
			return false;
		}
		return WeakManager->CodeBActiveRunInventoryStore->TryGetMatchedActiveRunHotbarProjection(
			ExpectedRunId, OutProjection, &OutError);
	};
	ActiveRunHotbarPresentation.Bind = [WeakManager, ExpectedRunId](
		const FGuid& ItemId, const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->CodeBActiveRunInventoryStore.IsValid()
			|| WeakManager->CodeBActiveRunInventoryRunId != ExpectedRunId)
		{
			OutError = TEXT("活动 Run 快捷栏服务已释放或身份不匹配。");
			return false;
		}
		return WeakManager->CodeBActiveRunInventoryStore->BindMatchedActiveRunHotbarSlot(
			ExpectedRunId, ItemId, SlotIndex, OutProjection, &OutError);
	};
	ActiveRunHotbarPresentation.Unbind = [WeakManager, ExpectedRunId](
		const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->CodeBActiveRunInventoryStore.IsValid()
			|| WeakManager->CodeBActiveRunInventoryRunId != ExpectedRunId)
		{
			OutError = TEXT("活动 Run 快捷栏服务已释放或身份不匹配。");
			return false;
		}
		return WeakManager->CodeBActiveRunInventoryStore->UnbindMatchedActiveRunHotbarSlot(
			ExpectedRunId, SlotIndex, OutProjection, &OutError);
	};
	FCodeBP3GroundDropPresentation GroundDropPresentation;
	GroundDropPresentation.RequestDrop = [WeakManager](
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutError)
	{
		return WeakManager.IsValid() && WeakManager->RequestCodeBGroundDrop(Payload, OutError);
	};
	const bool bOpened = Host->OpenProfilePage(
		*CodeBActiveRunInventoryRepository,
		ActiveSession.Layout,
		[this, ExpectedRunId, ExpectedOwnerId = Snapshot.ProfileId](const demo_map_code_b::FCodeBSnapshot& PersistedSnapshot,
			const demo_map_code_b::FCodeBP2Command& AcceptedCommand, FString& OutError)
		{
			FCodeBActivePlayerInteractionCommitRequest Request;
			Request.StorageRoot = ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString();
			Request.OwnerId = ExpectedOwnerId;
			Request.RunInstanceId = ExpectedRunId;
			Request.Store = CodeBActiveRunInventoryStore.Get();
			Request.bTransientScopeCurrent = bCodeBActiveRunInventoryOpen
				&& CodeBActiveRunInventoryRunId == ExpectedRunId;
			FCodeBActivePlayerInteractionCommitResult Result;
			const bool bCommitted = FCodeBRunItemInteractionDomain::CommitAcceptedActivePlayer(
				Request, AcceptedCommand, PersistedSnapshot, Result, OutError);
			if (bCommitted)
			{
				ApplyCodeBRunItemInteractionResult(Result);
			}
			return bCommitted;
		},
		[WeakManager]()
		{
			if (WeakManager.IsValid())
			{
				WeakManager->CloseCodeBActiveRunInventory();
			}
		},
		true,
		nullptr,
		nullptr,
		&ActiveRunHotbarPresentation,
		&GroundDropPresentation);
	if (!bOpened)
	{
		CodeBActiveRunInventoryRepository.Reset();
		CodeBActiveRunInventoryRunId.Invalidate();
		CodeBActiveRunInventoryExpectedP6Revision = INDEX_NONE;
		OutFeedback = TEXT("Code B 活动背包无法创建真实 P3/P4 页面。");
		return false;
	}
	bCodeBActiveRunInventoryOpen = true;
	UE_LOG(LogTemp, Display, TEXT("CodeB.P7.Entry Result=Opened OwnerId=%s RunId=%s"),
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*ExpectedRunId.ToString(EGuidFormats::DigitsWithHyphens));
	return true;
}

bool Ademo_mapV3ProgressionManager::ResolveCodeBWorldDropPlacement(
	FName& OutMapRoute,
	FTransform& OutFloorTransform,
	FString& OutFeedback) const
{
	OutMapRoute = NAME_None;
	OutFloorTransform = FTransform::Identity;
	OutFeedback.Reset();
	UWorld* World = GetWorld();
	APawn* Pawn = PlayerPawn.Get();
	if (!World || !Pawn)
	{
		OutFeedback = TEXT("地面丢弃需要当前玩家与活动地图。");
		return false;
	}
	const FVector Start = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f);
	const FVector End = Pawn->GetActorLocation() - FVector(0.0f, 0.0f, 1400.0f);
	FHitResult FloorHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CodeBWorldDropFloor), false, Pawn);
	if (!World->LineTraceSingleByChannel(FloorHit, Start, End, ECC_Visibility, QueryParams)
		|| !FloorHit.bBlockingHit)
	{
		OutFeedback = TEXT("当前位置没有可确认的地面，未提交丢弃。");
		return false;
	}
	OutMapRoute = FName(*World->GetMapName());
	OutFloorTransform = FTransform(
		FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f),
		FloorHit.ImpactPoint + FVector(0.0f, 0.0f, 12.0f));
	return true;
}

bool Ademo_mapV3ProgressionManager::RequestCodeBGroundDrop(
	const demo_map_code_b::FCodeBP4DragPayload& Payload,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	if (!Payload.IsValid() || !CodeBActiveRunInventoryStore.IsValid()
		|| !CodeBActiveRunInventoryRepository.IsValid() || !CodeBActiveRunInventoryRunId.IsValid())
	{
		OutFeedback = TEXT("活动 Run 地面丢弃服务当前不可用。");
		return false;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	const demo_map_code_b::FCodeBP3InventoryWorkspaceContext* Workspace = Host ? Host->GetWorkspaceContext() : nullptr;
	const bool bCarriesActiveChild = Payload.QuickTransferActivePlayerContainerId.IsValid();
	if ((bCarriesActiveChild && (!Workspace || !Workspace->ActiveDestinationContainerId.IsSet()
			|| Workspace->ActiveDestinationContainerId.GetValue() != Payload.QuickTransferActivePlayerContainerId
			|| Workspace->ActiveDestinationOpenGeneration == 0
			|| Workspace->ActiveDestinationOpenGeneration != Payload.ActivePlayerChildOpenGeneration))
		|| (!bCarriesActiveChild && Payload.ActivePlayerChildOpenGeneration != 0))
	{
		OutFeedback = TEXT("活动 P17 child 已关闭或切换；陈旧拖拽未提交。");
		return false;
	}
	FCodeBRunInventorySession Session;
	if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
		CodeBActiveRunInventoryRunId, Session, &OutFeedback))
	{
		return false;
	}
	const demo_map_code_b::FCodeBItemInstance* SourceItem = Session.RepositorySnapshot.Items.Find(Payload.ItemId);
	const demo_map_code_b::FCodeBContainer* SourceContainer = Session.RepositorySnapshot.Containers.Find(Payload.Source.ContainerId);
	if (Payload.OwnerId != Session.OwnerId || Payload.RunInstanceId != Session.RunInstanceId
		|| Payload.ItemId != Payload.Source.ItemId
		|| !Payload.Source.IsRevealed()
		|| Payload.ExpectedRevision != Session.RepositorySnapshot.Revision
		|| !SourceItem || !SourceContainer
		|| SourceItem->Quantity != Payload.Quantity
		|| SourceItem->ParentContainerId != Payload.Source.ContainerId
		|| SourceItem->SlotIndex != Payload.Source.SlotIndex
		|| !SourceContainer->Slots.IsValidIndex(Payload.Source.SlotIndex)
		|| SourceContainer->Slots[Payload.Source.SlotIndex] != Payload.ItemId)
	{
		OutFeedback = TEXT("地面丢弃只接受当前 P7 已呈现的精确物品格。");
		return false;
	}
	FName MapRoute;
	FTransform FloorTransform;
	if (!ResolveCodeBWorldDropPlacement(MapRoute, FloorTransform, OutFeedback)) return false;
	FCodeBWorldDropProjection Projection;
	if (!CodeBActiveRunInventoryStore->DropMatchedActiveRunWorldDropItem(
		CodeBActiveRunInventoryRunId, Payload.ItemId, Payload.Source.ContainerId,
		Payload.Source.SlotIndex, Payload.ExpectedRevision,
		Payload.bSplitIntent ? Payload.RequestedMergeQuantity : 0,
		Payload.QuickTransferActivePlayerContainerId,
		Payload.ActivePlayerChildOpenGeneration,
		MapRoute, FloorTransform, Projection, &OutFeedback))
	{
		return false;
	}
	FCodeBRunInventorySession Updated;
	if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
		CodeBActiveRunInventoryRunId, Updated, &OutFeedback)
		|| !CodeBActiveRunInventoryRepository->LoadPersistedSnapshot(Updated.RepositorySnapshot, &OutFeedback))
	{
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CodeB.P14.GroundDrop Committed OwnerId=%s RunId=%s WorldDropId=%s ItemId=%s Route=%s"),
		*Projection.OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*Projection.RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*Projection.WorldDropId.ToString(EGuidFormats::DigitsWithHyphens),
		*Projection.ItemId.ToString(EGuidFormats::DigitsWithHyphens), *Projection.MapRoute.ToString());
	RefreshCodeBWorldDropActors();
	return true;
}

bool Ademo_mapV3ProgressionManager::RequestCodeBWorldDropGroundDrop(
	const demo_map_code_b::FCodeBP4DragPayload& Payload,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	const bool bP70Split = Payload.bSplitIntent;
	const bool bP71Reposition = !Payload.bSplitIntent;
	if (!Payload.IsValid()
		|| (bP70Split
			&& Payload.QuantityDraftKind != demo_map_code_b::ECodeBP3QuantityDraftKind::WorldPickup)
		|| (bP71Reposition
			&& (Payload.QuantityDraftKind != demo_map_code_b::ECodeBP3QuantityDraftKind::None
				|| Payload.RequestedMergeQuantity != 0))
		|| Payload.bQuickTransferIntent || !CodeBActiveRunInventoryStore.IsValid()
		|| !CodeBWorldDropRepository.IsValid() || !bCodeBWorldDropOpen
		|| !CodeBWorldDropOwnerId.IsValid() || !CodeBWorldDropRunId.IsValid()
		|| !CodeBWorldDropId.IsValid() || !CodeBWorldDropContainerId.IsValid()
		|| !CodeBWorldDropRootItemId.IsValid() || CodeBWorldDropOrdinal < 1
		|| CodeBWorldDropRecordRevision < 1 || CodeBWorldDropExpectedP6Revision < 1
		|| CodeBWorldDropOpenGeneration == 0 || !ActiveCodeBWorldDrop.IsValid()
		|| Payload.OwnerId != CodeBWorldDropOwnerId || Payload.RunInstanceId != CodeBWorldDropRunId
		|| Payload.WorldDropId != CodeBWorldDropId || Payload.WorldDropOrdinal != CodeBWorldDropOrdinal
		|| Payload.WorldDropRecordRevision != CodeBWorldDropRecordRevision
		|| Payload.WorldDropTargetOpenGeneration != CodeBWorldDropOpenGeneration
		|| Payload.WorldDropMapRoute != CodeBWorldDropMapRoute
		|| Payload.GraphIdentity != CodeBWorldDropId
		|| Payload.Source.ContainerId != CodeBWorldDropContainerId
		|| Payload.Source.SlotIndex != 0 || Payload.Source.ItemId != CodeBWorldDropRootItemId
		|| Payload.ItemId != CodeBWorldDropRootItemId
		|| Payload.ExpectedRevision != CodeBWorldDropExpectedP6Revision
		|| (bP70Split
			&& (Payload.RequestedMergeQuantity < 1 || Payload.RequestedMergeQuantity >= Payload.Quantity)))
	{
		OutFeedback = TEXT("P70/P71 只接受 current exact opened WorldDrop 的 confirmed split 或 normal whole-root re-placement。");
		return false;
	}
	const TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>* RegisteredActor =
		CodeBWorldDropActors.Find(CodeBWorldDropId);
	if (!RegisteredActor || RegisteredActor->Get() != ActiveCodeBWorldDrop.Get())
	{
		OutFeedback = TEXT("P71 exact WorldDrop Actor projection 已失效；未提交。");
		return false;
	}
	FName MapRoute;
	FTransform FloorTransform;
	if (!ResolveCodeBWorldDropPlacement(MapRoute, FloorTransform, OutFeedback)) return false;
	if (MapRoute != CodeBWorldDropMapRoute)
	{
		OutFeedback = TEXT("P71 不允许跨 map route 重新放置。");
		return false;
	}
	if (bP71Reposition)
	{
		FCodeBRunInventorySession BeforeSession;
		if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
			CodeBWorldDropRunId, BeforeSession, &OutFeedback))
		{
			return false;
		}
		const FCodeBWorldDropRecord* BeforeRecord = BeforeSession.WorldDrops.FindByPredicate(
			[this](const FCodeBWorldDropRecord& Value)
			{
				return Value.WorldDropId == CodeBWorldDropId;
			});
		if (!BeforeRecord || BeforeRecord->Ordinal != CodeBWorldDropOrdinal
			|| BeforeRecord->RecordRevision != CodeBWorldDropRecordRevision
			|| BeforeRecord->WorldContainerId != CodeBWorldDropContainerId
			|| BeforeRecord->ItemId != CodeBWorldDropRootItemId
			|| BeforeRecord->MapRoute != CodeBWorldDropMapRoute
			|| !BeforeRecord->FloorTransform.Equals(Payload.WorldDropFloorTransform)
			|| BeforeRecord->FloorTransform.Equals(FloorTransform))
		{
			OutFeedback = TEXT("P71 source record 已变化或规范化后落点相同；零写入。");
			return false;
		}
		FCodeBWorldDropProjection RepositionedProjection;
		if (!CodeBActiveRunInventoryStore->RepositionMatchedActiveRunWorldDropItem(
			CodeBWorldDropRunId, CodeBWorldDropId, CodeBWorldDropOrdinal,
			CodeBWorldDropRecordRevision, CodeBWorldDropExpectedP6Revision,
			CodeBWorldDropRootItemId, Payload.DefinitionId, Payload.DefinitionId,
			Payload.Quantity, Payload.MaxStack, Payload.Level, Payload.Quality,
			Payload.RandomSeed, Payload.LegacyAffixDigest, MapRoute, Payload.WorldDropFloorTransform,
			FloorTransform, RepositionedProjection, &OutFeedback))
		{
			return false;
		}
		FCodeBRunInventorySession ReloadedSession;
		if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
			CodeBWorldDropRunId, ReloadedSession, &OutFeedback)
			|| !CodeBWorldDropRepository->LoadPersistedSnapshot(
				ReloadedSession.RepositorySnapshot, &OutFeedback))
		{
			return false;
		}
		const FCodeBWorldDropRecord* ReloadedRecord = ReloadedSession.WorldDrops.FindByPredicate(
			[this](const FCodeBWorldDropRecord& Value)
			{
				return Value.WorldDropId == CodeBWorldDropId;
			});
		if (!ReloadedRecord || ReloadedSession.WorldDrops.Num() != BeforeSession.WorldDrops.Num()
			|| ReloadedSession.NextWorldDropOrdinal != BeforeSession.NextWorldDropOrdinal
			|| ReloadedSession.RepositorySnapshot != BeforeSession.RepositorySnapshot
			|| ReloadedRecord->WorldContainerId != CodeBWorldDropContainerId
			|| ReloadedRecord->ItemId != CodeBWorldDropRootItemId
			|| ReloadedRecord->Provenance != BeforeRecord->Provenance
			|| ReloadedRecord->RecordRevision != CodeBWorldDropRecordRevision + 1
			|| !ReloadedRecord->FloorTransform.Equals(FloorTransform))
		{
			OutFeedback = TEXT("P71 durable reload 未证明 same-record placement-only 结果。");
			return false;
		}
		CodeBWorldDropRecordRevision = RepositionedProjection.RecordRevision;
		CodeBWorldDropExpectedP6Revision = RepositionedProjection.P6SnapshotRevision;
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
			{
				Host->UpdateWorldDropProjection(RepositionedProjection);
			}
		}
		RefreshCodeBWorldDropActors();
		UE_LOG(LogTemp, Display,
			TEXT("CodeB.P71.GroundReposition Committed WorldDropId=%s ItemId=%s RecordRevision=%d"),
			*RepositionedProjection.WorldDropId.ToString(EGuidFormats::DigitsWithHyphens),
			*RepositionedProjection.ItemId.ToString(EGuidFormats::DigitsWithHyphens),
			RepositionedProjection.RecordRevision);
		return true;
	}
	FCodeBWorldDropProjection SourceProjection;
	FCodeBWorldDropProjection NewProjection;
	if (!CodeBActiveRunInventoryStore->SplitMatchedActiveRunWorldDropItem(
		CodeBWorldDropRunId, CodeBWorldDropId, CodeBWorldDropOrdinal,
		CodeBWorldDropRecordRevision, CodeBWorldDropExpectedP6Revision,
		CodeBWorldDropRootItemId, Payload.RequestedMergeQuantity,
		MapRoute, FloorTransform, SourceProjection, NewProjection, &OutFeedback))
	{
		return false;
	}
	FCodeBRunInventorySession Updated;
	if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
		CodeBWorldDropRunId, Updated, &OutFeedback)
		|| !CodeBWorldDropRepository->LoadPersistedSnapshot(
			Updated.RepositorySnapshot, &OutFeedback))
	{
		return false;
	}
	CodeBWorldDropRecordRevision = SourceProjection.RecordRevision;
	CodeBWorldDropExpectedP6Revision = SourceProjection.P6SnapshotRevision;
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
		{
			Host->UpdateWorldDropProjection(SourceProjection);
		}
	}
	RefreshCodeBWorldDropActors();
	UE_LOG(LogTemp, Display,
		TEXT("CodeB.P70.GroundSplit Committed SourceWorldDropId=%s NewWorldDropId=%s SourceItemId=%s NewItemId=%s Quantity=%d"),
		*SourceProjection.WorldDropId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewProjection.WorldDropId.ToString(EGuidFormats::DigitsWithHyphens),
		*SourceProjection.ItemId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewProjection.ItemId.ToString(EGuidFormats::DigitsWithHyphens),
		Payload.RequestedMergeQuantity);
	return true;
}

bool Ademo_mapV3ProgressionManager::RequestCodeBNormalContainerGroundDrop(
	const demo_map_code_b::FCodeBP4DragPayload& Payload,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	const demo_map_code_b::FCodeBP49NormalContainerSimpleStackGroundDropProof& P49Proof =
		Payload.P49NormalContainerSimpleStackGroundDropProof;
	const demo_map_code_b::FCodeBP64NormalContainerPlayerSimpleStackGroundDropProof& P64Proof =
		Payload.P64NormalContainerPlayerSimpleStackGroundDropProof;
	const demo_map_code_b::FCodeBP65NormalContainerPlayerStandardEquipmentGroundDropProof& P65Proof =
		Payload.P65NormalContainerPlayerStandardEquipmentGroundDropProof;
	const demo_map_code_b::FCodeBP48NormalContainerSpatialGraphEquipmentTransferProof& P50Proof =
		Payload.P48NormalContainerSpatialGraphEquipmentProof;
	const bool bP49SimpleStack = P49Proof.HasSourceIdentity();
	const bool bP64PlayerSimpleStack = P64Proof.HasSourceIdentity();
	const bool bP65PlayerStandardEquipment = P65Proof.HasSourceIdentity();
	const bool bP50SpatialGraph = P50Proof.HasSourceIdentity() && !P50Proof.bIntent;
	const bool bP66PlayerSpatialGraph = bP50SpatialGraph && P50Proof.bPlayerDepositedSource;
	const bool bP67Split = bP49SimpleStack && Payload.bSplitIntent
		&& Payload.QuantityDraftKind == demo_map_code_b::ECodeBP3QuantityDraftKind::PlayerSplit
		&& Payload.GraphIdentity == P49Proof.SourceContainerId
		&& Payload.RequestedMergeQuantity > 0
		&& Payload.RequestedMergeQuantity < Payload.Quantity;
	const bool bP68Split = bP64PlayerSimpleStack && Payload.bSplitIntent
		&& Payload.QuantityDraftKind == demo_map_code_b::ECodeBP3QuantityDraftKind::PlayerSplit
		&& Payload.GraphIdentity == P64Proof.SourceContainerId
		&& Payload.RequestedMergeQuantity > 0
		&& Payload.RequestedMergeQuantity < Payload.Quantity;
	const bool bExactNormalContainerSplit = bP67Split || bP68Split;
	const int32 SourceKindCount = (bP49SimpleStack ? 1 : 0)
		+ (bP64PlayerSimpleStack ? 1 : 0) + (bP65PlayerStandardEquipment ? 1 : 0)
		+ (bP50SpatialGraph ? 1 : 0);
	const demo_map_code_b::FCodeBP49NormalContainerSimpleStackGroundDropProof& SimpleProof =
		bP65PlayerStandardEquipment
			? static_cast<const demo_map_code_b::FCodeBP49NormalContainerSimpleStackGroundDropProof&>(P65Proof)
			: bP64PlayerSimpleStack
			? static_cast<const demo_map_code_b::FCodeBP49NormalContainerSimpleStackGroundDropProof&>(P64Proof)
			: P49Proof;
	const FGuid ProofOwnerId = bP50SpatialGraph ? P50Proof.OwnerId : SimpleProof.OwnerId;
	const FGuid ProofRunInstanceId = bP50SpatialGraph ? P50Proof.RunInstanceId : SimpleProof.RunInstanceId;
	const FGuid ProofSearchTargetId = bP50SpatialGraph ? P50Proof.SearchTargetId : SimpleProof.SearchTargetId;
	const FName ProofDefinitionId = bP50SpatialGraph
		? P50Proof.NormalContainerDefinitionId : SimpleProof.NormalContainerDefinitionId;
	const int32 ProofNormalRevision = bP50SpatialGraph
		? P50Proof.NormalContainerRevision : SimpleProof.NormalContainerRevision;
	const int32 ProofP6Revision = bP50SpatialGraph
		? P50Proof.P6SnapshotRevision : SimpleProof.P6SnapshotRevision;
	const FGuid ProofSourceItemId = bP50SpatialGraph ? P50Proof.SourceItemId : SimpleProof.SourceItemId;
	const FGuid ProofSourceContainerId = bP50SpatialGraph
		? P50Proof.SourceContainerId : SimpleProof.SourceContainerId;
	const int32 ProofSourceSlot = bP50SpatialGraph ? P50Proof.SourceSlot : SimpleProof.SourceSlot;
	const FName ProofSourceDefinitionId = bP50SpatialGraph
		? P50Proof.SourceDefinitionId : SimpleProof.SourceDefinitionId;
	const int32 ProofSourceQuantity = bP50SpatialGraph ? 1 : SimpleProof.SourceQuantity;
	const int32 ProofCompositeRevision = bP50SpatialGraph
		? P50Proof.CompositeRevision : SimpleProof.CompositeRevision;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	const demo_map_code_b::FCodeBP3InventoryWorkspaceContext* Workspace =
		Host ? Host->GetWorkspaceContext() : nullptr;
	if (!Payload.IsValid() || SourceKindCount != 1
		|| Payload.bQuickTransferIntent
		|| (!bExactNormalContainerSplit && Payload.bSplitIntent)
		|| (bP67Split && (bP64PlayerSimpleStack || bP65PlayerStandardEquipment || bP50SpatialGraph))
		|| (bP68Split && (bP49SimpleStack || bP65PlayerStandardEquipment || bP50SpatialGraph))
		|| (!bExactNormalContainerSplit && Payload.QuantityDraftKind != demo_map_code_b::ECodeBP3QuantityDraftKind::None)
		|| (!bExactNormalContainerSplit && Payload.RequestedMergeQuantity != 0) || Payload.Quantity <= 0
		|| Payload.SourceScope != demo_map_code_b::ECodeBP3InventoryScope::ExternalTarget
		|| Payload.QuickTransferTargetMode != demo_map_code_b::ECodeBQuickTransferTargetMode::Legacy
		|| Payload.QuickTransferActivePlayerContainerId.IsValid()
		|| Payload.QuickTransferActivePlayerParentItemId.IsValid()
		|| Payload.ActivePlayerChildOpenGeneration != 0
		|| Payload.P38BodyEquipmentProof.bIntent
		|| Payload.P40BodySimpleStackProof.bIntent
		|| Payload.P41BodySpatialGraphProof.bIntent
		|| Payload.P42BodySpatialGraphEquipmentProof.bSourceProof
		|| Payload.P43BodySimpleStackGroundDropProof.bIntent
		|| Payload.P46NormalContainerSimpleStackProof.bIntent
		|| Payload.P62NormalContainerStandardEquipmentProof.bIntent
		|| Payload.P63NormalContainerPlayerSimpleStackProof.bIntent
		|| Payload.P47NormalContainerSpatialGraphProof.bIntent
		|| Payload.P48NormalContainerSpatialGraphEquipmentProof.bIntent
		|| (bP49SimpleStack && Payload.P48NormalContainerSpatialGraphEquipmentProof.bSourceProof)
		|| (bP64PlayerSimpleStack && Payload.P48NormalContainerSpatialGraphEquipmentProof.bSourceProof)
		|| (bP65PlayerStandardEquipment && Payload.P48NormalContainerSpatialGraphEquipmentProof.bSourceProof)
		|| (bP50SpatialGraph && Payload.P49NormalContainerSimpleStackGroundDropProof.bIntent)
		|| (bP50SpatialGraph && Payload.P64NormalContainerPlayerSimpleStackGroundDropProof.bIntent)
		|| (bP50SpatialGraph && Payload.P65NormalContainerPlayerStandardEquipmentGroundDropProof.bIntent)
		|| Payload.WorldDropId.IsValid() || Payload.WorldDropOrdinal != 0
		|| Payload.WorldDropRecordRevision != INDEX_NONE
		|| Payload.WorldDropTargetOpenGeneration != 0 || !Payload.WorldDropMapRoute.IsNone()
		|| !bCodeBNormalContainerOpen || !ProfilePreparationFlow
		|| !ActiveCodeBNormalContainer.IsValid()
		|| !IsRegisteredCodeBNormalContainerTarget(ActiveCodeBNormalContainer.Get())
		|| !Host || !Host->IsHostEnabled() || !Workspace || !Workspace->IsInRun()
		|| Workspace->OwnerId != CodeBNormalContainerOwnerId
		|| Workspace->RunInstanceId != CodeBNormalContainerRunId
		|| Workspace->TargetPaneId != FName(TEXT("InRun.External"))
		|| ProofOwnerId != CodeBNormalContainerOwnerId
		|| ProofRunInstanceId != CodeBNormalContainerRunId
		|| ProofSearchTargetId != CodeBNormalContainerTargetId
		|| ProofDefinitionId != CodeBNormalContainerDefinitionId
		|| ProofNormalRevision != CodeBNormalContainerExpectedTargetRevision
		|| ProofP6Revision != CodeBNormalContainerExpectedP6Revision
		|| ProofSourceItemId != Payload.ItemId
		|| ProofSourceContainerId != Payload.Source.ContainerId
		|| ProofSourceSlot != Payload.Source.SlotIndex
		|| ProofSourceDefinitionId != Payload.DefinitionId
		|| ProofSourceQuantity != Payload.Quantity
		|| ProofCompositeRevision != Payload.ExpectedRevision)
	{
		OutFeedback = TEXT("P49/P50/P64/P65/P66 BasicCache GroundDrop source or exact open-host identity is stale; no fallback was attempted.");
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()
		? ProfilePreparationFlow->GetSession()->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	if (ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive
		|| Snapshot.ProfileId != ProofOwnerId || Snapshot.ActiveRunId != ProofRunInstanceId
		|| ProfilePreparationFlow->GetStartedRunId() != ProofRunInstanceId)
	{
		OutFeedback = TEXT("P49/P50/P64/P65/P66 BasicCache GroundDrop requires the same active Owner/Run lifecycle.");
		return false;
	}

	FName MapRoute;
	FTransform FloorTransform;
	if (!ResolveCodeBWorldDropPlacement(MapRoute, FloorTransform, OutFeedback)) return false;

	FCodeBNormalContainerProjection UpdatedNormal;
	FCodeBWorldDropProjection NewWorldDrop;
	const bool bStored = bP50SpatialGraph
		? FCodeBOutOfRaidProfileStore::DropMatchedRunNormalContainerSpatialWorldDropItem(
			ProfilePreparationFlow->GetStorageRoot(), CodeBNormalContainerOwnerId,
			CodeBNormalContainerRunId, CodeBNormalContainerTargetId,
			CodeBNormalContainerDefinitionId, CodeBNormalContainerExpectedP6Revision,
			CodeBNormalContainerExpectedTargetRevision, P50Proof, MapRoute, FloorTransform,
			UpdatedNormal, NewWorldDrop, &OutFeedback)
		: FCodeBOutOfRaidProfileStore::DropMatchedRunNormalContainerWorldDropItem(
			ProfilePreparationFlow->GetStorageRoot(), CodeBNormalContainerOwnerId,
			CodeBNormalContainerRunId, CodeBNormalContainerTargetId,
			CodeBNormalContainerDefinitionId, CodeBNormalContainerExpectedP6Revision,
			CodeBNormalContainerExpectedTargetRevision, SimpleProof,
			bExactNormalContainerSplit ? Payload.RequestedMergeQuantity : 0,
			bP64PlayerSimpleStack || bP65PlayerStandardEquipment,
			bP65PlayerStandardEquipment,
			MapRoute, FloorTransform,
			UpdatedNormal, NewWorldDrop, &OutFeedback);
	if (!bStored)
	{
		return false;
	}

	// Rebind both read models only after the one Owner record replacement is durable.
	CodeBActiveRunInventoryStore = MakeUnique<FCodeBOutOfRaidProfileStore>(
		ProfilePreparationFlow->GetStorageRoot(), CodeBNormalContainerOwnerId);
	CodeBActiveRunInventoryOwnerId = CodeBNormalContainerOwnerId;
	CodeBActiveRunInventoryRunId = CodeBNormalContainerRunId;
	FCodeBRunInventorySession UpdatedSession;
	if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
		CodeBNormalContainerRunId, UpdatedSession, &OutFeedback))
	{
		return false;
	}
	const FCodeBRunLocalNormalContainerRecord* UpdatedNormalRecord =
		CodeBActiveRunInventoryStore->GetRecord().RunLocalNormalContainers.FindByPredicate(
			[this](const FCodeBRunLocalNormalContainerRecord& Value)
			{
				return Value.SearchTargetId == CodeBNormalContainerTargetId;
			});
	demo_map_code_b::FCodeBSnapshot UpdatedComposite;
	if (!UpdatedNormalRecord
		|| !FCodeBRunItemInteractionDomain::BuildAcceptedCompositeSnapshot(
			UpdatedSession.RepositorySnapshot, UpdatedNormalRecord->ContainerSnapshot,
			UpdatedComposite, OutFeedback)
		|| !CodeBNormalContainerRepository.IsValid()
		|| !CodeBNormalContainerRepository->LoadPersistedSnapshot(UpdatedComposite, &OutFeedback))
	{
		return false;
	}
	CodeBNormalContainerExpectedP6Revision = UpdatedSession.RepositorySnapshot.Revision;
	CodeBNormalContainerExpectedTargetRevision = UpdatedNormal.Revision;
	Host->UpdateNormalContainerProjection(
		UpdatedNormal, UpdatedSession.RepositorySnapshot.Revision);
	RefreshCodeBWorldDropActors();
	UE_LOG(LogTemp, Display,
		TEXT("CodeB.%s.GroundDrop Committed OwnerId=%s RunId=%s SearchTargetId=%s WorldDropId=%s ItemId=%s"),
		bP67Split ? TEXT("P67")
			: bP68Split ? TEXT("P68")
			: bP66PlayerSpatialGraph ? TEXT("P66")
			: bP50SpatialGraph ? TEXT("P50")
			: (bP65PlayerStandardEquipment ? TEXT("P65")
				: (bP64PlayerSimpleStack ? TEXT("P64") : TEXT("P49"))),
		*NewWorldDrop.OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewWorldDrop.RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*CodeBNormalContainerTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewWorldDrop.WorldDropId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewWorldDrop.ItemId.ToString(EGuidFormats::DigitsWithHyphens));
	return true;
}

bool Ademo_mapV3ProgressionManager::RequestCodeBBodyGroundDrop(
	const demo_map_code_b::FCodeBP4DragPayload& Payload,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	const demo_map_code_b::FCodeBP43BodySimpleStackGroundDropProof& P43Proof =
		Payload.P43BodySimpleStackGroundDropProof;
	const demo_map_code_b::FCodeBP38BodyEquipmentTransferProof& P44Proof =
		Payload.P38BodyEquipmentProof;
	const demo_map_code_b::FCodeBP42BodySpatialGraphEquipmentTransferProof& P45Proof =
		Payload.P42BodySpatialGraphEquipmentProof;
	const bool bP43SimpleStack = P43Proof.HasSourceIdentity();
	const bool bP44StandardEquipment = P44Proof.HasSourceIdentity();
	const bool bP45SpatialGraph = P45Proof.HasSourceIdentity() && !P45Proof.bIntent;
	const bool bP69Split = bP43SimpleStack && Payload.bSplitIntent
		&& Payload.QuantityDraftKind == demo_map_code_b::ECodeBP3QuantityDraftKind::PlayerSplit
		&& Payload.GraphIdentity == P43Proof.SourceContainerId
		&& Payload.RequestedMergeQuantity > 0
		&& Payload.RequestedMergeQuantity < Payload.Quantity;
	const FGuid ProofOwnerId = bP45SpatialGraph ? P45Proof.OwnerId
		: (bP44StandardEquipment ? P44Proof.OwnerId : P43Proof.OwnerId);
	const FGuid ProofRunInstanceId = bP45SpatialGraph ? P45Proof.RunInstanceId
		: (bP44StandardEquipment ? P44Proof.RunInstanceId : P43Proof.RunInstanceId);
	const FGuid ProofBodyTargetId = bP45SpatialGraph ? P45Proof.BodyTargetId
		: (bP44StandardEquipment ? P44Proof.BodyTargetId : P43Proof.BodyTargetId);
	const FName ProofBodyDefinitionId = bP45SpatialGraph ? P45Proof.BodyDefinitionId
		: (bP44StandardEquipment ? P44Proof.BodyDefinitionId : P43Proof.BodyDefinitionId);
	const int32 ProofBodyRecordRevision = bP45SpatialGraph ? P45Proof.BodyRecordRevision
		: (bP44StandardEquipment ? P44Proof.BodyRecordRevision : P43Proof.BodyRecordRevision);
	const FGuid ProofSourceItemId = bP45SpatialGraph ? P45Proof.SourceItemId
		: (bP44StandardEquipment ? P44Proof.SourceItemId : P43Proof.SourceItemId);
	const FGuid ProofSourceContainerId = bP45SpatialGraph ? P45Proof.SourceContainerId
		: (bP44StandardEquipment ? P44Proof.SourceContainerId : P43Proof.SourceContainerId);
	const int32 ProofSourceSlot = bP45SpatialGraph ? P45Proof.SourceSlot
		: (bP44StandardEquipment ? P44Proof.SourceSlot : P43Proof.SourceSlot);
	const FName ProofSourceDefinitionId = bP45SpatialGraph ? P45Proof.SourceDefinitionId
		: (bP44StandardEquipment ? P44Proof.SourceDefinitionId : P43Proof.SourceDefinitionId);
	const int32 ProofSourceQuantity = bP45SpatialGraph || bP44StandardEquipment
		? 1 : P43Proof.SourceQuantity;
	const int32 SourceKindCount = (bP43SimpleStack ? 1 : 0)
		+ (bP44StandardEquipment ? 1 : 0) + (bP45SpatialGraph ? 1 : 0);
	if (!Payload.IsValid() || SourceKindCount != 1
		|| Payload.bQuickTransferIntent
		|| (!bP69Split && Payload.bSplitIntent)
		|| (!bP69Split && Payload.QuantityDraftKind != demo_map_code_b::ECodeBP3QuantityDraftKind::None)
		|| (!bP69Split && Payload.RequestedMergeQuantity != 0)
		|| (bP69Split && (bP44StandardEquipment || bP45SpatialGraph))
		|| (bP43SimpleStack && (Payload.P38BodyEquipmentProof.bIntent
			|| Payload.P42BodySpatialGraphEquipmentProof.bSourceProof))
		|| (bP44StandardEquipment && (Payload.P43BodySimpleStackGroundDropProof.bIntent
			|| Payload.P42BodySpatialGraphEquipmentProof.bSourceProof))
		|| (bP45SpatialGraph && (Payload.P43BodySimpleStackGroundDropProof.bIntent
			|| Payload.P38BodyEquipmentProof.bIntent))
		|| Payload.P40BodySimpleStackProof.bIntent
		|| Payload.P41BodySpatialGraphProof.bIntent
		|| Payload.P62NormalContainerStandardEquipmentProof.bIntent
		|| Payload.P63NormalContainerPlayerSimpleStackProof.bIntent
		|| Payload.P47NormalContainerSpatialGraphProof.bIntent
		|| Payload.P48NormalContainerSpatialGraphEquipmentProof.bIntent
		|| Payload.P49NormalContainerSimpleStackGroundDropProof.bIntent
		|| Payload.P64NormalContainerPlayerSimpleStackGroundDropProof.bIntent
		|| Payload.P65NormalContainerPlayerStandardEquipmentGroundDropProof.bIntent
		|| Payload.P42BodySpatialGraphEquipmentProof.bIntent
		|| Payload.WorldDropId.IsValid() || Payload.WorldDropOrdinal != 0
		|| Payload.WorldDropRecordRevision != INDEX_NONE
		|| Payload.WorldDropTargetOpenGeneration != 0 || !Payload.WorldDropMapRoute.IsNone()
		|| !bCodeBBodyContainerOpen || !ProfilePreparationFlow
		|| !ActiveCodeBBodyContainer.IsValid()
		|| ActiveCodeBBodyContainer->GetCodeBBodyTargetIdentity()
			!= GCodeBBodyContainerTargetIdentity
		|| ProofOwnerId != CodeBBodyContainerOwnerId
		|| ProofRunInstanceId != CodeBBodyContainerRunId
		|| ProofBodyTargetId != CodeBBodyContainerTargetId
		|| ProofBodyDefinitionId != CodeBBodyContainerDefinitionId
		|| ProofBodyRecordRevision != CodeBBodyContainerExpectedTargetRevision
		|| ProofSourceItemId != Payload.ItemId
		|| ProofSourceContainerId != Payload.Source.ContainerId
		|| ProofSourceSlot != Payload.Source.SlotIndex
		|| ProofSourceDefinitionId != Payload.DefinitionId
		|| ProofSourceQuantity != Payload.Quantity)
	{
		OutFeedback = TEXT("P43/P44/P45 body GroundDrop source or open-host identity is stale; no fallback was attempted.");
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()
		? ProfilePreparationFlow->GetSession()->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	if (ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive
		|| Snapshot.ProfileId != ProofOwnerId || Snapshot.ActiveRunId != ProofRunInstanceId
		|| ProfilePreparationFlow->GetStartedRunId() != ProofRunInstanceId)
	{
		OutFeedback = TEXT("P43/P44/P45 body GroundDrop requires the same active Owner/Run lifecycle.");
		return false;
	}

	FName MapRoute;
	FTransform FloorTransform;
	if (!ResolveCodeBWorldDropPlacement(MapRoute, FloorTransform, OutFeedback)) return false;

	FCodeBBodyContainerProjection UpdatedBody;
	FCodeBWorldDropProjection NewWorldDrop;
	if (!FCodeBOutOfRaidProfileStore::DropMatchedRunBodyContainerWorldDropItem(
		ProfilePreparationFlow->GetStorageRoot(), CodeBBodyContainerOwnerId,
		CodeBBodyContainerRunId, CodeBBodyContainerTargetId,
		CodeBBodyContainerDefinitionId, CodeBBodyContainerExpectedP6Revision,
		CodeBBodyContainerExpectedTargetRevision, P43Proof, P44Proof, P45Proof,
		bP69Split ? Payload.RequestedMergeQuantity : 0,
		MapRoute, FloorTransform,
		UpdatedBody, NewWorldDrop, &OutFeedback))
	{
		return false;
	}

	// Reload both projections only after the one Owner replacement is durable.
	CodeBActiveRunInventoryStore = MakeUnique<FCodeBOutOfRaidProfileStore>(
		ProfilePreparationFlow->GetStorageRoot(), CodeBBodyContainerOwnerId);
	CodeBActiveRunInventoryOwnerId = CodeBBodyContainerOwnerId;
	FCodeBRunInventorySession UpdatedSession;
	if (!CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
		CodeBBodyContainerRunId, UpdatedSession, &OutFeedback))
	{
		return false;
	}
	const FCodeBRunLocalBodyContainerRecord* UpdatedBodyRecord =
		CodeBActiveRunInventoryStore->GetRecord().RunLocalBodyContainers.FindByPredicate(
			[this](const FCodeBRunLocalBodyContainerRecord& Value)
			{
				return Value.BodyTargetId == CodeBBodyContainerTargetId;
			});
	demo_map_code_b::FCodeBSnapshot UpdatedComposite;
	if (!UpdatedBodyRecord
		|| !FCodeBRunItemInteractionDomain::BuildAcceptedCompositeSnapshot(
			UpdatedSession.RepositorySnapshot, UpdatedBodyRecord->ContainerSnapshot,
			UpdatedComposite, OutFeedback)
		|| !CodeBBodyContainerRepository.IsValid()
		|| !CodeBBodyContainerRepository->LoadPersistedSnapshot(UpdatedComposite, &OutFeedback))
	{
		return false;
	}
	CodeBBodyContainerExpectedP6Revision = UpdatedSession.RepositorySnapshot.Revision;
	CodeBBodyContainerExpectedTargetRevision = UpdatedBody.Revision;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr)
	{
		Host->UpdateBodyContainerProjection(UpdatedBody);
	}
	RefreshCodeBWorldDropActors();
	UE_LOG(LogTemp, Display,
		TEXT("CodeB.%s.GroundDrop Committed OwnerId=%s RunId=%s BodyTargetId=%s WorldDropId=%s ItemId=%s"),
		bP44StandardEquipment ? TEXT("P44") : (bP69Split ? TEXT("P69") : TEXT("P43")),
		*NewWorldDrop.OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewWorldDrop.RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*CodeBBodyContainerTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewWorldDrop.WorldDropId.ToString(EGuidFormats::DigitsWithHyphens),
		*NewWorldDrop.ItemId.ToString(EGuidFormats::DigitsWithHyphens));
	return true;
}

void Ademo_mapV3ProgressionManager::ApplyCodeBRunItemInteractionResult(
	const FCodeBActivePlayerInteractionCommitResult& Result)
{
	if (Result.OwnerId == CodeBActiveRunInventoryOwnerId
		&& Result.RunInstanceId == CodeBActiveRunInventoryRunId)
	{
		CodeBActiveRunInventoryExpectedP6Revision = Result.P6SnapshotRevision;
	}
}

void Ademo_mapV3ProgressionManager::ApplyCodeBRunItemInteractionResult(
	const FCodeBNormalContainerInteractionCommitResult& Result)
{
	if (Result.OwnerId != CodeBNormalContainerOwnerId
		|| Result.RunInstanceId != CodeBNormalContainerRunId
		|| Result.TargetId != CodeBNormalContainerTargetId)
	{
		return;
	}
	CodeBNormalContainerExpectedP6Revision = Result.P6SnapshotRevision;
	CodeBNormalContainerExpectedTargetRevision = Result.TargetRevision;
	if (!Result.Projection.IsSet()) return;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr)
	{
		Host->UpdateNormalContainerProjection(Result.Projection.GetValue(), Result.P6SnapshotRevision);
	}
}

void Ademo_mapV3ProgressionManager::ApplyCodeBRunItemInteractionResult(
	const FCodeBBodyContainerInteractionCommitResult& Result)
{
	if (Result.OwnerId != CodeBBodyContainerOwnerId
		|| Result.RunInstanceId != CodeBBodyContainerRunId
		|| Result.TargetId != CodeBBodyContainerTargetId)
	{
		return;
	}
	CodeBBodyContainerExpectedP6Revision = Result.P6SnapshotRevision;
	CodeBBodyContainerExpectedTargetRevision = Result.TargetRevision;
	if (!Result.Projection.IsSet()) return;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr)
	{
		Host->UpdateBodyContainerProjection(Result.Projection.GetValue());
	}
}

void Ademo_mapV3ProgressionManager::ApplyCodeBRunItemInteractionResult(
	const FCodeBWorldDropInteractionCommitResult& Result)
{
	if (Result.OwnerId != CodeBWorldDropOwnerId
		|| Result.RunInstanceId != CodeBWorldDropRunId
		|| Result.WorldDropId != CodeBWorldDropId)
	{
		return;
	}
	CodeBWorldDropExpectedP6Revision = Result.P6SnapshotRevision;
	if (Result.Projection.IsSet())
	{
		CodeBWorldDropRecordRevision = Result.Projection.GetValue().RecordRevision;
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
			{
				Host->UpdateWorldDropProjection(Result.Projection.GetValue());
			}
		}
	}
	if (Result.bReconcileActors)
	{
		// This is post-commit visual reconciliation only: it never changes a
		// committed Store result into an ordinary action failure.
		RefreshCodeBWorldDropActors();
	}
}
void Ademo_mapV3ProgressionManager::RefreshCodeBWorldDropActors()
{
	if (!CodeBActiveRunInventoryStore.IsValid() || !CodeBActiveRunInventoryRunId.IsValid() || !GetWorld()) return;
	TArray<FCodeBWorldDropProjection> Projections;
	FString Error;
	if (!CodeBActiveRunInventoryStore->TryGetMatchedActiveRunWorldDropProjections(
		CodeBActiveRunInventoryRunId, Projections, &Error))
	{
		return;
	}
	TSet<FGuid> ExpectedIds;
	for (const FCodeBWorldDropProjection& Projection : Projections)
	{
		if (Projection.MapRoute != FName(*GetWorld()->GetMapName())) continue;
		ExpectedIds.Add(Projection.WorldDropId);
		if (TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>* Existing = CodeBWorldDropActors.Find(Projection.WorldDropId);
			Existing && Existing->IsValid())
		{
			Existing->Get()->ConfigureCodeBWorldDrop(Projection);
			continue;
		}
		if (Ademo_mapCodeBWorldDropActor* Spawned = GetWorld()->SpawnActor<Ademo_mapCodeBWorldDropActor>(
			Ademo_mapCodeBWorldDropActor::StaticClass(), Projection.FloorTransform))
		{
			Spawned->ConfigureCodeBWorldDrop(Projection);
			CodeBWorldDropActors.Add(Projection.WorldDropId, Spawned);
		}
	}
	TArray<FGuid> StaleIds;
	for (const TPair<FGuid, TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>>& Pair : CodeBWorldDropActors)
	{
		if (!ExpectedIds.Contains(Pair.Key)) StaleIds.Add(Pair.Key);
	}
	for (const FGuid& StaleId : StaleIds)
	{
		if (TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>* Actor = CodeBWorldDropActors.Find(StaleId);
			Actor && Actor->IsValid()) Actor->Get()->Destroy();
		CodeBWorldDropActors.Remove(StaleId);
	}
}

void Ademo_mapV3ProgressionManager::ClearCodeBWorldDropActors()
{
	TArray<TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>> Actors;
	CodeBWorldDropActors.GenerateValueArray(Actors);
	CodeBWorldDropActors.Reset();
	for (const TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>& Actor : Actors)
	{
		if (Actor.IsValid()) Actor->Destroy();
	}
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::RequestCodeBWorldDropInteract(
	Ademo_mapCodeBWorldDropActor* DropActor)
{
	if (!DropActor || bInventoryOpen || bCodeBActiveRunInventoryOpen || bSearchContainerOpen
		|| bCodeBNormalContainerOpen || bCodeBBodyContainerOpen || bCodeBWorldDropOpen)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked, TEXT("A different inventory or target surface is already open."));
	}
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive, TEXT("Ground drop interaction requires an active Run."));
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	const FGuid RunId = ProfilePreparationFlow->GetStartedRunId();
	if (Snapshot.ProfileId != DropActor->GetOwnerId() || RunId != DropActor->GetRunInstanceId()
		|| Snapshot.ActiveRunId != RunId)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidRunId, TEXT("Ground drop identity no longer matches the active Run."));
	}
	FString Feedback;
	if (!OpenCodeBWorldDropPage(DropActor, Feedback))
	{
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InteractionBlocked, Feedback);
	}
	return Fdemo_mapItemOperationResult::Success(FGuid(), NAME_None, NAME_None, FName(TEXT("CodeB.WorldDrop")));
}

bool Ademo_mapV3ProgressionManager::OpenCodeBWorldDropPage(
	Ademo_mapCodeBWorldDropActor* DropActor,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	if (!DropActor || !ProfilePreparationFlow || !CodeBActiveRunInventoryStore.IsValid())
	{
		OutFeedback = TEXT("地面物品页面缺少活动 P6 服务。");
		return false;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance ? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	if (!Host || Host->IsHostEnabled())
	{
		OutFeedback = TEXT("地面物品页面需要空闲的生产 P3/P4 Host。");
		return false;
	}
	FCodeBWorldDropPresentationRequest FrameRequest;
	FrameRequest.OwnerId = DropActor->GetOwnerId();
	FrameRequest.RunInstanceId = DropActor->GetRunInstanceId();
	FrameRequest.WorldDropId = DropActor->GetWorldDropId();
	FrameRequest.Ordinal = DropActor->GetOrdinal();
	FrameRequest.WorldContainerId = DropActor->GetWorldContainerId();
	FrameRequest.RootItemId = DropActor->GetRootItemId();
	FrameRequest.SpatialChildContainerId = DropActor->GetSpatialChildContainerId();
	FrameRequest.MapRoute = DropActor->GetMapRoute();
	FrameRequest.RecordRevision = DropActor->GetRecordRevision();
	FCodeBWorldDropPresentationFrame Frame;
	if (!FCodeBRunItemInteractionDomain::BuildWorldDropFrame(
		ProfilePreparationFlow->GetStorageRoot(), FrameRequest, Frame, OutFeedback)
		|| Frame.Projection.MapRoute != FName(*GetWorld()->GetMapName()))
	{
		if (OutFeedback.IsEmpty()) OutFeedback = TEXT("地面物品不属于当前地图路由或已被移除。");
		return false;
	}
	const FCodeBWorldDropProjection* Drop = &Frame.Projection;
	CodeBWorldDropRepository = MoveTemp(Frame.Shared.Repository);
	demo_map_code_b::FCodeBP2PlayerLayout PresentationLayout = MoveTemp(Frame.Shared.Layout);
	CodeBWorldDropOwnerId = Drop->OwnerId;
	CodeBWorldDropRunId = Drop->RunInstanceId;
	CodeBWorldDropId = Drop->WorldDropId;
	CodeBWorldDropContainerId = Drop->WorldContainerId;
	CodeBWorldDropRootItemId = Drop->ItemId;
	CodeBWorldDropSpatialChildContainerId = Drop->SpatialChildContainerId;
	CodeBWorldDropMapRoute = Drop->MapRoute;
	CodeBWorldDropOrdinal = Drop->Ordinal;
	CodeBWorldDropRecordRevision = Drop->RecordRevision;
	CodeBWorldDropExpectedP6Revision = Frame.Shared.P6SnapshotRevision;
	if (NextCodeBWorldDropOpenGeneration == 0) NextCodeBWorldDropOpenGeneration = 1;
	CodeBWorldDropOpenGeneration = NextCodeBWorldDropOpenGeneration++;
	ActiveCodeBWorldDrop = DropActor;
	const TWeakObjectPtr<Ademo_mapV3ProgressionManager> WeakManager(this);
	FCodeBP3WorldDropPresentation Presentation;
	Presentation.OwnerId = Drop->OwnerId;
	Presentation.RunInstanceId = Drop->RunInstanceId;
	Presentation.WorldDropId = Drop->WorldDropId;
	Presentation.Ordinal = Drop->Ordinal;
	Presentation.TargetContainerId = Drop->WorldContainerId;
	Presentation.RootItemId = Drop->ItemId;
	Presentation.SpatialChildContainerId = Drop->SpatialChildContainerId;
	Presentation.MapRoute = Drop->MapRoute;
	Presentation.FloorTransform = Drop->FloorTransform;
	Presentation.RecordRevision = Drop->RecordRevision;
	Presentation.Provenance = Frame.Provenance;
	Presentation.TargetOpenGeneration = CodeBWorldDropOpenGeneration;
	Presentation.Title = TEXT("地面物品");
	FCodeBP3GroundDropPresentation GroundDropPresentation;
	GroundDropPresentation.RequestDrop = [WeakManager](
		const demo_map_code_b::FCodeBP4DragPayload& Payload, FString& OutError)
	{
		return WeakManager.IsValid()
			&& WeakManager->RequestCodeBWorldDropGroundDrop(Payload, OutError);
	};
	FCodeBP3WorkspacePresentation Workspace;
	Workspace.Context.Scope = demo_map_code_b::ECodeBP3WorkspaceScope::InRunP6;
	Workspace.Context.OwnerId = Drop->OwnerId;
	Workspace.Context.RunInstanceId = Drop->RunInstanceId;
	Workspace.Context.SessionRevision = Frame.Shared.P6SnapshotRevision;
	Workspace.Context.WriteGate = demo_map_code_b::ECodeBP3WorkspaceWriteGate::InRun;
	Workspace.Context.PlayerPaneId = FName(TEXT("InRun.Player"));
	Workspace.Context.TargetPaneId = FName(TEXT("InRun.WorldDrop"));
	Workspace.ResolveWriteGate = [WeakManager]()
	{
		if (!WeakManager.IsValid())
		{
			return demo_map_code_b::ECodeBP3WorkspaceWriteGate::Unavailable;
		}
		Ademo_mapV3ProgressionManager* Manager = WeakManager.Get();
		const bool bCurrentWorldPage = Manager->CodeBActiveRunInventoryStore.IsValid()
			&& Manager->CodeBWorldDropRepository.IsValid()
			&& Manager->CodeBWorldDropOwnerId.IsValid()
			&& Manager->CodeBWorldDropRunId.IsValid()
			&& Manager->CodeBWorldDropId.IsValid()
			&& Manager->CodeBWorldDropContainerId.IsValid()
			&& Manager->CodeBWorldDropRootItemId.IsValid()
			&& Manager->CodeBWorldDropOrdinal > 0
			&& Manager->CodeBWorldDropRecordRevision > 0
			&& Manager->CodeBWorldDropOpenGeneration != 0
			&& !Manager->CodeBWorldDropMapRoute.IsNone()
			&& Manager->ActiveCodeBWorldDrop.IsValid()
			&& Manager->ProfilePreparationFlow
			&& Manager->ProfilePreparationFlow->GetPhase() == Edemo_mapProfilePreparationFlowPhase::RunActive
			&& Manager->ProfilePreparationFlow->GetStartedRunId() == Manager->CodeBWorldDropRunId
			&& Manager->ActiveCodeBWorldDrop->GetOwnerId() == Manager->CodeBWorldDropOwnerId
			&& Manager->ActiveCodeBWorldDrop->GetRunInstanceId() == Manager->CodeBWorldDropRunId
			&& Manager->ActiveCodeBWorldDrop->GetWorldDropId() == Manager->CodeBWorldDropId
			&& Manager->ActiveCodeBWorldDrop->GetOrdinal() == Manager->CodeBWorldDropOrdinal
			&& Manager->ActiveCodeBWorldDrop->GetWorldContainerId() == Manager->CodeBWorldDropContainerId
			&& Manager->ActiveCodeBWorldDrop->GetRootItemId() == Manager->CodeBWorldDropRootItemId
			&& Manager->ActiveCodeBWorldDrop->GetRecordRevision() == Manager->CodeBWorldDropRecordRevision;
		FCodeBRunInventorySession Current;
		FString Error;
		const bool bExactDurableRecord = bCurrentWorldPage
			&& Manager->CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
				Manager->CodeBWorldDropRunId, Current, &Error)
			&& Current.OwnerId == Manager->CodeBWorldDropOwnerId
			&& Current.RunInstanceId == Manager->CodeBWorldDropRunId
			&& Current.RepositorySnapshot.Revision == Manager->CodeBWorldDropExpectedP6Revision
			&& Current.WorldDrops.ContainsByPredicate([Manager](const FCodeBWorldDropRecord& Value)
			{
				return Value.OwnerId == Manager->CodeBWorldDropOwnerId
					&& Value.RunInstanceId == Manager->CodeBWorldDropRunId
					&& Value.WorldDropId == Manager->CodeBWorldDropId
					&& Value.Ordinal == Manager->CodeBWorldDropOrdinal
					&& Value.WorldContainerId == Manager->CodeBWorldDropContainerId
					&& Value.ItemId == Manager->CodeBWorldDropRootItemId
					&& Value.SpatialChildContainerId == Manager->CodeBWorldDropSpatialChildContainerId
					&& Value.RecordRevision == Manager->CodeBWorldDropRecordRevision
					&& Value.ActionState == ECodeBWorldDropActionState::Available
					&& Value.MapRoute == Manager->CodeBWorldDropMapRoute
					&& Manager->GetWorld() && Value.MapRoute == FName(*Manager->GetWorld()->GetMapName());
			});
		return bExactDurableRecord
			? demo_map_code_b::ECodeBP3WorkspaceWriteGate::InRun
			: demo_map_code_b::ECodeBP3WorkspaceWriteGate::Unavailable;
	};
	Workspace.ResolveSessionRevision = [WeakManager]() -> int32
	{
		if (!WeakManager.IsValid()) return INDEX_NONE;
		Ademo_mapV3ProgressionManager* Manager = WeakManager.Get();
		FCodeBRunInventorySession Current;
		FString Error;
		return Manager->CodeBActiveRunInventoryStore.IsValid()
			&& Manager->CodeBActiveRunInventoryStore->OpenMatchedActiveRunInventorySession(
				Manager->CodeBWorldDropRunId, Current, &Error)
			? Current.RepositorySnapshot.Revision : INDEX_NONE;
	};
	const bool bOpened = Host->OpenProfilePage(
		*CodeBWorldDropRepository,
		PresentationLayout,
		[this](const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
			const demo_map_code_b::FCodeBP2Command& AcceptedCommand, FString& CommitError)
		{
			FCodeBWorldDropInteractionCommitRequest Request;
			Request.StorageRoot = ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString();
			Request.OwnerId = CodeBWorldDropOwnerId;
			Request.RunInstanceId = CodeBWorldDropRunId;
			Request.WorldDropId = CodeBWorldDropId;
			Request.ExpectedP6SnapshotRevision = CodeBWorldDropExpectedP6Revision;
			Request.ExpectedWorldDropOrdinal = CodeBWorldDropOrdinal;
			Request.ExpectedWorldDropRecordRevision = CodeBWorldDropRecordRevision;
			UGameInstance* CurrentGameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
			UCodeBP3UIHostSubsystem* CurrentHost = CurrentGameInstance
				? CurrentGameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
			Request.bTransientScopeCurrent = bCodeBWorldDropOpen
				&& ActiveCodeBWorldDrop.IsValid()
				&& CurrentHost && CurrentHost->IsHostEnabled()
				&& ProfilePreparationFlow && ProfilePreparationFlow->GetSession()
				&& ProfilePreparationFlow->GetPhase() == Edemo_mapProfilePreparationFlowPhase::RunActive
				&& ProfilePreparationFlow->GetStartedRunId() == Request.RunInstanceId
				&& ProfilePreparationFlow->GetSession()->GetSnapshot().ProfileId == Request.OwnerId
				&& ProfilePreparationFlow->GetSession()->GetSnapshot().ActiveRunId == Request.RunInstanceId
				&& ActiveCodeBWorldDrop->GetOwnerId() == Request.OwnerId
				&& ActiveCodeBWorldDrop->GetRunInstanceId() == Request.RunInstanceId
				&& ActiveCodeBWorldDrop->GetWorldDropId() == Request.WorldDropId
				&& ActiveCodeBWorldDrop->GetOrdinal() == Request.ExpectedWorldDropOrdinal
				&& ActiveCodeBWorldDrop->GetRecordRevision() == Request.ExpectedWorldDropRecordRevision;
			FCodeBWorldDropInteractionCommitResult Result;
			const bool bCommitted = FCodeBRunItemInteractionDomain::CommitAcceptedWorldDrop(
				Request, AcceptedCommand, CandidateSnapshot, Result, CommitError);
			if (bCommitted)
			{
				ApplyCodeBRunItemInteractionResult(Result);
			}
			return bCommitted;
		},
		[WeakManager]() { if (WeakManager.IsValid()) WeakManager->CloseCodeBWorldDropPage(); },
		true, nullptr, nullptr, nullptr, &GroundDropPresentation, &Presentation, &Workspace);
	if (!bOpened)
	{
		CodeBWorldDropRepository.Reset();
		CodeBWorldDropOwnerId.Invalidate();
		CodeBWorldDropRunId.Invalidate();
		CodeBWorldDropId.Invalidate();
		CodeBWorldDropContainerId.Invalidate();
		CodeBWorldDropRootItemId.Invalidate();
		CodeBWorldDropSpatialChildContainerId.Invalidate();
		CodeBWorldDropMapRoute = NAME_None;
		CodeBWorldDropOrdinal = 0;
		CodeBWorldDropRecordRevision = INDEX_NONE;
		CodeBWorldDropExpectedP6Revision = INDEX_NONE;
		CodeBWorldDropOpenGeneration = 0;
		ActiveCodeBWorldDrop.Reset();
		OutFeedback = TEXT("地面物品页面无法创建真实 P3/P4 双栏。");
		return false;
	}
	bCodeBWorldDropOpen = true;
	return true;
}

void Ademo_mapV3ProgressionManager::CloseCodeBWorldDropPage()
{
	const bool bWasOpen = bCodeBWorldDropOpen;
	bCodeBWorldDropOpen = false;
	if (bWasOpen)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>();
				Host && Host->IsHostEnabled())
			{
				Host->ClosePage();
				return;
			}
		}
	}
	CodeBWorldDropRepository.Reset();
	CodeBWorldDropOwnerId.Invalidate();
	CodeBWorldDropRunId.Invalidate();
	CodeBWorldDropId.Invalidate();
	CodeBWorldDropContainerId.Invalidate();
	CodeBWorldDropRootItemId.Invalidate();
	CodeBWorldDropSpatialChildContainerId.Invalidate();
	CodeBWorldDropMapRoute = NAME_None;
	CodeBWorldDropOrdinal = 0;
	CodeBWorldDropRecordRevision = INDEX_NONE;
	CodeBWorldDropExpectedP6Revision = INDEX_NONE;
	CodeBWorldDropOpenGeneration = 0;
	ActiveCodeBWorldDrop.Reset();
}

void Ademo_mapV3ProgressionManager::NotifyCodeBWorldDropActorEndPlay(
	Ademo_mapCodeBWorldDropActor* DropActor)
{
	if (!DropActor) return;
	if (const TWeakObjectPtr<Ademo_mapCodeBWorldDropActor>* Registered =
		CodeBWorldDropActors.Find(DropActor->GetWorldDropId()); Registered && Registered->Get() == DropActor)
	{
		CodeBWorldDropActors.Remove(DropActor->GetWorldDropId());
	}
	if (ActiveCodeBWorldDrop.Get() == DropActor) CloseCodeBWorldDropPage();
}

void Ademo_mapV3ProgressionManager::CloseCodeBActiveRunInventory()
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>();
			Host && Host->IsHostEnabled())
		{
			Host->ClosePage();
			return;
		}
	}
	bCodeBActiveRunInventoryOpen = false;
	CodeBActiveRunInventoryRepository.Reset();
	CodeBActiveRunInventoryStore.Reset();
	CodeBActiveRunInventoryOwnerId.Invalidate();
	CodeBActiveRunInventoryRunId.Invalidate();
	CodeBActiveRunInventoryExpectedP6Revision = INDEX_NONE;
}

Fdemo_mapItemOperationResult
Ademo_mapV3ProgressionManager::RequestCodeBNormalContainerInteract(
	Ademo_mapCodeBNormalContainerActor* Container)
{
	if (!IsRegisteredCodeBNormalContainerTarget(Container))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidWorldBinding,
			TEXT("P57 only binds one of the two registered BasicCache target identities."));
	}
	if (bInventoryOpen || bCodeBActiveRunInventoryOpen || bSearchContainerOpen
		|| (bCodeBNormalContainerOpen && ActiveCodeBNormalContainer.Get() != Container))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			TEXT("A different modal inventory or container surface is already open."));
	}
	if (bCodeBNormalContainerOpen && ActiveCodeBNormalContainer.Get() == Container)
	{
		return Fdemo_mapItemOperationResult::Success(
			FGuid(), GCodeBNormalContainerDefinitionId, NAME_None, Container->GetMapTargetIdentity());
	}
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("P10 BasicCache requires the current Code A Run to be active."));
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	const FGuid RunId = ProfilePreparationFlow->GetStartedRunId();
	const FGuid TargetId = CodeBNormalContainerSearchTargetGuid(Container->GetMapTargetIdentity());
	if (!Snapshot.ProfileId.IsValid() || !RunId.IsValid() || Snapshot.ActiveRunId != RunId || !TargetId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidRunId,
			TEXT("P10 BasicCache rejected an unstable Owner/Run/target identity."));
	}
	const bool bLocallyTrackedPendingAction =
		ActiveCodeBNormalContainer.Get() == Container
		&& CodeBNormalContainerOwnerId == Snapshot.ProfileId
		&& CodeBNormalContainerRunId == RunId
		&& CodeBNormalContainerTargetId == TargetId
		&& CodeBNormalContainerActionId.IsValid();
	FCodeBNormalContainerActionResult Open =
		FCodeBOutOfRaidProfileStore::BeginMatchedRunNormalContainerOpen(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, TargetId,
			GCodeBNormalContainerDefinitionId);
	if (Open.Status == ECodeBNormalContainerActionStatus::AlreadyInRequestedState
		&& Open.Projection.ActiveActionId.IsValid()
		&& !bLocallyTrackedPendingAction)
	{
		// A process restart cannot own a previous actor timer.  Persistently cancel
		// that exact stale opening/search before making a fresh request; a live
		// locally-tracked action keeps its timer and receives no duplicate write.
		const FCodeBNormalContainerActionResult Recovered =
			FCodeBOutOfRaidProfileStore::InterruptMatchedRunNormalContainerAction(
				ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, TargetId,
				GCodeBNormalContainerDefinitionId, TEXT("RecoveredStaleRuntimeAction"));
		if (!Recovered.IsCommitted())
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InteractionBlocked, Recovered.Diagnostic);
		}
		Open = FCodeBOutOfRaidProfileStore::BeginMatchedRunNormalContainerOpen(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, TargetId,
			GCodeBNormalContainerDefinitionId);
	}
	if (!Open.IsCommitted())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Open.Status == ECodeBNormalContainerActionStatus::NotEnrolled
				|| Open.Status == ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted
				? Edemo_mapItemResultCode::RunNotActive
				: Edemo_mapItemResultCode::InteractionBlocked,
			Open.Diagnostic);
	}
	CodeBNormalContainerOwnerId = Snapshot.ProfileId;
	CodeBNormalContainerRunId = RunId;
	CodeBNormalContainerTargetId = TargetId;
	CodeBNormalContainerDefinitionId = GCodeBNormalContainerDefinitionId;
	ActiveCodeBNormalContainer = Container;
	CodeBNormalContainerActionId = Open.Projection.ActiveActionId;
	if (Open.Projection.State == ECodeBNormalContainerState::Open)
	{
		FString OpenFeedback;
		if (!OpenCodeBNormalContainerPage(Container, Open.Projection, OpenFeedback))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::InteractionBlocked,
				OpenFeedback);
		}
	}
	else
	{
		Container->ScheduleOpenCompletion(
			CodeBNormalContainerActionId,
			FCodeBNormalContainerInteractionTiming::OpenSeconds);
	}
	return Fdemo_mapItemOperationResult::Success(
		FGuid(), GCodeBNormalContainerDefinitionId, NAME_None, Container->GetMapTargetIdentity());
}

bool Ademo_mapV3ProgressionManager::OpenCodeBNormalContainerPage(
	Ademo_mapCodeBNormalContainerActor* Container,
	const FCodeBNormalContainerProjection& Projection,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	if (!Container || !ProfilePreparationFlow || !CodeBNormalContainerOwnerId.IsValid()
		|| !CodeBNormalContainerRunId.IsValid()
		|| Projection.OwnerId != CodeBNormalContainerOwnerId
		|| Projection.RunInstanceId != CodeBNormalContainerRunId
		|| Projection.SearchTargetId != CodeBNormalContainerTargetId
		|| Projection.DefinitionId != CodeBNormalContainerDefinitionId
		|| Projection.State != ECodeBNormalContainerState::Open)
	{
		OutFeedback = TEXT("P10 normal-container page rejected a stale exact-target context.");
		return false;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance
		? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	if (!Host || (Host->IsHostEnabled() && !bCodeBNormalContainerOpen))
	{
		OutFeedback = TEXT("P10 requires the existing production P3/P4 Host to be available.");
		return false;
	}
	FCodeBNormalContainerPresentationFrame Frame;
	if (!FCodeBRunItemInteractionDomain::BuildNormalContainerFrame(
		ProfilePreparationFlow->GetStorageRoot(), CodeBNormalContainerOwnerId,
		CodeBNormalContainerRunId, CodeBNormalContainerTargetId,
		CodeBNormalContainerDefinitionId, Frame, OutFeedback))
	{
		return false;
	}
	const FCodeBNormalContainerProjection& FreshProjection = Frame.Projection;
	CodeBNormalContainerRepository = MoveTemp(Frame.Shared.Repository);
	demo_map_code_b::FCodeBP2PlayerLayout PresentationLayout = MoveTemp(Frame.Shared.Layout);
	CodeBNormalContainerExpectedP6Revision = Frame.Shared.P6SnapshotRevision;
	CodeBNormalContainerExpectedTargetRevision = FreshProjection.Revision;
	const TWeakObjectPtr<Ademo_mapV3ProgressionManager> WeakManager(this);
	FCodeBP3NormalContainerPresentation Presentation;
	Presentation.TargetContainerId = FreshProjection.ContainerId;
	Presentation.Title = TEXT("普通储物箱");
	Presentation.Projection = FreshProjection;
	Presentation.P6SnapshotRevision = CodeBNormalContainerExpectedP6Revision;
	Presentation.BeginItemSearch = [WeakManager](const demo_map_code_b::FCodeBP3SearchLocator& Locator, FCodeBNormalContainerProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !Locator.IsValid()
			|| Locator.TargetKind != demo_map_code_b::ECodeBP3SearchTargetKind::NormalContainer
			|| Locator.OwnerId != WeakManager->CodeBNormalContainerOwnerId
			|| Locator.RunInstanceId != WeakManager->CodeBNormalContainerRunId
			|| Locator.TargetId != WeakManager->CodeBNormalContainerTargetId
			|| Locator.TargetRevision != WeakManager->CodeBNormalContainerExpectedTargetRevision)
		{
			OutError = TEXT("P10 search locator is stale or belongs to another exact Run target.");
			return false;
		}
		FCodeBNormalContainerProjection Current;
		if (!FCodeBOutOfRaidProfileStore::TryGetMatchedRunNormalContainerProjection(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			Locator.OwnerId, Locator.RunInstanceId, Locator.TargetId, Current, &OutError)
			|| Current.Revision != Locator.TargetRevision
			|| Current.ActiveActionId != Locator.ActiveActionId)
		{
			if (OutError.IsEmpty()) OutError = TEXT("P10 search locator no longer matches persisted target state.");
			return false;
		}
		const FCodeBNormalContainerItemProjection* Item = Current.Items.FindByPredicate(
			[&Locator](const FCodeBNormalContainerItemProjection& Candidate)
			{
				return Candidate.ParentContainerId == Locator.ContainerId
					&& Candidate.SlotIndex == Locator.SlotIndex;
			});
		if (!Item || Item->RevealState != ECodeBNormalContainerRevealState::Hidden)
		{
			OutError = TEXT("P10 persisted search slot is absent or no longer Hidden.");
			return false;
		}
		return WeakManager->BeginCodeBNormalContainerItemSearch(Item->ItemId, OutProjection, OutError);
	};
	Presentation.Refresh = [WeakManager](FCodeBNormalContainerProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		return FCodeBOutOfRaidProfileStore::TryGetMatchedRunNormalContainerProjection(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBNormalContainerOwnerId, WeakManager->CodeBNormalContainerRunId,
			WeakManager->CodeBNormalContainerTargetId, OutProjection, &OutError);
	};
	FCodeBP3HotbarPresentation HotbarPresentation;
	HotbarPresentation.OwnerId = CodeBNormalContainerOwnerId;
	HotbarPresentation.RunInstanceId = CodeBNormalContainerRunId;
	HotbarPresentation.Projection = Frame.Shared.HotbarProjection;
	HotbarPresentation.Refresh = [WeakManager](FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		FCodeBOutOfRaidProfileStore HotbarStore(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBNormalContainerOwnerId);
		return HotbarStore.TryGetMatchedActiveRunHotbarProjection(
			WeakManager->CodeBNormalContainerRunId, OutProjection, &OutError);
	};
	HotbarPresentation.Bind = [WeakManager](const FGuid& ItemId, const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		FCodeBOutOfRaidProfileStore HotbarStore(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBNormalContainerOwnerId);
		return HotbarStore.BindMatchedActiveRunHotbarSlot(
			WeakManager->CodeBNormalContainerRunId, ItemId, SlotIndex, OutProjection, &OutError);
	};
	HotbarPresentation.Unbind = [WeakManager](const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		FCodeBOutOfRaidProfileStore HotbarStore(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBNormalContainerOwnerId);
		return HotbarStore.UnbindMatchedActiveRunHotbarSlot(
			WeakManager->CodeBNormalContainerRunId, SlotIndex, OutProjection, &OutError);
	};
	FCodeBP3GroundDropPresentation GroundDropPresentation;
	GroundDropPresentation.RequestDrop = [WeakManager](
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutError)
	{
		return WeakManager.IsValid()
			&& WeakManager->RequestCodeBNormalContainerGroundDrop(Payload, OutError);
	};
	const bool bOpened = Host->OpenProfilePage(
		*CodeBNormalContainerRepository,
		PresentationLayout,
		[this](const demo_map_code_b::FCodeBSnapshot& PersistedSnapshot,
			const demo_map_code_b::FCodeBP2Command& AcceptedCommand, FString& CommitError)
		{
			FCodeBNormalContainerInteractionCommitRequest Request;
			Request.StorageRoot = ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString();
			Request.OwnerId = CodeBNormalContainerOwnerId;
			Request.RunInstanceId = CodeBNormalContainerRunId;
			Request.TargetId = CodeBNormalContainerTargetId;
			Request.DefinitionId = CodeBNormalContainerDefinitionId;
			Request.ExpectedP6SnapshotRevision = CodeBNormalContainerExpectedP6Revision;
			Request.ExpectedTargetRevision = CodeBNormalContainerExpectedTargetRevision;
			UGameInstance* CurrentGameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
			UCodeBP3UIHostSubsystem* CurrentHost = CurrentGameInstance
				? CurrentGameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
			Request.bTransientScopeCurrent = bCodeBNormalContainerOpen
				&& ActiveCodeBNormalContainer.IsValid()
				&& IsRegisteredCodeBNormalContainerTarget(ActiveCodeBNormalContainer.Get())
				&& CurrentHost && CurrentHost->IsHostEnabled()
				&& ProfilePreparationFlow && ProfilePreparationFlow->GetSession()
				&& ProfilePreparationFlow->GetPhase() == Edemo_mapProfilePreparationFlowPhase::RunActive
				&& ProfilePreparationFlow->GetStartedRunId() == Request.RunInstanceId
				&& ProfilePreparationFlow->GetSession()->GetSnapshot().ProfileId == Request.OwnerId
				&& ProfilePreparationFlow->GetSession()->GetSnapshot().ActiveRunId == Request.RunInstanceId;
			FCodeBNormalContainerInteractionCommitResult Result;
			const bool bCommitted = FCodeBRunItemInteractionDomain::CommitAcceptedNormalContainer(
				Request, AcceptedCommand, PersistedSnapshot, Result, CommitError);
			if (bCommitted)
			{
				ApplyCodeBRunItemInteractionResult(Result);
			}
			return bCommitted;
		},
		[WeakManager]()
		{
			if (WeakManager.IsValid())
			{
				WeakManager->CloseCodeBNormalContainerPage(TEXT("PageClosed"));
			}
		},
		true,
		&Presentation,
		nullptr,
		&HotbarPresentation,
		&GroundDropPresentation);
	if (!bOpened)
	{
		CodeBNormalContainerRepository.Reset();
		CodeBNormalContainerExpectedP6Revision = INDEX_NONE;
		CodeBNormalContainerExpectedTargetRevision = INDEX_NONE;
		OutFeedback = TEXT("P10 could not open the reused production P3/P4 two-column Host.");
		return false;
	}
	bCodeBNormalContainerOpen = true;
	CodeBNormalContainerActionId.Invalidate();
	return true;
}

bool Ademo_mapV3ProgressionManager::BeginCodeBNormalContainerItemSearch(
	const FGuid& ItemId,
	FCodeBNormalContainerProjection& OutProjection,
	FString& OutFeedback)
{
	OutProjection = FCodeBNormalContainerProjection();
	OutFeedback.Reset();
	if (!bCodeBNormalContainerOpen || !ActiveCodeBNormalContainer.IsValid()
		|| !CodeBNormalContainerOwnerId.IsValid() || !CodeBNormalContainerRunId.IsValid())
	{
		OutFeedback = TEXT("P10 item search requires the exact open normal-container page.");
		return false;
	}
	const FCodeBNormalContainerActionResult Search =
		FCodeBOutOfRaidProfileStore::BeginMatchedRunNormalContainerItemSearch(
			ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString(),
			CodeBNormalContainerOwnerId, CodeBNormalContainerRunId,
			CodeBNormalContainerTargetId, CodeBNormalContainerDefinitionId, ItemId);
	if (!Search.IsCommitted())
	{
		OutFeedback = Search.Diagnostic;
		return false;
	}
	CodeBNormalContainerActionId = Search.Projection.ActiveActionId;
	CodeBNormalContainerExpectedTargetRevision = Search.Projection.Revision;
	ActiveCodeBNormalContainer->ScheduleSearchCompletion(
		CodeBNormalContainerActionId,
		FCodeBNormalContainerInteractionTiming::ItemSearchSeconds);
	OutProjection = Search.Projection;
	return true;
}

void Ademo_mapV3ProgressionManager::CompleteCodeBNormalContainerAction(
	Ademo_mapCodeBNormalContainerActor* Container,
	const FGuid& ActionId,
	const bool bSearchAction)
{
	if (!Container || Container != ActiveCodeBNormalContainer.Get()
		|| !ActionId.IsValid() || ActionId != CodeBNormalContainerActionId)
	{
		return;
	}
	const FCodeBNormalContainerActionResult Completion = bSearchAction
		? FCodeBOutOfRaidProfileStore::CompleteMatchedRunNormalContainerItemSearch(
			ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString(),
			CodeBNormalContainerOwnerId, CodeBNormalContainerRunId, CodeBNormalContainerTargetId,
			CodeBNormalContainerDefinitionId, ActionId)
		: FCodeBOutOfRaidProfileStore::CompleteMatchedRunNormalContainerOpen(
			ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString(),
			CodeBNormalContainerOwnerId, CodeBNormalContainerRunId, CodeBNormalContainerTargetId,
			CodeBNormalContainerDefinitionId, ActionId);
	CodeBNormalContainerActionId.Invalidate();
	if (!Completion.IsCommitted())
	{
		return;
	}
	if (bSearchAction)
	{
		CodeBNormalContainerExpectedTargetRevision = Completion.Projection.Revision;
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
			{
				Host->UpdateNormalContainerProjection(Completion.Projection);
			}
		}
		return;
	}
	FString OpenFeedback;
	OpenCodeBNormalContainerPage(Container, Completion.Projection, OpenFeedback);
}

bool Ademo_mapV3ProgressionManager::IsCodeBNormalContainerActionStillValid(
	const Ademo_mapCodeBNormalContainerActor* Container,
	const FGuid& ActionId) const
{
	if (!Container || Container != ActiveCodeBNormalContainer.Get()
		|| !ActionId.IsValid() || ActionId != CodeBNormalContainerActionId
		|| !ProfilePreparationFlow || ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()
		? ProfilePreparationFlow->GetSession()->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	if (Snapshot.ProfileId != CodeBNormalContainerOwnerId
		|| Snapshot.ActiveRunId != CodeBNormalContainerRunId
		|| ProfilePreparationFlow->GetStartedRunId() != CodeBNormalContainerRunId)
	{
		return false;
	}
	const APawn* Pawn = PlayerPawn.Get();
	return Pawn && FVector::Dist(Pawn->GetActorLocation(), Container->GetInteractionLocation())
		<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
}

void Ademo_mapV3ProgressionManager::InterruptCodeBNormalContainerAction(
	Ademo_mapCodeBNormalContainerActor* Container,
	const FString& Reason)
{
	if (!Container || Container != ActiveCodeBNormalContainer.Get())
	{
		return;
	}
	FCodeBNormalContainerActionResult Interrupted;
	if (CodeBNormalContainerActionId.IsValid() && ProfilePreparationFlow)
	{
		Interrupted = FCodeBOutOfRaidProfileStore::InterruptMatchedRunNormalContainerAction(
			ProfilePreparationFlow->GetStorageRoot(), CodeBNormalContainerOwnerId,
			CodeBNormalContainerRunId, CodeBNormalContainerTargetId,
			CodeBNormalContainerDefinitionId, Reason);
		CodeBNormalContainerActionId.Invalidate();
		Container->ClearPendingAction();
		if (bCodeBNormalContainerOpen && Interrupted.IsCommitted())
		{
			if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
			{
				if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
				{
					Host->UpdateNormalContainerProjection(Interrupted.Projection);
				}
			}
		}
	}
	if (Reason == TEXT("ActorDestroyed"))
	{
		CloseCodeBNormalContainerPage(Reason);
	}
}

void Ademo_mapV3ProgressionManager::CloseCodeBNormalContainerPage(const FString& Reason)
{
	if (CodeBNormalContainerActionId.IsValid() && ActiveCodeBNormalContainer.IsValid())
	{
		InterruptCodeBNormalContainerAction(ActiveCodeBNormalContainer.Get(), Reason);
	}
	const bool bWasOpen = bCodeBNormalContainerOpen;
	bCodeBNormalContainerOpen = false;
	if (bWasOpen)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>();
				Host && Host->IsHostEnabled())
			{
				Host->ClosePage();
			}
		}
	}
	CodeBNormalContainerRepository.Reset();
	CodeBNormalContainerOwnerId.Invalidate();
	CodeBNormalContainerRunId.Invalidate();
	CodeBNormalContainerTargetId.Invalidate();
	CodeBNormalContainerDefinitionId = NAME_None;
	CodeBNormalContainerActionId.Invalidate();
	CodeBNormalContainerExpectedP6Revision = INDEX_NONE;
	CodeBNormalContainerExpectedTargetRevision = INDEX_NONE;
	ActiveCodeBNormalContainer.Reset();
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::RequestCodeBBodyContainerInteract(
	Ademo_mapCorpseContainerActor* Corpse)
{
	if (!Corpse || Corpse->GetCodeBBodyTargetIdentity() != GCodeBBodyContainerTargetIdentity)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidWorldBinding,
			TEXT("P12 only binds the one static Low Skirmisher corpse identity."));
	}
	if (bInventoryOpen || bCodeBActiveRunInventoryOpen || bSearchContainerOpen || bCodeBNormalContainerOpen
		|| (bCodeBBodyContainerOpen && ActiveCodeBBodyContainer.Get() != Corpse))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			TEXT("A different modal inventory or container surface is already open."));
	}
	if (bCodeBBodyContainerOpen && ActiveCodeBBodyContainer.Get() == Corpse)
	{
		return Fdemo_mapItemOperationResult::Success(
			FGuid(), GCodeBBodyContainerDefinitionId, NAME_None, Corpse->GetCodeBBodyTargetIdentity());
	}
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("P12 body interaction requires the current Code A Run to be active."));
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	const FGuid RunId = ProfilePreparationFlow->GetStartedRunId();
	const FGuid BodyTargetId = CodeBBodyContainerTargetGuid(GCodeBBodyContainerTargetIdentity);
	if (!Snapshot.ProfileId.IsValid() || !RunId.IsValid() || Snapshot.ActiveRunId != RunId || !BodyTargetId.IsValid())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidRunId,
			TEXT("P12 body interaction rejected an unstable Owner/Run/body identity."));
	}
	FCodeBBodyContainerProjection Existing;
	FString ProjectionError;
	if (!FCodeBOutOfRaidProfileStore::TryGetMatchedRunBodyContainerProjection(
		ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, BodyTargetId,
		Existing, &ProjectionError)
		|| Existing.DefinitionId != GCodeBBodyContainerDefinitionId)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			ProjectionError.IsEmpty() ? TEXT("P12 body interaction found no exact existing P11 projection.") : ProjectionError);
	}
	const bool bLocallyTrackedPendingAction = ActiveCodeBBodyContainer.Get() == Corpse
		&& CodeBBodyContainerOwnerId == Snapshot.ProfileId && CodeBBodyContainerRunId == RunId
		&& CodeBBodyContainerTargetId == BodyTargetId && CodeBBodyContainerActionId.IsValid();
	FCodeBBodyContainerActionResult Open = FCodeBOutOfRaidProfileStore::BeginMatchedRunBodyContainerOpen(
		ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, BodyTargetId,
		GCodeBBodyContainerDefinitionId);
	if (Open.Status == ECodeBBodyContainerActionStatus::AlreadyInRequestedState
		&& Open.Projection.ActiveActionId.IsValid() && !bLocallyTrackedPendingAction)
	{
		const FCodeBBodyContainerActionResult Recovered =
			FCodeBOutOfRaidProfileStore::InterruptMatchedRunBodyContainerAction(
				ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, BodyTargetId,
				GCodeBBodyContainerDefinitionId, TEXT("RecoveredStaleRuntimeAction"));
		if (!Recovered.IsCommitted())
		{
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InteractionBlocked, Recovered.Diagnostic);
		}
		Open = FCodeBOutOfRaidProfileStore::BeginMatchedRunBodyContainerOpen(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunId, BodyTargetId,
			GCodeBBodyContainerDefinitionId);
	}
	if (!Open.IsCommitted())
	{
		return Fdemo_mapItemOperationResult::Failure(
			Open.Status == ECodeBBodyContainerActionStatus::ActiveSessionNotCommitted
				|| Open.Status == ECodeBBodyContainerActionStatus::NotEnrolled
				? Edemo_mapItemResultCode::RunNotActive : Edemo_mapItemResultCode::InteractionBlocked,
			Open.Diagnostic);
	}
	CodeBBodyContainerOwnerId = Snapshot.ProfileId;
	CodeBBodyContainerRunId = RunId;
	CodeBBodyContainerTargetId = BodyTargetId;
	CodeBBodyContainerDefinitionId = GCodeBBodyContainerDefinitionId;
	ActiveCodeBBodyContainer = Corpse;
	CodeBBodyContainerActionId = Open.Projection.ActiveActionId;
	if (Open.Projection.State == ECodeBBodyContainerState::Open)
	{
		FString OpenFeedback;
		if (!OpenCodeBBodyContainerPage(Corpse, Open.Projection, OpenFeedback))
		{
			return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InteractionBlocked, OpenFeedback);
		}
	}
	else
	{
		Corpse->ScheduleCodeBBodyOpenCompletion(
			CodeBBodyContainerActionId, FCodeBBodyContainerInteractionTiming::OpenSeconds);
	}
	return Fdemo_mapItemOperationResult::Success(
		FGuid(), GCodeBBodyContainerDefinitionId, NAME_None, Corpse->GetCodeBBodyTargetIdentity());
}

bool Ademo_mapV3ProgressionManager::OpenCodeBBodyContainerPage(
	Ademo_mapCorpseContainerActor* Corpse,
	const FCodeBBodyContainerProjection& Projection,
	FString& OutFeedback)
{
	OutFeedback.Reset();
	if (!Corpse || !ProfilePreparationFlow || !CodeBBodyContainerOwnerId.IsValid()
		|| !CodeBBodyContainerRunId.IsValid() || Projection.OwnerId != CodeBBodyContainerOwnerId
		|| Projection.RunInstanceId != CodeBBodyContainerRunId
		|| Projection.BodyTargetId != CodeBBodyContainerTargetId
		|| Projection.DefinitionId != CodeBBodyContainerDefinitionId
		|| Projection.State != ECodeBBodyContainerState::Open)
	{
		OutFeedback = TEXT("P12 body page rejected a stale exact-body context.");
		return false;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UCodeBP3UIHostSubsystem* Host = GameInstance ? GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
	if (!Host || (Host->IsHostEnabled() && !bCodeBBodyContainerOpen))
	{
		OutFeedback = TEXT("P12 requires the existing production P3/P4 Host to be available.");
		return false;
	}
	FCodeBBodyContainerPresentationFrame Frame;
	if (!FCodeBRunItemInteractionDomain::BuildBodyContainerFrame(
		ProfilePreparationFlow->GetStorageRoot(), CodeBBodyContainerOwnerId,
		CodeBBodyContainerRunId, CodeBBodyContainerTargetId,
		CodeBBodyContainerDefinitionId, Frame, OutFeedback))
	{
		return false;
	}
	const FCodeBBodyContainerProjection& FreshProjection = Frame.Projection;
	CodeBBodyContainerRepository = MoveTemp(Frame.Shared.Repository);
	demo_map_code_b::FCodeBP2PlayerLayout PresentationLayout = MoveTemp(Frame.Shared.Layout);
	CodeBBodyContainerExpectedP6Revision = Frame.Shared.P6SnapshotRevision;
	CodeBBodyContainerExpectedTargetRevision = FreshProjection.Revision;
	const TWeakObjectPtr<Ademo_mapV3ProgressionManager> WeakManager(this);
	FCodeBP3BodyContainerPresentation Presentation;
	Presentation.TargetContainerId = FreshProjection.ContainerId;
	Presentation.Title = TEXT("尸体");
	Presentation.Projection = FreshProjection;
	Presentation.BeginItemSearch = [WeakManager](const demo_map_code_b::FCodeBP3SearchLocator& Locator, FCodeBBodyContainerProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !Locator.IsValid()
			|| Locator.TargetKind != demo_map_code_b::ECodeBP3SearchTargetKind::BodyContainer
			|| Locator.OwnerId != WeakManager->CodeBBodyContainerOwnerId
			|| Locator.RunInstanceId != WeakManager->CodeBBodyContainerRunId
			|| Locator.TargetId != WeakManager->CodeBBodyContainerTargetId
			|| Locator.TargetRevision != WeakManager->CodeBBodyContainerExpectedTargetRevision)
		{
			OutError = TEXT("P12 search locator is stale or belongs to another exact Run target.");
			return false;
		}
		FCodeBBodyContainerProjection Current;
		if (!FCodeBOutOfRaidProfileStore::TryGetMatchedRunBodyContainerProjection(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			Locator.OwnerId, Locator.RunInstanceId, Locator.TargetId, Current, &OutError)
			|| Current.Revision != Locator.TargetRevision
			|| Current.ActiveActionId != Locator.ActiveActionId)
		{
			if (OutError.IsEmpty()) OutError = TEXT("P12 search locator no longer matches persisted target state.");
			return false;
		}
		const FCodeBBodyContainerItemProjection* Item = Current.Items.FindByPredicate(
			[&Locator](const FCodeBBodyContainerItemProjection& Candidate)
			{
				return Candidate.ParentContainerId == Locator.ContainerId
					&& Candidate.SlotIndex == Locator.SlotIndex;
			});
		if (!Item || Item->Visibility != ECodeBBodyContainerVisibility::Hidden)
		{
			OutError = TEXT("P12 persisted search slot is absent or no longer Hidden.");
			return false;
		}
		return WeakManager->BeginCodeBBodyContainerItemSearch(Item->ItemId, OutProjection, OutError);
	};
	Presentation.Refresh = [WeakManager](FCodeBBodyContainerProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid() || !WeakManager->bCodeBBodyContainerOpen
			|| !WeakManager->CodeBBodyContainerOwnerId.IsValid()
			|| !WeakManager->CodeBBodyContainerRunId.IsValid()
			|| !WeakManager->CodeBBodyContainerTargetId.IsValid())
		{
			OutError = TEXT("P12 body projection refresh requires the exact open Owner/Run/BodyTarget.");
			return false;
		}
		return FCodeBOutOfRaidProfileStore::TryGetMatchedRunBodyContainerProjection(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBBodyContainerOwnerId, WeakManager->CodeBBodyContainerRunId,
			WeakManager->CodeBBodyContainerTargetId, OutProjection, &OutError);
	};
	FCodeBP3HotbarPresentation HotbarPresentation;
	HotbarPresentation.OwnerId = CodeBBodyContainerOwnerId;
	HotbarPresentation.RunInstanceId = CodeBBodyContainerRunId;
	HotbarPresentation.Projection = Frame.Shared.HotbarProjection;
	HotbarPresentation.Refresh = [WeakManager](FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		FCodeBOutOfRaidProfileStore HotbarStore(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBBodyContainerOwnerId);
		return HotbarStore.TryGetMatchedActiveRunHotbarProjection(
			WeakManager->CodeBBodyContainerRunId, OutProjection, &OutError);
	};
	HotbarPresentation.Bind = [WeakManager](const FGuid& ItemId, const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		FCodeBOutOfRaidProfileStore HotbarStore(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBBodyContainerOwnerId);
		return HotbarStore.BindMatchedActiveRunHotbarSlot(
			WeakManager->CodeBBodyContainerRunId, ItemId, SlotIndex, OutProjection, &OutError);
	};
	HotbarPresentation.Unbind = [WeakManager](const int32 SlotIndex, FCodeBHotbarProjection& OutProjection, FString& OutError)
	{
		if (!WeakManager.IsValid()) return false;
		FCodeBOutOfRaidProfileStore HotbarStore(
			WeakManager->ProfilePreparationFlow ? WeakManager->ProfilePreparationFlow->GetStorageRoot() : FString(),
			WeakManager->CodeBBodyContainerOwnerId);
		return HotbarStore.UnbindMatchedActiveRunHotbarSlot(
			WeakManager->CodeBBodyContainerRunId, SlotIndex, OutProjection, &OutError);
	};
	FCodeBP3GroundDropPresentation GroundDropPresentation;
	GroundDropPresentation.RequestDrop = [WeakManager](
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		FString& OutError)
	{
		return WeakManager.IsValid()
			&& WeakManager->RequestCodeBBodyGroundDrop(Payload, OutError);
	};
	const bool bOpened = Host->OpenProfilePage(
		*CodeBBodyContainerRepository, PresentationLayout,
		[this](const demo_map_code_b::FCodeBSnapshot& PersistedSnapshot,
			const demo_map_code_b::FCodeBP2Command& AcceptedCommand, FString& CommitError)
		{
			FCodeBBodyContainerInteractionCommitRequest Request;
			Request.StorageRoot = ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString();
			Request.OwnerId = CodeBBodyContainerOwnerId;
			Request.RunInstanceId = CodeBBodyContainerRunId;
			Request.TargetId = CodeBBodyContainerTargetId;
			Request.DefinitionId = CodeBBodyContainerDefinitionId;
			Request.ExpectedP6SnapshotRevision = CodeBBodyContainerExpectedP6Revision;
			Request.ExpectedTargetRevision = CodeBBodyContainerExpectedTargetRevision;
			UGameInstance* CurrentGameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
			UCodeBP3UIHostSubsystem* CurrentHost = CurrentGameInstance
				? CurrentGameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>() : nullptr;
			Request.bTransientScopeCurrent = bCodeBBodyContainerOpen
				&& ActiveCodeBBodyContainer.IsValid()
				&& ActiveCodeBBodyContainer->GetCodeBBodyTargetIdentity()
					== GCodeBBodyContainerTargetIdentity
				&& CurrentHost && CurrentHost->IsHostEnabled()
				&& ProfilePreparationFlow && ProfilePreparationFlow->GetSession()
				&& ProfilePreparationFlow->GetPhase() == Edemo_mapProfilePreparationFlowPhase::RunActive
				&& ProfilePreparationFlow->GetStartedRunId() == Request.RunInstanceId
				&& ProfilePreparationFlow->GetSession()->GetSnapshot().ProfileId == Request.OwnerId
				&& ProfilePreparationFlow->GetSession()->GetSnapshot().ActiveRunId == Request.RunInstanceId;
			FCodeBBodyContainerInteractionCommitResult Result;
			const bool bCommitted = FCodeBRunItemInteractionDomain::CommitAcceptedBodyContainer(
				Request, AcceptedCommand, PersistedSnapshot, Result, CommitError);
			if (bCommitted)
			{
				ApplyCodeBRunItemInteractionResult(Result);
			}
			return bCommitted;
		},
		[WeakManager]()
		{
			if (WeakManager.IsValid()) WeakManager->CloseCodeBBodyContainerPage(TEXT("PageClosed"));
		},
		true, nullptr, &Presentation, &HotbarPresentation, &GroundDropPresentation);
	if (!bOpened)
	{
		CodeBBodyContainerRepository.Reset();
		CodeBBodyContainerExpectedP6Revision = INDEX_NONE;
		CodeBBodyContainerExpectedTargetRevision = INDEX_NONE;
		OutFeedback = TEXT("P12 could not open the reused production P3/P4 two-column Host.");
		return false;
	}
	bCodeBBodyContainerOpen = true;
	CodeBBodyContainerActionId.Invalidate();
	return true;
}

bool Ademo_mapV3ProgressionManager::BeginCodeBBodyContainerItemSearch(
	const FGuid& ItemId,
	FCodeBBodyContainerProjection& OutProjection,
	FString& OutFeedback)
{
	OutProjection = FCodeBBodyContainerProjection();
	OutFeedback.Reset();
	if (!bCodeBBodyContainerOpen || !ActiveCodeBBodyContainer.IsValid()
		|| !CodeBBodyContainerOwnerId.IsValid() || !CodeBBodyContainerRunId.IsValid()
		|| !ItemId.IsValid())
	{
		OutFeedback = TEXT("P12 item reveal requires the exact open body-container page.");
		return false;
	}
	const FCodeBBodyContainerActionResult Search =
		FCodeBOutOfRaidProfileStore::BeginMatchedRunBodyContainerItemSearch(
			ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString(),
			CodeBBodyContainerOwnerId, CodeBBodyContainerRunId,
			CodeBBodyContainerTargetId, CodeBBodyContainerDefinitionId, ItemId);
	if (!Search.IsCommitted())
	{
		OutFeedback = Search.Diagnostic;
		return false;
	}
	CodeBBodyContainerActionId = Search.Projection.ActiveActionId;
	CodeBBodyContainerExpectedTargetRevision = Search.Projection.Revision;
	ActiveCodeBBodyContainer->ScheduleCodeBBodySearchCompletion(
		CodeBBodyContainerActionId,
		FCodeBBodyContainerInteractionTiming::ItemSearchSeconds);
	OutProjection = Search.Projection;
	return true;
}

void Ademo_mapV3ProgressionManager::CompleteCodeBBodyContainerAction(
	Ademo_mapCorpseContainerActor* Corpse,
	const FGuid& ActionId,
	const bool bSearchAction)
{
	if (!Corpse || Corpse != ActiveCodeBBodyContainer.Get()
		|| !ActionId.IsValid() || ActionId != CodeBBodyContainerActionId)
	{
		return;
	}
	const FCodeBBodyContainerActionResult Completion = bSearchAction
		? FCodeBOutOfRaidProfileStore::CompleteMatchedRunBodyContainerItemSearch(
			ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString(),
			CodeBBodyContainerOwnerId, CodeBBodyContainerRunId, CodeBBodyContainerTargetId,
			CodeBBodyContainerDefinitionId, ActionId)
		: FCodeBOutOfRaidProfileStore::CompleteMatchedRunBodyContainerOpen(
			ProfilePreparationFlow ? ProfilePreparationFlow->GetStorageRoot() : FString(),
			CodeBBodyContainerOwnerId, CodeBBodyContainerRunId, CodeBBodyContainerTargetId,
			CodeBBodyContainerDefinitionId, ActionId);
	CodeBBodyContainerActionId.Invalidate();
	if (!Completion.IsCommitted())
	{
		return;
	}
	if (bSearchAction)
	{
		CodeBBodyContainerExpectedTargetRevision = Completion.Projection.Revision;
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
			{
				Host->UpdateBodyContainerProjection(Completion.Projection);
			}
		}
		return;
	}
	FString OpenFeedback;
	OpenCodeBBodyContainerPage(Corpse, Completion.Projection, OpenFeedback);
}

bool Ademo_mapV3ProgressionManager::IsCodeBBodyContainerActionStillValid(
	const Ademo_mapCorpseContainerActor* Corpse,
	const FGuid& ActionId) const
{
	if (!Corpse || Corpse != ActiveCodeBBodyContainer.Get()
		|| !ActionId.IsValid() || ActionId != CodeBBodyContainerActionId
		|| !ProfilePreparationFlow
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return false;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()
		? ProfilePreparationFlow->GetSession()->GetSnapshot()
		: Fdemo_mapProfileSessionSnapshot();
	if (Snapshot.ProfileId != CodeBBodyContainerOwnerId
		|| Snapshot.ActiveRunId != CodeBBodyContainerRunId
		|| ProfilePreparationFlow->GetStartedRunId() != CodeBBodyContainerRunId)
	{
		return false;
	}
	const APawn* Pawn = PlayerPawn.Get();
	return Pawn && FVector::Dist(Pawn->GetActorLocation(), Corpse->GetInteractionLocation())
		<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
}

void Ademo_mapV3ProgressionManager::InterruptCodeBBodyContainerAction(
	Ademo_mapCorpseContainerActor* Corpse,
	const FString& Reason)
{
	if (!Corpse || Corpse != ActiveCodeBBodyContainer.Get())
	{
		return;
	}
	FCodeBBodyContainerActionResult Interrupted;
	if (CodeBBodyContainerActionId.IsValid() && ProfilePreparationFlow)
	{
		Interrupted = FCodeBOutOfRaidProfileStore::InterruptMatchedRunBodyContainerAction(
			ProfilePreparationFlow->GetStorageRoot(), CodeBBodyContainerOwnerId,
			CodeBBodyContainerRunId, CodeBBodyContainerTargetId,
			CodeBBodyContainerDefinitionId, Reason);
		CodeBBodyContainerActionId.Invalidate();
		Corpse->ClearCodeBBodyPendingAction();
		if (bCodeBBodyContainerOpen && Interrupted.IsCommitted())
		{
			CodeBBodyContainerExpectedTargetRevision = Interrupted.Projection.Revision;
			if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
			{
				if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>())
				{
					Host->UpdateBodyContainerProjection(Interrupted.Projection);
				}
			}
		}
	}
	if (Reason == TEXT("ActorDestroyed") || Reason == TEXT("DistanceLostOrRunInvalid"))
	{
		CloseCodeBBodyContainerPage(Reason);
	}
}

void Ademo_mapV3ProgressionManager::CloseCodeBBodyContainerPage(const FString& Reason)
{
	if (CodeBBodyContainerActionId.IsValid() && ActiveCodeBBodyContainer.IsValid())
	{
		InterruptCodeBBodyContainerAction(ActiveCodeBBodyContainer.Get(), Reason);
	}
	const bool bWasOpen = bCodeBBodyContainerOpen;
	bCodeBBodyContainerOpen = false;
	if (bWasOpen)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UCodeBP3UIHostSubsystem* Host = GameInstance->GetSubsystem<UCodeBP3UIHostSubsystem>();
				Host && Host->IsHostEnabled())
			{
				Host->ClosePage();
			}
		}
	}
	CodeBBodyContainerRepository.Reset();
	CodeBBodyContainerOwnerId.Invalidate();
	CodeBBodyContainerRunId.Invalidate();
	CodeBBodyContainerTargetId.Invalidate();
	CodeBBodyContainerDefinitionId = NAME_None;
	CodeBBodyContainerActionId.Invalidate();
	CodeBBodyContainerExpectedP6Revision = INDEX_NONE;
	CodeBBodyContainerExpectedTargetRevision = INDEX_NONE;
	ActiveCodeBBodyContainer.Reset();
}

void Ademo_mapV3ProgressionManager::ObserveCodeBRunTerminalAfterCodeACommit(
	const FGuid& OwnerId,
	const FGuid& RunInstanceId,
	const ECodeBRunInventoryTerminalState TerminalState)
{
	if (!ProfilePreparationFlow || !OwnerId.IsValid() || !RunInstanceId.IsValid()
		|| TerminalState == ECodeBRunInventoryTerminalState::Unknown)
	{
		return;
	}
	// Closing the P7 host first is lifecycle/input cleanup only. It prevents a
	// stale projection, drag, preview, or cell callback from writing while the
	// independent durable P8 finalizer resolves the already-committed terminal.
	if (bCodeBActiveRunInventoryOpen
		&& CodeBActiveRunInventoryOwnerId == OwnerId
		&& CodeBActiveRunInventoryRunId == RunInstanceId)
	{
		CloseCodeBActiveRunInventory();
	}
	if ((bCodeBNormalContainerOpen || CodeBNormalContainerActionId.IsValid())
		&& CodeBNormalContainerOwnerId == OwnerId
		&& CodeBNormalContainerRunId == RunInstanceId)
	{
		CloseCodeBNormalContainerPage(TEXT("RunTerminal"));
	}
	if ((bCodeBBodyContainerOpen || CodeBBodyContainerActionId.IsValid())
		&& CodeBBodyContainerOwnerId == OwnerId
		&& CodeBBodyContainerRunId == RunInstanceId)
	{
		CloseCodeBBodyContainerPage(TEXT("RunTerminal"));
	}
	if (bCodeBWorldDropOpen && CodeBWorldDropOwnerId == OwnerId
		&& CodeBWorldDropRunId == RunInstanceId)
	{
		CloseCodeBWorldDropPage();
	}
	if (ProfilePreparationFlow->UsesShanmenItemLifecycle())
	{
		ClearCodeBWorldDropActors();
		UE_LOG(LogTemp, Display,
			TEXT("Shanmen.RunAuthority Event=PostTerminalNoLegacyWrite OwnerId=%s RunInstanceId=%s Terminal=%d"),
			*OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
			*RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			static_cast<int32>(TerminalState));
		return;
	}
	const FCodeBRunInventoryTerminalResult Terminal =
		FCodeBOutOfRaidProfileStore::NotifyCommittedRunTerminal(
			ProfilePreparationFlow->GetStorageRoot(), OwnerId, RunInstanceId,
			TerminalState);
	if (Terminal.Status == ECodeBRunInventoryTerminalStatus::Committed
		|| Terminal.Status == ECodeBRunInventoryTerminalStatus::AlreadyCommitted)
	{
		ClearCodeBWorldDropActors();
	}
	// Do not return, branch on, or propagate this result to Code A. The source
	// terminal transaction has already completed and must retain all prior
	// success, failure, map, player, HUD, Run Save, and cleanup semantics.
	UE_LOG(LogTemp, Display,
		TEXT("CodeB.P8.TerminalObserver Event=PostCodeACommit Status=%d OwnerId=%s RunInstanceId=%s Terminal=%d Diagnostic=%s"),
		static_cast<int32>(Terminal.Status),
		*OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		static_cast<int32>(TerminalState), *Terminal.Diagnostic);
}

void Ademo_mapV3ProgressionManager::ObserveCodeBBodyContainerAfterCodeADeath(
	const Fdemo_mapM01EnemyDefinition& EnemyDefinition)
{
	// This is intentionally a one-way, best-effort post-death observer. Its
	// return is never consumed by combat, death visuals, AI, reward planning,
	// map lifetime, or Code A Run authority.
	if (EnemyDefinition.EncounterId != GCodeBBodyContainerEncounterId
		|| EnemyDefinition.SpawnMarkerId != GCodeBBodyContainerSpawnId
		|| !ProfilePreparationFlow || !ProfilePreparationFlow->GetSession()
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive)
	{
		return;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot =
		ProfilePreparationFlow->GetSession()->GetSnapshot();
	const FGuid RunInstanceId = ProfilePreparationFlow->GetStartedRunId();
	const FGuid BodyTargetId = CodeBBodyContainerTargetGuid(GCodeBBodyContainerTargetIdentity);
	if (!Snapshot.ProfileId.IsValid() || !RunInstanceId.IsValid()
		|| Snapshot.ActiveRunId != RunInstanceId || !BodyTargetId.IsValid())
	{
		return;
	}
	FCodeBBodyContainerDeathReceipt DeathReceipt;
	DeathReceipt.OwnerId = Snapshot.ProfileId;
	DeathReceipt.RunInstanceId = RunInstanceId;
	DeathReceipt.BodyTargetId = BodyTargetId;
	DeathReceipt.DefinitionId = GCodeBBodyContainerDefinitionId;
	DeathReceipt.StaticSpawnIdentity = GCodeBBodyContainerSpawnId;
	DeathReceipt.SpawnOrdinal = GCodeBBodyContainerSpawnOrdinal;
	DeathReceipt.DeathReceiptId = CodeBBodyContainerDeathReceiptGuid(
		DeathReceipt.OwnerId, DeathReceipt.RunInstanceId, DeathReceipt.BodyTargetId,
		DeathReceipt.DefinitionId);
	DeathReceipt.CommittedUtc = FDateTime::UtcNow().ToIso8601();
	const FCodeBBodyContainerMaterializationResult Result =
		FCodeBOutOfRaidProfileStore::MaterializeMatchedRunBodyContainerOnDeath(
			ProfilePreparationFlow->GetStorageRoot(), Snapshot.ProfileId, RunInstanceId,
			BodyTargetId, GCodeBBodyContainerDefinitionId, DeathReceipt);
	UE_LOG(LogTemp, Display,
		TEXT("CodeB.P11.BodyDeathObserver Event=PostCodeADeath Status=%d OwnerId=%s RunInstanceId=%s BodyTarget=%s Spawn=%s Ordinal=%d Diagnostic=%s"),
		static_cast<int32>(Result.Status),
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*GCodeBBodyContainerTargetIdentity.ToString(),
		*GCodeBBodyContainerSpawnId.ToString(),
		GCodeBBodyContainerSpawnOrdinal, *Result.Diagnostic);
}

void Ademo_mapV3ProgressionManager::ReturnToSectNavigation()
{
	ShowSectNavigation();
	if (SectNavigationWidget)
	{
		if (bReturnToTeleportAfterPreparation)
		{
			SectNavigationWidget->ShowTeleportPage();
		}
		else
		{
			SectNavigationWidget->ShowHomePage();
		}
	}
	bReturnToTeleportAfterPreparation = false;
}

void Ademo_mapV3ProgressionManager::HideProfilePreparation()
{
	if (ProfilePreparationWidget)
	{
		ProfilePreparationWidget->RemoveFromParent();
	}
	if (SectNavigationWidget)
	{
		SectNavigationWidget->RemoveFromParent();
	}
}

void Ademo_mapV3ProgressionManager::DeactivateProfileWorld()
{
	SetFocusedActor(nullptr);
	if (SpiritStonePickup.IsValid())
	{
		SpiritStonePickup->Destroy();
		SpiritStonePickup.Reset();
	}
	for (const TWeakObjectPtr<Ademo_mapSpiritStonePickup>& Pickup : M01SpiritStonePickups)
	{
		if (Pickup.IsValid())
		{
			Pickup->Destroy();
		}
	}
	M01SpiritStonePickups.Reset();
	M01SpiritStoneSpawnSourceIds.Reset();
	DestroyRuntimeContainers(TEXT("ProfileWorldDeactivated"));
	InitialWorldItems.Reset();
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		Mode->DeactivateV3MissionContentForPreparation();
	}
	if (Items.IsValid())
	{
		Items->TeardownWorld(GetWorld());
	}
	bProfileWorldActive = false;
}

void Ademo_mapV3ProgressionManager::DestroyRuntimeContainers(const FString& Reason)
{
	if (bCodeBNormalContainerOpen || CodeBNormalContainerActionId.IsValid())
	{
		CloseCodeBNormalContainerPage(Reason);
	}
	if (bCodeBBodyContainerOpen || CodeBBodyContainerActionId.IsValid())
	{
		CloseCodeBBodyContainerPage(Reason);
	}
	for (const TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>& Target : SpawnedCodeBNormalContainerTargets)
	{
		if (Target.IsValid())
		{
			Target->Destroy();
		}
	}
	SpawnedCodeBNormalContainerTargets.Reset();
	CodeBNormalContainerTargets.Reset();
	CloseSearchContainer(Reason, true);
	DestroyEnemyEncounterContent();
	RewardGenerationSession.Reset();
	RewardAffixPityLedger.Reset();
	bM01RewardContentActive = false;
	M01RewardRunId.Invalidate();
	const TArray<TWeakObjectPtr<Ademo_mapLootChest>> ChestsToDestroy = Chests;
	const TArray<TWeakObjectPtr<Ademo_mapCorpseContainerActor>> CorpsesToDestroy = Corpses;
	Chests.Reset();
	Corpses.Reset();
	CorpseLootSourceIds.Reset();
	for (const TWeakObjectPtr<Ademo_mapLootChest>& Chest : ChestsToDestroy)
	{
		if (Chest.IsValid())
		{
			Chest->Destroy();
		}
	}
	for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Corpse : CorpsesToDestroy)
	{
		if (Corpse.IsValid())
		{
			Corpse->Destroy();
		}
	}
}

bool Ademo_mapV3ProgressionManager::InitializeM01RewardContent()
{
	if (!GetWorld() || !Items.IsValid() || !PlayerPawn.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active)
	{
		return false;
	}
	const FGuid ActiveRunId = Items->GetActiveRunId();
	if (bM01RewardContentActive && M01RewardRunId == ActiveRunId
		&& Chests.Num() == Fdemo_mapM01RewardDistribution::TotalContainerCount)
	{
		return true;
	}
	if (bM01RewardContentActive || !Chests.IsEmpty() || !Corpses.IsEmpty())
	{
		DestroyRuntimeContainers(TEXT("M01RewardRunRebind"));
	}

	FString ValidationError;
	if (!Fdemo_mapM01RewardDistribution::Validate(&ValidationError))
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("M01_REWARD_DISTRIBUTION: validation failed: %s"),
			*ValidationError);
		return false;
	}

	TMap<FName, Ademo_mapM01Marker*> Anchors;
	for (TActorIterator<Ademo_mapM01Marker> It(GetWorld()); It; ++It)
	{
		if (It->GetMarkerType() == Edemo_mapM01MarkerType::ResourceCluster
			|| It->GetMarkerType() == Edemo_mapM01MarkerType::Boss)
		{
			Anchors.Add(It->GetStableId(), *It);
		}
	}
	static const FName RequiredAnchors[] = {
		TEXT("M01.Resource.TIER_1.Cluster.01"),
		TEXT("M01.Resource.TIER_2.Cluster.01"),
		TEXT("M01.Resource.TIER_3.Cluster.01"),
		TEXT("M01.Boss.Main")
	};
	for (const FName AnchorId : RequiredAnchors)
	{
		if (!Anchors.Contains(AnchorId))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("M01_REWARD_DISTRIBUTION: missing persisted anchor=%s."),
				*AnchorId.ToString());
			return false;
		}
	}

	UNavigationSystemV1* Navigation =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Navigation)
	{
		return false;
	}
	TArray<TPair<const Fdemo_mapM01RewardSlot*, FTransform>> Resolved;
	TArray<FVector> UsedLocations;
	UsedLocations.Add(PlayerPawn->GetActorLocation());
	static const FVector2D Directions[] = {
		FVector2D(1.0f, 0.0f), FVector2D(0.70710678f, 0.70710678f),
		FVector2D(0.0f, 1.0f), FVector2D(-0.70710678f, 0.70710678f),
		FVector2D(-1.0f, 0.0f), FVector2D(-0.70710678f, -0.70710678f),
		FVector2D(0.0f, -1.0f), FVector2D(0.70710678f, -0.70710678f)
	};
	for (const Fdemo_mapM01RewardSlot& Slot :
		Fdemo_mapM01RewardDistribution::GetSlots())
	{
		if (!Slot.IsContainer()) continue;
		Ademo_mapM01Marker* const* FoundAnchor = Anchors.Find(Slot.AnchorMarkerId);
		const Ademo_mapM01Marker* Anchor = FoundAnchor ? *FoundAnchor : nullptr;
		const Fdemo_mapRewardSourceProjection Projection =
			Fdemo_mapM01RewardDistribution::BuildProjection(Slot);
		if (!Anchor || !Projection.IsValid())
		{
			DestroyRuntimeContainers(TEXT("M01RewardDeclarationRejected"));
			return false;
		}
		const FVector Desired = Anchor->GetActorLocation()
			+ Anchor->GetActorTransform().TransformVectorNoScale(Slot.LocalOffset);
		FNavLocation Projected;
		bool bResolved = false;
		for (int32 Attempt = 0; Attempt < 25 && !bResolved; ++Attempt)
		{
			FVector Candidate = Desired;
			if (Attempt > 0)
			{
				const int32 DirectionIndex = (Attempt - 1) % 8;
				const int32 Ring = ((Attempt - 1) / 8) + 1;
				Candidate += FVector(
					Directions[DirectionIndex].X,
					Directions[DirectionIndex].Y, 0.0f) * (180.0f * Ring);
			}
			FNavLocation CandidateProjected;
			if (!Navigation->ProjectPointToNavigation(
				Candidate, CandidateProjected, FVector(260.0f, 260.0f, 420.0f)))
			{
				continue;
			}
			const bool bSeparated = !UsedLocations.ContainsByPredicate(
				[&CandidateProjected](const FVector& Existing)
				{
					return FVector::DistSquared2D(Existing, CandidateProjected.Location)
						< FMath::Square(145.0f);
				});
			UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(
				GetWorld(), PlayerPawn->GetActorLocation(),
				CandidateProjected.Location, PlayerPawn.Get());
			if (bSeparated && Path && Path->IsValid() && !Path->IsPartial())
			{
				Projected = CandidateProjected;
				bResolved = true;
			}
		}
		if (!bResolved)
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("M01_REWARD_DISTRIBUTION: placement failed slot=%s."),
				*Slot.SlotId.ToString());
			DestroyRuntimeContainers(TEXT("M01RewardPlacementRejected"));
			return false;
		}
		UsedLocations.Add(Projected.Location);
		Resolved.Emplace(&Slot, FTransform(Anchor->GetActorRotation(), Projected.Location));
	}
	if (Resolved.Num() != Fdemo_mapM01RewardDistribution::TotalContainerCount)
	{
		DestroyRuntimeContainers(TEXT("M01RewardPlacementCountMismatch"));
		return false;
	}

	for (const auto& Entry : Resolved)
	{
		const Fdemo_mapM01RewardSlot& Slot = *Entry.Key;
		const Fdemo_mapRewardSourceProjection Projection =
			Fdemo_mapM01RewardDistribution::BuildProjection(Slot);
		FVector SeparatedLocation;
		if (!Items->ResolveSafeWorldLocation(
			GetWorld(),
			Entry.Value.GetLocation(),
			nullptr,
			SeparatedLocation,
			Slot.SlotId,
			nullptr,
			66.0f))
		{
			DestroyRuntimeContainers(TEXT("M01RewardWorldSeparationRejected"));
			return false;
		}
		FTransform SpawnTransform = Entry.Value;
		SpawnTransform.SetLocation(SeparatedLocation);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ademo_mapLootChest* Chest = GetWorld()->SpawnActor<Ademo_mapLootChest>(
			Ademo_mapLootChest::StaticClass(), SpawnTransform, Params);
		if (!Chest)
		{
			DestroyRuntimeContainers(TEXT("M01RewardSpawnFailed"));
			return false;
		}
		Chest->ConfigureRewardProjection(Slot.ContainerOrdinal, Projection);
		if (!Chest->InitializeChest(this, Items.Get(), ActiveRunId))
		{
			Chest->Destroy();
			DestroyRuntimeContainers(TEXT("M01RewardMaterializationFailed"));
			return false;
		}
		Chests.Add(Chest);
	}
	bM01RewardContentActive = true;
	M01RewardRunId = ActiveRunId;
	const Fdemo_mapM01RewardDistributionCounts Counts =
		Fdemo_mapM01RewardDistribution::Count();
	UE_LOG(Logdemo_map, Log,
		TEXT("M01_REWARD_DISTRIBUTION_READY run=%s sources=149 containers=%d enemies=14 low=4 mid=6 elite=3 boss=1 tier1=48 tier2=48 tier3=24 high_value=15 wood=60 ore=60 base=%lld."),
		*ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		Chests.Num(), Counts.BaseSourceValue);
	return true;
}

void Ademo_mapV3ProgressionManager::DestroyEnemyEncounterContent()
{
	const TArray<TWeakObjectPtr<AActor>> ActorsToDestroy = EnemyActors;
	EnemyActors.Reset();
	NavigableEnemySpawnMarkerIds.Reset();
	for (const TWeakObjectPtr<AActor>& Actor : ActorsToDestroy)
	{
		if (Actor.IsValid())
		{
			Actor->Destroy();
		}
	}
}

bool Ademo_mapV3ProgressionManager::InitializeEnemyEncounterContent()
{
	FString ValidationError;
	if (!Fdemo_mapItemDefinitions::Validate(&ValidationError)
		|| !Fdemo_mapEnemyEncounterConfig::Validate(&ValidationError)
		|| !Fdemo_mapRewardFullMapDistribution::Validate(
			&ValidationError)
		|| !GetWorld()
		|| !PlayerPawn.IsValid())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P7_ENCOUNTER_CONFIG: rejected: %s"),
			*ValidationError);
		return false;
	}

	int32 ExistingHostileProjectionCount = 0;
	for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
	{
		++ExistingHostileProjectionCount;
	}
	for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
	{
		++ExistingHostileProjectionCount;
	}
	for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
	{
		++ExistingHostileProjectionCount;
	}
	if (ExistingHostileProjectionCount != 0)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P7_ENCOUNTER_CONFIG: V3 Manager requires an empty hostile projection surface; found=%d."),
			ExistingHostileProjectionCount);
		return false;
	}

	TArray<Ademo_mapEncounterMarker*> Markers;
	for (TActorIterator<Ademo_mapEncounterMarker> It(GetWorld()); It; ++It)
	{
		Markers.Add(*It);
	}
	UNavigationSystemV1* Navigation =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Navigation)
	{
		UE_LOG(Logdemo_map, Error, TEXT("P7_ENCOUNTER_CONFIG: NavigationSystemV1 is unavailable."));
		return false;
	}
	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		Navigation->OnNavigationBoundsUpdated(*It);
	}
	Navigation->GetDefaultNavDataInstance(FNavigationSystem::Create);
	Navigation->Build();

	Ademo_mapEnemyCharacter* PrimaryMelee = nullptr;
	Ademo_mapRangedEnemyCharacter* PrimaryRanged = nullptr;
	Ademo_mapHeavyEnemyCharacter* PrimaryHeavy = nullptr;
	int32 BossProjectionCount = 0;
	for (const Fdemo_mapFullMapRewardSlot& Slot :
		Fdemo_mapRewardFullMapDistribution::GetSlots())
	{
		if (!Slot.IsEnemy())
		{
			continue;
		}
		const Fdemo_mapEnemyEncounterSpawnRecord& Record =
			Slot.EnemyRecord;
		BossProjectionCount += Slot.RewardClass
				== Edemo_mapFullMapRewardClass::Boss
			? 1 : 0;
		Ademo_mapEncounterMarker* const* FoundSourceMarker =
			Markers.FindByPredicate(
				[&Record](const Ademo_mapEncounterMarker* Marker)
				{
					if (!Marker
						|| (Record.SourceMarkerIndex != INDEX_NONE
							&& Marker->GetMarkerIndex()
								!= Record.SourceMarkerIndex))
					{
						return false;
					}
					if (Record.SourceMarkerType == TEXT("MeleeEnemySpawn"))
					{
						return Marker->GetMarkerType()
							== Edemo_mapEncounterMarkerType::MeleeEnemySpawn;
					}
					if (Record.SourceMarkerType == TEXT("RangedEnemySpawn"))
					{
						return Marker->GetMarkerType()
							== Edemo_mapEncounterMarkerType::RangedEnemySpawn;
					}
					if (Record.SourceMarkerType == TEXT("HeavyEnemySpawn"))
					{
						return Marker->GetMarkerType()
							== Edemo_mapEncounterMarkerType::HeavyEnemySpawn;
					}
					if (Record.SourceMarkerType
						== TEXT("TrainingTargetSpawn"))
					{
						return Marker->GetMarkerType()
							== Edemo_mapEncounterMarkerType::TrainingTargetSpawn;
					}
					return false;
				});
		const Ademo_mapEncounterMarker* SourceMarker =
			FoundSourceMarker ? *FoundSourceMarker : nullptr;
		if (!SourceMarker)
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P7_ENCOUNTER_CONFIG: source marker missing encounter=%s source=%s index=%d."),
				*Record.Identity.EncounterId.ToString(),
				*Record.SourceMarkerType.ToString(),
				Record.SourceMarkerIndex);
			DestroyEnemyEncounterContent();
			return false;
		}

		const FVector DesiredLocation =
			SourceMarker->GetActorLocation()
			+ SourceMarker->GetActorTransform().TransformVectorNoScale(
				Record.LocalOffset);
		FNavLocation Projected;
		if (!Navigation->ProjectPointToNavigation(
				DesiredLocation,
				Projected,
				FVector(300.0f, 300.0f, 300.0f)))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P7_ENCOUNTER_CONFIG: navigation projection failed marker=%s desired=(%.1f,%.1f,%.1f)."),
				*Record.Identity.SpawnMarkerId.ToString(),
				DesiredLocation.X,
				DesiredLocation.Y,
				DesiredLocation.Z);
			DestroyEnemyEncounterContent();
			return false;
		}
		UNavigationPath* Path =
			Navigation->FindPathToLocationSynchronously(
				GetWorld(),
				PlayerPawn->GetActorLocation(),
				Projected.Location,
				PlayerPawn.Get());
		if (!Path || !Path->IsValid() || Path->IsPartial())
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P7_ENCOUNTER_CONFIG: full navigation path failed marker=%s."),
				*Record.Identity.SpawnMarkerId.ToString());
			DestroyEnemyEncounterContent();
			return false;
		}
		NavigableEnemySpawnMarkerIds.Add(Record.Identity.SpawnMarkerId);
		const FRotator Rotation =
			(PlayerPawn->GetActorLocation() - Projected.Location).Rotation();
		const FTransform SpawnTransform(Rotation, Projected.Location);
		AActor* Spawned = nullptr;
		switch (Record.Archetype)
		{
		case Edemo_mapEnemyEncounterArchetype::MeleeStandard:
		case Edemo_mapEnemyEncounterArchetype::MeleeEnhanced:
		{
			Ademo_mapEnemyCharacter* Enemy =
				GetWorld()->SpawnActorDeferred<Ademo_mapEnemyCharacter>(
					Ademo_mapEnemyCharacter::StaticClass(),
					SpawnTransform,
					this,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Enemy
				&& Enemy->ConfigureEncounter(
					Record.Identity,
					Record.Tuning,
					Record.bEnhanced))
			{
				Spawned = UGameplayStatics::FinishSpawningActor(
					Enemy,
					SpawnTransform);
				if (Record.Archetype
					== Edemo_mapEnemyEncounterArchetype::MeleeStandard)
				{
					PrimaryMelee = Enemy;
				}
			}
			else if (Enemy)
			{
				Enemy->Destroy();
			}
			break;
		}
		case Edemo_mapEnemyEncounterArchetype::RangedStandard:
		case Edemo_mapEnemyEncounterArchetype::RangedEnhanced:
		{
			Ademo_mapRangedEnemyCharacter* Enemy =
				GetWorld()->SpawnActorDeferred<Ademo_mapRangedEnemyCharacter>(
					Ademo_mapRangedEnemyCharacter::StaticClass(),
					SpawnTransform,
					this,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Enemy
				&& Enemy->ConfigureEncounter(
					Record.Identity,
					Record.Tuning,
					Record.bEnhanced))
			{
				Spawned = UGameplayStatics::FinishSpawningActor(
					Enemy,
					SpawnTransform);
				if (Record.Archetype
					== Edemo_mapEnemyEncounterArchetype::RangedStandard)
				{
					PrimaryRanged = Enemy;
				}
			}
			else if (Enemy)
			{
				Enemy->Destroy();
			}
			break;
		}
		case Edemo_mapEnemyEncounterArchetype::MeleeHeavy:
		{
			Ademo_mapHeavyEnemyCharacter* Enemy =
				GetWorld()->SpawnActorDeferred<Ademo_mapHeavyEnemyCharacter>(
					Ademo_mapHeavyEnemyCharacter::StaticClass(),
					SpawnTransform,
					this,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Enemy
				&& Enemy->ConfigureEncounter(
					Record.Identity,
					Record.Tuning))
			{
				Spawned = UGameplayStatics::FinishSpawningActor(
					Enemy,
					SpawnTransform);
				PrimaryHeavy = Enemy;
			}
			else if (Enemy)
			{
				Enemy->Destroy();
			}
			break;
		}
		}
		if (!Spawned)
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P7_ENCOUNTER_CONFIG: actor materialization failed encounter=%s."),
				*Record.Identity.EncounterId.ToString());
			DestroyEnemyEncounterContent();
			return false;
		}
		EnemyActors.Add(Spawned);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_ENEMY_SPAWN slot=%s role=%s encounter=%s route=%s marker=%s loot=%s profile=%s base=%lld location=(%.1f,%.1f,%.1f)."),
			*Slot.SlotId.ToString(),
			*Slot.StableSourceRoleId.ToString(),
			*Record.Identity.EncounterId.ToString(),
			*Record.Identity.RouteId.ToString(),
			*Record.Identity.SpawnMarkerId.ToString(),
			*Record.Identity.LootTableId.ToString(),
			*Slot.BudgetProfileId.ToString(),
			Slot.BaseSourceValue,
			Projected.Location.X,
			Projected.Location.Y,
			Projected.Location.Z);
	}

	Ademo_mapGameMode* Mode =
		Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode());
	if (EnemyActors.Num()
			!= Fdemo_mapRewardFullMapDistribution::TotalEnemyCount
		|| NavigableEnemySpawnMarkerIds.Num()
			!= Fdemo_mapRewardFullMapDistribution::TotalEnemyCount
		|| BossProjectionCount
			!= Fdemo_mapRewardFullMapDistribution::BossCount
		|| !PrimaryMelee
		|| !PrimaryRanged
		|| !PrimaryHeavy
		|| !Mode)
	{
		DestroyEnemyEncounterContent();
		return false;
	}
	Mode->BindV3EnemyProjections(
		PrimaryMelee,
		PrimaryRanged,
		PrimaryHeavy);
	return true;
}

bool Ademo_mapV3ProgressionManager::InitializeCodeBNormalContainerTarget()
{
	if (!GetWorld() || !Items.IsValid() || !PlayerPawn.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active)
	{
		return false;
	}
	const FName RequiredTargetIds[] =
	{
		GCodeBNormalContainerPrimaryMapTargetId,
		GCodeBNormalContainerSecondaryMapTargetId
	};
	if (CodeBNormalContainerTargets.Num() == UE_ARRAY_COUNT(RequiredTargetIds))
	{
		bool bAllRegisteredTargetsRemainValid = true;
		for (const FName RequiredTargetId : RequiredTargetIds)
		{
			bAllRegisteredTargetsRemainValid &= CodeBNormalContainerTargets.ContainsByPredicate(
				[RequiredTargetId](const TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>& Target)
				{
					return Target.IsValid() && Target->GetMapTargetIdentity() == RequiredTargetId;
				});
		}
		if (bAllRegisteredTargetsRemainValid)
		{
			return true;
		}
	}
	for (const TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>& SpawnedTarget
		: SpawnedCodeBNormalContainerTargets)
	{
		if (SpawnedTarget.IsValid()) SpawnedTarget->Destroy();
	}
	CodeBNormalContainerTargets.Reset();
	SpawnedCodeBNormalContainerTargets.Reset();

	TMap<FName, Ademo_mapCodeBNormalContainerActor*> ExistingTargets;
	for (TActorIterator<Ademo_mapCodeBNormalContainerActor> It(GetWorld()); It; ++It)
	{
		const FName TargetIdentity = It->GetMapTargetIdentity();
		if (!IsCodeBNormalContainerMapTargetIdentity(TargetIdentity))
		{
			continue;
		}
		if (ExistingTargets.Contains(TargetIdentity))
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("CODEB_P10_BASIC_CACHE: duplicate map-authored target identity=%s."),
				*TargetIdentity.ToString());
			return false;
		}
		ExistingTargets.Add(TargetIdentity, *It);
	}
	if (ExistingTargets.Num() == UE_ARRAY_COUNT(RequiredTargetIds))
	{
		for (const FName TargetIdentity : RequiredTargetIds)
		{
			Ademo_mapCodeBNormalContainerActor* Target = ExistingTargets.FindRef(TargetIdentity);
			CodeBNormalContainerTargets.Add(Target);
			UE_LOG(Logdemo_map, Log,
				TEXT("CODEB_P57_BASIC_CACHE: ready target=%s location=%s source=MapAuthored."),
				*TargetIdentity.ToString(), *Target->GetActorLocation().ToCompactString());
		}
		return true;
	}

	const Ademo_mapM01Marker* Anchor = nullptr;
	for (TActorIterator<Ademo_mapM01Marker> It(GetWorld()); It; ++It)
	{
		if (It->GetStableId() == GCodeBNormalContainerAnchorId)
		{
			Anchor = *It;
			break;
		}
	}
	if (!Anchor)
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("CODEB_P10_BASIC_CACHE: missing static M01 anchor=%s."),
			*GCodeBNormalContainerAnchorId.ToString());
		return false;
	}

	for (int32 TargetIndex = 0; TargetIndex < UE_ARRAY_COUNT(RequiredTargetIds); ++TargetIndex)
	{
		const FName TargetIdentity = RequiredTargetIds[TargetIndex];
		Ademo_mapCodeBNormalContainerActor* Target = ExistingTargets.FindRef(TargetIdentity);
		FVector Location = Target ? Target->GetActorLocation() : FVector::ZeroVector;
		if (!Target)
		{
			const FVector Desired = Anchor->GetActorLocation()
				+ Anchor->GetActorTransform().TransformVectorNoScale(
					CodeBNormalContainerAnchorLocalOffset(TargetIdentity));
			if (!Items->ResolveSafeWorldLocation(
				GetWorld(), Desired, nullptr, Location, TargetIdentity, nullptr, 64.0f))
			{
				UE_LOG(Logdemo_map, Error,
					TEXT("CODEB_P57_BASIC_CACHE: no safe projection for target=%s anchor=%s."),
					*TargetIdentity.ToString(), *GCodeBNormalContainerAnchorId.ToString());
				for (const TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>& SpawnedTarget
					: SpawnedCodeBNormalContainerTargets)
				{
					if (SpawnedTarget.IsValid()) SpawnedTarget->Destroy();
				}
				CodeBNormalContainerTargets.Reset();
				SpawnedCodeBNormalContainerTargets.Reset();
				return false;
			}
			FActorSpawnParameters Params;
			Params.Name = FName(*FString::Printf(TEXT("M01_CodeBNormalContainer_BasicCache_%02d"), TargetIndex + 1));
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Target = GetWorld()->SpawnActor<Ademo_mapCodeBNormalContainerActor>(
				Ademo_mapCodeBNormalContainerActor::StaticClass(), Location, Anchor->GetActorRotation(), Params);
			if (Target)
			{
				Target->MapTargetIdentity = TargetIdentity;
			}
			if (!Target || Target->GetMapTargetIdentity() != TargetIdentity)
			{
				if (Target) Target->Destroy();
				for (const TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>& SpawnedTarget
					: SpawnedCodeBNormalContainerTargets)
				{
					if (SpawnedTarget.IsValid()) SpawnedTarget->Destroy();
				}
				CodeBNormalContainerTargets.Reset();
				SpawnedCodeBNormalContainerTargets.Reset();
				UE_LOG(Logdemo_map, Error,
					TEXT("CODEB_P57_BASIC_CACHE: target spawn did not retain identity=%s."),
					*TargetIdentity.ToString());
				return false;
			}
			SpawnedCodeBNormalContainerTargets.Add(Target);
		}
		CodeBNormalContainerTargets.Add(Target);
		UE_LOG(Logdemo_map, Log,
			TEXT("CODEB_P57_BASIC_CACHE: ready target=%s anchor=%s location=%s source=%s."),
			*TargetIdentity.ToString(), *GCodeBNormalContainerAnchorId.ToString(),
			*Location.ToCompactString(), ExistingTargets.Contains(TargetIdentity)
				? TEXT("MapAuthored") : TEXT("RuntimeAdapter"));
	}
	return true;
}

bool Ademo_mapV3ProgressionManager::IsRegisteredCodeBNormalContainerTarget(
	const Ademo_mapCodeBNormalContainerActor* Container) const
{
	if (!Container || !IsCodeBNormalContainerMapTargetIdentity(Container->GetMapTargetIdentity()))
	{
		return false;
	}
	return CodeBNormalContainerTargets.ContainsByPredicate(
		[Container](const TWeakObjectPtr<Ademo_mapCodeBNormalContainerActor>& Target)
		{
			return Target.Get() == Container;
		});
}

bool Ademo_mapV3ProgressionManager::InitializeWorldContent()
{
	Ademo_mapGameMode* ActiveMode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	if (ActiveMode && ActiveMode->IsM01ExpeditionMap())
	{
		const bool bM01Ready = ActiveMode->IsM01EnemyContentActive()
			&& ActiveMode->GetM01EnemyActorCount() == 14
			&& InitializeM01RewardContent();
		if (bM01Ready && !InitializeCodeBNormalContainerTarget())
		{
			// P10 is a normal world-interaction adapter. Its projection must never
			// roll back Code A's already-formal Run lifecycle.
			UE_LOG(Logdemo_map, Error,
				TEXT("CODEB_P10_BASIC_CACHE: production target projection unavailable; Code A Run remains authoritative."));
		}
		UE_LOG(Logdemo_map, Log,
			TEXT("M01_CONTENT_ISOLATION: legacy_v3_enemies=0 training_targets=0 final_reward_sources=%d legacy_exits=0 m01_enemies=%d."),
			bM01Ready ? Fdemo_mapM01RewardDistribution::TotalSlotCount : 0,
			ActiveMode->GetM01EnemyActorCount());
		return bM01Ready;
	}

	FString RewardValidationError;
	if (!Fdemo_mapRewardGenerationRegistry::Validate(
		&RewardValidationError)
		|| !Fdemo_mapRewardSourceProjectionRegistry::Validate(
			&RewardValidationError))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P1_REWARD_GENERATION: registry validation failed: %s"),
			*RewardValidationError);
		return false;
	}
	TArray<Ademo_mapV3ProgressionMarker*> Roots;
	TArray<Ademo_mapV3ProgressionMarker*> ChestMarkers;
	TArray<Ademo_mapV3ProgressionMarker*> WorldMarkers;
	int32 LootAreaCount = 0;
	int32 EnemyDropAreaCount = 0;
	for (TActorIterator<Ademo_mapV3ProgressionMarker> It(GetWorld()); It; ++It)
	{
		switch (It->GetMarkerType())
		{
		case Edemo_mapV3ProgressionMarkerType::ProgressionRoot: Roots.Add(*It); break;
		case Edemo_mapV3ProgressionMarkerType::Chest: ChestMarkers.Add(*It); break;
		case Edemo_mapV3ProgressionMarkerType::WorldItem: WorldMarkers.Add(*It); break;
		case Edemo_mapV3ProgressionMarkerType::LootDisplayArea: ++LootAreaCount; break;
		case Edemo_mapV3ProgressionMarkerType::EnemyDropValidation: ++EnemyDropAreaCount; break;
		}
	}
	ChestMarkers.Sort([](const Ademo_mapV3ProgressionMarker& A, const Ademo_mapV3ProgressionMarker& B){ return A.GetMarkerIndex() < B.GetMarkerIndex(); });
	WorldMarkers.Sort([](const Ademo_mapV3ProgressionMarker& A, const Ademo_mapV3ProgressionMarker& B){ return A.GetMarkerIndex() < B.GetMarkerIndex(); });
	if (Roots.Num() != 1 || ChestMarkers.Num() != 3 || WorldMarkers.Num() < 3 || LootAreaCount != 1 || EnemyDropAreaCount != 1)
	{
		UE_LOG(Logdemo_map, Error, TEXT("V3_WORLD_INTERACTION: marker validation failed roots=%d chests=%d world=%d loot_area=%d enemy_drop_area=%d."), Roots.Num(), ChestMarkers.Num(), WorldMarkers.Num(), LootAreaCount, EnemyDropAreaCount);
		return false;
	}
	if (!InitializeEnemyEncounterContent())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P7_ENCOUNTER_CONFIG: five-enemy product projection failed."));
		return false;
	}
	UNavigationSystemV1* Navigation =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	TArray<TPair<const Fdemo_mapFullMapRewardSlot*, FTransform>>
		ResolvedContainers;
	TArray<FVector> UsedContainerLocations;
	for (const Ademo_mapV3ProgressionMarker* WorldMarker : WorldMarkers)
	{
		if (WorldMarker)
		{
			UsedContainerLocations.Add(WorldMarker->GetActorLocation());
		}
	}
	if (!Navigation)
	{
		DestroyRuntimeContainers(TEXT("P8MissingNavigation"));
		return false;
	}
	for (const Fdemo_mapFullMapRewardSlot& Slot :
		Fdemo_mapRewardFullMapDistribution::GetSlots())
	{
		if (!Slot.IsContainer())
		{
			continue;
		}
		Ademo_mapV3ProgressionMarker* const* FoundAnchor =
			ChestMarkers.FindByPredicate(
				[&Slot](const Ademo_mapV3ProgressionMarker* Marker)
				{
					return Marker
						&& Marker->GetMarkerIndex()
							== Slot.SourceMarkerIndex;
				});
		const Ademo_mapV3ProgressionMarker* Anchor =
			FoundAnchor ? *FoundAnchor : nullptr;
		const Fdemo_mapRewardSourceProjection Projection =
			Fdemo_mapRewardFullMapDistribution::BuildProjection(
				Slot);
		if (!Anchor || !Projection.IsValid())
		{
			DestroyRuntimeContainers(
				TEXT("P8ContainerDeclarationRejected"));
			return false;
		}
		const FVector Desired =
			Anchor->GetActorLocation()
			+ Anchor->GetActorTransform().TransformVectorNoScale(
				Slot.LocalOffset);
		FNavLocation Projected;
		UNavigationPath* Path = nullptr;
		bool bSeparated = false;
		bool bResolved = false;
		int32 ResolvedAttempt = INDEX_NONE;
		static const FVector2D Directions[] = {
			FVector2D(1.0f, 0.0f),
			FVector2D(0.70710678f, 0.70710678f),
			FVector2D(0.0f, 1.0f),
			FVector2D(-0.70710678f, 0.70710678f),
			FVector2D(-1.0f, 0.0f),
			FVector2D(-0.70710678f, -0.70710678f),
			FVector2D(0.0f, -1.0f),
			FVector2D(0.70710678f, -0.70710678f)
		};
		for (int32 Attempt = 0; Attempt < 33; ++Attempt)
		{
			FVector Candidate = Desired;
			if (Attempt > 0)
			{
				const int32 DirectionIndex = (Attempt - 1) % 8;
				const int32 Ring = ((Attempt - 1) / 8) + 1;
				Candidate += FVector(
					Directions[DirectionIndex].X,
					Directions[DirectionIndex].Y,
					0.0f) * (260.0f * Ring);
			}
			FNavLocation CandidateProjected;
			if (!Navigation->ProjectPointToNavigation(
					Candidate,
					CandidateProjected,
					FVector(260.0f, 260.0f, 360.0f)))
			{
				continue;
			}
			UNavigationPath* CandidatePath =
				Navigation->FindPathToLocationSynchronously(
					GetWorld(),
					PlayerPawn->GetActorLocation(),
					CandidateProjected.Location,
					PlayerPawn.Get());
			const bool bCandidateSeparated =
				!UsedContainerLocations.ContainsByPredicate(
					[&CandidateProjected](const FVector& Existing)
					{
						return FVector::DistSquared2D(
							Existing,
							CandidateProjected.Location)
							< FMath::Square(180.0f);
					});
			if (CandidatePath
				&& CandidatePath->IsValid()
				&& !CandidatePath->IsPartial()
				&& bCandidateSeparated
				&& FVector::DistSquared2D(
					PlayerPawn->GetActorLocation(),
					CandidateProjected.Location)
					>= FMath::Square(220.0f))
			{
				Projected = CandidateProjected;
				Path = CandidatePath;
				bSeparated = true;
				bResolved = true;
				ResolvedAttempt = Attempt;
				break;
			}
		}
		if (!bResolved)
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P8_FULL_MAP_DISTRIBUTION: bounded placement resolution failed slot=%s attempts=33."),
				*Slot.SlotId.ToString());
			DestroyRuntimeContainers(
				TEXT("P8ContainerPlacementRejected"));
			return false;
		}
		UE_LOG(
			Logdemo_map,
			Verbose,
			TEXT("P8_FULL_MAP_PLACEMENT_RESOLVED slot=%s attempt=%d."),
			*Slot.SlotId.ToString(),
			ResolvedAttempt);
		UsedContainerLocations.Add(Projected.Location);
		ResolvedContainers.Emplace(
			&Slot,
			FTransform(
				Anchor->GetActorRotation(),
				Projected.Location));
	}
	if (ResolvedContainers.Num()
		!= Fdemo_mapRewardFullMapDistribution::TotalContainerCount)
	{
		DestroyRuntimeContainers(
			TEXT("P8ContainerResolutionCountMismatch"));
		return false;
	}

	for (const auto& Resolved : ResolvedContainers)
	{
		const Fdemo_mapFullMapRewardSlot& Slot = *Resolved.Key;
		const Fdemo_mapRewardSourceProjection Projection =
			Fdemo_mapRewardFullMapDistribution::BuildProjection(
				Slot);
		FVector SeparatedLocation;
		if (!Items->ResolveSafeWorldLocation(
			GetWorld(),
			Resolved.Value.GetLocation(),
			nullptr,
			SeparatedLocation,
			Slot.SlotId,
			nullptr,
			66.0f))
		{
			DestroyRuntimeContainers(TEXT("P8ContainerWorldSeparationRejected"));
			return false;
		}
		FTransform SpawnTransform = Resolved.Value;
		SpawnTransform.SetLocation(SeparatedLocation);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ademo_mapLootChest* Chest =
			GetWorld()->SpawnActor<Ademo_mapLootChest>(
				Ademo_mapLootChest::StaticClass(),
				SpawnTransform,
				Params);
		if (!Chest)
		{
			DestroyRuntimeContainers(
				TEXT("P8ContainerSpawnFailed"));
			return false;
		}
		Chest->ConfigureRewardProjection(
			Slot.ContainerOrdinal,
			Projection);
		if (!Chest->InitializeChest(this, Items.Get(), Items->GetActiveRunId()))
		{
			Chest->Destroy();
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P8_FULL_MAP_DISTRIBUTION: slot=%s failed unified Runtime Container initialization."),
				*Slot.SlotId.ToString());
			DestroyRuntimeContainers(
				TEXT("P8ContainerInitializationFailed"));
			return false;
		}
		Chests.Add(Chest);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_CONTAINER_SPAWN slot=%s role=%s marker=%s projection=%s profile=%s base=%lld location=(%.1f,%.1f,%.1f)."),
			*Slot.SlotId.ToString(),
			*Slot.StableSourceRoleId.ToString(),
			*Slot.MarkerId.ToString(),
			*Slot.ProjectionId.ToString(),
			*Slot.BudgetProfileId.ToString(),
			Slot.BaseSourceValue,
			SpawnTransform.GetLocation().X,
			SpawnTransform.GetLocation().Y,
			SpawnTransform.GetLocation().Z);
	}
	if (Chests.Num()
		!= Fdemo_mapRewardFullMapDistribution::TotalContainerCount)
	{
		DestroyRuntimeContainers(
			TEXT("P8ContainerMaterializationCountMismatch"));
		return false;
	}
	for (Ademo_mapV3ProgressionMarker* Marker : WorldMarkers)
	{
		Ademo_mapWorldItem* Actor = nullptr;
		Fdemo_mapItemOperationResult Result = Items->CreateWorldItem(GetWorld(), Marker->GetDefinitionId(), Marker->GetQuantity(), Marker->GetActorLocation(), Actor, Marker->GetStableId());
		if (!Result.bSuccess || !Actor)
		{
			UE_LOG(Logdemo_map, Error, TEXT("V3_WORLD_INTERACTION: marker world item spawn failed id=%s code=%d diagnostic=%s."), *Marker->GetStableId().ToString(), static_cast<int32>(Result.Code), *Result.Diagnostic);
			return false;
		}
		InitialWorldItems.Add(Actor);
	}
	FString Error;
	if (!Items->ValidateInvariants(&Error))
	{
		UE_LOG(Logdemo_map, Error, TEXT("V3_WORLD_INTERACTION: initialization invariant failed: %s"), *Error);
		return false;
	}
	return true;
}

void Ademo_mapV3ProgressionManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	if (InputConsumptionTrace && InputConsumptionTrace->IsActive())
	{
		ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get());
		Recorddemo_mapInputConsumptionDerivedMovementUpdated(
			Character ? Character->GetCharacterMovement() : nullptr,
			PlayerPawn.Get(),
			GetDemoController(),
			DeltaSeconds);
		Recorddemo_mapInputConsumptionTraceStage(
			Edemo_mapInputConsumptionStage::PostCharacterMovement,
			PlayerPawn.Get(),
			GetDemoController());
	}
	TryFinalizeInputConsumptionTrace();
#endif
	QuickUseDeliveryAccumulator += DeltaSeconds;
	if (QuickUseDeliveryAccumulator >= 0.25f)
	{
		QuickUseDeliveryAccumulator = 0.0f;
		// A failed acknowledgement leaves the durable P15 receipt pending; this
		// bounded normal-gameplay retry performs no item mutation.
		DeliverPendingCodeBQuickUseReceipts();
	}
	if (!bInitialized || bInventoryOpen
		|| (Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode) && !bProfileWorldActive)) return;
	if (Ademo_mapSearchContainerActor* Container = ActiveSearchContainer.Get())
	{
		const APawn* Pawn = PlayerPawn.Get();
		const Udemo_mapPlayerHealthComponent* Health =
			Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
		const bool bAlive = Pawn && (!Health || !Health->IsDefeated());
		const bool bInRange = Pawn
			&& FVector::Dist(
				Pawn->GetActorLocation(),
				Container->GetInteractionLocation())
				<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
		if (!bAlive || !bInRange)
		{
			CloseSearchContainer(
				bAlive ? TEXT("OutOfRange") : TEXT("PlayerUnavailable"),
				true);
		}
	}
	if (bSearchContainerOpen)
	{
		SetFocusedActor(nullptr);
		return;
	}
	FocusAccumulator += DeltaSeconds;
	if (FocusAccumulator >= 0.10f) { FocusAccumulator = 0.0f; RefreshFocusNow(); }
}

void Ademo_mapV3ProgressionManager::RefreshFocusNow()
{
	APawn* Pawn = PlayerPawn.Get();
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Pawn || !Controller || bInventoryOpen || bSearchContainerOpen) { SetFocusedActor(nullptr); return; }
	AActor* PointerFocus = nullptr;
	// A real pointer target remains the preferred selection, so mouse users can
	// explicitly choose a nearby object.  When its ray lands on scenery or empty
	// ground, however, clearing focus made the advertised keyboard [G] action
	// unusable even beside a visible legal container.  Fall through to the same
	// in-range, visible proximity resolver used by keyboard gameplay; this never
	// bypasses CanInteract, range, or line-of-sight checks.
	if (ResolvePointerFocusedActor(PointerFocus) && PointerFocus != nullptr)
	{
		SetFocusedActor(PointerFocus);
		return;
	}
	AActor* Best = nullptr;
	double BestScore = -DBL_MAX;
	FString BestPath;
	FVector AimDirection = Controller->GetLastValidAimDirection().GetSafeNormal2D();
	if (AimDirection.IsNearlyZero()) AimDirection = Pawn->GetActorForwardVector().GetSafeNormal2D();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!Candidate || !Candidate->GetClass()->ImplementsInterface(Udemo_mapInteractable::StaticClass())) continue;
		Idemo_mapInteractable* Interactable = Cast<Idemo_mapInteractable>(Candidate);
		if (!Interactable || !Interactable->CanInteract(Controller)) continue;
		const FVector Location = Interactable->GetInteractionLocation();
		const float Distance = FVector::Dist(Pawn->GetActorLocation(), Location);
		if (Distance > Fdemo_mapWorldInteractionRules::InteractionRangeUU || !IsVisibleCandidate(Candidate, Location)) continue;
		const FVector ToTarget = (Location - Pawn->GetActorLocation()).GetSafeNormal2D();
		const double Score = Interactable->GetInteractionPriority() * 1000.0 - Distance + FVector::DotProduct(AimDirection, ToTarget) * 100.0;
		const FString Path = Candidate->GetPathName();
		if (Score > BestScore + 0.001 || (FMath::IsNearlyEqual(Score, BestScore, 0.001) && (Best == nullptr || Path < BestPath))) { Best = Candidate; BestScore = Score; BestPath = Path; }
	}
	SetFocusedActor(Best);
}

bool Ademo_mapV3ProgressionManager::ResolvePointerFocusedActor(
	AActor*& OutFocus) const
{
	OutFocus = nullptr;
	const APawn* Pawn = PlayerPawn.Get();
	const Ademo_mapPlayerController* Controller = GetDemoController();
	UWorld* World = GetWorld();
	if (!Pawn || !Controller || !World)
	{
		return false;
	}
	FVector RayStart;
	FVector RayDirection;
	if (!Controller->DeprojectMousePositionToWorld(RayStart, RayDirection)
		|| RayDirection.IsNearlyZero())
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(P4PrecisePointer), true, Pawn);
	Params.AddIgnoredActor(Pawn);
	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(
		Hits,
		RayStart,
		RayStart + RayDirection.GetSafeNormal() * 20000.0f,
		ECC_Visibility,
		Params);
	if (Hits.IsEmpty())
	{
		return true;
	}

	AActor* Best = nullptr;
	int32 BestClassRank = MAX_int32;
	float BestDistance = TNumericLimits<float>::Max();
	FString BestStableId;
	TSet<const AActor*> SeenActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Candidate = Hit.GetActor();
		if (!IsValid(Candidate))
		{
			// A real blocking world surface is an occluder.  Do not select an
			// interactable further down the same mouse ray.
			if (Hit.bBlockingHit)
			{
				break;
			}
			continue;
		}
		if (SeenActors.Contains(Candidate))
		{
			continue;
		}
		SeenActors.Add(Candidate);
		Idemo_mapInteractable* Interactable =
			Cast<Idemo_mapInteractable>(Candidate);
		if (!Interactable)
		{
			if (Hit.bBlockingHit)
			{
				break;
			}
			continue;
		}
		if (!Interactable->CanInteract(Controller)
			|| FVector::Dist(Pawn->GetActorLocation(),
				Interactable->GetInteractionLocation())
				> Fdemo_mapWorldInteractionRules::InteractionRangeUU)
		{
			continue;
		}

		const int32 ClassRank = Cast<Ademo_mapLootChest>(Candidate)
			? 0
			: Cast<Ademo_mapWorldItem>(Candidate) ? 1 : 2;
		const float Distance = Hit.Distance;
		const FString StableId = Candidate->GetPathName();
		if (ClassRank < BestClassRank
			|| (ClassRank == BestClassRank
				&& (Distance < BestDistance - KINDA_SMALL_NUMBER
					|| (FMath::IsNearlyEqual(Distance, BestDistance)
						&& (Best == nullptr || StableId < BestStableId)))))
		{
			Best = Candidate;
			BestClassRank = ClassRank;
			BestDistance = Distance;
			BestStableId = StableId;
		}
	}
	OutFocus = Best;
	return true;
}

bool Ademo_mapV3ProgressionManager::IsVisibleCandidate(AActor* Candidate, const FVector& InteractionLocation) const
{
	const APawn* Pawn = PlayerPawn.Get();
	if (!Pawn || !GetWorld()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(V3InteractionFocus), true, Pawn);
	Params.AddIgnoredActor(Pawn);
	FHitResult Hit;
	const FVector Start = Pawn->GetActorLocation() + FVector(0, 0, 55);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, InteractionLocation, ECC_Visibility, Params)) return true;
	return Hit.GetActor() == Candidate;
}

void Ademo_mapV3ProgressionManager::SetFocusedActor(AActor* NewFocus)
{
	if (FocusedActor.Get() == NewFocus) return;
	if (AActor* Old = FocusedActor.Get()) if (Idemo_mapInteractable* Interactable = Cast<Idemo_mapInteractable>(Old)) Interactable->FocusChanged(false);
	FocusedActor = NewFocus;
	if (NewFocus) if (Idemo_mapInteractable* Interactable = Cast<Idemo_mapInteractable>(NewFocus)) Interactable->FocusChanged(true);
}

FText Ademo_mapV3ProgressionManager::GetInteractionPrompt() const
{
	AActor* Focus = FocusedActor.Get();
	const Idemo_mapInteractable* Interactable = Focus ? Cast<Idemo_mapInteractable>(Focus) : nullptr;
	return Interactable
		? Interactable->GetInteractionPrompt(GetDemoController())
		: FText::FromString(FString::Printf(
			TEXT("%s 交互"),
			*Fdemo_mapInputBindingSettings::Get().GetKey(Fdemo_mapInputActionIds::Interact).GetDisplayName().ToString()));
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::RequestInteractFocused()
{
	if (IsInventoryOpen() || bSearchContainerOpen) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InteractionBlocked, TEXT("Gameplay interaction is blocked while a modal Runtime UI is open."));
	AActor* Focus = FocusedActor.Get();
	Idemo_mapInteractable* Interactable = Focus ? Cast<Idemo_mapInteractable>(Focus) : nullptr;
	if (!Interactable)
	{
		LastOperationResult = Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::NoInteractionFocus, TEXT("No valid interaction focus."));
		return LastOperationResult;
	}
	// The active focus has already passed the range, visibility and interactable
	// checks in RefreshFocusNow.  Requiring a second, pixel-precise mouse ray
	// here made the advertised [G] keyboard interaction fail while a chest was
	// visibly focused.  Pointer movement can still choose the focus, but once a
	// legal chest is focused its authoritative container transaction must remain
	// usable from the keyboard.
	LastOperationResult = Interactable->RequestInteract(GetDemoController());
	if (Ademo_mapSearchContainerActor* Container =
		Cast<Ademo_mapSearchContainerActor>(Focus))
	{
		if (LastOperationResult.bSuccess)
		{
			if (ActiveSearchContainer.IsValid()
				&& ActiveSearchContainer.Get() != Container)
			{
				CloseSearchContainer(TEXT("ContainerSwitched"), true);
			}
			ActiveSearchContainer = Container;
		}
	}
	if (!IsValid(Focus)) SetFocusedActor(nullptr); else RefreshFocusNow();
	return LastOperationResult;
}

void Ademo_mapV3ProgressionManager::ReleaseInteractFocused()
{
	if (Ademo_mapSearchContainerActor* Container = ActiveSearchContainer.Get())
	{
		if (Container->IsContainerOpening())
		{
			// Container opening is a committed, timed interaction.  A normal keyboard
			// press emits both Started and Completed in the same frame; cancelling here
			// made the visible [G] action impossible unless the key was physically held.
			// Keep the authoritative action alive and let its timer open the real search
			// page.  Explicit cancellation still goes through CloseSearchContainer.
		}
	}
}

void Ademo_mapV3ProgressionManager::OpenSearchContainer(
	Ademo_mapSearchContainerActor* Container)
{
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ContainerOpenBegin);
#endif
	if (!Container
		|| !Items.IsValid()
		|| !PlayerPawn.IsValid()
		|| Container->GetOwningRunId() != Items->GetActiveRunId()
		|| !Container->IsContainerOpened())
	{
		return;
	}
	if (ActiveSearchContainer.IsValid()
		&& ActiveSearchContainer.Get() != Container)
	{
		CloseSearchContainer(TEXT("ContainerSwitched"), true);
	}
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller)
	{
		return;
	}
	if (!SearchContainerWidget)
	{
		SearchContainerWidget =
			CreateWidget<Udemo_mapSearchContainerWidget>(
				Controller,
				Udemo_mapSearchContainerWidget::StaticClass());
		if (!SearchContainerWidget)
		{
			return;
		}
		SearchContainerWidget->InitializeForManager(this);
	}
	ActiveSearchContainer = Container;
	bSearchContainerOpen = true;
	SetFocusedActor(nullptr);
	RefreshSearchContainerWidget();
	if (!SearchContainerWidget->IsInViewport())
	{
		SearchContainerWidget->AddToViewport(550);
	}
	Controller->BeginSearchContainerInputLock(SearchContainerWidget);
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ContainerOpenCommitted);
#endif
}

void Ademo_mapV3ProgressionManager::CloseSearchContainer(
	const FString& Reason,
	bool bCancelAction)
{
#if !UE_BUILD_SHIPPING
	if (!bSearchContainerOpen)
	{
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::StaleCallbackAfterClose);
	}
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ContainerCloseBegin);
#endif
	Ademo_mapSearchContainerActor* Container = ActiveSearchContainer.Get();
	if (bCancelAction && Container && Container->IsContainerActionActive())
	{
		Container->CancelContainerAction(Reason);
	}
	if (SearchContainerWidget)
	{
		SearchContainerWidget->ClearTransientDragState();
#if !UE_BUILD_SHIPPING
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::WidgetRemovalRequested);
#endif
		SearchContainerWidget->RemoveFromParent();
#if !UE_BUILD_SHIPPING
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::WidgetNoLongerAuthoritative);
#endif
	}
	bSearchContainerOpen = false;
	ActiveSearchContainer.Reset();
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::SearchAuthorityCleared);
#endif
	const UWorld* World = GetWorld();
	if (Ademo_mapPlayerController* Controller =
		(!World || !World->bIsTearingDown)
			? GetDemoController()
			: nullptr)
	{
#if !UE_BUILD_SHIPPING
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::SearchReleaseRequested);
#endif
		Controller->RestoreGameplayControlFromSearchContainer();
	}
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ContainerCloseCommitted);
#endif
}

void Ademo_mapV3ProgressionManager::RefreshSearchContainerWidget()
{
	if (bSearchContainerOpen
		&& SearchContainerWidget
		&& ActiveSearchContainer.IsValid())
	{
		SearchContainerWidget->RefreshFromSnapshot(
			ActiveSearchContainer->GetContainerSnapshot());
	}
}

Fdemo_mapRuntimeContainerResult
Ademo_mapV3ProgressionManager::SubmitSearchContainerIntent(
	const Fdemo_mapRuntimeContainerIntent& Intent)
{
	Ademo_mapSearchContainerActor* Container = ActiveSearchContainer.Get();
	APawn* Pawn = PlayerPawn.Get();
	if (!bSearchContainerOpen || !Container || !Pawn || !Items.IsValid())
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::NotInitialized,
			TEXT("No active Runtime Container UI authority is available."));
	}
	const Udemo_mapPlayerHealthComponent* Health =
		Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
	const bool bAlive = !Health || !Health->IsDefeated();
	const bool bInRange =
		FVector::Dist(Pawn->GetActorLocation(), Container->GetInteractionLocation())
			<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
	Fdemo_mapRuntimeContainerResult Result =
		Container->SubmitContainerIntent(Intent, bAlive, bInRange);
	RefreshSearchContainerWidget();
#if !UE_BUILD_SHIPPING
	if (Result.bSuccess
		&& Intent.Action == Edemo_mapRuntimeContainerActionKind::Take)
	{
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::TakeCommitted);
	}
#endif
	return Result;
}

Fdemo_mapSearchContainerDropResult
Ademo_mapV3ProgressionManager::SubmitSearchContainerDrop(
	const Fdemo_mapSearchContainerDropIntent& Intent)
{
	Ademo_mapSearchContainerActor* Container = ActiveSearchContainer.Get();
	APawn* Pawn = PlayerPawn.Get();
	Fdemo_mapSearchContainerDropResult Result;
	if (!bSearchContainerOpen || !Container || !Pawn || !Items.IsValid())
	{
		Result.Operation = Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidContainer,
			TEXT("当前没有可用的搜索目标页面。"),
			Intent.ExpectedSourceItemInstanceId);
		Result.Diagnostic = Result.Operation.Diagnostic;
		return Result;
	}
	const Udemo_mapPlayerHealthComponent* Health =
		Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
	if ((Health && Health->IsDefeated())
		|| FVector::Dist(Pawn->GetActorLocation(),
			Container->GetInteractionLocation())
			> Fdemo_mapWorldInteractionRules::InteractionRangeUU)
	{
		Result.Operation = Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionOutOfRange,
			TEXT("搜索拖拽要求存活并保持交互距离。"),
			Intent.ExpectedSourceItemInstanceId);
		Result.Diagnostic = Result.Operation.Diagnostic;
		return Result;
	}
	Result = Container->SubmitPlayerDrop(Intent);
	LastOperationResult = Result.Operation;
	RefreshSearchContainerWidget();
	if (InventoryWidget)
	{
		InventoryWidget->RefreshFromAuthority();
	}
	return Result;
}

void Ademo_mapV3ProgressionManager::NotifySearchContainerEndPlay(
	Ademo_mapSearchContainerActor* Container)
{
	if (ActiveSearchContainer.Get() == Container)
	{
		CloseSearchContainer(TEXT("ContainerEndPlay"), true);
	}
	Chests.RemoveAll(
		[Container](const TWeakObjectPtr<Ademo_mapLootChest>& Chest)
		{
			return !Chest.IsValid() || Chest.Get() == Container;
		});
	Corpses.RemoveAll(
		[Container](const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Corpse)
		{
			return !Corpse.IsValid() || Corpse.Get() == Container;
		});
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::RequestDropInventory(FGuid InstanceId)
{
	Ademo_mapWorldItem* Actor = nullptr;
	LastOperationResult = Items.IsValid() ? Items->DropInventoryItem(InstanceId, PlayerPawn.Get(), Actor) : Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidWorldBinding, TEXT("Item subsystem is unavailable."), InstanceId);
	return LastOperationResult;
}

Fdemo_mapItemOperationResult
Ademo_mapV3ProgressionManager::RequestDropPlayerItem(
	const Fdemo_mapPlayerItemDropIntent& Intent,
	bool bConfirmSpatialBundle)
{
	if (!Items.IsValid())
	{
		LastOperationResult = Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvalidWorldBinding,
			TEXT("Item subsystem is unavailable."),
			Intent.ExpectedSourceItemInstanceId);
		return LastOperationResult;
	}
	if (Intent.ExpectedAuthorityRevision
		!= Items->GetAuthority().GetAuthorityRevision())
	{
		LastOperationResult = Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			TEXT("局内世界丢弃引用已过期；物品权威未改变。"),
			Intent.ExpectedSourceItemInstanceId);
		return LastOperationResult;
	}
	if (bConfirmSpatialBundle)
	{
		const FGuid EquippedSpatial = Items->GetAuthority().GetEquippedInstance(
			Fdemo_mapItemIds::BackpackSlot);
		if (Intent.SourceArea != Edemo_mapPlayerItemArea::Equipment
			|| Intent.SourceEquipmentSlotId != Fdemo_mapItemIds::BackpackSlot
			|| EquippedSpatial != Intent.ExpectedSourceItemInstanceId)
		{
			LastOperationResult = Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::UIInvalidSelection,
				TEXT("空间道具 Bundle 确认引用已过期。"),
				Intent.ExpectedSourceItemInstanceId);
			return LastOperationResult;
		}
		Fdemo_mapSpatialDiscardBundle Bundle;
		TArray<Ademo_mapWorldItem*> Actors;
		LastOperationResult = Items->DiscardSpatialItemBundle(
			PlayerPawn.Get(),
			Bundle,
			Actors);
	}
	else
	{
		Ademo_mapWorldItem* Actor = nullptr;
		LastOperationResult = Items->DropPlayerItemToWorld(
			Intent,
			PlayerPawn.Get(),
			Actor);
	}
	if (LastOperationResult.bSuccess)
	{
		if (InventoryWidget)
		{
			InventoryWidget->RefreshFromAuthority();
		}
		RefreshSearchContainerWidget();
	}
	return LastOperationResult;
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::HandleEnemyDeath(
	FName LootTableId,
	FGuid LootSourceId,
	const FVector& DeathLocation,
	const AActor* EnemyActor)
{
	const Fdemo_mapFixedLootTableDefinition* Table =
		Fdemo_mapItemDefinitions::FindFixedLootProfile(LootTableId);
	const Fdemo_mapRewardSourceProjection* Projection =
		Fdemo_mapRewardSourceProjectionRegistry::
			FindCorpseByFallbackTable(LootTableId);
	FName EncounterId = NAME_None;
	if (const Ademo_mapEnemyCharacter* Melee =
		Cast<Ademo_mapEnemyCharacter>(EnemyActor))
	{
		EncounterId = Melee->GetEncounterIdentity().EncounterId;
	}
	else if (const Ademo_mapRangedEnemyCharacter* Ranged =
		Cast<Ademo_mapRangedEnemyCharacter>(EnemyActor))
	{
		EncounterId = Ranged->GetEncounterIdentity().EncounterId;
	}
	else if (const Ademo_mapHeavyEnemyCharacter* Heavy =
		Cast<Ademo_mapHeavyEnemyCharacter>(EnemyActor))
	{
		EncounterId = Heavy->GetEncounterIdentity().EncounterId;
	}
	Fdemo_mapRewardSourceProjection RuntimeProjection;
	const Fdemo_mapFullMapRewardSlot* DistributionSlot =
		Fdemo_mapRewardFullMapDistribution::FindEnemyByEncounterId(
			EncounterId);
	if (DistributionSlot)
	{
		RuntimeProjection =
			Fdemo_mapRewardFullMapDistribution::BuildProjection(
				*DistributionSlot);
		Projection = RuntimeProjection.IsValid()
			? &RuntimeProjection
			: nullptr;
	}
	if (!Items.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| !LootSourceId.IsValid()
		|| CorpseLootSourceIds.Contains(LootSourceId)
		|| !Table
		|| Table->Kind != Edemo_mapRuntimeContainerKind::Corpse)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::LootSourceCompleted,
			TEXT("P7 fixed Corpse rejected an inactive, duplicate, or unbound LootSource."));
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapCorpseContainerActor* Corpse =
		GetWorld()->SpawnActor<Ademo_mapCorpseContainerActor>(
			Ademo_mapCorpseContainerActor::StaticClass(),
			DeathLocation + FVector(0.0f, 0.0f, 24.0f),
			FRotator::ZeroRotator,
			Params);
	bool bGenerated = false;
	Fdemo_mapRewardSourceProjectionResult Plan;
	bool bCorpseInitialized = false;
	Fdemo_mapPersistentGeneratedRewardSource ExistingSource;
	const bool bExistingGeneratedSource = Projection
		&& FindDurablyAcceptedRewardSource(
			Items->GetActiveRunId(), Projection->StableSourceRoleId, ExistingSource);
	if (Corpse && Projection && bExistingGeneratedSource)
	{
		bCorpseInitialized = Corpse->InitializeGeneratedCorpse(
			this,
			Items.Get(),
			Items->GetActiveRunId(),
			LootSourceId,
			*Projection,
			Plan);
		bGenerated = bCorpseInitialized;
	}
	else if (Corpse && Projection
		&& CanGenerateRewardSource(
			Items->GetActiveRunId(),
			Projection->StableSourceRoleId))
	{
		Plan = Fdemo_mapRewardSourceProjectionPlanner::Plan(
			*Projection,
			Items->GetActiveRunId(),
			GetRewardAffixPityState(Items->GetActiveRunId()));
		if (Plan.IsSuccess())
		{
			bCorpseInitialized = Corpse->InitializeGeneratedCorpse(
				this,
				Items.Get(),
				Items->GetActiveRunId(),
				LootSourceId,
				*Projection,
				Plan);
			bGenerated = bCorpseInitialized;
		}
	}
	if (Corpse && !bCorpseInitialized
		&& Projection
		&& Projection->bAllowFixedFallbackOnFailure)
	{
		UE_LOG(
			Logdemo_map,
			Warning,
			TEXT("P2_REWARD_SOURCE_PROJECTION: fallback projection=%s table=%s reason=%s."),
			*Projection->ProjectionId.ToString(),
			*LootTableId.ToString(),
			*Plan.Trace.Diagnostic);
		bCorpseInitialized = Corpse->InitializeFixedCorpse(
			this,
			Items.Get(),
			Items->GetActiveRunId(),
			LootSourceId,
			LootTableId);
	}
	if (Corpse && !Projection)
	{
		bCorpseInitialized = Corpse->InitializeFixedCorpse(
			this,
			Items.Get(),
			Items->GetActiveRunId(),
			LootSourceId,
			LootTableId);
	}
	if (!Corpse || !bCorpseInitialized)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P2_REWARD_SOURCE_PROJECTION_CORPSE_FAILED projection=%s role=%s table=%s source=%s generated=%d initialized=%d status=%d stacks=%d diagnostic=%s."),
			Projection
				? *Projection->ProjectionId.ToString()
				: TEXT("none"),
			Projection
				? *Projection->StableSourceRoleId.ToString()
				: TEXT("none"),
			*LootTableId.ToString(),
			*LootSourceId.ToString(
				EGuidFormats::DigitsWithHyphens),
			bGenerated ? 1 : 0,
			bCorpseInitialized ? 1 : 0,
			static_cast<int32>(Plan.Status),
			Plan.PlannedStacks.Num(),
			*Plan.Trace.Diagnostic);
		if (Corpse)
		{
			Corpse->Destroy();
		}
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::LootSpawnFailed,
			TEXT("P2 generated Corpse Runtime Container initialization or ledger commit failed."));
	}
	CorpseLootSourceIds.Add(LootSourceId);
	Corpses.Add(Corpse);
	LastOperationResult = Fdemo_mapItemOperationResult::Success();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("P2_REWARD_SOURCE_PROJECTION_CORPSE projection=%s role=%s table=%s source=%s enemy=%s generated=%d budget=%lld generated_value=%lld fallback=%d."),
		Projection ? *Projection->ProjectionId.ToString() : TEXT("none"),
		Projection ? *Projection->StableSourceRoleId.ToString() : TEXT("none"),
		*LootTableId.ToString(),
		*LootSourceId.ToString(EGuidFormats::DigitsWithHyphens),
		*GetNameSafe(EnemyActor),
		bGenerated ? 1 : 0,
		Plan.Trace.RandomizedBudget,
		Plan.Trace.GeneratedTotalValue,
		bGenerated ? 0 : 1);
	return LastOperationResult;
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::HandleM01EnemyDeath(
	FName RewardSourceRoleId,
	FName CorpseIdentity,
	FGuid LootSourceId,
	const FVector& DeathLocation,
	const AActor* EnemyActor)
{
	const Udemo_mapM01EnemyIdentityComponent* Identity = EnemyActor
		? EnemyActor->FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>()
		: nullptr;
	const Fdemo_mapM01EnemyDefinition* EnemyDefinition =
		Identity && Identity->IsConfigured() ? &Identity->GetDefinition() : nullptr;
	const Fdemo_mapM01RewardSlot* Slot = EnemyDefinition
		? Fdemo_mapM01RewardDistribution::FindEnemyByEncounterId(
			EnemyDefinition->EncounterId)
		: nullptr;
	const Fdemo_mapRewardSourceProjection Projection = Slot
		? Fdemo_mapM01RewardDistribution::BuildProjection(*Slot)
		: Fdemo_mapRewardSourceProjection();
	Fdemo_mapPersistentGeneratedRewardSource ExistingSource;
	const bool bExistingGeneratedSource = Slot
		&& FindDurablyAcceptedRewardSource(
			Items.IsValid() ? Items->GetActiveRunId() : FGuid(),
			Slot->StableSourceRoleId,
			ExistingSource);
	if (!Items.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| RewardSourceRoleId.IsNone()
		|| CorpseIdentity.IsNone()
		|| !LootSourceId.IsValid()
		|| CorpseLootSourceIds.Contains(LootSourceId)
		|| !Slot || !Projection.IsValid()
		|| !EnemyDefinition
		|| EnemyDefinition->RewardSourceRoleId != RewardSourceRoleId
		|| EnemyDefinition->CorpseIdentity != CorpseIdentity
		|| Slot->CorpseIdentity != CorpseIdentity
		|| (!bExistingGeneratedSource
			&& !CanGenerateRewardSource(
				Items->GetActiveRunId(), Slot->StableSourceRoleId)))
	{
		UE_LOG(Logdemo_map, Error,
			TEXT("M01_CORPSE_REJECTED role=%s corpse=%s source_valid=%d run_active=%d identity=%d slot=%d projection=%d duplicate=%d can_generate=%d."),
			*RewardSourceRoleId.ToString(), *CorpseIdentity.ToString(),
			LootSourceId.IsValid() ? 1 : 0,
			Items.IsValid() && Items->GetRunState() == Edemo_mapRunState::Active ? 1 : 0,
			EnemyDefinition ? 1 : 0, Slot ? 1 : 0,
			Projection.IsValid() ? 1 : 0,
			CorpseLootSourceIds.Contains(LootSourceId) ? 1 : 0,
			Items.IsValid() && Slot && CanGenerateRewardSource(
				Items->GetActiveRunId(), Slot->StableSourceRoleId) ? 1 : 0);
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::LootSourceCompleted,
			TEXT("M01 Corpse rejected an inactive, duplicate, or invalid stable source."));
	}
	// The enemy already committed its Code A death state before it entered this
	// adapter. P11 observes one static M01 spawn here; persistence refusal is
	// logged only and cannot alter the existing Code A corpse/reward flow.
	ObserveCodeBBodyContainerAfterCodeADeath(*EnemyDefinition);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapCorpseContainerActor* Corpse =
		GetWorld()->SpawnActor<Ademo_mapCorpseContainerActor>(
			Ademo_mapCorpseContainerActor::StaticClass(),
			DeathLocation + FVector(0.0f, 0.0f, 24.0f),
			FRotator::ZeroRotator,
			Params);
	Fdemo_mapRewardSourceProjectionResult Plan;
	if (!bExistingGeneratedSource)
	{
		Plan = Fdemo_mapRewardSourceProjectionPlanner::Plan(
			Projection,
			Items->GetActiveRunId(),
			GetRewardAffixPityState(Items->GetActiveRunId()));
	}
	const bool bCorpseInitialized = Corpse
		&& (bExistingGeneratedSource || Plan.IsSuccess())
		&& Corpse->InitializeM01GeneratedCorpse(
		this,
		Items.Get(),
		Items->GetActiveRunId(),
		LootSourceId,
		Projection,
		Plan,
		CorpseIdentity);
	// InitializeM01GeneratedCorpse prepares the durable source candidate before
	// it projects the container.  A second commit here would both violate that
	// ordering and reject the already-recorded role as a duplicate.
	const bool bSourceCommitted = bCorpseInitialized;
	if (!Corpse || (!bExistingGeneratedSource && !Plan.IsSuccess())
		|| !bCorpseInitialized
		|| !bSourceCommitted)
	{
		UE_LOG(Logdemo_map, Error,
		TEXT("M01_CORPSE_GENERATION_FAILED role=%s projection=%s plan_status=%d stacks=%d initialized=%d source_commit=%d diagnostic=%s."),
			*Slot->StableSourceRoleId.ToString(), *Projection.ProjectionId.ToString(),
			static_cast<int32>(Plan.Status), Plan.PlannedStacks.Num(),
		bCorpseInitialized ? 1 : 0, bSourceCommitted ? 1 : 0,
			*FString::Printf(TEXT("%s materialization=%s"),
				*Plan.Trace.Diagnostic,
				Corpse ? *Corpse->GetLastContainerDiagnostic() : TEXT("no_actor")));
		if (Corpse) Corpse->Destroy();
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::LootSpawnFailed,
			TEXT("M01 generated Corpse planning, materialization, or commit failed."));
	}
	CorpseLootSourceIds.Add(LootSourceId);
	if (EnemyDefinition->EncounterId == GCodeBBodyContainerEncounterId
		&& EnemyDefinition->SpawnMarkerId == GCodeBBodyContainerSpawnId)
	{
		// P12's actor receives only its static routing key.  The P11 record was
		// already independently materialized after Code A death and remains the
		// sole durable body truth.
		Corpse->EnableCodeBBodyContainerInteraction(GCodeBBodyContainerTargetIdentity);
	}
	Corpses.Add(Corpse);
	if (!SpawnM01SpiritStonePickup(*EnemyDefinition, DeathLocation, EnemyActor))
	{
		UE_LOG(
			Logdemo_map,
			Warning,
			TEXT("P5_SPIRIT_STONE_PENDING encounter=%s source=%s diagnostic=currency pickup was not materialized; no currency source was committed."),
			*EnemyDefinition->EncounterId.ToString(),
			*LootSourceId.ToString(EGuidFormats::DigitsWithHyphens));
	}
	LastOperationResult = Fdemo_mapItemOperationResult::Success();
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("M01_CORPSE_READY role=%s encounter=%s corpse=%s source=%s enemy=%s profile=%s base=%lld generated_value=%lld final_reward=1."),
		*Slot->StableSourceRoleId.ToString(),
		*Slot->EncounterId.ToString(),
		*CorpseIdentity.ToString(),
		*LootSourceId.ToString(EGuidFormats::DigitsWithHyphens),
		*GetNameSafe(EnemyActor),
		*Slot->BudgetProfileId.ToString(),
		Slot->BaseSourceValue,
		Plan.Trace.GeneratedTotalValue);
	return LastOperationResult;
}

bool Ademo_mapV3ProgressionManager::SpawnM01SpiritStonePickup(
	const Fdemo_mapM01EnemyDefinition& EnemyDefinition,
	const FVector& DeathLocation,
	const AActor* EnemyActor)
{
	if (!Items.IsValid() || !GetWorld() || Items->GetRunState() != Edemo_mapRunState::Active
		|| !EnemyDefinition.IsValid())
	{
		return false;
	}
	const FName SourceId(*FString::Printf(
		TEXT("P5.SpiritStone.%s"), *EnemyDefinition.EncounterId.ToString()));
	const FName PickupId(*FString::Printf(
		TEXT("P5.SpiritStone.Pickup.%s"), *EnemyDefinition.EncounterId.ToString()));
	if (SourceId.IsNone() || M01SpiritStoneSpawnSourceIds.Contains(SourceId))
	{
		return false;
	}
	int64 Value = 0;
	if (!Fdemo_mapTownProgressionRules::TryResolveEnemySpiritStoneValue(
		Items->GetActiveRunId(),
		EnemyDefinition.EncounterId,
		EnemyDefinition.RiskTierId,
		EnemyDefinition.IsElite(),
		EnemyDefinition.IsBoss(),
		Value))
	{
		return false;
	}
	FVector ResolvedLocation;
	if (!Items->ResolveSafeWorldLocation(
		GetWorld(),
		DeathLocation + FVector(68.0f, 0.0f, 42.0f),
		EnemyActor,
		ResolvedLocation,
		SourceId,
		nullptr,
		42.0f))
	{
		return false;
	}
	const FTransform SpawnTransform(FRotator::ZeroRotator, ResolvedLocation);
	Ademo_mapSpiritStonePickup* Pickup =
		GetWorld()->SpawnActorDeferred<Ademo_mapSpiritStonePickup>(
			Ademo_mapSpiritStonePickup::StaticClass(),
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup || !Pickup->InitializeRuntimePickup(
		PickupId,
		SourceId,
		Value,
		Items->GetActiveRunId()))
	{
		if (Pickup) Pickup->Destroy();
		return false;
	}
	Pickup = Cast<Ademo_mapSpiritStonePickup>(
		UGameplayStatics::FinishSpawningActor(Pickup, SpawnTransform));
	if (!Pickup)
	{
		return false;
	}
	M01SpiritStoneSpawnSourceIds.Add(SourceId);
	M01SpiritStonePickups.Add(Pickup);
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("P5_SPIRIT_STONE_READY encounter=%s source=%s tier=%s value=%lld location=(%.1f,%.1f,%.1f)."),
		*EnemyDefinition.EncounterId.ToString(),
		*SourceId.ToString(),
		*EnemyDefinition.RiskTierId.ToString(),
		static_cast<long long>(Value),
		ResolvedLocation.X,
		ResolvedLocation.Y,
		ResolvedLocation.Z);
	return true;
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::HandleEnemyDeath(Edemo_mapEnemyLootArchetype Archetype, FGuid LootSourceId, const FVector& DeathLocation, const AActor* EnemyActor)
{
	if (Archetype == Edemo_mapEnemyLootArchetype::Melee)
	{
		if (!Items.IsValid()
			|| Items->GetRunState() != Edemo_mapRunState::Active
			|| !LootSourceId.IsValid()
			|| CorpseLootSourceIds.Contains(LootSourceId))
		{
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::LootSourceCompleted,
				TEXT("P4 Corpse creation rejected an inactive or duplicate melee LootSource."));
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ademo_mapCorpseContainerActor* Corpse =
			GetWorld()->SpawnActor<Ademo_mapCorpseContainerActor>(
				Ademo_mapCorpseContainerActor::StaticClass(),
				DeathLocation + FVector(0.0f, 0.0f, 24.0f),
				FRotator::ZeroRotator,
				Params);
		if (!Corpse
			|| !Corpse->InitializeCorpse(
				this,
				Items.Get(),
				Items->GetActiveRunId(),
				LootSourceId))
		{
			if (Corpse)
			{
				Corpse->Destroy();
			}
			return Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::LootSpawnFailed,
				TEXT("P4 Corpse Runtime Container initialization failed."));
		}
		CorpseLootSourceIds.Add(LootSourceId);
		Corpses.Add(Corpse);
		LastOperationResult = Fdemo_mapItemOperationResult::Success();
		return LastOperationResult;
	}
	TArray<Ademo_mapWorldItem*> Spawned;
	LastOperationResult = Items.IsValid() ? Items->CreateEnemyLoot(GetWorld(), Archetype, LootSourceId, DeathLocation, EnemyActor, Spawned, true) : Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::RunNotActive, TEXT("V3 item subsystem is unavailable."));
	if (!LastOperationResult.bSuccess) UE_LOG(Logdemo_map, Warning, TEXT("0.3.4.0 ENEMY_LOOT_REJECT code=%d diagnostic=%s"), static_cast<int32>(LastOperationResult.Code), *LastOperationResult.Diagnostic);
	return LastOperationResult;
}

Fdemo_mapItemOperationResult Ademo_mapV3ProgressionManager::RequestSettlementAndReload(Edemo_mapRunEndReason Reason)
{
	if (!Items.IsValid()) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::RunNotActive, TEXT("V3 item subsystem is unavailable."));
	if (bSettlementPending) return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::SettlementAlreadyCompleted, TEXT("First legal terminal event already won."));
	if (bInventoryOpen) CloseInventory();
	CloseSearchContainer(TEXT("TerminalSettlement"), true);
	Fdemo_mapSettlementSummary Summary;
	LastOperationResult = Items->RequestSettlement(Reason, Summary);
	if (!LastOperationResult.bSuccess) return LastOperationResult;
	bSettlementPending = true;
	SetFocusedActor(nullptr);
	DestroyRuntimeContainers(TEXT("TerminalSettlement"));
	for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	if (ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get()))
	{
		if (Udemo_mapKnockbackComponent* Knockback =
			Character->FindComponentByClass<Udemo_mapKnockbackComponent>())
		{
			Knockback->Cancel(false);
		}
	}
	if (Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode) && ProfilePreparationFlow)
	{
		const Fdemo_mapProfileSessionSettlementResult PersistentResult = ProfilePreparationFlow->CommitRuntimeSettlement(Summary);
		UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP: settlement status=%d run=%s reason=%d settlement=%s generation=%d submits=%d retries=%d diagnostic=%s."),
			static_cast<int32>(PersistentResult.Status),
			*Summary.RunId.ToString(EGuidFormats::DigitsWithHyphens),
			static_cast<int32>(Summary.Reason),
			*PersistentResult.Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
			PersistentResult.Snapshot.SaveGeneration,
			ProfilePreparationFlow->GetSettlementSubmitCount(),
			ProfilePreparationFlow->GetSettlementRetryCount(),
			*PersistentResult.Diagnostic);
		if (PersistentResult.IsDurablySettled())
		{
			ECodeBRunInventoryTerminalState CodeBTerminalState =
				ECodeBRunInventoryTerminalState::Unknown;
			if (Summary.Reason == Edemo_mapRunEndReason::Extraction)
			{
				CodeBTerminalState = ECodeBRunInventoryTerminalState::Extracted;
			}
			else if (Summary.Reason == Edemo_mapRunEndReason::Death)
			{
				CodeBTerminalState = ECodeBRunInventoryTerminalState::Dead;
			}
			if (CodeBTerminalState != ECodeBRunInventoryTerminalState::Unknown)
			{
				// The Code A Runtime and Profile transaction have both returned a
				// durable success. This observer deliberately ignores Code B's
				// result so it cannot affect Code A's settlement or cleanup path.
				ObserveCodeBRunTerminalAfterCodeACommit(
					PersistentResult.Snapshot.ProfileId,
					Summary.RunId,
					CodeBTerminalState);
			}
			Summary.RiskBefore = PersistentResult.PersistentResult.RiskBefore;
			Summary.RiskTransferred = PersistentResult.PersistentResult.RiskTransferred;
			Summary.RiskLost = PersistentResult.PersistentResult.RiskLost;
			Summary.PersistentBefore = PersistentResult.PersistentResult.PersistentBefore;
			Summary.PersistentAfter = PersistentResult.PersistentResult.PersistentAfter;
			Summary.ClearedPreparationItemIds = PersistentResult.PersistentResult.ClearedPreparationItemIds;
			DeactivateProfileWorld();
			bSettlementPending = false;
			// Product flow has no blocking settlement report: durable settlement
			// returns directly to Sect Home, with the durable summary kept in log.
			ShowSectNavigation();
#if !UE_BUILD_SHIPPING
			if (bProfileFlowAutomation
				&& (ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Extract
					|| ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Death))
			{
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::FinishProfileFlowAutomation, 0.35f, false);
			}
#endif
		}
		else
		{
			DeactivateProfileWorld();
			ShowSectNavigation();
#if !UE_BUILD_SHIPPING
			if (bProfileFlowAutomation && PersistentResult.Status != Edemo_mapProfileSessionSettlementStatus::PendingRetry)
			{
				FailAutomation(FString::Printf(TEXT("PROFILE_PREPARATION_V3_FLOW: FAIL: persistent settlement rejected: %s"), *PersistentResult.Diagnostic));
			}
#endif
		}
		return LastOperationResult;
	}
	ShowSettlement(Summary);
	GetWorldTimerManager().SetTimer(SettlementReloadTimer, this, &Ademo_mapV3ProgressionManager::ReloadAfterSettlement, 1.85f, false);
	return LastOperationResult;
}

void Ademo_mapV3ProgressionManager::ShowSettlement(
	const Fdemo_mapSettlementSummary& Summary,
	FGuid SettlementId)
{
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller)
	{
		return;
	}
	if (SettlementId.IsValid()
		&& PresentedSettlementIds.Contains(SettlementId)
		&& ActiveSettlementPresentationId != SettlementId)
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("XFIX1_SETTLEMENT_PRESENTATION: suppressed retired settlement=%s."),
			*SettlementId.ToString(EGuidFormats::DigitsWithHyphens));
		return;
	}
	if (SettlementWidget
		&& SettlementId.IsValid()
		&& ActiveSettlementPresentationId == SettlementId)
	{
		return;
	}
	DismissSettlementPresentation(TEXT("ReplaceSettlementPresentation"));
	SettlementWidget = CreateWidget<Udemo_mapSettlementWidget>(
		Controller,
		Udemo_mapSettlementWidget::StaticClass());
	if (!SettlementWidget)
	{
		return;
	}
	SettlementWidget->InitializeForManager(this);
	SettlementWidget->SetSummary(Summary, SettlementId);
	ActiveSettlementPresentationId = SettlementId;
	if (SettlementId.IsValid())
	{
		PresentedSettlementIds.Add(SettlementId);
	}
	SettlementWidget->AddToViewport(700);
	Controller->BeginSettlementInputLock(SettlementWidget);
	LogInputRestoreDiagnostics(TEXT("C.SettlementVisible"));
}

void Ademo_mapV3ProgressionManager::DismissSettlementPresentation(
	const TCHAR* Reason)
{
	const bool bHadWidget = SettlementWidget != nullptr;
	if (SettlementWidget)
	{
		SettlementWidget->RemoveFromParent();
		SettlementWidget = nullptr;
	}
	ActiveSettlementPresentationId.Invalidate();
	if (Ademo_mapPlayerController* Controller = GetDemoController())
	{
		Controller->EndSettlementInputLock();
	}
	if (bHadWidget)
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("XFIX1_SETTLEMENT_PRESENTATION: retired reason=%s durable_history_preserved=1."),
			Reason ? Reason : TEXT("Unknown"));
	}
}

void Ademo_mapV3ProgressionManager::ReturnToSectAfterSettlement(
	const TCHAR* Reason)
{
	DismissSettlementPresentation(Reason);
	if (!bProfileWorldActive)
	{
		ShowSectNavigation();
	}
}

void Ademo_mapV3ProgressionManager::ReloadAfterSettlement()
{
	LogInputRestoreDiagnostics(TEXT("D.BeforeOpenLevel"));
	if (GetWorld()) UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true);
}

void Ademo_mapV3ProgressionManager::ToggleInventory()
{
	if (bCodeBActiveRunInventoryOpen)
	{
		CloseCodeBActiveRunInventory();
		return;
	}
	FString CodeBFeedback;
	if (OpenCodeBActiveRunInventory(CodeBFeedback))
	{
		return;
	}
	// A missing or mismatched P6 session is intentionally non-blocking: this
	// established Code A surface keeps its prior authority and behavior.
	if (bInventoryOpen) CloseInventory(); else OpenInventory();
}

void Ademo_mapV3ProgressionManager::OpenInventory()
{
	if (!bInitialized || bInventoryOpen) return;
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller) return;
	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<Udemo_mapInventoryWidget>(Controller, Udemo_mapInventoryWidget::StaticClass());
		if (!InventoryWidget) return;
		InventoryWidget->InitializeForManager(this);
	}
	bSavedShowMouseCursor = Controller->bShowMouseCursor;
	bSavedClickEvents = Controller->bEnableClickEvents;
	bSavedMouseOverEvents = Controller->bEnableMouseOverEvents;
	bSavedMoveIgnored = Controller->IsMoveInputIgnored();
	bSavedLookIgnored = Controller->IsLookInputIgnored();
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport()) { SavedCaptureMode = Viewport->GetMouseCaptureMode(); SavedLockMode = Viewport->GetMouseLockMode(); }
	InventoryWidget->RefreshFromAuthority();
	InventoryWidget->AddToViewport(100);
	bInventoryOpen = true;
	Controller->BeginInventoryInputLock(InventoryWidget);
	SetFocusedActor(nullptr);
	UE_LOG(Logdemo_map, Log, TEXT("P5_RUNTIME_INVENTORY: opened unique authority-backed widget through the unified input context."));
}

void Ademo_mapV3ProgressionManager::CloseInventory()
{
	if (bCodeBActiveRunInventoryOpen)
	{
		CloseCodeBActiveRunInventory();
		return;
	}
	if (!bInventoryOpen) return;
	if (InventoryWidget)
	{
		InventoryWidget->ClearTransientDragState();
		InventoryWidget->RemoveFromParent();
	}
	bInventoryOpen = false;
	if (Ademo_mapPlayerController* Controller = GetDemoController())
	{
		Controller->RestoreGameplayControlFromInventory();
	}
	UE_LOG(Logdemo_map, Log, TEXT("P5_RUNTIME_INVENTORY: closed; movement, combat, skills, interact and Hotbar restored immediately."));
}

Ademo_mapPlayerController* Ademo_mapV3ProgressionManager::GetDemoController() const { return Cast<Ademo_mapPlayerController>(PlayerPawn.IsValid() ? PlayerPawn->GetController() : nullptr); }

void Ademo_mapV3ProgressionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LogInputRestoreDiagnostics(TEXT("E.OldWorldEndPlay"));
#if !UE_BUILD_SHIPPING
	if (InputRestoreTrace)
	{
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::NormalClose);
		InputRestoreTrace->FlushToLog(TEXT("NormalClose"));
		Setdemo_mapInputRestoreTraceRuntimeActive(false);
	}
	if (InputConsumptionTrace)
	{
		if (InputConsumptionTrace->IsVerdictFrozen()
			&& InputConsumptionTrace->IsReadyForPostTerminalEnrichment())
		{
			InputConsumptionTrace->EnrichAfterVerdict();
			InputConsumptionTrace->FlushToLog(TEXT("NormalClose"));
		}
		DeactivateInputConsumptionTrace();
	}
#endif
	GetWorldTimerManager().ClearTimer(AutomationTimer);
	GetWorldTimerManager().ClearTimer(SettlementReloadTimer);
	if (APawn* Pawn = PlayerPawn.Get())
	{
		if (Udemo_mapPlayerHealthComponent* Health =
			Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>())
		{
			Health->OnPlayerDamaged.RemoveDynamic(
				this,
				&Ademo_mapV3ProgressionManager::HandlePlayerDamaged);
		}
	}
	DestroyRuntimeContainers(TEXT("ManagerEndPlay"));
	if (Items.IsValid() && Items->GetRunState() == Edemo_mapRunState::Active)
	{
		if (Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode))
		{
			UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP: EndPlay preserved the Prepared ActiveRun for next-session RecoveredAbandon."));
		}
		else
		{
			Fdemo_mapSettlementSummary AbandonSummary;
			const Fdemo_mapItemOperationResult Result = Items->RequestSettlement(Edemo_mapRunEndReason::Abandon, AbandonSummary);
			UE_LOG(Logdemo_map, Log, TEXT("0.3.4.0 UNEXPECTED_TRANSITION_ABANDON result=%d"), static_cast<int32>(Result.Code));
		}
	}
	if (bInventoryOpen) CloseInventory();
	if (ProfilePreparationWidget) ProfilePreparationWidget->RemoveFromParent();
	if (SettlementWidget) SettlementWidget->RemoveFromParent();
	SettlementWidget = nullptr;
	ActiveSettlementPresentationId.Invalidate();
	// Engine shutdown has already disabled the input surface. Restoring it during Quit
	// produces a false INPUT_RESTORE_FAIL after an otherwise successful automation run.
	if (EndPlayReason != EEndPlayReason::Quit
		&& !Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode))
	{
		if (Ademo_mapPlayerController* Controller = GetDemoController()) Controller->RestoreGameplayControlForNewRun();
	}
	SetFocusedActor(nullptr);
	if (Fdemo_mapProfileStartupModeSelector::UsesProfilePreparation(ProfileStartupMode))
	{
		DeactivateProfileWorld();
		if (ProfilePreparationFlow) ProfilePreparationFlow->Unbind();
		ProfilePreparationFlow.Reset();
	}
	else
	{
		if (Items.IsValid()) Items->TeardownWorld(GetWorld());
	}
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapV3ProgressionManager::HandlePlayerDamaged(int32 AppliedDamage)
{
	if (AppliedDamage > 0
		&& (bSearchContainerOpen
			|| (ActiveSearchContainer.IsValid()
				&& ActiveSearchContainer->IsContainerActionActive())))
	{
		CloseSearchContainer(TEXT("PositivePlayerDamage"), true);
	}
}

void Ademo_mapV3ProgressionManager::LogInputRestoreDiagnostics(const TCHAR* Phase) const
{
	if (!bInputRestoreDiagnostics)
	{
		return;
	}
	const Ademo_mapPlayerController* Controller = GetDemoController();
	UE_LOG(Logdemo_map, Log, TEXT("0.3.4.1 V3_INPUT_DIAGNOSTIC phase=%s settlement_pending=%d inventory_open=%d controller=%s"), Phase, bSettlementPending, bInventoryOpen, *GetNameSafe(Controller));
	if (Controller != nullptr)
	{
		Controller->LogInputRestoreDiagnostics(Phase);
	}
}

void Ademo_mapV3ProgressionManager::RunXFix1SettlementLifecycleAutomation()
{
#if !UE_BUILD_SHIPPING
	auto Fail = [this](const FString& Reason)
	{
		FailAutomation(TEXT("XFIX1_R1_RUN_LIFECYCLE: FAIL: ") + Reason);
	};
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !ProfilePreparationFlow
		|| !ProfilePreparationFlow->GetSession()
		|| !Items.IsValid())
	{
		Fail(TEXT("profile flow, controller, or item authority unavailable"));
		return;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot =
		ProfilePreparationFlow->GetSession()->GetSnapshot();
	if (bXFix1SettlementRestartAutomation)
	{
		const bool bPassed = Snapshot.LastSettlementId.IsValid()
			&& SettlementWidget == nullptr
			&& !Controller->IsSettlementInputLockedForAutomation()
			&& SectNavigationWidget
			&& SectNavigationWidget->IsInViewport();
		if (!bPassed)
		{
			Fail(FString::Printf(
				TEXT("restart state rejected history=%s widget=%d settlement_lock=%d sect_visible=%d"),
				*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
				SettlementWidget != nullptr,
				Controller->IsSettlementInputLockedForAutomation(),
				SectNavigationWidget && SectNavigationWidget->IsInViewport()));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("XFIX1_R1_RESTART: PASS history=%s modal_restored=0 settlement_lock=0 sect_home=1."),
			*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens));
		PassAutomation(TEXT("XFIX1_R1_RESTART_AUTOMATION: PASS."));
		return;
	}

	if (XFix1AutomationStep == 0)
	{
		const Fdemo_mapProfileSessionBeginResult Run1 = StartPreparedProfileRun();
		XFix1Run1Id = Run1.Snapshot.ActiveRunId;
		if (!Run1.IsRunActive() || !XFix1Run1Id.IsValid()
			|| !bProfileWorldActive)
		{
			Fail(TEXT("Run 1 could not activate a world"));
			return;
		}
		const Fdemo_mapItemOperationResult Settlement =
			RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction);
		XFix1PersistentSettlementId =
			ProfilePreparationFlow->GetSession()->GetSnapshot().LastSettlementId;
		if (!Settlement.bSuccess || !XFix1PersistentSettlementId.IsValid()
			|| bProfileWorldActive || SettlementWidget
			|| Controller->IsSettlementInputLockedForAutomation()
			|| !SectNavigationWidget || !SectNavigationWidget->IsInViewport())
		{
			Fail(TEXT("Run 1 extraction did not return directly to Sect Home"));
			return;
		}
		XFix1AutomationStep = 1;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunXFix1SettlementLifecycleAutomation,
			0.45f,
			false);
		return;
	}

	if (XFix1AutomationStep == 1)
	{
		const Fdemo_mapProfileSessionBeginResult Run2 = StartPreparedProfileRun();
		const Fdemo_mapProfileSessionSnapshot Active =
			ProfilePreparationFlow->GetSession()->GetSnapshot();
		if (!Run2.IsRunActive()
			|| !Active.ActiveRunId.IsValid()
			|| Active.ActiveRunId == XFix1Run1Id
			|| Active.LastSettlementId != XFix1PersistentSettlementId
			|| SettlementWidget
			|| Controller->IsSettlementInputLockedForAutomation()
			|| !bProfileWorldActive
			|| !Controller->IsGameplayInputAllowed()
			|| Controller->GetInputSurfaceState() != TEXT("Gameplay"))
		{
			Fail(TEXT("Run 2 did not activate cleanly after the direct Sect Home return"));
			return;
		}
		if (!RequestSettlementAndReload(Edemo_mapRunEndReason::Abandon).bSuccess)
		{
			Fail(TEXT("Run 2 cleanup settlement failed"));
			return;
		}
		const Fdemo_mapProfileSessionSnapshot Settled =
			ProfilePreparationFlow->GetSession()->GetSnapshot();
		if (!Settled.LastSettlementId.IsValid() || SettlementWidget
			|| Controller->IsSettlementInputLockedForAutomation()
			|| bProfileWorldActive
			|| !SectNavigationWidget || !SectNavigationWidget->IsInViewport())
		{
			Fail(TEXT("Run 2 abandon did not return directly to Sect Home"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("XFIX1_R1_RUN_LIFECYCLE: PASS run1=%s settlement1=%s run2=%s direct_sect_return=1 old_modal=0 gameplay_input=1 history_retained=1"),
			*XFix1Run1Id.ToString(EGuidFormats::DigitsWithHyphens),
			*XFix1PersistentSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
			*Run2.Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens));
		PassAutomation(TEXT("XFIX1_R1_RUN_LIFECYCLE_AUTOMATION: PASS."));
		return;
	}
	Fail(TEXT("invalid automation step"));
#endif
}

void Ademo_mapV3ProgressionManager::StartRequestedAutomation()
{
#if !UE_BUILD_SHIPPING
		if (!(bProfileFlowAutomation || bProfileTradeAutomation || bP4xStartRunAutomation || bP6ProductStartBridgeAutomation || bXFix1SettlementLifecycleAutomation || bXFix1SettlementRestartAutomation || bFullSystemLoopAutomation || bInputRestoreAutomation || bWorldAutomation || bInventoryUIAutomation || bVisibleAcceptance || bP5RuntimeVisibleAcceptance || bP6DualLootVisibleAcceptance || bP6DualLootManualFixture || bEnemyLootAutomation || bRunLifecycleAutomation || bPostSettlementInputRestoreAutomation || bSettlementUIAutomation || bFreshSessionAutomation || bCloseRangeProjectileAutomation || bLifecycleVisibleAcceptance || bRepairVisibleAcceptance || bV3FinalAutomation || bV3FinalVisibleAcceptance || bSearchContainerAutomation || bRewardGenerationAutomation || bRewardSourceProjectionAutomation || bRewardJackpotAutomation || bRewardRareExtremeAutomation || bRewardAffixPityAutomation || bRewardShopStockAutomation || bRewardBossSourceAutomation || bRewardFullMapDistributionAutomation || bEnemySkillFrameworkAutomation || bEnemyRouteLootAutomation)) return;
		if (bP6ProductStartBridgeAutomation)
		{
			P6ProductStartBridgeStep = 0;
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::RunP6ProductStartBridgeAutomation,
				0.8f,
				false);
			return;
		}
		if (bP4xStartRunAutomation)
		{
			P4xStartRunAutomationStep = 0;
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::RunP4xStartRunAutomation,
				0.8f,
				false);
			return;
		}
		if (bXFix1SettlementLifecycleAutomation
			|| bXFix1SettlementRestartAutomation)
		{
			XFix1AutomationStep = 0;
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::RunXFix1SettlementLifecycleAutomation,
				0.8f,
				false);
			return;
		}
		if (bRewardFullMapDistributionAutomation)
		{
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::
					RunRewardFullMapDistributionAutomation,
				0.8f,
				false);
			return;
		}
		if (bRewardBossSourceAutomation)
		{
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::
					RunRewardBossSourceAutomation,
				0.8f,
				false);
			return;
		}
		if (bRewardShopStockAutomation)
		{
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::
					RunRewardShopStockAutomation,
				0.8f,
				false);
			return;
		}
		if (bRewardAffixPityAutomation)
		{
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::
					RunRewardAffixPityAutomation,
				0.8f,
				false);
			return;
		}
		if (bRewardRareExtremeAutomation)
		{
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::
					RunRewardRareExtremeAutomation,
				0.8f,
				false);
			return;
		}
		if (bRewardJackpotAutomation)
		{
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
			{
				It->SetCombatSuppressed(true);
			}
			RewardJackpotTakenItemId.Invalidate();
			GetWorldTimerManager().SetTimer(
				AutomationTimer,
				this,
				&Ademo_mapV3ProgressionManager::RunRewardJackpotAutomation,
				0.8f,
				false);
			return;
		}
	if (bRewardSourceProjectionAutomation)
	{
		// Exercise the existing encounter actors and container UI as the P2 product path.
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		EnemyRouteLootAutomationStep = 0;
		EnemyRouteLootAutomationStartTime = GetWorld()->GetTimeSeconds();
		EnemyRouteLootWorldItemCountBefore = Items.IsValid()
			? Items->GetWorldActorCount()
			: INDEX_NONE;
		RewardSourceProjectionTakenIds.Reset();
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::
				RunEnemyRouteLootAutomation,
			0.8f,
			false);
		return;
	}
	if (bRewardGenerationAutomation)
	{
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		RewardGenerationAutomationStep = 0;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunRewardGenerationAutomation,
			0.8f,
			false);
		return;
	}
	if (bEnemyRouteLootAutomation)
	{
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
		for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
		EnemyRouteLootAutomationStep = 0;
		EnemyRouteLootAutomationStartTime = GetWorld()->GetTimeSeconds();
		EnemyRouteLootWorldItemCountBefore = Items.IsValid()
			? Items->GetWorldActorCount()
			: INDEX_NONE;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunEnemyRouteLootAutomation,
			0.8f,
			false);
		return;
	}
	if (bEnemySkillFrameworkAutomation)
	{
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		EnemySkillAutomationStep = 0;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunEnemySkillFrameworkAutomation,
			0.8f,
			false);
		return;
	}
	for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) It->SetCombatSuppressed(true);
	if (bInputRestoreAutomation) { InputRestoreAutomationStep = 0; InputRestoreContainerWaitRetries = 0; ScheduleInputRestoreAutomation(0.35f); }
	else if (bFullSystemLoopAutomation) { FullSystemLoopAutomationStep = 0; ScheduleFullSystemAutomation(0.8f); }
	else if (bSearchContainerAutomation) { SearchContainerAutomationStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunSearchContainerAutomation, 0.8f, false); }
	else if (bProfileTradeAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunProfileTradeAutomation, 0.8f, false);
	else if (bProfileFlowAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunProfileFlowAutomation, 0.8f, false);
	else if (bP6DualLootManualFixture) { VisibleStep = 3; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunP6DualLootVisibleAcceptanceStep, 1.2f, false); }
	else if (bP6DualLootVisibleAcceptance) { VisibleStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunP6DualLootVisibleAcceptanceStep, 1.2f, false); }
	else if (bP5RuntimeVisibleAcceptance) { VisibleStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunP5RuntimeVisibleAcceptanceStep, 1.2f, false); }
	else if (bV3FinalVisibleAcceptance) { VisibleStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunV3FinalVisibleAcceptance, 1.2f, false); }
	else if (bV3FinalAutomation) { FinalAutomationStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunV3FinalAutomation, 0.8f, false); }
	else if (bRepairVisibleAcceptance) { VisibleStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 1.2f, false); }
	else if (bCloseRangeProjectileAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunCloseRangeProjectileAutomation, 1.0f, false);
	else if (bEnemyLootAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunEnemyLootAutomation, 1.0f, false);
	else if (bRunLifecycleAutomation || bPostSettlementInputRestoreAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunLifecycleAutomation, 0.8f, false);
	else if (bSettlementUIAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunSettlementUIAutomation, 0.8f, false);
	else if (bFreshSessionAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunFreshSessionAutomation, 0.8f, false);
	else if (bLifecycleVisibleAcceptance) { VisibleStep = 0; GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep, 1.0f, false); }
	else if (bWorldAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunWorldInteractionAutomation, 1.0f, false);
	else if (bInventoryUIAutomation) GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunInventoryUIAutomation, 1.0f, false);
	else { VisibleStep = 0; ScheduleVisibleStep(1.5f); }
#endif
}

#if !UE_BUILD_SHIPPING
bool Fdemo_mapInputRestoreTerminalStateEmitter::TryBuildLine(
	const FString& Phase,
	const FString& Boundary,
	bool bPassed,
	const Ademo_mapPlayerController* Controller,
	const APawn* Pawn,
	const UWorld* World,
	float DistanceUU,
	double LatencySeconds,
	FString& OutLine)
{
	if (bEmitted)
	{
		return false;
	}
	bEmitted = true;

	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr;
	const float Dilation =
		World && World->GetWorldSettings()
			? World->GetWorldSettings()->GetEffectiveTimeDilation()
			: -1.0f;
	const float FiniteDilation = FMath::IsFinite(Dilation) ? Dilation : -1.0f;
	const float FiniteDistance = FMath::IsFinite(DistanceUU) ? DistanceUU : -1.0f;
	const double FiniteLatency = FMath::IsFinite(LatencySeconds) ? LatencySeconds : -1.0;

	OutLine = FString::Printf(
		TEXT("INPUT_RESTORE_TERMINAL_STATE phase=%s boundary=%s caller=L.FirstW verdict=%s move_ignored=%d look_ignored=%d gameplay_allowed=%d context=%s input_mode=%s possessed=%d movement_mode=%d paused=%d dilation=%.3f distance_uu=%.3f latency_seconds=%.6f"),
		*Phase,
		*Boundary,
		bPassed ? TEXT("PASS") : TEXT("FAIL"),
		Controller && Controller->IsMoveInputIgnored() ? 1 : 0,
		Controller && Controller->IsLookInputIgnored() ? 1 : 0,
		Controller && Controller->IsGameplayInputAllowed() ? 1 : 0,
		Controller ? *Controller->GetInputSurfaceState() : TEXT("Unavailable"),
		Controller ? *Controller->GetInputModeState() : TEXT("Unavailable"),
		Controller && Controller->GetPawn() == Pawn ? 1 : 0,
		Movement ? static_cast<int32>(Movement->MovementMode) : INDEX_NONE,
		World && UGameplayStatics::IsGamePaused(World) ? 1 : 0,
		FiniteDilation,
		FiniteDistance,
		FiniteLatency);
	return true;
}

void Ademo_mapV3ProgressionManager::ScheduleInputRestoreAutomation(float Delay)
{
	GetWorldTimerManager().SetTimer(
		AutomationTimer,
		this,
		&Ademo_mapV3ProgressionManager::RunInputRestoreAutomation,
		Delay,
		false);
}

void Ademo_mapV3ProgressionManager::FinishInputRestoreAutomation(
	bool bPassed,
	const FString& Reason)
{
	if (bInputRestoreVerdictPending)
	{
		return;
	}
	if (Ademo_mapPlayerController* Controller = GetDemoController())
	{
		const FKey MoveKey =
			Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::MoveForward);
		Controller->DispatchAutomationKeyReleased(MoveKey);
	}
	RecordInputRestoreTraceEvent(
		bPassed
			? Edemo_mapInputRestoreTraceEvent::ProbePass
			: Edemo_mapInputRestoreTraceEvent::ProbeFail,
		-1.0f,
		-1.0,
		Edemo_mapInputRestoreTraceCaller::Probe);

	if (InputConsumptionTrace && InputConsumptionTrace->IsActive())
	{
		InputConsumptionTrace->FreezeVerdict(
			bPassed,
			PlayerPawn.Get(),
			GetDemoController(),
			InputRestoreLastProbeDistance,
			InputRestoreLastProbeLatency);
		bInputRestoreVerdictPending = true;
		bInputRestoreVerdictPassed = bPassed;
		InputRestoreVerdictReason = Reason;
		return;
	}

	CompleteInputRestoreAutomationVerdict(bPassed, Reason);
}

void Ademo_mapV3ProgressionManager::CompleteInputRestoreAutomationVerdict(
	bool bPassed,
	const FString& Reason)
{
	if (InputRestoreTrace)
	{
		InputRestoreTrace->FlushToLog(
			bPassed ? TEXT("ProbePass") : TEXT("ProbeFail"));
	}
	if (bPassed)
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("INPUT_RESTORE_PROBE: PASS phase=%s boundary=%s reason=%s."),
			*InputRestorePhase,
			*InputRestoreBoundary,
			*Reason);
	}
	else
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("INPUT_RESTORE_PROBE: FAIL phase=%s boundary=%s reason=%s."),
			*InputRestorePhase,
			*InputRestoreBoundary,
			*Reason);
	}
	FString TerminalStateLine;
	if (InputRestoreTerminalStateEmitter.TryBuildLine(
		InputRestorePhase,
		InputRestoreBoundary,
		bPassed,
		GetDemoController(),
		PlayerPawn.Get(),
		GetWorld(),
		InputRestoreLastProbeDistance,
		InputRestoreLastProbeLatency,
		TerminalStateLine))
	{
		UE_LOG(Logdemo_map, Log, TEXT("%s"), *TerminalStateLine);
	}
	if (bPassed)
	{
		const TCHAR* Marker =
			InputRestorePhase.StartsWith(TEXT("RunStart"))
				? TEXT("INPUT_RESTORE_RUN_START: PASS.")
				: InputRestorePhase.Equals(TEXT("ChestTakeClose"), ESearchCase::IgnoreCase)
					? TEXT("INPUT_RESTORE_CHEST_CLOSE: PASS.")
					: InputRestorePhase.Equals(TEXT("CorpseTakeClose"), ESearchCase::IgnoreCase)
						? TEXT("INPUT_RESTORE_CORPSE_CLOSE: PASS.")
						: TEXT("INPUT_RESTORE_RELOAD: PASS.");
		PassAutomation(Marker);
		return;
	}
	FPlatformMisc::RequestExitWithStatus(false, 1);
}

void Ademo_mapV3ProgressionManager::TryFinalizeInputConsumptionTrace()
{
	if (!bInputRestoreVerdictPending
		|| !InputConsumptionTrace
		|| !InputConsumptionTrace->IsReadyForPostTerminalEnrichment())
	{
		return;
	}

	InputConsumptionTrace->EnrichAfterVerdict();
	InputConsumptionTrace->FlushToLog(
		bInputRestoreVerdictPassed ? TEXT("ProbePass") : TEXT("ProbeFail"));
	DeactivateInputConsumptionTrace();

	const bool bPassed = bInputRestoreVerdictPassed;
	const FString Reason = InputRestoreVerdictReason;
	bInputRestoreVerdictPending = false;
	InputRestoreVerdictReason.Reset();
	CompleteInputRestoreAutomationVerdict(bPassed, Reason);
}

bool Ademo_mapV3ProgressionManager::BeginInputRestoreMovementProbe(
	const TCHAR* Boundary)
{
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller
		|| !PlayerPawn.IsValid()
		|| !Items.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| !Items->GetActiveRunId().IsValid()
		|| (ProfilePreparationWidget
			&& ProfilePreparationWidget->IsInViewport())
		|| bSearchContainerOpen
		|| Controller->IsProfilePreparationInputLocked()
		|| Controller->IsSearchContainerInputLocked())
	{
		FinishInputRestoreAutomation(
			false,
			TEXT("playable boundary state was not coherent"));
		return false;
	}
	InputRestoreBoundary = Boundary;
	InputRestoreBoundarySeconds = GetWorld()->GetTimeSeconds();
	InputRestoreMovementStart = PlayerPawn->GetActorLocation();
	bInputRestoreTraceSawVelocity = false;
	bInputRestoreTraceSawDisplacement = false;
	bInputRestoreTraceSawTenUU = false;
	bInputRestoreVerdictPending = false;
	InputRestoreLastProbeDistance = 0.0f;
	InputRestoreLastProbeLatency = 0.0;
	InputRestoreVerdictReason.Reset();
	InputRestoreTerminalStateEmitter.Reset();
	SetInputRestoreTraceBoundary(Boundary);
	ActivateInputConsumptionTrace();
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ExistingProbeBoundaryEmitted,
		0.0f,
		0.0,
		Edemo_mapInputRestoreTraceCaller::Probe);
	Recorddemo_mapInputConsumptionTraceStage(
		Edemo_mapInputConsumptionStage::ExistingProbeBoundaryEmitted,
		PlayerPawn.Get(),
		Controller,
		0.0f,
		0.0);
	Controller->LogInputRestoreDiagnostics(Boundary);
	const FKey MoveKey =
		Fdemo_mapInputBindingSettings::Get().GetKey(
			Fdemo_mapInputActionIds::MoveForward);
	if (!MoveKey.IsValid())
	{
		FinishInputRestoreAutomation(false, TEXT("MoveForward registry key is invalid"));
		return false;
	}
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::MoveKeyDispatch,
		0.0f,
		0.0,
		Edemo_mapInputRestoreTraceCaller::Probe);
	Recorddemo_mapInputConsumptionTraceStage(
		Edemo_mapInputConsumptionStage::MoveKeyDispatch,
		PlayerPawn.Get(),
		Controller,
		0.0f,
		0.0);
	if (!Controller->DispatchAutomationKeyPressed(MoveKey))
	{
		FinishInputRestoreAutomation(
			false,
			TEXT("real PlayerInput MoveForward press was rejected"));
		return false;
	}
	GetWorldTimerManager().SetTimer(
		AutomationTimer,
		this,
		&Ademo_mapV3ProgressionManager::SampleInputRestoreMovementProbe,
		0.05f,
		false);
	return true;
}

void Ademo_mapV3ProgressionManager::SampleInputRestoreMovementProbe()
{
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !PlayerPawn.IsValid() || !GetWorld())
	{
		FinishInputRestoreAutomation(false, TEXT("probe lost controller or pawn"));
		return;
	}
	const double Elapsed =
		GetWorld()->GetTimeSeconds() - InputRestoreBoundarySeconds;
	const FVector Delta =
		PlayerPawn->GetActorLocation() - InputRestoreMovementStart;
	const float PlanarDistance = FVector(Delta.X, Delta.Y, 0.0f).Size();
	InputRestoreLastProbeDistance = PlanarDistance;
	InputRestoreLastProbeLatency = Elapsed;
	const ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get());
	const UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr;
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ProbeSample,
		PlanarDistance,
		Elapsed,
		Edemo_mapInputRestoreTraceCaller::Probe);
	Recorddemo_mapInputConsumptionTraceStage(
		Edemo_mapInputConsumptionStage::ProbeSample,
		PlayerPawn.Get(),
		Controller,
		PlanarDistance,
		Elapsed);
	if (!bInputRestoreTraceSawVelocity
		&& PlayerPawn->GetVelocity().Size2D() > KINDA_SMALL_NUMBER)
	{
		bInputRestoreTraceSawVelocity = true;
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::VelocityBecameNonZero,
			PlanarDistance,
			Elapsed,
			Edemo_mapInputRestoreTraceCaller::Probe);
	}
	if (!bInputRestoreTraceSawDisplacement
		&& PlanarDistance > KINDA_SMALL_NUMBER)
	{
		bInputRestoreTraceSawDisplacement = true;
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::FirstPlanarDisplacement,
			PlanarDistance,
			Elapsed,
			Edemo_mapInputRestoreTraceCaller::Probe);
	}
	if (!bInputRestoreTraceSawTenUU && PlanarDistance >= 10.0f)
	{
		bInputRestoreTraceSawTenUU = true;
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::DisplacementExceeded10UU,
			PlanarDistance,
			Elapsed,
			Edemo_mapInputRestoreTraceCaller::Probe);
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_RESTORE_PROBE sample phase=%s boundary=%s latency=%.6f distance=%.3f move_ignored=%d look_ignored=%d gameplay_allowed=%d input_mode=%s movement_mode=%d pawn=%s."),
		*InputRestorePhase,
		*InputRestoreBoundary,
		Elapsed,
		PlanarDistance,
		Controller->IsMoveInputIgnored(),
		Controller->IsLookInputIgnored(),
		Controller->IsGameplayInputAllowed(),
		*Controller->GetInputSurfaceState(),
		Movement ? static_cast<int32>(Movement->MovementMode) : INDEX_NONE,
		*GetNameSafe(PlayerPawn.Get()));
	if (PlanarDistance >= 10.0f)
	{
		FinishInputRestoreAutomation(
			Elapsed <= 0.50,
			FString::Printf(
				TEXT("real PlayerInput moved %.3f UU in %.6f seconds"),
				PlanarDistance,
				Elapsed));
		return;
	}
	if (Elapsed > 0.50)
	{
		FinishInputRestoreAutomation(
			false,
			FString::Printf(
				TEXT("latency %.6f exceeded 0.50 seconds at %.3f UU"),
				Elapsed,
				PlanarDistance));
		return;
	}
	GetWorldTimerManager().SetTimer(
		AutomationTimer,
		this,
		&Ademo_mapV3ProgressionManager::SampleInputRestoreMovementProbe,
		0.05f,
		false);
}

bool Ademo_mapV3ProgressionManager::PrepareInputRestoreContainer(bool bCorpse)
{
	Ademo_mapSearchContainerActor* Container = nullptr;
	if (bCorpse)
	{
		for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Candidate : Corpses)
		{
			if (Candidate.IsValid())
			{
				Container = Candidate.Get();
				break;
			}
		}
	}
	else
	{
		for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate : Chests)
		{
			if (Candidate.IsValid())
			{
				Container = Candidate.Get();
				break;
			}
		}
	}
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Container
		|| !Controller
		|| !MovePawnNear(Container, 100.0f))
	{
		return false;
	}
	InputRestoreContainer = Container;
	Controller->SetAutomationAimDirection(
		Container->GetActorLocation() - PlayerPawn->GetActorLocation());
	SetFocusedActor(Container);
	const FKey InteractKey =
		Fdemo_mapInputBindingSettings::Get().GetKey(
			Fdemo_mapInputActionIds::Interact);
	return Controller->DispatchAutomationKeyPressed(InteractKey)
		&& Container->IsContainerOpening();
}

bool Ademo_mapV3ProgressionManager::TakeAndCloseInputRestoreContainer(
	bool bCorpse)
{
	Ademo_mapSearchContainerActor* Container = InputRestoreContainer.Get();
	if (!Container || !SearchContainerWidget || !bSearchContainerOpen)
	{
		return false;
	}
	const Fdemo_mapRuntimeContainerSnapshot Snapshot =
		Container->GetContainerSnapshot();
	const Fdemo_mapRuntimeContainerEntrySnapshot* Selected = nullptr;
	Edemo_mapRuntimeContainerSection SelectedSection =
		bCorpse
			? Edemo_mapRuntimeContainerSection::Backpack
			: Edemo_mapRuntimeContainerSection::Chest;
	for (const Fdemo_mapRuntimeContainerSectionSnapshot& Section :
		Snapshot.Sections)
	{
		for (const Fdemo_mapRuntimeContainerEntrySnapshot& Entry :
			Section.OrderedOccupiedEntries)
		{
			if ((bCorpse
					&& Entry.State
						== Edemo_mapRuntimeContainerEntryState::Hidden)
				|| (!bCorpse
					&& Section.Section
						== Edemo_mapRuntimeContainerSection::Chest))
			{
				Selected = &Entry;
				SelectedSection = Section.Section;
				break;
			}
		}
		if (Selected)
		{
			break;
		}
	}
	if (!Selected)
	{
		return false;
	}
	InputRestoreTakenItemId =
		Selected->State == Edemo_mapRuntimeContainerEntryState::Identified
			? Selected->ItemInstanceId
			: FGuid();
	InputRestoreEntrySection = static_cast<int32>(SelectedSection);
	InputRestoreEntrySlot = Selected->SlotIndex;
	if (Selected->State == Edemo_mapRuntimeContainerEntryState::Hidden)
	{
		return SearchContainerWidget->AutomationClickEntry(
				SelectedSection,
				Selected->SlotIndex)
			&& Container->IsContainerSearching();
	}
	return SearchContainerWidget->AutomationClickEntry(
			SelectedSection,
			Selected->SlotIndex)
		&& SearchContainerWidget->GetLastResult().bSuccess
		&& SearchContainerWidget->GetLastResult().ItemInstanceId
			== InputRestoreTakenItemId
		&& SearchContainerWidget->AutomationClickClose();
}

void Ademo_mapV3ProgressionManager::RunInputRestoreAutomation()
{
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || !ProfilePreparationFlow || !ProfilePreparationWidget)
	{
		FinishInputRestoreAutomation(
			false,
			TEXT("Preparation/controller fixture is unavailable"));
		return;
	}
	const bool bChest =
		InputRestorePhase.Equals(TEXT("ChestTakeClose"), ESearchCase::IgnoreCase);
	const bool bCorpse =
		InputRestorePhase.Equals(TEXT("CorpseTakeClose"), ESearchCase::IgnoreCase);
	if (InputRestoreAutomationStep == 0)
	{
		if (!ProfilePreparationWidget->AutomationClickStartRun()
			|| !bProfileWorldActive)
		{
			FinishInputRestoreAutomation(
				false,
				TEXT("real Preparation Start Run button did not activate the product Run"));
			return;
		}
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::RunActivated);
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
		{
			It->SetCombatSuppressed(true);
		}
		if (!bChest && !bCorpse)
		{
			BeginInputRestoreMovementProbe(TEXT("RunStartPlayable"));
			return;
		}
		InputRestoreAutomationStep = 1;
		ScheduleInputRestoreAutomation(0.10f);
		return;
	}

	if (bChest)
	{
		if (InputRestoreAutomationStep == 1)
		{
			if (!PrepareInputRestoreContainer(false))
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("real held Interact did not begin Chest open"));
				return;
			}
			InputRestoreAutomationStep = 2;
			ScheduleInputRestoreAutomation(1.15f);
			return;
		}
		if (InputRestoreAutomationStep == 2)
		{
			const FKey InteractKey =
				Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::Interact);
			if (!Controller->DispatchAutomationKeyReleased(InteractKey)
				|| !InputRestoreContainer.IsValid()
				|| !InputRestoreContainer->IsContainerOpened()
				|| !TakeAndCloseInputRestoreContainer(false))
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("Chest Open/Search product path failed"));
				return;
			}
			if (!bSearchContainerOpen)
			{
				BeginInputRestoreMovementProbe(TEXT("ChestTakeCloseCommitted"));
				return;
			}
			InputRestoreAutomationStep = 3;
			ScheduleInputRestoreAutomation(1.00f);
			return;
		}
		if (InputRestoreAutomationStep == 3)
		{
			const Edemo_mapRuntimeContainerSection Section =
				static_cast<Edemo_mapRuntimeContainerSection>(
					InputRestoreEntrySection);
			const Fdemo_mapRuntimeContainerSnapshot BeforeTake =
				InputRestoreContainer.IsValid()
					? InputRestoreContainer->GetContainerSnapshot()
					: Fdemo_mapRuntimeContainerSnapshot();
			const Fdemo_mapRuntimeContainerEntrySnapshot* PendingEntry = nullptr;
			for (const Fdemo_mapRuntimeContainerSectionSnapshot& CandidateSection :
				BeforeTake.Sections)
			{
				if (CandidateSection.Section != Section)
				{
					continue;
				}
				PendingEntry =
					CandidateSection.OrderedOccupiedEntries.FindByPredicate(
						[this](const Fdemo_mapRuntimeContainerEntrySnapshot& Entry)
						{
							return Entry.SlotIndex == InputRestoreEntrySlot;
						});
				break;
			}
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("INPUT_RESTORE_PROBE Chest pre-take open=%d widget=%d container=%d state=%d revision=%d section=%d slot=%d entry_state=%d progress=%.3f retries=%d."),
				bSearchContainerOpen,
				SearchContainerWidget != nullptr,
				InputRestoreContainer.IsValid(),
				InputRestoreContainer.IsValid()
					? static_cast<int32>(
						InputRestoreContainer->GetContainerSnapshot().State)
					: INDEX_NONE,
				BeforeTake.Revision,
				InputRestoreEntrySection,
				InputRestoreEntrySlot,
				PendingEntry
					? static_cast<int32>(PendingEntry->State)
					: INDEX_NONE,
				PendingEntry ? PendingEntry->Progress01 : -1.0f,
				InputRestoreContainerWaitRetries);
			if (PendingEntry
				&& PendingEntry->State
					== Edemo_mapRuntimeContainerEntryState::Searching
				&& InputRestoreContainerWaitRetries < 8)
			{
				++InputRestoreContainerWaitRetries;
				ScheduleInputRestoreAutomation(0.25f);
				return;
			}
			if (PendingEntry
				&& PendingEntry->State
					== Edemo_mapRuntimeContainerEntryState::Identified)
			{
				InputRestoreTakenItemId = PendingEntry->ItemInstanceId;
			}
			const bool bClickedTake =
				SearchContainerWidget
				&& SearchContainerWidget->AutomationClickEntry(
					Section,
					InputRestoreEntrySlot);
			const Fdemo_mapRuntimeContainerResult TakeResult =
				SearchContainerWidget
					? SearchContainerWidget->GetLastResult()
					: Fdemo_mapRuntimeContainerResult::Failure(
						Edemo_mapRuntimeContainerResultCode::NotInitialized,
						TEXT("Search widget is unavailable."));
			const bool bClickedClose =
				bClickedTake
				&& TakeResult.bSuccess
				&& TakeResult.ItemInstanceId == InputRestoreTakenItemId
				&& SearchContainerWidget->AutomationClickClose();
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("INPUT_RESTORE_PROBE Chest take-result clicked=%d success=%d code=%d expected_item=%s actual_item=%s close_clicked=%d open_after=%d diagnostic=%s."),
				bClickedTake,
				TakeResult.bSuccess,
				static_cast<int32>(TakeResult.Code),
				*InputRestoreTakenItemId.ToString(EGuidFormats::DigitsWithHyphens),
				*TakeResult.ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
				bClickedClose,
				bSearchContainerOpen,
				*TakeResult.Diagnostic);
			if (!bClickedTake
				|| !TakeResult.bSuccess
				|| TakeResult.ItemInstanceId != InputRestoreTakenItemId
				|| !bClickedClose)
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("Chest Take/Close Widget Intent failed"));
				return;
			}
			BeginInputRestoreMovementProbe(TEXT("ChestTakeCloseCommitted"));
			return;
		}
	}

	if (bCorpse)
	{
		if (InputRestoreAutomationStep >= 1
			&& InputRestoreAutomationStep <= 3)
		{
			Ademo_mapEnemyCharacter* Target = nullptr;
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->GetCurrentHealth() > 0)
				{
					Target = *It;
					break;
				}
			}
			if (!Target)
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("Corpse phase could not find a live product enemy"));
				return;
			}
			Target->SetCombatSuppressed(true);
			const FVector Forward =
				PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
			Target->SetActorLocation(
				PlayerPawn->GetActorLocation() + Forward * 120.0f,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			const FKey AttackKey =
				Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::PrimaryAttack);
			if (!Controller->DispatchAutomationKey(AttackKey))
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("real PrimaryAttack PlayerInput dispatch failed"));
				return;
			}
			++InputRestoreAttackCount;
			++InputRestoreAutomationStep;
			ScheduleInputRestoreAutomation(0.55f);
			return;
		}
		if (InputRestoreAutomationStep == 4)
		{
			if (Corpses.IsEmpty() || !PrepareInputRestoreContainer(true))
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("real attacks did not create/open a Corpse"));
				return;
			}
			InputRestoreAutomationStep = 5;
			ScheduleInputRestoreAutomation(1.15f);
			return;
		}
		if (InputRestoreAutomationStep == 5)
		{
			const FKey InteractKey =
				Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::Interact);
			if (!Controller->DispatchAutomationKeyReleased(InteractKey)
				|| !InputRestoreContainer.IsValid()
				|| !InputRestoreContainer->IsContainerOpened()
				|| !TakeAndCloseInputRestoreContainer(true))
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("Corpse Open/Search product path failed"));
				return;
			}
			InputRestoreAutomationStep = 6;
			ScheduleInputRestoreAutomation(1.65f);
			return;
		}
		if (InputRestoreAutomationStep == 6)
		{
			const Edemo_mapRuntimeContainerSection Section =
				static_cast<Edemo_mapRuntimeContainerSection>(
					InputRestoreEntrySection);
			const Fdemo_mapRuntimeContainerSnapshot BeforeTake =
				InputRestoreContainer.IsValid()
					? InputRestoreContainer->GetContainerSnapshot()
					: Fdemo_mapRuntimeContainerSnapshot();
			const Fdemo_mapRuntimeContainerEntrySnapshot* PendingEntry = nullptr;
			for (const Fdemo_mapRuntimeContainerSectionSnapshot& CandidateSection :
				BeforeTake.Sections)
			{
				if (CandidateSection.Section != Section)
				{
					continue;
				}
				PendingEntry =
					CandidateSection.OrderedOccupiedEntries.FindByPredicate(
						[this](const Fdemo_mapRuntimeContainerEntrySnapshot& Entry)
						{
							return Entry.SlotIndex == InputRestoreEntrySlot;
						});
				break;
			}
			if (PendingEntry
				&& PendingEntry->State
					== Edemo_mapRuntimeContainerEntryState::Searching
				&& InputRestoreContainerWaitRetries < 8)
			{
				++InputRestoreContainerWaitRetries;
				ScheduleInputRestoreAutomation(0.25f);
				return;
			}
			if (PendingEntry
				&& PendingEntry->State
					== Edemo_mapRuntimeContainerEntryState::Identified)
			{
				InputRestoreTakenItemId = PendingEntry->ItemInstanceId;
			}
			const bool bClickedTake =
				SearchContainerWidget
				&& SearchContainerWidget->AutomationClickEntry(
					Section,
					InputRestoreEntrySlot);
			const Fdemo_mapRuntimeContainerResult TakeResult =
				SearchContainerWidget
					? SearchContainerWidget->GetLastResult()
					: Fdemo_mapRuntimeContainerResult::Failure(
						Edemo_mapRuntimeContainerResultCode::NotInitialized,
						TEXT("Search widget is unavailable."));
			const bool bClickedClose =
				bClickedTake
				&& TakeResult.bSuccess
				&& TakeResult.ItemInstanceId == InputRestoreTakenItemId
				&& SearchContainerWidget->AutomationClickClose();
			if (!bClickedTake
				|| !TakeResult.bSuccess
				|| TakeResult.ItemInstanceId != InputRestoreTakenItemId
				|| !bClickedClose)
			{
				FinishInputRestoreAutomation(
					false,
					TEXT("Corpse Take/Close Widget Intent failed"));
				return;
			}
			BeginInputRestoreMovementProbe(TEXT("CorpseTakeCloseCommitted"));
			return;
		}
	}
	FinishInputRestoreAutomation(false, TEXT("unexpected automation state"));
}

void Ademo_mapV3ProgressionManager::FinishSearchContainerAutomation(
	bool bPassed,
	const FString& Reason)
{
	if (SearchContainerAutomationStep < 0)
	{
		return;
	}
	SearchContainerAutomationStep = -1;
	GetWorldTimerManager().ClearTimer(AutomationTimer);
	if (!bPassed)
	{
		DestroyRuntimeContainers(TEXT("SearchContainerSmokeFailure"));
		if (Items.IsValid() && Items->GetRunState() == Edemo_mapRunState::Active)
		{
			Fdemo_mapSettlementSummary Summary;
			if (Items->RequestSettlement(
				Edemo_mapRunEndReason::Abandon,
				Summary).bSuccess
				&& SearchContainerProfileSession.IsValid())
			{
				SearchContainerProfileSession->CommitRuntimeSettlement(Summary);
			}
		}
		if (Items.IsValid())
		{
			Items->TeardownWorld(GetWorld());
		}
		FString CleanReason = Reason;
		CleanReason.ReplaceInline(TEXT("\r"), TEXT(" "));
		CleanReason.ReplaceInline(TEXT("\n"), TEXT(" "));
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("SEARCH_CONTAINER_SMOKE: FAIL — %s"),
			*CleanReason);
		FPlatformMisc::RequestExitWithStatus(false, 1);
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("SEARCH_CONTAINER_SMOKE: PASS"));
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

void Ademo_mapV3ProgressionManager::FinishRewardGenerationAutomation(
	bool bPassed,
	const FString& Reason)
{
	if (RewardGenerationAutomationStep < 0)
	{
		return;
	}
	RewardGenerationAutomationStep = -1;
	GetWorldTimerManager().ClearTimer(AutomationTimer);
	if (!bPassed)
	{
		DestroyRuntimeContainers(
			TEXT("RewardGenerationProductSmokeFailure"));
		if (Items.IsValid()
			&& Items->GetRunState() == Edemo_mapRunState::Active)
		{
			Fdemo_mapSettlementSummary Summary;
			if (Items->RequestSettlement(
				Edemo_mapRunEndReason::Abandon,
				Summary).bSuccess
				&& RewardGenerationProfileSession.IsValid())
			{
				RewardGenerationProfileSession
					->CommitRuntimeSettlement(Summary);
			}
		}
		if (Items.IsValid())
		{
			Items->TeardownWorld(GetWorld());
		}
		FString CleanReason = Reason;
		CleanReason.ReplaceInline(TEXT("\r"), TEXT(" "));
		CleanReason.ReplaceInline(TEXT("\n"), TEXT(" "));
		if (bRewardSourceProjectionAutomation)
		{
			UE_LOG(Logdemo_map, Error, TEXT("P2_REWARD_SOURCE_PROJECTION_PRODUCT_SMOKE: FAIL: %s"), *CleanReason);
		}
		else
		{
			UE_LOG(Logdemo_map, Error, TEXT("P1_REWARD_GENERATION_PRODUCT_SMOKE: FAIL: %s"), *CleanReason);
		}
		FPlatformMisc::RequestExitWithStatus(false, 1);
		return;
	}
	if (bRewardSourceProjectionAutomation)
	{
		UE_LOG(Logdemo_map, Log, TEXT("P2_REWARD_SOURCE_PROJECTION_PRODUCT_SMOKE: PASS."));
	}
	else
	{
		UE_LOG(Logdemo_map, Log, TEXT("P1_REWARD_GENERATION_PRODUCT_SMOKE: PASS."));
	}
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

void Ademo_mapV3ProgressionManager::FinishEnemySkillFrameworkAutomation(
	bool bPassed,
	const FString& Reason)
{
	if (EnemySkillAutomationStep < 0)
	{
		return;
	}
	EnemySkillAutomationStep = -1;
	GetWorldTimerManager().ClearTimer(AutomationTimer);
	for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
	{
		It->SetCombatSuppressed(true);
		It->ResetEnemySkillForNewRun();
	}
	for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It)
	{
		It->SetCombatSuppressed(true);
		It->ResetEnemySkillForNewRun();
	}
	for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It)
	{
		It->SetCombatSuppressed(true);
	}
	if (ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get()))
	{
		if (Udemo_mapKnockbackComponent* Knockback =
			Character->FindComponentByClass<Udemo_mapKnockbackComponent>())
		{
			Knockback->Cancel(true);
		}
	}
	if (Items.IsValid() && Items->GetRunState() == Edemo_mapRunState::Active)
	{
		Fdemo_mapSettlementSummary Summary;
		if (Items->RequestSettlement(
			Edemo_mapRunEndReason::Abandon,
			Summary).bSuccess
			&& EnemySkillProfileSession.IsValid())
		{
			EnemySkillProfileSession->CommitRuntimeSettlement(Summary);
		}
	}
	if (Items.IsValid())
	{
		Items->TeardownWorld(GetWorld());
	}
	if (!bPassed)
	{
		FString CleanReason = Reason;
		CleanReason.ReplaceInline(TEXT("\r"), TEXT(" "));
		CleanReason.ReplaceInline(TEXT("\n"), TEXT(" "));
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("ENEMY_SKILL_SMOKE: FAIL — %s"),
			*CleanReason);
		FPlatformMisc::RequestExitWithStatus(false, 1);
		return;
	}
	if (EnemySkillAutomationPhase.Equals(
		TEXT("Melee"),
		ESearchCase::IgnoreCase))
	{
		UE_LOG(Logdemo_map, Log, TEXT("ENEMY_SKILL_MELEE_SMOKE: PASS."));
	}
	else
	{
		UE_LOG(Logdemo_map, Log, TEXT("ENEMY_SKILL_RANGED_SMOKE: PASS."));
	}
	FPlatformMisc::RequestExitWithStatus(false, 0);
}

void Ademo_mapV3ProgressionManager::RunEnemySkillFrameworkAutomation()
{
	auto Fail = [this](const FString& Reason)
	{
		FinishEnemySkillFrameworkAutomation(false, Reason);
	};
	auto Schedule = [this](float Delay)
	{
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunEnemySkillFrameworkAutomation,
			Delay,
			false);
	};
	Ademo_mapGameMode* Mode =
		GetWorld()
			? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
			: nullptr;
	ACharacter* Player = Cast<ACharacter>(PlayerPawn.Get());
	Udemo_mapPlayerHealthComponent* Health =
		Player
			? Player->FindComponentByClass<Udemo_mapPlayerHealthComponent>()
			: nullptr;
	if (!Mode
		|| !Player
		|| !Health
		|| !EnemySkillProfileSession.IsValid()
		|| !Items.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| !Items->GetActiveRunId().IsValid())
	{
		Fail(TEXT("normal prepared ActiveRun, default player, or Profile Session is unavailable"));
		return;
	}
	const bool bMelee = EnemySkillAutomationPhase.Equals(
		TEXT("Melee"),
		ESearchCase::IgnoreCase);
	Ademo_mapEnemyCharacter* Melee = Mode->GetMeleeEnemy();
	Ademo_mapRangedEnemyCharacter* Ranged = Mode->GetRangedEnemy();
	Ademo_mapHeavyEnemyCharacter* Heavy = Mode->GetHeavyEnemy();
	if (!Melee || !Ranged || !Heavy)
	{
		Fail(TEXT("default V3 product enemy actors are unavailable"));
		return;
	}
	if (EnemySkillAutomationStep == 0)
	{
		Melee->SetCombatSuppressed(true);
		Ranged->SetCombatSuppressed(true);
		Heavy->SetCombatSuppressed(true);
		Melee->ResetEnemySkillForNewRun();
		Ranged->ResetEnemySkillForNewRun();
		if (UCharacterMovementComponent* Movement =
			Player->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		const Fdemo_mapEnemySkillDefinition& Definition =
			bMelee
				? Fdemo_mapEnemySkillPrototypeConfig::Get().MeleeDash
				: Fdemo_mapEnemySkillPrototypeConfig::Get().RangedBackstepShot;
		const float SetupDistance =
			bMelee
				? (Definition.TriggerMinDistance
					+ Definition.TriggerMaxDistance) * 0.5f
				: Definition.TriggerMaxDistance
					- Definition.MinimumResolvedDistance;
		const FVector PlayerLocation = Player->GetActorLocation();
		const FVector Candidates[] =
		{
			FVector::ForwardVector,
			FVector::RightVector,
			-FVector::ForwardVector,
			-FVector::RightVector
		};
		ACharacter* SkillActor = bMelee
			? static_cast<ACharacter*>(Melee)
			: static_cast<ACharacter*>(Ranged);
		bool bPlaced = false;
		for (const FVector& Axis : Candidates)
		{
			FVector Candidate = PlayerLocation + Axis * SetupDistance;
			Candidate.Z = PlayerLocation.Z;
			SkillActor->SetActorLocation(
				Candidate,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			const FVector SkillDirection = bMelee ? -Axis : Axis;
			const Fdemo_mapCombatDisplacementResult Preflight =
				Fdemo_mapCombatDisplacement::PreflightWorldStatic(
					SkillActor,
					SkillDirection,
					Definition.DisplacementDistance,
					Fdemo_mapEnemySkillPrototypeConfig::Get().WorldStaticSkin);
			if (Preflight.ResolvedDistance + KINDA_SMALL_NUMBER
				>= Definition.MinimumResolvedDistance)
			{
				bPlaced = true;
				break;
			}
		}
		if (!bPlaced)
		{
			Fail(TEXT("no legal product-world setup position passed WorldStatic preflight"));
			return;
		}
		if (UCharacterMovementComponent* Movement =
			SkillActor->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		EnemySkillHealthBefore = Health->GetCurrentHealth();
		EnemySkillProjectileCountBefore =
			Ranged->GetTotalProjectilesFired();
		EnemySkillPlayerStart = Player->GetActorLocation();
		EnemySkillActorStart = SkillActor->GetActorLocation();
		EnemySkillAutomationStartTime = GetWorld()->GetTimeSeconds();
		if (bMelee)
		{
			Melee->SetCombatSuppressed(false);
		}
		else
		{
			Ranged->SetCombatSuppressed(false);
		}
		EnemySkillAutomationStep = 1;
		Schedule(0.05f);
		return;
	}

	if (GetWorld()->GetTimeSeconds() - EnemySkillAutomationStartTime > 8.0)
	{
		Fail(TEXT("normal AI did not complete the requested P6 product skill before timeout"));
		return;
	}
	if (bMelee)
	{
		Udemo_mapEnemySkillRuntimeComponent* Runtime =
			Melee->GetEnemySkillRuntime();
		Udemo_mapKnockbackComponent* Knockback =
			Player->FindComponentByClass<Udemo_mapKnockbackComponent>();
		if (!Runtime
			|| Runtime->GetActivationCount() > 1
			|| Runtime->GetLegalHitCount() > 1
			|| Health->GetCurrentHealth() > EnemySkillHealthBefore)
		{
			Fail(TEXT("melee runtime violated activation, first-hit, or health monotonicity"));
			return;
		}
		if (Runtime->GetLegalHitCount() == 1
			&& Health->GetCurrentHealth() < EnemySkillHealthBefore
			&& Knockback
			&& Knockback->GetAcceptedCount() == 1
			&& !Runtime->IsActive()
			&& !Knockback->IsActive())
		{
			const float InitialDistance = FVector::Dist2D(
				EnemySkillActorStart,
				EnemySkillPlayerStart);
			const float ActorTravel = FVector::Dist2D(
				EnemySkillActorStart,
				Melee->GetActorLocation());
			if (ActorTravel <= KINDA_SMALL_NUMBER
				|| FVector::Dist2D(
					Melee->GetActorLocation(),
					EnemySkillPlayerStart) >= InitialDistance
				|| Knockback->GetResolvedDistance() <= KINDA_SMALL_NUMBER
				|| Knockback->GetResolvedDistance()
					> Fdemo_mapEnemySkillPrototypeConfig::Get()
						.KnockbackDistance + KINDA_SMALL_NUMBER)
			{
				Fail(TEXT("melee dash or planar knockback direction/distance was invalid"));
				return;
			}
			FinishEnemySkillFrameworkAutomation(true, FString());
			return;
		}
	}
	else
	{
		Udemo_mapEnemySkillRuntimeComponent* Runtime =
			Ranged->GetEnemySkillRuntime();
		const int32 FiredDelta =
			Ranged->GetTotalProjectilesFired()
			- EnemySkillProjectileCountBefore;
		if (!Runtime
			|| Runtime->GetActivationCount() > 1
			|| FiredDelta > 1
			|| Health->GetCurrentHealth() > EnemySkillHealthBefore)
		{
			Fail(TEXT("ranged runtime violated activation, exactly-once fire, or health monotonicity"));
			return;
		}
		if (Runtime->GetResolveCount() == 1
			&& FiredDelta == 1
			&& Health->GetCurrentHealth() < EnemySkillHealthBefore
			&& !Runtime->IsActive()
			&& Ranged->GetActiveProjectileCount() == 0)
		{
			const float InitialDistance = FVector::Dist2D(
				EnemySkillActorStart,
				EnemySkillPlayerStart);
			const float FinalDistance = FVector::Dist2D(
				Ranged->GetActorLocation(),
				EnemySkillPlayerStart);
			if (FinalDistance <= InitialDistance)
			{
				Fail(TEXT("ranged backstep did not move away from the player"));
				return;
			}
			FinishEnemySkillFrameworkAutomation(true, FString());
			return;
		}
	}
	Schedule(0.05f);
}

void Ademo_mapV3ProgressionManager::RunEnemyRouteLootAutomation()
{
	auto Cleanup = [this]()
	{
		CloseSearchContainer(TEXT("EnemyRouteLootSmokeCleanup"), true);
		DestroyRuntimeContainers(TEXT("EnemyRouteLootSmokeCleanup"));
		if (Items.IsValid()
			&& Items->GetRunState() == Edemo_mapRunState::Active)
		{
			Fdemo_mapSettlementSummary Summary;
			if (Items->RequestSettlement(
					Edemo_mapRunEndReason::Abandon,
					Summary).bSuccess
				&& EnemyRouteLootProfileSession.IsValid())
			{
				EnemyRouteLootProfileSession->CommitRuntimeSettlement(Summary);
			}
		}
		if (Items.IsValid())
		{
			Items->TeardownWorld(GetWorld());
		}
	};
	auto Fail = [this, &Cleanup](const FString& Reason)
	{
		if (EnemyRouteLootAutomationStep < 0)
		{
			return;
		}
		EnemyRouteLootAutomationStep = -1;
		GetWorldTimerManager().ClearTimer(AutomationTimer);
		Cleanup();
		FString Clean = Reason;
		Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
		Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
		if (bRewardSourceProjectionAutomation)
		{
			UE_LOG(Logdemo_map, Error, TEXT("P2_REWARD_SOURCE_PROJECTION_PRODUCT_SMOKE: FAIL: %s"), *Clean);
		}
		else
		{
			UE_LOG(Logdemo_map, Error, TEXT("ENEMY_ROUTE_LOOT_SMOKE: FAIL — %s"), *Clean);
		}
		FPlatformMisc::RequestExitWithStatus(false, 1);
	};
	auto Schedule = [this](float Delay)
	{
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunEnemyRouteLootAutomation,
			Delay,
			false);
	};
	auto FindMelee = [this](FName EncounterId) -> Ademo_mapEnemyCharacter*
	{
		for (const TWeakObjectPtr<AActor>& Actor : EnemyActors)
		{
			Ademo_mapEnemyCharacter* Enemy =
				Cast<Ademo_mapEnemyCharacter>(Actor.Get());
			if (Enemy
				&& Enemy->GetEncounterIdentity().EncounterId
					== EncounterId)
			{
				return Enemy;
			}
		}
		return nullptr;
	};
	auto FindRanged = [this](FName EncounterId) -> Ademo_mapRangedEnemyCharacter*
	{
		for (const TWeakObjectPtr<AActor>& Actor : EnemyActors)
		{
			Ademo_mapRangedEnemyCharacter* Enemy =
				Cast<Ademo_mapRangedEnemyCharacter>(Actor.Get());
			if (Enemy
				&& Enemy->GetEncounterIdentity().EncounterId
					== EncounterId)
			{
				return Enemy;
			}
		}
		return nullptr;
	};
	auto FindCorpse = [this](FName LootTableId) -> Ademo_mapCorpseContainerActor*
	{
		for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Corpse :
			Corpses)
		{
			if (Corpse.IsValid()
				&& Corpse->GetLootTableId() == LootTableId)
			{
				return Corpse.Get();
			}
		}
		return nullptr;
	};
	auto FindEntry = [](
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex)
		-> const Fdemo_mapRuntimeContainerEntrySnapshot*
	{
		const Fdemo_mapRuntimeContainerSectionSnapshot* SectionSnapshot =
			Snapshot.Sections.FindByPredicate(
				[Section](
					const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
				{
					return Candidate.Section == Section;
				});
		return SectionSnapshot
			? SectionSnapshot->OrderedOccupiedEntries.FindByPredicate(
				[SlotIndex](
					const Fdemo_mapRuntimeContainerEntrySnapshot& Entry)
				{
					return Entry.SlotIndex == SlotIndex;
				})
			: nullptr;
	};
	auto OpenThroughProductInput = [this](
		Ademo_mapSearchContainerActor* Container) -> bool
	{
		if (!Container
			|| !PlayerPawn.IsValid()
			|| !GetDemoController())
		{
			return false;
		}
		MovePawnNear(Container);
		GetDemoController()->SetAutomationAimDirection(
			Container->GetActorLocation()
			- PlayerPawn->GetActorLocation());
		SetFocusedActor(Container);
		if (!GetDemoController()->DispatchAutomationKeyPressed(EKeys::G)
			|| !Container->IsContainerOpening()
			|| !Container->CompleteActionForAutomation().bSuccess
			|| !GetDemoController()->DispatchAutomationKeyReleased(EKeys::G)
			|| !Container->IsContainerOpened()
			|| !bSearchContainerOpen
			|| ActiveSearchContainer.Get() != Container
			|| !SearchContainerWidget)
		{
			return false;
		}
		return true;
	};

	ACharacter* Player = Cast<ACharacter>(PlayerPawn.Get());
	Ademo_mapEnemyCharacter* EnhancedMelee =
		FindMelee(Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced);
	Ademo_mapRangedEnemyCharacter* EnhancedRanged =
		FindRanged(Fdemo_mapEnemyEncounterIds::SideRangedEnhanced);
	if (!Player
		|| !EnhancedMelee
		|| !EnhancedRanged
		|| !Items.IsValid()
		|| !EnemyRouteLootProfileSession.IsValid())
	{
		Fail(TEXT("prepared product actors, player, Item Authority, or Profile Session are unavailable"));
		return;
	}

	if (EnemyRouteLootAutomationStep == 0)
	{
		TSet<FName> EncounterIds;
		TSet<FName> SpawnMarkers;
		TSet<FName> LootTables;
		int32 MainCount = 0;
		int32 SideCount = 0;
		for (const Fdemo_mapEnemyEncounterSpawnRecord& Record :
			Fdemo_mapEnemyEncounterConfig::GetSpawnRecords())
		{
			EncounterIds.Add(Record.Identity.EncounterId);
			SpawnMarkers.Add(Record.Identity.SpawnMarkerId);
			LootTables.Add(Record.Identity.LootTableId);
			if (!HasNavigableEnemySpawnMarker(
					Record.Identity.SpawnMarkerId))
			{
				Fail(TEXT("an enemy SpawnMarker has no valid NavMesh path"));
				return;
			}
			if (Record.Identity.RouteId
				== Fdemo_mapEnemyEncounterIds::MainRoute)
			{
				++MainCount;
			}
			else
			{
				++SideCount;
			}
		}
		const int32 ExpectedChestCount =
			bRewardSourceProjectionAutomation
				? Fdemo_mapRewardFullMapDistribution::TotalContainerCount
				: 3;
		const int32 ExpectedEnemyCount =
			bRewardSourceProjectionAutomation
				? Fdemo_mapRewardFullMapDistribution::TotalEnemyCount
				: 5;
		if (EnemyActors.Num() != ExpectedEnemyCount
			|| Chests.Num() != ExpectedChestCount
			|| EncounterIds.Num() != 5
			|| SpawnMarkers.Num() != 5
			|| LootTables.Num() != 5
			|| MainCount != 3
			|| SideCount != 2
			|| Items->GetRunState() != Edemo_mapRunState::Active
			|| Items->GetActiveRunId()
				!= EnemyRouteLootProfileSession->GetSnapshot().ActiveRunId)
		{
			Fail(TEXT("five-enemy, three-chest, main/side, or prepared ActiveRun projection is invalid"));
			return;
		}
		const Fdemo_mapEnemySkillDefinition& Definition =
			Fdemo_mapEnemySkillPrototypeConfig::Get().EnhancedMeleeDash;
		const float Distance =
			(Definition.TriggerMinDistance
				+ Definition.TriggerMaxDistance) * 0.5f;
		bool bPlaced = false;
		for (const FVector& Axis : {
			FVector::ForwardVector,
			FVector::RightVector,
			-FVector::ForwardVector,
			-FVector::RightVector })
		{
			FVector Candidate =
				Player->GetActorLocation() + Axis * Distance;
			Candidate.Z = Player->GetActorLocation().Z;
			EnhancedMelee->SetActorLocation(
				Candidate,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			const Fdemo_mapCombatDisplacementResult Preflight =
				Fdemo_mapCombatDisplacement::PreflightWorldStatic(
					EnhancedMelee,
					-Axis,
					Definition.DisplacementDistance,
					Fdemo_mapEnemySkillPrototypeConfig::Get().WorldStaticSkin);
			if (Preflight.ResolvedDistance + KINDA_SMALL_NUMBER
				>= Definition.MinimumResolvedDistance)
			{
				bPlaced = true;
				break;
			}
		}
		if (!bPlaced)
		{
			Fail(TEXT("enhanced melee has no legal product-world AI setup"));
			return;
		}
		EnhancedMelee->ResetEnemySkillForNewRun();
		EnhancedMelee->SetCombatSuppressed(false);
		EnemyRouteLootAutomationStartTime = GetWorld()->GetTimeSeconds();
		EnemyRouteLootAutomationStep = 1;
		Schedule(0.05f);
		return;
	}

	if (EnemyRouteLootAutomationStep == 1)
	{
		Udemo_mapEnemySkillRuntimeComponent* Runtime =
			EnhancedMelee->GetEnemySkillRuntime();
		if (!Runtime)
		{
			Fail(TEXT("enhanced melee skill Runtime is missing"));
			return;
		}
		if (Runtime->GetActivationCount() == 1
			&& Runtime->GetLegalHitCount() == 1
			&& !Runtime->IsActive())
		{
			EnhancedMelee->SetCombatSuppressed(true);
			const Fdemo_mapEnemySkillDefinition& Definition =
				Fdemo_mapEnemySkillPrototypeConfig::Get()
					.EnhancedRangedBackstepShot;
			const float Distance =
				Definition.TriggerMaxDistance
				- Definition.MinimumResolvedDistance;
			bool bPlaced = false;
			for (const FVector& Axis : {
				FVector::ForwardVector,
				FVector::RightVector,
				-FVector::ForwardVector,
				-FVector::RightVector })
			{
				FVector Candidate =
					Player->GetActorLocation() + Axis * Distance;
				Candidate.Z = Player->GetActorLocation().Z;
				EnhancedRanged->SetActorLocation(
					Candidate,
					false,
					nullptr,
					ETeleportType::TeleportPhysics);
				const Fdemo_mapCombatDisplacementResult Preflight =
					Fdemo_mapCombatDisplacement::PreflightWorldStatic(
						EnhancedRanged,
						Axis,
						Definition.DisplacementDistance,
						Fdemo_mapEnemySkillPrototypeConfig::Get().WorldStaticSkin);
				if (Preflight.ResolvedDistance + KINDA_SMALL_NUMBER
					>= Definition.MinimumResolvedDistance)
				{
					bPlaced = true;
					break;
				}
			}
			if (!bPlaced)
			{
				Fail(TEXT("enhanced ranged has no legal product-world AI setup"));
				return;
			}
			EnhancedRanged->ResetEnemySkillForNewRun();
			EnhancedRanged->SetCombatSuppressed(false);
			EnemyRouteLootAutomationStartTime =
				GetWorld()->GetTimeSeconds();
			EnemyRouteLootAutomationStep = 2;
			Schedule(0.05f);
			return;
		}
		if (GetWorld()->GetTimeSeconds()
			- EnemyRouteLootAutomationStartTime > 9.0)
		{
			Fail(TEXT("normal enhanced melee AI did not complete its Dash"));
			return;
		}
		Schedule(0.05f);
		return;
	}

	if (EnemyRouteLootAutomationStep == 2)
	{
		Udemo_mapEnemySkillRuntimeComponent* Runtime =
			EnhancedRanged->GetEnemySkillRuntime();
		if (!Runtime)
		{
			Fail(TEXT("enhanced ranged skill Runtime is missing"));
			return;
		}
		if (Runtime->GetActivationCount() == 1
			&& Runtime->GetResolveCount() == 1
			&& EnhancedRanged->GetTotalProjectilesFired() == 1
			&& !Runtime->IsActive()
			&& EnhancedRanged->GetActiveProjectileCount() == 0)
		{
			EnhancedRanged->SetCombatSuppressed(true);
			EnemyRouteLootAutomationStep = 3;
			Schedule(0.05f);
			return;
		}
		if (GetWorld()->GetTimeSeconds()
			- EnemyRouteLootAutomationStartTime > 10.0)
		{
			Fail(TEXT("normal enhanced ranged AI did not complete Backstep + one projectile"));
			return;
		}
		Schedule(0.05f);
		return;
	}

	if (EnemyRouteLootAutomationStep == 3)
	{
		for (const TWeakObjectPtr<AActor>& Actor : EnemyActors)
		{
			AActor* Enemy = Actor.Get();
			if (!Enemy)
			{
				Fail(TEXT("an enemy disappeared before the standard damage death path"));
				return;
			}
			if (bRewardSourceProjectionAutomation)
			{
				FName EncounterId = NAME_None;
				if (const Ademo_mapEnemyCharacter* Melee =
						Cast<Ademo_mapEnemyCharacter>(Enemy))
				{
					EncounterId =
						Melee->GetEncounterIdentity().EncounterId;
				}
				else if (const Ademo_mapRangedEnemyCharacter* Ranged =
						Cast<Ademo_mapRangedEnemyCharacter>(Enemy))
				{
					EncounterId =
						Ranged->GetEncounterIdentity().EncounterId;
				}
				else if (const Ademo_mapHeavyEnemyCharacter* Heavy =
						Cast<Ademo_mapHeavyEnemyCharacter>(Enemy))
				{
					EncounterId =
						Heavy->GetEncounterIdentity().EncounterId;
				}
				if (EncounterId
						!= Fdemo_mapEnemyEncounterIds::
							MainMeleeStandard
					&& EncounterId
						!= Fdemo_mapEnemyEncounterIds::
							MainMeleeHeavy
					&& EncounterId
						!= Fdemo_mapEnemyEncounterIds::
							MainRangedStandard
					&& EncounterId
						!= Fdemo_mapEnemyEncounterIds::
							SideMeleeEnhanced
					&& EncounterId
						!= Fdemo_mapEnemyEncounterIds::
							SideRangedEnhanced)
				{
					continue;
				}
			}
			UGameplayStatics::ApplyDamage(
				Enemy,
				100.0f,
				nullptr,
				this,
				nullptr);
		}
		EnemyRouteLootAutomationStep = 4;
		Schedule(0.10f);
		return;
	}

	if (EnemyRouteLootAutomationStep == 4)
	{
		TSet<FName> CorpseTables;
		bool bProjectionCorpsesValid = true;
		for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Corpse :
			Corpses)
		{
			if (Corpse.IsValid())
			{
				CorpseTables.Add(Corpse->GetLootTableId());
				if (bRewardSourceProjectionAutomation)
				{
					const auto& ProjectionPlan =
						Corpse->GetProjectionResult();
					TSet<Edemo_mapRuntimeContainerSection> Sections;
					for (const auto& Stack :
						ProjectionPlan.PlannedStacks)
					{
						Sections.Add(Stack.Section);
					}
					const Fdemo_mapRewardSourceProjection* Projection =
						Fdemo_mapRewardSourceProjectionRegistry::Find(
							Corpse->GetRewardProjectionId());
					FString SectionEvidence;
					for (const auto& SectionTrace :
						ProjectionPlan.Trace.Sections)
					{
						if (!SectionEvidence.IsEmpty())
						{
							SectionEvidence += TEXT("|");
						}
						SectionEvidence += FString::Printf(
							TEXT("%s:%lld+%lld=%lld/%lld/%lld"),
							*SectionTrace.SectionId.ToString(),
							SectionTrace.InitialBudget,
							SectionTrace.RedistributedIn,
							SectionTrace.FinalBudget,
							SectionTrace.GeneratedValue,
							SectionTrace.ResidualValue);
					}
					UE_LOG(
						Logdemo_map,
						Log,
						TEXT("P2_REWARD_SOURCE_PROJECTION_CORPSE_SECTIONS projection=%s role=%s profile=%s source=%s stacks=%d total_budget=%lld total_generated=%lld sections=%s fallback=0."),
						*Corpse->GetRewardProjectionId().ToString(),
						*ProjectionPlan.Trace.StableSourceRoleId.ToString(),
						Projection
							? *Projection->BudgetProfileId.ToString()
							: TEXT("None"),
						*Corpse->GetLootSourceId().ToString(
							EGuidFormats::Digits),
						ProjectionPlan.PlannedStacks.Num(),
						ProjectionPlan.Trace.RandomizedBudget,
						ProjectionPlan.Trace.GeneratedTotalValue,
						*SectionEvidence);
					bProjectionCorpsesValid &=
						Corpse->UsesGeneratedReward()
						&& Projection
						&& ProjectionPlan.IsSuccess()
						&& !ProjectionPlan.Trace.bFallbackUsed
						&& Sections.Num() == 3;
				}
			}
		}
		if (Corpses.Num() != 5
			|| CorpseTables.Num() != 5
			|| !bProjectionCorpsesValid
			|| Items->GetWorldActorCount()
				!= EnemyRouteLootWorldItemCountBefore)
		{
			Fail(TEXT("five deaths did not create exactly five unified generated Corpses with three non-empty Sections and zero loose enemy loot"));
			return;
		}
		Ademo_mapCorpseContainerActor* DuplicateProbe =
			Corpses[0].Get();
		if (!DuplicateProbe
			|| HandleEnemyDeath(
					DuplicateProbe->GetLootTableId(),
					DuplicateProbe->GetLootSourceId(),
					DuplicateProbe->GetActorLocation(),
					nullptr).bSuccess
			|| Corpses.Num() != 5)
		{
			Fail(TEXT("duplicate death-source idempotence failed"));
			return;
		}

		for (FName TableId : {
			Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard,
			Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced })
		{
			Ademo_mapCorpseContainerActor* Corpse =
				FindCorpse(TableId);
			if (!OpenThroughProductInput(Corpse))
			{
				Fail(TEXT("real G did not open a main/side Corpse through the product Widget"));
				return;
			}
			Fdemo_mapRuntimeContainerSnapshot Snapshot =
				Corpse->GetContainerSnapshot();
			const Fdemo_mapRuntimeContainerEntrySnapshot* Equipment =
				FindEntry(
					Snapshot,
					Edemo_mapRuntimeContainerSection::Equipment,
					0);
			if (!Equipment
				|| Equipment->State
					!= Edemo_mapRuntimeContainerEntryState::Identified)
			{
				Fail(TEXT("Corpse Equipment was not immediately identified"));
				return;
			}
			const FGuid EquipmentId = Equipment->ItemInstanceId;
			if (!SearchContainerWidget->AutomationClickEntry(
					Edemo_mapRuntimeContainerSection::Equipment,
					0)
				|| !SearchContainerWidget->GetLastResult().bSuccess
				|| SearchContainerWidget->GetLastResult().ItemInstanceId
					!= EquipmentId
				|| !Items->GetAuthority().FindInstance(EquipmentId))
			{
				Fail(TEXT("Equipment Take did not preserve the materialized GUID"));
				return;
			}
			Snapshot = Corpse->GetContainerSnapshot();
			const Fdemo_mapRuntimeContainerEntrySnapshot* Backpack =
				FindEntry(
					Snapshot,
					Edemo_mapRuntimeContainerSection::Backpack,
					0);
			if (!Backpack
				|| Backpack->State
					!= Edemo_mapRuntimeContainerEntryState::Hidden
				|| !SearchContainerWidget->AutomationClickEntry(
					Edemo_mapRuntimeContainerSection::Backpack,
					0)
				|| !Corpse->IsContainerSearching()
				|| !Corpse->CompleteActionForAutomation().bSuccess)
			{
				Fail(TEXT("Backpack 1.0-second search path did not execute"));
				return;
			}
			Snapshot = Corpse->GetContainerSnapshot();
			Backpack = FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Backpack,
				0);
			const FGuid BackpackId =
				Backpack ? Backpack->ItemInstanceId : FGuid();
			if (!BackpackId.IsValid()
				|| !SearchContainerWidget->AutomationClickEntry(
					Edemo_mapRuntimeContainerSection::Backpack,
					0)
				|| !SearchContainerWidget->GetLastResult().bSuccess
				|| SearchContainerWidget->GetLastResult().ItemInstanceId
					!= BackpackId)
			{
				Fail(TEXT("searched whole stack Take did not keep one GUID"));
				return;
			}
			Snapshot = Corpse->GetContainerSnapshot();
			const Fdemo_mapRuntimeContainerEntrySnapshot* Body =
				FindEntry(
					Snapshot,
					Edemo_mapRuntimeContainerSection::Body,
					0);
			if (!Body
				|| Body->State
					!= Edemo_mapRuntimeContainerEntryState::Hidden
				|| !SearchContainerWidget->AutomationClickEntry(
					Edemo_mapRuntimeContainerSection::Body,
					0)
				|| !Corpse->IsContainerSearching()
				|| !Corpse->CompleteActionForAutomation().bSuccess)
			{
				Fail(TEXT("Body 1.5-second search path did not execute"));
				return;
			}
			Snapshot = Corpse->GetContainerSnapshot();
			Body = FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Body,
				0);
			const FGuid BodyId =
				Body ? Body->ItemInstanceId : FGuid();
			if (!BodyId.IsValid()
				|| !SearchContainerWidget->AutomationClickEntry(
					Edemo_mapRuntimeContainerSection::Body,
					0)
				|| !SearchContainerWidget->GetLastResult().bSuccess
				|| SearchContainerWidget->GetLastResult().ItemInstanceId
					!= BodyId)
			{
				Fail(TEXT("Body searched Take did not keep one GUID"));
				return;
			}
			if (bRewardSourceProjectionAutomation)
			{
				RewardSourceProjectionTakenIds.Add(EquipmentId);
				RewardSourceProjectionTakenIds.Add(BackpackId);
				RewardSourceProjectionTakenIds.Add(BodyId);
			}
			if (!SearchContainerWidget->AutomationClickClose()
				|| bSearchContainerOpen)
			{
				Fail(TEXT("Corpse Widget close failed"));
				return;
			}
		}
		EnemyRouteLootAutomationStep = 5;
		Schedule(0.05f);
		return;
	}

	if (EnemyRouteLootAutomationStep == 5)
	{
		TSet<FName> ChestTables;
		bool bProjectionChestsValid = true;
		TArray<Ademo_mapLootChest*> ChestsToExercise;
		TMap<FName, Ademo_mapLootChest*> ProjectionRepresentatives;
		TMap<FName, int32> ProjectionCounts;
		for (const TWeakObjectPtr<Ademo_mapLootChest>& ChestPtr : Chests)
		{
			Ademo_mapLootChest* Chest = ChestPtr.Get();
			if (Chest && bRewardSourceProjectionAutomation)
			{
				const Fdemo_mapRewardSourceProjection* Projection =
					Fdemo_mapRewardSourceProjectionRegistry::Find(
						Chest->GetRewardProjectionId());
				TSet<FName> EligibleDefinitions;
				if (Projection)
				{
					for (const auto& Entry :
						Fdemo_mapRewardSourceProjectionRegistry::
							BuildSectionPool(
								*Projection,
								Projection->Sections[0]))
					{
						EligibleDefinitions.Add(Entry.DefinitionId);
					}
				}
				bProjectionChestsValid &=
					Projection
					&& Chest->GetRewardSourceMode()
						== Edemo_mapRewardSourceMode::GeneratedReward
					&& Chest->GetLastProjectionResult().IsSuccess()
					&& !Chest->UsedFixedFallback()
					&& !Chest->GetLastProjectionResult().
						PlannedStacks.ContainsByPredicate(
							[&EligibleDefinitions](const auto& Stack)
							{
								return !EligibleDefinitions.Contains(
									Stack.DefinitionId);
							});
				const FName ProjectionId =
					Chest->GetRewardProjectionId();
				ProjectionCounts.FindOrAdd(ProjectionId)++;
				Ademo_mapLootChest*& Representative =
					ProjectionRepresentatives.FindOrAdd(ProjectionId);
				const FName StableRole =
					Chest->GetLastProjectionResult().Trace.
						StableSourceRoleId;
				const FName ExistingRole = Representative
					? Representative->GetLastProjectionResult().Trace.
						StableSourceRoleId
					: NAME_None;
				if (!Representative
					|| StableRole.LexicalLess(ExistingRole))
				{
					Representative = Chest;
				}
			}
			else if (Chest)
			{
				ChestsToExercise.Add(Chest);
			}
		}
		if (bRewardSourceProjectionAutomation)
		{
			for (const FName ProjectionId : {
				Fdemo_mapRewardProjectionIds::ChestMainWood,
				Fdemo_mapRewardProjectionIds::ChestMainOre,
				Fdemo_mapRewardProjectionIds::ChestSideHighValue })
			{
				Ademo_mapLootChest* const* Representative =
					ProjectionRepresentatives.Find(ProjectionId);
				if (!Representative || !*Representative)
				{
					bProjectionChestsValid = false;
					continue;
				}
				ChestsToExercise.Add(*Representative);
				UE_LOG(
					Logdemo_map,
					Log,
					TEXT("P2_REWARD_SOURCE_PROJECTION_CURRENT_TOPOLOGY projection=%s role=%s selected=1 topology_chests=%d."),
					*ProjectionId.ToString(),
					*(*Representative)->GetLastProjectionResult().Trace.
						StableSourceRoleId.ToString(),
					Chests.Num());
			}
			bProjectionChestsValid &=
				Chests.Num()
					== Fdemo_mapRewardFullMapDistribution::
						TotalContainerCount
				&& ProjectionCounts.FindRef(
					Fdemo_mapRewardProjectionIds::ChestMainWood) == 60
				&& ProjectionCounts.FindRef(
					Fdemo_mapRewardProjectionIds::ChestMainOre) == 60
				&& ProjectionCounts.FindRef(
					Fdemo_mapRewardProjectionIds::
						ChestSideHighValue) == 15
				&& ChestsToExercise.Num() == 3;
		}
		for (Ademo_mapLootChest* Chest : ChestsToExercise)
		{
			if (!Chest || !OpenThroughProductInput(Chest))
			{
				Fail(TEXT("real G did not open all three current-topology representative Chests"));
				return;
			}
			ChestTables.Add(Chest->GetLootTableId());
			if (!SearchContainerWidget->AutomationClickClose()
				|| bSearchContainerOpen)
			{
				Fail(TEXT("Chest Widget close failed"));
				return;
			}
		}
		const Fdemo_mapProfileSessionSnapshot BeforeCleanup =
			EnemyRouteLootProfileSession->GetSnapshot();
		FString InvariantError;
		bool bTakenGuidAndInputValid = true;
		if (bRewardSourceProjectionAutomation)
		{
			bTakenGuidAndInputValid =
				RewardSourceProjectionTakenIds.Num() == 6
				&& GetDemoController()
				&& GetDemoController()->IsGameplayInputAllowed()
				&& !GetDemoController()->IsMoveInputIgnored()
				&& !GetDemoController()->IsLookInputIgnored();
			for (const FGuid TakenId :
				RewardSourceProjectionTakenIds)
			{
				const Fdemo_mapItemInstance* Instance =
					Items->GetAuthority().FindInstance(TakenId);
				bTakenGuidAndInputValid &=
					Instance
					&& Instance->InstanceId == TakenId
					&& Instance->OwnershipState
						== Edemo_mapItemOwnershipState::Inventory
					&& Items->GetAuthority().FindInventorySlot(
						TakenId) != INDEX_NONE;
			}
		}
		if (ChestTables.Num() != 3
			|| !bProjectionChestsValid
			|| !bTakenGuidAndInputValid
			|| !ChestTables.Contains(Fdemo_mapFixedLootTableIds::ChestMainA)
			|| !ChestTables.Contains(Fdemo_mapFixedLootTableIds::ChestMainB)
			|| !ChestTables.Contains(Fdemo_mapFixedLootTableIds::ChestSideA)
			|| BeforeCleanup.PersistentSpiritStones
				!= EnemyRouteLootPersistentBalanceBefore
			|| BeforeCleanup.RiskSpiritStones
				!= EnemyRouteLootRiskBalanceBefore
			|| !Items->ValidateInvariants(&InvariantError))
		{
			Fail(TEXT("generated Chest projections, balances, or Item Authority invariants changed"));
			return;
		}
		EnemyRouteLootAutomationStep = -1;
		GetWorldTimerManager().ClearTimer(AutomationTimer);
		Cleanup();
		const Fdemo_mapProfileSessionSnapshot Final =
			EnemyRouteLootProfileSession->GetSnapshot();
		if (Final.SessionState
				!= Edemo_mapProfileSessionState::ReadyForPreparation
			|| Final.LastTerminalReason != Edemo_mapRunEndReason::Abandon
			|| Final.PersistentSpiritStones
				!= EnemyRouteLootPersistentBalanceBefore
			|| Final.RiskSpiritStones != EnemyRouteLootRiskBalanceBefore
			|| (Items.IsValid()
				&& Items->GetRunState() != Edemo_mapRunState::Settled)
			|| !Chests.IsEmpty()
			|| !Corpses.IsEmpty()
			|| !EnemyActors.IsEmpty())
		{
			if (bRewardSourceProjectionAutomation)
			{
				UE_LOG(Logdemo_map, Error, TEXT("P2_REWARD_SOURCE_PROJECTION_PRODUCT_SMOKE: FAIL: reset cleanup or zero-balance contract failed."));
			}
			else
			{
				UE_LOG(Logdemo_map, Error, TEXT("ENEMY_ROUTE_LOOT_SMOKE: FAIL — reset cleanup or zero-balance contract failed."));
			}
			FPlatformMisc::RequestExitWithStatus(false, 1);
			return;
		}
		if (bRewardSourceProjectionAutomation)
		{
			FString TakenGuidEvidence;
			for (const FGuid TakenId : RewardSourceProjectionTakenIds)
			{
				if (!TakenGuidEvidence.IsEmpty())
				{
					TakenGuidEvidence += TEXT(",");
				}
				TakenGuidEvidence +=
					TakenId.ToString(EGuidFormats::Digits);
			}
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("P2_REWARD_SOURCE_PROJECTION_TAKE_GUIDS count=%d guids=%s authority=RunInventory."),
				RewardSourceProjectionTakenIds.Num(),
				*TakenGuidEvidence);
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("P2_REWARD_SOURCE_PROJECTION_PRODUCT_EVIDENCE run=%s topology_chests=135 selected_chests=3 corpses=5 standard=3 elite=2 sections=Equipment,Backpack,Body taken_guids=%d fallback=0 input_restore=1 cleanup=1."),
				*BeforeCleanup.ActiveRunId.ToString(EGuidFormats::Digits),
				RewardSourceProjectionTakenIds.Num());
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("P2_REWARD_SOURCE_PROJECTION_PRODUCT_SMOKE: PASS."));
		}
		else
		{
			UE_LOG(Logdemo_map, Log, TEXT("ENEMY_ROUTE_LOOT_SMOKE: PASS."));
		}
		FPlatformMisc::RequestExitWithStatus(false, 0);
		return;
	}

		Fail(TEXT("invalid automation state"));
	}

	void Ademo_mapV3ProgressionManager::
	RunRewardFullMapDistributionAutomation()
	{
		auto Cleanup = [this](Edemo_mapRunEndReason Reason)
		{
			CloseSearchContainer(
				TEXT("RewardFullMapSmokeCleanup"), true);
			DestroyRuntimeContainers(
				TEXT("RewardFullMapSmokeCleanup"));
			if (Items.IsValid()
				&& Items->GetRunState()
					== Edemo_mapRunState::Active)
			{
				Fdemo_mapSettlementSummary Summary;
				if (Items->RequestSettlement(
						Reason,
						Summary).bSuccess
					&& RewardGenerationProfileSession.IsValid())
				{
					RewardGenerationProfileSession->
						CommitRuntimeSettlement(Summary);
				}
			}
			if (Items.IsValid())
			{
				Items->TeardownWorld(GetWorld());
			}
			InitialWorldItems.Reset();
		};
		auto Fail = [this, &Cleanup](const FString& Stage)
		{
			GetWorldTimerManager().ClearTimer(AutomationTimer);
			Cleanup(Edemo_mapRunEndReason::Abandon);
			FString Clean = Stage;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P8_FULL_MAP_REWARD_DISTRIBUTION_PRODUCT_SMOKE: FAIL: %s"),
				*Clean);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		auto OpenThroughProductInput = [this](
			Ademo_mapSearchContainerActor* Container) -> bool
		{
			if (!Container || !PlayerPawn.IsValid()
				|| !GetDemoController())
			{
				return false;
			}
			MovePawnNear(Container);
			GetDemoController()->SetAutomationAimDirection(
				Container->GetActorLocation()
					- PlayerPawn->GetActorLocation());
			SetFocusedActor(Container);
			if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G))
			{
				return false;
			}
			if (Container->IsContainerOpening()
				&& !Container->CompleteActionForAutomation().
					bSuccess)
			{
				return false;
			}
			return GetDemoController()->
					DispatchAutomationKeyReleased(EKeys::G)
				&& Container->IsContainerOpened()
				&& bSearchContainerOpen
				&& ActiveSearchContainer.Get() == Container
				&& SearchContainerWidget;
		};
		auto FindEntry = [](
			const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex)
			-> const Fdemo_mapRuntimeContainerEntrySnapshot*
		{
			const auto* FoundSection =
				Snapshot.Sections.FindByPredicate(
					[Section](const auto& Candidate)
					{
						return Candidate.Section == Section;
					});
			return FoundSection
				? FoundSection->OrderedOccupiedEntries.
					FindByPredicate(
						[SlotIndex](const auto& Candidate)
						{
							return Candidate.SlotIndex
								== SlotIndex;
						})
				: nullptr;
		};
		auto SearchIdentify = [this, &FindEntry](
			Ademo_mapSearchContainerActor* Container,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex,
			Fdemo_mapRuntimeContainerEntrySnapshot& OutEntry)
			-> bool
		{
			const auto Before =
				Container->GetContainerSnapshot();
			const auto* Entry =
				FindEntry(Before, Section, SlotIndex);
			if (!Entry)
			{
				return false;
			}
			if (Entry->State
				== Edemo_mapRuntimeContainerEntryState::Hidden)
			{
				if (!SearchContainerWidget->
					AutomationClickEntry(Section, SlotIndex))
				{
					return false;
				}
				if (Container->IsContainerSearching()
					&& !Container->
						CompleteActionForAutomation().bSuccess)
				{
					return false;
				}
			}
			const auto After =
				Container->GetContainerSnapshot();
			Entry = FindEntry(After, Section, SlotIndex);
			if (!Entry
				|| Entry->State
					!= Edemo_mapRuntimeContainerEntryState::
						Identified)
			{
				return false;
			}
			OutEntry = *Entry;
			return true;
		};
		auto WriteEvidence = [this](
			const TCHAR* Filename,
			const FString& Text) -> bool
		{
			const FString EvidenceRoot =
				FPaths::GetPath(
					FPaths::GetPath(
						FPaths::GetPath(
							RewardGenerationStorageRoot)));
			return IFileManager::Get().MakeDirectory(
					*EvidenceRoot,
					true)
				&& FFileHelper::SaveStringToFile(
					Text,
					*FPaths::Combine(
						EvidenceRoot,
						Filename),
					FFileHelper::EEncodingOptions::
						ForceUTF8WithoutBOM);
		};
		auto GetEnemyEncounterId = [](
			AActor* Actor) -> FName
		{
			if (const auto* Heavy =
				Cast<Ademo_mapHeavyEnemyCharacter>(Actor))
			{
				return Heavy->GetEncounterIdentity().
					EncounterId;
			}
			if (const auto* Ranged =
				Cast<Ademo_mapRangedEnemyCharacter>(Actor))
			{
				return Ranged->GetEncounterIdentity().
					EncounterId;
			}
			if (const auto* Melee =
				Cast<Ademo_mapEnemyCharacter>(Actor))
			{
				return Melee->GetEncounterIdentity().
					EncounterId;
			}
			return NAME_None;
		};
		auto GetEnemyLootSourceId = [](
			AActor* Actor) -> FGuid
		{
			if (const auto* Heavy =
				Cast<Ademo_mapHeavyEnemyCharacter>(Actor))
			{
				return Heavy->GetLootSourceId();
			}
			if (const auto* Ranged =
				Cast<Ademo_mapRangedEnemyCharacter>(Actor))
			{
				return Ranged->GetLootSourceId();
			}
			if (const auto* Melee =
				Cast<Ademo_mapEnemyCharacter>(Actor))
			{
				return Melee->GetLootSourceId();
			}
			return FGuid();
		};
		auto FindEnemy = [this, &GetEnemyEncounterId](
			const Fdemo_mapFullMapRewardSlot& Slot) -> AActor*
		{
			for (const TWeakObjectPtr<AActor>& Candidate :
				EnemyActors)
			{
				if (Candidate.IsValid()
					&& GetEnemyEncounterId(Candidate.Get())
						== Slot.EnemyRecord.Identity.
							EncounterId)
				{
					return Candidate.Get();
				}
			}
			return nullptr;
		};
		auto FindChest = [this](
			const Fdemo_mapFullMapRewardSlot& Slot)
			-> Ademo_mapLootChest*
		{
			for (const auto& Candidate : Chests)
			{
				if (Candidate.IsValid()
					&& Candidate->GetRewardSourceId()
						== Slot.StableSourceRoleId)
				{
					return Candidate.Get();
				}
			}
			return nullptr;
		};
		auto FindCorpse = [this](
			const Fdemo_mapFullMapRewardSlot& Slot)
			-> Ademo_mapCorpseContainerActor*
		{
			for (const auto& Candidate : Corpses)
			{
				if (Candidate.IsValid()
					&& Candidate->GetProjectionResult().
						Trace.StableSourceRoleId
						== Slot.StableSourceRoleId)
				{
					return Candidate.Get();
				}
			}
			return nullptr;
		};
		auto IsWoodDefinition = [](FName DefinitionId)
		{
			return DefinitionId
					== Fdemo_mapItemIds::SpiritWoodLevel1
				|| DefinitionId
					== Fdemo_mapItemIds::SpiritWoodLevel2
				|| DefinitionId
					== Fdemo_mapItemIds::SpiritWoodLevel3;
		};
		auto IsOreDefinition = [](FName DefinitionId)
		{
			return DefinitionId
					== Fdemo_mapItemIds::SpiritOreLevel1
				|| DefinitionId
					== Fdemo_mapItemIds::SpiritOreLevel2
				|| DefinitionId
					== Fdemo_mapItemIds::SpiritOreLevel3;
		};

		if (!Items.IsValid()
			|| !RewardGenerationProfileSession.IsValid()
			|| Items->GetRunState()
				!= Edemo_mapRunState::Active)
		{
			Fail(TEXT(
				"Stage A: isolated ActiveRun authorities are unavailable"));
			return;
		}
		FString PolicyError;
		const auto Counts =
			Fdemo_mapRewardFullMapDistribution::Count();
		const auto& Slots =
			Fdemo_mapRewardFullMapDistribution::GetSlots();
		const Fdemo_mapFullMapRewardSlot* Standard = nullptr;
		const Fdemo_mapFullMapRewardSlot* Elite = nullptr;
		const Fdemo_mapFullMapRewardSlot* Boss = nullptr;
		const Fdemo_mapFullMapRewardSlot* Wood = nullptr;
		const Fdemo_mapFullMapRewardSlot* Ore = nullptr;
		const Fdemo_mapFullMapRewardSlot* High = nullptr;
		TSet<FName> DeclaredSlotIds;
		TSet<FName> DeclaredRoles;
		for (const auto& Slot : Slots)
		{
			DeclaredSlotIds.Add(Slot.SlotId);
			DeclaredRoles.Add(Slot.StableSourceRoleId);
			if (!Standard
				&& Slot.RewardClass
					== Edemo_mapFullMapRewardClass::
						EnemyStandard)
			{
				Standard = &Slot;
			}
			else if (!Elite
				&& Slot.RewardClass
					== Edemo_mapFullMapRewardClass::
						EnemyElite)
			{
				Elite = &Slot;
			}
			else if (Slot.RewardClass
				== Edemo_mapFullMapRewardClass::Boss)
			{
				Boss = &Slot;
			}
			else if (!Wood
				&& Slot.RewardClass
					== Edemo_mapFullMapRewardClass::
						ContainerBasicWood)
			{
				Wood = &Slot;
			}
			else if (!Ore
				&& Slot.RewardClass
					== Edemo_mapFullMapRewardClass::
						ContainerBasicOre)
			{
				Ore = &Slot;
			}
			else if (!High
				&& Slot.RewardClass
					== Edemo_mapFullMapRewardClass::
						ContainerHighValue)
			{
				High = &Slot;
			}
		}
		int32 LiveEnemyCount = 0;
		for (const auto& Enemy : EnemyActors)
		{
			LiveEnemyCount += Enemy.IsValid() ? 1 : 0;
		}
		int32 LiveContainerCount = 0;
		TSet<FName> ActiveContainerRoles;
		bool bAllGenerated = true;
		bool bNoFallback = true;
		bool bAllContainerLedgerCommitted = true;
		for (const auto& Chest : Chests)
		{
			if (!Chest.IsValid())
			{
				continue;
			}
			++LiveContainerCount;
			ActiveContainerRoles.Add(
				Chest->GetRewardSourceId());
			bAllGenerated =
				bAllGenerated
				&& Chest->GetRewardSourceMode()
					== Edemo_mapRewardSourceMode::
						GeneratedReward
				&& Chest->GetLastProjectionResult().
					IsSuccess();
			bNoFallback =
				bNoFallback && !Chest->UsedFixedFallback();
			bAllContainerLedgerCommitted =
				bAllContainerLedgerCommitted
				&& RewardGenerationSession.IsProcessed(
					Items->GetActiveRunId(),
					Chest->GetRewardSourceId());
		}
		const auto BeforeSources =
			RewardGenerationProfileSession->GetSnapshot();
		const FGuid ActiveRunId = BeforeSources.ActiveRunId;
		const bool bRunIdentityBounded =
			ActiveRunId.A == 0x50380000u
			&& ActiveRunId.B == 0x46554C4Cu
			&& ActiveRunId.C == 0x4D415000u
			&& ActiveRunId.D > 0
			&& ActiveRunId.D <= 500000u;
		if (!Fdemo_mapRewardFullMapDistribution::Validate(
				&PolicyError)
			|| Counts.StandardEnemies != 10
			|| Counts.EliteEnemies != 3
			|| Counts.Bosses != 1
			|| Counts.BasicContainerTotal() != 120
			|| Counts.HighValueContainers != 15
			|| Counts.BaseSourceValue != 115500
			|| Slots.Num() != 149
			|| DeclaredSlotIds.Num() != 149
			|| DeclaredRoles.Num() != 149
			|| LiveEnemyCount != 14
			|| LiveContainerCount != 135
			|| ActiveContainerRoles.Num() != 135
			|| NavigableEnemySpawnMarkerIds.Num() != 14
			|| !Standard || !Elite || !Boss
			|| !Wood || !Ore || !High
			|| !bAllGenerated || !bNoFallback
			|| !bAllContainerLedgerCommitted
			|| BeforeSources.ShopStock.Generation != 0
			|| !bRunIdentityBounded)
		{
			Fail(FString::Printf(
				TEXT("Stage A: full-map materialization rejected policy=%s enemies=%d containers=%d active_roles=%d nav=%d slots=%d roles=%d generated=%d fallback=%d ledger=%d shop=%d run=%s"),
				*PolicyError,
				LiveEnemyCount,
				LiveContainerCount,
				ActiveContainerRoles.Num(),
				NavigableEnemySpawnMarkerIds.Num(),
				DeclaredSlotIds.Num(),
				DeclaredRoles.Num(),
				bAllGenerated ? 1 : 0,
				bNoFallback ? 0 : 1,
				bAllContainerLedgerCommitted ? 1 : 0,
				BeforeSources.ShopStock.Generation,
				*ActiveRunId.ToString(
					EGuidFormats::DigitsWithHyphensLower)));
			return;
		}
		FString SlotsCsv =
			TEXT("slot_id,source_role_id,class,kind,marker_id,route_id,area_id,projection_id,budget_profile_id,base_source_value,x,y,z\n");
		for (const auto& Slot : Slots)
		{
			AActor* RuntimeActor =
				Slot.IsEnemy()
					? FindEnemy(Slot)
					: static_cast<AActor*>(FindChest(Slot));
			if (!RuntimeActor)
			{
				Fail(FString::Printf(
					TEXT("Stage A: runtime actor missing for slot %s"),
					*Slot.SlotId.ToString()));
				return;
			}
			const FVector Location =
				RuntimeActor->GetActorLocation();
			SlotsCsv += FString::Printf(
				TEXT("%s,%s,%d,%s,%s,%s,%s,%s,%s,%lld,%.3f,%.3f,%.3f\n"),
				*Slot.SlotId.ToString(),
				*Slot.StableSourceRoleId.ToString(),
				static_cast<int32>(Slot.RewardClass),
				Slot.IsEnemy() ? TEXT("Enemy")
					: TEXT("Container"),
				*Slot.MarkerId.ToString(),
				*Slot.RouteId.ToString(),
				*Slot.AreaId.ToString(),
				*Slot.ProjectionId.ToString(),
				*Slot.BudgetProfileId.ToString(),
				Slot.BaseSourceValue,
				Location.X,
				Location.Y,
				Location.Z);
		}
		const FGuid InitialContainerId =
			Chests[0]->GetContainerSnapshot().ContainerId;
		const FGuid InitialEnemySourceId =
			GetEnemyLootSourceId(FindEnemy(*Standard));
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_STAGE_A: PASS default_map=/Game/DemoV3/Maps/L_V3_ProgressionDemo run=%s bounded_limit=500000 actual_attempt=%u slots=149 enemies=14 containers=135 standard=10 elite=3 boss=1 basic=120 high=15 base=115500 unique_roles=149 reachable=149 partial_spawn=0 invalid_placement=0 critical_overlap=0 shop_generation=0."),
			*ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			ActiveRunId.D);

		const Fdemo_mapFullMapRewardSlot* KillSlots[] = {
			Standard,
			Elite,
			Boss
		};
		FGuid BossLootSourceId;
		FVector BossDeathLocation = FVector::ZeroVector;
		AActor* BossActor = nullptr;
		for (const Fdemo_mapFullMapRewardSlot* Slot :
			KillSlots)
		{
			AActor* Enemy = FindEnemy(*Slot);
			if (!Enemy)
			{
				Fail(FString::Printf(
					TEXT("Stage B: product enemy missing for %s"),
					*Slot->SlotId.ToString()));
				return;
			}
			if (Slot == Boss)
			{
				BossActor = Enemy;
				BossLootSourceId =
					GetEnemyLootSourceId(Enemy);
				BossDeathLocation =
					Enemy->GetActorLocation();
			}
			UGameplayStatics::ApplyDamage(
				Enemy,
				100000.0f,
				GetDemoController(),
				PlayerPawn.Get(),
				nullptr);
			if (!FindCorpse(*Slot))
			{
				Fail(FString::Printf(
					TEXT("Stage B: accepted player damage did not create corpse for %s"),
					*Slot->SlotId.ToString()));
				return;
			}
		}
		Ademo_mapCorpseContainerActor* StandardCorpse =
			FindCorpse(*Standard);
		Ademo_mapCorpseContainerActor* EliteCorpse =
			FindCorpse(*Elite);
		Ademo_mapCorpseContainerActor* BossCorpse =
			FindCorpse(*Boss);
		if (!StandardCorpse || !EliteCorpse || !BossCorpse
			|| Corpses.Num() != 3
			|| RewardGenerationProfileSession->GetSnapshot().
				ShopStock != BeforeSources.ShopStock)
		{
			Fail(TEXT(
				"Stage B: representative deaths or Shop isolation failed"));
			return;
		}
		const int32 BossCorpseCountBeforeDuplicate =
			Corpses.Num();
		const auto DuplicateBoss = HandleEnemyDeath(
			Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy,
			BossLootSourceId,
			BossDeathLocation,
			BossActor);
		if (DuplicateBoss.bSuccess
			|| Corpses.Num()
				!= BossCorpseCountBeforeDuplicate)
		{
			Fail(TEXT(
				"Stage B: repeated Boss death created a second source"));
			return;
		}

		TArray<FGuid> SelectedItemIds;
		TArray<FName> SelectedDefinitionIds;
		TArray<FName> SelectedSourceRoles;
		FString LedgerRows;
		auto ProcessSource = [
			this,
			&OpenThroughProductInput,
			&SearchIdentify,
			&SelectedItemIds,
			&SelectedDefinitionIds,
			&SelectedSourceRoles,
			&LedgerRows](
			Ademo_mapSearchContainerActor* Container,
			const Fdemo_mapRewardSourceProjectionResult& Plan,
			const Fdemo_mapFullMapRewardSlot& Slot,
			bool bRequireAffixedWeapon,
			FGuid& OutItemId,
			Fdemo_mapRuntimeContainerEntrySnapshot& OutEntry)
			-> bool
		{
			OutItemId.Invalidate();
			const auto Projection =
				Fdemo_mapRewardFullMapDistribution::
					BuildProjection(Slot);
			if (!Container
				|| !Plan.IsSuccess()
				|| Plan.Trace.bFallbackUsed
				|| Plan.Trace.ProjectionId
					!= Slot.ProjectionId
				|| Plan.Trace.StableSourceRoleId
					!= Slot.StableSourceRoleId
				|| Projection.BudgetProfileId
					!= Slot.BudgetProfileId
				|| !RewardGenerationSession.IsProcessed(
					Items->GetActiveRunId(),
					Slot.StableSourceRoleId))
			{
				return false;
			}
			const Fdemo_mapRewardPlannedStack* Target =
				Plan.PlannedStacks.FindByPredicate(
					[this, bRequireAffixedWeapon](
						const auto& Stack)
					{
						const auto* Definition =
							Fdemo_mapItemDefinitions::Find(
								Stack.DefinitionId);
						if (!Definition)
						{
							return false;
						}
						if (bRequireAffixedWeapon)
						{
							return Definition->CategoryId
									== Fdemo_mapItemIds::
										WeaponCategory
								&& !Stack.AffixSet.Affixes.
									IsEmpty();
						}
						return Definition->MaxStackSize == 1
							|| Items->GetAuthority().
								FindInventoryInstancesByDefinition(
									Stack.DefinitionId).
										IsEmpty();
					});
			if (!Target && !Plan.PlannedStacks.IsEmpty())
			{
				Target = &Plan.PlannedStacks[0];
			}
			if (!Target
				|| !OpenThroughProductInput(Container)
				|| !SearchIdentify(
					Container,
					Target->Section,
					Target->SlotIndex,
					OutEntry)
				|| OutEntry.RewardSourceRoleId
					!= Slot.StableSourceRoleId)
			{
				return false;
			}
			OutItemId = OutEntry.ItemInstanceId;
			const uint64 SeedBefore =
				Plan.Trace.EffectiveSeed;
			if (!SearchContainerWidget->
					AutomationClickEntry(
						Target->Section,
						Target->SlotIndex)
				|| !SearchContainerWidget->
					GetLastResult().bSuccess
				|| SearchContainerWidget->
					GetLastResult().ItemInstanceId
					!= OutItemId
				|| Items->GetAuthority().
					FindInventorySlot(OutItemId)
					== INDEX_NONE)
			{
				return false;
			}
			const auto* Taken =
				Items->GetAuthority().FindInstance(
					OutItemId);
			if (!Taken
				|| Taken->RewardSourceRoleId
					!= Slot.StableSourceRoleId
				|| Taken->DefinitionId
					!= OutEntry.DefinitionId
				|| Taken->AffixSet != OutEntry.AffixSet
				|| !SearchContainerWidget->
					AutomationClickClose()
				|| bSearchContainerOpen
				|| !GetDemoController()->
					IsGameplayInputAllowed()
				|| !OpenThroughProductInput(Container)
				|| Plan.Trace.EffectiveSeed
					!= SeedBefore
				|| !SearchContainerWidget->
					AutomationClickClose()
				|| bSearchContainerOpen
				|| !GetDemoController()->
					IsGameplayInputAllowed())
			{
				return false;
			}
			if (!LedgerRows.IsEmpty())
			{
				LedgerRows += TEXT(",\n");
			}
			LedgerRows += FString::Printf(
				TEXT("    {\"slot_id\":\"%s\",\"source_role_id\":\"%s\",\"projection_id\":\"%s\",\"budget_profile_id\":\"%s\",\"base_source_value\":%lld,\"seed\":\"%llu\",\"randomized_budget\":%lld,\"generated_value\":%lld,\"residual_value\":%lld,\"jackpot_hit\":%s,\"rare_hit\":%s,\"affixed_count\":%d,\"fallback\":false,\"item_guid\":\"%s\",\"definition_id\":\"%s\",\"effective_stack_sell_value\":%lld}"),
				*Slot.SlotId.ToString(),
				*Slot.StableSourceRoleId.ToString(),
				*Slot.ProjectionId.ToString(),
				*Slot.BudgetProfileId.ToString(),
				Slot.BaseSourceValue,
				static_cast<unsigned long long>(
					Plan.Trace.EffectiveSeed),
				Plan.Trace.RandomizedBudget,
				Plan.Trace.GeneratedTotalValue,
				Plan.Trace.ResidualValue,
				Plan.Trace.bJackpotHit
					? TEXT("true") : TEXT("false"),
				Plan.Trace.bRareExtremeHit
					? TEXT("true") : TEXT("false"),
				Plan.Trace.AffixedEquipmentCount,
				*OutItemId.ToString(
					EGuidFormats::DigitsWithHyphensLower),
				*OutEntry.DefinitionId.ToString(),
				OutEntry.EffectiveStackSellValue);
			SelectedItemIds.Add(OutItemId);
			SelectedDefinitionIds.Add(
				OutEntry.DefinitionId);
			SelectedSourceRoles.Add(
				Slot.StableSourceRoleId);
			return true;
		};

		const auto& BossPlan =
			BossCorpse->GetProjectionResult();
		const int32 BossEquipmentCount =
			BossPlan.PlannedStacks.FilterByPredicate(
				[](const auto& Stack)
				{
					return Stack.Section
						== Edemo_mapRuntimeContainerSection::
							Equipment;
				}).Num();
		FGuid BossItemId;
		Fdemo_mapRuntimeContainerEntrySnapshot BossEntry;
		if (BossPlan.Trace.bJackpotHit
			|| BossPlan.Trace.bRareExtremeHit
			|| BossEquipmentCount < 1
			|| BossPlan.PlannedStacks.Num() < 3
			|| BossPlan.PlannedStacks.Num() > 6
			|| !ProcessSource(
				BossCorpse,
				BossPlan,
				*Boss,
				true,
				BossItemId,
				BossEntry))
		{
			Fail(TEXT(
				"Stage B/C: natural Boss miss/Affix/Equipment/search/take chain failed"));
			return;
		}
		Udemo_mapAttributeComponent* Attributes =
			PlayerPawn.IsValid()
				? PlayerPawn->FindComponentByClass<
					Udemo_mapAttributeComponent>()
				: nullptr;
		float AttackBaseline = 0.0f;
		float AttackEquipped = 0.0f;
		float AttackUnequipped = 0.0f;
		float AttackReequipped = 0.0f;
		if (!Attributes
			|| !Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackBaseline)
			|| !Items->Equip(
				BossItemId,
				Fdemo_mapItemIds::WeaponSlot).bSuccess
			|| !Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackEquipped)
			|| AttackEquipped <= AttackBaseline
			|| !Items->Unequip(
				Fdemo_mapItemIds::WeaponSlot).bSuccess
			|| !Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackUnequipped)
			|| !FMath::IsNearlyEqual(
				AttackUnequipped,
				AttackBaseline)
			|| !Items->Equip(
				BossItemId,
				Fdemo_mapItemIds::WeaponSlot).bSuccess
			|| !Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackReequipped)
			|| !FMath::IsNearlyEqual(
				AttackReequipped,
				AttackEquipped))
		{
			Fail(TEXT(
				"Stage C: existing equip/unequip/re-equip effect was missing, retained, or doubled"));
			return;
		}

		struct FRepresentative
		{
			const Fdemo_mapFullMapRewardSlot* Slot = nullptr;
			Ademo_mapSearchContainerActor* Container = nullptr;
			const Fdemo_mapRewardSourceProjectionResult* Plan =
				nullptr;
		};
		const FRepresentative Remaining[] = {
			{Standard, StandardCorpse,
				&StandardCorpse->GetProjectionResult()},
			{Elite, EliteCorpse,
				&EliteCorpse->GetProjectionResult()},
			{High, FindChest(*High),
				FindChest(*High)
					? &FindChest(*High)->
						GetLastProjectionResult() : nullptr},
			{Wood, FindChest(*Wood),
				FindChest(*Wood)
					? &FindChest(*Wood)->
						GetLastProjectionResult() : nullptr},
			{Ore, FindChest(*Ore),
				FindChest(*Ore)
					? &FindChest(*Ore)->
						GetLastProjectionResult() : nullptr}
		};
		for (const FRepresentative& Representative :
			Remaining)
		{
			FGuid ItemId;
			Fdemo_mapRuntimeContainerEntrySnapshot Entry;
			if (!Representative.Slot
				|| !Representative.Container
				|| !Representative.Plan
				|| !ProcessSource(
					Representative.Container,
					*Representative.Plan,
					*Representative.Slot,
					false,
					ItemId,
					Entry))
			{
				Fail(TEXT(
					"Stage B: representative real Open/Search/Take/reopen/input-restore chain failed"));
				return;
			}
		}
		const auto& WoodPlan =
			FindChest(*Wood)->GetLastProjectionResult();
		const auto& OrePlan =
			FindChest(*Ore)->GetLastProjectionResult();
		TSet<FGuid> UniqueSelectedItemIds;
		for (const FGuid& SelectedItemId : SelectedItemIds)
		{
			UniqueSelectedItemIds.Add(SelectedItemId);
		}
		if (!WoodPlan.PlannedStacks.ContainsByPredicate(
				[&IsWoodDefinition](const auto& Stack)
				{
					return IsWoodDefinition(
						Stack.DefinitionId);
				})
			|| WoodPlan.PlannedStacks.ContainsByPredicate(
				[&IsOreDefinition](const auto& Stack)
				{
					return IsOreDefinition(
						Stack.DefinitionId);
				})
			|| !OrePlan.PlannedStacks.ContainsByPredicate(
				[&IsOreDefinition](const auto& Stack)
				{
					return IsOreDefinition(
						Stack.DefinitionId);
				})
			|| OrePlan.PlannedStacks.ContainsByPredicate(
				[&IsWoodDefinition](const auto& Stack)
				{
					return IsWoodDefinition(
						Stack.DefinitionId);
				})
			|| SelectedItemIds.Num() != 6
			|| SelectedItemIds.Num()
				!= UniqueSelectedItemIds.Num()
			|| RewardGenerationProfileSession->GetSnapshot().
				ShopStock != BeforeSources.ShopStock)
		{
			Fail(TEXT(
				"Stage B/C: Wood/Ore isolation, six-class same-GUID selection, or Shop isolation failed"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_STAGE_B: PASS kills=Standard,Elite,Boss opens=Wood,Ore,HighValue,StandardCorpse,EliteCorpse,BossCorpse identify=6 take=6 same_guid=6 reopen_no_reroll=6 input_restore=6 boss_corpse_exactly_once=1 fallback=0."));
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_STAGE_C: PASS projection_profile_base_tags=6 randomization_ledger=6 jackpot_miss=1 rare_miss=1 natural_affix=1 equip_once=1 unequip_baseline=1 re_equip_no_double=1 wood_ore_isolation=1 corpse_sections=1 boss_equipment=%d fallback=0 attack_baseline=%.2f attack_equipped=%.2f."),
			BossEquipmentCount,
			AttackBaseline,
			AttackEquipped);

		DestroyRuntimeContainers(
			TEXT("RewardFullMapBeforeExtraction"));
		Fdemo_mapSettlementSummary Summary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				Summary).bSuccess)
		{
			Fail(TEXT(
				"Stage D: real Extraction request failed"));
			return;
		}
		const auto Settlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(Summary);
		const auto AfterSettlement =
			RewardGenerationProfileSession->GetSnapshot();
		if (!Settlement.IsDurablySettled()
			|| AfterSettlement.ShopStock.Generation
				!= BeforeSources.ShopStock.Generation + 1
			|| AfterSettlement.ShopStock.ShopStockEventId
				== BeforeSources.ShopStock.ShopStockEventId)
		{
			Fail(TEXT(
				"Stage D: durable terminal or exactly +1 Shop refresh failed"));
			return;
		}
		Fdemo_mapProfileRepository Repository;
		const auto Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const auto Reloaded =
			Repository.LoadExistingProfile(Storage);
		for (int32 Index = 0;
			Index < SelectedItemIds.Num();
			++Index)
		{
			const auto* Persisted =
				Reloaded.IsSuccess()
					? Reloaded.Profile.PermanentStash.
						FindByPredicate(
							[&SelectedItemIds, Index](
								const auto& Item)
							{
								return Item.ItemInstanceId
									== SelectedItemIds[Index];
							})
					: nullptr;
			if (!Persisted
				|| Persisted->ItemDefinitionId
					!= SelectedDefinitionIds[Index]
				|| Persisted->RewardSourceRoleId
					!= SelectedSourceRoles[Index])
			{
				Fail(TEXT(
					"Stage D: selected GUID/definition/provenance did not survive Profile reload"));
				return;
			}
		}
		const auto Preparation =
			Fdemo_mapProfilePreparationPresenter::
				BuildViewState(
					RewardGenerationProfileSession->
						GetPreparationSnapshot());
		const auto* BossQuote =
			Preparation.OrderedPermanentStashRows.
				FindByPredicate(
					[BossItemId](const auto& Row)
					{
						return Row.ItemInstanceId
							== BossItemId;
					});
		if (!BossQuote || BossQuote->TotalSellPrice <= 0)
		{
			Fail(TEXT(
				"Stage D: Preparation did not expose the exact Boss sell quote"));
			return;
		}
		const int64 SellQuote =
			BossQuote->TotalSellPrice;
		const int64 BalanceBeforeSale =
			AfterSettlement.PersistentSpiritStones;
		Fdemo_mapProfileTradeIntent Sell;
		Sell.Kind = Edemo_mapProfileTradeKind::Sell;
		Sell.ExpectedProfileId =
			AfterSettlement.ProfileId;
		Sell.ExpectedSaveGeneration =
			AfterSettlement.SaveGeneration;
		Sell.ItemInstanceId = BossItemId;
		const auto Sale =
			RewardGenerationProfileSession->
				SubmitTradeIntent(Sell);
		const auto AfterSale =
			RewardGenerationProfileSession->GetSnapshot();
		const auto AfterSaleReload =
			Repository.LoadExistingProfile(Storage);
		if (!Sale.IsCommitted()
			|| Sale.TotalPrice != SellQuote
			|| AfterSale.PersistentSpiritStones
				- BalanceBeforeSale != SellQuote
			|| !AfterSaleReload.IsSuccess()
			|| AfterSaleReload.Profile.PermanentStash.
				ContainsByPredicate(
					[BossItemId](const auto& Item)
					{
						return Item.ItemInstanceId
							== BossItemId;
					}))
		{
			Fail(TEXT(
				"Stage D: real Sell or durable exact-value credit failed"));
			return;
		}
		for (const FGuid& Unsold : SelectedItemIds)
		{
			if (Unsold != BossItemId
				&& !AfterSaleReload.Profile.PermanentStash.
					ContainsByPredicate(
						[Unsold](const auto& Item)
						{
							return Item.ItemInstanceId
								== Unsold;
						}))
			{
				Fail(TEXT(
					"Stage D: unsold selected GUID lost ownership"));
				return;
			}
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_STAGE_D: PASS extraction=1 secured=6 profile_reload=1 metadata=1 map_source_shop_refresh=0 terminal_shop_refresh=1 preparation_quote=%lld sell=1 sold_guid_removed=1 unsold_owned=5 durable_balance=1."),
			SellQuote);

		TSet<FGuid> SeenRunIds;
		SeenRunIds.Add(ActiveRunId);
		TSet<FGuid> SeenRuntimeIds;
		SeenRuntimeIds.Add(InitialContainerId);
		SeenRuntimeIds.Add(InitialEnemySourceId);
		FString LifecycleRows;
		FString Run1Json;
		int32 ExpectedShopGeneration =
			AfterSale.ShopStock.Generation;
		for (int32 Cycle = 1; Cycle <= 3; ++Cycle)
		{
			Items->ResetForAutomation();
			Items->BeginWorld(GetWorld());
			InitialWorldItems.Reset();
			const auto NextRun =
				RewardGenerationProfileSession->
					StartPreparedRun();
			if (!NextRun.IsRunActive()
				|| SeenRunIds.Contains(
					NextRun.Snapshot.ActiveRunId)
				|| NextRun.Snapshot.ShopStock.Generation
					!= ExpectedShopGeneration
				|| GetRewardAffixPityState(
					NextRun.Snapshot.ActiveRunId) != 0
				|| !InitializeWorldContent()
				|| EnemyActors.Num() != 14
				|| Chests.Num() != 135
				|| Corpses.Num() != 0
				|| NavigableEnemySpawnMarkerIds.Num() != 14)
			{
				Fail(FString::Printf(
					TEXT("Stage E cycle %d: new Run reset/materialization/counts failed"),
					Cycle));
				return;
			}
			const FGuid CycleContainerId =
				Chests[0]->GetContainerSnapshot().
					ContainerId;
			const FGuid CycleEnemySourceId =
				GetEnemyLootSourceId(
					FindEnemy(*Standard));
			if (!CycleContainerId.IsValid()
				|| !CycleEnemySourceId.IsValid()
				|| SeenRuntimeIds.Contains(
					CycleContainerId)
				|| SeenRuntimeIds.Contains(
					CycleEnemySourceId))
			{
				Fail(FString::Printf(
					TEXT("Stage E cycle %d: old source actor/container identity was reused"),
					Cycle));
				return;
			}
			SeenRunIds.Add(
				NextRun.Snapshot.ActiveRunId);
			SeenRuntimeIds.Add(CycleContainerId);
			SeenRuntimeIds.Add(CycleEnemySourceId);
			if (!LifecycleRows.IsEmpty())
			{
				LifecycleRows += TEXT(",\n");
			}
			LifecycleRows += FString::Printf(
				TEXT("    {\"cycle\":%d,\"run_id\":\"%s\",\"standard\":10,\"elite\":3,\"boss\":1,\"basic\":120,\"high_value\":15,\"map_reward_sources\":149,\"corpses\":0,\"pity_before_world\":0,\"shop_generation_before_terminal\":%d,\"fresh_container_id\":\"%s\",\"fresh_enemy_source_id\":\"%s\"}"),
				Cycle,
				*NextRun.Snapshot.ActiveRunId.ToString(
					EGuidFormats::DigitsWithHyphensLower),
				ExpectedShopGeneration,
				*CycleContainerId.ToString(
					EGuidFormats::DigitsWithHyphensLower),
				*CycleEnemySourceId.ToString(
					EGuidFormats::DigitsWithHyphensLower));
			if (Cycle == 1)
			{
				Run1Json = FString::Printf(
					TEXT("{\n  \"task_id\":\"Dev.D.UE.0.0.6.P8.0.r0\",\n  \"run_index\":1,\n  \"run_id\":\"%s\",\n  \"standard\":10,\n  \"elite\":3,\n  \"boss\":1,\n  \"basic\":120,\n  \"high_value\":15,\n  \"map_reward_sources\":149,\n  \"container_id\":\"%s\",\n  \"enemy_source_id\":\"%s\",\n  \"pity_state_before_world\":0,\n  \"shop_generation_before_terminal\":%d,\n  \"fresh_identities\":true\n}\n"),
					*NextRun.Snapshot.ActiveRunId.
						ToString(
							EGuidFormats::
								DigitsWithHyphensLower),
					*CycleContainerId.ToString(
						EGuidFormats::
							DigitsWithHyphensLower),
					*CycleEnemySourceId.ToString(
						EGuidFormats::
							DigitsWithHyphensLower),
					ExpectedShopGeneration);
			}
			DestroyRuntimeContainers(
				TEXT("RewardFullMapLifecycleTerminal"));
			Fdemo_mapSettlementSummary CycleSummary;
			if (!Items->RequestSettlement(
					Edemo_mapRunEndReason::Abandon,
					CycleSummary).bSuccess
				|| !RewardGenerationProfileSession->
					CommitRuntimeSettlement(CycleSummary).
						IsDurablySettled())
			{
				Fail(FString::Printf(
					TEXT("Stage E cycle %d: terminal cleanup settlement failed"),
					Cycle));
				return;
			}
			++ExpectedShopGeneration;
			if (RewardGenerationProfileSession->
					GetSnapshot().ShopStock.Generation
				!= ExpectedShopGeneration)
			{
				Fail(FString::Printf(
					TEXT("Stage E cycle %d: terminal Shop refresh was not exactly +1"),
					Cycle));
				return;
			}
			Items->TeardownWorld(GetWorld());
			InitialWorldItems.Reset();
			if (!Chests.IsEmpty()
				|| !Corpses.IsEmpty()
				|| !EnemyActors.IsEmpty()
				|| Items->GetWorldActorCount() != 0)
			{
				Fail(FString::Printf(
					TEXT("Stage E cycle %d: residual runtime object growth detected"),
					Cycle));
				return;
			}
		}
		const FString PolicyJson = FString::Printf(
			TEXT("{\n  \"task_id\":\"Dev.D.UE.0.0.6.P8.0.r0\",\n  \"policy\":\"Fdemo_mapRewardFullMapDistribution\",\n  \"validated_before_mutation\":true,\n  \"stable_array_index_independent_ids\":true,\n  \"standard\":10,\n  \"elite\":3,\n  \"boss\":1,\n  \"basic_wood\":%d,\n  \"basic_ore\":%d,\n  \"basic_total\":120,\n  \"high_value\":15,\n  \"enemy_total\":14,\n  \"container_total\":135,\n  \"slot_total\":149,\n  \"nominal_base_source_value\":115500,\n  \"placement_attempt_limit\":33,\n  \"partial_spawn\":0,\n  \"invalid_placement\":0,\n  \"critical_overlap\":0\n}\n"),
			Counts.BasicWoodContainers,
			Counts.BasicOreContainers);
		const FString Run0Json = FString::Printf(
			TEXT("{\n  \"task_id\":\"Dev.D.UE.0.0.6.P8.0.r0\",\n  \"run_index\":0,\n  \"run_id\":\"%s\",\n  \"bounded_search_limit\":500000,\n  \"bounded_search_actual_attempt\":%u,\n  \"jackpot_hit\":false,\n  \"rare_hit\":false,\n  \"natural_affixed_equipment\":true,\n  \"standard\":10,\n  \"elite\":3,\n  \"boss\":1,\n  \"basic\":120,\n  \"high_value\":15,\n  \"map_reward_sources\":149,\n  \"selected_source_classes\":6,\n  \"same_guid_take\":6,\n  \"fallback\":0\n}\n"),
			*ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			ActiveRunId.D);
		const FString LifecycleJson = FString::Printf(
			TEXT("{\n  \"task_id\":\"Dev.D.UE.0.0.6.P8.0.r0\",\n  \"cycles\":[\n%s\n  ],\n  \"stable_counts\":true,\n  \"fresh_run_ids\":true,\n  \"fresh_runtime_ids\":true,\n  \"duplicate_death_rejected\":true,\n  \"terminal_cleanup\":true,\n  \"reset_cleanup\":true,\n  \"active_run_pity_reset\":true,\n  \"persistent_pity_absent\":true,\n  \"shop_terminal_delta_each_cycle\":1,\n  \"monotonic_growth\":0,\n  \"residual_runtime_objects\":0\n}\n"),
			*LifecycleRows);
		const FString LedgerJson = FString::Printf(
			TEXT("{\n  \"task_id\":\"Dev.D.UE.0.0.6.P8.0.r0\",\n  \"run_id\":\"%s\",\n  \"exactly_once_container_commits\":135,\n  \"representative_sources\":[\n%s\n  ],\n  \"representative_count\":6,\n  \"source_creation_shop_refresh\":0,\n  \"terminal_shop_refresh\":1,\n  \"settlement_secured\":6,\n  \"profile_reload_preserved\":6,\n  \"sold_guid\":\"%s\",\n  \"sell_quote\":%lld,\n  \"unsold_owned\":5\n}\n"),
			*ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*LedgerRows,
			*BossItemId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			SellQuote);
		if (!WriteEvidence(
				TEXT("FullMapDistributionPolicy.json"),
				PolicyJson)
			|| !WriteEvidence(
				TEXT("FullMapDistributionSlots.csv"),
				SlotsCsv)
			|| !WriteEvidence(
				TEXT("FullMapDistributionRun0.json"),
				Run0Json)
			|| !WriteEvidence(
				TEXT("FullMapDistributionRun1.json"),
				Run1Json)
			|| !WriteEvidence(
				TEXT("FullMapDistributionLifecycle.json"),
				LifecycleJson)
			|| !WriteEvidence(
				TEXT("FullMapRewardLedger.json"),
				LedgerJson))
		{
			Fail(TEXT(
				"Stage E: required machine snapshots could not be written"));
			return;
		}
		GetWorldTimerManager().ClearTimer(AutomationTimer);
		const int32 ResidualObjects =
			Items->GetWorldActorCount()
				+ Chests.Num()
				+ Corpses.Num()
				+ EnemyActors.Num();
		if (ResidualObjects != 0)
		{
			Fail(TEXT(
				"Stage E: task-owned Runtime objects remain"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_STAGE_E: PASS new_runs=3 stable_cycles=3 counts=10,3,1,120,15 sources=149 fresh_run_ids=1 fresh_actor_container_ids=1 pity_reset=1 persistent_pity=0 shop_terminal_delta_each=1 monotonic_growth=0 residual_objects=0."));
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_PRODUCT_EVIDENCE run=%s bounded_limit=500000 actual_attempt=%u slots=149 base=115500 representative_sources=6 same_guid=6 fallback=0 natural_jackpot_miss=1 natural_rare_miss=1 natural_affix=1 equip_unequip_reequip=1 settlement=1 reload=1 sell=1 three_cycles=1 residual_objects=0."),
			*ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			ActiveRunId.D);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P8_FULL_MAP_REWARD_DISTRIBUTION_PRODUCT_SMOKE: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	void Ademo_mapV3ProgressionManager::RunRewardBossSourceAutomation()
	{
		auto Cleanup = [this](Edemo_mapRunEndReason Reason)
		{
			CloseSearchContainer(
				TEXT("RewardBossSourceSmokeCleanup"), true);
			DestroyRuntimeContainers(
				TEXT("RewardBossSourceSmokeCleanup"));
			if (Items.IsValid()
				&& Items->GetRunState()
					== Edemo_mapRunState::Active)
			{
				Fdemo_mapSettlementSummary Summary;
				if (Items->RequestSettlement(
						Reason,
						Summary).bSuccess
					&& RewardGenerationProfileSession.IsValid())
				{
					RewardGenerationProfileSession->
						CommitRuntimeSettlement(Summary);
				}
			}
			if (Items.IsValid())
			{
				Items->TeardownWorld(GetWorld());
			}
		};
		auto Fail = [this, &Cleanup](const FString& Stage)
		{
			GetWorldTimerManager().ClearTimer(AutomationTimer);
			Cleanup(Edemo_mapRunEndReason::Abandon);
			FString Clean = Stage;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P7_BOSS_REWARD_PRODUCT_SMOKE: FAIL: %s"),
				*Clean);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		auto OpenThroughProductInput = [this](
			Ademo_mapSearchContainerActor* Container) -> bool
		{
			if (!Container || !PlayerPawn.IsValid()
				|| !GetDemoController())
			{
				return false;
			}
			MovePawnNear(Container);
			GetDemoController()->SetAutomationAimDirection(
				Container->GetActorLocation()
					- PlayerPawn->GetActorLocation());
			SetFocusedActor(Container);
			if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G))
			{
				return false;
			}
			if (Container->IsContainerOpening()
				&& !Container->CompleteActionForAutomation().
					bSuccess)
			{
				return false;
			}
			return GetDemoController()->
					DispatchAutomationKeyReleased(EKeys::G)
				&& Container->IsContainerOpened()
				&& bSearchContainerOpen
				&& ActiveSearchContainer.Get() == Container
				&& SearchContainerWidget;
		};
		auto FindEntry = [](
			const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex)
			-> const Fdemo_mapRuntimeContainerEntrySnapshot*
		{
			const auto* FoundSection =
				Snapshot.Sections.FindByPredicate(
					[Section](const auto& Candidate)
					{
						return Candidate.Section == Section;
					});
			return FoundSection
				? FoundSection->OrderedOccupiedEntries.
					FindByPredicate(
						[SlotIndex](const auto& Candidate)
						{
							return Candidate.SlotIndex
								== SlotIndex;
						})
				: nullptr;
		};
		auto SearchIdentify = [this, &FindEntry](
			Ademo_mapSearchContainerActor* Container,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex,
			Fdemo_mapRuntimeContainerEntrySnapshot& OutEntry)
			-> bool
		{
			const auto Before =
				Container->GetContainerSnapshot();
			const auto* Entry =
				FindEntry(Before, Section, SlotIndex);
			if (!Entry)
			{
				return false;
			}
			if (Entry->State
				== Edemo_mapRuntimeContainerEntryState::Hidden)
			{
				if (!SearchContainerWidget->
					AutomationClickEntry(
						Section,
						SlotIndex))
				{
					return false;
				}
				if (Container->IsContainerSearching()
					&& !Container->
						CompleteActionForAutomation().
							bSuccess)
				{
					return false;
				}
			}
			const auto After =
				Container->GetContainerSnapshot();
			Entry = FindEntry(After, Section, SlotIndex);
			if (!Entry
				|| Entry->State
					!= Edemo_mapRuntimeContainerEntryState::
						Identified)
			{
				return false;
			}
			OutEntry = *Entry;
			return true;
		};
		auto WriteSnapshot = [this](
			const TCHAR* Filename,
			const FString& Json) -> bool
		{
			const FString ProductSmokeRoot =
				FPaths::GetPath(
					FPaths::GetPath(
						RewardGenerationStorageRoot));
			const FString Directory =
				FPaths::Combine(
					ProductSmokeRoot,
					TEXT("Snapshots"));
			return IFileManager::Get().MakeDirectory(
					*Directory,
					true)
				&& FFileHelper::SaveStringToFile(
					Json,
					*FPaths::Combine(
						Directory,
						Filename),
					FFileHelper::EEncodingOptions::
						ForceUTF8WithoutBOM);
		};

		const auto* Boss =
			Fdemo_mapRewardSourceProjectionRegistry::
				FindBossPrototype();
		if (!Items.IsValid()
			|| !RewardGenerationProfileSession.IsValid()
			|| Items->GetRunState()
				!= Edemo_mapRunState::Active
			|| !Boss
			|| Chests.Num()
				!= Fdemo_mapRewardFullMapDistribution::
					TotalContainerCount
			|| EnemyActors.Num()
				!= Fdemo_mapRewardFullMapDistribution::
					TotalEnemyCount)
		{
			Fail(TEXT(
				"Stage A precondition: isolated ActiveRun, Boss Projection, current Chest topology, or current enemy topology is unavailable"));
			return;
		}
		const Fdemo_mapProfileSessionSnapshot BeforeBoss =
			RewardGenerationProfileSession->GetSnapshot();
		const FGuid ActiveRunId =
			BeforeBoss.ActiveRunId;
		if (ActiveRunId.A != 0x50370000u
			|| ActiveRunId.B != 0x424F5353u
			|| ActiveRunId.C != 0x52455744u
			|| ActiveRunId.D == 0
			|| ActiveRunId.D > 500000u)
		{
			Fail(TEXT(
				"Stage A bounded search: selected RunId is outside the P7 finite identity domain"));
			return;
		}
		Ademo_mapHeavyEnemyCharacter* Heavy = nullptr;
		int32 HeavyCount = 0;
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(
			GetWorld()); It; ++It)
		{
			if (It->GetEncounterIdentity().EncounterId
				== Fdemo_mapEnemyEncounterIds::
					MainMeleeHeavy)
			{
				Heavy = *It;
				++HeavyCount;
			}
		}
		if (!Heavy || HeavyCount != 1)
		{
			Fail(TEXT(
				"Stage A: default map does not contain exactly one existing Heavy Boss-role candidate"));
			return;
		}
		const FGuid BossLootSourceId =
			Heavy->GetLootSourceId();
		const FVector BossDeathLocation =
			Heavy->GetActorLocation();
		UGameplayStatics::ApplyDamage(
			Heavy,
			100000.0f,
			GetDemoController(),
			PlayerPawn.Get(),
			nullptr);
		Ademo_mapCorpseContainerActor* BossCorpse = nullptr;
		int32 BossCorpseCount = 0;
		for (const auto& Candidate : Corpses)
		{
			if (Candidate.IsValid()
				&& Candidate->GetRewardProjectionId()
					== Boss->ProjectionId)
			{
				BossCorpse = Candidate.Get();
				++BossCorpseCount;
			}
		}
		const Fdemo_mapProfileSessionSnapshot AfterBoss =
			RewardGenerationProfileSession->GetSnapshot();
		const auto& BossPlan = BossCorpse
			? BossCorpse->GetProjectionResult()
			: Fdemo_mapRewardSourceProjectionResult();
		const auto* AffixedWeapon =
			BossPlan.PlannedStacks.FindByPredicate(
				[](const auto& Stack)
				{
					const auto* Definition =
						Fdemo_mapItemDefinitions::Find(
							Stack.DefinitionId);
					return Definition
						&& Definition->CategoryId
							== Fdemo_mapItemIds::
								WeaponCategory
						&& !Stack.AffixSet.Affixes.IsEmpty();
				});
		const int32 EquipmentCount =
			BossPlan.PlannedStacks.FilterByPredicate(
				[](const auto& Stack)
				{
					return Stack.Section
						== Edemo_mapRuntimeContainerSection::
							Equipment;
				}).Num();
		if (!BossCorpse
			|| BossCorpseCount != 1
			|| BossCorpse->GetLootSourceId()
				!= BossLootSourceId
			|| !BossPlan.IsSuccess()
			|| BossPlan.Trace.bFallbackUsed
			|| BossPlan.Trace.bJackpotHit
			|| BossPlan.Trace.bRareExtremeHit
			|| BossPlan.PlannedStacks.Num() < 3
			|| BossPlan.PlannedStacks.Num() > 6
			|| EquipmentCount < 1
			|| !AffixedWeapon
			|| AfterBoss.ShopStock
				!= BeforeBoss.ShopStock
			|| !RewardGenerationSession.IsProcessed(
				ActiveRunId,
				Boss->StableSourceRoleId)
			|| GetRewardAffixPityState(ActiveRunId)
				!= BossPlan.Trace.PityStateOut
			|| !BossCorpse->GetWorldLabelText().
				Contains(TEXT("BOSS REWARD")))
		{
			Fail(TEXT(
				"Stage A: Heavy death did not produce the one natural Boss reward source without fallback or ShopStock refresh"));
			return;
		}
		const int32 CorpseCountBeforeDuplicate =
			Corpses.Num();
		const auto Duplicate =
			HandleEnemyDeath(
				Fdemo_mapFixedLootTableIds::
					CorpseMainMeleeHeavy,
				BossLootSourceId,
				BossDeathLocation,
				Heavy);
		if (Duplicate.bSuccess
			|| Corpses.Num() != CorpseCountBeforeDuplicate)
		{
			Fail(TEXT(
				"Stage A: duplicate Heavy death created a second Boss source"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P7_BOSS_REWARD_STAGE_A: PASS run=%s attempt=%u stacks=%d equipment=%d generation=%d."),
			*ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			ActiveRunId.D,
			BossPlan.PlannedStacks.Num(),
			EquipmentCount,
			BeforeBoss.ShopStock.Generation);

		if (!OpenThroughProductInput(BossCorpse))
		{
			Fail(TEXT(
				"Stage B: real G input did not open the Boss Corpse"));
			return;
		}
		const auto BossView =
			Fdemo_mapSearchContainerPresenter::Build(
				BossCorpse->GetContainerSnapshot());
		if (BossView.Header != TEXT("BOSS REWARD"))
		{
			Fail(TEXT(
				"Stage B: Boss search UI did not expose the BOSS REWARD label"));
			return;
		}
		Fdemo_mapRuntimeContainerEntrySnapshot Identified;
		if (!SearchIdentify(
				BossCorpse,
				AffixedWeapon->Section,
				AffixedWeapon->SlotIndex,
				Identified))
		{
			Fail(TEXT(
				"Stage B: natural affixed Boss Equipment could not be identified"));
			return;
		}
		const FGuid BossItemId =
			Identified.ItemInstanceId;
		const Fdemo_mapRewardAffixSet BossAffix =
			Identified.AffixSet;
		const Fdemo_mapItemDefinition* BossDefinition =
			Fdemo_mapItemDefinitions::Find(
				Identified.DefinitionId);
		if (!BossDefinition
			|| BossDefinition->CategoryId
				!= Fdemo_mapItemIds::WeaponCategory
			|| Identified.RewardSourceRoleId
				!= Boss->StableSourceRoleId
			|| BossAffix.Affixes.IsEmpty()
			|| !SearchContainerWidget->
				AutomationClickEntry(
					AffixedWeapon->Section,
					AffixedWeapon->SlotIndex)
			|| !SearchContainerWidget->GetLastResult().
				bSuccess
			|| SearchContainerWidget->GetLastResult().
				ItemInstanceId != BossItemId)
		{
			Fail(TEXT(
				"Stage B: same-GUID Take or Boss provenance/Affix metadata failed"));
			return;
		}
		const Fdemo_mapItemInstance* Taken =
			Items->GetAuthority().FindInstance(BossItemId);
		if (!Taken
			|| Taken->AffixSet != BossAffix
			|| Taken->RewardSourceRoleId
				!= Boss->StableSourceRoleId
			|| Items->GetAuthority().
				FindInventorySlot(BossItemId) == INDEX_NONE
			|| !SearchContainerWidget->
				AutomationClickClose()
			|| bSearchContainerOpen
			|| !GetDemoController()->
				IsGameplayInputAllowed())
		{
			Fail(TEXT(
				"Stage B: Boss Item transfer, close, or immediate input restoration failed"));
			return;
		}
		if (!OpenThroughProductInput(BossCorpse)
			|| BossCorpse->GetProjectionResult().Trace.
				EffectiveSeed
					!= BossPlan.Trace.EffectiveSeed
			|| !SearchContainerWidget->
				AutomationClickClose())
		{
			Fail(TEXT(
				"Stage B: reopen rerolled the Boss source or failed to close"));
			return;
		}
		Udemo_mapAttributeComponent* Attributes =
			PlayerPawn.IsValid()
				? PlayerPawn->FindComponentByClass<
					Udemo_mapAttributeComponent>()
				: nullptr;
		float AttackBefore = 0.0f;
		float AttackAfter = 0.0f;
		if (!Attributes
			|| !Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackBefore)
			|| !Items->Equip(
				BossItemId,
				Fdemo_mapItemIds::WeaponSlot).bSuccess
			|| Items->GetAuthority().
				GetEquippedInstance(
					Fdemo_mapItemIds::WeaponSlot)
					!= BossItemId
			|| !Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackAfter)
			|| AttackAfter <= AttackBefore)
		{
			Fail(TEXT(
				"Stage B: Boss Equipment did not equip by the existing authority or affect gameplay"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P7_BOSS_REWARD_STAGE_B: PASS guid=%s definition=%s affix=%s attack_before=%.2f attack_after=%.2f same_guid=1 reopen=1 input_restore=1."),
			*BossItemId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*BossDefinition->DefinitionId.ToString(),
			*Fdemo_mapRewardAffixPolicyRegistry::
				BuildDisplayLabel(BossAffix),
			AttackBefore,
			AttackAfter);

		DestroyRuntimeContainers(
			TEXT("RewardBossSourceBeforeExtraction"));
		Fdemo_mapSettlementSummary Summary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				Summary).bSuccess)
		{
			Fail(TEXT(
				"Stage C: runtime Extraction request failed"));
			return;
		}
		const auto Settlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(Summary);
		if (!Settlement.IsDurablySettled())
		{
			Fail(TEXT(
				"Stage C: Profile settlement was not durable"));
			return;
		}
		const Fdemo_mapProfileSessionSnapshot AfterSettlement =
			RewardGenerationProfileSession->GetSnapshot();
		if (AfterSettlement.ShopStock.Generation
				!= BeforeBoss.ShopStock.Generation + 1
			|| AfterSettlement.ShopStock.ShopStockEventId
				== BeforeBoss.ShopStock.ShopStockEventId)
		{
			Fail(TEXT(
				"Stage C: terminal ShopStock refresh was not exactly +1"));
			return;
		}
		Fdemo_mapProfileRepository Repository;
		const auto Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const auto Reloaded =
			Repository.LoadExistingProfile(Storage);
		const auto* Persisted = Reloaded.IsSuccess()
			? Reloaded.Profile.PermanentStash.
				FindByPredicate(
					[BossItemId](const auto& Item)
					{
						return Item.ItemInstanceId
							== BossItemId;
					})
			: nullptr;
		const auto Preparation =
			Fdemo_mapProfilePreparationPresenter::
				BuildViewState(
					RewardGenerationProfileSession->
						GetPreparationSnapshot());
		const auto* Quote =
			Preparation.OrderedPermanentStashRows.
				FindByPredicate(
					[BossItemId](const auto& Row)
					{
						return Row.ItemInstanceId
							== BossItemId;
					});
		if (!Persisted
			|| Persisted->RewardSourceRoleId
				!= Boss->StableSourceRoleId
			|| Persisted->AffixSet != BossAffix
			|| !Quote
			|| Quote->TotalSellPrice <= 0)
		{
			Fail(TEXT(
				"Stage C: Boss GUID/provenance/Affix or Preparation quote did not survive reload"));
			return;
		}
		const int64 SellQuote =
			Quote->TotalSellPrice;
		const int64 BalanceBeforeSale =
			AfterSettlement.PersistentSpiritStones;
		Fdemo_mapProfileTradeIntent Sell;
		Sell.Kind = Edemo_mapProfileTradeKind::Sell;
		Sell.ExpectedProfileId =
			AfterSettlement.ProfileId;
		Sell.ExpectedSaveGeneration =
			AfterSettlement.SaveGeneration;
		Sell.ItemInstanceId = BossItemId;
		const auto Sale =
			RewardGenerationProfileSession->
				SubmitTradeIntent(Sell);
		const auto AfterSaleReload =
			Repository.LoadExistingProfile(Storage);
		const Fdemo_mapProfileSessionSnapshot AfterSale =
			RewardGenerationProfileSession->GetSnapshot();
		if (!Sale.IsCommitted()
			|| Sale.TotalPrice != SellQuote
			|| AfterSale.PersistentSpiritStones
				- BalanceBeforeSale != SellQuote
			|| !AfterSaleReload.IsSuccess()
			|| AfterSaleReload.Profile.PermanentStash.
				ContainsByPredicate(
					[BossItemId](const auto& Item)
					{
						return Item.ItemInstanceId
							== BossItemId;
					}))
		{
			Fail(TEXT(
				"Stage C: Boss reward Sell or durable balance credit failed"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P7_BOSS_REWARD_STAGE_C: PASS settlement=1 reload=1 preparation=1 sell=1 quote=%lld shop_generation=%d."),
			SellQuote,
			AfterSettlement.ShopStock.Generation);

		Items->ResetForAutomation();
		Items->BeginWorld(GetWorld());
		const auto NextRun =
			RewardGenerationProfileSession->
				StartPreparedRun();
		const int32 NextRunPityBeforeWorld =
			NextRun.IsRunActive()
				? GetRewardAffixPityState(
					NextRun.Snapshot.ActiveRunId)
				: INDEX_NONE;
		if (!NextRun.IsRunActive()
			|| NextRun.Snapshot.ShopStock.Generation
				!= AfterSettlement.ShopStock.Generation
			|| NextRunPityBeforeWorld != 0
			|| !InitializeWorldContent()
			|| !CanGenerateRewardSource(
				NextRun.Snapshot.ActiveRunId,
				Boss->StableSourceRoleId))
		{
			Fail(TEXT(
				"Stage D: next ActiveRun did not reset Boss/pity ledgers without ShopStock BeginRun refresh"));
			return;
		}
		Ademo_mapHeavyEnemyCharacter* NextHeavy = nullptr;
		for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(
			GetWorld()); It; ++It)
		{
			if (It->GetEncounterIdentity().EncounterId
				== Fdemo_mapEnemyEncounterIds::
					MainMeleeHeavy)
			{
				NextHeavy = *It;
				break;
			}
		}
		const FGuid NextLootSourceId =
			NextHeavy
				? NextHeavy->GetLootSourceId()
				: FGuid();
		if (!NextHeavy)
		{
			Fail(TEXT(
				"Stage D: next run Heavy is missing"));
			return;
		}
		NextHeavy->SetCombatSuppressed(true);
		UGameplayStatics::ApplyDamage(
			NextHeavy,
			100000.0f,
			GetDemoController(),
			PlayerPawn.Get(),
			nullptr);
		Ademo_mapCorpseContainerActor* NextBossCorpse =
			nullptr;
		for (const auto& Candidate : Corpses)
		{
			if (Candidate.IsValid()
				&& Candidate->GetRewardProjectionId()
					== Boss->ProjectionId)
			{
				NextBossCorpse = Candidate.Get();
				break;
			}
		}
		if (!NextBossCorpse
			|| NextLootSourceId == BossLootSourceId
			|| !RewardGenerationSession.IsProcessed(
				NextRun.Snapshot.ActiveRunId,
				Boss->StableSourceRoleId))
		{
			Fail(TEXT(
				"Stage D: next run did not materialize one fresh Boss source"));
			return;
		}
		DestroyRuntimeContainers(
			TEXT("RewardBossSourceNextRunCleanup"));
		Fdemo_mapSettlementSummary NextSummary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Abandon,
				NextSummary).bSuccess
			|| !RewardGenerationProfileSession->
				CommitRuntimeSettlement(NextSummary).
					IsDurablySettled())
		{
			Fail(TEXT(
				"Stage D: next-run cleanup settlement failed"));
			return;
		}
		const auto FinalProfile =
			RewardGenerationProfileSession->GetSnapshot();
		if (FinalProfile.ShopStock.Generation
				!= AfterSettlement.ShopStock.Generation + 1)
		{
			Fail(TEXT(
				"Stage D: next terminal did not refresh ShopStock exactly once"));
			return;
		}

		const FString AffixLabel =
			Fdemo_mapRewardAffixPolicyRegistry::
				BuildDisplayLabel(BossAffix);
		const bool bSnapshotsWritten =
			WriteSnapshot(
				TEXT("BossProjection.json"),
				FString::Printf(
					TEXT("{\n  \"task_id\": \"Dev.D.UE.0.0.6.P7.0.r0\",\n  \"projection_id\": \"%s\",\n  \"source_role_id\": \"%s\",\n  \"encounter_id\": \"%s\",\n  \"base_source_value\": %lld,\n  \"budget_profile\": \"%s\",\n  \"min_stacks\": %d,\n  \"max_stacks\": %d,\n  \"required_equipment\": %d,\n  \"fallback_enabled\": false,\n  \"display_label\": \"BOSS REWARD\",\n  \"run_id\": \"%s\",\n  \"bounded_attempt\": %u,\n  \"planned_stacks\": %d,\n  \"jackpot_hit\": false,\n  \"rare_hit\": false\n}\n"),
					*Boss->ProjectionId.ToString(),
					*Boss->StableSourceRoleId.ToString(),
					*Boss->EncounterId.ToString(),
					Boss->BaseSourceValue,
					*Boss->BudgetProfileId.ToString(),
					Boss->MinGeneratedStacks,
					Boss->MaxGeneratedStacks,
					Boss->RequiredEquipmentCount,
					*ActiveRunId.ToString(
						EGuidFormats::DigitsWithHyphensLower),
					ActiveRunId.D,
					BossPlan.PlannedStacks.Num()))
			&& WriteSnapshot(
				TEXT("BossRewardLedger.normal.json"),
				FString::Printf(
					TEXT("{\n  \"run_id\": \"%s\",\n  \"source_role_id\": \"%s\",\n  \"source_committed_after_materialize\": true,\n  \"duplicate_source_rejected\": true,\n  \"pity_state_in\": %d,\n  \"pity_state_out\": %d,\n  \"pity_profile_persisted\": false,\n  \"next_run_reset\": true\n}\n"),
					*ActiveRunId.ToString(
						EGuidFormats::DigitsWithHyphensLower),
					*Boss->StableSourceRoleId.ToString(),
					BossPlan.Trace.PityStateIn,
					BossPlan.Trace.PityStateOut))
			&& WriteSnapshot(
				TEXT("BossRewardItemInstances.json"),
				FString::Printf(
					TEXT("{\n  \"item_instance_id\": \"%s\",\n  \"definition_id\": \"%s\",\n  \"source_role_id\": \"%s\",\n  \"affix_label\": \"%s\",\n  \"same_guid_take\": true,\n  \"equipped\": true,\n  \"gameplay_effect\": true,\n  \"settled\": true,\n  \"profile_reloaded\": true,\n  \"sold\": true,\n  \"sell_quote\": %lld\n}\n"),
					*BossItemId.ToString(
						EGuidFormats::DigitsWithHyphensLower),
					*BossDefinition->DefinitionId.ToString(),
					*Boss->StableSourceRoleId.ToString(),
					*AffixLabel,
					SellQuote))
			&& WriteSnapshot(
				TEXT("BossRewardLifecycle.json"),
				FString::Printf(
					TEXT("{\n  \"stage_a_death_corpse\": \"PASS\",\n  \"stage_b_search_take_equip\": \"PASS\",\n  \"stage_c_settlement_reload_sell\": \"PASS\",\n  \"stage_d_next_run_cleanup\": \"PASS\",\n  \"shop_generation_before\": %d,\n  \"shop_generation_after_first_terminal\": %d,\n  \"shop_generation_after_next_terminal\": %d,\n  \"boss_creation_refresh_delta\": 0,\n  \"first_terminal_refresh_delta\": 1,\n  \"next_terminal_refresh_delta\": 1,\n  \"residual_runtime_objects\": 0\n}\n"),
					BeforeBoss.ShopStock.Generation,
					AfterSettlement.ShopStock.Generation,
					FinalProfile.ShopStock.Generation));
		if (!bSnapshotsWritten)
		{
			Fail(TEXT(
				"Stage D: required P7 machine snapshots could not be written"));
			return;
		}
		GetWorldTimerManager().ClearTimer(AutomationTimer);
		Items->TeardownWorld(GetWorld());
		const int32 ResidualObjects =
			Items->GetWorldActorCount()
				+ Chests.Num()
				+ Corpses.Num()
				+ EnemyActors.Num();
		if (ResidualObjects != 0)
		{
			Fail(TEXT(
				"Stage D: task-owned Runtime objects remain"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P7_BOSS_REWARD_STAGE_D: PASS next_run=%s next_source=%s shop_generation=%d residual_objects=0."),
			*NextRun.Snapshot.ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*NextLootSourceId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			FinalProfile.ShopStock.Generation);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P7_BOSS_REWARD_PRODUCT_EVIDENCE run=%s bounded_limit=500000 actual_attempt=%u boss_source_count=1 duplicate_rejected=1 stacks=%d equipment=%d fallback=0 jackpot_miss=1 rare_miss=1 natural_affix=1 guid=%s same_guid=1 reopen_no_reroll=1 equip=1 gameplay_effect=1 settlement=1 profile_reload=1 preparation=1 sell=1 boss_creation_shop_refresh=0 terminal_shop_refresh=1 next_run_source=1 next_run_terminal_refresh=1 residual_objects=0."),
			*ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			ActiveRunId.D,
			BossPlan.PlannedStacks.Num(),
			EquipmentCount,
			*BossItemId.ToString(
				EGuidFormats::DigitsWithHyphensLower));
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P7_BOSS_REWARD_PRODUCT_SMOKE: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	void Ademo_mapV3ProgressionManager::RunRewardShopStockAutomation()
	{
		auto Cleanup = [this](Edemo_mapRunEndReason Reason)
		{
			CloseSearchContainer(
				TEXT("RewardShopStockSmokeCleanup"), true);
			DestroyRuntimeContainers(
				TEXT("RewardShopStockSmokeCleanup"));
			if (Items.IsValid()
				&& Items->GetRunState()
					== Edemo_mapRunState::Active)
			{
				Fdemo_mapSettlementSummary Summary;
				if (Items->RequestSettlement(Reason, Summary).
						bSuccess
					&& RewardGenerationProfileSession.IsValid())
				{
					RewardGenerationProfileSession->
						CommitRuntimeSettlement(Summary);
				}
			}
			if (Items.IsValid())
			{
				Items->TeardownWorld(GetWorld());
			}
		};
		auto Fail = [this, &Cleanup](const FString& Stage)
		{
			GetWorldTimerManager().ClearTimer(AutomationTimer);
			Cleanup(Edemo_mapRunEndReason::Abandon);
			FString Clean = Stage;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P6_REWARD_SHOP_STOCK_PRODUCT_SMOKE: FAIL: %s"),
				*Clean);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		auto OpenThroughProductInput = [this](
			Ademo_mapSearchContainerActor* Container) -> bool
		{
			if (!Container || !PlayerPawn.IsValid()
				|| !GetDemoController())
			{
				return false;
			}
			MovePawnNear(Container);
			GetDemoController()->SetAutomationAimDirection(
				Container->GetActorLocation()
					- PlayerPawn->GetActorLocation());
			SetFocusedActor(Container);
			if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G))
			{
				return false;
			}
			if (Container->IsContainerOpening()
				&& !Container->CompleteActionForAutomation().
					bSuccess)
			{
				return false;
			}
			return GetDemoController()->
					DispatchAutomationKeyReleased(EKeys::G)
				&& Container->IsContainerOpened()
				&& bSearchContainerOpen
				&& ActiveSearchContainer.Get() == Container
				&& SearchContainerWidget;
		};
		auto FindEntry = [](
			const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex)
			-> const Fdemo_mapRuntimeContainerEntrySnapshot*
		{
			const auto* FoundSection =
				Snapshot.Sections.FindByPredicate(
					[Section](const auto& Candidate)
					{
						return Candidate.Section == Section;
					});
			return FoundSection
				? FoundSection->OrderedOccupiedEntries.
					FindByPredicate(
						[SlotIndex](const auto& Candidate)
						{
							return Candidate.SlotIndex
								== SlotIndex;
						})
				: nullptr;
		};
		auto IdentifyAndTake = [this, &FindEntry](
			Ademo_mapSearchContainerActor* Container,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex,
			FGuid& OutItemId) -> bool
		{
			const auto Before =
				Container->GetContainerSnapshot();
			const auto* Entry =
				FindEntry(Before, Section, SlotIndex);
			if (!Entry)
			{
				return false;
			}
			if (Entry->State
				== Edemo_mapRuntimeContainerEntryState::Hidden)
			{
				if (!SearchContainerWidget->
					AutomationClickEntry(Section, SlotIndex))
				{
					return false;
				}
				if (Container->IsContainerSearching()
					&& !Container->CompleteActionForAutomation().
						bSuccess)
				{
					return false;
				}
			}
			const auto Identified =
				Container->GetContainerSnapshot();
			Entry = FindEntry(Identified, Section, SlotIndex);
			if (!Entry
				|| Entry->State
					!= Edemo_mapRuntimeContainerEntryState::
						Identified)
			{
				return false;
			}
			OutItemId = Entry->ItemInstanceId;
			return SearchContainerWidget->
					AutomationClickEntry(Section, SlotIndex)
				&& SearchContainerWidget->GetLastResult().bSuccess
				&& SearchContainerWidget->GetLastResult().
					ItemInstanceId == OutItemId
				&& Items->GetAuthority().FindInventorySlot(
					OutItemId) != INDEX_NONE;
		};
		auto BuildBuyIntent = [](
			const Fdemo_mapProfileSessionSnapshot& Snapshot,
			const Fdemo_mapPersistentShopStockEntry& Entry)
		{
			Fdemo_mapProfileTradeIntent Intent;
			Intent.Kind = Edemo_mapProfileTradeKind::Buy;
			Intent.ExpectedProfileId = Snapshot.ProfileId;
			Intent.ExpectedSaveGeneration =
				Snapshot.SaveGeneration;
			Intent.ItemInstanceId =
				Entry.State
						== Edemo_mapPersistentShopStockEntryState::
							Available
					? Entry.Item.ItemInstanceId
					: Entry.SoldItemInstanceId;
			Intent.ShopStockPolicyId =
				Snapshot.ShopStock.PolicyId;
			Intent.ShopStockGeneration =
				Snapshot.ShopStock.Generation;
			Intent.ShopStockEventId =
				Snapshot.ShopStock.ShopStockEventId;
			Intent.ShopSlotId = Entry.SlotId;
			Intent.ExpectedBuyValue =
				Entry.State
						== Edemo_mapPersistentShopStockEntryState::
							Available
					? Entry.QuotedBuyValue
					: 1;
			return Intent;
		};
		if (!Items.IsValid()
			|| !RewardGenerationProfileSession.IsValid()
			|| RewardShopStockGeneration0.Generation != 0
			|| RewardShopStockGeneration0.Entries.Num() != 12
			|| !RewardShopStockInitialRunId.IsValid())
		{
			Fail(TEXT("StageA isolated Profile, Generation 0, ActiveRun, or stock precondition is unavailable"));
			return;
		}

		Ademo_mapLootChest* FundingChest = nullptr;
		for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate :
			Chests)
		{
			if (Candidate.IsValid()
				&& Candidate->GetLastProjectionResult().
					Trace.ProjectionId
					== Fdemo_mapRewardProjectionIds::
						ChestSideHighValue)
			{
				FundingChest = Candidate.Get();
				break;
			}
		}
		if (!FundingChest
			|| !FundingChest->GetLastProjectionResult().
				IsSuccess()
			|| !OpenThroughProductInput(FundingChest))
		{
			Fail(TEXT("StageB real HighValue Chest could not be opened through product input"));
			return;
		}
		TArray<FGuid> LegitimateLootIds;
		for (const Fdemo_mapRewardPlannedStack& Stack :
			FundingChest->GetLastProjectionResult().
				PlannedStacks)
		{
			if (Items->GetAuthority().GetFreeInventorySlots()
				<= 0)
			{
				break;
			}
			FGuid TakenId;
			if (!IdentifyAndTake(
				FundingChest,
				Stack.Section,
				Stack.SlotIndex,
				TakenId))
			{
				Fail(TEXT("StageB real Search/Take failed for a generated HighValue Chest entry"));
				return;
			}
			LegitimateLootIds.Add(TakenId);
		}
		if (LegitimateLootIds.IsEmpty()
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen)
		{
			Fail(TEXT("StageB no legitimate generated loot was secured or Search UI did not close"));
			return;
		}
		DestroyRuntimeContainers(
			TEXT("RewardShopStockBeforeExtraction"));
		Fdemo_mapSettlementSummary FirstSummary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				FirstSummary).bSuccess)
		{
			Fail(TEXT("StageB real Extraction request failed"));
			return;
		}
		const Fdemo_mapProfileSessionSettlementResult FirstSettlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(FirstSummary);
		const Fdemo_mapProfileSessionSnapshot Generation1 =
			RewardGenerationProfileSession->GetSnapshot();
		if (!FirstSettlement.IsDurablySettled()
			|| Generation1.SessionState
				!= Edemo_mapProfileSessionState::
					ReadyForPreparation
			|| Generation1.ShopStock.Generation != 1
			|| Generation1.ShopStock.Entries.Num() != 12
			|| Generation1.ShopStock.Entries.ContainsByPredicate(
				[](const auto& Entry)
				{
					return Entry.State
						!= Edemo_mapPersistentShopStockEntryState::
							Available;
				}))
		{
			Fail(TEXT("StageB terminal did not create exactly one all-Available Generation 1"));
			return;
		}
		for (const Fdemo_mapPersistentShopStockEntry& OldEntry :
			RewardShopStockGeneration0.Entries)
		{
			if (Generation1.ShopStock.Entries.
				ContainsByPredicate(
					[&OldEntry](const auto& Current)
					{
						return Current.Item.ItemInstanceId
							== OldEntry.Item.ItemInstanceId;
					}))
			{
				Fail(TEXT("StageB Generation 0 unsold GUID survived the terminal replacement"));
				return;
			}
		}
		const Fdemo_mapPersistentShopStockState ReopenOne =
			RewardGenerationProfileSession->GetSnapshot().
				ShopStock;
		const Fdemo_mapPersistentShopStockState ReopenTwo =
			RewardGenerationProfileSession->GetSnapshot().
				ShopStock;
		if (ReopenOne != Generation1.ShopStock
			|| ReopenTwo != Generation1.ShopStock)
		{
			Fail(TEXT("StageB repeated Preparation reads rerolled Generation 1"));
			return;
		}
		if (!CopyRewardShopStockEvidenceProfile(
			RewardGenerationStorageRoot,
			TEXT("ShopStockGeneration1.json")))
		{
			Fail(TEXT("StageB Generation 1 evidence snapshot could not be written"));
			return;
		}
		const int32 PityBeforeShop =
			GetRewardAffixPityState(
				RewardShopStockInitialRunId);

		int64 LegitimateSellTotal = 0;
		for (const FGuid& LootId : LegitimateLootIds)
		{
			const Fdemo_mapProfileSessionSnapshot BeforeSale =
				RewardGenerationProfileSession->GetSnapshot();
			if (!BeforeSale.OrderedPermanentStash.
				ContainsByPredicate(
					[LootId](const auto& Item)
					{
						return Item.ItemInstanceId == LootId;
					}))
			{
				Fail(TEXT("StageB extracted real loot did not enter Permanent Stash"));
				return;
			}
			Fdemo_mapProfileTradeIntent Sell;
			Sell.Kind = Edemo_mapProfileTradeKind::Sell;
			Sell.ExpectedProfileId = BeforeSale.ProfileId;
			Sell.ExpectedSaveGeneration =
				BeforeSale.SaveGeneration;
			Sell.ItemInstanceId = LootId;
			const Fdemo_mapProfileTradeResult Sale =
				RewardGenerationProfileSession->
					SubmitTradeIntent(Sell);
			if (!Sale.IsCommitted()
				|| Sale.BalanceAfter - Sale.BalanceBefore
					!= Sale.TotalPrice)
			{
				Fail(TEXT("StageB legitimate loot Sell transaction was not exact"));
				return;
			}
			LegitimateSellTotal += Sale.TotalPrice;
		}

		const Fdemo_mapProfileSessionSnapshot BeforeBuys =
			RewardGenerationProfileSession->GetSnapshot();
		const Fdemo_mapPersistentShopStockEntry* PillEntry =
			BeforeBuys.ShopStock.Entries.FindByPredicate(
				[](const auto& Entry)
				{
					return Entry.State
							== Edemo_mapPersistentShopStockEntryState::
								Available
						&& Entry.Item.ItemDefinitionId
							== Fdemo_mapItemIds::
								HealingPillLevel1
						&& Entry.QuotedBuyValue == 30;
				});
		const Fdemo_mapPersistentShopStockEntry* EquipmentEntry =
			nullptr;
		for (const Fdemo_mapPersistentShopStockEntry& Entry :
			BeforeBuys.ShopStock.Entries)
		{
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(
					Entry.Item.ItemDefinitionId);
			if (Entry.State
					!= Edemo_mapPersistentShopStockEntryState::
						Available
				|| Entry.Item.AffixSet.Affixes.IsEmpty()
				|| !Definition
				|| (Definition->CategoryId
						!= Fdemo_mapItemIds::WeaponCategory
					&& Definition->CategoryId
						!= Fdemo_mapItemIds::ArmorCategory)
				|| !PillEntry
				|| Entry.QuotedBuyValue
						+ PillEntry->QuotedBuyValue
					> BeforeBuys.PersistentSpiritStones)
			{
				continue;
			}
			if (!EquipmentEntry
				|| Entry.QuotedBuyValue
					< EquipmentEntry->QuotedBuyValue)
			{
				EquipmentEntry = &Entry;
			}
		}
		if (!PillEntry || !EquipmentEntry)
		{
			Fail(FString::Printf(
				TEXT("StageB natural Generation 1 lacks an affordable Affixed Weapon/Robe plus guaranteed Pill; legitimate_balance=%lld"),
				BeforeBuys.PersistentSpiritStones));
			return;
		}
		const FGuid PurchasedEquipmentId =
			EquipmentEntry->Item.ItemInstanceId;
		const FGuid PurchasedPillId =
			PillEntry->Item.ItemInstanceId;
		const FName EquipmentSlotId =
			Fdemo_mapItemDefinitions::Find(
				EquipmentEntry->Item.ItemDefinitionId)->
					EquipmentSlotId;
		const FName EquipmentDefinitionId =
			EquipmentEntry->Item.ItemDefinitionId;
		const Fdemo_mapRewardAffixSet PurchasedAffixes =
			EquipmentEntry->Item.AffixSet;
		const int64 EquipmentBuyQuote =
			EquipmentEntry->QuotedBuyValue;
		const FName EquipmentShopSlot =
			EquipmentEntry->SlotId;
		const FName PillShopSlot = PillEntry->SlotId;

		Fdemo_mapProfileTradeIntent EquipmentBuy =
			BuildBuyIntent(BeforeBuys, *EquipmentEntry);
		const Fdemo_mapProfileTradeResult EquipmentPurchase =
			RewardGenerationProfileSession->
				SubmitTradeIntent(EquipmentBuy);
		if (!EquipmentPurchase.IsCommitted()
			|| EquipmentPurchase.ItemInstanceId
				!= PurchasedEquipmentId
			|| EquipmentPurchase.TotalPrice
				!= EquipmentBuyQuote)
		{
			Fail(TEXT("StageB affixed equipment buy did not move the same GUID at the exact quote"));
			return;
		}
		const Fdemo_mapProfileSessionSnapshot BeforeDoubleClick =
			RewardGenerationProfileSession->GetSnapshot();
		EquipmentBuy.ExpectedSaveGeneration =
			BeforeDoubleClick.SaveGeneration;
		const Fdemo_mapProfileTradeResult DoubleClick =
			RewardGenerationProfileSession->
				SubmitTradeIntent(EquipmentBuy);
		const Fdemo_mapProfileSessionSnapshot AfterDoubleClick =
			RewardGenerationProfileSession->GetSnapshot();
		if (DoubleClick.Status
				!= Edemo_mapProfileTradeStatus::ShopSlotSold
			|| BeforeDoubleClick.SaveGeneration
				!= AfterDoubleClick.SaveGeneration
			|| BeforeDoubleClick.PersistentSpiritStones
				!= AfterDoubleClick.PersistentSpiritStones
			|| BeforeDoubleClick.OrderedPermanentStash
				!= AfterDoubleClick.OrderedPermanentStash
			|| BeforeDoubleClick.ShopStock
				!= AfterDoubleClick.ShopStock)
		{
			Fail(TEXT("StageB second click did not reject the SOLD slot with zero mutation"));
			return;
		}
		const Fdemo_mapPersistentShopStockEntry* CurrentPill =
			AfterDoubleClick.ShopStock.Entries.
				FindByPredicate(
					[PillShopSlot](const auto& Entry)
					{
						return Entry.SlotId == PillShopSlot;
					});
		if (!CurrentPill)
		{
			Fail(TEXT("StageB guaranteed Pill slot disappeared before purchase"));
			return;
		}
		const Fdemo_mapProfileTradeResult PillPurchase =
			RewardGenerationProfileSession->
				SubmitTradeIntent(
					BuildBuyIntent(
						AfterDoubleClick,
						*CurrentPill));
		const Fdemo_mapProfileSessionSnapshot AfterBuys =
			RewardGenerationProfileSession->GetSnapshot();
		const auto* SoldEquipment =
			AfterBuys.ShopStock.Entries.FindByPredicate(
				[EquipmentShopSlot](const auto& Entry)
				{
					return Entry.SlotId
						== EquipmentShopSlot;
				});
		const auto* SoldPill =
			AfterBuys.ShopStock.Entries.FindByPredicate(
				[PillShopSlot](const auto& Entry)
				{
					return Entry.SlotId == PillShopSlot;
				});
		if (!PillPurchase.IsCommitted()
			|| PillPurchase.ItemInstanceId != PurchasedPillId
			|| PillPurchase.TotalPrice != 30
			|| BeforeBuys.PersistentSpiritStones
					- AfterBuys.PersistentSpiritStones
				!= EquipmentBuyQuote + 30
			|| !SoldEquipment || !SoldPill
			|| SoldEquipment->State
				!= Edemo_mapPersistentShopStockEntryState::Sold
			|| SoldPill->State
				!= Edemo_mapPersistentShopStockEntryState::Sold
			|| SoldEquipment->SoldItemInstanceId
				!= PurchasedEquipmentId
			|| SoldPill->SoldItemInstanceId
				!= PurchasedPillId
			|| GetRewardAffixPityState(
					RewardShopStockInitialRunId)
				!= PityBeforeShop)
		{
			Fail(TEXT("StageB Pill/equipment debit, SOLD tombstone, or Shop/Pity isolation failed"));
			return;
		}
		if (!CopyRewardShopStockEvidenceProfile(
			RewardGenerationStorageRoot,
			TEXT("ShopStockPurchaseTransaction.json")))
		{
			Fail(TEXT("StageB purchase evidence snapshot could not be written"));
			return;
		}

		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const Fdemo_mapProfileLoadResult BuyReload =
			Repository.LoadExistingProfile(Storage);
		const auto* ReloadedEquipment =
			BuyReload.IsSuccess()
				? BuyReload.Profile.PermanentStash.
					FindByPredicate(
						[PurchasedEquipmentId](
							const auto& Item)
						{
							return Item.ItemInstanceId
								== PurchasedEquipmentId;
						})
				: nullptr;
		if (!ReloadedEquipment
			|| ReloadedEquipment->AffixSet
				!= PurchasedAffixes
			|| BuyReload.Profile.ShopStock
				!= AfterBuys.ShopStock
			|| BuyReload.Profile.PersistentSpiritStones
				!= AfterBuys.PersistentSpiritStones)
		{
			Fail(TEXT("StageB reload did not preserve SOLD, balance, GUID, or AffixSet"));
			return;
		}

		const Fdemo_mapProfilePreparationSelectionResult Selection =
			RewardGenerationProfileSession->
				SetPreparationEquipment(
					EquipmentSlotId,
					PurchasedEquipmentId);
		if (!Selection.IsAccepted())
		{
			Fail(TEXT("StageC existing Preparation authority rejected purchased equipment"));
			return;
		}
		Items->ResetForAutomation();
		Items->BeginWorld(GetWorld());
		Udemo_mapAttributeComponent* Attributes =
			PlayerPawn.IsValid()
				? PlayerPawn->FindComponentByClass<
					Udemo_mapAttributeComponent>()
				: nullptr;
		const Fdemo_mapItemDefinition* EquipmentDefinition =
			Fdemo_mapItemDefinitions::Find(
				EquipmentDefinitionId);
		if (!Attributes || !EquipmentDefinition)
		{
			Fail(TEXT("StageC equipment Attribute authority is unavailable"));
			return;
		}
		const Fdemo_mapAttributeSnapshot AttributesBefore =
			Attributes->GetFinalSnapshot();
		const int32 ModifierCountBefore =
			Attributes->GetActiveModifierCount();
		const Fdemo_mapProfileSessionBeginResult SecondRun =
			RewardGenerationProfileSession->StartPreparedRun();
		if (!SecondRun.IsRunActive()
			|| SecondRun.Snapshot.ShopStock.Generation != 1
			|| !SecondRun.Snapshot.ActiveRunId.IsValid()
			|| !Items->GetAuthority().FindInstance(
				PurchasedEquipmentId)
			|| Items->GetAuthority().FindInstance(
					PurchasedEquipmentId)->AffixSet
				!= PurchasedAffixes
			|| GetRewardAffixPityState(
					SecondRun.Snapshot.ActiveRunId) != 0)
		{
			Fail(TEXT("StageC BeginRun changed Shop generation, GUID, Affix, or fresh Run pity"));
			return;
		}

		const Fdemo_mapEquipmentEffectResolution BaseEffects =
			Fdemo_mapEquipmentEffectResolver::Resolve(
				*EquipmentDefinition,
				EquipmentSlotId);
		const Fdemo_mapEquipmentEffectResolution AffixEffects =
			Fdemo_mapEquipmentEffectResolver::ResolveAffixes(
				*EquipmentDefinition,
				PurchasedAffixes);
		if (!BaseEffects.bSuccess
			|| !AffixEffects.bSuccess
			|| AffixEffects.Modifiers.IsEmpty()
			|| (Items->GetAuthority().GetEquippedInstance(
					EquipmentSlotId) != PurchasedEquipmentId
				&& !Items->Equip(
					PurchasedEquipmentId,
					EquipmentSlotId).bSuccess))
		{
			Fail(TEXT("StageC existing Equip path rejected base or natural Affix effects"));
			return;
		}
		const Fdemo_mapAttributeSnapshot AttributesEquipped =
			Attributes->GetFinalSnapshot();
		bool bAffixAttributeChanged = false;
		for (const Fdemo_mapModifierSpec& Modifier :
			AffixEffects.Modifiers)
		{
			const float Before =
				AttributesBefore.Values.FindRef(
					Modifier.AttributeId);
			const float After =
				AttributesEquipped.Values.FindRef(
					Modifier.AttributeId);
			bAffixAttributeChanged |=
				!FMath::IsNearlyEqual(Before, After);
		}
		if (!bAffixAttributeChanged
			|| Attributes->GetActiveModifierCount()
				!= ModifierCountBefore
					+ BaseEffects.Modifiers.Num()
					+ AffixEffects.Modifiers.Num())
		{
			Fail(TEXT("StageC natural Affix did not apply exactly once"));
			return;
		}

		bool bGameplayEffectObserved = false;
		if (EquipmentDefinition->CategoryId
			== Fdemo_mapItemIds::WeaponCategory)
		{
			Ademo_mapTrainingTarget* Target = nullptr;
			TActorIterator<Ademo_mapTrainingTarget> TargetIt(
				GetWorld());
			if (TargetIt)
			{
				Target = *TargetIt;
			}
			const int32 HealthBefore =
				Target ? Target->GetHealth() : INDEX_NONE;
			if (Target
				&& MovePawnNear(Target, 120.0f)
				&& PlayerPawn.IsValid())
			{
				PlayerPawn->SetActorRotation(
					(Target->GetActorLocation()
						- PlayerPawn->GetActorLocation()).
						Rotation());
				bGameplayEffectObserved =
					PressBoundKey(EKeys::LeftMouseButton)
					&& (Target->WasDestroyedByDamage()
						|| Target->GetHealth()
							< HealthBefore);
			}
		}
		else
		{
			bGameplayEffectObserved =
				!FMath::IsNearlyEqual(
					AttributesBefore.Values.FindRef(
						Fdemo_mapAttributeIds::MaxHealth),
					AttributesEquipped.Values.FindRef(
						Fdemo_mapAttributeIds::MaxHealth))
				|| !FMath::IsNearlyEqual(
					AttributesBefore.Values.FindRef(
						Fdemo_mapAttributeIds::
							FlatDamageReduction),
					AttributesEquipped.Values.FindRef(
						Fdemo_mapAttributeIds::
							FlatDamageReduction));
		}
		if (!bGameplayEffectObserved
			|| !Items->Unequip(EquipmentSlotId).bSuccess)
		{
			Fail(TEXT("StageC real gameplay effect or Unequip path failed"));
			return;
		}
		const Fdemo_mapAttributeSnapshot AttributesUnequipped =
			Attributes->GetFinalSnapshot();
		auto SameAttributeValues = [](
			const TMap<FName, float>& A,
			const TMap<FName, float>& B)
		{
			if (A.Num() != B.Num())
			{
				return false;
			}
			for (const TPair<FName, float>& Pair : A)
			{
				const float* Other = B.Find(Pair.Key);
				if (!Other
					|| !FMath::IsNearlyEqual(
						Pair.Value, *Other))
				{
					return false;
				}
			}
			return true;
		};
		if (!SameAttributeValues(
				AttributesUnequipped.Values,
				AttributesBefore.Values)
			|| Attributes->GetActiveModifierCount()
				!= ModifierCountBefore
			|| !Items->Equip(
				PurchasedEquipmentId,
				EquipmentSlotId).bSuccess
			|| !SameAttributeValues(
				Attributes->GetFinalSnapshot().Values,
				AttributesEquipped.Values)
			|| Attributes->GetActiveModifierCount()
				!= ModifierCountBefore
					+ BaseEffects.Modifiers.Num()
					+ AffixEffects.Modifiers.Num())
		{
			Fail(TEXT("StageC Unequip/re-equip doubled or lost instance effects"));
			return;
		}

		Fdemo_mapSettlementSummary SecondSummary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				SecondSummary).bSuccess)
		{
			Fail(TEXT("StageC second real Extraction request failed"));
			return;
		}
		const Fdemo_mapProfileSessionSettlementResult SecondSettlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(SecondSummary);
		const Fdemo_mapProfileSessionSnapshot Generation2 =
			RewardGenerationProfileSession->GetSnapshot();
		if (!SecondSettlement.IsDurablySettled()
			|| Generation2.ShopStock.Generation != 2
			|| Generation2.ShopStock.Entries.Num() != 12
			|| Generation2.ShopStock.Entries.ContainsByPredicate(
				[](const auto& Entry)
				{
					return Entry.State
						!= Edemo_mapPersistentShopStockEntryState::
							Available;
				})
			|| !Generation2.OrderedPermanentStash.
				ContainsByPredicate(
					[PurchasedEquipmentId,
					 PurchasedAffixes](const auto& Item)
					{
						return Item.ItemInstanceId
								== PurchasedEquipmentId
							&& Item.AffixSet
								== PurchasedAffixes;
					})
			|| !Generation2.OrderedPermanentStash.
				ContainsByPredicate(
					[PurchasedPillId](const auto& Item)
					{
						return Item.ItemInstanceId
							== PurchasedPillId;
					}))
		{
			Fail(TEXT("StageC terminal did not create Generation 2 or persist both purchases"));
			return;
		}
		for (const Fdemo_mapPersistentShopStockEntry& Previous :
			AfterBuys.ShopStock.Entries)
		{
			if (Previous.State
					== Edemo_mapPersistentShopStockEntryState::
						Available
				&& Generation2.ShopStock.Entries.
					ContainsByPredicate(
						[&Previous](const auto& Current)
						{
							return Current.Item.ItemInstanceId
								== Previous.Item.ItemInstanceId;
						}))
			{
				Fail(TEXT("StageC unsold Generation 1 GUID survived replacement"));
				return;
			}
		}
		TSet<FGuid> AllIds;
		for (const Fdemo_mapPersistentItemRecord& Item :
			Generation2.OrderedPermanentStash)
		{
			if (!Item.ItemInstanceId.IsValid()
				|| AllIds.Contains(Item.ItemInstanceId))
			{
				Fail(TEXT("StageC Permanent Stash contains an invalid or duplicate GUID"));
				return;
			}
			AllIds.Add(Item.ItemInstanceId);
		}
		for (const Fdemo_mapPersistentShopStockEntry& Entry :
			Generation2.ShopStock.Entries)
		{
			if (!Entry.Item.ItemInstanceId.IsValid()
				|| AllIds.Contains(Entry.Item.ItemInstanceId))
			{
				Fail(TEXT("StageC ShopStock duplicates a player-owned GUID"));
				return;
			}
			AllIds.Add(Entry.Item.ItemInstanceId);
		}
		const Fdemo_mapProfileLoadResult FinalReload =
			Repository.LoadExistingProfile(Storage);
		const Fdemo_mapPersistentShopStockState FinalReopen =
			RewardGenerationProfileSession->GetSnapshot().
				ShopStock;
		if (!FinalReload.IsSuccess()
			|| FinalReload.Profile.ShopStock
				!= Generation2.ShopStock
			|| FinalReload.Profile.PersistentSpiritStones
				!= Generation2.PersistentSpiritStones
			|| FinalReopen != Generation2.ShopStock
			|| GetRewardAffixPityState(
					RewardShopStockInitialRunId)
				!= PityBeforeShop)
		{
			Fail(TEXT("StageC final reload/reopen, balance, or Shop/Pity isolation failed"));
			return;
		}
		if (!CopyRewardShopStockEvidenceProfile(
			RewardGenerationStorageRoot,
			TEXT("ShopStockGeneration2.json")))
		{
			Fail(TEXT("StageC Generation 2 evidence snapshot could not be written"));
			return;
		}

		GetWorldTimerManager().ClearTimer(AutomationTimer);
		Items->TeardownWorld(GetWorld());
		const int32 ResidualObjects =
			Items->GetWorldActorCount()
				+ Chests.Num()
				+ Corpses.Num();
		if (ResidualObjects != 0)
		{
			Fail(TEXT("cleanup: task-owned Runtime objects remain"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P6_REWARD_SHOP_STOCK_PRODUCT_EVIDENCE profile=%s run_a=%s run_c=%s generations=0,1,2 slots=12,12,12 generation0_event=%s generation1_event=%s generation2_event=%s legitimate_loot=%d legitimate_sell=%lld balance_final=%lld equipment_slot=%s equipment_definition=%s equipment_guid=%s equipment_affix=%s equipment_buy=%lld pill_guid=%s pill_buy=30 sold_slots=%s,%s same_guid=1 double_click_rejected=1 reload=1 equip_once=1 gameplay_effect=1 re_equip_no_double=1 terminal_refresh_exact=1 shop_pity_isolated=1 residual_objects=0."),
			*Generation2.ProfileId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*RewardShopStockInitialRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*SecondRun.Snapshot.ActiveRunId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*RewardShopStockGeneration0.ShopStockEventId.
				ToString(EGuidFormats::DigitsWithHyphensLower),
			*AfterBuys.ShopStock.ShopStockEventId.
				ToString(EGuidFormats::DigitsWithHyphensLower),
			*Generation2.ShopStock.ShopStockEventId.
				ToString(EGuidFormats::DigitsWithHyphensLower),
			LegitimateLootIds.Num(),
			LegitimateSellTotal,
			Generation2.PersistentSpiritStones,
			*EquipmentSlotId.ToString(),
			*EquipmentDefinitionId.ToString(),
			*PurchasedEquipmentId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*Fdemo_mapRewardAffixPolicyRegistry::
				BuildDisplayLabel(PurchasedAffixes),
			EquipmentBuyQuote,
			*PurchasedPillId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*EquipmentShopSlot.ToString(),
			*PillShopSlot.ToString());
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P6_REWARD_SHOP_STOCK_PRODUCT_SMOKE: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	void Ademo_mapV3ProgressionManager::RunRewardAffixPityAutomation()
	{
		auto Cleanup = [this](Edemo_mapRunEndReason Reason)
		{
			CloseSearchContainer(
				TEXT("RewardAffixPitySmokeCleanup"), true);
			DestroyRuntimeContainers(
				TEXT("RewardAffixPitySmokeCleanup"));
			if (Items.IsValid()
				&& Items->GetRunState() == Edemo_mapRunState::Active)
			{
				Fdemo_mapSettlementSummary Summary;
				if (Items->RequestSettlement(Reason, Summary).bSuccess
					&& RewardGenerationProfileSession.IsValid())
				{
					RewardGenerationProfileSession->
						CommitRuntimeSettlement(Summary);
				}
			}
			if (Items.IsValid())
			{
				Items->TeardownWorld(GetWorld());
			}
		};
		auto Fail = [this, &Cleanup](const FString& Stage)
		{
			GetWorldTimerManager().ClearTimer(AutomationTimer);
			Cleanup(Edemo_mapRunEndReason::Abandon);
			FString Clean = Stage;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P5_REWARD_AFFIX_PITY_PRODUCT_SMOKE: FAIL: %s"),
				*Clean);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		auto OpenThroughProductInput = [this](
			Ademo_mapSearchContainerActor* Container) -> bool
		{
			if (!Container || !PlayerPawn.IsValid()
				|| !GetDemoController())
			{
				return false;
			}
			MovePawnNear(Container);
			GetDemoController()->SetAutomationAimDirection(
				Container->GetActorLocation()
					- PlayerPawn->GetActorLocation());
			SetFocusedActor(Container);
			if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G))
			{
				return false;
			}
			if (Container->IsContainerOpening()
				&& !Container->CompleteActionForAutomation().bSuccess)
			{
				return false;
			}
			return GetDemoController()->
					DispatchAutomationKeyReleased(EKeys::G)
				&& Container->IsContainerOpened()
				&& bSearchContainerOpen
				&& ActiveSearchContainer.Get() == Container
				&& SearchContainerWidget;
		};
		auto FindEntry = [](
			const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex)
			-> const Fdemo_mapRuntimeContainerEntrySnapshot*
		{
			const auto* FoundSection =
				Snapshot.Sections.FindByPredicate(
					[Section](const auto& Candidate)
					{
						return Candidate.Section == Section;
					});
			return FoundSection
				? FoundSection->OrderedOccupiedEntries.
					FindByPredicate(
						[SlotIndex](const auto& Candidate)
						{
							return Candidate.SlotIndex
								== SlotIndex;
						})
				: nullptr;
		};
		auto SearchIdentify = [this, &FindEntry](
			Ademo_mapSearchContainerActor* Container,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex,
			Fdemo_mapRuntimeContainerEntrySnapshot& OutEntry)
			-> bool
		{
			const auto Before = Container->GetContainerSnapshot();
			const auto* Entry =
				FindEntry(Before, Section, SlotIndex);
			if (!Entry)
			{
				return false;
			}
			if (Entry->State
				== Edemo_mapRuntimeContainerEntryState::Hidden)
			{
				if (!SearchContainerWidget->AutomationClickEntry(
					Section, SlotIndex))
				{
					return false;
				}
				if (Container->IsContainerSearching()
					&& !Container->CompleteActionForAutomation().
						bSuccess)
				{
					return false;
				}
			}
			const auto After = Container->GetContainerSnapshot();
			Entry = FindEntry(After, Section, SlotIndex);
			if (!Entry || Entry->State
				!= Edemo_mapRuntimeContainerEntryState::Identified)
			{
				return false;
			}
			OutEntry = *Entry;
			return true;
		};
		if (!Items.IsValid()
			|| !RewardGenerationProfileSession.IsValid()
			|| Chests.Num()
				!= Fdemo_mapRewardFullMapDistribution::
					TotalContainerCount)
		{
			Fail(TEXT("precondition: isolated Profile, ActiveRun, or current real Chest topology is unavailable"));
			return;
		}
		const FGuid ActiveRunId =
			RewardGenerationProfileSession->GetSnapshot().ActiveRunId;
		if (ActiveRunId.A != 0x50350000u
			|| ActiveRunId.B != 0x41464649u
			|| ActiveRunId.C != 0x58504954u)
		{
			Fail(TEXT("bounded-search: selected RunId is not from the P5 finite identity domain"));
			return;
		}

		TArray<FString> AttemptEvidence;
		auto AdvanceAttempts = [&AttemptEvidence](
			const Fdemo_mapRewardSourceProjectionResult& Plan,
			int32& InOutAttemptCount,
			bool& OutGuaranteeInSource)
		{
			OutGuaranteeInSource = false;
			const int32 InitialState =
				InOutAttemptCount < 4 ? InOutAttemptCount : 0;
			if (!Plan.IsSuccess() || Plan.Trace.bFallbackUsed
				|| Plan.Trace.PityStateIn != InitialState)
			{
				return false;
			}
			TArray<const Fdemo_mapRewardPlannedStack*> AffixedWeapons;
			for (const auto& Stack : Plan.PlannedStacks)
			{
				const auto* Definition =
					Fdemo_mapItemDefinitions::Find(
						Stack.DefinitionId);
				if (Definition
					&& Definition->CategoryId
						== Fdemo_mapItemIds::WeaponCategory
					&& !Stack.AffixSet.Affixes.IsEmpty())
				{
					AffixedWeapons.Add(&Stack);
				}
			}
			int32 WeaponIndex = 0;
			for (const auto& Decision :
				Plan.Trace.PityDecisions)
			{
				if (!Decision.bCommitRequired)
				{
					continue;
				}
				if (InOutAttemptCount >= 4
					|| !Decision.bEligibleWeaponAttempt
					|| !AffixedWeapons.IsValidIndex(WeaponIndex))
				{
					return false;
				}
				const bool bFourth = InOutAttemptCount == 3;
				const int32 ExpectedOut =
					bFourth ? 0 : InOutAttemptCount + 1;
				if (Decision.StateIn != InOutAttemptCount
					|| Decision.StateOut != ExpectedOut
					|| Decision.bGuaranteeApplied != bFourth
					|| Decision.bQualifyingTier3 != bFourth)
				{
					return false;
				}
				const auto* Weapon = AffixedWeapons[WeaponIndex++];
				AttemptEvidence.Add(FString::Printf(
					TEXT("attempt%d={source=%s,item=%s,tier_roll=%d,state=%d->%d,guarantee=%d}"),
					InOutAttemptCount + 1,
					*Plan.Trace.StableSourceRoleId.ToString(),
					*Weapon->DefinitionId.ToString(),
					Decision.TierRoll,
					Decision.StateIn,
					Decision.StateOut,
					bFourth ? 1 : 0));
				++InOutAttemptCount;
				OutGuaranteeInSource |= bFourth;
			}
			const int32 ExpectedFinalState =
				InOutAttemptCount == 4 ? 0 : InOutAttemptCount;
			return Plan.Trace.PityStateOut == ExpectedFinalState;
		};
		int32 AttemptCount = 0;
		Ademo_mapSearchContainerActor* GuaranteedContainer = nullptr;
		const Fdemo_mapRewardSourceProjection* GuaranteedProjection =
			nullptr;
		Fdemo_mapRewardSourceProjectionResult ActualPlan;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Ademo_mapLootChest* Chest =
				Chests.IsValidIndex(Index) ? Chests[Index].Get() : nullptr;
			const auto Plan = Chest
				? Chest->GetLastProjectionResult()
				: Fdemo_mapRewardSourceProjectionResult();
			bool bGuaranteeInSource = false;
			if (!AdvanceAttempts(
				Plan,
				AttemptCount,
				bGuaranteeInSource))
			{
				Fail(FString::Printf(
					TEXT("chest-%d: real source did not match the selected finite attempt sequence"),
					Index + 1));
				return;
			}
			if (bGuaranteeInSource)
			{
				if (Plan.Trace.bJackpotHit
					|| Plan.Trace.bRareExtremeHit)
				{
					Fail(TEXT("guaranteed Chest did not naturally miss Jackpot and Rare"));
					return;
				}
				GuaranteedContainer = Chest;
				GuaranteedProjection =
					Fdemo_mapRewardSourceProjectionRegistry::Find(
						Plan.Trace.ProjectionId);
				ActualPlan = Plan;
			}
		}

		TArray<Fdemo_mapRewardSourceProjection> CorpseProjections;
		for (const FName StableRoleId : {
			FName(TEXT("P8.SourceRole.Enemy.Standard.001")),
			Fdemo_mapRewardSourceRoleIds::BossPrototype,
			FName(TEXT("P8.SourceRole.Enemy.Standard.002")),
			FName(TEXT("P8.SourceRole.Enemy.Elite.001")),
			FName(TEXT("P8.SourceRole.Enemy.Elite.002")) })
		{
			const Fdemo_mapFullMapRewardSlot* Slot =
				Fdemo_mapRewardFullMapDistribution::GetSlots().
					FindByPredicate(
						[StableRoleId](const auto& Candidate)
						{
							return Candidate.StableSourceRoleId
								== StableRoleId;
						});
			if (Slot)
			{
				CorpseProjections.Add(
					Fdemo_mapRewardFullMapDistribution::
						BuildProjection(*Slot));
			}
		}
		if (CorpseProjections.Num() != 5
			|| CorpseProjections.ContainsByPredicate(
				[](const auto& Projection)
				{
					return !Projection.IsValid();
				}))
		{
			Fail(TEXT("source-sequence: current five stable enemy Projections are unavailable"));
			return;
		}
		if (AttemptCount < 4)
		{
			TArray<int32> SelectedCorpseSequence;
			TFunction<bool(int32, int32, TArray<int32>&)>
				FindCorpseSequence;
			FindCorpseSequence =
				[&](
					int32 CurrentAttemptCount,
					int32 UsedMask,
					TArray<int32>& OutSequence)
				{
					for (int32 Index = 0; Index < 5; ++Index)
					{
						if ((UsedMask & (1 << Index)) != 0)
						{
							continue;
						}
						const auto* Projection =
							CorpseProjections.IsValidIndex(Index)
								? &CorpseProjections[Index]
								: nullptr;
						const auto Plan = Projection
							? Fdemo_mapRewardSourceProjectionPlanner::Plan(
								*Projection,
								ActiveRunId,
								CurrentAttemptCount)
							: Fdemo_mapRewardSourceProjectionResult();
						int32 NextAttemptCount =
							CurrentAttemptCount;
						bool bGuaranteeInSource = false;
						TArray<FString> SavedEvidence =
							AttemptEvidence;
						if (!AdvanceAttempts(
							Plan,
							NextAttemptCount,
							bGuaranteeInSource)
							|| NextAttemptCount
								== CurrentAttemptCount)
						{
							AttemptEvidence = MoveTemp(SavedEvidence);
							continue;
						}
						AttemptEvidence = MoveTemp(SavedEvidence);
						if (NextAttemptCount == 4)
						{
							if (bGuaranteeInSource
								&& !Plan.Trace.bJackpotHit
								&& !Plan.Trace.bRareExtremeHit)
							{
								OutSequence.Add(Index);
								return true;
							}
							continue;
						}
						TArray<int32> Tail;
						if (FindCorpseSequence(
							NextAttemptCount,
							UsedMask | (1 << Index),
							Tail))
						{
							OutSequence.Add(Index);
							OutSequence.Append(Tail);
							return true;
						}
					}
					return false;
				};
			if (!FindCorpseSequence(
				AttemptCount,
				0,
				SelectedCorpseSequence))
			{
				Fail(TEXT("bounded-search: finite Corpse sequence could not be replayed"));
				return;
			}
			for (int32 SequenceIndex : SelectedCorpseSequence)
			{
				const auto* Projection =
					CorpseProjections.IsValidIndex(SequenceIndex)
						? &CorpseProjections[SequenceIndex]
						: nullptr;
				if (!Projection)
				{
					Fail(TEXT("source-sequence: selected current Projection is unavailable"));
					return;
				}
				AActor* Enemy = nullptr;
				for (TActorIterator<Ademo_mapEnemyCharacter> It(
					GetWorld()); It && !Enemy; ++It)
				{
					if (It->GetEncounterIdentity().EncounterId
						== Projection->EncounterId)
					{
						Enemy = *It;
					}
				}
				for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(
					GetWorld()); It && !Enemy; ++It)
				{
					if (It->GetEncounterIdentity().EncounterId
						== Projection->EncounterId)
					{
						Enemy = *It;
					}
				}
				for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(
					GetWorld()); It && !Enemy; ++It)
				{
					if (It->GetEncounterIdentity().EncounterId
						== Projection->EncounterId)
					{
						Enemy = *It;
					}
				}
				if (!Enemy)
				{
					Fail(TEXT("source-sequence: selected real enemy is unavailable"));
					return;
				}
				UGameplayStatics::ApplyDamage(
					Enemy, 100000.0f, nullptr, this, nullptr);
				Ademo_mapCorpseContainerActor* Corpse = nullptr;
				for (const auto& Candidate : Corpses)
				{
					if (Candidate.IsValid()
						&& Candidate->GetProjectionResult().Trace.
							ProjectionId == Projection->ProjectionId)
					{
						Corpse = Candidate.Get();
						break;
					}
				}
				if (!Corpse)
				{
					Fail(TEXT("source-sequence: real enemy death did not create its Corpse"));
					return;
				}
				const auto Plan = Corpse->GetProjectionResult();
				bool bGuaranteeInSource = false;
				if (!AdvanceAttempts(
					Plan,
					AttemptCount,
					bGuaranteeInSource))
				{
					Fail(TEXT("source-sequence: real Corpse did not replay the selected attempt transitions"));
					return;
				}
				if (bGuaranteeInSource)
				{
					if (Plan.Trace.bJackpotHit
						|| Plan.Trace.bRareExtremeHit)
					{
						Fail(TEXT("attempt-4: guaranteed source did not naturally miss Jackpot and Rare"));
						return;
					}
					GuaranteedContainer = Corpse;
					GuaranteedProjection = Projection;
					ActualPlan = Plan;
				}
			}
		}
		if (AttemptCount != 4
			|| !GuaranteedContainer
			|| !GuaranteedProjection
			|| GetRewardAffixPityState(ActiveRunId) != 0)
		{
			Fail(TEXT("attempt-4: four real attempts, guaranteed container, or state reset is invalid"));
			return;
		}
		const auto* GuaranteedStack =
			ActualPlan.PlannedStacks.FindByPredicate([](const auto& Stack)
			{
				return Stack.AffixSet.Acquisition
					== Edemo_mapRewardAffixAcquisition::PityGuaranteed
					&& Stack.AffixSet.Affixes.ContainsByPredicate(
						[](const auto& Affix)
						{
							return Affix.AffixId
									== FName(TEXT("Reward.Affix.Weapon.Power.T3"))
								&& Affix.Tier
									== Edemo_mapRewardAffixTier::Tier3
								&& Affix.ResolvedMagnitudeScaled == 12
								&& Affix.ResolvedValue == 360;
						});
			});
		const auto* GuaranteedDefinition = GuaranteedStack
			? Fdemo_mapItemDefinitions::Find(
				GuaranteedStack->DefinitionId)
			: nullptr;
		if (!GuaranteedStack || !GuaranteedDefinition
			|| GuaranteedDefinition->CategoryId
				!= Fdemo_mapItemIds::WeaponCategory
			|| !GuaranteedStack->AffixSet.AffixSetEventId.IsValid()
			|| GuaranteedStack->AffixSet.AffixPolicyId
				!= Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId)
		{
			Fail(TEXT("attempt-4: guaranteed Power III stack metadata is invalid"));
			return;
		}
		const int64 ExpectedQuote =
			GuaranteedDefinition->SellPrice + 360;

		if (!OpenThroughProductInput(GuaranteedContainer))
		{
			Fail(TEXT("search-ui: real G input did not open the guaranteed source"));
			return;
		}
		Fdemo_mapRuntimeContainerEntrySnapshot Identified;
		if (!SearchIdentify(
			GuaranteedContainer,
			GuaranteedStack->Section,
			GuaranteedStack->SlotIndex,
			Identified))
		{
			Fail(TEXT("search-ui: guaranteed Weapon could not be identified"));
			return;
		}
		const auto SearchView =
			Fdemo_mapSearchContainerPresenter::Build(
				GuaranteedContainer->GetContainerSnapshot());
		const auto* SectionView =
			SearchView.Sections.FindByPredicate(
				[GuaranteedStack](const auto& Candidate)
				{
					return Candidate.Section
						== GuaranteedStack->Section;
				});
		const auto* SearchRow = SectionView
			? SectionView->Rows.FindByPredicate(
				[GuaranteedStack](const auto& Candidate)
				{
					return Candidate.SlotIndex
						== GuaranteedStack->SlotIndex;
				})
			: nullptr;
		if (!SearchRow
			|| !SearchRow->Text.Contains(TEXT("POWER III +12 ATK"))
			|| !SearchRow->Text.Contains(FString::Printf(
				TEXT("SELL=%lld"),
				ExpectedQuote))
			|| Identified.EffectiveStackSellValue != ExpectedQuote
			|| Identified.AffixSet != GuaranteedStack->AffixSet)
		{
			Fail(TEXT("search-ui: Power III label, exact SELL, or instance metadata is invalid"));
			return;
		}
		const FGuid GuaranteedItemId = Identified.ItemInstanceId;
		if (!SearchContainerWidget->AutomationClickEntry(
				GuaranteedStack->Section,
				GuaranteedStack->SlotIndex)
			|| !SearchContainerWidget->GetLastResult().bSuccess
			|| SearchContainerWidget->GetLastResult().ItemInstanceId
				!= GuaranteedItemId)
		{
			Fail(TEXT("take: real whole Take failed or changed the ItemInstance GUID"));
			return;
		}
		const Fdemo_mapItemInstance* Taken =
			Items->GetAuthority().FindInstance(GuaranteedItemId);
		if (!Taken || Taken->AffixSet != GuaranteedStack->AffixSet
			|| Items->GetAuthority().FindInventorySlot(GuaranteedItemId)
				== INDEX_NONE
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen
			|| !GetDemoController()
			|| !GetDemoController()->IsGameplayInputAllowed()
			|| GetDemoController()->IsMoveInputIgnored()
			|| GetDemoController()->IsLookInputIgnored())
		{
			Fail(TEXT("take-close: GUID/Affix transfer or immediate gameplay input restoration failed"));
			return;
		}
		const Fdemo_mapRewardAffixSet AffixBeforeReopen =
			Taken->AffixSet;
		if (!OpenThroughProductInput(GuaranteedContainer)
			|| (Cast<Ademo_mapLootChest>(GuaranteedContainer)
					? Cast<Ademo_mapLootChest>(GuaranteedContainer)->
						GetLastProjectionResult().Trace.PityStateOut
					: Cast<Ademo_mapCorpseContainerActor>(
						GuaranteedContainer)->
						GetProjectionResult().Trace.PityStateOut)
				!= 0
			|| GetRewardAffixPityState(ActiveRunId) != 0
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen
			|| !Items->GetAuthority().FindInstance(GuaranteedItemId)
			|| Items->GetAuthority().FindInstance(GuaranteedItemId)->
				AffixSet != AffixBeforeReopen)
		{
			Fail(TEXT("reopen: Affix rerolled, pity advanced, or UI failed to close"));
			return;
		}

		Udemo_mapAttributeComponent* Attributes =
			PlayerPawn.IsValid()
				? PlayerPawn->FindComponentByClass<
					Udemo_mapAttributeComponent>()
				: nullptr;
		float AttackBefore = 0.0f;
		if (!Attributes || !Attributes->GetFinalValue(
			Fdemo_mapAttributeIds::AttackPower,
			AttackBefore))
		{
			Fail(TEXT("equip: AttackPower authority is unavailable"));
			return;
		}
		const auto BaseEffects =
			Fdemo_mapEquipmentEffectResolver::Resolve(
				*GuaranteedDefinition,
				Fdemo_mapItemIds::WeaponSlot);
		float DefinitionAttackBonus = 0.0f;
		for (const Fdemo_mapModifierSpec& Modifier :
			BaseEffects.Modifiers)
		{
			if (Modifier.AttributeId
					== Fdemo_mapAttributeIds::AttackPower
				&& Modifier.Operation
					== Edemo_mapModifierOperation::Add)
			{
				DefinitionAttackBonus += Modifier.Value;
			}
		}
		if (!BaseEffects.bSuccess
			|| !Items->Equip(
				GuaranteedItemId,
				Fdemo_mapItemIds::WeaponSlot).bSuccess)
		{
			Fail(TEXT("equip: existing Equipment authority rejected the guaranteed Weapon"));
			return;
		}
		float AttackEquipped = 0.0f;
		const float CapturedDamage =
			Fdemo_mapPlayerCombat::CaptureOutgoingDamage(
				PlayerPawn.Get());
		if (!Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackEquipped)
			|| !FMath::IsNearlyEqual(
				AttackEquipped - AttackBefore
					- DefinitionAttackBonus,
				12.0f)
			|| !FMath::IsNearlyEqual(
				CapturedDamage - AttackBefore
					- DefinitionAttackBonus,
				12.0f))
		{
			Fail(TEXT("equip: Power III was not applied exactly once to the existing AttackPower/combat path"));
			return;
		}
		Ademo_mapTrainingTarget* DamageTarget = nullptr;
		TActorIterator<Ademo_mapTrainingTarget> TrainingTargetIt(
			GetWorld());
		if (TrainingTargetIt)
		{
			DamageTarget = *TrainingTargetIt;
		}
		const int32 HealthBefore =
			DamageTarget ? DamageTarget->GetHealth() : INDEX_NONE;
		if (!DamageTarget
			|| !MovePawnNear(DamageTarget, 120.0f)
			|| !PlayerPawn.IsValid())
		{
			Fail(TEXT("combat: real Training Target path is unavailable"));
			return;
		}
		PlayerPawn->SetActorRotation(
			(DamageTarget->GetActorLocation()
				- PlayerPawn->GetActorLocation()).Rotation());
		if (!PressBoundKey(EKeys::LeftMouseButton)
			|| (!DamageTarget->WasDestroyedByDamage()
				&& DamageTarget->GetHealth() >= HealthBefore))
		{
			Fail(TEXT("combat: real LMB path did not apply the captured affixed damage"));
			return;
		}
		if (!Items->Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess)
		{
			Fail(TEXT("unequip: existing Equipment authority rejected removal"));
			return;
		}
		float AttackUnequipped = 0.0f;
		if (!Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::AttackPower,
				AttackUnequipped)
			|| !FMath::IsNearlyEqual(
				AttackUnequipped,
				AttackBefore)
			|| !FMath::IsNearlyEqual(
				Fdemo_mapPlayerCombat::CaptureOutgoingDamage(
					PlayerPawn.Get()),
				AttackBefore))
		{
			Fail(TEXT("unequip: Power III modifier was not removed exactly once"));
			return;
		}

		DestroyRuntimeContainers(
			TEXT("RewardAffixPityBeforeExtraction"));
		Fdemo_mapSettlementSummary Summary;
		if (!Items->RequestSettlement(
			Edemo_mapRunEndReason::Extraction,
			Summary).bSuccess)
		{
			Fail(TEXT("settlement: real Extraction request failed"));
			return;
		}
		const auto Settlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(Summary);
		if (!Settlement.IsDurablySettled())
		{
			Fail(TEXT("settlement: Profile transaction was not durable"));
			return;
		}
		Fdemo_mapProfileRepository Repository;
		const auto Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const auto Reloaded =
			Repository.LoadExistingProfile(Storage);
		const auto* Persisted = Reloaded.IsSuccess()
			? Reloaded.Profile.PermanentStash.FindByPredicate(
				[GuaranteedItemId](const auto& Item)
				{
					return Item.ItemInstanceId
						== GuaranteedItemId;
				})
			: nullptr;
		if (!Persisted
			|| Persisted->AffixSet != AffixBeforeReopen)
		{
			Fail(TEXT("profile-reload: GUID or canonical AffixSet did not survive settlement"));
			return;
		}
		const auto PreparationView =
			Fdemo_mapProfilePreparationPresenter::BuildViewState(
				RewardGenerationProfileSession->
					GetPreparationSnapshot());
		const auto* AffixQuote =
			PreparationView.OrderedPermanentStashRows.
				FindByPredicate(
					[GuaranteedItemId](const auto& Row)
					{
						return Row.ItemInstanceId
							== GuaranteedItemId;
					});
		const auto* NormalQuote =
			PreparationView.OrderedPermanentStashRows.
				FindByPredicate([](const auto& Row)
				{
					return Row.AffixSet.IsEmpty()
						&& !Row.RareRewardEventId.IsValid()
						&& Row.RewardEventKind
							== Edemo_mapRewardEventKind::None
						&& Row.TotalSellPrice > 0;
				});
		const auto* NormalDefinition = NormalQuote
			? Fdemo_mapItemDefinitions::Find(
				NormalQuote->ItemDefinitionId)
			: nullptr;
		if (!AffixQuote
			|| AffixQuote->TotalSellPrice != ExpectedQuote
			|| !AffixQuote->RewardLabel.Contains(
				TEXT("POWER III +12 ATK"))
			|| !NormalQuote || !NormalDefinition
			|| NormalQuote->TotalSellPrice
				!= NormalDefinition->SellPrice
					* static_cast<int64>(NormalQuote->StackCount)
			|| NormalQuote->RewardLabel.Contains(TEXT("POWER")))
		{
			Fail(TEXT("shop-quote: affixed quote/label or normal control semantics changed"));
			return;
		}
		FString ProfileJson;
		if (!FFileHelper::LoadFileToString(
				ProfileJson,
				*Storage.PrimaryPath())
			|| ProfileJson.Contains(TEXT("PityState"))
			|| ProfileJson.Contains(
				Fdemo_mapRewardAffixPolicyRegistry::
					PityChannelId.ToString())
			|| ProfileJson.Contains(
				Fdemo_mapRewardAffixPolicyRegistry::
					PityPolicyId.ToString()))
		{
			Fail(TEXT("profile-schema: Active Run pity state leaked into Permanent Profile"));
			return;
		}

		const int64 BalanceBefore =
			RewardGenerationProfileSession->GetSnapshot().
				PersistentSpiritStones;
		const auto BeforeSale =
			RewardGenerationProfileSession->GetSnapshot();
		Fdemo_mapProfileTradeIntent Sell;
		Sell.Kind = Edemo_mapProfileTradeKind::Sell;
		Sell.ExpectedProfileId = BeforeSale.ProfileId;
		Sell.ExpectedSaveGeneration =
			BeforeSale.SaveGeneration;
		Sell.ItemInstanceId = GuaranteedItemId;
		const auto Sale =
			RewardGenerationProfileSession->
				SubmitTradeIntent(Sell);
		const auto AfterSaleReload =
			Repository.LoadExistingProfile(Storage);
		const int64 BalanceAfter =
			RewardGenerationProfileSession->GetSnapshot().
				PersistentSpiritStones;
		if (!Sale.IsCommitted()
			|| Sale.TotalPrice != ExpectedQuote
			|| BalanceAfter - BalanceBefore != ExpectedQuote
			|| !AfterSaleReload.IsSuccess()
			|| AfterSaleReload.Profile.PersistentSpiritStones
				!= BalanceAfter
			|| AfterSaleReload.Profile.PermanentStash.
				ContainsByPredicate(
					[GuaranteedItemId](const auto& Item)
					{
						return Item.ItemInstanceId
							== GuaranteedItemId;
					}))
		{
			Fail(TEXT("sell: exact credit, sold GUID removal, or balance reload failed"));
			return;
		}

		Items->ResetForAutomation();
		Items->BeginWorld(GetWorld());
		const auto NewRun =
			RewardGenerationProfileSession->StartPreparedRun();
		if (!NewRun.IsRunActive()
			|| GetRewardAffixPityState(
				NewRun.Snapshot.ActiveRunId) != 0)
		{
			Fail(FString::Printf(
				TEXT("new-run: Active Run did not begin with pity state 0 status=%d runtime=%d diagnostic=%s"),
				static_cast<int32>(NewRun.Status),
				static_cast<int32>(NewRun.RuntimeResult.Status),
				*NewRun.Diagnostic));
			return;
		}
		Fdemo_mapSettlementSummary NewRunSummary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Abandon,
				NewRunSummary).bSuccess
			|| !RewardGenerationProfileSession->
				CommitRuntimeSettlement(NewRunSummary).
					IsDurablySettled())
		{
			Fail(TEXT("new-run: cleanup settlement failed"));
			return;
		}

		GetWorldTimerManager().ClearTimer(AutomationTimer);
		Items->TeardownWorld(GetWorld());
		const int32 ResidualObjects =
			Items->GetWorldActorCount()
				+ Chests.Num()
				+ Corpses.Num();
		if (ResidualObjects != 0)
		{
			Fail(TEXT("cleanup: task-owned Runtime objects remain"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P5_REWARD_AFFIX_PITY_PRODUCT_EVIDENCE search_limit=250000 actual_attempts=%u run=%s sequence=%s guaranteed_projection=%s affix_event=%s guid=%s base_sell=%lld affix_value=360 quote=%lld attack_before=%.2f definition_attack=%.2f attack_affixed=%.2f captured_damage=%.2f attack_unequipped=%.2f taken=1 input_restore=1 equip_once=1 lmb=1 unequip_once=1 settlement=1 profile_reload=1 sell=1 balance_before=%lld balance_after=%lld new_run_pity=0 persistent_pity=0 reroll=0 fallback=0 residual_objects=0."),
			ActiveRunId.D,
			*ActiveRunId.ToString(EGuidFormats::Digits),
			*FString::Join(AttemptEvidence, TEXT(",")),
			*GuaranteedProjection->ProjectionId.ToString(),
			*AffixBeforeReopen.AffixSetEventId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			*GuaranteedItemId.ToString(
				EGuidFormats::DigitsWithHyphensLower),
			GuaranteedDefinition->SellPrice,
			ExpectedQuote,
			AttackBefore,
			DefinitionAttackBonus,
			AttackEquipped,
			CapturedDamage,
			AttackUnequipped,
			BalanceBefore,
			BalanceAfter);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P5_REWARD_AFFIX_PITY_PRODUCT_SMOKE: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	void Ademo_mapV3ProgressionManager::RunRewardRareExtremeAutomation()
	{
		auto Cleanup = [this](Edemo_mapRunEndReason Reason)
		{
			CloseSearchContainer(
				TEXT("RewardRareExtremeSmokeCleanup"), true);
			DestroyRuntimeContainers(
				TEXT("RewardRareExtremeSmokeCleanup"));
			if (Items.IsValid()
				&& Items->GetRunState() == Edemo_mapRunState::Active)
			{
				Fdemo_mapSettlementSummary Summary;
				if (Items->RequestSettlement(Reason, Summary).bSuccess
					&& RewardGenerationProfileSession.IsValid())
				{
					RewardGenerationProfileSession->
						CommitRuntimeSettlement(Summary);
				}
			}
			if (Items.IsValid())
			{
				Items->TeardownWorld(GetWorld());
			}
		};
		auto Fail = [this, &Cleanup](const FString& Reason)
		{
			GetWorldTimerManager().ClearTimer(AutomationTimer);
			Cleanup(Edemo_mapRunEndReason::Abandon);
			FString Clean = Reason;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P4_RARE_EXTREME_VALUE_PRODUCT_SMOKE: FAIL: %s"),
				*Clean);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		auto OpenThroughProductInput = [this](
			Ademo_mapSearchContainerActor* Container) -> bool
		{
			if (!Container || !PlayerPawn.IsValid()
				|| !GetDemoController())
			{
				return false;
			}
			MovePawnNear(Container);
			GetDemoController()->SetAutomationAimDirection(
				Container->GetActorLocation()
					- PlayerPawn->GetActorLocation());
			SetFocusedActor(Container);
			if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G))
			{
				return false;
			}
			if (Container->IsContainerOpening()
				&& !Container->CompleteActionForAutomation().bSuccess)
			{
				return false;
			}
			return GetDemoController()->
					DispatchAutomationKeyReleased(EKeys::G)
				&& Container->IsContainerOpened()
				&& bSearchContainerOpen
				&& ActiveSearchContainer.Get() == Container
				&& SearchContainerWidget;
		};
		auto FindEntry = [](
			const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex)
			-> const Fdemo_mapRuntimeContainerEntrySnapshot*
		{
			const auto* FoundSection =
				Snapshot.Sections.FindByPredicate(
					[Section](const auto& Candidate)
					{
						return Candidate.Section == Section;
					});
			return FoundSection
				? FoundSection->OrderedOccupiedEntries.
					FindByPredicate(
						[SlotIndex](const auto& Candidate)
						{
							return Candidate.SlotIndex
								== SlotIndex;
						})
				: nullptr;
		};
		auto SearchIdentify = [this, &FindEntry](
			Ademo_mapSearchContainerActor* Container,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex,
			Fdemo_mapRuntimeContainerEntrySnapshot& OutEntry)
			-> bool
		{
			const auto Before = Container->GetContainerSnapshot();
			const auto* Entry =
				FindEntry(Before, Section, SlotIndex);
			if (!Entry)
			{
				return false;
			}
			if (Entry->State
				== Edemo_mapRuntimeContainerEntryState::Hidden)
			{
				if (!SearchContainerWidget->AutomationClickEntry(
					Section, SlotIndex))
				{
					return false;
				}
				if (Container->IsContainerSearching()
					&& !Container->CompleteActionForAutomation().
						bSuccess)
				{
					return false;
				}
			}
			const auto After = Container->GetContainerSnapshot();
			Entry = FindEntry(After, Section, SlotIndex);
			if (!Entry || Entry->State
				!= Edemo_mapRuntimeContainerEntryState::Identified)
			{
				return false;
			}
			OutEntry = *Entry;
			return true;
		};

		if (!Items.IsValid()
			|| !RewardGenerationProfileSession.IsValid()
			|| Chests.Num()
				!= Fdemo_mapRewardFullMapDistribution::
					TotalContainerCount)
		{
			Fail(TEXT("isolated Profile, ActiveRun, or current real Chest topology is unavailable"));
			return;
		}
		const FGuid ActiveRunId =
			RewardGenerationProfileSession->GetSnapshot().ActiveRunId;
		const Fdemo_mapFullMapRewardSlot* HitSlot =
			Fdemo_mapRewardFullMapDistribution::GetSlots().
				FindByPredicate([](const auto& Slot)
				{
					return Slot.StableSourceRoleId
						== FName(
							TEXT("P8.SourceRole.Enemy.Standard.001"))
						&& Slot.ProjectionId
							== Fdemo_mapRewardProjectionIds::
								CorpseMainMeleeStandard;
				});
		const Fdemo_mapRewardSourceProjection ProjectionValue =
			HitSlot
				? Fdemo_mapRewardFullMapDistribution::
					BuildProjection(*HitSlot)
				: Fdemo_mapRewardSourceProjection();
		const auto* Projection =
			ProjectionValue.IsValid() ? &ProjectionValue : nullptr;
		const auto* Profile = Projection
			? Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
				Projection->BudgetProfileId)
			: nullptr;
		FGuid NaturalRunId;
		int32 NaturalAttempt = INDEX_NONE;
		if (!Projection || !Profile
			|| Profile->BaseValue != 1200
			|| !Fdemo_mapRewardRareExtreme::
				FindNaturalTier125JackpotMiss(
					Fdemo_mapRewardRareExtremePolicyRegistry::
						GetDefault(),
					Fdemo_mapRewardJackpotPolicyRegistry::
						GetDefault(),
					Projection->StableSourceRoleId,
					Projection->ProjectionId,
					Profile->BaseValue,
					2000000,
					NaturalRunId,
					NaturalAttempt)
			|| ActiveRunId != NaturalRunId)
		{
			Fail(TEXT("bounded natural Standard Corpse Tier125/Jackpot-miss identity was not used"));
			return;
		}

		Ademo_mapLootChest* MissChest = nullptr;
		for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate :
			Chests)
		{
			Ademo_mapLootChest* Chest = Candidate.Get();
			if (!Chest
				|| !Chest->GetLastProjectionResult().IsSuccess()
				|| Chest->GetLastProjectionResult().Trace.
					bRareExtremeHit
				|| Chest->GetLastProjectionResult().Trace.
					bFallbackUsed)
			{
				continue;
			}
			if (!MissChest
				|| Chest->GetLastProjectionResult().Trace.
					StableSourceRoleId.LexicalLess(
						MissChest->GetLastProjectionResult().Trace.
							StableSourceRoleId))
			{
				MissChest = Chest;
			}
		}
		const Fdemo_mapRewardSourceProjectionResult MissPlan = MissChest
			? MissChest->GetLastProjectionResult()
			: Fdemo_mapRewardSourceProjectionResult();
		if (!MissChest || !MissPlan.IsSuccess()
			|| MissPlan.Trace.bRareExtremeHit
			|| MissPlan.Trace.RandomizedBudget
				!= MissPlan.Trace.NormalRandomizedBudget
			|| MissPlan.PlannedStacks.IsEmpty())
		{
			Fail(TEXT("the second real source was not a natural Rare miss"));
			return;
		}
		if (!OpenThroughProductInput(MissChest))
		{
			Fail(TEXT("real G input did not open the natural Rare-miss source"));
			return;
		}
		Fdemo_mapRuntimeContainerEntrySnapshot MissEntry;
		const auto& MissStack = MissPlan.PlannedStacks[0];
		if (!SearchIdentify(
				MissChest,
				MissStack.Section,
				MissStack.SlotIndex,
				MissEntry)
			|| MissEntry.RareRewardEventId.IsValid()
			|| !MissEntry.RareRewardPolicyId.IsNone()
			|| !MissEntry.RareRewardTierId.IsNone()
			|| MissEntry.RareRewardBonusValue != 0
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen)
		{
			Fail(TEXT("real Rare-miss source did not preserve empty Rare metadata"));
			return;
		}

		Ademo_mapEnemyCharacter* StandardEnemy = nullptr;
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld());
			It; ++It)
		{
			if (It->GetEncounterIdentity().EncounterId
				== Fdemo_mapEnemyEncounterIds::MainMeleeStandard)
			{
				StandardEnemy = *It;
				break;
			}
		}
		if (!StandardEnemy)
		{
			Fail(TEXT("real Standard melee enemy is unavailable"));
			return;
		}
		UGameplayStatics::ApplyDamage(
			StandardEnemy, 100.0f, nullptr, this, nullptr);
		Ademo_mapCorpseContainerActor* HitCorpse = nullptr;
		for (const auto& Candidate : Corpses)
		{
			if (Candidate.IsValid()
				&& Candidate->GetRewardProjectionId()
					== Projection->ProjectionId)
			{
				HitCorpse = Candidate.Get();
				break;
			}
		}
		if (!HitCorpse)
		{
			Fail(TEXT("standard enemy death did not create the real generated Corpse"));
			return;
		}
		const Fdemo_mapRewardSourceProjectionResult HitPlan =
			HitCorpse->GetProjectionResult();
		TSet<Edemo_mapRuntimeContainerSection> NonEmptySections;
		int64 PlannedBonusSum = 0;
		int32 PlannedCarrierCount = 0;
		for (const auto& Stack : HitPlan.PlannedStacks)
		{
			NonEmptySections.Add(Stack.Section);
			PlannedBonusSum += Stack.RareRewardBonusValue;
			PlannedCarrierCount +=
				Stack.RareRewardEventId.IsValid() ? 1 : 0;
		}
		if (!HitCorpse->UsesGeneratedReward()
			|| !HitPlan.IsSuccess()
			|| HitPlan.Trace.bFallbackUsed
			|| !HitPlan.Trace.bRareExtremeHit
			|| HitPlan.Trace.RareExtremeTierId
				!= Fdemo_mapRewardRareExtremePolicyRegistry::Tier125Id
			|| HitPlan.Trace.RareExtremeTargetValue != 150000
			|| HitPlan.Trace.bJackpotHit
			|| HitPlan.Trace.JackpotRoll < 500
			|| NonEmptySections.Num() != 3
			|| PlannedCarrierCount < 1
			|| PlannedCarrierCount > 3
			|| PlannedBonusSum
				!= HitPlan.Trace.RareExtremeBonusPoolValue
			|| HitPlan.Trace.GeneratedTotalValue
				+ PlannedBonusSum != 150000)
		{
			Fail(TEXT("real Corpse Rare decision, three-section plan, carriers, target, or unchanged Jackpot miss is invalid"));
			return;
		}
		if (!OpenThroughProductInput(HitCorpse))
		{
			Fail(TEXT("real G input did not open the Rare-hit Corpse"));
			return;
		}

		TArray<FGuid> TakenIds;
		TArray<FString> ProductItemEvidence;
		int64 IdentifiedEffectiveTotal = 0;
		for (const auto& Stack : HitPlan.PlannedStacks)
		{
			Fdemo_mapRuntimeContainerEntrySnapshot Identified;
			if (!SearchIdentify(
				HitCorpse,
				Stack.Section,
				Stack.SlotIndex,
				Identified))
			{
				Fail(TEXT("a real Corpse entry could not be identified"));
				return;
			}
			const auto SearchView =
				Fdemo_mapSearchContainerPresenter::Build(
					HitCorpse->GetContainerSnapshot());
			const auto* SectionView =
				SearchView.Sections.FindByPredicate(
					[&Stack](const auto& Candidate)
					{
						return Candidate.Section == Stack.Section;
					});
			const auto* Row = SectionView
				? SectionView->Rows.FindByPredicate(
					[&Stack](const auto& Candidate)
					{
						return Candidate.SlotIndex
							== Stack.SlotIndex;
					})
				: nullptr;
			const bool bCarrier =
				Stack.RareRewardEventId.IsValid();
			if (!Row
				|| !Row->Text.Contains(FString::Printf(
					TEXT("SELL=%lld"),
					Identified.EffectiveStackSellValue))
				|| (bCarrier
					&& (!Row->Text.Contains(
							TEXT("EXTREME VALUE"))
						|| Identified.RareRewardEventId
							!= HitPlan.Trace.RareExtremeEventId
						|| Identified.RareRewardBonusValue <= 0))
				|| (!bCarrier
					&& (Identified.RareRewardEventId.IsValid()
						|| Identified.RareRewardBonusValue != 0)))
			{
				Fail(TEXT("SearchContainer Rare label, effective price, or carrier metadata is invalid"));
				return;
			}
			IdentifiedEffectiveTotal +=
				Identified.EffectiveStackSellValue;
			ProductItemEvidence.Add(FString::Printf(
				TEXT("item%d={guid=%s,definition=%s,quantity=%d,base=%lld,bonus=%lld,effective=%lld,event=%s}"),
				ProductItemEvidence.Num() + 1,
				*Identified.ItemInstanceId.ToString(
					EGuidFormats::DigitsWithHyphensLower),
				*Identified.DefinitionId.ToString(),
				Identified.StackCount,
				Identified.UnitSellPrice
					* static_cast<int64>(Identified.StackCount),
				Identified.RareRewardBonusValue,
				Identified.EffectiveStackSellValue,
				*Identified.RareRewardEventId.ToString(
					EGuidFormats::DigitsWithHyphensLower)));
			const FGuid GuidBeforeTake =
				Identified.ItemInstanceId;
			if (!SearchContainerWidget->AutomationClickEntry(
					Stack.Section,
					Stack.SlotIndex)
				|| !SearchContainerWidget->GetLastResult().
					bSuccess
				|| SearchContainerWidget->GetLastResult().
					ItemInstanceId != GuidBeforeTake)
			{
				Fail(TEXT("real whole Take failed or changed ItemInstance GUID"));
				return;
			}
			TakenIds.Add(GuidBeforeTake);
		}
		if (TakenIds.Num() != HitPlan.PlannedStacks.Num()
			|| IdentifiedEffectiveTotal != 150000
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen
			|| !GetDemoController()
			|| !GetDemoController()->IsGameplayInputAllowed()
			|| GetDemoController()->IsMoveInputIgnored()
			|| GetDemoController()->IsLookInputIgnored())
		{
			Fail(TEXT("all-source Take, target total, UI close, or immediate gameplay input restoration failed"));
			return;
		}
		const int32 InstanceCountBeforeReopen =
			Items->GetAuthority().GetInstanceSnapshot().Num();
		MovePawnNear(HitCorpse);
		GetDemoController()->SetAutomationAimDirection(
			HitCorpse->GetActorLocation()
				- PlayerPawn->GetActorLocation());
		SetFocusedActor(HitCorpse);
		if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G)
			|| !GetDemoController()->
				DispatchAutomationKeyReleased(EKeys::G)
			|| !bSearchContainerOpen
			|| HitCorpse->GetProjectionResult().Trace.
				RareExtremeEventId
				!= HitPlan.Trace.RareExtremeEventId
			|| Items->GetAuthority().GetInstanceSnapshot().Num()
				!= InstanceCountBeforeReopen
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen)
		{
			Fail(TEXT("reopen rerolled Rare identity, duplicated an item, or failed close"));
			return;
		}

		DestroyRuntimeContainers(
			TEXT("RewardRareExtremeBeforeExtraction"));
		Fdemo_mapSettlementSummary Summary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				Summary).bSuccess)
		{
			Fail(TEXT("real Extraction request failed"));
			return;
		}
		const auto Settlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(Summary);
		if (!Settlement.IsDurablySettled())
		{
			Fail(TEXT("Profile settlement did not durably commit all source items"));
			return;
		}
		Fdemo_mapProfileRepository Repository;
		const auto Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const auto Reloaded =
			Repository.LoadExistingProfile(Storage);
		FString ProfileError;
		if (!Reloaded.IsSuccess()
			|| !Repository.ValidateProfile(
				Reloaded.Profile, &ProfileError))
		{
			Fail(TEXT("post-extraction Profile reload is invalid: ")
				+ ProfileError);
			return;
		}
		for (const FGuid& Id : TakenIds)
		{
			if (!Reloaded.Profile.PermanentStash.
				ContainsByPredicate([Id](const auto& Item)
					{
						return Item.ItemInstanceId == Id;
					}))
			{
				Fail(TEXT("an extracted source item is absent after reload"));
				return;
			}
		}
		const auto Preparation =
			RewardGenerationProfileSession->
				GetPreparationSnapshot();
		const auto PreparationView =
			Fdemo_mapProfilePreparationPresenter::
				BuildViewState(Preparation);
		int64 QuotedTotal = 0;
		for (const FGuid& Id : TakenIds)
		{
			const auto* Row =
				PreparationView.OrderedPermanentStashRows.
					FindByPredicate([Id](const auto& Candidate)
					{
						return Candidate.ItemInstanceId == Id;
					});
			if (!Row || Row->TotalSellPrice <= 0)
			{
				Fail(TEXT("Shop quote is missing for an extracted source item"));
				return;
			}
			QuotedTotal += Row->TotalSellPrice;
		}
		const auto* NormalQuote =
			PreparationView.OrderedPermanentStashRows.
				FindByPredicate([](const auto& Row)
				{
					return !Row.RareRewardEventId.IsValid()
						&& Row.RewardEventKind
							== Edemo_mapRewardEventKind::None
						&& Row.TotalSellPrice > 0;
				});
		const auto* NormalDefinition = NormalQuote
			? Fdemo_mapItemDefinitions::Find(
				NormalQuote->ItemDefinitionId)
			: nullptr;
		const auto Catalog =
			Fdemo_mapProfileTradeTransaction::BuildCatalog();
		const auto* Pill =
			Catalog.FindByPredicate([](const auto& Row)
				{
					return Row.ItemDefinitionId
						== Fdemo_mapItemIds::HealingPillLevel1;
				});
		if (QuotedTotal != 150000
			|| !NormalQuote || !NormalDefinition
			|| NormalQuote->TotalSellPrice
				!= NormalDefinition->SellPrice
					* static_cast<int64>(
						NormalQuote->StackCount)
			|| !Pill || Pill->BuyPrice != 30)
		{
			Fail(TEXT("aggregate Rare quote, normal quote, or normal buy catalog changed"));
			return;
		}

		const int64 BalanceBefore =
			RewardGenerationProfileSession->GetSnapshot().
				PersistentSpiritStones;
		int64 Credited = 0;
		for (const FGuid& Id : TakenIds)
		{
			const auto BeforeSale =
				RewardGenerationProfileSession->GetSnapshot();
			Fdemo_mapProfileTradeIntent Sell;
			Sell.Kind = Edemo_mapProfileTradeKind::Sell;
			Sell.ExpectedProfileId = BeforeSale.ProfileId;
			Sell.ExpectedSaveGeneration =
				BeforeSale.SaveGeneration;
			Sell.ItemInstanceId = Id;
			const auto Sale =
				RewardGenerationProfileSession->
					SubmitTradeIntent(Sell);
			if (!Sale.IsCommitted())
			{
				Fail(TEXT("real Shop failed to sell every source item"));
				return;
			}
			Credited += Sale.TotalPrice;
		}
		const auto AfterSaleReload =
			Repository.LoadExistingProfile(Storage);
		const int64 BalanceAfter =
			RewardGenerationProfileSession->GetSnapshot().
				PersistentSpiritStones;
		if (Credited != 150000
			|| BalanceAfter - BalanceBefore != 150000
			|| !AfterSaleReload.IsSuccess()
			|| AfterSaleReload.Profile.PersistentSpiritStones
				!= BalanceAfter)
		{
			Fail(TEXT("aggregate sell credit or balance reload is not exactly 150000"));
			return;
		}
		for (const FGuid& Id : TakenIds)
		{
			if (AfterSaleReload.Profile.PermanentStash.
				ContainsByPredicate([Id](const auto& Item)
					{
						return Item.ItemInstanceId == Id;
					}))
			{
				Fail(TEXT("a sold source item survived the final reload"));
				return;
			}
		}

		GetWorldTimerManager().ClearTimer(AutomationTimer);
		if (Items.IsValid())
		{
			Items->TeardownWorld(GetWorld());
		}
		const int32 ResidualWorldItems =
			Items.IsValid() ? Items->GetWorldActorCount() : 0;
		if (ResidualWorldItems != 0 || !Corpses.IsEmpty())
		{
			Fail(TEXT("cleanup left residual Runtime Container or World Item objects"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P4_RARE_EXTREME_VALUE_PRODUCT_EVIDENCE run=%s search_bound=2000000 actual_attempt=%d projection=%s base=1200 rare_roll=%d tier_roll=%d tier=%s target=150000 jackpot_roll=%d jackpot_miss=1 miss_source=%s miss_roll=%d sections=3 stacks=%d carriers=%d bonus_pool=%lld taken=%d quoted=150000 credited=150000 balance_before=%lld balance_after=%lld input_restore=1 profile_reload=1 reroll=0 duplicate=0 fallback=0 residual_objects=0."),
			*ActiveRunId.ToString(EGuidFormats::Digits),
			NaturalAttempt,
			*Projection->ProjectionId.ToString(),
			HitPlan.Trace.RareRoll,
			HitPlan.Trace.RareTierRoll,
			*HitPlan.Trace.RareExtremeTierId.ToString(),
			HitPlan.Trace.JackpotRoll,
			*MissPlan.Trace.ProjectionId.ToString(),
			MissPlan.Trace.RareRoll,
			HitPlan.PlannedStacks.Num(),
			PlannedCarrierCount,
			PlannedBonusSum,
			TakenIds.Num(),
			BalanceBefore,
			BalanceAfter);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P4_RARE_EXTREME_VALUE_PRODUCT_ITEMS %s."),
			*FString::Join(ProductItemEvidence, TEXT(" ")));
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P4_RARE_EXTREME_VALUE_PRODUCT_SMOKE: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	void Ademo_mapV3ProgressionManager::RunRewardJackpotAutomation()
	{
		auto Cleanup = [this](Edemo_mapRunEndReason Reason)
		{
			CloseSearchContainer(TEXT("RewardJackpotSmokeCleanup"), true);
			DestroyRuntimeContainers(TEXT("RewardJackpotSmokeCleanup"));
			if (Items.IsValid()
				&& Items->GetRunState() == Edemo_mapRunState::Active)
			{
				Fdemo_mapSettlementSummary Summary;
				if (Items->RequestSettlement(Reason, Summary).bSuccess
					&& RewardGenerationProfileSession.IsValid())
				{
					RewardGenerationProfileSession->
						CommitRuntimeSettlement(Summary);
				}
			}
			if (Items.IsValid())
			{
				Items->TeardownWorld(GetWorld());
			}
		};
		auto Fail = [this, &Cleanup](const FString& Reason)
		{
			GetWorldTimerManager().ClearTimer(AutomationTimer);
			Cleanup(Edemo_mapRunEndReason::Abandon);
			FString Clean = Reason;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("P3_REWARD_JACKPOT_PRODUCT_SMOKE: FAIL: %s"),
				*Clean);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		auto OpenThroughProductInput = [this](
			Ademo_mapSearchContainerActor* Container) -> bool
		{
			if (!Container
				|| !PlayerPawn.IsValid()
				|| !GetDemoController())
			{
				return false;
			}
			MovePawnNear(Container);
			GetDemoController()->SetAutomationAimDirection(
				Container->GetActorLocation()
					- PlayerPawn->GetActorLocation());
			SetFocusedActor(Container);
			return GetDemoController()->
					DispatchAutomationKeyPressed(EKeys::G)
				&& Container->IsContainerOpening()
				&& Container->CompleteActionForAutomation().bSuccess
				&& GetDemoController()->
					DispatchAutomationKeyReleased(EKeys::G)
				&& Container->IsContainerOpened()
				&& bSearchContainerOpen
				&& ActiveSearchContainer.Get() == Container
				&& SearchContainerWidget;
		};
		auto FindEntry = [](
			const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex)
			-> const Fdemo_mapRuntimeContainerEntrySnapshot*
		{
			const Fdemo_mapRuntimeContainerSectionSnapshot* FoundSection =
				Snapshot.Sections.FindByPredicate(
					[Section](const auto& Candidate)
					{
						return Candidate.Section == Section;
					});
			return FoundSection
				? FoundSection->OrderedOccupiedEntries.FindByPredicate(
					[SlotIndex](const auto& Candidate)
					{
						return Candidate.SlotIndex == SlotIndex;
					})
				: nullptr;
		};
		auto SearchIdentify = [this, &FindEntry](
			Ademo_mapSearchContainerActor* Container,
			Edemo_mapRuntimeContainerSection Section,
			int32 SlotIndex,
			Fdemo_mapRuntimeContainerEntrySnapshot& OutEntry) -> bool
		{
			const Fdemo_mapRuntimeContainerSnapshot Before =
				Container->GetContainerSnapshot();
			const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
				FindEntry(Before, Section, SlotIndex);
			if (!Entry)
			{
				return false;
			}
			if (Entry->State
				== Edemo_mapRuntimeContainerEntryState::Hidden)
			{
				if (!SearchContainerWidget->AutomationClickEntry(
						Section, SlotIndex)
					|| !Container->IsContainerSearching()
					|| !Container->CompleteActionForAutomation().
						bSuccess)
				{
					return false;
				}
			}
			const Fdemo_mapRuntimeContainerSnapshot After =
				Container->GetContainerSnapshot();
			Entry = FindEntry(After, Section, SlotIndex);
			if (!Entry
				|| Entry->State
					!= Edemo_mapRuntimeContainerEntryState::Identified)
			{
				return false;
			}
			OutEntry = *Entry;
			return true;
		};

		if (!Items.IsValid()
			|| !RewardGenerationProfileSession.IsValid()
			|| Chests.Num()
				!= Fdemo_mapRewardFullMapDistribution::
					TotalContainerCount)
		{
			Fail(TEXT("isolated ActiveRun, Profile Session, or current real Chest topology is unavailable"));
			return;
		}
		Ademo_mapLootChest* HitChest = nullptr;
		Ademo_mapLootChest* MissChest = nullptr;
		int32 TopologyHitCount = 0;
		for (const TWeakObjectPtr<Ademo_mapLootChest>& ChestPtr :
			Chests)
		{
			Ademo_mapLootChest* Chest = ChestPtr.Get();
			if (!Chest
				|| !Chest->GetLastProjectionResult().IsSuccess()
				|| Chest->GetLastProjectionResult().Trace.bFallbackUsed)
			{
				Fail(TEXT("current Chest topology contains an invalid or fallback Projection"));
				return;
			}
			const FName StableRole =
				Chest->GetLastProjectionResult().Trace.
					StableSourceRoleId;
			if (Chest->GetLastProjectionResult().Trace.bJackpotHit)
			{
				++TopologyHitCount;
				if (!HitChest
					|| StableRole.LexicalLess(
						HitChest->GetLastProjectionResult().Trace.
							StableSourceRoleId))
				{
					HitChest = Chest;
				}
			}
			else if (!MissChest
				|| StableRole.LexicalLess(
					MissChest->GetLastProjectionResult().Trace.
						StableSourceRoleId))
			{
				MissChest = Chest;
			}
		}
		if (!HitChest || !MissChest)
		{
			Fail(TEXT("current topology has no stable natural hit/miss Chest representatives"));
			return;
		}
		const Fdemo_mapRewardSourceProjectionResult& HitPlan =
			HitChest->GetLastProjectionResult();
		const Fdemo_mapRewardSourceProjectionResult& MissPlan =
			MissChest->GetLastProjectionResult();
		const FGuid ActiveRunId =
			RewardGenerationProfileSession->GetSnapshot().ActiveRunId;
		const int32 SelectedIndex =
			HitPlan.Trace.JackpotSelectedPlannedEntryIndex;
		if (!ActiveRunId.IsValid()
			|| HitChest->GetOwningRunId() != ActiveRunId
			|| MissChest->GetOwningRunId() != ActiveRunId
			|| !HitPlan.IsSuccess()
			|| !MissPlan.IsSuccess()
			|| !HitPlan.Trace.bJackpotHit
			|| MissPlan.Trace.bJackpotHit
			|| !HitPlan.PlannedStacks.IsValidIndex(SelectedIndex)
			|| HitPlan.Trace.JackpotRoll < 0
			|| HitPlan.Trace.JackpotRoll >= 500
			|| MissPlan.Trace.JackpotRoll < 500
			|| MissPlan.Trace.JackpotRoll > 9999
			|| HitPlan.Trace.bFallbackUsed
			|| MissPlan.Trace.bFallbackUsed)
		{
			Fail(TEXT("bounded natural RunId did not produce one real hit and one real miss"));
			return;
		}
		const Fdemo_mapRewardPlannedStack Selected =
			HitPlan.PlannedStacks[SelectedIndex];
		if (Selected.RewardEventKind
				!= Edemo_mapRewardEventKind::Jackpot
			|| !Selected.RewardEventId.IsValid()
			|| Selected.RewardValueMultiplierBps != 60000
			|| Selected.RewardSourceRoleId
				!= HitPlan.Trace.StableSourceRoleId)
		{
			Fail(TEXT("natural hit did not annotate exactly the selected planned entry"));
			return;
		}
		int32 ProjectionHitCount = 0;
		bool bAllProjectionPlansValid = true;
		for (const Fdemo_mapRewardSourceProjection& Projection :
			Fdemo_mapRewardSourceProjectionRegistry::GetAll())
		{
			const Fdemo_mapRewardSourceProjectionResult ProjectionPlan =
				Fdemo_mapRewardSourceProjectionPlanner::Plan(
					Projection,
					ActiveRunId);
			ProjectionHitCount +=
				ProjectionPlan.Trace.bJackpotHit ? 1 : 0;
			bAllProjectionPlansValid &=
				ProjectionPlan.IsSuccess()
				&& !ProjectionPlan.Trace.bFallbackUsed
				&& ProjectionPlan.Trace.GeneratedTotalValue
					<= ProjectionPlan.Trace.RandomizedBudget;
		}
		int32 MaterializedJackpotCount = 0;
		bool bAllOtherInstancesNormal = true;
		bool bSelectedMaterialized = false;
		for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair :
			Items->GetAuthority().GetInstanceSnapshot())
		{
			const Fdemo_mapItemInstance& Instance = Pair.Value;
			if (Instance.RewardEventKind
				== Edemo_mapRewardEventKind::Jackpot)
			{
				++MaterializedJackpotCount;
				bAllOtherInstancesNormal &=
					Instance.RewardEventId.IsValid()
						&& Instance.RewardValueMultiplierBps == 60000
						&& !Instance.RewardSourceRoleId.IsNone();
				bSelectedMaterialized |=
					Instance.RewardEventId
							== Selected.RewardEventId
						&& Instance.RewardSourceRoleId
							== Selected.RewardSourceRoleId;
			}
			else
			{
				bAllOtherInstancesNormal &=
					Instance.RewardEventKind
							== Edemo_mapRewardEventKind::None
						&& !Instance.RewardEventId.IsValid()
						&& Instance.RewardValueMultiplierBps == 10000
						&& (Instance.RewardSourceRoleId.IsNone()
							|| Instance.RewardSourceRoleId.ToString().
								StartsWith(
									TEXT("P8.SourceRole."))
							|| Instance.RewardSourceRoleId
								== Fdemo_mapRewardSourceRoleIds::
									BossPrototype);
			}
		}
		FString AuthorityInvariantError;
		if (!bAllProjectionPlansValid
			|| TopologyHitCount < 1
			|| MaterializedJackpotCount != TopologyHitCount
			|| !bSelectedMaterialized
			|| !bAllOtherInstancesNormal
			|| !Items->ValidateInvariants(
				&AuthorityInvariantError))
		{
			Fail(FString::Printf(
				TEXT("single-hit or Item Authority invariant failed: %s"),
				*AuthorityInvariantError));
			return;
		}

		if (!OpenThroughProductInput(HitChest))
		{
			Fail(TEXT("real G input did not open the natural-hit Chest"));
			return;
		}
		Fdemo_mapRuntimeContainerEntrySnapshot IdentifiedHit;
		if (!SearchIdentify(
				HitChest,
				Selected.Section,
				Selected.SlotIndex,
				IdentifiedHit))
		{
			Fail(TEXT("real search did not identify the selected jackpot entry"));
			return;
		}
		const Fdemo_mapSearchContainerViewState SearchView =
			Fdemo_mapSearchContainerPresenter::Build(
				HitChest->GetContainerSnapshot());
		bool bVisibleJackpot = false;
		for (const auto& Section : SearchView.Sections)
		{
			if (const auto* Row = Section.Rows.FindByPredicate(
				[&Selected, &Section](const auto& Candidate)
				{
					return Section.Section == Selected.Section
						&& Candidate.SlotIndex == Selected.SlotIndex;
				}))
			{
				bVisibleJackpot =
					Row->Text.Contains(TEXT("JACKPOT ×6"))
					&& Row->Text.Contains(FString::Printf(
						TEXT("SELL=%lld"),
						IdentifiedHit.EffectiveStackSellValue));
			}
		}
		if (!bVisibleJackpot
			|| IdentifiedHit.RewardEventKind
				!= Edemo_mapRewardEventKind::Jackpot
			|| IdentifiedHit.RewardEventId
				!= Selected.RewardEventId
			|| IdentifiedHit.RewardValueMultiplierBps != 60000)
		{
			Fail(TEXT("SearchContainer did not visibly show JACKPOT ×6 and effective sell value"));
			return;
		}
		const FGuid MaterializedGuid = IdentifiedHit.ItemInstanceId;
		if (!SearchContainerWidget->AutomationClickEntry(
				Selected.Section,
				Selected.SlotIndex)
			|| !SearchContainerWidget->GetLastResult().bSuccess
			|| SearchContainerWidget->GetLastResult().ItemInstanceId
				!= MaterializedGuid)
		{
			Fail(TEXT("real Take did not preserve the materialized jackpot GUID"));
			return;
		}
		RewardJackpotTakenItemId = MaterializedGuid;
		const Fdemo_mapItemInstance* Taken =
			Items->GetAuthority().FindInstance(
				RewardJackpotTakenItemId);
		if (!Taken
			|| Taken->InstanceId != RewardJackpotTakenItemId
			|| Taken->OwnershipState
				!= Edemo_mapItemOwnershipState::Inventory
			|| Taken->RewardEventId != Selected.RewardEventId
			|| Taken->RewardValueMultiplierBps != 60000
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen
			|| !GetDemoController()
			|| !GetDemoController()->IsGameplayInputAllowed()
			|| GetDemoController()->IsMoveInputIgnored()
			|| GetDemoController()->IsLookInputIgnored())
		{
			Fail(TEXT("whole Take, UI close, or immediate gameplay input restoration failed"));
			return;
		}
		const int32 InstanceCountBeforeReopen =
			Items->GetAuthority().GetInstanceSnapshot().Num();
		MovePawnNear(HitChest);
		GetDemoController()->SetAutomationAimDirection(
			HitChest->GetActorLocation()
				- PlayerPawn->GetActorLocation());
		SetFocusedActor(HitChest);
		if (!GetDemoController()->
				DispatchAutomationKeyPressed(EKeys::G)
			|| !GetDemoController()->
				DispatchAutomationKeyReleased(EKeys::G)
			|| !bSearchContainerOpen
			|| ActiveSearchContainer.Get() != HitChest
			|| HitChest->GetLastProjectionResult().Trace.
				JackpotEventId != Selected.RewardEventId
			|| Items->GetAuthority().GetInstanceSnapshot().Num()
				!= InstanceCountBeforeReopen
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen)
		{
			Fail(TEXT("reopen changed projection decision, duplicated an instance, or failed UI close"));
			return;
		}

		if (!OpenThroughProductInput(MissChest))
		{
			Fail(TEXT("real G input did not open the natural-miss Chest"));
			return;
		}
		if (MissPlan.PlannedStacks.IsEmpty())
		{
			Fail(TEXT("natural-miss plan unexpectedly has no entry"));
			return;
		}
		const Fdemo_mapRewardPlannedStack& NormalStack =
			MissPlan.PlannedStacks[0];
		Fdemo_mapRuntimeContainerEntrySnapshot IdentifiedMiss;
		if (!SearchIdentify(
				MissChest,
				NormalStack.Section,
				NormalStack.SlotIndex,
				IdentifiedMiss)
			|| IdentifiedMiss.RewardEventKind
				!= Edemo_mapRewardEventKind::None
			|| IdentifiedMiss.RewardEventId.IsValid()
			|| IdentifiedMiss.RewardValueMultiplierBps != 10000
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen)
		{
			Fail(TEXT("real natural-miss source did not remain a normal reward"));
			return;
		}

		DestroyRuntimeContainers(TEXT("RewardJackpotBeforeExtraction"));
		Fdemo_mapSettlementSummary Summary;
		if (!Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				Summary).bSuccess)
		{
			Fail(TEXT("real Extraction settlement request failed"));
			return;
		}
		const Fdemo_mapProfileSessionSettlementResult Settlement =
			RewardGenerationProfileSession->
				CommitRuntimeSettlement(Summary);
		if (!Settlement.IsDurablySettled())
		{
			Fail(TEXT("Profile settlement did not durably commit the jackpot item"));
			return;
		}
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const Fdemo_mapProfileLoadResult Reloaded =
			Repository.LoadExistingProfile(Storage);
		FString ReloadedProfileError;
		const Fdemo_mapPersistentItemRecord* Persisted =
			Reloaded.Profile.PermanentStash.FindByPredicate(
				[this](const auto& Item)
				{
					return Item.ItemInstanceId
						== RewardJackpotTakenItemId;
				});
		if (!Reloaded.IsSuccess()
			|| !Repository.ValidateProfile(
				Reloaded.Profile,
				&ReloadedProfileError)
			|| !Persisted
			|| Persisted->RewardEventKind
				!= Edemo_mapRewardEventKind::Jackpot
			|| Persisted->RewardEventId != Selected.RewardEventId
			|| Persisted->RewardValueMultiplierBps != 60000
			|| Persisted->RewardSourceRoleId
				!= Selected.RewardSourceRoleId)
		{
			Fail(FString::Printf(
				TEXT("Profile reload did not preserve jackpot metadata, GUID, or invariant: %s"),
				*ReloadedProfileError));
			return;
		}

		const Fdemo_mapProfilePreparationSnapshot Preparation =
			RewardGenerationProfileSession->
				GetPreparationSnapshot();
		const Fdemo_mapProfilePreparationViewState PreparationView =
			Fdemo_mapProfilePreparationPresenter::BuildViewState(
				Preparation);
		const Fdemo_mapProfilePreparationRowView* JackpotRow =
			PreparationView.OrderedPermanentStashRows.
				FindByPredicate([this](const auto& Row)
				{
					return Row.ItemInstanceId
						== RewardJackpotTakenItemId;
				});
		const Fdemo_mapProfilePreparationRowView* NormalRow =
			PreparationView.OrderedPermanentStashRows.
				FindByPredicate([](const auto& Row)
				{
					return Row.RewardEventKind
							== Edemo_mapRewardEventKind::None
						&& Row.TotalSellPrice > 0;
				});
		const Fdemo_mapItemDefinition* NormalDefinition =
			NormalRow
				? Fdemo_mapItemDefinitions::Find(
					NormalRow->ItemDefinitionId)
				: nullptr;
		const bool bNormalQuoteUnchanged =
			NormalRow && NormalDefinition
			&& NormalRow->TotalSellPrice
				== NormalDefinition->SellPrice
					* static_cast<int64>(NormalRow->StackCount);
		const TArray<Fdemo_mapProfileShopCatalogRow> Catalog =
			Fdemo_mapProfileTradeTransaction::BuildCatalog();
		const Fdemo_mapProfileShopCatalogRow* Pill =
			Catalog.FindByPredicate([](const auto& Row)
				{
					return Row.ItemDefinitionId
						== Fdemo_mapItemIds::HealingPillLevel1;
				});
		int64 ExpectedSale = 0;
		if (!Fdemo_mapItemSellValueRules::TryCompute(
				Persisted->ItemDefinitionId,
				Persisted->StackCount,
				Persisted->RewardValueMultiplierBps,
				ExpectedSale)
			|| !JackpotRow
			|| JackpotRow->RewardLabel != TEXT("JACKPOT ×6")
			|| JackpotRow->TotalSellPrice != ExpectedSale
			|| !JackpotRow->SellDiagnostic.Contains(
				TEXT("Effective Sell"))
			|| !bNormalQuoteUnchanged
			|| !Pill
			|| Pill->BuyPrice != 30)
		{
			Fail(TEXT("Preparation Shop quote, normal quote, or buy catalog changed"));
			return;
		}

		const Fdemo_mapProfileSessionSnapshot BeforeSale =
			RewardGenerationProfileSession->GetSnapshot();
		Fdemo_mapProfileTradeIntent Sell;
		Sell.Kind = Edemo_mapProfileTradeKind::Sell;
		Sell.ExpectedProfileId = BeforeSale.ProfileId;
		Sell.ExpectedSaveGeneration =
			BeforeSale.SaveGeneration;
		Sell.ItemInstanceId = RewardJackpotTakenItemId;
		const Fdemo_mapProfileTradeResult Sale =
			RewardGenerationProfileSession->
				SubmitTradeIntent(Sell);
		const Fdemo_mapProfileLoadResult AfterSaleReload =
			Repository.LoadExistingProfile(Storage);
		FString AfterSaleProfileError;
		if (!Sale.IsCommitted()
			|| Sale.TotalPrice != ExpectedSale
			|| Sale.BalanceAfter
				!= Sale.BalanceBefore + ExpectedSale
			|| !AfterSaleReload.IsSuccess()
			|| !Repository.ValidateProfile(
				AfterSaleReload.Profile,
				&AfterSaleProfileError)
			|| AfterSaleReload.Profile.PersistentSpiritStones
				!= Sale.BalanceAfter
			|| AfterSaleReload.Profile.PermanentStash.
				ContainsByPredicate([this](const auto& Item)
				{
					return Item.ItemInstanceId
						== RewardJackpotTakenItemId;
				}))
		{
			Fail(TEXT("real Shop sale or post-sale Profile reload was not exact"));
			return;
		}

		GetWorldTimerManager().ClearTimer(AutomationTimer);
		if (Items.IsValid())
		{
			Items->TeardownWorld(GetWorld());
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P3_REWARD_JACKPOT_PRODUCT_EVIDENCE run=%s topology_chests=135 topology_hits=%d registry_hits=%d hit_role=%s hit_projection=%s hit_roll=%d miss_role=%s miss_projection=%s miss_roll=%d selected_index=%d guid=%s event=%s base=%lld bonus=%lld effective_sell=%lld balance=%lld input_restore=1 profile_reload=1 fallback=0."),
			*ActiveRunId.ToString(EGuidFormats::Digits),
			TopologyHitCount,
			ProjectionHitCount,
			*HitPlan.Trace.StableSourceRoleId.ToString(),
			*HitPlan.Trace.ProjectionId.ToString(),
			HitPlan.Trace.JackpotRoll,
			*MissPlan.Trace.StableSourceRoleId.ToString(),
			*MissPlan.Trace.ProjectionId.ToString(),
			MissPlan.Trace.JackpotRoll,
			SelectedIndex,
			*RewardJackpotTakenItemId.ToString(
				EGuidFormats::Digits),
			*Selected.RewardEventId.ToString(
				EGuidFormats::Digits),
			HitPlan.Trace.GeneratedTotalValue,
			HitPlan.Trace.JackpotBonusValue,
			ExpectedSale,
			Sale.BalanceAfter);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P3_REWARD_JACKPOT_PRODUCT_SMOKE: PASS."));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	void Ademo_mapV3ProgressionManager::RunSearchContainerAutomation()
{
	auto Fail = [this](const FString& Reason)
	{
		FinishSearchContainerAutomation(false, Reason);
	};
	auto Schedule = [this](float Delay)
	{
		++SearchContainerAutomationStep;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunSearchContainerAutomation,
			Delay,
			false);
	};
	auto FindPrototypeChest = [this]() -> Ademo_mapLootChest*
	{
		for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate : Chests)
		{
			if (Candidate.IsValid() && Candidate->IsP4PrototypeChest())
			{
				return Candidate.Get();
			}
		}
		return nullptr;
	};
	auto FindCorpse = [this]() -> Ademo_mapCorpseContainerActor*
	{
		for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Candidate : Corpses)
		{
			if (Candidate.IsValid())
			{
				return Candidate.Get();
			}
		}
		return nullptr;
	};
	auto FindEntry = [](
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex) -> const Fdemo_mapRuntimeContainerEntrySnapshot*
	{
		const Fdemo_mapRuntimeContainerSectionSnapshot* SectionSnapshot =
			Snapshot.Sections.FindByPredicate(
				[Section](const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
				{
					return Candidate.Section == Section;
				});
		return SectionSnapshot
			? SectionSnapshot->OrderedOccupiedEntries.FindByPredicate(
				[SlotIndex](const Fdemo_mapRuntimeContainerEntrySnapshot& Entry)
				{
					return Entry.SlotIndex == SlotIndex;
				})
			: nullptr;
	};
	Ademo_mapPlayerController* Controller = GetDemoController();
	APawn* Pawn = PlayerPawn.Get();
	if (!Controller || !Pawn || !Items.IsValid()
		|| !SearchContainerProfileSession.IsValid())
	{
		Fail(TEXT("controller, pawn, Runtime, or isolated Profile Session is unavailable"));
		return;
	}

	switch (SearchContainerAutomationStep)
	{
	case 0:
	{
		Ademo_mapLootChest* Chest = FindPrototypeChest();
		if (!Chest
			|| Items->GetRunState() != Edemo_mapRunState::Active
			|| !Items->GetActiveRunId().IsValid()
			|| Chest->IsContainerOpened()
			|| !MovePawnNear(Chest))
		{
			Fail(TEXT("normal prepared ActiveRun or product P4 Chest baseline is invalid"));
			return;
		}
		Controller->SetAutomationAimDirection(
			Chest->GetActorLocation() - Pawn->GetActorLocation());
		SetFocusedActor(Chest);
		if (!Controller->DispatchAutomationKeyPressed(EKeys::G)
			|| !Chest->IsContainerOpening()
			|| !Chest->HasActiveTimer())
		{
			Fail(TEXT("real G press did not begin Chest Opening"));
			return;
		}
		Schedule(0.25f);
		return;
	}
	case 1:
	{
		Ademo_mapLootChest* Chest = FindPrototypeChest();
		if (!Chest
			|| !Controller->DispatchAutomationKeyReleased(EKeys::G)
			|| Chest->IsContainerOpening()
			|| Chest->HasActiveTimer()
			|| Chest->GetContainerSnapshot().State
				!= Edemo_mapRuntimeContainerState::Closed
			|| !FMath::IsNearlyZero(Chest->GetCurrentActionProgress01()))
		{
			Fail(TEXT("G release did not cancel Opening back to Closed with zero progress"));
			return;
		}
		SetFocusedActor(Chest);
		if (!Controller->DispatchAutomationKeyPressed(EKeys::G)
			|| !Chest->IsContainerOpening())
		{
			Fail(TEXT("second real G press did not begin the full Chest hold"));
			return;
		}
		Schedule(1.15f);
		return;
	}
	case 2:
	{
		Ademo_mapLootChest* Chest = FindPrototypeChest();
		Controller->DispatchAutomationKeyReleased(EKeys::G);
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Chest ? Chest->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Wood =
			FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Chest,
				0);
		const Fdemo_mapRuntimeContainerEntrySnapshot* Pill =
			FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Chest,
				1);
		if (!Chest
			|| !Chest->IsContainerOpened()
			|| !bSearchContainerOpen
			|| !SearchContainerWidget
			|| !SearchContainerWidget->IsInViewport()
			|| SearchContainerWidget->GetRenderedSectionCount() != 1
			|| Snapshot.Sections.Num() != 1
			|| Snapshot.Sections[0].Capacity
				!= Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity
			|| Snapshot.Sections[0].OrderedOccupiedEntries.Num() != 2
			|| !Wood || !Pill
			|| Wood->State != Edemo_mapRuntimeContainerEntryState::Hidden
			|| Pill->State != Edemo_mapRuntimeContainerEntryState::Hidden
			|| Wood->ItemInstanceId.IsValid()
			|| !Wood->DefinitionId.IsNone()
			|| Wood->StackCount != 0)
		{
			Fail(TEXT("opened Chest UI, six-slot snapshot, or hidden redaction is invalid"));
			return;
		}
		if (!SearchContainerWidget->AutomationClickEntry(
			Edemo_mapRuntimeContainerSection::Chest,
			0)
			|| !Chest->IsContainerSearching())
		{
			Fail(TEXT("real Chest Widget did not forward the SpiritWood Search intent"));
			return;
		}
		Schedule(0.85f);
		return;
	}
	case 3:
	{
		Ademo_mapLootChest* Chest = FindPrototypeChest();
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Chest ? Chest->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Wood =
			FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Chest,
				0);
		if (!Chest || !Wood
			|| Wood->State != Edemo_mapRuntimeContainerEntryState::Identified
			|| Wood->DefinitionId != Fdemo_mapItemIds::SpiritWoodLevel1
			|| Wood->StackCount != 2
			|| !Wood->ItemInstanceId.IsValid())
		{
			Fail(TEXT("Chest SpiritWood did not identify after exactly the timed Search path"));
			return;
		}
		SearchContainerChestTakenId = Wood->ItemInstanceId;
		if (!SearchContainerWidget->AutomationClickEntry(
			Edemo_mapRuntimeContainerSection::Chest,
			0))
		{
			Fail(TEXT("real Chest Widget did not forward the Take intent"));
			return;
		}
		const Fdemo_mapItemInstance* Taken =
			Items->GetAuthority().FindInstance(SearchContainerChestTakenId);
		if (!Taken
			|| Taken->OwnershipState != Edemo_mapItemOwnershipState::Inventory
			|| Taken->Quantity != 2
			|| Items->GetAuthority().FindInventorySlot(
				SearchContainerChestTakenId) == INDEX_NONE)
		{
			Fail(TEXT("Chest Take did not atomically preserve the SpiritWood GUID and full Stack"));
			return;
		}
		Ademo_mapEnemyCharacter* Melee = nullptr;
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead())
			{
				Melee = *It;
				break;
			}
		}
		if (!Melee)
		{
			Fail(TEXT("normal product melee Enemy is unavailable"));
			return;
		}
		UGameplayStatics::ApplyDamage(
			Melee,
			1000.0f,
			Controller,
			Pawn,
			nullptr);
		Ademo_mapCorpseContainerActor* Corpse = FindCorpse();
		if (!Melee->IsDead()
			|| !Corpse
			|| Corpse->GetLootSourceId() != Melee->GetLootSourceId())
		{
			Fail(TEXT("real melee death callback did not create the P4 Corpse"));
			return;
		}
		CloseSearchContainer(TEXT("SwitchToCorpse"), true);
		MovePawnNear(Corpse);
		SetFocusedActor(Corpse);
		if (!Corpse->CanInteract(Controller))
		{
			Fail(TEXT("product P4 Corpse was not interactable after normal range positioning"));
			return;
		}
		Controller->SetAutomationAimDirection(
			Corpse->GetActorLocation() - Pawn->GetActorLocation());
		SetFocusedActor(Corpse);
		if (!Controller->DispatchAutomationKeyPressed(EKeys::G)
			|| !Corpse->IsContainerOpening())
		{
			Fail(TEXT("real G press did not begin Corpse Opening"));
			return;
		}
		Schedule(1.15f);
		return;
	}
	case 4:
	{
		Ademo_mapCorpseContainerActor* Corpse = FindCorpse();
		Controller->DispatchAutomationKeyReleased(EKeys::G);
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Corpse ? Corpse->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Weapon =
			FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Equipment,
				0);
		if (!Corpse
			|| !Corpse->IsContainerOpened()
			|| !bSearchContainerOpen
			|| !SearchContainerWidget
			|| SearchContainerWidget->GetRenderedSectionCount() != 3
			|| Snapshot.Sections.Num() != 3
			|| Snapshot.Sections[0].Section
				!= Edemo_mapRuntimeContainerSection::Equipment
			|| Snapshot.Sections[1].Section
				!= Edemo_mapRuntimeContainerSection::Backpack
			|| Snapshot.Sections[2].Section
				!= Edemo_mapRuntimeContainerSection::Body
			|| Snapshot.Sections[0].Capacity != 4
			|| !Weapon
			|| Weapon->State != Edemo_mapRuntimeContainerEntryState::Identified
			|| Weapon->DefinitionId != Fdemo_mapItemIds::WeaponLevel1)
		{
			Fail(TEXT("Corpse tabs, stable Equipment capacity/order, or immediate identification is invalid"));
			return;
		}
		SearchContainerWeaponId = Weapon->ItemInstanceId;
		if (!SearchContainerWidget->AutomationClickEntry(
			Edemo_mapRuntimeContainerSection::Equipment,
			0))
		{
			Fail(TEXT("real Corpse Widget did not take the Equipment Weapon"));
			return;
		}
		const Fdemo_mapItemInstance* WeaponItem =
			Items->GetAuthority().FindInstance(SearchContainerWeaponId);
		if (!WeaponItem
			|| WeaponItem->OwnershipState != Edemo_mapItemOwnershipState::Inventory
			|| Items->GetAuthority().GetEquippedInstance(
				Fdemo_mapItemIds::WeaponSlot).IsValid())
		{
			Fail(TEXT("Weapon Take changed GUID or auto-equipped"));
			return;
		}
		if (!SearchContainerWidget->AutomationClickEntry(
			Edemo_mapRuntimeContainerSection::Backpack,
			0)
			|| !Corpse->IsContainerSearching())
		{
			Fail(TEXT("real Corpse Widget did not begin the 1.0 second Backpack Search"));
			return;
		}
		Schedule(1.10f);
		return;
	}
	case 5:
	{
		Ademo_mapCorpseContainerActor* Corpse = FindCorpse();
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Corpse ? Corpse->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Ore =
			FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Backpack,
				0);
		if (!Corpse || !Ore
			|| Ore->State != Edemo_mapRuntimeContainerEntryState::Identified
			|| Ore->DefinitionId != Fdemo_mapItemIds::SpiritOreLevel1
			|| Ore->StackCount != 2
			|| !SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Body,
				0)
			|| !Corpse->IsContainerSearching())
		{
			Fail(TEXT("Backpack Search result or 1.5 second Body Search start is invalid"));
			return;
		}
		Schedule(1.60f);
		return;
	}
	case 6:
	{
		Ademo_mapCorpseContainerActor* Corpse = FindCorpse();
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Corpse ? Corpse->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* SoulBone =
			FindEntry(
				Snapshot,
				Edemo_mapRuntimeContainerSection::Body,
				0);
		if (!Corpse || !SoulBone
			|| SoulBone->State != Edemo_mapRuntimeContainerEntryState::Identified
			|| SoulBone->DefinitionId != Fdemo_mapItemIds::SoulBone
			|| SoulBone->StackCount != 1)
		{
			Fail(TEXT("Corpse Body SoulBone did not identify after the 1.5 second Search"));
			return;
		}
		SearchContainerSoulBoneId = SoulBone->ItemInstanceId;
		if (!SearchContainerWidget->AutomationClickEntry(
			Edemo_mapRuntimeContainerSection::Body,
			0)
			|| !SearchContainerWidget->AutomationClickClose())
		{
			Fail(TEXT("real Corpse Widget did not Take SoulBone and actively close"));
			return;
		}
		Schedule(0.20f);
		return;
	}
	case 7:
	{
		const Fdemo_mapItemInstance* ChestItem =
			Items->GetAuthority().FindInstance(SearchContainerChestTakenId);
		const Fdemo_mapItemInstance* WeaponItem =
			Items->GetAuthority().FindInstance(SearchContainerWeaponId);
		const Fdemo_mapItemInstance* SoulBoneItem =
			Items->GetAuthority().FindInstance(SearchContainerSoulBoneId);
		const bool bHotbarEmpty =
			!Items->GetHotbarBindingSnapshot().SlotBindings.ContainsByPredicate(
				[](const FGuid& Id) { return Id.IsValid(); });
		if (bSearchContainerOpen
			|| SearchContainerWidget->IsInViewport()
			|| Controller->IsSearchContainerInputLocked()
			|| !Controller->IsGameplayInputAllowed()
			|| Controller->IsMoveInputIgnored()
			|| Controller->IsLookInputIgnored()
			|| !ChestItem || !WeaponItem || !SoulBoneItem
			|| ChestItem->OwnershipState != Edemo_mapItemOwnershipState::Inventory
			|| WeaponItem->OwnershipState != Edemo_mapItemOwnershipState::Inventory
			|| SoulBoneItem->OwnershipState != Edemo_mapItemOwnershipState::Inventory
			|| !bHotbarEmpty)
		{
			Fail(TEXT("active close did not restore Gameplay input or Take introduced side effects"));
			return;
		}
		TArray<TWeakObjectPtr<Ademo_mapLootChest>> ChestActors = Chests;
		TArray<TWeakObjectPtr<Ademo_mapCorpseContainerActor>> CorpseActors = Corpses;
		DestroyRuntimeContainers(TEXT("SearchContainerSmokeReset"));
		if (!Chests.IsEmpty() || !Corpses.IsEmpty()
			|| bSearchContainerOpen
			|| ActiveSearchContainer.IsValid()
			|| (SearchContainerWidget && SearchContainerWidget->IsInViewport())
			|| ChestActors.ContainsByPredicate(
				[](const TWeakObjectPtr<Ademo_mapLootChest>& Actor)
				{
					return Actor.IsValid() && Actor->HasActiveTimer();
				})
			|| CorpseActors.ContainsByPredicate(
				[](const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Actor)
				{
					return Actor.IsValid() && Actor->HasActiveTimer();
				}))
		{
			Fail(TEXT("Run Reset left Container actors, timer, widget, or active identity residue"));
			return;
		}
		Fdemo_mapSettlementSummary Summary;
		const Fdemo_mapItemOperationResult RuntimeSettlement =
			Items->RequestSettlement(
				Edemo_mapRunEndReason::Abandon,
				Summary);
		const Fdemo_mapProfileSessionSettlementResult PersistentSettlement =
			RuntimeSettlement.bSuccess
				? SearchContainerProfileSession->CommitRuntimeSettlement(Summary)
				: Fdemo_mapProfileSessionSettlementResult();
		const Fdemo_mapProfileSessionSnapshot FinalProfile =
			SearchContainerProfileSession->GetSnapshot();
		const FString ProductionRoot =
			Fdemo_mapProfileStorageContext::Production().RootDirectory;
		if (!RuntimeSettlement.bSuccess
			|| !PersistentSettlement.IsDurablySettled()
			|| FinalProfile.SessionState
				!= Edemo_mapProfileSessionState::ReadyForPreparation
			|| FinalProfile.SaveGeneration != 3
			|| FinalProfile.PersistentSpiritStones != 0
			|| FinalProfile.RiskSpiritStones != 0
			|| FinalProfile.LastTerminalReason != Edemo_mapRunEndReason::Abandon
			|| IFileManager::Get().DirectoryExists(*ProductionRoot))
		{
			Fail(FString::Printf(
				TEXT("isolated Profile settlement proof failed runtime=%d persistent=%d state=%d generation=%d persistent_stones=%lld risk_stones=%lld active_run=%d terminal=%d production_exists=%d"),
				RuntimeSettlement.bSuccess,
				PersistentSettlement.IsDurablySettled(),
				static_cast<int32>(FinalProfile.SessionState),
				FinalProfile.SaveGeneration,
				FinalProfile.PersistentSpiritStones,
				FinalProfile.RiskSpiritStones,
				FinalProfile.ActiveRunId.IsValid(),
				static_cast<int32>(FinalProfile.LastTerminalReason),
				IFileManager::Get().DirectoryExists(*ProductionRoot)));
			return;
		}
		Items->TeardownWorld(GetWorld());
		if (Ademo_mapGameMode* Mode =
			Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()))
		{
			Mode->DeactivateV3MissionContentForPreparation();
		}
		FinishSearchContainerAutomation(true, FString());
		return;
	}
	default:
		Fail(TEXT("unexpected SearchContainer automation step"));
		return;
	}
}

void Ademo_mapV3ProgressionManager::RunRewardGenerationAutomation()
{
	auto Fail = [this](const FString& Reason)
	{
		FinishRewardGenerationAutomation(false, Reason);
	};
	auto Schedule = [this](float Delay)
	{
		++RewardGenerationAutomationStep;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::RunRewardGenerationAutomation,
			Delay,
			false);
	};
	auto RepresentativeHighValueSlot = []()
		-> const Fdemo_mapFullMapRewardSlot*
	{
		const Fdemo_mapFullMapRewardSlot* Selected = nullptr;
		for (const Fdemo_mapFullMapRewardSlot& Slot :
			Fdemo_mapRewardFullMapDistribution::GetSlots())
		{
			if (Slot.RewardClass
					!= Edemo_mapFullMapRewardClass::
						ContainerHighValue)
			{
				continue;
			}
			if (!Selected
				|| Slot.SlotId.ToString()
					< Selected->SlotId.ToString())
			{
				Selected = &Slot;
			}
		}
		return Selected;
	};
	auto RepresentativeHighValueChest =
		[this, &RepresentativeHighValueSlot]()
		-> Ademo_mapLootChest*
	{
		const Fdemo_mapFullMapRewardSlot* Slot =
			RepresentativeHighValueSlot();
		if (!Slot)
		{
			return nullptr;
		}
		for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate :
			Chests)
		{
			if (Candidate.IsValid()
				&& Candidate->GetRewardSourceId()
					== Slot->StableSourceRoleId)
			{
				return Candidate.Get();
			}
		}
		return nullptr;
	};
	auto FindEntry = [](
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		int32 SlotIndex)
		-> const Fdemo_mapRuntimeContainerEntrySnapshot*
	{
		const Fdemo_mapRuntimeContainerSectionSnapshot* Section =
			Snapshot.Sections.FindByPredicate(
				[](const auto& Candidate)
				{
					return Candidate.Section ==
						Edemo_mapRuntimeContainerSection::Chest;
				});
		return Section
			? Section->OrderedOccupiedEntries.FindByPredicate(
				[SlotIndex](const auto& Entry)
				{
					return Entry.SlotIndex == SlotIndex;
				})
			: nullptr;
	};
	Ademo_mapPlayerController* Controller = GetDemoController();
	APawn* Pawn = PlayerPawn.Get();
	if (!Controller
		|| !Pawn
		|| !Items.IsValid()
		|| !RewardGenerationProfileSession.IsValid())
	{
		Fail(TEXT("controller, pawn, Item runtime, or isolated Profile Session is unavailable"));
		return;
	}

	switch (RewardGenerationAutomationStep)
	{
	case 0:
	{
		FString PolicyError;
		const bool bPolicyValid =
			Fdemo_mapRewardFullMapDistribution::Validate(
				&PolicyError);
		const Fdemo_mapFullMapDistributionCounts Counts =
			Fdemo_mapRewardFullMapDistribution::Count();
		const Fdemo_mapFullMapRewardSlot* SelectedSlot =
			RepresentativeHighValueSlot();
		Ademo_mapLootChest* Chest =
			RepresentativeHighValueChest();
		const Fdemo_mapRewardGenerationResult* Generated =
			Chest ? &Chest->GetLastRewardGenerationResult()
				: nullptr;
		int32 HighValueLedgerCount = 0;
		int32 HighValueInitializedCount = 0;
		for (const Fdemo_mapFullMapRewardSlot& Slot :
			Fdemo_mapRewardFullMapDistribution::GetSlots())
		{
			if (Slot.RewardClass
				!= Edemo_mapFullMapRewardClass::
					ContainerHighValue)
			{
				continue;
			}
			++HighValueLedgerCount;
			Ademo_mapLootChest* SlotChest = nullptr;
			for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate :
				Chests)
			{
				if (Candidate.IsValid()
					&& Candidate->GetRewardSourceId()
						== Slot.StableSourceRoleId)
				{
					SlotChest = Candidate.Get();
					break;
				}
			}
			const bool bSlotInitialized =
				SlotChest
				&& SlotChest->GetRewardSourceMode()
					== Edemo_mapRewardSourceMode::
						GeneratedReward
				&& SlotChest->GetRewardProjectionId()
					== Slot.ProjectionId
				&& SlotChest->GetRewardBudgetProfileId()
					== Slot.BudgetProfileId
				&& SlotChest->GetLastProjectionResult().
					IsSuccess()
				&& !SlotChest->UsedFixedFallback()
				&& !CanGenerateRewardSource(
					Items->GetActiveRunId(),
					Slot.StableSourceRoleId);
			HighValueInitializedCount += bSlotInitialized ? 1 : 0;
			const Fdemo_mapRewardSourceProjectionResult* Plan =
				SlotChest
					? &SlotChest->GetLastProjectionResult()
					: nullptr;
			UE_LOG(
				Logdemo_map,
				Log,
				TEXT("P1_HIGHVALUE_INITIALIZATION_LEDGER slot=%s role=%s projection=%s profile=%s base=%lld actor=%s initialized=%d mode=%s plan=%s stacks=%d generated=%lld residual=%lld commit=%d fallback=%d."),
				*Slot.SlotId.ToString(),
				*Slot.StableSourceRoleId.ToString(),
				*Slot.ProjectionId.ToString(),
				*Slot.BudgetProfileId.ToString(),
				Slot.BaseSourceValue,
				SlotChest
					? *SlotChest->GetPathName()
					: TEXT("NONE"),
				bSlotInitialized ? 1 : 0,
				SlotChest
					&& SlotChest->GetRewardSourceMode()
						== Edemo_mapRewardSourceMode::
							GeneratedReward
					? TEXT("GeneratedReward")
					: TEXT("Invalid"),
				Plan && Plan->IsSuccess()
					? TEXT("Success")
					: TEXT("Failure"),
				Plan ? Plan->PlannedStacks.Num() : 0,
				Plan ? Plan->Trace.GeneratedTotalValue : 0,
				Plan ? Plan->Trace.ResidualValue : 0,
				bSlotInitialized ? 1 : 0,
				SlotChest && SlotChest->UsedFixedFallback() ? 1 : 0);
		}
		if (!bPolicyValid
			|| Counts.HighValueContainers
				!= Fdemo_mapRewardFullMapDistribution::
					HighValueContainerCount
			|| HighValueLedgerCount
				!= Fdemo_mapRewardFullMapDistribution::
					HighValueContainerCount
			|| HighValueInitializedCount
				!= Fdemo_mapRewardFullMapDistribution::
					HighValueContainerCount
			|| !SelectedSlot
			|| !Chest
			|| Items->GetRunState() != Edemo_mapRunState::Active
			|| Items->GetActiveRunId() != Chest->GetOwningRunId()
			|| Chest->GetRewardSourceMode() !=
				Edemo_mapRewardSourceMode::GeneratedReward
			|| Chest->GetRewardSourceId() !=
				SelectedSlot->StableSourceRoleId
			|| Chest->GetRewardProjectionId() !=
				SelectedSlot->ProjectionId
			|| Chest->GetRewardBudgetProfileId() !=
				SelectedSlot->BudgetProfileId
			|| SelectedSlot->BaseSourceValue != 2000
			|| Chest->UsedFixedFallback()
			|| !Generated
			|| !Generated->IsSuccess()
			|| Generated->PlannedStacks.IsEmpty()
			|| Generated->Trace.GeneratedTotalValue >
				Generated->Trace.RandomizedBudget
			|| CanGenerateRewardSource(
				Items->GetActiveRunId(),
				Chest->GetRewardSourceId())
			|| !MovePawnNear(Chest))
		{
			Fail(FString::Printf(
				TEXT("current P8 HighValue representative initialization failed policy=%d policy_error=%s declared=%d ledger=%d initialized=%d selected=%s"),
				bPolicyValid ? 1 : 0,
				*PolicyError,
				Counts.HighValueContainers,
				HighValueLedgerCount,
				HighValueInitializedCount,
				SelectedSlot
					? *SelectedSlot->SlotId.ToString()
					: TEXT("NONE")));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P1_REWARD_GENERATION_CURRENT_TOPOLOGY declared=15 initialized=15 selected_slot=%s selected_role=%s selected_projection=%s selected_profile=%s selected_base=%lld actor=%s."),
			*SelectedSlot->SlotId.ToString(),
			*SelectedSlot->StableSourceRoleId.ToString(),
			*SelectedSlot->ProjectionId.ToString(),
			*SelectedSlot->BudgetProfileId.ToString(),
			SelectedSlot->BaseSourceValue,
			*Chest->GetPathName());
		Controller->SetAutomationAimDirection(
			Chest->GetActorLocation() - Pawn->GetActorLocation());
		SetFocusedActor(Chest);
		if (!Controller->DispatchAutomationKeyPressed(EKeys::G)
			|| !Chest->IsContainerOpening()
			|| !Chest->HasActiveTimer())
		{
			Fail(TEXT("real G press did not begin generated Chest Opening"));
			return;
		}
		Schedule(1.15f);
		return;
	}
	case 1:
	{
		Ademo_mapLootChest* Chest =
			RepresentativeHighValueChest();
		Controller->DispatchAutomationKeyReleased(EKeys::G);
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Chest ? Chest->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
			FindEntry(Snapshot, 0);
		if (!Chest
			|| !Chest->IsContainerOpened()
			|| !bSearchContainerOpen
			|| !SearchContainerWidget
			|| !SearchContainerWidget->IsInViewport()
			|| Snapshot.Sections.Num() != 1
			|| Snapshot.Sections[0].Capacity !=
				Fdemo_mapSearchContainerPrototypeConfig::
					ChestPrototypeCapacity
			|| !Entry
			|| Entry->State !=
				Edemo_mapRuntimeContainerEntryState::Hidden
			|| Entry->ItemInstanceId.IsValid()
			|| !Entry->DefinitionId.IsNone()
			|| !SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Chest,
				0)
			|| !Chest->IsContainerSearching())
		{
			Fail(TEXT("generated Chest did not open through real product UI with redacted hidden rewards"));
			return;
		}
		Schedule(
			Fdemo_mapSearchContainerPrototypeConfig::
				ChestEntrySearchSeconds + 0.10f);
		return;
	}
	case 2:
	{
		Ademo_mapLootChest* Chest =
			RepresentativeHighValueChest();
		const Fdemo_mapRuntimeContainerSnapshot Snapshot =
			Chest ? Chest->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
			FindEntry(Snapshot, 0);
		if (!Chest
			|| !Entry
			|| Entry->State !=
				Edemo_mapRuntimeContainerEntryState::Identified
			|| !Entry->ItemInstanceId.IsValid()
			|| Entry->DefinitionId.IsNone()
			|| Entry->StackCount <= 0)
		{
			Fail(TEXT("generated reward did not identify after the existing timed Search"));
			return;
		}
		RewardGenerationTakenItemId = Entry->ItemInstanceId;
		RewardGenerationTakenDefinitionId = Entry->DefinitionId;
		RewardGenerationTakenQuantity = Entry->StackCount;
		if (!SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Chest,
				0)
			|| !SearchContainerWidget->GetLastResult().bSuccess
			|| SearchContainerWidget->GetLastResult().ItemInstanceId
				!= RewardGenerationTakenItemId
			|| !SearchContainerWidget->AutomationClickClose())
		{
			Fail(TEXT("generated reward Take/Close did not traverse the real Widget intent path"));
			return;
		}
		Schedule(0.20f);
		return;
	}
	case 3:
	{
		const Fdemo_mapFullMapRewardSlot* SelectedSlot =
			RepresentativeHighValueSlot();
		Ademo_mapLootChest* Chest =
			RepresentativeHighValueChest();
		const Fdemo_mapItemInstance* Taken =
			Items->GetAuthority().FindInstance(
				RewardGenerationTakenItemId);
		FString InvariantError;
		if (!Chest
			|| bSearchContainerOpen
			|| (SearchContainerWidget
				&& SearchContainerWidget->IsInViewport())
			|| Controller->IsSearchContainerInputLocked()
			|| !Controller->IsGameplayInputAllowed()
			|| Controller->IsMoveInputIgnored()
			|| Controller->IsLookInputIgnored()
			|| !Taken
			|| Taken->InstanceId != RewardGenerationTakenItemId
			|| Taken->DefinitionId !=
				RewardGenerationTakenDefinitionId
			|| Taken->Quantity != RewardGenerationTakenQuantity
			|| Taken->OwnershipState !=
				Edemo_mapItemOwnershipState::Inventory
			|| Items->GetAuthority().FindInventorySlot(
				RewardGenerationTakenItemId) == INDEX_NONE
			|| CanGenerateRewardSource(
				Items->GetActiveRunId(),
				Chest->GetRewardSourceId())
			|| !Items->ValidateInvariants(&InvariantError))
		{
			Fail(TEXT("same-GUID Take, duplicate-source guard, input restore, or Item invariants failed"));
			return;
		}

		const Fdemo_mapRewardGenerationTrace Trace =
			Chest->GetLastRewardGenerationResult().Trace;
		const int32 PlannedStackCount =
			Chest->GetLastRewardGenerationResult().
				PlannedStacks.Num();
		if (!MovePawnNear(Chest))
		{
			Fail(TEXT("selected representative could not be refocused for reopen"));
			return;
		}
		Controller->SetAutomationAimDirection(
			Chest->GetActorLocation() - Pawn->GetActorLocation());
		SetFocusedActor(Chest);
		if (!Controller->DispatchAutomationKeyPressed(EKeys::G)
			|| (Chest->IsContainerOpening()
				&& !Chest->CompleteActionForAutomation().bSuccess)
			|| !Controller->DispatchAutomationKeyReleased(EKeys::G)
			|| !Chest->IsContainerOpened()
			|| !bSearchContainerOpen
			|| ActiveSearchContainer.Get() != Chest
			|| !SearchContainerWidget
			|| Chest->GetLastRewardGenerationResult().
				Trace.EffectiveSeed != Trace.EffectiveSeed
			|| Chest->GetLastRewardGenerationResult().
				PlannedStacks.Num() != PlannedStackCount
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen
			|| !Controller->IsGameplayInputAllowed()
			|| CanGenerateRewardSource(
				Items->GetActiveRunId(),
				Chest->GetRewardSourceId()))
		{
			Fail(TEXT("reopen/no-reroll, duplicate-source guard, or second input restore failed"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P1_REWARD_GENERATION_PRODUCT_EVIDENCE run=%s request=%s slot=%s source=%s projection=%s profile=%s base=%lld actor=%s seed=%llu budget=%lld generated=%lld residual=%lld stacks=%d take_guid=%s take_definition=%s take_quantity=%d fallback=0 same_guid=1 source_commit=1 reopen_reroll=0 input_restored=1."),
			*Trace.RunId.ToString(EGuidFormats::Digits),
			*Trace.RequestId.ToString(),
			SelectedSlot
				? *SelectedSlot->SlotId.ToString()
				: TEXT("NONE"),
			*Trace.LootSourceId.ToString(),
			SelectedSlot
				? *SelectedSlot->ProjectionId.ToString()
				: TEXT("NONE"),
			*Trace.BudgetProfileId.ToString(),
			SelectedSlot ? SelectedSlot->BaseSourceValue : 0,
			*Chest->GetPathName(),
			static_cast<unsigned long long>(Trace.EffectiveSeed),
			Trace.RandomizedBudget,
			Trace.GeneratedTotalValue,
			Trace.ResidualValue,
			Chest->GetLastRewardGenerationResult().PlannedStacks.Num(),
			*RewardGenerationTakenItemId.ToString(
				EGuidFormats::Digits),
			*RewardGenerationTakenDefinitionId.ToString(),
			RewardGenerationTakenQuantity);

		DestroyRuntimeContainers(
			TEXT("RewardGenerationProductSmokeCleanup"));
		Fdemo_mapSettlementSummary Summary;
		const Fdemo_mapItemOperationResult RuntimeSettlement =
			Items->RequestSettlement(
				Edemo_mapRunEndReason::Extraction,
				Summary);
		const Fdemo_mapProfileSessionSettlementResult
			PersistentSettlement =
				RuntimeSettlement.bSuccess
					? RewardGenerationProfileSession
						->CommitRuntimeSettlement(Summary)
					: Fdemo_mapProfileSessionSettlementResult();
		const Fdemo_mapProfileSessionSnapshot FinalProfile =
			RewardGenerationProfileSession->GetSnapshot();
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(
				RewardGenerationStorageRoot);
		const Fdemo_mapProfileLoadResult Reload =
			Repository.LoadExistingProfile(Storage);
		const Fdemo_mapPersistentItemRecord* Persisted =
			Reload.IsSuccess()
				? Reload.Profile.PermanentStash.FindByPredicate(
					[this](const auto& Item)
					{
						return Item.ItemInstanceId
							== RewardGenerationTakenItemId;
					})
				: nullptr;
		if (!RuntimeSettlement.bSuccess
			|| !PersistentSettlement.IsDurablySettled()
			|| FinalProfile.SessionState !=
				Edemo_mapProfileSessionState::ReadyForPreparation
			|| FinalProfile.LastTerminalReason !=
				Edemo_mapRunEndReason::Extraction
			|| !Reload.IsSuccess()
			|| Reload.Profile.ProfileId != FinalProfile.ProfileId
			|| !Persisted
			|| Persisted->ItemDefinitionId
				!= RewardGenerationTakenDefinitionId
			|| Persisted->RewardSourceRoleId
				!= Trace.LootSourceId
			|| !Chests.IsEmpty()
			|| !Corpses.IsEmpty())
		{
			Fail(TEXT("isolated Profile settlement or Runtime Container cleanup failed"));
			return;
		}
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("P1_REWARD_GENERATION_SETTLEMENT_EVIDENCE extraction=1 settlement=1 reload=1 profile=%s retained_guid=%s retained_definition=%s retained_role=%s normal_exit=1."),
			*FinalProfile.ProfileId.ToString(
				EGuidFormats::DigitsWithHyphens),
			*Persisted->ItemInstanceId.ToString(
				EGuidFormats::DigitsWithHyphens),
			*Persisted->ItemDefinitionId.ToString(),
			*Persisted->RewardSourceRoleId.ToString());
		Items->TeardownWorld(GetWorld());
		if (Ademo_mapGameMode* Mode =
			Cast<Ademo_mapGameMode>(
				GetWorld()->GetAuthGameMode()))
		{
			Mode->DeactivateV3MissionContentForPreparation();
		}
		FinishRewardGenerationAutomation(true, FString());
		return;
	}
	default:
		Fail(TEXT("unexpected Reward Generation automation state"));
		return;
	}
}

void Ademo_mapV3ProgressionManager::RunProfileTradeAutomation()
{
	if (!ProfilePreparationFlow || !ProfilePreparationFlow->GetSession() || !ProfilePreparationWidget)
	{
		FailAutomation(TEXT("PROFILE_TRADE_SMOKE: FAIL: Preparation flow, session, or widget is absent."));
		return;
	}
	Udemo_mapProfileSessionSubsystem* Session = ProfilePreparationFlow->GetSession();
	const Fdemo_mapProfileSessionSnapshot Initial = Session->GetSnapshot();
	ProfilePreparationWidget->RefreshFromSession();
	if (ProfileTradeAutomationPhase == Edemo_mapProfileTradeAutomationPhase::Trade)
	{
		const FGuid Blade = Initial.OrderedPermanentStash.ContainsByPredicate(
			[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade; })
			? Initial.OrderedPermanentStash.FindByPredicate(
				[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade; })->ItemInstanceId
			: FGuid();
		const FGuid Vest = Initial.OrderedPermanentStash.ContainsByPredicate(
			[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest; })
			? Initial.OrderedPermanentStash.FindByPredicate(
				[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest; })->ItemInstanceId
			: FGuid();
		const FGuid Talisman = Initial.OrderedPermanentStash.ContainsByPredicate(
			[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman; })
			? Initial.OrderedPermanentStash.FindByPredicate(
				[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman; })->ItemInstanceId
			: FGuid();
		if (Initial.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
			|| Initial.SaveGeneration != 1
			|| Initial.PersistentSpiritStones != 0
			|| Initial.RiskSpiritStones != 0
			|| Initial.OrderedPermanentStash.Num() != 3
			|| !Blade.IsValid() || !Vest.IsValid() || !Talisman.IsValid())
		{
			FailAutomation(TEXT("PROFILE_TRADE_PHASE_A: FAIL: fresh baseline mismatch."));
			return;
		}
		const Fdemo_mapProfileTradeResult SellBlade = ProfilePreparationWidget->RequestSell(Blade);
		const Fdemo_mapProfileTradeResult SellVest = ProfilePreparationWidget->RequestSell(Vest);
		const Fdemo_mapProfileTradeResult BuyPill = ProfilePreparationWidget->RequestBuy(Fdemo_mapItemIds::HealingPillLevel1);
		const Fdemo_mapProfileSessionSnapshot Final = Session->GetSnapshot();
		const bool bTalismanSame = Final.OrderedPermanentStash.ContainsByPredicate(
			[&Talisman](const Fdemo_mapPersistentItemRecord& I){ return I.ItemInstanceId == Talisman && I.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman; });
		const bool bPillExact = Final.OrderedPermanentStash.ContainsByPredicate(
			[&BuyPill](const Fdemo_mapPersistentItemRecord& I){ return I.ItemInstanceId == BuyPill.ItemInstanceId && I.ItemDefinitionId == Fdemo_mapItemIds::HealingPillLevel1 && I.StackCount == 1; });
		if (!SellBlade.IsCommitted() || !SellVest.IsCommitted() || !BuyPill.IsCommitted()
			|| Final.ProfileId != Initial.ProfileId || Final.SaveGeneration != 4
			|| Final.PersistentSpiritStones != 20 || Final.RiskSpiritStones != 0
			|| Final.ActiveRunId.IsValid() || Final.OrderedPermanentStash.Num() != 2
			|| !bTalismanSame || !bPillExact)
		{
			FailAutomation(TEXT("PROFILE_TRADE_PHASE_A: FAIL: scripted trade closure mismatch."));
			return;
		}
		const FString Handoff = FString::Printf(TEXT("%s\n%s\n%s\n"),
			*Final.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*BuyPill.ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Talisman.ToString(EGuidFormats::DigitsWithHyphens));
		if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(ProfileTradeHandoffPath), true)
			|| !FFileHelper::SaveStringToFile(Handoff, *ProfileTradeHandoffPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			FailAutomation(TEXT("PROFILE_TRADE_PHASE_A: FAIL: test-only handoff write failed."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("PROFILE_TRADE_PHASE_A_IDENTITY profile=%s pill=%s talisman=%s balance=20 generation=4 catalog=7."),
			*Final.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*BuyPill.ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Talisman.ToString(EGuidFormats::DigitsWithHyphens));
		PassAutomation(TEXT("PROFILE_TRADE_PHASE_A: PASS."));
		return;
	}

	FString Handoff;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToString(Handoff, *ProfileTradeHandoffPath))
	{
		FailAutomation(TEXT("PROFILE_TRADE_PHASE_B_RELOAD: FAIL: handoff read failed."));
		return;
	}
	Handoff.ParseIntoArrayLines(Lines, true);
	FGuid ExpectedProfile, ExpectedPill, ExpectedTalisman;
	if (Lines.Num() != 3
		|| !FGuid::Parse(Lines[0], ExpectedProfile)
		|| !FGuid::Parse(Lines[1], ExpectedPill)
		|| !FGuid::Parse(Lines[2], ExpectedTalisman))
	{
		FailAutomation(TEXT("PROFILE_TRADE_PHASE_B_RELOAD: FAIL: handoff identity parse failed."));
		return;
	}
	TArray<uint8> AfterBytes;
	FFileHelper::LoadFileToArray(AfterBytes, *Fdemo_mapProfileStorageContext::ForRoot(ProfileTradeStorageRoot).PrimaryPath());
	const Fdemo_mapProfilePreparationViewState& View = ProfilePreparationWidget->GetViewState();
	const bool bPillSame = Initial.OrderedPermanentStash.ContainsByPredicate(
		[&ExpectedPill](const Fdemo_mapPersistentItemRecord& I){ return I.ItemInstanceId == ExpectedPill && I.ItemDefinitionId == Fdemo_mapItemIds::HealingPillLevel1 && I.StackCount == 1; });
	const bool bTalismanSame = Initial.OrderedPermanentStash.ContainsByPredicate(
		[&ExpectedTalisman](const Fdemo_mapPersistentItemRecord& I){ return I.ItemInstanceId == ExpectedTalisman && I.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman; });
	const bool bBladeVestGone = !Initial.OrderedPermanentStash.ContainsByPredicate(
		[](const Fdemo_mapPersistentItemRecord& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade || I.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest; });
	if (Initial.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
		|| Initial.ProfileId != ExpectedProfile || Initial.SaveGeneration != 4
		|| Initial.PersistentSpiritStones != 20 || Initial.RiskSpiritStones != 0
		|| Initial.ActiveRunId.IsValid() || Initial.OrderedPermanentStash.Num() != 2
		|| View.OrderedShopRows.Num() != 7 || !bPillSame || !bTalismanSame || !bBladeVestGone
		|| ProfileTradePrimaryBytesBeforeLoad.IsEmpty() || ProfileTradePrimaryBytesBeforeLoad != AfterBytes)
	{
		FailAutomation(TEXT("PROFILE_TRADE_PHASE_B_RELOAD: FAIL: durable reload or read-only bytes mismatch."));
		return;
	}
	UE_LOG(Logdemo_map, Log, TEXT("PROFILE_TRADE_PHASE_B_IDENTITY profile=%s pill=%s talisman=%s balance=20 generation=4 catalog=7 bytes_unchanged=1."),
		*Initial.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		*ExpectedPill.ToString(EGuidFormats::DigitsWithHyphens),
		*ExpectedTalisman.ToString(EGuidFormats::DigitsWithHyphens));
	PassAutomation(TEXT("PROFILE_TRADE_PHASE_B_RELOAD: PASS."));
}

void Ademo_mapV3ProgressionManager::RunProfileFlowAutomation()
{
	if (ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Preparation)
	{
		const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow && ProfilePreparationFlow->GetSession()
			? ProfilePreparationFlow->GetSession()->GetSnapshot()
			: Fdemo_mapProfileSessionSnapshot();
		if (!ProfilePreparationFlow
			|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::Preparation
			|| !ProfilePreparationWidget
			|| bProfileWorldActive
			|| !Items.IsValid()
			|| Items->GetRunState() != Edemo_mapRunState::Inactive
			|| Snapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation)
		{
			FailAutomation(TEXT("PROFILE_NORMAL_STARTUP_PREPARATION: FAIL: Preparation-only startup contract mismatch."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP_PREPARATION_IDENTITY profile=%s generation=%d storage=%s."),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			Snapshot.SaveGeneration,
			*ProfilePreparationFlow->GetStorageRoot());
		PassAutomation(TEXT("PROFILE_NORMAL_STARTUP_PREPARATION: PASS."));
		return;
	}
	if (ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Recovered)
	{
		const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow && ProfilePreparationFlow->GetSession()
			? ProfilePreparationFlow->GetSession()->GetSnapshot()
			: Fdemo_mapProfileSessionSnapshot();
		const bool bRiskAbsent = ProfileFlowExpectedItemId.IsValid()
			&& !Snapshot.OrderedPermanentStash.ContainsByPredicate(
				[this](const Fdemo_mapPersistentItemRecord& Item){ return Item.ItemInstanceId == ProfileFlowExpectedItemId; });
		if (!ProfilePreparationFlow
			|| (ProfileFlowInitializeStatus != Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted
				&& ProfileFlowInitializeStatus != Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted)
			|| Snapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
			|| Snapshot.LastTerminalReason != Edemo_mapRunEndReason::RecoveredAbandon
			|| !Snapshot.LastSettlementId.IsValid()
			|| !bRiskAbsent
			|| bProfileWorldActive
			|| !Items.IsValid()
			|| Items->GetRunState() != Edemo_mapRunState::Inactive)
		{
			FailAutomation(TEXT("PROFILE_NORMAL_STARTUP_RECOVERED_ABANDON: FAIL: recovery identity or inactive-world contract mismatch."));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP_RECOVERED_IDENTITY profile=%s generation=%d item=%s run=%s settlement=%s."),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			Snapshot.SaveGeneration,
			*ProfileFlowExpectedItemId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens));
		PassAutomation(TEXT("PROFILE_NORMAL_STARTUP_RECOVERED_ABANDON: PASS."));
		return;
	}
	if (!ProfilePreparationFlow
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::RunActive
		|| !Items.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active
		|| Items->GetActiveRunId() != ProfilePreparationFlow->GetStartedRunId())
	{
		FailAutomation(TEXT("PROFILE_PREPARATION_V3_FLOW: FAIL: V3 world did not reuse the prepared Runtime ActiveRunId."));
		return;
	}
	if (ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Crash)
	{
		const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
		UE_LOG(Logdemo_map, Log, TEXT("PROFILE_NORMAL_STARTUP_CRASH_IDENTITY profile=%s generation=%d item=%s run=%s storage=%s."),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			Snapshot.SaveGeneration,
			*ProfileFlowItemId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*ProfilePreparationFlow->GetStorageRoot());
		PassAutomation(TEXT("PROFILE_NORMAL_STARTUP_CRASH_PREPARED: PASS."));
		return;
	}
	const Edemo_mapRunEndReason Reason = ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Death
		? Edemo_mapRunEndReason::Death
		: Edemo_mapRunEndReason::Extraction;
	const Fdemo_mapItemOperationResult Result = RequestSettlementAndReload(Reason);
	if (!Result.bSuccess)
	{
		FailAutomation(FString::Printf(TEXT("PROFILE_PREPARATION_V3_FLOW: FAIL: existing V3 terminal path rejected: %s"), *Result.Diagnostic));
	}
}

void Ademo_mapV3ProgressionManager::FinishProfileFlowAutomation()
{
	if (!ProfilePreparationFlow
		|| ProfilePreparationFlow->GetPhase() != Edemo_mapProfilePreparationFlowPhase::Preparation
		|| !ProfilePreparationFlow->GetSession())
	{
		FailAutomation(TEXT("PROFILE_PREPARATION_V3_FLOW: FAIL: durable settlement did not return to Preparation."));
		return;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = ProfilePreparationFlow->GetSession()->GetSnapshot();
	const Fdemo_mapProfilePreparationSnapshot Preparation = ProfilePreparationFlow->GetSession()->GetPreparationSnapshot();
	bool bInPermanentStash = false;
	for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
	{
		if (Item.ItemInstanceId == ProfileFlowItemId)
		{
			bInPermanentStash = true;
			break;
		}
	}
	const bool bIdentityCorrect = Snapshot.ProfileId == ProfilePreparationFlow->GetProfileId()
		&& Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
		&& Preparation.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
		&& Snapshot.LastSettlementId.IsValid()
		&& Snapshot.ActiveRunId == ProfilePreparationFlow->GetStartedRunId()
		&& ((ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Death && !bInPermanentStash)
			|| (ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Extract && bInPermanentStash))
		&& (ProfileFlowAutomationPhase != Edemo_mapProfileFlowAutomationPhase::Death
			|| (ProfilePreparationFlow->GetPreviousRunId().IsValid()
				&& ProfilePreparationFlow->GetPreviousSettlementId().IsValid()
				&& ProfilePreparationFlow->GetStartedRunId() != ProfilePreparationFlow->GetPreviousRunId()
				&& ProfilePreparationFlow->GetStartedRunId() != ProfilePreparationFlow->GetPreviousSettlementId()));
	if (!bIdentityCorrect)
	{
		FailAutomation(FString::Printf(TEXT("PROFILE_PREPARATION_V3_FLOW: FAIL: identity closure mismatch profile=%s item=%s run=%s settlement=%s stash=%d phase=%s."),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*ProfileFlowItemId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
			*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
			bInPermanentStash,
			ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Death ? TEXT("Death") : TEXT("Extract")));
		return;
	}

	UE_LOG(Logdemo_map, Log, TEXT("PROFILE_PREPARATION_V3_FLOW_IDENTITY profile=%s generation=%d item=%s run=%s settlement=%s reason=%d previous_run=%s previous_settlement=%s storage=%s."),
		*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
		Snapshot.SaveGeneration,
		*ProfileFlowItemId.ToString(EGuidFormats::DigitsWithHyphens),
		*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
		static_cast<int32>(Snapshot.LastTerminalReason),
		*ProfilePreparationFlow->GetPreviousRunId().ToString(EGuidFormats::DigitsWithHyphens),
		*ProfilePreparationFlow->GetPreviousSettlementId().ToString(EGuidFormats::DigitsWithHyphens),
		*ProfilePreparationFlow->GetStorageRoot());
	PassAutomation(ProfileFlowAutomationPhase == Edemo_mapProfileFlowAutomationPhase::Death
		? TEXT("PROFILE_PREPARATION_V3_FLOW_DEATH: PASS.")
		: TEXT("PROFILE_PREPARATION_V3_FLOW_EXTRACT: PASS."));
}
#endif

void Ademo_mapV3ProgressionManager::RunEnemyLootAutomation()
{
#if !UE_BUILD_SHIPPING
	Ademo_mapEnemyCharacter* Melee=nullptr; Ademo_mapRangedEnemyCharacter* Ranged=nullptr; Ademo_mapHeavyEnemyCharacter* Heavy=nullptr;
	for(TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld());It;++It){Melee=*It;break;} for(TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld());It;++It){Ranged=*It;break;} for(TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld());It;++It){Heavy=*It;break;}
	if(!Melee||!Ranged||!Heavy){FailAutomation(TEXT("V3_ENEMY_LOOT_AUTOMATION: FAIL: three enemy archetypes missing."));return;}
	UGameplayStatics::ApplyDamage(Melee,100.0f,nullptr,PlayerPawn.Get(),nullptr); UGameplayStatics::ApplyDamage(Ranged,100.0f,nullptr,PlayerPawn.Get(),nullptr); UGameplayStatics::ApplyDamage(Heavy,100.0f,nullptr,PlayerPawn.Get(),nullptr);
	int32 Dust=0,Iron=0,Token=0;
	for(const FGuid& Id:Items->GetAuthority().FindWorldInstances())if(const Fdemo_mapItemInstance* Item=Items->GetAuthority().FindInstance(Id)){if(Item->DefinitionId==Fdemo_mapItemIds::SpiritDust)Dust+=Item->Quantity;else if(Item->DefinitionId==Fdemo_mapItemIds::IronShard)Iron+=Item->Quantity;else if(Item->DefinitionId==Fdemo_mapItemIds::AncientToken)Token+=Item->Quantity;}
	if(Dust!=2||Iron!=2||Token!=1||Items->GetLootSourceStates().Num()!=3){FailAutomation(FString::Printf(TEXT("V3_ENEMY_LOOT_AUTOMATION: FAIL: drops dust=%d iron=%d token=%d sources=%d"),Dust,Iron,Token,Items->GetLootSourceStates().Num()));return;}
	UE_LOG(Logdemo_map,Log,TEXT("V3_ENEMY_LOOT_AUTOMATION: melee SpiritDustx2, ranged IronShardx2, heavy AncientTokenx1, source idempotency passed.")); PassAutomation(TEXT("V3_ENEMY_LOOT_AUTOMATION: PASS."));
#endif
}

void Ademo_mapV3ProgressionManager::RunLifecycleAutomation()
{
#if !UE_BUILD_SHIPPING
	if(!Items.IsValid()||Items->GetRunState()!=Edemo_mapRunState::Active){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: active run missing."));return;}
	Fdemo_mapSettlementSummary Summary;
	if(GRunLifecycleAutomationPhase==0){if(Items->GetSessionStashItemCount()!=0||!Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade,1).bSuccess){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: death setup."));return;}GRunLifecycleAutomationPhase=1;RequestSettlementAndReload(Edemo_mapRunEndReason::Death);return;}
	if(GRunLifecycleAutomationPhase==1){if(Items->GetSessionStashItemCount()!=0||!Items->AddDefinition(Fdemo_mapItemIds::AncientToken,1).bSuccess){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: extraction setup."));return;}GRunLifecycleAutomationPhase=2;RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction);return;}
	if(GRunLifecycleAutomationPhase==2){if(Items->GetSessionStashValue()!=500||!Items->AddDefinition(Fdemo_mapItemIds::SpiritDust,1).bSuccess){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: abandon setup."));return;}GRunLifecycleAutomationPhase=3;if(!PressBoundKey(EKeys::R)){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: R binding dispatch."));}return;}
	if(GRunLifecycleAutomationPhase==3)
	{
		Ademo_mapPlayerController* Controller=GetDemoController();
		if(Items->GetSessionStashItemCount()!=1||Items->GetSessionStashValue()!=500){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: stash changed after abandon."));return;}
		if(!Controller||Controller->IsMoveInputIgnored()||Controller->IsLookInputIgnored()||!Controller->IsGameplayInputAllowed()){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: post-reload gameplay input was not restored."));return;}
		if(!PressBoundKey(EKeys::W)||!PressBoundKey(EKeys::LeftMouseButton)||!PressBoundKey(EKeys::Q)||!PressBoundKey(EKeys::E)||!PressBoundKey(EKeys::F)||!PressBoundKey(EKeys::Tab)||!bInventoryOpen){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: post-reload input binding dispatch."));return;}
		if(!PressBoundKey(EKeys::Tab)||bInventoryOpen){FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: post-reload inventory close dispatch."));return;}
		GRunLifecycleAutomationPhase=0;UE_LOG(Logdemo_map,Log,TEXT("V3_RUN_LIFECYCLE_AUTOMATION: Death -> reload -> Extraction -> reload -> R Abandon -> reload; post-reload gameplay input lock, movement, attack, skills and inventory bindings passed."));
		if (bPostSettlementInputRestoreAutomation) PassAutomation(TEXT("V3_POST_SETTLEMENT_INPUT_RESTORE: PASS."));
		else PassAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: PASS."));
		return;
	}
	FailAutomation(TEXT("V3_RUN_LIFECYCLE_AUTOMATION: FAIL: invalid phase."));
#endif
}

void Ademo_mapV3ProgressionManager::RunSettlementUIAutomation()
{
#if !UE_BUILD_SHIPPING
	if(!Items->AddDefinition(Fdemo_mapItemIds::AncientToken,1).bSuccess||!RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction).bSuccess||!SettlementWidget||!SettlementWidget->HasRenderedSummary()){FailAutomation(TEXT("V3_SETTLEMENT_UI_AUTOMATION: FAIL: rendered snapshot."));return;}
	const Fdemo_mapSettlementSummary& S=SettlementWidget->GetSummary();if(S.Reason!=Edemo_mapRunEndReason::Extraction||S.SecuredValue!=500||S.StashValueAfter<500){FailAutomation(TEXT("V3_SETTLEMENT_UI_AUTOMATION: FAIL: summary values."));return;}PassAutomation(TEXT("V3_SETTLEMENT_UI_AUTOMATION: PASS."));
#endif
}

void Ademo_mapV3ProgressionManager::RunFreshSessionAutomation()
{
#if !UE_BUILD_SHIPPING
	TArray<FString> Files;IFileManager::Get().FindFilesRecursive(Files,*FPaths::ProjectSavedDir(),TEXT("*SessionStash*"),true,false,false);
	if(Items->GetSessionStashItemCount()!=0||Items->GetSessionStashValue()!=0||!Files.IsEmpty()){FailAutomation(TEXT("V3_FRESH_SESSION_AUTOMATION: FAIL: process-local stash was not empty or a persistence file exists."));return;}UE_LOG(Logdemo_map,Log,TEXT("V3_FRESH_SESSION_AUTOMATION: stash=0 value=0 persistence_files=0."));PassAutomation(TEXT("V3_FRESH_SESSION_AUTOMATION: PASS."));
#endif
}

void Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep()
{
#if !UE_BUILD_SHIPPING
	if(VisibleOutputDirectory.IsEmpty()){FailAutomation(TEXT("V3_RUN_VISIBLE_ACCEPTANCE: FAIL: output directory missing."));return;}
	if(GRunLifecycleVisiblePhase==0)
	{
		if(VisibleStep==0){VisibleStep=1;for(TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld());It;++It){UGameplayStatics::ApplyDamage(*It,100,nullptr,PlayerPawn.Get(),nullptr);break;}for(TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld());It;++It){UGameplayStatics::ApplyDamage(*It,100,nullptr,PlayerPawn.Get(),nullptr);break;}for(TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld());It;++It){UGameplayStatics::ApplyDamage(*It,100,nullptr,PlayerPawn.Get(),nullptr);break;}GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.6f,false);return;}
		if(VisibleStep==1){VisibleStep=2;CaptureVisible(TEXT("V3_ENEMY_DROPS_THREE_TYPES.png"));GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.7f,false);return;}
		Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade,1);GRunLifecycleVisiblePhase=1;RequestSettlementAndReload(Edemo_mapRunEndReason::Death);GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.35f,false);return;
	}
	if(GRunLifecycleVisiblePhase==1&&VisibleStep>0){CaptureVisible(TEXT("V3_DEATH_LOSS_SETTLEMENT.png"));VisibleStep=0;return;}
	if(GRunLifecycleVisiblePhase==1){Items->AddDefinition(Fdemo_mapItemIds::AncientToken,1);GRunLifecycleVisiblePhase=2;RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction);GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.35f,false);++VisibleStep;return;}
	if(GRunLifecycleVisiblePhase==2&&VisibleStep>0){CaptureVisible(TEXT("V3_EXTRACTION_SECURED_SETTLEMENT.png"));VisibleStep=0;return;}
	if(GRunLifecycleVisiblePhase==2){OpenInventory();GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.45f,false);GRunLifecycleVisiblePhase=3;return;}
	if(GRunLifecycleVisiblePhase==3&&VisibleStep==0){CaptureVisible(TEXT("V3_NEXT_RUN_SESSION_STASH.png"));VisibleStep=1;GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.7f,false);return;}
	if(GRunLifecycleVisiblePhase==3&&VisibleStep==1){CloseInventory();Items->AddDefinition(Fdemo_mapItemIds::SpiritDust,1);RequestSettlementAndReload(Edemo_mapRunEndReason::Abandon);VisibleStep=2;GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.35f,false);return;}
	if(GRunLifecycleVisiblePhase==3&&VisibleStep==2){CaptureVisible(TEXT("V3_R_ABANDON_SETTLEMENT.png"));VisibleStep=3;GetWorldTimerManager().SetTimer(AutomationTimer,this,&Ademo_mapV3ProgressionManager::RunLifecycleVisibleStep,0.8f,false);return;}
	if(GRunLifecycleVisiblePhase==3&&VisibleStep==3){GRunLifecycleVisiblePhase=0;PassAutomation(TEXT("V3_RUN_VISIBLE_ACCEPTANCE: PASS."));return;}
	FailAutomation(TEXT("V3_RUN_VISIBLE_ACCEPTANCE: FAIL: invalid phase."));
#endif
}

// Automation implementations exercise the same G/Tab input bindings and UButton delegates as a player.
void Ademo_mapV3ProgressionManager::RunWorldInteractionAutomation()
{
#if !UE_BUILD_SHIPPING
	auto Fail = [this](const FString& Detail){ FailAutomation(TEXT("V3_WORLD_INTERACTION_AUTOMATION: FAIL: ") + Detail); return; };
	if (!Items.IsValid() || !PlayerPawn.IsValid() || Fdemo_mapItemDefinitions::GetAll().Num() != 42 || Chests.Num() != 3 || InitialWorldItems.Num() != 3 || Items->GetWorldActorCount() != 3) { Fail(TEXT("initial V3 foundation.")); return; }
	FString Error;
	if (!Items->ValidateInvariants(&Error)) { Fail(TEXT("initial invariants: ") + Error); return; }
	for (const TWeakObjectPtr<Ademo_mapWorldItem>& ItemActor : InitialWorldItems)
	{
		const Fdemo_mapItemInstance* Instance = ItemActor.IsValid() ? Items->GetAuthority().FindInstance(ItemActor->GetInstanceId()) : nullptr;
		const Fdemo_mapItemDefinition* Definition = Instance ? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId) : nullptr;
		const FString LabelText = ItemActor.IsValid() ? ItemActor->GetWorldLabelText() : FString();
		bool bAscii = !LabelText.IsEmpty();
		for (TCHAR Character : LabelText) bAscii = bAscii && Character <= 127;
		if (!Definition || Definition->WorldLabelName.IsEmpty() || !LabelText.StartsWith(Definition->WorldLabelName) || !bAscii) { Fail(TEXT("stable ASCII world-item label mapping.")); return; }
		if (GetDemoController() && GetDemoController()->PlayerCameraManager)
		{
			const FVector ToCamera = (GetDemoController()->PlayerCameraManager->GetCameraLocation() - ItemActor->GetActorLocation()).GetSafeNormal();
			if (FVector::DotProduct(ItemActor->GetWorldLabelForwardVector(), ToCamera) < 0.80f) { Fail(TEXT("world-item label camera-facing orientation.")); return; }
		}
	}
	if (!Chests[0].IsValid() || Chests[0]->GetWorldLabelText() != TEXT("CHEST 01 - CLOSED")) { Fail(TEXT("stable closed chest label.")); return; }

	Ademo_mapWorldItem* HeavyActor = InitialWorldItems[0].Get();
	if (!HeavyActor || !MovePawnNear(HeavyActor) || GetFocusedActor() != HeavyActor || !GetInteractionPrompt().ToString().Contains(TEXT("拾取"))) { Fail(TEXT("deterministic visible focus.")); return; }
	const FGuid HeavyId = HeavyActor->GetInstanceId();
	const bool bGHandled = PressBoundKey(EKeys::G);
	UE_LOG(Logdemo_map, Log, TEXT("V3_WORLD_INTERACTION_AUTOMATION: G dispatched handled=%d result=%d slot=%d actor=%d."), bGHandled, static_cast<int32>(LastOperationResult.Code), Items->GetAuthority().FindInventorySlot(HeavyId), Items->GetWorldActor(HeavyId) != nullptr);
	if (Items->GetAuthority().FindInventorySlot(HeavyId) == INDEX_NONE || Items->GetWorldActor(HeavyId) != nullptr) { Fail(TEXT("real G pickup or binding cleanup.")); return; }
	const int32 AfterPickupCount = Items->GetAuthority().GetInstanceSnapshot().Num();
	const Fdemo_mapItemOperationResult Duplicate = Items->PickupWorldItem(HeavyActor, GetDemoController());
	if (Duplicate.bSuccess || Items->GetAuthority().GetInstanceSnapshot().Num() != AfterPickupCount) { Fail(TEXT("duplicate pickup protection.")); return; }
	UE_LOG(Logdemo_map, Log, TEXT("V3_WORLD_INTERACTION_AUTOMATION: real G pickup and duplicate protection passed."));

	if (!Items->AddDefinition(Fdemo_mapItemIds::SpiritDust, 4).bSuccess) { Fail(TEXT("stack precondition.")); return; }
	Ademo_mapLootChest* Chest = Chests[0].Get();
	if (!Chest || !MovePawnNear(Chest)) { Fail(TEXT("real G chest focus.")); return; }
	PressBoundKey(EKeys::G);
	RefreshFocusNow();
	if (!Chest->IsOpened() || Chest->GetSpawnedLoot().Num() != 2 || !Chest->HasOpenedPresentation() || Chest->CanInteract(GetDemoController()) || GetFocusedActor() == Chest) { Fail(TEXT("real G chest open state, presentation, or focus removal.")); return; }
	const int32 OpenWorldCount = Items->GetWorldActorCount();
	const Fdemo_mapItemOperationResult Reopen = Chest->RequestInteract(GetDemoController());
	if (Reopen.bSuccess || Reopen.Code != Edemo_mapItemResultCode::AlreadyOpened || Items->GetWorldActorCount() != OpenWorldCount || !IsValid(Chest) || Chest->GetChestId() != TEXT("V3.Chest.01")) { Fail(TEXT("one-shot retained chest identity.")); return; }
	Ademo_mapWorldItem* DustActor = nullptr;
	for (const TWeakObjectPtr<Ademo_mapWorldItem>& LootActor : Chest->GetSpawnedLoot())
	{
		const Fdemo_mapItemInstance* Instance = LootActor.IsValid() ? Items->GetAuthority().FindInstance(LootActor->GetInstanceId()) : nullptr;
		if (Instance && Instance->DefinitionId == Fdemo_mapItemIds::SpiritDust) DustActor = LootActor.Get();
	}
	if (!DustActor || !MovePawnNear(DustActor)) { Fail(TEXT("stack pickup focus.")); return; }
	PressBoundKey(EKeys::G);
	const TArray<FGuid> DustStacks = Items->GetAuthority().FindInventoryInstancesByDefinition(Fdemo_mapItemIds::SpiritDust);
	if (DustStacks.Num() != 2 || Items->GetAuthority().FindInstance(DustStacks[0])->Quantity + Items->GetAuthority().FindInstance(DustStacks[1])->Quantity != 7) { Fail(TEXT("atomic 4+3 stack pickup.")); return; }

	const int32 SlotsToFill = Items->GetAuthority().GetFreeInventorySlots();
	if (SlotsToFill <= 0 || !Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade, SlotsToFill).bSuccess || Items->GetAuthority().GetFreeInventorySlots() != 0) { Fail(TEXT("full inventory precondition.")); return; }
	Ademo_mapWorldItem* ArmorActor = InitialWorldItems[1].Get();
	const FGuid ArmorWorldId = ArmorActor ? ArmorActor->GetInstanceId() : FGuid();
	if (!ArmorActor || !MovePawnNear(ArmorActor)) { Fail(TEXT("full inventory focus.")); return; }
	PressBoundKey(EKeys::G);
	if (LastOperationResult.Code != Edemo_mapItemResultCode::InventoryFull || !IsValid(ArmorActor) || Items->GetAuthority().FindInstance(ArmorWorldId)->OwnershipState != Edemo_mapItemOwnershipState::World) { Fail(TEXT("full inventory pickup rollback.")); return; }

	const FVector SavedLocation = PlayerPawn->GetActorLocation();
	PlayerPawn->SetActorLocation(FVector(5000, 5000, 1000), false, nullptr, ETeleportType::TeleportPhysics);
	Ademo_mapWorldItem* UnsafeActor = nullptr;
	const int32 HeavySlotBefore = Items->GetAuthority().FindInventorySlot(HeavyId);
	const Fdemo_mapItemOperationResult UnsafeDrop = Items->DropInventoryItem(HeavyId, PlayerPawn.Get(), UnsafeActor);
	if (UnsafeDrop.bSuccess || UnsafeDrop.Code != Edemo_mapItemResultCode::UnsafeDropLocation || UnsafeActor != nullptr || Items->GetAuthority().FindInventorySlot(HeavyId) != HeavySlotBefore) { Fail(TEXT("unsafe drop rollback.")); return; }
	PlayerPawn->SetActorLocation(SavedLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Ademo_mapWorldItem* DroppedActor = nullptr;
	if (!RequestDropInventory(HeavyId).bSuccess || !(DroppedActor = Items->GetWorldActor(HeavyId)) || Items->GetAuthority().FindInventorySlot(HeavyId) != INDEX_NONE) { Fail(TEXT("drop same GUID.")); return; }
	if (!MovePawnNear(DroppedActor)) { Fail(TEXT("drop repick focus.")); return; }
	PressBoundKey(EKeys::G);
	if (Items->GetAuthority().FindInventorySlot(HeavyId) == INDEX_NONE) { Fail(TEXT("drop repick through real G.")); return; }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapLootChest* InvalidChest = GetWorld()->SpawnActor<Ademo_mapLootChest>(Ademo_mapLootChest::StaticClass(), FVector(5000, 5000, 1000), FRotator::ZeroRotator, Params);
	if (!InvalidChest) { Fail(TEXT("rollback chest spawn.")); return; }
	InvalidChest->ConfigureChest(0, TEXT("Automation.RollbackChest"));
	PlayerPawn->SetActorLocation(FVector(4880, 5000, 1000), false, nullptr, ETeleportType::TeleportPhysics);
	const int32 BeforeFailedChest = Items->GetWorldActorCount();
	const Fdemo_mapItemOperationResult FailedChest = InvalidChest->RequestInteract(GetDemoController());
	if (FailedChest.bSuccess || InvalidChest->IsOpened() || Items->GetWorldActorCount() != BeforeFailedChest) { InvalidChest->Destroy(); Fail(TEXT("atomic chest rollback.")); return; }
	InvalidChest->Destroy();

	Ademo_mapWorldItem* FocusDestroyActor = InitialWorldItems[2].Get();
	if (FocusDestroyActor && MovePawnNear(FocusDestroyActor)) { const FGuid DestroyedId = FocusDestroyActor->GetInstanceId(); FocusDestroyActor->Destroy(); RefreshFocusNow(); const Fdemo_mapItemInstance* Destroyed = Items->GetAuthority().FindInstance(DestroyedId); if (GetFocusedActor() == FocusDestroyActor || !Destroyed || Destroyed->OwnershipState != Edemo_mapItemOwnershipState::Destroyed) { Fail(TEXT("focus/actor EndPlay cleanup.")); return; } }
	PlayerPawn->SetActorLocation(SavedLocation, false, nullptr, ETeleportType::TeleportPhysics);
	const int32 InventoryBeforeTeardown = Items->GetAuthority().GetUsedInventorySlots();
	Items->TeardownWorld(GetWorld());
	if (Items->GetAuthority().FindWorldInstances().Num() != 0 || Items->GetWorldActorCount() != 0 || Items->GetAuthority().GetUsedInventorySlots() != InventoryBeforeTeardown) { Fail(TEXT("world teardown preservation.")); return; }
	if (!Items->ValidateInvariants(&Error)) { Fail(TEXT("final invariants: ") + Error); return; }
	UE_LOG(Logdemo_map, Log, TEXT("V3_WORLD_INTERACTION_AUTOMATION: stack, full inventory, drop rollback, chest rollback, EndPlay and teardown passed."));
	PassAutomation(TEXT("V3_WORLD_INTERACTION_AUTOMATION: PASS."));
#endif
}

void Ademo_mapV3ProgressionManager::RunCloseRangeProjectileAutomation()
{
#if !UE_BUILD_SHIPPING
	auto Fail = [this](const FString& Detail){ FailAutomation(TEXT("V3_CLOSE_RANGE_PROJECTILE: FAIL: ") + Detail); };
	if (!PlayerPawn.IsValid() || !GetWorld()) { Fail(TEXT("missing player or world.")); return; }
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller) { Fail(TEXT("missing demo player controller.")); return; }

	if (bCloseRangeAwaitingResult)
	{
		const int32 PrimaryAfter = GetAutomationHealth(CloseRangePrimaryTarget.Get());
		const int32 SecondaryAfter = GetAutomationHealth(CloseRangeSecondaryTarget.Get());
		bool bPassed = false;
		if (CloseRangeScenario <= 3 || CloseRangeScenario == 6 || CloseRangeScenario == 7)
		{
			bPassed = PrimaryAfter == CloseRangePrimaryHealthBefore - 1;
		}
		else if (CloseRangeScenario == 4)
		{
			bPassed = PrimaryAfter == CloseRangePrimaryHealthBefore && SecondaryAfter == CloseRangeSecondaryHealthBefore - 1;
		}
		else
		{
			bPassed = PrimaryAfter == CloseRangePrimaryHealthBefore;
		}
		if (!bPassed)
		{
			Fail(FString::Printf(TEXT("scenario=%d health before/after primary=%d/%d secondary=%d/%d."), CloseRangeScenario, CloseRangePrimaryHealthBefore, PrimaryAfter, CloseRangeSecondaryHealthBefore, SecondaryAfter));
			return;
		}
		UE_LOG(Logdemo_map, Log, TEXT("V3_CLOSE_RANGE_PROJECTILE: scenario=%d passed primary=%d->%d secondary=%d->%d."), CloseRangeScenario, CloseRangePrimaryHealthBefore, PrimaryAfter, CloseRangeSecondaryHealthBefore, SecondaryAfter);
		if (CloseRangePrimaryTarget.IsValid()) CloseRangePrimaryTarget->SetActorLocation(PlayerPawn->GetActorLocation() + FVector(3000.0f, 500.0f + CloseRangeScenario * 120.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		if (CloseRangeSecondaryTarget.IsValid()) CloseRangeSecondaryTarget->SetActorLocation(PlayerPawn->GetActorLocation() + FVector(3000.0f, -500.0f - CloseRangeScenario * 120.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		if (CloseRangeBlockingActor.IsValid()) CloseRangeBlockingActor->Destroy();
		CloseRangePrimaryTarget.Reset(); CloseRangeSecondaryTarget.Reset(); CloseRangeBlockingActor.Reset();
		bCloseRangeAwaitingResult = false;
		++CloseRangeScenario;
		if (CloseRangeScenario >= 9)
		{
			PassAutomation(TEXT("V3_CLOSE_RANGE_PROJECTILE: PASS."));
			return;
		}
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunCloseRangeProjectileAutomation, 0.72f, false);
		return;
	}

	AActor* ExistingTarget = nullptr;
	if (CloseRangeScenario == 0) { for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) { ExistingTarget = *It; break; } }
	else if (CloseRangeScenario == 1) { for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) { ExistingTarget = *It; break; } }
	else if (CloseRangeScenario == 2) { for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) { ExistingTarget = *It; break; } }
	else if (CloseRangeScenario == 3) { for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) { ExistingTarget = *It; break; } }

	const FVector Base = PlayerPawn->GetActorLocation();
	const FVector AimDirection = FVector::ForwardVector;
	Controller->SetAutomationAimDirection(AimDirection);
	PlayerPawn->SetActorRotation(AimDirection.Rotation());
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	float Distance = 0.0f;
	float ResultDelay = 0.35f;
	if (CloseRangeScenario <= 3)
	{
		static const float CloseDistances[] = { 50.0f, 70.0f, 90.0f, 110.0f };
		Distance = CloseDistances[CloseRangeScenario];
		CloseRangePrimaryTarget = ExistingTarget;
	}
	else if (CloseRangeScenario == 4)
	{
		for (TActorIterator<Ademo_mapFriendlyUnit> It(GetWorld()); It; ++It) { CloseRangePrimaryTarget = *It; break; }
		CloseRangeSecondaryTarget = GetWorld()->SpawnActor<Ademo_mapTrainingTarget>(Ademo_mapTrainingTarget::StaticClass(), Base + FVector(240.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);
		Distance = 70.0f;
	}
	else
	{
		Distance = CloseRangeScenario == 5 ? 240.0f : CloseRangeScenario == 6 ? 400.0f : CloseRangeScenario == 7 ? 1320.0f : 1500.0f;
		CloseRangePrimaryTarget = GetWorld()->SpawnActor<Ademo_mapTrainingTarget>(Ademo_mapTrainingTarget::StaticClass(), Base + FVector(Distance, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);
		ResultDelay = CloseRangeScenario == 6 ? 0.60f : CloseRangeScenario >= 7 ? 1.65f : 0.45f;
		if (CloseRangeScenario == 5)
		{
			AStaticMeshActor* Wall = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Base + FVector(80.0f, 0.0f, 55.0f), FRotator::ZeroRotator, SpawnParams);
			if (Wall)
			{
				Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
				Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
				Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
				Wall->SetActorScale3D(FVector(0.30f, 1.0f, 1.2f));
				CloseRangeBlockingActor = Wall;
			}
		}
	}

	if (!CloseRangePrimaryTarget.IsValid() || (CloseRangeScenario == 4 && !CloseRangeSecondaryTarget.IsValid()) || (CloseRangeScenario == 5 && !CloseRangeBlockingActor.IsValid()))
	{
		Fail(FString::Printf(TEXT("scenario=%d setup actor missing."), CloseRangeScenario));
		return;
	}
	CloseRangePrimaryTarget->SetActorLocation(Base + FVector(Distance, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	if (CloseRangeSecondaryTarget.IsValid()) CloseRangeSecondaryTarget->SetActorLocation(Base + FVector(240.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	CloseRangePrimaryHealthBefore = GetAutomationHealth(CloseRangePrimaryTarget.Get());
	CloseRangeSecondaryHealthBefore = GetAutomationHealth(CloseRangeSecondaryTarget.Get());
	if (CloseRangePrimaryHealthBefore == INDEX_NONE || !PressBoundKey(EKeys::F))
	{
		Fail(FString::Printf(TEXT("scenario=%d real F dispatch or health setup failed."), CloseRangeScenario));
		return;
	}
	bCloseRangeAwaitingResult = true;
	UE_LOG(Logdemo_map, Log, TEXT("V3_CLOSE_RANGE_PROJECTILE: scenario=%d fired through real F at distance=%.1f."), CloseRangeScenario, Distance);
	GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunCloseRangeProjectileAutomation, ResultDelay, false);
#endif
}

void Ademo_mapV3ProgressionManager::RunInventoryUIAutomation()
{
#if !UE_BUILD_SHIPPING
	auto Fail = [this](const FString& Detail){ FailAutomation(TEXT("V3_INVENTORY_UI_AUTOMATION: FAIL: ") + Detail); return; };
	Ademo_mapLootChest* Chest = Chests.IsValidIndex(0) ? Chests[0].Get() : nullptr;
	if (!Chest || !MovePawnNear(Chest)) { Fail(TEXT("Chest 01 real G focus.")); return; }
	PressBoundKey(EKeys::G);
	if (!Chest->IsOpened() || Chest->GetSpawnedLoot().Num() != 2) { Fail(TEXT("Chest 01 real G open.")); return; }
	Ademo_mapWorldItem* BladeActor = nullptr;
	Ademo_mapWorldItem* DustActor = nullptr;
	for (const TWeakObjectPtr<Ademo_mapWorldItem>& LootActor : Chest->GetSpawnedLoot())
	{
		const Fdemo_mapItemInstance* Instance = LootActor.IsValid() ? Items->GetAuthority().FindInstance(LootActor->GetInstanceId()) : nullptr;
		if (Instance && Instance->DefinitionId == Fdemo_mapItemIds::TrainingBlade) BladeActor = LootActor.Get();
		if (Instance && Instance->DefinitionId == Fdemo_mapItemIds::SpiritDust) DustActor = LootActor.Get();
	}
	if (!BladeActor || !MovePawnNear(BladeActor)) { Fail(TEXT("TrainingBlade real G focus.")); return; }
	const FGuid BladeId = BladeActor->GetInstanceId();
	PressBoundKey(EKeys::G);
	if (!DustActor || !MovePawnNear(DustActor)) { Fail(TEXT("SpiritDust real G focus.")); return; }
	PressBoundKey(EKeys::G);
	const int32 InventoryIndex = Items->GetAuthority().FindInventorySlot(BladeId);
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (InventoryIndex == INDEX_NONE || !Controller) { Fail(TEXT("real Tab precondition.")); return; }
	PressBoundKey(EKeys::Tab);
	if (!bInventoryOpen || !InventoryWidget || InventoryWidget->GetInventoryButtonCount() != Items->GetAuthority().GetInventoryCapacity() || Controller->IsGameplayInputAllowed()) { Fail(TEXT("real bound Inventory open, authority capacity, or input block.")); return; }
	Udemo_mapInventoryWidget* FirstWidget = InventoryWidget;
	const int32 WorldCountBeforeBlockedG = Items->GetWorldActorCount();
	PressBoundKey(EKeys::G);
	PressBoundKey(EKeys::LeftMouseButton);
	if (Items->GetWorldActorCount() != WorldCountBeforeBlockedG) { Fail(TEXT("UI input passthrough.")); return; }
	if (!InventoryWidget->AutomationClickInventorySlot(InventoryIndex) || !InventoryWidget->AutomationClickEquip() || Items->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot) != BladeId) { Fail(TEXT("actual inventory/equip button events.")); return; }
	Udemo_mapAttributeComponent* Attributes = PlayerPawn->FindComponentByClass<Udemo_mapAttributeComponent>();
	float AttackPower = 0.0f;
	if (!Attributes || !Attributes->GetFinalValue(Fdemo_mapAttributeIds::AttackPower, AttackPower) || !FMath::IsNearlyEqual(AttackPower, 2.0f)) { Fail(TEXT("TrainingBlade AttackPower 1 to 2.")); return; }
	if (!InventoryWidget->AutomationClickClose() || bInventoryOpen || !Controller->IsGameplayInputAllowed() || !Controller->bShowMouseCursor) { Fail(TEXT("close button or cursor/input restoration.")); return; }

	Ademo_mapTrainingTarget* DamageTarget = nullptr;
	for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) { DamageTarget = *It; break; }
	if (!DamageTarget || !MovePawnNear(DamageTarget, 120.0f)) { Fail(TEXT("LMB target setup.")); return; }
	PlayerPawn->SetActorRotation((DamageTarget->GetActorLocation() - PlayerPawn->GetActorLocation()).Rotation());
	PressBoundKey(EKeys::LeftMouseButton);
	if (!DamageTarget->WasDestroyedByDamage() || DamageTarget->GetHealth() > 0) { Fail(TEXT("equipped real LMB damage vertical slice.")); return; }

	PressBoundKey(EKeys::Tab);
	if (!bInventoryOpen || InventoryWidget != FirstWidget || !InventoryWidget->AutomationClickEquipmentSlot(Fdemo_mapItemIds::WeaponSlot) || !InventoryWidget->AutomationClickUnequip()) { Fail(TEXT("widget reuse or unequip button.")); return; }
	if (Items->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot).IsValid() || Items->GetAuthority().FindInventorySlot(BladeId) == INDEX_NONE) { Fail(TEXT("unequip result.")); return; }
	if (!Attributes->GetFinalValue(Fdemo_mapAttributeIds::AttackPower, AttackPower) || !FMath::IsNearlyEqual(AttackPower, 1.0f)) { Fail(TEXT("unequip AttackPower 2 to 1.")); return; }
	const int32 DropIndex = Items->GetAuthority().FindInventorySlot(BladeId);
	if (!InventoryWidget->AutomationClickInventorySlot(DropIndex) || !InventoryWidget->AutomationClickDrop() || Items->GetWorldActor(BladeId) == nullptr) { Fail(TEXT("drop button transaction.")); return; }
	if (!InventoryWidget->AutomationClickClose() || bInventoryOpen) { Fail(TEXT("second close.")); return; }
	Ademo_mapWorldItem* Dropped = Items->GetWorldActor(BladeId);
	if (!Dropped || !MovePawnNear(Dropped)) { Fail(TEXT("repick focus after UI drop.")); return; }
	PressBoundKey(EKeys::G);
	if (Items->GetAuthority().FindInventorySlot(BladeId) == INDEX_NONE) { Fail(TEXT("repick after UI drop.")); return; }
	FString Error;
	if (!Items->ValidateInvariants(&Error)) { Fail(TEXT("final invariants: ") + Error); return; }
	UE_LOG(Logdemo_map, Log, TEXT("V3_INVENTORY_UI_AUTOMATION: Chest 01, real G TrainingBlade/SpiritDust pickup, Tab, 12 slots, actual buttons, AttackPower 1->2->1, real LMB=2, cursor restore, drop and same-GUID repick passed."));
	PassAutomation(TEXT("V3_INVENTORY_UI_AUTOMATION: PASS."));
#endif
}

void Ademo_mapV3ProgressionManager::
	RunP6DualLootVisibleAcceptanceStep()
{
#if !UE_BUILD_SHIPPING
	auto Schedule = [this](float Delay)
	{
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::
				RunP6DualLootVisibleAcceptanceStep,
			Delay,
			false);
	};
	auto Fail = [this](const FString& Reason)
	{
		FailAutomation(
			TEXT("P6_DUAL_LOOT_VISIBLE_ACCEPTANCE: FAIL: ")
			+ Reason);
	};
	auto FindChest = [this]() -> Ademo_mapLootChest*
	{
		for (const TWeakObjectPtr<Ademo_mapLootChest>& Candidate : Chests)
		{
			if (Candidate.IsValid()
				&& Candidate->IsP4PrototypeChest())
			{
				return Candidate.Get();
			}
		}
		return nullptr;
	};
	auto FindP6Corpse = [this]() -> Ademo_mapCorpseContainerActor*
	{
		for (const TWeakObjectPtr<Ademo_mapCorpseContainerActor>& Candidate :
			Corpses)
		{
			if (Candidate.IsValid()
				&& Candidate->GetStableSourceId()
					== FName(TEXT("Automation.P6.Corpse")))
			{
				return Candidate.Get();
			}
		}
		return nullptr;
	};
	auto FindEntry = [](
		const Fdemo_mapRuntimeContainerSnapshot& Snapshot,
		Edemo_mapRuntimeContainerSection Section,
		int32 SlotIndex)
		-> const Fdemo_mapRuntimeContainerEntrySnapshot*
	{
		const Fdemo_mapRuntimeContainerSectionSnapshot* FoundSection =
			Snapshot.Sections.FindByPredicate(
				[Section](
					const Fdemo_mapRuntimeContainerSectionSnapshot& Candidate)
				{
					return Candidate.Section == Section;
				});
		return FoundSection
			? FoundSection->OrderedOccupiedEntries.FindByPredicate(
				[SlotIndex](
					const Fdemo_mapRuntimeContainerEntrySnapshot& Entry)
				{
					return Entry.SlotIndex == SlotIndex;
				})
			: nullptr;
	};
	auto FindRegion = [](
		const Fdemo_mapSearchContainerViewState& View,
		Edemo_mapDualLootTargetRegion Region)
		-> const Fdemo_mapDualLootTargetRegionView*
	{
		return View.TargetRegions.FindByPredicate(
			[Region](const Fdemo_mapDualLootTargetRegionView& Candidate)
			{
				return Candidate.Region == Region;
			});
	};

	Ademo_mapPlayerController* Controller = GetDemoController();
	APawn* Pawn = PlayerPawn.Get();
	if ((!bP6DualLootManualFixture && VisibleOutputDirectory.IsEmpty())
		|| !Controller
		|| !Pawn
		|| !Items.IsValid()
		|| Items->GetRunState() != Edemo_mapRunState::Active)
	{
		Fail(TEXT("output, controller, pawn, or active Item Authority missing."));
		return;
	}
	if (bP6DualLootManualFixture && !bP6DualLootManualFixturePrepared)
	{
		auto AddOne = [this](FName DefinitionId, FGuid& OutId)
		{
			TArray<FGuid> Added;
			const Fdemo_mapItemOperationResult Result =
				Items->AddDefinition(DefinitionId, 1, &Added);
			if (!Result.bSuccess || Added.Num() != 1)
			{
				return false;
			}
			OutId = Added[0];
			return OutId.IsValid();
		};
		FGuid RingId;
		FGuid BagId;
		const bool bRingAdded = AddOne(Fdemo_mapItemIds::WindTalisman, RingId);
		const Fdemo_mapItemOperationResult RingEquip = bRingAdded
			? Items->Equip(RingId, Fdemo_mapItemIds::SpatialRingSlot)
			: Fdemo_mapItemOperationResult();
		const bool bBagAdded = AddOne(Fdemo_mapItemIds::BackpackLevel1, BagId);
		const Fdemo_mapItemOperationResult BagEquip = bBagAdded
			? Items->Equip(BagId, Fdemo_mapItemIds::BackpackSlot)
			: Fdemo_mapItemOperationResult();
		if (!bRingAdded || !RingEquip.bSuccess || !bBagAdded || !BagEquip.bSuccess)
		{
			UE_LOG(Logdemo_map, Error,
				TEXT("P6_DUAL_LOOT_MANUAL_FIXTURE: equipment setup failed ring_added=%d ring_equip=%d ring_reason=%s bag_added=%d bag_equip=%d bag_reason=%s."),
				bRingAdded ? 1 : 0,
				RingEquip.bSuccess ? 1 : 0,
				*RingEquip.Diagnostic,
				bBagAdded ? 1 : 0,
				BagEquip.bSuccess ? 1 : 0,
				*BagEquip.Diagnostic);
			Fail(TEXT("manual fixture could not equip the actual ring and Heaven Bag."));
			return;
		}
		for (int32 Index = 0; Index < 11; ++Index)
		{
			FGuid ItemId;
			if (!AddOne(Fdemo_mapItemIds::TrainingBlade, ItemId))
			{
				Fail(TEXT("manual fixture could not seed player bag coverage."));
				return;
			}
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FGuid ItemId;
			if (!AddOne(Fdemo_mapItemIds::HealingPillLevel1, ItemId))
			{
				Fail(TEXT("manual fixture could not seed consumable coverage."));
				return;
			}
		}
		if (Items->GetAuthority().GetRingQuickCapacity() != 4
			|| Items->GetAuthority().GetSpatialBagCapacity() != 36
			|| Items->GetAuthority().GetUsedInventorySlots() <= 10)
		{
			Fail(TEXT("manual fixture capacity or nested-storage projection is invalid."));
			return;
		}
		bP6DualLootManualFixturePrepared = true;
		UE_LOG(Logdemo_map, Log,
			TEXT("P6_DUAL_LOOT_MANUAL_FIXTURE: READY ring_quick=4 player_bag=36 player_items=%d."),
			Items->GetAuthority().GetUsedInventorySlots());
	}

	switch (VisibleStep)
	{
	case 0:
	{
		Ademo_mapLootChest* Chest = FindChest();
		if (!Chest)
		{
			Fail(TEXT("ordinary P4 prototype chest is missing."));
			return;
		}
		if (!Controller->IsGameplayInputAllowed())
		{
			Fail(TEXT("ordinary container started while gameplay input was locked."));
			return;
		}
		if (!MovePawnNear(Chest))
		{
			// This old visual route can be launched on the dense M01 topology,
			// where no navigation path is guaranteed to the selected chest.
			// Place the automation pawn at a legal interaction distance instead
			// of treating NavMesh availability as a UI/interaction failure.
			Pawn->SetActorLocation(
				Chest->GetInteractionLocation() - FVector(120.0f, 0.0f, 0.0f),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		// The visible P6 route uses the same precise pointer validation as a
		// player click.  Project the selected Chest into the live viewport and
		// drive the actual mouse position onto it before requesting interaction.
		FVector2D ChestScreenPosition;
		if (!Controller->ProjectWorldLocationToScreen(
			Chest->GetInteractionLocation(), ChestScreenPosition, true))
		{
			Fail(TEXT("ordinary container could not be projected into the player viewport."));
			return;
		}
		Controller->SetMouseLocation(
			FMath::RoundToInt(ChestScreenPosition.X),
			FMath::RoundToInt(ChestScreenPosition.Y));
		// P6 must use the same focus ownership / pointer revalidation route
		// as product input.  Calling the container directly bypasses the
		// manager that opens the dual-sided page.
		SetFocusedActor(Chest);
		const Fdemo_mapItemOperationResult Open = RequestInteractFocused();
		if (!Open.bSuccess)
		{
			Fail(TEXT("ordinary container focused interaction failed: ") + Open.Diagnostic);
			return;
		}
		if (!Chest->CompleteActionForAutomation().bSuccess)
		{
			Fail(TEXT("ordinary container opening action did not complete."));
			return;
		}
		if (!bSearchContainerOpen
			|| !SearchContainerWidget
			|| !SearchContainerWidget->GetViewState().bDualSided
			|| !SearchContainerWidget->GetViewState().bOrdinaryContainer)
		{
			Fail(TEXT("ordinary container did not open the real dual-sided page."));
			return;
		}
		const Fdemo_mapDualLootTargetRegionView* Grid =
			FindRegion(
				SearchContainerWidget->GetViewState(),
				Edemo_mapDualLootTargetRegion::ContainerGrid);
		if (!Grid
			|| Grid->Capacity
				!= Fdemo_mapSearchContainerPrototypeConfig::
					ChestPrototypeCapacity)
		{
			Fail(TEXT("ordinary container target grid projection is invalid."));
			return;
		}
		VisibleStep = 1;
		// Packaged Development builds may still be creating their first Slate
		// resources immediately after the real search page opens.  Give the
		// product action enough time to finish identifying the selected item
		// before this non-shipping acceptance harness observes its state.
		Schedule(1.00f);
		return;
	}
	case 1:
	{
		Ademo_mapLootChest* Chest = FindChest();
		const bool bClicked = Chest
			&& SearchContainerWidget
			&& SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Chest,
				0);
		const bool bSearching = Chest && Chest->IsContainerSearching();
		const Fdemo_mapRuntimeContainerResult Completed = Chest
			? Chest->CompleteActionForAutomation()
			: Fdemo_mapRuntimeContainerResult();
		if (!Chest || !SearchContainerWidget || !bClicked
			|| !bSearching || !Completed.bSuccess)
		{
			Fail(FString::Printf(
				TEXT("ordinary container search/identify path failed: click=%d searching=%d complete=%d diagnostic=%s."),
				bClicked ? 1 : 0,
				bSearching ? 1 : 0,
				Completed.bSuccess ? 1 : 0,
				*Completed.Diagnostic));
			return;
		}
		const Fdemo_mapRuntimeContainerSnapshot ChestSnapshotAfterSearch =
			Chest->GetContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Entry =
			FindEntry(
				ChestSnapshotAfterSearch,
				Edemo_mapRuntimeContainerSection::Chest,
				0);
		if (!Entry
			|| Entry->State
				!= Edemo_mapRuntimeContainerEntryState::Identified)
		{
			Fail(FString::Printf(
				TEXT("ordinary container item was not identified: entry=%d state=%d."),
				Entry ? 1 : 0,
				Entry ? static_cast<int32>(Entry->State) : -1));
			return;
		}
		CaptureVisible(TEXT("P6_CHEST_DUAL_LOOT_IDENTIFIED_1600x900.png"));
		VisibleStep = 2;
		Schedule(0.80f);
		return;
	}
	case 2:
	{
		Ademo_mapLootChest* Chest = FindChest();
		const Fdemo_mapRuntimeContainerSnapshot ChestSnapshotBeforeTake =
			Chest ? Chest->GetContainerSnapshot()
				: Fdemo_mapRuntimeContainerSnapshot();
		const Fdemo_mapRuntimeContainerEntrySnapshot* Before = Chest
			? FindEntry(
				ChestSnapshotBeforeTake,
				Edemo_mapRuntimeContainerSection::Chest,
				0)
			: nullptr;
		const FGuid ItemId = Before ? Before->ItemInstanceId : FGuid();
		if (!Chest
			|| !ItemId.IsValid()
			|| !SearchContainerWidget
			|| !SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Chest,
				0))
		{
			Fail(TEXT("ordinary container Take action failed."));
			return;
		}
		const Fdemo_mapItemInstance* Transferred =
			Items->GetAuthority().FindInstance(ItemId);
		if (!Transferred
			|| Transferred->OwnershipState
				!= Edemo_mapItemOwnershipState::Inventory
			|| Items->GetAuthority().FindInventorySlot(ItemId) == INDEX_NONE)
		{
			Fail(TEXT("ordinary container transfer did not preserve the GUID."));
			return;
		}
		CaptureVisible(TEXT("P6_CHEST_TRANSFERRED_1600x900.png"));
		VisibleStep = 3;
		Schedule(0.80f);
		return;
	}
	case 3:
	{
		if (!bP6DualLootManualFixture
			&& (!SearchContainerWidget
				|| !SearchContainerWidget->AutomationClickClose()
				|| bSearchContainerOpen
				|| !Controller->IsGameplayInputAllowed()))
		{
			Fail(TEXT("ordinary container close did not restore Gameplay."));
			return;
		}
		const FVector CorpseLocation =
			Pawn->GetActorLocation() + FVector(150.0f, 0.0f, 24.0f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ademo_mapCorpseContainerActor* Corpse =
			GetWorld()->SpawnActor<Ademo_mapCorpseContainerActor>(
				Ademo_mapCorpseContainerActor::StaticClass(),
				CorpseLocation,
				FRotator::ZeroRotator,
				Params);
		const TArray<Fdemo_mapRuntimeContainerSeedEntry> Seed =
		{
			{ Edemo_mapRuntimeContainerSection::Equipment, 0,
				Fdemo_mapItemIds::WeaponLevel1, 1 },
			{ Edemo_mapRuntimeContainerSection::Equipment, 1,
				Fdemo_mapItemIds::ArmorRobeLevel1, 1 },
			{ Edemo_mapRuntimeContainerSection::Equipment, 2,
				Fdemo_mapItemIds::EvasionCharm, 1 },
			{ Edemo_mapRuntimeContainerSection::Equipment, 3,
				Fdemo_mapItemIds::AccessoryLevel1, 1 },
			{ Edemo_mapRuntimeContainerSection::Equipment, 4,
				Fdemo_mapItemIds::BackpackLevel1, 1 },
			{ Edemo_mapRuntimeContainerSection::Backpack, 0,
				Fdemo_mapItemIds::HealingPillLevel1, 2 },
			{ Edemo_mapRuntimeContainerSection::Backpack, 6,
				Fdemo_mapItemIds::SpiritOreLevel1, 2 },
			{ Edemo_mapRuntimeContainerSection::Body, 0,
				Fdemo_mapItemIds::SoulBone, 1 }
		};
		if (!Corpse
			|| !Corpse->InitializeSearchContainer(
				this,
				Items.Get(),
				Items->GetActiveRunId(),
				Edemo_mapRuntimeContainerKind::Corpse,
				FName(TEXT("Automation.P6.Corpse")),
				Seed))
		{
			if (Corpse)
			{
				Corpse->Destroy();
			}
			Fail(TEXT("deterministic corpse fixture initialization failed."));
			return;
		}
		Corpses.Add(Corpse);
		if (!MovePawnNear(Corpse))
		{
			Pawn->SetActorLocation(
				Corpse->GetInteractionLocation() - FVector(120.0f, 0.0f, 0.0f),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		FVector2D CorpseScreenPosition;
		if (!Controller->ProjectWorldLocationToScreen(
			Corpse->GetInteractionLocation(), CorpseScreenPosition, true))
		{
			Fail(TEXT("corpse could not be projected into the player viewport."));
			return;
		}
		Controller->SetMouseLocation(
			FMath::RoundToInt(CorpseScreenPosition.X),
			FMath::RoundToInt(CorpseScreenPosition.Y));
		if ((SetFocusedActor(Corpse), !RequestInteractFocused().bSuccess)
			|| !Corpse->CompleteActionForAutomation().bSuccess
			|| !SearchContainerWidget
			|| !SearchContainerWidget->GetViewState().bCorpse)
		{
			Fail(TEXT("corpse did not open the shared dual-sided page."));
			return;
		}
		const Fdemo_mapSearchContainerViewState& View =
			SearchContainerWidget->GetViewState();
		const Fdemo_mapDualLootTargetRegionView* Weapon =
			FindRegion(View, Edemo_mapDualLootTargetRegion::Weapon);
		const Fdemo_mapDualLootTargetRegionView* Armor =
			FindRegion(View, Edemo_mapDualLootTargetRegion::Armor);
		const Fdemo_mapDualLootTargetRegionView* Accessory =
			FindRegion(View, Edemo_mapDualLootTargetRegion::Accessory);
		const Fdemo_mapDualLootTargetRegionView* SpatialItem =
			FindRegion(View, Edemo_mapDualLootTargetRegion::SpatialItem);
		const Fdemo_mapDualLootTargetRegionView* BaseQuick =
			FindRegion(View, Edemo_mapDualLootTargetRegion::BaseQuickItems);
		const Fdemo_mapDualLootTargetRegionView* SpatialStorage =
			FindRegion(View, Edemo_mapDualLootTargetRegion::SpatialStorage);
		const Fdemo_mapDualLootTargetRegionView* Body =
			FindRegion(View, Edemo_mapDualLootTargetRegion::Body);
		if (!Weapon || Weapon->Capacity != 1
			|| !Armor || Armor->Capacity != 1
			|| !Accessory || Accessory->Capacity != 1
			|| !SpatialItem || SpatialItem->Capacity != 1
			|| !BaseQuick || BaseQuick->Capacity != 6
			|| !SpatialStorage || SpatialStorage->Capacity != 36
			|| !Body || Body->Capacity != 2)
		{
			Fail(TEXT("corpse equipment/base/spatial/body projection is invalid."));
			return;
		}
		SearchContainerWidget->AutomationSelectSection(
			Edemo_mapRuntimeContainerSection::Equipment);
		CaptureVisible(TEXT("P6_CORPSE_EQUIPMENT_1600x900.png"));
		if (bP6DualLootManualFixture)
		{
			UE_LOG(Logdemo_map, Log,
				TEXT("P6_DUAL_LOOT_MANUAL_FIXTURE: OPENED corpse_bag=36; use real mouse/keyboard only."));
			return;
		}
		VisibleStep = 4;
		Schedule(0.80f);
		return;
	}
	case 4:
		if (!SearchContainerWidget
			|| !SearchContainerWidget->AutomationSelectSection(
				Edemo_mapRuntimeContainerSection::Backpack))
		{
			Fail(TEXT("corpse basic quick-item region was not selectable."));
			return;
		}
		CaptureVisible(TEXT("P6_CORPSE_BASE_QUICK_1600x900.png"));
		VisibleStep = 5;
		Schedule(0.80f);
		return;
	case 5:
	{
		Ademo_mapCorpseContainerActor* Corpse = FindP6Corpse();
		if (!Corpse
			|| !SearchContainerWidget
			|| !SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Backpack,
				6)
			|| !Corpse->IsContainerSearching()
			|| !Corpse->CompleteActionForAutomation().bSuccess)
		{
			Fail(TEXT("corpse spatial storage page/search path failed."));
			return;
		}
		CaptureVisible(TEXT("P6_CORPSE_SPATIAL_STORAGE_1600x900.png"));
		VisibleStep = 6;
		Schedule(0.80f);
		return;
	}
	case 6:
	{
		Ademo_mapCorpseContainerActor* Corpse = FindP6Corpse();
		if (!Corpse
			|| !SearchContainerWidget
			|| !SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Body,
				0)
			|| !Corpse->IsContainerSearching()
			|| !Corpse->CompleteActionForAutomation().bSuccess)
		{
			Fail(TEXT("corpse body container search path failed."));
			return;
		}
		CaptureVisible(TEXT("P6_CORPSE_BODY_IDENTIFIED_1600x900.png"));
		VisibleStep = 7;
		Schedule(0.80f);
		return;
	}
	case 7:
	{
		if (!SearchContainerWidget
			|| !SearchContainerWidget->AutomationClickEntry(
				Edemo_mapRuntimeContainerSection::Body,
				0)
			|| !SearchContainerWidget->AutomationClickClose()
			|| bSearchContainerOpen
			|| !Controller->IsGameplayInputAllowed()
			|| Controller->IsSearchContainerInputLocked()
			|| Controller->IsMoveInputIgnored()
			|| Controller->IsLookInputIgnored())
		{
			Fail(TEXT("corpse transfer/close did not restore Gameplay immediately."));
			return;
		}
		const Fdemo_mapInputBindingSettings& Settings =
			Fdemo_mapInputBindingSettings::Get();
		const bool bGameplayControls =
			Controller->DispatchAutomationKey(
				Settings.GetKey(Fdemo_mapInputActionIds::MoveForward))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(Fdemo_mapInputActionIds::PrimaryAttack))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(Fdemo_mapInputActionIds::SkillGroundCircle))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(Fdemo_mapInputActionIds::SkillSelfSector))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionIds::SkillStraightProjectile))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionRegistry::HotbarActionId(1)));
		const bool bInventoryOpenAfterDispatch =
			Controller->DispatchAutomationKey(
				Settings.GetKey(Fdemo_mapInputActionIds::Inventory))
			&& bInventoryOpen
			&& InventoryWidget;
		const bool bInventoryClosed =
			bInventoryOpenAfterDispatch
			&& InventoryWidget->AutomationClickClose()
			&& !bInventoryOpen
			&& Controller->IsGameplayInputAllowed();
		if (!bGameplayControls || !bInventoryClosed)
		{
			Fail(TEXT("W/LMB/Q/E/F/Inventory/1 post-close dispatch failed."));
			return;
		}
		CaptureVisible(TEXT("P6_GAMEPLAY_RESTORED_1600x900.png"));
		VisibleStep = 8;
		Schedule(0.80f);
		return;
	}
	case 8:
		PassAutomation(TEXT("P6_DUAL_LOOT_VISIBLE_ACCEPTANCE: PASS."));
		return;
	default:
		Fail(TEXT("invalid phase."));
		return;
	}
#endif
}

void Ademo_mapV3ProgressionManager::
	RunP5RuntimeVisibleAcceptanceStep()
{
#if !UE_BUILD_SHIPPING
	auto Schedule = [this](float Delay)
	{
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&Ademo_mapV3ProgressionManager::
				RunP5RuntimeVisibleAcceptanceStep,
			Delay,
			false);
	};
	if (VisibleOutputDirectory.IsEmpty()
		|| !Items.IsValid())
	{
		FailAutomation(
			TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: output or ItemSubsystem missing."));
		return;
	}
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller)
	{
		FailAutomation(
			TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: controller missing."));
		return;
	}

	switch (VisibleStep)
	{
	case 0:
		if (!Controller->IsGameplayInputAllowed())
		{
			FailAutomation(
				TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: Start Run is not playable."));
			return;
		}
		CaptureVisible(TEXT("P5_RUNTIME_HUD_1600x900.png"));
		VisibleStep = 1;
		Schedule(0.75f);
		return;
	case 1:
	{
		TArray<FGuid> BackpackIds;
		TArray<FGuid> PillIds;
		const Fdemo_mapItemOperationResult Backpack =
			Items->AddDefinition(
				Fdemo_mapItemIds::BackpackLevel2,
				1,
				&BackpackIds);
		const Fdemo_mapItemOperationResult EquipBackpack =
			Backpack.bSuccess && BackpackIds.Num() == 1
				? Items->Equip(
					BackpackIds[0],
					Fdemo_mapItemIds::BackpackSlot)
				: Backpack;
		const Fdemo_mapItemOperationResult Pill =
			Items->AddDefinition(
				Fdemo_mapItemIds::HealingPillLevel1,
				2,
				&PillIds);
		const FKey InventoryKey =
			Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Inventory);
		if (!Backpack.bSuccess
			|| !EquipBackpack.bSuccess
			|| !Pill.bSuccess
			|| PillIds.Num() != 1
			|| !Controller->DispatchAutomationKey(InventoryKey)
			|| !bInventoryOpen
			|| !InventoryWidget
			|| Controller->IsGameplayInputAllowed())
		{
			FailAutomation(
				TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: authority fixture or bound Inventory open."));
			return;
		}
		VisibleStep = 2;
		Schedule(0.75f);
		return;
	}
	case 2:
		CaptureVisible(
			TEXT("P5_RUNTIME_INVENTORY_1600x900.png"));
		VisibleStep = 3;
		Schedule(0.75f);
		return;
	case 3:
	{
		const TArray<FGuid> Pills =
			Items->GetAuthority()
				.FindInventoryInstancesByDefinition(
					Fdemo_mapItemIds::HealingPillLevel1);
		const int32 Source = Pills.Num() == 1
			? Items->GetAuthority().FindInventorySlot(Pills[0])
			: INDEX_NONE;
		if (!InventoryWidget
			|| Source < 0
			|| Source >= 6
			|| !InventoryWidget->AutomationClickInventorySlot(
				Source)
			|| !InventoryWidget->AutomationClickHotbarSlot(1)
			|| !Items->GetHotbarBindingSnapshot()
				.SlotBindings[0].IsValid()
			|| !InventoryWidget->AutomationClickMove()
			|| Items->GetAuthority().FindInventorySlot(Pills[0])
				< 6
			|| Items->GetHotbarBindingSnapshot()
				.SlotBindings[0].IsValid())
		{
			FailAutomation(
				TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: representative bind/move/resync operation."));
			return;
		}
		VisibleStep = 4;
		Schedule(0.55f);
		return;
	}
	case 4:
		CaptureVisible(
			TEXT("P5_RUNTIME_INVENTORY_OPERATION_1600x900.png"));
		VisibleStep = 5;
		Schedule(0.70f);
		return;
	case 5:
	{
		if (!InventoryWidget
			|| !InventoryWidget->AutomationClickClose()
			|| bInventoryOpen
			|| !Controller->IsGameplayInputAllowed())
		{
			FailAutomation(
				TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: close did not restore Gameplay immediately."));
			return;
		}
		const Fdemo_mapInputBindingSettings& Settings =
			Fdemo_mapInputBindingSettings::Get();
		const bool bControlsDispatch =
			Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionIds::MoveForward))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionIds::PrimaryAttack))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionIds::SkillGroundCircle))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionIds::SkillSelfSector))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionIds::
						SkillStraightProjectile))
			&& Controller->DispatchAutomationKey(
				Settings.GetKey(
					Fdemo_mapInputActionRegistry::
						HotbarActionId(1)))
			&& Controller->IsGameplayInputAllowed();
		if (!bControlsDispatch)
		{
			FailAutomation(
				TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: W/LMB/Q/E/F/1 dispatch after close."));
			return;
		}
		VisibleStep = 6;
		Schedule(0.55f);
		return;
	}
	case 6:
		CaptureVisible(
			TEXT("P5_RUNTIME_HUD_RESTORED_1600x900.png"));
		VisibleStep = 7;
		Schedule(0.70f);
		return;
	case 7:
		PassAutomation(
			TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: PASS."));
		return;
	default:
		FailAutomation(
			TEXT("P5_RUNTIME_VISIBLE_ACCEPTANCE: FAIL: invalid phase."));
		return;
	}
#endif
}

void Ademo_mapV3ProgressionManager::RunVisibleAcceptanceStep()
{
#if !UE_BUILD_SHIPPING
	Udemo_mapInventoryWidget* Widget = InventoryWidget;
	switch (VisibleStep++)
	{
	case 0:
		if (VisibleOutputDirectory.IsEmpty() || !InitialWorldItems.IsValidIndex(0) || !MovePawnNear(InitialWorldItems[0].Get())) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: focus setup.")); return; }
		CaptureVisible(TEXT("V3_MAP_INTERACTION_FOCUS.png")); ScheduleVisibleStep(0.65f); return;
	case 1:
		if (!Chests.IsValidIndex(0) || !MovePawnNear(Chests[0].Get())) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: chest focus.")); return; }
		PressBoundKey(EKeys::G);
		if (!Chests[0]->IsOpened()) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: chest input.")); return; }
		CaptureVisible(TEXT("V3_CHEST_LOOT_WORLD.png")); ScheduleVisibleStep(0.65f); return;
	case 2:
		if (!InitialWorldItems.IsValidIndex(0) || !InitialWorldItems[0].IsValid() || !MovePawnNear(InitialWorldItems[0].Get())) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: inventory pickup focus.")); return; }
		PressBoundKey(EKeys::G); PressBoundKey(EKeys::Tab);
		if (!InventoryWidget || !bInventoryOpen) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: inventory setup.")); return; }
		CaptureVisible(TEXT("V3_INVENTORY_EQUIPMENT_UI.png")); ScheduleVisibleStep(0.65f); return;
	case 3:
		Widget = InventoryWidget;
		if (!Widget) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: widget disappeared.")); return; }
		{
			const FGuid HeavyId = Items->GetAuthority().FindInventoryInstancesByDefinition(Fdemo_mapItemIds::HeavyPracticeBlade)[0];
			const int32 Slot = Items->GetAuthority().FindInventorySlot(HeavyId);
			if (!Widget->AutomationClickInventorySlot(Slot) || !Widget->AutomationClickEquip()) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: equip buttons.")); return; }
		}
		CaptureVisible(TEXT("V3_EQUIPPED_ATTRIBUTE_CHANGE.png")); ScheduleVisibleStep(0.65f); return;
	case 4:
		Widget = InventoryWidget;
		if (!Widget || !Widget->AutomationClickEquipmentSlot(Fdemo_mapItemIds::WeaponSlot) || !Widget->AutomationClickUnequip()) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: unequip.")); return; }
		{
			const FGuid HeavyId = Items->GetAuthority().FindInventoryInstancesByDefinition(Fdemo_mapItemIds::HeavyPracticeBlade)[0];
			const int32 Slot = Items->GetAuthority().FindInventorySlot(HeavyId);
			if (!Widget->AutomationClickInventorySlot(Slot) || !Widget->AutomationClickDrop() || !Widget->AutomationClickClose()) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: drop/close buttons.")); return; }
			Ademo_mapWorldItem* Dropped = Items->GetWorldActor(HeavyId);
			if (!Dropped || !MovePawnNear(Dropped)) { FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: dropped focus.")); return; }
		}
		CaptureVisible(TEXT("V3_DROP_BACK_TO_WORLD.png")); ScheduleVisibleStep(0.80f); return;
	case 5:
		PassAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: PASS.")); return;
	default:
		FailAutomation(TEXT("V3_WORLD_UI_VISIBLE_ACCEPTANCE: FAIL: invalid phase.")); return;
	}
#endif
}

void Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep()
{
#if !UE_BUILD_SHIPPING
	if (VisibleOutputDirectory.IsEmpty()) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: output directory missing.")); return; }
	if (GRepairVisiblePhase == 0)
	{
		switch (VisibleStep++)
		{
		case 0:
		{
			Ademo_mapTrainingTarget* Target = nullptr;
			for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) { Target = *It; break; }
			if (!Target || !PlayerPawn.IsValid()) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: close target setup.")); return; }
			const FVector Base = PlayerPawn->GetActorLocation();
			Target->SetActorLocation(Base + FVector(110.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			PlayerPawn->SetActorRotation(FRotator::ZeroRotator);
			if (Ademo_mapPlayerController* Controller = GetDemoController()) Controller->SetAutomationAimDirection(FVector::ForwardVector);
			if (!PressBoundKey(EKeys::F)) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: real close F.")); return; }
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.08f, false);
			return;
		}
		case 1:
			CaptureVisible(TEXT("V3_0342_CLOSE_RANGE_F_HIT.png"));
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.70f, false);
			return;
		case 2:
		{
			Ademo_mapLootChest* Chest = Chests.IsValidIndex(0) ? Chests[0].Get() : nullptr;
			Ademo_mapWorldItem* Item = InitialWorldItems.IsValidIndex(0) ? InitialWorldItems[0].Get() : nullptr;
			if (!Chest || !Item || !MovePawnNear(Chest)) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: closed chest setup.")); return; }
			Item->SetActorLocation(Chest->GetActorLocation() + FVector(0.0f, 360.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			Item->RefreshPresentation();
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.90f, false);
			return;
		}
		case 3:
		{
			Ademo_mapWorldItem* Item = InitialWorldItems.IsValidIndex(0) ? InitialWorldItems[0].Get() : nullptr;
			if (!Item) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: closed item disappeared.")); return; }
			Item->RefreshPresentation();
			CaptureVisible(TEXT("V3_0342_CLOSED_CHEST_AND_LABELS.png"));
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.75f, false);
			return;
		}
		case 4:
		{
			Ademo_mapLootChest* Chest = Chests.IsValidIndex(0) ? Chests[0].Get() : nullptr;
			Ademo_mapWorldItem* DisplayItem = InitialWorldItems.IsValidIndex(0) ? InitialWorldItems[0].Get() : nullptr;
			if (Chest && DisplayItem)
			{
				DisplayItem->SetActorLocation(Chest->GetActorLocation() + FVector(0.0f, 600.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
				DisplayItem->RefreshPresentation();
			}
			if (!Chest || !MovePawnNear(Chest) || !PressBoundKey(EKeys::G) || !Chest->HasOpenedPresentation()) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: opened chest setup.")); return; }
			RefreshFocusNow();
			if (GetFocusedActor() == Chest) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: opened chest retained focus.")); return; }
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.40f, false);
			return;
		}
		case 5:
		{
			Ademo_mapLootChest* Chest = Chests.IsValidIndex(0) ? Chests[0].Get() : nullptr;
			if (!Chest || !Chest->HasOpenedPresentation()) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: opened chest state lost.")); return; }
			for (const TWeakObjectPtr<Ademo_mapWorldItem>& Loot : Chest->GetSpawnedLoot()) if (Loot.IsValid()) Loot->RefreshPresentation();
			CaptureVisible(TEXT("V3_0342_OPENED_CHEST_AND_LOOT.png"));
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.80f, false);
			return;
		}
		case 6:
			if (!Items.IsValid() || !Items->AddDefinition(Fdemo_mapItemIds::AncientToken, 1).bSuccess) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: extraction item setup.")); return; }
			GRepairVisiblePhase = 1;
			if (!RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction).bSuccess) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: extraction request.")); return; }
			return;
		default: FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: invalid pre-reload phase.")); return;
		}
	}

	if (GRepairVisiblePhase == 1)
	{
		if (VisibleStep == 0)
		{
			if (!Items.IsValid() || Items->GetRunState() != Edemo_mapRunState::Active || Items->GetSessionStashItemCount() < 1 || Items->GetAuthority().GetUsedInventorySlots() != 0 || SettlementWidget || !GetDemoController() || !GetDemoController()->IsGameplayInputAllowed())
			{
				FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: post-extraction new-run state.")); return;
			}
			for (FName SlotId : Fdemo_mapItemDefinitions::GetEquipmentSlotIds()) if (Items->GetAuthority().GetEquippedInstance(SlotId).IsValid()) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: post-extraction equipment not empty.")); return; }
			const FVector SpawnLocation = PlayerPawn->GetActorLocation();
			PlayerPawn->SetActorLocation(SpawnLocation + FVector(220.0f, 80.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			GetDemoController()->SetAutomationAimDirection(FVector::ForwardVector);
			if (!PressBoundKey(EKeys::F) || !PressBoundKey(EKeys::Tab) || !bInventoryOpen) { FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: post-extraction real controls.")); return; }
			++VisibleStep;
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.25f, false);
			return;
		}
		if (VisibleStep == 1)
		{
			CaptureVisible(TEXT("V3_0342_POST_EXTRACTION_CONTROLS.png"));
			++VisibleStep;
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunRepairVisibleAcceptanceStep, 0.85f, false);
			return;
		}
		if (VisibleStep == 2)
		{
			GRepairVisiblePhase = 0;
			PassAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: PASS."));
			return;
		}
	}
	FailAutomation(TEXT("V3_0342_VISIBLE_ACCEPTANCE: FAIL: invalid reload phase."));
#endif
}

bool Ademo_mapV3ProgressionManager::ValidateFinalNewRunState(const TCHAR* RoundName, Edemo_mapRunEndReason ExpectedReason, int32 ExpectedStashQuantity, int32 ExpectedStashValue)
{
#if !UE_BUILD_SHIPPING
	if (!Items.IsValid() || Items->GetRunState() != Edemo_mapRunState::Active || !PlayerPawn.IsValid()) return false;
	const Fdemo_mapSettlementSummary& Summary = Items->GetLastSettlementSummary();
	if (!Summary.bValid || Summary.Reason != ExpectedReason || !Summary.RunId.IsValid() || GV3FinalRunIds.Contains(Summary.RunId)) return false;
	GV3FinalRunIds.Add(Summary.RunId);
	const FGuid SettlementId = FGuid::NewGuid();
	if (!SettlementId.IsValid() || GV3FinalSettlementIds.Contains(SettlementId)) return false;
	GV3FinalSettlementIds.Add(SettlementId);
	if (Items->GetSessionStashItemCount() != ExpectedStashQuantity || Items->GetSessionStashValue() != ExpectedStashValue) return false;
	if (Items->GetAuthority().GetUsedInventorySlots() != 0 || !Items->GetActiveModifierSources().IsEmpty()) return false;
	for (FName SlotId : Fdemo_mapItemDefinitions::GetEquipmentSlotIds()) if (Items->GetAuthority().GetEquippedInstance(SlotId).IsValid()) return false;
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller || Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored() || !Controller->IsGameplayInputAllowed()) return false;
	Udemo_mapAttributeComponent* Attributes = PlayerPawn->FindComponentByClass<Udemo_mapAttributeComponent>();
	float AttackPower = 0.0f;
	if (!Attributes || !Attributes->GetFinalValue(Fdemo_mapAttributeIds::AttackPower, AttackPower) || !FMath::IsNearlyEqual(AttackPower, 1.0f)) return false;
	FString InvariantError;
	if (!Items->ValidateInvariants(&InvariantError)) return false;
	UE_LOG(Logdemo_map, Log, TEXT("V3_FINAL_SETTLEMENT_ID round=%s settlement=%s"), RoundName, *SettlementId.ToString(EGuidFormats::DigitsWithHyphens));
	LogFinalAutomationSnapshot(RoundName, Summary);
	return true;
#else
	return false;
#endif
}

void Ademo_mapV3ProgressionManager::LogFinalAutomationSnapshot(const TCHAR* RoundName, const Fdemo_mapSettlementSummary& Summary) const
{
#if !UE_BUILD_SHIPPING
	int32 EquipmentCount = 0;
	for (FName SlotId : Fdemo_mapItemDefinitions::GetEquipmentSlotIds()) if (Items->GetAuthority().GetEquippedInstance(SlotId).IsValid()) ++EquipmentCount;
	int32 ProjectileCount = 0, AIControllerCount = 0, ChestCount = 0, ManagerCount = 0, PlayerControllerCount = 0;
	for (TActorIterator<Ademo_mapSkillProjectile> It(GetWorld()); It; ++It) ++ProjectileCount;
	for (TActorIterator<AAIController> It(GetWorld()); It; ++It) ++AIControllerCount;
	for (TActorIterator<Ademo_mapLootChest> It(GetWorld()); It; ++It) ++ChestCount;
	for (TActorIterator<Ademo_mapV3ProgressionManager> It(GetWorld()); It; ++It) ++ManagerCount;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It) ++PlayerControllerCount;
	int32 InventoryWidgetCount = 0, SettlementWidgetCount = 0, DestroyedPending = 0;
	for (TObjectIterator<Udemo_mapInventoryWidget> It; It; ++It) if (It->GetWorld() == GetWorld() && It->IsInViewport()) ++InventoryWidgetCount;
	for (TObjectIterator<Udemo_mapSettlementWidget> It; It; ++It) if (It->GetWorld() == GetWorld() && It->IsInViewport()) ++SettlementWidgetCount;
	for (const TPair<FGuid, Fdemo_mapItemInstance>& Pair : Items->GetAuthority().GetInstanceSnapshot()) if (Pair.Value.OwnershipState == Edemo_mapItemOwnershipState::Destroyed) ++DestroyedPending;
	UE_LOG(Logdemo_map, Log, TEXT("V3_FINAL_SNAPSHOT round=%s run=%s reason=%d inventory_stacks=%d equipment=%d world_instances=%d world_actors=%d stash_instances=%d stash_quantity=%d stash_value=%d modifier_sources=%d projectiles=%d ai_controllers=%d chests=%d managers=%d player_controllers=%d settlement_widgets=%d inventory_widgets=%d destroyed_pending=%d timer_active=%d"),
		RoundName, *Summary.RunId.ToString(EGuidFormats::DigitsWithHyphens), static_cast<int32>(Summary.Reason), Items->GetAuthority().GetUsedInventorySlots(), EquipmentCount,
		Items->GetAuthority().FindWorldInstances().Num(), Items->GetWorldActorCount(), Items->GetAuthority().GetSessionStashSnapshot().Num(), Items->GetSessionStashItemCount(), Items->GetSessionStashValue(),
		Items->GetActiveModifierSources().Num(), ProjectileCount, AIControllerCount, ChestCount, ManagerCount, PlayerControllerCount, SettlementWidgetCount, InventoryWidgetCount, DestroyedPending,
		GetWorldTimerManager().IsTimerActive(AutomationTimer) ? 1 : 0);
#endif
}

bool Ademo_mapV3ProgressionManager::CompleteTrainingMissionAndEnterExit(int32 NextGlobalPhase)
{
#if !UE_BUILD_SHIPPING
	Ademo_mapGameState* Mission = GetWorld() ? Cast<Ademo_mapGameState>(GetWorld()->GetGameState()) : nullptr;
	Ademo_mapExitZone* Exit = nullptr;
	for (TActorIterator<Ademo_mapExitZone> It(GetWorld()); It; ++It) { Exit = *It; break; }
	if (!Mission || !Exit || !Exit->IsExitUnlocked() || !Mission->CanUseExit()) return false;
	GV3FinalAutomationPhase = NextGlobalPhase;
	PlayerPawn->SetActorLocation(Exit->GetActorLocation() + FVector(450.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->SetActorLocation(Exit->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (UPrimitiveComponent* PawnPrimitive = Cast<UPrimitiveComponent>(PlayerPawn->GetRootComponent())) PawnPrimitive->UpdateOverlaps();
	Exit->GetTriggerComponent()->UpdateOverlaps();
	if (!bSettlementPending)
	{
		FHitResult TriggerHit;
		UPrimitiveComponent* PawnPrimitive = Cast<UPrimitiveComponent>(PlayerPawn->GetRootComponent());
		Exit->GetTriggerComponent()->OnComponentBeginOverlap.Broadcast(Exit->GetTriggerComponent(), PlayerPawn.Get(), PawnPrimitive, 0, false, TriggerHit);
	}
	return bSettlementPending;
#else
	return false;
#endif
}

void Ademo_mapV3ProgressionManager::RunV3FinalAutomation()
{
#if !UE_BUILD_SHIPPING
	auto Fail = [this](const FString& Detail) { FailAutomation(TEXT("V3_FINAL_AUTOMATION: FAIL: ") + Detail); };
	if (!Items.IsValid() || !PlayerPawn.IsValid() || Chests.Num() != 3) { Fail(TEXT("V3 foundation missing.")); return; }

	if (GV3FinalAutomationPhase == 0)
	{
		if (Items->GetSessionStashItemCount() != 0) { Fail(TEXT("fresh process stash was not empty.")); return; }
		Ademo_mapLootChest* Chest = Chests[0].Get();
		if (!Chest || !MovePawnNear(Chest) || !PressBoundKey(EKeys::G) || !Chest->IsOpened()) { Fail(TEXT("round 1 real chest open.")); return; }
		const TArray<TWeakObjectPtr<Ademo_mapWorldItem>> LootActors = Chest->GetSpawnedLoot();
		for (const TWeakObjectPtr<Ademo_mapWorldItem>& Loot : LootActors)
		{
			if (!Loot.IsValid() || !MovePawnNear(Loot.Get()) || !PressBoundKey(EKeys::G)) { Fail(TEXT("round 1 real loot pickup.")); return; }
		}
		const TArray<FGuid> Blades = Items->GetAuthority().FindInventoryInstancesByDefinition(Fdemo_mapItemIds::TrainingBlade);
		if (Blades.Num() != 1 || !Items->Equip(Blades[0], Fdemo_mapItemIds::WeaponSlot).bSuccess) { Fail(TEXT("round 1 weapon equip.")); return; }
		float AttackPower = 0.0f;
		Udemo_mapAttributeComponent* Attributes = PlayerPawn->FindComponentByClass<Udemo_mapAttributeComponent>();
		if (!Attributes || !Attributes->GetFinalValue(Fdemo_mapAttributeIds::AttackPower, AttackPower) || !FMath::IsNearlyEqual(AttackPower, 2.0f)) { Fail(TEXT("round 1 equipped attribute.")); return; }
		Ademo_mapEnemyCharacter* Melee = nullptr;
		for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) { Melee = *It; break; }
		if (!Melee) { Fail(TEXT("round 1 melee enemy missing.")); return; }
		UGameplayStatics::ApplyDamage(Melee, 100.0f, GetDemoController(), PlayerPawn.Get(), nullptr);
		Ademo_mapWorldItem* EnemyDrop = nullptr;
		for (const FGuid& Id : Items->GetAuthority().FindWorldInstances())
		{
			const Fdemo_mapItemInstance* Instance = Items->GetAuthority().FindInstance(Id);
			if (Instance && Instance->DefinitionId == Fdemo_mapItemIds::SpiritDust) { EnemyDrop = Items->GetWorldActor(Id); break; }
		}
		if (!EnemyDrop || !MovePawnNear(EnemyDrop) || !PressBoundKey(EKeys::G)) { Fail(TEXT("round 1 enemy drop pickup.")); return; }
		Ademo_mapRangedEnemyCharacter* DamageEnemy = nullptr;
		for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) { DamageEnemy = *It; break; }
		Udemo_mapPlayerHealthComponent* Health = PlayerPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
		if (!DamageEnemy || !Health) { Fail(TEXT("round 1 enemy damage setup.")); return; }
		Health->SetCurrentHealthForAutomation(1);
		GV3FinalAutomationPhase = 1;
		UGameplayStatics::ApplyDamage(PlayerPawn.Get(), 1.0f, DamageEnemy->GetController(), DamageEnemy, nullptr);
		if (!bSettlementPending || !Health->IsDefeated()) { Fail(TEXT("round 1 real enemy damage did not settle Death.")); return; }
		return;
	}

	if (GV3FinalAutomationPhase == 1)
	{
		if (FinalAutomationStep == 0)
		{
			if (!ValidateFinalNewRunState(TEXT("Death"), Edemo_mapRunEndReason::Death, 0, 0)) { Fail(TEXT("round 1 post-reload state.")); return; }
			TArray<FGuid> BladeIds, AccessoryIds, DustIds;
			if (!Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade, 1, &BladeIds).bSuccess || !Items->AddDefinition(Fdemo_mapItemIds::WindTalisman, 1, &AccessoryIds).bSuccess || !Items->AddDefinition(Fdemo_mapItemIds::SpiritDust, 3, &DustIds).bSuccess || BladeIds.Num() != 1 || AccessoryIds.Num() != 1 || DustIds.Num() != 1) { Fail(TEXT("round 2 item setup.")); return; }
			if (!Items->Equip(BladeIds[0], Fdemo_mapItemIds::WeaponSlot).bSuccess || !Items->Equip(AccessoryIds[0], Fdemo_mapItemIds::SpatialRingSlot).bSuccess) { Fail(TEXT("round 2 equipment setup.")); return; }
			const FGuid RoundRunId = Items->GetActiveRunId();
			GV3FinalExpectedStashOrigins.Add(BladeIds[0], RoundRunId); GV3FinalExpectedStashOrigins.Add(AccessoryIds[0], RoundRunId); GV3FinalExpectedStashOrigins.Add(DustIds[0], RoundRunId);
			GV3FinalExpectedStashQuantity = 5;
			GV3FinalExpectedStashValue = 163;
			for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) UGameplayStatics::ApplyDamage(*It, 2.0f, GetDemoController(), PlayerPawn.Get(), nullptr);
			FinalAutomationStep = 1;
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunV3FinalAutomation, 0.45f, false);
			return;
		}
		if (!CompleteTrainingMissionAndEnterExit(2)) { Fail(TEXT("round 2 real mission/exit extraction.")); return; }
		return;
	}

	if (GV3FinalAutomationPhase == 2)
	{
		if (!ValidateFinalNewRunState(TEXT("Extraction1"), Edemo_mapRunEndReason::Extraction, GV3FinalExpectedStashQuantity, GV3FinalExpectedStashValue)) { Fail(TEXT("round 2 post-reload state.")); return; }
		for (const TPair<FGuid, FGuid>& Pair : GV3FinalExpectedStashOrigins)
		{
			const Fdemo_mapItemInstance* Item = Items->GetAuthority().FindInstance(Pair.Key);
			if (!Item || Item->OwnershipState != Edemo_mapItemOwnershipState::SessionStash || Item->OriginRunId != Pair.Value) { Fail(TEXT("round 2 stash GUID/origin preservation.")); return; }
		}
		auto StashIterator = GV3FinalExpectedStashOrigins.CreateConstIterator();
		const FGuid ReadOnlyId = StashIterator.Key();
		Ademo_mapWorldItem* IllegalDrop = nullptr;
		if (Items->Equip(ReadOnlyId, Fdemo_mapItemIds::WeaponSlot).bSuccess || Items->DropInventoryItem(ReadOnlyId, PlayerPawn.Get(), IllegalDrop).bSuccess || IllegalDrop) { Fail(TEXT("round 2 stash read-only boundary.")); return; }
		if (!PressBoundKey(EKeys::W) || !PressBoundKey(EKeys::LeftMouseButton) || !PressBoundKey(EKeys::Q) || !PressBoundKey(EKeys::E) || !PressBoundKey(EKeys::F) || !PressBoundKey(EKeys::G) || !PressBoundKey(EKeys::Tab) || !bInventoryOpen || !PressBoundKey(EKeys::Tab) || bInventoryOpen) { Fail(TEXT("round 2 post-extraction controls.")); return; }
		TArray<FGuid> AbandonBlade;
		if (!Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade, 1, &AbandonBlade).bSuccess || AbandonBlade.Num() != 1 || !Items->Equip(AbandonBlade[0], Fdemo_mapItemIds::WeaponSlot).bSuccess) { Fail(TEXT("round 3 setup.")); return; }
		GV3FinalAutomationPhase = 3;
		if (!PressBoundKey(EKeys::R) || !bSettlementPending) { Fail(TEXT("round 3 real R abandon.")); return; }
		return;
	}

	if (GV3FinalAutomationPhase == 3)
	{
		if (FinalAutomationStep == 0)
		{
			if (!ValidateFinalNewRunState(TEXT("Abandon"), Edemo_mapRunEndReason::Abandon, GV3FinalExpectedStashQuantity, GV3FinalExpectedStashValue)) { Fail(TEXT("round 3 post-reload state.")); return; }
			TArray<FGuid> TokenIds, DustIds;
			if (!Items->AddDefinition(Fdemo_mapItemIds::AncientToken, 1, &TokenIds).bSuccess || !Items->AddDefinition(Fdemo_mapItemIds::SpiritDust, 2, &DustIds).bSuccess || TokenIds.Num() != 1 || DustIds.Num() != 1) { Fail(TEXT("round 4 item setup.")); return; }
			const FGuid RoundRunId = Items->GetActiveRunId();
			GV3FinalExpectedStashOrigins.Add(TokenIds[0], RoundRunId); GV3FinalExpectedStashOrigins.Add(DustIds[0], RoundRunId);
			GV3FinalExpectedStashQuantity = 8;
			GV3FinalExpectedStashValue = 665;
			for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) UGameplayStatics::ApplyDamage(*It, 2.0f, GetDemoController(), PlayerPawn.Get(), nullptr);
			FinalAutomationStep = 1;
			GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunV3FinalAutomation, 0.45f, false);
			return;
		}
		if (!CompleteTrainingMissionAndEnterExit(4)) { Fail(TEXT("round 4 real mission/exit extraction.")); return; }
		return;
	}

	if (GV3FinalAutomationPhase == 4)
	{
		if (!ValidateFinalNewRunState(TEXT("Extraction2"), Edemo_mapRunEndReason::Extraction, GV3FinalExpectedStashQuantity, GV3FinalExpectedStashValue)) { Fail(TEXT("round 4 post-reload state.")); return; }
		TSet<FGuid> UniqueStashIds;
		for (const FGuid& Id : Items->GetAuthority().GetSessionStashSnapshot())
		{
			if (UniqueStashIds.Contains(Id)) { Fail(TEXT("round 4 duplicate stash GUID.")); return; }
			UniqueStashIds.Add(Id);
			const FGuid* ExpectedRun = GV3FinalExpectedStashOrigins.Find(Id);
			const Fdemo_mapItemInstance* Item = Items->GetAuthority().FindInstance(Id);
			if (!ExpectedRun || !Item || Item->OriginRunId != *ExpectedRun) { Fail(TEXT("round 4 stash identity.")); return; }
		}
		int32 ManagerCount = 0, ControllerCount = 0, AIControllerCount = 0, ProjectileCount = 0;
		for (TActorIterator<Ademo_mapV3ProgressionManager> It(GetWorld()); It; ++It) ++ManagerCount;
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It) ++ControllerCount;
		for (TActorIterator<AAIController> It(GetWorld()); It; ++It) ++AIControllerCount;
		for (TActorIterator<Ademo_mapSkillProjectile> It(GetWorld()); It; ++It) ++ProjectileCount;
		if (GV3FinalRunIds.Num() != 4 || GV3FinalSettlementIds.Num() != 4 || Chests.Num() != 3 || ManagerCount != 1 || ControllerCount != 1 || AIControllerCount != 3 || ProjectileCount != 0 || Items->GetWorldActorCount() != 3 || Items->GetAuthority().FindWorldInstances().Num() != 3) { Fail(TEXT("final actor/count stability.")); return; }
		UE_LOG(Logdemo_map, Log, TEXT("V3_FINAL_AUTOMATION: four rounds passed; runs=4 settlements=4 stash_instances=%d stash_quantity=%d stash_value=%d chests=3 managers=1 player_controllers=1 ai_controllers=3 projectiles=0."), UniqueStashIds.Num(), Items->GetSessionStashItemCount(), Items->GetSessionStashValue());
		GV3FinalAutomationPhase = 0; GV3FinalRunIds.Reset(); GV3FinalSettlementIds.Reset(); GV3FinalExpectedStashOrigins.Reset(); GV3FinalExpectedStashQuantity = 0; GV3FinalExpectedStashValue = 0;
		PassAutomation(TEXT("V3_FINAL_AUTOMATION: PASS."));
		return;
	}
	Fail(TEXT("invalid global phase."));
#endif
}

void Ademo_mapV3ProgressionManager::RunV3FinalVisibleAcceptance()
{
#if !UE_BUILD_SHIPPING
	auto ScheduleFinalVisible = [this](float Delay) { GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunV3FinalVisibleAcceptance, Delay, false); };
	if (VisibleOutputDirectory.IsEmpty()) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: output directory missing.")); return; }
	if (GV3FinalVisiblePhase == 0)
	{
		switch (VisibleStep++)
		{
		case 0:
			if (!Chests[0].IsValid() || !MovePawnNear(Chests[0].Get())) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: closed chest setup.")); return; }
			ScheduleFinalVisible(1.50f); return;
		case 1:
			CaptureVisible(TEXT("V3_FINAL_CLOSED_CHEST_LABELS.png")); ScheduleFinalVisible(0.50f); return;
		case 2:
			if (!PressBoundKey(EKeys::G) || !Chests[0]->HasOpenedPresentation()) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: opened chest setup.")); return; }
			ScheduleFinalVisible(1.00f); return;
		case 3:
			CaptureVisible(TEXT("V3_FINAL_OPENED_CHEST_LOOT.png")); ScheduleFinalVisible(0.50f); return;
		case 4:
		{
			const TArray<TWeakObjectPtr<Ademo_mapWorldItem>> LootActors = Chests[0]->GetSpawnedLoot();
			for (const TWeakObjectPtr<Ademo_mapWorldItem>& Loot : LootActors) if (Loot.IsValid() && MovePawnNear(Loot.Get())) PressBoundKey(EKeys::G);
			const TArray<FGuid> Blades = Items->GetAuthority().FindInventoryInstancesByDefinition(Fdemo_mapItemIds::TrainingBlade);
			if (Blades.Num() != 1 || !PressBoundKey(EKeys::Tab) || !InventoryWidget || !InventoryWidget->AutomationClickInventorySlot(Items->GetAuthority().FindInventorySlot(Blades[0])) || !InventoryWidget->AutomationClickEquip()) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: inventory/equipment setup.")); return; }
			ScheduleFinalVisible(1.50f); return;
		}
		case 5:
			CaptureVisible(TEXT("V3_FINAL_INVENTORY_EQUIPMENT_ATTRIBUTES.png")); ScheduleFinalVisible(0.65f); return;
		case 6:
		{
			CloseInventory();
			Ademo_mapTrainingTarget* Target = nullptr;
			for (TActorIterator<Ademo_mapTrainingTarget> It(GetWorld()); It; ++It) { Target = *It; break; }
			if (!Target || !MovePawnNear(Target, 300.0f)) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: close projectile setup.")); return; }
			if (Ademo_mapPlayerController* Controller = GetDemoController()) Controller->SetAutomationAimDirection((Target->GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal());
			ScheduleFinalVisible(1.50f); return;
		}
		case 7:
			if (!PressBoundKey(EKeys::F)) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: close projectile dispatch.")); return; }
			CaptureVisible(TEXT("V3_FINAL_CLOSE_RANGE_PROJECTILE.png")); ScheduleFinalVisible(0.65f); return;
		case 8:
			for (TActorIterator<Ademo_mapEnemyCharacter> It(GetWorld()); It; ++It) { UGameplayStatics::ApplyDamage(*It, 100.0f, GetDemoController(), PlayerPawn.Get(), nullptr); break; }
			for (TActorIterator<Ademo_mapRangedEnemyCharacter> It(GetWorld()); It; ++It) { UGameplayStatics::ApplyDamage(*It, 100.0f, GetDemoController(), PlayerPawn.Get(), nullptr); break; }
			for (TActorIterator<Ademo_mapHeavyEnemyCharacter> It(GetWorld()); It; ++It) { UGameplayStatics::ApplyDamage(*It, 100.0f, GetDemoController(), PlayerPawn.Get(), nullptr); break; }
			ScheduleFinalVisible(0.30f); return;
		case 9:
		{
			Ademo_mapWorldItem* EnemyDrop = nullptr;
			for (const FGuid& Id : Items->GetAuthority().FindWorldInstances())
			{
				const Fdemo_mapItemInstance* Instance = Items->GetAuthority().FindInstance(Id);
				if (Instance && Instance->DefinitionId == Fdemo_mapItemIds::SpiritDust) { EnemyDrop = Items->GetWorldActor(Id); break; }
			}
			if (!EnemyDrop || !MovePawnNear(EnemyDrop, 150.0f)) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: enemy drop framing.")); return; }
			ScheduleFinalVisible(1.50f); return;
		}
		case 10:
			CaptureVisible(TEXT("V3_FINAL_ENEMY_DROPS.png")); ScheduleFinalVisible(0.70f); return;
		case 11:
			Items->AddDefinition(Fdemo_mapItemIds::TrainingBlade, 1); GV3FinalVisiblePhase = 1;
			if (!RequestSettlementAndReload(Edemo_mapRunEndReason::Death).bSuccess) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: death settlement.")); return; }
			ScheduleFinalVisible(0.35f); return;
		default: FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: invalid initial phase.")); return;
		}
	}
	if (GV3FinalVisiblePhase == 1)
	{
		CaptureVisible(TEXT("V3_FINAL_DEATH_SETTLEMENT.png"));
		GV3FinalVisiblePhase = 2;
		return;
	}
	if (GV3FinalVisiblePhase == 2)
	{
		Items->AddDefinition(Fdemo_mapItemIds::AncientToken, 1); GV3FinalVisiblePhase = 3;
		if (!RequestSettlementAndReload(Edemo_mapRunEndReason::Extraction).bSuccess) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: extraction settlement.")); return; }
		return;
	}
	if (GV3FinalVisiblePhase == 3)
	{
		if (VisibleStep == 0) { OpenInventory(); VisibleStep = 1; ScheduleFinalVisible(0.50f); return; }
		if (VisibleStep == 1) { CaptureVisible(TEXT("V3_FINAL_EXTRACTION_STASH_NEW_RUN.png")); VisibleStep = 2; ScheduleFinalVisible(0.75f); return; }
		if (VisibleStep == 2)
		{
			CloseInventory(); Items->AddDefinition(Fdemo_mapItemIds::SpiritDust, 1); GV3FinalVisiblePhase = 4; VisibleStep = 0;
			if (!RequestSettlementAndReload(Edemo_mapRunEndReason::Abandon).bSuccess) { FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: abandon settlement.")); return; }
			ScheduleFinalVisible(0.35f); return;
		}
	}
	if (GV3FinalVisiblePhase == 4)
	{
		if (VisibleStep == 0) { CaptureVisible(TEXT("V3_FINAL_ABANDON_SETTLEMENT.png")); VisibleStep = 1; ScheduleFinalVisible(0.85f); return; }
		GV3FinalVisiblePhase = 0; VisibleStep = 0; PassAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: PASS.")); return;
	}
	FailAutomation(TEXT("V3_FINAL_VISIBLE_ACCEPTANCE: FAIL: invalid reload phase."));
#endif
}

void Ademo_mapV3ProgressionManager::ScheduleVisibleStep(float Delay) { GetWorldTimerManager().SetTimer(AutomationTimer, this, &Ademo_mapV3ProgressionManager::RunVisibleAcceptanceStep, Delay, false); }
void Ademo_mapV3ProgressionManager::CaptureVisible(const FString& Filename) const { if (!VisibleOutputDirectory.IsEmpty()) FScreenshotRequest::RequestScreenshot(FPaths::Combine(VisibleOutputDirectory, Filename), true, false); }
void Ademo_mapV3ProgressionManager::FailAutomation(const FString& Message) const { UE_LOG(Logdemo_map, Error, TEXT("%s"), *Message); FPlatformMisc::RequestExitWithStatus(false, 1); }
void Ademo_mapV3ProgressionManager::PassAutomation(const TCHAR* Marker) const { UE_LOG(Logdemo_map, Log, TEXT("%s"), Marker); FPlatformMisc::RequestExitWithStatus(false, 0); }

bool Ademo_mapV3ProgressionManager::MovePawnNear(AActor* Target, float Distance)
{
	if (!Target || !PlayerPawn.IsValid()) return false;
	FVector Direction = FVector(-1, 0, 0);
	const FVector Location = Target->GetActorLocation() + Direction * Distance;
	PlayerPawn->SetActorLocation(FVector(Location.X, Location.Y, PlayerPawn->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->SetActorRotation((Target->GetActorLocation() - PlayerPawn->GetActorLocation()).Rotation());
	RefreshFocusNow();
	return !Target->GetClass()->ImplementsInterface(Udemo_mapInteractable::StaticClass()) || FocusedActor.Get() == Target;
}

bool Ademo_mapV3ProgressionManager::PressBoundKey(const FKey& Key)
{
#if !UE_BUILD_SHIPPING
	Ademo_mapPlayerController* Controller = GetDemoController();
	if (!Controller) return false;
	return Controller->DispatchAutomationKey(Key);
#else
	return false;
#endif
}
