#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FRecord =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord;
	using FJournal =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal;
	using FCheckpoint =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint;
	using FCheckpointResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointResult;
	using ECheckpointStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ValidateOrderedRecords(const TArray<FRecord>& Records)
	{
		TSet<FGuid> SeenCommandIds;
		for (const FRecord& Record : Records)
		{
			if (!Record.IsValid())
			{
				return false;
			}
			const FGuid& CommandId = Record.GetCommand().GetCommandId();
			if (!CommandId.IsValid() || SeenCommandIds.Contains(CommandId))
			{
				return false;
			}
			SeenCommandIds.Add(CommandId);
		}
		return true;
	}

	FGuid DeriveCheckpointId(const TArray<FRecord>& Records)
	{
		if (!ValidateOrderedRecords(Records))
		{
			return FGuid();
		}

		TArray<FString> Parts;
		Parts.Reserve(1 + Records.Num() * 3);
		Parts.Add(FString::FromInt(Records.Num()));
		for (const FRecord& Record : Records)
		{
			Parts.Add(GuidDigits(Record.GetCommand().GetCommandId()));
			Parts.Add(GuidDigits(Record.GetReceipt().GetReceiptId()));
			Parts.Add(FString::FromInt(
				static_cast<int32>(Record.GetAdapterStatus())));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionProductRetryStepCommandJournalCheckpoint.r1"),
			Parts);
	}
}

bool FCheckpoint::TryCreate(
	const TArray<FRecord>& OrderedRecords,
	FCheckpoint& OutCheckpoint)
{
	OutCheckpoint = FCheckpoint();
	if (!ValidateOrderedRecords(OrderedRecords))
	{
		return false;
	}

	OutCheckpoint.Records = OrderedRecords;
	OutCheckpoint.CheckpointId = DeriveCheckpointId(OutCheckpoint.Records);
	if (!OutCheckpoint.IsValid())
	{
		OutCheckpoint = FCheckpoint();
		return false;
	}
	return true;
}

bool FCheckpoint::IsValid() const
{
	return CheckpointId.IsValid()
		&& ValidateOrderedRecords(Records)
		&& CheckpointId == DeriveCheckpointId(Records);
}

bool FCheckpoint::Matches(const FCheckpoint& Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| CheckpointId != Other.CheckpointId
		|| Records.Num() != Other.Records.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const FRecord& Left = Records[Index];
		const FRecord& Right = Other.Records[Index];
		if (Left.GetAdapterStatus() != Right.GetAdapterStatus()
			|| Left.GetReceipt().GetReceiptId()
				!= Right.GetReceipt().GetReceiptId()
			|| !Left.GetCommand().Matches(Right.GetCommand()))
		{
			return false;
		}
	}
	return true;
}

bool FCheckpoint::TryGetRecordAt(
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

bool FCheckpointResult::IsSuccess() const
{
	return (Status == ECheckpointStatus::Exported
			|| Status == ECheckpointStatus::Restored)
		&& Checkpoint.IsValid();
}

FCheckpointResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointService::
Export(const FJournal& Journal)
{
	FCheckpointResult Result;
	if (!Journal.IsValid())
	{
		Result.Status = ECheckpointStatus::JournalInvalid;
		Result.Diagnostic = TEXT(
			"Retry-command journal must be valid before checkpoint export.");
		return Result;
	}
	if (!FCheckpoint::TryCreate(Journal.Records, Result.Checkpoint))
	{
		Result.Diagnostic = TEXT(
			"Retry-command journal records cannot form a valid checkpoint.");
		return Result;
	}
	Result.Status = ECheckpointStatus::Exported;
	Result.Diagnostic = TEXT(
		"Exported an immutable ordered retry-command journal checkpoint.");
	return Result;
}

FCheckpointResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointService::
Restore(
	const FCheckpoint& Checkpoint,
	FJournal& InOutJournal)
{
	FCheckpointResult Result;
	if (!Checkpoint.IsValid())
	{
		Result.Diagnostic = TEXT(
			"Retry-command journal restore requires one valid checkpoint.");
		return Result;
	}
	Result.Checkpoint = Checkpoint;
	if (!InOutJournal.IsValid())
	{
		Result.Status = ECheckpointStatus::JournalInvalid;
		Result.Diagnostic = TEXT(
			"Retry-command journal restore target is internally inconsistent.");
		return Result;
	}
	if (InOutJournal.GetRecordCount() != 0)
	{
		Result.Status = ECheckpointStatus::TargetNotEmpty;
		Result.Diagnostic = TEXT(
			"Retry-command journal restore refuses to overwrite existing records.");
		return Result;
	}

	FJournal Candidate;
	for (const FRecord& Record : Checkpoint.Records)
	{
		const int32 Index = Candidate.Records.Add(Record);
		Candidate.RecordIndexByCommandId.Add(
			Record.GetCommand().GetCommandId(), Index);
	}
	if (!Candidate.IsValid()
		|| Candidate.GetRecordCount() != Checkpoint.GetRecordCount())
	{
		Result.Status = ECheckpointStatus::RestoreStateInvalid;
		Result.Diagnostic = TEXT(
			"Retry-command checkpoint cannot rebuild a valid journal index.");
		return Result;
	}

	InOutJournal = MoveTemp(Candidate);
	Result.Status = ECheckpointStatus::Restored;
	Result.Diagnostic = TEXT(
		"Restored the immutable retry-command journal checkpoint.");
	return Result;
}
