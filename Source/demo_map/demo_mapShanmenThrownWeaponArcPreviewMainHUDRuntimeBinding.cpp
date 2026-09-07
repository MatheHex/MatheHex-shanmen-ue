#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding.h"

#include "demo_mapShanmenThrownWeaponHotbarConfirmationAdapter.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FChoice = Fdemo_mapShanmenThrownWeaponInputChoiceState;
	using FChoiceCommand = Fdemo_mapShanmenThrownWeaponInputChoiceCommand;
	using FChoiceReducer = Fdemo_mapShanmenThrownWeaponInputChoiceReducer;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using FSurface =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	using FTransitionRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest;
	using FUpdateResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	FState PhysicalCursorFor(const FState& State)
	{
		return State.IsVisible() ? State : FState();
	}

	Fdemo_mapShanmenThrownWeaponArcChoiceBasis MakeTeardownBasis()
	{
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
		const bool bCaptured =
			Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
				FVector::ZeroVector,
				FVector::ForwardVector,
				FVector::RightVector,
				Basis);
		check(bCaptured && Basis.IsValid());
		return Basis;
	}
}

bool FBinding::TryInitialize(
	const FGuid& FallbackSurfaceInstanceId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (bOperationInProgress || Owner.IsActive()
		|| AttachedHUDSurface != nullptr || BoundSurface != nullptr)
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD runtime binding initializes only from empty state.");
		return false;
	}
	if (!FallbackSurface.TryInitialize(
			FallbackSurfaceInstanceId, OutDiagnostic))
	{
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD runtime binding failed initialization invariants.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Arc preview MainHUD runtime binding initialized its permanent fallback surface.");
	return true;
}

bool FBinding::TryAttachHUD(FSurface& Surface, FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress || !Surface.IsValid()
		|| Surface.GetConsumerDefinitionId()
			!= FSurface::StableConsumerDefinitionId()
		|| Surface.GetSurfaceInstanceId()
			== FallbackSurface.GetSurfaceInstanceId())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD attach requires one distinct initialized surface and stable binding.");
		return false;
	}
	if (AttachedHUDSurface == &Surface)
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD surface is already attached.");
		return true;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	if (Owner.IsActive() && !TryMoveOwnerTo(Surface, OutDiagnostic))
	{
		return false;
	}
	AttachedHUDSurface = &Surface;
	if (!IsValid())
	{
		AttachedHUDSurface = nullptr;
		OutDiagnostic = TEXT(
			"Arc preview MainHUD attach failed post-operation invariants.");
		return false;
	}
	OutDiagnostic = Owner.IsActive()
		? TEXT("Arc preview ownership moved to the attached MainHUD surface.")
		: TEXT("Arc preview MainHUD surface attached for the next Run.");
	return true;
}

bool FBinding::TryDetachHUD(FSurface& Surface, FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress || !Surface.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD detach requires one stable initialized binding.");
		return false;
	}
	if (AttachedHUDSurface != &Surface)
	{
		OutDiagnostic = TEXT(
			"Arc preview ignored a stale MainHUD detach notification.");
		return true;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	if (Owner.IsActive() && BoundSurface == &Surface
		&& !TryMoveOwnerTo(FallbackSurface, OutDiagnostic))
	{
		return false;
	}
	AttachedHUDSurface = nullptr;
	if (!IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD detach failed post-operation invariants.");
		return false;
	}
	OutDiagnostic = Owner.IsActive()
		? TEXT("Arc preview ownership moved to the permanent fallback before HUD teardown.")
		: TEXT("Arc preview MainHUD detached while no Run was active.");
	return true;
}

bool FBinding::TryBeginRun(
	const FGuid& RunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress || !RunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD Run binding requires initialized stable state and Run identity.");
		return false;
	}
	if (Owner.IsActive())
	{
		if (Owner.GetRunId() == RunId)
		{
			OutDiagnostic = TEXT(
				"Arc preview MainHUD runtime binding already owns this Run.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Arc preview MainHUD runtime binding rejects a second active Run.");
		return false;
	}

	FSurface* const InitialSurface = AttachedHUDSurface != nullptr
		? AttachedHUDSurface
		: &FallbackSurface;
	if (!InitialSurface->GetSurfaceCursor().IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD Run binding requires an empty initial physical surface.");
		return false;
	}
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	if (!Owner.TryBegin(RunId, *InitialSurface, OutDiagnostic))
	{
		return false;
	}
	BoundSurface = InitialSurface;
	if (!IsValid())
	{
		FString EndDiagnostic;
		Owner.TryEnd(RunId, EndDiagnostic);
		BoundSurface = nullptr;
		OutDiagnostic = TEXT(
			"Arc preview MainHUD Run binding failed joint owner/surface invariants.");
		return false;
	}
	OutDiagnostic = InitialSurface == AttachedHUDSurface
		? TEXT("Arc preview Run bound directly to the attached MainHUD surface.")
		: TEXT("Arc preview Run bound to the permanent fallback surface.");
	return true;
}

bool FBinding::TryUpdate(
	const int32 HotbarSlotNumber,
	const FChoice& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FUpdateResult& OutResult,
	FString& OutDiagnostic)
{
	OutResult = FUpdateResult();
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress || !Owner.IsActive())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD update requires one active stable runtime binding.");
		return false;
	}
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	return TryApplyUpdate(
		HotbarSlotNumber,
		CurrentChoice,
		ChoicePolicy,
		SourceBasis,
		Lifecycle,
		Coordinator,
		OutResult,
		OutDiagnostic);
}

bool FBinding::TryClear(
	const FChoice& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FUpdateResult& OutResult,
	FString& OutDiagnostic)
{
	OutResult = FUpdateResult();
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress || !Owner.IsActive())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD clear requires one active stable runtime binding.");
		return false;
	}
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	return TryApplyClear(
		CurrentChoice,
		Lifecycle,
		Coordinator,
		OutResult,
		OutDiagnostic);
}

bool FBinding::TryApplyUpdate(
	const int32 HotbarSlotNumber,
	const FChoice& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FUpdateResult& OutResult,
	FString& OutDiagnostic)
{
	OutResult = Owner.TryUpdate(
		HotbarSlotNumber,
		CurrentChoice,
		ChoicePolicy,
		CanonicalSegmentCount,
		SourceBasis,
		Lifecycle,
		Coordinator);
	if (OutResult.IsAccepted())
	{
		OutDiagnostic = OutResult.GetDiagnostic();
		return IsValid();
	}
	if (OutResult.NeedsRecovery())
	{
		const auto Recovery = Owner.TryRecoverRejected();
		if (Recovery.IsAccepted() && IsValid())
		{
			OutDiagnostic = Recovery.GetDiagnostic();
			return true;
		}
		OutDiagnostic = Recovery.GetDiagnostic().IsEmpty()
			? OutResult.GetDiagnostic()
			: Recovery.GetDiagnostic();
		return false;
	}
	OutDiagnostic = OutResult.GetDiagnostic();
	return false;
}

bool FBinding::TryApplyClear(
	const FChoice& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FUpdateResult& OutResult,
	FString& OutDiagnostic)
{
	OutResult = FUpdateResult();
	if (!CurrentChoice.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD clear requires one valid caller-owned choice.");
		return false;
	}

	FChoice HiddenChoice = CurrentChoice;
	if (CurrentChoice.HasArcTargetIntent())
	{
		FChoiceCommand ClearCommand;
		if (!FChoiceCommand::TryCaptureArcTargetClear(
				CurrentChoice.GetRevision(), ClearCommand))
		{
			OutDiagnostic = TEXT(
				"Arc preview MainHUD clear could not capture its local clear command.");
			return false;
		}
		const auto ClearedChoice = FChoiceReducer::Reduce(
			CurrentChoice, ClearCommand);
		if (!ClearedChoice.DidChange())
		{
			OutDiagnostic = TEXT(
				"Arc preview MainHUD clear could not derive a local hidden choice.");
			return false;
		}
		HiddenChoice = ClearedChoice.State;
	}

	return TryApplyUpdate(
		1,
		HiddenChoice,
		Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::
			GetCanonical(),
		MakeTeardownBasis(),
		Lifecycle,
		Coordinator,
		OutResult,
		OutDiagnostic);
}

bool FBinding::TryEndRun(
	const FGuid& ExpectedRunId,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress || !ExpectedRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD Run teardown requires valid stable state and Run identity.");
		return false;
	}
	if (!Owner.IsActive())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD runtime binding is already outside a Run.");
		return true;
	}
	if (Owner.GetRunId() != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD runtime binding rejects mismatched Run teardown.");
		return false;
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	if (Owner.GetState().IsVisible())
	{
		FUpdateResult ClearResult;
		if (!TryApplyClear(
				Owner.GetState().GetChoiceState(),
				Lifecycle,
				Coordinator,
				ClearResult,
				OutDiagnostic))
		{
			return false;
		}
	}
	if (!Owner.TryEnd(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	BoundSurface = nullptr;
	if (!IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview MainHUD teardown failed empty-state invariants.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Arc preview MainHUD runtime binding cleared and ended its Run scope.");
	return true;
}

bool FBinding::TryMoveOwnerTo(
	FSurface& NewSurface,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Owner.IsActive() || BoundSurface == nullptr
		|| !BoundSurface->IsValid() || !NewSurface.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview surface handoff requires one active old and new physical surface.");
		return false;
	}
	if (BoundSurface == &NewSurface)
	{
		OutDiagnostic = TEXT(
			"Arc preview owner already targets this physical surface.");
		return true;
	}

	const FState AuthoritativeCursor = Owner.GetHostCursor();
	const FState ExpectedPhysicalCursor = PhysicalCursorFor(
		AuthoritativeCursor);
	const EAction Action = ExpectedPhysicalCursor.IsVisible()
		? EAction::AdoptExact
		: EAction::BindFresh;
	bool bRehydrated = false;
	if (Action == EAction::AdoptExact
		&& NewSurface.GetSurfaceCursor().IsEmpty())
	{
		if (!NewSurface.TryRehydrateVisibleForHandoff(
				Owner.GetRunId(),
				Owner.GetConsumerDefinitionId(),
				ExpectedPhysicalCursor,
				OutDiagnostic))
		{
			return false;
		}
		bRehydrated = true;
	}

	FTransitionRequest Request;
	if (!FTransitionRequest::TryCreate(
			Owner.GetRunId(),
			Owner.GetConsumerDefinitionId(),
			AuthoritativeCursor,
			NewSurface.GetSurfaceInstanceId(),
			Action,
			Request,
			OutDiagnostic))
	{
		if (bRehydrated)
		{
			FString RollbackDiagnostic;
			NewSurface.TryDiscardRehydratedVisibleForHandoff(
				ExpectedPhysicalCursor, RollbackDiagnostic);
		}
		return false;
	}
	const auto Transition = OwnershipTransition.Execute(Request, NewSurface);
	if (!Transition.IsAccepted() || !Transition.HasTransitionTicket())
	{
		if (bRehydrated)
		{
			FString RollbackDiagnostic;
			NewSurface.TryDiscardRehydratedVisibleForHandoff(
				ExpectedPhysicalCursor, RollbackDiagnostic);
		}
		OutDiagnostic = Transition.GetDiagnostic();
		return false;
	}

	const auto Handoff = OwnerHandoff.Execute(
		Owner,
		Transition.GetTransitionTicket(),
		*BoundSurface,
		NewSurface);
	if (Handoff.IsAccepted())
	{
		BoundSurface = &NewSurface;
		OutDiagnostic = Handoff.GetDiagnostic();
		return IsValid();
	}
	if (Handoff.NeedsManualRecovery())
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint
			Checkpoint;
		FString CheckpointDiagnostic;
		if (Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint::
				TryCreate(Handoff, Checkpoint, CheckpointDiagnostic))
		{
			const auto Recovery = OwnerHandoffRecovery.Execute(
				Owner, Checkpoint, *BoundSurface, NewSurface);
			if (Recovery.IsAccepted())
			{
				BoundSurface = &NewSurface;
				OutDiagnostic = Recovery.GetDiagnostic();
				return IsValid();
			}
			OutDiagnostic = Recovery.GetDiagnostic();
			return false;
		}
		OutDiagnostic = CheckpointDiagnostic;
		return false;
	}
	if (bRehydrated)
	{
		FString RollbackDiagnostic;
		NewSurface.TryDiscardRehydratedVisibleForHandoff(
			ExpectedPhysicalCursor, RollbackDiagnostic);
	}
	OutDiagnostic = Handoff.GetDiagnostic();
	return false;
}

bool FBinding::IsValid() const
{
	if (!FallbackSurface.IsInitialized())
	{
		return !FallbackSurface.IsValid()
			&& AttachedHUDSurface == nullptr && BoundSurface == nullptr
			&& Owner.IsEmpty() && !bOperationInProgress;
	}
	if (!FallbackSurface.IsValid()
		|| (AttachedHUDSurface != nullptr
			&& (!AttachedHUDSurface->IsValid()
				|| AttachedHUDSurface->GetSurfaceInstanceId()
					== FallbackSurface.GetSurfaceInstanceId())))
	{
		return false;
	}
	if (!Owner.IsActive())
	{
		return Owner.IsEmpty() && BoundSurface == nullptr;
	}
	if (!Owner.IsValid() || !Owner.IsSynchronized()
		|| BoundSurface == nullptr || !BoundSurface->IsValid()
		|| Owner.GetConsumerDefinitionId()
			!= BoundSurface->GetConsumerDefinitionId()
		|| !StatesMatchOrAreEmpty(
			PhysicalCursorFor(Owner.GetHostCursor()),
			BoundSurface->GetSurfaceCursor()))
	{
		return false;
	}
	return !Owner.GetBoundSurfaceInstanceId().IsValid()
		|| Owner.GetBoundSurfaceInstanceId()
			== BoundSurface->GetSurfaceInstanceId();
}
