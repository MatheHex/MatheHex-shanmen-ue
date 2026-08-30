#include "demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h"

namespace
{
	bool IsLifecycleCommandKind(
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind Kind)
	{
		return Kind
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
				ExecuteStep
			|| Kind
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
					PrepareTerminal
			|| Kind
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
					SealAndEnd;
	}

	bool IsEmptyStepRequest(
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request)
	{
		return !Request.RequestId.IsValid()
			&& !Request.ExpectedIntentId.IsValid();
	}
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommand::TryCaptureStep(
	const FGuid& RequestedCommandId,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenFormationInfluenceLifecycleCommand();
	if (!RequestedCommandId.IsValid()
		|| !RequestedCorrelation.IsValid()
		|| !Request.IsValid()
		|| RequestedCommandId != Request.RequestId)
	{
		return false;
	}
	OutCommand.CommandId = RequestedCommandId;
	OutCommand.Correlation = RequestedCorrelation;
	OutCommand.Kind =
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep;
	OutCommand.StepRequest = Request;
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommand::
TryCapturePrepareTerminal(
	const FGuid& RequestedCommandId,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand)
{
	return TryCaptureWithoutStep(
		RequestedCommandId,
		RequestedCorrelation,
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::PrepareTerminal,
		OutCommand);
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommand::TryCaptureSealAndEnd(
	const FGuid& RequestedCommandId,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand)
{
	return TryCaptureWithoutStep(
		RequestedCommandId,
		RequestedCorrelation,
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::SealAndEnd,
		OutCommand);
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommand::
TryCaptureWithoutStep(
	const FGuid& RequestedCommandId,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	Edemo_mapShanmenFormationInfluenceLifecycleCommandKind RequestedKind,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenFormationInfluenceLifecycleCommand();
	if (!RequestedCommandId.IsValid()
		|| !RequestedCorrelation.IsValid()
		|| (RequestedKind
			!= Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
				PrepareTerminal
			&& RequestedKind
				!= Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
					SealAndEnd))
	{
		return false;
	}
	OutCommand.CommandId = RequestedCommandId;
	OutCommand.Correlation = RequestedCorrelation;
	OutCommand.Kind = RequestedKind;
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommand::IsValid() const
{
	if (!CommandId.IsValid()
		|| !Correlation.IsValid()
		|| !IsLifecycleCommandKind(Kind))
	{
		return false;
	}
	return Kind
		== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep
		? StepRequest.IsValid() && CommandId == StepRequest.RequestId
		: IsEmptyStepRequest(StepRequest);
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommand::Matches(
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| CommandId != Other.CommandId
		|| Correlation != Other.Correlation
		|| Kind != Other.Kind)
	{
		return false;
	}
	return Kind
		== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep
		? StepRequest.Matches(Other.StepRequest)
		: true;
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult::IsSuccess() const
{
	return Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::Applied
		&& CommandId.IsValid()
		&& IsLifecycleCommandKind(Kind)
		&& Lifecycle.IsSuccess();
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult::
IsDurableRecord() const
{
	if (!CommandId.IsValid()
		|| !IsLifecycleCommandKind(Kind)
		|| bReplay
		|| bRecoveryAttempted
		|| !bRouterStateCommitted)
	{
		return false;
	}
	if (IsSuccess())
	{
		return true;
	}
	if (Status
		!= Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
			LifecycleRejected
		|| !Lifecycle.bCoordinatorStateCommitted)
	{
		return false;
	}
	return Lifecycle.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepRejected
		|| Lifecycle.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected;
}

Fdemo_mapShanmenFormationInfluenceLifecycleResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter::Execute(
	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator& TargetCoordinator,
	UWorld* World,
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command)
{
	switch (Command.GetKind())
	{
	case Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep:
		return TargetCoordinator.TryExecuteStep(
			Host, Command.GetCorrelation(), Command.GetStepRequest());
	case Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
		PrepareTerminal:
		return TargetCoordinator.TryPrepareTerminal(
			Host, Command.GetCorrelation());
	case Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::SealAndEnd:
		return TargetCoordinator.TrySealAndEnd(
			World, Host, Command.GetCorrelation());
	default:
		return Fdemo_mapShanmenFormationInfluenceLifecycleResult();
	}
}

Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter::TryRoute(
	UWorld* World,
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command)
{
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Result;
	Result.CommandId = Command.GetCommandId();
	Result.Kind = Command.GetKind();
	if (!Command.IsValid())
	{
		Result.Diagnostic =
			TEXT("Influence lifecycle routing requires one valid typed command.");
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				RouterInvalid;
		Result.Diagnostic =
			TEXT("Influence lifecycle command Router is internally inconsistent.");
		return Result;
	}
	if (!Host.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected;
		Result.Lifecycle.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::HostInvalid;
		Result.Diagnostic =
			TEXT("Influence lifecycle command requires one valid ProductHost.");
		Result.Lifecycle.Diagnostic = Result.Diagnostic;
		return Result;
	}
	if (Command.GetCorrelation() != Host.GetSession().GetCorrelation())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected;
		Result.Lifecycle.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::
				CorrelationMismatch;
		Result.Diagnostic =
			TEXT("Influence lifecycle command rejected a stale or foreign Run correlation.");
		Result.Lifecycle.Diagnostic = Result.Diagnostic;
		return Result;
	}
	if (!Host.HasInfluenceAuthority())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected;
		Result.Lifecycle.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::
				LedgerUnavailable;
		Result.Diagnostic =
			TEXT("Influence lifecycle command requires one Host-owned ledger.");
		Result.Lifecycle.Diagnostic = Result.Diagnostic;
		return Result;
	}
	if (Coordinator.IsBound()
		&& (Coordinator.GetBoundCorrelation() != Command.GetCorrelation()
			|| Coordinator.GetBoundLedgerId()
				!= Host.GetInfluenceLedger().GetLedgerId()))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected;
		Result.Lifecycle.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::
				BindingConflict;
		Result.Diagnostic =
			TEXT("Influence lifecycle command Router is bound to different Host evidence.");
		Result.Lifecycle.Diagnostic = Result.Diagnostic;
		return Result;
	}

	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const FRecord& Existing = Records[Index];
		if (Existing.Command.GetCommandId() != Command.GetCommandId())
		{
			continue;
		}
		if (!Existing.Command.Matches(Command))
		{
			Result.Status =
				Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					CommandIdConflict;
			Result.Diagnostic =
				TEXT("Influence lifecycle CommandId was reused with another payload.");
			return Result;
		}
		if (Command.GetKind()
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::
					SealAndEnd
			&& Existing.Result.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected)
		{
			return TryRecoverEnd(World, Host, Command, Index);
		}
		Result = Existing.Result;
		Result.bReplay = true;
		Result.bRouterStateCommitted = false;
		Result.Diagnostic = Existing.Result.IsSuccess()
			? TEXT("Influence lifecycle command replayed accepted receipts.")
			: TEXT("Influence lifecycle command replayed its durable rejection.");
		return Result;
	}
	if (Command.GetKind()
		!= Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep)
	{
		for (const FRecord& Existing : Records)
		{
			if (Existing.Command.GetKind() == Command.GetKind())
			{
				Result.Status =
					Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
						OperationIdentityConflict;
				Result.Diagnostic =
					TEXT("Influence lifecycle terminal operation already has a different CommandId.");
				return Result;
			}
		}
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Candidate = *this;
	Result.Lifecycle = Execute(Candidate.Coordinator, World, Host, Command);
	Result.Status = Result.Lifecycle.IsSuccess()
		? Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::Applied
		: Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
			LifecycleRejected;
	Result.Diagnostic = Result.Lifecycle.Diagnostic;
	if (!Result.Lifecycle.bCoordinatorStateCommitted)
	{
		return Result;
	}

	Result.bRouterStateCommitted = true;
	FRecord Record;
	Record.Command = Command;
	Record.Result = Result;
	Candidate.Records.Add(MoveTemp(Record));
	if (!Result.IsDurableRecord() || !Candidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::StateInvalid;
		Result.bRouterStateCommitted = false;
		Result.Diagnostic =
			TEXT("Influence lifecycle command produced invalid staged state.");
		return Result;
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter::TryRecoverEnd(
	UWorld* World,
	Fdemo_mapShanmenFormationProductHost& Host,
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command,
	int32 RecordIndex)
{
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Result;
	Result.CommandId = Command.GetCommandId();
	Result.Kind = Command.GetKind();
	Result.bRecoveryAttempted = true;
	if (!Records.IsValidIndex(RecordIndex))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Influence lifecycle recovery record index is invalid.");
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Candidate = *this;
	Result.Lifecycle = Execute(Candidate.Coordinator, World, Host, Command);
	Result.Status = Result.Lifecycle.IsSuccess()
		? Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::Applied
		: Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
			LifecycleRejected;
	Result.Diagnostic = Result.Lifecycle.Diagnostic;
	if (!Result.Lifecycle.bCoordinatorStateCommitted)
	{
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Stored = Result;
	Stored.bRecoveryAttempted = false;
	Stored.bRouterStateCommitted = true;
	Candidate.Records[RecordIndex].Result = Stored;
	if (!Stored.IsDurableRecord() || !Candidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Influence lifecycle recovery produced invalid staged state.");
		return Result;
	}

	*this = MoveTemp(Candidate);
	Result.bRouterStateCommitted = true;
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter::IsValid() const
{
	if (!Coordinator.IsValid())
	{
		return false;
	}
	if (Records.IsEmpty())
	{
		return !Coordinator.IsBound();
	}
	if (!Coordinator.IsBound())
	{
		return false;
	}

	TSet<FGuid> CommandIds;
	for (const FRecord& Record : Records)
	{
		if (!Record.Command.IsValid()
			|| !Record.Result.IsDurableRecord()
			|| Record.Result.CommandId != Record.Command.GetCommandId()
			|| Record.Result.Kind != Record.Command.GetKind()
			|| Record.Command.GetCorrelation()
				!= Coordinator.GetBoundCorrelation()
			|| CommandIds.Contains(Record.Command.GetCommandId()))
		{
			return false;
		}
		CommandIds.Add(Record.Command.GetCommandId());
	}
	return true;
}
