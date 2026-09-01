#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "demo_mapShanmenCombatConditionStatus.h"

#include "demo_mapShanmenCombatConditionPresentationEvent.generated.h"

/** Transition cues a presentation consumer may emit from two status copies. */
UENUM(BlueprintType)
enum class Edemo_mapShanmenCombatConditionPresentationCue : uint8
{
	Invalid,
	Activated,
	Refreshed,
	Expired
};

/**
 * Immutable proof of one observable Meridian Shock presentation transition.
 *
 * The value owns no cursor, dispatch, timer, widget, condition, or attribute
 * state. Each consumer supplies its own previous snapshot and may deduplicate
 * repeated polling by EventId.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenCombatConditionPresentationEvent
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Other) const;

	const FGuid& GetEventId() const { return EventId; }
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& GetPreviousStatus()
		const
	{
		return PreviousStatus;
	}
	const Fdemo_mapShanmenCombatConditionStatusSnapshot& GetCurrentStatus()
		const
	{
		return CurrentStatus;
	}
	const FGuid& GetPreviousStatusId() const
	{
		return PreviousStatus.GetStatusId();
	}
	const FGuid& GetCurrentStatusId() const
	{
		return CurrentStatus.GetStatusId();
	}
	const FGuid& GetRunId() const { return CurrentStatus.GetRunId(); }
	const FGuid& GetTargetEntityId() const
	{
		return CurrentStatus.GetTargetEntityId();
	}
	const FGuid& GetTimelineId() const
	{
		return CurrentStatus.GetTimelineId();
	}
	FName GetDefinitionId() const
	{
		return CurrentStatus.GetDefinitionId();
	}
	Edemo_mapShanmenCombatConditionPresentationCue GetCue() const
	{
		return Cue;
	}
	int64 GetObservedTick() const
	{
		return CurrentStatus.GetObservedTick();
	}
	int64 GetPreviousConditionRevision() const
	{
		return PreviousStatus.GetConditionRevision();
	}
	int64 GetCurrentConditionRevision() const
	{
		return CurrentStatus.GetConditionRevision();
	}
	int64 GetRemainingTicks() const
	{
		return CurrentStatus.GetRemainingTicks();
	}

private:
	friend class Fdemo_mapShanmenCombatConditionPresentationEventAdapter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Presentation", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenCombatConditionStatusSnapshot PreviousStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Presentation", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenCombatConditionStatusSnapshot CurrentStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Presentation", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenCombatConditionPresentationCue Cue =
		Edemo_mapShanmenCombatConditionPresentationCue::Invalid;
};

enum class Edemo_mapShanmenCombatConditionPresentationAdaptStatus : uint8
{
	Adapted,
	NoTransition,
	PreviousStatusInvalid,
	CurrentStatusInvalid,
	IdentityMismatch,
	StaleObservation,
	TransitionRejected,
	EventRejected
};

struct Fdemo_mapShanmenCombatConditionPresentationAdaptResult
{
	Edemo_mapShanmenCombatConditionPresentationAdaptStatus Status =
		Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
			PreviousStatusInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenCombatConditionPresentationEvent Event;

	bool IsAdapted() const
	{
		return Status
			== Edemo_mapShanmenCombatConditionPresentationAdaptStatus::Adapted
			&& Event.IsValid();
	}
	bool IsNoTransition() const
	{
		return Status
			== Edemo_mapShanmenCombatConditionPresentationAdaptStatus::
				NoTransition
			&& !Event.IsValid();
	}
};

/** Stateless two-snapshot transition classifier and event projector. */
class Fdemo_mapShanmenCombatConditionPresentationEventAdapter
{
public:
	static Fdemo_mapShanmenCombatConditionPresentationAdaptResult Adapt(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& PreviousStatus,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& CurrentStatus);
};

/** Blueprint-pure facade; callers retain their own previous status copy. */
UCLASS()
class DEMO_MAP_API Udemo_mapShanmenCombatConditionPresentationLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Shanmen|Combat|Conditions|Presentation")
	static bool TryAdaptMeridianShockTransition(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& PreviousStatus,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& CurrentStatus,
		Fdemo_mapShanmenCombatConditionPresentationEvent& OutEvent);
};
