#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceSession.h"

enum class Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus : uint8
{
	Invalid,
	CommandInvalid,
	GameplayBlocked,
	InputSurfaceBlocked,
	InputModeBlocked,
	ChoiceSessionUnavailable,
	Delegated,
	SessionProtocolRejected
};

/** Immutable audit evidence for one device-independent choice edit. */
class Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasRejectedBySession() const;
	Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetChoiceSessionResolutionCount() const
	{
		return ChoiceSessionResolutionCount;
	}
	int32 GetChoiceSessionSubmissionCount() const
	{
		return ChoiceSessionSubmissionCount;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& GetCommand() const
	{
		return Command;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult&
	GetSessionResult() const
	{
		return SessionResult;
	}

private:
	friend struct Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter;

	Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus Status =
		Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus::Invalid;
	FString Diagnostic;
	int32 ChoiceSessionResolutionCount = 0;
	int32 ChoiceSessionSubmissionCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
	Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult SessionResult;
};

/**
 * Stateless PlayerController seam in front of the sole P20.11 choice session.
 *
 * It validates one frozen P20.10 command, checks the existing gameplay surface
 * and input mode, resolves the authoritative GameMode once, then submits once.
 * The session remains the only reducer/state owner. This adapter owns no key,
 * UI, World, Actor, Run, lifecycle, retry loop, or alternate choice state.
 */
struct Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter
{
	using FResolveChoiceSession = TFunctionRef<bool()>;
	using FSubmitChoiceCommand = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult(
			const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&)>;

	static Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult Route(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command,
		bool bGameplayInputAllowed,
		bool bGameplaySurface,
		bool bGameOnlyInputMode,
		FResolveChoiceSession ResolveChoiceSession,
		FSubmitChoiceCommand SubmitChoiceCommand);
};
