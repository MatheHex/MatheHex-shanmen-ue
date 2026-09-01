#include "demo_mapShanmenCombatConditionStatus.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenCombatConditionComponent.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString CanonicalFloat(const float Value)
	{
		return FString::Printf(TEXT("%.6f"), static_cast<double>(Value));
	}

	FGuid MakeStatusId(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& Status)
	{
		if (!Status.GetRunId().IsValid()
			|| !Status.GetTargetEntityId().IsValid()
			|| !Status.GetTimelineId().IsValid()
			|| Status.GetDefinitionId().IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.StatusSnapshot.r1"),
			{
				GuidDigits(Status.GetRunId()),
				GuidDigits(Status.GetTargetEntityId()),
				GuidDigits(Status.GetTimelineId()),
				Status.GetDefinitionId().ToString(),
				LexToString(Status.GetObservedTick()),
				LexToString(Status.GetExpiryTick()),
				LexToString(Status.GetRemainingTicks()),
				LexToString(Status.GetConditionRevision()),
				LexToString(Status.GetDurationTicks()),
				LexToString(Status.GetTimelineTicksPerSecond()),
				CanonicalFloat(Status.GetMoveSpeedMultiplier()),
				Status.IsActive() ? TEXT("1") : TEXT("0")
			});
	}
}

bool Fdemo_mapShanmenCombatConditionStatusSnapshot::IsValid() const
{
	if (!StatusId.IsValid()
		|| !RunId.IsValid()
		|| !TargetEntityId.IsValid()
		|| !TimelineId.IsValid()
		|| TimelineId
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId)
		|| DefinitionId
			!= Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		|| ObservedTick < 0
		|| ConditionRevision < 0
		|| DurationTicks
			!= Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDurationTicks()
		|| TimelineTicksPerSecond
			!= Fdemo_mapShanmenCombatRunFixedTimeline::
				CanonicalTicksPerSecond()
		|| !FMath::IsNearlyEqual(
			MoveSpeedMultiplier,
			Udemo_mapShanmenCombatConditionComponent::
				MeridianShockMoveSpeedMultiplier()))
	{
		return false;
	}

	if (bActive)
	{
		if (ConditionRevision <= 0
			|| ExpiryTick <= ObservedTick
			|| RemainingTicks != ExpiryTick - ObservedTick
			|| RemainingTicks <= 0
			|| RemainingTicks > DurationTicks)
		{
			return false;
		}
	}
	else if (ExpiryTick != INDEX_NONE || RemainingTicks != 0)
	{
		return false;
	}

	return StatusId == MakeStatusId(*this);
}

bool Fdemo_mapShanmenCombatConditionStatusSnapshot::Matches(
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& Other) const
{
	return IsValid() && Other.IsValid() && StatusId == Other.StatusId;
}

bool Fdemo_mapShanmenCombatConditionStatusSnapshot::TryCapture(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const int64 RequestedObservedTick,
	const int64 RequestedExpiryTick,
	const int64 RequestedConditionRevision,
	const bool bRequestedActive,
	Fdemo_mapShanmenCombatConditionStatusSnapshot& OutStatus)
{
	OutStatus = Fdemo_mapShanmenCombatConditionStatusSnapshot();
	if (bRequestedActive
		&& RequestedExpiryTick <= RequestedObservedTick)
	{
		return false;
	}

	Fdemo_mapShanmenCombatConditionStatusSnapshot Candidate;
	Candidate.RunId = RequestedRunId;
	Candidate.TargetEntityId = RequestedTargetEntityId;
	Candidate.TimelineId = RequestedTimelineId;
	Candidate.DefinitionId =
		Udemo_mapShanmenCombatConditionComponent::
			MeridianShockDefinitionId();
	Candidate.ObservedTick = RequestedObservedTick;
	Candidate.ExpiryTick = RequestedExpiryTick;
	Candidate.RemainingTicks = bRequestedActive
		? RequestedExpiryTick - RequestedObservedTick
		: 0;
	Candidate.ConditionRevision = RequestedConditionRevision;
	Candidate.DurationTicks =
		Udemo_mapShanmenCombatConditionComponent::
			MeridianShockDurationTicks();
	Candidate.TimelineTicksPerSecond =
		Fdemo_mapShanmenCombatRunFixedTimeline::CanonicalTicksPerSecond();
	Candidate.MoveSpeedMultiplier =
		Udemo_mapShanmenCombatConditionComponent::
			MeridianShockMoveSpeedMultiplier();
	Candidate.bActive = bRequestedActive;
	Candidate.StatusId = MakeStatusId(Candidate);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutStatus = Candidate;
	return true;
}
