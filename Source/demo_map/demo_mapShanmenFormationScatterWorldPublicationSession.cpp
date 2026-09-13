#include "demo_mapShanmenFormationScatterWorldPublicationSession.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using ESessionState =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionState;
	using ESessionStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionStatus;
	using EPublicationStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsTerminalState(
		const Edemo_mapShanmenFormationSessionState State)
	{
		return State == Edemo_mapShanmenFormationSessionState::Cancelled
			|| State == Edemo_mapShanmenFormationSessionState::Ended;
	}

	ESessionState ToSessionState(
		const Edemo_mapShanmenFormationSessionState State)
	{
		return State == Edemo_mapShanmenFormationSessionState::Cancelled
			? ESessionState::Cancelled
			: ESessionState::Ended;
	}

	ESessionStatus ToPublicationStatus(const EPublicationStatus Status)
	{
		switch (Status)
		{
		case EPublicationStatus::Published:
			return ESessionStatus::Published;
		case EPublicationStatus::Recovered:
			return ESessionStatus::Recovered;
		case EPublicationStatus::Replayed:
			return ESessionStatus::Replayed;
		default:
			return ESessionStatus::PublicationRejected;
		}
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Reject(
		const ESessionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult::
IsPublicationSuccess() const
{
	return (Status == ESessionStatus::Published
			|| Status == ESessionStatus::Recovered
			|| Status == ESessionStatus::Replayed)
		&& Publication.IsSuccess();
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult::
IsTeardownSuccess() const
{
	return (Status == ESessionStatus::TeardownComplete
			|| Status == ESessionStatus::TeardownReplayed)
		&& World.IsTeardownSuccess();
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult::IsValid()
	const
{
	if (Status == ESessionStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == ESessionStatus::Published
		|| Status == ESessionStatus::Recovered
		|| Status == ESessionStatus::Replayed)
	{
		return Publication.IsSuccess();
	}
	if (Status == ESessionStatus::TeardownComplete
		|| Status == ESessionStatus::TeardownReplayed)
	{
		return World.IsTeardownSuccess();
	}
	if (Status == ESessionStatus::PublicationRejected)
	{
		return Publication.IsValid() && !Publication.IsSuccess();
	}
	return !Publication.IsSuccess() && !World.IsTeardownSuccess();
}

FGuid Fdemo_mapShanmenFormationScatterWorldPublicationSession::BuildSessionId(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		InHandoffEvidence,
	const FString& ActorClassPath)
{
	if (!InHandoffEvidence.IsValid() || ActorClassPath.IsEmpty())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPublicationSession.r1"),
		{
			GuidDigits(InHandoffEvidence.GetEvidenceId()),
			GuidDigits(
				InHandoffEvidence.GetDeploymentEvidence()
					.GetDeployment().GetDeploymentId()),
			ActorClassPath
		});
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryStart(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		InHandoffEvidence,
	TSubclassOf<AActor> InActorClass,
	Fdemo_mapShanmenFormationScatterWorldPublicationSession& OutSession)
{
	OutSession =
		Fdemo_mapShanmenFormationScatterWorldPublicationSession();
	UClass* RawClass = InActorClass.Get();
	if (!InHandoffEvidence.IsValid() || !RawClass
		|| RawClass->HasAnyClassFlags(
			CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		return false;
	}

	OutSession.HandoffEvidence = InHandoffEvidence;
	OutSession.ActorClass = InActorClass;
	OutSession.SessionId = BuildSessionId(
		InHandoffEvidence, RawClass->GetPathName());
	OutSession.State = ESessionState::Ready;
	if (!OutSession.IsValid())
	{
		OutSession =
			Fdemo_mapShanmenFormationScatterWorldPublicationSession();
		return false;
	}
	return true;
}

Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult
Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryPublish(
	UWorld* World)
{
	if (!IsValid())
	{
		return Reject(
			ESessionStatus::SessionInvalid,
			TEXT("World publication requires one valid product-owned session."));
	}
	if (IsTerminal())
	{
		return Reject(
			ESessionStatus::SessionTerminal,
			TEXT("A terminal World publication session cannot publish again."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			ESessionStatus::WorldInvalid,
			TEXT("World publication requires one live World."));
	}
	if (BoundWorld.IsValid() && BoundWorld.Get() != World)
	{
		return Reject(
			ESessionStatus::WorldConflict,
			TEXT("One publication session cannot cross World ownership."));
	}

	BoundWorld = World;
	Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort Port(
		World, ActorClass, WorldAdapter);
	const ESessionState PreviousState = State;
	Fdemo_mapShanmenFormationScatterWorldPublicationResult Publication =
		Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
			HandoffEvidence, Port, Ledger);
	if (!Publication.IsSuccess())
	{
		if (PreviousState != ESessionState::Published)
		{
			CompletionEvidence =
				Fdemo_mapShanmenFormationScatterWorldPublicationEvidence();
			State = Ledger.IsEmpty()
				? ESessionState::Ready
				: ESessionState::Publishing;
		}
		Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Result;
		Result.Status = ESessionStatus::PublicationRejected;
		Result.Diagnostic = Publication.Diagnostic;
		Result.Publication = Publication;
		return Result;
	}

	CompletionEvidence = Publication.Evidence;
	State = ESessionState::Published;
	Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Result;
	Result.Status = ToPublicationStatus(Publication.Status);
	Result.Diagnostic = Publication.Diagnostic;
	Result.Publication = Publication;
	return Result;
}

Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult
Fdemo_mapShanmenFormationScatterWorldPublicationSession::TryTeardown(
	UWorld* World,
	const Edemo_mapShanmenFormationSessionState TerminalState)
{
	if (!IsValid())
	{
		return Reject(
			ESessionStatus::SessionInvalid,
			TEXT("World teardown requires one valid product-owned session."));
	}
	if (!IsTerminalState(TerminalState))
	{
		return Reject(
			ESessionStatus::TerminalStateInvalid,
			TEXT("World teardown accepts only Cancelled or Ended."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			ESessionStatus::WorldInvalid,
			TEXT("World teardown requires one live World."));
	}
	if (BoundWorld.IsValid() && BoundWorld.Get() != World)
	{
		return Reject(
			ESessionStatus::WorldConflict,
			TEXT("One publication session cannot teardown another World."));
	}
	if (IsTerminal()
		&& State != ToSessionState(TerminalState))
	{
		return Reject(
			ESessionStatus::TerminalConflict,
			TEXT("A completed teardown cannot change terminal reason."));
	}

	BoundWorld = World;
	Fdemo_mapShanmenFormationWorldTeardownRequest Request;
	Request.DeploymentId =
		HandoffEvidence.GetDeploymentEvidence()
			.GetDeployment().GetDeploymentId();
	Request.TerminalState = TerminalState;
	Request.CommittedAnchorCount = HandoffEvidence.GetHandoffs().Num();
	const Fdemo_mapShanmenFormationWorldResult WorldResult =
		WorldAdapter.TryTeardownDeployment(World, Request);
	if (!WorldResult.IsTeardownSuccess())
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Result;
		Result.Status = ESessionStatus::TeardownRejected;
		Result.Diagnostic = WorldResult.Diagnostic;
		Result.World = WorldResult;
		return Result;
	}

	State = ToSessionState(TerminalState);
	TeardownReceipt = WorldResult.TeardownReceipt;
	Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult Result;
	Result.Status = WorldResult.Status
			== Edemo_mapShanmenFormationWorldStatus::TeardownReplayed
		? ESessionStatus::TeardownReplayed
		: ESessionStatus::TeardownComplete;
	Result.Diagnostic = WorldResult.Diagnostic;
	Result.World = WorldResult;
	return Result;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationSession::IsTerminal()
	const
{
	return State == ESessionState::Cancelled
		|| State == ESessionState::Ended;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationSession::IsValid() const
{
	UClass* RawClass = ActorClass.Get();
	if (!SessionId.IsValid() || !HandoffEvidence.IsValid() || !RawClass
		|| RawClass->HasAnyClassFlags(
			CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
		|| SessionId != BuildSessionId(
			HandoffEvidence, RawClass->GetPathName())
		|| State == ESessionState::Empty
		|| !Ledger.IsValid()
		|| !Ledger.IsCompatibleWith(
			HandoffEvidence, RawClass->GetPathName())
		|| !WorldAdapter.IsValid())
	{
		return false;
	}

	const int32 HandoffCount = HandoffEvidence.GetHandoffs().Num();
	if (State == ESessionState::Ready)
	{
		return Ledger.IsEmpty() && !CompletionEvidence.IsValid()
			&& !TeardownReceipt.IsValid();
	}
	if (State == ESessionState::Publishing)
	{
		return BoundWorld.IsValid() && !Ledger.IsEmpty()
			&& Ledger.GetPublishedCount() < HandoffCount
			&& !CompletionEvidence.IsValid()
			&& !TeardownReceipt.IsValid();
	}
	if (State == ESessionState::Published)
	{
		return BoundWorld.IsValid()
			&& Ledger.GetPublishedCount() == HandoffCount
			&& CompletionEvidence.IsValid()
			&& CompletionEvidence.GetHandoffEvidence() == HandoffEvidence
			&& CompletionEvidence.GetLedger() == Ledger
			&& !TeardownReceipt.IsValid();
	}
	if (!BoundWorld.IsValid() || !IsTerminal()
		|| !WorldAdapter.IsTeardownComplete()
		|| !TeardownReceipt.IsValid()
		|| TeardownReceipt.DeploymentId
			!= HandoffEvidence.GetDeploymentEvidence()
				.GetDeployment().GetDeploymentId()
		|| TeardownReceipt.TerminalState
			!= (State == ESessionState::Cancelled
				? Edemo_mapShanmenFormationSessionState::Cancelled
				: Edemo_mapShanmenFormationSessionState::Ended)
		|| TeardownReceipt.CommittedAnchorCount != HandoffCount)
	{
		return false;
	}
	return Ledger.GetPublishedCount() == HandoffCount
		? CompletionEvidence.IsValid()
			&& CompletionEvidence.GetHandoffEvidence() == HandoffEvidence
			&& CompletionEvidence.GetLedger() == Ledger
		: !CompletionEvidence.IsValid();
}
