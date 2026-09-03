#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseProductController.h"

class Fdemo_mapCombatRunCoordinator;

/** Run-issued identity and frozen action for one exact Divine Sense pulse. */
class Fdemo_mapPlayerDivineSenseActionReservation
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

enum class Edemo_mapShanmenDivineSenseProductPrepareStatus : uint8
{
	Ready,
	ConfigUnavailable,
	ReservationRejected,
	IntentRejected
};

/** Complete pre-controller proof joining canonical policy, Run identity and Intent. */
struct Fdemo_mapShanmenDivineSenseProductPrepareResult
{
	Edemo_mapShanmenDivineSenseProductPrepareStatus Status =
		Edemo_mapShanmenDivineSenseProductPrepareStatus::ConfigUnavailable;
	Fdemo_mapShanmenDivineSenseProductConfig Config;
	Fdemo_mapPlayerDivineSenseActionReservation Reservation;
	Fdemo_mapShanmenDivineSenseProductIntent Intent;
	FString Diagnostic;

	bool IsReady() const;
};

/**
 * Sole P19.7 product authority for Divine Sense policy and activation identity.
 *
 * These values are versioned P-stage defaults, not final balance. Input and
 * GameMode may request a pulse, but cannot select its definition, cost,
 * capacity, action snapshot, scan ordinal, subject budget or IntentId.
 * Resource balances, World evidence and Controller lifecycle stay with the
 * existing P19.1-P19.6 authorities.
 */
struct Fdemo_mapShanmenDivineSenseProductAuthority
{
	static FName CanonicalContentVersion();
	static FString CanonicalContentDigest();
	static FName CanonicalScanRuleId();
	static FName CanonicalCostRuleId();
	static double CanonicalRadius();
	static int32 CanonicalMaximumResults();
	static EShanmenDivineSenseOcclusionPolicy CanonicalOcclusionPolicy();
	static float CanonicalSpiritEnergyCost();
	static int32 CanonicalPulseCapacity();
	static int32 CanonicalScanOrdinal();
	static int32 CanonicalSubjectActorBudget();
	static FGuid CanonicalConfigId();

	static bool TryCreateCanonicalConfig(
		Fdemo_mapShanmenDivineSenseProductConfig& OutConfig);
	static bool IsCanonicalConfig(
		const Fdemo_mapShanmenDivineSenseProductConfig& Config);
	static FGuid MakeIntentId(
		const Fdemo_mapPlayerDivineSenseActionReservation& Reservation);
	static Fdemo_mapShanmenDivineSenseProductPrepareResult PrepareIntent(
		Fdemo_mapCombatRunCoordinator& Coordinator);
};
