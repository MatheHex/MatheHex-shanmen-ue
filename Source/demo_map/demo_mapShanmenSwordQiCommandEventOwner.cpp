#include "demo_mapShanmenSwordQiCommandEventOwner.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
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

bool Fdemo_mapShanmenSwordQiCommandEventResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSwordQiCommandEventStatus::Applied
		&& bEventCommitted
		&& Event.IsValid()
		&& Input.IsAccepted()
		&& Input.InputEventId == Event.GetInputEventId()
		&& Input.RunId == Event.GetRunId();
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
	return RouteEvent(Event, true, RouteInput);
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::TryReplay(
	const Fdemo_mapShanmenSwordQiCommandEvent& Event,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
		RouteInput)
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
	if (!Event.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::EventInvalid,
			TEXT("Sword Qi replay requires one valid immutable command event."));
	}
	if (Event.GetRunId() != RunId
		|| Event.GetEventSequence() >= NextEventSequence)
	{
		return Reject(
			Edemo_mapShanmenSwordQiCommandEventStatus::EventNotOwned,
			TEXT("Sword Qi replay event was not committed by this active Run owner."));
	}
	return RouteEvent(Event, false, RouteInput);
}

Fdemo_mapShanmenSwordQiCommandEventResult
Fdemo_mapShanmenSwordQiCommandEventOwner::RouteEvent(
	const Fdemo_mapShanmenSwordQiCommandEvent& Event,
	const bool bNewEvent,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
		RouteInput)
{
	Fdemo_mapShanmenSwordQiCommandEventResult Result;
	Result.bNewEvent = bNewEvent;
	Result.bEventCommitted = !bNewEvent;
	Result.Event = Event;
	Result.Input = RouteInput(Event.GetInputEventId());

	if (bNewEvent && Result.Input.bProductRouteInvoked)
	{
		++NextEventSequence;
		Result.bEventCommitted = true;
	}
	const bool bInputIdentityValid =
		Result.Input.InputEventId == Event.GetInputEventId()
		&& Result.Input.RunId == RunId
		&& (!Result.Input.bProductRouteInvoked
			|| Result.Input.IntentId
				== Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
					RunId,
					Event.GetInputEventId()));
	if (!bInputIdentityValid || !IsValid())
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
