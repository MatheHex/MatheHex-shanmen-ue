#include "demo_mapShanmenControlledWeaponActiveRunRoute.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "ShanmenControlledWeaponExecution.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	Fdemo_mapShanmenControlledWeaponActiveRunResult Reject(
		const Edemo_mapShanmenControlledWeaponActiveRunError Error,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenControlledWeaponActiveRunResult Result;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenControlledWeaponActiveRunIntent::IsValid() const
{
	FShanmenControlledWeaponDefinition FrozenDefinition;
	FShanmenControlledWeaponOffenseSnapshot FrozenOffense;
	return SourceItemInstanceId.IsValid()
		&& FShanmenControlledWeaponDefinition::TryCapture(
			Definition, FrozenDefinition)
		&& FShanmenControlledWeaponOffenseSnapshot::TryCapture(
			ControlPower, FrozenOffense)
		&& Motion.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponActiveRunResult::IsStarted() const
{
	return Error == Edemo_mapShanmenControlledWeaponActiveRunError::None
		&& Preparation.IsPrepared()
		&& Attachment.IsAttached()
		&& Attachment.ItemInstanceId
			== Preparation.Evidence.ItemInstanceId
		&& Attachment.ActivationId
			== Preparation.Action.GetActivationId();
}

Fdemo_mapShanmenControlledWeaponActiveRunResult
Fdemo_mapShanmenControlledWeaponActiveRunRoute::TryStart(
	const Udemo_mapShanmenItemAuthoritySubsystem* Authority,
	const Udemo_mapItemSubsystem* Runtime,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenControlledWeaponRunHost& Host,
	AActor* SourceActor,
	AActor* WeaponActor,
	UPrimitiveComponent* WeaponCollisionRoot,
	const Fdemo_mapShanmenControlledWeaponActiveRunIntent& Intent)
{
	if (!Authority || !Runtime)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponActiveRunError::
				DependenciesUnavailable,
			TEXT("Controlled-weapon start requires both active item authorities."));
	}
	if (!Intent.IsValid()
		|| !SourceActor
		|| !WeaponActor
		|| SourceActor == WeaponActor
		|| !WeaponCollisionRoot
		|| WeaponCollisionRoot->GetOwner() != WeaponActor
		|| WeaponActor->GetRootComponent() != WeaponCollisionRoot)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponActiveRunError::IntentInvalid,
			TEXT("Controlled-weapon start intent or physical Actor binding is invalid."));
	}
	if (!Coordinator.IsReady())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponActiveRunError::
				CoordinatorNotReady,
			TEXT("Controlled-weapon start requires the canonical active combat Run."));
	}

	FGuid SourceEntityId;
	if (!Coordinator.GetEntityRegistry().TryResolveObject(
			Coordinator.GetRunId(), SourceActor, INDEX_NONE, SourceEntityId)
		|| SourceEntityId != Coordinator.GetPlayerEntityId())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponActiveRunError::SourceMismatch,
			TEXT("Only the canonical player Actor may source a controlled weapon."));
	}

	Fdemo_mapShanmenControlledWeaponActiveRunResult Result;
	Fdemo_mapShanmenControlledWeaponPrepareRequest Request;
	Request.SourceEntityId = SourceEntityId;
	Request.SourceItemInstanceId = Intent.SourceItemInstanceId;
	Request.ActivationSequence = Intent.ActivationSequence;
	Request.Definition = Intent.Definition;
	Request.ControlPower = Intent.ControlPower;
	Request.SourceTags = Intent.SourceTags;
	Result.Preparation =
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareActiveRun(
			*Authority, *Runtime, Request);
	if (!Result.Preparation.IsPrepared())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponActiveRunError::
				PreparationRejected;
		Result.Diagnostic = Result.Preparation.Diagnostic;
		return Result;
	}

	Result.Attachment = Host.TryAttach(
		Result.Preparation,
		Coordinator,
		SourceActor,
		WeaponActor,
		WeaponCollisionRoot,
		Intent.Motion);
	if (!Result.Attachment.IsAttached())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponActiveRunError::
				AttachmentRejected;
		Result.Diagnostic = FString::Printf(
			TEXT("P6 Host rejected the prepared controlled weapon (error %d)."),
			static_cast<int32>(Result.Attachment.Error));
		return Result;
	}

	Result.Error = Edemo_mapShanmenControlledWeaponActiveRunError::None;
	Result.Diagnostic =
		TEXT("The exact deployed flying sword entered the canonical active-Run Host.");
	if (!Result.IsStarted())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponActiveRunError::ResultInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon start failed final cross-boundary validation.");
	}
	return Result;
}
