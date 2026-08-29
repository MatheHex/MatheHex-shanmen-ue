#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceLeaseExecutor.h"

/**
 * Run-scoped product composition for the concrete influence lease executor.
 *
 * The caller continues to own ProductHost and every command identity. This
 * runtime owns only executor lifetime plus an immutable Correlation/Ledger
 * binding. Each call delegates exactly one command through the P8.15 adapter;
 * it never drains, schedules, retries, seals, or tears down the Host itself.
 */
class Fdemo_mapShanmenFormationInfluenceProductRuntime
{
public:
	Fdemo_mapShanmenFormationInfluenceExecutionResult TryExecuteOne(
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationInfluenceExecutionCommand& Command);

	bool IsValid() const;
	bool IsBound() const { return BoundLedgerId.IsValid(); }
	const FGuid& GetBoundLedgerId() const { return BoundLedgerId; }
	const Fdemo_mapShanmenRunCorrelation& GetBoundCorrelation() const
	{
		return BoundCorrelation;
	}
	int32 GetActiveLeaseCount() const
	{
		return Executor.GetActiveLeaseCount();
	}
	int32 GetCompletedIntentCount() const
	{
		return Executor.GetCompletedIntentCount();
	}
	int32 GetExecutorAttemptCount() const
	{
		return Executor.GetAttemptCount();
	}
	const Fdemo_mapShanmenFormationInfluenceLeaseExecutor&
	GetExecutor() const
	{
		return Executor;
	}

private:
	Fdemo_mapShanmenRunCorrelation BoundCorrelation;
	FGuid BoundLedgerId;
	Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
};
