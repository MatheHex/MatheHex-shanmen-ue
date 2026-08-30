#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritEvasionCommandRouter.h"

class Fdemo_mapCombatRunCoordinator;

/**
 * Immutable product-owned content and motion policy for Spirit Evasion.
 *
 * This is the sole place that chooses the initial 0.0.10 values. Input,
 * GameMode and the command router may only consume the captured snapshots.
 */
class Fdemo_mapShanmenSpiritEvasionProductConfig
{
public:
	static FName CanonicalContentVersion();
	static FString CanonicalContentDigest();
	static FName CanonicalDefenseRuleId();
	static FName CanonicalMovementPolicyId();
	static FGuid CanonicalConfigId();
	static bool TryCreateCanonical(
		Fdemo_mapShanmenSpiritEvasionProductConfig& OutConfig);

	bool IsValid() const;
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FShanmenSpiritEvasionDefinition& GetDefinition() const
	{
		return Definition;
	}
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& GetPolicy() const
	{
		return Policy;
	}
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& GetTrajectory() const
	{
		return Trajectory;
	}

private:
	FGuid ConfigId;
	FShanmenContentStamp Content;
	FShanmenSpiritEvasionDefinition Definition;
	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
};

/** Run-issued identity and frozen action for exactly one activation attempt. */
class Fdemo_mapPlayerSpiritEvasionActionReservation
{
public:
	bool IsValid() const;
	uint64 GetActivationSequence() const { return ActivationSequence; }
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FGuid& GetActivationId() const { return Action.GetActivationId(); }

private:
	friend class Fdemo_mapCombatRunCoordinator;

	uint64 ActivationSequence = 0;
	FGuid ConfigId;
	FShanmenCombatActionSnapshot Action;
};

enum class Edemo_mapShanmenSpiritEvasionProductStartStatus : uint8
{
	Ready,
	InvalidDirection,
	ConfigUnavailable,
	ReservationRejected,
	CommandRejected
};

/** Complete pre-dispatch proof joining config, Run identity and typed command. */
struct Fdemo_mapShanmenSpiritEvasionProductStartResult
{
	Edemo_mapShanmenSpiritEvasionProductStartStatus Status =
		Edemo_mapShanmenSpiritEvasionProductStartStatus::ConfigUnavailable;
	Fdemo_mapShanmenSpiritEvasionProductConfig Config;
	Fdemo_mapPlayerSpiritEvasionActionReservation Reservation;
	Fdemo_mapShanmenSpiritEvasionCommand Command;
	FString Diagnostic;

	bool IsReady() const;
};

/**
 * Pure product composition boundary before the existing P10.7 router.
 *
 * It validates direction before mutating Run sequence state, captures the one
 * canonical config, asks the active Run to reserve identity, and freezes the
 * exact typed command. It owns no input binding, Actor, component, resource,
 * clock, movement or lifecycle state.
 */
struct Fdemo_mapShanmenSpiritEvasionProductAuthority
{
	static Fdemo_mapShanmenSpiritEvasionProductStartResult PrepareStart(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FVector& CandidateDirection);
};
