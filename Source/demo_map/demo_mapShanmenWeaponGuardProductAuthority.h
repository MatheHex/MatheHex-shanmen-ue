#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenWeaponGuardProductHost.h"

class Fdemo_mapCombatRunCoordinator;

/**
 * Immutable product-owned content, timing and facing policy for weapon guard.
 *
 * Runtime callers supply only the exact equipped-item identity and a sample
 * from their monotonic timeline. Input, GameMode and the Run coordinator may
 * consume this snapshot but may not choose alternative balance values.
 */
class Fdemo_mapShanmenWeaponGuardProductConfig
{
public:
	static FName CanonicalContentVersion();
	static FString CanonicalContentDigest();
	static FName CanonicalDefenseRuleId();
	static FName CanonicalPerfectRuleId();
	static FName CanonicalArcRuleId();
	static float CanonicalGuardFraction();
	static int64 CanonicalPerfectWindowTickCount();
	static double CanonicalMinimumFacingDot();
	static FGuid CanonicalConfigId();
	static bool TryCreateCanonical(
		Fdemo_mapShanmenWeaponGuardProductConfig& OutConfig);

	bool IsValid() const;
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FShanmenWeaponGuardDefinition& GetDefinition() const
	{
		return Definition;
	}
	FName GetPerfectRuleId() const { return PerfectRuleId; }
	FName GetArcRuleId() const { return ArcRuleId; }
	int64 GetPerfectWindowTickCount() const
	{
		return PerfectWindowTickCount;
	}
	double GetMinimumFacingDot() const { return MinimumFacingDot; }

private:
	FGuid ConfigId;
	FShanmenContentStamp Content;
	FShanmenWeaponGuardDefinition Definition;
	FName PerfectRuleId = NAME_None;
	FName ArcRuleId = NAME_None;
	int64 PerfectWindowTickCount = 0;
	double MinimumFacingDot = 0.0;
};

/** Run-issued identity and frozen item-backed action for one guard attempt. */
class Fdemo_mapPlayerWeaponGuardActionReservation
{
public:
	bool IsValid() const;
	uint64 GetActivationSequence() const { return ActivationSequence; }
	const FGuid& GetConfigId() const { return ConfigId; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FGuid& GetActivationId() const { return Action.GetActivationId(); }

private:
	friend class Fdemo_mapCombatRunCoordinator;

	uint64 ActivationSequence = 0;
	FGuid ConfigId;
	FGuid SourceItemInstanceId;
	FShanmenCombatActionSnapshot Action;
};

enum class Edemo_mapShanmenWeaponGuardProductStartStatus : uint8
{
	Ready,
	InvalidSourceItem,
	InvalidTimeline,
	ConfigUnavailable,
	ReservationRejected,
	HostRejected
};

/** Complete proof joining canonical config, Run identity and one active host. */
struct Fdemo_mapShanmenWeaponGuardProductStartResult
{
	Edemo_mapShanmenWeaponGuardProductStartStatus Status =
		Edemo_mapShanmenWeaponGuardProductStartStatus::ConfigUnavailable;
	Fdemo_mapShanmenWeaponGuardProductConfig Config;
	Fdemo_mapPlayerWeaponGuardActionReservation Reservation;
	Fdemo_mapShanmenWeaponGuardHostStartResult HostStart;
	Fdemo_mapShanmenWeaponGuardProductHost Host;
	FString Diagnostic;

	bool IsReady() const;
};

/**
 * Pure product composition boundary before command, input and Impact routing.
 *
 * The caller must obtain SourceItemInstanceId from the existing equipment
 * authority and timing from an existing monotonic clock. This layer validates
 * both before consuming Run sequence state, reserves deterministic action
 * identity, and starts P11.5's sole per-action host. It owns no inventory,
 * Actor, component, input binding, clock, durability or incoming Impact route.
 */
struct Fdemo_mapShanmenWeaponGuardProductAuthority
{
	static Fdemo_mapShanmenWeaponGuardProductStartResult PrepareStart(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& SourceItemInstanceId,
		const FGuid& TimelineId,
		int64 ActiveStartTick);
};
