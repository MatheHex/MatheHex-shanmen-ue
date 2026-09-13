#include "demo_mapShanmenFormationMasteryOperationAuthorization.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EAuthorizationStatus =
		Edemo_mapShanmenFormationMasteryOperationAuthorizationStatus;
	using EOperationTarget =
		Edemo_mapShanmenFormationMasteryOperationTarget;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString CanonicalName(const FName Value)
	{
		FString Result = Value.ToString();
		Result.ToLowerInline();
		return Result;
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	const FShanmenFormationAnchorProgress* FindAnchorProgress(
		const FShanmenFormationDeployment& Deployment,
		const FName AnchorDefinitionId)
	{
		for (const FShanmenFormationAnchorProgress& Anchor :
			Deployment.GetAnchors())
		{
			if (Anchor.GetAnchorDefinitionId() == AnchorDefinitionId)
			{
				return &Anchor;
			}
		}
		return nullptr;
	}

	Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult Reject(
		const EAuthorizationStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

FGuid Fdemo_mapShanmenFormationMasteryOperationAuthorization::
	BuildAuthorizationId(
		const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
			Authorization)
{
	if (!Authorization.OperationId.IsValid()
		|| !Authorization.MasteryReadId.IsValid()
		|| Authorization.MasteryAuthorityRevision < 0
		|| !FShanmenFormationMasteryPolicy::IsTierValid(
			Authorization.MasteryTier)
		|| !Authorization.RunId.IsValid()
		|| !Authorization.OwnerId.IsValid()
		|| !Authorization.ActivationId.IsValid()
		|| !Authorization.DeploymentId.IsValid()
		|| Authorization.DiagramDefinitionId.IsNone()
		|| !Authorization.Content.IsValid()
		|| !FShanmenFormationMasteryPolicy::IsDeliveryModeValid(
			Authorization.DeliveryMode)
		|| Authorization.DeploymentState
			!= EShanmenFormationDeploymentState::Deploying
		|| Authorization.DeploymentReceiptCount <= 0
		|| Authorization.TotalAnchorCount <= 0
		|| Authorization.CommittedAnchorCount < 0
		|| Authorization.CommittedAnchorCount
			>= Authorization.TotalAnchorCount)
	{
		return FGuid();
	}

	const bool bAnchorTarget =
		Authorization.Target == EOperationTarget::Anchor
		&& (Authorization.DeliveryMode
				== EShanmenFormationMaterialDeliveryMode::ProximityFill
			|| Authorization.DeliveryMode
				== EShanmenFormationMaterialDeliveryMode::RemoteThrow)
		&& !Authorization.AnchorDefinitionId.IsNone()
		&& Authorization.AnchorInstanceId.IsValid();
	const bool bDeploymentTarget =
		Authorization.Target == EOperationTarget::Deployment
		&& Authorization.DeliveryMode
			== EShanmenFormationMaterialDeliveryMode::ScatterFormation
		&& Authorization.AnchorDefinitionId.IsNone()
		&& !Authorization.AnchorInstanceId.IsValid()
		&& Authorization.CommittedAnchorCount == 0;
	if (!bAnchorTarget && !bDeploymentTarget)
	{
		return FGuid();
	}

	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.MasteryOperationAuthorization.r1"),
		{
			GuidDigits(Authorization.OperationId),
			GuidDigits(Authorization.MasteryReadId),
			LexToString(Authorization.MasteryAuthorityRevision),
			FString::FromInt(
				static_cast<int32>(Authorization.MasteryTier)),
			GuidDigits(Authorization.RunId),
			GuidDigits(Authorization.OwnerId),
			GuidDigits(Authorization.ActivationId),
			GuidDigits(Authorization.DeploymentId),
			CanonicalName(Authorization.DiagramDefinitionId),
			CanonicalName(Authorization.Content.Version),
			Authorization.Content.Digest,
			FString::FromInt(
				static_cast<int32>(Authorization.DeliveryMode)),
			FString::FromInt(static_cast<int32>(Authorization.Target)),
			CanonicalName(Authorization.AnchorDefinitionId),
			GuidDigits(Authorization.AnchorInstanceId),
			FString::FromInt(
				static_cast<int32>(Authorization.DeploymentState)),
			FString::FromInt(Authorization.DeploymentReceiptCount),
			FString::FromInt(Authorization.CommittedAnchorCount),
			FString::FromInt(Authorization.TotalAnchorCount)
		});
}

bool Fdemo_mapShanmenFormationMasteryOperationAuthorization::IsValid() const
{
	return AuthorizationId.IsValid()
		&& AuthorizationId == BuildAuthorizationId(*this);
}

bool Fdemo_mapShanmenFormationMasteryOperationAuthorization::operator==(
	const Fdemo_mapShanmenFormationMasteryOperationAuthorization& Other) const
{
	return AuthorizationId == Other.AuthorizationId
		&& OperationId == Other.OperationId
		&& MasteryReadId == Other.MasteryReadId
		&& MasteryAuthorityRevision == Other.MasteryAuthorityRevision
		&& MasteryTier == Other.MasteryTier
		&& RunId == Other.RunId
		&& OwnerId == Other.OwnerId
		&& ActivationId == Other.ActivationId
		&& DeploymentId == Other.DeploymentId
		&& DiagramDefinitionId == Other.DiagramDefinitionId
		&& ContentMatches(Content, Other.Content)
		&& DeliveryMode == Other.DeliveryMode
		&& Target == Other.Target
		&& AnchorDefinitionId == Other.AnchorDefinitionId
		&& AnchorInstanceId == Other.AnchorInstanceId
		&& DeploymentState == Other.DeploymentState
		&& DeploymentReceiptCount == Other.DeploymentReceiptCount
		&& CommittedAnchorCount == Other.CommittedAnchorCount
		&& TotalAnchorCount == Other.TotalAnchorCount;
}

bool Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult::
	IsValid() const
{
	if (Status == EAuthorizationStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	return Status == EAuthorizationStatus::Authorized
		? Authorization.IsValid()
		: !Authorization.IsValid();
}

bool Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult::
	IsAuthorized() const
{
	return Status == EAuthorizationStatus::Authorized && IsValid();
}

Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult
Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const FGuid& OperationId,
	const EShanmenFormationMaterialDeliveryMode DeliveryMode,
	const FName AnchorDefinitionId)
{
	if (!Projection.IsProjected())
	{
		return Reject(
			EAuthorizationStatus::MasteryProjectionRejected,
			TEXT("Formation material delivery requires a projected mastery authority read."));
	}
	if (!Deployment.IsValid())
	{
		return Reject(
			EAuthorizationStatus::DeploymentInvalid,
			TEXT("Formation material delivery requires one valid deployment."));
	}
	if (Deployment.GetState()
		!= EShanmenFormationDeploymentState::Deploying)
	{
		return Reject(
			EAuthorizationStatus::DeploymentNotAcceptingMaterials,
			TEXT("Formation material delivery requires a deployment that is accepting materials."));
	}

	const FShanmenCombatActionSnapshot& Action = Deployment.GetAction();
	const Fdemo_mapShanmenFormationMasteryAuthorityRead& MasteryRead =
		Projection.AuthorityRead;
	if (Action.GetOwnerId() != MasteryRead.GetOwnerId())
	{
		return Reject(
			EAuthorizationStatus::OwnerMismatch,
			TEXT("Formation mastery evidence belongs to another deployment owner."));
	}
	if (!ContentMatches(Action.GetContent(), MasteryRead.GetContent()))
	{
		return Reject(
			EAuthorizationStatus::ContentMismatch,
			TEXT("Formation mastery evidence does not match deployment content."));
	}
	if (!OperationId.IsValid())
	{
		return Reject(
			EAuthorizationStatus::OperationIdentityInvalid,
			TEXT("Formation material delivery requires one stable operation identity."));
	}
	if (!FShanmenFormationMasteryPolicy::IsDeliveryModeValid(DeliveryMode))
	{
		return Reject(
			EAuthorizationStatus::DeliveryModeInvalid,
			TEXT("Formation material delivery requested an invalid operation mode."));
	}
	if (!MasteryRead.GetMasteryPolicy().CanUseDeliveryMode(DeliveryMode))
	{
		return Reject(
			EAuthorizationStatus::CapabilityDenied,
			TEXT("Formation mastery does not authorize the requested delivery mode."));
	}

	const bool bScatter = DeliveryMode
		== EShanmenFormationMaterialDeliveryMode::ScatterFormation;
	if ((bScatter && !AnchorDefinitionId.IsNone())
		|| (!bScatter && AnchorDefinitionId.IsNone()))
	{
		return Reject(
			EAuthorizationStatus::TargetShapeInvalid,
			TEXT("Formation delivery target does not match the requested operation mode."));
	}
	if (bScatter && Deployment.GetCommittedAnchorCount() != 0)
	{
		return Reject(
			EAuthorizationStatus::ScatterRequiresFreshDeployment,
			TEXT("Scatter formation is one whole-deployment operation and requires zero committed anchors."));
	}

	const FShanmenFormationAnchorProgress* Anchor = nullptr;
	if (!bScatter)
	{
		Anchor = FindAnchorProgress(Deployment, AnchorDefinitionId);
		if (!Anchor)
		{
			return Reject(
				EAuthorizationStatus::AnchorUnavailable,
				TEXT("Formation material delivery requires an anchor in the exact deployment."));
		}
		if (Anchor->IsCommitted())
		{
			return Reject(
				EAuthorizationStatus::AnchorAlreadyCommitted,
				TEXT("Formation material delivery cannot target an already committed anchor."));
		}
	}

	Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult Result;
	Result.Status = EAuthorizationStatus::Authorized;
	Result.Diagnostic =
		TEXT("Formation mastery authorized one exact material-delivery operation.");
	Result.Authorization.OperationId = OperationId;
	Result.Authorization.MasteryReadId = MasteryRead.GetReadId();
	Result.Authorization.MasteryAuthorityRevision =
		MasteryRead.GetAuthorityRevision();
	Result.Authorization.MasteryTier =
		MasteryRead.GetMasteryPolicy().GetTier();
	Result.Authorization.RunId = Action.GetRunId();
	Result.Authorization.OwnerId = Action.GetOwnerId();
	Result.Authorization.ActivationId = Action.GetActivationId();
	Result.Authorization.DeploymentId = Deployment.GetDeploymentId();
	Result.Authorization.DiagramDefinitionId =
		Deployment.GetDiagram().GetDiagramDefinitionId();
	Result.Authorization.Content = Action.GetContent();
	Result.Authorization.DeliveryMode = DeliveryMode;
	Result.Authorization.Target = bScatter
		? EOperationTarget::Deployment
		: EOperationTarget::Anchor;
	Result.Authorization.AnchorDefinitionId = AnchorDefinitionId;
	Result.Authorization.AnchorInstanceId = Anchor
		? Anchor->GetAnchorInstanceId()
		: FGuid();
	Result.Authorization.DeploymentState = Deployment.GetState();
	Result.Authorization.DeploymentReceiptCount =
		Deployment.GetReceipts().Num();
	Result.Authorization.CommittedAnchorCount =
		Deployment.GetCommittedAnchorCount();
	Result.Authorization.TotalAnchorCount = Deployment.GetAnchors().Num();
	Result.Authorization.AuthorizationId =
		Fdemo_mapShanmenFormationMasteryOperationAuthorization::
			BuildAuthorizationId(Result.Authorization);
	if (!Result.Authorization.IsValid())
	{
		return Reject(
			EAuthorizationStatus::DeploymentInvalid,
			TEXT("Formation deployment could not produce immutable mastery authorization."));
	}
	return Result;
}

bool Fdemo_mapShanmenFormationMasteryOperationAuthorizer::
	IsCurrentAuthorization(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const FGuid& OperationId,
		const EShanmenFormationMaterialDeliveryMode DeliveryMode,
		const FName AnchorDefinitionId,
		const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
			Authorization)
{
	const auto Current = Authorize(
		Projection,
		Deployment,
		OperationId,
		DeliveryMode,
		AnchorDefinitionId);
	return Authorization.IsValid()
		&& Current.IsAuthorized()
		&& Current.Authorization == Authorization;
}
