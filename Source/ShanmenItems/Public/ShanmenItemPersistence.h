#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"

enum class EShanmenItemStoreFailureStage : uint8
{
	None,
	CreateDirectory,
	WriteTemp,
	FlushOrCloseTemp,
	ReadBackTemp,
	ValidateTemp,
	PrepareBackup,
	AtomicReplace,
	ReadBackCommittedPrimary,
	CleanupTemp
};

enum class EShanmenItemSaveStatus : uint8
{
	Saved,
	ValidationRejected,
	SerializationFailed,
	TempWriteFailed,
	TempFlushOrCloseFailed,
	TempVerificationFailed,
	BackupPreparationFailed,
	AtomicReplaceFailed,
	PostCommitVerificationFailed
};

enum class EShanmenItemLoadStatus : uint8
{
	LoadedPrimary,
	RecoveredFromBackup,
	PrimaryMissingBackupRecovered,
	Missing,
	FutureSchemaRejected,
	CorruptPrimaryNoValidBackup,
	InvalidDocument,
	ReadFailed,
	WriteRecoveryFailed
};

enum class EShanmenItemOpenStatus : uint8
{
	CreatedFromMigration,
	OpenedExisting,
	RecoveredExisting,
	InvalidRequest,
	MigrationConflict,
	PersistenceFailure
};

/** Legacy-independent evidence retained for the one-time authority handoff. */
struct SHANMENITEMS_API FShanmenItemMigrationEvidence
{
	FGuid MigrationId;
	FGuid OwnerId;
	int32 SourceProfileSchema = INDEX_NONE;
	int32 SourceSaveGeneration = INDEX_NONE;
	int32 SourceCodeBPersistentRevision = INDEX_NONE;
	int32 SourceCodeBRepositoryRevision = INDEX_NONE;
	int32 DefinitionCount = 0;
	int32 ContainerCount = 0;
	int32 ItemCount = 0;
	FString SourceFingerprint;
	FString CandidateDigest;

	bool IsValid() const;
	bool operator==(const FShanmenItemMigrationEvidence& Other) const;
};

/** Complete, versioned persistence document for the sole 0.0.10 item authority. */
struct SHANMENITEMS_API FShanmenItemAuthorityDocument
{
	static constexpr int32 LegacySchemaVersion = 1;
	static constexpr int32 CurrentSchemaVersion = 2;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid DocumentId;
	FGuid OwnerId;
	int32 SaveGeneration = 0;
	FString CreatedUtc;
	FString LastSavedUtc;
	FShanmenItemMigrationEvidence Migration;
	FString InitialSnapshotDigest;
	FString SnapshotDigest;
	FShanmenItemAuthoritySnapshot Authority;

	bool operator==(const FShanmenItemAuthorityDocument& Other) const;
};

struct SHANMENITEMS_API FShanmenItemStorageContext
{
	FString RootDirectory;
	FGuid OwnerId;
#if WITH_DEV_AUTOMATION_TESTS
	EShanmenItemStoreFailureStage InjectedFailure =
		EShanmenItemStoreFailureStage::None;
#endif

	static FShanmenItemStorageContext Production(const FGuid& OwnerId);
	static FShanmenItemStorageContext ForRoot(
		const FString& Root, const FGuid& OwnerId);
	FString StorageDirectory() const;
	FString PrimaryPath() const;
	FString BackupPath() const;
	FString TempPath() const;
	FString CorruptDirectory() const;
};

struct SHANMENITEMS_API FShanmenItemSaveResult
{
	EShanmenItemSaveStatus Status =
		EShanmenItemSaveStatus::ValidationRejected;
	FString Diagnostic;
	FString PrimaryPath;
	FString BackupPath;
	FString TempPath;
	bool bDiskStateChanged = false;
	bool bCleanupSucceeded = true;
	int32 CommittedGeneration = INDEX_NONE;

	bool IsSuccess() const
	{
		return Status == EShanmenItemSaveStatus::Saved;
	}
};

struct SHANMENITEMS_API FShanmenItemLoadResult
{
	EShanmenItemLoadStatus Status = EShanmenItemLoadStatus::ReadFailed;
	FString Diagnostic;
	FString PrimaryPath;
	FString BackupPath;
	FString TempPath;
	FString QuarantinedPath;
	bool bDiskStateChanged = false;
	/** True when schema-1 bytes were validated and normalized to schema 2 in memory. */
	bool bSchemaUpgraded = false;
	FShanmenItemAuthorityDocument Document;

	bool IsSuccess() const
	{
		return Status == EShanmenItemLoadStatus::LoadedPrimary
			|| Status == EShanmenItemLoadStatus::RecoveredFromBackup
			|| Status == EShanmenItemLoadStatus::PrimaryMissingBackupRecovered;
	}
};

struct SHANMENITEMS_API FShanmenItemOpenResult
{
	EShanmenItemOpenStatus Status =
		EShanmenItemOpenStatus::PersistenceFailure;
	FString Diagnostic;
	bool bDiskStateChanged = false;
	FShanmenItemAuthorityDocument Document;

	bool IsSuccess() const
	{
		return Status == EShanmenItemOpenStatus::CreatedFromMigration
			|| Status == EShanmenItemOpenStatus::OpenedExisting
			|| Status == EShanmenItemOpenStatus::RecoveredExisting;
	}
};

/**
 * Owns the schema-2 JSON document and its atomic filesystem protocol. It never
 * reads or writes legacy Code A/Code B data and has no product startup hook.
 */
class SHANMENITEMS_API FShanmenItemAuthorityStore
{
public:
	FShanmenItemOpenResult OpenOrCreateFromMigration(
		const FShanmenItemAuthoritySnapshot& MigrationCandidate,
		const FShanmenItemMigrationEvidence& Migration,
		const FShanmenItemStorageContext& Storage) const;

	FShanmenItemSaveResult SaveAuthority(
		FShanmenItemAuthorityDocument& InOutDocument,
		const FShanmenItemAuthoritySnapshot& NewAuthority,
		const FShanmenItemStorageContext& Storage) const;

	FShanmenItemLoadResult LoadExisting(
		const FShanmenItemStorageContext& Storage) const;

	static bool ValidateDocument(
		const FShanmenItemAuthorityDocument& Document,
		FString* OutError = nullptr);
	static bool ComputeSnapshotDigest(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FString& OutDigest,
		FString* OutError = nullptr);
	/** Exact digest codec used by schema 1 before RewardMetadata existed. */
	static bool ComputeLegacySchema1SnapshotDigest(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FString& OutDigest,
		FString* OutError = nullptr);
};
