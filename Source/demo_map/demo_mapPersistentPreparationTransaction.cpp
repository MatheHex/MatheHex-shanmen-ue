#include "demo_mapPersistentPreparationTransaction.h"

#include "demo_mapProfileRepository.h"

namespace
{
	Fdemo_mapPersistentPreparationCommitResult Reject(
		Edemo_mapPersistentPreparationCommitStatus Status,
		const FString& Diagnostic,
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapPersistentPreparationCommitResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.PreviousGeneration = Profile.SaveGeneration;
		Result.CommittedGeneration = Profile.SaveGeneration;
		return Result;
	}
}

Fdemo_mapPersistentPreparationCommitResult Fdemo_mapPersistentPreparationTransaction::Execute(
	Fdemo_mapPersistentProfile& InOutProfile,
	const Fdemo_mapPersistentPreparationCommitIntent& Intent,
	const Fdemo_mapProfileRepository& Repository,
	const Fdemo_mapProfileStorageContext& Storage) const
{
	FString ValidationError;
	if (!Repository.ValidateProfile(InOutProfile, &ValidationError))
		return Reject(Edemo_mapPersistentPreparationCommitStatus::ProfileValidationRejected, ValidationError, InOutProfile);
	if (Intent.ExpectedProfileId != InOutProfile.ProfileId)
		return Reject(Edemo_mapPersistentPreparationCommitStatus::ProfileIdentityMismatch, TEXT("Persistent layout intent ProfileId is stale."), InOutProfile);
	if (Intent.ExpectedSaveGeneration != InOutProfile.SaveGeneration)
		return Reject(Edemo_mapPersistentPreparationCommitStatus::ProfileGenerationMismatch, TEXT("Persistent layout intent SaveGeneration is stale."), InOutProfile);
	if (InOutProfile.ActiveRun.bHasActiveRun)
		return Reject(Edemo_mapPersistentPreparationCommitStatus::SessionStateRejected, TEXT("Persistent layout commits are allowed only in ReadyForPreparation."), InOutProfile);
	if (Intent.Layout == InOutProfile.PreparationLayout)
	{
		Fdemo_mapPersistentPreparationCommitResult Result = Reject(
			Edemo_mapPersistentPreparationCommitStatus::NoOp,
			TEXT("Persistent layout already matches; generation and bytes remain stable."),
			InOutProfile);
		Result.RepositorySaveStatus = Edemo_mapProfileSaveStatus::Saved;
		Result.CommittedProfile = InOutProfile;
		return Result;
	}

	Fdemo_mapPersistentProfile Candidate = InOutProfile;
	Candidate.PreparationLayout = Intent.Layout;
	if (!Repository.ValidateProfile(Candidate, &ValidationError))
		return Reject(
			Edemo_mapPersistentPreparationCommitStatus::LayoutValidationRejected,
			TEXT("Persistent layout rejected: ") + ValidationError,
			InOutProfile);

	Fdemo_mapPersistentProfile Committed = Candidate;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Committed, Storage);
	Fdemo_mapPersistentPreparationCommitResult Result = Reject(
		Edemo_mapPersistentPreparationCommitStatus::RepositorySaveRejected,
		Save.Diagnostic,
		InOutProfile);
	Result.bDiskStateChanged = Save.bDiskStateChanged;
	Result.RepositorySaveStatus = Save.Status;
	if (Save.Status == Edemo_mapProfileSaveStatus::PostCommitVerificationFailed)
	{
		Result.Status = Edemo_mapPersistentPreparationCommitStatus::CommitOutcomeRequiresReload;
		return Result;
	}
	if (!Save.IsSuccess()) return Result;

	InOutProfile = Committed;
	Result.Status = Edemo_mapPersistentPreparationCommitStatus::Committed;
	Result.Diagnostic = TEXT("Persistent preparation layout committed exactly once.");
	Result.CommittedGeneration = Committed.SaveGeneration;
	Result.CommittedProfile = Committed;
	return Result;
}
