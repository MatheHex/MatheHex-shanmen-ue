#include "demo_mapShanmenSwordRhythmEffectCueExecutionDriver.h"

namespace
{
	Fdemo_mapShanmenSwordRhythmEffectCueDriverResult Reject(
		const Edemo_mapShanmenSwordRhythmEffectCueDriverStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueDriverResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	void MapExecution(
		Fdemo_mapShanmenSwordRhythmEffectCueDriverResult& Result)
	{
		if (!Result.Execution.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::ExecutionRejected;
			Result.Diagnostic = Result.Execution.Diagnostic;
			return;
		}
		switch (Result.Execution.Status)
		{
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::Succeeded:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RetryRecorded:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::RetryRecorded;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptReplayed:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpSucceeded:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpExecuted;
			break;
		case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpReplayed:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpReplayed;
			break;
		default:
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::ExecutionRejected;
			break;
		}
		Result.Diagnostic = Result.Execution.Diagnostic;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDriverResult::IsSuccess() const
{
	switch (Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::Executed:
		return Preparation.IsRouted()
			&& Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::Succeeded
			&& Execution.IsSuccess();
	case Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::RetryRecorded:
		return Preparation.IsRouted()
			&& Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					RetryRecorded
			&& Execution.IsSuccess();
	case Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptReplayed:
		return (Preparation.IsRouted()
				|| Preparation.Status
					== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
						AlreadyAcknowledged)
			&& Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					AttemptReplayed
			&& Execution.IsSuccess();
	case Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpExecuted:
		return Preparation.IsRouted()
			&& Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpSucceeded
			&& Execution.IsSuccess();
	case Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::NoOpReplayed:
		return (Preparation.IsRouted()
				|| Preparation.Status
					== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
						AlreadyAcknowledged)
			&& Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpReplayed
			&& Execution.IsSuccess();
	case Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AlreadyAcknowledged:
		return Preparation.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
					AlreadyAcknowledged
			&& ExistingAcknowledgement.IsValid()
			&& Execution.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
					RouteUnavailable
			&& !Execution.bExecutorInvoked;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDriverResult::IsAcknowledged() const
{
	return IsSuccess()
		&& (ExistingAcknowledgement.IsValid()
			|| Execution.Submission.IsAcknowledged());
}

Fdemo_mapShanmenSwordRhythmEffectCueDriverResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& Coordinator,
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event,
	const FGuid& AttemptId,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& Executor)
{
	if (!Coordinator.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::CoordinatorInvalid,
			TEXT("Cue execution driver requires one valid consumer coordinator."));
	}
	if (!Event.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::EventInvalid,
			TEXT("Cue execution driver requires one immutable valid event."));
	}
	if (!AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::AttemptInvalid,
			TEXT("Cue execution driver requires one caller-owned attempt identity."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueDriverResult Result;
	Result.Preparation = Coordinator.Prepare(Event);
	if (Result.Preparation.IsRouted())
	{
		Result.Execution =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
				Coordinator,
				Result.Preparation.Route,
				AttemptId,
				Executor);
		MapExecution(Result);
		return Result;
	}
	if (Result.Preparation.Status
		!= Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
			AlreadyAcknowledged)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::PreparationRejected;
		Result.Diagnostic = Result.Preparation.Diagnostic;
		return Result;
	}

	if (!Coordinator.TryGetLastAcknowledgement(
			Result.ExistingAcknowledgement)
		|| !Result.ExistingAcknowledgement.GetDelivery().GetEvent().Matches(Event))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Acknowledged event lost its matching consumer evidence.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute ExistingRoute;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::TryCreate(
			Result.ExistingAcknowledgement.GetDelivery(), ExistingRoute))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Acknowledged delivery could not rebuild its canonical route.");
		return Result;
	}

	Result.Execution =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
			Coordinator, ExistingRoute, AttemptId, Executor);
	if (Result.Execution.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
				AttemptReplayed
		|| Result.Execution.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpReplayed)
	{
		MapExecution(Result);
		return Result;
	}
	if (Result.Execution.Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
				RouteUnavailable
		&& !Result.Execution.bExecutorInvoked)
	{
		Result.Status = Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::
			AlreadyAcknowledged;
		Result.Diagnostic = Result.Preparation.Diagnostic;
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueDriverStatus::ExecutionRejected;
	Result.Diagnostic = Result.Execution.Diagnostic;
	return Result;
}
