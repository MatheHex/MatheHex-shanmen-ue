#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.h"

namespace
{
	using FCommand =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand;
	using FReceipt =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt;
	using FRecord =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalRecord;
	using FJournal =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal;
	using FJournalResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalResult;
	using EAdapterStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus;
	using EJournalStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalStatus;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;

	bool MatchesRecordedStatus(
		const EAdapterStatus Status,
		const FReceipt& Receipt)
	{
		if (!Receipt.IsValid())
		{
			return false;
		}

		switch (Status)
		{
		case EAdapterStatus::RecordedHandled:
			return Receipt.IsHandled();
		case EAdapterStatus::RecordedDecisionRejected:
			return Receipt.GetStep().Status == EStepStatus::DecisionRejected
				&& !Receipt.DidExecuteRetry();
		case EAdapterStatus::RecordedExecutionRejected:
			return Receipt.GetStep().Status == EStepStatus::ExecutionRejected
				&& Receipt.DidExecuteRetry();
		default:
			return false;
		}
	}
}

bool FRecord::IsValid() const
{
	return MatchesRecordedStatus(AdapterStatus, Receipt);
}

bool FJournalResult::IsSuccess() const
{
	const bool bRecorded = Status == EJournalStatus::Recorded && !bReplay;
	const bool bReplayed = Status == EJournalStatus::Replayed && bReplay;
	return (bRecorded || bReplayed)
		&& MatchesRecordedStatus(AdapterStatus, Receipt);
}

bool FJournalResult::IsReplay() const
{
	return Status == EJournalStatus::Replayed && bReplay && IsSuccess();
}

bool FJournal::IsValid() const
{
	if (Records.Num() != RecordIndexByCommandId.Num())
	{
		return false;
	}

	TSet<FGuid> SeenCommandIds;
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const FRecord& Record = Records[Index];
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
		const int32* StoredIndex = RecordIndexByCommandId.Find(CommandId);
		if (!StoredIndex || *StoredIndex != Index)
		{
			return false;
		}
	}
	return true;
}

bool FJournal::TryGetRecord(
	const FGuid& CommandId,
	FRecord& OutRecord) const
{
	OutRecord = FRecord();
	if (!CommandId.IsValid() || !IsValid())
	{
		return false;
	}

	const int32* StoredIndex = RecordIndexByCommandId.Find(CommandId);
	if (!StoredIndex || !Records.IsValidIndex(*StoredIndex))
	{
		return false;
	}
	const FRecord& Record = Records[*StoredIndex];
	if (!Record.IsValid()
		|| Record.GetCommand().GetCommandId() != CommandId)
	{
		return false;
	}
	OutRecord = Record;
	return true;
}

FJournalResult FJournal::TryExecute(
	const FCommand& Command,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FJournalResult Result;
	if (!IsValid())
	{
		Result.Status = EJournalStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Retry-step command journal state is inconsistent.");
		return Result;
	}
	if (!Command.IsValid())
	{
		Result.Diagnostic = TEXT(
			"Retry-step command journal requires one valid immutable command.");
		return Result;
	}

	if (const int32* StoredIndex =
			RecordIndexByCommandId.Find(Command.GetCommandId()))
	{
		if (!Records.IsValidIndex(*StoredIndex)
			|| !Records[*StoredIndex].IsValid())
		{
			Result.Status = EJournalStatus::StateInvalid;
			Result.Diagnostic = TEXT(
				"Retry-step command journal index does not resolve a valid record.");
			return Result;
		}

		const FRecord& Record = Records[*StoredIndex];
		if (!Record.GetCommand().Matches(Command))
		{
			Result.Status = EJournalStatus::CommandReplayConflict;
			Result.Diagnostic = TEXT(
				"CommandId already belongs to another immutable retry-step command.");
			return Result;
		}

		Result.Status = EJournalStatus::Replayed;
		Result.Diagnostic = TEXT(
			"Returned the previously recorded immutable retry-step receipt.");
		Result.AdapterStatus = Record.GetAdapterStatus();
		Result.Receipt = Record.GetReceipt();
		Result.bReplay = true;
		if (!Result.IsSuccess())
		{
			Result = FJournalResult();
			Result.Status = EJournalStatus::StateInvalid;
			Result.Diagnostic = TEXT(
				"Stored retry-step receipt failed replay validation.");
		}
		return Result;
	}

	const auto AdapterResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandAdapter::
			TryExecute(Command, Host, VisualExecutor, AudioExecutor);
	Result.AdapterStatus = AdapterResult.Status;
	Result.Diagnostic = AdapterResult.Diagnostic;
	if (!AdapterResult.HasReceipt())
	{
		Result.Status = EJournalStatus::AdapterRejected;
		return Result;
	}

	FRecord NewRecord;
	NewRecord.AdapterStatus = AdapterResult.Status;
	NewRecord.Receipt = AdapterResult.Receipt;
	if (!NewRecord.IsValid())
	{
		Result.Status = EJournalStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Retry-step adapter receipt cannot form a valid journal record.");
		Result.AdapterStatus = EAdapterStatus::ReceiptInvalid;
		return Result;
	}

	FJournal Candidate = *this;
	const int32 NewIndex = Candidate.Records.Add(NewRecord);
	Candidate.RecordIndexByCommandId.Add(Command.GetCommandId(), NewIndex);
	if (!Candidate.IsValid())
	{
		Result.Status = EJournalStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Retry-step command journal rejected an inconsistent candidate state.");
		return Result;
	}

	*this = MoveTemp(Candidate);
	Result.Status = EJournalStatus::Recorded;
	Result.Receipt = NewRecord.Receipt;
	if (!Result.IsSuccess())
	{
		Result.Status = EJournalStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Recorded retry-step command result failed validation.");
		Result.Receipt = FReceipt();
	}
	return Result;
}
