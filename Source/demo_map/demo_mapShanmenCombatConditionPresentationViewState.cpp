#include "demo_mapShanmenCombatConditionPresentationViewState.h"

#include "ShanmenDeterministicId.h"

namespace
{
	bool IsKnownMode(
		const Edemo_mapShanmenCombatConditionPresentationViewMode Mode)
	{
		return Mode
			== Edemo_mapShanmenCombatConditionPresentationViewMode::Hidden
			|| Mode
				== Edemo_mapShanmenCombatConditionPresentationViewMode::Visible;
	}

	Edemo_mapShanmenCombatConditionPresentationViewMode ModeForEvent(
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event)
	{
		if (!Event.IsValid())
		{
			return Edemo_mapShanmenCombatConditionPresentationViewMode::Invalid;
		}
		switch (Event.GetCue())
		{
		case Edemo_mapShanmenCombatConditionPresentationCue::Activated:
		case Edemo_mapShanmenCombatConditionPresentationCue::Refreshed:
			return Event.GetCurrentStatus().IsActive()
				? Edemo_mapShanmenCombatConditionPresentationViewMode::Visible
				: Edemo_mapShanmenCombatConditionPresentationViewMode::Invalid;
		case Edemo_mapShanmenCombatConditionPresentationCue::Expired:
			return !Event.GetCurrentStatus().IsActive()
				? Edemo_mapShanmenCombatConditionPresentationViewMode::Hidden
				: Edemo_mapShanmenCombatConditionPresentationViewMode::Invalid;
		default:
			return Edemo_mapShanmenCombatConditionPresentationViewMode::Invalid;
		}
	}

	FGuid MakeViewStateId(
		const Fdemo_mapShanmenCombatConditionPresentationViewState& State)
	{
		if (!State.GetSourceEventId().IsValid()
			|| !State.GetCurrentStatusId().IsValid()
			|| !IsKnownMode(State.GetMode()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.PresentationViewState.r1"),
			{
				State.GetSourceEventId().ToString(EGuidFormats::Digits),
				State.GetCurrentStatusId().ToString(EGuidFormats::Digits),
				FString::FromInt(static_cast<uint8>(State.GetMode())),
				LexToString(State.GetObservedTick()),
				LexToString(State.GetConditionRevision())
			});
	}

	bool HasSameAuthorityIdentity(
		const Fdemo_mapShanmenCombatConditionPresentationViewState& Previous,
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event)
	{
		return Previous.GetRunId() == Event.GetRunId()
			&& Previous.GetTargetEntityId() == Event.GetTargetEntityId()
			&& Previous.GetTimelineId() == Event.GetTimelineId()
			&& Previous.GetDefinitionId() == Event.GetDefinitionId();
	}

	Fdemo_mapShanmenCombatConditionPresentationViewReduceResult Reject(
		const Edemo_mapShanmenCombatConditionPresentationViewReduceStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenCombatConditionPresentationViewReduceResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

}

bool Fdemo_mapShanmenCombatConditionPresentationViewState::IsEmpty() const
{
	return !ViewStateId.IsValid()
		&& !SourceEvent.GetEventId().IsValid()
		&& !SourceEvent.GetPreviousStatusId().IsValid()
		&& !SourceEvent.GetCurrentStatusId().IsValid()
		&& SourceEvent.GetCue()
			== Edemo_mapShanmenCombatConditionPresentationCue::Invalid
		&& !RunId.IsValid()
		&& !TargetEntityId.IsValid()
		&& !TimelineId.IsValid()
		&& DefinitionId.IsNone()
		&& Mode
			== Edemo_mapShanmenCombatConditionPresentationViewMode::Invalid
		&& ObservedTick == INDEX_NONE
		&& RemainingTicks == 0
		&& DurationTicks == 0
		&& TimelineTicksPerSecond == 0
		&& ConditionRevision == INDEX_NONE
		&& MoveSpeedMultiplier == 1.0f;
}

bool Fdemo_mapShanmenCombatConditionPresentationViewState::IsValid() const
{
	if (!ViewStateId.IsValid()
		|| !SourceEvent.IsValid()
		|| !IsKnownMode(Mode)
		|| Mode != ModeForEvent(SourceEvent))
	{
		return false;
	}

	const Fdemo_mapShanmenCombatConditionStatusSnapshot& Status =
		SourceEvent.GetCurrentStatus();
	return RunId == Status.GetRunId()
		&& TargetEntityId == Status.GetTargetEntityId()
		&& TimelineId == Status.GetTimelineId()
		&& DefinitionId == Status.GetDefinitionId()
		&& ObservedTick == Status.GetObservedTick()
		&& RemainingTicks == Status.GetRemainingTicks()
		&& DurationTicks == Status.GetDurationTicks()
		&& TimelineTicksPerSecond == Status.GetTimelineTicksPerSecond()
		&& ConditionRevision == Status.GetConditionRevision()
		&& MoveSpeedMultiplier == Status.GetMoveSpeedMultiplier()
		&& ViewStateId == MakeViewStateId(*this);
}

bool Fdemo_mapShanmenCombatConditionPresentationViewState::Matches(
	const Fdemo_mapShanmenCombatConditionPresentationViewState& Other) const
{
	return IsValid() && Other.IsValid()
		&& ViewStateId == Other.ViewStateId;
}

Fdemo_mapShanmenCombatConditionPresentationViewState
Fdemo_mapShanmenCombatConditionPresentationViewReducer::BuildState(
	const Fdemo_mapShanmenCombatConditionPresentationEvent& Event)
{
	Fdemo_mapShanmenCombatConditionPresentationViewState State;
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& Status =
		Event.GetCurrentStatus();
	State.SourceEvent = Event;
	State.RunId = Status.GetRunId();
	State.TargetEntityId = Status.GetTargetEntityId();
	State.TimelineId = Status.GetTimelineId();
	State.DefinitionId = Status.GetDefinitionId();
	State.Mode = ModeForEvent(Event);
	State.ObservedTick = Status.GetObservedTick();
	State.RemainingTicks = Status.GetRemainingTicks();
	State.DurationTicks = Status.GetDurationTicks();
	State.TimelineTicksPerSecond = Status.GetTimelineTicksPerSecond();
	State.ConditionRevision = Status.GetConditionRevision();
	State.MoveSpeedMultiplier = Status.GetMoveSpeedMultiplier();
	State.ViewStateId = MakeViewStateId(State);
	return State;
}

Fdemo_mapShanmenCombatConditionPresentationViewReduceResult
Fdemo_mapShanmenCombatConditionPresentationViewReducer::Reduce(
	const Fdemo_mapShanmenCombatConditionPresentationViewState& PreviousState,
	const Fdemo_mapShanmenCombatConditionPresentationEvent& Event)
{
	if (!Event.IsValid())
	{
		return Reject(
			Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
				EventInvalid,
			TEXT("Presentation view reduction requires one valid event."));
	}

	if (PreviousState.IsEmpty())
	{
		if (Event.GetCue()
			!= Edemo_mapShanmenCombatConditionPresentationCue::Activated)
		{
			return Reject(
				Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
					PreviousStateRequired,
				TEXT("A consumer must observe activation before later events."));
		}
	}
	else
	{
		if (!PreviousState.IsValid())
		{
			return Reject(
				Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
					PreviousStateInvalid,
				TEXT("Consumer-owned previous presentation state is invalid."));
		}
		if (PreviousState.GetSourceEventId() == Event.GetEventId())
		{
			Fdemo_mapShanmenCombatConditionPresentationViewReduceResult Result;
			Result.Status =
				Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
					DuplicateEvent;
			Result.Diagnostic =
				TEXT("Consumer has already reduced this exact event.");
			Result.State = PreviousState;
			return Result;
		}
		if (!HasSameAuthorityIdentity(PreviousState, Event))
		{
			return Reject(
				Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
					IdentityMismatch,
				TEXT("Presentation view rejects a foreign authority event."));
		}
		if (Event.GetObservedTick() < PreviousState.GetObservedTick()
			|| Event.GetCurrentConditionRevision()
				<= PreviousState.GetConditionRevision())
		{
			return Reject(
				Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
					StaleEvent,
				TEXT("Presentation view rejects a stale consumer event."));
		}
		if (Event.GetPreviousStatusId()
			!= PreviousState.GetCurrentStatusId())
		{
			return Reject(
				Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
					SequenceMismatch,
				TEXT("Presentation event does not continue the consumer cursor."));
		}
	}

	const Fdemo_mapShanmenCombatConditionPresentationViewState Candidate =
		BuildState(Event);
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
				ViewStateRejected,
			TEXT("Presentation view state failed deterministic validation."));
	}

	Fdemo_mapShanmenCombatConditionPresentationViewReduceResult Result;
	Result.Status =
		Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::Reduced;
	Result.Diagnostic =
		TEXT("Consumer-owned Meridian Shock presentation view was reduced.");
	Result.State = Candidate;
	return Result;
}

bool Udemo_mapShanmenCombatConditionPresentationViewLibrary::
	TryReduceMeridianShockPresentationView(
		const Fdemo_mapShanmenCombatConditionPresentationViewState& PreviousState,
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event,
		Fdemo_mapShanmenCombatConditionPresentationViewState& OutState)
{
	OutState = Fdemo_mapShanmenCombatConditionPresentationViewState();
	const Fdemo_mapShanmenCombatConditionPresentationViewReduceResult Result =
		Fdemo_mapShanmenCombatConditionPresentationViewReducer::Reduce(
			PreviousState,
			Event);
	if (!Result.IsReduced())
	{
		return false;
	}
	OutState = Result.State;
	return true;
}
