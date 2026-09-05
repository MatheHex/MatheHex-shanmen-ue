#include "demo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenThrownWeaponProductLifecycle.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using EBridgeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus;
	using EProjectStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus;
	using EReduceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;

	bool IsPreviewChoice(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice)
	{
		return Choice.IsValid()
			&& Choice.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Choice.HasArcTargetIntent();
	}

	bool IsEmptyBridge(
		const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult& Bridge)
	{
		return Bridge.GetStatus() == EBridgeStatus::Invalid
			&& !Bridge.IsCaptured();
	}

	bool IsEmptyProjection(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult&
			Projection)
	{
		return Projection.GetStatus() == EProjectStatus::Invalid
			&& !Projection.IsProjected();
	}

	bool IsEmptyReduction(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult&
			Reduction)
	{
		return Reduction.GetStatus() == EReduceStatus::Invalid
			&& !Reduction.IsApplied()
			&& !Reduction.IsDuplicate();
	}

	bool HasValidPrevious(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State)
	{
		return State.IsEmpty() || State.IsValid();
	}

	bool IsRejectedReduction(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult&
			Reduction)
	{
		return Reduction.GetStatus() != EReduceStatus::Invalid
			&& !Reduction.IsApplied()
			&& !Reduction.IsDuplicate();
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult::IsValid() const
{
	if (Status == EStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| CaptureCount < 0 || CaptureCount > 1
		|| ProjectCount < 0 || ProjectCount > 1
		|| ReduceCount < 0 || ReduceCount > 1)
	{
		return false;
	}

	const bool bChoiceValid = ChoiceState.IsValid();
	const bool bPreviewChoice = IsPreviewChoice(ChoiceState);
	const bool bPreviousUsable = HasValidPrevious(PreviousState);
	const bool bNoBridge = IsEmptyBridge(Bridge);
	const bool bNoProjection = IsEmptyProjection(Projection);
	const bool bNoReduction = IsEmptyReduction(Reduction);

	switch (Status)
	{
	case EStatus::ChoiceRejected:
		return !bChoiceValid
			&& CaptureCount == 0 && ProjectCount == 0 && ReduceCount == 0
			&& bNoBridge && bNoProjection && bNoReduction
			&& State.IsEmpty();

	case EStatus::PreviousStateRejected:
		return bChoiceValid
			&& !PreviousState.IsEmpty() && !PreviousState.IsValid()
			&& CaptureCount == 0 && ProjectCount == 0 && ReduceCount == 0
			&& bNoBridge && bNoProjection && bNoReduction
			&& State.IsEmpty();

	case EStatus::NoPresentationRequired:
		return bChoiceValid && !bPreviewChoice && PreviousState.IsEmpty()
			&& CaptureCount == 0 && ProjectCount == 0 && ReduceCount == 0
			&& bNoBridge && bNoProjection && bNoReduction
			&& State.IsEmpty();

	case EStatus::CaptureRejected:
		return bPreviewChoice && bPreviousUsable
			&& CaptureCount == 1 && ProjectCount == 0 && ReduceCount == 0
			&& Bridge.GetStatus() != EBridgeStatus::Invalid
			&& !Bridge.IsCaptured()
			&& bNoProjection && bNoReduction && State.IsEmpty();

	case EStatus::ProjectRejected:
		return bPreviewChoice && bPreviousUsable
			&& CaptureCount == 1 && ProjectCount == 1 && ReduceCount == 0
			&& Bridge.IsCaptured()
			&& Projection.GetStatus() != EProjectStatus::Invalid
			&& !Projection.IsProjected()
			&& bNoReduction && State.IsEmpty();

	case EStatus::ReduceRejected:
		if (!bChoiceValid || !bPreviousUsable
			|| ReduceCount != 1
			|| !IsRejectedReduction(Reduction)
			|| !State.IsEmpty())
		{
			return false;
		}
		return bPreviewChoice
			? CaptureCount == 1 && ProjectCount == 1
				&& Bridge.IsCaptured() && Projection.IsProjected()
			: CaptureCount == 0 && ProjectCount == 0
				&& PreviousState.IsValid()
				&& bNoBridge && bNoProjection;

	case EStatus::Replaced:
		return bPreviewChoice && bPreviousUsable
			&& CaptureCount == 1 && ProjectCount == 1 && ReduceCount == 1
			&& Bridge.IsCaptured() && Projection.IsProjected()
			&& Reduction.IsReplaced() && State.IsVisible()
			&& State.Matches(Reduction.GetState());

	case EStatus::Cleared:
		return bChoiceValid && !bPreviewChoice && PreviousState.IsValid()
			&& CaptureCount == 0 && ProjectCount == 0 && ReduceCount == 1
			&& bNoBridge && bNoProjection
			&& Reduction.IsCleared() && State.IsHidden()
			&& State.Matches(Reduction.GetState());

	case EStatus::Duplicate:
		if (!bChoiceValid || !bPreviousUsable
			|| ReduceCount != 1
			|| !Reduction.IsDuplicate()
			|| !State.IsValid()
			|| !State.Matches(Reduction.GetState()))
		{
			return false;
		}
		return bPreviewChoice
			? CaptureCount == 1 && ProjectCount == 1
				&& Bridge.IsCaptured() && Projection.IsProjected()
			: CaptureCount == 0 && ProjectCount == 0
				&& PreviousState.IsValid()
				&& bNoBridge && bNoProjection;

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult::IsCompleted() const
{
	return IsValid()
		&& (Status == EStatus::NoPresentationRequired
			|| Status == EStatus::Replaced
			|| Status == EStatus::Cleared
			|| Status == EStatus::Duplicate);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult::DidChange() const
{
	return IsCompleted()
		&& (Status == EStatus::Replaced || Status == EStatus::Cleared);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult::IsNoChange() const
{
	return IsCompleted()
		&& (Status == EStatus::NoPresentationRequired
			|| Status == EStatus::Duplicate);
}

Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult
Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
	const int32 HotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const int32 SegmentCount,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& PreviousState,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult Result;
	Result.ChoiceState = CurrentChoice;
	Result.PreviousState = PreviousState;
	if (!CurrentChoice.IsValid())
	{
		Result.Status = EStatus::ChoiceRejected;
		Result.Diagnostic =
			TEXT("Arc preview update requires one valid current choice.");
		return Result;
	}
	if (!HasValidPrevious(PreviousState))
	{
		Result.Status = EStatus::PreviousStateRejected;
		Result.Diagnostic =
			TEXT("Arc preview update rejects an invalid consumer state.");
		return Result;
	}

	if (!IsPreviewChoice(CurrentChoice))
	{
		if (PreviousState.IsEmpty())
		{
			Result.Status = EStatus::NoPresentationRequired;
			Result.Diagnostic =
				TEXT("Current choice requires no Arc preview presentation.");
			return Result;
		}

		Result.ReduceCount = 1;
		Result.Reduction =
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
				PreviousState, CurrentChoice);
		if (Result.Reduction.IsCleared())
		{
			Result.Status = EStatus::Cleared;
			Result.State = Result.Reduction.GetState();
		}
		else if (Result.Reduction.IsDuplicate())
		{
			Result.Status = EStatus::Duplicate;
			Result.State = Result.Reduction.GetState();
		}
		else
		{
			Result.Status = EStatus::ReduceRejected;
		}
		Result.Diagnostic = Result.Reduction.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview clear reduction failed.")
			: Result.Reduction.GetDiagnostic();
		return Result;
	}

	Result.CaptureCount = 1;
	Result.Bridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			HotbarSlotNumber,
			CurrentChoice,
			ChoicePolicy,
			SegmentCount,
			Lifecycle,
			Coordinator);
	if (!Result.Bridge.IsCaptured())
	{
		Result.Status = EStatus::CaptureRejected;
		Result.Diagnostic = Result.Bridge.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview live product capture failed.")
			: Result.Bridge.GetDiagnostic();
		return Result;
	}

	Result.ProjectCount = 1;
	Result.Projection =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			Result.Bridge, SourceBasis);
	if (!Result.Projection.IsProjected())
	{
		Result.Status = EStatus::ProjectRejected;
		Result.Diagnostic = Result.Projection.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview presentation projection failed.")
			: Result.Projection.GetDiagnostic();
		return Result;
	}

	Result.ReduceCount = 1;
	Result.Reduction =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			PreviousState, Result.Projection.GetState());
	if (Result.Reduction.IsReplaced())
	{
		Result.Status = EStatus::Replaced;
		Result.State = Result.Reduction.GetState();
	}
	else if (Result.Reduction.IsDuplicate())
	{
		Result.Status = EStatus::Duplicate;
		Result.State = Result.Reduction.GetState();
	}
	else
	{
		Result.Status = EStatus::ReduceRejected;
	}
	Result.Diagnostic = Result.Reduction.GetDiagnostic().IsEmpty()
		? TEXT("Arc preview replacement reduction failed.")
		: Result.Reduction.GetDiagnostic();
	return Result;
}
