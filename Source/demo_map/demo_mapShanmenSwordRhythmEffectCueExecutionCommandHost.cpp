#include "demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeDispatchId(
		const FGuid& HostId,
		const int64 Sequence,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& Command)
	{
		if (!HostId.IsValid()
			|| Sequence < 0
			|| Sequence > MAX_int32
			|| !Command.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionCommandDispatch.r1"),
			{
				GuidDigits(HostId),
				FString::Printf(TEXT("%lld"), static_cast<long long>(Sequence)),
				GuidDigits(Command.GetCommandId()),
				FString::FromInt(static_cast<uint8>(Command.GetKind())),
				GuidDigits(Command.GetRunId()),
				GuidDigits(Command.GetBatchId()),
				GuidDigits(Command.GetVisualAttemptId()),
				GuidDigits(Command.GetAudioAttemptId())
			});
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Reject(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope,
		const Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus
			Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Result;
		Result.Status = Status;
		Result.HostId = Envelope.GetHostId();
		Result.Sequence = Envelope.GetSequence();
		Result.DispatchId = Envelope.GetDispatchId();
		Result.CommandId = Envelope.GetCommand().GetCommandId();
		Result.RunId = Envelope.GetCommand().GetRunId();
		Result.BatchId = Envelope.GetCommand().GetBatchId();
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::TryCapture(
	const FGuid& RequestedHostId,
	const int64 RequestedSequence,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand& RequestedCommand,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutEnvelope)
{
	OutEnvelope =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope();
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Candidate;
	Candidate.HostId = RequestedHostId;
	Candidate.Sequence = RequestedSequence;
	Candidate.Command = RequestedCommand;
	Candidate.DispatchId = MakeDispatchId(
		Candidate.HostId, Candidate.Sequence, Candidate.Command);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutEnvelope = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::IsValid()
	const
{
	return HostId.IsValid()
		&& Sequence >= 0
		&& Sequence <= MAX_int32
		&& Command.IsValid()
		&& DispatchId.IsValid()
		&& DispatchId == MakeDispatchId(HostId, Sequence, Command);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& HostId == Other.HostId
		&& Sequence == Other.Sequence
		&& DispatchId == Other.DispatchId
		&& Command.Matches(Other.Command);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult::
IsSuccess() const
{
	return Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				Routed
		&& HostId.IsValid()
		&& Sequence >= 0
		&& DispatchId.IsValid()
		&& CommandId.IsValid()
		&& RunId.IsValid()
		&& BatchId.IsValid()
		&& Router.IsSuccess()
		&& Router.CommandId == CommandId
		&& Router.RunId == RunId
		&& Router.BatchId == BatchId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult::
IsDurableRecord() const
{
	if (bReplay
		|| !bHostStateCommitted
		|| !HostId.IsValid()
		|| Sequence < 0
		|| !DispatchId.IsValid()
		|| !CommandId.IsValid()
		|| !RunId.IsValid()
		|| !BatchId.IsValid()
		|| !Router.IsDurableRecord()
		|| Router.CommandId != CommandId
		|| Router.RunId != RunId
		|| Router.BatchId != BatchId)
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::Routed)
	{
		return Router.IsSuccess();
	}
	return Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				RouterRejected
		&& !Router.IsSuccess();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord::IsValid()
	const
{
	return Envelope.IsValid()
		&& Result.IsDurableRecord()
		&& Result.HostId == Envelope.GetHostId()
		&& Result.Sequence == Envelope.GetSequence()
		&& Result.DispatchId == Envelope.GetDispatchId()
		&& Result.CommandId == Envelope.GetCommand().GetCommandId()
		&& Result.RunId == Envelope.GetCommand().GetRunId()
		&& Result.BatchId == Envelope.GetCommand().GetBatchId();
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::TryRoute(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Envelope)
{
	return TryRouteInternal(Envelope, nullptr, nullptr);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::
TryRouteProcessNext(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Envelope,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	return TryRouteInternal(Envelope, &VisualExecutor, &AudioExecutor);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::TryRouteInternal(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Envelope,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor* VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor* AudioExecutor)
{
	if (!Envelope.IsValid())
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				EnvelopeInvalid,
			TEXT("Cue command Host requires one valid frozen envelope."));
	}
	if (!IsValid())
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				HostInvalid,
			TEXT("Cue command Host is internally inconsistent."));
	}

	const auto Kind = Envelope.GetCommand().GetKind();
	const bool bHasExecutors = VisualExecutor && AudioExecutor;
	const bool bRequiresExecutors = Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::ProcessNext;
	if (bHasExecutors != bRequiresExecutors)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				ExecutorContextMismatch,
			TEXT("Cue command envelope used the wrong Host routing overload."));
	}

	if (Envelope.GetSequence() < NextSequence)
	{
		const auto& Existing = Records[static_cast<int32>(Envelope.GetSequence())];
		if (!Existing.Envelope.Matches(Envelope))
		{
			return Reject(
				Envelope,
				Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
					SequenceConflict,
				TEXT("Cue command sequence was reused with another envelope."));
		}
		auto Result = Existing.Result;
		Result.bReplay = true;
		Result.bHostStateCommitted = false;
		Result.Diagnostic =
			TEXT("Cue command envelope replayed its first durable Host receipt.");
		return Result;
	}
	if (Envelope.GetSequence() > NextSequence)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				SequenceGap,
			TEXT("Cue command envelope skipped the next contiguous sequence."));
	}
	if (bTerminal)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				LifecycleConflict,
			TEXT("Cue command Host terminal fence rejects new envelopes."));
	}
	if (IsEmpty() && Kind
		!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				LifecycleConflict,
			TEXT("Cue command Host sequence zero must create its batch."));
	}
	if (IsBound() && Envelope.GetHostId() != HostId)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				HostIdentityConflict,
			TEXT("Cue command envelope names a foreign HostId."));
	}
	if (IsBound()
		&& (Envelope.GetCommand().GetRunId() != RunId
			|| Envelope.GetCommand().GetBatchId() != BatchId))
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				LifecycleConflict,
			TEXT("Cue command envelope names a foreign batch lifecycle."));
	}
	if (IsBound() && Kind
		== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				LifecycleConflict,
			TEXT("Cue command Host already owns one batch lifecycle."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Candidate = *this;
	if (Candidate.IsEmpty())
	{
		Candidate.HostId = Envelope.GetHostId();
		Candidate.RunId = Envelope.GetCommand().GetRunId();
		Candidate.BatchId = Envelope.GetCommand().GetBatchId();
	}
	const auto RouterResult = bRequiresExecutors
		? Candidate.Router.TryRouteProcessNext(
			Envelope.GetCommand(), *VisualExecutor, *AudioExecutor)
		: Candidate.Router.TryRoute(Envelope.GetCommand());
	if (RouterResult.bReplay)
	{
		return Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				CommandReplayConflict,
			TEXT("Cue Router CommandId already belongs to another Host sequence."));
	}
	if (!RouterResult.IsDurableRecord())
	{
		auto Result = Reject(
			Envelope,
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				RouterRejected,
			RouterResult.Diagnostic);
		Result.Router = RouterResult;
		return Result;
	}
	return Commit(Envelope, RouterResult, MoveTemp(Candidate));
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::Commit(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Envelope,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandResult RouterResult,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Candidate)
{
	auto Result = Reject(
		Envelope,
		RouterResult.IsSuccess()
			? Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::Routed
			: Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				RouterRejected,
		RouterResult.Diagnostic);
	Result.Router = MoveTemp(RouterResult);
	Result.bHostStateCommitted = true;
	Candidate.bTerminal = Candidate.Router.IsEnded();
	++Candidate.NextSequence;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Record;
	Record.Envelope = Envelope;
	Record.Result = Result;
	Candidate.Records.Add(MoveTemp(Record));
	if (!Result.IsDurableRecord() || !Candidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
				StateInvalid;
		Result.bHostStateCommitted = false;
		Result.Diagnostic =
			TEXT("Cue command produced invalid staged Host state.");
		return Result;
	}
	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::IsValid() const
{
	if (!HostId.IsValid() && !RunId.IsValid() && !BatchId.IsValid())
	{
		return NextSequence == 0
			&& !bTerminal
			&& Router.IsValid()
			&& Router.IsEmpty()
			&& Records.IsEmpty();
	}
	if (!HostId.IsValid()
		|| !RunId.IsValid()
		|| !BatchId.IsValid()
		|| NextSequence <= 0
		|| NextSequence != Records.Num()
		|| !Router.IsValid()
		|| !Router.IsBound()
		|| Router.GetRunId() != RunId
		|| Router.GetBatchId() != BatchId
		|| Router.GetRecordCount() != Records.Num()
		|| bTerminal != Router.IsEnded())
	{
		return false;
	}

	TSet<FGuid> DispatchIds;
	TSet<FGuid> CommandIds;
	int32 TerminalCount = 0;
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Record = Records[Index];
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandRecord RouterRecord;
		if (!Record.IsValid()
			|| Record.Envelope.GetHostId() != HostId
			|| Record.Envelope.GetSequence() != Index
			|| Record.Envelope.GetCommand().GetRunId() != RunId
			|| Record.Envelope.GetCommand().GetBatchId() != BatchId
			|| DispatchIds.Contains(Record.Envelope.GetDispatchId())
			|| CommandIds.Contains(Record.Envelope.GetCommand().GetCommandId())
			|| !Router.TryGetRecord(
				Record.Envelope.GetCommand().GetCommandId(), RouterRecord)
			|| !RouterRecord.Command.Matches(Record.Envelope.GetCommand())
			|| RouterRecord.Result.Status != Record.Result.Router.Status)
		{
			return false;
		}
		DispatchIds.Add(Record.Envelope.GetDispatchId());
		CommandIds.Add(Record.Envelope.GetCommand().GetCommandId());
		if (Index == 0
			&& (Record.Envelope.GetCommand().GetKind()
					!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create
				|| !Record.Result.IsSuccess()))
		{
			return false;
		}
		if (Record.Result.Router.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandStatus::Ended)
		{
			++TerminalCount;
			if (Index != Records.Num() - 1)
			{
				return false;
			}
		}
	}
	return TerminalCount == (bTerminal ? 1 : 0);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::IsEmpty() const
{
	return IsValid()
		&& !HostId.IsValid()
		&& !RunId.IsValid()
		&& !BatchId.IsValid()
		&& NextSequence == 0
		&& !bTerminal
		&& Router.IsEmpty()
		&& Records.IsEmpty();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost::TryGetRecord(
	const int64 RequestedSequence,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord& OutRecord)
	const
{
	OutRecord = Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord();
	if (!IsValid()
		|| RequestedSequence < 0
		|| RequestedSequence >= Records.Num())
	{
		return false;
	}
	OutRecord = Records[static_cast<int32>(RequestedSequence)];
	return OutRecord.IsValid();
}
