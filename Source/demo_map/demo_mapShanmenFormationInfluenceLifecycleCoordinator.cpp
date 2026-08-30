#include "demo_mapShanmenFormationInfluenceLifecycleCoordinator.h"

bool Fdemo_mapShanmenFormationInfluenceLifecycleResult::IsSuccess() const
{
	if (!bCoordinatorStateCommitted)
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepAccepted:
		return Step.IsSuccess();
	case Edemo_mapShanmenFormationInfluenceLifecycleStatus::TerminalPrepared:
	case Edemo_mapShanmenFormationInfluenceLifecycleStatus::TerminalReplayed:
		return TerminalPreparation.IsSuccess();
	case Edemo_mapShanmenFormationInfluenceLifecycleStatus::Completed:
	case Edemo_mapShanmenFormationInfluenceLifecycleStatus::
		CompletionReplayed:
		return Seal.IsSuccess() && End.IsSuccess();
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator::IsValid() const
{
	if (!ExecutionService.IsValid())
	{
		return false;
	}
	const bool bHasCorrelation = BoundCorrelation.IsValid();
	const bool bHasLedger = BoundLedgerId.IsValid();
	if (bHasCorrelation != bHasLedger)
	{
		return false;
	}
	if (!ExecutionService.IsBound())
	{
		return true;
	}
	return bHasLedger
		&& BoundLedgerId == ExecutionService.GetBoundLedgerId()
		&& BoundCorrelation == ExecutionService.GetBoundCorrelation();
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator::ValidateHost(
	const Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	Fdemo_mapShanmenFormationInfluenceLifecycleResult& OutResult) const
{
	if (!IsValid())
	{
		OutResult.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::StateInvalid;
		OutResult.Diagnostic =
			TEXT("Influence lifecycle Coordinator is internally inconsistent.");
		return false;
	}
	if (!Host.IsValid())
	{
		OutResult.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::HostInvalid;
		OutResult.Diagnostic =
			TEXT("Influence lifecycle coordination requires one valid ProductHost.");
		return false;
	}
	if (RequestedCorrelation != Host.GetSession().GetCorrelation())
	{
		OutResult.Status = Edemo_mapShanmenFormationInfluenceLifecycleStatus::
			CorrelationMismatch;
		OutResult.Diagnostic =
			TEXT("Influence lifecycle coordination rejected a stale or foreign Run correlation.");
		return false;
	}
	if (!Host.HasInfluenceAuthority())
	{
		OutResult.Status = Edemo_mapShanmenFormationInfluenceLifecycleStatus::
			LedgerUnavailable;
		OutResult.Diagnostic =
			TEXT("Influence lifecycle coordination requires one Host-owned ledger.");
		return false;
	}
	if (IsBound()
		&& (BoundLedgerId != Host.GetInfluenceLedger().GetLedgerId()
			|| BoundCorrelation != RequestedCorrelation))
	{
		OutResult.Status = Edemo_mapShanmenFormationInfluenceLifecycleStatus::
			BindingConflict;
		OutResult.Diagnostic =
			TEXT("Influence lifecycle Coordinator is already bound to different Host evidence.");
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator::BindTo(
	const Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	const FGuid LedgerId = Host.GetInfluenceLedger().GetLedgerId();
	if (!IsBound())
	{
		BoundCorrelation = RequestedCorrelation;
		BoundLedgerId = LedgerId;
	}
	return BoundLedgerId == LedgerId
		&& BoundCorrelation == RequestedCorrelation;
}

Fdemo_mapShanmenFormationInfluenceLifecycleResult
Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator::TryExecuteStep(
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request)
{
	Fdemo_mapShanmenFormationInfluenceLifecycleResult Result;
	if (!ValidateHost(Host, RequestedCorrelation, Result))
	{
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Candidate = *this;
	Result.Step = Candidate.ExecutionService.TryExecuteOne(
		Host, RequestedCorrelation, Request);
	if (!Result.Step.bServiceStateCommitted)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepRejected;
		Result.Diagnostic = Result.Step.Diagnostic;
		return Result;
	}
	if (!Candidate.BindTo(Host, RequestedCorrelation) || !Candidate.IsValid())
	{
		Result.Diagnostic =
			TEXT("Influence lifecycle step failed final Coordinator validation.");
		return Result;
	}

	*this = MoveTemp(Candidate);
	Result.Status = Result.Step.IsSuccess()
		? Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepAccepted
		: Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepRejected;
	Result.Diagnostic = Result.Step.Diagnostic;
	Result.bCoordinatorStateCommitted = true;
	return Result;
}

Fdemo_mapShanmenFormationInfluenceLifecycleResult
Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator::TryPrepareTerminal(
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	Fdemo_mapShanmenFormationInfluenceLifecycleResult Result;
	if (!ValidateHost(Host, RequestedCorrelation, Result))
	{
		return Result;
	}

	Result.TerminalPreparation =
		Host.TryPrepareTerminalInfluence(RequestedCorrelation);
	if (!Result.TerminalPreparation.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::TerminalRejected;
		Result.Diagnostic = Result.TerminalPreparation.Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Candidate = *this;
	if (!Candidate.BindTo(Host, RequestedCorrelation) || !Candidate.IsValid())
	{
		Result.Diagnostic =
			TEXT("Terminal preparation failed final Coordinator validation.");
		return Result;
	}
	*this = MoveTemp(Candidate);
	Result.Status = Result.TerminalPreparation.Status
		== Edemo_mapShanmenFormationHostInfluenceStatus::TerminalReplayed
		? Edemo_mapShanmenFormationInfluenceLifecycleStatus::TerminalReplayed
		: Edemo_mapShanmenFormationInfluenceLifecycleStatus::TerminalPrepared;
	Result.Diagnostic = Result.TerminalPreparation.Diagnostic;
	Result.bCoordinatorStateCommitted = true;
	return Result;
}

Fdemo_mapShanmenFormationInfluenceLifecycleResult
Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator::TrySealAndEnd(
	UWorld* World,
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	Fdemo_mapShanmenFormationInfluenceLifecycleResult Result;
	if (!ValidateHost(Host, RequestedCorrelation, Result))
	{
		return Result;
	}

	Result.Seal = Host.TrySealInfluence(RequestedCorrelation);
	if (!Result.Seal.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::SealRejected;
		Result.Diagnostic = Result.Seal.Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Candidate = *this;
	if (!Candidate.BindTo(Host, RequestedCorrelation) || !Candidate.IsValid())
	{
		Result.Diagnostic =
			TEXT("Influence seal failed final Coordinator validation.");
		return Result;
	}
	Result.End = Host.TryEndAndTeardown(World, RequestedCorrelation);
	*this = MoveTemp(Candidate);
	Result.bCoordinatorStateCommitted = true;
	if (!Result.End.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected;
		Result.Diagnostic = Result.End.Diagnostic;
		return Result;
	}
	Result.Status = Result.End.Status
		== Edemo_mapShanmenFormationHostStatus::TeardownReplayed
		? Edemo_mapShanmenFormationInfluenceLifecycleStatus::
			CompletionReplayed
		: Edemo_mapShanmenFormationInfluenceLifecycleStatus::Completed;
	Result.Diagnostic = Result.End.Diagnostic;
	return Result;
}
