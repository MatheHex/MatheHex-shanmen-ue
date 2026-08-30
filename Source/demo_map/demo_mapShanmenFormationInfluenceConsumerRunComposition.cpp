#include "demo_mapShanmenFormationInfluenceConsumerRunComposition.h"

#include "demo_mapAttributeComponent.h"

bool Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult::
	IsSuccess() const
{
	if (!RunId.IsValid()
		|| !bAliasChecked
		|| !bWorldResolutionChecked
		|| !bDeliveryAttempted
		|| !Alias.IsSuccess()
		|| !WorldResolution.IsSuccess()
		|| !Application.IsSuccess()
		|| Alias.RunId != RunId
		|| WorldResolution.ExpectedRunId != RunId
		|| WorldResolution.RegistryRunId != RunId
		|| Alias.EntityId != WorldResolution.ResolvedEntityId
		|| Alias.EntityId != Application.Delivery.SubjectEntityId
		|| Alias.AliasObjectUniqueId
			!= WorldResolution.Resolution.AttributeComponentUniqueId
		|| Application.SubjectResolution.ResolutionId
			!= WorldResolution.Resolution.ResolutionId
		|| Application.SubjectResolution.AttributeComponentUniqueId
			!= Alias.AliasObjectUniqueId)
	{
		return false;
	}

	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
		Activated:
		return Application.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				Activated;
	case Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
		ActivationReplayed:
		return Application.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				ActivationReplayed;
	default:
		return false;
	}
}

Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult
Fdemo_mapShanmenFormationInfluenceConsumerRunComposition::TryActivate(
	Fdemo_mapCombatRunCoordinator& CombatRun,
	const UObject* RegisteredSubjectObject,
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost& LifecycleHost,
	const Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery& Delivery,
	Udemo_mapAttributeComponent* AttributeComponent,
	const int32 RegisteredBodyIndex,
	const int32 AttributeBodyIndex)
{
	Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult Result;
	Result.RunId = CombatRun.GetRunId();
	Result.Alias = CombatRun.TryBindEntityAlias(
		RegisteredSubjectObject,
		RegisteredBodyIndex,
		AttributeComponent,
		AttributeBodyIndex);
	Result.bAliasChecked = true;
	if (!Result.Alias.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				AliasRejected;
		Result.Diagnostic = Result.Alias.Diagnostic.IsEmpty()
			? TEXT("Combat Run rejected the explicit consumer alias.")
			: Result.Alias.Diagnostic;
		return Result;
	}

	Result.WorldResolution =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			CombatRun.GetEntityRegistry(),
			Result.RunId,
			AttributeComponent,
			AttributeBodyIndex);
	Result.bWorldResolutionChecked = true;
	if (!Result.WorldResolution.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				WorldResolutionRejected;
		Result.Diagnostic = Result.WorldResolution.Diagnostic.IsEmpty()
			? TEXT("Combat Run alias did not produce a valid consumer resolution.")
			: Result.WorldResolution.Diagnostic;
		return Result;
	}
	if (Result.WorldResolution.ResolvedEntityId != Result.Alias.EntityId)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer World resolution disagrees with the committed alias identity.");
		return Result;
	}

	Result.Application = LifecycleHost.TryActivateConsumerDelivery(
		ProductHost,
		Delivery,
		Result.WorldResolution.Resolution,
		AttributeComponent);
	Result.bDeliveryAttempted = true;
	if (!Result.Application.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				DeliveryApplicationRejected;
		Result.Diagnostic = Result.Application.Diagnostic.IsEmpty()
			? TEXT("Lifecycle Host rejected consumer delivery application.")
			: Result.Application.Diagnostic;
		return Result;
	}

	switch (Result.Application.Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
		Activated:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				Activated;
		break;
	case Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
		ActivationReplayed:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				ActivationReplayed;
		break;
	default:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer activation returned an unexpected success status.");
		return Result;
	}
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
			Activated
		? TEXT("Combat Run consumer alias resolved and activated one delivery.")
		: TEXT("Combat Run consumer activation replayed without duplicate mutation.");
	if (!Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer Run composition produced inconsistent nested evidence.");
	}
	return Result;
}
