#include "demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.h"

namespace
{
	bool IsCommandKind(
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind Kind)
	{
		return Kind
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create
			|| Kind
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::
					ProcessNext
			|| Kind
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::End;
	}

	bool EventsMatch(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Left,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!Left[Index].Matches(Right[Index]))
			{
				return false;
			}
		}
		return true;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Reject(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Result;
		Result.Status = Status;
		Result.CommandId = Command.GetCommandId();
		Result.Kind = Command.GetKind();
		Result.RunId = Command.GetRunId();
		Result.BatchId = Command.GetBatchId();
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureCreate(
	const FGuid& RequestedCommandId,
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& RequestedEvents,
	const FGuid& RequestedVisualConsumerId,
	const FGuid& RequestedAudioConsumerId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand();
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Candidate;
	if (!RequestedCommandId.IsValid()
		|| !Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate(
			RequestedEvents,
			RequestedVisualConsumerId,
			RequestedAudioConsumerId,
			Candidate))
	{
		return false;
	}
	OutCommand.CommandId = RequestedCommandId;
	OutCommand.Kind =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create;
	OutCommand.RunId = Candidate.GetRunId();
	OutCommand.BatchId = Candidate.GetBatchId();
	OutCommand.Events = RequestedEvents;
	OutCommand.VisualConsumerId = RequestedVisualConsumerId;
	OutCommand.AudioConsumerId = RequestedAudioConsumerId;
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
TryCaptureProcessNext(
	const FGuid& RequestedCommandId,
	const FGuid& RequestedRunId,
	const FGuid& RequestedBatchId,
	const FGuid& RequestedVisualAttemptId,
	const FGuid& RequestedAudioAttemptId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand();
	if (!RequestedCommandId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedBatchId.IsValid()
		|| !RequestedVisualAttemptId.IsValid()
		|| !RequestedAudioAttemptId.IsValid())
	{
		return false;
	}
	OutCommand.CommandId = RequestedCommandId;
	OutCommand.Kind =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::ProcessNext;
	OutCommand.RunId = RequestedRunId;
	OutCommand.BatchId = RequestedBatchId;
	OutCommand.VisualAttemptId = RequestedVisualAttemptId;
	OutCommand.AudioAttemptId = RequestedAudioAttemptId;
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
	const FGuid& RequestedCommandId,
	const FGuid& RequestedRunId,
	const FGuid& RequestedBatchId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand();
	if (!RequestedCommandId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedBatchId.IsValid())
	{
		return false;
	}
	OutCommand.CommandId = RequestedCommandId;
	OutCommand.Kind =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::End;
	OutCommand.RunId = RequestedRunId;
	OutCommand.BatchId = RequestedBatchId;
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::IsValid() const
{
	if (!CommandId.IsValid()
		|| !RunId.IsValid()
		|| !BatchId.IsValid()
		|| !IsCommandKind(Kind))
	{
		return false;
	}
	if (Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Candidate;
		return !VisualAttemptId.IsValid()
			&& !AudioAttemptId.IsValid()
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate(
				Events,
				VisualConsumerId,
				AudioConsumerId,
				Candidate)
			&& Candidate.GetRunId() == RunId
			&& Candidate.GetBatchId() == BatchId;
	}
	if (!Events.IsEmpty()
		|| VisualConsumerId.IsValid()
		|| AudioConsumerId.IsValid())
	{
		return false;
	}
	return Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::ProcessNext
		? VisualAttemptId.IsValid() && AudioAttemptId.IsValid()
		: !VisualAttemptId.IsValid() && !AudioAttemptId.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| CommandId != Other.CommandId
		|| Kind != Other.Kind
		|| RunId != Other.RunId
		|| BatchId != Other.BatchId)
	{
		return false;
	}
	if (Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create)
	{
		return VisualConsumerId == Other.VisualConsumerId
			&& AudioConsumerId == Other.AudioConsumerId
			&& EventsMatch(Events, Other.Events);
	}
	if (Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::ProcessNext)
	{
		return VisualAttemptId == Other.VisualAttemptId
			&& AudioAttemptId == Other.AudioAttemptId;
	}
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult::
IsSuccess() const
{
	if (!CommandId.IsValid()
		|| !RunId.IsValid()
		|| !BatchId.IsValid()
		|| !IsCommandKind(Kind)
		|| TotalEvents <= 0
		|| CompletedEvents < 0
		|| CompletedEvents > TotalEvents)
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Created:
		return Kind
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create
			&& CompletedEvents == 0;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Processed:
		return Kind
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::
					ProcessNext
			&& Session.IsSuccess()
			&& Session.CompletedEvents == CompletedEvents
			&& Session.TotalEvents == TotalEvents;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Ended:
		return Kind
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::End
			&& CompletedEvents == TotalEvents;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult::
IsDurableRecord() const
{
	if (!CommandId.IsValid()
		|| !RunId.IsValid()
		|| !BatchId.IsValid()
		|| !IsCommandKind(Kind)
		|| bReplay
		|| !bRouterStateCommitted
		|| TotalEvents <= 0
		|| CompletedEvents < 0
		|| CompletedEvents > TotalEvents)
	{
		return false;
	}
	if (IsSuccess())
	{
		return true;
	}
	if (Status
		!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
			SessionRejected)
	{
		return false;
	}
	if (Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::ProcessNext)
	{
		return Session.Status
				== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventRejected
			&& Session.Event.IsValid()
			&& Session.EventIndex == CompletedEvents
			&& Session.CompletedEvents == CompletedEvents
			&& Session.TotalEvents == TotalEvents;
	}
	return Kind
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::End
		&& CompletedEvents < TotalEvents
		&& !Diagnostic.IsEmpty();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord::IsValid() const
{
	return Command.IsValid()
		&& Result.IsDurableRecord()
		&& Result.CommandId == Command.GetCommandId()
		&& Result.Kind == Command.GetKind()
		&& Result.RunId == Command.GetRunId()
		&& Result.BatchId == Command.GetBatchId();
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::TryRoute(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command)
{
	return TryRouteInternal(Command, nullptr, nullptr);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::
TryRouteProcessNext(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	return TryRouteInternal(Command, &VisualExecutor, &AudioExecutor);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::TryRouteInternal(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor* VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor* AudioExecutor)
{
	if (!Command.IsValid())
	{
		return Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				CommandInvalid,
			TEXT("Cue execution routing requires one valid typed command."));
	}
	if (!IsValid())
	{
		return Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				RouterInvalid,
			TEXT("Cue execution command Router is internally inconsistent."));
	}
	const bool bHasExecutors = VisualExecutor && AudioExecutor;
	const bool bRequiresExecutors = Command.GetKind()
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::
			ProcessNext;
	if (bHasExecutors != bRequiresExecutors)
	{
		return Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				ExecutorContextMismatch,
			TEXT("Cue execution command used the wrong routing overload."));
	}

	for (const auto& Existing : Records)
	{
		if (Existing.Command.GetCommandId() != Command.GetCommandId())
		{
			continue;
		}
		if (!Existing.Command.Matches(Command))
		{
			return Reject(
				Command,
				Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					CommandIdConflict,
				TEXT("Cue execution CommandId was reused with another payload."));
		}
		auto Result = Existing.Result;
		Result.bReplay = true;
		Result.bRouterStateCommitted = false;
		Result.Diagnostic =
			TEXT("Cue execution command replayed its first durable receipt.");
		return Result;
	}

	if (Command.GetKind()
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create)
	{
		if (IsBound())
		{
			return Reject(
				Command,
				Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					LifecycleConflict,
				TEXT("Cue execution Router already owns one batch lifecycle."));
		}
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Candidate =
			*this;
		if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate(
				Command.GetEvents(),
				Command.GetVisualConsumerId(),
				Command.GetAudioConsumerId(),
				Candidate.Session))
		{
			return Reject(
				Command,
				Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					StateInvalid,
				TEXT("Cue execution Create could not stage its frozen session."));
		}
		Candidate.RunId = Candidate.Session.GetRunId();
		Candidate.BatchId = Candidate.Session.GetBatchId();
		Candidate.TotalEvents = Candidate.Session.NumEvents();
		auto Result = Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Created,
			TEXT("Cue execution batch lifecycle was created."));
		Result.CompletedEvents = 0;
		Result.TotalEvents = Candidate.TotalEvents;
		return Commit(Command, MoveTemp(Result), MoveTemp(Candidate));
	}

	if (!IsBound() || bEnded || !Session.IsValid())
	{
		return Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				LifecycleConflict,
			TEXT("Cue execution command requires one active batch lifecycle."));
	}
	if (Command.GetRunId() != RunId || Command.GetBatchId() != BatchId)
	{
		return Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				IdentityMismatch,
			TEXT("Cue execution command names a stale or foreign batch."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Candidate = *this;
	if (Command.GetKind()
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::ProcessNext)
	{
		auto Result = Reject(
			Command,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				StateInvalid,
			TEXT("Cue execution ProcessNext produced no session receipt."));
		Result.Session = Candidate.Session.ProcessNext(
			Command.GetVisualAttemptId(),
			Command.GetAudioAttemptId(),
			*VisualExecutor,
			*AudioExecutor);
		Result.CompletedEvents = Result.Session.CompletedEvents;
		Result.TotalEvents = Result.Session.TotalEvents;
		Result.Diagnostic = Result.Session.Diagnostic;
		if (Result.Session.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					Processed;
		}
		else if (Result.Session.Status
			== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventRejected)
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
					SessionRejected;
		}
		else
		{
			return Result;
		}
		return Commit(Command, MoveTemp(Result), MoveTemp(Candidate));
	}

	FString EndDiagnostic;
	const int32 CompletedBeforeEnd = Candidate.Session.NumCompletedEvents();
	const bool bEndedNow = Candidate.Session.TryEnd(RunId, EndDiagnostic);
	auto Result = Reject(
		Command,
		bEndedNow
			? Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Ended
			: Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
				SessionRejected,
		MoveTemp(EndDiagnostic));
	Result.CompletedEvents = bEndedNow ? TotalEvents : CompletedBeforeEnd;
	Result.TotalEvents = TotalEvents;
	Candidate.bEnded = bEndedNow;
	return Commit(Command, MoveTemp(Result), MoveTemp(Candidate));
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::Commit(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult Result,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter Candidate)
{
	Result.bRouterStateCommitted = true;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord Record;
	Record.Command = Command;
	Record.Result = Result;
	Candidate.Records.Add(MoveTemp(Record));
	if (!Result.IsDurableRecord() || !Candidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::StateInvalid;
		Result.bRouterStateCommitted = false;
		Result.Diagnostic =
			TEXT("Cue execution command produced invalid staged Router state.");
		return Result;
	}
	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::IsValid() const
{
	if (!RunId.IsValid() && !BatchId.IsValid())
	{
		return TotalEvents == 0
			&& !bEnded
			&& Session.IsEmpty()
			&& Records.IsEmpty();
	}
	if (!RunId.IsValid()
		|| !BatchId.IsValid()
		|| TotalEvents <= 0
		|| Records.IsEmpty())
	{
		return false;
	}
	if (bEnded)
	{
		if (!Session.IsEmpty())
		{
			return false;
		}
	}
	else if (!Session.IsValid()
		|| Session.GetRunId() != RunId
		|| Session.GetBatchId() != BatchId
		|| Session.NumEvents() != TotalEvents)
	{
		return false;
	}

	TSet<FGuid> CommandIds;
	int32 CreateCount = 0;
	int32 EndedCount = 0;
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Record = Records[Index];
		if (!Record.IsValid()
			|| Record.Command.GetRunId() != RunId
			|| Record.Command.GetBatchId() != BatchId
			|| Record.Result.TotalEvents != TotalEvents
			|| CommandIds.Contains(Record.Command.GetCommandId()))
		{
			return false;
		}
		CommandIds.Add(Record.Command.GetCommandId());
		if (Record.Command.GetKind()
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create)
		{
			++CreateCount;
			if (Index != 0
				|| Record.Result.Status
					!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::
						Created)
			{
				return false;
			}
		}
		if (Record.Result.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Ended)
		{
			++EndedCount;
			if (Index != Records.Num() - 1)
			{
				return false;
			}
		}
	}
	return CreateCount == 1
		&& EndedCount == (bEnded ? 1 : 0);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::IsEmpty() const
{
	return !RunId.IsValid()
		&& !BatchId.IsValid()
		&& TotalEvents == 0
		&& !bEnded
		&& Session.IsEmpty()
		&& Records.IsEmpty();
}

int32 Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::
NumCompletedEvents() const
{
	if (!IsValid())
	{
		return 0;
	}
	return bEnded ? TotalEvents : Session.NumCompletedEvents();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter::TryGetRecord(
	const FGuid& RequestedCommandId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord& OutRecord) const
{
	OutRecord = Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord();
	if (!RequestedCommandId.IsValid() || !IsValid())
	{
		return false;
	}
	for (const auto& Record : Records)
	{
		if (Record.Command.GetCommandId() == RequestedCommandId)
		{
			OutRecord = Record;
			return OutRecord.IsValid();
		}
	}
	return false;
}
