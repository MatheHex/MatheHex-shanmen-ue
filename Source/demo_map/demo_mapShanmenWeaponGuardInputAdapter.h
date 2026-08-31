#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenWeaponGuardProductSession.h"

/** One opaque sample from the caller-owned monotonic guard timeline. */
class Fdemo_mapShanmenWeaponGuardInputTimelineSample
{
public:
	static bool TryCapture(
		const FGuid& TimelineId,
		int64 ActiveStartTick,
		Fdemo_mapShanmenWeaponGuardInputTimelineSample& OutSample);

	bool IsValid() const;
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetActiveStartTick() const { return ActiveStartTick; }

private:
	FGuid TimelineId;
	int64 ActiveStartTick = INDEX_NONE;
};

enum class Edemo_mapShanmenWeaponGuardInputStatus : uint8
{
	Applied,
	GameplayBlocked,
	ProductRouteUnavailable,
	TimelineRejected,
	ProductRejected
};

/** Proof that one eligible input sampled and delegated at most once. */
struct Fdemo_mapShanmenWeaponGuardInputResult
{
	Edemo_mapShanmenWeaponGuardInputStatus Status =
		Edemo_mapShanmenWeaponGuardInputStatus::GameplayBlocked;
	bool bTimelineSampled = false;
	bool bProductRouteInvoked = false;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample TimelineSample;
	Fdemo_mapShanmenWeaponGuardProductRouteResult ProductRoute;
	FString Diagnostic;

	bool IsAccepted() const;
};

enum class Edemo_mapShanmenWeaponGuardReleaseInputStatus : uint8
{
	Applied,
	ProductRouteUnavailable,
	ProductRejected
};

/** Proof that a physical release delegates once and never consults UI lock. */
struct Fdemo_mapShanmenWeaponGuardReleaseInputResult
{
	Edemo_mapShanmenWeaponGuardReleaseInputStatus Status =
		Edemo_mapShanmenWeaponGuardReleaseInputStatus::ProductRouteUnavailable;
	bool bProductRouteInvoked = false;
	Fdemo_mapShanmenWeaponGuardSessionTransitionResult Transition;
	FString Diagnostic;

	bool IsAccepted() const;
};

/**
 * Stateless device-to-product seam for a future weapon-guard press.
 *
 * The existing gameplay gate and route availability are checked before the
 * external monotonic timeline is sampled. An eligible input samples exactly
 * once and delegates only to P11.8. It owns no key, item identity, clock,
 * Run, host, retry, balance, Actor or incoming Impact route.
 */
struct Fdemo_mapShanmenWeaponGuardInputAdapter
{
	static Fdemo_mapShanmenWeaponGuardInputResult RouteStartInput(
		bool bGameplayInputAllowed,
		bool bProductRouteAvailable,
		TFunctionRef<Fdemo_mapShanmenWeaponGuardInputTimelineSample()>
			SampleTimeline,
		TFunctionRef<Fdemo_mapShanmenWeaponGuardProductRouteResult(
			const FGuid& TimelineId,
			int64 ActiveStartTick)> RouteStartIntent);

	/** Release intentionally has no gameplay gate, preventing a stuck hold. */
	static Fdemo_mapShanmenWeaponGuardReleaseInputResult RouteReleaseInput(
		bool bProductRouteAvailable,
		TFunctionRef<Fdemo_mapShanmenWeaponGuardSessionTransitionResult()>
			RouteReleaseIntent);
};
