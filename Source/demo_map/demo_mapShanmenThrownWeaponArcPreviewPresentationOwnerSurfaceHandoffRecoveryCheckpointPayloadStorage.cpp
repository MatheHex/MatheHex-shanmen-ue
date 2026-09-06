#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

namespace
{
	using FContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext;
	using FEnvelope =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope;
	using FCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec;
	using IFileSystem =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem;
	using EFileReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus;
	using EFileWriteStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus;
	using ESaveStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveStatus;
	using ELoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadStatus;

	constexpr TCHAR StorageDirectoryName[] =
		TEXT("ShanmenArcPreviewRecoveryCheckpointPayloads");
	constexpr TCHAR PrimaryExtension[] = TEXT(".smarc-payload");
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

	bool IsVerifiedEnvelopeBytes(
		const TArray<uint8>& Bytes,
		const FEnvelope& Expected)
	{
		const auto Decoded = FCodec::Decode(Bytes);
		return Decoded.IsSuccess()
			&& Decoded.GetEnvelope().Matches(Expected);
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
DirectoryExists(const FString& Path) const
{
	return IFileManager::Get().DirectoryExists(*Path);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
CreateDirectoryTree(const FString& Path)
{
	return DirectoryExists(Path)
		|| IFileManager::Get().MakeDirectory(*Path, true);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
FileExists(const FString& Path) const
{
	return IFileManager::Get().FileExists(*Path);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
DeleteFile(const FString& Path)
{
	return !FileExists(Path)
		|| IFileManager::Get().Delete(*Path, false, true, true);
}

Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
WriteAndFlush(const FString& Path, const TArray<uint8>& Bytes)
{
	TUniquePtr<IFileHandle> Handle(
		FPlatformFileManager::Get().GetPlatformFile().OpenWrite(
			*Path, false, false));
	if (!Handle)
	{
		return EFileWriteStatus::OpenFailed;
	}
	if (!Bytes.IsEmpty()
		&& !Handle->Write(Bytes.GetData(), Bytes.Num()))
	{
		Handle.Reset();
		return EFileWriteStatus::WriteFailed;
	}
	if (!Handle->Flush(true))
	{
		Handle.Reset();
		return EFileWriteStatus::FlushFailed;
	}
	Handle.Reset();
	return EFileWriteStatus::WrittenAndFlushed;
}

Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
ReadBounded(
	const FString& Path,
	const int64 MaximumBytes,
	TArray<uint8>& OutBytes,
	int64& OutObservedSize) const
{
	OutBytes.Reset();
	OutObservedSize = INDEX_NONE;
	if (MaximumBytes < 0)
	{
		return EFileReadStatus::Failed;
	}
	if (!FileExists(Path))
	{
		return EFileReadStatus::Missing;
	}
	TUniquePtr<IFileHandle> Handle(
		FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Path));
	if (!Handle)
	{
		return EFileReadStatus::Failed;
	}
	const int64 FileSize = Handle->Size();
	OutObservedSize = FileSize;
	if (FileSize < 0)
	{
		return EFileReadStatus::Failed;
	}
	if (FileSize > MaximumBytes || FileSize > MAX_int32)
	{
		return EFileReadStatus::TooLarge;
	}
	OutBytes.SetNumUninitialized(static_cast<int32>(FileSize));
	if (FileSize > 0
		&& !Handle->Read(OutBytes.GetData(), FileSize))
	{
		OutBytes.Reset();
		return EFileReadStatus::Failed;
	}
	if (Handle->Size() != FileSize)
	{
		OutBytes.Reset();
		OutObservedSize = Handle->Size();
		return OutObservedSize > MaximumBytes
			? EFileReadStatus::TooLarge
			: EFileReadStatus::Failed;
	}
	return EFileReadStatus::Read;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem::
AtomicReplace(
	const FString& DestinationPath,
	const FString& SourcePath)
{
	return FileExists(SourcePath)
		&& IFileManager::Get().Move(
			*DestinationPath,
			*SourcePath,
			true,
			false,
			true,
			true);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext::
TryCreate(
	const FString& AbsoluteRootDirectory,
	const FGuid& InExpectedJournalId,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext&
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
			"Checkpoint payload storage requires one bounded absolute caller-owned root.");
		return false;
	}
	if (!InExpectedJournalId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Checkpoint payload storage requires one valid expected journal identity.");
		return false;
	}

	FString NormalizedRoot = FPaths::ConvertRelativePathToFull(Trimmed);
	FPaths::NormalizeDirectoryName(NormalizedRoot);
	if (NormalizedRoot.IsEmpty()
		|| NormalizedRoot.Len() > MaximumRootDirectoryCharacters())
	{
		OutDiagnostic = TEXT(
			"Checkpoint payload storage root could not be normalized safely.");
		return false;
	}
	const FString Directory = NormalizedDirectory(
		FPaths::Combine(NormalizedRoot, StorageDirectoryName));
	const FString Filename =
		InExpectedJournalId.ToString(EGuidFormats::Digits) + PrimaryExtension;
	const FString Primary = FPaths::Combine(Directory, Filename);
	const FString Temporary = Primary + TemporaryExtension;
	if (Directory.IsEmpty() || Primary.IsEmpty() || Temporary.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Checkpoint payload storage paths could not be derived.");
		return false;
	}

	OutContext.RootDirectory = MoveTemp(NormalizedRoot);
	OutContext.StorageDirectory = Directory;
	OutContext.PrimaryPath = Primary;
	OutContext.TemporaryPath = Temporary;
	OutContext.ExpectedJournalId = InExpectedJournalId;
	if (!OutContext.IsValid())
	{
		OutContext = {};
		OutDiagnostic = TEXT(
			"Checkpoint payload storage context failed canonical validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Checkpoint payload storage context created for one journal slot.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext::
IsValid() const
{
	if (RootDirectory.IsEmpty() || HasEmbeddedNull(RootDirectory)
		|| RootDirectory.Len() > MaximumRootDirectoryCharacters()
		|| FPaths::IsRelative(RootDirectory)
		|| !ExpectedJournalId.IsValid())
	{
		return false;
	}
	const FString ExpectedDirectory = NormalizedDirectory(
		FPaths::Combine(RootDirectory, StorageDirectoryName));
	const FString ExpectedPrimary = FPaths::Combine(
		ExpectedDirectory,
		ExpectedJournalId.ToString(EGuidFormats::Digits) + PrimaryExtension);
	return StorageDirectory == ExpectedDirectory
		&& PrimaryPath == ExpectedPrimary
		&& TemporaryPath == ExpectedPrimary + TemporaryExtension
		&& FPaths::GetPath(PrimaryPath) == FPaths::GetPath(TemporaryPath);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveResult::
IsSuccess() const
{
	return Status == ESaveStatus::Saved;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadResult::
IsSuccess() const
{
	return Status == ELoadStatus::Loaded;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageAdapter::
Save(
	const FContext& Context,
	const FEnvelope& Envelope,
	IFileSystem& FileSystem) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveResult
		Result;
	Result.PrimaryPath = Context.GetPrimaryPath();
	Result.TemporaryPath = Context.GetTemporaryPath();
	Result.EnvelopeId = Envelope.GetEnvelopeId();
	if (!Context.IsValid())
	{
		Result.Status = ESaveStatus::ContextRejected;
		Result.Diagnostic = TEXT(
			"Checkpoint payload save rejected an invalid storage context.");
		return Result;
	}
	if (!Envelope.IsValid())
	{
		Result.Status = ESaveStatus::EnvelopeRejected;
		Result.Diagnostic = TEXT(
			"Checkpoint payload save rejected an invalid envelope.");
		return Result;
	}
	if (Envelope.GetJournalId() != Context.GetExpectedJournalId())
	{
		Result.Status = ESaveStatus::JournalSlotMismatch;
		Result.Diagnostic = TEXT(
			"Checkpoint payload envelope does not match the caller journal slot.");
		return Result;
	}

	TArray<uint8> Encoded;
	if (!FCodec::TryEncode(Envelope, Encoded)
		|| Encoded.IsEmpty()
		|| Encoded.Num() > FCodec::MaximumEncodedBytes())
	{
		Result.Status = ESaveStatus::EncodingRejected;
		Result.Diagnostic = TEXT(
			"Checkpoint payload envelope could not be canonically encoded.");
		return Result;
	}
	Result.EncodedByteCount = Encoded.Num();

	if (!FileSystem.DirectoryExists(Context.GetStorageDirectory())
		&& !FileSystem.CreateDirectoryTree(Context.GetStorageDirectory()))
	{
		Result.Status = ESaveStatus::DirectoryCreationFailed;
		Result.Diagnostic = TEXT(
			"Checkpoint payload storage directory could not be created.");
		return Result;
	}
	if (FileSystem.FileExists(Context.GetTemporaryPath())
		&& !FileSystem.DeleteFile(Context.GetTemporaryPath()))
	{
		Result.Status = ESaveStatus::StaleTemporaryCleanupFailed;
		Result.Diagnostic = TEXT(
			"Stale checkpoint payload temporary file could not be removed.");
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
				"Checkpoint payload temporary file could not be opened.");
			break;
		case EFileWriteStatus::WriteFailed:
			Result.Status = ESaveStatus::TemporaryWriteFailed;
			Result.Diagnostic = TEXT(
				"Checkpoint payload temporary write failed.");
			break;
		case EFileWriteStatus::FlushFailed:
			Result.Status = ESaveStatus::TemporaryFlushFailed;
			Result.Diagnostic = TEXT(
				"Checkpoint payload temporary full flush failed.");
			break;
		default:
			Result.Status = ESaveStatus::TemporaryWriteFailed;
			Result.Diagnostic = TEXT(
				"Checkpoint payload temporary write returned an invalid status.");
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
			"Checkpoint payload temporary file could not be read back.");
		return Result;
	}
	if (TemporaryRead != EFileReadStatus::Read
		|| TemporarySize != Encoded.Num()
		|| TemporaryBytes != Encoded
		|| !IsVerifiedEnvelopeBytes(TemporaryBytes, Envelope))
	{
		Result.Status = ESaveStatus::TemporaryValidationFailed;
		Result.Diagnostic = TEXT(
			"Checkpoint payload temporary bytes failed exact codec verification.");
		return Result;
	}

	if (!FileSystem.AtomicReplace(
			Context.GetPrimaryPath(), Context.GetTemporaryPath()))
	{
		Result.Status = ESaveStatus::AtomicReplaceFailed;
		Result.Diagnostic = TEXT(
			"Checkpoint payload same-volume primary replacement failed.");
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
			"Committed checkpoint payload could not be read; caller must load to resolve the outcome.");
		return Result;
	}
	if (CommittedRead != EFileReadStatus::Read
		|| CommittedSize != Encoded.Num()
		|| CommittedBytes != Encoded
		|| !IsVerifiedEnvelopeBytes(CommittedBytes, Envelope))
	{
		Result.Status = ESaveStatus::CommittedValidationFailed;
		Result.Diagnostic = TEXT(
			"Committed checkpoint payload failed verification; caller must load to resolve the outcome.");
		return Result;
	}

	Result.Status = ESaveStatus::Saved;
	Result.Diagnostic = TEXT(
		"Checkpoint payload atomically replaced and verified as evidence only.");
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageAdapter::
Load(
	const FContext& Context,
	const IFileSystem& FileSystem) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadResult
		Result;
	Result.PrimaryPath = Context.GetPrimaryPath();
	if (!Context.IsValid())
	{
		Result.Status = ELoadStatus::ContextRejected;
		Result.Diagnostic = TEXT(
			"Checkpoint payload load rejected an invalid storage context.");
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
			"Checkpoint payload primary does not exist.");
		return Result;
	}
	if (ReadStatus == EFileReadStatus::TooLarge)
	{
		Result.Status = ELoadStatus::SizeRejected;
		Result.Diagnostic = TEXT(
			"Checkpoint payload primary exceeds the bounded codec size.");
		return Result;
	}
	if (ReadStatus != EFileReadStatus::Read)
	{
		Result.Status = ELoadStatus::ReadFailed;
		Result.Diagnostic = TEXT(
			"Checkpoint payload primary could not be read.");
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
	if (Decoded.GetEnvelope().GetJournalId()
		!= Context.GetExpectedJournalId())
	{
		Result.Status = ELoadStatus::JournalSlotMismatch;
		Result.Diagnostic = TEXT(
			"Decoded checkpoint payload belongs to a different journal slot.");
		return Result;
	}

	Result.Status = ELoadStatus::Loaded;
	Result.Diagnostic = TEXT(
		"Checkpoint payload loaded and decoded as evidence; journal and live authority checks remain pending.");
	Result.Envelope = Decoded.GetEnvelope();
	return Result;
}
