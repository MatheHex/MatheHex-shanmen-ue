#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.h"

namespace
{
	using FCapture =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;
	using FRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest;
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult;

	bool AreDistinct(const FGuid& A, const FGuid& B, const FGuid& C)
	{
		return A.IsValid() && B.IsValid() && C.IsValid()
			&& A != B && A != C && B != C;
	}

	void CaptureFinalState(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		FResult& Result)
	{
		Result.FinalNextSequence = Host.GetNextSequence();
		Result.FinalRecordCount = Host.GetRecordCount();
		Result.bFinalTerminal = Host.IsTerminal();
		if (Host.IsBound())
		{
			Result.BatchId = Host.GetBatchId();
		}
	}

	bool RecordMatches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const int64 Sequence,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			Envelope)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Record;
		return Host.TryGetRecord(Sequence, Record)
			&& Record.Envelope.Matches(Envelope);
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture::
IsValid() const
{
	return HostId.IsValid()
		&& CreateSequence == 0
		&& ProcessSequence == 1
		&& EndSequence == 2
		&& AreDistinct(CreateCommandId, ProcessCommandId, EndCommandId)
		&& VisualConsumerId.IsValid()
		&& AudioConsumerId.IsValid()
		&& VisualConsumerId != AudioConsumerId
		&& VisualAttemptId.IsValid()
		&& AudioAttemptId.IsValid()
		&& VisualAttemptId != AudioAttemptId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest::
IsValid() const
{
	if (!Projection.IsValid()
		|| !CreateRequest.IsValid()
		|| CreateRequest.GetProjections().Num() != 1
		|| !CreateRequest.GetProjections()[0].Matches(Projection)
		|| !ProcessEnvelope.IsValid()
		|| !EndEnvelope.IsValid())
	{
		return false;
	}

	const auto& CreateEnvelope = CreateRequest.GetEnvelope();
	const auto& CreateCommand = CreateEnvelope.GetCommand();
	const auto& ProcessCommand = ProcessEnvelope.GetCommand();
	const auto& EndCommand = EndEnvelope.GetCommand();
	if (CreateEnvelope.GetSequence() != 0
		|| ProcessEnvelope.GetSequence() != 1
		|| EndEnvelope.GetSequence() != 2
		|| CreateEnvelope.GetHostId() != ProcessEnvelope.GetHostId()
		|| CreateEnvelope.GetHostId() != EndEnvelope.GetHostId()
		|| CreateCommand.GetKind()
			!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create
		|| ProcessCommand.GetKind()
			!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::
				ProcessNext
		|| EndCommand.GetKind()
			!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::End
		|| !AreDistinct(
			CreateCommand.GetCommandId(),
			ProcessCommand.GetCommandId(),
			EndCommand.GetCommandId())
		|| ProcessCommand.GetRunId() != Projection.GetRunId()
		|| EndCommand.GetRunId() != Projection.GetRunId()
		|| ProcessCommand.GetBatchId() != CreateCommand.GetBatchId()
		|| EndCommand.GetBatchId() != CreateCommand.GetBatchId()
		|| !ProcessCommand.GetVisualAttemptId().IsValid()
		|| !ProcessCommand.GetAudioAttemptId().IsValid()
		|| ProcessCommand.GetVisualAttemptId()
			== ProcessCommand.GetAudioAttemptId())
	{
		return false;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand ExpectedProcess;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand ExpectedEnd;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope
		ExpectedProcessEnvelope;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope
		ExpectedEndEnvelope;
	return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureProcessNext(
				ProcessCommand.GetCommandId(),
				CreateCommand.GetRunId(),
				CreateCommand.GetBatchId(),
				ProcessCommand.GetVisualAttemptId(),
				ProcessCommand.GetAudioAttemptId(),
				ExpectedProcess)
		&& ExpectedProcess.Matches(ProcessCommand)
		&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
			TryCapture(
				CreateEnvelope.GetHostId(),
				1,
				ExpectedProcess,
				ExpectedProcessEnvelope)
		&& ExpectedProcessEnvelope.Matches(ProcessEnvelope)
		&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
			EndCommand.GetCommandId(),
			CreateCommand.GetRunId(),
			CreateCommand.GetBatchId(),
			ExpectedEnd)
		&& ExpectedEnd.Matches(EndCommand)
		&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
			TryCapture(
				CreateEnvelope.GetHostId(), 2, ExpectedEnd, ExpectedEndEnvelope)
		&& ExpectedEndEnvelope.Matches(EndEnvelope);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest::
Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest&
		Other) const
{
	return IsValid() && Other.IsValid()
		&& Projection.Matches(Other.Projection)
		&& CreateRequest.Matches(Other.CreateRequest)
		&& ProcessEnvelope.Matches(Other.ProcessEnvelope)
		&& EndEnvelope.Matches(Other.EndEnvelope);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
Capture(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
		Projection,
	const FCapture& Identity)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureResult
		Result;
	if (!Identity.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				IdentityInvalid;
		Result.Diagnostic = TEXT(
			"Product transaction requires explicit distinct identities and sequences 0, 1 and 2.");
		return Result;
	}
	if (!Projection.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				ProjectionInvalid;
		Result.Diagnostic = TEXT(
			"Product transaction requires one valid frozen projection.");
		return Result;
	}

	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection>
		Projections = {Projection};
	const auto Create =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				Identity.HostId,
				Identity.CreateSequence,
				Identity.CreateCommandId,
				Projections,
				Identity.VisualConsumerId,
				Identity.AudioConsumerId);
	if (!Create.IsCaptured())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				CreateRejected;
		Result.Diagnostic = Create.Diagnostic;
		return Result;
	}

	const auto& CreateCommand = Create.Request.GetEnvelope().GetCommand();
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand ProcessCommand;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureProcessNext(
				Identity.ProcessCommandId,
				CreateCommand.GetRunId(),
				CreateCommand.GetBatchId(),
				Identity.VisualAttemptId,
				Identity.AudioAttemptId,
				ProcessCommand)
		|| !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
			TryCapture(
				Identity.HostId,
				Identity.ProcessSequence,
				ProcessCommand,
				Result.Request.ProcessEnvelope))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				ProcessRejected;
		Result.Diagnostic = TEXT(
			"P12.18/P12.19 rejected the frozen ProcessNext operation.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand EndCommand;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::TryCaptureEnd(
			Identity.EndCommandId,
			CreateCommand.GetRunId(),
			CreateCommand.GetBatchId(),
			EndCommand)
		|| !Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
			TryCapture(
				Identity.HostId,
				Identity.EndSequence,
				EndCommand,
				Result.Request.EndEnvelope))
	{
		Result.Request = FRequest();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				EndRejected;
		Result.Diagnostic = TEXT(
			"P12.18/P12.19 rejected the frozen End operation.");
		return Result;
	}

	Result.Request.Projection = Projection;
	Result.Request.CreateRequest = Create.Request;
	if (!Result.Request.IsValid())
	{
		Result.Request = FRequest();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				RequestRejected;
		Result.Diagnostic = TEXT(
			"Captured Product transaction failed self-validation.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
			Captured;
	Result.Diagnostic = TEXT(
		"Captured one immutable single-projection Product transaction.");
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult::
IsCompleted() const
{
	const bool bStatusValid = Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Completed
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Resumed
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Replayed;
	return bStatusValid
		&& RunId.IsValid()
		&& ConfigId.IsValid()
		&& CuePolicyId.IsValid()
		&& EventId.IsValid()
		&& HostId.IsValid()
		&& BatchId.IsValid()
		&& Create.IsSuccess()
		&& Process.IsSuccess()
		&& End.IsSuccess()
		&& FinalNextSequence == 3
		&& FinalRecordCount == 3
		&& bFinalTerminal
		&& bAnyReplay == (Create.bReplay || Process.bReplay || End.bReplay)
		&& bAllReplay == (Create.bReplay && Process.bReplay && End.bReplay)
		&& (Status
			!= Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Replayed
			|| bAllReplay);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult::
HasDurableProgress() const
{
	return HostId.IsValid()
		&& RunId.IsValid()
		&& FinalRecordCount > 0
		&& FinalNextSequence == FinalRecordCount;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::TryExecute(
	const FRequest& Request,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FResult Result;
	if (!Request.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				RequestInvalid;
		Result.Diagnostic = TEXT(
			"Product transaction rejected an invalid frozen request.");
		CaptureFinalState(Host, Result);
		return Result;
	}

	const auto& Projection = Request.GetProjection();
	const auto& CreateEnvelope = Request.GetCreateRequest().GetEnvelope();
	Result.RunId = Projection.GetRunId();
	Result.ConfigId = Projection.GetConfigId();
	Result.CuePolicyId = Projection.GetCuePolicyId();
	Result.EventId = Projection.GetEvent().GetEventId();
	Result.HostId = CreateEnvelope.GetHostId();
	Result.BatchId = CreateEnvelope.GetCommand().GetBatchId();

	Result.Create =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Request.GetCreateRequest(), Host);
	if (!Result.Create.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				CreateRejected;
		Result.Diagnostic = Result.Create.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}

	Result.Process = Host.TryRouteProcessNext(
		Request.GetProcessEnvelope(), VisualExecutor, AudioExecutor);
	if (!Result.Process.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				ProcessRejected;
		Result.bAnyReplay = Result.Create.bReplay || Result.Process.bReplay;
		Result.Diagnostic = Result.Process.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}
	if (!Result.Process.Router.Session.IsBatchComplete())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				ProcessRetryPending;
		Result.bAnyReplay = Result.Create.bReplay || Result.Process.bReplay;
		Result.Diagnostic = Result.Process.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}

	Result.End = Host.TryRoute(Request.GetEndEnvelope());
	Result.bAnyReplay =
		Result.Create.bReplay || Result.Process.bReplay || Result.End.bReplay;
	Result.bAllReplay =
		Result.Create.bReplay && Result.Process.bReplay && Result.End.bReplay;
	if (!Result.End.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				EndRejected;
		Result.Diagnostic = Result.End.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}

	CaptureFinalState(Host, Result);
	if (!Host.IsValid()
		|| Host.GetHostId() != Result.HostId
		|| Host.GetRunId() != Result.RunId
		|| Host.GetBatchId() != Result.BatchId
		|| Result.FinalNextSequence != 3
		|| Result.FinalRecordCount != 3
		|| !Result.bFinalTerminal
		|| !RecordMatches(Host, 0, CreateEnvelope)
		|| !RecordMatches(Host, 1, Request.GetProcessEnvelope())
		|| !RecordMatches(Host, 2, Request.GetEndEnvelope()))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Product transaction completed without its exact three-record terminal proof.");
		return Result;
	}

	Result.Status = Result.bAllReplay
		? Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
			Replayed
		: Result.bAnyReplay
			? Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Resumed
			: Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
				Completed;
	Result.Diagnostic = Result.bAllReplay
		? TEXT("Replayed one completed Product transaction without executor re-entry.")
		: Result.bAnyReplay
			? TEXT("Resumed one Product transaction from durable Host evidence.")
			: TEXT("Completed one single-projection Product transaction.");
	return Result;
}
