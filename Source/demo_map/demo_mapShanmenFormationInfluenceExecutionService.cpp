#include "demo_mapShanmenFormationInfluenceExecutionService.h"

bool Fdemo_mapShanmenFormationInfluenceServiceResult::IsSuccess() const
{
	return Status
		== Edemo_mapShanmenFormationInfluenceServiceStatus::Accepted
		&& bServiceStateCommitted
		&& Route.IsSuccess()
		&& Execution.IsSuccess();
}

bool Fdemo_mapShanmenFormationInfluenceExecutionService::IsValid() const
{
	if (!Router.IsValid() || !Runtime.IsValid()
		|| Router.IsBound() != Runtime.IsBound())
	{
		return false;
	}
	return !Router.IsBound()
		|| (Router.GetBoundLedgerId() == Runtime.GetBoundLedgerId()
			&& Router.GetBoundCorrelation() == Runtime.GetBoundCorrelation());
}

Fdemo_mapShanmenFormationInfluenceServiceResult
Fdemo_mapShanmenFormationInfluenceExecutionService::TryExecuteOne(
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request)
{
	Fdemo_mapShanmenFormationInfluenceServiceResult Result;
	if (!IsValid())
	{
		Result.Diagnostic =
			TEXT("Influence execution Service is internally inconsistent.");
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionService Candidate = *this;
	Result.Route = Candidate.Router.TryRoute(
		Host, RequestedCorrelation, Request);
	if (!Result.Route.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceServiceStatus::RouteRejected;
		Result.Diagnostic = Result.Route.Diagnostic;
		return Result;
	}

	Result.Execution = Candidate.Runtime.TryExecuteOne(
		Host, RequestedCorrelation, Result.Route.Command);
	if (!Result.Execution.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceServiceStatus::ExecutionRejected;
		Result.Diagnostic = Result.Execution.Diagnostic;
		if (Candidate.IsValid())
		{
			*this = MoveTemp(Candidate);
			Result.bServiceStateCommitted = true;
		}
		return Result;
	}

	if (!Candidate.IsValid())
	{
		Result.Diagnostic =
			TEXT("Influence execution Service candidate failed validation.");
		return Result;
	}

	*this = MoveTemp(Candidate);
	Result.Status =
		Edemo_mapShanmenFormationInfluenceServiceStatus::Accepted;
	Result.Diagnostic = Result.Execution.Diagnostic;
	Result.bServiceStateCommitted = true;
	return Result;
}
