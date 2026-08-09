#include "demo_map0909BCodeBItemBridge.h"

#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "demo_mapGameMode.h"

FString Fdemo_map0909BCodeBItemBridge::ObserveConfirmedActivation(
	Ademo_mapGameMode& GameMode,
	const FGuid& StartAttemptId,
	const FGuid& OwnerId,
	const FGuid& RunInstanceId,
	const FCodeBLoadoutSelection& LoadoutSelection)
{
	FString Diagnostic;
	GameMode.Observe0909BConfirmedRun(OwnerId, RunInstanceId, LoadoutSelection, Diagnostic);
	return FString::Printf(TEXT("StartAttemptId=%s | %s"),
		*StartAttemptId.ToString(EGuidFormats::DigitsWithHyphens), *Diagnostic);
}

bool Fdemo_map0909BCodeBItemBridge::VerifyNoActiveRunSession(
	const FString& StorageRoot,
	const FGuid& OwnerId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!OwnerId.IsValid())
	{
		OutDiagnostic = TEXT("Technical rollback had no valid Profile owner to inspect.");
		return false;
	}
	const bool bHasActive = FCodeBOutOfRaidProfileStore::HasActiveRunInventorySession(
		StorageRoot, OwnerId, &OutDiagnostic);
	if (bHasActive)
	{
		OutDiagnostic = TEXT("A P6 active-run session exists after technical deployment failure.");
		return false;
	}
	if (!OutDiagnostic.IsEmpty())
	{
		return false;
	}
	OutDiagnostic = TEXT("No P6 active-run session exists after technical deployment failure.");
	return true;
}
