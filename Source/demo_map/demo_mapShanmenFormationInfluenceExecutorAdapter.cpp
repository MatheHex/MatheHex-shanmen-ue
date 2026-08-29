#include "demo_mapShanmenFormationInfluenceExecutorAdapter.h"

namespace
{
	bool IsKnownOutcome(
		const Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome)
	{
		return Outcome
			== Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure
			|| Outcome
				== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionResult Reject(
		const Edemo_mapShanmenFormationInfluenceExecutionStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceExecutionResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceExecutorReceipt FromLedgerReceipt(
		const Fdemo_mapShanmenFormationInfluenceAttemptReceipt& Receipt)
	{
		Fdemo_mapShanmenFormationInfluenceExecutorReceipt Result;
		Result.LedgerId = Receipt.LedgerId;
		Result.IntentId = Receipt.IntentId;
		Result.AttemptId = Receipt.AttemptId;
		Result.ExecutorReceiptId = Receipt.ExecutorReceiptId;
		Result.Outcome = Receipt.Outcome;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceAttemptCommand ToAcknowledgement(
		const Fdemo_mapShanmenFormationInfluenceExecutorReceipt& Receipt)
	{
		Fdemo_mapShanmenFormationInfluenceAttemptCommand Result;
		Result.IntentId = Receipt.IntentId;
		Result.AttemptId = Receipt.AttemptId;
		Result.ExecutorReceiptId = Receipt.ExecutorReceiptId;
		Result.Outcome = Receipt.Outcome;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceExecutionCommand::IsValid() const
{
	return IntentId.IsValid() && AttemptId.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceExecutorInvocation::IsValid() const
{
	return LedgerId.IsValid() && Intent.IsValid() && AttemptId.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceExecutorReceipt::IsValid() const
{
	return LedgerId.IsValid() && IntentId.IsValid() && AttemptId.IsValid()
		&& ExecutorReceiptId.IsValid() && IsKnownOutcome(Outcome);
}

bool Fdemo_mapShanmenFormationInfluenceExecutorReceipt::Matches(
	const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation) const
{
	return IsValid() && Invocation.IsValid()
		&& LedgerId == Invocation.LedgerId
		&& IntentId == Invocation.Intent.IntentId
		&& AttemptId == Invocation.AttemptId;
}

bool Fdemo_mapShanmenFormationInfluenceExecutorResult::IsSuccess() const
{
	return Status
		== Edemo_mapShanmenFormationInfluenceExecutorStatus::Completed
		&& Receipt.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceExecutionResult::IsSuccess() const
{
	if (!Invocation.IsValid() || !Executor.IsSuccess()
		|| !Executor.Receipt.Matches(Invocation)
		|| !HostAcknowledgement.IsSuccess())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceExecutionStatus::Succeeded:
		return bExecutorInvoked
			&& Executor.Receipt.Outcome
				== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded
			&& HostAcknowledgement.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::Acknowledged;
	case Edemo_mapShanmenFormationInfluenceExecutionStatus::RetryRecorded:
		return bExecutorInvoked
			&& Executor.Receipt.Outcome
				== Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure
			&& HostAcknowledgement.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::RetryRecorded;
	case Edemo_mapShanmenFormationInfluenceExecutionStatus::AttemptReplayed:
		return !bExecutorInvoked
			&& (HostAcknowledgement.Status
					== Edemo_mapShanmenFormationHostInfluenceStatus::RetryReplayed
				|| HostAcknowledgement.Status
					== Edemo_mapShanmenFormationHostInfluenceStatus::
						AcknowledgementReplayed);
	default:
		return false;
	}
}

Fdemo_mapShanmenFormationInfluenceExecutionResult
Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationInfluenceExecutionCommand& Command,
	Idemo_mapShanmenFormationInfluenceExecutor& Executor)
{
	if (!Host.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::HostInvalid,
			TEXT("Influence execution requires one valid product host."));
	}
	if (RequestedCorrelation != Host.GetSession().GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::
				CorrelationMismatch,
			TEXT("Influence execution rejected a stale or foreign Run correlation."));
	}
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::CommandInvalid,
			TEXT("Influence execution requires valid intent and attempt identities."));
	}
	if (!Host.HasInfluenceAuthority())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::LedgerUnavailable,
			TEXT("No Host-owned influence ledger is available."));
	}

	const auto& Ledger = Host.GetInfluenceLedger();
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt ExistingAttempt;
	if (Ledger.TryGetAttemptReceipt(
			Command.IntentId, Command.AttemptId, ExistingAttempt))
	{
		Fdemo_mapShanmenFormationInfluenceIntent ExistingIntent;
		if (!Ledger.TryGetIntent(Command.IntentId, ExistingIntent))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid,
				TEXT("Ledger attempt evidence lost its owned intent."));
		}
		Fdemo_mapShanmenFormationInfluenceExecutionResult Result;
		Result.Invocation.LedgerId = Ledger.GetLedgerId();
		Result.Invocation.Intent = MoveTemp(ExistingIntent);
		Result.Invocation.AttemptId = Command.AttemptId;
		Result.Executor.Status =
			Edemo_mapShanmenFormationInfluenceExecutorStatus::Completed;
		Result.Executor.Diagnostic =
			TEXT("The exact executor evidence replayed from the Host ledger.");
		Result.Executor.Receipt = FromLedgerReceipt(ExistingAttempt);
		Result.HostAcknowledgement = Host.TryAcknowledgeInfluence(
			RequestedCorrelation,
			ToAcknowledgement(Result.Executor.Receipt));
		if (!Result.HostAcknowledgement.IsSuccess())
		{
			Result.Status = Edemo_mapShanmenFormationInfluenceExecutionStatus::
				AcknowledgementRejected;
			Result.Diagnostic = Result.HostAcknowledgement.Diagnostic;
			return Result;
		}
		Result.Status =
			Edemo_mapShanmenFormationInfluenceExecutionStatus::AttemptReplayed;
		Result.Diagnostic = Result.Executor.Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceIntent Pending;
	if (!Host.TryPeekNextInfluenceIntent(Pending))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::NoPendingIntent,
			TEXT("The Host ledger has no pending influence intent."));
	}
	if (Pending.IntentId != Command.IntentId)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::IntentOutOfOrder,
			TEXT("Only the canonical first pending intent may execute."));
	}

	Fdemo_mapShanmenFormationInfluenceExecutionResult Result;
	Result.Invocation.LedgerId = Ledger.GetLedgerId();
	Result.Invocation.Intent = Pending;
	Result.Invocation.AttemptId = Command.AttemptId;
	if (!Result.Invocation.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid;
		Result.Diagnostic = TEXT("Host pending evidence formed an invalid invocation.");
		return Result;
	}

	Result.bExecutorInvoked = true;
	Result.Executor = Executor.Execute(Result.Invocation);
	if (Result.Executor.Status
		== Edemo_mapShanmenFormationInfluenceExecutorStatus::Rejected)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceExecutionStatus::ExecutorRejected;
		Result.Diagnostic = Result.Executor.Diagnostic;
		return Result;
	}
	if (!Result.Executor.IsSuccess()
		|| !Result.Executor.Receipt.Matches(Result.Invocation))
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceExecutionStatus::
			ExecutorEvidenceMismatch;
		Result.Diagnostic =
			TEXT("Executor evidence did not match the immutable invocation.");
		return Result;
	}

	Result.HostAcknowledgement = Host.TryAcknowledgeInfluence(
		RequestedCorrelation, ToAcknowledgement(Result.Executor.Receipt));
	if (!Result.HostAcknowledgement.IsSuccess())
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceExecutionStatus::
			AcknowledgementRejected;
		Result.Diagnostic = Result.HostAcknowledgement.Diagnostic;
		return Result;
	}
	Result.Status = Result.Executor.Receipt.Outcome
		== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded
		? Edemo_mapShanmenFormationInfluenceExecutionStatus::Succeeded
		: Edemo_mapShanmenFormationInfluenceExecutionStatus::RetryRecorded;
	Result.Diagnostic = Result.HostAcknowledgement.Diagnostic;
	return Result;
}
