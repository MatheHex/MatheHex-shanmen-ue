#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenWeaponGuardWorldAdapter.h"

class AActor;
class UWorld;

enum class Edemo_mapShanmenWeaponGuardDefenseStatus : uint8
{
	ComposedQualified,
	ComposedOutsideArc,
	InvalidInput,
	BaseDefenseInvalid,
	TimingProjectionRejected,
	WorldEvaluationRejected,
	LayerConflict,
	SnapshotCompositionRejected
};

/** UObject-free proof of one append-only weapon-guard defense composition. */
struct Fdemo_mapShanmenWeaponGuardDefenseResult
{
	Edemo_mapShanmenWeaponGuardDefenseStatus Status =
		Edemo_mapShanmenWeaponGuardDefenseStatus::InvalidInput;
	FGuid ReceiptId;
	FShanmenWeaponGuardTimingProjectionReceipt TimingProjection;
	Fdemo_mapShanmenWeaponGuardWorldResult WorldEvaluation;
	FShanmenDefenseSnapshot BaseDefense;
	FShanmenDefenseSnapshot Defense;
	FString Diagnostic;

	bool IsSuccess() const;
	bool HasGuardLayer() const
	{
		return IsSuccess()
			&& Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedQualified;
	}
};

/**
 * Stateless per-impact composition boundary for P11.0-P11.3.
 *
 * The caller owns the action/window, timeline observation, World Actors and
 * base defense capture. This coordinator only projects P11.1, evaluates P11.3
 * and appends the exact qualified P11.2 layer. It owns no Actor, clock, Tick,
 * input, Impact resolution, resource transaction, durability or retry.
 */
class Fdemo_mapShanmenWeaponGuardDefenseCoordinator
{
public:
	static Fdemo_mapShanmenWeaponGuardDefenseResult Compose(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* DefenderActor,
		AActor* ThreatActor,
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenWeaponPerfectGuardPolicy& TimingPolicy,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		const FShanmenWeaponGuardArcPolicy& ArcPolicy,
		const FShanmenHitCandidate& Candidate,
		const FShanmenDefenseSnapshot& BaseDefense);
};
