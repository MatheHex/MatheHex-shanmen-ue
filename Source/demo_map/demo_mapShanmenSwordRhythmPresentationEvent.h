#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmPresentation.h"

#include "demo_mapShanmenSwordRhythmPresentationEvent.generated.h"

/** Narrow animation/state cue derived from one accepted rhythm read model. */
UENUM(BlueprintType)
enum class Edemo_mapShanmenSwordRhythmPresentationCue : uint8
{
	Invalid,
	SequenceStarted,
	PreciseLink,
	SequenceRestartedEarly,
	SequenceRestartedLate
};

/**
 * Immutable event value for presentation consumers.
 *
 * It is safe to poll: identical source state produces the same EventId. Each
 * UI or animation consumer may therefore keep its own last-seen identity
 * without mutating combat, Session or another consumer's cursor.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmPresentationEvent
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmPresentationEvent& Other) const;

	const FGuid& GetEventId() const { return EventId; }
	const FGuid& GetPresentationStateId() const
	{
		return PresentationStateId;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetActivationId() const { return ActivationId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	Edemo_mapShanmenSwordRhythmPresentationCue GetCue() const
	{
		return Cue;
	}
	int32 GetObservationRevision() const { return ObservationRevision; }
	int64 GetPreviousInputTick() const { return PreviousInputTick; }
	int64 GetCurrentInputTick() const { return CurrentInputTick; }
	int64 GetTransitionOffsetTicks() const { return TransitionOffsetTicks; }
	int64 GetLinkOpenOffsetTicks() const { return LinkOpenOffsetTicks; }
	int64 GetLinkCloseOffsetTicks() const { return LinkCloseOffsetTicks; }
	int64 GetTimelineTicksPerSecond() const { return TimelineTicksPerSecond; }

private:
	friend class Fdemo_mapShanmenSwordRhythmPresentationEventAdapter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	FGuid EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	FGuid PresentationStateId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	FGuid ActivationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenSwordRhythmPresentationCue Cue =
		Edemo_mapShanmenSwordRhythmPresentationCue::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int32 ObservationRevision = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int64 PreviousInputTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int64 CurrentInputTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int64 TransitionOffsetTicks = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int64 LinkOpenOffsetTicks = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int64 LinkCloseOffsetTicks = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|PresentationEvent", meta = (AllowPrivateAccess = "true"))
	int64 TimelineTicksPerSecond = 0;
};

enum class Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus : uint8
{
	Adapted,
	StateInvalid,
	CueUnsupported,
	EventRejected
};

struct Fdemo_mapShanmenSwordRhythmPresentationEventAdaptResult
{
	Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus Status =
		Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::StateInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmPresentationEvent Event;

	bool IsAdapted() const
	{
		return Status
			== Edemo_mapShanmenSwordRhythmPresentationEventAdaptStatus::Adapted
			&& Event.IsValid();
	}
};

/** Stateless read-model-to-event adapter; performs no dispatch or animation. */
class Fdemo_mapShanmenSwordRhythmPresentationEventAdapter
{
public:
	static Fdemo_mapShanmenSwordRhythmPresentationEventAdaptResult Adapt(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State);
};
