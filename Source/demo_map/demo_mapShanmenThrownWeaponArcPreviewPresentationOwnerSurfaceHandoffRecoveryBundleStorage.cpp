#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.h"

#include "Misc/Paths.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using FBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;
	using FCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleCodec;
	using FContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext;
	using IFileSystem =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem;
	using EFileReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileReadStatus;
	using EFileWriteStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileWriteStatus;
	using ESaveStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus;
	using ELoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus;
	using ERecordKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;
	using FRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;

	constexpr TCHAR StorageDirectoryName[] =
		TEXT("ShanmenArcPreviewRecoveryBundles");
	constexpr TCHAR PrimaryExtension[] = TEXT(".smarc-bundle");
	constexpr TCHAR TemporaryExtension[] = TEXT(".tmp");

	bool HasEmbeddedNull(const FString& Value)
	{
		for (const TCHAR Character : Value)
		{
			if (Character == TEXT('\0'))
			{
				return true;
			}
		}
		return false;
	}

	FString NormalizedDirectory(const FString& Value)
	{
		FString Result = Value;
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}

	bool IsValidMinimumGeneration(const int32 MinimumGeneration)
	{
		return MinimumGeneration >= 1
			&& MinimumGeneration <= FBundle::MaximumGeneration();
	}

	bool TryDeriveStableLineageId(
		const FBundle& Bundle,
		FGuid& OutLineageId)
	{
		OutLineageId.Invalidate();
		if (!Bundle.IsValid())
		{
			return false;
		}
		FRecord First;
		if (!Bundle.GetJournal().TryGetRecordAt(0, First)
			|| !First.IsValid()
			|| First.GetKind() != ERecordKind::CheckpointPrepared
			|| First.GetSequence() != 0
			|| First.GetPreviousRecordId().IsValid()
			|| First.GetRecoveryReceiptId().IsValid())
		{
			return false;
		}
		OutLineageId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryBundleLineage.r1"),
			{
				First.GetRecordId().ToString(EGuidFormats::Digits),
				First.GetCheckpointId().ToString(EGuidFormats::Digits),
				First.GetRunId().ToString(EGuidFormats::Digits),
				First.GetConsumerDefinitionDigest().ToString(
					EGuidFormats::Digits)
			});
		return OutLineageId.IsValid();
	}

	bool IsVerifiedBundleBytes(
		const TArray<uint8>& Bytes,
		const FBundle& Expected)
	{
		const auto Decoded = FCodec::Decode(Bytes);
		return Decoded.IsSuccess()
			&& Decoded.GetBundle().Matches(Expected);
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext::
TryCreate(
	const FString& AbsoluteRootDirectory,
	const FGuid& InExpectedLineageId,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
		OutContext,
	FString& OutDiagnostic)
{
	OutContext = {};
	OutDiagnostic.Reset();
	const FString Trimmed = AbsoluteRootDirectory.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || HasEmbeddedNull(Trimmed)
		|| Trimmed.Len() > MaximumRootDirectoryCharacters()
		|| FPaths::IsRelative(Trimmed))
	{
		OutDiagnostic = TEXT(
			"Recovery bundle storage requires one bounded absolute caller-owned root.");
		return false;
	}
	if (!InExpectedLineageId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Recovery bundle storage requires one valid expected lineage identity.");
		return false;
	}

	FString NormalizedRoot = FPaths::ConvertRelativePathToFull(Trimmed);
	FPaths::NormalizeDirectoryName(NormalizedRoot);
	if (NormalizedRoot.IsEmpty()
		|| NormalizedRoot.Len() > MaximumRootDirectoryCharacters())
	{
		OutDiagnostic = TEXT(
			"Recovery bundle storage root could not be normalized safely.");
		return false;
	}
	const FString Directory = NormalizedDirectory(
		FPaths::Combine(NormalizedRoot, StorageDirectoryName));
	const FString Filename =
		InExpectedLineageId.ToString(EGuidFormats::Digits) + PrimaryExtension;
	const FString Primary = FPaths::Combine(Directory, Filename);
	const FString Temporary = Primary + TemporaryExtension;
	if (Directory.IsEmpty() || Primary.IsEmpty() || Temporary.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Recovery bundle storage paths could not be derived.");
		return false;
	}

	OutContext.RootDirectory = MoveTemp(NormalizedRoot);
	OutContext.StorageDirectory = Directory;
	OutContext.PrimaryPath = Primary;
	OutContext.TemporaryPath = Temporary;
	OutContext.ExpectedLineageId = InExpectedLineageId;
	if (!OutContext.IsValid())
	{
		OutContext = {};
		OutDiagnostic = TEXT(
			"Recovery bundle storage context failed canonical validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Recovery bundle storage context created for one stable lineage slot.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext::
IsValid() const
{
	if (RootDirectory.IsEmpty() || HasEmbeddedNull(RootDirectory)
		|| RootDirectory.Len() > MaximumRootDirectoryCharacters()
		|| FPaths::IsRelative(RootDirectory)
		|| !ExpectedLineageId.IsValid())
	{
		return false;
	}
	const FString ExpectedDirectory = NormalizedDirectory(
		FPaths::Combine(RootDirectory, StorageDirectoryName));
	const FString ExpectedPrimary = FPaths::Combine(
		ExpectedDirectory,
		ExpectedLineageId.ToString(EGuidFormats::Digits) + PrimaryExtension);
	return StorageDirectory == ExpectedDirectory
		&& PrimaryPath == ExpectedPrimary
		&& TemporaryPath == ExpectedPrimary + TemporaryExtension
		&& FPaths::GetPath(PrimaryPath) == FPaths::GetPath(TemporaryPath);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveResult::
IsSuccess() const
{
	return Status == ESaveStatus::Saved
		|| Status == ESaveStatus::AlreadyCurrent;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveResult::
WasAlreadyCurrent() const
{
	return Status == ESaveStatus::AlreadyCurrent;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadResult::
IsSuccess() const
{
	return Status == ELoadStatus::Loaded;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter::
TryDeriveLineageId(
	const FBundle& Bundle,
	FGuid& OutLineageId)
{
	return TryDeriveStableLineageId(Bundle, OutLineageId);
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter::
Save(
	const FContext& Context,
	const FBundle& Bundle,
	const int32 MinimumGeneration,
	IFileSystem& FileSystem) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveResult
		Result;
	Result.PrimaryPath = Context.GetPrimaryPath();
	Result.TemporaryPath = Context.GetTemporaryPath();
	Result.BundleId = Bundle.GetBundleId();
	Result.Generation = Bundle.GetGeneration();
	Result.MinimumGeneration = MinimumGeneration;
	if (!Context.IsValid())
	{
		Result.Status = ESaveStatus::ContextRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle save rejected an invalid storage context.");
		return Result;
	}
	if (!Bundle.IsValid()
		|| !TryDeriveStableLineageId(Bundle, Result.LineageId))
	{
		Result.Status = ESaveStatus::BundleRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle save rejected invalid canonical evidence.");
		return Result;
	}
	if (Result.LineageId != Context.GetExpectedLineageId())
	{
		Result.Status = ESaveStatus::LineageSlotMismatch;
		Result.Diagnostic = TEXT(
			"Recovery bundle does not belong to the caller lineage slot.");
		return Result;
	}
	if (!IsValidMinimumGeneration(MinimumGeneration))
	{
		Result.Status = ESaveStatus::MinimumGenerationRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle save requires a trusted minimum generation from 1 through 8.");
		return Result;
	}
	if (Bundle.GetGeneration() < MinimumGeneration)
	{
		Result.Status = ESaveStatus::GenerationBelowWatermark;
		Result.Diagnostic = TEXT(
			"Recovery bundle generation is below the caller-owned trusted watermark.");
		return Result;
	}

	TArray<uint8> Encoded;
	if (!FCodec::TryEncode(Bundle, Encoded)
		|| Encoded.IsEmpty()
		|| Encoded.Num() > FCodec::MaximumEncodedBytes())
	{
		Result.Status = ESaveStatus::EncodingRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle could not be canonically encoded.");
		return Result;
	}
	Result.EncodedByteCount = Encoded.Num();

	if (FileSystem.FileExists(Context.GetPrimaryPath()))
	{
		TArray<uint8> ExistingBytes;
		int64 ExistingSize = INDEX_NONE;
		const EFileReadStatus ExistingRead = FileSystem.ReadBounded(
			Context.GetPrimaryPath(),
			FCodec::MaximumEncodedBytes(),
			ExistingBytes,
			ExistingSize);
		if (ExistingRead == EFileReadStatus::TooLarge)
		{
			Result.Status = ESaveStatus::ExistingSizeRejected;
			Result.Diagnostic = TEXT(
				"Existing recovery bundle exceeds the bounded codec size and was not replaced.");
			return Result;
		}
		if (ExistingRead != EFileReadStatus::Read)
		{
			Result.Status = ESaveStatus::ExistingReadFailed;
			Result.Diagnostic = TEXT(
				"Existing recovery bundle could not be read and was not replaced.");
			return Result;
		}
		const auto ExistingDecoded = FCodec::Decode(ExistingBytes);
		if (!ExistingDecoded.IsSuccess())
		{
			Result.Status = ESaveStatus::ExistingDecodeRejected;
			Result.Diagnostic = TEXT(
				"Existing recovery bundle could not be verified and was not replaced.");
			return Result;
		}
		FGuid ExistingLineageId;
		if (!TryDeriveStableLineageId(
				ExistingDecoded.GetBundle(), ExistingLineageId)
			|| ExistingLineageId != Context.GetExpectedLineageId())
		{
			Result.Status = ESaveStatus::ExistingLineageMismatch;
			Result.Diagnostic = TEXT(
				"Existing recovery bundle belongs to a different stable lineage.");
			return Result;
		}
		const int32 ExistingGeneration =
			ExistingDecoded.GetBundle().GetGeneration();
		if (ExistingGeneration > Bundle.GetGeneration())
		{
			Result.Status = ESaveStatus::ExistingGenerationNewer;
			Result.Diagnostic = TEXT(
				"Existing recovery bundle is newer than the save candidate.");
			return Result;
		}
		if (ExistingGeneration == Bundle.GetGeneration())
		{
			if (ExistingDecoded.GetBundle().Matches(Bundle)
				&& ExistingBytes == Encoded)
			{
				Result.Status = ESaveStatus::AlreadyCurrent;
				Result.Diagnostic = TEXT(
					"Recovery bundle slot already contains identical canonical evidence.");
				return Result;
			}
			Result.Status = ESaveStatus::ExistingGenerationConflict;
			Result.Diagnostic = TEXT(
				"Existing recovery bundle conflicts with another bundle in the same generation.");
			return Result;
		}
	}

	if (!FileSystem.DirectoryExists(Context.GetStorageDirectory())
		&& !FileSystem.CreateDirectoryTree(Context.GetStorageDirectory()))
	{
		Result.Status = ESaveStatus::DirectoryCreationFailed;
		Result.Diagnostic = TEXT(
			"Recovery bundle storage directory could not be created.");
		return Result;
	}
	if (FileSystem.FileExists(Context.GetTemporaryPath())
		&& !FileSystem.DeleteFile(Context.GetTemporaryPath()))
	{
		Result.Status = ESaveStatus::StaleTemporaryCleanupFailed;
		Result.Diagnostic = TEXT(
			"Stale recovery bundle temporary file could not be removed.");
		Result.bTemporaryFileMayRemain = true;
		return Result;
	}

	const EFileWriteStatus WriteStatus =
		FileSystem.WriteAndFlush(Context.GetTemporaryPath(), Encoded);
	if (WriteStatus != EFileWriteStatus::WrittenAndFlushed)
	{
		Result.bTemporaryFileMayRemain = true;
		switch (WriteStatus)
		{
		case EFileWriteStatus::OpenFailed:
			Result.Status = ESaveStatus::TemporaryOpenFailed;
			Result.Diagnostic = TEXT(
				"Recovery bundle temporary file could not be opened.");
			break;
		case EFileWriteStatus::WriteFailed:
			Result.Status = ESaveStatus::TemporaryWriteFailed;
			Result.Diagnostic = TEXT(
				"Recovery bundle temporary write failed.");
			break;
		case EFileWriteStatus::FlushFailed:
			Result.Status = ESaveStatus::TemporaryFlushFailed;
			Result.Diagnostic = TEXT(
				"Recovery bundle temporary full flush failed.");
			break;
		default:
			Result.Status = ESaveStatus::TemporaryWriteFailed;
			Result.Diagnostic = TEXT(
				"Recovery bundle temporary write returned an invalid status.");
			break;
		}
		return Result;
	}
	Result.bTemporaryFileMayRemain = true;

	TArray<uint8> TemporaryBytes;
	int64 TemporarySize = INDEX_NONE;
	const EFileReadStatus TemporaryRead = FileSystem.ReadBounded(
		Context.GetTemporaryPath(),
		FCodec::MaximumEncodedBytes(),
		TemporaryBytes,
		TemporarySize);
	if (TemporaryRead == EFileReadStatus::Missing
		|| TemporaryRead == EFileReadStatus::Failed)
	{
		Result.Status = ESaveStatus::TemporaryReadBackFailed;
		Result.Diagnostic = TEXT(
			"Recovery bundle temporary file could not be read back.");
		return Result;
	}
	if (TemporaryRead != EFileReadStatus::Read
		|| TemporarySize != Encoded.Num()
		|| TemporaryBytes != Encoded
		|| !IsVerifiedBundleBytes(TemporaryBytes, Bundle))
	{
		Result.Status = ESaveStatus::TemporaryValidationFailed;
		Result.Diagnostic = TEXT(
			"Recovery bundle temporary bytes failed exact codec verification.");
		return Result;
	}

	if (!FileSystem.AtomicReplace(
			Context.GetPrimaryPath(), Context.GetTemporaryPath()))
	{
		Result.Status = ESaveStatus::AtomicReplaceFailed;
		Result.Diagnostic = TEXT(
			"Recovery bundle same-volume primary replacement failed.");
		return Result;
	}
	Result.bDidReplacePrimary = true;
	Result.bTemporaryFileMayRemain = false;

	TArray<uint8> CommittedBytes;
	int64 CommittedSize = INDEX_NONE;
	const EFileReadStatus CommittedRead = FileSystem.ReadBounded(
		Context.GetPrimaryPath(),
		FCodec::MaximumEncodedBytes(),
		CommittedBytes,
		CommittedSize);
	if (CommittedRead == EFileReadStatus::Missing
		|| CommittedRead == EFileReadStatus::Failed)
	{
		Result.Status = ESaveStatus::CommittedReadBackFailed;
		Result.Diagnostic = TEXT(
			"Committed recovery bundle could not be read; caller must load to resolve the outcome.");
		return Result;
	}
	if (CommittedRead != EFileReadStatus::Read
		|| CommittedSize != Encoded.Num()
		|| CommittedBytes != Encoded
		|| !IsVerifiedBundleBytes(CommittedBytes, Bundle))
	{
		Result.Status = ESaveStatus::CommittedValidationFailed;
		Result.Diagnostic = TEXT(
			"Committed recovery bundle failed verification; caller must load to resolve the outcome.");
		return Result;
	}

	Result.Status = ESaveStatus::Saved;
	Result.Diagnostic = TEXT(
		"Recovery bundle atomically replaced and verified as evidence only.");
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter::
Load(
	const FContext& Context,
	const int32 MinimumGeneration,
	const IFileSystem& FileSystem) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadResult
		Result;
	Result.PrimaryPath = Context.GetPrimaryPath();
	Result.MinimumGeneration = MinimumGeneration;
	if (!Context.IsValid())
	{
		Result.Status = ELoadStatus::ContextRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle load rejected an invalid storage context.");
		return Result;
	}
	if (!IsValidMinimumGeneration(MinimumGeneration))
	{
		Result.Status = ELoadStatus::MinimumGenerationRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle load requires a trusted minimum generation from 1 through 8.");
		return Result;
	}

	TArray<uint8> Bytes;
	const EFileReadStatus ReadStatus = FileSystem.ReadBounded(
		Context.GetPrimaryPath(),
		FCodec::MaximumEncodedBytes(),
		Bytes,
		Result.ObservedByteCount);
	if (ReadStatus == EFileReadStatus::Missing)
	{
		Result.Status = ELoadStatus::Missing;
		Result.Diagnostic = TEXT(
			"Recovery bundle primary does not exist.");
		return Result;
	}
	if (ReadStatus == EFileReadStatus::TooLarge)
	{
		Result.Status = ELoadStatus::SizeRejected;
		Result.Diagnostic = TEXT(
			"Recovery bundle primary exceeds the bounded codec size.");
		return Result;
	}
	if (ReadStatus != EFileReadStatus::Read)
	{
		Result.Status = ELoadStatus::ReadFailed;
		Result.Diagnostic = TEXT(
			"Recovery bundle primary could not be read.");
		return Result;
	}

	const auto Decoded = FCodec::Decode(Bytes);
	Result.DecodeStatus = Decoded.GetStatus();
	if (!Decoded.IsSuccess())
	{
		Result.Status = ELoadStatus::DecodeRejected;
		Result.Diagnostic = Decoded.GetDiagnostic();
		return Result;
	}
	Result.Generation = Decoded.GetBundle().GetGeneration();
	if (!TryDeriveStableLineageId(
			Decoded.GetBundle(), Result.LineageId)
		|| Result.LineageId != Context.GetExpectedLineageId())
	{
		Result.Status = ELoadStatus::LineageSlotMismatch;
		Result.Diagnostic = TEXT(
			"Decoded recovery bundle belongs to a different stable lineage.");
		return Result;
	}
	if (Result.Generation < MinimumGeneration)
	{
		Result.Status = ELoadStatus::GenerationBelowWatermark;
		Result.Diagnostic = TEXT(
			"Decoded recovery bundle generation is below the caller-owned trusted watermark.");
		return Result;
	}

	Result.Status = ELoadStatus::Loaded;
	Result.Diagnostic = TEXT(
		"Recovery bundle loaded as evidence; current journal and live authority checks remain pending.");
	Result.Bundle = Decoded.GetBundle();
	return Result;
}
