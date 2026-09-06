#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.h"

// Reuse the P20.51 one-shot filesystem seam and its full-flush local backend.
// The operations are byte-oriented; only the P20.53 context and codec select
// the recovery-bundle slot and representation.
using Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem =
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem;
using Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleLocalFileSystem =
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem;
using Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileWriteStatus =
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus;
using Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileReadStatus =
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus;

/**
 * One caller-selected absolute root plus one stable recovery lineage.
 *
 * The lineage is derived from the immutable first journal record rather than
 * the journal identity, which changes after every append. All generations in
 * one lineage therefore replace one primary file in one directory.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext
{
public:
	static constexpr int32 MaximumRootDirectoryCharacters() { return 1024; }

	static bool TryCreate(
		const FString& AbsoluteRootDirectory,
		const FGuid& ExpectedLineageId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			OutContext,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FString& GetRootDirectory() const { return RootDirectory; }
	const FString& GetStorageDirectory() const { return StorageDirectory; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	const FString& GetTemporaryPath() const { return TemporaryPath; }
	const FGuid& GetExpectedLineageId() const { return ExpectedLineageId; }

private:
	FString RootDirectory;
	FString StorageDirectory;
	FString PrimaryPath;
	FString TemporaryPath;
	FGuid ExpectedLineageId;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus
	: uint8
{
	Invalid,
	Saved,
	AlreadyCurrent,
	ContextRejected,
	BundleRejected,
	LineageSlotMismatch,
	MinimumGenerationRejected,
	GenerationBelowWatermark,
	EncodingRejected,
	ExistingReadFailed,
	ExistingSizeRejected,
	ExistingDecodeRejected,
	ExistingLineageMismatch,
	ExistingGenerationNewer,
	ExistingGenerationConflict,
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

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveResult
{
public:
	bool IsSuccess() const;
	bool WasAlreadyCurrent() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	const FString& GetTemporaryPath() const { return TemporaryPath; }
	const FGuid& GetLineageId() const { return LineageId; }
	const FGuid& GetBundleId() const { return BundleId; }
	int32 GetGeneration() const { return Generation; }
	int32 GetMinimumGeneration() const { return MinimumGeneration; }
	int64 GetEncodedByteCount() const { return EncodedByteCount; }
	bool DidReplacePrimary() const { return bDidReplacePrimary; }
	bool TemporaryFileMayRemain() const { return bTemporaryFileMayRemain; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus::
				Invalid;
	FString Diagnostic;
	FString PrimaryPath;
	FString TemporaryPath;
	FGuid LineageId;
	FGuid BundleId;
	int32 Generation = 0;
	int32 MinimumGeneration = 0;
	int64 EncodedByteCount = 0;
	bool bDidReplacePrimary = false;
	bool bTemporaryFileMayRemain = false;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
	: uint8
{
	Invalid,
	Loaded,
	ContextRejected,
	MinimumGenerationRejected,
	Missing,
	ReadFailed,
	SizeRejected,
	DecodeRejected,
	LineageSlotMismatch,
	GenerationBelowWatermark
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadResult
{
public:
	bool IsSuccess() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FString& GetPrimaryPath() const { return PrimaryPath; }
	const FGuid& GetLineageId() const { return LineageId; }
	int32 GetGeneration() const { return Generation; }
	int32 GetMinimumGeneration() const { return MinimumGeneration; }
	int64 GetObservedByteCount() const { return ObservedByteCount; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus
	GetDecodeStatus() const
	{
		return DecodeStatus;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
	GetBundle() const
	{
		return Bundle;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus::
				Invalid;
	FString Diagnostic;
	FString PrimaryPath;
	FGuid LineageId;
	int32 Generation = 0;
	int32 MinimumGeneration = 0;
	int64 ObservedByteCount = INDEX_NONE;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus
		DecodeStatus =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle
		Bundle;
};

/**
 * Stateless single-slot storage protocol for canonical recovery bundles.
 *
 * MinimumGeneration is a caller-owned trusted watermark. It is deliberately
 * not persisted beside the bundle, because a same-file watermark can be
 * rolled back with the evidence it is meant to fence. Loaded bundles remain
 * evidence only and never execute recovery or mutate a journal/surface.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter
{
public:
	static bool TryDeriveLineageId(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			Bundle,
		FGuid& OutLineageId);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveResult
	Save(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			Context,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle&
			Bundle,
		int32 MinimumGeneration,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem&
			FileSystem) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadResult
	Load(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
			Context,
		int32 MinimumGeneration,
		const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem&
			FileSystem) const;
};
