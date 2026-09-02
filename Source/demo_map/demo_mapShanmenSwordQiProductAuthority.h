#pragma once

#include "CoreMinimal.h"
#include "ShanmenSwordQiExecution.h"

class Fdemo_mapCombatRunCoordinator;

/**
 * Immutable P18.3 product policy for the first playable sword-qi route.
 *
 * These are versioned P-stage defaults, not final balance. Input, GameMode,
 * the command route and the physical Host may consume this snapshot but may
 * not reconstruct individual damage, speed or range values.
 */
class Fdemo_mapShanmenSwordQiProductConfig
{
public:
	static FName CanonicalContentVersion();
	static FString CanonicalContentDigest();
	static FName CanonicalDetectorId();
	static FName CanonicalFormulaId();
	static FGuid CanonicalConfigId();
	static bool TryCreateCanonical(
		Fdemo_mapShanmenSwordQiProductConfig& OutConfig);

	bool IsValid() const;
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FShanmenSwordQiDefinition& GetDefinition() const
	{
		return Definition;
	}

private:
	FGuid ConfigId;
	FShanmenContentStamp Content;
	FShanmenSwordQiDefinition Definition;
};

/** Run-issued identity and frozen action for one exact sword-qi attempt. */
class Fdemo_mapPlayerSwordQiActionReservation
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

/**
 * Frozen, device-independent launch command.
 *
 * Its identity is the Run-issued ActivationId. Replaying the same command can
 * therefore never reserve another sequence or create a second carrier.
 */
class Fdemo_mapShanmenSwordQiLaunchCommand
{
public:
	static bool TryCapture(
		const Fdemo_mapPlayerSwordQiActionReservation& Reservation,
		const Fdemo_mapShanmenSwordQiProductConfig& Config,
		const FShanmenSwordQiOffenseSnapshot& Offense,
		const FVector& Origin,
		const FVector& AimDirection,
		Fdemo_mapShanmenSwordQiLaunchCommand& OutCommand);

	bool IsValid() const;
	bool Matches(const Fdemo_mapShanmenSwordQiLaunchCommand& Other) const;
	const FGuid& GetCommandId() const { return Action.GetActivationId(); }
	const FGuid& GetRunId() const { return Action.GetRunId(); }
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSwordQiDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenSwordQiOffenseSnapshot& GetOffense() const
	{
		return Offense;
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetAimDirection() const { return AimDirection; }

private:
	FGuid ConfigId;
	FShanmenCombatActionSnapshot Action;
	FShanmenSwordQiDefinition Definition;
	FShanmenSwordQiOffenseSnapshot Offense;
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ZeroVector;
};

enum class Edemo_mapShanmenSwordQiProductStartStatus : uint8
{
	Ready,
	InvalidSourceItem,
	InvalidOffense,
	InvalidOrigin,
	InvalidDirection,
	ConfigUnavailable,
	ReservationRejected,
	CommandRejected
};

/** Complete pre-route proof joining product policy, Run identity and command. */
struct Fdemo_mapShanmenSwordQiProductStartResult
{
	Edemo_mapShanmenSwordQiProductStartStatus Status =
		Edemo_mapShanmenSwordQiProductStartStatus::ConfigUnavailable;
	Fdemo_mapShanmenSwordQiProductConfig Config;
	Fdemo_mapPlayerSwordQiActionReservation Reservation;
	Fdemo_mapShanmenSwordQiLaunchCommand Command;
	FString Diagnostic;

	bool IsReady() const;
};

/**
 * Product composition boundary before player-action arbitration and routing.
 *
 * Every externally sampled value is validated before the Run sequence is
 * consumed. The exact sword identity must come from the caller's existing
 * equipment authority; this class neither looks up nor mutates inventory.
 */
struct Fdemo_mapShanmenSwordQiProductAuthority
{
	static Fdemo_mapShanmenSwordQiProductStartResult PrepareLaunch(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& SourceItemInstanceId,
		float AttackPower,
		const FVector& Origin,
		const FVector& AimDirection);
};
