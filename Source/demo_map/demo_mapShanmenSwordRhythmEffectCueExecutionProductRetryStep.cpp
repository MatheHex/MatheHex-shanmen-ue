#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.h"

namespace
{
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;
	using EDecisionOutcome =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome;
	using ERetryStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryStatus;

	void CaptureFinalState(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		FResult& Result)
	{
		Result.FinalNextSequence = Host.GetNextSequence();
		Result.FinalRecordCount = Host.GetRecordCount();
		Result.bFinalTerminal = Host.IsTerminal();
	}

	EStepStatus MapStopOutcome(const EDecisionOutcome Outcome)
	{
		switch (Outcome)
		{
		case EDecisionOutcome::StopBudgetExhausted:
			return EStepStatus::StoppedBudgetExhausted;
		case EDecisionOutcome::StopCompleted:
			return EStepStatus::StoppedCompleted;
		case EDecisionOutcome::StopNotRetryable:
			return EStepStatus::StoppedNotRetryable;
		default:
			return EStepStatus::StateInvalid;
		}
	}

	EStepStatus MapExecutionStatus(const ERetryStatus Status)
	{
		switch (Status)
		{
		case ERetryStatus::Completed:
		case ERetryStatus::Resumed:
		case ERetryStatus::Replayed:
			return EStepStatus::Completed;
		case ERetryStatus::RetryPending:
			return EStepStatus::RetryPending;
		case ERetryStatus::StateInvalid:
			return EStepStatus::StateInvalid;
		case ERetryStatus::PreparedRejected:
		case ERetryStatus::HostStateMismatch:
		case ERetryStatus::ProcessRejected:
		case ERetryStatus::EndRejected:
		default:
			return EStepStatus::ExecutionRejected;
		}
	}

	bool IsStopStatus(const EStepStatus Status)
	{
		return Status == EStepStatus::StoppedBudgetExhausted
			|| Status == EStepStatus::StoppedCompleted
			|| Status == EStepStatus::StoppedNotRetryable;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult::
DidExecuteRetry() const
{
	return Decision.IsDecided()
		&& Decision.Decision.ShouldRetry()
		&& Execution.Prepared.IsValid()
		&& Execution.Prepared.Matches(
			Decision.Decision.GetPreparedRetry());
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult::
IsHandled() const
{
	if (!Decision.IsDecided()
		|| FinalNextSequence < 0
		|| FinalRecordCount < 0
		|| FinalNextSequence != FinalRecordCount
		|| NextRenewalsUsed
			!= Decision.Decision.GetNextRenewalsUsed())
	{
		return false;
	}

	if (IsStopStatus(Status))
	{
		return !Decision.Decision.ShouldRetry()
			&& !Execution.Prepared.IsValid()
			&& FinalNextSequence
				== Decision.Decision.GetObservedNextSequence()
			&& FinalRecordCount
				== Decision.Decision.GetObservedRecordCount()
			&& bFinalTerminal
				== Decision.Decision.WasHostTerminal()
			&& ((Status == EStepStatus::StoppedBudgetExhausted
					&& Decision.Decision.GetOutcome()
						== EDecisionOutcome::StopBudgetExhausted)
				|| (Status == EStepStatus::StoppedCompleted
					&& Decision.Decision.GetOutcome()
						== EDecisionOutcome::StopCompleted)
				|| (Status == EStepStatus::StoppedNotRetryable
					&& Decision.Decision.GetOutcome()
						== EDecisionOutcome::StopNotRetryable));
	}

	if (!DidExecuteRetry()
		|| FinalNextSequence != Execution.FinalNextSequence
		|| FinalRecordCount != Execution.FinalRecordCount
		|| bFinalTerminal != Execution.bFinalTerminal)
	{
		return false;
	}
	return (Status == EStepStatus::Completed && Execution.IsCompleted())
		|| (Status == EStepStatus::RetryPending
			&& Execution.Status == ERetryStatus::RetryPending
			&& Execution.HasDurableProgress()
			&& !bFinalTerminal);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult::
CanRequestAnotherStep() const
{
	return Status == EStepStatus::RetryPending
		&& IsHandled()
		&& NextRenewalsUsed
			< Decision.Decision.GetRequest().Policy.MaxRenewals;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
TryRunStep(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
		PreparedDispatch,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
		Request)
{
	FResult Result;
	Result.NextRenewalsUsed = Request.IsValid() ? Request.RenewalsUsed : 0;
	Result.Decision =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
			Decide(PreparedDispatch, Host, Request);
	if (!Result.Decision.IsDecided())
	{
		Result.Status = EStepStatus::DecisionRejected;
		Result.Diagnostic = Result.Decision.Diagnostic;
		CaptureFinalState(Host, Result);
		return Result;
	}

	Result.NextRenewalsUsed =
		Result.Decision.Decision.GetNextRenewalsUsed();
	if (!Result.Decision.Decision.ShouldRetry())
	{
		Result.Status = MapStopOutcome(
			Result.Decision.Decision.GetOutcome());
		Result.Diagnostic = Result.Decision.Diagnostic;
		CaptureFinalState(Host, Result);
		if (!Result.IsHandled())
		{
			Result.Status = EStepStatus::StateInvalid;
			Result.Diagnostic = TEXT(
				"Retry step stop outcome failed durable-evidence validation.");
		}
		return Result;
	}

	Result.Execution =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			TryExecutePreparedRetry(
				Result.Decision.Decision.GetPreparedRetry(),
				Host,
				VisualExecutor,
				AudioExecutor);
	Result.Status = MapExecutionStatus(Result.Execution.Status);
	Result.Diagnostic = Result.Execution.Diagnostic;
	CaptureFinalState(Host, Result);
	if ((Result.Status == EStepStatus::Completed
			|| Result.Status == EStepStatus::RetryPending)
		&& !Result.IsHandled())
	{
		Result.Status = EStepStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Retry step execution outcome failed durable-evidence validation.");
	}
	return Result;
}
