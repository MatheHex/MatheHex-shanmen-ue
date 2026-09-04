#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcSourceBasisAdapter.h"

/**
 * Immutable, device-independent request to launch the currently selected Arc.
 *
 * The command freezes the consumer-owned choice and projection policy at the
 * input boundary. It owns no key, UI, Actor, World, Run, item, or product
 * state. Equal canonical inputs reproduce the same command identity.
 */
class Fdemo_mapShanmenThrownWeaponArcLaunchCommand
{
public:
	static constexpr int32 MaximumHotbarSlotNumber = 9;

	static bool TryCapture(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy,
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand& OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Other) const;
	const FGuid& GetCommandId() const { return CommandId; }
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetPolicy() const
	{
		return Policy;
	}

private:
	FGuid CommandId;
	int32 HotbarSlotNumber = INDEX_NONE;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
};

enum class Edemo_mapShanmenThrownWeaponArcLaunchInputStatus : uint8
{
	Invalid,
	CommandInvalid,
	GameplayBlocked,
	InputSurfaceBlocked,
	InputModeBlocked,
	ProductRouteUnavailable,
	ChoiceStateInvalid,
	ChoiceStateMismatch,
	Delegated,
	ProductProtocolRejected
};

/** Immutable audit evidence for one Arc launch command boundary decision. */
class Fdemo_mapShanmenThrownWeaponArcLaunchInputResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	Edemo_mapShanmenThrownWeaponArcLaunchInputStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetProductRouteResolutionCount() const
	{
		return ProductRouteResolutionCount;
	}
	int32 GetChoiceStateReadCount() const { return ChoiceStateReadCount; }
	int32 GetProductRouteInvocationCount() const
	{
		return ProductRouteInvocationCount;
	}
	const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& GetCommand() const
	{
		return Command;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceState&
	GetCurrentChoiceState() const
	{
		return CurrentChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult&
	GetProductRoute() const
	{
		return ProductRoute;
	}

private:
	friend struct Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter;

	Edemo_mapShanmenThrownWeaponArcLaunchInputStatus Status =
		Edemo_mapShanmenThrownWeaponArcLaunchInputStatus::Invalid;
	FString Diagnostic;
	int32 ProductRouteResolutionCount = 0;
	int32 ChoiceStateReadCount = 0;
	int32 ProductRouteInvocationCount = 0;
	Fdemo_mapShanmenThrownWeaponArcLaunchCommand Command;
	Fdemo_mapShanmenThrownWeaponInputChoiceState CurrentChoiceState;
	Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult ProductRoute;
};

/**
 * Stateless command-to-product seam for a future Arc launch interaction.
 *
 * It checks the existing gameplay surface and input mode before resolving the
 * authoritative GameMode route. One eligible command reads the current choice
 * once, rejects a stale snapshot, and delegates exactly once to P20.14. It
 * owns no physical binding and never samples source geometry itself.
 */
struct Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter
{
	using FResolveProductRoute = TFunctionRef<bool()>;
	using FReadCurrentChoiceState = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceState()>;
	using FRouteProduct = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult(
			int32,
			const Fdemo_mapShanmenThrownWeaponArcChoicePolicy&)>;

	static Fdemo_mapShanmenThrownWeaponArcLaunchInputResult Route(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command,
		bool bGameplayInputAllowed,
		bool bGameplaySurface,
		bool bGameOnlyInputMode,
		FResolveProductRoute ResolveProductRoute,
		FReadCurrentChoiceState ReadCurrentChoiceState,
		FRouteProduct RouteProduct);
};
