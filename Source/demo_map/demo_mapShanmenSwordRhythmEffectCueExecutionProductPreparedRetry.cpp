#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FPrepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry;
	using FPrepareResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareResult;
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult;
	using EChannels =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryChannels;

	enum class EHostCompatibility : uint8
	{
		Invalid,
		Fresh,
		ProcessCommitted,
		Completed
	};

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool RenewsVisual(const EChannels Channels)
	{
		return Channels == EChannels::Visual
			|| Channels == EChannels::VisualAndAudio;
	}

	bool RenewsAudio(const EChannels Channels)
	{
		return Channels == EChannels::Audio
			|| Channels == EChannels::VisualAndAudio;
	}

	bool IsRenewalChannels(const EChannels Channels)
	{
		return Channels == EChannels::Visual
			|| Channels == EChannels::Audio
			|| Channels == EChannels::VisualAndAudio;
	}

	EChannels MakeChannels(
		const bool bVisualPending,
		const bool bAudioPending)
	{
		if (bVisualPending && bAudioPending)
		{
			return EChannels::VisualAndAudio;
		}
		if (bVisualPending)
		{
			return EChannels::Visual;
		}
		return bAudioPending ? EChannels::Audio : EChannels::None;
	}

	FGuid DeriveRoleId(
		const TCHAR* Role,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed&
			Seed,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Source)
	{
		const auto& Projection = PreparedDispatch.GetProjection();
		const auto& PlanSeed = PreparedDispatch.GetPlan().GetSeed();
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionProductPreparedRetry.r1"),
			{
				Role,
				GuidDigits(Seed.RetrySeed),
				GuidDigits(PlanSeed.DispatchSeed),
				GuidDigits(PlanSeed.VisualConsumerScopeId),
				GuidDigits(PlanSeed.AudioConsumerScopeId),
				GuidDigits(Projection.GetRunId()),
				GuidDigits(Projection.GetConfigId()),
				GuidDigits(Projection.GetCuePolicyId()),
				GuidDigits(Projection.GetEvent().GetEventId()),
				FString::FromInt(Projection.GetObservationRevision()),
				GuidDigits(Source.GetDispatchId()),
				FString::Printf(
					TEXT("%lld"),
					static_cast<long long>(Source.GetSequence()))
			});
	}

	TArray<FGuid> RootIdentities(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Source)
	{
		const auto& Identity =
			PreparedDispatch.GetPlan().GetTransactionIdentity();
		const auto& SourceCommand = Source.GetCommand();
		return {
			Identity.HostId,
			Identity.CreateCommandId,
			Identity.ProcessCommandId,
			Identity.EndCommandId,
			Identity.VisualConsumerId,
			Identity.AudioConsumerId,
			Identity.VisualAttemptId,
			Identity.AudioAttemptId,
			Source.GetDispatchId(),
			SourceCommand.GetCommandId(),
			SourceCommand.GetVisualAttemptId(),
			SourceCommand.GetAudioAttemptId()
		};
	}

	bool IsDistinctFrom(
		const FGuid& Value,
		const TArray<FGuid>& Existing)
	{
		return Value.IsValid() && !Existing.Contains(Value);
	}

	bool TryBuildRenewal(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed&
			Seed,
		const EChannels Channels,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& Source,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutProcess,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope& OutEnd)
	{
		OutProcess =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope();
		OutEnd = Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope();
		if (!PreparedDispatch.IsValid()
			|| !Seed.IsValid()
			|| !IsRenewalChannels(Channels)
			|| !Source.IsValid()
			|| Source.GetSequence() < 1
			|| Source.GetSequence() > MAX_int32 - 2
			|| Source.GetCommand().GetKind()
				!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::
					ProcessNext)
		{
			return false;
		}

		const auto& SourceCommand = Source.GetCommand();
		const FGuid ProcessCommandId = DeriveRoleId(
			TEXT("Command.ProcessNext"), PreparedDispatch, Seed, Source);
		const FGuid EndCommandId =
			DeriveRoleId(TEXT("Command.End"), PreparedDispatch, Seed, Source);
		const FGuid VisualAttemptId = RenewsVisual(Channels)
			? DeriveRoleId(TEXT("Attempt.Visual"), PreparedDispatch, Seed, Source)
			: SourceCommand.GetVisualAttemptId();
		const FGuid AudioAttemptId = RenewsAudio(Channels)
			? DeriveRoleId(TEXT("Attempt.Audio"), PreparedDispatch, Seed, Source)
			: SourceCommand.GetAudioAttemptId();
		const TArray<FGuid> Existing = RootIdentities(PreparedDispatch, Source);
		if (!IsDistinctFrom(ProcessCommandId, Existing)
			|| !IsDistinctFrom(EndCommandId, Existing)
			|| ProcessCommandId == EndCommandId
			|| VisualAttemptId == AudioAttemptId
			|| (RenewsVisual(Channels)
				&& (!IsDistinctFrom(VisualAttemptId, Existing)
					|| VisualAttemptId == ProcessCommandId
					|| VisualAttemptId == EndCommandId))
			|| (RenewsAudio(Channels)
				&& (!IsDistinctFrom(AudioAttemptId, Existing)
					|| AudioAttemptId == ProcessCommandId
					|| AudioAttemptId == EndCommandId
					|| (RenewsVisual(Channels)
						&& AudioAttemptId == VisualAttemptId))))
		{
			return false;
		}

		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand ProcessCommand;
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand EndCommand;
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureProcessNext(
					ProcessCommandId,
					SourceCommand.GetRunId(),
					SourceCommand.GetBatchId(),
					VisualAttemptId,
					AudioAttemptId,
					ProcessCommand)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(
					Source.GetHostId(),
					Source.GetSequence() + 1,
					ProcessCommand,
					OutProcess)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
				EndCommandId,
				SourceCommand.GetRunId(),
				SourceCommand.GetBatchId(),
				EndCommand)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(
					Source.GetHostId(),
					Source.GetSequence() + 2,
					EndCommand,
					OutEnd);
	}

	bool RecordMatches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord*
			OutRecord = nullptr)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Record;
		if (!Host.TryGetRecord(Envelope.GetSequence(), Record)
			|| !Record.Envelope.Matches(Envelope))
		{
			return false;
		}
		if (OutRecord)
		{
			*OutRecord = MoveTemp(Record);
		}
		return true;
	}

	bool MatchesPreparedRoot(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host)
	{
		if (!PreparedDispatch.IsValid() || !Host.IsValid() || !Host.IsBound())
		{
			return false;
		}
		const auto Root =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
				Capture(
					PreparedDispatch.GetProjection(),
					PreparedDispatch.GetPlan().GetTransactionIdentity());
		if (!Root.IsCaptured())
		{
			return false;
		}
		const auto& CreateEnvelope = Root.Request.GetCreateRequest().GetEnvelope();
		return Host.GetHostId() == CreateEnvelope.GetHostId()
			&& Host.GetRunId() == CreateEnvelope.GetCommand().GetRunId()
			&& Host.GetBatchId() == CreateEnvelope.GetCommand().GetBatchId()
			&& RecordMatches(Host, CreateEnvelope)
			&& RecordMatches(Host, Root.Request.GetProcessEnvelope());
	}

	bool IsRetryPendingRecord(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord&
			Record,
		EChannels& OutChannels)
	{
		OutChannels = EChannels::None;
		if (!Record.IsValid()
			|| !Record.Result.IsSuccess()
			|| Record.Envelope.GetCommand().GetKind()
				!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::
					ProcessNext
			|| Record.Result.Router.Session.Status
				!= Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending
			|| !Record.Result.Router.Session.Host.IsSuccess())
		{
			return false;
		}
		const auto& HostResult = Record.Result.Router.Session.Host;
		OutChannels = MakeChannels(
			!HostResult.Visual.IsAcknowledged(),
			!HostResult.Audio.IsAcknowledged());
		return IsRenewalChannels(OutChannels);
	}

	EHostCompatibility ClassifyHost(
		const FPrepared& Prepared,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host)
	{
		if (!Prepared.IsValid()
			|| !MatchesPreparedRoot(Prepared.GetPreparedDispatch(), Host)
			|| !RecordMatches(Host, Prepared.GetSourceProcessEnvelope()))
		{
			return EHostCompatibility::Invalid;
		}

		const int64 ProcessSequence =
			Prepared.GetRetryProcessEnvelope().GetSequence();
		const int64 EndSequence = Prepared.GetEndEnvelope().GetSequence();
		if (!Host.IsTerminal() && Host.GetNextSequence() == ProcessSequence)
		{
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Source;
			EChannels Channels = EChannels::None;
			return Prepared.GetSourceProcessEnvelope().GetSequence()
						== ProcessSequence - 1
					&& RecordMatches(
						Host, Prepared.GetSourceProcessEnvelope(), &Source)
					&& IsRetryPendingRecord(Source, Channels)
					&& Channels == Prepared.GetRenewedChannels()
				? EHostCompatibility::Fresh
				: EHostCompatibility::Invalid;
		}
		if (!Host.IsTerminal() && Host.GetNextSequence() == EndSequence)
		{
			return RecordMatches(Host, Prepared.GetRetryProcessEnvelope())
				? EHostCompatibility::ProcessCommitted
				: EHostCompatibility::Invalid;
		}
		if (Host.IsTerminal() && Host.GetNextSequence() == EndSequence + 1)
		{
			return RecordMatches(Host, Prepared.GetRetryProcessEnvelope())
					&& RecordMatches(Host, Prepared.GetEndEnvelope())
				? EHostCompatibility::Completed
				: EHostCompatibility::Invalid;
		}
		return EHostCompatibility::Invalid;
	}

	void CaptureFinalState(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		FResult& Result)
	{
		Result.FinalNextSequence = Host.GetNextSequence();
		Result.FinalRecordCount = Host.GetRecordCount();
		Result.bFinalTerminal = Host.IsTerminal();
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry::
RenewsVisual() const
{
	return ::RenewsVisual(RenewedChannels);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry::
RenewsAudio() const
{
	return ::RenewsAudio(RenewedChannels);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry::
IsValid() const
{
	if (!PreparedDispatch.IsValid()
		|| !Seed.IsValid()
		|| !IsRenewalChannels(RenewedChannels)
		|| !SourceProcessEnvelope.IsValid())
	{
		return false;
	}
	const auto Root =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(
				PreparedDispatch.GetProjection(),
				PreparedDispatch.GetPlan().GetTransactionIdentity());
	if (!Root.IsCaptured()
		|| SourceProcessEnvelope.GetHostId()
			!= Root.Request.GetProcessEnvelope().GetHostId()
		|| SourceProcessEnvelope.GetCommand().GetRunId()
			!= Root.Request.GetProcessEnvelope().GetCommand().GetRunId()
		|| SourceProcessEnvelope.GetCommand().GetBatchId()
			!= Root.Request.GetProcessEnvelope().GetCommand().GetBatchId()
		|| (SourceProcessEnvelope.GetSequence() == 1
			&& !SourceProcessEnvelope.Matches(
				Root.Request.GetProcessEnvelope())))
	{
		return false;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope ExpectedProcess;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope ExpectedEnd;
	return TryBuildRenewal(
			PreparedDispatch,
			Seed,
			RenewedChannels,
			SourceProcessEnvelope,
			ExpectedProcess,
			ExpectedEnd)
		&& ExpectedProcess.Matches(RetryProcessEnvelope)
		&& ExpectedEnd.Matches(EndEnvelope);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry& Other)
	const
{
	return IsValid()
		&& Other.IsValid()
		&& PreparedDispatch.Matches(Other.PreparedDispatch)
		&& Seed.RetrySeed == Other.Seed.RetrySeed
		&& RenewedChannels == Other.RenewedChannels
		&& SourceProcessEnvelope.Matches(Other.SourceProcessEnvelope)
		&& RetryProcessEnvelope.Matches(Other.RetryProcessEnvelope)
		&& EndEnvelope.Matches(Other.EndEnvelope);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareResult::
IsPrepared() const
{
	return Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				Prepared
		&& Prepared.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult::
IsCompleted() const
{
	const bool bStatusValid = Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				Completed
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				Resumed
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				Replayed;
	return bStatusValid
		&& Prepared.IsValid()
		&& Process.IsSuccess()
		&& Process.Router.Session.IsBatchComplete()
		&& End.IsSuccess()
		&& FinalNextSequence == Prepared.GetEndEnvelope().GetSequence() + 1
		&& FinalRecordCount == FinalNextSequence
		&& bFinalTerminal
		&& bAnyReplay == (Process.bReplay || End.bReplay)
		&& bAllReplay == (Process.bReplay && End.bReplay)
		&& (Status
				!= Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
					Replayed
			|| bAllReplay);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult::
HasDurableProgress() const
{
	return Prepared.IsValid()
		&& FinalRecordCount
			> Prepared.GetRetryProcessEnvelope().GetSequence()
		&& FinalNextSequence == FinalRecordCount;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
PrepareRetry(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
		PreparedDispatch,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed&
		Seed)
{
	FPrepareResult Result;
	if (!PreparedDispatch.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				PreparedDispatchInvalid;
		Result.Diagnostic = TEXT(
			"Prepared retry requires one valid frozen Product dispatch root.");
		return Result;
	}
	if (!Seed.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				SeedInvalid;
		Result.Diagnostic = TEXT(
			"Prepared retry requires one explicit caller-owned retry seed.");
		return Result;
	}
	if (!Host.IsValid() || !Host.IsBound())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				HostInvalid;
		Result.Diagnostic = TEXT(
			"Prepared retry requires one valid bound command Host.");
		return Result;
	}
	if (!MatchesPreparedRoot(PreparedDispatch, Host))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				PreparedRootMismatch;
		Result.Diagnostic = TEXT(
			"Command Host does not retain the prepared dispatch's exact Create and initial Process root.");
		return Result;
	}
	if (Host.IsTerminal() || Host.GetNextSequence() < 2)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				RetryStateUnavailable;
		Result.Diagnostic = TEXT(
			"Command Host has no active retry-pending continuation.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Source;
	EChannels Channels = EChannels::None;
	if (!Host.TryGetRecord(Host.GetNextSequence() - 1, Source)
		|| !IsRetryPendingRecord(Source, Channels))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				RetryStateUnavailable;
		Result.Diagnostic = TEXT(
			"Latest durable Host receipt is not a retry-pending ProcessNext.");
		return Result;
	}

	Result.Prepared.PreparedDispatch = PreparedDispatch;
	Result.Prepared.Seed = Seed;
	Result.Prepared.RenewedChannels = Channels;
	Result.Prepared.SourceProcessEnvelope = Source.Envelope;
	if (!TryBuildRenewal(
			PreparedDispatch,
			Seed,
			Channels,
			Source.Envelope,
			Result.Prepared.RetryProcessEnvelope,
			Result.Prepared.EndEnvelope))
	{
		Result.Prepared = FPrepared();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				CaptureRejected;
		Result.Diagnostic = TEXT(
			"Retry renewal could not derive distinct commands, attempts and contiguous envelopes.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
			Prepared;
	if (!Result.IsPrepared()
		|| ClassifyHost(Result.Prepared, Host) != EHostCompatibility::Fresh)
	{
		Result.Prepared = FPrepared();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Prepared retry failed frozen-value or live-source consistency validation.");
		return Result;
	}
	Result.Diagnostic = TEXT(
		"Prepared one caller-owned retry continuation without Host mutation.");
	return Result;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
TryExecutePreparedRetry(
	const FPrepared& Prepared,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FResult Result;
	Result.Prepared = Prepared;
	if (!Result.Prepared.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				PreparedRejected;
		Result.Diagnostic = TEXT(
			"Prepared retry execution requires one valid immutable continuation.");
		CaptureFinalState(Host, Result);
		return Result;
	}
	if (ClassifyHost(Result.Prepared, Host) == EHostCompatibility::Invalid)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				HostStateMismatch;
		Result.Diagnostic = TEXT(
			"Command Host is not at this prepared retry's fresh, process-committed or completed fence.");
		CaptureFinalState(Host, Result);
		return Result;
	}

	Result.Process = Host.TryRouteProcessNext(
		Result.Prepared.GetRetryProcessEnvelope(),
		VisualExecutor,
		AudioExecutor);
	if (!Result.Process.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				ProcessRejected;
		Result.bAnyReplay = Result.Process.bReplay;
		Result.Diagnostic = Result.Process.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}
	if (!Result.Process.Router.Session.IsBatchComplete())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				RetryPending;
		Result.bAnyReplay = Result.Process.bReplay;
		Result.Diagnostic = Result.Process.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}

	Result.End = Host.TryRoute(Result.Prepared.GetEndEnvelope());
	Result.bAnyReplay = Result.Process.bReplay || Result.End.bReplay;
	Result.bAllReplay = Result.Process.bReplay && Result.End.bReplay;
	if (!Result.End.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				EndRejected;
		Result.Diagnostic = Result.End.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}

	CaptureFinalState(Host, Result);
	if (!Host.IsValid()
		|| !Host.IsTerminal()
		|| !RecordMatches(Host, Result.Prepared.GetRetryProcessEnvelope())
		|| !RecordMatches(Host, Result.Prepared.GetEndEnvelope())
		|| Result.FinalNextSequence
			!= Result.Prepared.GetEndEnvelope().GetSequence() + 1
		|| Result.FinalRecordCount != Result.FinalNextSequence)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Prepared retry completed without its exact terminal Host proof.");
		return Result;
	}

	Result.Status = Result.bAllReplay
		? Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
			Replayed
		: Result.bAnyReplay
			? Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				Resumed
			: Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus::
				Completed;
	Result.Diagnostic = Result.bAllReplay
		? TEXT("Replayed one completed prepared retry without executor re-entry.")
		: Result.bAnyReplay
			? TEXT("Resumed one prepared retry from durable Process evidence.")
			: TEXT("Completed one caller-driven prepared retry continuation.");
	return Result;
}
