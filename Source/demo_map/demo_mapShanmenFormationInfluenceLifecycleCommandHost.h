#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerProductRuntime.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h"

class UWorld;

/**
 * Caller-facing owner of one lifecycle Router, one consumer runtime, and their
 * durable lifecycle receipts.
 *
 * Opening freezes ProductHost identity values and the consumer bridge value
 * state. New consumer Apply/Remove commands must match this Host's active
 * authoritative lease: native Apply follows lease Apply, and native Remove
 * precedes lease Remove. Exact completed consumer commands remain read-only
 * replays after lease removal. The actual ProductHost, World, and attribute
 * components remain caller-owned and must be supplied for every explicit
 * operation. This Host never discovers work, drains queues, retries,
 * schedules, persists, or owns engine-object pointers.
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
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
	TryActivateConsumer(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const FGuid& SubjectEntityId,
		Udemo_mapAttributeComponent* AttributeComponent,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& ApplyCommand);
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
	TryDeactivateConsumer(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& RemoveCommand);

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
	const Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime&
	GetConsumerRuntime() const
	{
		return ConsumerRuntime;
	}
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
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
	RejectConsumer(
		Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus Status,
		const TCHAR* Diagnostic) const;
	bool TryGetAuthoritativeActiveLease(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command,
		Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& OutLease) const;
	bool IsCompletedConsumerCommand(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command) const;

	Fdemo_mapShanmenRunCorrelation Correlation;
	FGuid LedgerId;
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime ConsumerRuntime;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Router;
};
