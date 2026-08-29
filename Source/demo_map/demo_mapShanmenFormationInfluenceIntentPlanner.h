#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationCoverageTransitionReducer.h"

enum class Edemo_mapShanmenFormationInfluenceOperation : uint8
{
	Apply,
	Remove
};

/** Authored identity only; magnitude, duration, stacking, and executor stay downstream. */
struct Fdemo_mapShanmenFormationInfluencePolicy
{
	FName PolicyDefinitionId = NAME_None;
	FName InfluenceDefinitionId = NAME_None;
	FShanmenContentStamp Content;

	bool IsValid() const;
};

/**
 * Generic immutable operation envelope for a later effect authority.
 *
 * CauseId identifies the evidence that requested the operation. P8.11 uses a
 * coverage-transition FactId; later baseline/lifecycle reconcilers may reuse
 * the envelope while publishing their own self-validating batch contracts.
 */
struct Fdemo_mapShanmenFormationInfluenceIntent
{
	FGuid IntentId;
	FGuid RunId;
	FGuid OwnerId;
	FGuid SourceEntityId;
	FGuid DeploymentId;
	FGuid AreaId;
	FGuid SubjectEntityId;
	FName PolicyDefinitionId = NAME_None;
	FName InfluenceDefinitionId = NAME_None;
	Edemo_mapShanmenFormationInfluenceOperation Operation =
		Edemo_mapShanmenFormationInfluenceOperation::Apply;
	FGuid CauseId;
	FShanmenContentStamp Content;

	bool IsValid() const;
};

/** Self-contained delta plan sealed to one Area, policy, and transition receipt. */
struct Fdemo_mapShanmenFormationInfluenceTransitionBatch
{
	FGuid BatchId;
	Fdemo_mapShanmenFormationAreaSnapshot Area;
	FGuid SourceEntityId;
	Fdemo_mapShanmenFormationInfluencePolicy Policy;
	Fdemo_mapShanmenFormationCoverageTransitionReceipt Transition;
	TArray<Fdemo_mapShanmenFormationInfluenceIntent> Intents;
	int32 ApplyCount = 0;
	int32 RemoveCount = 0;

	bool IsValid() const;
	bool IsNoOp() const
	{
		return IsValid() && Intents.IsEmpty();
	}
};

enum class Edemo_mapShanmenFormationInfluencePlanStatus : uint8
{
	Planned,
	AreaInvalid,
	SourceInvalid,
	PolicyInvalid,
	TransitionInvalid,
	AreaMismatch,
	ContentMismatch,
	ReceiptRejected
};

struct Fdemo_mapShanmenFormationInfluencePlanResult
{
	Edemo_mapShanmenFormationInfluencePlanStatus Status =
		Edemo_mapShanmenFormationInfluencePlanStatus::AreaInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceTransitionBatch Batch;

	bool IsSuccess() const;
};

/**
 * Pure transition-to-intent planner.
 *
 * Entered emits Apply, Left emits Remove, and the two unchanged transition
 * kinds emit nothing. The planner owns no World, Actor, cadence, state,
 * gameplay effect, damage, item, UI, or persistence behavior.
 */
class Fdemo_mapShanmenFormationInfluenceIntentPlanner
{
public:
	static Fdemo_mapShanmenFormationInfluencePlanResult PlanTransition(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const FGuid& SourceEntityId,
		const Fdemo_mapShanmenFormationInfluencePolicy& Policy,
		const Fdemo_mapShanmenFormationCoverageTransitionReceipt& Transition);
};
