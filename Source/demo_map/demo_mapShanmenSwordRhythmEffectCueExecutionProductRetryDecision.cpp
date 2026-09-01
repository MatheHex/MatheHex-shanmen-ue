#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FDecision =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision;
	using FDecisionResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionResult;
	using FRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest;
	using EDecisionOutcome =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionOutcome;
	using EDecisionStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionStatus;
	using EPrepareStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryPrepareStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool RecordsMatch(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord& A,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord& B)
	{
		return A.IsValid() && B.IsValid()
			&& A.Envelope.Matches(B.Envelope)
			&& A.Result.Status == B.Result.Status
			&& A.Result.bReplay == B.Result.bReplay
			&& A.Result.bHostStateCommitted == B.Result.bHostStateCommitted
			&& A.Result.Router.Status == B.Result.Router.Status;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed
	DeriveRetrySeed(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord&
			Latest,
		const FRequest& Request)
	{
		const auto& Projection = PreparedDispatch.GetProjection();
		const auto& PlanSeed = PreparedDispatch.GetPlan().GetSeed();
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetrySeed
			Result;
		Result.RetrySeed = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionProductRetryDecision.r1"),
			{
				GuidDigits(Request.Policy.PolicySeed),
				FString::FromInt(Request.Policy.MaxRenewals),
				FString::FromInt(Request.RenewalsUsed),
				GuidDigits(PlanSeed.DispatchSeed),
				GuidDigits(PlanSeed.VisualConsumerScopeId),
				GuidDigits(PlanSeed.AudioConsumerScopeId),
				GuidDigits(Projection.GetRunId()),
				GuidDigits(Projection.GetEvent().GetEventId()),
				FString::FromInt(Projection.GetObservationRevision()),
				GuidDigits(Host.GetHostId()),
				GuidDigits(Host.GetBatchId()),
				FString::Printf(
					TEXT("%lld"),
					static_cast<long long>(Host.GetNextSequence())),
				GuidDigits(Latest.Envelope.GetDispatchId()),
				GuidDigits(Latest.Envelope.GetCommand().GetCommandId())
			});
		return Result;
	}

	EDecisionStatus MapRejectedPreparation(const EPrepareStatus Status)
	{
		switch (Status)
		{
		case EPrepareStatus::PreparedDispatchInvalid:
			return EDecisionStatus::PreparedDispatchInvalid;
		case EPrepareStatus::HostInvalid:
			return EDecisionStatus::HostInvalid;
		case EPrepareStatus::PreparedRootMismatch:
			return EDecisionStatus::PreparedRootMismatch;
		case EPrepareStatus::StateInvalid:
		case EPrepareStatus::SeedInvalid:
			return EDecisionStatus::StateInvalid;
		case EPrepareStatus::CaptureRejected:
		default:
			return EDecisionStatus::PreparationRejected;
		}
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision::
IsValid() const
{
	if (!PreparedDispatch.IsValid()
		|| !Request.IsValid()
		|| !RetrySeed.IsValid()
		|| !ObservedRecord.IsValid()
		|| ObservedNextSequence <= 0
		|| ObservedRecordCount <= 0
		|| ObservedNextSequence != ObservedRecordCount
		|| ObservedRecord.Envelope.GetSequence() + 1
			!= ObservedNextSequence
		|| ObservedRecord.Envelope.GetHostId()
			!= PreparedDispatch.GetPlan().GetTransactionIdentity().HostId)
	{
		return false;
	}

	const bool bPreparedRetryValid = PreparedRetry.IsValid();
	switch (Outcome)
	{
	case EDecisionOutcome::Retry:
		return PreparationStatus == EPrepareStatus::Prepared
			&& !bObservedTerminal
			&& Request.HasBudget()
			&& NextRenewalsUsed == Request.RenewalsUsed + 1
			&& bPreparedRetryValid
			&& PreparedRetry.GetPreparedDispatch().Matches(PreparedDispatch)
			&& PreparedRetry.GetSeed().RetrySeed == RetrySeed.RetrySeed
			&& PreparedRetry.GetSourceProcessEnvelope().Matches(
				ObservedRecord.Envelope);
	case EDecisionOutcome::StopBudgetExhausted:
		return PreparationStatus == EPrepareStatus::Prepared
			&& !bObservedTerminal
			&& !Request.HasBudget()
			&& Request.RenewalsUsed == Request.Policy.MaxRenewals
			&& NextRenewalsUsed == Request.RenewalsUsed
			&& !bPreparedRetryValid;
	case EDecisionOutcome::StopCompleted:
		return PreparationStatus == EPrepareStatus::RetryStateUnavailable
			&& bObservedTerminal
			&& NextRenewalsUsed == Request.RenewalsUsed
			&& !bPreparedRetryValid;
	case EDecisionOutcome::StopNotRetryable:
		return PreparationStatus == EPrepareStatus::RetryStateUnavailable
			&& !bObservedTerminal
			&& NextRenewalsUsed == Request.RenewalsUsed
			&& !bPreparedRetryValid;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision::
Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision&
		Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| !PreparedDispatch.Matches(Other.PreparedDispatch)
		|| !Request.Matches(Other.Request)
		|| Outcome != Other.Outcome
		|| PreparationStatus != Other.PreparationStatus
		|| RetrySeed.RetrySeed != Other.RetrySeed.RetrySeed
		|| !RecordsMatch(ObservedRecord, Other.ObservedRecord)
		|| ObservedNextSequence != Other.ObservedNextSequence
		|| ObservedRecordCount != Other.ObservedRecordCount
		|| bObservedTerminal != Other.bObservedTerminal
		|| NextRenewalsUsed != Other.NextRenewalsUsed)
	{
		return false;
	}
	return Outcome == EDecisionOutcome::Retry
		? PreparedRetry.Matches(Other.PreparedRetry)
		: !PreparedRetry.IsValid() && !Other.PreparedRetry.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionResult::
IsDecided() const
{
	return Status == EDecisionStatus::Decided
		&& PreparationStatus == Decision.GetPreparationStatus()
		&& Decision.IsValid();
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionService::
Decide(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
		PreparedDispatch,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
		Request)
{
	FDecisionResult Result;
	if (!Request.IsValid())
	{
		Result.Status = EDecisionStatus::RequestInvalid;
		Result.Diagnostic = TEXT(
			"Retry decision requires one valid caller-owned policy and consumed-renewal count.");
		return Result;
	}
	if (!PreparedDispatch.IsValid())
	{
		Result.Status = EDecisionStatus::PreparedDispatchInvalid;
		Result.Diagnostic = TEXT(
			"Retry decision requires one valid frozen Product dispatch root.");
		return Result;
	}
	if (!Host.IsValid() || !Host.IsBound() || Host.GetNextSequence() <= 0)
	{
		Result.Status = EDecisionStatus::HostInvalid;
		Result.Diagnostic = TEXT(
			"Retry decision requires one valid bound command Host with durable evidence.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord Latest;
	if (!Host.TryGetRecord(Host.GetNextSequence() - 1, Latest))
	{
		Result.Status = EDecisionStatus::HostInvalid;
		Result.Diagnostic = TEXT(
			"Retry decision could not read the Host's latest durable receipt.");
		return Result;
	}

	const auto RetrySeed =
		DeriveRetrySeed(PreparedDispatch, Host, Latest, Request);
	const auto Preparation =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryService::
			PrepareRetry(PreparedDispatch, Host, RetrySeed);
	Result.PreparationStatus = Preparation.Status;
	if (Preparation.Status != EPrepareStatus::Prepared
		&& Preparation.Status != EPrepareStatus::RetryStateUnavailable)
	{
		Result.Status = MapRejectedPreparation(Preparation.Status);
		Result.Diagnostic = Preparation.Diagnostic;
		return Result;
	}

	Result.Decision.PreparedDispatch = PreparedDispatch;
	Result.Decision.Request = Request;
	Result.Decision.PreparationStatus = Preparation.Status;
	Result.Decision.RetrySeed = RetrySeed;
	Result.Decision.ObservedRecord = Latest;
	Result.Decision.ObservedNextSequence = Host.GetNextSequence();
	Result.Decision.ObservedRecordCount = Host.GetRecordCount();
	Result.Decision.bObservedTerminal = Host.IsTerminal();
	Result.Decision.NextRenewalsUsed = Request.RenewalsUsed;

	if (Preparation.Status == EPrepareStatus::Prepared)
	{
		if (Request.HasBudget())
		{
			Result.Decision.Outcome = EDecisionOutcome::Retry;
			Result.Decision.NextRenewalsUsed = Request.RenewalsUsed + 1;
			Result.Decision.PreparedRetry = Preparation.Prepared;
			Result.Diagnostic = TEXT(
				"Retry budget and durable receipt prepared one caller-owned continuation.");
		}
		else
		{
			Result.Decision.Outcome = EDecisionOutcome::StopBudgetExhausted;
			Result.Diagnostic = TEXT(
				"Latest receipt is retryable, but the caller-owned renewal budget is exhausted.");
		}
	}
	else if (Host.IsTerminal())
	{
		Result.Decision.Outcome = EDecisionOutcome::StopCompleted;
		Result.Diagnostic = TEXT(
			"Command Host is already terminal; no retry continuation is available.");
	}
	else
	{
		Result.Decision.Outcome = EDecisionOutcome::StopNotRetryable;
		Result.Diagnostic = TEXT(
			"Latest durable receipt is not retryable; caller should stop.");
	}

	Result.Status = EDecisionStatus::Decided;
	if (!Result.IsDecided())
	{
		Result.Decision = FDecision();
		Result.Status = EDecisionStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Retry decision failed immutable-value consistency validation.");
	}
	return Result;
}
