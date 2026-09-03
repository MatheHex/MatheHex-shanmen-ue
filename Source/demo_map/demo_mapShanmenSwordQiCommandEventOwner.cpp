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

bool Fdemo_mapShanmenSwordQiCommandEventResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		&& bEventCommitted
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
	return RouteFrozenRequest(Request, RouteFrozenInput);
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
		return Result;
	}

	Result.Status = Result.Input.IsAccepted()
		? Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		: Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected;
	Result.Diagnostic = Result.Input.Diagnostic;
	return Result;
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::RouteFrozenRequest(
	const Fdemo_mapShanmenSwordQiCommandRequest& Request,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
		const FGuid&,
		const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput)
{
	const Fdemo_mapShanmenSwordQiCommandEvent& Event = Request.GetEvent();
	Fdemo_mapShanmenSwordQiCommandEventResult Result;
	Result.bEventCommitted = true;
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
		return Result;
	}

	Result.Status = Result.Input.IsAccepted()
		? Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		: Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected;
	Result.Diagnostic = Result.Input.Diagnostic;
	return Result;
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
	return NextEventSequence > 0
		&& (RunId.IsValid() || NextEventSequence == 1);
}

bool Fdemo_mapShanmenSwordQiCommandEventOwner::IsEmpty() const
{
	return !RunId.IsValid() && NextEventSequence == 1;
}

void Fdemo_mapShanmenSwordQiCommandEventOwner::Reset()
{
	*this = Fdemo_mapShanmenSwordQiCommandEventOwner();
}
