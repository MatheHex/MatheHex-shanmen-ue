#include "demo_mapShanmenSpiritEvasionCommandRouter.h"

#include "demo_mapCombatRunCoordinator.h"
#include "GameFramework/Character.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool IsControlSignal(Edemo_mapShanmenSpiritEvasionCommandKind Kind)
	{
		return Kind == Edemo_mapShanmenSpiritEvasionCommandKind::Cancel
			|| Kind == Edemo_mapShanmenSpiritEvasionCommandKind::Interrupt
			|| Kind
				== Edemo_mapShanmenSpiritEvasionCommandKind::FinishRecovery;
	}

	bool HasEmptyStartPayload(
		const Fdemo_mapShanmenSpiritEvasionCommand& Command)
	{
		return !Command.GetAction().IsValid()
			&& !Command.GetDefinition().IsValid()
			&& !Command.GetPolicy().IsValid()
			&& !Command.GetTrajectory().IsValid()
			&& Command.GetCandidateDirection().IsNearlyZero();
	}

	bool ValidateRoute(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const Fdemo_mapShanmenSpiritEvasionCommand& Command,
		Fdemo_mapShanmenSpiritEvasionCommandResult& OutResult)
	{
		OutResult.Kind = Command.GetKind();
		OutResult.ActivationId = Command.GetActivationId();
		if (!Command.IsValid())
		{
			OutResult.Diagnostic = TEXT("Spirit Evasion command is invalid.");
			return false;
		}
		if (!::IsValid(Component))
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionCommandStatus::
				ComponentUnavailable;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion component is unavailable.");
			return false;
		}
		if (!::IsValid(Owner) || Component->GetOwner() != Owner)
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionCommandStatus::
				ComponentOwnerMismatch;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion component does not belong to the routed player.");
			return false;
		}
		if (Command.GetKind()
			!= Edemo_mapShanmenSpiritEvasionCommandKind::Start)
		{
			return true;
		}
		if (!Coordinator.IsReady())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionCommandStatus::CoordinatorNotReady;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion start requires one active combat Run.");
			return false;
		}

		const FShanmenCombatActionSnapshot& Action = Command.GetAction();
		if (Action.GetRunId() != Coordinator.GetRunId())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionCommandStatus::RunMismatch;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion action does not belong to the active Run.");
			return false;
		}
		if (Action.GetSourceEntityId() != Coordinator.GetPlayerEntityId())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionCommandStatus::SourceMismatch;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion action does not name the active player entity.");
			return false;
		}
		FGuid ResolvedSourceEntityId;
		if (!Coordinator.GetEntityRegistry().TryResolveObject(
				Action.GetRunId(), Owner, INDEX_NONE, ResolvedSourceEntityId)
			|| ResolvedSourceEntityId != Action.GetSourceEntityId())
		{
			OutResult.Status =
				Edemo_mapShanmenSpiritEvasionCommandStatus::SourceMismatch;
			OutResult.Diagnostic =
				TEXT("Spirit Evasion owner is not the action's registered entity.");
			return false;
		}
		return true;
	}

	Fdemo_mapShanmenSpiritEvasionCommandResult RouteControlSignal(
		Udemo_mapShanmenSpiritEvasionComponent& Component,
		const Fdemo_mapShanmenSpiritEvasionCommand& Command,
		Fdemo_mapShanmenSpiritEvasionCommandResult Result)
	{
		switch (Command.GetKind())
		{
		case Edemo_mapShanmenSpiritEvasionCommandKind::Cancel:
			Result.Step = Component.TryCancel();
			break;
		case Edemo_mapShanmenSpiritEvasionCommandKind::Interrupt:
			Result.Step = Component.TryInterrupt();
			break;
		case Edemo_mapShanmenSpiritEvasionCommandKind::FinishRecovery:
			Result.Step = Component.TryFinishRecovery();
			break;
		case Edemo_mapShanmenSpiritEvasionCommandKind::Start:
			Result.Diagnostic =
				TEXT("Spirit Evasion start was dispatched as a control signal.");
			return Result;
		}
		Result.Status = Result.Step.IsSuccess()
			? Edemo_mapShanmenSpiritEvasionCommandStatus::Applied
			: Edemo_mapShanmenSpiritEvasionCommandStatus::ComponentRejected;
		Result.Diagnostic = Result.Step.IsSuccess()
			? TEXT("Spirit Evasion lifecycle command was applied.")
			: TEXT("Spirit Evasion component rejected the lifecycle command.");
		return Result;
	}
}

bool Fdemo_mapShanmenSpiritEvasionCommand::TryCaptureStart(
	const FShanmenCombatActionSnapshot& RequestedAction,
	const FShanmenSpiritEvasionDefinition& RequestedDefinition,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& RequestedPolicy,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& RequestedTrajectory,
	const FVector& RequestedDirection,
	Fdemo_mapShanmenSpiritEvasionCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSpiritEvasionCommand();
	if (!RequestedAction.IsValid()
		|| !RequestedDefinition.IsValid()
		|| !RequestedPolicy.IsValid()
		|| !RequestedTrajectory.IsValid()
		|| RequestedAction.GetActionDefinitionId()
			!= FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
		|| RequestedDefinition.GetActionDefinitionId()
			!= RequestedAction.GetActionDefinitionId()
		|| !IsFiniteVector(RequestedDirection)
		|| RequestedDirection.IsNearlyZero())
	{
		return false;
	}

	OutCommand.Kind = Edemo_mapShanmenSpiritEvasionCommandKind::Start;
	OutCommand.Action = RequestedAction;
	OutCommand.Definition = RequestedDefinition;
	OutCommand.Policy = RequestedPolicy;
	OutCommand.Trajectory = RequestedTrajectory;
	OutCommand.CandidateDirection = RequestedDirection.GetSafeNormal();
	if (!OutCommand.IsValid())
	{
		OutCommand = Fdemo_mapShanmenSpiritEvasionCommand();
		return false;
	}
	return true;
}

Fdemo_mapShanmenSpiritEvasionCommand
Fdemo_mapShanmenSpiritEvasionCommand::MakeSignal(
	Edemo_mapShanmenSpiritEvasionCommandKind RequestedKind)
{
	Fdemo_mapShanmenSpiritEvasionCommand Command;
	Command.Kind = RequestedKind;
	return Command;
}

Fdemo_mapShanmenSpiritEvasionCommand
Fdemo_mapShanmenSpiritEvasionCommand::MakeCancel()
{
	return MakeSignal(Edemo_mapShanmenSpiritEvasionCommandKind::Cancel);
}

Fdemo_mapShanmenSpiritEvasionCommand
Fdemo_mapShanmenSpiritEvasionCommand::MakeInterrupt()
{
	return MakeSignal(Edemo_mapShanmenSpiritEvasionCommandKind::Interrupt);
}

Fdemo_mapShanmenSpiritEvasionCommand
Fdemo_mapShanmenSpiritEvasionCommand::MakeFinishRecovery()
{
	return MakeSignal(
		Edemo_mapShanmenSpiritEvasionCommandKind::FinishRecovery);
}

bool Fdemo_mapShanmenSpiritEvasionCommand::IsValid() const
{
	if (Kind == Edemo_mapShanmenSpiritEvasionCommandKind::Start)
	{
		return Action.IsValid()
			&& Definition.IsValid()
			&& Policy.IsValid()
			&& Trajectory.IsValid()
			&& Action.GetActionDefinitionId()
				== FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
			&& Definition.GetActionDefinitionId()
				== Action.GetActionDefinitionId()
			&& IsFiniteVector(CandidateDirection)
			&& CandidateDirection.IsNormalized();
	}
	return IsControlSignal(Kind) && HasEmptyStartPayload(*this);
}

bool Fdemo_mapShanmenSpiritEvasionInstallationResult::IsSuccess() const
{
	return (Status
			== Edemo_mapShanmenSpiritEvasionInstallationStatus::Installed
		|| Status
			== Edemo_mapShanmenSpiritEvasionInstallationStatus::
			AlreadyInstalled)
		&& ::IsValid(Component)
		&& ExistingComponentCount <= 1;
}

bool Fdemo_mapShanmenSpiritEvasionCommandResult::IsAccepted() const
{
	if (Status != Edemo_mapShanmenSpiritEvasionCommandStatus::Applied)
	{
		return false;
	}
	return Kind == Edemo_mapShanmenSpiritEvasionCommandKind::Start
		? ActivationId.IsValid() && Start.IsSuccess()
		: !ActivationId.IsValid() && Step.IsSuccess();
}

Fdemo_mapShanmenSpiritEvasionInstallationResult
Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(ACharacter* Owner)
{
	Fdemo_mapShanmenSpiritEvasionInstallationResult Result;
	if (!::IsValid(Owner))
	{
		return Result;
	}

	TArray<Udemo_mapShanmenSpiritEvasionComponent*> Existing;
	Owner->GetComponents<Udemo_mapShanmenSpiritEvasionComponent>(Existing);
	Result.ExistingComponentCount = Existing.Num();
	if (Existing.Num() > 1)
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionInstallationStatus::
			DuplicateComponents;
		return Result;
	}

	if (Existing.Num() == 1)
	{
		Result.Component = Existing[0];
		if (!Result.Component->IsRegistered())
		{
			Result.Component->RegisterComponent();
		}
		Result.Status = Result.Component->IsRegistered()
			? Edemo_mapShanmenSpiritEvasionInstallationStatus::AlreadyInstalled
			: Edemo_mapShanmenSpiritEvasionInstallationStatus::RegistrationFailed;
		return Result;
	}

	Result.Component = NewObject<Udemo_mapShanmenSpiritEvasionComponent>(
		Owner, TEXT("RuntimeShanmenSpiritEvasion"));
	if (!Result.Component)
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionInstallationStatus::CreationFailed;
		return Result;
	}
	Owner->AddInstanceComponent(Result.Component);
	Result.Component->RegisterComponent();
	if (!Result.Component->IsRegistered())
	{
		Owner->RemoveInstanceComponent(Result.Component);
		Result.Component = nullptr;
		Result.Status =
			Edemo_mapShanmenSpiritEvasionInstallationStatus::RegistrationFailed;
		return Result;
	}
	Result.Status = Edemo_mapShanmenSpiritEvasionInstallationStatus::Installed;
	return Result;
}

Fdemo_mapShanmenSpiritEvasionCommandResult
Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
	Udemo_mapShanmenSpiritEvasionComponent* Component,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	ACharacter* Owner,
	const Fdemo_mapShanmenSpiritEvasionCommand& Command)
{
	Fdemo_mapShanmenSpiritEvasionCommandResult Result;
	if (!ValidateRoute(Component, Coordinator, Owner, Command, Result))
	{
		return Result;
	}
	if (Command.GetKind()
		!= Edemo_mapShanmenSpiritEvasionCommandKind::Start)
	{
		return RouteControlSignal(*Component, Command, MoveTemp(Result));
	}

	Result.Start = Component->TryStart(
		Command.GetAction(),
		Command.GetDefinition(),
		Command.GetPolicy(),
		Command.GetTrajectory(),
		Command.GetCandidateDirection());
	Result.Status = Result.Start.IsSuccess()
		? Edemo_mapShanmenSpiritEvasionCommandStatus::Applied
		: Edemo_mapShanmenSpiritEvasionCommandStatus::ComponentRejected;
	Result.Diagnostic = Result.Start.IsSuccess()
		? TEXT("Spirit Evasion start command was applied.")
		: TEXT("Spirit Evasion component rejected the start command.");
	return Result;
}

#if WITH_DEV_AUTOMATION_TESTS
Fdemo_mapShanmenSpiritEvasionCommandResult
Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRouteAtForAutomation(
	Udemo_mapShanmenSpiritEvasionComponent* Component,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	ACharacter* Owner,
	const Fdemo_mapShanmenSpiritEvasionCommand& Command,
	double StartTimeSeconds,
	Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort)
{
	if (Command.GetKind()
		!= Edemo_mapShanmenSpiritEvasionCommandKind::Start)
	{
		return TryRoute(Component, Coordinator, Owner, Command);
	}

	Fdemo_mapShanmenSpiritEvasionCommandResult Result;
	if (!ValidateRoute(Component, Coordinator, Owner, Command, Result))
	{
		return Result;
	}
	Result.Start = Component->TryStartAtForAutomation(
		Command.GetAction(),
		Command.GetDefinition(),
		Command.GetPolicy(),
		Command.GetTrajectory(),
		Command.GetCandidateDirection(),
		StartTimeSeconds,
		PreflightPort);
	Result.Status = Result.Start.IsSuccess()
		? Edemo_mapShanmenSpiritEvasionCommandStatus::Applied
		: Edemo_mapShanmenSpiritEvasionCommandStatus::ComponentRejected;
	Result.Diagnostic = Result.Start.IsSuccess()
		? TEXT("Spirit Evasion automation start command was applied.")
		: TEXT("Spirit Evasion component rejected the automation start command.");
	return Result;
}
#endif
