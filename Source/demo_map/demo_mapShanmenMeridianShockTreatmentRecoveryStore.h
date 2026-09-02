#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenMeridianShockTreatmentRecoveryIntent.h"
#include "demo_mapShanmenMeridianShockTreatmentRecoveryProof.h"

enum class Edemo_mapShanmenTreatmentRecoveryStoreFailureStage : uint8
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

enum class Edemo_mapShanmenTreatmentRecoverySaveStatus : uint8
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

enum class Edemo_mapShanmenTreatmentRecoveryLoadStatus : uint8
{
	LoadedPrimary,
	MigratedPreviousSchema,
	RecoveredFromBackup,
	PrimaryMissingBackupRecovered,
	Missing,
	FutureSchemaRejected,
	CorruptPrimaryNoValidBackup,
	InvalidDocument,
	ReadFailed,
	WriteRecoveryFailed
};

enum class Edemo_mapShanmenTreatmentRecoveryMutationStatus : uint8
{
	Recorded,
	AlreadyRecorded,
	IntentRecorded,
	IntentAlreadyRecorded,
	Promoted,
	AlreadyPromoted,
	Forgotten,
	AlreadyAbsent,
	IntentForgotten,
	IntentAlreadyAbsent,
	InvalidRequest,
	Conflict,
	LoadFailed,
	SaveFailed
};

/**
 * Complete Run-scoped persistence document for prepared Meridian Shock
 * treatment intents and committed treatment proofs. It contains no item
 * quantity, inventory projection or active-condition state.
 */
struct Fdemo_mapShanmenTreatmentRecoveryDocument
{
	static constexpr int32 PreviousSchemaVersion = 1;
	static constexpr int32 CurrentSchemaVersion = 2;
	static constexpr int32 MaximumIntentCount = 256;
	static constexpr int32 MaximumProofCount = 256;
	static constexpr int32 MaximumEntryCount = 256;

	int32 SchemaVersion = CurrentSchemaVersion;
	FGuid DocumentId;
	FGuid OwnerId;
	FGuid RunId;
	int32 SaveGeneration = 0;
	FString CreatedUtc;
	FString LastSavedUtc;
	FGuid IntentSetId;
	TArray<Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent> Intents;
	FGuid ProofSetId;
	TArray<Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof> Proofs;

	bool operator==(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Other) const;
};

struct Fdemo_mapShanmenTreatmentRecoveryStorageContext
{
	FString RootDirectory;
	FGuid OwnerId;
	FGuid RunId;
#if WITH_DEV_AUTOMATION_TESTS
	Edemo_mapShanmenTreatmentRecoveryStoreFailureStage InjectedFailure =
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::None;
#endif

	static Fdemo_mapShanmenTreatmentRecoveryStorageContext Production(
		const FGuid& OwnerId,
		const FGuid& RunId);
	static Fdemo_mapShanmenTreatmentRecoveryStorageContext ForRoot(
		const FString& Root,
		const FGuid& OwnerId,
		const FGuid& RunId);
	FString StorageDirectory() const;
	FString PrimaryPath() const;
	FString BackupPath() const;
	FString TempPath() const;
	FString CorruptDirectory() const;
	bool IsValid() const;
};

struct Fdemo_mapShanmenTreatmentRecoverySaveResult
{
	Edemo_mapShanmenTreatmentRecoverySaveStatus Status =
		Edemo_mapShanmenTreatmentRecoverySaveStatus::ValidationRejected;
	FString Diagnostic;
	FString PrimaryPath;
	FString BackupPath;
	FString TempPath;
	bool bDiskStateChanged = false;
	bool bCleanupSucceeded = true;
	int32 CommittedGeneration = INDEX_NONE;

	bool IsSuccess() const
	{
		return Status == Edemo_mapShanmenTreatmentRecoverySaveStatus::Saved;
	}
};

struct Fdemo_mapShanmenTreatmentRecoveryLoadResult
{
	Edemo_mapShanmenTreatmentRecoveryLoadStatus Status =
		Edemo_mapShanmenTreatmentRecoveryLoadStatus::ReadFailed;
	FString Diagnostic;
	FString PrimaryPath;
	FString BackupPath;
	FString TempPath;
	FString QuarantinedPath;
	bool bDiskStateChanged = false;
	bool bMigratedFromPreviousSchema = false;
	bool bRecoveredFromBackup = false;
	int32 SourceSchemaVersion = INDEX_NONE;
	Fdemo_mapShanmenTreatmentRecoveryDocument Document;

	bool IsSuccess() const
	{
		return Status
				== Edemo_mapShanmenTreatmentRecoveryLoadStatus::LoadedPrimary
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
					MigratedPreviousSchema
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
					RecoveredFromBackup
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
					PrimaryMissingBackupRecovered;
	}
};

struct Fdemo_mapShanmenTreatmentRecoveryMutationResult
{
	Edemo_mapShanmenTreatmentRecoveryMutationStatus Status =
		Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
	FString Diagnostic;
	bool bDiskStateChanged = false;
	Fdemo_mapShanmenTreatmentRecoveryDocument Document;

	bool IsSuccess() const
	{
		return Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Recorded
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					AlreadyRecorded
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					IntentRecorded
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					IntentAlreadyRecorded
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Promoted
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					AlreadyPromoted
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Forgotten
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					AlreadyAbsent
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					IntentForgotten
			|| Status
				== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
					IntentAlreadyAbsent;
	}
};

/**
 * Sole persistence owner for condition-domain Meridian Shock treatment intents
 * and proofs. Every mutation is canonicalized, written through a verified
 * temporary file, backed up, atomically replaced on the same volume and read
 * back before it is reported as committed.
 */
class Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore
{
public:
	Fdemo_mapShanmenTreatmentRecoveryMutationResult RecordIntent(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const;
	Fdemo_mapShanmenTreatmentRecoveryMutationResult PromoteIntentToProof(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent,
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const;
	Fdemo_mapShanmenTreatmentRecoveryMutationResult ForgetIntent(
		const FGuid& TreatmentId,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const;
	Fdemo_mapShanmenTreatmentRecoveryMutationResult RecordProof(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const;
	Fdemo_mapShanmenTreatmentRecoveryMutationResult ForgetProof(
		const FGuid& TreatmentId,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const;
	Fdemo_mapShanmenTreatmentRecoveryLoadResult LoadExisting(
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const;

	static bool ValidateDocument(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document,
		FString* OutError = nullptr);
};
