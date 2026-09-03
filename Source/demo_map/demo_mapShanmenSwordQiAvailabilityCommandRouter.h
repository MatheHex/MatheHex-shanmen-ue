#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordQiCommandEventOwner.h"

/** Device-independent decisions admitted by the Sword Qi availability seam. */
enum class Edemo_mapShanmenSwordQiAvailabilityCommandKind : uint8
{
	Issue,
	Retry,
	Cancel
};

/** Immutable optimistic command captured from one availability projection. */
class Fdemo_mapShanmenSwordQiAvailabilityCommand
{
public:
	static bool TryCapture(
		const FGuid& ExpectedProjectionId,
		Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind,
		Fdemo_mapShanmenSwordQiAvailabilityCommand& OutCommand);

	bool IsValid() const;
	const FGuid& GetExpectedProjectionId() const
	{
		return ExpectedProjectionId;
	}
	Edemo_mapShanmenSwordQiAvailabilityCommandKind GetKind() const
	{
		return Kind;
	}

private:
	FGuid ExpectedProjectionId;
	Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind =
		Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue;
};

enum class Edemo_mapShanmenSwordQiAvailabilityCommandStatus : uint8
{
	Dispatched,
	CommandInvalid,
	ProjectionUnavailable,
	ProjectionStale,
	CommandUnavailable,
	OwnerRejected,
	ProjectionPostconditionFailed
};

/**
 * Complete proof for one optimistic availability decision.
 *
 * Dispatched means the current projection admitted the decision and the sole
 * command-event owner was invoked. Product acceptance remains visible only in
 * CommandEvent; the outer status never converts a downstream rejection into
 * success.
 */
struct Fdemo_mapShanmenSwordQiAvailabilityCommandResult
{
	Edemo_mapShanmenSwordQiAvailabilityCommandStatus Status =
		Edemo_mapShanmenSwordQiAvailabilityCommandStatus::CommandInvalid;
	Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind =
		Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue;
	FGuid ExpectedProjectionId;
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Before;
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection After;
	Fdemo_mapShanmenSwordQiCommandEventResult CommandEvent;
	Fdemo_mapShanmenSwordQiPendingRetryCancellation Cancellation;
	FString Diagnostic;

	bool IsDispatched() const
	{
		return Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::Dispatched;
	}
};

/**
 * Stateless optimistic router over the P18.9 availability projection.
 *
 * The expected projection is re-read immediately before dispatch. Stale or
 * state-incompatible commands fail before either callback is invoked. Issue
 * delegates to the existing fresh-input owner route; Retry receives only the
 * owner-frozen sample; Cancel reaches only the existing cancellation route.
 * This seam owns no key, UI, polling, retry loop, sequence, item, attribute,
 * Actor, inventory, projectile or damage authority.
 */
struct Fdemo_mapShanmenSwordQiAvailabilityCommandRouter
{
	static Fdemo_mapShanmenSwordQiAvailabilityCommandResult TryRoute(
		Fdemo_mapShanmenSwordQiCommandEventOwner& Owner,
		const Fdemo_mapShanmenSwordQiAvailabilityCommand& Command,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(const FGuid&)>
			RouteIssueInput,
		TFunctionRef<Fdemo_mapShanmenSwordQiInputResult(
			const FGuid&,
			const Fdemo_mapShanmenSwordQiInputSample&)> RouteFrozenInput);
};
