#pragma once

#include "CoreMinimal.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionTypes.h"
#include "demo_mapProfileTradeTypes.h"
#include "demo_mapPersistentPreparationTransaction.h"
#include "demo_mapPersistentWarehouseTransaction.h"
#include "demo_mapSpiritStoneTypes.h"
#include "demo_mapTownUpgradeTypes.h"

class Udemo_mapItemSubsystem;

/**
 * Explicitly initialized, pure-value session coordinator. A later thin GameInstanceSubsystem may own
 * this object; construction itself performs no disk, Profile, Runtime, or Gameplay work.
 */
class Fdemo_mapProfileSessionCoordinator
{
public:
	Fdemo_mapProfileSessionInitializeResult InitializeSession(const Fdemo_mapProfileStorageContext& Storage);
	Fdemo_mapProfileSessionBeginResult BeginRun(const Fdemo_mapBeginRunRequest& Request, Udemo_mapItemSubsystem& Runtime);
	Fdemo_mapProfileSessionSettlementResult CommitRuntimeSettlement(const Fdemo_mapSettlementSummary& Summary);
	Fdemo_mapProfileGeneratedRewardSourceResult CommitGeneratedRewardSource(
		const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt);
	TArray<Fdemo_mapPersistentGeneratedRewardSource>
	GetActiveGeneratedRewardSources() const;
	Fdemo_mapProfileSessionSettlementResult RetryPendingSettlement();
	Fdemo_mapProfileTradeResult SubmitTrade(
		const Fdemo_mapProfileTradeIntent& Intent,
		const TSet<FGuid>& SelectedForDeployment);
	Fdemo_mapPersistentPreparationCommitResult CommitPreparationLayout(
		const Fdemo_mapPersistentPreparationCommitIntent& Intent);
	Fdemo_mapWarehouseMoveResult MoveWarehouseItem(
		const Fdemo_mapWarehouseMoveIntent& Intent);
	Fdemo_mapSpiritStonePickupResult CollectSpiritStone(
		const Fdemo_mapSpiritStonePickupIntent& Intent);
	Fdemo_mapTownUpgradeResult SubmitTownUpgrade(
		const Fdemo_mapTownUpgradeIntent& Intent);
	Fdemo_mapProfileSessionSnapshot GetSnapshot() const;
	/**
	 * Copies the complete validated Profile only at the stable out-of-raid
	 * migration boundary. The returned value carries no write capability.
	 */
	bool TryCaptureStableProfileForItemMigration(
		Fdemo_mapPersistentProfile& OutProfile,
		FString* OutDiagnostic = nullptr) const;

#if WITH_DEV_AUTOMATION_TESTS
	void SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage Stage) { NextRepositoryFailure = Stage; }
#endif

private:
	Fdemo_mapProfileSessionInitializeResult ResolvePreparedRunOnInitialization();
	Fdemo_mapProfileSessionSettlementResult ExecutePendingSettlement();
	bool EnsureShopStockForPreparation(FString& OutDiagnostic);
	Fdemo_mapProfileStorageContext OperationStorage();
	void SetState(Edemo_mapProfileSessionState NewState, Edemo_mapProfileSessionErrorClass ErrorClass, const FString& Diagnostic);
	static bool IsMatchingTerminalReload(const Fdemo_mapPersistentProfile& Reloaded, const FGuid& ExpectedRunId, Edemo_mapRunEndReason ExpectedReason);
	static Edemo_mapRunEndReason TerminalReason(const Fdemo_mapPersistentProfile& Profile);

	Fdemo_mapProfileRepository Repository;
	TOptional<Fdemo_mapPersistentProfile> CurrentProfile;
	TOptional<Fdemo_mapProfileStorageContext> StorageContext;
	TOptional<Fdemo_mapRuntimeSettlementSnapshot> PendingSettlementSnapshot;
	Edemo_mapProfileSessionState State = Edemo_mapProfileSessionState::Uninitialized;
	Edemo_mapProfileSessionErrorClass ErrorClassification = Edemo_mapProfileSessionErrorClass::None;
	FString VisibleDiagnostic;
	bool bOperationInProgress = false;
#if WITH_DEV_AUTOMATION_TESTS
	Edemo_mapProfileFailureStage NextRepositoryFailure = Edemo_mapProfileFailureStage::None;
#endif
};
