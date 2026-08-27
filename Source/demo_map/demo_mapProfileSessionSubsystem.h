#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapProfileSessionCoordinator.h"
#include "demo_mapTownUpgradeTypes.h"
#include "demo_mapProfileSessionSubsystem.generated.h"

/**
 * Thin, C++-only GameInstance owner for the persistent Profile session coordinator.
 * Engine construction and Initialize remain fully lazy: only InitializeSession may touch storage.
 */
UCLASS()
class Udemo_mapProfileSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	Fdemo_mapProfileSessionInitializeResult InitializeSession(const Fdemo_mapProfileStorageContext& Storage);
	Fdemo_mapProfileSessionBeginResult BeginRun(const Fdemo_mapBeginRunRequest& Request);
	Fdemo_mapProfileSessionSettlementResult CommitRuntimeSettlement(const Fdemo_mapSettlementSummary& Summary);
	Fdemo_mapProfileGeneratedRewardSourceResult CommitGeneratedRewardSource(
		const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt);
	TArray<Fdemo_mapPersistentGeneratedRewardSource>
	GetActiveGeneratedRewardSources() const;
	Fdemo_mapProfileSessionSettlementResult RetryPendingSettlement();
	Fdemo_mapProfileTradeResult SubmitTradeIntent(const Fdemo_mapProfileTradeIntent& Intent);
	Fdemo_mapProfileSessionSnapshot GetSnapshot() const;
	/** Read-only, stable legacy source capture for the explicit 0.0.10 cutover. */
	bool TryCaptureStableProfileForItemMigration(
		Fdemo_mapPersistentProfile& OutProfile,
		FString* OutDiagnostic = nullptr) const;
	bool AreLegacyItemWritesRetired(
		FString* OutDiagnostic = nullptr) const;
	Fdemo_mapProfilePreparationSnapshot GetPreparationSnapshot() const;
	Fdemo_mapProfilePreparationSelectionResult SetPreparationEquipment(FName SlotId, const FGuid& ItemInstanceId);
	Fdemo_mapProfilePreparationSelectionResult SetPreparationMaterial(const FGuid& ItemInstanceId, bool bSelected);
	Fdemo_mapProfilePreparationSelectionResult SetPreparationHotbarSlot(int32 ExternalSlotNumber, const FGuid& ItemInstanceId);
	Fdemo_mapProfilePreparationSelectionResult ClearPreparationSelection();
	Fdemo_mapWarehouseMoveResult MoveWarehouseItem(
		int32 SourceSlotIndex,
		int32 TargetSlotIndex);
	/** P2 persistent player-owned drag/drop; its revision token is SaveGeneration. */
	Fdemo_mapPlayerItemDropResult ExecutePreparationItemDrop(
		const Fdemo_mapPlayerItemDropIntent& Intent);
	/** Starts the existing Profile/Run transaction without consulting a legacy preparation layout. */
	Fdemo_mapProfileSessionBeginResult StartRunWithoutPreparation();
	/** Legacy preparation-widget route retained only for historical tools and tests. */
	Fdemo_mapProfileSessionBeginResult StartPreparedRun();
	Fdemo_mapSpiritStonePickupResult CollectFixedSpiritStone();
	Fdemo_mapSpiritStonePickupResult CollectSpiritStone(
		FName PickupId,
		FName SourceId,
		int64 Value);
	/** A short HUD-only acknowledgement of the most recently committed pickup. */
	int64 GetRecentSpiritStoneGain() const;
	Fdemo_mapTownUpgradeResult RequestTownUpgrade();
	bool IsExplicitlyInitialized() const { return bExplicitlyInitialized; }

#if WITH_DEV_AUTOMATION_TESTS
	void SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage Stage);
#endif

private:
	Fdemo_mapProfileSessionInitializeResult RejectInitialize(const FString& Diagnostic) const;
	Fdemo_mapProfileSessionBeginResult RejectBegin(const FString& Diagnostic) const;
	Fdemo_mapProfileSessionSettlementResult RejectSettlement(
		Edemo_mapProfileSessionSettlementStatus Status,
		const FString& Diagnostic) const;
	Fdemo_mapProfileSessionInitializeResult CurrentInitializeResult(const FString& Diagnostic) const;
	Fdemo_mapProfilePreparationSelectionResult AcceptPreparation(const FString& Diagnostic) const;
	Fdemo_mapProfilePreparationSelectionResult RejectPreparation(
		Edemo_mapProfilePreparationSelectionStatus Status,
		const FString& Diagnostic) const;
	Fdemo_mapProfilePreparationSelectionResult CommitPreparationLayout(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		const Fdemo_mapPersistentPreparationLayout& Layout,
		const FString& SuccessDiagnostic);
	bool EnsurePreparationContext(const Fdemo_mapProfileSessionSnapshot& SessionSnapshot, FString& OutDiagnostic);
	bool IsPreparationIdentityCurrent(const Fdemo_mapProfileSessionSnapshot& SessionSnapshot) const;
	bool IsPreparationItemSelected(const FGuid& ItemInstanceId) const;
	void BindPreparationIdentity(const Fdemo_mapProfileSessionSnapshot& SessionSnapshot);
	void ClearPreparationSelectionInternal(const FString& Diagnostic = FString());
	void ApplyBeginResultToPreparation(const Fdemo_mapProfileSessionBeginResult& Result);
	void SynchronizePreparationWithSession(const Fdemo_mapProfileSessionSnapshot& SessionSnapshot, const FString& Diagnostic);

	TUniquePtr<Fdemo_mapProfileSessionCoordinator> Coordinator;
	FString InjectedStorageRoot;
	FGuid PreparationProfileId;
	int32 PreparationSaveGeneration = 0;
	FGuid SelectedWeaponId;
	FGuid SelectedArmorId;
	FGuid SelectedAccessoryId;
	TArray<FGuid> SelectedMaterialIds;
	TOptional<Fdemo_mapProfileTradeResult> LastTradeResult;
	FString PreparationDiagnostic;
	int64 RecentSpiritStoneGain = 0;
	double RecentSpiritStoneGainExpirySeconds = 0.0;
	bool bPreparationIdentityBound = false;
	bool bExplicitlyInitialized = false;
};
