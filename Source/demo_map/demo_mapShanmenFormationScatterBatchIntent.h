#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationMasteryOperationAuthorization.h"

/** One exact authored material requirement routed to one deployed anchor. */
class Fdemo_mapShanmenFormationScatterMaterialIntent
{
public:
	bool IsValid() const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetAuthorizationId() const { return AuthorizationId; }
	const FGuid& GetOperationId() const { return OperationId; }
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	int32 GetAnchorOrder() const { return AnchorOrder; }
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetAnchorInstanceId() const { return AnchorInstanceId; }
	int32 GetRequirementOrder() const { return RequirementOrder; }
	FName GetMaterialDefinitionId() const { return MaterialDefinitionId; }
	int32 GetQuantity() const { return Quantity; }

	bool operator==(
		const Fdemo_mapShanmenFormationScatterMaterialIntent& Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterBatchPlanner;

	static FGuid BuildIntentId(
		const Fdemo_mapShanmenFormationScatterMaterialIntent& Intent);

	FGuid IntentId;
	FGuid AuthorizationId;
	FGuid OperationId;
	FGuid DeploymentId;
	int32 AnchorOrder = INDEX_NONE;
	FName AnchorDefinitionId = NAME_None;
	FGuid AnchorInstanceId;
	int32 RequirementOrder = INDEX_NONE;
	FName MaterialDefinitionId = NAME_None;
	int32 Quantity = 0;
};

/** Canonical scatter sub-command for every requirement at one exact anchor. */
class Fdemo_mapShanmenFormationScatterAnchorIntent
{
public:
	bool IsValid() const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetAuthorizationId() const { return AuthorizationId; }
	const FGuid& GetOperationId() const { return OperationId; }
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	int32 GetAnchorOrder() const { return AnchorOrder; }
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetAnchorInstanceId() const { return AnchorInstanceId; }
	const FVector& GetWorldLocation() const { return WorldLocation; }
	const TArray<Fdemo_mapShanmenFormationScatterMaterialIntent>&
	GetMaterialIntents() const
	{
		return MaterialIntents;
	}
	int64 GetTotalMaterialQuantity() const { return TotalMaterialQuantity; }

	bool operator==(
		const Fdemo_mapShanmenFormationScatterAnchorIntent& Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterBatchPlanner;

	static FGuid BuildIntentId(
		const Fdemo_mapShanmenFormationScatterAnchorIntent& Intent);

	FGuid IntentId;
	FGuid AuthorizationId;
	FGuid OperationId;
	FGuid DeploymentId;
	int32 AnchorOrder = INDEX_NONE;
	FName AnchorDefinitionId = NAME_None;
	FGuid AnchorInstanceId;
	FVector WorldLocation = FVector::ZeroVector;
	TArray<Fdemo_mapShanmenFormationScatterMaterialIntent> MaterialIntents;
	int64 TotalMaterialQuantity = 0;
};

/**
 * Self-validating, immutable expansion of one current Master scatter
 * authorization into every ordered anchor and authored material requirement.
 */
class Fdemo_mapShanmenFormationScatterBatchIntent
{
public:
	bool IsValid() const;
	const FGuid& GetBatchIntentId() const { return BatchIntentId; }
	const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
	GetAuthorization() const
	{
		return Authorization;
	}
	const TArray<Fdemo_mapShanmenFormationScatterAnchorIntent>&
	GetAnchorIntents() const
	{
		return AnchorIntents;
	}
	int32 GetTotalRequirementCount() const
	{
		return TotalRequirementCount;
	}
	int64 GetTotalMaterialQuantity() const { return TotalMaterialQuantity; }

	bool operator==(
		const Fdemo_mapShanmenFormationScatterBatchIntent& Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterBatchPlanner;

	static FGuid BuildBatchIntentId(
		const Fdemo_mapShanmenFormationScatterBatchIntent& Intent);

	FGuid BatchIntentId;
	Fdemo_mapShanmenFormationMasteryOperationAuthorization Authorization;
	TArray<Fdemo_mapShanmenFormationScatterAnchorIntent> AnchorIntents;
	int32 TotalRequirementCount = 0;
	int64 TotalMaterialQuantity = 0;
};

enum class Edemo_mapShanmenFormationScatterBatchPlanStatus : uint8
{
	Invalid,
	Planned,
	AuthorizationInvalid,
	AuthorizationNotScatter,
	MasteryProjectionRejected,
	DeploymentInvalid,
	AuthorizationStale,
	DeploymentShapeInvalid,
	RequirementInvalid,
	IntentInvalid
};

/** Auditable result for one bounded, side-effect-free scatter expansion. */
struct Fdemo_mapShanmenFormationScatterBatchPlanResult
{
	Edemo_mapShanmenFormationScatterBatchPlanStatus Status =
		Edemo_mapShanmenFormationScatterBatchPlanStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterBatchIntent Batch;

	bool IsValid() const;
	bool IsPlanned() const;
};

/**
 * Pure planner from P27.16 authorization to a canonical whole-deployment
 * command intent. It allocates no inventory, commits no anchor, and owns no
 * World, input, timing, retry, persistence, presentation or product state.
 */
struct Fdemo_mapShanmenFormationScatterBatchPlanner
{
	static Fdemo_mapShanmenFormationScatterBatchPlanResult Plan(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
			Authorization);

	static bool IsCurrentBatch(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenFormationScatterBatchIntent& Batch);
};
