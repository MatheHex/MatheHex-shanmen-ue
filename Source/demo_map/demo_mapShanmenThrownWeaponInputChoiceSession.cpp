#include "demo_mapShanmenThrownWeaponInputChoiceSession.h"

namespace
{
	bool IsLiveArcAimEdit(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		switch (Command.GetKind())
		{
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			SetArcTargetIntent:
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			AdjustArcApex:
		case Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
			ClearArcTargetIntent:
			return true;
		default:
			return false;
		}
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Accept(
		const Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus Status,
		const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult& Reduced)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Result;
		Result.Status = Status;
		Result.ReduceStatus = Reduced.Status;
		Result.Diagnostic = Reduced.Diagnostic;
		Result.State = Reduced.State;
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Reject(
		const Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus Status,
		const Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus ReduceStatus,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Result;
		Result.Status = Status;
		Result.ReduceStatus = ReduceStatus;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult::IsSuccess() const
{
	return (Status
			== Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::Applied
		|| Status
			== Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::NoChange
		|| Status
			== Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::Replay)
		&& State.IsValid();
}

Fdemo_mapShanmenThrownWeaponInputChoiceSession::
Fdemo_mapShanmenThrownWeaponInputChoiceSession()
	: State(Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial())
{
}

Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult
Fdemo_mapShanmenThrownWeaponInputChoiceSession::Submit(
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command,
	const bool bCombatRunActive,
	const bool bProductLifecycleEmpty)
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reduced =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(State, Command);
	if (!Reduced.IsSuccess())
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Result;
		Result.Status = Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::
			ReductionRejected;
		Result.ReduceStatus = Reduced.Status;
		Result.Diagnostic = Reduced.Diagnostic;
		return Result;
	}

	if (Reduced.Status
		== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Replay)
	{
		return Accept(
			Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::Replay,
			Reduced);
	}
	if (Reduced.Status
		== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::NoChange)
	{
		return Accept(
			Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::NoChange,
			Reduced);
	}
	const bool bLiveArcAimEdit = IsLiveArcAimEdit(Command);
	if (bCombatRunActive && !bLiveArcAimEdit)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::
				CombatRunActive,
			Reduced.Status,
			TEXT("Thrown-weapon input choice is frozen during an active combat Run."));
	}
	if (!bProductLifecycleEmpty && !bLiveArcAimEdit)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::
				ProductLifecycleNotEmpty,
			Reduced.Status,
			TEXT("Thrown-weapon input choice requires an empty product lifecycle."));
	}

	if (!Reduced.State.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::
				ReductionRejected,
			Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::StateRejected,
			TEXT("Thrown-weapon input choice session rejected invalid reduced state."));
	}
	State = Reduced.State;
	return Accept(
		Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::Applied,
		Reduced);
}
