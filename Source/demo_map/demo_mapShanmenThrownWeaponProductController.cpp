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

	FShanmenThrownWeaponArcRequestCapture BuildArcRequest(
		const FShanmenCombatActionSnapshot& Action,
		const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection,
		const Fdemo_mapShanmenThrownWeaponProductCapture& Product)
	{
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
			Product.GetArcPolicy();
		FShanmenThrownWeaponArcRequestCapture Request;
		Request.Action = Action;
		Request.TechniqueTier = ArcPolicy.GetTechniqueTier();
		Request.Origin = Selection.GetOrigin();
		Request.Target = Selection.GetTarget();
		Request.GravityMagnitude = ArcPolicy.GetGravityMagnitude();
		Request.ApexClearance = Selection.GetApexClearance();
		Request.MaximumLaunchSpeed = Product.GetDefinition().GetLaunchSpeed();
		Request.MaximumFlightTime = ArcPolicy.GetMaximumFlightTime();
		return Request;
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
	OutIntent.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight;
	OutIntent.AimDirection = RequestedAimDirection.GetSafeNormal();
	OutIntent.MaximumDistance = RequestedMaximumDistance;
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponSelectionIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCaptureArc(
	const FGuid& RequestedSelectionId,
	const FGuid& RequestedRunId,
	const FGuid& RequestedSourceItemInstanceId,
	const FVector& RequestedOrigin,
	const FVector& RequestedTarget,
	double RequestedApexClearance,
	Fdemo_mapShanmenThrownWeaponSelectionIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponSelectionIntent();
	if (!RequestedSelectionId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedSourceItemInstanceId.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedTarget)
		|| RequestedOrigin.Equals(
			RequestedTarget, UE_DOUBLE_SMALL_NUMBER)
		|| !FMath::IsFinite(RequestedApexClearance)
		|| RequestedApexClearance <= 0.0)
	{
		return false;
	}

	OutIntent.SelectionId = RequestedSelectionId;
	OutIntent.RunId = RequestedRunId;
	OutIntent.SourceItemInstanceId = RequestedSourceItemInstanceId;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc;
	OutIntent.Target = RequestedTarget;
	OutIntent.ApexClearance = RequestedApexClearance;
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponSelectionIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponSelectionIntent::IsValid() const
{
	const bool bCommonValid = SelectionId.IsValid()
		&& RunId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& IsFiniteVector(Origin);
	if (!bCommonValid)
	{
		return false;
	}

	switch (TrajectoryKind)
	{
	case Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight:
		return IsFiniteVector(AimDirection)
			&& AimDirection.IsNormalized()
			&& FMath::IsFinite(MaximumDistance)
			&& MaximumDistance > KINDA_SMALL_NUMBER
			&& Target == FVector::ZeroVector
			&& ApexClearance == 0.0;
	case Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc:
		return AimDirection == FVector::ZeroVector
			&& MaximumDistance == 0.0f
			&& IsFiniteVector(Target)
			&& !Origin.Equals(Target, UE_DOUBLE_SMALL_NUMBER)
			&& FMath::IsFinite(ApexClearance)
			&& ApexClearance > 0.0;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponSelectionIntent::Matches(
	const Fdemo_mapShanmenThrownWeaponSelectionIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& SelectionId == Other.SelectionId
		&& RunId == Other.RunId
		&& SourceItemInstanceId == Other.SourceItemInstanceId
		&& Origin == Other.Origin
		&& TrajectoryKind == Other.TrajectoryKind
		&& AimDirection == Other.AimDirection
		&& MaximumDistance == Other.MaximumDistance
		&& Target == Other.Target
		&& ApexClearance == Other.ApexClearance;
}

bool Fdemo_mapShanmenThrownWeaponArcProductPolicy::TryCapture(
	EShanmenThrownWeaponTechniqueTier RequestedTechniqueTier,
	double RequestedGravityMagnitude,
	double RequestedMaximumFlightTime,
	Fdemo_mapShanmenThrownWeaponArcProductPolicy& OutPolicy)
{
	OutPolicy = Fdemo_mapShanmenThrownWeaponArcProductPolicy();
	if ((RequestedTechniqueTier
			!= EShanmenThrownWeaponTechniqueTier::Intermediate
			&& RequestedTechniqueTier
				!= EShanmenThrownWeaponTechniqueTier::Master)
		|| !FMath::IsFinite(RequestedGravityMagnitude)
		|| RequestedGravityMagnitude <= 0.0
		|| !FMath::IsFinite(RequestedMaximumFlightTime)
		|| RequestedMaximumFlightTime <= 0.0)
	{
		return false;
	}

	OutPolicy.TechniqueTier = RequestedTechniqueTier;
	OutPolicy.GravityMagnitude = RequestedGravityMagnitude;
	OutPolicy.MaximumFlightTime = RequestedMaximumFlightTime;
	return OutPolicy.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcProductPolicy::IsValid() const
{
	return (TechniqueTier
			== EShanmenThrownWeaponTechniqueTier::Intermediate
			|| TechniqueTier == EShanmenThrownWeaponTechniqueTier::Master)
		&& FMath::IsFinite(GravityMagnitude)
		&& GravityMagnitude > 0.0
		&& FMath::IsFinite(MaximumFlightTime)
		&& MaximumFlightTime > 0.0;
}

bool Fdemo_mapShanmenThrownWeaponArcProductPolicy::Matches(
	const Fdemo_mapShanmenThrownWeaponArcProductPolicy& Other) const
{
	return IsValid() && Other.IsValid()
		&& TechniqueTier == Other.TechniqueTier
		&& GravityMagnitude == Other.GravityMagnitude
		&& MaximumFlightTime == Other.MaximumFlightTime;
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
			TechniquePower, OutCapture.Offense)
		|| OutCapture.Definition.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		OutCapture = Fdemo_mapShanmenThrownWeaponProductCapture();
		return false;
	}
	OutCapture.SourceTags = RequestedSourceTags;
	return OutCapture.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponProductCapture::TryCaptureArc(
	const FShanmenThrownWeaponDefinitionCapture& RequestedDefinition,
	float TechniquePower,
	const FGameplayTagContainer& RequestedSourceTags,
	EShanmenThrownWeaponTechniqueTier TechniqueTier,
	double GravityMagnitude,
	double MaximumFlightTime,
	Fdemo_mapShanmenThrownWeaponProductCapture& OutCapture)
{
	OutCapture = Fdemo_mapShanmenThrownWeaponProductCapture();
	if (!FShanmenThrownWeaponDefinition::TryCapture(
			RequestedDefinition, OutCapture.Definition)
		|| OutCapture.Definition.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		|| !FShanmenThrownWeaponOffenseSnapshot::TryCapture(
			TechniquePower, OutCapture.Offense)
		|| !Fdemo_mapShanmenThrownWeaponArcProductPolicy::TryCapture(
			TechniqueTier,
			GravityMagnitude,
			MaximumFlightTime,
			OutCapture.ArcPolicy))
	{
		OutCapture = Fdemo_mapShanmenThrownWeaponProductCapture();
		return false;
	}
	OutCapture.SourceTags = RequestedSourceTags;
	return OutCapture.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponProductCapture::IsValid() const
{
	if (!Definition.IsValid() || !Offense.IsValid())
	{
		return false;
	}
	if (Definition.GetActionDefinitionId()
		== FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
	{
		return !ArcPolicy.IsValid();
	}
	return Definition.GetActionDefinitionId()
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		&& ArcPolicy.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponProductCapture::Matches(
	const Fdemo_mapShanmenThrownWeaponProductCapture& Other) const
{
	return IsValid() && Other.IsValid()
		&& DefinitionsMatch(Definition, Other.Definition)
		&& Offense.GetTechniquePower()
			== Other.Offense.GetTechniquePower()
		&& SourceTags == Other.SourceTags
		&& ((!ArcPolicy.IsValid() && !Other.ArcPolicy.IsValid())
			|| ArcPolicy.Matches(Other.ArcPolicy));
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
	const FName ProductActionDefinitionId =
		Product.GetDefinition().GetActionDefinitionId();
	const bool bTrajectoryMatchesProduct =
		(Selection.GetTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
			&& ProductActionDefinitionId
				== FShanmenThrownWeaponDefinition::StraightActionDefinitionId())
		|| (Selection.GetTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
			&& ProductActionDefinitionId
				== FShanmenThrownWeaponDefinition::ArcActionDefinitionId());
	if (!bTrajectoryMatchesProduct)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::TrajectoryMismatch,
			Selection,
			TEXT("Selection trajectory and immutable product definition disagree."));
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
	if (const FRejectedArcSelection* Existing =
		RejectedArcSelections.Find(Selection.GetSelectionId()))
	{
		if (!Existing->Selection.Matches(Selection)
			|| !Existing->Product.Matches(Product))
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponProductStatus::SelectionIdConflict,
				Selection,
				TEXT("Thrown-weapon SelectionId was reused with another frozen payload."));
		}
		Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
			Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected,
			Selection,
			TEXT("Arc planning rejection was already recorded for this selection."));
		Result.bReusedSelection = true;
		Result.ActivationSequence = Existing->ActivationSequence;
		Result.ActivationId = Existing->Action.GetActivationId();
		Result.Diagnostic = Existing->Diagnostic;
		return Result;
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
			ItemId,
			ProductActionDefinitionId,
			Reservation,
			ReservationDiagnostic))
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
	ActionCapture.ActionDefinitionId = Reservation.ActionDefinitionId;
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
	bool bCommandCaptured = false;
	if (Selection.GetTrajectoryKind()
		== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
	{
		bCommandCaptured =
			Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCapture(
				Correlation,
				Action,
				Product.GetDefinition(),
				Product.GetOffense(),
				Selection.GetOrigin(),
				Selection.GetAimDirection(),
				Selection.GetMaximumDistance(),
				Captured.Command);
	}
	else
	{
		const FShanmenThrownWeaponArcPlanResult ArcResult =
			FShanmenThrownWeaponArcPlanner::Plan(
				BuildArcRequest(Action, Selection, Product));
		if (!ArcResult.IsPlanned())
		{
			Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
				Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected,
				Selection,
				TEXT("Validated Arc selection could not produce a product-owned plan."));
			Result.ActivationSequence = Reservation.ActivationSequence;
			Result.ActivationId = Reservation.ActivationId;
			Result.Diagnostic = ArcResult.Diagnostic.IsEmpty()
				? Result.Diagnostic : ArcResult.Diagnostic;

			FRejectedArcSelection Rejected;
			Rejected.Selection = Selection;
			Rejected.Product = Product;
			Rejected.ActivationSequence = Reservation.ActivationSequence;
			Rejected.Action = Action;
			Rejected.Diagnostic = Result.Diagnostic;
			if (IsEmpty())
			{
				RunId = Selection.GetRunId();
			}
			RejectedArcSelections.Add(
				Selection.GetSelectionId(), MoveTemp(Rejected));
			if (!IsValid())
			{
				RejectedArcSelections.Remove(Selection.GetSelectionId());
				if (IsEmpty())
				{
					RunId.Invalidate();
				}
				Result.Status =
					Edemo_mapShanmenThrownWeaponProductStatus::ControllerInvalid;
				Result.Diagnostic =
					TEXT("Rejected Arc selection failed controller invariants.");
			}
			return Result;
		}
		bCommandCaptured =
			Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCaptureArc(
				Correlation,
				Action,
				Product.GetDefinition(),
				Product.GetOffense(),
				ArcResult.Plan,
				Captured.Command);
	}
	if (!bCommandCaptured)
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
		if (IsEmpty())
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
		if (const FRejectedArcSelection* Rejected =
			RejectedArcSelections.Find(Selection.GetSelectionId()))
		{
			if (!Rejected->Selection.Matches(Selection))
			{
				return Reject(
					Edemo_mapShanmenThrownWeaponProductStatus::SelectionIdConflict,
					Selection,
					TEXT("Cancellation recovery rejected a conflicting selection payload."));
			}
			Fdemo_mapShanmenThrownWeaponProductResult Result = Reject(
				Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected,
				Selection,
				TEXT("An Arc planning rejection has no item cancellation to recover."));
			Result.bReusedSelection = true;
			Result.ActivationSequence = Rejected->ActivationSequence;
			Result.ActivationId = Rejected->Action.GetActivationId();
			Result.Diagnostic = Rejected->Diagnostic;
			return Result;
		}
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
	if (IsEmpty())
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
		bool bTrajectoryValid = false;
		if (Captured.Selection.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
		{
			bTrajectoryValid = Captured.Command.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
				&& Captured.Selection.GetOrigin()
					== Captured.Command.GetOrigin()
				&& Captured.Selection.GetAimDirection()
					== Captured.Command.GetAimDirection()
				&& Captured.Selection.GetMaximumDistance()
					== Captured.Command.GetMaximumDistance()
				&& !Captured.Product.GetArcPolicy().IsValid();
		}
		else if (Captured.Selection.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		{
			const FShanmenThrownWeaponArcRequest& ArcRequest =
				Captured.Command.GetArcPlan().GetRequest();
			const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
				Captured.Product.GetArcPolicy();
			bTrajectoryValid = Captured.Command.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
				&& ArcPolicy.IsValid()
				&& ArcRequest.GetOrigin()
					== Captured.Selection.GetOrigin()
				&& ArcRequest.GetTarget()
					== Captured.Selection.GetTarget()
				&& ArcRequest.GetApexClearance()
					== Captured.Selection.GetApexClearance()
				&& ArcRequest.GetTechniqueTier()
					== ArcPolicy.GetTechniqueTier()
				&& ArcRequest.GetGravityMagnitude()
					== ArcPolicy.GetGravityMagnitude()
				&& ArcRequest.GetMaximumFlightTime()
					== ArcPolicy.GetMaximumFlightTime()
				&& ArcRequest.GetMaximumLaunchSpeed()
					== Captured.Product.GetDefinition().GetLaunchSpeed();
		}
		if (Pair.Key != Captured.Selection.GetSelectionId()
			|| !Captured.Selection.IsValid()
			|| !Captured.Product.IsValid()
			|| Captured.ActivationSequence == 0
			|| !Captured.Command.IsValid()
			|| Captured.Selection.GetRunId() != RunId
			|| Captured.Command.GetRunId() != RunId
			|| Captured.Selection.GetSourceItemInstanceId()
				!= Action.GetSourceItemInstanceId()
			|| !bTrajectoryValid
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

	for (const TPair<FGuid, FRejectedArcSelection>& Pair
		: RejectedArcSelections)
	{
		const FRejectedArcSelection& Rejected = Pair.Value;
		const FShanmenCombatActionSnapshot& Action = Rejected.Action;
		const FShanmenThrownWeaponArcPlanResult ArcResult =
			FShanmenThrownWeaponArcPlanner::Plan(
				BuildArcRequest(
					Action, Rejected.Selection, Rejected.Product));
		if (CapturedSelections.Contains(Pair.Key)
			|| Pair.Key != Rejected.Selection.GetSelectionId()
			|| !Rejected.Selection.IsValid()
			|| Rejected.Selection.GetTrajectoryKind()
				!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc
			|| !Rejected.Product.IsValid()
			|| Rejected.Product.GetDefinition().GetActionDefinitionId()
				!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
			|| Rejected.ActivationSequence == 0
			|| !Action.IsValid()
			|| Action.GetRunId() != RunId
			|| Rejected.Selection.GetRunId() != RunId
			|| Action.GetSourceItemInstanceId()
				!= Rejected.Selection.GetSourceItemInstanceId()
			|| Action.GetActionDefinitionId()
				!= Rejected.Product.GetDefinition().GetActionDefinitionId()
			|| !Action.GetSourceTags().HasAll(
				Rejected.Product.GetSourceTags())
			|| !Action.GetSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer())
			|| !Action.GetSourceTags().HasTagExact(
				FShanmenItemNativeTags::ItemWeaponThrown())
			|| Action.GetActivationId()
				!= FShanmenCombatIdFactory::MakeActivationId(
					Action.GetRunId(),
					Action.GetSourceEntityId(),
					Action.GetActionDefinitionId(),
					Rejected.ActivationSequence)
			|| !ArcResult.IsValid()
			|| ArcResult.Status
				!= EShanmenThrownWeaponArcPlanStatus::Unreachable
			|| ArcResult.IsPlanned()
			|| Rejected.Diagnostic != ArcResult.Diagnostic
			|| Sequences.Contains(Rejected.ActivationSequence)
			|| ActivationIds.Contains(Action.GetActivationId()))
		{
			return false;
		}
		Sequences.Add(Rejected.ActivationSequence);
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
