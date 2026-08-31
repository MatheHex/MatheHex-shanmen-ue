#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.h"

namespace
{
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchResult;
	using FIdentity =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;

	bool RequestMatchesIdentity(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest&
			Request,
		const FIdentity& Identity)
	{
		if (!Request.IsValid() || !Identity.IsValid())
		{
			return false;
		}
		const auto& CreateEnvelope = Request.GetCreateRequest().GetEnvelope();
		const auto& CreateCommand = CreateEnvelope.GetCommand();
		const auto& ProcessEnvelope = Request.GetProcessEnvelope();
		const auto& ProcessCommand = ProcessEnvelope.GetCommand();
		const auto& EndEnvelope = Request.GetEndEnvelope();
		const auto& EndCommand = EndEnvelope.GetCommand();
		return CreateEnvelope.GetHostId() == Identity.HostId
			&& ProcessEnvelope.GetHostId() == Identity.HostId
			&& EndEnvelope.GetHostId() == Identity.HostId
			&& CreateEnvelope.GetSequence() == Identity.CreateSequence
			&& ProcessEnvelope.GetSequence() == Identity.ProcessSequence
			&& EndEnvelope.GetSequence() == Identity.EndSequence
			&& CreateCommand.GetCommandId() == Identity.CreateCommandId
			&& ProcessCommand.GetCommandId() == Identity.ProcessCommandId
			&& EndCommand.GetCommandId() == Identity.EndCommandId
			&& CreateCommand.GetVisualConsumerId()
				== Identity.VisualConsumerId
			&& CreateCommand.GetAudioConsumerId()
				== Identity.AudioConsumerId
			&& ProcessCommand.GetVisualAttemptId()
				== Identity.VisualAttemptId
			&& ProcessCommand.GetAudioAttemptId()
				== Identity.AudioAttemptId;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchResult::
IsSuccess() const
{
	const bool bStatusMatches =
		(Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					Dispatched
			&& Dispatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Dispatched)
		|| (Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					Resumed
			&& Dispatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Resumed)
		|| (Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					Replayed
			&& Dispatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Replayed);
	if (!bStatusMatches
		|| !Projection.IsCaptured()
		|| !PlanCapture.IsCaptured()
		|| !Dispatch.IsSuccess()
		|| !PlanCapture.Plan.MatchesProjection(Projection.Projection)
		|| !Dispatch.Projection.Projection.Matches(Projection.Projection)
		|| !RequestMatchesIdentity(
			Dispatch.TransactionCapture.Request,
			PlanCapture.Plan.GetTransactionIdentity()))
	{
		return false;
	}
	return Dispatch.TransactionCapture.Request.GetProjection().Matches(
		Projection.Projection);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch::
TryDispatchCurrent(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
		Seed,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FResult Result;
	Result.Projection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
			CaptureCurrent(Session);
	if (!Result.Projection.IsCaptured())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				ProjectionRejected;
		Result.Diagnostic = Result.Projection.Diagnostic;
		return Result;
	}

	Result.PlanCapture =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Result.Projection.Projection, Seed);
	if (!Result.PlanCapture.IsCaptured())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				PlanRejected;
		Result.Diagnostic = Result.PlanCapture.Diagnostic;
		return Result;
	}

	Result.Dispatch =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchProjection(
				Result.Projection.Projection,
				Result.PlanCapture.Plan.GetTransactionIdentity(),
				Host,
				VisualExecutor,
				AudioExecutor);
	switch (Result.Dispatch.Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
		Dispatched:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				Dispatched;
		break;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
		Resumed:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				Resumed;
		break;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
		Replayed:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				Replayed;
		break;
	default:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				DispatchIncomplete;
		Result.Diagnostic = Result.Dispatch.Diagnostic;
		return Result;
	}

	if (!Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Planned Product dispatch completed without consistent projection, plan and transaction evidence.");
		return Result;
	}
	Result.Diagnostic = Result.Dispatch.Diagnostic;
	return Result;
}
