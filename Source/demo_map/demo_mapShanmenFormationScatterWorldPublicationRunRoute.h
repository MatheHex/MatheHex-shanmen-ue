#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationScatterWorldPublicationCommandHost.h"

class AActor;
class UWorld;

/** Upper Run-lifecycle event accepted by the sole P27.26 publication route. */
enum class Edemo_mapShanmenFormationScatterWorldPublicationRunEvent : uint8
{
	Invalid,
	Publish,
	Cancel,
	End
};

enum class Edemo_mapShanmenFormationScatterWorldPublicationRunRouteStatus
	: uint8
{
	Invalid,
	Applied,
	Recovered,
	Replayed,
	RouteInvalid,
	EventInvalid,
	RunMismatch,
	CommandCaptureRejected,
	HostRejected,
	StateInvalid
};

/** Pointer-free evidence for one Run event translated through the P27.25 Host. */
struct Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
{
	Edemo_mapShanmenFormationScatterWorldPublicationRunRouteStatus Status =
		Edemo_mapShanmenFormationScatterWorldPublicationRunRouteStatus::Invalid;
	FString Diagnostic;
	FGuid RouteId;
	FGuid RunId;
	FGuid CommandId;
	Edemo_mapShanmenFormationScatterWorldPublicationRunEvent Event =
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent::Invalid;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandResult Command;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Sole Run-bound composition owner for one scatter World-publication Host.
 *
 * One route binds the immutable resource/deployment handoff to the exact Run
 * already recorded by that handoff. Upper lifecycle events are translated to
 * deterministic P27.25 commands and never bypass the Host. Process-local
 * takeover moves the entire owner and invalidates the previous route; process
 * reconstruction derives the same route and command identities so the lower
 * publication layers can adopt their exact World-tagged Actor batch.
 *
 * This route owns no item, Deployment, Actor or World authority, retains no
 * World pointer, and performs no discovery, input interpretation, polling,
 * scheduling or background retry.
 */
class Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute
{
public:
	static bool TryOpen(
		const FGuid& RunId,
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		TSubclassOf<AActor> ActorClass,
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute& OutRoute);
	/** Move the complete live route; the previous owner becomes empty. */
	static bool TryTakeover(
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute& Previous,
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute& OutRoute);

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult TryRoute(
		const FGuid& ExpectedRunId,
		UWorld* World,
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent Event);
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult TryPublish(
		const FGuid& ExpectedRunId,
		UWorld* World);
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult TryCancel(
		const FGuid& ExpectedRunId,
		UWorld* World);
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult TryEnd(
		const FGuid& ExpectedRunId,
		UWorld* World);

	bool IsValid() const;
	bool IsEmpty() const;
	const FGuid& GetRouteId() const { return RouteId; }
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost&
	GetHost() const
	{
		return Host;
	}

private:
	static FGuid BuildRouteId(const FGuid& RunId, const FGuid& HostId);
	static FGuid BuildCommandId(
		const FGuid& RouteId,
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent Event);
	static bool TryCaptureCommand(
		const FGuid& CommandId,
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent Event,
		const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
			Binding,
		Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand);
	void Clear();

	FGuid RouteId;
	FGuid RunId;
	Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost Host;
};
