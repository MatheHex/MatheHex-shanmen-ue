#include "demo_mapShanmenFormationInfluenceExecutionRouter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	const FName AttemptNamespace(
		TEXT("Shanmen.Formation.InfluenceExecutionAttempt.r1"));

	FGuid MakeAttemptId(
		const FGuid& LedgerId,
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request)
	{
		if (!LedgerId.IsValid() || !Request.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			AttemptNamespace,
			{
				LedgerId.ToString(EGuidFormats::Digits),
				Request.RequestId.ToString(EGuidFormats::Digits),
				Request.ExpectedIntentId.ToString(EGuidFormats::Digits)
			});
	}

	Fdemo_mapShanmenFormationInfluenceRouteResult Reject(
		const Edemo_mapShanmenFormationInfluenceRouteStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceRouteResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

}

bool Fdemo_mapShanmenFormationInfluenceExecutionRequest::IsValid() const
{
	return RequestId.IsValid() && ExpectedIntentId.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceExecutionRequest::Matches(
	const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Other) const
{
	return IsValid() && Other.IsValid()
		&& RequestId == Other.RequestId
		&& ExpectedIntentId == Other.ExpectedIntentId;
}

bool Fdemo_mapShanmenFormationInfluenceRouteResult::IsSuccess() const
{
	if (!Command.IsValid())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceRouteStatus::Routed:
	case Edemo_mapShanmenFormationInfluenceRouteStatus::RequestReplayed:
	case Edemo_mapShanmenFormationInfluenceRouteStatus::HostEvidenceRecovered:
		return true;
	default:
		return false;
	}
}

Fdemo_mapShanmenFormationInfluenceRouteResult
Fdemo_mapShanmenFormationInfluenceExecutionRouter::TryRoute(
	const Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::RouterInvalid,
			TEXT("Influence execution Router is internally inconsistent."));
	}
	if (!Host.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::HostInvalid,
			TEXT("Influence execution routing requires one valid ProductHost."));
	}
	if (RequestedCorrelation != Host.GetSession().GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::CorrelationMismatch,
			TEXT("Influence execution routing rejected stale Run correlation."));
	}
	if (!Request.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::RequestInvalid,
			TEXT("Influence execution routing requires valid request and intent identities."));
	}
	if (!Host.HasInfluenceAuthority())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::LedgerUnavailable,
			TEXT("Influence execution routing requires one Host-owned ledger."));
	}

	const auto& Ledger = Host.GetInfluenceLedger();
	const FGuid LedgerId = Ledger.GetLedgerId();
	if (IsBound()
		&& (BoundLedgerId != LedgerId
			|| BoundCorrelation != RequestedCorrelation))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::BindingConflict,
			TEXT("Influence execution Router is bound to different Host evidence."));
	}

	for (const FRecord& Existing : Records)
	{
		if (Existing.Request.RequestId != Request.RequestId)
		{
			continue;
		}
		if (!Existing.Request.Matches(Request))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceRouteStatus::RequestConflict,
				TEXT("Influence execution RequestId was reused with another intent."));
		}
		Fdemo_mapShanmenFormationInfluenceRouteResult Result;
		Result.Status =
			Edemo_mapShanmenFormationInfluenceRouteStatus::RequestReplayed;
		Result.Diagnostic =
			TEXT("Influence execution request replayed its exact command.");
		Result.Command = Existing.Command;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionCommand Command;
	Command.IntentId = Request.ExpectedIntentId;
	Command.AttemptId = MakeAttemptId(LedgerId, Request);
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::StateInvalid,
			TEXT("Influence execution request formed an invalid command."));
	}
	for (const FRecord& Existing : Records)
	{
		if (Existing.Command.AttemptId == Command.AttemptId)
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceRouteStatus::AttemptCollision,
				TEXT("Deterministic influence attempt identity collided with another request."));
		}
	}

	Edemo_mapShanmenFormationInfluenceRouteStatus AcceptedStatus =
		Edemo_mapShanmenFormationInfluenceRouteStatus::Routed;
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt ExistingAttempt;
	if (Ledger.TryGetAttemptReceipt(
		Request.ExpectedIntentId, Command.AttemptId, ExistingAttempt))
	{
		AcceptedStatus = Edemo_mapShanmenFormationInfluenceRouteStatus::
			HostEvidenceRecovered;
	}
	else
	{
		Fdemo_mapShanmenFormationInfluenceIntent Pending;
		if (!Host.TryPeekNextInfluenceIntent(Pending))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceRouteStatus::IntentUnavailable,
				TEXT("Host ledger has no matching pending or historical attempt."));
		}
		if (Pending.IntentId != Request.ExpectedIntentId)
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceRouteStatus::IntentOutOfOrder,
				TEXT("Only the canonical first pending influence intent may route."));
		}
	}

	Fdemo_mapShanmenFormationInfluenceExecutionRouter Candidate = *this;
	if (!Candidate.IsBound())
	{
		Candidate.BoundCorrelation = RequestedCorrelation;
		Candidate.BoundLedgerId = LedgerId;
	}
	FRecord Record;
	Record.Request = Request;
	Record.Command = Command;
	Candidate.Records.Add(MoveTemp(Record));
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceRouteStatus::StateInvalid,
			TEXT("Influence execution Router candidate failed validation."));
	}
	*this = MoveTemp(Candidate);

	Fdemo_mapShanmenFormationInfluenceRouteResult Result;
	Result.Status = AcceptedStatus;
	Result.Diagnostic = AcceptedStatus
		== Edemo_mapShanmenFormationInfluenceRouteStatus::Routed
		? TEXT("Influence execution request routed to the first pending intent.")
		: TEXT("Influence execution command recovered from Host attempt evidence.");
	Result.Command = Command;
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceExecutionRouter::IsValid() const
{
	const bool bHasCorrelation = BoundCorrelation.IsValid();
	const bool bHasLedger = BoundLedgerId.IsValid();
	if (bHasCorrelation != bHasLedger)
	{
		return false;
	}
	if (!bHasLedger)
	{
		return Records.IsEmpty();
	}

	TSet<FGuid> RequestIds;
	TSet<FGuid> AttemptIds;
	for (const FRecord& Record : Records)
	{
		if (!Record.Request.IsValid()
			|| !Record.Command.IsValid()
			|| Record.Command.IntentId != Record.Request.ExpectedIntentId
			|| Record.Command.AttemptId
				!= MakeAttemptId(BoundLedgerId, Record.Request)
			|| RequestIds.Contains(Record.Request.RequestId)
			|| AttemptIds.Contains(Record.Command.AttemptId))
		{
			return false;
		}
		RequestIds.Add(Record.Request.RequestId);
		AttemptIds.Add(Record.Command.AttemptId);
	}
	return !Records.IsEmpty();
}

bool Fdemo_mapShanmenFormationInfluenceExecutionRouter::TryGetCommand(
	const FGuid& RequestId,
	Fdemo_mapShanmenFormationInfluenceExecutionCommand& OutCommand) const
{
	OutCommand = Fdemo_mapShanmenFormationInfluenceExecutionCommand();
	if (!RequestId.IsValid() || !IsValid())
	{
		return false;
	}
	for (const FRecord& Record : Records)
	{
		if (Record.Request.RequestId == RequestId)
		{
			OutCommand = Record.Command;
			return true;
		}
	}
	return false;
}
