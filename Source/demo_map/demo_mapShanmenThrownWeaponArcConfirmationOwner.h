#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcLaunchInputAdapter.h"

/**
 * Immutable, device-independent evidence that one logical Arc confirmation
 * occurred. The caller owns the event identity; this type owns the slot and
 * projection policy captured for that event.
 */
class Fdemo_mapShanmenThrownWeaponArcConfirmationIntent
{
public:
	static bool TryCapture(
		const FGuid& ConfirmationEventId,
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy,
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& OutIntent);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetConfirmationEventId() const
	{
		return ConfirmationEventId;
	}
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetPolicy() const
	{
		return Policy;
	}

private:
	FGuid IntentId;
	FGuid ConfirmationEventId;
	int32 HotbarSlotNumber = INDEX_NONE;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
};

enum class Edemo_mapShanmenThrownWeaponArcConfirmationStatus : uint8
{
	Invalid,
	IntentInvalid,
	OwnerInvalid,
	EventConflict,
	ConfirmationBlocked,
	ChoiceStateInvalid,
	CommandCaptureRejected,
	Routed,
	InputProtocolRejected
};

/** Immutable audit evidence for one original confirmation or exact replay. */
class Fdemo_mapShanmenThrownWeaponArcConfirmationResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasReplay() const { return bReplay; }
	Edemo_mapShanmenThrownWeaponArcConfirmationStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetChoiceStateReadCount() const { return ChoiceStateReadCount; }
	int32 GetLaunchRouteInvocationCount() const
	{
		return LaunchRouteInvocationCount;
	}
	const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& GetIntent() const
	{
		return Intent;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& GetLaunchCommand() const
	{
		return LaunchCommand;
	}
	const Fdemo_mapShanmenThrownWeaponArcLaunchInputResult& GetLaunchInput() const
	{
		return LaunchInput;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcConfirmationOwner;

	Edemo_mapShanmenThrownWeaponArcConfirmationStatus Status =
		Edemo_mapShanmenThrownWeaponArcConfirmationStatus::Invalid;
	bool bReplay = false;
	FString Diagnostic;
	int32 ChoiceStateReadCount = 0;
	int32 LaunchRouteInvocationCount = 0;
	Fdemo_mapShanmenThrownWeaponArcConfirmationIntent Intent;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponArcLaunchCommand LaunchCommand;
	Fdemo_mapShanmenThrownWeaponArcLaunchInputResult LaunchInput;
};

/**
 * Consumer-local owner for recent explicit Arc confirmation events.
 *
 * A new valid event is consumed once. If confirmation is locally eligible,
 * the owner reads the sole current choice once, captures the P20.15 command,
 * and invokes that route once. An exact retained replay returns frozen
 * evidence without reading or routing again; reusing an event identity with
 * different slot/policy fails closed. Retention is deliberately bounded and
 * owns no physical key, UI, World, Actor, item, Run, or product lifecycle.
 */
class Fdemo_mapShanmenThrownWeaponArcConfirmationOwner
{
public:
	static constexpr int32 MaximumRetainedEvents = 32;
	using FReadCurrentChoiceState = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceState()>;
	using FRouteLaunchCommand = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponArcLaunchInputResult(
			const Fdemo_mapShanmenThrownWeaponArcLaunchCommand&)>;

	Fdemo_mapShanmenThrownWeaponArcConfirmationResult Confirm(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent,
		bool bConfirmationAllowed,
		FReadCurrentChoiceState ReadCurrentChoiceState,
		FRouteLaunchCommand RouteLaunchCommand);

	bool IsValid() const;
	bool IsEmpty() const { return RetainedEvents.IsEmpty(); }
	int32 NumRetainedEvents() const { return RetainedEvents.Num(); }
	void Reset() { RetainedEvents.Reset(); }

private:
	struct FRetainedEvent
	{
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent Intent;
		Fdemo_mapShanmenThrownWeaponArcConfirmationResult Result;
	};

	void Retain(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent,
		const Fdemo_mapShanmenThrownWeaponArcConfirmationResult& Result);

	TArray<FRetainedEvent> RetainedEvents;
};
