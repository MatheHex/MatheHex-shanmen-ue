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

/** Immutable logical command request after the first valid spatial sample. */
class Fdemo_mapShanmenSwordQiCommandRequest
{
public:
	bool IsValid() const;
	bool Matches(const Fdemo_mapShanmenSwordQiCommandRequest& Other) const;
	const Fdemo_mapShanmenSwordQiCommandEvent& GetEvent() const
	{
		return Event;
	}
	const Fdemo_mapShanmenSwordQiInputSample& GetSample() const
	{
		return Sample;
	}

private:
	friend class Fdemo_mapShanmenSwordQiCommandEventOwner;

	Fdemo_mapShanmenSwordQiCommandEvent Event;
	Fdemo_mapShanmenSwordQiInputSample Sample;
};

enum class Edemo_mapShanmenSwordQiCommandEventStatus : uint8
{
	Applied,
	OwnerInactive,
	OwnerInvalid,
	SequenceExhausted,
	EventInvalid,
	RequestInvalid,
	RequestNotOwned,
	PendingRetryOccupied,
	PendingRetryUnavailable,
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
	bool bPendingRetryAttempt = false;
	bool bPendingRetryStored = false;
	Fdemo_mapShanmenSwordQiCommandEvent Event;
	Fdemo_mapShanmenSwordQiCommandRequest Request;
	Fdemo_mapShanmenSwordQiInputResult Input;
	FString Diagnostic;

	bool IsAccepted() const;
	bool CanReplay() const
	{
		return bEventCommitted && Request.IsValid();
	}
};

/** Audit proof that one pending frozen request was explicitly cancelled. */
struct Fdemo_mapShanmenSwordQiPendingRetryCancellation
{
	FGuid RunId;
	Fdemo_mapShanmenSwordQiCommandRequest Request;

	bool IsValid() const;
};

/** Run-teardown evidence before logical command identity state is reset. */
struct Fdemo_mapShanmenSwordQiCommandEventEndSummary
{
	FGuid RunId;
	uint64 CommittedEventCount = 0;
	Fdemo_mapShanmenSwordQiCommandRequest PendingRetryAtTeardown;

	bool IsValid() const;
	bool HadPendingRetry() const
	{
		return PendingRetryAtTeardown.IsValid();
	}
};

/**
 * Run-scoped owner of logical Sword Qi command-event identity.
 *
 * A fresh command receives one deterministic InputEventId and delegates once
 * to the P18.5 input adapter. Pre-route gates do not consume the sequence.
 * The first valid spatial sample freezes an immutable command request and
 * commits its event identity. Product rejection cannot replace that sample.
 * Explicit replay requires the frozen request, never calls an external
 * sampler and never allocates another sequence. No physical key, spatial
 * authority, retry loop, item, attribute, Actor, inventory, projectile or
 * damage authority lives here. Exactly one HostBusy request may be retained
 * for explicit retry or cancellation; no other rejection enters that slot.
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
		const Fdemo_mapShanmenSwordQiCommandRequest& Request,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
			const FGuid&,
			const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput);
	Fdemo_mapShanmenSwordQiCommandEventResult TryRetryPending(
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
			const FGuid&,
			const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput);
	bool TryCancelPending(
		Fdemo_mapShanmenSwordQiPendingRetryCancellation& OutCancellation,
		FString& OutDiagnostic);
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
	bool HasPendingRetry() const { return PendingRetryRequest.IsValid(); }
	const Fdemo_mapShanmenSwordQiCommandRequest* GetPendingRetryRequest() const
	{
		return HasPendingRetry() ? &PendingRetryRequest : nullptr;
	}

private:
	Fdemo_mapShanmenSwordQiCommandEventResult RouteNewEvent(
		const Fdemo_mapShanmenSwordQiCommandEvent& Event,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
			RouteInput);
	Fdemo_mapShanmenSwordQiCommandEventResult RouteFrozenRequest(
		const Fdemo_mapShanmenSwordQiCommandRequest& Request,
		bool bPendingRetryAttempt,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
			const FGuid&,
			const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput);
	void UpdatePendingRetry(
		Fdemo_mapShanmenSwordQiCommandEventResult& InOutResult);
	static bool IsRetryableProductRejection(
		const Fdemo_mapShanmenSwordQiCommandEventResult& Result);

	FGuid RunId;
	uint64 NextEventSequence = 1;
	Fdemo_mapShanmenSwordQiCommandRequest PendingRetryRequest;
};
