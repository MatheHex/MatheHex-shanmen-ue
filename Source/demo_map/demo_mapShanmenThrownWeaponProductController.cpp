#include "demo_mapShanmenThrownWeaponProductController.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool DefinitionsMatch(
		const FShanmenThrownWeaponDefinition& Left,
		const FShanmenThrownWeaponDefinition& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetDetectorId() == Right.GetDetectorId()
			&& Left.GetFormulaId() == Right.GetFormulaId()
			&& Left.GetBaseDamage() == Right.GetBaseDamage()
			&& Left.GetTechniquePowerCoefficient()
				== Right.GetTechniquePowerCoefficient()
			&& Left.GetLaunchSpeed() == Right.GetLaunchSpeed()
			&& Left.GetDamageTags() == Right.GetDamageTags()
			&& Left.GetRequiredTargetTags()
				== Right.GetRequiredTargetTags()
			&& Left.RejectsSelf() == Right.RejectsSelf();
	}

	Fdemo_mapShanmenThrownWeaponProductResult Reject(
		Edemo_mapShanmenThrownWeaponProductStatus Status,
		const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenThrownWeaponProductResult Result;
		Result.Status = Status;
		Result.SelectionId = Selection.GetSelectionId();
		Result.RunId = Selection.GetRunId();
		Result.SourceItemInstanceId = Selection.GetSourceItemInstanceId();
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponProductResult RouteCaptured(
		Fdemo_mapShanmenThrownWeaponRunHost& Host,
		Fdemo_mapShanmenThrownWeaponRunCommandRouter& Router,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection,
		uint64 ActivationSequence,
		const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Command,
		bool bReusedSelection)
	{
		Fdemo_mapShanmenThrownWeaponProductResult Result;
		Result.bReusedSelection = bReusedSelection;
		Result.SelectionId = Selection.GetSelectionId();
		Result.RunId = Selection.GetRunId();
		Result.SourceItemInstanceId = Selection.GetSourceItemInstanceId();
		Result.ActivationSequence = ActivationSequence;
		Result.ActivationId = Command.GetIntentId();
		Result.Command = Router.TryRoute(
			Host,
			World,
			ProjectileClass,
			Authority,
			Coordinator,
			SourceActor,
			Command);
		Result.Status = Result.Command.IsAccepted()
			? Edemo_mapShanmenThrownWeaponProductStatus::Applied
			: Edemo_mapShanmenThrownWeaponProductStatus::RouterRejected;
		Result.Diagnostic = Result.Command.Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
	const FGuid& RequestedSelectionId,
	const FGuid& RequestedRunId,
	const FGuid& RequestedSourceItemInstanceId,
	const FVector& RequestedOrigin,
	const FVector& RequestedAimDirection,
	float RequestedMaximumDistance,
	Fdemo_mapShanmenThrownWeaponSelectionIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponSelectionIntent();
	if (!RequestedSelectionId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedSourceItemInstanceId.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedAimDirection)
		|| RequestedAimDirection.IsNearlyZero()
		|| !FMath::IsFinite(RequestedMaximumDistance)
		|| RequestedMaximumDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutIntent.SelectionId = RequestedSelectionId;
	OutIntent.RunId = RequestedRunId;
	OutIntent.SourceItemInstanceId = RequestedSourceItemInstanceId;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.AimDirection = RequestedAimDirection.GetSafeNormal();
	OutIntent.MaximumDistance = RequestedMaximumDistance;
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponSelectionIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponSelectionIntent::IsValid() const
{
	return SelectionId.IsValid()
		&& RunId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(AimDirection)
		&& AimDirection.IsNormalized()
		&& FMath::IsFinite(MaximumDistance)
		&& MaximumDistance > KINDA_SMALL_NUMBER;
}

bool Fdemo_mapShanmenThrownWeaponSelectionIntent::Matches(
	const Fdemo_mapShanmenThrownWeaponSelectionIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& SelectionId == Other.SelectionId
		&& RunId == Other.RunId
		&& SourceItemInstanceId == Other.SourceItemInstanceId
		&& Origin == Other.Origin
		&& AimDirection == Other.AimDirection
		&& MaximumDistance == Other.MaximumDistance;
}

bool Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
	const FShanmenThrownWeaponDefinitionCapture& RequestedDefinition,
	float TechniquePower,
	const FGameplayTagContainer& RequestedSourceTags,
	Fdemo_mapShanmenThrownWeaponProductCapture& OutCapture)
{
	OutCapture = Fdemo_mapShanmenThrownWeaponProductCapture();
	if (!FShanmenThrownWeaponDefinition::TryCapture(
			RequestedDefinition, OutCapture.Definition)
		|| !FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			TechniquePower, OutCapture.Offense))
	{
		OutCapture = Fdemo_mapShanmenThrownWeaponProductCapture();
		return false;
	}
	OutCapture.SourceTags = RequestedSourceTags;
	return OutCapture.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponProductCapture::IsValid() const
{
	return Definition.IsValid() && Offense.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponProductCapture::Matches(
	const Fdemo_mapShanmenThrownWeaponProductCapture& Other) const
{
	return IsValid() && Other.IsValid()
		&& DefinitionsMatch(Definition, Other.Definition)
		&& Offense.GetTechniquePower()
			== Other.Offense.GetTechniquePower()
		&& SourceTags == Other.SourceTags;
}

bool Fdemo_mapShanmenThrownWeaponProductResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenThrownWeaponProductStatus::Applied
		&& HasCapturedAction()
		&& Command.IsAccepted()
		&& Command.IntentId == ActivationId
		&& Command.RunId == RunId
		&& Command.ItemInstanceId == SourceItemInstanceId;
}

bool Fdemo_mapShanmenThrownWeaponProductResult::IsRecoveryApplied() const
{
	return Status
			== Edemo_mapShanmenThrownWeaponProductStatus::RecoveryApplied
		&& HasCapturedAction()
		&& Command.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
		&& Command.IsDurableTerminal();
}

bool Fdemo_mapShanmenThrownWeaponProductResult::HasCapturedAction() const
{
	return SelectionId.IsValid()
		&& RunId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& ActivationSequence > 0
		&& ActivationId.IsValid();
}

Fdemo_mapShanmenThrownWeaponProductResult
Fdemo_mapShanmenThrownWeaponProductController::TrySubmit(
	Fdemo_mapShanmenThrownWeaponRunHost& Host,
	Fdemo_mapShanmenThrownWeaponRunCommandRouter& Router,
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* SourceActor,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection,
	const Fdemo_mapShanmenThrownWeaponProductCapture& Product)
{
	if (!Coordinator.IsReady())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::CoordinatorNotReady,
			Selection,
			TEXT("Thrown-weapon product requires one ready combat Run."));
	}
	if (!Selection.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::SelectionInvalid,
			Selection,
			TEXT("Thrown-weapon product selection is invalid."));
	}
	if (!Product.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::CaptureInvalid,
			Selection,
			TEXT("Thrown-weapon product combat capture is invalid."));
	}
	if (!Correlation.IsValid()
		|| Correlation.ActiveRunId != Selection.GetRunId()
		|| Coordinator.GetRunId() != Selection.GetRunId()
		|| !Correlation.OrderedPreparedItemInstanceIds.Contains(
			Selection.GetSourceItemInstanceId())
		|| !Correlation.OrderedRunInventoryItemInstanceIds.Contains(
			Selection.GetSourceItemInstanceId()))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::RunMismatch,
			Selection,
			TEXT("Selection, item correlation, and combat owner must name one active Run."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ControllerInvalid,
			Selection,
			TEXT("Thrown-weapon product controller is invalid."));
	}
	if (!IsEmpty() && RunId != Selection.GetRunId())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ControllerRunMismatch,
			Selection,
			TEXT("Thrown-weapon product controller belongs to another Run."));
	}

	if (const FCapturedSelection* Existing =
		CapturedSelections.Find(Selection.GetSelectionId()))
	{
		if (!Existing->Selection.Matches(Selection)
			|| !Existing->Product.Matches(Product))
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponProductStatus::SelectionIdConflict,
				Selection,
				TEXT("Thrown-weapon SelectionId was reused with another frozen payload."));
		}
		return RouteCaptured(
			Host,
			Router,
			World,
			ProjectileClass,
			Authority,
			Coordinator,
			SourceActor,
			Selection,
			Existing->ActivationSequence,
			Existing->Command,
			true);
	}

	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::AuthorityNotReady,
			Selection,
			TEXT("Thrown-weapon selection requires the ready item authority on the Game Thread."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::SnapshotUnavailable,
			Selection,
			TEXT("Thrown-weapon selection could not capture item authority."));
	}
	if (!Snapshot.Content.IsValid()
		|| Snapshot.AuthorityRevision < Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::SnapshotStale,
			Selection,
			TEXT("Thrown-weapon item evidence predates the active Run."));
	}

	const FGuid& ItemId = Selection.GetSourceItemInstanceId();
	const FShanmenItemInstance* Item = Snapshot.Items.FindByPredicate(
		[&ItemId](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == ItemId;
		});
	// Active-Run Quantity lives in the committed reservation. The source item
	// may therefore expose zero loose Quantity here; P7.1 alone owns the
	// availability calculation and pending-intent exclusion.
	if (!Item
		|| Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ItemNotFound,
			Selection,
			TEXT("The exact selected item is absent from this active Run scope."));
	}
	const FShanmenItemDefinition* ItemDefinition =
		Snapshot.Definitions.FindByPredicate(
			[Item](const FShanmenItemDefinition& Candidate)
			{
				return Candidate.DefinitionId == Item->DefinitionId;
			});
	if (!ItemDefinition
		|| !ItemDefinition->IsValid()
		|| !ItemDefinition->Supports(EShanmenItemResourceKind::Quantity)
		|| !ItemDefinition->ItemTags.HasTagExact(
			FShanmenItemNativeTags::ItemWeaponThrown()))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::
				DefinitionNotThrownWeapon,
			Selection,
			TEXT("Only authority-tagged Quantity thrown weapons may be selected."));
	}

	Fdemo_mapPlayerThrownWeaponActionReservation Reservation;
	FString ReservationDiagnostic;
	if (!Coordinator.TryReservePlayerThrownWeaponAction(
			ItemId, Reservation, ReservationDiagnostic))
	{
		Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::
				SequenceReservationRejected,
			Selection,
			TEXT("Combat Run rejected the thrown-weapon action sequence."));
		Result.Diagnostic = ReservationDiagnostic.IsEmpty()
			? Result.Diagnostic : ReservationDiagnostic;
		return Result;
	}

	FShanmenCombatActionCapture ActionCapture;
	ActionCapture.RunId = Selection.GetRunId();
	ActionCapture.OwnerId = Correlation.OwnerId;
	ActionCapture.ActivationId = Reservation.ActivationId;
	ActionCapture.SourceEntityId = Reservation.SourceEntityId;
	ActionCapture.SourceItemInstanceId = ItemId;
	ActionCapture.ActionDefinitionId =
		FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
	ActionCapture.Content = Snapshot.Content;
	ActionCapture.SourceTags = Product.GetSourceTags();
	ActionCapture.SourceTags.AddTag(
		FShanmenCombatNativeTags::SourcePlayer());
	ActionCapture.SourceTags.AppendTags(ItemDefinition->ItemTags);
	FShanmenCombatActionSnapshot Action;
	if (!FShanmenCombatActionSnapshot::TryCapture(ActionCapture, Action))
	{
		Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ActionCaptureRejected,
			Selection,
			TEXT("Validated selection could not enter the immutable action contract."));
		Result.ActivationSequence = Reservation.ActivationSequence;
		Result.ActivationId = Reservation.ActivationId;
		return Result;
	}

	FCapturedSelection Captured;
	Captured.Selection = Selection;
	Captured.Product = Product;
	Captured.ActivationSequence = Reservation.ActivationSequence;
	if (!Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCapture(
			Correlation,
			Action,
			Product.GetDefinition(),
			Product.GetOffense(),
			Selection.GetOrigin(),
			Selection.GetAimDirection(),
			Selection.GetMaximumDistance(),
			Captured.Command))
	{
		Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::CommandCaptureRejected,
			Selection,
			TEXT("Validated selection could not enter the P7.4 command contract."));
		Result.ActivationSequence = Reservation.ActivationSequence;
		Result.ActivationId = Reservation.ActivationId;
		return Result;
	}

	if (IsEmpty())
	{
		RunId = Selection.GetRunId();
	}
	CapturedSelections.Add(
		Selection.GetSelectionId(), MoveTemp(Captured));
	if (!IsValid())
	{
		CapturedSelections.Remove(Selection.GetSelectionId());
		if (CapturedSelections.IsEmpty())
		{
			RunId.Invalidate();
		}
		Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ControllerInvalid,
			Selection,
			TEXT("Captured selection failed product-controller invariants."));
		Result.ActivationSequence = Reservation.ActivationSequence;
		Result.ActivationId = Reservation.ActivationId;
		return Result;
	}

	const FCapturedSelection& Stored =
		CapturedSelections.FindChecked(Selection.GetSelectionId());
	return RouteCaptured(
		Host,
		Router,
		World,
		ProjectileClass,
		Authority,
		Coordinator,
		SourceActor,
		Selection,
		Stored.ActivationSequence,
		Stored.Command,
		false);
}

Fdemo_mapShanmenThrownWeaponProductResult
Fdemo_mapShanmenThrownWeaponProductController::TryRecoverCancellation(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapShanmenThrownWeaponRunCommandRouter& Router,
	const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection)
{
	if (!Selection.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::SelectionInvalid,
			Selection,
			TEXT("Cancellation recovery requires one valid selection."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ControllerInvalid,
			Selection,
			TEXT("Cancellation recovery requires a valid product controller."));
	}
	const FCapturedSelection* Existing =
		CapturedSelections.Find(Selection.GetSelectionId());
	if (!Existing)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::SelectionNotFound,
			Selection,
			TEXT("No captured thrown-weapon selection exists for recovery."));
	}
	if (!Existing->Selection.Matches(Selection))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::SelectionIdConflict,
			Selection,
			TEXT("Cancellation recovery rejected a conflicting selection payload."));
	}

	Fdemo_mapShanmenThrownWeaponProductResult Result;
	Result.bReusedSelection = true;
	Result.SelectionId = Selection.GetSelectionId();
	Result.RunId = Selection.GetRunId();
	Result.SourceItemInstanceId = Selection.GetSourceItemInstanceId();
	Result.ActivationSequence = Existing->ActivationSequence;
	Result.ActivationId = Existing->Command.GetIntentId();
	Result.Command = Router.TryRecoverCancellation(
		Authority, Existing->Command);
	Result.Status = Result.Command.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
		? Edemo_mapShanmenThrownWeaponProductStatus::RecoveryApplied
		: Edemo_mapShanmenThrownWeaponProductStatus::RouterRejected;
	Result.Diagnostic = Result.Command.Diagnostic;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponProductController::IsValid() const
{
	if (CapturedSelections.IsEmpty())
	{
		return !RunId.IsValid();
	}
	if (!RunId.IsValid())
	{
		return false;
	}

	TSet<uint64> Sequences;
	TSet<FGuid> ActivationIds;
	for (const TPair<FGuid, FCapturedSelection>& Pair : CapturedSelections)
	{
		const FCapturedSelection& Captured = Pair.Value;
		const FShanmenCombatActionSnapshot& Action =
			Captured.Command.GetAction();
		if (Pair.Key != Captured.Selection.GetSelectionId()
			|| !Captured.Selection.IsValid()
			|| !Captured.Product.IsValid()
			|| Captured.ActivationSequence == 0
			|| !Captured.Command.IsValid()
			|| Captured.Selection.GetRunId() != RunId
			|| Captured.Command.GetRunId() != RunId
			|| Captured.Selection.GetSourceItemInstanceId()
				!= Action.GetSourceItemInstanceId()
			|| Captured.Selection.GetOrigin()
				!= Captured.Command.GetOrigin()
			|| Captured.Selection.GetAimDirection()
				!= Captured.Command.GetAimDirection()
			|| Captured.Selection.GetMaximumDistance()
				!= Captured.Command.GetMaximumDistance()
			|| !DefinitionsMatch(
				Captured.Product.GetDefinition(),
				Captured.Command.GetDefinition())
			|| Captured.Product.GetOffense().GetTechniquePower()
				!= Captured.Command.GetOffense().GetTechniquePower()
			|| !Action.GetSourceTags().HasAll(
				Captured.Product.GetSourceTags())
			|| !Action.GetSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer())
			|| !Action.GetSourceTags().HasTagExact(
				FShanmenItemNativeTags::ItemWeaponThrown())
			|| Action.GetActivationId()
				!= FShanmenCombatIdFactory::MakeActivationId(
					Action.GetRunId(),
					Action.GetSourceEntityId(),
					Action.GetActionDefinitionId(),
					Captured.ActivationSequence)
			|| Sequences.Contains(Captured.ActivationSequence)
			|| ActivationIds.Contains(Action.GetActivationId()))
		{
			return false;
		}
		Sequences.Add(Captured.ActivationSequence);
		ActivationIds.Add(Action.GetActivationId());
	}
	return true;
}

const Fdemo_mapShanmenThrownWeaponRunCommandIntent*
Fdemo_mapShanmenThrownWeaponProductController::FindCapturedCommand(
	const FGuid& SelectionId) const
{
	const FCapturedSelection* Captured = CapturedSelections.Find(SelectionId);
	return IsValid() && Captured ? &Captured->Command : nullptr;
}

void Fdemo_mapShanmenThrownWeaponProductController::Reset()
{
	*this = Fdemo_mapShanmenThrownWeaponProductController();
}
