#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

class Fdemo_mapProfileRepository;

enum class Edemo_mapPersistentPreparationCommitStatus : uint8
{
	Committed,
	NoOp,
	ProfileValidationRejected,
	ProfileIdentityMismatch,
	ProfileGenerationMismatch,
	SessionStateRejected,
	LayoutValidationRejected,
	RepositorySaveRejected,
	CommitOutcomeRequiresReload
};

struct Fdemo_mapPersistentPreparationCommitIntent
{
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	Fdemo_mapPersistentPreparationLayout Layout;
};

struct Fdemo_mapPersistentPreparationCommitResult
{
	Edemo_mapPersistentPreparationCommitStatus Status = Edemo_mapPersistentPreparationCommitStatus::ProfileValidationRejected;
	FString Diagnostic;
	int32 PreviousGeneration = 0;
	int32 CommittedGeneration = 0;
	bool bDiskStateChanged = false;
	Edemo_mapProfileSaveStatus RepositorySaveStatus = Edemo_mapProfileSaveStatus::ValidationRejected;
	TOptional<Fdemo_mapPersistentProfile> CommittedProfile;

	bool IsSuccess() const
	{
		return Status == Edemo_mapPersistentPreparationCommitStatus::Committed
			|| Status == Edemo_mapPersistentPreparationCommitStatus::NoOp;
	}
};

class Fdemo_mapPersistentPreparationTransaction
{
public:
	Fdemo_mapPersistentPreparationCommitResult Execute(
		Fdemo_mapPersistentProfile& InOutProfile,
		const Fdemo_mapPersistentPreparationCommitIntent& Intent,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage) const;
};
