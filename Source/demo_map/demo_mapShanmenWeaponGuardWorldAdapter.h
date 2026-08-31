#pragma once

#include "CoreMinimal.h"
#include "ShanmenWeaponGuardArc.h"
#include "ShanmenWorldEntityRegistry.h"

class AActor;
class UWorld;

enum class Edemo_mapShanmenWeaponGuardWorldStatus : uint8
{
	Evaluated,
	RuntimeInputInvalid,
	ArcBindingRejected,
	WorldInvalid,
	RegistryInactive,
	RunMismatch,
	DefenderUnavailable,
	ThreatUnavailable,
	ActorWorldMismatch,
	DefenderUnregistered,
	ThreatUnregistered,
	IdentityConflict,
	DefenderIdentityMismatch,
	ThreatIdentityMismatch,
	TransformInvalid,
	DirectionSampleRejected,
	ArcEvaluationRejected
};

/** UObject-free audit result of one synchronous World direction sample. */
struct Fdemo_mapShanmenWeaponGuardWorldResult
{
	Edemo_mapShanmenWeaponGuardWorldStatus Status =
		Edemo_mapShanmenWeaponGuardWorldStatus::RuntimeInputInvalid;
	FGuid ReceiptId;
	FGuid RunId;
	FGuid DefenderEntityId;
	FGuid ThreatEntityId;
	FVector DefenderLocation = FVector::ZeroVector;
	FVector ThreatLocation = FVector::ZeroVector;
	FShanmenWeaponGuardThreatSample Sample;
	FShanmenWeaponGuardArcEvaluation Evaluation;
	FString Diagnostic;

	bool IsSuccess() const;
	bool IsQualified() const
	{
		return IsSuccess() && Evaluation.IsQualified();
	}
};

/**
 * Stateless synchronous bridge from registered live Actors to P11.2.
 *
 * The caller owns cadence and supplies both Actors explicitly. The adapter
 * samples each transform once, retains no UObject pointer, performs no trace,
 * discovery, Tick, timer, input read, damage, resource mutation or retry, and
 * never infers facing semantics from the contact normal.
 */
class Fdemo_mapShanmenWeaponGuardWorldAdapter
{
public:
	static Fdemo_mapShanmenWeaponGuardWorldResult Evaluate(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* DefenderActor,
		AActor* ThreatActor,
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenWeaponGuardArcPolicy& Policy,
		const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection,
		const FShanmenHitCandidate& Candidate);
};
