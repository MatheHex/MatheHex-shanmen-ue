#include "demo_mapShanmenSwordQiProductSession.h"

#include "demo_mapCombatRunCoordinator.h"
#include "GameFramework/Actor.h"

namespace
{
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

	bool GateMatches(
		const Fdemo_mapShanmenPlayerActionGateResult& Gate,
		const Fdemo_mapShanmenSwordQiLaunchCommand& Command)
	{
		return Gate.IsValid()
			&& Gate.Arbitration.RequestedAction
				== Edemo_mapShanmenPlayerActionKind::SwordQi
			&& Gate.Arbitration.RunId == Command.GetRunId()
			&& Gate.Arbitration.PlayerEntityId
				== Command.GetAction().GetSourceEntityId();
	}

	void DestroyCarrier(
		const Fdemo_mapShanmenSwordQiSpawnResult& Spawn)
	{
		if (Ademo_mapShanmenSwordQiProjectile* Projectile =
			Spawn.Projectile.Get())
		{
			Projectile->Destroy();
		}
	}
}

bool Fdemo_mapShanmenSwordQiProductRouteResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSwordQiProductRouteStatus::Applied
		&& CommandId.IsValid()
		&& RunId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& LaunchId.IsValid()
		&& ActionGate.IsAuthorized()
		&& ActionGate.Arbitration.RequestedAction
			== Edemo_mapShanmenPlayerActionKind::SwordQi
		&& ActionGate.Arbitration.RunId == RunId
		&& IsActionStartup(Startup, CommandId)
		&& IsActionActive(Active, CommandId)
		&& HostStart.IsStarted()
		&& HostStart.LaunchId == LaunchId
		&& !Interruption.IsValid();
}

bool Fdemo_mapShanmenSwordQiProductRouteResult::IsTerminal() const
{
	if (IsAccepted())
	{
		return true;
	}
	if (!CommandId.IsValid()
		|| !RunId.IsValid()
		|| !SourceItemInstanceId.IsValid()
		|| LaunchId.IsValid())
	{
		return false;
	}
	if (Status ==
		Edemo_mapShanmenSwordQiProductRouteStatus::ActionGateRejected)
	{
		return ActionGate.IsValid()
			&& !ActionGate.IsAuthorized()
			&& !Startup.IsValid()
			&& !Active.IsValid()
			&& !Interruption.IsValid();
	}
	if (!ActionGate.IsAuthorized())
	{
		return false;
	}
	if (Status ==
		Edemo_mapShanmenSwordQiProductRouteStatus::CarrierSpawnRejected
		|| Status ==
			Edemo_mapShanmenSwordQiProductRouteStatus::ExecutionRejected)
	{
		return !Startup.IsValid()
			&& !Active.IsValid()
			&& !Interruption.IsValid();
	}
	if (Status ==
		Edemo_mapShanmenSwordQiProductRouteStatus::ActionStartRejected)
	{
		return !Startup.IsValid()
			&& !Active.IsValid()
			&& !Interruption.IsValid();
	}
	if (Status ==
		Edemo_mapShanmenSwordQiProductRouteStatus::ActionCommitRejected)
	{
		return IsActionStartup(Startup, CommandId)
			&& !Active.IsValid()
			&& !Interruption.IsValid();
	}
	if (Status ==
		Edemo_mapShanmenSwordQiProductRouteStatus::
			LaunchRejectedInterrupted)
	{
		return IsActionStartup(Startup, CommandId)
			&& IsActionActive(Active, CommandId)
			&& Interruption.IsValid()
			&& Interruption.GetActivationId() == CommandId
			&& Interruption.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted;
	}
	return Status ==
		Edemo_mapShanmenSwordQiProductRouteStatus::LaunchRecoveryRejected
		&& IsActionStartup(Startup, CommandId)
		&& IsActionActive(Active, CommandId);
}

Fdemo_mapShanmenSwordQiProductRouteResult
Fdemo_mapShanmenSwordQiProductSession::TryRoute(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* SourceActor,
	const Fdemo_mapShanmenSwordQiLaunchCommand& Command,
	TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()> AuthorizeAction)
{
	Fdemo_mapShanmenSwordQiProductRouteResult Result;
	Result.CommandId = Command.GetCommandId();
	Result.RunId = Command.GetRunId();
	Result.SourceItemInstanceId =
		Command.GetAction().GetSourceItemInstanceId();
	if (!Coordinator.IsReady())
	{
		Result.Diagnostic =
			TEXT("Sword Qi route requires one ready combat Run.");
		return Result;
	}
	if (!Command.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::CommandInvalid;
		Result.Diagnostic = TEXT("Sword Qi launch command is invalid.");
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::SessionInvalid;
		Result.Diagnostic = TEXT("Sword Qi product Session is invalid.");
		return Result;
	}
	if (Coordinator.GetRunId() != Command.GetRunId())
	{
		Result.Status = Edemo_mapShanmenSwordQiProductRouteStatus::RunMismatch;
		Result.Diagnostic =
			TEXT("Sword Qi command does not belong to this combat Run.");
		return Result;
	}
	if (RunId.IsValid() && RunId != Command.GetRunId())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::SessionRunMismatch;
		Result.Diagnostic =
			TEXT("Sword Qi product Session belongs to another combat Run.");
		return Result;
	}

	if (const FProcessedCommand* Existing =
		ProcessedCommands.Find(Command.GetCommandId()))
	{
		if (!Existing->Command.Matches(Command))
		{
			Result.Status =
				Edemo_mapShanmenSwordQiProductRouteStatus::CommandIdConflict;
			Result.Diagnostic =
				TEXT("Sword Qi ActivationId was reused with another payload.");
			return Result;
		}
		Result = Existing->Result;
		Result.bReplay = true;
		Result.Diagnostic =
			TEXT("Sword Qi command returned terminal receipts without authorization or Actor I/O.");
		return Result;
	}

	if (!CoordinatorMatches(Coordinator, Command.GetAction(), SourceActor))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::SourceMismatch;
		Result.Diagnostic =
			TEXT("Sword Qi source Actor is not the action's registered player entity.");
		return Result;
	}
	if (!IsEmpty())
	{
		Result.Status = Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy;
		Result.Diagnostic =
			TEXT("Sword Qi terminal proof must be retired before another command starts.");
		return Result;
	}

	Result.ActionGate = AuthorizeAction();
	if (!GateMatches(Result.ActionGate, Command))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::ActionGateInvalid;
		Result.Diagnostic =
			TEXT("Sword Qi action authorization did not match the frozen command.");
		return Result;
	}
	if (!Result.ActionGate.IsAuthorized())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::ActionGateRejected;
		Result.Diagnostic = Result.ActionGate.Diagnostic;
		RecordTerminal(Command, Result);
		return Result;
	}

	Result.Spawn = Fdemo_mapShanmenSwordQiRunHost::SpawnStagedCarrier(
		World,
		ProjectileClass,
		SourceActor,
		Command.GetOrigin());
	if (!Result.Spawn.IsSpawned())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::CarrierSpawnRejected;
		Result.Diagnostic =
			TEXT("Sword Qi could not create one collision-inert carrier.");
		RecordTerminal(Command, Result);
		return Result;
	}

	FShanmenSwordQiExecution Execution;
	if (!FShanmenSwordQiExecution::TryCreate(
			Command.GetAction(),
			Command.GetDefinition(),
			Command.GetOffense(),
			Execution))
	{
		DestroyCarrier(Result.Spawn);
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::ExecutionRejected;
		Result.Diagnostic =
			TEXT("Sword Qi execution rejected the frozen product payload.");
		RecordTerminal(Command, Result);
		return Result;
	}

	FShanmenActionOrchestrator ActionRuntime;
	if (!FShanmenActionOrchestrator::TryStart(
			Command.GetAction(), ActionRuntime, Result.Startup))
	{
		DestroyCarrier(Result.Spawn);
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::ActionStartRejected;
		Result.Diagnostic =
			TEXT("Sword Qi action runtime rejected the frozen action.");
		RecordTerminal(Command, Result);
		return Result;
	}
	if (!ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Result.Active))
	{
		DestroyCarrier(Result.Spawn);
		Result.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::ActionCommitRejected;
		Result.Diagnostic =
			TEXT("Sword Qi action rejected the Startup-to-Active commit.");
		RecordTerminal(Command, Result);
		return Result;
	}

	Ademo_mapShanmenSwordQiProjectile* Projectile = Result.Spawn.Projectile.Get();
	Result.HostStart = Host.TryLaunchCarrier(
		ActionRuntime,
		Execution,
		*Projectile,
		Coordinator,
		SourceActor,
		Command.GetOrigin(),
		Command.GetAimDirection(),
		true);
	if (!Result.HostStart.IsStarted())
	{
		DestroyCarrier(Result.Spawn);
		FShanmenActionOrchestrator InterruptedRuntime = ActionRuntime;
		if (InterruptedRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Result.Interruption))
		{
			Result.Status = Edemo_mapShanmenSwordQiProductRouteStatus::
				LaunchRejectedInterrupted;
			Result.Diagnostic =
				TEXT("Sword Qi launch failed after Active; the action was interrupted and the inert carrier removed.");
		}
		else
		{
			Result.Status = Edemo_mapShanmenSwordQiProductRouteStatus::
				LaunchRecoveryRejected;
			Result.Diagnostic =
				TEXT("Sword Qi launch and local post-commit interruption both failed closed.");
		}
		RecordTerminal(Command, Result);
		return Result;
	}

	Result.Status = Edemo_mapShanmenSwordQiProductRouteStatus::Applied;
	Result.LaunchId = Result.HostStart.LaunchId;
	Result.Diagnostic =
		TEXT("Sword Qi authorization, Active commit and physical flight were applied exactly once.");
	RecordTerminal(Command, Result);
	return Result;
}

bool Fdemo_mapShanmenSwordQiProductSession::TryInterrupt()
{
	return IsValid() && Host.TryInterrupt();
}

bool Fdemo_mapShanmenSwordQiProductSession::TryExpireRange()
{
	return IsValid() && Host.TryExpireRange();
}

bool Fdemo_mapShanmenSwordQiProductSession::TryRetireTerminal(
	Fdemo_mapShanmenSwordQiTerminalReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSwordQiTerminalReceipt();
	if (!IsValid() || !Host.IsTerminal())
	{
		return false;
	}
	OutReceipt = Host.GetTerminalReceipt();
	if (!OutReceipt.IsValid() || !Host.Reset())
	{
		OutReceipt = Fdemo_mapShanmenSwordQiTerminalReceipt();
		return false;
	}
	return IsValid() && IsEmpty();
}

bool Fdemo_mapShanmenSwordQiProductSession::TryAppendOccupancy(
	Fdemo_mapShanmenPlayerActionOccupancySnapshot& InOutSnapshot) const
{
	if (!IsValid())
	{
		InOutSnapshot.Invalidate();
		return false;
	}
	if (!Host.IsInFlight())
	{
		return true;
	}
	return InOutSnapshot.TryRegisterClaim(
		Edemo_mapShanmenPlayerActionKind::SwordQi,
		GetOccupancyOwnerId(),
		Edemo_mapShanmenPlayerActionClaimPreemption::None);
}

bool Fdemo_mapShanmenSwordQiProductSession::Reset()
{
	if (Host.IsInFlight())
	{
		return false;
	}
	if (Host.IsTerminal() && !Host.Reset())
	{
		return false;
	}
	RunId.Invalidate();
	ProcessedCommands.Reset();
	return IsValid() && IsEmpty();
}

bool Fdemo_mapShanmenSwordQiProductSession::IsValid() const
{
	if (ProcessedCommands.IsEmpty() != !RunId.IsValid())
	{
		return false;
	}
	for (const TPair<FGuid, FProcessedCommand>& Pair : ProcessedCommands)
	{
		if (!Pair.Key.IsValid()
			|| Pair.Key != Pair.Value.Command.GetCommandId()
			|| !Pair.Value.Command.IsValid()
			|| Pair.Value.Command.GetRunId() != RunId
			|| Pair.Value.Result.bReplay
			|| Pair.Value.Result.CommandId != Pair.Key
			|| Pair.Value.Result.RunId != RunId
			|| !Pair.Value.Result.IsTerminal())
		{
			return false;
		}
	}
	if (Host.GetState() == Edemo_mapShanmenSwordQiHostState::Empty)
	{
		return true;
	}
	if (!Host.IsValid())
	{
		return false;
	}
	const FShanmenCombatActionSnapshot& Action =
		Host.GetActionRuntime().GetAction();
	const FProcessedCommand* Active =
		ProcessedCommands.Find(Action.GetActivationId());
	return RunId.IsValid()
		&& Action.GetRunId() == RunId
		&& Active
		&& Active->Result.IsAccepted();
}

FGuid Fdemo_mapShanmenSwordQiProductSession::GetOccupancyOwnerId() const
{
	return Host.IsInFlight()
		? Host.GetActionRuntime().GetAction().GetActivationId()
		: FGuid();
}

void Fdemo_mapShanmenSwordQiProductSession::RecordTerminal(
	const Fdemo_mapShanmenSwordQiLaunchCommand& Command,
	const Fdemo_mapShanmenSwordQiProductRouteResult& Result)
{
	if (!Command.IsValid()
		|| Result.bReplay
		|| !Result.IsTerminal()
		|| ProcessedCommands.Contains(Command.GetCommandId()))
	{
		return;
	}
	if (!RunId.IsValid())
	{
		RunId = Command.GetRunId();
	}
	if (RunId != Command.GetRunId())
	{
		return;
	}
	FProcessedCommand Record;
	Record.Command = Command;
	Record.Result = Result;
	ProcessedCommands.Add(Command.GetCommandId(), MoveTemp(Record));
}
