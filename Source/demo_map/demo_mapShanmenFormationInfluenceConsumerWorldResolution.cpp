#include "demo_mapShanmenFormationInfluenceConsumerWorldResolution.h"

#include "ShanmenWorldEntityRegistry.h"
#include "demo_mapAttributeComponent.h"

bool Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult::
IsSuccess() const
{
	return Status
			== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				Resolved
		&& bRegistryChecked && ExpectedRunId.IsValid()
		&& RegistryRunId == ExpectedRunId && ResolvedEntityId.IsValid()
		&& BodyIndex >= INDEX_NONE && Resolution.IsValid()
		&& Resolution.SubjectEntityId == ResolvedEntityId;
}

Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult
Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
	const FShanmenWorldEntityRegistry& EntityRegistry,
	const FGuid& ExpectedRunId,
	const Udemo_mapAttributeComponent* AttributeComponent,
	const int32 BodyIndex)
{
	Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult Result;
	Result.ExpectedRunId = ExpectedRunId;
	Result.RegistryRunId = EntityRegistry.GetRunId();
	Result.BodyIndex = BodyIndex;

	if (!ExpectedRunId.IsValid())
	{
		Result.Diagnostic = TEXT(
			"Consumer World resolution requires one valid expected Run id.");
		return Result;
	}

	Result.bRegistryChecked = true;
	if (!Result.RegistryRunId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				RegistryUnavailable;
		Result.Diagnostic = TEXT(
			"Consumer World resolution requires one active entity registry.");
		return Result;
	}
	if (Result.RegistryRunId != ExpectedRunId)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				RegistryRunMismatch;
		Result.Diagnostic = TEXT(
			"Consumer World resolution rejected a foreign registry Run.");
		return Result;
	}
	if (!::IsValid(AttributeComponent))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				AttributeComponentUnavailable;
		Result.Diagnostic = TEXT(
			"Consumer World resolution requires one live AttributeComponent.");
		return Result;
	}
	if (BodyIndex < INDEX_NONE)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				BodyIndexInvalid;
		Result.Diagnostic = TEXT(
			"Consumer World resolution received an invalid body index.");
		return Result;
	}

	if (!EntityRegistry.TryResolveObject(
			ExpectedRunId,
			AttributeComponent,
			BodyIndex,
			Result.ResolvedEntityId))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				EntityNotFound;
		Result.Diagnostic = TEXT(
			"AttributeComponent has no matching binding in the entity registry.");
		return Result;
	}

	if (!Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution::
		TryCreate(
			Result.ResolvedEntityId,
			AttributeComponent,
			Result.Resolution))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				ResolutionRejected;
		Result.Diagnostic = TEXT(
			"Registry identity could not produce a consumer subject resolution.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::Resolved;
	Result.Diagnostic = TEXT(
		"AttributeComponent resolved through explicit entity registry binding.");
	if (!Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"Consumer World resolution produced inconsistent evidence.");
	}
	return Result;
}
