#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus
	: uint8
{
	WrittenAndFlushed,
	OpenFailed,
	WriteFailed,
	FlushFailed
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus
	: uint8
{
	Read,
	Missing,
	TooLarge,
	Failed
};

/**
 * Caller-owned filesystem seam for one checkpoint-payload slot.
 *
 * AtomicReplace must perform a same-volume replacement. Returning false must
 * leave the destination unchanged. The adapter never retries any operation.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem() =
		default;

	virtual bool DirectoryExists(const FString& Path) const = 0;
	virtual bool CreateDirectoryTree(const FString& Path) = 0;
	virtual bool FileExists(const FString& Path) const = 0;
	virtual bool DeleteFile(const FString& Path) = 0;
	virtual Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus
	WriteAndFlush(const FString& Path, const TArray<uint8>& Bytes) = 0;
	virtual Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus
	ReadBounded(
		const FString& Path,
		int64 MaximumBytes,
		TArray<uint8>& OutBytes,
		int64& OutObservedSize) const = 0;
	virtual bool AtomicReplace(
		const FString& DestinationPath,
		const FString& SourcePath) = 0;
};

/** Concrete local implementation. No global storage path is selected here. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem final
	: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem
{
public:
	virtual bool DirectoryExists(const FString& Path) const override;
	virtual bool CreateDirectoryTree(const FString& Path) override;
	virtual bool FileExists(const FString& Path) const override;
	virtual bool DeleteFile(const FString& Path) override;
	virtual Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus
	WriteAndFlush(
		const FString& Path,
		const TArray<uint8>& Bytes) override;
	virtual Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus
	ReadBounded(
		const FString& Path,
		int64 MaximumBytes,
		TArray<uint8>& OutBytes,
		int64& OutObservedSize) const override;
	virtual bool AtomicReplace(
		const FString& DestinationPath,
		const FString& SourcePath) override;
};

/**
 * One caller-selected absolute root plus one expected journal identity.
 * Primary and temporary files are deterministically placed in one directory,
 * so replacement cannot cross volumes or silently change journal slots.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext
{
public:
	static constexpr int32 MaximumRootDirectoryCharacters() { return 1024; }

	static bool TryCreate(
		const FString& AbsoluteRootDirectory,
		const FGuid& ExpectedJournalId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext&
			OutContext,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FString& GetRootDirectory() const { return RootDirectory; }
	const FString& GetStorageDirectory() const { return StorageDirectory; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	const FString& GetTemporaryPath() const { return TemporaryPath; }
	const FGuid& GetExpectedJournalId() const { return ExpectedJournalId; }

private:
	FString RootDirectory;
	FString StorageDirectory;
	FString PrimaryPath;
	FString TemporaryPath;
	FGuid ExpectedJournalId;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveStatus
	: uint8
{
	Invalid,
	Saved,
	ContextRejected,
	EnvelopeRejected,
	JournalSlotMismatch,
	EncodingRejected,
	DirectoryCreationFailed,
	StaleTemporaryCleanupFailed,
	TemporaryOpenFailed,
	TemporaryWriteFailed,
	TemporaryFlushFailed,
	TemporaryReadBackFailed,
	TemporaryValidationFailed,
	AtomicReplaceFailed,
	CommittedReadBackFailed,
	CommittedValidationFailed
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveResult
{
public:
	bool IsSuccess() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	const FString& GetTemporaryPath() const { return TemporaryPath; }
	const FGuid& GetEnvelopeId() const { return EnvelopeId; }
	int64 GetEncodedByteCount() const { return EncodedByteCount; }
	bool DidReplacePrimary() const { return bDidReplacePrimary; }
	bool TemporaryFileMayRemain() const { return bTemporaryFileMayRemain; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveStatus::
				Invalid;
	FString Diagnostic;
	FString PrimaryPath;
	FString TemporaryPath;
	FGuid EnvelopeId;
	int64 EncodedByteCount = 0;
	bool bDidReplacePrimary = false;
	bool bTemporaryFileMayRemain = false;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadStatus
	: uint8
{
	Invalid,
	Loaded,
	ContextRejected,
	Missing,
	ReadFailed,
	SizeRejected,
	DecodeRejected,
	JournalSlotMismatch
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadResult
{
public:
	bool IsSuccess() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	int64 GetObservedByteCount() const { return ObservedByteCount; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
	GetDecodeStatus() const
	{
		return DecodeStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
	GetEnvelope() const
	{
		return Envelope;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadStatus::
				Invalid;
	FString Diagnostic;
	FString PrimaryPath;
	int64 ObservedByteCount = INDEX_NONE;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus
		DecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope
		Envelope;
};

/**
 * Stateless one-shot storage protocol. Loaded envelopes remain evidence only:
 * this type accepts no journal, recovery coordinator, owner or surface.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageAdapter
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveResult
	Save(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext&
			Context,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope&
			Envelope,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem&
			FileSystem) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadResult
	Load(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext&
			Context,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem&
			FileSystem) const;
};
