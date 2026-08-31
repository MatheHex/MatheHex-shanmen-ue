#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenWeaponGuardDefenseCoordinator.h"

enum class Edemo_mapShanmenWeaponGuardHostStartStatus : uint8
{
	Rejected,
	Started
};

enum class Edemo_mapShanmenWeaponGuardHostStartError : uint8
{
	None,
	InvalidInput,
	ActionStartRejected,
	ActionCommitRejected,
	WindowRejected,
	TimingPolicyRejected,
	ArcPolicyRejected,
	StateDesynchronized
};

/** Full proof that one product host owns the exact committed guard chain. */
struct Fdemo_mapShanmenWeaponGuardHostStartResult
{
	Edemo_mapShanmenWeaponGuardHostStartStatus Status =
		Edemo_mapShanmenWeaponGuardHostStartStatus::Rejected;
	Edemo_mapShanmenWeaponGuardHostStartError Error =
		Edemo_mapShanmenWeaponGuardHostStartError::InvalidInput;
	FGuid HostId;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt ActiveCommit;
	FShanmenWeaponGuardWindowReceipt Window;
	FShanmenWeaponPerfectGuardPolicy TimingPolicy;
	FShanmenWeaponGuardArcPolicy ArcPolicy;

	bool IsValid() const;
	bool IsSuccess() const;
};

enum class Edemo_mapShanmenWeaponGuardHostDefenseStatus : uint8
{
	Rejected,
	ComposedQualified,
	ComposedOutsideArc
};

enum class Edemo_mapShanmenWeaponGuardHostDefenseError : uint8
{
	None,
	HostNotReady,
	HostNotActive,
	NonMonotonicObservation,
	ObservationRejected,
	CompositionRejected,
	StateDesynchronized
};

/** One host-bound, monotonic per-impact defense composition. */
struct Fdemo_mapShanmenWeaponGuardHostDefenseResult
{
	Edemo_mapShanmenWeaponGuardHostDefenseStatus Status =
		Edemo_mapShanmenWeaponGuardHostDefenseStatus::Rejected;
	Edemo_mapShanmenWeaponGuardHostDefenseError Error =
		Edemo_mapShanmenWeaponGuardHostDefenseError::HostNotReady;
	FGuid HostId;
	FGuid ReceiptId;
	FShanmenWeaponGuardTimelineObservation Observation;
	Fdemo_mapShanmenWeaponGuardDefenseResult Composition;

	bool IsValid() const;
	bool IsSuccess() const;
	bool HasGuardLayer() const
	{
		return IsSuccess()
			&& Status
				== Edemo_mapShanmenWeaponGuardHostDefenseStatus::
				ComposedQualified;
	}
};

enum class Edemo_mapShanmenWeaponGuardHostTransitionStatus : uint8
{
	Rejected,
	EnteredRecovery,
	Completed,
	Interrupted,
	AlreadyTerminal
};

enum class Edemo_mapShanmenWeaponGuardHostTransitionError : uint8
{
	None,
	HostNotReady,
	HostNotActive,
	ActionTransitionRejected,
	StateDesynchronized
};

/** One accepted or rejected host lifecycle operation. */
struct Fdemo_mapShanmenWeaponGuardHostTransitionResult
{
	Edemo_mapShanmenWeaponGuardHostTransitionStatus Status =
		Edemo_mapShanmenWeaponGuardHostTransitionStatus::Rejected;
	Edemo_mapShanmenWeaponGuardHostTransitionError Error =
		Edemo_mapShanmenWeaponGuardHostTransitionError::HostNotReady;
	FGuid HostId;
	FShanmenActionTransitionReceipt Transition;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Sole value-type product owner for one active weapon-guard action.
 *
 * The caller supplies authored policies and explicit monotonic ticks. This
 * host owns the action runtime, exact guard window and policy bindings, then
 * delegates each impact composition to P11.4. It stores no Actor, World,
 * component, Tick, Timer, input binding, resource balance or durability.
 */
class Fdemo_mapShanmenWeaponGuardProductHost
{
public:
	static Fdemo_mapShanmenWeaponGuardHostStartResult TryStart(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenWeaponGuardDefinition& Definition,
		const FGuid& TimelineId,
		int64 ActiveStartTick,
		int64 PerfectEndTick,
		FName PerfectRuleId,
		FName ArcRuleId,
		double MinimumFacingDot,
		Fdemo_mapShanmenWeaponGuardProductHost& OutHost);

	Fdemo_mapShanmenWeaponGuardHostDefenseResult TryComposeDefense(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* DefenderActor,
		AActor* ThreatActor,
		int64 ObservedTick,
		const FShanmenHitCandidate& Candidate,
		const FShanmenDefenseSnapshot& BaseDefense);

	Fdemo_mapShanmenWeaponGuardHostTransitionResult TryEnterRecovery();
	Fdemo_mapShanmenWeaponGuardHostTransitionResult TryComplete();
	Fdemo_mapShanmenWeaponGuardHostTransitionResult TryInterrupt();

	bool IsValid() const;
	bool IsActive() const;
	bool IsRecovery() const;
	bool IsTerminal() const;
	const FGuid& GetHostId() const { return HostId; }
	int64 GetLastObservedTick() const { return LastObservedTick; }
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const FShanmenWeaponGuardWindow& GetWindow() const { return Window; }
	const FShanmenWeaponPerfectGuardPolicy& GetTimingPolicy() const
	{
		return TimingPolicy;
	}
	const FShanmenWeaponGuardArcPolicy& GetArcPolicy() const
	{
		return ArcPolicy;
	}

private:
	FGuid HostId;
	int64 LastObservedTick = INDEX_NONE;
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponPerfectGuardPolicy TimingPolicy;
	FShanmenWeaponGuardArcPolicy ArcPolicy;
	bool bStarted = false;
};
