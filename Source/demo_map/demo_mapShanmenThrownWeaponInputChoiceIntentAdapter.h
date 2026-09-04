#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceControllerAdapter.h"

/** One device-independent request to edit the consumer-owned choice. */
enum class Edemo_mapShanmenThrownWeaponInputChoiceIntentKind : uint8
{
	Invalid,
	SelectTrajectory,
	SetArcTargetIntent,
	AdjustArcApex,
	ClearArcTargetIntent
};

/**
 * Immutable logical choice-edit intent without revision or device ownership.
 *
 * Capture canonicalizes target and apex values through the frozen P20.10
 * command contract. Equal logical values reproduce the same intent identity.
 */
class Fdemo_mapShanmenThrownWeaponInputChoiceIntent
{
public:
	static bool TryCaptureTrajectorySelection(
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
	static bool TryCaptureArcTargetIntent(
		const FVector2D& RawTargetIntent,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
	static bool TryCaptureArcApexAdjustment(
		double RawNormalizedDelta,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
	static bool TryCaptureArcTargetClear(
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);

	bool TryCaptureCommand(
		uint64 ExpectedRevision,
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand) const;
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	Edemo_mapShanmenThrownWeaponInputChoiceIntentKind GetKind() const
	{
		return Kind;
	}
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	const FVector2D& GetArcTargetIntent() const { return ArcTargetIntent; }
	double GetArcApexAdjustmentDelta() const
	{
		return ArcApexAdjustmentDelta;
	}

private:
	FGuid IntentId;
	Edemo_mapShanmenThrownWeaponInputChoiceIntentKind Kind =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentKind::Invalid;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	FVector2D ArcTargetIntent = FVector2D::ZeroVector;
	double ArcApexAdjustmentDelta = 0.0;
};

enum class Edemo_mapShanmenThrownWeaponInputChoiceIntentStatus : uint8
{
	Invalid,
	IntentInvalid,
	ChoiceSourceUnavailable,
	ChoiceStateInvalid,
	CommandCaptureRejected,
	Delegated,
	ControllerProtocolRejected
};

/** Immutable evidence for one logical-intent-to-controller decision. */
class Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasRejectedByController() const;
	Edemo_mapShanmenThrownWeaponInputChoiceIntentStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetChoiceSourceResolutionCount() const
	{
		return ChoiceSourceResolutionCount;
	}
	int32 GetChoiceStateReadCount() const { return ChoiceStateReadCount; }
	int32 GetCommandCaptureCount() const { return CommandCaptureCount; }
	int32 GetControllerRouteCount() const { return ControllerRouteCount; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& GetIntent() const
	{
		return Intent;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& GetCommand() const
	{
		return Command;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult&
	GetControllerResult() const
	{
		return ControllerResult;
	}

private:
	friend struct Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter;

	Edemo_mapShanmenThrownWeaponInputChoiceIntentStatus Status =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentStatus::Invalid;
	FString Diagnostic;
	int32 ChoiceSourceResolutionCount = 0;
	int32 ChoiceStateReadCount = 0;
	int32 CommandCaptureCount = 0;
	int32 ControllerRouteCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
	Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult ControllerResult;
};

/**
 * Stateless logical-input seam in front of the sole P20.18 controller route.
 *
 * One valid intent resolves and reads the authoritative choice once, freezes
 * that revision into one P20.10 command, then delegates once to P20.18. It
 * owns no key, UI, World, Actor, session, retry loop, or alternate state.
 */
struct Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter
{
	using FResolveChoiceSource = TFunctionRef<bool()>;
	using FReadChoiceState = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceState()>;
	using FRouteController = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult(
			const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&)>;

	static Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult Route(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent,
		FResolveChoiceSource ResolveChoiceSource,
		FReadChoiceState ReadChoiceState,
		FRouteController RouteController);
};
