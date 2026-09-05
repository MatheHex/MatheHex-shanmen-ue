#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSession.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenThrownWeaponProductLifecycle.h"

namespace
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	using EUpdate = Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	bool IsPreviewChoice(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice)
	{
		return Choice.IsValid()
			&& Choice.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Choice.HasArcTargetIntent();
	}

	bool StatesMatchOrAreEmpty(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Left,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty())
			|| Left.Matches(Right);
	}

	bool StateBelongsToRun(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State,
		const FGuid& RunId)
	{
		return State.IsEmpty()
			|| (RunId.IsValid()
				&& State.IsValid()
				&& State.GetRunId() == RunId);
	}

	bool IsEmptyUpdate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult& Update)
	{
		return Update.GetStatus() == EUpdate::Invalid
			&& Update.GetDiagnostic().IsEmpty()
			&& Update.GetCaptureCount() == 0
			&& Update.GetProjectCount() == 0
			&& Update.GetReduceCount() == 0
			&& Update.GetState().IsEmpty();
	}

}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession::
	RejectBeforeUpdate(
		const ESession Status,
		const FGuid& SessionRunId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State,
		const TCHAR* Diagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SessionRunId = SessionRunId;
	Result.PreviousState = State;
	Result.State = State;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult::
	IsValid() const
{
	if (Status == ESession::Invalid
		|| Diagnostic.IsEmpty()
		|| CoordinatorCallCount < 0
		|| CoordinatorCallCount > 1
		|| (!PreviousState.IsEmpty() && !PreviousState.IsValid())
		|| (!State.IsEmpty() && !State.IsValid()))
	{
		return false;
	}

	const bool bNoUpdate = IsEmptyUpdate(Update);
	const bool bStateUnchanged =
		StatesMatchOrAreEmpty(PreviousState, State);
	const bool bPreviousInScope =
		StateBelongsToRun(PreviousState, SessionRunId);
	const bool bStateInScope = StateBelongsToRun(State, SessionRunId);

	switch (Status)
	{
	case ESession::SessionInactive:
		return !SessionRunId.IsValid()
			&& PreviousState.IsEmpty()
			&& State.IsEmpty()
			&& CoordinatorCallCount == 0
			&& bNoUpdate;

	case ESession::SessionInvalid:
		return CoordinatorCallCount == 0
			&& bNoUpdate
			&& bStateUnchanged;

	case ESession::ProductUnavailable:
	case ESession::RunMismatch:
		return SessionRunId.IsValid()
			&& bPreviousInScope
			&& bStateInScope
			&& bStateUnchanged
			&& CoordinatorCallCount == 0
			&& bNoUpdate;

	case ESession::UpdateRejected:
		return SessionRunId.IsValid()
			&& bPreviousInScope
			&& bStateInScope
			&& bStateUnchanged
			&& CoordinatorCallCount == 1
			&& Update.IsValid()
			&& !Update.IsCompleted()
			&& StatesMatchOrAreEmpty(
				PreviousState, Update.GetPreviousState());

	case ESession::StateRejected:
		return SessionRunId.IsValid()
			&& bPreviousInScope
			&& bStateInScope
			&& bStateUnchanged
			&& CoordinatorCallCount == 1
			&& Update.IsCompleted()
			&& !StateBelongsToRun(Update.GetState(), SessionRunId)
			&& StatesMatchOrAreEmpty(
				PreviousState, Update.GetPreviousState());

	case ESession::Applied:
		return SessionRunId.IsValid()
			&& bPreviousInScope
			&& bStateInScope
			&& CoordinatorCallCount == 1
			&& Update.IsCompleted()
			&& Update.DidChange()
			&& StatesMatchOrAreEmpty(
				PreviousState, Update.GetPreviousState())
			&& StatesMatchOrAreEmpty(State, Update.GetState());

	case ESession::NoChange:
		return SessionRunId.IsValid()
			&& bPreviousInScope
			&& bStateInScope
			&& CoordinatorCallCount == 1
			&& Update.IsCompleted()
			&& Update.IsNoChange()
			&& StatesMatchOrAreEmpty(
				PreviousState, Update.GetPreviousState())
			&& StatesMatchOrAreEmpty(State, Update.GetState());

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult::
	IsAccepted() const
{
	return IsValid()
		&& (Status == ESession::Applied || Status == ESession::NoChange);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult::
	DidChange() const
{
	return IsValid() && Status == ESession::Applied;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult::
	IsNoChange() const
{
	return IsValid() && Status == ESession::NoChange;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !RequestedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Arc preview presentation Session requires valid empty state and Run identity.");
		return false;
	}
	if (IsActive())
	{
		if (RunId == RequestedRunId)
		{
			OutDiagnostic =
				TEXT("Arc preview presentation Session is already bound to this Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("Arc preview presentation Session rejects a second active Run.");
		return false;
	}

	RunId = RequestedRunId;
	if (!IsValid())
	{
		Reset();
		OutDiagnostic =
			TEXT("Arc preview presentation Session failed closed during Run binding.");
		return false;
	}
	OutDiagnostic = TEXT("Arc preview presentation Session bound to one Run.");
	return true;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession::TryUpdate(
	const int32 HotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const int32 SegmentCount,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator)
{
	if (!IsValid())
	{
		return RejectBeforeUpdate(
			ESession::SessionInvalid,
			RunId,
			State,
			TEXT("Arc preview presentation Session invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeUpdate(
			ESession::SessionInactive,
			RunId,
			State,
			TEXT("Arc preview presentation update requires one active Run scope."));
	}

	if (IsPreviewChoice(CurrentChoice))
	{
		if (!Lifecycle.IsActive()
			|| !Lifecycle.IsValid()
			|| !Coordinator.IsReady())
		{
			return RejectBeforeUpdate(
				ESession::ProductUnavailable,
				RunId,
				State,
				TEXT("Visible Arc preview requires active read-only product and Run sources."));
		}
		if (Lifecycle.GetRunId() != RunId
			|| Coordinator.GetRunId() != RunId)
		{
			return RejectBeforeUpdate(
				ESession::RunMismatch,
				RunId,
				State,
				TEXT("Arc preview product sources do not match the Session Run."));
		}
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult Result;
	Result.SessionRunId = RunId;
	Result.PreviousState = State;
	Result.State = State;
	Result.CoordinatorCallCount = 1;
	Result.Update =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			HotbarSlotNumber,
			CurrentChoice,
			ChoicePolicy,
			SegmentCount,
			SourceBasis,
			State,
			Lifecycle,
			Coordinator);
	if (!Result.Update.IsCompleted())
	{
		Result.Status = ESession::UpdateRejected;
		Result.Diagnostic = Result.Update.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview presentation update failed closed.")
			: Result.Update.GetDiagnostic();
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Candidate =
		*this;
	Candidate.State = Result.Update.GetState();
	if (!Candidate.IsValid())
	{
		Result.Status = ESession::StateRejected;
		Result.Diagnostic =
			TEXT("Arc preview presentation Session rejected an out-of-scope result.");
		return Result;
	}

	*this = Candidate;
	Result.State = State;
	Result.Status = Result.Update.DidChange()
		? ESession::Applied
		: ESession::NoChange;
	Result.Diagnostic = Result.Update.GetDiagnostic();
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Arc preview presentation Session end requires valid state and Run identity.");
		return false;
	}
	if (!IsActive())
	{
		OutDiagnostic =
			TEXT("Arc preview presentation Session is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Arc preview presentation Session rejects mismatched Run teardown.");
		return false;
	}

	Reset();
	OutDiagnostic =
		TEXT("Arc preview presentation Session ended and discarded its Run state.");
	return true;
}

void Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession::Reset()
{
	*this = Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession::IsValid()
	const
{
	if (!RunId.IsValid())
	{
		return State.IsEmpty();
	}
	return State.IsEmpty()
		|| (State.IsValid() && State.GetRunId() == RunId);
}
