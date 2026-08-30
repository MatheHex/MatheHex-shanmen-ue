#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceExecutionService.h"

class UWorld;

enum class Edemo_mapShanmenFormationInfluenceLifecycleStatus : uint8
{
	StepAccepted,
	StepRejected,
	TerminalPrepared,
	TerminalReplayed,
	Completed,
	CompletionReplayed,
	HostInvalid,
	CorrelationMismatch,
	LedgerUnavailable,
	BindingConflict,
	TerminalRejected,
	SealRejected,
	EndRejected,
	StateInvalid
};

/** One explicit lifecycle operation with every nested authority receipt. */
struct Fdemo_mapShanmenFormationInfluenceLifecycleResult
{
	Edemo_mapShanmenFormationInfluenceLifecycleStatus Status =
		Edemo_mapShanmenFormationInfluenceLifecycleStatus::StateInvalid;
	FString Diagnostic;
	bool bCoordinatorStateCommitted = false;
	Fdemo_mapShanmenFormationInfluenceServiceResult Step;
	Fdemo_mapShanmenFormationHostInfluenceResult TerminalPreparation;
	Fdemo_mapShanmenFormationHostInfluenceResult Seal;
	Fdemo_mapShanmenFormationHostResult End;

	bool IsSuccess() const;
};

/**
 * Caller-driven lifecycle boundary around one P8.19 execution Service.
 *
 * The Coordinator freezes one Run/Ledger binding and exposes only three
 * explicit operations: execute one caller request, prepare terminal intents,
 * and seal plus end after the caller drains every intent. It never discovers
 * requests, loops, retries, schedules, owns ProductHost, or touches World
 * objects beyond forwarding the caller-owned pointer to Host teardown.
 */
class Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator
{
public:
	Fdemo_mapShanmenFormationInfluenceLifecycleResult TryExecuteStep(
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request);
	Fdemo_mapShanmenFormationInfluenceLifecycleResult TryPrepareTerminal(
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);
	Fdemo_mapShanmenFormationInfluenceLifecycleResult TrySealAndEnd(
		UWorld* World,
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);

	bool IsValid() const;
	bool IsBound() const { return BoundLedgerId.IsValid(); }
	const FGuid& GetBoundLedgerId() const { return BoundLedgerId; }
	const Fdemo_mapShanmenRunCorrelation& GetBoundCorrelation() const
	{
		return BoundCorrelation;
	}
	int32 GetRouteRecordCount() const
	{
		return ExecutionService.GetRouteRecordCount();
	}
	int32 GetActiveLeaseCount() const
	{
		return ExecutionService.GetActiveLeaseCount();
	}
	int32 GetCompletedIntentCount() const
	{
		return ExecutionService.GetCompletedIntentCount();
	}
	int32 GetExecutorAttemptCount() const
	{
		return ExecutionService.GetExecutorAttemptCount();
	}
	const Fdemo_mapShanmenFormationInfluenceExecutionService&
	GetExecutionService() const
	{
		return ExecutionService;
	}

private:
	bool ValidateHost(
		const Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		Fdemo_mapShanmenFormationInfluenceLifecycleResult& OutResult) const;
	bool BindTo(
		const Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation);

	Fdemo_mapShanmenRunCorrelation BoundCorrelation;
	FGuid BoundLedgerId;
	Fdemo_mapShanmenFormationInfluenceExecutionService ExecutionService;
};
