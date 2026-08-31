#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.h"

/** Caller-owned root identity and logical consumer scopes for one plan. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed
{
	FGuid DispatchSeed;
	FGuid VisualConsumerScopeId;
	FGuid AudioConsumerScopeId;

	bool IsValid() const;
};

/**
 * Immutable deterministic identity plan for one frozen Product projection.
 *
 * All eight P12.21 transport/execution identities are role-isolated hashes of
 * the explicit seed, consumer scopes and projection identity. The plan owns no
 * Host, executor, allocator, registry, sequence or retry state.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan&
			Other) const;
	bool MatchesProjection(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
			Other) const;
	bool MatchesRequest(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest&
			Other) const;
	bool MatchesCurrentSession(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session) const;

	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
	GetSeed() const
	{
		return Seed;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
	GetProjection() const
	{
		return Projection;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture&
	GetTransactionIdentity() const
	{
		return TransactionIdentity;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory;

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed Seed;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection Projection;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture
		TransactionIdentity;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus
	: uint8
{
	Captured,
	SeedInvalid,
	ProjectionInvalid,
	IdentityDerivationRejected,
	PlanRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
				SeedInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan Plan;

	bool IsCaptured() const
	{
		return Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
					Captured
			&& Plan.IsValid();
	}
};

/** Stateless deterministic projection-to-transaction-identity factory. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureResult
	Capture(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
			Projection,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
			Seed);
};
