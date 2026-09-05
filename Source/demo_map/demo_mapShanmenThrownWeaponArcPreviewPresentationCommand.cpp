#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommand.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	using EProject =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownCommand(const ECommand Kind)
	{
		return Kind == ECommand::Show
			|| Kind == ECommand::Replace
			|| Kind == ECommand::Hide
			|| Kind == ECommand::NoOp;
	}

	bool StatesMatchOrAreEmpty(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Left,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool StateBelongsToRun(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State,
		const FGuid& RunId)
	{
		return State.IsEmpty()
			|| (State.IsValid() && State.GetRunId() == RunId);
	}

	bool SameScope(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Left,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetPlayerEntityId() == Right.GetPlayerEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId();
	}

	bool TryClassifyTransition(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Previous,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State,
		ECommand& OutKind)
	{
		OutKind = ECommand::Invalid;
		if ((!Previous.IsEmpty() && !Previous.IsValid())
			|| (!State.IsEmpty() && !State.IsValid()))
		{
			return false;
		}

		if (Previous.IsVisible())
		{
			if (State.IsVisible())
			{
				OutKind = Previous.Matches(State)
					? ECommand::NoOp
					: ECommand::Replace;
			}
			else
			{
				OutKind = ECommand::Hide;
			}
			return true;
		}

		OutKind = State.IsVisible() ? ECommand::Show : ECommand::NoOp;
		return true;
	}

	FGuid MakeCommandId(
		const ECommand Kind,
		const FGuid& RunId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Previous,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State)
	{
		if (!IsKnownCommand(Kind) || !RunId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationCommand.r1"),
			{
				FString::FromInt(static_cast<int32>(Kind)),
				GuidDigits(RunId),
				Previous.IsEmpty()
					? TEXT("EMPTY")
					: GuidDigits(Previous.GetPresentationStateId()),
				State.IsEmpty()
					? TEXT("EMPTY")
					: GuidDigits(State.GetPresentationStateId())
			});
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::IsValid()
	const
{
	if (!CommandId.IsValid()
		|| !IsKnownCommand(Kind)
		|| !RunId.IsValid()
		|| !StateBelongsToRun(PreviousState, RunId)
		|| !StateBelongsToRun(State, RunId))
	{
		return false;
	}
	if (PreviousState.IsValid() && State.IsValid())
	{
		if (!SameScope(PreviousState, State))
		{
			return false;
		}
		if (!PreviousState.Matches(State)
			&& State.GetChoiceRevision()
				<= PreviousState.GetChoiceRevision())
		{
			return false;
		}
	}

	ECommand Expected = ECommand::Invalid;
	return TryClassifyTransition(PreviousState, State, Expected)
		&& Kind == Expected
		&& CommandId == MakeCommandId(
			Kind, RunId, PreviousState, State);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& CommandId == Other.CommandId
		&& Kind == Other.Kind
		&& RunId == Other.RunId
		&& StatesMatchOrAreEmpty(PreviousState, Other.PreviousState)
		&& StatesMatchOrAreEmpty(State, Other.State);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::
	RequiresRenderMutation() const
{
	return IsValid() && Kind != ECommand::NoOp;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::IsShow() const
{
	return IsValid() && Kind == ECommand::Show && State.IsVisible();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::IsReplace()
	const
{
	return IsValid() && Kind == ECommand::Replace
		&& PreviousState.IsVisible() && State.IsVisible();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::IsHide() const
{
	return IsValid() && Kind == ECommand::Hide
		&& PreviousState.IsVisible() && !State.IsVisible();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand::IsNoOp() const
{
	return IsValid() && Kind == ECommand::NoOp;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult::
	IsValid() const
{
	if (Status == EProject::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}

	switch (Status)
	{
	case EProject::SessionResultInvalid:
		return !SessionResult.IsValid() && !Command.IsValid();

	case EProject::SessionUpdateRejected:
		return SessionResult.IsValid()
			&& !SessionResult.IsAccepted()
			&& !Command.IsValid();

	case EProject::TransitionRejected:
	case EProject::CommandRejected:
		return SessionResult.IsAccepted() && !Command.IsValid();

	case EProject::Projected:
		return SessionResult.IsAccepted()
			&& Command.IsValid()
			&& Command.GetRunId() == SessionResult.GetSessionRunId()
			&& StatesMatchOrAreEmpty(
				Command.GetPreviousState(),
				SessionResult.GetPreviousState())
			&& StatesMatchOrAreEmpty(
				Command.GetState(), SessionResult.GetState());

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult::
	IsProjected() const
{
	return IsValid() && Status == EProject::Projected;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::Reject(
	const EProject Status,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
		SessionResult,
	const TCHAR* Diagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
		Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SessionResult = SessionResult;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::Project(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
		SessionResult)
{
	if (!SessionResult.IsValid())
	{
		return Reject(
			EProject::SessionResultInvalid,
			SessionResult,
			TEXT("Arc preview presentation command requires a valid Session result."));
	}
	if (!SessionResult.IsAccepted())
	{
		return Reject(
			EProject::SessionUpdateRejected,
			SessionResult,
			TEXT("Rejected Arc preview Session updates cannot emit renderer commands."));
	}

	ECommand Kind = ECommand::Invalid;
	if (!TryClassifyTransition(
			SessionResult.GetPreviousState(),
			SessionResult.GetState(),
			Kind))
	{
		return Reject(
			EProject::TransitionRejected,
			SessionResult,
			TEXT("Arc preview Session states do not describe a visual transition."));
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
		Result;
	Result.Status = EProject::Projected;
	Result.SessionResult = SessionResult;
	Result.Command.Kind = Kind;
	Result.Command.RunId = SessionResult.GetSessionRunId();
	Result.Command.PreviousState = SessionResult.GetPreviousState();
	Result.Command.State = SessionResult.GetState();
	Result.Command.CommandId = MakeCommandId(
		Kind,
		Result.Command.RunId,
		Result.Command.PreviousState,
		Result.Command.State);
	if (!Result.Command.IsValid())
	{
		return Reject(
			EProject::CommandRejected,
			SessionResult,
			TEXT("Projected Arc preview presentation command failed validation."));
	}

	switch (Kind)
	{
	case ECommand::Show:
		Result.Diagnostic =
			TEXT("Projected one Show command for a visible Arc preview.");
		break;
	case ECommand::Replace:
		Result.Diagnostic =
			TEXT("Projected one Replace command for newer Arc geometry.");
		break;
	case ECommand::Hide:
		Result.Diagnostic =
			TEXT("Projected one Hide command for a cleared Arc preview.");
		break;
	case ECommand::NoOp:
		Result.Diagnostic =
			TEXT("Projected one NoOp because the renderer-visible state is unchanged.");
		break;
	default:
		return Reject(
			EProject::TransitionRejected,
			SessionResult,
			TEXT("Arc preview presentation transition has no command mapping."));
	}
	return Result;
}
