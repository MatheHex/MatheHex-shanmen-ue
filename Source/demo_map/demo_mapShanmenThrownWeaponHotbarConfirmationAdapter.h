#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcConfirmationOwner.h"

/** Sole product-owned Arc projection policy used by hotbar confirmation. */
struct Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource
{
	static const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetCanonical();
};

enum class Edemo_mapShanmenThrownWeaponHotbarConfirmationStatus : uint8
{
	Invalid,
	InvalidSlot,
	InputBlocked,
	ConsumerUnavailable,
	ChoiceStateInvalid,
	StraightDelegated,
	StraightRouteProtocolRejected,
	ConfirmationSequenceExhausted,
	ArcPolicyInvalid,
	ArcIntentCaptureRejected,
	ArcDelegated,
	ArcRouteProtocolRejected
};

/**
 * Immutable audit evidence for one physical hotbar press at the
 * trajectory-aware confirmation boundary.
 */
class Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult
{
public:
	bool IsValid() const;
	bool ShouldPassThrough() const;
	bool IsAccepted() const;
	Edemo_mapShanmenThrownWeaponHotbarConfirmationStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	int32 GetChoiceStateReadCount() const { return ChoiceStateReadCount; }
	int32 GetPolicyReadCount() const { return PolicyReadCount; }
	int32 GetStraightRouteInvocationCount() const
	{
		return StraightRouteInvocationCount;
	}
	int32 GetArcRouteInvocationCount() const
	{
		return ArcRouteInvocationCount;
	}
	uint64 GetConfirmationOrdinal() const { return ConfirmationOrdinal; }
	const FGuid& GetConfirmationEventId() const
	{
		return ConfirmationEventId;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetPolicy() const
	{
		return Policy;
	}
	const Fdemo_mapShanmenThrownWeaponInputResult& GetStraightInput() const
	{
		return StraightInput;
	}
	const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& GetArcIntent() const
	{
		return ArcIntent;
	}
	const Fdemo_mapShanmenThrownWeaponArcConfirmationResult&
	GetArcConfirmation() const
	{
		return ArcConfirmation;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter;

	Edemo_mapShanmenThrownWeaponHotbarConfirmationStatus Status =
		Edemo_mapShanmenThrownWeaponHotbarConfirmationStatus::Invalid;
	FString Diagnostic;
	int32 HotbarSlotNumber = INDEX_NONE;
	int32 ChoiceStateReadCount = 0;
	int32 PolicyReadCount = 0;
	int32 StraightRouteInvocationCount = 0;
	int32 ArcRouteInvocationCount = 0;
	uint64 ConfirmationOrdinal = 0;
	FGuid ConfirmationEventId;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
	Fdemo_mapShanmenThrownWeaponInputResult StraightInput;
	Fdemo_mapShanmenThrownWeaponArcConfirmationIntent ArcIntent;
	Fdemo_mapShanmenThrownWeaponArcConfirmationResult ArcConfirmation;
};

/**
 * Controller-local bridge from the existing 1-9 hotbar path to either the
 * existing Straight route or P20.16 explicit Arc confirmation.
 *
 * Every Arc press consumes a fresh deterministic ordinal. The adapter owns
 * no physical key, UI, World, Actor, item, Run, inventory, or product state.
 */
class Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter
{
public:
	using FReadCurrentChoiceState = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceState()>;
	using FReadArcPolicy = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy()>;
	using FRouteStraight = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputResult(int32)>;
	using FRouteArcConfirmation = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponArcConfirmationResult(
			const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent&)>;

	static FGuid MakeConfirmationEventId(uint64 ConfirmationOrdinal);

	Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult Route(
		int32 HotbarSlotNumber,
		bool bInputAllowed,
		bool bConsumerAvailable,
		FReadCurrentChoiceState ReadCurrentChoiceState,
		FReadArcPolicy ReadArcPolicy,
		FRouteStraight RouteStraight,
		FRouteArcConfirmation RouteArcConfirmation);

	bool IsValid() const { return NextConfirmationOrdinal != 0; }
	void Reset() { NextConfirmationOrdinal = 1; }
	uint64 GetNextConfirmationOrdinal() const
	{
		return NextConfirmationOrdinal;
	}

#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING
	void SetNextConfirmationOrdinalForAutomation(const uint64 Value)
	{
		NextConfirmationOrdinal = Value;
	}
#endif

private:
	uint64 NextConfirmationOrdinal = 1;
};
