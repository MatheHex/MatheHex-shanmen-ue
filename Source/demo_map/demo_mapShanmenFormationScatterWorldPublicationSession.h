#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationScatterWorldPublication.h"

class AActor;
class UWorld;

enum class Edemo_mapShanmenFormationScatterWorldPublicationSessionState
	: uint8
{
	Empty,
	Ready,
	Publishing,
	Published,
	Cancelled,
	Ended
};

enum class Edemo_mapShanmenFormationScatterWorldPublicationSessionStatus
	: uint8
{
	Invalid,
	Published,
	Recovered,
	Replayed,
	TeardownComplete,
	TeardownReplayed,
	SessionInvalid,
	SessionTerminal,
	WorldInvalid,
	WorldConflict,
	PublicationRejected,
	TerminalStateInvalid,
	TerminalConflict,
	TeardownRejected
};

struct Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult
{
	Edemo_mapShanmenFormationScatterWorldPublicationSessionStatus Status =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterWorldPublicationResult Publication;
	Fdemo_mapShanmenFormationWorldResult World;

	bool IsValid() const;
	bool IsPublicationSuccess() const;
	bool IsTeardownSuccess() const;
};

/**
 * Sole transient owner for one complete P27.22 batch in one World.
 *
 * The source deployment and resource facts remain immutable. This session
 * owns only the P27.23 publication ledger, one existing World adapter, and
 * one terminal World cleanup receipt.
 */
class Fdemo_mapShanmenFormationScatterWorldPublicationSession
{
public:
	static bool TryStart(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		TSubclassOf<AActor> ActorClass,
		Fdemo_mapShanmenFormationScatterWorldPublicationSession& OutSession);

	Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult TryPublish(
		UWorld* World);
	Fdemo_mapShanmenFormationScatterWorldPublicationSessionResult TryTeardown(
		UWorld* World,
		Edemo_mapShanmenFormationSessionState TerminalState);

	bool IsValid() const;
	bool IsTerminal() const;
	const FGuid& GetSessionId() const { return SessionId; }
	FString GetActorClassPath() const
	{
		const UClass* RawClass = ActorClass.Get();
		return RawClass ? RawClass->GetPathName() : FString();
	}
	Edemo_mapShanmenFormationScatterWorldPublicationSessionState GetState()
		const
	{
		return State;
	}
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
	GetHandoffEvidence() const
	{
		return HandoffEvidence;
	}
	const Fdemo_mapShanmenFormationScatterWorldPublicationLedger& GetLedger()
		const
	{
		return Ledger;
	}
	const Fdemo_mapShanmenFormationScatterWorldPublicationEvidence&
	GetCompletionEvidence() const
	{
		return CompletionEvidence;
	}
	const Fdemo_mapShanmenFormationWorldAdapter& GetWorldAdapter() const
	{
		return WorldAdapter;
	}
	const Fdemo_mapShanmenFormationWorldTeardownReceipt& GetTeardownReceipt()
		const
	{
		return TeardownReceipt;
	}
	bool IsBoundToWorld(const UWorld* World) const
	{
		return World && BoundWorld.Get() == World;
	}

private:
	static FGuid BuildSessionId(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		const FString& ActorClassPath);

	FGuid SessionId;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence
		HandoffEvidence;
	TSubclassOf<AActor> ActorClass;
	TWeakObjectPtr<UWorld> BoundWorld;
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger Ledger;
	Fdemo_mapShanmenFormationScatterWorldPublicationEvidence
		CompletionEvidence;
	Fdemo_mapShanmenFormationWorldAdapter WorldAdapter;
	Fdemo_mapShanmenFormationWorldTeardownReceipt TeardownReceipt;
	Edemo_mapShanmenFormationScatterWorldPublicationSessionState State =
		Edemo_mapShanmenFormationScatterWorldPublicationSessionState::Empty;
};
