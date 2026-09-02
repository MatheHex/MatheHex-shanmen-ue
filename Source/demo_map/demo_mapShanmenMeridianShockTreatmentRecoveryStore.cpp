#include "demo_mapShanmenMeridianShockTreatmentRecoveryStore.h"

#include "ShanmenDeterministicId.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	enum class Edemo_mapRecoveryReadKind : uint8
	{
		Missing,
		Valid,
		FutureSchema,
		Invalid,
		ReadFailure
	};

	struct Fdemo_mapRecoveryReadResult
	{
		Edemo_mapRecoveryReadKind Kind = Edemo_mapRecoveryReadKind::Invalid;
		int32 SourceSchemaVersion = INDEX_NONE;
		FString Diagnostic;
		TArray<uint8> Bytes;
		Fdemo_mapShanmenTreatmentRecoveryDocument Document;
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

	bool TryGuid(
		const TSharedPtr<FJsonObject>& Object,
		const TCHAR* Field,
		FGuid& OutGuid)
	{
		FString Text;
		return Object.IsValid()
			&& Object->TryGetStringField(Field, Text)
			&& Text.Len() == 32
			&& FGuid::ParseExact(Text, EGuidFormats::Digits, OutGuid)
			&& OutGuid.IsValid();
	}

	bool TryInt32(
		const TSharedPtr<FJsonObject>& Object,
		const TCHAR* Field,
		int32& OutValue)
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
		OutValue = static_cast<int32>(Number);
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

	FGuid ExpectedDocumentId(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.MeridianShock.RecoveryStore.r1"),
			{ GuidDigits(Document.OwnerId), GuidDigits(Document.RunId) });
	}

	FGuid ExpectedProofSetIdForSchema(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document,
		const int32 SchemaVersion)
	{
		TArray<FString> Parts;
		Parts.Reserve(3 + Document.Proofs.Num());
		Parts.Add(FString::FromInt(SchemaVersion));
		Parts.Add(GuidDigits(Document.OwnerId));
		Parts.Add(GuidDigits(Document.RunId));
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
			Document.Proofs)
		{
			FString Encoded;
			if (!Proof.TryEncode(Encoded))
			{
				return FGuid();
			}
			Parts.Add(MoveTemp(Encoded));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.MeridianShock.ProofSet.r1"),
			Parts);
	}

	FGuid ExpectedProofSetId(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document)
	{
		return ExpectedProofSetIdForSchema(Document, Document.SchemaVersion);
	}

	FGuid ExpectedIntentSetId(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document)
	{
		TArray<FString> Parts;
		Parts.Reserve(3 + Document.Intents.Num());
		Parts.Add(FString::FromInt(Document.SchemaVersion));
		Parts.Add(GuidDigits(Document.OwnerId));
		Parts.Add(GuidDigits(Document.RunId));
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent :
			Document.Intents)
		{
			FString Encoded;
			if (!Intent.TryEncode(Encoded))
			{
				return FGuid();
			}
			Parts.Add(MoveTemp(Encoded));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.MeridianShock.IntentSet.r1"),
			Parts);
	}

	void CanonicalizeIntents(
		TArray<Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent>& Intents)
	{
		Intents.Sort([](
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Left,
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Right)
		{
			return GuidDigits(Left.GetTreatmentId())
				< GuidDigits(Right.GetTreatmentId());
		});
	}

	void CanonicalizeProofs(
		TArray<Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof>& Proofs)
	{
		Proofs.Sort([](
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Left,
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Right)
		{
			return GuidDigits(Left.GetTreatmentId())
				< GuidDigits(Right.GetTreatmentId());
		});
	}

	void RefreshSetIds(Fdemo_mapShanmenTreatmentRecoveryDocument& Document)
	{
		Document.IntentSetId = ExpectedIntentSetId(Document);
		Document.ProofSetId = ExpectedProofSetId(Document);
	}

	void InitializeDocument(
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage,
		Fdemo_mapShanmenTreatmentRecoveryDocument& OutDocument)
	{
		OutDocument = Fdemo_mapShanmenTreatmentRecoveryDocument();
		OutDocument.SchemaVersion =
			Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion;
		OutDocument.OwnerId = Storage.OwnerId;
		OutDocument.RunId = Storage.RunId;
		OutDocument.DocumentId = ExpectedDocumentId(OutDocument);
		OutDocument.CreatedUtc = FDateTime::UtcNow().ToIso8601();
		OutDocument.LastSavedUtc = OutDocument.CreatedUtc;
		RefreshSetIds(OutDocument);
	}

	bool TryAdvanceDocument(
		Fdemo_mapShanmenTreatmentRecoveryDocument& InOutDocument)
	{
		if (InOutDocument.SaveGeneration < 0
			|| InOutDocument.SaveGeneration == MAX_int32)
		{
			return false;
		}
		InOutDocument.SaveGeneration++;
		InOutDocument.LastSavedUtc = FDateTime::UtcNow().ToIso8601();
		RefreshSetIds(InOutDocument);
		return true;
	}

	bool IntentMatchesProof(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent,
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof)
	{
		return Intent.IsValid()
			&& Proof.IsValid()
			&& Intent.GetTreatmentId() == Proof.GetTreatmentId()
			&& Intent.GetRunId() == Proof.GetRunId()
			&& Intent.GetTargetEntityId() == Proof.GetTargetEntityId()
			&& Intent.GetTimelineId() == Proof.GetTimelineId()
			&& Intent.GetItemInstanceId() == Proof.GetItemInstanceId()
			&& Intent.GetItemDefinitionId() == Proof.GetItemDefinitionId()
			&& Intent.GetConditionDefinitionId()
				== Proof.GetConditionDefinitionId()
			&& Intent.GetTreatmentTick() == Proof.GetTreatedAtTick()
			&& Intent.GetExpectedConditionRevision()
				== Proof.GetConditionRevisionBefore();
	}

	bool ValidateDocumentForSchema(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document,
		const int32 ExpectedSchemaVersion,
		FString* OutError)
	{
		const bool bPreviousSchema = ExpectedSchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::
				PreviousSchemaVersion;
		const bool bCurrentSchema = ExpectedSchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::
				CurrentSchemaVersion;
		FDateTime Created;
		FDateTime Saved;
		if ((!bPreviousSchema && !bCurrentSchema)
			|| Document.SchemaVersion != ExpectedSchemaVersion
			|| !Document.DocumentId.IsValid()
			|| !Document.OwnerId.IsValid()
			|| !Document.RunId.IsValid()
			|| Document.SaveGeneration <= 0
			|| Document.Intents.Num()
				> Fdemo_mapShanmenTreatmentRecoveryDocument::
					MaximumIntentCount
			|| Document.Proofs.Num()
				> Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumProofCount
			|| Document.Intents.Num() + Document.Proofs.Num()
				> Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumEntryCount
			|| Document.DocumentId != ExpectedDocumentId(Document)
			|| !Document.ProofSetId.IsValid()
			|| !FDateTime::ParseIso8601(*Document.CreatedUtc, Created)
			|| !FDateTime::ParseIso8601(*Document.LastSavedUtc, Saved)
			|| Saved < Created
			|| (bPreviousSchema
				&& (!Document.Intents.IsEmpty()
					|| Document.IntentSetId.IsValid()))
			|| (bCurrentSchema && !Document.IntentSetId.IsValid()))
		{
			SetError(
				OutError,
				TEXT("Treatment recovery document identity, generation, timestamp or bounds are invalid."));
			return false;
		}

		FString PreviousTreatmentId;
		TSet<FGuid> IntentIds;
		TSet<FGuid> TreatmentIds;
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent :
			Document.Intents)
		{
			const FString TreatmentId = GuidDigits(Intent.GetTreatmentId());
			if (!Intent.IsValid()
				|| Intent.GetRunId() != Document.RunId
				|| (!PreviousTreatmentId.IsEmpty()
					&& TreatmentId <= PreviousTreatmentId)
				|| IntentIds.Contains(Intent.GetIntentId())
				|| TreatmentIds.Contains(Intent.GetTreatmentId()))
			{
				SetError(
					OutError,
					TEXT("Treatment recovery intents are invalid, cross-Run, duplicated or non-canonical."));
				return false;
			}
			PreviousTreatmentId = TreatmentId;
			IntentIds.Add(Intent.GetIntentId());
			TreatmentIds.Add(Intent.GetTreatmentId());
		}

		PreviousTreatmentId.Reset();
		TSet<FGuid> ProofIds;
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
			Document.Proofs)
		{
			const FString TreatmentId = GuidDigits(Proof.GetTreatmentId());
			if (!Proof.IsValid()
				|| Proof.GetRunId() != Document.RunId
				|| (!PreviousTreatmentId.IsEmpty()
					&& TreatmentId <= PreviousTreatmentId)
				|| ProofIds.Contains(Proof.GetProofId())
				|| TreatmentIds.Contains(Proof.GetTreatmentId()))
			{
				SetError(
					OutError,
					TEXT("Treatment recovery proofs are invalid, cross-Run, duplicated, overlap an intent or are non-canonical."));
				return false;
			}
			PreviousTreatmentId = TreatmentId;
			ProofIds.Add(Proof.GetProofId());
			TreatmentIds.Add(Proof.GetTreatmentId());
		}

		if (Document.ProofSetId
			!= ExpectedProofSetIdForSchema(Document, ExpectedSchemaVersion)
			|| (bCurrentSchema
				&& Document.IntentSetId != ExpectedIntentSetId(Document)))
		{
			SetError(
				OutError,
				TEXT("Treatment recovery set identity does not match its canonical payload."));
			return false;
		}
		return true;
	}

	bool JsonToBytes(
		const TSharedRef<FJsonObject>& Object,
		TArray<uint8>& OutBytes,
		FString* OutError)
	{
		FString Payload;
		const TSharedRef<
			TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<
				TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
		if (!FJsonSerializer::Serialize(Object, Writer))
		{
			SetError(OutError, TEXT("Treatment recovery JSON serialization failed."));
			return false;
		}
		FTCHARToUTF8 Utf8(*Payload);
		OutBytes.Reset();
		OutBytes.Append(
			reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		return !OutBytes.IsEmpty();
	}

	bool SerializeDocument(
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document,
		TArray<uint8>& OutBytes,
		FString* OutError)
	{
		if (!Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::
			ValidateDocument(Document, OutError))
		{
			return false;
		}
		TArray<TSharedPtr<FJsonValue>> IntentValues;
		IntentValues.Reserve(Document.Intents.Num());
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent :
			Document.Intents)
		{
			FString Encoded;
			if (!Intent.TryEncode(Encoded))
			{
				SetError(OutError, TEXT("Treatment intent encoding failed."));
				return false;
			}
			IntentValues.Add(MakeShared<FJsonValueString>(MoveTemp(Encoded)));
		}
		TArray<TSharedPtr<FJsonValue>> ProofValues;
		ProofValues.Reserve(Document.Proofs.Num());
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
			Document.Proofs)
		{
			FString Encoded;
			if (!Proof.TryEncode(Encoded))
			{
				SetError(OutError, TEXT("Treatment proof encoding failed."));
				return false;
			}
			ProofValues.Add(MakeShared<FJsonValueString>(MoveTemp(Encoded)));
		}

		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("SchemaVersion"), Document.SchemaVersion);
		Root->SetStringField(TEXT("DocumentId"), GuidDigits(Document.DocumentId));
		Root->SetStringField(TEXT("OwnerId"), GuidDigits(Document.OwnerId));
		Root->SetStringField(TEXT("RunId"), GuidDigits(Document.RunId));
		Root->SetNumberField(TEXT("SaveGeneration"), Document.SaveGeneration);
		Root->SetStringField(TEXT("CreatedUtc"), Document.CreatedUtc);
		Root->SetStringField(TEXT("LastSavedUtc"), Document.LastSavedUtc);
		Root->SetStringField(TEXT("IntentSetId"), GuidDigits(Document.IntentSetId));
		Root->SetArrayField(TEXT("Intents"), MoveTemp(IntentValues));
		Root->SetStringField(TEXT("ProofSetId"), GuidDigits(Document.ProofSetId));
		Root->SetArrayField(TEXT("Proofs"), MoveTemp(ProofValues));
		return JsonToBytes(Root, OutBytes, OutError);
	}

	Fdemo_mapRecoveryReadResult DeserializeDocument(
		const TArray<uint8>& Bytes)
	{
		Fdemo_mapRecoveryReadResult Result;
		Result.Bytes = Bytes;
		if (Bytes.IsEmpty())
		{
			Result.Diagnostic = TEXT("Treatment recovery document is empty.");
			return Result;
		}
		FUTF8ToTCHAR Converted(
			reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
		const FString Json(Converted.Length(), Converted.Get());
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader =
			TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			Result.Diagnostic =
				TEXT("Treatment recovery document JSON could not be parsed.");
			return Result;
		}

		int32 SchemaVersion = INDEX_NONE;
		if (!TryInt32(Root, TEXT("SchemaVersion"), SchemaVersion))
		{
			Result.Diagnostic =
				TEXT("Treatment recovery SchemaVersion is invalid.");
			return Result;
		}
		Result.SourceSchemaVersion = SchemaVersion;
		if (SchemaVersion
			> Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion)
		{
			Result.Kind = Edemo_mapRecoveryReadKind::FutureSchema;
			Result.Diagnostic =
				TEXT("Treatment recovery document uses a future schema.");
			return Result;
		}
		const bool bPreviousSchema = SchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::
				PreviousSchemaVersion;
		const bool bCurrentSchema = SchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion;
		const bool bExactFields = bPreviousSchema
			? HasExactFields(Root,
				{
					TEXT("SchemaVersion"), TEXT("DocumentId"),
					TEXT("OwnerId"), TEXT("RunId"),
					TEXT("SaveGeneration"), TEXT("CreatedUtc"),
					TEXT("LastSavedUtc"), TEXT("ProofSetId"),
					TEXT("Proofs")
				})
			: bCurrentSchema && HasExactFields(Root,
				{
					TEXT("SchemaVersion"), TEXT("DocumentId"),
					TEXT("OwnerId"), TEXT("RunId"),
					TEXT("SaveGeneration"), TEXT("CreatedUtc"),
					TEXT("LastSavedUtc"), TEXT("IntentSetId"),
					TEXT("Intents"), TEXT("ProofSetId"),
					TEXT("Proofs")
				});
		if (!bExactFields)
		{
			Result.Diagnostic =
				TEXT("Treatment recovery schema or fields are unsupported.");
			return Result;
		}

		const TArray<TSharedPtr<FJsonValue>>* IntentValues = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* ProofValues = nullptr;
		Fdemo_mapShanmenTreatmentRecoveryDocument Document;
		Document.SchemaVersion = SchemaVersion;
		if (!TryGuid(Root, TEXT("DocumentId"), Document.DocumentId)
			|| !TryGuid(Root, TEXT("OwnerId"), Document.OwnerId)
			|| !TryGuid(Root, TEXT("RunId"), Document.RunId)
			|| !TryInt32(
				Root, TEXT("SaveGeneration"), Document.SaveGeneration)
			|| !Root->TryGetStringField(TEXT("CreatedUtc"), Document.CreatedUtc)
			|| !Root->TryGetStringField(
				TEXT("LastSavedUtc"), Document.LastSavedUtc)
			|| (bCurrentSchema
				&& (!TryGuid(Root, TEXT("IntentSetId"), Document.IntentSetId)
					|| !Root->TryGetArrayField(TEXT("Intents"), IntentValues)
					|| !IntentValues
					|| IntentValues->Num()
						> Fdemo_mapShanmenTreatmentRecoveryDocument::
							MaximumIntentCount))
			|| !TryGuid(Root, TEXT("ProofSetId"), Document.ProofSetId)
			|| !Root->TryGetArrayField(TEXT("Proofs"), ProofValues)
			|| !ProofValues
			|| ProofValues->Num()
				> Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumProofCount)
		{
			Result.Diagnostic =
				TEXT("Treatment recovery identity or payload fields are invalid.");
			return Result;
		}

		if (IntentValues)
		{
			Document.Intents.Reserve(IntentValues->Num());
			for (const TSharedPtr<FJsonValue>& Value : *IntentValues)
			{
				FString Encoded;
				Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Intent;
				if (!Value.IsValid()
					|| !Value->TryGetString(Encoded)
					|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::
						TryDecode(Encoded, Intent))
				{
					Result.Diagnostic =
						TEXT("Treatment recovery intent payload is invalid.");
					return Result;
				}
				Document.Intents.Add(MoveTemp(Intent));
			}
		}

		Document.Proofs.Reserve(ProofValues->Num());
		for (const TSharedPtr<FJsonValue>& Value : *ProofValues)
		{
			FString Encoded;
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof Proof;
			if (!Value.IsValid()
				|| !Value->TryGetString(Encoded)
				|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::
					TryDecode(Encoded, Proof))
			{
				Result.Diagnostic =
					TEXT("Treatment recovery proof payload is invalid.");
				return Result;
			}
			Document.Proofs.Add(MoveTemp(Proof));
		}
		FString Error;
		if (!ValidateDocumentForSchema(Document, SchemaVersion, &Error))
		{
			Result.Diagnostic = Error;
			return Result;
		}
		if (bPreviousSchema)
		{
			Document.SchemaVersion =
				Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion;
			RefreshSetIds(Document);
			if (!Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::
				ValidateDocument(Document, &Error))
			{
				Result.Diagnostic = Error;
				return Result;
			}
		}
		Result.Kind = Edemo_mapRecoveryReadKind::Valid;
		Result.Diagnostic = bPreviousSchema
			? TEXT("Previous treatment recovery schema parsed and projected for migration.")
			: TEXT("Treatment recovery document parsed and validated.");
		Result.Document = MoveTemp(Document);
		return Result;
	}

	Fdemo_mapRecoveryReadResult ReadDocumentAtPath(const FString& Path)
	{
		Fdemo_mapRecoveryReadResult Result;
		if (!IFileManager::Get().FileExists(*Path))
		{
			Result.Kind = Edemo_mapRecoveryReadKind::Missing;
			Result.Diagnostic =
				TEXT("Treatment recovery document does not exist.");
			return Result;
		}
		if (!FFileHelper::LoadFileToArray(Result.Bytes, *Path))
		{
			Result.Kind = Edemo_mapRecoveryReadKind::ReadFailure;
			Result.Diagnostic =
				TEXT("Treatment recovery document could not be read.");
			return Result;
		}
		return DeserializeDocument(Result.Bytes);
	}

	bool ShouldFail(
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage,
		const Edemo_mapShanmenTreatmentRecoveryStoreFailureStage Stage)
	{
#if WITH_DEV_AUTOMATION_TESTS
		return Storage.InjectedFailure == Stage;
#else
		return false;
#endif
	}

	bool WriteBytesWithFullFlush(
		const FString& Path,
		const TArray<uint8>& Bytes,
		const bool bInjectFlushFailure,
		FString& OutError)
	{
		TUniquePtr<IFileHandle> Handle(
			FPlatformFileManager::Get().GetPlatformFile().OpenWrite(
				*Path, false, false));
		if (!Handle || !Handle->Write(Bytes.GetData(), Bytes.Num()))
		{
			OutError = TEXT("Treatment recovery temporary write failed.");
			return false;
		}
		if (bInjectFlushFailure)
		{
			Handle.Reset();
			OutError =
				TEXT("Injected treatment recovery temporary flush failure.");
			return false;
		}
		if (!Handle->Flush(true))
		{
			Handle.Reset();
			OutError =
				TEXT("Treatment recovery temporary full flush failed.");
			return false;
		}
		Handle.Reset();
		return true;
	}

	FString ByteSignature(const TArray<uint8>& Bytes)
	{
		uint8 Digest[FSHA1::DigestSize]{};
		FSHA1::HashBuffer(
			Bytes.IsEmpty() ? nullptr : Bytes.GetData(),
			static_cast<uint64>(Bytes.Num()),
			Digest);
		FString Result;
		Result.Reserve(FSHA1::DigestSize * 2);
		for (const uint8 Byte : Digest)
		{
			Result += FString::Printf(TEXT("%02X"), Byte);
		}
		return Result;
	}

	FString PreserveCorruptPrimary(
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage,
		const TArray<uint8>& Bytes,
		FString& OutError)
	{
		if ((!IFileManager::Get().DirectoryExists(*Storage.CorruptDirectory())
				&& !IFileManager::Get().MakeDirectory(
					*Storage.CorruptDirectory(), true)))
		{
			OutError =
				TEXT("Treatment recovery corrupt directory could not be created.");
			return FString();
		}
		const FString Path = FPaths::Combine(
			Storage.CorruptDirectory(),
			FString::Printf(
				TEXT("%s_%s.corrupt.json"),
				*ByteSignature(Bytes),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		if (!FFileHelper::SaveArrayToFile(Bytes, *Path))
		{
			OutError =
				TEXT("Treatment recovery corrupt bytes could not be preserved.");
			return FString();
		}
		TArray<uint8> Verified;
		if (!FFileHelper::LoadFileToArray(Verified, *Path) || Verified != Bytes)
		{
			OutError =
				TEXT("Preserved treatment recovery bytes did not verify.");
			return FString();
		}
		return Path;
	}

	Fdemo_mapShanmenTreatmentRecoverySaveResult CommitDocument(
		Fdemo_mapShanmenTreatmentRecoveryDocument& InOutDocument,
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Candidate,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage,
		const Fdemo_mapShanmenTreatmentRecoveryDocument* ExpectedExisting)
	{
		Fdemo_mapShanmenTreatmentRecoverySaveResult Result;
		Result.PrimaryPath = Storage.PrimaryPath();
		Result.BackupPath = Storage.BackupPath();
		Result.TempPath = Storage.TempPath();
		FString Error;
		if (!Storage.IsValid()
			|| Candidate.OwnerId != Storage.OwnerId
			|| Candidate.RunId != Storage.RunId
			|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::
				ValidateDocument(Candidate, &Error))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoverySaveStatus::ValidationRejected;
			Result.Diagnostic = Error.IsEmpty()
				? TEXT("Treatment recovery storage context is invalid.")
				: Error;
			return Result;
		}
		TArray<uint8> CandidateBytes;
		if (!SerializeDocument(Candidate, CandidateBytes, &Error))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoverySaveStatus::SerializationFailed;
			Result.Diagnostic = Error;
			return Result;
		}
		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::
					CreateDirectory)
			|| (!IFileManager::Get().DirectoryExists(
					*Storage.StorageDirectory())
				&& !IFileManager::Get().MakeDirectory(
					*Storage.StorageDirectory(), true)))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoverySaveStatus::TempWriteFailed;
			Result.Diagnostic =
				TEXT("Treatment recovery storage directory could not be created.");
			return Result;
		}
		if (IFileManager::Get().FileExists(*Result.TempPath)
			&& !IFileManager::Get().Delete(
				*Result.TempPath, false, true, true))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoverySaveStatus::TempWriteFailed;
			Result.Diagnostic =
				TEXT("Stale treatment recovery temporary file could not be removed.");
			return Result;
		}
		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::WriteTemp))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoverySaveStatus::TempWriteFailed;
			Result.Diagnostic =
				TEXT("Injected treatment recovery temporary write failure.");
			return Result;
		}
		const bool bInjectFlush = ShouldFail(
			Storage,
			Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::
				FlushOrCloseTemp);
		if (!WriteBytesWithFullFlush(
				Result.TempPath, CandidateBytes, bInjectFlush, Error))
		{
			Result.Status = bInjectFlush
				? Edemo_mapShanmenTreatmentRecoverySaveStatus::
					TempFlushOrCloseFailed
				: Edemo_mapShanmenTreatmentRecoverySaveStatus::TempWriteFailed;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.bDiskStateChanged = true;
		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::ReadBackTemp))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
				TempVerificationFailed;
			Result.Diagnostic =
				TEXT("Injected treatment recovery temporary read-back failure.");
			return Result;
		}
		const Fdemo_mapRecoveryReadResult TempRead =
			ReadDocumentAtPath(Result.TempPath);
		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::ValidateTemp)
			|| TempRead.Kind != Edemo_mapRecoveryReadKind::Valid
			|| TempRead.Bytes != CandidateBytes
			|| !(TempRead.Document == Candidate))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
				TempVerificationFailed;
			Result.Diagnostic =
				TEXT("Treatment recovery temporary verification failed.");
			return Result;
		}

		const Fdemo_mapRecoveryReadResult Existing =
			ReadDocumentAtPath(Result.PrimaryPath);
		if (!ExpectedExisting)
		{
			if (Existing.Kind != Edemo_mapRecoveryReadKind::Missing
				|| IFileManager::Get().FileExists(*Result.BackupPath))
			{
				Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
					BackupPreparationFailed;
				Result.Diagnostic =
					TEXT("First treatment recovery publish found unexpected durable state; reopen is required.");
				return Result;
			}
		}
		else
		{
			if (Existing.Kind != Edemo_mapRecoveryReadKind::Valid
				|| !(Existing.Document == *ExpectedExisting))
			{
				Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
					BackupPreparationFailed;
				Result.Diagnostic =
					TEXT("Treatment recovery primary differs from the loaded generation; reopen is required.");
				return Result;
			}
			if (ShouldFail(
					Storage,
					Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::
						PrepareBackup)
				|| IFileManager::Get().Copy(
					*Result.BackupPath,
					*Result.PrimaryPath,
					true,
					true) != COPY_OK)
			{
				Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
					BackupPreparationFailed;
				Result.Diagnostic =
					TEXT("Treatment recovery backup preparation failed.");
				return Result;
			}
			TArray<uint8> BackupBytes;
			if (!FFileHelper::LoadFileToArray(
					BackupBytes, *Result.BackupPath)
				|| BackupBytes != Existing.Bytes)
			{
				Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
					BackupPreparationFailed;
				Result.Diagnostic =
					TEXT("Treatment recovery backup byte verification failed.");
				return Result;
			}
		}

		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::AtomicReplace)
			|| !IFileManager::Get().Move(
				*Result.PrimaryPath,
				*Result.TempPath,
				true,
				false,
				true,
				true))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoverySaveStatus::AtomicReplaceFailed;
			Result.Diagnostic =
				TEXT("Treatment recovery same-volume replacement failed.");
			return Result;
		}
		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::
					ReadBackCommittedPrimary))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
				PostCommitVerificationFailed;
			Result.Diagnostic =
				TEXT("Injected treatment recovery post-commit ambiguity; reopen is required.");
			return Result;
		}
		const Fdemo_mapRecoveryReadResult Committed =
			ReadDocumentAtPath(Result.PrimaryPath);
		if (Committed.Kind != Edemo_mapRecoveryReadKind::Valid
			|| Committed.Bytes != CandidateBytes
			|| !(Committed.Document == Candidate))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::
				PostCommitVerificationFailed;
			Result.Diagnostic =
				TEXT("Committed treatment recovery primary did not verify; reopen is required.");
			return Result;
		}

		InOutDocument = Candidate;
		Result.Status = Edemo_mapShanmenTreatmentRecoverySaveStatus::Saved;
		Result.Diagnostic =
			TEXT("Treatment recovery document committed and verified.");
		Result.CommittedGeneration = Candidate.SaveGeneration;
		if (ShouldFail(
				Storage,
				Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::CleanupTemp))
		{
			Result.bCleanupSucceeded = false;
			Result.Diagnostic +=
				TEXT(" Injected cleanup warning after atomic replacement.");
		}
		else if (IFileManager::Get().FileExists(*Result.TempPath))
		{
			Result.bCleanupSucceeded = IFileManager::Get().Delete(
				*Result.TempPath, false, true, true);
		}
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryLoadResult MigratePreviousDocument(
		const Fdemo_mapRecoveryReadResult& Previous,
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage,
		const bool bRecoveredFromBackup,
		const bool bPriorDiskStateChanged,
		const FString& QuarantinedPath)
	{
		Fdemo_mapShanmenTreatmentRecoveryLoadResult Result;
		Result.PrimaryPath = Storage.PrimaryPath();
		Result.BackupPath = Storage.BackupPath();
		Result.TempPath = Storage.TempPath();
		Result.QuarantinedPath = QuarantinedPath;
		Result.SourceSchemaVersion = Previous.SourceSchemaVersion;
		Result.bRecoveredFromBackup = bRecoveredFromBackup;
		Result.bDiskStateChanged = bPriorDiskStateChanged;
		if (Previous.Kind != Edemo_mapRecoveryReadKind::Valid
			|| Previous.SourceSchemaVersion
				!= Fdemo_mapShanmenTreatmentRecoveryDocument::
					PreviousSchemaVersion
			|| Previous.Document.SaveGeneration == MAX_int32)
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryLoadStatus::WriteRecoveryFailed;
			Result.Diagnostic =
				TEXT("Previous treatment recovery schema cannot advance its migration generation.");
			return Result;
		}

		const Fdemo_mapShanmenTreatmentRecoveryDocument Before =
			Previous.Document;
		Fdemo_mapShanmenTreatmentRecoveryDocument Candidate = Before;
		Candidate.SaveGeneration++;
		Candidate.LastSavedUtc = FDateTime::UtcNow().ToIso8601();
		RefreshSetIds(Candidate);
		Fdemo_mapShanmenTreatmentRecoveryDocument Caller = Before;
		const Fdemo_mapShanmenTreatmentRecoverySaveResult Saved = CommitDocument(
			Caller, Candidate, Storage, &Before);
		Result.bDiskStateChanged =
			Result.bDiskStateChanged || Saved.bDiskStateChanged;
		Result.Document = Caller;
		if (!Saved.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryLoadStatus::WriteRecoveryFailed;
			Result.Diagnostic = FString::Printf(
				TEXT("Previous treatment recovery schema migration failed: %s"),
				*Saved.Diagnostic);
			return Result;
		}

		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryLoadStatus::MigratedPreviousSchema;
		Result.Diagnostic = bRecoveredFromBackup
			? TEXT("Previous treatment recovery backup was restored and durably migrated to schema 2.")
			: TEXT("Previous treatment recovery primary was durably migrated to schema 2.");
		Result.bMigratedFromPreviousSchema = true;
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryMutationResult MutationFromSave(
		const Fdemo_mapShanmenTreatmentRecoverySaveResult& Saved,
		const Fdemo_mapShanmenTreatmentRecoveryDocument& Document,
		const Edemo_mapShanmenTreatmentRecoveryMutationStatus SuccessStatus)
	{
		Fdemo_mapShanmenTreatmentRecoveryMutationResult Result;
		Result.Diagnostic = Saved.Diagnostic;
		Result.bDiskStateChanged = Saved.bDiskStateChanged;
		Result.Document = Document;
		Result.Status = Saved.IsSuccess()
			? SuccessStatus
			: Edemo_mapShanmenTreatmentRecoveryMutationStatus::SaveFailed;
		return Result;
	}
}

bool Fdemo_mapShanmenTreatmentRecoveryDocument::operator==(
	const Fdemo_mapShanmenTreatmentRecoveryDocument& Other) const
{
	if (SchemaVersion != Other.SchemaVersion
		|| DocumentId != Other.DocumentId
		|| OwnerId != Other.OwnerId
		|| RunId != Other.RunId
		|| SaveGeneration != Other.SaveGeneration
		|| CreatedUtc != Other.CreatedUtc
		|| LastSavedUtc != Other.LastSavedUtc
		|| IntentSetId != Other.IntentSetId
		|| Intents.Num() != Other.Intents.Num()
		|| ProofSetId != Other.ProofSetId
		|| Proofs.Num() != Other.Proofs.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Intents.Num(); ++Index)
	{
		if (!Intents[Index].Matches(Other.Intents[Index]))
		{
			return false;
		}
	}
	for (int32 Index = 0; Index < Proofs.Num(); ++Index)
	{
		if (!Proofs[Index].Matches(Other.Proofs[Index]))
		{
			return false;
		}
	}
	return true;
}

Fdemo_mapShanmenTreatmentRecoveryStorageContext
Fdemo_mapShanmenTreatmentRecoveryStorageContext::Production(
	const FGuid& OwnerId,
	const FGuid& RunId)
{
	return ForRoot(FPaths::ProjectSavedDir(), OwnerId, RunId);
}

Fdemo_mapShanmenTreatmentRecoveryStorageContext
Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
	const FString& Root,
	const FGuid& OwnerId,
	const FGuid& RunId)
{
	Fdemo_mapShanmenTreatmentRecoveryStorageContext Result;
	Result.RootDirectory = Root.TrimStartAndEnd().IsEmpty()
		? FString()
		: FPaths::ConvertRelativePathToFull(Root);
	Result.OwnerId = OwnerId;
	Result.RunId = RunId;
	return Result;
}

FString Fdemo_mapShanmenTreatmentRecoveryStorageContext::StorageDirectory()
	const
{
	return FPaths::Combine(
		RootDirectory,
		TEXT("ShanmenCombatConditions"),
		TEXT("MeridianShockTreatment"),
		GuidDigits(OwnerId));
}

FString Fdemo_mapShanmenTreatmentRecoveryStorageContext::PrimaryPath() const
{
	return FPaths::Combine(
		StorageDirectory(), GuidDigits(RunId) + TEXT(".json"));
}

FString Fdemo_mapShanmenTreatmentRecoveryStorageContext::BackupPath() const
{
	return PrimaryPath() + TEXT(".bak");
}

FString Fdemo_mapShanmenTreatmentRecoveryStorageContext::TempPath() const
{
	return PrimaryPath() + TEXT(".tmp");
}

FString Fdemo_mapShanmenTreatmentRecoveryStorageContext::CorruptDirectory()
	const
{
	return FPaths::Combine(StorageDirectory(), TEXT("Corrupt"));
}

bool Fdemo_mapShanmenTreatmentRecoveryStorageContext::IsValid() const
{
	return !RootDirectory.IsEmpty() && OwnerId.IsValid() && RunId.IsValid();
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::ValidateDocument(
	const Fdemo_mapShanmenTreatmentRecoveryDocument& Document,
	FString* OutError)
{
	return ValidateDocumentForSchema(
		Document,
		Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion,
		OutError);
}

Fdemo_mapShanmenTreatmentRecoveryLoadResult
Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::LoadExisting(
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const
{
	Fdemo_mapShanmenTreatmentRecoveryLoadResult Result;
	Result.PrimaryPath = Storage.PrimaryPath();
	Result.BackupPath = Storage.BackupPath();
	Result.TempPath = Storage.TempPath();
	if (!Storage.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryLoadStatus::InvalidDocument;
		Result.Diagnostic =
			TEXT("Treatment recovery load storage context is invalid.");
		return Result;
	}

	const Fdemo_mapRecoveryReadResult Primary =
		ReadDocumentAtPath(Result.PrimaryPath);
	if (Primary.Kind == Edemo_mapRecoveryReadKind::Valid)
	{
		if (Primary.Document.OwnerId != Storage.OwnerId
			|| Primary.Document.RunId != Storage.RunId)
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryLoadStatus::InvalidDocument;
			Result.Diagnostic =
				TEXT("Treatment recovery primary identity differs from its path.");
			return Result;
		}
		if (Primary.SourceSchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::
				PreviousSchemaVersion)
		{
			return MigratePreviousDocument(
				Primary, Storage, false, false, FString());
		}
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryLoadStatus::LoadedPrimary;
		Result.Diagnostic =
			TEXT("Treatment recovery primary loaded without a write.");
		Result.SourceSchemaVersion = Primary.SourceSchemaVersion;
		Result.Document = Primary.Document;
		return Result;
	}
	if (Primary.Kind == Edemo_mapRecoveryReadKind::FutureSchema)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryLoadStatus::FutureSchemaRejected;
		Result.Diagnostic = Primary.Diagnostic;
		return Result;
	}
	if (Primary.Kind == Edemo_mapRecoveryReadKind::ReadFailure)
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::ReadFailed;
		Result.Diagnostic = Primary.Diagnostic;
		return Result;
	}

	const Fdemo_mapRecoveryReadResult Backup =
		ReadDocumentAtPath(Result.BackupPath);
	if (Backup.Kind == Edemo_mapRecoveryReadKind::FutureSchema)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryLoadStatus::FutureSchemaRejected;
		Result.Diagnostic =
			TEXT("Treatment recovery backup uses a future schema; no downgrade or reset was performed.");
		return Result;
	}
	if (Backup.Kind == Edemo_mapRecoveryReadKind::ReadFailure)
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::ReadFailed;
		Result.Diagnostic = Backup.Diagnostic;
		return Result;
	}
	if (Backup.Kind != Edemo_mapRecoveryReadKind::Valid
		|| Backup.Document.OwnerId != Storage.OwnerId
		|| Backup.Document.RunId != Storage.RunId)
	{
		const bool bBothAbsent =
			Primary.Kind == Edemo_mapRecoveryReadKind::Missing
			&& Backup.Kind == Edemo_mapRecoveryReadKind::Missing;
		Result.Status = bBothAbsent
			? Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing
			: Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				CorruptPrimaryNoValidBackup;
		Result.Diagnostic = bBothAbsent
			? TEXT("Treatment recovery primary and valid backup are absent.")
			: TEXT("Treatment recovery durable state exists but no valid current-schema source can be loaded; no reset was performed.");
		return Result;
	}

	FString Error;
	if (Primary.Kind != Edemo_mapRecoveryReadKind::Missing)
	{
		Result.QuarantinedPath =
			PreserveCorruptPrimary(Storage, Primary.Bytes, Error);
		if (Result.QuarantinedPath.IsEmpty())
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				WriteRecoveryFailed;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.bDiskStateChanged = true;
	}
	if (ShouldFail(
			Storage,
			Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::CreateDirectory)
		|| (!IFileManager::Get().DirectoryExists(*Storage.StorageDirectory())
			&& !IFileManager::Get().MakeDirectory(
				*Storage.StorageDirectory(), true)))
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			WriteRecoveryFailed;
		Result.Diagnostic =
			TEXT("Treatment recovery directory could not be created for recovery.");
		return Result;
	}
	if (IFileManager::Get().FileExists(*Result.TempPath)
		&& !IFileManager::Get().Delete(*Result.TempPath, false, true, true))
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			WriteRecoveryFailed;
		Result.Diagnostic =
			TEXT("Stale treatment recovery temporary file could not be removed.");
		return Result;
	}
	const bool bInjectFlush = ShouldFail(
		Storage,
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::FlushOrCloseTemp);
	if (ShouldFail(
			Storage,
			Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::WriteTemp)
		|| !WriteBytesWithFullFlush(
			Result.TempPath, Backup.Bytes, bInjectFlush, Error))
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			WriteRecoveryFailed;
		Result.Diagnostic = Error.IsEmpty()
			? TEXT("Injected treatment recovery temporary recovery failure.")
			: Error;
		return Result;
	}
	Result.bDiskStateChanged = true;
	const Fdemo_mapRecoveryReadResult Temp = ShouldFail(
		Storage,
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::ReadBackTemp)
		? Fdemo_mapRecoveryReadResult()
		: ReadDocumentAtPath(Result.TempPath);
	if (ShouldFail(
			Storage,
			Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::ValidateTemp)
		|| Temp.Kind != Edemo_mapRecoveryReadKind::Valid
		|| Temp.Bytes != Backup.Bytes
		|| !(Temp.Document == Backup.Document))
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			WriteRecoveryFailed;
		Result.Diagnostic =
			TEXT("Treatment recovery backup temporary verification failed.");
		return Result;
	}
	if (ShouldFail(
			Storage,
			Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::AtomicReplace)
		|| !IFileManager::Get().Move(
			*Result.PrimaryPath,
			*Result.TempPath,
			true,
			false,
			true,
			true))
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			WriteRecoveryFailed;
		Result.Diagnostic =
			TEXT("Treatment recovery backup replacement failed.");
		return Result;
	}
	const Fdemo_mapRecoveryReadResult Recovered = ShouldFail(
		Storage,
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::
			ReadBackCommittedPrimary)
		? Fdemo_mapRecoveryReadResult()
		: ReadDocumentAtPath(Result.PrimaryPath);
	if (Recovered.Kind != Edemo_mapRecoveryReadKind::Valid
		|| Recovered.Bytes != Backup.Bytes
		|| !(Recovered.Document == Backup.Document))
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			WriteRecoveryFailed;
		Result.Diagnostic =
			TEXT("Recovered treatment recovery primary did not verify.");
		return Result;
	}
	Result.Status = Primary.Kind == Edemo_mapRecoveryReadKind::Missing
		? Edemo_mapShanmenTreatmentRecoveryLoadStatus::
			PrimaryMissingBackupRecovered
		: Edemo_mapShanmenTreatmentRecoveryLoadStatus::RecoveredFromBackup;
	Result.Diagnostic =
		TEXT("Verified treatment recovery backup restored without changing generation.");
	Result.SourceSchemaVersion = Recovered.SourceSchemaVersion;
	Result.bRecoveredFromBackup = true;
	if (Recovered.SourceSchemaVersion
		== Fdemo_mapShanmenTreatmentRecoveryDocument::PreviousSchemaVersion)
	{
		return MigratePreviousDocument(
			Recovered,
			Storage,
			true,
			Result.bDiskStateChanged,
			Result.QuarantinedPath);
	}
	Result.Document = Recovered.Document;
	return Result;
}

Fdemo_mapShanmenTreatmentRecoveryMutationResult
Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::RecordIntent(
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent,
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const
{
	Fdemo_mapShanmenTreatmentRecoveryMutationResult Result;
	if (!Storage.IsValid()
		|| !Intent.IsValid()
		|| Intent.GetRunId() != Storage.RunId)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment intent or Owner/Run storage context is invalid.");
		return Result;
	}

	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Loaded =
		LoadExisting(Storage);
	const bool bCreating = Loaded.Status
		== Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing;
	if (!Loaded.IsSuccess() && !bCreating)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::LoadFailed;
		Result.Diagnostic = Loaded.Diagnostic;
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryDocument Document;
	if (bCreating)
	{
		InitializeDocument(Storage, Document);
	}
	else
	{
		Document = Loaded.Document;
	}

	for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Existing :
		Document.Proofs)
	{
		if (Existing.GetTreatmentId() != Intent.GetTreatmentId())
		{
			continue;
		}
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Document;
		if (IntentMatchesProof(Intent, Existing))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				AlreadyPromoted;
			Result.Diagnostic =
				TEXT("Treatment intent already has its exact durable proof.");
		}
		else
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
			Result.Diagnostic =
				TEXT("TreatmentId conflicts with a different durable proof.");
		}
		return Result;
	}

	for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Existing :
		Document.Intents)
	{
		if (Existing.GetTreatmentId() != Intent.GetTreatmentId())
		{
			continue;
		}
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Document;
		if (Existing.Matches(Intent))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				IntentAlreadyRecorded;
			Result.Diagnostic =
				TEXT("Exact treatment intent was already durably recorded.");
		}
		else
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
			Result.Diagnostic =
				TEXT("TreatmentId conflicts with a different durable intent.");
		}
		return Result;
	}

	if (Document.Intents.Num()
			>= Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumIntentCount
		|| Document.Intents.Num() + Document.Proofs.Num()
			>= Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumEntryCount)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery intent or entry bound is exhausted.");
		return Result;
	}
	const Fdemo_mapShanmenTreatmentRecoveryDocument Before = Document;
	Document.Intents.Add(Intent);
	CanonicalizeIntents(Document.Intents);
	if (!TryAdvanceDocument(Document))
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery generation bound is exhausted.");
		return Result;
	}
	Fdemo_mapShanmenTreatmentRecoveryDocument Caller = Before;
	const Fdemo_mapShanmenTreatmentRecoverySaveResult Saved = CommitDocument(
		Caller,
		Document,
		Storage,
		bCreating ? nullptr : &Before);
	Result = MutationFromSave(
		Saved,
		Caller,
		Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentRecorded);
	Result.bDiskStateChanged =
		Result.bDiskStateChanged || Loaded.bDiskStateChanged;
	return Result;
}

Fdemo_mapShanmenTreatmentRecoveryMutationResult
Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::PromoteIntentToProof(
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent,
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof,
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const
{
	Fdemo_mapShanmenTreatmentRecoveryMutationResult Result;
	if (!Storage.IsValid()
		|| Intent.GetRunId() != Storage.RunId
		|| Proof.GetRunId() != Storage.RunId
		|| !IntentMatchesProof(Intent, Proof))
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment intent and proof do not describe one exact Owner/Run mutation.");
		return Result;
	}

	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Loaded =
		LoadExisting(Storage);
	if (!Loaded.IsSuccess())
	{
		Result.Status = Loaded.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing
			? Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict
			: Edemo_mapShanmenTreatmentRecoveryMutationStatus::LoadFailed;
		Result.Diagnostic = Loaded.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing
			? TEXT("No durable treatment intent exists to promote.")
			: Loaded.Diagnostic;
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryDocument Document = Loaded.Document;
	for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Existing :
		Document.Proofs)
	{
		if (Existing.GetTreatmentId() != Proof.GetTreatmentId())
		{
			continue;
		}
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Document;
		if (Existing.Matches(Proof))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				AlreadyPromoted;
			Result.Diagnostic =
				TEXT("Exact treatment intent was already atomically promoted.");
		}
		else
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
			Result.Diagnostic =
				TEXT("TreatmentId conflicts with a different durable proof.");
		}
		return Result;
	}

	int32 IntentIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Document.Intents.Num(); ++Index)
	{
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Existing =
			Document.Intents[Index];
		if (Existing.GetTreatmentId() != Intent.GetTreatmentId())
		{
			continue;
		}
		if (!Existing.Matches(Intent))
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
			Result.Diagnostic =
				TEXT("TreatmentId conflicts with a different durable intent.");
			Result.bDiskStateChanged = Loaded.bDiskStateChanged;
			Result.Document = Document;
			return Result;
		}
		IntentIndex = Index;
		break;
	}
	if (IntentIndex == INDEX_NONE)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
		Result.Diagnostic =
			TEXT("Exact durable treatment intent is absent; proof was not recorded.");
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Document;
		return Result;
	}

	const Fdemo_mapShanmenTreatmentRecoveryDocument Before = Document;
	Document.Intents.RemoveAt(IntentIndex);
	Document.Proofs.Add(Proof);
	CanonicalizeIntents(Document.Intents);
	CanonicalizeProofs(Document.Proofs);
	if (!TryAdvanceDocument(Document))
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery generation bound is exhausted.");
		return Result;
	}
	Fdemo_mapShanmenTreatmentRecoveryDocument Caller = Before;
	const Fdemo_mapShanmenTreatmentRecoverySaveResult Saved = CommitDocument(
		Caller, Document, Storage, &Before);
	Result = MutationFromSave(
		Saved,
		Caller,
		Edemo_mapShanmenTreatmentRecoveryMutationStatus::Promoted);
	Result.bDiskStateChanged =
		Result.bDiskStateChanged || Loaded.bDiskStateChanged;
	return Result;
}

Fdemo_mapShanmenTreatmentRecoveryMutationResult
Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::ForgetIntent(
	const FGuid& TreatmentId,
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const
{
	Fdemo_mapShanmenTreatmentRecoveryMutationResult Result;
	if (!Storage.IsValid() || !TreatmentId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery intent-forget request is invalid.");
		return Result;
	}
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Loaded =
		LoadExisting(Storage);
	if (Loaded.Status == Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing)
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
			IntentAlreadyAbsent;
		Result.Diagnostic =
			TEXT("No treatment recovery document exists; intent is already absent.");
		return Result;
	}
	if (!Loaded.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::LoadFailed;
		Result.Diagnostic = Loaded.Diagnostic;
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryDocument Document = Loaded.Document;
	for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
		Document.Proofs)
	{
		if (Proof.GetTreatmentId() == TreatmentId)
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				AlreadyPromoted;
			Result.Diagnostic =
				TEXT("Treatment intent is absent because its durable proof exists.");
			Result.bDiskStateChanged = Loaded.bDiskStateChanged;
			Result.Document = Document;
			return Result;
		}
	}
	const int32 Removed = Document.Intents.RemoveAll(
		[&TreatmentId](
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent)
		{
			return Intent.GetTreatmentId() == TreatmentId;
		});
	if (Removed == 0)
	{
		Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
			IntentAlreadyAbsent;
		Result.Diagnostic =
			TEXT("Treatment intent is already absent from durable state.");
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Loaded.Document;
		return Result;
	}
	if (Removed != 1)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
		Result.Diagnostic =
			TEXT("Treatment recovery document contains conflicting intent identity.");
		return Result;
	}
	const Fdemo_mapShanmenTreatmentRecoveryDocument Before = Loaded.Document;
	if (!TryAdvanceDocument(Document))
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery generation bound is exhausted.");
		return Result;
	}
	Fdemo_mapShanmenTreatmentRecoveryDocument Caller = Before;
	const Fdemo_mapShanmenTreatmentRecoverySaveResult Saved = CommitDocument(
		Caller, Document, Storage, &Before);
	Result = MutationFromSave(
		Saved,
		Caller,
		Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentForgotten);
	Result.bDiskStateChanged =
		Result.bDiskStateChanged || Loaded.bDiskStateChanged;
	return Result;
}

Fdemo_mapShanmenTreatmentRecoveryMutationResult
Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::RecordProof(
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof,
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const
{
	Fdemo_mapShanmenTreatmentRecoveryMutationResult Result;
	if (!Storage.IsValid()
		|| !Proof.IsValid()
		|| Proof.GetRunId() != Storage.RunId)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment proof or Owner/Run storage context is invalid.");
		return Result;
	}

	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Loaded =
		LoadExisting(Storage);
	const bool bCreating = Loaded.Status
		== Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing;
	if (!Loaded.IsSuccess() && !bCreating)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::LoadFailed;
		Result.Diagnostic = Loaded.Diagnostic;
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryDocument Document;
	if (bCreating)
	{
		InitializeDocument(Storage, Document);
	}
	else
	{
		Document = Loaded.Document;
	}

	for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Existing :
		Document.Proofs)
	{
		if (Existing.GetTreatmentId() != Proof.GetTreatmentId())
		{
			continue;
		}
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Document;
		if (Existing.Matches(Proof))
		{
			Result.Status = Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				AlreadyRecorded;
			Result.Diagnostic =
				TEXT("Exact treatment proof was already durably recorded.");
		}
		else
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
			Result.Diagnostic =
				TEXT("TreatmentId conflicts with a different durable proof.");
		}
		return Result;
	}
	for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Intent :
		Document.Intents)
	{
		if (Intent.GetTreatmentId() == Proof.GetTreatmentId())
		{
			Result.Status =
				Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
			Result.Diagnostic =
				TEXT("Durable treatment intent must be atomically promoted, not bypassed by RecordProof.");
			Result.bDiskStateChanged = Loaded.bDiskStateChanged;
			Result.Document = Document;
			return Result;
		}
	}

	if (Document.Proofs.Num()
			>= Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumProofCount
		|| Document.Intents.Num() + Document.Proofs.Num()
			>= Fdemo_mapShanmenTreatmentRecoveryDocument::MaximumEntryCount)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery proof or generation bound is exhausted.");
		return Result;
	}
	const Fdemo_mapShanmenTreatmentRecoveryDocument Before = Document;
	Document.Proofs.Add(Proof);
	CanonicalizeProofs(Document.Proofs);
	if (!TryAdvanceDocument(Document))
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery generation bound is exhausted.");
		return Result;
	}
	Fdemo_mapShanmenTreatmentRecoveryDocument Caller = Before;
	const Fdemo_mapShanmenTreatmentRecoverySaveResult Saved = CommitDocument(
		Caller,
		Document,
		Storage,
		bCreating ? nullptr : &Before);
	Result = MutationFromSave(
		Saved,
		Caller,
		Edemo_mapShanmenTreatmentRecoveryMutationStatus::Recorded);
	Result.bDiskStateChanged =
		Result.bDiskStateChanged || Loaded.bDiskStateChanged;
	return Result;
}

Fdemo_mapShanmenTreatmentRecoveryMutationResult
Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore::ForgetProof(
	const FGuid& TreatmentId,
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage) const
{
	Fdemo_mapShanmenTreatmentRecoveryMutationResult Result;
	if (!Storage.IsValid() || !TreatmentId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery forget request is invalid.");
		return Result;
	}
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Loaded =
		LoadExisting(Storage);
	if (Loaded.Status == Edemo_mapShanmenTreatmentRecoveryLoadStatus::Missing)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyAbsent;
		Result.Diagnostic =
			TEXT("No treatment recovery document exists; proof is already absent.");
		return Result;
	}
	if (!Loaded.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::LoadFailed;
		Result.Diagnostic = Loaded.Diagnostic;
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		return Result;
	}

	Fdemo_mapShanmenTreatmentRecoveryDocument Document = Loaded.Document;
	const int32 Removed = Document.Proofs.RemoveAll(
		[&TreatmentId](
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof)
		{
			return Proof.GetTreatmentId() == TreatmentId;
		});
	if (Removed == 0)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyAbsent;
		Result.Diagnostic =
			TEXT("Treatment proof is already absent from durable state.");
		Result.bDiskStateChanged = Loaded.bDiskStateChanged;
		Result.Document = Loaded.Document;
		return Result;
	}
	if (Removed != 1)
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict;
		Result.Diagnostic =
			TEXT("Treatment recovery document contains conflicting proof identity.");
		return Result;
	}
	const Fdemo_mapShanmenTreatmentRecoveryDocument Before = Loaded.Document;
	if (!TryAdvanceDocument(Document))
	{
		Result.Status =
			Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest;
		Result.Diagnostic =
			TEXT("Treatment recovery generation bound is exhausted.");
		return Result;
	}
	Fdemo_mapShanmenTreatmentRecoveryDocument Caller = Before;
	const Fdemo_mapShanmenTreatmentRecoverySaveResult Saved = CommitDocument(
		Caller, Document, Storage, &Before);
	Result = MutationFromSave(
		Saved,
		Caller,
		Edemo_mapShanmenTreatmentRecoveryMutationStatus::Forgotten);
	Result.bDiskStateChanged =
		Result.bDiskStateChanged || Loaded.bDiskStateChanged;
	return Result;
}
