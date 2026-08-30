#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritEvasionMovement.h"
#include "demo_mapCombatDisplacement.h"

class ACharacter;

/** Mutable product values captured for one named spirit-evasion movement policy. */
struct Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture
{
	FName MovementPolicyId = NAME_None;
	float RequestedDistance = 0.0f;
	float MinimumResolvedDistance = 0.0f;
	float WorldStaticClearance = 0.0f;
};

/** Immutable displacement values selected by the product policy authority. */
class Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture& Capture,
		Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& OutPolicy);

	bool IsValid() const;
	FName GetMovementPolicyId() const { return MovementPolicyId; }
	float GetRequestedDistance() const { return RequestedDistance; }
	float GetMinimumResolvedDistance() const
	{
		return MinimumResolvedDistance;
	}
	float GetWorldStaticClearance() const { return WorldStaticClearance; }

private:
	FName MovementPolicyId = NAME_None;
	float RequestedDistance = 0.0f;
	float MinimumResolvedDistance = 0.0f;
	float WorldStaticClearance = 0.0f;
};

/** Immutable binding between one action-window request and one product policy. */
class Fdemo_mapShanmenSpiritEvasionMovementPlan
{
public:
	bool IsValid() const;
	const FGuid& GetPlanId() const { return PlanId; }
	const FShanmenSpiritEvasionMovementRequest& GetRequest() const
	{
		return Request;
	}
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& GetPolicy() const
	{
		return Policy;
	}

private:
	friend class Fdemo_mapShanmenSpiritEvasionMovementAdapter;

	FGuid PlanId;
	FShanmenSpiritEvasionMovementRequest Request;
	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
};

enum class Edemo_mapShanmenSpiritEvasionMovementPreflightStatus : uint8
{
	InvalidPlan,
	CharacterUnavailable,
	InsufficientResolvedDistance,
	Ready
};

/** Auditable, read-only WorldStatic preflight result. It never moves the Actor. */
struct Fdemo_mapShanmenSpiritEvasionMovementPreflightResult
{
	Edemo_mapShanmenSpiritEvasionMovementPreflightStatus Status =
		Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::InvalidPlan;
	FGuid PlanId;
	Fdemo_mapCombatDisplacementResult Displacement;

	bool IsReady() const
	{
		return Status
			== Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready
			&& PlanId.IsValid();
	}
};

/**
 * Product bridge for P10.1 movement requests.
 *
 * The bridge freezes explicit policy values and performs one read-only swept
 * preflight. It owns no input lifetime, timer, trajectory, resource balance,
 * movement component mutation or gameplay default values.
 */
class Fdemo_mapShanmenSpiritEvasionMovementAdapter
{
public:
	static bool TryBuildPlan(
		const FShanmenSpiritEvasionMovementRequest& Request,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		Fdemo_mapShanmenSpiritEvasionMovementPlan& OutPlan);

	static bool IsResolvedDistanceAccepted(
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan,
		float ResolvedDistance);

	static Fdemo_mapShanmenSpiritEvasionMovementPreflightResult
	PreflightWorldStatic(
		const ACharacter* Character,
		const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan);
};
