#include "demo_mapShanmenSwordRhythmPresentationEvent.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownCue(
		const Edemo_mapShanmenSwordRhythmPresentationCue Cue)
	{
		return Cue
			== Edemo_mapShanmenSwordRhythmPresentationCue::SequenceStarted
			|| Cue
				== Edemo_mapShanmenSwordRhythmPresentationCue::PreciseLink
			|| Cue
				== Edemo_mapShanmenSwordRhythmPresentationCue::
					SequenceRestartedEarly
			|| Cue
				== Edemo_mapShanmenSwordRhythmPresentationCue::
					SequenceRestartedLate;
	}

	Edemo_mapShanmenSwordRhythmPresentationCue CueForBand(
		const EShanmenSwordRhythmBand Band)
	{
		switch (Band)
		{
		case EShanmenSwordRhythmBand::Started:
			return Edemo_mapShanmenSwordRhythmPresentationCue::
				SequenceStarted;
		case EShanmenSwordRhythmBand::PreciseLinked:
			return Edemo_mapShanmenSwordRhythmPresentationCue::PreciseLink;
		case EShanmenSwordRhythmBand::RestartedEarly:
			return Edemo_mapShanmenSwordRhythmPresentationCue::
				SequenceRestartedEarly;
		case EShanmenSwordRhythmBand::RestartedLate:
			return Edemo_mapShanmenSwordRhythmPresentationCue::
				SequenceRestartedLate;
		default:
			return Edemo_mapShanmenSwordRhythmPresentationCue::Invalid;
		}
	}

	FGuid MakeEventId(
		const Fdemo_mapShanmenSwordRhythmPresentationEvent& Event)
	{
		if (!Event.GetPresentationStateId().IsValid()
			|| !Event.GetRunId().IsValid()
			|| !Event.GetReceiptId().IsValid()
			|| !Event.GetActivationId().IsValid()
			|| !Event.GetTimelineId().IsValid()
			|| !IsKnownCue(Event.GetCue()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.PresentationEvent.r1"),
			{
				GuidDigits(Event.GetPresentationStateId()),
				GuidDigits(Event.GetRunId()),
				GuidDigits(Event.GetReceiptId()),
				GuidDigits(Event.GetActivationId()),
				GuidDigits(Event.GetTimelineId()),
				FString::FromInt(static_cast<int32>(Event.GetCue())),
				FString::FromInt(Event.GetObservationRevision()),
				LexToString(Event.GetPreviousInputTick()),
				LexToString(Event.GetCurrentInputTick()),
				LexToString(Event.GetTransitionOffsetTicks()),
				LexToString(Event.GetLinkOpenOffsetTicks()),
				LexToString(Event.GetLinkCloseOffsetTicks()),
				LexToString(Event.GetTimelineTicksPerSecond())
			});
	}

	Fdemo_mapShanmenSwordRhythmPresentationEventAdaptResult Reject(
		const Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmPresentationEventAdaptResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmPresentationEvent::IsValid() const
{
	if (!EventId.IsValid()
		|| !PresentationStateId.IsValid()
		|| !RunId.IsValid()
		|| !ReceiptId.IsValid()
		|| !ActivationId.IsValid()
		|| !TimelineId.IsValid()
		|| !IsKnownCue(Cue)
		|| ObservationRevision <= 0
		|| CurrentInputTick < 0
		|| LinkOpenOffsetTicks < 0
		|| LinkCloseOffsetTicks <= LinkOpenOffsetTicks
		|| TimelineTicksPerSecond <= 0)
	{
		return false;
	}

	if (Cue
		== Edemo_mapShanmenSwordRhythmPresentationCue::SequenceStarted)
	{
		if (PreviousInputTick != INDEX_NONE
			|| TransitionOffsetTicks != INDEX_NONE)
		{
			return false;
		}
	}
	else
	{
		if (PreviousInputTick < 0 || CurrentInputTick < PreviousInputTick)
		{
			return false;
		}
		const int64 ExpectedOffset = CurrentInputTick - PreviousInputTick;
		if (TransitionOffsetTicks != ExpectedOffset)
		{
			return false;
		}

		const bool bCueMatchesOffset =
			(Cue
					== Edemo_mapShanmenSwordRhythmPresentationCue::
						SequenceRestartedEarly
				&& ExpectedOffset < LinkOpenOffsetTicks)
			|| (Cue
					== Edemo_mapShanmenSwordRhythmPresentationCue::PreciseLink
				&& ExpectedOffset >= LinkOpenOffsetTicks
				&& ExpectedOffset < LinkCloseOffsetTicks)
			|| (Cue
					== Edemo_mapShanmenSwordRhythmPresentationCue::
						SequenceRestartedLate
				&& ExpectedOffset >= LinkCloseOffsetTicks);
		if (!bCueMatchesOffset)
		{
			return false;
		}
	}

	return EventId == MakeEventId(*this);
}

bool Fdemo_mapShanmenSwordRhythmPresentationEvent::Matches(
	const Fdemo_mapShanmenSwordRhythmPresentationEvent& Other) const
{
	return IsValid() && Other.IsValid() && EventId == Other.EventId;
}

Fdemo_mapShanmenSwordRhythmPresentationEventAdaptResult
Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt(
	const Fdemo_mapShanmenSwordRhythmPresentationState& State)
{
	if (!State.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::StateInvalid,
			TEXT("Sword-rhythm presentation event requires one valid read model."));
	}

	const Edemo_mapShanmenSwordRhythmPresentationCue Cue =
		CueForBand(State.GetBand());
	if (!IsKnownCue(Cue))
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::CueUnsupported,
			TEXT("Sword-rhythm presentation band has no event cue mapping."));
	}

	Fdemo_mapShanmenSwordRhythmPresentationEvent Candidate;
	Candidate.PresentationStateId = State.GetPresentationStateId();
	Candidate.RunId = State.GetRunId();
	Candidate.ReceiptId = State.GetReceiptId();
	Candidate.ActivationId = State.GetActivationId();
	Candidate.TimelineId = State.GetTimelineId();
	Candidate.Cue = Cue;
	Candidate.ObservationRevision = State.GetObservationRevision();
	Candidate.PreviousInputTick = State.GetPreviousInputTick();
	Candidate.CurrentInputTick = State.GetCurrentInputTick();
	Candidate.TransitionOffsetTicks =
		State.GetPreviousInputTick() == INDEX_NONE
		? INDEX_NONE
		: State.GetCurrentInputTick() - State.GetPreviousInputTick();
	Candidate.LinkOpenOffsetTicks = State.GetLinkOpenOffsetTicks();
	Candidate.LinkCloseOffsetTicks = State.GetLinkCloseOffsetTicks();
	Candidate.TimelineTicksPerSecond = State.GetTimelineTicksPerSecond();
	Candidate.EventId = MakeEventId(Candidate);
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::EventRejected,
			TEXT("Sword-rhythm presentation event failed self-validation."));
	}

	Fdemo_mapShanmenSwordRhythmPresentationEventAdaptResult Result;
	Result.Status =
		Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::Adapted;
	Result.Diagnostic =
		TEXT("Read-only sword-rhythm presentation event was adapted.");
	Result.Event = Candidate;
	return Result;
}
