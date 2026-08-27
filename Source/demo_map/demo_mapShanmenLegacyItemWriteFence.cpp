#include "demo_mapShanmenLegacyItemWriteFence.h"

#include "ShanmenItemPersistence.h"
#include "demo_mapPersistentProfileTypes.h"

#include "HAL/FileManager.h"

namespace
{
	bool SameItemBearingRun(
		const Fdemo_mapPersistentActiveRunRecord& Before,
		const Fdemo_mapPersistentActiveRunRecord& Candidate)
	{
		return Before.bHasActiveRun == Candidate.bHasActiveRun
			&& Before.ActiveRunId == Candidate.ActiveRunId
			&& Before.ActiveRunState == Candidate.ActiveRunState
			&& Before.DeployedItemIds == Candidate.DeployedItemIds
			&& Before.ActiveRunItems == Candidate.ActiveRunItems
			&& Before.GeneratedRewardSources
				== Candidate.GeneratedRewardSources;
	}

	bool RejectDifference(
		const TCHAR* Field,
		FString* OutDiagnostic)
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic = FString::Printf(
				TEXT("Legacy Profile item writer is retired; %s cannot change after 0.0.10 cutover."),
				Field);
		}
		return false;
	}
}

Fdemo_mapShanmenLegacyItemWriteFenceProbe
Fdemo_mapShanmenLegacyItemWriteFence::Inspect(
	const FString& StorageRoot,
	const FGuid& OwnerId)
{
	Fdemo_mapShanmenLegacyItemWriteFenceProbe Result;
	if (StorageRoot.IsEmpty() || !OwnerId.IsValid())
	{
		Result.Diagnostic =
			TEXT("Legacy item write-fence inspection requires a storage root and OwnerId.");
		return Result;
	}

	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(StorageRoot, OwnerId);
	Result.PrimaryPath = Storage.PrimaryPath();
	Result.BackupPath = Storage.BackupPath();
	if (IFileManager::Get().FileExists(*Result.PrimaryPath))
	{
		Result.State =
			Edemo_mapShanmenLegacyItemWriteFenceState::RetiredByPrimary;
		Result.Diagnostic =
			TEXT("0.0.10 authority primary exists; legacy item writers are retired.");
		return Result;
	}
	if (IFileManager::Get().FileExists(*Result.BackupPath))
	{
		Result.State =
			Edemo_mapShanmenLegacyItemWriteFenceState::RetiredByBackup;
		Result.Diagnostic =
			TEXT("0.0.10 authority backup exists; legacy item writers remain retired pending recovery.");
		return Result;
	}

	Result.State = Edemo_mapShanmenLegacyItemWriteFenceState::Open;
	Result.Diagnostic =
		TEXT("No 0.0.10 authority document exists; legacy migration sources remain writable.");
	return Result;
}

bool Fdemo_mapShanmenLegacyItemWriteFence::PreservesRetiredProfileItems(
	const Fdemo_mapPersistentProfile& DurableBefore,
	const Fdemo_mapPersistentProfile& Candidate,
	FString* OutDiagnostic)
{
	if (OutDiagnostic)
	{
		OutDiagnostic->Reset();
	}
	if (!DurableBefore.ProfileId.IsValid()
		|| DurableBefore.ProfileId != Candidate.ProfileId)
	{
		return RejectDifference(TEXT("ProfileId"), OutDiagnostic);
	}
	if (DurableBefore.PermanentStash != Candidate.PermanentStash)
	{
		return RejectDifference(TEXT("PermanentStash"), OutDiagnostic);
	}
	if (!(DurableBefore.ShopStock == Candidate.ShopStock))
	{
		return RejectDifference(TEXT("ShopStock"), OutDiagnostic);
	}
	if (!(DurableBefore.PreparationLayout == Candidate.PreparationLayout))
	{
		return RejectDifference(TEXT("PreparationLayout"), OutDiagnostic);
	}
	if (!(DurableBefore.WarehouseLayout == Candidate.WarehouseLayout))
	{
		return RejectDifference(TEXT("WarehouseLayout"), OutDiagnostic);
	}
	if (!SameItemBearingRun(DurableBefore.ActiveRun, Candidate.ActiveRun))
	{
		return RejectDifference(TEXT("ActiveRun item projection"), OutDiagnostic);
	}
	if (OutDiagnostic)
	{
		*OutDiagnostic =
			TEXT("Candidate preserves every retired legacy Profile item field.");
	}
	return true;
}
