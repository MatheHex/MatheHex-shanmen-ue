#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h"

class UWorld;

/**
 * Caller-facing owner of one lifecycle Router binding and its durable receipts.
 *
 * Opening freezes only ProductHost identity values. The actual ProductHost and
 * World remain caller-owned and must be supplied for every explicit command.
 * This Host never discovers work, drains queues, retries, schedules, persists,
 * or owns engine-object pointers.
 */
class Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost
{
public:
	static bool TryOpen(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost& OutHost);

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult TrySubmit(
		UWorld* World,
		Fdemo_mapShanmenFormationProductHost& ProductHost,
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command);

	bool TryGetReceipt(
		const FGuid& CommandId,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord&
			OutReceipt) const;
	bool IsValid() const;
	bool IsEmpty() const { return Router.IsEmpty(); }
	int32 GetReceiptCount() const { return Router.GetRecordCount(); }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	const FGuid& GetLedgerId() const { return LedgerId; }
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter&
	GetRouter() const
	{
		return Router;
	}

private:
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Reject(
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command,
		Edemo_mapShanmenFormationInfluenceLifecycleStatus LifecycleStatus,
		const TCHAR* Diagnostic) const;

	Fdemo_mapShanmenRunCorrelation Correlation;
	FGuid LedgerId;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Router;
};
