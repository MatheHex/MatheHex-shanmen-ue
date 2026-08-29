#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationCoverageCoordinator.h"
#include "demo_mapShanmenFormationInfluenceDispatchLedger.h"
#include "demo_mapShanmenFormationWorldAdapter.h"

class AActor;
class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationHostStatus : uint8
{
	Prepared,
	CommittedPendingPlacement,
	Placed,
	Replayed,
	Cancelled,
	Ended,
	TeardownReplayed,
	HostInvalid,
	CorrelationMismatch,
	RequestInvalid,
	PlacementPending,
	PlacementMissing,
	PlacementBindingConflict,
	SessionRejected,
	WorldRejected,
	TerminalConflict,
	TerminalRecoveryRequired,
	InfluenceTerminalRequired
};

/** One explicit product-host operation result; nested authority receipts remain visible. */
struct Fdemo_mapShanmenFormationHostResult
{
	Edemo_mapShanmenFormationHostStatus Status =
		Edemo_mapShanmenFormationHostStatus::RequestInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationSessionResult Session;
	Fdemo_mapShanmenFormationWorldResult World;
	Fdemo_mapShanmenFormationAnchorPlacementIntent PlacementIntent;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationHostCoverageStatus : uint8
{
	Coordinated,
	Reset,
	ResetReplayed,
	HostInvalid,
	CorrelationMismatch,
	SessionNotActive,
	SessionTerminal,
	PlacementPending,
	PlacementIncomplete,
	WorldMismatch,
	AreaRejected,
	CoordinatorRejected,
	InfluenceOrchestrationRequired
};

/** Coverage operation result owned by the existing formation product host. */
struct Fdemo_mapShanmenFormationHostCoverageResult
{
	Edemo_mapShanmenFormationHostCoverageStatus Status =
		Edemo_mapShanmenFormationHostCoverageStatus::HostInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationAreaBuildResult Area;
	Fdemo_mapShanmenFormationCoverageCoordinatorResult Coordination;
	Fdemo_mapShanmenFormationCoverageTrackerResult TrackerReset;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationHostInfluenceStatus : uint8
{
	Coordinated,
	CoordinateReplayed,
	Reset,
	ResetReplayed,
	TerminalPrepared,
	TerminalReplayed,
	RetryRecorded,
	RetryReplayed,
	Acknowledged,
	AcknowledgementReplayed,
	Sealed,
	SealReplayed,
	HostInvalid,
	CorrelationMismatch,
	RequestInvalid,
	LifecycleConflict,
	CoverageRejected,
	PlannerRejected,
	DispatchRejected,
	AcknowledgementRejected,
	SealRejected,
	StateInvalid
};

/** One host-owned orchestration result with every nested immutable receipt. */
struct Fdemo_mapShanmenFormationHostInfluenceResult
{
	Edemo_mapShanmenFormationHostInfluenceStatus Status =
		Edemo_mapShanmenFormationHostInfluenceStatus::RequestInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationHostCoverageResult Coverage;
	Fdemo_mapShanmenFormationInfluencePlanResult TransitionPlan;
	Fdemo_mapShanmenFormationInfluenceReconciliationResult ReconciliationPlan;
	Fdemo_mapShanmenFormationInfluenceSubmitResult Dispatch;
	Fdemo_mapShanmenFormationInfluenceAcknowledgeResult Acknowledgement;
	Fdemo_mapShanmenFormationInfluenceSealResult Seal;

	bool IsSuccess() const;
};

/**
 * Sole transient product owner for formation material, deployment, and World delivery.
 *
 * The host deliberately exposes prepare -> durable commit -> placement as three
 * commands. Once commit publishes a placement intent, no different anchor may
 * advance until that exact intent is placed or the deploying session cancels.
 * World failure is forward-only: the committed material is never rolled back and
 * exact retries are fenced to the first valid World / Actor-class binding.
 */
class Fdemo_mapShanmenFormationProductHost
{
public:
	static bool TryStart(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenFormationDiagramDefinition& Diagram,
		const FVector& Origin,
		const FVector& Forward,
		Fdemo_mapShanmenFormationProductHost& OutHost,
		FShanmenActionTransitionReceipt& OutStartup,
		FShanmenActionTransitionReceipt& OutActive,
		FShanmenFormationDeploymentReceipt& OutBegin);

	Fdemo_mapShanmenFormationHostResult TryPrepareAnchor(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);
	Fdemo_mapShanmenFormationHostResult TryCommitPreparedAnchor(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);
	Fdemo_mapShanmenFormationHostResult TryPlaceCommittedAnchor(
		UWorld* World,
		TSubclassOf<AActor> ActorClass,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);
	Fdemo_mapShanmenFormationHostResult TryCancelAndTeardown(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		UWorld* World,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationHostResult TryEndAndTeardown(
		UWorld* World,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationHostCoverageResult TryCoordinateCoverage(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const TArray<AActor*>& SourceActors,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationCoverageCommand& Command);
	Fdemo_mapShanmenFormationHostCoverageResult TryResetCoverage(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationHostInfluenceResult TryCoordinateInfluence(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const TArray<AActor*>& SourceActors,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationCoverageCommand& Command,
		const Fdemo_mapShanmenFormationInfluencePolicy& Policy);
	Fdemo_mapShanmenFormationHostInfluenceResult TryResetInfluence(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationHostInfluenceResult TryPrepareTerminalInfluence(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationHostInfluenceResult TryAcknowledgeInfluence(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationInfluenceAttemptCommand& Command);
	Fdemo_mapShanmenFormationHostInfluenceResult TrySealInfluence(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);

	bool IsValid() const;
	bool HasCoverageBaseline() const { return CoverageTracker.IsPrimed(); }
	bool HasInfluenceAuthority() const
	{
		return InfluenceLedger.GetLedgerId().IsValid();
	}
	bool HasActiveInfluenceScope() const
	{
		return ActiveInfluenceScope.IsSet();
	}
	bool IsInfluenceTerminalPrepared() const
	{
		return bInfluenceTerminalPrepared;
	}
	int32 GetPendingInfluenceIntentCount() const
	{
		return InfluenceLedger.GetPendingIntentCount();
	}
	bool TryPeekNextInfluenceIntent(
		Fdemo_mapShanmenFormationInfluenceIntent& OutIntent) const
	{
		return InfluenceLedger.TryPeekNextPending(OutIntent);
	}
	const Fdemo_mapShanmenFormationInfluenceDispatchLedger&
	GetInfluenceLedger() const
	{
		return InfluenceLedger;
	}
	bool TryGetCoverageBaseline(
		Fdemo_mapShanmenFormationCoverageReceipt& OutBaseline) const
	{
		return CoverageTracker.TryGetBaseline(OutBaseline);
	}
	bool HasPendingPlacement() const { return bHasPendingPlacement; }
	const Fdemo_mapShanmenFormationAnchorPlacementIntent*
	GetPendingPlacement() const
	{
		return bHasPendingPlacement ? &PendingPlacement : nullptr;
	}
	const Fdemo_mapShanmenFormationProductSession& GetSession() const
	{
		return Session;
	}
	const Fdemo_mapShanmenFormationWorldAdapter& GetWorldAdapter() const
	{
		return WorldAdapter;
	}

private:
	Fdemo_mapShanmenFormationHostCoverageResult CoordinateCoverageInternal(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const TArray<AActor*>& SourceActors,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationCoverageCommand& Command);
	Fdemo_mapShanmenFormationHostCoverageResult ResetCoverageInternal(
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	bool CanTeardownInfluence(FString& OutDiagnostic) const;
	void ClearPendingPlacement();
	bool PendingMatches(FName AnchorDefinitionId, const FGuid& AttemptId) const;

	Fdemo_mapShanmenFormationProductSession Session;
	Fdemo_mapShanmenFormationWorldAdapter WorldAdapter;
	Fdemo_mapShanmenFormationCoverageTracker CoverageTracker;
	Fdemo_mapShanmenFormationInfluenceDispatchLedger InfluenceLedger;
	TOptional<Fdemo_mapShanmenFormationInfluenceScope> ActiveInfluenceScope;
	TOptional<Fdemo_mapShanmenFormationInfluenceReconciliationBatch>
		LastInfluenceClearBatch;
	FGuid InfluenceSourceEntityId;
	Fdemo_mapShanmenFormationAnchorPlacementIntent PendingPlacement;
	TWeakObjectPtr<UWorld> BoundPlacementWorld;
	TSubclassOf<AActor> BoundPlacementClass;
	FString BoundPlacementClassPath;
	bool bHasPendingPlacement = false;
	bool bPlacementBindingFrozen = false;
	bool bInfluenceTerminalPrepared = false;
};
