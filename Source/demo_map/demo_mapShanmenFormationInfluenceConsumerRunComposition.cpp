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

bool Fdemo_mapShanmenFormationInfluenceConsumerRunDeactivationResult::
	IsSuccess() const
{
	if (!RunId.IsValid()
		|| !bActivationEvidenceChecked
		|| !bDeliveryAttempted
		|| !Activation.IsSuccess()
		|| Activation.RunId != RunId
		|| !Application.IsSuccess()
		|| !Activation.Application.Delivery.Matches(Application.Delivery)
		|| Activation.Application.Runtime.RuntimeId
			!= Application.Runtime.RuntimeId)
	{
		return false;
	}

	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
		Deactivated:
		return Application.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				Deactivated;
	case Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
		DeactivationReplayed:
		return Application.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				DeactivationReplayed;
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

Fdemo_mapShanmenFormationInfluenceConsumerRunDeactivationResult
Fdemo_mapShanmenFormationInfluenceConsumerRunComposition::TryDeactivate(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost& LifecycleHost,
	const Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult&
		ActivationEvidence)
{
	Fdemo_mapShanmenFormationInfluenceConsumerRunDeactivationResult Result;
	Result.RunId = ActivationEvidence.RunId;
	Result.Activation = ActivationEvidence;
	Result.bActivationEvidenceChecked = true;
	if (!ActivationEvidence.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				ActivationEvidenceRejected;
		Result.Diagnostic =
			TEXT("Consumer deactivation requires one successful Run activation receipt.");
		return Result;
	}

	Result.Application = LifecycleHost.TryDeactivateConsumerDelivery(
		ProductHost,
		ActivationEvidence.Application.Delivery);
	Result.bDeliveryAttempted = true;
	if (!Result.Application.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				DeliveryApplicationRejected;
		Result.Diagnostic = Result.Application.Diagnostic.IsEmpty()
			? TEXT("Lifecycle Host rejected exact consumer delivery removal.")
			: Result.Application.Diagnostic;
		return Result;
	}

	switch (Result.Application.Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
		Deactivated:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				Deactivated;
		break;
	case Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
		DeactivationReplayed:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				DeactivationReplayed;
		break;
	default:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer deactivation returned an unexpected success status.");
		return Result;
	}
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
			Deactivated
		? TEXT("Removed the exact delivery proven by the Run activation receipt.")
		: TEXT("Exact Run delivery removal replayed without duplicate mutation.");
	if (!Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer Run deactivation produced inconsistent nested evidence.");
	}
	return Result;
}
