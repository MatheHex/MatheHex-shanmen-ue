#include "demo_mapShanmenFormationScatterWorldPublicationCommandHost.h"

#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using ECommandOperation =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation;
	using ECommandStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandStatus;
	using ESessionStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionStatus;
	using ESessionState =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionState;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsCommandOperation(const ECommandOperation Operation)
	{
		return Operation == ECommandOperation::Publish
			|| Operation == ECommandOperation::Cancel
			|| Operation == ECommandOperation::End;
	}

	bool IsSessionSuccess(
		const ECommandOperation Operation,
		const Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult&
			Session)
	{
		return Operation == ECommandOperation::Publish
			? Session.IsPublicationSuccess()
			: (Operation == ECommandOperation::Cancel
				|| Operation == ECommandOperation::End)
				&& Session.IsTeardownSuccess();
	}
}

FGuid Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding::
BuildHostId(
	const FGuid& InSessionId,
	const FGuid& InHandoffEvidenceId,
	const FGuid& InDeploymentId,
	const FString& InActorClassPath)
{
	if (!InSessionId.IsValid() || !InHandoffEvidenceId.IsValid()
		|| !InDeploymentId.IsValid() || InActorClassPath.IsEmpty())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPublicationCommandHost.r1"),
		{
			GuidDigits(InSessionId),
			GuidDigits(InHandoffEvidenceId),
			GuidDigits(InDeploymentId),
			InActorClassPath
		});
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding::IsValid()
	const
{
	return HostId.IsValid() && SessionId.IsValid()
		&& HandoffEvidenceId.IsValid() && DeploymentId.IsValid()
		&& !ActorClassPath.IsEmpty()
		&& HostId == BuildHostId(
			SessionId, HandoffEvidenceId, DeploymentId, ActorClassPath);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding::Matches(
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& HostId == Other.HostId
		&& SessionId == Other.SessionId
		&& HandoffEvidenceId == Other.HandoffEvidenceId
		&& DeploymentId == Other.DeploymentId
		&& ActorClassPath == Other.ActorClassPath;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
TryCapturePublish(
	const FGuid& InCommandId,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
		InBinding,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand)
{
	return TryCapture(
		InCommandId, InBinding, ECommandOperation::Publish, OutCommand);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommand::TryCaptureCancel(
	const FGuid& InCommandId,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
		InBinding,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand)
{
	return TryCapture(
		InCommandId, InBinding, ECommandOperation::Cancel, OutCommand);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommand::TryCaptureEnd(
	const FGuid& InCommandId,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
		InBinding,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand)
{
	return TryCapture(
		InCommandId, InBinding, ECommandOperation::End, OutCommand);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommand::TryCapture(
	const FGuid& InCommandId,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
		InBinding,
	const ECommandOperation InOperation,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenFormationScatterWorldPublicationCommand();
	if (!InCommandId.IsValid() || !InBinding.IsValid()
		|| !IsCommandOperation(InOperation))
	{
		return false;
	}
	OutCommand.CommandId = InCommandId;
	OutCommand.Binding = InBinding;
	OutCommand.Operation = InOperation;
	return OutCommand.IsValid();
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommand::IsValid() const
{
	return CommandId.IsValid() && Binding.IsValid()
		&& IsCommandOperation(Operation);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommand::Matches(
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Other) const
{
	return IsValid() && Other.IsValid()
		&& CommandId == Other.CommandId
		&& Binding.Matches(Other.Binding)
		&& Operation == Other.Operation;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult::
IsSuccess() const
{
	return (Status == ECommandStatus::Applied
			|| Status == ECommandStatus::Recovered
			|| Status == ECommandStatus::Replayed)
		&& CommandId.IsValid() && IsCommandOperation(Operation)
		&& IsSessionSuccess(Operation, Session);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult::IsValid()
	const
{
	if (Status == ECommandStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (IsSuccess())
	{
		return true;
	}
	if (Status == ECommandStatus::SessionRejected)
	{
		return CommandId.IsValid() && IsCommandOperation(Operation)
			&& Session.IsValid()
			&& !IsSessionSuccess(Operation, Session);
	}
	return !Session.IsPublicationSuccess() && !Session.IsTeardownSuccess();
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult::
IsDurableRecord() const
{
	if (!IsValid() || bReplay || bRecoveryAttempted
		|| !bHostStateCommitted)
	{
		return false;
	}
	return Status == ECommandStatus::Applied
		|| Status == ECommandStatus::SessionRejected;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord::IsValid()
	const
{
	return Command.IsValid() && Result.IsDurableRecord()
		&& Result.CommandId == Command.GetCommandId()
		&& Result.Operation == Command.GetOperation();
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	const TSubclassOf<AActor> ActorClass,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost& OutHost)
{
	OutHost = Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost();
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Candidate;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
			HandoffEvidence, ActorClass, Candidate.Session))
	{
		return false;
	}

	Candidate.Binding.SessionId = Candidate.Session.GetSessionId();
	Candidate.Binding.HandoffEvidenceId = HandoffEvidence.GetEvidenceId();
	Candidate.Binding.DeploymentId =
		HandoffEvidence.GetDeploymentEvidence()
			.GetDeployment().GetDeploymentId();
	Candidate.Binding.ActorClassPath = Candidate.Session.GetActorClassPath();
	Candidate.Binding.HostId =
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding::
			BuildHostId(
				Candidate.Binding.SessionId,
				Candidate.Binding.HandoffEvidenceId,
				Candidate.Binding.DeploymentId,
				Candidate.Binding.ActorClassPath);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutHost = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryTakeover(
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost& Previous,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost& OutHost)
{
	if (&Previous == &OutHost || !Previous.IsValid()
		|| OutHost.Binding.IsValid() || OutHost.Session.IsValid()
		|| !OutHost.Records.IsEmpty())
	{
		return false;
	}
	OutHost = MoveTemp(Previous);
	Previous = Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost();
	if (!OutHost.IsValid())
	{
		OutHost = Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::
IsTerminalOperation(const ECommandOperation Operation)
{
	return Operation == ECommandOperation::Cancel
		|| Operation == ECommandOperation::End;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::IsRecoverable(
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command,
	const Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult&
		SessionResult,
	const ESessionState CurrentState)
{
	if (!Command.IsValid() || !SessionResult.IsValid())
	{
		return false;
	}
	if (Command.GetOperation() == ECommandOperation::Publish)
	{
		return SessionResult.Status == ESessionStatus::PublicationRejected
			&& CurrentState == ESessionState::Publishing;
	}
	return IsTerminalOperation(Command.GetOperation())
		&& SessionResult.Status == ESessionStatus::TeardownRejected
		&& SessionResult.World.Status
			== Edemo_mapShanmenFormationWorldStatus::TeardownRecoveryRequired;
}

Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult
Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::Execute(
	Fdemo_mapShanmenFormationScatterWorldPublicationSession& TargetSession,
	UWorld* World,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command)
{
	switch (Command.GetOperation())
	{
	case ECommandOperation::Publish:
		return TargetSession.TryPublish(World);
	case ECommandOperation::Cancel:
		return TargetSession.TryTeardown(
			World, Edemo_mapShanmenFormationSessionState::Cancelled);
	case ECommandOperation::End:
		return TargetSession.TryTeardown(
			World, Edemo_mapShanmenFormationSessionState::Ended);
	default:
		return Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult();
	}
}

Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult
Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TrySubmit(
	UWorld* World,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command)
{
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult Result;
	Result.CommandId = Command.GetCommandId();
	Result.Operation = Command.GetOperation();
	if (!IsValid())
	{
		Result.Status = ECommandStatus::HostInvalid;
		Result.Diagnostic = TEXT("Publication command requires one valid Host.");
		return Result;
	}
	if (!Command.IsValid())
	{
		Result.Status = ECommandStatus::CommandInvalid;
		Result.Diagnostic = TEXT("Publication command requires one valid immutable command.");
		return Result;
	}
	if (!Binding.Matches(Command.GetBinding()))
	{
		Result.Status = ECommandStatus::BindingConflict;
		Result.Diagnostic = TEXT("Publication command rejected foreign session or deployment identity.");
		return Result;
	}

	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Existing = Records[Index];
		if (Existing.Command.GetCommandId() != Command.GetCommandId())
		{
			continue;
		}
		if (!Existing.Command.Matches(Command))
		{
			Result.Status = ECommandStatus::CommandIdConflict;
			Result.Diagnostic = TEXT("Publication CommandId was reused with another payload.");
			return Result;
		}
		if (Existing.Result.Status == ECommandStatus::SessionRejected)
		{
			return TryRecover(World, Command, Index);
		}
		Result = Existing.Result;
		Result.Status = ECommandStatus::Replayed;
		Result.bReplay = true;
		Result.bHostStateCommitted = false;
		Result.Diagnostic = TEXT("Publication command replayed its stored successful receipt.");
		return Result;
	}

	for (const auto& Existing : Records)
	{
		const bool bSameOperation =
			Existing.Command.GetOperation() == Command.GetOperation();
		const bool bBothTerminal = IsTerminalOperation(
			Existing.Command.GetOperation())
			&& IsTerminalOperation(Command.GetOperation());
		if (bSameOperation || bBothTerminal)
		{
			Result.Status = ECommandStatus::OperationIdentityConflict;
			Result.Diagnostic = TEXT("Publication operation already owns another CommandId.");
			return Result;
		}
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Candidate =
		*this;
	Result.Session = Execute(Candidate.Session, World, Command);
	Result.Status = IsSessionSuccess(Command.GetOperation(), Result.Session)
		? ECommandStatus::Applied
		: ECommandStatus::SessionRejected;
	Result.Diagnostic = Result.Session.Diagnostic;
	if (Result.Status == ECommandStatus::SessionRejected
		&& !IsRecoverable(
			Command, Result.Session, Candidate.Session.GetState()))
	{
		return Result;
	}

	Result.bHostStateCommitted = true;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord Record;
	Record.Command = Command;
	Record.Result = Result;
	Candidate.Records.Add(MoveTemp(Record));
	if (!Result.IsDurableRecord() || !Candidate.IsValid())
	{
		Result.Status = ECommandStatus::StateInvalid;
		Result.bHostStateCommitted = false;
		Result.Diagnostic = TEXT("Publication command produced invalid staged Host state.");
		return Result;
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult
Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryRecover(
	UWorld* World,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command,
	const int32 RecordIndex)
{
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult Result;
	Result.CommandId = Command.GetCommandId();
	Result.Operation = Command.GetOperation();
	Result.bRecoveryAttempted = true;
	if (!Records.IsValidIndex(RecordIndex)
		|| Records[RecordIndex].Result.Status != ECommandStatus::SessionRejected)
	{
		Result.Status = ECommandStatus::StateInvalid;
		Result.Diagnostic = TEXT("Publication recovery record is invalid.");
		return Result;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Candidate =
		*this;
	Result.Session = Execute(Candidate.Session, World, Command);
	const bool bSuccess =
		IsSessionSuccess(Command.GetOperation(), Result.Session);
	if (!bSuccess
		&& !IsRecoverable(
			Command, Result.Session, Candidate.Session.GetState()))
	{
		Result.Status = ECommandStatus::SessionRejected;
		Result.Diagnostic = Result.Session.Diagnostic;
		return Result;
	}

	Result.Status = bSuccess
		? ECommandStatus::Recovered
		: ECommandStatus::SessionRejected;
	Result.Diagnostic = Result.Session.Diagnostic;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult Stored = Result;
	Stored.Status = bSuccess ? ECommandStatus::Applied : ECommandStatus::SessionRejected;
	Stored.bRecoveryAttempted = false;
	Stored.bHostStateCommitted = true;
	Candidate.Records[RecordIndex].Result = Stored;
	if (!Stored.IsDurableRecord() || !Candidate.IsValid())
	{
		Result.Status = ECommandStatus::StateInvalid;
		Result.Diagnostic = TEXT("Publication recovery produced invalid staged Host state.");
		return Result;
	}
	*this = MoveTemp(Candidate);
	Result.bHostStateCommitted = true;
	return Result;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::IsValid()
	const
{
	const auto& Handoff = Session.GetHandoffEvidence();
	if (!Binding.IsValid() || !Session.IsValid()
		|| Binding.GetSessionId() != Session.GetSessionId()
		|| Binding.GetHandoffEvidenceId() != Handoff.GetEvidenceId()
		|| Binding.GetDeploymentId()
			!= Handoff.GetDeploymentEvidence()
				.GetDeployment().GetDeploymentId()
		|| Binding.GetActorClassPath() != Session.GetActorClassPath()
		|| Records.Num() > 2)
	{
		return false;
	}

	TSet<FGuid> CommandIds;
	bool bHasPublish = false;
	bool bHasTerminal = false;
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord*
		PublishRecord = nullptr;
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord*
		TerminalRecord = nullptr;
	for (const auto& Record : Records)
	{
		if (!Record.IsValid()
			|| !Record.Command.GetBinding().Matches(Binding)
			|| CommandIds.Contains(Record.Command.GetCommandId()))
		{
			return false;
		}
		CommandIds.Add(Record.Command.GetCommandId());
		if (Record.Command.GetOperation() == ECommandOperation::Publish)
		{
			if (bHasPublish)
			{
				return false;
			}
			bHasPublish = true;
			PublishRecord = &Record;
		}
		else
		{
			if (bHasTerminal)
			{
				return false;
			}
			bHasTerminal = true;
			TerminalRecord = &Record;
		}
	}
	if (!bHasTerminal && Session.IsTerminal())
	{
		return false;
	}
	if (bHasTerminal)
	{
		if (!TerminalRecord)
		{
			return false;
		}
		if (TerminalRecord->Result.Status == ECommandStatus::SessionRejected)
		{
			return !Session.IsTerminal();
		}
		const ESessionState ExpectedState =
			TerminalRecord->Command.GetOperation() == ECommandOperation::Cancel
			? ESessionState::Cancelled
			: ESessionState::Ended;
		if (TerminalRecord->Result.Status != ECommandStatus::Applied
			|| Session.GetState() != ExpectedState)
		{
			return false;
		}
	}
	if (bHasPublish && !PublishRecord)
	{
		return false;
	}
	if (PublishRecord
		&& PublishRecord->Result.Status == ECommandStatus::SessionRejected
		&& !bHasTerminal && Session.GetState() != ESessionState::Publishing)
	{
		return false;
	}
	if (PublishRecord
		&& PublishRecord->Result.Status == ECommandStatus::Applied
		&& !bHasTerminal && Session.GetState() != ESessionState::Published)
	{
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryGetRecord(
	const FGuid& CommandId,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord& OutRecord)
	const
{
	OutRecord =
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord();
	if (!CommandId.IsValid() || !IsValid())
	{
		return false;
	}
	for (const auto& Record : Records)
	{
		if (Record.Command.GetCommandId() == CommandId)
		{
			OutRecord = Record;
			return OutRecord.IsValid();
		}
	}
	return false;
}
