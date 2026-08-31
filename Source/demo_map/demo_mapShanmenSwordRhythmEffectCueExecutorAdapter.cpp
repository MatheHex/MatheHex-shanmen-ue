#include "demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsExecutorOutcome(
		const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome)
	{
		return Outcome
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure
			|| Outcome
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded;
	}

	FGuid MakeInvocationId(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const FGuid& AttemptId)
	{
		if (!Route.IsValid() || Route.NumCommands() <= 0
			|| !AttemptId.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Route.GetRouteId()),
			GuidDigits(Route.GetDelivery().GetScope().GetScopeId()),
			GuidDigits(AttemptId),
			FString::FromInt(Route.NumCommands())
		};
		for (const auto& Command : Route.GetCommands())
		{
			if (!Command.IsValid()
				|| Command.GetChannel() != Route.GetChannel())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Command.GetCommandId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutorInvocation.r1"),
			Parts);
	}

	FGuid MakeExecutorReceiptId(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation,
		const FGuid& ExecutorReceiptId,
		const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome)
	{
		if (!Invocation.IsValid() || !ExecutorReceiptId.IsValid()
			|| !IsExecutorOutcome(Outcome))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutorReceipt.r1"),
			{
				GuidDigits(Invocation.GetInvocationId()),
				GuidDigits(Invocation.GetRoute().GetRouteId()),
				GuidDigits(Invocation.GetAttemptId()),
				GuidDigits(ExecutorReceiptId),
				FString::FromInt(static_cast<uint8>(Outcome))
			});
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult Reject(
		const Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	bool IsReplaySubmission(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult& Submission)
	{
		return Submission.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					RetryReplayed
			|| Submission.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					AcknowledgementReplayed;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& InRoute,
	const FGuid& InAttemptId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& OutInvocation)
{
	OutInvocation = Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation();
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Candidate;
	Candidate.Route = InRoute;
	Candidate.AttemptId = InAttemptId;
	Candidate.InvocationId = MakeInvocationId(
		Candidate.Route, Candidate.AttemptId);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutInvocation = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::IsValid() const
{
	return InvocationId.IsValid()
		&& InvocationId == MakeInvocationId(Route, AttemptId);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Other) const
{
	return IsValid() && Other.IsValid()
		&& InvocationId == Other.InvocationId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& InInvocation,
	const FGuid& InExecutorReceiptId,
	const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome InOutcome,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt();
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt Candidate;
	Candidate.Invocation = InInvocation;
	Candidate.ExecutorReceiptId = InExecutorReceiptId;
	Candidate.Outcome = InOutcome;
	Candidate.ReceiptId = MakeExecutorReceiptId(
		Candidate.Invocation,
		Candidate.ExecutorReceiptId,
		Candidate.Outcome);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::IsValid() const
{
	return ReceiptId.IsValid()
		&& ReceiptId == MakeExecutorReceiptId(
			Invocation, ExecutorReceiptId, Outcome);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
		ExpectedInvocation) const
{
	return IsValid() && ExpectedInvocation.IsValid()
		&& Invocation.Matches(ExpectedInvocation);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed
		&& Receipt.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult::IsSuccess() const
{
	switch (Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::Succeeded:
		return bExecutorInvoked && !bNoOp && Invocation.IsValid()
			&& Executor.IsSuccess() && Executor.Receipt.Matches(Invocation)
			&& Executor.Receipt.GetOutcome()
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded
			&& Submission.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					Acknowledged
			&& Submission.IsAcknowledged();
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RetryRecorded:
		return bExecutorInvoked && !bNoOp && Invocation.IsValid()
			&& Executor.IsSuccess() && Executor.Receipt.Matches(Invocation)
			&& Executor.Receipt.GetOutcome()
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure
			&& Submission.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					RetryRecorded
			&& Submission.IsSuccess();
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptReplayed:
		return !bExecutorInvoked && !bNoOp && Invocation.IsValid()
			&& Executor.IsSuccess() && Executor.Receipt.Matches(Invocation)
			&& IsReplaySubmission(Submission);
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpSucceeded:
		return !bExecutorInvoked && bNoOp && !Invocation.IsValid()
			&& !Executor.IsSuccess()
			&& Submission.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpAcknowledged
			&& Submission.IsAcknowledged();
	case Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpReplayed:
		return !bExecutorInvoked && bNoOp && !Invocation.IsValid()
			&& !Executor.IsSuccess()
			&& Submission.Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpReplayed
			&& Submission.IsAcknowledged();
	default:
		return false;
	}
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutorAdapter::TryExecute(
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& Coordinator,
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
	const FGuid& AttemptId,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& Executor)
{
	if (!Coordinator.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
				CoordinatorInvalid,
			TEXT("Cue execution requires one valid consumer coordinator."));
	}
	if (!Route.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RouteInvalid,
			TEXT("Cue execution requires one valid prepared consumer route."));
	}
	if (!Route.GetDelivery().GetScope().Matches(Coordinator.GetScope()))
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::ScopeMismatch,
			TEXT("Cue route belongs to a different Run or consumer scope."));
	}
	if (!AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptInvalid,
			TEXT("Cue execution requires one caller-owned attempt identity."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt Existing;
	if (Coordinator.TryGetAttemptReceipt(Route, AttemptId, Existing))
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult Result;
		Result.Submission = Coordinator.Submit(Route, Existing.GetCommand());
		if (Existing.GetCommand().GetOutcome()
			== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::NoOpSucceeded)
		{
			Result.bNoOp = true;
			if (Result.Submission.Status
				!= Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpReplayed)
			{
				return Reject(
					Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid,
					TEXT("Stored no-op attempt did not replay from coordinator evidence."));
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpReplayed;
			Result.Diagnostic =
				TEXT("Stored zero-command attempt replayed without an executor.");
			return Result;
		}

		if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::TryCreate(
				Route, AttemptId, Result.Invocation)
			|| !Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
				Result.Invocation,
				Existing.GetCommand().GetExecutorReceiptId(),
				Existing.GetCommand().GetOutcome(),
				Result.Executor.Receipt))
		{
			return Reject(
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid,
				TEXT("Stored attempt could not rebuild its executor evidence."));
		}
		Result.Executor.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
		Result.Executor.Diagnostic =
			TEXT("Executor evidence replayed from the consumer coordinator.");
		if (!IsReplaySubmission(Result.Submission))
		{
			return Reject(
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid,
				TEXT("Stored attempt did not replay from coordinator evidence."));
		}
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptReplayed;
		Result.Diagnostic = Result.Executor.Diagnostic;
		return Result;
	}

	if (Coordinator.IsPending())
	{
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt Latest;
		if (!Coordinator.TryGetLatestReceipt(Latest))
		{
			return Reject(
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid,
				TEXT("Pending coordinator lost its current attempt evidence."));
		}
		if (!Latest.GetRoute().Matches(Route))
		{
			return Reject(
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RoutePending,
				TEXT("A retryable earlier route must complete before execution advances."));
		}
	}

	const auto Prepared = Coordinator.Prepare(
		Route.GetDelivery().GetEvent());
	if (!Prepared.IsRouted() || !Prepared.Route.Matches(Route))
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RouteUnavailable,
			Prepared.Diagnostic.IsEmpty()
				? TEXT("Cue route is stale, acknowledged, or not current.")
				: Prepared.Diagnostic);
	}

	if (Route.NumCommands() == 0)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand NoOp;
		if (!Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand::TryCreate(
				Route,
				AttemptId,
				FGuid(),
				Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::NoOpSucceeded,
				NoOp))
		{
			return Reject(
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid,
				TEXT("Zero-command route could not form explicit no-op evidence."));
		}
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult Result;
		Result.bNoOp = true;
		Result.Submission = Coordinator.Submit(Route, NoOp);
		if (Result.Submission.Status
			!= Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				NoOpAcknowledged)
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptRejected;
			Result.Diagnostic = Result.Submission.Diagnostic;
			return Result;
		}
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::NoOpSucceeded;
		Result.Diagnostic =
			TEXT("Zero-command route acknowledged without invoking an executor.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionResult Result;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation::TryCreate(
			Route, AttemptId, Result.Invocation))
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
				InvocationRejected,
			TEXT("Consumer route could not form an immutable batch invocation."));
	}

	Result.bExecutorInvoked = true;
	Result.Executor = Executor.Execute(Result.Invocation);
	if (Result.Executor.Status
		== Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::ExecutorRejected;
		Result.Diagnostic = Result.Executor.Diagnostic;
		return Result;
	}
	if (!Result.Executor.IsSuccess()
		|| !Result.Executor.Receipt.Matches(Result.Invocation))
	{
		Result.Status = Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::
			ExecutorEvidenceMismatch;
		Result.Diagnostic =
			TEXT("Executor receipt did not match the immutable batch invocation.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Attempt;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand::TryCreate(
			Route,
			AttemptId,
			Result.Executor.Receipt.GetExecutorReceiptId(),
			Result.Executor.Receipt.GetOutcome(),
			Attempt))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Executor receipt could not form coordinator attempt evidence.");
		return Result;
	}
	Result.Submission = Coordinator.Submit(Route, Attempt);
	if (Result.Submission.Status
		== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
			Acknowledged)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::Succeeded;
	}
	else if (Result.Submission.Status
		== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
			RetryRecorded)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::RetryRecorded;
	}
	else
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionStatus::AttemptRejected;
	}
	Result.Diagnostic = Result.Submission.Diagnostic;
	return Result;
}
