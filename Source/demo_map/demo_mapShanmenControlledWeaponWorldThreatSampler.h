#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"
#include "demo_mapShanmenControlledWeaponThreatSampleRouter.h"

class UWorld;
struct Fdemo_mapShanmenControlledWeaponWorldLifecycle;

enum class Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus : uint8
{
	Sampled,
	NotDue,
	NotOrbiting,
	OwnerInvalid,
	OwnerInactive,
	DependenciesUnavailable,
	RunMismatch,
	TimelineMismatch,
	RouterMismatch,
	SequenceExhausted,
	CollisionUnavailable,
	IntentCaptureRejected,
	RouteRejected,
	ResultInvalid
};

/** Audit for one bounded product cadence pulse. */
struct Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult
{
	Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus Status =
		Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::OwnerInactive;
	FGuid RunId;
	FGuid ItemInstanceId;
	FGuid IntentId;
	int64 ObservedTick = INDEX_NONE;
	int64 ScheduledTick = INDEX_NONE;
	int64 SampleSequence = INDEX_NONE;
	int32 RawOverlapCount = 0;
	int32 RoutedContactCount = 0;
	Fdemo_mapShanmenControlledWeaponThreatSampleResult Route;
	FString Diagnostic;

	bool IsSampled() const;
	bool IsNoOp() const;
	bool IsSuccess() const { return IsSampled() || IsNoOp(); }
};

/**
 * Run-scoped cadence owner for the canonical training flying sword's orbit
 * threat sample.
 *
 * It reads the existing 30 Hz combat timeline, performs at most one World box
 * overlap per owner pulse, resolves only registered entities, and submits one
 * exact-item request to the P6.22 Router. It owns no Actor, timer, damage,
 * effect, inventory state, or alternate combat authority.
 */
class Fdemo_mapShanmenControlledWeaponWorldThreatSampler
{
public:
	/** Prototype cadence: one query every three canonical timeline ticks. */
	static int64 CanonicalSampleIntervalTicks() { return 3; }
	static FGuid MakeIntentId(
		const FGuid& RunId,
		const FGuid& TimelineId,
		const FGuid& ItemInstanceId,
		int64 SampleSequence,
		int64 ScheduledTick);

	bool TryBegin(
		const FGuid& RunId,
		const FGuid& TimelineId,
		const Fdemo_mapShanmenControlledWeaponWorldLifecycle& Lifecycle,
		FString& OutDiagnostic);

	Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult TrySample(
		UWorld* World,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		const Fdemo_mapShanmenControlledWeaponWorldLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		Fdemo_mapShanmenControlledWeaponThreatSampleRouter& Router);

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsActiveForRun(const FGuid& ExpectedRunId) const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetNextScheduledTick() const { return NextScheduledTick; }
	int64 NumCommittedSamples() const { return CommittedSampleCount; }
	void Reset();

private:
	FGuid RunId;
	FGuid ItemInstanceId;
	FGuid TimelineId;
	int64 NextScheduledTick = 0;
	int64 CommittedSampleCount = 0;
};
