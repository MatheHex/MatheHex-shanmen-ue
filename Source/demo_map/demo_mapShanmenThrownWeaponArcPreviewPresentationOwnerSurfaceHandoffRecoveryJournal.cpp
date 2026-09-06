#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FCheckpoint =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt;
	using FRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using FAppendResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult;
	using FDecodeResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeResult;
	using ERecordKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;
	using EDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using EAppendStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus;
	using EDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus;
	using ESourceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus;
	using FCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;

	constexpr uint8 JournalMagic[] =
	{
		'S', 'M', 'A', 'P', 'R', 'J', 'N', 'L'
	};

	FString GuidKey(const FGuid& Value)
	{
		return Value.IsValid()
			? Value.ToString(EGuidFormats::Digits)
			: TEXT("NONE");
	}

	bool IsKnownRecordKind(const ERecordKind Kind)
	{
		return Kind == ERecordKind::CheckpointPrepared
			|| Kind == ERecordKind::RecoveryCommitted;
	}

	bool IsRecoverableSourceFailure(const ESourceStatus Status)
	{
		return Status == ESourceStatus::RetirementResponseInvalid
			|| Status == ESourceStatus::RetirementInvariantViolation
			|| Status == ESourceStatus::CommitInvariantViolation;
	}

	FGuid ConsumerDigest(const FName ConsumerDefinitionId)
	{
		if (ConsumerDefinitionId.IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryConsumer.r1"),
			{ConsumerDefinitionId.ToString()});
	}

	FGuid DeriveRecordId(const FRecord& Record)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryJournalRecord.r2"),
			{
				FString::FromInt(static_cast<int32>(Record.GetKind())),
				FString::FromInt(Record.GetSequence()),
				GuidKey(Record.GetPreviousRecordId()),
				GuidKey(Record.GetCheckpointId()),
				GuidKey(Record.GetTransitionTicketId()),
				GuidKey(Record.GetRunId()),
				GuidKey(Record.GetConsumerDefinitionDigest()),
				GuidKey(Record.GetSourceRetirementResponseId()),
				GuidKey(Record.GetPreviousSurfaceInstanceId()),
				GuidKey(Record.GetSurfaceInstanceId()),
				GuidKey(Record.GetPreviousSurfaceCursorId()),
				GuidKey(Record.GetSurfaceCursorId()),
				GuidKey(Record.GetRecoveryReceiptId()),
				FString::FromInt(
					static_cast<int32>(Record.GetSourceFailureStatus()))
			});
	}

	FGuid DeriveJournalId(const TArray<FRecord>& Records)
	{
		TArray<FString> Parts;
		Parts.Reserve(1 + Records.Num());
		Parts.Add(FString::FromInt(Records.Num()));
		for (const FRecord& Record : Records)
		{
			if (!Record.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidKey(Record.GetRecordId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryJournal.r2"),
			Parts);
	}

	FGuid DerivePreviousSchemaDigest(const FRecord& Record)
	{
		if (!Record.IsValid()
			|| Record.GetKind() != ERecordKind::CheckpointPrepared
			|| Record.GetSequence() != 0
			|| Record.GetPreviousRecordId().IsValid()
			|| Record.GetRecoveryReceiptId().IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryJournalRecord.r1"),
			{
				FString::FromInt(static_cast<int32>(Record.GetKind())),
				GuidKey(Record.GetCheckpointId()),
				GuidKey(Record.GetTransitionTicketId()),
				GuidKey(Record.GetRunId()),
				GuidKey(Record.GetConsumerDefinitionDigest()),
				GuidKey(Record.GetSourceRetirementResponseId()),
				GuidKey(Record.GetPreviousSurfaceInstanceId()),
				GuidKey(Record.GetSurfaceInstanceId()),
				GuidKey(Record.GetPreviousSurfaceCursorId()),
				GuidKey(Record.GetSurfaceCursorId()),
				FString::FromInt(
					static_cast<int32>(Record.GetSourceFailureStatus()))
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

	void AppendHeader(
		TArray<uint8>& OutBytes,
		const int32 SchemaVersion,
		const int32 RecordCount,
		const FGuid& JournalId)
	{
		OutBytes.Append(JournalMagic, UE_ARRAY_COUNT(JournalMagic));
		AppendUint32BigEndian(
			OutBytes, static_cast<uint32>(SchemaVersion));
		AppendUint32BigEndian(
			OutBytes, static_cast<uint32>(RecordCount));
		AppendGuid(OutBytes, JournalId);
	}

}

bool FRecord::IsValid() const
{
	return IsKnownRecordKind(Kind)
		&& Sequence >= 0
		&& ((Sequence == 0 && !PreviousRecordId.IsValid())
			|| (Sequence > 0 && PreviousRecordId.IsValid()))
		&& RecordId.IsValid()
		&& CheckpointId.IsValid()
		&& TransitionTicketId.IsValid()
		&& RunId.IsValid()
		&& ConsumerDefinitionDigest.IsValid()
		&& PreviousSurfaceInstanceId.IsValid()
		&& SurfaceInstanceId.IsValid()
		&& PreviousSurfaceInstanceId != SurfaceInstanceId
		&& PreviousSurfaceCursorId.IsValid()
		&& SurfaceCursorId.IsValid()
		&& IsRecoverableSourceFailure(SourceFailureStatus)
		&& ((Kind == ERecordKind::CheckpointPrepared
				&& !RecoveryReceiptId.IsValid())
			|| (Kind == ERecordKind::RecoveryCommitted
				&& RecoveryReceiptId.IsValid()))
		&& RecordId == DeriveRecordId(*this);
}

bool FRecord::Matches(const FRecord& Other) const
{
	return IsValid() && Other.IsValid()
		&& Kind == Other.Kind
		&& Sequence == Other.Sequence
		&& RecordId == Other.RecordId
		&& PreviousRecordId == Other.PreviousRecordId
		&& CheckpointId == Other.CheckpointId
		&& TransitionTicketId == Other.TransitionTicketId
		&& RunId == Other.RunId
		&& ConsumerDefinitionDigest == Other.ConsumerDefinitionDigest
		&& SourceRetirementResponseId
			== Other.SourceRetirementResponseId
		&& PreviousSurfaceInstanceId == Other.PreviousSurfaceInstanceId
		&& SurfaceInstanceId == Other.SurfaceInstanceId
		&& PreviousSurfaceCursorId == Other.PreviousSurfaceCursorId
		&& SurfaceCursorId == Other.SurfaceCursorId
		&& RecoveryReceiptId == Other.RecoveryReceiptId
		&& SourceFailureStatus == Other.SourceFailureStatus;
}

bool FRecord::MatchesCheckpoint(const FCheckpoint& Checkpoint) const
{
	return IsValid() && Checkpoint.IsValid()
		&& CheckpointId == Checkpoint.GetCheckpointId()
		&& TransitionTicketId
			== Checkpoint.GetTransitionTicket().GetTicketId()
		&& RunId == Checkpoint.GetTransitionTicket().GetRunId()
		&& ConsumerDefinitionDigest == ConsumerDigest(
			Checkpoint.GetTransitionTicket().GetConsumerDefinitionId())
		&& SourceRetirementResponseId
			== Checkpoint.GetSourceRetirementResponseId()
		&& PreviousSurfaceInstanceId
			== Checkpoint.GetPreviousSurfaceInstanceId()
		&& SurfaceInstanceId == Checkpoint.GetSurfaceInstanceId()
		&& PreviousSurfaceCursorId
			== Checkpoint.GetPreviousSurfaceCursor().GetPresentationStateId()
		&& SurfaceCursorId
			== Checkpoint.GetSurfaceCursor().GetPresentationStateId()
		&& SourceFailureStatus == Checkpoint.GetSourceFailureStatus();
}

bool FRecord::MatchesRecoveryReceipt(
	const FCheckpoint& Checkpoint,
	const FReceipt& Receipt) const
{
	return IsValid()
		&& Kind == ERecordKind::RecoveryCommitted
		&& MatchesCheckpoint(Checkpoint)
		&& Receipt.IsValid()
		&& Receipt.MatchesCheckpoint(Checkpoint)
		&& RecoveryReceiptId == Receipt.GetReceiptId();
}

bool FAppendResult::IsValid() const
{
	if (Diagnostic.IsEmpty()
		|| PreviousRecordCount < 0
		|| RecordCount < 0)
	{
		return false;
	}
	if (Status == EAppendStatus::Appended)
	{
		return Record.IsValid()
			&& RecordCount == PreviousRecordCount + 1;
	}
	if (Status == EAppendStatus::Replayed)
	{
		return Record.IsValid()
			&& RecordCount == PreviousRecordCount;
	}
	return Status != EAppendStatus::Invalid
		&& !Record.IsValid()
		&& RecordCount == PreviousRecordCount;
}

bool FAppendResult::IsSuccess() const
{
	return IsValid()
		&& (Status == EAppendStatus::Appended
			|| Status == EAppendStatus::Replayed);
}

bool FAppendResult::DidAppend() const
{
	return IsValid() && Status == EAppendStatus::Appended;
}

bool FAppendResult::IsReplay() const
{
	return IsValid() && Status == EAppendStatus::Replayed;
}

bool FJournal::IsValid() const
{
	if (Records.Num() > MaxRecordCount())
	{
		return false;
	}
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const FRecord& Record = Records[Index];
		if (!Record.IsValid() || Record.GetSequence() != Index)
		{
			return false;
		}
		if (Index == 0)
		{
			if (Record.GetPreviousRecordId().IsValid()
				|| Record.GetKind() != ERecordKind::CheckpointPrepared)
			{
				return false;
			}
			continue;
		}

		const FRecord& Previous = Records[Index - 1];
		if (Record.GetPreviousRecordId() != Previous.GetRecordId())
		{
			return false;
		}
		if (Previous.GetKind() == ERecordKind::CheckpointPrepared)
		{
			if (Record.GetKind() != ERecordKind::RecoveryCommitted
				|| Record.GetCheckpointId() != Previous.GetCheckpointId())
			{
				return false;
			}
		}
		else if (Record.GetKind() != ERecordKind::CheckpointPrepared
			|| Record.GetCheckpointId() == Previous.GetCheckpointId())
		{
			return false;
		}
	}
	return DeriveJournalId(Records).IsValid();
}

FGuid FJournal::GetJournalId() const
{
	return IsValid() ? DeriveJournalId(Records) : FGuid();
}

EDisposition FJournal::GetLatestDisposition() const
{
	if (!IsValid() || Records.IsEmpty())
	{
		return EDisposition::Empty;
	}
	return Records.Last().GetKind() == ERecordKind::CheckpointPrepared
		? EDisposition::CheckpointPending
		: EDisposition::RecoveryCommitted;
}

bool FJournal::TryGetRecordAt(
	const int32 Index,
	FRecord& OutRecord) const
{
	OutRecord = FRecord();
	if (!IsValid() || !Records.IsValidIndex(Index))
	{
		return false;
	}
	OutRecord = Records[Index];
	return OutRecord.IsValid();
}

bool FJournal::TryGetLatestRecord(FRecord& OutRecord) const
{
	return TryGetRecordAt(Records.Num() - 1, OutRecord);
}

bool FJournal::MatchesLatestCheckpoint(
	const FCheckpoint& Checkpoint) const
{
	return IsValid() && !Records.IsEmpty()
		&& Records.Last().MatchesCheckpoint(Checkpoint);
}

bool FJournal::MatchesLatestRecovery(
	const FCheckpoint& Checkpoint,
	const FReceipt& Receipt) const
{
	return IsValid() && !Records.IsEmpty()
		&& Records.Last().MatchesRecoveryReceipt(Checkpoint, Receipt);
}

FAppendResult FJournal::AppendCheckpoint(const FCheckpoint& Checkpoint)
{
	const int32 PreviousCount = Records.Num();
	if (!IsValid())
	{
		return MakeResult(
			EAppendStatus::JournalInvalid,
			TEXT("Arc preview handoff recovery journal must be valid before append."),
			PreviousCount);
	}
	if (!Checkpoint.IsValid())
	{
		return MakeResult(
			EAppendStatus::CheckpointInvalid,
			TEXT("Arc preview handoff recovery journal requires one valid checkpoint."),
			PreviousCount);
	}
	if (!Records.IsEmpty() && Records.Last().MatchesCheckpoint(Checkpoint))
	{
		return MakeResult(
			EAppendStatus::Replayed,
			TEXT("Arc preview handoff recovery checkpoint was already journaled."),
			PreviousCount,
			Records.Last());
	}
	if (!Records.IsEmpty()
		&& Records.Last().GetKind() == ERecordKind::CheckpointPrepared)
	{
		return MakeResult(
			EAppendStatus::PendingCheckpointConflict,
			TEXT("Arc preview handoff recovery journal already has another pending checkpoint."),
			PreviousCount);
	}
	if (Records.Num() >= MaxRecordCount())
	{
		return MakeResult(
			EAppendStatus::CapacityExceeded,
			TEXT("Arc preview handoff recovery journal capacity is exhausted without eviction."),
			PreviousCount);
	}

	const FGuid PreviousRecordId = Records.IsEmpty()
		? FGuid()
		: Records.Last().GetRecordId();
	const FRecord Record = MakeRecord(
		ERecordKind::CheckpointPrepared,
		Records.Num(),
		PreviousRecordId,
		Checkpoint,
		FReceipt());
	FJournal Candidate = *this;
	Candidate.Records.Add(Record);
	if (!Record.IsValid() || !Candidate.IsValid())
	{
		return MakeResult(
			EAppendStatus::RecordInvariantViolation,
			TEXT("Arc preview handoff recovery checkpoint record failed chain validation."),
			PreviousCount);
	}
	*this = MoveTemp(Candidate);
	return MakeResult(
		EAppendStatus::Appended,
		TEXT("Appended one immutable Arc preview handoff recovery checkpoint record."),
		PreviousCount,
		Record);
}

FRecord FJournal::MakeRecord(
	const ERecordKind Kind,
	const int32 Sequence,
	const FGuid& PreviousRecordId,
	const FCheckpoint& Checkpoint,
	const FReceipt& Receipt)
{
	FRecord Record;
	if (!Checkpoint.IsValid()
		|| (Kind == ERecordKind::RecoveryCommitted
			&& (!Receipt.IsValid()
				|| !Receipt.MatchesCheckpoint(Checkpoint)))
		|| (Kind == ERecordKind::CheckpointPrepared
			&& Receipt.IsValid()))
	{
		return Record;
	}

	Record.Kind = Kind;
	Record.Sequence = Sequence;
	Record.PreviousRecordId = PreviousRecordId;
	Record.CheckpointId = Checkpoint.GetCheckpointId();
	Record.TransitionTicketId =
		Checkpoint.GetTransitionTicket().GetTicketId();
	Record.RunId = Checkpoint.GetTransitionTicket().GetRunId();
	Record.ConsumerDefinitionDigest = ConsumerDigest(
		Checkpoint.GetTransitionTicket().GetConsumerDefinitionId());
	Record.SourceRetirementResponseId =
		Checkpoint.GetSourceRetirementResponseId();
	Record.PreviousSurfaceInstanceId =
		Checkpoint.GetPreviousSurfaceInstanceId();
	Record.SurfaceInstanceId = Checkpoint.GetSurfaceInstanceId();
	Record.PreviousSurfaceCursorId =
		Checkpoint.GetPreviousSurfaceCursor().GetPresentationStateId();
	Record.SurfaceCursorId =
		Checkpoint.GetSurfaceCursor().GetPresentationStateId();
	Record.RecoveryReceiptId = Receipt.GetReceiptId();
	Record.SourceFailureStatus = Checkpoint.GetSourceFailureStatus();
	Record.RecordId = DeriveRecordId(Record);
	return Record.IsValid() ? Record : FRecord();
}

FAppendResult FJournal::AppendRecoveryReceipt(
	const FCheckpoint& Checkpoint,
	const FReceipt& Receipt)
{
	const int32 PreviousCount = Records.Num();
	if (!IsValid())
	{
		return MakeResult(
			EAppendStatus::JournalInvalid,
			TEXT("Arc preview handoff recovery journal must be valid before append."),
			PreviousCount);
	}
	if (!Checkpoint.IsValid())
	{
		return MakeResult(
			EAppendStatus::CheckpointInvalid,
			TEXT("Arc preview handoff recovery receipt append requires one valid checkpoint."),
			PreviousCount);
	}
	if (!Receipt.IsValid() || !Receipt.MatchesCheckpoint(Checkpoint))
	{
		return MakeResult(
			EAppendStatus::ReceiptInvalid,
			TEXT("Arc preview handoff recovery receipt does not match its checkpoint."),
			PreviousCount);
	}
	if (!Records.IsEmpty()
		&& Records.Last().MatchesRecoveryReceipt(Checkpoint, Receipt))
	{
		return MakeResult(
			EAppendStatus::Replayed,
			TEXT("Arc preview handoff recovery receipt was already journaled."),
			PreviousCount,
			Records.Last());
	}
	if (Records.IsEmpty()
		|| Records.Last().GetKind() != ERecordKind::CheckpointPrepared)
	{
		return MakeResult(
			EAppendStatus::CheckpointNotPending,
			TEXT("Arc preview handoff recovery receipt requires the latest checkpoint to be pending."),
			PreviousCount);
	}
	if (!Records.Last().MatchesCheckpoint(Checkpoint))
	{
		return MakeResult(
			EAppendStatus::PendingCheckpointConflict,
			TEXT("Arc preview handoff recovery receipt does not match the pending checkpoint."),
			PreviousCount);
	}
	if (Records.Num() >= MaxRecordCount())
	{
		return MakeResult(
			EAppendStatus::CapacityExceeded,
			TEXT("Arc preview handoff recovery journal capacity is exhausted without eviction."),
			PreviousCount);
	}

	const FRecord Record = MakeRecord(
		ERecordKind::RecoveryCommitted,
		Records.Num(),
		Records.Last().GetRecordId(),
		Checkpoint,
		Receipt);
	FJournal Candidate = *this;
	Candidate.Records.Add(Record);
	if (!Record.IsValid() || !Candidate.IsValid())
	{
		return MakeResult(
			EAppendStatus::RecordInvariantViolation,
			TEXT("Arc preview handoff recovery receipt record failed chain validation."),
			PreviousCount);
	}
	*this = MoveTemp(Candidate);
	return MakeResult(
		EAppendStatus::Appended,
		TEXT("Appended one immutable Arc preview handoff recovery receipt record."),
		PreviousCount,
		Record);
}

FAppendResult FJournal::MakeResult(
	const EAppendStatus Status,
	const TCHAR* Diagnostic,
	const int32 PreviousRecordCount,
	const FRecord& Record) const
{
	FAppendResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.PreviousRecordCount = PreviousRecordCount;
	Result.RecordCount = Records.Num();
	Result.Record = Record;
	return Result;
}

bool FDecodeResult::IsSuccess() const
{
	return (Status == EDecodeStatus::DecodedCurrent
			|| Status == EDecodeStatus::MigratedPrevious)
		&& (SourceSchemaVersion == FCodec::CurrentSchemaVersion()
			|| SourceSchemaVersion == FCodec::PreviousSchemaVersion())
		&& !Diagnostic.IsEmpty()
		&& Journal.IsValid();
}

bool FDecodeResult::WasMigrated() const
{
	return IsSuccess()
		&& Status == EDecodeStatus::MigratedPrevious
		&& SourceSchemaVersion == FCodec::PreviousSchemaVersion();
}

FDecodeResult FCodec::MakeDecodeResult(
	const EDecodeStatus Status,
	const TCHAR* Diagnostic,
	const int32 SourceSchemaVersion,
	const FJournal& Journal)
{
	FDecodeResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SourceSchemaVersion = SourceSchemaVersion;
	Result.Journal = Journal;
	return Result;
}

bool FCodec::TryEncode(
	const FJournal& Journal,
	TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	if (!Journal.IsValid())
	{
		return false;
	}
	OutBytes.Reserve(
		HeaderSize() + Journal.GetRecordCount() * CurrentRecordSize());
	AppendHeader(
		OutBytes,
		CurrentSchemaVersion(),
		Journal.GetRecordCount(),
		Journal.GetJournalId());
	for (const FRecord& Record : Journal.Records)
	{
		AppendUint32BigEndian(
			OutBytes, static_cast<uint32>(Record.Kind));
		AppendUint32BigEndian(
			OutBytes, static_cast<uint32>(Record.Sequence));
		AppendGuid(OutBytes, Record.RecordId);
		AppendGuid(OutBytes, Record.PreviousRecordId);
		AppendGuid(OutBytes, Record.CheckpointId);
		AppendGuid(OutBytes, Record.TransitionTicketId);
		AppendGuid(OutBytes, Record.RunId);
		AppendGuid(OutBytes, Record.ConsumerDefinitionDigest);
		AppendGuid(OutBytes, Record.SourceRetirementResponseId);
		AppendGuid(OutBytes, Record.PreviousSurfaceInstanceId);
		AppendGuid(OutBytes, Record.SurfaceInstanceId);
		AppendGuid(OutBytes, Record.PreviousSurfaceCursorId);
		AppendGuid(OutBytes, Record.SurfaceCursorId);
		AppendGuid(OutBytes, Record.RecoveryReceiptId);
		AppendUint32BigEndian(
			OutBytes, static_cast<uint32>(Record.SourceFailureStatus));
	}
	if (OutBytes.Num()
		!= HeaderSize() + Journal.GetRecordCount() * CurrentRecordSize())
	{
		OutBytes.Reset();
		return false;
	}
	return true;
}

bool FCodec::TryEncodePreviousSchema(
	const FJournal& Journal,
	TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	if (!Journal.IsValid() || Journal.Records.Num() != 1)
	{
		return false;
	}
	const FRecord& Record = Journal.Records[0];
	const FGuid LegacyDigest = DerivePreviousSchemaDigest(Record);
	if (!LegacyDigest.IsValid())
	{
		return false;
	}

	OutBytes.Reserve(HeaderSize() + PreviousRecordSize());
	AppendHeader(
		OutBytes,
		PreviousSchemaVersion(),
		1,
		Journal.GetJournalId());
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Record.Kind));
	AppendGuid(OutBytes, Record.CheckpointId);
	AppendGuid(OutBytes, Record.TransitionTicketId);
	AppendGuid(OutBytes, Record.RunId);
	AppendGuid(OutBytes, Record.ConsumerDefinitionDigest);
	AppendGuid(OutBytes, Record.SourceRetirementResponseId);
	AppendGuid(OutBytes, Record.PreviousSurfaceInstanceId);
	AppendGuid(OutBytes, Record.SurfaceInstanceId);
	AppendGuid(OutBytes, Record.PreviousSurfaceCursorId);
	AppendGuid(OutBytes, Record.SurfaceCursorId);
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Record.SourceFailureStatus));
	AppendGuid(OutBytes, LegacyDigest);
	if (OutBytes.Num() != HeaderSize() + PreviousRecordSize())
	{
		OutBytes.Reset();
		return false;
	}
	return true;
}

FDecodeResult FCodec::Decode(const TArray<uint8>& Bytes)
{
	if (Bytes.IsEmpty())
	{
		return MakeDecodeResult(
			EDecodeStatus::InputEmpty,
			TEXT("Arc preview handoff recovery journal bytes are empty."));
	}
	if (Bytes.Num() < HeaderSize())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview handoff recovery journal header is truncated."));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(JournalMagic); ++Index)
	{
		if (Bytes[Index] != JournalMagic[Index])
		{
			return MakeDecodeResult(
				EDecodeStatus::MagicMismatch,
				TEXT("Arc preview handoff recovery journal magic is invalid."));
		}
	}

	int32 Offset = UE_ARRAY_COUNT(JournalMagic);
	uint32 SchemaVersionValue = 0;
	uint32 RecordCountValue = 0;
	FGuid EncodedJournalId;
	if (!TryReadUint32BigEndian(Bytes, Offset, SchemaVersionValue)
		|| SchemaVersionValue > static_cast<uint32>(MAX_int32)
		|| !TryReadUint32BigEndian(Bytes, Offset, RecordCountValue)
		|| RecordCountValue > static_cast<uint32>(MAX_int32)
		|| !TryReadGuid(Bytes, Offset, EncodedJournalId)
		|| Offset != HeaderSize())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview handoff recovery journal header could not be decoded."));
	}
	const int32 SchemaVersion = static_cast<int32>(SchemaVersionValue);
	const int32 RecordCount = static_cast<int32>(RecordCountValue);
	if (SchemaVersion != CurrentSchemaVersion()
		&& SchemaVersion != PreviousSchemaVersion())
	{
		return MakeDecodeResult(
			EDecodeStatus::UnsupportedSchema,
			TEXT("Arc preview handoff recovery journal schema is unsupported."),
			SchemaVersion);
	}
	if (RecordCount < 0 || RecordCount > FJournal::MaxRecordCount())
	{
		return MakeDecodeResult(
			EDecodeStatus::RecordCountOutOfRange,
			TEXT("Arc preview handoff recovery journal record count exceeds its bound."),
			SchemaVersion);
	}

	if (SchemaVersion == PreviousSchemaVersion())
	{
		if (RecordCount != 1)
		{
			return MakeDecodeResult(
				EDecodeStatus::PreviousSchemaShapeUnsupported,
				TEXT("Previous Arc preview journal schema only supports one pending checkpoint."),
				SchemaVersion);
		}
		if (Bytes.Num() != HeaderSize() + PreviousRecordSize())
		{
			return MakeDecodeResult(
				EDecodeStatus::SizeMismatch,
				TEXT("Previous Arc preview journal record is truncated or has trailing bytes."),
				SchemaVersion);
		}

		uint32 KindValue = 0;
		uint32 SourceStatusValue = 0;
		FGuid LegacyDigest;
		FRecord Record;
		if (!TryReadUint32BigEndian(Bytes, Offset, KindValue)
			|| !TryReadGuid(Bytes, Offset, Record.CheckpointId)
			|| !TryReadGuid(Bytes, Offset, Record.TransitionTicketId)
			|| !TryReadGuid(Bytes, Offset, Record.RunId)
			|| !TryReadGuid(Bytes, Offset, Record.ConsumerDefinitionDigest)
			|| !TryReadGuid(Bytes, Offset, Record.SourceRetirementResponseId)
			|| !TryReadGuid(Bytes, Offset, Record.PreviousSurfaceInstanceId)
			|| !TryReadGuid(Bytes, Offset, Record.SurfaceInstanceId)
			|| !TryReadGuid(Bytes, Offset, Record.PreviousSurfaceCursorId)
			|| !TryReadGuid(Bytes, Offset, Record.SurfaceCursorId)
			|| !TryReadUint32BigEndian(Bytes, Offset, SourceStatusValue)
			|| !TryReadGuid(Bytes, Offset, LegacyDigest)
			|| Offset != Bytes.Num())
		{
			return MakeDecodeResult(
				EDecodeStatus::SizeMismatch,
				TEXT("Previous Arc preview journal record could not be decoded."),
				SchemaVersion);
		}
		if (KindValue > static_cast<uint32>(MAX_uint8)
			|| SourceStatusValue > static_cast<uint32>(MAX_uint8))
		{
			return MakeDecodeResult(
				EDecodeStatus::RecordInvalid,
				TEXT("Previous Arc preview journal enum value is out of range."),
				SchemaVersion);
		}
		Record.Kind = static_cast<ERecordKind>(KindValue);
		Record.Sequence = 0;
		Record.SourceFailureStatus =
			static_cast<ESourceStatus>(SourceStatusValue);
		Record.RecordId = DeriveRecordId(Record);
		if (!Record.IsValid()
			|| Record.GetKind() != ERecordKind::CheckpointPrepared
			|| LegacyDigest != DerivePreviousSchemaDigest(Record))
		{
			return MakeDecodeResult(
				EDecodeStatus::RecordInvalid,
				TEXT("Previous Arc preview journal record failed canonical digest validation."),
				SchemaVersion);
		}
		FJournal Journal;
		Journal.Records.Add(Record);
		if (!Journal.IsValid())
		{
			return MakeDecodeResult(
				EDecodeStatus::JournalInvalid,
				TEXT("Previous Arc preview journal could not migrate into a valid current chain."),
				SchemaVersion);
		}
		if (EncodedJournalId != Journal.GetJournalId())
		{
			return MakeDecodeResult(
				EDecodeStatus::JournalIdentityMismatch,
				TEXT("Previous Arc preview journal identity does not match its migrated record."),
				SchemaVersion);
		}
		return MakeDecodeResult(
			EDecodeStatus::MigratedPrevious,
			TEXT("Migrated one previous-schema Arc preview recovery checkpoint journal."),
			SchemaVersion,
			Journal);
	}

	const int64 ExpectedSize = static_cast<int64>(HeaderSize())
		+ static_cast<int64>(RecordCount) * CurrentRecordSize();
	if (ExpectedSize != Bytes.Num())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Current Arc preview journal is truncated or has trailing bytes."),
			SchemaVersion);
	}

	FJournal Journal;
	Journal.Records.Reserve(RecordCount);
	for (int32 Index = 0; Index < RecordCount; ++Index)
	{
		uint32 KindValue = 0;
		uint32 SequenceValue = 0;
		uint32 SourceStatusValue = 0;
		FRecord Record;
		if (!TryReadUint32BigEndian(Bytes, Offset, KindValue)
			|| !TryReadUint32BigEndian(Bytes, Offset, SequenceValue)
			|| SequenceValue > static_cast<uint32>(MAX_int32)
			|| !TryReadGuid(Bytes, Offset, Record.RecordId)
			|| !TryReadGuid(Bytes, Offset, Record.PreviousRecordId)
			|| !TryReadGuid(Bytes, Offset, Record.CheckpointId)
			|| !TryReadGuid(Bytes, Offset, Record.TransitionTicketId)
			|| !TryReadGuid(Bytes, Offset, Record.RunId)
			|| !TryReadGuid(Bytes, Offset, Record.ConsumerDefinitionDigest)
			|| !TryReadGuid(Bytes, Offset, Record.SourceRetirementResponseId)
			|| !TryReadGuid(Bytes, Offset, Record.PreviousSurfaceInstanceId)
			|| !TryReadGuid(Bytes, Offset, Record.SurfaceInstanceId)
			|| !TryReadGuid(Bytes, Offset, Record.PreviousSurfaceCursorId)
			|| !TryReadGuid(Bytes, Offset, Record.SurfaceCursorId)
			|| !TryReadGuid(Bytes, Offset, Record.RecoveryReceiptId)
			|| !TryReadUint32BigEndian(Bytes, Offset, SourceStatusValue))
		{
			return MakeDecodeResult(
				EDecodeStatus::SizeMismatch,
				TEXT("Current Arc preview journal record could not be decoded."),
				SchemaVersion);
		}
		if (KindValue > static_cast<uint32>(MAX_uint8)
			|| SourceStatusValue > static_cast<uint32>(MAX_uint8))
		{
			return MakeDecodeResult(
				EDecodeStatus::RecordInvalid,
				TEXT("Current Arc preview journal enum value is out of range."),
				SchemaVersion);
		}
		Record.Kind = static_cast<ERecordKind>(KindValue);
		Record.Sequence = static_cast<int32>(SequenceValue);
		Record.SourceFailureStatus =
			static_cast<ESourceStatus>(SourceStatusValue);
		if (!Record.IsValid())
		{
			return MakeDecodeResult(
				EDecodeStatus::RecordInvalid,
				TEXT("Current Arc preview journal record failed canonical identity validation."),
				SchemaVersion);
		}
		Journal.Records.Add(Record);
	}
	if (Offset != Bytes.Num())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Current Arc preview journal decoder did not consume the exact payload."),
			SchemaVersion);
	}
	if (!Journal.IsValid())
	{
		return MakeDecodeResult(
			EDecodeStatus::JournalInvalid,
			TEXT("Current Arc preview journal chain is internally inconsistent."),
			SchemaVersion);
	}
	if (EncodedJournalId != Journal.GetJournalId())
	{
		return MakeDecodeResult(
			EDecodeStatus::JournalIdentityMismatch,
			TEXT("Current Arc preview journal identity does not match its record chain."),
			SchemaVersion);
	}
	return MakeDecodeResult(
		EDecodeStatus::DecodedCurrent,
		TEXT("Decoded one canonical Arc preview handoff recovery journal."),
		SchemaVersion,
		Journal);
}
