#include "demo_mapShanmenCombatConditionPresentationEvent.h"

#include "ShanmenDeterministicId.h"

namespace
{
	struct FTransitionClassification
	{
		Edemo_mapShanmenCombatConditionPresentationAdaptStatus Status =
			Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
				TransitionRejected;
		Edemo_mapShanmenCombatConditionPresentationCue Cue =
			Edemo_mapShanmenCombatConditionPresentationCue::Invalid;
		const TCHAR* Diagnostic = TEXT(
			"Meridian Shock presentation transition was rejected.");
	};

	bool IsKnownCue(
		const Edemo_mapShanmenCombatConditionPresentationCue Cue)
	{
		return Cue == Edemo_mapShanmenCombatConditionPresentationCue::Activated
			|| Cue
				== Edemo_mapShanmenCombatConditionPresentationCue::Refreshed
			|| Cue == Edemo_mapShanmenCombatConditionPresentationCue::Expired;
	}

	bool HasSameAuthorityIdentity(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& Previous,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& Current)
	{
		return Previous.GetRunId() == Current.GetRunId()
			&& Previous.GetTargetEntityId() == Current.GetTargetEntityId()
			&& Previous.GetTimelineId() == Current.GetTimelineId()
			&& Previous.GetDefinitionId() == Current.GetDefinitionId();
	}

	FTransitionClassification ClassifyTransition(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& Previous,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& Current)
	{
		if (!Previous.IsValid())
		{
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
					PreviousStatusInvalid,
				Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
				TEXT("Presentation transition requires a valid previous status.")
			};
		}
		if (!Current.IsValid())
		{
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
					CurrentStatusInvalid,
				Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
				TEXT("Presentation transition requires a valid current status.")
			};
		}
		if (!HasSameAuthorityIdentity(Previous, Current))
		{
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
					IdentityMismatch,
				Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
				TEXT("Presentation transition rejects cross-authority snapshots.")
			};
		}
		if (Current.GetObservedTick() < Previous.GetObservedTick()
			|| Current.GetConditionRevision()
				< Previous.GetConditionRevision())
		{
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
					StaleObservation,
				Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
				TEXT("Presentation transition rejects a stale current snapshot.")
			};
		}
		if (Previous.Matches(Current))
		{
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
					NoTransition,
				Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
				TEXT("Repeated polling has no presentation transition.")
			};
		}

		const int64 PreviousRevision = Previous.GetConditionRevision();
		const int64 CurrentRevision = Current.GetConditionRevision();
		if (!Previous.IsActive() && Current.IsActive())
		{
			if (CurrentRevision <= PreviousRevision)
			{
				return {};
			}
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::Adapted,
				Edemo_mapShanmenCombatConditionPresentationCue::Activated,
				TEXT("Meridian Shock became active.")
			};
		}
		if (Previous.IsActive() && Current.IsActive())
		{
			if (CurrentRevision == PreviousRevision)
			{
				if (Current.GetExpiryTick() != Previous.GetExpiryTick()
					|| Current.GetRemainingTicks()
						> Previous.GetRemainingTicks())
				{
					return {};
				}
				return {
					Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
						NoTransition,
					Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
					TEXT("Countdown progress does not emit a transition event.")
				};
			}
			if (Current.GetExpiryTick() < Previous.GetExpiryTick())
			{
				return {};
			}
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::Adapted,
				Edemo_mapShanmenCombatConditionPresentationCue::Refreshed,
				TEXT("Meridian Shock was refreshed by committed authority.")
			};
		}
		if (Previous.IsActive() && !Current.IsActive())
		{
			if (CurrentRevision <= PreviousRevision
				|| Current.GetObservedTick() < Previous.GetExpiryTick())
			{
				return {};
			}
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::Adapted,
				Edemo_mapShanmenCombatConditionPresentationCue::Expired,
				TEXT("Meridian Shock reached its authoritative expiry.")
			};
		}

		if (CurrentRevision == PreviousRevision)
		{
			return {
				Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
					NoTransition,
				Edemo_mapShanmenCombatConditionPresentationCue::Invalid,
				TEXT("Inactive timeline progress has no presentation transition.")
			};
		}
		return {};
	}

	FGuid MakeEventId(
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event)
	{
		if (!Event.GetPreviousStatusId().IsValid()
			|| !Event.GetCurrentStatusId().IsValid()
			|| !IsKnownCue(Event.GetCue()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.PresentationEvent.r1"),
			{
				Event.GetPreviousStatusId().ToString(EGuidFormats::Digits),
				Event.GetCurrentStatusId().ToString(EGuidFormats::Digits),
				FString::FromInt(static_cast<uint8>(Event.GetCue()))
			});
	}
}

bool Fdemo_mapShanmenCombatConditionPresentationEvent::IsValid() const
{
	const FTransitionClassification Classification =
		ClassifyTransition(PreviousStatus, CurrentStatus);
	return EventId.IsValid()
		&& Classification.Status
			== Edemo_mapShanmenCombatConditionPresentationAdaptStatus::Adapted
		&& Classification.Cue == Cue
		&& EventId == MakeEventId(*this);
}

bool Fdemo_mapShanmenCombatConditionPresentationEvent::Matches(
	const Fdemo_mapShanmenCombatConditionPresentationEvent& Other) const
{
	return IsValid() && Other.IsValid() && EventId == Other.EventId;
}

Fdemo_mapShanmenCombatConditionPresentationAdaptResult
Fdemo_mapShanmenCombatConditionPresentationEventAdapter::Adapt(
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& PreviousStatus,
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& CurrentStatus)
{
	const FTransitionClassification Classification =
		ClassifyTransition(PreviousStatus, CurrentStatus);
	Fdemo_mapShanmenCombatConditionPresentationAdaptResult Result;
	Result.Status = Classification.Status;
	Result.Diagnostic = Classification.Diagnostic;
	if (Classification.Status
		!= Edemo_mapShanmenCombatConditionPresentationAdaptStatus::Adapted)
	{
		return Result;
	}

	Fdemo_mapShanmenCombatConditionPresentationEvent Candidate;
	Candidate.PreviousStatus = PreviousStatus;
	Candidate.CurrentStatus = CurrentStatus;
	Candidate.Cue = Classification.Cue;
	Candidate.EventId = MakeEventId(Candidate);
	if (!Candidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
				EventRejected;
		Result.Diagnostic =
			TEXT("Presentation event failed deterministic self-validation.");
		return Result;
	}

	Result.Event = Candidate;
	return Result;
}

bool Udemo_mapShanmenCombatConditionPresentationLibrary::
	TryAdaptMeridianShockTransition(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& PreviousStatus,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& CurrentStatus,
		Fdemo_mapShanmenCombatConditionPresentationEvent& OutEvent)
{
	OutEvent = Fdemo_mapShanmenCombatConditionPresentationEvent();
	const Fdemo_mapShanmenCombatConditionPresentationAdaptResult Result =
		Fdemo_mapShanmenCombatConditionPresentationEventAdapter::Adapt(
			PreviousStatus,
			CurrentStatus);
	if (!Result.IsAdapted())
	{
		return false;
	}
	OutEvent = Result.Event;
	return true;
}
