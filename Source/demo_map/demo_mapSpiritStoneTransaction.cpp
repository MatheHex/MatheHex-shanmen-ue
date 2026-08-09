#include "demo_mapSpiritStoneTransaction.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"

namespace
{
	Fdemo_mapSpiritStonePickupResult Reject(
		Edemo_mapSpiritStonePickupStatus Status,
		const FString& Diagnostic,
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapSpiritStonePickupResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.PreviousGeneration = Profile.SaveGeneration;
		Result.CommittedGeneration = Profile.SaveGeneration;
		Result.RiskBefore = Profile.ActiveRun.RiskSpiritStones;
		Result.RiskAfter = Profile.ActiveRun.RiskSpiritStones;
		return Result;
	}
}

Fdemo_mapSpiritStonePickupResult Fdemo_mapSpiritStoneTransaction::Execute(
	Fdemo_mapPersistentProfile& InOutProfile,
	const Fdemo_mapSpiritStonePickupIntent& Intent,
	const Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	FString ValidationError;
	if (!Repository.ValidateProfile(InOutProfile, &ValidationError))
		return Reject(Edemo_mapSpiritStonePickupStatus::ProfileValidationRejected, ValidationError, InOutProfile);
	if (Intent.ExpectedProfileId != InOutProfile.ProfileId)
		return Reject(Edemo_mapSpiritStonePickupStatus::ProfileIdentityMismatch, TEXT("Spirit Stone intent ProfileId is stale."), InOutProfile);
	if (Intent.ExpectedSaveGeneration != InOutProfile.SaveGeneration)
		return Reject(Edemo_mapSpiritStonePickupStatus::ProfileGenerationMismatch, TEXT("Spirit Stone intent SaveGeneration is stale."), InOutProfile);
	if (!InOutProfile.ActiveRun.bHasActiveRun
		|| (InOutProfile.ActiveRun.ActiveRunState != Edemo_mapPersistentActiveRunState::Prepared
			&& InOutProfile.ActiveRun.ActiveRunState != Edemo_mapPersistentActiveRunState::InProgress))
		return Reject(Edemo_mapSpiritStonePickupStatus::RunNotActive, TEXT("Spirit Stone pickup requires the active run."), InOutProfile);
	if (Intent.ExpectedActiveRunId != InOutProfile.ActiveRun.ActiveRunId)
		return Reject(Edemo_mapSpiritStonePickupStatus::RunIdMismatch, TEXT("Spirit Stone intent RunId is stale."), InOutProfile);
	if (Intent.PickupId.IsNone() || Intent.SourceId.IsNone())
		return Reject(Edemo_mapSpiritStonePickupStatus::SourceRejected, TEXT("Spirit Stone pickup/source identity is required."), InOutProfile);
	const bool bLegacyFixedPickup =
		Intent.PickupId == Fdemo_mapSpiritStonePickupContract::PickupId
		&& Intent.SourceId == Fdemo_mapSpiritStonePickupContract::SourceId;
	if ((bLegacyFixedPickup
			&& Intent.Value != Fdemo_mapSpiritStoneRules::FixedWorldPickupValueForFutureTasks)
		|| (!bLegacyFixedPickup
			&& (Intent.Value < 15 || Intent.Value > 120)))
		return Reject(Edemo_mapSpiritStonePickupStatus::ValueRejected, TEXT("Spirit Stone pickup value is outside its authorized contract."), InOutProfile);
	if (InOutProfile.ActiveRun.ConsumedSpiritStoneSourceIds.Contains(Intent.SourceId))
		return Reject(Edemo_mapSpiritStonePickupStatus::DuplicateSource, TEXT("Spirit Stone source was already consumed in this run."), InOutProfile);
	if (Intent.Value > MAX_int64 - InOutProfile.ActiveRun.RiskSpiritStones)
		return Reject(Edemo_mapSpiritStonePickupStatus::CurrencyOverflow, TEXT("Risk Spirit Stone addition would overflow int64."), InOutProfile);

	Fdemo_mapPersistentProfile Candidate = InOutProfile;
	Candidate.ActiveRun.RiskSpiritStones += Intent.Value;
	Candidate.ActiveRun.ConsumedSpiritStoneSourceIds.Add(Intent.SourceId);
	if (!Repository.ValidateProfile(Candidate, &ValidationError))
		return Reject(Edemo_mapSpiritStonePickupStatus::ProfileValidationRejected, TEXT("Spirit Stone candidate rejected: ") + ValidationError, InOutProfile);

	Fdemo_mapPersistentProfile Committed = Candidate;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Committed, Storage);
	Fdemo_mapSpiritStonePickupResult Result = Reject(Edemo_mapSpiritStonePickupStatus::RepositorySaveRejected, Save.Diagnostic, InOutProfile);
	Result.bDiskStateChanged = Save.bDiskStateChanged;
	Result.RepositorySaveStatus = Save.Status;
	if (Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Result.Status = Edemo_mapSpiritStonePickupStatus::CommitOutcomeRequiresReload;
		return Result;
	}
	if (!Save.IsSuccess()) return Result;

	InOutProfile = Committed;
	Result.Status = Edemo_mapSpiritStonePickupStatus::Committed;
	Result.Diagnostic = FString::Printf(TEXT("Risk Spirit Stone source committed exactly once: +%lld."), Intent.Value);
	Result.CommittedGeneration = Committed.SaveGeneration;
	Result.RiskAfter = Committed.ActiveRun.RiskSpiritStones;
	Result.CommittedProfile = Committed;
	return Result;
}
