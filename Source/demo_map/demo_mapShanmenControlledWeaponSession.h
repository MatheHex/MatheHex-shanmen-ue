#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponAdapter.h"

enum class Edemo_mapShanmenControlledWeaponSessionState : uint8
{
	Empty,
	Active,
	Completed,
	Interrupted
};

/**
 * Product-session owner for one authority-approved controlled weapon.
 *
 * Actor movement, input, collision sampling, vitality delivery, and item
 * mutation remain outside. This object atomically coordinates the existing
 * action lifecycle with P6.0 commands and contact emissions so product callers
 * cannot recall a sword while leaving an Active action or open detector.
 */
class Fdemo_mapShanmenControlledWeaponSession
{
public:
	static bool TryStart(
		const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
		Fdemo_mapShanmenControlledWeaponSession& OutSession,
		FShanmenActionTransitionReceipt& OutStartup,
		FShanmenActionTransitionReceipt& OutActive);

	bool IsValid() const;
	bool IsActive() const;
	bool IsTerminal() const;

	/** Launch or redirect only; Recall must use TryRecallAndComplete. */
	bool TryIssueControl(
		int64 ExpectedSequence,
		EShanmenControlledWeaponCommandKind Kind,
		const FVector& DesiredDirection,
		FShanmenControlledWeaponCommandReceipt& OutReceipt);
	bool TryBeginOrbitThreatWindow(FShanmenWorldHitContext& OutContext);
	bool TryAcceptOrbitThreatCandidate(const FShanmenHitCandidate& Candidate);
	bool TryEndOrbitThreatWindow(FShanmenDetectorEmissionReceipt& OutReceipt);
	bool TryEndOrbitThreatWindow();
	bool TryEvaluateOrbitThreatReceipt(
		const FShanmenDetectorEmissionReceipt& Emission,
		const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
		FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const;
	bool TryBeginContactWindow(FShanmenWorldHitContext& OutContext);
	bool TryResolveCandidate(
		const FShanmenHitCandidate& Candidate,
		const FShanmenTargetVitalitySnapshot& TargetVitality,
		const FShanmenDefenseSnapshot& Defense,
		FShanmenControlledWeaponImpactReceipt& OutReceipt);
	bool TryEndContactWindow();

	/** Recalls the item and completes Active -> Recovery -> Completed atomically. */
	bool TryRecallAndComplete(
		int64 ExpectedSequence,
		FShanmenControlledWeaponCommandReceipt& OutRecall,
		FShanmenActionTransitionReceipt& OutRecovery,
		FShanmenActionTransitionReceipt& OutCompleted);

	/** Closes any contact window and interrupts the action atomically. */
	bool TryInterrupt(FShanmenActionTransitionReceipt& OutInterrupted);
	void Reset();

	Edemo_mapShanmenControlledWeaponSessionState GetState() const
	{
		return State;
	}
	const Fdemo_mapShanmenControlledWeaponAuthorityEvidence& GetEvidence() const
	{
		return Evidence;
	}
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const FShanmenControlledWeaponExecution& GetExecution() const
	{
		return Execution;
	}

private:
	Fdemo_mapShanmenControlledWeaponAuthorityEvidence Evidence;
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenControlledWeaponExecution Execution;
	Edemo_mapShanmenControlledWeaponSessionState State =
		Edemo_mapShanmenControlledWeaponSessionState::Empty;
};
