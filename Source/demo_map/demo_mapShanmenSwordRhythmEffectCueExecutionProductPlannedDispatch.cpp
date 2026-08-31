#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.h"

namespace
{
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchResult;
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
		|| !PlanCapture.Plan.MatchesRequest(
			Dispatch.TransactionCapture.Request))
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
	const auto Prepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Session, Seed);
	Result.Projection = Prepared.Projection;
	Result.PlanCapture = Prepared.PlanCapture;
	if (!Prepared.IsPrepared())
	{
		switch (Prepared.Status)
		{
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
			ProjectionRejected:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					ProjectionRejected;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
			PlanRejected:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					PlanRejected;
			break;
		default:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
					StateInvalid;
			break;
		}
		Result.Diagnostic = Prepared.Diagnostic;
		return Result;
	}

	const auto Dispatched =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			TryDispatchPrepared(
				Prepared.Prepared,
				Host,
				VisualExecutor,
				AudioExecutor);
	Result.Dispatch = Dispatched.Dispatch;
	switch (Dispatched.Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
		Dispatched:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				Dispatched;
		break;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
		Resumed:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				Resumed;
		break;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
		Replayed:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				Replayed;
		break;
	default:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				DispatchIncomplete;
		Result.Diagnostic = Dispatched.Diagnostic;
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
