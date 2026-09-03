#include "demo_mapShanmenSwordQiCommandEventOwner.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SamplesEqual(
		const Fdemo_mapShanmenSwordQiInputSample& Left,
		const Fdemo_mapShanmenSwordQiInputSample& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetOrigin() == Right.GetOrigin()
			&& Left.GetAimDirection().Equals(Right.GetAimDirection());
	}

	Fdemo_mapShanmenSwordQiCommandEventResult Reject(
		const Edemo_mapShanmenSwordQiCommandEventStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordQiCommandEventResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordQiCommandEvent::IsValid() const
{
	return RunId.IsValid()
		&& EventSequence > 0
		&& InputEventId
			== Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
				RunId,
				EventSequence);
}

bool Fdemo_mapShanmenSwordQiCommandRequest::IsValid() const
{
	return Event.IsValid() && Sample.IsValid();
}

bool Fdemo_mapShanmenSwordQiCommandRequest::Matches(
	const Fdemo_mapShanmenSwordQiCommandRequest& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& Event.GetRunId() == Other.Event.GetRunId()
		&& Event.GetInputEventId() == Other.Event.GetInputEventId()
		&& Event.GetEventSequence() == Other.Event.GetEventSequence()
		&& SamplesEqual(Sample, Other.Sample);
}

bool Fdemo_mapShanmenSwordQiCommandAvailabilityProjection::IsValid() const
{
	bool bShapeValid = false;
	switch (State)
	{
	case Edemo_mapShanmenSwordQiCommandAvailabilityState::OwnerInactive:
		bShapeValid = !RunId.IsValid()
			&& NextEventSequence == 1
			&& !PendingEvent.IsValid();
		break;
	case Edemo_mapShanmenSwordQiCommandAvailabilityState::IssueReady:
		bShapeValid = RunId.IsValid()
			&& NextEventSequence > 0
			&& !PendingEvent.IsValid();
		break;
	case Edemo_mapShanmenSwordQiCommandAvailabilityState::PendingRetry:
		bShapeValid = RunId.IsValid()
			&& NextEventSequence > 1
			&& PendingEvent.IsValid()
			&& PendingEvent.GetRunId() == RunId
			&& PendingEvent.GetEventSequence() < NextEventSequence;
		break;
	default:
		return false;
	}

	return bShapeValid
		&& ProjectionId
			== Fdemo_mapShanmenSwordQiCommandEventOwner::
				MakeAvailabilityProjectionId(*this);
}

bool Fdemo_mapShanmenSwordQiCommandEventResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		&& bEventCommitted
		&& !bPendingRetryStored
		&& Event.IsValid()
		&& Request.IsValid()
		&& Request.GetEvent().GetRunId() == Event.GetRunId()
		&& Request.GetEvent().GetInputEventId() == Event.GetInputEventId()
		&& Request.GetEvent().GetEventSequence() == Event.GetEventSequence()
		&& Input.IsAccepted()
		&& Input.InputEventId == Event.GetInputEventId()
		&& Input.RunId == Event.GetRunId()
		&& SamplesEqual(Input.Sample, Request.GetSample());
}

bool Fdemo_mapShanmenSwordQiPendingRetryCancellation::IsValid() const
{
	return RunId.IsValid()
		&& Request.IsValid()
		&& Request.GetEvent().GetRunId() == RunId;
}

bool Fdemo_mapShanmenSwordQiCommandEventEndSummary::IsValid() const
{
	return RunId.IsValid()
		&& (!PendingRetryAtTeardown.IsValid()
			|| PendingRetryAtTeardown.GetEvent().GetRunId() == RunId);
}

FGuid Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
	const FGuid& RunId,
	const uint64 EventSequence)
{
	if (!RunId.IsValid() || EventSequence == 0)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.SwordQi.CommandEvent.r1")),
		{
			GuidDigits(RunId),
			FString::Printf(TEXT("%llu"), EventSequence)
		});
}

FGuid Fdemo_mapShanmenSwordQiCommandEventOwner::
MakeAvailabilityProjectionId(
	const Fdemo_mapShanmenSwordQiCommandAvailabilityProjection& Projection)
{
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.SwordQi.CommandAvailability.r1")),
		{
			FString::FromInt(static_cast<int32>(Projection.State)),
			GuidDigits(Projection.RunId),
			FString::Printf(
				TEXT("%llu"),
				Projection.NextEventSequence),
			GuidDigits(Projection.PendingEvent.GetInputEventId()),
			FString::Printf(
				TEXT("%llu"),
				Projection.PendingEvent.GetEventSequence())
		});
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsActive())
	{
		if (IsValid() && RunId == RequestedRunId)
		{
			OutDiagnostic =
				TEXT("Sword Qi command-event owner is already bound to this Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("An active Sword Qi command-event owner cannot change Run identity.");
		return false;
	}
	if (!RequestedRunId.IsValid() || !IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Sword Qi command-event owner requires empty valid state and one Run identity.");
		return false;
	}

	RunId = RequestedRunId;
	if (!IsValid())
	{
		Reset();
		OutDiagnostic =
			TEXT("Sword Qi command-event owner failed closed during Run binding.");
		return false;
	}
	OutDiagnostic =
		TEXT("Sword Qi command-event identity bound to the active combat Run.");
	return true;
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::TryIssue(
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
		RouteInput)
{
	if (!IsActive())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInactive,
			TEXT("Sword Qi logical command requires one active Run owner."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInvalid,
			TEXT("Sword Qi command-event owner invariants are invalid."));
	}
	if (HasPendingRetry())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::PendingRetryOccupied,
			TEXT("Explicitly retry or cancel the pending Sword Qi request before issuing another command."));
	}
	if (NextEventSequence == MAX_uint64)
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::SequenceExhausted,
			TEXT("Sword Qi command-event sequence exhausted without wrapping identity."));
	}

	Fdemo_mapShanmenSwordQiCommandEvent Event;
	Event.RunId = RunId;
	Event.EventSequence = NextEventSequence;
	Event.InputEventId = MakeInputEventId(RunId, NextEventSequence);
	if (!Event.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::EventInvalid,
			TEXT("Sword Qi logical command could not derive a stable event identity."));
	}
	return RouteNewEvent(Event, RouteInput);
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::TryReplay(
	const Fdemo_mapShanmenSwordQiCommandRequest& Request,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
		const FGuid&,
		const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput)
{
	if (!IsActive())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInactive,
			TEXT("Sword Qi event replay requires one active Run owner."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInvalid,
			TEXT("Sword Qi command-event owner invariants are invalid."));
	}
	if (HasPendingRetry())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::PendingRetryOccupied,
			TEXT("Use the pending-retry route while a Sword Qi HostBusy request is retained."));
	}
	if (!Request.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::RequestInvalid,
			TEXT("Sword Qi replay requires one valid frozen command request."));
	}
	const Fdemo_mapShanmenSwordQiCommandEvent& Event = Request.GetEvent();
	if (Event.GetRunId() != RunId
		|| Event.GetEventSequence() >= NextEventSequence)
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::RequestNotOwned,
			TEXT("Sword Qi replay request was not committed by this active Run owner."));
	}
	return RouteFrozenRequest(Request, false, RouteFrozenInput);
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::TryRetryPending(
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
		const FGuid&,
		const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput)
{
	if (!IsActive())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInactive,
			TEXT("Sword Qi pending retry requires one active Run owner."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInvalid,
			TEXT("Sword Qi command-event owner invariants are invalid."));
	}
	if (!HasPendingRetry())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::PendingRetryUnavailable,
			TEXT("No Sword Qi HostBusy request is pending explicit retry."));
	}

	const Fdemo_mapShanmenSwordQiCommandRequest Request = PendingRetryRequest;
	return RouteFrozenRequest(Request, true, RouteFrozenInput);
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::TryCancelPending(
	Fdemo_mapShanmenSwordQiPendingRetryCancellation& OutCancellation,
	FString& OutDiagnostic)
{
	OutCancellation = Fdemo_mapShanmenSwordQiPendingRetryCancellation();
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic =
			TEXT("Sword Qi pending cancellation requires one active Run owner.");
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic =
			TEXT("Sword Qi command-event owner invariants are invalid.");
		return false;
	}
	if (!HasPendingRetry())
	{
		OutDiagnostic = TEXT("No Sword Qi pending retry exists to cancel.");
		return false;
	}

	Fdemo_mapShanmenSwordQiPendingRetryCancellation Cancellation;
	Cancellation.RunId = RunId;
	Cancellation.Request = PendingRetryRequest;
	if (!Cancellation.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword Qi pending retry could not produce valid cancellation evidence.");
		return false;
	}

	PendingRetryRequest = Fdemo_mapShanmenSwordQiCommandRequest();
	if (!IsValid())
	{
		PendingRetryRequest = Cancellation.Request;
		OutDiagnostic =
			TEXT("Sword Qi pending cancellation failed owner postconditions.");
		return false;
	}

	OutCancellation = Cancellation;
	OutDiagnostic = TEXT("Cancelled one pending frozen Sword Qi request.");
	return true;
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::TryProjectAvailability(
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection& OutProjection,
	FString& OutDiagnostic) const
{
	OutProjection = Fdemo_mapShanmenSwordQiCommandAvailabilityProjection();
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic =
			TEXT("Sword Qi command availability requires a valid owner state.");
		return false;
	}

	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Candidate;
	Candidate.RunId = RunId;
	Candidate.NextEventSequence = NextEventSequence;
	if (!IsActive())
	{
		Candidate.State =
			Edemo_mapShanmenSwordQiCommandAvailabilityState::OwnerInactive;
		OutDiagnostic = TEXT("Sword Qi command owner is inactive.");
	}
	else if (HasPendingRetry())
	{
		Candidate.State =
			Edemo_mapShanmenSwordQiCommandAvailabilityState::PendingRetry;
		Candidate.PendingEvent = PendingRetryRequest.GetEvent();
		OutDiagnostic =
			TEXT("Sword Qi command owner requires explicit retry or cancel.");
	}
	else
	{
		Candidate.State =
			Edemo_mapShanmenSwordQiCommandAvailabilityState::IssueReady;
		OutDiagnostic = TEXT("Sword Qi command owner can issue a new request.");
	}
	Candidate.ProjectionId = MakeAvailabilityProjectionId(Candidate);
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword Qi command availability projection failed closed.");
		return false;
	}

	OutProjection = Candidate;
	return true;
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::RouteNewEvent(
	const Fdemo_mapShanmenSwordQiCommandEvent& Event,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
		RouteInput)
{
	Fdemo_mapShanmenSwordQiCommandEventResult Result;
	Result.bNewEvent = true;
	Result.Event = Event;
	Result.Input = RouteInput(Event.GetInputEventId());

	const bool bInputIdentityValid =
		Result.Input.InputEventId == Event.GetInputEventId()
		&& Result.Input.RunId == RunId
		&& (!Result.Input.bProductRouteInvoked
			|| Result.Input.IntentId
				== Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
					RunId,
					Event.GetInputEventId()));
	const bool bValidSampleCaptured = bInputIdentityValid
		&& Result.Input.bSpatialSampled
		&& Result.Input.Sample.IsValid();
	if (bValidSampleCaptured)
	{
		Result.Request.Event = Event;
		Result.Request.Sample = Result.Input.Sample;
	}
	const bool bMustConsumeIdentity = bValidSampleCaptured
		|| Result.Input.bProductRouteInvoked;
	if (bMustConsumeIdentity)
	{
		++NextEventSequence;
		Result.bEventCommitted = true;
	}
	const bool bRouteShapeValid = !Result.Input.bProductRouteInvoked
		|| bValidSampleCaptured;
	if (!bInputIdentityValid
		|| !bRouteShapeValid
		|| (bValidSampleCaptured && !Result.Request.IsValid())
		|| !IsValid())
	{
		Result.Status = Edemo_mapShanmenSwordQiCommandEventStatus::
			OwnerPostconditionFailed;
		Result.Diagnostic =
			TEXT("Sword Qi command adapter returned inconsistent event evidence.");
		UpdatePendingRetry(Result);
		return Result;
	}

	Result.Status = Result.Input.IsAccepted()
		? Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		: Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected;
	Result.Diagnostic = Result.Input.Diagnostic;
	UpdatePendingRetry(Result);
	return Result;
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::RouteFrozenRequest(
	const Fdemo_mapShanmenSwordQiCommandRequest& Request,
	const bool bPendingRetryAttempt,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
		const FGuid&,
		const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput)
{
	const Fdemo_mapShanmenSwordQiCommandEvent& Event = Request.GetEvent();
	Fdemo_mapShanmenSwordQiCommandEventResult Result;
	Result.bEventCommitted = true;
	Result.bPendingRetryAttempt = bPendingRetryAttempt;
	Result.Event = Event;
	Result.Request = Request;
	Result.Input = RouteFrozenInput(
		Event.GetInputEventId(),
		Request.GetSample());

	const bool bInputIdentityValid =
		Result.Input.InputEventId == Event.GetInputEventId()
		&& Result.Input.RunId == RunId
		&& (!Result.Input.bProductRouteInvoked
			|| Result.Input.IntentId
				== Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
					RunId,
					Event.GetInputEventId()));
	const bool bFrozenSamplePreserved = !Result.Input.bSpatialSampled
		|| SamplesEqual(Result.Input.Sample, Request.GetSample());
	const bool bRouteShapeValid = !Result.Input.bProductRouteInvoked
		|| Result.Input.bSpatialSampled;
	if (!bInputIdentityValid
		|| !bFrozenSamplePreserved
		|| !bRouteShapeValid
		|| !Request.IsValid()
		|| !IsValid())
	{
		Result.Status = Edemo_mapShanmenSwordQiCommandEventStatus::
			OwnerPostconditionFailed;
		Result.Diagnostic =
			TEXT("Sword Qi frozen replay returned inconsistent request evidence.");
		UpdatePendingRetry(Result);
		return Result;
	}

	Result.Status = Result.Input.IsAccepted()
		? Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		: Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected;
	Result.Diagnostic = Result.Input.Diagnostic;
	UpdatePendingRetry(Result);
	return Result;
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::
IsRetryableProductRejection(
	const Fdemo_mapShanmenSwordQiCommandEventResult& Result)
{
	return Result.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
		&& Result.bEventCommitted
		&& Result.Request.IsValid()
		&& Result.Input.Status
			== Edemo_mapShanmenSwordQiInputStatus::ProductRejected
		&& Result.Input.bProductRouteInvoked
		&& Result.Input.Product.Status
			== Edemo_mapShanmenSwordQiControllerStatus::RouteRejected
		&& Result.Input.Product.Route.Status
			== Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy;
}

void Fdemo_mapShanmenSwordQiCommandEventOwner::UpdatePendingRetry(
	Fdemo_mapShanmenSwordQiCommandEventResult& InOutResult)
{
	if (IsRetryableProductRejection(InOutResult))
	{
		if (!HasPendingRetry()
			|| PendingRetryRequest.Matches(InOutResult.Request))
		{
			PendingRetryRequest = InOutResult.Request;
		}
		else
		{
			InOutResult.Status = Edemo_mapShanmenSwordQiCommandEventStatus::
				OwnerPostconditionFailed;
			InOutResult.Diagnostic =
				TEXT("Sword Qi retry slot refused replacement by another frozen request.");
		}
	}
	else if (HasPendingRetry()
		&& PendingRetryRequest.Matches(InOutResult.Request)
		&& InOutResult.Input.bProductRouteInvoked
		&& InOutResult.Status
			!= Edemo_mapShanmenSwordQiCommandEventStatus::
				OwnerPostconditionFailed)
	{
		PendingRetryRequest = Fdemo_mapShanmenSwordQiCommandRequest();
	}

	InOutResult.bPendingRetryStored = HasPendingRetry()
		&& PendingRetryRequest.Matches(InOutResult.Request);
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::TryEnd(
	const FGuid& ExpectedRunId,
	Fdemo_mapShanmenSwordQiCommandEventEndSummary& OutSummary,
	FString& OutDiagnostic)
{
	OutSummary = Fdemo_mapShanmenSwordQiCommandEventEndSummary();
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		if (IsValid() && IsEmpty())
		{
			OutDiagnostic =
				TEXT("Sword Qi command-event owner is already empty.");
			return ExpectedRunId.IsValid();
		}
		OutDiagnostic =
			TEXT("Inactive Sword Qi command-event owner state is invalid.");
		return false;
	}

	OutSummary.RunId = RunId;
	OutSummary.CommittedEventCount = NumCommittedEvents();
	OutSummary.PendingRetryAtTeardown = PendingRetryRequest;
	if (!ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic =
			TEXT("Sword Qi command-event teardown identity does not match the active Run.");
		return false;
	}
	if (!IsValid() || !OutSummary.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword Qi command-event owner was inconsistent at teardown.");
		return false;
	}

	Reset();
	OutDiagnostic =
		TEXT("Released Sword Qi logical command-event identity state.");
	return true;
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::IsValid() const
{
	if (NextEventSequence == 0
		|| (!RunId.IsValid() && NextEventSequence != 1))
	{
		return false;
	}
	if (!HasPendingRetry())
	{
		return true;
	}

	const Fdemo_mapShanmenSwordQiCommandEvent& PendingEvent =
		PendingRetryRequest.GetEvent();
	return RunId.IsValid()
		&& PendingEvent.GetRunId() == RunId
		&& PendingEvent.GetEventSequence() < NextEventSequence;
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::IsEmpty() const
{
	return !RunId.IsValid()
		&& NextEventSequence == 1
		&& !HasPendingRetry();
}

void Fdemo_mapShanmenSwordQiCommandEventOwner::Reset()
{
	*this = Fdemo_mapShanmenSwordQiCommandEventOwner();
}
