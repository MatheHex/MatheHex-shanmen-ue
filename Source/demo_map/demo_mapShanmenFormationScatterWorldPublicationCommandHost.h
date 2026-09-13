#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationScatterWorldPublicationSession.h"

class AActor;
class UWorld;

/** Immutable identity of one P27.24 session at the product command boundary. */
class Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
			Other) const;
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetSessionId() const { return SessionId; }
	const FGuid& GetHandoffEvidenceId() const { return HandoffEvidenceId; }
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	const FString& GetActorClassPath() const { return ActorClassPath; }

private:
	friend class Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost;

	static FGuid BuildHostId(
		const FGuid& SessionId,
		const FGuid& HandoffEvidenceId,
		const FGuid& DeploymentId,
		const FString& ActorClassPath);

	FGuid HostId;
	FGuid SessionId;
	FGuid HandoffEvidenceId;
	FGuid DeploymentId;
	FString ActorClassPath;
};

enum class Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation
	: uint8
{
	Invalid,
	Publish,
	Cancel,
	End
};

/** Frozen caller intent for one explicit publication or terminal operation. */
class Fdemo_mapShanmenFormationScatterWorldPublicationCommand
{
public:
	static bool TryCapturePublish(
		const FGuid& CommandId,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
			Binding,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand);
	static bool TryCaptureCancel(
		const FGuid& CommandId,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
			Binding,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand);
	static bool TryCaptureEnd(
		const FGuid& CommandId,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
			Binding,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Other)
		const;
	const FGuid& GetCommandId() const { return CommandId; }
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
	GetBinding() const
	{
		return Binding;
	}
	Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation
	GetOperation() const
	{
		return Operation;
	}

private:
	static bool TryCapture(
		const FGuid& CommandId,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
			Binding,
		Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation
			Operation,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand);

	FGuid CommandId;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding Binding;
	Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation Operation =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation::Invalid;
};

enum class Edemo_mapShanmenFormationScatterWorldPublicationCommandStatus
	: uint8
{
	Invalid,
	Applied,
	Recovered,
	Replayed,
	HostInvalid,
	CommandInvalid,
	BindingConflict,
	CommandIdConflict,
	OperationIdentityConflict,
	SessionRejected,
	StateInvalid
};

/** Auditable outer receipt around one P27.24 session operation. */
struct Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult
{
	Edemo_mapShanmenFormationScatterWorldPublicationCommandStatus Status =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandStatus::Invalid;
	FGuid CommandId;
	Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation Operation =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation::Invalid;
	bool bReplay = false;
	bool bRecoveryAttempted = false;
	bool bHostStateCommitted = false;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Session;

	bool IsValid() const;
	bool IsSuccess() const;
	bool IsDurableRecord() const;
};

/** Frozen command and its latest durable product-host receipt. */
struct Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord
{
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Command;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult Result;

	bool IsValid() const;
};

/**
 * Sole caller-facing command owner for one complete scatter publication.
 *
 * The Host owns exactly one P27.24 Session and routes one immutable command per
 * call. Exact successful commands replay stored receipts without touching the
 * World. A partial publication or teardown rejection may only advance through
 * an exact retry of its original command. The Host never discovers work,
 * schedules retries, owns a World pointer, mutates resource authority, or
 * bypasses the existing publication Session and World adapter.
 */
class Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost
{
public:
	static bool TryOpen(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		TSubclassOf<AActor> ActorClass,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost& OutHost);
	/** Move the complete live owner and receipts; the previous owner is reset. */
	static bool TryTakeover(
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost& Previous,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost& OutHost);

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult TrySubmit(
		UWorld* World,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command);

	bool IsValid() const;
	bool IsEmpty() const { return Records.IsEmpty(); }
	int32 GetRecordCount() const { return Records.Num(); }
	bool TryGetRecord(
		const FGuid& CommandId,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord&
			OutRecord) const;
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
	GetBinding() const
	{
		return Binding;
	}
	const Fdemo_mapShanmenFormationScatterWorldPublicationSession& GetSession()
		const
	{
		return Session;
	}

private:
	static bool IsTerminalOperation(
		Edemo_mapShanmenFormationScatterWorldPublicationCommandOperation
			Operation);
	static bool IsRecoverable(
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command,
		const Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult&
			Session,
		Edemo_mapShanmenFormationScatterWorldPublicationSessionState
			CurrentState);
	static Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Execute(
		Fdemo_mapShanmenFormationScatterWorldPublicationSession& Session,
		UWorld* World,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command);
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult TryRecover(
		UWorld* World,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommand& Command,
		int32 RecordIndex);

	Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding Binding;
	Fdemo_mapShanmenFormationScatterWorldPublicationSession Session;
	TArray<Fdemo_mapShanmenFormationScatterWorldPublicationCommandRecord>
		Records;
};
