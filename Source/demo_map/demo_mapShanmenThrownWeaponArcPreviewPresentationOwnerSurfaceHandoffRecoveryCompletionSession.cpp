#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.h"

#include "Misc/Paths.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using FRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest;
	using FCompletion =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion;
	using FCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionCodec;
	using FDecodeResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeResult;
	using EDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus;
	using FAdmissionRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest;
	using FAdmissionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using FJournalRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;
	using FJournalCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;
	using EJournalDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus;
	using EJournalDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using EJournalRecordKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;

	constexpr uint8 CompletionMagic[] =
	{
		'S', 'M', 'A', 'R', 'C', 'D', 'O', 'N'
	};

	FGuid MakeRequestId(
		const FGuid& CompletionAuthorityDomainId,
		const FAdmissionRequest& AdmissionRequest)
	{
		if (!CompletionAuthorityDomainId.IsValid()
			|| !AdmissionRequest.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryCompletionRequest.r1"),
			{
				CompletionAuthorityDomainId.ToString(EGuidFormats::Digits),
				AdmissionRequest.GetRequestId().ToString(EGuidFormats::Digits),
				AdmissionRequest.GetAuthorityDomainId().ToString(
					EGuidFormats::Digits),
				AdmissionRequest.GetLineageId().ToString(EGuidFormats::Digits),
				AdmissionRequest.GetExpectedJournalId().ToString(
					EGuidFormats::Digits),
				AdmissionRequest.GetExpectedCheckpointId().ToString(
					EGuidFormats::Digits)
			});
	}

	bool JournalsMatch(const FJournal& Left, const FJournal& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.GetJournalId() != Right.GetJournalId()
			|| Left.GetRecordCount() != Right.GetRecordCount())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.GetRecordCount(); ++Index)
		{
			FJournalRecord LeftRecord;
			FJournalRecord RightRecord;
			if (!Left.TryGetRecordAt(Index, LeftRecord)
				|| !Right.TryGetRecordAt(Index, RightRecord)
				|| !LeftRecord.Matches(RightRecord))
			{
				return false;
			}
		}
		return true;
	}

	int32 PendingGeneration(const FJournal& Journal)
	{
		if (!Journal.IsValid()
			|| Journal.GetLatestDisposition()
				!= EJournalDisposition::CheckpointPending
			|| Journal.GetRecordCount() <= 0
			|| Journal.GetRecordCount() % 2 == 0)
		{
			return 0;
		}
		const int32 Generation = (Journal.GetRecordCount() + 1) / 2;
		return Generation >= 1
			&& Generation
				<= Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle::
					MaximumGeneration()
			? Generation
			: 0;
	}

	int32 TerminalGeneration(const FJournal& Journal)
	{
		if (!Journal.IsValid()
			|| Journal.GetLatestDisposition()
				!= EJournalDisposition::RecoveryCommitted
			|| Journal.GetRecordCount() <= 0
			|| Journal.GetRecordCount() % 2 != 0)
		{
			return 0;
		}
		const int32 Generation = Journal.GetRecordCount() / 2;
		return Generation >= 1
			&& Generation
				<= Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle::
					MaximumGeneration()
			? Generation
			: 0;
	}

	bool IsTerminalExtension(
		const FJournal& Source,
		const FJournal& Terminal,
		const FGuid& CheckpointId,
		const FGuid& RecoveryReceiptId)
	{
		const int32 Generation = PendingGeneration(Source);
		if (Generation <= 0
			|| TerminalGeneration(Terminal) != Generation
			|| Terminal.GetRecordCount() != Source.GetRecordCount() + 1
			|| !CheckpointId.IsValid() || !RecoveryReceiptId.IsValid())
		{
			return false;
		}
		for (int32 Index = 0; Index < Source.GetRecordCount(); ++Index)
		{
			FJournalRecord SourceRecord;
			FJournalRecord TerminalRecord;
			if (!Source.TryGetRecordAt(Index, SourceRecord)
				|| !Terminal.TryGetRecordAt(Index, TerminalRecord)
				|| !SourceRecord.Matches(TerminalRecord))
			{
				return false;
			}
		}
		FJournalRecord SourceLatest;
		FJournalRecord TerminalLatest;
		return Source.TryGetLatestRecord(SourceLatest)
			&& Terminal.TryGetLatestRecord(TerminalLatest)
			&& SourceLatest.GetKind()
				== EJournalRecordKind::CheckpointPrepared
			&& SourceLatest.GetCheckpointId() == CheckpointId
			&& TerminalLatest.GetKind()
				== EJournalRecordKind::RecoveryCommitted
			&& TerminalLatest.GetCheckpointId() == CheckpointId
			&& TerminalLatest.GetRecoveryReceiptId() == RecoveryReceiptId
			&& TerminalLatest.GetPreviousRecordId()
				== SourceLatest.GetRecordId();
	}

	bool TryBuildJournalBytes(
		const FJournal& SourceJournal,
		const FJournal& TerminalJournal,
		TArray<uint8>& OutSourceBytes,
		TArray<uint8>& OutTerminalBytes)
	{
		OutSourceBytes.Reset();
		OutTerminalBytes.Reset();
		return SourceJournal.IsValid() && TerminalJournal.IsValid()
			&& FJournalCodec::TryEncode(SourceJournal, OutSourceBytes)
			&& FJournalCodec::TryEncode(TerminalJournal, OutTerminalBytes)
			&& OutSourceBytes.Num() >= FJournalCodec::HeaderSize()
			&& OutSourceBytes.Num() <= FCodec::MaximumJournalEncodedBytes()
			&& OutTerminalBytes.Num() >= FJournalCodec::HeaderSize()
			&& OutTerminalBytes.Num() <= FCodec::MaximumJournalEncodedBytes();
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

	FGuid MakeCompletionDigest(
		const int32 SchemaVersion,
		const int32 Generation,
		const FRequest& Request,
		const FGuid& SourceBundleId,
		const FGuid& RecoveryReceiptId,
		const FGuid& TerminalJournalId,
		const TArray<uint8>& SourceBytes,
		const TArray<uint8>& TerminalBytes)
	{
		if (SchemaVersion != FCodec::CurrentSchemaVersion()
			|| Generation < 1
			|| Generation
				> Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle::
					MaximumGeneration()
			|| !Request.IsValid() || !SourceBundleId.IsValid()
			|| !RecoveryReceiptId.IsValid() || !TerminalJournalId.IsValid()
			|| SourceBytes.IsEmpty() || TerminalBytes.IsEmpty())
		{
			return FGuid();
		}
		const FAdmissionRequest& Admission = Request.GetAdmissionRequest();
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryCompletionDigest.r1"),
			{
				FString::FromInt(SchemaVersion),
				FString::FromInt(Generation),
				Request.GetCompletionAuthorityDomainId().ToString(
					EGuidFormats::Digits),
				Admission.GetAuthorityDomainId().ToString(EGuidFormats::Digits),
				Admission.GetLineageId().ToString(EGuidFormats::Digits),
				Admission.GetExpectedJournalId().ToString(EGuidFormats::Digits),
				Admission.GetExpectedCheckpointId().ToString(
					EGuidFormats::Digits),
				Admission.GetRequestId().ToString(EGuidFormats::Digits),
				Request.GetRequestId().ToString(EGuidFormats::Digits),
				SourceBundleId.ToString(EGuidFormats::Digits),
				RecoveryReceiptId.ToString(EGuidFormats::Digits),
				TerminalJournalId.ToString(EGuidFormats::Digits),
				FString::FromInt(SourceBytes.Num()),
				BytesKey(SourceBytes),
				FString::FromInt(TerminalBytes.Num()),
				BytesKey(TerminalBytes)
			});
	}

	FGuid MakeCompletionId(
		const int32 SchemaVersion,
		const int32 Generation,
		const FRequest& Request,
		const FGuid& SourceBundleId,
		const FGuid& RecoveryReceiptId,
		const FGuid& TerminalJournalId,
		const FGuid& CompletionDigest)
	{
		if (!CompletionDigest.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryCompletion.r1"),
			{
				FString::FromInt(SchemaVersion),
				FString::FromInt(Generation),
				Request.GetRequestId().ToString(EGuidFormats::Digits),
				SourceBundleId.ToString(EGuidFormats::Digits),
				RecoveryReceiptId.ToString(EGuidFormats::Digits),
				TerminalJournalId.ToString(EGuidFormats::Digits),
				CompletionDigest.ToString(EGuidFormats::Digits)
			});
	}

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
}

bool FRequest::TryCreate(
	const FGuid& InCompletionAuthorityDomainId,
	const FAdmissionRequest& InAdmissionRequest,
	FRequest& OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = {};
	OutDiagnostic.Reset();
	if (!InCompletionAuthorityDomainId.IsValid()
		|| !InAdmissionRequest.IsValid()
		|| InCompletionAuthorityDomainId
			== InAdmissionRequest.GetAuthorityDomainId())
	{
		OutDiagnostic = TEXT(
			"Recovery completion requires a valid authority domain distinct from the pending-bundle authority.");
		return false;
	}
	OutRequest.CompletionAuthorityDomainId = InCompletionAuthorityDomainId;
	OutRequest.AdmissionRequest = InAdmissionRequest;
	OutRequest.RequestId = MakeRequestId(
		InCompletionAuthorityDomainId, InAdmissionRequest);
	if (!OutRequest.IsValid())
	{
		OutRequest = {};
		OutDiagnostic = TEXT(
			"Recovery completion request failed deterministic validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Recovery completion request created for one exact admission transaction.");
	return true;
}

bool FRequest::IsValid() const
{
	return CompletionAuthorityDomainId.IsValid()
		&& AdmissionRequest.IsValid()
		&& CompletionAuthorityDomainId
			!= AdmissionRequest.GetAuthorityDomainId()
		&& RequestId.IsValid()
		&& RequestId == MakeRequestId(
			CompletionAuthorityDomainId, AdmissionRequest);
}

bool FCompletion::TryCreate(
	const FRequest& InRequest,
	const FAdmissionResult& AdmissionResult,
	const FJournal& InSourceJournal,
	const FJournal& InTerminalJournal,
	FCompletion& OutCompletion)
{
	OutCompletion = {};
	if (!InRequest.IsValid() || !AdmissionResult.IsValid()
		|| !AdmissionResult.IsSuccess()
		|| AdmissionResult.GetRequest().GetRequestId()
			!= InRequest.GetAdmissionRequest().GetRequestId()
		|| !AdmissionResult.GetRecoveryResult().HasReceipt()
		|| !AdmissionResult.GetCheckpoint().IsValid()
		|| AdmissionResult.GetCheckpoint().GetCheckpointId()
			!= InRequest.GetAdmissionRequest().GetExpectedCheckpointId()
		|| InSourceJournal.GetJournalId()
			!= InRequest.GetAdmissionRequest().GetExpectedJournalId())
	{
		return false;
	}
	const auto& Receipt = AdmissionResult.GetRecoveryResult().GetReceipt();
	const int32 Generation = PendingGeneration(InSourceJournal);
	if (Generation <= 0
		|| AdmissionResult.GetLoadedGeneration() != Generation
		|| !AdmissionResult.GetLoadedBundleId().IsValid()
		|| !Receipt.IsValid()
		|| !Receipt.MatchesCheckpoint(AdmissionResult.GetCheckpoint())
		|| !IsTerminalExtension(
			InSourceJournal,
			InTerminalJournal,
			AdmissionResult.GetCheckpoint().GetCheckpointId(),
			Receipt.GetReceiptId()))
	{
		return false;
	}

	FCompletion Candidate;
	Candidate.SchemaVersion = FCodec::CurrentSchemaVersion();
	Candidate.Generation = Generation;
	Candidate.Request = InRequest;
	Candidate.SourceBundleId = AdmissionResult.GetLoadedBundleId();
	Candidate.RecoveryReceiptId = Receipt.GetReceiptId();
	Candidate.SourceJournal = InSourceJournal;
	Candidate.TerminalJournal = InTerminalJournal;
	TArray<uint8> SourceBytes;
	TArray<uint8> TerminalBytes;
	if (!TryBuildJournalBytes(
			Candidate.SourceJournal,
			Candidate.TerminalJournal,
			SourceBytes,
			TerminalBytes))
	{
		return false;
	}
	Candidate.CompletionDigest = MakeCompletionDigest(
		Candidate.SchemaVersion,
		Candidate.Generation,
		Candidate.Request,
		Candidate.SourceBundleId,
		Candidate.RecoveryReceiptId,
		Candidate.TerminalJournal.GetJournalId(),
		SourceBytes,
		TerminalBytes);
	Candidate.CompletionId = MakeCompletionId(
		Candidate.SchemaVersion,
		Candidate.Generation,
		Candidate.Request,
		Candidate.SourceBundleId,
		Candidate.RecoveryReceiptId,
		Candidate.TerminalJournal.GetJournalId(),
		Candidate.CompletionDigest);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCompletion = MoveTemp(Candidate);
	return true;
}

bool FCompletion::IsValid() const
{
	TArray<uint8> SourceBytes;
	TArray<uint8> TerminalBytes;
	return SchemaVersion == FCodec::CurrentSchemaVersion()
		&& Generation == PendingGeneration(SourceJournal)
		&& Generation == TerminalGeneration(TerminalJournal)
		&& CompletionId.IsValid() && CompletionDigest.IsValid()
		&& Request.IsValid() && SourceBundleId.IsValid()
		&& RecoveryReceiptId.IsValid()
		&& Request.GetAdmissionRequest().GetExpectedJournalId()
			== SourceJournal.GetJournalId()
		&& IsTerminalExtension(
			SourceJournal,
			TerminalJournal,
			Request.GetAdmissionRequest().GetExpectedCheckpointId(),
			RecoveryReceiptId)
		&& TryBuildJournalBytes(
			SourceJournal, TerminalJournal, SourceBytes, TerminalBytes)
		&& CompletionDigest == MakeCompletionDigest(
			SchemaVersion,
			Generation,
			Request,
			SourceBundleId,
			RecoveryReceiptId,
			TerminalJournal.GetJournalId(),
			SourceBytes,
			TerminalBytes)
		&& CompletionId == MakeCompletionId(
			SchemaVersion,
			Generation,
			Request,
			SourceBundleId,
			RecoveryReceiptId,
			TerminalJournal.GetJournalId(),
			CompletionDigest);
}

bool FCompletion::Matches(const FCompletion& Other) const
{
	return IsValid() && Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& Generation == Other.Generation
		&& CompletionId == Other.CompletionId
		&& CompletionDigest == Other.CompletionDigest
		&& Request.GetRequestId() == Other.Request.GetRequestId()
		&& SourceBundleId == Other.SourceBundleId
		&& RecoveryReceiptId == Other.RecoveryReceiptId
		&& JournalsMatch(SourceJournal, Other.SourceJournal)
		&& JournalsMatch(TerminalJournal, Other.TerminalJournal);
}

bool FCompletion::MatchesRequest(const FRequest& OtherRequest) const
{
	return IsValid() && OtherRequest.IsValid()
		&& Request.GetRequestId() == OtherRequest.GetRequestId();
}

bool FCompletion::MatchesCurrentJournal(const FJournal& CurrentJournal) const
{
	return IsValid() && CurrentJournal.IsValid()
		&& (JournalsMatch(CurrentJournal, SourceJournal)
			|| JournalsMatch(CurrentJournal, TerminalJournal));
}

bool FDecodeResult::IsSuccess() const
{
	return Status == EDecodeStatus::Decoded
		&& SourceSchemaVersion == FCodec::CurrentSchemaVersion()
		&& SourceJournalDecodeStatus == EJournalDecodeStatus::DecodedCurrent
		&& TerminalJournalDecodeStatus == EJournalDecodeStatus::DecodedCurrent
		&& !Diagnostic.IsEmpty() && Completion.IsValid();
}

FDecodeResult FCodec::MakeDecodeResult(
	const EDecodeStatus Status,
	const TCHAR* Diagnostic,
	const int32 SourceSchemaVersion,
	const EJournalDecodeStatus SourceJournalDecodeStatus,
	const EJournalDecodeStatus TerminalJournalDecodeStatus,
	const FCompletion& Completion)
{
	FDecodeResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SourceSchemaVersion = SourceSchemaVersion;
	Result.SourceJournalDecodeStatus = SourceJournalDecodeStatus;
	Result.TerminalJournalDecodeStatus = TerminalJournalDecodeStatus;
	Result.Completion = Completion;
	return Result;
}

bool FCodec::TryEncode(
	const FCompletion& Completion,
	TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	TArray<uint8> SourceBytes;
	TArray<uint8> TerminalBytes;
	if (!Completion.IsValid()
		|| !TryBuildJournalBytes(
			Completion.GetSourceJournal(),
			Completion.GetTerminalJournal(),
			SourceBytes,
			TerminalBytes))
	{
		return false;
	}
	const int64 TotalSize = static_cast<int64>(HeaderSize())
		+ SourceBytes.Num() + TerminalBytes.Num();
	if (TotalSize > MaximumEncodedBytes()
		|| TotalSize > static_cast<int64>(MAX_uint32))
	{
		return false;
	}

	const FAdmissionRequest& Admission =
		Completion.GetRequest().GetAdmissionRequest();
	OutBytes.Reserve(static_cast<int32>(TotalSize));
	OutBytes.Append(CompletionMagic, UE_ARRAY_COUNT(CompletionMagic));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Completion.GetSchemaVersion()));
	AppendUint32BigEndian(OutBytes, static_cast<uint32>(TotalSize));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Completion.GetGeneration()));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(SourceBytes.Num()));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(TerminalBytes.Num()));
	AppendGuid(OutBytes, Completion.GetCompletionId());
	AppendGuid(OutBytes, Completion.GetCompletionDigest());
	AppendGuid(
		OutBytes,
		Completion.GetRequest().GetCompletionAuthorityDomainId());
	AppendGuid(OutBytes, Admission.GetAuthorityDomainId());
	AppendGuid(OutBytes, Admission.GetLineageId());
	AppendGuid(OutBytes, Admission.GetExpectedJournalId());
	AppendGuid(OutBytes, Admission.GetExpectedCheckpointId());
	AppendGuid(OutBytes, Admission.GetRequestId());
	AppendGuid(OutBytes, Completion.GetRequest().GetRequestId());
	AppendGuid(OutBytes, Completion.GetSourceBundleId());
	AppendGuid(OutBytes, Completion.GetRecoveryReceiptId());
	AppendGuid(OutBytes, Completion.GetTerminalJournal().GetJournalId());
	if (OutBytes.Num() != HeaderSize())
	{
		OutBytes.Reset();
		return false;
	}
	OutBytes.Append(SourceBytes);
	OutBytes.Append(TerminalBytes);
	return OutBytes.Num() == TotalSize;
}

FDecodeResult FCodec::Decode(const TArray<uint8>& Bytes)
{
	if (Bytes.IsEmpty())
	{
		return MakeDecodeResult(
			EDecodeStatus::InputEmpty,
			TEXT("Recovery completion input is empty."));
	}
	if (Bytes.Num() < HeaderSize() || Bytes.Num() > MaximumEncodedBytes())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Recovery completion size is outside the bounded schema."));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CompletionMagic); ++Index)
	{
		if (Bytes[Index] != CompletionMagic[Index])
		{
			return MakeDecodeResult(
				EDecodeStatus::MagicMismatch,
				TEXT("Recovery completion magic does not match."));
		}
	}

	int32 Offset = UE_ARRAY_COUNT(CompletionMagic);
	uint32 SchemaValue = 0;
	uint32 DeclaredSizeValue = 0;
	uint32 GenerationValue = 0;
	uint32 SourceSizeValue = 0;
	uint32 TerminalSizeValue = 0;
	FGuid ExpectedCompletionId;
	FGuid ExpectedCompletionDigest;
	FGuid CompletionAuthorityDomainId;
	FGuid PendingAuthorityDomainId;
	FGuid LineageId;
	FGuid ExpectedSourceJournalId;
	FGuid ExpectedCheckpointId;
	FGuid ExpectedAdmissionRequestId;
	FGuid ExpectedCompletionRequestId;
	FGuid SourceBundleId;
	FGuid RecoveryReceiptId;
	FGuid ExpectedTerminalJournalId;
	if (!TryReadUint32BigEndian(Bytes, Offset, SchemaValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, DeclaredSizeValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, GenerationValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, SourceSizeValue)
		|| !TryReadUint32BigEndian(Bytes, Offset, TerminalSizeValue)
		|| !TryReadGuid(Bytes, Offset, ExpectedCompletionId)
		|| !TryReadGuid(Bytes, Offset, ExpectedCompletionDigest)
		|| !TryReadGuid(Bytes, Offset, CompletionAuthorityDomainId)
		|| !TryReadGuid(Bytes, Offset, PendingAuthorityDomainId)
		|| !TryReadGuid(Bytes, Offset, LineageId)
		|| !TryReadGuid(Bytes, Offset, ExpectedSourceJournalId)
		|| !TryReadGuid(Bytes, Offset, ExpectedCheckpointId)
		|| !TryReadGuid(Bytes, Offset, ExpectedAdmissionRequestId)
		|| !TryReadGuid(Bytes, Offset, ExpectedCompletionRequestId)
		|| !TryReadGuid(Bytes, Offset, SourceBundleId)
		|| !TryReadGuid(Bytes, Offset, RecoveryReceiptId)
		|| !TryReadGuid(Bytes, Offset, ExpectedTerminalJournalId)
		|| Offset != HeaderSize())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Recovery completion header is truncated."));
	}
	if (SchemaValue != static_cast<uint32>(CurrentSchemaVersion()))
	{
		return MakeDecodeResult(
			EDecodeStatus::UnsupportedSchema,
			TEXT("Recovery completion schema is unsupported."),
			static_cast<int32>(SchemaValue));
	}
	const int32 SchemaVersion = static_cast<int32>(SchemaValue);
	if (GenerationValue < 1
		|| GenerationValue
			> static_cast<uint32>(
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle::
					MaximumGeneration()))
	{
		return MakeDecodeResult(
			EDecodeStatus::GenerationOutOfRange,
			TEXT("Recovery completion generation is outside the journal bound."),
			SchemaVersion);
	}
	if (SourceSizeValue < static_cast<uint32>(FJournalCodec::HeaderSize())
		|| SourceSizeValue > static_cast<uint32>(MaximumJournalEncodedBytes())
		|| TerminalSizeValue < static_cast<uint32>(FJournalCodec::HeaderSize())
		|| TerminalSizeValue > static_cast<uint32>(MaximumJournalEncodedBytes()))
	{
		return MakeDecodeResult(
			EDecodeStatus::SectionSizeOutOfRange,
			TEXT("Recovery completion journal section exceeds its bound."),
			SchemaVersion);
	}
	const int64 ExpectedSize = static_cast<int64>(HeaderSize())
		+ SourceSizeValue + TerminalSizeValue;
	if (DeclaredSizeValue != static_cast<uint32>(Bytes.Num())
		|| ExpectedSize != Bytes.Num())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Recovery completion sections do not exactly fill the input."),
			SchemaVersion);
	}

	FAdmissionRequest AdmissionRequest;
	FString Diagnostic;
	FRequest CompletionRequest;
	if (!FAdmissionRequest::TryCreate(
			PendingAuthorityDomainId,
			LineageId,
			ExpectedSourceJournalId,
			ExpectedCheckpointId,
			AdmissionRequest,
			Diagnostic)
		|| AdmissionRequest.GetRequestId() != ExpectedAdmissionRequestId
		|| !FRequest::TryCreate(
			CompletionAuthorityDomainId,
			AdmissionRequest,
			CompletionRequest,
			Diagnostic)
		|| CompletionRequest.GetRequestId() != ExpectedCompletionRequestId)
	{
		return MakeDecodeResult(
			EDecodeStatus::IdentityMismatch,
			TEXT("Recovery completion request identity is invalid."),
			SchemaVersion);
	}

	TArray<uint8> SourceBytes;
	SourceBytes.Append(Bytes.GetData() + HeaderSize(), SourceSizeValue);
	TArray<uint8> TerminalBytes;
	TerminalBytes.Append(
		Bytes.GetData() + HeaderSize() + SourceSizeValue,
		TerminalSizeValue);
	const auto SourceDecoded = FJournalCodec::Decode(SourceBytes);
	if (!SourceDecoded.IsSuccess() || SourceDecoded.WasMigrated())
	{
		return MakeDecodeResult(
			EDecodeStatus::SourceJournalRejected,
			TEXT("Recovery completion source journal is not canonical current evidence."),
			SchemaVersion,
			SourceDecoded.GetStatus());
	}
	const auto TerminalDecoded = FJournalCodec::Decode(TerminalBytes);
	if (!TerminalDecoded.IsSuccess() || TerminalDecoded.WasMigrated())
	{
		return MakeDecodeResult(
			EDecodeStatus::TerminalJournalRejected,
			TEXT("Recovery completion terminal journal is not canonical current evidence."),
			SchemaVersion,
			SourceDecoded.GetStatus(),
			TerminalDecoded.GetStatus());
	}
	if (SourceDecoded.GetJournal().GetJournalId()
			!= ExpectedSourceJournalId
		|| TerminalDecoded.GetJournal().GetJournalId()
			!= ExpectedTerminalJournalId)
	{
		return MakeDecodeResult(
			EDecodeStatus::JournalIdentityMismatch,
			TEXT("Recovery completion journal identities do not match the header."),
			SchemaVersion,
			SourceDecoded.GetStatus(),
			TerminalDecoded.GetStatus());
	}

	const int32 Generation = static_cast<int32>(GenerationValue);
	const FGuid ActualDigest = MakeCompletionDigest(
		SchemaVersion,
		Generation,
		CompletionRequest,
		SourceBundleId,
		RecoveryReceiptId,
		ExpectedTerminalJournalId,
		SourceBytes,
		TerminalBytes);
	if (!ExpectedCompletionDigest.IsValid()
		|| ExpectedCompletionDigest != ActualDigest)
	{
		return MakeDecodeResult(
			EDecodeStatus::DigestMismatch,
			TEXT("Recovery completion digest does not match its exact journals."),
			SchemaVersion,
			SourceDecoded.GetStatus(),
			TerminalDecoded.GetStatus());
	}
	if (!ExpectedCompletionId.IsValid()
		|| ExpectedCompletionId != MakeCompletionId(
			SchemaVersion,
			Generation,
			CompletionRequest,
			SourceBundleId,
			RecoveryReceiptId,
			ExpectedTerminalJournalId,
			ExpectedCompletionDigest))
	{
		return MakeDecodeResult(
			EDecodeStatus::IdentityMismatch,
			TEXT("Recovery completion identity is invalid."),
			SchemaVersion,
			SourceDecoded.GetStatus(),
			TerminalDecoded.GetStatus());
	}

	FCompletion Candidate;
	Candidate.SchemaVersion = SchemaVersion;
	Candidate.Generation = Generation;
	Candidate.CompletionId = ExpectedCompletionId;
	Candidate.CompletionDigest = ExpectedCompletionDigest;
	Candidate.Request = CompletionRequest;
	Candidate.SourceBundleId = SourceBundleId;
	Candidate.RecoveryReceiptId = RecoveryReceiptId;
	Candidate.SourceJournal = SourceDecoded.GetJournal();
	Candidate.TerminalJournal = TerminalDecoded.GetJournal();
	if (!Candidate.IsValid())
	{
		return MakeDecodeResult(
			EDecodeStatus::CompletionRejected,
			TEXT("Recovery completion source/terminal relation is invalid."),
			SchemaVersion,
			SourceDecoded.GetStatus(),
			TerminalDecoded.GetStatus());
	}
	TArray<uint8> Canonical;
	if (!TryEncode(Candidate, Canonical) || Canonical != Bytes)
	{
		return MakeDecodeResult(
			EDecodeStatus::NonCanonicalRepresentation,
			TEXT("Recovery completion bytes are not canonical."),
			SchemaVersion,
			SourceDecoded.GetStatus(),
			TerminalDecoded.GetStatus());
	}
	return MakeDecodeResult(
		EDecodeStatus::Decoded,
		TEXT("Recovery completion decoded as canonical terminal evidence."),
		SchemaVersion,
		SourceDecoded.GetStatus(),
		TerminalDecoded.GetStatus(),
		Candidate);
}

namespace
{
	using FStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext;
	using FStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter;
	using IFileSystem =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem;
	using EFileReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileReadStatus;
	using EFileWriteStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileWriteStatus;
	using ESaveStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus;
	using ELoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus;

	constexpr TCHAR CompletionStorageDirectoryName[] =
		TEXT("ShanmenArcPreviewRecoveryCompletions");
	constexpr TCHAR CompletionPrimaryExtension[] = TEXT(".smarc-completion");
	constexpr TCHAR CompletionTemporaryExtension[] = TEXT(".tmp");

	bool CompletionPathHasEmbeddedNull(const FString& Value)
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

	FString NormalizedCompletionDirectory(const FString& Value)
	{
		FString Result = Value;
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}

	bool IsValidCompletionMinimumGeneration(const int32 Generation)
	{
		return Generation >= 1
			&& Generation
				<= Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle::
					MaximumGeneration();
	}

	bool IsVerifiedCompletionBytes(
		const TArray<uint8>& Bytes,
		const FCompletion& Expected)
	{
		const auto Decoded = FCodec::Decode(Bytes);
		return Decoded.IsSuccess()
			&& Decoded.GetCompletion().Matches(Expected);
	}
}

bool FStorageContext::TryCreate(
	const FString& AbsoluteRootDirectory,
	const FGuid& InExpectedLineageId,
	FStorageContext& OutContext,
	FString& OutDiagnostic)
{
	OutContext = {};
	OutDiagnostic.Reset();
	const FString Trimmed = AbsoluteRootDirectory.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || CompletionPathHasEmbeddedNull(Trimmed)
		|| Trimmed.Len() > MaximumRootDirectoryCharacters()
		|| FPaths::IsRelative(Trimmed))
	{
		OutDiagnostic = TEXT(
			"Recovery completion storage requires one bounded absolute caller-owned root.");
		return false;
	}
	if (!InExpectedLineageId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Recovery completion storage requires one valid expected lineage identity.");
		return false;
	}

	FString NormalizedRoot = FPaths::ConvertRelativePathToFull(Trimmed);
	FPaths::NormalizeDirectoryName(NormalizedRoot);
	if (NormalizedRoot.IsEmpty()
		|| NormalizedRoot.Len() > MaximumRootDirectoryCharacters())
	{
		OutDiagnostic = TEXT(
			"Recovery completion storage root could not be normalized safely.");
		return false;
	}
	const FString Directory = NormalizedCompletionDirectory(
		FPaths::Combine(NormalizedRoot, CompletionStorageDirectoryName));
	const FString Filename =
		InExpectedLineageId.ToString(EGuidFormats::Digits)
		+ CompletionPrimaryExtension;
	const FString Primary = FPaths::Combine(Directory, Filename);
	const FString Temporary = Primary + CompletionTemporaryExtension;
	if (Directory.IsEmpty() || Primary.IsEmpty() || Temporary.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Recovery completion storage paths could not be derived.");
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
			"Recovery completion storage context failed canonical validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Recovery completion storage context created for one stable lineage slot.");
	return true;
}

bool FStorageContext::IsValid() const
{
	if (RootDirectory.IsEmpty()
		|| CompletionPathHasEmbeddedNull(RootDirectory)
		|| RootDirectory.Len() > MaximumRootDirectoryCharacters()
		|| FPaths::IsRelative(RootDirectory)
		|| !ExpectedLineageId.IsValid())
	{
		return false;
	}
	const FString ExpectedDirectory = NormalizedCompletionDirectory(
		FPaths::Combine(RootDirectory, CompletionStorageDirectoryName));
	const FString ExpectedPrimary = FPaths::Combine(
		ExpectedDirectory,
		ExpectedLineageId.ToString(EGuidFormats::Digits)
			+ CompletionPrimaryExtension);
	return StorageDirectory == ExpectedDirectory
		&& PrimaryPath == ExpectedPrimary
		&& TemporaryPath == ExpectedPrimary + CompletionTemporaryExtension
		&& FPaths::GetPath(PrimaryPath) == FPaths::GetPath(TemporaryPath);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveResult::
IsSuccess() const
{
	return Status == ESaveStatus::Saved
		|| Status == ESaveStatus::AlreadyCurrent;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveResult::
WasAlreadyCurrent() const
{
	return Status == ESaveStatus::AlreadyCurrent;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadResult::
IsSuccess() const
{
	return Status == ELoadStatus::Loaded;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveResult
FStorageAdapter::Save(
	const FStorageContext& Context,
	const FCompletion& Completion,
	const int32 MinimumGeneration,
	IFileSystem& FileSystem) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveResult
		Result;
	Result.CompletionId = Completion.GetCompletionId();
	Result.Generation = Completion.GetGeneration();
	Result.MinimumGeneration = MinimumGeneration;
	if (!Context.IsValid())
	{
		Result.Status = ESaveStatus::ContextRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion save rejected an invalid storage context.");
		return Result;
	}
	if (!Completion.IsValid())
	{
		Result.Status = ESaveStatus::CompletionRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion save rejected invalid canonical evidence.");
		return Result;
	}
	Result.LineageId =
		Completion.GetRequest().GetAdmissionRequest().GetLineageId();
	if (Result.LineageId != Context.GetExpectedLineageId())
	{
		Result.Status = ESaveStatus::LineageSlotMismatch;
		Result.Diagnostic = TEXT(
			"Recovery completion does not belong to the caller lineage slot.");
		return Result;
	}
	if (!IsValidCompletionMinimumGeneration(MinimumGeneration))
	{
		Result.Status = ESaveStatus::MinimumGenerationRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion save requires a trusted minimum generation from 1 through 8.");
		return Result;
	}
	if (Completion.GetGeneration() < MinimumGeneration)
	{
		Result.Status = ESaveStatus::GenerationBelowWatermark;
		Result.Diagnostic = TEXT(
			"Recovery completion generation is below the trusted completion watermark.");
		return Result;
	}

	TArray<uint8> Encoded;
	if (!FCodec::TryEncode(Completion, Encoded)
		|| Encoded.IsEmpty()
		|| Encoded.Num() > FCodec::MaximumEncodedBytes())
	{
		Result.Status = ESaveStatus::EncodingRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion could not be canonically encoded.");
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
				"Existing recovery completion exceeds the bounded codec size.");
			return Result;
		}
		if (ExistingRead != EFileReadStatus::Read)
		{
			Result.Status = ESaveStatus::ExistingReadFailed;
			Result.Diagnostic = TEXT(
				"Existing recovery completion could not be read and was not replaced.");
			return Result;
		}
		const auto ExistingDecoded = FCodec::Decode(ExistingBytes);
		if (!ExistingDecoded.IsSuccess())
		{
			Result.Status = ESaveStatus::ExistingDecodeRejected;
			Result.Diagnostic = TEXT(
				"Existing recovery completion could not be verified and was not replaced.");
			return Result;
		}
		const FCompletion& Existing = ExistingDecoded.GetCompletion();
		if (Existing.GetRequest().GetAdmissionRequest().GetLineageId()
			!= Context.GetExpectedLineageId())
		{
			Result.Status = ESaveStatus::ExistingLineageMismatch;
			Result.Diagnostic = TEXT(
				"Existing recovery completion belongs to another lineage.");
			return Result;
		}
		if (Existing.GetGeneration() > Completion.GetGeneration())
		{
			Result.Status = ESaveStatus::ExistingGenerationNewer;
			Result.Diagnostic = TEXT(
				"Existing recovery completion is newer than the candidate.");
			return Result;
		}
		if (Existing.GetGeneration() == Completion.GetGeneration())
		{
			if (Existing.Matches(Completion) && ExistingBytes == Encoded)
			{
				Result.Status = ESaveStatus::AlreadyCurrent;
				Result.Diagnostic = TEXT(
					"Recovery completion slot already contains identical evidence.");
				return Result;
			}
			Result.Status = ESaveStatus::ExistingGenerationConflict;
			Result.Diagnostic = TEXT(
				"Recovery completion conflicts with another completion in the same generation.");
			return Result;
		}
	}

	if (!FileSystem.DirectoryExists(Context.GetStorageDirectory())
		&& !FileSystem.CreateDirectoryTree(Context.GetStorageDirectory()))
	{
		Result.Status = ESaveStatus::DirectoryCreationFailed;
		Result.Diagnostic = TEXT(
			"Recovery completion storage directory could not be created.");
		return Result;
	}
	if (FileSystem.FileExists(Context.GetTemporaryPath())
		&& !FileSystem.DeleteFile(Context.GetTemporaryPath()))
	{
		Result.Status = ESaveStatus::StaleTemporaryCleanupFailed;
		Result.Diagnostic = TEXT(
			"Stale recovery completion temporary file could not be removed.");
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
				"Recovery completion temporary file could not be opened.");
			break;
		case EFileWriteStatus::WriteFailed:
			Result.Status = ESaveStatus::TemporaryWriteFailed;
			Result.Diagnostic = TEXT(
				"Recovery completion temporary write failed.");
			break;
		case EFileWriteStatus::FlushFailed:
			Result.Status = ESaveStatus::TemporaryFlushFailed;
			Result.Diagnostic = TEXT(
				"Recovery completion temporary full flush failed.");
			break;
		default:
			Result.Status = ESaveStatus::TemporaryWriteFailed;
			Result.Diagnostic = TEXT(
				"Recovery completion temporary write returned an invalid status.");
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
			"Recovery completion temporary file could not be read back.");
		return Result;
	}
	if (TemporaryRead != EFileReadStatus::Read
		|| TemporarySize != Encoded.Num()
		|| TemporaryBytes != Encoded
		|| !IsVerifiedCompletionBytes(TemporaryBytes, Completion))
	{
		Result.Status = ESaveStatus::TemporaryValidationFailed;
		Result.Diagnostic = TEXT(
			"Recovery completion temporary bytes failed exact verification.");
		return Result;
	}

	if (!FileSystem.AtomicReplace(
		Context.GetPrimaryPath(), Context.GetTemporaryPath()))
	{
		Result.Status = ESaveStatus::AtomicReplaceFailed;
		Result.Diagnostic = TEXT(
			"Recovery completion same-volume replacement failed.");
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
			"Committed recovery completion could not be read back.");
		return Result;
	}
	if (CommittedRead != EFileReadStatus::Read
		|| CommittedSize != Encoded.Num()
		|| CommittedBytes != Encoded
		|| !IsVerifiedCompletionBytes(CommittedBytes, Completion))
	{
		Result.Status = ESaveStatus::CommittedValidationFailed;
		Result.Diagnostic = TEXT(
			"Committed recovery completion failed exact verification.");
		return Result;
	}

	Result.Status = ESaveStatus::Saved;
	Result.Diagnostic = TEXT(
		"Recovery completion atomically replaced and verified before authority advance.");
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadResult
FStorageAdapter::Load(
	const FStorageContext& Context,
	const int32 MinimumGeneration,
	const IFileSystem& FileSystem) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadResult
		Result;
	Result.MinimumGeneration = MinimumGeneration;
	if (!Context.IsValid())
	{
		Result.Status = ELoadStatus::ContextRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion load rejected an invalid storage context.");
		return Result;
	}
	if (!IsValidCompletionMinimumGeneration(MinimumGeneration))
	{
		Result.Status = ELoadStatus::MinimumGenerationRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion load requires a trusted minimum generation from 1 through 8.");
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
			"Recovery completion primary does not exist.");
		return Result;
	}
	if (ReadStatus == EFileReadStatus::TooLarge)
	{
		Result.Status = ELoadStatus::SizeRejected;
		Result.Diagnostic = TEXT(
			"Recovery completion primary exceeds the bounded codec size.");
		return Result;
	}
	if (ReadStatus != EFileReadStatus::Read)
	{
		Result.Status = ELoadStatus::ReadFailed;
		Result.Diagnostic = TEXT(
			"Recovery completion primary could not be read.");
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
	Result.Generation = Decoded.GetCompletion().GetGeneration();
	Result.LineageId = Decoded.GetCompletion()
		.GetRequest().GetAdmissionRequest().GetLineageId();
	if (Result.LineageId != Context.GetExpectedLineageId())
	{
		Result.Status = ELoadStatus::LineageSlotMismatch;
		Result.Diagnostic = TEXT(
			"Decoded recovery completion belongs to another lineage.");
		return Result;
	}
	if (Result.Generation < MinimumGeneration)
	{
		Result.Status = ELoadStatus::GenerationBelowWatermark;
		Result.Diagnostic = TEXT(
			"Decoded recovery completion is below the trusted completion watermark.");
		return Result;
	}

	Result.Status = ELoadStatus::Loaded;
	Result.Diagnostic = TEXT(
		"Recovery completion loaded as exact terminal evidence.");
	Result.Completion = Decoded.GetCompletion();
	return Result;
}

namespace
{
	using FSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionResult;
	using FSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession;
	using ESessionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionStatus;
	using EAuthorityReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus;
	using EAuthorityAdvanceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus;
	using FAuthorityAdvanceRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest;
	using IAuthority =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority;
	using FPendingStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext;

	int32 CurrentJournalGeneration(const FJournal& Journal)
	{
		const int32 Pending = PendingGeneration(Journal);
		return Pending > 0 ? Pending : TerminalGeneration(Journal);
	}
}

bool FSessionResult::IsSuccess() const
{
	return IsValid()
		&& (Status == ESessionStatus::Completed
			|| Status == ESessionStatus::CompletedAfterAuthorityRecheck
			|| Status == ESessionStatus::Replayed);
}

bool FSessionResult::IsReplay() const
{
	return IsValid() && Status == ESessionStatus::Replayed;
}

bool FSessionResult::DidMutateSurface() const
{
	return bAdmissionInvoked && AdmissionResult.IsValid()
		&& AdmissionResult.DidMutateSurface();
}

bool FSessionResult::Validate() const
{
	if (Status == ESessionStatus::Invalid || Diagnostic.IsEmpty()
		|| DidMutateSurface())
	{
		return false;
	}
	if (Status == ESessionStatus::RequestRejected)
	{
		return !Request.IsValid() || TargetGeneration == 0;
	}
	if (!Request.IsValid())
	{
		return false;
	}
	if (Status == ESessionStatus::OperationInProgress)
	{
		return !bAdmissionInvoked && !bCompletionVerified
			&& !bAuthorityCurrent;
	}
	if (Status == ESessionStatus::CurrentJournalRejected)
	{
		return !bAdmissionInvoked && !bCompletionVerified
			&& !bAuthorityCurrent;
	}
	if (TargetGeneration < 1
		|| TargetGeneration
			> Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle::
				MaximumGeneration())
	{
		return false;
	}
	if (Status == ESessionStatus::Replayed)
	{
		return !bAdmissionInvoked && bCompletionVerified
			&& bAuthorityCurrent && Completion.IsValid()
			&& Completion.MatchesRequest(Request)
			&& Completion.GetGeneration() == TargetGeneration
			&& JournalsMatch(
				TerminalJournal, Completion.GetTerminalJournal());
	}
	if (Status == ESessionStatus::Completed
		|| Status == ESessionStatus::CompletedAfterAuthorityRecheck)
	{
		return bAdmissionInvoked && AdmissionResult.IsSuccess()
			&& JournalAppendResult.IsSuccess()
			&& Completion.IsValid() && Completion.MatchesRequest(Request)
			&& Completion.GetGeneration() == TargetGeneration
			&& JournalsMatch(
				TerminalJournal, Completion.GetTerminalJournal())
			&& bCompletionVerified && bAuthorityCurrent
			&& (CompletionSaveStatus == ESaveStatus::Saved
				|| CompletionSaveStatus == ESaveStatus::AlreadyCurrent)
			&& CompletionLoadStatus == ELoadStatus::Loaded;
	}
	if (Status
			== ESessionStatus::CompletionEvidenceCommittedAuthorityPending
		|| Status == ESessionStatus::CompletionOutcomeUnresolved)
	{
		return bAdmissionInvoked && AdmissionResult.IsSuccess()
			&& JournalAppendResult.IsSuccess()
			&& Completion.IsValid() && bCompletionVerified
			&& !bAuthorityCurrent;
	}
	return !bAuthorityCurrent;
}

FSessionResult FSession::ExecuteExplicit(
	const FRequest& Request,
	const FPendingStorageContext& PendingStorageContext,
	const FStorageContext& CompletionStorageContext,
	const FJournal& CurrentJournal,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner& Owner,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
		ExpectedRetiredSurface,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface& NewSurface,
	IFileSystem& FileSystem,
	IAuthority& Authority)
{
	FSessionResult Result;
	Result.Request = Request;
	auto Finish = [&Result](
		const ESessionStatus Status,
		const TCHAR* Diagnostic) -> FSessionResult
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.bValidated = Result.Validate();
		return Result;
	};

	if (!Request.IsValid()
		|| !PendingStorageContext.IsValid()
		|| !CompletionStorageContext.IsValid()
		|| PendingStorageContext.GetExpectedLineageId()
			!= Request.GetAdmissionRequest().GetLineageId()
		|| CompletionStorageContext.GetExpectedLineageId()
			!= Request.GetAdmissionRequest().GetLineageId())
	{
		return Finish(
			ESessionStatus::RequestRejected,
			TEXT("Recovery completion rejected invalid or cross-lineage input before callbacks."));
	}
	Result.TargetGeneration = CurrentJournalGeneration(CurrentJournal);
	if (Result.TargetGeneration <= 0)
	{
		return Finish(
			ESessionStatus::CurrentJournalRejected,
			TEXT("Recovery completion requires a valid pending or terminal journal generation."));
	}
	if (bOperationInProgress)
	{
		return Finish(
			ESessionStatus::OperationInProgress,
			TEXT("Recovery completion rejected callback re-entry on the same Session."));
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const FGuid& CompletionAuthorityDomainId =
		Request.GetCompletionAuthorityDomainId();
	const FGuid& LineageId =
		Request.GetAdmissionRequest().GetLineageId();
	const auto AuthorityRead = Authority.Read(
		CompletionAuthorityDomainId, LineageId);
	Result.CompletionAuthorityReadStatus = AuthorityRead.GetStatus();
	if (!AuthorityRead.IsValid()
		|| AuthorityRead.GetStatus() == EAuthorityReadStatus::Rejected)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityReadRejected,
			TEXT("Completion authority returned a rejected or malformed read."));
	}
	if (AuthorityRead.GetStatus() == EAuthorityReadStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityUnavailable,
			TEXT("Completion authority is unavailable before recovery admission."));
	}

	Result.PreviousCompletionGeneration = 0;
	if (AuthorityRead.GetStatus() == EAuthorityReadStatus::Current)
	{
		const auto& State = AuthorityRead.GetState();
		if (!State.IsValid()
			|| State.GetAuthorityDomainId() != CompletionAuthorityDomainId
			|| State.GetLineageId() != LineageId)
		{
			return Finish(
				ESessionStatus::CompletionAuthorityStateRejected,
				TEXT("Completion authority state does not match the requested domain and lineage."));
		}
		Result.PreviousCompletionGeneration = State.GetGeneration();
		if (State.GetGeneration() > Result.TargetGeneration)
		{
			return Finish(
				ESessionStatus::CompletionAuthorityAhead,
				TEXT("Trusted completion watermark is ahead of the caller journal generation."));
		}
		if (State.GetGeneration() == Result.TargetGeneration)
		{
			FStorageAdapter Storage;
			const auto Loaded = Storage.Load(
				CompletionStorageContext,
				State.GetGeneration(),
				FileSystem);
			Result.CompletionLoadStatus = Loaded.GetStatus();
			if (!Loaded.IsSuccess())
			{
				return Finish(
					ESessionStatus::TrustedCompletionLoadRejected,
					TEXT("Trusted completion could not be loaded; recovery remains closed."));
			}
			Result.Completion = Loaded.GetCompletion();
			if (Loaded.GetGeneration() != Result.TargetGeneration
				|| State.GetBundleId()
					!= Result.Completion.GetCompletionId()
				|| !Result.Completion.MatchesRequest(Request)
				|| !Result.Completion.MatchesCurrentJournal(CurrentJournal))
			{
				return Finish(
					ESessionStatus::TrustedCompletionMismatch,
					TEXT("Trusted completion identity or journal evidence does not match caller intent."));
			}
			Result.TerminalJournal =
				Result.Completion.GetTerminalJournal();
			Result.bCompletionVerified = true;
			Result.bAuthorityCurrent = true;
			return Finish(
				ESessionStatus::Replayed,
				TEXT("Exact trusted completion replayed without invoking recovery admission."));
		}
	}

	if (CurrentJournal.GetLatestDisposition()
			!= EJournalDisposition::CheckpointPending
		|| CurrentJournal.GetJournalId()
			!= Request.GetAdmissionRequest().GetExpectedJournalId()
		|| PendingGeneration(CurrentJournal) != Result.TargetGeneration)
	{
		return Finish(
			ESessionStatus::CurrentJournalRejected,
			TEXT("A new completion requires the exact caller-selected pending journal."));
	}

	Result.bAdmissionInvoked = true;
	Result.AdmissionResult = AdmissionSession.ExecuteExplicit(
		Request.GetAdmissionRequest(),
		PendingStorageContext,
		CurrentJournal,
		Owner,
		ExpectedRetiredSurface,
		NewSurface,
		FileSystem,
		Authority);
	if (!Result.AdmissionResult.IsValid()
		|| !Result.AdmissionResult.IsSuccess()
		|| Result.AdmissionResult.GetLoadedGeneration()
			!= Result.TargetGeneration
		|| !Result.AdmissionResult.GetRecoveryResult().HasReceipt())
	{
		return Finish(
			ESessionStatus::AdmissionRejected,
			TEXT("P20.55 did not produce an exact successful recovery admission receipt."));
	}

	Result.TerminalJournal = CurrentJournal;
	Result.JournalAppendResult = Result.TerminalJournal.AppendRecoveryReceipt(
		Result.AdmissionResult.GetCheckpoint(),
		Result.AdmissionResult.GetRecoveryResult().GetReceipt());
	if (!Result.JournalAppendResult.IsSuccess()
		|| Result.TerminalJournal.GetLatestDisposition()
			!= EJournalDisposition::RecoveryCommitted)
	{
		return Finish(
			ESessionStatus::JournalAppendRejected,
			TEXT("Recovery receipt could not extend the pending journal as a terminal copy."));
	}
	if (!FCompletion::TryCreate(
			Request,
			Result.AdmissionResult,
			CurrentJournal,
			Result.TerminalJournal,
			Result.Completion))
	{
		return Finish(
			ESessionStatus::CompletionCreationRejected,
			TEXT("Canonical recovery completion could not be created from exact admitted evidence."));
	}

	FStorageAdapter Storage;
	const int32 MinimumGeneration =
		FMath::Max(1, Result.PreviousCompletionGeneration);
	const auto Saved = Storage.Save(
		CompletionStorageContext,
		Result.Completion,
		MinimumGeneration,
		FileSystem);
	Result.CompletionSaveStatus = Saved.GetStatus();
	if (!Saved.IsSuccess() && !Saved.DidReplacePrimary())
	{
		return Finish(
			ESessionStatus::CompletionSaveRejected,
			TEXT("Recovery completion could not be durably saved."));
	}
	const auto Loaded = Storage.Load(
		CompletionStorageContext,
		MinimumGeneration,
		FileSystem);
	Result.CompletionLoadStatus = Loaded.GetStatus();
	if (!Loaded.IsSuccess()
		|| Loaded.GetGeneration() != Result.TargetGeneration
		|| !Loaded.GetCompletion().Matches(Result.Completion))
	{
		return Finish(
			ESessionStatus::CompletionVerificationRejected,
			TEXT("Saved recovery completion could not be verified before authority advance."));
	}
	Result.bCompletionVerified = true;

	FAuthorityAdvanceRequest AdvanceRequest;
	FString AdvanceDiagnostic;
	if (!FAuthorityAdvanceRequest::TryCreate(
			CompletionAuthorityDomainId,
			LineageId,
			Result.PreviousCompletionGeneration,
			Result.TargetGeneration,
			Result.Completion.GetCompletionId(),
			AdvanceRequest,
			AdvanceDiagnostic))
	{
		return Finish(
			ESessionStatus::CompletionAdvanceRejected,
			TEXT("Completion watermark request could not be created."));
	}
	const auto Advanced = Authority.CompareAndAdvance(AdvanceRequest);
	Result.CompletionAuthorityAdvanceStatus = Advanced.GetStatus();
	if (!Advanced.IsValid())
	{
		return Finish(
			ESessionStatus::CompletionAdvanceRejected,
			TEXT("Completion authority returned a malformed advance result."));
	}
	if (Advanced.IsSuccess())
	{
		if (!Advanced.GetReceipt().Matches(AdvanceRequest)
			|| !Advanced.GetState().MatchesRequest(AdvanceRequest))
		{
			return Finish(
				ESessionStatus::CompletionAdvanceRejected,
				TEXT("Completion authority success did not match the exact request."));
		}
		Result.bAuthorityCurrent = true;
		return Finish(
			ESessionStatus::Completed,
			TEXT("Recovery completion was verified before its trusted watermark advanced."));
	}
	if (Advanced.GetStatus() == EAuthorityAdvanceStatus::Conflict)
	{
		return Finish(
			ESessionStatus::CompletionAdvanceConflict,
			TEXT("Completion watermark compare-and-advance conflicted."));
	}
	if (Advanced.GetStatus() == EAuthorityAdvanceStatus::Rejected)
	{
		return Finish(
			ESessionStatus::CompletionAdvanceRejected,
			TEXT("Completion watermark advance was rejected."));
	}
	if (Advanced.GetStatus() == EAuthorityAdvanceStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::CompletionEvidenceCommittedAuthorityPending,
			TEXT("Completion evidence is durable but its trusted watermark remains pending."));
	}

	const auto Recheck = Authority.Read(
		CompletionAuthorityDomainId, LineageId);
	Result.CompletionAuthorityRecheckStatus = Recheck.GetStatus();
	if (Recheck.IsValid()
		&& Recheck.GetStatus() == EAuthorityReadStatus::Current
		&& Recheck.GetState().MatchesRequest(AdvanceRequest))
	{
		Result.bAuthorityCurrent = true;
		return Finish(
			ESessionStatus::CompletedAfterAuthorityRecheck,
			TEXT("Unknown completion watermark outcome resolved by one exact re-read."));
	}
	return Finish(
		ESessionStatus::CompletionOutcomeUnresolved,
		TEXT("Completion evidence is durable but one authority re-read could not resolve the watermark."));
}
