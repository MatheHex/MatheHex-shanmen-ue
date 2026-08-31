#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h"

/**
 * Caller-owned immutable projection and deterministic plan prepared together.
 *
 * The value retains no ProductSession, Host, executor or retry ownership. A
 * caller may therefore delay or replay dispatch without consulting live
 * Product state again.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			Other) const;
	bool MatchesCurrentSession(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session) const;

	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
	GetProjection() const
	{
		return Projection;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan&
	GetPlan() const
	{
		return Plan;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService;

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection Projection;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan Plan;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus
	: uint8
{
	Prepared,
	ProjectionRejected,
	PlanRejected,
	StateInvalid
};

/** Complete evidence for one current-state preparation pass. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareStatus::
				ProjectionRejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
		Projection;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureResult
		PlanCapture;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
		Prepared;

	bool IsPrepared() const;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus
	: uint8
{
	Dispatched,
	Resumed,
	Replayed,
	PreparedRejected,
	DispatchIncomplete,
	StateInvalid
};

/** Complete evidence for one bounded attempt to consume a prepared value. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchStatus::
				PreparedRejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
		Prepared;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult Dispatch;

	bool IsSuccess() const;
};

/** Stateless prepare-now / dispatch-later Product boundary. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService
{
public:
	/** Read the current ProductSession once and freeze projection plus plan. */
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchPrepareResult
	PrepareCurrent(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
			Seed);

	/** Consume an already-frozen value without reading ProductSession. */
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchResult
	TryDispatchPrepared(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			Prepared,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
};
