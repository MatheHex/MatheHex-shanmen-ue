#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h"

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus
	: uint8
{
	Dispatched,
	Resumed,
	Replayed,
	ProjectionRejected,
	PlanRejected,
	DispatchIncomplete,
	StateInvalid
};

/** Complete evidence for one single-read planned current dispatch. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchStatus::
				ProjectionRejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
		Projection;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureResult
		PlanCapture;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult Dispatch;

	bool IsSuccess() const;
};

/**
 * Stateless current ProductSession -> deterministic plan -> frozen dispatch.
 *
 * One call reads the current projection once, captures one P12.23 plan, then
 * feeds that exact projection and identity to P12.22 once. Every seed, Host
 * and executor remains caller-owned; no retry or future Session state is read.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchResult
	TryDispatchCurrent(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
			Seed,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
};
