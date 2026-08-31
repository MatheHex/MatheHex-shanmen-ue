#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.h"

namespace
{
	using FPrepareResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult;
	using FDispatchResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchResult;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch::
IsValid() const
{
	return Projection.IsValid()
		&& Plan.IsValid()
		&& Plan.MatchesProjection(Projection);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch::
Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
		Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& Projection.Matches(Other.Projection)
		&& Plan.Matches(Other.Plan);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch::
MatchesCurrentSession(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session) const
{
	return IsValid() && Plan.MatchesCurrentSession(Session);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult::
IsPrepared() const
{
	return Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
				Prepared
		&& Projection.IsCaptured()
		&& PlanCapture.IsCaptured()
		&& Prepared.IsValid()
		&& Prepared.GetProjection().Matches(Projection.Projection)
		&& Prepared.GetPlan().Matches(PlanCapture.Plan);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchResult::
IsSuccess() const
{
	const bool bStatusMatches =
		(Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					Dispatched
			&& Dispatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Dispatched)
		|| (Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					Resumed
			&& Dispatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Resumed)
		|| (Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
					Replayed
			&& Dispatch.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Replayed);
	return bStatusMatches
		&& Prepared.IsValid()
		&& Dispatch.IsSuccess()
		&& Dispatch.Projection.Projection.Matches(Prepared.GetProjection())
		&& Prepared.GetPlan().MatchesRequest(
			Dispatch.TransactionCapture.Request);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
PrepareCurrent(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
		Seed)
{
	FPrepareResult Result;
	Result.Projection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
			CaptureCurrent(Session);
	if (!Result.Projection.IsCaptured())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
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
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
				PlanRejected;
		Result.Diagnostic = Result.PlanCapture.Diagnostic;
		return Result;
	}

	Result.Prepared.Projection = Result.Projection.Projection;
	Result.Prepared.Plan = Result.PlanCapture.Plan;
	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
			Prepared;
	if (!Result.IsPrepared())
	{
		Result.Prepared =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Prepared Product dispatch failed projection-to-plan consistency validation.");
		return Result;
	}
	Result.Diagnostic = TEXT(
		"Prepared one caller-owned immutable Product dispatch value.");
	return Result;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
TryDispatchPrepared(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
		Prepared,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FDispatchResult Result;
	Result.Prepared = Prepared;
	if (!Result.Prepared.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				PreparedRejected;
		Result.Diagnostic = TEXT(
			"Prepared Product dispatch requires one valid frozen projection and matching plan.");
		return Result;
	}

	Result.Dispatch =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchProjection(
				Result.Prepared.GetProjection(),
				Result.Prepared.GetPlan().GetTransactionIdentity(),
				Host,
				VisualExecutor,
				AudioExecutor);
	switch (Result.Dispatch.Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
		Dispatched:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				Dispatched;
		break;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
		Resumed:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				Resumed;
		break;
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
		Replayed:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				Replayed;
		break;
	default:
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				DispatchIncomplete;
		Result.Diagnostic = Result.Dispatch.Diagnostic;
		return Result;
	}

	if (!Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Prepared Product dispatch completed without consistent frozen and transaction evidence.");
		return Result;
	}
	Result.Diagnostic = Result.Dispatch.Diagnostic;
	return Result;
}
