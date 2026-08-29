#include "demo_mapShanmenFormationInfluenceProductRuntime.h"

namespace
{
	Fdemo_mapShanmenFormationInfluenceExecutionResult Reject(
		const Edemo_mapShanmenFormationInfluenceExecutionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceExecutionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceProductRuntime::IsValid() const
{
	if (!Executor.IsConsistent())
	{
		return false;
	}
	const bool bHasCorrelation = BoundCorrelation.IsValid();
	const bool bHasLedger = BoundLedgerId.IsValid();
	if (bHasCorrelation != bHasLedger)
	{
		return false;
	}
	return bHasLedger
		|| (Executor.GetActiveLeaseCount() == 0
			&& Executor.GetCompletedIntentCount() == 0
			&& Executor.GetAttemptCount() == 0);
}

Fdemo_mapShanmenFormationInfluenceExecutionResult
Fdemo_mapShanmenFormationInfluenceProductRuntime::TryExecuteOne(
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationInfluenceExecutionCommand& Command)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid,
			TEXT("Influence product runtime is internally inconsistent."));
	}
	if (!Host.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::HostInvalid,
			TEXT("Influence product runtime requires one valid ProductHost."));
	}
	if (RequestedCorrelation != Host.GetSession().GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::
				CorrelationMismatch,
			TEXT("Influence product runtime rejected a stale or foreign Run correlation."));
	}
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::CommandInvalid,
			TEXT("Influence product runtime requires one valid execution command."));
	}
	if (!Host.HasInfluenceAuthority())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::
				LedgerUnavailable,
			TEXT("Influence product runtime requires one Host-owned ledger."));
	}

	const FGuid CandidateLedgerId =
		Host.GetInfluenceLedger().GetLedgerId();
	if (IsBound()
		&& (BoundLedgerId != CandidateLedgerId
			|| BoundCorrelation != RequestedCorrelation))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid,
			TEXT("Influence product runtime is already bound to different Host evidence."));
	}

	const bool bWasUnbound = !IsBound();
	if (bWasUnbound)
	{
		const auto& Ledger = Host.GetInfluenceLedger();
		if (Ledger.GetPendingIntentCount() != Ledger.GetIntentCount())
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid,
				TEXT("An unbound influence product runtime cannot attach after successful Host history."));
		}
		BoundCorrelation = RequestedCorrelation;
		BoundLedgerId = CandidateLedgerId;
	}

	const auto Result =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Host, RequestedCorrelation, Command, Executor);
	if (bWasUnbound && !Result.bExecutorInvoked && !Result.IsSuccess())
	{
		BoundCorrelation = Fdemo_mapShanmenRunCorrelation();
		BoundLedgerId.Invalidate();
	}
	return Result;
}
