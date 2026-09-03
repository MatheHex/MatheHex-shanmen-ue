#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordQiInputAdapter.h"

class Fdemo_mapShanmenSwordQiCommandEventOwner;

/** Immutable identity allocated for one logical Sword Qi start command. */
class Fdemo_mapShanmenSwordQiCommandEvent
{
public:
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetInputEventId() const { return InputEventId; }
	uint64 GetEventSequence() const { return EventSequence; }

private:
	friend class Fdemo_mapShanmenSwordQiCommandEventOwner;

	FGuid RunId;
	FGuid InputEventId;
	uint64 EventSequence = 0;
};

enum class Edemo_mapShanmenSwordQiCommandEventStatus : uint8
{
	Applied,
	OwnerInactive,
	OwnerInvalid,
	SequenceExhausted,
	EventInvalid,
	EventNotOwned,
	InputRejected,
	OwnerPostconditionFailed
};

/** Evidence for one new logical command or one explicit event replay. */
struct Fdemo_mapShanmenSwordQiCommandEventResult
{
	Edemo_mapShanmenSwordQiCommandEventStatus Status =
		Edemo_mapShanmenSwordQiCommandEventStatus::OwnerInactive;
	bool bNewEvent = false;
	bool bEventCommitted = false;
	Fdemo_mapShanmenSwordQiCommandEvent Event;
	Fdemo_mapShanmenSwordQiInputResult Input;
	FString Diagnostic;

	bool IsAccepted() const;
	bool CanReplay() const
	{
		return bEventCommitted && Event.IsValid();
	}
};

/** Run-teardown evidence before logical command identity state is reset. */
struct Fdemo_mapShanmenSwordQiCommandEventEndSummary
{
	FGuid RunId;
	uint64 CommittedEventCount = 0;

	bool IsValid() const { return RunId.IsValid(); }
};

/**
 * Run-scoped owner of logical Sword Qi command-event identity.
 *
 * A fresh command receives one deterministic InputEventId and delegates once
 * to the P18.5 input adapter. Pre-route gates do not consume the sequence.
 * Once the product route is invoked, the event is committed even when the
 * product rejects it, preventing automatic identity reuse after side effects.
 * Explicit replay requires the immutable committed event value and never
 * allocates another sequence. No physical key, spatial truth, retry loop,
 * item, attribute, Actor, inventory, projectile or damage authority lives
 * here.
 */
class Fdemo_mapShanmenSwordQiCommandEventOwner
{
public:
	static FGuid MakeInputEventId(
		const FGuid& RunId,
		uint64 EventSequence);

	bool TryBegin(const FGuid& RequestedRunId, FString& OutDiagnostic);
	Fdemo_mapShanmenSwordQiCommandEventResult TryIssue(
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
			RouteInput);
	Fdemo_mapShanmenSwordQiCommandEventResult TryReplay(
		const Fdemo_mapShanmenSwordQiCommandEvent& Event,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
			RouteInput);
	bool TryEnd(
		const FGuid& ExpectedRunId,
		Fdemo_mapShanmenSwordQiCommandEventEndSummary& OutSummary,
		FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const;
	const FGuid& GetRunId() const { return RunId; }
	uint64 GetNextEventSequence() const { return NextEventSequence; }
	uint64 NumCommittedEvents() const
	{
		return NextEventSequence > 0 ? NextEventSequence - 1 : 0;
	}

private:
	Fdemo_mapShanmenSwordQiCommandEventResult RouteEvent(
		const Fdemo_mapShanmenSwordQiCommandEvent& Event,
		bool bNewEvent,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
			RouteInput);

	FGuid RunId;
	uint64 NextEventSequence = 1;
};
