#include "demo_mapShanmenThrownWeaponRunCommandRouter.h"

#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& SameContent(Left.GetContent(), Right.GetContent())
			&& Left.GetSourceTags() == Right.GetSourceTags();
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

	bool CoordinatorMatches(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenCombatActionSnapshot& Action,
		const AActor* SourceActor)
	{
		if (!Coordinator.IsReady()
			|| !Action.IsValid()
			|| !::IsValid(SourceActor)
			|| Coordinator.GetRunId() != Action.GetRunId()
			|| Coordinator.GetPlayerEntityId()
				!= Action.GetSourceEntityId())
		{
			return false;
		}
		FGuid ResolvedSourceEntityId;
		return Coordinator.GetEntityRegistry().TryResolveObject(
			Action.GetRunId(),
			const_cast<AActor*>(SourceActor),
			INDEX_NONE,
			ResolvedSourceEntityId)
			&& ResolvedSourceEntityId == Action.GetSourceEntityId();
	}

	bool IsActionStartup(
		const FShanmenActionTransitionReceipt& Receipt,
		const FGuid& ActivationId)
	{
		return Receipt.IsValid()
			&& Receipt.GetActivationId() == ActivationId
			&& Receipt.GetFromPhase() == EShanmenCombatActionPhase::Idle
			&& Receipt.GetToPhase() == EShanmenCombatActionPhase::Startup
			&& !Receipt.HasReachedCommitPoint();
	}

	bool IsActionActive(
		const FShanmenActionTransitionReceipt& Receipt,
		const FGuid& ActivationId)
	{
		return Receipt.IsValid()
			&& Receipt.GetActivationId() == ActivationId
			&& Receipt.GetFromPhase() == EShanmenCombatActionPhase::Startup
			&& Receipt.GetToPhase() == EShanmenCombatActionPhase::Active
			&& Receipt.CrossedCommitPointNow()
			&& Receipt.HasReachedCommitPoint();
	}
}

bool Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCapture(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FShanmenCombatActionSnapshot& RequestedAction,
	const FShanmenThrownWeaponDefinition& RequestedDefinition,
	const FShanmenThrownWeaponOffenseSnapshot& RequestedOffense,
	const FVector& RequestedOrigin,
	const FVector& RequestedAimDirection,
	float RequestedMaximumDistance,
	Fdemo_mapShanmenThrownWeaponRunCommandIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponRunCommandIntent();
	if (!RequestedCorrelation.IsValid()
		|| !RequestedAction.IsValid()
		|| !RequestedDefinition.IsValid()
		|| !RequestedOffense.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedAimDirection)
		|| RequestedAimDirection.IsNearlyZero()
		|| !FMath::IsFinite(RequestedMaximumDistance)
		|| RequestedMaximumDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutIntent.Correlation = RequestedCorrelation;
	OutIntent.Action = RequestedAction;
	OutIntent.Definition = RequestedDefinition;
	OutIntent.Offense = RequestedOffense;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.AimDirection = RequestedAimDirection.GetSafeNormal();
	OutIntent.MaximumDistance = RequestedMaximumDistance;
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponRunCommandIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponRunCommandIntent::IsValid() const
{
	const FGuid& ItemId = Action.GetSourceItemInstanceId();
	return Correlation.IsValid()
		&& Action.IsValid()
		&& Definition.IsValid()
		&& Offense.IsValid()
		&& Action.GetRunId() == Correlation.ActiveRunId
		&& Action.GetOwnerId() == Correlation.OwnerId
		&& Action.GetActionDefinitionId()
			== FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId()
		&& Definition.GetActionDefinitionId()
			== Action.GetActionDefinitionId()
		&& ItemId.IsValid()
		&& Correlation.OrderedPreparedItemInstanceIds.Contains(ItemId)
		&& Correlation.OrderedRunInventoryItemInstanceIds.Contains(ItemId)
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(AimDirection)
		&& AimDirection.IsNormalized()
		&& FMath::IsFinite(MaximumDistance)
		&& MaximumDistance > KINDA_SMALL_NUMBER;
}

bool Fdemo_mapShanmenThrownWeaponRunCommandIntent::Matches(
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& Correlation == Other.Correlation
		&& ActionsMatch(Action, Other.Action)
		&& DefinitionsMatch(Definition, Other.Definition)
		&& Offense.GetTechniquePower()
			== Other.Offense.GetTechniquePower()
		&& Origin == Other.Origin
		&& AimDirection == Other.AimDirection
		&& MaximumDistance == Other.MaximumDistance;
}

bool Fdemo_mapShanmenThrownWeaponRunCommandResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenThrownWeaponRunCommandStatus::Applied
		&& IntentId.IsValid()
		&& RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& LaunchId.IsValid()
		&& IsActionStartup(Startup, IntentId)
		&& IsActionActive(Active, IntentId)
		&& Preparation.IsPrepared()
		&& Preparation.Action.GetActivationId() == IntentId
		&& Preparation.Action.GetRunId() == RunId
		&& Preparation.Action.GetSourceItemInstanceId() == ItemInstanceId
		&& HostStart.IsStarted()
		&& HostStart.LaunchId == LaunchId
		&& HostStart.Launch.IsCommitted()
		&& !Cancellation.IsFinalized();
}

bool Fdemo_mapShanmenThrownWeaponRunCommandResult::IsDurableTerminal() const
{
	if (IsAccepted())
	{
		return true;
	}
	if (!IntentId.IsValid()
		|| !RunId.IsValid()
		|| !ItemInstanceId.IsValid()
		|| LaunchId.IsValid()
		|| !IsActionStartup(Startup, IntentId)
		|| !Preparation.IsPrepared()
		|| Preparation.Action.GetActivationId() != IntentId
		|| Preparation.Action.GetRunId() != RunId
		|| Preparation.Action.GetSourceItemInstanceId() != ItemInstanceId
		|| HostStart.IsStarted())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenThrownWeaponRunCommandStatus::
			LaunchRejectedCancelled)
	{
		return !HostStart.Launch.IsCommitted()
			&& Cancellation.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::Cancelled
			&& Cancellation.IsFinalized()
			&& !Cancellation.FinalizeRequest.bCommit;
	}
	if (Status
		!= Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryRequired)
	{
		return false;
	}
	return HostStart.Launch.IsCommitted()
		|| !Cancellation.IsFinalized()
		|| Cancellation.Status
			!= Edemo_mapShanmenThrownWeaponItemStatus::Cancelled;
}

Fdemo_mapShanmenThrownWeaponRunCommandResult
Fdemo_mapShanmenThrownWeaponRunCommandRouter::TryRoute(
	Fdemo_mapShanmenThrownWeaponRunHost& Host,
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* SourceActor,
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Intent)
{
	Fdemo_mapShanmenThrownWeaponRunCommandResult Result;
	Result.IntentId = Intent.GetIntentId();
	Result.RunId = Intent.GetRunId();
	Result.ItemInstanceId = Intent.GetAction().GetSourceItemInstanceId();
	if (!Coordinator.IsReady())
	{
		Result.Diagnostic =
			TEXT("Thrown-weapon command requires one ready combat Run.");
		return Result;
	}
	if (!Intent.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::IntentInvalid;
		Result.Diagnostic = TEXT("Thrown-weapon command intent is invalid.");
		return Result;
	}
	if (Coordinator.GetRunId() != Intent.GetRunId())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::RunMismatch;
		Result.Diagnostic =
			TEXT("Thrown-weapon command does not belong to this combat Run.");
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::RouterInvalid;
		Result.Diagnostic = TEXT("Thrown-weapon command Router is invalid.");
		return Result;
	}
	if (!IsEmpty() && RunId != Intent.GetRunId())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::RouterRunMismatch;
		Result.Diagnostic =
			TEXT("Thrown-weapon command Router belongs to another Run.");
		return Result;
	}

	if (const FProcessedIntent* Existing =
		ProcessedIntents.Find(Intent.GetIntentId()))
	{
		if (!Existing->Intent.Matches(Intent))
		{
			Result.Status =
				Edemo_mapShanmenThrownWeaponRunCommandStatus::IntentIdConflict;
			Result.Diagnostic =
				TEXT("Thrown-weapon ActivationId was reused with another payload.");
			return Result;
		}
		Result = Existing->Result;
		Result.bReplay = true;
		Result.Diagnostic =
			TEXT("Thrown-weapon command returned its terminal receipts without I/O.");
		return Result;
	}

	if (!CoordinatorMatches(Coordinator, Intent.GetAction(), SourceActor))
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::SourceMismatch;
		Result.Diagnostic =
			TEXT("Thrown-weapon source Actor is not the action's registered entity.");
		return Result;
	}
	if (Host.GetState() != Edemo_mapShanmenThrownWeaponHostState::Empty)
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::HostBusy;
		Result.Diagnostic =
			TEXT("Thrown-weapon Host must be empty before a new activation.");
		return Result;
	}

	FShanmenActionOrchestrator ActionRuntime;
	if (!FShanmenActionOrchestrator::TryStart(
		Intent.GetAction(), ActionRuntime, Result.Startup))
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::ActionStartRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon action runtime rejected the frozen action.");
		return Result;
	}
	FShanmenThrownWeaponExecution Execution;
	if (!FShanmenThrownWeaponExecution::TryCreate(
		Intent.GetAction(),
		Intent.GetDefinition(),
		Intent.GetOffense(),
		Execution))
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::ExecutionRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon execution rejected the frozen combat payload.");
		return Result;
	}

	Result.Preparation =
		Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
			Authority,
			Intent.GetCorrelation(),
			Intent.GetAction());
	if (!Result.Preparation.IsPrepared())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::PrepareRejected;
		Result.Diagnostic = Result.Preparation.Diagnostic;
		return Result;
	}

	if (!ActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, Result.Active))
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::ActionCommitRejected;
		Result.Cancellation =
			Fdemo_mapShanmenThrownWeaponItemAdapter::CancelBeforeLaunch(
				Authority, Intent.GetCorrelation(), Result.Preparation);
		Result.Status = Result.Cancellation.IsFinalized()
			&& Result.Cancellation.Status
				== Edemo_mapShanmenThrownWeaponItemStatus::Cancelled
			? Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			: Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryRequired;
		Result.Diagnostic = Result.Status
			== Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled
			? TEXT("Action commit failed and the prepared Quantity was cancelled.")
			: TEXT("Action commit failed and durable cancellation requires recovery.");
		RecordTerminal(Intent, Result);
		return Result;
	}

	Result.HostStart = Host.TrySpawnAndLaunchPrepared(
		World,
		ProjectileClass,
		Authority,
		Intent.GetCorrelation(),
		Result.Preparation,
		ActionRuntime,
		Execution,
		Coordinator,
		SourceActor,
		Intent.GetOrigin(),
		Intent.GetAimDirection(),
		Intent.GetMaximumDistance());
	if (Result.HostStart.IsStarted())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::Applied;
		Result.LaunchId = Result.HostStart.LaunchId;
		Result.Diagnostic =
			TEXT("Thrown-weapon Quantity and straight flight committed exactly once.");
		RecordTerminal(Intent, Result);
		return Result;
	}

	if (Result.HostStart.Launch.IsCommitted())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryRequired;
		Result.Diagnostic =
			TEXT("Thrown-weapon Quantity committed but Host adoption failed; do not relaunch.");
		RecordTerminal(Intent, Result);
		return Result;
	}

	Result.Cancellation =
		Fdemo_mapShanmenThrownWeaponItemAdapter::CancelBeforeLaunch(
			Authority, Intent.GetCorrelation(), Result.Preparation);
	const bool bCancelled = Result.Cancellation.IsFinalized()
		&& Result.Cancellation.Status
			== Edemo_mapShanmenThrownWeaponItemStatus::Cancelled;
	Result.Status = bCancelled
		? Edemo_mapShanmenThrownWeaponRunCommandStatus::
			LaunchRejectedCancelled
		: Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryRequired;
	Result.Diagnostic = bCancelled
		? TEXT("Pre-launch failure cancelled the prepared Quantity exactly once.")
		: TEXT("Pre-launch failure left a prepared Quantity requiring cancellation recovery.");
	RecordTerminal(Intent, Result);
	return Result;
}

Fdemo_mapShanmenThrownWeaponRunCommandResult
Fdemo_mapShanmenThrownWeaponRunCommandRouter::TryRecoverCancellation(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Intent)
{
	Fdemo_mapShanmenThrownWeaponRunCommandResult Result;
	Result.IntentId = Intent.GetIntentId();
	Result.RunId = Intent.GetRunId();
	Result.ItemInstanceId = Intent.GetAction().GetSourceItemInstanceId();
	if (!Intent.IsValid() || !IsValid())
	{
		Result.Status = !Intent.IsValid()
			? Edemo_mapShanmenThrownWeaponRunCommandStatus::IntentInvalid
			: Edemo_mapShanmenThrownWeaponRunCommandStatus::RouterInvalid;
		Result.Diagnostic =
			TEXT("Cancellation recovery requires valid intent and Router state.");
		return Result;
	}
	FProcessedIntent* Existing = ProcessedIntents.Find(Intent.GetIntentId());
	if (!Existing)
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryNotFound;
		Result.Diagnostic =
			TEXT("No terminal thrown-weapon record exists for recovery.");
		return Result;
	}
	if (!Existing->Intent.Matches(Intent))
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::IntentIdConflict;
		Result.Diagnostic =
			TEXT("Cancellation recovery rejected a conflicting ActivationId payload.");
		return Result;
	}
	if (Existing->Result.Status
		!= Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryRequired)
	{
		Result = Existing->Result;
		Result.bReplay = true;
		Result.Diagnostic =
			TEXT("Thrown-weapon record is already terminal and needs no recovery.");
		return Result;
	}
	if (Existing->Result.HostStart.Launch.IsCommitted())
	{
		Result = Existing->Result;
		Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::RecoveryNotApplicable;
		Result.Diagnostic =
			TEXT("A post-commit Host failure cannot be recovered by cancelling Quantity.");
		return Result;
	}

	Existing->Result.Cancellation =
		Fdemo_mapShanmenThrownWeaponItemAdapter::CancelBeforeLaunch(
			Authority,
			Intent.GetCorrelation(),
			Existing->Result.Preparation);
	if (Existing->Result.Cancellation.IsFinalized()
		&& Existing->Result.Cancellation.Status
			== Edemo_mapShanmenThrownWeaponItemStatus::Cancelled)
	{
		Existing->Result.Status =
			Edemo_mapShanmenThrownWeaponRunCommandStatus::
				LaunchRejectedCancelled;
		Existing->Result.Diagnostic =
			TEXT("Prepared thrown-weapon Quantity cancellation recovered durably.");
	}
	else
	{
		Existing->Result.Diagnostic = Existing->Result.Cancellation.Diagnostic;
	}
	return Existing->Result;
}

bool Fdemo_mapShanmenThrownWeaponRunCommandRouter::IsValid() const
{
	if (ProcessedIntents.IsEmpty())
	{
		return !RunId.IsValid();
	}
	if (!RunId.IsValid())
	{
		return false;
	}
	for (const TPair<FGuid, FProcessedIntent>& Pair : ProcessedIntents)
	{
		const FProcessedIntent& Processed = Pair.Value;
		if (Pair.Key != Processed.Intent.GetIntentId()
			|| !Processed.Intent.IsValid()
			|| Processed.Intent.GetRunId() != RunId
			|| Processed.Result.bReplay
			|| !Processed.Result.IsDurableTerminal()
			|| Processed.Result.IntentId != Pair.Key
			|| Processed.Result.RunId != RunId
			|| Processed.Result.ItemInstanceId
				!= Processed.Intent.GetAction().GetSourceItemInstanceId())
		{
			return false;
		}
	}
	return true;
}

void Fdemo_mapShanmenThrownWeaponRunCommandRouter::Reset()
{
	*this = Fdemo_mapShanmenThrownWeaponRunCommandRouter();
}

void Fdemo_mapShanmenThrownWeaponRunCommandRouter::RecordTerminal(
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent& Intent,
	const Fdemo_mapShanmenThrownWeaponRunCommandResult& Result)
{
	if (!Intent.IsValid() || Result.bReplay || !Result.IsDurableTerminal())
	{
		return;
	}
	if (IsEmpty())
	{
		RunId = Intent.GetRunId();
	}
	FProcessedIntent Processed;
	Processed.Intent = Intent;
	Processed.Result = Result;
	ProcessedIntents.Add(Intent.GetIntentId(), MoveTemp(Processed));
}
