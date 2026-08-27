#pragma once

#include "CoreMinimal.h"

#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "ShanmenItemPersistence.h"
#include "ShanmenItemTypes.h"
#include "demo_mapPersistentProfileTypes.h"

/** Fail-closed outcomes for the one-time 0.0.9B -> 0.0.10 item handoff. */
enum class Edemo_mapShanmenItemMigrationError : uint8
{
	None,
	InvalidTargetContent,
	InvalidCodeAProfile,
	ActiveRunUnsupported,
	InvalidCodeBRecord,
	SourceIdentityMismatch,
	SourceProvenanceMismatch,
	SourceItemMismatch,
	SourceContainerMismatch,
	CandidateRejected
};

/** Immutable evidence for one deterministic, read-only migration candidate. */
struct Fdemo_mapShanmenItemMigrationReceipt
{
	bool bSuccess = false;
	Edemo_mapShanmenItemMigrationError Error =
		Edemo_mapShanmenItemMigrationError::CandidateRejected;
	FGuid MigrationId;
	FGuid OwnerId;
	FGuid ScopeId;
	int32 SourceProfileSchema = INDEX_NONE;
	int32 SourceSaveGeneration = INDEX_NONE;
	int32 SourceCodeBPersistentRevision = INDEX_NONE;
	int32 SourceCodeBRepositoryRevision = INDEX_NONE;
	int32 DefinitionCount = 0;
	int32 ContainerCount = 0;
	int32 ItemCount = 0;
	FString SourceFingerprint;
	FString CandidateDigest;

	bool IsSuccess() const
	{
		return bSuccess
			&& Error == Edemo_mapShanmenItemMigrationError::None
			&& MigrationId.IsValid()
			&& OwnerId.IsValid()
			&& ScopeId.IsValid()
			&& SourceProfileSchema > 0
			&& SourceSaveGeneration >= 0
			&& SourceCodeBPersistentRevision > 0
			&& SourceCodeBRepositoryRevision >= 0
			&& !SourceFingerprint.IsEmpty()
			&& !CandidateDigest.IsEmpty();
	}

	FShanmenItemMigrationEvidence ToPersistenceEvidence() const;
};

struct Fdemo_mapShanmenItemMigrationResult
{
	Edemo_mapShanmenItemMigrationError Error =
		Edemo_mapShanmenItemMigrationError::CandidateRejected;
	FString Diagnostic;
	FShanmenItemAuthoritySnapshot Candidate;
	Fdemo_mapShanmenItemMigrationReceipt Receipt;

	bool IsSuccess() const
	{
		return Error == Edemo_mapShanmenItemMigrationError::None
			&& Receipt.IsSuccess();
	}
};

/**
 * Reads the current Code A Profile and committed Code B sidecar without
 * mutating either source. It returns one canonical ShanmenItems snapshot only
 * when identity, quantity, provenance and container closure agree exactly.
 */
struct Fdemo_mapShanmenItemMigration
{
	static Fdemo_mapShanmenItemMigrationResult BuildCandidate(
		const Fdemo_mapPersistentProfile& CodeAProfile,
		const FCodeBOutOfRaidInventoryRecord& CodeBRecord,
		const FShanmenContentStamp& TargetContent);
};
