#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.h"

namespace
{
	using FProjectionResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult;
	using FIdentity =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult;

	FResult DispatchCapturedProjection(
		const FProjectionResult& Projection,
		const FIdentity& Identity,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
	{
		FResult Result;
		Result.Projection = Projection;
		if (!Result.Projection.IsCaptured())
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					ProjectionRejected;
			Result.Diagnostic = Result.Projection.Diagnostic;
			return Result;
		}

		Result.TransactionCapture =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
				Capture(Result.Projection.Projection, Identity);
		if (!Result.TransactionCapture.IsCaptured())
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					CaptureRejected;
			Result.Diagnostic = Result.TransactionCapture.Diagnostic;
			return Result;
		}

		Result.Transaction =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction::
				TryExecute(
					Result.TransactionCapture.Request,
					Host,
					VisualExecutor,
					AudioExecutor);
		switch (Result.Transaction.Status)
		{
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
			Completed:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Dispatched;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
			Resumed:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Resumed;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
			Replayed:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Replayed;
			break;
		default:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					TransactionIncomplete;
			Result.Diagnostic = Result.Transaction.Diagnostic;
			return Result;
		}

		if (!Result.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					StateInvalid;
			Result.Diagnostic = TEXT(
				"Product dispatch completed without consistent projection and transaction evidence.");
			return Result;
		}
		Result.Diagnostic = Result.Transaction.Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult::
IsSuccess() const
{
	const bool bStatusMatches =
		(Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Dispatched
			&& Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Completed)
		|| (Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Resumed
			&& Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Resumed)
		|| (Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Replayed
			&& Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					Replayed);
	if (!bStatusMatches
		|| !Projection.IsCaptured()
		|| !TransactionCapture.IsCaptured()
		|| !Transaction.IsCompleted()
		|| !TransactionCapture.Request.GetProjection().Matches(
			Projection.Projection))
	{
		return false;
	}

	const auto& CapturedProjection = Projection.Projection;
	return Transaction.RunId == CapturedProjection.GetRunId()
		&& Transaction.ConfigId == CapturedProjection.GetConfigId()
		&& Transaction.CuePolicyId == CapturedProjection.GetCuePolicyId()
		&& Transaction.EventId
			== CapturedProjection.GetEvent().GetEventId();
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
TryDispatchProjection(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
		Projection,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture&
		Identity,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FProjectionResult ProjectionResult;
	ProjectionResult.Projection = Projection;
	if (Projection.IsValid())
	{
		ProjectionResult.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				Captured;
		ProjectionResult.Diagnostic = TEXT(
			"Accepted one caller-frozen Product projection for dispatch.");
	}
	else
	{
		ProjectionResult.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				ProjectionRejected;
		ProjectionResult.Diagnostic = TEXT(
			"Product dispatch rejected an invalid caller-frozen projection.");
	}
	return DispatchCapturedProjection(
		ProjectionResult, Identity, Host, VisualExecutor, AudioExecutor);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
TryDispatchCurrent(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture&
		Identity,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	const FProjectionResult Projection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
			CaptureCurrent(Session);
	return DispatchCapturedProjection(
		Projection, Identity, Host, VisualExecutor, AudioExecutor);
}
