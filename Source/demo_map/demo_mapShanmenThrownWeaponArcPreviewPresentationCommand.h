#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSession.h"

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind
	: uint8
{
	Invalid,
	Show,
	Replace,
	Hide,
	NoOp
};

/**
 * Immutable renderer-neutral delta for one accepted presentation update.
 *
 * Show and Replace carry a visible State payload. Hide carries the previous
 * visible State and the resulting non-visible State. NoOp records an accepted
 * update that requires no renderer mutation. The command owns no renderer,
 * widget, component, Actor, World, timer, input or product authority.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Other)
		const;
	bool RequiresRenderMutation() const;
	bool IsShow() const;
	bool IsReplace() const;
	bool IsHide() const;
	bool IsNoOp() const;

	const FGuid& GetCommandId() const { return CommandId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind GetKind()
		const
	{
		return Kind;
	}
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousState() const
	{
		return PreviousState;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector;

	FGuid CommandId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind Kind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind::Invalid;
	FGuid RunId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus
	: uint8
{
	Invalid,
	SessionResultInvalid,
	SessionUpdateRejected,
	TransitionRejected,
	CommandRejected,
	Projected
};

/** Self-validating audit result for one session-result projection. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
{
public:
	bool IsValid() const;
	bool IsProjected() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
	GetSessionResult() const
	{
		return SessionResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& GetCommand()
		const
	{
		return Command;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
		SessionResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand Command;
};

/** Pure accepted-session-result to renderer-neutral command projector. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector
{
public:
	static
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
	Project(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
			SessionResult);

private:
	static
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
	Reject(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus
			Status,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
			SessionResult,
		const TCHAR* Diagnostic);
};
