#pragma once

#include "CoreMinimal.h"

/** Immutable observation of one Run-bound fixed timeline tick. */
struct Fdemo_mapShanmenCombatRunTimelineSample
{
public:
	static bool TryCapture(
		const FGuid& TimelineId,
		int64 CurrentTick,
		Fdemo_mapShanmenCombatRunTimelineSample& OutSample);

	bool IsValid() const;
	const FGuid& GetSampleId() const { return SampleId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetCurrentTick() const { return CurrentTick; }

private:
	FGuid SampleId;
	FGuid TimelineId;
	int64 CurrentTick = INDEX_NONE;
};

/**
 * Run-bound, fixed-rate monotonic product clock.
 *
 * It consumes the GameMode's existing DeltaSeconds stream, but exposes only
 * whole 30 Hz ticks. It owns no Actor, input, timer, frame number or wall
 * clock, and its identity is deterministically derived from the active Run.
 */
class Fdemo_mapShanmenCombatRunFixedTimeline
{
public:
	static int64 CanonicalTicksPerSecond() { return 30; }
	static FGuid MakeTimelineId(const FGuid& RunId);

	bool TryBegin(const FGuid& RunId, FString& OutDiagnostic);
	bool TryAdvance(
		double DeltaSeconds,
		int64& OutAdvancedTicks,
		FString& OutDiagnostic);
	bool TryCapture(
		Fdemo_mapShanmenCombatRunTimelineSample& OutSample) const;
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsActiveForRun(const FGuid& RunId) const;
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetCurrentTick() const { return CurrentTick; }
	double GetSubTickSeconds() const { return SubTickSeconds; }

private:
	FGuid RunId;
	FGuid TimelineId;
	int64 CurrentTick = 0;
	double SubTickSeconds = 0.0;
};
