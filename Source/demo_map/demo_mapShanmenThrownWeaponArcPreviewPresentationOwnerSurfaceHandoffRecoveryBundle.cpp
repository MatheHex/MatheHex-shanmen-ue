#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;
	using FCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleCodec;
	using FDecodeResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeResult;
	using EDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using FJournalCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;
	using EJournalDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus;
	using EJournalDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using EJournalRecordKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;
	using FJournalRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;
	using FEnvelope =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope;
	using FPayloadCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec;
	using EPayloadDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus;
	using FCheckpoint =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint;

	constexpr uint8 BundleMagic[] =
	{
		'S', 'M', 'A', 'R', 'C', 'B', 'N', 'D'
	};

	void AppendUint32BigEndian(TArray<uint8>& OutBytes, const uint32 Value)
	{
		OutBytes.Add(static_cast<uint8>((Value >> 24) & 0xff));
		OutBytes.Add(static_cast<uint8>((Value >> 16) & 0xff));
		OutBytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
		OutBytes.Add(static_cast<uint8>(Value & 0xff));
	}

	void AppendGuid(TArray<uint8>& OutBytes, const FGuid& Value)
	{
		AppendUint32BigEndian(OutBytes, Value.A);
		AppendUint32BigEndian(OutBytes, Value.B);
		AppendUint32BigEndian(OutBytes, Value.C);
		AppendUint32BigEndian(OutBytes, Value.D);
	}

	bool TryReadUint32BigEndian(
		const TArray<uint8>& Bytes,
		int32& InOutOffset,
		uint32& OutValue)
	{
		OutValue = 0;
		if (InOutOffset < 0 || InOutOffset > Bytes.Num() - 4)
		{
			return false;
		}
		OutValue = (static_cast<uint32>(Bytes[InOutOffset]) << 24)
			| (static_cast<uint32>(Bytes[InOutOffset + 1]) << 16)
			| (static_cast<uint32>(Bytes[InOutOffset + 2]) << 8)
			| static_cast<uint32>(Bytes[InOutOffset + 3]);
		InOutOffset += 4;
		return true;
	}

	bool TryReadGuid(
		const TArray<uint8>& Bytes,
		int32& InOutOffset,
		FGuid& OutGuid)
	{
		OutGuid.Invalidate();
		uint32 A = 0;
		uint32 B = 0;
		uint32 C = 0;
		uint32 D = 0;
		if (!TryReadUint32BigEndian(Bytes, InOutOffset, A)
			|| !TryReadUint32BigEndian(Bytes, InOutOffset, B)
			|| !TryReadUint32BigEndian(Bytes, InOutOffset, C)
			|| !TryReadUint32BigEndian(Bytes, InOutOffset, D))
		{
			return false;
		}
		OutGuid = FGuid(A, B, C, D);
		return true;
	}

	FString BytesKey(const TArray<uint8>& Bytes)
	{
		static constexpr TCHAR HexDigits[] = TEXT("0123456789ABCDEF");
		FString Result;
		Result.Reserve(Bytes.Num() * 2);
		for (const uint8 Byte : Bytes)
		{
			Result.AppendChar(HexDigits[(Byte >> 4) & 0x0f]);
			Result.AppendChar(HexDigits[Byte & 0x0f]);
		}
		return Result;
	}

	int32 DeriveGeneration(const FJournal& Journal)
	{
		if (!Journal.IsValid()
			|| Journal.GetLatestDisposition()
				!= EJournalDisposition::CheckpointPending
			|| Journal.GetRecordCount() <= 0
			|| Journal.GetRecordCount() % 2 == 0)
		{
			return 0;
		}
		FJournalRecord Latest;
		const int32 Generation = (Journal.GetRecordCount() + 1) / 2;
		if (!Journal.TryGetLatestRecord(Latest)
			|| Latest.GetKind() != EJournalRecordKind::CheckpointPrepared
			|| Latest.GetSequence() != Journal.GetRecordCount() - 1
			|| Generation < 1 || Generation > FBundle::MaximumGeneration())
		{
			return 0;
		}
		return Generation;
	}

	bool TryBuildCanonicalSections(
		const FJournal& Journal,
		const FEnvelope& Envelope,
		TArray<uint8>& OutJournalBytes,
		TArray<uint8>& OutPayloadBytes)
	{
		OutJournalBytes.Reset();
		OutPayloadBytes.Reset();
		return Journal.IsValid() && Envelope.IsValid()
			&& DeriveGeneration(Journal) > 0
			&& Envelope.MatchesJournal(Journal)
			&& FJournalCodec::TryEncode(Journal, OutJournalBytes)
			&& FPayloadCodec::TryEncode(Envelope, OutPayloadBytes)
			&& OutJournalBytes.Num() >= FJournalCodec::HeaderSize()
			&& OutJournalBytes.Num() <= FCodec::MaximumJournalEncodedBytes()
			&& OutPayloadBytes.Num() >= FPayloadCodec::HeaderSize()
			&& OutPayloadBytes.Num() <= FCodec::MaximumPayloadEncodedBytes();
	}

	FGuid DeriveBundleDigest(
		const int32 SchemaVersion,
		const int32 Generation,
		const FGuid& JournalId,
		const FGuid& EnvelopeId,
		const TArray<uint8>& JournalBytes,
		const TArray<uint8>& PayloadBytes)
	{
		if (SchemaVersion != FCodec::CurrentSchemaVersion()
			|| Generation < 1 || Generation > FBundle::MaximumGeneration()
			|| !JournalId.IsValid() || !EnvelopeId.IsValid()
			|| JournalBytes.IsEmpty() || PayloadBytes.IsEmpty())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryBundleDigest.r1"),
			{
				FString::FromInt(SchemaVersion),
				FString::FromInt(Generation),
				JournalId.ToString(EGuidFormats::Digits),
				EnvelopeId.ToString(EGuidFormats::Digits),
				FString::FromInt(JournalBytes.Num()),
				BytesKey(JournalBytes),
				FString::FromInt(PayloadBytes.Num()),
				BytesKey(PayloadBytes)
			});
	}

	FGuid DeriveBundleId(
		const int32 SchemaVersion,
		const int32 Generation,
		const FGuid& JournalId,
		const FGuid& EnvelopeId,
		const FGuid& BundleDigest)
	{
		if (SchemaVersion != FCodec::CurrentSchemaVersion()
			|| Generation < 1 || Generation > FBundle::MaximumGeneration()
			|| !JournalId.IsValid() || !EnvelopeId.IsValid()
			|| !BundleDigest.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryBundle.r1"),
			{
				FString::FromInt(SchemaVersion),
				FString::FromInt(Generation),
				JournalId.ToString(EGuidFormats::Digits),
				EnvelopeId.ToString(EGuidFormats::Digits),
				BundleDigest.ToString(EGuidFormats::Digits)
			});
	}
}

bool FBundle::TryCreate(
	const FJournal& InJournal,
	const FEnvelope& InEnvelope,
	FBundle& OutBundle)
{
	OutBundle = FBundle();
	TArray<uint8> JournalBytes;
	TArray<uint8> PayloadBytes;
	const int32 CandidateGeneration = DeriveGeneration(InJournal);
	if (CandidateGeneration <= 0
		|| !TryBuildCanonicalSections(
			InJournal, InEnvelope, JournalBytes, PayloadBytes))
	{
		return false;
	}

	FBundle Candidate;
	Candidate.SchemaVersion = FCodec::CurrentSchemaVersion();
	Candidate.Generation = CandidateGeneration;
	Candidate.Journal = InJournal;
	Candidate.Envelope = InEnvelope;
	Candidate.BundleDigest = DeriveBundleDigest(
		Candidate.SchemaVersion,
		Candidate.Generation,
		Candidate.Journal.GetJournalId(),
		Candidate.Envelope.GetEnvelopeId(),
		JournalBytes,
		PayloadBytes);
	Candidate.BundleId = DeriveBundleId(
		Candidate.SchemaVersion,
		Candidate.Generation,
		Candidate.Journal.GetJournalId(),
		Candidate.Envelope.GetEnvelopeId(),
		Candidate.BundleDigest);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutBundle = MoveTemp(Candidate);
	return true;
}

bool FBundle::IsValid() const
{
	TArray<uint8> JournalBytes;
	TArray<uint8> PayloadBytes;
	return SchemaVersion == FCodec::CurrentSchemaVersion()
		&& Generation == DeriveGeneration(Journal)
		&& BundleId.IsValid() && BundleDigest.IsValid()
		&& TryBuildCanonicalSections(
			Journal, Envelope, JournalBytes, PayloadBytes)
		&& BundleDigest == DeriveBundleDigest(
			SchemaVersion,
			Generation,
			Journal.GetJournalId(),
			Envelope.GetEnvelopeId(),
			JournalBytes,
			PayloadBytes)
		&& BundleId == DeriveBundleId(
			SchemaVersion,
			Generation,
			Journal.GetJournalId(),
			Envelope.GetEnvelopeId(),
			BundleDigest);
}

bool FBundle::Matches(const FBundle& Other) const
{
	return IsValid() && Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& Generation == Other.Generation
		&& BundleId == Other.BundleId
		&& BundleDigest == Other.BundleDigest
		&& Journal.GetJournalId() == Other.Journal.GetJournalId()
		&& Envelope.Matches(Other.Envelope);
}

bool FBundle::TryCopyPendingCheckpointEvidenceForJournal(
	const FJournal& CurrentJournal,
	const int32 MinimumGeneration,
	FCheckpoint& OutCheckpoint) const
{
	OutCheckpoint = FCheckpoint();
	return IsValid()
		&& MinimumGeneration >= 1
		&& MinimumGeneration <= MaximumGeneration()
		&& Generation >= MinimumGeneration
		&& CurrentJournal.IsValid()
		&& CurrentJournal.GetJournalId() == Journal.GetJournalId()
		&& CurrentJournal.GetRecordCount() == Journal.GetRecordCount()
		&& Envelope.TryUnwrapForJournal(CurrentJournal, OutCheckpoint)
		&& OutCheckpoint.IsValid();
}

bool FDecodeResult::IsSuccess() const
{
	return Status == EDecodeStatus::Decoded
		&& SourceSchemaVersion == FCodec::CurrentSchemaVersion()
		&& JournalDecodeStatus == EJournalDecodeStatus::DecodedCurrent
		&& PayloadDecodeStatus == EPayloadDecodeStatus::Decoded
		&& !Diagnostic.IsEmpty() && Bundle.IsValid();
}

FDecodeResult FCodec::MakeDecodeResult(
	const EDecodeStatus Status,
	const TCHAR* Diagnostic,
	const int32 SourceSchemaVersion,
	const EJournalDecodeStatus JournalDecodeStatus,
	const EPayloadDecodeStatus PayloadDecodeStatus,
	const FBundle& Bundle)
{
	FDecodeResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SourceSchemaVersion = SourceSchemaVersion;
	Result.JournalDecodeStatus = JournalDecodeStatus;
	Result.PayloadDecodeStatus = PayloadDecodeStatus;
	Result.Bundle = Bundle;
	return Result;
}

bool FCodec::TryEncode(const FBundle& Bundle, TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	TArray<uint8> JournalBytes;
	TArray<uint8> PayloadBytes;
	if (!Bundle.IsValid()
		|| !TryBuildCanonicalSections(
			Bundle.GetJournal(),
			Bundle.GetEnvelope(),
			JournalBytes,
			PayloadBytes))
	{
		return false;
	}
	const int64 TotalSize = static_cast<int64>(HeaderSize())
		+ JournalBytes.Num() + PayloadBytes.Num();
	if (TotalSize > MaximumEncodedBytes()
		|| TotalSize > static_cast<int64>(MAX_uint32))
	{
		return false;
	}

	OutBytes.Reserve(static_cast<int32>(TotalSize));
	OutBytes.Append(BundleMagic, UE_ARRAY_COUNT(BundleMagic));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Bundle.GetSchemaVersion()));
	AppendUint32BigEndian(OutBytes, static_cast<uint32>(TotalSize));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Bundle.GetGeneration()));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(JournalBytes.Num()));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(PayloadBytes.Num()));
	AppendGuid(OutBytes, Bundle.GetBundleId());
	AppendGuid(OutBytes, Bundle.GetBundleDigest());
	AppendGuid(OutBytes, Bundle.GetJournal().GetJournalId());
	AppendGuid(OutBytes, Bundle.GetEnvelope().GetEnvelopeId());
	if (OutBytes.Num() != HeaderSize())
	{
		OutBytes.Reset();
		return false;
	}
	OutBytes.Append(JournalBytes);
	OutBytes.Append(PayloadBytes);
	return OutBytes.Num() == TotalSize;
}

FDecodeResult FCodec::Decode(const TArray<uint8>& Bytes)
{
	if (Bytes.IsEmpty())
	{
		return MakeDecodeResult(
			EDecodeStatus::InputEmpty,
			TEXT("Arc preview handoff recovery bundle input is empty."));
	}
	if (Bytes.Num() < HeaderSize() || Bytes.Num() > MaximumEncodedBytes())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview handoff recovery bundle size is outside the bounded schema."));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BundleMagic); ++Index)
	{
		if (Bytes[Index] != BundleMagic[Index])
		{
			return MakeDecodeResult(
				EDecodeStatus::MagicMismatch,
				TEXT("Arc preview handoff recovery bundle magic does not match."));
		}
	}

	int32 Offset = UE_ARRAY_COUNT(BundleMagic);
	uint32 SchemaValue = 0;
	uint32 DeclaredSizeValue = 0;
	uint32 GenerationValue = 0;
	uint32 JournalSizeValue = 0;
	uint32 PayloadSizeValue = 0;
	FGuid ExpectedBundleId;
	FGuid ExpectedBundleDigest;
	FGuid ExpectedJournalId;
	FGuid ExpectedEnvelopeId;
	if (!TryReadUint32BigEndian(Bytes, Offset, SchemaValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, DeclaredSizeValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, GenerationValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, JournalSizeValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, PayloadSizeValue)
		|| !TryReadGuid(Bytes, Offset, ExpectedBundleId)
		|| !TryReadGuid(Bytes, Offset, ExpectedBundleDigest)
		|| !TryReadGuid(Bytes, Offset, ExpectedJournalId)
		|| !TryReadGuid(Bytes, Offset, ExpectedEnvelopeId)
		|| Offset != HeaderSize())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview handoff recovery bundle header is truncated."));
	}
	if (SchemaValue != static_cast<uint32>(CurrentSchemaVersion()))
	{
		return MakeDecodeResult(
			EDecodeStatus::UnsupportedSchema,
			TEXT("Arc preview handoff recovery bundle schema is unsupported."),
			static_cast<int32>(SchemaValue));
	}
	const int32 SchemaVersion = static_cast<int32>(SchemaValue);
	if (GenerationValue < 1
		|| GenerationValue > static_cast<uint32>(FBundle::MaximumGeneration()))
	{
		return MakeDecodeResult(
			EDecodeStatus::GenerationOutOfRange,
			TEXT("Arc preview handoff recovery bundle generation is outside the journal bound."),
			SchemaVersion);
	}
	if (JournalSizeValue < static_cast<uint32>(FJournalCodec::HeaderSize())
		|| JournalSizeValue > static_cast<uint32>(MaximumJournalEncodedBytes())
		|| PayloadSizeValue < static_cast<uint32>(FPayloadCodec::HeaderSize())
		|| PayloadSizeValue > static_cast<uint32>(MaximumPayloadEncodedBytes()))
	{
		return MakeDecodeResult(
			EDecodeStatus::SectionSizeOutOfRange,
			TEXT("Arc preview handoff recovery bundle section length exceeds its bound."),
			SchemaVersion);
	}
	const int64 ExpectedSize = static_cast<int64>(HeaderSize())
		+ JournalSizeValue + PayloadSizeValue;
	if (DeclaredSizeValue != static_cast<uint32>(Bytes.Num())
		|| ExpectedSize != Bytes.Num())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview handoff recovery bundle declared sections do not exactly fill the input."),
			SchemaVersion);
	}

	TArray<uint8> JournalBytes;
	JournalBytes.Append(Bytes.GetData() + HeaderSize(), JournalSizeValue);
	TArray<uint8> PayloadBytes;
	PayloadBytes.Append(
		Bytes.GetData() + HeaderSize() + JournalSizeValue,
		PayloadSizeValue);
	const int32 Generation = static_cast<int32>(GenerationValue);
	const FGuid ActualBundleDigest = DeriveBundleDigest(
		SchemaVersion,
		Generation,
		ExpectedJournalId,
		ExpectedEnvelopeId,
		JournalBytes,
		PayloadBytes);
	if (!ExpectedBundleDigest.IsValid()
		|| ExpectedBundleDigest != ActualBundleDigest)
	{
		return MakeDecodeResult(
			EDecodeStatus::BundleDigestMismatch,
			TEXT("Arc preview handoff recovery bundle digest does not match its exact sections."),
			SchemaVersion);
	}
	if (!ExpectedBundleId.IsValid()
		|| ExpectedBundleId != DeriveBundleId(
			SchemaVersion,
			Generation,
			ExpectedJournalId,
			ExpectedEnvelopeId,
			ExpectedBundleDigest))
	{
		return MakeDecodeResult(
			EDecodeStatus::BundleIdentityMismatch,
			TEXT("Arc preview handoff recovery bundle identity is invalid."),
			SchemaVersion);
	}

	const auto JournalDecoded = FJournalCodec::Decode(JournalBytes);
	if (!JournalDecoded.IsSuccess()
		|| JournalDecoded.WasMigrated()
		|| JournalDecoded.GetStatus() != EJournalDecodeStatus::DecodedCurrent)
	{
		return MakeDecodeResult(
			EDecodeStatus::JournalDecodeRejected,
			TEXT("Arc preview handoff recovery bundle journal is not canonical current evidence."),
			SchemaVersion,
			JournalDecoded.GetStatus());
	}
	const auto PayloadDecoded = FPayloadCodec::Decode(PayloadBytes);
	if (!PayloadDecoded.IsSuccess())
	{
		return MakeDecodeResult(
			EDecodeStatus::PayloadDecodeRejected,
			TEXT("Arc preview handoff recovery bundle payload could not be decoded."),
			SchemaVersion,
			JournalDecoded.GetStatus(),
			PayloadDecoded.GetStatus());
	}
	if (JournalDecoded.GetJournal().GetJournalId() != ExpectedJournalId)
	{
		return MakeDecodeResult(
			EDecodeStatus::JournalIdentityMismatch,
			TEXT("Arc preview handoff recovery bundle journal identity does not match its header."),
			SchemaVersion,
			JournalDecoded.GetStatus(),
			PayloadDecoded.GetStatus());
	}
	if (PayloadDecoded.GetEnvelope().GetEnvelopeId() != ExpectedEnvelopeId)
	{
		return MakeDecodeResult(
			EDecodeStatus::EnvelopeIdentityMismatch,
			TEXT("Arc preview handoff recovery bundle envelope identity does not match its header."),
			SchemaVersion,
			JournalDecoded.GetStatus(),
			PayloadDecoded.GetStatus());
	}

	FBundle Bundle;
	if (!FBundle::TryCreate(
			JournalDecoded.GetJournal(),
			PayloadDecoded.GetEnvelope(),
			Bundle)
		|| Bundle.GetGeneration() != Generation)
	{
		return MakeDecodeResult(
			EDecodeStatus::PairingRejected,
			TEXT("Arc preview handoff recovery bundle sections are not one pending generation."),
			SchemaVersion,
			JournalDecoded.GetStatus(),
			PayloadDecoded.GetStatus());
	}
	if (Bundle.GetBundleId() != ExpectedBundleId
		|| Bundle.GetBundleDigest() != ExpectedBundleDigest)
	{
		return MakeDecodeResult(
			EDecodeStatus::BundleIdentityMismatch,
			TEXT("Arc preview handoff recovery bundle reconstructed identity does not match."),
			SchemaVersion,
			JournalDecoded.GetStatus(),
			PayloadDecoded.GetStatus());
	}
	TArray<uint8> Reencoded;
	if (!TryEncode(Bundle, Reencoded) || Reencoded != Bytes)
	{
		return MakeDecodeResult(
			EDecodeStatus::NonCanonicalRepresentation,
			TEXT("Arc preview handoff recovery bundle input is not canonical."),
			SchemaVersion,
			JournalDecoded.GetStatus(),
			PayloadDecoded.GetStatus());
	}
	return MakeDecodeResult(
		EDecodeStatus::Decoded,
		TEXT("Decoded one canonical generation-bound Arc preview recovery bundle."),
		SchemaVersion,
		JournalDecoded.GetStatus(),
		PayloadDecoded.GetStatus(),
		Bundle);
}
