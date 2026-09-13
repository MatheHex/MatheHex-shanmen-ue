#pragma once

#include "CoreMinimal.h"
#include "ShanmenFormationDeployment.h"
#include "demo_mapShanmenFormationMasteryAuthorityAdapter.h"

/** Exact target shape for one mastery-gated material-delivery operation. */
enum class Edemo_mapShanmenFormationMasteryOperationTarget : uint8
{
	Invalid,
	Anchor,
	Deployment
};

/**
 * Immutable proof that one mastery read authorizes one exact operation against
 * one exact formation deployment snapshot.
 */
class Fdemo_mapShanmenFormationMasteryOperationAuthorization
{
public:
	bool IsValid() const;
	const FGuid& GetAuthorizationId() const { return AuthorizationId; }
	const FGuid& GetOperationId() const { return OperationId; }
	const FGuid& GetMasteryReadId() const { return MasteryReadId; }
	int64 GetMasteryAuthorityRevision() const
	{
		return MasteryAuthorityRevision;
	}
	EShanmenFormationMasteryTier GetMasteryTier() const
	{
		return MasteryTier;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	const FGuid& GetActivationId() const { return ActivationId; }
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	FName GetDiagramDefinitionId() const { return DiagramDefinitionId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	EShanmenFormationMaterialDeliveryMode GetDeliveryMode() const
	{
		return DeliveryMode;
	}
	Edemo_mapShanmenFormationMasteryOperationTarget GetTarget() const
	{
		return Target;
	}
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetAnchorInstanceId() const { return AnchorInstanceId; }
	EShanmenFormationDeploymentState GetDeploymentState() const
	{
		return DeploymentState;
	}
	int32 GetDeploymentReceiptCount() const
	{
		return DeploymentReceiptCount;
	}
	int32 GetCommittedAnchorCount() const
	{
		return CommittedAnchorCount;
	}
	int32 GetTotalAnchorCount() const { return TotalAnchorCount; }

	bool operator==(
		const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationMasteryOperationAuthorizer;

	static FGuid BuildAuthorizationId(
		const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
			Authorization);

	FGuid AuthorizationId;
	FGuid OperationId;
	FGuid MasteryReadId;
	int64 MasteryAuthorityRevision = INDEX_NONE;
	EShanmenFormationMasteryTier MasteryTier =
		EShanmenFormationMasteryTier::Invalid;
	FGuid RunId;
	FGuid OwnerId;
	FGuid ActivationId;
	FGuid DeploymentId;
	FName DiagramDefinitionId = NAME_None;
	FShanmenContentStamp Content;
	EShanmenFormationMaterialDeliveryMode DeliveryMode =
		EShanmenFormationMaterialDeliveryMode::Invalid;
	Edemo_mapShanmenFormationMasteryOperationTarget Target =
		Edemo_mapShanmenFormationMasteryOperationTarget::Invalid;
	FName AnchorDefinitionId = NAME_None;
	FGuid AnchorInstanceId;
	EShanmenFormationDeploymentState DeploymentState =
		EShanmenFormationDeploymentState::Uninitialized;
	int32 DeploymentReceiptCount = 0;
	int32 CommittedAnchorCount = 0;
	int32 TotalAnchorCount = 0;
};

enum class Edemo_mapShanmenFormationMasteryOperationAuthorizationStatus : uint8
{
	Invalid,
	Authorized,
	MasteryProjectionRejected,
	DeploymentInvalid,
	DeploymentNotAcceptingMaterials,
	OwnerMismatch,
	ContentMismatch,
	OperationIdentityInvalid,
	DeliveryModeInvalid,
	CapabilityDenied,
	TargetShapeInvalid,
	AnchorUnavailable,
	AnchorAlreadyCommitted,
	ScatterRequiresFreshDeployment
};

/** Auditable result for one bounded, side-effect-free authorization attempt. */
struct Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult
{
	Edemo_mapShanmenFormationMasteryOperationAuthorizationStatus Status =
		Edemo_mapShanmenFormationMasteryOperationAuthorizationStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationMasteryOperationAuthorization Authorization;

	bool IsValid() const;
	bool IsAuthorized() const;
};

/**
 * Stateless product seam from P27.15 mastery evidence to one P27.14 operation.
 *
 * Proximity and remote delivery target one uncommitted anchor. Scatter targets
 * the whole deployment. The authorizer owns no mastery source, input, distance,
 * timing, material mutation, World execution, cache, retry or product state.
 */
struct Fdemo_mapShanmenFormationMasteryOperationAuthorizer
{
	static Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult
	Authorize(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const FGuid& OperationId,
		EShanmenFormationMaterialDeliveryMode DeliveryMode,
		FName AnchorDefinitionId = NAME_None);

	static bool IsCurrentAuthorization(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const FGuid& OperationId,
		EShanmenFormationMaterialDeliveryMode DeliveryMode,
		FName AnchorDefinitionId,
		const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
			Authorization);
};
