#include "demo_mapShanmenFormationScatterBatchIntent.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EPlanStatus = Edemo_mapShanmenFormationScatterBatchPlanStatus;
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

	double CanonicalZero(const double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	Fdemo_mapShanmenFormationScatterBatchPlanResult Reject(
		const EPlanStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterBatchPlanResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

FGuid Fdemo_mapShanmenFormationScatterMaterialIntent::BuildIntentId(
	const Fdemo_mapShanmenFormationScatterMaterialIntent& Intent)
{
	if (!Intent.AuthorizationId.IsValid()
		|| !Intent.OperationId.IsValid()
		|| !Intent.DeploymentId.IsValid()
		|| Intent.AnchorOrder < 0
		|| Intent.AnchorDefinitionId.IsNone()
		|| !Intent.AnchorInstanceId.IsValid()
		|| Intent.RequirementOrder < 0
		|| Intent.MaterialDefinitionId.IsNone()
		|| Intent.Quantity <= 0)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterMaterialIntent.r1"),
		{
			GuidDigits(Intent.AuthorizationId),
			GuidDigits(Intent.OperationId),
			GuidDigits(Intent.DeploymentId),
			FString::FromInt(Intent.AnchorOrder),
			CanonicalName(Intent.AnchorDefinitionId),
			GuidDigits(Intent.AnchorInstanceId),
			FString::FromInt(Intent.RequirementOrder),
			CanonicalName(Intent.MaterialDefinitionId),
			FString::FromInt(Intent.Quantity)
		});
}

bool Fdemo_mapShanmenFormationScatterMaterialIntent::IsValid() const
{
	return IntentId.IsValid() && IntentId == BuildIntentId(*this);
}

bool Fdemo_mapShanmenFormationScatterMaterialIntent::operator==(
	const Fdemo_mapShanmenFormationScatterMaterialIntent& Other) const
{
	return IntentId == Other.IntentId
		&& AuthorizationId == Other.AuthorizationId
		&& OperationId == Other.OperationId
		&& DeploymentId == Other.DeploymentId
		&& AnchorOrder == Other.AnchorOrder
		&& AnchorDefinitionId == Other.AnchorDefinitionId
		&& AnchorInstanceId == Other.AnchorInstanceId
		&& RequirementOrder == Other.RequirementOrder
		&& MaterialDefinitionId == Other.MaterialDefinitionId
		&& Quantity == Other.Quantity;
}

FGuid Fdemo_mapShanmenFormationScatterAnchorIntent::BuildIntentId(
	const Fdemo_mapShanmenFormationScatterAnchorIntent& Intent)
{
	if (!Intent.AuthorizationId.IsValid()
		|| !Intent.OperationId.IsValid()
		|| !Intent.DeploymentId.IsValid()
		|| Intent.AnchorOrder < 0
		|| Intent.AnchorDefinitionId.IsNone()
		|| !Intent.AnchorInstanceId.IsValid()
		|| !IsFiniteVector(Intent.WorldLocation)
		|| Intent.MaterialIntents.IsEmpty()
		|| Intent.TotalMaterialQuantity <= 0)
	{
		return FGuid();
	}

	TArray<FString> Parts = {
		GuidDigits(Intent.AuthorizationId),
		GuidDigits(Intent.OperationId),
		GuidDigits(Intent.DeploymentId),
		FString::FromInt(Intent.AnchorOrder),
		CanonicalName(Intent.AnchorDefinitionId),
		GuidDigits(Intent.AnchorInstanceId),
		DoubleBits(Intent.WorldLocation.X),
		DoubleBits(Intent.WorldLocation.Y),
		DoubleBits(Intent.WorldLocation.Z),
		LexToString(Intent.TotalMaterialQuantity),
		FString::FromInt(Intent.MaterialIntents.Num())
	};
	for (const Fdemo_mapShanmenFormationScatterMaterialIntent& Material :
		Intent.MaterialIntents)
	{
		if (!Material.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Material.GetIntentId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterAnchorIntent.r1"), Parts);
}

bool Fdemo_mapShanmenFormationScatterAnchorIntent::IsValid() const
{
	if (!IntentId.IsValid()
		|| !AuthorizationId.IsValid()
		|| !OperationId.IsValid()
		|| !DeploymentId.IsValid()
		|| AnchorOrder < 0
		|| AnchorDefinitionId.IsNone()
		|| !AnchorInstanceId.IsValid()
		|| !IsFiniteVector(WorldLocation)
		|| MaterialIntents.IsEmpty()
		|| TotalMaterialQuantity <= 0)
	{
		return false;
	}

	int64 QuantitySum = 0;
	TSet<FGuid> IntentIds;
	TSet<FName> MaterialIds;
	for (int32 Index = 0; Index < MaterialIntents.Num(); ++Index)
	{
		const Fdemo_mapShanmenFormationScatterMaterialIntent& Material =
			MaterialIntents[Index];
		if (!Material.IsValid()
			|| Material.GetAuthorizationId() != AuthorizationId
			|| Material.GetOperationId() != OperationId
			|| Material.GetDeploymentId() != DeploymentId
			|| Material.GetAnchorOrder() != AnchorOrder
			|| Material.GetAnchorDefinitionId() != AnchorDefinitionId
			|| Material.GetAnchorInstanceId() != AnchorInstanceId
			|| Material.GetRequirementOrder() != Index
			|| IntentIds.Contains(Material.GetIntentId())
			|| MaterialIds.Contains(Material.GetMaterialDefinitionId()))
		{
			return false;
		}
		IntentIds.Add(Material.GetIntentId());
		MaterialIds.Add(Material.GetMaterialDefinitionId());
		QuantitySum += Material.GetQuantity();
	}
	return QuantitySum == TotalMaterialQuantity
		&& IntentId == BuildIntentId(*this);
}

bool Fdemo_mapShanmenFormationScatterAnchorIntent::operator==(
	const Fdemo_mapShanmenFormationScatterAnchorIntent& Other) const
{
	return IntentId == Other.IntentId
		&& AuthorizationId == Other.AuthorizationId
		&& OperationId == Other.OperationId
		&& DeploymentId == Other.DeploymentId
		&& AnchorOrder == Other.AnchorOrder
		&& AnchorDefinitionId == Other.AnchorDefinitionId
		&& AnchorInstanceId == Other.AnchorInstanceId
		&& WorldLocation == Other.WorldLocation
		&& MaterialIntents == Other.MaterialIntents
		&& TotalMaterialQuantity == Other.TotalMaterialQuantity;
}

FGuid Fdemo_mapShanmenFormationScatterBatchIntent::BuildBatchIntentId(
	const Fdemo_mapShanmenFormationScatterBatchIntent& Intent)
{
	if (!Intent.Authorization.IsValid()
		|| Intent.Authorization.GetDeliveryMode()
			!= EShanmenFormationMaterialDeliveryMode::ScatterFormation
		|| Intent.Authorization.GetTarget()
			!= EOperationTarget::Deployment
		|| Intent.AnchorIntents.IsEmpty()
		|| Intent.TotalRequirementCount <= 0
		|| Intent.TotalMaterialQuantity <= 0)
	{
		return FGuid();
	}
	FShanmenFormationMasteryPolicy Policy;
	if (!FShanmenFormationMasteryPolicy::TryCreate(
			Intent.Authorization.GetMasteryTier(), Policy)
		|| !Policy.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ScatterFormation))
	{
		return FGuid();
	}

	TArray<FString> Parts = {
		GuidDigits(Intent.Authorization.GetAuthorizationId()),
		GuidDigits(Intent.Authorization.GetOperationId()),
		GuidDigits(Intent.Authorization.GetDeploymentId()),
		FString::FromInt(Intent.TotalRequirementCount),
		LexToString(Intent.TotalMaterialQuantity),
		FString::FromInt(Intent.AnchorIntents.Num())
	};
	for (const Fdemo_mapShanmenFormationScatterAnchorIntent& Anchor :
		Intent.AnchorIntents)
	{
		if (!Anchor.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Anchor.GetIntentId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterBatchIntent.r1"), Parts);
}

bool Fdemo_mapShanmenFormationScatterBatchIntent::IsValid() const
{
	if (!BatchIntentId.IsValid()
		|| !Authorization.IsValid()
		|| Authorization.GetDeliveryMode()
			!= EShanmenFormationMaterialDeliveryMode::ScatterFormation
		|| Authorization.GetTarget() != EOperationTarget::Deployment
		|| !Authorization.GetAnchorDefinitionId().IsNone()
		|| Authorization.GetAnchorInstanceId().IsValid()
		|| Authorization.GetCommittedAnchorCount() != 0
		|| AnchorIntents.Num() != Authorization.GetTotalAnchorCount()
		|| TotalRequirementCount <= 0
		|| TotalMaterialQuantity <= 0)
	{
		return false;
	}

	int32 RequirementCount = 0;
	int64 QuantitySum = 0;
	TSet<FGuid> IntentIds;
	TSet<FName> AnchorDefinitionIds;
	TSet<FGuid> AnchorInstanceIds;
	for (int32 Index = 0; Index < AnchorIntents.Num(); ++Index)
	{
		const Fdemo_mapShanmenFormationScatterAnchorIntent& Anchor =
			AnchorIntents[Index];
		if (!Anchor.IsValid()
			|| Anchor.GetAuthorizationId()
				!= Authorization.GetAuthorizationId()
			|| Anchor.GetOperationId() != Authorization.GetOperationId()
			|| Anchor.GetDeploymentId() != Authorization.GetDeploymentId()
			|| Anchor.GetAnchorOrder() != Index
			|| IntentIds.Contains(Anchor.GetIntentId())
			|| AnchorDefinitionIds.Contains(Anchor.GetAnchorDefinitionId())
			|| AnchorInstanceIds.Contains(Anchor.GetAnchorInstanceId()))
		{
			return false;
		}
		IntentIds.Add(Anchor.GetIntentId());
		AnchorDefinitionIds.Add(Anchor.GetAnchorDefinitionId());
		AnchorInstanceIds.Add(Anchor.GetAnchorInstanceId());
		RequirementCount += Anchor.GetMaterialIntents().Num();
		QuantitySum += Anchor.GetTotalMaterialQuantity();
	}
	return RequirementCount == TotalRequirementCount
		&& QuantitySum == TotalMaterialQuantity
		&& BatchIntentId == BuildBatchIntentId(*this);
}

bool Fdemo_mapShanmenFormationScatterBatchIntent::operator==(
	const Fdemo_mapShanmenFormationScatterBatchIntent& Other) const
{
	return BatchIntentId == Other.BatchIntentId
		&& Authorization == Other.Authorization
		&& AnchorIntents == Other.AnchorIntents
		&& TotalRequirementCount == Other.TotalRequirementCount
		&& TotalMaterialQuantity == Other.TotalMaterialQuantity;
}

bool Fdemo_mapShanmenFormationScatterBatchPlanResult::IsValid() const
{
	if (Status == EPlanStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	return Status == EPlanStatus::Planned
		? Batch.IsValid()
		: !Batch.IsValid();
}

bool Fdemo_mapShanmenFormationScatterBatchPlanResult::IsPlanned() const
{
	return Status == EPlanStatus::Planned && IsValid();
}

Fdemo_mapShanmenFormationScatterBatchPlanResult
Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenFormationMasteryOperationAuthorization&
		Authorization)
{
	if (!Authorization.IsValid())
	{
		return Reject(
			EPlanStatus::AuthorizationInvalid,
			TEXT("Formation scatter planning requires one valid operation authorization."));
	}
	if (Authorization.GetDeliveryMode()
			!= EShanmenFormationMaterialDeliveryMode::ScatterFormation
		|| Authorization.GetTarget() != EOperationTarget::Deployment)
	{
		return Reject(
			EPlanStatus::AuthorizationNotScatter,
			TEXT("Formation scatter planning requires a whole-deployment scatter authorization."));
	}
	if (!Projection.IsProjected())
	{
		return Reject(
			EPlanStatus::MasteryProjectionRejected,
			TEXT("Formation scatter planning requires current projected mastery evidence."));
	}
	if (!Deployment.IsValid())
	{
		return Reject(
			EPlanStatus::DeploymentInvalid,
			TEXT("Formation scatter planning requires one valid deployment."));
	}
	if (!Fdemo_mapShanmenFormationMasteryOperationAuthorizer::
			IsCurrentAuthorization(
				Projection,
				Deployment,
				Authorization.GetOperationId(),
				EShanmenFormationMaterialDeliveryMode::ScatterFormation,
				NAME_None,
				Authorization))
	{
		return Reject(
			EPlanStatus::AuthorizationStale,
			TEXT("Formation scatter authorization is stale for current mastery or deployment evidence."));
	}

	const TArray<FShanmenFormationAnchorDefinition>& Definitions =
		Deployment.GetDiagram().GetAnchors();
	const TArray<FShanmenFormationAnchorProgress>& Progress =
		Deployment.GetAnchors();
	if (Definitions.IsEmpty()
		|| Definitions.Num() != Progress.Num()
		|| Definitions.Num() != Authorization.GetTotalAnchorCount())
	{
		return Reject(
			EPlanStatus::DeploymentShapeInvalid,
			TEXT("Formation scatter deployment shape does not match its authorization."));
	}

	Fdemo_mapShanmenFormationScatterBatchPlanResult Result;
	Result.Batch.Authorization = Authorization;
	Result.Batch.AnchorIntents.Reserve(Definitions.Num());
	for (int32 AnchorIndex = 0; AnchorIndex < Definitions.Num(); ++AnchorIndex)
	{
		const FShanmenFormationAnchorDefinition& Definition =
			Definitions[AnchorIndex];
		const FShanmenFormationAnchorProgress& AnchorProgress =
			Progress[AnchorIndex];
		if (!Definition.IsValid()
			|| Definition.GetOrder() != AnchorIndex
			|| AnchorProgress.IsCommitted()
			|| AnchorProgress.GetAnchorDefinitionId()
				!= Definition.GetAnchorDefinitionId()
			|| !AnchorProgress.GetAnchorInstanceId().IsValid()
			|| !IsFiniteVector(AnchorProgress.GetWorldLocation()))
		{
			return Reject(
				EPlanStatus::DeploymentShapeInvalid,
				TEXT("Formation scatter encountered a mismatched deployed anchor."));
		}

		Fdemo_mapShanmenFormationScatterAnchorIntent AnchorIntent;
		AnchorIntent.AuthorizationId = Authorization.GetAuthorizationId();
		AnchorIntent.OperationId = Authorization.GetOperationId();
		AnchorIntent.DeploymentId = Authorization.GetDeploymentId();
		AnchorIntent.AnchorOrder = AnchorIndex;
		AnchorIntent.AnchorDefinitionId = Definition.GetAnchorDefinitionId();
		AnchorIntent.AnchorInstanceId = AnchorProgress.GetAnchorInstanceId();
		AnchorIntent.WorldLocation = AnchorProgress.GetWorldLocation();
		AnchorIntent.MaterialIntents.Reserve(
			Definition.GetRequirements().Num());
		for (int32 RequirementIndex = 0;
			RequirementIndex < Definition.GetRequirements().Num();
			++RequirementIndex)
		{
			const FShanmenFormationMaterialRequirement& Requirement =
				Definition.GetRequirements()[RequirementIndex];
			if (!Requirement.IsValid()
				|| Requirement.GetOrder() != RequirementIndex)
			{
				return Reject(
					EPlanStatus::RequirementInvalid,
					TEXT("Formation scatter encountered an invalid authored material requirement."));
			}

			Fdemo_mapShanmenFormationScatterMaterialIntent MaterialIntent;
			MaterialIntent.AuthorizationId =
				Authorization.GetAuthorizationId();
			MaterialIntent.OperationId = Authorization.GetOperationId();
			MaterialIntent.DeploymentId = Authorization.GetDeploymentId();
			MaterialIntent.AnchorOrder = AnchorIndex;
			MaterialIntent.AnchorDefinitionId =
				Definition.GetAnchorDefinitionId();
			MaterialIntent.AnchorInstanceId =
				AnchorProgress.GetAnchorInstanceId();
			MaterialIntent.RequirementOrder = RequirementIndex;
			MaterialIntent.MaterialDefinitionId =
				Requirement.GetMaterialDefinitionId();
			MaterialIntent.Quantity = Requirement.GetQuantity();
			MaterialIntent.IntentId =
				Fdemo_mapShanmenFormationScatterMaterialIntent::
					BuildIntentId(MaterialIntent);
			if (!MaterialIntent.IsValid())
			{
				return Reject(
					EPlanStatus::IntentInvalid,
					TEXT("Formation scatter material intent failed deterministic validation."));
			}
			AnchorIntent.TotalMaterialQuantity +=
				MaterialIntent.GetQuantity();
			AnchorIntent.MaterialIntents.Add(MaterialIntent);
		}
		AnchorIntent.IntentId =
			Fdemo_mapShanmenFormationScatterAnchorIntent::
				BuildIntentId(AnchorIntent);
		if (!AnchorIntent.IsValid())
		{
			return Reject(
				EPlanStatus::IntentInvalid,
				TEXT("Formation scatter anchor intent failed deterministic validation."));
		}
		Result.Batch.TotalRequirementCount +=
			AnchorIntent.GetMaterialIntents().Num();
		Result.Batch.TotalMaterialQuantity +=
			AnchorIntent.GetTotalMaterialQuantity();
		Result.Batch.AnchorIntents.Add(AnchorIntent);
	}

	Result.Batch.BatchIntentId =
		Fdemo_mapShanmenFormationScatterBatchIntent::
			BuildBatchIntentId(Result.Batch);
	if (!Result.Batch.IsValid())
	{
		return Reject(
			EPlanStatus::IntentInvalid,
			TEXT("Formation scatter batch failed deterministic validation."));
	}
	Result.Status = EPlanStatus::Planned;
	Result.Diagnostic =
		TEXT("Formation scatter authorization expanded into canonical anchor material intents.");
	return Result;
}

bool Fdemo_mapShanmenFormationScatterBatchPlanner::IsCurrentBatch(
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenFormationScatterBatchIntent& Batch)
{
	if (!Batch.IsValid())
	{
		return false;
	}
	const Fdemo_mapShanmenFormationScatterBatchPlanResult Current = Plan(
		Projection, Deployment, Batch.GetAuthorization());
	return Current.IsPlanned() && Current.Batch == Batch;
}
