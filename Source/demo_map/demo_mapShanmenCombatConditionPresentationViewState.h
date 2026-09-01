#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "demo_mapShanmenCombatConditionPresentationEvent.h"

#include "demo_mapShanmenCombatConditionPresentationViewState.generated.h"

/** Consumer-local display mode derived from one authoritative condition event. */
UENUM(BlueprintType)
enum class Edemo_mapShanmenCombatConditionPresentationViewMode : uint8
{
	Invalid,
	Hidden,
	Visible
};

/**
 * Immutable display model for one consumer's latest Meridian Shock event.
 *
 * The value owns no condition, timeline, dispatch, widget, timer, or shared
 * cursor. A HUD, audio, VFX, or debug consumer may keep its own copy and pass
 * it back to the stateless reducer when it observes the next event.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenCombatConditionPresentationViewState
{
	GENERATED_BODY()

public:
	bool IsEmpty() const;
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenCombatConditionPresentationViewState& Other)
		const;

	const FGuid& GetViewStateId() const { return ViewStateId; }
	const FGuid& GetSourceEventId() const
	{
		return SourceEvent.GetEventId();
	}
	const FGuid& GetCurrentStatusId() const
	{
		return SourceEvent.GetCurrentStatusId();
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	FName GetDefinitionId() const { return DefinitionId; }
	Edemo_mapShanmenCombatConditionPresentationCue GetLastCue() const
	{
		return SourceEvent.GetCue();
	}
	Edemo_mapShanmenCombatConditionPresentationViewMode GetMode() const
	{
		return Mode;
	}
	int64 GetObservedTick() const { return ObservedTick; }
	int64 GetRemainingTicks() const { return RemainingTicks; }
	int64 GetDurationTicks() const { return DurationTicks; }
	int64 GetTimelineTicksPerSecond() const
	{
		return TimelineTicksPerSecond;
	}
	int64 GetConditionRevision() const { return ConditionRevision; }
	float GetMoveSpeedMultiplier() const { return MoveSpeedMultiplier; }
	bool IsVisible() const
	{
		return Mode
			== Edemo_mapShanmenCombatConditionPresentationViewMode::Visible;
	}

private:
	friend class Fdemo_mapShanmenCombatConditionPresentationViewReducer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	FGuid ViewStateId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenCombatConditionPresentationEvent SourceEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	FGuid TargetEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	FName DefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenCombatConditionPresentationViewMode Mode =
		Edemo_mapShanmenCombatConditionPresentationViewMode::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	int64 ObservedTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	int64 RemainingTicks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	int64 DurationTicks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	int64 TimelineTicksPerSecond = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	int64 ConditionRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|PresentationView", meta = (AllowPrivateAccess = "true"))
	float MoveSpeedMultiplier = 1.0f;
};

enum class Edemo_mapShanmenCombatConditionPresentationViewReduceStatus : uint8
{
	Reduced,
	DuplicateEvent,
	EventInvalid,
	PreviousStateRequired,
	PreviousStateInvalid,
	IdentityMismatch,
	StaleEvent,
	SequenceMismatch,
	ViewStateRejected
};

struct Fdemo_mapShanmenCombatConditionPresentationViewReduceResult
{
	Edemo_mapShanmenCombatConditionPresentationViewReduceStatus Status =
		Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
			EventInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenCombatConditionPresentationViewState State;

	bool IsReduced() const
	{
		return Status
			== Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
				Reduced
			&& State.IsValid();
	}
	bool IsDuplicate() const
	{
		return Status
			== Edemo_mapShanmenCombatConditionPresentationViewReduceStatus::
				DuplicateEvent
			&& State.IsValid();
	}
};

/** Stateless event reducer; the caller owns and advances its previous state. */
class Fdemo_mapShanmenCombatConditionPresentationViewReducer
{
public:
	static Fdemo_mapShanmenCombatConditionPresentationViewReduceResult Reduce(
		const Fdemo_mapShanmenCombatConditionPresentationViewState& PreviousState,
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event);

private:
	static Fdemo_mapShanmenCombatConditionPresentationViewState BuildState(
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event);
};

/** Blueprint-pure facade; no consumer state is stored by this library. */
UCLASS()
class DEMO_MAP_API Udemo_mapShanmenCombatConditionPresentationViewLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Shanmen|Combat|Conditions|PresentationView")
	static bool TryReduceMeridianShockPresentationView(
		const Fdemo_mapShanmenCombatConditionPresentationViewState& PreviousState,
		const Fdemo_mapShanmenCombatConditionPresentationEvent& Event,
		Fdemo_mapShanmenCombatConditionPresentationViewState& OutState);
};
