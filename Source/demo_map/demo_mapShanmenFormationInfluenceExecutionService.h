#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceExecutionRouter.h"
#include "demo_mapShanmenFormationInfluenceProductRuntime.h"

enum class Edemo_mapShanmenFormationInfluenceServiceStatus : uint8
{
	Accepted,
	RouteRejected,
	ExecutionRejected,
	StateInvalid
};

/** One explicit route-and-execute result with both authority receipts visible. */
struct Fdemo_mapShanmenFormationInfluenceServiceResult
{
	Edemo_mapShanmenFormationInfluenceServiceStatus Status =
		Edemo_mapShanmenFormationInfluenceServiceStatus::StateInvalid;
	FString Diagnostic;
	bool bServiceStateCommitted = false;
	Fdemo_mapShanmenFormationInfluenceRouteResult Route;
	Fdemo_mapShanmenFormationInfluenceExecutionResult Execution;

	bool IsSuccess() const;
};

/**
 * Single-step product composition for request routing and lease execution.
 *
 * One call routes at most one caller-owned request and delegates at most one
 * command to the P8.17 runtime. The Service owns only Router and Runtime value
 * state; ProductHost remains caller-owned and authoritative for pending order,
 * attempt receipts, acknowledgement, seal, and teardown. It never loops,
 * schedules, retries, discovers product objects, or advances Host lifecycle.
 */
class Fdemo_mapShanmenFormationInfluenceExecutionService
{
public:
	Fdemo_mapShanmenFormationInfluenceServiceResult TryExecuteOne(
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request);

	bool IsValid() const;
	bool IsBound() const
	{
		return Router.IsBound() && Runtime.IsBound();
	}
	const FGuid& GetBoundLedgerId() const
	{
		return Router.GetBoundLedgerId();
	}
	const Fdemo_mapShanmenRunCorrelation& GetBoundCorrelation() const
	{
		return Router.GetBoundCorrelation();
	}
	int32 GetRouteRecordCount() const { return Router.GetRecordCount(); }
	int32 GetActiveLeaseCount() const
	{
		return Runtime.GetActiveLeaseCount();
	}
	int32 GetCompletedIntentCount() const
	{
		return Runtime.GetCompletedIntentCount();
	}
	int32 GetExecutorAttemptCount() const
	{
		return Runtime.GetExecutorAttemptCount();
	}
	const Fdemo_mapShanmenFormationInfluenceExecutionRouter&
	GetRouter() const
	{
		return Router;
	}
	const Fdemo_mapShanmenFormationInfluenceProductRuntime&
	GetRuntime() const
	{
		return Runtime;
	}

private:
	Fdemo_mapShanmenFormationInfluenceExecutionRouter Router;
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
};
