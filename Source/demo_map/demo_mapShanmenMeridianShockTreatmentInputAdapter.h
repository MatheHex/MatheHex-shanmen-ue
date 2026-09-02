#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenMeridianShockTreatmentProductLifecycle.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

/** Outcome of classifying and routing one physical hotbar press. */
enum class Edemo_mapShanmenMeridianShockTreatmentInputStatus : uint8
{
	PassThrough,
	Applied,
	RecoveryRequired,
	InvalidSlot,
	SnapshotUnavailable,
	SnapshotStale,
	ItemEvidenceRejected,
	ProductRunMismatch,
	TimelineUnavailable,
	RequestSequenceExhausted,
	RequestCaptureRejected,
	ProductRejected
};

/** Immutable audit evidence for one treatment input decision. */
struct Fdemo_mapShanmenMeridianShockTreatmentInputResult
{
	Edemo_mapShanmenMeridianShockTreatmentInputStatus Status =
		Edemo_mapShanmenMeridianShockTreatmentInputStatus::PassThrough;
	int32 HotbarSlotNumber = INDEX_NONE;
	int32 AuthorityRevision = INDEX_NONE;
	uint64 RequestOrdinal = 0;
	FGuid RunId;
	FGuid ItemInstanceId;
	FGuid RequestId;
	bool bTimelineSampled = false;
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Route;
	FString Diagnostic;

	bool ShouldPassThrough() const
	{
		return Status
			== Edemo_mapShanmenMeridianShockTreatmentInputStatus::PassThrough;
	}
	bool IsHandled() const { return !ShouldPassThrough(); }
	bool IsAccepted() const
	{
		return Status
			== Edemo_mapShanmenMeridianShockTreatmentInputStatus::Applied
			&& Route.IsCommitted();
	}
};

/**
 * Sole device-to-product bridge for the canonical Meridian Shock treatment
 * item. Non-treatment items pass through before timeline sampling. Once the
 * exact product semantic is recognized, every outcome is handled so the item
 * can never fall through to generic health-consumable behavior.
 */
class Fdemo_mapShanmenMeridianShockTreatmentInputAdapter
{
public:
	static FGuid MakeRequestId(
		const FGuid& CorrelationId,
		const FGuid& RunId,
		const FGuid& ItemInstanceId,
		int32 HotbarSlotNumber,
		int32 AuthorityRevision,
		const FGuid& TimelineSampleId,
		uint64 RequestOrdinal);

	Fdemo_mapShanmenMeridianShockTreatmentInputResult RouteHotbarInput(
		Udemo_mapShanmenItemAuthoritySubsystem* Authority,
		Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle& Lifecycle,
		const Fdemo_mapShanmenCombatRunFixedTimeline& Timeline,
		int32 HotbarSlotNumber);

	void Reset() { NextRequestOrdinal = 1; }
	uint64 GetNextRequestOrdinal() const { return NextRequestOrdinal; }

private:
	uint64 NextRequestOrdinal = 1;
};
