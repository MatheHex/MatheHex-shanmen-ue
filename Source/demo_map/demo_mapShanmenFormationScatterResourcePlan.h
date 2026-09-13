#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"
#include "demo_mapShanmenFormationScatterBatchIntent.h"
#include "demo_mapShanmenRunCorrelation.h"

/** One exact physical-stack slice assigned to one authored material intent. */
class Fdemo_mapShanmenFormationScatterResourceSlice
{
public:
	bool IsValid() const;
	const FGuid& GetSliceId() const { return SliceId; }
	const FGuid& GetPlanningScopeId() const { return PlanningScopeId; }
	int32 GetSliceOrder() const { return SliceOrder; }
	const FGuid& GetAnchorIntentId() const { return AnchorIntentId; }
	const FGuid& GetMaterialIntentId() const { return MaterialIntentId; }
	int32 GetAnchorOrder() const { return AnchorOrder; }
	int32 GetRequirementOrder() const { return RequirementOrder; }
	FName GetMaterialDefinitionId() const { return MaterialDefinitionId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	int32 GetQuantity() const { return Quantity; }
	int32 GetSourceQuantityBefore() const { return SourceQuantityBefore; }
	int32 GetSourceQuantityAfter() const { return SourceQuantityAfter; }

	bool operator==(
		const Fdemo_mapShanmenFormationScatterResourceSlice& Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterResourcePlanner;

	static FGuid BuildSliceId(
		const Fdemo_mapShanmenFormationScatterResourceSlice& Slice);

	FGuid SliceId;
	FGuid PlanningScopeId;
	int32 SliceOrder = INDEX_NONE;
	FGuid AnchorIntentId;
	FGuid MaterialIntentId;
	int32 AnchorOrder = INDEX_NONE;
	int32 RequirementOrder = INDEX_NONE;
	FName MaterialDefinitionId = NAME_None;
	FGuid ItemInstanceId;
	int32 Quantity = 0;
	int32 SourceQuantityBefore = 0;
	int32 SourceQuantityAfter = 0;
};

/**
 * One authority-compatible prepare command per physical stack. Ordered slices
 * retain every destination even when one stack supplies multiple anchors.
 */
class Fdemo_mapShanmenFormationScatterResourceReservationIntent
{
public:
	bool IsValid() const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetPlanningScopeId() const { return PlanningScopeId; }
	int32 GetItemOrder() const { return ItemOrder; }
	FName GetMaterialDefinitionId() const { return MaterialDefinitionId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	int32 GetExpectedQuantityBefore() const
	{
		return ExpectedQuantityBefore;
	}
	int32 GetQuantity() const { return Quantity; }
	int32 GetQuantityAfter() const { return QuantityAfter; }
	const TArray<FGuid>& GetSliceIds() const { return SliceIds; }
	const FShanmenItemRunQuantityIntentRequest& GetPrepareRequest() const
	{
		return PrepareRequest;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterResourcePlanner;

	static FGuid BuildIntentId(
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Intent);
	static FGuid BuildRequestId(
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Intent);

	FGuid IntentId;
	FGuid PlanningScopeId;
	int32 ItemOrder = INDEX_NONE;
	FName MaterialDefinitionId = NAME_None;
	FGuid ItemInstanceId;
	int32 ExpectedQuantityBefore = 0;
	int32 Quantity = 0;
	int32 QuantityAfter = 0;
	TArray<FGuid> SliceIds;
	FShanmenItemRunQuantityIntentRequest PrepareRequest;
};

/**
 * Immutable whole-batch allocation against one exact ShanmenItems snapshot.
 * It is a preflight artifact only and never mutates the item authority.
 */
class Fdemo_mapShanmenFormationScatterResourcePlan
{
public:
	bool IsValid() const;
	const FGuid& GetPlanId() const { return PlanId; }
	const FGuid& GetPlanningScopeId() const { return PlanningScopeId; }
	const Fdemo_mapShanmenFormationScatterBatchIntent& GetBatch() const
	{
		return Batch;
	}
	const FGuid& GetCorrelationId() const { return CorrelationId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	const FGuid& GetScopeId() const { return ScopeId; }
	const FGuid& GetActiveRunId() const { return ActiveRunId; }
	const FGuid& GetLifecycleReceiptId() const
	{
		return LifecycleReceiptId;
	}
	int32 GetAuthorityRevision() const { return AuthorityRevision; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const TArray<Fdemo_mapShanmenFormationScatterResourceSlice>&
	GetSlices() const
	{
		return Slices;
	}
	const TArray<
		Fdemo_mapShanmenFormationScatterResourceReservationIntent>&
	GetReservations() const
	{
		return Reservations;
	}
	int32 GetTotalRequirementCount() const
	{
		return TotalRequirementCount;
	}
	int64 GetTotalAllocatedQuantity() const
	{
		return TotalAllocatedQuantity;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterResourcePlanner;

	static FGuid BuildPlanningScopeId(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);
	static FGuid BuildPlanId(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);

	FGuid PlanId;
	FGuid PlanningScopeId;
	Fdemo_mapShanmenFormationScatterBatchIntent Batch;
	FGuid CorrelationId;
	FGuid OwnerId;
	FGuid ScopeId;
	FGuid ActiveRunId;
	FGuid LifecycleReceiptId;
	int32 AuthorityRevision = INDEX_NONE;
	FShanmenContentStamp Content;
	TArray<Fdemo_mapShanmenFormationScatterResourceSlice> Slices;
	TArray<Fdemo_mapShanmenFormationScatterResourceReservationIntent>
		Reservations;
	int32 TotalRequirementCount = 0;
	int64 TotalAllocatedQuantity = 0;
};

enum class Edemo_mapShanmenFormationScatterResourcePlanStatus : uint8
{
	Invalid,
	Planned,
	BatchInvalid,
	MasteryProjectionRejected,
	DeploymentInvalid,
	BatchStale,
	RunCorrelationInvalid,
	RunMismatch,
	SnapshotInvalid,
	ContentMismatch,
	SnapshotStale,
	LifecycleInvalid,
	ItemAuthorityInvalid,
	ConflictingIntent,
	QuantityUnavailable,
	PlanInvalid
};

/** Auditable result for one bounded, side-effect-free resource preflight. */
struct Fdemo_mapShanmenFormationScatterResourcePlanResult
{
	Edemo_mapShanmenFormationScatterResourcePlanStatus Status =
		Edemo_mapShanmenFormationScatterResourcePlanStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterResourcePlan Plan;

	bool IsValid() const;
	bool IsPlanned() const;
};

/**
 * Pure cumulative allocator from one current P27.17 batch and one exact
 * ShanmenItems snapshot to authority-compatible prepare commands. It performs
 * no reserve, commit, cancellation, deployment or World operation.
 */
struct Fdemo_mapShanmenFormationScatterResourcePlanner
{
	static Fdemo_mapShanmenFormationScatterResourcePlanResult Plan(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterBatchIntent& Batch);

	static bool IsCurrentPlan(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);
};
