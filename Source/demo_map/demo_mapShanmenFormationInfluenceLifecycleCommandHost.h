#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerProductRuntime.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h"

class UWorld;

/**
 * Pointer-free evidence that a caller resolved one subject to one concrete
 * AttributeComponent for an explicit consumer activation.
 *
 * The component pointer remains a transient call argument. This value stores
 * only its process-local UObject unique id and never owns or discovers it.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution
{
	FGuid ResolutionId;
	FGuid SubjectEntityId;
	uint32 AttributeComponentUniqueId = 0;

	static bool TryCreate(
		const FGuid& SubjectEntityId,
		const Udemo_mapAttributeComponent* AttributeComponent,
		Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution&
			OutResolution);

	bool IsValid() const;
	bool Matches(
		const FGuid& ExpectedSubjectEntityId,
		const Udemo_mapAttributeComponent* AttributeComponent) const;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus : uint8
{
	Prepared,
	HostInvalid,
	LifecycleCommandIdInvalid,
	LifecycleReceiptNotFound,
	LifecycleReceiptRejected,
	LifecycleOperationMismatch,
	DefinitionInvalid,
	LeaseNotActive,
	ProjectionRejected,
	CommandBuildRejected,
	StateInvalid
};

/**
 * Frozen caller payload derived from one successful Apply lifecycle receipt.
 *
 * The delivery is read-only evidence. It does not bind a component or mutate
 * either the authoritative lease executor or the native consumer registry.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery
{
	FGuid LifecycleCommandId;
	FGuid SubjectEntityId;
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot AuthoritativeLease;
	Fdemo_mapShanmenFormationInfluenceConsumerDefinition Definition;
	Fdemo_mapShanmenFormationInfluenceConsumerProjection Projection;
	Fdemo_mapShanmenFormationInfluenceConsumerCommand Apply;
	Fdemo_mapShanmenFormationInfluenceConsumerCommand Remove;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery& Other)
		const;
};

/** Source receipt, projection evidence, and one immutable command delivery. */
struct Fdemo_mapShanmenFormationInfluenceConsumerCommandDeliveryResult
{
	Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::HostInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord SourceReceipt;
	Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult
		ProjectionAttempt;
	Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery Delivery;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus
	: uint8
{
	Activated,
	ActivationReplayed,
	Deactivated,
	DeactivationReplayed,
	HostInvalid,
	DeliveryInvalid,
	SourceReceiptNotFound,
	SourceReceiptRejected,
	SubjectResolutionInvalid,
	SubjectMismatch,
	AttributeComponentUnavailable,
	AttributeComponentMismatch,
	RuntimeRejected,
	StateInvalid
};

/**
 * One explicit application of a P8.35 delivery at the existing product
 * composition root. Source receipt, pointer-free subject resolution, and the
 * nested native runtime receipt remain visible for audit and replay.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationResult
{
	Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
			HostInvalid;
	FString Diagnostic;
	bool bSourceReceiptChecked = false;
	bool bSubjectResolutionChecked = false;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord SourceReceipt;
	Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery Delivery;
	Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution
		SubjectResolution;
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult Runtime;

	bool IsSuccess() const;
};

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
	Fdemo_mapShanmenFormationInfluenceConsumerCommandDeliveryResult
	TryPrepareConsumerCommands(
		const FGuid& AppliedLifecycleCommandId,
		const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Definition)
		const;
	Fdemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationResult
	TryActivateConsumerDelivery(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery&
			Delivery,
		const Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution&
			SubjectResolution,
		Udemo_mapAttributeComponent* AttributeComponent);
	Fdemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationResult
	TryDeactivateConsumerDelivery(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery&
			Delivery);

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
