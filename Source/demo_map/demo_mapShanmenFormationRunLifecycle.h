#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationProductController.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Product-visible outcome for one ordered formation -> Combat Run end. */
enum class Edemo_mapShanmenFormationRunLifecycleEndStatus : uint8
{
	Ended,
	LifecycleInactive,
	LifecycleInvalid,
	CoordinatorNotActive,
	RunMismatch,
	ProductTeardownRejected,
	CoordinatorEndRejected
};

/** Auditable result for one forward-only formation Run teardown attempt. */
struct Fdemo_mapShanmenFormationRunLifecycleEndResult
{
	Edemo_mapShanmenFormationRunLifecycleEndStatus Status =
		Edemo_mapShanmenFormationRunLifecycleEndStatus::LifecycleInactive;
	FGuid RunId;
	bool bReusedProductTeardown = false;
	Fdemo_mapShanmenFormationControllerEndSummary ProductTeardown;
	FString Diagnostic;

	bool IsEnded() const
	{
		return Status
			== Edemo_mapShanmenFormationRunLifecycleEndStatus::Ended
			&& RunId.IsValid()
			&& ProductTeardown.IsValid();
	}
};

/**
 * Sole composition owner for one formation controller and its Combat Run end.
 *
 * Product teardown always completes before shared combat identities are
 * released. Because cancellation can mutate durable material authority and
 * remove World actors, successful product teardown is checkpointed. If the
 * Coordinator rejects its release, an exact retry reuses that checkpoint and
 * never cancels the product twice.
 */
class Fdemo_mapShanmenFormationRunLifecycle
{
public:
	bool TryBegin(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);

	Fdemo_mapShanmenFormationControllerResult TrySubmit(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenFormationIntent& Intent);
	Fdemo_mapShanmenFormationAnchorOperationResult TryExecuteAnchorOperation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		TSubclassOf<AActor> ActorClass,
		const Fdemo_mapShanmenFormationAnchorOperation& Operation);

	Fdemo_mapShanmenFormationRunLifecycleEndResult TryEndRun(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		UWorld* World,
		Fdemo_mapCombatRunCoordinator& Coordinator);

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const;
	bool HasProductTeardownCheckpoint() const
	{
		return ProductTeardownCheckpoint.IsSet();
	}
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenFormationProductController& GetController() const
	{
		return Controller;
	}
	const Fdemo_mapShanmenFormationControllerEndSummary*
	GetProductTeardownCheckpoint() const
	{
		return ProductTeardownCheckpoint.IsSet()
			? &ProductTeardownCheckpoint.GetValue()
			: nullptr;
	}

private:
	void Clear();

	FGuid RunId;
	Fdemo_mapShanmenFormationProductController Controller;
	TOptional<Fdemo_mapShanmenFormationControllerEndSummary>
		ProductTeardownCheckpoint;
};
