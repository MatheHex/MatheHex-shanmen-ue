#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationProductController.h"
#include "demo_mapShanmenFormationScatterWorldPublicationRunRoute.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Product-visible outcome for one ordered formation -> Combat Run end. */
enum class Edemo_mapShanmenFormationRunLifecycleEndStatus : uint8
{
	Ended,
	ProductTeardownComplete,
	LifecycleInactive,
	LifecycleInvalid,
	CoordinatorNotActive,
	RunMismatch,
	ScatterPublicationTeardownRejected,
	ProductTeardownRejected,
	CoordinatorEndRejected
};

/** Auditable result for one forward-only formation Run teardown attempt. */
struct Fdemo_mapShanmenFormationRunLifecycleEndResult
{
	Edemo_mapShanmenFormationRunLifecycleEndStatus Status =
		Edemo_mapShanmenFormationRunLifecycleEndStatus::LifecycleInactive;
	FGuid RunId;
	bool bHadScatterPublicationRoute = false;
	bool bReusedScatterPublicationTeardown = false;
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
		ScatterPublicationTeardown;
	bool bReusedProductTeardown = false;
	Fdemo_mapShanmenFormationControllerEndSummary ProductTeardown;
	FString Diagnostic;

	bool HasValidScatterPublicationTeardown() const
	{
		return !bHadScatterPublicationRoute
			|| (ScatterPublicationTeardown.IsSuccess()
				&& ScatterPublicationTeardown.RunId == RunId
				&& ScatterPublicationTeardown.Event
					== Edemo_mapShanmenFormationScatterWorldPublicationRunEvent::End);
	}

	bool IsEnded() const
	{
		return Status
			== Edemo_mapShanmenFormationRunLifecycleEndStatus::Ended
			&& RunId.IsValid()
			&& HasValidScatterPublicationTeardown()
			&& ProductTeardown.IsValid();
	}

	bool IsProductTeardownComplete() const
	{
		return Status
			== Edemo_mapShanmenFormationRunLifecycleEndStatus::
				ProductTeardownComplete
			&& RunId.IsValid()
			&& HasValidScatterPublicationTeardown()
			&& ProductTeardown.IsValid();
	}
};

/**
 * Sole composition owner for one formation controller, optional scatter
 * publication route and its Combat Run end.
 *
 * Scatter publication ends before formation product teardown, which completes
 * before shared combat identities are released. Because either teardown can
 * remove World actors or mutate durable material authority, each successful
 * step is checkpointed independently. Exact retries reuse those checkpoints
 * and never re-enter World or cancel the product twice.
 */
class Fdemo_mapShanmenFormationRunLifecycle
{
public:
	/** Move the complete live lifecycle; the previous owner becomes empty. */
	static bool TryTakeover(
		Fdemo_mapShanmenFormationRunLifecycle& Previous,
		Fdemo_mapShanmenFormationRunLifecycle& OutLifecycle);

	bool TryBegin(
		Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);
	/** Opens or exactly replays the sole scatter-publication route slot. */
	bool TryOpenScatterPublicationRoute(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		TSubclassOf<AActor> ActorClass,
		FString& OutDiagnostic);
	/** Routes the explicit product Publish event through the sole route slot. */
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
	TryPublishScatterPublication(UWorld* World);

	Fdemo_mapShanmenFormationControllerResult TrySubmit(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductController&
			SpiritEnergyController,
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
	/**
	 * Completes formation-owned material and World cleanup without releasing
	 * shared Combat Run identities. The durable result is reused on retries.
	 */
	Fdemo_mapShanmenFormationRunLifecycleEndResult TryTeardownProduct(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		UWorld* World,
		Fdemo_mapCombatRunCoordinator& Coordinator);
	/**
	 * Clears the checkpointed lifecycle only after the outer composition owner
	 * has released the exact shared Combat Run.
	 */
	bool TryAcknowledgeCoordinatorEnded(
		const FGuid& ExpectedRunId,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const;
	bool HasProductTeardownCheckpoint() const
	{
		return ProductTeardownCheckpoint.IsSet();
	}
	bool HasScatterPublicationRoute() const
	{
		return ScatterPublicationRoute.IsSet();
	}
	bool HasScatterPublicationTeardownCheckpoint() const
	{
		return ScatterPublicationTeardownCheckpoint.IsSet();
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
	const Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute*
	GetScatterPublicationRoute() const
	{
		return ScatterPublicationRoute.IsSet()
			? &ScatterPublicationRoute.GetValue()
			: nullptr;
	}
	const Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult*
	GetScatterPublicationTeardownCheckpoint() const
	{
		return ScatterPublicationTeardownCheckpoint.IsSet()
			? &ScatterPublicationTeardownCheckpoint.GetValue()
			: nullptr;
	}

private:
	void Clear();

	FGuid RunId;
	Fdemo_mapShanmenFormationProductController Controller;
	TOptional<Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute>
		ScatterPublicationRoute;
	TOptional<
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult>
		ScatterPublicationTeardownCheckpoint;
	TOptional<Fdemo_mapShanmenFormationControllerEndSummary>
		ProductTeardownCheckpoint;
};
