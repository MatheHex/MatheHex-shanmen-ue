#include "ShanmenItemPersistence.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

namespace
{
	enum class EReadKind : uint8
	{
		Missing,
		Valid,
		FutureSchema,
		Invalid,
		ReadFailure
	};

	struct FReadResult
	{
		EReadKind Kind = EReadKind::Invalid;
		FString Diagnostic;
		TArray<uint8> Bytes;
		bool bSchemaUpgraded = false;
		FShanmenItemAuthorityDocument Document;
	};

	void SetError(FString* OutError, const FString& Error)
	{
		if (OutError)
		{
			*OutError = Error;
		}
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool TryGuid(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FGuid& Out)
	{
		FString Text;
		return Object.IsValid()
			&& Object->TryGetStringField(Field, Text)
			&& Text.Len() == 32
			&& FGuid::ParseExact(Text, EGuidFormats::Digits, Out)
			&& Out.IsValid();
	}

	bool TryInt32(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& Out)
	{
		double Number = 0.0;
		if (!Object.IsValid()
			|| !Object->TryGetNumberField(Field, Number)
			|| !FMath::IsFinite(Number)
			|| Number < static_cast<double>(MIN_int32)
			|| Number > static_cast<double>(MAX_int32)
			|| FMath::FloorToDouble(Number) != Number)
		{
			return false;
		}
		Out = static_cast<int32>(Number);
		return true;
	}

	bool HasExactFields(
		const TSharedPtr<FJsonObject>& Object,
		const TArray<FString>& Expected)
	{
		if (!Object.IsValid() || Object->Values.Num() != Expected.Num())
		{
			return false;
		}
		for (const FString& Field : Expected)
		{
			if (!Object->HasField(Field))
			{
				return false;
			}
		}
		return true;
	}

	bool IsSha256(const FString& Text)
	{
		if (Text.Len() != 64)
		{
			return false;
		}
		for (const TCHAR Character : Text)
		{
			if (!FChar::IsHexDigit(Character))
			{
				return false;
			}
		}
		return true;
	}

	bool HashBytes(const TArray<uint8>& Bytes, FString& OutDigest)
	{
		OutDigest.Reset();
		uint8 Digest[SHA256_DIGEST_LENGTH]{};
		const uint8* Data = Bytes.IsEmpty() ? nullptr : Bytes.GetData();
		if (!SHA256(Data, static_cast<size_t>(Bytes.Num()), Digest))
		{
			return false;
		}
		OutDigest.Reserve(SHA256_DIGEST_LENGTH * 2);
		for (const uint8 Byte : Digest)
		{
			OutDigest += FString::Printf(TEXT("%02X"), Byte);
		}
		return IsSha256(OutDigest);
	}

	bool JsonToBytes(
		const TSharedRef<FJsonObject>& Object,
		TArray<uint8>& OutBytes,
		FString* OutError)
	{
		FString Payload;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
		if (!FJsonSerializer::Serialize(Object, Writer))
		{
			SetError(OutError, TEXT("Authority document JSON serialization failed."));
			return false;
		}
		FTCHARToUTF8 Utf8(*Payload);
		OutBytes.Reset();
		OutBytes.Append(
			reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		return !OutBytes.IsEmpty();
	}

	bool CanonicalizeSnapshot(
		const FShanmenItemAuthoritySnapshot& Input,
		FShanmenItemAuthoritySnapshot& OutCanonical,
		FString* OutError)
	{
		FShanmenItemRepository Repository;
		EShanmenItemTransactionError Error = EShanmenItemTransactionError::None;
		if (!Repository.TryLoadSnapshot(Input, &Error))
		{
			SetError(OutError, FString::Printf(
				TEXT("Authority snapshot invariant validation failed (%d)."),
				static_cast<int32>(Error)));
			return false;
		}
		OutCanonical = Repository.CaptureSnapshot();
		return true;
	}

	bool AuthorityBelongsTo(
		const FShanmenItemAuthoritySnapshot& Authority,
		const FGuid& OwnerId)
	{
		if (!OwnerId.IsValid())
		{
			return false;
		}
		for (const FShanmenItemContainer& Container : Authority.Containers)
		{
			if (Container.OwnerId != OwnerId)
			{
				return false;
			}
		}
		for (const FShanmenItemInstance& Item : Authority.Items)
		{
			if (Item.OwnerId != OwnerId)
			{
				return false;
			}
		}
		for (const FShanmenItemReservationSnapshot& Reservation : Authority.Reservations)
		{
			if (Reservation.OwnerId != OwnerId)
			{
				return false;
			}
		}
		return true;
	}

	FGuid ExpectedDocumentId(const FShanmenItemAuthorityDocument& Document)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("Shanmen.Items.AuthorityDocument.r1")),
			{ GuidDigits(Document.OwnerId), GuidDigits(Document.Migration.MigrationId) });
	}

	bool ShouldFail(
		const FShanmenItemStorageContext& Storage,
		EShanmenItemStoreFailureStage Stage)
	{
#if WITH_DEV_AUTOMATION_TESTS
		return Storage.InjectedFailure == Stage;
#else
		return false;
#endif
	}

	bool SnapshotToObject(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		TSharedPtr<FJsonObject>& OutObject,
		FString* OutError)
	{
		OutObject = FJsonObjectConverter::UStructToJsonObject(Snapshot);
		if (!OutObject.IsValid())
		{
			SetError(OutError, TEXT("Authority snapshot reflection serialization failed."));
			return false;
		}
		return true;
	}

	bool RemoveRewardMetadataForSchema1(
		const TSharedPtr<FJsonObject>& AuthorityObject,
		FString* OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!AuthorityObject.IsValid()
			|| !AuthorityObject->TryGetArrayField(TEXT("Items"), Items)
			|| !Items)
		{
			SetError(OutError, TEXT("Authority snapshot has no Items array."));
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Items)
		{
			const TSharedPtr<FJsonObject> Item = Value.IsValid()
				? Value->AsObject() : nullptr;
			if (!Item.IsValid() || !Item->HasField(TEXT("RewardMetadata")))
			{
				SetError(OutError, TEXT("Authority schema-2 item has no RewardMetadata field."));
				return false;
			}
			Item->RemoveField(TEXT("RewardMetadata"));
		}
		return true;
	}

	bool PrepareSchema1AuthorityForStrictConversion(
		const TSharedPtr<FJsonObject>& AuthorityObject,
		FString* OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!AuthorityObject.IsValid()
			|| !AuthorityObject->TryGetArrayField(TEXT("Items"), Items)
			|| !Items)
		{
			SetError(OutError, TEXT("Authority schema-1 snapshot has no Items array."));
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Items)
		{
			const TSharedPtr<FJsonObject> Item = Value.IsValid()
				? Value->AsObject() : nullptr;
			if (!Item.IsValid() || Item->HasField(TEXT("RewardMetadata")))
			{
				SetError(OutError, TEXT("Authority schema-1 item fields are invalid."));
				return false;
			}
			TSharedPtr<FJsonObject> DefaultMetadata =
				FJsonObjectConverter::UStructToJsonObject(
					FShanmenItemRewardMetadata());
			if (!DefaultMetadata.IsValid())
			{
				SetError(OutError, TEXT("Default reward metadata JSON could not be created."));
				return false;
			}
			Item->SetObjectField(
				TEXT("RewardMetadata"), DefaultMetadata.ToSharedRef());
		}
		return true;
	}

	bool SameMigrationSource(
		const FShanmenItemMigrationEvidence& Left,
		const FShanmenItemMigrationEvidence& Right)
	{
		return Left.MigrationId == Right.MigrationId
			&& Left.OwnerId == Right.OwnerId
			&& Left.SourceProfileSchema == Right.SourceProfileSchema
			&& Left.SourceSaveGeneration == Right.SourceSaveGeneration
			&& Left.SourceCodeBPersistentRevision
				== Right.SourceCodeBPersistentRevision
			&& Left.SourceCodeBRepositoryRevision
				== Right.SourceCodeBRepositoryRevision
			&& Left.DefinitionCount == Right.DefinitionCount
			&& Left.ContainerCount == Right.ContainerCount
			&& Left.ItemCount == Right.ItemCount
			&& Left.SourceFingerprint == Right.SourceFingerprint;
	}

	bool ValidateLegacySchema1Document(
		const FShanmenItemAuthorityDocument& Document,
		FString* OutError)
	{
		FDateTime Created;
		FDateTime Saved;
		if (Document.SchemaVersion
				!= FShanmenItemAuthorityDocument::LegacySchemaVersion
			|| !Document.DocumentId.IsValid()
			|| !Document.OwnerId.IsValid()
			|| Document.SaveGeneration < 0
			|| !Document.Migration.IsValid()
			|| Document.Migration.OwnerId != Document.OwnerId
			|| Document.DocumentId != ExpectedDocumentId(Document)
			|| !FDateTime::ParseIso8601(*Document.CreatedUtc, Created)
			|| !FDateTime::ParseIso8601(*Document.LastSavedUtc, Saved)
			|| Saved < Created
			|| !IsSha256(Document.InitialSnapshotDigest)
			|| !IsSha256(Document.SnapshotDigest))
		{
			SetError(OutError, TEXT("Authority schema-1 document identity or evidence is invalid."));
			return false;
		}
		FShanmenItemAuthoritySnapshot Canonical;
		FString Error;
		if (!CanonicalizeSnapshot(Document.Authority, Canonical, &Error)
			|| !(Canonical == Document.Authority)
			|| !AuthorityBelongsTo(Document.Authority, Document.OwnerId))
		{
			SetError(OutError, Error.IsEmpty()
				? TEXT("Authority schema-1 snapshot is non-canonical or crosses owners.")
				: Error);
			return false;
		}
		FString ActualDigest;
		if (!FShanmenItemAuthorityStore::ComputeLegacySchema1SnapshotDigest(
				Document.Authority, ActualDigest, &Error)
			|| ActualDigest != Document.SnapshotDigest)
		{
			SetError(OutError, Error.IsEmpty()
				? TEXT("Authority schema-1 SnapshotDigest does not match its payload.")
				: Error);
			return false;
		}
		return true;
	}

	bool SerializeDocument(
		const FShanmenItemAuthorityDocument& Document,
		TArray<uint8>& OutBytes,
		FString* OutError)
	{
		if (!FShanmenItemAuthorityStore::ValidateDocument(Document, OutError))
		{
			return false;
		}
		TSharedPtr<FJsonObject> AuthorityObject;
		if (!SnapshotToObject(Document.Authority, AuthorityObject, OutError))
		{
			return false;
		}

		const TSharedRef<FJsonObject> Migration = MakeShared<FJsonObject>();
		Migration->SetStringField(TEXT("MigrationId"), GuidDigits(Document.Migration.MigrationId));
		Migration->SetStringField(TEXT("OwnerId"), GuidDigits(Document.Migration.OwnerId));
		Migration->SetNumberField(TEXT("SourceProfileSchema"), Document.Migration.SourceProfileSchema);
		Migration->SetNumberField(TEXT("SourceSaveGeneration"), Document.Migration.SourceSaveGeneration);
		Migration->SetNumberField(TEXT("SourceCodeBPersistentRevision"), Document.Migration.SourceCodeBPersistentRevision);
		Migration->SetNumberField(TEXT("SourceCodeBRepositoryRevision"), Document.Migration.SourceCodeBRepositoryRevision);
		Migration->SetNumberField(TEXT("DefinitionCount"), Document.Migration.DefinitionCount);
		Migration->SetNumberField(TEXT("ContainerCount"), Document.Migration.ContainerCount);
		Migration->SetNumberField(TEXT("ItemCount"), Document.Migration.ItemCount);
		Migration->SetStringField(TEXT("SourceFingerprint"), Document.Migration.SourceFingerprint);
		Migration->SetStringField(TEXT("CandidateDigest"), Document.Migration.CandidateDigest);

		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("SchemaVersion"), Document.SchemaVersion);
		Root->SetStringField(TEXT("DocumentId"), GuidDigits(Document.DocumentId));
		Root->SetStringField(TEXT("OwnerId"), GuidDigits(Document.OwnerId));
		Root->SetNumberField(TEXT("SaveGeneration"), Document.SaveGeneration);
		Root->SetStringField(TEXT("CreatedUtc"), Document.CreatedUtc);
		Root->SetStringField(TEXT("LastSavedUtc"), Document.LastSavedUtc);
		Root->SetObjectField(TEXT("Migration"), Migration);
		Root->SetStringField(TEXT("InitialSnapshotDigest"), Document.InitialSnapshotDigest);
		Root->SetStringField(TEXT("SnapshotDigest"), Document.SnapshotDigest);
		Root->SetObjectField(TEXT("Authority"), AuthorityObject.ToSharedRef());
		return JsonToBytes(Root, OutBytes, OutError);
	}

	FReadResult DeserializeDocument(const TArray<uint8>& Bytes)
	{
		FReadResult Result;
		Result.Bytes = Bytes;
		if (Bytes.IsEmpty())
		{
			Result.Diagnostic = TEXT("Authority document is empty.");
			return Result;
		}
		FUTF8ToTCHAR Converted(
			reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
		const FString Json(Converted.Length(), Converted.Get());
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			Result.Diagnostic = TEXT("Authority document JSON could not be parsed.");
			return Result;
		}

		int32 SchemaVersion = INDEX_NONE;
		if (!TryInt32(Root, TEXT("SchemaVersion"), SchemaVersion))
		{
			Result.Diagnostic = TEXT("Authority document SchemaVersion is invalid.");
			return Result;
		}
		if (SchemaVersion > FShanmenItemAuthorityDocument::CurrentSchemaVersion)
		{
			Result.Kind = EReadKind::FutureSchema;
			Result.Diagnostic = TEXT("Authority document uses a future schema.");
			return Result;
		}
		const bool bLegacySchema = SchemaVersion
			== FShanmenItemAuthorityDocument::LegacySchemaVersion;
		if ((!bLegacySchema
				&& SchemaVersion
					!= FShanmenItemAuthorityDocument::CurrentSchemaVersion)
			|| !HasExactFields(Root,
				{
					TEXT("SchemaVersion"), TEXT("DocumentId"), TEXT("OwnerId"),
					TEXT("SaveGeneration"), TEXT("CreatedUtc"), TEXT("LastSavedUtc"),
					TEXT("Migration"), TEXT("InitialSnapshotDigest"),
					TEXT("SnapshotDigest"), TEXT("Authority")
				}))
		{
			Result.Diagnostic = TEXT("Authority document schema or top-level fields are unsupported.");
			return Result;
		}

		const TSharedPtr<FJsonObject>* MigrationObject = nullptr;
		const TSharedPtr<FJsonObject>* AuthorityObject = nullptr;
		FShanmenItemAuthorityDocument Document;
		Document.SchemaVersion = SchemaVersion;
		if (!TryGuid(Root, TEXT("DocumentId"), Document.DocumentId)
			|| !TryGuid(Root, TEXT("OwnerId"), Document.OwnerId)
			|| !TryInt32(Root, TEXT("SaveGeneration"), Document.SaveGeneration)
			|| !Root->TryGetStringField(TEXT("CreatedUtc"), Document.CreatedUtc)
			|| !Root->TryGetStringField(TEXT("LastSavedUtc"), Document.LastSavedUtc)
			|| !Root->TryGetStringField(TEXT("InitialSnapshotDigest"), Document.InitialSnapshotDigest)
			|| !Root->TryGetStringField(TEXT("SnapshotDigest"), Document.SnapshotDigest)
			|| !Root->TryGetObjectField(TEXT("Migration"), MigrationObject)
			|| !MigrationObject || !MigrationObject->IsValid()
			|| !Root->TryGetObjectField(TEXT("Authority"), AuthorityObject)
			|| !AuthorityObject || !AuthorityObject->IsValid())
		{
			Result.Diagnostic = TEXT("Authority document identity or payload fields are invalid.");
			return Result;
		}

		if (!HasExactFields(*MigrationObject,
			{
				TEXT("MigrationId"), TEXT("OwnerId"), TEXT("SourceProfileSchema"),
				TEXT("SourceSaveGeneration"), TEXT("SourceCodeBPersistentRevision"),
				TEXT("SourceCodeBRepositoryRevision"), TEXT("DefinitionCount"),
				TEXT("ContainerCount"), TEXT("ItemCount"),
				TEXT("SourceFingerprint"), TEXT("CandidateDigest")
			})
			|| !TryGuid(*MigrationObject, TEXT("MigrationId"), Document.Migration.MigrationId)
			|| !TryGuid(*MigrationObject, TEXT("OwnerId"), Document.Migration.OwnerId)
			|| !TryInt32(*MigrationObject, TEXT("SourceProfileSchema"), Document.Migration.SourceProfileSchema)
			|| !TryInt32(*MigrationObject, TEXT("SourceSaveGeneration"), Document.Migration.SourceSaveGeneration)
			|| !TryInt32(*MigrationObject, TEXT("SourceCodeBPersistentRevision"), Document.Migration.SourceCodeBPersistentRevision)
			|| !TryInt32(*MigrationObject, TEXT("SourceCodeBRepositoryRevision"), Document.Migration.SourceCodeBRepositoryRevision)
			|| !TryInt32(*MigrationObject, TEXT("DefinitionCount"), Document.Migration.DefinitionCount)
			|| !TryInt32(*MigrationObject, TEXT("ContainerCount"), Document.Migration.ContainerCount)
			|| !TryInt32(*MigrationObject, TEXT("ItemCount"), Document.Migration.ItemCount)
			|| !(*MigrationObject)->TryGetStringField(TEXT("SourceFingerprint"), Document.Migration.SourceFingerprint)
			|| !(*MigrationObject)->TryGetStringField(TEXT("CandidateDigest"), Document.Migration.CandidateDigest))
		{
			Result.Diagnostic = TEXT("Authority migration evidence is invalid.");
			return Result;
		}

		FString SchemaPreparationError;
		if (bLegacySchema
			&& !PrepareSchema1AuthorityForStrictConversion(
				*AuthorityObject, &SchemaPreparationError))
		{
			Result.Diagnostic = SchemaPreparationError;
			return Result;
		}
		FText ConversionFailure;
		if (!FJsonObjectConverter::JsonObjectToUStruct(
				AuthorityObject->ToSharedRef(), &Document.Authority,
				0, 0, true, &ConversionFailure))
		{
			Result.Diagnostic = FString::Printf(
				TEXT("Authority snapshot JSON is invalid: %s"),
				*ConversionFailure.ToString());
			return Result;
		}
		FString ValidationError;
		if (bLegacySchema)
		{
			if (!ValidateLegacySchema1Document(Document, &ValidationError))
			{
				Result.Diagnostic = ValidationError;
				return Result;
			}
			Document.SchemaVersion =
				FShanmenItemAuthorityDocument::CurrentSchemaVersion;
			if (!FShanmenItemAuthorityStore::ComputeSnapshotDigest(
					Document.Authority, Document.SnapshotDigest,
					&ValidationError))
			{
				Result.Diagnostic = ValidationError;
				return Result;
			}
		}
		if (!FShanmenItemAuthorityStore::ValidateDocument(
				Document, &ValidationError))
		{
			Result.Diagnostic = ValidationError;
			return Result;
		}
		Result.Kind = EReadKind::Valid;
		Result.bSchemaUpgraded = bLegacySchema;
		Result.Diagnostic = bLegacySchema
			? TEXT("Authority schema-1 document validated and normalized to schema 2 in memory.")
			: TEXT("Authority document parsed and validated.");
		Result.Document = MoveTemp(Document);
		return Result;
	}

	FReadResult ReadDocumentAtPath(const FString& Path)
	{
		FReadResult Result;
		if (!IFileManager::Get().FileExists(*Path))
		{
			Result.Kind = EReadKind::Missing;
			Result.Diagnostic = TEXT("Authority document does not exist.");
			return Result;
		}
		if (!FFileHelper::LoadFileToArray(Result.Bytes, *Path))
		{
			Result.Kind = EReadKind::ReadFailure;
			Result.Diagnostic = TEXT("Authority document could not be read.");
			return Result;
		}
		return DeserializeDocument(Result.Bytes);
	}

	bool WriteBytesWithFullFlush(
		const FString& Path,
		const TArray<uint8>& Bytes,
		bool bInjectFlushFailure,
		FString& OutError)
	{
		TUniquePtr<IFileHandle> Handle(
			FPlatformFileManager::Get().GetPlatformFile().OpenWrite(
				*Path, false, false));
		if (!Handle || !Handle->Write(Bytes.GetData(), Bytes.Num()))
		{
			OutError = TEXT("Authority temporary write failed.");
			return false;
		}
		if (bInjectFlushFailure)
		{
			Handle.Reset();
			OutError = TEXT("Injected authority temporary flush/close failure.");
			return false;
		}
		if (!Handle->Flush(true))
		{
			Handle.Reset();
			OutError = TEXT("Authority temporary full flush failed.");
			return false;
		}
		Handle.Reset();
		return true;
	}

	FString PreserveCorruptPrimary(
		const FShanmenItemStorageContext& Storage,
		const TArray<uint8>& Bytes,
		FString& OutError)
	{
		if ((!IFileManager::Get().DirectoryExists(*Storage.CorruptDirectory())
				&& !IFileManager::Get().MakeDirectory(*Storage.CorruptDirectory(), true)))
		{
			OutError = TEXT("Authority corrupt-preservation directory could not be created.");
			return FString();
		}
		FString Signature;
		if (!HashBytes(Bytes, Signature))
		{
			OutError = TEXT("Authority corrupt bytes could not be hashed.");
			return FString();
		}
		const FString Path = FPaths::Combine(
			Storage.CorruptDirectory(),
			FString::Printf(TEXT("%s_%s.corrupt.json"),
				*Signature, *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		if (!FFileHelper::SaveArrayToFile(Bytes, *Path))
		{
			OutError = TEXT("Authority corrupt bytes could not be preserved.");
			return FString();
		}
		TArray<uint8> Verified;
		if (!FFileHelper::LoadFileToArray(Verified, *Path) || Verified != Bytes)
		{
			OutError = TEXT("Preserved authority corrupt bytes did not verify.");
			return FString();
		}
		return Path;
	}

	FShanmenItemSaveResult CommitDocument(
		FShanmenItemAuthorityDocument& InOutDocument,
		const FShanmenItemAuthorityDocument& Candidate,
		const FShanmenItemStorageContext& Storage)
	{
		FShanmenItemSaveResult Result;
		Result.PrimaryPath = Storage.PrimaryPath();
		Result.BackupPath = Storage.BackupPath();
		Result.TempPath = Storage.TempPath();
		FString Error;
		if (!FShanmenItemAuthorityStore::ValidateDocument(Candidate, &Error)
			|| Storage.OwnerId != Candidate.OwnerId)
		{
			Result.Status = EShanmenItemSaveStatus::ValidationRejected;
			Result.Diagnostic = Error.IsEmpty()
				? TEXT("Authority storage owner does not match the document.") : Error;
			return Result;
		}
		TArray<uint8> CandidateBytes;
		if (!SerializeDocument(Candidate, CandidateBytes, &Error))
		{
			Result.Status = EShanmenItemSaveStatus::SerializationFailed;
			Result.Diagnostic = Error;
			return Result;
		}
		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::CreateDirectory)
			|| (!IFileManager::Get().DirectoryExists(*Storage.StorageDirectory())
				&& !IFileManager::Get().MakeDirectory(*Storage.StorageDirectory(), true)))
		{
			Result.Status = EShanmenItemSaveStatus::TempWriteFailed;
			Result.Diagnostic = TEXT("Authority storage directory could not be created.");
			return Result;
		}
		if (IFileManager::Get().FileExists(*Result.TempPath)
			&& !IFileManager::Get().Delete(*Result.TempPath, false, true, true))
		{
			Result.Status = EShanmenItemSaveStatus::TempWriteFailed;
			Result.Diagnostic = TEXT("Stale authority temporary file could not be removed.");
			return Result;
		}
		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::WriteTemp))
		{
			Result.Status = EShanmenItemSaveStatus::TempWriteFailed;
			Result.Diagnostic = TEXT("Injected authority temporary write failure.");
			return Result;
		}
		if (!WriteBytesWithFullFlush(
				Result.TempPath, CandidateBytes,
				ShouldFail(Storage, EShanmenItemStoreFailureStage::FlushOrCloseTemp),
				Error))
		{
			Result.Status = ShouldFail(Storage, EShanmenItemStoreFailureStage::FlushOrCloseTemp)
				? EShanmenItemSaveStatus::TempFlushOrCloseFailed
				: EShanmenItemSaveStatus::TempWriteFailed;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.bDiskStateChanged = true;
		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::ReadBackTemp))
		{
			Result.Status = EShanmenItemSaveStatus::TempVerificationFailed;
			Result.Diagnostic = TEXT("Injected authority temporary read-back failure.");
			return Result;
		}
		const FReadResult TempRead = ReadDocumentAtPath(Result.TempPath);
		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::ValidateTemp)
			|| TempRead.Kind != EReadKind::Valid
			|| TempRead.Bytes != CandidateBytes
			|| !(TempRead.Document == Candidate))
		{
			Result.Status = EShanmenItemSaveStatus::TempVerificationFailed;
			Result.Diagnostic = TEXT("Authority temporary document verification failed.");
			return Result;
		}

		const FReadResult Existing = ReadDocumentAtPath(Result.PrimaryPath);
		if (Existing.Kind == EReadKind::Missing)
		{
			if (IFileManager::Get().FileExists(*Result.BackupPath))
			{
				Result.Status = EShanmenItemSaveStatus::BackupPreparationFailed;
				Result.Diagnostic = TEXT("Authority primary is missing while a backup exists; reopen recovery is required.");
				return Result;
			}
		}
		else
		{
			if (Existing.Kind != EReadKind::Valid
				|| !(Existing.Document == InOutDocument))
			{
				Result.Status = EShanmenItemSaveStatus::BackupPreparationFailed;
				Result.Diagnostic = TEXT("Authority primary differs from the caller generation or is invalid.");
				return Result;
			}
			if (ShouldFail(Storage, EShanmenItemStoreFailureStage::PrepareBackup)
				|| IFileManager::Get().Copy(
					*Result.BackupPath, *Result.PrimaryPath, true, true) != COPY_OK)
			{
				Result.Status = EShanmenItemSaveStatus::BackupPreparationFailed;
				Result.Diagnostic = TEXT("Authority backup preparation failed.");
				return Result;
			}
			TArray<uint8> BackupBytes;
			if (!FFileHelper::LoadFileToArray(BackupBytes, *Result.BackupPath)
				|| BackupBytes != Existing.Bytes)
			{
				Result.Status = EShanmenItemSaveStatus::BackupPreparationFailed;
				Result.Diagnostic = TEXT("Authority backup byte verification failed.");
				return Result;
			}
		}

		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::AtomicReplace)
			|| !IFileManager::Get().Move(
				*Result.PrimaryPath, *Result.TempPath, true, false, true, true))
		{
			Result.Status = EShanmenItemSaveStatus::AtomicReplaceFailed;
			Result.Diagnostic = TEXT("Authority same-volume primary replacement failed.");
			return Result;
		}
		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::ReadBackCommittedPrimary))
		{
			Result.Status = EShanmenItemSaveStatus::PostCommitVerificationFailed;
			Result.Diagnostic = TEXT("Injected committed authority read-back failure; reopen is required to resolve the outcome.");
			return Result;
		}
		const FReadResult Committed = ReadDocumentAtPath(Result.PrimaryPath);
		if (Committed.Kind != EReadKind::Valid
			|| Committed.Bytes != CandidateBytes
			|| !(Committed.Document == Candidate))
		{
			Result.Status = EShanmenItemSaveStatus::PostCommitVerificationFailed;
			Result.Diagnostic = TEXT("Committed authority primary verification failed; reopen is required.");
			return Result;
		}

		InOutDocument = Candidate;
		Result.Status = EShanmenItemSaveStatus::Saved;
		Result.Diagnostic = TEXT("Authority document committed and verified.");
		Result.CommittedGeneration = Candidate.SaveGeneration;
		if (ShouldFail(Storage, EShanmenItemStoreFailureStage::CleanupTemp))
		{
			Result.bCleanupSucceeded = false;
			Result.Diagnostic += TEXT(" Injected cleanup warning after atomic replacement.");
		}
		else if (IFileManager::Get().FileExists(*Result.TempPath))
		{
			Result.bCleanupSucceeded =
				IFileManager::Get().Delete(*Result.TempPath, false, true, true);
		}
		return Result;
	}
}

bool FShanmenItemMigrationEvidence::IsValid() const
{
	return MigrationId.IsValid()
		&& OwnerId.IsValid()
		&& SourceProfileSchema > 0
		&& SourceSaveGeneration >= 0
		&& SourceCodeBPersistentRevision > 0
		&& SourceCodeBRepositoryRevision >= 0
		&& DefinitionCount >= 0
		&& ContainerCount >= 0
		&& ItemCount >= 0
		&& !SourceFingerprint.IsEmpty()
		&& !CandidateDigest.IsEmpty();
}

bool FShanmenItemMigrationEvidence::operator==(
	const FShanmenItemMigrationEvidence& Other) const
{
	return MigrationId == Other.MigrationId
		&& OwnerId == Other.OwnerId
		&& SourceProfileSchema == Other.SourceProfileSchema
		&& SourceSaveGeneration == Other.SourceSaveGeneration
		&& SourceCodeBPersistentRevision == Other.SourceCodeBPersistentRevision
		&& SourceCodeBRepositoryRevision == Other.SourceCodeBRepositoryRevision
		&& DefinitionCount == Other.DefinitionCount
		&& ContainerCount == Other.ContainerCount
		&& ItemCount == Other.ItemCount
		&& SourceFingerprint == Other.SourceFingerprint
		&& CandidateDigest == Other.CandidateDigest;
}

bool FShanmenItemAuthorityDocument::operator==(
	const FShanmenItemAuthorityDocument& Other) const
{
	return SchemaVersion == Other.SchemaVersion
		&& DocumentId == Other.DocumentId
		&& OwnerId == Other.OwnerId
		&& SaveGeneration == Other.SaveGeneration
		&& CreatedUtc == Other.CreatedUtc
		&& LastSavedUtc == Other.LastSavedUtc
		&& Migration == Other.Migration
		&& InitialSnapshotDigest == Other.InitialSnapshotDigest
		&& SnapshotDigest == Other.SnapshotDigest
		&& Authority == Other.Authority;
}

FShanmenItemStorageContext FShanmenItemStorageContext::Production(
	const FGuid& OwnerId)
{
	return ForRoot(FPaths::ProjectSavedDir(), OwnerId);
}

FShanmenItemStorageContext FShanmenItemStorageContext::ForRoot(
	const FString& Root, const FGuid& OwnerId)
{
	FShanmenItemStorageContext Result;
	Result.RootDirectory = FPaths::ConvertRelativePathToFull(Root);
	Result.OwnerId = OwnerId;
	return Result;
}

FString FShanmenItemStorageContext::StorageDirectory() const
{
	return FPaths::Combine(RootDirectory, TEXT("ShanmenItems"), TEXT("Authority"));
}

FString FShanmenItemStorageContext::PrimaryPath() const
{
	return FPaths::Combine(StorageDirectory(), GuidDigits(OwnerId) + TEXT(".json"));
}

FString FShanmenItemStorageContext::BackupPath() const
{
	return PrimaryPath() + TEXT(".bak");
}

FString FShanmenItemStorageContext::TempPath() const
{
	return PrimaryPath() + TEXT(".tmp");
}

FString FShanmenItemStorageContext::CorruptDirectory() const
{
	return FPaths::Combine(StorageDirectory(), TEXT("Corrupt"));
}

bool FShanmenItemAuthorityStore::ComputeSnapshotDigest(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	FString& OutDigest,
	FString* OutError)
{
	FShanmenItemAuthoritySnapshot Canonical;
	if (!CanonicalizeSnapshot(Snapshot, Canonical, OutError))
	{
		return false;
	}
	TSharedPtr<FJsonObject> Object;
	if (!SnapshotToObject(Canonical, Object, OutError))
	{
		return false;
	}
	TArray<uint8> Bytes;
	if (!JsonToBytes(Object.ToSharedRef(), Bytes, OutError)
		|| !HashBytes(Bytes, OutDigest))
	{
		SetError(OutError, TEXT("Authority snapshot SHA-256 generation failed."));
		return false;
	}
	return true;
}

bool FShanmenItemAuthorityStore::ComputeLegacySchema1SnapshotDigest(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	FString& OutDigest,
	FString* OutError)
{
	FShanmenItemAuthoritySnapshot Canonical;
	if (!CanonicalizeSnapshot(Snapshot, Canonical, OutError))
	{
		return false;
	}
	TSharedPtr<FJsonObject> Object;
	if (!SnapshotToObject(Canonical, Object, OutError)
		|| !RemoveRewardMetadataForSchema1(Object, OutError))
	{
		return false;
	}
	TArray<uint8> Bytes;
	if (!JsonToBytes(Object.ToSharedRef(), Bytes, OutError)
		|| !HashBytes(Bytes, OutDigest))
	{
		SetError(OutError, TEXT("Authority schema-1 snapshot SHA-256 generation failed."));
		return false;
	}
	return true;
}

bool FShanmenItemAuthorityStore::ValidateDocument(
	const FShanmenItemAuthorityDocument& Document,
	FString* OutError)
{
	FDateTime Created;
	FDateTime Saved;
	if (Document.SchemaVersion != FShanmenItemAuthorityDocument::CurrentSchemaVersion
		|| !Document.DocumentId.IsValid()
		|| !Document.OwnerId.IsValid()
		|| Document.SaveGeneration < 0
		|| !Document.Migration.IsValid()
		|| Document.Migration.OwnerId != Document.OwnerId
		|| Document.DocumentId != ExpectedDocumentId(Document)
		|| !FDateTime::ParseIso8601(*Document.CreatedUtc, Created)
		|| !FDateTime::ParseIso8601(*Document.LastSavedUtc, Saved)
		|| Saved < Created
		|| !IsSha256(Document.InitialSnapshotDigest)
		|| !IsSha256(Document.SnapshotDigest))
	{
		SetError(OutError, TEXT("Authority document identity, generation, timestamp, or migration evidence is invalid."));
		return false;
	}
	FShanmenItemAuthoritySnapshot Canonical;
	FString CanonicalError;
	if (!CanonicalizeSnapshot(Document.Authority, Canonical, &CanonicalError)
		|| !(Canonical == Document.Authority)
		|| !AuthorityBelongsTo(Document.Authority, Document.OwnerId))
	{
		SetError(OutError, CanonicalError.IsEmpty()
			? TEXT("Authority document snapshot is non-canonical or crosses owners.")
			: CanonicalError);
		return false;
	}
	FString ActualDigest;
	if (!ComputeSnapshotDigest(Document.Authority, ActualDigest, &CanonicalError)
		|| ActualDigest != Document.SnapshotDigest)
	{
		SetError(OutError, CanonicalError.IsEmpty()
			? TEXT("Authority document SnapshotDigest does not match its payload.")
			: CanonicalError);
		return false;
	}
	return true;
}

FShanmenItemSaveResult FShanmenItemAuthorityStore::SaveAuthority(
	FShanmenItemAuthorityDocument& InOutDocument,
	const FShanmenItemAuthoritySnapshot& NewAuthority,
	const FShanmenItemStorageContext& Storage) const
{
	FShanmenItemSaveResult Rejected;
	Rejected.PrimaryPath = Storage.PrimaryPath();
	Rejected.BackupPath = Storage.BackupPath();
	Rejected.TempPath = Storage.TempPath();
	FString Error;
	if (!ValidateDocument(InOutDocument, &Error)
		|| !Storage.OwnerId.IsValid()
		|| Storage.OwnerId != InOutDocument.OwnerId
		|| Storage.RootDirectory.IsEmpty())
	{
		Rejected.Status = EShanmenItemSaveStatus::ValidationRejected;
		Rejected.Diagnostic = Error.IsEmpty()
			? TEXT("Authority save storage context is invalid.") : Error;
		return Rejected;
	}
	FShanmenItemAuthoritySnapshot Canonical;
	if (!CanonicalizeSnapshot(NewAuthority, Canonical, &Error)
		|| !AuthorityBelongsTo(Canonical, InOutDocument.OwnerId))
	{
		Rejected.Status = EShanmenItemSaveStatus::ValidationRejected;
		Rejected.Diagnostic = Error.IsEmpty()
			? TEXT("New authority snapshot crosses owners.") : Error;
		return Rejected;
	}
	if (Canonical == InOutDocument.Authority)
	{
		const FReadResult Existing = ReadDocumentAtPath(Storage.PrimaryPath());
		if (Existing.Kind == EReadKind::Valid
			&& Existing.Document == InOutDocument)
		{
			if (!Existing.bSchemaUpgraded)
			{
				Rejected.Status = EShanmenItemSaveStatus::Saved;
				Rejected.Diagnostic = TEXT("Authority snapshot and durable primary are unchanged; no disk write was required.");
				Rejected.CommittedGeneration = InOutDocument.SaveGeneration;
				return Rejected;
			}
			// A schema-1 primary is logically equal after read-time conversion,
			// but still requires one normal atomic commit to publish schema 2.
		}
		else
		{
			Rejected.Status = EShanmenItemSaveStatus::BackupPreparationFailed;
			Rejected.Diagnostic = TEXT("Unchanged authority cannot be acknowledged because the durable primary differs or is invalid; reopen is required.");
			return Rejected;
		}
	}
	if (InOutDocument.SaveGeneration == MAX_int32)
	{
		Rejected.Status = EShanmenItemSaveStatus::ValidationRejected;
		Rejected.Diagnostic = TEXT("Authority SaveGeneration cannot be incremented.");
		return Rejected;
	}
	FShanmenItemAuthorityDocument Candidate = InOutDocument;
	Candidate.Authority = MoveTemp(Canonical);
	Candidate.SaveGeneration++;
	Candidate.LastSavedUtc = FDateTime::UtcNow().ToIso8601();
	if (!ComputeSnapshotDigest(Candidate.Authority, Candidate.SnapshotDigest, &Error))
	{
		Rejected.Status = EShanmenItemSaveStatus::SerializationFailed;
		Rejected.Diagnostic = Error;
		return Rejected;
	}
	return CommitDocument(InOutDocument, Candidate, Storage);
}

FShanmenItemLoadResult FShanmenItemAuthorityStore::LoadExisting(
	const FShanmenItemStorageContext& Storage) const
{
	FShanmenItemLoadResult Result;
	Result.PrimaryPath = Storage.PrimaryPath();
	Result.BackupPath = Storage.BackupPath();
	Result.TempPath = Storage.TempPath();
	if (!Storage.OwnerId.IsValid() || Storage.RootDirectory.IsEmpty())
	{
		Result.Status = EShanmenItemLoadStatus::InvalidDocument;
		Result.Diagnostic = TEXT("Authority load storage context is invalid.");
		return Result;
	}
	const FReadResult Primary = ReadDocumentAtPath(Result.PrimaryPath);
	if (Primary.Kind == EReadKind::Valid)
	{
		if (Primary.Document.OwnerId != Storage.OwnerId)
		{
			Result.Status = EShanmenItemLoadStatus::InvalidDocument;
			Result.Diagnostic = TEXT("Authority primary owner differs from its storage path.");
			return Result;
		}
		Result.Status = EShanmenItemLoadStatus::LoadedPrimary;
		Result.bSchemaUpgraded = Primary.bSchemaUpgraded;
		Result.Diagnostic = Primary.bSchemaUpgraded
			? TEXT("Authority schema-1 primary loaded and normalized to schema 2 in memory without a write.")
			: TEXT("Authority primary loaded without a write.");
		Result.Document = Primary.Document;
		return Result;
	}
	if (Primary.Kind == EReadKind::FutureSchema)
	{
		Result.Status = EShanmenItemLoadStatus::FutureSchemaRejected;
		Result.Diagnostic = Primary.Diagnostic;
		return Result;
	}
	if (Primary.Kind == EReadKind::ReadFailure)
	{
		Result.Status = EShanmenItemLoadStatus::ReadFailed;
		Result.Diagnostic = Primary.Diagnostic;
		return Result;
	}

	const FReadResult Backup = ReadDocumentAtPath(Result.BackupPath);
	if (Backup.Kind != EReadKind::Valid
		|| Backup.Document.OwnerId != Storage.OwnerId)
	{
		Result.Status = Primary.Kind == EReadKind::Missing
			? EShanmenItemLoadStatus::Missing
			: EShanmenItemLoadStatus::CorruptPrimaryNoValidBackup;
		Result.Diagnostic = Primary.Kind == EReadKind::Missing
			? TEXT("Authority primary and valid backup are absent.")
			: TEXT("Authority primary is invalid and no valid backup exists; no reset was performed.");
		return Result;
	}

	FString Error;
	if (Primary.Kind != EReadKind::Missing)
	{
		Result.QuarantinedPath = PreserveCorruptPrimary(Storage, Primary.Bytes, Error);
		if (Result.QuarantinedPath.IsEmpty())
		{
			Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.bDiskStateChanged = true;
	}
	if (!IFileManager::Get().DirectoryExists(*Storage.StorageDirectory())
		&& !IFileManager::Get().MakeDirectory(*Storage.StorageDirectory(), true))
	{
		Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
		Result.Diagnostic = TEXT("Authority recovery directory could not be created.");
		return Result;
	}
	if (IFileManager::Get().FileExists(*Result.TempPath)
		&& !IFileManager::Get().Delete(*Result.TempPath, false, true, true))
	{
		Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
		Result.Diagnostic = TEXT("Stale authority recovery temporary file could not be removed.");
		return Result;
	}
	if (ShouldFail(Storage, EShanmenItemStoreFailureStage::WriteTemp)
		|| !WriteBytesWithFullFlush(
			Result.TempPath, Backup.Bytes,
			ShouldFail(Storage, EShanmenItemStoreFailureStage::FlushOrCloseTemp),
			Error))
	{
		Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
		Result.Diagnostic = Error.IsEmpty()
			? TEXT("Injected authority recovery temporary failure.") : Error;
		return Result;
	}
	Result.bDiskStateChanged = true;
	const FReadResult Temp = ShouldFail(Storage, EShanmenItemStoreFailureStage::ReadBackTemp)
		? FReadResult() : ReadDocumentAtPath(Result.TempPath);
	if (ShouldFail(Storage, EShanmenItemStoreFailureStage::ValidateTemp)
		|| Temp.Kind != EReadKind::Valid
		|| Temp.Bytes != Backup.Bytes
		|| !(Temp.Document == Backup.Document))
	{
		Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
		Result.Diagnostic = TEXT("Authority recovery temporary verification failed.");
		return Result;
	}
	if (ShouldFail(Storage, EShanmenItemStoreFailureStage::AtomicReplace)
		|| !IFileManager::Get().Move(
			*Result.PrimaryPath, *Result.TempPath, true, false, true, true))
	{
		Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
		Result.Diagnostic = TEXT("Authority recovery primary replacement failed.");
		return Result;
	}
	const FReadResult Recovered = ShouldFail(
		Storage, EShanmenItemStoreFailureStage::ReadBackCommittedPrimary)
		? FReadResult() : ReadDocumentAtPath(Result.PrimaryPath);
	if (Recovered.Kind != EReadKind::Valid
		|| Recovered.Bytes != Backup.Bytes
		|| !(Recovered.Document == Backup.Document))
	{
		Result.Status = EShanmenItemLoadStatus::WriteRecoveryFailed;
		Result.Diagnostic = TEXT("Recovered authority primary verification failed; reopen is required.");
		return Result;
	}
	Result.Status = Primary.Kind == EReadKind::Missing
		? EShanmenItemLoadStatus::PrimaryMissingBackupRecovered
		: EShanmenItemLoadStatus::RecoveredFromBackup;
	Result.Diagnostic = TEXT("Verified authority backup restored without changing its generation.");
	Result.bSchemaUpgraded = Recovered.bSchemaUpgraded;
	Result.Document = Recovered.Document;
	return Result;
}

FShanmenItemOpenResult FShanmenItemAuthorityStore::OpenOrCreateFromMigration(
	const FShanmenItemAuthoritySnapshot& MigrationCandidate,
	const FShanmenItemMigrationEvidence& Migration,
	const FShanmenItemStorageContext& Storage) const
{
	FShanmenItemOpenResult Result;
	FShanmenItemAuthoritySnapshot Canonical;
	FString Error;
	FString InitialDigest;
	FString LegacyInitialDigest;
	if (!Migration.IsValid()
		|| !Storage.OwnerId.IsValid()
		|| Storage.OwnerId != Migration.OwnerId
		|| Storage.RootDirectory.IsEmpty()
		|| !CanonicalizeSnapshot(MigrationCandidate, Canonical, &Error)
		|| !AuthorityBelongsTo(Canonical, Migration.OwnerId)
		|| Canonical.Definitions.Num() != Migration.DefinitionCount
		|| Canonical.Containers.Num() != Migration.ContainerCount
		|| Canonical.Items.Num() != Migration.ItemCount
		|| !ComputeSnapshotDigest(Canonical, InitialDigest, &Error)
		|| !ComputeLegacySchema1SnapshotDigest(
			Canonical, LegacyInitialDigest, &Error))
	{
		Result.Status = EShanmenItemOpenStatus::InvalidRequest;
		Result.Diagnostic = Error.IsEmpty()
			? TEXT("Migration evidence, counts, owner, or candidate is invalid.") : Error;
		return Result;
	}

	const bool bAnyDurableFile =
		IFileManager::Get().FileExists(*Storage.PrimaryPath())
		|| IFileManager::Get().FileExists(*Storage.BackupPath());
	if (bAnyDurableFile)
	{
		const FShanmenItemLoadResult Loaded = LoadExisting(Storage);
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		if (!Loaded.IsSuccess())
		{
			Result.Status = EShanmenItemOpenStatus::PersistenceFailure;
			Result.Diagnostic = Loaded.Diagnostic;
			return Result;
		}
		const bool bCurrentInitial =
			Loaded.Document.InitialSnapshotDigest == InitialDigest;
		const bool bLegacyInitial =
			Loaded.Document.InitialSnapshotDigest == LegacyInitialDigest;
		const bool bMigrationMatches =
			Loaded.Document.Migration == Migration
			|| (bLegacyInitial
				&& SameMigrationSource(Loaded.Document.Migration, Migration));
		if (!bMigrationMatches || (!bCurrentInitial && !bLegacyInitial))
		{
			Result.Status = EShanmenItemOpenStatus::MigrationConflict;
			Result.Diagnostic = TEXT("An authority document already exists for different migration evidence.");
			return Result;
		}

		FShanmenItemAuthorityDocument OpenedDocument = Loaded.Document;
		FShanmenItemAuthoritySnapshot UpgradedAuthority =
			OpenedDocument.Authority;
		bool bMetadataEnriched = false;
		if (bLegacyInitial)
		{
			TMap<FGuid, const FShanmenItemInstance*> InitialItems;
			for (const FShanmenItemInstance& Item : Canonical.Items)
			{
				InitialItems.Add(Item.ItemInstanceId, &Item);
			}
			for (FShanmenItemInstance& Item : UpgradedAuthority.Items)
			{
				const FShanmenItemInstance* Initial =
					InitialItems.FindRef(Item.ItemInstanceId);
				if (!Initial)
				{
					continue;
				}
				if (Initial->DefinitionId != Item.DefinitionId
					|| (!Item.RewardMetadata.IsEmpty()
						&& !(Item.RewardMetadata == Initial->RewardMetadata)
						&& !Initial->RewardMetadata.IsEmpty()))
				{
					Result.Status = EShanmenItemOpenStatus::MigrationConflict;
					Result.Diagnostic = TEXT("Legacy authority identity conflicts with current migration reward metadata.");
					return Result;
				}
				if (Item.RewardMetadata.IsEmpty()
					&& !Initial->RewardMetadata.IsEmpty())
				{
					Item.RewardMetadata = Initial->RewardMetadata;
					bMetadataEnriched = true;
				}
			}
		}
		if (Loaded.bSchemaUpgraded || bMetadataEnriched)
		{
			const FShanmenItemSaveResult Upgraded = SaveAuthority(
				OpenedDocument, UpgradedAuthority, Storage);
			Result.bDiskStateChanged =
				Result.bDiskStateChanged || Upgraded.bDiskStateChanged;
			if (!Upgraded.IsSuccess())
			{
				Result.Status = EShanmenItemOpenStatus::PersistenceFailure;
				Result.Diagnostic = Upgraded.Diagnostic;
				return Result;
			}
		}
		Result.Status = Loaded.Status == EShanmenItemLoadStatus::LoadedPrimary
			? EShanmenItemOpenStatus::OpenedExisting
			: EShanmenItemOpenStatus::RecoveredExisting;
		Result.Diagnostic = Loaded.bSchemaUpgraded || bMetadataEnriched
			? TEXT("Existing authority matched legacy migration evidence; schema 2 and immutable reward metadata were atomically published.")
			: TEXT("Existing authority document matches the migration and was reopened without reimporting legacy data.");
		Result.Document = MoveTemp(OpenedDocument);
		return Result;
	}

	FShanmenItemAuthorityDocument Document;
	Document.SchemaVersion = FShanmenItemAuthorityDocument::CurrentSchemaVersion;
	Document.OwnerId = Migration.OwnerId;
	Document.Migration = Migration;
	Document.DocumentId = ExpectedDocumentId(Document);
	Document.SaveGeneration = 0;
	Document.CreatedUtc = FDateTime::UtcNow().ToIso8601();
	Document.LastSavedUtc = Document.CreatedUtc;
	Document.InitialSnapshotDigest = InitialDigest;
	Document.SnapshotDigest = InitialDigest;
	Document.Authority = MoveTemp(Canonical);
	if (!ValidateDocument(Document, &Error))
	{
		Result.Status = EShanmenItemOpenStatus::InvalidRequest;
		Result.Diagnostic = Error;
		return Result;
	}
	FShanmenItemAuthorityDocument Candidate = Document;
	Candidate.SaveGeneration = 1;
	Candidate.LastSavedUtc = FDateTime::UtcNow().ToIso8601();
	const FShanmenItemSaveResult Saved = CommitDocument(Document, Candidate, Storage);
	Result.bDiskStateChanged = Saved.bDiskStateChanged;
	if (!Saved.IsSuccess())
	{
		Result.Status = EShanmenItemOpenStatus::PersistenceFailure;
		Result.Diagnostic = Saved.Diagnostic;
		return Result;
	}
	Result.Status = EShanmenItemOpenStatus::CreatedFromMigration;
	Result.Diagnostic = TEXT("Migration candidate atomically published as the new authority document.");
	Result.Document = MoveTemp(Document);
	return Result;
}
