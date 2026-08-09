#include "demo_mapProfileSessionCoordinator.h"

#include "demo_mapTownUpgradeTransaction.h"

#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "demo_mapProfileShopStockTransaction.h"
#include "demo_mapProfileTradeTransaction.h"
#include "demo_mapPersistentPreparationTransaction.h"
#include "demo_mapSpiritStoneTransaction.h"

namespace
{
	bool IsPreparedRun(const Fdemo_mapPersistentProfile& Profile, const FGuid& ExpectedRunId)
	{
		return Profile.ActiveRun.bHasActiveRun
			&& Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::Prepared
			&& Profile.ActiveRun.ActiveRunId == ExpectedRunId;
	}
}

Fdemo_mapProfileSessionInitializeResult Fdemo_mapProfileSessionCoordinator::InitializeSession(const Fdemo_mapProfileStorageContext& Storage)
{
	Fdemo_mapProfileSessionInitializeResult Result;
	if (bOperationInProgress)
	{
		Result.Diagnostic = TEXT("A session operation is already in progress.");
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	CurrentProfile.Reset();
	PendingSettlementSnapshot.Reset();
	StorageContext = Storage;
	SetState(Edemo_mapProfileSessionState::Uninitialized, Edemo_mapProfileSessionErrorClass::None, FString());

	const Fdemo_mapProfileLoadResult Load = Repository.LoadOrCreateDefaultProfile(OperationStorage());
	if (!Load.IsSuccess())
	{
		SetState(Edemo_mapProfileSessionState::FatalProfileError, Edemo_mapProfileSessionErrorClass::LoadFailure, Load.Diagnostic);
		Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
		Result.Diagnostic = Load.Diagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	CurrentProfile = Load.Profile;
	if (CurrentProfile->ActiveRun.bHasActiveRun)
	{
		if (CurrentProfile->ActiveRun.ActiveRunState != Edemo_mapPersistentActiveRunState::Prepared)
		{
			SetState(Edemo_mapProfileSessionState::FatalProfileError, Edemo_mapProfileSessionErrorClass::LoadFailure, TEXT("Loaded Profile contains an unsupported non-Prepared active run state."));
			Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
			Result.Diagnostic = VisibleDiagnostic;
			Result.Snapshot = GetSnapshot();
			return Result;
		}
		return ResolvePreparedRunOnInitialization();
	}

	FString ShopDiagnostic;
	if (!EnsureShopStockForPreparation(ShopDiagnostic))
	{
		SetState(
			Edemo_mapProfileSessionState::RecoveryRequired,
			Edemo_mapProfileSessionErrorClass::PersistenceFailure,
			ShopDiagnostic);
		Result.Status =
			Edemo_mapProfileSessionInitializeStatus::RecoveryRequired;
		Result.Diagnostic = VisibleDiagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}
	SetState(
		Edemo_mapProfileSessionState::ReadyForPreparation,
		Edemo_mapProfileSessionErrorClass::None,
		ShopDiagnostic.IsEmpty() ? Load.Diagnostic : ShopDiagnostic);
	Result.Status = Edemo_mapProfileSessionInitializeStatus::Ready;
	Result.Diagnostic = Load.Diagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionInitializeResult Fdemo_mapProfileSessionCoordinator::ResolvePreparedRunOnInitialization()
{
	Fdemo_mapProfileSessionInitializeResult Result;
	const FGuid RunId = CurrentProfile->ActiveRun.ActiveRunId;
	Fdemo_mapProfileSettlementRequest Request;
	Request.ExpectedProfileId = CurrentProfile->ProfileId;
	Request.ExpectedSaveGeneration = CurrentProfile->SaveGeneration;
	Request.ExpectedActiveRunId = RunId;
	Request.RequestedEndReason = Edemo_mapRunEndReason::RecoveredAbandon;

	const Fdemo_mapProfileSettlementResult Settlement = Fdemo_mapProfileSettlementTransaction().Execute(
		CurrentProfile.GetValue(), Request, Repository, OperationStorage());
	if (Settlement.Status == Edemo_mapProfileSettlementStatus::Committed)
	{
		FString ShopDiagnostic;
		EnsureShopStockForPreparation(ShopDiagnostic);
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None,
			ShopDiagnostic.IsEmpty() ? TEXT("Prepared ActiveRun was durably resolved as RecoveredAbandon.") : ShopDiagnostic);
		Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted;
	}
	else if (Settlement.Status == Edemo_mapProfileSettlementStatus::AlreadyCommitted)
	{
		FString ShopDiagnostic;
		EnsureShopStockForPreparation(ShopDiagnostic);
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None,
			ShopDiagnostic.IsEmpty() ? TEXT("Prepared recovery was already durably committed.") : ShopDiagnostic);
		Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted;
	}
	else if (Settlement.Status == Edemo_mapProfileSettlementStatus::CommitOutcomeRequiresReload)
	{
		const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
		if (Reload.IsSuccess() && IsMatchingTerminalReload(Reload.Profile, RunId, Edemo_mapRunEndReason::RecoveredAbandon))
		{
			CurrentProfile = Reload.Profile;
			FString ShopDiagnostic;
			EnsureShopStockForPreparation(ShopDiagnostic);
			SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None,
				ShopDiagnostic.IsEmpty() ? TEXT("RecoveredAbandon commit was reconciled by one reload.") : ShopDiagnostic);
			Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted;
		}
		else if (Reload.IsSuccess() && IsPreparedRun(Reload.Profile, RunId))
		{
			CurrentProfile = Reload.Profile;
			SetState(Edemo_mapProfileSessionState::RecoveryRequired, Edemo_mapProfileSessionErrorClass::AmbiguousCommit, TEXT("Prepared recovery remains pending after one deterministic reload."));
			Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveryRequired;
		}
		else
		{
			SetState(Edemo_mapProfileSessionState::FatalProfileError, Edemo_mapProfileSessionErrorClass::AmbiguousCommit, TEXT("Prepared recovery reload did not yield the original Prepared run or its RecoveredAbandon tombstone."));
			Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
		}
	}
	else
	{
		SetState(Edemo_mapProfileSessionState::RecoveryRequired, Edemo_mapProfileSessionErrorClass::PreparedRecoveryFailure, Settlement.Diagnostic);
		Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveryRequired;
	}
	Result.Diagnostic = VisibleDiagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionBeginResult Fdemo_mapProfileSessionCoordinator::BeginRun(const Fdemo_mapBeginRunRequest& Request, Udemo_mapItemSubsystem& Runtime)
{
	Fdemo_mapProfileSessionBeginResult Result;
	if (bOperationInProgress || State != Edemo_mapProfileSessionState::ReadyForPreparation || !CurrentProfile.IsSet() || !StorageContext.IsSet())
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
		Result.Diagnostic = TEXT("BeginRun requires an initialized ReadyForPreparation session.");
		Result.Snapshot = GetSnapshot();
		return Result;
	}
	if (Request.ExpectedProfileId != CurrentProfile->ProfileId || Request.ExpectedSaveGeneration != CurrentProfile->SaveGeneration)
	{
		SetState(State, Edemo_mapProfileSessionErrorClass::StaleIntent, TEXT("BeginRun intent has a stale ProfileId or SaveGeneration."));
		Result.Status = Edemo_mapProfileSessionBeginStatus::StaleIntent;
		Result.Diagnostic = VisibleDiagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	const Fdemo_mapItemOperationResult RuntimePreparation =
		Runtime.PrepareForPersistentRun();
	if (!RuntimePreparation.bSuccess)
	{
		SetState(
			Edemo_mapProfileSessionState::ReadyForPreparation,
			Edemo_mapProfileSessionErrorClass::RuntimeMaterializationFailure,
			RuntimePreparation.Diagnostic);
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = RuntimePreparation.Diagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	Result.PersistentResult = Fdemo_mapProfileBeginRunTransaction().Execute(
		CurrentProfile.GetValue(), Request, Repository, OperationStorage());
	if (Result.PersistentResult.Status == Edemo_mapBeginRunStatus::CommitOutcomeRequiresReload)
	{
		const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
		if (Reload.IsSuccess())
		{
			CurrentProfile = Reload.Profile;
			SetState(CurrentProfile->ActiveRun.bHasActiveRun ? Edemo_mapProfileSessionState::RecoveryRequired : Edemo_mapProfileSessionState::ReadyForPreparation,
				Edemo_mapProfileSessionErrorClass::AmbiguousCommit, TEXT("BeginRun commit outcome required one reload; Runtime was not materialized."));
		}
		else
		{
			SetState(Edemo_mapProfileSessionState::FatalProfileError, Edemo_mapProfileSessionErrorClass::AmbiguousCommit, Reload.Diagnostic);
		}
		Result.Status = Edemo_mapProfileSessionBeginStatus::CommitOutcomeRequiresReload;
		Result.Diagnostic = VisibleDiagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}
	if (!Result.PersistentResult.IsCommitted() || !Result.PersistentResult.CommittedLoadoutPlan.IsSet())
	{
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::PersistenceFailure, Result.PersistentResult.Diagnostic);
		Result.Status = Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected;
		Result.Diagnostic = Result.PersistentResult.Diagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	Fdemo_mapPreparedRunRuntimeRequest RuntimeRequest;
	RuntimeRequest.CommittedPlan = Result.PersistentResult.CommittedLoadoutPlan.GetValue();
	Result.RuntimeResult = Runtime.MaterializePreparedRun(RuntimeRequest);
	if (!Result.RuntimeResult.IsMaterialized())
	{
		SetState(Edemo_mapProfileSessionState::RecoveryRequired, Edemo_mapProfileSessionErrorClass::RuntimeMaterializationFailure, Result.RuntimeResult.Diagnostic);
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = Result.RuntimeResult.Diagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	SetState(Edemo_mapProfileSessionState::RunActive, Edemo_mapProfileSessionErrorClass::None, TEXT("Persistent BeginRun and Runtime materialization committed the same ActiveRun."));
	Result.Status = Edemo_mapProfileSessionBeginStatus::CommittedAndMaterialized;
	Result.Diagnostic = VisibleDiagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfileSessionCoordinator::CommitRuntimeSettlement(const Fdemo_mapSettlementSummary& Summary)
{
	Fdemo_mapProfileSessionSettlementResult Result;
	if (bOperationInProgress || State != Edemo_mapProfileSessionState::RunActive || !CurrentProfile.IsSet() || !StorageContext.IsSet())
	{
		Result.Status = Edemo_mapProfileSessionSettlementStatus::SessionStateRejected;
		Result.Diagnostic = TEXT("Runtime Settlement commit requires one active session and no pending retry.");
		Result.Snapshot = GetSnapshot();
		return Result;
	}
	const bool bReasonAllowed = Summary.Reason == Edemo_mapRunEndReason::Extraction
		|| Summary.Reason == Edemo_mapRunEndReason::Death
		|| Summary.Reason == Edemo_mapRunEndReason::Abandon
		|| Summary.Reason == Edemo_mapRunEndReason::ActivationFailure;
	if (!Summary.bValid || !Summary.RuntimeSnapshot.bValid || !bReasonAllowed
		|| Summary.RunId != CurrentProfile->ActiveRun.ActiveRunId
		|| Summary.RuntimeSnapshot.ActiveRunId != Summary.RunId
		|| Summary.RuntimeSnapshot.CommittedEndReason != Summary.Reason)
	{
		SetState(State, Edemo_mapProfileSessionErrorClass::SettlementEvidenceInvalid, TEXT("Runtime first-event Settlement Summary/Snapshot does not match the active persistent run."));
		Result.Status = Edemo_mapProfileSessionSettlementStatus::EvidenceRejected;
		Result.Diagnostic = VisibleDiagnostic;
		Result.Snapshot = GetSnapshot();
		return Result;
	}

	PendingSettlementSnapshot = Summary.RuntimeSnapshot;
	return ExecutePendingSettlement();
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfileSessionCoordinator::RetryPendingSettlement()
{
	Fdemo_mapProfileSessionSettlementResult Result;
	if (bOperationInProgress || State != Edemo_mapProfileSessionState::PendingSettlementRetry || !PendingSettlementSnapshot.IsSet())
	{
		Result.Status = Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement;
		Result.Diagnostic = TEXT("No immutable pending Settlement Snapshot is available for retry.");
		Result.Snapshot = GetSnapshot();
		return Result;
	}
	return ExecutePendingSettlement();
}

Fdemo_mapPersistentPreparationCommitResult Fdemo_mapProfileSessionCoordinator::CommitPreparationLayout(
	const Fdemo_mapPersistentPreparationCommitIntent& Intent)
{
	if (bOperationInProgress || State != Edemo_mapProfileSessionState::ReadyForPreparation
		|| !CurrentProfile.IsSet() || !StorageContext.IsSet())
	{
		Fdemo_mapPersistentPreparationCommitResult Result;
		Result.Status = Edemo_mapPersistentPreparationCommitStatus::SessionStateRejected;
		Result.Diagnostic = TEXT("Persistent layout commit requires ReadyForPreparation.");
		return Result;
	}
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	Fdemo_mapPersistentPreparationCommitResult Result = Fdemo_mapPersistentPreparationTransaction().Execute(
		CurrentProfile.GetValue(), Intent, Repository, OperationStorage());
	if (Result.Status != Edemo_mapPersistentPreparationCommitStatus::CommitOutcomeRequiresReload)
	{
		SetState(
			Edemo_mapProfileSessionState::ReadyForPreparation,
			Result.IsSuccess() ? Edemo_mapProfileSessionErrorClass::None : Edemo_mapProfileSessionErrorClass::PersistenceFailure,
			Result.Diagnostic);
		return Result;
	}

	const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
	if (Reload.IsSuccess()
		&& Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.SaveGeneration == Intent.ExpectedSaveGeneration + 1
		&& Reload.Profile.PreparationLayout == Intent.Layout)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapPersistentPreparationCommitStatus::Committed;
		Result.CommittedGeneration = Reload.Profile.SaveGeneration;
		Result.CommittedProfile = Reload.Profile;
		Result.Diagnostic = TEXT("Persistent layout commit reconciled after exactly one reload; no replay occurred.");
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None, Result.Diagnostic);
		return Result;
	}
	if (Reload.IsSuccess() && Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.SaveGeneration == Intent.ExpectedSaveGeneration)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapPersistentPreparationCommitStatus::RepositorySaveRejected;
		Result.Diagnostic = TEXT("Persistent layout ambiguity reconciled as unchanged Before state.");
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::PersistenceFailure, Result.Diagnostic);
		return Result;
	}
	SetState(Edemo_mapProfileSessionState::RecoveryRequired, Edemo_mapProfileSessionErrorClass::AmbiguousCommit,
		TEXT("Persistent layout reload matched neither Before nor IntendedAfter."));
	Result.Diagnostic = VisibleDiagnostic;
	return Result;
}

Fdemo_mapWarehouseMoveResult
Fdemo_mapProfileSessionCoordinator::MoveWarehouseItem(
	const Fdemo_mapWarehouseMoveIntent& Intent)
{
	if (bOperationInProgress
		|| State != Edemo_mapProfileSessionState::ReadyForPreparation
		|| !CurrentProfile.IsSet()
		|| !StorageContext.IsSet())
	{
		Fdemo_mapWarehouseMoveResult Result;
		Result.Status =
			Edemo_mapWarehouseMoveStatus::SessionNotReady;
		Result.Diagnostic =
			TEXT("Warehouse move requires ReadyForPreparation.");
		return Result;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	Fdemo_mapWarehouseMoveResult Result =
		Fdemo_mapPersistentWarehouseTransaction().Execute(
			CurrentProfile.GetValue(),
			Intent,
			Repository,
			OperationStorage());
	if (Result.Status
		!= Edemo_mapWarehouseMoveStatus::CommitOutcomeRequiresReload)
	{
		SetState(
			Edemo_mapProfileSessionState::ReadyForPreparation,
			Result.IsSuccess()
				? Edemo_mapProfileSessionErrorClass::None
				: Edemo_mapProfileSessionErrorClass::PersistenceFailure,
			Result.Diagnostic);
		return Result;
	}

	const Fdemo_mapProfileLoadResult Reload =
		Repository.LoadExistingProfile(StorageContext.GetValue());
	if (Reload.IsSuccess()
		&& Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.SaveGeneration
			== Intent.ExpectedSaveGeneration + 1
		&& Reload.Profile.WarehouseLayout.bInitialized)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapWarehouseMoveStatus::Committed;
		Result.CommittedGeneration = Reload.Profile.SaveGeneration;
		Result.CommittedProfile = Reload.Profile;
		Result.Diagnostic =
			TEXT("Warehouse move reconciled after one reload.");
		SetState(
			Edemo_mapProfileSessionState::ReadyForPreparation,
			Edemo_mapProfileSessionErrorClass::None,
			Result.Diagnostic);
		return Result;
	}
	if (Reload.IsSuccess()
		&& Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.SaveGeneration
			== Intent.ExpectedSaveGeneration)
	{
		CurrentProfile = Reload.Profile;
		Result.Status =
			Edemo_mapWarehouseMoveStatus::RepositorySaveRejected;
		Result.Diagnostic =
			TEXT("Warehouse move ambiguity reconciled as unchanged Before state.");
		SetState(
			Edemo_mapProfileSessionState::ReadyForPreparation,
			Edemo_mapProfileSessionErrorClass::PersistenceFailure,
			Result.Diagnostic);
		return Result;
	}
	SetState(
		Edemo_mapProfileSessionState::RecoveryRequired,
		Edemo_mapProfileSessionErrorClass::AmbiguousCommit,
		TEXT("Warehouse move reload matched neither Before nor IntendedAfter."));
	Result.Diagnostic = VisibleDiagnostic;
	return Result;
}

Fdemo_mapSpiritStonePickupResult Fdemo_mapProfileSessionCoordinator::CollectSpiritStone(
	const Fdemo_mapSpiritStonePickupIntent& Intent)
{
	if (bOperationInProgress || State != Edemo_mapProfileSessionState::RunActive
		|| !CurrentProfile.IsSet() || !StorageContext.IsSet())
	{
		Fdemo_mapSpiritStonePickupResult Result;
		Result.Status = Edemo_mapSpiritStonePickupStatus::RunNotActive;
		Result.Diagnostic = TEXT("Spirit Stone pickup requires RunActive.");
		return Result;
	}
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	Fdemo_mapSpiritStonePickupResult Result = Fdemo_mapSpiritStoneTransaction().Execute(
		CurrentProfile.GetValue(), Intent, Repository, OperationStorage());
	if (Result.Status != Edemo_mapSpiritStonePickupStatus::CommitOutcomeRequiresReload)
	{
		SetState(State,
			Result.IsCommitted() ? Edemo_mapProfileSessionErrorClass::None : Edemo_mapProfileSessionErrorClass::PersistenceFailure,
			Result.Diagnostic);
		return Result;
	}

	const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
	if (Reload.IsSuccess()
		&& Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.ActiveRun.ActiveRunId == Intent.ExpectedActiveRunId
		&& Reload.Profile.SaveGeneration == Intent.ExpectedSaveGeneration + 1
		&& Reload.Profile.ActiveRun.ConsumedSpiritStoneSourceIds.Contains(Intent.SourceId))
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapSpiritStonePickupStatus::Committed;
		Result.CommittedGeneration = Reload.Profile.SaveGeneration;
		Result.RiskAfter = Reload.Profile.ActiveRun.RiskSpiritStones;
		Result.CommittedProfile = Reload.Profile;
		Result.Diagnostic = TEXT("Spirit Stone commit reconciled after exactly one reload; source was not replayed.");
		SetState(Edemo_mapProfileSessionState::RunActive, Edemo_mapProfileSessionErrorClass::None, Result.Diagnostic);
		return Result;
	}
	if (Reload.IsSuccess() && Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.ActiveRun.ActiveRunId == Intent.ExpectedActiveRunId
		&& Reload.Profile.SaveGeneration == Intent.ExpectedSaveGeneration)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapSpiritStonePickupStatus::RepositorySaveRejected;
		Result.Diagnostic = TEXT("Spirit Stone ambiguity reconciled as unchanged Before state.");
		SetState(Edemo_mapProfileSessionState::RunActive, Edemo_mapProfileSessionErrorClass::PersistenceFailure, Result.Diagnostic);
		return Result;
	}
	SetState(Edemo_mapProfileSessionState::RecoveryRequired, Edemo_mapProfileSessionErrorClass::AmbiguousCommit,
		TEXT("Spirit Stone reload matched neither Before nor IntendedAfter."));
	Result.Diagnostic = VisibleDiagnostic;
	return Result;
}

Fdemo_mapTownUpgradeResult Fdemo_mapProfileSessionCoordinator::SubmitTownUpgrade(
	const Fdemo_mapTownUpgradeIntent& Intent)
{
	if (bOperationInProgress || State != Edemo_mapProfileSessionState::ReadyForPreparation
		|| !CurrentProfile.IsSet() || !StorageContext.IsSet())
	{
		Fdemo_mapTownUpgradeResult Result;
		Result.Status = Edemo_mapTownUpgradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Town upgrade requires an initialized ReadyForPreparation session.");
		return Result;
	}
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	Fdemo_mapTownUpgradeResult Result = Fdemo_mapTownUpgradeTransaction().Execute(
		CurrentProfile.GetValue(), Intent, Repository, OperationStorage());
	if (Result.Status != Edemo_mapTownUpgradeStatus::CommitOutcomeRequiresReload)
	{
		SetState(
			State,
			Result.IsCommitted() ? Edemo_mapProfileSessionErrorClass::None
				: Edemo_mapProfileSessionErrorClass::PersistenceFailure,
			Result.Diagnostic);
		return Result;
	}

	const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
	if (Reload.IsSuccess()
		&& Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.SaveGeneration == Intent.ExpectedSaveGeneration + 1
		&& Reload.Profile.TownLevel == Intent.RequestedNextLevel)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapTownUpgradeStatus::Committed;
		Result.CommittedGeneration = Reload.Profile.SaveGeneration;
		Result.TownLevelAfter = Reload.Profile.TownLevel;
		Result.PersistentSpiritStonesAfter = Reload.Profile.PersistentSpiritStones;
		Result.CommittedProfile = Reload.Profile;
		Result.Diagnostic = TEXT("Town upgrade reconciled after one reload; construction was not replayed.");
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None, Result.Diagnostic);
		return Result;
	}
	if (Reload.IsSuccess() && Reload.Profile.ProfileId == Intent.ExpectedProfileId
		&& Reload.Profile.SaveGeneration == Intent.ExpectedSaveGeneration)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapTownUpgradeStatus::RepositorySaveRejected;
		Result.Diagnostic = TEXT("Town upgrade ambiguity reconciled as unchanged Before state.");
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::PersistenceFailure, Result.Diagnostic);
		return Result;
	}
	SetState(Edemo_mapProfileSessionState::RecoveryRequired, Edemo_mapProfileSessionErrorClass::AmbiguousCommit,
		TEXT("Town upgrade reload matched neither Before nor IntendedAfter."));
	Result.Diagnostic = VisibleDiagnostic;
	return Result;
}

Fdemo_mapProfileTradeResult Fdemo_mapProfileSessionCoordinator::SubmitTrade(
	const Fdemo_mapProfileTradeIntent& Intent,
	const TSet<FGuid>& SelectedForDeployment)
{
	Fdemo_mapProfileTradeResult Result;
	Result.Kind = Intent.Kind;
	if (bOperationInProgress)
	{
		Result.Status = Edemo_mapProfileTradeStatus::OperationInProgress;
		Result.Diagnostic = TEXT("A Profile session operation is already in progress.");
		return Result;
	}
	if (!Fdemo_mapProfileTradeTransaction::IsSessionStateAllowed(static_cast<uint8>(State))
		|| !CurrentProfile.IsSet()
		|| !StorageContext.IsSet())
	{
		Result.Status = Edemo_mapProfileTradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Trade requires an initialized ReadyForPreparation session.");
		return Result;
	}
	if (Intent.ExpectedProfileId != CurrentProfile->ProfileId
		|| Intent.ExpectedSaveGeneration != CurrentProfile->SaveGeneration)
	{
		Result.Status = Edemo_mapProfileTradeStatus::StaleIntent;
		Result.ProfileId = CurrentProfile->ProfileId;
		Result.SaveGenerationBefore = CurrentProfile->SaveGeneration;
		Result.SaveGenerationAfter = CurrentProfile->SaveGeneration;
		Result.BalanceBefore = CurrentProfile->PersistentSpiritStones;
		Result.BalanceAfter = CurrentProfile->PersistentSpiritStones;
		Result.Diagnostic = TEXT("Trade intent has a stale ProfileId or SaveGeneration.");
		SetState(State, Edemo_mapProfileSessionErrorClass::StaleIntent, Result.Diagnostic);
		return Result;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	TSet<FGuid> ProtectedLayoutIds = SelectedForDeployment;
	auto Protect = [&ProtectedLayoutIds](const FGuid& Id) { if (Id.IsValid()) ProtectedLayoutIds.Add(Id); };
	Protect(CurrentProfile->PreparationLayout.WeaponItemInstanceId);
	Protect(CurrentProfile->PreparationLayout.ArmorItemInstanceId);
	Protect(CurrentProfile->PreparationLayout.AccessoryItemInstanceId);
	Protect(CurrentProfile->PreparationLayout.SpatialRingItemInstanceId);
	Protect(CurrentProfile->PreparationLayout.BackpackItemInstanceId);
	for (const FGuid& Id : CurrentProfile->PreparationLayout.OrderedRunInventoryItemInstanceIds) Protect(Id);
	for (const FGuid& Id : CurrentProfile->PreparationLayout.HotbarItemInstanceIds) Protect(Id);
	Fdemo_mapProfileTradeTransactionResult Transaction = Fdemo_mapProfileTradeTransaction().Execute(
		CurrentProfile.GetValue(), Intent, ProtectedLayoutIds, Repository, OperationStorage());
	Result = Transaction.Result;
	if (Result.Status == Edemo_mapProfileTradeStatus::Committed)
	{
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation,
			Edemo_mapProfileSessionErrorClass::None, Result.Diagnostic);
		return Result;
	}
	if (Result.Status != Edemo_mapProfileTradeStatus::CommitOutcomeRequiresReload)
	{
		const Edemo_mapProfileSessionErrorClass ErrorClass =
			Result.Status == Edemo_mapProfileTradeStatus::StaleIntent
				? Edemo_mapProfileSessionErrorClass::StaleIntent
				: (Result.Status == Edemo_mapProfileTradeStatus::PersistentCommitRejected
					? Edemo_mapProfileSessionErrorClass::PersistenceFailure
					: Edemo_mapProfileSessionErrorClass::None);
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, ErrorClass, Result.Diagnostic);
		return Result;
	}

	const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
	if (Reload.IsSuccess()
		&& Transaction.bHasIntendedAfter
		&& Fdemo_mapProfileTradeTransaction::MatchesIntendedCommit(Reload.Profile, Transaction.IntendedAfter))
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapProfileTradeStatus::ReconciledAfterReload;
		Result.SaveGenerationAfter = Reload.Profile.SaveGeneration;
		Result.BalanceAfter = Reload.Profile.PersistentSpiritStones;
		Result.Diagnostic = TEXT("Trade commit was reconciled as IntendedAfter by exactly one reload; no replay occurred.");
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation,
			Edemo_mapProfileSessionErrorClass::None, Result.Diagnostic);
		return Result;
	}
	if (Reload.IsSuccess() && Reload.Profile == Transaction.Before)
	{
		CurrentProfile = Reload.Profile;
		Result.Status = Edemo_mapProfileTradeStatus::PersistentCommitRejected;
		Result.SaveGenerationAfter = Reload.Profile.SaveGeneration;
		Result.BalanceAfter = Reload.Profile.PersistentSpiritStones;
		Result.Diagnostic = TEXT("Trade ambiguity reconciled as unchanged Before state; caller may submit a new intent.");
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation,
			Edemo_mapProfileSessionErrorClass::PersistenceFailure, Result.Diagnostic);
		return Result;
	}

	Result.Status = Edemo_mapProfileTradeStatus::FatalRecovery;
	Result.Diagnostic = Reload.IsSuccess()
		? TEXT("One trade reload matched neither Before nor IntendedAfter; session entered RecoveryRequired.")
		: Reload.Diagnostic;
	SetState(Edemo_mapProfileSessionState::RecoveryRequired,
		Edemo_mapProfileSessionErrorClass::AmbiguousCommit, Result.Diagnostic);
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfileSessionCoordinator::ExecutePendingSettlement()
{
	Fdemo_mapProfileSessionSettlementResult Result;
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const Fdemo_mapRuntimeSettlementSnapshot Evidence = PendingSettlementSnapshot.GetValue();
	Fdemo_mapProfileSettlementRequest Request;
	Request.ExpectedProfileId = CurrentProfile->ProfileId;
	Request.ExpectedSaveGeneration = CurrentProfile->SaveGeneration;
	Request.ExpectedActiveRunId = Evidence.ActiveRunId;
	Request.RequestedEndReason = Evidence.CommittedEndReason;
	Request.RuntimeSnapshot = Evidence;
	Result.PersistentResult = Fdemo_mapProfileSettlementTransaction().Execute(
		CurrentProfile.GetValue(), Request, Repository, OperationStorage());

	if (Result.PersistentResult.Status == Edemo_mapProfileSettlementStatus::Committed)
	{
		PendingSettlementSnapshot.Reset();
		FString ShopDiagnostic;
		EnsureShopStockForPreparation(ShopDiagnostic);
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None,
			ShopDiagnostic.IsEmpty() ? TEXT("Runtime first-event Snapshot was durably committed.") : ShopDiagnostic);
		Result.Status = Edemo_mapProfileSessionSettlementStatus::Committed;
	}
	else if (Result.PersistentResult.Status == Edemo_mapProfileSettlementStatus::AlreadyCommitted)
	{
		PendingSettlementSnapshot.Reset();
		FString ShopDiagnostic;
		EnsureShopStockForPreparation(ShopDiagnostic);
		SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None,
			ShopDiagnostic.IsEmpty() ? TEXT("The same Runtime Settlement was already durably committed.") : ShopDiagnostic);
		Result.Status = Edemo_mapProfileSessionSettlementStatus::AlreadyCommitted;
	}
	else if (Result.PersistentResult.Status == Edemo_mapProfileSettlementStatus::CommitOutcomeRequiresReload)
	{
		const Fdemo_mapProfileLoadResult Reload = Repository.LoadExistingProfile(StorageContext.GetValue());
		if (Reload.IsSuccess() && IsMatchingTerminalReload(Reload.Profile, Evidence.ActiveRunId, Evidence.CommittedEndReason))
		{
			CurrentProfile = Reload.Profile;
			PendingSettlementSnapshot.Reset();
			FString ShopDiagnostic;
			EnsureShopStockForPreparation(ShopDiagnostic);
			SetState(Edemo_mapProfileSessionState::ReadyForPreparation, Edemo_mapProfileSessionErrorClass::None,
				ShopDiagnostic.IsEmpty() ? TEXT("Settlement commit was reconciled by one reload.") : ShopDiagnostic);
			Result.Status = Edemo_mapProfileSessionSettlementStatus::ReconciledAfterReload;
		}
		else if (Reload.IsSuccess() && IsPreparedRun(Reload.Profile, Evidence.ActiveRunId))
		{
			CurrentProfile = Reload.Profile;
			SetState(Edemo_mapProfileSessionState::PendingSettlementRetry, Edemo_mapProfileSessionErrorClass::AmbiguousCommit, TEXT("Settlement remains Prepared after one reload; the immutable first-event Snapshot is retained."));
			Result.Status = Edemo_mapProfileSessionSettlementStatus::PendingRetry;
		}
		else
		{
			SetState(Edemo_mapProfileSessionState::FatalProfileError, Edemo_mapProfileSessionErrorClass::AmbiguousCommit, TEXT("Settlement reload did not yield the original Prepared run or matching tombstone."));
			Result.Status = Edemo_mapProfileSessionSettlementStatus::FatalProfileError;
		}
	}
	else
	{
		SetState(Edemo_mapProfileSessionState::PendingSettlementRetry, Edemo_mapProfileSessionErrorClass::PersistenceFailure, Result.PersistentResult.Diagnostic);
		Result.Status = Edemo_mapProfileSessionSettlementStatus::PendingRetry;
	}
	Result.Diagnostic = VisibleDiagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionSnapshot Fdemo_mapProfileSessionCoordinator::GetSnapshot() const
{
	Fdemo_mapProfileSessionSnapshot Snapshot;
	Snapshot.SessionState = State;
	Snapshot.ErrorClassification = ErrorClassification;
	Snapshot.VisibleDiagnostic = VisibleDiagnostic;
	Snapshot.bCanBeginRun = State == Edemo_mapProfileSessionState::ReadyForPreparation;
	Snapshot.bCanRetrySettlement = State == Edemo_mapProfileSessionState::PendingSettlementRetry && PendingSettlementSnapshot.IsSet();
	if (CurrentProfile.IsSet())
	{
		Snapshot.ProfileId = CurrentProfile->ProfileId;
		Snapshot.SaveGeneration = CurrentProfile->SaveGeneration;
		Snapshot.PersistentSpiritStones = CurrentProfile->PersistentSpiritStones;
		Snapshot.TownLevel = CurrentProfile->TownLevel;
		Snapshot.RiskSpiritStones = CurrentProfile->ActiveRun.RiskSpiritStones;
	Snapshot.OrderedPermanentStash = CurrentProfile->PermanentStash;
	Snapshot.ShopStock = CurrentProfile->ShopStock;
	Snapshot.PreparationLayout = CurrentProfile->PreparationLayout;
	Snapshot.WarehouseLayout = CurrentProfile->WarehouseLayout;
		Snapshot.ConsumedSpiritStoneSourceIds = CurrentProfile->ActiveRun.ConsumedSpiritStoneSourceIds;
		Snapshot.ActiveRunId = CurrentProfile->ActiveRun.ActiveRunId;
		Snapshot.LastSettlementId = CurrentProfile->LastSettlementId;
		Snapshot.LastTerminalReason = TerminalReason(CurrentProfile.GetValue());
	}
	return Snapshot;
}

bool Fdemo_mapProfileSessionCoordinator::EnsureShopStockForPreparation(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!CurrentProfile.IsSet() || !StorageContext.IsSet())
	{
		OutDiagnostic =
			TEXT("ShopStock preparation requires a loaded Profile and storage.");
		return false;
	}
	const bool bHadUsableStock = CurrentProfile->ShopStock.bInitialized;
	Fdemo_mapProfileShopStockEnsureResult Ensure =
		Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
			CurrentProfile.GetValue(),
			Repository,
			OperationStorage());
	if (Ensure.IsSuccess())
	{
		OutDiagnostic = Ensure.Diagnostic;
		return true;
	}
	if (Ensure.Status
		== Edemo_mapProfileShopStockEnsureStatus::CommitOutcomeRequiresReload)
	{
		const Fdemo_mapProfileLoadResult Reload =
			Repository.LoadExistingProfile(StorageContext.GetValue());
		if (Reload.IsSuccess()
			&& Ensure.bHasIntendedAfter
			&& Reload.Profile.ProfileId == Ensure.Before.ProfileId
			&& Reload.Profile.SaveGeneration
				== Ensure.Before.SaveGeneration + 1
			&& Reload.Profile.ShopStock
				== Ensure.IntendedAfter.ShopStock)
		{
			CurrentProfile = Reload.Profile;
			OutDiagnostic =
				TEXT("ShopStock commit reconciled after exactly one reload; no restock replay occurred.");
			return true;
		}
	}
	OutDiagnostic = Ensure.Diagnostic.IsEmpty()
		? TEXT("ShopStock initialization or pending restock failed atomically.")
		: Ensure.Diagnostic;
	// A failed restock retains the complete previous generation for use and
	// is retried after the next process/session entry. Initial generation has
	// no safe product stock and therefore cannot enter Preparation.
	return bHadUsableStock;
}

Fdemo_mapProfileStorageContext Fdemo_mapProfileSessionCoordinator::OperationStorage()
{
	Fdemo_mapProfileStorageContext Result = StorageContext.GetValue();
#if WITH_DEV_AUTOMATION_TESTS
	StorageContext->InjectedFailure = Edemo_mapProfileFailureStage::None;
	Result.InjectedFailure = NextRepositoryFailure != Edemo_mapProfileFailureStage::None
		? NextRepositoryFailure
		: Result.InjectedFailure;
	NextRepositoryFailure = Edemo_mapProfileFailureStage::None;
#endif
	return Result;
}

void Fdemo_mapProfileSessionCoordinator::SetState(
	Edemo_mapProfileSessionState NewState,
	Edemo_mapProfileSessionErrorClass ErrorClass,
	const FString& Diagnostic)
{
	State = NewState;
	ErrorClassification = ErrorClass;
	VisibleDiagnostic = Diagnostic;
}

bool Fdemo_mapProfileSessionCoordinator::IsMatchingTerminalReload(
	const Fdemo_mapPersistentProfile& Reloaded,
	const FGuid& ExpectedRunId,
	Edemo_mapRunEndReason ExpectedReason)
{
	return !Reloaded.ActiveRun.bHasActiveRun
		&& Reloaded.ActiveRun.ActiveRunId == ExpectedRunId
		&& Reloaded.ActiveRun.CommittedSettlementId == Reloaded.LastSettlementId
		&& TerminalReason(Reloaded) == ExpectedReason;
}

Edemo_mapRunEndReason Fdemo_mapProfileSessionCoordinator::TerminalReason(const Fdemo_mapPersistentProfile& Profile)
{
	switch (Profile.ActiveRun.ActiveRunState)
	{
	case Edemo_mapPersistentActiveRunState::Extraction: return Edemo_mapRunEndReason::Extraction;
	case Edemo_mapPersistentActiveRunState::Death: return Edemo_mapRunEndReason::Death;
	case Edemo_mapPersistentActiveRunState::Abandon: return Edemo_mapRunEndReason::Abandon;
	case Edemo_mapPersistentActiveRunState::RecoveredAbandon: return Edemo_mapRunEndReason::RecoveredAbandon;
	case Edemo_mapPersistentActiveRunState::ActivationFailure: return Edemo_mapRunEndReason::ActivationFailure;
	default: return Edemo_mapRunEndReason::None;
	}
}
