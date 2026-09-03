#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseProductAuthority.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class UWorld;

/**
 * Immutable retry token for one Run-issued Divine Sense use attempt.
 *
 * The route is the only writer. Callers may retain this value for retry or
 * replay, but cannot supply product config, action identity or Intent fields.
 */
class Fdemo_mapShanmenDivineSenseProductUseAttempt
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseProductUseAttempt& Other) const;

	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const FGuid& GetConfigId() const { return ConfigId; }
	const Fdemo_mapShanmenDivineSenseProductPrepareResult& GetPrepared() const
	{
		return Prepared;
	}
	const Fdemo_mapShanmenDivineSenseProductIntent& GetIntent() const
	{
		return Prepared.Intent;
	}

private:
	friend struct Fdemo_mapShanmenDivineSenseProductRoute;

	FGuid ControllerId;
	FGuid RunId;
	FGuid SourceEntityId;
	FGuid ConfigId;
	Fdemo_mapShanmenDivineSenseProductPrepareResult Prepared;
};

enum class Edemo_mapShanmenDivineSenseProductRouteStatus : uint8
{
	Invalid,
	Applied,
	Replayed,
	ControllerUnavailable,
	CoordinatorUnavailable,
	RunMismatch,
	LiveInputRejected,
	AvailabilityRejected,
	ProductPrepareRejected,
	AttemptRejected,
	ControllerRejected,
	StateDesynchronized
};

/** Complete proof from a device-independent use request to P19.6. */
struct Fdemo_mapShanmenDivineSenseProductRouteResult
{
	Edemo_mapShanmenDivineSenseProductRouteStatus Status =
		Edemo_mapShanmenDivineSenseProductRouteStatus::Invalid;
	FString Diagnostic;
	bool bReusedAttempt = false;
	Fdemo_mapShanmenDivineSenseProductUseAttempt Attempt;
	Fdemo_mapShanmenDivineSenseProductAvailability AvailabilityBefore;
	Fdemo_mapShanmenDivineSenseProductControllerResult ControllerResult;

	bool IsValid() const;
	bool IsAccepted() const;
	bool IsReplay() const
	{
		return IsAccepted()
			&& Status
				== Edemo_mapShanmenDivineSenseProductRouteStatus::Replayed;
	}
	bool HasAttempt() const { return Attempt.IsValid(); }
};

/**
 * Stateless Run-scoped product route for Divine Sense.
 *
 * Begin accepts only the existing SpiritEnergy authority snapshot. TryUse
 * takes one device-independent "use" request plus an explicit caller-owned
 * World batch. It validates every observable fence before P19.7 consumes an
 * activation sequence, then delegates only the authority-created Intent to
 * the P19.6 Controller. TryRetry reuses the immutable attempt and never asks
 * the Combat Run for another identity. Input binding, Actor discovery, UI and
 * final balance remain outside this seam.
 */
struct Fdemo_mapShanmenDivineSenseProductRoute
{
	static bool TryBegin(
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenActionResourceSnapshot& OpeningSpiritEnergy,
		FString& OutDiagnostic);

	static Fdemo_mapShanmenDivineSenseProductRouteResult TryUse(
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	static Fdemo_mapShanmenDivineSenseProductRouteResult TryRetry(
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const Fdemo_mapShanmenDivineSenseProductUseAttempt& Attempt,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	static Fdemo_mapShanmenDivineSenseProductControllerEndResult TryEnd(
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator);
};
