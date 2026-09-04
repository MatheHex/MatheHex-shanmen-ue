#include "demo_mapShanmenThrownWeaponInputChoiceControllerAdapter.h"

namespace
{
	using EControllerStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus;
	using ESessionStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus;
	using EReduceStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus;

	bool HasNoSessionEvidence(
		const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult& Result)
	{
		return Result.Status == ESessionStatus::ReductionRejected
			&& Result.ReduceStatus == EReduceStatus::StateInvalid
			&& Result.Diagnostic.IsEmpty()
			&& !Result.State.IsValid();
	}

	bool IsKnownReductionRejection(const EReduceStatus Status)
	{
		return Status == EReduceStatus::RevisionMismatch
			|| Status == EReduceStatus::RevisionExhausted
			|| Status == EReduceStatus::ModeMismatch
			|| Status == EReduceStatus::StateRejected;
	}

	bool IsSessionResultConsistent(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command,
		const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult& Result)
	{
		if (!Command.IsValid() || Result.Diagnostic.IsEmpty())
		{
			return false;
		}

		switch (Result.Status)
		{
		case ESessionStatus::Applied:
		case ESessionStatus::Replay:
		{
			const EReduceStatus ExpectedReduceStatus =
				Result.Status == ESessionStatus::Applied
					? EReduceStatus::Reduced
					: EReduceStatus::Replay;
			if (Result.ReduceStatus != ExpectedReduceStatus
				|| !Result.State.IsValid()
				|| Command.GetExpectedRevision() >= MAX_uint64 - 1
				|| Result.State.GetRevision()
					!= Command.GetExpectedRevision() + 1
				|| Result.State.GetLastCommandId() != Command.GetCommandId())
			{
				return false;
			}
			const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Replay =
				Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
					Result.State, Command);
			return Replay.Status == EReduceStatus::Replay
				&& Replay.State.Matches(Result.State);
		}

		case ESessionStatus::NoChange:
		{
			if (Result.ReduceStatus != EReduceStatus::NoChange
				|| !Result.State.IsValid()
				|| Result.State.GetRevision()
					!= Command.GetExpectedRevision())
			{
				return false;
			}
			const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult NoChange =
				Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
					Result.State, Command);
			return NoChange.Status == EReduceStatus::NoChange
				&& NoChange.State.Matches(Result.State);
		}

		case ESessionStatus::CombatRunActive:
		case ESessionStatus::ProductLifecycleNotEmpty:
			return Result.ReduceStatus == EReduceStatus::Reduced
				&& !Result.State.IsValid();

		case ESessionStatus::ReductionRejected:
			return IsKnownReductionRejection(Result.ReduceStatus)
				&& !Result.State.IsValid();

		default:
			return false;
		}
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult::IsValid() const
{
	if (Status == EControllerStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == EControllerStatus::CommandInvalid)
	{
		return !Command.IsValid()
			&& ChoiceSessionResolutionCount == 0
			&& ChoiceSessionSubmissionCount == 0
			&& HasNoSessionEvidence(SessionResult);
	}
	if (!Command.IsValid())
	{
		return false;
	}

	switch (Status)
	{
	case EControllerStatus::GameplayBlocked:
	case EControllerStatus::InputSurfaceBlocked:
	case EControllerStatus::InputModeBlocked:
		return ChoiceSessionResolutionCount == 0
			&& ChoiceSessionSubmissionCount == 0
			&& HasNoSessionEvidence(SessionResult);

	case EControllerStatus::ChoiceSessionUnavailable:
		return ChoiceSessionResolutionCount == 1
			&& ChoiceSessionSubmissionCount == 0
			&& HasNoSessionEvidence(SessionResult);

	case EControllerStatus::Delegated:
		return ChoiceSessionResolutionCount == 1
			&& ChoiceSessionSubmissionCount == 1
			&& IsSessionResultConsistent(Command, SessionResult)
			&& Diagnostic == SessionResult.Diagnostic;

	case EControllerStatus::SessionProtocolRejected:
		return ChoiceSessionResolutionCount == 1
			&& ChoiceSessionSubmissionCount == 1
			&& !IsSessionResultConsistent(Command, SessionResult);

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult::IsAccepted() const
{
	return IsValid()
		&& Status == EControllerStatus::Delegated
		&& SessionResult.IsSuccess();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult::
WasRejectedBySession() const
{
	return IsValid()
		&& Status == EControllerStatus::Delegated
		&& !SessionResult.IsSuccess();
}

Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult
Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter::Route(
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command,
	const bool bGameplayInputAllowed,
	const bool bGameplaySurface,
	const bool bGameOnlyInputMode,
	FResolveChoiceSession ResolveChoiceSession,
	FSubmitChoiceCommand SubmitChoiceCommand)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult Result;
	Result.Command = Command;
	if (!Command.IsValid())
	{
		Result.Status = EControllerStatus::CommandInvalid;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit command is invalid.");
		return Result;
	}
	if (!bGameplayInputAllowed)
	{
		Result.Status = EControllerStatus::GameplayBlocked;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit is blocked by gameplay state.");
		return Result;
	}
	if (!bGameplaySurface)
	{
		Result.Status = EControllerStatus::InputSurfaceBlocked;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit requires the Gameplay surface.");
		return Result;
	}
	if (!bGameOnlyInputMode)
	{
		Result.Status = EControllerStatus::InputModeBlocked;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit requires GameOnly input mode.");
		return Result;
	}

	Result.ChoiceSessionResolutionCount = 1;
	if (!ResolveChoiceSession())
	{
		Result.Status = EControllerStatus::ChoiceSessionUnavailable;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit requires the authoritative GameMode session.");
		return Result;
	}

	Result.ChoiceSessionSubmissionCount = 1;
	Result.SessionResult = SubmitChoiceCommand(Command);
	if (!IsSessionResultConsistent(Command, Result.SessionResult))
	{
		Result.Status = EControllerStatus::SessionProtocolRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice edit rejected invalid session evidence.");
		return Result;
	}

	Result.Status = EControllerStatus::Delegated;
	Result.Diagnostic = Result.SessionResult.Diagnostic;
	return Result;
}
